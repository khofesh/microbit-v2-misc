#ifndef BLE_ALARM_H
#define BLE_ALARM_H

#include <stdint.h>
#include "events.h"

//
// Alarm event types carried by the Alarm event characteristic.
//
#define BLE_ALARM_TYPE_MOVING 1
#define BLE_ALARM_TYPE_MOVED 2
#define BLE_ALARM_TYPE_CLEARED 3

struct BleCommand
{
    uint8_t cmd;
    uint8_t arg;
};

//
// Pure payload codec (host testable, no hardware access).
//
void ble_state_encode(uint8_t *out, AlarmState state, uint8_t sensitivity, uint8_t soundMode,
                      bool muted);
void ble_alarm_encode(uint8_t *out, uint8_t type, uint16_t seq, uint8_t tilt);
bool ble_cmd_decode(const uint8_t *data, uint16_t length, BleCommand &out);

//
// Hardware side.
//
void ble_alarm_init();
bool ble_alarm_connected();
uint32_t ble_alarm_ms_since_disconnect();
void ble_alarm_resume_advertising();

#endif
