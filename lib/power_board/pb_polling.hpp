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

#include "version_text.hpp"

// Power board firmware v1.0.3 serves UART between its slow passes and
// refreshes voltage and currents every 100 ms; older versions answer once
// per ~750 ms pass and drop a second request that arrives in the same pass.
// An unknown version (board info not received yet) counts as old.
inline bool pbSupportsFastPolling(const char* firmware_version) {
  return versionAtLeast(firmware_version, 1, 0, 3);
}

inline constexpr uint32_t PB_SLOW_POLL_PERIOD_MS = 1000;

// 0 = request on every HwMonitor tick.
inline uint32_t pbBatteryPollPeriodMs(const char* firmware_version) {
  return pbSupportsFastPolling(firmware_version) ? 0 : PB_SLOW_POLL_PERIOD_MS;
}
