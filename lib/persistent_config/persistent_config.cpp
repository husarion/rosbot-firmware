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

#include "persistent_config.hpp"

#include <STM32FreeRTOS.h>
#include <stm32f4xx_hal.h>

#include <cstddef>
#include <cstring>

namespace {

constexpr uint32_t kStorageAddr = 0x080E0000;  // STM32F407ZGT6 sector 11 base
constexpr uint32_t kStorageSector = FLASH_SECTOR_11;
constexpr uint32_t kMagic = 0x52424F54;  // 'RBOT'

struct Record {
  uint32_t magic;
  uint8_t backend;
  uint8_t pad[3];
  char ns[persistent_config::kNamespaceMaxLen];
  uint32_t crc;
};
static_assert(sizeof(Record) % 4 == 0,
              "Record must be word-aligned for HAL_FLASH_Program");

uint32_t crc32(const uint8_t* data, size_t len) {
  uint32_t crc = 0xFFFFFFFF;
  for (size_t i = 0; i < len; ++i) {
    crc ^= data[i];
    for (int j = 0; j < 8; ++j) {
      uint32_t mask = -(crc & 1u);
      crc = (crc >> 1) ^ (0xEDB88320u & mask);
    }
  }
  return ~crc;
}

bool s_loaded = false;
persistent_config::Config s_cached{};

}  // namespace

namespace persistent_config {

Config load() {
  const auto* stored = reinterpret_cast<const Record*>(kStorageAddr);
  Config out{};
  if (stored->magic == kMagic) {
    const uint32_t expected =
        crc32(reinterpret_cast<const uint8_t*>(stored), offsetof(Record, crc));
    if (expected == stored->crc) {
      out.backend = static_cast<CommBackend>(stored->backend);
      std::memcpy(out.ns, stored->ns, kNamespaceMaxLen);
      out.ns[kNamespaceMaxLen - 1] = '\0';
      s_cached = out;
      s_loaded = true;
      return out;
    }
  }
  out.backend = CommBackend::MAVLINK;
  out.ns[0] = '\0';
  s_cached = out;
  s_loaded = true;
  return out;
}

void save(const Config& cfg) {
  // Sector erase stalls the instruction bus for 1-3 s, exceeding the motor
  // watchdog and starving MAVLink TX. Caller must invoke before scheduler
  // start; see the header.
  configASSERT(xTaskGetSchedulerState() != taskSCHEDULER_RUNNING);

  if (s_loaded && cfg.backend == s_cached.backend &&
      std::memcmp(cfg.ns, s_cached.ns, kNamespaceMaxLen) == 0) {
    return;
  }

  Record record{};
  record.magic = kMagic;
  record.backend = static_cast<uint8_t>(cfg.backend);
  std::memcpy(record.ns, cfg.ns, kNamespaceMaxLen);
  record.ns[kNamespaceMaxLen - 1] = '\0';
  record.crc =
      crc32(reinterpret_cast<const uint8_t*>(&record), offsetof(Record, crc));

  HAL_FLASH_Unlock();

  FLASH_EraseInitTypeDef erase{};
  erase.TypeErase = FLASH_TYPEERASE_SECTORS;
  erase.Sector = kStorageSector;
  erase.NbSectors = 1;
  erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  uint32_t sector_error = 0;
  if (HAL_FLASHEx_Erase(&erase, &sector_error) == HAL_OK) {
    const auto* src = reinterpret_cast<const uint32_t*>(&record);
    for (size_t i = 0; i < sizeof(Record) / 4; ++i) {
      HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, kStorageAddr + i * 4, src[i]);
    }
    s_cached = cfg;
  }

  HAL_FLASH_Lock();
}

}  // namespace persistent_config
