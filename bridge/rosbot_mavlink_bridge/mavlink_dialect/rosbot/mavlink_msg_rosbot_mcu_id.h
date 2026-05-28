#pragma once
// MESSAGE ROSBOT_MCU_ID PACKING

#define MAVLINK_MSG_ID_ROSBOT_MCU_ID 11020


typedef struct __mavlink_rosbot_mcu_id_t {
 char uid[24]; /*<  Hex-encoded 12-byte MCU UID.*/
} mavlink_rosbot_mcu_id_t;

#define MAVLINK_MSG_ID_ROSBOT_MCU_ID_LEN 24
#define MAVLINK_MSG_ID_ROSBOT_MCU_ID_MIN_LEN 24
#define MAVLINK_MSG_ID_11020_LEN 24
#define MAVLINK_MSG_ID_11020_MIN_LEN 24

#define MAVLINK_MSG_ID_ROSBOT_MCU_ID_CRC 247
#define MAVLINK_MSG_ID_11020_CRC 247

#define MAVLINK_MSG_ROSBOT_MCU_ID_FIELD_UID_LEN 24

#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_ROSBOT_MCU_ID { \
    11020, \
    "ROSBOT_MCU_ID", \
    1, \
    {  { "uid", NULL, MAVLINK_TYPE_CHAR, 24, 0, offsetof(mavlink_rosbot_mcu_id_t, uid) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_ROSBOT_MCU_ID { \
    "ROSBOT_MCU_ID", \
    1, \
    {  { "uid", NULL, MAVLINK_TYPE_CHAR, 24, 0, offsetof(mavlink_rosbot_mcu_id_t, uid) }, \
         } \
}
#endif

/**
 * @brief Pack a rosbot_mcu_id message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param uid  Hex-encoded 12-byte MCU UID.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_mcu_id_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               const char *uid)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_MCU_ID_LEN];

    _mav_put_char_array(buf, 0, uid, 24);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_MCU_ID_LEN);
#else
    mavlink_rosbot_mcu_id_t packet;

    mav_array_assign_char(packet.uid, uid, 24);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_MCU_ID_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_MCU_ID;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_ROSBOT_MCU_ID_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_MCU_ID_LEN, MAVLINK_MSG_ID_ROSBOT_MCU_ID_CRC);
}

/**
 * @brief Pack a rosbot_mcu_id message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param uid  Hex-encoded 12-byte MCU UID.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_mcu_id_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               const char *uid)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_MCU_ID_LEN];

    _mav_put_char_array(buf, 0, uid, 24);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_MCU_ID_LEN);
#else
    mavlink_rosbot_mcu_id_t packet;

    mav_array_memcpy(packet.uid, uid, sizeof(char)*24);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_MCU_ID_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_MCU_ID;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ROSBOT_MCU_ID_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_MCU_ID_LEN, MAVLINK_MSG_ID_ROSBOT_MCU_ID_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ROSBOT_MCU_ID_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_MCU_ID_LEN);
#endif
}

/**
 * @brief Pack a rosbot_mcu_id message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param uid  Hex-encoded 12-byte MCU UID.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_mcu_id_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   const char *uid)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_MCU_ID_LEN];

    _mav_put_char_array(buf, 0, uid, 24);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_MCU_ID_LEN);
#else
    mavlink_rosbot_mcu_id_t packet;

    mav_array_assign_char(packet.uid, uid, 24);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_MCU_ID_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_MCU_ID;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_ROSBOT_MCU_ID_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_MCU_ID_LEN, MAVLINK_MSG_ID_ROSBOT_MCU_ID_CRC);
}

/**
 * @brief Encode a rosbot_mcu_id struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_mcu_id C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_mcu_id_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_rosbot_mcu_id_t* rosbot_mcu_id)
{
    return mavlink_msg_rosbot_mcu_id_pack(system_id, component_id, msg, rosbot_mcu_id->uid);
}

/**
 * @brief Encode a rosbot_mcu_id struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_mcu_id C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_mcu_id_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_rosbot_mcu_id_t* rosbot_mcu_id)
{
    return mavlink_msg_rosbot_mcu_id_pack_chan(system_id, component_id, chan, msg, rosbot_mcu_id->uid);
}

/**
 * @brief Encode a rosbot_mcu_id struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_mcu_id C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_mcu_id_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_rosbot_mcu_id_t* rosbot_mcu_id)
{
    return mavlink_msg_rosbot_mcu_id_pack_status(system_id, component_id, _status, msg,  rosbot_mcu_id->uid);
}

/**
 * @brief Send a rosbot_mcu_id message
 * @param chan MAVLink channel to send the message
 *
 * @param uid  Hex-encoded 12-byte MCU UID.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_rosbot_mcu_id_send(mavlink_channel_t chan, const char *uid)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_MCU_ID_LEN];

    _mav_put_char_array(buf, 0, uid, 24);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_MCU_ID, buf, MAVLINK_MSG_ID_ROSBOT_MCU_ID_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_MCU_ID_LEN, MAVLINK_MSG_ID_ROSBOT_MCU_ID_CRC);
#else
    mavlink_rosbot_mcu_id_t packet;

    mav_array_assign_char(packet.uid, uid, 24);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_MCU_ID, (const char *)&packet, MAVLINK_MSG_ID_ROSBOT_MCU_ID_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_MCU_ID_LEN, MAVLINK_MSG_ID_ROSBOT_MCU_ID_CRC);
#endif
}

/**
 * @brief Send a rosbot_mcu_id message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_rosbot_mcu_id_send_struct(mavlink_channel_t chan, const mavlink_rosbot_mcu_id_t* rosbot_mcu_id)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_rosbot_mcu_id_send(chan, rosbot_mcu_id->uid);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_MCU_ID, (const char *)rosbot_mcu_id, MAVLINK_MSG_ID_ROSBOT_MCU_ID_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_MCU_ID_LEN, MAVLINK_MSG_ID_ROSBOT_MCU_ID_CRC);
#endif
}

#if MAVLINK_MSG_ID_ROSBOT_MCU_ID_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_rosbot_mcu_id_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  const char *uid)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;

    _mav_put_char_array(buf, 0, uid, 24);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_MCU_ID, buf, MAVLINK_MSG_ID_ROSBOT_MCU_ID_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_MCU_ID_LEN, MAVLINK_MSG_ID_ROSBOT_MCU_ID_CRC);
#else
    mavlink_rosbot_mcu_id_t *packet = (mavlink_rosbot_mcu_id_t *)msgbuf;

    mav_array_assign_char(packet->uid, uid, 24);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_MCU_ID, (const char *)packet, MAVLINK_MSG_ID_ROSBOT_MCU_ID_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_MCU_ID_LEN, MAVLINK_MSG_ID_ROSBOT_MCU_ID_CRC);
#endif
}
#endif

#endif

// MESSAGE ROSBOT_MCU_ID UNPACKING


/**
 * @brief Get field uid from rosbot_mcu_id message
 *
 * @return  Hex-encoded 12-byte MCU UID.
 */
static inline uint16_t mavlink_msg_rosbot_mcu_id_get_uid(const mavlink_message_t* msg, char *uid)
{
    return _MAV_RETURN_char_array(msg, uid, 24,  0);
}

/**
 * @brief Decode a rosbot_mcu_id message into a struct
 *
 * @param msg The message to decode
 * @param rosbot_mcu_id C-struct to decode the message contents into
 */
static inline void mavlink_msg_rosbot_mcu_id_decode(const mavlink_message_t* msg, mavlink_rosbot_mcu_id_t* rosbot_mcu_id)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_rosbot_mcu_id_get_uid(msg, rosbot_mcu_id->uid);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_ROSBOT_MCU_ID_LEN? msg->len : MAVLINK_MSG_ID_ROSBOT_MCU_ID_LEN;
        memset(rosbot_mcu_id, 0, MAVLINK_MSG_ID_ROSBOT_MCU_ID_LEN);
    memcpy(rosbot_mcu_id, _MAV_PAYLOAD(msg), len);
#endif
}
