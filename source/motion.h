#ifndef MOTION_H
#define MOTION_H

#include <stdint.h>
#include "events.h"

struct Vec3
{
    float x;
    float y;
    float z;
};

struct Thresholds
{
    int16_t moveMg;
    int16_t tiltDeg;
};

struct MotionState
{
    Vec3 gLp;         // low-pass gravity estimate
    Vec3 base;        // gravity captured at arming
    float lastDyn;    // |a - gLp| for the most recent sample
    uint16_t stillMs; // time spent below moveMg / 2
    uint8_t spikeCount;
    bool initialized;
    bool moving;
    bool still;
    bool movedLatched;
    bool tiltAlarmed;
};

//
// Pure motion detection core (host testable, no hardware access).
//
void motion_reset(MotionState &s, Vec3 base);
void motion_capture_base(MotionState &s);
MotionEventValue motion_update(MotionState &s, Vec3 a, const Thresholds &t);
float motion_tilt_deg(const MotionState &s);
const Thresholds &motion_thresholds(uint8_t level);

//
// Hardware wrapper.
//
void motion_start();
void motion_set_sensitivity(uint8_t level);
void motion_capture_base();
void motion_reset_after_wake();
void motion_mark_moving();
Vec3 motion_read();
const MotionState &motion_state();
float motion_dyn();
float motion_tilt();

#endif
