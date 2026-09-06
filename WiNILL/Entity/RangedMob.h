#pragma once
#include <glm/glm.hpp>
#include <cmath>
#include <cstdlib>
#include <vector>
#include "Bullet.h"
#include "Settings.h"   // g_ArenaExX/Y (폴리모프 페이즈2 확장 아레나)

class RangedMob {
public:
    static constexpr float BASE_HP = 360.0f;
    static constexpr float VISUAL_BASE_PX = 16.0f * 1.6f;

    float worldX, worldY;
    float targetX, targetY;
    float hp         = BASE_HP;
    bool  alive      = true;
    bool  exploded   = false;
    bool  scored     = false;
    float deathScale = 1.0f;
    glm::vec3 color = glm::vec3(0.15f, 0.85f, 1.0f);   // Lens: icy cyan

    float wanderTimer = 0.0f;
    float rotAngle    = 0.0f;   // Layer 1 accumulated angle

    // FSM
    enum class State { IDLE, CHARGING, BURST };
    State lensState   = State::IDLE;
    float fireTimer   = 0.0f;   // IDLE countdown to next charge
    float chargeAngle = 0.0f;   // accumulated rotation during CHARGING
    float chargeSpeed = 0.0f;   // current spin speed during CHARGING
    float burstTimer  = 0.0f;   // time since entering BURST
    int   burstShots  = 0;      // shots fired in current burst

    static constexpr float WANDER_INTERVAL = 3.0f;
    static constexpr float FIRE_INTERVAL   = 4.0f;   // idle gap between bursts
    static constexpr float IDLE_ROT        = 0.40f;  // rad/s (slow idle spin)
    static constexpr float CHARGE_ROT_MAX  = 9.0f;   // rad/s (peak charge spin)
    static constexpr float CHARGE_ROTATIONS = 2.0f;  // full rotations before snap
    static constexpr float SHOT_INTERVAL   = 0.13f;  // seconds between burst shots
    static constexpr float SHOT_DELAY      = 0.08f;  // brief hold after alignment

    int screenW, screenH;

    RangedMob(float sx, float sy, int sw, int sh)
        : worldX(sx), worldY(sy), targetX(sx), targetY(sy)
        , screenW(sw), screenH(sh)
    {
        pickNewTarget();
        wanderTimer = (float)(rand() % 300) / 100.0f;
        fireTimer   = (float)(rand() % 200) / 100.0f;  // stagger initial charge
    }

    void pickNewTarget() {
        float margin = 80.0f;
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
            deathScale -= dt * 3.0f;
            if (deathScale < 0.0f) deathScale = 0.0f;
            return;
        }

        // Wander
        wanderTimer += dt;
        if (wanderTimer >= WANDER_INTERVAL) {
            pickNewTarget();
            wanderTimer = 0.0f;
        }
        worldX += (targetX - worldX) * 4.0f * speedMult * dt;
        worldY += (targetY - worldY) * 4.0f * speedMult * dt;

        // ── FSM ──
        if (lensState == State::IDLE) {
            rotAngle += IDLE_ROT * dt;
            fireTimer += dt;
            if (fireTimer >= FIRE_INTERVAL) {
                lensState    = State::CHARGING;
                chargeAngle  = 0.0f;
                chargeSpeed  = IDLE_ROT;
                fireTimer    = 0.0f;
            }

        } else if (lensState == State::CHARGING) {
            // exponential ramp-up toward CHARGE_ROT_MAX
            chargeSpeed += (CHARGE_ROT_MAX - chargeSpeed) * 5.0f * dt;
            float da = chargeSpeed * dt;
            rotAngle    += da;
            chargeAngle += da;

            if (chargeAngle >= CHARGE_ROTATIONS * 2.0f * 3.14159f) {
                // snap rotAngle to nearest alignment point
                // two layers merge when 2*rotAngle ≡ 0.7854 (mod π/2)
                const float period = 0.7854f;  // π/4
                float mod = fmodf(rotAngle - 0.3927f, period);
                if (mod < 0.0f) mod += period;
                if (mod > period * 0.5f) rotAngle += (period - mod);
                else                     rotAngle -= mod;

                lensState   = State::BURST;
                burstTimer  = 0.0f;
                burstShots  = 0;
            }

        } else {  // BURST
            burstTimer += dt;
            // 3 shots after brief hold, evenly spaced
            for (int i = 0; i < 3; i++) {
                float t = SHOT_DELAY + (float)i * SHOT_INTERVAL;
                if (burstShots == i && burstTimer >= t) {
                    Bullet b(worldX, worldY, playerCX, playerCY);
                    b.speed      = 720.0f;
                    b.isEnemy    = true;
                    b.homing     = true;
                    b.homingTurn = 0.28f;
                    b.color      = glm::vec3(0.15f, 0.85f, 1.0f);
                    bullets.push_back(b);
                    burstShots++;
                    break;
                }
            }
            // back to idle after last shot clears
            if (burstShots >= 3 &&
                burstTimer >= SHOT_DELAY + 2.0f * SHOT_INTERVAL + 0.35f) {
                lensState = State::IDLE;
                fireTimer = 0.0f;
            }
        }
    }
};
