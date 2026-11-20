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

// newlib system calls. There is no console: stdout (lwIP's assert printf)
// is discarded; the firmware writes text through platform::Uart.

#include <errno.h>
#include <sys/stat.h>

#include "fault.hpp"

int _write(int fd, const char* buf, int len) {
  (void)fd;
  (void)buf;
  return len;
}

int _read(int fd, char* buf, int len) {
  (void)fd;
  (void)buf;
  (void)len;
  return 0;
}

int _close(int fd) {
  (void)fd;
  return -1;
}

int _fstat(int fd, struct stat* st) {
  (void)fd;
  st->st_mode = S_IFCHR;
  return 0;
}

int _isatty(int fd) {
  (void)fd;
  return 1;
}

int _lseek(int fd, int offset, int whence) {
  (void)fd;
  (void)offset;
  (void)whence;
  return 0;
}

int _getpid(void) { return 1; }

int _kill(int pid, int sig) {
  (void)pid;
  (void)sig;
  errno = EINVAL;
  return -1;
}

void _exit(int status) {
  (void)status;
  fatalErrorWithCode(kFaultExit, 0, 0);
}
