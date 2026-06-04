#pragma once
#include <glm/glm.hpp>
#include <cmath>
#include <cstdlib>

// 잡몹 종류 — 같은 monsters 벡터에서 kind 로 분기 (충돌/렌더 재사용)
//   NORMAL/SPLITTER/BLINKER 외 CHARGER/WEAVER/BRUTE 는 점수 비례 자연 스폰
enum class MobKind { NORMAL, SPLITTER, BLINKER, CHARGER, WEAVER, BRUTE };

// 종류별 처치 보상 — 총알/근접/광역 처치 모두 같은 값 쓰도록 공용화
inline void MobKillReward(MobKind k, int splitGen,
                          float& xpBase, float& scoreBase) {
    xpBase = 1.0f; scoreBase = 100.0f;
    switch (k) {
    case MobKind::SPLITTER: xpBase = (splitGen >= 2) ? 1.0f : 2.0f; scoreBase = 120.0f; break;
    case MobKind::BLINKER:  xpBase = 6.0f; scoreBase = 250.0f; break;
    case MobKind::CHARGER:  xpBase = 3.0f; scoreBase = 180.0f; break;
    case MobKind::WEAVER:   xpBase = 3.0f; scoreBase = 160.0f; break;
    case MobKind::BRUTE:    xpBase = 8.0f; scoreBase = 400.0f; break;
    default: break;
    }
}

class Monster {
public:
    float worldX, worldY;
    float speed = 150.0f;  // 생성자에서 120~180 랜덤
    float hp = 90.0f;
    bool  alive    = true;
    bool  exploded = false;  // 사망 폭발/분열 1회만 처리하는 플래그
    bool  scored   = false;  // 처치 보상(EXP/점수/콤보) 지급 완료 플래그
    bool  summoned = false;  // 보스 소환물 (더 크게)
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
    // 돌진체(CHARGER) / 회피체(WEAVER)
    float   chargeTimer = 0.0f;
    int     chargeState = 0;     // 0 접근 / 1 준비(텔레그래프) / 2 돌진
    float   dashDirX = 0.0f, dashDirY = 0.0f;
    float   weavePhase = 0.0f;

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
            hp   *= 4.5f;
            speed *= 0.55f;
            sizeScale = scale * 2.2f;
        }
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
            if (!blinkWarn && blinkTimer >= BLINK_INTERVAL) {
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
            if (dist < 22.0f) playerHP -= 7.0f * deltaTime;
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
            if (dist < 20.0f * sizeScale) playerHP -= 6.0f * deltaTime;
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
            if (dist < 20.0f * sizeScale) playerHP -= 5.0f * deltaTime;
            return;
        }

        // NORMAL / SPLITTER / BRUTE — 플레이어 추격
        if (dist > 5.0f) {
            worldX += (dx / dist) * speed * speedMult * deltaTime;
            worldY += (dy / dist) * speed * speedMult * deltaTime;
        }
        if (dist < 20.0f * sizeScale) playerHP -= 5.0f * deltaTime;
    }
};
