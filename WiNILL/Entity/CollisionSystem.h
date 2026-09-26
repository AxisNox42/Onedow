#pragma once

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>

#include "MonsterManager.h"
#include "Bullet.h"
#include "PlayerStats.h"
#include "Juice.h"

static inline float SegDistSq(float px, float py, float ax, float ay,
                              float bx, float by) {
    const float abx = bx - ax;
    const float aby = by - ay;
    const float len2 = abx * abx + aby * aby;
    float t = 0.0f;
    if (len2 > 1e-6f) {
        t = ((px - ax) * abx + (py - ay) * aby) / len2;
        t = std::max(0.0f, std::min(1.0f, t));
    }
    const float cx = ax + abx * t;
    const float cy = ay + aby * t;
    const float dx = px - cx;
    const float dy = py - cy;
    return dx * dx + dy * dy;
}

static inline float SegDist(float px, float py, float ax, float ay,
                            float bx, float by) {
    return std::sqrt(SegDistSq(px, py, ax, ay, bx, by));
}

static inline bool RicochetTo(Bullet& bullet, float fromX, float fromY,
                              MonsterManager& manager) {
    float bestD2 = 200.0f * 200.0f;
    float targetX = 0.0f;
    float targetY = 0.0f;
    bool found = false;
    auto consider = [&](float x, float y) {
        const float dx = x - fromX;
        const float dy = y - fromY;
        const float d2 = dx * dx + dy * dy;
        if (d2 > 4.0f && d2 < bestD2) {
            bestD2 = d2;
            targetX = x;
            targetY = y;
            found = true;
        }
    };
    for (auto* monster : manager.monsters)
        if (monster->alive) consider(monster->worldX, monster->worldY);
    for (auto* ranged : manager.rangedMobs)
        if (ranged->alive) consider(ranged->worldX, ranged->worldY);
    if (!found) return false;
    const float dx = targetX - bullet.x;
    const float dy = targetY - bullet.y;
    const float length = std::sqrt(dx * dx + dy * dy);
    if (length < 1.0f) return false;
    bullet.dirX = dx / length;
    bullet.dirY = dy / length;
    bullet.prevX = bullet.x;
    bullet.prevY = bullet.y;
    return true;
}

static inline float CritRoll(const PlayerStats& stats, bool& isCrit) {
    if (stats.critChance > 0 && (rand() % 100) < stats.critChance) {
        isCrit = true;
        return stats.critMult;
    }
    isCrit = false;
    return 1.0f;
}

class CollisionSystem {
public:
    static bool Update(float playerCX, float playerCY,
                       MonsterManager& manager, std::vector<Bullet>& bullets,
                       float& playerHP, float& scoreAccum, long long& score,
                       PlayerStats& stats, long long& /*xp*/) {
        bool playerHit = false;

        for (auto& bullet : bullets) {
            if (!bullet.active) continue;
            if (bullet.isEnemy) {
                const float hitRadius = 10.0f * stats.playerSizeMult;
                if (SegDistSq(playerCX, playerCY, bullet.prevX, bullet.prevY,
                              bullet.x, bullet.y) < hitRadius * hitRadius) {
                    const float damage = bullet.enemyDmg > 0.0f
                        ? bullet.enemyDmg : 10.0f;
                    HurtPlayer(playerHP, damage * stats.rmobDmgMult);
                    bullet.active = false;
                    playerHit = true;
                }
                continue;
            }

            const float minX = std::min(bullet.prevX, bullet.x);
            const float maxX = std::max(bullet.prevX, bullet.x);
            const float minY = std::min(bullet.prevY, bullet.y);
            const float maxY = std::max(bullet.prevY, bullet.y);
            bool consumed = false;

            for (auto* monster : manager.monsters) {
                if (!monster->alive) continue;
                const float hitRadius = 15.0f * monster->sizeScale;
                if (monster->worldX < minX - hitRadius || monster->worldX > maxX + hitRadius ||
                    monster->worldY < minY - hitRadius || monster->worldY > maxY + hitRadius)
                    continue;
                if (SegDistSq(monster->worldX, monster->worldY,
                              bullet.prevX, bullet.prevY, bullet.x, bullet.y) >=
                    hitRadius * hitRadius)
                    continue;

                bool isCrit = false;
                const float baseDamage = bullet.lockedDmg > 0.0f
                    ? bullet.lockedDmg
                    : stats.GetBaseDamage() * stats.GetDamageMultiplier() *
                      bullet.dmgMult * CritRoll(stats, isCrit);
                const float dealt = std::min(baseDamage, monster->hp);
                monster->hp -= dealt;
                SpawnDamageNumber(monster->worldX, monster->worldY, dealt,
                                  dealt >= 40.0f || isCrit);
                SpawnSparks(bullet.x, bullet.y, isCrit ? 6 : 3,
                            isCrit ? 1.0f : bullet.color.r,
                            isCrit ? 0.85f : bullet.color.g,
                            isCrit ? 0.3f : bullet.color.b);
                if (monster->hp <= 0.0f) {
                    monster->alive = false;
                    monster->scored = true;
                    AddKillCombo();
                    float baseXp = 0.0f;
                    float baseScore = 0.0f;
                    MobKillReward(monster->kind, baseXp, baseScore);
                    const float rewardMult = MobRewardMult(monster->kind);
                    baseXp *= rewardMult;
                    baseScore *= rewardMult;
                    const long long pickupXp = (long long)(
                        (baseXp + MobXpBonus(monster->kind, stats)) * stats.xpMult);
                    SpawnStardust(monster->worldX, monster->worldY,
                                  StardustRewardFor(monster->kind), playerCX,
                                  playerCY, pickupXp);
                    stats.killCount += 1;
                    scoreAccum += baseScore;
                    score = (long long)scoreAccum;
                    if (stats.vampire || stats.lifesteal2) {
                        ++stats.vampireKillStreak;
                        if (stats.vampireKillStreak >= stats.GetVampireKillNeed()) {
                            stats.vampireKillStreak = 0;
                            playerHP = std::min(stats.maxHP, playerHP + 1.0f);
                        }
                    }
                    if (stats.GetLifestealPerKill() > 0.0f)
                        playerHP = std::min(stats.maxHP,
                                            playerHP + stats.GetLifestealPerKill() * rewardMult);
                }

                bool keepAlive = stats.pierce &&
                    (rand() % 100) < (stats.pierceChance + bullet.pierceBonusPct);
                if (!keepAlive && bullet.bouncesLeft > 0 &&
                    (rand() % 100) < stats.ricochetChance &&
                    RicochetTo(bullet, monster->worldX, monster->worldY, manager)) {
                    if (bullet.lockedDmg <= 0.0f) bullet.lockedDmg = baseDamage;
                    --bullet.bouncesLeft;
                    keepAlive = true;
                }
                if (!keepAlive) bullet.active = false;
                consumed = true;
                break;
            }

            if (consumed) continue;
            for (auto* ranged : manager.rangedMobs) {
                if (!ranged->alive) continue;
                constexpr float HIT_RADIUS = 20.0f;
                if (ranged->worldX < minX - HIT_RADIUS || ranged->worldX > maxX + HIT_RADIUS ||
                    ranged->worldY < minY - HIT_RADIUS || ranged->worldY > maxY + HIT_RADIUS)
                    continue;
                if (SegDistSq(ranged->worldX, ranged->worldY,
                              bullet.prevX, bullet.prevY, bullet.x, bullet.y) >=
                    HIT_RADIUS * HIT_RADIUS)
                    continue;

                bool isCrit = false;
                const float baseDamage = bullet.lockedDmg > 0.0f
                    ? bullet.lockedDmg
                    : stats.GetBaseDamage() * stats.GetDamageMultiplier() *
                      bullet.dmgMult * CritRoll(stats, isCrit);
                const float dealt = std::min(baseDamage, ranged->hp);
                ranged->hp -= dealt;
                SpawnDamageNumber(ranged->worldX, ranged->worldY, dealt,
                                  dealt >= 40.0f || isCrit);
                SpawnSparks(bullet.x, bullet.y, isCrit ? 6 : 3,
                            isCrit ? 1.0f : bullet.color.r,
                            isCrit ? 0.85f : bullet.color.g,
                            isCrit ? 0.3f : bullet.color.b);
                if (ranged->hp <= 0.0f) {
                    ranged->alive = false;
                    ranged->scored = true;
                    AddKillCombo();
                    const long long pickupXp = (long long)(
                        (25.0f + (float)stats.rangedXpBonus) * stats.xpMult);
                    SpawnStardust(ranged->worldX, ranged->worldY, 3,
                                  playerCX, playerCY, pickupXp);
                    stats.killCount += 1;
                    scoreAccum += 300.0f;
                    score = (long long)scoreAccum;
                    if (stats.vampire || stats.lifesteal2) {
                        ++stats.vampireKillStreak;
                        if (stats.vampireKillStreak >= stats.GetVampireKillNeed()) {
                            stats.vampireKillStreak = 0;
                            playerHP = std::min(stats.maxHP, playerHP + 1.0f);
                        }
                    }
                    if (stats.GetLifestealPerKill() > 0.0f)
                        playerHP = std::min(stats.maxHP,
                                            playerHP + stats.GetLifestealPerKill());
                    if (stats.hackRanged && (rand() % 100) < 20) {
                        constexpr int COUNT = 5;
                        for (int i = 0; i < COUNT; ++i) {
                            const float angle = (float)i / (float)COUNT * 6.2831853f +
                                ((float)(rand() % 100) - 50.0f) * 0.01f;
                            Bullet shard(ranged->worldX, ranged->worldY,
                                         ranged->worldX + cosf(angle) * 100.0f,
                                         ranged->worldY + sinf(angle) * 100.0f);
                            shard.speed = stats.bulletSpeed * 0.9f;
                            shard.homing = true;
                            shard.homingTurn = 5.5f;
                            shard.dmgMult = 0.6f;
                            shard.color = glm::vec3(0.2f, 1.0f, 0.6f);
                            bullets.push_back(shard);
                        }
                    }
                }

                bool keepAlive = stats.pierce &&
                    (rand() % 100) < (stats.pierceChance + bullet.pierceBonusPct);
                if (!keepAlive && bullet.bouncesLeft > 0 &&
                    (rand() % 100) < stats.ricochetChance &&
                    RicochetTo(bullet, ranged->worldX, ranged->worldY, manager)) {
                    if (bullet.lockedDmg <= 0.0f) bullet.lockedDmg = baseDamage;
                    --bullet.bouncesLeft;
                    keepAlive = true;
                }
                if (!keepAlive) bullet.active = false;
                break;
            }
        }

        bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
                                     [](const Bullet& bullet) { return !bullet.active; }),
                      bullets.end());
        return playerHit;
    }
};
