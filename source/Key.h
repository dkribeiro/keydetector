// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 dkribeiro

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
