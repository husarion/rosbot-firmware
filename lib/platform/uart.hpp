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

#pragma once

#include <cstddef>
#include <cstdint>

#include "freertos.hpp"
#include "pin.hpp"

namespace platform {

struct UartDmaStreams;

struct UartConfig {
  USART_TypeDef* usart;  // USART1, USART2 or USART3
  Pin tx;
  Pin rx;
  uint8_t af;             // GPIO_AF7_USARTx
  size_t tx_queue_bytes;  // bytes that can wait for the TX DMA
};

// 8N1 UART on registers. TX: a FreeRTOS stream buffer drained by DMA in
// chunks. RX: circular DMA into a 512 B ring, read by polling its counter;
// no USART interrupt is used, since every reader is a periodic task.
//
// Writers must be serialised by the caller (MavlinkNode's tx mutex, the
// debug-serial mutex, one PowerBoard task). Before the scheduler runs, and
// while createTasks() has interrupts masked, every write is polled.
class Uart {
 public:
  explicit Uart(const UartConfig& cfg) : cfg_(cfg) {}
  Uart(const Uart&) = delete;
  Uart& operator=(const Uart&) = delete;

  // Re-opening drops anything queued or received.
  bool open(uint32_t baud);
  void close();

  // All or nothing, never blocks: false if len does not fit the TX queue.
  bool writeFrame(const uint8_t* data, size_t len);

  // Blocks until everything is queued.
  void write(const uint8_t* data, size_t len);
  void print(const char* text);
  void println(const char* text);
  void printf(const char* fmt, ...) __attribute__((format(printf, 2, 3)));

  // Blocks until the last byte has left the shift register.
  void flush();

  size_t available() const;
  int read();  // -1 when empty
  size_t read(uint8_t* buf, size_t len);

  void onTxDmaComplete();  // TX DMA stream IRQ (complete or error) only

 private:
  static constexpr size_t kRxBufBytes = 512;
  static constexpr size_t kTxChunkBytes = 256;

  bool polledTx() const;
  void writePolled(const uint8_t* data, size_t len);
  void kickTx();
  void startTxDma(size_t len);
  size_t rxHead() const;

  UartConfig cfg_;
  const UartDmaStreams* dma_ = nullptr;
  StreamBufferHandle_t tx_queue_ = nullptr;
  volatile bool tx_active_ = false;
  size_t rx_tail_ = 0;
  alignas(4) uint8_t rx_buf_[kRxBufBytes] = {};
  alignas(4) uint8_t tx_chunk_[kTxChunkBytes] = {};
};

}  // namespace platform
