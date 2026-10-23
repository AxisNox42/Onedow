#pragma once

// ─────────────────────────────────────────────────────────────
// NORMA, the window boss (Docs/Norma_Boss_Design.md). Placeholder body: a
// plain square. It appears once, on the final (boss) wave, and the run is won
// when it falls. This file holds the numbers and the pure rules the boss,
// the game loop and the HUD share.
//
// Patterns: window patterns and the cross barrage take turns.
//   1 Hide        slip into the real window farthest from the player, or
//                 into a pane the boss opened earlier, and fire from inside
//                 it; it never opens a pane for this
//   2 Refuge      open shelters near the player (reusing open ones), then
//                 flood the field
//   3 CloseWindow (phase 2) count down the pane the player stands in, then
//                 crush everything inside it
//   8 Throw       fill a small pane with shots, hurl it at the player, and
//                 burst it where it lands
//   9 Summon      load the boss's panes, then pour mobs out of them
//   7 Aim         a stream that keeps tracking the player
//  11 Dash        the body itself charges the player a few times
// The boss cannot touch OS windows itself: it posts Asks, and the game loop
// serves them through os_pane and writes the result back on the boss.
// ─────────────────────────────────────────────────────────────
#include <algorithm>
#include <cmath>
#include <vector>

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

// Pattern timings and shots (first guesses, Normal difficulty).
constexpr float kRecoverSeconds = 0.7f;     // opening between patterns
constexpr float kRecoverPhase2 = 0.4f;
constexpr float kShotDamage = 9.0f;
// Every boss shot flies this much faster in phase 2.
constexpr float kPhase2ShotSpeed = 1.15f;
// 1 Hide
constexpr float kHideTelegraph = 1.2f;      // laser, bracket snap, move in
constexpr float kHideSeconds = 8.0f;
constexpr float kHideExit = 0.6f;
constexpr float kHideInterval = 0.18f;
constexpr float kHideRingInterval = 1.2f;
constexpr int   kHideRingShots = 24;
constexpr float kHideTurnRate = 1.047f;
constexpr float kHideShotSpeed = 240.0f;
// 2 Refuge: the flood; the boss's open panes are the shelters.
constexpr float kRefugeTelegraph = 1.5f;
// If no pane is within reach (scaled by the player's move speed), the nearest
// one glides over to kRefugeGap of reach from the player in kRefugePull
// seconds, early in the telegraph; with none open, one opens there.
constexpr float kRefugeReach = 320.0f;      // px at move speed x1
constexpr float kRefugeReachMin = 180.0f, kRefugeReachMax = 420.0f;
constexpr float kRefugeGap = 0.5f;
constexpr float kRefugePull = 0.8f;
constexpr float kRefugeSeconds = 4.0f;
constexpr float kRefugeWaveInterval = 0.4f;
constexpr int   kRefugeShots = 48;
constexpr float kRefugeShotSpeed = 290.0f;
constexpr float kRefugeLinger = 1.0f;       // pause before the next pattern
// 3 CloseWindow
constexpr float kCloseCountdown = 3.0f;
constexpr float kCloseDamage = 35.0f;       // to the player left inside
constexpr float kCloseMobDamage = 200.0f;   // to mobs left inside

// 7 Aim: the aim follows the player at a capped turn rate, so moving steadily
// across the stream gets out of it.
constexpr float kAimTelegraph = 0.8f;
constexpr float kAimSeconds = 3.5f;
constexpr float kAimStreamInterval = 0.05f;
constexpr float kAimStreamSpeed = 500.0f;
constexpr float kAimTrackTelegraph = 4.0f;  // rad/s while warning
constexpr float kAimTrackRate = 1.8f;       // rad/s while firing
constexpr float kAimJitter = 0.04f;         // rad, alternating
constexpr float kAimLineLength = 360.0f;    // warning line, well short of the edge

// 8 Throw: the box grows out of the boss's body to its spot (kThrowAppear),
// opens there, sits while it fills, flies at where the player
// stood with speed growing e-fold every kThrowRamp seconds, waits a moment,
// then bursts. Closing it before then (in pause) cancels the burst.
constexpr float kThrowW = 320.0f, kThrowH = 240.0f;
constexpr float kThrowAppear = 0.6f;
constexpr float kThrowFill = 1.0f;
constexpr float kThrowStartSpeed = 60.0f;   // screen px/s
constexpr float kThrowRamp = 0.18f;
constexpr float kThrowFuse = 0.45f;
constexpr int   kThrowRing = 48;
constexpr int   kThrowInner = 24;
constexpr float kThrowRingSpeed = 330.0f;
constexpr float kThrowInnerSpeed = 180.0f;
constexpr float kThrowSeconds = 3.0f;       // the boss's share of the pattern
// 9 Summon: each gate (one of the boss's open panes) loads, then spawns mobs
// inside itself. Closing or minimizing a gate (in pause) stops its load.
constexpr int   kSummonGates = 2;
constexpr float kSummonLoad = 2.5f;
constexpr int   kSummonPhase1 = 4;          // mobs per gate
constexpr int   kSummonPhase2 = 6;
constexpr float kSummonSeconds = 3.0f;

// 11 Dash: the body charges the player kDashCount times. Each charge winds
// up (the body draws back, tracking the player until kDashLock of the wind,
// then locked, and red ghost squares march along its lane), then shoots
// along the lane past where the player was, easing out, and rests a moment.
// Touching the player on the way hurts and shoves it (once per charge).
constexpr int   kDashCount = 3;
constexpr int   kDashCountPhase2 = 4;
constexpr float kDashWind = 0.65f;
constexpr float kDashWindPhase2 = 0.5f;
constexpr float kDashLock = 0.65f;
constexpr float kDashPullBack = 26.0f;      // world px
constexpr float kDashTime = 0.3f;
constexpr float kDashOvershoot = 180.0f;    // world px past the player
constexpr float kDashMinLength = 260.0f, kDashMaxLength = 760.0f;
constexpr float kDashRest = 0.25f;
constexpr float kDashDamage = 18.0f;
constexpr float kDashKnock = 240.0f;        // world px of shove
constexpr float kDashReach = 22.0f;         // past the body's half size
inline int DashCountFor(int phase) { return phase >= 2 ? kDashCountPhase2 : kDashCount; }
inline float DashWindFor(int phase) { return phase >= 2 ? kDashWindPhase2 : kDashWind; }

// 10 Ram: the boss takes one of its panes and hurls it across the screen.
// While the boss controls it the pane's glow turns red. It squeezes down to
// a small block (so its lane can be sidestepped) and draws back from the
// player (aim, tracking until kRamLock of the way, then locked; its lane to
// the screen edge shows once locked), then flies, speeding up e-fold every
// kRamRamp seconds, until it hits the screen edge. Its frame sweeping over
// the player (or a mob) hurts and shoves it; being inside the pane when it
// sets off is safe. Dragging it away (Ctrl) breaks the control. The pane
// keeps its small size afterwards.
constexpr float kRamAim = 1.3f;
constexpr float kRamAimPhase2 = 1.0f;
constexpr float kRamLock = 0.55f;
constexpr float kRamW = 300.0f, kRamH = 220.0f;   // squeezed size, screen px
constexpr float kRamPullBack = 70.0f;       // screen px
constexpr float kRamStartSpeed = 250.0f;    // screen px/s
constexpr float kRamRamp = 0.18f;
constexpr float kRamMaxSpeed = 1800.0f;
constexpr float kRamSettle = 0.5f;          // red fades back after the hit
constexpr float kRamDamage = 25.0f;
constexpr float kRamMobDamage = 150.0f;
constexpr float kRamKnock = 260.0f;         // world px of shove
constexpr float kRamSeconds = 3.0f;         // the boss's share of the pattern
inline float RamAimFor(int phase) { return phase >= 2 ? kRamAimPhase2 : kRamAim; }

// Panes are not tied to patterns: the boss opens one every kPaneSpawn*
// seconds (the first soon after it arrives) while it has fewer than
// kMaxBossPanes open, and skips the turn when at the cap. They stay until the
// player closes them or they wear out; patterns use whatever is open.
constexpr int   kMaxBossPanes = 4;
constexpr float kPaneSpawnFirst = 1.5f;
constexpr float kPaneSpawnPhase1 = 8.0f;
constexpr float kPaneSpawnPhase2 = 5.5f;

// Sizes of the panes the boss opens, in screen pixels (random within these).
constexpr float kRefugeMinW = 560.0f, kRefugeMaxW = 920.0f;
constexpr float kRefugeMinH = 400.0f, kRefugeMaxH = 640.0f;
// Boss panes wander while the fight runs: a slow drift that every few
// seconds turns and speeds up (slow start, faster finish), bouncing off the
// screen edges.
constexpr float kPaneCruiseMin = 40.0f, kPaneCruiseMax = 110.0f;   // px/s
constexpr float kPaneTurnMin = 3.0f, kPaneTurnMax = 6.0f;          // seconds
constexpr float kPaneRampSeconds = 0.8f;   // speed builds up over this after a turn

// Laser flash and bracket snap when the boss claims or opens a pane.
constexpr float kLaserSeconds = 0.25f;
constexpr float kSnapSeconds = 0.25f;
constexpr float kSnapGrow = 0.6f;           // brackets start 1.6x and close in

enum class Pattern { None = 0, Hide = 1, Refuge = 2, CloseWindow = 3, Aim = 7,
                     Throw = 8, Summon = 9, Ram = 10, Dash = 11 };
enum class Stage { Recover, Wait, Telegraph, Act, Exit };

// Requests from the boss to the game loop, served once per frame.
enum class Ask { ClaimFarWindow, OpenRefuges, ClosePlayerWindow, ReleaseWindows,
                 ThrowBox, OpenGates, RamWindow };
inline std::vector<Ask> g_Asks;

// Effect cues from the boss; the game loop turns them into shockwaves,
// sparks and shake (the boss cannot reach those systems itself).
//   Volley     a stream volley left the body (a = stream angle)
//   Muzzle     one aimed shot left the body (a = its angle)
//   Burst      a pattern opened or a big ring left (a = strength 0..2)
//   Wave       a flood ring left (a = ring speed)
//   EnterPane  the boss slipped into its pane
//   LeavePane  the boss came back out
//   PhaseShift phase 2 began
enum class Fx { Volley, Muzzle, Burst, Wave, EnterPane, LeavePane, PhaseShift };
struct Event {
    Fx fx;
    float x = 0.0f, y = 0.0f, a = 0.0f;
};
inline std::vector<Event> g_Events;

// Recoil after a volley: the body swells, then settles.
constexpr float kRecoilSeconds = 0.12f;
// Afterimages left while the boss glides into or out of a pane.
constexpr int kGhostCount = 6;
constexpr float kGhostSpacing = 0.035f;     // seconds between afterimages

// Predictive aim: the angle from (sx, sy) that meets a target at (px, py)
// moving at (vx, vy) with a shot of `speed`, scaled by `lead` (0 = aim at
// where it is now, 1 = full intercept, >1 = past it). Falls back to the
// current position when no intercept exists.
inline float LeadAngle(float sx, float sy, float px, float py, float vx, float vy,
                       float speed, float lead = 1.0f) {
    const float rx = px - sx, ry = py - sy;
    const float a = vx * vx + vy * vy - speed * speed;
    const float b = 2.0f * (rx * vx + ry * vy);
    const float c = rx * rx + ry * ry;
    float t = -1.0f;
    if (std::fabs(a) < 1e-4f) {
        if (std::fabs(b) > 1e-4f) t = -c / b;
    } else {
        const float disc = b * b - 4.0f * a * c;
        if (disc >= 0.0f) {
            const float root = std::sqrt(disc);
            const float t1 = (-b - root) / (2.0f * a), t2 = (-b + root) / (2.0f * a);
            t = (t1 > 0.0f && (t2 <= 0.0f || t1 < t2)) ? t1 : t2;
        }
    }
    if (t <= 0.0f) return std::atan2(ry, rx);
    t = std::min(t, 2.0f) * lead;   // never chase more than two seconds ahead
    return std::atan2(ry + vy * t, rx + vx * t);
}

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

// The pattern after `count` earlier ones: window patterns on even turns,
// a direct attack (cross barrage or aimed stream) on odd ones. CloseWindow
// joins the window pool in phase 2.
inline Pattern NextPattern(int count, int phase, int roll) {
    const int r = roll < 0 ? -roll : roll;
    if (count % 2 == 1) return (r / 7) % 2 == 0 ? Pattern::Aim : Pattern::Dash;
    const int windowKinds = phase >= 2 ? 6 : 5;
    switch (r % windowKinds) {
    case 0:  return Pattern::Hide;
    case 1:  return Pattern::Refuge;
    case 2:  return Pattern::Throw;
    case 3:  return Pattern::Summon;
    case 4:  return Pattern::Ram;
    default: return Pattern::CloseWindow;
    }
}

} // namespace norma
