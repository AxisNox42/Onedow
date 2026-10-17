#pragma once
// Genesis elite cage: live thrown Swarms run to slots on a ring around the
// player, lock into a polygon and shrink it to the center. Touching a live
// edge hurts once; killing a vertex opens a gap. Pure math and tuning live
// here so they can be regression-tested headlessly; the state machine is in
// MonsterManager::UpdateGenesisLaunchers (hivePhase 4).
#include <algorithm>

namespace GenesisCage {

inline constexpr float COMMAND_TIME    = 0.4f;    // vertices hold, core flares
inline constexpr float DEPLOY_TIME     = 0.8f;    // vertices run to the ring
inline constexpr float LOCK_TIME       = 1.5f;    // ring shrinks to the center
inline constexpr float RADIUS          = 240.0f;  // outside the 190px Static Field
inline constexpr int   MIN_NODES       = 3;
inline constexpr int   MAX_NODES       = 6;
inline constexpr float RECRUIT_RANGE   = 600.0f;  // Swarm to player
inline constexpr float COOLDOWN        = 12.0f;   // also the spawn-time wait
inline constexpr float EDGE_HIT_RADIUS = 14.0f;
inline constexpr float EDGE_DAMAGE     = 12.0f;

inline float SlotAngle(float a0, int slot, int count) {
    return a0 + (float)slot * 6.2831853f / (float)count;
}

// Share of the way from a vertex's start to its slot, eased.
inline float DeployBlend(float t) {
    const float u = std::clamp(t / DEPLOY_TIME, 0.0f, 1.0f);
    return u * u * (3.0f - 2.0f * u);
}

inline float LockRadius(float t) {
    return RADIUS * (1.0f - std::clamp(t / LOCK_TIME, 0.0f, 1.0f));
}

// Squared distance from (px, py) to the segment a-b.
inline float SegmentDistSq(float px, float py, float ax, float ay, float bx, float by) {
    const float sx = bx - ax, sy = by - ay;
    const float lenSq = sx * sx + sy * sy;
    float t = lenSq > 1e-6f ? ((px - ax) * sx + (py - ay) * sy) / lenSq : 0.0f;
    t = std::clamp(t, 0.0f, 1.0f);
    const float dx = px - (ax + sx * t), dy = py - (ay + sy * t);
    return dx * dx + dy * dy;
}

}  // namespace GenesisCage
