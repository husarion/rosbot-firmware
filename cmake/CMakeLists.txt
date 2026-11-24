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

cmake_minimum_required(VERSION 3.25)

# Lives in cmake/ (with COLCON_IGNORE), not the repo root: a CMakeLists.txt
# there makes colcon take the whole repo for one package and stop before
# bridge/rosbot_mavlink_bridge.
get_filename_component(REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/.. ABSOLUTE)

if(NOT CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/arm-none-eabi.cmake)
endif()

project(rosbot_firmware C CXX ASM)

# The build tree holds FetchContent sub-projects colcon would pick up.
file(TOUCH ${CMAKE_BINARY_DIR}/COLCON_IGNORE)

set(FW_VERSION "v2.5.0")

# One build: the flags below do not depend on the CMake build type.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type (flags are set below)" FORCE)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(MCU_FLAGS -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard)

# -O2. Against -Os on ROSbot 3 (2026-11-19): idle 96.1 %
# vs 94.9 %, link task 2.4 % vs 3.3 % CPU, image 108 KB vs 72 KB of 896 KB.
set(FW_OPT_LEVEL "2" CACHE STRING "GCC optimisation level for firmware code (s, 2, ...)")
set(CMAKE_C_FLAGS_DEBUG "")
set(CMAKE_CXX_FLAGS_DEBUG "")
set(CMAKE_C_FLAGS_RELEASE "")
set(CMAKE_CXX_FLAGS_RELEASE "")
set(CMAKE_ASM_FLAGS_DEBUG "")
set(CMAKE_ASM_FLAGS_RELEASE "")
add_compile_options(
  ${MCU_FLAGS}
  -O${FW_OPT_LEVEL}
  -g
  -ffunction-sections
  -fdata-sections
  -funroll-loops
  $<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-use-cxa-atexit>)
add_link_options(
  ${MCU_FLAGS}
  --specs=nano.specs
  -Wl,--gc-sections,--relax
  -Wl,--check-sections
  -Wl,--unresolved-symbols=report-all
  -Wl,--warn-common
  -Wl,--print-memory-usage)

include(dependencies.cmake)

set(WARNINGS
  -Wall
  -Wextra
  -Wfatal-errors
  -Wmissing-field-initializers
  -Wdouble-promotion
  -Wshadow
  # MAVLink char[N] fields need no NUL by spec; the generated strncpy into
  # them truncates on purpose, and -O2 inlining reports it in our files.
  -Wno-stringop-truncation)

file(GLOB _lib_dirs LIST_DIRECTORIES true ${REPO_ROOT}/lib/*)
file(GLOB_RECURSE _common_src CONFIGURE_DEPENDS
  ${REPO_ROOT}/lib/*.cpp
  ${REPO_ROOT}/lib/*.c)
list(FILTER _common_src EXCLUDE REGEX "/lib/ethernet/")

# Everything both boards share. OBJECT, not STATIC: IRQ handlers that
# nothing calls must still reach the link.
add_library(fw_common OBJECT
  ${_common_src}
  ${STM32F4_STARTUP_SOURCES}
  ${BOARD_DIR}/heap_newlib.c
  ${BOARD_DIR}/syscalls.c)
target_include_directories(fw_common PUBLIC
  ${REPO_ROOT}/include
  ${_lib_dirs})
# Generated MAVLink headers.
target_include_directories(fw_common SYSTEM PUBLIC
  ${REPO_ROOT}/bridge/rosbot_mavlink_bridge/mavlink_dialect/rosbot)
target_compile_definitions(fw_common PUBLIC
  FW_VERSION="${FW_VERSION}")
target_link_libraries(fw_common PUBLIC stm32f4_hal freertos lwip etl)
target_compile_options(fw_common PRIVATE ${WARNINGS})

# Vendored, kept as upstream wrote it.
set_source_files_properties(
  ${REPO_ROOT}/lib/vl53l0x/VL53L0X.cpp
  PROPERTIES COMPILE_OPTIONS -Wno-shadow)

add_library(fw_ethernet OBJECT
  ${REPO_ROOT}/lib/ethernet/ethernet.cpp
  ${REPO_ROOT}/lib/ethernet/ethernetif.cpp)
target_link_libraries(fw_ethernet PUBLIC fw_common)
target_compile_options(fw_ethernet PRIVATE ${WARNINGS})

# HSE_HZ: the board's crystal; the clock tree and HAL's clock math need it.
function(add_firmware name)
  cmake_parse_arguments(ARG "" "HSE_HZ" "OBJECTS" ${ARGN})
  file(GLOB _src CONFIGURE_DEPENDS ${REPO_ROOT}/src/${name}/*.cpp)
  add_executable(${name} ${_src} ${BOARD_DIR}/system.cpp
                 ${STM32F4_CLOCK_SOURCES})
  target_compile_definitions(${name} PRIVATE HSE_VALUE=${ARG_HSE_HZ}U)
  set_target_properties(${name} PROPERTIES SUFFIX ".elf")
  target_link_libraries(${name} PRIVATE fw_common ${ARG_OBJECTS})
  target_compile_options(${name} PRIVATE ${WARNINGS})
  target_link_options(${name} PRIVATE
    -T${BOARD_DIR}/STM32F407ZGTx_FLASH.ld
    -Wl,-Map=$<TARGET_FILE_DIR:${name}>/${name}.map)
  set_property(TARGET ${name} APPEND PROPERTY LINK_DEPENDS
    ${BOARD_DIR}/STM32F407ZGTx_FLASH.ld)
  add_custom_command(TARGET ${name} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${name}>
            $<TARGET_FILE_DIR:${name}>/${name}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${name}>
    VERBATIM)
endfunction()

add_firmware(rosbot HSE_HZ 8000000)  # CORE2 v1.3+ Q1, measured on HW
add_firmware(rosbot_xl HSE_HZ 24000000 OBJECTS fw_ethernet)  # Digital Board Q1
