// Copyright 2022 Husarion sp. z o.o.
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

#include <etl/span.h>

#include <cstdint>

#include "range_interface.hpp"

#define MAX_NUM_RANGE_SENSORS 4
#define MAX_FRAME_ID_LENGTH 16

struct RangesData {
  float range[MAX_NUM_RANGE_SENSORS] = {};
  char frame_id[MAX_NUM_RANGE_SENSORS][MAX_FRAME_ID_LENGTH] = {};
  uint8_t count = 0;
};

/// Holds N independent RangeInterface pointers.
class RangeArray {
 public:
  RangeArray() = default;
  /// Sensors beyond MAX_NUM_RANGE_SENSORS are ignored.
  explicit RangeArray(etl::span<RangeInterface* const> sensors);

  void init();
  void update();

  const RangesData getData() const { return data_; }
  uint8_t count() const { return static_cast<uint8_t>(sensors_.size()); }
  bool isAvailable() const { return !sensors_.empty(); }

  RangeInterface* operator[](uint8_t idx) {
    return (idx < sensors_.size()) ? sensors_[idx] : nullptr;
  }
  const RangeInterface* operator[](uint8_t idx) const {
    return (idx < sensors_.size()) ? sensors_[idx] : nullptr;
  }

 private:
  etl::span<RangeInterface* const> sensors_;
  RangesData data_ = {};
};

extern RangeArray g_ranges;
