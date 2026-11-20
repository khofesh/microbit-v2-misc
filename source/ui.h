#ifndef UI_H
#define UI_H

#include <stdint.h>

void ui_init();

//
// Called around deep sleep so neither UI fiber touches the display or the
// speaker while the power manager has them shut down.
//
bool ui_prepare_sleep();
void ui_end_sleep();
bool ui_sleep_allowed();

void ui_show_check();
void ui_show_sensitivity(uint8_t level);
void ui_show_sound_mode(uint8_t mode);
void ui_show_ble(bool connected);
void ui_show_tilt(int degrees);

void ui_dyn_begin();
void ui_dyn_end();

#endif
