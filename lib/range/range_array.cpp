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

#include "range_array.hpp"

#include <Arduino.h>
#include <etl/algorithm.h>

RangeArray::RangeArray(etl::span<RangeInterface* const> sensors)
    : sensors_(sensors.first(
          etl::min(sensors.size(), size_t{MAX_NUM_RANGE_SENSORS}))) {
  data_.count = count();
}

void RangeArray::init() {
  for (auto* sensor : sensors_) sensor->powerOff();
  for (auto* sensor : sensors_) sensor->init();
}

void RangeArray::update() {
  for (size_t i = 0; i < sensors_.size(); i++) {
    sensors_[i]->update();
    data_.range[i] = sensors_[i]->getData().range;
    strncpy(data_.frame_id[i], sensors_[i]->name(), MAX_FRAME_ID_LENGTH - 1);
  }
}
