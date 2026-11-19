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

#include "uart_math.hpp"

void setUp() {}
void tearDown() {}

// Reference: HAL UART_BRR_SAMPLING16 (mantissa << 4 | rounded fraction).
uint32_t halBrr(uint32_t pclk, uint32_t baud) {
  const uint32_t div100 = (pclk * 25U) / (4U * baud);
  const uint32_t mant = div100 / 100U;
  const uint32_t frac = ((div100 - mant * 100U) * 16U + 50U) / 100U;
  return (mant << 4) + (frac & 0xF0U) + (frac & 0x0FU);
}

void test_brr_matches_hal_for_board_uarts() {
  // USART1 on APB2 (84 MHz), USART2/3 on APB1 (42 MHz).
  TEST_ASSERT_EQUAL_UINT32(91, platform::uartBrr(84000000, 921600));
  TEST_ASSERT_EQUAL_UINT32(45, platform::uartBrr(42000000, 921600));
  TEST_ASSERT_EQUAL_UINT32(1094, platform::uartBrr(42000000, 38400));
  TEST_ASSERT_EQUAL_UINT32(halBrr(84000000, 921600),
                           platform::uartBrr(84000000, 921600));
  TEST_ASSERT_EQUAL_UINT32(halBrr(42000000, 921600),
                           platform::uartBrr(42000000, 921600));
  TEST_ASSERT_EQUAL_UINT32(halBrr(42000000, 38400),
                           platform::uartBrr(42000000, 38400));
  TEST_ASSERT_EQUAL_UINT32(halBrr(84000000, 115200),
                           platform::uartBrr(84000000, 115200));
}

void test_rx_available_follows_ndtr() {
  TEST_ASSERT_EQUAL(0u, platform::circularRxAvailable(512, 512, 0));
  TEST_ASSERT_EQUAL(10u, platform::circularRxAvailable(512, 502, 0));
  TEST_ASSERT_EQUAL(4u, platform::circularRxAvailable(512, 502, 6));
}

void test_rx_available_across_wrap() {
  // Head wrapped to 3, reader still at 510.
  TEST_ASSERT_EQUAL(5u, platform::circularRxAvailable(512, 509, 510));
  // NDTR reads 0 for an instant before reload: head is 0.
  TEST_ASSERT_EQUAL(2u, platform::circularRxAvailable(512, 0, 510));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_brr_matches_hal_for_board_uarts);
  RUN_TEST(test_rx_available_follows_ndtr);
  RUN_TEST(test_rx_available_across_wrap);
  return UNITY_END();
}
