#ifndef SLEEP_H
#define SLEEP_H

#include <stdint.h>

enum SleepWakeCause : uint8_t
{
    SLEEP_WAKE_UNKNOWN = 0,
    SLEEP_WAKE_MOTION = 1,
    SLEEP_WAKE_BUTTON_A = 2,
    SLEEP_WAKE_BUTTON_B = 3
};

//
// Configure the LSM303AGR INT1 motion interrupt, enable the wake sources and
// block in deep sleep. Returns after wake-up, leaving the cause for sleep_exit().
//
void sleep_enter(uint8_t sensitivity);

//
// Restore the accelerometer and button configuration, then report the wake cause.
//
SleepWakeCause sleep_exit();

#endif
