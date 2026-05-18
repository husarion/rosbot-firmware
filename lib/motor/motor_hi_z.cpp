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

#include "motor_hi_z.hpp"

// 10-bit ADC, Vref = 3.3V. Matches ADC_MAX_VALUE used elsewhere on the
// board (e.g. battery_adc).
static constexpr float kAdcVref = 3.3f;
static constexpr float kAdcCounts = 1023.0f;
static constexpr float kAdcToVolt = kAdcVref / kAdcCounts;

void MotorHiZ::init() {
  setMode(Mode::Neutral);

  // Motor owns its encoder's lifecycle: init it here so the control task
  // doesn't need to coordinate a separate g_encoders.init() pass.
  if (encoder_) encoder_->init();

  PinName pwm_pn = digitalPinToPinName(cfg_.pwm_pin);
  TIM_TypeDef* tim = (TIM_TypeDef*)pinmap_peripheral(pwm_pn, PinMap_PWM);
  pwm_channel_ = STM_PIN_CHANNEL(pinmap_function(pwm_pn, PinMap_PWM));

  pwm_timer_ = new HardwareTimer(tim);
  pwm_timer_->setPWM(pwm_channel_, pwm_pn, cfg_.pwm_freq, 0);
  pwm_arr_ = pwm_timer_->getOverflow(TICK_FORMAT);

  if (cfg_.current_sense_pin != 0xFF) {
    pinMode(cfg_.current_sense_pin, INPUT_ANALOG);
  }
}

MotorData MotorHiZ::getData() const {
  MotorData d;
  if (encoder_) {
    const auto enc = encoder_->getData();
    d.position = enc.position;
    d.velocity = enc.velocity;
  }
  d.effort = current_effort_.load(std::memory_order_relaxed);
  d.target_velocity = target_velocity_.load(std::memory_order_relaxed);
  return d;
}

void MotorHiZ::setMode(Mode movement) {
  if (movement == current_mode_) return;
  current_mode_ = movement;

  uint8_t pin_a = cfg_.inv_dir ? cfg_.in_b_pin : cfg_.in_a_pin;
  uint8_t pin_b = cfg_.inv_dir ? cfg_.in_a_pin : cfg_.in_b_pin;

  switch (movement) {
    case Mode::Forward:
      pinMode(pin_a, INPUT);  // Hi-Z → receives PWM
      pinMode(pin_b, OUTPUT);
      digitalWrite(pin_b, LOW);
      break;

    case Mode::Reverse:
      pinMode(pin_a, OUTPUT);
      digitalWrite(pin_a, LOW);
      pinMode(pin_b, INPUT);  // Hi-Z → receives PWM
      break;

    case Mode::Brake:
      pinMode(cfg_.in_a_pin, OUTPUT);
      pinMode(cfg_.in_b_pin, OUTPUT);
      digitalWrite(cfg_.in_a_pin, HIGH);
      digitalWrite(cfg_.in_b_pin, HIGH);
      pwm_timer_->setCaptureCompare(pwm_channel_, pwm_arr_);
      break;

    case Mode::Neutral:
    default:
      pinMode(cfg_.in_a_pin, OUTPUT);
      pinMode(cfg_.in_b_pin, OUTPUT);
      digitalWrite(cfg_.in_a_pin, LOW);
      digitalWrite(cfg_.in_b_pin, LOW);
      pwm_timer_->setCaptureCompare(pwm_channel_, 0);
      break;
  }
}

void MotorHiZ::applyPWM(float duty) {
  duty = constrain(duty, -1.0f, 1.0f);
  // Default effort = commanded duty. Overridden below if a current sensor
  // or a back-EMF model is available (then it becomes a torque value).
  current_effort_.store(duty, std::memory_order_relaxed);

  if (fabs(duty) < 0.01f) {
    setMode(Mode::Brake);
    return;
  }

  setMode(duty > 0 ? Mode::Forward : Mode::Reverse);

  uint16_t pwm_value = static_cast<uint16_t>(fabs(duty) * pwm_arr_);
  pwm_timer_->setCaptureCompare(pwm_channel_, pwm_value);

  const bool has_sensor = !current_sense_disabled_ &&
                          cfg_.current_sense_pin != 0xFF &&
                          cfg_.current_per_volt > 0.0f;
  if (has_sensor) {
    sampleCurrent();
  } else {
    estimateCurrent(duty);
  }
}

void MotorHiZ::sampleCurrent() {
  const uint16_t raw = analogRead(cfg_.current_sense_pin);
  float i = raw * kAdcToVolt * cfg_.current_per_volt;
  // ISEN output (e.g. MAX22205 CSO) is unipolar — sign comes from the
  // commanded direction.
  if (current_mode_ == Mode::Reverse) i = -i;
  applyCurrentSample(i);
}

void MotorHiZ::estimateCurrent(float duty) {
  if (cfg_.winding_resistance <= 0.0f) return;
  // Use live supply voltage when a provider is registered (battery sags
  // under load — assuming a fixed 12 V makes back-EMF estimate go negative
  // near no-load); else fall back to the static config value.
  const float v_supply = supply_v_fn_ ? supply_v_fn_() : cfg_.supply_voltage;
  if (v_supply <= 0.0f) return;
  // Steady-state DC motor model:  V = R·I + Ke·ω_motor  →
  //   I = (duty·V_supply − Ke·ω_motor) / R
  // Sign falls out of the equation: regen (PWM·V smaller than back-EMF)
  // gives negative I, which translates to negative effort.
  const float omega_output = encoder_ ? encoder_->getData().velocity : 0.0f;
  const float omega_motor = omega_output * cfg_.gear_ratio;
  const float v_motor = duty * v_supply;
  const float i = (v_motor - cfg_.back_emf_constant * omega_motor) /
                  cfg_.winding_resistance;
  applyCurrentSample(i);
}

void MotorHiZ::applyCurrentSample(float i_motor) {
  const float a = cfg_.current_filter_alpha;
  current_filtered_ = a * i_motor + (1.0f - a) * current_filtered_;
  current_effort_.store(current_filtered_ * cfg_.torque_constant,
                        std::memory_order_relaxed);
}

void MotorHiZ::setVelocity(float vel) {
  float v = constrain(vel, -cfg_.max_velocity, cfg_.max_velocity);
  if (fabs(v) < cfg_.min_velocity) v = 0.0f;
  target_velocity_.store(v, std::memory_order_relaxed);
}

void MotorHiZ::setNeutral() {
  target_velocity_.store(0.0f, std::memory_order_relaxed);
  current_filtered_ = 0.0f;
  setMode(Mode::Neutral);
  pid_.reset();
}

void MotorHiZ::brake() {
  target_velocity_.store(0.0f, std::memory_order_relaxed);
  current_effort_.store(0.0f, std::memory_order_relaxed);
  current_filtered_ = 0.0f;
  setMode(Mode::Brake);
  pid_.reset();
}

void MotorHiZ::update(float dt, bool move) {
  // Sample the encoder right before PID so the control loop always sees
  // the freshest reading and a fixed-phase dt.
  if (encoder_) encoder_->update();

  if (!move) {
    brake();
    return;
  }

  const float target = target_velocity_.load(std::memory_order_relaxed);
  const float current = encoder_ ? encoder_->getData().velocity : 0.0f;
  const float output = pid_.compute(target, current, dt);

  applyPWM(output);
}

void MotorHiZ::reset() {
  brake();
  target_velocity_.store(0.0f, std::memory_order_relaxed);
  current_effort_.store(0.0f, std::memory_order_relaxed);
  pid_.reset();
  if (encoder_) encoder_->reset();
}
