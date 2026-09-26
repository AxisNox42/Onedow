#pragma once

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include <glm/glm.hpp>

#include "PlayerStats.h"

// The playable roster contains only the six live signals: Rotor (ROTOR),
// Genesis (GENESIS), Swarm (SWARM), Gravis, Quasar, and the separate Scope
// ranged mob. Retired mutation variants have no runtime type anymore.
enum class MobKind {
    ROTOR,
    GENESIS,
    SWARM,
    GRAVIS,
    QUASAR,
};

inline void MobKillReward(MobKind kind, float& xpBase, float& scoreBase) {
    xpBase = 1.0f;
    scoreBase = 100.0f;
    switch (kind) {
    case MobKind::GENESIS: xpBase = 7.0f;  scoreBase = 300.0f; break;
    case MobKind::SWARM:    xpBase = 0.12f; scoreBase = 12.0f;  break;
    case MobKind::GRAVIS:  xpBase = 11.0f; scoreBase = 520.0f; break;
    case MobKind::QUASAR:  xpBase = 9.0f;  scoreBase = 480.0f; break;
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
    default:               return 0;
    }
}

inline float MobRewardMult(MobKind kind) {
    return kind == MobKind::SWARM ? 0.20f : 1.0f;
}

inline float MobXpBonus(MobKind kind, const PlayerStats& stats) {
    float bonus = (float)stats.mobXpBonus;
    const int index = (int)kind;
    if (index >= 0 && index < PlayerStats::MOB_KIND_XP_SLOTS)
        bonus += (float)stats.mobKindXpBonus[index];
    return bonus;
}

class Monster {
public:
    float worldX = 0.0f;
    float worldY = 0.0f;
    float speed = 150.0f;
    float hp = 90.0f;
    bool alive = true;
    bool exploded = false;
    bool scored = false;
    bool noBlast = false;
    bool summoned = false;
    glm::vec3 color = glm::vec3(1.0f, 0.27f, 0.0f);

    MobKind kind = MobKind::ROTOR;
    float sizeScale = 1.0f;
    float contactDmg = 5.0f;
    float singularityGrace = 0.0f;

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

    // The optional legacy parameters are harmless call-site compatibility
    // for previews and Genesis children; they do not select retired variants.
    void MakeKind(MobKind requested, int /*unusedGeneration*/ = 0,
                  float scale = 1.0f) {
        kind = requested;
        sizeScale = scale;
        if (kind == MobKind::GENESIS) {
            color = glm::vec3(0.2f, 0.75f, 0.55f);
            hp *= 3.5f;
            speed *= 0.35f;
            sizeScale = scale * 2.2f;
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
            sizeScale = scale;
        } else if (kind == MobKind::GRAVIS) {
            color = glm::vec3(0.58f, 0.42f, 1.0f);
            hp *= 8.0f;
            speed *= 0.24f;
            sizeScale = scale * 2.65f;
            contactDmg = 6.0f;
            gravisVisualAngle = (float)(rand() % 628) * 0.01f;
            gravisDriftAngle = (float)(rand() % 628) * 0.01f;
            gravisDriftTimer = 1.0f + (float)(rand() % 120) * 0.01f;
        } else if (kind == MobKind::QUASAR) {
            color = glm::vec3(0.42f, 0.56f, 1.0f);
            hp *= 6.4f;
            speed *= 0.52f;
            sizeScale = scale * 3.15f;
            contactDmg = 4.0f;
            quasarState = 0;
            quasarTimer = (float)(rand() % 120) * 0.01f;
            quasarVisualAngle = (float)(rand() % 628) * 0.01f;
            quasarMoveAngle = (float)(rand() % 628) * 0.01f;
            quasarMoveTurn = ((float)(rand() % 121) - 60.0f) * 0.01f;
            quasarMoveTimer = 0.8f + (float)(rand() % 101) * 0.01f;
        }
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

    void Update(float playerCX, float playerCY, float deltaTime,
                float& playerHP, float speedMult = 1.0f,
                float gateWX = -1.0f, float gateWY = -1.0f,
                float gateWW = -1.0f, float gateWH = -1.0f) {
        if (!alive) return;
        if (singularityGrace > 0.0f)
            singularityGrace = std::max(0.0f, singularityGrace - deltaTime);
        if (burnTimer > 0.0f) {
            hp -= burnDps * deltaTime;
            burnTimer -= deltaTime;
            if (burnTimer <= 0.0f) { burnTimer = 0.0f; burnDps = 0.0f; }
            if (hp <= 0.0f) { hp = 0.0f; alive = false; return; }
        }

        const float dx = playerCX - worldX;
        const float dy = playerCY - worldY;
        const float distance = std::sqrt(dx * dx + dy * dy) + 1e-4f;

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
            if (!anchored) {
                const float ax = spawnAnchorX - worldX;
                const float ay = spawnAnchorY - worldY;
                const float anchorDistance = std::sqrt(ax * ax + ay * ay);
                if (anchorDistance > 12.0f) {
                    worldX += (ax / anchorDistance) * speed * speedMult * deltaTime;
                    worldY += (ay / anchorDistance) * speed * speedMult * deltaTime;
                } else {
                    anchored = true;
                }
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
