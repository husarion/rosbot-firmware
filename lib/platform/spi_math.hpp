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

#include <cstdint>

namespace platform {

// SPI_CR1.BR for the fastest clock not above max_hz (BR = n gives
// pclk / 2^(n+1)), the rule the Arduino core's spi_init() used. Saturates
// at /256.
constexpr uint8_t spiBaudRateBits(uint32_t pclk_hz, uint32_t max_hz) {
  uint8_t br = 0;
  while (br < 7 && (pclk_hz >> (br + 1)) > max_hz) ++br;
  return br;
}

}  // namespace platform
