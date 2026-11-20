# SPEC — Motion Alarm

Target: BBC micro:bit V2 (nRF52833), CODAL C++. Build: `python build.py` → `MICROBIT.hex`.

## 1. Goal

Detect when the micro:bit is **moving** (in motion right now) or has been **moved** (it's still again, but its position or tilt has changed since it was armed). On either event, make a sound and show an LED sign. Buttons A/B control arming, sensitivity and sound.

## 2. Hardware

| Item | Use |
|---|---|
| LSM303AGR accelerometer (on-board) | Motion and tilt sensing |
| On-board speaker | Default sound output ("buzzer") |
| P0 + GND (optional) | External piezo buzzer, driven by the same audio signal |
| 5×5 LED matrix | Status and alarm signs |
| Buttons A, B, touch logo | User input |

Audio output uses `uBit.audio.virtualOutputPin`, as in `source/samples/AudioTest.cpp` `analogPitch()`. The speaker and P0 can be switched on or off separately with `uBit.audio.setSpeakerEnabled()` / `setPinEnabled()`.

## 3. Definitions

Sample rate: 50 Hz (`uBit.accelerometer.setPeriod(20)`), range ±2 g (`setRange(2)`). All values are in mg.

| Term | Definition |
|---|---|
| `a` | Raw sample `(x, y, z)` |
| `g_lp` | Low-pass gravity estimate: `g_lp += (a - g_lp) * 0.1` |
| `dyn` | Dynamic acceleration: `\|a - g_lp\|` |
| `base` | `g_lp` captured at the moment the alarm is armed |
| `tilt` | Angle between `g_lp` and `base`: `acos(dot / (\|g_lp\|·\|base\|))`, in degrees |
| **MOVING** | `dyn > T_move` for ≥ 3 consecutive samples (60 ms) |
| **STILL** | `dyn < T_move / 2` for 1000 ms continuously |
| **MOVED** | Device is STILL **and** `tilt > T_tilt`, **or** a MOVING episode happened while armed (latched) |

### Sensitivity levels

| Level | LED bars | `T_move` (mg) | `T_tilt` (°) |
|---|---|---|---|
| 1 Low | 1 | 250 | 25 |
| 2 Medium (default) | 2 | 120 | 15 |
| 3 High | 3 | 60 | 8 |

## 4. State machine

```
            A                 countdown done
DISARMED ───────► ARMING ─────────────────────► ARMED
   ▲                │ A (cancel)                  │ MOVING
   │                ▼                             ▼
   │            DISARMED                       ALARM_MOVING
   │                                              │ STILL for 1 s
   │   A+B (ack) / A (disarm)                     ▼
   └──────────────────────────────────────── ALARM_MOVED  (latched)
                                                  │ MOVING again
                                                  └──► ALARM_MOVING
```

| State | Entry action | LED | Sound |
|---|---|---|---|
| DISARMED | Stop sound | Centre pixel dim (brightness 20) | Silent |
| ARMING | 3 s countdown, then capture `base` | `3`, `2`, `1` | Short tick each second |
| ARMED | — | Centre pixel heartbeat (on 100 ms every 2 s) | Silent |
| ALARM_MOVING | Start siren | Animated arrows (rotating, 150 ms/frame) | Two-tone siren 880 Hz / 660 Hz, 250 ms each |
| ALARM_MOVED | Start chirp | Blinking `!` (500 ms on/off) | 3 × 100 ms beeps at 1500 Hz every 2 s |

Leaving the alarm states requires a user action (latched), so an event that happened while nobody was watching is still visible afterwards.

## 5. Buttons (suggested)

| Input | DISARMED | ARMING | ARMED | ALARM_* |
|---|---|---|---|---|
| **A** short | Arm (→ ARMING) | Cancel (→ DISARMED) | Disarm | Disarm |
| **A** long (≥ 1 s) | Recalibrate `base` now (shows ✓) | — | Recalibrate `base` | — |
| **B** short | Cycle sensitivity 1→2→3 (show bars for 1 s) | — | Show current sensitivity | Mute/unmute sound (LEDs keep alarming) |
| **B** long (≥ 1 s) | Cycle sound mode: Siren → Beep → Silent (LED only) | — | Same | — |
| **A+B** | Show status: tilt angle scrolled, e.g. `T12` | — | Show live `dyn` bar graph while held | Acknowledge: → ARMED with new `base` |
| Touch logo (optional) | Test sound + LED sign for 1 s | — | — | — |

Settings (sensitivity, sound mode) are kept in `uBit.storage` (KeyValueStorage) so they survive a reset.

## 6. Software design

```
source/
  main.cpp          uBit.init(), wire up events, start fibers, release_fiber()
  motion.h/.cpp     sampling, g_lp, dyn, tilt, MOVING/STILL/MOVED detection
  alarm.h/.cpp      state machine, button handlers, settings
  ui.h/.cpp         LED images/animations, sound patterns
```

- **motion fiber** (50 Hz): reads the accelerometer, updates the filters, and raises custom events on `uBit.messageBus`:
  - `MOTION_EVT_ID` (custom ID ≥ 9000), values `EVT_MOVING`, `EVT_STILL`, `EVT_MOVED`.
- **alarm**: listens for motion events and `MICROBIT_ID_BUTTON_A/B/AB` events (`MICROBIT_BUTTON_EVT_CLICK`, `MICROBIT_BUTTON_EVT_LONG_CLICK`), and handles state transitions.
- **ui fiber**: draws the LED animation for the current state; a separate **sound fiber** plays the current pattern. Neither blocks detection. All waits use `uBit.sleep()`.
- Detection logic in `motion.cpp` is pure functions (`update(sample) → event`) with no hardware access, so it can be tested on a host machine.
- No dynamic allocation in the main loop; images are `static const MicroBitImage`.

## 7. Acceptance criteria

1. While disarmed, shaking the board causes no sound and no alarm sign.
2. After arming and the 3 s countdown, lifting the board off the table triggers ALARM_MOVING within 200 ms.
3. Putting the board down at a different tilt (> `T_tilt`) shows `!` and plays the beep pattern after about 1 s of stillness.
4. Putting it back down at the same tilt still shows MOVED (latched).
5. Light taps on the table don't trigger the alarm at sensitivity 1, but do at sensitivity 3.
6. B (mute) during an alarm stops the sound but not the LEDs; A+B acknowledges and re-arms.
7. Sensitivity and sound mode survive a reset.
8. No `microbit_panic`; runs ≥ 1 h armed without lockup.

## 8. Out of scope / later

- BLE or radio notification to a second micro:bit (radio alarm relay).
- Battery-saving deep sleep with accelerometer wake-up interrupt.
- Logging events with timestamps to flash.

## 9. Open questions

- Buzzer: on-board speaker only, or also an external piezo on P0?
- Should MOVED latch forever, or auto-clear after N minutes?
- Countdown length: is 3 s enough to put the board down?
