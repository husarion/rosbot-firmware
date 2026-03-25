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

#pragma once

#include <SPI.h>

#include "transport.hpp"

struct SpiTransportConfig {
  uint32_t mosi_pin;
  uint32_t miso_pin;
  uint32_t sck_pin;
  uint32_t spi_speed;
  BitOrder bit_order;
  uint8_t spi_mode;
};

/// SPI transport implementation using STM32 HW SPI.
class SpiTransport : public Transport {
 public:
  explicit SpiTransport(const SpiTransportConfig& cfg);

  bool init() override;
  void transfer(uint8_t byte) override;

 private:
  SpiTransportConfig cfg_;
  SPIClass spi_;
};
