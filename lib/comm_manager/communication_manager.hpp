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

#include <HardwareSerial.h>

#include <cstring>
#include <functional>

enum class TransportType { kSerial, kEthernet };

struct SerialConfig {
  HardwareSerial* serial;
  uint32_t baudrate;
  uint8_t rxPin;
  uint8_t txPin;
  uint32_t timeout_ms;
  const char* name;
};

struct CommunicationManagerConfig {
  /// Primary transport: serial (Robot A) or ethernet (Robot B)
  TransportType primary_type = TransportType::kSerial;

  /// Used only when primary_type == kSerial
  SerialConfig primary_serial = {};

  /// Always present — debug output or fallback communication channel
  SerialConfig diagnostic_serial = {};

  /// If returns true during selection window → diagnostic serial becomes
  /// the ROS communication channel (and debug is disabled on it)
  std::function<bool()> useDiagnosticCondition = nullptr;

  /// Called once after diagnostic serial is chosen for communication
  std::function<void()> onDiagnosticSelected = nullptr;

  uint16_t check_interval_ms = 50;
  uint16_t resend_ready_interval_ms = 250;
  const char* ns_default = "";
};

class CommunicationManager {
 public:
  static constexpr size_t NS_MAX_LENGTH = 32;

  explicit CommunicationManager(CommunicationManagerConfig cfg) : cfg_(cfg) {}

  // ============== Lifecycle ==============

  void init() {
    initSerial(cfg_.diagnostic_serial);
    if (cfg_.primary_type == TransportType::kSerial) {
      initSerial(cfg_.primary_serial);
    }
  }

  /// @brief  Wait up to @p timeout_ms for diagnostic serial activity.
  ///         If detected → ROS transport = diagnostic serial, debug OFF.
  ///         If timeout  → ROS transport = primary (serial or ethernet), debug
  ///         ON.
  /// @return Pointer to SerialConfig when serial transport chosen;
  ///         nullptr when ethernet is the selected transport.
  const SerialConfig* selectTransport(uint32_t timeout_ms = 2000) {
    uint32_t start = millis();

    while ((millis() - start) < timeout_ms) {
      if (cfg_.useDiagnosticCondition && cfg_.useDiagnosticCondition()) {
        selected_type_ = TransportType::kSerial;
        selected_serial_ = &cfg_.diagnostic_serial;
        debug_available_ = false;  // diagnostic busy → no debug
        if (cfg_.onDiagnosticSelected) cfg_.onDiagnosticSelected();
        return selected_serial_;
      }
      delay(cfg_.check_interval_ms);
    }

    // Timeout — use primary transport
    selected_type_ = cfg_.primary_type;
    debug_available_ = true;  // diagnostic free → debug OK

    if (cfg_.primary_type == TransportType::kSerial) {
      selected_serial_ = &cfg_.primary_serial;
      return selected_serial_;
    }

    // Ethernet — no serial config to return
    selected_serial_ = nullptr;
    return nullptr;
  }

  /// Negotiate namespace over the active serial link (FW/NS/ACK handshake).
  /// For ethernet transport falls back to ns_default immediately.
  bool configureNamespace(uint16_t timeout_ms = 1000) {
    if (selected_type_ == TransportType::kSerial && selected_serial_) {
      if (waitForHostConfig(*selected_serial_->serial, timeout_ms)) {
        return true;
      }
    }
    // Ethernet or handshake timeout → use default
    strncpy(namespace_, cfg_.ns_default, NS_MAX_LENGTH);
    namespace_[NS_MAX_LENGTH - 1] = '\0';
    return true;
  }

  // ============== Accessors ==============

  TransportType selectedTransportType() const { return selected_type_; }
  bool isSerialTransport() const {
    return selected_type_ == TransportType::kSerial;
  }
  const SerialConfig* selectedSerialConfig() const { return selected_serial_; }

  /// @return true when diagnostic serial is free for debug output
  ///         (i.e. communication goes via primary transport, not diagnostic)
  bool hasDebugSerial() const { return debug_available_; }

  /// @return diagnostic HardwareSerial* if available for debug, nullptr
  /// otherwise
  HardwareSerial* debugSerial() {
    return debug_available_ ? cfg_.diagnostic_serial.serial : nullptr;
  }

  const char* getNamespace() const { return namespace_; }

 private:
  CommunicationManagerConfig cfg_;
  TransportType selected_type_ = TransportType::kSerial;
  const SerialConfig* selected_serial_ = nullptr;
  bool debug_available_ = false;
  char namespace_[NS_MAX_LENGTH] = {};

  // ============== Private helpers ==============

  void initSerial(const SerialConfig& cfg) {
    cfg.serial->setRx(cfg.rxPin);
    cfg.serial->setTx(cfg.txPin);
    cfg.serial->begin(cfg.baudrate);
    cfg.serial->setTimeout(cfg.timeout_ms);
  }

  bool waitForHostConfig(HardwareSerial& serial, uint32_t timeout_ms) {
    uint32_t start_time = millis();
    char buffer[NS_MAX_LENGTH] = {0};
    size_t idx = 0;
    bool got_line = false;
    uint32_t last_ready = 0;

    const char* fw_version = "0.0.0";
#if defined(FW_VERSION)
    fw_version = FW_VERSION;
#endif

    serial.printf("FW: %s\r\n", fw_version);
    serial.flush();
    last_ready = millis();

    while (millis() - start_time < timeout_ms && !got_line) {
      while (serial.available()) {
        char c = serial.read();
        if (c == '\n') {
          got_line = true;
          break;
        }
        if (c != '\r' && idx < NS_MAX_LENGTH - 1) {
          buffer[idx++] = c;
        }
      }
      if (millis() - last_ready >= cfg_.resend_ready_interval_ms) {
        serial.printf("FW: %s\r\n", fw_version);
        serial.flush();
        last_ready = millis();
      }
    }

    if (got_line && idx >= 3 && strncmp(buffer, "NS:", 3) == 0) {
      strncpy(namespace_, buffer + 3, NS_MAX_LENGTH);
      namespace_[NS_MAX_LENGTH - 1] = '\0';
      serial.println("ACK");
      serial.flush();
      return true;
    }
    return false;
  }
};

extern CommunicationManager g_comm_mgr;
