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

#include <cmath>
#include <limits>

#include "rosbot_mavlink_bridge/range_value.hpp"

using rosbot_mavlink_bridge::rangeFromDistanceSensor;

TEST(RangeValue, InvalidSignalIsNan)
{
  EXPECT_TRUE(std::isnan(rangeFromDistanceSensor(0, 1, 90, 1)));
}

TEST(RangeValue, InRangeIsMetres)
{
  EXPECT_FLOAT_EQ(0.42f, rangeFromDistanceSensor(42, 1, 90, 0));
}

TEST(RangeValue, OutOfRangeFollowsRep117)
{
  EXPECT_EQ(-std::numeric_limits<float>::infinity(), rangeFromDistanceSensor(0, 1, 90, 0));
  EXPECT_EQ(std::numeric_limits<float>::infinity(), rangeFromDistanceSensor(95, 1, 90, 0));
}
