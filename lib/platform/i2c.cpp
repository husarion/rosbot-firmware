// Copyright 2026 Husarion sp. z o.o.
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

#include "i2c.hpp"

#include <initializer_list>

#include "clock.hpp"
#include "i2c_bus_clear.hpp"

namespace platform {

// DMA1 RX streams per RM0090 table 42, as the BNO055 DMA path used them
// (HW-verified on both boards).
struct I2cDmaStream {
  I2C_TypeDef* instance;
  DMA_Stream_TypeDef* stream;
  uint32_t channel;
  IRQn_Type stream_irq;
  IRQn_Type ev_irq;
  IRQn_Type er_irq;
};

namespace {

constexpr uint32_t kSpeedHz = 400000;
constexpr uint32_t kBlockingTimeoutMs = 10;
constexpr uint32_t kIrqPriority = 5;  // ISRs call *FromISR

const I2cDmaStream kStreams[] = {
    {I2C1, DMA1_Stream0, DMA_CHANNEL_1, DMA1_Stream0_IRQn, I2C1_EV_IRQn,
     I2C1_ER_IRQn},
    {I2C2, DMA1_Stream3, DMA_CHANNEL_7, DMA1_Stream3_IRQn, I2C2_EV_IRQn,
     I2C2_ER_IRQn},
    {I2C3, DMA1_Stream2, DMA_CHANNEL_3, DMA1_Stream2_IRQn, I2C3_EV_IRQn,
     I2C3_ER_IRQn},
};

I2c* s_owner[3] = {};

I2c* ownerOf(const I2C_HandleTypeDef* h) {
  for (I2c* bus : s_owner) {
    if (bus != nullptr && bus->handle() == h) return bus;
  }
  return nullptr;
}

void resetAndEnableClock(const I2C_TypeDef* instance) {
  if (instance == I2C1) {
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_I2C1_FORCE_RESET();
    __HAL_RCC_I2C1_RELEASE_RESET();
  } else if (instance == I2C2) {
    __HAL_RCC_I2C2_CLK_ENABLE();
    __HAL_RCC_I2C2_FORCE_RESET();
    __HAL_RCC_I2C2_RELEASE_RESET();
  } else {
    __HAL_RCC_I2C3_CLK_ENABLE();
    __HAL_RCC_I2C3_FORCE_RESET();
    __HAL_RCC_I2C3_RELEASE_RESET();
  }
}

bool schedulerRunning() {
  return xTaskGetSchedulerState() == taskSCHEDULER_RUNNING;
}

// Errors that mean the bus or the peripheral is stuck, not a device that
// simply did not answer (AF = NACK).
constexpr uint32_t kBusFaultErrors =
    HAL_I2C_ERROR_BERR | HAL_I2C_ERROR_ARLO | HAL_I2C_ERROR_TIMEOUT;

}  // namespace

// Holds the bus mutex for its scope; a no-op before the scheduler runs.
class I2c::Lock {
 public:
  explicit Lock(SemaphoreHandle_t mutex)
      : mutex_(schedulerRunning() ? mutex : nullptr) {
    if (mutex_ != nullptr) xSemaphoreTake(mutex_, portMAX_DELAY);
  }
  ~Lock() {
    if (mutex_ != nullptr) xSemaphoreGive(mutex_);
  }
  Lock(const Lock&) = delete;
  Lock& operator=(const Lock&) = delete;

 private:
  SemaphoreHandle_t mutex_;
};

bool I2c::init() {
  size_t slot = 0;
  for (; slot < 3 && kStreams[slot].instance != cfg_.instance; ++slot) {
  }
  if (slot == 3) return false;
  dma_ = &kStreams[slot];
  s_owner[slot] = this;

  if (mutex_ == nullptr) mutex_ = xSemaphoreCreateMutex();
  if (done_ == nullptr) done_ = xSemaphoreCreateBinary();
  if (mutex_ == nullptr || done_ == nullptr) return false;
  if (!configureHardware()) return false;

  HAL_NVIC_SetPriority(dma_->stream_irq, kIrqPriority, 0);
  HAL_NVIC_SetPriority(dma_->ev_irq, kIrqPriority, 0);
  HAL_NVIC_SetPriority(dma_->er_irq, kIrqPriority, 0);
  // Enabled only around IT/DMA transfers (armTransfer/awaitTransfer).
  setIrqsEnabled(false);
  return true;
}

void I2c::setIrqsEnabled(bool on) {
  for (IRQn_Type irq : {dma_->stream_irq, dma_->ev_irq, dma_->er_irq}) {
    if (on) {
      NVIC_ClearPendingIRQ(irq);
      NVIC_EnableIRQ(irq);
    } else {
      NVIC_DisableIRQ(irq);
    }
  }
}

bool I2c::configureHardware() {
  // Same pin setup as the core's pin map: AF open-drain, external pull-ups.
  cfg_.scl.setAsAlternateOpenDrain(cfg_.af);
  cfg_.sda.setAsAlternateOpenDrain(cfg_.af);
  resetAndEnableClock(cfg_.instance);

  // 2:1 duty: with PCLK1 = 42 MHz the 16:9 duty Arduino's setClock() picked
  // rounds CCR so the bus runs at 336 kHz; 2:1 divides evenly (CCR = 35,
  // exactly 400 kHz) and still meets fast-mode tLOW/tHIGH minimums.
  handle_.Instance = cfg_.instance;
  handle_.Init.ClockSpeed = kSpeedHz;
  handle_.Init.DutyCycle = I2C_DUTYCYCLE_2;
  handle_.Init.OwnAddress1 = 0;
  handle_.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  handle_.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  handle_.Init.OwnAddress2 = 0;
  handle_.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  handle_.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&handle_) != HAL_OK) return false;

  __HAL_RCC_DMA1_CLK_ENABLE();
  dma_rx_.Instance = dma_->stream;
  dma_rx_.Init.Channel = dma_->channel;
  dma_rx_.Init.Direction = DMA_PERIPH_TO_MEMORY;
  dma_rx_.Init.PeriphInc = DMA_PINC_DISABLE;
  dma_rx_.Init.MemInc = DMA_MINC_ENABLE;
  dma_rx_.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  dma_rx_.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  dma_rx_.Init.Mode = DMA_NORMAL;
  dma_rx_.Init.Priority = DMA_PRIORITY_HIGH;
  dma_rx_.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  if (HAL_DMA_Init(&dma_rx_) != HAL_OK) return false;
  __HAL_LINKDMA(&handle_, hdmarx, dma_rx_);
  return true;
}

// A device holding SDA/SCL, or the F4 peripheral's stuck-BUSY state, makes
// every later transfer fail until the peripheral is reset; on rosbot_xl
// that would take the EEPROM down with the IMU. Called with the bus lock
// held (or before the scheduler), so no other transfer can start meanwhile.
void I2c::recover() {
  setIrqsEnabled(false);
  HAL_DMA_Abort(&dma_rx_);  // HAL_ERROR when idle, harmless
  HAL_I2C_DeInit(&handle_);
  clearI2cBus(cfg_.scl, cfg_.sda, [] { clock::delayUs(5); });  // 100 kHz
  configureHardware();
  xSemaphoreTake(done_, 0);
  ++recoveries_;
}

bool I2c::finish(HAL_StatusTypeDef status) {
  if (status == HAL_OK) return true;
  last_error_ = HAL_I2C_GetError(&handle_);
  // HAL_BUSY: BUSY flag stuck or the handle left mid-transfer.
  if (status == HAL_BUSY || status == HAL_TIMEOUT ||
      (last_error_ & kBusFaultErrors) != 0) {
    if (status == HAL_BUSY) last_error_ |= HAL_I2C_ERROR_TIMEOUT;
    recover();
  }
  return false;
}

// Once the scheduler runs, every transfer goes through HAL's IT API and the
// task sleeps until it ends. The polled HAL calls have steps that must
// follow each other within a byte time (for a 1-byte read, STOP right after
// ADDR is cleared); a task switch in between corrupted VL53L0X transactions
// and left the sensors timing out (HW-seen on ROSbot 3). In IT mode those
// steps run in the I2C ISR. The polled calls stay for boot, before any task
// can preempt them.
bool I2c::writeReg(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len) {
  Lock lock(mutex_);
  auto* buf = const_cast<uint8_t*>(data);
  if (!schedulerRunning()) {
    return finish(HAL_I2C_Mem_Write(&handle_, addr << 1, reg,
                                    I2C_MEMADD_SIZE_8BIT, buf, len,
                                    kBlockingTimeoutMs));
  }
  armTransfer();
  return started(HAL_I2C_Mem_Write_IT(&handle_, addr << 1, reg,
                                      I2C_MEMADD_SIZE_8BIT, buf, len)) &&
         awaitTransfer(kBlockingTimeoutMs);
}

bool I2c::readReg(uint8_t addr, uint8_t reg, uint8_t* data, size_t len) {
  Lock lock(mutex_);
  if (!schedulerRunning()) {
    return finish(HAL_I2C_Mem_Read(&handle_, addr << 1, reg,
                                   I2C_MEMADD_SIZE_8BIT, data, len,
                                   kBlockingTimeoutMs));
  }
  armTransfer();
  return started(HAL_I2C_Mem_Read_IT(&handle_, addr << 1, reg,
                                     I2C_MEMADD_SIZE_8BIT, data, len)) &&
         awaitTransfer(kBlockingTimeoutMs);
}

bool I2c::writeThenRead(uint8_t addr, const uint8_t* out, size_t out_len,
                        uint8_t* in, size_t in_len) {
  Lock lock(mutex_);
  return transmit(addr, out, out_len) && receive(addr, in, in_len);
}

bool I2c::write(uint8_t addr, const uint8_t* data, size_t len) {
  Lock lock(mutex_);
  return transmit(addr, data, len);
}

bool I2c::transmit(uint8_t addr, const uint8_t* data, size_t len) {
  auto* buf = const_cast<uint8_t*>(data);
  if (!schedulerRunning()) {
    return finish(HAL_I2C_Master_Transmit(&handle_, addr << 1, buf, len,
                                          kBlockingTimeoutMs));
  }
  armTransfer();
  return started(HAL_I2C_Master_Transmit_IT(&handle_, addr << 1, buf, len)) &&
         awaitTransfer(kBlockingTimeoutMs);
}

bool I2c::receive(uint8_t addr, uint8_t* data, size_t len) {
  if (!schedulerRunning()) {
    return finish(HAL_I2C_Master_Receive(&handle_, addr << 1, data, len,
                                         kBlockingTimeoutMs));
  }
  armTransfer();
  return started(HAL_I2C_Master_Receive_IT(&handle_, addr << 1, data, len)) &&
         awaitTransfer(kBlockingTimeoutMs);
}

// Before every IT/DMA start: the ISR may complete before the caller waits.
// The bus IRQs stay off between transfers: with a device unpowered (BNO055
// on ROSbot 3 with IMU_POWER_ON low) the idle peripheral kept raising error
// interrupts and starved every task (HW-seen: the link went silent).
void I2c::armTransfer() {
  xSemaphoreTake(done_, 0);  // drop a completion left by an earlier timeout
  xfer_ok_ = false;
  irq_count_ = 0;
  storm_ = false;
  // Only a DMA read that completes clears this. Left set by one that timed
  // out, it makes HAL's start-bit handler turn on DMAEN for the next IT
  // write, whose TXE is then never served: an IRQ storm on every later read
  // (HW, ROSbot XL, 2026-11-20). Seq_Receive_DMA sets it again.
  dma_rx_.XferCpltCallback = nullptr;
  __HAL_I2C_CLEAR_FLAG(
      &handle_, I2C_FLAG_BERR | I2C_FLAG_ARLO | I2C_FLAG_AF | I2C_FLAG_OVR);
  setIrqsEnabled(true);
}

bool I2c::probe(uint8_t addr) {
  Lock lock(mutex_);
  return finish(
      HAL_I2C_IsDeviceReady(&handle_, addr << 1, 1, kBlockingTimeoutMs));
}

// Register write (IT) then repeated start and read (DMA): the same bus
// sequence as HAL_I2C_Mem_Read_DMA, but that one runs its address phase as a
// busy-wait in the caller, and the BNO055 stretches the clock there, which
// cost 230-450 us of CPU per IMU read (HW-measured with DWT).
bool I2c::readRegDma(uint8_t addr, uint8_t reg, uint8_t* data, size_t len,
                     uint32_t timeout_ms) {
  if (!schedulerRunning()) return readReg(addr, reg, data, len);
  Lock lock(mutex_);
  reg_ = reg;
  armTransfer();
  if (!started(HAL_I2C_Master_Seq_Transmit_IT(&handle_, addr << 1, &reg_, 1,
                                              I2C_FIRST_FRAME)) ||
      !awaitTransfer(timeout_ms)) {
    return false;
  }
  armTransfer();
  return started(HAL_I2C_Master_Seq_Receive_DMA(&handle_, addr << 1, data, len,
                                                I2C_LAST_FRAME)) &&
         awaitTransfer(timeout_ms);
}

// Result of an IT/DMA start call; on failure the IRQs go off again.
bool I2c::started(HAL_StatusTypeDef status) {
  if (status == HAL_OK) return true;
  setIrqsEnabled(false);
  return finish(status);
}

bool I2c::awaitTransfer(uint32_t timeout_ms) {
  const bool completed =
      xSemaphoreTake(done_, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
  setIrqsEnabled(false);
  if (!completed) {
    // HAL's abort calls are no use here (Master_Abort_IT only acts in
    // MODE_MASTER); a full reset leaves no DMA armed and the handle READY.
    last_error_ = HAL_I2C_ERROR_TIMEOUT;
    recover();
    return false;
  }
  if (!xfer_ok_) {
    last_error_ = HAL_I2C_GetError(&handle_);
    if (storm_) last_error_ |= HAL_I2C_ERROR_TIMEOUT;
    if ((last_error_ & kBusFaultErrors) != 0) recover();
  }
  return xfer_ok_;
}

// A transfer raises a handful of IRQs per byte at most. A device in a bad
// state can leave a flag HAL never clears, and the IRQ then re-enters
// forever, starving every task; past the budget the transfer is failed
// from here and the task resets the bus.
bool I2c::irqBudgetSpent() {
  if (++irq_count_ <= kIrqBudget) return false;
  setIrqsEnabled(false);
  storm_ = true;
  onTransferDone(false);
  return true;
}

void I2c::onIrqEvent() {
  if (!irqBudgetSpent()) HAL_I2C_EV_IRQHandler(&handle_);
}
void I2c::onIrqError() {
  if (!irqBudgetSpent()) HAL_I2C_ER_IRQHandler(&handle_);
}
void I2c::onDmaRx() { HAL_DMA_IRQHandler(&dma_rx_); }

void I2c::onTransferDone(bool ok) {
  xfer_ok_ = ok;
  BaseType_t woken = pdFALSE;
  xSemaphoreGiveFromISR(done_, &woken);
  portYIELD_FROM_ISR(woken);
}

}  // namespace platform

// HAL's completion callbacks are weak; these are the only definitions.
extern "C" void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef* h) {
  if (auto* bus = platform::ownerOf(h)) bus->onTransferDone(true);
}
extern "C" void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef* h) {
  if (auto* bus = platform::ownerOf(h)) bus->onTransferDone(true);
}
extern "C" void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef* h) {
  if (auto* bus = platform::ownerOf(h)) bus->onTransferDone(true);
}
extern "C" void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef* h) {
  if (auto* bus = platform::ownerOf(h)) bus->onTransferDone(true);
}
extern "C" void HAL_I2C_ErrorCallback(I2C_HandleTypeDef* h) {
  if (auto* bus = platform::ownerOf(h)) bus->onTransferDone(false);
}

#define PLATFORM_I2C_IRQS(n)                                              \
  extern "C" void I2C##n##_EV_IRQHandler(void) {                          \
    if (platform::s_owner[n - 1]) platform::s_owner[n - 1]->onIrqEvent(); \
  }                                                                       \
  extern "C" void I2C##n##_ER_IRQHandler(void) {                          \
    if (platform::s_owner[n - 1]) platform::s_owner[n - 1]->onIrqError(); \
  }
PLATFORM_I2C_IRQS(1)
PLATFORM_I2C_IRQS(2)
PLATFORM_I2C_IRQS(3)
#undef PLATFORM_I2C_IRQS

extern "C" void DMA1_Stream0_IRQHandler(void) {
  if (platform::s_owner[0]) platform::s_owner[0]->onDmaRx();
}
extern "C" void DMA1_Stream3_IRQHandler(void) {
  if (platform::s_owner[1]) platform::s_owner[1]->onDmaRx();
}
extern "C" void DMA1_Stream2_IRQHandler(void) {
  if (platform::s_owner[2]) platform::s_owner[2]->onDmaRx();
}
