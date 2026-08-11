# KeyDetector — Design Spec (v1)

**Date:** 2026-08-11
**Status:** Approved for planning

## Summary

A free, lightweight audio-effect plugin that detects the musical key of
incoming audio in real time and displays it. It passes audio through
unaltered and shows the detected key while a track plays. Built once in
C++/JUCE, it ships as **AU** and **VST3** for **macOS and Windows**, with
simple double-click installers.

Inspired by [ifeelvoid/keyfinder](https://github.com/ifeelvoid/keyfinder)
(MIT). That project is a Swift/SwiftUI macOS app plus a JUCE plugin; its
Swift DSP engine is not directly reusable in a cross-platform C++ plugin,
so we reimplement its key-detection *method* (Krumhansl-Schmuckler) in C++.

## Goals

- Real-time musical key detection displayed inside the DAW.
- Works in the major DAWs on macOS and Windows.
- Trivial to install (double-click) and free.
- Minimal footprint — no features beyond key detection.

## Non-Goals (explicitly out of scope for v1)

- BPM / tempo detection.
- Camelot wheel notation.
- Batch / audio-file analysis (this is a real-time insert plugin only).
- DJ-software export (Rekordbox, Serato, Traktor, etc.).
- MIDI output.
- Standalone application.
- **Pro Tools / AAX** — requires an Avid developer account, the AAX SDK,
  and mandatory code signing. Noted as a possible future add.

## Platforms & Formats

| Platform | Formats | Notes |
|----------|---------|-------|
| macOS    | AU (`.component`), VST3 (`.vst3`) | Universal binary (Apple Silicon + Intel), macOS 11+ |
| Windows  | VST3 (`.vst3`) | x64 |

Target DAWs: Logic Pro (AU), Ableton Live, FL Studio, Cubase, Studio One,
Bitwig, Reaper (VST3).

## Architecture

Single C++/JUCE project built with CMake (JUCE fetched via CMake so there
is no vendored copy to maintain).

```
audio in ──┬─► out (unmodified passthrough)
           │
           └─► Analyzer (DSP core)
                 FFT (per block)
                 → fold spectrum into 12 chroma bins
                 → accumulate into rolling chroma histogram
                 → correlate vs 24 Krumhansl-Schmuckler profiles
                 → best key + confidence ──► UI
```

### Components

1. **`KeyAnalyzer` (pure DSP core, no JUCE UI/host deps)**
   - Input: mono audio blocks (stereo is summed to mono before analysis).
   - Uses `juce::dsp::FFT` for the transform; everything else is plain
     C++ so it can be unit-tested without a host.
   - Maintains an accumulating chroma histogram (12 pitch classes).
   - `detect()` → `{ Key key, float confidence }`.
   - `reset()` → clears the histogram.
   - **What it does:** turns accumulated audio into a best-guess key +
     confidence. **How you use it:** feed blocks, read `detect()`, call
     `reset()` to start a new section. **Depends on:** JUCE FFT only.

2. **`KeyProfiles` (data)**
   - The 24 Krumhansl-Schmuckler weighting profiles (12 major + 12 minor),
     stored as constants. Major and minor base profiles rotated across the
     12 tonics.

3. **`PluginProcessor` (JUCE `AudioProcessor`)**
   - Passthrough of the audio buffer, unchanged.
   - Feeds a mono copy of each block to `KeyAnalyzer`.
   - Exposes current detection to the editor (lock-free/atomic hand-off).

4. **`PluginEditor` (JUCE UI)**
   - Large key label (e.g. `A minor`).
   - Confidence bar.
   - **Reset** button → clears the analyzer histogram.
   - No presets, no parameters exposed to automation.

### Key-detection method

1. Per audio block, compute an FFT (window size ~16384 for frequency
   resolution; block-buffered as needed).
2. Map spectral energy to 12 chroma bins by folding each frequency bin to
   its pitch class (log-frequency → pitch class), summing magnitudes.
3. Add the block's chroma to an accumulating histogram so the estimate
   stabilizes over a few seconds of audio.
4. Pearson-correlate the normalized histogram against all 24 profiles.
5. Detected key = highest correlation. **Confidence** = margin between the
   top-1 and top-2 correlation scores (normalized to 0–1).

## Data Flow / Threading

- Audio thread: passthrough + push chroma into the histogram. No locks; the
  histogram lives on the audio thread and publishes a small result struct
  to the UI via an atomic snapshot.
- UI thread: polls the atomic snapshot on a timer (~15–30 Hz) and repaints.
- **Reset** sets a flag consumed on the audio thread to clear the histogram.

## Error Handling

- No audio / silence: confidence stays low; UI shows `—` until enough
  signal is accumulated (below an energy threshold → no update).
- Sample-rate / block-size changes: `prepareToPlay` re-initializes the FFT
  and clears the histogram.
- Mono/stereo/multichannel: sum to mono before analysis; never alter the
  passthrough channels.

## Testing

- **DSP core unit tests** (host-free): synthesize or load a handful of
  known-key WAV fixtures (a few major and minor loops), run them through
  `KeyAnalyzer`, assert the detected key matches. This is the primary
  correctness gate.
- **Confidence sanity:** noise/silence yields low confidence; a strong
  tonal loop yields high confidence.
- **Plugin validation:** run `pluginval` against the built AU/VST3 to catch
  host-contract issues.

## Packaging / Install

- **macOS:** a signed + notarized `.pkg` that installs the `.component`
  (AU) and `.vst3` into the standard system folders. Double-click to
  install.
- **Windows:** an Inno Setup installer that places the `.vst3` into
  `C:\Program Files\Common Files\VST3`. Double-click to install.
- **CI:** GitHub Actions builds macOS (universal) and Windows (x64) and
  produces both installers as release artifacts on a tagged build.

## Build Tooling

- CMake + JUCE (JUCE pulled via CMake `FetchContent`).
- From-scratch setup assumed: the implementation plan will include
  toolchain steps (Xcode command-line tools / MSVC, CMake) and initial
  project scaffolding.

## Open Questions

None blocking. Code-signing identities (Apple Developer ID for
notarization; optional Windows signing cert) are operational details
handled during the packaging step, not design decisions.
