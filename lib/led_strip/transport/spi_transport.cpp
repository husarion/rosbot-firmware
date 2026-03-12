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

#include "spi_transport.hpp"

#include <STM32FreeRTOS.h>

SpiTransport::SpiTransport(const SpiTransportConfig& cfg)
    : cfg_(cfg), spi_(cfg.mosi_pin, cfg.miso_pin, cfg.sck_pin) {}

bool SpiTransport::init() {
  SPISettings settings(cfg_.spi_speed, cfg_.bit_order, cfg_.spi_mode,
                       SPI_TRANSMITONLY);
  spi_.beginTransaction(CS_PIN_CONTROLLED_BY_USER, settings);
  return true;
}

void SpiTransport::transfer(uint8_t byte) {
  spi_.transfer(CS_PIN_CONTROLLED_BY_USER, byte);
}
