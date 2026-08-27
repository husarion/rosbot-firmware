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

#include "lan9303.hpp"

// ETH_HandleTypeDef / HAL_ETH_Read|WritePHYRegister / the ETH peripheral
// instance all come in transitively via Arduino.h -> stm32f4xx_hal.h,
// gated on HAL_ETH_MODULE_ENABLED (set in hal_conf_custom.h for this
// board). No extra include needed, and no dependency on
// STM32Ethernet's private EthHandle: HAL_ETH_Read/WritePHYRegister only
// touch heth->Instance (always the same ETH peripheral) and
// heth->Init.PhyAddress/heth->State (both local to whatever handle we
// pass), so a throwaway handle built here is equivalent to the
// library's own — see stm32f4xx_hal_eth.c.

namespace {

// System Control and Status register byte offsets (datasheet §13.2.4).
constexpr uint16_t kSwitchCsrDataOffset = 0x1AC;
constexpr uint16_t kSwitchCsrCmdOffset = 0x1B0;

// SWITCH_CSR_CMD bit fields (§13.2.4.5).
constexpr uint32_t kCsrBusy = 1u << 31;
constexpr uint32_t kCsrRead = 1u << 30;    // R_nW: 1 = read, 0 = write
constexpr uint32_t kCsrByteEnableAll = 0xFu << 16;

// Switch Fabric CSR indices, i.e. values for SWITCH_CSR_CMD's
// CSR_ADDR[15:0] (Table 13-14).
constexpr uint16_t kSweVlanCmd = 0x180B;
constexpr uint16_t kSweVlanWrData = 0x180C;
constexpr uint16_t kSweVlanCmdSts = 0x1810;
constexpr uint16_t kSweGlbIngressCfg = 0x1840;
constexpr uint16_t kSwePortIngressCfg = 0x1841;

// SWE_GLB_INGRESS_CFG bits (§13.4.3.16).
constexpr uint32_t kVlanEnable = 1u << 0;
constexpr uint32_t k8021QVlanDisable = 1u << 15;  // force PVID-based rules

// SWE_PORT_INGRESS_CFG bits (§13.4.3.17): one "Enable Membership
// Checking" bit per port, bit N = port N.
constexpr uint32_t kMembershipCheckAllPorts = 0x7u << 0;

// SWE_VLAN_CMD bits (§13.4.3.8).
constexpr uint32_t kVlanCmdRead = 1u << 5;
constexpr uint32_t kVlanCmdPvidTable = 1u << 4;  // vs. cleared = VLAN table

constexpr uint32_t kCsrPollTimeoutMs = 20;

}  // namespace

uint16_t Lan9303::smiPhyAddr(uint16_t byte_offset) {
  // §10.2, Table 10-1 Note 10-1: PHY Address bit 4 is fixed 1 for SMI;
  // bits 3:0 carry system-register byte-offset bits 9:6.
  return 0x10 | ((byte_offset >> 6) & 0x0F);
}

uint16_t Lan9303::smiRegAddr(uint16_t byte_offset, bool high_word) {
  // Register Address bits 4:0 carry byte-offset bits 5:1; bit 0 of the
  // result is the explicit low/high word select (matches
  // ethernetif.cpp's LAN9303_To_SMI_Address_Conv exactly).
  uint16_t addr = (byte_offset >> 1) & 0x1F;
  return high_word ? (addr | 0x01) : (addr & ~0x01);
}

bool Lan9303::readSystemReg(uint16_t byte_offset, uint32_t& value) {
  ETH_HandleTypeDef heth{};
  heth.Instance = ETH;
  heth.Init.PhyAddress = smiPhyAddr(byte_offset);

  uint32_t lo = 0, hi = 0;
  if (HAL_ETH_ReadPHYRegister(&heth, smiRegAddr(byte_offset, false), &lo) !=
      HAL_OK) {
    return false;
  }
  if (HAL_ETH_ReadPHYRegister(&heth, smiRegAddr(byte_offset, true), &hi) !=
      HAL_OK) {
    return false;
  }
  value = ((hi & 0xFFFFu) << 16) | (lo & 0xFFFFu);
  return true;
}

bool Lan9303::writeSystemReg(uint16_t byte_offset, uint32_t value) {
  ETH_HandleTypeDef heth{};
  heth.Instance = ETH;
  heth.Init.PhyAddress = smiPhyAddr(byte_offset);

  if (HAL_ETH_WritePHYRegister(&heth, smiRegAddr(byte_offset, false),
                               value & 0xFFFFu) != HAL_OK) {
    return false;
  }
  if (HAL_ETH_WritePHYRegister(&heth, smiRegAddr(byte_offset, true),
                               (value >> 16) & 0xFFFFu) != HAL_OK) {
    return false;
  }
  return true;
}

bool Lan9303::readSwitchCsr(uint16_t csr_addr, uint32_t& value) {
  uint32_t cmd = kCsrBusy | kCsrRead | kCsrByteEnableAll | csr_addr;
  if (!writeSystemReg(kSwitchCsrCmdOffset, cmd)) return false;

  uint32_t status = 0;
  uint32_t start = millis();
  do {
    if (!readSystemReg(kSwitchCsrCmdOffset, status)) return false;
    if ((status & kCsrBusy) == 0) {
      return readSystemReg(kSwitchCsrDataOffset, value);
    }
  } while (millis() - start < kCsrPollTimeoutMs);
  return false;
}

bool Lan9303::writeSwitchCsr(uint16_t csr_addr, uint32_t value) {
  if (!writeSystemReg(kSwitchCsrDataOffset, value)) return false;

  uint32_t cmd = kCsrBusy | kCsrByteEnableAll | csr_addr;  // R_nW=0: write
  if (!writeSystemReg(kSwitchCsrCmdOffset, cmd)) return false;

  uint32_t status = 0;
  uint32_t start = millis();
  do {
    if (!readSystemReg(kSwitchCsrCmdOffset, status)) return false;
    if ((status & kCsrBusy) == 0) return true;
  } while (millis() - start < kCsrPollTimeoutMs);
  return false;
}

bool Lan9303::vlanOpComplete() {
  uint32_t start = millis();
  do {
    uint32_t sts = 0;
    if (!readSwitchCsr(kSweVlanCmdSts, sts)) return false;
    if ((sts & 0x1u) == 0) return true;  // Operation Pending cleared
  } while (millis() - start < kCsrPollTimeoutMs);
  return false;
}

bool Lan9303::writeVlanEntry(uint8_t slot, uint16_t vid, bool member_mcu,
                             bool member_sbc, bool member_external) {
  // §13.4.3.9 VLAN-table data layout: bit17/15/13 = member port 2/1/0,
  // bit16/14/12 = un-tag port 2/1/0 (left 0 — ports stay plain access
  // ports, see class comment), bits 11:0 = VID.
  uint32_t data = (static_cast<uint32_t>(vid) & 0xFFFu) |
                  (member_mcu ? (1u << 13) : 0) |
                  (member_sbc ? (1u << 15) : 0) |
                  (member_external ? (1u << 17) : 0);
  if (!writeSwitchCsr(kSweVlanWrData, data)) return false;

  // Write, VLAN table: bit5 (read) and bit4 (PVIDnVLAN) both cleared.
  uint32_t cmd = static_cast<uint32_t>(slot) & 0x0Fu;
  if (!writeSwitchCsr(kSweVlanCmd, cmd)) return false;
  return vlanOpComplete();
}

bool Lan9303::writePortVid(uint8_t port, uint16_t vid) {
  // §13.4.3.9 Port-VID-table data layout: bits 11:0 = default VID,
  // bits 14:12 = default priority (left 0).
  uint32_t data = static_cast<uint32_t>(vid) & 0xFFFu;
  if (!writeSwitchCsr(kSweVlanWrData, data)) return false;

  uint32_t cmd = kVlanCmdPvidTable | (static_cast<uint32_t>(port) & 0x0Fu);
  if (!writeSwitchCsr(kSweVlanCmd, cmd)) return false;
  return vlanOpComplete();
}

Lan9303::Status Lan9303::isolateExternalPort() {
  // Step 1: VLAN 1 = {MCU, SBC}, no Port 2. Written and read back before
  // anything is enforced — see the safety invariant in the header.
  if (!writeVlanEntry(kVlanSlotInternal, kVidInternal, /*mcu=*/true,
                      /*sbc=*/true, /*external=*/false)) {
    return Status::kVlanTimeout;
  }

  // Step 2: VLAN 2 = {external RJ45} only.
  if (!writeVlanEntry(kVlanSlotExternal, kVidExternal, /*mcu=*/false,
                      /*sbc=*/false, /*external=*/true)) {
    return Status::kVlanTimeout;
  }

  // Step 3: assign each port's default VID so untagged traffic (all of
  // it — nothing on this board sends 802.1Q tags) lands in the right
  // VLAN.
  if (!writePortVid(kPortMcu, kVidInternal) ||
      !writePortVid(kPortSbc, kVidInternal) ||
      !writePortVid(kPortExternal, kVidExternal)) {
    return Status::kVlanTimeout;
  }

  // Step 4: verify the table before enabling enforcement. Re-read both
  // VLAN slots by issuing a VLAN-table read command and checking
  // SWE_VLAN_RD_DATA.
  constexpr uint16_t kSweVlanRdData = 0x180E;
  auto verifySlot = [](uint8_t slot, uint32_t expected) -> bool {
    uint32_t cmd = kVlanCmdRead | (static_cast<uint32_t>(slot) & 0x0Fu);
    if (!writeSwitchCsr(kSweVlanCmd, cmd)) return false;
    if (!vlanOpComplete()) return false;
    uint32_t got = 0;
    if (!readSwitchCsr(kSweVlanRdData, got)) return false;
    return (got & 0x3FFFFu) == expected;
  };
  const uint32_t kExpectedInternal =
      kVidInternal | (1u << 13) | (1u << 15);
  const uint32_t kExpectedExternal = kVidExternal | (1u << 17);
  if (!verifySlot(kVlanSlotInternal, kExpectedInternal) ||
      !verifySlot(kVlanSlotExternal, kExpectedExternal)) {
    return Status::kVerifyFailed;
  }

  // Step 5: turn on enforcement. Only reached once every prior write
  // verified — see the safety invariant in the header.
  // SWE_PORT_INGRESS_CFG also carries the per-port "Enable Learning on
  // Ingress" bits (5:3, default 111b) — read-modify-write so enabling
  // membership checking doesn't silently disable MAC learning.
  uint32_t port_ingress_cfg = 0;
  if (!readSwitchCsr(kSwePortIngressCfg, port_ingress_cfg)) {
    return Status::kCsrTimeout;
  }
  if (!writeSwitchCsr(kSwePortIngressCfg,
                      port_ingress_cfg | kMembershipCheckAllPorts)) {
    return Status::kCsrTimeout;
  }
  // Same RMW discipline: SWE_GLB_INGRESS_CFG also carries unrelated QoS
  // defaults (e.g. "Use Precedence", "VL Higher Priority" both default
  // 1b) that a blind overwrite would silently zero.
  uint32_t glb_ingress_cfg = 0;
  if (!readSwitchCsr(kSweGlbIngressCfg, glb_ingress_cfg)) {
    return Status::kCsrTimeout;
  }
  if (!writeSwitchCsr(kSweGlbIngressCfg, glb_ingress_cfg | kVlanEnable |
                                             k8021QVlanDisable)) {
    return Status::kCsrTimeout;
  }

  return Status::kOk;
}
