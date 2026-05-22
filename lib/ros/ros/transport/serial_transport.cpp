// Copyright 2022 Husarion sp. z o.o.
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

#include "serial_transport.hpp"

#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include <stream_buffer.h>

#include "communication_manager.hpp"

namespace {

constexpr size_t kTxStreamSize = 2048;
constexpr size_t kTxDmaBufSize = 256;
constexpr uint32_t kTxStreamSendTimeoutMs = 5;

// FreeRTOS-safe NVIC priority. configMAX_SYSCALL_INTERRUPT_PRIORITY = 5
// in this project (see STM32FreeRTOSConfig.h); IRQs at priority 5 are
// masked by FreeRTOS critical sections so the active-flag handshake in
// serial_transport_write is race-free with the TC IRQ.
constexpr uint32_t kIrqPriority = 5;

// STM32F4 USART_TX → DMA mapping (RM0090 Table 43). USART3_TX uses the
// alt mapping (Stream 4 Ch7) instead of the primary (Stream 3 Ch4) to
// avoid a duplicate-symbol clash with the DMA1_Stream3_IRQHandler that
// imu_bno055.cpp provides for I2C2_RX on rosbot_xl. lib code is shared,
// so even though rosbot doesn't use I2C2, the handler symbol is in the
// link.
struct UartTxDmaMap {
  USART_TypeDef* uart;
  DMA_TypeDef* dma_ctrl;  // DMA1 or DMA2 — for clock enable
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

// Single-instance state — only one Serial is the active micro-ROS
// transport at a time (CommunicationManager picks one at boot).
StreamBufferHandle_t s_tx_stream = nullptr;
DMA_HandleTypeDef s_hdma_tx = {};
USART_TypeDef* s_uart = nullptr;
const UartTxDmaMap* s_map = nullptr;
alignas(4) uint8_t s_dma_buf[kTxDmaBufSize];
volatile bool s_dma_active = false;

const SerialConfig* getConfig(struct uxrCustomTransport* transport) {
  if (transport == nullptr || transport->args == nullptr) return nullptr;
  return static_cast<const SerialConfig*>(transport->args);
}

void txCpltCallback(DMA_HandleTypeDef* /*hdma*/) {
  // Runs in DMA stream IRQ context. Drain the next chunk from the TX
  // stream buffer; if empty, mark idle so the next write() will kick.
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

// Weak hooks; lib/mavlink supplies the strong overrides when linked.
extern "C" void __attribute__((weak)) mavlink_serial_dma2_stream7_isr(void) {}
extern "C" void __attribute__((weak)) mavlink_serial_dma1_stream4_isr(void) {}

extern "C" void DMA2_Stream7_IRQHandler(void) {
  if (s_hdma_tx.Instance == DMA2_Stream7) HAL_DMA_IRQHandler(&s_hdma_tx);
  mavlink_serial_dma2_stream7_isr();
}
extern "C" void DMA1_Stream4_IRQHandler(void) {
  if (s_hdma_tx.Instance == DMA1_Stream4) HAL_DMA_IRQHandler(&s_hdma_tx);
  mavlink_serial_dma1_stream4_isr();
}

bool serial_transport_open(struct uxrCustomTransport* transport) {
  const SerialConfig* cfg = getConfig(transport);
  if (cfg == nullptr || cfg->serial == nullptr) return false;

  cfg->serial->setRx(cfg->rxPin);
  cfg->serial->setTx(cfg->txPin);
  cfg->serial->setTimeout(cfg->timeout_ms);
  cfg->serial->begin(cfg->baudrate);
  if (!cfg->serial->operator bool()) return false;

  s_map = findTxMap(cfg->serial);
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

  // USART starts requesting DMA when DR empties from this point on.
  SET_BIT(s_uart->CR3, USART_CR3_DMAT);

  s_dma_active = false;
  return true;
}

bool serial_transport_close(struct uxrCustomTransport* transport) {
  const SerialConfig* cfg = getConfig(transport);
  if (cfg == nullptr || cfg->serial == nullptr) return false;

  if (s_uart != nullptr) {
    CLEAR_BIT(s_uart->CR3, USART_CR3_DMAT);
  }
  if (s_hdma_tx.Instance != nullptr) {
    HAL_DMA_Abort(&s_hdma_tx);
    HAL_DMA_DeInit(&s_hdma_tx);
    s_hdma_tx.Instance = nullptr;
  }
  if (s_map != nullptr) {
    HAL_NVIC_DisableIRQ(s_map->irqn);
  }

  s_dma_active = false;
  s_uart = nullptr;
  s_map = nullptr;

  cfg->serial->end();
  return true;
}

// DMA-driven TX. Bytes are pushed into a FreeRTOS stream buffer; the
// DMA TC IRQ chains the next chunk autonomously, so write() returns
// without waiting for the line to drain. Backpressure: if the stream
// buffer is full, xStreamBufferSend blocks the calling task for up to
// kTxStreamSendTimeoutMs ms, then returns the partial count — preferred
// over silent drops which would corrupt xrce-dds framing.
size_t serial_transport_write(struct uxrCustomTransport* /*transport*/,
                              const uint8_t* buf, size_t len,
                              uint8_t* /*errcode*/) {
  if (s_tx_stream == nullptr || s_uart == nullptr || s_map == nullptr) {
    return 0;
  }
  if (buf == nullptr || len == 0) return 0;

  const size_t sent = xStreamBufferSend(s_tx_stream, buf, len,
                                        pdMS_TO_TICKS(kTxStreamSendTimeoutMs));

  // Atomic "test-and-set" of the active flag against the TC IRQ. The
  // critical section masks priority-5 IRQs on Cortex-M (BASEPRI), so
  // s_dma_active cannot change under us mid-check.
  taskENTER_CRITICAL();
  const bool was_idle = !s_dma_active;
  if (was_idle) s_dma_active = true;
  taskEXIT_CRITICAL();

  if (was_idle) {
    // No DMA running yet → no IRQ can fire here. Pull whatever was just
    // pushed and start the first chunk; the IRQ chain takes over after.
    const size_t to_send =
        xStreamBufferReceive(s_tx_stream, s_dma_buf, kTxDmaBufSize, 0);
    if (to_send > 0) {
      HAL_DMA_Start_IT(&s_hdma_tx, reinterpret_cast<uint32_t>(s_dma_buf),
                       reinterpret_cast<uint32_t>(&s_uart->DR), to_send);
    } else {
      // Should not happen — we just sent at least 1 byte. Defensive.
      s_dma_active = false;
    }
  }

  return sent;
}

// Replaces Stream::readBytes (which busy-polls without yielding —
// burns CPU while waiting). We poll available() with vTaskDelay(1)
// gaps so the task is Blocked between checks; idle time accumulates
// for other tasks instead of being eaten by the RX wait.
size_t serial_transport_read(struct uxrCustomTransport* transport, uint8_t* buf,
                             size_t len, int timeout, uint8_t* /*errcode*/) {
  const SerialConfig* cfg = getConfig(transport);
  if (cfg == nullptr || cfg->serial == nullptr || buf == nullptr || len == 0) {
    return 0;
  }

  HardwareSerial* serial = cfg->serial;

  TickType_t ticks_to_wait =
      (timeout > 0) ? pdMS_TO_TICKS(static_cast<uint32_t>(timeout)) : 0;

  TimeOut_t timeout_state;
  vTaskSetTimeOutState(&timeout_state);

  size_t got = 0;

  while (got < len) {
    const int avail = serial->available();

    if (avail > 0) {
      size_t to_read = static_cast<size_t>(avail);
      if (to_read > (len - got)) {
        to_read = len - got;
      }

      for (size_t i = 0; i < to_read; ++i) {
        const int b = serial->read();
        if (b < 0) {
          break;
        }
        buf[got++] = static_cast<uint8_t>(b);
      }

      continue;
    }

    if (ticks_to_wait == 0) {
      break;
    }

    if (xTaskCheckForTimeOut(&timeout_state, &ticks_to_wait) != pdFALSE) {
      break;
    }

    vTaskDelay(1);
  }

  return got;
}
