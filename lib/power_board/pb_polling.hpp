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

#include "battery_interface.hpp"
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

inline constexpr uint32_t PB_PARAM_FLAGS_POLL_PERIOD_MS = 1000;

enum class PbRequest : uint8_t { None, BatteryState, BoardInfo, ParamFlags };

// Side requests (board info, param flags) go half a battery period after the
// previous request: back-to-back requests to a slow PB lost replies on HW
// (2026-09, cause unconfirmed). With a fast PB the period is 0, so a side
// request just takes one tick from battery polling.
inline PbRequest pbNextRequest(uint32_t battery_period_ms,
                               uint32_t since_battery_ms, uint32_t since_any_ms,
                               uint32_t since_flags_ms,
                               bool board_info_pending) {
  const uint32_t gap_ms = battery_period_ms / 2;
  if (board_info_pending && since_any_ms > gap_ms) return PbRequest::BoardInfo;
  if (since_flags_ms >= PB_PARAM_FLAGS_POLL_PERIOD_MS && since_any_ms > gap_ms)
    return PbRequest::ParamFlags;
  if (since_battery_ms > battery_period_ms) return PbRequest::BatteryState;
  return PbRequest::None;
}

// ParamFlags_1 (cmd 20) byte 4: bit 0 IND_ADPT_SNS, bit 1 USBC_ADPT_SNS.
// The PB charges from USB-C when both are present.
inline constexpr uint8_t PB_FLAGS_ADAPTER_BYTE = 4;

inline uint8_t pbChargerTypeFromFlags(uint8_t adapter_byte) {
  if (adapter_byte & 0x02) return CHARGER_TYPE_USB_C;
  if (adapter_byte & 0x01) return CHARGER_TYPE_INDUCTIVE;
  return CHARGER_TYPE_NONE;
}
