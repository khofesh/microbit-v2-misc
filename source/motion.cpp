#include "motion.h"

#include <math.h>

#ifndef HOST_TEST
#include "MicroBit.h"

extern MicroBit uBit;
#endif

static const Thresholds motionThresholds[3] = {
    {250, 25}, // level 1, low
    {120, 15}, // level 2, medium (default)
    {60, 8}    // level 3, high
};

static const uint8_t MOTION_SAMPLE_MS = 20;
static const uint8_t MOTION_SPIKES = 3;
static const uint16_t MOTION_STILL_MS = 1000;
static const float MOTION_LP_ALPHA = 0.1f;

static float vectorLength(Vec3 v)
{
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

const Thresholds &motion_thresholds(uint8_t level)
{
    if (level < 1)
        level = 1;
    if (level > 3)
        level = 3;

    return motionThresholds[level - 1];
}

void motion_reset(MotionState &s, Vec3 base)
{
    s.gLp = base;
    s.base = base;
    s.lastDyn = 0.0f;
    s.stillMs = 0;
    s.spikeCount = 0;
    s.initialized = true;
    s.moving = false;
    s.still = false;
    s.movedLatched = false;
    s.tiltAlarmed = false;
}

void motion_capture_base(MotionState &s)
{
    s.base = s.gLp;
    s.stillMs = 0;
    s.spikeCount = 0;
    s.moving = false;
    s.still = false;
    s.movedLatched = false;
    s.tiltAlarmed = false;
}

float motion_tilt_deg(const MotionState &s)
{
    float a = vectorLength(s.gLp);
    float b = vectorLength(s.base);

    if (a < 1.0f || b < 1.0f)
        return 0.0f;

    float c = (s.gLp.x * s.base.x + s.gLp.y * s.base.y + s.gLp.z * s.base.z) / (a * b);

    if (c > 1.0f)
        c = 1.0f;
    if (c < -1.0f)
        c = -1.0f;

    return acosf(c) * 57.29578f;
}

MotionEventValue motion_update(MotionState &s, Vec3 a, const Thresholds &t)
{
    if (!s.initialized)
    {
        motion_reset(s, a);
        return MOTION_NONE;
    }

    s.gLp.x += (a.x - s.gLp.x) * MOTION_LP_ALPHA;
    s.gLp.y += (a.y - s.gLp.y) * MOTION_LP_ALPHA;
    s.gLp.z += (a.z - s.gLp.z) * MOTION_LP_ALPHA;

    Vec3 d = {a.x - s.gLp.x, a.y - s.gLp.y, a.z - s.gLp.z};
    float dyn = vectorLength(d);

    s.lastDyn = dyn;

    if (dyn > (float)t.moveMg)
    {
        if (s.spikeCount < 255)
            s.spikeCount++;
    }
    else
    {
        s.spikeCount = 0;
    }

    if (!s.moving && s.spikeCount >= MOTION_SPIKES)
    {
        s.moving = true;
        s.still = false;
        s.stillMs = 0;
        s.movedLatched = false;
        s.tiltAlarmed = false;
        return MOTION_MOVING;
    }

    if (dyn < (float)t.moveMg / 2.0f)
    {
        if (s.stillMs < 60000 - MOTION_SAMPLE_MS)
            s.stillMs += MOTION_SAMPLE_MS;
    }
    else
    {
        s.stillMs = 0;
        s.still = false;
    }

    if (s.stillMs < MOTION_STILL_MS)
        return MOTION_NONE;

    float tilt = motion_tilt_deg(s);

    if (s.moving)
    {
        // A moving episode has finished. Always report MOVED; whether the
        // final tilt changed does not matter.
        s.moving = false;
        s.still = true;
        s.movedLatched = true;
        s.tiltAlarmed = tilt > (float)t.tiltDeg;
        return MOTION_MOVED;
    }

    if (!s.still)
    {
        s.still = true;

        if (tilt > (float)t.tiltDeg)
        {
            s.movedLatched = true;
            s.tiltAlarmed = true;
            return MOTION_MOVED;
        }

        return MOTION_STILL;
    }

    if (tilt > (float)t.tiltDeg && !s.tiltAlarmed)
    {
        s.movedLatched = true;
        s.tiltAlarmed = true;
        return MOTION_MOVED;
    }

    if (tilt < (float)t.tiltDeg * 0.5f)
        s.tiltAlarmed = false;

    return MOTION_NONE;
}

#ifndef HOST_TEST

static MotionState motionHardwareState;
static uint8_t motionSensitivity = 2;

void motion_set_sensitivity(uint8_t level)
{
    if (level < 1)
        level = 1;
    if (level > 3)
        level = 3;

    motionSensitivity = level;
}

Vec3 motion_read()
{
    Vec3 a = {(float)uBit.accelerometer.getX(), (float)uBit.accelerometer.getY(),
              (float)uBit.accelerometer.getZ()};

    return a;
}

const MotionState &motion_state()
{
    return motionHardwareState;
}

float motion_dyn()
{
    return motionHardwareState.lastDyn;
}

float motion_tilt()
{
    return motion_tilt_deg(motionHardwareState);
}

void motion_capture_base()
{
    motion_capture_base(motionHardwareState);
}

void motion_reset_after_wake()
{
    Vec3 a = motion_read();
    motion_reset(motionHardwareState, a);
}

void motion_mark_moving()
{
    motionHardwareState.moving = true;
    motionHardwareState.still = false;
    motionHardwareState.stillMs = 0;
    motionHardwareState.movedLatched = false;
    motionHardwareState.tiltAlarmed = false;
}

static void motionFiber()
{
    while (true)
    {
        Vec3 a = motion_read();
        MotionEventValue evt =
            motion_update(motionHardwareState, a, motion_thresholds(motionSensitivity));

        if (evt != MOTION_NONE)
            MicroBitEvent(MOTION_EVT_ID, evt);

        uBit.sleep(MOTION_SAMPLE_MS);
    }
}

void motion_start()
{
    uBit.accelerometer.setRange(2);
    uBit.accelerometer.setPeriod(MOTION_SAMPLE_MS);

    Vec3 a = motion_read();
    motion_reset(motionHardwareState, a);

    create_fiber(motionFiber);
}

#else

void motion_start() {}

void motion_set_sensitivity(uint8_t level)
{
    (void)level;
}

void motion_capture_base() {}

void motion_reset_after_wake() {}

void motion_mark_moving() {}

Vec3 motion_read()
{
    Vec3 a = {0, 0, 0};
    return a;
}

const MotionState &motion_state()
{
    static MotionState s;
    return s;
}

float motion_dyn()
{
    return 0.0f;
}

float motion_tilt()
{
    return 0.0f;
}

#endif
