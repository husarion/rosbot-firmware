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
#include "communication_manager.hpp"
#include "config.hpp"
#include "encoder_array.hpp"
#include "hardware_encoder.hpp"
#include "imu_bno055.hpp"
#include "led_indicator.hpp"
#include "led_strip.hpp"
#include "motor_array.hpp"
#include "motor_hi_z.hpp"
#include "power_board.hpp"
#include "ros/ros_node.hpp"
#include "rtos.hpp"

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

// ───────── Motors (compatible with MAX22205) ─────────
// TODO: Can be improved and used tourque control
static MotorHiZ motor_fl(motor_fl_config, &enc_fl, PIDController(pid_config));
static MotorHiZ motor_fr(motor_fr_config, &enc_fr, PIDController(pid_config));
static MotorHiZ motor_rl(motor_rl_config, &enc_rl, PIDController(pid_config));
static MotorHiZ motor_rr(motor_rr_config, &enc_rr, PIDController(pid_config));
static MotorInterface* motors[] = {&motor_fl, &motor_fr, &motor_rl, &motor_rr};
static constexpr uint8_t MOTOR_COUNT = sizeof(motors) / sizeof(motors[0]);
static constexpr uint8_t DRIVER_GROUP_COUNT =
    sizeof(driver_groups) / sizeof(driver_groups[0]);

// ───────── Power Board ─────────
PowerBoard power_board(power_board_config);

// ─────────Extern variables─────────
BatteryInterface* g_battery = &power_board;
EncoderArray g_encoders(encoders, ENCODER_COUNT);
ImuInterface* g_imu = &imu_bno055;
LedIndicator g_indicator(led_status_config);
LedStrip g_led_strip;
MotorArray g_motors(motors, MOTOR_COUNT, driver_groups, DRIVER_GROUP_COUNT);

bool useAlt() { return digitalRead(PUSH_BUTTON1) == LOW; }

void confirmAlt() { digitalWrite(GRN_LED, HIGH); }

CommunicationManagerConfig communication_config = {
    .primary_type = TransportType::kEthernet,
    .diagnostic_serial = DIAGNOSTIC_SERIAL_CONFIG,
    .useDiagnosticCondition = useAlt,
    .onDiagnosticSelected = confirmAlt};

CommunicationManager g_comm_mgr(communication_config);

static float supplyVoltage() { return g_battery->getData().voltage; }

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
  pinMode(PB_SHD_DETECT, INPUT_PULLUP);
  pinMode(PB_SHD_CONFIRM, OUTPUT);
  digitalWrite(PB_SHD_CONFIRM, LOW);

  // I2C
  i2c.begin();
  i2c.setClock(400000);

  delay(50);
}

void setMaxMotorsCurrent(Revision rev) {
  switch (rev) {
    case Revision::V1_2:
      pinMode(ILIM1, INPUT);
      pinMode(ILIM2, INPUT);
      pinMode(ILIM3, INPUT);
      pinMode(ILIM4, INPUT);
      break;

    case Revision::V1_1:
      pinMode(ILIM1, OUTPUT);
      pinMode(ILIM2, OUTPUT);
      pinMode(ILIM3, OUTPUT);
      pinMode(ILIM4, OUTPUT);
      digitalWrite(ILIM1, HIGH);
      digitalWrite(ILIM2, HIGH);
      digitalWrite(ILIM3, HIGH);
      digitalWrite(ILIM4, HIGH);
      // V1_1 uses DRV8870 do not have a real current sensor.
      motor_fl.disableCurrentSensor();
      motor_fr.disableCurrentSensor();
      motor_rl.disableCurrentSensor();
      motor_rr.disableCurrentSensor();
      break;

    default:
      break;
  }
}

/*───────── Setup ─────────*/
void setup() {
  boardPheripheralsInit();

  // Pre-communication
  g_comm_mgr.init();
  const auto* transport = g_comm_mgr.selectTransport();
  g_comm_mgr.configureNamespace();
  g_ros_node.setNamespace(g_comm_mgr.getNamespace());

  // Revision specific configuration
  board_revision.init();
  auto rev = board_revision.revision();
  setMaxMotorsCurrent(rev);
  auto fan_config =
      (rev == Revision::V1_1) ? rev1_1_fan_config : rev1_2_fan_config;

  // Components initialization
  Ethernet.begin(MAC, CLIENT_IP);
  g_encoders.init();
  ntc.init();
  g_fan.init(fan_config);
  imu_bno055.init();
  g_indicator.init();
  g_led_strip.init(strip_config, &s_transport);
  for (auto* m : {&motor_fl, &motor_fr, &motor_rl, &motor_rr}) {
    m->setSupplyVoltageProvider(supplyVoltage);
  }
  g_motors.init();
  power_board.init();
  if (g_comm_mgr.isSerialTransport()) {
    g_ros_node.serialTransportInit(*transport);
  } else {
    g_ros_node.ethernetTransportInit(AGENT_IP, AGENT_PORT);
  }
  g_ros_node.setDiagnosticSerial(g_comm_mgr.debugSerial());

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
