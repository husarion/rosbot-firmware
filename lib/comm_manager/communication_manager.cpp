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

#include "communication_manager.hpp"

#include <Arduino.h>

#include <cstring>

namespace {

const char* fwVersion() {
#if defined(FW_VERSION)
  return FW_VERSION;
#else
  return "0.0.0";
#endif
}

void sendVersionPrompt(HardwareSerial& s) {
  s.printf("FW: %s\r\n", fwVersion());
  s.flush();
}

// Returns true after consuming a newline; *idx then holds the line length.
bool consumeAvailable(HardwareSerial& s, char* buf, size_t* idx,
                      size_t max_len) {
  while (s.available()) {
    const char c = s.read();
    if (c == '\n') return true;
    if (c != '\r' && *idx < max_len - 1) {
      buf[(*idx)++] = c;
    }
  }
  return false;
}

}  // namespace

CommunicationManager::CommunicationManager(CommunicationManagerConfig cfg)
    : cfg_(cfg), selected_backend_(cfg.backend_default) {}

void CommunicationManager::init() {
  initSerial(cfg_.diagnostic_serial);
  if (cfg_.primary_type == TransportType::kSerial) {
    initSerial(cfg_.primary_serial);
  }
}

const SerialConfig* CommunicationManager::selectTransport(uint32_t timeout_ms) {
  const uint32_t start = millis();

  while ((millis() - start) < timeout_ms) {
    if (cfg_.useDiagnosticCondition && cfg_.useDiagnosticCondition()) {
      selected_type_ = TransportType::kSerial;
      selected_serial_ = &cfg_.diagnostic_serial;
      debug_available_ = false;
      if (cfg_.onDiagnosticSelected) cfg_.onDiagnosticSelected();
      return selected_serial_;
    }
    delay(cfg_.check_interval_ms);
  }

  selected_type_ = cfg_.primary_type;
  debug_available_ = true;

  if (cfg_.primary_type == TransportType::kSerial) {
    selected_serial_ = &cfg_.primary_serial;
    return selected_serial_;
  }

  selected_serial_ = nullptr;
  return nullptr;
}

void CommunicationManager::configureNamespace(uint16_t timeout_ms) {
  HardwareSerial* config_serial = nullptr;

  if (selected_type_ == TransportType::kEthernet) {
    config_serial = cfg_.diagnostic_serial.serial;
  } else if (selected_serial_ != nullptr) {
    config_serial = selected_serial_->serial;
  }

  if (config_serial != nullptr &&
      waitForHostConfig(*config_serial, timeout_ms)) {
    return;
  }

  std::strncpy(namespace_.data(), cfg_.ns_default, NS_MAX_LENGTH);
  namespace_[NS_MAX_LENGTH - 1] = '\0';
}

HardwareSerial* CommunicationManager::debugSerial() {
  return debug_available_ ? cfg_.diagnostic_serial.serial : nullptr;
}

void CommunicationManager::initSerial(const SerialConfig& cfg) {
  cfg.serial->setRx(cfg.rxPin);
  cfg.serial->setTx(cfg.txPin);
  cfg.serial->begin(cfg.baudrate);
  cfg.serial->setTimeout(cfg.timeout_ms);
}

bool CommunicationManager::waitForHostConfig(HardwareSerial& serial,
                                             uint32_t timeout_ms) {
  std::array<char, NS_MAX_LENGTH> buffer{};
  size_t idx = 0;
  const uint32_t start = millis();
  uint32_t last_prompt = millis();

  sendVersionPrompt(serial);

  // END terminates the handshake so BACKEND:/NS: may arrive in any order.
  // Legacy hosts that don't send END fall through on timeout instead.
  bool received_ns = false;
  while ((millis() - start) < timeout_ms) {
    if (consumeAvailable(serial, buffer.data(), &idx, NS_MAX_LENGTH)) {
      if (parseAndStoreBackend(serial, buffer.data(), idx)) {
        idx = 0;
        buffer.fill('\0');
        continue;
      }
      if (parseAndStoreNamespace(serial, buffer.data(), idx)) {
        received_ns = true;
        idx = 0;
        buffer.fill('\0');
        continue;
      }
      if (parseEnd(serial, buffer.data(), idx)) {
        return received_ns;
      }
      idx = 0;
      buffer.fill('\0');
    }
    if ((millis() - last_prompt) >= cfg_.resend_ready_interval_ms) {
      sendVersionPrompt(serial);
      last_prompt = millis();
    }
  }
  return received_ns;
}

bool CommunicationManager::parseAndStoreNamespace(HardwareSerial& serial,
                                                  const char* buf, size_t len) {
  constexpr const char* kPrefix = "NS:";
  constexpr size_t kPrefixLen = 3;

  if (len < kPrefixLen || std::strncmp(buf, kPrefix, kPrefixLen) != 0) {
    return false;
  }

  std::strncpy(namespace_.data(), buf + kPrefixLen, NS_MAX_LENGTH);
  namespace_[NS_MAX_LENGTH - 1] = '\0';

  serial.println("ACK");
  serial.flush();
  return true;
}

bool CommunicationManager::parseEnd(HardwareSerial& serial, const char* buf,
                                    size_t len) {
  constexpr const char* kEnd = "END";
  constexpr size_t kEndLen = 3;
  if (len != kEndLen || std::strncmp(buf, kEnd, kEndLen) != 0) {
    return false;
  }
  serial.println("ACK");
  serial.flush();
  return true;
}

bool CommunicationManager::parseAndStoreBackend(HardwareSerial& serial,
                                                const char* buf, size_t len) {
  constexpr const char* kPrefix = "BACKEND:";
  constexpr size_t kPrefixLen = 8;
  constexpr const char* kMavlink = "mavlink";
  constexpr const char* kMicroRos = "microros";

  if (len < kPrefixLen || std::strncmp(buf, kPrefix, kPrefixLen) != 0) {
    return false;
  }

  const char* value = buf + kPrefixLen;
  const size_t value_len = len - kPrefixLen;

  if (value_len == std::strlen(kMavlink) &&
      std::strncmp(value, kMavlink, value_len) == 0) {
    selected_backend_ = CommBackend::MAVLINK;
  } else if (value_len == std::strlen(kMicroRos) &&
             std::strncmp(value, kMicroRos, value_len) == 0) {
    selected_backend_ = CommBackend::MICRO_ROS;
  } else {
    serial.println("NAK");
    serial.flush();
    return true;
  }

  serial.println("ACK");
  serial.flush();
  return true;
}
