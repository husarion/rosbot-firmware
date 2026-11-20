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

#include "FreeRTOS.h"
#include "fault.hpp"
#include "stm32f4xx.h"
#include "task.h"

extern "C" void xPortSysTickHandler(void);
extern "C" void setup(void);

namespace {

// 2 MHz into the VCO (RM0090 recommends it, less PLL jitter), VCO 336 MHz:
// SYSCLK 168 MHz, 48 MHz on PLLQ; APB1 42 MHz, APB2 84 MHz, whichever source.
constexpr uint32_t kPllInputHz = 2000000;
static_assert(HSE_VALUE % kPllInputHz == 0, "crystal must divide to 2 MHz");

bool startPll(uint32_t source) {
  RCC_OscInitTypeDef osc = {};
  if (source == RCC_PLLSOURCE_HSE) {
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.PLL.PLLM = HSE_VALUE / kPllInputHz;
  } else {
    // A crystal that never started is switched off again.
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_HSE;
    osc.HSIState = RCC_HSI_ON;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.HSEState = RCC_HSE_OFF;
    osc.PLL.PLLM = HSI_VALUE / kPllInputHz;
  }
  osc.PLL.PLLState = RCC_PLL_ON;
  osc.PLL.PLLSource = source;
  osc.PLL.PLLN = 168;
  osc.PLL.PLLP = RCC_PLLP_DIV2;
  osc.PLL.PLLQ = 7;
  return HAL_RCC_OscConfig(&osc) == HAL_OK;
}

// The crystal (CORE2 8 MHz, Digital Board 24 MHz) keeps MCU time within
// tens of ppm; the internal RC drifted ~2000 ppm on HW and made the bridge
// re-seed TIMESYNC every 30-40 s. The RC stays as the fallback when the
// crystal does not start within HSE_STARTUP_TIMEOUT.
void systemClockConfig() {
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  const bool crystal = startPll(RCC_PLLSOURCE_HSE);
  if (!crystal && !startPll(RCC_PLLSOURCE_HSI)) {
    fatalErrorWithCode(kFaultClock, 0, nullptr);
  }

  RCC_ClkInitTypeDef clk = {};
  clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
  clk.APB1CLKDivider = RCC_HCLK_DIV4;
  clk.APB2CLKDivider = RCC_HCLK_DIV2;
  if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_5) != HAL_OK) {
    fatalErrorWithCode(kFaultClock, 0, nullptr);
  }
  // A crystal that stops later raises an NMI (CSS); see NMI_Handler.
  if (crystal) HAL_RCC_EnableCSS();
}

}  // namespace

// Runs before every other static constructor, so constructors already see
// HAL and the final clock (the order the Arduino core used).
__attribute__((constructor(101))) static void boardInit() {
  // All bits preemption: FreeRTOS requirement on Cortex-M.
  HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  HAL_Init();
  systemClockConfig();
}

extern "C" void SysTick_Handler(void) {
  HAL_IncTick();
  if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
    xPortSysTickHandler();
  }
}

extern "C" void NMI_Handler(void) {
  // CSS already moved SYSCLK to the RC; the reset retries the crystal.
  if (RCC->CIR & RCC_CIR_CSSF) fatalErrorWithCode(kFaultClock, 0, nullptr);
  fatalErrorWithCode(kFaultNmi, 0, nullptr);
}

// Pass the stacked exception frame (MSP or PSP, by EXC_RETURN bit 2) so the
// faulting PC survives the reset.
#define FAULT_VECTOR(name, code)                      \
  extern "C" __attribute__((naked)) void name(void) { \
    __asm volatile(                                   \
        "tst lr, #4\n"                                \
        "ite eq\n"                                    \
        "mrseq r0, msp\n"                             \
        "mrsne r0, psp\n"                             \
        "mov r1, %0\n"                                \
        "b faultFromException\n" ::"i"(code));        \
  }
FAULT_VECTOR(HardFault_Handler, kFaultHardFault)
FAULT_VECTOR(MemManage_Handler, kFaultMemManage)
FAULT_VECTOR(BusFault_Handler, kFaultBusFault)
FAULT_VECTOR(UsageFault_Handler, kFaultUsageFault)
#undef FAULT_VECTOR

int main() {
  setup();
  // setup() ends in vTaskStartScheduler(), which only returns when the
  // kernel cannot allocate the idle task.
  fatalError();
}
