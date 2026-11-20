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

#pragma once

#include <cstddef>
#include <cstdint>

#include "freertos.hpp"
#include "pin.hpp"

namespace platform {

struct I2cDmaStream;

struct I2cConfig {
  I2C_TypeDef* instance;  // I2C1, I2C2 or I2C3
  Pin scl;
  Pin sda;
  uint8_t af;  // GPIO_AF4_I2Cx
};

// I2C master at 400 kHz on HAL. Every transfer holds the bus mutex once the
// scheduler runs, so tasks sharing a bus (rosbot_xl: IMU + EEPROM) cannot
// interleave. Addresses are 7-bit. A transfer that ends in a bus fault
// (timeout, BERR, ARLO, stuck BUSY; not a plain NACK) fails and then
// resets the peripheral and clears the bus, so the next one can work.
class I2c {
 public:
  explicit I2c(const I2cConfig& cfg) : cfg_(cfg) {}
  I2c(const I2c&) = delete;
  I2c& operator=(const I2c&) = delete;

  bool init();

  bool writeReg(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len);
  // Register address, repeated start, read.
  bool readReg(uint8_t addr, uint8_t reg, uint8_t* data, size_t len);
  // Write, STOP, then read: the Arduino Wire sequence some drivers rely on.
  bool writeThenRead(uint8_t addr, const uint8_t* out, size_t out_len,
                     uint8_t* in, size_t in_len);
  bool write(uint8_t addr, const uint8_t* data, size_t len);
  bool probe(uint8_t addr);

  // readReg() through DMA, for long blocks; every transfer already sleeps
  // instead of polling once the scheduler runs.
  // Falls back to readReg() before the scheduler runs. The buffer must be in
  // SRAM, not CCM (DMA1 cannot reach CCM).
  bool readRegDma(uint8_t addr, uint8_t reg, uint8_t* data, size_t len,
                  uint32_t timeout_ms);

  // HAL error bits of the last failed transfer (HAL_I2C_ERROR_*).
  uint32_t lastError() const { return last_error_; }
  // Bus resets after faults since boot.
  uint32_t recoveries() const { return recoveries_; }

  void onIrqEvent();
  void onIrqError();
  void onDmaRx();
  void onTransferDone(bool ok);
  I2C_HandleTypeDef* handle() { return &handle_; }

 private:
  class Lock;
  bool configureHardware();
  void setIrqsEnabled(bool on);
  void recover();
  bool finish(HAL_StatusTypeDef status);
  bool awaitTransfer(uint32_t timeout_ms);
  bool started(HAL_StatusTypeDef status);
  bool irqBudgetSpent();
  static constexpr uint32_t kIrqBudget = 200;
  void armTransfer();
  bool transmit(uint8_t addr, const uint8_t* data, size_t len);
  bool receive(uint8_t addr, uint8_t* data, size_t len);

  I2cConfig cfg_;
  const I2cDmaStream* dma_ = nullptr;
  I2C_HandleTypeDef handle_ = {};
  DMA_HandleTypeDef dma_rx_ = {};
  SemaphoreHandle_t mutex_ = nullptr;
  SemaphoreHandle_t done_ = nullptr;
  volatile bool xfer_ok_ = false;
  volatile uint32_t irq_count_ = 0;
  volatile bool storm_ = false;
  uint8_t reg_ = 0;  // register byte the IT transmit reads after return
  uint32_t last_error_ = 0;
  uint32_t recoveries_ = 0;
};

}  // namespace platform
