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

#include "communication_manager.hpp"
#include "freertos.hpp"
#include "time_base.hpp"

// Monotonic time since boot; the bridge owns wall-clock time via TIMESYNC.
inline int64_t rtos_timestamp_ns() {
  return static_cast<int64_t>(time_boot_us() * 1000ULL);
}

void createQueues();
void createTasks();

// Priority levels
// 7 - Highest (configMAX_PRIORITIES)
// 0 - Idle (tskIDLE_PRIORITY)
enum Priority : UBaseType_t {
  BLOCKING = 1,
  OBSERVING = 2,
  SENSORS = 3,
  COMMUNICATION = 4,
  CONTROL = 5,
  SAFETY = 6
};

enum Stack : uint16_t {
  MINIMAL = 0,
  XXS = 16,
  XS = 32,
  S = 64,
  M = 128,
  L = 256,
  XL = 512,
  XXL = 1024
};

struct TaskConfig {
  const char* name;
  Priority priority;
  Stack stack;
  float frequency;
  void (*function)(void*);
};

inline TickType_t frequencyToTicks(float freq) {
  return freq == 0 ? 0 : (TickType_t)(configTICK_RATE_HZ / freq);
}

inline TickType_t taskGetPeriod(void* params) {
  return frequencyToTicks(static_cast<const TaskConfig*>(params)->frequency);
}

struct TaskHandleWrapper {
  TaskHandle_t handle = nullptr;

  // The task keeps a pointer to cfg, so cfg must outlive it.
  void create(const TaskConfig& cfg) {
    auto result = xTaskCreate(
        cfg.function, cfg.name, configMINIMAL_STACK_SIZE + cfg.stack,
        const_cast<TaskConfig*>(&cfg), cfg.priority, &handle);
    if (g_comm_mgr.hasDebugSerial()) {
      if (result != pdPASS) {
        g_comm_mgr.debugSerial()->printf(
            "Failed to create task %s, error code: %ld\r\n", cfg.name,
            static_cast<long>(result));
      } else {
        g_comm_mgr.debugSerial()->printf("Created %s task\r\n", cfg.name);
      }
    }
  }

  void destroy() {
    if (handle != nullptr) {
      vTaskDelete(handle);
      handle = nullptr;
    }
  }
};
