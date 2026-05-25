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

// Common interface so the uRos task can drive either RosNode or
// MavlinkNode through g_link.
class RoboticsLink {
 public:
  virtual ~RoboticsLink() = default;
  virtual void loop() = 0;
  virtual bool isConnected() const = 0;
  virtual void setNamespace(const char* ns) = 0;
  virtual void setDiagnosticSerial(HardwareSerial* serial) = 0;
};

// Assigned in setup() before vTaskStartScheduler(); read-only thereafter.
extern RoboticsLink* g_link;
