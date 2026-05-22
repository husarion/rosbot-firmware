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

#include <HardwareSerial.h>

// Abstract upstream-link layer. Both RosNode (micro-ROS) and MavlinkNode
// implement this so the FreeRTOS task harness in src/<variant>/rtos.cpp can
// drive either build without an #ifdef ladder. See MAVLINK_MIGRATION.md
// §8.2 "Implementation note".
class RoboticsLink {
 public:
  virtual ~RoboticsLink() = default;
  /// State-machine + spin tick. Called at the uRos task period.
  virtual void loop() = 0;
  /// True when the upstream peer is reachable and entities exist.
  virtual bool isConnected() const = 0;
  /// Logical namespace selected on boot via CommunicationManager.
  virtual void setNamespace(const char* ns) = 0;
  /// Optional FTDI serial for STATUSTEXT / log forwarding.
  virtual void setDiagnosticSerial(HardwareSerial* serial) = 0;
};

/// Selected at runtime in the variant's setup() once the host driver
/// has indicated its choice via CommunicationManager::getSelectedBackend
/// (default: MICRO_ROS when no BACKEND: line is seen). nullptr until
/// setup() finishes the dispatch — but FreeRTOS tasks only read this
/// after vTaskStartScheduler(), by which point the assignment is done.
extern RoboticsLink* g_link;
