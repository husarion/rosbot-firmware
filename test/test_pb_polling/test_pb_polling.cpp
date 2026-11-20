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

#include "pb_polling.hpp"

void setUp() {}
void tearDown() {}

void test_pb_before_1_0_3_is_polled_once_a_second() {
  TEST_ASSERT_EQUAL_UINT32(1000, pbBatteryPollPeriodMs("v1.0.2"));
  TEST_ASSERT_EQUAL_UINT32(1000, pbBatteryPollPeriodMs("v1.0.0"));
  TEST_ASSERT_EQUAL_UINT32(1000, pbBatteryPollPeriodMs("v0.9.9"));
  TEST_ASSERT_FALSE(pbSupportsFastPolling("v1.0.2"));
}

void test_unknown_version_is_polled_slowly() {
  TEST_ASSERT_EQUAL_UINT32(1000, pbBatteryPollPeriodMs(""));
  TEST_ASSERT_EQUAL_UINT32(1000, pbBatteryPollPeriodMs(nullptr));
  TEST_ASSERT_EQUAL_UINT32(1000, pbBatteryPollPeriodMs("garbage"));
}

void test_pb_1_0_3_and_newer_is_polled_every_tick() {
  TEST_ASSERT_EQUAL_UINT32(0, pbBatteryPollPeriodMs("v1.0.3"));
  TEST_ASSERT_EQUAL_UINT32(0, pbBatteryPollPeriodMs("v1.0.3-dirty"));
  TEST_ASSERT_EQUAL_UINT32(0, pbBatteryPollPeriodMs("v1.1.0"));
  TEST_ASSERT_TRUE(pbSupportsFastPolling("v1.0.10"));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_pb_before_1_0_3_is_polled_once_a_second);
  RUN_TEST(test_unknown_version_is_polled_slowly);
  RUN_TEST(test_pb_1_0_3_and_newer_is_polled_every_tick);
  return UNITY_END();
}
