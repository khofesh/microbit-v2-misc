#ifndef MOTION_ALARM_EVENTS_H
#define MOTION_ALARM_EVENTS_H

#include <stdint.h>

//
// Custom event IDs. Must be >= 9000 to avoid clashing with CODAL IDs.
//
#define MOTION_EVT_ID 9000
#define ALARM_EVT_ID 9001
#define BLE_CMD_EVT_ID 9002

//
// ALARM_EVT_ID values. 0..5 are AlarmState values; ALARM_EVT_SETTINGS means
// only a setting changed and the state itself did not.
//
#define ALARM_EVT_SETTINGS 0x10

enum AlarmState : uint8_t
{
    ALARM_DISARMED = 0,
    ALARM_ARMING = 1,
    ALARM_ARMED = 2,
    ALARM_MOVING = 3,
    ALARM_MOVED = 4,
    ALARM_SLEEP = 5
};

enum SoundMode : uint8_t
{
    SOUND_SIREN = 0,
    SOUND_BEEP = 1,
    SOUND_SILENT = 2
};

enum MotionEventValue : uint8_t
{
    MOTION_NONE = 0,
    MOTION_MOVING = 1,
    MOTION_STILL = 2,
    MOTION_MOVED = 3
};

enum BleCommandId : uint8_t
{
    BLE_CMD_ARM = 1,
    BLE_CMD_DISARM = 2,
    BLE_CMD_ACK = 3,
    BLE_CMD_MUTE = 4,
    BLE_CMD_SET_SENS = 5,
    BLE_CMD_SET_SOUND = 6
};

inline uint16_t ble_cmd_pack(uint8_t cmd, uint8_t arg)
{
    return (uint16_t)(((uint16_t)arg << 8) | cmd);
}

inline uint8_t ble_cmd_get_cmd(uint16_t value)
{
    return (uint8_t)(value & 0xff);
}

inline uint8_t ble_cmd_get_arg(uint16_t value)
{
    return (uint8_t)(value >> 8);
}

#endif
