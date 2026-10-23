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

#include "ntc_math.hpp"

namespace {

// rosbot_xl values (include/rosbot_xl/config.hpp).
NtcConfig xlConfig() {
  NtcConfig cfg{};
  cfg.pullup_resistance = 5230.0f;
  cfg.c1 = 1.112613927e-03f;
  cfg.c2 = 2.37277392e-04f;
  cfg.c3 = 7.1670e-08f;
  cfg.offset = 273.15f + 3.0f;
  cfg.adc_max = 1023.0f;
  return cfg;
}

}  // namespace

void setUp() {}
void tearDown() {}

void test_mid_scale_reads_room_temperature() {
  TEST_ASSERT_FLOAT_WITHIN(0.5f, 37.4f, ntcAdcToCelsius(xlConfig(), 512.0f));
}

void test_cold_end_is_still_valid() {
  TEST_ASSERT_FLOAT_WITHIN(0.5f, -36.6f, ntcAdcToCelsius(xlConfig(), 1000.0f));
}

void test_open_sensor_is_nan() {
  TEST_ASSERT_TRUE(std::isnan(ntcAdcToCelsius(xlConfig(), 1023.0f)));
}

void test_shorted_sensor_is_nan() {
  TEST_ASSERT_TRUE(std::isnan(ntcAdcToCelsius(xlConfig(), 0.0f)));
  TEST_ASSERT_TRUE(std::isnan(ntcAdcToCelsius(xlConfig(), 10.0f)));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_mid_scale_reads_room_temperature);
  RUN_TEST(test_cold_end_is_still_valid);
  RUN_TEST(test_open_sensor_is_nan);
  RUN_TEST(test_shorted_sensor_is_nan);
  return UNITY_END();
}
