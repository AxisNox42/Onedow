#pragma once
#include <glm/glm.hpp>
#include <cmath>
#include <cstdlib>
#include <vector>
#include "Bullet.h"
#include "Settings.h"   // g_ArenaExX/Y (폴리모프 페이즈2 확장 아레나)

class RangedMob {
public:
    float worldX, worldY;
    float targetX, targetY;
    float hp         = 150.0f;
    bool  alive      = true;
    bool  exploded   = false;   // 사망 폭발 이펙트 1회만 spawn
    bool  scored     = false;   // 처치 보상 지급 완료 플래그
    float deathScale = 1.0f;    // 사망 애니메이션: 1→0 으로 줄어듦
    glm::vec3 color = glm::vec3(0.8f, 0.0f, 0.8f);

    float wanderTimer = 0.0f;
    float fireTimer   = 0.0f;

    static constexpr float WANDER_INTERVAL = 3.0f;
    static constexpr float FIRE_INTERVAL   = 1.5f;

    int screenW, screenH;

    RangedMob(float sx, float sy, int sw, int sh)
        : worldX(sx), worldY(sy), targetX(sx), targetY(sy)
        , screenW(sw), screenH(sh)
    {
        pickNewTarget();
        wanderTimer = (float)(rand() % 300) / 100.0f;  // stagger wander
        fireTimer   = (float)(rand() % 150) / 100.0f;  // stagger fire
    }

    void pickNewTarget() {
        float margin = 80.0f;
        // 확장 아레나(폴리모프 페이즈2)면 보이는 영역 끝까지 배회
        float minX = -g_ArenaExX + margin, maxX = (float)screenW + g_ArenaExX - margin;
        float minY = -g_ArenaExY + margin, maxY = (float)screenH + g_ArenaExY - margin;
        float rw = maxX - minX, rh = maxY - minY;
        if (rw < 1.0f) rw = 1.0f;
        if (rh < 1.0f) rh = 1.0f;
        targetX = minX + (float)(rand() % (int)rw);
        targetY = minY + (float)(rand() % (int)rh);
    }

    void Update(float playerCX, float playerCY, float dt,
                std::vector<Bullet>& bullets, float speedMult = 1.0f) {
        if (!alive) {
            // 사망 애니메이션: 창이 서서히 수축 (~0.33초)
            deathScale -= dt * 3.0f;
            if (deathScale < 0.0f) deathScale = 0.0f;
            return;
        }

        // Wander: pick new random target every WANDER_INTERVAL seconds
        wanderTimer += dt;
        if (wanderTimer >= WANDER_INTERVAL) {
            pickNewTarget();
            wanderTimer = 0.0f;
        }
        // Smooth lerp toward target
        worldX += (targetX - worldX) * 4.0f * speedMult * dt;
        worldY += (targetY - worldY) * 4.0f * speedMult * dt;

        // Fire a red bullet at the player every FIRE_INTERVAL seconds
        fireTimer += dt;
        if (fireTimer >= FIRE_INTERVAL) {
            Bullet b(worldX, worldY, playerCX, playerCY);
            b.speed   = 600.0f;
            b.isEnemy = true;
            b.color   = glm::vec3(0.8f, 0.1f, 0.1f);
            bullets.push_back(b);
            fireTimer = 0.0f;
        }
    }
};
