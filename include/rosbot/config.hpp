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

#include "battery_adc.hpp"
#include "communication_manager.hpp"
#include "hardware_encoder.hpp"
#include "imu_bno055.hpp"
#include "led_indicator.hpp"
#include "motor_array.hpp"
#include "motor_hi_z.hpp"
#include "pid.hpp"
#include "pin.hpp"
#include "range_vl53l0.hpp"

using platform::AnalogIn;
using platform::Pin;
using platform::Port;

// Same value as Arduino's PI, so RAD_PER_TICK keeps its float bits.
inline constexpr double kPi = 3.1415926535897932384626433832795;

// ───────── Arduino settings ─────────
inline constexpr uint32_t ADC_MAX_VALUE =
    1023;  // 10-bit ADC resolution (0-1023)

// ───────── Battery ─────────
inline constexpr uint8_t BATTERY_NUM_CELLS = 3;
inline constexpr float BATTERY_CELL_CAPACITY = 2.6f;  // Ah
inline constexpr float BATTERY_DESIGN_CAPACITY =
    BATTERY_NUM_CELLS * BATTERY_CELL_CAPACITY;

inline constexpr BatteryAdcConfig battery_adc_config = {
    .adc = AnalogIn{Pin{Port::A, 5}, 5},
    .v_ref = 3.3f,
    .num_cells = BATTERY_NUM_CELLS,
    .divider = (5.6e4 + 1.0e4) / 1.0e4,
    .correction = 0.986f,
    .adc_resolution_scale = 1.0f / ADC_MAX_VALUE,
};

// ───────── Buttons ─────────
inline constexpr Pin PUSH_BUTTON1{Port::G, 12};
inline constexpr Pin PUSH_BUTTON2{Port::G, 13};

// ───────── Encoders ─────────
inline constexpr float GEAR_RATIO = 34.0f;
inline constexpr uint16_t ENCODER_CPR = 48;
inline constexpr float TICKS_PER_REVOLUTION = ENCODER_CPR * GEAR_RATIO;
inline constexpr float RAD_PER_TICK =
    static_cast<float>(2.0 * kPi / static_cast<double>(TICKS_PER_REVOLUTION));
inline constexpr float LOW_PASS_ALPHA = 0.2f;

inline constexpr HardwareEncoderConfig enc_fl_config = {
    .pin_a = Pin{Port::B, 6},
    .pin_b = Pin{Port::B, 7},
    .timer = TIM4,
    .inv_dir = false,
    .rad_per_tick = RAD_PER_TICK,
    .frame_id = "fl_wheel_joint",
    .alpha = LOW_PASS_ALPHA,
};

inline constexpr HardwareEncoderConfig enc_fr_config = {
    .pin_a = Pin{Port::A, 0},
    .pin_b = Pin{Port::A, 1},
    .timer = TIM2,
    .inv_dir = true,
    .rad_per_tick = RAD_PER_TICK,
    .frame_id = "fr_wheel_joint",
    .alpha = LOW_PASS_ALPHA,
};

inline constexpr HardwareEncoderConfig enc_rl_config = {
    .pin_a = Pin{Port::B, 4},
    .pin_b = Pin{Port::A, 7},
    .timer = TIM3,
    .inv_dir = false,
    .rad_per_tick = RAD_PER_TICK,
    .frame_id = "rl_wheel_joint",
    .alpha = LOW_PASS_ALPHA,
};

inline constexpr HardwareEncoderConfig enc_rr_config = {
    .pin_a = Pin{Port::C, 6},
    .pin_b = Pin{Port::C, 7},
    .timer = TIM8,
    .inv_dir = true,
    .rad_per_tick = RAD_PER_TICK,
    .frame_id = "rr_wheel_joint",
    .alpha = LOW_PASS_ALPHA,
};

// ───────── IMU ─────────
inline constexpr Pin IMU_POWER_ON{Port::G, 4};
// BNO055 COM3 (address select) is an MCU GPIO here with no pull resistor;
// drive HIGH → 0x29. rosbot_xl hard-wires this line to 3V3 instead.
inline constexpr Pin IMU_ADDR_SEL{Port::C, 8};
inline platform::I2c imu_i2c({.instance = I2C3,
                              .scl = Pin{Port::A, 8},
                              .sda = Pin{Port::C, 9},
                              .af = GPIO_AF4_I2C3});
inline constexpr ImuBno055Config imu_bno055_config = {
    .bus = &imu_i2c,
    .i2c_addr = 0x29,
    .int_pin = Pin{Port::A, 6},
    .axis_config = bno055::kRemapConfigP0,
    .axis_sign = bno055::kRemapSignP3,
};

// ───────── LEDs ─────────
inline constexpr Pin RED_LED{Port::E, 2};
inline constexpr Pin GRN_LED{Port::E, 3};
inline constexpr Pin GRN_LED2{Port::E, 4};
inline constexpr Pin green_leds[] = {GRN_LED, GRN_LED2};

inline constexpr LedIndicatorConfig led_status_config = {
    .pin = RED_LED,
    .initial_on = true,
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

inline constexpr DriverGroupConfig right_motors_driver = {Pin{Port::C, 13},
                                                          Pin{Port::E, 0}};
inline constexpr DriverGroupConfig left_motors_driver = {Pin{Port::C, 14},
                                                         Pin{Port::E, 1}};
inline constexpr DriverGroupConfig driver_groups[] = {
    right_motors_driver,
    left_motors_driver,
};

inline constexpr MotorHiZConfig motor_fl_config = {
    .pwm = {.pin = Pin{Port::F, 9},
            .timer = TIM14,
            .channel = 1,
            .af = GPIO_AF9_TIM14},
    .in_a_pin = Pin{Port::E, 5},
    .in_b_pin = Pin{Port::E, 6},
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
    .pwm = {.pin = Pin{Port::F, 6},
            .timer = TIM10,
            .channel = 1,
            .af = GPIO_AF3_TIM10},
    .in_a_pin = Pin{Port::G, 10},
    .in_b_pin = Pin{Port::G, 11},
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
    .pwm = {.pin = Pin{Port::F, 8},
            .timer = TIM13,
            .channel = 1,
            .af = GPIO_AF9_TIM13},
    .in_a_pin = Pin{Port::C, 15},
    .in_b_pin = Pin{Port::F, 2},
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
    .pwm = {.pin = Pin{Port::F, 7},
            .timer = TIM11,
            .channel = 1,
            .af = GPIO_AF3_TIM11},
    .in_a_pin = Pin{Port::D, 3},
    .in_b_pin = Pin{Port::D, 4},
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
inline platform::I2c range_i2c({.instance = I2C1,
                                .scl = Pin{Port::B, 8},
                                .sda = Pin{Port::B, 9},
                                .af = GPIO_AF4_I2C1});
inline constexpr RangeVl53l0xConfig range_fl_config = {
    .bus = &range_i2c,
    .xshut_pin = Pin{Port::D, 8},
    .i2c_address = 0x30,
    .frame_id = "fl_range",
};

inline constexpr RangeVl53l0xConfig range_fr_config = {
    .bus = &range_i2c,
    .xshut_pin = Pin{Port::B, 1},
    .i2c_address = 0x31,
    .frame_id = "fr_range",
};

inline constexpr RangeVl53l0xConfig range_rl_config = {
    .bus = &range_i2c,
    .xshut_pin = Pin{Port::D, 10},
    .i2c_address = 0x32,
    .frame_id = "rl_range",
};

inline constexpr RangeVl53l0xConfig range_rr_config = {
    .bus = &range_i2c,
    .xshut_pin = Pin{Port::D, 9},
    .i2c_address = 0x33,
    .frame_id = "rr_range",
};

// ───────── Publishers ─────────
inline QueueHandle_t battery_queue;
inline QueueHandle_t imu_queue;
inline QueueHandle_t joint_state_queue;
inline QueueHandle_t ranges_queue;

inline constexpr Pin buttons_pins[] = {PUSH_BUTTON2, PUSH_BUTTON1};

// Publisher configs live in src/rosbot/mavlink_entities.cpp.

// ───────── SBC Interface ─────────
inline constexpr uint32_t SBC_SERIAL_TIMEOUT_MS = 100;
inline constexpr Pin SBC_STATUS{Port::G,
                                6};  // Detect RPi which is a pullup pin
inline constexpr Pin RPI_CONSOLE{Port::G, 5};
inline constexpr Pin RPI_BTN{Port::G, 7};

// Primary: SBC Serial (SBC connection)
inline platform::Uart sbc_uart({.usart = USART1,
                                .tx = Pin{Port::A, 9},
                                .rx = Pin{Port::A, 10},
                                .af = GPIO_AF7_USART1,
                                .tx_queue_bytes = 2048});
inline constexpr SerialConfig SBC_SERIAL_CONFIG = {
    .uart = &sbc_uart, .baudrate = 921600, .name = "SBC_SERIAL"};

// Secondary: FTDI Serial (Rear panel USB connection). Can carry MAVLink, so
// it gets the same TX queue as the SBC link.
inline platform::Uart ftdi_uart({.usart = USART3,
                                 .tx = Pin{Port::B, 10},
                                 .rx = Pin{Port::B, 11},
                                 .af = GPIO_AF7_USART3,
                                 .tx_queue_bytes = 2048});
inline constexpr SerialConfig DIAGNOSTIC_SERIAL_CONFIG = {
    .uart = &ftdi_uart, .baudrate = 921600, .name = "FTDI_SERIAL"};
