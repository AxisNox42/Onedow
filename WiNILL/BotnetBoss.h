#pragma once
#include <vector>
#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#include "Bullet.h"

// ─────────────────────────────────────────────────────────────
// BOTNET.exe — 봇넷 (물량형 보스)
//   거대 본체(기존 봇넷 노드 2배) 둘레를 5개의 "프로세스"가 빠르게 공전.
//   주기적으로 프로세스 하나를 플레이어에게 집어던짐(직선 돌진, 접촉 피해).
//   부하 없으면(빗나가면) ~2초 뒤 본체로 재흡수되어 다시 공전.
//   페이즈2(HP 50%↓): 모든 프로세스가 중앙으로 모여 느리게 회전 →
//     난이도 하락 + 프로세스 사이 안전 경로 확보(숨 돌릴 틈).
// ─────────────────────────────────────────────────────────────
class BotnetBoss {
public:
    float worldX, worldY;
    float hp, maxHp;
    bool  alive = true;
    bool  exploded = false;
    int   screenW, screenH;

    bool  phase2     = false;
    float orbitRot   = 0.0f;
    float throwTimer  = 0.0f;

    struct Proc {
        float angle;        // 공전 슬롯 각도(고정 오프셋)
        int   state = 0;    // 0=공전 / 1=투척(돌진) / 2=복귀
        float x = 0, y = 0; // 현재 월드 좌표 (렌더/충돌 공용)
        float vx = 0, vy = 0;
        float t = 0;        // 상태 경과
    };
    Proc procs[5];

    static constexpr int   NPROC      = 5;
    static constexpr float BODY       = 70.0f;     // 2배 노드
    static constexpr float ORBIT_R    = 165.0f;
    static constexpr float ORBIT_SPD  = 2.2f;      // 빠른 공전(rad/s)
    static constexpr float P2_ORBIT_R = 70.0f;     // 페이즈2: 중앙 근접
    static constexpr float P2_SPD     = 0.7f;      // 페이즈2: 느린 회전
    static constexpr float PROC_SIZE  = 26.0f;
    static constexpr float THROW_INT  = 2.4f;
    static constexpr float THROW_SPD  = 560.0f;
    static constexpr float THROW_LIFE = 1.0f;      // 돌진 지속 → 이후 복귀

    BotnetBoss(int sw, int sh, float hpInit) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.5f; worldY = sh * 0.45f;   // main 이 스폰 시 덮어씀
        for (int i = 0; i < NPROC; i++)
            procs[i].angle = (float)i * (6.2831853f / (float)NPROC);
    }

    void Update(float px, float py, float dt, float& playerHP, std::vector<Bullet>& /*bullets*/) {
        if (!alive) return;
        if (!phase2 && hp <= maxHp * 0.5f) phase2 = true;

        float spd  = phase2 ? P2_SPD : ORBIT_SPD;
        float oR   = phase2 ? P2_ORBIT_R : ORBIT_R;
        orbitRot += spd * dt;

        // 본체 접촉 데미지
        float bdx = px - worldX, bdy = py - worldY;
        if (bdx*bdx + bdy*bdy < BODY * BODY) playerHP -= 11.0f * dt;

        // 프로세스 투척 (페이즈2 에선 안 던짐 — 난이도 하락)
        throwTimer += dt;
        if (!phase2 && throwTimer >= THROW_INT) {
            throwTimer = 0.0f;
            for (int i = 0; i < NPROC; i++) {
                if (procs[i].state == 0) {
                    Proc& p = procs[i];
                    float dx = px - p.x, dy = py - p.y;
                    float d  = std::sqrt(dx*dx + dy*dy) + 1e-3f;
                    p.vx = dx / d * THROW_SPD; p.vy = dy / d * THROW_SPD;
                    p.state = 1; p.t = 0.0f;
                    break;
                }
            }
        }

        for (int i = 0; i < NPROC; i++) {
            Proc& p = procs[i];
            float slotX = worldX + cosf(orbitRot + p.angle) * oR;
            float slotY = worldY + sinf(orbitRot + p.angle) * oR;
            if (p.state == 0) {            // 공전
                p.x = slotX; p.y = slotY;
            } else if (p.state == 1) {     // 투척(돌진)
                p.x += p.vx * dt; p.y += p.vy * dt;
                p.t += dt;
                if (p.t >= THROW_LIFE) { p.state = 2; p.t = 0.0f; }
            } else {                       // 복귀(슬롯으로 끌려감)
                float dx = slotX - p.x, dy = slotY - p.y;
                p.x += dx * 6.0f * dt; p.y += dy * 6.0f * dt;
                if (dx*dx + dy*dy < 18.0f * 18.0f) p.state = 0;
            }
            // 프로세스 접촉 데미지 (공전/돌진 모두)
            float pdx = px - p.x, pdy = py - p.y;
            if (pdx*pdx + pdy*pdy < (PROC_SIZE + 6.0f) * (PROC_SIZE + 6.0f))
                playerHP -= (p.state == 1 ? 16.0f : 9.0f) * dt;
        }
    }
};
