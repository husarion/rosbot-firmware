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

#include "device.hpp"

namespace platform {

enum class Port : uint8_t { A, B, C, D, E, F, G, H, I };

enum class Pull : uint8_t { kNone = 0b00, kUp = 0b01, kDown = 0b10 };

enum class Speed : uint8_t { kLow, kMedium, kHigh, kVeryHigh };

// One GPIO line, constexpr so it can live in a board config: Pin{Port::E, 2}.
class Pin {
 public:
  constexpr Pin(Port port, uint8_t index)
      : Pin(GPIOA_BASE + static_cast<uintptr_t>(port) * kPortStride, index) {}
  constexpr Pin(uintptr_t port_base, uint8_t index)
      : port_base_(port_base), index_(index) {}

  // ODR is written before MODER, so the line never glitches to the old
  // output level. The port clock must be on first: with it off the BSRR
  // write is dropped and the pin starts low (seen on HW: the BNO055 address
  // pin, first user of GPIOC, came up low and the IMU moved to 0x28).
  void setAsOutput(bool initial_high = false) const {
    enableClockMasked();
    write(initial_high);
    configure(kModeOutput, Pull::kNone);
  }

  // Open-drain output, e.g. to bit-bang a shared I2C line: high releases it.
  void setAsOutputOpenDrain(bool initial_high = true) const {
    enableClockMasked();
    write(initial_high);
    configure(kModeOutput, Pull::kNone, Speed::kVeryHigh, 0, true);
  }

  void setAsInput(Pull pull = Pull::kNone) const {
    configure(kModeInput, pull);
  }

  void setAsAnalog() const { configure(kModeAnalog, Pull::kNone); }

  // Push-pull alternate function; af is the GPIO_AFx_* number.
  void setAsAlternate(uint8_t af, Pull pull = Pull::kNone,
                      Speed speed = Speed::kVeryHigh) const {
    configure(kModeAlternate, pull, speed, af);
  }

  void setAsAlternateOpenDrain(uint8_t af, Pull pull = Pull::kNone,
                               Speed speed = Speed::kVeryHigh) const {
    configure(kModeAlternate, pull, speed, af, /*open_drain=*/true);
  }

  void write(bool high) const { port()->BSRR = high ? mask() : (mask() << 16); }
  void set() const { write(true); }
  void reset() const { write(false); }
  // Not atomic against another writer of the same line; a pin has one owner.
  void toggle() const { write((port()->ODR & mask()) == 0); }
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
  static constexpr uint32_t kModeAlternate = 0b10;
  static constexpr uint32_t kModeAnalog = 0b11;
  static constexpr uintptr_t kPortStride = 0x400;

  GPIO_TypeDef* port() const {
    return reinterpret_cast<GPIO_TypeDef*>(port_base_);
  }
  uint32_t mask() const { return 1UL << index_; }

  // MODER/PUPDR/OSPEEDR/OTYPER and RCC are shared read-modify-write
  // registers; MotorHiZ reconfigures pins at runtime from the control task,
  // so every update runs with interrupts masked (a few cycles).
  void configure(uint32_t mode, Pull pull, Speed speed = Speed::kVeryHigh,
                 uint8_t af = 0, bool open_drain = false) const {
    const uint32_t shift2 = 2U * index_;
    const uint32_t clear2 = ~(0b11UL << shift2);
    const uint32_t primask = __get_PRIMASK();
    __disable_irq();
    enableClock();
    GPIO_TypeDef* gpio = port();
    // Very high speed (the default) and push-pull match what the Arduino
    // core's pinMode() set, so replacing it does not change edge rates.
    gpio->OSPEEDR =
        (gpio->OSPEEDR & clear2) | (static_cast<uint32_t>(speed) << shift2);
    if (open_drain) {
      gpio->OTYPER |= mask();
    } else {
      gpio->OTYPER &= ~mask();
    }
    gpio->PUPDR =
        (gpio->PUPDR & clear2) | (static_cast<uint32_t>(pull) << shift2);
    if (mode == kModeAlternate) {
      const uint32_t shift4 = 4U * (index_ & 7U);
      volatile uint32_t& afr = gpio->AFR[index_ >> 3];
      afr = (afr & ~(0xFUL << shift4)) | (static_cast<uint32_t>(af) << shift4);
    }
    gpio->MODER = (gpio->MODER & clear2) | (mode << shift2);
    __set_PRIMASK(primask);
  }

  void enableClockMasked() const {
    const uint32_t primask = __get_PRIMASK();
    __disable_irq();
    enableClock();
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
