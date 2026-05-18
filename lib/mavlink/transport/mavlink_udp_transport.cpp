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

#include "mavlink_udp_transport.hpp"

#include <STM32FreeRTOS.h>
#include <stream_buffer.h>

extern "C" {
#include <lwip/ip_addr.h>
#include <lwip/pbuf.h>
#include <lwip/udp.h>
}

namespace {

// MAVLink v2 max frame is 280 B. Holds ~7 frames before drop. uRos task is
// expected to drain fast enough that this floor is never hit; if it is, we
// have a starvation bug to chase, not a queue-size to tune.
constexpr size_t kRxStreamSize = 2048;

struct udp_pcb* s_pcb = nullptr;
StreamBufferHandle_t s_rx_stream = nullptr;
ip_addr_t s_peer_addr;
uint16_t s_peer_port = 0;

// Runs on LwIP scheduler thread (not ISR). Plain stream-buffer send is OK.
void rxCallback(void* /*arg*/, struct udp_pcb* /*pcb*/, struct pbuf* p,
                const ip_addr_t* /*addr*/, u16_t /*port*/) {
  if (p == nullptr) return;
  for (struct pbuf* q = p; q != nullptr; q = q->next) {
    // timeout=0 — drop on overflow rather than block LwIP. MAVLink frames
    // missed here will be re-tried at the next telemetry tick anyway.
    xStreamBufferSend(s_rx_stream, q->payload, q->len, 0);
  }
  pbuf_free(p);
}

}  // namespace

bool MavlinkUdpTransport::open() {
  if (s_rx_stream == nullptr) {
    s_rx_stream = xStreamBufferCreate(kRxStreamSize, /*trigger_level=*/1);
    if (s_rx_stream == nullptr) return false;
  } else {
    xStreamBufferReset(s_rx_stream);
  }

  IP_ADDR4(&s_peer_addr, cfg_.peer_ip[0], cfg_.peer_ip[1], cfg_.peer_ip[2],
           cfg_.peer_ip[3]);
  s_peer_port = cfg_.peer_port;

  s_pcb = udp_new();
  if (s_pcb == nullptr) return false;

  if (udp_bind(s_pcb, IP4_ADDR_ANY, cfg_.local_port) != ERR_OK) {
    udp_remove(s_pcb);
    s_pcb = nullptr;
    return false;
  }

  udp_recv(s_pcb, rxCallback, nullptr);
  return true;
}

void MavlinkUdpTransport::close() {
  if (s_pcb != nullptr) {
    udp_remove(s_pcb);
    s_pcb = nullptr;
  }
}

size_t MavlinkUdpTransport::write(const uint8_t* buf, size_t len) {
  if (s_pcb == nullptr || buf == nullptr || len == 0) return 0;

  struct pbuf* p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
  if (p == nullptr) return 0;
  if (pbuf_take(p, buf, len) != ERR_OK) {
    pbuf_free(p);
    return 0;
  }

  err_t err = udp_sendto(s_pcb, p, &s_peer_addr, s_peer_port);
  pbuf_free(p);
  return (err == ERR_OK) ? len : 0;
}

size_t MavlinkUdpTransport::read(uint8_t* buf, size_t len,
                                 uint32_t timeout_ms) {
  if (s_rx_stream == nullptr || buf == nullptr || len == 0) return 0;
  return xStreamBufferReceive(s_rx_stream, buf, len, pdMS_TO_TICKS(timeout_ms));
}
