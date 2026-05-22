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

// Abstract upstream-link backend. Both RosNode and MavlinkNode implement
// it so the uRos task drives either via g_link.
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

// nullptr until setup() assigns it before vTaskStartScheduler();
// tasks only read it from their loop bodies, past that point.
extern RoboticsLink* g_link;
