// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 dkribeiro

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
