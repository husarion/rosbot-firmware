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

#include <unity.h>

#include <cstring>

// lib/platform is not built for env:native (clock.cpp needs the real HAL),
// so the unit under test is compiled into this TU.
#include "timer.cpp"

namespace {

constexpr uint32_t kOc1Pwm1Preload = 0x68;  // OC1M=110, OC1PE

}  // namespace

void setUp() {
  std::memset(fake_tims, 0, sizeof(fake_tims));
  std::memset(fake_tim_clock_on, 0, sizeof(fake_tim_clock_on));
  std::memset(fake_gpio_memory, 0, sizeof(fake_gpio_memory));
  // Arduino generic F407 clock tree: HCLK 168 MHz, APB1 /4, APB2 /2.
  fake_rcc.CFGR = RCC_CFGR_PPRE1_DIV4 | RCC_CFGR_PPRE2_DIV2;
  fake_pclk1_hz = 42000000;
  fake_pclk2_hz = 84000000;
}
void tearDown() {}

void test_timer_clock_doubles_divided_apb() {
  TEST_ASSERT_EQUAL_UINT32(84000000, platform::timerClockHz(TIM14));
  TEST_ASSERT_EQUAL_UINT32(84000000, platform::timerClockHz(TIM5));
  TEST_ASSERT_EQUAL_UINT32(168000000, platform::timerClockHz(TIM10));
  TEST_ASSERT_EQUAL_UINT32(168000000, platform::timerClockHz(TIM8));
  fake_rcc.CFGR = 0;
  TEST_ASSERT_EQUAL_UINT32(42000000, platform::timerClockHz(TIM14));
}

// Motor PWM: PF9 = TIM14_CH1 (APB1), 20 kHz.
void test_motor_pwm_on_apb1_timer() {
  platform::PwmOutput pwm({platform::Pin(platform::Port::F, 9), TIM14, 1, 9});
  pwm.init(20000);

  TEST_ASSERT_TRUE(fake_tim_clock_on[14]);
  TEST_ASSERT_EQUAL_UINT32(0, TIM14->PSC);
  TEST_ASSERT_EQUAL_UINT32(4199, TIM14->ARR);
  TEST_ASSERT_EQUAL_UINT32(4200, pwm.period());
  TEST_ASSERT_EQUAL_HEX32(TIM_CR1_ARPE | TIM_CR1_CEN, TIM14->CR1);
  TEST_ASSERT_EQUAL_HEX32(kOc1Pwm1Preload, TIM14->CCMR1);
  TEST_ASSERT_EQUAL_HEX32(TIM_CCER_CC1E, TIM14->CCER);
  TEST_ASSERT_EQUAL_HEX32(TIM_EGR_UG, TIM14->EGR);
  TEST_ASSERT_EQUAL_UINT32(0, TIM14->CCR1);

  pwm.setCompare(pwm.period());
  TEST_ASSERT_EQUAL_UINT32(4200, TIM14->CCR1);
}

// Motor PWM: PF6 = TIM10_CH1 (APB2), 20 kHz.
void test_motor_pwm_on_apb2_timer() {
  platform::PwmOutput pwm({platform::Pin(platform::Port::F, 6), TIM10, 1, 3});
  pwm.init(20000);
  TEST_ASSERT_EQUAL_UINT32(0, TIM10->PSC);
  TEST_ASSERT_EQUAL_UINT32(8399, TIM10->ARR);
}

// Same PSC/ARR as HardwareTimer::setOverflow(HERTZ_FORMAT) when the period
// does not fit 16 bits: factor = cycles / 0x10000 + 1.
void test_low_frequency_uses_prescaler_like_arduino() {
  platform::PwmOutput pwm({platform::Pin(platform::Port::F, 6), TIM10, 1, 3});
  pwm.init(100);  // 1 680 000 cycles -> factor 26
  TEST_ASSERT_EQUAL_UINT32(25, TIM10->PSC);
  TEST_ASSERT_EQUAL_UINT32(64614, TIM10->ARR);
}

void test_channel_three_uses_ccmr2_and_ccr3() {
  TIM3->CCMR2 = 0xFF00;  // channel 4 settings must survive
  platform::PwmOutput pwm({platform::Pin(platform::Port::B, 0), TIM3, 3, 2});
  pwm.init(20000);
  pwm.setCompare(123);
  TEST_ASSERT_EQUAL_HEX32(0xFF00 | kOc1Pwm1Preload, TIM3->CCMR2);
  TEST_ASSERT_EQUAL_HEX32(TIM_CCER_CC1E << 8, TIM3->CCER);
  TEST_ASSERT_EQUAL_UINT32(123, TIM3->CCR3);
}

void test_pin_is_af_with_pullup() {
  platform::PwmOutput pwm({platform::Pin(platform::Port::F, 9), TIM14, 1, 9});
  pwm.init(20000);
  auto* gpiof = reinterpret_cast<GPIO_TypeDef*>(GPIOA_BASE + 5 * 0x400);
  TEST_ASSERT_EQUAL_HEX32(0b10UL << 18, gpiof->MODER);
  TEST_ASSERT_EQUAL_HEX32(0b01UL << 18, gpiof->PUPDR);
  TEST_ASSERT_EQUAL_HEX32(0x9UL << 4, gpiof->AFR[1]);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_timer_clock_doubles_divided_apb);
  RUN_TEST(test_motor_pwm_on_apb1_timer);
  RUN_TEST(test_motor_pwm_on_apb2_timer);
  RUN_TEST(test_low_frequency_uses_prescaler_like_arduino);
  RUN_TEST(test_channel_three_uses_ccmr2_and_ccr3);
  RUN_TEST(test_pin_is_af_with_pullup);
  return UNITY_END();
}
