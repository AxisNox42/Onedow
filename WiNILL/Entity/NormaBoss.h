#pragma once

// ─────────────────────────────────────────────────────────────
// NORMA, the window boss (Docs/Norma_Boss_Design.md). Placeholder body: a
// plain square. It appears once, on the final (boss) wave, and the run is won
// when it falls. This file holds the numbers and the pure rules the boss,
// the game loop and the HUD share.
//
// Patterns: window patterns and the cross barrage take turns.
//   2 Refuge      open shelters near the player (reusing open ones), then
//                 flood the field
//   3 CloseWindow (phase 2) count down the pane the player stands in, then
//                 crush everything inside it
//   8 Throw       fill a small pane with shots, hurl it at the player, and
//                 burst it where it lands
//   7 Aim         a stream that keeps tracking the player
//  11 Dash        the body itself charges the player a few times
//  12 Detonate    set every pane the boss has open to blow, then, once they
//                 have, open fresh ones elsewhere
// The boss cannot touch OS windows itself: it posts Asks, and the game loop
// serves them through os_pane and writes the result back on the boss.
// ─────────────────────────────────────────────────────────────
#include <algorithm>
#include <cmath>
#include <vector>

namespace norma {

// Hit points before the run's enemy HP scaling. Tuned for a level-20 player
// (about 19 augments) reaching the boss wave: the real boss then carries
// about 2.4x this (the run's HP ramp at the final wave), roughly two minutes
// of landed fire at the damage a level-20 build is expected to deal. The
// bare level-1 gun (50 dmg every 0.2s, 250/s) needs over two minutes even
// against the unscaled figure.
constexpr float kBaseHp = 30000.0f;
// Phase 2 begins at or below this share of max HP.
constexpr float kPhase2Ratio = 0.40f;
// The boss keeps about this far from the player, drifting slowly.
constexpr float kHoverDistance = 420.0f;
constexpr float kHoverBand = 60.0f;
constexpr float kMoveSpeed = 70.0f;
// The drift eases in and out: speed climbs from 0 over kHoverRamp px beyond the
// band, and the body gains or sheds it at kHoverAccel px/s per second, so it
// never lurches between standing and moving.
constexpr float kHoverRamp = 140.0f;
constexpr float kHoverAccel = 150.0f;
constexpr float kCoastDecay = 5.0f;         // 1/s: how fast a loose drift dies out
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
constexpr float kRefugeWaveInterval = 0.6f;
constexpr int   kRefugeShots = 30;
constexpr float kRefugeShotSpeed = 290.0f;
constexpr float kRefugeLinger = 1.0f;       // pause before the next pattern
// 12 Detonate: the panes in use are shut down on purpose (their fuse, see
// space::kFuseSeconds), then the boss opens new ones.
constexpr float kDetonateFuseWait = 1.8f;   // fuse plus a beat for the panes to fold shut
constexpr float kDetonateReopen = 1.2f;     // the new panes' laser and bracket snap
// 3 CloseWindow
constexpr float kCloseCountdown = 3.0f;
constexpr float kCloseDamage = 35.0f;       // to the player left inside
constexpr float kCloseMobDamage = 200.0f;   // to mobs left inside

// 7 Aim: the aim follows the player at a capped turn rate, so moving steadily
// across the stream gets out of it.
constexpr float kAimTelegraph = 0.8f;
constexpr float kAimSeconds = 3.5f;
constexpr float kAimStreamInterval = 0.08f;
constexpr float kAimStreamSpeed = 500.0f;
constexpr float kAimTrackTelegraph = 4.0f;  // rad/s while warning
constexpr float kAimTrackRate = 1.8f;       // rad/s while firing
constexpr float kAimJitter = 0.04f;         // rad, alternating
constexpr float kAimLineLength = 360.0f;    // warning line, well short of the edge
constexpr float kAimMuzzle = 1.8f;          // muzzle star distance, in body units (base)
constexpr float kAimDrill = 1.6f;           // rad/s the cube turns about the aim while firing

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
constexpr int   kThrowRing = 30;
constexpr int   kThrowInner = 14;
constexpr float kThrowRingSpeed = 330.0f;
constexpr float kThrowInnerSpeed = 180.0f;
constexpr float kThrowSeconds = 3.0f;       // the boss's share of the pattern
// 11 Dash: the body charges the player kDashCount times. Each charge winds
// up: the body reddens, draws back and squashes, tracking the player until
// kDashLock of the wind; red chevrons run to the landing spot, where a red
// target ring fills over the wind-up and gets inward ticks once the lane
// locks; the body shivers just before it goes. It then shoots to the
// landing spot (past where the player was), stretching along the lane, and
// rests a moment.
// Touching the player on the way hurts and shoves it (once per charge).
constexpr int   kDashCount = 3;
constexpr int   kDashCountPhase2 = 4;
// The first charge of a turn winds up long (time to read it); the rest
// follow quickly, as one combo.
constexpr float kDashWindFirst = 1.2f, kDashWindFirstPhase2 = 1.0f;
constexpr float kDashWindNext = 0.7f, kDashWindNextPhase2 = 0.6f;
constexpr float kDashTremble = 0.15f;       // the body shivers this long before it goes
constexpr float kDashLock = 0.55f;       // locks early: the committed lane shows longer
constexpr float kDashPullBack = 60.0f;      // world px
constexpr float kDashTime = 0.42f;        // eased in and out (smoothstep)
constexpr float kDashOvershoot = 180.0f;    // world px past the player
constexpr float kDashMinLength = 260.0f, kDashMaxLength = 760.0f;
constexpr float kDashRest = 0.35f;
// Between charges the body backs off to at least kDashRunway from the player
// (at up to kDashBackSpeed), so no charge starts point-blank; the rest runs
// on up to kDashRestMax while it does.
constexpr float kDashRunway = 320.0f;       // world px
constexpr float kDashBackSpeed = 900.0f;    // world px/s
constexpr float kDashRestMax = 0.9f;
// A charge that lands throws the player off to the side of the lane (not
// along it, which pinned players to the screen edge), and the body deals no
// contact damage for kDashContactGrace after.
constexpr float kDashSideKnock = 0.85f;     // share of the shove across the lane
constexpr float kDashContactGrace = 1.0f;
constexpr float kDashDamage = 18.0f;
constexpr float kDashKnock = 240.0f;        // world px of shove
constexpr float kDashReach = 22.0f;         // past the body's half size
inline int DashCountFor(int phase) { return phase >= 2 ? kDashCountPhase2 : kDashCount; }
// The tesseract's corners, and where corner i lands on screen (unit cube,
// before roll) at the given yaw and pitch. The boss turns its cube with
// these; the drawing projects the same way.
inline constexpr float kCubeCorner[8][3] = {
    { -1, -1, -1 }, { 1, -1, -1 }, { 1, 1, -1 }, { -1, 1, -1 },
    { -1, -1, 1 }, { 1, -1, 1 }, { 1, 1, 1 }, { -1, 1, 1 } };
inline void CubeCornerOnScreen(int i, float yaw, float pitch, float& sx, float& sy) {
    const float px = kCubeCorner[i][0], py = kCubeCorner[i][1], pz = kCubeCorner[i][2];
    const float x1 = px * std::cos(yaw) + pz * std::sin(yaw);
    const float z1 = -px * std::sin(yaw) + pz * std::cos(yaw);
    sx = x1;
    sy = py * std::cos(pitch) - z1 * std::sin(pitch);
}

// Health on the body: the outer cube's eight stars, one per eighth of HP.
// They go dark in this corner order (fixed, so a dark star rides with the
// turning cube instead of hopping between corners).
constexpr int kHealthStars = 8;
inline constexpr int kStarOutOrder[8] = { 1, 3, 4, 6, 0, 5, 2, 7 };
inline int LitStarsFor(float hp, float maxHp) {
    if (maxHp <= 0.0f || hp <= 0.0f) return 0;
    const float share = std::clamp(hp / maxHp, 0.0f, 1.0f);
    return std::max(1, (int)std::ceil(share * (float)kHealthStars - 1e-4f));
}
constexpr float kStarOutFlash = 0.35f;      // a star going dark flares this long

inline float DashWindFor(int phase, int charge) {
    if (charge == 0) return phase >= 2 ? kDashWindFirstPhase2 : kDashWindFirst;
    return phase >= 2 ? kDashWindNextPhase2 : kDashWindNext;
}

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
// The pane slamming the screen edge bursts like a thrown box, at this share
// of its load, fanned back off the wall.
constexpr float kRamBurstShare = 2.0f / 3.0f;
constexpr float kRamBurstFan = 4.18879f;    // 240 degrees
constexpr float kRamDamage = 25.0f;
constexpr float kRamMobDamage = 150.0f;
constexpr float kRamKnock = 260.0f;         // world px of shove
constexpr float kRamSeconds = 3.0f;         // the boss's share of the pattern
inline float RamAimFor(int phase) { return phase >= 2 ? kRamAimPhase2 : kRamAim; }

// Panes belong to patterns: Detonate blows the boss's panes and opens
// fresh ones, and Refuge opens one when none is near; nothing opens them on a clock. The
// others (Refuge, Ram, Throw) use whatever is open. At most this many stand.
constexpr int   kMaxBossPanes = 2;
constexpr int   kMaxBossPanesPhase2 = 3;
inline int MaxBossPanesFor(int phase) { return phase >= 2 ? kMaxBossPanesPhase2 : kMaxBossPanes; }

// Sizes of the panes the boss opens, in screen pixels (random within these).
constexpr float kRefugeMinW = 420.0f, kRefugeMaxW = 640.0f;
constexpr float kRefugeMinH = 300.0f, kRefugeMaxH = 460.0f;
constexpr float kPaneMaxShareW = 0.35f, kPaneMaxShareH = 0.42f;   // of the screen
// Boss panes stand still, and every so often hop to a new spot: brackets
// mark the spot for kPaneHopWarn, then the pane glides there in kPaneHopMove.
constexpr float kPaneHoldMin = 5.0f, kPaneHoldMax = 7.0f;   // seconds between hops
constexpr float kPaneHopWarn = 0.6f;
constexpr float kPaneHopMove = 0.5f;

// Openings to hit the boss: after its charges, and when a rammed pane hits
// the screen edge, it holds still, exposed, taking extra damage.
constexpr float kExposeAfterDash = 1.6f;
constexpr float kExposeAfterRam = 2.0f;
constexpr float kExposeDamage = 1.5f;

// Laser flash and bracket snap when the boss claims or opens a pane.
constexpr float kLaserSeconds = 0.25f;
constexpr float kSnapSeconds = 0.25f;
constexpr float kSnapGrow = 0.6f;           // brackets start 1.6x and close in

enum class Pattern { None = 0, Refuge = 2, CloseWindow = 3, Aim = 7,
                     Throw = 8, Ram = 10, Dash = 11, Detonate = 12 };
enum class Stage { Recover, Wait, Telegraph, Act, Exit };

// Requests from the boss to the game loop, served once per frame.
enum class Ask { OpenRefuges, ClosePlayerWindow, ReleaseWindows,
                 ThrowBox, RamWindow, DetonatePanes,
                 ReopenPanes };
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
enum class Fx { Volley, Muzzle, Burst, Wave, EnterPane, LeavePane, PhaseShift, StarOut };
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

// Smooth version of HoverSpeed: zero inside the band, then rising evenly (with
// an eased start) to kMoveSpeed over kHoverRamp, signed like HoverSpeed.
inline float HoverSpeedSmooth(float distance) {
    const float over = distance - (kHoverDistance + kHoverBand);
    const float under = (kHoverDistance - kHoverBand) - distance;
    const float edge = over > 0.0f ? over : under > 0.0f ? -under : 0.0f;
    const float t = std::clamp(std::fabs(edge) / kHoverRamp, 0.0f, 1.0f);
    const float eased = t * t * (3.0f - 2.0f * t);
    return (edge >= 0.0f ? 1.0f : -1.0f) * eased * kMoveSpeed;
}

// The pattern after `count` earlier ones: window patterns on even turns,
// a direct attack (cross barrage or aimed stream) on odd ones. CloseWindow
// joins the window pool in phase 2.
inline Pattern NextPattern(int count, int phase, int roll) {
    const int r = roll < 0 ? -roll : roll;
    if (count % 2 == 1) return (r / 7) % 2 == 0 ? Pattern::Aim : Pattern::Dash;
    const int windowKinds = phase >= 2 ? 5 : 4;
    switch (r % windowKinds) {
    case 0:  return Pattern::Refuge;
    case 1:  return Pattern::Throw;
    case 2:  return Pattern::Ram;
    case 3:  return Pattern::Detonate;
    default: return Pattern::CloseWindow;
    }
}

} // namespace norma
