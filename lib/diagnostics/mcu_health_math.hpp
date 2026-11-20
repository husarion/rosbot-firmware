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

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

// Pure parts of the MCU health report, kept free of FreeRTOS and CMSIS so
// the host tests can drive them.
namespace diagnostics {

// One task as FreeRTOS reports it (uxTaskGetSystemState).
struct TaskSample {
  const void* id;  // task handle; identifies a task across samples
  const char* name;
  uint32_t run_time;    // run-time stats counter
  uint32_t stack_free;  // high-water mark, bytes
};

inline constexpr uint16_t kIdleUnknown = UINT16_MAX;

struct TaskLoad {
  uint16_t idle_permille = 0;  // 1000 when nothing ran
  uint32_t stack_min_free = UINT32_MAX;
  const char* stack_min_task = "";
};

// CPU use between two samples (per-task counter deltas, matched by id) and
// the smallest stack headroom in the newer one. A task missing from prev
// counts from zero, so a task created in between is charged its whole
// counter: harmless for a 1 Hz report.
inline TaskLoad computeTaskLoad(const TaskSample* prev, size_t n_prev,
                                const TaskSample* cur, size_t n_cur,
                                const char* idle_name = "IDLE") {
  TaskLoad out;
  uint64_t total = 0;
  uint64_t idle = 0;
  for (size_t i = 0; i < n_cur; ++i) {
    uint32_t before = 0;
    for (size_t j = 0; j < n_prev; ++j) {
      if (prev[j].id == cur[i].id) before = prev[j].run_time;
    }
    const uint32_t delta = cur[i].run_time - before;  // wraps correctly
    total += delta;
    if (std::strcmp(cur[i].name, idle_name) == 0) idle += delta;
    if (cur[i].stack_free < out.stack_min_free) {
      out.stack_min_free = cur[i].stack_free;
      out.stack_min_task = cur[i].name;
    }
  }
  out.idle_permille =
      total == 0 ? 1000 : static_cast<uint16_t>(idle * 1000U / total);
  return out;
}

// Reset cause bits as ROSBOT_DIAGNOSTICS.reset_cause defines them.
enum ResetCause : uint16_t {
  kResetPin = 1,
  kResetPowerOn = 2,
  kResetSoftware = 4,
  kResetIwdg = 8,
  kResetWwdg = 16,
  kResetLowPower = 32,
  kResetFirmwareFault = 64,
};

// RCC_CSR reset flags (RM0090 7.3.21), bit positions.
inline constexpr uint32_t kCsrBorRst = 1UL << 25;
inline constexpr uint32_t kCsrPinRst = 1UL << 26;
inline constexpr uint32_t kCsrPorRst = 1UL << 27;
inline constexpr uint32_t kCsrSftRst = 1UL << 28;
inline constexpr uint32_t kCsrIwdgRst = 1UL << 29;
inline constexpr uint32_t kCsrWwdgRst = 1UL << 30;
inline constexpr uint32_t kCsrLpwrRst = 1UL << 31;

// Every internal reset also drives NRST and sets the pin flag, so "pin"
// means the reset button only when no other flag explains the reset.
// fatalError() is a software reset; the CCM marker tells it from others.
inline uint16_t decodeResetCause(uint32_t csr, bool firmware_fault) {
  uint16_t out = 0;
  if (csr & (kCsrPorRst | kCsrBorRst)) out |= kResetPowerOn;
  if (csr & kCsrSftRst) out |= kResetSoftware;
  if (csr & kCsrIwdgRst) out |= kResetIwdg;
  if (csr & kCsrWwdgRst) out |= kResetWwdg;
  if (csr & kCsrLpwrRst) out |= kResetLowPower;
  if (out == 0 && (csr & kCsrPinRst)) out |= kResetPin;
  if (firmware_fault) out |= kResetFirmwareFault;
  return out;
}

// ROSBOT_DIAGNOSTICS.clock_source.
enum ClockSource : uint8_t {
  kClockUnknown = 0,
  kClockCrystal = 1,
  kClockInternal = 2,
};

// RCC_PLLCFGR PLLSRC (RM0090 7.3.2): set when the PLL runs from HSE.
inline constexpr uint32_t kPllCfgrPllSrcHse = 1UL << 22;

inline ClockSource decodeClockSource(uint32_t pllcfgr) {
  return (pllcfgr & kPllCfgrPllSrcHse) ? kClockCrystal : kClockInternal;
}

}  // namespace diagnostics
