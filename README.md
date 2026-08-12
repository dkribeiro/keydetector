# KeyDetector

**Free real-time musical key detector (AU / VST3) for Logic Pro, Ableton Live, FL Studio, Cubase, Studio One, Bitwig & Reaper.**

KeyDetector is a lightweight audio-effect plugin that detects the musical key of
incoming audio in real time and displays it. Drop it on a track or bus — it
passes audio through untouched and shows the detected key while the track plays.

- 🎛️ **Formats:** AU + VST3 — macOS (universal: Apple Silicon + Intel) and Windows (x64)
- ⚡ **Real-time:** continuous key detection with a confidence meter
- 🎹 **Key-only, no clutter:** large key label, confidence bar, and a Reset button — nothing else
- 🆓 **Free & open source** (MIT)

## Install

### macOS
Download `KeyDetector-macOS.pkg` from the [latest release](../../releases) and
double-click it. It installs the AU (`.component`) and VST3 into the standard
system plugin folders. Rescan plugins in your DAW.

### Windows
Download `KeyDetector-Windows-Setup.exe` from the [latest release](../../releases)
and run it. It installs the VST3 into `C:\Program Files\Common Files\VST3`.

## Usage

Insert KeyDetector on any track or bus as an audio effect. Play the audio — the
detected key (e.g. `A minor`) appears and stabilizes over a few seconds as the
analyzer accumulates signal. Hit **Reset** to clear the analysis and re-detect a
new section. KeyDetector never alters your audio; it only listens.

## How it works

KeyDetector uses the **Krumhansl-Schmuckler** key-finding algorithm:

1. A 16384-point FFT turns each audio frame into a spectrum.
2. Spectral energy is folded into a 12-bin **chroma** histogram (one bin per
   pitch class) that accumulates over time.
3. The histogram is Pearson-correlated against all 24 key profiles (12 major +
   12 minor).
4. The best-correlating key wins; confidence is the margin over the runner-up.

## Build from source

Requires CMake ≥ 3.22 and a C++17 toolchain (Xcode on macOS, MSVC on Windows).
JUCE 8 and Catch2 are fetched automatically by CMake.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target KeyDetector_All
# run the unit tests
ctest --test-dir build -C Release --output-on-failure
```

Build artefacts land under `build/KeyDetector_artefacts/Release/`.

### Packaging

- macOS: `./packaging/macos/build_pkg.sh 0.1.0` produces `KeyDetector-macOS.pkg`.
- Windows: compile `packaging/windows/installer.iss` with Inno Setup 6 (`ISCC.exe`).

## Compatibility

Tested/targeted for Logic Pro (AU), Ableton Live, FL Studio, Cubase, Studio One,
Bitwig, and Reaper (VST3). Pro Tools (AAX) is not supported.

## License

[MIT](LICENSE). *Logic Pro, Ableton Live, FL Studio, Cubase, Studio One, Bitwig,
Reaper, and VST are trademarks of their respective owners; KeyDetector is not
affiliated with or endorsed by them.*
