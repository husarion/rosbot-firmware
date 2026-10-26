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

#include <gtest/gtest.h>

#include <cstdint>

#include "rosbot_mavlink_bridge/timesync_estimator.hpp"

using rosbot_mavlink_bridge::TimesyncEstimator;

namespace
{
constexpr std::int64_t kMs = 1000000;
constexpr std::int64_t kOffset = 1700000000000000000;  // wall - mcu
}  // namespace

TEST(TimesyncEstimator, TwoWayMidpointCancelsSymmetricDelay)
{
  TimesyncEstimator est(1.0);
  const std::int64_t t1 = kOffset + 10 * kMs;
  const std::int64_t mcu = 10 * kMs + 2 * kMs;  // MCU stamps after 2 ms uplink
  const std::int64_t t4 = t1 + 4 * kMs;          // 2 ms downlink
  ASSERT_TRUE(est.addTwoWay(t1, mcu, t4));
  EXPECT_TRUE(est.synced());
  EXPECT_TRUE(est.twoWay());
  EXPECT_EQ(est.offset(), kOffset);
  EXPECT_EQ(est.lastRtt(), 4 * kMs);
}

TEST(TimesyncEstimator, FirstTwoWayReplacesBiasedOneWay)
{
  TimesyncEstimator est(0.05);
  // One-way sample carries the 3 ms uplink delay.
  est.addOneWay(kOffset + 13 * kMs, 10 * kMs);
  EXPECT_EQ(est.offset(), kOffset + 3 * kMs);
  ASSERT_TRUE(est.addTwoWay(kOffset + 20 * kMs, 23 * kMs, kOffset + 26 * kMs));
  EXPECT_EQ(est.offset(), kOffset);
  // One-way samples are ignored from now on.
  est.addOneWay(kOffset + 50 * kMs, 40 * kMs);
  EXPECT_EQ(est.offset(), kOffset);
}

TEST(TimesyncEstimator, RejectsRttAboveTwiceMedian)
{
  TimesyncEstimator est(1.0);
  std::int64_t t = 0;
  for (int i = 0; i < 6; ++i, t += 100 * kMs) {
    ASSERT_TRUE(est.addTwoWay(kOffset + t, t + kMs, kOffset + t + 2 * kMs));
  }
  // 10 ms RTT vs 2 ms median: delayed in a queue, and asymmetric.
  EXPECT_FALSE(est.addTwoWay(kOffset + t, t + 9 * kMs, kOffset + t + 10 * kMs));
  EXPECT_EQ(est.offset(), kOffset);
}

TEST(TimesyncEstimator, RejectsNegativeRtt)
{
  TimesyncEstimator est(1.0);
  EXPECT_FALSE(est.addTwoWay(kOffset + 10 * kMs, 5 * kMs, kOffset + 9 * kMs));
  EXPECT_FALSE(est.synced());
}

TEST(TimesyncEstimator, EwmaSmoothsAcceptedSamples)
{
  TimesyncEstimator est(0.5);
  ASSERT_TRUE(est.addTwoWay(kOffset, kMs, kOffset + 2 * kMs));
  // Next sample says the offset moved by +2 ms; alpha 0.5 takes half.
  ASSERT_TRUE(est.addTwoWay(kOffset + 100 * kMs, 99 * kMs, kOffset + 102 * kMs));
  EXPECT_EQ(est.offset(), kOffset + kMs);
}

TEST(TimesyncEstimator, AveragesTheFirstSamples)
{
  TimesyncEstimator est(0.05);
  // RTT 2 ms each; offsets kOffset and kOffset + 2 ms → plain mean +1 ms,
  // not 5 % of the way.
  ASSERT_TRUE(est.addTwoWay(kOffset, kMs, kOffset + 2 * kMs));
  ASSERT_TRUE(est.addTwoWay(kOffset + 100 * kMs, 99 * kMs, kOffset + 102 * kMs));
  EXPECT_EQ(est.offset(), kOffset + kMs);
}

TEST(TimesyncEstimator, ReseedsAfterAPersistentStep)
{
  TimesyncEstimator est(0.05);
  std::int64_t t = 0;
  for (int i = 0; i < 10; ++i, t += 100 * kMs) {
    ASSERT_TRUE(est.addTwoWay(kOffset + t, t + kMs, kOffset + t + 2 * kMs));
  }
  // MCU reboots: its clock restarts at 0 while wall time keeps going.
  const std::int64_t new_offset = kOffset + t;
  for (int i = 0; i < TimesyncEstimator::kStepSamples; ++i, t += 100 * kMs) {
    const std::int64_t mcu = t - (new_offset - kOffset);
    ASSERT_TRUE(est.addTwoWay(kOffset + t, mcu + kMs, kOffset + t + 2 * kMs));
  }
  EXPECT_EQ(est.offset(), new_offset);
}

TEST(TimesyncEstimator, IgnoresASingleOutlier)
{
  TimesyncEstimator est(0.05);
  std::int64_t t = 0;
  for (int i = 0; i < 10; ++i, t += 100 * kMs) {
    ASSERT_TRUE(est.addTwoWay(kOffset + t, t + kMs, kOffset + t + 2 * kMs));
  }
  // One sample 200 ms off with a normal RTT, then back to normal.
  ASSERT_TRUE(est.addTwoWay(kOffset + t, t - 199 * kMs, kOffset + t + 2 * kMs));
  t += 100 * kMs;
  ASSERT_TRUE(est.addTwoWay(kOffset + t, t + kMs, kOffset + t + 2 * kMs));
  EXPECT_EQ(est.offset(), kOffset);
}

TEST(TimesyncEstimator, ResetForgetsEverything)
{
  TimesyncEstimator est(0.05);
  ASSERT_TRUE(est.addTwoWay(kOffset, kMs, kOffset + 2 * kMs));
  est.reset();
  EXPECT_FALSE(est.synced());
  EXPECT_FALSE(est.twoWay());
  est.addOneWay(kOffset + 13 * kMs, 10 * kMs);
  EXPECT_EQ(est.offset(), kOffset + 3 * kMs);
}
