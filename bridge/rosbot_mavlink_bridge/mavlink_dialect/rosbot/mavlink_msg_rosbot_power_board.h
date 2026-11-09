#pragma once
// MESSAGE ROSBOT_POWER_BOARD PACKING

#define MAVLINK_MSG_ID_ROSBOT_POWER_BOARD 11021


typedef struct __mavlink_rosbot_power_board_t {
 uint8_t valid; /*<  1 when the power board has reported its versions.*/
 char firmware_version[12]; /*<  Power board firmware version, e.g. v1.0.1.*/
 char board_version[12]; /*<  Power board hardware revision, e.g. v1.4.*/
} mavlink_rosbot_power_board_t;

#define MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_LEN 25
#define MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_MIN_LEN 25
#define MAVLINK_MSG_ID_11021_LEN 25
#define MAVLINK_MSG_ID_11021_MIN_LEN 25

#define MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_CRC 77
#define MAVLINK_MSG_ID_11021_CRC 77

#define MAVLINK_MSG_ROSBOT_POWER_BOARD_FIELD_FIRMWARE_VERSION_LEN 12
#define MAVLINK_MSG_ROSBOT_POWER_BOARD_FIELD_BOARD_VERSION_LEN 12

#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_ROSBOT_POWER_BOARD { \
    11021, \
    "ROSBOT_POWER_BOARD", \
    3, \
    {  { "valid", NULL, MAVLINK_TYPE_UINT8_T, 0, 0, offsetof(mavlink_rosbot_power_board_t, valid) }, \
         { "firmware_version", NULL, MAVLINK_TYPE_CHAR, 12, 1, offsetof(mavlink_rosbot_power_board_t, firmware_version) }, \
         { "board_version", NULL, MAVLINK_TYPE_CHAR, 12, 13, offsetof(mavlink_rosbot_power_board_t, board_version) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_ROSBOT_POWER_BOARD { \
    "ROSBOT_POWER_BOARD", \
    3, \
    {  { "valid", NULL, MAVLINK_TYPE_UINT8_T, 0, 0, offsetof(mavlink_rosbot_power_board_t, valid) }, \
         { "firmware_version", NULL, MAVLINK_TYPE_CHAR, 12, 1, offsetof(mavlink_rosbot_power_board_t, firmware_version) }, \
         { "board_version", NULL, MAVLINK_TYPE_CHAR, 12, 13, offsetof(mavlink_rosbot_power_board_t, board_version) }, \
         } \
}
#endif

/**
 * @brief Pack a rosbot_power_board message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param valid  1 when the power board has reported its versions.
 * @param firmware_version  Power board firmware version, e.g. v1.0.1.
 * @param board_version  Power board hardware revision, e.g. v1.4.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_power_board_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint8_t valid, const char *firmware_version, const char *board_version)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_LEN];
    _mav_put_uint8_t(buf, 0, valid);
    _mav_put_char_array(buf, 1, firmware_version, 12);
    _mav_put_char_array(buf, 13, board_version, 12);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_LEN);
#else
    mavlink_rosbot_power_board_t packet;
    packet.valid = valid;
    mav_array_assign_char(packet.firmware_version, firmware_version, 12);
    mav_array_assign_char(packet.board_version, board_version, 12);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_POWER_BOARD;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_CRC);
}

/**
 * @brief Pack a rosbot_power_board message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param valid  1 when the power board has reported its versions.
 * @param firmware_version  Power board firmware version, e.g. v1.0.1.
 * @param board_version  Power board hardware revision, e.g. v1.4.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_power_board_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint8_t valid, const char *firmware_version, const char *board_version)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_LEN];
    _mav_put_uint8_t(buf, 0, valid);
    _mav_put_char_array(buf, 1, firmware_version, 12);
    _mav_put_char_array(buf, 13, board_version, 12);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_LEN);
#else
    mavlink_rosbot_power_board_t packet;
    packet.valid = valid;
    mav_array_memcpy(packet.firmware_version, firmware_version, sizeof(char)*12);
    mav_array_memcpy(packet.board_version, board_version, sizeof(char)*12);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_POWER_BOARD;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_LEN);
#endif
}

/**
 * @brief Pack a rosbot_power_board message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param valid  1 when the power board has reported its versions.
 * @param firmware_version  Power board firmware version, e.g. v1.0.1.
 * @param board_version  Power board hardware revision, e.g. v1.4.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_power_board_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint8_t valid,const char *firmware_version,const char *board_version)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_LEN];
    _mav_put_uint8_t(buf, 0, valid);
    _mav_put_char_array(buf, 1, firmware_version, 12);
    _mav_put_char_array(buf, 13, board_version, 12);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_LEN);
#else
    mavlink_rosbot_power_board_t packet;
    packet.valid = valid;
    mav_array_assign_char(packet.firmware_version, firmware_version, 12);
    mav_array_assign_char(packet.board_version, board_version, 12);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_POWER_BOARD;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_CRC);
}

/**
 * @brief Encode a rosbot_power_board struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_power_board C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_power_board_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_rosbot_power_board_t* rosbot_power_board)
{
    return mavlink_msg_rosbot_power_board_pack(system_id, component_id, msg, rosbot_power_board->valid, rosbot_power_board->firmware_version, rosbot_power_board->board_version);
}

/**
 * @brief Encode a rosbot_power_board struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_power_board C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_power_board_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_rosbot_power_board_t* rosbot_power_board)
{
    return mavlink_msg_rosbot_power_board_pack_chan(system_id, component_id, chan, msg, rosbot_power_board->valid, rosbot_power_board->firmware_version, rosbot_power_board->board_version);
}

/**
 * @brief Encode a rosbot_power_board struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_power_board C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_power_board_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_rosbot_power_board_t* rosbot_power_board)
{
    return mavlink_msg_rosbot_power_board_pack_status(system_id, component_id, _status, msg,  rosbot_power_board->valid, rosbot_power_board->firmware_version, rosbot_power_board->board_version);
}

/**
 * @brief Send a rosbot_power_board message
 * @param chan MAVLink channel to send the message
 *
 * @param valid  1 when the power board has reported its versions.
 * @param firmware_version  Power board firmware version, e.g. v1.0.1.
 * @param board_version  Power board hardware revision, e.g. v1.4.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_rosbot_power_board_send(mavlink_channel_t chan, uint8_t valid, const char *firmware_version, const char *board_version)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_LEN];
    _mav_put_uint8_t(buf, 0, valid);
    _mav_put_char_array(buf, 1, firmware_version, 12);
    _mav_put_char_array(buf, 13, board_version, 12);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD, buf, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_CRC);
#else
    mavlink_rosbot_power_board_t packet;
    packet.valid = valid;
    mav_array_assign_char(packet.firmware_version, firmware_version, 12);
    mav_array_assign_char(packet.board_version, board_version, 12);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD, (const char *)&packet, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_CRC);
#endif
}

/**
 * @brief Send a rosbot_power_board message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_rosbot_power_board_send_struct(mavlink_channel_t chan, const mavlink_rosbot_power_board_t* rosbot_power_board)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_rosbot_power_board_send(chan, rosbot_power_board->valid, rosbot_power_board->firmware_version, rosbot_power_board->board_version);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD, (const char *)rosbot_power_board, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_CRC);
#endif
}

#if MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_rosbot_power_board_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint8_t valid, const char *firmware_version, const char *board_version)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint8_t(buf, 0, valid);
    _mav_put_char_array(buf, 1, firmware_version, 12);
    _mav_put_char_array(buf, 13, board_version, 12);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD, buf, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_CRC);
#else
    mavlink_rosbot_power_board_t *packet = (mavlink_rosbot_power_board_t *)msgbuf;
    packet->valid = valid;
    mav_array_assign_char(packet->firmware_version, firmware_version, 12);
    mav_array_assign_char(packet->board_version, board_version, 12);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD, (const char *)packet, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_CRC);
#endif
}
#endif

#endif

// MESSAGE ROSBOT_POWER_BOARD UNPACKING


/**
 * @brief Get field valid from rosbot_power_board message
 *
 * @return  1 when the power board has reported its versions.
 */
static inline uint8_t mavlink_msg_rosbot_power_board_get_valid(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  0);
}

/**
 * @brief Get field firmware_version from rosbot_power_board message
 *
 * @return  Power board firmware version, e.g. v1.0.1.
 */
static inline uint16_t mavlink_msg_rosbot_power_board_get_firmware_version(const mavlink_message_t* msg, char *firmware_version)
{
    return _MAV_RETURN_char_array(msg, firmware_version, 12,  1);
}

/**
 * @brief Get field board_version from rosbot_power_board message
 *
 * @return  Power board hardware revision, e.g. v1.4.
 */
static inline uint16_t mavlink_msg_rosbot_power_board_get_board_version(const mavlink_message_t* msg, char *board_version)
{
    return _MAV_RETURN_char_array(msg, board_version, 12,  13);
}

/**
 * @brief Decode a rosbot_power_board message into a struct
 *
 * @param msg The message to decode
 * @param rosbot_power_board C-struct to decode the message contents into
 */
static inline void mavlink_msg_rosbot_power_board_decode(const mavlink_message_t* msg, mavlink_rosbot_power_board_t* rosbot_power_board)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    rosbot_power_board->valid = mavlink_msg_rosbot_power_board_get_valid(msg);
    mavlink_msg_rosbot_power_board_get_firmware_version(msg, rosbot_power_board->firmware_version);
    mavlink_msg_rosbot_power_board_get_board_version(msg, rosbot_power_board->board_version);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_LEN? msg->len : MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_LEN;
        memset(rosbot_power_board, 0, MAVLINK_MSG_ID_ROSBOT_POWER_BOARD_LEN);
    memcpy(rosbot_power_board, _MAV_PAYLOAD(msg), len);
#endif
}
