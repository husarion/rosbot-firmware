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

#include <netinet/in.h>

#include <atomic>
#include <string>

#include "rosbot_mavlink_bridge/transport/transport_interface.hpp"

namespace rosbot_mavlink_bridge {

struct UdpConfig {
  std::string peer_ip;
  std::uint16_t peer_port = 14555;
  std::uint16_t local_port = 14550;
};

class UdpTransport : public Transport {
 public:
  explicit UdpTransport(const UdpConfig& cfg) : cfg_(cfg) {}
  ~UdpTransport() override { UdpTransport::close(); }

  bool open() override;
  void close() override;
  std::size_t write(const std::uint8_t* buf, std::size_t len) override;
  std::size_t read(std::uint8_t* buf, std::size_t len, int timeout_ms) override;

 private:
  UdpConfig cfg_;
  std::atomic<int> fd_{-1};
  sockaddr_in peer_addr_{};
};

}  // namespace rosbot_mavlink_bridge
