#pragma once
// MESSAGE ROSBOT_WHEEL_SETPOINTS PACKING

#define MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS 11010


typedef struct __mavlink_rosbot_wheel_setpoints_t {
 uint64_t time_boot_us; /*<  Bridge-side stamp for diagnostics [us].*/
 float velocity[4]; /*<  Target wheel angular velocity [rad/s].*/
} mavlink_rosbot_wheel_setpoints_t;

#define MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_LEN 24
#define MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_MIN_LEN 24
#define MAVLINK_MSG_ID_11010_LEN 24
#define MAVLINK_MSG_ID_11010_MIN_LEN 24

#define MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_CRC 52
#define MAVLINK_MSG_ID_11010_CRC 52

#define MAVLINK_MSG_ROSBOT_WHEEL_SETPOINTS_FIELD_VELOCITY_LEN 4

#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_ROSBOT_WHEEL_SETPOINTS { \
    11010, \
    "ROSBOT_WHEEL_SETPOINTS", \
    2, \
    {  { "time_boot_us", NULL, MAVLINK_TYPE_UINT64_T, 0, 0, offsetof(mavlink_rosbot_wheel_setpoints_t, time_boot_us) }, \
         { "velocity", NULL, MAVLINK_TYPE_FLOAT, 4, 8, offsetof(mavlink_rosbot_wheel_setpoints_t, velocity) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_ROSBOT_WHEEL_SETPOINTS { \
    "ROSBOT_WHEEL_SETPOINTS", \
    2, \
    {  { "time_boot_us", NULL, MAVLINK_TYPE_UINT64_T, 0, 0, offsetof(mavlink_rosbot_wheel_setpoints_t, time_boot_us) }, \
         { "velocity", NULL, MAVLINK_TYPE_FLOAT, 4, 8, offsetof(mavlink_rosbot_wheel_setpoints_t, velocity) }, \
         } \
}
#endif

/**
 * @brief Pack a rosbot_wheel_setpoints message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_us  Bridge-side stamp for diagnostics [us].
 * @param velocity  Target wheel angular velocity [rad/s].
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_wheel_setpoints_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint64_t time_boot_us, const float *velocity)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_LEN];
    _mav_put_uint64_t(buf, 0, time_boot_us);
    _mav_put_float_array(buf, 8, velocity, 4);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_LEN);
#else
    mavlink_rosbot_wheel_setpoints_t packet;
    packet.time_boot_us = time_boot_us;
    mav_array_assign_float(packet.velocity, velocity, 4);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_LEN, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_CRC);
}

/**
 * @brief Pack a rosbot_wheel_setpoints message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_us  Bridge-side stamp for diagnostics [us].
 * @param velocity  Target wheel angular velocity [rad/s].
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_wheel_setpoints_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint64_t time_boot_us, const float *velocity)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_LEN];
    _mav_put_uint64_t(buf, 0, time_boot_us);
    _mav_put_float_array(buf, 8, velocity, 4);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_LEN);
#else
    mavlink_rosbot_wheel_setpoints_t packet;
    packet.time_boot_us = time_boot_us;
    mav_array_memcpy(packet.velocity, velocity, sizeof(float)*4);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_LEN, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_LEN);
#endif
}

/**
 * @brief Pack a rosbot_wheel_setpoints message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param time_boot_us  Bridge-side stamp for diagnostics [us].
 * @param velocity  Target wheel angular velocity [rad/s].
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_wheel_setpoints_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint64_t time_boot_us,const float *velocity)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_LEN];
    _mav_put_uint64_t(buf, 0, time_boot_us);
    _mav_put_float_array(buf, 8, velocity, 4);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_LEN);
#else
    mavlink_rosbot_wheel_setpoints_t packet;
    packet.time_boot_us = time_boot_us;
    mav_array_assign_float(packet.velocity, velocity, 4);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_LEN, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_CRC);
}

/**
 * @brief Encode a rosbot_wheel_setpoints struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_wheel_setpoints C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_wheel_setpoints_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_rosbot_wheel_setpoints_t* rosbot_wheel_setpoints)
{
    return mavlink_msg_rosbot_wheel_setpoints_pack(system_id, component_id, msg, rosbot_wheel_setpoints->time_boot_us, rosbot_wheel_setpoints->velocity);
}

/**
 * @brief Encode a rosbot_wheel_setpoints struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_wheel_setpoints C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_wheel_setpoints_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_rosbot_wheel_setpoints_t* rosbot_wheel_setpoints)
{
    return mavlink_msg_rosbot_wheel_setpoints_pack_chan(system_id, component_id, chan, msg, rosbot_wheel_setpoints->time_boot_us, rosbot_wheel_setpoints->velocity);
}

/**
 * @brief Encode a rosbot_wheel_setpoints struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_wheel_setpoints C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_wheel_setpoints_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_rosbot_wheel_setpoints_t* rosbot_wheel_setpoints)
{
    return mavlink_msg_rosbot_wheel_setpoints_pack_status(system_id, component_id, _status, msg,  rosbot_wheel_setpoints->time_boot_us, rosbot_wheel_setpoints->velocity);
}

/**
 * @brief Send a rosbot_wheel_setpoints message
 * @param chan MAVLink channel to send the message
 *
 * @param time_boot_us  Bridge-side stamp for diagnostics [us].
 * @param velocity  Target wheel angular velocity [rad/s].
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_rosbot_wheel_setpoints_send(mavlink_channel_t chan, uint64_t time_boot_us, const float *velocity)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_LEN];
    _mav_put_uint64_t(buf, 0, time_boot_us);
    _mav_put_float_array(buf, 8, velocity, 4);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS, buf, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_LEN, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_CRC);
#else
    mavlink_rosbot_wheel_setpoints_t packet;
    packet.time_boot_us = time_boot_us;
    mav_array_assign_float(packet.velocity, velocity, 4);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS, (const char *)&packet, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_LEN, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_CRC);
#endif
}

/**
 * @brief Send a rosbot_wheel_setpoints message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_rosbot_wheel_setpoints_send_struct(mavlink_channel_t chan, const mavlink_rosbot_wheel_setpoints_t* rosbot_wheel_setpoints)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_rosbot_wheel_setpoints_send(chan, rosbot_wheel_setpoints->time_boot_us, rosbot_wheel_setpoints->velocity);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS, (const char *)rosbot_wheel_setpoints, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_LEN, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_CRC);
#endif
}

#if MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_rosbot_wheel_setpoints_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint64_t time_boot_us, const float *velocity)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint64_t(buf, 0, time_boot_us);
    _mav_put_float_array(buf, 8, velocity, 4);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS, buf, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_LEN, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_CRC);
#else
    mavlink_rosbot_wheel_setpoints_t *packet = (mavlink_rosbot_wheel_setpoints_t *)msgbuf;
    packet->time_boot_us = time_boot_us;
    mav_array_assign_float(packet->velocity, velocity, 4);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS, (const char *)packet, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_LEN, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_CRC);
#endif
}
#endif

#endif

// MESSAGE ROSBOT_WHEEL_SETPOINTS UNPACKING


/**
 * @brief Get field time_boot_us from rosbot_wheel_setpoints message
 *
 * @return  Bridge-side stamp for diagnostics [us].
 */
static inline uint64_t mavlink_msg_rosbot_wheel_setpoints_get_time_boot_us(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint64_t(msg,  0);
}

/**
 * @brief Get field velocity from rosbot_wheel_setpoints message
 *
 * @return  Target wheel angular velocity [rad/s].
 */
static inline uint16_t mavlink_msg_rosbot_wheel_setpoints_get_velocity(const mavlink_message_t* msg, float *velocity)
{
    return _MAV_RETURN_float_array(msg, velocity, 4,  8);
}

/**
 * @brief Decode a rosbot_wheel_setpoints message into a struct
 *
 * @param msg The message to decode
 * @param rosbot_wheel_setpoints C-struct to decode the message contents into
 */
static inline void mavlink_msg_rosbot_wheel_setpoints_decode(const mavlink_message_t* msg, mavlink_rosbot_wheel_setpoints_t* rosbot_wheel_setpoints)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    rosbot_wheel_setpoints->time_boot_us = mavlink_msg_rosbot_wheel_setpoints_get_time_boot_us(msg);
    mavlink_msg_rosbot_wheel_setpoints_get_velocity(msg, rosbot_wheel_setpoints->velocity);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_LEN? msg->len : MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_LEN;
        memset(rosbot_wheel_setpoints, 0, MAVLINK_MSG_ID_ROSBOT_WHEEL_SETPOINTS_LEN);
    memcpy(rosbot_wheel_setpoints, _MAV_PAYLOAD(msg), len);
#endif
}
