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

namespace platform {

// USART_BRR for 16x oversampling, computed exactly like HAL's
// UART_BRR_SAMPLING16 (which the Arduino core used): USARTDIV is truncated
// to 1/100 before the fraction is rounded, so e.g. 42 MHz / 921600 gives 45
// (+1.27 %) where plain rounding would give 46 (-0.93 %).
constexpr uint32_t uartBrr(uint32_t pclk_hz, uint32_t baud) {
  const uint32_t div100 =
      static_cast<uint32_t>((static_cast<uint64_t>(pclk_hz) * 25U) /
                            (4U * static_cast<uint64_t>(baud)));
  const uint32_t mantissa = div100 / 100U;
  const uint32_t fraction = ((div100 - mantissa * 100U) * 16U + 50U) / 100U;
  return (mantissa << 4) + (fraction & 0xF0U) + (fraction & 0x0FU);
}

// Bytes a circular RX DMA has written past tail. ndtr counts down from size
// and reloads at 0, so the write position is size - ndtr (mod size).
constexpr size_t circularRxAvailable(size_t size, size_t ndtr, size_t tail) {
  const size_t head = (size - ndtr) % size;
  return (head + size - tail) % size;
}

}  // namespace platform
