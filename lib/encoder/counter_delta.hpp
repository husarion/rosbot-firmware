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

#include <cstdint>

// Signed tick delta between two reads of a timer counting 0..counter_max and
// wrapping. Correct while the true delta stays within half the range.
inline int32_t counterDelta(uint32_t cnt, uint32_t last_cnt,
                            uint32_t counter_max) {
  const int32_t range = static_cast<int32_t>(counter_max) + 1;
  int32_t delta = static_cast<int32_t>(cnt - last_cnt);
  if (delta > range / 2) {
    delta -= range;
  } else if (delta < -range / 2) {
    delta += range;
  }
  return delta;
}
