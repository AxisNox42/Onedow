#pragma once

// ─────────────────────────────────────────────────────────────
// NORMA, the window boss (Docs/Norma_Boss_Design.md). Placeholder body: a
// plain square. It appears once, on the final (boss) wave, and the run is won
// when it falls. Patterns arrive in a later step; this file holds the numbers
// and the pure rules the boss and the HUD share.
// ─────────────────────────────────────────────────────────────
#include <algorithm>

namespace norma {

// Hit points before the run's enemy HP scaling (an untested first guess).
constexpr float kBaseHp = 9000.0f;
// Phase 2 begins at or below this share of max HP.
constexpr float kPhase2Ratio = 0.40f;
// The boss keeps about this far from the player, drifting slowly.
constexpr float kHoverDistance = 420.0f;
constexpr float kHoverBand = 60.0f;
constexpr float kMoveSpeed = 70.0f;
constexpr float kSizeScale = 3.0f;
constexpr float kContactDps = 12.0f;
// How long the phase-change flash lasts.
constexpr float kPhaseFlashSeconds = 1.2f;

inline int PhaseFor(float hp, float maxHp) {
    if (maxHp <= 0.0f) return 1;
    return hp <= maxHp * kPhase2Ratio ? 2 : 1;
}

// Signed speed toward the player (+) or away (-) that keeps the hover band.
inline float HoverSpeed(float distance) {
    if (distance > kHoverDistance + kHoverBand) return kMoveSpeed;
    if (distance < kHoverDistance - kHoverBand) return -kMoveSpeed;
    return 0.0f;
}

} // namespace norma
