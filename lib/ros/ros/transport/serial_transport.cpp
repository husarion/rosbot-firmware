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

#include "serial_transport.hpp"

#include <Arduino.h>
#include <STM32FreeRTOS.h>

#include "communication_manager.hpp"

namespace {

const SerialConfig* getConfig(struct uxrCustomTransport* transport) {
  if (transport == nullptr || transport->args == nullptr) {
    return nullptr;
  }
  return static_cast<const SerialConfig*>(transport->args);
}

}  // namespace

bool serial_transport_open(struct uxrCustomTransport* transport) {
  const SerialConfig* cfg = getConfig(transport);
  if (cfg == nullptr || cfg->serial == nullptr) {
    return false;
  }

  cfg->serial->setRx(cfg->rxPin);
  cfg->serial->setTx(cfg->txPin);
  cfg->serial->setTimeout(cfg->timeout_ms);
  cfg->serial->begin(cfg->baudrate);

  return cfg->serial->operator bool();
}

bool serial_transport_close(struct uxrCustomTransport* transport) {
  const SerialConfig* cfg = getConfig(transport);
  if (cfg == nullptr || cfg->serial == nullptr) {
    return false;
  }

  cfg->serial->end();
  return true;
}

size_t serial_transport_write(struct uxrCustomTransport* transport,
                              const uint8_t* buf, size_t len,
                              uint8_t* /*errcode*/) {
  const SerialConfig* cfg = getConfig(transport);
  if (cfg == nullptr || cfg->serial == nullptr || buf == nullptr) {
    return 0;
  }

  return cfg->serial->write(buf, len);
}

// Replaces Stream::readBytes (which busy-polls without yielding —
// burns CPU while waiting). We poll available() with vTaskDelay(1)
// gaps so the task is Blocked between checks; idle time accumulates
// for other tasks instead of being eaten by the RX wait.
size_t serial_transport_read(struct uxrCustomTransport* transport, uint8_t* buf,
                             size_t len, int timeout, uint8_t* /*errcode*/) {
  const SerialConfig* cfg = getConfig(transport);
  if (cfg == nullptr || cfg->serial == nullptr || buf == nullptr || len == 0) {
    return 0;
  }

  HardwareSerial* serial = cfg->serial;

  TickType_t ticks_to_wait =
      (timeout > 0) ? pdMS_TO_TICKS(static_cast<uint32_t>(timeout)) : 0;

  TimeOut_t timeout_state;
  vTaskSetTimeOutState(&timeout_state);

  size_t got = 0;

  while (got < len) {
    const int avail = serial->available();

    if (avail > 0) {
      size_t to_read = static_cast<size_t>(avail);
      if (to_read > (len - got)) {
        to_read = len - got;
      }

      for (size_t i = 0; i < to_read; ++i) {
        const int b = serial->read();
        if (b < 0) {
          break;
        }
        buf[got++] = static_cast<uint8_t>(b);
      }

      continue;
    }

    if (ticks_to_wait == 0) {
      break;
    }

    if (xTaskCheckForTimeOut(&timeout_state, &ticks_to_wait) != pdFALSE) {
      break;
    }

    vTaskDelay(1);
  }

  return got;
}
