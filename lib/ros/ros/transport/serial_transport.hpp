// Copyright 2022 Husarion sp. z o.o.
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
#include <cstdint>

extern "C" {
#include <uxr/client/profile/transport/custom/custom_transport.h>
}

bool serial_transport_open(struct uxrCustomTransport* transport);
bool serial_transport_close(struct uxrCustomTransport* transport);

size_t serial_transport_write(struct uxrCustomTransport* transport,
                              const uint8_t* buf, size_t len, uint8_t* errcode);

size_t serial_transport_read(struct uxrCustomTransport* transport, uint8_t* buf,
                             size_t len, int timeout, uint8_t* errcode);
