#pragma once
// Single owner of how fast simulated gameplay runs relative to the frame.
// Hit-stop, the death slow-motion and later time effects are inputs here
// instead of separate adjustments scattered through the main loop.
// Standard-library only so it can be unit tested without a window.
#include <algorithm>

inline constexpr float DYING_TIME_SCALE = 0.15f;

struct GameplayTimeInputs {
    bool hitStop = false;   // freeze simulation, rendering continues
    bool dying = false;     // player death slow-motion
};

inline float GameplayTimeScale(const GameplayTimeInputs& in) {
    if (in.hitStop) return 0.0f;
    if (in.dying) return DYING_TIME_SCALE;
    return 1.0f;
}

// frameDelta is the clamped frame time (already scaled by smoke captures).
inline float ComputeGameplayDelta(float frameDelta,
                                  const GameplayTimeInputs& in) {
    return std::max(0.0f, frameDelta) * GameplayTimeScale(in);
}
