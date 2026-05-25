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

#include "battery_adc.hpp"
#include "communication_manager.hpp"
#include "hardware_encoder.hpp"
#include "imu_bno055.hpp"
#include "led_indicator.hpp"
#include "motor_array.hpp"
#include "motor_hi_z.hpp"
#include "pid.hpp"
#include "range_vl53l0.hpp"

// ───────── Arduino settings ─────────
inline constexpr uint32_t ADC_MAX_VALUE =
    1023;  // 10-bit ADC resolution (0-1023)

// ───────── Battery ─────────
inline constexpr BatteryAdcConfig battery_adc_config = {
    .adc_pin = PA5,
    .v_ref = 3.3f,
    .v_min = 9.6f,
    .v_max = 12.6f,
    .divider = (5.6e4 + 1.0e4) / 1.0e4,
    .correction = 0.986f,
    .adc_resolution_scale = 1.0f / ADC_MAX_VALUE,
};

// ───────── Buttons ─────────
inline constexpr uint8_t PUSH_BUTTON1 = PG12;
inline constexpr uint8_t PUSH_BUTTON2 = PG13;

// ───────── Encoders ─────────
inline constexpr float GEAR_RATIO = 34.0f;
inline constexpr uint16_t ENCODER_CPR = 48;
inline constexpr float TICKS_PER_REVOLUTION = ENCODER_CPR * GEAR_RATIO;
inline constexpr float RAD_PER_TICK = (2.0f * PI) / TICKS_PER_REVOLUTION;
inline constexpr float LOW_PASS_ALPHA = 0.2f;

inline constexpr HardwareEncoderConfig enc_fl_config = {
    .pin_a = PB6,
    .pin_b = PB7,
    .timer = TIM4,
    .inv_dir = false,
    .rad_per_tick = RAD_PER_TICK,
    .frame_id = "fl_wheel_joint",
    .alpha = LOW_PASS_ALPHA,
};

inline constexpr HardwareEncoderConfig enc_fr_config = {
    .pin_a = PA0,
    .pin_b = PA1,
    .timer = TIM2,
    .inv_dir = true,
    .rad_per_tick = RAD_PER_TICK,
    .frame_id = "fr_wheel_joint",
    .alpha = LOW_PASS_ALPHA,
};

inline constexpr HardwareEncoderConfig enc_rl_config = {
    .pin_a = PB4,
    .pin_b = PA7,
    .timer = TIM3,
    .inv_dir = false,
    .rad_per_tick = RAD_PER_TICK,
    .frame_id = "rl_wheel_joint",
    .alpha = LOW_PASS_ALPHA,
};

inline constexpr HardwareEncoderConfig enc_rr_config = {
    .pin_a = PC6,
    .pin_b = PC7,
    .timer = TIM8,
    .inv_dir = true,
    .rad_per_tick = RAD_PER_TICK,
    .frame_id = "rr_wheel_joint",
    .alpha = LOW_PASS_ALPHA,
};

// ───────── IMU ─────────
inline constexpr uint8_t IMU_POWER_ON = PG4;
inline constexpr uint8_t IMU_I2C_SDA = PC9;
inline constexpr uint8_t IMU_I2C_SCL = PA8;

inline TwoWire imu_i2c(IMU_I2C_SDA, IMU_I2C_SCL);
inline constexpr ImuBno055Config imu_bno055_config = {
    .bus = &imu_i2c,
    .i2c_addr = 0x29,
    .sensor_id = 0xA0,
    .int_pin = PA6,
    .axis_config = Adafruit_BNO055::REMAP_CONFIG_P0,
    .axis_sign = Adafruit_BNO055::REMAP_SIGN_P3,
};

// ───────── LEDs ─────────
inline constexpr uint8_t RED_LED = PE2;
inline constexpr uint8_t GRN_LED = PE3;
inline constexpr uint8_t GRN_LED2 = PE4;

inline constexpr LedIndicatorConfig led_status_config = {
    .pin = RED_LED,
    .initial_state = HIGH,
    .blink_period_ms = 500,
    .label = "STATUS",
};

// ───────── Motors ─────────
inline constexpr uint32_t MOTOR_PWM_FREQ = 20000;  // 20 kHz
inline constexpr float MAX_VELOCITY = 30.0f;
inline constexpr float MIN_VELOCITY = 0.0f;

// Gear-motor spec sheet: 12 V, 285 RPM no-load, 275 mA no-load, 4.3 kg·cm
// stall, 34:1 gearbox. Derived (assuming η ≈ 0.75):
//   I_stall ≈ 1.67 A,  R ≈ 7.2 Ω,  Ke ≈ 0.00989 V·s/rad
//   Kt_total = Ke × N × η ≈ 0.25 Nm/A  (sanity: 0.25 × 1.67 ≈ 0.42 Nm)
// DRV8848 has no current sense, so effort comes from the back-EMF model.
inline constexpr float MOTOR_TORQUE_CONSTANT = 0.25f;  // [Nm/A]
inline constexpr float MOTOR_RESISTANCE = 7.2f;        // [Ω]
inline constexpr float MOTOR_BACK_EMF = 0.008f;        // [V·s/rad]
inline constexpr float MOTOR_SUPPLY_VOLTAGE = 12.0f;   // [V] nominal

inline constexpr DriverGroupConfig right_motors_driver = {PC13, PE0};
inline constexpr DriverGroupConfig left_motors_driver = {PC14, PE1};
inline constexpr DriverGroupConfig driver_groups[] = {
    right_motors_driver,
    left_motors_driver,
};

inline constexpr MotorHiZConfig motor_fl_config = {
    .pwm_pin = PF9,
    .in_a_pin = PE5,
    .in_b_pin = PE6,
    .inv_dir = false,
    .max_velocity = MAX_VELOCITY,
    .min_velocity = MIN_VELOCITY,
    .pwm_freq = MOTOR_PWM_FREQ,
    .frame_id = "fl_wheel_joint",
    .torque_constant = MOTOR_TORQUE_CONSTANT,
    .gear_ratio = GEAR_RATIO,
    .winding_resistance = MOTOR_RESISTANCE,
    .back_emf_constant = MOTOR_BACK_EMF,
    .supply_voltage = MOTOR_SUPPLY_VOLTAGE,
};

inline constexpr MotorHiZConfig motor_fr_config = {
    .pwm_pin = PF6,
    .in_a_pin = PG10,
    .in_b_pin = PG11,
    .inv_dir = true,
    .max_velocity = MAX_VELOCITY,
    .min_velocity = MIN_VELOCITY,
    .pwm_freq = MOTOR_PWM_FREQ,
    .frame_id = "fr_wheel_joint",
    .torque_constant = MOTOR_TORQUE_CONSTANT,
    .gear_ratio = GEAR_RATIO,
    .winding_resistance = MOTOR_RESISTANCE,
    .back_emf_constant = MOTOR_BACK_EMF,
    .supply_voltage = MOTOR_SUPPLY_VOLTAGE,
};

inline constexpr MotorHiZConfig motor_rl_config = {
    .pwm_pin = PF8,
    .in_a_pin = PC15,
    .in_b_pin = PF2,
    .inv_dir = false,
    .max_velocity = MAX_VELOCITY,
    .min_velocity = MIN_VELOCITY,
    .pwm_freq = MOTOR_PWM_FREQ,
    .frame_id = "rl_wheel_joint",
    .torque_constant = MOTOR_TORQUE_CONSTANT,
    .gear_ratio = GEAR_RATIO,
    .winding_resistance = MOTOR_RESISTANCE,
    .back_emf_constant = MOTOR_BACK_EMF,
    .supply_voltage = MOTOR_SUPPLY_VOLTAGE,
};

inline constexpr MotorHiZConfig motor_rr_config = {
    .pwm_pin = PF7,
    .in_a_pin = PD3,
    .in_b_pin = PD4,
    .inv_dir = true,
    .max_velocity = MAX_VELOCITY,
    .min_velocity = MIN_VELOCITY,
    .pwm_freq = MOTOR_PWM_FREQ,
    .frame_id = "rr_wheel_joint",
    .torque_constant = MOTOR_TORQUE_CONSTANT,
    .gear_ratio = GEAR_RATIO,
    .winding_resistance = MOTOR_RESISTANCE,
    .back_emf_constant = MOTOR_BACK_EMF,
    .supply_voltage = MOTOR_SUPPLY_VOLTAGE,
};

// ───────── PID ─────────
// PID configuration is the same for all motors
inline constexpr PIDConfig pid_config = {
    .kp = 0.3f,
    .ki = 0.4f,
    .kd = 0.0f,
    .kv = 0.03f,  // velocity feedforward (slightly below 1/MAX_VELOCITY=0.04)
    .min_output = -1.0f,
    .max_output = 1.0f,
    .max_brake_output = 0.3f,   // allow up to 30% reverse PWM for braking
    .min_power_to_move = 0.3f,  // boost low outputs to overcome motor dead zone
    .compensation_up_to_speed =
        2.0f,  // boost decays linearly to 0 at this speed [rad/s]
};

// ───────── Ranges ─────────
inline constexpr uint8_t RANGE_I2C_SDA = PB9;
inline constexpr uint8_t RANGE_I2C_SCL = PB8;

inline TwoWire range_i2c(RANGE_I2C_SDA, RANGE_I2C_SCL);
inline constexpr RangeVl53l0xConfig range_fl_config = {
    .bus = &range_i2c,
    .xshut_pin = PD8,
    .i2c_address = 0x30,
    .frame_id = "fl_range",
};

inline constexpr RangeVl53l0xConfig range_fr_config = {
    .bus = &range_i2c,
    .xshut_pin = PB1,
    .i2c_address = 0x31,
    .frame_id = "fr_range",
};

inline constexpr RangeVl53l0xConfig range_rl_config = {
    .bus = &range_i2c,
    .xshut_pin = PD10,
    .i2c_address = 0x32,
    .frame_id = "rl_range",
};

inline constexpr RangeVl53l0xConfig range_rr_config = {
    .bus = &range_i2c,
    .xshut_pin = PD9,
    .i2c_address = 0x33,
    .frame_id = "rr_range",
};

// ───────── ROS ─────────
inline constexpr const char* NODE_NAME = "rosbot_mcu";
inline constexpr uint16_t DOMAIN_ID = 255;  // 255 inherit from Micro ROS Agent
inline constexpr uint32_t SPIN_TIME_MS = 5;
inline constexpr uint32_t TIMER_MS = 10;
inline constexpr uint16_t PING_WATCHDOG_MS = 200;
inline constexpr uint16_t PING_TIMEOUT_MS = 100;
inline constexpr uint8_t PING_ATTEMPTS = 1;

// ───────── Publishers ─────────
inline QueueHandle_t battery_queue;
inline QueueHandle_t imu_queue;
inline QueueHandle_t joint_state_queue;
inline QueueHandle_t ranges_queue;

inline constexpr uint8_t BATTERY_NUM_CELLS = 3;
inline constexpr float BATTERY_CELL_CAPACITY = 2.6f;  // Ah
inline constexpr float BATTERY_DESIGN_CAPACITY =
    BATTERY_NUM_CELLS * BATTERY_CELL_CAPACITY;
inline constexpr uint8_t buttons_pins[2] = {PUSH_BUTTON2, PUSH_BUTTON1};

// Publisher configs live in src/rosbot/ros.cpp — keeping lib/ros headers
// out of here prevents ODR collisions with lib/mavlink's identically-
// named queue payload structs.

// ───────── SBC Interface ─────────
inline constexpr uint32_t SBC_SERIAL_TIMEOUT_MS = 100;
inline constexpr uint8_t SBC_STATUS = PG6;  // Detect RPi which is a pullup pin
inline constexpr uint8_t RPI_CONSOLE = PG5;
inline constexpr uint8_t RPI_BTN = PG7;

// Primary: SBC Serial (SBC connection)
inline constexpr SerialConfig SBC_SERIAL_CONFIG = {.serial = &Serial1,
                                                   .baudrate = 921600,
                                                   .rxPin = PA10,
                                                   .txPin = PA9,
                                                   .timeout_ms = 1,
                                                   .name = "SBC_SERIAL"};

// Secondary: FTDI Serial (Rear panel USB connection)
inline constexpr SerialConfig DIAGNOSTIC_SERIAL_CONFIG = {
    .serial = &Serial3,
    .baudrate = 921600,
    .rxPin = PB11,
    .txPin = PB10,
    .timeout_ms = 1,
    .name = "FTDI_SERIAL"};
