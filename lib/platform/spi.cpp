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

#include "spi.hpp"

#include "spi_math.hpp"

namespace platform {

bool Spi::init() {
  SPI_TypeDef* spi = cfg_.instance;
  uint32_t pclk = 0;
  if (spi == SPI1) {
    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_SPI1_FORCE_RESET();
    __HAL_RCC_SPI1_RELEASE_RESET();
    pclk = HAL_RCC_GetPCLK2Freq();
  } else if (spi == SPI2) {
    __HAL_RCC_SPI2_CLK_ENABLE();
    __HAL_RCC_SPI2_FORCE_RESET();
    __HAL_RCC_SPI2_RELEASE_RESET();
    pclk = HAL_RCC_GetPCLK1Freq();
  } else if (spi == SPI3) {
    __HAL_RCC_SPI3_CLK_ENABLE();
    __HAL_RCC_SPI3_FORCE_RESET();
    __HAL_RCC_SPI3_RELEASE_RESET();
    pclk = HAL_RCC_GetPCLK1Freq();
  } else {
    return false;
  }

  // Pin setup as in the core's pin map: AF push-pull with pull-up; SCK
  // pulled toward its idle level (CPOL).
  const bool cpol = (cfg_.mode & 0b10) != 0;
  cfg_.mosi.setAsAlternate(cfg_.af, Pull::kUp);
  cfg_.miso.setAsAlternate(cfg_.af, Pull::kUp);
  cfg_.sck.setAsAlternate(cfg_.af, cpol ? Pull::kUp : Pull::kDown);

  spi->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI |
             (static_cast<uint32_t>(spiBaudRateBits(pclk, cfg_.max_hz))
              << SPI_CR1_BR_Pos) |
             (cpol ? SPI_CR1_CPOL : 0U) |
             ((cfg_.mode & 0b01) ? SPI_CR1_CPHA : 0U);
  spi->CR2 = 0;
  spi->CR1 |= SPI_CR1_SPE;
  return true;
}

void Spi::transfer(uint8_t value) {
  SPI_TypeDef* spi = cfg_.instance;
  while (!(spi->SR & SPI_SR_TXE)) {
  }
  *reinterpret_cast<volatile uint8_t*>(&spi->DR) = value;
  while (!(spi->SR & SPI_SR_RXNE)) {
  }
  (void)spi->DR;  // nothing to receive; reading keeps OVR clear
  while (spi->SR & SPI_SR_BSY) {
  }
}

}  // namespace platform
