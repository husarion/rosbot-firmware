#pragma once
// MESSAGE ROSBOT_PANEL_LEDS PACKING

#define MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS 11011


typedef struct __mavlink_rosbot_panel_leds_t {
 uint8_t mask; /*<  Desired LED bitmask.*/
} mavlink_rosbot_panel_leds_t;

#define MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_LEN 1
#define MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_MIN_LEN 1
#define MAVLINK_MSG_ID_11011_LEN 1
#define MAVLINK_MSG_ID_11011_MIN_LEN 1

#define MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_CRC 226
#define MAVLINK_MSG_ID_11011_CRC 226



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_ROSBOT_PANEL_LEDS { \
    11011, \
    "ROSBOT_PANEL_LEDS", \
    1, \
    {  { "mask", NULL, MAVLINK_TYPE_UINT8_T, 0, 0, offsetof(mavlink_rosbot_panel_leds_t, mask) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_ROSBOT_PANEL_LEDS { \
    "ROSBOT_PANEL_LEDS", \
    1, \
    {  { "mask", NULL, MAVLINK_TYPE_UINT8_T, 0, 0, offsetof(mavlink_rosbot_panel_leds_t, mask) }, \
         } \
}
#endif

/**
 * @brief Pack a rosbot_panel_leds message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param mask  Desired LED bitmask.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_panel_leds_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint8_t mask)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_LEN];
    _mav_put_uint8_t(buf, 0, mask);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_LEN);
#else
    mavlink_rosbot_panel_leds_t packet;
    packet.mask = mask;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_LEN, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_CRC);
}

/**
 * @brief Pack a rosbot_panel_leds message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param mask  Desired LED bitmask.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_panel_leds_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint8_t mask)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_LEN];
    _mav_put_uint8_t(buf, 0, mask);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_LEN);
#else
    mavlink_rosbot_panel_leds_t packet;
    packet.mask = mask;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_LEN, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_LEN);
#endif
}

/**
 * @brief Pack a rosbot_panel_leds message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param mask  Desired LED bitmask.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_panel_leds_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint8_t mask)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_LEN];
    _mav_put_uint8_t(buf, 0, mask);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_LEN);
#else
    mavlink_rosbot_panel_leds_t packet;
    packet.mask = mask;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_LEN, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_CRC);
}

/**
 * @brief Encode a rosbot_panel_leds struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_panel_leds C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_panel_leds_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_rosbot_panel_leds_t* rosbot_panel_leds)
{
    return mavlink_msg_rosbot_panel_leds_pack(system_id, component_id, msg, rosbot_panel_leds->mask);
}

/**
 * @brief Encode a rosbot_panel_leds struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_panel_leds C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_panel_leds_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_rosbot_panel_leds_t* rosbot_panel_leds)
{
    return mavlink_msg_rosbot_panel_leds_pack_chan(system_id, component_id, chan, msg, rosbot_panel_leds->mask);
}

/**
 * @brief Encode a rosbot_panel_leds struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_panel_leds C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_panel_leds_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_rosbot_panel_leds_t* rosbot_panel_leds)
{
    return mavlink_msg_rosbot_panel_leds_pack_status(system_id, component_id, _status, msg,  rosbot_panel_leds->mask);
}

/**
 * @brief Send a rosbot_panel_leds message
 * @param chan MAVLink channel to send the message
 *
 * @param mask  Desired LED bitmask.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_rosbot_panel_leds_send(mavlink_channel_t chan, uint8_t mask)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_LEN];
    _mav_put_uint8_t(buf, 0, mask);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS, buf, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_LEN, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_CRC);
#else
    mavlink_rosbot_panel_leds_t packet;
    packet.mask = mask;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS, (const char *)&packet, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_LEN, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_CRC);
#endif
}

/**
 * @brief Send a rosbot_panel_leds message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_rosbot_panel_leds_send_struct(mavlink_channel_t chan, const mavlink_rosbot_panel_leds_t* rosbot_panel_leds)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_rosbot_panel_leds_send(chan, rosbot_panel_leds->mask);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS, (const char *)rosbot_panel_leds, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_LEN, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_CRC);
#endif
}

#if MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_rosbot_panel_leds_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint8_t mask)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint8_t(buf, 0, mask);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS, buf, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_LEN, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_CRC);
#else
    mavlink_rosbot_panel_leds_t *packet = (mavlink_rosbot_panel_leds_t *)msgbuf;
    packet->mask = mask;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS, (const char *)packet, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_LEN, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_CRC);
#endif
}
#endif

#endif

// MESSAGE ROSBOT_PANEL_LEDS UNPACKING


/**
 * @brief Get field mask from rosbot_panel_leds message
 *
 * @return  Desired LED bitmask.
 */
static inline uint8_t mavlink_msg_rosbot_panel_leds_get_mask(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  0);
}

/**
 * @brief Decode a rosbot_panel_leds message into a struct
 *
 * @param msg The message to decode
 * @param rosbot_panel_leds C-struct to decode the message contents into
 */
static inline void mavlink_msg_rosbot_panel_leds_decode(const mavlink_message_t* msg, mavlink_rosbot_panel_leds_t* rosbot_panel_leds)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    rosbot_panel_leds->mask = mavlink_msg_rosbot_panel_leds_get_mask(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_LEN? msg->len : MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_LEN;
        memset(rosbot_panel_leds, 0, MAVLINK_MSG_ID_ROSBOT_PANEL_LEDS_LEN);
    memcpy(rosbot_panel_leds, _MAV_PAYLOAD(msg), len);
#endif
}
