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
#include <cstring>

#include "stm32f4xx.h"

// Helpers over the fake GPIO block in stm32f4xx.h. Real hardware applies a
// BSRR write to ODR at once; the fake memory does not, so tests call
// latch() after code that writes a pin.
namespace fake_gpio {

constexpr uintptr_t kPortStride = 0x400;

inline uintptr_t portBase(uint8_t port_index) {
  return GPIOA_BASE + port_index * kPortStride;
}

inline GPIO_TypeDef* port(uint8_t port_index) {
  return reinterpret_cast<GPIO_TypeDef*>(portBase(port_index));
}

inline void reset() {
  std::memset(fake_gpio_memory, 0, sizeof(fake_gpio_memory));
}

inline void latch(uint8_t port_index) {
  GPIO_TypeDef* gpio = port(port_index);
  const uint32_t bsrr = gpio->BSRR;
  gpio->ODR = (gpio->ODR & ~(bsrr >> 16)) | (bsrr & 0xFFFFU);
  gpio->BSRR = 0;
}

inline bool outputHigh(uint8_t port_index, uint8_t pin) {
  return (port(port_index)->ODR >> pin) & 1U;
}

inline void setInput(uint8_t port_index, uint8_t pin, bool high) {
  GPIO_TypeDef* gpio = port(port_index);
  gpio->IDR = high ? (gpio->IDR | (1U << pin)) : (gpio->IDR & ~(1U << pin));
}

}  // namespace fake_gpio
