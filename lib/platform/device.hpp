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

// CMSIS device + HAL. Until the Arduino core is gone (migration step 9) its
// stm32_def.h must be the first device header in a TU, or the core's own
// headers fail to build.
#if __has_include(<stm32_def.h>)
#include <stm32_def.h>
#else
#include "stm32f4xx.h"
#endif
