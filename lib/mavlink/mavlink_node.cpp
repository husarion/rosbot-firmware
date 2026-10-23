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
#include "time_base.hpp"

namespace {
constexpr uint8_t kChannel = MAVLINK_COMM_0;
constexpr size_t kMaxFrame = MAVLINK_MAX_PACKET_LEN;
constexpr size_t kLogBufSize = 64;
}  // namespace

uint64_t MavlinkNode::timeBootUs() { return time_boot_us(); }

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
    const bool lock = diag_mutex_ != nullptr &&
                      xTaskGetSchedulerState() == taskSCHEDULER_RUNNING;
    if (lock) xSemaphoreTake(diag_mutex_, portMAX_DELAY);
    diag_serial_->println(buf);
    if (lock) xSemaphoreGive(diag_mutex_);
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
  drainRx();

  switch (state_) {
    case WAITING:
      if (peer_seen_) {
        state_ = CONNECTED;
        last_timesync_ms_ = 0;
        // Once per connection: the peer is listening now, and a restarted
        // bridge gets it again without an MCU reset.
        log(MAV_SEVERITY_INFO, "%s", cfg_.boot_banner);
        if (boot_notice_ != nullptr)
          log(MAV_SEVERITY_ERROR, "%s", boot_notice_);
      }
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
  uint8_t system_status = critical_               ? MAV_STATE_CRITICAL
                          : (state_ == CONNECTED) ? MAV_STATE_ACTIVE
                                                  : MAV_STATE_STANDBY;
  mavlink_msg_heartbeat_pack(cfg_.sysid, cfg_.compid, &m, cfg_.mav_type,
                             cfg_.autopilot, /*base_mode=*/0,
                             /*custom_mode=*/0, system_status);
  sendMessage(m);
}

void MavlinkNode::emitTimesyncIfDue(uint32_t now_ms) {
  if ((now_ms - last_timesync_ms_) < cfg_.timesync_period_ms &&
      last_timesync_ms_ != 0) {
    return;
  }
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

  // Every match, not the first: several commands share COMMAND_LONG and
  // each filters its own MAV_CMD_* inside onMessage().
  for (size_t i = 0; i < cfg_.sub_count; ++i) {
    if (cfg_.subscribers[i]->msgId() == msg.msgid) {
      cfg_.subscribers[i]->onMessage(msg, *this);
    }
  }
}
