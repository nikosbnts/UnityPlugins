# JucePlugins — CLAUDE.md

## Project Overview

Research-grade audio plugin project for a diploma thesis on spatial audio perception. Contains two JUCE-based VST3/Unity plugins:

- **BinauralPlugin** — HRTF-based binaural rendering with optional VBAP-hybrid spatial audio, plus a full test-session UI for psychoacoustic experiments
- **VBAPPlugin** — Multi-speaker Vector-Based Amplitude Panning (2–8 speakers) with the same test-session framework

Both plugins are also exported as Unity native audio plugins (`audiopluginBinaural.dll`, `audiopluginVBAP.dll`) copied to `C:/Users/nikos/Desktop/Diplomatiki/VST-Test/Assets/Plugins/`.

---

## Build System

- **CMake** (≥ 3.24), C++20
- **JUCE** fetched via CPM from GitHub master
- Build directory: `build/`

```
cmake -B build
cmake --build build --config Release
```

Formats built: VST3, Standalone, Unity. Built plugins auto-copied to:
- VST3 → `VST3/`
- Unity DLLs → `C:/Users/nikos/Desktop/Diplomatiki/VST-Test/Assets/Plugins/`

---

## Source Structure

```
Source/
├── Common/
│   ├── TestSession.h       # Session/trial data structs, CSV export, statistics
│   ├── VbapEngine2D.h      # Header-only 2D VBAP algorithm (up to 36 speakers)
│   └── AudioPlayer.h       # Thread-safe mono audio file player (atomic controls)
├── BinauralPlugin/
│   ├── HrirBank.h/cpp      # HRTF loader — loads 360 WAV files at 1° azimuth steps
│   ├── PluginProcessor.h/cpp   # Binaural engine: ring-buffer FIR convolution, HRTF crossfade, VBAP hybrid
│   └── BinauralTestSessionEditor.h/cpp  # Dark-theme UI, 5 screens (Setup/Trial/Feedback/Summary)
└── VBAPPlugin/
    ├── PluginProcessor.h/cpp   # Simple per-sample VBAP gain application
    └── VbapTestSessionEditor.h/cpp  # Light-theme UI, speaker config
```

---

## HRTF Dataset

**ARI HRTF database** measured on a **Neumann KU100** dummy head.
- Located at `Assets/0ele/` in three sample-rate variants: `44K_16bit`, `48K_24bit`, `96K_24bit`
- Active at runtime: `48K_24bit_0ele/` (primary), with fallback to the hardcoded Assets path
- File naming: `azi_NNN,D_ele_0,0.wav` — European decimal notation (comma), elevation fixed at 0°
- 360 integer-degree files used (1° step); fractional-angle files in the folder are extra ARI measurement points that the loader ignores

HRTF search paths (in `PluginProcessor.cpp`):
1. `~/Documents/BinauralPlugin/HRIR/48K_24bit_0ele/`
2. `C:/Users/nikos/Desktop/Diplomatiki/JucePlugins/Assets/0ele/48K_24bit_0ele/`

---

## Binaural Audio Pipeline

```
DAW/File Input
  → Mono downmix
  → SmoothedValue azimuth ramp (20 ms)
  → Layout mode selection (Direct HRTF or VBAP 5/7/9/12/18 hybrid)
  → buildRenderSelection() → RenderSelection (HRTF pair + gains)
  → Ring-buffer FIR convolution (dual HRTFs → L/R ears)
  → 40 ms crossfade on HRTF pair change
  → Stereo output
```

Layout modes (`layoutMode` param 0–5):
| Mode | Description |
|------|-------------|
| 0 | Direct single HRTF (mirror axis) |
| 1 | VBAP 5-speaker hybrid |
| 2 | VBAP 7-speaker hybrid |
| 3 | VBAP 9-speaker hybrid |
| 4 | VBAP 12-speaker hybrid |
| 5 | VBAP 18-speaker hybrid |

Topology param selects cinema vs symmetric speaker arrangements.

---

## Test Session Framework (Common/TestSession.h)

Screen state machine: **Setup → Trial → Feedback → Summary**

- Researcher mode: configure session, angles, layout chips, audio file
- Subject mode: simplified start screen
- Stores `TrialResult` per trial: target angle, response angle, angular error, confidence (1–5), timestamp
- Summary stats: mean/best/worst error per configuration
- Exports to CSV (Excel-compatible)

---

## Key Parameters

**BinauralPlugin:**
- `inputGain` (0.0–1.0)
- `sourceAzimuth` (0–360°)
- `layoutMode` (0–5)
- `topology` (0–1)

**VBAPPlugin:**
- `inputGain`, `sourceAzimuth`, `speakerCount` (2–8)
- `speakerAz1`–`speakerAz8` (individual speaker angles)

---

## Important Notes

- `HrirBank::loadFromFolder()` expects exactly 360 integer-degree files; if any are missing the bank fails to load silently
- The Unity copy path is hardcoded — building on another machine requires updating `UNITY_PLUGIN_COPY_DIR` in CMakeLists.txt
- `AudioPlayer` is downmix-to-mono only; stereo stimulus files lose their right channel
- VBAP engine is header-only (`VbapEngine2D.h`) and shared between both plugins
- Do not edit code without explicit user confirmation (per project workflow)
