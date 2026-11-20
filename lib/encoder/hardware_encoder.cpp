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

#include "hardware_encoder.hpp"

#include "clock.hpp"
#include "counter_delta.hpp"
#include "freertos.hpp"
#include "timer.hpp"

static TIM_HandleTypeDef s_htim_pool[4];
static uint8_t s_htim_count = 0;

// ---------- helpers ------------------------------------------------

static uint8_t timerAF(TIM_TypeDef* tim) {
  if (tim == TIM1 || tim == TIM2) return GPIO_AF1_TIM1;

  if (tim == TIM3 || tim == TIM4 || tim == TIM5) return GPIO_AF2_TIM3;

  if (tim == TIM8) return GPIO_AF3_TIM8;

  // TIM6, TIM7 no AF
  configASSERT(!"Timer does not support GPIO Alternate Function");
  return 0;
}

HardwareEncoder::HardwareEncoder(const HardwareEncoderConfig& cfg)
    : cfg_(cfg) {}

void HardwareEncoder::init() {
  configASSERT(s_htim_count < sizeof(s_htim_pool) / sizeof(s_htim_pool[0]));
  timer_handle_ = &s_htim_pool[s_htim_count++];

  platform::enableTimerClock(cfg_.timer);

  const uint8_t af = timerAF(cfg_.timer);
  cfg_.pin_a.setAsAlternate(af, platform::Pull::kUp, platform::Speed::kHigh);
  cfg_.pin_b.setAsAlternate(af, platform::Pull::kUp, platform::Speed::kHigh);

  // Timer config
  timer_handle_->Instance = cfg_.timer;
  timer_handle_->Init.Prescaler = 0;
  timer_handle_->Init.CounterMode = TIM_COUNTERMODE_UP;
  timer_handle_->Init.Period = CNT_MAX;
  timer_handle_->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  timer_handle_->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  // Encoder mode x4
  encoder_cfg_.EncoderMode = TIM_ENCODERMODE_TI12;
  encoder_cfg_.IC1Polarity =
      (cfg_.inv_dir) ? TIM_ICPOLARITY_RISING : TIM_ICPOLARITY_FALLING;
  encoder_cfg_.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  encoder_cfg_.IC1Prescaler = TIM_ICPSC_DIV1;
  encoder_cfg_.IC1Filter = 0x0F;
  encoder_cfg_.IC2Polarity = TIM_ICPOLARITY_RISING;
  encoder_cfg_.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  encoder_cfg_.IC2Prescaler = TIM_ICPSC_DIV1;
  encoder_cfg_.IC2Filter = 0x0F;

  HAL_TIM_Encoder_Init(timer_handle_, &encoder_cfg_);
  HAL_TIM_Encoder_Start(timer_handle_, TIM_CHANNEL_ALL);

  reset();
}

void HardwareEncoder::reset() {
  __HAL_TIM_SET_COUNTER(timer_handle_, 0);
  last_cnt_ = 0;
  total_ticks_ = 0;
  last_time_us_ = platform::clock::counterUs();
  data_ = {};
  last_velocity_ = 0.0f;
}

void HardwareEncoder::update() {
  const uint32_t now = platform::clock::counterUs();
  const uint32_t dt_us = now - last_time_us_;

  if (dt_us >= MIN_DT_US) {
    const uint32_t cnt = getTicks();
    const int32_t delta = counterDelta(cnt, last_cnt_, CNT_MAX);

    total_ticks_ += delta;
    data_.position = static_cast<float>(static_cast<double>(total_ticks_) *
                                        static_cast<double>(cfg_.rad_per_tick));

    const float delta_pos = static_cast<float>(delta) * cfg_.rad_per_tick;

    float dt = static_cast<float>(dt_us) * US_TO_SEC;
    float vel = delta_pos / dt;
    data_.velocity = lowPass(last_velocity_, vel);

    last_velocity_ = data_.velocity;
    last_cnt_ = cnt;
    last_time_us_ = now;
  }
}
