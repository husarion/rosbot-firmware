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

#include <cmath>
#include <cstring>

#include "battery_interface.hpp"

// ── Configuration ───────────────────────────────────────────────────

struct PowerBoardConfig {
  HardwareSerial& serial;
  uint32_t baudrate = 115200;
  uint32_t timeout_ms = 100;
  float v_min = 27.0f;  // minimum battery voltage [V]
  float v_max = 42.0f;  // maximum battery voltage [V]
};

// ── Class ───────────────────────────────────────────────────────────

class PowerBoard : public BatteryInterface {
 public:
  // ── Additional power board data ─────────────────────────────────

  struct BoardInfo {
    char firmware_version[16] = {};
    char board_version[16] = {};

    bool isValid() const { return firmware_version[0] != '\0'; }
  };

  // ── Constructor ─────────────────────────────────────────────────

  explicit PowerBoard(const PowerBoardConfig& config);

  // ── BatteryInterface ────────────────────────────────────────────

  void init() override;
  void update() override;
  const char* name() const override { return "PowerBoard"; }

  // ── PowerBoard-specific API ─────────────────────────────────────

  void requestBoardInfo();
  void requestBatteryState();

  const BoardInfo& boardInfo() const { return board_info_; }

  bool hasBoardInfoUpdate();
  bool hasBatteryUpdate();

 private:
  // ── Protocol constants ──────────────────────────────────────────

  static constexpr char FRAME_START = '<';
  static constexpr char FRAME_END = '>';
  static constexpr uint8_t MAX_ARGS = 32;
  static constexpr size_t RX_BUF_SIZE = 128;
  static constexpr size_t MIN_FRAME = 8;  // < CC SS CC >

  enum Cmd : uint8_t {
    CMD_BOARD_INFO = 1,
    CMD_BATTERY_STATE = 2,
  };

  static constexpr uint8_t BOARD_INFO_ARG_LEN = 25;
  static constexpr uint8_t BATTERY_STATE_ARG_LEN = 18;
  static constexpr uint8_t FIRMWARE_VERSION_LEN = 11;
  static constexpr uint8_t BOARD_VERSION_LEN = 12;
  static constexpr uint8_t FIRMWARE_VERSION_OFFSET = 1;
  static constexpr uint8_t BOARD_VERSION_OFFSET = 13;

  struct Frame {
    uint8_t cmd = 0;
    uint8_t arg_size = 0;
    uint8_t args[MAX_ARGS] = {};
    uint8_t checksum = 0;
  };

  // ── Hex encoding/decoding ───────────────────────────────────────

  static uint8_t hexToNibble(uint8_t c);
  static bool hexToByte(const uint8_t* src, uint8_t& out);
  static void byteToHex(uint8_t val, uint8_t* dst);

  // ── Frame operations ────────────────────────────────────────────

  void sendFrame(uint8_t cmd, const uint8_t* args, uint8_t arg_count);
  bool parseNextFrame(const uint8_t* data, size_t len, size_t& pos,
                      Frame& frame);
  void dispatch(const Frame& frame);

  void handleBoardInfo(const Frame& frame);
  void handleBatteryState(const Frame& frame);

  // ── Byte-order helpers ──────────────────────────────────────────

  static uint16_t readU16(const uint8_t* p) {
    return (static_cast<uint16_t>(p[0]) << 8) | p[1];
  }
  static int16_t readS16(const uint8_t* p) {
    return static_cast<int16_t>(readU16(p));
  }

  // ── Members ─────────────────────────────────────────────────────

  PowerBoardConfig cfg_;
  uint8_t rx_buf_[RX_BUF_SIZE] = {};

  BoardInfo board_info_;

  bool battery_updated_ = false;
  bool board_info_updated_ = false;
};
