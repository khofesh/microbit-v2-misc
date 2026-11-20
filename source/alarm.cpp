#include "alarm.h"

#include "MicroBit.h"
#include "ble_alarm.h"
#include "motion.h"
#include "sleep.h"
#include "ui.h"

extern MicroBit uBit;

#define ALARM_TIMER_ID 9100
#define ALARM_TIMER_EVT 1

static const uint32_t ALARM_TICK_MS = 100;
static const uint32_t ARMING_MS = 5000;
static const uint32_t AUTO_CLEAR_MS = 60000;
static const uint32_t SLEEP_STILL_MS = 60000;
static const uint32_t SLEEP_BLE_GRACE_MS = 60000;

static AlarmState state = ALARM_DISARMED;
static uint8_t sensitivity = 2;
static uint8_t soundMode = SOUND_SIREN;
static bool muted = false;
static bool still = false;

static uint32_t stateSince = 0;
static uint32_t alarmSince = 0;
static uint32_t stillSince = 0;

static uint32_t nowMs()
{
    return (uint32_t)system_timer_current_time();
}

static void raiseSettings()
{
    MicroBitEvent evt(ALARM_EVT_ID, ALARM_EVT_SETTINGS);
}

static void setState(AlarmState next)
{
    if (state == next)
        return;

    state = next;
    stateSince = nowMs();

    switch (next)
    {
    case ALARM_DISARMED:
    case ALARM_ARMING:
    case ALARM_ARMED:
        muted = false;
        still = false;
        break;

    case ALARM_MOVING:
        still = false;
        break;

    default:
        break;
    }

    MicroBitEvent evt(ALARM_EVT_ID, (uint16_t)next);
}

static void loadSettings()
{
    sensitivity = 2;
    soundMode = SOUND_SIREN;

    KeyValuePair *pair = uBit.storage.get("alarm_sens");
    if (pair != NULL)
    {
        if (pair->value[0] >= 1 && pair->value[0] <= 3)
            sensitivity = pair->value[0];

        delete pair;
    }

    pair = uBit.storage.get("alarm_snd");
    if (pair != NULL)
    {
        if (pair->value[0] <= SOUND_SILENT)
            soundMode = pair->value[0];

        delete pair;
    }

    motion_set_sensitivity(sensitivity);
}

static void saveSensitivity()
{
    uBit.storage.put("alarm_sens", &sensitivity, 1);
}

static void saveSoundMode()
{
    uBit.storage.put("alarm_snd", &soundMode, 1);
}

static void handleMotion(MicroBitEvent e)
{
    uint32_t now = nowMs();

    switch ((MotionEventValue)e.value)
    {
    case MOTION_MOVING:
        still = false;

        if (state == ALARM_ARMED || state == ALARM_MOVED)
        {
            setState(ALARM_MOVING);
            alarmSince = now;
        }
        else if (state == ALARM_MOVING)
        {
            alarmSince = now;
        }
        break;

    case MOTION_MOVED:
        if (state == ALARM_ARMED || state == ALARM_MOVING)
        {
            setState(ALARM_MOVED);
            alarmSince = now;
        }
        else if (state == ALARM_MOVED)
        {
            alarmSince = now;
        }
        break;

    case MOTION_STILL:
        if (state == ALARM_ARMED)
        {
            still = true;
            stillSince = now;
        }
        break;

    default:
        break;
    }
}

static void handleButtonA(bool longPress)
{
    still = false;

    if (longPress)
    {
        if (state == ALARM_DISARMED || state == ALARM_ARMED)
        {
            motion_capture_base();
            ui_show_check();
        }
        return;
    }

    switch (state)
    {
    case ALARM_DISARMED:
        setState(ALARM_ARMING);
        break;

    case ALARM_ARMING:
    case ALARM_ARMED:
    case ALARM_MOVING:
    case ALARM_MOVED:
        setState(ALARM_DISARMED);
        break;

    default:
        break;
    }
}

static void handleButtonB(bool longPress)
{
    if (longPress)
    {
        if (state == ALARM_DISARMED || state == ALARM_ARMED)
        {
            soundMode = (soundMode + 1) % 3;
            saveSoundMode();
            ui_show_sound_mode(soundMode);
            raiseSettings();
        }
        return;
    }

    switch (state)
    {
    case ALARM_DISARMED:
        sensitivity = (sensitivity % 3) + 1;
        motion_set_sensitivity(sensitivity);
        saveSensitivity();
        ui_show_sensitivity(sensitivity);
        raiseSettings();
        break;

    case ALARM_ARMED:
        ui_show_sensitivity(sensitivity);
        break;

    case ALARM_MOVING:
    case ALARM_MOVED:
        muted = !muted;
        raiseSettings();
        break;

    default:
        break;
    }
}

static void handleButtonAB()
{
    if (state == ALARM_DISARMED)
    {
        ui_show_tilt((int)(motion_tilt() + 0.5f));
    }
    else if (state == ALARM_MOVING || state == ALARM_MOVED)
    {
        motion_capture_base();
        setState(ALARM_ARMED);
    }
}

static void handleBleCommand(MicroBitEvent e)
{
    uint8_t command = ble_cmd_get_cmd(e.value);
    uint8_t arg = ble_cmd_get_arg(e.value);

    switch (command)
    {
    case BLE_CMD_ARM:
        if (state == ALARM_DISARMED)
            setState(ALARM_ARMING);
        break;

    case BLE_CMD_DISARM:
        if (state == ALARM_ARMING || state == ALARM_ARMED || state == ALARM_MOVING ||
            state == ALARM_MOVED)
            setState(ALARM_DISARMED);
        break;

    case BLE_CMD_ACK:
        if (state == ALARM_MOVING || state == ALARM_MOVED)
        {
            motion_capture_base();
            setState(ALARM_ARMED);
        }
        break;

    case BLE_CMD_MUTE:
        if (state == ALARM_MOVING || state == ALARM_MOVED)
        {
            muted = (arg != 0);
            raiseSettings();
        }
        break;

    case BLE_CMD_SET_SENS:
        sensitivity = arg;
        motion_set_sensitivity(sensitivity);
        saveSensitivity();
        ui_show_sensitivity(sensitivity);
        raiseSettings();
        break;

    case BLE_CMD_SET_SOUND:
        soundMode = arg;
        saveSoundMode();
        ui_show_sound_mode(soundMode);
        raiseSettings();
        break;

    default:
        break;
    }
}

static void handleTimer()
{
    uint32_t now = nowMs();

    if (state == ALARM_ARMING && now - stateSince >= ARMING_MS)
    {
        motion_capture_base();
        setState(ALARM_ARMED);
        return;
    }

    if (state == ALARM_MOVED && now - alarmSince >= AUTO_CLEAR_MS)
    {
        motion_capture_base();
        setState(ALARM_ARMED);
        return;
    }

    if (state != ALARM_ARMED || !still)
        return;

    if (now - stillSince < SLEEP_STILL_MS)
        return;

    if (ble_alarm_connected())
        return;

    if (ble_alarm_ms_since_disconnect() < SLEEP_BLE_GRACE_MS)
        return;

    // Do not sleep on top of a motion/tilt event that is still queued.
    if (motion_state().moving || motion_tilt() > (float)motion_thresholds(sensitivity).tiltDeg)
        return;

    if (!ui_sleep_allowed())
        return;

    setState(ALARM_SLEEP);

    if (!ui_prepare_sleep() || ble_alarm_connected() ||
        ble_alarm_ms_since_disconnect() < SLEEP_BLE_GRACE_MS)
    {
        ui_end_sleep();
        setState(ALARM_ARMED);
        still = true;
        stillSince = now - SLEEP_STILL_MS + 1000;
        return;
    }

    sleep_enter(sensitivity);
    SleepWakeCause cause = sleep_exit();

    motion_reset_after_wake();
    ui_end_sleep();
    ble_alarm_resume_advertising();

    if (cause == SLEEP_WAKE_BUTTON_A || cause == SLEEP_WAKE_BUTTON_B)
    {
        // Button::setSleep(false) has already queued the matching press, which
        // will be handled as a normal ARMED press once this handler returns.
        setState(ALARM_ARMED);
    }
    else if (cause == SLEEP_WAKE_MOTION)
    {
        motion_mark_moving();
        setState(ALARM_MOVING);
        alarmSince = nowMs();
    }
    else
    {
        setState(ALARM_ARMED);
    }
}

static void onMotion(MicroBitEvent e)
{
    handleMotion(e);
}

static void onButtonAClick(MicroBitEvent)
{
    handleButtonA(false);
}

static void onButtonALongClick(MicroBitEvent)
{
    handleButtonA(true);
}

static void onButtonBClick(MicroBitEvent)
{
    handleButtonB(false);
}

static void onButtonBLongClick(MicroBitEvent)
{
    handleButtonB(true);
}

static void onButtonABClick(MicroBitEvent)
{
    handleButtonAB();
}

static void onButtonABDown(MicroBitEvent)
{
    if (state == ALARM_ARMED)
        ui_dyn_begin();
}

static void onButtonABUp(MicroBitEvent)
{
    ui_dyn_end();
}

static void onLogoClick(MicroBitEvent)
{
    ui_show_ble(ble_alarm_connected());
}

static void onBleCommand(MicroBitEvent e)
{
    handleBleCommand(e);
}

static void onTimer(MicroBitEvent)
{
    handleTimer();
}

void alarm_init()
{
    loadSettings();

    state = ALARM_DISARMED;
    stateSince = nowMs();
    muted = false;
    still = false;

    uBit.messageBus.listen(MOTION_EVT_ID, MICROBIT_EVT_ANY, onMotion);

    uBit.messageBus.listen(MICROBIT_ID_BUTTON_A, MICROBIT_BUTTON_EVT_CLICK, onButtonAClick);
    uBit.messageBus.listen(MICROBIT_ID_BUTTON_A, MICROBIT_BUTTON_EVT_LONG_CLICK,
                           onButtonALongClick);
    uBit.messageBus.listen(MICROBIT_ID_BUTTON_B, MICROBIT_BUTTON_EVT_CLICK, onButtonBClick);
    uBit.messageBus.listen(MICROBIT_ID_BUTTON_B, MICROBIT_BUTTON_EVT_LONG_CLICK,
                           onButtonBLongClick);

    uBit.messageBus.listen(MICROBIT_ID_BUTTON_AB, MICROBIT_BUTTON_EVT_CLICK, onButtonABClick);
    uBit.messageBus.listen(MICROBIT_ID_BUTTON_AB, MICROBIT_BUTTON_EVT_DOWN, onButtonABDown);
    uBit.messageBus.listen(MICROBIT_ID_BUTTON_AB, MICROBIT_BUTTON_EVT_UP, onButtonABUp);

    uBit.messageBus.listen(MICROBIT_ID_LOGO, MICROBIT_BUTTON_EVT_CLICK, onLogoClick);

    uBit.messageBus.listen(BLE_CMD_EVT_ID, MICROBIT_EVT_ANY, onBleCommand);
    uBit.messageBus.listen(ALARM_TIMER_ID, ALARM_TIMER_EVT, onTimer);

    MicroBitEvent evt(ALARM_EVT_ID, (uint16_t)ALARM_DISARMED);

    system_timer_event_every(ALARM_TICK_MS, ALARM_TIMER_ID, ALARM_TIMER_EVT);
}

AlarmState alarm_state()
{
    return state;
}

uint8_t alarm_sensitivity()
{
    return sensitivity;
}

uint8_t alarm_sound_mode()
{
    return soundMode;
}

bool alarm_muted()
{
    return muted;
}

uint32_t alarm_countdown_ms()
{
    if (state != ALARM_ARMING)
        return 0;

    uint32_t elapsed = nowMs() - stateSince;

    if (elapsed >= ARMING_MS)
        return 0;

    return ARMING_MS - elapsed;
}

float alarm_tilt_deg()
{
    return motion_tilt();
}

float alarm_dyn()
{
    return motion_dyn();
}
