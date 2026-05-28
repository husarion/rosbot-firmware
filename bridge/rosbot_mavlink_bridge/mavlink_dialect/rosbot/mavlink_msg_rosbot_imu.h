#pragma once
// MESSAGE ROSBOT_IMU PACKING

#define MAVLINK_MSG_ID_ROSBOT_IMU 11001


typedef struct __mavlink_rosbot_imu_t {
 uint64_t time_boot_us; /*<  Timestamp since MCU boot [us].*/
 float quaternion[4]; /*<  Orientation {x,y,z,w}.*/
 float angular_velocity[3]; /*<  Gyro [rad/s], IMU frame.*/
 float linear_acceleration[3]; /*<  Accel [m/s^2], IMU frame.*/
} mavlink_rosbot_imu_t;

#define MAVLINK_MSG_ID_ROSBOT_IMU_LEN 48
#define MAVLINK_MSG_ID_ROSBOT_IMU_MIN_LEN 48
#define MAVLINK_MSG_ID_11001_LEN 48
#define MAVLINK_MSG_ID_11001_MIN_LEN 48

#define MAVLINK_MSG_ID_ROSBOT_IMU_CRC 251
#define MAVLINK_MSG_ID_11001_CRC 251

#define MAVLINK_MSG_ROSBOT_IMU_FIELD_QUATERNION_LEN 4
#define MAVLINK_MSG_ROSBOT_IMU_FIELD_ANGULAR_VELOCITY_LEN 3
#define MAVLINK_MSG_ROSBOT_IMU_FIELD_LINEAR_ACCELERATION_LEN 3

#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_ROSBOT_IMU { \
    11001, \
    "ROSBOT_IMU", \
    4, \
    {  { "time_boot_us", NULL, MAVLINK_TYPE_UINT64_T, 0, 0, offsetof(mavlink_rosbot_imu_t, time_boot_us) }, \
         { "quaternion", NULL, MAVLINK_TYPE_FLOAT, 4, 8, offsetof(mavlink_rosbot_imu_t, quaternion) }, \
         { "angular_velocity", NULL, MAVLINK_TYPE_FLOAT, 3, 24, offsetof(mavlink_rosbot_imu_t, angular_velocity) }, \
         { "linear_acceleration", NULL, MAVLINK_TYPE_FLOAT, 3, 36, offsetof(mavlink_rosbot_imu_t, linear_acceleration) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_ROSBOT_IMU { \
    "ROSBOT_IMU", \
    4, \
    {  { "time_boot_us", NULL, MAVLINK_TYPE_UINT64_T, 0, 0, offsetof(mavlink_rosbot_imu_t, time_boot_us) }, \
         { "quaternion", NULL, MAVLINK_TYPE_FLOAT, 4, 8, offsetof(mavlink_rosbot_imu_t, quaternion) }, \
         { "angular_velocity", NULL, MAVLINK_TYPE_FLOAT, 3, 24, offsetof(mavlink_rosbot_imu_t, angular_velocity) }, \
         { "linear_acceleration", NULL, MAVLINK_TYPE_FLOAT, 3, 36, offsetof(mavlink_rosbot_imu_t, linear_acceleration) }, \
         } \
}
#endif

/**
 * @brief Pack a rosbot_imu message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_us  Timestamp since MCU boot [us].
 * @param quaternion  Orientation {x,y,z,w}.
 * @param angular_velocity  Gyro [rad/s], IMU frame.
 * @param linear_acceleration  Accel [m/s^2], IMU frame.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_imu_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint64_t time_boot_us, const float *quaternion, const float *angular_velocity, const float *linear_acceleration)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_IMU_LEN];
    _mav_put_uint64_t(buf, 0, time_boot_us);
    _mav_put_float_array(buf, 8, quaternion, 4);
    _mav_put_float_array(buf, 24, angular_velocity, 3);
    _mav_put_float_array(buf, 36, linear_acceleration, 3);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_IMU_LEN);
#else
    mavlink_rosbot_imu_t packet;
    packet.time_boot_us = time_boot_us;
    mav_array_assign_float(packet.quaternion, quaternion, 4);
    mav_array_assign_float(packet.angular_velocity, angular_velocity, 3);
    mav_array_assign_float(packet.linear_acceleration, linear_acceleration, 3);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_IMU_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_IMU;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_ROSBOT_IMU_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_CRC);
}

/**
 * @brief Pack a rosbot_imu message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_us  Timestamp since MCU boot [us].
 * @param quaternion  Orientation {x,y,z,w}.
 * @param angular_velocity  Gyro [rad/s], IMU frame.
 * @param linear_acceleration  Accel [m/s^2], IMU frame.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_imu_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint64_t time_boot_us, const float *quaternion, const float *angular_velocity, const float *linear_acceleration)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_IMU_LEN];
    _mav_put_uint64_t(buf, 0, time_boot_us);
    _mav_put_float_array(buf, 8, quaternion, 4);
    _mav_put_float_array(buf, 24, angular_velocity, 3);
    _mav_put_float_array(buf, 36, linear_acceleration, 3);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_IMU_LEN);
#else
    mavlink_rosbot_imu_t packet;
    packet.time_boot_us = time_boot_us;
    mav_array_memcpy(packet.quaternion, quaternion, sizeof(float)*4);
    mav_array_memcpy(packet.angular_velocity, angular_velocity, sizeof(float)*3);
    mav_array_memcpy(packet.linear_acceleration, linear_acceleration, sizeof(float)*3);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_IMU_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_IMU;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ROSBOT_IMU_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ROSBOT_IMU_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_LEN);
#endif
}

/**
 * @brief Pack a rosbot_imu message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param time_boot_us  Timestamp since MCU boot [us].
 * @param quaternion  Orientation {x,y,z,w}.
 * @param angular_velocity  Gyro [rad/s], IMU frame.
 * @param linear_acceleration  Accel [m/s^2], IMU frame.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_imu_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint64_t time_boot_us,const float *quaternion,const float *angular_velocity,const float *linear_acceleration)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_IMU_LEN];
    _mav_put_uint64_t(buf, 0, time_boot_us);
    _mav_put_float_array(buf, 8, quaternion, 4);
    _mav_put_float_array(buf, 24, angular_velocity, 3);
    _mav_put_float_array(buf, 36, linear_acceleration, 3);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_IMU_LEN);
#else
    mavlink_rosbot_imu_t packet;
    packet.time_boot_us = time_boot_us;
    mav_array_assign_float(packet.quaternion, quaternion, 4);
    mav_array_assign_float(packet.angular_velocity, angular_velocity, 3);
    mav_array_assign_float(packet.linear_acceleration, linear_acceleration, 3);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_IMU_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_IMU;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_ROSBOT_IMU_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_CRC);
}

/**
 * @brief Encode a rosbot_imu struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_imu C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_imu_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_rosbot_imu_t* rosbot_imu)
{
    return mavlink_msg_rosbot_imu_pack(system_id, component_id, msg, rosbot_imu->time_boot_us, rosbot_imu->quaternion, rosbot_imu->angular_velocity, rosbot_imu->linear_acceleration);
}

/**
 * @brief Encode a rosbot_imu struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_imu C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_imu_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_rosbot_imu_t* rosbot_imu)
{
    return mavlink_msg_rosbot_imu_pack_chan(system_id, component_id, chan, msg, rosbot_imu->time_boot_us, rosbot_imu->quaternion, rosbot_imu->angular_velocity, rosbot_imu->linear_acceleration);
}

/**
 * @brief Encode a rosbot_imu struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_imu C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_imu_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_rosbot_imu_t* rosbot_imu)
{
    return mavlink_msg_rosbot_imu_pack_status(system_id, component_id, _status, msg,  rosbot_imu->time_boot_us, rosbot_imu->quaternion, rosbot_imu->angular_velocity, rosbot_imu->linear_acceleration);
}

/**
 * @brief Send a rosbot_imu message
 * @param chan MAVLink channel to send the message
 *
 * @param time_boot_us  Timestamp since MCU boot [us].
 * @param quaternion  Orientation {x,y,z,w}.
 * @param angular_velocity  Gyro [rad/s], IMU frame.
 * @param linear_acceleration  Accel [m/s^2], IMU frame.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_rosbot_imu_send(mavlink_channel_t chan, uint64_t time_boot_us, const float *quaternion, const float *angular_velocity, const float *linear_acceleration)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_IMU_LEN];
    _mav_put_uint64_t(buf, 0, time_boot_us);
    _mav_put_float_array(buf, 8, quaternion, 4);
    _mav_put_float_array(buf, 24, angular_velocity, 3);
    _mav_put_float_array(buf, 36, linear_acceleration, 3);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_IMU, buf, MAVLINK_MSG_ID_ROSBOT_IMU_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_CRC);
#else
    mavlink_rosbot_imu_t packet;
    packet.time_boot_us = time_boot_us;
    mav_array_assign_float(packet.quaternion, quaternion, 4);
    mav_array_assign_float(packet.angular_velocity, angular_velocity, 3);
    mav_array_assign_float(packet.linear_acceleration, linear_acceleration, 3);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_IMU, (const char *)&packet, MAVLINK_MSG_ID_ROSBOT_IMU_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_CRC);
#endif
}

/**
 * @brief Send a rosbot_imu message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_rosbot_imu_send_struct(mavlink_channel_t chan, const mavlink_rosbot_imu_t* rosbot_imu)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_rosbot_imu_send(chan, rosbot_imu->time_boot_us, rosbot_imu->quaternion, rosbot_imu->angular_velocity, rosbot_imu->linear_acceleration);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_IMU, (const char *)rosbot_imu, MAVLINK_MSG_ID_ROSBOT_IMU_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_CRC);
#endif
}

#if MAVLINK_MSG_ID_ROSBOT_IMU_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_rosbot_imu_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint64_t time_boot_us, const float *quaternion, const float *angular_velocity, const float *linear_acceleration)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint64_t(buf, 0, time_boot_us);
    _mav_put_float_array(buf, 8, quaternion, 4);
    _mav_put_float_array(buf, 24, angular_velocity, 3);
    _mav_put_float_array(buf, 36, linear_acceleration, 3);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_IMU, buf, MAVLINK_MSG_ID_ROSBOT_IMU_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_CRC);
#else
    mavlink_rosbot_imu_t *packet = (mavlink_rosbot_imu_t *)msgbuf;
    packet->time_boot_us = time_boot_us;
    mav_array_assign_float(packet->quaternion, quaternion, 4);
    mav_array_assign_float(packet->angular_velocity, angular_velocity, 3);
    mav_array_assign_float(packet->linear_acceleration, linear_acceleration, 3);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_IMU, (const char *)packet, MAVLINK_MSG_ID_ROSBOT_IMU_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_CRC);
#endif
}
#endif

#endif

// MESSAGE ROSBOT_IMU UNPACKING


/**
 * @brief Get field time_boot_us from rosbot_imu message
 *
 * @return  Timestamp since MCU boot [us].
 */
static inline uint64_t mavlink_msg_rosbot_imu_get_time_boot_us(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint64_t(msg,  0);
}

/**
 * @brief Get field quaternion from rosbot_imu message
 *
 * @return  Orientation {x,y,z,w}.
 */
static inline uint16_t mavlink_msg_rosbot_imu_get_quaternion(const mavlink_message_t* msg, float *quaternion)
{
    return _MAV_RETURN_float_array(msg, quaternion, 4,  8);
}

/**
 * @brief Get field angular_velocity from rosbot_imu message
 *
 * @return  Gyro [rad/s], IMU frame.
 */
static inline uint16_t mavlink_msg_rosbot_imu_get_angular_velocity(const mavlink_message_t* msg, float *angular_velocity)
{
    return _MAV_RETURN_float_array(msg, angular_velocity, 3,  24);
}

/**
 * @brief Get field linear_acceleration from rosbot_imu message
 *
 * @return  Accel [m/s^2], IMU frame.
 */
static inline uint16_t mavlink_msg_rosbot_imu_get_linear_acceleration(const mavlink_message_t* msg, float *linear_acceleration)
{
    return _MAV_RETURN_float_array(msg, linear_acceleration, 3,  36);
}

/**
 * @brief Decode a rosbot_imu message into a struct
 *
 * @param msg The message to decode
 * @param rosbot_imu C-struct to decode the message contents into
 */
static inline void mavlink_msg_rosbot_imu_decode(const mavlink_message_t* msg, mavlink_rosbot_imu_t* rosbot_imu)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    rosbot_imu->time_boot_us = mavlink_msg_rosbot_imu_get_time_boot_us(msg);
    mavlink_msg_rosbot_imu_get_quaternion(msg, rosbot_imu->quaternion);
    mavlink_msg_rosbot_imu_get_angular_velocity(msg, rosbot_imu->angular_velocity);
    mavlink_msg_rosbot_imu_get_linear_acceleration(msg, rosbot_imu->linear_acceleration);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_ROSBOT_IMU_LEN? msg->len : MAVLINK_MSG_ID_ROSBOT_IMU_LEN;
        memset(rosbot_imu, 0, MAVLINK_MSG_ID_ROSBOT_IMU_LEN);
    memcpy(rosbot_imu, _MAV_PAYLOAD(msg), len);
#endif
}
