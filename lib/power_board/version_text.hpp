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
