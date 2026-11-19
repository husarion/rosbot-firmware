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

# Host unit tests: hardware-free code from lib/, CMSIS/FreeRTOS replaced by
# test/native_stubs. Run: cmake -S test -B build/test && cmake --build
# build/test && ctest --test-dir build/test

cmake_minimum_required(VERSION 3.25)
project(rosbot_firmware_tests CXX C)

# The build tree holds FetchContent sub-projects colcon would pick up.
file(TOUCH ${CMAKE_BINARY_DIR}/COLCON_IGNORE)

get_filename_component(REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/.. ABSOLUTE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS ON)

include(FetchContent)
FetchContent_Declare(unity
  GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity
  GIT_TAG v2.6.1
  GIT_SHALLOW TRUE
  SOURCE_SUBDIR do-not-add)
# Same pin as cmake/dependencies.cmake.
FetchContent_Declare(etl
  GIT_REPOSITORY https://github.com/ETLCPP/etl
  GIT_TAG 20.49.0
  GIT_SHALLOW TRUE
  SOURCE_SUBDIR do-not-add)
FetchContent_MakeAvailable(unity etl)

add_library(unity STATIC ${unity_SOURCE_DIR}/src/unity.c)
target_include_directories(unity SYSTEM PUBLIC ${unity_SOURCE_DIR}/src)

set(WARNINGS -Wall -Wextra -Wdouble-promotion -Wshadow)

# Stubs first, so they shadow the real CMSIS/FreeRTOS headers.
set(TEST_INCLUDES
  ${CMAKE_CURRENT_SOURCE_DIR}/native_stubs
  ${REPO_ROOT}/lib/battery
  ${REPO_ROOT}/lib/boot_option
  ${REPO_ROOT}/lib/diagnostics
  ${REPO_ROOT}/lib/encoder
  ${REPO_ROOT}/lib/fan
  ${REPO_ROOT}/lib/indicator
  ${REPO_ROOT}/lib/pid
  ${REPO_ROOT}/lib/platform
  ${REPO_ROOT}/lib/power_board
  ${REPO_ROOT}/lib/time_base)

# Libraries whose sources build on the host as they are. A test that needs
# a hardware driver includes its .cpp directly, against the fake registers.
add_library(host_libs STATIC
  ${REPO_ROOT}/lib/pid/pid.cpp
  ${REPO_ROOT}/lib/time_base/time_base.cpp)
target_include_directories(host_libs PUBLIC ${TEST_INCLUDES})
target_include_directories(host_libs SYSTEM PUBLIC ${etl_SOURCE_DIR}/include)
target_compile_options(host_libs PRIVATE ${WARNINGS})

enable_testing()
file(GLOB _tests CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/test_*/test_*.cpp)
foreach(src ${_tests})
  get_filename_component(name ${src} NAME_WE)
  add_executable(${name} ${src})
  target_link_libraries(${name} PRIVATE host_libs unity)
  target_compile_options(${name} PRIVATE ${WARNINGS})
  add_test(NAME ${name} COMMAND ${name})
endforeach()
