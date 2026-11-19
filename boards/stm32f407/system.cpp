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

// Internal 16 MHz RC -> PLL -> 168 MHz, APB1 42 MHz, APB2 84 MHz: the clock
// tree of the stm32duino generic F407ZG variant this firmware ran on before.
// The RC is +-1 % over temperature; see the plan's finding 14 (crystal).
void systemClockConfig() {
  RCC_OscInitTypeDef osc = {};
  RCC_ClkInitTypeDef clk = {};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  osc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  osc.HSIState = RCC_HSI_ON;
  osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  osc.PLL.PLLState = RCC_PLL_ON;
  osc.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  osc.PLL.PLLM = 8;
  osc.PLL.PLLN = 168;
  osc.PLL.PLLP = RCC_PLLP_DIV2;
  osc.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&osc) != HAL_OK)
    fatalErrorWithCode(kFaultClock, 0, nullptr);

  clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
  clk.APB1CLKDivider = RCC_HCLK_DIV4;
  clk.APB2CLKDivider = RCC_HCLK_DIV2;
  if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_5) != HAL_OK) {
    fatalErrorWithCode(kFaultClock, 0, nullptr);
  }
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

extern "C" void NMI_Handler(void) { fatalErrorWithCode(kFaultNmi, 0, nullptr); }

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
