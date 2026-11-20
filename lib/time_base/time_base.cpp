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

#include "time_base.hpp"

#include "clock.hpp"
#include "freertos.hpp"

namespace {
uint32_t s_last_us = 0;
uint64_t s_accum_us = 0;
}  // namespace

uint64_t time_boot_us() {
  // Read before the critical section so a preempting caller can be detected
  // below.
  const uint32_t now = platform::clock::counterUs();

  const UBaseType_t mask = taskENTER_CRITICAL_FROM_ISR();
  // Signed delta: a caller preempted between the read and here arrives with
  // an older `now`, which must not be counted as a ~71 min wrap.
  const int32_t delta = static_cast<int32_t>(now - s_last_us);
  if (delta > 0) {
    s_accum_us += static_cast<uint32_t>(delta);
    s_last_us = now;
  }
  const uint64_t result = s_accum_us;
  taskEXIT_CRITICAL_FROM_ISR(mask);
  return result;
}

// FreeRTOS run-time stats hooks (configGENERATE_RUN_TIME_STATS). They must
// not reprogram TIM5, which already runs as the 1 MHz clock: 16 us per
// count, wrapping after ~19 h.
void vConfigureTimerForRunTimeStats(void) {}

uint32_t vGetTimerValueForRunTimeStats(void) {
  return static_cast<uint32_t>(time_boot_us() >> 4);
}
