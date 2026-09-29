#include "ProjectileSystem.h"

#include "Bullet.h"
#include "MonsterManager.h"
#include "../System/FakeWindow.h"
#include <algorithm>
#include <cmath>

namespace {
float TurnAngleToward(float currentAngle, float targetAngle, float maxStep) {
    constexpr float kPi = 3.14159265f;
    constexpr float kTau = 6.2831853f;
    float difference = targetAngle - currentAngle;
    while (difference > kPi) difference -= kTau;
    while (difference < -kPi) difference += kTau;
    difference = std::max(-maxStep, std::min(maxStep, difference));
    return currentAngle + difference;
}
}

void UpdateProjectiles(std::vector<Bullet>& bullets,
                       const MonsterManager& monsters,
                       const SpatialBounds& playerBounds,
                       float fixedDelta, float timeStopTimer,
                       float hyperFocusTimer,
                       float screenWidth, float screenHeight,
                       float viewZoom) {
    const float playerX = playerBounds.x + playerBounds.width * 0.5f;
    const float playerY = playerBounds.y + playerBounds.height * 0.5f;
    const float homingDelta = fixedDelta * (hyperFocusTimer > 0.0f ? 0.7f : 1.0f);

    for (auto& bullet : bullets) {
        if (bullet.homing && !bullet.isEnemy && bullet.active) {
            float targetX = 0.0f, targetY = 0.0f;
            if (monsters.FindNearestEnemy(
                    bullet.x, bullet.y,
                    playerBounds.x, playerBounds.y,
                    playerBounds.x + playerBounds.width,
                    playerBounds.y + playerBounds.height,
                    targetX, targetY)) {
                const float dx = targetX - bullet.x;
                const float dy = targetY - bullet.y;
                const float length = std::sqrt(dx * dx + dy * dy);
                if (length > 0.001f) {
                    const float currentAngle = std::atan2(bullet.dirY, bullet.dirX);
                    const float targetAngle = std::atan2(dy / length, dx / length);
                    const float nextAngle = TurnAngleToward(
                        currentAngle, targetAngle, bullet.homingTurn * fixedDelta);
                    bullet.dirX = std::cos(nextAngle);
                    bullet.dirY = std::sin(nextAngle);
                }
            }
        }

        if (bullet.homing && bullet.isEnemy && bullet.active && timeStopTimer <= 0.0f) {
            const float dx = playerX - bullet.x;
            const float dy = playerY - bullet.y;
            const float length = std::sqrt(dx * dx + dy * dy);
            if (length > 300.0f) {
                const float currentAngle = std::atan2(bullet.dirY, bullet.dirX);
                const float targetAngle = std::atan2(dy / length, dx / length);
                const float nextAngle = TurnAngleToward(
                    currentAngle, targetAngle, bullet.homingTurn * homingDelta);
                bullet.dirX = std::cos(nextAngle);
                bullet.dirY = std::sin(nextAngle);
            }
        }

        if (!(bullet.isEnemy && timeStopTimer > 0.0f)) {
            const float bulletDelta = bullet.isEnemy && hyperFocusTimer > 0.0f
                ? fixedDelta * 0.7f : fixedDelta;
            bullet.Update(bulletDelta);
        }

        const float safeZoom = std::max(viewZoom, 0.01f);
        const float marginX = screenWidth * 0.5f * (1.0f / safeZoom - 1.0f) + 200.0f;
        const float marginY = screenHeight * 0.5f * (1.0f / safeZoom - 1.0f) + 200.0f;
        if (bullet.x < -marginX || bullet.x > screenWidth + marginX ||
            bullet.y < -marginY || bullet.y > screenHeight + marginY)
            bullet.active = false;
    }
}
