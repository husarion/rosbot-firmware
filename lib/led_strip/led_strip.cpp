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

#include "led_strip.hpp"

bool LedStrip::init(const LedStripConfig& cfg, Transport* transport) {
  transport_ = transport;
  num_leds = cfg.num_leds;

  if (num_leds > MAX_NUM_LEDS || !transport_) return false;

  // ── Initialize transport ────────────────────────────────
  if (!transport_->init()) return false;

  // ── Identity remap ──────────────────────────────────────
  for (uint8_t i = 0; i < num_leds; ++i) {
    remap_[i] = i;
  }

  // ── Apply remap swaps ───────────────────────────────────
  for (uint8_t i = 0; i < cfg.swap_count; ++i) {
    uint8_t a = cfg.swaps[i].a;
    uint8_t b = cfg.swaps[i].b;
    if (a < num_leds && b < num_leds) {
      uint8_t tmp = remap_[a];
      remap_[a] = remap_[b];
      remap_[b] = tmp;
    }
  }

  // ── Apply initial color ─────────────────────────────────
  setColor(cfg.init_r, cfg.init_g, cfg.init_b);

  return true;
}

void LedStrip::sendStartFrame() {
  for (uint8_t i = 0; i < 4; ++i) transport_->transfer(0x00);
}

void LedStrip::sendStopFrame() {
  for (uint8_t i = 0; i < 4; ++i) transport_->transfer(0xFF);
}

void LedStrip::show() {
  sendStartFrame();
  for (uint8_t i = 0; i < num_leds; ++i) {
    transport_->transfer(brightness_[i] | 0xE0);
    transport_->transfer(b_[i]);
    transport_->transfer(g_[i]);
    transport_->transfer(r_[i]);
  }
  sendStopFrame();
}

void LedStrip::setColor(uint8_t r, uint8_t g, uint8_t b) {
  for (uint8_t i = 0; i < num_leds; ++i) {
    r_[i] = r;
    g_[i] = g;
    b_[i] = b;
    brightness_[i] = brightnessFromColor(r, g, b);
  }
}

void LedStrip::setFromRGB8(const uint8_t* rgb_data, uint32_t size) {
  uint32_t count = (size < num_leds) ? size : num_leds;
  for (uint32_t i = 0; i < count; i++) {
    uint8_t r = rgb_data[i * 3 + 0];
    uint8_t g = rgb_data[i * 3 + 1];
    uint8_t b = rgb_data[i * 3 + 2];
    uint8_t mapped = remap(i);
    r_[mapped] = r;
    g_[mapped] = g;
    b_[mapped] = b;
    brightness_[mapped] = brightnessFromColor(r, g, b);
  }
}

void LedStrip::clear() { setColor(0, 0, 0); }

bool LedStrip::setLED(uint8_t idx, uint8_t r, uint8_t g, uint8_t b) {
  if (!setBuffer(idx, r, g, b)) return false;
  show();
  return true;
}

bool LedStrip::setBuffer(uint8_t idx, uint8_t r, uint8_t g, uint8_t b) {
  if (!validIndex(idx)) return false;
  uint8_t mapped = remap(idx);
  r_[mapped] = r;
  g_[mapped] = g;
  b_[mapped] = b;
  brightness_[mapped] = brightnessFromColor(r, g, b);
  return true;
}

uint8_t LedStrip::brightnessFromColor(uint8_t r, uint8_t g, uint8_t b) const {
  uint8_t mx = r;
  if (g > mx) mx = g;
  if (b > mx) mx = b;
  // Map 0-255 → 0-31.  31 = 100% brightness.
  return static_cast<uint8_t>((static_cast<uint16_t>(mx) * 31u) / 255u);
}
