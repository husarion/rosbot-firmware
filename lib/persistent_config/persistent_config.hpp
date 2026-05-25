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

#include <cstddef>
#include <cstdint>

#include "comm_backend.hpp"

// Persisted in STM32F407 flash sector 11 (0x080E0000, 128 KB). The linker
// does not reserve the sector — keep total .text under sector 10.
// load() returns defaults (MAVLINK, empty namespace) on a fresh or
// corrupt sector. save() is a no-op when the cached value matches, so
// wear scales with config changes, not boots.
namespace persistent_config {

inline constexpr size_t kNamespaceMaxLen = 32;

struct Config {
  CommBackend backend;
  char ns[kNamespaceMaxLen];
};

Config load();

// Must run before vTaskStartScheduler(); the 1-3 s sector erase stalls
// the instruction bus and starves the motor watchdog / MAVLink TX DMA.
// Asserts if invoked once the scheduler is running.
void save(const Config& cfg);

}  // namespace persistent_config
