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

// HAL configuration for both boards. Values match the stm32duino core
// (Husarion fork) this firmware used before, including the ETH section
// for the LAN9303 switch on ROSbot XL.

#ifndef STM32F4XX_HAL_CONF_H
#define STM32F4XX_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_ETH_LEGACY_MODULE_ENABLED
#define HAL_EXTI_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED

// HSE_VALUE is per board, from add_firmware() in cmake/CMakeLists.txt, and
// defined only for the sources that read it (STM32F4_CLOCK_SOURCES,
// system.cpp).
#define HSE_STARTUP_TIMEOUT 100U
#define HSI_VALUE 16000000U
#define LSI_VALUE 32000U
#define LSE_VALUE 32768U
#define LSE_STARTUP_TIMEOUT 5000U
#define EXTERNAL_CLOCK_VALUE 12288000U

#define VDD_VALUE 3300U
// Before the scheduler only; xPortStartScheduler() moves SysTick to the
// lowest priority.
#define TICK_INT_PRIORITY 0x00U
#define USE_RTOS 0U
#define PREFETCH_ENABLE 1U
#define INSTRUCTION_CACHE_ENABLE 1U
#define DATA_CACHE_ENABLE 1U

#define USE_HAL_ETH_REGISTER_CALLBACKS 0U
#define USE_HAL_I2C_REGISTER_CALLBACKS 0U
#define USE_HAL_TIM_REGISTER_CALLBACKS 0U

#define USE_SPI_CRC 0U

// ETH (legacy driver). MAC_ADDR* only seed the HAL init struct;
// lib/ethernet sets the real address.
#define MAC_ADDR0 2U
#define MAC_ADDR1 0U
#define MAC_ADDR2 0U
#define MAC_ADDR3 0U
#define MAC_ADDR4 0U
#define MAC_ADDR5 0U
#define ETH_RX_BUF_SIZE ETH_MAX_PACKET_SIZE
#define ETH_TX_BUF_SIZE ETH_MAX_PACKET_SIZE
#define ETH_RXBUFNB 5U
#define ETH_TXBUFNB 5U

#define PHY_RESET_DELAY 0x000000FFU
#define PHY_CONFIG_DELAY 0x00000FFFU
#define PHY_READ_TO 0x0000FFFFU
#define PHY_WRITE_TO 0x0000FFFFU

// LAN9303: its PHY registers are reached through the switch's indirect
// SMI addressing (see lib/ethernet/ethernetif.cpp), hence the remapped
// PHY_* offsets the HAL driver itself uses.
#define LAN9303
#define LAN9303_PHY_ADDRESS 0x0000
#define PHY_ADDR_MASK 0x03C0
#define PHY_ADDR_INIT_VAL 0x0010
#define PHY_REG_MASK 0x003F
#define PHY_REG_ADDR_INIT_VAL 0x0000
#define PHY_REG_HI_WORD_MASK 0x0001
#define PHY_REG_LO_WORD_MASK 0x0000
#define HIGH_WORD_MASK 0x0001
#define LOW_WORD_MASK 0x0000
#define PHY_BCR ((uint16_t)0x01C0)
#define PHY_BSR ((uint16_t)0x01C4)
#define PHY_BSR_AUTONEG_COMPLETE 0x0020
#define PHY_BASE_X_FULL_DUPLEX 0x4000
#define PHY_BASE_X_HALF_DUPLEX 0x2000
#define PHY_BASE_T_FULL_DUPLEX 0x1000
#define PHY_BASE_T_HALF_DUPLEX 0x0800
#define PHY_BASE_T2_FULL_DUPLEX 0x0400
#define PHY_BASE_T2_HALF_DUPLEX 0x0200
#define PHY_RESET ((uint16_t)0x8000)
#define PHY_LOOPBACK ((uint16_t)0x4000)
#define PHY_FULLDUPLEX_100M ((uint16_t)0x2100)
#define PHY_HALFDUPLEX_100M ((uint16_t)0x2000)
#define PHY_FULLDUPLEX_10M ((uint16_t)0x0100)
#define PHY_HALFDUPLEX_10M ((uint16_t)0x0000)
#define PHY_AUTONEGOTIATION ((uint16_t)0x1000)
#define PHY_RESTART_AUTONEGOTIATION ((uint16_t)0x0200)
#define PHY_POWERDOWN ((uint16_t)0x0800)
#define PHY_ISOLATE ((uint16_t)0x0400)
#define PHY_AUTONEGO_COMPLETE ((uint16_t)0x0020)
#define PHY_LINKED_STATUS ((uint16_t)0x0004)
#define PHY_JABBER_DETECTION ((uint16_t)0x0002)
#define PHY_SR ((uint16_t)0x01DC)
#define PHY_SPEED_STATUS ((uint16_t)0x0004)
#define PHY_DUPLEX_STATUS ((uint16_t)0x0010)
#define PHY_ISFR ((uint16_t)0x001D)
#define PHY_IMR ((uint16_t)0x001E)
#define PHY_ISFR_INT4 ((uint16_t)0x0010)

#include "Legacy/stm32f4xx_hal_eth_legacy.h"
#include "stm32f4xx_hal_cortex.h"
#include "stm32f4xx_hal_dma.h"
#include "stm32f4xx_hal_exti.h"
#include "stm32f4xx_hal_flash.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_i2c.h"
#include "stm32f4xx_hal_pwr.h"
#include "stm32f4xx_hal_rcc.h"
#include "stm32f4xx_hal_tim.h"

#define assert_param(expr) ((void)0U)

#ifdef __cplusplus
}
#endif

#endif  // STM32F4XX_HAL_CONF_H
