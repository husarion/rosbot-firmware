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

#include <array>
#include <cstddef>
#include <cstdint>

#include "comm_backend.hpp"

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
  TransportType primary_type = TransportType::kSerial;
  SerialConfig primary_serial = {};
  // Debug output, or the comm channel when useDiagnosticCondition fires.
  SerialConfig diagnostic_serial = {};

  // When this returns true during the selection window, the diagnostic
  // serial is promoted to the comm channel and debug output is silenced.
  // Function pointer (not std::function) to avoid the embedded heap path.
  bool (*useDiagnosticCondition)() = nullptr;
  void (*onDiagnosticSelected)() = nullptr;

  uint16_t check_interval_ms = 50;
  uint16_t resend_ready_interval_ms = 250;
  const char* ns_default = "";
  CommBackend backend_default = CommBackend::MAVLINK;
};

class CommunicationManager {
 public:
  static constexpr size_t NS_MAX_LENGTH = 32;

  explicit CommunicationManager(CommunicationManagerConfig cfg);

  // ============== Lifecycle ==============

  void init();

  // Returns the chosen SerialConfig, or nullptr when ethernet is primary.
  const SerialConfig* selectTransport(uint32_t timeout_ms = 1500);

  // Handshakes the namespace; falls back to ns_default on timeout / ethernet.
  void configureNamespace(uint16_t timeout_ms = 2500);

  TransportType selectedTransportType() const { return selected_type_; }
  bool isSerialTransport() const {
    return selected_type_ == TransportType::kSerial;
  }
  const SerialConfig* selectedSerialConfig() const { return selected_serial_; }

  // True when diagnostic serial is free for debug printing.
  bool hasDebugSerial() const { return debug_available_; }
  HardwareSerial* debugSerial();

  const char* getNamespace() const { return namespace_.data(); }
  CommBackend getSelectedBackend() const { return selected_backend_; }

  // ns_default must point to memory outliving this manager.
  void setBackendDefault(CommBackend b) {
    cfg_.backend_default = b;
    selected_backend_ = b;
  }
  void setNamespaceDefault(const char* ns) { cfg_.ns_default = ns; }

 private:
  void initSerial(const SerialConfig& cfg);
  bool waitForHostConfig(HardwareSerial& serial, uint32_t timeout_ms);
  bool parseAndStoreNamespace(HardwareSerial& serial, const char* buf,
                              size_t len);
  bool parseAndStoreBackend(HardwareSerial& serial, const char* buf,
                            size_t len);
  bool parseEnd(HardwareSerial& serial, const char* buf, size_t len);

  CommunicationManagerConfig cfg_;
  TransportType selected_type_ = TransportType::kSerial;
  const SerialConfig* selected_serial_ = nullptr;
  bool debug_available_ = false;
  std::array<char, NS_MAX_LENGTH> namespace_{};
  CommBackend selected_backend_;
};

extern CommunicationManager g_comm_mgr;
