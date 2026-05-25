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

#include <STM32Ethernet.h>
#include <STM32FreeRTOS.h>

#include "animations/led_animations.hpp"
#include "battery_interface.hpp"
#include "communication_manager.hpp"
#include "config.hpp"
#include "fan.hpp"
#include "imu_interface.hpp"
#include "led_indicator.hpp"
#include "led_strip.hpp"
#include "mavlink_node.hpp"
#include "mavlink_types.hpp"
#include "motor_array.hpp"
#include "ntc.hpp"
#include "power_board.hpp"
#include "robotics_link.hpp"
#include "ros/ros_node.hpp"

extern FanController g_fan;
extern PowerBoard power_board;

RoboticsLink* g_link = nullptr;

void createQueues() {
  battery_queue = xQueueCreate(1, sizeof(BatteryStamped));
  imu_queue = xQueueCreate(1, sizeof(ImuStamped));
  joint_state_queue = xQueueCreate(1, sizeof(JointStateStamped));
  led_strip_queue = xQueueCreate(1, sizeof(LedFrameMsg));
}

void hwMonitorTask(
    void* p);  // Bat + Fan + Indicator: Merged due limited stack size
void imuTask(void* p);
void ledStripTask(void* p);
void monitorTask(void* p);
void motorControlTask(void* p);
void shutdownTask(void* p);
void uRosTask(void* p);

TaskConfig tasks[] = {
    {"HwMonitor", Priority::OBSERVING, Stack::S, 10, hwMonitorTask},
    {"Imu", Priority::SENSORS, Stack::M, 100, imuTask},
    {"LedStrip", Priority::COMMUNICATION, Stack::M, 30, ledStripTask},
#ifndef RELEASE
    {"Monitor", Priority::BLOCKING, Stack::XL, 1, monitorTask},
#endif
    {"MotorControl", Priority::CONTROL, Stack::XXS, 200, motorControlTask},
    {"Shutdown", Priority::OBSERVING, Stack::M, 3, shutdownTask},
    {"uRos", Priority::COMMUNICATION, Stack::XXL, 1000, uRosTask},
};

TaskHandleWrapper taskHandles[sizeof(tasks) / sizeof(tasks[0])];

void createTasks() {
  for (size_t i = 0; i < sizeof(tasks) / sizeof(tasks[0]); i++) {
    taskHandles[i].create(tasks[i]);
  }
}

void hwMonitorTask(void* p) {
  TickType_t period = taskGetPeriod(p);
  TickType_t wake_time = xTaskGetTickCount();
  BatteryStamped data = {};
  TickType_t last_battery_update = 0;

  while (true) {
    // LED Indicator
    bool battery_low = g_battery->isLow();
    bool error_state = false;
    g_indicator.update(battery_low, !g_link->isConnected(), error_state);

    if (xTaskGetTickCount() - last_battery_update > pdMS_TO_TICKS(1000)) {
      // Fan
      float temp = ntc.readCelsius();
      g_fan.update(temp);

      // Battery
      if (!power_board.boardInfo().isValid()) {
        power_board.requestBoardInfo();
      }

      power_board.requestBatteryState();
      power_board.update();

      if (power_board.hasBatteryUpdate()) {
        bool connected = rtos_get_timestamp_ns(data.timestamp_ns);
        data.data = power_board.getData();
        if (connected) {
          xQueueOverwrite(battery_queue, &data);
        }
      }
      last_battery_update = xTaskGetTickCount();
    }

    vTaskDelayUntil(&wake_time, period);
  }
}

void imuTask(void* p) {
  TickType_t period = taskGetPeriod(p);
  TickType_t wake_time = xTaskGetTickCount();
  ImuStamped data = {};

  while (true) {
    if (rtos_get_timestamp_ns(data.timestamp_ns)) {
      g_imu->update();
      data.data = g_imu->getData();

      xQueueOverwrite(imu_queue, &data);
    }
    vTaskDelayUntil(&wake_time, period);
  }
}

void ledStripTask(void* p) {
  TickType_t period = taskGetPeriod(p);
  TickType_t wake_time = xTaskGetTickCount();

  LedFrameMsg frame;
  const TickType_t timeout = pdMS_TO_TICKS(LED_STRIP_TIMEOUT_MS);
  const TickType_t idle_period = pdMS_TO_TICKS(IDLE_ANIMATION_CHANGE_MS);
  const TickType_t interval = pdMS_TO_TICKS(IDLE_ANIMATION_INTERVAL_MS);

  TickType_t last_msg_time = 0;
  TickType_t last_idle_change = 0;
  int idle_state = 0;
  bool reset = true;

  while (true) {
    TickType_t now = xTaskGetTickCount();

    if (xQueueReceive(led_strip_queue, &frame, 0) == pdTRUE) {
      g_led_strip.setFromRGB8(frame.rgb_data, frame.pixel_count);
      g_led_strip.show();
      last_msg_time = now;
      idle_state = 0;
      reset = true;
    } else if (((now - last_msg_time) > timeout &&
                (now - last_idle_change) > idle_period)) {
      if (idle_state == 0) {
        idleAnimation(g_led_strip, 0xA0, 0xA0, 0xA0, interval, reset);
        idle_state = 1;
        reset = false;
      } else {
        idleAnimation(g_led_strip, 0xA0, 0x00, 0x00, interval);
        idle_state = 0;
      }

      last_idle_change = now;
    }
    vTaskDelay(period);
  }
}

void monitorTask(void* p) {
  TickType_t period = taskGetPeriod(p);
  TickType_t wake_time = xTaskGetTickCount();
  char buf[768];

  while (true) {
    vTaskGetRunTimeStats(buf);
    if (g_comm_mgr.hasDebugSerial()) {
      g_comm_mgr.debugSerial()->printf("\r\n%s\r\n", buf);

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

void shutdownTask(void* p) {
  TickType_t period = taskGetPeriod(p);
  TickType_t wake_time = xTaskGetTickCount();

  EthernetClient eth_client;
  while (true) {
    if (digitalRead(PB_SHD_DETECT) == HIGH) {
      if (eth_client.connect(AGENT_IP, 3000, 100)) {
        eth_client.println("GET /shutdown HTTP/1.1");
        eth_client.stop();
      }

      vTaskDelay(pdMS_TO_TICKS(SHUTDOWN_WAIT_MS));
      digitalWrite(PB_SHD_CONFIRM, HIGH);
      vTaskSuspend(nullptr);
    }
    vTaskDelayUntil(&wake_time, period);
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
