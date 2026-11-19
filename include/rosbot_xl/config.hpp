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
#include "pin.hpp"
#include "power_board.hpp"
#include "transport/spi_transport.hpp"

using platform::AnalogIn;
using platform::Pin;
using platform::Port;

// ───────── Arduino settings ─────────
inline constexpr uint32_t ADC_MAX_VALUE =
    1023;  // 10-bit ADC resolution (0-1023)

// ───────── Board Peripherals ─────────
inline constexpr Pin AUDIO_SHDN{Port::B, 2};
inline constexpr uint8_t AUDIO_DAC_OUT = PA4;
inline constexpr Pin EN_LOC_5V{Port::F, 13};
inline constexpr Pin GPIO_HUB_NRST{Port::E, 1};

// Shared by the IMU and the EEPROM.
inline platform::I2c i2c({.instance = I2C2,
                          .scl = Pin{Port::F, 1},
                          .sda = Pin{Port::F, 0},
                          .af = GPIO_AF4_I2C2});

inline platform::Uart pb_uart({.usart = USART2,
                               .tx = Pin{Port::D, 5},
                               .rx = Pin{Port::D, 6},
                               .af = GPIO_AF7_USART2,
                               .tx_queue_bytes = 256});

// ───────── Buttons ─────────
inline constexpr Pin PUSH_BUTTON1{Port::F, 11};
inline constexpr Pin PUSH_BUTTON2{Port::F, 12};  // MCU reset button

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
inline constexpr float RAD_PER_TICK =
    static_cast<float>(2.0 * PI / static_cast<double>(TICKS_PER_REVOLUTION));
inline constexpr float LOW_PASS_ALPHA = 0.25f;

inline constexpr HardwareEncoderConfig enc_fl_config = {
    .pin_a = Pin{Port::D, 12},
    .pin_b = Pin{Port::D, 13},
    .timer = TIM4,
    .inv_dir = true,
    .rad_per_tick = RAD_PER_TICK,
    .frame_id = "fl_wheel_joint",
    .alpha = LOW_PASS_ALPHA,
};

inline constexpr HardwareEncoderConfig enc_fr_config = {
    .pin_a = Pin{Port::C, 6},
    .pin_b = Pin{Port::C, 7},
    .timer = TIM3,
    .inv_dir = false,
    .rad_per_tick = RAD_PER_TICK,
    .frame_id = "fr_wheel_joint",
    .alpha = LOW_PASS_ALPHA,
};

inline constexpr HardwareEncoderConfig enc_rl_config = {
    .pin_a = Pin{Port::A, 15},
    .pin_b = Pin{Port::B, 3},
    .timer = TIM2,
    .inv_dir = true,
    .rad_per_tick = RAD_PER_TICK,
    .frame_id = "rl_wheel_joint",
    .alpha = LOW_PASS_ALPHA,
};

inline constexpr HardwareEncoderConfig enc_rr_config = {
    .pin_a = Pin{Port::E, 9},
    .pin_b = Pin{Port::E, 11},
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
    .pullup_resistance = 5230.0f,
    .c1 = 1.112613927e-03f,
    .c2 = 2.37277392e-04f,
    .c3 = 7.1670e-08f,
    .offset = 273.15 + 3,  // Kelvin to Celsius offset + calibration offset
    .adc_max = 1023.0f};
inline Ntc ntc(AnalogIn{Pin{Port::B, 1}, 9}, ntc_cfg);

inline constexpr Pin FAN_PP_PIN{Port::C, 13};
inline constexpr Pin FAN_PWM_PIN{Port::B, 0};

inline constexpr FanConfig rev1_2_fan_config{
    .pin = FAN_PWM_PIN,
    .timer_instance = TIM8,  // explicit, no auto-detection
    .timer_channel = 2,      // PB0 is TIM8_CH2N
    .gpio_af = GPIO_AF3_TIM8,
    .complementary = true,
    .mode = FanMode::Proportional,
    .pwm_frequency = 25000,  // 25 kHz — standard PC fan
    .temp_low = 30,          // off at or below 30 °C
    .temp_high = 55,         // 100% at or above 55 °C
    .duty_min = 20,          // lowest duty that still spins the fan
};

inline constexpr FanConfig rev1_1_fan_config{.pin = FAN_PP_PIN,
                                             .mode = FanMode::AlwaysOn};

// ───────── IMU ─────────
inline constexpr ImuBno055Config imu_bno055_config = {
    .bus = &i2c,
    .i2c_addr = 0x29,
    .int_pin = Pin{Port::F, 2},
    .axis_config = bno055::kRemapConfigP1,
    .axis_sign = bno055::kRemapSignP2,
};

// ───────── LEDs ─────────
inline constexpr Pin RED_LED{Port::E, 4};
inline constexpr Pin GRN_LED{Port::E, 3};
inline constexpr Pin green_leds[] = {GRN_LED};

inline constexpr LedIndicatorConfig led_status_config = {
    .pin = RED_LED,
    .initial_on = true,
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
inline constexpr Pin ILIM1{Port::E, 10};
inline constexpr Pin ILIM2{Port::G, 15};
inline constexpr Pin ILIM3{Port::G, 7};
inline constexpr Pin ILIM4{Port::D, 14};

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

// DRV8870 / MAX22205 have no nSLEEP/nFAULT wired to the MCU, so there are no
// driver groups. The pins the rosbot config uses for them are PC13 (rev 1.1
// fan, unconnected on rev 1.2), PC14 and PE0 (unconnected) and PE1
// (GPIO_HUB_NRST).

inline constexpr MotorHiZConfig motor_fl_config = {
    .pwm = {.pin = Pin{Port::F, 9},
            .timer = TIM14,
            .channel = 1,
            .af = GPIO_AF9_TIM14},
    .in_a_pin = Pin{Port::D, 10},
    .in_b_pin = Pin{Port::D, 11},
    .inv_dir = true,
    .max_velocity = MAX_VELOCITY,
    .min_velocity = MIN_VELOCITY,
    .pwm_freq = MOTOR_PWM_FREQ,
    .frame_id = "fl_wheel_joint",
    .current_sense = AnalogIn{Pin{Port::A, 0}, 0},
    .current_per_volt = MOTOR_CURRENT_PER_VOLT,
    .torque_constant = MOTOR_TORQUE_CONSTANT,
    .gear_ratio = GEAR_RATIO,
    .winding_resistance = MOTOR_RESISTANCE,
    .back_emf_constant = MOTOR_BACK_EMF,
    .supply_voltage = MOTOR_SUPPLY_VOLTAGE,
};

inline constexpr MotorHiZConfig motor_fr_config = {
    .pwm = {.pin = Pin{Port::F, 8},
            .timer = TIM13,
            .channel = 1,
            .af = GPIO_AF9_TIM13},
    .in_a_pin = Pin{Port::G, 5},
    .in_b_pin = Pin{Port::G, 6},
    .inv_dir = false,
    .max_velocity = MAX_VELOCITY,
    .min_velocity = MIN_VELOCITY,
    .pwm_freq = MOTOR_PWM_FREQ,
    .frame_id = "fr_wheel_joint",
    .current_sense = AnalogIn{Pin{Port::A, 3}, 3},
    .current_per_volt = MOTOR_CURRENT_PER_VOLT,
    .torque_constant = MOTOR_TORQUE_CONSTANT,
    .gear_ratio = GEAR_RATIO,
    .winding_resistance = MOTOR_RESISTANCE,
    .back_emf_constant = MOTOR_BACK_EMF,
    .supply_voltage = MOTOR_SUPPLY_VOLTAGE,
};

inline constexpr MotorHiZConfig motor_rl_config = {
    .pwm = {.pin = Pin{Port::F, 7},
            .timer = TIM11,
            .channel = 1,
            .af = GPIO_AF3_TIM11},
    .in_a_pin = Pin{Port::G, 11},
    .in_b_pin = Pin{Port::G, 12},
    .inv_dir = true,
    .max_velocity = MAX_VELOCITY,
    .min_velocity = MIN_VELOCITY,
    .pwm_freq = MOTOR_PWM_FREQ,
    .frame_id = "rl_wheel_joint",
    .current_sense = AnalogIn{Pin{Port::C, 3}, 13},
    .current_per_volt = MOTOR_CURRENT_PER_VOLT,
    .torque_constant = MOTOR_TORQUE_CONSTANT,
    .gear_ratio = GEAR_RATIO,
    .winding_resistance = MOTOR_RESISTANCE,
    .back_emf_constant = MOTOR_BACK_EMF,
    .supply_voltage = MOTOR_SUPPLY_VOLTAGE,
};

inline constexpr MotorHiZConfig motor_rr_config = {
    .pwm = {.pin = Pin{Port::F, 6},
            .timer = TIM10,
            .channel = 1,
            .af = GPIO_AF3_TIM10},
    .in_a_pin = Pin{Port::E, 12},
    .in_b_pin = Pin{Port::E, 13},
    .inv_dir = false,
    .max_velocity = MAX_VELOCITY,
    .min_velocity = MIN_VELOCITY,
    .pwm_freq = MOTOR_PWM_FREQ,
    .frame_id = "rr_wheel_joint",
    .current_sense = AnalogIn{Pin{Port::C, 2}, 12},
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

// ───────── Ethernet ─────────
inline byte MAC[6] = {0x02, 0x47, 0x00, 0x00, 0x00, 0x01};
inline IPAddress CLIENT_IP = {192, 168, 77, 3};
inline IPAddress SBC_IP = {192, 168, 77, 2};

// ───────── Publishers ─────────
inline QueueHandle_t battery_queue;
inline QueueHandle_t imu_queue;
inline QueueHandle_t joint_state_queue;
inline QueueHandle_t led_strip_queue;

// HCP18650ZC-3S3P: 3 series groups of 3 parallel cells, 3.7 V / 2.6 Ah each.
inline constexpr uint8_t BATTERY_SERIES_CELLS = 3;
inline constexpr uint8_t BATTERY_PARALLEL_CELLS = 3;
inline constexpr float BATTERY_CELL_CAPACITY = 2.6f;  // Ah
inline constexpr float BATTERY_DESIGN_CAPACITY =
    BATTERY_PARALLEL_CELLS * BATTERY_CELL_CAPACITY;
inline constexpr Pin buttons_pins[] = {PUSH_BUTTON1};

// Publisher configs live in src/rosbot_xl/mavlink_entities.cpp.

// ───────── Power Board - Battery ─────────
inline constexpr PowerBoardConfig power_board_config = {
    .uart = pb_uart,
    .baudrate = 38400,
    .num_cells = BATTERY_SERIES_CELLS,
    .capacity_ah = BATTERY_DESIGN_CAPACITY,
};

// ───────── Power Board - Shutdown ─────────
// Power Board send high signal when shutdown button is pressed
inline constexpr Pin PB_SHD_DETECT{Port::D, 4};
// Power Board monitors if signal is high to confirm shutdown
inline constexpr Pin PB_SHD_CONFIRM{Port::D, 7};

// ───────── SBC Interface ─────────
// SBC has few seconds to shutdown after receiving shutdown command
inline constexpr uint16_t SHUTDOWN_WAIT_MS = 10000;
// Debug text only: rosbot_xl's MAVLink link is Ethernet.
inline platform::Uart ftdi_uart({.usart = USART1,
                                 .tx = Pin{Port::A, 9},
                                 .rx = Pin{Port::A, 10},
                                 .af = GPIO_AF7_USART1,
                                 .tx_queue_bytes = 1024});
inline constexpr SerialConfig DIAGNOSTIC_SERIAL_CONFIG = {
    .uart = &ftdi_uart, .baudrate = 921600, .name = "FTDI_SERIAL"};
