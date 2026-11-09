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

#include <cstring>

#include "version_text.hpp"

void setUp() {}
void tearDown() {}

void test_stops_at_nul_and_ignores_trailing_garbage() {
  const char raw[11] = {'v',  '1',    '.', '0',    '.', '1',
                        '\0', '\x7f', 'x', '\xff', 'y'};
  char out[16];
  sanitizeVersionText(raw, sizeof(raw), out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("v1.0.1", out);
}

void test_strips_trailing_newline() {
  const char raw[12] = {'v', '1', '.', '4', '\n', '\0', 0, 0, 0, 0, 0, 0};
  char out[16];
  sanitizeVersionText(raw, sizeof(raw), out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("v1.4", out);
}

void test_all_bytes_printable_is_truncated_to_destination() {
  const char raw[11] = {'v', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0'};
  char out[8];
  sanitizeVersionText(raw, sizeof(raw), out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("v123456", out);
}

void test_leading_garbage_gives_empty_string() {
  const char raw[4] = {'\xff', 'v', '1', '2'};
  char out[8];
  std::memset(out, 'z', sizeof(out));
  sanitizeVersionText(raw, sizeof(raw), out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("", out);
}

void test_zero_sized_destination_is_untouched() {
  char out[1] = {'q'};
  sanitizeVersionText("v1", 2, out, 0);
  TEST_ASSERT_EQUAL_CHAR('q', out[0]);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_stops_at_nul_and_ignores_trailing_garbage);
  RUN_TEST(test_strips_trailing_newline);
  RUN_TEST(test_all_bytes_printable_is_truncated_to_destination);
  RUN_TEST(test_leading_garbage_gives_empty_string);
  RUN_TEST(test_zero_sized_destination_is_untouched);
  return UNITY_END();
}
