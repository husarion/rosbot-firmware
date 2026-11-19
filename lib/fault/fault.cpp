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

#include "fault.hpp"

#include <cstddef>

#include "device.hpp"
#include "freertos.hpp"

namespace {

// The linker script places nothing in CCM RAM and the startup code never
// clears it, so this survives a system reset. After power-on it holds
// garbage, hence the magic and its complement.
struct ResetMarker {
  uint32_t magic;
  uint32_t magic_inv;
  uint32_t code;
  uint32_t pc;
  char task[16];
};
static_assert(sizeof(ResetMarker) == 32, "marker layout");
ResetMarker* const s_marker = reinterpret_cast<ResetMarker*>(0x1000FFE0);
constexpr uint32_t kMagic = 0xFA17C0DE;

// Holding every timer in reset stops all PWM (motors, fan) before a possible
// debugger halt; on the reset path it is redundant but harmless.
void stopAllTimers() {
  RCC->APB1RSTR |=
      RCC_APB1RSTR_TIM2RST | RCC_APB1RSTR_TIM3RST | RCC_APB1RSTR_TIM4RST |
      RCC_APB1RSTR_TIM5RST | RCC_APB1RSTR_TIM6RST | RCC_APB1RSTR_TIM7RST |
      RCC_APB1RSTR_TIM12RST | RCC_APB1RSTR_TIM13RST | RCC_APB1RSTR_TIM14RST;
  RCC->APB2RSTR |= RCC_APB2RSTR_TIM1RST | RCC_APB2RSTR_TIM8RST |
                   RCC_APB2RSTR_TIM9RST | RCC_APB2RSTR_TIM10RST |
                   RCC_APB2RSTR_TIM11RST;
}

}  // namespace

extern "C" void fatalErrorWithCode(uint32_t code, uint32_t pc,
                                   const char* task) {
  __disable_irq();
  stopAllTimers();
  // C_DEBUGEN survives a system reset (only power-on clears it), so after an
  // ST-Link session this can halt with no debugger attached — hence the
  // timers are stopped first.
  if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) __BKPT(0);
  s_marker->code = code;
  s_marker->pc = pc;
  size_t i = 0;
  for (; task != nullptr && i + 1 < sizeof(s_marker->task) && task[i]; ++i) {
    s_marker->task[i] = task[i];
  }
  for (; i < sizeof(s_marker->task); ++i) s_marker->task[i] = '\0';
  s_marker->magic = kMagic;
  s_marker->magic_inv = ~kMagic;
  NVIC_SystemReset();
}

// Exception frame: r0 r1 r2 r3 r12 lr pc xpsr.
extern "C" void faultFromException(const uint32_t* frame, uint32_t code) {
  fatalErrorWithCode(code, frame[6], nullptr);
}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t /*task*/,
                                              char* name) {
  fatalErrorWithCode(kFaultStackOverflow, 0, name);
}

extern "C" void vApplicationMallocFailedHook(void) {
  fatalErrorWithCode(kFaultMallocFailed, 0, nullptr);
}

FaultInfo consumeFaultInfo() {
  FaultInfo out{};
  out.fault_reset = s_marker->magic == kMagic && s_marker->magic_inv == ~kMagic;
  if (out.fault_reset) {
    out.code = static_cast<uint8_t>(s_marker->code);
    out.pc = s_marker->pc;
    for (size_t i = 0; i + 1 < sizeof(out.task); ++i)
      out.task[i] = s_marker->task[i];
  }
  s_marker->magic = 0;
  s_marker->magic_inv = 0;
  return out;
}
