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
