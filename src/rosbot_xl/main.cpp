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

#include <Arduino.h>

#include "battery_interface.hpp"
#include "config.hpp"
#include "encoder_array.hpp"
#include "hardware_encoder.hpp"
#include "imu_bno055.hpp"
#include "led_indicator.hpp"
#include "led_strip.hpp"
#include "motor_array.hpp"
#include "motor_drv8848.hpp"
#include "ros/ros_node.hpp"
#include "rtos.hpp"
#include "serial_manager.hpp"

// ───────── Battery ─────────
// BatteryAdc battery_adc(battery_adc_config);

// ───────── Board Revision ─────────
static BoardRevision board_revision(board_revision_config);

// ───────── Encoders ─────────
static HardwareEncoder enc_fl(enc_fl_config);
static HardwareEncoder enc_fr(enc_fr_config);
static HardwareEncoder enc_rl(enc_rl_config);
static HardwareEncoder enc_rr(enc_rr_config);
static EncoderInterface* encoders[] = {&enc_fl, &enc_fr, &enc_rl, &enc_rr};
static constexpr uint8_t ENCODER_COUNT = sizeof(encoders) / sizeof(encoders[0]);

// ───────── Fan ─────────
FanController g_fan;

// ───────── IMU ─────────
static ImuBno055 imu_bno055(imu_bno055_config);

// ───────── LED Strip ─────────
static SpiTransport s_transport(spi_config);

// ───────── Motors ─────────
static MotorDrv8848 motor_fl(motor_fl_config, &enc_fl,
                             PIDController(pid_config));
static MotorDrv8848 motor_fr(motor_fr_config, &enc_fr,
                             PIDController(pid_config));
static MotorDrv8848 motor_rl(motor_rl_config, &enc_rl,
                             PIDController(pid_config));
static MotorDrv8848 motor_rr(motor_rr_config, &enc_rr,
                             PIDController(pid_config));
static MotorInterface* motors[] = {&motor_fl, &motor_fr, &motor_rl, &motor_rr};
static constexpr uint8_t MOTOR_COUNT = sizeof(motors) / sizeof(motors[0]);
static constexpr uint8_t DRIVER_GROUP_COUNT =
    sizeof(driver_groups) / sizeof(driver_groups[0]);

// ─────────Extern variables─────────
// BatteryInterface* g_battery = &battery_adc;
EncoderArray g_encoders(encoders, ENCODER_COUNT);
ImuInterface* g_imu = &imu_bno055;
LedIndicator g_indicator(led_status_config);
LedStrip g_led_strip;
MotorArray g_motors(motors, MOTOR_COUNT, driver_groups, DRIVER_GROUP_COUNT);

SerialManager g_serialManager({.main = FTDI_SERIAL_CONFIG});

void boardPheripheralsInit() {
  // Audio
  pinMode(AUDIO_SHDN, OUTPUT);
  digitalWrite(AUDIO_SHDN, HIGH);

  // Buttons
  pinMode(PUSH_BUTTON1, INPUT_PULLUP);
  pinMode(PUSH_BUTTON2, INPUT_PULLUP);

  // Fan
  pinMode(FAN_PP_PIN, OUTPUT);
  digitalWrite(FAN_PP_PIN, LOW);

  // LEDs
  pinMode(RED_LED, OUTPUT);
  pinMode(GRN_LED, OUTPUT);
  digitalWrite(RED_LED, HIGH);

  // Peripheral Power
  pinMode(EN_LOC_5V, OUTPUT);
  digitalWrite(EN_LOC_5V, HIGH);

  // Power board
  pinMode(PWR_BRD_GPIO_INPUT, INPUT_PULLUP);

  // I2C
  imu_i2c.begin();
  imu_i2c.setClock(400000);

  delay(50);
}

/*───────── Setup ─────────*/
void setup() {
  boardPheripheralsInit();

  // Pre-communication
  g_serialManager.init();
  const auto& selected_serial = g_serialManager.selectCommunicationSerial(0);
  g_serialManager.configureNamespace();
  g_ros_node.setNamespace(g_serialManager.getNamespace());

  // Board revision detection
  board_revision.init();
  auto rev = board_revision.revision();
  auto fan_config =
      (rev == Revision::V1_1) ? rev1_1_fan_config : rev1_2_fan_config;

  // Sensors initialization
  // battery_adc.init();
  g_encoders.init();
  ntc.init();
  g_fan.init(fan_config);
  imu_bno055.init();
  g_indicator.init();
  g_led_strip.init(strip_config, &s_transport);
  g_motors.init();
  g_ros_node.transportInit(selected_serial);

  // RTOS
  createQueues();
  createTasks();
  vTaskStartScheduler();
}

/*───────── Loop ─────────*/
void loop() {}

/*───────── Runtime stats ─────────*/
HardwareTimer RunTimeStatsTimer(TIM5);

void vConfigureTimerForRunTimeStats(void) {
  RunTimeStatsTimer.setPrescaleFactor(
      1680);  // every 10 µs (168MHz / 1680 = 100kHz)
  RunTimeStatsTimer.setOverflow(0xFFFFFFFF);
  RunTimeStatsTimer.refresh();
  RunTimeStatsTimer.resume();
}

uint32_t vGetTimerValueForRunTimeStats(void) {
  return RunTimeStatsTimer.getCount();
}
