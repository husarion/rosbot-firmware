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

#include "pid.hpp"

void setUp() {}
void tearDown() {}

namespace {

PIDConfig pOnly() {
  PIDConfig cfg{};
  cfg.kp = 1.0f;
  return cfg;
}

}  // namespace

void test_non_finite_input_resets_and_returns_zero() {
  PIDConfig cfg = pOnly();
  cfg.ki = 1.0f;
  PIDController pid(cfg);
  for (int i = 0; i < 5; ++i) pid.compute(0.5f, 0.0f, 0.1f);

  TEST_ASSERT_EQUAL_FLOAT(0.0f, pid.compute(NAN, 0.0f, 0.1f));
  TEST_ASSERT_EQUAL_FLOAT(0.0f, pid.compute(0.5f, INFINITY, 0.1f));
  TEST_ASSERT_EQUAL_FLOAT(0.0f, pid.compute(0.5f, 0.0f, NAN));

  // The integral built up above must be gone.
  PIDController fresh(cfg);
  TEST_ASSERT_EQUAL_FLOAT(fresh.compute(0.5f, 0.0f, 0.1f),
                          pid.compute(0.5f, 0.0f, 0.1f));
}

void test_non_positive_dt_returns_zero() {
  PIDController pid(pOnly());
  TEST_ASSERT_EQUAL_FLOAT(0.0f, pid.compute(1.0f, 0.0f, 0.0f));
  TEST_ASSERT_EQUAL_FLOAT(0.0f, pid.compute(1.0f, 0.0f, -0.1f));
}

void test_output_is_clamped() {
  PIDConfig cfg = pOnly();
  cfg.kp = 10.0f;
  PIDController pid(cfg);
  TEST_ASSERT_EQUAL_FLOAT(1.0f, pid.compute(1.0f, 0.0f, 0.1f));
  TEST_ASSERT_EQUAL_FLOAT(-1.0f, pid.compute(-1.0f, 0.0f, 0.1f));
}

void test_integral_is_bounded() {
  PIDConfig cfg{};
  cfg.ki = 1.0f;
  cfg.max_integral = 0.5f;
  PIDController pid(cfg);
  float out = 0.0f;
  for (int i = 0; i < 10; ++i) out = pid.compute(1.0f, 0.0f, 1.0f);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.5f, out);
}

void test_acceleration_limit_ramps_the_setpoint() {
  PIDConfig cfg = pOnly();
  cfg.max_accel = 1.0f;
  PIDController pid(cfg);
  // 1 rad/s^2 over 0.1 s: the target only moves 0.1 towards 10.
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.1f, pid.compute(10.0f, 0.0f, 0.1f));
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.2f, pid.compute(10.0f, 0.0f, 0.1f));
}

void test_braking_against_motion_is_limited() {
  PIDConfig cfg = pOnly();
  cfg.max_brake_output = 0.3f;
  PIDController pid(cfg);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, -0.3f, pid.compute(0.0f, 1.0f, 0.1f));
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.3f, pid.compute(0.0f, -1.0f, 0.1f));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_non_finite_input_resets_and_returns_zero);
  RUN_TEST(test_non_positive_dt_returns_zero);
  RUN_TEST(test_output_is_clamped);
  RUN_TEST(test_integral_is_bounded);
  RUN_TEST(test_acceleration_limit_ramps_the_setpoint);
  RUN_TEST(test_braking_against_motion_is_limited);
  return UNITY_END();
}
