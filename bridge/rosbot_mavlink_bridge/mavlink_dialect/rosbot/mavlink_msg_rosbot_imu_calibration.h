#pragma once
// MESSAGE ROSBOT_IMU_CALIBRATION PACKING

#define MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION 11004


typedef struct __mavlink_rosbot_imu_calibration_t {
 uint8_t sys; /*<  System calibration level, 0..3.*/
 uint8_t gyro; /*<  Gyroscope calibration level, 0..3.*/
 uint8_t accel; /*<  Accelerometer calibration level, 0..3.*/
 uint8_t mag; /*<  Magnetometer calibration level, 0..3.*/
 uint8_t state; /*<  Last save: 0 none, 1 saving, 2 saved, 3 rejected (not fully calibrated), 4 failed (I2C or flash).*/
 uint8_t save_seq; /*<  Incremented each time a save finishes, so a host can tell a fresh result from a stale one.*/
 uint8_t has_saved; /*<  1 when flash holds calibration offsets that are applied at boot.*/
 uint8_t session; /*<  1 while a calibration session is running.*/
} mavlink_rosbot_imu_calibration_t;

#define MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_LEN 8
#define MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_MIN_LEN 8
#define MAVLINK_MSG_ID_11004_LEN 8
#define MAVLINK_MSG_ID_11004_MIN_LEN 8

#define MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_CRC 188
#define MAVLINK_MSG_ID_11004_CRC 188



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_ROSBOT_IMU_CALIBRATION { \
    11004, \
    "ROSBOT_IMU_CALIBRATION", \
    8, \
    {  { "sys", NULL, MAVLINK_TYPE_UINT8_T, 0, 0, offsetof(mavlink_rosbot_imu_calibration_t, sys) }, \
         { "gyro", NULL, MAVLINK_TYPE_UINT8_T, 0, 1, offsetof(mavlink_rosbot_imu_calibration_t, gyro) }, \
         { "accel", NULL, MAVLINK_TYPE_UINT8_T, 0, 2, offsetof(mavlink_rosbot_imu_calibration_t, accel) }, \
         { "mag", NULL, MAVLINK_TYPE_UINT8_T, 0, 3, offsetof(mavlink_rosbot_imu_calibration_t, mag) }, \
         { "state", NULL, MAVLINK_TYPE_UINT8_T, 0, 4, offsetof(mavlink_rosbot_imu_calibration_t, state) }, \
         { "save_seq", NULL, MAVLINK_TYPE_UINT8_T, 0, 5, offsetof(mavlink_rosbot_imu_calibration_t, save_seq) }, \
         { "has_saved", NULL, MAVLINK_TYPE_UINT8_T, 0, 6, offsetof(mavlink_rosbot_imu_calibration_t, has_saved) }, \
         { "session", NULL, MAVLINK_TYPE_UINT8_T, 0, 7, offsetof(mavlink_rosbot_imu_calibration_t, session) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_ROSBOT_IMU_CALIBRATION { \
    "ROSBOT_IMU_CALIBRATION", \
    8, \
    {  { "sys", NULL, MAVLINK_TYPE_UINT8_T, 0, 0, offsetof(mavlink_rosbot_imu_calibration_t, sys) }, \
         { "gyro", NULL, MAVLINK_TYPE_UINT8_T, 0, 1, offsetof(mavlink_rosbot_imu_calibration_t, gyro) }, \
         { "accel", NULL, MAVLINK_TYPE_UINT8_T, 0, 2, offsetof(mavlink_rosbot_imu_calibration_t, accel) }, \
         { "mag", NULL, MAVLINK_TYPE_UINT8_T, 0, 3, offsetof(mavlink_rosbot_imu_calibration_t, mag) }, \
         { "state", NULL, MAVLINK_TYPE_UINT8_T, 0, 4, offsetof(mavlink_rosbot_imu_calibration_t, state) }, \
         { "save_seq", NULL, MAVLINK_TYPE_UINT8_T, 0, 5, offsetof(mavlink_rosbot_imu_calibration_t, save_seq) }, \
         { "has_saved", NULL, MAVLINK_TYPE_UINT8_T, 0, 6, offsetof(mavlink_rosbot_imu_calibration_t, has_saved) }, \
         { "session", NULL, MAVLINK_TYPE_UINT8_T, 0, 7, offsetof(mavlink_rosbot_imu_calibration_t, session) }, \
         } \
}
#endif

/**
 * @brief Pack a rosbot_imu_calibration message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param sys  System calibration level, 0..3.
 * @param gyro  Gyroscope calibration level, 0..3.
 * @param accel  Accelerometer calibration level, 0..3.
 * @param mag  Magnetometer calibration level, 0..3.
 * @param state  Last save: 0 none, 1 saving, 2 saved, 3 rejected (not fully calibrated), 4 failed (I2C or flash).
 * @param save_seq  Incremented each time a save finishes, so a host can tell a fresh result from a stale one.
 * @param has_saved  1 when flash holds calibration offsets that are applied at boot.
 * @param session  1 while a calibration session is running.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_imu_calibration_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint8_t sys, uint8_t gyro, uint8_t accel, uint8_t mag, uint8_t state, uint8_t save_seq, uint8_t has_saved, uint8_t session)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_LEN];
    _mav_put_uint8_t(buf, 0, sys);
    _mav_put_uint8_t(buf, 1, gyro);
    _mav_put_uint8_t(buf, 2, accel);
    _mav_put_uint8_t(buf, 3, mag);
    _mav_put_uint8_t(buf, 4, state);
    _mav_put_uint8_t(buf, 5, save_seq);
    _mav_put_uint8_t(buf, 6, has_saved);
    _mav_put_uint8_t(buf, 7, session);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_LEN);
#else
    mavlink_rosbot_imu_calibration_t packet;
    packet.sys = sys;
    packet.gyro = gyro;
    packet.accel = accel;
    packet.mag = mag;
    packet.state = state;
    packet.save_seq = save_seq;
    packet.has_saved = has_saved;
    packet.session = session;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_CRC);
}

/**
 * @brief Pack a rosbot_imu_calibration message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param sys  System calibration level, 0..3.
 * @param gyro  Gyroscope calibration level, 0..3.
 * @param accel  Accelerometer calibration level, 0..3.
 * @param mag  Magnetometer calibration level, 0..3.
 * @param state  Last save: 0 none, 1 saving, 2 saved, 3 rejected (not fully calibrated), 4 failed (I2C or flash).
 * @param save_seq  Incremented each time a save finishes, so a host can tell a fresh result from a stale one.
 * @param has_saved  1 when flash holds calibration offsets that are applied at boot.
 * @param session  1 while a calibration session is running.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_imu_calibration_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint8_t sys, uint8_t gyro, uint8_t accel, uint8_t mag, uint8_t state, uint8_t save_seq, uint8_t has_saved, uint8_t session)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_LEN];
    _mav_put_uint8_t(buf, 0, sys);
    _mav_put_uint8_t(buf, 1, gyro);
    _mav_put_uint8_t(buf, 2, accel);
    _mav_put_uint8_t(buf, 3, mag);
    _mav_put_uint8_t(buf, 4, state);
    _mav_put_uint8_t(buf, 5, save_seq);
    _mav_put_uint8_t(buf, 6, has_saved);
    _mav_put_uint8_t(buf, 7, session);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_LEN);
#else
    mavlink_rosbot_imu_calibration_t packet;
    packet.sys = sys;
    packet.gyro = gyro;
    packet.accel = accel;
    packet.mag = mag;
    packet.state = state;
    packet.save_seq = save_seq;
    packet.has_saved = has_saved;
    packet.session = session;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_LEN);
#endif
}

/**
 * @brief Pack a rosbot_imu_calibration message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param sys  System calibration level, 0..3.
 * @param gyro  Gyroscope calibration level, 0..3.
 * @param accel  Accelerometer calibration level, 0..3.
 * @param mag  Magnetometer calibration level, 0..3.
 * @param state  Last save: 0 none, 1 saving, 2 saved, 3 rejected (not fully calibrated), 4 failed (I2C or flash).
 * @param save_seq  Incremented each time a save finishes, so a host can tell a fresh result from a stale one.
 * @param has_saved  1 when flash holds calibration offsets that are applied at boot.
 * @param session  1 while a calibration session is running.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_imu_calibration_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint8_t sys,uint8_t gyro,uint8_t accel,uint8_t mag,uint8_t state,uint8_t save_seq,uint8_t has_saved,uint8_t session)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_LEN];
    _mav_put_uint8_t(buf, 0, sys);
    _mav_put_uint8_t(buf, 1, gyro);
    _mav_put_uint8_t(buf, 2, accel);
    _mav_put_uint8_t(buf, 3, mag);
    _mav_put_uint8_t(buf, 4, state);
    _mav_put_uint8_t(buf, 5, save_seq);
    _mav_put_uint8_t(buf, 6, has_saved);
    _mav_put_uint8_t(buf, 7, session);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_LEN);
#else
    mavlink_rosbot_imu_calibration_t packet;
    packet.sys = sys;
    packet.gyro = gyro;
    packet.accel = accel;
    packet.mag = mag;
    packet.state = state;
    packet.save_seq = save_seq;
    packet.has_saved = has_saved;
    packet.session = session;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_CRC);
}

/**
 * @brief Encode a rosbot_imu_calibration struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_imu_calibration C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_imu_calibration_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_rosbot_imu_calibration_t* rosbot_imu_calibration)
{
    return mavlink_msg_rosbot_imu_calibration_pack(system_id, component_id, msg, rosbot_imu_calibration->sys, rosbot_imu_calibration->gyro, rosbot_imu_calibration->accel, rosbot_imu_calibration->mag, rosbot_imu_calibration->state, rosbot_imu_calibration->save_seq, rosbot_imu_calibration->has_saved, rosbot_imu_calibration->session);
}

/**
 * @brief Encode a rosbot_imu_calibration struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_imu_calibration C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_imu_calibration_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_rosbot_imu_calibration_t* rosbot_imu_calibration)
{
    return mavlink_msg_rosbot_imu_calibration_pack_chan(system_id, component_id, chan, msg, rosbot_imu_calibration->sys, rosbot_imu_calibration->gyro, rosbot_imu_calibration->accel, rosbot_imu_calibration->mag, rosbot_imu_calibration->state, rosbot_imu_calibration->save_seq, rosbot_imu_calibration->has_saved, rosbot_imu_calibration->session);
}

/**
 * @brief Encode a rosbot_imu_calibration struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_imu_calibration C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_imu_calibration_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_rosbot_imu_calibration_t* rosbot_imu_calibration)
{
    return mavlink_msg_rosbot_imu_calibration_pack_status(system_id, component_id, _status, msg,  rosbot_imu_calibration->sys, rosbot_imu_calibration->gyro, rosbot_imu_calibration->accel, rosbot_imu_calibration->mag, rosbot_imu_calibration->state, rosbot_imu_calibration->save_seq, rosbot_imu_calibration->has_saved, rosbot_imu_calibration->session);
}

/**
 * @brief Send a rosbot_imu_calibration message
 * @param chan MAVLink channel to send the message
 *
 * @param sys  System calibration level, 0..3.
 * @param gyro  Gyroscope calibration level, 0..3.
 * @param accel  Accelerometer calibration level, 0..3.
 * @param mag  Magnetometer calibration level, 0..3.
 * @param state  Last save: 0 none, 1 saving, 2 saved, 3 rejected (not fully calibrated), 4 failed (I2C or flash).
 * @param save_seq  Incremented each time a save finishes, so a host can tell a fresh result from a stale one.
 * @param has_saved  1 when flash holds calibration offsets that are applied at boot.
 * @param session  1 while a calibration session is running.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_rosbot_imu_calibration_send(mavlink_channel_t chan, uint8_t sys, uint8_t gyro, uint8_t accel, uint8_t mag, uint8_t state, uint8_t save_seq, uint8_t has_saved, uint8_t session)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_LEN];
    _mav_put_uint8_t(buf, 0, sys);
    _mav_put_uint8_t(buf, 1, gyro);
    _mav_put_uint8_t(buf, 2, accel);
    _mav_put_uint8_t(buf, 3, mag);
    _mav_put_uint8_t(buf, 4, state);
    _mav_put_uint8_t(buf, 5, save_seq);
    _mav_put_uint8_t(buf, 6, has_saved);
    _mav_put_uint8_t(buf, 7, session);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION, buf, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_CRC);
#else
    mavlink_rosbot_imu_calibration_t packet;
    packet.sys = sys;
    packet.gyro = gyro;
    packet.accel = accel;
    packet.mag = mag;
    packet.state = state;
    packet.save_seq = save_seq;
    packet.has_saved = has_saved;
    packet.session = session;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION, (const char *)&packet, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_CRC);
#endif
}

/**
 * @brief Send a rosbot_imu_calibration message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_rosbot_imu_calibration_send_struct(mavlink_channel_t chan, const mavlink_rosbot_imu_calibration_t* rosbot_imu_calibration)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_rosbot_imu_calibration_send(chan, rosbot_imu_calibration->sys, rosbot_imu_calibration->gyro, rosbot_imu_calibration->accel, rosbot_imu_calibration->mag, rosbot_imu_calibration->state, rosbot_imu_calibration->save_seq, rosbot_imu_calibration->has_saved, rosbot_imu_calibration->session);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION, (const char *)rosbot_imu_calibration, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_CRC);
#endif
}

#if MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_rosbot_imu_calibration_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint8_t sys, uint8_t gyro, uint8_t accel, uint8_t mag, uint8_t state, uint8_t save_seq, uint8_t has_saved, uint8_t session)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint8_t(buf, 0, sys);
    _mav_put_uint8_t(buf, 1, gyro);
    _mav_put_uint8_t(buf, 2, accel);
    _mav_put_uint8_t(buf, 3, mag);
    _mav_put_uint8_t(buf, 4, state);
    _mav_put_uint8_t(buf, 5, save_seq);
    _mav_put_uint8_t(buf, 6, has_saved);
    _mav_put_uint8_t(buf, 7, session);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION, buf, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_CRC);
#else
    mavlink_rosbot_imu_calibration_t *packet = (mavlink_rosbot_imu_calibration_t *)msgbuf;
    packet->sys = sys;
    packet->gyro = gyro;
    packet->accel = accel;
    packet->mag = mag;
    packet->state = state;
    packet->save_seq = save_seq;
    packet->has_saved = has_saved;
    packet->session = session;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION, (const char *)packet, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_LEN, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_CRC);
#endif
}
#endif

#endif

// MESSAGE ROSBOT_IMU_CALIBRATION UNPACKING


/**
 * @brief Get field sys from rosbot_imu_calibration message
 *
 * @return  System calibration level, 0..3.
 */
static inline uint8_t mavlink_msg_rosbot_imu_calibration_get_sys(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  0);
}

/**
 * @brief Get field gyro from rosbot_imu_calibration message
 *
 * @return  Gyroscope calibration level, 0..3.
 */
static inline uint8_t mavlink_msg_rosbot_imu_calibration_get_gyro(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  1);
}

/**
 * @brief Get field accel from rosbot_imu_calibration message
 *
 * @return  Accelerometer calibration level, 0..3.
 */
static inline uint8_t mavlink_msg_rosbot_imu_calibration_get_accel(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  2);
}

/**
 * @brief Get field mag from rosbot_imu_calibration message
 *
 * @return  Magnetometer calibration level, 0..3.
 */
static inline uint8_t mavlink_msg_rosbot_imu_calibration_get_mag(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  3);
}

/**
 * @brief Get field state from rosbot_imu_calibration message
 *
 * @return  Last save: 0 none, 1 saving, 2 saved, 3 rejected (not fully calibrated), 4 failed (I2C or flash).
 */
static inline uint8_t mavlink_msg_rosbot_imu_calibration_get_state(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  4);
}

/**
 * @brief Get field save_seq from rosbot_imu_calibration message
 *
 * @return  Incremented each time a save finishes, so a host can tell a fresh result from a stale one.
 */
static inline uint8_t mavlink_msg_rosbot_imu_calibration_get_save_seq(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  5);
}

/**
 * @brief Get field has_saved from rosbot_imu_calibration message
 *
 * @return  1 when flash holds calibration offsets that are applied at boot.
 */
static inline uint8_t mavlink_msg_rosbot_imu_calibration_get_has_saved(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  6);
}

/**
 * @brief Get field session from rosbot_imu_calibration message
 *
 * @return  1 while a calibration session is running.
 */
static inline uint8_t mavlink_msg_rosbot_imu_calibration_get_session(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  7);
}

/**
 * @brief Decode a rosbot_imu_calibration message into a struct
 *
 * @param msg The message to decode
 * @param rosbot_imu_calibration C-struct to decode the message contents into
 */
static inline void mavlink_msg_rosbot_imu_calibration_decode(const mavlink_message_t* msg, mavlink_rosbot_imu_calibration_t* rosbot_imu_calibration)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    rosbot_imu_calibration->sys = mavlink_msg_rosbot_imu_calibration_get_sys(msg);
    rosbot_imu_calibration->gyro = mavlink_msg_rosbot_imu_calibration_get_gyro(msg);
    rosbot_imu_calibration->accel = mavlink_msg_rosbot_imu_calibration_get_accel(msg);
    rosbot_imu_calibration->mag = mavlink_msg_rosbot_imu_calibration_get_mag(msg);
    rosbot_imu_calibration->state = mavlink_msg_rosbot_imu_calibration_get_state(msg);
    rosbot_imu_calibration->save_seq = mavlink_msg_rosbot_imu_calibration_get_save_seq(msg);
    rosbot_imu_calibration->has_saved = mavlink_msg_rosbot_imu_calibration_get_has_saved(msg);
    rosbot_imu_calibration->session = mavlink_msg_rosbot_imu_calibration_get_session(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_LEN? msg->len : MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_LEN;
        memset(rosbot_imu_calibration, 0, MAVLINK_MSG_ID_ROSBOT_IMU_CALIBRATION_LEN);
    memcpy(rosbot_imu_calibration, _MAV_PAYLOAD(msg), len);
#endif
}
