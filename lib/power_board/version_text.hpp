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

#include <cstddef>

// The power board copies its version strings from fixed-size buffers, so a
// short string is followed by whatever bytes sat after it in flash, and the
// hardware revision may end in a newline. Keep the leading run of printable
// non-space ASCII and always NUL-terminate.
inline void sanitizeVersionText(const char* src, size_t src_len, char* dst,
                                size_t dst_size) {
  if (dst_size == 0) return;
  size_t n = 0;
  while (n < src_len && n + 1 < dst_size && src[n] >= 0x21 && src[n] <= 0x7E) {
    dst[n] = src[n];
    ++n;
  }
  dst[n] = '\0';
}

// True when `text` is "vMAJOR.MINOR.PATCH" (anything after PATCH is ignored)
// and not older than the given version. Anything unparsable counts as older.
inline bool versionAtLeast(const char* text, int major, int minor, int patch) {
  if (text == nullptr || text[0] != 'v') return false;
  const char* p = text + 1;
  int parts[3] = {0, 0, 0};
  for (int i = 0; i < 3; ++i) {
    if (*p < '0' || *p > '9') return false;
    int value = 0;
    while (*p >= '0' && *p <= '9') {
      value = value * 10 + (*p - '0');
      if (value > 9999) return false;
      ++p;
    }
    parts[i] = value;
    if (i < 2) {
      if (*p != '.') return false;
      ++p;
    }
  }
  if (parts[0] != major) return parts[0] > major;
  if (parts[1] != minor) return parts[1] > minor;
  return parts[2] >= patch;
}
