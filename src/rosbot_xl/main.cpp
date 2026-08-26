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

#include "battery_interface.hpp"
#include "boot_option.hpp"
#include "comm_backend.hpp"
#include "communication_manager.hpp"
#include "config.hpp"
#include "hardware_encoder.hpp"
#include "imu_bno055.hpp"
#include "imu_calibration_boot.hpp"
#include "led_indicator.hpp"
#include "led_strip.hpp"
#include "mavlink_node.hpp"
#include "motor_array.hpp"
#include "motor_hi_z.hpp"
#include "persistent_config.hpp"
#include "power_board.hpp"
#include "robotics_link.hpp"
#include "ros/ros_node.hpp"
#include "rtos.hpp"

// ───────── Board Revision ─────────
static BoardRevision board_revision(board_revision_config);

// ───────── Encoders ─────────
static HardwareEncoder enc_fl(enc_fl_config);
static HardwareEncoder enc_fr(enc_fr_config);
static HardwareEncoder enc_rl(enc_rl_config);
static HardwareEncoder enc_rr(enc_rr_config);

// ───────── Fan ─────────
FanController g_fan;

// ───────── IMU ─────────
ImuBno055 imu_bno055(imu_bno055_config);

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
ImuInterface* g_imu = &imu_bno055;
LedIndicator g_indicator(led_status_config);
LedStrip g_led_strip;
MotorArray g_motors(motors, MOTOR_COUNT, driver_groups, DRIVER_GROUP_COUNT);

// Resolved once by resolveBootAction() at the very start of setup(),
// before anything else reads PUSH_BUTTON1 — see boot_option.hpp.
// kChangeTransport and kCalibrateImu are mutually exclusive by
// construction (one gesture, classified by hold duration), so no extra
// coordination is needed here.
static bool s_transport_diagnostic_requested = false;

bool useAlt() { return s_transport_diagnostic_requested; }

CommunicationManagerConfig communication_config = {
    .primary_type = TransportType::kEthernet,
    .diagnostic_serial = DIAGNOSTIC_SERIAL_CONFIG,
    .useDiagnosticCondition = useAlt,
    // BootOption already confirmed the choice on the green LED before
    // selectTransport() runs — no separate signal needed here.
    .onDiagnosticSelected = nullptr};

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

// Lives in .data so g_comm_mgr's ns_default pointer stays valid.
static persistent_config::Config s_persistent;

/*───────── Setup ─────────*/
void setup() {
  boardPheripheralsInit();

  // Resolve the boot gesture before anything else touches the button —
  // see boot_option.hpp. rosbot_xl only exposes PUSH_BUTTON1 as a
  // readable GPIO (PUSH_BUTTON2 is wired to MCU NRST) and has a single
  // green LED, so green_led2 is left unused (0).
  const uint8_t boot_buttons[] = {PUSH_BUTTON1};
  const BootOptionConfig boot_option_config = {
      .buttons = boot_buttons,
      .button_count = 1,
      .green_led = GRN_LED,
  };
  const BootAction boot_action = resolveBootAction(boot_option_config);
  s_transport_diagnostic_requested =
      boot_action == BootAction::kChangeTransport;
  const bool imu_calibration_requested =
      boot_action == BootAction::kCalibrateImu;

  s_persistent = persistent_config::load();
  g_comm_mgr.setBackendDefault(s_persistent.backend);
  g_comm_mgr.setNamespaceDefault(s_persistent.ns);

  g_comm_mgr.init();
  // useAlt() already returns a fixed value decided by resolveBootAction()
  // above — no need for selectTransport()'s own ~1.5 s polling window on
  // top of it. timeout_ms=1 still lets its single check fire.
  const SerialConfig* transport = g_comm_mgr.selectTransport(1);
  g_comm_mgr.configureNamespace();

  persistent_config::Config now{};
  now.backend = g_comm_mgr.getSelectedBackend();
  std::strncpy(now.ns, g_comm_mgr.getNamespace(),
               persistent_config::kNamespaceMaxLen);
  now.ns[persistent_config::kNamespaceMaxLen - 1] = '\0';
  now.has_imu_calibration = s_persistent.has_imu_calibration;
  now.imu_calibration = s_persistent.imu_calibration;

  // Revision specific configuration
  board_revision.init();
  auto rev = board_revision.revision();
  setMaxMotorsCurrent(rev);
  auto fan_config =
      (rev == Revision::V1_1) ? rev1_1_fan_config : rev1_2_fan_config;

  Ethernet.begin(MAC, CLIENT_IP);
  ntc.init();
  g_fan.init(fan_config);

  const bool imu_ready = imu_bno055.init();
  if (imu_ready) {
    if (now.has_imu_calibration) {
      imu_bno055.applyCalibrationOffsets(now.imu_calibration);
    }
    if (imu_calibration_requested) {
      ImuCalibrationOffsets captured{};
      if (imu_calibration_boot::run(imu_bno055, RED_LED, GRN_LED,
                                    /*green_led2=*/0, captured,
                                    g_comm_mgr.debugSerial())) {
        now.has_imu_calibration = true;
        now.imu_calibration = captured;
      }
    }
  }
  persistent_config::save(now);

  g_indicator.init();
  g_led_strip.init(strip_config, &s_transport);
  for (auto* m : {&motor_fl, &motor_fr, &motor_rl, &motor_rr}) {
    m->setSupplyVoltageProvider(supplyVoltage);
  }
  g_motors.init();
  power_board.init();

  // MAVLink uses the UDP transport bound in g_mavlink_node's ctor;
  // micro-ROS chooses serial vs ethernet at runtime.
  if (g_comm_mgr.getSelectedBackend() == CommBackend::MAVLINK) {
    g_mavlink_node.setNamespace(g_comm_mgr.getNamespace());
    g_mavlink_node.setDiagnosticSerial(g_comm_mgr.debugSerial());
    g_mavlink_node.begin();
    g_link = &g_mavlink_node;
  } else {
    g_ros_node.setNamespace(g_comm_mgr.getNamespace());
    if (g_comm_mgr.isSerialTransport()) {
      g_ros_node.serialTransportInit(*transport);
    } else {
      g_ros_node.ethernetTransportInit(AGENT_IP, AGENT_PORT);
    }
    g_ros_node.setDiagnosticSerial(g_comm_mgr.debugSerial());
    g_link = &g_ros_node;
  }

  // Must run after any boot-time IMU calibration and before the
  // scheduler starts — see enableDmaReads()'s doc comment.
  if (imu_ready) {
    imu_bno055.enableDmaReads();
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
