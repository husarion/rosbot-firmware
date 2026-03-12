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

#include <cstdint>
#include <cstring>

#include "transport/transport.hpp"

/// Maximum number of LEDs supported (compile-time upper bound).
static constexpr uint8_t MAX_NUM_LEDS = 18;

// Message format for LED strip frames (for ROS topic subscription).
struct LedFrameMsg {
  uint8_t rgb_data[MAX_NUM_LEDS * 3];
  uint32_t pixel_count;
};

/// A pair of indices to swap in the remap table.
struct SwapPair {
  uint8_t a;
  uint8_t b;
};

/// Configuration for a pixel LED strip
struct LedStripConfig {
  uint8_t num_leds;

  const SwapPair* swaps;
  uint8_t swap_count;

  uint8_t init_r = 0;
  uint8_t init_g = 0;
  uint8_t init_b = 0;
};

/// APA102/SK9822 LED strip controller.
class LedStrip {
 public:
  LedStrip() = default;
  ~LedStrip() = default;

  /// Non-copyable (owns heap buffers).
  LedStrip(const LedStrip&) = delete;
  LedStrip& operator=(const LedStrip&) = delete;

  bool init(const LedStripConfig& config, Transport* transport);
  void show();

  // ═══ Whole-strip operations ═══════════════════════════════

  void setColor(uint8_t r, uint8_t g, uint8_t b);
  void setFromRGB8(const uint8_t* rgb_data, uint32_t pixel_count);
  void clear();

  // ═══ Per-LED operations ══════════════════════════════════

  bool setLED(uint8_t idx, uint8_t r, uint8_t g, uint8_t b);
  bool setBuffer(uint8_t idx, uint8_t r, uint8_t g, uint8_t b);

  // ═══ Getters ═════════════════════════════════════════════

  uint8_t size() const { return num_leds; }

 private:
  void sendStartFrame();
  void sendStopFrame();
  bool validIndex(uint8_t idx) const { return idx < num_leds; }
  uint8_t remap(uint8_t idx) const { return remap_[idx]; }
  uint8_t brightnessFromColor(uint8_t r, uint8_t g, uint8_t b) const;

  Transport* transport_ = nullptr;
  uint8_t num_leds = 0;

  uint8_t r_[MAX_NUM_LEDS] = {};
  uint8_t g_[MAX_NUM_LEDS] = {};
  uint8_t b_[MAX_NUM_LEDS] = {};
  uint8_t brightness_[MAX_NUM_LEDS] = {};
  uint8_t remap_[MAX_NUM_LEDS] = {};
};

extern LedStrip g_led_strip;
