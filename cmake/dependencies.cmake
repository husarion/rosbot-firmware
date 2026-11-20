# Copyright 2026 Husarion sp. z o.o.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Third-party sources, pinned. SOURCE_SUBDIR points at a directory that
# does not exist, so FetchContent only downloads and never runs a
# project's own CMakeLists.txt; the targets are defined below.

include(FetchContent)

set(_no_cmake SOURCE_SUBDIR do-not-add)

# HAL 1.8.1 + CMSIS device 2.6.8: the versions the stm32duino core shipped.
# The core's ETH driver was HAL's Legacy one (identical code).
FetchContent_Declare(stm32f4_hal
  GIT_REPOSITORY https://github.com/STMicroelectronics/stm32f4xx-hal-driver
  GIT_TAG v1.8.1
  GIT_SHALLOW TRUE
  ${_no_cmake})
FetchContent_Declare(cmsis_device_f4
  GIT_REPOSITORY https://github.com/STMicroelectronics/cmsis-device-f4
  GIT_TAG v2.6.8
  GIT_SHALLOW TRUE
  ${_no_cmake})
# Same kernel as STM32duino FreeRTOS 10.3.3.
FetchContent_Declare(freertos_kernel
  GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS-Kernel
  GIT_TAG V10.3.1-kernel-only
  GIT_SHALLOW TRUE
  ${_no_cmake})
FetchContent_Declare(lwip
  GIT_REPOSITORY https://github.com/STMicroelectronics/stm32_mw_lwip
  GIT_TAG v2.2.1_20250804
  GIT_SHALLOW TRUE
  ${_no_cmake})
FetchContent_Declare(etl
  GIT_REPOSITORY https://github.com/ETLCPP/etl
  GIT_TAG 20.49.0
  GIT_SHALLOW TRUE
  ${_no_cmake})

FetchContent_MakeAvailable(stm32f4_hal cmsis_device_f4 freertos_kernel lwip etl)

set(BOARD_DIR ${REPO_ROOT}/boards/stm32f407)

# CMSIS + HAL + startup, shared by both boards.
set(_hal ${stm32f4_hal_SOURCE_DIR}/Src)
# Startup goes into the firmware's own objects (an archive member is only
# linked when referenced, and nothing references the vector table).
set(STM32F4_STARTUP_SOURCES
  ${cmsis_device_f4_SOURCE_DIR}/Source/Templates/gcc/startup_stm32f407xx.s)
# The only vendor sources that read HSE_VALUE, which differs per board:
# add_firmware() compiles them once per board.
set(STM32F4_CLOCK_SOURCES
  ${cmsis_device_f4_SOURCE_DIR}/Source/Templates/system_stm32f4xx.c
  ${_hal}/stm32f4xx_hal_rcc.c
  ${_hal}/stm32f4xx_hal_rcc_ex.c)
set_source_files_properties(${STM32F4_STARTUP_SOURCES} ${STM32F4_CLOCK_SOURCES}
  PROPERTIES COMPILE_OPTIONS -w)
add_library(stm32f4_hal STATIC
  ${_hal}/stm32f4xx_hal.c
  ${_hal}/stm32f4xx_hal_cortex.c
  ${_hal}/stm32f4xx_hal_dma.c
  ${_hal}/stm32f4xx_hal_dma_ex.c
  ${_hal}/stm32f4xx_hal_exti.c
  ${_hal}/stm32f4xx_hal_flash.c
  ${_hal}/stm32f4xx_hal_flash_ex.c
  ${_hal}/stm32f4xx_hal_flash_ramfunc.c
  ${_hal}/stm32f4xx_hal_gpio.c
  ${_hal}/stm32f4xx_hal_i2c.c
  ${_hal}/stm32f4xx_hal_i2c_ex.c
  ${_hal}/stm32f4xx_hal_pwr.c
  ${_hal}/stm32f4xx_hal_pwr_ex.c
  ${_hal}/stm32f4xx_hal_tim.c
  ${_hal}/stm32f4xx_hal_tim_ex.c
  ${_hal}/Legacy/stm32f4xx_hal_eth.c)
target_include_directories(stm32f4_hal SYSTEM PUBLIC
  ${BOARD_DIR}
  ${REPO_ROOT}/third_party/cmsis_core/Include
  ${cmsis_device_f4_SOURCE_DIR}/Include
  ${stm32f4_hal_SOURCE_DIR}/Inc)
target_compile_definitions(stm32f4_hal PUBLIC
  STM32F407xx
  USE_HAL_DRIVER
  USER_VECT_TAB_ADDRESS)
# Vendor code: its warnings are not ours to fix.
target_compile_options(stm32f4_hal PRIVATE -w)

set(_rtos ${freertos_kernel_SOURCE_DIR})
add_library(freertos STATIC
  ${_rtos}/event_groups.c
  ${_rtos}/list.c
  ${_rtos}/queue.c
  ${_rtos}/stream_buffer.c
  ${_rtos}/tasks.c
  ${_rtos}/timers.c
  ${_rtos}/portable/GCC/ARM_CM4F/port.c)
target_include_directories(freertos SYSTEM PUBLIC
  ${BOARD_DIR}
  ${_rtos}/include
  ${_rtos}/portable/GCC/ARM_CM4F)
target_compile_options(freertos PRIVATE -w)

# NO_SYS lwIP: core, IPv4 and the Ethernet netif are all lib/ethernet uses.
file(GLOB _lwip_src CONFIGURE_DEPENDS
  ${lwip_SOURCE_DIR}/src/core/*.c
  ${lwip_SOURCE_DIR}/src/core/ipv4/*.c)
add_library(lwip STATIC ${_lwip_src} ${lwip_SOURCE_DIR}/src/netif/ethernet.c)
target_include_directories(lwip SYSTEM PUBLIC
  ${REPO_ROOT}/include
  ${lwip_SOURCE_DIR}/src/include
  ${lwip_SOURCE_DIR}/system)
target_compile_options(lwip PRIVATE -w)

add_library(etl INTERFACE)
target_include_directories(etl SYSTEM INTERFACE ${etl_SOURCE_DIR}/include)
