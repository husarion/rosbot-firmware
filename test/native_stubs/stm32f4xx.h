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

// Host-test stand-in for the CMSIS device header (plus the few HAL RCC
// helpers platform/ uses): GPIO ports, timers and RCC live in plain memory
// the test can inspect; GPIO ports are laid out at the F4 stride.

#pragma once

#include <cstdint>

#define __IO volatile

typedef struct {
  __IO uint32_t MODER;
  __IO uint32_t OTYPER;
  __IO uint32_t OSPEEDR;
  __IO uint32_t PUPDR;
  __IO uint32_t IDR;
  __IO uint32_t ODR;
  __IO uint32_t BSRR;
  __IO uint32_t LCKR;
  __IO uint32_t AFR[2];
} GPIO_TypeDef;

typedef struct {
  __IO uint32_t CFGR;
  __IO uint32_t AHB1ENR;
  __IO uint32_t APB2ENR;
} RCC_TypeDef;

typedef struct {
  __IO uint32_t SR;
  __IO uint32_t CR1;
  __IO uint32_t CR2;
  __IO uint32_t SMPR1;
  __IO uint32_t SMPR2;
  __IO uint32_t JOFR1;
  __IO uint32_t JOFR2;
  __IO uint32_t JOFR3;
  __IO uint32_t JOFR4;
  __IO uint32_t HTR;
  __IO uint32_t LTR;
  __IO uint32_t SQR1;
  __IO uint32_t SQR2;
  __IO uint32_t SQR3;
  __IO uint32_t JSQR;
  __IO uint32_t JDR1;
  __IO uint32_t JDR2;
  __IO uint32_t JDR3;
  __IO uint32_t JDR4;
  __IO uint32_t DR;
} ADC_TypeDef;

typedef struct {
  __IO uint32_t CSR;
  __IO uint32_t CCR;
  __IO uint32_t CDR;
} ADC_Common_TypeDef;

typedef struct {
  __IO uint32_t CR1;
  __IO uint32_t CR2;
  __IO uint32_t SMCR;
  __IO uint32_t DIER;
  __IO uint32_t SR;
  __IO uint32_t EGR;
  __IO uint32_t CCMR1;
  __IO uint32_t CCMR2;
  __IO uint32_t CCER;
  __IO uint32_t CNT;
  __IO uint32_t PSC;
  __IO uint32_t ARR;
  __IO uint32_t RCR;
  __IO uint32_t CCR1;
  __IO uint32_t CCR2;
  __IO uint32_t CCR3;
  __IO uint32_t CCR4;
  __IO uint32_t BDTR;
  __IO uint32_t DCR;
  __IO uint32_t DMAR;
  __IO uint32_t OR;
} TIM_TypeDef;

// Layout only; no test drives a USART.
typedef struct {
  __IO uint32_t SR;
  __IO uint32_t DR;
  __IO uint32_t BRR;
  __IO uint32_t CR1;
  __IO uint32_t CR2;
  __IO uint32_t CR3;
  __IO uint32_t GTPR;
} USART_TypeDef;

inline RCC_TypeDef fake_rcc;
alignas(GPIO_TypeDef) inline uint8_t fake_gpio_memory[9 * 0x400];
inline uint32_t fake_primask = 0;
inline int fake_irq_disable_count = 0;

#define RCC (&fake_rcc)

inline TIM_TypeDef fake_tims[15];
inline bool fake_tim_clock_on[15];
inline uint32_t fake_pclk1_hz = 42000000;
inline uint32_t fake_pclk2_hz = 84000000;
inline uint32_t HAL_RCC_GetPCLK1Freq() { return fake_pclk1_hz; }
inline uint32_t HAL_RCC_GetPCLK2Freq() { return fake_pclk2_hz; }

#define TIM1 (&fake_tims[1])
#define TIM2 (&fake_tims[2])
#define TIM3 (&fake_tims[3])
#define TIM4 (&fake_tims[4])
#define TIM5 (&fake_tims[5])
#define TIM6 (&fake_tims[6])
#define TIM7 (&fake_tims[7])
#define TIM8 (&fake_tims[8])
#define TIM9 (&fake_tims[9])
#define TIM10 (&fake_tims[10])
#define TIM11 (&fake_tims[11])
#define TIM12 (&fake_tims[12])
#define TIM13 (&fake_tims[13])
#define TIM14 (&fake_tims[14])
#define __HAL_RCC_TIM1_CLK_ENABLE() (fake_tim_clock_on[1] = true)
#define __HAL_RCC_TIM2_CLK_ENABLE() (fake_tim_clock_on[2] = true)
#define __HAL_RCC_TIM3_CLK_ENABLE() (fake_tim_clock_on[3] = true)
#define __HAL_RCC_TIM4_CLK_ENABLE() (fake_tim_clock_on[4] = true)
#define __HAL_RCC_TIM5_CLK_ENABLE() (fake_tim_clock_on[5] = true)
#define __HAL_RCC_TIM6_CLK_ENABLE() (fake_tim_clock_on[6] = true)
#define __HAL_RCC_TIM7_CLK_ENABLE() (fake_tim_clock_on[7] = true)
#define __HAL_RCC_TIM8_CLK_ENABLE() (fake_tim_clock_on[8] = true)
#define __HAL_RCC_TIM9_CLK_ENABLE() (fake_tim_clock_on[9] = true)
#define __HAL_RCC_TIM10_CLK_ENABLE() (fake_tim_clock_on[10] = true)
#define __HAL_RCC_TIM11_CLK_ENABLE() (fake_tim_clock_on[11] = true)
#define __HAL_RCC_TIM12_CLK_ENABLE() (fake_tim_clock_on[12] = true)
#define __HAL_RCC_TIM13_CLK_ENABLE() (fake_tim_clock_on[13] = true)
#define __HAL_RCC_TIM14_CLK_ENABLE() (fake_tim_clock_on[14] = true)

#define RCC_CFGR_PPRE1 (0x7UL << 10)
#define RCC_CFGR_PPRE1_DIV1 0x00000000U
#define RCC_CFGR_PPRE1_DIV4 0x00001400U
#define RCC_CFGR_PPRE2 (0x7UL << 13)
#define RCC_CFGR_PPRE2_DIV1 0x00000000U
#define RCC_CFGR_PPRE2_DIV2 0x00008000U
inline ADC_TypeDef fake_adc1;
inline ADC_Common_TypeDef fake_adc_common;
#define ADC1 (&fake_adc1)
#define ADC123_COMMON (&fake_adc_common)
#define RCC_APB2ENR_ADC1EN (1UL << 8)
#define ADC_CR1_RES_Pos 24U
#define ADC_CR2_ADON (1UL << 0)
#define ADC_CR2_SWSTART (1UL << 30)
#define ADC_SR_EOC (1UL << 1)
#define ADC_CCR_ADCPRE_Pos 16U
#define ADC_CCR_ADCPRE (0x3UL << 16)
#define TIM_CR1_CEN (1UL << 0)
#define TIM_CR1_ARPE (1UL << 7)
#define TIM_EGR_UG (1UL << 0)
#define TIM_CCMR1_OC1PE (1UL << 3)
#define TIM_CCMR1_OC1M_1 (0x2UL << 4)
#define TIM_CCMR1_OC1M_2 (0x4UL << 4)
#define TIM_CCER_CC1E (1UL << 0)
#define GPIOA_BASE (reinterpret_cast<uintptr_t>(fake_gpio_memory))
#define RCC_AHB1ENR_GPIOAEN (1UL << 0)

inline uint32_t __get_PRIMASK() { return fake_primask; }
inline void __set_PRIMASK(uint32_t primask) { fake_primask = primask; }
inline void __disable_irq() {
  fake_primask = 1;
  ++fake_irq_disable_count;
}
