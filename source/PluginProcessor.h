// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 dkribeiro

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
