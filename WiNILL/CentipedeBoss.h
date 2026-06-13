#pragma once
#include <vector>
#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#include "Bullet.h"

// ─────────────────────────────────────────────────────────────
// BUG.proc — 버그 (지네형 보스)
//   큰 머리 + 점점 작아지는 꼬리 세그먼트가 따라오는 지네. 평소엔 맵을
//   지그재그로 배회하며 압박(이때 피격 가능 = 딜 타임).
//   주기적으로 '벽 돌진': 0.8초 전 경로 예고선을 그린 뒤,
//     ① 현재 위치에서 화면 밖으로 빠져나가고(나갈 때 화면 진동),
//     ② 반대편 가장자리에서 다시 들어와 벽→벽 직선으로 고속 돌진
//   하는 구조(이때 무적 = 지정 불가). 텔레포트 느낌 없이 자연스럽게
//   "나갔다 돌아오는" 연출. 머리/세그먼트 접촉은 큰 피해.
// ─────────────────────────────────────────────────────────────
class CentipedeBoss {
public:
    float worldX, worldY;          // 머리 위치
    float hp, maxHp;
    bool  alive = true;
    bool  exploded = false;
    int   screenW, screenH;

    // 상태: 0=배회(피격가능) / 1=돌진예고 / 2=화면밖 이탈 / 3=재진입 돌진(무적)
    int   state      = 0;
    float stateTimer = 0.0f;
    float wanderTimer = 0.0f;
    float heading    = 0.0f;       // 배회/이탈 진행 방향(rad)

    // 돌진 경로 (렌더가 예고선으로 읽음 — 재진입 돌진의 벽→벽 직선)
    float dashFromX = 0, dashFromY = 0, dashToX = 0, dashToY = 0;
    float dashT = 0.0f;

    bool  wasInside  = true;       // 직전 프레임 머리가 화면 안이었는지
    bool  shakePulse = false;      // 화면 밖으로 나가는 순간 1회 — main 이 읽고 끔

    std::vector<glm::vec2> trail;  // 머리 궤적(세그먼트 추종용)

    static constexpr int   NSEG       = 9;       // 꼬리 세그먼트 수
    static constexpr int   SEG_STEP   = 6;       // 세그먼트 간 궤적 인덱스 간격
    static constexpr float HEAD       = 88.0f;   // 머리 크기(2배 — 큰 버그 머리)
    static constexpr float SEG_NEAR   = 55.0f;   // 머리에 가장 가까운 세그먼트(머리보다 작게)
    static constexpr float SEG_FAR    = 22.0f;   // 꼬리 끝 세그먼트(가장 작음)
    static constexpr float WANDER_SPD = 230.0f;
    static constexpr float DASH_SPD   = 1500.0f; // 벽 돌진 속도
    static constexpr float WANDER_T   = 4.5f;    // 배회 시간(딜 타임)
    static constexpr float TELEGRAPH  = 0.8f;    // 돌진 예고
    static constexpr float TURN_INT   = 0.5f;    // 지그재그 방향 전환 주기

    CentipedeBoss(int sw, int sh, float hpInit) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.5f; worldY = sh * 0.4f;   // main 이 스폰 시 덮어씀
        heading = (float)(rand() % 628) * 0.01f;
        trail.assign(NSEG * SEG_STEP + 8, glm::vec2(worldX, worldY));
    }

    bool vulnerable() const { return state == 0; }   // 배회 중에만 피격 가능

    // 세그먼트 크기 — 머리에서 멀어질수록 선형으로 작아짐(점점 작아지는 지네)
    static float segSize(int i) {
        float t = (NSEG > 1) ? (float)(i - 1) / (float)(NSEG - 1) : 0.0f;
        return SEG_NEAR + (SEG_FAR - SEG_NEAR) * t;
    }

    bool onScreen(float x, float y) const {
        return x >= 0.0f && x <= (float)screenW && y >= 0.0f && y <= (float)screenH;
    }

    void pickDash() {
        // 재진입 돌진: 한쪽 가장자리 → 반대쪽 가장자리 직선 (수평/수직 랜덤)
        float m = 60.0f;
        if (rand() % 2) {   // 수평 돌진
            float y = m + (float)(rand() % (int)std::max(1.0f, (float)screenH - 2*m));
            bool l2r = rand() % 2;
            dashFromX = l2r ? -m : (float)screenW + m;  dashFromY = y;
            dashToX   = l2r ? (float)screenW + m : -m;  dashToY   = y;
        } else {            // 수직 돌진
            float x = m + (float)(rand() % (int)std::max(1.0f, (float)screenW - 2*m));
            bool t2b = rand() % 2;
            dashFromX = x; dashFromY = t2b ? -m : (float)screenH + m;
            dashToX   = x; dashToY   = t2b ? (float)screenH + m : -m;
        }
    }

    void Update(float px, float py, float dt, float& playerHP, std::vector<Bullet>& /*bullets*/) {
        if (!alive) return;
        stateTimer += dt;

        if (state == 0) {            // ── 배회(지그재그) ──
            wanderTimer += dt;
            if (wanderTimer >= TURN_INT) {
                wanderTimer = 0.0f;
                heading += ((rand() % 2) ? 1.0f : -1.0f) * 0.7f;   // 지그재그 꺾임
            }
            // 가장자리 근처면 중앙으로 부드럽게 선회
            float toC = atan2f(screenH*0.5f - worldY, screenW*0.5f - worldX);
            float m = 120.0f;
            if (worldX < m || worldX > screenW - m || worldY < m || worldY > screenH - m) {
                float d = toC - heading;
                while (d >  3.14159265f) d -= 6.2831853f;
                while (d < -3.14159265f) d += 6.2831853f;
                heading += d * 2.0f * dt;
            }
            worldX += cosf(heading) * WANDER_SPD * dt;
            worldY += sinf(heading) * WANDER_SPD * dt;
            if (stateTimer >= WANDER_T) { state = 1; stateTimer = 0.0f; pickDash(); }
        }
        else if (state == 1) {       // ── 돌진 예고 ──
            if (stateTimer >= TELEGRAPH) {
                state = 2; stateTimer = 0.0f;
                // 현재 위치에서 화면 바깥쪽(중앙 반대편)으로 머리를 향하게 — 밖으로 이탈
                heading = atan2f(worldY - screenH*0.5f, worldX - screenW*0.5f);
                wasInside = onScreen(worldX, worldY);
            }
        }
        else if (state == 2) {       // ── 화면 밖 이탈(무적) ──
            worldX += cosf(heading) * DASH_SPD * dt;
            worldY += sinf(heading) * DASH_SPD * dt;
            bool inside = onScreen(worldX, worldY);
            if (wasInside && !inside) shakePulse = true;   // 경계를 넘는 순간 진동 1회
            wasInside = inside;
            // 몸 전체가 화면 밖으로 빠지면 반대편에서 재진입 돌진 시작
            float M = 200.0f;
            if (worldX < -M || worldX > screenW + M || worldY < -M || worldY > screenH + M) {
                state = 3; stateTimer = 0.0f; dashT = 0.0f;
                worldX = dashFromX; worldY = dashFromY;
                // 궤적을 진입점으로 모아 화면을 가로지르는 잔상 방지(둘 다 화면 밖이라 안 보임)
                for (auto& p : trail) p = glm::vec2(worldX, worldY);
            }
        }
        else {                       // ── 재진입 돌진(무적) ──
            float dx = dashToX - dashFromX, dy = dashToY - dashFromY;
            float len = std::sqrt(dx*dx + dy*dy) + 1e-3f;
            dashT += DASH_SPD * dt / len;   // 0→1
            worldX = dashFromX + dx * dashT;
            worldY = dashFromY + dy * dashT;
            if (dashT >= 1.0f) { state = 0; stateTimer = 0.0f; heading = atan2f(dy, dx); }
        }

        // 궤적 기록 (세그먼트 추종)
        trail.insert(trail.begin(), glm::vec2(worldX, worldY));
        if ((int)trail.size() > NSEG * SEG_STEP + 8) trail.pop_back();

        bool dashing = (state == 2 || state == 3);
        // 머리 접촉 피해 (돌진 중 더 아픔)
        float hcr = HEAD * 0.78f;
        float hdx = px - worldX, hdy = py - worldY;
        if (hdx*hdx + hdy*hdy < hcr * hcr)
            playerHP -= (dashing ? 22.0f : 12.0f) * dt;
        // 세그먼트 접촉 피해 (각 마디 크기 기준)
        for (int i = 1; i <= NSEG; i++) {
            glm::vec2 s = segPos(i);
            float sr = segSize(i) + 4.0f;
            float sdx = px - s.x, sdy = py - s.y;
            if (sdx*sdx + sdy*sdy < sr * sr) playerHP -= 8.0f * dt;
        }
    }

    glm::vec2 segPos(int i) const {
        int idx = i * SEG_STEP;
        if (idx >= (int)trail.size()) idx = (int)trail.size() - 1;
        if (idx < 0) idx = 0;
        return trail[idx];
    }
};
