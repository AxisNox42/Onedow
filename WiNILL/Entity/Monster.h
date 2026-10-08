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

class RangedMob;
class Monster;

struct RegulusLink {
    Monster* monsterTarget = nullptr;
    RangedMob* rangedTarget = nullptr;
    float cooldown = 0.0f;
};

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

inline void MobKillReward(MobKind kind, float& xpBase, float& scoreBase) {
    xpBase = 1.0f;
    scoreBase = 100.0f;
    switch (kind) {
    case MobKind::ROTOR:   xpBase = 2.0f;  scoreBase = 100.0f; break;
    case MobKind::GIMBAL:  xpBase = 4.0f;  scoreBase = 220.0f; break;
    case MobKind::GENESIS: xpBase = 10.0f; scoreBase = 300.0f; break;
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
    case MobKind::GENESIS: return 15;
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

inline long long MobExperienceReward(MobKind kind, const PlayerStats& stats) {
    float xpBase, scoreBase;
    MobKillReward(kind, xpBase, scoreBase);
    // Currency/score reductions must not turn a tier-1 kill into zero XP.
    return (long long)((xpBase + MobXpBonus(kind, stats)) * stats.xpMult);
}

class Monster {
public:
    static constexpr float GIMBAL_CHARGE_ROTATIONS = 2.0f;
    static constexpr float GIMBAL_PREFERRED_DISTANCE = 700.0f;
    static constexpr float REGULUS_BASE_HP = 360.0f;
    static constexpr float REGULUS_LINK_RANGE = 800.0f;
    static constexpr float REGULUS_LINK_COOLDOWN = 3.0f;
    static constexpr float REGULUS_SHIELD_REGEN_DELAY = 8.0f;
    static constexpr int REGULUS_LINK_CAPACITY = 8;
    static constexpr float ANTARES_BASE_HP = 600.0f;
    static constexpr int ANTARES_TRAIL_COUNT = 6;
    static constexpr float MAGNETAR_BASE_HP = 450.0f;
    static constexpr int MAGNETAR_MAX_STACKS = 8;
    static constexpr float MAGNETAR_FIELD_RADIUS = 145.0f;

    // Antares is a bull: it keeps accelerating while it chases, turns worse
    // the faster it goes, and drops back to stage 1 once it overshoots.
    // Ramming the player knocks them away and stuns Antares; a finished run
    // (skid) ends in a short rest.
    enum class AntaresState { CHASE, SKID, STUNNED, REST };
    static constexpr float ANTARES_STUN_TIME = 0.6f;
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
    std::array<RegulusLink, REGULUS_LINK_CAPACITY> regulusLinks{};

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

    // Quasar state.
    int quasarState = 0;
    float quasarTimer = 0.0f;
    float quasarAimX = 1.0f;
    float quasarAimY = 0.0f;
    float quasarVisualAngle = 0.0f;
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
    float magnetarStacks = (float)MAGNETAR_MAX_STACKS;
    float magnetarRegenRate = 0.3f;
    float magnetarVisualAngle = 0.0f;
    float magnetarInterceptTimer = 0.0f;
    float magnetarInterceptX = 0.0f;
    float magnetarInterceptY = 0.0f;

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
            color = glm::vec3(0.2f, 0.75f, 0.55f);
            hp *= 3.5f;
            speed *= 0.35f;
            sizeScale = 2.2f;
            spawnTimer = 0.0f;
            hivePhase = 0;
            hiveOpenFactor = 0.0f;
            hiveSpawned = false;
            hiveSpawnCount = 0;
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
            sizeScale = 2.65f;
            contactDmg = 6.0f;
            gravisVisualAngle = (float)(rand() % 628) * 0.01f;
            gravisDriftAngle = (float)(rand() % 628) * 0.01f;
            gravisDriftTimer = 1.0f + (float)(rand() % 120) * 0.01f;
        } else if (kind == MobKind::QUASAR) {
            color = glm::vec3(0.42f, 0.56f, 1.0f);
            hp *= 6.4f;
            speed *= 0.52f;
            sizeScale = 3.15f;
            contactDmg = 4.0f;
            quasarState = 0;
            quasarTimer = (float)(rand() % 120) * 0.01f;
            quasarVisualAngle = (float)(rand() % 628) * 0.01f;
            quasarMoveAngle = (float)(rand() % 628) * 0.01f;
            quasarMoveTurn = ((float)(rand() % 121) - 60.0f) * 0.01f;
            quasarMoveTimer = 0.8f + (float)(rand() % 101) * 0.01f;
        } else if (kind == MobKind::GIMBAL) {
            color = glm::vec3(0.20f, 1.0f, 0.72f);
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
            sizeScale = 1.50f;
            contactDmg = 0.0f;
            regulusBaseHp = hp;
            regulusShield = {};
            regulusShield.maxHp = regulusBaseHp * 0.9f;
            regulusShield.hp = regulusShield.maxHp;
            regulusShieldRegenTimer = 0.0f;
            regulusVisualAngle = (float)(rand() % 628) * 0.01f;
            regulusDriftAngle = (float)(rand() % 628) * 0.01f;
            regulusDriftTimer = 0.8f + (float)(rand() % 120) * 0.01f;
            regulusLinks = {};
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
        } else if (kind == MobKind::MAGNETAR) {
            color = glm::vec3(1.0f, 0.84f, 0.28f);
            hp *= MAGNETAR_BASE_HP / 90.0f;
            speed *= 0.78f;
            sizeScale = 1.45f;
            contactDmg = 5.0f;
            magnetarStacks = (float)MAGNETAR_MAX_STACKS;
            magnetarRegenRate = 0.3f;
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
                             : kind == MobKind::QUASAR ? 0.16f : 0.0f;
        return deathEffectTimer < duration;
    }

    void UpdateDeathEffect(float delta) {
        if (!alive && DeathEffectVisible())
            deathEffectTimer += std::max(0.0f, delta);
    }

    void SpawnGenesisDeathEffect(float base, bool reduced) const {
        const float radius = MobDeathParticleRadius(base);
        EnemyParticleBurst burst;
        burst.count = 24; burst.directions = 6; burst.life = 0.45f;
        burst.phase = hiveOrbitAngle - 0.5235988f;
        burst.startRadius = 0.10f; burst.jitter = 0.42f;
        burst.speedMin = radius * 3.5f; burst.speedMax = radius * 5.2f;
        burst.sizeMin = 4.0f; burst.sizeMax = 14.0f; burst.colorLift = 0.55f;
        SpawnEnemyParticleBurst(worldX, worldY, color, radius, burst, reduced);
        burst.count = 48; burst.directions = 0; burst.life = 0.74f; burst.delay = 0.04f;
        burst.startRadius = 0.12f; burst.tangent = 0.16f;
        burst.speedMin = radius * 3.8f; burst.speedMax = radius * 5.3f;
        burst.sizeMin = 2.0f; burst.sizeMax = 7.0f; burst.colorLift = 0.30f;
        SpawnEnemyParticleBurst(worldX, worldY, color, radius, burst, reduced);
        SpawnMobDeathSparks(worldX, worldY, color, radius, 16, reduced);
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

    template <typename Line, typename Disc, typename Hexagon>
    void DrawQuasarDeathEffect(Line line, Disc disc, Hexagon hexagon) const {
        const float base = (summoned ? 28.0f : 18.0f) * sizeScale;
        const float glint = std::clamp(deathEffectTimer / 0.05f, 0.0f, 1.0f);
        const float fade = 1.0f - std::clamp((deathEffectTimer - 0.05f) / 0.11f, 0.0f, 1.0f);
        const float charge = quasarState == 1 ? 0.22f
            : (quasarState == 2 ? std::min(1.0f, quasarTimer / 0.55f)
               : (quasarState == 3 ? 1.0f : 0.0f));
        const float major = base * (1.32f + charge * 0.10f) * (1.0f - glint * 0.72f);
        const float minor = base * 0.42f * (1.0f - glint * 0.60f);
        const float spin = quasarVisualAngle + glint * 0.25f;
        const float ax = cosf(spin), ay = sinf(spin);
        const glm::vec3 bright = color + (glm::vec3(1.0f) - color) * (0.30f + glint * 0.65f);
        // Short body arcs only; the firing axis is absent from this recipe.
        for (int side = 0; side < 2; ++side) {
            for (int i = 0; i < 16; ++i) {
                const float a = 0.22f + (float)side * 3.1415927f + (float)i * 2.70f / 16.0f;
                const float b = a + 2.70f / 16.0f;
                line(worldX + ax * cosf(a) * major - ay * sinf(a) * minor,
                     worldY + ay * cosf(a) * major + ax * sinf(a) * minor,
                     worldX + ax * cosf(b) * major - ay * sinf(b) * minor,
                     worldY + ay * cosf(b) * major + ax * sinf(b) * minor,
                     1.8f, bright.r, bright.g, bright.b, fade);
            }
        }
        const float coreRadius = base * (0.30f - glint * 0.10f) * fade;
        disc(worldX, worldY, coreRadius * (1.8f + glint),
             bright.r, bright.g, bright.b, fade * 0.25f);
        hexagon(worldX, worldY, coreRadius, spin,
                bright.r, bright.g, bright.b, fade);
    }

    // Each recipe lives beside its mob; the renderer supplies batched primitives.
    template <typename Line, typename Disc, typename Hexagon>
    void DrawDeathEffect(Line line, Disc disc, Hexagon hexagon) const {
        if (!DeathEffectVisible()) return;
        switch (kind) {
        case MobKind::GRAVIS: DrawGravisDeathEffect(line, disc, hexagon); break;
        case MobKind::QUASAR: DrawQuasarDeathEffect(line, disc, hexagon); break;
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

    void SpawnQuasarDeathEffect(float base, bool reduced) const {
        const float radius = MobDeathParticleRadius(base);
        EnemyParticleBurst burst;
        burst.count = 12; burst.life = 0.18f; burst.phase = quasarVisualAngle;
        burst.startRadius = 0.18f; burst.sizeMin = 2.0f; burst.sizeMax = 4.0f;
        burst.speedMin = radius * 1.5f; burst.speedMax = radius * 3.0f;
        burst.colorLift = 0.22f;
        SpawnEnemyParticleBurst(worldX, worldY, color, radius, burst, reduced);
        // Fast, small blue-white specks in the original square-particle renderer.
        burst.count = 44; burst.life = 0.74f; burst.delay = 0.05f;
        burst.startRadius = 0.12f; burst.sizeMax = 10.0f; burst.colorLift = 0.50f;
        burst.speedMin = radius * 3.8f; burst.speedMax = radius * 5.3f;
        SpawnEnemyParticleBurst(worldX, worldY, color, radius, burst, reduced);
        SpawnMobDeathSparks(worldX, worldY, color, radius, 16, reduced);
    }

    void SpawnDeathEffect(float visualTime, bool reduced = false,
                           bool animateBody = true) {
        if (exploded) return;
        exploded = true;
        if (animateBody && (kind == MobKind::GRAVIS || kind == MobKind::QUASAR))
            deathEffectTimer = 0.0f;
        const float base = (summoned ? 28.0f : 18.0f) * sizeScale;
        switch (kind) {
        case MobKind::ROTOR: SpawnRotorDeathEffect(base, visualTime, reduced); break;
        case MobKind::SWARM: SpawnSwarmDeathEffect(base, visualTime, reduced); break;
        case MobKind::GENESIS: SpawnGenesisDeathEffect(base, reduced); break;
        case MobKind::GRAVIS: SpawnGravisDeathEffect(base, reduced); break;
        case MobKind::QUASAR: SpawnQuasarDeathEffect(base, reduced); break;
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
        // Include the rotating shell and node glow, with a small edge gap.
        const float margin = (summoned ? 28.0f : 18.0f) * sizeScale
                           * 1.35f * 1.75f + 8.0f;
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
                // Scope snaps its rotating layers into alignment before firing.
                // Gimbal has one ring, so align to the nearest quarter turn.
                const float period = 1.5707963f;
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
                    shot.color = glm::vec3(0.24f, 1.0f, 0.78f);
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
        regulusShield.flashTimer = std::max(0.0f,
                                            regulusShield.flashTimer - deltaTime);
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

        if (kind == MobKind::MAGNETAR) {
            magnetarVisualAngle += deltaTime * 0.72f;
            magnetarStacks = std::min((float)MAGNETAR_MAX_STACKS,
                magnetarStacks + std::max(0.0f, magnetarRegenRate) * deltaTime);
            if (bullets && magnetarStacks >= 1.0f) {
                const float fieldRadiusSq = MAGNETAR_FIELD_RADIUS *
                                            MAGNETAR_FIELD_RADIUS;
                for (auto& bullet : *bullets) {
                    if (!bullet.active || bullet.isEnemy ||
                        magnetarStacks < 1.0f) continue;
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
                    bullet.active = false;
                    magnetarStacks -= 1.0f;
                    magnetarInterceptX = nearestX;
                    magnetarInterceptY = nearestY;
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
            static constexpr float COOLDOWN_TIME = 3.2f;
            static constexpr float LOCK_TIME = 0.55f;
            static constexpr float BEAM_TIME = 2.50f;
            static constexpr float RECOVER_TIME = 1.4f;
            static constexpr float BEAM_LENGTH = 6000.0f;
            static constexpr float BEAM_RADIUS = 34.0f;
            static constexpr float BEAM_DPS = 34.0f;

            quasarTimer += deltaTime;
            quasarVisualAngle += deltaTime * 0.08f;
            auto turnTowardPlayer = [&](float radiansPerSecond) {
                float targetX = dx / distance;
                float targetY = dy / distance;
                if (quasarAimX * targetX + quasarAimY * targetY < 0.0f) {
                    targetX = -targetX;
                    targetY = -targetY;
                }
                const float current = atan2f(quasarAimY, quasarAimX);
                const float target = atan2f(targetY, targetX);
                float delta = target - current;
                while (delta > 3.14159265f) delta -= 6.28318531f;
                while (delta < -3.14159265f) delta += 6.28318531f;
                const float maxStep = radiansPerSecond * deltaTime;
                delta = std::max(-maxStep, std::min(maxStep, delta));
                const float next = current + delta;
                quasarAimX = cosf(next);
                quasarAimY = sinf(next);
            };

            if (quasarState == 0) {
                quasarMoveTimer -= deltaTime;
                if (quasarMoveTimer <= 0.0f) {
                    quasarMoveTurn = ((float)(rand() % 121) - 60.0f) * 0.01f;
                    quasarMoveTimer = 0.8f + (float)(rand() % 101) * 0.01f;
                }
                quasarMoveAngle += quasarMoveTurn * deltaTime;
                worldX += cosf(quasarMoveAngle) * speed * speedMult * deltaTime;
                worldY += sinf(quasarMoveAngle) * speed * speedMult * deltaTime;
                if (quasarTimer >= COOLDOWN_TIME) {
                    quasarState = 1;
                    quasarTimer = 0.0f;
                }
            } else if (quasarState == 1) {
                turnTowardPlayer(0.82f);
                const float laneDistance = fabsf(dx * quasarAimY - dy * quasarAimX);
                if (laneDistance <= BEAM_RADIUS * 0.85f) {
                    quasarState = 2;
                    quasarTimer = 0.0f;
                }
            } else if (quasarState == 2) {
                if (quasarTimer >= LOCK_TIME) {
                    quasarState = 3;
                    quasarTimer = 0.0f;
                }
            } else if (quasarState == 3) {
                const float sweepLinearSpeed = 260.0f;
                const float sweepAngularSpeed = std::max(0.035f,
                    std::min(0.18f, sweepLinearSpeed / std::max(distance, 180.0f)));
                turnTowardPlayer(sweepAngularSpeed);
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
