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

#include "clock.hpp"

#include "freertos.hpp"
#include "timer.hpp"

namespace platform::clock {

void init() {
  enableTimerClock(TIM5);
  TIM5->CR1 = 0;
  TIM5->PSC = timerClockHz(TIM5) / 1000000U - 1U;
  TIM5->ARR = 0xFFFFFFFFU;
  TIM5->CNT = 0;
  TIM5->EGR = TIM_EGR_UG;  // load PSC now, not at the first overflow
  TIM5->SR = 0;
  // Stop with the core on a debugger halt, so timeouts do not all fire on
  // resume.
  DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_TIM5_STOP;
  TIM5->CR1 = TIM_CR1_CEN;
}

uint32_t counterUs() { return TIM5->CNT; }

void delayUs(uint32_t us) {
  const uint32_t start = counterUs();
  while (counterUs() - start < us) {
  }
}

void delayMs(uint32_t ms) {
  if (ms == 0) return;
  if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
    // +1: the first tick may be only a fraction of a period away.
    vTaskDelay(pdMS_TO_TICKS(ms) + 1);
    return;
  }
  for (uint32_t i = 0; i < ms; ++i) delayUs(1000);
}

}  // namespace platform::clock
