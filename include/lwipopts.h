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

// lwIP configuration for rosbot_xl. Values are those STM32Ethernet's
// lwipopts_default.h set when it owned the stack (before migration step 8),
// so the network behaves as before; trim them in a separate, HW-tested change.

#pragma once

// Bare-metal API driven from one task (see ethernet.cpp): not thread-safe.
#define NO_SYS 1
#define SYS_LIGHTWEIGHT_PROT 0
#define LWIP_NOASSERT

#define MEM_ALIGNMENT 4
#define MEM_SIZE (10 * 1024)
#define MEMP_NUM_PBUF 10
#define MEMP_NUM_UDP_PCB 6
#define MEMP_NUM_TCP_PCB 10
#define MEMP_NUM_TCP_PCB_LISTEN 6
#define MEMP_NUM_TCP_SEG 8
#define MEMP_NUM_SYS_TIMEOUT 10
#define PBUF_POOL_SIZE 8
#define PBUF_POOL_BUFSIZE 1524

#define LWIP_TCP 1
#define TCP_TTL 255
#define LWIP_SO_RCVTIMEO 1
#define LWIP_SO_RCVRCVTIMEO_NONSTANDARD 1
#define LWIP_SO_SNDTIMEO 1
#define LWIP_SO_SNDRCVTIMEO_NONSTANDARD 1
#define TCP_QUEUE_OOSEQ 0
#define TCP_MSS (1500 - 40)
#define TCP_SND_BUF (4 * TCP_MSS)
#define TCP_SND_QUEUELEN (2 * TCP_SND_BUF / TCP_MSS)
#define TCP_WND (3 * TCP_MSS)
#define LWIP_TCP_KEEPALIVE 1
#define LWIP_RANDOMIZE_INITIAL_LOCAL_PORTS 1

#define LWIP_ICMP 1
#define LWIP_RAW 1
#define DEFAULT_RAW_RECVMBOX_SIZE 3
#define LWIP_DHCP 1
#define LWIP_UDP 1
#define UDP_TTL 255
#define LWIP_STATS 0
#define LWIP_PROVIDE_ERRNO
#define LWIP_NETIF_HOSTNAME 1
#define LWIP_NETIF_STATUS_CALLBACK 1
#define LWIP_NETIF_LINK_CALLBACK 1
#define LWIP_DHCP_CHECK_LINK_UP 1
#define LWIP_NETCONN 0
#define LWIP_SOCKET 0
#define LWIP_DNS 1

// The F407 MAC computes and checks IP/UDP/TCP/ICMP checksums.
#define CHECKSUM_GEN_IP 0
#define CHECKSUM_GEN_UDP 0
#define CHECKSUM_GEN_TCP 0
#define CHECKSUM_CHECK_IP 0
#define CHECKSUM_CHECK_UDP 0
#define CHECKSUM_CHECK_TCP 0
#define CHECKSUM_GEN_ICMP 0

// Read by lib/ethernet/ethernetif.cpp.
#define ETHERNET_RMII_MODE_CONFIGURATION 1
