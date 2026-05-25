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

#include "mavlink_node.hpp"

#include <Arduino.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "publishers/publisher_interface.hpp"
#include "subscribers/subscriber_interface.hpp"

namespace {
constexpr uint8_t kChannel = MAVLINK_COMM_0;
constexpr size_t kMaxFrame = MAVLINK_MAX_PACKET_LEN;
constexpr size_t kLogBufSize = 64;
}  // namespace

// Folds the uint32 micros() delta into a uint64 accumulator so the value
// stays monotonic across the ~71 min wrap. Single-writer (uRos task).
uint64_t MavlinkNode::timeBootUs() {
  static uint32_t s_last_us = 0;
  static uint64_t s_accum_us = 0;
  const uint32_t now = micros();
  s_accum_us += static_cast<uint32_t>(now - s_last_us);
  s_last_us = now;
  return s_accum_us;
}

bool MavlinkNode::begin() {
  if (transport_open_) return true;
  if (tx_mutex_ == nullptr) {
    tx_mutex_ = xSemaphoreCreateMutex();
    if (tx_mutex_ == nullptr) return false;
  }
  transport_open_ = transport_.open();
  return transport_open_;
}

bool MavlinkNode::sendMessage(mavlink_message_t& msg) {
  if (!transport_open_) return false;
  uint8_t buf[kMaxFrame];
  uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
  if (tx_mutex_ == nullptr) return transport_.write(buf, len) == len;
  xSemaphoreTake(tx_mutex_, portMAX_DELAY);
  const size_t written = transport_.write(buf, len);
  xSemaphoreGive(tx_mutex_);
  return written == len;
}

void MavlinkNode::log(uint8_t severity, const char* fmt, ...) {
  char buf[kLogBufSize];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  if (diag_serial_ != nullptr) {
    diag_serial_->println(buf);
  }

  if (!transport_open_) return;
  // kLogBufSize stays ≤ 50 so the firmware string survives MAVLink's
  // STATUSTEXT truncation intact.
  mavlink_message_t m;
  mavlink_msg_statustext_pack(cfg_.sysid, cfg_.compid, &m, severity, buf,
                              /*id=*/0, /*chunk_seq=*/0);
  sendMessage(m);
}

void MavlinkNode::loop() {
  if (!transport_open_) {
    if (!begin()) return;
  }

  const uint32_t now = millis();

  emitHeartbeatIfDue(now);
  emitBootBannerIfDue(now);
  drainRx();

  switch (state_) {
    case WAITING:
      if (peer_seen_) {
        state_ = AWAIT_TIMESYNC;
        last_timesync_ms_ = 0;
      }
      break;

    case AWAIT_TIMESYNC:
      emitTimesyncIfDue(now);
      state_ = CONNECTED;
      break;

    case CONNECTED:
      emitTimesyncIfDue(now);
      if ((now - last_peer_heartbeat_ms_) > cfg_.peer_timeout_ms) {
        state_ = DISCONNECTED;
      } else {
        for (size_t i = 0; i < cfg_.pub_count; ++i) {
          cfg_.publishers[i]->publish(*this);
        }
      }
      break;

    case DISCONNECTED:
      peer_seen_ = false;
      boot_banner_sent_ = false;
      boot_banner_attempts_ = 0;
      state_ = WAITING;
      break;
  }
}

void MavlinkNode::emitHeartbeatIfDue(uint32_t now_ms) {
  if ((now_ms - last_heartbeat_ms_) < cfg_.heartbeat_period_ms &&
      last_heartbeat_ms_ != 0) {
    return;
  }
  last_heartbeat_ms_ = now_ms;
  mavlink_message_t m;
  uint8_t system_status =
      (state_ == CONNECTED) ? MAV_STATE_ACTIVE : MAV_STATE_STANDBY;
  mavlink_msg_heartbeat_pack(cfg_.sysid, cfg_.compid, &m, cfg_.mav_type,
                             cfg_.autopilot, /*base_mode=*/0,
                             /*custom_mode=*/0, system_status);
  sendMessage(m);
}

void MavlinkNode::emitBootBannerIfDue(uint32_t now_ms) {
  constexpr uint8_t kBannerAttempts = 10;
  if (boot_banner_sent_) return;
  if (state_ == CONNECTED) {
    boot_banner_sent_ = true;
    return;
  }
  if (last_heartbeat_ms_ == 0) return;
  if ((now_ms - last_boot_banner_ms_) < 1000 && last_boot_banner_ms_ != 0) {
    return;
  }
  last_boot_banner_ms_ = now_ms;
  log(MAV_SEVERITY_INFO, "%s", cfg_.boot_banner);
  boot_banner_attempts_++;
  if (boot_banner_attempts_ >= kBannerAttempts) boot_banner_sent_ = true;
}

void MavlinkNode::emitTimesyncIfDue(uint32_t now_ms) {
  const uint32_t period = (state_ == CONNECTED)
                              ? cfg_.timesync_period_ms
                              : cfg_.timesync_active_period_ms;
  if ((now_ms - last_timesync_ms_) < period && last_timesync_ms_ != 0) return;
  last_timesync_ms_ = now_ms;

  mavlink_message_t m;
  const int64_t ts1_ns = static_cast<int64_t>(timeBootUs() * 1000ULL);
  mavlink_msg_timesync_pack(cfg_.sysid, cfg_.compid, &m,
                            /*tc1=*/0, ts1_ns);
  sendMessage(m);
}

void MavlinkNode::drainRx() {
  uint8_t byte;
  while (transport_.read(&byte, 1, 0) == 1) {
    if (mavlink_parse_char(kChannel, byte, &rx_msg_, &rx_status_)) {
      dispatchMessage(rx_msg_);
    }
  }
}

void MavlinkNode::dispatchMessage(const mavlink_message_t& msg) {
  switch (msg.msgid) {
    case MAVLINK_MSG_ID_HEARTBEAT:
      last_peer_heartbeat_ms_ = millis();
      peer_seen_ = true;
      return;
    case MAVLINK_MSG_ID_TIMESYNC: {
      mavlink_timesync_t ts;
      mavlink_msg_timesync_decode(&msg, &ts);
      // tc1==0 marks a peer-initiated request — echo our clock back.
      if (ts.tc1 == 0) {
        mavlink_message_t reply;
        const int64_t tc1_ns = static_cast<int64_t>(timeBootUs() * 1000ULL);
        mavlink_msg_timesync_pack(cfg_.sysid, cfg_.compid, &reply, tc1_ns,
                                  ts.ts1);
        sendMessage(reply);
      }
      return;
    }
    default:
      break;
  }

  for (size_t i = 0; i < cfg_.sub_count; ++i) {
    if (cfg_.subscribers[i]->msgId() == msg.msgid) {
      cfg_.subscribers[i]->onMessage(msg, *this);
      return;
    }
  }
}
