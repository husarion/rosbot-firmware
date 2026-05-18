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
#include <HardwareTimer.h>

#include <atomic>

#include "encoder_interface.hpp"
#include "motor_interface.hpp"
#include "pid.hpp"

struct MotorHiZConfig {
  uint8_t pwm_pin;     // PWM output
  uint8_t in_a_pin;    // direction input A
  uint8_t in_b_pin;    // direction input B
  bool inv_dir;        // polarity inversion
  float max_velocity;  // [rad/s] clamp
  float min_velocity;  // [rad/s] deadband
  uint32_t pwm_freq;   // [Hz]
  const char* frame_id;
  uint8_t current_sense_pin = 0xFF;   // 0xFF = no analog current sensor
  float current_per_volt = 0.0f;      // [A/V] — I_motor = V_isen * this
  float current_filter_alpha = 0.2f;  // EMA on current: 1 = raw, 0 = frozen
  float torque_constant = 1.0f;       // Output-shaft torque per ampere of motor
                                      // current (Kt_motor × N × η).
  // Back-EMF model: I = (duty * supply_voltage - back_emf_constant *
  // omega_motor) / winding_resistance, used as a fallback when no analog
  // current sensor is available. Set winding_resistance to 0 to disable.
  float gear_ratio = 1.0f;          // [motor:output reduction]
  float winding_resistance = 0.0f;  // R [Ω], 0 = disable estimator
  float back_emf_constant = 0.0f;   // Ke [V·s/rad] motor shaft
  float supply_voltage = 0.0f;      // [V] nominal supply
};

/// DRV8848-based motor with Hi-Z control scheme.
/// PWM applied to nSLEEP/EN, direction via IN_A / IN_B pins.
class MotorHiZ : public MotorInterface {
 public:
  MotorHiZ() = default;
  explicit MotorHiZ(const MotorHiZConfig& cfg, EncoderInterface* encoder,
                    PIDController pid)
      : cfg_(cfg), encoder_(encoder), pid_(pid) {}

  void init() override;
  void update(float dt, bool move = true) override;
  void reset() override;
  void setVelocity(float vel) override;
  void brake() override;
  void setNeutral() override;

  MotorData getData() const override;
  const char* name() const override { return cfg_.frame_id; }

  // Disable the analog current sensor at runtime
  // (e.g. some board revisions lack the MAX22205 CSO).
  // After this is called, effort falls back to commanded PWM duty.
  void disableCurrentSensor() { current_sense_disabled_ = true; }

  /// Source of dynamic supply voltage for the back-EMF estimator. When
  /// unset (nullptr) the estimator falls back to cfg_.supply_voltage.
  /// Free function pointer — no std::function (avoids heap on MCU).
  void setSupplyVoltageProvider(float (*fn)()) { supply_v_fn_ = fn; }

 private:
  enum class Mode : uint8_t { Forward, Reverse, Brake, Neutral };

  void setMode(Mode mode);
  void applyPWM(float duty);
  void sampleCurrent();
  void estimateCurrent(float duty);
  void applyCurrentSample(float i_motor);

  MotorHiZConfig cfg_ = {};
  EncoderInterface* encoder_ = nullptr;
  PIDController pid_;
  HardwareTimer* pwm_timer_ = nullptr;
  uint32_t pwm_channel_ = 0;
  uint16_t pwm_arr_ = 0;

  std::atomic<float> target_velocity_{0.0f};
  std::atomic<float> current_effort_{0.0f};
  Mode current_mode_ = Mode::Neutral;
  bool current_sense_disabled_ = false;
  float current_filtered_ = 0.0f;  // EMA state for I_motor [A]
  float (*supply_v_fn_)() = nullptr;
};
