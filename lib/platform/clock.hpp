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

// Microsecond hardware clock on TIM5 (32-bit, 1 MHz, free-running). Neither
// board uses TIM5 for anything else.
namespace platform::clock {

// Starts TIM5. Call first thing in setup(); counterUs() reads 0 until then.
void init();

// Raw counter, wraps every ~71.6 min. Fine for short deltas with unsigned
// subtraction; use time_boot_us() for timestamps. Safe from any context.
uint32_t counterUs();

// Busy-waits on the counter.
void delayUs(uint32_t us);

// Blocks the calling task once the scheduler runs (never shorter than
// requested); busy-waits before that.
void delayMs(uint32_t ms);

}  // namespace platform::clock
