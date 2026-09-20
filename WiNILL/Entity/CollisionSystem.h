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
static inline float SegDistSq(float px, float py, float ax, float ay, float bx, float by) {
    float abx = bx - ax, aby = by - ay;
    float len2 = abx*abx + aby*aby;
    float t = 0.0f;
    if (len2 > 1e-6f) {
        t = ((px - ax)*abx + (py - ay)*aby) / len2;
        if (t < 0.0f) t = 0.0f; else if (t > 1.0f) t = 1.0f;
    }
    float cx = ax + abx*t, cy = ay + aby*t;
    float dx = px - cx, dy = py - cy;
    return dx*dx + dy*dy;
}

static inline float SegDist(float px, float py, float ax, float ay, float bx, float by) {
    return std::sqrt(SegDistSq(px, py, ax, ay, bx, by));
}

// HE탄 — 대포 관통 종료 시 소형 폭발
static inline void TryHEShellBlast(float cx, float cy, const PlayerStats& stats,
                                   MonsterManager& mm) {
    if (!stats.heShells) return;
    float pct = stats.heShells2 ? 0.35f : 0.25f;
    float rad = stats.heShells2 ? 110.0f : 80.0f;
    float blastDmg = stats.GetBaseDamage() * stats.GetDamageMultiplier(0.0f) * pct;
    const float r2 = rad * rad;
    auto hitRad = [&](float& ex, float& ey, float& hp, bool& al) {
        float dx = ex - cx, dy = ey - cy;
        if (dx*dx + dy*dy < r2) {
            hp -= blastDmg;
            if (hp <= 0.0f) al = false;
        }
    };
    for (auto m : mm.monsters)    if (m->alive)  hitRad(m->worldX,  m->worldY,  m->hp,  m->alive);
    for (auto r : mm.rangedMobs)  if (r->alive)  hitRad(r->worldX,  r->worldY,  r->hp,  r->alive);
    for (auto bm : mm.bombers)    if (bm->alive) hitRad(bm->worldX, bm->worldY, bm->hp, bm->alive);
    SpawnEnemyExplosion(cx, cy, 1.0f, 0.45f, 0.08f, true);
}

// 연쇄 작용(리코셰) — 적중 지점에서 가장 가까운 '다른' 적으로 총알 방향 전환.
//   성공 시 true. (적중한 적은 fromX/fromY 와 동일 위치라 d²≈0 으로 제외됨)
static inline bool RicochetTo(Bullet& b, float fromX, float fromY, MonsterManager& mm) {
    // 연쇄 작용 — 주변 200px 내에 적이 있을 때만 튕김 (없으면 그냥 소멸)
    float bestD2 = 200.0f * 200.0f, bx = 0, by = 0; bool found = false;
    auto consider = [&](float ex, float ey) {
        float dx = ex - fromX, dy = ey - fromY;
        float d2 = dx*dx + dy*dy;
        if (d2 > 4.0f && d2 < bestD2) { bestD2 = d2; bx = ex; by = ey; found = true; }
    };
    for (auto m  : mm.monsters)   if (m->alive)  consider(m->worldX,  m->worldY);
    for (auto r  : mm.rangedMobs) if (r->alive)  consider(r->worldX,  r->worldY);
    for (auto bm : mm.bombers)    if (bm->alive) consider(bm->worldX, bm->worldY);
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

static inline void ApplySilverBurn(Monster* m, float atkDmg) {
    if (!m || !m->alive) return;
    m->burnTimer = 1.0f;
    m->burnDps   = atkDmg * 1.20f;
}

static inline void TryHackFirewallOnKill(MobKind kind, const PlayerStats& stats) {
    if (!stats.hackFirewall || kind != MobKind::SHIELDED) return;
    if (rand() % 100 >= 10) return;
    GrantPlayerShield(stats.maxHP * 0.20f, 3.0f);
}

static inline void MinigunCycloneHit(PlayerStats& stats) {
    if (stats.minigunCyclone)
        stats.minigunHitBoost = std::min(stats.minigunHitBoost + 0.1f, 0.6f);
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

        // 레지스트리 에러 강화 오라 — 노드 주변(반경 130) 적은 받는 피해 -5%(체력↑ 프록시).
        //   매우 희귀(보통 0개)라 프레임당 한 번만 좌표 수집 → 히트당 비용 거의 0.
        static std::vector<glm::vec2> regAura;
        regAura.clear();
        for (auto* rg : mm.monsters)
            if (rg->alive && rg->kind == MobKind::REGERROR)
                regAura.push_back(glm::vec2(rg->worldX, rg->worldY));
        const float REG_AURA_R2 = 145.0f * 145.0f;   // 가짜창 290px 의 절반 (범위 +50px 반영)

        for (auto& b : bullets) {
            if (!b.active) continue;

            if (b.isEnemy) {
                // 적 총알 → 플레이어 (스윕 판정)
                //   탄막(불릿헬) 회피 가독성 — 피격 판정을 비주얼보다 작은
                //   '중심 코어' 크기로 축소 (기존 20 → 10, 면적 1/4)
                const float hitRadius = 10.0f * stats.playerSizeMult;
                if (SegDistSq(playerCX, playerCY, b.prevX, b.prevY, b.x, b.y)
                    < hitRadius * hitRadius) {
                    float ed = (b.enemyDmg > 0.0f) ? b.enemyDmg : 10.0f;
                    HurtPlayer(playerHP, ed * stats.rmobDmgMult);
                    b.active  = false;
                    playerHit = true;
                }
                continue;
            }

            // 플레이어 총알 vs 잡몹
            const float segMinX = std::min(b.prevX, b.x);
            const float segMaxX = std::max(b.prevX, b.x);
            const float segMinY = std::min(b.prevY, b.y);
            const float segMaxY = std::max(b.prevY, b.y);
            bool consumed = false;
            for (auto m : mm.monsters) {
                if (!m->alive) continue;
                const float hitRadius = 15.0f * m->sizeScale;
                if (m->worldX < segMinX - hitRadius || m->worldX > segMaxX + hitRadius ||
                    m->worldY < segMinY - hitRadius || m->worldY > segMaxY + hitRadius)
                    continue;
                if (SegDistSq(m->worldX, m->worldY, b.prevX, b.prevY, b.x, b.y)
                    < hitRadius * hitRadius) {   // 분열체 등 큰 몹은 히트박스도 큼
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
                    // 보호막체: 방패 ON 동안 피해 85% 감소 (광역 공격은 우회)
                    if (m->kind == MobKind::SHIELDED && m->shieldActive) baseDealt *= 0.15f;
                    // 크래셔 강화 디버프 — 돌진(chargeState==2) 중 크래셔는 받는 피해 -10%
                    if (stats.crasherBoost && m->kind == MobKind::CHARGER && m->chargeState == 2)
                        baseDealt *= 0.90f;
                    // 레지스트리 에러 강화 오라 — 노드 주변 적은 받는 피해 -5%
                    if (!regAura.empty() && m->kind != MobKind::REGERROR) {
                        for (auto& rp : regAura) {
                            float ddx = m->worldX - rp.x, ddy = m->worldY - rp.y;
                            if (ddx*ddx + ddy*ddy < REG_AURA_R2) { baseDealt *= 0.95f; break; }
                        }
                    }
                    float dealtThisHit = (baseDealt < m->hp) ? baseDealt : m->hp;
                    m->hp -= dealtThisHit;
                    if (b.remainingDmg > 0.0f) b.remainingDmg -= dealtThisHit;
                    SpawnDamageNumber(m->worldX, m->worldY, dealtThisHit, dealtThisHit >= 40.0f || isCrit);
                    SpawnSparks(b.x, b.y, isCrit ? 6 : 3,
                                isCrit ? 1.0f : b.color.r, isCrit ? 0.85f : b.color.g,
                                isCrit ? 0.3f : b.color.b);   // 명중 스파크
                    MinigunCycloneHit(stats);
                    if (b.silverBurn)
                        ApplySilverBurn(m, stats.GetBaseDamage()
                                            * stats.GetDamageMultiplier(pd) * b.dmgMult);

                    if (m->hp <= 0.0f) {
                        m->alive = false;
                        m->scored = true;   // 총알 처치 — 보상 지급 완료 표시
                        TryHackFirewallOnKill(m->kind, stats);
                        AddKillCombo();
                        // 종류별 기본 EXP/점수 (공용 테이블, 엘리트 ×2.5)
                        float baseXp, baseScore;
                        MobKillReward(m->kind, m->splitGen, m->elite, baseXp, baseScore,
                                      stats.splitterBoost);
                        float rwm = MobRewardMult(m->kind);
                        baseXp *= rwm;
                        baseScore *= rwm;
                        float gained = (baseXp + MobDebuffXpBonus(m->kind, m->elite, stats))
                                     * stats.xpMult;
                        SpawnStardust(m->worldX, m->worldY,
                                      StardustRewardFor(m->kind), playerCX, playerCY,
                                      (long long)gained);
                        stats.killCount += 1;
                        scoreAccum      += baseScore;
                        score = (long long)scoreAccum;
                        if (stats.vampire || stats.lifesteal2) {
                            ++stats.vampireKillStreak;
                            if (stats.vampireKillStreak >= stats.GetVampireKillNeed()) {
                                stats.vampireKillStreak = 0;
                                playerHP += 1.0f;
                                if (playerHP > stats.maxHP)
                                    playerHP = stats.maxHP;
                            }
                        }
                        if (stats.GetLifestealPerKill() > 0.0f) {
                            playerHP += stats.GetLifestealPerKill() * MobRewardMult(m->kind);
                            if (playerHP > stats.maxHP) playerHP = stats.maxHP;
                        }
                    }

                    // 관통 판정: cannon 잔존 데미지 OR pierce 30%
                    bool keepAlive = false;
                    if (b.remainingDmg > 0.001f) keepAlive = true;
                    if (stats.pierce && (rand() % 100) < (stats.pierceChance + b.pierceBonusPct)) keepAlive = true;
                    // 연쇄 작용(리코셰) — 가까운 다른 적으로 튕김
                    if (!keepAlive && b.bouncesLeft > 0 &&
                        (rand() % 100) < stats.ricochetChance &&
                        RicochetTo(b, m->worldX, m->worldY, mm)) {
                        if (b.lockedDmg <= 0.0f) b.lockedDmg = baseDealt;
                        --b.bouncesLeft;
                        keepAlive = true;
                    }
                    if (!keepAlive) {
                        if (stats.heShells && b.remainingDmg > 0.001f)
                            TryHEShellBlast(b.x, b.y, stats, mm);
                        b.active = false;
                    }
                    consumed = true;
                    break;
                }
            }

            // 플레이어 총알 vs 자폭병
            if (!consumed) {
                for (auto bm : mm.bombers) {
                    if (!bm->alive) continue;
                    if (bm->worldX < segMinX - 18.0f || bm->worldX > segMaxX + 18.0f ||
                        bm->worldY < segMinY - 18.0f || bm->worldY > segMaxY + 18.0f)
                        continue;
                    if (SegDistSq(bm->worldX, bm->worldY, b.prevX, b.prevY, b.x, b.y)
                        < 18.0f * 18.0f) {
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
                        SpawnSparks(b.x, b.y, isCrit ? 6 : 3,
                                    isCrit ? 1.0f : b.color.r, isCrit ? 0.85f : b.color.g,
                                    isCrit ? 0.3f : b.color.b);
                        MinigunCycloneHit(stats);
                        if (bm->hp <= 0.0f) {
                            bm->alive = false;
                            bm->scored = true;       // 총알 처치 — 보상 지급 완료 표시
                            AddKillCombo();
                            TriggerHitStop(0.03f);   // 자폭병 처치 — 짧은 크런치
                            float gained = (25.0f + (float)stats.bomberXpBonus)
                                         * stats.xpMult;
                            SpawnStardust(bm->worldX, bm->worldY, 3,
                                          playerCX, playerCY, (long long)gained);
                            stats.killCount += 1;
                            scoreAccum      += 200.0f;
                            score = (long long)scoreAccum;
                            if (stats.vampire || stats.lifesteal2) {
                                ++stats.vampireKillStreak;
                                if (stats.vampireKillStreak >= stats.GetVampireKillNeed()) {
                                    stats.vampireKillStreak = 0;
                                    playerHP += 1.0f;
                                    if (playerHP > stats.maxHP)
                                        playerHP = stats.maxHP;
                                }
                            }
                            if (stats.GetLifestealPerKill() > 0.0f) {
                                playerHP += stats.GetLifestealPerKill();
                                if (playerHP > stats.maxHP) playerHP = stats.maxHP;
                            }
                            // HACK_BOMBER: 20% 확률 폭발
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
                            }
                        }
                        bool keepAlive = false;
                        if (b.remainingDmg > 0.001f) keepAlive = true;
                        if (stats.pierce && (rand() % 100) < (stats.pierceChance + b.pierceBonusPct)) keepAlive = true;
                        if (!keepAlive && b.bouncesLeft > 0 &&
                            (rand() % 100) < stats.ricochetChance &&
                            RicochetTo(b, bm->worldX, bm->worldY, mm)) {
                            if (b.lockedDmg <= 0.0f) b.lockedDmg = baseDealt;
                            --b.bouncesLeft;
                            keepAlive = true;
                        }
                        if (!keepAlive) {
                            if (stats.heShells && b.remainingDmg > 0.001f)
                                TryHEShellBlast(b.x, b.y, stats, mm);
                            b.active = false;
                        }
                        consumed = true;
                        break;
                    }
                }
            }

            // 플레이어 총알 vs 원거리 몹
            if (!consumed) {
                for (auto r : mm.rangedMobs) {
                    if (!r->alive) continue;
                    if (r->worldX < segMinX - 20.0f || r->worldX > segMaxX + 20.0f ||
                        r->worldY < segMinY - 20.0f || r->worldY > segMaxY + 20.0f)
                        continue;
                    if (SegDistSq(r->worldX, r->worldY, b.prevX, b.prevY, b.x, b.y)
                        < 20.0f * 20.0f) {
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
                        SpawnSparks(b.x, b.y, isCrit ? 6 : 3,
                                    isCrit ? 1.0f : b.color.r, isCrit ? 0.85f : b.color.g,
                                    isCrit ? 0.3f : b.color.b);
                        MinigunCycloneHit(stats);

                        if (r->hp <= 0.0f) {
                            r->alive = false;
                            r->scored = true;        // 총알 처치 — 보상 지급 완료 표시
                            AddKillCombo();
                            TriggerHitStop(0.03f);   // 원거리 몹 처치 — 짧은 크런치
                            float gained = (25.0f + (float)stats.rangedXpBonus)
                                         * stats.xpMult;
                            SpawnStardust(r->worldX, r->worldY, 3,
                                          playerCX, playerCY, (long long)gained);
                            stats.killCount += 1;
                            scoreAccum      += 300.0f;
                            score = (long long)scoreAccum;
                            if (stats.vampire || stats.lifesteal2) {
                                ++stats.vampireKillStreak;
                                if (stats.vampireKillStreak >= stats.GetVampireKillNeed()) {
                                    stats.vampireKillStreak = 0;
                                    playerHP += 1.0f;
                                    if (playerHP > stats.maxHP)
                                        playerHP = stats.maxHP;
                                }
                            }
                            if (stats.GetLifestealPerKill() > 0.0f) {
                                playerHP += stats.GetLifestealPerKill();
                                if (playerHP > stats.maxHP) playerHP = stats.maxHP;
                            }
                            // HACK_RANGED: 20% 확률 유도탄 5발
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
                        if (stats.pierce && (rand() % 100) < (stats.pierceChance + b.pierceBonusPct)) keepAlive = true;
                        if (!keepAlive && b.bouncesLeft > 0 &&
                            (rand() % 100) < stats.ricochetChance &&
                            RicochetTo(b, r->worldX, r->worldY, mm)) {
                            if (b.lockedDmg <= 0.0f) b.lockedDmg = baseDealt;
                            --b.bouncesLeft;
                            keepAlive = true;
                        }
                        if (!keepAlive) {
                            if (stats.heShells && b.remainingDmg > 0.001f)
                                TryHEShellBlast(b.x, b.y, stats, mm);
                            b.active = false;
                        }
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
