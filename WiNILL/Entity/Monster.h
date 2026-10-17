#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <vector>

#include <glm/glm.hpp>

#include "Bullet.h"
#include "PlayerStats.h"
#include "EnemyShield.h"
#include "Experience.h"

class RangedMob;
class Monster;

// Magnetar's ward lattice: a regular icosahedron (12 vertices, 20 faces).
// Shared by the gameplay (which face a shot meets) and the drawing.
namespace MagnetarLattice {
inline constexpr float kPhi = 1.6180340f;
inline constexpr float kV[12][3] = {
    {-1, kPhi, 0}, {1, kPhi, 0}, {-1, -kPhi, 0}, {1, -kPhi, 0},
    {0, -1, kPhi}, {0, 1, kPhi}, {0, -1, -kPhi}, {0, 1, -kPhi},
    {kPhi, 0, -1}, {kPhi, 0, 1}, {-kPhi, 0, -1}, {-kPhi, 0, 1}};
inline constexpr int kF[20][3] = {
    {0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11}, {1, 5, 9}, {5, 11, 4},
    {11, 10, 2}, {10, 7, 6}, {7, 1, 8}, {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8},
    {3, 8, 9}, {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}};

struct Proj { float x, y, z; };

// Rotates a point by yaw (about the vertical) then pitch (about the
// horizontal) and projects it orthographically: x, y on screen, z = depth
// (positive faces the viewer).
inline Proj Project(float px, float py, float pz, float yaw, float pitch) {
    const float cy = cosf(yaw), sy = sinf(yaw);
    const float x1 = px * cy + pz * sy, z1 = -px * sy + pz * cy;
    const float cp = cosf(pitch), sp = sinf(pitch);
    return { x1, py * cp - z1 * sp, py * sp + z1 * cp };
}

// The face a shot meets when it crosses the ward rim at screen angle `ang`:
// the one whose turned normal points most toward that side of the rim,
// leaning to the viewer so it is a visible face drawn on that side.
inline int FaceToward(float ang, float yaw, float pitch) {
    const float tx = cosf(ang) * 0.857f, ty = sinf(ang) * 0.857f, tz = 0.514f;
    int best = 0;
    float bestDot = -1e9f;
    for (int f = 0; f < 20; ++f) {
        const float nx = kV[kF[f][0]][0] + kV[kF[f][1]][0] + kV[kF[f][2]][0];
        const float ny = kV[kF[f][0]][1] + kV[kF[f][1]][1] + kV[kF[f][2]][1];
        const float nz = kV[kF[f][0]][2] + kV[kF[f][1]][2] + kV[kF[f][2]][2];
        const Proj n = Project(nx, ny, nz, yaw, pitch);
        const float d = n.x * tx + n.y * ty + n.z * tz;
        if (d > bestDot) { bestDot = d; best = f; }
    }
    return best;
}
}  // namespace MagnetarLattice

// The close-range roster contains six live signals. Scope remains the
// separate ranged mob; retired mutation variants have no runtime type anymore.
enum class MobKind {
    ROTOR,
    GENESIS,
    SWARM,
    GRAVIS,
    QUASAR,
    GIMBAL,
    REGULUS,
    ANTARES,
    MAGNETAR,
};

// Elite mobs carry a second ability and an elite frame.
inline constexpr bool IsEliteMob(MobKind kind) {
    return kind == MobKind::GRAVIS || kind == MobKind::GENESIS ||
           kind == MobKind::QUASAR;
}

inline constexpr float ANTARES_STUN_TIME_SEC = 0.6f;

inline void MobKillReward(MobKind kind, float& xpBase, float& scoreBase) {
    xpBase = 1.0f;
    scoreBase = 100.0f;
    switch (kind) {
    case MobKind::ROTOR:   xpBase = 2.0f;  scoreBase = 100.0f; break;
    case MobKind::GIMBAL:  xpBase = 4.0f;  scoreBase = 220.0f; break;
    case MobKind::GENESIS: xpBase = 15.0f; scoreBase = 450.0f; break;
    case MobKind::SWARM:   xpBase = 1.0f;  scoreBase = 12.0f;  break;
    case MobKind::GRAVIS:  xpBase = 16.0f; scoreBase = 520.0f; break;
    case MobKind::QUASAR:  xpBase = 12.0f; scoreBase = 480.0f; break;
    case MobKind::REGULUS: xpBase = 6.0f;  scoreBase = 260.0f; break;
    case MobKind::ANTARES: xpBase = 8.0f;  scoreBase = 350.0f; break;
    case MobKind::MAGNETAR: xpBase = 10.0f; scoreBase = 520.0f; break;
    default: break;
    }
}

inline int StardustRewardFor(MobKind kind) {
    switch (kind) {
    case MobKind::ROTOR:  return 1;
    case MobKind::SWARM:    return 1;
    case MobKind::GENESIS: return 20;
    case MobKind::GRAVIS:  return 15;
    case MobKind::QUASAR:  return 7;
    case MobKind::GIMBAL:  return 2;
    case MobKind::REGULUS: return 3;
    case MobKind::ANTARES: return 4;
    case MobKind::MAGNETAR: return 6;
    default:               return 0;
    }
}

inline float MobRewardMult(MobKind kind) {
    return kind == MobKind::SWARM ? 0.20f : 1.0f;
}

inline float MobXpBonus(MobKind kind, const PlayerStats& stats) {
    float bonus = (float)stats.mobXpBonus;
    if (kind == MobKind::ROTOR) bonus += (float)stats.rotorXpBonus;
    const int index = (int)kind;
    if (index >= 0 && index < PlayerStats::MOB_KIND_XP_SLOTS)
        bonus += (float)stats.mobKindXpBonus[index];
    return bonus;
}

inline XpAmount MobExperienceReward(MobKind kind, const PlayerStats& stats) {
    float xpBase, scoreBase;
    MobKillReward(kind, xpBase, scoreBase);
    // Currency/score reductions must not turn a tier-1 kill into zero XP.
    return XpFromReward((double)(xpBase + MobXpBonus(kind, stats)) * stats.xpMult);
}

class Monster {
public:
    static constexpr float GIMBAL_CHARGE_ROTATIONS = 2.0f;
    static constexpr float GIMBAL_PREFERRED_DISTANCE = 700.0f;
    static constexpr float REGULUS_BASE_HP = 360.0f;
    // Regulus fires an instant shield laser at a random eligible ally in
    // range; the shield is granted at once and nothing stays connected.
    static constexpr float REGULUS_SHOT_RANGE = 500.0f;
    static constexpr float REGULUS_SHOT_INTERVAL = 1.0f;
    static constexpr float REGULUS_BEAM_TIME = 0.25f;   // laser afterglow
    static constexpr float REGULUS_SHIELD_REGEN_DELAY = 8.0f;
    // Quasar beam: when the cooldown ends the axis snaps straight onto the
    // player and locks; the beam fires QUASAR_LOCK_TIME later on that fixed
    // angle (no sweep), so the lock window is the dodge window.
    static constexpr float QUASAR_LOCK_TIME = 0.7f;
    // The last QUASAR_PRECHARGE_TIME of the cooldown is the wind-up: the
    // Quasar stops drifting and its bands fold and draw energy in, before the
    // direction is chosen.
    static constexpr float QUASAR_COOLDOWN_TIME = 3.2f;
    static constexpr float QUASAR_PRECHARGE_TIME = 1.0f;
    // Death: a supernova. The body collapses, holds, then blasts at
    // QUASAR_NOVA_BLAST; the kill's stardust (with its XP) is held until that
    // moment and released on the shock ring.
    static constexpr float QUASAR_NOVA_COLLAPSE = 1.0f;   // collapse ends, hold begins
    static constexpr float QUASAR_NOVA_BLAST = 1.25f;
    // Wave time after the blast; the Genesis wave lasts the same. Kept short
    // so a tier-4 kill does not leave rings over the arena.
    static constexpr float QUASAR_NOVA_WAVE_TIME = 0.60f;
    static constexpr float QUASAR_NOVA_TIME = QUASAR_NOVA_BLAST + QUASAR_NOVA_WAVE_TIME;
    // Quasar shock front's final radius / base (halo is 1.95).
    static constexpr float QUASAR_NOVA_REACH = 3.64f;
    // Genesis death: the same build-up as the Quasar nova (instability, then
    // a squeeze), then a single blast wave. The kill stardust comes out with
    // the wave, like the Quasar's.
    static constexpr float GENESIS_WAVE_1 = QUASAR_NOVA_BLAST;
    static constexpr float GENESIS_WAVE_GAP = 0.60f;
    static constexpr int GENESIS_WAVE_COUNT = 1;
    // Wave life in seconds = GENESIS_WAVE_LIFE_BASE + GENESIS_WAVE_LIFE_PER_REACH * reach.
    static constexpr float GENESIS_WAVE_LIFE_BASE = 0.31f;
    static constexpr float GENESIS_WAVE_LIFE_PER_REACH = 0.06f;
    static constexpr float GENESIS_WAVE_REACH = 4.76f;    // wave radius / base (plates reach ~2.45)
    static constexpr float GENESIS_WAVE_FALLOFF = 0.75f;  // each next wave's share of the last
    static constexpr float GENESIS_DEATH_TIME = GENESIS_WAVE_1 + QUASAR_NOVA_WAVE_TIME;
    static_assert(GENESIS_WAVE_GAP * (GENESIS_WAVE_COUNT - 1) + GENESIS_WAVE_LIFE_BASE +
                  GENESIS_WAVE_LIFE_PER_REACH * GENESIS_WAVE_REACH <= QUASAR_NOVA_WAVE_TIME,
                  "the last Genesis wave must finish inside the shared wave time");
    static constexpr float QUASAR_BEAM_RADIUS = 55.0f;
    static constexpr float QUASAR_BEAM_DPS = 45.0f;
    // While firing, the beam turns very slowly (at most this rate, rad/s)
    // toward the player. The beam runs both ways, so it turns whichever way
    // brings the nearer end onto the player.
    static constexpr float QUASAR_BEAM_TURN = 0.08f;
    static constexpr float ANTARES_BASE_HP = 600.0f;
    static constexpr int ANTARES_TRAIL_COUNT = 6;
    // Genesis shell core: three shells lock in one by one while tracking
    // (one stack each); at 3/3 it aims 0.8s and throws all three Swarms, then
    // regroups 0.2s with the shells empty. About 6s per volley. Thrown
    // Swarms fly, then stun like Antares.
    static constexpr float GENESIS_TRACK_TIME = 4.6f;
    static constexpr int GENESIS_STACKS = 3;              // shells = Swarms per volley
    static constexpr float GENESIS_STACK_TIME = GENESIS_TRACK_TIME / GENESIS_STACKS;
    static constexpr float GENESIS_AIM_TIME = 0.8f;
    // Throw: each Swarm is fed down its spoke into the core (FEED), then
    // leaves; shots are THROW_GAP apart and the phase ends THROW_END after
    // the last one.
    static constexpr float GENESIS_THROW_GAP = 0.20f;
    static constexpr float GENESIS_FEED_TIME = 0.12f;
    static constexpr float GENESIS_THROW_END = 0.10f;
    static constexpr float GENESIS_RECOVER_TIME = 0.2f;
    static constexpr float GENESIS_TURN_RATE = 0.9f;      // rad/s while tracking
    // Thrown Swarm speed by share of the flight distance: fast launch,
    // braking into the landing (0-60% / 60-80% / 80-100%).
    static constexpr float GENESIS_THROW_SPEED_LAUNCH = 1400.0f;  // px/s
    static constexpr float GENESIS_THROW_SPEED_MID = 1100.0f;
    static constexpr float GENESIS_THROW_SPEED_LAND = 600.0f;
    static constexpr float GENESIS_THROW_SPLIT_MID = 0.60f;
    static constexpr float GENESIS_THROW_SPLIT_LAND = 0.80f;
    // Each Genesis may have at most this many of its own thrown Swarms
    // alive (in flight, stunned or chasing); separate from swarmCap.
    static constexpr int GENESIS_THROWN_CAP = 15;
    // The first Swarm of a volley lands on the player's position when the aim
    // locks; the rest land at random points within this radius of it. There
    // is no range limit.
    static constexpr float GENESIS_SCATTER_RADIUS = 110.0f;
    // A thrown Swarm spins in proportion to its flight speed (rad per px):
    // fast off the muzzle, slowing as it brakes into the landing.
    static constexpr float THROWN_SWARM_SPIN_PER_PX = 0.016f;
    // Landing shockwave around each thrown Swarm.
    static constexpr float THROWN_SWARM_BLAST_RADIUS = 80.0f;
    static constexpr float THROWN_SWARM_BLAST_DAMAGE = 6.0f;
    static constexpr float THROWN_SWARM_DAMAGE = 10.0f;
    static constexpr float THROWN_SWARM_STUN = ANTARES_STUN_TIME_SEC;
    static constexpr float MAGNETAR_BASE_HP = 450.0f;
    // Each of the 20 lattice faces is its own shield: a face stops
    // MAGNETAR_FACE_HITS shots from its side, then breaks and lets shots
    // through there until it restores MAGNETAR_FACE_RESTORE seconds later.
    static constexpr int MAGNETAR_FACES = 20;
    static constexpr int MAGNETAR_FACE_HITS = 4;
    static constexpr float MAGNETAR_FACE_RESTORE = 8.0f;
    static constexpr float MAGNETAR_FIELD_RADIUS = 145.0f;
    // A face that stops a shot flashes white for this long.
    static constexpr float MAGNETAR_HIT_FLASH = 0.20f;

    // Antares is a bull: it keeps accelerating while it chases, turns worse
    // the faster it goes, and drops back to stage 1 once it overshoots.
    // Ramming the player knocks them away and stuns Antares; a finished run
    // (skid) ends in a short rest. Once inside the map, hitting its edge
    // while running stuns it too, so a missed run never carries it off-map.
    enum class AntaresState { CHASE, SKID, STUNNED, REST };
    static constexpr float ANTARES_STUN_TIME = ANTARES_STUN_TIME_SEC;
    static constexpr float ANTARES_REST_TIME = 0.8f;
    static constexpr float ANTARES_MIN_SPEED = 60.0f;
    static constexpr float ANTARES_MAX_SPEED = 700.0f;
    // Only a stage-4 run (500 px/s and up) counts as overshooting the player.
    static constexpr float ANTARES_PASS_SPEED = 500.0f;

    float worldX = 0.0f;
    float worldY = 0.0f;
    float speed = 150.0f;
    float hp = 90.0f;
    bool alive = true;
    bool exploded = false;
    bool scored = false;
    bool noBlast = false;
    bool summoned = false;
    float hitFlashTimer = 0.0f;
    float deathEffectTimer = -1.0f;
    // Elite-kill experiment: set at the kill when the hit stop fires. During
    // the death's waves (EliteKillFxWeight) the rings warp slightly, inside
    // the death recipes themselves.
    bool eliteKillFx = false;
    // Kill reward held back for a delayed death (Quasar supernova).
    bool dustPending = false;
    int pendingDustValue = 0;
    // Timed death debris (Quasar blast, Genesis waves) already spawned; -1
    // when none is pending. Reduced matches the kill's particle budget.
    int deathBurstsSpawned = -1;
    bool deathBurstsReduced = false;
    XpAmount pendingDustXp = 0;
    glm::vec3 color = glm::vec3(1.0f, 0.27f, 0.0f);

    MobKind kind = MobKind::ROTOR;
    float sizeScale = 1.0f;
    float contactDmg = 5.0f;
    float singularityGrace = 0.0f;

    EnemyShieldState regulusShield;
    float regulusBaseHp = 0.0f;
    float regulusShieldRegenTimer = 0.0f;
    float regulusVisualAngle = 0.0f;
    float regulusDriftAngle = 0.0f;
    float regulusDriftTimer = 0.0f;
    float regulusShotTimer = 0.5f;    // time until the next shield laser

    // Genesis station state.
    float spawnTimer = 0.0f;
    bool anchored = false;
    float spawnAnchorX = 0.0f;
    float spawnAnchorY = 0.0f;
    int hivePhase = 0;
    float hiveOpenFactor = 0.0f;
    bool hiveSpawned = false;
    int hiveSpawnCount = 0;
    float hiveOrbitAngle = 0.0f;
    float hivePulseTimer = 0.0f;
    bool genesisEgress = false;
    float genesisEgressX = 0.0f;
    float genesisEgressY = 0.0f;
    float genesisEgressTimer = 0.0f;
    // Genesis shell core. hiveOrbitAngle is the facing (muzzle direction),
    // hivePhase 0 track+charge / 1 aim / 2 throw / 3 regroup, spawnTimer
    // times the phase and hiveSpawnCount counts throws in this volley.
    float genesisShellSpin = 0.0f;    // shell ring offset from the facing
    float genesisSpinBase = 0.0f;     // spin at the last lock (muzzle gap on the facing)
    float genesisAimAngle = 0.0f;
    std::array<float, 3> genesisShotX{};   // landing points, rolled at aim lock
    std::array<float, 3> genesisShotY{};
    float genesisRecoil = 0.0f;       // 1 at a throw, decays back to 0
    float genesisChildHpMult = 1.0f;  // spawn-time HP ramp, passed to thrown Swarms
    int genesisId = 0;                // owner key for the per-Genesis thrown cap
    inline static int s_NextGenesisId = 1;

    // Thrown Swarm: 1 flight, 2 landing stun, 0 ordinary chase. The Antares
    // trail buffer doubles as this Swarm's afterimage history.
    bool genesisThrown = false;       // counts toward its owner's GENESIS_THROWN_CAP
    int thrownOwnerId = 0;            // genesisId of the Genesis that threw it
    int thrownPhase = 0;
    float thrownTimer = 0.0f;
    float thrownDuration = 0.0f;
    float thrownDistance = 0.0f;
    float thrownStartX = 0.0f, thrownStartY = 0.0f;
    float thrownTargetX = 0.0f, thrownTargetY = 0.0f;
    float thrownHeading = 0.0f;
    float thrownSpin = 0.0f;          // body angle; turns with flight speed

    // Quasar state.
    int quasarState = 0;
    float quasarTimer = 0.0f;
    float quasarAimX = 1.0f;
    float quasarAimY = 0.0f;
    bool quasarAimSet = false;        // aim chosen for this attack (at wind-up start)
    float quasarVisualAngle = 0.0f;
    float quasarShellSpin = 0.0f;   // accretion shell spin; speeds up with heat
    float quasarMoveAngle = 0.0f;
    float quasarMoveTurn = 0.0f;
    float quasarMoveTimer = 0.0f;

    // Gravis state.
    float gravisVisualAngle = 0.0f;
    float gravisDriftAngle = 0.0f;
    float gravisDriftTimer = 0.0f;

    // Gimbal mirrors Scope's accelerating two-rotation charge for one shot.
    float gimbalAttackTimer = 0.0f;
    float gimbalChargeAngle = 0.0f;
    float gimbalChargeSpeed = 0.0f;
    float gimbalBurstTimer = 0.0f;
    float gimbalVisualAngle = 0.0f;
    float gimbalOrbitSign = 1.0f;
    bool gimbalCharging = false;
    bool gimbalBursting = false;
    bool gimbalFired = false;

    // Antares chase state. Trail samples (x, y, angle) drive afterimages.
    AntaresState antaresState = AntaresState::CHASE;
    float antaresStateTimer = 0.0f;
    float antaresHeadingX = 0.0f;
    float antaresHeadingY = 0.0f;
    float antaresMoveSpeed = 0.0f;
    float antaresSpinSpeed = 0.0f;
    float antaresVisualAngle = 0.0f;
    float antaresPrevFacing = 1.0f;   // dot(heading, to-player) last frame
    float antaresSkidDecel = 0.0f;
    // Player displacement requested by a ram; the game loop applies and clears it.
    float antaresKnockX = 0.0f;
    float antaresKnockY = 0.0f;
    float antaresTrailTimer = 0.0f;
    std::array<glm::vec3, ANTARES_TRAIL_COUNT> antaresTrail{};
    // Spawns sit off-map; the edge only holds Antares once it has come in.
    bool antaresInArena = false;
    // Per face: shots it can still stop, seconds since it broke (< 0 while
    // intact) and seconds since it last restored (drives the snap-back look).
    std::array<int, MAGNETAR_FACES> magnetarFaceHits{};
    std::array<float, MAGNETAR_FACES> magnetarFaceBroken{};
    std::array<float, MAGNETAR_FACES> magnetarFaceRestored{};
    float magnetarVisualAngle = 0.0f;
    float magnetarFacing = 0.0f;       // magnetic axis; the N pole tracks the player
    float magnetarInterceptTimer = 0.0f;
    float magnetarInterceptX = 0.0f;
    float magnetarInterceptY = 0.0f;
    // Per face: hit flash time left (MAGNETAR_HIT_FLASH .. 0).
    std::array<float, MAGNETAR_FACES> magnetarFaceHitFlash{};

    // Damage over time and Gravis' temporary collision grace period.
    float burnTimer = 0.0f;
    float burnDps = 0.0f;

    Monster(float startX, float startY, float hpMul = 1.0f,
            float speedMul = 1.0f, bool isSummoned = false)
        : worldX(startX), worldY(startY), summoned(isSummoned) {
        hp *= hpMul;
        speed = (120.0f + (float)(rand() % 61)) * speedMul;
        color = summoned ? glm::vec3(0.9f, 0.3f, 0.3f)
                         : glm::vec3(1.0f, 0.27f, 0.0f);
    }

    float ApplyDamage(float damage) {
        const bool hadShield = regulusShield.hp > 0.0f;
        const float dealt = ApplyEnemyDamageWithShield(
            hp, alive, hitFlashTimer, regulusShield, damage,
            worldX, worldY, (summoned ? 28.0f : 18.0f) * sizeScale,
            kind == MobKind::REGULUS);
        if (hadShield && regulusShield.hp <= 0.0f && kind == MobKind::REGULUS)
            regulusShieldRegenTimer = REGULUS_SHIELD_REGEN_DELAY;
        return dealt;
    }

    void MakeKind(MobKind requested) {
        kind = requested;
        sizeScale = 1.0f;
        if (kind == MobKind::GENESIS) {
            color = glm::vec3(0.70f, 0.08f, 0.12f);
            genesisChildHpMult = hp / 90.0f;   // progression x trial ramp at spawn
            hp *= 7.0f;
            speed *= 0.35f;
            sizeScale = 1.7f;   // tier-4 body; orbit frame + plates add the rest
            genesisId = s_NextGenesisId++;
            spawnTimer = 0.0f;
            hivePhase = 0;
            hiveOpenFactor = 0.0f;
            hiveSpawned = false;
            hiveSpawnCount = 0;
            genesisShellSpin = 0.0f;
            genesisSpinBase = 0.0f;
            hiveOrbitAngle = (float)(rand() % 628) * 0.01f;
            spawnAnchorX = worldX;
            spawnAnchorY = worldY;
        } else if (kind == MobKind::SWARM) {
            color = glm::vec3(0.55f, 0.0f, 0.0f);
            hp *= 0.35f;
            speed *= 1.05f;
            sizeScale = 1.0f;
        } else if (kind == MobKind::GRAVIS) {
            color = glm::vec3(0.58f, 0.42f, 1.0f);
            hp *= 8.0f;
            speed *= 0.24f;
            sizeScale = 2.1f;
            contactDmg = 6.0f;
            gravisVisualAngle = (float)(rand() % 628) * 0.01f;
            gravisDriftAngle = (float)(rand() % 628) * 0.01f;
            gravisDriftTimer = 1.0f + (float)(rand() % 120) * 0.01f;
        } else if (kind == MobKind::QUASAR) {
            color = glm::vec3(0.42f, 0.56f, 1.0f);
            hp *= 6.4f;
            speed *= 0.52f;
            sizeScale = 2.4f;   // tier 4: outer halo + tilted orbit add the rest
            contactDmg = 4.0f;
            quasarState = 0;
            quasarTimer = (float)(rand() % 120) * 0.01f;
            quasarVisualAngle = (float)(rand() % 628) * 0.01f;
            quasarMoveAngle = (float)(rand() % 628) * 0.01f;
            quasarMoveTurn = ((float)(rand() % 121) - 60.0f) * 0.01f;
            quasarMoveTimer = 0.8f + (float)(rand() % 101) * 0.01f;
        } else if (kind == MobKind::GIMBAL) {
            color = glm::vec3(0.62f, 0.95f, 0.28f);
            hp *= 1.65f;
            speed *= 0.58f;
            sizeScale = 1.12f;
            contactDmg = 4.0f;
            gimbalAttackTimer = 1.25f + (float)(rand() % 90) * 0.01f;
            gimbalChargeAngle = 0.0f;
            gimbalChargeSpeed = 0.4f;
            gimbalBurstTimer = 0.0f;
            gimbalVisualAngle = (float)(rand() % 628) * 0.01f;
            gimbalOrbitSign = (rand() & 1) ? 1.0f : -1.0f;
            gimbalCharging = false;
            gimbalBursting = false;
            gimbalFired = false;
        } else if (kind == MobKind::REGULUS) {
            color = glm::vec3(0.722f, 0.937f, 1.0f);
            hp *= REGULUS_BASE_HP / 90.0f;
            speed *= 0.42f;
            sizeScale = 1.35f;
            contactDmg = 0.0f;
            regulusBaseHp = hp;
            regulusShield = {};
            regulusShield.maxHp = regulusBaseHp * 0.9f;
            regulusShield.hp = regulusShield.maxHp;
            regulusShieldRegenTimer = 0.0f;
            regulusVisualAngle = (float)(rand() % 628) * 0.01f;
            regulusDriftAngle = (float)(rand() % 628) * 0.01f;
            regulusDriftTimer = 0.8f + (float)(rand() % 120) * 0.01f;
            regulusShotTimer = 0.5f;
        } else if (kind == MobKind::ANTARES) {
            color = glm::vec3(0.91f, 0.275f, 0.227f);
            hp *= ANTARES_BASE_HP / 90.0f;
            sizeScale = 1.6f;
            antaresState = AntaresState::CHASE;
            antaresStateTimer = 0.0f;
            antaresHeadingX = antaresHeadingY = 0.0f;
            antaresMoveSpeed = ANTARES_MIN_SPEED;
            antaresSpinSpeed = 0.5236f;
            antaresVisualAngle = (float)(rand() % 628) * 0.01f;
            antaresPrevFacing = 1.0f;
            antaresSkidDecel = 0.0f;
            antaresKnockX = antaresKnockY = 0.0f;
            antaresTrailTimer = 0.0f;
            antaresTrail.fill(glm::vec3(worldX, worldY, antaresVisualAngle));
            antaresInArena = false;
        } else if (kind == MobKind::MAGNETAR) {
            color = glm::vec3(1.0f, 0.84f, 0.28f);
            hp *= MAGNETAR_BASE_HP / 90.0f;
            speed *= 0.78f;
            sizeScale = 1.8f;
            contactDmg = 5.0f;
            magnetarFaceHits.fill(MAGNETAR_FACE_HITS);
            magnetarFaceBroken.fill(-1.0f);
            magnetarFaceRestored.fill(99.0f);
            magnetarVisualAngle = (float)(rand() % 628) * 0.01f;
            magnetarInterceptTimer = 0.0f;
        }
    }

    // Stages 1-4 are read from the current speed, so they rise continuously
    // and fall back together with the speed after an overshoot.
    int AntaresStage() const {
        if (antaresMoveSpeed >= ANTARES_PASS_SPEED) return 4;
        if (antaresMoveSpeed >= 350.0f) return 3;
        if (antaresMoveSpeed >= 200.0f) return 2;
        return 1;
    }

    // Frame edge plus the player's body, so a ram lands on touch, not overlap.
    float AntaresRamRange() const { return 26.0f * sizeScale + 18.0f; }

    // Keeps Antares inside the map (world-space edges). Until its frame is
    // fully inside it is left alone; after that a run that hits an edge is
    // stopped there and stunned, as if it slammed into a wall. A stage 1
    // crawl just slides along the edge, so a player hugging the wall does
    // not stun-lock it.
    void ConfineAntares(float left, float top, float right, float bottom) {
        if (!alive || kind != MobKind::ANTARES || thrownPhase != 0) return;
        const float radius = 26.0f * sizeScale;
        const float minX = left + radius, maxX = right - radius;
        const float minY = top + radius, maxY = bottom - radius;
        if (minX >= maxX || minY >= maxY) return;
        const bool inside = worldX >= minX && worldX <= maxX &&
                            worldY >= minY && worldY <= maxY;
        if (!antaresInArena) {
            antaresInArena = inside;
            return;
        }
        if (inside) return;
        worldX = std::clamp(worldX, minX, maxX);
        worldY = std::clamp(worldY, minY, maxY);
        if ((antaresState == AntaresState::CHASE ||
             antaresState == AntaresState::SKID) && AntaresStage() >= 2) {
            antaresState = AntaresState::STUNNED;
            antaresStateTimer = 0.0f;
            antaresMoveSpeed = 0.0f;
        }
    }

    // 0-1 position between the slowest and the top speed.
    float AntaresSpeedRatio() const {
        return std::clamp((antaresMoveSpeed - ANTARES_MIN_SPEED) /
                          (ANTARES_MAX_SPEED - ANTARES_MIN_SPEED), 0.0f, 1.0f);
    }

    // Afterimages grow with speed (0 at the slowest, 6 at the top) and fade
    // one by one as a skid bleeds the speed off.
    int AntaresGhostCount() const {
        return (int)std::ceil((float)ANTARES_TRAIL_COUNT * AntaresSpeedRatio());
    }

    void SpawnRotorDeathEffect(float base, float visualTime, bool reduced) const {
        const float radius = MobDeathParticleRadius(base);
        EnemyParticleBurst burst;
        burst.count = 20; burst.directions = 4;
        burst.life = 0.44f; burst.speedMin = radius * 3.5f; burst.speedMax = radius * 5.2f;
        burst.phase = visualTime * 0.78f + (float)((size_t)this % 628) * 0.01f;
        burst.startRadius = 0.16f; burst.jitter = 0.45f;
        burst.tangent = 0.4f; burst.colorLift = 0.10f;
        SpawnEnemyParticleBurst(worldX, worldY, color, radius, burst, reduced);
        SpawnMobDeathSparks(worldX, worldY, color, radius, 6, reduced);
    }

    void SpawnSwarmDeathEffect(float base, float visualTime, bool reduced) const {
        const float radius = MobDeathParticleRadius(base);
        EnemyParticleBurst burst;
        burst.count = 16; burst.directions = 3;
        burst.life = 0.40f; burst.speedMin = radius * 3.5f; burst.speedMax = radius * 5.2f;
        burst.sizeMin = 2.0f; burst.sizeMax = 4.0f;
        burst.phase = visualTime * 0.62f + (float)((size_t)this % 628) * 0.01f - 1.5708f;
        burst.startRadius = 0.12f; burst.jitter = 0.55f; burst.colorLift = 0.08f;
        SpawnEnemyParticleBurst(worldX, worldY, color, radius, burst, reduced);
        SpawnMobDeathSparks(worldX, worldY, color, radius, 4, reduced);
    }

    void SpawnGimbalDeathEffect(float base, bool reduced) const {
        const float radius = MobDeathParticleRadius(base);
        EnemyParticleBurst burst;
        burst.count = 38; burst.directions = 0; burst.life = 0.58f;
        burst.phase = gimbalVisualAngle; burst.randomPlacement = true;
        burst.startRadius = 0.10f; burst.jitter = 0.0f;
        burst.speedMin = radius * 3.2f; burst.speedMax = radius * 5.0f;
        burst.sizeMin = 2.8f; burst.sizeMax = 7.0f; burst.colorLift = 0.20f;
        SpawnEnemyParticleBurst(worldX, worldY, color, radius, burst, reduced);
        burst.count = 12; burst.life = 0.48f; burst.delay = 0.035f;
        burst.speedMin = radius * 3.8f; burst.speedMax = radius * 5.2f;
        burst.sizeMin = 5.0f; burst.sizeMax = 10.0f; burst.colorLift = 0.54f;
        SpawnEnemyParticleBurst(worldX, worldY, color, radius, burst, reduced);
        SpawnMobDeathSparks(worldX, worldY, color, radius, 10, reduced);
    }

    void SpawnAntaresDeathEffect(float base, bool reduced) const {
        const float radius = MobDeathParticleRadius(base);
        EnemyParticleBurst burst;
        burst.count = 30; burst.directions = 5; burst.life = 0.50f;
        burst.phase = antaresVisualAngle;
        burst.startRadius = 0.18f; burst.jitter = 0.40f;
        burst.speedMin = radius * 3.6f; burst.speedMax = radius * 5.4f;
        burst.sizeMin = 2.5f; burst.sizeMax = 8.0f; burst.colorLift = 0.30f;
        SpawnEnemyParticleBurst(worldX, worldY, color, radius, burst, reduced);
        SpawnMobDeathSparks(worldX, worldY, color, radius, 12, reduced);
    }

    void SpawnRegulusDeathEffect(float base, bool reduced) const {
        const float radius = MobDeathParticleRadius(base);
        EnemyParticleBurst burst;
        burst.count = 40;
        burst.life = 0.54f;
        burst.randomPlacement = true;
        burst.startRadius = 0.16f;
        burst.speedMin = radius * 3.2f;
        burst.speedMax = radius * 5.0f;
        burst.sizeMin = 2.8f;
        burst.sizeMax = 8.5f;
        burst.colorLift = 0.38f;
        SpawnEnemyParticleBurst(worldX, worldY, color, radius, burst, reduced);
        SpawnMobDeathSparks(worldX, worldY, color, radius, 12, reduced);
    }

    bool DeathEffectVisible() const {
        if (deathEffectTimer < 0.0f) return false;
        const float duration = kind == MobKind::GRAVIS ? 0.18f
                             : kind == MobKind::QUASAR ? QUASAR_NOVA_TIME
                             : kind == MobKind::GENESIS ? GENESIS_DEATH_TIME : 0.0f;
        return deathEffectTimer < duration;
    }

    void UpdateDeathEffect(float delta) {
        if (!alive && DeathEffectVisible()) {
            deathEffectTimer += std::max(0.0f, delta);
            SpawnStagedDeathBursts();
        }
    }

    // Elite-kill distortion strength 0..1 over the death animation: it swells
    // in just after the blast and settles before the wave ends; the rings
    // warp slightly while it is up. Zero during the collapse.
    float EliteKillFxWeight() const {
        if (!eliteKillFx || deathEffectTimer < 0.0f) return 0.0f;
        // GENESIS_WAVE_1 == QUASAR_NOVA_BLAST and both share the wave time,
        // so one window serves both.
        const float start = QUASAR_NOVA_BLAST + 0.05f;
        const float end = QUASAR_NOVA_BLAST + QUASAR_NOVA_WAVE_TIME - 0.10f;
        const float t = deathEffectTimer;
        if (t <= start || t >= end) return 0.0f;
        const float rise = std::clamp((t - start) / 0.15f, 0.0f, 1.0f);
        const float fall = std::clamp((end - t) / ((end - start) * 0.55f), 0.0f, 1.0f);
        const float v = std::min(rise, fall);
        return v * v * (3.0f - 2.0f * v);
    }

    // When a delayed death releases its held kill reward.
    float DeathRewardTime() const {
        return kind == MobKind::GENESIS ? GENESIS_WAVE_1 : QUASAR_NOVA_BLAST;
    }

    // True once, when a held kill reward should be released (the Quasar
    // supernova blast, the Genesis first wave). A death without a body
    // animation releases it at once.
    bool TakePendingDust() {
        if (!dustPending) return false;
        if (deathEffectTimer >= 0.0f && deathEffectTimer < DeathRewardTime() &&
            DeathEffectVisible())
            return false;
        dustPending = false;
        return true;
    }

    // Instability: a few hot sparks leak off the shaking core (at the kill).
    void SpawnGenesisInstabilityBurst(float base, bool reduced) const {
        const float radius = MobDeathParticleRadius(base);
        EnemyParticleBurst burst;
        burst.count = 10; burst.life = QUASAR_NOVA_COLLAPSE; burst.phase = hiveOrbitAngle;
        burst.startRadius = 0.10f; burst.jitter = 0.5f;
        burst.speedMin = radius * 0.35f; burst.speedMax = radius * 0.7f;
        burst.sizeMin = 2.0f; burst.sizeMax = 4.0f; burst.colorLift = 0.55f;
        SpawnEnemyParticleBurst(worldX, worldY, color, radius, burst, reduced);
    }

    // Debris for blast wave k, spawned when that wave goes off; it stays
    // inside the wave's reach.
    void SpawnGenesisWaveBurst(float base, int k, bool reduced) const {
        const float radius = MobDeathParticleRadius(base);
        static constexpr int kCount[] = { 44, 28 };
        static_assert(GENESIS_WAVE_COUNT <= 2, "kCount needs a burst per wave");
        const float f = GENESIS_WAVE_COUNT > 1
            ? (float)k / (float)(GENESIS_WAVE_COUNT - 1) : 0.0f;        // 0..1
        const float falloff = powf(GENESIS_WAVE_FALLOFF, (float)k);
        EnemyParticleBurst burst;
        burst.count = kCount[k]; burst.phase = hiveOrbitAngle;
        burst.life = 0.60f - 0.20f * f; burst.tangent = 0.18f;
        burst.startRadius = 0.12f; burst.jitter = 0.35f;
        burst.rangeScale = GENESIS_WAVE_REACH * falloff * base / radius;
        burst.speedMin = radius * 6.0f * falloff;
        burst.speedMax = radius * 8.2f * falloff;
        burst.sizeMin = 2.0f; burst.sizeMax = 9.0f - 5.0f * f;
        burst.colorLift = 0.45f - 0.25f * f;
        SpawnEnemyParticleBurst(worldX, worldY, color, radius, burst, reduced);
    }

    template <typename Line, typename Disc, typename Hexagon>
    void DrawGravisDeathEffect(Line line, Disc disc, Hexagon hexagon) const {
        const float base = (summoned ? 28.0f : 18.0f) * sizeScale;
        const float collapse = std::clamp(deathEffectTimer / 0.08f, 0.0f, 1.0f);
        const float pull = collapse * collapse;
        const float fade = 1.0f - std::clamp((deathEffectTimer - 0.08f) / 0.10f, 0.0f, 1.0f);
        const glm::vec3 bright = color + (glm::vec3(1.0f) - color) * (pull * 0.65f);
        if (deathEffectTimer < 0.08f) {
            const float orbit = base * 1.05f * (1.0f - pull * 0.95f);
            const float spin = gravisVisualAngle + pull * 1.8f;
            for (int i = 0; i < 6; ++i) {
                const float a = spin + (float)i * 1.0471976f;
                const float b = a + 1.0471976f;
                const float x = worldX + cosf(a) * orbit, y = worldY + sinf(a) * orbit;
                line(x, y, worldX + cosf(b) * orbit, worldY + sinf(b) * orbit,
                     1.6f, bright.r, bright.g, bright.b, 0.85f);
                line(worldX, worldY, x, y, 1.0f,
                     bright.r, bright.g, bright.b, 0.48f);
                hexagon(x, y, base * 0.18f * (1.0f - pull * 0.80f), a,
                        bright.r, bright.g, bright.b, 0.90f);
            }
        }
        const float coreRadius = base * (deathEffectTimer < 0.08f
                                      ? 0.58f * (1.0f - pull * 0.85f) : 0.30f * fade);
        disc(worldX, worldY, coreRadius * 1.4f,
             bright.r, bright.g, bright.b, fade * 0.18f);
        hexagon(worldX, worldY, coreRadius * 0.65f, gravisVisualAngle,
                bright.r, bright.g, bright.b, fade);
    }

    // Quasar supernova (QUASAR_NOVA_TIME): collapse to QUASAR_NOVA_COLLAPSE
    // (every layer is drawn into the core, spinning faster, the core
    // flickers), hold until QUASAR_NOVA_BLAST (one white point), then a short
    // blast: a brief central flash, one thin white shock front that stops
    // a short way past the halo (QUASAR_NOVA_REACH), and one uneven remnant shell
    // behind it. At most two rings at once. Lines and small discs only; no
    // dashed segments.
    template <typename Line, typename Disc, typename Hexagon>
    void DrawQuasarDeathEffect(Line line, Disc disc, Hexagon /*hexagon*/) const {
        const float base = (summoned ? 28.0f : 18.0f) * sizeScale;
        const float t = deathEffectTimer;
        const float seed = (float)((size_t)this % 97u) * 0.37f;
        auto ease = [](float v) {
            v = std::clamp(v, 0.0f, 1.0f);
            return v * v * (3.0f - 2.0f * v);
        };
        auto easeOut = [](float v) {
            v = std::clamp(v, 0.0f, 1.0f);
            return 1.0f - (1.0f - v) * (1.0f - v) * (1.0f - v);
        };
        const glm::vec3 white(1.0f);
        const float fxWarp = EliteKillFxWeight();
        auto ellipse = [&](float th, float a, float b, int n, float w,
                           const glm::vec3& c, float alpha,
                           float ox = 0.0f, float oy = 0.0f) {
            const float ux = cosf(th), uy = sinf(th);
            float px = 0.0f, py = 0.0f;
            for (int i = 0; i <= n; ++i) {
                const float s = 6.2831853f * (float)i / (float)n;
                const float warp = 1.0f + 0.025f * fxWarp *
                    (sinf(s * 5.0f + t * 21.0f + seed) + 0.5f * sinf(s * 3.0f - t * 15.0f));
                const float x = worldX + ox + (ux * cosf(s) * a - uy * sinf(s) * b) * warp;
                const float y = worldY + oy + (uy * cosf(s) * a + ux * sinf(s) * b) * warp;
                if (i > 0) line(px, py, x, y, w, c.r, c.g, c.b, alpha);
                px = x; py = y;
            }
        };

        // 1. Collapse.
        if (t < QUASAR_NOVA_COLLAPSE) {
            const float u = t / QUASAR_NOVA_COLLAPSE;
            const float k = 1.0f - 0.92f * ease(u);
            const float spinUp = quasarShellSpin + 16.0f * u * u;   // spins up harder as it shrinks
            const glm::vec3 c = color + (white - color) * (0.25f + 0.65f * u);
            const float a = 0.95f;
            ellipse(-quasarVisualAngle * 0.6f + seed * 2.0f + spinUp * 0.5f,
                    base * 2.25f * k, base * 0.78f * k, 36, 1.4f, c, a * 0.8f);
            ellipse(0.0f, base * 1.95f * k, base * 1.95f * k, 40, 1.5f, c, a * 0.7f);
            ellipse(quasarVisualAngle + seed + spinUp, base * 1.55f * k, base * 0.40f * k,
                    28, 2.0f, c, a);
            ellipse(spinUp * 1.4f, base * 0.97f * k, base * 0.40f * k, 24, 1.2f, c, a * 0.8f);
            ellipse(0.0f, base * k, base * k, 32, 1.8f, c, a);
            const float flick = 0.6f + 0.4f * sinf(t * 83.0f + seed) * sinf(t * 47.0f);
            disc(worldX, worldY, base * (0.30f * k + 0.10f), c.r, c.g, c.b, 0.30f * flick);
            disc(worldX, worldY, base * (0.12f + 0.06f * u), 1.0f, 1.0f, 1.0f,
                 0.75f + 0.25f * flick);
        }
        // 2. Hold: one white point, a faint ring tightening onto it.
        else if (t < QUASAR_NOVA_BLAST) {
            const float u = (t - QUASAR_NOVA_COLLAPSE) /
                            (QUASAR_NOVA_BLAST - QUASAR_NOVA_COLLAPSE);
            disc(worldX, worldY, base * 0.30f, 1.0f, 1.0f, 1.0f, 0.18f);
            disc(worldX, worldY, base * (0.14f - 0.04f * u), 1.0f, 1.0f, 1.0f, 1.0f);
            ellipse(0.0f, base * (0.55f - 0.30f * u), base * (0.55f - 0.30f * u), 24, 1.0f,
                    color + (white - color) * 0.6f, 0.45f + 0.4f * u);
        }
        // 3. Blast (QUASAR_NOVA_WAVE_TIME): a brief tight flash inside the
        // body, the thin white front rolling out to QUASAR_NOVA_REACH, and one
        // uneven remnant shell following it. Two rings at most at any time.
        if (t >= QUASAR_NOVA_BLAST) {
            const float b = t - QUASAR_NOVA_BLAST;
            const float life = QUASAR_NOVA_TIME - QUASAR_NOVA_BLAST;
            const float v = std::clamp(b / life, 0.0f, 1.0f);
            // Flash: gone before the remnant shell rises.
            const float flash = std::clamp(b / 0.15f, 0.0f, 1.0f);
            if (flash < 1.0f) {
                const float rf = base * (0.15f + 0.85f * easeOut(flash));
                ellipse(0.0f, rf, rf, 40, 1.0f + 1.5f * (1.0f - flash), white,
                        0.90f * (1.0f - flash));
            }
            // Shock front.
            const float front = std::clamp(b / (life * 0.85f), 0.0f, 1.0f);
            const float frontFade = 1.0f - ease((v - 0.45f) / 0.55f);
            if (frontFade > 0.0f) {
                const float rw = base * (0.3f + (QUASAR_NOVA_REACH - 0.3f) * easeOut(front));
                ellipse(0.0f, rw, rw, 64, 1.0f + 1.0f * (1.0f - front), white,
                        (0.90f - 0.35f * front) * frontFade);
            }
            // Remnant shell: continuous, uneven, a step behind the front.
            const float fade = 1.0f - ease((v - 0.35f) / 0.65f);
            const float rise = ease((b - 0.12f) / 0.18f);
            const float shellR = base * (0.9f + (QUASAR_NOVA_REACH * 0.85f - 0.9f) * easeOut(v));
            const glm::vec3 shellC = color + (white - color) * 0.35f;
            if (rise > 0.0f) {
                static constexpr int kSeg = 72;
                float px = 0.0f, py = 0.0f;
                for (int i = 0; i <= kSeg; ++i) {
                    const float a = 6.2831853f * (float)i / (float)kSeg;
                    const float rr = shellR * (1.0f + 0.04f * sinf(a * 4.0f + seed)
                        + 0.02f * fxWarp * sinf(a * 5.0f + t * 21.0f + seed));
                    const float x = worldX + cosf(a) * rr, y = worldY + sinf(a) * rr;
                    if (i > 0) {
                        const float f = 0.5f + 0.5f * sinf(a * 3.0f + seed) * sinf(a * 5.0f + seed * 2.0f);
                        line(px, py, x, y, 0.8f + 1.0f * f, shellC.r, shellC.g, shellC.b,
                             (0.25f + 0.45f * f) * fade * rise);
                    }
                    px = x; py = y;
                }
            }
            disc(worldX, worldY, base * 0.06f, 1.0f, 1.0f, 1.0f, 0.6f * fade);
        }
    }

    // Genesis death (GENESIS_DEATH_TIME). Same build-up as the Quasar nova:
    // until QUASAR_NOVA_COLLAPSE the core goes unstable (it shakes harder and
    // harder, its triangle twists, its ring warps) while the shells and
    // plates tremble; until GENESIS_WAVE_1 everything is squeezed inward.
    // Then GENESIS_WAVE_COUNT blast waves (one by default) go off
    // GENESIS_WAVE_GAP apart, each weaker than the last, stopping a short
    // way past the plates; the first wipes the body out. Lines and dots only.
    template <typename Line, typename Disc, typename Hexagon>
    void DrawGenesisDeathEffect(Line line, Disc disc, Hexagon /*hexagon*/) const {
        const float base = (summoned ? 28.0f : 18.0f) * sizeScale;
        const float R = base * 2.05f;
        const float t = deathEffectTimer;
        const float seed = (float)((size_t)this % 97u) * 0.37f;
        auto clamp01 = [](float v) { return std::clamp(v, 0.0f, 1.0f); };
        auto ease = [&](float v) { v = clamp01(v); return v * v * (3.0f - 2.0f * v); };
        auto easeOut = [&](float v) {
            v = clamp01(v);
            return 1.0f - (1.0f - v) * (1.0f - v) * (1.0f - v);
        };
        const glm::vec3 hot(1.0f, 0.62f, 0.52f), prod(1.0f, 0.24f, 0.20f), white(1.0f);
        const float x = worldX, y = worldY;
        const float ring = hiveOrbitAngle + genesisShellSpin;
        const float fxWarp = EliteKillFxWeight();

        // Blast waves: the first reaches GENESIS_WAVE_REACH (in base units),
        // each later one reaches GENESIS_WAVE_FALLOFF of the one before and is
        // fainter. Each is a thin white front with one red shell behind it.
        for (int k = 0; k < GENESIS_WAVE_COUNT; ++k) {
            const float reach = GENESIS_WAVE_REACH * powf(GENESIS_WAVE_FALLOFF, (float)k);
            const float waveLife = GENESIS_WAVE_LIFE_BASE + GENESIS_WAVE_LIFE_PER_REACH * reach;
            const float w = t - (GENESIS_WAVE_1 + GENESIS_WAVE_GAP * (float)k);
            if (w < 0.0f || w > waveLife) continue;
            const float p = w / waveLife;
            const float strength = 1.0f - 0.30f * (float)k;     // 1.0, 0.7
            const float rr = base * (0.5f + (reach - 0.5f) * easeOut(p));
            const glm::vec3 c = k == 0 ? white : (k == 1 ? hot : (k == 2 ? prod : color));
            const float a = (1.0f - p) * (0.55f + 0.45f * strength);
            const float width = 1.0f + 1.0f * strength * (1.0f - p);
            float px = 0.0f, py = 0.0f;
            for (int i = 0; i <= 56; ++i) {
                const float ang = 6.2831853f * (float)i / 56.0f;
                // A slight warp keeps the waves from reading as clean circles.
                const float wr = rr * (1.0f + 0.05f * (1.0f - p) * sinf(ang * 3.0f + seed + (float)k)
                    + 0.02f * fxWarp * (sinf(ang * 5.0f + t * 21.0f + (float)k * 1.9f)
                                        + 0.5f * sinf(ang * 3.0f - t * 15.0f)));
                const float qx = x + cosf(ang) * wr, qy = y + sinf(ang) * wr;
                if (i > 0) line(px, py, qx, qy, width, c.r, c.g, c.b, a);
                px = qx; py = qy;
            }
            if (k == 0) {                                         // trailing red shell
                const float r2 = rr * 0.86f;
                for (int i = 0; i < 48; ++i) {
                    const float a0 = 6.2831853f * (float)i / 48.0f;
                    const float a1 = 6.2831853f * (float)(i + 1) / 48.0f;
                    line(x + cosf(a0) * r2, y + sinf(a0) * r2, x + cosf(a1) * r2, y + sinf(a1) * r2,
                         1.2f, prod.r, prod.g, prod.b, 0.5f * (1.0f - p));
                }
            }
            if (w < 0.08f)
                disc(x, y, base * 0.07f * strength, 1.0f, 1.0f, 1.0f, 1.0f - w / 0.08f);
        }
        if (t >= GENESIS_WAVE_1) return;                          // body is gone

        // Build-up: instability, then the squeeze into the first wave.
        const float pre = clamp01(t / QUASAR_NOVA_COLLAPSE);
        const float squeeze = 1.0f - 0.20f * ease((t - QUASAR_NOVA_COLLAPSE) /
                                                  (GENESIS_WAVE_1 - QUASAR_NOVA_COLLAPSE));
        const float shake = base * 0.07f * pre * pre;
        const glm::vec3 shellC = color + (hot - color) * pre;
        int edge = 0;
        auto shellEdge = [&](float ax, float ay, float bx, float by, float width) {
            ++edge;
            const float jx = shake * sinf(t * 47.0f + (float)edge * 1.3f);
            const float jy = shake * cosf(t * 53.0f + (float)edge * 2.1f);
            line(x + (ax - x) * squeeze + jx, y + (ay - y) * squeeze + jy,
                 x + (bx - x) * squeeze + jx, y + (by - y) * squeeze + jy,
                 width, shellC.r, shellC.g, shellC.b, 0.92f);
        };
        const float half = 0.9076f, ro = R, ri = R * 0.74f;
        for (int k = 0; k < 3; ++k) {
            const float mid = ring + 1.0471976f + 2.0943951f * (float)k;
            float px[9], py[9];
            for (int i = 0; i <= 4; ++i) {
                const float a = mid - half + half * 0.5f * (float)i;
                px[i] = x + cosf(a) * ro;  py[i] = y + sinf(a) * ro;
            }
            const float ih = half - 0.1047f;
            for (int i = 0; i <= 3; ++i) {
                const float a = mid + ih - ih * (2.0f / 3.0f) * (float)i;
                px[5 + i] = x + cosf(a) * ri;  py[5 + i] = y + sinf(a) * ri;
            }
            for (int i = 0; i < 9; ++i) {
                const int n = (i + 1) % 9;
                shellEdge(px[i], py[i], px[n], py[n], 1.8f);
            }
            const float pm = mid + 0.2443f, ph = 0.5585f, po = R * 1.19f, pi = R * 1.03f;
            float qx[7], qy[7];
            for (int i = 0; i <= 3; ++i) {
                const float a = pm - ph * 0.8f + ph * 0.6f * (float)i;
                qx[i] = x + cosf(a) * po;  qy[i] = y + sinf(a) * po;
            }
            for (int i = 0; i <= 2; ++i) {
                const float a = pm + ph - ph * (float)i;
                qx[4 + i] = x + cosf(a) * pi;  qy[4 + i] = y + sinf(a) * pi;
            }
            for (int i = 0; i < 7; ++i) {
                const int n = (i + 1) % 7;
                shellEdge(qx[i], qy[i], qx[n], qy[n], 1.3f);
            }
            // Seated Swarm flickers hot while the core destabilises.
            const float cxs = x + cosf(mid) * (ro + ri) * 0.5f * squeeze;
            const float cys = y + sinf(mid) * (ro + ri) * 0.5f * squeeze;
            const float fl = 0.5f + 0.5f * sinf(t * 40.0f + (float)k * 2.1f);
            const float tr = R * 0.10f;
            for (int v = 0; v < 3; ++v) {
                const float a0 = mid + 2.0943951f * (float)v, a1 = a0 + 2.0943951f;
                line(cxs + cosf(a0) * tr, cys + sinf(a0) * tr,
                     cxs + cosf(a1) * tr, cys + sinf(a1) * tr,
                     1.4f, hot.r, hot.g, hot.b, 0.5f + 0.5f * fl * pre);
            }
        }

        // Unstable core: shaking harder, triangle twisting, ring warping; it
        // tightens with the squeeze.
        const float jx = shake * 1.6f * sinf(t * 61.0f + seed);
        const float jy = shake * 1.6f * sinf(t * 57.0f + seed * 1.3f);
        const float cx = x + jx, cy = y + jy;
        const glm::vec3 c = color + (hot - color) * pre;
        const float coreR = R * 0.40f * squeeze;
        float px = 0.0f, py = 0.0f;
        for (int i = 0; i <= 40; ++i) {
            const float a = 6.2831853f * (float)i / 40.0f;
            const float r = coreR * (1.0f + 0.16f * pre * sinf(a * 3.0f + t * 25.0f)
                                         + 0.08f * pre * sinf(a * 5.0f - t * 31.0f));
            const float qx = cx + cosf(a) * r, qy = cy + sinf(a) * r;
            if (i > 0) line(px, py, qx, qy, 1.6f + 1.0f * pre, c.r, c.g, c.b, 0.95f);
            px = qx; py = qy;
        }
        const float twist = ring + 3.0f * pre * pre * sinf(t * 17.0f + seed);
        for (int layer = 0; layer < 2; ++layer) {
            const float tr = R * (layer ? 0.12f : 0.24f) * (1.0f + 0.25f * pre) * squeeze;
            const float tw = layer ? -twist * 1.4f + 3.1415927f : twist;
            for (int v = 0; v < 3; ++v) {
                const float a0 = tw + 2.0943951f * (float)v, a1 = a0 + 2.0943951f;
                line(cx + cosf(a0) * tr, cy + sinf(a0) * tr,
                     cx + cosf(a1) * tr, cy + sinf(a1) * tr,
                     1.4f, hot.r, hot.g, hot.b, 0.9f);
            }
        }
        disc(cx, cy, base * (0.09f + 0.10f * pre), 1.0f, 1.0f, 1.0f, 0.95f);
        disc(cx, cy, R * (0.30f + 0.15f * pre) * squeeze, prod.r, prod.g, prod.b,
             0.10f + 0.25f * pre);
    }

    // Each recipe lives beside its mob; the renderer supplies batched primitives.
    template <typename Line, typename Disc, typename Hexagon>
    void DrawDeathEffect(Line line, Disc disc, Hexagon hexagon) const {
        if (!DeathEffectVisible()) return;
        switch (kind) {
        case MobKind::GRAVIS: DrawGravisDeathEffect(line, disc, hexagon); break;
        case MobKind::QUASAR: DrawQuasarDeathEffect(line, disc, hexagon); break;
        case MobKind::GENESIS: DrawGenesisDeathEffect(line, disc, hexagon); break;
        default: break;
        }
    }

    void SpawnGravisDeathEffect(float base, bool reduced) const {
        const float radius = MobDeathParticleRadius(base);
        EnemyParticleBurst burst;
        burst.count = 12; burst.life = 0.10f; burst.phase = gravisVisualAngle;
        burst.startRadius = 0.85f; burst.inward = true;
        burst.speedMin = radius * 8.0f; burst.speedMax = radius * 10.0f;
        burst.sizeMin = 2.5f; burst.sizeMax = 5.0f; burst.colorLift = 0.08f;
        SpawnEnemyParticleBurst(worldX, worldY, color, radius, burst, reduced);
        burst.count = 36; burst.life = 0.70f; burst.delay = 0.08f;
        burst.startRadius = 0.10f; burst.inward = false; burst.tangent = 0.40f;
        burst.speedMin = radius * 3.8f; burst.speedMax = radius * 5.3f;
        burst.sizeMin = 3.0f; burst.sizeMax = 13.0f; burst.colorLift = 0.22f;
        SpawnEnemyParticleBurst(worldX, worldY, color, radius, burst, reduced);
        SpawnMobDeathSparks(worldX, worldY, color, radius, 14, reduced);
    }

    // Collapse: matter drawn in from the halo toward the core (at the kill).
    void SpawnQuasarCollapseBurst(float base, bool reduced) const {
        const float radius = MobDeathParticleRadius(base);
        EnemyParticleBurst burst;
        burst.count = 20; burst.life = QUASAR_NOVA_COLLAPSE * 0.92f;
        burst.phase = quasarVisualAngle;
        burst.startRadius = 0.85f; burst.inward = true; burst.tangent = 0.35f;
        burst.sizeMin = 2.0f; burst.sizeMax = 4.5f;
        burst.speedMin = radius * 0.75f; burst.speedMax = radius * 1.0f;
        burst.colorLift = 0.30f;
        SpawnEnemyParticleBurst(worldX, worldY, color, radius, burst, reduced);
    }

    // Blast (spawned at QUASAR_NOVA_BLAST): a fast blue-white shell, then
    // slower heavier debris, both kept inside the shock front's reach.
    void SpawnQuasarBlastBurst(float base, bool reduced) const {
        const float radius = MobDeathParticleRadius(base);
        EnemyParticleBurst burst;
        burst.phase = quasarVisualAngle;
        burst.rangeScale = QUASAR_NOVA_REACH * base / radius;
        burst.count = 56; burst.life = QUASAR_NOVA_WAVE_TIME;
        burst.startRadius = 0.06f; burst.sizeMin = 2.0f; burst.sizeMax = 6.0f;
        burst.speedMin = radius * 2.4f; burst.speedMax = radius * 3.4f;
        burst.colorLift = 0.65f;
        SpawnEnemyParticleBurst(worldX, worldY, color, radius, burst, reduced);
        burst.count = 30; burst.life = QUASAR_NOVA_WAVE_TIME + 0.10f; burst.delay = 0.04f;
        burst.tangent = 0.25f; burst.sizeMin = 4.0f; burst.sizeMax = 10.0f;
        burst.speedMin = radius * 1.2f; burst.speedMax = radius * 2.0f;
        burst.colorLift = 0.35f;
        SpawnEnemyParticleBurst(worldX, worldY, color, radius, burst, reduced);
    }

    // Spawns the timed debris of a body death animation (Quasar blast,
    // Genesis waves) as deathEffectTimer passes each mark, so the debris
    // comes out with its ring instead of during the collapse.
    void SpawnStagedDeathBursts() {
        if (deathBurstsSpawned < 0) return;
        const float base = (summoned ? 28.0f : 18.0f) * sizeScale;
        if (kind == MobKind::QUASAR) {
            if (deathBurstsSpawned == 0 && deathEffectTimer >= QUASAR_NOVA_BLAST) {
                SpawnQuasarBlastBurst(base, deathBurstsReduced);
                deathBurstsSpawned = 1;
            }
        } else if (kind == MobKind::GENESIS) {
            while (deathBurstsSpawned < GENESIS_WAVE_COUNT &&
                   deathEffectTimer >= GENESIS_WAVE_1 +
                                       GENESIS_WAVE_GAP * (float)deathBurstsSpawned) {
                SpawnGenesisWaveBurst(base, deathBurstsSpawned, deathBurstsReduced);
                ++deathBurstsSpawned;
            }
        }
    }

    void SpawnDeathEffect(float visualTime, bool reduced = false,
                           bool animateBody = true) {
        if (exploded) return;
        exploded = true;
        if (animateBody && (kind == MobKind::GRAVIS || kind == MobKind::QUASAR ||
                            kind == MobKind::GENESIS))
            deathEffectTimer = 0.0f;
        const float base = (summoned ? 28.0f : 18.0f) * sizeScale;
        switch (kind) {
        case MobKind::ROTOR: SpawnRotorDeathEffect(base, visualTime, reduced); break;
        case MobKind::SWARM: SpawnSwarmDeathEffect(base, visualTime, reduced); break;
        case MobKind::GENESIS:
        case MobKind::QUASAR:
            if (animateBody) {
                // The blast debris follows on the body's timer
                // (SpawnStagedDeathBursts from UpdateDeathEffect).
                if (kind == MobKind::QUASAR) SpawnQuasarCollapseBurst(base, reduced);
                else SpawnGenesisInstabilityBurst(base, reduced);
                deathBurstsSpawned = 0;
                deathBurstsReduced = reduced;
            } else if (kind == MobKind::QUASAR) {
                SpawnQuasarBlastBurst(base, reduced);      // no body: blast at once
            } else {
                for (int k = 0; k < GENESIS_WAVE_COUNT; ++k)
                    SpawnGenesisWaveBurst(base, k, reduced);
            }
            break;
        case MobKind::GRAVIS: SpawnGravisDeathEffect(base, reduced); break;
        case MobKind::GIMBAL: SpawnGimbalDeathEffect(base, reduced); break;
        case MobKind::REGULUS: SpawnRegulusDeathEffect(base, reduced); break;
        case MobKind::ANTARES: SpawnAntaresDeathEffect(base, reduced); break;
        case MobKind::MAGNETAR: {
            const float radius = MobDeathParticleRadius(base);
            EnemyParticleBurst burst;
            burst.count = 32; burst.directions = 4; burst.life = 0.48f;
            burst.phase = magnetarVisualAngle;
            burst.startRadius = 0.10f; burst.jitter = 0.28f;
            burst.speedMin = radius * 3.0f; burst.speedMax = radius * 5.4f;
            burst.sizeMin = 2.5f; burst.sizeMax = 8.0f;
            burst.colorLift = 0.46f;
            SpawnEnemyParticleBurst(worldX, worldY, color, radius, burst, reduced);
            SpawnMobDeathSparks(worldX, worldY, color, radius, 10, reduced);
            break;
        }
        }
    }

    void ConstrainGenesisToViewport(float left, float top, float right, float bottom) {
        if (kind != MobKind::GENESIS) return;
        // Include the hull, rear damper plates and node glow, with a gap.
        const float margin = (summoned ? 28.0f : 18.0f) * sizeScale
                           * 2.75f + 8.0f;
        const float minX = left + std::min(margin, std::max(0.0f, right - left) * 0.5f);
        const float minY = top + std::min(margin, std::max(0.0f, bottom - top) * 0.5f);
        const float maxX = std::max(minX, right - margin);
        const float maxY = std::max(minY, bottom - margin);
        spawnAnchorX = std::clamp(spawnAnchorX, minX, maxX);
        spawnAnchorY = std::clamp(spawnAnchorY, minY, maxY);
        worldX = std::clamp(worldX, minX, maxX);
        worldY = std::clamp(worldY, minY, maxY);
    }

    void ConstrainGimbalToViewport(float left, float top, float right, float bottom) {
        if (kind != MobKind::GIMBAL) return;
        // Keep the complete rotating body and node glow inside the camera view.
        const float margin = 34.0f * sizeScale;
        const float minX = left + std::min(margin, std::max(0.0f, right - left) * 0.5f);
        const float minY = top + std::min(margin, std::max(0.0f, bottom - top) * 0.5f);
        const float maxX = std::max(minX, right - margin);
        const float maxY = std::max(minY, bottom - margin);
        worldX = std::clamp(worldX, minX, maxX);
        worldY = std::clamp(worldY, minY, maxY);
    }

    void ConstrainRegulusToViewport(float left, float top,
                                    float right, float bottom) {
        if (kind != MobKind::REGULUS) return;
        // Keep the enlarged outer shield ring fully visible and turn its drift
        // back into the arena when it reaches a screen edge.
        const float base = (summoned ? 28.0f : 18.0f) * sizeScale;
        const float margin = base * 2.12f + 14.0f;
        const float halfWidth = std::max(0.0f, right - left) * 0.5f;
        const float halfHeight = std::max(0.0f, bottom - top) * 0.5f;
        const float minX = left + std::min(margin, halfWidth);
        const float minY = top + std::min(margin, halfHeight);
        const float maxX = std::max(minX, right - margin);
        const float maxY = std::max(minY, bottom - margin);
        const bool hitX = worldX < minX || worldX > maxX;
        const bool hitY = worldY < minY || worldY > maxY;
        worldX = std::clamp(worldX, minX, maxX);
        worldY = std::clamp(worldY, minY, maxY);
        if (hitX) regulusDriftAngle = 3.14159265f - regulusDriftAngle;
        if (hitY) regulusDriftAngle = -regulusDriftAngle;
    }

    // 0 at rest, rising through search/charge, 1 while firing, then decaying
    // over the 1.4s recovery. Drives the shell spin rate and disc flattening.
    // Ward lattice orientation, shared by the hit test and the drawing.
    float MagnetarYaw() const { return magnetarVisualAngle * 0.6f; }
    float MagnetarPitch() const { return 0.45f + 0.15f * sinf(magnetarVisualAngle * 0.3f); }
    int MagnetarFaceToward(float ang) const {
        return MagnetarLattice::FaceToward(ang, MagnetarYaw(), MagnetarPitch());
    }

    // Wind-up progress 0..1 over the last QUASAR_PRECHARGE_TIME of the
    // cooldown (state 0); 0 otherwise.
    float QuasarPrecharge() const {
        if (quasarState != 0) return 0.0f;
        const float start = QUASAR_COOLDOWN_TIME - QUASAR_PRECHARGE_TIME;
        return std::clamp((quasarTimer - start) / QUASAR_PRECHARGE_TIME, 0.0f, 1.0f);
    }

    float QuasarHeat() const {
        switch (quasarState) {
        case 0: return 0.15f * QuasarPrecharge();
        case 1: return 0.15f;
        case 2: return 0.15f + 0.85f * std::min(1.0f, quasarTimer / QUASAR_LOCK_TIME);
        case 3: return 1.0f;
        case 4: return 1.0f - std::min(1.0f, quasarTimer / 1.4f);
        default: return 0.0f;
        }
    }

    // Shell stacks as a continuous 0..3: shell k is full once this reaches
    // k + 1. They charge while tracking, stay full through the aim, empty one
    // per throw and are empty while regrouping.
    float GenesisStackFill() const {
        switch (hivePhase) {
        case 0: return std::clamp(spawnTimer / GENESIS_STACK_TIME, 0.0f, (float)GENESIS_STACKS);
        case 1: return (float)GENESIS_STACKS;
        case 2: return (float)std::max(0, GENESIS_STACKS - hiveSpawnCount);
        default: return 0.0f;
        }
    }
    int GenesisStacks() const { return (int)GenesisStackFill(); }

    // Seconds since throw `shot` (0..2) left the core, or -1 if it has not.
    float GenesisThrowAge(int shot) const {
        if (hivePhase == 2) {
            if (shot >= hiveSpawnCount) return -1.0f;
            return spawnTimer + (float)(hiveSpawnCount - 1 - shot) * GENESIS_THROW_GAP;
        }
        if (hivePhase == 3)
            return GENESIS_THROW_END + spawnTimer +
                   (float)(GENESIS_STACKS - 1 - shot) * GENESIS_THROW_GAP;
        return -1.0f;
    }

    // Time to cover `fraction` (0..1) of a `dist` px throw under the
    // three-stage speed profile.
    static float ThrownFlightTime(float dist, float fraction) {
        const float d1 = dist * GENESIS_THROW_SPLIT_MID;
        const float d2 = dist * (GENESIS_THROW_SPLIT_LAND - GENESIS_THROW_SPLIT_MID);
        const float d = dist * std::clamp(fraction, 0.0f, 1.0f);
        if (d <= d1) return d / GENESIS_THROW_SPEED_LAUNCH;
        float t = d1 / GENESIS_THROW_SPEED_LAUNCH;
        if (d <= d1 + d2) return t + (d - d1) / GENESIS_THROW_SPEED_MID;
        t += d2 / GENESIS_THROW_SPEED_MID;
        return t + (d - d1 - d2) / GENESIS_THROW_SPEED_LAND;
    }

    // Share of the distance covered after `t` seconds (inverse of the above).
    static float ThrownFlightFraction(float dist, float t) {
        if (dist <= 0.001f) return 1.0f;
        const float t1 = ThrownFlightTime(dist, GENESIS_THROW_SPLIT_MID);
        const float t2 = ThrownFlightTime(dist, GENESIS_THROW_SPLIT_LAND);
        float d;
        if (t <= t1) d = t * GENESIS_THROW_SPEED_LAUNCH;
        else if (t <= t2) d = dist * GENESIS_THROW_SPLIT_MID + (t - t1) * GENESIS_THROW_SPEED_MID;
        else d = dist * GENESIS_THROW_SPLIT_LAND + (t - t2) * GENESIS_THROW_SPEED_LAND;
        return std::min(1.0f, d / dist);
    }

    void BeginThrownFlight(float targetX, float targetY) {
        genesisThrown = true;
        thrownPhase = 1;
        thrownTimer = 0.0f;
        thrownStartX = worldX;
        thrownStartY = worldY;
        thrownTargetX = targetX;
        thrownTargetY = targetY;
        const float dx = targetX - worldX, dy = targetY - worldY;
        const float dist = std::sqrt(dx * dx + dy * dy);
        thrownDistance = dist;
        thrownDuration = std::max(0.001f, ThrownFlightTime(dist, 1.0f));
        thrownHeading = atan2f(dy, dx);
        thrownSpin = thrownHeading;
        for (auto& sample : antaresTrail)
            sample = glm::vec3(worldX, worldY, thrownSpin);
        antaresTrailTimer = 0.0f;
    }

    // Returns true when the Swarm vanished on the player (no reward).
    bool UpdateThrownSwarm(float deltaTime, float distance, float& playerHP) {
        thrownTimer += deltaTime;
        if (thrownPhase == 1) {
            const float prevU = ThrownFlightFraction(thrownDistance, thrownTimer - deltaTime);
            const float u = ThrownFlightFraction(thrownDistance, thrownTimer);
            thrownSpin += (u - prevU) * thrownDistance * THROWN_SWARM_SPIN_PER_PX;
            worldX = thrownStartX + (thrownTargetX - thrownStartX) * u;
            worldY = thrownStartY + (thrownTargetY - thrownStartY) * u;
            if (singularityGrace <= 0.0f && distance < 26.0f * sizeScale + 6.0f) {
                HurtPlayer(playerHP, THROWN_SWARM_DAMAGE);
                alive = false;
                scored = true;      // vanished on impact: no kill reward
                noBlast = true;
                return true;
            }
            if (u >= 1.0f) {
                thrownPhase = 2;
                thrownTimer = 0.0f;
                // A miss is still dangerous: the landing sends out a shockwave.
                if (singularityGrace <= 0.0f && distance < THROWN_SWARM_BLAST_RADIUS)
                    HurtPlayer(playerHP, THROWN_SWARM_BLAST_DAMAGE);
            }
        } else if (thrownTimer >= THROWN_SWARM_STUN) {
            thrownPhase = 0;
        }
        // Keep sampling through the stun so the trail collapses onto the body.
        antaresTrailTimer += deltaTime;
        if (antaresTrailTimer >= 0.03f) {
            antaresTrailTimer = 0.0f;
            for (int i = ANTARES_TRAIL_COUNT - 1; i > 0; --i)
                antaresTrail[i] = antaresTrail[i - 1];
            antaresTrail[0] = glm::vec3(worldX, worldY, thrownSpin);
        }
        return false;
    }

    void BeginGenesisEgress(float dirX, float dirY) {
        const float length = std::sqrt(dirX * dirX + dirY * dirY);
        if (length <= 0.0001f) return;
        genesisEgressX = dirX / length;
        genesisEgressY = dirY / length;
        genesisEgressTimer = 0.0f;
        genesisEgress = true;
    }

    void TryContact(float distance, float& playerHP, float dps,
                    float deltaTime, float threshold = -1.0f,
                    float gateWX = -1.0f, float gateWY = -1.0f,
                    float gateWW = -1.0f, float gateWH = -1.0f) const {
        if (singularityGrace > 0.0f) return;
        if (gateWW > 0.0f) {
            const float margin = 14.0f * sizeScale;
            if (worldX < gateWX - margin || worldX > gateWX + gateWW + margin ||
                worldY < gateWY - margin || worldY > gateWY + gateWH + margin)
                return;
        }
        const float contactRange = threshold >= 0.0f
            ? threshold : 26.0f * sizeScale;
        if (distance < contactRange)
            HurtPlayer(playerHP, dps * deltaTime);
    }

    void UpdateGimbal(float playerCX, float playerCY, float deltaTime,
                      float distance, float dx, float dy, float& playerHP,
                      float speedMult, std::vector<Bullet>* bullets,
                      float gateWX, float gateWY,
                      float gateWW, float gateWH) {
        static constexpr float PREFERRED_DISTANCE = GIMBAL_PREFERRED_DISTANCE;
        static constexpr float DISTANCE_BAND = 80.0f;
        static constexpr float IDLE_ROT = 0.40f;
        static constexpr float CHARGE_ROT_MAX = 9.0f;
        static constexpr float SHOT_DELAY = 0.08f;
        static constexpr float BURST_END = SHOT_DELAY + 0.35f;
        static constexpr float SHOT_COOLDOWN = 2.85f;

        if (!gimbalCharging && !gimbalBursting) {
            gimbalVisualAngle += IDLE_ROT * deltaTime;
            gimbalAttackTimer -= deltaTime;
            if (gimbalAttackTimer <= 0.0f) {
                gimbalCharging = true;
                gimbalChargeAngle = 0.0f;
                gimbalChargeSpeed = IDLE_ROT;
            }
        }
        if (gimbalCharging) {
            gimbalChargeSpeed += (CHARGE_ROT_MAX - gimbalChargeSpeed)
                               * 5.0f * deltaTime;
            const float da = gimbalChargeSpeed * deltaTime;
            gimbalVisualAngle += da;
            gimbalChargeAngle += da;
            if (gimbalChargeAngle >= GIMBAL_CHARGE_ROTATIONS * 6.2831853f) {
                // The lens flips on its pin axis; settle it face-on (a half
                // turn period) so the shot leaves a fully open lens.
                const float period = 3.1415927f;
                float mod = fmodf(gimbalVisualAngle, period);
                if (mod < 0.0f) mod += period;
                if (mod > period * 0.5f) gimbalVisualAngle += period - mod;
                else gimbalVisualAngle -= mod;
                gimbalCharging = false;
                gimbalBursting = true;
                gimbalBurstTimer = 0.0f;
                gimbalFired = false;
            }
        } else if (gimbalBursting) {
            gimbalBurstTimer += deltaTime;
            if (!gimbalFired && gimbalBurstTimer >= SHOT_DELAY) {
                if (bullets) {
                    Bullet shot(worldX, worldY, playerCX, playerCY);
                    shot.isEnemy = true;
                    shot.homing = false;
                    shot.speed = 360.0f;
                    shot.enemyDmg = 8.0f;
                    shot.sizeScale = 0.95f;
                    shot.color = glm::vec3(0.70f, 1.0f, 0.36f);
                    bullets->push_back(shot);
                }
                gimbalFired = true;
            }
            if (gimbalBurstTimer >= BURST_END) {
                gimbalBursting = false;
                gimbalBurstTimer = 0.0f;
                gimbalChargeAngle = 0.0f;
                gimbalChargeSpeed = IDLE_ROT;
                gimbalAttackTimer = SHOT_COOLDOWN + (float)(rand() % 70) * 0.01f;
            }
        }

        const float moveSpeed = std::min(speed * speedMult, 112.0f);
        float moveX = 0.0f, moveY = 0.0f, rate = 0.0f;
        if (distance > PREFERRED_DISTANCE + DISTANCE_BAND) {
            moveX = dx / distance; moveY = dy / distance;
            rate = moveSpeed;
        } else if (distance < PREFERRED_DISTANCE - DISTANCE_BAND) {
            moveX = -dx / distance; moveY = -dy / distance;
            rate = std::min(moveSpeed * 0.68f, 72.0f);
        } else {
            moveX = -dy / distance * gimbalOrbitSign;
            moveY = dx / distance * gimbalOrbitSign;
            rate = std::min(moveSpeed * 0.34f, 34.0f);
        }
        worldX += moveX * rate * deltaTime;
        worldY += moveY * rate * deltaTime;
        TryContact(distance, playerHP, contactDmg, deltaTime, -1.0f,
                   gateWX, gateWY, gateWW, gateWH);
    }

    void UpdateAntares(float deltaTime, float distance, float dx, float dy,
                       float& playerHP, float speedMult,
                       float gateWX, float gateWY, float gateWW, float gateWH) {
        static constexpr float ACCEL = 128.0f;           // 60 -> 700 px/s in 5s
        static constexpr float TURN_SLOW = 3.1416f;      // 180 deg/s at stage 1
        static constexpr float TURN_FAST = 0.6981f;      //  40 deg/s at top speed
        static constexpr float SPIN_SLOW = 0.5236f;      //  30 deg/s
        static constexpr float SPIN_FAST = 9.4248f;      // 540 deg/s
        static constexpr float PASS_RANGE = 320.0f;
        static constexpr float FLYOFF_RANGE = 700.0f;    // a wide miss also ends the run
        static constexpr float SKID_TIME = 0.45f;
        static constexpr float IMPACT_DAMAGE = 12.0f;    // at top speed
        static constexpr float KNOCK_PER_SPEED = 0.55f;  // 33px at 60, 385px at 700

        const float toX = dx / distance, toY = dy / distance;
        if (antaresHeadingX == 0.0f && antaresHeadingY == 0.0f) {
            antaresHeadingX = toX;
            antaresHeadingY = toY;
        }
        antaresStateTimer += deltaTime;
        const float facing = antaresHeadingX * toX + antaresHeadingY * toY;

        if (antaresState == AntaresState::CHASE) {
            const float ratio = AntaresSpeedRatio();
            // Faster means wider turns, so a sidestep makes it overshoot.
            const float current = atan2f(antaresHeadingY, antaresHeadingX);
            float turn = atan2f(toY, toX) - current;
            while (turn > 3.14159265f) turn -= 6.28318531f;
            while (turn < -3.14159265f) turn += 6.28318531f;
            const float maxTurn = (TURN_SLOW + (TURN_FAST - TURN_SLOW) * ratio)
                                * deltaTime;
            const float next = current + std::clamp(turn, -maxTurn, maxTurn);
            antaresHeadingX = cosf(next);
            antaresHeadingY = sinf(next);
            antaresMoveSpeed = std::min(ANTARES_MAX_SPEED,
                                        antaresMoveSpeed + ACCEL * deltaTime);

            // Touching the player is a ram at any speed: damage and knockback
            // scale with speed, and the impact stuns Antares instead of
            // letting the two bodies overlap.
            if (singularityGrace <= 0.0f && distance < AntaresRamRange()) {
                HurtPlayer(playerHP, IMPACT_DAMAGE * antaresMoveSpeed /
                                     ANTARES_MAX_SPEED);
                const float knock = KNOCK_PER_SPEED * antaresMoveSpeed;
                antaresKnockX += toX * knock;
                antaresKnockY += toY * knock;
                antaresState = AntaresState::STUNNED;
                antaresStateTimer = 0.0f;
                antaresMoveSpeed = 0.0f;
            } else if (antaresMoveSpeed >= ANTARES_PASS_SPEED) {
                // At 500 px/s or more, the player crossing behind it nearby,
                // or a wide miss carrying it far away, means it ran past:
                // skid straight ahead and fall back to stage 1.
                const bool crossed = antaresPrevFacing > 0.0f && facing <= 0.0f &&
                                     distance < PASS_RANGE;
                const bool flewOff = facing < 0.0f && distance > FLYOFF_RANGE;
                if (crossed || flewOff) {
                    antaresState = AntaresState::SKID;
                    antaresStateTimer = 0.0f;
                    antaresSkidDecel = (antaresMoveSpeed - ANTARES_MIN_SPEED) / SKID_TIME;
                }
            }
        } else if (antaresState == AntaresState::SKID) {
            antaresMoveSpeed = std::max(ANTARES_MIN_SPEED,
                                        antaresMoveSpeed - antaresSkidDecel * deltaTime);
            if (antaresStateTimer >= SKID_TIME) {
                antaresState = AntaresState::REST;
                antaresStateTimer = 0.0f;
                antaresMoveSpeed = 0.0f;
            }
        } else {
            // STUNNED and REST hold still, then turn to the player and start
            // a fresh run from stage 1.
            const float hold = antaresState == AntaresState::STUNNED
                ? ANTARES_STUN_TIME : ANTARES_REST_TIME;
            antaresMoveSpeed = 0.0f;
            if (antaresStateTimer >= hold) {
                antaresState = AntaresState::CHASE;
                antaresStateTimer = 0.0f;
                antaresMoveSpeed = ANTARES_MIN_SPEED;
                antaresHeadingX = toX;
                antaresHeadingY = toY;
            }
        }
        antaresPrevFacing = antaresHeadingX * toX + antaresHeadingY * toY;

        // Spin tracks speed, so the frame visibly winds up and down with it.
        antaresSpinSpeed = SPIN_SLOW + (SPIN_FAST - SPIN_SLOW) * AntaresSpeedRatio();
        antaresVisualAngle += antaresSpinSpeed * deltaTime;

        const float step = antaresMoveSpeed * speedMult * deltaTime;
        worldX += antaresHeadingX * step;
        worldY += antaresHeadingY * step;

        antaresTrailTimer += deltaTime;
        // Fixed-interval samples space the ghosts by distance travelled, so a
        // faster run draws a longer, more spread-out trail.
        if (antaresTrailTimer >= 0.04f) {
            antaresTrailTimer = 0.0f;
            for (int i = ANTARES_TRAIL_COUNT - 1; i > 0; --i)
                antaresTrail[i] = antaresTrail[i - 1];
            antaresTrail[0] = glm::vec3(worldX, worldY, antaresVisualAngle);
        }

        TryContact(distance, playerHP, contactDmg, deltaTime, -1.0f,
                   gateWX, gateWY, gateWW, gateWH);
    }

    void Update(float playerCX, float playerCY, float deltaTime,
                float& playerHP, float speedMult = 1.0f,
                float gateWX = -1.0f, float gateWY = -1.0f,
                float gateWW = -1.0f, float gateWH = -1.0f,
                std::vector<Bullet>* bullets = nullptr) {
        if (!alive) return;
        hitFlashTimer = std::max(0.0f, hitFlashTimer - deltaTime);
        magnetarInterceptTimer = std::max(0.0f,
                                          magnetarInterceptTimer - deltaTime);
        for (float& flash : magnetarFaceHitFlash)
            flash = std::max(0.0f, flash - deltaTime);
        regulusShield.flashTimer = std::max(0.0f,
                                            regulusShield.flashTimer - deltaTime);
        regulusShield.grantTimer = std::max(0.0f,
                                            regulusShield.grantTimer - deltaTime);
        if (kind == MobKind::REGULUS && regulusShieldRegenTimer > 0.0f) {
            regulusShieldRegenTimer = std::max(0.0f,
                                               regulusShieldRegenTimer - deltaTime);
            if (regulusShieldRegenTimer <= 0.0f) {
                regulusShield.hp = regulusShield.maxHp;
                regulusShield.flashTimer = MOB_HIT_FLASH_TIME;
            }
        }
        if (singularityGrace > 0.0f)
            singularityGrace = std::max(0.0f, singularityGrace - deltaTime);
        if (burnTimer > 0.0f) {
            ApplyDamage(burnDps * deltaTime);
            burnTimer -= deltaTime;
            if (burnTimer <= 0.0f) { burnTimer = 0.0f; burnDps = 0.0f; }
            if (!alive) return;
        }

        const float dx = playerCX - worldX;
        const float dy = playerCY - worldY;
        const float distance = std::sqrt(dx * dx + dy * dy) + 1e-4f;

        // A thrown Swarm neither chases nor deals contact damage until its
        // flight and landing stun are over.
        if (thrownPhase != 0) {
            UpdateThrownSwarm(deltaTime, distance, playerHP);
            return;
        }

        if (kind == MobKind::MAGNETAR) {
            magnetarVisualAngle += deltaTime * 0.72f;
            {
                // The N pole swings toward the player at a limited rate.
                float turn = atan2f(dy, dx) - magnetarFacing;
                while (turn > 3.14159265f) turn -= 6.28318531f;
                while (turn < -3.14159265f) turn += 6.28318531f;
                const float maxTurn = 3.0f * deltaTime;
                magnetarFacing += std::clamp(turn, -maxTurn, maxTurn);
            }
            // Broken faces restore on their own, MAGNETAR_FACE_RESTORE after
            // they broke.
            for (int f = 0; f < MAGNETAR_FACES; ++f) {
                magnetarFaceRestored[f] += deltaTime;
                if (magnetarFaceBroken[f] < 0.0f) continue;
                magnetarFaceBroken[f] += deltaTime;
                if (magnetarFaceBroken[f] >= MAGNETAR_FACE_RESTORE) {
                    magnetarFaceBroken[f] = -1.0f;
                    magnetarFaceHits[f] = MAGNETAR_FACE_HITS;
                    magnetarFaceRestored[f] = 0.0f;
                }
            }
            if (bullets) {
                const float fieldRadiusSq = MAGNETAR_FIELD_RADIUS *
                                            MAGNETAR_FIELD_RADIUS;
                for (auto& bullet : *bullets) {
                    if (!bullet.active || bullet.isEnemy ||
                        bullet.wardPassed == this) continue;
                    const float sx = bullet.x - bullet.prevX;
                    const float sy = bullet.y - bullet.prevY;
                    const float segLenSq = sx * sx + sy * sy;
                    float t = segLenSq > 0.001f
                        ? ((worldX - bullet.prevX) * sx +
                           (worldY - bullet.prevY) * sy) / segLenSq : 0.0f;
                    t = std::max(0.0f, std::min(1.0f, t));
                    const float nearestX = bullet.prevX + sx * t;
                    const float nearestY = bullet.prevY + sy * t;
                    const float bx = nearestX - worldX;
                    const float by = nearestY - worldY;
                    if (bx * bx + by * by > fieldRadiusSq) continue;
                    // Hit point on the rim, on the side the shot came from.
                    float ex = bullet.prevX - worldX, ey = bullet.prevY - worldY;
                    if (ex * ex + ey * ey < 1.0f) { ex = bx; ey = by; }
                    const float hitA = atan2f(ey, ex);
                    // Only the face on that side decides: a broken one lets
                    // the shot through (and it is not checked again).
                    const int face = MagnetarFaceToward(hitA);
                    if (magnetarFaceBroken[face] >= 0.0f) {
                        bullet.wardPassed = this;
                        continue;
                    }
                    bullet.active = false;
                    magnetarFaceHitFlash[face] = MAGNETAR_HIT_FLASH;
                    if (--magnetarFaceHits[face] <= 0) {
                        magnetarFaceHits[face] = 0;
                        magnetarFaceBroken[face] = 0.0f;
                    }
                    const float nx = cosf(hitA), ny = sinf(hitA);
                    magnetarInterceptX = worldX + nx * MAGNETAR_FIELD_RADIUS;
                    magnetarInterceptY = worldY + ny * MAGNETAR_FIELD_RADIUS;
                    magnetarInterceptTimer = 0.16f;
                }
            }
            worldX += dx / distance * speed * speedMult * deltaTime;
            worldY += dy / distance * speed * speedMult * deltaTime;
            TryContact(distance, playerHP, contactDmg, deltaTime,
                       -1.0f, gateWX, gateWY, gateWW, gateWH);
            return;
        }

        if (kind == MobKind::GRAVIS) {
            gravisVisualAngle += deltaTime * 0.72f;
            gravisDriftTimer -= deltaTime;
            if (gravisDriftTimer <= 0.0f) {
                gravisDriftAngle += ((float)(rand() % 101) - 50.0f) * 0.012f;
                gravisDriftTimer = 1.2f + (float)(rand() % 140) * 0.01f;
            }
            worldX += cosf(gravisDriftAngle) * speed * speedMult * deltaTime;
            worldY += sinf(gravisDriftAngle) * speed * speedMult * deltaTime;
            TryContact(distance, playerHP, contactDmg, deltaTime);
            return;
        }

        if (kind == MobKind::QUASAR) {
            static constexpr float COOLDOWN_TIME = QUASAR_COOLDOWN_TIME;
            static constexpr float LOCK_TIME = QUASAR_LOCK_TIME;
            static constexpr float BEAM_TIME = 2.50f;
            static constexpr float RECOVER_TIME = 1.4f;
            static constexpr float BEAM_LENGTH = 6000.0f;
            static constexpr float BEAM_RADIUS = QUASAR_BEAM_RADIUS;
            static constexpr float BEAM_DPS = QUASAR_BEAM_DPS;

            quasarTimer += deltaTime;
            quasarVisualAngle += deltaTime * 0.24f;
            quasarShellSpin += deltaTime * 2.2f * (1.0f + 2.0f * QuasarHeat());
            if (quasarState == 0) {
                quasarMoveTimer -= deltaTime;
                if (quasarMoveTimer <= 0.0f) {
                    quasarMoveTurn = ((float)(rand() % 121) - 60.0f) * 0.01f;
                    quasarMoveTimer = 0.8f + (float)(rand() % 101) * 0.01f;
                }
                // Drift until the wind-up starts, then hold position. The
                // axis is chosen once, at the start of the wind-up, and stays
                // put through the wind-up and the lock (1.7s of fixed
                // telegraph) so the band never chases the player.
                if (QuasarPrecharge() <= 0.0f) {
                    quasarAimSet = false;
                    quasarMoveAngle += quasarMoveTurn * deltaTime;
                    worldX += cosf(quasarMoveAngle) * speed * speedMult * deltaTime;
                    worldY += sinf(quasarMoveAngle) * speed * speedMult * deltaTime;
                } else if (!quasarAimSet) {
                    if (distance > 0.001f) {
                        quasarAimX = dx / distance;
                        quasarAimY = dy / distance;
                    }
                    quasarAimSet = true;
                }
                if (quasarTimer >= COOLDOWN_TIME) {
                    quasarState = 1;
                    quasarTimer = 0.0f;
                }
            } else if (quasarState == 1) {
                // Lock. The aim was chosen at the start of the wind-up; only
                // if that was skipped (e.g. a forced state) is it taken now.
                if (!quasarAimSet) {
                    if (distance > 0.001f) {
                        quasarAimX = dx / distance;
                        quasarAimY = dy / distance;
                    }
                }
                quasarAimSet = true;
                quasarState = 2;
                quasarTimer = 0.0f;
            } else if (quasarState == 2) {
                if (quasarTimer >= LOCK_TIME) {
                    quasarState = 3;
                    quasarTimer = 0.0f;
                }
            } else if (quasarState == 3) {
                // Very slow turn toward the player while firing. The axis
                // repeats every pi (both ends fire), so the gap is folded into
                // [-pi/2, pi/2] and the nearer end swings onto the player; the
                // step is capped so it stops on the player instead of jittering.
                if (distance > 0.001f) {
                    const float aim = atan2f(quasarAimY, quasarAimX);
                    float gap = atan2f(dy, dx) - aim;
                    gap -= 3.1415927f * floorf(gap / 3.1415927f + 0.5f);
                    const float maxStep = QUASAR_BEAM_TURN * deltaTime;
                    const float next = aim + std::clamp(gap, -maxStep, maxStep);
                    quasarAimX = cosf(next);
                    quasarAimY = sinf(next);
                }
                const float growT = std::min(1.0f, quasarTimer / 0.70f);
                const float reach = BEAM_LENGTH * growT * growT * growT;
                const float ax = worldX - quasarAimX * reach;
                const float ay = worldY - quasarAimY * reach;
                const float bx = worldX + quasarAimX * reach;
                const float by = worldY + quasarAimY * reach;
                const float abx = bx - ax, aby = by - ay;
                const float length2 = abx * abx + aby * aby;
                if (length2 > 0.001f) {
                    float u = ((playerCX - ax) * abx + (playerCY - ay) * aby) / length2;
                    u = std::max(0.0f, std::min(1.0f, u));
                    const float hx = ax + abx * u - playerCX;
                    const float hy = ay + aby * u - playerCY;
                    if (hx * hx + hy * hy <= BEAM_RADIUS * BEAM_RADIUS)
                        HurtPlayer(playerHP, BEAM_DPS * deltaTime);
                }
                if (quasarTimer >= BEAM_TIME) {
                    quasarState = 4;
                    quasarTimer = 0.0f;
                }
            } else if (quasarTimer >= RECOVER_TIME) {
                quasarState = 0;
                quasarTimer = 0.0f;
            }
            TryContact(distance, playerHP, contactDmg, deltaTime, -1.0f,
                       gateWX, gateWY, gateWW, gateWH);
            return;
        }

        if (kind == MobKind::GIMBAL) {
            UpdateGimbal(playerCX, playerCY, deltaTime, distance, dx, dy,
                         playerHP, speedMult, bullets,
                         gateWX, gateWY, gateWW, gateWH);
            return;
        }

        if (kind == MobKind::ANTARES) {
            UpdateAntares(deltaTime, distance, dx, dy, playerHP, speedMult,
                          gateWX, gateWY, gateWW, gateWH);
            return;
        }

        if (kind == MobKind::REGULUS) {
            regulusVisualAngle += deltaTime * 0.32f;
            regulusDriftTimer -= deltaTime;
            if (regulusDriftTimer <= 0.0f) {
                regulusDriftAngle += ((float)(rand() % 201) - 100.0f) * 0.012f;
                regulusDriftTimer = 0.8f + (float)(rand() % 150) * 0.01f;
            }
            const float movementSpeed = std::min(speed * speedMult, 90.0f);
            worldX += cosf(regulusDriftAngle) * movementSpeed * deltaTime;
            worldY += sinf(regulusDriftAngle) * movementSpeed * deltaTime;
            return;
        }

        if (genesisEgress) {
            genesisEgressTimer += deltaTime;
            const float egressSpeed = speed * 1.55f * speedMult;
            worldX += genesisEgressX * egressSpeed * deltaTime;
            worldY += genesisEgressY * egressSpeed * deltaTime;
            if (genesisEgressTimer >= 0.62f) {
                genesisEgressTimer = 0.0f;
                genesisEgress = false;
            }
            TryContact(distance, playerHP, contactDmg, deltaTime, -1.0f,
                       gateWX, gateWY, gateWW, gateWH);
            return;
        }

        if (kind == MobKind::GENESIS) {
            const float ax = spawnAnchorX - worldX;
            const float ay = spawnAnchorY - worldY;
            const float anchorDistance = std::sqrt(ax * ax + ay * ay);
            if (anchorDistance > 12.0f) {
                anchored = false;
                const float step = std::min(anchorDistance, speed * speedMult * deltaTime);
                worldX += (ax / anchorDistance) * step;
                worldY += (ay / anchorDistance) * step;
            } else {
                worldX = spawnAnchorX;
                worldY = spawnAnchorY;
                anchored = true;
            }
            TryContact(distance, playerHP, contactDmg, deltaTime, -1.0f,
                       gateWX, gateWY, gateWW, gateWH);
            return;
        }

        if (distance > 5.0f) {
            worldX += (dx / distance) * speed * speedMult * deltaTime;
            worldY += (dy / distance) * speed * speedMult * deltaTime;
        }
        TryContact(distance, playerHP, contactDmg, deltaTime, -1.0f,
                   gateWX, gateWY, gateWW, gateWH);
    }
};
