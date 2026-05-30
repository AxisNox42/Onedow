#pragma once
#include <glm/glm.hpp>
#include <cmath>

class Bullet {
public:
    float x, y;
    float dirX, dirY;
    float speed = 1200.0f;
    bool active = true;
    bool isEnemy = false;

    // 유도탄 (탄환 세례 등) — 매 프레임 가장 가까운 적 쪽으로 방향 보정
    bool  homing       = false;
    float homingTurn   = 5.0f;   // 라디안/초
    // 개별 데미지 배율 (탄환 세례 등 0.5x)
    float dmgMult      = 1.0f;
    // 대포(CANNON) 잔존 데미지 — >0 면 cannon 모드 (이 값으로 데미지 적용, 죽인 적 hp 만큼 차감, 0 될 때까지 관통)
    float remainingDmg = 0.0f;
    // 사거리 제한 (SHOTGUN 등). 0 = 무제한. 누적 이동거리가 maxRange 넘으면 deactivate
    float maxRange  = 0.0f;
    float traveled  = 0.0f;
    // 렌더 크기 배율 (CANNON 총알 5배 등)
    float sizeScale = 1.0f;

    glm::vec3 color = glm::vec3(1.0f, 1.0f, 0.0f);

    Bullet(float startX, float startY, float targetX, float targetY) {
        x = startX; y = startY;
        float dx = targetX - startX;
        float dy = targetY - startY;
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist > 0.0f) { dirX = dx / dist; dirY = dy / dist; }
        else              { dirX = 0.0f;      dirY = -1.0f;     }
    }

    void Update(float dt) {
        float step = speed * dt;
        x += dirX * step;
        y += dirY * step;
        if (maxRange > 0.0f) {
            traveled += step;
            if (traveled > maxRange) active = false;
        }
    }
};
