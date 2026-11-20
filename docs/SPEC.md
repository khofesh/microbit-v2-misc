# SPEC — Motion Alarm

Target: BBC micro:bit V2 (nRF52833), CODAL C++. Build: `python build.py` → `MICROBIT.hex`.

## 1. Goal

Detect when the micro:bit is **moving** (in motion right now) or has been **moved** (it's still again, but its position or tilt has changed since it was armed). On either event, make a sound and show an LED sign. Buttons A/B control arming, sensitivity and sound.

A paired **phone** gets a BLE notification on every alarm and can arm, disarm, acknowledge and mute remotely. While armed, still and with no phone connected, the board goes into **deep sleep** to save battery and wakes on motion.

## 2. Hardware

| Item                               | Use                                        |
| ---------------------------------- | ------------------------------------------ |
| LSM303AGR accelerometer (on-board) | Motion and tilt sensing, wake-up interrupt |
| On-board speaker                   | Sound output ("buzzer"), the only output   |
| 5×5 LED matrix                     | Status and alarm signs                     |
| Buttons A, B, touch logo           | User input                                 |
| nRF52833 BLE (Nordic SoftDevice)   | Phone notifications and remote control     |
| Phone with BLE (Android or iOS)    | Receives notifications                     |
| Battery pack (2×AAA, JST)          | Unattended use; reason for deep sleep      |

Audio output uses `uBit.audio.virtualOutputPin`, as in `source/samples/AudioTest.cpp` `analogPitch()`. Sound goes to the on-board speaker only: call `uBit.audio.setPinEnabled(false)` at start-up so nothing is driven on P0, and keep `setSpeakerEnabled(true)`.

## 3. Definitions

Sample rate: 50 Hz (`uBit.accelerometer.setPeriod(20)`), range ±2 g (`setRange(2)`). All values are in mg.

| Term       | Definition                                                                                                      |
| ---------- | --------------------------------------------------------------------------------------------------------------- |
| `a`        | Raw sample `(x, y, z)`                                                                                          |
| `g_lp`     | Low-pass gravity estimate: `g_lp += (a - g_lp) * 0.1`                                                           |
| `dyn`      | Dynamic acceleration: `\|a - g_lp\|`                                                                            |
| `base`     | `g_lp` captured at the moment the alarm is armed                                                                |
| `tilt`     | Angle between `g_lp` and `base`: `acos(dot / (\|g_lp\|·\|base\|))`, in degrees                                  |
| **MOVING** | `dyn > T_move` for ≥ 3 consecutive samples (60 ms)                                                              |
| **STILL**  | `dyn < T_move / 2` for 1000 ms continuously                                                                     |
| **MOVED**  | Device is STILL after a MOVING episode while armed (regardless of final `tilt`), or STILL with `tilt > T_tilt` |

### Sensitivity levels

| Level              | LED bars | `T_move` (mg) | `T_tilt` (°) |
| ------------------ | -------- | ------------- | ------------ |
| 1 Low              | 1        | 250           | 25           |
| 2 Medium (default) | 2        | 120           | 15           |
| 3 High             | 3        | 60            | 8            |

## 4. State machine

```
            A                 countdown done (5 s)
DISARMED ───────► ARMING ─────────────────────► ARMED ◄──────────────┐
   ▲                │ A (cancel)                  │ MOVING            │
   │                ▼                             ▼                   │
   │            DISARMED                       ALARM_MOVING           │ auto-clear: 60 s
   │                                              │ STILL for 1 s     │ with no motion
   │   A+B (ack) / A (disarm)                     ▼                   │ (new base)
   └──────────────────────────────────────── ALARM_MOVED ────────────┘
                                                  │ MOVING again
                                                  └──► ALARM_MOVING

ARMED ──(STILL for 60 s, no phone connected)──► SLEEP ──(motion IRQ)──► ALARM_MOVING
                                                  └──(button A/B)──► ARMED (then handle press)
```

Phone commands (§6) cause the same transitions as the matching buttons.

| State        | Entry action                       | LED                                          | Sound                                       |
| ------------ | ---------------------------------- | -------------------------------------------- | ------------------------------------------- |
| DISARMED     | Stop sound                         | Centre pixel dim (brightness 20)             | Silent                                      |
| ARMING       | 5 s countdown, then capture `base` | `5`, `4`, `3`, `2`, `1`                      | Short tick each second                      |
| ARMED        | —                                  | Centre pixel heartbeat (on 100 ms every 2 s) | Silent                                      |
| ALARM_MOVING | Start siren, notify phone          | Animated arrows (rotating, 150 ms/frame)     | Two-tone siren 880 Hz / 660 Hz, 250 ms each |
| ALARM_MOVED  | Start chirp, start 60 s timer, notify phone | Blinking `!` (500 ms on/off)        | 3 × 100 ms beeps at 1500 Hz every 2 s       |
| SLEEP        | See §7                             | Off                                          | Silent                                      |

ALARM_MOVED stays on until the user acknowledges it (A+B or phone), disarms it (A or phone), or until it **auto-clears after 60 s** with no new motion. Auto-clear goes back to ARMED and captures the current position as the new `base`. Any MOVING event during those 60 s goes back to ALARM_MOVING and restarts the timer when it settles again.

## 5. Buttons

| Input                 | DISARMED                                           | ARMING              | ARMED                                | ALARM\_\*                              |
| --------------------- | -------------------------------------------------- | ------------------- | ------------------------------------ | -------------------------------------- |
| **A** short           | Arm (→ ARMING)                                     | Cancel (→ DISARMED) | Disarm                               | Disarm                                 |
| **A** long (≥ 1 s)    | Recalibrate `base` now (shows ✓)                   | —                   | Recalibrate `base`                   | —                                      |
| **B** short           | Cycle sensitivity 1→2→3 (show bars for 1 s)        | —                   | Show current sensitivity             | Mute/unmute sound (LEDs keep alarming) |
| **B** long (≥ 1 s)    | Cycle sound mode: Siren → Beep → Silent (LED only) | —                   | Same                                 | —                                      |
| **A+B**               | Show status: tilt angle scrolled, e.g. `T12`       | —                   | Show live `dyn` bar graph while held | Acknowledge: → ARMED with new `base`   |
| Touch logo (optional) | Show BLE status: `C` connected / `-` not connected | —                   | Same                                 | —                                      |

In SLEEP, pressing A or B wakes the board into ARMED and then handles the press as in the ARMED column.

**BLE pairing:** hold A+B and press reset to enter CODAL's pairing mode, then pair from the phone (§6). This is CODAL's built-in behaviour and happens before the alarm firmware starts.

Settings (sensitivity, sound mode) are kept in `uBit.storage` (KeyValueStorage) so they survive a reset. Bonded phones are stored by CODAL.

## 6. BLE phone notification

### Configuration (`codal.json`)

Start from `codal.ble.json` and keep only what's needed:

```json
"config": {
    "DEVICE_BLE": 1,
    "MICROBIT_BLE_ENABLED": 1,
    "MICROBIT_BLE_PAIRING_MODE": 1,
    "MICROBIT_BLE_SECURITY_LEVEL": "SECURITY_MODE_ENCRYPTION_NO_MITM",
    "MICROBIT_BLE_WHITELIST": 1,
    "MICROBIT_BLE_DFU_SERVICE": 0,
    "MICROBIT_BLE_DEVICE_INFORMATION_SERVICE": 1,
    "MICROBIT_BLE_EVENT_SERVICE": 0,
    "MICROBIT_BLE_PARTIAL_FLASHING": 0
}
```

- Pairing is "Just Works" with an encrypted link, and only bonded phones may connect (whitelist). This stops a stranger nearby from disarming the alarm.
- `uBit.radio` is not used; CODAL can't run radio and BLE together.
- BLE takes a large share of RAM and flash for the SoftDevice. Check the `MICROBIT.map` size after enabling it.

### GATT service: Motion Alarm Service

A custom service built on `MicroBitBLEService` (see `libraries/codal-microbit-v2/inc/MicroBitBLEService.h`). UUIDs share the base `c95aXXXX-36d7-4d56-bfbc-f6b93aab3e5e`.

| Characteristic | UUID (`XXXX`) | Properties            | Value                                                                                       |
| -------------- | ------------- | --------------------- | ------------------------------------------------------------------------------------------- |
| Service        | `0001`        | —                     | —                                                                                           |
| State          | `0002`        | Read, Notify          | 4 bytes: `state`, `sensitivity` (1–3), `sound_mode` (0 siren / 1 beep / 2 silent), `flags` (bit0 muted) |
| Alarm event    | `0003`        | Read, Indicate        | 4 bytes: `type` (1 MOVING, 2 MOVED, 3 CLEARED), `seq` (`uint16_t` LE), `tilt` (degrees, 0–180) |
| Command        | `0004`        | Write (encrypted)     | 2 bytes: `cmd`, `arg` (see below)                                                           |

**Alarm event** uses indications (acknowledged by the phone), so an alarm isn't silently lost. **State** uses notifications and is sent on every state or setting change.

| `cmd` | Name     | `arg`                | Same as        |
| ----- | -------- | -------------------- | -------------- |
| 1     | ARM      | —                    | A in DISARMED  |
| 2     | DISARM   | —                    | A              |
| 3     | ACK      | —                    | A+B in alarm   |
| 4     | MUTE     | 0 off / 1 on         | B in alarm     |
| 5     | SET_SENS | 1–3                  | B in DISARMED  |
| 6     | SET_SOUND| 0–2                  | B long         |

Invalid commands or arguments are ignored. Commands that don't apply in the current state (e.g. ACK while ARMED) are ignored too.

### Behaviour

| Event                          | BLE action                                                           |
| ------------------------------ | -------------------------------------------------------------------- |
| Enter ALARM_MOVING / ALARM_MOVED | Indicate Alarm event; notify State                                 |
| Ack / disarm / auto-clear      | Indicate Alarm event `CLEARED`; notify State                         |
| Any other state/setting change | Notify State                                                         |
| Phone connects                 | Phone reads State and Alarm event to catch up on anything it missed |
| Phone disconnects              | Keep advertising; alarm works locally as usual                       |

### Phone side

- **Testing:** nRF Connect (Android/iOS). Bond, enable notify/indicate on `0002`/`0003`, and write commands to `0004`.
- **Real notifications:** the OS only shows a pop-up when an app turns the BLE indication into one. That needs a small companion app. A Web Bluetooth page (Chrome on Android) is the cheapest option, but it is out of scope here (§10).

## 7. Deep sleep with accelerometer wake-up

Applies in ARMED only, and only when **no phone is connected**. DISARMED, ARMING and alarm states never sleep. CODAL suspends the SoftDevice during deep sleep, which would drop the phone connection, so a connected phone keeps the board awake (normal `uBit.sleep()` idle).

### Entry

After ARMED + STILL for 60 s with no BLE connection, configure the LSM303AGR to raise an interrupt on motion, enable wake sources, then call `uBit.power.deepSleep()`. BLE advertising stops while asleep and resumes on wake.

### Accelerometer interrupt

The LSM303AGR INT1 line reaches the nRF on `uBit.io.irq1` (P0.25, active-low, pulled up by CODAL). CODAL normally uses INT1 for "data ready" (`CTRL_REG3_A = 0x10`). Before sleeping:

1. Save the current `CTRL_REG2_A`, `CTRL_REG3_A`, `INT1_CFG_A`, `INT1_THS_A` and `INT1_DURATION_A` values.
2. Write the following registers over `uBit.i2c` (address `0x32`):
   - `CTRL_REG2_A`: high-pass filter on the INT1 path.
   - `INT1_THS_A`: `T_move` converted to register units (16 mg/LSB at ±2 g).
   - `INT1_DURATION_A = 1`.
   - `INT1_CFG_A = 0x2A` (OR of X/Y/Z high events).
   - `CTRL_REG3_A = 0x40` (IA1 on INT1).
3. Read `INT1_SRC_A` to clear any interrupt still pending.
4. `uBit.io.irq1.wakeOnActive(true)`, `uBit.buttonA.wakeOnActive(true)`, `uBit.buttonB.wakeOnActive(true)`.

On wake, restore the saved registers so normal CODAL sampling works again, read `INT1_SRC_A` to find the wake reason, and then:

| Wake cause                     | Next state                                                    |
| ------------------------------ | ------------------------------------------------------------- |
| Accelerometer IRQ (IA bit set) | ALARM_MOVING immediately (no MOVING confirmation); advertise so the phone can reconnect and read the alarm |
| Button A/B                     | ARMED, then handle the press                                  |

The register values and the IRQ polarity must be checked on real hardware. If the CODAL driver gets in the way of reconfiguring INT1, fall back to a timed `deepSleep(500)` loop with a short wake-and-sample burst in between. That uses more power, but keeps the same behaviour.

### Power target

- About 20 µA average in SLEEP, plus the board's DAPLink/USB chip. Measure on battery with USB unplugged, because USB keeps the interface chip powered.
- The display, speaker and microphone are off in SLEEP. CODAL powers them down; make sure nothing calls them from a fiber.
- Awake with BLE advertising or connected uses noticeably more; measure that too.

## 8. Software design

```
source/
  main.cpp          uBit.init(), wire up events, start fibers, release_fiber()
  motion.h/.cpp     sampling, g_lp, dyn, tilt, MOVING/STILL/MOVED detection (pure)
  alarm.h/.cpp      state machine, button handlers, auto-clear timer, settings
  ble_alarm.h/.cpp  Motion Alarm GATT service (MicroBitBLEService subclass), command parsing
  sleep.h/.cpp      LSM303 INT1 setup/restore, wake sources, deep-sleep entry
  ui.h/.cpp         LED images/animations, sound patterns
```

- **motion fiber** (50 Hz): reads the accelerometer, updates the filters, and raises custom events on `uBit.messageBus`:
  - `MOTION_EVT_ID` (custom ID ≥ 9000), values `EVT_MOVING`, `EVT_STILL`, `EVT_MOVED`.
- **alarm**: listens for motion events, `MICROBIT_ID_BUTTON_A/B/AB` events (`MICROBIT_BUTTON_EVT_CLICK`, `MICROBIT_BUTTON_EVT_LONG_CLICK`) and `BLE_CMD_EVT_ID` events, and handles state transitions. It raises `ALARM_EVT_ID` on each state change; `ble_alarm` and `ui` listen for it.
- **ble_alarm**: `onDataWritten()` validates the command and raises `BLE_CMD_EVT_ID` (no state changes inside the BLE callback). On `ALARM_EVT_ID` it updates State and Alarm event and sends notify/indicate when enabled. It tracks connect/disconnect for the sleep rule.
- **ui fiber**: draws the LED animation for the current state; a separate **sound fiber** plays the current pattern. Neither blocks detection. All waits use `uBit.sleep()`.
- Detection logic in `motion.cpp` and command/payload encode/decode in `ble_alarm.cpp` are pure functions with no hardware access, so they can be tested on a host machine.
- No dynamic allocation in the main loop; images are `static const MicroBitImage`.

## 9. Acceptance criteria

### Core

1. While disarmed, shaking the board causes no sound and no alarm sign.
2. After arming and the 5 s countdown, lifting the board off the table triggers ALARM_MOVING within 200 ms.
3. Putting the board down after moving it shows `!` and plays the beep pattern after about 1 s of stillness, whether or not the tilt changed.
4. If left untouched, ALARM_MOVED clears by itself after 60 s and returns to ARMED. Moving it again during those 60 s restarts the alarm.
5. Light taps on the table don't trigger the alarm at sensitivity 1, but do at sensitivity 3.
6. B (mute) during an alarm stops the sound but not the LEDs; A+B acknowledges and re-arms.
7. Settings survive a reset.
8. Nothing is driven on P0 at any time (check with a scope or LED on P0).
9. No `microbit_panic`; runs ≥ 1 h armed without lockup.

### BLE

10. A phone that has bonded via pairing mode can connect; an unbonded phone is rejected.
11. With a phone connected and indications on, the phone receives the Alarm event within 1 s of ALARM_MOVING.
12. ARM, DISARM, ACK, MUTE, SET_SENS and SET_SOUND from the phone have the same effect as the buttons, and the State notification reflects them.
13. Garbage writes to the Command characteristic are ignored without a crash.
14. A phone that connects after an alarm can read the last Alarm event and current State.
15. Disconnecting the phone doesn't affect local alarm behaviour.

### Deep sleep

16. 60 s after the board is armed and still, with no phone connected, the display turns off and current drops to the sleep target (measured on battery).
17. With a phone connected, the board does not deep sleep.
18. Lifting a sleeping board triggers ALARM_MOVING within 500 ms, and a phone can reconnect and read the alarm.
19. Pressing A on a sleeping board disarms it; pressing B shows the sensitivity.

## 10. Out of scope / later

- Companion phone app or Web Bluetooth page that turns BLE indications into OS notifications.
- Logging events with timestamps to flash.

## 11. Decisions

| Question             | Decision                                         |
| -------------------- | ------------------------------------------------ |
| Buzzer output        | On-board speaker only; P0 disabled               |
| MOVED latching       | Auto-clears after 1 minute with no new motion    |
| Arming countdown     | 5 s                                              |
| Remote notification  | BLE to a bonded phone; no radio relay            |
| Deep sleep vs BLE    | Sleep only when no phone is connected            |
