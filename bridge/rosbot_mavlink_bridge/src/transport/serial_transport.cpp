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

#include "rosbot_mavlink_bridge/transport/serial_transport.hpp"

#include <fcntl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

namespace rosbot_mavlink_bridge {

namespace {
speed_t mapBaud(int baud) {
  switch (baud) {
    case 9600:
      return B9600;
    case 19200:
      return B19200;
    case 38400:
      return B38400;
    case 57600:
      return B57600;
    case 115200:
      return B115200;
    case 230400:
      return B230400;
    case 460800:
      return B460800;
    case 921600:
      return B921600;
    default:
      return B921600;
  }
}
}  // namespace

bool SerialTransport::open() {
  if (fd_.load() >= 0) return true;

  int fd = ::open(cfg_.port.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd < 0) return false;

  termios tty{};
  if (::tcgetattr(fd, &tty) != 0) {
    ::close(fd);
    return false;
  }

  // Raw 8N1; MAVLink owns framing.
  cfmakeraw(&tty);
  tty.c_cflag |= CLOCAL | CREAD;
  tty.c_cflag &= ~PARENB;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CSIZE;
  tty.c_cflag |= CS8;
  tty.c_cflag &= ~CRTSCTS;
  tty.c_cc[VMIN] = 0;
  tty.c_cc[VTIME] = 0;

  speed_t baud = mapBaud(cfg_.baudrate);
  ::cfsetispeed(&tty, baud);
  ::cfsetospeed(&tty, baud);

  if (::tcsetattr(fd, TCSANOW, &tty) != 0) {
    ::close(fd);
    return false;
  }

  ::tcflush(fd, TCIOFLUSH);
  fd_.store(fd);
  return true;
}

void SerialTransport::close() {
  int fd = fd_.exchange(-1);
  if (fd >= 0) ::close(fd);
}

std::size_t SerialTransport::write(const std::uint8_t* buf, std::size_t len) {
  int fd = fd_.load();
  if (fd < 0) return 0;
  ssize_t n = ::write(fd, buf, len);
  return (n > 0) ? static_cast<std::size_t>(n) : 0;
}

std::size_t SerialTransport::read(std::uint8_t* buf, std::size_t len,
                                  int timeout_ms) {
  int fd = fd_.load();
  if (fd < 0) return 0;

  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(fd, &rfds);
  timeval tv{};
  tv.tv_sec = timeout_ms / 1000;
  tv.tv_usec = (timeout_ms % 1000) * 1000;
  int sel =
      ::select(fd + 1, &rfds, nullptr, nullptr, timeout_ms < 0 ? nullptr : &tv);
  if (sel <= 0) return 0;

  ssize_t n = ::read(fd, buf, len);
  return (n > 0) ? static_cast<std::size_t>(n) : 0;
}

}  // namespace rosbot_mavlink_bridge
