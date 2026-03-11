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

#include "hardware_encoder.hpp"
#include "imu_bno055.hpp"
#include "led_indicator.hpp"
#include "led_strip.hpp"
#include "motor_array.hpp"
#include "motor_drv8848.hpp"
#include "pid.hpp"
#include "ros/publishers/battery_publisher.hpp"
#include "ros/publishers/buttons_publisher.hpp"
#include "ros/publishers/imu_publisher.hpp"
#include "ros/publishers/joint_state_publisher.hpp"
#include "serial_manager.hpp"
#include "transport/spi_transport.hpp"

// ────────────── Board ──────────────
static constexpr uint8_t AUDIO_SHDN = PB2;
static constexpr uint8_t AUDIO_DAC_OUT = PA4;
static constexpr uint8_t EN_LOC_5V = PF13;

// ────────────── Buttons ──────────────
static constexpr uint8_t PUSH_BUTTON1 = PF11;
static constexpr uint8_t PUSH_BUTTON2 = PF12; // MCU reset button

// ────────────── Encoders ──────────────
constexpr float GEAR_RATIO = 50.0f;
constexpr uint16_t ENCODER_CPR = 64;
constexpr float TICKS_PER_REVOLUTION = ENCODER_CPR * GEAR_RATIO;
constexpr float RAD_PER_TICK = (2.0f * PI) / TICKS_PER_REVOLUTION;
constexpr float LOW_PASS_ALPHA = 0.05f;

inline constexpr HardwareEncoderConfig enc_fl_config = {
    .pin_a = PD12,
    .pin_b = PD13,
    .timer = TIM4,
    .inv_dir = true,
    .rad_per_tick = RAD_PER_TICK,
    .frame_id = "fl_wheel_joint",
    .alpha = LOW_PASS_ALPHA,
};

inline constexpr HardwareEncoderConfig enc_fr_config = {
    .pin_a = PC6,
    .pin_b = PC7,
    .timer = TIM3,
    .inv_dir = false,
    .rad_per_tick = RAD_PER_TICK,
    .frame_id = "fr_wheel_joint",
    .alpha = LOW_PASS_ALPHA,
};

inline constexpr HardwareEncoderConfig enc_rl_config = {
    .pin_a = PA15,
    .pin_b = PB3,
    .timer = TIM2,
    .inv_dir = true,
    .rad_per_tick = RAD_PER_TICK,
    .frame_id = "rl_wheel_joint",
    .alpha = LOW_PASS_ALPHA,
};

inline constexpr HardwareEncoderConfig enc_rr_config = {
    .pin_a = PE9,
    .pin_b = PE11,
    .timer = TIM1,
    .inv_dir = false,
    .rad_per_tick = RAD_PER_TICK,
    .frame_id = "rr_wheel_joint",
    .alpha = LOW_PASS_ALPHA,
};

// ────────────── Fan ──────────────
#define FAN_PP_PIN PC13
#define FAN_PWM_PIN PB_0_ALT1
#define FAN_PWM_TIMER TIM3
#define FAN_PWM_CHANNEL 3
#define FAN_PWM_FREQUENCY 1000
#define FAN_TEMP_THRSH_UP 35
#define FAN_TEMP_THRSH_DOWN 30

// ────────────── IMU ──────────────
static constexpr uint8_t IMU_I2C_SDA = PF0;
static constexpr uint8_t IMU_I2C_SCL = PF1;

inline TwoWire imu_i2c(IMU_I2C_SDA, IMU_I2C_SCL);
inline constexpr ImuBno055Config imu_bno055_config = {
    .bus = &imu_i2c,
    .i2c_addr = 0x29,
    .sensor_id = 0x37,
    .int_pin = PF2,
    .axis_config = Adafruit_BNO055::REMAP_CONFIG_P1,
    .axis_sign = Adafruit_BNO055::REMAP_SIGN_P0,
};

// ────────────── LEDs ──────────────
static constexpr uint8_t RED_LED = PE4;
static constexpr uint8_t GRN_LED = PE3;

inline constexpr LedIndicatorConfig led_status_config = {
    .pin = RED_LED,
    .initial_state = HIGH,
    .blink_period_ms = 500,
    .label = "STATUS",
};

// ────────────── LED Strip ──────────────
static constexpr uint16_t IDLE_ANIMATION_CHANGE_MS = 2000;
static constexpr uint16_t IDLE_ANIMATION_INTERVAL_MS = 60;
static constexpr uint16_t LED_STRIP_TIMEOUT_MS = 1000;

inline constexpr SpiTransportConfig spi_config = {
    .mosi_pin  = PB15,
    .miso_pin  = PB14,
    .sck_pin   = PB10,
    .spi_speed = 4000000,
    .bit_order = MSBFIRST,
    .spi_mode  = SPI_MODE3,
};

inline constexpr SwapPair swaps[] = {
    {13, 17},
    {14, 16},
};
inline constexpr LedStripConfig strip_config = {
    .num_leds     = 18,
    .swaps             = swaps,
    .swap_count        = sizeof(swaps) / sizeof(swaps[0]),
    .init_r            = 0x0F,
    .init_g            = 0x00,
    .init_b            = 0x00,
};

// ────────────── Motors ──────────────
constexpr uint32_t MOTOR_PWM_FREQ = 20000;  // 20 kHz
constexpr float MAX_VELOCITY = 22.0f;
constexpr float MIN_VELOCITY = 0.5f;

inline constexpr DriverGroupConfig right_motors_driver = {PC13, PE0};
inline constexpr DriverGroupConfig left_motors_driver = {PC14, PE1};
inline constexpr DriverGroupConfig driver_groups[] = {
    right_motors_driver,
    left_motors_driver,
};

inline constexpr MotorDrv8848Config motor_fl_config = {
    .pwm_pin = PF9,
    .in_a_pin = PD10,
    .in_b_pin = PD11,
    .inv_dir = true,
    .max_velocity = MAX_VELOCITY,
    .min_velocity = MIN_VELOCITY,
    .pwm_freq = MOTOR_PWM_FREQ,
    .frame_id = "fl_wheel_joint",
};

inline constexpr MotorDrv8848Config motor_fr_config = {
    .pwm_pin = PF8,
    .in_a_pin = PG5,
    .in_b_pin = PG6,
    .inv_dir = false,
    .max_velocity = MAX_VELOCITY,
    .min_velocity = MIN_VELOCITY,
    .pwm_freq = MOTOR_PWM_FREQ,
    .frame_id = "fr_wheel_joint",
};

inline constexpr MotorDrv8848Config motor_rl_config = {
    .pwm_pin = PF7,
    .in_a_pin = PG11,
    .in_b_pin = PG12,
    .inv_dir = true,
    .max_velocity = MAX_VELOCITY,
    .min_velocity = MIN_VELOCITY,
    .pwm_freq = MOTOR_PWM_FREQ,
    .frame_id = "rl_wheel_joint",
};

inline constexpr MotorDrv8848Config motor_rr_config = {
    .pwm_pin = PF6,
    .in_a_pin = PE12,
    .in_b_pin = PE13,
    .inv_dir = false,
    .max_velocity = MAX_VELOCITY,
    .min_velocity = MIN_VELOCITY,
    .pwm_freq = MOTOR_PWM_FREQ,
    .frame_id = "rr_wheel_joint",
};

// ────────────── PID ──────────────
// PID configuration is the same for all motors
inline constexpr PIDConfig pid_config = {
    .kp = 0.15f,
    .ki = 0.55f,
    .kd = 0.007f,
    .min_output = -1.0f,
    .max_output = 1.0f,
};

// ────────────── ROS ──────────────
static constexpr const char* NODE_NAME = "rosbot_mcu";
static constexpr uint16_t DOMAIN_ID = 255;  // 255 inherit from Micro ROS Agent
static constexpr uint32_t PING_TIMEOUT_MS = 100;
static constexpr uint8_t PING_ATTEMPTS = 3;

// ────────────── Publishers ──────────────
inline QueueHandle_t battery_queue;
inline QueueHandle_t imu_queue;
inline QueueHandle_t joint_state_queue;
inline QueueHandle_t led_strip_queue;

static constexpr uint8_t BATTERY_NUM_CELLS = 3;
static constexpr float BATTERY_CELL_CAPACITY = 2.6f;  // Ah
static constexpr float BATTERY_DESIGN_CAPACITY =
    BATTERY_NUM_CELLS * BATTERY_CELL_CAPACITY;
inline constexpr BatteryPublisherConfig battery_pub_config = {
    .topic = "battery",
    .queue = battery_queue,
    .frame_id = "base_link",
    .design_capacity = BATTERY_DESIGN_CAPACITY,
    .num_cells = BATTERY_NUM_CELLS,
};

inline constexpr uint8_t buttons_pins[] = {PUSH_BUTTON1};
inline constexpr ButtonsPublisherConfig buttons_pub_config = {
    .topic = "buttons",
    .pins = buttons_pins,
    .num_buttons = sizeof(buttons_pins) / sizeof(buttons_pins[0]),
};

inline constexpr ImuPublisherConfig imu_pub_config = {
    .topic = "_imu/data_raw",
    .queue = imu_queue,
    .frame_id = "imu_link",
};

inline constexpr JointStatePublisherConfig joint_state_pub_config = {
    .topic = "_motors_response",
    .queue = joint_state_queue,
    .frame_id = "base_link",
};

// ────────────── PowerBoard ──────────────
static constexpr uint8_t PWR_BRD_GPIO_INPUT = PD4;   // PB5 on power board -> output push pull
static constexpr uint8_t PWR_BRD_GPIO_OUTPUT = PD7;  // PB8 on power board -> input
static constexpr HardwareSerial& PWR_BRD_SERIAL = Serial2;
static constexpr uint32_t PWR_BRD_SERIAL_BAUDRATE = 38400;
static constexpr uint8_t PWR_BRD_SERIAL_RX = PD6;
static constexpr uint8_t PWR_BRD_SERIAL_TX = PD5;
static constexpr uint32_t PWR_BRD_SERIAL_CONFIG = 0x06;
static constexpr uint32_t PWR_BRD_SERIAL_TIMEOUT_MS = 10;

// ────────────── SBC Interface ──────────────
static constexpr uint32_t SBC_CONNECT_TIMEOUT_MS = 10;
static constexpr uint32_t POWEROFF_DELAY_MS = 5000;

inline constexpr SerialConfig FTDI_SERIAL_CONFIG = {.serial = &Serial1,
                                                    .baudrate = 921600,
                                                    .rxPin = PA10,
                                                    .txPin = PA9,
                                                    .timeout_ms = 1,
                                                    .name = "FTDI_SERIAL"};