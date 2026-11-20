# TESTLOG — Motion Alarm

Date: 2026-11-20

## Build

| Build                             | Flash                       | Static RAM (`.data` + `.bss`) | Heap     |
| --------------------------------- | --------------------------- | ----------------------------- | -------- |
| Baseline, BLE off (before work)   | 123356 B / 372 KB (33.09 %) | —                             | —        |
| Motion alarm, BLE off             | 109204 B / 372 KB (29.30 %) | —                             | —        |
| Motion alarm, BLE on (final)      | 126876 B / 372 KB (34.04 %) | 8720 B                        | 111928 B |

All builds completed without warnings or errors in the new application files. Only upstream
warnings remain (CODAL `-Woverloaded-virtual`, an unused DFU helper and `source/samples/`).
`clang-format` applied to `source/*.cpp`, `source/*.h` (excluding `source/samples/`) and
`tests/`.

## Host tests

`cmake -S tests -B build/tests && cmake --build build/tests && ctest --test-dir build/tests`

| Test                                                                 | Result |
| -------------------------------------------------------------------- | ------ |
| Flat and still produces no MOVING/MOVED                              | PASS   |
| Two-sample spike does not produce MOVING                             | PASS   |
| Three-sample spike produces MOVING                                   | PASS   |
| MOVING followed by 1 s stillness produces MOVED                      | PASS   |
| Slow tilt past `T_tilt` produces MOVED without MOVING                | PASS   |
| Sensitivity 1 ignores a tap that triggers at 2 and 3                 | PASS   |
| Sensitivity 1 ignores a 24° tilt that triggers at 2 and 3            | PASS   |
| Threshold table matches SPEC §3                                      | PASS   |
| State / Alarm event encoding                                         | PASS   |
| Command decode accepts every valid command and argument              | PASS   |
| Command decode rejects garbage, wrong lengths and bad arguments      | PASS   |

## Acceptance criteria

Hardware items are marked pending: no micro:bit was attached to the build machine, so no
flash, pairing, current measurement or soak run could be performed.

| AC  | Description                                                          | Result  |
| --- | -------------------------------------------------------------------- | ------- |
| 1   | Disarmed shaking is silent and shows no alarm sign                   | Pending |
| 2   | Lifting after arming triggers ALARM_MOVING within 200 ms             | Pending |
| 3   | Putting it down shows `!` and beeps after ~1 s of stillness          | Pending |
| 4   | ALARM_MOVED auto-clears after 60 s; new motion restarts it           | Pending |
| 5   | Light taps: no alarm at sensitivity 1, alarm at 3                    | Pending |
| 6   | B mutes sound but not LEDs; A+B acknowledges and re-arms             | Pending |
| 7   | Sensitivity and sound mode survive a reset                           | Pending |
| 8   | Nothing is driven on P0                                              | Pending |
| 9   | No panic; armed for >= 1 h                                           | Pending |
| 10  | Bonded phone connects; unbonded phone is rejected                    | Pending |
| 11  | Alarm indication arrives within 1 s of ALARM_MOVING                  | Pending |
| 12  | ARM/DISARM/ACK/MUTE/SET_SENS/SET_SOUND behave like the buttons       | Pending |
| 13  | Garbage command writes are ignored without a crash                   | Pending |
| 14  | A late-connecting phone reads State and the last Alarm event         | Pending |
| 15  | Disconnecting the phone does not change local behaviour              | Pending |
| 16  | 60 s armed + still + no phone: display off, sleep current measured   | Pending |
| 17  | A connected phone prevents deep sleep                                | Pending |
| 18  | Lifting a sleeping board alarms within 500 ms; phone can reconnect   | Pending |
| 19  | A on a sleeping board disarms; B shows sensitivity                   | Pending |

## Risk spikes

| Spike                                      | Result                                                                                                  |
| ------------------------------------------ | ------------------------------------------------------------------------------------------------------- |
| A — accelerometer wake-up                  | Not measured on hardware. Implementation writes `CTRL_REG2/INT1_CFG/THS/DURATION/CTRL_REG3`, clears `INT1_SRC`, enables button wake sources and restores the registers on wake. The timed `deepSleep(500)` fallback is not enabled. |
| B — BLE fits and pairs                     | Build fits: 126876 B flash (34 %), 8.7 KB static RAM, ~109 KB heap. Pairing/bonding not tested on hardware. |
