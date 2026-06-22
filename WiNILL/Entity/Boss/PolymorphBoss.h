#pragma once
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#include "Bullet.h"
#include "PlayerStats.h"

// ── GLITCH.exe — environment boss ──
//   SINGULARITY : persistent black hole; triangles feed it → burst at max size
//   DISPLACE    : telegraphed player-window hijack (clear orange landing zone)
//   PHANTOM     : interference static + brief phase slips (mobs stay visible)

enum class PForm { SINGULARITY, DISPLACE, PHANTOM };

struct PSwarm {
    float x, y, vx, vy, life;
    bool  alive;
    bool  pulled;
};

struct PGlitchBar {
    float x, y, w, h, vx, vy, life;
    bool  alive;
};

struct PolyGlitchFX {
    float playerAlpha   = 1.0f;
    bool  snapWarn      = false;
    float snapWarnX = 0.0f, snapWarnY = 0.0f;
    float snapWarnW = 0.0f, snapWarnH = 0.0f;
    float snapFromX = 0.0f, snapFromY = 0.0f;
    bool  slipWarn      = false;
    float slipWarnT     = 0.0f;
    float slipDirX      = 0.0f, slipDirY = 0.0f;
    bool  shakePulse    = false;
    float staticBand    = 0.0f;
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

    std::vector<PGlitchBar> bars;
    float displaceCd = 0.0f;
    float snapTimer = 0.0f;
    float snapTargetX = 0.0f, snapTargetY = 0.0f;

    float phantomSlipCd = 0.0f;

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

    bool damageable() const { return vulnTimer > 0.0f; }
    bool reflecting() const { return false; }

    bool droneActive(int i) const {
        switch (form) {
        case PForm::SINGULARITY: return i < 3;
        case PForm::DISPLACE:    return i >= 2 && i <= 4;
        case PForm::PHANTOM:     return i >= 3;
        default: return false;
        }
    }

    static void clampWin(float& wx, float& wy, float ww, float wh, int sw, int sh) {
        const float bar = 90.0f;
        if (wx < 0.0f) wx = 0.0f;
        if (wy < 0.0f) wy = 0.0f;
        if (wx + ww > (float)sw) wx = (float)sw - ww;
        if (wy + wh > (float)sh - bar) wy = (float)sh - bar - wh;
    }

    void resetFx() {
        fx.playerAlpha = 1.0f;
        fx.shakePulse  = false;
        fx.staticBand  = 0.0f;
        fx.snapWarn    = false;
        fx.snapWarnX = fx.snapWarnY = fx.snapWarnW = fx.snapWarnH = 0.0f;
        fx.snapFromX = fx.snapFromY = 0.0f;
        fx.slipWarn  = false;
        fx.slipWarnT = 0.0f;
        fx.slipDirX = fx.slipDirY = 0.0f;
    }

    void tickFxForForm() {
        fx.playerAlpha = 1.0f;
        switch (form) {
        case PForm::PHANTOM:
            fx.staticBand = fmodf(glitchT * (enraged ? 0.55f : 0.42f), 1.0f);
            break;
        default:
            fx.staticBand = 0.0f;
            break;
        }
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
        displaceCd = enraged ? 1.0f : 1.4f;
        snapTimer = 0.0f;
        phantomSlipCd = enraged ? 2.4f : 3.2f;
        bars.clear();
        swarm.clear();
        resetFx();

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

    void spawnGlitchBar() {
        PGlitchBar b;
        b.alive = true;
        b.life = enraged ? 3.5f : 2.8f;
        b.w = enraged ? 200.0f : 170.0f;
        b.h = enraged ? 26.0f : 20.0f;
        if (rand() % 2 == 0) {
            b.x = (rand() % 2) ? -b.w - 20.0f : (float)screenW + 20.0f;
            b.y = 80.0f + (float)(rand() % std::max(1, screenH - 200));
            b.vx = (b.x < 0.0f ? 1.0f : -1.0f) * (enraged ? 280.0f : 220.0f);
            b.vy = 0.0f;
        } else {
            b.x = 40.0f + (float)(rand() % std::max(1, screenW - 80));
            b.y = (rand() % 2) ? -b.h - 20.0f : (float)screenH + 20.0f;
            b.vx = 0.0f;
            b.vy = (b.y < 0.0f ? 1.0f : -1.0f) * (enraged ? 250.0f : 190.0f);
        }
        bars.push_back(b);
    }

    void beginSnap(float winX, float winY, float winW, float winH) {
        float margin = 40.0f;
        int spanX = std::max(1, (int)((float)screenW - winW - margin * 2.0f));
        int spanY = std::max(1, (int)((float)screenH - 90.0f - winH - margin * 2.0f));
        snapTargetX = margin + (float)(rand() % spanX);
        snapTargetY = margin + (float)(rand() % spanY);
        fx.snapWarn = true;
        fx.snapWarnX = snapTargetX;
        fx.snapWarnY = snapTargetY;
        fx.snapWarnW = winW;
        fx.snapWarnH = winH;
        fx.snapFromX = winX + winW * 0.5f;
        fx.snapFromY = winY + winH * 0.5f;
        snapTimer = enraged ? 0.85f : 1.05f;
    }

    void applySnap(float& winX, float& winY, float winW, float winH, float& playerHP) {
        winX = snapTargetX;
        winY = snapTargetY;
        clampWin(winX, winY, winW, winH, screenW, screenH);
        fx.snapWarn = false;
        fx.shakePulse = true;
        for (auto& b : bars) {
            if (!b.alive) continue;
            if (winX + winW > b.x && winX < b.x + b.w &&
                winY + winH > b.y && winY < b.y + b.h) {
                HurtPlayer(playerHP, enraged ? 5.0f : 3.0f);
                break;
            }
        }
        vulnTimer = enraged ? 0.85f : 1.05f;
    }

    void queuePhaseSlip() {
        float ang = (float)(rand() % 628) * 0.01f;
        fx.slipDirX = cosf(ang);
        fx.slipDirY = sinf(ang);
        fx.slipWarn = true;
        fx.slipWarnT = enraged ? 0.70f : 0.90f;
    }

    void phaseSlip(float& winX, float& winY, float winW, float winH,
                   float dirX, float dirY) {
        float dist = enraged ? 34.0f : 24.0f;
        winX += dirX * dist;
        winY += dirY * dist;
        clampWin(winX, winY, winW, winH, screenW, screenH);
        fx.shakePulse = true;
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

    void updateBars(float px, float py, float pw, float ph, float dt, float& playerHP) {
        for (auto& b : bars) {
            if (!b.alive) continue;
            b.x += b.vx * dt;
            b.y += b.vy * dt;
            b.life -= dt;
            if (px + pw > b.x && px < b.x + b.w && py + ph > b.y && py < b.y + b.h)
                HurtPlayer(playerHP, 6.0f * dt);
            if (b.life <= 0.0f ||
                b.x < -400.0f || b.x > screenW + 400.0f ||
                b.y < -400.0f || b.y > screenH + 400.0f)
                b.alive = false;
        }
        bars.erase(std::remove_if(bars.begin(), bars.end(),
            [](const PGlitchBar& b) { return !b.alive; }), bars.end());
    }

    void Update(float px, float py, float dt, float& playerHP,
                std::vector<Bullet>& bullets,
                float& winX, float& winY, float winW, float winH) {
        if (!alive) return;

        checkEnrage();
        glitchT += dt;
        droneSpin += dt * (enraged ? 2.8f : 2.1f);
        for (int i = 0; i < DRONE_N; i++)
            droneAng[i] += dt * (1.4f + (float)i * 0.22f) * (droneActive(i) ? 1.35f : 0.45f);

        if (vulnTimer > 0.0f) vulnTimer -= dt;

        formTimer += dt;
        if (formTimer >= formDuration && !triWarn && !holeBursting && snapTimer <= 0.0f)
            pickForm(false);

        tickFxForForm();

        switch (form) {
        case PForm::SINGULARITY:
            fx.snapWarn = false;
            worldX += (holeX - worldX) * 2.5f * dt;
            worldY += (holeY - worldY - 100.0f) * 2.5f * dt;
            if (triWarn) {
                triWarnTimer -= dt;
                if (triWarnTimer <= 0.0f) {
                    triWarn = false;
                    blackHoleActive = true;
                    holeR = holeRMin;
                    singularitySpawn = 0.0f;
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
                        holeR = holeRMin;
                        vulnTimer = enraged ? 2.4f : 2.8f;
                    }
                } else {
                    singularitySpawn -= dt;
                    if (singularitySpawn <= 0.0f) {
                        singularitySpawn = enraged ? 0.14f : 0.18f;
                        spawnEdgeTriangle();
                    }
                    float pdx = holeX - px, pdy = holeY - py;
                    float pd = std::sqrt(pdx * pdx + pdy * pdy) + 1e-3f;
                    if (pd < 380.0f) {
                        float pull = (380.0f - pd) * (enraged ? 0.00055f : 0.0004f);
                        winX += (pdx / pd) * pull * dt * 60.0f;
                        winY += (pdy / pd) * pull * dt * 60.0f;
                        clampWin(winX, winY, winW, winH, screenW, screenH);
                    }
                }
            }
            updateSwarm(px, py, dt, playerHP, bullets);
            break;

        case PForm::DISPLACE:
            worldX += sinf(glitchT * 2.1f) * 14.0f * dt;
            worldY += cosf(glitchT * 1.7f) * 11.0f * dt;
            displaceCd -= dt;
            if (displaceCd <= 0.0f && snapTimer <= 0.0f) {
                beginSnap(winX, winY, winW, winH);
                displaceCd = enraged ? 2.4f : 3.2f;
            }
            if (snapTimer > 0.0f) {
                fx.snapFromX = winX + winW * 0.5f;
                fx.snapFromY = winY + winH * 0.5f;
                snapTimer -= dt;
                if (snapTimer <= 0.0f)
                    applySnap(winX, winY, winW, winH, playerHP);
            }
            if ((int)(glitchT * 1.8f) != (int)((glitchT - dt) * 1.8f))
                spawnGlitchBar();
            updateBars(winX, winY, winW, winH, dt, playerHP);
            if (formTimer > formDuration * 0.78f && vulnTimer <= 0.0f)
                vulnTimer = enraged ? 1.6f : 2.0f;
            break;

        case PForm::PHANTOM:
            fx.snapWarn = false;
            worldX = (float)screenW * 0.5f + sinf(glitchT * 1.0f) * 36.0f;
            worldY = (float)screenH * 0.22f + cosf(glitchT * 0.85f) * 20.0f;
            if (fx.slipWarn) {
                fx.slipWarnT -= dt;
                if (fx.slipWarnT <= 0.0f) {
                    fx.slipWarn = false;
                    phaseSlip(winX, winY, winW, winH, fx.slipDirX, fx.slipDirY);
                    phantomSlipCd = enraged ? 3.4f : 4.5f;
                }
            } else {
                phantomSlipCd -= dt;
                if (phantomSlipCd <= 0.0f)
                    queuePhaseSlip();
            }
            if (fmodf(glitchT, enraged ? 2.4f : 3.0f) < dt)
                vulnTimer = enraged ? 1.6f : 2.0f;
            break;
        }
    }
};
