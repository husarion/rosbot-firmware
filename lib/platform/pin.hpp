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

#include "stm32f4xx.h"

namespace platform {

enum class Pull : uint8_t { kNone = 0b00, kUp = 0b01, kDown = 0b10 };

// One GPIO line, addressed by port base and line index so it can be a
// constexpr member of a board config: Pin{GPIOA_BASE, 5}.
class Pin {
 public:
  constexpr Pin(uintptr_t port_base, uint8_t index)
      : port_base_(port_base), index_(index) {}

  // ODR is written before MODER, so the line never glitches to the old
  // output level.
  void setAsOutput(bool initial_high = false) const {
    write(initial_high);
    configure(kModeOutput, Pull::kNone);
  }

  void setAsInput(Pull pull = Pull::kNone) const {
    configure(kModeInput, pull);
  }

  void setAsAnalog() const { configure(kModeAnalog, Pull::kNone); }

  void write(bool high) const { port()->BSRR = high ? mask() : (mask() << 16); }
  void set() const { write(true); }
  void reset() const { write(false); }
  bool read() const { return (port()->IDR & mask()) != 0; }

  uintptr_t portBase() const { return port_base_; }
  uint8_t index() const { return index_; }

  constexpr bool operator==(const Pin& other) const {
    return port_base_ == other.port_base_ && index_ == other.index_;
  }
  constexpr bool operator!=(const Pin& other) const {
    return !(*this == other);
  }

 private:
  static constexpr uint32_t kModeInput = 0b00;
  static constexpr uint32_t kModeOutput = 0b01;
  static constexpr uint32_t kModeAnalog = 0b11;
  static constexpr uintptr_t kPortStride = 0x400;

  GPIO_TypeDef* port() const {
    return reinterpret_cast<GPIO_TypeDef*>(port_base_);
  }
  uint32_t mask() const { return 1UL << index_; }

  // MODER/PUPDR/OSPEEDR/OTYPER and RCC are shared read-modify-write
  // registers; MotorHiZ reconfigures pins at runtime from the control task,
  // so every update runs with interrupts masked (a few cycles).
  void configure(uint32_t mode, Pull pull) const {
    const uint32_t shift2 = 2U * index_;
    const uint32_t clear2 = ~(0b11UL << shift2);
    const uint32_t primask = __get_PRIMASK();
    __disable_irq();
    enableClock();
    GPIO_TypeDef* gpio = port();
    // Very high speed and push-pull match what the Arduino core's pinMode()
    // set, so replacing it does not change edge rates.
    gpio->OSPEEDR = (gpio->OSPEEDR & clear2) | (0b11UL << shift2);
    gpio->OTYPER &= ~mask();
    gpio->PUPDR =
        (gpio->PUPDR & clear2) | (static_cast<uint32_t>(pull) << shift2);
    gpio->MODER = (gpio->MODER & clear2) | (mode << shift2);
    __set_PRIMASK(primask);
  }

  void enableClock() const {
    const uint32_t port_index = (port_base_ - GPIOA_BASE) / kPortStride;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN << port_index;
    // Read-back delay, as in __HAL_RCC_GPIOx_CLK_ENABLE().
    (void)RCC->AHB1ENR;
  }

  uintptr_t port_base_;
  uint8_t index_;
};

}  // namespace platform
