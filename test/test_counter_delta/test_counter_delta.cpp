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

#include "counter_delta.hpp"

namespace {
constexpr uint32_t kMax16 = 0xFFFF;
}  // namespace

void setUp() {}
void tearDown() {}

void test_plain_forward_and_backward() {
  TEST_ASSERT_EQUAL_INT32(25, counterDelta(125, 100, kMax16));
  TEST_ASSERT_EQUAL_INT32(-25, counterDelta(100, 125, kMax16));
  TEST_ASSERT_EQUAL_INT32(0, counterDelta(7, 7, kMax16));
}

void test_wrap_forward() {
  TEST_ASSERT_EQUAL_INT32(21, counterDelta(5, 0xFFF0, kMax16));
}

void test_wrap_backward() {
  TEST_ASSERT_EQUAL_INT32(-21, counterDelta(0xFFF0, 5, kMax16));
}

void test_half_range_boundary() {
  TEST_ASSERT_EQUAL_INT32(0x8000, counterDelta(0x8000, 0, kMax16));
  TEST_ASSERT_EQUAL_INT32(-0x7FFF, counterDelta(0x8001, 0, kMax16));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_plain_forward_and_backward);
  RUN_TEST(test_wrap_forward);
  RUN_TEST(test_wrap_backward);
  RUN_TEST(test_half_range_boundary);
  return UNITY_END();
}
