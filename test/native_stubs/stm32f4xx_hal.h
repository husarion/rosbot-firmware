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

#pragma once

// The slice of the HAL TIM API lib/fan uses, acting on the fake TIM
// registers in stm32f4xx.h and recording which outputs were started.

#include "stm32f4xx.h"

typedef enum { HAL_OK = 0, HAL_ERROR = 1 } HAL_StatusTypeDef;

typedef struct {
  uint32_t Prescaler;
  uint32_t CounterMode;
  uint32_t Period;
  uint32_t ClockDivision;
  uint32_t RepetitionCounter;
  uint32_t AutoReloadPreload;
} TIM_Base_InitTypeDef;

typedef struct {
  TIM_TypeDef* Instance;
  TIM_Base_InitTypeDef Init;
} TIM_HandleTypeDef;

typedef struct {
  uint32_t OCMode;
  uint32_t Pulse;
  uint32_t OCPolarity;
  uint32_t OCNPolarity;
  uint32_t OCFastMode;
  uint32_t OCIdleState;
  uint32_t OCNIdleState;
} TIM_OC_InitTypeDef;

#define TIM_COUNTERMODE_UP 0U
#define TIM_CLOCKDIVISION_DIV1 0U
#define TIM_AUTORELOAD_PRELOAD_ENABLE TIM_CR1_ARPE
#define TIM_OCMODE_PWM1 0x60U
#define TIM_OCPOLARITY_HIGH 0U
#define TIM_OCNPOLARITY_HIGH 0U
#define TIM_OCNPOLARITY_LOW 8U
#define TIM_OCFAST_DISABLE 0U
#define TIM_OCIDLESTATE_RESET 0U
#define TIM_OCNIDLESTATE_RESET 0U
#define TIM_BDTR_MOE (1UL << 15)

struct FakeTimHal {
  int pwm_started = 0;   // HAL_TIM_PWM_Start minus _Stop
  int pwmn_started = 0;  // HAL_TIMEx_PWMN_Start minus _Stop
  uint32_t last_channel = 0xFFFFFFFFU;
  uint32_t oc_mode = 0;
  bool deinit = false;
};
inline FakeTimHal fake_tim_hal;

inline volatile uint32_t& fakeCcr(TIM_TypeDef* tim, uint32_t channel) {
  switch (channel) {
    case 0x0U:
      return tim->CCR1;
    case 0x4U:
      return tim->CCR2;
    case 0x8U:
      return tim->CCR3;
    default:
      return tim->CCR4;
  }
}

inline HAL_StatusTypeDef HAL_TIM_PWM_Init(TIM_HandleTypeDef* h) {
  h->Instance->PSC = h->Init.Prescaler;
  h->Instance->ARR = h->Init.Period;
  h->Instance->CR1 = h->Init.AutoReloadPreload;
  return HAL_OK;
}

inline HAL_StatusTypeDef HAL_TIM_PWM_DeInit(TIM_HandleTypeDef*) {
  fake_tim_hal.deinit = true;
  return HAL_OK;
}

inline HAL_StatusTypeDef HAL_TIM_PWM_ConfigChannel(TIM_HandleTypeDef* h,
                                                   const TIM_OC_InitTypeDef* oc,
                                                   uint32_t channel) {
  fake_tim_hal.oc_mode = oc->OCMode;
  fakeCcr(h->Instance, channel) = oc->Pulse;
  return HAL_OK;
}

inline HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef*, uint32_t ch) {
  ++fake_tim_hal.pwm_started;
  fake_tim_hal.last_channel = ch;
  return HAL_OK;
}

inline HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef*, uint32_t) {
  --fake_tim_hal.pwm_started;
  return HAL_OK;
}

inline HAL_StatusTypeDef HAL_TIMEx_PWMN_Start(TIM_HandleTypeDef*, uint32_t ch) {
  ++fake_tim_hal.pwmn_started;
  fake_tim_hal.last_channel = ch;
  return HAL_OK;
}

inline HAL_StatusTypeDef HAL_TIMEx_PWMN_Stop(TIM_HandleTypeDef*, uint32_t) {
  --fake_tim_hal.pwmn_started;
  return HAL_OK;
}

#define IS_TIM_BREAK_INSTANCE(t) ((t) == TIM1 || (t) == TIM8)
#define __HAL_TIM_MOE_ENABLE(h) ((h)->Instance->BDTR |= TIM_BDTR_MOE)
#define __HAL_TIM_GET_AUTORELOAD(h) ((h)->Instance->ARR)
#define __HAL_TIM_SET_COMPARE(h, ch, v) (fakeCcr((h)->Instance, (ch)) = (v))
