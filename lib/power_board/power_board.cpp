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

#include "power_board.hpp"

// ═══════════════════════════════════════════════════════════════════
//  Constructor — stores config, does NOT touch hardware
// ═══════════════════════════════════════════════════════════════════

PowerBoard::PowerBoard(const PowerBoardConfig& config) : cfg_(config) {}

// ═══════════════════════════════════════════════════════════════════
//  init() — configures UART, call after HAL/clock init
// ═══════════════════════════════════════════════════════════════════

void PowerBoard::init() {
  cfg_.serial.begin(cfg_.baudrate);
  cfg_.serial.setTimeout(cfg_.timeout_ms);
}

// ═══════════════════════════════════════════════════════════════════
//  update() — read incoming data and parse frames
// ═══════════════════════════════════════════════════════════════════

void PowerBoard::update() {
  const size_t len = cfg_.serial.readBytes(rx_buf_, RX_BUF_SIZE);
  if (len == 0) return;

  Frame frame;
  size_t pos = 0;

  while (parseNextFrame(rx_buf_, len, pos, frame)) {
    dispatch(frame);
  }
}

// ═══════════════════════════════════════════════════════════════════
//  Requests
// ═══════════════════════════════════════════════════════════════════

void PowerBoard::requestBoardInfo() {
  const uint8_t args[1] = {0};
  sendFrame(CMD_BOARD_INFO, args, sizeof(args));
}

void PowerBoard::requestBatteryState() {
  uint8_t args[BATTERY_STATE_ARG_LEN] = {};
  sendFrame(CMD_BATTERY_STATE, args, sizeof(args));
}

// ═══════════════════════════════════════════════════════════════════
//  One-shot update flags
// ═══════════════════════════════════════════════════════════════════

bool PowerBoard::hasBatteryUpdate() {
  if (!battery_updated_ || data_.voltage <= 4.0f)
    return false;  // BUG: Sometimes PB sends ~3.39V
  battery_updated_ = false;
  return true;
}

bool PowerBoard::hasBoardInfoUpdate() {
  if (!board_info_updated_) return false;
  board_info_updated_ = false;
  return true;
}

// ═══════════════════════════════════════════════════════════════════
//  Hex conversion
// ═══════════════════════════════════════════════════════════════════

uint8_t PowerBoard::hexToNibble(uint8_t c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return 0xFF;
}

bool PowerBoard::hexToByte(const uint8_t* src, uint8_t& out) {
  const uint8_t hi = hexToNibble(src[0]);
  const uint8_t lo = hexToNibble(src[1]);
  if ((hi | lo) > 0x0F) return false;
  out = (hi << 4) | lo;
  return true;
}

void PowerBoard::byteToHex(uint8_t val, uint8_t* dst) {
  static constexpr char lut[] = "0123456789abcdef";
  dst[0] = lut[val >> 4];
  dst[1] = lut[val & 0x0F];
}

// ═══════════════════════════════════════════════════════════════════
//  Send frame
// ═══════════════════════════════════════════════════════════════════

void PowerBoard::sendFrame(uint8_t cmd, const uint8_t* args,
                           uint8_t arg_count) {
  uint8_t tx[MIN_FRAME + MAX_ARGS * 2];
  uint8_t* p = tx;

  uint8_t checksum = cmd ^ arg_count;

  *p++ = FRAME_START;
  byteToHex(cmd, p);
  p += 2;
  byteToHex(arg_count, p);
  p += 2;

  for (uint8_t i = 0; i < arg_count; ++i) {
    byteToHex(args[i], p);
    p += 2;
    checksum ^= args[i];
  }

  byteToHex(checksum, p);
  p += 2;
  *p++ = FRAME_END;

  cfg_.serial.write(tx, static_cast<size_t>(p - tx));
}

// ═══════════════════════════════════════════════════════════════════
//  Parse next frame from buffer
// ═══════════════════════════════════════════════════════════════════

bool PowerBoard::parseNextFrame(const uint8_t* data, size_t len, size_t& pos,
                                Frame& frame) {
  while (pos < len) {
    // Look for frame start marker
    if (data[pos] != FRAME_START) {
      ++pos;
      continue;
    }

    const size_t remaining = len - pos;
    if (remaining < MIN_FRAME) return false;

    const uint8_t* p = &data[pos + 1];

    if (!hexToByte(p, frame.cmd)) {
      ++pos;
      continue;
    }
    p += 2;

    if (!hexToByte(p, frame.arg_size)) {
      ++pos;
      continue;
    }
    p += 2;

    if (frame.arg_size > MAX_ARGS) {
      ++pos;
      continue;
    }

    // Check if buffer contains the full frame
    const size_t needed = MIN_FRAME + frame.arg_size * 2;
    if (remaining < needed) return false;

    // Decode args and accumulate checksum
    uint8_t xor_acc = frame.cmd ^ frame.arg_size;
    bool ok = true;

    for (uint8_t i = 0; i < frame.arg_size; ++i) {
      if (!hexToByte(p, frame.args[i])) {
        ok = false;
        break;
      }
      xor_acc ^= frame.args[i];
      p += 2;
    }
    if (!ok) {
      ++pos;
      continue;
    }

    if (!hexToByte(p, frame.checksum)) {
      ++pos;
      continue;
    }

    // Verify checksum
    if (xor_acc != frame.checksum) {
      ++pos;
      continue;
    }

    pos += needed;
    return true;
  }
  return false;
}

// ═══════════════════════════════════════════════════════════════════
//  Dispatch parsed frame to handler
// ═══════════════════════════════════════════════════════════════════

void PowerBoard::dispatch(const Frame& frame) {
  switch (frame.cmd) {
    case CMD_BOARD_INFO:
      handleBoardInfo(frame);
      break;
    case CMD_BATTERY_STATE:
      handleBatteryState(frame);
      break;
    default:
      break;
  }
}

// ═══════════════════════════════════════════════════════════════════
//  CMD 1 — Board info
//
//  args[ 1..11] -> firmware version (11 chars)
//  args[13..24] -> board version    (12 chars)
// ═══════════════════════════════════════════════════════════════════

void PowerBoard::handleBoardInfo(const Frame& frame) {
  if (frame.arg_size != BOARD_INFO_ARG_LEN) return;

  memset(&board_info_, 0, sizeof(board_info_));
  memcpy(board_info_.firmware_version, &frame.args[FIRMWARE_VERSION_OFFSET],
         FIRMWARE_VERSION_LEN);
  memcpy(board_info_.board_version, &frame.args[BOARD_VERSION_OFFSET],
         BOARD_VERSION_LEN);

  board_info_updated_ = true;
}

// ═══════════════════════════════════════════════════════════════════
//  CMD 2 — Battery state
//
//  args[ 1.. 2] -> voltage          (uint16, x0.001 V)
//  args[ 3.. 4] -> temperature      (int16,  deg C)
//  args[ 5.. 6] -> discharge current(uint16, x0.001 A)
//  args[ 7.. 8] -> charge current   (uint16, x0.001 A)
//  args[11..12] -> design capacity  (uint16, x0.001 Ah)
//  args[14]     -> status
//  args[15]     -> health
//  args[16]     -> technology
//  args[17]     -> present
// ═══════════════════════════════════════════════════════════════════

void PowerBoard::handleBatteryState(const Frame& frame) {
  if (frame.arg_size != BATTERY_STATE_ARG_LEN) return;

  const uint8_t* a = frame.args;

  // Base interface data
  data_.voltage = readU16(&a[1]) * 0.001f;

  const int16_t temp_raw = readS16(&a[3]);
  data_.temperature =
      (temp_raw > 200 || temp_raw < -100) ? NAN : static_cast<float>(temp_raw);

  const float discharge = readU16(&a[5]) * 0.001f;
  const float charge = readU16(&a[7]) * 0.001f;
  data_.current = charge - discharge;

  // Percentage not provided by this protocol
  if (cfg_.v_max > cfg_.v_min) {
    static float voltage_range_inv_ = 1.0f / (cfg_.v_max - cfg_.v_min);
    float pct = (data_.voltage - cfg_.v_min) * voltage_range_inv_;
    data_.percentage = fmaxf(0.0f, fminf(1.0f, pct));
  } else {
    data_.percentage = NAN;
  }

  data_.status = a[14];
  data_.health = a[15];
  data_.present = static_cast<bool>(a[17]);

  battery_updated_ = true;
}
