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

#include <cstdint>

#include "mcu_health_math.hpp"

namespace diagnostics {

struct McuHealth {
  uint32_t uptime_ms;
  uint16_t reset_cause;        // ResetCause bits
  uint16_t cpu_idle_permille;  // since the previous sample, or kIdleUnknown
  uint32_t heap_free;
  uint32_t heap_min_free;   // never handed out since boot
  uint32_t stack_min_free;  // bytes, smallest of any task
  char stack_min_task[16];
  uint8_t fault_code;    // FaultCode of the last fatalError(), 0 if none
  uint32_t fault_pc;     // faulting PC for exception codes
  char fault_task[16];   // task for a stack overflow
  uint8_t clock_source;  // ClockSource
};

// Reads and clears the RCC reset flags. Call once at boot with what
// consumeFaultInfo() returned.
void captureResetCause(bool firmware_fault, uint8_t fault_code,
                       uint32_t fault_pc, const char* fault_task);

// CPU idle is measured since the previous call, so call it from one task
// at a steady rate (the diagnostics publisher, 1 Hz).
McuHealth sampleMcuHealth();

}  // namespace diagnostics
