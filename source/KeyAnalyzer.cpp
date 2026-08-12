// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 dkribeiro

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
