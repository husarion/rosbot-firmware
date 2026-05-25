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

#include <IPAddress.h>

#include "mavlink_transport_interface.hpp"

// LwIP raw-API UDP transport. MCU binds local_port and sends to
// (peer_ip, peer_port). MAVROS default: local=14555, peer=14550.
struct MavlinkUdpConfig {
  IPAddress peer_ip;
  uint16_t peer_port;
  uint16_t local_port;
};

class MavlinkUdpTransport : public MavlinkTransport {
 public:
  explicit MavlinkUdpTransport(const MavlinkUdpConfig& cfg) : cfg_(cfg) {}
  bool open() override;
  void close() override;
  size_t write(const uint8_t* buf, size_t len) override;
  size_t read(uint8_t* buf, size_t len, uint32_t timeout_ms) override;

 private:
  MavlinkUdpConfig cfg_;
};
