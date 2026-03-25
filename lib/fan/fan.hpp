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

#include <Arduino.h>

#include "stm32f4xx_hal.h"

// ─── Types ───────────────────────────────────────────────────────────

enum class FanMode : uint8_t {
  AlwaysOn,     // GPIO HIGH — constant speed (no timer needed)
  Threshold,    // on/off with hysteresis
  Proportional  // linear PWM between temp_low..temp_high
};

struct FanConfig {
  uint32_t pin;
  TIM_TypeDef* timer_instance = nullptr;  // e.g. TIM8
  uint32_t timer_channel = 0;             // logical 1-4
  uint8_t gpio_af = 0;                    // e.g. GPIO_AF3_TIM8
  bool complementary = false;             // true for CHxN
  FanMode mode = FanMode::AlwaysOn;
  uint32_t pwm_frequency = 25000;  // Hz
  int8_t temp_low = 35;            // °C — fan off / min duty
  int8_t temp_high = 50;           // °C — fan 100%
  uint8_t duty_min = 0;            // % — minimalny duty (0 = pełne wyłączenie)
  uint8_t duty_max = 100;          // % — maksymalny duty (100 = pełna moc)
};

// ─── Class ───────────────────────────────────────────────────────────

class FanController {
 public:
  FanController() = default;
  ~FanController();

  FanController(const FanController&) = delete;
  FanController& operator=(const FanController&) = delete;

  void init(const FanConfig& config);
  void update(float temp);
  void setDuty(uint8_t duty);
  void stop();

  uint8_t currentDuty() const { return current_duty_; }
  bool isRunning() const { return running_; }

 private:
  void enableTimerClock();
  uint32_t getTimerClockHz() const;
  void configureTimeBase();
  void configureGpio();
  void configureChannel();
  void startPwm();
  void stopPwm();
  void applyDuty(uint8_t duty);

  /// Maps logical channel number (1-4) to HAL constant.
  static constexpr uint32_t toHalChannel(uint32_t ch) {
    // TIM_CHANNEL_1 = 0x00, _2 = 0x04, _3 = 0x08, _4 = 0x0C
    return (ch >= 1 && ch <= 4) ? (ch - 1) * 4u : 0u;
  }

  FanConfig config_{};
  TIM_HandleTypeDef htim_{};
  uint8_t current_duty_ = 0;
  bool running_ = false;
  bool initialized_ = false;
};
