// Copyright 2026 Husarion sp. z o.o.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mavlink_serial_transport.hpp"

#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include <stream_buffer.h>

namespace {

constexpr size_t kTxStreamSize = 2048;
constexpr size_t kTxDmaBufSize = 256;
constexpr uint32_t kTxStreamSendTimeoutMs = 5;
constexpr uint32_t kIrqPriority = 5;

struct UartTxDmaMap {
  USART_TypeDef* uart;
  DMA_TypeDef* dma_ctrl;
  DMA_Stream_TypeDef* stream;
  uint32_t channel;
  IRQn_Type irqn;
};

const UartTxDmaMap* findTxMap(HardwareSerial* serial) {
#if defined(USART1_BASE) && defined(ENABLE_HWSERIAL1)
  if (serial == &Serial1) {
    static const UartTxDmaMap kMap = {USART1, DMA2, DMA2_Stream7, DMA_CHANNEL_4,
                                      DMA2_Stream7_IRQn};
    return &kMap;
  }
#endif
#if defined(USART3_BASE) && defined(ENABLE_HWSERIAL3)
  if (serial == &Serial3) {
    static const UartTxDmaMap kMap = {USART3, DMA1, DMA1_Stream4, DMA_CHANNEL_7,
                                      DMA1_Stream4_IRQn};
    return &kMap;
  }
#endif
  return nullptr;
}

StreamBufferHandle_t s_tx_stream = nullptr;
DMA_HandleTypeDef s_hdma_tx = {};
USART_TypeDef* s_uart = nullptr;
const UartTxDmaMap* s_map = nullptr;
alignas(4) uint8_t s_dma_buf[kTxDmaBufSize];
volatile bool s_dma_active = false;

void txCpltCallback(DMA_HandleTypeDef* /*hdma*/) {
  BaseType_t hpw = pdFALSE;
  size_t len =
      xStreamBufferReceiveFromISR(s_tx_stream, s_dma_buf, kTxDmaBufSize, &hpw);
  if (len > 0) {
    HAL_DMA_Start_IT(&s_hdma_tx, reinterpret_cast<uint32_t>(s_dma_buf),
                     reinterpret_cast<uint32_t>(&s_uart->DR), len);
  } else {
    s_dma_active = false;
  }
  portYIELD_FROM_ISR(hpw);
}

}  // namespace

// Strong overrides of lib/ros's weak no-op DMA hooks (shared single binary).
extern "C" void mavlink_serial_dma2_stream7_isr(void) {
  if (s_hdma_tx.Instance == DMA2_Stream7) HAL_DMA_IRQHandler(&s_hdma_tx);
}
extern "C" void mavlink_serial_dma1_stream4_isr(void) {
  if (s_hdma_tx.Instance == DMA1_Stream4) HAL_DMA_IRQHandler(&s_hdma_tx);
}

bool MavlinkSerialTransport::open() {
  if (cfg_.serial == nullptr) return false;

  cfg_.serial->setRx(cfg_.rxPin);
  cfg_.serial->setTx(cfg_.txPin);
  cfg_.serial->setTimeout(cfg_.timeout_ms);
  cfg_.serial->begin(cfg_.baudrate);
  if (!cfg_.serial->operator bool()) return false;

  s_map = findTxMap(cfg_.serial);
  if (s_map == nullptr) return false;
  s_uart = s_map->uart;

  if (s_tx_stream == nullptr) {
    s_tx_stream = xStreamBufferCreate(kTxStreamSize, /*trigger_level=*/1);
    if (s_tx_stream == nullptr) return false;
  } else {
    xStreamBufferReset(s_tx_stream);
  }

  if (s_map->dma_ctrl == DMA1) {
    __HAL_RCC_DMA1_CLK_ENABLE();
  } else {
    __HAL_RCC_DMA2_CLK_ENABLE();
  }

  s_hdma_tx.Instance = s_map->stream;
  s_hdma_tx.Init.Channel = s_map->channel;
  s_hdma_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
  s_hdma_tx.Init.PeriphInc = DMA_PINC_DISABLE;
  s_hdma_tx.Init.MemInc = DMA_MINC_ENABLE;
  s_hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  s_hdma_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  s_hdma_tx.Init.Mode = DMA_NORMAL;
  s_hdma_tx.Init.Priority = DMA_PRIORITY_MEDIUM;
  s_hdma_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

  if (HAL_DMA_Init(&s_hdma_tx) != HAL_OK) return false;
  s_hdma_tx.XferCpltCallback = txCpltCallback;

  HAL_NVIC_SetPriority(s_map->irqn, kIrqPriority, 0);
  HAL_NVIC_EnableIRQ(s_map->irqn);

  SET_BIT(s_uart->CR3, USART_CR3_DMAT);
  s_dma_active = false;
  return true;
}

void MavlinkSerialTransport::close() {
  if (cfg_.serial == nullptr) return;
  if (s_uart != nullptr) CLEAR_BIT(s_uart->CR3, USART_CR3_DMAT);
  if (s_hdma_tx.Instance != nullptr) {
    HAL_DMA_Abort(&s_hdma_tx);
    HAL_DMA_DeInit(&s_hdma_tx);
    s_hdma_tx.Instance = nullptr;
  }
  if (s_map != nullptr) HAL_NVIC_DisableIRQ(s_map->irqn);
  s_dma_active = false;
  s_uart = nullptr;
  s_map = nullptr;
  cfg_.serial->end();
}

size_t MavlinkSerialTransport::write(const uint8_t* buf, size_t len) {
  if (s_tx_stream == nullptr || s_uart == nullptr || s_map == nullptr) return 0;
  if (buf == nullptr || len == 0) return 0;

  const size_t sent = xStreamBufferSend(s_tx_stream, buf, len,
                                        pdMS_TO_TICKS(kTxStreamSendTimeoutMs));

  // The whole check-receive-arm sequence must be atomic vs txCpltCallback
  // and any concurrent writer; otherwise a writer can be stranded with a
  // full stream and s_dma_active=false. The FromISR receive is the only
  // stream-buffer API legal inside taskENTER_CRITICAL.
  taskENTER_CRITICAL();
  if (!s_dma_active) {
    BaseType_t hpw = pdFALSE;
    const size_t to_send = xStreamBufferReceiveFromISR(s_tx_stream, s_dma_buf,
                                                       kTxDmaBufSize, &hpw);
    if (to_send > 0) {
      s_dma_active = true;
      HAL_DMA_Start_IT(&s_hdma_tx, reinterpret_cast<uint32_t>(s_dma_buf),
                       reinterpret_cast<uint32_t>(&s_uart->DR), to_send);
    }
  }
  taskEXIT_CRITICAL();
  return sent;
}

size_t MavlinkSerialTransport::read(uint8_t* buf, size_t len,
                                    uint32_t timeout_ms) {
  if (cfg_.serial == nullptr || buf == nullptr || len == 0) return 0;

  TickType_t ticks_to_wait = (timeout_ms > 0) ? pdMS_TO_TICKS(timeout_ms) : 0;
  TimeOut_t timeout_state;
  vTaskSetTimeOutState(&timeout_state);

  size_t got = 0;
  while (got < len) {
    const int avail = cfg_.serial->available();
    if (avail > 0) {
      size_t to_read = static_cast<size_t>(avail);
      if (to_read > (len - got)) to_read = len - got;
      for (size_t i = 0; i < to_read; ++i) {
        const int b = cfg_.serial->read();
        if (b < 0) break;
        buf[got++] = static_cast<uint8_t>(b);
      }
      continue;
    }
    if (ticks_to_wait == 0) break;
    if (xTaskCheckForTimeOut(&timeout_state, &ticks_to_wait) != pdFALSE) break;
    vTaskDelay(1);
  }
  return got;
}
