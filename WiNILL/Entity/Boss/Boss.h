#pragma once
#include <glm/glm.hpp>
#include <cmath>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include "Monster.h"
#include "DrawPrim.h"

// ─────────────────────────────────────────────────────────────
// SLIME.worm — 메모리 누수 프로세스 보스 (v2)
//
//   P1: 플레이어 추격 + 돌진(벽 튕김) + 산성 궤적 + 주기적 FORK 소환
//   P2 50%: 돌진↑ · 소환↑ · 분열체(main 스폰)
//   P3 25%: 연속 돌진 · 산성↑ · OVERFLOW 소환
// ─────────────────────────────────────────────────────────────

struct SlimeAcid {
    float x = 0, y = 0;
    float life = 0.0f;
    float maxLife = 1.4f;
    float radius = 28.0f;
};

class Boss {
public:
    float worldX = 0, worldY = 0;
    float hp = 7000.0f, maxHp = 7000.0f;
    bool  alive = true;
    bool  exploded = false;

    static inline float WIN_W = 700.0f;
    static inline float WIN_H = 700.0f;
    static inline float BODY_SIZE = 50.0f;

    glm::vec3 color = glm::vec3(0.9f, 0.85f, 0.95f);

    float sizeScale = 1.0f;
    int   splitGen = 0;
    bool  chargeOnly = false;

    bool  phase2 = false;
    bool  phase3 = false;

    std::vector<SlimeAcid> acids;

    float targetX = 0, targetY = 0;
    float wanderTimer = 0.0f;

    enum class Skill { IDLE, TELEGRAPH, CHARGING, RECOVER };
    Skill skill = Skill::IDLE;
    float skillTimer = 0.0f;
    float idleCooldown = 4.2f;

    float chargeDirX = 1.0f, chargeDirY = 0.0f;
    int   bounceCount = 0;
    int   chainRushLeft_ = 0;

    float summonTimer = 0.0f;
    bool  summonPending = false;
    bool  overflowBurst_ = false;

    int screenW = 0, screenH = 0;

    static constexpr float TELEGRAPH_TIME = 0.95f;
    static constexpr float CHARGE_SPEED   = 1780.0f;
    static constexpr float CHARGE_DAMAGE  = 58.0f;
    static constexpr float CHARGE_RADIUS  = 58.0f;
    static constexpr float RECOVER_TIME   = 0.55f;

    static constexpr int   BOUNCE_MAX          = 5;
    static constexpr int   IMPACT_SUMMON_COUNT = 4;

    static constexpr float SUMMON_INTERVAL = 2.6f;
    static constexpr float SUMMON_WARN     = 0.65f;
    static constexpr int   SUMMON_COUNT    = 6;
    static constexpr float SUMMON_RING_R   = 78.0f;

    Boss(float sx, float sy, int sw, int sh, float maxHpInit = 7000.0f)
        : worldX(sx), worldY(sy), targetX(sx), targetY(sy)
        , screenW(sw), screenH(sh)
    {
        hp = maxHp = maxHpInit;
    }

    const wchar_t* stateTag() const {
        if (phase3) return L"OVERFLOW";
        if (phase2) return L"FORK";
        switch (skill) {
        case Skill::TELEGRAPH: return L"RUSH◉";
        case Skill::CHARGING:  return L"RUSH";
        case Skill::RECOVER:   return L"COOL";
        default: return summonPending ? L"FORK◉" : L"HUNT";
        }
    }

    void pickNewTarget() {
        float margin = 100.0f;
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

        if (!phase2 && !chargeOnly && hp <= maxHp * 0.5f) enterPhase2();
        if (!phase3 && !chargeOnly && hp <= maxHp * 0.25f) enterPhase3();

        float p2 = phase2 ? 1.0f : 0.0f;
        float p3 = phase3 ? 1.0f : 0.0f;
        float huntSpd = (2.2f + p2 * 0.9f + p3 * 0.6f) * (chargeOnly ? 1.15f : 1.0f);
        float sumMul = 1.0f + p2 * 0.55f + p3 * 0.45f;

        tickAcid(playerCX, playerCY, dt, playerHP);

        if (!chargeOnly) {
            summonTimer += dt * sumMul;
            if (!summonPending && summonTimer >= SUMMON_INTERVAL - SUMMON_WARN)
                summonPending = true;
            if (summonTimer >= SUMMON_INTERVAL) {
                summonTimer = 0.0f;
                summonPending = false;
                forkRing(outSummons, phase3 ? SUMMON_COUNT + 2 : SUMMON_COUNT);
            }
        }

        skillTimer += dt;
        switch (skill) {
        case Skill::IDLE:
            tickHunt(playerCX, playerCY, dt, huntSpd);
            if (skillTimer >= idleCooldown) beginTelegraph(playerCX, playerCY);
            break;

        case Skill::TELEGRAPH:
            if (skillTimer >= TELEGRAPH_TIME) {
                skill = Skill::CHARGING;
                skillTimer = 0.0f;
            }
            break;

        case Skill::CHARGING:
            tickCharge(playerCX, playerCY, dt, playerHP, outSummons);
            break;

        case Skill::RECOVER:
            tickHunt(playerCX, playerCY, dt, huntSpd * 0.65f);
            if (skillTimer >= RECOVER_TIME) {
                if (chainRushLeft_ > 0) {
                    chainRushLeft_--;
                    beginTelegraph(playerCX, playerCY);
                } else {
                    skill = Skill::IDLE;
                    skillTimer = 0.0f;
                    idleCooldown = phase3 ? 1.6f : (phase2 ? 2.2f : 3.4f);
                }
            }
            break;
        }
    }

    void renderAcid(float gt) const {
        for (auto& a : acids) {
            if (a.life <= 0.0f) continue;
            float t = a.life / a.maxLife;
            float pulse = 0.55f + 0.45f * sinf(gt * 14.0f + a.x * 0.03f);
            float alpha = t * pulse * 0.55f;
            drawCircle(a.x, a.y, a.radius, 0.35f, 0.95f, 0.28f, alpha);
            drawCircle(a.x, a.y, a.radius * 0.55f, 0.55f, 1.0f, 0.45f, alpha * 0.75f);
        }
    }

private:
    void enterPhase2() {
        phase2 = true;
        idleCooldown = 2.4f;
        overflowBurst_ = true;
    }

    void enterPhase3() {
        phase3 = true;
        idleCooldown = 1.8f;
        chainRushLeft_ = 1;
    }

    void tickHunt(float px, float py, float dt, float spd) {
        wanderTimer += dt;
        if (wanderTimer >= 2.4f) {
            pickNewTarget();
            wanderTimer = 0.0f;
        }
        float tx = px * 0.78f + targetX * 0.22f;
        float ty = py * 0.78f + targetY * 0.22f;
        float dx = tx - worldX, dy = ty - worldY;
        float d = sqrtf(dx * dx + dy * dy) + 1e-3f;
        worldX += dx / d * spd * 85.0f * dt;
        worldY += dy / d * spd * 85.0f * dt;
    }

    void beginTelegraph(float px, float py) {
        float dx = px - worldX, dy = py - worldY;
        float l = sqrtf(dx * dx + dy * dy) + 1e-3f;
        chargeDirX = dx / l;
        chargeDirY = dy / l;
        skill = Skill::TELEGRAPH;
        skillTimer = 0.0f;
        bounceCount = 0;
    }

    void dropAcid(float x, float y) {
        if ((int)acids.size() >= acidCap()) {
            auto it = std::min_element(acids.begin(), acids.end(),
                [](const SlimeAcid& a, const SlimeAcid& b) { return a.life < b.life; });
            if (it != acids.end()) *it = SlimeAcid{};
        }
        SlimeAcid a;
        a.x = x; a.y = y;
        a.radius = (22.0f + (float)(rand() % 12)) * (0.85f + sizeScale * 0.15f);
        a.maxLife = a.life = phase3 ? 2.0f : (phase2 ? 1.7f : 1.4f);
        acids.push_back(a);
    }

    int acidCap() const {
        if (phase3) return 18;
        if (phase2) return 14;
        return chargeOnly ? 8 : 10;
    }

    void tickAcid(float px, float py, float dt, float& playerHP) {
        for (auto& a : acids) {
            if (a.life <= 0.0f) continue;
            a.life -= dt;
            float dx = px - a.x, dy = py - a.y;
            if (dx * dx + dy * dy < (a.radius + 8.0f) * (a.radius + 8.0f)) {
                float rate = phase3 ? 14.0f : (phase2 ? 10.0f : 7.0f);
                playerHP -= rate * dt;
            }
        }
        acids.erase(std::remove_if(acids.begin(), acids.end(),
            [](const SlimeAcid& a) { return a.life <= 0.0f; }), acids.end());
    }

    void forkRing(std::vector<Monster*>& out, int count) {
        float hpMul = phase3 ? 2.4f : (phase2 ? 2.0f : 1.7f);
        float spdMul = phase3 ? 1.05f : (phase2 ? 0.92f : 0.85f);
        for (int i = 0; i < count; i++) {
            float ang = (float)i / (float)count * 6.2831853f;
            float sx = worldX + cosf(ang) * SUMMON_RING_R;
            float sy = worldY + sinf(ang) * SUMMON_RING_R;
            out.push_back(new Monster(sx, sy, hpMul, spdMul, true));
        }
    }

    void tickCharge(float px, float py, float dt, float& playerHP,
                    std::vector<Monster*>& outSummons)
    {
        float spd = CHARGE_SPEED * (1.0f + (phase2 ? 0.12f : 0.0f) + (phase3 ? 0.18f : 0.0f));
        worldX += chargeDirX * spd * dt;
        worldY += chargeDirY * spd * dt;

        if (skillTimer > 0.04f && (int)(skillTimer * 20.0f) % 3 == 0)
            dropAcid(worldX, worldY);

        float dx = px - worldX, dy = py - worldY;
        if (dx * dx + dy * dy < CHARGE_RADIUS * CHARGE_RADIUS)
            playerHP -= CHARGE_DAMAGE * dt * (phase3 ? 4.8f : 4.2f);

        bool bounced = false;
        if (worldX < 0.0f) {
            worldX = 0.0f; chargeDirX = fabsf(chargeDirX); bounced = true;
        } else if (worldX > (float)screenW) {
            worldX = (float)screenW; chargeDirX = -fabsf(chargeDirX); bounced = true;
        }
        if (worldY < 0.0f) {
            worldY = 0.0f; chargeDirY = fabsf(chargeDirY); bounced = true;
        } else if (worldY > (float)screenH) {
            worldY = (float)screenH; chargeDirY = -fabsf(chargeDirY); bounced = true;
        }

        if (bounced) {
            ++bounceCount;
            dropAcid(worldX, worldY);
            if (!chargeOnly)
                forkRing(outSummons, phase3 ? IMPACT_SUMMON_COUNT + 1 : IMPACT_SUMMON_COUNT);
            if (phase3 && bounceCount == 2) chainRushLeft_ = 1;

            if (bounceCount >= BOUNCE_MAX) {
                skill = Skill::RECOVER;
                skillTimer = 0.0f;
                bounceCount = 0;
            }
        }
    }

public:
    // P2 진입 시 main에서 1회 호출 — 즉시 FORK 링
    void onPhase2Burst(std::vector<Monster*>& outSummons) {
        if (!overflowBurst_) return;
        overflowBurst_ = false;
        forkRing(outSummons, SUMMON_COUNT + 2);
        for (int i = 0; i < 4; i++)
            dropAcid(worldX + (float)(rand() % 80 - 40), worldY + (float)(rand() % 80 - 40));
    }
};
