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

#include <Arduino.h>

#include "ntc_math.hpp"

// ─── NTC Temperature Sensor ─────────────────────────────────────
class Ntc {
 public:
  explicit Ntc(const NtcConfig& config);

  void init();

  /// NAN when the sensor reads open, shorted or out of range.
  float readCelsius() const;

 private:
  NtcConfig config_;
};
