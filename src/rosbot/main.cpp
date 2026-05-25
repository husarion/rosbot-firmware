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

#include <cstring>

#include "battery_adc.hpp"
#include "battery_interface.hpp"
#include "comm_backend.hpp"
#include "communication_manager.hpp"
#include "config.hpp"
#include "hardware_encoder.hpp"
#include "imu_bno055.hpp"
#include "led_indicator.hpp"
#include "mavlink_node.hpp"
#include "motor_array.hpp"
#include "motor_hi_z.hpp"
#include "persistent_config.hpp"
#include "range_array.hpp"
#include "range_vl53l0.hpp"
#include "robotics_link.hpp"
#include "ros/ros_node.hpp"
#include "rtos.hpp"

// ───────── Battery ─────────
BatteryAdc battery_adc(battery_adc_config);

// ───────── Encoders ─────────
static HardwareEncoder enc_fl(enc_fl_config);
static HardwareEncoder enc_fr(enc_fr_config);
static HardwareEncoder enc_rl(enc_rl_config);
static HardwareEncoder enc_rr(enc_rr_config);

// ───────── IMU ─────────
ImuBno055 imu_bno055(imu_bno055_config);

// ───────── Motors ─────────
static MotorHiZ motor_fl(motor_fl_config, &enc_fl, PIDController(pid_config));
static MotorHiZ motor_fr(motor_fr_config, &enc_fr, PIDController(pid_config));
static MotorHiZ motor_rl(motor_rl_config, &enc_rl, PIDController(pid_config));
static MotorHiZ motor_rr(motor_rr_config, &enc_rr, PIDController(pid_config));
static MotorInterface* motors[] = {&motor_fl, &motor_fr, &motor_rl, &motor_rr};
static constexpr uint8_t MOTOR_COUNT = sizeof(motors) / sizeof(motors[0]);
static constexpr uint8_t DRIVER_GROUP_COUNT =
    sizeof(driver_groups) / sizeof(driver_groups[0]);

// ───────── Ranges ─────────
RangeVl53l0x range_fl(range_fl_config);
RangeVl53l0x range_fr(range_fr_config);
RangeVl53l0x range_rl(range_rl_config);
RangeVl53l0x range_rr(range_rr_config);
static RangeInterface* range_sensors[] = {&range_fl, &range_fr, &range_rl,
                                          &range_rr};
static constexpr uint8_t RANGE_COUNT =
    sizeof(range_sensors) / sizeof(range_sensors[0]);

// ───────── Extern variables ─────────
BatteryInterface* g_battery = &battery_adc;
ImuInterface* g_imu = &imu_bno055;
LedIndicator g_indicator(led_status_config);
MotorArray g_motors(motors, MOTOR_COUNT, driver_groups, DRIVER_GROUP_COUNT);
RangeArray g_ranges(range_sensors, RANGE_COUNT);

bool useAlt() {
  return digitalRead(PUSH_BUTTON1) == LOW || digitalRead(PUSH_BUTTON2) == LOW;
}

void confirmAlt() {
  digitalWrite(GRN_LED, HIGH);
  digitalWrite(GRN_LED2, HIGH);
}

CommunicationManagerConfig communication_config = {
    .primary_type = TransportType::kSerial,
    .primary_serial = SBC_SERIAL_CONFIG,
    .diagnostic_serial = DIAGNOSTIC_SERIAL_CONFIG,
    .useDiagnosticCondition = useAlt,
    .onDiagnosticSelected = confirmAlt};
CommunicationManager g_comm_mgr(communication_config);

static float supplyVoltage() { return g_battery->getData().voltage; }

void boardPheripheralsInit() {
  // Initialize Buttons
  pinMode(PUSH_BUTTON1, INPUT_PULLUP);
  pinMode(PUSH_BUTTON2, INPUT_PULLUP);

  // Initialize LEDs
  pinMode(RED_LED, OUTPUT);
  pinMode(GRN_LED, OUTPUT);
  pinMode(GRN_LED2, OUTPUT);
  digitalWrite(RED_LED, HIGH);

  // Enable power for IMU sensor
  pinMode(IMU_POWER_ON, OUTPUT);
  digitalWrite(IMU_POWER_ON, HIGH);

  // Initialize I2C
  imu_i2c.begin();
  imu_i2c.setClock(400000);
  range_i2c.begin();
  range_i2c.setClock(400000);

  delay(20);
}

// Lives in .data so g_comm_mgr's ns_default pointer stays valid.
static persistent_config::Config s_persistent;

/*───────── Setup ─────────*/
void setup() {
  boardPheripheralsInit();

  s_persistent = persistent_config::load();
  g_comm_mgr.setBackendDefault(s_persistent.backend);
  g_comm_mgr.setNamespaceDefault(s_persistent.ns);

  g_comm_mgr.init();
  const SerialConfig* transport = g_comm_mgr.selectTransport();
  g_comm_mgr.configureNamespace();

  persistent_config::Config now{};
  now.backend = g_comm_mgr.getSelectedBackend();
  std::strncpy(now.ns, g_comm_mgr.getNamespace(),
               persistent_config::kNamespaceMaxLen);
  now.ns[persistent_config::kNamespaceMaxLen - 1] = '\0';
  persistent_config::save(now);

  battery_adc.init();
  imu_bno055.init();
  g_indicator.init();
  for (auto* m : {&motor_fl, &motor_fr, &motor_rl, &motor_rr}) {
    m->setSupplyVoltageProvider(supplyVoltage);
  }
  g_motors.init();  // motors own encoders → enc init happens here
  g_ranges.init();

  // Only the chosen backend's transport is opened; the other stays inert.
  if (g_comm_mgr.getSelectedBackend() == CommBackend::MAVLINK) {
    g_mavlink_node.setNamespace(g_comm_mgr.getNamespace());
    g_mavlink_node.setDiagnosticSerial(g_comm_mgr.debugSerial());
    g_mavlink_node.begin();
    g_link = &g_mavlink_node;
  } else {
    g_ros_node.setNamespace(g_comm_mgr.getNamespace());
    g_ros_node.serialTransportInit(*transport);
    g_ros_node.setDiagnosticSerial(g_comm_mgr.debugSerial());
    g_link = &g_ros_node;
  }

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
