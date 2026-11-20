#include "MicroBit.h"

#include "alarm.h"
#include "ble_alarm.h"
#include "motion.h"
#include "ui.h"

MicroBit uBit;

int main()
{
    uBit.init();

    // Sound goes to the on-board speaker only; nothing may be driven on P0.
    uBit.audio.setPinEnabled(false);
    uBit.audio.setSpeakerEnabled(true);

    ui_init();
    alarm_init();
    motion_start();
    ble_alarm_init();

    release_fiber();
}
