# Graph Report - microbit-v2-misc  (2026-11-20)

## Corpus Check
- 50 files · ~30,754 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 8 file(s) not represented in the graph (top: (none) 8)

## Summary
- 586 nodes · 969 edges · 30 communities (25 shown, 5 thin omitted)
- Extraction: 91% EXTRACTED · 9% INFERRED · 0% AMBIGUOUS · INFERRED: 88 edges (avg confidence: 0.86)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `93af147b`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- Note
- OOB.cpp
- MicrophoneTest.cpp
- DisplayTest.cpp
- DeepSleepTest.cpp
- NoiseProfiler
- OOB_v3.cpp
- PowerManagementTest.cpp
- BlinkyTest.cpp
- USBFlashTest.cpp
- module.json
- CapTouchTest.cpp
- ButtonTest.cpp
- modes
- AccelerometerTest.cpp
- RadioTestRx.cpp
- ui.cpp
- motion.cpp
- alarm.cpp
- Overlay
- PLAN — Motion Alarm
- SPEC — Motion Alarm
- test_ble.cpp
- SerialNumber.cpp
- TESTLOG — Motion Alarm
- AudioTest.cpp
- BLETest.cpp

## God Nodes (most connected - your core abstractions)
1. `Note` - 49 edges
2. `MotionAlarmService` - 18 edges
3. `handleTimer()` - 16 edges
4. `motion_reset()` - 14 edges
5. `handleBleCommand()` - 13 edges
6. `deepsleep_test_send_time()` - 13 edges
7. `feed()` - 13 edges
8. `SPEC — Motion Alarm` - 12 edges
9. `motion_update()` - 11 edges
10. `modes` - 11 edges

## Surprising Connections (you probably didn't know these)
- `Tasks` --references--> `motion_update()`  [INFERRED]
  docs/PLAN.md → source/motion.cpp
- `Tasks` --references--> `motion_reset()`  [INFERRED]
  docs/PLAN.md → source/motion.cpp
- `Tasks` --references--> `analogPitch()`  [INFERRED]
  docs/PLAN.md → source/samples/AudioTest.cpp
- `2. Hardware` --references--> `analogPitch()`  [INFERRED]
  docs/SPEC.md → source/samples/AudioTest.cpp
- `Tasks` --references--> `sleep_enter()`  [INFERRED]
  docs/PLAN.md → source/sleep.cpp

## Import Cycles
- None detected.

## Communities (30 total, 5 thin omitted)

### Community 0 - "Note"
Cohesion: 0.04
Nodes (49): Note, A, A3, A4, A5, B, B3, B4 (+41 more)

### Community 2 - "OOB.cpp"
Cohesion: 0.13
Nodes (26): clap(), dotChaser(), insertNewTarget(), intro(), next(), onLogoTouch(), OOB_onButtonA(), OOB_onButtonAExtra() (+18 more)

### Community 3 - "MicrophoneTest.cpp"
Cohesion: 0.08
Nodes (10): MakeCodeMicrophoneTemplate, level, microphone, mems_clap_test(), onLoud(), onQuiet(), stream_test_all(), stream_test_getValue_interval() (+2 more)

### Community 5 - "DisplayTest.cpp"
Cohesion: 0.10
Nodes (4): onButtonABPressed(), onButtonAPressed(), onButtonBPressed(), onShakePressed()

### Community 6 - "DeepSleepTest.cpp"
Cohesion: 0.26
Nodes (18): deepsleep_test(), deepsleep_test1(), deepsleep_test1_fiber(), deepsleep_test2(), deepsleep_test2_fiber(), deepsleep_test3(), deepsleep_test3_onTimer(), deepsleep_test4() (+10 more)

### Community 7 - "NoiseProfiler"
Cohesion: 0.17
Nodes (5): NoiseProfiler, noiseProfile, NoiseProfiler::NoiseProfiler(), samples, variance

### Community 8 - "OOB_v3.cpp"
Cohesion: 0.26
Nodes (10): onButtonA(), onButtonAB(), onButtonB(), onButtonLogo(), onScreenDown(), onShake(), onStart(), out_of_box_experience() (+2 more)

### Community 9 - "PowerManagementTest.cpp"
Cohesion: 0.16
Nodes (3): power_management_deep_sleep_until_button_b(), power_management_deep_sleep_until_P0_high(), power_management_enter_standby()

### Community 10 - "BlinkyTest.cpp"
Cohesion: 0.23
Nodes (7): button_blinky_test(), button_blinky_test2(), fade_test(), onButtonA(), onButtonB(), setCol(), setDisplay()

### Community 11 - "USBFlashTest.cpp"
Cohesion: 0.24
Nodes (6): onEraseFlash(), onPartialEraseFlash(), onReadFlash(), onRemount(), onStartFlashTest(), onWriteFlash()

### Community 12 - "module.json"
Cohesion: 0.17
Nodes (11): bin, dependencies, codal-microbit, description, extraIncludes, license, name, scripts (+3 more)

### Community 13 - "CapTouchTest.cpp"
Cohesion: 0.24
Nodes (6): onCalibrate(), onPrint(), onTouchLogo(), onTouchP0(), onTouchP1(), onTouchP2()

### Community 14 - "ButtonTest.cpp"
Cohesion: 0.22
Nodes (4): listenerRemoved(), onButtonA(), onButtonAB(), onButtonB()

### Community 15 - "modes"
Cohesion: 0.18
Nodes (11): modes, BUTTON_A, BUTTON_B, CLAP, DOTCHASER, INTRO, LOGO_BUTTON, NEXT (+3 more)

### Community 17 - "AccelerometerTest.cpp"
Cohesion: 0.28
Nodes (3): g_to_pix(), spirit_level(), spirit_level2()

### Community 19 - "ui.cpp"
Cohesion: 0.06
Nodes (41): Done when, Phase 5 — BLE service and phone control, Tasks, alarm_countdown_ms(), alarm_muted(), alarm_sensitivity(), alarm_sound_mode(), alarm_state() (+33 more)

### Community 20 - "motion.cpp"
Cohesion: 0.12
Nodes (27): alarm_dyn(), motion_dyn(), motion_read(), motion_reset(), motion_reset_after_wake(), motion_start(), motion_tilt(), motion_tilt_deg() (+19 more)

### Community 21 - "alarm.cpp"
Cohesion: 0.12
Nodes (37): alarm_init(), handleBleCommand(), handleButtonA(), handleButtonAB(), handleButtonB(), handleMotion(), handleTimer(), loadSettings() (+29 more)

### Community 22 - "Overlay"
Cohesion: 0.25
Nodes (8): Overlay, OVERLAY_BLE, OVERLAY_CHECK, OVERLAY_DYN, OVERLAY_NONE, OVERLAY_SENSITIVITY, OVERLAY_SOUND, OVERLAY_TILT

### Community 23 - "PLAN — Motion Alarm"
Cohesion: 0.07
Nodes (31): Done when, Done when, Done when, Done when, Done when, Done when, Done when, Order and rationale (+23 more)

### Community 24 - "SPEC — Motion Alarm"
Cohesion: 0.09
Nodes (21): 10. Out of scope / later, 11. Decisions, 1. Goal, 3. Definitions, 4. State machine, 5. Buttons, 6. BLE phone notification, 7. Deep sleep with accelerometer wake-up (+13 more)

### Community 25 - "test_ble.cpp"
Cohesion: 0.14
Nodes (12): 8. Software design, ble_cmd_decode(), ble_cmd_get_arg(), ble_cmd_get_cmd(), ble_cmd_pack(), testAlarmEncode(), testBleCodec(), testDecodeInvalid() (+4 more)

### Community 27 - "TESTLOG — Motion Alarm"
Cohesion: 0.33
Nodes (5): Acceptance criteria, Build, Host tests, Risk spikes, TESTLOG — Motion Alarm

### Community 28 - "AudioTest.cpp"
Cohesion: 0.47
Nodes (4): 2. Hardware, analogPitch(), audio_virtual_pin_melody(), playScale()

### Community 29 - "BLETest.cpp"
Cohesion: 0.47
Nodes (3): onConnected(), onDelim(), onDisconnected()

## Knowledge Gaps
- **82 isolated node(s):** `codal-microbit`, `chars`, `stateValue`, `alarmValue`, `commandValue` (+77 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 258 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **5 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Note` connect `Note` to `AudioTest.cpp`?**
  _High betweenness centrality (0.150) - this node is a cross-community bridge._
- **Are the 12 inferred relationships involving `handleTimer()` (e.g. with `ble_alarm_connected()` and `ble_alarm_ms_since_disconnect()`) actually correct?**
  _`handleTimer()` has 12 INFERRED edges - model-reasoned connections that need verification._
- **What connects `codal-microbit`, `chars`, `stateValue` to the rest of the system?**
  _82 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Note` be split into smaller, more focused modules?**
  _Cohesion score 0.04081632653061224 - nodes in this community are weakly interconnected._
- **Why does `SPEC — Motion Alarm` connect `SPEC — Motion Alarm` to `test_ble.cpp`, `AudioTest.cpp`?**
  _High betweenness centrality (0.068) - this node is a cross-community bridge._
- **Are the 8 inferred relationships involving `motion_reset()` (e.g. with `Tasks` and `testFlatAndStill()`) actually correct?**
  _`motion_reset()` has 8 INFERRED edges - model-reasoned connections that need verification._
- **Should `OOB.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.1310344827586207 - nodes in this community are weakly interconnected._