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

/// Upstream-link implementation selected at boot.
///
/// The choice is communicated from the host driver during the pre-comm
/// handshake (see CommunicationManager::waitForHostConfig — the host
/// sends a "BACKEND:microros" / "BACKEND:mavlink" line on the SBC serial
/// link before the namespace exchange). When the line is absent (timeout
/// or older host driver), the firmware falls back to MICRO_ROS so that
/// existing deployments keep working.
enum class CommBackend : uint8_t {
  MICRO_ROS = 0,
  MAVLINK = 1,
};
