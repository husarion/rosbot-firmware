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

#include "pin.hpp"

namespace platform {

// I2C bus clear (UM10204 3.1.16): a slave that lost sync mid-byte can hold
// SDA low forever. Up to nine SCL pulses let it finish the byte, then a
// STOP returns the bus to idle. Leaves both pins as open-drain outputs,
// released; the caller hands them back to the peripheral. halfPeriod()
// waits half an SCL period. Returns true when both lines read high.
template <typename HalfPeriod>
bool clearI2cBus(const Pin& scl, const Pin& sda, HalfPeriod halfPeriod) {
  sda.setAsOutputOpenDrain(true);
  scl.setAsOutputOpenDrain(true);
  halfPeriod();
  for (int i = 0; i < 9 && !sda.read(); ++i) {
    scl.reset();
    halfPeriod();
    scl.set();
    halfPeriod();
  }
  // STOP: SDA rises while SCL is high.
  scl.reset();
  halfPeriod();
  sda.reset();
  halfPeriod();
  scl.set();
  halfPeriod();
  sda.set();
  halfPeriod();
  return scl.read() && sda.read();
}

}  // namespace platform
