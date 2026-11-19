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

#include <cmath>
#include <cstring>
#include <initializer_list>

#include "fake_gpio.hpp"
#include "fan.cpp"
#include "timer.cpp"

namespace {

constexpr uint8_t kPortC = 2;
constexpr uint32_t kArr25kHz = 168000000 / 25000 - 1;  // TIM8 on APB2 x2

FanConfig pwmConfig(FanMode mode) {
  return FanConfig{.pin = platform::Pin{platform::Port::B, 0},
                   .timer_instance = TIM8,
                   .timer_channel = 2,
                   .gpio_af = 3,
                   .complementary = true,
                   .mode = mode,
                   .pwm_frequency = 25000,
                   .temp_low = 30,
                   .temp_high = 55,
                   .duty_min = 20};
}

uint32_t ccr() { return TIM8->CCR2; }
uint32_t ccrFor(uint8_t duty) { return duty * kArr25kHz / 100; }

}  // namespace

void setUp() {
  fake_gpio::reset();
  std::memset(fake_tims, 0, sizeof(fake_tims));
  std::memset(fake_tim_clock_on, 0, sizeof(fake_tim_clock_on));
  fake_tim_hal = FakeTimHal{};
  fake_rcc.CFGR = RCC_CFGR_PPRE1_DIV4 | RCC_CFGR_PPRE2_DIV2;
  fake_pclk1_hz = 42000000;
  fake_pclk2_hz = 84000000;
}
void tearDown() {}

void test_pwm_init_runs_complementary_output_at_25khz_off() {
  const FanConfig cfg = pwmConfig(FanMode::Proportional);
  FanController fan;
  fan.init(cfg);
  TEST_ASSERT_TRUE(fake_tim_clock_on[8]);
  TEST_ASSERT_EQUAL_UINT32(0, TIM8->PSC);
  TEST_ASSERT_EQUAL_UINT32(kArr25kHz, TIM8->ARR);
  TEST_ASSERT_EQUAL(1, fake_tim_hal.pwmn_started);
  TEST_ASSERT_EQUAL(0, fake_tim_hal.pwm_started);
  TEST_ASSERT_EQUAL_HEX32(0x4, fake_tim_hal.last_channel);  // CH2
  TEST_ASSERT_TRUE(TIM8->BDTR & TIM_BDTR_MOE);              // advanced timer
  TEST_ASSERT_EQUAL_UINT32(0, ccr());
  TEST_ASSERT_FALSE(fan.isRunning());
}

void test_slow_timer_gets_a_prescaler_that_fits_16_bits() {
  FanConfig cfg = pwmConfig(FanMode::Proportional);
  cfg.pwm_frequency = 100;  // 168 MHz / 100 Hz needs PSC > 0
  FanController fan;
  fan.init(cfg);
  TEST_ASSERT_LESS_OR_EQUAL_UINT32(0xFFFF, TIM8->ARR);
  const uint32_t f = 168000000 / ((TIM8->PSC + 1) * (TIM8->ARR + 1));
  TEST_ASSERT_UINT32_WITHIN(1, 100, f);
}

void test_proportional_curve() {
  const FanConfig cfg = pwmConfig(FanMode::Proportional);
  FanController fan;
  fan.init(cfg);

  fan.update(30.0f);  // at temp_low: off
  TEST_ASSERT_EQUAL_UINT8(0, fan.currentDuty());
  TEST_ASSERT_FALSE(fan.isRunning());

  fan.update(30.1f);  // just above: duty_min
  TEST_ASSERT_EQUAL_UINT8(20, fan.currentDuty());
  TEST_ASSERT_TRUE(fan.isRunning());

  fan.update(42.5f);  // midpoint: 20 + 0.5 * 80
  TEST_ASSERT_EQUAL_UINT8(60, fan.currentDuty());
  TEST_ASSERT_EQUAL_UINT32(ccrFor(60), ccr());

  fan.update(55.0f);
  TEST_ASSERT_EQUAL_UINT8(100, fan.currentDuty());
  TEST_ASSERT_EQUAL_UINT32(kArr25kHz, ccr());

  fan.update(90.0f);
  TEST_ASSERT_EQUAL_UINT8(100, fan.currentDuty());
}

void test_unreadable_temperature_runs_the_fan_flat_out() {
  for (FanMode mode : {FanMode::Proportional, FanMode::Threshold}) {
    const FanConfig cfg = pwmConfig(mode);
    FanController fan;
    fan.init(cfg);
    fan.update(NAN);
    TEST_ASSERT_EQUAL_UINT8(100, fan.currentDuty());
    TEST_ASSERT_TRUE(fan.isRunning());
    fan.update(INFINITY);
    TEST_ASSERT_EQUAL_UINT8(100, fan.currentDuty());
  }
}

void test_threshold_mode_has_hysteresis() {
  const FanConfig cfg = pwmConfig(FanMode::Threshold);
  FanController fan;
  fan.init(cfg);

  fan.update(54.9f);
  TEST_ASSERT_FALSE(fan.isRunning());
  fan.update(55.0f);
  TEST_ASSERT_TRUE(fan.isRunning());
  TEST_ASSERT_EQUAL_UINT8(100, fan.currentDuty());
  fan.update(40.0f);  // between the thresholds: keeps running
  TEST_ASSERT_TRUE(fan.isRunning());
  fan.update(30.0f);
  TEST_ASSERT_FALSE(fan.isRunning());
  TEST_ASSERT_EQUAL_UINT32(0, ccr());
  fan.update(40.0f);  // and stays off until temp_high again
  TEST_ASSERT_FALSE(fan.isRunning());
}

void test_set_duty_clamps_and_tracks_running() {
  const FanConfig cfg = pwmConfig(FanMode::Proportional);
  FanController fan;
  fan.setDuty(50);  // before init: ignored
  TEST_ASSERT_EQUAL_UINT8(0, fan.currentDuty());
  fan.init(cfg);
  fan.setDuty(150);
  TEST_ASSERT_EQUAL_UINT8(100, fan.currentDuty());
  TEST_ASSERT_TRUE(fan.isRunning());
  fan.setDuty(0);
  TEST_ASSERT_FALSE(fan.isRunning());
}

void test_stop_and_destructor_release_the_timer() {
  const FanConfig cfg = pwmConfig(FanMode::Proportional);
  {
    FanController fan;
    fan.init(cfg);
    fan.update(55.0f);
    fan.stop();
    TEST_ASSERT_EQUAL(0, fake_tim_hal.pwmn_started);
    TEST_ASSERT_EQUAL_UINT32(0, ccr());
    TEST_ASSERT_FALSE(fan.isRunning());
  }
  TEST_ASSERT_TRUE(fake_tim_hal.deinit);
}

void test_always_on_drives_the_pin_and_ignores_temperature() {
  const FanConfig cfg{.pin = platform::Pin{platform::Port::C, 13},
                      .mode = FanMode::AlwaysOn};
  {
    FanController fan;
    fan.init(cfg);
    fake_gpio::latch(kPortC);
    TEST_ASSERT_TRUE(fake_gpio::outputHigh(kPortC, 13));
    TEST_ASSERT_EQUAL_UINT8(100, fan.currentDuty());
    fan.update(0.0f);
    fan.setDuty(0);
    TEST_ASSERT_EQUAL_UINT8(100, fan.currentDuty());
    TEST_ASSERT_EQUAL(0, fake_tim_hal.pwm_started + fake_tim_hal.pwmn_started);
    fan.stop();
    fake_gpio::latch(kPortC);
    TEST_ASSERT_FALSE(fake_gpio::outputHigh(kPortC, 13));
  }
  TEST_ASSERT_FALSE(fake_tim_hal.deinit);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_pwm_init_runs_complementary_output_at_25khz_off);
  RUN_TEST(test_slow_timer_gets_a_prescaler_that_fits_16_bits);
  RUN_TEST(test_proportional_curve);
  RUN_TEST(test_unreadable_temperature_runs_the_fan_flat_out);
  RUN_TEST(test_threshold_mode_has_hysteresis);
  RUN_TEST(test_set_duty_clamps_and_tracks_running);
  RUN_TEST(test_stop_and_destructor_release_the_timer);
  RUN_TEST(test_always_on_drives_the_pin_and_ignores_temperature);
  return UNITY_END();
}
