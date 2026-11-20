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

#include <vector>

#include "fake_gpio.hpp"
#include "i2c_bus_clear.hpp"

namespace {

// Separate ports: the fake BSRR keeps only the last write per port until
// latch(), unlike hardware.
constexpr uint8_t kSclPort = 1;  // PB6
constexpr uint8_t kSdaPort = 2;  // PC7
constexpr uint8_t kScl = 6;
constexpr uint8_t kSda = 7;
const platform::Pin kSclPin{platform::Port::B, kScl};
const platform::Pin kSdaPin{platform::Port::C, kSda};

// A slave stuck mid-byte: holds SDA low until it has seen this many SCL
// rising edges.
int s_release_after = 0;
int s_rising_edges = 0;
bool s_scl_was_high = true;
struct Sample {
  bool scl;
  bool sda;
};
std::vector<Sample> s_trace;

// Open-drain lines with pull-ups: a line reads high unless someone pulls.
void halfPeriod() {
  fake_gpio::latch(kSclPort);
  fake_gpio::latch(kSdaPort);
  const bool scl = fake_gpio::outputHigh(kSclPort, kScl);
  if (scl && !s_scl_was_high) ++s_rising_edges;
  s_scl_was_high = scl;
  const bool slave_holds_sda = s_rising_edges < s_release_after;
  const bool sda = fake_gpio::outputHigh(kSdaPort, kSda) && !slave_holds_sda;
  fake_gpio::setInput(kSclPort, kScl, scl);
  fake_gpio::setInput(kSdaPort, kSda, sda);
  s_trace.push_back({scl, sda});
}

int sclPulsesBeforeStop() {
  // The STOP adds one SCL low/high of its own at the end.
  int pulses = 0;
  for (size_t i = 1; i < s_trace.size(); ++i) {
    if (s_trace[i].scl && !s_trace[i - 1].scl) ++pulses;
  }
  return pulses - 1;
}

void assertEndsWithStop() {
  const size_t n = s_trace.size();
  TEST_ASSERT_GREATER_OR_EQUAL(2, n);
  // Last two samples: SCL high with SDA low, then SDA released.
  TEST_ASSERT_TRUE(s_trace[n - 2].scl);
  TEST_ASSERT_FALSE(s_trace[n - 2].sda);
  TEST_ASSERT_TRUE(s_trace[n - 1].scl);
  TEST_ASSERT_TRUE(s_trace[n - 1].sda);
}

bool run(int release_after) {
  s_release_after = release_after;
  // The stuck slave already pulls SDA low before the first half period.
  fake_gpio::setInput(kSclPort, kScl, true);
  fake_gpio::setInput(kSdaPort, kSda, release_after == 0);
  return platform::clearI2cBus(kSclPin, kSdaPin, halfPeriod);
}

}  // namespace

void setUp() {
  fake_gpio::reset();
  s_rising_edges = 0;
  s_scl_was_high = true;
  s_trace.clear();
}
void tearDown() {}

void test_lines_become_open_drain_outputs() {
  run(0);
  TEST_ASSERT_EQUAL_HEX32(1U << kScl, fake_gpio::port(kSclPort)->OTYPER);
  TEST_ASSERT_EQUAL_HEX32(1U << kSda, fake_gpio::port(kSdaPort)->OTYPER);
  TEST_ASSERT_EQUAL_HEX32(0b01U << (2 * kScl),
                          fake_gpio::port(kSclPort)->MODER);
  TEST_ASSERT_EQUAL_HEX32(0b01U << (2 * kSda),
                          fake_gpio::port(kSdaPort)->MODER);
}

void test_idle_bus_gets_only_a_stop() {
  TEST_ASSERT_TRUE(run(0));
  TEST_ASSERT_EQUAL(0, sclPulsesBeforeStop());
  assertEndsWithStop();
}

void test_clocks_until_the_slave_releases_sda() {
  TEST_ASSERT_TRUE(run(3));
  TEST_ASSERT_EQUAL(3, sclPulsesBeforeStop());
  assertEndsWithStop();
}

void test_gives_up_after_nine_pulses() {
  TEST_ASSERT_FALSE(run(100));
  TEST_ASSERT_EQUAL(9, sclPulsesBeforeStop());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_lines_become_open_drain_outputs);
  RUN_TEST(test_idle_bus_gets_only_a_stop);
  RUN_TEST(test_clocks_until_the_slave_releases_sda);
  RUN_TEST(test_gives_up_after_nine_pulses);
  return UNITY_END();
}
