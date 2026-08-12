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
