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

#include "device.hpp"
#include "freertos.hpp"

namespace {

// The linker script places nothing in CCM RAM and the startup code never
// clears it, so this survives a system reset. After power-on it holds
// garbage, hence the magic and its complement.
struct ResetMarker {
  uint32_t magic;
  uint32_t magic_inv;
};
ResetMarker* const s_marker = reinterpret_cast<ResetMarker*>(0x1000FFF8);
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

extern "C" void fatalError(void) {
  __disable_irq();
  stopAllTimers();
  // C_DEBUGEN survives a system reset (only power-on clears it), so after an
  // ST-Link session this can halt with no debugger attached — hence the
  // timers are stopped first.
  if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) __BKPT(0);
  s_marker->magic = kMagic;
  s_marker->magic_inv = ~kMagic;
  NVIC_SystemReset();
}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t /*task*/,
                                              char* /*name*/) {
  fatalError();
}

extern "C" void vApplicationMallocFailedHook(void) { fatalError(); }

bool consumeFaultResetFlag() {
  const bool set = s_marker->magic == kMagic && s_marker->magic_inv == ~kMagic;
  s_marker->magic = 0;
  s_marker->magic_inv = 0;
  return set;
}
