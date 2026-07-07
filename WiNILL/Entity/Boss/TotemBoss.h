#pragma once
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <glm/glm.hpp>
#include "Bullet.h"
#include "DrawPrim.h"
#include "TextRenderer.h"
#include "PlayerStats.h"

extern TextRenderer g_TextS;

// FLAGSHIP.sys — 와이어프레임 대형 기함 + 플레이어 주변 스웜 드론
class TotemBoss {
public:
    struct Interceptor {
        float x = 0.0f, y = 0.0f;
        float vx = 0.0f, vy = 0.0f;
        float wanderAng = 0.0f;
        float buzzPhase = 0.0f;
        float orbitRad = 100.0f;
        float hp = 0.0f, maxHp = 0.0f;
        bool  alive = false;
        float shootCd = 0.0f;
        float hitFlash = 0.0f;
        float aimAng = 0.0f;
    };

    float worldX, worldY;
    float hp, maxHp;
    bool  alive = true;
    bool  exploded = false;
    int   screenW, screenH;
    float facing = 0.0f;

    std::vector<Interceptor> ints;

    float moveSpeed = 52.0f;
    float preferDist = 300.0f;
    float hullFlash = 0.0f;

    float broadsideCd = 2.2f;
    float intSpawnCd = 0.35f;
    float purifierCd  = 7.5f;

    enum class YamPhase { Idle, Charge, Fire };
    YamPhase yamPhase = YamPhase::Idle;
    float yamCd = 6.0f;
    float yamTimer = 0.0f;
    float yamAim = 0.0f;

    bool  purActive = false;
    float purX = 0.0f, purY = 0.0f;
    float purTimer = 0.0f;

    static constexpr float BODY            = 88.0f;
    static constexpr float INT_HIT         = 14.0f;
    static constexpr float MAP_PAD         = 96.0f;
    static constexpr float INT_HP_RATIO    = 0.05f;
    static constexpr float INT_SPAWN_INT   = 1.0f;
    static constexpr int   MAX_INT_ALIVE   = 26;

    float intHpMax() const {
        return (maxHp > 0.0f) ? maxHp * INT_HP_RATIO : 1.0f;
    }

    bool vulnerable() const { return false; }
    float vulnTimer = 0.0f;
    int wave = 1;
    bool isSkillSealed(int) const { return false; }
    float statDamageMult() const { return 1.0f; }
    int aliveTotems() const { return aliveInterceptors(); }
    void onTotemDamaged(int) {}
    void onTotemKilled(int) {}
    void damageExpandShield(float) {}
    void renderOrbitGuide(float) const {}
    void renderRiteWeb(float) const {}
    void renderLinks(float) const {}
    void renderLaser() const {}
    void repositionBays() {}

    TotemBoss(int sw, int sh, float hpInit) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.5f;
        worldY = sh * 0.36f;
        ints.reserve(MAX_INT_ALIVE);
    }

    int aliveInterceptors() const {
        int n = 0;
        for (const auto& ic : ints) if (ic.alive) ++n;
        return n;
    }

    void clampPos() {
        float m = MAP_PAD;
        if (worldX < m) worldX = m;
        if (worldX > screenW - m) worldX = screenW - m;
        if (worldY < m) worldY = m;
        if (worldY > screenH - m) worldY = screenH - m;
    }

    void localToWorld(float lx, float ly, float& wx, float& wy) const {
        float c = cosf(facing), s = sinf(facing);
        wx = worldX + lx * c - ly * s;
        wy = worldY + lx * s + ly * c;
    }

    static void drawWireSeg(float x0, float y0, float x1, float y1, float thick,
                            float r, float g, float b, float a) {
        float dx = x1 - x0, dy = y1 - y0;
        float len = sqrtf(dx * dx + dy * dy);
        if (len < 0.5f) return;
        float nx = -dy / len * thick * 0.5f;
        float ny =  dx / len * thick * 0.5f;
        BatchTri(x0 + nx, y0 + ny, x1 + nx, y1 + ny, x1 - nx, y1 - ny, r, g, b, a);
        BatchTri(x0 + nx, y0 + ny, x1 - nx, y1 - ny, x0 - nx, y0 - ny, r, g, b, a);
    }

    void drawWireLoop(const float (*pts)[2], int n, float thick,
                      float r, float g, float b, float a) const {
        float x0, y0, x1, y1;
        localToWorld(pts[0][0], pts[0][1], x0, y0);
        for (int i = 1; i < n; i++) {
            localToWorld(pts[i][0], pts[i][1], x1, y1);
            drawWireSeg(x0, y0, x1, y1, thick, r, g, b, a);
            x0 = x1; y0 = y1;
        }
        localToWorld(pts[0][0], pts[0][1], x1, y1);
        drawWireSeg(x0, y0, x1, y1, thick, r, g, b, a);
    }

    void fireDir(std::vector<Bullet>& b, float ox, float oy, float dx, float dy,
                 float sp, glm::vec3 col, float sz = 1.0f) {
        Bullet bb(ox, oy, ox + dx * 100.0f, oy + dy * 100.0f);
        bb.isEnemy = true; bb.speed = sp; bb.color = col; bb.sizeScale = sz;
        b.push_back(bb);
    }

    void initInterceptor(Interceptor& ic, float px, float py) {
        ic.alive = true;
        ic.maxHp = ic.hp = intHpMax();
        float launch = (float)(rand() % 628) * 0.01f;
        ic.x = worldX + cosf(launch) * 36.0f;
        ic.y = worldY + sinf(launch) * 36.0f;
        ic.vx = cosf(launch) * 120.0f;
        ic.vy = sinf(launch) * 120.0f;
        ic.wanderAng = (float)(rand() % 628) * 0.01f;
        ic.buzzPhase = (float)(rand() % 628) * 0.01f;
        ic.orbitRad = 75.0f + (float)(rand() % 110);
        ic.shootCd = 0.25f + (float)(rand() % 40) * 0.01f;
        ic.hitFlash = 0.0f;
        ic.aimAng = atan2f(py - ic.y, px - ic.x);
        (void)px;
    }

    bool spawnInterceptor(float px, float py) {
        for (auto& ic : ints) {
            if (!ic.alive) {
                initInterceptor(ic, px, py);
                return true;
            }
        }
        if ((int)ints.size() >= MAX_INT_ALIVE) return false;
        ints.emplace_back();
        initInterceptor(ints.back(), px, py);
        return true;
    }

    void updateMovement(float px, float py, float dt) {
        float dx = px - worldX, dy = py - worldY;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist > 4.0f) {
            float nx = dx / dist, ny = dy / dist;
            facing = atan2f(dy, dx);
            if (dist > preferDist + 50.0f) {
                worldX += nx * moveSpeed * dt;
                worldY += ny * moveSpeed * dt;
            } else if (dist < preferDist - 50.0f) {
                worldX -= nx * moveSpeed * 0.65f * dt;
                worldY -= ny * moveSpeed * 0.65f * dt;
            } else {
                worldX += -ny * moveSpeed * 0.5f * dt;
                worldY += nx * moveSpeed * 0.5f * dt;
            }
        }
        clampPos();
    }

    void updateInterceptors(float px, float py, float dt, std::vector<Bullet>& bullets) {
        for (auto& ic : ints) {
            if (!ic.alive) continue;
            if (ic.hitFlash > 0.0f) ic.hitFlash -= dt;

            ic.wanderAng += dt * (2.8f + sinf(ic.buzzPhase) * 1.4f);
            ic.buzzPhase += dt * (8.0f + fmodf(ic.orbitRad, 3.0f));
            float rad = ic.orbitRad + sinf(ic.buzzPhase * 1.65f) * 38.0f;
            float gx = px + cosf(ic.wanderAng) * rad;
            float gy = py + sinf(ic.wanderAng) * rad * 0.72f;

            float ax = (gx - ic.x) * 4.5f;
            float ay = (gy - ic.y) * 4.5f;
            ic.vx = ic.vx * 0.86f + ax * dt;
            ic.vy = ic.vy * 0.86f + ay * dt;
            float spd = sqrtf(ic.vx * ic.vx + ic.vy * ic.vy);
            const float maxSpd = 265.0f;
            if (spd > maxSpd) {
                ic.vx *= maxSpd / spd;
                ic.vy *= maxSpd / spd;
            }
            ic.x += ic.vx * dt;
            ic.y += ic.vy * dt;
            ic.aimAng = atan2f(ic.vy, ic.vx);

            ic.shootCd -= dt;
            if (ic.shootCd <= 0.0f) {
                float bx = px - ic.x, by = py - ic.y;
                float bd = sqrtf(bx * bx + by * by);
                if (bd > 1.0f) {
                    ic.aimAng = atan2f(by, bx);
                    fireDir(bullets, ic.x, ic.y, bx / bd, by / bd,
                            290.0f + (float)(rand() % 70),
                            glm::vec3(0.55f, 0.95f, 1.0f), 0.75f);
                }
                ic.shootCd = 0.85f + (float)(rand() % 55) * 0.01f;
            }
        }
    }

    void fireBroadside(std::vector<Bullet>& bullets) {
        float side = (rand() % 2) ? 1.0f : -1.0f;
        float fx, fy, sx, sy;
        localToWorld(48.0f, 0.0f, fx, fy);
        localToWorld(0.0f, side * 34.0f, sx, sy);
        float dx = cosf(facing), dy = sinf(facing);
        float px = -dy * side, py = dx * side;
        for (int i = 0; i < 7; i++) {
            float t = (float)i * 0.14f;
            fireDir(bullets, sx + dx * t * 30.0f, sy + dy * t * 30.0f,
                    dx + px * 0.22f, dy + py * 0.22f,
                    340.0f + (float)(rand() % 50), glm::vec3(1.0f, 0.72f, 0.22f), 0.9f);
        }
        fireDir(bullets, fx, fy, dx, dy, 280.0f, glm::vec3(1.0f, 0.55f, 0.15f), 1.1f);
    }

    void startYamato(float px, float py) {
        yamPhase = YamPhase::Charge;
        yamTimer = 1.35f;
        yamAim = atan2f(py - worldY, px - worldX);
    }

    void fireYamato(std::vector<Bullet>& bullets) {
        float dx = cosf(yamAim), dy = sinf(yamAim);
        float ox, oy;
        localToWorld(72.0f, 0.0f, ox, oy);
        for (int i = 0; i < 5; i++) {
            float t = (float)i * 38.0f;
            fireDir(bullets, ox + dx * t, oy + dy * t, dx, dy,
                    520.0f, glm::vec3(1.0f, 0.35f, 0.12f), 1.35f + (float)i * 0.08f);
        }
    }

    void startPurifier(float px, float py) {
        purActive = true;
        purX = px; purY = py;
        purTimer = 1.05f;
    }

    void firePurifier(std::vector<Bullet>& bullets) {
        for (int i = 0; i < 16; i++) {
            float a = (float)i * 0.393f;
            fireDir(bullets, purX, purY, cosf(a), sinf(a),
                    260.0f + (float)(rand() % 40), glm::vec3(0.95f, 0.88f, 0.35f), 0.85f);
        }
    }

    void onIntDamaged(int idx) {
        if (idx < 0 || idx >= (int)ints.size()) return;
        if (!ints[idx].alive) return;
        ints[idx].hitFlash = 0.14f;
    }

    void onIntKilled(int idx) {
        if (idx < 0 || idx >= (int)ints.size()) return;
        ints[idx].alive = false;
        ints[idx].hp = 0.0f;
    }

    void Update(float px, float py, float dt, float& playerHP,
                std::vector<Bullet>& bullets,
                float& /*pullX*/, float& /*pullY*/) {
        if (!alive) return;

        if (hullFlash > 0.0f) hullFlash -= dt;

        updateMovement(px, py, dt);
        updateInterceptors(px, py, dt, bullets);

        intSpawnCd -= dt;
        if (intSpawnCd <= 0.0f) {
            spawnInterceptor(px, py);
            intSpawnCd = INT_SPAWN_INT;
        }

        float ddx = px - worldX, ddy = py - worldY;
        if (ddx * ddx + ddy * ddy < (BODY * 0.85f) * (BODY * 0.85f))
            HurtPlayer(playerHP, 14.0f * dt);

        broadsideCd -= dt;
        if (broadsideCd <= 0.0f) {
            fireBroadside(bullets);
            broadsideCd = 3.2f + (float)(rand() % 30) * 0.04f;
        }

        if (yamPhase == YamPhase::Idle) {
            yamCd -= dt;
            if (yamCd <= 0.0f) startYamato(px, py);
        } else if (yamPhase == YamPhase::Charge) {
            yamTimer -= dt;
            if (yamTimer <= 0.0f) {
                fireYamato(bullets);
                yamPhase = YamPhase::Idle;
                yamCd = 11.0f + (float)(rand() % 50) * 0.06f;
            }
        }

        if (purActive) {
            purTimer -= dt;
            if (purTimer <= 0.0f) {
                float pdx = px - purX, pdy = py - purY;
                if (pdx * pdx + pdy * pdy < 72.0f * 72.0f)
                    HurtPlayer(playerHP, 22.0f);
                firePurifier(bullets);
                purActive = false;
            }
        } else {
            purifierCd -= dt;
            if (purifierCd <= 0.0f) {
                startPurifier(px, py);
                purifierCd = 8.5f + (float)(rand() % 40) * 0.05f;
            }
        }
    }

    void renderHull(float t) const {
        float pulse = 0.5f + 0.5f * sinf(t * 2.8f);
        float flash = (hullFlash > 0.0f) ? hullFlash / 0.18f : 0.0f;
        float cr = 0.35f + flash * 0.35f;
        float cg = 0.82f + flash * 0.1f;
        float cb = 1.0f;
        float tr = 0.95f + pulse * 0.05f;
        float tg = 0.72f + pulse * 0.1f;
        float tb = 0.18f;

        static const float hull[][2] = {
            { 82.0f, 0.0f }, { 48.0f, 28.0f }, { -8.0f, 34.0f },
            { -78.0f, 22.0f }, { -88.0f, 0.0f }, { -78.0f, -22.0f },
            { -8.0f, -34.0f }, { 48.0f, -28.0f }
        };
        drawWireLoop(hull, 8, 2.6f, cr, cg, cb, 0.95f);

        static const float spine[][2] = { { -72.0f, 0.0f }, { 70.0f, 0.0f } };
        float sx0, sy0, sx1, sy1;
        localToWorld(spine[0][0], spine[0][1], sx0, sy0);
        localToWorld(spine[1][0], spine[1][1], sx1, sy1);
        drawWireSeg(sx0, sy0, sx1, sy1, 1.8f, cr * 0.7f, cg * 0.7f, cb * 0.7f, 0.75f);

        static const float wingL[][2] = { { 10.0f, 18.0f }, { -40.0f, 30.0f }, { -62.0f, 14.0f } };
        static const float wingR[][2] = { { 10.0f, -18.0f }, { -40.0f, -30.0f }, { -62.0f, -14.0f } };
        drawWireLoop(wingL, 3, 1.6f, cr * 0.55f, cg * 0.55f, cb * 0.55f, 0.7f);
        drawWireLoop(wingR, 3, 1.6f, cr * 0.55f, cg * 0.55f, cb * 0.55f, 0.7f);

        float bx, by, ex0, ey0, ex1, ey1;
        localToWorld(58.0f, 0.0f, bx, by);
        drawWireSeg(bx - 8.0f, by, bx + 14.0f, by, 2.0f, tr, tg, tb, 0.9f);
        localToWorld(-74.0f, 16.0f, ex0, ey0);
        localToWorld(-86.0f, 24.0f, ex1, ey1);
        drawWireSeg(ex0, ey0, ex1, ey1, 2.2f, 0.3f, 0.9f, 1.0f, 0.55f + pulse * 0.3f);
        localToWorld(-74.0f, -16.0f, ex0, ey0);
        localToWorld(-86.0f, -24.0f, ex1, ey1);
        drawWireSeg(ex0, ey0, ex1, ey1, 2.2f, 0.3f, 0.9f, 1.0f, 0.55f + pulse * 0.3f);

        localToWorld(68.0f, 0.0f, bx, by);
        for (int i = -2; i <= 2; i++) {
            float ox = bx + (float)i * 5.0f;
            drawWireSeg(ox, by - 6.0f, ox + 10.0f, by, 1.2f, tr, tg, tb, 0.65f);
        }
    }

    static void drawCursorShard(float cx, float cy, float size, float ang,
                                float r, float g, float b, float a) {
        static const float lx[] = { 0.0f,  0.62f,  0.34f,  0.0f, -0.34f, -0.62f };
        static const float ly[] = { -1.0f, -0.18f, 0.62f, 0.38f, 0.62f, -0.18f };
        float wx[6], wy[6];
        float c = cosf(ang), s = sinf(ang);
        for (int i = 0; i < 6; i++) {
            float px = lx[i] * size, py = ly[i] * size;
            wx[i] = cx + px * c - py * s;
            wy[i] = cy + px * s + py * c;
        }
        for (int i = 0; i < 6; i++) {
            int j = (i + 1) % 6;
            drawWireSeg(wx[i], wy[i], wx[j], wy[j], 1.4f, r, g, b, a);
        }
        for (int i = 0; i < 6; i++) {
            int j = (i + 1) % 6;
            BatchTri(cx, cy, wx[i], wy[i], wx[j], wy[j], r * 0.25f, g * 0.25f, b * 0.25f, a * 0.35f);
        }
    }

    static bool inWinPt(float px, float py, float wx, float wy, float ww, float wh) {
        return px >= wx && px <= wx + ww && py >= wy && py <= wy + wh;
    }

    void renderInterceptorsInWin(float t, float wx, float wy, float ww, float wh) const {
        for (const auto& ic : ints) {
            if (!ic.alive) continue;
            if (!inWinPt(ic.x, ic.y, wx, wy, ww, wh)) continue;
            float flash = (ic.hitFlash > 0.0f) ? ic.hitFlash / 0.14f : 0.0f;
            float pulse = 0.5f + 0.5f * sinf(t * 7.0f + ic.buzzPhase);
            float sz = 10.0f + pulse * 2.0f;
            drawWireSeg(ic.x - sz * 0.3f, ic.y, ic.x + sz * 0.5f, ic.y, 1.0f,
                        0.25f, 0.7f, 0.95f, 0.35f + pulse * 0.15f);
            drawCursorShard(ic.x, ic.y, sz, ic.aimAng,
                            0.35f + flash * 0.45f, 0.88f + flash * 0.1f, 1.0f, 0.9f);
        }
    }

    void renderTelegraphs(float t) const {
        if (yamPhase == YamPhase::Charge) {
            float prog = 1.0f - yamTimer / 1.35f;
            if (prog < 0.0f) prog = 0.0f;
            if (prog > 1.0f) prog = 1.0f;
            float ox, oy, lx, ly;
            localToWorld(70.0f, 0.0f, ox, oy);
            float dx = cosf(yamAim), dy = sinf(yamAim);
            for (int s = 0; s < 14; s++) {
                float u = (float)s / 13.0f;
                float len = 100.0f + u * 440.0f;
                lx = ox + dx * len;
                ly = oy + dy * len;
                float lx0 = ox + dx * (len - 28.0f);
                float ly0 = oy + dy * (len - 28.0f);
                drawWireSeg(lx0, ly0, lx, ly, 1.5f + prog * 1.5f,
                            1.0f, 0.3f + prog * 0.3f, 0.12f, 0.2f + prog * 0.55f);
            }
            g_TextS.Draw(L"YAMATO", ox - 28.0f, oy - 42.0f, 0.52f,
                         1.0f, 0.35f, 0.12f, 0.5f + prog * 0.5f);
        }
        if (purActive) {
            float prog = 1.0f - purTimer / 1.05f;
            float rad = 36.0f + prog * 40.0f;
            for (int i = 0; i < 12; i++) {
                float a0 = (float)i * 0.524f;
                float a1 = a0 + 0.42f;
                drawWireSeg(purX + cosf(a0) * rad, purY + sinf(a0) * rad,
                            purX + cosf(a1) * rad, purY + sinf(a1) * rad,
                            1.8f, 0.95f, 0.85f, 0.3f, 0.15f + prog * 0.45f);
            }
        }
        (void)t;
    }

    void renderCore(float t) const {
        renderHull(t);
        renderTelegraphs(t);
    }

    float yamatoDisplayCd() const {
        if (yamPhase == YamPhase::Charge) return yamTimer;
        return yamCd;
    }

    const wchar_t* yamatoLabel() const {
        return (yamPhase == YamPhase::Charge) ? L"YAMATO" : L"YAMATO CD";
    }

    float swarmSpawnCd() const { return intSpawnCd; }
};
