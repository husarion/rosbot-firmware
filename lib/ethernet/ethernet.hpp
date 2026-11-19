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

#include <cstdint>

#include "lwip/ip_addr.h"

namespace ethernet {

struct Config {
  uint8_t mac[6];
  ip4_addr_t ip;
  ip4_addr_t netmask;
  ip4_addr_t gateway;
};

// Brings up the MAC + LAN9303 and lwIP with a static address, then starts
// the task that owns lwIP from here on (receive, timers, link checks, every
// raw-API callback). Call once from setup(), before the scheduler starts.
bool begin(const Config& cfg);

// PHY link state as of the last link check (every 500 ms).
bool linkUp();

}  // namespace ethernet
