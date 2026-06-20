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
//   본체: 큰 눈알이 굴러다니며 이동할수록 체력↓·크기↓ + 누수 궤적
//   누수 바닥이 자라면 leak.node 생성 (소환 잡몹 없음)
//   P2/P3: 누수·노드 생성 가속
// ─────────────────────────────────────────────────────────────

struct LeakNodeSpawn {
    float x = 0, y = 0;
    float hp = 800.0f;
    float scale = 0.45f;
    int   gen = 1;
};

struct LeakPool {
    float x = 0, y = 0;
    float life = 0.0f;
    float maxLife = 3.0f;
    float radius = 26.0f;
    float grow = 10.0f;
    float age = 0.0f;
    bool  nodeSpawned = false;
};

class Boss {
public:
    float worldX = 0, worldY = 0;
    float hp = 7000.0f, maxHp = 7000.0f;
    bool  alive = true;
    bool  exploded = false;

    static inline float WIN_W = 700.0f;
    static inline float WIN_H = 700.0f;
    static inline float BODY_SIZE = 68.0f;

    glm::vec3 color = glm::vec3(0.92f, 0.94f, 0.98f);

    float sizeScale = 1.85f;
    int   splitGen = 0;
    bool  leakNode = false;

    bool  phase2 = false;
    bool  phase3 = false;

    float rollAngle = 0.0f;

    std::vector<LeakPool> pools;

    float targetX = 0, targetY = 0;
    float wanderTimer = 0.0f;
    float trailTimer = 0.0f;

    int screenW = 0, screenH = 0;

    static constexpr float CONTACT_RADIUS = 58.0f;
    static constexpr float CONTACT_DPS    = 18.0f;

    static constexpr float MOVE_HP_DRAIN  = 0.28f;
    static constexpr float MOVE_SHRINK    = 0.00062f;
    static constexpr float MIN_BODY_SCALE = 0.72f;
    static constexpr float MAX_BODY_SCALE = 1.85f;

    static constexpr float NODE_SPAWN_RADIUS = 42.0f;
    static constexpr float NODE_SPAWN_AGE    = 1.35f;
    static constexpr int   NODE_SPAWN_MAXGEN = 2;

    Boss(float sx, float sy, int sw, int sh, float maxHpInit = 7000.0f)
        : worldX(sx), worldY(sy), targetX(sx), targetY(sy)
        , screenW(sw), screenH(sh)
    {
        hp = maxHp = maxHpInit;
    }

    const wchar_t* stateTag() const {
        if (phase3) return L"GC FAIL";
        if (phase2) return L"HEAP+";
        return leakNode ? L"NODE" : L"ROLL";
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
                std::vector<Monster*>& /*outSummons*/,
                std::vector<LeakNodeSpawn>& outNodes)
    {
        if (!alive) return;

        if (!phase2 && !leakNode && hp <= maxHp * 0.5f) phase2 = true;
        if (!phase3 && !leakNode && hp <= maxHp * 0.25f) phase3 = true;

        float p2 = phase2 ? 1.0f : 0.0f;
        float p3 = phase3 ? 1.0f : 0.0f;
        float driftSpd = (1.2f + p2 * 0.45f + p3 * 0.35f) * (leakNode ? 1.05f : 1.0f);

        tickPools(playerCX, playerCY, dt, playerHP, outNodes);
        tickDrift(playerCX, playerCY, dt, driftSpd);
        tickTrail(dt, p2, p3);

        float cr = CONTACT_RADIUS * sizeScale;
        float dx = playerCX - worldX, dy = playerCY - worldY;
        if (dx * dx + dy * dy < cr * cr)
            playerHP -= CONTACT_DPS * dt * (1.0f + p2 * 0.3f + p3 * 0.45f);

        if (hp <= 0.0f) alive = false;
    }

    void burstPools(int count = 5) {
        for (int i = 0; i < count; i++)
            spawnPool(worldX + (float)(rand() % 120 - 60),
                      worldY + (float)(rand() % 120 - 60),
                      leakNode ? 0.85f : 1.15f);
    }

    void renderPools(float gt) const {
        for (auto& p : pools) {
            if (p.life <= 0.0f) continue;
            float t = p.life / p.maxLife;
            float pulse = 0.5f + 0.5f * sinf(gt * 11.0f + p.x * 0.02f);
            float alpha = t * pulse * 0.52f;
            float ready = (!p.nodeSpawned && p.radius >= NODE_SPAWN_RADIUS &&
                           p.age >= NODE_SPAWN_AGE) ? 1.15f : 1.0f;
            drawCircle(p.x, p.y, p.radius, 0.18f, 0.28f, 0.92f, alpha);
            drawCircle(p.x, p.y, p.radius * 0.5f * ready, 0.42f, 0.52f, 1.0f, alpha * 0.7f);
            if (ready > 1.0f) {
                float blink = 0.35f + 0.25f * sinf(gt * 14.0f + p.y * 0.03f);
                drawCircle(p.x, p.y, 10.0f, 0.85f, 0.9f, 1.0f, blink * t);
            }
        }
    }

    void renderBody(float gt) const {
        float r = BODY_SIZE * sizeScale;
        float wobble = sinf(gt * 3.5f) * r * 0.03f;
        float cx = worldX + wobble;
        float cy = worldY;

        float scleraR = r * 1.05f;
        drawCircle(cx, cy, scleraR * 1.08f, 0.35f, 0.42f, 0.75f, 0.22f);
        drawCircle(cx, cy, scleraR, 0.94f, 0.96f, 0.99f, 0.96f);

        float irisR = r * (leakNode ? 0.38f : 0.44f);
        float irisDist = r * (leakNode ? 0.14f : 0.2f);
        float ix = cx + cosf(rollAngle) * irisDist;
        float iy = cy + sinf(rollAngle) * irisDist;
        drawCircle(ix, iy, irisR, 0.28f, 0.52f, 0.95f, 1.0f);
        drawCircle(ix + cosf(rollAngle + 0.4f) * irisR * 0.35f,
                   iy + sinf(rollAngle + 0.4f) * irisR * 0.35f,
                   irisR * 0.42f, 0.04f, 0.06f, 0.12f, 0.95f);

        float px = ix + cosf(rollAngle) * irisR * 0.28f;
        float py = iy + sinf(rollAngle) * irisR * 0.28f;
        drawCircle(px, py, irisR * 0.22f, 0.02f, 0.02f, 0.05f, 1.0f);
        drawCircle(px - irisR * 0.08f, py - irisR * 0.08f, irisR * 0.07f,
                   0.95f, 0.98f, 1.0f, 0.85f);

        if (!leakNode) {
            for (int i = 0; i < 5; i++) {
                float a = rollAngle * 0.5f + (float)i * 1.257f;
                float vx = cx + cosf(a) * scleraR * 0.82f;
                float vy = cy + sinf(a) * scleraR * 0.82f;
                drawCircle(vx, vy, 2.2f, 0.85f, 0.15f, 0.2f, 0.55f);
            }
        }
        if (phase3 && !leakNode) {
            float ar = scleraR * (1.35f + 0.12f * sinf(gt * 5.0f));
            drawCircle(cx, cy, ar, 0.5f, 0.35f, 1.0f, 0.07f);
        }
    }

private:
    void tickDrift(float px, float py, float dt, float spd) {
        wanderTimer += dt;
        if (wanderTimer >= 3.0f) {
            pickNewTarget();
            wanderTimer = 0.0f;
        }
        float tx = px * 0.58f + targetX * 0.42f;
        float ty = py * 0.58f + targetY * 0.42f;
        float dx = tx - worldX, dy = ty - worldY;
        float d = sqrtf(dx * dx + dy * dy) + 1e-3f;
        float step = spd * 68.0f * dt;
        float prevX = worldX, prevY = worldY;
        worldX += dx / d * step;
        worldY += dy / d * step;
        clampToScreen();

        float moved = sqrtf((worldX - prevX) * (worldX - prevX) +
                            (worldY - prevY) * (worldY - prevY));
        if (moved > 0.05f) {
            rollAngle += moved * 0.045f;
            if (!leakNode) {
                hp -= moved * MOVE_HP_DRAIN;
                sizeScale -= moved * MOVE_SHRINK;
                if (sizeScale < MIN_BODY_SCALE) sizeScale = MIN_BODY_SCALE;
            }
        }
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
        float interval = leakNode ? 0.62f : (0.48f - p2 * 0.05f - p3 * 0.04f);
        if (trailTimer >= interval) {
            trailTimer = 0.0f;
            spawnPool(worldX, worldY, leakNode ? 0.72f : (0.9f + p2 * 0.08f));
        }
    }

    int poolCap() const {
        if (phase3) return leakNode ? 10 : 18;
        if (phase2) return leakNode ? 8 : 14;
        return leakNode ? 6 : 11;
    }

    void spawnPool(float x, float y, float scale) {
        if ((int)pools.size() >= poolCap()) {
            auto it = std::min_element(pools.begin(), pools.end(),
                [](const LeakPool& a, const LeakPool& b) { return a.life < b.life; });
            if (it != pools.end()) *it = LeakPool{};
        }
        LeakPool p;
        p.x = x; p.y = y;
        p.radius = (22.0f + (float)(rand() % 16)) * scale * (0.8f + sizeScale * 0.08f);
        p.maxLife = p.life = phase3 ? 4.5f : (phase2 ? 3.8f : 3.0f);
        p.grow = (9.0f + (float)(rand() % 7)) * scale *
                 (1.0f + (phase2 ? 0.22f : 0.0f) + (phase3 ? 0.32f : 0.0f));
        pools.push_back(p);
    }

    void trySpawnNodeFromPool(LeakPool& p, std::vector<LeakNodeSpawn>& out) {
        if (p.nodeSpawned) return;
        if (p.radius < NODE_SPAWN_RADIUS) return;
        if (p.age < NODE_SPAWN_AGE) return;
        int gen = leakNode ? (splitGen + 1) : 1;
        if (gen > NODE_SPAWN_MAXGEN) return;

        p.nodeSpawned = true;
        LeakNodeSpawn ns;
        ns.x = p.x;
        ns.y = p.y;
        ns.gen = gen;
        ns.scale = 0.36f + p.radius / 130.0f;
        if (ns.scale > 0.52f) ns.scale = 0.52f;
        ns.hp = maxHp * (0.11f + 0.03f * (float)gen) * ns.scale;
        if (ns.hp < 400.0f) ns.hp = 400.0f;
        out.push_back(ns);
    }

    void tickPools(float px, float py, float dt, float& playerHP,
                   std::vector<LeakNodeSpawn>& outNodes) {
        for (auto& p : pools) {
            if (p.life <= 0.0f) continue;
            p.life -= dt;
            p.age += dt;
            p.radius += p.grow * dt;
            trySpawnNodeFromPool(p, outNodes);

            float dx = px - p.x, dy = py - p.y;
            float hitR = p.radius + 6.0f;
            if (dx * dx + dy * dy < hitR * hitR) {
                float rate = phase3 ? 10.0f : (phase2 ? 7.5f : 5.5f);
                playerHP -= rate * dt * (p.radius / 42.0f);
            }
        }
        pools.erase(std::remove_if(pools.begin(), pools.end(),
            [](const LeakPool& p) { return p.life <= 0.0f; }), pools.end());
    }
};
