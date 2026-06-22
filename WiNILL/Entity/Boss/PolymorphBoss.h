#pragma once
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <utility>
#include <glm/glm.hpp>
#include "Bullet.h"
#include "Monster.h"
#include "PlayerStats.h"

// ── GLITCH.exe — 블랙홀 중심 보스 ──
//   블랙홀은 항상 성장(억제 거의 불가). 패턴 전환 시 랜덤 위치 텔레포트.
//   SINGULARITY : 청록 — 몹 소환↑ + 삼각형
//   DISPLACE    : 주황 — 블랙홀 궤도 이동
//   PHANTOM     : 보라 — 블랙홀 투척

enum class PForm { SINGULARITY, DISPLACE, PHANTOM };

struct PFormColors {
    float accentR, accentG, accentB;
    float winNR, winNG, winNB;
    float glowR, glowG, glowB;
    float barR, barG, barB;
};

struct PSwarm {
    float x, y, vx, vy, life;
    bool  alive;
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
    float formDuration = 17.0f;
    float glitchT = 0.0f;

    PolyGlitchFX fx;

    static constexpr int DRONE_N = 6;
    float droneAng[DRONE_N] = {};
    float droneSpin = 0.0f;

    std::vector<PSwarm> swarm;

    bool  blackHoleActive = true;
    float holeX = 0.0f, holeY = 0.0f;
    float holeVx = 0.0f, holeVy = 0.0f;
    float holeAnchorX = 0.0f, holeAnchorY = 0.0f;
    float bossAnchorX = 0.0f, bossAnchorY = 0.0f;
    float holeR = 70.0f;
    float holeRMin = 55.0f;
    float holeRMax = 210.0f;
    bool  holeBursting = false;
    float holeBurstT = 0.0f;
    float holeBurstEmit = 0.0f;
    static constexpr float HOLE_BURST_DUR = 2.8f;

    float holeOrbitT = 0.0f;
    bool  holeThrowing = false;
    float throwCd = 1.5f;

    std::vector<std::pair<float, float>> mobSpawnQueue;
    float mobSpawnCd = 0.0f;
    float singularitySpawn = 0.0f;
    float directShotCd = 0.0f;
    bool  mobClearPending = false;

    static constexpr float BODY = 78.0f;
    static constexpr float SWARM_DMG = 6.0f;
    static constexpr float CORE_DPS  = 22.0f;

    PolymorphBoss(int sw, int sh, float hpInit) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.5f;
        worldY = sh * 0.28f;
        holeX = sw * 0.5f;
        holeY = (sh - 90.0f) * 0.5f;
        holeRMax = 210.0f;
        for (int i = 0; i < DRONE_N; i++)
            droneAng[i] = (float)i / (float)DRONE_N * 6.2831853f;
        pickForm(true);
    }

    PFormColors formColors() const {
        switch (form) {
        case PForm::DISPLACE:
            return { 1.0f, 0.42f, 0.10f,  1.0f, 0.38f, 0.12f,  1.0f, 0.35f, 0.08f,  1.0f, 0.55f, 0.15f };
        case PForm::PHANTOM:
            return { 0.78f, 0.48f, 1.0f,  0.82f, 0.55f, 1.0f,  0.65f, 0.40f, 0.95f,  0.72f, 0.45f, 1.0f };
        default:
            return { 0.12f, 0.98f, 0.82f,  0.85f, 0.25f, 1.0f,  0.10f, 0.92f, 0.75f,  0.10f, 0.98f, 0.80f };
        }
    }

    bool damageable() const { return true; }
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
    void resetFx() { fx.shakePulse = false; }

    float randRange(float lo, float hi) const {
        if (hi <= lo) return lo;
        return lo + (float)(rand() % 10001) * 0.0001f * (hi - lo);
    }

    void placeFormAnchors(bool snap) {
        const float pad = 150.0f;
        const float top = pad * 0.6f;
        const float bot = (float)screenH - pad - 90.0f;
        const float right = (float)screenW - pad;
        holeAnchorX = randRange(pad, right);
        holeAnchorY = randRange(pad + 40.0f, bot);
        bossAnchorX = randRange(pad, right);
        bossAnchorY = randRange(top, bot * 0.42f);
        holeOrbitT = randRange(0.0f, 6.2831853f);
        if (snap) {
            holeX = holeAnchorX;
            holeY = holeAnchorY;
            holeVx = holeVy = 0.0f;
            holeThrowing = false;
        }
        worldX = bossAnchorX;
        worldY = bossAnchorY;
    }

    void pickForm(bool first) {
        PForm prev = form;
        do { form = (PForm)(rand() % 3); } while (!first && form == prev);
        if (!first && prev == PForm::SINGULARITY)
            requestMobClear();

        formTimer = 0.0f;
        formDuration = enraged ? 14.0f : 18.0f;
        holeBursting = false;
        holeBurstT = 0.0f;
        holeBurstEmit = 0.0f;
        mobSpawnCd = 0.35f;
        singularitySpawn = 0.0f;
        directShotCd = 0.6f;
        throwCd = 1.2f;
        mobSpawnQueue.clear();
        swarm.clear();
        resetFx();
        blackHoleActive = true;
        placeFormAnchors(true);
    }

    void checkEnrage() {
        if (!enraged && hp <= maxHp * 0.40f) {
            enraged = true;
            formDuration = 14.0f;
        }
    }

    float holeWinSize() const {
        return std::max(260.0f, holeR * 3.0f + 90.0f);
    }

    float holeFillPct() const {
        if (holeRMax <= holeRMin + 0.001f) return 0.0f;
        float p = (holeR - holeRMin) / (holeRMax - holeRMin);
        if (p < 0.0f) p = 0.0f;
        if (p > 1.0f) p = 1.0f;
        return p;
    }

    float pullRadius() const { return holeR * 2.7f + 95.0f; }
    float coreRadius() const { return holeR * 0.38f + 8.0f; }

    void spawnEdgeTriangle() {
        PSwarm s;
        s.alive = true;
        s.life = enraged ? 5.5f : 4.5f;
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
        float sp = enraged ? 120.0f : 100.0f;
        s.vx = dx / d * sp;
        s.vy = dy / d * sp;
        swarm.push_back(s);
    }

    void queueMobNearHole() {
        float ang = (float)(rand() % 628) * 0.01f;
        float dist = enraged ? 260.0f : 220.0f;
        mobSpawnQueue.push_back({ holeX + cosf(ang) * dist,
                                  holeY + sinf(ang) * dist });
    }

    void feedHole(float amount) {
        holeR += amount;
        if (holeR > holeRMax) holeR = holeRMax;
    }

    void onTriangleShot() { /* 억제 거의 불가 — 무시 */ }

    void passiveHoleGrowth(float dt) {
        float rate = enraged ? 9.5f : 7.0f;
        if (form == PForm::SINGULARITY) rate *= 1.4f;
        holeR += rate * dt;
        if (holeR > holeRMax) holeR = holeRMax;
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
        auto c = formColors();
        int n = enraged ? 20 : 16;
        for (int i = 0; i < n; i++) {
            float ang = angOff + (float)i / (float)n * 6.2831853f;
            float tx = holeX + cosf(ang) * 280.0f;
            float ty = holeY + sinf(ang) * 280.0f;
            Bullet b(holeX, holeY, tx, ty);
            b.isEnemy  = true;
            b.enemyDmg = enraged ? 7.0f : 5.5f;
            b.speed    = enraged ? 500.0f : 420.0f;
            b.color    = glm::vec3(c.accentR, c.accentG, c.accentB);
            bullets.push_back(b);
        }
    }

    void triggerHoleBurst(std::vector<Bullet>& bullets) {
        if (holeBursting) return;
        holeBursting = true;
        holeBurstT = HOLE_BURST_DUR;
        holeBurstEmit = 0.0f;
        fireHoleBurstWave(bullets, 0.0f);
        fx.shakePulse = true;
        holeRMax += enraged ? 28.0f : 20.0f;
    }

    void pullPoint(float& x, float& y, float pullMul, float dt) const {
        float pullR = pullRadius();
        float dx = holeX - x, dy = holeY - y;
        float d2 = dx * dx + dy * dy;
        if (d2 < 1.0f || d2 > pullR * pullR) return;
        float d = std::sqrt(d2);
        float t = 1.0f - d / pullR;
        float str = (enraged ? 480.0f : 380.0f) * t * t * pullMul;
        x += (dx / d) * str * dt;
        y += (dy / d) * str * dt;
    }

    void applyBlackHoleField(std::vector<Bullet>& bullets,
                             std::vector<Monster*>& monsters,
                             float px, float py, float& playerHP, float dt,
                             float& outPullX, float& outPullY) {
        if (!blackHoleActive) return;

        float pullR = pullRadius();
        float pullR2 = pullR * pullR;
        float coreR = coreRadius();
        float coreR2 = coreR * coreR;

        for (auto& b : bullets) {
            if (!b.active) continue;
            pullPoint(b.x, b.y, 1.15f, dt);
            float dx = holeX - b.x, dy = holeY - b.y;
            float d = std::sqrt(dx * dx + dy * dy) + 1e-3f;
            if (d < pullR) {
                float bend = 5.5f * dt * (1.0f - d / pullR);
                b.dirX += (dx / d) * bend;
                b.dirY += (dy / d) * bend;
                float len = std::sqrt(b.dirX * b.dirX + b.dirY * b.dirY) + 1e-3f;
                b.dirX /= len;
                b.dirY /= len;
            }
            float bdx = b.x - holeX, bdy = b.y - holeY;
            if (bdx * bdx + bdy * bdy < coreR2) {
                b.active = false;
                if (b.isEnemy) feedHole(enraged ? 3.2f : 2.6f);
            }
        }

        for (auto* m : monsters) {
            if (!m->alive) continue;
            pullPoint(m->worldX, m->worldY, m->summoned ? 1.25f : 1.0f, dt);
            float mdx = m->worldX - holeX, mdy = m->worldY - holeY;
            if (mdx * mdx + mdy * mdy < coreR2) {
                m->alive = false;
                m->hp = 0.0f;
                feedHole(enraged ? 6.0f : 5.0f);
            }
        }

        pullPoint(worldX, worldY, 0.28f, dt);

        float pdx = px - holeX, pdy = py - holeY;
        float pd2 = pdx * pdx + pdy * pdy;
        if (pd2 > 1.0f && pd2 < pullR2) {
            float pd = std::sqrt(pd2);
            float t = 1.0f - pd / pullR;
            float str = (enraged ? 300.0f : 240.0f) * t * t;
            outPullX += (holeX - px) / pd * str * dt;
            outPullY += (holeY - py) / pd * str * dt;
        }
        if (pd2 < coreR2)
            HurtPlayer(playerHP, CORE_DPS * dt);

        if (holeR >= holeRMax - 0.5f && !holeBursting)
            triggerHoleBurst(bullets);
    }

    void updateSwarm(float px, float py, float dt, float& playerHP) {
        for (auto& s : swarm) {
            if (!s.alive) continue;
            pullPoint(s.x, s.y, 1.1f, dt);
            float dx = holeX - s.x, dy = holeY - s.y;
            float d = std::sqrt(dx * dx + dy * dy) + 1e-3f;
            s.vx += (dx / d) * (enraged ? 220.0f : 180.0f) * dt;
            s.vy += (dy / d) * (enraged ? 220.0f : 180.0f) * dt;
            float sp2 = s.vx * s.vx + s.vy * s.vy;
            float maxSp = enraged ? 820.0f : 680.0f;
            if (sp2 > maxSp * maxSp) {
                float sc = maxSp / std::sqrt(sp2);
                s.vx *= sc; s.vy *= sc;
            }
            s.x += s.vx * dt;
            s.y += s.vy * dt;
            s.life -= dt;

            float pdx = px - s.x, pdy = py - s.y;
            if (pdx * pdx + pdy * pdy < 12.0f * 12.0f) {
                HurtPlayer(playerHP, SWARM_DMG);
                s.alive = false;
                continue;
            }
            float hdx = s.x - holeX, hdy = s.y - holeY;
            if (hdx * hdx + hdy * hdy < coreRadius() * coreRadius()) {
                s.alive = false;
                feedHole(enraged ? 2.8f : 2.2f);
            }
            if (s.life <= 0.0f ||
                s.x < -160.0f || s.x > screenW + 160.0f ||
                s.y < -160.0f || s.y > screenH + 160.0f)
                s.alive = false;
        }
        swarm.erase(std::remove_if(swarm.begin(), swarm.end(),
            [](const PSwarm& s) { return !s.alive; }), swarm.end());
    }

    void updateHoleMotion(float px, float py, float dt) {
        switch (form) {
        case PForm::SINGULARITY: {
            float tx = holeAnchorX + sinf(glitchT * 0.55f) * 58.0f;
            float ty = holeAnchorY + cosf(glitchT * 0.42f) * 44.0f;
            holeX += (tx - holeX) * 1.4f * dt;
            holeY += (ty - holeY) * 1.4f * dt;
            worldX += (holeX - worldX) * 2.0f * dt;
            worldY += (holeY - worldY - 105.0f) * 2.0f * dt;
            break;
        }
        case PForm::DISPLACE: {
            holeOrbitT += dt;
            float tx = holeAnchorX + sinf(holeOrbitT * 1.05f) * 185.0f;
            float ty = holeAnchorY + cosf(holeOrbitT * 0.88f) * 135.0f;
            float pad = 100.0f;
            if (tx < pad) tx = pad;
            if (tx > screenW - pad) tx = screenW - pad;
            if (ty < pad) ty = pad;
            if (ty > screenH - pad - 90.0f) ty = screenH - pad - 90.0f;
            holeX += (tx - holeX) * (enraged ? 2.4f : 1.9f) * dt;
            holeY += (ty - holeY) * (enraged ? 2.4f : 1.9f) * dt;
            worldX = bossAnchorX + sinf(glitchT * 1.2f) * 72.0f;
            worldY = bossAnchorY + cosf(glitchT * 0.95f) * 26.0f;
            break;
        }
        case PForm::PHANTOM: {
            worldX = bossAnchorX + sinf(glitchT * 0.85f) * 58.0f;
            worldY = bossAnchorY + cosf(glitchT * 0.7f) * 20.0f;

            throwCd -= dt;
            if (!holeThrowing && throwCd <= 0.0f) {
                throwCd = enraged ? 2.8f : 3.6f;
                float dx = px - holeX, dy = py - holeY;
                float d = std::sqrt(dx * dx + dy * dy) + 1e-3f;
                float spd = enraged ? 460.0f : 380.0f;
                holeVx = dx / d * spd;
                holeVy = dy / d * spd;
                holeThrowing = true;
                fx.shakePulse = true;
            }

            if (holeThrowing) {
                holeX += holeVx * dt;
                holeY += holeVy * dt;
                float drag = 1.0f - (enraged ? 1.35f : 1.1f) * dt;
                if (drag < 0.0f) drag = 0.0f;
                holeVx *= drag;
                holeVy *= drag;
                float sp2 = holeVx * holeVx + holeVy * holeVy;
                float pdx = px - holeX, pdy = py - holeY;
                if (sp2 < 55.0f * 55.0f || pdx * pdx + pdy * pdy < 90.0f * 90.0f)
                    holeThrowing = false;
            }
            if (!holeThrowing) {
                float hx = holeAnchorX + sinf(glitchT * 1.1f) * 48.0f;
                float hy = holeAnchorY + cosf(glitchT * 0.9f) * 38.0f;
                holeX += (hx - holeX) * 1.8f * dt;
                holeY += (hy - holeY) * 1.8f * dt;
                holeVx = holeVy = 0.0f;
            }
            clampHole();
            break;
        }
        }
        if (form != PForm::PHANTOM) clampHole();
    }

    void clampHole() {
        if (holeX < 80.0f) holeX = 80.0f;
        if (holeX > screenW - 80.0f) holeX = screenW - 80.0f;
        if (holeY < 80.0f) holeY = 80.0f;
        if (holeY > screenH - 120.0f) holeY = screenH - 120.0f;
    }

    void Update(float px, float py, float dt, float& playerHP,
                std::vector<Bullet>& bullets,
                std::vector<Monster*>& monsters,
                float& playerPullX, float& playerPullY) {
        if (!alive) return;

        checkEnrage();
        glitchT += dt;
        droneSpin += dt * (enraged ? 2.8f : 2.1f);
        for (int i = 0; i < DRONE_N; i++)
            droneAng[i] += dt * (1.4f + (float)i * 0.22f) * (droneActive(i) ? 1.35f : 0.45f);

        formTimer += dt;
        if (formTimer >= formDuration && !holeBursting)
            pickForm(false);

        passiveHoleGrowth(dt);

        if (holeBursting) {
            holeBurstT -= dt;
            holeBurstEmit -= dt;
            if (holeBurstEmit <= 0.0f) {
                holeBurstEmit = enraged ? 0.06f : 0.075f;
                float spin = (HOLE_BURST_DUR - holeBurstT) * (enraged ? 4.0f : 3.2f);
                fireHoleBurstWave(bullets, spin);
                if ((int)(holeBurstT * 12.0f) % 2 == 0)
                    fx.shakePulse = true;
            }
            if (holeBurstT <= 0.0f) {
                holeBursting = false;
                holeR = holeRMax * 0.78f;
            }
        }

        updateHoleMotion(px, py, dt);
        applyBlackHoleField(bullets, monsters, px, py, playerHP, dt,
                            playerPullX, playerPullY);

        auto c = formColors();
        switch (form) {
        case PForm::SINGULARITY:
            singularitySpawn -= dt;
            if (singularitySpawn <= 0.0f) {
                singularitySpawn = enraged ? 0.16f : 0.20f;
                spawnEdgeTriangle();
            }
            mobSpawnCd -= dt;
            if (mobSpawnCd <= 0.0f) {
                mobSpawnCd = enraged ? 1.2f : 1.7f;
                if ((int)mobSpawnQueue.size() < 4)
                    queueMobNearHole();
            }
            updateSwarm(px, py, dt, playerHP);
            break;

        case PForm::DISPLACE:
            directShotCd -= dt;
            if (directShotCd <= 0.0f) {
                directShotCd = enraged ? 0.85f : 1.1f;
                fireAtPlayer(bullets, px, py, enraged ? 6 : 4, 0.50f,
                             enraged ? 480.0f : 400.0f, enraged ? 6.0f : 4.8f,
                             c.accentR, c.accentG, c.accentB);
            }
            break;

        case PForm::PHANTOM:
            directShotCd -= dt;
            if (directShotCd <= 0.0f) {
                directShotCd = enraged ? 1.0f : 1.35f;
                fireAtPlayer(bullets, px, py, enraged ? 8 : 6, 0.70f,
                             enraged ? 520.0f : 440.0f, enraged ? 5.8f : 4.6f,
                             c.accentR, c.accentG, c.accentB);
            }
            break;
        }
    }
};
