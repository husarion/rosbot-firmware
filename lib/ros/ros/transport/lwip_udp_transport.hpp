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

#include <micro_ros_arduino.h>

// Event-driven micro-ROS UDP transport built directly on LwIP raw API.
// Replaces arduino_native_ethernet_udp_transport_*: read() blocks the calling
// task on a FreeRTOS stream buffer instead of polling EthernetUDP, so the
// uRos task spends quiet periods in the Blocked state and idle accumulates.
//
// Receive path: udp_recv() callback fires from LwIP's scheduler thread when
// an incoming UDP packet is parsed; it copies the payload into a stream
// buffer. read() blocks on xStreamBufferReceive with the requested timeout.
//
// Send path: udp_sendto() runs from the caller's thread (uRos). LwIP TX is
// already DMA-driven by the STM32 ETH driver, so write() is fire-and-forget.

#ifdef __cplusplus
extern "C" {
#endif

bool lwip_udp_transport_open(struct uxrCustomTransport* transport);
bool lwip_udp_transport_close(struct uxrCustomTransport* transport);
size_t lwip_udp_transport_write(struct uxrCustomTransport* transport,
                                const uint8_t* buf, size_t len, uint8_t* err);
size_t lwip_udp_transport_read(struct uxrCustomTransport* transport,
                               uint8_t* buf, size_t len, int timeout,
                               uint8_t* err);

#ifdef __cplusplus
}
#endif
