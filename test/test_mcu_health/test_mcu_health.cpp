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

#include "mcu_health_math.hpp"

using diagnostics::computeTaskLoad;
using diagnostics::decodeResetCause;
using diagnostics::TaskSample;

namespace {
int a, b, c;  // task identities
}  // namespace

void setUp() {}
void tearDown() {}

void test_idle_share_comes_from_counter_deltas() {
  const TaskSample prev[] = {{&a, "IDLE", 1000, 400}, {&b, "Imu", 500, 300}};
  const TaskSample cur[] = {{&a, "IDLE", 1900, 400}, {&b, "Imu", 600, 300}};
  const auto load = computeTaskLoad(prev, 2, cur, 2);
  TEST_ASSERT_EQUAL_UINT16(900, load.idle_permille);  // 900 of 1000
}

void test_counter_wrap_still_gives_the_delta() {
  const TaskSample prev[] = {{&a, "IDLE", 0xFFFFFF00u, 100}, {&b, "X", 0, 100}};
  const TaskSample cur[] = {{&a, "IDLE", 0x00000100u, 100},
                            {&b, "X", 512, 100}};
  const auto load = computeTaskLoad(prev, 2, cur, 2);
  TEST_ASSERT_EQUAL_UINT16(500, load.idle_permille);  // 512 idle of 1024
}

void test_tasks_are_matched_by_identity_not_position() {
  const TaskSample prev[] = {{&b, "Imu", 100, 300}, {&a, "IDLE", 100, 400}};
  const TaskSample cur[] = {{&a, "IDLE", 400, 400}, {&b, "Imu", 200, 300}};
  const auto load = computeTaskLoad(prev, 2, cur, 2);
  TEST_ASSERT_EQUAL_UINT16(750, load.idle_permille);
}

void test_smallest_stack_and_its_task() {
  const TaskSample cur[] = {
      {&a, "IDLE", 0, 400}, {&b, "Range", 0, 96}, {&c, "Link", 0, 2000}};
  const auto load = computeTaskLoad(nullptr, 0, cur, 3);
  TEST_ASSERT_EQUAL_UINT32(96, load.stack_min_free);
  TEST_ASSERT_EQUAL_STRING("Range", load.stack_min_task);
}

void test_no_activity_reads_as_fully_idle() {
  const TaskSample s[] = {{&a, "IDLE", 10, 400}};
  TEST_ASSERT_EQUAL_UINT16(1000, computeTaskLoad(s, 1, s, 1).idle_permille);
}

void test_reset_causes() {
  using namespace diagnostics;
  // Power-on sets POR, BOR and PIN together.
  TEST_ASSERT_EQUAL_HEX16(
      kResetPowerOn,
      decodeResetCause(kCsrPorRst | kCsrBorRst | kCsrPinRst, false));
  TEST_ASSERT_EQUAL_HEX16(kResetPin, decodeResetCause(kCsrPinRst, false));
  // NVIC_SystemReset drives NRST too: software wins over the pin flag.
  TEST_ASSERT_EQUAL_HEX16(kResetSoftware,
                          decodeResetCause(kCsrSftRst | kCsrPinRst, false));
  TEST_ASSERT_EQUAL_HEX16(kResetSoftware | kResetFirmwareFault,
                          decodeResetCause(kCsrSftRst | kCsrPinRst, true));
  TEST_ASSERT_EQUAL_HEX16(kResetIwdg,
                          decodeResetCause(kCsrIwdgRst | kCsrPinRst, false));
  TEST_ASSERT_EQUAL_HEX16(kResetLowPower | kResetWwdg,
                          decodeResetCause(kCsrLpwrRst | kCsrWwdgRst, false));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_idle_share_comes_from_counter_deltas);
  RUN_TEST(test_counter_wrap_still_gives_the_delta);
  RUN_TEST(test_tasks_are_matched_by_identity_not_position);
  RUN_TEST(test_smallest_stack_and_its_task);
  RUN_TEST(test_no_activity_reads_as_fully_idle);
  RUN_TEST(test_reset_causes);
  return UNITY_END();
}
