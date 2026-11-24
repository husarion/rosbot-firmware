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

#include <unity.h>

#include <cmath>
#include <cstdio>
#include <deque>
#include <string>
#include <vector>

#include "power_board.cpp"

// platform::Uart stand-in: the test feeds RX bytes and reads back TX.
namespace {
std::deque<uint8_t> s_rx;
std::string s_tx;
uint32_t s_now_ms = 0;
}  // namespace

namespace platform {
bool Uart::open(uint32_t) { return true; }
size_t Uart::available() const { return s_rx.size(); }
int Uart::read() {
  if (s_rx.empty()) return -1;
  const int c = s_rx.front();
  s_rx.pop_front();
  return c;
}
void Uart::write(const uint8_t* data, size_t len) {
  s_tx.append(reinterpret_cast<const char*>(data), len);
}
}  // namespace platform

uint64_t time_boot_us() { return static_cast<uint64_t>(s_now_ms) * 1000U; }

namespace {

platform::Uart s_uart(platform::UartConfig{
    nullptr, {platform::Port::D, 5}, {platform::Port::D, 6}, 7, 64});

// Independent encoder for frames the power board sends: "<CC SS args.. XX>"
// in hex, XX = XOR of cmd, size and every arg.
std::string frame(uint8_t cmd, const std::vector<uint8_t>& args,
                  bool upper = false, int corrupt_checksum = 0) {
  uint8_t x = cmd ^ static_cast<uint8_t>(args.size());
  std::string out = "<";
  char hex[3];
  const char* fmt = upper ? "%02X" : "%02x";
  snprintf(hex, sizeof(hex), fmt, cmd);
  out += hex;
  snprintf(hex, sizeof(hex), fmt, static_cast<unsigned>(args.size()));
  out += hex;
  for (uint8_t a : args) {
    snprintf(hex, sizeof(hex), fmt, a);
    out += hex;
    x ^= a;
  }
  snprintf(hex, sizeof(hex), fmt, static_cast<uint8_t>(x ^ corrupt_checksum));
  out += hex;
  return out + ">";
}

std::vector<uint8_t> boardInfoArgs(const char* fw, const char* hw) {
  std::vector<uint8_t> args(25, 0);
  for (size_t i = 0; fw[i] != '\0' && i < 11; ++i) args[1 + i] = fw[i];
  for (size_t i = 0; hw[i] != '\0' && i < 12; ++i) args[13 + i] = hw[i];
  return args;
}

std::vector<uint8_t> batteryArgs(uint16_t mv, int16_t temp_c,
                                 uint16_t discharge_ma, uint16_t charge_ma,
                                 uint8_t status, uint8_t health,
                                 uint8_t present) {
  std::vector<uint8_t> a(18, 0);
  a[1] = mv >> 8;
  a[2] = mv & 0xFF;
  a[3] = static_cast<uint16_t>(temp_c) >> 8;
  a[4] = static_cast<uint16_t>(temp_c) & 0xFF;
  a[5] = discharge_ma >> 8;
  a[6] = discharge_ma & 0xFF;
  a[7] = charge_ma >> 8;
  a[8] = charge_ma & 0xFF;
  a[14] = status;
  a[15] = health;
  a[17] = present;
  return a;
}

void feed(const std::string& bytes) {
  for (char c : bytes) s_rx.push_back(static_cast<uint8_t>(c));
}

PowerBoard makeBoard() {
  return PowerBoard(PowerBoardConfig{s_uart, 38400, 3, 7.8f});
}

}  // namespace

void setUp() {
  s_rx.clear();
  s_tx.clear();
  s_now_ms = 1000;
}
void tearDown() {}

void test_requests_are_encoded_as_hex_frames() {
  auto pb = makeBoard();
  pb.requestBoardInfo();
  TEST_ASSERT_EQUAL_STRING("<01010000>", s_tx.c_str());
  s_tx.clear();
  pb.requestBatteryState();
  TEST_ASSERT_EQUAL_STRING(frame(2, std::vector<uint8_t>(18, 0)).c_str(),
                           s_tx.c_str());
}

void test_board_info_reply_sets_versions_and_fast_polling() {
  auto pb = makeBoard();
  TEST_ASSERT_FALSE(pb.supportsFastPolling());
  TEST_ASSERT_EQUAL_UINT32(PB_SLOW_POLL_PERIOD_MS, pb.batteryPollPeriodMs());

  feed(frame(1, boardInfoArgs("v1.0.3", "rev1.2\n")));
  pb.update();

  TEST_ASSERT_TRUE(pb.hasBoardInfoUpdate());
  TEST_ASSERT_FALSE(pb.hasBoardInfoUpdate());
  TEST_ASSERT_EQUAL_STRING("v1.0.3", pb.boardInfo().firmware_version);
  TEST_ASSERT_EQUAL_STRING("rev1.2", pb.boardInfo().board_version);
  TEST_ASSERT_TRUE(pb.supportsFastPolling());
  TEST_ASSERT_EQUAL_UINT32(0, pb.batteryPollPeriodMs());
}

void test_frame_split_across_updates_is_reassembled() {
  auto pb = makeBoard();
  const std::string f = frame(1, boardInfoArgs("v1.0.2", "r"));
  feed(f.substr(0, 20));
  pb.update();
  TEST_ASSERT_FALSE(pb.hasBoardInfoUpdate());
  feed(f.substr(20));
  pb.update();
  TEST_ASSERT_TRUE(pb.hasBoardInfoUpdate());
  TEST_ASSERT_EQUAL_STRING("v1.0.2", pb.boardInfo().firmware_version);
}

void test_noise_and_bad_checksum_are_skipped() {
  auto pb = makeBoard();
  feed("xx<zz>" + frame(1, boardInfoArgs("v9.9.9", "bad"), false, 0x01) +
       "\r\n" + frame(1, boardInfoArgs("v1.0.4", "good"), true));
  pb.update();
  TEST_ASSERT_TRUE(pb.hasBoardInfoUpdate());
  TEST_ASSERT_EQUAL_STRING("v1.0.4", pb.boardInfo().firmware_version);
  TEST_ASSERT_EQUAL_STRING("good", pb.boardInfo().board_version);
}

void test_full_buffer_of_garbage_does_not_block_later_frames() {
  auto pb = makeBoard();
  feed(std::string(200, 'z'));
  pb.update();
  pb.update();
  feed(frame(1, boardInfoArgs("v1.0.3", "x")));
  pb.update();
  TEST_ASSERT_TRUE(pb.hasBoardInfoUpdate());
}

void test_wrong_arg_count_is_ignored() {
  auto pb = makeBoard();
  feed(frame(2, std::vector<uint8_t>(17, 0)));
  pb.update();
  TEST_ASSERT_FALSE(pb.hasBatteryUpdate());
}

void test_battery_state_is_decoded() {
  auto pb = makeBoard();
  feed(frame(2, batteryArgs(12345, 25, 1500, 250, 2, 0, 1)));
  pb.update();

  TEST_ASSERT_TRUE(pb.hasBatteryUpdate());
  TEST_ASSERT_FALSE(pb.hasBatteryUpdate());
  const BatteryData d = pb.getData();
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 12.345f, d.voltage);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 25.0f, d.temperature);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, -1.25f, d.current);  // charge - discharge
  TEST_ASSERT_EQUAL_UINT8(2, d.status);
  TEST_ASSERT_TRUE(d.present);
  TEST_ASSERT_TRUE(d.percentage >= 0.0f && d.percentage <= 1.0f);
  // No board info yet: an old PB always sends 0, which is not "good".
  TEST_ASSERT_EQUAL_UINT8(POWER_SUPPLY_HEALTH_UNKNOWN, d.health);
}

void test_health_is_trusted_only_from_fast_polling_firmware() {
  auto pb = makeBoard();
  feed(frame(1, boardInfoArgs("v1.0.3", "r")));
  struct Case {
    uint8_t raw;
    uint8_t expected;
  };
  const Case cases[] = {{0, POWER_SUPPLY_HEALTH_GOOD},
                        {3, 3},
                        {POWER_SUPPLY_HEALTH_SAFETY_TIMER_EXPIRE,
                         POWER_SUPPLY_HEALTH_SAFETY_TIMER_EXPIRE},
                        {9, POWER_SUPPLY_HEALTH_UNKNOWN}};
  for (const auto& c : cases) {
    feed(frame(2, batteryArgs(12000, 20, 0, 0, 0, c.raw, 1)));
    pb.update();
    TEST_ASSERT_EQUAL_UINT8(c.expected, pb.getData().health);
  }
}

void test_implausible_voltage_drops_the_whole_frame() {
  auto pb = makeBoard();
  feed(frame(2, batteryArgs(12000, 20, 0, 0, 1, 0, 1)));
  pb.update();
  pb.hasBatteryUpdate();
  feed(frame(2, batteryArgs(3390, 30, 5000, 0, 2, 0, 1)));
  pb.update();
  TEST_ASSERT_FALSE(pb.hasBatteryUpdate());
  const BatteryData d = pb.getData();
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 12.0f, d.voltage);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 20.0f, d.temperature);
  TEST_ASSERT_EQUAL_UINT8(1, d.status);
}

void test_frame_without_battery_marks_it_absent() {
  auto pb = makeBoard();
  feed(frame(2, batteryArgs(12000, 20, 0, 0, 1, 0, 1)));
  pb.update();
  pb.hasBatteryUpdate();
  feed(frame(2, batteryArgs(0, 0, 0, 0, 1, 0, 0)));
  pb.update();
  TEST_ASSERT_TRUE(pb.hasBatteryUpdate());
  const BatteryData d = pb.getData();
  TEST_ASSERT_FALSE(d.present);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 12.0f, d.voltage);
  TEST_ASSERT_TRUE(std::isnan(d.percentage));
}

void test_out_of_range_temperature_reads_nan() {
  auto pb = makeBoard();
  feed(frame(2, batteryArgs(12000, 201, 0, 0, 0, 0, 1)));
  pb.update();
  TEST_ASSERT_TRUE(std::isnan(pb.getData().temperature));
  feed(frame(2, batteryArgs(12000, -101, 0, 0, 0, 0, 1)));
  pb.update();
  TEST_ASSERT_TRUE(std::isnan(pb.getData().temperature));
  feed(frame(2, batteryArgs(12000, -100, 0, 0, 0, 0, 1)));
  pb.update();
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, -100.0f, pb.getData().temperature);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_requests_are_encoded_as_hex_frames);
  RUN_TEST(test_board_info_reply_sets_versions_and_fast_polling);
  RUN_TEST(test_frame_split_across_updates_is_reassembled);
  RUN_TEST(test_noise_and_bad_checksum_are_skipped);
  RUN_TEST(test_full_buffer_of_garbage_does_not_block_later_frames);
  RUN_TEST(test_wrong_arg_count_is_ignored);
  RUN_TEST(test_battery_state_is_decoded);
  RUN_TEST(test_health_is_trusted_only_from_fast_polling_firmware);
  RUN_TEST(test_implausible_voltage_drops_the_whole_frame);
  RUN_TEST(test_frame_without_battery_marks_it_absent);
  RUN_TEST(test_out_of_range_temperature_reads_nan);
  return UNITY_END();
}
