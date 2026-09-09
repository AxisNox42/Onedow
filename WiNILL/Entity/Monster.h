#pragma once
#include <glm/glm.hpp>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include "PlayerStats.h"

// 잡몹 종류 — 같은 monsters 벡터에서 kind 로 분기 (충돌/렌더 재사용)
//   NORMAL/SPLITTER/BLINKER 외:
//   - CHARGER/WEAVER/BRUTE : 점수 비례 자연 스폰
//   - ORBITER/SPAWNER/SHIELDED : 디버프 보유 시 등장
enum class MobKind {
    NORMAL, SPLITTER, BLINKER,
    CHARGER, WEAVER, BRUTE,
    ORBITER, SPAWNER, SHIELDED,
    // 확장 (인덱스 9~) — MarkMobSeen(<9) 대상 아님. 도감은 CodexMobId 로 별도 추적.
    DDOS,        // 디도스 — 점수 비례 물량 swarm (프로세스 1→3). 능력 없음
    BADSECTOR,   // 배드 섹터 — 육각형, 죽으면 임시 감속 구역 생성 (디버프/자연 스폰)
    REGERROR,    // 레지스트리 에러 — X본체+공전, 가짜창 내 적 강화 오라 (디버프/자연 스폰)
    GRAVIS,      // Tier 3 gravity-field station controller
    QUASAR       // Tier 2.5 long-range line controller
};

// 엘리트 변종 — 0 없음 / 1 신속 / 2 강인 / 3 폭발성. 어떤 잡몹에든 드물게 부여.
enum class EliteMod { NONE = 0, SWIFT = 1, TANKY = 2, VOLATILE = 3 };

// 종류별 처치 보상 — 총알/근접/광역 처치 모두 같은 값 쓰도록 공용화 (엘리트면 ×2.5)
inline void MobKillReward(MobKind k, int splitGen, int elite,
                          float& xpBase, float& scoreBase,
                          bool splitterBoost = false) {
    xpBase = 1.0f; scoreBase = 100.0f;
    switch (k) {
    case MobKind::SPLITTER:
        xpBase = (splitGen >= 2 && !splitterBoost) ? 1.0f : 2.0f;
        scoreBase = 120.0f;
        break;
    case MobKind::BLINKER:  xpBase = 6.0f; scoreBase = 250.0f; break;
    case MobKind::CHARGER:  xpBase = 3.0f; scoreBase = 180.0f; break;
    case MobKind::WEAVER:   xpBase = 3.0f; scoreBase = 160.0f; break;
    case MobKind::BRUTE:    xpBase = 8.0f; scoreBase = 400.0f; break;
    case MobKind::ORBITER:  xpBase = 5.0f; scoreBase = 200.0f; break;
    case MobKind::SPAWNER:  xpBase = 7.0f; scoreBase = 300.0f; break;
    case MobKind::SHIELDED: xpBase = 5.0f; scoreBase = 220.0f; break;
    case MobKind::DDOS:     xpBase = 0.12f; scoreBase = 12.0f;  break;  // 물량형 — 보상 최저
    case MobKind::BADSECTOR:xpBase = 30.0f;scoreBase = 350.0f; break;
    case MobKind::REGERROR: xpBase = 40.0f;scoreBase = 500.0f; break;
    case MobKind::GRAVIS:   xpBase = 11.0f; scoreBase = 520.0f; break;
    case MobKind::QUASAR:   xpBase = 9.0f; scoreBase = 480.0f; break;
    default: break;
    }
    if (elite) { xpBase *= 2.5f; scoreBase *= 2.5f; }
}

inline int StardustRewardFor(MobKind kind) {
    switch (kind) {
    case MobKind::NORMAL:  return 1;
    case MobKind::DDOS:    return 1;
    case MobKind::SPAWNER: return 15;
    case MobKind::GRAVIS:  return 15;
    case MobKind::QUASAR:  return 7;
    default:               return 0;
    }
}

inline float MobRewardMult(MobKind k) {
    if (k == MobKind::DDOS) return 0.20f;
    return 1.0f;
}

inline float MobDebuffXpBonus(MobKind k, int elite, const PlayerStats& stats) {
    float bonus = (float)stats.mobXpBonus;
    int idx = (int)k;
    if (idx >= 0 && idx < PlayerStats::MOB_KIND_XP_SLOTS)
        bonus += (float)stats.mobKindXpBonus[idx];
    if (k != MobKind::NORMAL)
        bonus += (float)stats.specialMobXpBonus;
    if (elite)
        bonus += (float)stats.eliteXpBonus;
    return bonus;
}

class Monster {
public:
    float worldX, worldY;
    float speed = 150.0f;  // 생성자에서 120~180 랜덤
    float hp = 90.0f;
    bool  alive    = true;
    bool  exploded = false;  // 사망 폭발/분열 1회만 처리하는 플래그
    bool  scored   = false;  // 처치 보상(EXP/점수/콤보) 지급 완료 플래그
    bool  noBlast  = false;  // 연쇄폭발에 죽은 몹 → 또 폭발하지 않음 (무한 연쇄 방지)
    bool  summoned = false;  // 보스 소환물 (더 크게)
    int   elite    = 0;      // 엘리트 변종 (0 없음 / 1 신속 / 2 강인 / 3 폭발성)
    glm::vec3 color;

    // ── 종류별 ──
    MobKind kind      = MobKind::NORMAL;
    float   sizeScale = 1.0f;   // 시각/충돌 크기 (분열체 세대별 축소)
    int     splitGen  = 0;      // 분열체 세대 (0 원본 → 최대 2)
    // 점멸체
    float   blinkTimer  = 0.0f;
    float   blinkWarnT  = 0.0f;
    bool    blinkWarn   = false;
    float   blinkTargetX = 0.0f, blinkTargetY = 0.0f;
    float   blinkIntervalMul = 1.0f;   // 트로이목마 강화 디버프 시 <1 (쿨다운 단축)
    // 돌진체(CHARGER) / 회피체(WEAVER)
    float   chargeTimer = 0.0f;
    int     chargeState = 0;     // 0 접근 / 1 준비(텔레그래프) / 2 돌진
    float   dashDirX = 0.0f, dashDirY = 0.0f;   // SHIELDED 도 재사용(플레이어 방향 = 방패 정면)
    float   weavePhase = 0.0f;
    // 공전체(ORBITER) / 소환체(SPAWNER) / 보호막체(SHIELDED)
    float weaveAmp      = 0.85f;   // WEAVER 지그재그 진폭 (기본)
    float contactDmg    = 5.0f;    // 접촉 초당 피해
    float singularityGrace = 0.0f; // 특이점 필드 — 접촉 피해 면역(초)
    float   orbitAngle  = 0.0f;
    float   orbitRadius = 0.0f;
    float   spawnTimer  = 0.0f;
    bool    anchored    = false;   // SPAWNER(봇넷 노드) — 사정거리 도달 후 고정(추격 X)
    // Hive FSM (SPAWNER 전용)
    int     hivePhase      = 0;     // 0=ORBIT 1=OPEN 2=SPAWN 3=CLOSE
    float   hiveOpenFactor = 0.0f;  // 괄호 궤도 확장 (0=닫힘, 1=완전 개방)
    bool    hiveSpawned    = false; // 현재 사이클 소환 완료 여부
    int     hiveSpawnCount = 0;     // SPAWN phase emits one child per interval
    float   hiveOrbitAngle = 0.0f;  // shared rigid rotation for shell, scope, and egress axis
    float   hivePulseTimer = 0.0f;  // short primary sight-texture pulse after each summon
    bool    genesisEgress = false;  // summoned child exits through one of two fixed lanes
    float   genesisEgressX = 0.0f, genesisEgressY = 0.0f;
    float   genesisEgressTimer = 0.0f;
    bool    shieldActive = true;
    float   shieldTimer  = 0.0f;
    float   burnTimer    = 0.0f;   // 은탄환 화상 DoT
    float   burnDps      = 0.0f;

    // QUASAR FSM: cooldown -> acquire -> precision lock -> beam -> recovery.
    int     quasarState  = 0;
    float   quasarTimer  = 0.0f;
    float   quasarAimX   = 1.0f, quasarAimY = 0.0f;
    float   quasarVisualAngle = 0.0f;
    float   quasarMoveAngle = 0.0f;
    float   quasarMoveTurn = 0.0f;
    float   quasarMoveTimer = 0.0f;
    float   gravisVisualAngle = 0.0f;
    float   gravisDriftAngle = 0.0f;
    float   gravisDriftTimer = 0.0f;

    static constexpr float BLINK_INTERVAL = 1.8f;   // 점멸 주기
    static constexpr float BLINK_WARN      = 0.40f; // 점멸 전 잔상 경고
    static constexpr float BLINK_CLOSE     = 0.55f; // 플레이어 쪽으로 55% 점프

    void BeginGenesisEgress(float dirX, float dirY) {
        const float len = std::sqrt(dirX * dirX + dirY * dirY);
        if (len <= 0.0001f) return;
        genesisEgressX = dirX / len;
        genesisEgressY = dirY / len;
        genesisEgressTimer = 0.0f;
        genesisEgress = true;
    }

    Monster(float startX, float startY,
            float hpMul = 1.0f, float speedMul = 1.0f,
            bool isSummoned = false)
        : worldX(startX), worldY(startY) {
        hp    *= hpMul;
        speed  = (120.0f + (float)(rand() % 61)) * speedMul; // 120~180
        summoned = isSummoned;
        color = glm::vec3(1.0f, 0.27f, 0.0f);   // Rotor: orange-red default
        if (summoned) color = glm::vec3(0.9f, 0.3f, 0.3f);
    }

    // 종류 지정 — 생성 직후 호출 (체력/속도/색 보정)
    void MakeKind(MobKind k, int gen = 0, float scale = 1.0f) {
        kind = k; splitGen = gen; sizeScale = scale;
        if (k == MobKind::SPLITTER) {
            color = glm::vec3(0.35f, 0.9f, 0.45f);      // 초록 점액
            hp   *= 0.9f * scale;                       // 세대 작을수록 체력↓
            speed = speed * (0.85f + 0.15f * (float)gen);
        } else if (k == MobKind::BLINKER) {
            color = glm::vec3(0.7f, 0.4f, 1.0f);        // 보라/점멸
            hp   *= 0.55f;                              // 약하지만 잡기 까다로움
            blinkTimer = (float)(rand() % 100) * 0.01f; // 위상 분산
        } else if (k == MobKind::CHARGER) {
            color = glm::vec3(1.0f, 0.55f, 0.15f);      // 주황 — 돌진 전 텔레그래프
            hp   *= 1.1f;
            speed *= 0.8f;                              // 평소 느림, 돌진 시 폭발적
            chargeTimer = (float)(rand() % 100) * 0.01f;
        } else if (k == MobKind::WEAVER) {
            color = glm::vec3(0.2f, 0.9f, 1.0f);
            hp   *= 0.7f;
            speed *= 1.15f;
            weavePhase = (float)(rand() % 628) * 0.01f;
            weaveAmp   = 0.85f;
        } else if (k == MobKind::BRUTE) {
            color = glm::vec3(0.65f, 0.12f, 0.15f);
            hp   *= 3.2f;
            speed *= 0.55f;
            sizeScale = scale * 2.2f;
            contactDmg = 5.0f;
        } else if (k == MobKind::ORBITER) {
            color = glm::vec3(1.0f, 0.85f, 0.2f);       // 노랑 — 공전하며 스파이럴 인
            hp   *= 0.6f;
            speed *= 1.2f;
            orbitRadius = 270.0f + (float)(rand() % 90);
            orbitAngle  = (float)(rand() % 628) * 0.01f;
        } else if (k == MobKind::SPAWNER) {
            color = glm::vec3(0.2f, 0.75f, 0.55f);
            hp   *= 3.5f;
            speed *= 0.35f;
            sizeScale = scale * 2.2f;
            spawnTimer = 0.0f;
            hivePhase = 0; hiveOpenFactor = 0.0f; hiveSpawned = false;
            hiveSpawnCount = 0;
            hiveOrbitAngle = (float)(rand() % 628) * 0.01f;
        } else if (k == MobKind::SHIELDED) {
            color = glm::vec3(0.3f, 0.5f, 1.0f);        // 파랑 — 주기적 보호막
            speed *= 0.85f;
            shieldActive = true; shieldTimer = 0.0f;
        } else if (k == MobKind::DDOS) {
            color = glm::vec3(0.55f, 0.0f, 0.0f);       // 버건디 — 작은 노드 떼
            hp   *= 0.35f;
            speed *= 1.05f;
            sizeScale = scale * 1.0f;
        } else if (k == MobKind::BADSECTOR) {
            color = glm::vec3(0.6f, 0.25f, 0.85f);      // 보라 — 손상 섹터(육각)
            hp   *= 1.3f;
            speed *= 0.8f;
            sizeScale = scale * 1.2f;
        } else if (k == MobKind::REGERROR) {
            color = glm::vec3(1.0f, 0.3f, 0.3f);        // 적 — X형 노드
            hp   *= 2.0f;
            speed *= 0.6f;
            sizeScale = scale * 1.25f;
            orbitAngle = 0.0f;
        } else if (k == MobKind::GRAVIS) {
            color = glm::vec3(0.58f, 0.42f, 1.0f);
            hp *= 8.0f;
            speed *= 0.24f;
            sizeScale = scale * 2.65f;
            contactDmg = 6.0f;
            gravisVisualAngle = (float)(rand() % 628) * 0.01f;
            gravisDriftAngle = (float)(rand() % 628) * 0.01f;
            gravisDriftTimer = 1.0f + (float)(rand() % 120) * 0.01f;
        } else if (k == MobKind::QUASAR) {
            color = glm::vec3(0.42f, 0.56f, 1.0f);
            hp *= 6.4f;                 // 576 base HP, roughly 1.6x SCOPE
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

    // 엘리트 변종 부여 — 종류 지정(MakeKind) 뒤에 호출
    void MakeElite(int e) {
        elite = e;
        if (e == 1)      { speed *= 1.6f; }                          // 신속
        else if (e == 2) { hp *= 2.2f; sizeScale *= 1.35f; speed *= 0.85f; } // 강인
        else if (e == 3) { hp *= 1.3f; speed *= 0.65f; }            // 폭발성(빨강) — 무거워 느리게
    }

    void TryContact(float dist, float& playerHP, float dps, float deltaTime,
                    float thresh = -1.0f,
                    float gateWX = -1.0f, float gateWY = -1.0f,
                    float gateWW = -1.0f, float gateWH = -1.0f) const {
        if (singularityGrace > 0.0f) return;
        if (gateWW > 0.0f) {
            float m = 14.0f * sizeScale;
            if (worldX < gateWX - m || worldX > gateWX + gateWW + m ||
                worldY < gateWY - m || worldY > gateWY + gateWH + m)
                return;
        }
        float t = (thresh >= 0.0f) ? thresh : (26.0f * sizeScale);
        if (dist < t) HurtPlayer(playerHP, dps * deltaTime);
    }

    void Update(float playerCX, float playerCY, float deltaTime,
                float& playerHP, float speedMult = 1.0f,
                float gateWX = -1.0f, float gateWY = -1.0f,
                float gateWW = -1.0f, float gateWH = -1.0f) {
        if (!alive) return;
        if (singularityGrace > 0.0f) {
            singularityGrace -= deltaTime;
            if (singularityGrace < 0.0f) singularityGrace = 0.0f;
        }
        if (burnTimer > 0.0f) {
            float tick = burnDps * deltaTime;
            hp -= tick;
            burnTimer -= deltaTime;
            if (burnTimer <= 0.0f) { burnTimer = 0.0f; burnDps = 0.0f; }
            if (hp <= 0.0f) { hp = 0.0f; alive = false; return; }
        }
        float dx = playerCX - worldX;
        float dy = playerCY - worldY;
        float dist = std::sqrt(dx * dx + dy * dy) + 1e-4f;

        if (kind == MobKind::GRAVIS) {
            gravisVisualAngle += deltaTime * 0.72f;
            gravisDriftTimer -= deltaTime;
            if (gravisDriftTimer <= 0.0f) {
                gravisDriftAngle += ((float)(rand() % 101) - 50.0f) * 0.012f;
                gravisDriftTimer = 1.2f + (float)(rand() % 140) * 0.01f;
            }
            worldX += cosf(gravisDriftAngle) * speed * speedMult * deltaTime;
            worldY += sinf(gravisDriftAngle) * speed * speedMult * deltaTime;
            TryContact(dist, playerHP, contactDmg, deltaTime);
            return;
        }

        if (kind == MobKind::QUASAR) {
            static constexpr float COOLDOWN_TIME = 3.2f;
            static constexpr float LOCK_TIME     = 0.55f;
            static constexpr float BEAM_TIME     = 2.50f;
            static constexpr float RECOVER_TIME  = 1.4f;
            static constexpr float BEAM_LENGTH   = 6000.0f;
            static constexpr float BEAM_RADIUS   = 34.0f;
            static constexpr float BEAM_DPS      = 34.0f;

            quasarTimer += deltaTime;
            // The body always spins at the same rate. Attack states only alter
            // the firing axis, not the self-rotation cadence.
            quasarVisualAngle += deltaTime * 0.08f;

            auto turnTowardPlayer = [&](float radiansPerSecond) {
                float targetX = dx / dist;
                float targetY = dy / dist;
                // The beam is an axis, so use the nearer of the two polar directions.
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
                // Free drift: movement is intentionally unrelated to the player.
                quasarMoveTimer -= deltaTime;
                if (quasarMoveTimer <= 0.0f) {
                    quasarMoveTurn = ((float)(rand() % 121) - 60.0f) * 0.01f;
                    quasarMoveTimer = 0.8f + (float)(rand() % 101) * 0.01f;
                }
                quasarMoveAngle += quasarMoveTurn * deltaTime;
                worldX += cosf(quasarMoveAngle) * speed * speedMult * deltaTime;
                worldY += sinf(quasarMoveAngle) * speed * speedMult * deltaTime;
                if (quasarTimer >= COOLDOWN_TIME) {
                    // Start charging from the current polar axis. The weapon may
                    // fire before it fully faces the player, keeping the sweep
                    // readable instead of snapping into an unavoidable lock.
                    quasarState = 1;
                    quasarTimer = 0.0f;
                }
            } else if (quasarState == 1) {
                // Rotate the polar axis until its infinite beam lane actually
                // intersects the player. It can begin from any facing angle.
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
                // Limit the beam's tangential movement in world space. With a
                // fixed angular velocity, a distant player sees v = r * omega
                // grow with distance, making the laser sweep unfairly fast.
                // The inverse-distance angular cap keeps the visible sweep
                // readable while preserving the original near-range cadence.
                const float sweepLinearSpeed = 260.0f;
                const float sweepAngularSpeed = std::max(0.035f,
                    std::min(0.18f, sweepLinearSpeed / std::max(dist, 180.0f)));
                turnTowardPlayer(sweepAngularSpeed);
                const float growT = std::min(1.0f, quasarTimer / 0.70f);
                const float reachT = growT * growT * growT;
                const float reach = BEAM_LENGTH * reachT;
                const float ax = worldX - quasarAimX * reach;
                const float ay = worldY - quasarAimY * reach;
                const float bx = worldX + quasarAimX * reach;
                const float by = worldY + quasarAimY * reach;
                const float abx = bx - ax, aby = by - ay;
                const float len2 = abx * abx + aby * aby;
                if (len2 > 0.001f) {
                    float u = ((playerCX - ax) * abx +
                               (playerCY - ay) * aby) / len2;
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

            TryContact(dist, playerHP, contactDmg, deltaTime, -1.0f,
                       gateWX, gateWY, gateWW, gateWH);
            return;
        }

        // GENESIS children are born at the core, then use the current open
        // lane before returning to their normal player-seeking behavior.
        if (genesisEgress) {
            genesisEgressTimer += deltaTime;
            const float egressSpeed = speed * 1.55f * speedMult;
            worldX += genesisEgressX * egressSpeed * deltaTime;
            worldY += genesisEgressY * egressSpeed * deltaTime;
            if (genesisEgressTimer >= 0.62f) {
                genesisEgressTimer = 0.0f;
                genesisEgress = false;
            }
            TryContact(dist, playerHP, contactDmg, deltaTime, -1.0f,
                       gateWX, gateWY, gateWW, gateWH);
            return;
        }

        if (kind == MobKind::BLINKER) {
            // 점멸체 — 평소 느리게 표류, 주기마다 잔상 경고 후 플레이어 쪽으로 순간이동
            blinkTimer += deltaTime;
            if (!blinkWarn && blinkTimer >= BLINK_INTERVAL * blinkIntervalMul) {
                blinkWarn = true; blinkWarnT = 0.0f;
                float jump = dist * BLINK_CLOSE;
                blinkTargetX = worldX + (dx / dist) * jump;
                blinkTargetY = worldY + (dy / dist) * jump;
            }
            if (blinkWarn) {
                blinkWarnT += deltaTime;
                if (blinkWarnT >= BLINK_WARN) {        // 점멸 실행
                    worldX = blinkTargetX; worldY = blinkTargetY;
                    blinkWarn = false; blinkTimer = 0.0f;
                }
            } else if (dist > 5.0f) {                  // 느린 표류
                worldX += (dx / dist) * speed * 0.30f * speedMult * deltaTime;
                worldY += (dy / dist) * speed * 0.30f * speedMult * deltaTime;
            }
            TryContact(dist, playerHP, 7.0f, deltaTime, 26.0f,
                       gateWX, gateWY, gateWW, gateWH);
            return;
        }

        if (kind == MobKind::CHARGER) {
            // 돌진체 — 접근 → 멈춰서 준비(텔레그래프) → 플레이어 쪽으로 폭발 돌진
            if (chargeState == 0) {                     // 접근
                if (dist > 5.0f) {
                    worldX += (dx / dist) * speed * speedMult * deltaTime;
                    worldY += (dy / dist) * speed * speedMult * deltaTime;
                }
                chargeTimer += deltaTime;
                if (chargeTimer >= 1.8f && dist < 480.0f) { chargeState = 1; chargeTimer = 0.0f; }
            } else if (chargeState == 1) {              // 준비 (멈춤 + 조준)
                dashDirX = dx / dist; dashDirY = dy / dist;
                chargeTimer += deltaTime;
                if (chargeTimer >= 0.5f) { chargeState = 2; chargeTimer = 0.0f; }
            } else {                                    // 돌진
                worldX += dashDirX * speed * 4.0f * speedMult * deltaTime;
                worldY += dashDirY * speed * 4.0f * speedMult * deltaTime;
                chargeTimer += deltaTime;
                if (chargeTimer >= 0.35f) { chargeState = 0; chargeTimer = 0.0f; }
            }
            TryContact(dist, playerHP, 6.0f, deltaTime, -1.0f,
                       gateWX, gateWY, gateWW, gateWH);
            return;
        }

        if (kind == MobKind::WEAVER) {
            // 회피체 — 전진하면서 좌우로 지그재그 (조준 까다로움)
            weavePhase += deltaTime * 6.5f;
            if (dist > 5.0f) {
                float fX = dx / dist, fY = dy / dist;
                float pX = -fY, pY = fX;               // 진행방향 수직
                float w  = sinf(weavePhase) * weaveAmp;
                worldX += (fX * speed + pX * speed * w) * speedMult * deltaTime;
                worldY += (fY * speed + pY * speed * w) * speedMult * deltaTime;
            }
            TryContact(dist, playerHP, 5.0f, deltaTime, -1.0f,
                       gateWX, gateWY, gateWW, gateWH);
            return;
        }

        if (kind == MobKind::ORBITER) {
            // 공전 위성 — 플레이어 주위를 돌며 반경을 서서히 줄여 스파이럴 인
            orbitAngle  += deltaTime * 1.7f * speedMult;
            orbitRadius -= 24.0f * deltaTime;
            if (orbitRadius < 14.0f) orbitRadius = 14.0f;
            float tx = playerCX + cosf(orbitAngle) * orbitRadius;
            float ty = playerCY + sinf(orbitAngle) * orbitRadius;
            float k = std::min(1.0f, 7.0f * deltaTime);
            worldX += (tx - worldX) * k;
            worldY += (ty - worldY) * k;
            TryContact(dist, playerHP, 5.0f, deltaTime, -1.0f,
                       gateWX, gateWY, gateWW, gateWH);
            return;
        }

        if (kind == MobKind::SHIELDED) {
            // 보호막체 — 평소 접근. 주기적으로 방패 ON(2s)/OFF(1.6s). OFF 일 때만 약점.
            dashDirX = dx / dist; dashDirY = dy / dist;   // 방패 정면 = 플레이어 방향
            shieldTimer += deltaTime;
            if (shieldActive && shieldTimer >= 2.0f)      { shieldActive = false; shieldTimer = 0.0f; }
            else if (!shieldActive && shieldTimer >= 1.6f){ shieldActive = true;  shieldTimer = 0.0f; }
            if (dist > 5.0f) {
                worldX += (dx / dist) * speed * speedMult * deltaTime;
                worldY += (dy / dist) * speed * speedMult * deltaTime;
            }
            TryContact(dist, playerHP, 5.0f, deltaTime, -1.0f,
                       gateWX, gateWY, gateWW, gateWH);
            return;
        }

        // SPAWNER(봇넷 노드) — 사정거리까지만 진입 후 제자리 고정. 플레이어를 추격하지 않음.
        //   (소환사가 끝까지 쫓아오는 게 이상해서 변경. 잡몹 소환은 그대로, 총알은 안 쏨.)
        if (kind == MobKind::SPAWNER) {
            // 봇넷 노드 — 플레이어를 쫓지 않고 정해진 자리(blinkTarget=스폰 시 설정)로 이동 후 고정
            if (!anchored) {
                float hx = blinkTargetX - worldX, hy = blinkTargetY - worldY;
                float hd = std::sqrt(hx*hx + hy*hy);
                if (hd > 12.0f) {
                    worldX += (hx / hd) * speed * speedMult * deltaTime;
                    worldY += (hy / hd) * speed * speedMult * deltaTime;
                } else {
                    anchored = true;   // 배치 완료 → 이후 영구 고정
                }
            }
            TryContact(dist, playerHP, contactDmg, deltaTime, -1.0f,
                       gateWX, gateWY, gateWW, gateWH);
            return;
        }

        // NORMAL / SPLITTER / BRUTE — 플레이어 추격
        if (dist > 5.0f) {
            worldX += (dx / dist) * speed * speedMult * deltaTime;
            worldY += (dy / dist) * speed * speedMult * deltaTime;
        }
        TryContact(dist, playerHP, contactDmg, deltaTime, -1.0f,
                   gateWX, gateWY, gateWW, gateWH);
    }
};
