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

#include "soc_estimator.hpp"

namespace {

// Pack voltage under load for a given true SoC: OCV minus I*R.
float loadedVoltage(float soc, float discharge_a, const SocEstimatorConfig& c) {
  float cell = 3.27f;
  for (float v = 3.27f; v <= 4.20f; v += 0.001f) {
    if (SocEstimator::ocvToSoc(v) >= soc) {
      cell = v;
      break;
    }
  }
  return cell * c.num_cells - discharge_a * c.r_internal_ohm;
}

}  // namespace

void setUp() {}
void tearDown() {}

void test_curve_endpoints_and_monotonic() {
  TEST_ASSERT_EQUAL_FLOAT(0.0f, SocEstimator::ocvToSoc(3.0f));
  TEST_ASSERT_EQUAL_FLOAT(1.0f, SocEstimator::ocvToSoc(4.3f));
  float prev = 0.0f;
  for (float v = 3.27f; v <= 4.20f; v += 0.01f) {
    const float s = SocEstimator::ocvToSoc(v);
    TEST_ASSERT_TRUE(s >= prev);
    prev = s;
  }
}

void test_first_sample_initialises_from_ocv() {
  SocEstimatorConfig c;
  SocEstimator e(c);
  TEST_ASSERT_TRUE(std::isnan(e.soc()));
  const float s = e.update(3.84f * 3, 0.0f, 0.1f);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.50f, s);
}

void test_load_step_does_not_jump() {
  SocEstimatorConfig c;
  SocEstimator e(c);
  const float before = [&] {
    float s = 0;
    for (int i = 0; i < 600; ++i) {
      s = e.update(loadedVoltage(0.60f, 0.5f, c), -0.5f, 0.1f);
    }
    return s;
  }();

  // Sudden 8 A load: voltage sags by ~0.8 V. A linear voltage map would drop
  // by ~25 points; the estimator must stay within a few.
  float worst = 0.0f;
  for (int i = 0; i < 100; ++i) {
    const float s = e.update(loadedVoltage(0.60f, 8.0f, c), -8.0f, 0.1f);
    worst = fmaxf(worst, fabsf(s - before));
  }
  TEST_ASSERT_TRUE(worst < 0.03f);
}

void test_discharge_integrates_current() {
  SocEstimatorConfig c;
  SocEstimator e(c);
  e.update(loadedVoltage(0.80f, 0.0f, c), 0.0f, 0.1f);
  // 1 h at 1 A out of 7.8 Ah is ~12.8 points, with the OCV term held neutral
  // by feeding the matching voltage.
  float s = 0.0f;
  for (int i = 0; i < 3600; ++i) {
    const float truth = 0.80f - (static_cast<float>(i) / 3600.0f) / 7.8f;
    s = e.update(loadedVoltage(truth, 1.0f, c), -1.0f, 1.0f);
  }
  TEST_ASSERT_FLOAT_WITHIN(0.03f, 0.80f - 1.0f / 7.8f, s);
}

void test_rest_converges_to_ocv() {
  SocEstimatorConfig c;
  SocEstimator e(c);
  e.update(3.60f * 3, 0.0f, 0.1f);
  float s = 0.0f;
  for (int i = 0; i < 1200; ++i) s = e.update(3.84f * 3, 0.0f, 1.0f);
  TEST_ASSERT_FLOAT_WITHIN(0.02f, 0.50f, s);
}

void test_bad_input_is_ignored() {
  SocEstimatorConfig c;
  SocEstimator e(c);
  const float s0 = e.update(11.5f, -1.0f, 0.1f);
  TEST_ASSERT_EQUAL_FLOAT(s0, e.update(NAN, -1.0f, 0.1f));
  TEST_ASSERT_EQUAL_FLOAT(s0, e.update(11.5f, NAN, 0.1f));
  TEST_ASSERT_EQUAL_FLOAT(s0, e.update(11.5f, -1.0f, 0.0f));
  TEST_ASSERT_EQUAL_FLOAT(s0, e.update(11.5f, -1.0f, 60.0f));
}

void test_charging_raises_soc_and_clamps() {
  SocEstimatorConfig c;
  SocEstimator e(c);
  e.update(4.15f * 3, 0.0f, 0.1f);
  float s = 0.0f;
  for (int i = 0; i < 1800; ++i) s = e.update(4.18f * 3, 2.0f, 1.0f);
  const float after_cc = s;
  // CV phase: current tapers off at 4.2 V per cell.
  for (int i = 0; i < 3600; ++i) s = e.update(4.20f * 3, 0.05f, 1.0f);
  TEST_ASSERT_TRUE(s > after_cc);
  TEST_ASSERT_TRUE(s <= 1.0f);
  TEST_ASSERT_FLOAT_WITHIN(0.02f, 1.0f, s);
}

void test_voltage_only_smooths_a_sag() {
  SocEstimatorConfig c;
  SocEstimator e(c);
  for (int i = 0; i < 600; ++i) e.updateVoltageOnly(3.84f * 3, 0.1f);
  // A one-second 0.5 V sag must not move the output by more than a few points.
  float s = 0.0f;
  for (int i = 0; i < 10; ++i) s = e.updateVoltageOnly(3.84f * 3 - 0.5f, 0.1f);
  TEST_ASSERT_FLOAT_WITHIN(0.03f, 0.50f, s);
}

void test_voltage_only_converges_and_ignores_bad_input() {
  SocEstimatorConfig c;
  SocEstimator e(c);
  e.updateVoltageOnly(3.60f * 3, 0.1f);
  float s = 0.0f;
  for (int i = 0; i < 1200; ++i) s = e.updateVoltageOnly(3.84f * 3, 1.0f);
  TEST_ASSERT_FLOAT_WITHIN(0.02f, 0.50f, s);
  TEST_ASSERT_EQUAL_FLOAT(s, e.updateVoltageOnly(NAN, 1.0f));
  TEST_ASSERT_EQUAL_FLOAT(s, e.updateVoltageOnly(11.5f, 0.0f));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_curve_endpoints_and_monotonic);
  RUN_TEST(test_first_sample_initialises_from_ocv);
  RUN_TEST(test_load_step_does_not_jump);
  RUN_TEST(test_discharge_integrates_current);
  RUN_TEST(test_rest_converges_to_ocv);
  RUN_TEST(test_bad_input_is_ignored);
  RUN_TEST(test_charging_raises_soc_and_clamps);
  RUN_TEST(test_voltage_only_smooths_a_sag);
  RUN_TEST(test_voltage_only_converges_and_ignores_bad_input);
  return UNITY_END();
}
