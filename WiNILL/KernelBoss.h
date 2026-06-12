#pragma once
#include <vector>
#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#include "Bullet.h"

// ─────────────────────────────────────────────────────────────
// KERNEL.sys — 커널 프로세스 (고정형 · 순수 DPS 체크 보스)
//   랜덤 위치에 박힌 거대 코어. 움직이지 않는다.
//   패시브: 1초당 최대체력 1% 자동 감소(스스로 붕괴) → 빨리 딜 안 하면
//           소환물이 누적되어 화면을 채운다(시간 압박).
//   리듬: 팽창(EXPAND) ↔ 수축(CONTRACT) 을 번갈아 반복.
//     - EXPAND : 짧은 예고 후 사방으로 빠른 링 탄막 방출(밀어내기 느낌, 거리 강제).
//     - CONTRACT: 플레이어를 향해 느린 "프로세스" 탄 클러스터 소환(피하며 본체 딜).
//   2페이즈 없음 — 소환물 피하며 본체를 빨리 깎는 DPS 체크.
// ─────────────────────────────────────────────────────────────
class KernelBoss {
public:
    float worldX, worldY;
    float hp, maxHp;
    bool  alive = true;
    bool  exploded = false;
    int   screenW, screenH;

    // 상태 머신: 0=대기, 1=팽창 예고, 2=팽창 방출, 3=수축 소환
    int   state      = 0;
    float stateTimer = 0.0f;
    float pulse      = 0.0f;   // 시각 호흡 (렌더가 읽음)
    float drainAccum = 0.0f;   // 패시브 감소 누적(정수 손실 방지)

    static constexpr float BODY        = 92.0f;   // 거대 코어
    static constexpr float TELEGRAPH   = 0.7f;    // 팽창 예고 시간
    static constexpr float EXPAND_HOLD = 0.35f;   // 팽창 방출 직후 텀
    static constexpr float CONTRACT_T  = 1.1f;    // 수축(소환) 지속
    static constexpr float IDLE_T      = 1.3f;    // 사이클 사이 대기
    static constexpr int   RING_N      = 28;      // 팽창 링 탄 수
    static constexpr float RING_SPEED  = 460.0f;
    static constexpr int   SUMMON_N    = 5;       // 수축 1회 소환 클러스터 수
    static constexpr float SUMMON_SPEED= 190.0f;  // 느린 소환탄
    static constexpr float DRAIN_RATE  = 0.01f;   // 1초당 maxHp 1% 자가 감소

    KernelBoss(int sw, int sh, float hpInit) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.5f; worldY = sh * 0.45f;   // main 이 스폰 시 위치 덮어씀
    }

    void fireDir(std::vector<Bullet>& b, float dx, float dy, float sp, glm::vec3 col) {
        Bullet bb(worldX, worldY, worldX + dx * 100.0f, worldY + dy * 100.0f);
        bb.isEnemy = true; bb.speed = sp; bb.color = col;
        b.push_back(bb);
    }

    void Update(float px, float py, float dt, float& playerHP, std::vector<Bullet>& bullets) {
        if (!alive) return;
        pulse += dt;

        // 패시브 자가 붕괴 — 1초당 maxHp 1%
        drainAccum += maxHp * DRAIN_RATE * dt;
        if (drainAccum >= 1.0f) { hp -= drainAccum; drainAccum = 0.0f; if (hp <= 0.0f) { hp = 0.0f; alive = false; return; } }

        // 본체 접촉 데미지
        float ddx = px - worldX, ddy = py - worldY;
        if (ddx*ddx + ddy*ddy < BODY * BODY) playerHP -= 11.0f * dt;

        stateTimer += dt;
        switch (state) {
        case 0:  // 대기 → 팽창 예고
            if (stateTimer >= IDLE_T) { state = 1; stateTimer = 0.0f; }
            break;
        case 1:  // 팽창 예고 → 방출
            if (stateTimer >= TELEGRAPH) {
                stateTimer = 0.0f; state = 2;
                float off = (float)(rand() % 100) * 0.01f;
                for (int i = 0; i < RING_N; i++) {
                    float a = off + (float)i / (float)RING_N * 6.2831853f;
                    fireDir(bullets, cosf(a), sinf(a), RING_SPEED, glm::vec3(1.0f, 0.55f, 0.2f));
                }
            }
            break;
        case 2:  // 방출 직후 텀 → 수축
            if (stateTimer >= EXPAND_HOLD) { state = 3; stateTimer = 0.0f; sumTimer = 0.0f; }
            break;
        case 3:  // 수축: 플레이어 조준 느린 클러스터 반복 소환
            sumTimer += dt;
            if (sumTimer >= 0.30f) {
                sumTimer = 0.0f;
                float base = atan2f(py - worldY, px - worldX);
                for (int i = 0; i < SUMMON_N; i++) {
                    float a = base + ((float)i / (float)(SUMMON_N - 1) - 0.5f) * 0.7f;
                    fireDir(bullets, cosf(a), sinf(a), SUMMON_SPEED, glm::vec3(0.6f, 1.0f, 0.7f));
                }
            }
            if (stateTimer >= CONTRACT_T) { state = 0; stateTimer = 0.0f; }
            break;
        }
    }

    bool telegraphing() const { return state == 1; }   // 렌더가 예고 링 그릴 때
    float telegraphProg() const { return state == 1 ? (stateTimer / TELEGRAPH) : 0.0f; }

private:
    float sumTimer = 0.0f;
};
