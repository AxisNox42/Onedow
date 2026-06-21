#pragma once
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#include "Bullet.h"
#include "PlayerStats.h"

// ── POLYMORPH.vir — single-phase shape-shifter ──
//   TRIANGLE : edge wave rows (kill swarm for EXP, dodge for HP)
//   RHOMBUS  : telegraphed screen-edge laser toward player
//   DIAMOND  : reflects player bullets (wait for form swap or swarm DPS)
//   Enrage at 40% HP: faster form swap + patterns (no zoom / no adds)

enum class PForm { TRIANGLE, RHOMBUS, DIAMOND };

struct PSwarm { float x, y, vx, vy; float life; bool alive; };

class PolymorphBoss {
public:
    float worldX, worldY;
    float hp, maxHp;
    bool  alive = true, exploded = false;
    bool  enraged = false;
    int   screenW, screenH;

    PForm form = PForm::TRIANGLE;
    float formTimer = 0.0f;
    float formDuration = 8.0f;

    float targetX, targetY, wanderTimer = 0.0f;

    std::vector<PSwarm> swarm;
    bool  triWarn   = false;
    bool  triActive = false;
    float triWarnTimer = 0.0f;
    float triTimer     = 0.0f;
    float triCd        = 0.5f;
    float triSpawnAccum = 0.0f;
    float triDirX = 1.0f, triDirY = 0.0f;

    bool  laserActive = false;
    bool  laserWarn   = false;
    float laserWarnTimer = 0.0f;
    float laserX = 0, laserY = 0, laserDirX = 1, laserDirY = 0;
    float laserTimer = 0.0f, laserCd = 0.0f;
    float laserHalf = 18.0f;

    static constexpr float BODY = 105.0f;
    static constexpr float SWARM_DMG = 5.0f;
    static constexpr float LASER_DPS = 11.0f;

    PolymorphBoss(int sw, int sh, float hpInit) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.5f;
        worldY = sh * 0.30f;
        targetX = worldX;
        targetY = worldY;
        pickForm(true);
    }

    bool reflecting() const { return form == PForm::DIAMOND; }
    bool damageable() const { return !reflecting(); }

    void pickNewTarget() {
        float m = 140.0f;
        targetX = m + (float)(rand() % std::max(1, screenW - 2 * (int)m));
        targetY = m + (float)(rand() % std::max(1, screenH - 2 * (int)m));
    }

    void pickForm(bool first) {
        PForm prev = form;
        do { form = (PForm)(rand() % 3); } while (!first && form == prev);
        formTimer = 0.0f;
        formDuration = enraged ? 5.5f : 8.0f;
        triWarn = triActive = false;
        triCd = enraged ? 1.2f : 2.0f;
        triSpawnAccum = 0.0f;
        triTimer = 0.0f;
        laserActive = laserWarn = false;
        laserCd = enraged ? 0.45f : 0.65f;
        laserHalf = enraged ? 24.0f : 18.0f;
    }

    void checkEnrage() {
        if (!enraged && hp <= maxHp * 0.40f) {
            enraged = true;
            formDuration = 5.5f;
            laserHalf = 24.0f;
        }
    }

    void startTriWarn() {
        triWarn = true;
        triWarnTimer = enraged ? 0.70f : 0.90f;
        switch (rand() % 4) {
        case 0: triDirX =  1; triDirY =  0; break;
        case 1: triDirX = -1; triDirY =  0; break;
        case 2: triDirX =  0; triDirY =  1; break;
        default:triDirX =  0; triDirY = -1; break;
        }
    }

    void spawnTriRow() {
        float sp = (enraged ? 780.0f : 680.0f) + (float)(rand() % 120);
        int   perRow = enraged ? 5 : 4;
        float life   = enraged ? 4.0f : 3.2f;
        float minX = 0.0f, maxX = (float)screenW;
        float minY = 0.0f, maxY = (float)screenH;
        int   spanX = std::max(1, (int)(maxX - minX));
        int   spanY = std::max(1, (int)(maxY - minY));
        for (int i = 0; i < perRow; i++) {
            PSwarm s;
            s.life = life;
            s.alive = true;
            if (triDirX != 0.0f) {
                s.x  = (triDirX > 0) ? minX - 20.0f : maxX + 20.0f;
                s.y  = minY + (float)(rand() % spanY);
                s.vx = triDirX * sp;
                s.vy = 0.0f;
            } else {
                s.x  = minX + (float)(rand() % spanX);
                s.y  = (triDirY > 0) ? minY - 20.0f : maxY + 20.0f;
                s.vx = 0.0f;
                s.vy = triDirY * sp;
            }
            swarm.push_back(s);
        }
    }

    float laserReach() const {
        return std::sqrt((float)(screenW * screenW + screenH * screenH)) * 1.15f;
    }

    void aimLaser(float px, float py) {
        float m = 24.0f;
        switch (rand() % 4) {
        case 0: laserX = (float)(rand() % screenW); laserY = -m; break;
        case 1: laserX = (float)(rand() % screenW); laserY = screenH + m; break;
        case 2: laserX = -m; laserY = (float)(rand() % screenH); break;
        default:laserX = screenW + m; laserY = (float)(rand() % screenH); break;
        }
        float dx = px - laserX, dy = py - laserY;
        float d  = std::sqrt(dx * dx + dy * dy) + 1e-3f;
        laserDirX = dx / d;
        laserDirY = dy / d;
    }

    static float segDist(float px, float py, float ax, float ay, float bx, float by) {
        float abx = bx - ax, aby = by - ay, l2 = abx * abx + aby * aby, t = 0.0f;
        if (l2 > 1e-6f) {
            t = ((px - ax) * abx + (py - ay) * aby) / l2;
            t = t < 0 ? 0 : (t > 1 ? 1 : t);
        }
        float cx = ax + abx * t, cy = ay + aby * t;
        float dx = px - cx, dy = py - cy;
        return std::sqrt(dx * dx + dy * dy);
    }

    void Update(float px, float py, float dt, float& playerHP,
                std::vector<Bullet>& /*bullets*/) {
        if (!alive) return;

        checkEnrage();

        wanderTimer += dt;
        if (wanderTimer >= 2.2f) {
            pickNewTarget();
            wanderTimer = 0.0f;
        }
        float drift = enraged ? 1.5f : 1.1f;
        worldX += (targetX - worldX) * drift * dt;
        worldY += (targetY - worldY) * drift * dt;

        formTimer += dt;
        if (formTimer >= formDuration && !triActive && !triWarn &&
            !laserActive && !laserWarn)
            pickForm(false);

        switch (form) {
        case PForm::TRIANGLE: {
            if (!triActive && !triWarn) {
                triCd -= dt;
                if (triCd <= 0.0f) startTriWarn();
            }
            if (triWarn) {
                triWarnTimer -= dt;
                if (triWarnTimer <= 0.0f) {
                    triWarn = false;
                    triActive = true;
                    triTimer = 0.0f;
                    triSpawnAccum = 0.0f;
                }
            }
            if (triActive) {
                triTimer += dt;
                triSpawnAccum += dt;
                float rowInterval = enraged ? 0.11f : 0.15f;
                if (triSpawnAccum >= rowInterval) {
                    triSpawnAccum = 0.0f;
                    spawnTriRow();
                }
                if (triTimer >= 2.8f) {
                    triActive = false;
                    triCd = enraged ? 1.2f : 2.0f;
                }
            }
            break;
        }
        case PForm::RHOMBUS: {
            if (!laserActive && !laserWarn) {
                laserCd -= dt;
                if (laserCd <= 0.0f) {
                    aimLaser(px, py);
                    laserWarn = true;
                    laserWarnTimer = enraged ? 0.35f : 0.45f;
                }
            }
            if (laserWarn) {
                laserWarnTimer -= dt;
                if (laserWarnTimer <= 0.0f) {
                    laserWarn = false;
                    laserActive = true;
                    laserTimer = 0.0f;
                }
            }
            if (laserActive) {
                laserTimer += dt;
                float ex = laserX + laserDirX * laserReach();
                float ey = laserY + laserDirY * laserReach();
                float hitR = enraged ? 16.0f : 14.0f;
                if (segDist(px, py, laserX, laserY, ex, ey) < hitR)
                    HurtPlayer(playerHP, (enraged ? 14.0f : LASER_DPS) * dt);
                if (laserTimer >= 0.55f) {
                    laserActive = false;
                    laserCd = enraged ? 0.45f : 0.65f;
                }
            }
            break;
        }
        case PForm::DIAMOND:
            break;
        }

        for (auto& s : swarm) {
            if (!s.alive) continue;
            s.x += s.vx * dt;
            s.y += s.vy * dt;
            s.life -= dt;
            float dx = px - s.x, dy = py - s.y;
            if (dx * dx + dy * dy < 14.0f * 14.0f) {
                HurtPlayer(playerHP, SWARM_DMG);
                s.alive = false;
            }
            if (s.life <= 0.0f ||
                s.x < -120.0f || s.x > screenW + 120.0f ||
                s.y < -120.0f || s.y > screenH + 120.0f)
                s.alive = false;
        }
        swarm.erase(std::remove_if(swarm.begin(), swarm.end(),
            [](const PSwarm& s) { return !s.alive; }), swarm.end());
    }
};
