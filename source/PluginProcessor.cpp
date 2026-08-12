// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 dkribeiro

#include "PluginProcessor.h"
#include "PluginEditor.h"

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
    return new KeyDetectorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KeyDetectorProcessor();
}
