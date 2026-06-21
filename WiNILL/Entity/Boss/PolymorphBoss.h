#pragma once
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#include "Bullet.h"
#include "PlayerStats.h"

// ── GLITCH.exe — map/environment boss (not a direct DPS check) ──
//   SINGULARITY : edge warn → central black hole, edge triangles get sucked in
//   DISPLACE    : fake window snap + sliding corrupt panels (layout hazard)
//   PHANTOM     : cursor offset + player window blink / afterimage
//   Boss only damageable during brief sync windows after each attack cycle.

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
    float cursorOffX  = 0.0f, cursorOffY  = 0.0f;
    bool  cursorGlitch  = false;
    float playerAlpha   = 1.0f;
    bool  showGhostWin  = false;
    float ghostX = 0.0f, ghostY = 0.0f, ghostW = 0.0f, ghostH = 0.0f;
    bool  snapWarn      = false;
    float snapWarnX = 0.0f, snapWarnY = 0.0f;
    float snapWarnW = 0.0f, snapWarnH = 0.0f;
    bool  shakePulse    = false;
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

    // SINGULARITY
    std::vector<PSwarm> swarm;
    bool  triWarn = false;
    bool  blackHoleActive = false;
    float triWarnTimer = 0.0f;
    float singularityTimer = 0.0f;
    float singularityCd = 1.8f;
    float singularitySpawn = 0.0f;
    float holeX = 0.0f, holeY = 0.0f;
    float holeRadius = 0.0f;
    float holeGrow = 0.0f;

    // DISPLACE
    std::vector<PGlitchBar> bars;
    float displaceCd = 0.0f;
    float snapTimer = 0.0f;
    float snapTargetX = 0.0f, snapTargetY = 0.0f;
    float driftX = 0.0f, driftY = 0.0f;

    // PHANTOM
    float phantomPhase = 0.0f;

    static constexpr float BODY = 72.0f;
    static constexpr float SWARM_DMG = 7.0f;
    static constexpr float CORE_DMG  = 9.0f;

    PolymorphBoss(int sw, int sh, float hpInit) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.5f;
        worldY = sh * 0.28f;
        holeX = sw * 0.5f;
        holeY = (sh - 90.0f) * 0.5f;
        pickForm(true);
    }

    bool damageable() const { return vulnTimer > 0.0f; }
    bool reflecting() const { return false; }

    static void clampWin(float& wx, float& wy, float ww, float wh, int sw, int sh) {
        const float bar = 90.0f;
        if (wx < 0.0f) wx = 0.0f;
        if (wy < 0.0f) wy = 0.0f;
        if (wx + ww > (float)sw) wx = (float)sw - ww;
        if (wy + wh > (float)sh - bar) wy = (float)sh - bar - wh;
    }

    void resetFx() {
        fx = PolyGlitchFX{};
    }

    void pickForm(bool first) {
        PForm prev = form;
        do { form = (PForm)(rand() % 3); } while (!first && form == prev);
        formTimer = 0.0f;
        formDuration = enraged ? 6.5f : 9.0f;
        triWarn = blackHoleActive = false;
        singularityCd = enraged ? 1.2f : 1.8f;
        singularityTimer = 0.0f;
        singularitySpawn = 0.0f;
        holeRadius = 0.0f;
        displaceCd = enraged ? 0.8f : 1.2f;
        snapTimer = 0.0f;
        fx.snapWarn = false;
        bars.clear();
        resetFx();
        switch (form) {
        case PForm::DISPLACE:
            driftX = ((rand() % 2) ? 1.0f : -1.0f) * (enraged ? 55.0f : 38.0f);
            driftY = ((rand() % 2) ? 1.0f : -1.0f) * (enraged ? 42.0f : 28.0f);
            break;
        default: break;
        }
    }

    void checkEnrage() {
        if (!enraged && hp <= maxHp * 0.40f) {
            enraged = true;
            formDuration = 6.5f;
        }
    }

    void startSingularityWarn() {
        triWarn = true;
        triWarnTimer = enraged ? 0.75f : 0.95f;
        holeX = (float)screenW * 0.5f;
        holeY = ((float)screenH - 90.0f) * 0.5f;
        holeRadius = 0.0f;
    }

    void spawnEdgeTriangle() {
        PSwarm s;
        s.alive = true;
        s.pulled = true;
        s.life = enraged ? 5.5f : 4.5f;
        float m = 30.0f;
        switch (rand() % 4) {
        case 0: s.x = (float)(rand() % screenW); s.y = -m; break;
        case 1: s.x = (float)(rand() % screenW); s.y = screenH + m; break;
        case 2: s.x = -m; s.y = (float)(rand() % screenH); break;
        default:s.x = screenW + m; s.y = (float)(rand() % screenH); break;
        }
        float dx = holeX - s.x, dy = holeY - s.y;
        float d = std::sqrt(dx * dx + dy * dy) + 1e-3f;
        float sp = enraged ? 140.0f : 110.0f;
        s.vx = dx / d * sp;
        s.vy = dy / d * sp;
        swarm.push_back(s);
    }

    void spawnGlitchBar() {
        PGlitchBar b;
        b.alive = true;
        b.life = enraged ? 3.8f : 3.0f;
        b.w = enraged ? 220.0f : 180.0f;
        b.h = enraged ? 28.0f : 22.0f;
        if (rand() % 2 == 0) {
            b.x = (rand() % 2) ? -b.w - 20.0f : (float)screenW + 20.0f;
            b.y = 80.0f + (float)(rand() % std::max(1, screenH - 200));
            b.vx = (b.x < 0.0f ? 1.0f : -1.0f) * (enraged ? 320.0f : 240.0f);
            b.vy = 0.0f;
        } else {
            b.x = 40.0f + (float)(rand() % std::max(1, screenW - 80));
            b.y = (rand() % 2) ? -b.h - 20.0f : (float)screenH + 20.0f;
            b.vx = 0.0f;
            b.vy = (b.y < 0.0f ? 1.0f : -1.0f) * (enraged ? 280.0f : 210.0f);
        }
        bars.push_back(b);
    }

    void beginSnap(float winW, float winH) {
        float margin = 36.0f;
        int spanX = std::max(1, (int)((float)screenW - winW - margin * 2.0f));
        int spanY = std::max(1, (int)((float)screenH - 90.0f - winH - margin * 2.0f));
        snapTargetX = margin + (float)(rand() % spanX);
        snapTargetY = margin + (float)(rand() % spanY);
        fx.snapWarn = true;
        fx.snapWarnX = snapTargetX;
        fx.snapWarnY = snapTargetY;
        fx.snapWarnW = winW;
        fx.snapWarnH = winH;
        snapTimer = enraged ? 0.55f : 0.72f;
    }

    void applySnap(float& winX, float& winY, float winW, float winH, float& playerHP) {
        winX = snapTargetX;
        winY = snapTargetY;
        clampWin(winX, winY, winW, winH, screenW, screenH);
        fx.snapWarn = false;
        fx.shakePulse = true;
        HurtPlayer(playerHP, enraged ? 8.0f : 5.0f);
    }

    void chipBoss(float frac) {
        float d = maxHp * frac;
        if (d < 1.0f) d = 1.0f;
        hp -= d;
        if (hp <= 0.0f) alive = false;
    }

    void updateSwarm(float px, float py, float dt, float& playerHP) {
        for (auto& s : swarm) {
            if (!s.alive) continue;
            if (s.pulled && blackHoleActive) {
                float dx = holeX - s.x, dy = holeY - s.y;
                float d = std::sqrt(dx * dx + dy * dy) + 1e-3f;
                float pull = enraged ? 520.0f : 420.0f;
                s.vx += (dx / d) * pull * dt;
                s.vy += (dy / d) * pull * dt;
                float maxSp = enraged ? 980.0f : 820.0f;
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
            if (blackHoleActive) {
                float hdx = s.x - holeX, hdy = s.y - holeY;
                if (hdx * hdx + hdy * hdy < (holeRadius + 8.0f) * (holeRadius + 8.0f)) {
                    s.alive = false;
                    chipBoss(enraged ? 0.0025f : 0.0018f);
                    float adx = px - holeX, ady = py - holeY;
                    if (adx * adx + ady * ady < (holeRadius + 55.0f) * (holeRadius + 55.0f))
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
                HurtPlayer(playerHP, 10.0f * dt);
            if (b.life <= 0.0f ||
                b.x < -400.0f || b.x > screenW + 400.0f ||
                b.y < -400.0f || b.y > screenH + 400.0f)
                b.alive = false;
        }
        bars.erase(std::remove_if(bars.begin(), bars.end(),
            [](const PGlitchBar& b) { return !b.alive; }), bars.end());
    }

    void Update(float px, float py, float dt, float& playerHP,
                std::vector<Bullet>& /*bullets*/,
                float& winX, float& winY, float winW, float winH) {
        if (!alive) return;

        checkEnrage();
        glitchT += dt;
        if (vulnTimer > 0.0f) vulnTimer -= dt;

        formTimer += dt;
        if (formTimer >= formDuration && !triWarn && !blackHoleActive && snapTimer <= 0.0f)
            pickForm(false);

        resetFx();

        switch (form) {
        case PForm::SINGULARITY:
            worldX += (holeX - worldX) * 2.0f * dt;
            worldY += (holeY - worldY - 120.0f) * 2.0f * dt;
            if (!triWarn && !blackHoleActive) {
                singularityCd -= dt;
                if (singularityCd <= 0.0f) startSingularityWarn();
            }
            if (triWarn) {
                triWarnTimer -= dt;
                if (triWarnTimer <= 0.0f) {
                    triWarn = false;
                    blackHoleActive = true;
                    singularityTimer = enraged ? 3.8f : 3.2f;
                    singularitySpawn = 0.0f;
                    holeGrow = 0.0f;
                }
            }
            if (blackHoleActive) {
                singularityTimer -= dt;
                singularitySpawn -= dt;
                holeGrow += dt;
                holeRadius = std::min(enraged ? 105.0f : 88.0f, holeGrow * (enraged ? 95.0f : 78.0f));
                if (singularitySpawn <= 0.0f) {
                    singularitySpawn = enraged ? 0.06f : 0.085f;
                    spawnEdgeTriangle();
                }
                float pdx = holeX - px, pdy = holeY - py;
                float pd = std::sqrt(pdx * pdx + pdy * pdy) + 1e-3f;
                if (pd < 420.0f) {
                    float pull = (420.0f - pd) * (enraged ? 0.0009f : 0.00065f);
                    winX += (pdx / pd) * pull * dt * 60.0f;
                    winY += (pdy / pd) * pull * dt * 60.0f;
                    clampWin(winX, winY, winW, winH, screenW, screenH);
                }
                if (singularityTimer <= 0.0f) {
                    blackHoleActive = false;
                    singularityCd = enraged ? 1.2f : 1.8f;
                    vulnTimer = enraged ? 1.0f : 1.3f;
                }
            }
            updateSwarm(px, py, dt, playerHP);
            break;

        case PForm::DISPLACE:
            worldX += sinf(glitchT * 2.4f) * 18.0f * dt;
            worldY += cosf(glitchT * 1.9f) * 14.0f * dt;
            displaceCd -= dt;
            if (displaceCd <= 0.0f && snapTimer <= 0.0f && !fx.snapWarn) {
                beginSnap(winW, winH);
                displaceCd = enraged ? 2.0f : 2.8f;
            }
            if (snapTimer > 0.0f) {
                snapTimer -= dt;
                if (snapTimer <= 0.0f)
                    applySnap(winX, winY, winW, winH, playerHP);
            }
            winX += driftX * dt;
            winY += driftY * dt;
            clampWin(winX, winY, winW, winH, screenW, screenH);
            if (displaceCd > 1.2f && (int)(glitchT * 3.0f) != (int)((glitchT - dt) * 3.0f))
                spawnGlitchBar();
            updateBars(winX, winY, winW, winH, dt, playerHP);
            if (formTimer > formDuration * 0.82f && vulnTimer <= 0.0f)
                vulnTimer = 0.6f;
            break;

        case PForm::PHANTOM:
            worldX = (float)screenW * 0.5f + sinf(glitchT * 1.1f) * 40.0f;
            worldY = (float)screenH * 0.22f + cosf(glitchT * 0.9f) * 22.0f;
            phantomPhase += dt;
            fx.cursorGlitch = true;
            fx.cursorOffX = sinf(glitchT * 6.8f) * (enraged ? 120.0f : 95.0f)
                          + sinf(glitchT * 13.7f) * 35.0f;
            fx.cursorOffY = cosf(glitchT * 5.4f) * (enraged ? 95.0f : 75.0f)
                          + cosf(glitchT * 11.2f) * 28.0f;
            {
                float blinkT = fmodf(phantomPhase, enraged ? 0.28f : 0.38f);
                fx.playerAlpha = (blinkT < (enraged ? 0.13f : 0.17f)) ? 0.12f : 1.0f;
            }
            fx.showGhostWin = true;
            fx.ghostW = winW;
            fx.ghostH = winH;
            fx.ghostX = winX + sinf(glitchT * 3.3f) * 140.0f;
            fx.ghostY = winY + cosf(glitchT * 2.7f) * 110.0f;
            clampWin(fx.ghostX, fx.ghostY, fx.ghostW, fx.ghostH, screenW, screenH);
            if (fmodf(phantomPhase, enraged ? 2.2f : 2.8f) < dt)
                vulnTimer = enraged ? 0.55f : 0.75f;
            break;
        }
    }
};
