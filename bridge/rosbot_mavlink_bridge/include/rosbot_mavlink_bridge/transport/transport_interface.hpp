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

namespace rosbot_mavlink_bridge {

class Transport {
 public:
  virtual ~Transport() = default;
  virtual bool open() = 0;
  virtual void close() = 0;
  virtual std::size_t write(const std::uint8_t* buf, std::size_t len) = 0;
  virtual std::size_t read(std::uint8_t* buf, std::size_t len,
                           int timeout_ms) = 0;
};

}  // namespace rosbot_mavlink_bridge
