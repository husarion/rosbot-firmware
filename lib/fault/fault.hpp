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

#ifdef __cplusplus
extern "C" {
#endif

// Stops every timer (motor PWM), then resets the MCU, which leaves every pin
// Hi-Z. With a debugger attached it halts on a breakpoint before the reset.
// Safe from any context: task, ISR, fault handler.
void fatalError(void) __attribute__((noreturn));

#ifdef __cplusplus
}

// Routes HardFault/MemManage/BusFault/UsageFault to fatalError(). The
// handlers in STM32FreeRTOS are strong symbols that spin with PWM still
// running, so the vector table is copied to RAM instead. Call first in
// setup().
void installFaultHandlers();

// True once after a reset caused by fatalError().
bool consumeFaultResetFlag();
#endif
