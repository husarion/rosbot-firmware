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

// Last-known good handshake state, persisted in STM32F407 flash sector 11
// (0x080E0000, 128 KB). Sector 11 is well past our ~22 % code-fill, but the
// linker is not asked to reserve it — keep the code under sector 10.
//
// Fresh flash (erased to 0xFF) reads back as defaults: MAVLINK + empty
// namespace. main.cpp uses load() to seed the handshake fallbacks, then
// save()s after the handshake settles. save() is no-op when the new
// values match the last load(), so flash wear scales with config changes,
// not boots.
namespace persistent_config {

inline constexpr size_t kNamespaceMaxLen = 32;

struct Config {
  CommBackend backend;
  char ns[kNamespaceMaxLen];
};

Config load();
void save(const Config& cfg);

}  // namespace persistent_config
