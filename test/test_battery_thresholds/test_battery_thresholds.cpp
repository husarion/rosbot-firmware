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

#include "battery_interface.hpp"

namespace {

class FakeBattery : public BatteryInterface {
 public:
  void init() override {}
  void update() override {}
  const char* name() const override { return "fake"; }
  void setPercentage(float p) { data_.percentage = p; }
};

}  // namespace

void setUp() {}
void tearDown() {}

void test_low_trips_below_threshold_and_clears_above_hysteresis() {
  FakeBattery b;
  b.setPercentage(0.20f);
  TEST_ASSERT_FALSE(b.isLow());  // strictly below 20 % to trip
  b.setPercentage(0.19f);
  TEST_ASSERT_TRUE(b.isLow());
  b.setPercentage(0.215f);  // inside the 2 % band: stays low
  TEST_ASSERT_TRUE(b.isLow());
  b.setPercentage(0.225f);
  TEST_ASSERT_FALSE(b.isLow());
}

void test_critical_has_its_own_state() {
  FakeBattery b;
  b.setPercentage(0.04f);
  TEST_ASSERT_TRUE(b.isCritical());
  TEST_ASSERT_TRUE(b.isLow());
  b.setPercentage(0.06f);
  TEST_ASSERT_TRUE(b.isCritical());
  b.setPercentage(0.08f);
  TEST_ASSERT_FALSE(b.isCritical());
  TEST_ASSERT_TRUE(b.isLow());
}

void test_unknown_percentage_never_trips() {
  FakeBattery b;  // percentage starts as NaN
  TEST_ASSERT_TRUE(std::isnan(b.getData().percentage));
  TEST_ASSERT_FALSE(b.isLow());
  TEST_ASSERT_FALSE(b.isCritical());
}

void test_nan_after_low_releases_the_latch() {
  FakeBattery b;
  b.setPercentage(0.10f);
  TEST_ASSERT_TRUE(b.isLow());
  b.setPercentage(NAN);
  TEST_ASSERT_FALSE(b.isLow());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_low_trips_below_threshold_and_clears_above_hysteresis);
  RUN_TEST(test_critical_has_its_own_state);
  RUN_TEST(test_unknown_percentage_never_trips);
  RUN_TEST(test_nan_after_low_releases_the_latch);
  return UNITY_END();
}
