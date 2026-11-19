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

#include "uart.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "uart_math.hpp"

namespace platform {

// Streams and channels per RM0090 DMA request tables. USART3 TX takes the
// Stream 4 alternative: Stream 3 is the BNO055 I2C2 RX stream.
struct UartDmaStreams {
  USART_TypeDef* usart;
  DMA_TypeDef* dma;
  DMA_Stream_TypeDef* tx;
  uint8_t tx_index;
  uint8_t tx_channel;
  IRQn_Type tx_irq;
  DMA_Stream_TypeDef* rx;
  uint8_t rx_index;
  uint8_t rx_channel;
};

namespace {

constexpr uint32_t kTxIrqPriority = 5;  // calls *FromISR

// Index = slot in s_tx_owner, matching the IRQ handlers at the bottom.
const UartDmaStreams kStreams[] = {
    {USART1, DMA2, DMA2_Stream7, 7, 4, DMA2_Stream7_IRQn, DMA2_Stream2, 2, 4},
    {USART2, DMA1, DMA1_Stream6, 6, 4, DMA1_Stream6_IRQn, DMA1_Stream5, 5, 4},
    {USART3, DMA1, DMA1_Stream4, 4, 7, DMA1_Stream4_IRQn, DMA1_Stream1, 1, 4},
};

Uart* s_tx_owner[3] = {};

const UartDmaStreams* streamsFor(const USART_TypeDef* usart, size_t* slot) {
  for (size_t i = 0; i < 3; ++i) {
    if (kStreams[i].usart == usart) {
      *slot = i;
      return &kStreams[i];
    }
  }
  return nullptr;
}

// LIFCR/HIFCR bit offset of a stream's flag group.
constexpr uint32_t kFlagShift[4] = {0, 6, 16, 22};
constexpr uint32_t kAllFlags = 0x3DU;  // FEIF, DMEIF, TEIF, HTIF, TCIF

void clearStreamFlags(DMA_TypeDef* dma, uint8_t index) {
  const uint32_t bits = kAllFlags << kFlagShift[index & 3U];
  if (index < 4) {
    dma->LIFCR = bits;
  } else {
    dma->HIFCR = bits;
  }
}

void disableStream(DMA_Stream_TypeDef* stream) {
  stream->CR &= ~DMA_SxCR_EN;
  while (stream->CR & DMA_SxCR_EN) {
  }
}

uint32_t pclkFor(const USART_TypeDef* usart) {
  return usart == USART1 ? HAL_RCC_GetPCLK2Freq() : HAL_RCC_GetPCLK1Freq();
}

void enableClocks(const UartDmaStreams& s) {
  if (s.usart == USART1) RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
  if (s.usart == USART2) RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
  if (s.usart == USART3) RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
  RCC->AHB1ENR |= (s.dma == DMA1) ? RCC_AHB1ENR_DMA1EN : RCC_AHB1ENR_DMA2EN;
  (void)RCC->AHB1ENR;
}

bool schedulerRunning() {
  return xTaskGetSchedulerState() == taskSCHEDULER_RUNNING;
}

}  // namespace

bool Uart::open(uint32_t baud) {
  size_t slot = 0;
  dma_ = streamsFor(cfg_.usart, &slot);
  if (dma_ == nullptr) return false;
  if (tx_queue_ == nullptr) {
    tx_queue_ = xStreamBufferCreate(cfg_.tx_queue_bytes, 1);
    if (tx_queue_ == nullptr) return false;
  }
  close();
  s_tx_owner[slot] = this;
  enableClocks(*dma_);

  USART_TypeDef* usart = cfg_.usart;
  usart->CR1 = 0;
  usart->CR2 = 0;
  usart->CR3 = 0;
  usart->BRR = uartBrr(pclkFor(usart), baud);

  DMA_Stream_TypeDef* rx = dma_->rx;
  rx->PAR = reinterpret_cast<uint32_t>(&usart->DR);
  rx->M0AR = reinterpret_cast<uint32_t>(rx_buf_);
  rx->NDTR = kRxBufBytes;
  rx->FCR = 0;  // direct mode
  rx->CR = (static_cast<uint32_t>(dma_->rx_channel) << DMA_SxCR_CHSEL_Pos) |
           DMA_SxCR_MINC | DMA_SxCR_CIRC | DMA_SxCR_PL_0;
  clearStreamFlags(dma_->dma, dma_->rx_index);
  rx->CR |= DMA_SxCR_EN;
  rx_tail_ = 0;

  DMA_Stream_TypeDef* tx = dma_->tx;
  tx->PAR = reinterpret_cast<uint32_t>(&usart->DR);
  tx->FCR = 0;
  tx->CR = (static_cast<uint32_t>(dma_->tx_channel) << DMA_SxCR_CHSEL_Pos) |
           DMA_SxCR_MINC | DMA_SxCR_DIR_0 | DMA_SxCR_PL_0 | DMA_SxCR_TCIE |
           DMA_SxCR_TEIE;
  NVIC_SetPriority(dma_->tx_irq, kTxIrqPriority);
  NVIC_EnableIRQ(dma_->tx_irq);
  tx_active_ = false;

  // Same pin setup as the core's pin map: AF push-pull with pull-up.
  cfg_.tx.setAsAlternate(cfg_.af, Pull::kUp);
  cfg_.rx.setAsAlternate(cfg_.af, Pull::kUp);

  usart->CR3 = USART_CR3_DMAR | USART_CR3_DMAT;
  usart->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
  return true;
}

void Uart::close() {
  if (dma_ == nullptr) return;
  cfg_.usart->CR1 = 0;
  cfg_.usart->CR3 = 0;
  NVIC_DisableIRQ(dma_->tx_irq);
  disableStream(dma_->tx);
  disableStream(dma_->rx);
  clearStreamFlags(dma_->dma, dma_->tx_index);
  clearStreamFlags(dma_->dma, dma_->rx_index);
  tx_active_ = false;
  if (tx_queue_ != nullptr) xStreamBufferReset(tx_queue_);
}

bool Uart::polledTx() const { return !schedulerRunning(); }

void Uart::writePolled(const uint8_t* data, size_t len) {
  while (tx_active_) {
  }
  for (size_t i = 0; i < len; ++i) {
    while (!(cfg_.usart->SR & USART_SR_TXE)) {
    }
    cfg_.usart->DR = data[i];
  }
}

bool Uart::writeFrame(const uint8_t* data, size_t len) {
  if (dma_ == nullptr || data == nullptr || len == 0) return false;
  if (polledTx()) {
    writePolled(data, len);
    return true;
  }
  // A partial frame would corrupt the stream for the receiver. Writers are
  // serialised and the DMA IRQ only frees space, so the check holds.
  if (xStreamBufferSpacesAvailable(tx_queue_) < len) return false;
  xStreamBufferSend(tx_queue_, data, len, 0);
  kickTx();
  return true;
}

void Uart::write(const uint8_t* data, size_t len) {
  if (dma_ == nullptr || data == nullptr) return;
  if (polledTx()) {
    writePolled(data, len);
    return;
  }
  while (len > 0) {
    const size_t sent = xStreamBufferSend(tx_queue_, data, len, 0);
    data += sent;
    len -= sent;
    kickTx();
    if (len > 0) vTaskDelay(1);
  }
}

void Uart::print(const char* text) {
  write(reinterpret_cast<const uint8_t*>(text), std::strlen(text));
}

void Uart::println(const char* text) {
  print(text);
  print("\r\n");
}

void Uart::printf(const char* fmt, ...) {
  char buf[256];
  va_list args;
  va_start(args, fmt);
  const int n = vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  if (n > 0) {
    write(reinterpret_cast<const uint8_t*>(buf),
          static_cast<size_t>(n) < sizeof(buf) ? static_cast<size_t>(n)
                                               : sizeof(buf) - 1);
  }
}

void Uart::flush() {
  if (dma_ == nullptr) return;
  const bool running = schedulerRunning();
  while (tx_active_ ||
         (tx_queue_ != nullptr && !xStreamBufferIsEmpty(tx_queue_)) ||
         !(cfg_.usart->SR & USART_SR_TC)) {
    if (running) vTaskDelay(1);
  }
}

// The check-receive-arm sequence must be atomic against onTxDmaComplete(),
// or a queue could be left full with no DMA running. The FromISR receive is
// the only stream-buffer call allowed inside a critical section.
void Uart::kickTx() {
  taskENTER_CRITICAL();
  if (!tx_active_) {
    BaseType_t woken = pdFALSE;
    const size_t len = xStreamBufferReceiveFromISR(tx_queue_, tx_chunk_,
                                                   kTxChunkBytes, &woken);
    if (len > 0) startTxDma(len);
  }
  taskEXIT_CRITICAL();
}

void Uart::startTxDma(size_t len) {
  tx_active_ = true;
  DMA_Stream_TypeDef* tx = dma_->tx;
  clearStreamFlags(dma_->dma, dma_->tx_index);
  cfg_.usart->SR = ~USART_SR_TC;  // rc_w0
  tx->M0AR = reinterpret_cast<uint32_t>(tx_chunk_);
  tx->NDTR = len;
  tx->CR |= DMA_SxCR_EN;
}

// Transfer complete or transfer error: either way the stream has stopped
// (hardware clears EN on an error). A failed chunk is dropped; the receiver
// sees one bad frame instead of TX stalling for good.
void Uart::onTxDmaComplete() {
  clearStreamFlags(dma_->dma, dma_->tx_index);
  BaseType_t woken = pdFALSE;
  const size_t len =
      xStreamBufferReceiveFromISR(tx_queue_, tx_chunk_, kTxChunkBytes, &woken);
  if (len > 0) {
    startTxDma(len);
  } else {
    tx_active_ = false;
  }
  portYIELD_FROM_ISR(woken);
}

size_t Uart::rxHead() const {
  return (kRxBufBytes - dma_->rx->NDTR) % kRxBufBytes;
}

size_t Uart::available() const {
  if (dma_ == nullptr) return 0;
  return circularRxAvailable(kRxBufBytes, dma_->rx->NDTR, rx_tail_);
}

int Uart::read() {
  if (available() == 0) return -1;
  const uint8_t byte = rx_buf_[rx_tail_];
  rx_tail_ = (rx_tail_ + 1) % kRxBufBytes;
  return byte;
}

size_t Uart::read(uint8_t* buf, size_t len) {
  size_t got = 0;
  size_t avail = available();
  while (got < len && avail > 0) {
    buf[got++] = rx_buf_[rx_tail_];
    rx_tail_ = (rx_tail_ + 1) % kRxBufBytes;
    --avail;
  }
  return got;
}

}  // namespace platform

// Weak in the startup file; one per TX stream in kStreams.
extern "C" void DMA2_Stream7_IRQHandler(void) {
  if (platform::s_tx_owner[0] != nullptr) {
    platform::s_tx_owner[0]->onTxDmaComplete();
  }
}
extern "C" void DMA1_Stream6_IRQHandler(void) {
  if (platform::s_tx_owner[1] != nullptr) {
    platform::s_tx_owner[1]->onTxDmaComplete();
  }
}
extern "C" void DMA1_Stream4_IRQHandler(void) {
  if (platform::s_tx_owner[2] != nullptr) {
    platform::s_tx_owner[2]->onTxDmaComplete();
  }
}
