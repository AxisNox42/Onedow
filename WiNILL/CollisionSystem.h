#pragma once
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#include "MonsterManager.h"
#include "Bullet.h"
#include "PlayerStats.h"
#include "Juice.h"

// 점 P 와 선분 AB(이전위치→현재위치) 사이 최단 거리 — 스윕(레이캐스트) 충돌 판정
//   탄속이 빨라 프레임 사이에 적을 지나쳐도, 경로 선분으로 판정해 명중 처리
static inline float SegDist(float px, float py, float ax, float ay, float bx, float by) {
    float abx = bx - ax, aby = by - ay;
    float len2 = abx*abx + aby*aby;
    float t = 0.0f;
    if (len2 > 1e-6f) {
        t = ((px - ax)*abx + (py - ay)*aby) / len2;
        if (t < 0.0f) t = 0.0f; else if (t > 1.0f) t = 1.0f;
    }
    float cx = ax + abx*t, cy = ay + aby*t;
    float dx = px - cx, dy = py - cy;
    return std::sqrt(dx*dx + dy*dy);
}

// 연쇄 작용(리코셰) — 적중 지점에서 가장 가까운 '다른' 적으로 총알 방향 전환.
//   성공 시 true. (적중한 적은 fromX/fromY 와 동일 위치라 d²≈0 으로 제외됨)
static inline bool RicochetTo(Bullet& b, float fromX, float fromY, MonsterManager& mm) {
    float bestD2 = 1e18f, bx = 0, by = 0; bool found = false;
    auto consider = [&](float ex, float ey) {
        float dx = ex - fromX, dy = ey - fromY;
        float d2 = dx*dx + dy*dy;
        if (d2 > 4.0f && d2 < bestD2) { bestD2 = d2; bx = ex; by = ey; found = true; }
    };
    for (auto m  : mm.monsters)   if (m->alive)  consider(m->worldX,  m->worldY);
    for (auto r  : mm.rangedMobs) if (r->alive)  consider(r->worldX,  r->worldY);
    for (auto bm : mm.bombers)    if (bm->alive) consider(bm->worldX, bm->worldY);
    if (mm.boss && mm.boss->alive) consider(mm.boss->worldX, mm.boss->worldY);
    if (!found) return false;
    float dx = bx - b.x, dy = by - b.y;
    float d = std::sqrt(dx*dx + dy*dy);
    if (d < 1.0f) return false;
    b.dirX = dx / d; b.dirY = dy / d;
    b.prevX = b.x;   b.prevY = b.y;     // 스윕 판정 리셋
    return true;
}

// 치명타 굴림 — 갓 계산된(fresh) 데미지에만 적용. 성공 시 critMult, 실패 시 1.0
static inline float CritRoll(const PlayerStats& stats, bool& isCrit) {
    if (stats.critChance > 0 && (rand() % 100) < stats.critChance) {
        isCrit = true;  return stats.critMult;
    }
    isCrit = false; return 1.0f;
}

class CollisionSystem {
public:
    // 반환값: 이번 프레임에 플레이어가 피해를 받았는지 (LIGHT_STEP 타이머용)
    static bool Update(float playerCX, float playerCY,
                       MonsterManager& mm, std::vector<Bullet>& bullets,
                       float& playerHP, float& scoreAccum, long long& score,
                       PlayerStats& stats, long long& xp)
    {
        bool playerHit = false;

        for (auto& b : bullets) {
            if (!b.active) continue;

            if (b.isEnemy) {
                // 적 총알 → 플레이어 (스윕 판정)
                float dist = SegDist(playerCX, playerCY, b.prevX, b.prevY, b.x, b.y);
                if (dist < 20.0f) {
                    float ed = (b.enemyDmg > 0.0f) ? b.enemyDmg : 10.0f;
                    playerHP -= ed * stats.rmobDmgMult;
                    b.active  = false;
                    playerHit = true;
                }
                continue;
            }

            // 플레이어 총알 vs 잡몹
            bool consumed = false;
            for (auto m : mm.monsters) {
                if (!m->alive) continue;
                float d = SegDist(m->worldX, m->worldY, b.prevX, b.prevY, b.x, b.y);
                if (d < 15.0f * m->sizeScale) {   // 분열체 등 큰 몹은 히트박스도 큼
                    float pd = glm::distance(glm::vec2(playerCX, playerCY),
                                             glm::vec2(m->worldX, m->worldY));
                    // CANNON: remainingDmg / 포탑: turretDmg / 그 외: 일반 계산
                    float baseDealt;
                    bool  isCrit = false;
                    if (b.remainingDmg > 0.0f) {
                        baseDealt = b.remainingDmg;
                    } else if (b.turretDmg > 0.0f) {
                        baseDealt = b.turretDmg;
                    } else if (b.lockedDmg > 0.0f) {
                        baseDealt = b.lockedDmg;       // 튕긴 총알 — 거리 재계산 안 함
                    } else {
                        baseDealt = stats.GetBaseDamage()
                                  * stats.GetDamageMultiplier(pd)
                                  * b.dmgMult * CritRoll(stats, isCrit);
                    }
                    float dealtThisHit = (baseDealt < m->hp) ? baseDealt : m->hp;
                    m->hp -= dealtThisHit;
                    if (b.remainingDmg > 0.0f) b.remainingDmg -= dealtThisHit;
                    SpawnDamageNumber(m->worldX, m->worldY, dealtThisHit, dealtThisHit >= 40.0f || isCrit);

                    if (m->hp <= 0.0f) {
                        m->alive = false;
                        m->scored = true;   // 총알 처치 — 보상 지급 완료 표시
                        AddKillCombo();
                        // 종류별 기본 EXP/점수 (분열체 자식은 낮게, 점멸체는 높게)
                        float baseXp = 1.0f, baseScore = 100.0f;
                        if (m->kind == MobKind::SPLITTER) {
                            baseXp = (m->splitGen >= 2) ? 1.0f : 2.0f; baseScore = 120.0f;
                        } else if (m->kind == MobKind::BLINKER) {
                            baseXp = 6.0f; baseScore = 250.0f;
                        }
                        float gained = (baseXp + (float)stats.meleeXpBonus)
                                     * stats.xpMult;
                        xp              += (long long)gained;
                        stats.killCount += 1;
                        scoreAccum      += baseScore;
                        score = (long long)scoreAccum;
                        if (stats.vampire) {
                            ++stats.vampireKillStreak;
                            if (stats.vampireKillStreak >= 10) {
                                stats.vampireKillStreak = 0;
                                playerHP += 1.0f;
                                if (playerHP > stats.maxHP)
                                    playerHP = stats.maxHP;
                            }
                        }
                        if (stats.lifestealPerKill > 0.0f) {
                            playerHP += stats.lifestealPerKill;
                            if (playerHP > stats.maxHP) playerHP = stats.maxHP;
                        }
                    }

                    // 관통 판정: cannon 잔존 데미지 OR pierce 30%
                    bool keepAlive = false;
                    if (b.remainingDmg > 0.001f) keepAlive = true;
                    if (stats.pierce && (rand() % 100) < stats.pierceChance) keepAlive = true;
                    // 연쇄 작용(리코셰) — 가까운 다른 적으로 튕김
                    if (!keepAlive && b.bouncesLeft > 0 &&
                        (rand() % 100) < stats.ricochetChance &&
                        RicochetTo(b, m->worldX, m->worldY, mm)) {
                        if (b.lockedDmg <= 0.0f) b.lockedDmg = baseDealt;
                        --b.bouncesLeft;
                        keepAlive = true;
                    }
                    if (!keepAlive) b.active = false;
                    consumed = true;
                    break;
                }
            }

            // 플레이어 총알 vs 자폭병
            if (!consumed) {
                for (auto bm : mm.bombers) {
                    if (!bm->alive) continue;
                    float d = SegDist(bm->worldX, bm->worldY, b.prevX, b.prevY, b.x, b.y);
                    if (d < 18.0f) {
                        float pd = glm::distance(glm::vec2(playerCX, playerCY),
                                                 glm::vec2(bm->worldX, bm->worldY));
                        float baseDealt;
                        bool  isCrit = false;
                        if (b.remainingDmg > 0.0f) baseDealt = b.remainingDmg;
                        else if (b.turretDmg > 0.0f) baseDealt = b.turretDmg;
                        else if (b.lockedDmg > 0.0f) baseDealt = b.lockedDmg;
                        else baseDealt = stats.GetBaseDamage()
                                       * stats.GetDamageMultiplier(pd)
                                       * b.dmgMult * CritRoll(stats, isCrit);
                        float dealtThisHit = (baseDealt < bm->hp) ? baseDealt : bm->hp;
                        bm->hp -= dealtThisHit;
                        if (b.remainingDmg > 0.0f) b.remainingDmg -= dealtThisHit;
                        SpawnDamageNumber(bm->worldX, bm->worldY, dealtThisHit, dealtThisHit >= 40.0f || isCrit);
                        if (bm->hp <= 0.0f) {
                            bm->alive = false;
                            bm->scored = true;       // 총알 처치 — 보상 지급 완료 표시
                            AddKillCombo();
                            TriggerHitStop(0.03f);   // 자폭병 처치 — 짧은 크런치
                            float gained = (25.0f + (float)stats.meleeXpBonus)
                                         * stats.xpMult;
                            xp              += (long long)gained;
                            stats.killCount += 1;
                            scoreAccum      += 200.0f;
                            score = (long long)scoreAccum;
                            if (stats.vampire) {
                                ++stats.vampireKillStreak;
                                if (stats.vampireKillStreak >= 10) {
                                    stats.vampireKillStreak = 0;
                                    playerHP += 1.0f;
                                    if (playerHP > stats.maxHP)
                                        playerHP = stats.maxHP;
                                }
                            }
                            if (stats.lifestealPerKill > 0.0f) {
                                playerHP += stats.lifestealPerKill;
                                if (playerHP > stats.maxHP) playerHP = stats.maxHP;
                            }
                            // HACK_BOMBER: 20% 확률 폭발 (적에게만 피해, VFX는 main.cpp 에서)
                            if (stats.hackBomber && (rand() % 100) < 20) {
                                bm->hackBlastPending = true;  // main.cpp 에서 폭발 VFX spawn
                                float hackDmg = stats.GetBaseDamage()
                                              * stats.GetDamageMultiplier(0.0f) * 1.5f;
                                float hackR   = 150.0f;
                                float hcx = bm->worldX, hcy = bm->worldY;
                                for (auto m2 : mm.monsters) {
                                    if (!m2->alive) continue;
                                    float ddx = m2->worldX - hcx, ddy = m2->worldY - hcy;
                                    if (ddx*ddx + ddy*ddy < hackR * hackR) {
                                        m2->hp -= hackDmg;
                                        if (m2->hp <= 0) m2->alive = false;
                                    }
                                }
                                for (auto r2 : mm.rangedMobs) {
                                    if (!r2->alive) continue;
                                    float ddx = r2->worldX - hcx, ddy = r2->worldY - hcy;
                                    if (ddx*ddx + ddy*ddy < hackR * hackR) {
                                        r2->hp -= hackDmg;
                                        if (r2->hp <= 0) r2->alive = false;
                                    }
                                }
                                for (auto bm2 : mm.bombers) {
                                    if (!bm2->alive || bm2 == bm) continue;
                                    float ddx = bm2->worldX - hcx, ddy = bm2->worldY - hcy;
                                    if (ddx*ddx + ddy*ddy < hackR * hackR) {
                                        bm2->hp -= hackDmg;
                                        if (bm2->hp <= 0) bm2->alive = false;
                                    }
                                }
                                if (mm.boss && mm.boss->alive) {
                                    float ddx = mm.boss->worldX - hcx, ddy = mm.boss->worldY - hcy;
                                    if (ddx*ddx + ddy*ddy < hackR * hackR) {
                                        mm.boss->hp -= hackDmg;
                                        if (mm.boss->hp <= 0) mm.boss->alive = false;
                                    }
                                }
                            }
                        }
                        bool keepAlive = false;
                        if (b.remainingDmg > 0.001f) keepAlive = true;
                        if (stats.pierce && (rand() % 100) < stats.pierceChance) keepAlive = true;
                        if (!keepAlive && b.bouncesLeft > 0 &&
                            (rand() % 100) < stats.ricochetChance &&
                            RicochetTo(b, bm->worldX, bm->worldY, mm)) {
                            if (b.lockedDmg <= 0.0f) b.lockedDmg = baseDealt;
                            --b.bouncesLeft;
                            keepAlive = true;
                        }
                        if (!keepAlive) b.active = false;
                        consumed = true;
                        break;
                    }
                }
            }

            // 플레이어 총알 vs 보스
            if (!consumed && mm.boss && mm.boss->alive) {
                auto* bs = mm.boss;
                float d = SegDist(bs->worldX, bs->worldY, b.prevX, b.prevY, b.x, b.y);
                if (d < Boss::BODY_SIZE * 0.65f) {
                    float pd = glm::distance(glm::vec2(playerCX, playerCY),
                                             glm::vec2(bs->worldX, bs->worldY));
                    float baseDealt;
                    bool  isCrit = false;
                    if (b.remainingDmg > 0.0f) baseDealt = b.remainingDmg;
                    else if (b.turretDmg > 0.0f) baseDealt = b.turretDmg;
                    else if (b.lockedDmg > 0.0f) baseDealt = b.lockedDmg;
                    else baseDealt = stats.GetBaseDamage()
                                   * stats.GetDamageMultiplier(pd)
                                   * b.dmgMult * CritRoll(stats, isCrit);
                    float dealtThisHit = (baseDealt < bs->hp) ? baseDealt : bs->hp;
                    bs->hp -= dealtThisHit;
                    if (b.remainingDmg > 0.0f) b.remainingDmg -= dealtThisHit;
                    SpawnDamageNumber(bs->worldX, bs->worldY, dealtThisHit, dealtThisHit >= 40.0f || isCrit);
                    if (bs->hp <= 0.0f) {
                        bs->alive = false; // 보상/연출은 main 에서 처리
                    }
                    bool keepAlive = false;
                    if (b.remainingDmg > 0.001f) keepAlive = true;
                    if (stats.pierce && (rand() % 100) < stats.pierceChance) keepAlive = true;
                    if (!keepAlive && b.bouncesLeft > 0 &&
                        (rand() % 100) < stats.ricochetChance &&
                        RicochetTo(b, bs->worldX, bs->worldY, mm)) {
                        if (b.lockedDmg <= 0.0f) b.lockedDmg = baseDealt;
                        --b.bouncesLeft;
                        keepAlive = true;
                    }
                    if (!keepAlive) b.active = false;
                    consumed = true;
                }
            }

            // 플레이어 총알 vs 원거리 몹 — cannon 잔존 데미지 / pierce 모두 적용
            if (!consumed) {
                for (auto r : mm.rangedMobs) {
                    if (!r->alive) continue;
                    float d = SegDist(r->worldX, r->worldY, b.prevX, b.prevY, b.x, b.y);
                    if (d < 20.0f) {
                        float pd = glm::distance(glm::vec2(playerCX, playerCY),
                                                 glm::vec2(r->worldX, r->worldY));
                        float baseDealt;
                        bool  isCrit = false;
                        if (b.remainingDmg > 0.0f) {
                            baseDealt = b.remainingDmg;
                        } else if (b.turretDmg > 0.0f) {
                            baseDealt = b.turretDmg;
                        } else if (b.lockedDmg > 0.0f) {
                            baseDealt = b.lockedDmg;       // 튕긴 총알 — 거리 재계산 안 함
                        } else {
                            baseDealt = stats.GetBaseDamage()
                                      * stats.GetDamageMultiplier(pd)
                                      * b.dmgMult * CritRoll(stats, isCrit);
                        }
                        float dealtThisHit = (baseDealt < r->hp) ? baseDealt : r->hp;
                        r->hp -= dealtThisHit;
                        if (b.remainingDmg > 0.0f) b.remainingDmg -= dealtThisHit;
                        SpawnDamageNumber(r->worldX, r->worldY, dealtThisHit, dealtThisHit >= 40.0f || isCrit);

                        if (r->hp <= 0.0f) {
                            r->alive = false;
                            r->scored = true;        // 총알 처치 — 보상 지급 완료 표시
                            AddKillCombo();
                            TriggerHitStop(0.03f);   // 원거리 몹 처치 — 짧은 크런치
                            float gained = (25.0f + (float)stats.rangedXpBonus)
                                         * stats.xpMult;
                            xp              += (long long)gained;
                            stats.killCount += 1;
                            scoreAccum      += 300.0f;
                            score = (long long)scoreAccum;
                            if (stats.vampire) {
                                ++stats.vampireKillStreak;
                                if (stats.vampireKillStreak >= 10) {
                                    stats.vampireKillStreak = 0;
                                    playerHP += 1.0f;
                                    if (playerHP > stats.maxHP)
                                        playerHP = stats.maxHP;
                                }
                            }
                            if (stats.lifestealPerKill > 0.0f) {
                                playerHP += stats.lifestealPerKill;
                                if (playerHP > stats.maxHP) playerHP = stats.maxHP;
                            }
                            // HACK_RANGED: 20% 확률 유도탄 5발 (적에게만 피해 — player bullet)
                            if (stats.hackRanged && (rand() % 100) < 20) {
                                const int N = 5;
                                for (int k = 0; k < N; k++) {
                                    float a = (float)k / (float)N * 6.2831853f
                                            + ((float)(rand() % 100) - 50.0f) * 0.01f;
                                    Bullet nb(r->worldX, r->worldY,
                                              r->worldX + cosf(a) * 100.0f,
                                              r->worldY + sinf(a) * 100.0f);
                                    nb.speed      = stats.bulletSpeed * 0.9f;
                                    nb.homing     = true;
                                    nb.homingTurn = 5.5f;
                                    nb.dmgMult    = 0.6f;
                                    nb.color      = glm::vec3(0.2f, 1.0f, 0.6f);
                                    bullets.push_back(nb);
                                }
                            }
                        }

                        bool keepAlive = false;
                        if (b.remainingDmg > 0.001f) keepAlive = true;
                        if (stats.pierce && (rand() % 100) < stats.pierceChance) keepAlive = true;
                        if (!keepAlive && b.bouncesLeft > 0 &&
                            (rand() % 100) < stats.ricochetChance &&
                            RicochetTo(b, r->worldX, r->worldY, mm)) {
                            if (b.lockedDmg <= 0.0f) b.lockedDmg = baseDealt;
                            --b.bouncesLeft;
                            keepAlive = true;
                        }
                        if (!keepAlive) b.active = false;
                        break;
                    }
                }
            }
        }

        bullets.erase(
            std::remove_if(bullets.begin(), bullets.end(),
                [](const Bullet& b){ return !b.active; }),
            bullets.end());

        return playerHit;
    }
};
