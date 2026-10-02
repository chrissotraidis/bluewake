// SPDX-License-Identifier: GPL-3.0-or-later
// Display-rate policy adapted from Elliott Tate's Windows fork (f70305c).
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

inline int bw_smooth_requested(const char* value) {
    return value != nullptr && std::strcmp(value, "display") == 0 ? -1
        : value != nullptr && std::atoi(value) >= 3 ? 3 : 1;
}

inline int bw_smooth_steps(int requested, float refresh) {
    // Keep the preference; temporarily use 60 when a display cannot show 120.
    // An unknown/invalid refresh also uses 60 until SDL reports its mode.
    if (!std::isfinite(refresh) || refresh <= 0.f) return 1;
    if (requested == -1)
        return std::clamp(static_cast<int>(std::min(refresh, 240.f) / 30.f + .05f) - 1, 1, 7);
    return requested >= 3 && refresh >= 119.f ? 3 : 1;
}
