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

#include "imu_calibration_runtime.hpp"

#include "persistent_config.hpp"

namespace imu_calibration {

void serviceSave(ImuInterface& imu) {
  g_control.state.store(static_cast<uint8_t>(SaveState::kSaving));

  SaveState result = SaveState::kSaved;
  ImuCalibrationStatus status{};
  ImuCalibrationOffsets offsets{};
  if (!imu.calibrationStatus(status) || !status.fullyCalibrated()) {
    result = SaveState::kRejected;
  } else if (!imu.readCalibrationOffsets(offsets) ||
             !persistent_config::saveImuCalibration(offsets)) {
    result = SaveState::kFailed;
  }

  g_control.state.store(static_cast<uint8_t>(result));
  g_control.save_seq.fetch_add(1);
  if (result == SaveState::kSaved) stopSession();
}

void startSession(uint32_t now_ms) {
  g_control.session_started_ms.store(now_ms);
  g_control.session_active.store(true);
}

void stopSession() { g_control.session_active.store(false); }

bool sessionActive(uint32_t now_ms) {
  if (!g_control.session_active.load()) return false;
  if (now_ms - g_control.session_started_ms.load() > kSessionTimeoutMs) {
    stopSession();
    return false;
  }
  return true;
}

}  // namespace imu_calibration
