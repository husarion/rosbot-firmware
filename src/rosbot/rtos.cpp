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

#include "rtos.hpp"

#include <STM32FreeRTOS.h>

#include "battery_interface.hpp"
#include "communication_manager.hpp"
#include "config.hpp"
#include "imu_interface.hpp"
#include "led_indicator.hpp"
#include "mavlink_node.hpp"
#include "mavlink_types.hpp"
#include "motor_array.hpp"
#include "robotics_link.hpp"
#include "ros/ros_node.hpp"

RoboticsLink* g_link = nullptr;

void createQueues() {
  battery_queue = xQueueCreate(1, sizeof(BatteryStamped));
  imu_queue = xQueueCreate(1, sizeof(ImuStamped));
  joint_state_queue = xQueueCreate(1, sizeof(JointStateStamped));
  ranges_queue = xQueueCreate(1, sizeof(RangesStamped));
}

void batteryTask(void* p);
void imuTask(void* p);
void ledIndicatorTask(void* p);
void monitorTask(void* p);
void motorControlTask(void* p);
void rangeTask(void* p);
void uRosTask(void* p);

inline TaskConfig tasks[] = {
    {"Battery", Priority::SENSORS, Stack::XS, 10, batteryTask},
    {"Imu", Priority::SENSORS, Stack::M, 100, imuTask},
    {"LedIndicator", Priority::OBSERVING, Stack::XS, 20, ledIndicatorTask},
#ifndef RELEASE
    {"Monitor", Priority::BLOCKING, Stack::XL, 1, monitorTask},
#endif
    {"MotorControl", Priority::CONTROL, Stack::M, 200, motorControlTask},
    {"Range", Priority::SENSORS, Stack::M, 10, rangeTask},
    {"uRos", Priority::COMMUNICATION, Stack::XXL, 200, uRosTask},
};

inline TaskHandleWrapper taskHandles[sizeof(tasks) / sizeof(tasks[0])];

void createTasks() {
  for (size_t i = 0; i < sizeof(tasks) / sizeof(tasks[0]); i++) {
    taskHandles[i].create(tasks[i]);
  }
}

void batteryTask(void* p) {
  TickType_t period = taskGetPeriod(p);
  TickType_t wake_time = xTaskGetTickCount();
  BatteryStamped data = {};

  while (true) {
    bool connected = rtos_get_timestamp_ns(data.timestamp_ns);
    g_battery->update();  // TODO: DMA should be used
    data.data = g_battery->getData();

    if (connected) {
      xQueueOverwrite(battery_queue, &data);
    }
    vTaskDelayUntil(&wake_time, period);
  }
}

void imuTask(void* p) {
  TickType_t period = taskGetPeriod(p);
  TickType_t wake_time = xTaskGetTickCount();
  ImuStamped data = {};

  while (true) {
    bool connected = rtos_get_timestamp_ns(data.timestamp_ns);
    g_imu->update();
    data.data = g_imu->getData();

    if (connected) {
      xQueueOverwrite(imu_queue, &data);
    }
    vTaskDelayUntil(&wake_time, period);
  }
}

void ledIndicatorTask(void* p) {
  TickType_t period = taskGetPeriod(p);
  TickType_t wake_time = xTaskGetTickCount();

  while (true) {
    bool battery_low = g_battery->isLow();
    bool error_state = false;

    g_indicator.update(battery_low, !g_link->isConnected(), error_state);
    vTaskDelayUntil(&wake_time, period);
  }
}

void monitorTask(void* p) {
  TickType_t period = taskGetPeriod(p);
  TickType_t wake_time = xTaskGetTickCount();
  char buf[768];

  while (true) {
    vTaskGetRunTimeStats(buf);
    if (g_comm_mgr.hasDebugSerial()) {
      g_comm_mgr.debugSerial()->printf("%s\r\n", buf);

      for (size_t i = 0; i < sizeof(taskHandles) / sizeof(taskHandles[0]);
           i++) {
        g_comm_mgr.debugSerial()->printf(
            "%-16s %-6u B\r\n", tasks[i].name,
            uxTaskGetStackHighWaterMark(taskHandles[i].handle) *
                sizeof(StackType_t));
      }
      g_comm_mgr.debugSerial()->printf("\r\nFree heap memory: %u B\r\n",
                                       (unsigned)xPortGetFreeHeapSize());
    }

    vTaskDelayUntil(&wake_time, period);
  }
}

void motorControlTask(void* p) {
  TickType_t period = taskGetPeriod(p);
  TickType_t wake_time = xTaskGetTickCount();
  JointStateStamped data = {};

  while (true) {
    bool connected = rtos_get_timestamp_ns(data.timestamp_ns);

    g_motors.update();  // updates all motors, including encoders
    data.data = g_motors.getData();
    if (connected) {
      xQueueOverwrite(joint_state_queue, &data);
    }

    vTaskDelayUntil(&wake_time, period);
  }
}

void rangeTask(void* p) {
  TickType_t period = taskGetPeriod(p);
  TickType_t wake_time = xTaskGetTickCount();
  RangesStamped data = {};

  while (true) {
    bool connected = rtos_get_timestamp_ns(data.timestamp_ns);
    g_ranges.update();
    data.data = g_ranges.getData();

    if (connected) {
      xQueueOverwrite(ranges_queue, &data);
    }
    vTaskDelay(period);
  }
}

void uRosTask(void* p) {
  TickType_t period = taskGetPeriod(p);
  TickType_t wake_time = xTaskGetTickCount();
  while (true) {
    g_link->loop();
    vTaskDelayUntil(&wake_time, period);
  }
}
