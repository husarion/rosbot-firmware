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

#include "lwip_udp_transport.hpp"

#include <STM32FreeRTOS.h>
#include <stream_buffer.h>

extern "C" {
#include <lwip/ip_addr.h>
#include <lwip/pbuf.h>
#include <lwip/udp.h>
}

namespace {

// Holds at least 10 typical xrce-dds packets (~150-200 B each); LED strip
// frames + buttons + cmd are all comfortably below this size, so the queue
// only fills if uRos is starved for many ms.
constexpr size_t kRxStreamSize = 2048;

struct udp_pcb* s_pcb = nullptr;
StreamBufferHandle_t s_rx_stream = nullptr;
ip_addr_t s_agent_addr;
uint16_t s_agent_port = 0;

// Runs in LwIP scheduler-thread context (ethernet_schedu task), not ISR.
// Using xStreamBufferSend (not *FromISR) is correct here.
void rxCallback(void* /*arg*/, struct udp_pcb* /*pcb*/, struct pbuf* p,
                const ip_addr_t* /*addr*/, u16_t /*port*/) {
  if (p == nullptr) return;
  for (struct pbuf* q = p; q != nullptr; q = q->next) {
    // timeout=0 — drop bytes if the buffer is full rather than blocking the
    // LwIP thread; UDP is loss-tolerant and a starved uRos is the bigger
    // problem than a missed packet.
    xStreamBufferSend(s_rx_stream, q->payload, q->len, 0);
  }
  pbuf_free(p);
}

}  // namespace

bool lwip_udp_transport_open(struct uxrCustomTransport* transport) {
  auto* loc = static_cast<struct micro_ros_agent_locator*>(transport->args);

  if (s_rx_stream == nullptr) {
    s_rx_stream = xStreamBufferCreate(kRxStreamSize, /*trigger_level=*/1);
    if (s_rx_stream == nullptr) return false;
  } else {
    xStreamBufferReset(s_rx_stream);
  }

  IP_ADDR4(&s_agent_addr, loc->address[0], loc->address[1], loc->address[2],
           loc->address[3]);
  s_agent_port = static_cast<uint16_t>(loc->port);

  s_pcb = udp_new();
  if (s_pcb == nullptr) return false;

  // Local port = agent port — same convention as the original Arduino
  // transport (udp_client.begin(locator->port)). Replies from the agent
  // arrive on this port.
  if (udp_bind(s_pcb, IP4_ADDR_ANY, s_agent_port) != ERR_OK) {
    udp_remove(s_pcb);
    s_pcb = nullptr;
    return false;
  }

  udp_recv(s_pcb, rxCallback, nullptr);
  return true;
}

bool lwip_udp_transport_close(struct uxrCustomTransport* /*transport*/) {
  if (s_pcb != nullptr) {
    udp_remove(s_pcb);
    s_pcb = nullptr;
  }
  return true;
}

size_t lwip_udp_transport_write(struct uxrCustomTransport* /*transport*/,
                                const uint8_t* buf, size_t len,
                                uint8_t* /*errcode*/) {
  if (s_pcb == nullptr) return 0;

  struct pbuf* p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
  if (p == nullptr) return 0;

  if (pbuf_take(p, buf, len) != ERR_OK) {
    pbuf_free(p);
    return 0;
  }

  err_t err = udp_sendto(s_pcb, p, &s_agent_addr, s_agent_port);
  pbuf_free(p);
  return (err == ERR_OK) ? len : 0;
}

size_t lwip_udp_transport_read(struct uxrCustomTransport* /*transport*/,
                               uint8_t* buf, size_t len, int timeout,
                               uint8_t* /*errcode*/) {
  if (s_rx_stream == nullptr) return 0;
  return xStreamBufferReceive(s_rx_stream, buf, len, pdMS_TO_TICKS(timeout));
}
