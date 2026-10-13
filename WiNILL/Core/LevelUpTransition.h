#pragma once
// Short level-up signal that leads into the augment selection screen.
// The game keeps running at full speed (no slow-motion: it read as lag);
// the player is protected and XP rings play until the selection opens.
// Owned by GameManager next to the augment reward queue, so clearing the
// queue also clears this. Runs on real (frame) time. Standard-library only
// for unit tests.
#include <algorithm>

struct LevelUpTransition {
    static constexpr float DURATION = 0.35f;

    void Begin() { active = true; elapsed = 0.0f; }
    void Cancel() { active = false; elapsed = 0.0f; }
    bool Active() const { return active; }

    // Advances by real time. Returns true exactly once, on the frame the
    // signal finishes; the caller then opens the augment selection.
    bool Update(float realDelta) {
        if (!active) return false;
        elapsed += std::max(0.0f, realDelta);
        if (elapsed < DURATION) return false;
        Cancel();
        return true;
    }

    // 0..1 over the signal, for presentation.
    float Progress() const {
        return active ? std::min(1.0f, elapsed / DURATION) : 0.0f;
    }

private:
    bool active = false;
    float elapsed = 0.0f;
};
