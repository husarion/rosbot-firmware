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

#include "mavlink_serial_transport.hpp"

#include <STM32FreeRTOS.h>

bool MavlinkSerialTransport::open() {
  return cfg_.uart != nullptr && cfg_.uart->open(cfg_.baudrate);
}

void MavlinkSerialTransport::close() {
  if (cfg_.uart != nullptr) cfg_.uart->close();
}

size_t MavlinkSerialTransport::write(const uint8_t* buf, size_t len) {
  if (cfg_.uart == nullptr) return 0;
  return cfg_.uart->writeFrame(buf, len) ? len : 0;
}

// RX lands in the UART's circular DMA buffer; without a USART interrupt
// (owned by the Arduino core until migration step 9) the wait is a 1-tick
// poll of its counter.
size_t MavlinkSerialTransport::read(uint8_t* buf, size_t len,
                                    uint32_t timeout_ms) {
  if (cfg_.uart == nullptr || buf == nullptr || len == 0) return 0;

  TickType_t ticks_to_wait = (timeout_ms > 0) ? pdMS_TO_TICKS(timeout_ms) : 0;
  TimeOut_t timeout_state;
  vTaskSetTimeOutState(&timeout_state);

  size_t got = 0;
  while (got < len) {
    const size_t n = cfg_.uart->read(buf + got, len - got);
    if (n > 0) {
      got += n;
      continue;
    }
    if (ticks_to_wait == 0) break;
    if (xTaskCheckForTimeOut(&timeout_state, &ticks_to_wait) != pdFALSE) break;
    vTaskDelay(1);
  }
  return got;
}
