#pragma once
// MESSAGE ROSBOT_POWER_SUPPLY PACKING

#define MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY 11005


typedef struct __mavlink_rosbot_power_supply_t {
 uint16_t design_capacity_mah; /*< [mAh] Pack design capacity.*/
 uint8_t status; /*<  sensor_msgs/BatteryState POWER_SUPPLY_STATUS_* value reported by the power board.*/
 uint8_t charger_type; /*<  0 none, 1 USB-C, 2 inductive, 255 unknown (not reported yet).*/
 uint8_t present; /*<  1 when a battery is connected.*/
} mavlink_rosbot_power_supply_t;

#define MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_LEN 5
#define MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_MIN_LEN 5
#define MAVLINK_MSG_ID_11005_LEN 5
#define MAVLINK_MSG_ID_11005_MIN_LEN 5

#define MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_CRC 162
#define MAVLINK_MSG_ID_11005_CRC 162



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_ROSBOT_POWER_SUPPLY { \
    11005, \
    "ROSBOT_POWER_SUPPLY", \
    4, \
    {  { "status", NULL, MAVLINK_TYPE_UINT8_T, 0, 2, offsetof(mavlink_rosbot_power_supply_t, status) }, \
         { "charger_type", NULL, MAVLINK_TYPE_UINT8_T, 0, 3, offsetof(mavlink_rosbot_power_supply_t, charger_type) }, \
         { "present", NULL, MAVLINK_TYPE_UINT8_T, 0, 4, offsetof(mavlink_rosbot_power_supply_t, present) }, \
         { "design_capacity_mah", NULL, MAVLINK_TYPE_UINT16_T, 0, 0, offsetof(mavlink_rosbot_power_supply_t, design_capacity_mah) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_ROSBOT_POWER_SUPPLY { \
    "ROSBOT_POWER_SUPPLY", \
    4, \
    {  { "status", NULL, MAVLINK_TYPE_UINT8_T, 0, 2, offsetof(mavlink_rosbot_power_supply_t, status) }, \
         { "charger_type", NULL, MAVLINK_TYPE_UINT8_T, 0, 3, offsetof(mavlink_rosbot_power_supply_t, charger_type) }, \
         { "present", NULL, MAVLINK_TYPE_UINT8_T, 0, 4, offsetof(mavlink_rosbot_power_supply_t, present) }, \
         { "design_capacity_mah", NULL, MAVLINK_TYPE_UINT16_T, 0, 0, offsetof(mavlink_rosbot_power_supply_t, design_capacity_mah) }, \
         } \
}
#endif

/**
 * @brief Pack a rosbot_power_supply message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param status  sensor_msgs/BatteryState POWER_SUPPLY_STATUS_* value reported by the power board.
 * @param charger_type  0 none, 1 USB-C, 2 inductive, 255 unknown (not reported yet).
 * @param present  1 when a battery is connected.
 * @param design_capacity_mah [mAh] Pack design capacity.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_power_supply_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint8_t status, uint8_t charger_type, uint8_t present, uint16_t design_capacity_mah)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_LEN];
    _mav_put_uint16_t(buf, 0, design_capacity_mah);
    _mav_put_uint8_t(buf, 2, status);
    _mav_put_uint8_t(buf, 3, charger_type);
    _mav_put_uint8_t(buf, 4, present);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_LEN);
#else
    mavlink_rosbot_power_supply_t packet;
    packet.design_capacity_mah = design_capacity_mah;
    packet.status = status;
    packet.charger_type = charger_type;
    packet.present = present;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_CRC);
}

/**
 * @brief Pack a rosbot_power_supply message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param status  sensor_msgs/BatteryState POWER_SUPPLY_STATUS_* value reported by the power board.
 * @param charger_type  0 none, 1 USB-C, 2 inductive, 255 unknown (not reported yet).
 * @param present  1 when a battery is connected.
 * @param design_capacity_mah [mAh] Pack design capacity.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_power_supply_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint8_t status, uint8_t charger_type, uint8_t present, uint16_t design_capacity_mah)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_LEN];
    _mav_put_uint16_t(buf, 0, design_capacity_mah);
    _mav_put_uint8_t(buf, 2, status);
    _mav_put_uint8_t(buf, 3, charger_type);
    _mav_put_uint8_t(buf, 4, present);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_LEN);
#else
    mavlink_rosbot_power_supply_t packet;
    packet.design_capacity_mah = design_capacity_mah;
    packet.status = status;
    packet.charger_type = charger_type;
    packet.present = present;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_LEN);
#endif
}

/**
 * @brief Pack a rosbot_power_supply message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param status  sensor_msgs/BatteryState POWER_SUPPLY_STATUS_* value reported by the power board.
 * @param charger_type  0 none, 1 USB-C, 2 inductive, 255 unknown (not reported yet).
 * @param present  1 when a battery is connected.
 * @param design_capacity_mah [mAh] Pack design capacity.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_rosbot_power_supply_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint8_t status,uint8_t charger_type,uint8_t present,uint16_t design_capacity_mah)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_LEN];
    _mav_put_uint16_t(buf, 0, design_capacity_mah);
    _mav_put_uint8_t(buf, 2, status);
    _mav_put_uint8_t(buf, 3, charger_type);
    _mav_put_uint8_t(buf, 4, present);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_LEN);
#else
    mavlink_rosbot_power_supply_t packet;
    packet.design_capacity_mah = design_capacity_mah;
    packet.status = status;
    packet.charger_type = charger_type;
    packet.present = present;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_CRC);
}

/**
 * @brief Encode a rosbot_power_supply struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_power_supply C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_power_supply_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_rosbot_power_supply_t* rosbot_power_supply)
{
    return mavlink_msg_rosbot_power_supply_pack(system_id, component_id, msg, rosbot_power_supply->status, rosbot_power_supply->charger_type, rosbot_power_supply->present, rosbot_power_supply->design_capacity_mah);
}

/**
 * @brief Encode a rosbot_power_supply struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_power_supply C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_power_supply_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_rosbot_power_supply_t* rosbot_power_supply)
{
    return mavlink_msg_rosbot_power_supply_pack_chan(system_id, component_id, chan, msg, rosbot_power_supply->status, rosbot_power_supply->charger_type, rosbot_power_supply->present, rosbot_power_supply->design_capacity_mah);
}

/**
 * @brief Encode a rosbot_power_supply struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param rosbot_power_supply C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_rosbot_power_supply_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_rosbot_power_supply_t* rosbot_power_supply)
{
    return mavlink_msg_rosbot_power_supply_pack_status(system_id, component_id, _status, msg,  rosbot_power_supply->status, rosbot_power_supply->charger_type, rosbot_power_supply->present, rosbot_power_supply->design_capacity_mah);
}

/**
 * @brief Send a rosbot_power_supply message
 * @param chan MAVLink channel to send the message
 *
 * @param status  sensor_msgs/BatteryState POWER_SUPPLY_STATUS_* value reported by the power board.
 * @param charger_type  0 none, 1 USB-C, 2 inductive, 255 unknown (not reported yet).
 * @param present  1 when a battery is connected.
 * @param design_capacity_mah [mAh] Pack design capacity.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_rosbot_power_supply_send(mavlink_channel_t chan, uint8_t status, uint8_t charger_type, uint8_t present, uint16_t design_capacity_mah)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_LEN];
    _mav_put_uint16_t(buf, 0, design_capacity_mah);
    _mav_put_uint8_t(buf, 2, status);
    _mav_put_uint8_t(buf, 3, charger_type);
    _mav_put_uint8_t(buf, 4, present);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY, buf, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_CRC);
#else
    mavlink_rosbot_power_supply_t packet;
    packet.design_capacity_mah = design_capacity_mah;
    packet.status = status;
    packet.charger_type = charger_type;
    packet.present = present;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY, (const char *)&packet, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_CRC);
#endif
}

/**
 * @brief Send a rosbot_power_supply message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_rosbot_power_supply_send_struct(mavlink_channel_t chan, const mavlink_rosbot_power_supply_t* rosbot_power_supply)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_rosbot_power_supply_send(chan, rosbot_power_supply->status, rosbot_power_supply->charger_type, rosbot_power_supply->present, rosbot_power_supply->design_capacity_mah);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY, (const char *)rosbot_power_supply, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_CRC);
#endif
}

#if MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_rosbot_power_supply_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint8_t status, uint8_t charger_type, uint8_t present, uint16_t design_capacity_mah)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint16_t(buf, 0, design_capacity_mah);
    _mav_put_uint8_t(buf, 2, status);
    _mav_put_uint8_t(buf, 3, charger_type);
    _mav_put_uint8_t(buf, 4, present);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY, buf, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_CRC);
#else
    mavlink_rosbot_power_supply_t *packet = (mavlink_rosbot_power_supply_t *)msgbuf;
    packet->design_capacity_mah = design_capacity_mah;
    packet->status = status;
    packet->charger_type = charger_type;
    packet->present = present;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY, (const char *)packet, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_MIN_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_LEN, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_CRC);
#endif
}
#endif

#endif

// MESSAGE ROSBOT_POWER_SUPPLY UNPACKING


/**
 * @brief Get field status from rosbot_power_supply message
 *
 * @return  sensor_msgs/BatteryState POWER_SUPPLY_STATUS_* value reported by the power board.
 */
static inline uint8_t mavlink_msg_rosbot_power_supply_get_status(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  2);
}

/**
 * @brief Get field charger_type from rosbot_power_supply message
 *
 * @return  0 none, 1 USB-C, 2 inductive, 255 unknown (not reported yet).
 */
static inline uint8_t mavlink_msg_rosbot_power_supply_get_charger_type(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  3);
}

/**
 * @brief Get field present from rosbot_power_supply message
 *
 * @return  1 when a battery is connected.
 */
static inline uint8_t mavlink_msg_rosbot_power_supply_get_present(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  4);
}

/**
 * @brief Get field design_capacity_mah from rosbot_power_supply message
 *
 * @return [mAh] Pack design capacity.
 */
static inline uint16_t mavlink_msg_rosbot_power_supply_get_design_capacity_mah(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  0);
}

/**
 * @brief Decode a rosbot_power_supply message into a struct
 *
 * @param msg The message to decode
 * @param rosbot_power_supply C-struct to decode the message contents into
 */
static inline void mavlink_msg_rosbot_power_supply_decode(const mavlink_message_t* msg, mavlink_rosbot_power_supply_t* rosbot_power_supply)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    rosbot_power_supply->design_capacity_mah = mavlink_msg_rosbot_power_supply_get_design_capacity_mah(msg);
    rosbot_power_supply->status = mavlink_msg_rosbot_power_supply_get_status(msg);
    rosbot_power_supply->charger_type = mavlink_msg_rosbot_power_supply_get_charger_type(msg);
    rosbot_power_supply->present = mavlink_msg_rosbot_power_supply_get_present(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_LEN? msg->len : MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_LEN;
        memset(rosbot_power_supply, 0, MAVLINK_MSG_ID_ROSBOT_POWER_SUPPLY_LEN);
    memcpy(rosbot_power_supply, _MAV_PAYLOAD(msg), len);
#endif
}
