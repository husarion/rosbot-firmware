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

#include <cmath>
#include <cstdint>

struct NtcConfig {
  uint8_t pin;              // analog input pin
  float pullup_resistance;  // pull-up resistor value [Ohm]
  float c1;                 // Steinhart-Hart coefficient
  float c2;                 // Steinhart-Hart coefficient
  float c3;                 // Steinhart-Hart coefficient
  float offset = 273.15f;   // Kelvin -> Celsius
  float adc_max = 1023.0f;  // 10-bit: 1023, 12-bit: 4095
};

// An open sensor (ADC at max) or a shorted one (ADC near 0) converts to far
// outside this window.
inline constexpr float kNtcMinValidCelsius = -40.0f;
inline constexpr float kNtcMaxValidCelsius = 150.0f;

// Divider Vcc -> R_pullup -> ADC pin -> NTC -> GND, then Steinhart-Hart
// 1/T = c1 + c2*ln(R) + c3*ln(R)^3. NAN for an open, shorted or
// out-of-range sensor.
inline float ntcAdcToCelsius(const NtcConfig& cfg, float adc) {
  if (adc >= cfg.adc_max) return NAN;
  const float resistance = cfg.pullup_resistance * adc / (cfg.adc_max - adc);
  const float log_r = logf(resistance);
  const float inv_t = cfg.c1 + cfg.c2 * log_r + cfg.c3 * log_r * log_r * log_r;
  const float celsius = (1.0f / inv_t) - cfg.offset;
  if (!(celsius >= kNtcMinValidCelsius && celsius <= kNtcMaxValidCelsius)) {
    return NAN;
  }
  return celsius;
}
