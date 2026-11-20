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

#include "timer.hpp"

#include "freertos.hpp"

namespace platform {

namespace {

bool onApb2(const TIM_TypeDef* timer) {
  return timer == TIM1 || timer == TIM8 || timer == TIM9 || timer == TIM10 ||
         timer == TIM11;
}

}  // namespace

void enableTimerClock(TIM_TypeDef* timer) {
  if (timer == TIM1) __HAL_RCC_TIM1_CLK_ENABLE();
  if (timer == TIM2) __HAL_RCC_TIM2_CLK_ENABLE();
  if (timer == TIM3) __HAL_RCC_TIM3_CLK_ENABLE();
  if (timer == TIM4) __HAL_RCC_TIM4_CLK_ENABLE();
  if (timer == TIM5) __HAL_RCC_TIM5_CLK_ENABLE();
  if (timer == TIM6) __HAL_RCC_TIM6_CLK_ENABLE();
  if (timer == TIM7) __HAL_RCC_TIM7_CLK_ENABLE();
  if (timer == TIM8) __HAL_RCC_TIM8_CLK_ENABLE();
  if (timer == TIM9) __HAL_RCC_TIM9_CLK_ENABLE();
  if (timer == TIM10) __HAL_RCC_TIM10_CLK_ENABLE();
  if (timer == TIM11) __HAL_RCC_TIM11_CLK_ENABLE();
  if (timer == TIM12) __HAL_RCC_TIM12_CLK_ENABLE();
  if (timer == TIM13) __HAL_RCC_TIM13_CLK_ENABLE();
  if (timer == TIM14) __HAL_RCC_TIM14_CLK_ENABLE();
}

uint32_t timerClockHz(const TIM_TypeDef* timer) {
  if (onApb2(timer)) {
    const uint32_t pclk2 = HAL_RCC_GetPCLK2Freq();
    const bool divided = (RCC->CFGR & RCC_CFGR_PPRE2) != RCC_CFGR_PPRE2_DIV1;
    return divided ? 2 * pclk2 : pclk2;
  }
  const uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
  const bool divided = (RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1;
  return divided ? 2 * pclk1 : pclk1;
}

void PwmOutput::init(uint32_t frequency_hz) {
  configASSERT(cfg_.channel >= 1 && cfg_.channel <= 4);
  configASSERT(cfg_.timer != TIM1 && cfg_.timer != TIM8);
  TIM_TypeDef* tim = cfg_.timer;
  enableTimerClock(tim);

  // Same PSC/ARR choice as the Arduino HardwareTimer it replaces.
  const uint32_t period_cycles = timerClockHz(tim) / frequency_hz;
  const uint32_t prescaler = period_cycles / 0x10000U + 1U;
  period_ = period_cycles / prescaler;

  tim->CR1 = TIM_CR1_ARPE;
  tim->PSC = prescaler - 1U;
  tim->ARR = period_ - 1U;

  const uint32_t shift = ((cfg_.channel - 1U) & 1U) * 8U;
  volatile uint32_t& ccmr = (cfg_.channel <= 2) ? tim->CCMR1 : tim->CCMR2;
  ccmr = (ccmr & ~(0xFFU << shift)) |
         ((TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1PE) << shift);
  ccr() = 0;
  tim->EGR = TIM_EGR_UG;  // load PSC/ARR/CCR before the first period

  const uint32_t cc_shift = (cfg_.channel - 1U) * 4U;
  tim->CCER = (tim->CCER & ~(0xFU << cc_shift)) | (TIM_CCER_CC1E << cc_shift);

  // Arduino's pin map puts a pull-up on these timer pins; kept for parity.
  cfg_.pin.setAsAlternate(cfg_.af, Pull::kUp);
  tim->CR1 |= TIM_CR1_CEN;
}

void PwmOutput::setCompare(uint32_t ticks) const { ccr() = ticks; }

volatile uint32_t& PwmOutput::ccr() const {
  TIM_TypeDef* tim = cfg_.timer;
  switch (cfg_.channel) {
    case 1:
      return tim->CCR1;
    case 2:
      return tim->CCR2;
    case 3:
      return tim->CCR3;
    default:
      return tim->CCR4;
  }
}

}  // namespace platform
