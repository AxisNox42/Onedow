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
    float spiral2Angle= 0.0f;   // 페이즈2: 역회전 두 번째 나선
    float fireTimer   = 0.0f;
    float burstTimer  = 0.0f;
    float driftT      = 0.0f;
    bool  phase2      = false;  // HP 50% 이하 — 이중 나선 + 조준 버스트

    static constexpr float BODY        = 46.0f;
    static constexpr float SPIN        = 1.3f;     // 나선 회전 (rad/s) — 너프: 1.7
    static constexpr int   ARMS        = 3;        // 나선 팔 개수 — 너프: 4
    static constexpr float FIRE_INT    = 0.11f;    // 나선탄 발사 간격 — 너프: 0.06 (발사율 절반)
    static constexpr float BSPEED      = 260.0f;   // 느린 나선탄 — 너프: 300
    static constexpr float BURST_INT   = 6.5f;     // 방사 버스트 주기 — 너프: 4.5 (덜 자주)
    static constexpr int   BURST_N     = 18;       // 버스트 탄 수 — 너프: 26
    static constexpr float BURST_SPEED = 380.0f;   // 너프: 430

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
        // 페이즈2 진입 (HP 50% 이하) — 이중 나선 + 조준 버스트
        if (!phase2 && hp <= maxHp * 0.5f) phase2 = true;
        // 느린 리사주 궤도로 떠다님
        driftT += dt;
        worldX = baseX + cosf(driftT * 0.5f) * (screenW * 0.13f);
        worldY = baseY + sinf(driftT * 0.8f) * (screenH * 0.10f);
        // 본체 접촉 데미지 (너프: 14 → 10)
        float dx = px - worldX, dy = py - worldY;
        if (dx*dx + dy*dy < BODY * BODY) playerHP -= 10.0f * dt;
        // 회전 나선탄 (여러 팔). 페이즈2: 역회전 두 번째 나선 추가
        spiralAngle  += SPIN * dt;
        spiral2Angle -= SPIN * dt;   // 반대 방향
        fireTimer += dt;
        if (fireTimer >= FIRE_INT) {
            fireTimer -= FIRE_INT;
            for (int a = 0; a < ARMS; a++) {
                float ang = spiralAngle + (float)a * (6.2831853f / (float)ARMS);
                fireDir(bullets, cosf(ang), sinf(ang), BSPEED, glm::vec3(1.0f, 0.4f, 0.85f));
            }
            if (phase2) {
                for (int a = 0; a < ARMS; a++) {
                    float ang = spiral2Angle + (float)a * (6.2831853f / (float)ARMS);
                    fireDir(bullets, cosf(ang), sinf(ang), BSPEED * 0.85f,
                            glm::vec3(0.6f, 0.85f, 1.0f));   // 시안 — 역나선 구분
                }
            }
        }
        // 주기적 버스트 — 페이즈1: 방사형 / 페이즈2: 플레이어 조준 콘 + 더 자주
        burstTimer += dt;
        float burstInt = phase2 ? BURST_INT * 0.55f : BURST_INT;
        if (burstTimer >= burstInt) {
            burstTimer = 0.0f;
            if (!phase2) {
                for (int i = 0; i < BURST_N; i++) {
                    float ang = (float)i / (float)BURST_N * 6.2831853f;
                    fireDir(bullets, cosf(ang), sinf(ang), BURST_SPEED, glm::vec3(1.0f, 0.8f, 0.3f));
                }
            } else {
                // 플레이어 방향 ±조준 콘 (피하되 압박)
                float base = atan2f(py - worldY, px - worldX);
                int   n    = 11;
                float half = 0.55f;   // 약 ±31°
                for (int i = 0; i < n; i++) {
                    float ratio = (n > 1) ? (float)i / (float)(n - 1) : 0.5f;
                    float ang   = base + (ratio - 0.5f) * 2.0f * half;
                    fireDir(bullets, cosf(ang), sinf(ang), BURST_SPEED, glm::vec3(1.0f, 0.6f, 0.25f));
                }
            }
        }
    }
};
