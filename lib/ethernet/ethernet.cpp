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

#include "ethernet.hpp"

#include "ethernetif.h"
#include "freertos.hpp"
#include "lwip/dhcp.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/timeouts.h"
#include "netif/ethernet.h"
#include "time_base.hpp"

namespace {

// Same task shape and link-check period as STM32Ethernet's
// ethernet_scheduler, which this replaces.
constexpr uint32_t kLinkCheckMs = 500;
constexpr UBaseType_t kTaskPriority = tskIDLE_PRIORITY + 5;
constexpr uint16_t kTaskExtraStackWords = 200;

netif s_netif;

void taskLoop(void*) {
  uint32_t last_link_check = time_boot_ms();
  while (true) {
    vTaskDelay(1);
    ethernetif_input(&s_netif);
    if (time_boot_ms() - last_link_check >= kLinkCheckMs) {
      ethernetif_set_link(&s_netif);
      last_link_check = time_boot_ms();
    }
    sys_check_timeouts();
  }
}

}  // namespace

// Weak in ethernetif.cpp: follow the PHY link with the interface state.
extern "C" void ethernetif_notify_conn_changed(struct netif* netif) {
  if (netif_is_link_up(netif)) {
    netif_set_up(netif);
  } else {
    netif_set_down(netif);
  }
}

namespace ethernet {

bool begin(const Config& cfg) {
  lwip_init();
  ethernetif_set_mac_addr(cfg.mac);
  netif_add(&s_netif, &cfg.ip, &cfg.netmask, &cfg.gateway, nullptr,
            &ethernetif_init, &ethernet_input);
  netif_set_default(&s_netif);
  if (netif_is_link_up(&s_netif)) {
    netif_set_up(&s_netif);
  } else {
    netif_set_down(&s_netif);
  }
  netif_set_link_callback(&s_netif, ethernetif_update_config);
  // Tell a DHCP server on the LAN about the static address, so it does not
  // hand it out (what Ethernet.begin(mac, ip) did).
  dhcp_inform(&s_netif);

  return xTaskCreate(taskLoop, "ethernet_scheduler",
                     configMINIMAL_STACK_SIZE + kTaskExtraStackWords, nullptr,
                     kTaskPriority, nullptr) == pdPASS;
}

bool linkUp() { return netif_is_link_up(&s_netif); }

}  // namespace ethernet
