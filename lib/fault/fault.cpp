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

#include <Arduino.h>
#include <STM32FreeRTOS.h>

#include <cstring>

namespace {

constexpr size_t kVectorCount = 16 + FPU_IRQn + 1;
constexpr size_t kHardFault = 3;
constexpr size_t kUsageFault = 6;

// VTOR needs the table aligned to its size rounded up to a power of two.
alignas(512) uint32_t s_vectors[kVectorCount];
static_assert(sizeof(s_vectors) <= 512, "VTOR alignment too small");

// The linker script places nothing in CCM RAM and the startup code never
// clears it, so this survives a system reset. After power-on it holds
// garbage, hence the magic and its complement.
struct ResetMarker {
  uint32_t magic;
  uint32_t magic_inv;
};
ResetMarker* const s_marker = reinterpret_cast<ResetMarker*>(0x1000FFF8);
constexpr uint32_t kMagic = 0xFA17C0DE;

void faultHandler() { fatalError(); }

}  // namespace

extern "C" void fatalError(void) {
  __disable_irq();
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

void installFaultHandlers() {
  std::memcpy(s_vectors, reinterpret_cast<const void*>(SCB->VTOR),
              sizeof(s_vectors));
  for (size_t i = kHardFault; i <= kUsageFault; ++i) {
    s_vectors[i] = reinterpret_cast<uint32_t>(&faultHandler);
  }
  const uint32_t primask = __get_PRIMASK();
  __disable_irq();
  SCB->VTOR = reinterpret_cast<uint32_t>(s_vectors);
  __DSB();
  __set_PRIMASK(primask);
}

bool consumeFaultResetFlag() {
  const bool set = s_marker->magic == kMagic && s_marker->magic_inv == ~kMagic;
  s_marker->magic = 0;
  s_marker->magic_inv = 0;
  return set;
}
