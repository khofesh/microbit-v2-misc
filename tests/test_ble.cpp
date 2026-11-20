#include "test.h"

#include "ble_alarm.h"

static void testStateEncode()
{
    uint8_t data[4];

    ble_state_encode(data, ALARM_ARMED, 3, SOUND_BEEP, true);

    CHECK_EQ(data[0], ALARM_ARMED);
    CHECK_EQ(data[1], 3);
    CHECK_EQ(data[2], SOUND_BEEP);
    CHECK_EQ(data[3], 1);

    ble_state_encode(data, ALARM_DISARMED, 1, SOUND_SIREN, false);
    CHECK_EQ(data[0], ALARM_DISARMED);
    CHECK_EQ(data[3], 0);
}

static void testAlarmEncode()
{
    uint8_t data[4];

    ble_alarm_encode(data, BLE_ALARM_TYPE_MOVED, 0x1234, 37);

    CHECK_EQ(data[0], BLE_ALARM_TYPE_MOVED);
    CHECK_EQ(data[1], 0x34);
    CHECK_EQ(data[2], 0x12);
    CHECK_EQ(data[3], 37);

    ble_alarm_encode(data, BLE_ALARM_TYPE_MOVING, 0xffff, 250);
    CHECK_EQ(data[3], 180);
}

static void testDecodeValid()
{
    BleCommand command;

    const uint8_t arm[] = {BLE_CMD_ARM, 0};
    CHECK(ble_cmd_decode(arm, 2, command));
    CHECK_EQ(command.cmd, BLE_CMD_ARM);

    const uint8_t disarm[] = {BLE_CMD_DISARM, 0};
    CHECK(ble_cmd_decode(disarm, 2, command));
    CHECK_EQ(command.cmd, BLE_CMD_DISARM);

    const uint8_t ack[] = {BLE_CMD_ACK, 0};
    CHECK(ble_cmd_decode(ack, 2, command));
    CHECK_EQ(command.cmd, BLE_CMD_ACK);

    const uint8_t muteOn[] = {BLE_CMD_MUTE, 1};
    CHECK(ble_cmd_decode(muteOn, 2, command));
    CHECK_EQ(command.arg, 1);

    const uint8_t muteOff[] = {BLE_CMD_MUTE, 0};
    CHECK(ble_cmd_decode(muteOff, 2, command));
    CHECK_EQ(command.arg, 0);

    const uint8_t sens[] = {BLE_CMD_SET_SENS, 3};
    CHECK(ble_cmd_decode(sens, 2, command));
    CHECK_EQ(command.arg, 3);

    const uint8_t sound[] = {BLE_CMD_SET_SOUND, 2};
    CHECK(ble_cmd_decode(sound, 2, command));
    CHECK_EQ(command.arg, 2);
}

static void testDecodeInvalid()
{
    BleCommand command;

    const uint8_t badCmd[] = {0, 0};
    CHECK(!ble_cmd_decode(badCmd, 2, command));

    const uint8_t badCmd2[] = {7, 0};
    CHECK(!ble_cmd_decode(badCmd2, 2, command));

    const uint8_t badCmd3[] = {255, 0};
    CHECK(!ble_cmd_decode(badCmd3, 2, command));

    const uint8_t badMute[] = {BLE_CMD_MUTE, 2};
    CHECK(!ble_cmd_decode(badMute, 2, command));

    const uint8_t badSensLow[] = {BLE_CMD_SET_SENS, 0};
    CHECK(!ble_cmd_decode(badSensLow, 2, command));

    const uint8_t badSensHigh[] = {BLE_CMD_SET_SENS, 4};
    CHECK(!ble_cmd_decode(badSensHigh, 2, command));

    const uint8_t badSound[] = {BLE_CMD_SET_SOUND, 3};
    CHECK(!ble_cmd_decode(badSound, 2, command));

    const uint8_t good[] = {BLE_CMD_ARM, 0};
    CHECK(!ble_cmd_decode(good, 0, command));
    CHECK(!ble_cmd_decode(good, 1, command));
    CHECK(!ble_cmd_decode(good, 3, command));
    CHECK(!ble_cmd_decode(NULL, 2, command));
}

static void testPacking()
{
    uint16_t packed = ble_cmd_pack(BLE_CMD_SET_SENS, 3);
    CHECK_EQ(ble_cmd_get_cmd(packed), BLE_CMD_SET_SENS);
    CHECK_EQ(ble_cmd_get_arg(packed), 3);
}

void testBleCodec()
{
    testStateEncode();
    testAlarmEncode();
    testDecodeValid();
    testDecodeInvalid();
    testPacking();
}
