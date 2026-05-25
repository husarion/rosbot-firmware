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
// Single MAVLink channel — only one MavlinkNode exists per firmware build.
constexpr uint8_t kChannel = MAVLINK_COMM_0;
// Largest MAVLink v2 frame.
constexpr size_t kMaxFrame = MAVLINK_MAX_PACKET_LEN;
// Bigger than any STATUSTEXT we emit, generously sized.
constexpr size_t kLogBufSize = 64;
}  // namespace

uint64_t MavlinkNode::timeBootUs() { return static_cast<uint64_t>(micros()); }

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
  // Mutex guards the transport against concurrent publish() calls. Publishers
  // run on the uRos task today so contention is zero, but the contract holds
  // if a future task posts a STATUSTEXT or wheel-stop from a different
  // context.
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
  mavlink_message_t m;
  // mavlink_msg_statustext_pack truncates text to 50 chars internally —
  // kLogBufSize stays under that to keep the firmware string == bridge
  // log line.
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
  // Banner emission is independent of state — a bridge that joins late
  // (within the kBannerAttempts × 1 s window) still gets to see it. After
  // that window the bridge auto-promotes, so reboots survive too.
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
      // Phase 2 fills this in — for the skeleton we move straight on once we
      // see any peer activity, which is enough for Phase 1's CI exit
      // criterion (HEARTBEAT visible).
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
      // Motor watchdog has already cut motors after 500 ms (D12). Reset
      // and wait for the peer to re-appear.
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
  // Emit the banner up to kBannerAttempts times at 1 s intervals after
  // boot. The bridge auto-promotes after a similar timeout (§10.2 +
  // pragmatic note), so a bridge that misses the window still works on
  // subsequent reconnects.
  constexpr uint8_t kBannerAttempts = 10;
  if (boot_banner_sent_) return;
  // CONNECTED means we exchanged HEARTBEATs both ways; banner was emitted
  // in the same loop iteration as our first HEARTBEAT, so the bridge has
  // already seen it (or will auto-promote via banner_grace_seconds_).
  // Keeps STATUSTEXT off the channel once the link is established.
  if (state_ == CONNECTED) {
    boot_banner_sent_ = true;
    return;
  }
  if (last_heartbeat_ms_ == 0) return;  // wait until first heartbeat lands
  if ((now_ms - last_boot_banner_ms_) < 1000 && last_boot_banner_ms_ != 0) {
    return;
  }
  last_boot_banner_ms_ = now_ms;
  log(MAV_SEVERITY_INFO, "%s", cfg_.boot_banner);
  boot_banner_attempts_++;
  if (boot_banner_attempts_ >= kBannerAttempts) boot_banner_sent_ = true;
}

void MavlinkNode::emitTimesyncIfDue(uint32_t now_ms) {
  // Phase 1: send periodic TIMESYNC requests so the bridge can warm its
  // offset filter early. We don't apply the reply on the firmware side —
  // bridge owns the wall-clock translation (D15).
  const uint32_t period = (state_ == CONNECTED)
                              ? cfg_.timesync_period_ms
                              : cfg_.timesync_active_period_ms;
  if ((now_ms - last_timesync_ms_) < period && last_timesync_ms_ != 0) return;
  last_timesync_ms_ = now_ms;

  mavlink_message_t m;
  // MCU-initiated: tc1=0, ts1=mcu_now_ns. Bridge echoes ts1 and fills tc1.
  const int64_t ts1_ns = static_cast<int64_t>(timeBootUs() * 1000ULL);
  mavlink_msg_timesync_pack(cfg_.sysid, cfg_.compid, &m,
                            /*tc1=*/0, ts1_ns);
  sendMessage(m);
}

void MavlinkNode::drainRx() {
  uint8_t byte;
  // Pull whatever the transport has buffered. The serial transport blocks
  // up to 1 ms internally and UDP returns immediately on empty; using a
  // 0 ms timeout keeps the spin tight either way.
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
      // If tc1==0 it's a request — echo our own clock back. Bridge does the
      // same when *it* receives our request; this branch only matters if
      // the bridge ever initiates one (current bridge does not).
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
