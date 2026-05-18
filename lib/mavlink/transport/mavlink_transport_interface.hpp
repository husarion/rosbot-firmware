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

#include <cstddef>
#include <cstdint>

// Minimal transport facade for MAVLink. MAVLink does its own framing + CRC
// so the wire-protocol layer only needs raw byte exchange. read() must not
// busy-spin — concrete transports either block on an OS primitive
// (stream buffer) or yield with vTaskDelay(1). See MAVLINK_MIGRATION.md
// §3 "Transport callbacks" and §13 "transport patterns preserved".
class MavlinkTransport {
 public:
  virtual ~MavlinkTransport() = default;
  virtual bool open() = 0;
  virtual void close() = 0;
  /// Non-blocking write — returns bytes accepted. Implementations are
  /// expected to be fire-and-forget (DMA on serial, LwIP on UDP).
  virtual size_t write(const uint8_t* buf, size_t len) = 0;
  /// Block up to @p timeout_ms; return bytes copied (may be 0 on timeout).
  virtual size_t read(uint8_t* buf, size_t len, uint32_t timeout_ms) = 0;
};
