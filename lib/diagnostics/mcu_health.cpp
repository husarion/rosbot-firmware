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

#include "mcu_health.hpp"

#include <cstring>

#include "device.hpp"
#include "freertos.hpp"
#include "time_base.hpp"

namespace diagnostics {
namespace {

constexpr size_t kMaxTasks = 16;

uint16_t s_reset_cause = 0;
uint8_t s_fault_code = 0;
uint32_t s_fault_pc = 0;
char s_fault_task[16] = {};
TaskSample s_prev[kMaxTasks];
size_t s_prev_n = 0;

}  // namespace

void captureResetCause(bool firmware_fault, uint8_t fault_code,
                       uint32_t fault_pc, const char* fault_task) {
  s_reset_cause = decodeResetCause(RCC->CSR, firmware_fault);
  RCC->CSR |= RCC_CSR_RMVF;
  if (firmware_fault) {
    s_fault_code = fault_code;
    s_fault_pc = fault_pc;
    std::strncpy(s_fault_task, fault_task, sizeof(s_fault_task) - 1);
  }
}

McuHealth sampleMcuHealth() {
  TaskStatus_t status[kMaxTasks];
  uint32_t total_run_time = 0;
  const UBaseType_t n =
      uxTaskGetSystemState(status, kMaxTasks, &total_run_time);

  TaskSample cur[kMaxTasks];
  for (UBaseType_t i = 0; i < n; ++i) {
    cur[i] = {status[i].xHandle, status[i].pcTaskName,
              status[i].ulRunTimeCounter,
              static_cast<uint32_t>(status[i].usStackHighWaterMark *
                                    sizeof(StackType_t))};
  }
  const TaskLoad load = computeTaskLoad(s_prev, s_prev_n, cur, n);

  McuHealth out{};
  out.uptime_ms = time_boot_ms();
  out.reset_cause = s_reset_cause;
  // The first call has no previous sample, and since boot the first task
  // is charged the whole of setup(): report unknown instead.
  out.cpu_idle_permille = s_prev_n == 0 ? kIdleUnknown : load.idle_permille;
  out.heap_free = static_cast<uint32_t>(xPortGetFreeHeapSize());
  out.heap_min_free = static_cast<uint32_t>(xPortGetMinimumEverFreeHeapSize());
  out.stack_min_free = load.stack_min_free;
  std::strncpy(out.stack_min_task, load.stack_min_task,
               sizeof(out.stack_min_task) - 1);
  out.fault_code = s_fault_code;
  out.fault_pc = s_fault_pc;
  std::memcpy(out.fault_task, s_fault_task, sizeof(out.fault_task));
  out.clock_source = decodeClockSource(RCC->PLLCFGR);

  // Task names live in the TCBs, which never go away: no task is deleted
  // after boot.
  std::memcpy(s_prev, cur, sizeof(TaskSample) * n);
  s_prev_n = n;
  return out;
}

}  // namespace diagnostics
