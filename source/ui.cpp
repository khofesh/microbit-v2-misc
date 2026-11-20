#include "ui.h"

#include "MicroBit.h"
#include "alarm.h"
#include "events.h"
#include "motion.h"

extern MicroBit uBit;

static const MicroBitImage imageCheck("000,000,000,000,255\n"
                                      "000,000,000,255,000\n"
                                      "255,000,255,000,000\n"
                                      "000,255,000,000,000\n"
                                      "000,000,000,000,000\n");

static const MicroBitImage imageArrowUp("000,000,255,000,000\n"
                                        "000,255,255,255,000\n"
                                        "000,000,255,000,000\n"
                                        "000,000,255,000,000\n"
                                        "000,000,255,000,000\n");

static const MicroBitImage imageArrowRight("000,000,000,000,000\n"
                                           "000,255,000,000,000\n"
                                           "255,255,255,255,255\n"
                                           "000,255,000,000,000\n"
                                           "000,000,000,000,000\n");

static const MicroBitImage imageArrowDown("000,000,255,000,000\n"
                                          "000,000,255,000,000\n"
                                          "000,000,255,000,000\n"
                                          "000,255,255,255,000\n"
                                          "000,000,255,000,000\n");

static const MicroBitImage imageArrowLeft("000,000,000,000,000\n"
                                          "000,000,000,255,000\n"
                                          "255,255,255,255,255\n"
                                          "000,000,000,255,000\n"
                                          "000,000,000,000,000\n");

static const MicroBitImage imageBang("000,000,255,000,000\n"
                                     "000,000,255,000,000\n"
                                     "000,000,255,000,000\n"
                                     "000,000,000,000,000\n"
                                     "000,000,255,000,000\n");

static const MicroBitImage *const arrows[4] = {&imageArrowUp, &imageArrowRight, &imageArrowDown,
                                               &imageArrowLeft};

enum Overlay
{
    OVERLAY_NONE = 0,
    OVERLAY_CHECK,
    OVERLAY_SENSITIVITY,
    OVERLAY_SOUND,
    OVERLAY_BLE,
    OVERLAY_TILT,
    OVERLAY_DYN
};

static volatile Overlay overlay = OVERLAY_NONE;
static volatile uint32_t overlayUntil = 0;
static volatile int overlayArg = 0;
static volatile bool uiLocked = false;
static volatile bool audioIdle = false;
static volatile bool ledIdle = false;

static bool soundActive(AlarmState state)
{
    return !uiLocked && alarm_state() == state && !alarm_muted() &&
           alarm_sound_mode() != SOUND_SILENT;
}

static void startTone(int frequency)
{
    uBit.audio.virtualOutputPin.setAnalogValue(128);
    uBit.audio.virtualOutputPin.setAnalogPeriodUs(1000000 / frequency);
}

static void stopTone()
{
    uBit.audio.virtualOutputPin.setAnalogValue(0);
}

static void sleepWhile(AlarmState state, int milliseconds)
{
    for (int elapsed = 0; elapsed < milliseconds; elapsed += 50)
    {
        if (!soundActive(state))
            return;

        int remaining = milliseconds - elapsed;
        uBit.sleep(remaining < 50 ? remaining : 50);
    }
}

static void playTick()
{
    startTone(1200);
    uBit.sleep(30);
    stopTone();
}

static void playSiren()
{
    startTone(880);
    sleepWhile(ALARM_MOVING, 250);
    stopTone();

    if (!soundActive(ALARM_MOVING))
        return;

    startTone(660);
    sleepWhile(ALARM_MOVING, 250);
    stopTone();
}

static void playMovingBeeps()
{
    startTone(1000);
    uBit.sleep(100);
    stopTone();
    sleepWhile(ALARM_MOVING, 400);
}

static void playMovedBeeps()
{
    for (int i = 0; i < 3; i++)
    {
        if (!soundActive(ALARM_MOVED))
            return;

        startTone(1500);
        uBit.sleep(100);
        stopTone();

        if (!soundActive(ALARM_MOVED))
            return;

        uBit.sleep(100);
    }

    sleepWhile(ALARM_MOVED, 1500);
}

static void soundFiber()
{
    uint32_t lastTick = 0xffffffffu;

    while (true)
    {
        if (uiLocked)
        {
            uBit.sleep(100);
            continue;
        }

        AlarmState state = alarm_state();

        if (state == ALARM_SLEEP)
        {
            stopTone();
            audioIdle = true;
            uBit.sleep(50);
            continue;
        }

        audioIdle = false;

        switch (state)
        {
        case ALARM_ARMING:
        {
            uint32_t ticks = (alarm_countdown_ms() + 999) / 1000;

            if (ticks != lastTick && ticks > 0)
            {
                lastTick = ticks;
                playTick();
            }
            else
            {
                uBit.sleep(20);
            }
            break;
        }

        case ALARM_MOVING:
            lastTick = 0xffffffffu;

            if (alarm_muted() || alarm_sound_mode() == SOUND_SILENT)
            {
                stopTone();
                uBit.sleep(100);
            }
            else if (alarm_sound_mode() == SOUND_BEEP)
            {
                playMovingBeeps();
            }
            else
            {
                playSiren();
            }
            break;

        case ALARM_MOVED:
            lastTick = 0xffffffffu;

            if (alarm_muted() || alarm_sound_mode() == SOUND_SILENT)
            {
                stopTone();
                uBit.sleep(100);
            }
            else
            {
                playMovedBeeps();
            }
            break;

        default:
            lastTick = 0xffffffffu;
            stopTone();
            uBit.sleep(50);
            break;
        }
    }
}

static void drawSensitivity(uint8_t level)
{
    uBit.display.image.clear();

    for (uint8_t i = 0; i < level && i < 3; i++)
    {
        for (int y = 0; y < 5; y++)
            uBit.display.image.setPixelValue(i * 2, y, 255);
    }
}

static void drawDyn()
{
    const Thresholds &thresholds = motion_thresholds(alarm_sensitivity());
    float fullScale = (float)thresholds.moveMg * 2.0f;

    int columns = 0;

    if (fullScale > 0.0f)
        columns = (int)(alarm_dyn() * 5.0f / fullScale);

    if (columns < 0)
        columns = 0;
    if (columns > 5)
        columns = 5;

    uBit.display.image.clear();

    for (int x = 0; x < columns; x++)
        for (int y = 0; y < 5; y++)
            uBit.display.image.setPixelValue(x, y, 255);
}

static void drawOverlay(Overlay which, int arg)
{
    switch (which)
    {
    case OVERLAY_CHECK:
        uBit.display.image.clear();
        uBit.display.image.paste(imageCheck);
        break;

    case OVERLAY_SENSITIVITY:
        drawSensitivity((uint8_t)arg);
        break;

    case OVERLAY_SOUND:
        uBit.display.image.clear();
        uBit.display.image.print(arg == SOUND_SIREN ? 'S' : arg == SOUND_BEEP ? 'B' : '-');
        break;

    case OVERLAY_BLE:
        uBit.display.image.clear();
        uBit.display.image.print(arg ? 'C' : '-');
        break;

    case OVERLAY_TILT:
        uBit.display.scroll(ManagedString("T") + ManagedString(arg), 80);
        overlay = OVERLAY_NONE;
        break;

    default:
        break;
    }
}

static void showOverlay(Overlay which, int arg, uint32_t milliseconds)
{
    overlayArg = arg;
    overlayUntil = (uint32_t)system_timer_current_time() + milliseconds;
    overlay = which;
}

static void ledFiber()
{
    uint32_t lastArmingDigit = 0xffffffffu;
    int lastOverlayKey = -1;

    while (true)
    {
        if (uiLocked)
        {
            uBit.sleep(100);
            continue;
        }

        uint32_t now = (uint32_t)system_timer_current_time();
        AlarmState state = alarm_state();

        if (overlay == OVERLAY_DYN)
        {
            ledIdle = false;
            drawDyn();
            uBit.sleep(50);
            continue;
        }

        if (overlay != OVERLAY_NONE && now < overlayUntil)
        {
            ledIdle = false;

            int key = (int)overlay * 1000 + overlayArg;
            if (key != lastOverlayKey)
            {
                lastOverlayKey = key;
                drawOverlay(overlay, overlayArg);
            }

            uBit.sleep(50);
            continue;
        }

        if (overlay != OVERLAY_NONE)
        {
            overlay = OVERLAY_NONE;
            lastOverlayKey = -1;
        }

        if (state == ALARM_SLEEP)
        {
            uBit.display.image.clear();
            ledIdle = true;
            uBit.sleep(100);
            continue;
        }

        ledIdle = false;

        switch (state)
        {
        case ALARM_DISARMED:
            lastArmingDigit = 0xffffffffu;
            uBit.display.image.clear();
            uBit.display.image.setPixelValue(2, 2, 20);
            break;

        case ALARM_ARMING:
        {
            uint32_t remaining = alarm_countdown_ms();
            uint32_t digit = (remaining + 999) / 1000;

            if (digit < 1)
                digit = 1;

            if (digit != lastArmingDigit)
            {
                lastArmingDigit = digit;

                uBit.display.image.clear();
                uBit.display.image.print((char)('0' + digit));
            }
            break;
        }

        case ALARM_ARMED:
            lastArmingDigit = 0xffffffffu;
            uBit.display.image.clear();

            if (now % 2000 < 100)
                uBit.display.image.setPixelValue(2, 2, 255);
            break;

        case ALARM_MOVING:
            lastArmingDigit = 0xffffffffu;
            uBit.display.image.clear();
            uBit.display.image.paste(*arrows[(now / 150) % 4]);
            break;

        case ALARM_MOVED:
            lastArmingDigit = 0xffffffffu;
            uBit.display.image.clear();

            if ((now / 500) % 2 == 0)
                uBit.display.image.paste(imageBang);
            break;

        default:
            break;
        }

        uBit.sleep(20);
    }
}

static void onAlarmEvent(MicroBitEvent e)
{
    if (e.value == ALARM_EVT_SETTINGS)
        return;

    overlay = OVERLAY_NONE;
    uBit.display.stopAnimation();
}

void ui_init()
{
    uBit.messageBus.listen(ALARM_EVT_ID, MICROBIT_EVT_ANY, onAlarmEvent);

    create_fiber(ledFiber);
    create_fiber(soundFiber);
}

bool ui_prepare_sleep()
{
    for (int i = 0; i < 50; i++)
    {
        if (audioIdle && ledIdle)
            break;

        uBit.sleep(10);
    }

    if (!audioIdle || !ledIdle)
        return false;

    uiLocked = true;
    return true;
}

void ui_end_sleep()
{
    audioIdle = false;
    ledIdle = false;
    uiLocked = false;
}

bool ui_sleep_allowed()
{
    return !uiLocked && overlay == OVERLAY_NONE;
}

void ui_show_check()
{
    showOverlay(OVERLAY_CHECK, 0, 1000);
}

void ui_show_sensitivity(uint8_t level)
{
    showOverlay(OVERLAY_SENSITIVITY, level, 1000);
}

void ui_show_sound_mode(uint8_t mode)
{
    showOverlay(OVERLAY_SOUND, mode, 1000);
}

void ui_show_ble(bool connected)
{
    showOverlay(OVERLAY_BLE, connected ? 1 : 0, 1500);
}

void ui_show_tilt(int degrees)
{
    showOverlay(OVERLAY_TILT, degrees, 2000);
}

void ui_dyn_begin()
{
    overlay = OVERLAY_DYN;
}

void ui_dyn_end()
{
    if (overlay == OVERLAY_DYN)
        overlay = OVERLAY_NONE;
}
