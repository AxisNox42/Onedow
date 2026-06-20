#pragma once
#include <glm/glm.hpp>
#include <cmath>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include "Monster.h"
#include "DrawPrim.h"

// ─────────────────────────────────────────────────────────────
// HANG.exe — 응답 없음 프로세스 (pick 0)
//
//   느리게 추격 + 주기적 LAG 장판(플레이어 위치) + 근접 피해
//   P2: 추격·장판↑   P3: 장판 지속↑
// ─────────────────────────────────────────────────────────────

struct HangZone {
    float x = 0, y = 0;
    float radius = 20.0f;
    float life = 0.0f;
    float maxLife = 2.8f;
    float grow = 22.0f;
};

class Boss {
public:
    float worldX = 0, worldY = 0;
    float hp = 7000.0f, maxHp = 7000.0f;
    bool  alive = true;
    bool  exploded = false;

    static inline float WIN_W = 300.0f;
    static inline float WIN_H = 220.0f;
    static inline float BODY_SIZE = 95.0f;

    glm::vec3 color = glm::vec3(0.72f, 0.74f, 0.78f);

    bool  phase2 = false;
    bool  phase3 = false;

    std::vector<HangZone> zones;

    float lagTimer = 0.0f;
    bool  lagPending = false;
    float lagX = 0.0f, lagY = 0.0f;

    float stallTimer = 0.0f;
    bool  stalled = false;

    int screenW = 0, screenH = 0;

    static constexpr float CONTACT_R   = 72.0f;
    static constexpr float CONTACT_DPS = 20.0f;
    static constexpr float LAG_INTERVAL = 4.8f;
    static constexpr float LAG_WARN     = 0.75f;

    Boss(float sx, float sy, int sw, int sh, float maxHpInit = 7000.0f)
        : worldX(sx), worldY(sy), screenW(sw), screenH(sh)
    {
        hp = maxHp = maxHpInit;
    }

    const wchar_t* stateTag() const {
        if (phase3) return L"DEADLOCK";
        if (phase2) return L"NOT RESP";
        if (lagPending) return L"LAG◉";
        if (stalled) return L"FREEZE";
        return L"HANG";
    }

    void Update(float playerCX, float playerCY, float dt,
                float& playerHP,
                std::vector<Monster*>& /*outSummons*/)
    {
        if (!alive) return;

        if (!phase2 && hp <= maxHp * 0.5f) phase2 = true;
        if (!phase3 && hp <= maxHp * 0.25f) phase3 = true;

        float p2 = phase2 ? 1.0f : 0.0f;
        float p3 = phase3 ? 1.0f : 0.0f;

        tickZones(playerCX, playerCY, dt, playerHP, p2, p3);
        tickLag(playerCX, playerCY, dt, p2, p3);

        if (!stalled)
            tickChase(playerCX, playerCY, dt, 0.95f + p2 * 0.35f + p3 * 0.2f);

        float dx = playerCX - worldX, dy = playerCY - worldY;
        if (dx * dx + dy * dy < CONTACT_R * CONTACT_R)
            playerHP -= CONTACT_DPS * dt * (1.0f + p2 * 0.25f + p3 * 0.35f);
    }

    void renderZones(float gt) const {
        for (auto& z : zones) {
            if (z.life <= 0.0f) continue;
            float t = z.life / z.maxLife;
            float pulse = 0.55f + 0.45f * sinf(gt * 10.0f + z.x * 0.02f);
            float a = t * pulse * 0.38f;
            drawCircle(z.x, z.y, z.radius, 0.55f, 0.58f, 0.62f, a);
            drawCircle(z.x, z.y, z.radius * 0.45f, 0.75f, 0.78f, 0.82f, a * 0.6f);
        }
    }

    void renderBody(float gt) const {
        float wl = worldX - WIN_W * 0.5f;
        float wt = worldY - WIN_H * 0.5f;
        float th = 28.0f;

        drawRect(wl, wt, WIN_W, WIN_H, 0.12f, 0.13f, 0.16f, 0.92f);
        drawRect(wl, wt, WIN_W, th,
                 stalled ? 0.55f : 0.42f, 0.44f, 0.48f, 0.96f);
        drawNeonBorder(wl, wt, WIN_W, WIN_H, 0.65f, 0.68f, 0.74f);

        float spin = stalled ? 0.0f : gt * 2.2f;
        float cx = worldX, cy = worldY + 8.0f;
        float hs = 22.0f;
        float topY = cy - hs * 0.55f;
        float botY = cy + hs * 0.55f;
        float lw = hs * 0.38f;
        BatchTri(cx - lw, topY, cx + lw, topY, cx, topY + hs * 0.42f,
                 0.82f, 0.84f, 0.88f, 0.85f);
        BatchTri(cx - lw, botY, cx + lw, botY, cx, botY - hs * 0.42f,
                 0.72f, 0.74f, 0.78f, 0.85f);
        if (!stalled) {
            float sand = sinf(spin) * 4.0f;
            drawRect(cx - 3.0f, cy - 2.0f + sand, 6.0f, 4.0f, 0.9f, 0.55f, 0.2f, 0.7f);
        }
    }

private:
    void tickChase(float px, float py, float dt, float spd) {
        float dx = px - worldX, dy = py - worldY;
        float d = sqrtf(dx * dx + dy * dy) + 1e-3f;
        worldX += dx / d * spd * 62.0f * dt;
        worldY += dy / d * spd * 62.0f * dt;
        float m = BODY_SIZE + 12.0f;
        if (worldX < m) worldX = m;
        if (worldX > (float)screenW - m) worldX = (float)screenW - m;
        if (worldY < m) worldY = m;
        if (worldY > (float)screenH - m) worldY = (float)screenH - m;
    }

    void tickLag(float px, float py, float dt, float p2, float p3) {
        float mul = 1.0f + p2 * 0.4f + p3 * 0.35f;
        lagTimer += dt * mul;

        if (!lagPending && lagTimer >= LAG_INTERVAL - LAG_WARN) {
            lagPending = true;
            lagX = px;
            lagY = py;
            stalled = true;
            stallTimer = 0.0f;
        }
        if (lagPending) {
            stallTimer += dt;
            if (lagTimer >= LAG_INTERVAL) {
                lagTimer = 0.0f;
                lagPending = false;
                stalled = false;
                spawnZone(lagX, lagY, p2, p3);
            }
        }
    }

    int zoneCap() const {
        if (phase3) return 7;
        if (phase2) return 5;
        return 4;
    }

    void spawnZone(float x, float y, float p2, float p3) {
        if ((int)zones.size() >= zoneCap()) {
            auto it = std::min_element(zones.begin(), zones.end(),
                [](const HangZone& a, const HangZone& b) { return a.life < b.life; });
            if (it != zones.end()) *it = HangZone{};
        }
        HangZone z;
        z.x = x; z.y = y;
        z.radius = 26.0f + p2 * 8.0f;
        z.maxLife = z.life = phase3 ? 3.4f : (phase2 ? 2.9f : 2.5f);
        z.grow = 18.0f + p2 * 6.0f + p3 * 4.0f;
        zones.push_back(z);
    }

    void tickZones(float px, float py, float dt, float& playerHP, float p2, float p3) {
        for (auto& z : zones) {
            if (z.life <= 0.0f) continue;
            z.life -= dt;
            z.radius += z.grow * dt;
            float dx = px - z.x, dy = py - z.y;
            if (dx * dx + dy * dy < (z.radius + 4.0f) * (z.radius + 4.0f)) {
                float rate = phase3 ? 9.0f : (phase2 ? 7.0f : 5.5f);
                playerHP -= rate * dt;
            }
        }
        zones.erase(std::remove_if(zones.begin(), zones.end(),
            [](const HangZone& z) { return z.life <= 0.0f; }), zones.end());
    }
};
