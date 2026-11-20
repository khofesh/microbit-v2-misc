# PLAN — Motion Alarm

Implementation plan for `docs/SPEC.md`. Each phase ends with a working build on the board and a commit. "AC n" refers to acceptance criterion n in SPEC §9.

## Order and rationale

| Phase | Goal                                   | Size | Depends on |
| ----- | -------------------------------------- | ---- | ---------- |
| 0     | Skeleton, host tests, risk spikes      | M    | —          |
| 1     | Motion detection                       | M    | 0          |
| 2     | State machine, buttons, LEDs, sound    | L    | 1          |
| 3     | Settings, mute, sound modes, auto-clear| M    | 2          |
| 4     | Deep sleep with accelerometer wake     | L    | 2, spike A |
| 5     | BLE service and phone control          | L    | 3, spike B |
| 6     | BLE + sleep integration                | S    | 4, 5       |
| 7     | Hardening and acceptance run           | M    | 6          |

The two riskiest parts (reprogramming the accelerometer interrupt, and BLE fitting in memory) are tried out as throwaway spikes in phase 0. If either fails, the SPEC changes before any real code depends on it.

Sleep (4) comes before BLE (5) because it doesn't need the BLE config and is easier to debug without the SoftDevice running.

## Phase 0 — Skeleton, host tests, risk spikes

### Tasks

- [ ] Create the files from SPEC §8 as empty modules: `motion`, `alarm`, `ble_alarm`, `sleep`, `ui`. `main.cpp` calls `uBit.init()`, `uBit.audio.setPinEnabled(false)`, then `release_fiber()`.
- [ ] Define the event IDs in one header (`events.h`): `MOTION_EVT_ID`, `ALARM_EVT_ID`, `BLE_CMD_EVT_ID` (≥ 9000) and their values.
- [ ] Host test tree `tests/`: a plain CMake project built with the host `g++`, compiling `motion.cpp` and the `ble_alarm` codec with `-DHOST_TEST`, plus a minimal assert-based runner. Run with `cmake -S tests -B build/tests && cmake --build build/tests && ctest --test-dir build/tests`.
- [ ] Record the baseline flash/RAM use from `build/MICROBIT.map` (BLE off).
- [ ] Debug output: `uBit.serial.printf` at 115200 baud, read with `screen /dev/ttyACM0 115200`.

### Spike A — accelerometer wake-up (throwaway branch)

- [ ] Write the INT1 registers from SPEC §7, call `uBit.power.deepSleep()`, check that a tap wakes the board.
- [ ] Confirm the polarity on `irq1` (active-low) and that `INT1_SRC_A` shows the IA bit after a motion wake.
- [ ] Restore the registers and check that `uBit.accelerometer.getX()` still updates afterwards.
- [ ] Rough current measurement on battery with USB unplugged.

If it fails, switch SPEC §7 to the timed `deepSleep(500)` fallback and note it in SPEC §11.

### Spike B — BLE fits and pairs (throwaway branch)

- [ ] Apply the SPEC §6 `codal.json` config, build, and record flash/RAM from the map file.
- [ ] Add a dummy `MicroBitBLEService` with one notify characteristic. Pair via A+B + reset, bond from nRF Connect, receive a notification.
- [ ] Check that a second, unbonded phone can't connect (whitelist).

If RAM is too tight, turn off `MICROBIT_BLE_DEVICE_INFORMATION_SERVICE` too, or move `source/samples/` out of `source/` so it isn't compiled.

### Done when

The skeleton builds and runs, `ctest` passes with a dummy test, and both spikes have a written result (works / fallback chosen).

## Phase 1 — Motion detection

### Tasks

- [ ] `motion.cpp` pure core: `struct MotionState`, `void motion_reset(MotionState&, Vec3 base)`, `MotionEvt motion_update(MotionState&, Vec3 a, const Thresholds&)`. Implements `g_lp`, `dyn`, `tilt`, MOVING (3 samples), STILL (1000 ms), MOVED (SPEC §3). Integer or `float` math; no `double`.
- [ ] Sensitivity table (SPEC §3) as `static const Thresholds`.
- [ ] Host tests with synthetic sample streams:
  - flat and still → no event
  - spike above `T_move` for 2 samples → no MOVING; for 3 samples → MOVING
  - moving, then still for 1 s → MOVED, even with the same tilt
  - slow tilt past `T_tilt` without a spike → MOVED after STILL
  - each sensitivity level against the same tap profile
- [ ] Hardware wrapper: a 50 Hz fiber reads `uBit.accelerometer`, calls `motion_update`, and fires `MOTION_EVT_ID` on each event.
- [ ] Debug mode (temporary): print `dyn`, `tilt` and events over serial to tune thresholds on the real board.

### Done when

The host tests pass. On the board, the serial log shows MOVING when the board is lifted and MOVED when it is put down. Thresholds are tuned so that table taps behave as AC 5 describes.

## Phase 2 — State machine, buttons, LEDs, sound

### Tasks

- [ ] `alarm.cpp`: states DISARMED, ARMING, ARMED, ALARM_MOVING, ALARM_MOVED, SLEEP (stub). Transitions from SPEC §4, driven only by events (motion, buttons, timers). Fires `ALARM_EVT_ID` on every change.
- [ ] Buttons: listen for `MICROBIT_BUTTON_EVT_CLICK` / `LONG_CLICK` on A, B and AB. A short/long and A+B ack first; B features come in phase 3.
- [ ] ARMING: 5 s countdown `5`…`1` with a tick, then capture `base` via `motion_reset`.
- [ ] `ui.cpp` LED fiber: per-state images and animations from SPEC §4 (dim dot, heartbeat, countdown, rotating arrows, blinking `!`). `static const MicroBitImage` only.
- [ ] `ui.cpp` sound fiber: siren and beep patterns through `uBit.audio.virtualOutputPin` (as `analogPitch()` in `AudioTest.cpp`). It stops as soon as the state changes.
- [ ] Make sure neither fiber blocks the 50 Hz motion fiber (check the sample rate over serial while the siren plays).

### Done when

AC 1, 2, 3 and 8 pass by hand. The sample rate stays at 50 Hz during alarms.

## Phase 3 — Settings, mute, sound modes, auto-clear

### Tasks

- [ ] B short: cycle sensitivity (DISARMED), show bars (ARMED), mute/unmute (ALARM_*).
- [ ] B long: cycle sound mode Siren → Beep → Silent.
- [ ] A long: recalibrate `base`, show ✓.
- [ ] A+B in DISARMED: scroll the tilt angle; in ARMED: live `dyn` bar graph while held.
- [ ] Touch logo: placeholder for BLE status (shows `-` until phase 5).
- [ ] ALARM_MOVED 60 s auto-clear timer → ARMED with a new `base`; restart it on each new MOVED.
- [ ] Persist sensitivity and sound mode in `uBit.storage`; load at boot with defaults when the keys are missing.

### Done when

AC 4, 5, 6 and 7 pass by hand.

## Phase 4 — Deep sleep with accelerometer wake

### Tasks

- [ ] `sleep.cpp`: `sleep_enter()` (save registers, write INT1 config, clear `INT1_SRC_A`, set wake sources, `uBit.power.deepSleep()`), `sleep_exit()` (restore registers, return the wake cause). Based on the spike A result.
- [ ] `alarm.cpp`: ARMED + STILL for 60 s → SLEEP. The wake cause maps to ALARM_MOVING or ARMED + button handling (SPEC §7).
- [ ] Make sure the UI and sound fibers are idle before sleeping (no display or audio calls during SLEEP).
- [ ] Reset the motion filters after wake so a stale `g_lp` doesn't cause a false MOVED.
- [ ] Measure current in SLEEP on battery with USB unplugged; write the result in SPEC §7.

### Done when

AC 16, 18 (without the phone part) and 19 pass. The measured sleep current is recorded.

## Phase 5 — BLE service and phone control

### Tasks

- [ ] `codal.json`: the BLE config from SPEC §6. Rebuild and compare the map size with the phase 0 baseline.
- [ ] `ble_alarm.cpp`: `MicroBitBLEService` subclass with the service and State / Alarm event / Command characteristics (UUID base from SPEC §6). Command write requires encryption.
- [ ] Pure codec functions (host-tested): encode State, encode Alarm event, decode and validate Command. Tests cover every `cmd`, out-of-range `arg`, wrong length and empty writes.
- [ ] `onDataWritten()` only validates and fires `BLE_CMD_EVT_ID`; `alarm.cpp` handles it like the matching button.
- [ ] On `ALARM_EVT_ID`: update characteristic values, notify State, indicate Alarm event (MOVING / MOVED / CLEARED) when the phone has enabled them. Increment `seq`.
- [ ] Track connect/disconnect (`onConnect` / `onDisconnect`) for the touch-logo status and the sleep rule.
- [ ] Test with nRF Connect: bond, subscribe, trigger alarms, send every command, send garbage.

### Done when

AC 10–15 pass with nRF Connect. AC 1–9 still pass with BLE enabled.

## Phase 6 — BLE + sleep integration

### Tasks

- [ ] Block SLEEP while a phone is connected; allow it again 60 s after disconnect (if still ARMED and STILL).
- [ ] After a motion wake, make sure advertising resumes and a reconnecting phone reads the alarm via the State and Alarm event characteristics.
- [ ] Re-measure current: asleep, awake advertising, and awake connected. Record it in SPEC §7.

### Done when

AC 17 and AC 18 (with the phone part) pass.

## Phase 7 — Hardening and acceptance run

### Tasks

- [ ] Soak test: armed for ≥ 1 h, with alarms and sleep cycles at random, phone connecting and disconnecting (AC 9).
- [ ] Remove the temporary serial debug output, or put it behind `DMESG` so it's off by default.
- [ ] Check build warnings in our files; run `clang-format` on `source/` (excluding `source/samples/`).
- [ ] Final flash/RAM numbers from the map file.
- [ ] Run all of AC 1–19 and record pass/fail with the date in `docs/TESTLOG.md`.
- [ ] Update `README.md` with usage: buttons, pairing, phone setup with nRF Connect.
- [ ] Rebuild the graph: `graphify update .`

### Done when

All acceptance criteria pass and are logged.

## Risks

| Risk                                                          | Impact                         | Mitigation                                                             |
| ------------------------------------------------------------- | ------------------------------ | ---------------------------------------------------------------------- |
| CODAL's LSM303 driver fights INT1 reconfiguration             | No motion wake                 | Spike A; fallback to timed `deepSleep(500)` sampling                   |
| BLE + SoftDevice leaves too little RAM                        | Build fails or heap panics     | Spike B; drop optional BLE services; stop compiling `source/samples/`  |
| Thresholds don't separate taps from real movement             | False alarms or missed alarms  | Tune on hardware in phase 1 with serial logging; sensitivity levels    |
| Sound/LED fibers starve the motion fiber                      | Late or missed detection       | Check the sample rate during alarms (phase 2)                          |
| Deep sleep wake latency too long                              | AC 18 fails (500 ms)           | Skip the MOVING confirmation after a motion wake (already in SPEC §7)  |
| Pairing mode (A+B + reset) confuses users                     | Can't connect a phone          | Document it in the README; show `C` / `-` on touch logo                |
| `build.py` checks out other library versions than the submodules | Build differs locally vs CI | Keep submodules in line with `target-locked.json`                      |
