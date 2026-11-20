#ifndef ALARM_H
#define ALARM_H

#include <stdint.h>
#include "events.h"

void alarm_init();

AlarmState alarm_state();
uint8_t alarm_sensitivity();
uint8_t alarm_sound_mode();
bool alarm_muted();

//
// Milliseconds left in the ARMING countdown (0 when not arming).
//
uint32_t alarm_countdown_ms();

float alarm_tilt_deg();
float alarm_dyn();

#endif
