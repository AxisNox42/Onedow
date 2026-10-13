#pragma once
// Shared easing curves. Standard-library only so gameplay timing code and
// unit tests can use them without pulling in UI layout headers.
#include <algorithm>

// Cubic ease-out on a clamped [0,1] input: fast start, soft landing.
inline float EaseOutCubic(float t) {
    t = std::max(0.0f, std::min(1.0f, t));
    const float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
}
