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

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace rosbot_mavlink_bridge
{

// MCU boot clock → bridge wall clock offset (wall = mcu + offset()).
//
// Two-way samples come from a bridge-initiated TIMESYNC: t1 sent, MCU stamp,
// t4 received; the midpoint cancels the symmetric part of the link delay.
// One-way samples (MCU-initiated TIMESYNC, received at bridge time) include
// the full uplink delay, so they only seed the estimate until the first
// two-way sample replaces it.
class TimesyncEstimator
{
public:
  static constexpr std::size_t kRttWindow = 8;
  // Below this many RTTs there is no meaningful median yet.
  static constexpr std::size_t kMinRttsForFilter = 4;

  explicit TimesyncEstimator(double alpha)
  : alpha_(alpha) {}

  void addOneWay(std::int64_t bridge_ns, std::int64_t mcu_ns)
  {
    if (two_way_) {
      return;
    }
    update(bridge_ns - mcu_ns);
  }

  // Returns false when the sample was rejected (queued somewhere on the way).
  bool addTwoWay(std::int64_t t1_ns, std::int64_t mcu_ns, std::int64_t t4_ns)
  {
    const std::int64_t rtt = t4_ns - t1_ns;
    if (rtt < 0) {
      return false;
    }
    rtts_[rtt_next_] = rtt;
    rtt_next_ = (rtt_next_ + 1) % kRttWindow;
    rtt_count_ = std::min(rtt_count_ + 1, kRttWindow);
    if (rtt_count_ >= kMinRttsForFilter && rtt > 2 * medianRtt()) {
      return false;
    }

    const std::int64_t sample = t1_ns + rtt / 2 - mcu_ns;
    if (!two_way_) {
      two_way_ = true;
      synced_ = false;  // drop the biased one-way estimate outright
    }
    update(sample);
    last_rtt_ns_ = rtt;
    return true;
  }

  bool synced() const {return synced_;}
  bool twoWay() const {return two_way_;}
  std::int64_t offset() const {return offset_;}
  std::int64_t lastRtt() const {return last_rtt_ns_;}

private:
  void update(std::int64_t sample)
  {
    if (!synced_) {
      offset_ = sample;
      synced_ = true;
      return;
    }
    // EWMA on the difference: a double holds a ~1.7e18 ns offset only to
    // ~256 ns, but the step itself is small.
    offset_ += static_cast<std::int64_t>(
      alpha_ * static_cast<double>(sample - offset_));
  }

  std::int64_t medianRtt() const
  {
    std::array<std::int64_t, kRttWindow> sorted = rtts_;
    auto mid = sorted.begin() + rtt_count_ / 2;
    std::nth_element(sorted.begin(), mid, sorted.begin() + rtt_count_);
    return *mid;
  }

  double alpha_;
  std::int64_t offset_ = 0;
  bool synced_ = false;
  bool two_way_ = false;
  std::array<std::int64_t, kRttWindow> rtts_{};
  std::size_t rtt_count_ = 0;
  std::size_t rtt_next_ = 0;
  std::int64_t last_rtt_ns_ = 0;
};

}  // namespace rosbot_mavlink_bridge
