#pragma once
#include <vector>
#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#include "Bullet.h"

// ─────────────────────────────────────────────────────────────
// FIREWALL.sys — 방화벽 (방어형 보스)
//   거대 본체를 3개의 회전 보호막 아크가 감싼다. 보호막에 막힌
//   플레이어 총알은 흡수(무피해) → 보호막 '사이 틈'으로 쏴야 본체에 딜.
//   핵심 = 회전 속도 완급:
//     - 2초 매우 느림(SLOW) : 틈이 거의 멈춰 딜 타이밍을 줌.
//     - 1초 빠름(FAST)      : 틈이 빠르게 돌아 위치 변경/회피 강제.
//   완급의 리듬으로 답답함을 줄이고 박자감을 준다. (2페이즈 없음)
// ─────────────────────────────────────────────────────────────
class FirewallBoss {
public:
    float worldX, worldY;
    float hp, maxHp;
    bool  alive = true;
    bool  exploded = false;
    int   screenW, screenH;

    float shieldRot  = 0.0f;   // 보호막 회전각 (렌더/판정 공용)
    float phaseTimer = 0.0f;
    bool  fast       = false;  // false=느림(2초) / true=빠름(1초)
    float fireTimer  = 0.0f;
    float skillTimer = 0.0f;   // 차단 펄스(방사형 링) 스킬 쿨다운
    float skillWarn  = 0.0f;   // 펄스 예고(렌더가 읽음) — >0 동안 본체 깜빡
    float tgtX = 0, tgtY = 0, moveTimer = 0.0f;
    bool  moveInit = false;

    static constexpr float BODY        = 84.0f;    // 본체 2배급
    static constexpr int   SHIELDS     = 3;        // 보호막 아크 3개 (120° 간격)
    static constexpr float SHIELD_R    = 168.0f;   // 보호막 반경
    static constexpr float SHIELD_HALF = 0.62f;    // 아크 반각(rad) ≈ ±35° → 틈 3개
    static constexpr float SLOW_T      = 2.0f;
    static constexpr float FAST_T      = 1.0f;
    static constexpr float SLOW_SPD    = 0.30f;    // rad/s (느림)
    static constexpr float FAST_SPD    = 3.4f;     // rad/s (빠름)
    static constexpr float FIRE_INT    = 1.4f;
    static constexpr float BSPEED      = 320.0f;
    static constexpr float MOVE_SPEED  = 48.0f;    // 느린 배회
    static constexpr float SKILL_INT   = 5.5f;     // 차단 펄스 주기
    static constexpr float SKILL_WARN  = 0.8f;     // 펄스 예고 시간
    static constexpr int   PULSE_N     = 30;       // 방사형 링 탄 수
    static constexpr float PULSE_SPD   = 300.0f;

    FirewallBoss(int sw, int sh, float hpInit) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.5f; worldY = sh * 0.45f;   // main 이 스폰 시 덮어씀
    }

    void fireDir(std::vector<Bullet>& b, float dx, float dy, float sp, glm::vec3 col) {
        Bullet bb(worldX, worldY, worldX + dx * 100.0f, worldY + dy * 100.0f);
        bb.isEnemy = true; bb.speed = sp; bb.color = col;
        b.push_back(bb);
    }

    // 보호막이 해당 (본체 기준)각도를 막고 있는가 — 막혔으면 총알 흡수
    bool shieldBlocks(float ang) const {
        for (int s = 0; s < SHIELDS; s++) {
            float c = shieldRot + (float)s * (6.2831853f / (float)SHIELDS);
            float d = ang - c;
            while (d >  3.14159265f) d -= 6.2831853f;
            while (d < -3.14159265f) d += 6.2831853f;
            if (fabsf(d) < SHIELD_HALF) return true;
        }
        return false;
    }

    void Update(float px, float py, float dt, float& playerHP, std::vector<Bullet>& bullets) {
        if (!alive) return;

        // 회전 완급 — 느림(2초) ↔ 빠름(1초)
        phaseTimer += dt;
        float dur = fast ? FAST_T : SLOW_T;
        if (phaseTimer >= dur) { phaseTimer = 0.0f; fast = !fast; }
        shieldRot += (fast ? FAST_SPD : SLOW_SPD) * dt;
        if (shieldRot > 6.2831853f) shieldRot -= 6.2831853f;

        // 느린 배회 이동 — 주기적으로 화면 안 임의 지점으로 (고정형 → 이동형)
        moveTimer -= dt;
        float marg = BODY + 90.0f;
        if (!moveInit || moveTimer <= 0.0f) {
            moveInit = true;
            moveTimer = 2.5f + (float)(rand() % 150) * 0.01f;
            int rx = screenW - (int)(2.0f * marg); if (rx < 1) rx = 1;
            int ry = screenH - (int)(2.0f * marg); if (ry < 1) ry = 1;
            tgtX = marg + (float)(rand() % rx);
            tgtY = marg + (float)(rand() % ry);
        }
        {
            float mdx = tgtX - worldX, mdy = tgtY - worldY;
            float md  = std::sqrt(mdx*mdx + mdy*mdy);
            if (md > 1.0f) {
                float step = MOVE_SPEED * dt; if (step > md) step = md;
                worldX += mdx / md * step; worldY += mdy / md * step;
            }
        }

        // 본체 접촉 데미지
        float ddx = px - worldX, ddy = py - worldY;
        if (ddx*ddx + ddy*ddy < BODY * BODY) playerHP -= 11.0f * dt;

        // 주기적 견제 사격 — 플레이어 조준 3발 부채꼴
        fireTimer += dt;
        if (fireTimer >= FIRE_INT) {
            fireTimer = 0.0f;
            float base = atan2f(py - worldY, px - worldX);
            for (int i = -1; i <= 1; i++) {
                float a = base + (float)i * 0.22f;
                fireDir(bullets, cosf(a), sinf(a), BSPEED, glm::vec3(1.0f, 0.45f, 0.25f));
            }
        }

        // 공격 스킬 — '차단 펄스': 예고 후 사방으로 방사형 링 탄막 방출 (거리 강제)
        if (skillWarn > 0.0f) skillWarn -= dt;
        skillTimer += dt;
        if (skillTimer >= SKILL_INT && skillWarn <= 0.0f) {
            skillWarn = SKILL_WARN;   // 예고 시작
        }
        if (skillWarn > 0.0f && skillTimer >= SKILL_INT + SKILL_WARN) {
            skillTimer = 0.0f; skillWarn = 0.0f;
            float off = (float)(rand() % 100) * 0.01f;
            for (int i = 0; i < PULSE_N; i++) {
                float a = off + (float)i / (float)PULSE_N * 6.2831853f;
                fireDir(bullets, cosf(a), sinf(a), PULSE_SPD, glm::vec3(1.0f, 0.6f, 0.2f));
            }
        }
    }
    bool pulseWarning() const { return skillWarn > 0.0f; }
};
