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

#include "boot_option.cpp"
#include "fake_gpio.hpp"

namespace {

constexpr uint8_t kPortE = 4;
// Second LED on its own port: the fake BSRR keeps only the last write
// per port until latch(), unlike hardware.
constexpr uint8_t kPortF = 5;
constexpr uint8_t kButton1 = 0;
constexpr uint8_t kButton2 = 1;
constexpr uint8_t kGreen1 = 5;
constexpr uint8_t kGreen2 = 6;

const platform::Pin kButtons[] = {{platform::Port::E, kButton1},
                                  {platform::Port::E, kButton2}};
const platform::Pin kGreens[] = {{platform::Port::E, kGreen1},
                                 {platform::Port::F, kGreen2}};

uint32_t s_now_ms = 0;
// Button 1 held (low) during [press, release); UINT32_MAX = never.
uint32_t s_press_ms = UINT32_MAX;
uint32_t s_release_ms = UINT32_MAX;
uint8_t s_pressed_button = kButton1;
int s_green_rising_edges = 0;
bool s_green_was_on = false;

void applyButtons() {
  const bool held = s_now_ms >= s_press_ms && s_now_ms < s_release_ms;
  fake_gpio::setInput(kPortE, kButton1, true);
  fake_gpio::setInput(kPortE, kButton2, true);
  if (held) fake_gpio::setInput(kPortE, s_pressed_button, false);
}

void observeGreens() {
  fake_gpio::latch(kPortE);
  fake_gpio::latch(kPortF);
  const bool on = fake_gpio::outputHigh(kPortE, kGreen1);
  TEST_ASSERT_EQUAL(on, fake_gpio::outputHigh(kPortF, kGreen2));
  if (on && !s_green_was_on) ++s_green_rising_edges;
  s_green_was_on = on;
}

BootOptionConfig config() {
  BootOptionConfig cfg;
  cfg.buttons = kButtons;
  cfg.green_leds = kGreens;
  return cfg;  // calibration_hold_ms 3000, press_detect_window_ms 1500
}

BootAction run() {
  applyButtons();
  const BootAction action = resolveBootAction(config());
  observeGreens();
  return action;
}

}  // namespace

uint64_t time_boot_us() { return static_cast<uint64_t>(s_now_ms) * 1000U; }

namespace platform::clock {
void delayMs(uint32_t ms) {
  observeGreens();
  s_now_ms += ms;
  applyButtons();
}
}  // namespace platform::clock

void setUp() {
  fake_gpio::reset();
  s_now_ms = 0;
  s_press_ms = UINT32_MAX;
  s_release_ms = UINT32_MAX;
  s_pressed_button = kButton1;
  s_green_rising_edges = 0;
  s_green_was_on = false;
}
void tearDown() {}

void test_no_press_gives_up_after_detect_window() {
  TEST_ASSERT_EQUAL(BootAction::kNone, run());
  TEST_ASSERT_UINT32_WITHIN(10, 1500, s_now_ms);
  TEST_ASSERT_GREATER_OR_EQUAL_UINT32(1500, s_now_ms);
  TEST_ASSERT_EQUAL(0, s_green_rising_edges);
}

void test_tap_selects_transport_and_latches_green() {
  s_press_ms = 0;
  s_release_ms = 200;
  TEST_ASSERT_EQUAL(BootAction::kChangeTransport, run());
  TEST_ASSERT_UINT32_WITHIN(10, 200, s_now_ms);
  TEST_ASSERT_TRUE(fake_gpio::outputHigh(kPortE, kGreen1));
}

void test_press_starting_late_in_window_still_counts() {
  s_press_ms = 1400;
  s_release_ms = 1600;
  TEST_ASSERT_EQUAL(BootAction::kChangeTransport, run());
}

void test_press_after_window_is_ignored() {
  s_press_ms = 1600;
  s_release_ms = 1800;
  TEST_ASSERT_EQUAL(BootAction::kNone, run());
}

void test_either_button_qualifies() {
  s_pressed_button = kButton2;
  s_press_ms = 0;
  s_release_ms = 100;
  TEST_ASSERT_EQUAL(BootAction::kChangeTransport, run());
}

void test_hold_enters_calibration_without_waiting_for_release() {
  s_press_ms = 0;
  s_release_ms = 10000;
  TEST_ASSERT_EQUAL(BootAction::kCalibrateImu, run());
  // Decided at 3 s, then 3 confirm blinks of 150 ms on + 150 ms off.
  TEST_ASSERT_UINT32_WITHIN(10, 3000 + 3 * 300, s_now_ms);
  TEST_ASSERT_EQUAL(3, s_green_rising_edges);
  TEST_ASSERT_FALSE(fake_gpio::outputHigh(kPortE, kGreen1));
}

void test_release_just_before_threshold_is_a_tap() {
  s_press_ms = 0;
  s_release_ms = 2990;
  TEST_ASSERT_EQUAL(BootAction::kChangeTransport, run());
  TEST_ASSERT_EQUAL(1, s_green_rising_edges);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_no_press_gives_up_after_detect_window);
  RUN_TEST(test_tap_selects_transport_and_latches_green);
  RUN_TEST(test_press_starting_late_in_window_still_counts);
  RUN_TEST(test_press_after_window_is_ignored);
  RUN_TEST(test_either_button_qualifies);
  RUN_TEST(test_hold_enters_calibration_without_waiting_for_release);
  RUN_TEST(test_release_just_before_threshold_is_a_tap);
  return UNITY_END();
}
