#pragma once
#include <glm/glm.hpp>
#include <cmath>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include "Monster.h"
#include "DrawPrim.h"

// ─────────────────────────────────────────────────────────────
// OVERLAY.dll — DWM 투명 오버레이 납치 보스
//
//   거대 가짜 창이 떠다니며 테두리·낙하 패널로 압박
//   약점: 타이틀바만 피격 (클라이언트 영역은 허공)
//   P2: 창 확대 + 패널 가속  P3: TOPMOST 스냅
// ─────────────────────────────────────────────────────────────

struct OverlayPane {
    float x = 0, y = 0;
    float w = 120.0f, h = 88.0f;
    float life = 0.0f;
    float maxLife = 4.0f;
};

class Boss {
public:
    float worldX = 0, worldY = 0;
    float hp = 7000.0f, maxHp = 7000.0f;
    bool  alive = true;
    bool  exploded = false;

    static inline float WIN_W = 520.0f;
    static inline float WIN_H = 380.0f;
    static inline float BODY_SIZE = 260.0f;   // 근접 판정 보조
    static constexpr float TITLE_H = 34.0f;
    static constexpr float BORDER_D = 14.0f;

    glm::vec3 color = glm::vec3(0.55f, 0.72f, 1.0f);

    float sizeScale = 1.0f;
    bool  phase2 = false;
    bool  phase3 = false;

    std::vector<OverlayPane> panes;

    float targetX = 0, targetY = 0;
    float wanderTimer = 0.0f;

    float paneTimer = 0.0f;
    bool  panePending = false;
    float paneX = 0.0f, paneY = 0.0f;

    float snapTimer = 0.0f;
    bool  snapPending = false;
    float snapX = 0.0f, snapY = 0.0f;

    int screenW = 0, screenH = 0;

    static constexpr float PANE_INTERVAL = 4.4f;
    static constexpr float PANE_WARN     = 0.85f;
    static constexpr float SNAP_INTERVAL = 7.5f;
    static constexpr float SNAP_WARN     = 0.75f;

    Boss(float sx, float sy, int sw, int sh, float maxHpInit = 7000.0f)
        : worldX(sx), worldY(sy), targetX(sx), targetY(sy)
        , screenW(sw), screenH(sh)
    {
        hp = maxHp = maxHpInit;
    }

    float winLeft()   const { return worldX - WIN_W * sizeScale * 0.5f; }
    float winTop()    const { return worldY - WIN_H * sizeScale * 0.5f; }
    float winRight()  const { return worldX + WIN_W * sizeScale * 0.5f; }
    float winBottom() const { return worldY + WIN_H * sizeScale * 0.5f; }

    void hurtFocus(float& hx, float& hy) const {
        hx = worldX;
        hy = winTop() + TITLE_H * sizeScale * 0.5f;
    }

    float hurtReach() const { return WIN_W * sizeScale * 0.52f; }

    const wchar_t* stateTag() const {
        if (phase3) return L"TOPMOST";
        if (phase2) return L"GLASS+";
        if (snapPending) return L"SNAP◉";
        if (panePending) return L"PANE◉";
        return L"OVERLAY";
    }

    static float segRectDist(float ax, float ay, float bx, float by,
                             float left, float top, float right, float bottom) {
        float best = 1e9f;
        for (int i = 0; i <= 10; i++) {
            float u = (float)i / 10.0f;
            float px = ax + (bx - ax) * u;
            float py = ay + (by - ay) * u;
            float cx = px; if (cx < left) cx = left; if (cx > right) cx = right;
            float cy = py; if (cy < top)  cy = top;  if (cy > bottom) cy = bottom;
            float dx = px - cx, dy = py - cy;
            float d2 = dx * dx + dy * dy;
            if (d2 < best) best = d2;
        }
        return sqrtf(best);
    }

    bool shotHitsSeg(float ax, float ay, float bx, float by) const {
        float barBot = winTop() + TITLE_H * sizeScale;
        return segRectDist(ax, ay, bx, by, winLeft(), winTop(), winRight(), barBot) < 8.0f;
    }

    void pickNewTarget() {
        float margin = 120.0f;
        float rw = (float)(screenW - 2 * (int)margin);
        float rh = (float)(screenH - 2 * (int)margin);
        if (rw < 1) rw = 1; if (rh < 1) rh = 1;
        targetX = margin + (float)(rand() % (int)rw);
        targetY = margin + (float)(rand() % (int)rh);
    }

    void Update(float playerCX, float playerCY, float dt,
                float& playerHP,
                std::vector<Monster*>& /*outSummons*/)
    {
        if (!alive) return;

        if (!phase2 && hp <= maxHp * 0.5f) { phase2 = true; sizeScale = 1.16f; }
        if (!phase3 && hp <= maxHp * 0.25f) { phase3 = true; sizeScale = 1.26f; }

        float p2 = phase2 ? 1.0f : 0.0f;
        float p3 = phase3 ? 1.0f : 0.0f;

        tickPanes(playerCX, playerCY, dt, playerHP);
        tickDrift(playerCX, playerCY, dt, 1.0f + p2 * 0.35f + p3 * 0.25f);
        tickBorder(playerCX, playerCY, dt, playerHP, p2, p3);
        tickPaneDrop(playerCX, playerCY, dt, p2, p3);
        tickSnap(playerCX, playerCY, dt, p2, p3);
    }

    void renderPanes(float gt) const {
        for (auto& p : panes) {
            if (p.life <= 0.0f) continue;
            float t = p.life / p.maxLife;
            float pulse = 0.55f + 0.45f * sinf(gt * 13.0f + p.x * 0.02f);
            float a = t * pulse * 0.42f;
            drawRect(p.x - p.w * 0.5f, p.y - p.h * 0.5f, p.w, p.h,
                     0.08f, 0.10f, 0.18f, a * 0.85f);
            drawNeonBorder(p.x - p.w * 0.5f, p.y - p.h * 0.5f, p.w, p.h,
                           0.45f, 0.65f, 1.0f);
        }
    }

    void renderWindow(float gt) const {
        float wl = winLeft(), wt = winTop();
        float ww = WIN_W * sizeScale, wh = WIN_H * sizeScale;
        float th = TITLE_H * sizeScale;

        drawRect(wl, wt, ww, wh, 0.05f, 0.06f, 0.10f, 0.88f);
        drawRect(wl, wt, ww, th, 0.18f, 0.22f, 0.38f, 0.96f);
        drawRect(wl + ww - th * 0.95f, wt + th * 0.18f, th * 0.72f, th * 0.64f,
                 0.75f, 0.18f, 0.22f, 0.92f);
        drawRect(wl + ww - th * 1.85f, wt + th * 0.28f, th * 0.38f, th * 0.12f,
                 0.35f, 0.55f, 0.95f, 0.85f);
        drawNeonBorder(wl, wt, ww, wh, color.r, color.g, color.b);

        float cy = wt + th + 12.0f;
        while (cy < wt + wh - 8.0f) {
            float flick = 0.04f + 0.03f * sinf(gt * 9.0f + cy * 0.08f);
            drawRect(wl + 10.0f, cy, ww - 20.0f, 2.0f,
                     0.35f, 0.55f, 1.0f, flick);
            cy += 14.0f;
        }

        float pulse = 0.7f + 0.3f * sinf(gt * 3.2f);
        drawRect(wl + 12.0f, wt + th * 0.22f, ww * 0.42f, th * 0.55f,
                 0.85f * pulse, 0.92f * pulse, 1.0f, 0.55f);

        if (phase3) {
            float a = 0.06f + 0.04f * sinf(gt * 5.0f);
            drawRect(wl - 6.0f, wt - 6.0f, ww + 12.0f, wh + 12.0f,
                     0.55f, 0.35f, 1.0f, a);
        }
    }

private:
    void tickDrift(float px, float py, float dt, float spd) {
        wanderTimer += dt;
        if (wanderTimer >= 2.8f) {
            pickNewTarget();
            wanderTimer = 0.0f;
        }
        float tx = px * 0.55f + targetX * 0.45f;
        float ty = py * 0.55f + targetY * 0.45f;
        float dx = tx - worldX, dy = ty - worldY;
        float d = sqrtf(dx * dx + dy * dy) + 1e-3f;
        worldX += dx / d * spd * 58.0f * dt;
        worldY += dy / d * spd * 58.0f * dt;
        clampToScreen();
    }

    void clampToScreen() {
        float mx = WIN_W * sizeScale * 0.5f + 16.0f;
        float my = WIN_H * sizeScale * 0.5f + 16.0f;
        if (worldX < mx) worldX = mx;
        if (worldX > (float)screenW - mx) worldX = (float)screenW - mx;
        if (worldY < my) worldY = my;
        if (worldY > (float)screenH - my) worldY = (float)screenH - my;
    }

    void tickPaneDrop(float px, float py, float dt, float p2, float p3) {
        float mul = 1.0f + p2 * 0.35f + p3 * 0.45f;
        paneTimer += dt * mul;
        if (!panePending && paneTimer >= PANE_INTERVAL - PANE_WARN) {
            panePending = true;
            paneX = px;
            paneY = py;
        }
        if (panePending && paneTimer >= PANE_INTERVAL) {
            paneTimer = 0.0f;
            panePending = false;
            spawnPane(paneX, paneY, p2, p3);
        }
    }

    void tickSnap(float px, float py, float dt, float p2, float p3) {
        if (!phase2) return;
        float mul = phase3 ? 1.45f : 1.0f;
        snapTimer += dt * mul;
        if (!snapPending && snapTimer >= SNAP_INTERVAL - SNAP_WARN) {
            snapPending = true;
            snapX = px + (float)(rand() % 80 - 40);
            snapY = py + (float)(rand() % 80 - 40);
        }
        if (snapPending && snapTimer >= SNAP_INTERVAL) {
            snapTimer = 0.0f;
            snapPending = false;
            worldX = snapX;
            worldY = snapY;
            clampToScreen();
            spawnPane(snapX, snapY, p2, p3);
        }
    }

    int paneCap() const {
        if (phase3) return 10;
        if (phase2) return 8;
        return 6;
    }

    void spawnPane(float x, float y, float p2, float p3) {
        if ((int)panes.size() >= paneCap()) {
            auto it = std::min_element(panes.begin(), panes.end(),
                [](const OverlayPane& a, const OverlayPane& b) { return a.life < b.life; });
            if (it != panes.end()) *it = OverlayPane{};
        }
        OverlayPane p;
        p.x = x; p.y = y;
        p.w = 110.0f + (float)(rand() % 40) + p2 * 20.0f;
        p.h = 78.0f + (float)(rand() % 30) + p2 * 14.0f;
        p.maxLife = p.life = phase3 ? 5.0f : (phase2 ? 4.2f : 3.6f);
        panes.push_back(p);
    }

    void tickPanes(float px, float py, float dt, float& playerHP) {
        for (auto& p : panes) {
            if (p.life <= 0.0f) continue;
            p.life -= dt;
            if (px >= p.x - p.w * 0.5f && px <= p.x + p.w * 0.5f &&
                py >= p.y - p.h * 0.5f && py <= p.y + p.h * 0.5f) {
                float rate = phase3 ? 13.0f : (phase2 ? 10.0f : 7.5f);
                playerHP -= rate * dt;
            }
        }
        panes.erase(std::remove_if(panes.begin(), panes.end(),
            [](const OverlayPane& p) { return p.life <= 0.0f; }), panes.end());
    }

    void tickBorder(float px, float py, float dt, float& playerHP, float p2, float p3) {
        float bd = BORDER_D * (1.0f + p2 * 0.25f + p3 * 0.35f);
        float wl = winLeft(), wr = winRight(), wt = winTop(), wb = winBottom();

        bool borderHit = false;
        if (px >= wl - bd && px <= wr + bd && py >= wt - bd && py <= wb + bd) {
            bool insideCore = px >= wl + bd && px <= wr - bd &&
                              py >= wt + TITLE_H * sizeScale + bd && py <= wb - bd;
            if (!insideCore) borderHit = true;
        }
        if (borderHit) {
            float rate = 16.0f + p2 * 5.0f + p3 * 7.0f;
            playerHP -= rate * dt;
        }
    }
};
