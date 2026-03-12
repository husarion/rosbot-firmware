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

#include "animations/led_animations.hpp"
#include "battery_interface.hpp"
#include "config.hpp"
#include "encoder_array.hpp"
#include "fan.hpp"
#include "imu_interface.hpp"
#include "led_indicator.hpp"
#include "led_strip.hpp"
#include "motor_array.hpp"
#include "ntc.hpp"
#include "ros/publishers/battery_publisher.hpp"
#include "ros/publishers/imu_publisher.hpp"
#include "ros/publishers/joint_state_publisher.hpp"
#include "ros/ros_node.hpp"
#include "serial_manager.hpp"

// Externs
extern FanController g_fan;

// ───── Queues ─────
void createQueues() {
  battery_queue = xQueueCreate(1, sizeof(BatteryStamped));
  imu_queue = xQueueCreate(1, sizeof(ImuStamped));
  joint_state_queue = xQueueCreate(1, sizeof(EncodersStamped));
  led_strip_queue = xQueueCreate(1, sizeof(LedFrameMsg));
}

// ───── Create all tasks ─────
void batteryTask(void* p);
void encoderTask(void* p);
void fanTask(void* p);
void imuTask(void* p);
void ledIndicatorTask(void* p);
void monitorTask(void* p);
void motorControlTask(void* p);
void ledAnimationTask(void* p);
void uRosTask(void* p);
void uRosPingTask(void* p);

inline TaskConfig tasks[] = {
    // {"Battery", Priority::SENSORS, Stack::SMALL, 10, batteryTask},
    {"Encoder", Priority::CONTROL, Stack::SMALL, 500, encoderTask},
    {"Fan", Priority::OBSERVING, Stack::SMALL, 10, fanTask},
    {"Imu", Priority::SENSORS, Stack::SMALL, 100, imuTask},
    {"LedAnimation", Priority::OBSERVING, Stack::MEDIUM, 25, ledAnimationTask},
    {"LedIndicator", Priority::OBSERVING, Stack::XSMALL, 20, ledIndicatorTask},
    // {"Monitor", Priority::OBSERVING, Stack::MEDIUM, 1, monitorTask},
    {"MotorControl", Priority::CONTROL, Stack::MEDIUM, 200, motorControlTask},
    {"uRos", Priority::COMMUNICATION, Stack::LARGE, 100, uRosTask},
    {"uRosPing", Priority::OBSERVING, Stack::MEDIUM, 2, uRosPingTask},
};

inline TaskHandleWrapper taskHandles[sizeof(tasks) / sizeof(tasks[0])];

void createTasks() {
  for (size_t i = 0; i < sizeof(tasks) / sizeof(tasks[0]); i++) {
    taskHandles[i].create(tasks[i]);
  }
}

// ───── Task functions ─────
// void batteryTask(void* p) {
//   TickType_t period = taskGetPeriod(p);
//   TickType_t wake_time = xTaskGetTickCount();
//   BatteryStamped data = {};

//   while (true) {
//     bool connected = rtos_get_timestamp_ns(data.timestamp_ns);
//     g_battery->update();  // TODO: DMA should be used
//     data.data = g_battery->getData();

//     if (connected) {
//       xQueueOverwrite(battery_queue, &data);
//     }
//     vTaskDelayUntil(&wake_time, period);
//   }
// }

void encoderTask(void* p) {
  TickType_t period = taskGetPeriod(p);
  TickType_t wake_time = xTaskGetTickCount();
  EncodersStamped data = {};

  while (true) {
    bool connected = rtos_get_timestamp_ns(data.timestamp_ns);
    g_encoders.update();
    data.data = g_encoders.getData();

    if (connected) {
      xQueueOverwrite(joint_state_queue, &data);
    }
    vTaskDelayUntil(&wake_time, period);
  }
}

void fanTask(void* p) {
  TickType_t period = taskGetPeriod(p);
  TickType_t wake_time = xTaskGetTickCount();

  while (true) {
    float temp = ntc.readCelsius();
    g_fan.update(temp);
  
    vTaskDelayUntil(&wake_time, period);
  }
}


void imuTask(void* p) {
  TickType_t period = taskGetPeriod(p);
  TickType_t wake_time = xTaskGetTickCount();
  ImuStamped data = {};

  while (true) {
    bool connected = rtos_get_timestamp_ns(data.timestamp_ns);
    g_imu->update();  // TODO: DMA should be used
    data.data = g_imu->getData();

    if (connected) {
      xQueueOverwrite(imu_queue, &data);
    }
    vTaskDelayUntil(&wake_time, period);
  }
}

void ledAnimationTask(void* p) {
  TickType_t period = taskGetPeriod(p);

  LedFrameMsg frame;
  const TickType_t timeout = pdMS_TO_TICKS(LED_STRIP_TIMEOUT_MS);
  const TickType_t idle_period = pdMS_TO_TICKS(IDLE_ANIMATION_CHANGE_MS);
  const TickType_t interval = pdMS_TO_TICKS(IDLE_ANIMATION_INTERVAL_MS);

  TickType_t last_msg_time = xTaskGetTickCount();
  TickType_t last_idle_change = 0;
  int idle_state = 0;

  while (true) {
    TickType_t now = xTaskGetTickCount();

    if (xQueueReceive(led_strip_queue, &frame, 0) == pdTRUE) {
      g_led_strip.setFromRGB8(frame.rgb_data, frame.pixel_count);
      g_led_strip.show();
      last_msg_time = now;
    }

    if ((now - last_msg_time) > timeout && (now - last_idle_change) > idle_period) {
      if (idle_state == 0) {
        idleAnimation(g_led_strip, 0xA0, 0xA0, 0xA0, interval);
        idle_state = 1;
      } else {
        idleAnimation(g_led_strip, 0xA0, 0x00, 0x00, interval);
        idle_state = 0;
      }

      last_idle_change = now;
    }
    vTaskDelay(period);
  }
}

void ledIndicatorTask(void* p) {
  TickType_t period = taskGetPeriod(p);
  TickType_t wake_time = xTaskGetTickCount();

  while (true) {
    bool battery_low = false;  // g_battery->isLow();
    bool error_state = false;

    g_indicator.update(battery_low, !g_ros_node.isConnected(), error_state);
    vTaskDelayUntil(&wake_time, period);
  }
}

void monitorTask(void* p) {
  TickType_t period = taskGetPeriod(p);
  TickType_t wake_time = xTaskGetTickCount();
  char buf[1000];

  while (true) {
    vTaskGetRunTimeStats(buf);
    g_serialManager.debug().printf("%s\r\n", buf);

    vTaskDelayUntil(&wake_time, period);
  }
}

void motorControlTask(void* p) {
  TickType_t period = taskGetPeriod(p);
  TickType_t wake_time = xTaskGetTickCount();

  while (true) {
    g_motors.update();

    vTaskDelayUntil(&wake_time, period);
  }
}

void uRosTask(void* p) {
  TickType_t period = taskGetPeriod(p);
  TickType_t wake_time = xTaskGetTickCount();
  while (true) {
    g_ros_node.publishLoop();
    vTaskDelayUntil(&wake_time, period);
  }
}

void uRosPingTask(void* p) {
  TickType_t period = taskGetPeriod(p);
  TickType_t wake_time = xTaskGetTickCount();

  while (true) {
    g_ros_node.loop();
    vTaskDelayUntil(&wake_time, period);
  }
}
