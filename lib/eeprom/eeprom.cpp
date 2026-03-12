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

#include "eeprom.hpp"

#include <cstring>

// ─── Constructor ─────────────────────────────────────────

Eeprom::Eeprom(const EepromConfig& cfg)
    : i2c_bus_{cfg.i2c_bus},
      dev_id_{cfg.dev_id},
      page_size_{cfg.page_size},
      write_delay_ms_{cfg.write_delay_ms} {}

// ─── Write single byte ──────────────────────────────────

EepromStatus Eeprom::writeByte(uint8_t block, uint8_t addr, uint8_t value) {
  const uint8_t payload[] = {addr, value};

  i2c_bus_.beginTransmission(controlByte(block));
  const bool ok = i2c_bus_.write(payload, sizeof(payload)) == sizeof(payload);
  i2c_bus_.endTransmission();

  if (!ok) return EepromStatus::WriteError;

  delay(write_delay_ms_);
  return EepromStatus::Ok;
}

// ─── Read single byte ───────────────────────────────────

EepromStatus Eeprom::readByte(uint8_t block, uint8_t addr, uint8_t& value) {
  i2c_bus_.beginTransmission(controlByte(block));
  i2c_bus_.write(addr);
  i2c_bus_.endTransmission();

  i2c_bus_.requestFrom(controlByte(block), static_cast<uint8_t>(1));

  if (!i2c_bus_.available()) return EepromStatus::ReadError;

  const int raw = i2c_bus_.read();
  if (raw < 0) return EepromStatus::ReadError;

  value = static_cast<uint8_t>(raw);
  return EepromStatus::Ok;
}

// ─── Write page ──────────────────────────────────────────

EepromStatus Eeprom::writePage(uint8_t block, uint8_t addr, const uint8_t* data,
                               uint8_t size) {
  if (data == nullptr || size == 0 || size > page_size_) {
    return EepromStatus::InvalidArg;
  }

  const uint8_t total = size + 1;
  uint8_t payload[total];
  payload[0] = addr;
  std::memcpy(payload + 1, data, size);

  i2c_bus_.beginTransmission(controlByte(block));
  const bool ok = i2c_bus_.write(payload, total) == total;
  i2c_bus_.endTransmission();

  if (!ok) return EepromStatus::WriteError;

  delay(write_delay_ms_);
  return EepromStatus::Ok;
}

// ─── Read page ───────────────────────────────────────────

EepromStatus Eeprom::readPage(uint8_t block, uint8_t addr, uint8_t* data,
                              uint8_t size) {
  if (data == nullptr || size == 0) {
    return EepromStatus::InvalidArg;
  }

  i2c_bus_.beginTransmission(controlByte(block));
  i2c_bus_.write(addr);
  i2c_bus_.endTransmission();

  i2c_bus_.requestFrom(controlByte(block), static_cast<uint8_t>(size));

  if (!i2c_bus_.available()) return EepromStatus::ReadError;

  return (i2c_bus_.readBytes(data, size) == size) ? EepromStatus::Ok
                                                  : EepromStatus::ReadError;
}

// ── BoardRevision ───────────────────────────────────────
BoardRevision::BoardRevision(const BoardRevisionConfig cfg) : cfg_(cfg) {}

void BoardRevision::init() {
  char buf[cfg_.max_length + 1];

  for (uint8_t attempt = 0; attempt < cfg_.retry_count; ++attempt) {
    if (cfg_.eeprom.readPage(cfg_.block, cfg_.addr,
                             reinterpret_cast<uint8_t*>(buf),
                             cfg_.max_length) != EepromStatus::Ok) {
      continue;
    }

    buf[cfg_.max_length] = '\0';
    revision_ = parse(buf);

    if (revision_ != Revision::Unknown) return;
  }

  revision_ = Revision::Unknown;
}

Revision BoardRevision::parse(const char* str) {
  for (uint8_t i = 0; i < kTableSize; ++i) {
    if (std::strcmp(str, kRevisionTable[i].label) == 0) {
      return kRevisionTable[i].rev;
    }
  }
  return Revision::Unknown;
}

const char* BoardRevision::toString() const {
  for (uint8_t i = 0; i < kTableSize; ++i) {
    if (kRevisionTable[i].rev == revision_) {
      return kRevisionTable[i].label;
    }
  }
  return "unknown";
}
