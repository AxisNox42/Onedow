#pragma once
#include <glm/glm.hpp>
#include <cmath>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include "Monster.h"
#include "DrawPrim.h"

// ─────────────────────────────────────────────────────────────
// LEAK.sys — 힙 메모리 누수 보스
//
//   P1: 느린 추격 + 이동 궤적에 누수 구역 · 주기적 ALLOC 소환
//   P2 50%: 팽창 + 누수 노드 2기 · 즉시 누수 버스트
//   P3 25%: GC 실패 — 소환 흡수 회복 · 누수 구역 성장 가속
// ─────────────────────────────────────────────────────────────

struct LeakPool {
    float x = 0, y = 0;
    float life = 0.0f;
    float maxLife = 3.0f;
    float radius = 26.0f;
    float grow = 10.0f;
};

class Boss {
public:
    float worldX = 0, worldY = 0;
    float hp = 7000.0f, maxHp = 7000.0f;
    bool  alive = true;
    bool  exploded = false;

    static inline float WIN_W = 700.0f;
    static inline float WIN_H = 700.0f;
    static inline float BODY_SIZE = 48.0f;

    glm::vec3 color = glm::vec3(0.45f, 0.62f, 1.0f);

    float sizeScale = 1.0f;
    int   splitGen = 0;
    bool  leakNode = false;          // 위성 누수 노드 (본체 아님)

    bool  phase2 = false;
    bool  phase3 = false;

    std::vector<LeakPool> pools;

    float targetX = 0, targetY = 0;
    float wanderTimer = 0.0f;
    float trailTimer = 0.0f;

    float allocTimer = 0.0f;
    bool  summonPending = false;     // ALLOC 경고 (main 렌더 호환)

    float dripTimer = 0.0f;
    bool  dripPending = false;
    float dripX = 0.0f, dripY = 0.0f;

    bool  p2BurstReady_ = false;

    int screenW = 0, screenH = 0;

    static constexpr float CONTACT_RADIUS = 52.0f;
    static constexpr float CONTACT_DPS    = 22.0f;

    static constexpr float ALLOC_INTERVAL = 3.2f;
    static constexpr float ALLOC_WARN     = 0.7f;
    static constexpr int   ALLOC_COUNT    = 5;
    static constexpr float ALLOC_RING_R   = 82.0f;

    static constexpr float DRIP_INTERVAL = 5.2f;
    static constexpr float DRIP_WARN     = 0.9f;

    Boss(float sx, float sy, int sw, int sh, float maxHpInit = 7000.0f)
        : worldX(sx), worldY(sy), targetX(sx), targetY(sy)
        , screenW(sw), screenH(sh)
    {
        hp = maxHp = maxHpInit;
    }

    const wchar_t* stateTag() const {
        if (phase3) return L"GC FAIL";
        if (phase2) return L"HEAP+";
        if (dripPending) return L"DRIP◉";
        if (summonPending) return L"ALLOC◉";
        return leakNode ? L"NODE" : L"LEAK";
    }

    void pickNewTarget() {
        float margin = 90.0f;
        float rw = (float)(screenW - 2 * (int)margin);
        float rh = (float)(screenH - 2 * (int)margin);
        if (rw < 1) rw = 1; if (rh < 1) rh = 1;
        targetX = margin + (float)(rand() % (int)rw);
        targetY = margin + (float)(rand() % (int)rh);
    }

    void Update(float playerCX, float playerCY, float dt,
                float& playerHP,
                std::vector<Monster*>& outSummons)
    {
        if (!alive) return;

        if (!phase2 && !leakNode && hp <= maxHp * 0.5f) enterPhase2();
        if (!phase3 && !leakNode && hp <= maxHp * 0.25f) enterPhase3();

        float p2 = phase2 ? 1.0f : 0.0f;
        float p3 = phase3 ? 1.0f : 0.0f;
        float driftSpd = (1.35f + p2 * 0.55f + p3 * 0.35f) * (leakNode ? 1.1f : 1.0f);
        float allocMul = 1.0f + p2 * 0.45f + p3 * 0.35f;

        tickPools(playerCX, playerCY, dt, playerHP);
        tickDrift(playerCX, playerCY, dt, driftSpd);
        tickTrail(dt, p2, p3);

        if (!leakNode) {
            allocTimer += dt * allocMul;
            if (!summonPending && allocTimer >= ALLOC_INTERVAL - ALLOC_WARN)
                summonPending = true;
            if (allocTimer >= ALLOC_INTERVAL) {
                allocTimer = 0.0f;
                summonPending = false;
                allocRing(outSummons, phase3 ? ALLOC_COUNT + 2 : ALLOC_COUNT);
            }

            dripTimer += dt;
            if (!dripPending && dripTimer >= DRIP_INTERVAL - DRIP_WARN) {
                dripPending = true;
                dripX = playerCX;
                dripY = playerCY;
            }
            if (dripPending && dripTimer >= DRIP_INTERVAL) {
                dripTimer = 0.0f;
                dripPending = false;
                spawnPool(dripX, dripY, phase3 ? 1.55f : (phase2 ? 1.25f : 1.0f));
            }
        }

        float cr = CONTACT_RADIUS * sizeScale;
        float dx = playerCX - worldX, dy = playerCY - worldY;
        if (dx * dx + dy * dy < cr * cr)
            playerHP -= CONTACT_DPS * dt * (1.0f + p2 * 0.35f + p3 * 0.5f);
    }

    void TickAbsorb(std::vector<Monster*>& monsters) {
        if (!phase3 || leakNode || !alive) return;
        float ar = BODY_SIZE * sizeScale * 2.5f;
        for (auto it = monsters.begin(); it != monsters.end(); ) {
            Monster* m = *it;
            if (!m->alive || !m->summoned) { ++it; continue; }
            float dx = m->worldX - worldX, dy = m->worldY - worldY;
            if (dx * dx + dy * dy < ar * ar) {
                hp = (std::min)(maxHp, hp + m->hp * 0.32f);
                delete m;
                it = monsters.erase(it);
            } else ++it;
        }
    }

    void renderPools(float gt) const {
        for (auto& p : pools) {
            if (p.life <= 0.0f) continue;
            float t = p.life / p.maxLife;
            float pulse = 0.5f + 0.5f * sinf(gt * 11.0f + p.x * 0.02f);
            float alpha = t * pulse * 0.5f;
            drawCircle(p.x, p.y, p.radius, 0.22f, 0.35f, 0.95f, alpha);
            drawCircle(p.x, p.y, p.radius * 0.55f, 0.45f, 0.55f, 1.0f, alpha * 0.65f);
            drawRect(p.x - 3.0f, p.y - 2.0f, 6.0f, 4.0f, 0.85f, 0.9f, 1.0f, alpha * 0.35f);
        }
    }

    void renderBody(float gt) const {
        float r = BODY_SIZE * sizeScale;
        float pulse = 0.9f + 0.1f * sinf(gt * 2.8f + worldY * 0.01f);
        float pr = r * pulse;
        float br = color.r, bg = color.g, bb = color.b;
        if (phase3) { br *= 1.05f; bg *= 0.85f; }
        drawCircle(worldX, worldY, pr * 1.12f, br * 0.35f, bg * 0.35f, bb * 0.55f, 0.35f);
        drawCircle(worldX, worldY, pr, br, bg, bb, 0.88f);
        drawCircle(worldX, worldY, pr * 0.55f, 0.75f, 0.85f, 1.0f, 0.55f);
        float wob = sinf(gt * 4.2f) * pr * 0.12f;
        drawRect(worldX - pr * 0.28f + wob, worldY - pr * 0.12f,
                 pr * 0.56f, pr * 0.24f, 0.92f, 0.95f, 1.0f, 0.45f);
        if (phase3) {
            float ar = pr * (1.4f + 0.15f * sinf(gt * 6.0f));
            drawCircle(worldX, worldY, ar, 0.55f, 0.35f, 1.0f, 0.08f);
        }
    }

    void onPhase2Burst(std::vector<Monster*>& outSummons) {
        if (!p2BurstReady_) return;
        p2BurstReady_ = false;
        allocRing(outSummons, ALLOC_COUNT + 1);
        for (int i = 0; i < 5; i++)
            spawnPool(worldX + (float)(rand() % 100 - 50),
                      worldY + (float)(rand() % 100 - 50), 1.1f);
    }

private:
    void enterPhase2() {
        phase2 = true;
        sizeScale = 1.12f;
        p2BurstReady_ = true;
    }

    void enterPhase3() {
        phase3 = true;
        sizeScale = 1.18f;
    }

    void tickDrift(float px, float py, float dt, float spd) {
        wanderTimer += dt;
        if (wanderTimer >= 3.0f) {
            pickNewTarget();
            wanderTimer = 0.0f;
        }
        float tx = px * 0.62f + targetX * 0.38f;
        float ty = py * 0.62f + targetY * 0.38f;
        float dx = tx - worldX, dy = ty - worldY;
        float d = sqrtf(dx * dx + dy * dy) + 1e-3f;
        worldX += dx / d * spd * 72.0f * dt;
        worldY += dy / d * spd * 72.0f * dt;
        clampToScreen();
    }

    void clampToScreen() {
        float m = BODY_SIZE * sizeScale + 8.0f;
        if (worldX < m) worldX = m;
        if (worldX > (float)screenW - m) worldX = (float)screenW - m;
        if (worldY < m) worldY = m;
        if (worldY > (float)screenH - m) worldY = (float)screenH - m;
    }

    void tickTrail(float dt, float p2, float p3) {
        trailTimer += dt;
        float interval = leakNode ? 0.55f : (0.42f - p2 * 0.06f - p3 * 0.04f);
        if (trailTimer >= interval) {
            trailTimer = 0.0f;
            spawnPool(worldX, worldY, leakNode ? 0.75f : (0.85f + p2 * 0.1f));
        }
    }

    int poolCap() const {
        if (phase3) return leakNode ? 10 : 16;
        if (phase2) return leakNode ? 8 : 13;
        return leakNode ? 6 : 9;
    }

    void spawnPool(float x, float y, float scale) {
        if ((int)pools.size() >= poolCap()) {
            auto it = std::min_element(pools.begin(), pools.end(),
                [](const LeakPool& a, const LeakPool& b) { return a.life < b.life; });
            if (it != pools.end()) *it = LeakPool{};
        }
        LeakPool p;
        p.x = x; p.y = y;
        p.radius = (20.0f + (float)(rand() % 14)) * scale * (0.85f + sizeScale * 0.12f);
        p.maxLife = p.life = phase3 ? 4.2f : (phase2 ? 3.5f : 2.8f);
        p.grow = (8.0f + (float)(rand() % 6)) * scale * (1.0f + (phase2 ? 0.25f : 0.0f) + (phase3 ? 0.35f : 0.0f));
        pools.push_back(p);
    }

    void tickPools(float px, float py, float dt, float& playerHP) {
        for (auto& p : pools) {
            if (p.life <= 0.0f) continue;
            p.life -= dt;
            p.radius += p.grow * dt;
            float dx = px - p.x, dy = py - p.y;
            float hitR = p.radius + 6.0f;
            if (dx * dx + dy * dy < hitR * hitR) {
                float rate = phase3 ? 11.0f : (phase2 ? 8.5f : 6.0f);
                playerHP -= rate * dt * (p.radius / 40.0f);
            }
        }
        pools.erase(std::remove_if(pools.begin(), pools.end(),
            [](const LeakPool& p) { return p.life <= 0.0f; }), pools.end());
    }

    void allocRing(std::vector<Monster*>& out, int count) {
        float hpMul = phase3 ? 2.2f : (phase2 ? 1.85f : 1.55f);
        float spdMul = phase3 ? 0.95f : (phase2 ? 0.82f : 0.72f);
        for (int i = 0; i < count; i++) {
            float ang = (float)i / (float)count * 6.2831853f;
            float sx = worldX + cosf(ang) * ALLOC_RING_R;
            float sy = worldY + sinf(ang) * ALLOC_RING_R;
            out.push_back(new Monster(sx, sy, hpMul, spdMul, true));
        }
    }
};
