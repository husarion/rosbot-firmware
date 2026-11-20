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

#include "pin.hpp"

namespace platform {

struct SpiConfig {
  SPI_TypeDef* instance;
  Pin mosi;
  Pin miso;
  Pin sck;
  uint8_t af;       // GPIO_AFx_SPIy
  uint32_t max_hz;  // rounded down to pclk / 2^n
  uint8_t mode;     // 0..3: CPOL << 1 | CPHA
};

// Blocking 8-bit SPI master, MSB first, software NSS (no chip select).
class Spi {
 public:
  explicit Spi(const SpiConfig& cfg) : cfg_(cfg) {}

  bool init();
  void transfer(uint8_t value);

 private:
  SpiConfig cfg_;
};

}  // namespace platform
