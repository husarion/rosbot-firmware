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

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Empty: take arm-none-eabi-* from PATH.
set(ARM_TOOLCHAIN_DIR "" CACHE PATH "Directory holding arm-none-eabi-gcc")
if(ARM_TOOLCHAIN_DIR)
  set(_prefix "${ARM_TOOLCHAIN_DIR}/arm-none-eabi-")
else()
  set(_prefix "arm-none-eabi-")
endif()

set(CMAKE_C_COMPILER "${_prefix}gcc")
set(CMAKE_CXX_COMPILER "${_prefix}g++")
set(CMAKE_ASM_COMPILER "${_prefix}gcc")
set(CMAKE_OBJCOPY "${_prefix}objcopy" CACHE FILEPATH "")
set(CMAKE_SIZE "${_prefix}size" CACHE FILEPATH "")

# Compiler checks would try to link a hosted executable.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
