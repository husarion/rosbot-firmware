#pragma once
// MESSAGE ROSBOT_LED_STRIP PACKING

#define MAVLINK_MSG_ID_ROSBOT_LED_STRIP 11012


typedef struct __mavlink_rosbot_led_strip_t {
 uint8_t count; /*<  Number of valid pixels (0..18).*/
 uint8_t rgb[54]; /*<  Pixel data, RGB triplets, count*3 valid bytes.*/
} mavlink_rosbot_led_strip_t;

#define MAVLINK_MSG_ID_ROSBOT_LED_STRIP_LEN 55
#define MAVLINK_MSG_ID_ROSBOT_LED_STRIP_MIN_LEN 55
#define MAVLINK_MSG_ID_11012_LEN 55
#define MAVLINK_MSG_ID_11012_MIN_LEN 55

#define MAVLINK_MSG_ID_ROSBOT_LED_STRIP_CRC 86
#define MAVLINK_MSG_ID_11012_CRC 86

#define MAVLINK_MSG_ROSBOT_LED_STRIP_FIELD_RGB_LEN 54

#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_ROSBOT_LED_STRIP { \
    11012, \
    "ROSBOT_LED_STRIP", \
    2, \
    {  { "count", NULL, MAVLINK_TYPE_UINT8_T, 0, 0, offsetof(mavlink_rosbot_led_strip_t, count) }, \
         { "rgb", NULL, MAVLINK_TYPE_UINT8_T, 54, 1, offsetof(mavlink_rosbot_led_strip_t, rgb) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_ROSBOT_LED_STRIP { \
    "ROSBOT_LED_STRIP", \
    2, \
    {  { "count", NULL, MAVLINK_TYPE_UINT8_T, 0, 0, offsetof(mavlink_rosbot_led_strip_t, count) }, \
         { "rgb", NULL, MAVLINK_TYPE_UINT8_T, 54, 1, offsetof(mavlink_rosbot_led_strip_t, rgb) }, \
         } \
}
#endif

/**
 * @brief Pack a rosbot_led_strip message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param count  Number of valid pixels (0..18).
 * @param rgb  Pixel data, RGB triplets, count*3 valid bytes.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_led_strip_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint8_t count, const uint8_t *rgb)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_LED_STRIP_LEN];
    _mav_put_uint8_t(buf, 0, count);
    _mav_put_uint8_t_array(buf, 1, rgb, 54);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_LEN);
#else
    mavlink_rosbot_led_strip_t packet;
    packet.count = count;
    mav_array_assign_uint8_t(packet.rgb, rgb, 54);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_LED_STRIP;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_LEN, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_CRC);
}

/**
 * @brief Pack a rosbot_led_strip message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param count  Number of valid pixels (0..18).
 * @param rgb  Pixel data, RGB triplets, count*3 valid bytes.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_led_strip_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint8_t count, const uint8_t *rgb)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_LED_STRIP_LEN];
    _mav_put_uint8_t(buf, 0, count);
    _mav_put_uint8_t_array(buf, 1, rgb, 54);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_LEN);
#else
    mavlink_rosbot_led_strip_t packet;
    packet.count = count;
    mav_array_memcpy(packet.rgb, rgb, sizeof(uint8_t)*54);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_LED_STRIP;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_LEN, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_LEN);
#endif
}

/**
 * @brief Pack a rosbot_led_strip message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param count  Number of valid pixels (0..18).
 * @param rgb  Pixel data, RGB triplets, count*3 valid bytes.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_led_strip_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint8_t count,const uint8_t *rgb)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_LED_STRIP_LEN];
    _mav_put_uint8_t(buf, 0, count);
    _mav_put_uint8_t_array(buf, 1, rgb, 54);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_LEN);
#else
    mavlink_rosbot_led_strip_t packet;
    packet.count = count;
    mav_array_assign_uint8_t(packet.rgb, rgb, 54);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_LED_STRIP;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_LEN, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_CRC);
}

/**
 * @brief Encode a rosbot_led_strip struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_led_strip C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_led_strip_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_rosbot_led_strip_t* rosbot_led_strip)
{
    return mavlink_msg_rosbot_led_strip_pack(system_id, component_id, msg, rosbot_led_strip->count, rosbot_led_strip->rgb);
}

/**
 * @brief Encode a rosbot_led_strip struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_led_strip C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_led_strip_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_rosbot_led_strip_t* rosbot_led_strip)
{
    return mavlink_msg_rosbot_led_strip_pack_chan(system_id, component_id, chan, msg, rosbot_led_strip->count, rosbot_led_strip->rgb);
}

/**
 * @brief Encode a rosbot_led_strip struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_led_strip C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_led_strip_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_rosbot_led_strip_t* rosbot_led_strip)
{
    return mavlink_msg_rosbot_led_strip_pack_status(system_id, component_id, _status, msg,  rosbot_led_strip->count, rosbot_led_strip->rgb);
}

/**
 * @brief Send a rosbot_led_strip message
 * @param chan MAVLink channel to send the message
 *
 * @param count  Number of valid pixels (0..18).
 * @param rgb  Pixel data, RGB triplets, count*3 valid bytes.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_rosbot_led_strip_send(mavlink_channel_t chan, uint8_t count, const uint8_t *rgb)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_LED_STRIP_LEN];
    _mav_put_uint8_t(buf, 0, count);
    _mav_put_uint8_t_array(buf, 1, rgb, 54);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_LED_STRIP, buf, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_LEN, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_CRC);
#else
    mavlink_rosbot_led_strip_t packet;
    packet.count = count;
    mav_array_assign_uint8_t(packet.rgb, rgb, 54);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_LED_STRIP, (const char *)&packet, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_LEN, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_CRC);
#endif
}

/**
 * @brief Send a rosbot_led_strip message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_rosbot_led_strip_send_struct(mavlink_channel_t chan, const mavlink_rosbot_led_strip_t* rosbot_led_strip)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_rosbot_led_strip_send(chan, rosbot_led_strip->count, rosbot_led_strip->rgb);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_LED_STRIP, (const char *)rosbot_led_strip, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_LEN, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_CRC);
#endif
}

#if MAVLINK_MSG_ID_ROSBOT_LED_STRIP_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_rosbot_led_strip_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint8_t count, const uint8_t *rgb)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint8_t(buf, 0, count);
    _mav_put_uint8_t_array(buf, 1, rgb, 54);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_LED_STRIP, buf, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_LEN, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_CRC);
#else
    mavlink_rosbot_led_strip_t *packet = (mavlink_rosbot_led_strip_t *)msgbuf;
    packet->count = count;
    mav_array_assign_uint8_t(packet->rgb, rgb, 54);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_LED_STRIP, (const char *)packet, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_LEN, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_CRC);
#endif
}
#endif

#endif

// MESSAGE ROSBOT_LED_STRIP UNPACKING


/**
 * @brief Get field count from rosbot_led_strip message
 *
 * @return  Number of valid pixels (0..18).
 */
static inline uint8_t mavlink_msg_rosbot_led_strip_get_count(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  0);
}

/**
 * @brief Get field rgb from rosbot_led_strip message
 *
 * @return  Pixel data, RGB triplets, count*3 valid bytes.
 */
static inline uint16_t mavlink_msg_rosbot_led_strip_get_rgb(const mavlink_message_t* msg, uint8_t *rgb)
{
    return _MAV_RETURN_uint8_t_array(msg, rgb, 54,  1);
}

/**
 * @brief Decode a rosbot_led_strip message into a struct
 *
 * @param msg The message to decode
 * @param rosbot_led_strip C-struct to decode the message contents into
 */
static inline void mavlink_msg_rosbot_led_strip_decode(const mavlink_message_t* msg, mavlink_rosbot_led_strip_t* rosbot_led_strip)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    rosbot_led_strip->count = mavlink_msg_rosbot_led_strip_get_count(msg);
    mavlink_msg_rosbot_led_strip_get_rgb(msg, rosbot_led_strip->rgb);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_ROSBOT_LED_STRIP_LEN? msg->len : MAVLINK_MSG_ID_ROSBOT_LED_STRIP_LEN;
        memset(rosbot_led_strip, 0, MAVLINK_MSG_ID_ROSBOT_LED_STRIP_LEN);
    memcpy(rosbot_led_strip, _MAV_PAYLOAD(msg), len);
#endif
}
