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

constexpr uint32_t kSectorSize = 128 * 1024;
constexpr uint32_t kSlotCount = kSectorSize / sizeof(Record);

const Record* slot(uint32_t i) {
  return reinterpret_cast<const Record*>(kStorageAddr + i * sizeof(Record));
}

bool slotErased(uint32_t i) {
  const auto* words = reinterpret_cast<const uint32_t*>(slot(i));
  for (size_t w = 0; w < sizeof(Record) / 4; ++w) {
    if (words[w] != 0xFFFFFFFFu) return false;
  }
  return true;
}

bool slotValid(uint32_t i) {
  const Record* r = slot(i);
  return r->magic == kMagic && crc32(reinterpret_cast<const uint8_t*>(r),
                                     offsetof(Record, crc)) == r->crc;
}

bool legacyValid() {
  const auto* legacy = reinterpret_cast<const LegacyRecord*>(kStorageAddr);
  return legacy->magic == kMagic &&
         crc32(reinterpret_cast<const uint8_t*>(legacy),
               offsetof(LegacyRecord, crc)) == legacy->crc;
}

// Records are appended, never rewritten: the newest valid one wins, and the
// first fully erased slot is where the next one goes. Programming a slot
// costs microseconds, which is what makes a save legal once the scheduler
// runs — only a full sector (~1900 saves) needs the 1-3 s erase. A slot left
// half-programmed by a power loss fails its CRC and is skipped, so the
// previous record stays in charge.
struct Scan {
  int32_t newest = -1;
  int32_t next_free = -1;
};

Scan scan() {
  Scan out;
  for (uint32_t i = 0; i < kSlotCount; ++i) {
    if (slotErased(i)) {
      out.next_free = static_cast<int32_t>(i);
      break;
    }
    if (slotValid(i)) out.newest = static_cast<int32_t>(i);
  }
  return out;
}

bool s_loaded = false;
persistent_config::Config s_cached{};

bool programSlot(uint32_t i, const Record& record) {
  const auto* src = reinterpret_cast<const uint32_t*>(&record);
  const uint32_t base = kStorageAddr + i * sizeof(Record);
  for (size_t w = 0; w < sizeof(Record) / 4; ++w) {
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, base + w * 4, src[w]) !=
        HAL_OK) {
      return false;
    }
  }
  return slotValid(i);
}

}  // namespace

namespace persistent_config {

Config load() {
  Config out{};
  const Scan found = scan();
  if (found.newest >= 0) {
    const Record* stored = slot(found.newest);
    out.backend = static_cast<CommBackend>(stored->backend);
    std::memcpy(out.ns, stored->ns, kNamespaceMaxLen);
    out.ns[kNamespaceMaxLen - 1] = '\0';
    out.has_imu_calibration = stored->has_imu_calibration != 0;
    out.imu_calibration = stored->imu_calibration;
  } else if (legacyValid()) {
    // Pre-IMU-calibration layout, only ever at offset 0 — an
    // already-deployed robot's backend/namespace survives this update
    // instead of silently resetting.
    const auto* legacy = reinterpret_cast<const LegacyRecord*>(kStorageAddr);
    out.backend = static_cast<CommBackend>(legacy->backend);
    std::memcpy(out.ns, legacy->ns, kNamespaceMaxLen);
    out.ns[kNamespaceMaxLen - 1] = '\0';
  } else {
    out.backend = CommBackend::MAVLINK;
    out.ns[0] = '\0';
  }
  s_cached = out;
  s_loaded = true;
  return out;
}

bool save(const Config& cfg) {
  if (s_loaded && cfg.backend == s_cached.backend &&
      std::memcmp(cfg.ns, s_cached.ns, kNamespaceMaxLen) == 0 &&
      cfg.has_imu_calibration == s_cached.has_imu_calibration &&
      std::memcmp(&cfg.imu_calibration, &s_cached.imu_calibration,
                  sizeof(ImuCalibrationOffsets)) == 0) {
    return true;
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

  const Scan found = scan();
  const bool scheduler_running =
      xTaskGetSchedulerState() == taskSCHEDULER_RUNNING;
  // A legacy record occupies slot 0 without being a valid Record; appending
  // after it would leave load() unable to tell which is newer, so the first
  // save on such a unit rewrites the sector.
  const bool need_erase =
      found.next_free < 0 || (found.newest < 0 && found.next_free != 0);
  if (need_erase && scheduler_running) {
    // The sector erase stalls the instruction bus for 1-3 s, starving the
    // motor watchdog and MAVLink TX. The next boot can do it.
    return false;
  }

  HAL_FLASH_Unlock();
  // Stale error flags from an earlier operation make the next program
  // request fail outright (PGSERR), which would read as a flash fault.
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                         FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR |
                         FLASH_FLAG_PGSERR);
  bool ok = false;
  if (need_erase) {
    FLASH_EraseInitTypeDef erase{};
    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.Sector = kStorageSector;
    erase.NbSectors = 1;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    uint32_t sector_error = 0;
    ok = HAL_FLASHEx_Erase(&erase, &sector_error) == HAL_OK &&
         programSlot(0, record);
  } else {
    ok = programSlot(static_cast<uint32_t>(found.next_free), record);
  }
  HAL_FLASH_Lock();

  if (ok) s_cached = cfg;
  return ok;
}

bool saveImuCalibration(const ImuCalibrationOffsets& offsets) {
  Config cfg = s_loaded ? s_cached : load();
  cfg.has_imu_calibration = true;
  cfg.imu_calibration = offsets;
  return save(cfg);
}

bool hasImuCalibration() { return s_loaded && s_cached.has_imu_calibration; }

}  // namespace persistent_config
