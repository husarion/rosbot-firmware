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
#include <math.h>

// ─── Configuration ───────────────────────────────────────────────
struct NtcConfig {
  uint8_t pin;              // analog input pin
  float pullup_resistance;  // pull-up resistor value [Ohm]
  float c1;                 // Steinhart-Hart coefficient
  float c2;                 // Steinhart-Hart coefficient
  float c3;                 // Steinhart-Hart coefficient
  float offset = 273.15f;   // Kelvin -> Celsius
  float adc_max = 1023.0f;  // 10-bit: 1023, 12-bit: 4095
};

// ─── NTC Temperature Sensor ─────────────────────────────────────
class Ntc {
 public:
  explicit Ntc(const NtcConfig& config);

  void init();

  float readCelsius() const;

 private:
  float adcToResistance(float adc_value) const;
  float resistanceToCelsius(float resistance) const;

  NtcConfig config_;
};
