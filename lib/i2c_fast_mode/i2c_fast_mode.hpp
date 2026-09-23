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

#include <Wire.h>

inline constexpr uint32_t kI2cFastModeHz = 400000;

// STM32duino's setClock() always picks the 16/9 duty cycle for fast mode.
// With PCLK1 = 42 MHz that rounds CCR up to 5 and the bus runs at 336 kHz
// (read back from I2C->CCR on ROSbot 3). The 2:1 duty cycle divides evenly
// (CCR = 35, exactly 400 kHz) and still meets the fast-mode minimums:
// tLOW 1.67 us >= 1.3 us, tHIGH 0.83 us >= 0.6 us.
inline void setI2cFastMode(TwoWire& bus) {
  bus.setClock(kI2cFastModeHz);
  I2C_HandleTypeDef* h = bus.getHandle();
  __HAL_I2C_DISABLE(h);
  h->Init.DutyCycle = I2C_DUTYCYCLE_2;
  HAL_I2C_Init(h);
  __HAL_I2C_ENABLE(h);
}
