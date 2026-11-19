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
  if (HAL_RCC_OscConfig(&osc) != HAL_OK) fatalError();

  clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
  clk.APB1CLKDivider = RCC_HCLK_DIV4;
  clk.APB2CLKDivider = RCC_HCLK_DIV2;
  if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_5) != HAL_OK) fatalError();
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

extern "C" void NMI_Handler(void) { fatalError(); }
extern "C" void HardFault_Handler(void) { fatalError(); }
extern "C" void MemManage_Handler(void) { fatalError(); }
extern "C" void BusFault_Handler(void) { fatalError(); }
extern "C" void UsageFault_Handler(void) { fatalError(); }

int main() {
  setup();
  // setup() ends in vTaskStartScheduler(), which only returns when the
  // kernel cannot allocate the idle task.
  fatalError();
}
