#pragma once
// MESSAGE ROSBOT_BUTTONS PACKING

#define MAVLINK_MSG_ID_ROSBOT_BUTTONS 11003


typedef struct __mavlink_rosbot_buttons_t {
 uint64_t time_boot_us; /*<  Timestamp since MCU boot [us].*/
 uint8_t mask; /*<  Pressed-button bitmask.*/
} mavlink_rosbot_buttons_t;

#define MAVLINK_MSG_ID_ROSBOT_BUTTONS_LEN 9
#define MAVLINK_MSG_ID_ROSBOT_BUTTONS_MIN_LEN 9
#define MAVLINK_MSG_ID_11003_LEN 9
#define MAVLINK_MSG_ID_11003_MIN_LEN 9

#define MAVLINK_MSG_ID_ROSBOT_BUTTONS_CRC 95
#define MAVLINK_MSG_ID_11003_CRC 95



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_ROSBOT_BUTTONS { \
    11003, \
    "ROSBOT_BUTTONS", \
    2, \
    {  { "time_boot_us", NULL, MAVLINK_TYPE_UINT64_T, 0, 0, offsetof(mavlink_rosbot_buttons_t, time_boot_us) }, \
         { "mask", NULL, MAVLINK_TYPE_UINT8_T, 0, 8, offsetof(mavlink_rosbot_buttons_t, mask) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_ROSBOT_BUTTONS { \
    "ROSBOT_BUTTONS", \
    2, \
    {  { "time_boot_us", NULL, MAVLINK_TYPE_UINT64_T, 0, 0, offsetof(mavlink_rosbot_buttons_t, time_boot_us) }, \
         { "mask", NULL, MAVLINK_TYPE_UINT8_T, 0, 8, offsetof(mavlink_rosbot_buttons_t, mask) }, \
         } \
}
#endif

/**
 * @brief Pack a rosbot_buttons message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_us  Timestamp since MCU boot [us].
 * @param mask  Pressed-button bitmask.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_buttons_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint64_t time_boot_us, uint8_t mask)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_BUTTONS_LEN];
    _mav_put_uint64_t(buf, 0, time_boot_us);
    _mav_put_uint8_t(buf, 8, mask);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_BUTTONS_LEN);
#else
    mavlink_rosbot_buttons_t packet;
    packet.time_boot_us = time_boot_us;
    packet.mask = mask;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_BUTTONS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_BUTTONS;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_ROSBOT_BUTTONS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_BUTTONS_LEN, MAVLINK_MSG_ID_ROSBOT_BUTTONS_CRC);
}

/**
 * @brief Pack a rosbot_buttons message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_us  Timestamp since MCU boot [us].
 * @param mask  Pressed-button bitmask.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_buttons_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint64_t time_boot_us, uint8_t mask)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_BUTTONS_LEN];
    _mav_put_uint64_t(buf, 0, time_boot_us);
    _mav_put_uint8_t(buf, 8, mask);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_BUTTONS_LEN);
#else
    mavlink_rosbot_buttons_t packet;
    packet.time_boot_us = time_boot_us;
    packet.mask = mask;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_BUTTONS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_BUTTONS;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ROSBOT_BUTTONS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_BUTTONS_LEN, MAVLINK_MSG_ID_ROSBOT_BUTTONS_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ROSBOT_BUTTONS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_BUTTONS_LEN);
#endif
}

/**
 * @brief Pack a rosbot_buttons message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param time_boot_us  Timestamp since MCU boot [us].
 * @param mask  Pressed-button bitmask.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_buttons_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint64_t time_boot_us,uint8_t mask)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_BUTTONS_LEN];
    _mav_put_uint64_t(buf, 0, time_boot_us);
    _mav_put_uint8_t(buf, 8, mask);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_BUTTONS_LEN);
#else
    mavlink_rosbot_buttons_t packet;
    packet.time_boot_us = time_boot_us;
    packet.mask = mask;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_BUTTONS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_BUTTONS;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_ROSBOT_BUTTONS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_BUTTONS_LEN, MAVLINK_MSG_ID_ROSBOT_BUTTONS_CRC);
}

/**
 * @brief Encode a rosbot_buttons struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_buttons C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_buttons_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_rosbot_buttons_t* rosbot_buttons)
{
    return mavlink_msg_rosbot_buttons_pack(system_id, component_id, msg, rosbot_buttons->time_boot_us, rosbot_buttons->mask);
}

/**
 * @brief Encode a rosbot_buttons struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_buttons C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_buttons_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_rosbot_buttons_t* rosbot_buttons)
{
    return mavlink_msg_rosbot_buttons_pack_chan(system_id, component_id, chan, msg, rosbot_buttons->time_boot_us, rosbot_buttons->mask);
}

/**
 * @brief Encode a rosbot_buttons struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_buttons C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_buttons_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_rosbot_buttons_t* rosbot_buttons)
{
    return mavlink_msg_rosbot_buttons_pack_status(system_id, component_id, _status, msg,  rosbot_buttons->time_boot_us, rosbot_buttons->mask);
}

/**
 * @brief Send a rosbot_buttons message
 * @param chan MAVLink channel to send the message
 *
 * @param time_boot_us  Timestamp since MCU boot [us].
 * @param mask  Pressed-button bitmask.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_rosbot_buttons_send(mavlink_channel_t chan, uint64_t time_boot_us, uint8_t mask)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_BUTTONS_LEN];
    _mav_put_uint64_t(buf, 0, time_boot_us);
    _mav_put_uint8_t(buf, 8, mask);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_BUTTONS, buf, MAVLINK_MSG_ID_ROSBOT_BUTTONS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_BUTTONS_LEN, MAVLINK_MSG_ID_ROSBOT_BUTTONS_CRC);
#else
    mavlink_rosbot_buttons_t packet;
    packet.time_boot_us = time_boot_us;
    packet.mask = mask;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_BUTTONS, (const char *)&packet, MAVLINK_MSG_ID_ROSBOT_BUTTONS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_BUTTONS_LEN, MAVLINK_MSG_ID_ROSBOT_BUTTONS_CRC);
#endif
}

/**
 * @brief Send a rosbot_buttons message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_rosbot_buttons_send_struct(mavlink_channel_t chan, const mavlink_rosbot_buttons_t* rosbot_buttons)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_rosbot_buttons_send(chan, rosbot_buttons->time_boot_us, rosbot_buttons->mask);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_BUTTONS, (const char *)rosbot_buttons, MAVLINK_MSG_ID_ROSBOT_BUTTONS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_BUTTONS_LEN, MAVLINK_MSG_ID_ROSBOT_BUTTONS_CRC);
#endif
}

#if MAVLINK_MSG_ID_ROSBOT_BUTTONS_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_rosbot_buttons_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint64_t time_boot_us, uint8_t mask)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint64_t(buf, 0, time_boot_us);
    _mav_put_uint8_t(buf, 8, mask);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_BUTTONS, buf, MAVLINK_MSG_ID_ROSBOT_BUTTONS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_BUTTONS_LEN, MAVLINK_MSG_ID_ROSBOT_BUTTONS_CRC);
#else
    mavlink_rosbot_buttons_t *packet = (mavlink_rosbot_buttons_t *)msgbuf;
    packet->time_boot_us = time_boot_us;
    packet->mask = mask;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_BUTTONS, (const char *)packet, MAVLINK_MSG_ID_ROSBOT_BUTTONS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_BUTTONS_LEN, MAVLINK_MSG_ID_ROSBOT_BUTTONS_CRC);
#endif
}
#endif

#endif

// MESSAGE ROSBOT_BUTTONS UNPACKING


/**
 * @brief Get field time_boot_us from rosbot_buttons message
 *
 * @return  Timestamp since MCU boot [us].
 */
static inline uint64_t mavlink_msg_rosbot_buttons_get_time_boot_us(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint64_t(msg,  0);
}

/**
 * @brief Get field mask from rosbot_buttons message
 *
 * @return  Pressed-button bitmask.
 */
static inline uint8_t mavlink_msg_rosbot_buttons_get_mask(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  8);
}

/**
 * @brief Decode a rosbot_buttons message into a struct
 *
 * @param msg The message to decode
 * @param rosbot_buttons C-struct to decode the message contents into
 */
static inline void mavlink_msg_rosbot_buttons_decode(const mavlink_message_t* msg, mavlink_rosbot_buttons_t* rosbot_buttons)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    rosbot_buttons->time_boot_us = mavlink_msg_rosbot_buttons_get_time_boot_us(msg);
    rosbot_buttons->mask = mavlink_msg_rosbot_buttons_get_mask(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_ROSBOT_BUTTONS_LEN? msg->len : MAVLINK_MSG_ID_ROSBOT_BUTTONS_LEN;
        memset(rosbot_buttons, 0, MAVLINK_MSG_ID_ROSBOT_BUTTONS_LEN);
    memcpy(rosbot_buttons, _MAV_PAYLOAD(msg), len);
#endif
}
