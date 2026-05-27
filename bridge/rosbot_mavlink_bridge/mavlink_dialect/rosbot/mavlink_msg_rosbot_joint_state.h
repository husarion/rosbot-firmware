#pragma once
// MESSAGE ROSBOT_JOINT_STATE PACKING

#define MAVLINK_MSG_ID_ROSBOT_JOINT_STATE 11002


typedef struct __mavlink_rosbot_joint_state_t {
 uint64_t time_boot_us; /*<  Timestamp since MCU boot [us].*/
 float position[4]; /*<  Wheel position [rad].*/
 float velocity[4]; /*<  Wheel angular velocity [rad/s].*/
 float effort[4]; /*<  Wheel effort [Nm or PWM duty fallback].*/
} mavlink_rosbot_joint_state_t;

#define MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_LEN 56
#define MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_MIN_LEN 56
#define MAVLINK_MSG_ID_11002_LEN 56
#define MAVLINK_MSG_ID_11002_MIN_LEN 56

#define MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_CRC 57
#define MAVLINK_MSG_ID_11002_CRC 57

#define MAVLINK_MSG_ROSBOT_JOINT_STATE_FIELD_POSITION_LEN 4
#define MAVLINK_MSG_ROSBOT_JOINT_STATE_FIELD_VELOCITY_LEN 4
#define MAVLINK_MSG_ROSBOT_JOINT_STATE_FIELD_EFFORT_LEN 4

#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_ROSBOT_JOINT_STATE { \
    11002, \
    "ROSBOT_JOINT_STATE", \
    4, \
    {  { "time_boot_us", NULL, MAVLINK_TYPE_UINT64_T, 0, 0, offsetof(mavlink_rosbot_joint_state_t, time_boot_us) }, \
         { "position", NULL, MAVLINK_TYPE_FLOAT, 4, 8, offsetof(mavlink_rosbot_joint_state_t, position) }, \
         { "velocity", NULL, MAVLINK_TYPE_FLOAT, 4, 24, offsetof(mavlink_rosbot_joint_state_t, velocity) }, \
         { "effort", NULL, MAVLINK_TYPE_FLOAT, 4, 40, offsetof(mavlink_rosbot_joint_state_t, effort) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_ROSBOT_JOINT_STATE { \
    "ROSBOT_JOINT_STATE", \
    4, \
    {  { "time_boot_us", NULL, MAVLINK_TYPE_UINT64_T, 0, 0, offsetof(mavlink_rosbot_joint_state_t, time_boot_us) }, \
         { "position", NULL, MAVLINK_TYPE_FLOAT, 4, 8, offsetof(mavlink_rosbot_joint_state_t, position) }, \
         { "velocity", NULL, MAVLINK_TYPE_FLOAT, 4, 24, offsetof(mavlink_rosbot_joint_state_t, velocity) }, \
         { "effort", NULL, MAVLINK_TYPE_FLOAT, 4, 40, offsetof(mavlink_rosbot_joint_state_t, effort) }, \
         } \
}
#endif

/**
 * @brief Pack a rosbot_joint_state message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_us  Timestamp since MCU boot [us].
 * @param position  Wheel position [rad].
 * @param velocity  Wheel angular velocity [rad/s].
 * @param effort  Wheel effort [Nm or PWM duty fallback].
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_joint_state_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint64_t time_boot_us, const float *position, const float *velocity, const float *effort)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_LEN];
    _mav_put_uint64_t(buf, 0, time_boot_us);
    _mav_put_float_array(buf, 8, position, 4);
    _mav_put_float_array(buf, 24, velocity, 4);
    _mav_put_float_array(buf, 40, effort, 4);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_LEN);
#else
    mavlink_rosbot_joint_state_t packet;
    packet.time_boot_us = time_boot_us;
    mav_array_assign_float(packet.position, position, 4);
    mav_array_assign_float(packet.velocity, velocity, 4);
    mav_array_assign_float(packet.effort, effort, 4);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_JOINT_STATE;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_LEN, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_CRC);
}

/**
 * @brief Pack a rosbot_joint_state message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_us  Timestamp since MCU boot [us].
 * @param position  Wheel position [rad].
 * @param velocity  Wheel angular velocity [rad/s].
 * @param effort  Wheel effort [Nm or PWM duty fallback].
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_joint_state_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint64_t time_boot_us, const float *position, const float *velocity, const float *effort)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_LEN];
    _mav_put_uint64_t(buf, 0, time_boot_us);
    _mav_put_float_array(buf, 8, position, 4);
    _mav_put_float_array(buf, 24, velocity, 4);
    _mav_put_float_array(buf, 40, effort, 4);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_LEN);
#else
    mavlink_rosbot_joint_state_t packet;
    packet.time_boot_us = time_boot_us;
    mav_array_memcpy(packet.position, position, sizeof(float)*4);
    mav_array_memcpy(packet.velocity, velocity, sizeof(float)*4);
    mav_array_memcpy(packet.effort, effort, sizeof(float)*4);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_JOINT_STATE;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_LEN, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_LEN);
#endif
}

/**
 * @brief Pack a rosbot_joint_state message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param time_boot_us  Timestamp since MCU boot [us].
 * @param position  Wheel position [rad].
 * @param velocity  Wheel angular velocity [rad/s].
 * @param effort  Wheel effort [Nm or PWM duty fallback].
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_joint_state_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint64_t time_boot_us,const float *position,const float *velocity,const float *effort)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_LEN];
    _mav_put_uint64_t(buf, 0, time_boot_us);
    _mav_put_float_array(buf, 8, position, 4);
    _mav_put_float_array(buf, 24, velocity, 4);
    _mav_put_float_array(buf, 40, effort, 4);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_LEN);
#else
    mavlink_rosbot_joint_state_t packet;
    packet.time_boot_us = time_boot_us;
    mav_array_assign_float(packet.position, position, 4);
    mav_array_assign_float(packet.velocity, velocity, 4);
    mav_array_assign_float(packet.effort, effort, 4);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_JOINT_STATE;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_LEN, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_CRC);
}

/**
 * @brief Encode a rosbot_joint_state struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_joint_state C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_joint_state_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_rosbot_joint_state_t* rosbot_joint_state)
{
    return mavlink_msg_rosbot_joint_state_pack(system_id, component_id, msg, rosbot_joint_state->time_boot_us, rosbot_joint_state->position, rosbot_joint_state->velocity, rosbot_joint_state->effort);
}

/**
 * @brief Encode a rosbot_joint_state struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_joint_state C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_joint_state_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_rosbot_joint_state_t* rosbot_joint_state)
{
    return mavlink_msg_rosbot_joint_state_pack_chan(system_id, component_id, chan, msg, rosbot_joint_state->time_boot_us, rosbot_joint_state->position, rosbot_joint_state->velocity, rosbot_joint_state->effort);
}

/**
 * @brief Encode a rosbot_joint_state struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_joint_state C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_joint_state_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_rosbot_joint_state_t* rosbot_joint_state)
{
    return mavlink_msg_rosbot_joint_state_pack_status(system_id, component_id, _status, msg,  rosbot_joint_state->time_boot_us, rosbot_joint_state->position, rosbot_joint_state->velocity, rosbot_joint_state->effort);
}

/**
 * @brief Send a rosbot_joint_state message
 * @param chan MAVLink channel to send the message
 *
 * @param time_boot_us  Timestamp since MCU boot [us].
 * @param position  Wheel position [rad].
 * @param velocity  Wheel angular velocity [rad/s].
 * @param effort  Wheel effort [Nm or PWM duty fallback].
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_rosbot_joint_state_send(mavlink_channel_t chan, uint64_t time_boot_us, const float *position, const float *velocity, const float *effort)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_LEN];
    _mav_put_uint64_t(buf, 0, time_boot_us);
    _mav_put_float_array(buf, 8, position, 4);
    _mav_put_float_array(buf, 24, velocity, 4);
    _mav_put_float_array(buf, 40, effort, 4);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE, buf, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_LEN, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_CRC);
#else
    mavlink_rosbot_joint_state_t packet;
    packet.time_boot_us = time_boot_us;
    mav_array_assign_float(packet.position, position, 4);
    mav_array_assign_float(packet.velocity, velocity, 4);
    mav_array_assign_float(packet.effort, effort, 4);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE, (const char *)&packet, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_LEN, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_CRC);
#endif
}

/**
 * @brief Send a rosbot_joint_state message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_rosbot_joint_state_send_struct(mavlink_channel_t chan, const mavlink_rosbot_joint_state_t* rosbot_joint_state)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_rosbot_joint_state_send(chan, rosbot_joint_state->time_boot_us, rosbot_joint_state->position, rosbot_joint_state->velocity, rosbot_joint_state->effort);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE, (const char *)rosbot_joint_state, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_LEN, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_CRC);
#endif
}

#if MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_rosbot_joint_state_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint64_t time_boot_us, const float *position, const float *velocity, const float *effort)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint64_t(buf, 0, time_boot_us);
    _mav_put_float_array(buf, 8, position, 4);
    _mav_put_float_array(buf, 24, velocity, 4);
    _mav_put_float_array(buf, 40, effort, 4);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE, buf, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_LEN, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_CRC);
#else
    mavlink_rosbot_joint_state_t *packet = (mavlink_rosbot_joint_state_t *)msgbuf;
    packet->time_boot_us = time_boot_us;
    mav_array_assign_float(packet->position, position, 4);
    mav_array_assign_float(packet->velocity, velocity, 4);
    mav_array_assign_float(packet->effort, effort, 4);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE, (const char *)packet, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_LEN, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_CRC);
#endif
}
#endif

#endif

// MESSAGE ROSBOT_JOINT_STATE UNPACKING


/**
 * @brief Get field time_boot_us from rosbot_joint_state message
 *
 * @return  Timestamp since MCU boot [us].
 */
static inline uint64_t mavlink_msg_rosbot_joint_state_get_time_boot_us(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint64_t(msg,  0);
}

/**
 * @brief Get field position from rosbot_joint_state message
 *
 * @return  Wheel position [rad].
 */
static inline uint16_t mavlink_msg_rosbot_joint_state_get_position(const mavlink_message_t* msg, float *position)
{
    return _MAV_RETURN_float_array(msg, position, 4,  8);
}

/**
 * @brief Get field velocity from rosbot_joint_state message
 *
 * @return  Wheel angular velocity [rad/s].
 */
static inline uint16_t mavlink_msg_rosbot_joint_state_get_velocity(const mavlink_message_t* msg, float *velocity)
{
    return _MAV_RETURN_float_array(msg, velocity, 4,  24);
}

/**
 * @brief Get field effort from rosbot_joint_state message
 *
 * @return  Wheel effort [Nm or PWM duty fallback].
 */
static inline uint16_t mavlink_msg_rosbot_joint_state_get_effort(const mavlink_message_t* msg, float *effort)
{
    return _MAV_RETURN_float_array(msg, effort, 4,  40);
}

/**
 * @brief Decode a rosbot_joint_state message into a struct
 *
 * @param msg The message to decode
 * @param rosbot_joint_state C-struct to decode the message contents into
 */
static inline void mavlink_msg_rosbot_joint_state_decode(const mavlink_message_t* msg, mavlink_rosbot_joint_state_t* rosbot_joint_state)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    rosbot_joint_state->time_boot_us = mavlink_msg_rosbot_joint_state_get_time_boot_us(msg);
    mavlink_msg_rosbot_joint_state_get_position(msg, rosbot_joint_state->position);
    mavlink_msg_rosbot_joint_state_get_velocity(msg, rosbot_joint_state->velocity);
    mavlink_msg_rosbot_joint_state_get_effort(msg, rosbot_joint_state->effort);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_LEN? msg->len : MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_LEN;
        memset(rosbot_joint_state, 0, MAVLINK_MSG_ID_ROSBOT_JOINT_STATE_LEN);
    memcpy(rosbot_joint_state, _MAV_PAYLOAD(msg), len);
#endif
}
