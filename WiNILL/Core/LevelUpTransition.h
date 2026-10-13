#pragma once
// Level-up slow-motion that leads into the augment selection screen.
// Owned by GameManager next to the augment reward queue, so clearing the
// queue also clears this. Runs on real (frame) time; the gameplay speed it
// produces is fed to GameplayTime.h. Standard-library only for unit tests.
#include "Easing.h"
#include <algorithm>

struct LevelUpTransition {
    static constexpr float SLOW_DURATION = 0.45f;  // 1.0 -> MIN_SCALE
    static constexpr float HOLD_DURATION = 0.20f;  // stay at MIN_SCALE
    static constexpr float TOTAL_DURATION = SLOW_DURATION + HOLD_DURATION;
    static constexpr float MIN_SCALE = 0.15f;

    void Begin() { active = true; elapsed = 0.0f; }
    void Cancel() { active = false; elapsed = 0.0f; }
    bool Active() const { return active; }

    // Advances by real time. Returns true exactly once, on the frame the
    // transition finishes; the caller then opens the augment selection.
    bool Update(float realDelta) {
        if (!active) return false;
        elapsed += std::max(0.0f, realDelta);
        if (elapsed < TOTAL_DURATION) return false;
        Cancel();
        return true;
    }

    float TimeScale() const {
        if (!active) return 1.0f;
        return 1.0f - (1.0f - MIN_SCALE) * EaseOutCubic(elapsed / SLOW_DURATION);
    }

    // 0..1 over the whole transition, for presentation.
    float Progress() const {
        return active ? std::min(1.0f, elapsed / TOTAL_DURATION) : 0.0f;
    }

private:
    bool active = false;
    float elapsed = 0.0f;
};
