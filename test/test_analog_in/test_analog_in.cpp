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

// lib/platform is not built for env:native, see test_pwm.
#include "analog_in.cpp"

uint32_t g_delay_us = 0;
void platform::clock::delayUs(uint32_t us) { g_delay_us += us; }

void setUp() {
  std::memset(&fake_adc1, 0, sizeof(fake_adc1));
  std::memset(&fake_adc_common, 0, sizeof(fake_adc_common));
  std::memset(fake_gpio_memory, 0, sizeof(fake_gpio_memory));
  fake_rcc.APB2ENR = 0;
  fake_primask = 0;
  g_delay_us = 0;
}
void tearDown() {}

// XL current sense, PC3 = ADC1_IN13.
const platform::AnalogIn kPc3(platform::Pin(platform::Port::C, 3), 13);

void test_init_matches_arduino_adc_setup() {
  kPc3.init();
  TEST_ASSERT_EQUAL_HEX32(RCC_APB2ENR_ADC1EN, fake_rcc.APB2ENR);
  TEST_ASSERT_EQUAL_HEX32(0b01UL << 16, fake_adc_common.CCR);  // PCLK2/4
  TEST_ASSERT_EQUAL_HEX32(0b01UL << 24, fake_adc1.CR1);        // 10-bit
  TEST_ASSERT_EQUAL_HEX32(ADC_CR2_ADON, fake_adc1.CR2);
  TEST_ASSERT_EQUAL_HEX32(0b001UL << 9, fake_adc1.SMPR1);  // IN13: 15 cyc
  TEST_ASSERT_EQUAL_UINT32(3, g_delay_us);
  auto* gpioc = reinterpret_cast<GPIO_TypeDef*>(GPIOA_BASE + 2 * 0x400);
  TEST_ASSERT_EQUAL_HEX32(0b11UL << 6, gpioc->MODER);
}

void test_second_channel_keeps_adc_and_sets_own_sampling() {
  kPc3.init();
  const platform::AnalogIn pa5(platform::Pin(platform::Port::A, 5), 5);
  pa5.init();
  TEST_ASSERT_EQUAL_UINT32(3, g_delay_us);  // ADC powered up only once
  TEST_ASSERT_EQUAL_HEX32(0b001UL << 15, fake_adc1.SMPR2);
  TEST_ASSERT_EQUAL_HEX32(0b001UL << 9, fake_adc1.SMPR1);
}

void test_read_converts_selected_channel() {
  kPc3.init();
  fake_adc1.SR = ADC_SR_EOC;
  fake_adc1.DR = 517;
  TEST_ASSERT_EQUAL_UINT16(517, kPc3.read());
  TEST_ASSERT_EQUAL_UINT32(13, fake_adc1.SQR3);
  TEST_ASSERT_TRUE(fake_adc1.CR2 & ADC_CR2_SWSTART);
  TEST_ASSERT_EQUAL_UINT32(0, fake_primask);
}

void test_read_returns_zero_when_conversion_never_ends() {
  kPc3.init();
  fake_adc1.SR = 0;
  fake_adc1.DR = 517;
  TEST_ASSERT_EQUAL_UINT16(0, kPc3.read());
  TEST_ASSERT_EQUAL_UINT32(0, fake_primask);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_init_matches_arduino_adc_setup);
  RUN_TEST(test_second_channel_keeps_adc_and_sets_own_sampling);
  RUN_TEST(test_read_converts_selected_channel);
  RUN_TEST(test_read_returns_zero_when_conversion_never_ends);
  return UNITY_END();
}
