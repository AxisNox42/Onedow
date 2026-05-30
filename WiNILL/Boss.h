#pragma once
#include <glm/glm.hpp>
#include <cmath>
#include <cstdlib>
#include <vector>
#include "Monster.h"

// ─────────────────────────────────────────────────────────────
// 슬라임 (Slime) — 첫 보스
//   - FakeWindow 700×700 보유
//   - 움직임: 랜덤 wander
//   - 스킬:
//     A. 돌진: 1.5s 빨간 텔레그래프 → 화면 끝까지 대시
//              벽에 튕길 때마다 강화 잡몹 3마리 소환 (카타리나 궁극기 스타일)
//              최대 3회 바운스 후 IDLE 복귀
//     B. 소환: 4초마다 강화 잡몹 5마리 spawn (HP×2, 속도×0.8)
// ─────────────────────────────────────────────────────────────
class Boss {
public:
    float worldX, worldY;
    float hp    = 7000.0f;
    float maxHp = 7000.0f;
    bool  alive = true;
    bool  exploded = false;

    static constexpr float WIN_W = 700.0f;
    static constexpr float WIN_H = 700.0f;
    static constexpr float BODY_SIZE = 50.0f;  // 플레이어(25) 2배

    glm::vec3 color = glm::vec3(0.9f, 0.85f, 0.95f);

    // ── Wander ──
    float targetX, targetY;
    float wanderTimer  = 0.0f;
    static constexpr float WANDER_INTERVAL = 3.0f;
    static constexpr float WANDER_SPEED    = 1.4f;

    // ── 스킬 상태 머신 ──
    enum class Skill { IDLE, TELEGRAPH, CHARGING };
    Skill skill = Skill::IDLE;
    float skillTimer    = 0.0f;
    float idleCooldown  = 7.0f;
    static constexpr float TELEGRAPH_TIME = 1.5f;
    float chargeDirX = 0.0f, chargeDirY = 0.0f;
    static constexpr float CHARGE_SPEED   = 1400.0f;
    static constexpr float CHARGE_DAMAGE  = 35.0f;
    static constexpr float CHARGE_RADIUS  = 50.0f;

    // ── 바운스 ──
    int   bounceCount = 0;
    static constexpr int   BOUNCE_MAX          = 3;  // 3회 바운스 후 IDLE 복귀
    static constexpr int   IMPACT_SUMMON_COUNT = 3;  // 바운스 시 소환 수

    // ── 소환 ──
    float summonTimer = 0.0f;
    static constexpr float SUMMON_INTERVAL = 4.0f;
    static constexpr int   SUMMON_COUNT    = 5;

    int screenW, screenH;

    Boss(float sx, float sy, int sw, int sh, float maxHpInit = 7000.0f)
        : worldX(sx), worldY(sy)
        , targetX(sx), targetY(sy)
        , screenW(sw), screenH(sh)
    {
        hp    = maxHpInit;
        maxHp = maxHpInit;
        pickNewTarget();
    }

    void pickNewTarget() {
        float margin = 120.0f;
        float rw = (float)(screenW - 2 * (int)margin);
        float rh = (float)(screenH - 2 * (int)margin);
        if (rw < 1) rw = 1; if (rh < 1) rh = 1;
        targetX = margin + (float)(rand() % (int)rw);
        targetY = margin + (float)(rand() % (int)rh);
    }

    // dt 동안 보스 갱신.
    // outSummons : 이번 프레임에 소환할 강화 잡몹들이 push 됨
    void Update(float playerCX, float playerCY, float dt,
                float& playerHP,
                std::vector<Monster*>& outSummons)
    {
        if (!alive) return;

        // ── 소환 타이머 (스킬과 독립적으로) ──
        summonTimer += dt;
        if (summonTimer >= SUMMON_INTERVAL) {
            summonTimer = 0.0f;
            for (int i = 0; i < SUMMON_COUNT; i++) {
                float ang = (float)i / SUMMON_COUNT * 6.2831853f;
                float r   = 70.0f;
                float sx  = worldX + cosf(ang) * r;
                float sy  = worldY + sinf(ang) * r;
                outSummons.push_back(new Monster(sx, sy, 2.0f, 0.8f, /*summoned=*/true));
            }
        }

        // ── 스킬 상태 머신 ──
        skillTimer += dt;
        switch (skill) {
        case Skill::IDLE:
            // wander
            wanderTimer += dt;
            if (wanderTimer >= WANDER_INTERVAL) {
                pickNewTarget();
                wanderTimer = 0.0f;
            }
            worldX += (targetX - worldX) * WANDER_SPEED * dt;
            worldY += (targetY - worldY) * WANDER_SPEED * dt;

            if (skillTimer >= idleCooldown) {
                // 돌진 텔레그래프 — 플레이어 방향
                float dx = playerCX - worldX;
                float dy = playerCY - worldY;
                float l  = std::sqrt(dx*dx + dy*dy);
                if (l < 1e-3f) { dx = 1; dy = 0; l = 1; }
                chargeDirX  = dx / l;
                chargeDirY  = dy / l;
                skill       = Skill::TELEGRAPH;
                skillTimer  = 0.0f;
                bounceCount = 0;
            }
            break;

        case Skill::TELEGRAPH:
            // 정지 + 빨간 범위 표시. 종료 후 대시 시작
            if (skillTimer >= TELEGRAPH_TIME) {
                skill      = Skill::CHARGING;
                skillTimer = 0.0f;
            }
            break;

        case Skill::CHARGING:
        {
            worldX += chargeDirX * CHARGE_SPEED * dt;
            worldY += chargeDirY * CHARGE_SPEED * dt;

            // 플레이어 접촉 데미지
            float dx = playerCX - worldX;
            float dy = playerCY - worldY;
            if (dx*dx + dy*dy < CHARGE_RADIUS * CHARGE_RADIUS) {
                playerHP -= CHARGE_DAMAGE * dt * 4.0f;
            }

            // ── 벽 바운스 + 충돌 지점 강화 잡몹 소환 ──
            bool bounced = false;
            if (worldX < 0.0f) {
                worldX = 0.0f;
                chargeDirX = std::fabsf(chargeDirX);
                bounced = true;
            } else if (worldX > (float)screenW) {
                worldX = (float)screenW;
                chargeDirX = -std::fabsf(chargeDirX);
                bounced = true;
            }
            if (worldY < 0.0f) {
                worldY = 0.0f;
                chargeDirY = std::fabsf(chargeDirY);
                bounced = true;
            } else if (worldY > (float)screenH) {
                worldY = (float)screenH;
                chargeDirY = -std::fabsf(chargeDirY);
                bounced = true;
            }

            if (bounced) {
                ++bounceCount;
                // 카타리나 궁극기 스타일: 충돌 지점에서 방사형 강화 잡몹 소환
                for (int i = 0; i < IMPACT_SUMMON_COUNT; i++) {
                    float ang = (float)i / (float)IMPACT_SUMMON_COUNT * 6.2831853f;
                    float r   = 45.0f;
                    outSummons.push_back(new Monster(
                        worldX + cosf(ang) * r,
                        worldY + sinf(ang) * r,
                        2.5f, 1.3f, /*summoned=*/true));
                }
                if (bounceCount >= BOUNCE_MAX) {
                    // 바운스 MAX 도달 → IDLE 복귀
                    skill        = Skill::IDLE;
                    skillTimer   = 0.0f;
                    idleCooldown = 5.0f;
                    bounceCount  = 0;
                    pickNewTarget();
                }
            }
            break;
        }
        }
    }
};
