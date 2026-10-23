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

#include <cstdint>

#include "STM32FreeRTOS.h"
#include "time_base.hpp"

namespace {

uint32_t s_micros = 0;
// Runs once inside the next critical section, i.e. after time_boot_us()
// has already read micros() — the window a preempting task would hit.
void (*s_on_enter_critical)() = nullptr;

void advance(uint32_t us) {
  // time_boot_us() only accepts steps below 2^31 us.
  while (us > 0) {
    const uint32_t step = us > 0x40000000u ? 0x40000000u : us;
    s_micros += step;
    us -= step;
    time_boot_us();
  }
}

}  // namespace

uint32_t micros() { return s_micros; }

UBaseType_t testEnterCritical() {
  if (s_on_enter_critical != nullptr) {
    auto hook = s_on_enter_critical;
    s_on_enter_critical = nullptr;
    hook();
  }
  return 0;
}

void testExitCritical(UBaseType_t /*mask*/) {}

void setUp() {}
void tearDown() {}

void test_follows_micros() {
  const uint64_t t0 = time_boot_us();
  advance(500);
  TEST_ASSERT_EQUAL_UINT64(t0 + 500, time_boot_us());
}

void test_survives_micros_wrap() {
  advance(0xFFFFFF00u - s_micros);
  const uint64_t before = time_boot_us();
  advance(0x200);  // micros() wraps to 0x100
  TEST_ASSERT_EQUAL_UINT32(0x100u, s_micros);
  TEST_ASSERT_EQUAL_UINT64(before + 0x200, time_boot_us());
}

void test_stale_reading_does_not_move_the_clock() {
  const uint64_t t0 = time_boot_us();
  s_micros -= 1000;
  TEST_ASSERT_EQUAL_UINT64(t0, time_boot_us());
  s_micros += 1010;
  TEST_ASSERT_EQUAL_UINT64(t0 + 10, time_boot_us());
}

void test_preemption_does_not_double_count() {
  const uint64_t t0 = time_boot_us();
  s_micros += 100;
  // A higher-priority task runs between the outer micros() read and its
  // critical section, 100 us later.
  s_on_enter_critical = [] {
    s_micros += 100;
    time_boot_us();
  };
  const uint64_t outer = time_boot_us();
  TEST_ASSERT_EQUAL_UINT64(t0 + 200, outer);
  s_micros += 100;
  TEST_ASSERT_EQUAL_UINT64(t0 + 300, time_boot_us());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_follows_micros);
  RUN_TEST(test_survives_micros_wrap);
  RUN_TEST(test_stale_reading_does_not_move_the_clock);
  RUN_TEST(test_preemption_does_not_double_count);
  return UNITY_END();
}
