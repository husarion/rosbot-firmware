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

#include "pin.hpp"

namespace {

constexpr uintptr_t kPortStride = 0x400;

GPIO_TypeDef* fakePort(uint8_t port_index) {
  return reinterpret_cast<GPIO_TypeDef*>(GPIOA_BASE + port_index * kPortStride);
}

}  // namespace

void setUp() {
  std::memset(fake_gpio_memory, 0, sizeof(fake_gpio_memory));
  fake_rcc.AHB1ENR = 0;
  fake_primask = 0;
  fake_irq_disable_count = 0;
}
void tearDown() {}

void test_output_sets_level_mode_and_clock() {
  const platform::Pin pin(GPIOA_BASE + 2 * kPortStride, 5);  // PC5
  GPIO_TypeDef* gpio = fakePort(2);
  gpio->MODER = 0xFFFFFFFF;  // analog on every line
  gpio->OTYPER = 0xFFFF;

  pin.setAsOutput(true);

  TEST_ASSERT_EQUAL_HEX32(1UL << 5, gpio->BSRR);
  TEST_ASSERT_EQUAL_HEX32(0xFFFFF7FF, gpio->MODER);
  TEST_ASSERT_EQUAL_HEX32(0xFFDF, gpio->OTYPER);
  TEST_ASSERT_EQUAL_HEX32(0b11UL << 10, gpio->OSPEEDR);
  TEST_ASSERT_EQUAL_HEX32(1UL << 2, fake_rcc.AHB1ENR);
}

void test_output_low_uses_reset_half_of_bsrr() {
  const platform::Pin pin(GPIOA_BASE, 15);
  pin.setAsOutput();
  TEST_ASSERT_EQUAL_HEX32(1UL << 31, fakePort(0)->BSRR);
}

void test_input_pull_touches_only_its_line() {
  const platform::Pin pin(GPIOA_BASE + 6 * kPortStride, 3);  // PG3
  GPIO_TypeDef* gpio = fakePort(6);
  gpio->MODER = 0x55555555;  // all outputs
  gpio->PUPDR = 0xAAAAAAAA;  // all pull-down

  pin.setAsInput(platform::Pull::kUp);

  TEST_ASSERT_EQUAL_HEX32(0x55555515, gpio->MODER);
  TEST_ASSERT_EQUAL_HEX32(0xAAAAAA6A, gpio->PUPDR);
  TEST_ASSERT_EQUAL_HEX32(1UL << 6, fake_rcc.AHB1ENR);
}

void test_analog_mode() {
  const platform::Pin pin(GPIOA_BASE + kPortStride, 0);
  pin.setAsAnalog();
  TEST_ASSERT_EQUAL_HEX32(0b11, fakePort(1)->MODER);
  TEST_ASSERT_EQUAL_HEX32(0, fakePort(1)->PUPDR);
}

void test_read_follows_idr() {
  const platform::Pin pin(GPIOA_BASE, 7);
  fakePort(0)->IDR = 1UL << 7;
  TEST_ASSERT_TRUE(pin.read());
  fakePort(0)->IDR = ~(UINT32_C(1) << 7);
  TEST_ASSERT_FALSE(pin.read());
}

void test_configure_masks_irqs_and_restores_primask() {
  const platform::Pin pin(GPIOA_BASE, 1);

  pin.setAsInput();
  TEST_ASSERT_EQUAL(1, fake_irq_disable_count);
  TEST_ASSERT_EQUAL_UINT32(0, fake_primask);

  fake_primask = 1;  // caller already in a critical section
  pin.setAsInput();
  TEST_ASSERT_EQUAL_UINT32(1, fake_primask);
}

void test_port_constructor_matches_base() {
  const platform::Pin by_port(platform::Port::G, 3);
  const platform::Pin by_base(GPIOA_BASE + 6 * kPortStride, 3);
  TEST_ASSERT_TRUE(by_port == by_base);
}

void test_toggle_inverts_output_latch() {
  const platform::Pin pin(platform::Port::B, 4);
  fakePort(1)->ODR = 0;
  pin.toggle();
  TEST_ASSERT_EQUAL_HEX32(1UL << 4, fakePort(1)->BSRR);
  fakePort(1)->ODR = 1UL << 4;
  pin.toggle();
  TEST_ASSERT_EQUAL_HEX32(1UL << 20, fakePort(1)->BSRR);
}

void test_alternate_sets_afr_speed_and_pull() {
  const platform::Pin low(platform::Port::F, 6);
  const platform::Pin high(platform::Port::F, 9);
  GPIO_TypeDef* gpio = fakePort(5);
  gpio->AFR[0] = 0xFFFFFFFF;
  gpio->AFR[1] = 0xFFFFFFFF;

  low.setAsAlternate(3, platform::Pull::kUp, platform::Speed::kHigh);
  high.setAsAlternate(9);

  TEST_ASSERT_EQUAL_HEX32(0xF3FFFFFF, gpio->AFR[0]);
  TEST_ASSERT_EQUAL_HEX32(0xFFFFFF9F, gpio->AFR[1]);
  TEST_ASSERT_EQUAL_HEX32((0b10UL << 12) | (0b10UL << 18), gpio->MODER);
  TEST_ASSERT_EQUAL_HEX32(0b01UL << 12, gpio->PUPDR);
  TEST_ASSERT_EQUAL_HEX32((0b10UL << 12) | (0b11UL << 18), gpio->OSPEEDR);
}

void test_open_drain_output_releases_line_before_driving_it() {
  const platform::Pin scl(GPIOA_BASE + kPortStride, 6);  // PB6
  GPIO_TypeDef* gpio = fakePort(1);
  scl.setAsOutputOpenDrain();
  TEST_ASSERT_EQUAL_HEX32(1UL << 6, gpio->BSRR);  // high = released
  TEST_ASSERT_EQUAL_HEX32(1UL << 6, gpio->OTYPER);
  TEST_ASSERT_EQUAL_HEX32(0b01UL << 12, gpio->MODER);
  TEST_ASSERT_EQUAL_HEX32(0, gpio->PUPDR);
}

void test_alternate_open_drain_sets_otyper() {
  const platform::Pin sda(platform::Port::C, 9);
  sda.setAsAlternateOpenDrain(4);
  GPIO_TypeDef* gpio = fakePort(2);
  TEST_ASSERT_EQUAL_HEX32(1UL << 9, gpio->OTYPER);
  TEST_ASSERT_EQUAL_HEX32(0x4UL << 4, gpio->AFR[1]);
  TEST_ASSERT_EQUAL_HEX32(0b10UL << 18, gpio->MODER);
  sda.setAsAlternate(4);
  TEST_ASSERT_EQUAL_HEX32(0, gpio->OTYPER);
}

void test_equality() {
  constexpr platform::Pin a(0x40020000, 3);
  constexpr platform::Pin b(0x40020000, 3);
  constexpr platform::Pin c(0x40020400, 3);
  static_assert(a == b, "same port and line");
  static_assert(a != c, "different port");
  TEST_ASSERT_TRUE(a == b);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_output_sets_level_mode_and_clock);
  RUN_TEST(test_output_low_uses_reset_half_of_bsrr);
  RUN_TEST(test_input_pull_touches_only_its_line);
  RUN_TEST(test_analog_mode);
  RUN_TEST(test_read_follows_idr);
  RUN_TEST(test_configure_masks_irqs_and_restores_primask);
  RUN_TEST(test_port_constructor_matches_base);
  RUN_TEST(test_toggle_inverts_output_latch);
  RUN_TEST(test_alternate_sets_afr_speed_and_pull);
  RUN_TEST(test_open_drain_output_releases_line_before_driving_it);
  RUN_TEST(test_alternate_open_drain_sets_otyper);
  RUN_TEST(test_equality);
  return UNITY_END();
}
