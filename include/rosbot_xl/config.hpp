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
#include <STM32Ethernet.h>

#include "communication_manager.hpp"
#include "eeprom.hpp"
#include "fan.hpp"
#include "hardware_encoder.hpp"
#include "imu_bno055.hpp"
#include "led_indicator.hpp"
#include "led_strip.hpp"
#include "motor_array.hpp"
#include "motor_hi_z.hpp"
#include "ntc.hpp"
#include "pid.hpp"
#include "power_board.hpp"
#include "transport/spi_transport.hpp"

// ───────── Arduino settings ─────────
inline constexpr uint32_t ADC_MAX_VALUE =
    1023;  // 10-bit ADC resolution (0-1023)

// ───────── Board Peripherals ─────────
inline constexpr uint8_t AUDIO_SHDN = PB2;
inline constexpr uint8_t AUDIO_DAC_OUT = PA4;
inline constexpr uint8_t EN_LOC_5V = PF13;

inline constexpr uint8_t I2C_SDA = PF0;
inline constexpr uint8_t I2C_SCL = PF1;
inline TwoWire i2c(I2C_SDA, I2C_SCL);

inline constexpr uint8_t PB_SERIAL_RX = PD6;
inline constexpr uint8_t PB_SERIAL_TX = PD5;
inline HardwareSerial pb_serial(PB_SERIAL_RX, PB_SERIAL_TX);

// ───────── Buttons ─────────
inline constexpr uint8_t PUSH_BUTTON1 = PF11;
inline constexpr uint8_t PUSH_BUTTON2 = PF12;  // MCU reset button

// ───────── EEPROM ─────────
inline constexpr EepromConfig eeprom_config = {
    .i2c_bus = i2c,
    .dev_id = 0x50,
    .page_size = 16,
    .write_delay_ms = 5,
};
inline Eeprom eeprom(eeprom_config);

inline BoardRevisionConfig board_revision_config = {
    .eeprom = eeprom,
    .block = 0x00,
    .addr = 0x00,
    .max_length = 4,
    .retry_count = 3,
};

// ───────── Encoders ─────────
inline constexpr float GEAR_RATIO = 50.0f;
inline constexpr uint16_t ENCODER_CPR = 64;
inline constexpr float TICKS_PER_REVOLUTION = ENCODER_CPR * GEAR_RATIO;
inline constexpr float RAD_PER_TICK = (2.0f * PI) / TICKS_PER_REVOLUTION;
inline constexpr float LOW_PASS_ALPHA = 0.25f;

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

// ───────── Extra GPIO ─────────
inline constexpr uint8_t CAN1_TX = PD1;  // External transceiver needed
inline constexpr uint8_t CAN1_RX = PD0;  // External transceiver needed

inline constexpr uint8_t EXT_ADC1 = PF3;
inline constexpr uint8_t EXT_ADC2 = PF4;
inline constexpr uint8_t EXT_ADC3 = PF5;

inline constexpr uint8_t EXT_GPIO1 = PG2;
inline constexpr uint8_t EXT_GPIO2 = PG3;
inline constexpr uint8_t EXT_GPIO3 = PG4;

inline constexpr uint8_t EXT_PWM1 = PE5;
inline constexpr uint8_t EXT_PWM2 = PE6;
inline constexpr uint8_t EXT_PWM3 = PB14;

inline constexpr uint8_t I2C1_SDA = PB7;  // External pull-up needed
inline constexpr uint8_t I2C1_SCL = PB6;  // External pull-up needed
inline constexpr uint8_t I2C3_SDA = PC9;  // External pull-up needed
inline constexpr uint8_t I2C3_SCL = PA8;  // External pull-up needed

inline constexpr uint8_t SPI1_MOSI = PB5;
inline constexpr uint8_t SPI1_MISO = PA6;
inline constexpr uint8_t SPI1_SCK = PA5;

inline constexpr uint8_t USART6_RX = PG9;
inline constexpr uint8_t USART6_TX = PG14;

// ───────── Fan and temperature ─────────
inline constexpr NtcConfig ntc_cfg{
    .pin = PB1,
    .pullup_resistance = 5230.0f,
    .c1 = 1.112613927e-03f,
    .c2 = 2.37277392e-04f,
    .c3 = 7.1670e-08f,
    .offset = 273.15 + 3,  // Kelvin to Celsius offset + calibration offset
    .adc_max = 1023.0f};
inline Ntc ntc(ntc_cfg);

inline constexpr uint8_t FAN_PP_PIN = PC13;
inline constexpr uint8_t FAN_PWM_PIN = PB0;

inline constexpr FanConfig rev1_2_fan_config{
    .pin = FAN_PWM_PIN,
    .timer_instance = TIM8,  // ← jawnie, bez auto-detekcji
    .timer_channel = 2,      // CH2 — library wykryje CH2N z PinMap
    .gpio_af = GPIO_AF3_TIM8,
    .complementary = true,  // CH2N = odwrócony sygnał
    .mode = FanMode::Proportional,
    .pwm_frequency = 25000,  // 25 kHz — standard PC fan
    .temp_low = 30,          // poniżej 30°C — duty_min
    .temp_high = 55,         // powyżej 55°C — duty_max
    .duty_min = 20,          // 20% — minimalne obroty (żeby fan nie zatkał)
    .duty_max = 100          // 100% — full blast
};

inline constexpr FanConfig rev1_1_fan_config{.pin = FAN_PP_PIN,
                                             .mode = FanMode::AlwaysOn};

// ───────── IMU ─────────
inline constexpr ImuBno055Config imu_bno055_config = {
    .bus = &i2c,
    .i2c_addr = 0x29,
    .sensor_id = 0x37,
    .int_pin = PF2,
    .axis_config = Adafruit_BNO055::REMAP_CONFIG_P1,
    .axis_sign = Adafruit_BNO055::REMAP_SIGN_P2,
};

// ───────── LEDs ─────────
inline constexpr uint8_t RED_LED = PE4;
inline constexpr uint8_t GRN_LED = PE3;

inline constexpr LedIndicatorConfig led_status_config = {
    .pin = RED_LED,
    .initial_state = HIGH,
    .blink_period_ms = 500,
    .label = "STATUS",
};

// ───────── LED Strip ─────────
inline constexpr uint16_t IDLE_ANIMATION_CHANGE_MS = 3000;
inline constexpr uint16_t IDLE_ANIMATION_INTERVAL_MS = 35;
inline constexpr uint16_t LED_STRIP_TIMEOUT_MS = 1000;

inline constexpr SpiTransportConfig spi_config = {
    .mosi_pin = PB15,
    .miso_pin = PB14,
    .sck_pin = PB10,
    .spi_speed = 4000000,
    .bit_order = MSBFIRST,
    .spi_mode = SPI_MODE3,
};

inline constexpr SwapPair swaps[] = {
    {13, 17},
    {14, 16},
};
inline constexpr LedStripConfig strip_config = {
    .num_leds = 18,
    .swaps = swaps,
    .swap_count = sizeof(swaps) / sizeof(swaps[0]),
    .init_r = 0x0F,
    .init_g = 0x00,
    .init_b = 0x00,
};

// ───────── Motors ─────────
inline constexpr uint32_t MOTOR_PWM_FREQ = 20000;  // 20 kHz
inline constexpr float MAX_VELOCITY = 23.0f;
inline constexpr float MIN_VELOCITY = 0.0f;

// Table 2:
// https://www.analog.com/media/en/technical-documentation/data-sheets/max22205.pdf
inline constexpr uint8_t ILIM1 = PE10;
inline constexpr uint8_t ILIM2 = PG15;
inline constexpr uint8_t ILIM3 = PG7;
inline constexpr uint8_t ILIM4 = PD14;

// MAX22205 current-sense (rev 1.2 only). The CSO/ISEN pin sources
//   I_isen = I_motor / KISEN          (KISEN = 7500 A/A, datasheet)
// across an external R_sense_ext = 3.3 kΩ to GND, so:
//   I_motor [A] = V_isen [V] * (KISEN / R_sense_ext)
inline constexpr float MAX22205_KISEN = 7500.0f;  // [A/A]
inline constexpr float ISEN_RSENSE = 3300.0f;     // [Ω]
inline constexpr float MOTOR_CURRENT_PER_VOLT =
    MAX22205_KISEN / ISEN_RSENSE;  // [A/V] ≈ 2.27

// Gear-motor spec sheet: 12 V, 220 RPM no-load, 250 mA no-load, 21 kg·cm
// stall, 50:1 gearbox. Derived (assuming η ≈ 0.78):
//   I_stall ≈ 5.3 A,  R ≈ 2.17 Ω,  Ke ≈ 0.00994 V·s/rad
//   Kt_total = Ke × N × η ≈ 0.39 Nm/A   (sanity: 0.39 × 5.3 ≈ 2.06 Nm)
// On rev 1.2 the MAX22205 CSO is the primary effort source; the back-EMF
// model below is the fallback used on revisions without a real sensor.
inline constexpr float MOTOR_TORQUE_CONSTANT = 0.39f;  // [Nm/A]
inline constexpr float MOTOR_RESISTANCE = 2.17f;       // [Ω]
inline constexpr float MOTOR_BACK_EMF = 0.00994f;      // [V·s/rad]
inline constexpr float MOTOR_SUPPLY_VOLTAGE = 12.0f;   // [V] nominal

inline constexpr DriverGroupConfig right_motors_driver = {PC13, PE0};
inline constexpr DriverGroupConfig left_motors_driver = {PC14, PE1};
inline constexpr DriverGroupConfig driver_groups[] = {
    right_motors_driver,
    left_motors_driver,
};

inline constexpr MotorHiZConfig motor_fl_config = {
    .pwm_pin = PF9,
    .in_a_pin = PD10,
    .in_b_pin = PD11,
    .inv_dir = true,
    .max_velocity = MAX_VELOCITY,
    .min_velocity = MIN_VELOCITY,
    .pwm_freq = MOTOR_PWM_FREQ,
    .frame_id = "fl_wheel_joint",
    .current_sense_pin = PA0,
    .current_per_volt = MOTOR_CURRENT_PER_VOLT,
    .torque_constant = MOTOR_TORQUE_CONSTANT,
    .gear_ratio = GEAR_RATIO,
    .winding_resistance = MOTOR_RESISTANCE,
    .back_emf_constant = MOTOR_BACK_EMF,
    .supply_voltage = MOTOR_SUPPLY_VOLTAGE,
};

inline constexpr MotorHiZConfig motor_fr_config = {
    .pwm_pin = PF8,
    .in_a_pin = PG5,
    .in_b_pin = PG6,
    .inv_dir = false,
    .max_velocity = MAX_VELOCITY,
    .min_velocity = MIN_VELOCITY,
    .pwm_freq = MOTOR_PWM_FREQ,
    .frame_id = "fr_wheel_joint",
    .current_sense_pin = PA3,
    .current_per_volt = MOTOR_CURRENT_PER_VOLT,
    .torque_constant = MOTOR_TORQUE_CONSTANT,
    .gear_ratio = GEAR_RATIO,
    .winding_resistance = MOTOR_RESISTANCE,
    .back_emf_constant = MOTOR_BACK_EMF,
    .supply_voltage = MOTOR_SUPPLY_VOLTAGE,
};

inline constexpr MotorHiZConfig motor_rl_config = {
    .pwm_pin = PF7,
    .in_a_pin = PG11,
    .in_b_pin = PG12,
    .inv_dir = true,
    .max_velocity = MAX_VELOCITY,
    .min_velocity = MIN_VELOCITY,
    .pwm_freq = MOTOR_PWM_FREQ,
    .frame_id = "rl_wheel_joint",
    .current_sense_pin = PC3,
    .current_per_volt = MOTOR_CURRENT_PER_VOLT,
    .torque_constant = MOTOR_TORQUE_CONSTANT,
    .gear_ratio = GEAR_RATIO,
    .winding_resistance = MOTOR_RESISTANCE,
    .back_emf_constant = MOTOR_BACK_EMF,
    .supply_voltage = MOTOR_SUPPLY_VOLTAGE,
};

inline constexpr MotorHiZConfig motor_rr_config = {
    .pwm_pin = PF6,
    .in_a_pin = PE12,
    .in_b_pin = PE13,
    .inv_dir = false,
    .max_velocity = MAX_VELOCITY,
    .min_velocity = MIN_VELOCITY,
    .pwm_freq = MOTOR_PWM_FREQ,
    .frame_id = "rr_wheel_joint",
    .current_sense_pin = PC2,
    .current_per_volt = MOTOR_CURRENT_PER_VOLT,
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
    .kv = 0.04,  // 1.0f / MAX_VELOCITY velocity feedforward (~0.045)
    .min_output = -1.0f,
    .max_output = 1.0f,
    .max_brake_output = 0.3f,  // allow up to 30% reverse PWM for braking
};

// ───────── ROS ─────────
inline constexpr const char* NODE_NAME = "rosbot_mcu";
inline constexpr uint16_t DOMAIN_ID = 255;  // 255 inherit from Micro ROS Agent
inline constexpr uint32_t SPIN_TIME_MS = 10;
inline constexpr uint32_t TIMER_MS = 10;
inline constexpr uint16_t PING_WATCHDOG_MS = 200;
inline constexpr uint16_t PING_TIMEOUT_MS = 100;
inline constexpr uint8_t PING_ATTEMPTS = 1;

inline byte MAC[6] = {0x02, 0x47, 0x00, 0x00, 0x00, 0x01};
inline IPAddress CLIENT_IP = {192, 168, 77, 3};
inline IPAddress AGENT_IP = {192, 168, 77, 2};
inline uint16_t AGENT_PORT = 8888;

// ───────── Publishers ─────────
inline QueueHandle_t battery_queue;
inline QueueHandle_t imu_queue;
inline QueueHandle_t joint_state_queue;
inline QueueHandle_t led_strip_queue;

inline constexpr uint8_t BATTERY_NUM_CELLS = 3;
inline constexpr float BATTERY_CELL_CAPACITY = 2.6f;  // Ah
inline constexpr float BATTERY_DESIGN_CAPACITY =
    BATTERY_NUM_CELLS * BATTERY_CELL_CAPACITY;
inline constexpr uint8_t buttons_pins[] = {PUSH_BUTTON1};

// Publisher configs live in src/rosbot_xl/ros.cpp — see
// include/rosbot/config.hpp for the ODR rationale.

// ───────── Power Board - Battery ─────────
inline constexpr PowerBoardConfig power_board_config = {
    .serial = pb_serial,
    .baudrate = 38400,
    .timeout_ms = 100,
    .v_min = 9.6f,
    .v_max = 12.6f,
};

// ───────── Power Board - Shutdown ─────────
// Power Board send high signal when shutdown button is pressed
inline constexpr uint8_t PB_SHD_DETECT = PD4;
// Power Board monitors if signal is high to confirm shutdown
inline constexpr uint8_t PB_SHD_CONFIRM = PD7;

// ───────── SBC Interface ─────────
// SBC has few seconds to shutdown after receiving shutdown command
inline constexpr uint16_t SHUTDOWN_WAIT_MS = 10000;
inline constexpr SerialConfig DIAGNOSTIC_SERIAL_CONFIG = {
    .serial = &Serial1,
    .baudrate = 921600,
    .rxPin = PA10,
    .txPin = PA9,
    .timeout_ms = 1,
    .name = "FTDI_SERIAL"};
