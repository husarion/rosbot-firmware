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

#include <initializer_list>

#include "battery_wire.hpp"

using battery_wire::healthToWire;
using battery_wire::temperatureToCentiDegrees;

void setUp() {}
void tearDown() {}

void test_temperature_is_centidegrees() {
  TEST_ASSERT_EQUAL_INT16(2750, temperatureToCentiDegrees(27.5f));
  TEST_ASSERT_EQUAL_INT16(-500, temperatureToCentiDegrees(-5.0f));
}

void test_unusable_temperature_is_unknown() {
  TEST_ASSERT_EQUAL_INT16(INT16_MAX, temperatureToCentiDegrees(NAN));
  TEST_ASSERT_EQUAL_INT16(INT16_MAX, temperatureToCentiDegrees(INFINITY));
  TEST_ASSERT_EQUAL_INT16(INT16_MAX, temperatureToCentiDegrees(400.0f));
}

void test_good_is_ok_without_faults() {
  const auto h = healthToWire(1);
  TEST_ASSERT_EQUAL_UINT8(battery_wire::kChargeStateOk, h.charge_state);
  TEST_ASSERT_EQUAL_UINT32(0, h.fault_bitmask);
}

void test_cold_sets_under_temperature_fault() {
  const auto h = healthToWire(6);
  TEST_ASSERT_EQUAL_UINT8(battery_wire::kChargeStateUnhealthy, h.charge_state);
  TEST_ASSERT_EQUAL_UINT32(battery_wire::kFaultUnderTemperature,
                           h.fault_bitmask);
}

void test_unspecified_failure_is_unhealthy_without_fault_bit() {
  const auto h = healthToWire(5);
  TEST_ASSERT_EQUAL_UINT8(battery_wire::kChargeStateUnhealthy, h.charge_state);
  TEST_ASSERT_EQUAL_UINT32(0, h.fault_bitmask);
}

void test_unknown_and_out_of_range_are_not_provided() {
  for (uint8_t v : {0, 9, 255}) {
    const auto h = healthToWire(v);
    TEST_ASSERT_EQUAL_UINT8(battery_wire::kChargeStateUndefined,
                            h.charge_state);
    TEST_ASSERT_EQUAL_UINT32(0, h.fault_bitmask);
  }
}

void test_current_keeps_sign_and_avoids_unknown_marker() {
  TEST_ASSERT_EQUAL_INT16(-150, battery_wire::currentToCentiAmps(-1.5f));
  TEST_ASSERT_EQUAL_INT16(256, battery_wire::currentToCentiAmps(2.56f));
  TEST_ASSERT_EQUAL_INT16(-2, battery_wire::currentToCentiAmps(-0.01f));
  TEST_ASSERT_EQUAL_INT16(0, battery_wire::currentToCentiAmps(0.0f));
  TEST_ASSERT_EQUAL_INT16(-1, battery_wire::currentToCentiAmps(NAN));
  TEST_ASSERT_EQUAL_INT16(INT16_MAX, battery_wire::currentToCentiAmps(1e6f));
  TEST_ASSERT_EQUAL_INT16(INT16_MIN + 1,
                          battery_wire::currentToCentiAmps(-1e6f));
}

void test_capacity_is_mah_or_zero() {
  TEST_ASSERT_EQUAL_UINT16(8850, battery_wire::capacityToMah(8.85f));
  TEST_ASSERT_EQUAL_UINT16(0, battery_wire::capacityToMah(NAN));
  TEST_ASSERT_EQUAL_UINT16(0, battery_wire::capacityToMah(-1.0f));
  TEST_ASSERT_EQUAL_UINT16(UINT16_MAX, battery_wire::capacityToMah(100.0f));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_temperature_is_centidegrees);
  RUN_TEST(test_unusable_temperature_is_unknown);
  RUN_TEST(test_good_is_ok_without_faults);
  RUN_TEST(test_cold_sets_under_temperature_fault);
  RUN_TEST(test_unspecified_failure_is_unhealthy_without_fault_bit);
  RUN_TEST(test_unknown_and_out_of_range_are_not_provided);
  RUN_TEST(test_current_keeps_sign_and_avoids_unknown_marker);
  RUN_TEST(test_capacity_is_mah_or_zero);
  return UNITY_END();
}
