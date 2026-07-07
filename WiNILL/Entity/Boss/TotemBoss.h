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

// FLAGSHIP.sys — 느린 대형 기함 (궤도 드론 + 야마토 + 정화자)
//   · 느린 기동(사거리 유지 + 측면 드리프트)
//   · 인터셉터 궤도/교전
//   · 야마토 직선 포격, 현측 포격, 정화자 타격 예고
class TotemBoss {
public:
    struct Interceptor {
        float x = 0.0f, y = 0.0f;
        float orbitAng = 0.0f;
        float hp = 0.0f, maxHp = 0.0f;
        bool  alive = false;
        bool  aggressive = false;
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

    static constexpr int N_INT = 8;
    Interceptor ints[N_INT];
    int intCap = 4;
    int lastIntCap = 4;

    float moveSpeed = 52.0f;
    float preferDist = 300.0f;
    float hullFlash = 0.0f;

    float broadsideCd = 2.2f;
    float intLaunchCd = 4.5f;
    float purifierCd  = 7.5f;

    enum class YamPhase { Idle, Charge, Fire };
    YamPhase yamPhase = YamPhase::Idle;
    float yamCd = 6.0f;
    float yamTimer = 0.0f;
    float yamAim = 0.0f;

    bool  purActive = false;
    float purX = 0.0f, purY = 0.0f;
    float purTimer = 0.0f;

    static constexpr float BODY       = 88.0f;
    static constexpr float INT_HIT    = 14.0f;
    static constexpr float MAP_PAD    = 96.0f;
    static constexpr float INT_ORBIT  = 118.0f;
    static constexpr float INT_HP_RATIO = 0.05f;

    float intHpMax() const {
        return (maxHp > 0.0f) ? maxHp * INT_HP_RATIO : 1.0f;
    }

    // main.cpp 호환 스텁
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

    TotemBoss(int sw, int sh, float hpInit) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.5f;
        worldY = sh * 0.36f;
        syncPhase();
        initInterceptors(true);
    }

    int aliveInterceptors() const {
        int n = 0;
        for (int i = 0; i < N_INT; i++) if (ints[i].alive) ++n;
        return n;
    }

    void syncPhase() {
        lastIntCap = intCap;
        float r = (maxHp > 0.0f) ? hp / maxHp : 1.0f;
        if (r > 0.66f) intCap = 4;
        else if (r > 0.33f) intCap = 6;
        else intCap = 8;
        if (intCap > lastIntCap) {
            for (int i = lastIntCap; i < intCap; i++) {
                ints[i].alive = true;
                ints[i].maxHp = ints[i].hp = intHpMax();
                ints[i].aggressive = false;
                ints[i].orbitAng = (float)i * (6.283f / (float)intCap);
                ints[i].shootCd = 0.4f;
            }
        }
    }

    void initInterceptors(bool fresh) {
        for (int i = 0; i < N_INT; i++) {
            bool live = (i < intCap);
            if (!fresh && !ints[i].alive && !live) continue;
            if (fresh || (live && ints[i].maxHp <= 0.0f)) {
                ints[i].alive = live;
                ints[i].maxHp = ints[i].hp = live ? intHpMax() : 0.0f;
                ints[i].aggressive = false;
                ints[i].orbitAng = (float)i * (6.283f / (float)(intCap > 0 ? intCap : 1));
                ints[i].shootCd = (float)(rand() % 80) * 0.01f;
                ints[i].hitFlash = 0.0f;
            }
        }
    }

    void repositionBays() {}  // legacy no-op

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

    void fireDir(std::vector<Bullet>& b, float ox, float oy, float dx, float dy,
                 float sp, glm::vec3 col, float sz = 1.0f) {
        Bullet bb(ox, oy, ox + dx * 100.0f, oy + dy * 100.0f);
        bb.isEnemy = true; bb.speed = sp; bb.color = col; bb.sizeScale = sz;
        b.push_back(bb);
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
        float gtSpin = dt * (0.9f + (float)(8 - intCap) * 0.08f);
        for (int i = 0; i < N_INT; i++) {
            Interceptor& ic = ints[i];
            if (!ic.alive || i >= intCap) continue;
            if (ic.hitFlash > 0.0f) ic.hitFlash -= dt;
            ic.orbitAng += gtSpin * (ic.aggressive ? 1.8f : 1.0f);
            ic.aimAng = ic.orbitAng + 1.571f;
            float rad = ic.aggressive ? INT_ORBIT * 0.72f : INT_ORBIT;
            float ox = cosf(ic.orbitAng) * rad;
            float oy = sinf(ic.orbitAng) * rad * 0.65f;
            if (ic.aggressive) {
                float tx = px - worldX, ty = py - worldY;
                float td = sqrtf(tx * tx + ty * ty);
                if (td > 1.0f) { ox += tx / td * 42.0f; oy += ty / td * 28.0f; }
            }
            ic.x = worldX + ox;
            ic.y = worldY + oy;
            ic.shootCd -= dt;
            if (ic.shootCd <= 0.0f) {
                float bx = px - ic.x, by = py - ic.y;
                float bd = sqrtf(bx * bx + by * by);
                if (bd > 1.0f) {
                    ic.aimAng = atan2f(by / bd, bx / bd);
                    fireDir(bullets, ic.x, ic.y, bx / bd, by / bd,
                            300.0f + (float)(rand() % 60),
                            glm::vec3(0.55f, 0.95f, 1.0f), 0.75f);
                }
                ic.shootCd = 0.95f + (float)(rand() % 50) * 0.01f;
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
        for (int s = -2; s <= 2; s++) {
            float px = -dy * (float)s * 0.12f, py = dx * (float)s * 0.12f;
            fireDir(bullets, ox, oy, dx + px, dy + py, 420.0f,
                    glm::vec3(1.0f, 0.5f, 0.2f), 0.95f);
        }
    }

    void startPurifier(float px, float py) {
        purActive = true;
        purX = px; purY = py;
        purTimer = 1.05f;
    }

    void firePurifier(float& playerHP, std::vector<Bullet>& bullets) {
        for (int i = 0; i < 16; i++) {
            float a = (float)i * 0.393f;
            fireDir(bullets, purX, purY, cosf(a), sinf(a),
                    260.0f + (float)(rand() % 40), glm::vec3(0.95f, 0.88f, 0.35f), 0.85f);
        }
        float dx = worldX - purX, dy = worldY - purY;
        if (dx * dx + dy * dy > 1.0f) {
            float d = sqrtf(dx * dx + dy * dy);
            fireDir(bullets, purX, purY, dx / d, dy / d, 380.0f,
                    glm::vec3(1.0f, 0.92f, 0.45f), 1.2f);
        }
        float pdx = purX - worldX, pdy = purY - worldY;
        // player damage checked in Update with external px,py — store radius hit
        (void)playerHP;
        (void)pdx;
    }

    void launchInterceptors() {
        for (int i = 0; i < intCap; i++) {
            if (!ints[i].alive) {
                ints[i].alive = true;
                ints[i].hp = ints[i].maxHp = intHpMax();
            }
            ints[i].aggressive = true;
            ints[i].shootCd = 0.15f;
        }
    }

    void onIntDamaged(int idx) {
        if (idx < 0 || idx >= N_INT) return;
        if (!ints[idx].alive) return;
        ints[idx].hitFlash = 0.14f;
    }

    void onIntKilled(int idx) {
        if (idx < 0 || idx >= N_INT) return;
        ints[idx].alive = false;
        ints[idx].hp = 0.0f;
        ints[idx].aggressive = false;
    }

    void Update(float px, float py, float dt, float& playerHP,
                std::vector<Bullet>& bullets,
                float& /*pullX*/, float& /*pullY*/) {
        if (!alive) return;

        syncPhase();
        if (hullFlash > 0.0f) hullFlash -= dt;

        updateMovement(px, py, dt);
        updateInterceptors(px, py, dt, bullets);

        float ddx = px - worldX, ddy = py - worldY;
        if (ddx * ddx + ddy * ddy < (BODY * 0.85f) * (BODY * 0.85f))
            HurtPlayer(playerHP, 14.0f * dt);

        broadsideCd -= dt;
        if (broadsideCd <= 0.0f) {
            fireBroadside(bullets);
            broadsideCd = 3.2f + (float)(rand() % 30) * 0.04f;
        }

        intLaunchCd -= dt;
        if (intLaunchCd <= 0.0f) {
            launchInterceptors();
            intLaunchCd = 5.5f + (float)(rand() % 40) * 0.05f;
        }

        if (yamPhase == YamPhase::Idle) {
            yamCd -= dt;
            if (yamCd <= 0.0f) startYamato(px, py);
        } else if (yamPhase == YamPhase::Charge) {
            yamTimer -= dt;
            if (yamTimer <= 0.0f) {
                yamPhase = YamPhase::Fire;
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
                firePurifier(playerHP, bullets);
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

    void renderEnginePlume(float lx, float ly, float t) const {
        float ex, ey;
        localToWorld(lx, ly, ex, ey);
        float pulse = 0.5f + 0.5f * sinf(t * 8.0f + lx);
        drawCircle(ex, ey, 16.0f + pulse * 6.0f, 0.2f, 0.85f, 1.0f, 0.35f + pulse * 0.25f);
        drawCircle(ex, ey, 9.0f, 0.85f, 0.98f, 1.0f, 0.65f);
    }

    void renderHull(float t) const {
        float pulse = 0.5f + 0.5f * sinf(t * 2.8f);
        float flash = (hullFlash > 0.0f) ? hullFlash / 0.18f : 0.0f;

        float pts[8][2] = {
            { 78.0f,  0.0f }, { 42.0f, 32.0f }, { -58.0f, 36.0f }, { -82.0f, 18.0f },
            { -82.0f, -18.0f }, { -58.0f, -36.0f }, { 42.0f, -32.0f }, { 78.0f, 0.0f }
        };
        for (int i = 0; i < 8; i++) {
            float x0, y0, x1, y1;
            int j = (i + 1) % 8;
            localToWorld(pts[i][0], pts[i][1], x0, y0);
            localToWorld(pts[j][0], pts[j][1], x1, y1);
            float mx = (x0 + x1) * 0.5f, my = (y0 + y1) * 0.5f;
            float dx = x1 - x0, dy = y1 - y0;
            float len = sqrtf(dx * dx + dy * dy);
            float ang = atan2f(dy, dx);
            (void)ang;
            drawRect(mx - len * 0.5f, my - 5.0f, len, 10.0f,
                     0.16f + flash * 0.2f, 0.18f + flash * 0.15f, 0.22f + flash * 0.1f, 0.92f);
        }

        float bx, by, dx0, dy0, dx1, dy1;
        localToWorld(-18.0f, 0.0f, bx, by);
        drawRect(bx - 48.0f, by - 22.0f, 96.0f, 44.0f, 0.10f, 0.11f, 0.14f, 0.95f);
        localToWorld(34.0f, 24.0f, dx0, dy0);
        localToWorld(34.0f, -24.0f, dx1, dy1);
        drawRect(dx0 - 14.0f, dy0 - 8.0f, 28.0f, 16.0f, 0.08f, 0.09f, 0.12f, 0.9f);
        drawRect(dx1 - 14.0f, dy1 - 8.0f, 28.0f, 16.0f, 0.08f, 0.09f, 0.12f, 0.9f);

        localToWorld(52.0f, 0.0f, bx, by);
        drawMercedes(bx, by, 22.0f + pulse * 3.0f, 0.95f, 0.82f, 0.28f, 0.92f);
        localToWorld(62.0f, 0.0f, bx, by);
        drawTriangle(bx, by, 20.0f, 0.35f, 0.92f, 1.0f, 0.85f);

        renderEnginePlume(-76.0f, 20.0f, t);
        renderEnginePlume(-76.0f, -20.0f, t);

        float minx = worldX - BODY, miny = worldY - BODY * 0.7f;
        drawNeonBorder(minx, miny, BODY * 2.0f, BODY * 1.4f,
                       0.35f + flash * 0.4f, 0.88f + flash * 0.1f, 1.0f);
    }

    // 커서형 파임 다이아 — 한쪽 모서리가 파인 6각 실루엣
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
            BatchTri(cx, cy, wx[i], wy[i], wx[j], wy[j], r, g, b, a);
        }
    }

    void renderInterceptors(float t) const {
        for (int i = 0; i < N_INT; i++) {
            const Interceptor& ic = ints[i];
            if (!ic.alive || i >= intCap) continue;
            float flash = (ic.hitFlash > 0.0f) ? ic.hitFlash / 0.14f : 0.0f;
            float pulse = 0.5f + 0.5f * sinf(t * 6.0f + ic.orbitAng);
            float sz = 11.0f + pulse * 2.5f;
            float aim = ic.aimAng;
            drawCircle(ic.x, ic.y, sz * 0.85f, 0.15f, 0.65f, 0.95f, 0.18f + pulse * 0.12f);
            drawCursorShard(ic.x, ic.y, sz, aim,
                            0.35f + flash * 0.45f, 0.88f + flash * 0.1f, 1.0f, 0.92f);
            drawCursorShard(ic.x, ic.y, sz * 0.42f, aim,
                            0.12f, 0.22f, 0.28f, 0.75f);
        }
    }

    void renderTelegraphs(float t, float px, float py) const {
        if (yamPhase == YamPhase::Charge) {
            float prog = 1.0f - yamTimer / 1.35f;
            if (prog < 0.0f) prog = 0.0f;
            if (prog > 1.0f) prog = 1.0f;
            float ox, oy;
            localToWorld(70.0f, 0.0f, ox, oy);
            float dx = cosf(yamAim), dy = sinf(yamAim);
            for (int s = 0; s < 12; s++) {
                float u = (float)s / 11.0f;
                float len = 120.0f + u * 420.0f;
                drawRect(ox + dx * len - 3.0f, oy + dy * len - 3.0f, 6.0f, 6.0f,
                         1.0f, 0.25f + prog * 0.35f, 0.1f, 0.15f + prog * 0.45f);
            }
            wchar_t warn[16];
            swprintf_s(warn, L"YAMATO");
            g_TextS.Draw(warn, ox - 28.0f, oy - 42.0f, 0.52f,
                         1.0f, 0.35f, 0.12f, 0.5f + prog * 0.5f);
        }
        if (purActive) {
            float prog = 1.0f - purTimer / 1.05f;
            float rad = 36.0f + prog * 40.0f;
            float a = 0.12f + prog * 0.35f;
            drawCircle(purX, purY, rad, 0.95f, 0.85f, 0.28f, a);
            drawCircle(purX, purY, rad * 0.55f, 1.0f, 0.95f, 0.5f, a * 1.2f);
            for (int i = 0; i < 4; i++) {
                float ang = t * 2.5f + (float)i * 1.571f;
                drawRect(purX + cosf(ang) * rad - 2.0f, purY + sinf(ang) * rad - 2.0f,
                         4.0f, 4.0f, 1.0f, 0.92f, 0.4f, 0.5f);
            }
        }
        (void)px; (void)py;
    }

    void renderCore(float t) const { renderShip(t, worldX, worldY); }

    void renderShip(float t, float /*px*/, float /*py*/) const {
        renderHull(t);
        renderInterceptors(t);
        renderTelegraphs(t, 0.0f, 0.0f);
    }

    float yamatoDisplayCd() const {
        if (yamPhase == YamPhase::Charge) return yamTimer;
        return yamCd;
    }

    const wchar_t* yamatoLabel() const {
        return (yamPhase == YamPhase::Charge) ? L"YAMATO" : L"YAMATO CD";
    }
};
