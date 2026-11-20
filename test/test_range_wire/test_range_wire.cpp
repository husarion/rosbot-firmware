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

#include "range_wire.hpp"

void setUp() {}
void tearDown() {}

void test_nan_goes_out_as_invalid_signal() {
  const auto r = range_wire::encode(NAN);
  TEST_ASSERT_EQUAL_UINT16(0, r.current_cm);
  TEST_ASSERT_EQUAL_UINT8(range_wire::kSignalQualityInvalid, r.signal_quality);
}

void test_a_reading_goes_out_in_cm() {
  const auto r = range_wire::encode(0.42f);
  TEST_ASSERT_EQUAL_UINT16(42, r.current_cm);
  TEST_ASSERT_EQUAL_UINT8(range_wire::kSignalQualityUnknown, r.signal_quality);
}

void test_out_of_range_values_stay_valid() {
  TEST_ASSERT_EQUAL_UINT16(UINT16_MAX - 1, range_wire::encode(1e6f).current_cm);
  TEST_ASSERT_EQUAL_UINT8(range_wire::kSignalQualityInvalid,
                          range_wire::encode(-1.0f).signal_quality);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_nan_goes_out_as_invalid_signal);
  RUN_TEST(test_a_reading_goes_out_in_cm);
  RUN_TEST(test_out_of_range_values_stay_valid);
  return UNITY_END();
}
