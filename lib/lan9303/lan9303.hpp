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

#pragma once

#include <Arduino.h>

#include <cstdint>

// Port-based VLAN isolation for the on-board LAN9303 switch (rosbot_xl
// only): Port 0 = MCU, Port 1 = SBC, Port 2 = external RJ45. Factory
// default is a flat bridge across all three ports, so the SBC's static
// mgmt address (192.168.77.2) is reachable from whatever the external
// jack is plugged into — two robots on the same bench switch collide
// over that address (confirmed via a live ARP conflict). This puts
// Port 0+1 in one VLAN and Port 2 in another so 192.168.77.0/24 never
// leaves the robot.
//
// All register semantics are from the LAN9303/LAN9303i datasheet,
// Microchip DS00002308A: §10.2 (SMI extended addressing), §13.2.4.4-5
// (SWITCH_CSR_DATA/CMD indirection), §13.4.3.8-19 (VLAN/ingress CSRs).
// Reachable over the same MDIO/MDC pins STM32Ethernet already uses for
// PHY link status — see ethernetif.cpp's LAN9303_To_SMI_Address_Conv for
// the same address-conversion formula, already proven on this board for
// PHY_BSR/PHY_BCR.
class Lan9303 {
 public:
  enum class Status : uint8_t {
    kOk,
    kCsrTimeout,     // SWITCH_CSR_CMD busy bit never cleared
    kVlanTimeout,    // SWE_VLAN_CMD_STS operation-pending never cleared
    kVerifyFailed,   // a written register didn't read back as written
  };

  // Programs VLAN 1 = {Port0, Port1} and VLAN 2 = {Port2}, then enables
  // ingress VLAN enforcement. Call once, after Ethernet.begin() has
  // brought up the link (so the switch is confirmed out of reset and the
  // SMI/MDIO path is live).
  //
  // Safety invariant: Port0/Port1 membership in VLAN 1 is written and
  // verified *before* VLAN enforcement is turned on, and enforcement is
  // only enabled if every step verified. On any failure this returns
  // early leaving enforcement off — the switch stays in its default flat
  // -bridge behavior (today's status quo) rather than applying a
  // half-written VLAN table, so a partial failure can never cut STM32
  // off from the SBC.
  static Status isolateExternalPort();

 private:
  // Port indices as wired on this board (see ARCHITECTURE.md).
  static constexpr uint8_t kPortMcu = 0;
  static constexpr uint8_t kPortSbc = 1;
  static constexpr uint8_t kPortExternal = 2;

  static constexpr uint8_t kVlanSlotInternal = 0;  // VID 1: MCU + SBC
  static constexpr uint8_t kVlanSlotExternal = 1;  // VID 2: external jack
  static constexpr uint16_t kVidInternal = 1;
  static constexpr uint16_t kVidExternal = 2;

  // §10.2 SMI Slave Controller: PHY Address bit 4 is fixed 1 for SMI
  // (vs. plain MIIM), bits 3:0 carry system-register-byte-offset bits
  // 9:6; Register Address bits 4:0 carry byte-offset bits 5:1, with
  // Register Address bit 0 acting as the low/high 16-bit-word select
  // for the 32-bit register underneath.
  static uint16_t smiPhyAddr(uint16_t byte_offset);
  static uint16_t smiRegAddr(uint16_t byte_offset, bool high_word);

  // Raw access to a directly-addressable "System Control and Status"
  // register (e.g. SWITCH_CSR_DATA/CMD themselves) via two 16-bit SMI
  // transactions. Returns false if either HAL_ETH_Read/WritePHYRegister
  // call reports a bus timeout.
  static bool readSystemReg(uint16_t byte_offset, uint32_t& value);
  static bool writeSystemReg(uint16_t byte_offset, uint32_t value);

  // §13.2.4.4-5: indirect access to a Switch Fabric CSR (e.g.
  // SWE_VLAN_CMD, SWE_GLB_INGRESS_CFG) through SWITCH_CSR_DATA/CMD.
  // Returns false on a CSR_BUSY timeout.
  static bool readSwitchCsr(uint16_t csr_addr, uint32_t& value);
  static bool writeSwitchCsr(uint16_t csr_addr, uint32_t value);

  // §13.4.3.8-11: one VLAN-table or Port-VID-table transaction via
  // SWE_VLAN_CMD/WR_DATA, polling SWE_VLAN_CMD_STS for completion.
  static bool writeVlanEntry(uint8_t slot, uint16_t vid, bool member_mcu,
                             bool member_sbc, bool member_external);
  static bool writePortVid(uint8_t port, uint16_t vid);
  static bool vlanOpComplete();
};
