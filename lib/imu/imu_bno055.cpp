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

#include "imu_bno055.hpp"

#include <STM32FreeRTOS.h>
#include <wiring_constants.h>

namespace {

// BNO055 has acceleration, magnetometer, gyroscope, euler and quaternion
// data laid out contiguously starting at register 0x08. We DMA-read the
// whole 32-byte block in one transaction and ignore mag/euler bytes.
constexpr uint8_t kStartReg = 0x08;     // ACC_DATA_X_LSB
constexpr uint16_t kBlockLen = 32;      // 0x08..0x27 inclusive
constexpr uint16_t kReadTimeoutMs = 4;  // 32 B @ 400 kHz takes ~0.7 ms

// FreeRTOS-safe NVIC priority. Must be numerically >=
// configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY (5) so ISRs can call the
// *FromISR API.
constexpr uint32_t kIrqPriority = 5;

// STM32F4 DMA1 mapping for I2C RX. Reference: RM0090, Table 42.
struct DmaRxMap {
  DMA_Stream_TypeDef* stream;
  uint32_t channel;
  IRQn_Type irqn;
};

const DmaRxMap* findRxMap(I2C_TypeDef* inst) {
#ifdef I2C1
  static const DmaRxMap kI2C1Rx = {DMA1_Stream0, DMA_CHANNEL_1,
                                   DMA1_Stream0_IRQn};
  if (inst == I2C1) return &kI2C1Rx;
#endif
#ifdef I2C2
  static const DmaRxMap kI2C2Rx = {DMA1_Stream3, DMA_CHANNEL_7,
                                   DMA1_Stream3_IRQn};
  if (inst == I2C2) return &kI2C2Rx;
#endif
#ifdef I2C3
  static const DmaRxMap kI2C3Rx = {DMA1_Stream2, DMA_CHANNEL_3,
                                   DMA1_Stream2_IRQn};
  if (inst == I2C3) return &kI2C3Rx;
#endif
  return nullptr;
}

// DMA1 buffers must live in regular SRAM (DMA1 cannot reach CCM on F4).
// Aligning to 4 bytes is safe for the F4 DMA controller.
alignas(4) uint8_t s_buf[kBlockLen];

I2C_HandleTypeDef* s_hi2c = nullptr;
DMA_HandleTypeDef s_hdma_rx = {};
SemaphoreHandle_t s_done_sem = nullptr;
volatile bool s_xfer_ok = false;

inline int16_t le16(const uint8_t* p) {
  return static_cast<int16_t>(static_cast<uint16_t>(p[0]) |
                              (static_cast<uint16_t>(p[1]) << 8));
}

}  // namespace

extern "C" void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef* hi2c) {
  if (hi2c != s_hi2c || s_done_sem == nullptr) return;
  BaseType_t hpw = pdFALSE;
  s_xfer_ok = true;
  xSemaphoreGiveFromISR(s_done_sem, &hpw);
  portYIELD_FROM_ISR(hpw);
}

// On NACK/bus errors HAL invokes the framework's HAL_I2C_ErrorCallback in
// twi.c (strong symbol — cannot be overridden); the I2C state machine
// resets but the success semaphore is never given, so update() falls
// through on the xSemaphoreTake timeout below.

// Stream IRQ handlers for the streams we may bind to. Each guards against
// stray firings by checking that we own that stream.
extern "C" void DMA1_Stream0_IRQHandler() {
  if (s_hdma_rx.Instance == DMA1_Stream0) HAL_DMA_IRQHandler(&s_hdma_rx);
}
extern "C" void DMA1_Stream2_IRQHandler() {
  if (s_hdma_rx.Instance == DMA1_Stream2) HAL_DMA_IRQHandler(&s_hdma_rx);
}
extern "C" void DMA1_Stream3_IRQHandler() {
  if (s_hdma_rx.Instance == DMA1_Stream3) HAL_DMA_IRQHandler(&s_hdma_rx);
}

ImuBno055::ImuBno055(const ImuBno055Config& cfg)
    : cfg_(cfg), bno_(cfg.sensor_id, cfg.i2c_addr, cfg.bus) {}

bool ImuBno055::init() {
  if (!bno_.begin(OPERATION_MODE_NDOF)) {
    return false;
  }

  bno_.setAxisRemap(cfg_.axis_config);
  bno_.setAxisSign(cfg_.axis_sign);

  // false: neither board revision populates the XIN32/XOUT32 32kHz
  // crystal footprint (datasheet §5.5, pins 26/27) — asking for it would
  // still work (the chip self-checks the crystal signal on switch and
  // falls back to its ~±3% internal oscillator if none is found, per
  // §5.5.1/5.5.2), but costs ~600ms extra per §5.5.1 for a fallback that
  // was always going to happen anyway. This was `true` originally and
  // was NOT the cause of the calibration-freeze bug investigated
  // alongside this change (see ARCHITECTURE.md "IMU calibration" — that
  // was the I2C/DMA IRQ-priority conflict, unrelated to the clock
  // source); it's just correct now that we know there's no crystal.
  bno_.setExtCrystalUse(false);

  return true;
}

// Deliberately NOT part of init(): linking our DMA handle to s_hi2c and
// lowering the I2C event/error IRQ priority breaks Wire's own blocking
// transactions (HAL_I2C_Master_Receive_IT, used by Adafruit_BNO055's
// non-DMA calls — getCalibration/getSensorOffsets/setSensorOffsets).
// HW-verified: raw I2C reads succeed before this call, fail with a
// generic HAL error immediately after. Call this once, right before
// vTaskStartScheduler() — after boot-time IMU calibration (which needs
// those blocking calls) is done, and before update() is ever invoked
// from a task (its `s_done_sem == nullptr` guard makes calling update()
// before this a silent no-op, not a crash).
bool ImuBno055::enableDmaReads() {
  s_hi2c = cfg_.bus->getHandle();
  const DmaRxMap* map = findRxMap(s_hi2c->Instance);
  if (map == nullptr) {
    return false;
  }

  __HAL_RCC_DMA1_CLK_ENABLE();

  s_hdma_rx.Instance = map->stream;
  s_hdma_rx.Init.Channel = map->channel;
  s_hdma_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
  s_hdma_rx.Init.PeriphInc = DMA_PINC_DISABLE;
  s_hdma_rx.Init.MemInc = DMA_MINC_ENABLE;
  s_hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  s_hdma_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  s_hdma_rx.Init.Mode = DMA_NORMAL;
  s_hdma_rx.Init.Priority = DMA_PRIORITY_HIGH;
  s_hdma_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  s_hdma_rx.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
  s_hdma_rx.Init.MemBurst = DMA_MBURST_SINGLE;
  s_hdma_rx.Init.PeriphBurst = DMA_PBURST_SINGLE;

  if (HAL_DMA_Init(&s_hdma_rx) != HAL_OK) {
    return false;
  }
  __HAL_LINKDMA(s_hi2c, hdmarx, s_hdma_rx);

  HAL_NVIC_SetPriority(map->irqn, kIrqPriority, 0);
  HAL_NVIC_EnableIRQ(map->irqn);

  // Wire's I2C IRQs were configured by the framework at priority 2 (above
  // configMAX_SYSCALL_INTERRUPT_PRIORITY=5). Lower them so HAL's I2C IRQ
  // path can call FreeRTOS *FromISR API safely from our overrides.
  IRQn_Type ev_irq = (IRQn_Type)0;
  IRQn_Type er_irq = (IRQn_Type)0;
#ifdef I2C1
  if (s_hi2c->Instance == I2C1) {
    ev_irq = I2C1_EV_IRQn;
    er_irq = I2C1_ER_IRQn;
  }
#endif
#ifdef I2C2
  if (s_hi2c->Instance == I2C2) {
    ev_irq = I2C2_EV_IRQn;
    er_irq = I2C2_ER_IRQn;
  }
#endif
#ifdef I2C3
  if (s_hi2c->Instance == I2C3) {
    ev_irq = I2C3_EV_IRQn;
    er_irq = I2C3_ER_IRQn;
  }
#endif
  HAL_NVIC_SetPriority(ev_irq, kIrqPriority, 0);
  HAL_NVIC_SetPriority(er_irq, kIrqPriority, 0);

  s_done_sem = xSemaphoreCreateBinary();
  return s_done_sem != nullptr;
}

void ImuBno055::update() {
  if (s_done_sem == nullptr) return;

  s_xfer_ok = false;
  // Drain any stale completion left over from a previous timeout.
  xSemaphoreTake(s_done_sem, 0);

  if (HAL_I2C_Mem_Read_DMA(s_hi2c, cfg_.i2c_addr << 1, kStartReg,
                           I2C_MEMADD_SIZE_8BIT, s_buf, kBlockLen) != HAL_OK) {
    return;
  }

  if (xSemaphoreTake(s_done_sem, pdMS_TO_TICKS(kReadTimeoutMs)) != pdTRUE) {
    HAL_I2C_Master_Abort_IT(s_hi2c, cfg_.i2c_addr << 1);
    return;
  }
  if (!s_xfer_ok) return;

  // Acceleration: 1 LSB = 0.01 m/s²  (offsets 0..5)
  data_.acceleration[0] = le16(&s_buf[0]) / 100.0f;
  data_.acceleration[1] = le16(&s_buf[2]) / 100.0f;
  data_.acceleration[2] = le16(&s_buf[4]) / 100.0f;

  // Gyroscope: 1 LSB = 1/16 deg/s → rad/s  (offsets 12..17)
  constexpr float kGyroToRad = (1.0f / 16.0f) * DEG_TO_RAD;
  data_.angular_velocity[0] = le16(&s_buf[12]) * kGyroToRad;
  data_.angular_velocity[1] = le16(&s_buf[14]) * kGyroToRad;
  data_.angular_velocity[2] = le16(&s_buf[16]) * kGyroToRad;

  // Quaternion: w,x,y,z order in the register block, 1 LSB = 1/2^14
  // (offsets 24..31). ImuData stores x,y,z,w.
  constexpr float kQuatScale = 1.0f / (1 << 14);
  const float qw = le16(&s_buf[24]) * kQuatScale;
  const float qx = le16(&s_buf[26]) * kQuatScale;
  const float qy = le16(&s_buf[28]) * kQuatScale;
  const float qz = le16(&s_buf[30]) * kQuatScale;
  data_.orientation[0] = qx;
  data_.orientation[1] = qy;
  data_.orientation[2] = qz;
  data_.orientation[3] = qw;
}

ImuCalibrationStatus ImuBno055::getCalibrationStatus() {
  ImuCalibrationStatus status{};
  bno_.getCalibration(&status.system, &status.gyro, &status.accel, &status.mag);
  return status;
}

bool ImuBno055::captureCalibrationOffsets(ImuCalibrationOffsets& out) {
  adafruit_bno055_offsets_t offsets{};
  if (!bno_.getSensorOffsets(offsets)) {
    return false;
  }
  out.accel[0] = offsets.accel_offset_x;
  out.accel[1] = offsets.accel_offset_y;
  out.accel[2] = offsets.accel_offset_z;
  out.mag[0] = offsets.mag_offset_x;
  out.mag[1] = offsets.mag_offset_y;
  out.mag[2] = offsets.mag_offset_z;
  out.gyro[0] = offsets.gyro_offset_x;
  out.gyro[1] = offsets.gyro_offset_y;
  out.gyro[2] = offsets.gyro_offset_z;
  out.accel_radius = offsets.accel_radius;
  out.mag_radius = offsets.mag_radius;
  return true;
}

void ImuBno055::applyCalibrationOffsets(const ImuCalibrationOffsets& offsets) {
  adafruit_bno055_offsets_t bno_offsets{};
  bno_offsets.accel_offset_x = offsets.accel[0];
  bno_offsets.accel_offset_y = offsets.accel[1];
  bno_offsets.accel_offset_z = offsets.accel[2];
  bno_offsets.mag_offset_x = offsets.mag[0];
  bno_offsets.mag_offset_y = offsets.mag[1];
  bno_offsets.mag_offset_z = offsets.mag[2];
  bno_offsets.gyro_offset_x = offsets.gyro[0];
  bno_offsets.gyro_offset_y = offsets.gyro[1];
  bno_offsets.gyro_offset_z = offsets.gyro[2];
  bno_offsets.accel_radius = offsets.accel_radius;
  bno_offsets.mag_radius = offsets.mag_radius;
  bno_.setSensorOffsets(bno_offsets);
}

bool ImuBno055::probeCalibStatRaw(uint8_t& raw_byte, uint8_t& wire_error) {
  constexpr uint8_t kCalibStatReg = 0x35;
  raw_byte = 0;
  cfg_.bus->beginTransmission(cfg_.i2c_addr);
  cfg_.bus->write(kCalibStatReg);
  wire_error = cfg_.bus->endTransmission(false);  // repeated start, no stop
  if (wire_error != 0) return false;

  if (cfg_.bus->requestFrom(static_cast<uint8_t>(cfg_.i2c_addr),
                            static_cast<uint8_t>(1)) != 1) {
    wire_error = 0xFF;  // not a real Wire code — flags the requestFrom miss
    return false;
  }
  raw_byte = cfg_.bus->read();
  return true;
}
