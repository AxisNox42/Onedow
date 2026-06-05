#pragma once
#include <vector>
#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#include "Bullet.h"

// ─────────────────────────────────────────────────────────────
// SPAM.dll — 탄막(불릿헬) 보스
//   느린 리사주 궤도로 떠다니며 회전 나선탄(여러 팔)을 끊임없이 뿌리고,
//   주기적으로 사방 방사 버스트를 쏜다. 탄속이 느려 피할 수 있게 —
//   화면을 채우는 "팝업 스팸" 압박형. (다른 보스들과 메커니즘 겹치지 않음)
// ─────────────────────────────────────────────────────────────
class SpamBoss {
public:
    float worldX, worldY, baseX, baseY;
    float hp, maxHp;
    bool  alive = true;
    bool  exploded = false;
    int   screenW, screenH;

    float spiralAngle = 0.0f;
    float fireTimer   = 0.0f;
    float burstTimer  = 0.0f;
    float driftT      = 0.0f;

    static constexpr float BODY        = 46.0f;
    static constexpr float SPIN        = 1.7f;     // 나선 회전 (rad/s)
    static constexpr int   ARMS        = 4;        // 나선 팔 개수
    static constexpr float FIRE_INT    = 0.06f;    // 나선탄 발사 간격
    static constexpr float BSPEED      = 300.0f;   // 느린 나선탄 (피할 수 있게)
    static constexpr float BURST_INT   = 4.5f;     // 방사 버스트 주기
    static constexpr int   BURST_N     = 26;       // 버스트 탄 수
    static constexpr float BURST_SPEED = 430.0f;

    SpamBoss(int sw, int sh, float hpInit) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        baseX = sw * 0.5f; baseY = sh * 0.30f;
        worldX = baseX; worldY = baseY;
    }

    void fireDir(std::vector<Bullet>& b, float dx, float dy, float sp, glm::vec3 col) {
        Bullet bb(worldX, worldY, worldX + dx * 100.0f, worldY + dy * 100.0f);
        bb.isEnemy = true; bb.speed = sp; bb.color = col;
        b.push_back(bb);
    }

    void Update(float px, float py, float dt, float& playerHP, std::vector<Bullet>& bullets) {
        if (!alive) return;
        // 느린 리사주 궤도로 떠다님
        driftT += dt;
        worldX = baseX + cosf(driftT * 0.5f) * (screenW * 0.13f);
        worldY = baseY + sinf(driftT * 0.8f) * (screenH * 0.10f);
        // 본체 접촉 데미지
        float dx = px - worldX, dy = py - worldY;
        if (dx*dx + dy*dy < BODY * BODY) playerHP -= 14.0f * dt;
        // 회전 나선탄 (여러 팔)
        spiralAngle += SPIN * dt;
        fireTimer += dt;
        if (fireTimer >= FIRE_INT) {
            fireTimer -= FIRE_INT;
            for (int a = 0; a < ARMS; a++) {
                float ang = spiralAngle + (float)a * (6.2831853f / (float)ARMS);
                fireDir(bullets, cosf(ang), sinf(ang), BSPEED, glm::vec3(1.0f, 0.4f, 0.85f));
            }
        }
        // 주기적 방사 버스트 (사방)
        burstTimer += dt;
        if (burstTimer >= BURST_INT) {
            burstTimer = 0.0f;
            for (int i = 0; i < BURST_N; i++) {
                float ang = (float)i / (float)BURST_N * 6.2831853f;
                fireDir(bullets, cosf(ang), sinf(ang), BURST_SPEED, glm::vec3(1.0f, 0.8f, 0.3f));
            }
        }
    }
};
