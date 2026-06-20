#pragma once
#include <glm/glm.hpp>
#include <cmath>
#include <cstdlib>
#include <algorithm>

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
    REGERROR     // 레지스트리 에러 — X본체+공전, 가짜창 내 적 강화 오라 (디버프/자연 스폰)
};

// 엘리트 변종 — 0 없음 / 1 신속 / 2 강인 / 3 폭발성. 어떤 잡몹에든 드물게 부여.
enum class EliteMod { NONE = 0, SWIFT = 1, TANKY = 2, VOLATILE = 3 };

// 종류별 처치 보상 — 총알/근접/광역 처치 모두 같은 값 쓰도록 공용화 (엘리트면 ×2.5)
inline void MobKillReward(MobKind k, int splitGen, int elite,
                          float& xpBase, float& scoreBase) {
    xpBase = 1.0f; scoreBase = 100.0f;
    switch (k) {
    case MobKind::SPLITTER: xpBase = (splitGen >= 2) ? 1.0f : 2.0f; scoreBase = 120.0f; break;
    case MobKind::BLINKER:  xpBase = 6.0f; scoreBase = 250.0f; break;
    case MobKind::CHARGER:  xpBase = 3.0f; scoreBase = 180.0f; break;
    case MobKind::WEAVER:   xpBase = 3.0f; scoreBase = 160.0f; break;
    case MobKind::BRUTE:    xpBase = 8.0f; scoreBase = 400.0f; break;
    case MobKind::ORBITER:  xpBase = 5.0f; scoreBase = 200.0f; break;
    case MobKind::SPAWNER:  xpBase = 7.0f; scoreBase = 300.0f; break;
    case MobKind::SHIELDED: xpBase = 5.0f; scoreBase = 220.0f; break;
    case MobKind::DDOS:     xpBase = 0.5f; scoreBase = 40.0f;  break;  // 물량 swarm — 보상 미미
    case MobKind::BADSECTOR:xpBase = 30.0f;scoreBase = 350.0f; break;
    case MobKind::REGERROR: xpBase = 40.0f;scoreBase = 500.0f; break;
    default: break;
    }
    if (elite) { xpBase *= 2.5f; scoreBase *= 2.5f; }
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
    float   orbitAngle  = 0.0f;
    float   orbitRadius = 0.0f;
    float   spawnTimer  = 0.0f;
    bool    anchored    = false;   // SPAWNER(봇넷 노드) — 사정거리 도달 후 고정(추격 X)
    bool    shieldActive = true;
    float   shieldTimer  = 0.0f;

    static constexpr float BLINK_INTERVAL = 1.8f;   // 점멸 주기
    static constexpr float BLINK_WARN      = 0.40f; // 점멸 전 잔상 경고
    static constexpr float BLINK_CLOSE     = 0.55f; // 플레이어 쪽으로 55% 점프

    Monster(float startX, float startY,
            float hpMul = 1.0f, float speedMul = 1.0f,
            bool isSummoned = false)
        : worldX(startX), worldY(startY) {
        hp    *= hpMul;
        speed  = (120.0f + (float)(rand() % 61)) * speedMul; // 120~180
        summoned = isSummoned;
        color = glm::vec3(
            (rand() % 60 + 40) / 100.0f,
            (rand() % 60 + 40) / 100.0f,
            (rand() % 60 + 40) / 100.0f
        );
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
            color = glm::vec3(0.2f, 0.9f, 1.0f);        // 시안 — 좌우로 흔들며 접근
            hp   *= 0.7f;
            speed *= 1.15f;
            weavePhase = (float)(rand() % 628) * 0.01f;
        } else if (k == MobKind::BRUTE) {
            color = glm::vec3(0.65f, 0.12f, 0.15f);     // 짙은 적 — 크고 단단함
            hp   *= 3.2f;                               // 너프: 4.5 → 3.2 (안 죽고 쌓이던 문제)
            speed *= 0.55f;
            sizeScale = scale * 2.2f;
        } else if (k == MobKind::ORBITER) {
            color = glm::vec3(1.0f, 0.85f, 0.2f);       // 노랑 — 공전하며 스파이럴 인
            hp   *= 0.6f;
            speed *= 1.2f;
            orbitRadius = 270.0f + (float)(rand() % 90);
            orbitAngle  = (float)(rand() % 628) * 0.01f;
        } else if (k == MobKind::SPAWNER) {
            color = glm::vec3(0.2f, 0.75f, 0.55f);      // 청록 — 작은 잡몹 소환
            hp   *= 2.4f;
            speed *= 0.5f;
            spawnTimer = 2.5f;
        } else if (k == MobKind::SHIELDED) {
            color = glm::vec3(0.3f, 0.5f, 1.0f);        // 파랑 — 주기적 보호막
            speed *= 0.85f;
            shieldActive = true; shieldTimer = 0.0f;
        } else if (k == MobKind::DDOS) {
            color = glm::vec3(1.0f, 0.35f, 0.55f);      // 분홍빨강 — 작은 프로세스 떼
            hp   *= 0.35f;                              // 매우 약함 (물량)
            speed *= 1.05f;
            sizeScale = scale * 0.72f;                  // 작지만 보이게
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
        }
    }

    // 엘리트 변종 부여 — 종류 지정(MakeKind) 뒤에 호출
    void MakeElite(int e) {
        elite = e;
        if (e == 1)      { speed *= 1.6f; }                          // 신속
        else if (e == 2) { hp *= 2.2f; sizeScale *= 1.35f; speed *= 0.85f; } // 강인
        else if (e == 3) { hp *= 1.3f; speed *= 0.65f; }            // 폭발성(빨강) — 무거워 느리게
    }

    void Update(float playerCX, float playerCY, float deltaTime,
                float& playerHP, float speedMult = 1.0f) {
        if (!alive) return;
        float dx = playerCX - worldX;
        float dy = playerCY - worldY;
        float dist = std::sqrt(dx * dx + dy * dy) + 1e-4f;

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
            if (dist < 26.0f) playerHP -= 7.0f * deltaTime;
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
            if (dist < 26.0f * sizeScale) playerHP -= 6.0f * deltaTime;
            return;
        }

        if (kind == MobKind::WEAVER) {
            // 회피체 — 전진하면서 좌우로 지그재그 (조준 까다로움)
            weavePhase += deltaTime * 6.5f;
            if (dist > 5.0f) {
                float fX = dx / dist, fY = dy / dist;
                float pX = -fY, pY = fX;               // 진행방향 수직
                float w  = sinf(weavePhase) * 0.85f;
                worldX += (fX * speed + pX * speed * w) * speedMult * deltaTime;
                worldY += (fY * speed + pY * speed * w) * speedMult * deltaTime;
            }
            if (dist < 26.0f * sizeScale) playerHP -= 5.0f * deltaTime;
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
            if (dist < 26.0f * sizeScale) playerHP -= 5.0f * deltaTime;
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
            if (dist < 26.0f * sizeScale) playerHP -= 5.0f * deltaTime;
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
            if (dist < 26.0f * sizeScale) playerHP -= 5.0f * deltaTime;
            return;
        }

        // NORMAL / SPLITTER / BRUTE — 플레이어 추격
        if (dist > 5.0f) {
            worldX += (dx / dist) * speed * speedMult * deltaTime;
            worldY += (dy / dist) * speed * speedMult * deltaTime;
        }
        if (dist < 26.0f * sizeScale) playerHP -= 5.0f * deltaTime;
    }
};
