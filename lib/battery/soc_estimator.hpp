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
#include <cstddef>
#include <cstdint>

// Rest voltage per cell (ascending) against SoC, linearly interpolated.
struct OcvCurve {
  const float* cell_v;
  const float* soc;
  size_t n;
};

namespace ocv_curves {

// Generic 18650 Li-ion curve; 3.27 V is 0 %, in line with a 3.2 V/cell
// (9.6 V) cut-off. Not measured and too low between ~20 and 70 %; both
// robots use kRosbotXl.
inline constexpr float kGenericV[] = {
    3.27f, 3.61f, 3.69f, 3.71f, 3.73f, 3.75f, 3.77f, 3.79f, 3.80f, 3.82f, 3.84f,
    3.85f, 3.87f, 3.91f, 3.95f, 3.98f, 4.02f, 4.08f, 4.11f, 4.15f, 4.20f};
inline constexpr float kGenericSoc[] = {
    0.00f, 0.05f, 0.10f, 0.15f, 0.20f, 0.25f, 0.30f, 0.35f, 0.40f, 0.45f, 0.50f,
    0.55f, 0.60f, 0.65f, 0.70f, 0.75f, 0.80f, 0.85f, 0.90f, 0.95f, 1.00f};
inline constexpr OcvCurve kGeneric{kGenericV, kGenericSoc,
                                   sizeof(kGenericV) / sizeof(kGenericV[0])};

// Based on real measurements: ROSbot XL pack HCP18650ZC-3S3P, bench robot,
// 2026-11-19, ~24 C. 10 % discharge steps at 1.8-2.9 A, 15-60 min rests;
// SoC counted back from the 9.6 V loaded cut-off (9.034 Ah). VBAT from the
// BQ25713 (64 mV/pack step = 21 mV/cell). Data and method:
// rosbot_xl_pb_firmware docs/battery-measurements.md.
inline constexpr float kRosbotXlV[] = {3.328f, 3.413f, 3.477f, 3.563f,
                                       3.605f, 3.627f, 3.691f, 3.797f,
                                       3.883f, 3.989f, 4.096f, 4.200f};
inline constexpr float kRosbotXlSoc[] = {0.000f, 0.018f, 0.116f, 0.214f,
                                         0.312f, 0.411f, 0.509f, 0.607f,
                                         0.705f, 0.803f, 0.902f, 1.000f};
inline constexpr OcvCurve kRosbotXl{kRosbotXlV, kRosbotXlSoc,
                                    sizeof(kRosbotXlV) / sizeof(kRosbotXlV[0])};

}  // namespace ocv_curves

struct SocEstimatorConfig {
  uint8_t num_cells = 3;  // series count; parallel cells only add capacity
  float capacity_ah = 7.8f;
  const OcvCurve* ocv = &ocv_curves::kGeneric;
  // Pack resistance used to undo load sag. ROSbot XL measured ~0.105 ohm incl.
  // the PB path (2026-11-18/19); the OCV correction is weighted down under
  // load, so an error here only slows convergence.
  float r_internal_ohm = 0.10f;
  float correction_tau_s = 60.0f;
  float load_ref_a = 2.0f;
  float voltage_only_tau_s = 30.0f;
};

class SocEstimator {
 public:
  explicit SocEstimator(const SocEstimatorConfig& cfg) : cfg_(cfg) {}

  // current_a > 0 charges the pack. Returns SoC in [0, 1].
  float update(float voltage_v, float current_a, float dt_s) {
    if (!std::isfinite(voltage_v) || !std::isfinite(current_a) ||
        cfg_.num_cells == 0) {
      return soc_;
    }

    const float ocv = voltage_v - current_a * cfg_.r_internal_ohm;
    const float soc_ocv =
        ocvToSoc(ocv / static_cast<float>(cfg_.num_cells), *cfg_.ocv);

    if (!initialized_) {
      soc_ = soc_ocv;
      initialized_ = true;
      return soc_;
    }

    // A stalled or wrapped timestamp must not integrate a bogus charge.
    if (!(dt_s > 0.0f) || dt_s > kMaxDtS) return soc_;

    soc_ += current_a * dt_s / (3600.0f * cfg_.capacity_ah);

    const float load = current_a / cfg_.load_ref_a;
    const float trust = 1.0f / (1.0f + load * load);
    soc_ += (soc_ocv - soc_) * trust * dt_s / cfg_.correction_tau_s;

    soc_ = fminf(fmaxf(soc_, 0.0f), 1.0f);
    return soc_;
  }

  // No current measurement (ROSbot 3): rest-voltage curve through a low-pass
  // filter. Load sag still biases the result low while driving; the filter
  // only keeps it from flickering around the low-battery threshold.
  float updateVoltageOnly(float voltage_v, float dt_s) {
    if (!std::isfinite(voltage_v) || cfg_.num_cells == 0) return soc_;

    const float soc_ocv =
        ocvToSoc(voltage_v / static_cast<float>(cfg_.num_cells), *cfg_.ocv);
    if (!initialized_) {
      soc_ = soc_ocv;
      initialized_ = true;
      return soc_;
    }

    if (!(dt_s > 0.0f) || dt_s > kMaxDtS) return soc_;
    soc_ += (soc_ocv - soc_) * fminf(dt_s / cfg_.voltage_only_tau_s, 1.0f);
    return soc_;
  }

  float soc() const { return initialized_ ? soc_ : NAN; }
  void reset() { initialized_ = false; }

  static float ocvToSoc(float cell_v,
                        const OcvCurve& c = ocv_curves::kGeneric) {
    if (cell_v <= c.cell_v[0]) return c.soc[0];
    if (cell_v >= c.cell_v[c.n - 1]) return c.soc[c.n - 1];
    size_t i = 1;
    while (cell_v > c.cell_v[i]) ++i;
    const float t =
        (cell_v - c.cell_v[i - 1]) / (c.cell_v[i] - c.cell_v[i - 1]);
    return c.soc[i - 1] + t * (c.soc[i] - c.soc[i - 1]);
  }

 private:
  static constexpr float kMaxDtS = 5.0f;

  SocEstimatorConfig cfg_;
  float soc_ = NAN;
  bool initialized_ = false;
};
