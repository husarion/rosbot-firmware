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

#include <STM32FreeRTOS.h>

#include "communication_manager.hpp"

#ifdef USE_MAVLINK
#include <Arduino.h>

// MAVLink build: the bridge owns wall-clock translation via TIMESYNC, so
// the MCU stamps everything with monotonic time_boot_ns (= micros() * 1000)
// and the producer never has to gate on a sync handshake.
static inline bool rtos_get_timestamp_ns(int64_t& timestamp_ns) {
  timestamp_ns = static_cast<int64_t>(micros()) * 1000LL;
  return true;
}
#else
#include <micro_ros_arduino.h>

// micro-ROS build: only enqueue once the agent has synced the wall clock,
// so publishers can stamp with sec/nsec that match the ROS-side time.
static inline bool rtos_get_timestamp_ns(int64_t& timestamp_ns) {
  if (rmw_uros_epoch_synchronized()) {
    timestamp_ns = rmw_uros_epoch_nanos();
    return true;
  }
  return false;
}
#endif

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

static inline uint16_t taskGetFreq(void* params) {
  return static_cast<uint16_t>(reinterpret_cast<uintptr_t>(params));
}

static inline TickType_t taskGetPeriod(void* params) {
  return frequencyToTicks(taskGetFreq(params));
}

struct TaskHandleWrapper {
  TaskHandle_t handle = nullptr;

  void create(const TaskConfig& cfg) {
    void* freq_param =
        reinterpret_cast<void*>(static_cast<uintptr_t>(cfg.frequency));
    auto result = xTaskCreate(cfg.function, cfg.name,
                              configMINIMAL_STACK_SIZE + cfg.stack, freq_param,
                              cfg.priority, &handle);
    if (g_comm_mgr.hasDebugSerial()) {
      if (result != pdPASS) {
        g_comm_mgr.debugSerial()->printf(
            "Failed to create task %s, error code: %d\r\n", cfg.name, result);
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
