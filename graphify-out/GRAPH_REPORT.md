# Graph Report - microbit-v2-misc  (2026-11-20)

## Corpus Check
- Corpus is ~21,185 words - fits in a single context window. You may not need a graph.

## Summary
- 352 nodes · 491 edges · 19 communities (16 shown, 3 thin omitted)
- Extraction: 100% EXTRACTED · 0% INFERRED · 0% AMBIGUOUS · INFERRED: 2 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Community Hubs (Navigation)
- Musical Note Constants
- Main & Audio/BLE Samples
- OOB Demo (v2)
- Microphone & Level Detection
- Display Tests
- Deep Sleep Tests
- Noise Profiler
- OOB Experience (v3)
- Power Management Tests
- Blinky & LED Power
- USB Flash Storage Test
- Yotta Module Manifest
- Capacitive Touch Tests
- Button Event Tests
- OOB Mode States
- Accelerometer & Compass
- Radio Tx/Rx Tests

## God Nodes (most connected - your core abstractions)
1. `Note` - 49 edges
2. `deepsleep_test_send_time()` - 13 edges
3. `modes` - 11 edges
4. `play_note()` - 11 edges
5. `NoiseProfiler` - 10 edges
6. `out_of_box_experience_v2()` - 10 edges
7. `deepsleep_test_zeroone()` - 7 edges
8. `deepsleep_test()` - 6 edges
9. `MakeCodeMicrophoneTemplate` - 6 edges
10. `onStartFlashTest()` - 6 edges

## Surprising Connections (you probably didn't know these)
- `clap()` --calls--> `mems_clap_test()`  [INFERRED]
  source/samples/OOB.cpp → source/samples/MicrophoneTest.cpp

## Import Cycles
- None detected.

## Communities (19 total, 3 thin omitted)

### Community 0 - "Musical Note Constants"
Cohesion: 0.04
Nodes (49): Note, A, A3, A4, A5, B, B3, B4 (+41 more)

### Community 1 - "Main & Audio/BLE Samples"
Cohesion: 0.09
Nodes (8): analogPitch(), audio_virtual_pin_melody(), playScale(), onConnected(), onDelim(), onDisconnected(), isPiezoMic2(), showSerialNumber()

### Community 2 - "OOB Demo (v2)"
Cohesion: 0.13
Nodes (26): clap(), dotChaser(), insertNewTarget(), intro(), next(), onLogoTouch(), OOB_onButtonA(), OOB_onButtonAExtra() (+18 more)

### Community 3 - "Microphone & Level Detection"
Cohesion: 0.08
Nodes (10): MakeCodeMicrophoneTemplate, level, microphone, mems_clap_test(), onLoud(), onQuiet(), stream_test_all(), stream_test_getValue_interval() (+2 more)

### Community 5 - "Display Tests"
Cohesion: 0.10
Nodes (4): onButtonABPressed(), onButtonAPressed(), onButtonBPressed(), onShakePressed()

### Community 6 - "Deep Sleep Tests"
Cohesion: 0.26
Nodes (18): deepsleep_test(), deepsleep_test1(), deepsleep_test1_fiber(), deepsleep_test2(), deepsleep_test2_fiber(), deepsleep_test3(), deepsleep_test3_onTimer(), deepsleep_test4() (+10 more)

### Community 7 - "Noise Profiler"
Cohesion: 0.17
Nodes (5): NoiseProfiler, noiseProfile, NoiseProfiler::NoiseProfiler(), samples, variance

### Community 8 - "OOB Experience (v3)"
Cohesion: 0.23
Nodes (10): onButtonA(), onButtonAB(), onButtonB(), onButtonLogo(), onScreenDown(), onShake(), onStart(), out_of_box_experience() (+2 more)

### Community 9 - "Power Management Tests"
Cohesion: 0.16
Nodes (3): power_management_deep_sleep_until_button_b(), power_management_deep_sleep_until_P0_high(), power_management_enter_standby()

### Community 10 - "Blinky & LED Power"
Cohesion: 0.23
Nodes (7): button_blinky_test(), button_blinky_test2(), fade_test(), onButtonA(), onButtonB(), setCol(), setDisplay()

### Community 11 - "USB Flash Storage Test"
Cohesion: 0.24
Nodes (6): onEraseFlash(), onPartialEraseFlash(), onReadFlash(), onRemount(), onStartFlashTest(), onWriteFlash()

### Community 12 - "Yotta Module Manifest"
Cohesion: 0.17
Nodes (11): bin, dependencies, codal-microbit, description, extraIncludes, license, name, scripts (+3 more)

### Community 13 - "Capacitive Touch Tests"
Cohesion: 0.24
Nodes (6): onCalibrate(), onPrint(), onTouchLogo(), onTouchP0(), onTouchP1(), onTouchP2()

### Community 14 - "Button Event Tests"
Cohesion: 0.22
Nodes (4): listenerRemoved(), onButtonA(), onButtonAB(), onButtonB()

### Community 15 - "OOB Mode States"
Cohesion: 0.18
Nodes (11): modes, BUTTON_A, BUTTON_B, CLAP, DOTCHASER, INTRO, LOGO_BUTTON, NEXT (+3 more)

### Community 17 - "Accelerometer & Compass"
Cohesion: 0.28
Nodes (3): g_to_pix(), spirit_level(), spirit_level2()

## Knowledge Gaps
- **66 isolated node(s):** `codal-microbit`, `C`, `CSharp`, `D`, `Eb` (+61 more)
  These have ≤1 connection - possible missing edges. (Counts symbols only; 190 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **3 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Note` connect `Musical Note Constants` to `Main & Audio/BLE Samples`?**
  _High betweenness centrality (0.239) - this node is a cross-community bridge._
- **What connects `codal-microbit`, `C`, `CSharp` to the rest of the system?**
  _66 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Musical Note Constants` be split into smaller, more focused modules?**
  _Cohesion score 0.04081632653061224 - nodes in this community are weakly interconnected._
- **Why does `modes` connect `OOB Mode States` to `OOB Demo (v2)`?**
  _High betweenness centrality (0.053) - this node is a cross-community bridge._
- **Should `Main & Audio/BLE Samples` be split into smaller, more focused modules?**
  _Cohesion score 0.08522727272727272 - nodes in this community are weakly interconnected._
- **Should `OOB Demo (v2)` be split into smaller, more focused modules?**
  _Cohesion score 0.1310344827586207 - nodes in this community are weakly interconnected._
- **Should `Microphone & Level Detection` be split into smaller, more focused modules?**
  _Cohesion score 0.0812807881773399 - nodes in this community are weakly interconnected._