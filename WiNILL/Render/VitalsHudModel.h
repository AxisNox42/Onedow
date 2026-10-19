#pragma once
// ─────────────────────────────────────────────────────────────
// Vitals HUD model (no GL): the HP arc around the player and the XP line
// along the bottom edge of the screen.
//   HP: 240° solid arc open at the top, with a tick every 10%.
//       One fixed colour; red only below 25%. A red "ghost" keeps the HP
//       just lost for kGhostHold seconds, then drains down to the real HP.
//   XP: its own line, away from the player, with the level beside it.
// Drawing lives in VitalsHud.h.
// ─────────────────────────────────────────────────────────────
#include <algorithm>
#include <cstdint>

namespace vitals_hud {

inline constexpr int kHpSegments = 10;       // 10% steps (9 ticks between them)
inline constexpr float kDeg = 3.14159265f / 180.0f;
// Screen space (y down): 210° is upper left, 90° is straight down. The arc
// fills from its left end, counter-clockwise through the bottom to -30°.
inline constexpr float kArcStart = 210.0f * kDeg;
inline constexpr float kArcSweep = -240.0f * kDeg;
inline constexpr float kDangerFrac = 0.25f;
inline constexpr float kGhostHold = 0.40f;    // seconds the lost HP stays red
inline constexpr float kGhostDrain = 1.20f;   // HP fraction per second afterwards

inline bool IsDanger(float hpFrac) { return hpFrac > 0.0f && hpFrac < kDangerFrac; }

struct HpGhost {
    float frac = 1.0f;   // HP fraction the red ghost reaches
    float hold = 0.0f;

    void Reset(float hpFrac) { frac = hpFrac; hold = 0.0f; }

    void Update(float hpFrac, float delta) {
        if (hpFrac >= frac) {                 // healed or unchanged: no ghost
            frac = hpFrac;
            hold = 0.0f;
            return;
        }
        if (hpFrac < lastHp) hold = kGhostHold;   // a new hit restarts the hold
        lastHp = hpFrac;
        if (hold > 0.0f) {
            hold -= delta;
            if (hold >= 0.0f) return;
            delta = -hold;                    // the part of this step past the hold
            hold = 0.0f;
        }
        frac = std::max(hpFrac, frac - kGhostDrain * delta);
    }

private:
    float lastHp = 2.0f;
};

inline float XpFill(std::int64_t xp, std::int64_t needed, bool atCap) {
    if (atCap) return 1.0f;
    if (needed <= 0) return 0.0f;
    return std::clamp((float)((double)xp / (double)needed), 0.0f, 1.0f);
}

}  // namespace vitals_hud
