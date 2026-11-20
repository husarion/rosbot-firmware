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
#include <cstdio>
#include <vector>

#include "fake_gpio.hpp"
#include "led_indicator.cpp"

namespace {

uint32_t s_now_ms = 0;

constexpr uint8_t kPortE = 4;
constexpr uint8_t kPin = 2;
constexpr uint16_t kBlinkMs = 500;

// Morse SOS in ms: dot 200, dash 600, gap 200, letter gap 600, repeat 1400.
const std::vector<std::pair<bool, uint32_t>> kSos = {
    {true, 200},  {false, 200}, {true, 200},  {false, 200}, {true, 200},
    {false, 600}, {true, 600},  {false, 200}, {true, 600},  {false, 200},
    {true, 600},  {false, 600}, {true, 200},  {false, 200}, {true, 200},
    {false, 200}, {true, 200},  {false, 1400}};

uint32_t sosCycleMs() {
  uint32_t total = 0;
  for (const auto& step : kSos) total += step.second;
  return total;
}

bool sosLevelAt(uint32_t t) {
  t %= sosCycleMs();
  for (const auto& step : kSos) {
    if (t < step.second) return step.first;
    t -= step.second;
  }
  return false;
}

LedIndicator makeIndicator(bool initial_on = false) {
  return LedIndicator(LedIndicatorConfig{platform::Pin{platform::Port::E, kPin},
                                         initial_on, kBlinkMs, "TEST"});
}

bool led() { return fake_gpio::outputHigh(kPortE, kPin); }

void update(LedIndicator& ind, bool battery_low, bool disconnected,
            bool error) {
  ind.update(battery_low, disconnected, error);
  fake_gpio::latch(kPortE);
}

}  // namespace

uint64_t time_boot_us() { return static_cast<uint64_t>(s_now_ms) * 1000U; }

void setUp() {
  fake_gpio::reset();
  s_now_ms = 0;
}
void tearDown() {}

void test_init_drives_initial_level() {
  auto ind = makeIndicator(true);
  ind.init();
  fake_gpio::latch(kPortE);
  TEST_ASSERT_TRUE(led());
}

void test_all_ok_is_off_and_disconnected_is_solid_on() {
  auto ind = makeIndicator(true);
  ind.init();
  update(ind, false, true, false);
  TEST_ASSERT_TRUE(led());
  s_now_ms = 10000;
  update(ind, false, true, false);
  TEST_ASSERT_TRUE(led());
  update(ind, false, false, false);
  TEST_ASSERT_FALSE(led());
}

void test_battery_low_blinks_and_wins_over_disconnected() {
  auto ind = makeIndicator(false);
  ind.init();
  bool previous = led();
  int toggles = 0;
  for (s_now_ms = 1; s_now_ms <= 5 * kBlinkMs; ++s_now_ms) {
    update(ind, true, true, false);
    if (led() != previous) {
      ++toggles;
      TEST_ASSERT_EQUAL_UINT32(0, s_now_ms % kBlinkMs);
      previous = led();
    }
  }
  TEST_ASSERT_EQUAL(5, toggles);
}

void test_error_plays_sos_pattern_while_it_lasts() {
  auto ind = makeIndicator(false);
  ind.init();
  const uint32_t span = 3 * sosCycleMs();
  for (s_now_ms = 0; s_now_ms < span; ++s_now_ms) {
    update(ind, true, true, true);
    char msg[32];
    snprintf(msg, sizeof(msg), "t=%u ms", static_cast<unsigned>(s_now_ms));
    TEST_ASSERT_EQUAL_MESSAGE(sosLevelAt(s_now_ms), led(), msg);
  }
}

void test_short_error_still_shows_two_full_cycles() {
  auto ind = makeIndicator(false);
  ind.init();
  update(ind, false, false, true);
  const uint32_t two_cycles = 2 * sosCycleMs();
  for (s_now_ms = 1; s_now_ms < two_cycles; ++s_now_ms) {
    update(ind, false, false, false);
    TEST_ASSERT_EQUAL(sosLevelAt(s_now_ms), led());
  }
  update(ind, false, false, false);
  TEST_ASSERT_FALSE(led());
  // Back to normal priorities afterwards.
  s_now_ms += 1;
  update(ind, false, true, false);
  TEST_ASSERT_TRUE(led());
}

void test_calibrating_blinks_fast_then_turns_off() {
  auto ind = makeIndicator(false);
  ind.init();
  bool previous = led();
  int toggles = 0;
  for (s_now_ms = 1; s_now_ms <= 1000; ++s_now_ms) {
    ind.calibrating(false);
    fake_gpio::latch(kPortE);
    if (led() != previous) {
      ++toggles;
      previous = led();
    }
  }
  TEST_ASSERT_EQUAL(10, toggles);
  ind.calibrating(true);
  fake_gpio::latch(kPortE);
  TEST_ASSERT_FALSE(led());
}

void test_calibrating_cancels_a_running_sos() {
  auto ind = makeIndicator(false);
  ind.init();
  update(ind, false, false, true);
  TEST_ASSERT_TRUE(led());
  s_now_ms = 50;
  ind.calibrating(true);
  // The SOS would still be in its first dot here; after calibration ends
  // the indicator follows the plain priorities again.
  update(ind, false, false, false);
  TEST_ASSERT_FALSE(led());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_init_drives_initial_level);
  RUN_TEST(test_all_ok_is_off_and_disconnected_is_solid_on);
  RUN_TEST(test_battery_low_blinks_and_wins_over_disconnected);
  RUN_TEST(test_error_plays_sos_pattern_while_it_lasts);
  RUN_TEST(test_short_error_still_shows_two_full_cycles);
  RUN_TEST(test_calibrating_blinks_fast_then_turns_off);
  RUN_TEST(test_calibrating_cancels_a_running_sos);
  return UNITY_END();
}
