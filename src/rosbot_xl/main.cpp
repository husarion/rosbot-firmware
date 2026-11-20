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

#include <cstring>

#include "battery_interface.hpp"
#include "boot_option.hpp"
#include "clock.hpp"
#include "communication_manager.hpp"
#include "fault.hpp"
#include "hardware_encoder.hpp"
#include "imu_bno055.hpp"
#include "imu_calibration_boot.hpp"
#include "led_indicator.hpp"
#include "led_strip.hpp"
#include "mavlink_node.hpp"
#include "mcu_health.hpp"
#include "motor_array.hpp"
#include "motor_hi_z.hpp"
#include "persistent_config.hpp"
#include "power_board.hpp"
#include "rosbot_xl/config.hpp"
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

// ───────── Power Board ─────────
PowerBoard power_board(power_board_config);

// ─────────Extern variables─────────
BatteryInterface* g_battery = &power_board;
ImuInterface* g_imu = &imu_bno055;
LedIndicator g_indicator(led_status_config);
LedStrip g_led_strip;
MotorArray g_motors(motors);

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

void boardPeripheralsInit() {
  // Audio
  AUDIO_SHDN.setAsOutput(true);

  // Active-low hub reset; the pull-up keeps the hub running.
  GPIO_HUB_NRST.setAsInput(platform::Pull::kUp);

  // Buttons
  PUSH_BUTTON1.setAsInput(platform::Pull::kUp);
  PUSH_BUTTON2.setAsInput(platform::Pull::kUp);

  // Fan
  FAN_PP_PIN.setAsOutput(false);

  // LEDs
  RED_LED.setAsOutput(true);
  GRN_LED.setAsOutput(false);

  // Peripheral Power
  EN_LOC_5V.setAsOutput(true);

  // Power board
  PB_SHD_DETECT.setAsInput(platform::Pull::kUp);
  PB_SHD_CONFIRM.setAsOutput(false);

  // I2C
  i2c.init();

  platform::clock::delayMs(50);
}

void setMaxMotorsCurrent(Revision rev) {
  switch (rev) {
    case Revision::V1_2:
      for (const Pin& ilim : {ILIM1, ILIM2, ILIM3, ILIM4}) ilim.setAsInput();
      break;

    case Revision::V1_1:
      for (const Pin& ilim : {ILIM1, ILIM2, ILIM3, ILIM4}) {
        ilim.setAsOutput(true);
      }
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
// Called from main() (boards/stm32f407/system.cpp).
extern "C" void setup() {
  platform::clock::init();
  const FaultInfo fault = consumeFaultInfo();
  const bool fault_reset = fault.fault_reset;
  diagnostics::captureResetCause(fault.fault_reset, fault.code, fault.pc,
                                 fault.task);
  boardPeripheralsInit();

  // Resolve the boot gesture before anything else touches the button —
  // see boot_option.hpp. rosbot_xl only exposes PUSH_BUTTON1 as a
  // readable GPIO (PUSH_BUTTON2 is wired to MCU NRST).
  const BootOptionConfig boot_option_config = {
      .buttons = buttons_pins,
      .green_leds = green_leds,
  };
  const BootAction boot_action = resolveBootAction(boot_option_config);
  s_transport_diagnostic_requested =
      boot_action == BootAction::kChangeTransport;
  const bool imu_calibration_requested =
      boot_action == BootAction::kCalibrateImu;

  s_persistent = persistent_config::load();
  g_comm_mgr.setNamespaceDefault(s_persistent.ns);

  g_comm_mgr.init();
  // useAlt() already returns a fixed value decided by resolveBootAction()
  // above — no need for selectTransport()'s own ~1.5 s polling window on
  // top of it. timeout_ms=1 still lets its single check fire.
  g_comm_mgr.selectTransport(1);
  g_comm_mgr.configureNamespace();

  persistent_config::Config now{};
  std::strncpy(now.ns, g_comm_mgr.getNamespace(),
               persistent_config::kNamespaceMaxLen);
  now.ns[persistent_config::kNamespaceMaxLen - 1] = '\0';
  now.has_imu_calibration = s_persistent.has_imu_calibration;
  now.imu_calibration = s_persistent.imu_calibration;

  // Revision specific configuration
  board_revision.init();
  auto rev = board_revision.revision();
  setMaxMotorsCurrent(rev);
  const FanConfig& fan_config =
      (rev == Revision::V1_1) ? rev1_1_fan_config : rev1_2_fan_config;

  ethernet::begin(ETHERNET_CONFIG);
  ntc.init();
  g_fan.init(fan_config);

  const bool imu_ready = imu_bno055.init();
  if (imu_ready) {
    // The saved offsets go in after the window, and only if it failed:
    // loaded before it, they made the chip report gyro/accel/mag 3 at once,
    // so the window "succeeded" instantly and saved them again (HW, ROSbot
    // XL, 2026-11-20).
    bool calibrated_now = false;
    if (imu_calibration_requested) {
      ImuCalibrationOffsets captured{};
      if (imu_calibration_boot::run(imu_bno055, RED_LED, green_leds, captured,
                                    g_comm_mgr.debugSerial())) {
        now.has_imu_calibration = true;
        now.imu_calibration = captured;
        calibrated_now = true;
      }
    }
    if (now.has_imu_calibration && !calibrated_now) {
      imu_bno055.applyCalibrationOffsets(now.imu_calibration);
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

  g_mavlink_node.setNamespace(g_comm_mgr.getNamespace());
  g_mavlink_node.setDiagnosticSerial(g_comm_mgr.debugSerial(),
                                     g_comm_mgr.debugMutex());
  if (fault_reset) {
    g_mavlink_node.setBootNotice("MCU reset after a firmware fault");
  }
  g_mavlink_node.begin();

  // RTOS
  createQueues();
  createTasks();
  vTaskStartScheduler();
}
