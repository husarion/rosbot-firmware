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

void enableTimerClock(TIM_TypeDef* timer);

// Kernel clock of the timer: 2x its APB clock whenever that APB divides.
uint32_t timerClockHz(const TIM_TypeDef* timer);

struct PwmConfig {
  Pin pin;
  TIM_TypeDef* timer;  // general-purpose timer (no break/dead-time unit)
  uint8_t channel;     // 1..4
  uint8_t af;          // GPIO_AFx_TIMy for this pin
};

// Edge-aligned PWM1 output, active high, preloaded ARR and CCR.
class PwmOutput {
 public:
  explicit PwmOutput(const PwmConfig& cfg) : cfg_(cfg) {}

  // Starts the timer at 0 % duty.
  void init(uint32_t frequency_hz);

  // Timer ticks per period (ARR + 1); compare() == period() is 100 %.
  uint32_t period() const { return period_; }

  // Takes effect at the next update event.
  void setCompare(uint32_t ticks) const;

 private:
  volatile uint32_t& ccr() const;

  PwmConfig cfg_;
  uint32_t period_ = 0;
};

}  // namespace platform
