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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Why fatalError() ran; kept across the reset and reported in diagnostics.
enum FaultCode {
  kFaultUnknown = 0,
  kFaultAssert = 1,         // configASSERT
  kFaultStackOverflow = 2,  // detail: task name
  kFaultMallocFailed = 3,
  kFaultHardFault = 4,  // detail: faulting PC
  kFaultMemManage = 5,
  kFaultBusFault = 6,
  kFaultUsageFault = 7,
  kFaultNmi = 8,
  kFaultClock = 9,  // clock tree did not start, or the crystal stopped
  kFaultExit = 10,  // libc _exit()
};

// Stops every timer (motor PWM), then resets the MCU, which leaves every pin
// Hi-Z. With a debugger attached it halts on a breakpoint before the reset.
// Safe from any context: task, ISR, fault handler.
void fatalErrorWithCode(uint32_t code, uint32_t pc, const char* task)
    __attribute__((noreturn));
static inline __attribute__((noreturn)) void fatalError(void) {
  fatalErrorWithCode(kFaultUnknown, 0, 0);
}

// Exception entry for the fault vectors: frame is the stacked exception
// frame (MSP or PSP), code a FaultCode.
void faultFromException(const uint32_t* frame, uint32_t code)
    __attribute__((noreturn));

#ifdef __cplusplus
}

struct FaultInfo {
  bool fault_reset;  // the last reset came from fatalError()
  uint8_t code;      // FaultCode
  uint32_t pc;       // faulting PC for the exception codes, else 0
  char task[16];     // task name for kFaultStackOverflow
};

// What the previous run left behind; call once at boot (clears it).
FaultInfo consumeFaultInfo();
#endif
