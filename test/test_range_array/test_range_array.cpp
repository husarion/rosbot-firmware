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

#include "range_array.hpp"

void setUp() {}
void tearDown() {}

namespace {

struct FakeRange : RangeInterface {
  bool is_lost = false;
  bool recovers = true;
  int inits = 0;
  int power_offs = 0;

  void init() override {
    ++inits;
    if (recovers) is_lost = false;
  }
  void update() override { data_.range = is_lost ? NAN : 1.0f; }
  void powerOff() override { ++power_offs; }
  void powerOn() override {}
  const char* name() const override { return "fake"; }
  bool lost() const override { return is_lost; }
};

}  // namespace

void test_healthy_sensors_are_never_reinitialised() {
  FakeRange a, b;
  RangeInterface* s[] = {&a, &b};
  RangeArray arr(s);
  for (uint32_t t = 0; t < 60000; t += 100) arr.update(t);
  TEST_ASSERT_EQUAL(0, a.inits);
  TEST_ASSERT_EQUAL(0, b.inits);
}

void test_a_lost_sensor_reinitialises_the_whole_array() {
  FakeRange a, b;
  RangeInterface* s[] = {&a, &b};
  RangeArray arr(s);
  arr.update(0);
  b.is_lost = true;
  arr.update(500);  // within the first backoff
  TEST_ASSERT_EQUAL(0, b.inits);
  arr.update(1000);
  TEST_ASSERT_EQUAL(1, a.inits);
  TEST_ASSERT_EQUAL(1, b.inits);
  TEST_ASSERT_EQUAL(1, a.power_offs);  // every XSHUT low before re-addressing
  TEST_ASSERT_FALSE(b.lost());
}

void test_retries_back_off_up_to_the_cap() {
  FakeRange a;
  a.recovers = false;
  RangeInterface* s[] = {&a};
  RangeArray arr(s);
  arr.update(0);
  a.is_lost = true;
  uint32_t last = 0;
  uint32_t gaps[8] = {};
  int n = 0;
  for (uint32_t t = 0; t <= 100000 && n < 8; t += 100) {
    const int before = a.inits;
    arr.update(t);
    if (a.inits != before) {
      gaps[n++] = t - last;
      last = t;
    }
  }
  TEST_ASSERT_EQUAL_UINT32(1000, gaps[0]);
  TEST_ASSERT_EQUAL_UINT32(2000, gaps[1]);
  TEST_ASSERT_EQUAL_UINT32(4000, gaps[2]);
  TEST_ASSERT_EQUAL_UINT32(8000, gaps[3]);
  TEST_ASSERT_EQUAL_UINT32(RangeArray::kReinitMaxMs, gaps[4]);
  TEST_ASSERT_EQUAL_UINT32(RangeArray::kReinitMaxMs, gaps[5]);
}

void test_backoff_restarts_after_recovery() {
  FakeRange a;
  RangeInterface* s[] = {&a};
  RangeArray arr(s);
  arr.update(0);
  a.is_lost = true;
  arr.update(1000);  // recovers
  arr.update(1100);
  a.is_lost = true;
  arr.update(1500);
  TEST_ASSERT_EQUAL(1, a.inits);
  arr.update(2100);  // a fresh 1 s backoff from the last healthy update
  TEST_ASSERT_EQUAL(2, a.inits);
}

void test_lost_sensor_reads_nan() {
  FakeRange a;
  RangeInterface* s[] = {&a};
  RangeArray arr(s);
  a.is_lost = true;
  a.recovers = false;
  arr.update(0);
  TEST_ASSERT_TRUE(std::isnan(arr.getData().range[0]));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_healthy_sensors_are_never_reinitialised);
  RUN_TEST(test_a_lost_sensor_reinitialises_the_whole_array);
  RUN_TEST(test_retries_back_off_up_to_the_cap);
  RUN_TEST(test_backoff_restarts_after_recovery);
  RUN_TEST(test_lost_sensor_reads_nan);
  return UNITY_END();
}
