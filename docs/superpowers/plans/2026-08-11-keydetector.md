# KeyDetector Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a free cross-platform (macOS + Windows) audio-effect plugin (AU + VST3) that detects the musical key of incoming audio in real time and displays it.

**Architecture:** A single C++/JUCE project built with CMake (JUCE fetched via `FetchContent`). A pure, host-free DSP core (`KeyAnalyzer`) turns audio into a key estimate using the Krumhansl-Schmuckler algorithm; a JUCE `AudioProcessor` passes audio through untouched, feeds a mono copy to the analyzer, and publishes the result to a minimal editor via atomics.

**Tech Stack:** C++17, JUCE 8, CMake ≥ 3.22, Catch2 v3 (tests), `pluginval` (host-contract validation), GitHub Actions (CI), `pkgbuild`/`productbuild` (macOS installer), Inno Setup (Windows installer).

## Global Constraints

- Formats: **AU + VST3** only. No AAX/Pro Tools in v1.
- Platforms: **macOS 11+ universal (arm64 + x86_64)**, **Windows x64**.
- Plugin is an **effect** that passes audio through **unmodified** — it must never alter the output buffer.
- **Key detection only** — no BPM, no Camelot, no file/batch analysis, no MIDI, no export, no presets, no automatable parameters, no standalone app.
- DSP core (`KeyAnalyzer`, `KeyProfiles`, `Key`) must have **no host/UI dependencies** so it is unit-testable without a DAW. It may depend on `juce_dsp` and `juce_core` only.
- Pitch-class convention: `0 = C, 1 = C#, … 9 = A, … 11 = B`.
- Keep it lean — do not add features, files, or dependencies beyond what a task requires.

---

## File Structure

```
CMakeLists.txt                     # root: JUCE fetch, plugin target, tests toggle
source/
  Key.h / Key.cpp                  # Key struct + keyToString()
  KeyProfiles.h / KeyProfiles.cpp  # 24 Krumhansl-Schmuckler profiles (major+minor)
  KeyAnalyzer.h / KeyAnalyzer.cpp  # pure DSP core: audio -> {key, confidence}
  PluginProcessor.h / .cpp         # passthrough + analyzer wiring + atomic hand-off
  PluginEditor.h / .cpp            # minimal UI: key label, confidence bar, reset
tests/
  CMakeLists.txt                   # Catch2 test target
  KeyTests.cpp                     # keyToString tests
  KeyAnalyzerTests.cpp             # DSP detection tests (synthesized triads)
packaging/
  macos/build_pkg.sh               # builds signed/notarized .pkg
  windows/installer.iss            # Inno Setup script
.github/workflows/build.yml        # CI: build + test both platforms, upload artifacts
```

---

### Task 1: CMake scaffold, test harness, and Key type

Establishes the build + test pipeline end-to-end using the smallest real unit (`Key`), so every later task has a green pipeline to build on.

**Files:**
- Create: `CMakeLists.txt`
- Create: `source/Key.h`
- Create: `source/Key.cpp`
- Create: `tests/CMakeLists.txt`
- Create: `tests/KeyTests.cpp`
- Create: `.gitignore`

**Interfaces:**
- Consumes: nothing.
- Produces:
  - `enum class Mode { Major, Minor };`
  - `struct Key { int tonic = 0; Mode mode = Mode::Major; bool valid = false; };`
  - `juce::String keyToString (const Key& key);` — returns `"C major"`, `"A minor"`, or `"—"` (em dash) when `!valid`.

- [ ] **Step 1: Create `.gitignore`**

```gitignore
build/
.DS_Store
```

- [ ] **Step 2: Write the root `CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.22)
project(KeyDetector VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)
endif()

include(FetchContent)
FetchContent_Declare(
  JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.4
)
FetchContent_MakeAvailable(JUCE)

option(KEYDETECTOR_BUILD_TESTS "Build unit tests" ON)
if(KEYDETECTOR_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
```

- [ ] **Step 3: Write `source/Key.h`**

```cpp
#pragma once
#include <juce_core/juce_core.h>

enum class Mode { Major, Minor };

struct Key
{
    int  tonic = 0;              // 0 = C, 1 = C#, ... 11 = B
    Mode mode  = Mode::Major;
    bool valid = false;
};

juce::String keyToString (const Key& key);
```

- [ ] **Step 4: Write the failing test `tests/KeyTests.cpp`**

```cpp
#include <catch2/catch_test_macros.hpp>
#include "Key.h"

TEST_CASE ("keyToString formats major and minor keys")
{
    REQUIRE (keyToString (Key { 0, Mode::Major, true }) == juce::String ("C major"));
    REQUIRE (keyToString (Key { 9, Mode::Minor, true }) == juce::String ("A minor"));
}

TEST_CASE ("keyToString shows a dash when invalid")
{
    REQUIRE (keyToString (Key { 0, Mode::Major, false }) == juce::String (juce::CharPointer_UTF8 ("\xe2\x80\x94")));
}
```

- [ ] **Step 5: Write `tests/CMakeLists.txt`**

```cmake
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.2
)
FetchContent_MakeAvailable(Catch2)

add_executable(KeyDetectorTests
  KeyTests.cpp
  ${CMAKE_SOURCE_DIR}/source/Key.cpp
)

target_include_directories(KeyDetectorTests PRIVATE ${CMAKE_SOURCE_DIR}/source)

target_link_libraries(KeyDetectorTests PRIVATE
  Catch2::Catch2WithMain
  juce::juce_core
  juce::juce_recommended_config_flags
)

target_compile_definitions(KeyDetectorTests PRIVATE
  JUCE_STANDALONE_APPLICATION=1
  JUCE_USE_CURL=0
  JUCE_WEB_BROWSER=0
)

include(Catch)
catch_discover_tests(KeyDetectorTests)
```

- [ ] **Step 6: Run the test to verify it fails (no implementation yet)**

Run:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target KeyDetectorTests
```
Expected: FAIL — link error, `keyToString` is undefined (declared in `Key.h`, not implemented).

- [ ] **Step 7: Write `source/Key.cpp`**

```cpp
#include "Key.h"

juce::String keyToString (const Key& key)
{
    if (! key.valid)
        return juce::String (juce::CharPointer_UTF8 ("\xe2\x80\x94")); // em dash

    static const char* const names[] =
        { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

    return juce::String (names[key.tonic % 12])
         + (key.mode == Mode::Major ? " major" : " minor");
}
```

- [ ] **Step 8: Build and run the tests — verify they pass**

Run:
```bash
cmake --build build --target KeyDetectorTests
ctest --test-dir build --output-on-failure
```
Expected: PASS — all `KeyDetectorTests` assertions green.

- [ ] **Step 9: Commit**

```bash
git add CMakeLists.txt .gitignore source/Key.h source/Key.cpp tests/
git commit -m "feat: CMake+Catch2 scaffold and Key type"
```

---

### Task 2: KeyAnalyzer DSP core

The heart of the plugin: a pure, host-free class that accumulates audio into a chroma histogram and reports the best-matching key via Krumhansl-Schmuckler correlation. Tested with synthesized triads (deterministic, no binary fixtures).

**Files:**
- Create: `source/KeyProfiles.h`
- Create: `source/KeyProfiles.cpp`
- Create: `source/KeyAnalyzer.h`
- Create: `source/KeyAnalyzer.cpp`
- Create: `tests/KeyAnalyzerTests.cpp`
- Modify: `tests/CMakeLists.txt` (add the new sources + `juce_dsp`)

**Interfaces:**
- Consumes: `Key`, `Mode` from Task 1.
- Produces:
  - `namespace KeyProfiles { extern const std::array<float, 12> majorProfile; extern const std::array<float, 12> minorProfile; }`
  - Class `KeyAnalyzer` with:
    - `void prepare (double sampleRate);`
    - `void reset();`
    - `void processBlock (const float* mono, int numSamples);`
    - `struct Result { Key key; float confidence = 0.0f; };`
    - `Result detect() const;`

- [ ] **Step 1: Write `source/KeyProfiles.h`**

```cpp
#pragma once
#include <array>

// Krumhansl-Schmuckler key profiles, indexed by scale degree relative to the
// tonic (index 0 = tonic). Rotate across 12 tonics to build all 24 keys.
namespace KeyProfiles
{
    extern const std::array<float, 12> majorProfile;
    extern const std::array<float, 12> minorProfile;
}
```

- [ ] **Step 2: Write `source/KeyProfiles.cpp`**

```cpp
#include "KeyProfiles.h"

namespace KeyProfiles
{
    const std::array<float, 12> majorProfile =
        { 6.35f, 2.23f, 3.48f, 2.33f, 4.38f, 4.09f, 2.52f, 5.19f, 2.39f, 3.66f, 2.29f, 2.88f };

    const std::array<float, 12> minorProfile =
        { 6.33f, 2.68f, 3.52f, 5.38f, 2.60f, 3.53f, 2.54f, 4.75f, 3.98f, 2.69f, 3.34f, 3.17f };
}
```

- [ ] **Step 3: Write `source/KeyAnalyzer.h`**

```cpp
#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include "Key.h"

// Pure DSP core: feed mono audio blocks, read the detected key. No host/UI deps.
class KeyAnalyzer
{
public:
    KeyAnalyzer() = default;

    void prepare (double sampleRate);
    void reset();
    void processBlock (const float* mono, int numSamples);

    struct Result { Key key; float confidence = 0.0f; };
    Result detect() const;

private:
    void analyseFrame();

    static constexpr int fftOrder = 14;          // 2^14 = 16384
    static constexpr int fftSize  = 1 << fftOrder;

    double sampleRate = 44100.0;

    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window
        { (size_t) fftSize, juce::dsp::WindowingFunction<float>::hann };

    std::array<float, (size_t) fftSize * 2> fftBuffer {}; // time-domain in, magnitudes out
    int fillCount = 0;

    std::array<double, 12> chroma {};   // accumulating pitch-class histogram
    double totalEnergy = 0.0;
};
```

- [ ] **Step 4: Write the failing tests `tests/KeyAnalyzerTests.cpp`**

```cpp
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>
#include "KeyAnalyzer.h"

namespace
{
    constexpr double kSampleRate = 44100.0;

    double midiToFreq (int midi)
    {
        return 440.0 * std::pow (2.0, (midi - 69) / 12.0);
    }

    // Build a signal that sustains a chord (given MIDI notes) for `frames`
    // full FFT frames, so at least one frame is analysed.
    std::vector<float> makeChord (const std::vector<int>& midiNotes, int frames)
    {
        const int fftSize = 1 << 14;
        std::vector<float> out ((size_t) fftSize * frames, 0.0f);
        for (size_t n = 0; n < out.size(); ++n)
        {
            double s = 0.0;
            for (int note : midiNotes)
                s += std::sin (2.0 * juce::MathConstants<double>::pi
                               * midiToFreq (note) * (double) n / kSampleRate);
            out[n] = (float) (0.3 * s / (double) midiNotes.size());
        }
        return out;
    }
}

TEST_CASE ("KeyAnalyzer detects a C major triad as C major")
{
    KeyAnalyzer a;
    a.prepare (kSampleRate);
    auto sig = makeChord ({ 60, 64, 67 }, 4); // C4 E4 G4
    a.processBlock (sig.data(), (int) sig.size());

    auto r = a.detect();
    REQUIRE (r.key.valid);
    REQUIRE (r.key.tonic == 0);            // C
    REQUIRE (r.key.mode == Mode::Major);
    REQUIRE (r.confidence > 0.0f);
}

TEST_CASE ("KeyAnalyzer detects an A minor triad as A minor")
{
    KeyAnalyzer a;
    a.prepare (kSampleRate);
    auto sig = makeChord ({ 69, 72, 76 }, 4); // A4 C5 E5
    a.processBlock (sig.data(), (int) sig.size());

    auto r = a.detect();
    REQUIRE (r.key.valid);
    REQUIRE (r.key.tonic == 9);            // A
    REQUIRE (r.key.mode == Mode::Minor);
}

TEST_CASE ("KeyAnalyzer reports invalid on silence")
{
    KeyAnalyzer a;
    a.prepare (kSampleRate);
    std::vector<float> silence ((size_t) (1 << 14) * 2, 0.0f);
    a.processBlock (silence.data(), (int) silence.size());

    REQUIRE_FALSE (a.detect().key.valid);
}

TEST_CASE ("reset clears accumulated analysis")
{
    KeyAnalyzer a;
    a.prepare (kSampleRate);
    auto sig = makeChord ({ 60, 64, 67 }, 4);
    a.processBlock (sig.data(), (int) sig.size());
    a.reset();
    REQUIRE_FALSE (a.detect().key.valid);
}
```

- [ ] **Step 5: Add the new sources to `tests/CMakeLists.txt`**

Replace the `add_executable` and `target_link_libraries` blocks with:

```cmake
add_executable(KeyDetectorTests
  KeyTests.cpp
  KeyAnalyzerTests.cpp
  ${CMAKE_SOURCE_DIR}/source/Key.cpp
  ${CMAKE_SOURCE_DIR}/source/KeyProfiles.cpp
  ${CMAKE_SOURCE_DIR}/source/KeyAnalyzer.cpp
)

target_include_directories(KeyDetectorTests PRIVATE ${CMAKE_SOURCE_DIR}/source)

target_link_libraries(KeyDetectorTests PRIVATE
  Catch2::Catch2WithMain
  juce::juce_core
  juce::juce_dsp
  juce::juce_recommended_config_flags
)
```

- [ ] **Step 6: Run the tests to verify they fail**

Run:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target KeyDetectorTests
```
Expected: FAIL — link error, `KeyAnalyzer::prepare/processBlock/detect` undefined (header exists, `.cpp` does not yet).

- [ ] **Step 7: Write `source/KeyAnalyzer.cpp`**

```cpp
#include "KeyAnalyzer.h"
#include "KeyProfiles.h"
#include <algorithm>
#include <cmath>

namespace
{
    // Pearson correlation between a mean-centred 12-bin histogram and a profile
    // rotated so profile[p] aligns with pitch class (tonic + p) mod 12.
    double correlate (const std::array<double, 12>& centredHist,
                      const std::array<float, 12>& profile,
                      int tonic)
    {
        double pmean = 0.0;
        for (float v : profile) pmean += v;
        pmean /= 12.0;

        double num = 0.0, dh = 0.0, dp = 0.0;
        for (int p = 0; p < 12; ++p)
        {
            const int pc = (tonic + p) % 12;
            const double pv = profile[(size_t) p] - pmean;
            num += centredHist[(size_t) pc] * pv;
            dh  += centredHist[(size_t) pc] * centredHist[(size_t) pc];
            dp  += pv * pv;
        }
        if (dh <= 0.0 || dp <= 0.0) return -1.0;
        return num / std::sqrt (dh * dp);
    }
}

void KeyAnalyzer::prepare (double sr)
{
    sampleRate = sr;
    reset();
}

void KeyAnalyzer::reset()
{
    chroma.fill (0.0);
    fftBuffer.fill (0.0f);
    fillCount = 0;
    totalEnergy = 0.0;
}

void KeyAnalyzer::processBlock (const float* mono, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        fftBuffer[(size_t) fillCount++] = mono[i];
        if (fillCount == fftSize)
        {
            analyseFrame();
            fillCount = 0;
        }
    }
}

void KeyAnalyzer::analyseFrame()
{
    window.multiplyWithWindowingTable (fftBuffer.data(), (size_t) fftSize);
    fft.performFrequencyOnlyForwardTransform (fftBuffer.data());

    const int numBins = fftSize / 2;
    for (int bin = 1; bin < numBins; ++bin)
    {
        const double freq = bin * sampleRate / (double) fftSize;
        if (freq < 27.5 || freq > 5000.0) continue; // ~A0 .. musical upper bound

        const double mag  = (double) fftBuffer[(size_t) bin];
        const double midi = 69.0 + 12.0 * std::log2 (freq / 440.0);
        int pc = ((int) std::lround (midi)) % 12;
        if (pc < 0) pc += 12;

        chroma[(size_t) pc] += mag;
        totalEnergy += mag;
    }

    // Zero the whole buffer so the next frame starts clean (real FFT needs it).
    fftBuffer.fill (0.0f);
}

KeyAnalyzer::Result KeyAnalyzer::detect() const
{
    Result r;
    if (totalEnergy <= 0.0)
        return r; // key.valid stays false

    std::array<double, 12> centred {};
    double mean = 0.0;
    for (double v : chroma) mean += v;
    mean /= 12.0;
    for (int i = 0; i < 12; ++i) centred[(size_t) i] = chroma[(size_t) i] - mean;

    double best = -2.0, second = -2.0;
    for (int m = 0; m < 2; ++m)
    {
        const auto& profile = (m == 0) ? KeyProfiles::majorProfile
                                       : KeyProfiles::minorProfile;
        for (int tonic = 0; tonic < 12; ++tonic)
        {
            const double c = correlate (centred, profile, tonic);
            if (c > best)
            {
                second = best;
                best   = c;
                r.key  = Key { tonic, (m == 0 ? Mode::Major : Mode::Minor), true };
            }
            else if (c > second)
            {
                second = c;
            }
        }
    }

    r.confidence = (float) juce::jlimit (0.0, 1.0, best - second);
    return r;
}
```

- [ ] **Step 8: Build and run the tests — verify they pass**

Run:
```bash
cmake --build build --target KeyDetectorTests
ctest --test-dir build --output-on-failure
```
Expected: PASS — all four `KeyAnalyzer` cases green (C major, A minor, silence invalid, reset).

- [ ] **Step 9: Commit**

```bash
git add source/KeyProfiles.h source/KeyProfiles.cpp source/KeyAnalyzer.h source/KeyAnalyzer.cpp tests/KeyAnalyzerTests.cpp tests/CMakeLists.txt
git commit -m "feat: Krumhansl-Schmuckler key analyzer DSP core"
```

---

### Task 3: Plugin target — passthrough processor with analyzer wiring

Adds the JUCE plugin target and a processor that passes audio through untouched, feeds a mono sum to `KeyAnalyzer`, and publishes the result via atomics. Editor is a temporary generic one (replaced in Task 4) so the plugin builds and loads now.

**Files:**
- Modify: `CMakeLists.txt` (add `juce_add_plugin` target)
- Create: `source/PluginProcessor.h`
- Create: `source/PluginProcessor.cpp`

**Interfaces:**
- Consumes: `KeyAnalyzer`, `KeyAnalyzer::Result`, `Key`, `Mode`.
- Produces:
  - Class `KeyDetectorProcessor : juce::AudioProcessor` with public:
    - `void requestReset();` — thread-safe; clears the analyzer on the audio thread.
    - `KeyAnalyzer::Result getLatestResult() const;` — thread-safe snapshot for the UI.
  - `juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();`

- [ ] **Step 1: Add the plugin target to the root `CMakeLists.txt`**

Insert immediately after `FetchContent_MakeAvailable(JUCE)` and before the `option(KEYDETECTOR_BUILD_TESTS ...)` block:

```cmake
juce_add_plugin(KeyDetector
  PRODUCT_NAME "KeyDetector"
  COMPANY_NAME "KeyDetector"
  BUNDLE_ID "com.keydetector.keydetector"
  PLUGIN_MANUFACTURER_CODE Kdet
  PLUGIN_CODE Kdt1
  FORMATS AU VST3
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  COPY_PLUGIN_AFTER_BUILD TRUE
)

target_sources(KeyDetector PRIVATE
  source/PluginProcessor.cpp
  source/Key.cpp
  source/KeyProfiles.cpp
  source/KeyAnalyzer.cpp
)

target_compile_definitions(KeyDetector PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(KeyDetector PRIVATE
  juce::juce_audio_utils
  juce::juce_dsp
  juce::juce_recommended_config_flags
  juce::juce_recommended_warning_flags
)
```

- [ ] **Step 2: Write `source/PluginProcessor.h`**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "KeyAnalyzer.h"

class KeyDetectorProcessor : public juce::AudioProcessor
{
public:
    KeyDetectorProcessor();
    ~KeyDetectorProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "KeyDetector"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

    // UI hand-off (thread-safe)
    void requestReset() { resetRequested.store (true); }
    KeyAnalyzer::Result getLatestResult() const;

private:
    KeyAnalyzer analyzer;
    juce::AudioBuffer<float> monoBuffer;

    std::atomic<int>   encodedKey   { -1 };    // -1 invalid; 0..11 major; 12..23 minor
    std::atomic<float> confidence   { 0.0f };
    std::atomic<bool>  resetRequested { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeyDetectorProcessor)
};
```

- [ ] **Step 3: Write `source/PluginProcessor.cpp`**

```cpp
#include "PluginProcessor.h"

KeyDetectorProcessor::KeyDetectorProcessor()
    : juce::AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

void KeyDetectorProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    analyzer.prepare (sampleRate);
    monoBuffer.setSize (1, samplesPerBlock);
}

bool KeyDetectorProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& main = layouts.getMainOutputChannelSet();
    if (main != juce::AudioChannelSet::mono()
        && main != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == main;
}

void KeyDetectorProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    if (resetRequested.exchange (false))
        analyzer.reset();

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Build a mono sum WITHOUT touching the passthrough buffer.
    monoBuffer.setSize (1, numSamples, false, false, true);
    monoBuffer.clear();
    for (int ch = 0; ch < numChannels; ++ch)
        monoBuffer.addFrom (0, 0, buffer, ch, 0, numSamples);
    if (numChannels > 0)
        monoBuffer.applyGain (1.0f / (float) numChannels);

    analyzer.processBlock (monoBuffer.getReadPointer (0), numSamples);

    const auto r = analyzer.detect();
    encodedKey.store (r.key.valid
        ? (r.key.tonic + (r.key.mode == Mode::Major ? 0 : 12))
        : -1);
    confidence.store (r.confidence);

    // Audio passes through unmodified — buffer is never written.
}

KeyAnalyzer::Result KeyDetectorProcessor::getLatestResult() const
{
    KeyAnalyzer::Result r;
    const int e = encodedKey.load();
    if (e >= 0)
    {
        r.key.valid = true;
        r.key.tonic = e % 12;
        r.key.mode  = (e < 12) ? Mode::Major : Mode::Minor;
    }
    r.confidence = confidence.load();
    return r;
}

juce::AudioProcessorEditor* KeyDetectorProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this); // temporary; replaced in Task 4
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KeyDetectorProcessor();
}
```

- [ ] **Step 4: Build the AU and VST3 targets — verify they compile and link**

Run:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target KeyDetector_AU KeyDetector_VST3
```
Expected: PASS — both `KeyDetector_artefacts/Release/AU/KeyDetector.component` and `.../VST3/KeyDetector.vst3` are produced.

- [ ] **Step 5: Confirm the DSP tests still pass**

Run:
```bash
cmake --build build --target KeyDetectorTests
ctest --test-dir build --output-on-failure
```
Expected: PASS — Task 1 and Task 2 tests remain green.

- [ ] **Step 6: Commit**

```bash
git add CMakeLists.txt source/PluginProcessor.h source/PluginProcessor.cpp
git commit -m "feat: passthrough plugin target with analyzer wiring"
```

---

### Task 4: Editor UI — key label, confidence bar, reset

Replaces the generic editor with a minimal custom UI that polls the processor on a timer and draws the key, a confidence bar, and a Reset button.

**Files:**
- Create: `source/PluginEditor.h`
- Create: `source/PluginEditor.cpp`
- Modify: `source/PluginProcessor.cpp` (`createEditor` returns the new editor)
- Modify: `CMakeLists.txt` (add `source/PluginEditor.cpp` to `target_sources`)

**Interfaces:**
- Consumes: `KeyDetectorProcessor::getLatestResult()`, `KeyDetectorProcessor::requestReset()`, `keyToString`.
- Produces: `class KeyDetectorEditor : juce::AudioProcessorEditor`.

- [ ] **Step 1: Add the editor source to `target_sources(KeyDetector ...)` in the root `CMakeLists.txt`**

```cmake
target_sources(KeyDetector PRIVATE
  source/PluginProcessor.cpp
  source/PluginEditor.cpp
  source/Key.cpp
  source/KeyProfiles.cpp
  source/KeyAnalyzer.cpp
)
```

- [ ] **Step 2: Write `source/PluginEditor.h`**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "Key.h"

class KeyDetectorEditor : public juce::AudioProcessorEditor,
                          private juce::Timer
{
public:
    explicit KeyDetectorEditor (KeyDetectorProcessor&);
    ~KeyDetectorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    KeyDetectorProcessor& processorRef;
    juce::TextButton resetButton { "Reset" };
    KeyAnalyzer::Result current;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeyDetectorEditor)
};
```

- [ ] **Step 3: Write `source/PluginEditor.cpp`**

```cpp
#include "PluginEditor.h"

KeyDetectorEditor::KeyDetectorEditor (KeyDetectorProcessor& p)
    : juce::AudioProcessorEditor (&p), processorRef (p)
{
    addAndMakeVisible (resetButton);
    resetButton.onClick = [this] { processorRef.requestReset(); };

    setSize (300, 200);
    startTimerHz (20);
}

KeyDetectorEditor::~KeyDetectorEditor()
{
    stopTimer();
}

void KeyDetectorEditor::timerCallback()
{
    current = processorRef.getLatestResult();
    repaint();
}

void KeyDetectorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1e1e));

    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (44.0f, juce::Font::bold));
    g.drawText (keyToString (current.key),
                getLocalBounds().removeFromTop (110),
                juce::Justification::centred);

    auto bar = getLocalBounds().reduced (30, 0)
                   .withTop (120).withHeight (16);

    g.setColour (juce::Colours::darkgrey);
    g.fillRect (bar);
    g.setColour (juce::Colours::limegreen);
    g.fillRect (bar.withWidth (
        (int) (bar.getWidth() * juce::jlimit (0.0f, 1.0f, current.confidence))));

    g.setColour (juce::Colours::grey);
    g.setFont (12.0f);
    g.drawText ("confidence", bar.translated (0, 20),
                juce::Justification::centred);
}

void KeyDetectorEditor::resized()
{
    resetButton.setBounds (getLocalBounds()
        .removeFromBottom (44).reduced (100, 8));
}
```

- [ ] **Step 4: Point `createEditor` at the new editor in `source/PluginProcessor.cpp`**

Add the include near the top of the file:

```cpp
#include "PluginProcessor.h"
#include "PluginEditor.h"
```

Replace the body of `createEditor`:

```cpp
juce::AudioProcessorEditor* KeyDetectorProcessor::createEditor()
{
    return new KeyDetectorEditor (*this);
}
```

- [ ] **Step 5: Build and verify the plugin compiles with the new editor**

Run:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target KeyDetector_AU KeyDetector_VST3
```
Expected: PASS — both plugin artefacts build with the custom editor.

- [ ] **Step 6: Manual smoke test (record the result)**

Load `KeyDetector.component` (Logic) or `KeyDetector.vst3` (Reaper/Ableton) on a track, play a song with a clear key, and confirm the window shows a key that settles and a confidence bar that rises. Toggle **Reset** and confirm the display re-analyzes.

Expected: key label updates within a few seconds; audio is unaffected (bypass vs. active sound identical).

- [ ] **Step 7: Commit**

```bash
git add CMakeLists.txt source/PluginEditor.h source/PluginEditor.cpp source/PluginProcessor.cpp
git commit -m "feat: minimal editor with key label, confidence bar, reset"
```

---

### Task 5: pluginval validation + CI

Gates the plugin against host-contract issues with `pluginval` and wires a GitHub Actions pipeline that builds + tests both platforms and uploads artifacts.

**Files:**
- Create: `.github/workflows/build.yml`

**Interfaces:**
- Consumes: the built AU/VST3 artefacts and the `ctest` suite.
- Produces: CI artifacts `keydetector-macos` and `keydetector-windows`.

- [ ] **Step 1: Run `pluginval` locally against the VST3 (strictness 10)**

Run (macOS example; download the matching `pluginval` binary for your OS from https://github.com/Tracktion/pluginval/releases):
```bash
pluginval --strictness-level 10 --validate \
  "build/KeyDetector_artefacts/Release/VST3/KeyDetector.vst3"
```
Expected: `ALL TESTS PASSED`. If it flags a contract issue, fix it in `PluginProcessor` before proceeding (do not suppress).

- [ ] **Step 2: Write `.github/workflows/build.yml`**

```yaml
name: build

on:
  push:
    tags: ['v*']
  pull_request:
  workflow_dispatch:

jobs:
  build:
    strategy:
      fail-fast: false
      matrix:
        include:
          - os: macos-14
            name: macos
          - os: windows-latest
            name: windows
    runs-on: ${{ matrix.os }}
    steps:
      - uses: actions/checkout@v4

      - name: Configure
        run: cmake -B build -DCMAKE_BUILD_TYPE=Release

      - name: Build plugin
        run: cmake --build build --config Release --target KeyDetector_All

      - name: Build tests
        run: cmake --build build --config Release --target KeyDetectorTests

      - name: Run tests
        run: ctest --test-dir build -C Release --output-on-failure

      - name: Upload artifacts
        uses: actions/upload-artifact@v4
        with:
          name: keydetector-${{ matrix.name }}
          path: |
            build/KeyDetector_artefacts/Release/VST3/*.vst3
            build/KeyDetector_artefacts/Release/AU/*.component
          if-no-files-found: ignore
```

- [ ] **Step 3: Verify the CI config locally by running its build/test commands**

Run:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target KeyDetector_All KeyDetectorTests
ctest --test-dir build -C Release --output-on-failure
```
Expected: PASS — plugin and tests build; `ctest` green. (`KeyDetector_All` is the aggregate target JUCE generates for all formats.)

- [ ] **Step 4: Commit and push, then confirm the Actions run is green**

```bash
git add .github/workflows/build.yml
git commit -m "ci: build + test macOS and Windows, upload plugin artifacts"
git push
```
Then check the run:
```bash
gh run watch
```
Expected: both `build (macos)` and `build (windows)` jobs succeed.

---

### Task 6: Installers — macOS .pkg and Windows Inno Setup

Produces double-click installers so users can install without touching plugin folders.

**Files:**
- Create: `packaging/macos/build_pkg.sh`
- Create: `packaging/windows/installer.iss`

**Interfaces:**
- Consumes: built `KeyDetector.component`, `KeyDetector.vst3` (macOS) and `KeyDetector.vst3` (Windows).
- Produces: `KeyDetector-macOS.pkg` and `KeyDetector-Windows-Setup.exe`.

- [ ] **Step 1: Write `packaging/macos/build_pkg.sh`**

```bash
#!/usr/bin/env bash
set -euo pipefail

# Builds a macOS installer that drops the AU + VST3 into the standard
# system plugin folders. Run from the repo root after a Release build.
#
# Optional signing/notarization env vars (leave unset for an unsigned pkg):
#   INSTALLER_SIGN_ID  e.g. "Developer ID Installer: Your Name (TEAMID)"

ART="build/KeyDetector_artefacts/Release"
STAGE="$(mktemp -d)"
VERSION="${1:-0.1.0}"

mkdir -p "$STAGE/Library/Audio/Plug-Ins/Components"
mkdir -p "$STAGE/Library/Audio/Plug-Ins/VST3"

cp -R "$ART/AU/KeyDetector.component"  "$STAGE/Library/Audio/Plug-Ins/Components/"
cp -R "$ART/VST3/KeyDetector.vst3"     "$STAGE/Library/Audio/Plug-Ins/VST3/"

pkgbuild \
  --root "$STAGE" \
  --identifier "com.keydetector.keydetector" \
  --version "$VERSION" \
  --install-location "/" \
  "KeyDetector-macOS.pkg"

if [[ -n "${INSTALLER_SIGN_ID:-}" ]]; then
  productsign --sign "$INSTALLER_SIGN_ID" \
    "KeyDetector-macOS.pkg" "KeyDetector-macOS-signed.pkg"
  mv "KeyDetector-macOS-signed.pkg" "KeyDetector-macOS.pkg"
  echo "Signed. Notarize with: xcrun notarytool submit KeyDetector-macOS.pkg --keychain-profile <profile> --wait"
fi

rm -rf "$STAGE"
echo "Built KeyDetector-macOS.pkg (version $VERSION)"
```

- [ ] **Step 2: Make it executable and build the pkg (macOS)**

Run:
```bash
chmod +x packaging/macos/build_pkg.sh
cmake --build build --config Release --target KeyDetector_All
./packaging/macos/build_pkg.sh 0.1.0
```
Expected: `KeyDetector-macOS.pkg` is produced. Double-clicking it installs to `/Library/Audio/Plug-Ins/...` (unsigned build shows a Gatekeeper prompt — expected until notarization is configured).

- [ ] **Step 3: Write `packaging/windows/installer.iss`**

```iss
; Inno Setup script for KeyDetector (VST3, Windows x64).
; Build the plugin in Release first, then compile this with Inno Setup 6:
;   ISCC.exe packaging\windows\installer.iss

#define MyAppName "KeyDetector"
#define MyAppVersion "0.1.0"

[Setup]
AppName={#MyAppName}
AppVersion={#MyAppVersion}
DefaultDirName={autopf}\Common Files\VST3
DisableDirPage=yes
ArchitecturesInstallIn64BitMode=x64
OutputBaseFilename=KeyDetector-Windows-Setup
Compression=lzma2
SolidCompression=yes
PrivilegesRequired=admin

[Files]
Source: "..\..\build\KeyDetector_artefacts\Release\VST3\KeyDetector.vst3\*"; \
  DestDir: "{commoncf64}\VST3\KeyDetector.vst3"; \
  Flags: recursesubdirs createallsubdirs ignoreversion

[Run]
```

- [ ] **Step 4: Build the Windows installer (on Windows with Inno Setup 6 installed)**

Run:
```bat
cmake --build build --config Release --target KeyDetector_VST3
ISCC.exe packaging\windows\installer.iss
```
Expected: `packaging\windows\Output\KeyDetector-Windows-Setup.exe` is produced and installs the VST3 into `C:\Program Files\Common Files\VST3\KeyDetector.vst3`.

- [ ] **Step 5: Commit**

```bash
git add packaging/
git commit -m "build: macOS pkg and Windows Inno Setup installers"
```

---

## Notes for the implementer

- **JUCE version:** pinned to `8.0.4`. If FetchContent is slow on first configure, that's JUCE cloning once into `build/_deps` — subsequent configures are cached.
- **Deprecation warnings** from `juce::Font (float, style)` in the editor are harmless; leave them unless `juce_recommended_warning_flags` is set to treat warnings as errors (it is not, by default).
- **Do not** add parameters, state serialization, or extra formats to hit v1 — they are explicitly out of scope.
- If `pluginval` or a host rejects the mono/stereo bus arrangement, the fix belongs in `isBusesLayoutSupported` — do not widen scope elsewhere.
