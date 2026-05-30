#pragma once
#include <glm/glm.hpp>
#include <cmath>
#include <cstdlib>

class Monster {
public:
    float worldX, worldY;
    float speed = 150.0f;  // 생성자에서 120~180 랜덤으로 재설정됨
    float hp = 90.0f;        // 50→90 (소프트 콜리전 보상 + 전반 체력 상향)
    bool  alive    = true;
    bool  exploded = false;  // 사망 폭발 이펙트 1회만 spawn 하기 위한 플래그
    bool  summoned = false;  // 보스 소환물 (시각적으로 더 크게 그림)
    glm::vec3 color;

    Monster(float startX, float startY,
            float hpMul = 1.0f, float speedMul = 1.0f,
            bool isSummoned = false)
        : worldX(startX), worldY(startY) {
        hp    *= hpMul;
        speed  = (120.0f + (float)(rand() % 61)) * speedMul; // 120~180 랜덤
        summoned = isSummoned;
        color = glm::vec3(
            (rand() % 60 + 40) / 100.0f,
            (rand() % 60 + 40) / 100.0f,
            (rand() % 60 + 40) / 100.0f
        );
        if (summoned) {
            // 소환물은 보라/붉은 톤
            color = glm::vec3(0.9f, 0.3f, 0.3f);
        }
    }

    void Update(float playerCX, float playerCY, float deltaTime,
                float& playerHP, float speedMult = 1.0f) {
        if (!alive) return;
        float dx = playerCX - worldX;
        float dy = playerCY - worldY;
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist > 5.0f) {
            worldX += (dx / dist) * speed * speedMult * deltaTime;
            worldY += (dy / dist) * speed * speedMult * deltaTime;
        }
        if (dist < 20.0f) {
            playerHP -= 5.0f * deltaTime;
        }
    }
};
