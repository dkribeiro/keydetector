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
