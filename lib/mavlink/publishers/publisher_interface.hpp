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

#include "mavlink_node.hpp"

// Periodic-publisher facade for the MAVLink stack. Each concrete publisher
// reads a FreeRTOS queue, builds the matching mavlink_message_t, and forwards
// it through MavlinkNode::sendMessage(). Rate limiting is enforced internally
// by the publisher (matches MAVLINK_MIGRATION.md §9 "Telemetry rates").
class MavlinkPublisherInterface {
 public:
  virtual ~MavlinkPublisherInterface() = default;
  /// Called every uRos tick. Each implementation gates itself by the
  /// configured publish period so the node loop doesn't need to schedule
  /// individual publishers.
  virtual void publish(MavlinkNode& node) = 0;
};
