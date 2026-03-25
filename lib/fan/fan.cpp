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

FanController::~FanController() {
  stop();
  if (initialized_ && config_.mode != FanMode::AlwaysOn) {
    HAL_TIM_PWM_DeInit(&htim_);
  }
}

void FanController::init(const FanConfig& config) {
  if (initialized_) return;
  config_ = config;

  if (config_.mode == FanMode::AlwaysOn) {
    pinMode(config_.pin, OUTPUT);
    digitalWrite(config_.pin, HIGH);
    current_duty_ = 100;
    running_ = true;
    initialized_ = true;
    return;
  }

  enableTimerClock();
  configureTimeBase();
  configureGpio();
  configureChannel();
  applyDuty(0);
  startPwm();

  initialized_ = true;
}

void FanController::enableTimerClock() {
  TIM_TypeDef* t = config_.timer_instance;

  if (t == TIM1) {
    __HAL_RCC_TIM1_CLK_ENABLE();
  } else if (t == TIM2) {
    __HAL_RCC_TIM2_CLK_ENABLE();
  } else if (t == TIM3) {
    __HAL_RCC_TIM3_CLK_ENABLE();
  } else if (t == TIM4) {
    __HAL_RCC_TIM4_CLK_ENABLE();
  } else if (t == TIM5) {
    __HAL_RCC_TIM5_CLK_ENABLE();
  } else if (t == TIM8) {
    __HAL_RCC_TIM8_CLK_ENABLE();
  }
#ifdef TIM9
  else if (t == TIM9) {
    __HAL_RCC_TIM9_CLK_ENABLE();
  }
#endif
#ifdef TIM10
  else if (t == TIM10) {
    __HAL_RCC_TIM10_CLK_ENABLE();
  }
#endif
#ifdef TIM11
  else if (t == TIM11) {
    __HAL_RCC_TIM11_CLK_ENABLE();
  }
#endif
#ifdef TIM12
  else if (t == TIM12) {
    __HAL_RCC_TIM12_CLK_ENABLE();
  }
#endif
}

uint32_t FanController::getTimerClockHz() const {
  // STM32F4 timer clock rules:
  //   APB2 timers: TIM1, TIM8, TIM9, TIM10, TIM11
  //   APB1 timers: TIM2-5, TIM12-14
  //   If APBx prescaler > 1 → timer_clk = 2 × APBx_clk
  //   If APBx prescaler = 1 → timer_clk = APBx_clk

  RCC_ClkInitTypeDef clk{};
  uint32_t latency;
  HAL_RCC_GetClockConfig(&clk, &latency);

  TIM_TypeDef* t = config_.timer_instance;

  const bool is_apb2 = (t == TIM1 || t == TIM8
#ifdef TIM9
                        || t == TIM9
#endif
#ifdef TIM10
                        || t == TIM10
#endif
#ifdef TIM11
                        || t == TIM11
#endif
  );

  if (is_apb2) {
    const uint32_t pclk2 = HAL_RCC_GetPCLK2Freq();
    return (clk.APB2CLKDivider == RCC_HCLK_DIV1) ? pclk2 : pclk2 * 2;
  }

  const uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
  return (clk.APB1CLKDivider == RCC_HCLK_DIV1) ? pclk1 : pclk1 * 2;
}

void FanController::configureTimeBase() {
  const uint32_t tim_clk = getTimerClockHz();

  // Find smallest PSC so that ARR fits in 16 bits (0xFFFF).
  // f_pwm = tim_clk / (PSC+1) / (ARR+1)
  uint32_t psc = 0;
  uint32_t arr = (tim_clk / config_.pwm_frequency) - 1;

  while (arr > 0xFFFF && psc < 0xFFFF) {
    ++psc;
    arr = tim_clk / ((psc + 1) * config_.pwm_frequency) - 1;
  }

  htim_.Instance = config_.timer_instance;
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
  GPIO_TypeDef* port = digitalPinToPort(config_.pin);

  // Enable port clock
  if (port == GPIOA) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
  } else if (port == GPIOB) {
    __HAL_RCC_GPIOB_CLK_ENABLE();
  } else if (port == GPIOC) {
    __HAL_RCC_GPIOC_CLK_ENABLE();
  } else if (port == GPIOD) {
    __HAL_RCC_GPIOD_CLK_ENABLE();
  } else if (port == GPIOE) {
    __HAL_RCC_GPIOE_CLK_ENABLE();
  }

  GPIO_InitTypeDef gpio{};
  gpio.Pin = digitalPinToBitMask(config_.pin);
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  gpio.Alternate = config_.gpio_af;  // forced — bypasses pinmap
  HAL_GPIO_Init(port, &gpio);
}

// ═══════════════════════════════════════════════════════════════════════
//  PWM channel
// ═══════════════════════════════════════════════════════════════════════

void FanController::configureChannel() {
  const uint32_t ch = toHalChannel(config_.timer_channel);

  TIM_OC_InitTypeDef oc{};
  oc.OCMode = TIM_OCMODE_PWM1;
  oc.Pulse = 0;
  oc.OCPolarity = TIM_OCPOLARITY_HIGH;
  oc.OCNPolarity =
      config_.complementary ? TIM_OCNPOLARITY_HIGH : TIM_OCNPOLARITY_LOW;
  oc.OCFastMode = TIM_OCFAST_DISABLE;
  oc.OCIdleState = TIM_OCIDLESTATE_RESET;
  oc.OCNIdleState = TIM_OCNIDLESTATE_RESET;

  HAL_TIM_PWM_ConfigChannel(&htim_, &oc, ch);
}

// ═══════════════════════════════════════════════════════════════════════
//  Start / Stop PWM output
// ═══════════════════════════════════════════════════════════════════════

void FanController::startPwm() {
  const uint32_t ch = toHalChannel(config_.timer_channel);

  if (config_.complementary) {
    HAL_TIMEx_PWMN_Start(&htim_, ch);
  } else {
    HAL_TIM_PWM_Start(&htim_, ch);
  }

  // Advanced timers (TIM1, TIM8) require Master Output Enable
  if (IS_TIM_BREAK_INSTANCE(config_.timer_instance)) {
    __HAL_TIM_MOE_ENABLE(&htim_);
  }
}

void FanController::stopPwm() {
  const uint32_t ch = toHalChannel(config_.timer_channel);

  if (config_.complementary) {
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

  const uint32_t ch = toHalChannel(config_.timer_channel);
  const uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim_);
  const uint32_t ccr = (static_cast<uint32_t>(current_duty_) * arr) / 100;

  __HAL_TIM_SET_COMPARE(&htim_, ch, ccr);
}

void FanController::setDuty(uint8_t duty) {
  if (!initialized_ || config_.mode == FanMode::AlwaysOn) return;
  applyDuty(duty);
  running_ = (current_duty_ > 0);
}

// ═══════════════════════════════════════════════════════════════════════
//  Temperature-driven control
// ═══════════════════════════════════════════════════════════════════════

void FanController::update(float temp) {
  if (!initialized_ || config_.mode == FanMode::AlwaysOn) return;

  if (config_.mode == FanMode::Threshold) {
    if (!running_ && temp >= config_.temp_high) {
      applyDuty(100);
      running_ = true;
    } else if (running_ && temp <= config_.temp_low) {
      applyDuty(0);
      running_ = false;
    }
    return;
  }

  // Proportional
  if (temp <= config_.temp_low) {
    applyDuty(0);
    running_ = false;
  } else if (temp >= config_.temp_high) {
    applyDuty(100);
    running_ = true;
  } else {
    static const auto range = config_.temp_high - config_.temp_low;
    const auto normalized = (temp - config_.temp_low) / range;
    applyDuty(static_cast<uint8_t>(
        (normalized * (100u - config_.duty_min) + config_.duty_min)));
    running_ = true;
  }
}

// ═══════════════════════════════════════════════════════════════════════
//  Stop
// ═══════════════════════════════════════════════════════════════════════

void FanController::stop() {
  if (!initialized_) return;

  if (config_.mode == FanMode::AlwaysOn) {
    digitalWrite(config_.pin, LOW);
  } else {
    applyDuty(0);
    stopPwm();
  }

  current_duty_ = 0;
  running_ = false;
}
