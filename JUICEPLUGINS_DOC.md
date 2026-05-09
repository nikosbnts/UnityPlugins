# JucePlugins — Technical Documentation

## Overview

Research-grade audio plugin project developed for a diploma thesis on spatial audio perception. The project contains two JUCE-based VST3 plugins that are also compiled as Unity native audio plugins:

- **BinauralPlugin** — HRTF-based binaural spatial rendering with optional VBAP-hybrid modes
- **VBAPPlugin** — Vector-Based Amplitude Panning for 2–8 discrete loudspeakers

Both plugins share a common test-session framework designed for psychoacoustic localization experiments.

---

## Build System

| Item | Value |
|------|-------|
| Build tool | CMake ≥ 3.24 |
| Language | C++20 |
| JUCE source | CPM-fetched from GitHub master |
| Build directory | `build/` |
| Output formats | VST3, Standalone, Unity (`.dll`) |

```bash
cmake -B build
cmake --build build --config Release
```

After building:
- VST3 plugins are copied to `VST3/`
- Unity DLLs (`audiopluginBinaural.dll`, `audiopluginVBAP.dll`) are copied to `VST-Test/Assets/Plugins/`

The Unity copy destination is hardcoded in `CMakeLists.txt` as `UNITY_PLUGIN_COPY_DIR`. Building on a different machine requires updating that variable.

---

## Source Structure

```
Source/
├── Common/
│   ├── TestSession.h       — session/trial data structs, CSV export, statistics
│   ├── VbapEngine2D.h      — header-only 2D VBAP algorithm (up to 36 speakers)
│   └── AudioPlayer.h       — thread-safe mono audio file player
├── BinauralPlugin/
│   ├── HrirBank.h/cpp      — HRTF loader: 360 WAV files at 1° azimuth steps
│   ├── PluginProcessor.h/cpp — binaural engine: FIR convolution, HRTF crossfade, VBAP hybrid
│   └── BinauralTestSessionEditor.h/cpp — dark-theme UI, 5 screens
└── VBAPPlugin/
    ├── PluginProcessor.h/cpp — per-sample VBAP gain application, 8-channel output
    └── VbapTestSessionEditor.h/cpp — light-theme UI, speaker config
```

---

## Common Modules

### VbapEngine2D.h

Header-only 2D VBAP implementation in the `vbap` namespace. Supports up to 36 speakers (`kMaxSpeakers = 36`).

**Algorithm:**
1. Sort speakers by azimuth angle
2. Test each adjacent speaker pair: solve the 2×2 linear system to find gains `gA`, `gB` for the pair spanning the source direction
3. If both gains are non-negative (within a small epsilon), normalize them and return
4. If no pair contains the source, fall back to the nearest single speaker with gain = 1.0

Key functions:
- `computeVBAP_N(sourceAz, speakerAz[], n, outGains[])` — main entry point
- `solvePair(srcAz, spkA, spkB, &gA, &gB)` — 2×2 matrix solve via determinant
- `azToDir(azDeg)` — converts azimuth to a unit 2D direction vector (x=sin, y=cos)
- `DefaultVBAPLayoutAngles(n)` — returns standard speaker positions for n=2..8

The same algorithm is duplicated in C# inside the Unity project (`VBAPPluginRuntimeUI.cs`, `BinauralPluginRuntimeUI.cs`) so the Unity scene can visualize VBAP gains in real time without calling back into the plugin.

### TestSession.h

Shared struct that drives the experiment state machine for both plugins.

**State machine:** `Setup → Trial → Feedback → Summary`

```cpp
struct TrialResult {
    int   trialNumber;
    float targetAngle;
    float responseAngle;
    float angularError;    // shortest arc, 0–180°
    int   confidence;      // 1–5
    juce::String configName;
    juce::String timestamp;
};
```

Key methods:
- `submitCurrentTrial()` — records result, increments trial counter, transitions to Feedback
- `advanceAfterFeedback()` — transitions to next Trial or to Summary
- `toCSV()` — generates a semicolon-delimited CSV (Excel-compatible; decimal numbers use comma as separator)
- `angularError(a, b)` — shortest angular distance between two azimuths (0–180°)
- `parseAngles(text)` — parses a comma/space/semicolon-separated angle list from a text field
- `preset8()` — returns `{0, 45, 90, 135, 180, 225, 270, 315}` as a standard 8-trial set

CSV header block: `Session Name`, `Participant ID`, `Audio Source`, `Audio File`, `Total Trials`  
Per-trial columns: `Trial`, `Configuration`, `TargetAngle`, `ResponseAngle`, `AngularError`, `Confidence`, `Timestamp`

### AudioPlayer.h

A thread-safe, single-class audio file player designed to live inside the processor and be called from `processBlock`.

- `loadFile(file)` — called from the GUI thread; decodes and downmixes to mono via `SpinLock`
- `getNextSample()` — called from the audio thread; pulls one mono sample, advances read position
- Supports looping (`setLooping`), play/pause/stop, and exposes `isPlaying()` / `isLoaded()`
- Uses `std::atomic` for all control flags (play, loaded, loop, readPosition)
- Uses `SpinLock::ScopedTryLockType` in `getNextSample()` — returns 0.0 if lock is not immediately available (non-blocking audio thread behavior)

**Important:** stereo files are downmixed to mono (L+R averaged). The right channel is discarded.

---

## HRTF Dataset

**ARI HRTF database** measured on a **Neumann KU100** binaural dummy head.

| Sample rate variant | Folder name |
|--------------------|-------------|
| 44.1 kHz / 16-bit | `44K_16bit_0ele/` |
| 48 kHz / 24-bit | `48K_24bit_0ele/` ← **active** |
| 96 kHz / 24-bit | `96K_24bit_0ele/` |

File naming convention: `azi_NNN,0_ele_0,0.wav` (European decimal notation — comma as decimal separator). Elevation is fixed at 0°.

360 integer-degree files are loaded (0° through 359°, step = 1°). Fractional-angle ARI measurement files in the folder are silently ignored by the loader.

**Search paths (in priority order):**
1. `~/Documents/BinauralPlugin/HRIR/48K_24bit_0ele/` — portable path for running on any machine
2. `C:/Users/nikos/Desktop/Diplomatiki/JucePlugins/Assets/0ele/48K_24bit_0ele/` — developer fallback

If any of the 360 required files is missing, `HrirBank::loadFromFolder()` returns `false` and the plugin silently produces no output.

### HrirBank Implementation

```
HrirBank::loadFromFolder()
  → iterate az = 0..359
  → construct filename "azi_N,0_ele_0,0.wav"
  → read 2-channel WAV into HrirBank::Entry { azimuthDeg, ir[2ch], sampleRate }
  → fail if any file is missing or channel count < 2
  → verify sample rate consistency across all files
  → store in entries[] (indexed directly by azimuth degree)

HrirBank::getNearest(azimuthDeg)
  → round to nearest integer, wrap to [0, 359]
  → direct array access: entries[roundedAz]
```

---

## Binaural Plugin — Audio Pipeline

```
DAW/File Input
  → buildMonoInput()   [stereo → mono downmix, or internal AudioPlayer]
  → inputGain multiplication
  → pushInputSample()  [circular ring buffer, size = max HRIR length]
  → smoothedSourceAzimuth.getNextValue()  [20 ms linear ramp, per sample]
  → buildRenderSelection()   [determines which HRTF pair to use]
  → crossfade logic          [40 ms sin/cos crossfade on HRTF change]
  → renderSelectionSample()  [FIR convolution → L + R output]
  → Stereo output
```

### Layout Modes

The `layoutMode` parameter selects how the virtual source is rendered:

| Mode | Name | Description |
|------|------|-------------|
| 0 | Direct HRTF | Single nearest HRTF at the source azimuth (axis-mirrored) |
| 1 | VBAP 5 | 5-speaker VBAP hybrid; two topologies |
| 2 | VBAP 7 | 7-speaker VBAP hybrid; two topologies |
| 3 | VBAP 9 | 9-speaker VBAP hybrid; two topologies |
| 4 | VBAP 12 | 12-speaker VBAP hybrid; two topologies |
| 5 | VBAP 18 | 18-speaker VBAP hybrid; symmetric only |

In VBAP modes, the engine finds the two speakers that bracket the source direction, then weights two HRTFs (one per speaker) by the VBAP gains. This produces a binaural image that trades a bit of HRTF accuracy for smoother azimuth panning.

### Topology Parameter

For modes 1–4, a second `topology` parameter (0 or 1) selects between two speaker arrangements:

| Mode | Topology 0 | Topology 1 |
|------|-----------|-----------|
| VBAP 5 | Cinema (0, 30, 120, 240, 330°) | Symmetric (0, 60, 120, 240, 300°) |
| VBAP 7 | Cinema (0, 30, 120, 150, 210, 240, 330°) | Symmetric (evenly spaced ≈51.4° steps) |
| VBAP 9 | Symmetric (40° steps) | Asymmetric (0, 30, 60, 100, 150, 210, 260, 300, 330°) |
| VBAP 12 | Symmetric (30° steps) | Asymmetric (irregular cinema-style layout) |

VBAP 18 uses a fixed symmetric arrangement (20° steps) regardless of topology.

### FIR Convolution

A ring buffer (`inputHistory`) stores recent mono input samples. Its length equals the longest HRIR in the bank. For each output sample, the convolution iterates over all HRIR taps:

```cpp
float acc = 0;
for (int i = 0; i < tapCount; i++)
    acc += inputHistory[readPos--] * ir[i];
output = acc * gain;
```

Two convolutions are computed per sample (L channel and R channel), and in crossfade periods, four convolutions run simultaneously (previous + current selection × 2 ears).

### HRTF Crossfade

When `buildRenderSelection()` detects an HRTF pair change (different azimuth indices or gain delta > 0.02), a 40 ms crossfade is triggered:

```
fadeIn  = sin(alpha × π/2)   // current selection ramps in
fadeOut = cos(alpha × π/2)   // previous selection ramps out
output  = prev × fadeOut + curr × fadeIn
```

This is a standard equal-power crossfade. The crossfade is triggered per-sample inside `processBlock`, not at block boundaries.

### Parameters (BinauralPlugin)

| ID | Name | Range | Default |
|----|------|-------|---------|
| `inputGain` | Input Gain | 0.0–1.0 | 1.0 |
| `sourceAzimuth` | Source Azimuth | 0–360° | 0° |
| `layoutMode` | Layout Mode | 0–5 (choice) | 0 |
| `topology` | Topology | 0–1 (choice) | 0 |

---

## VBAP Plugin — Audio Pipeline

```
DAW/File Input
  → tempMonoInput  [stereo → mono downmix, or internal AudioPlayer]
  → computeVBAP_N(sourceAzimuth, speakerAz[], n, gains[])
  → for each output channel ch:
       buffer[ch][i] = monoIn[i] × inputGain × gains[ch]
  → unused channels cleared to zero
  → 2–8 channel discrete output
```

The VBAP plugin is deliberately simple. There is no convolution, no smoothing, and no crossfade — gain changes take effect immediately at the block boundary.

### Parameters (VBAPPlugin)

| ID | Name | Range | Default |
|----|------|-------|---------|
| `inputGain` | Input Gain | 0.0–1.0 | 1.0 |
| `sourceAzimuth` | Source Azimuth | 0–360° | 0° |
| `speakerCount` | Speaker Count | 2–8 | 2 |
| `speakerAz1`–`speakerAz8` | Speaker 1–8 Azimuth | 0–360° | default layout |

Default azimuth angles for each speaker count match the `DefaultVBAPLayoutAngles()` function.

---

## Test Session UI

Both plugins embed a full psychoacoustic test session editor using JUCE's `AudioProcessorEditor`.

**Screen flow:**

```
Setup screen
  └── researcher configures: session name, participant ID,
      target angles (manual / preset / random),
      configuration labels, audio file or DAW input
  └── [Start Session] →

Trial screen (repeated per trial)
  └── source angle is set; subject hears spatialized audio
  └── subject clicks a virtual compass to indicate perceived direction
  └── subject selects confidence (1–5)
  └── [Submit] →

Feedback screen
  └── shows correct angle, user's response, angular error
  └── auto-advances after a timeout →

Summary screen
  └── displays mean / best / worst angular error
  └── [Export CSV] button
```

---

## Key Design Notes

- The VBAP algorithm is duplicated in C# (Unity side) to enable real-time gain visualization without plugin round-trips
- The azimuth smoothing in BinauralPlugin uses `juce::SmoothedValue` with `unwrapTargetAzimuthNearReference()` to prevent wrap-around discontinuities (e.g., jumping from 350° to 5° takes the short path)
- `AudioPlayer::getNextSample()` uses `ScopedTryLockType` (non-blocking) to protect against the GUI thread holding the lock during a file load
- Both plugins serialize parameters via `AudioProcessorValueTreeState` XML state for DAW preset persistence
- The Unity copy path in `CMakeLists.txt` is hardcoded; change `UNITY_PLUGIN_COPY_DIR` when building on a different machine
