#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <cstring>
#include <cstdint>

// ─── Config ──────────────────────────────────────────────
struct EepromConfig {
    TwoWire& i2c_bus;
    uint8_t   dev_id;
    uint8_t   page_size;
    uint8_t   write_delay_ms = 5;
};

// ─── Status ──────────────────────────────────────────────
enum class EepromStatus : uint8_t {
    Ok,
    WriteError,
    ReadError,
    InvalidArg
};

// ─── Class ───────────────────────────────────────────────
class Eeprom {
public:
    explicit Eeprom(const EepromConfig& cfg);

    EepromStatus writeByte(uint8_t block, uint8_t addr, uint8_t value);
    EepromStatus readByte(uint8_t block, uint8_t addr, uint8_t& value);

    EepromStatus writePage(uint8_t block, uint8_t addr,
                                        const uint8_t* data, uint8_t size);
    EepromStatus readPage(uint8_t block, uint8_t addr,
                                       uint8_t* data, uint8_t size);

private:
    TwoWire& i2c_bus_;
    uint8_t   dev_id_;
    uint8_t   page_size_;
    uint8_t   write_delay_ms_;

    constexpr uint8_t controlByte(uint8_t block) const {
        return dev_id_ | block;
    }
};

enum class Revision : uint8_t {
    Unknown,
    V1_1,
    V1_2,
};

// ── BoardRevisionConfig ───────────────────
struct BoardRevisionConfig {
    Eeprom&  eeprom;
    uint8_t  block        = 0x00;
    uint8_t  addr         = 0x00;
    uint8_t  max_length   = 4;      // maks. długość stringa "v1.2"
    uint8_t  retry_count  = 5;
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

    static Revision  parse(const char* str);

    // ── Lookup table: string ↔ enum ─────────────────────────
    struct Entry {
        const char* label;
        Revision    rev;
    };

    static constexpr Entry kRevisionTable[] = {
        {"v1.1", Revision::V1_1},
        {"v1.2", Revision::V1_2},
    };

    static constexpr uint8_t kTableSize =
        sizeof(kRevisionTable) / sizeof(kRevisionTable[0]);
};