#pragma once

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>

#include "MonsterManager.h"
#include "Bullet.h"
#include "PlayerStats.h"
#include "Juice.h"
#include "../Core/Codex.h"

// Kill stardust: spawned now, or held on the mob for a delayed death and
// released on the blast ring (Quasar supernova, Genesis first wave).
inline void SpawnKillStardust(Monster& m, int value, float playerCX, float playerCY,
                              XpAmount xp) {
    if (m.kind == MobKind::QUASAR || m.kind == MobKind::GENESIS) {
        m.dustPending = true;
        m.pendingDustValue = value;
        m.pendingDustXp = xp;
        return;
    }
    SpawnStardust(m.worldX, m.worldY, value, playerCX, playerCY, xp);
}

// A thrown Genesis Swarm's kill, on every kill path: the combo and the codex
// only — no score, XP/stardust, lifesteal, run kill count (warlord), vampire
// streak, or 무한 세례 kill.
inline void CreditThrownSwarmKill(Monster& m) {
    m.scored = true;
    AddKillCombo(/*countsForRain=*/false);
    RegisterCodexMobKill(m.kind);
}

// The only owner of ordinary kill rewards. Callers decide when a death is
// eligible; scored prevents the same death being credited by another weapon.
struct KillRewardSink {
    float& playerHP;
    float& scoreAccum;
    long long& score;
    PlayerStats& stats;
    float playerCX, playerCY;
};

inline void CreditRegularKill(KillRewardSink& s, CodexMobId mobId, float scoreBase) {
    AddKillCombo();
    s.stats.RegisterKill();
    RegisterCodexMobKill(mobId);
    s.scoreAccum += scoreBase;
    s.score = (long long)s.scoreAccum;
    s.playerHP = s.stats.ApplyKillLifesteal(s.playerHP);
    s.playerHP = s.stats.ApplyVampireKill(s.playerHP);
}

inline void CollectMonsterKill(KillRewardSink& s, Monster& m) {
    if (m.scored) return;
    if (m.genesisThrown) {
        CreditThrownSwarmKill(m);
        return;
    }
    m.scored = true;
    float xpBase, scoreBase;
    MobKillReward(m.kind, xpBase, scoreBase);
    scoreBase *= MobRewardMult(m.kind);
    const XpAmount pickupXp = MobExperienceReward(m.kind, s.stats);
    SpawnKillStardust(m, StardustRewardFor(m.kind),
                      s.playerCX, s.playerCY, pickupXp);
    CreditRegularKill(s, CodexMobIdForKind(m.kind), scoreBase);
}

inline void CollectScopeKill(KillRewardSink& s, RangedMob& r) {
    if (r.scored) return;
    r.scored = true;
    const XpAmount pickupXp = RangedMob::ExperienceReward(s.stats);
    SpawnStardust(r.worldX, r.worldY, 3,
                  s.playerCX, s.playerCY, pickupXp);
    CreditRegularKill(s, CM_SCOPE, 300.0f);
}

// Releases held kill stardust whose moment has come. Call every frame.
inline void ReleasePendingStardust(MonsterManager& manager, float playerCX, float playerCY) {
    for (auto* m : manager.monsters) {
        if (!m || !m->TakePendingDust()) continue;
        const float base = (m->summoned ? 28.0f : 18.0f) * m->sizeScale;
        SpawnStardust(m->worldX, m->worldY, m->pendingDustValue, playerCX, playerCY,
                      m->pendingDustXp, base * 0.9f);
    }
}

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
                       PlayerStats& stats) {
        bool playerHit = false;
        KillRewardSink rewards{playerHP, scoreAccum, score, stats, playerCX, playerCY};

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

                bool isCrit = bullet.lockedDmg > 0.0f && bullet.lockedCrit;
                const float baseDamage = bullet.lockedDmg > 0.0f
                    ? bullet.lockedDmg
                    : stats.GetBaseDamage() * stats.GetDamageMultiplier() *
                      bullet.dmgMult * CritRoll(stats, isCrit);
                const float dealt = manager.ApplyDamage(*monster, baseDamage);
                SpawnDamageNumber(monster->worldX, monster->worldY, dealt,
                                  isCrit);
                SpawnSparks(bullet.x, bullet.y, isCrit ? 6 : 3,
                            isCrit ? 1.0f : bullet.color.r,
                            isCrit ? 0.85f : bullet.color.g,
                            isCrit ? 0.3f : bullet.color.b);
                if (!monster->alive) CollectMonsterKill(rewards, *monster);

                bool keepAlive = stats.pierce &&
                    (rand() % 100) < (stats.pierceChance + bullet.pierceBonusPct);
                if (!keepAlive && bullet.bouncesLeft > 0 &&
                    (rand() % 100) < stats.ricochetChance &&
                    RicochetTo(bullet, monster->worldX, monster->worldY, manager)) {
                    if (bullet.lockedDmg <= 0.0f) {
                        bullet.lockedDmg = baseDamage;
                        bullet.lockedCrit = isCrit;
                    }
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

                bool isCrit = bullet.lockedDmg > 0.0f && bullet.lockedCrit;
                const float baseDamage = bullet.lockedDmg > 0.0f
                    ? bullet.lockedDmg
                    : stats.GetBaseDamage() * stats.GetDamageMultiplier() *
                      bullet.dmgMult * CritRoll(stats, isCrit);
                const float dealt = manager.ApplyDamage(*ranged, baseDamage);
                SpawnDamageNumber(ranged->worldX, ranged->worldY, dealt,
                                  isCrit);
                SpawnSparks(bullet.x, bullet.y, isCrit ? 6 : 3,
                            isCrit ? 1.0f : bullet.color.r,
                            isCrit ? 0.85f : bullet.color.g,
                            isCrit ? 0.3f : bullet.color.b);
                if (!ranged->alive) {
                    CollectScopeKill(rewards, *ranged);
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
                    if (bullet.lockedDmg <= 0.0f) {
                        bullet.lockedDmg = baseDamage;
                        bullet.lockedCrit = isCrit;
                    }
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
