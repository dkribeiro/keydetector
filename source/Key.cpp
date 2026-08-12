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
