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

#include "pin.hpp"

namespace platform {

// Single-shot 10-bit conversion on ADC1. Every analog pin both boards use is
// on ADC1; channel is the ADC1_INx number of the pin.
class AnalogIn {
 public:
  static constexpr uint16_t kMaxValue = 1023;

  constexpr AnalogIn(Pin pin, uint8_t channel) : pin_(pin), channel_(channel) {}

  void init() const;

  // Blocks ~1.2 us with interrupts masked, so tasks sharing ADC1 cannot
  // interleave. Returns 0 if the conversion does not finish.
  uint16_t read() const;

 private:
  Pin pin_;
  uint8_t channel_;
};

}  // namespace platform
