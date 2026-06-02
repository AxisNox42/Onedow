#pragma once
#include <glm/glm.hpp>
#include <cmath>
#include <cstdlib>

// 잡몹 종류 — 같은 monsters 벡터에서 kind 로 분기 (충돌/렌더 재사용)
enum class MobKind { NORMAL, SPLITTER, BLINKER };

class Monster {
public:
    float worldX, worldY;
    float speed = 150.0f;  // 생성자에서 120~180 랜덤
    float hp = 90.0f;
    bool  alive    = true;
    bool  exploded = false;  // 사망 폭발/분열 1회만 처리하는 플래그
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

        // NORMAL / SPLITTER — 플레이어 추격
        if (dist > 5.0f) {
            worldX += (dx / dist) * speed * speedMult * deltaTime;
            worldY += (dy / dist) * speed * speedMult * deltaTime;
        }
        if (dist < 20.0f * sizeScale) playerHP -= 5.0f * deltaTime;
    }
};
