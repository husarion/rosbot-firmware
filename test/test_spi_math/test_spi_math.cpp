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

#include "spi_math.hpp"

void setUp() {}
void tearDown() {}

void test_led_strip_gets_arduino_divider() {
  // SPI2 on APB1 (42 MHz), 4 MHz asked: /16 = 2.625 MHz, as measured on HW.
  TEST_ASSERT_EQUAL_UINT8(3, platform::spiBaudRateBits(42000000, 4000000));
}

void test_exact_and_saturating_dividers() {
  TEST_ASSERT_EQUAL_UINT8(0, platform::spiBaudRateBits(42000000, 21000000));
  TEST_ASSERT_EQUAL_UINT8(1, platform::spiBaudRateBits(42000000, 20999999));
  TEST_ASSERT_EQUAL_UINT8(7, platform::spiBaudRateBits(42000000, 1000));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_led_strip_gets_arduino_divider);
  RUN_TEST(test_exact_and_saturating_dividers);
  return UNITY_END();
}
