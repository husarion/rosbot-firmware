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

#include <Arduino.h>
#include <STM32FreeRTOS.h>

namespace {
uint32_t s_last_us = 0;
uint64_t s_accum_us = 0;
}  // namespace

uint64_t time_boot_us() {
  // micros() must be read outside the critical section: with SysTick masked
  // it misses the pending tick and steps back ~1 ms.
  const uint32_t now = micros();

  const UBaseType_t mask = taskENTER_CRITICAL_FROM_ISR();
  // Signed delta: a caller preempted between micros() and here arrives with
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
