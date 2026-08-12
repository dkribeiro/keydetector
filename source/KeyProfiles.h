// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 dkribeiro

#pragma once
#include <array>

// Krumhansl-Schmuckler key profiles, indexed by scale degree relative to the
// tonic (index 0 = tonic). Rotate across 12 tonics to build all 24 keys.
namespace KeyProfiles
{
    extern const std::array<float, 12> majorProfile;
    extern const std::array<float, 12> minorProfile;
}
