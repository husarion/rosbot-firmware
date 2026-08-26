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
  uint8_t has_imu_calibration;
  uint8_t pad[2];
  char ns[persistent_config::kNamespaceMaxLen];
  ImuCalibrationOffsets imu_calibration;
  uint32_t crc;
};
static_assert(sizeof(Record) % 4 == 0,
              "Record must be word-aligned for HAL_FLASH_Program");

// On-flash layout before IMU calibration was added — `magic` is
// unchanged, so a pre-upgrade record still passes that check, but its
// `crc` sat where `imu_calibration` now lives; without this fallback an
// already-deployed unit's saved backend/namespace would silently reset
// to defaults on first boot of this firmware, since the new Record's CRC
// reads erased flash (0xFF) as the "stored" checksum and (astronomically
// reliably) fails to match. Never written by this firmware — read-only,
// migration path in load() only.
struct LegacyRecord {
  uint32_t magic;
  uint8_t backend;
  uint8_t pad[3];
  char ns[persistent_config::kNamespaceMaxLen];
  uint32_t crc;
};
static_assert(sizeof(LegacyRecord) == 44,
              "legacy on-flash layout must not change — it's a migration "
              "target, not live storage");

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
      out.has_imu_calibration = stored->has_imu_calibration != 0;
      out.imu_calibration = stored->imu_calibration;
      s_cached = out;
      s_loaded = true;
      return out;
    }

    // Not a valid current-format record. Same magic (unchanged across
    // the format change) can also mean a pre-upgrade unit — check that
    // before falling back to full defaults, so an already-deployed
    // robot's backend/namespace survives this firmware update instead
    // of silently resetting.
    const auto* legacy = reinterpret_cast<const LegacyRecord*>(kStorageAddr);
    const uint32_t legacy_expected = crc32(
        reinterpret_cast<const uint8_t*>(legacy), offsetof(LegacyRecord, crc));
    if (legacy_expected == legacy->crc) {
      out.backend = static_cast<CommBackend>(legacy->backend);
      std::memcpy(out.ns, legacy->ns, kNamespaceMaxLen);
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
      std::memcmp(cfg.ns, s_cached.ns, kNamespaceMaxLen) == 0 &&
      cfg.has_imu_calibration == s_cached.has_imu_calibration &&
      std::memcmp(&cfg.imu_calibration, &s_cached.imu_calibration,
                  sizeof(ImuCalibrationOffsets)) == 0) {
    return;
  }

  Record record{};
  record.magic = kMagic;
  record.backend = static_cast<uint8_t>(cfg.backend);
  record.has_imu_calibration = cfg.has_imu_calibration ? 1 : 0;
  std::memcpy(record.ns, cfg.ns, kNamespaceMaxLen);
  record.ns[kNamespaceMaxLen - 1] = '\0';
  record.imu_calibration = cfg.imu_calibration;
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
