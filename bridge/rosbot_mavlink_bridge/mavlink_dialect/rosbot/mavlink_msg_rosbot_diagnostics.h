#pragma once
// MESSAGE ROSBOT_DIAGNOSTICS PACKING

#define MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS 11030


typedef struct __mavlink_rosbot_diagnostics_t {
 uint32_t uptime_ms; /*< [ms] Time since MCU boot.*/
 uint32_t heap_free; /*< [bytes] Free heap now.*/
 uint32_t heap_min_free; /*< [bytes] Heap never handed out since boot, a lower bound of the free heap.*/
 uint32_t imu_errors; /*<  Failed IMU reads.*/
 uint32_t link_tx_dropped; /*<  Telemetry frames dropped because the link TX queue was full.*/
 uint32_t link_rx_errors; /*<  Incoming MAVLink frames the parser dropped (CRC or framing).*/
 uint32_t fault_pc; /*<  Program counter of the exception that caused the last firmware fault, 0 if not an exception.*/
 uint16_t reset_cause; /*<  Cause of the last MCU reset, bitmask: 1 reset pin, 2 power-on or brown-out, 4 software, 8 independent watchdog, 16 window watchdog, 32 low-power, 64 firmware fault (fatalError).*/
 uint16_t cpu_idle; /*< [d%] CPU idle over the last report period, 0.1 % units; 65535 in the first message after boot (no period yet).*/
 uint16_t stack_min_free; /*< [bytes] Smallest stack headroom of any task since boot.*/
 uint16_t range_timeouts[4]; /*<  Range sensor reads that timed out, order FL, FR, RL, RR (ROSbot; zero elsewhere).*/
 uint16_t i2c_recoveries[2]; /*<  I2C bus resets after faults: IMU bus, sensor/EEPROM bus.*/
 uint16_t motor_faults; /*<  Motor driver fault events (nFAULT asserted).*/
 uint16_t link_reconnects; /*<  Times the bridge connection was (re)established.*/
 char stack_min_task[16]; /*<  Name of the task with the smallest stack headroom.*/
 uint8_t imu_ok; /*<  1 when the IMU initialised and its last read succeeded.*/
 uint8_t range_ok; /*<  Bitmask of range sensors that initialised, bit 0 = FL.*/
 uint8_t fault_code; /*<  Why the last firmware fault reset happened: 0 none or unknown, 1 assert, 2 stack overflow, 3 out of heap, 4 hard fault, 5 memory-management fault, 6 bus fault, 7 usage fault, 8 NMI, 9 clock start failure, 10 libc exit.*/
 char fault_task[16]; /*<  Task whose stack overflowed (fault_code 2).*/
 uint8_t flags; /*<  Bitmask: 1 battery low, 2 battery critical, 4 motor fault active, 8 power board answering, 16 Ethernet link up, 32 board has range sensors, 64 board has a power board, 128 board has Ethernet.*/
} mavlink_rosbot_diagnostics_t;

#define MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_LEN 86
#define MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_MIN_LEN 86
#define MAVLINK_MSG_ID_11030_LEN 86
#define MAVLINK_MSG_ID_11030_MIN_LEN 86

#define MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_CRC 190
#define MAVLINK_MSG_ID_11030_CRC 190

#define MAVLINK_MSG_ROSBOT_DIAGNOSTICS_FIELD_RANGE_TIMEOUTS_LEN 4
#define MAVLINK_MSG_ROSBOT_DIAGNOSTICS_FIELD_I2C_RECOVERIES_LEN 2
#define MAVLINK_MSG_ROSBOT_DIAGNOSTICS_FIELD_STACK_MIN_TASK_LEN 16
#define MAVLINK_MSG_ROSBOT_DIAGNOSTICS_FIELD_FAULT_TASK_LEN 16

#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_ROSBOT_DIAGNOSTICS { \
    11030, \
    "ROSBOT_DIAGNOSTICS", \
    20, \
    {  { "uptime_ms", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_rosbot_diagnostics_t, uptime_ms) }, \
         { "heap_free", NULL, MAVLINK_TYPE_UINT32_T, 0, 4, offsetof(mavlink_rosbot_diagnostics_t, heap_free) }, \
         { "heap_min_free", NULL, MAVLINK_TYPE_UINT32_T, 0, 8, offsetof(mavlink_rosbot_diagnostics_t, heap_min_free) }, \
         { "imu_errors", NULL, MAVLINK_TYPE_UINT32_T, 0, 12, offsetof(mavlink_rosbot_diagnostics_t, imu_errors) }, \
         { "link_tx_dropped", NULL, MAVLINK_TYPE_UINT32_T, 0, 16, offsetof(mavlink_rosbot_diagnostics_t, link_tx_dropped) }, \
         { "link_rx_errors", NULL, MAVLINK_TYPE_UINT32_T, 0, 20, offsetof(mavlink_rosbot_diagnostics_t, link_rx_errors) }, \
         { "reset_cause", NULL, MAVLINK_TYPE_UINT16_T, 0, 28, offsetof(mavlink_rosbot_diagnostics_t, reset_cause) }, \
         { "cpu_idle", NULL, MAVLINK_TYPE_UINT16_T, 0, 30, offsetof(mavlink_rosbot_diagnostics_t, cpu_idle) }, \
         { "stack_min_free", NULL, MAVLINK_TYPE_UINT16_T, 0, 32, offsetof(mavlink_rosbot_diagnostics_t, stack_min_free) }, \
         { "range_timeouts", NULL, MAVLINK_TYPE_UINT16_T, 4, 34, offsetof(mavlink_rosbot_diagnostics_t, range_timeouts) }, \
         { "i2c_recoveries", NULL, MAVLINK_TYPE_UINT16_T, 2, 42, offsetof(mavlink_rosbot_diagnostics_t, i2c_recoveries) }, \
         { "motor_faults", NULL, MAVLINK_TYPE_UINT16_T, 0, 46, offsetof(mavlink_rosbot_diagnostics_t, motor_faults) }, \
         { "link_reconnects", NULL, MAVLINK_TYPE_UINT16_T, 0, 48, offsetof(mavlink_rosbot_diagnostics_t, link_reconnects) }, \
         { "stack_min_task", NULL, MAVLINK_TYPE_CHAR, 16, 50, offsetof(mavlink_rosbot_diagnostics_t, stack_min_task) }, \
         { "imu_ok", NULL, MAVLINK_TYPE_UINT8_T, 0, 66, offsetof(mavlink_rosbot_diagnostics_t, imu_ok) }, \
         { "range_ok", NULL, MAVLINK_TYPE_UINT8_T, 0, 67, offsetof(mavlink_rosbot_diagnostics_t, range_ok) }, \
         { "fault_pc", NULL, MAVLINK_TYPE_UINT32_T, 0, 24, offsetof(mavlink_rosbot_diagnostics_t, fault_pc) }, \
         { "fault_code", NULL, MAVLINK_TYPE_UINT8_T, 0, 68, offsetof(mavlink_rosbot_diagnostics_t, fault_code) }, \
         { "fault_task", NULL, MAVLINK_TYPE_CHAR, 16, 69, offsetof(mavlink_rosbot_diagnostics_t, fault_task) }, \
         { "flags", NULL, MAVLINK_TYPE_UINT8_T, 0, 85, offsetof(mavlink_rosbot_diagnostics_t, flags) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_ROSBOT_DIAGNOSTICS { \
    "ROSBOT_DIAGNOSTICS", \
    20, \
    {  { "uptime_ms", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_rosbot_diagnostics_t, uptime_ms) }, \
         { "heap_free", NULL, MAVLINK_TYPE_UINT32_T, 0, 4, offsetof(mavlink_rosbot_diagnostics_t, heap_free) }, \
         { "heap_min_free", NULL, MAVLINK_TYPE_UINT32_T, 0, 8, offsetof(mavlink_rosbot_diagnostics_t, heap_min_free) }, \
         { "imu_errors", NULL, MAVLINK_TYPE_UINT32_T, 0, 12, offsetof(mavlink_rosbot_diagnostics_t, imu_errors) }, \
         { "link_tx_dropped", NULL, MAVLINK_TYPE_UINT32_T, 0, 16, offsetof(mavlink_rosbot_diagnostics_t, link_tx_dropped) }, \
         { "link_rx_errors", NULL, MAVLINK_TYPE_UINT32_T, 0, 20, offsetof(mavlink_rosbot_diagnostics_t, link_rx_errors) }, \
         { "reset_cause", NULL, MAVLINK_TYPE_UINT16_T, 0, 28, offsetof(mavlink_rosbot_diagnostics_t, reset_cause) }, \
         { "cpu_idle", NULL, MAVLINK_TYPE_UINT16_T, 0, 30, offsetof(mavlink_rosbot_diagnostics_t, cpu_idle) }, \
         { "stack_min_free", NULL, MAVLINK_TYPE_UINT16_T, 0, 32, offsetof(mavlink_rosbot_diagnostics_t, stack_min_free) }, \
         { "range_timeouts", NULL, MAVLINK_TYPE_UINT16_T, 4, 34, offsetof(mavlink_rosbot_diagnostics_t, range_timeouts) }, \
         { "i2c_recoveries", NULL, MAVLINK_TYPE_UINT16_T, 2, 42, offsetof(mavlink_rosbot_diagnostics_t, i2c_recoveries) }, \
         { "motor_faults", NULL, MAVLINK_TYPE_UINT16_T, 0, 46, offsetof(mavlink_rosbot_diagnostics_t, motor_faults) }, \
         { "link_reconnects", NULL, MAVLINK_TYPE_UINT16_T, 0, 48, offsetof(mavlink_rosbot_diagnostics_t, link_reconnects) }, \
         { "stack_min_task", NULL, MAVLINK_TYPE_CHAR, 16, 50, offsetof(mavlink_rosbot_diagnostics_t, stack_min_task) }, \
         { "imu_ok", NULL, MAVLINK_TYPE_UINT8_T, 0, 66, offsetof(mavlink_rosbot_diagnostics_t, imu_ok) }, \
         { "range_ok", NULL, MAVLINK_TYPE_UINT8_T, 0, 67, offsetof(mavlink_rosbot_diagnostics_t, range_ok) }, \
         { "fault_pc", NULL, MAVLINK_TYPE_UINT32_T, 0, 24, offsetof(mavlink_rosbot_diagnostics_t, fault_pc) }, \
         { "fault_code", NULL, MAVLINK_TYPE_UINT8_T, 0, 68, offsetof(mavlink_rosbot_diagnostics_t, fault_code) }, \
         { "fault_task", NULL, MAVLINK_TYPE_CHAR, 16, 69, offsetof(mavlink_rosbot_diagnostics_t, fault_task) }, \
         { "flags", NULL, MAVLINK_TYPE_UINT8_T, 0, 85, offsetof(mavlink_rosbot_diagnostics_t, flags) }, \
         } \
}
#endif

/**
 * @brief Pack a rosbot_diagnostics message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param uptime_ms [ms] Time since MCU boot.
 * @param heap_free [bytes] Free heap now.
 * @param heap_min_free [bytes] Heap never handed out since boot, a lower bound of the free heap.
 * @param imu_errors  Failed IMU reads.
 * @param link_tx_dropped  Telemetry frames dropped because the link TX queue was full.
 * @param link_rx_errors  Incoming MAVLink frames the parser dropped (CRC or framing).
 * @param reset_cause  Cause of the last MCU reset, bitmask: 1 reset pin, 2 power-on or brown-out, 4 software, 8 independent watchdog, 16 window watchdog, 32 low-power, 64 firmware fault (fatalError).
 * @param cpu_idle [d%] CPU idle over the last report period, 0.1 % units; 65535 in the first message after boot (no period yet).
 * @param stack_min_free [bytes] Smallest stack headroom of any task since boot.
 * @param range_timeouts  Range sensor reads that timed out, order FL, FR, RL, RR (ROSbot; zero elsewhere).
 * @param i2c_recoveries  I2C bus resets after faults: IMU bus, sensor/EEPROM bus.
 * @param motor_faults  Motor driver fault events (nFAULT asserted).
 * @param link_reconnects  Times the bridge connection was (re)established.
 * @param stack_min_task  Name of the task with the smallest stack headroom.
 * @param imu_ok  1 when the IMU initialised and its last read succeeded.
 * @param range_ok  Bitmask of range sensors that initialised, bit 0 = FL.
 * @param fault_pc  Program counter of the exception that caused the last firmware fault, 0 if not an exception.
 * @param fault_code  Why the last firmware fault reset happened: 0 none or unknown, 1 assert, 2 stack overflow, 3 out of heap, 4 hard fault, 5 memory-management fault, 6 bus fault, 7 usage fault, 8 NMI, 9 clock start failure, 10 libc exit.
 * @param fault_task  Task whose stack overflowed (fault_code 2).
 * @param flags  Bitmask: 1 battery low, 2 battery critical, 4 motor fault active, 8 power board answering, 16 Ethernet link up, 32 board has range sensors, 64 board has a power board, 128 board has Ethernet.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_diagnostics_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint32_t uptime_ms, uint32_t heap_free, uint32_t heap_min_free, uint32_t imu_errors, uint32_t link_tx_dropped, uint32_t link_rx_errors, uint16_t reset_cause, uint16_t cpu_idle, uint16_t stack_min_free, const uint16_t *range_timeouts, const uint16_t *i2c_recoveries, uint16_t motor_faults, uint16_t link_reconnects, const char *stack_min_task, uint8_t imu_ok, uint8_t range_ok, uint32_t fault_pc, uint8_t fault_code, const char *fault_task, uint8_t flags)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_LEN];
    _mav_put_uint32_t(buf, 0, uptime_ms);
    _mav_put_uint32_t(buf, 4, heap_free);
    _mav_put_uint32_t(buf, 8, heap_min_free);
    _mav_put_uint32_t(buf, 12, imu_errors);
    _mav_put_uint32_t(buf, 16, link_tx_dropped);
    _mav_put_uint32_t(buf, 20, link_rx_errors);
    _mav_put_uint32_t(buf, 24, fault_pc);
    _mav_put_uint16_t(buf, 28, reset_cause);
    _mav_put_uint16_t(buf, 30, cpu_idle);
    _mav_put_uint16_t(buf, 32, stack_min_free);
    _mav_put_uint16_t(buf, 46, motor_faults);
    _mav_put_uint16_t(buf, 48, link_reconnects);
    _mav_put_uint8_t(buf, 66, imu_ok);
    _mav_put_uint8_t(buf, 67, range_ok);
    _mav_put_uint8_t(buf, 68, fault_code);
    _mav_put_uint8_t(buf, 85, flags);
    _mav_put_uint16_t_array(buf, 34, range_timeouts, 4);
    _mav_put_uint16_t_array(buf, 42, i2c_recoveries, 2);
    _mav_put_char_array(buf, 50, stack_min_task, 16);
    _mav_put_char_array(buf, 69, fault_task, 16);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_LEN);
#else
    mavlink_rosbot_diagnostics_t packet;
    packet.uptime_ms = uptime_ms;
    packet.heap_free = heap_free;
    packet.heap_min_free = heap_min_free;
    packet.imu_errors = imu_errors;
    packet.link_tx_dropped = link_tx_dropped;
    packet.link_rx_errors = link_rx_errors;
    packet.fault_pc = fault_pc;
    packet.reset_cause = reset_cause;
    packet.cpu_idle = cpu_idle;
    packet.stack_min_free = stack_min_free;
    packet.motor_faults = motor_faults;
    packet.link_reconnects = link_reconnects;
    packet.imu_ok = imu_ok;
    packet.range_ok = range_ok;
    packet.fault_code = fault_code;
    packet.flags = flags;
    mav_array_assign_uint16_t(packet.range_timeouts, range_timeouts, 4);
    mav_array_assign_uint16_t(packet.i2c_recoveries, i2c_recoveries, 2);
    mav_array_assign_char(packet.stack_min_task, stack_min_task, 16);
    mav_array_assign_char(packet.fault_task, fault_task, 16);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_LEN, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_CRC);
}

/**
 * @brief Pack a rosbot_diagnostics message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param uptime_ms [ms] Time since MCU boot.
 * @param heap_free [bytes] Free heap now.
 * @param heap_min_free [bytes] Heap never handed out since boot, a lower bound of the free heap.
 * @param imu_errors  Failed IMU reads.
 * @param link_tx_dropped  Telemetry frames dropped because the link TX queue was full.
 * @param link_rx_errors  Incoming MAVLink frames the parser dropped (CRC or framing).
 * @param reset_cause  Cause of the last MCU reset, bitmask: 1 reset pin, 2 power-on or brown-out, 4 software, 8 independent watchdog, 16 window watchdog, 32 low-power, 64 firmware fault (fatalError).
 * @param cpu_idle [d%] CPU idle over the last report period, 0.1 % units; 65535 in the first message after boot (no period yet).
 * @param stack_min_free [bytes] Smallest stack headroom of any task since boot.
 * @param range_timeouts  Range sensor reads that timed out, order FL, FR, RL, RR (ROSbot; zero elsewhere).
 * @param i2c_recoveries  I2C bus resets after faults: IMU bus, sensor/EEPROM bus.
 * @param motor_faults  Motor driver fault events (nFAULT asserted).
 * @param link_reconnects  Times the bridge connection was (re)established.
 * @param stack_min_task  Name of the task with the smallest stack headroom.
 * @param imu_ok  1 when the IMU initialised and its last read succeeded.
 * @param range_ok  Bitmask of range sensors that initialised, bit 0 = FL.
 * @param fault_pc  Program counter of the exception that caused the last firmware fault, 0 if not an exception.
 * @param fault_code  Why the last firmware fault reset happened: 0 none or unknown, 1 assert, 2 stack overflow, 3 out of heap, 4 hard fault, 5 memory-management fault, 6 bus fault, 7 usage fault, 8 NMI, 9 clock start failure, 10 libc exit.
 * @param fault_task  Task whose stack overflowed (fault_code 2).
 * @param flags  Bitmask: 1 battery low, 2 battery critical, 4 motor fault active, 8 power board answering, 16 Ethernet link up, 32 board has range sensors, 64 board has a power board, 128 board has Ethernet.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_diagnostics_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint32_t uptime_ms, uint32_t heap_free, uint32_t heap_min_free, uint32_t imu_errors, uint32_t link_tx_dropped, uint32_t link_rx_errors, uint16_t reset_cause, uint16_t cpu_idle, uint16_t stack_min_free, const uint16_t *range_timeouts, const uint16_t *i2c_recoveries, uint16_t motor_faults, uint16_t link_reconnects, const char *stack_min_task, uint8_t imu_ok, uint8_t range_ok, uint32_t fault_pc, uint8_t fault_code, const char *fault_task, uint8_t flags)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_LEN];
    _mav_put_uint32_t(buf, 0, uptime_ms);
    _mav_put_uint32_t(buf, 4, heap_free);
    _mav_put_uint32_t(buf, 8, heap_min_free);
    _mav_put_uint32_t(buf, 12, imu_errors);
    _mav_put_uint32_t(buf, 16, link_tx_dropped);
    _mav_put_uint32_t(buf, 20, link_rx_errors);
    _mav_put_uint32_t(buf, 24, fault_pc);
    _mav_put_uint16_t(buf, 28, reset_cause);
    _mav_put_uint16_t(buf, 30, cpu_idle);
    _mav_put_uint16_t(buf, 32, stack_min_free);
    _mav_put_uint16_t(buf, 46, motor_faults);
    _mav_put_uint16_t(buf, 48, link_reconnects);
    _mav_put_uint8_t(buf, 66, imu_ok);
    _mav_put_uint8_t(buf, 67, range_ok);
    _mav_put_uint8_t(buf, 68, fault_code);
    _mav_put_uint8_t(buf, 85, flags);
    _mav_put_uint16_t_array(buf, 34, range_timeouts, 4);
    _mav_put_uint16_t_array(buf, 42, i2c_recoveries, 2);
    _mav_put_char_array(buf, 50, stack_min_task, 16);
    _mav_put_char_array(buf, 69, fault_task, 16);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_LEN);
#else
    mavlink_rosbot_diagnostics_t packet;
    packet.uptime_ms = uptime_ms;
    packet.heap_free = heap_free;
    packet.heap_min_free = heap_min_free;
    packet.imu_errors = imu_errors;
    packet.link_tx_dropped = link_tx_dropped;
    packet.link_rx_errors = link_rx_errors;
    packet.fault_pc = fault_pc;
    packet.reset_cause = reset_cause;
    packet.cpu_idle = cpu_idle;
    packet.stack_min_free = stack_min_free;
    packet.motor_faults = motor_faults;
    packet.link_reconnects = link_reconnects;
    packet.imu_ok = imu_ok;
    packet.range_ok = range_ok;
    packet.fault_code = fault_code;
    packet.flags = flags;
    mav_array_memcpy(packet.range_timeouts, range_timeouts, sizeof(uint16_t)*4);
    mav_array_memcpy(packet.i2c_recoveries, i2c_recoveries, sizeof(uint16_t)*2);
    mav_array_memcpy(packet.stack_min_task, stack_min_task, sizeof(char)*16);
    mav_array_memcpy(packet.fault_task, fault_task, sizeof(char)*16);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_LEN, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_LEN);
#endif
}

/**
 * @brief Pack a rosbot_diagnostics message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param uptime_ms [ms] Time since MCU boot.
 * @param heap_free [bytes] Free heap now.
 * @param heap_min_free [bytes] Heap never handed out since boot, a lower bound of the free heap.
 * @param imu_errors  Failed IMU reads.
 * @param link_tx_dropped  Telemetry frames dropped because the link TX queue was full.
 * @param link_rx_errors  Incoming MAVLink frames the parser dropped (CRC or framing).
 * @param reset_cause  Cause of the last MCU reset, bitmask: 1 reset pin, 2 power-on or brown-out, 4 software, 8 independent watchdog, 16 window watchdog, 32 low-power, 64 firmware fault (fatalError).
 * @param cpu_idle [d%] CPU idle over the last report period, 0.1 % units; 65535 in the first message after boot (no period yet).
 * @param stack_min_free [bytes] Smallest stack headroom of any task since boot.
 * @param range_timeouts  Range sensor reads that timed out, order FL, FR, RL, RR (ROSbot; zero elsewhere).
 * @param i2c_recoveries  I2C bus resets after faults: IMU bus, sensor/EEPROM bus.
 * @param motor_faults  Motor driver fault events (nFAULT asserted).
 * @param link_reconnects  Times the bridge connection was (re)established.
 * @param stack_min_task  Name of the task with the smallest stack headroom.
 * @param imu_ok  1 when the IMU initialised and its last read succeeded.
 * @param range_ok  Bitmask of range sensors that initialised, bit 0 = FL.
 * @param fault_pc  Program counter of the exception that caused the last firmware fault, 0 if not an exception.
 * @param fault_code  Why the last firmware fault reset happened: 0 none or unknown, 1 assert, 2 stack overflow, 3 out of heap, 4 hard fault, 5 memory-management fault, 6 bus fault, 7 usage fault, 8 NMI, 9 clock start failure, 10 libc exit.
 * @param fault_task  Task whose stack overflowed (fault_code 2).
 * @param flags  Bitmask: 1 battery low, 2 battery critical, 4 motor fault active, 8 power board answering, 16 Ethernet link up, 32 board has range sensors, 64 board has a power board, 128 board has Ethernet.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_diagnostics_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint32_t uptime_ms,uint32_t heap_free,uint32_t heap_min_free,uint32_t imu_errors,uint32_t link_tx_dropped,uint32_t link_rx_errors,uint16_t reset_cause,uint16_t cpu_idle,uint16_t stack_min_free,const uint16_t *range_timeouts,const uint16_t *i2c_recoveries,uint16_t motor_faults,uint16_t link_reconnects,const char *stack_min_task,uint8_t imu_ok,uint8_t range_ok,uint32_t fault_pc,uint8_t fault_code,const char *fault_task,uint8_t flags)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_LEN];
    _mav_put_uint32_t(buf, 0, uptime_ms);
    _mav_put_uint32_t(buf, 4, heap_free);
    _mav_put_uint32_t(buf, 8, heap_min_free);
    _mav_put_uint32_t(buf, 12, imu_errors);
    _mav_put_uint32_t(buf, 16, link_tx_dropped);
    _mav_put_uint32_t(buf, 20, link_rx_errors);
    _mav_put_uint32_t(buf, 24, fault_pc);
    _mav_put_uint16_t(buf, 28, reset_cause);
    _mav_put_uint16_t(buf, 30, cpu_idle);
    _mav_put_uint16_t(buf, 32, stack_min_free);
    _mav_put_uint16_t(buf, 46, motor_faults);
    _mav_put_uint16_t(buf, 48, link_reconnects);
    _mav_put_uint8_t(buf, 66, imu_ok);
    _mav_put_uint8_t(buf, 67, range_ok);
    _mav_put_uint8_t(buf, 68, fault_code);
    _mav_put_uint8_t(buf, 85, flags);
    _mav_put_uint16_t_array(buf, 34, range_timeouts, 4);
    _mav_put_uint16_t_array(buf, 42, i2c_recoveries, 2);
    _mav_put_char_array(buf, 50, stack_min_task, 16);
    _mav_put_char_array(buf, 69, fault_task, 16);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_LEN);
#else
    mavlink_rosbot_diagnostics_t packet;
    packet.uptime_ms = uptime_ms;
    packet.heap_free = heap_free;
    packet.heap_min_free = heap_min_free;
    packet.imu_errors = imu_errors;
    packet.link_tx_dropped = link_tx_dropped;
    packet.link_rx_errors = link_rx_errors;
    packet.fault_pc = fault_pc;
    packet.reset_cause = reset_cause;
    packet.cpu_idle = cpu_idle;
    packet.stack_min_free = stack_min_free;
    packet.motor_faults = motor_faults;
    packet.link_reconnects = link_reconnects;
    packet.imu_ok = imu_ok;
    packet.range_ok = range_ok;
    packet.fault_code = fault_code;
    packet.flags = flags;
    mav_array_assign_uint16_t(packet.range_timeouts, range_timeouts, 4);
    mav_array_assign_uint16_t(packet.i2c_recoveries, i2c_recoveries, 2);
    mav_array_assign_char(packet.stack_min_task, stack_min_task, 16);
    mav_array_assign_char(packet.fault_task, fault_task, 16);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_LEN, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_CRC);
}

/**
 * @brief Encode a rosbot_diagnostics struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_diagnostics C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_diagnostics_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_rosbot_diagnostics_t* rosbot_diagnostics)
{
    return mavlink_msg_rosbot_diagnostics_pack(system_id, component_id, msg, rosbot_diagnostics->uptime_ms, rosbot_diagnostics->heap_free, rosbot_diagnostics->heap_min_free, rosbot_diagnostics->imu_errors, rosbot_diagnostics->link_tx_dropped, rosbot_diagnostics->link_rx_errors, rosbot_diagnostics->reset_cause, rosbot_diagnostics->cpu_idle, rosbot_diagnostics->stack_min_free, rosbot_diagnostics->range_timeouts, rosbot_diagnostics->i2c_recoveries, rosbot_diagnostics->motor_faults, rosbot_diagnostics->link_reconnects, rosbot_diagnostics->stack_min_task, rosbot_diagnostics->imu_ok, rosbot_diagnostics->range_ok, rosbot_diagnostics->fault_pc, rosbot_diagnostics->fault_code, rosbot_diagnostics->fault_task, rosbot_diagnostics->flags);
}

/**
 * @brief Encode a rosbot_diagnostics struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_diagnostics C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_diagnostics_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_rosbot_diagnostics_t* rosbot_diagnostics)
{
    return mavlink_msg_rosbot_diagnostics_pack_chan(system_id, component_id, chan, msg, rosbot_diagnostics->uptime_ms, rosbot_diagnostics->heap_free, rosbot_diagnostics->heap_min_free, rosbot_diagnostics->imu_errors, rosbot_diagnostics->link_tx_dropped, rosbot_diagnostics->link_rx_errors, rosbot_diagnostics->reset_cause, rosbot_diagnostics->cpu_idle, rosbot_diagnostics->stack_min_free, rosbot_diagnostics->range_timeouts, rosbot_diagnostics->i2c_recoveries, rosbot_diagnostics->motor_faults, rosbot_diagnostics->link_reconnects, rosbot_diagnostics->stack_min_task, rosbot_diagnostics->imu_ok, rosbot_diagnostics->range_ok, rosbot_diagnostics->fault_pc, rosbot_diagnostics->fault_code, rosbot_diagnostics->fault_task, rosbot_diagnostics->flags);
}

/**
 * @brief Encode a rosbot_diagnostics struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_diagnostics C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_diagnostics_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_rosbot_diagnostics_t* rosbot_diagnostics)
{
    return mavlink_msg_rosbot_diagnostics_pack_status(system_id, component_id, _status, msg,  rosbot_diagnostics->uptime_ms, rosbot_diagnostics->heap_free, rosbot_diagnostics->heap_min_free, rosbot_diagnostics->imu_errors, rosbot_diagnostics->link_tx_dropped, rosbot_diagnostics->link_rx_errors, rosbot_diagnostics->reset_cause, rosbot_diagnostics->cpu_idle, rosbot_diagnostics->stack_min_free, rosbot_diagnostics->range_timeouts, rosbot_diagnostics->i2c_recoveries, rosbot_diagnostics->motor_faults, rosbot_diagnostics->link_reconnects, rosbot_diagnostics->stack_min_task, rosbot_diagnostics->imu_ok, rosbot_diagnostics->range_ok, rosbot_diagnostics->fault_pc, rosbot_diagnostics->fault_code, rosbot_diagnostics->fault_task, rosbot_diagnostics->flags);
}

/**
 * @brief Send a rosbot_diagnostics message
 * @param chan MAVLink channel to send the message
 *
 * @param uptime_ms [ms] Time since MCU boot.
 * @param heap_free [bytes] Free heap now.
 * @param heap_min_free [bytes] Heap never handed out since boot, a lower bound of the free heap.
 * @param imu_errors  Failed IMU reads.
 * @param link_tx_dropped  Telemetry frames dropped because the link TX queue was full.
 * @param link_rx_errors  Incoming MAVLink frames the parser dropped (CRC or framing).
 * @param reset_cause  Cause of the last MCU reset, bitmask: 1 reset pin, 2 power-on or brown-out, 4 software, 8 independent watchdog, 16 window watchdog, 32 low-power, 64 firmware fault (fatalError).
 * @param cpu_idle [d%] CPU idle over the last report period, 0.1 % units; 65535 in the first message after boot (no period yet).
 * @param stack_min_free [bytes] Smallest stack headroom of any task since boot.
 * @param range_timeouts  Range sensor reads that timed out, order FL, FR, RL, RR (ROSbot; zero elsewhere).
 * @param i2c_recoveries  I2C bus resets after faults: IMU bus, sensor/EEPROM bus.
 * @param motor_faults  Motor driver fault events (nFAULT asserted).
 * @param link_reconnects  Times the bridge connection was (re)established.
 * @param stack_min_task  Name of the task with the smallest stack headroom.
 * @param imu_ok  1 when the IMU initialised and its last read succeeded.
 * @param range_ok  Bitmask of range sensors that initialised, bit 0 = FL.
 * @param fault_pc  Program counter of the exception that caused the last firmware fault, 0 if not an exception.
 * @param fault_code  Why the last firmware fault reset happened: 0 none or unknown, 1 assert, 2 stack overflow, 3 out of heap, 4 hard fault, 5 memory-management fault, 6 bus fault, 7 usage fault, 8 NMI, 9 clock start failure, 10 libc exit.
 * @param fault_task  Task whose stack overflowed (fault_code 2).
 * @param flags  Bitmask: 1 battery low, 2 battery critical, 4 motor fault active, 8 power board answering, 16 Ethernet link up, 32 board has range sensors, 64 board has a power board, 128 board has Ethernet.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_rosbot_diagnostics_send(mavlink_channel_t chan, uint32_t uptime_ms, uint32_t heap_free, uint32_t heap_min_free, uint32_t imu_errors, uint32_t link_tx_dropped, uint32_t link_rx_errors, uint16_t reset_cause, uint16_t cpu_idle, uint16_t stack_min_free, const uint16_t *range_timeouts, const uint16_t *i2c_recoveries, uint16_t motor_faults, uint16_t link_reconnects, const char *stack_min_task, uint8_t imu_ok, uint8_t range_ok, uint32_t fault_pc, uint8_t fault_code, const char *fault_task, uint8_t flags)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_LEN];
    _mav_put_uint32_t(buf, 0, uptime_ms);
    _mav_put_uint32_t(buf, 4, heap_free);
    _mav_put_uint32_t(buf, 8, heap_min_free);
    _mav_put_uint32_t(buf, 12, imu_errors);
    _mav_put_uint32_t(buf, 16, link_tx_dropped);
    _mav_put_uint32_t(buf, 20, link_rx_errors);
    _mav_put_uint32_t(buf, 24, fault_pc);
    _mav_put_uint16_t(buf, 28, reset_cause);
    _mav_put_uint16_t(buf, 30, cpu_idle);
    _mav_put_uint16_t(buf, 32, stack_min_free);
    _mav_put_uint16_t(buf, 46, motor_faults);
    _mav_put_uint16_t(buf, 48, link_reconnects);
    _mav_put_uint8_t(buf, 66, imu_ok);
    _mav_put_uint8_t(buf, 67, range_ok);
    _mav_put_uint8_t(buf, 68, fault_code);
    _mav_put_uint8_t(buf, 85, flags);
    _mav_put_uint16_t_array(buf, 34, range_timeouts, 4);
    _mav_put_uint16_t_array(buf, 42, i2c_recoveries, 2);
    _mav_put_char_array(buf, 50, stack_min_task, 16);
    _mav_put_char_array(buf, 69, fault_task, 16);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS, buf, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_LEN, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_CRC);
#else
    mavlink_rosbot_diagnostics_t packet;
    packet.uptime_ms = uptime_ms;
    packet.heap_free = heap_free;
    packet.heap_min_free = heap_min_free;
    packet.imu_errors = imu_errors;
    packet.link_tx_dropped = link_tx_dropped;
    packet.link_rx_errors = link_rx_errors;
    packet.fault_pc = fault_pc;
    packet.reset_cause = reset_cause;
    packet.cpu_idle = cpu_idle;
    packet.stack_min_free = stack_min_free;
    packet.motor_faults = motor_faults;
    packet.link_reconnects = link_reconnects;
    packet.imu_ok = imu_ok;
    packet.range_ok = range_ok;
    packet.fault_code = fault_code;
    packet.flags = flags;
    mav_array_assign_uint16_t(packet.range_timeouts, range_timeouts, 4);
    mav_array_assign_uint16_t(packet.i2c_recoveries, i2c_recoveries, 2);
    mav_array_assign_char(packet.stack_min_task, stack_min_task, 16);
    mav_array_assign_char(packet.fault_task, fault_task, 16);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS, (const char *)&packet, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_LEN, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_CRC);
#endif
}

/**
 * @brief Send a rosbot_diagnostics message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_rosbot_diagnostics_send_struct(mavlink_channel_t chan, const mavlink_rosbot_diagnostics_t* rosbot_diagnostics)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_rosbot_diagnostics_send(chan, rosbot_diagnostics->uptime_ms, rosbot_diagnostics->heap_free, rosbot_diagnostics->heap_min_free, rosbot_diagnostics->imu_errors, rosbot_diagnostics->link_tx_dropped, rosbot_diagnostics->link_rx_errors, rosbot_diagnostics->reset_cause, rosbot_diagnostics->cpu_idle, rosbot_diagnostics->stack_min_free, rosbot_diagnostics->range_timeouts, rosbot_diagnostics->i2c_recoveries, rosbot_diagnostics->motor_faults, rosbot_diagnostics->link_reconnects, rosbot_diagnostics->stack_min_task, rosbot_diagnostics->imu_ok, rosbot_diagnostics->range_ok, rosbot_diagnostics->fault_pc, rosbot_diagnostics->fault_code, rosbot_diagnostics->fault_task, rosbot_diagnostics->flags);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS, (const char *)rosbot_diagnostics, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_LEN, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_CRC);
#endif
}

#if MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_rosbot_diagnostics_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint32_t uptime_ms, uint32_t heap_free, uint32_t heap_min_free, uint32_t imu_errors, uint32_t link_tx_dropped, uint32_t link_rx_errors, uint16_t reset_cause, uint16_t cpu_idle, uint16_t stack_min_free, const uint16_t *range_timeouts, const uint16_t *i2c_recoveries, uint16_t motor_faults, uint16_t link_reconnects, const char *stack_min_task, uint8_t imu_ok, uint8_t range_ok, uint32_t fault_pc, uint8_t fault_code, const char *fault_task, uint8_t flags)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint32_t(buf, 0, uptime_ms);
    _mav_put_uint32_t(buf, 4, heap_free);
    _mav_put_uint32_t(buf, 8, heap_min_free);
    _mav_put_uint32_t(buf, 12, imu_errors);
    _mav_put_uint32_t(buf, 16, link_tx_dropped);
    _mav_put_uint32_t(buf, 20, link_rx_errors);
    _mav_put_uint32_t(buf, 24, fault_pc);
    _mav_put_uint16_t(buf, 28, reset_cause);
    _mav_put_uint16_t(buf, 30, cpu_idle);
    _mav_put_uint16_t(buf, 32, stack_min_free);
    _mav_put_uint16_t(buf, 46, motor_faults);
    _mav_put_uint16_t(buf, 48, link_reconnects);
    _mav_put_uint8_t(buf, 66, imu_ok);
    _mav_put_uint8_t(buf, 67, range_ok);
    _mav_put_uint8_t(buf, 68, fault_code);
    _mav_put_uint8_t(buf, 85, flags);
    _mav_put_uint16_t_array(buf, 34, range_timeouts, 4);
    _mav_put_uint16_t_array(buf, 42, i2c_recoveries, 2);
    _mav_put_char_array(buf, 50, stack_min_task, 16);
    _mav_put_char_array(buf, 69, fault_task, 16);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS, buf, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_LEN, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_CRC);
#else
    mavlink_rosbot_diagnostics_t *packet = (mavlink_rosbot_diagnostics_t *)msgbuf;
    packet->uptime_ms = uptime_ms;
    packet->heap_free = heap_free;
    packet->heap_min_free = heap_min_free;
    packet->imu_errors = imu_errors;
    packet->link_tx_dropped = link_tx_dropped;
    packet->link_rx_errors = link_rx_errors;
    packet->fault_pc = fault_pc;
    packet->reset_cause = reset_cause;
    packet->cpu_idle = cpu_idle;
    packet->stack_min_free = stack_min_free;
    packet->motor_faults = motor_faults;
    packet->link_reconnects = link_reconnects;
    packet->imu_ok = imu_ok;
    packet->range_ok = range_ok;
    packet->fault_code = fault_code;
    packet->flags = flags;
    mav_array_assign_uint16_t(packet->range_timeouts, range_timeouts, 4);
    mav_array_assign_uint16_t(packet->i2c_recoveries, i2c_recoveries, 2);
    mav_array_assign_char(packet->stack_min_task, stack_min_task, 16);
    mav_array_assign_char(packet->fault_task, fault_task, 16);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS, (const char *)packet, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_LEN, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_CRC);
#endif
}
#endif

#endif

// MESSAGE ROSBOT_DIAGNOSTICS UNPACKING


/**
 * @brief Get field uptime_ms from rosbot_diagnostics message
 *
 * @return [ms] Time since MCU boot.
 */
static inline uint32_t mavlink_msg_rosbot_diagnostics_get_uptime_ms(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  0);
}

/**
 * @brief Get field heap_free from rosbot_diagnostics message
 *
 * @return [bytes] Free heap now.
 */
static inline uint32_t mavlink_msg_rosbot_diagnostics_get_heap_free(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  4);
}

/**
 * @brief Get field heap_min_free from rosbot_diagnostics message
 *
 * @return [bytes] Heap never handed out since boot, a lower bound of the free heap.
 */
static inline uint32_t mavlink_msg_rosbot_diagnostics_get_heap_min_free(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  8);
}

/**
 * @brief Get field imu_errors from rosbot_diagnostics message
 *
 * @return  Failed IMU reads.
 */
static inline uint32_t mavlink_msg_rosbot_diagnostics_get_imu_errors(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  12);
}

/**
 * @brief Get field link_tx_dropped from rosbot_diagnostics message
 *
 * @return  Telemetry frames dropped because the link TX queue was full.
 */
static inline uint32_t mavlink_msg_rosbot_diagnostics_get_link_tx_dropped(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  16);
}

/**
 * @brief Get field link_rx_errors from rosbot_diagnostics message
 *
 * @return  Incoming MAVLink frames the parser dropped (CRC or framing).
 */
static inline uint32_t mavlink_msg_rosbot_diagnostics_get_link_rx_errors(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  20);
}

/**
 * @brief Get field reset_cause from rosbot_diagnostics message
 *
 * @return  Cause of the last MCU reset, bitmask: 1 reset pin, 2 power-on or brown-out, 4 software, 8 independent watchdog, 16 window watchdog, 32 low-power, 64 firmware fault (fatalError).
 */
static inline uint16_t mavlink_msg_rosbot_diagnostics_get_reset_cause(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  28);
}

/**
 * @brief Get field cpu_idle from rosbot_diagnostics message
 *
 * @return [d%] CPU idle over the last report period, 0.1 % units; 65535 in the first message after boot (no period yet).
 */
static inline uint16_t mavlink_msg_rosbot_diagnostics_get_cpu_idle(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  30);
}

/**
 * @brief Get field stack_min_free from rosbot_diagnostics message
 *
 * @return [bytes] Smallest stack headroom of any task since boot.
 */
static inline uint16_t mavlink_msg_rosbot_diagnostics_get_stack_min_free(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  32);
}

/**
 * @brief Get field range_timeouts from rosbot_diagnostics message
 *
 * @return  Range sensor reads that timed out, order FL, FR, RL, RR (ROSbot; zero elsewhere).
 */
static inline uint16_t mavlink_msg_rosbot_diagnostics_get_range_timeouts(const mavlink_message_t* msg, uint16_t *range_timeouts)
{
    return _MAV_RETURN_uint16_t_array(msg, range_timeouts, 4,  34);
}

/**
 * @brief Get field i2c_recoveries from rosbot_diagnostics message
 *
 * @return  I2C bus resets after faults: IMU bus, sensor/EEPROM bus.
 */
static inline uint16_t mavlink_msg_rosbot_diagnostics_get_i2c_recoveries(const mavlink_message_t* msg, uint16_t *i2c_recoveries)
{
    return _MAV_RETURN_uint16_t_array(msg, i2c_recoveries, 2,  42);
}

/**
 * @brief Get field motor_faults from rosbot_diagnostics message
 *
 * @return  Motor driver fault events (nFAULT asserted).
 */
static inline uint16_t mavlink_msg_rosbot_diagnostics_get_motor_faults(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  46);
}

/**
 * @brief Get field link_reconnects from rosbot_diagnostics message
 *
 * @return  Times the bridge connection was (re)established.
 */
static inline uint16_t mavlink_msg_rosbot_diagnostics_get_link_reconnects(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  48);
}

/**
 * @brief Get field stack_min_task from rosbot_diagnostics message
 *
 * @return  Name of the task with the smallest stack headroom.
 */
static inline uint16_t mavlink_msg_rosbot_diagnostics_get_stack_min_task(const mavlink_message_t* msg, char *stack_min_task)
{
    return _MAV_RETURN_char_array(msg, stack_min_task, 16,  50);
}

/**
 * @brief Get field imu_ok from rosbot_diagnostics message
 *
 * @return  1 when the IMU initialised and its last read succeeded.
 */
static inline uint8_t mavlink_msg_rosbot_diagnostics_get_imu_ok(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  66);
}

/**
 * @brief Get field range_ok from rosbot_diagnostics message
 *
 * @return  Bitmask of range sensors that initialised, bit 0 = FL.
 */
static inline uint8_t mavlink_msg_rosbot_diagnostics_get_range_ok(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  67);
}

/**
 * @brief Get field fault_pc from rosbot_diagnostics message
 *
 * @return  Program counter of the exception that caused the last firmware fault, 0 if not an exception.
 */
static inline uint32_t mavlink_msg_rosbot_diagnostics_get_fault_pc(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  24);
}

/**
 * @brief Get field fault_code from rosbot_diagnostics message
 *
 * @return  Why the last firmware fault reset happened: 0 none or unknown, 1 assert, 2 stack overflow, 3 out of heap, 4 hard fault, 5 memory-management fault, 6 bus fault, 7 usage fault, 8 NMI, 9 clock start failure, 10 libc exit.
 */
static inline uint8_t mavlink_msg_rosbot_diagnostics_get_fault_code(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  68);
}

/**
 * @brief Get field fault_task from rosbot_diagnostics message
 *
 * @return  Task whose stack overflowed (fault_code 2).
 */
static inline uint16_t mavlink_msg_rosbot_diagnostics_get_fault_task(const mavlink_message_t* msg, char *fault_task)
{
    return _MAV_RETURN_char_array(msg, fault_task, 16,  69);
}

/**
 * @brief Get field flags from rosbot_diagnostics message
 *
 * @return  Bitmask: 1 battery low, 2 battery critical, 4 motor fault active, 8 power board answering, 16 Ethernet link up, 32 board has range sensors, 64 board has a power board, 128 board has Ethernet.
 */
static inline uint8_t mavlink_msg_rosbot_diagnostics_get_flags(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  85);
}

/**
 * @brief Decode a rosbot_diagnostics message into a struct
 *
 * @param msg The message to decode
 * @param rosbot_diagnostics C-struct to decode the message contents into
 */
static inline void mavlink_msg_rosbot_diagnostics_decode(const mavlink_message_t* msg, mavlink_rosbot_diagnostics_t* rosbot_diagnostics)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    rosbot_diagnostics->uptime_ms = mavlink_msg_rosbot_diagnostics_get_uptime_ms(msg);
    rosbot_diagnostics->heap_free = mavlink_msg_rosbot_diagnostics_get_heap_free(msg);
    rosbot_diagnostics->heap_min_free = mavlink_msg_rosbot_diagnostics_get_heap_min_free(msg);
    rosbot_diagnostics->imu_errors = mavlink_msg_rosbot_diagnostics_get_imu_errors(msg);
    rosbot_diagnostics->link_tx_dropped = mavlink_msg_rosbot_diagnostics_get_link_tx_dropped(msg);
    rosbot_diagnostics->link_rx_errors = mavlink_msg_rosbot_diagnostics_get_link_rx_errors(msg);
    rosbot_diagnostics->fault_pc = mavlink_msg_rosbot_diagnostics_get_fault_pc(msg);
    rosbot_diagnostics->reset_cause = mavlink_msg_rosbot_diagnostics_get_reset_cause(msg);
    rosbot_diagnostics->cpu_idle = mavlink_msg_rosbot_diagnostics_get_cpu_idle(msg);
    rosbot_diagnostics->stack_min_free = mavlink_msg_rosbot_diagnostics_get_stack_min_free(msg);
    mavlink_msg_rosbot_diagnostics_get_range_timeouts(msg, rosbot_diagnostics->range_timeouts);
    mavlink_msg_rosbot_diagnostics_get_i2c_recoveries(msg, rosbot_diagnostics->i2c_recoveries);
    rosbot_diagnostics->motor_faults = mavlink_msg_rosbot_diagnostics_get_motor_faults(msg);
    rosbot_diagnostics->link_reconnects = mavlink_msg_rosbot_diagnostics_get_link_reconnects(msg);
    mavlink_msg_rosbot_diagnostics_get_stack_min_task(msg, rosbot_diagnostics->stack_min_task);
    rosbot_diagnostics->imu_ok = mavlink_msg_rosbot_diagnostics_get_imu_ok(msg);
    rosbot_diagnostics->range_ok = mavlink_msg_rosbot_diagnostics_get_range_ok(msg);
    rosbot_diagnostics->fault_code = mavlink_msg_rosbot_diagnostics_get_fault_code(msg);
    mavlink_msg_rosbot_diagnostics_get_fault_task(msg, rosbot_diagnostics->fault_task);
    rosbot_diagnostics->flags = mavlink_msg_rosbot_diagnostics_get_flags(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_LEN? msg->len : MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_LEN;
        memset(rosbot_diagnostics, 0, MAVLINK_MSG_ID_ROSBOT_DIAGNOSTICS_LEN);
    memcpy(rosbot_diagnostics, _MAV_PAYLOAD(msg), len);
#endif
}
