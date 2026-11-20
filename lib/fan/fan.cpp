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

#include "fan.hpp"

#include <cmath>

#include "timer.hpp"

FanController::~FanController() {
  stop();
  if (initialized_ && config_->mode != FanMode::AlwaysOn) {
    HAL_TIM_PWM_DeInit(&htim_);
  }
}

void FanController::init(const FanConfig& config) {
  if (initialized_) return;
  config_ = &config;

  if (config_->mode == FanMode::AlwaysOn) {
    config_->pin.setAsOutput(true);
    current_duty_ = 100;
    running_ = true;
    initialized_ = true;
    return;
  }

  platform::enableTimerClock(config_->timer_instance);
  configureTimeBase();
  configureGpio();
  configureChannel();
  applyDuty(0);
  startPwm();

  initialized_ = true;
}

void FanController::configureTimeBase() {
  const uint32_t tim_clk = platform::timerClockHz(config_->timer_instance);

  // Find smallest PSC so that ARR fits in 16 bits (0xFFFF).
  // f_pwm = tim_clk / (PSC+1) / (ARR+1)
  uint32_t psc = 0;
  uint32_t arr = (tim_clk / config_->pwm_frequency) - 1;

  while (arr > 0xFFFF && psc < 0xFFFF) {
    ++psc;
    arr = tim_clk / ((psc + 1) * config_->pwm_frequency) - 1;
  }

  htim_.Instance = config_->timer_instance;
  htim_.Init.Prescaler = psc;
  htim_.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim_.Init.Period = arr;
  htim_.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim_.Init.RepetitionCounter = 0;
  htim_.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

  HAL_TIM_PWM_Init(&htim_);
}

// ═══════════════════════════════════════════════════════════════════════
//  GPIO — forced Alternate Function
// ═══════════════════════════════════════════════════════════════════════

void FanController::configureGpio() {
  config_->pin.setAsAlternate(config_->gpio_af, platform::Pull::kNone,
                              platform::Speed::kLow);
}

// ═══════════════════════════════════════════════════════════════════════
//  PWM channel
// ═══════════════════════════════════════════════════════════════════════

void FanController::configureChannel() {
  const uint32_t ch = toHalChannel(config_->timer_channel);

  TIM_OC_InitTypeDef oc{};
  oc.OCMode = TIM_OCMODE_PWM1;
  oc.Pulse = 0;
  oc.OCPolarity = TIM_OCPOLARITY_HIGH;
  oc.OCNPolarity =
      config_->complementary ? TIM_OCNPOLARITY_HIGH : TIM_OCNPOLARITY_LOW;
  oc.OCFastMode = TIM_OCFAST_DISABLE;
  oc.OCIdleState = TIM_OCIDLESTATE_RESET;
  oc.OCNIdleState = TIM_OCNIDLESTATE_RESET;

  HAL_TIM_PWM_ConfigChannel(&htim_, &oc, ch);
}

// ═══════════════════════════════════════════════════════════════════════
//  Start / Stop PWM output
// ═══════════════════════════════════════════════════════════════════════

void FanController::startPwm() {
  const uint32_t ch = toHalChannel(config_->timer_channel);

  if (config_->complementary) {
    HAL_TIMEx_PWMN_Start(&htim_, ch);
  } else {
    HAL_TIM_PWM_Start(&htim_, ch);
  }

  // Advanced timers (TIM1, TIM8) require Master Output Enable
  if (IS_TIM_BREAK_INSTANCE(config_->timer_instance)) {
    __HAL_TIM_MOE_ENABLE(&htim_);
  }
}

void FanController::stopPwm() {
  const uint32_t ch = toHalChannel(config_->timer_channel);

  if (config_->complementary) {
    HAL_TIMEx_PWMN_Stop(&htim_, ch);
  } else {
    HAL_TIM_PWM_Stop(&htim_, ch);
  }
}

// ═══════════════════════════════════════════════════════════════════════
//  Duty cycle
// ═══════════════════════════════════════════════════════════════════════

void FanController::applyDuty(uint8_t duty) {
  current_duty_ = (duty > 100) ? 100 : duty;

  const uint32_t ch = toHalChannel(config_->timer_channel);
  const uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim_);
  const uint32_t ccr = (static_cast<uint32_t>(current_duty_) * arr) / 100;

  __HAL_TIM_SET_COMPARE(&htim_, ch, ccr);
}

void FanController::setDuty(uint8_t duty) {
  if (!initialized_ || config_->mode == FanMode::AlwaysOn) return;
  applyDuty(duty);
  running_ = (current_duty_ > 0);
}

// ═══════════════════════════════════════════════════════════════════════
//  Temperature-driven control
// ═══════════════════════════════════════════════════════════════════════

void FanController::update(float temp) {
  if (!initialized_ || config_->mode == FanMode::AlwaysOn) return;

  if (!std::isfinite(temp)) {
    applyDuty(100);
    running_ = true;
    return;
  }

  if (config_->mode == FanMode::Threshold) {
    if (!running_ && temp >= config_->temp_high) {
      applyDuty(100);
      running_ = true;
    } else if (running_ && temp <= config_->temp_low) {
      applyDuty(0);
      running_ = false;
    }
    return;
  }

  // Proportional
  if (temp <= config_->temp_low) {
    applyDuty(0);
    running_ = false;
  } else if (temp >= config_->temp_high) {
    applyDuty(100);
    running_ = true;
  } else {
    const auto range = config_->temp_high - config_->temp_low;
    const auto normalized = (temp - config_->temp_low) / range;
    applyDuty(static_cast<uint8_t>(
        (normalized * (100u - config_->duty_min) + config_->duty_min)));
    running_ = true;
  }
}

// ═══════════════════════════════════════════════════════════════════════
//  Stop
// ═══════════════════════════════════════════════════════════════════════

void FanController::stop() {
  if (!initialized_) return;

  if (config_->mode == FanMode::AlwaysOn) {
    config_->pin.reset();
  } else {
    applyDuty(0);
    stopPwm();
  }

  current_duty_ = 0;
  running_ = false;
}
