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

#include "analog_in.hpp"

#include "clock.hpp"

namespace platform {

namespace {

// Same ADC setup the Arduino core's analogRead() used: PCLK2/4 = 21 MHz,
// 10-bit, 15-cycle sampling.
constexpr uint32_t kAdcPrescalerDiv4 = 0b01;
constexpr uint32_t kResolution10Bit = 0b01;
constexpr uint32_t kSampling15Cycles = 0b001;
constexpr uint32_t kEocPollLimit = 1000;

void enableAdc1() {
  if (ADC1->CR2 & ADC_CR2_ADON) return;
  RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
  (void)RCC->APB2ENR;
  ADC123_COMMON->CCR = (ADC123_COMMON->CCR & ~ADC_CCR_ADCPRE) |
                       (kAdcPrescalerDiv4 << ADC_CCR_ADCPRE_Pos);
  ADC1->CR1 = kResolution10Bit << ADC_CR1_RES_Pos;
  ADC1->SQR1 = 0;  // one conversion per sequence
  ADC1->CR2 = ADC_CR2_ADON;
  // tSTAB: the ADC needs up to 3 us after ADON before the first conversion.
  clock::delayUs(3);
}

}  // namespace

void AnalogIn::init() const {
  pin_.setAsAnalog();
  const uint32_t primask = __get_PRIMASK();
  __disable_irq();
  enableAdc1();
  if (channel_ < 10) {
    const uint32_t shift = 3U * channel_;
    ADC1->SMPR2 =
        (ADC1->SMPR2 & ~(0b111UL << shift)) | (kSampling15Cycles << shift);
  } else {
    const uint32_t shift = 3U * (channel_ - 10U);
    ADC1->SMPR1 =
        (ADC1->SMPR1 & ~(0b111UL << shift)) | (kSampling15Cycles << shift);
  }
  (void)ADC1->DR;  // clears a stale EOC
  __set_PRIMASK(primask);
}

uint16_t AnalogIn::read() const {
  const uint32_t primask = __get_PRIMASK();
  __disable_irq();
  ADC1->SQR3 = channel_;
  (void)ADC1->DR;  // a late EOC from a timed-out read must not pass as ours
  ADC1->CR2 |= ADC_CR2_SWSTART;
  uint32_t polls = 0;
  while (!(ADC1->SR & ADC_SR_EOC) && ++polls < kEocPollLimit) {
  }
  // Reading DR clears EOC.
  const uint16_t value =
      polls < kEocPollLimit ? static_cast<uint16_t>(ADC1->DR) : 0;
  __set_PRIMASK(primask);
  return value;
}

}  // namespace platform
