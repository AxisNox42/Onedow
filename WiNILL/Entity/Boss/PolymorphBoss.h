#pragma once
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <utility>
#include <glm/glm.hpp>
#include "Bullet.h"
#include "PlayerStats.h"

// ── GLITCH.exe — 3패턴 직접 대결 보스 ──
//   SINGULARITY : 블랙홀 + 삼각형 탄막 (이 구간만 mob.exe 소환)
//   DISPLACE    : 보스 이동 + 조준 탄막
//   PHANTOM     : 보스 돌진 + 부채꼴 탄막

enum class PForm { SINGULARITY, DISPLACE, PHANTOM };

struct PSwarm {
    float x, y, vx, vy, life;
    bool  alive;
    bool  pulled;
};

struct PolyGlitchFX {
    bool  shakePulse = false;
};

class PolymorphBoss {
public:
    float worldX, worldY;
    float hp, maxHp;
    bool  alive = true, exploded = false;
    bool  enraged = false;
    int   screenW, screenH;

    PForm form = PForm::SINGULARITY;
    float formTimer = 0.0f;
    float formDuration = 9.0f;
    float vulnTimer = 0.0f;
    float glitchT = 0.0f;

    PolyGlitchFX fx;

    static constexpr int DRONE_N = 6;
    float droneAng[DRONE_N] = {};
    float droneSpin = 0.0f;

    std::vector<PSwarm> swarm;
    bool  triWarn = false;
    bool  blackHoleActive = false;
    float triWarnTimer = 0.0f;
    float singularitySpawn = 0.0f;
    float holeX = 0.0f, holeY = 0.0f;
    float holeR = 70.0f;
    float holeRMin = 70.0f;
    float holeRMax = 230.0f;
    bool  holeBursting = false;
    float holeBurstT = 0.0f;
    float holeBurstEmit = 0.0f;
    static constexpr float HOLE_BURST_DUR = 4.0f;

    std::vector<std::pair<float, float>> mobSpawnQueue;
    float mobSpawnCd = 0.0f;
    float directShotCd = 0.0f;
    float dashCd = 0.0f;
    bool  mobClearPending = false;

    static constexpr float BODY = 78.0f;
    static constexpr float SWARM_DMG = 7.0f;
    static constexpr float CORE_DMG  = 8.0f;

    PolymorphBoss(int sw, int sh, float hpInit) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.5f;
        worldY = sh * 0.28f;
        holeX = sw * 0.5f;
        holeY = (sh - 90.0f) * 0.5f;
        holeRMax = 230.0f;
        for (int i = 0; i < DRONE_N; i++)
            droneAng[i] = (float)i / (float)DRONE_N * 6.2831853f;
        pickForm(true);
    }

    bool damageable() const {
        if (form == PForm::SINGULARITY && blackHoleActive)
            return false;
        if (vulnTimer > 0.0f)
            return true;
        return form != PForm::SINGULARITY;
    }

    bool reflecting() const { return false; }

    bool consumeMobClear() {
        if (!mobClearPending) return false;
        mobClearPending = false;
        return true;
    }

    bool droneActive(int i) const {
        switch (form) {
        case PForm::SINGULARITY: return i < 3;
        case PForm::DISPLACE:    return i >= 2 && i <= 4;
        case PForm::PHANTOM:     return i >= 3;
        default: return false;
        }
    }

    void requestMobClear() { mobClearPending = true; }

    void resetFx() {
        fx.shakePulse = false;
    }

    void pickForm(bool first) {
        PForm prev = form;
        do { form = (PForm)(rand() % 3); } while (!first && form == prev);
        formTimer = 0.0f;
        formDuration = enraged ? 7.0f : 10.0f;
        triWarn = false;
        blackHoleActive = false;
        singularitySpawn = 0.0f;
        holeR = holeRMin;
        holeBursting = false;
        holeBurstT = 0.0f;
        holeBurstEmit = 0.0f;
        holeRMax = enraged ? 270.0f : 230.0f;
        mobSpawnCd = 0.5f;
        directShotCd = 0.8f;
        dashCd = 1.2f;
        mobSpawnQueue.clear();
        swarm.clear();
        resetFx();
        requestMobClear();

        if (form == PForm::SINGULARITY) {
            triWarn = true;
            triWarnTimer = enraged ? 0.85f : 1.05f;
            holeX = (float)screenW * 0.5f;
            holeY = ((float)screenH - 90.0f) * 0.5f;
        }
    }

    void checkEnrage() {
        if (!enraged && hp <= maxHp * 0.40f) {
            enraged = true;
            formDuration = 7.0f;
            holeRMax = 270.0f;
        }
    }

    float holeWinSize() const {
        return std::max(280.0f, holeR * 3.1f + 80.0f);
    }

    float holeFillPct() const {
        if (holeRMax <= holeRMin + 0.001f) return 0.0f;
        float p = (holeR - holeRMin) / (holeRMax - holeRMin);
        if (p < 0.0f) p = 0.0f;
        if (p > 1.0f) p = 1.0f;
        return p;
    }

    void spawnEdgeTriangle() {
        PSwarm s;
        s.alive = true;
        s.pulled = true;
        s.life = enraged ? 6.0f : 5.0f;
        float m = 36.0f;
        int edge = rand() % 4;
        switch (edge) {
        case 0: s.x = (float)(rand() % std::max(1, screenW)); s.y = -m; break;
        case 1: s.x = (float)(rand() % std::max(1, screenW)); s.y = screenH + m; break;
        case 2: s.x = -m; s.y = (float)(rand() % std::max(1, screenH)); break;
        default:s.x = screenW + m; s.y = (float)(rand() % std::max(1, screenH)); break;
        }
        float dx = holeX - s.x, dy = holeY - s.y;
        float d = std::sqrt(dx * dx + dy * dy) + 1e-3f;
        float sp = enraged ? 130.0f : 105.0f;
        s.vx = dx / d * sp;
        s.vy = dy / d * sp;
        swarm.push_back(s);
    }

    void queueMobNearHole() {
        float ang = (float)(rand() % 628) * 0.01f;
        float dist = enraged ? 280.0f : 240.0f;
        mobSpawnQueue.push_back({ holeX + cosf(ang) * dist,
                                  holeY + sinf(ang) * dist });
    }

    void fireAtPlayer(std::vector<Bullet>& bullets, float px, float py,
                      int count, float spread, float speed, float dmg,
                      float cr, float cg, float cb) {
        float dx = px - worldX, dy = py - worldY;
        float base = std::atan2(dy, dx);
        for (int i = 0; i < count; i++) {
            float t = (count <= 1) ? 0.0f
                : ((float)i / (float)(count - 1) - 0.5f) * spread;
            float ang = base + t;
            float tx = worldX + cosf(ang) * 520.0f;
            float ty = worldY + sinf(ang) * 520.0f;
            Bullet b(worldX, worldY, tx, ty);
            b.isEnemy  = true;
            b.enemyDmg = dmg;
            b.speed    = speed;
            b.color    = glm::vec3(cr, cg, cb);
            bullets.push_back(b);
        }
    }

    void fireHoleBurstWave(std::vector<Bullet>& bullets, float angOff = 0.0f) {
        int n = enraged ? 22 : 18;
        for (int i = 0; i < n; i++) {
            float ang = angOff + (float)i / (float)n * 6.2831853f;
            float tx = holeX + cosf(ang) * 260.0f;
            float ty = holeY + sinf(ang) * 260.0f;
            Bullet b(holeX, holeY, tx, ty);
            b.isEnemy  = true;
            b.enemyDmg = enraged ? 7.5f : 5.8f;
            b.speed    = enraged ? 520.0f : 440.0f;
            b.color    = glm::vec3(0.85f, 0.22f, 1.0f);
            bullets.push_back(b);
        }
    }

    void triggerHoleBurst(std::vector<Bullet>& bullets) {
        if (holeBursting) return;
        holeBursting = true;
        holeBurstT = HOLE_BURST_DUR;
        holeBurstEmit = 0.0f;
        requestMobClear();
        fireHoleBurstWave(bullets, 0.0f);
        fx.shakePulse = true;
    }

    void absorbTriangle() {
        holeR += enraged ? 3.6f : 3.0f;
        if (holeR >= holeRMax)
            holeR = holeRMax;
    }

    void onTriangleShot() {
        holeR -= enraged ? 8.0f : 6.0f;
        if (holeR < holeRMin) holeR = holeRMin;
    }

    void updateSwarm(float px, float py, float dt, float& playerHP,
                     std::vector<Bullet>& bullets) {
        for (auto& s : swarm) {
            if (!s.alive) continue;
            if (s.pulled && blackHoleActive && !holeBursting) {
                float dx = holeX - s.x, dy = holeY - s.y;
                float d = std::sqrt(dx * dx + dy * dy) + 1e-3f;
                float pull = enraged ? 500.0f : 400.0f;
                s.vx += (dx / d) * pull * dt;
                s.vy += (dy / d) * pull * dt;
                float maxSp = enraged ? 920.0f : 780.0f;
                float sp2 = s.vx * s.vx + s.vy * s.vy;
                if (sp2 > maxSp * maxSp) {
                    float sc = maxSp / std::sqrt(sp2);
                    s.vx *= sc; s.vy *= sc;
                }
            }
            s.x += s.vx * dt;
            s.y += s.vy * dt;
            s.life -= dt;

            float pdx = px - s.x, pdy = py - s.y;
            if (pdx * pdx + pdy * pdy < 13.0f * 13.0f) {
                HurtPlayer(playerHP, SWARM_DMG);
                s.alive = false;
                continue;
            }
            if (blackHoleActive && !holeBursting) {
                float hdx = s.x - holeX, hdy = s.y - holeY;
                if (hdx * hdx + hdy * hdy < (holeR * 0.35f + 10.0f) * (holeR * 0.35f + 10.0f)) {
                    s.alive = false;
                    absorbTriangle();
                    if (holeR >= holeRMax - 0.5f)
                        triggerHoleBurst(bullets);
                    float adx = px - holeX, ady = py - holeY;
                    if (adx * adx + ady * ady < (holeR + 50.0f) * (holeR + 50.0f))
                        HurtPlayer(playerHP, CORE_DMG);
                }
            }
            if (s.life <= 0.0f ||
                s.x < -160.0f || s.x > screenW + 160.0f ||
                s.y < -160.0f || s.y > screenH + 160.0f)
                s.alive = false;
        }
        swarm.erase(std::remove_if(swarm.begin(), swarm.end(),
            [](const PSwarm& s) { return !s.alive; }), swarm.end());
    }

    void Update(float px, float py, float dt, float& playerHP,
                std::vector<Bullet>& bullets) {
        if (!alive) return;

        checkEnrage();
        glitchT += dt;
        droneSpin += dt * (enraged ? 2.8f : 2.1f);
        for (int i = 0; i < DRONE_N; i++)
            droneAng[i] += dt * (1.4f + (float)i * 0.22f) * (droneActive(i) ? 1.35f : 0.45f);

        if (vulnTimer > 0.0f) vulnTimer -= dt;

        formTimer += dt;
        if (formTimer >= formDuration && !triWarn && !holeBursting)
            pickForm(false);

        switch (form) {
        case PForm::SINGULARITY:
            worldX += (holeX - worldX) * 2.5f * dt;
            worldY += (holeY - worldY - 100.0f) * 2.5f * dt;
            if (triWarn) {
                triWarnTimer -= dt;
                if (triWarnTimer <= 0.0f) {
                    triWarn = false;
                    blackHoleActive = true;
                    holeR = holeRMin;
                    singularitySpawn = 0.0f;
                    mobSpawnCd = 0.4f;
                }
            }
            if (blackHoleActive) {
                if (holeBursting) {
                    holeBurstT -= dt;
                    float burstProg = holeBurstT / HOLE_BURST_DUR;
                    if (burstProg < 0.0f) burstProg = 0.0f;
                    holeR = holeRMin + (holeRMax - holeRMin) * burstProg;
                    holeBurstEmit -= dt;
                    if (holeBurstEmit <= 0.0f) {
                        holeBurstEmit = enraged ? 0.055f : 0.07f;
                        float spin = (HOLE_BURST_DUR - holeBurstT) * (enraged ? 4.2f : 3.4f);
                        fireHoleBurstWave(bullets, spin);
                        if ((int)(holeBurstT * 14.0f) % 2 == 0)
                            fx.shakePulse = true;
                    }
                    if (holeBurstT <= 0.0f) {
                        holeBursting = false;
                        blackHoleActive = false;
                        holeR = holeRMin;
                        vulnTimer = enraged ? 2.0f : 2.4f;
                        requestMobClear();
                    }
                } else {
                    singularitySpawn -= dt;
                    if (singularitySpawn <= 0.0f) {
                        singularitySpawn = enraged ? 0.14f : 0.18f;
                        spawnEdgeTriangle();
                    }
                    mobSpawnCd -= dt;
                    if (mobSpawnCd <= 0.0f) {
                        mobSpawnCd = enraged ? 2.0f : 2.6f;
                        if ((int)mobSpawnQueue.size() < 3)
                            queueMobNearHole();
                    }
                }
            }
            updateSwarm(px, py, dt, playerHP, bullets);
            break;

        case PForm::DISPLACE:
            worldX = (float)screenW * 0.5f + sinf(glitchT * 1.3f) * 110.0f;
            worldY = (float)screenH * 0.24f + cosf(glitchT * 1.0f) * 36.0f;
            directShotCd -= dt;
            if (directShotCd <= 0.0f) {
                directShotCd = enraged ? 0.75f : 1.0f;
                fireAtPlayer(bullets, px, py, enraged ? 7 : 5, 0.55f,
                             enraged ? 500.0f : 420.0f, enraged ? 6.5f : 5.0f,
                             1.0f, 0.45f, 0.15f);
            }
            break;

        case PForm::PHANTOM:
            worldX = (float)screenW * 0.5f + sinf(glitchT * 0.9f) * 64.0f;
            worldY = (float)screenH * 0.22f + cosf(glitchT * 0.75f) * 24.0f;
            dashCd -= dt;
            if (dashCd <= 0.0f) {
                dashCd = enraged ? 2.4f : 3.2f;
                float dx = px - worldX, dy = py - worldY;
                float d = std::sqrt(dx * dx + dy * dy) + 1e-3f;
                float leap = enraged ? 160.0f : 120.0f;
                worldX += (dx / d) * leap;
                worldY += (dy / d) * leap;
                if (worldX < 80.0f) worldX = 80.0f;
                if (worldX > screenW - 80.0f) worldX = screenW - 80.0f;
                if (worldY < 80.0f) worldY = 80.0f;
                if (worldY > screenH - 120.0f) worldY = screenH - 120.0f;
                fireAtPlayer(bullets, px, py, enraged ? 9 : 7, 0.75f,
                             enraged ? 540.0f : 460.0f, enraged ? 6.0f : 4.8f,
                             0.75f, 0.55f, 1.0f);
                fx.shakePulse = true;
            }
            break;
        }
    }
};
