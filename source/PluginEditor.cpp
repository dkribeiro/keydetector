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
