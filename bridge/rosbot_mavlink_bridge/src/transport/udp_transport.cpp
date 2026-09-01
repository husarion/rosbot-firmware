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

#include "rosbot_mavlink_bridge/transport/udp_transport.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

namespace rosbot_mavlink_bridge
{

bool UdpTransport::open()
{
  if (fd_.load() >= 0) {return true;}

  int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {return false;}

  // Skip TIME_WAIT so bridge restarts are immediate.
  int reuse = 1;
  ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  sockaddr_in local{};
  local.sin_family = AF_INET;
  local.sin_addr.s_addr = htonl(INADDR_ANY);
  local.sin_port = htons(cfg_.local_port);
  if (::bind(fd, reinterpret_cast<sockaddr *>(&local), sizeof(local)) < 0) {
    ::close(fd);
    return false;
  }

  std::memset(&peer_addr_, 0, sizeof(peer_addr_));
  peer_addr_.sin_family = AF_INET;
  peer_addr_.sin_port = htons(cfg_.peer_port);
  if (::inet_pton(AF_INET, cfg_.peer_ip.c_str(), &peer_addr_.sin_addr) != 1) {
    ::close(fd);
    return false;
  }

  fd_.store(fd);
  return true;
}

void UdpTransport::close()
{
  int fd = fd_.exchange(-1);
  if (fd >= 0) {::close(fd);}
}

std::size_t UdpTransport::write(const std::uint8_t * buf, std::size_t len)
{
  int fd = fd_.load();
  if (fd < 0) {return 0;}
  ssize_t n =
    ::sendto(fd, buf, len, 0, reinterpret_cast<const sockaddr *>(&peer_addr_),
               sizeof(peer_addr_));
  return (n > 0) ? static_cast<std::size_t>(n) : 0;
}

std::size_t UdpTransport::read(
  std::uint8_t * buf, std::size_t len,
  int timeout_ms)
{
  int fd = fd_.load();
  if (fd < 0) {return 0;}

  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(fd, &rfds);
  timeval tv{};
  tv.tv_sec = timeout_ms / 1000;
  tv.tv_usec = (timeout_ms % 1000) * 1000;
  int sel =
    ::select(fd + 1, &rfds, nullptr, nullptr, timeout_ms < 0 ? nullptr : &tv);
  if (sel <= 0) {return 0;}

  ssize_t n = ::recv(fd, buf, len, 0);
  return (n > 0) ? static_cast<std::size_t>(n) : 0;
}

}  // namespace rosbot_mavlink_bridge
