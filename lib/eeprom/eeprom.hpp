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

#pragma once

#include <Arduino.h>
#include <Wire.h>

#include <cstdint>
#include <cstring>

// ─── Config ──────────────────────────────────────────────
struct EepromConfig {
  TwoWire& i2c_bus;
  uint8_t dev_id;
  uint8_t page_size;
  uint8_t write_delay_ms = 5;
};

// ─── Status ──────────────────────────────────────────────
enum class EepromStatus : uint8_t { Ok, WriteError, ReadError, InvalidArg };

// ─── Class ───────────────────────────────────────────────
class Eeprom {
 public:
  explicit Eeprom(const EepromConfig& cfg);

  EepromStatus writeByte(uint8_t block, uint8_t addr, uint8_t value);
  EepromStatus readByte(uint8_t block, uint8_t addr, uint8_t& value);

  EepromStatus writePage(uint8_t block, uint8_t addr, const uint8_t* data,
                         uint8_t size);
  EepromStatus readPage(uint8_t block, uint8_t addr, uint8_t* data,
                        uint8_t size);

 private:
  TwoWire& i2c_bus_;
  uint8_t dev_id_;
  uint8_t page_size_;
  uint8_t write_delay_ms_;

  constexpr uint8_t controlByte(uint8_t block) const { return dev_id_ | block; }
};

enum class Revision : uint8_t {
  Unknown,
  V1_1,
  V1_2,
};

// ── BoardRevisionConfig ───────────────────
struct BoardRevisionConfig {
  Eeprom& eeprom;
  uint8_t block = 0x00;
  uint8_t addr = 0x00;
  uint8_t max_length = 4;
  uint8_t retry_count = 5;
};

// ── Class ───────────────────────
class BoardRevision {
 public:
  explicit BoardRevision(const BoardRevisionConfig cfg);

  void init();
  Revision revision() const { return revision_; }
  const char* toString() const;

 private:
  BoardRevisionConfig cfg_;
  Revision revision_ = Revision::Unknown;

  static Revision parse(const char* str);

  // ── Lookup table: string ↔ enum ─────────────────────────
  struct Entry {
    const char* label;
    Revision rev;
  };

  static inline constexpr Entry kRevisionTable[] = {
      {"v1.1", Revision::V1_1},
      {"v1.2", Revision::V1_2},
  };

  static inline constexpr uint8_t kTableSize =
      sizeof(kRevisionTable) / sizeof(kRevisionTable[0]);
};
