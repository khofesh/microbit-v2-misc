#include "ble_alarm.h"

#include <string.h>

void ble_state_encode(uint8_t *out, AlarmState state, uint8_t sensitivity, uint8_t soundMode,
                      bool muted)
{
    out[0] = (uint8_t)state;
    out[1] = sensitivity;
    out[2] = soundMode;
    out[3] = muted ? 1 : 0;
}

void ble_alarm_encode(uint8_t *out, uint8_t type, uint16_t seq, uint8_t tilt)
{
    if (tilt > 180)
        tilt = 180;

    out[0] = type;
    out[1] = (uint8_t)(seq & 0xff);
    out[2] = (uint8_t)(seq >> 8);
    out[3] = tilt;
}

bool ble_cmd_decode(const uint8_t *data, uint16_t length, BleCommand &out)
{
    if (data == NULL || length != 2)
        return false;

    uint8_t cmd = data[0];
    uint8_t arg = data[1];

    switch (cmd)
    {
    case BLE_CMD_ARM:
    case BLE_CMD_DISARM:
    case BLE_CMD_ACK:
        break;

    case BLE_CMD_MUTE:
        if (arg > 1)
            return false;
        break;

    case BLE_CMD_SET_SENS:
        if (arg < 1 || arg > 3)
            return false;
        break;

    case BLE_CMD_SET_SOUND:
        if (arg > 2)
            return false;
        break;

    default:
        return false;
    }

    out.cmd = cmd;
    out.arg = arg;
    return true;
}

#ifndef HOST_TEST

#include "MicroBit.h"
#include "alarm.h"

extern MicroBit uBit;

#if CONFIG_ENABLED(DEVICE_BLE) && CONFIG_ENABLED(MICROBIT_BLE_ENABLED)

using namespace codal;

namespace
{

const uint8_t alarmBaseUUID[16] = {0xc9, 0x5a, 0x00, 0x00, 0x36, 0xd7, 0x4d, 0x56,
                                   0xbf, 0xbc, 0xf6, 0xb9, 0x3a, 0xab, 0x3e, 0x5e};

const uint16_t alarmServiceUUID = 0x0001;
const uint16_t alarmCharUUID[3] = {0x0002, 0x0003, 0x0004};

enum
{
    IDX_STATE = 0,
    IDX_ALARM = 1,
    IDX_COMMAND = 2,
    IDX_COUNT = 3
};

bool linkConnected = false;
bool linkEverConnected = false;
uint32_t linkDisconnectTime = 0;

class MotionAlarmService : public MicroBitBLEService
{
public:
    MotionAlarmService();

    int characteristicCount() override { return IDX_COUNT; }

    MicroBitBLEChar *characteristicPtr(int idx) override { return &chars[idx]; }

    void handleAlarmEvent(MicroBitEvent e);

protected:
    void onDataWritten(const microbit_ble_evt_write_t *params) override;
    void onConnect(const microbit_ble_evt_t *p_ble_evt) override;
    void onDisconnect(const microbit_ble_evt_t *p_ble_evt) override;

private:
    MicroBitBLEChar chars[IDX_COUNT];
    uint8_t stateValue[4];
    uint8_t alarmValue[4];
    uint8_t commandValue[2];
    AlarmState previousState;
    uint16_t seq;

    void updateState();
    void sendAlarm(uint8_t type);
};

MotionAlarmService *serviceInstance = NULL;

MotionAlarmService::MotionAlarmService()
{
    previousState = ALARM_DISARMED;
    seq = 0;

    memset(stateValue, 0, sizeof(stateValue));
    memset(alarmValue, 0, sizeof(alarmValue));
    memset(commandValue, 0, sizeof(commandValue));

    ble_state_encode(stateValue, alarm_state(), alarm_sensitivity(), alarm_sound_mode(),
                     alarm_muted());

    RegisterBaseUUID(alarmBaseUUID);
    CreateService(alarmServiceUUID);

    CreateCharacteristic(IDX_STATE, alarmCharUUID[IDX_STATE], stateValue, sizeof(stateValue),
                         sizeof(stateValue), microbit_propREAD | microbit_propNOTIFY);

    CreateCharacteristic(IDX_ALARM, alarmCharUUID[IDX_ALARM], alarmValue, sizeof(alarmValue),
                         sizeof(alarmValue), microbit_propREAD | microbit_propINDICATE);

    CreateCharacteristic(IDX_COMMAND, alarmCharUUID[IDX_COMMAND], commandValue,
                         sizeof(commandValue), sizeof(commandValue), microbit_propWRITE);
}

void MotionAlarmService::updateState()
{
    ble_state_encode(stateValue, alarm_state(), alarm_sensitivity(), alarm_sound_mode(),
                     alarm_muted());

    setChrValue(IDX_STATE, stateValue, sizeof(stateValue));

    if (getConnected())
        notifyChrValue(IDX_STATE, stateValue, sizeof(stateValue));
}

void MotionAlarmService::sendAlarm(uint8_t type)
{
    uint8_t tilt = (uint8_t)alarm_tilt_deg();

    ble_alarm_encode(alarmValue, type, seq++, tilt);
    setChrValue(IDX_ALARM, alarmValue, sizeof(alarmValue));

    if (getConnected())
        indicateChrValue(IDX_ALARM, alarmValue, sizeof(alarmValue));
}

void MotionAlarmService::handleAlarmEvent(MicroBitEvent e)
{
    if (e.value != ALARM_EVT_SETTINGS)
    {
        AlarmState state = (AlarmState)e.value;

        if (state == ALARM_MOVING)
            sendAlarm(BLE_ALARM_TYPE_MOVING);
        else if (state == ALARM_MOVED)
            sendAlarm(BLE_ALARM_TYPE_MOVED);
        else if (previousState == ALARM_MOVING || previousState == ALARM_MOVED)
            sendAlarm(BLE_ALARM_TYPE_CLEARED);

        previousState = state;
    }

    updateState();
}

void MotionAlarmService::onDataWritten(const microbit_ble_evt_write_t *params)
{
    if (params->handle == valueHandle(IDX_COMMAND))
    {
        BleCommand command;

        if (ble_cmd_decode(params->data, params->len, command))
            MicroBitEvent evt(BLE_CMD_EVT_ID, ble_cmd_pack(command.cmd, command.arg));
    }
}

void MotionAlarmService::onConnect(const microbit_ble_evt_t *)
{
    linkConnected = true;
    linkEverConnected = true;
}

void MotionAlarmService::onDisconnect(const microbit_ble_evt_t *)
{
    linkConnected = false;
    linkDisconnectTime = (uint32_t)system_timer_current_time();
}

void alarmEventListener(MicroBitEvent e)
{
    if (serviceInstance)
        serviceInstance->handleAlarmEvent(e);
}

} // namespace

void ble_alarm_init()
{
    if (serviceInstance == NULL)
        serviceInstance = new MotionAlarmService();

    uBit.messageBus.listen(ALARM_EVT_ID, MICROBIT_EVT_ANY, alarmEventListener);
}

bool ble_alarm_connected()
{
    return linkConnected;
}

uint32_t ble_alarm_ms_since_disconnect()
{
    if (linkConnected)
        return 0;

    if (!linkEverConnected)
        return 0xffffffffu;

    return (uint32_t)system_timer_current_time() - linkDisconnectTime;
}

void ble_alarm_resume_advertising()
{
    if (!linkConnected)
        uBit.bleManager.advertise();
}

#else

void ble_alarm_init() {}

bool ble_alarm_connected()
{
    return false;
}

uint32_t ble_alarm_ms_since_disconnect()
{
    return 0xffffffffu;
}

void ble_alarm_resume_advertising() {}

#endif

#else

void ble_alarm_init() {}

bool ble_alarm_connected()
{
    return false;
}

uint32_t ble_alarm_ms_since_disconnect()
{
    return 0xffffffffu;
}

void ble_alarm_resume_advertising() {}

#endif
