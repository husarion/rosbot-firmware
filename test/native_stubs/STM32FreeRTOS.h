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

// Host-test stand-in for STM32FreeRTOS: the critical-section hooks call into
// the test, which can simulate preemption there.

#pragma once

#include <cstdint>

typedef uint32_t UBaseType_t;

// Defined by the test that needs them.
UBaseType_t testEnterCritical();
void testExitCritical(UBaseType_t mask);

#define taskENTER_CRITICAL_FROM_ISR() testEnterCritical()
#define taskEXIT_CRITICAL_FROM_ISR(mask) testExitCritical(mask)
