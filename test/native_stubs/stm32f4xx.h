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

// Host-test stand-in for the CMSIS device header: the GPIO ports and RCC
// live in plain memory the test can inspect, laid out at the F4 stride.

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
  __IO uint32_t AHB1ENR;
} RCC_TypeDef;

inline RCC_TypeDef fake_rcc;
alignas(GPIO_TypeDef) inline uint8_t fake_gpio_memory[9 * 0x400];
inline uint32_t fake_primask = 0;
inline int fake_irq_disable_count = 0;

#define RCC (&fake_rcc)
#define GPIOA_BASE (reinterpret_cast<uintptr_t>(fake_gpio_memory))
#define RCC_AHB1ENR_GPIOAEN (1UL << 0)

inline uint32_t __get_PRIMASK() { return fake_primask; }
inline void __set_PRIMASK(uint32_t primask) { fake_primask = primask; }
inline void __disable_irq() {
  fake_primask = 1;
  ++fake_irq_disable_count;
}
