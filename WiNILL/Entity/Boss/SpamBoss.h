#pragma once
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <glm/glm.hpp>
#include "Bullet.h"
#include "DrawPrim.h"
#include "TextRenderer.h"
#include "PlayerStats.h"

extern TextRenderer g_TextS;

// SPAM.dll — 팝업 광고 폭주 프로세스
//   본체(코어) 자체의 공격력은 낮음. 진짜 위협은 "오류 팝업" 클러터:
//     - 코어 주변에 계속 오류 팝업(시스템 에러 다이얼로그)이 쌓여 코어를 가림 = 그동안 코어 무적("창 = 시야 가림 방패")
//     - 팝업은 각자 수명이 있고, 제때 부수지 않으면 스스로 만료되며 소형 방사형 폭발
//     - 부수면(제때 처리하면) 안전하게 사라짐 — "먼저 처리" 자체가 보상
//   2페이즈: "업데이트 설치" 다운로드 게이지 — 충전 중 코어 완전 무적(팝업만 유효 타깃),
//            완료 시 그 순간 남아있는 팝업 수에 비례한 대형 방사형 버스트 + 팝업 재도배
class SpamBoss {
public:
    static constexpr const wchar_t* BOSS_NAME = L"SPAM.dll";

    struct Popup {
        float x = 0.0f, y = 0.0f;
        float life = 0.0f;
        float maxLife = 6.0f;
        float spawnT = 0.0f;   // 0→1 등장 팝인 애니메이션
        bool  alive = true;
    };

    float worldX, worldY;
    float hp, maxHp;
    bool  alive = true;
    bool  exploded = false;
    int   screenW, screenH;
    bool  phase2 = false;
    float hullFlash = 0.0f;
    bool  shakePulse = false;   // 다운로드 버스트 순간 (main.cpp 히트스탑/쉐이크)
    bool  dlBurstFx  = false;   // 다운로드 버스트 큰 이펙트 트리거

    std::vector<Popup> popups;
    std::vector<glm::vec2> explodeFxQueue;   // 팝업 만료-폭발 위치 (main.cpp가 소비 후 clear)

    static constexpr float BODY          = 50.0f;
    static constexpr float MAP_PAD       = 110.0f;
    static constexpr float SHIELD_RADIUS = 96.0f;   // 이 반경 안에 살아있는 팝업이 있으면 코어 무적
    static constexpr float POPUP_HIT_R   = 22.0f;
    static constexpr int   MAX_POPUPS    = 20;

    float spawnCd = 1.1f;
    float pokeCd  = 3.4f;

    enum class DlState { Idle, Charging };
    DlState dlState = DlState::Idle;
    float dlCd = 8.0f;
    float dlT  = 0.0f;
    static constexpr float DL_CHARGE_DUR = 3.2f;
    static constexpr float DL_CD_MIN = 9.0f, DL_CD_JIT = 3.0f;

    float dmgTakenMult = 1.0f;

    SpamBoss(int sw, int sh, float hpInit) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.5f;
        worldY = sh * 0.4f;
    }

    float statDamageMult() const { return 1.0f; }

    static void clampToArena(float& x, float& y, int sw, int sh) {
        float m = MAP_PAD;
        if (x < m) x = m; if (x > sw - m) x = sw - m;
        if (y < m) y = m; if (y > sh - m) y = sh - m;
    }

    void fireDir(std::vector<Bullet>& b, float ox, float oy, float dx, float dy,
                 float sp, glm::vec3 col, float sz = 1.0f) {
        Bullet bb(ox, oy, ox + dx * 100.0f, oy + dy * 100.0f);
        bb.isEnemy = true; bb.speed = sp; bb.color = col; bb.sizeScale = sz;
        b.push_back(bb);
    }

    bool shielded() const {
        for (const auto& p : popups) {
            if (!p.alive) continue;
            float dx = p.x - worldX, dy = p.y - worldY;
            if (dx * dx + dy * dy < SHIELD_RADIUS * SHIELD_RADIUS) return true;
        }
        return false;
    }
    bool charging() const { return dlState == DlState::Charging; }
    bool coreInvulnerable() const { return charging() || shielded(); }

    void explodePopup(Popup& p, std::vector<Bullet>& bullets) {
        int n = 6;
        for (int i = 0; i < n; i++) {
            float a = (float)i * (6.2831853f / (float)n) + (float)(rand() % 100) * 0.001f;
            fireDir(bullets, p.x, p.y, cosf(a), sinf(a),
                    200.0f + (float)(rand() % 50), glm::vec3(1.0f, 0.35f, 0.3f), 0.72f);
        }
        explodeFxQueue.push_back(glm::vec2(p.x, p.y));
    }

    void spawnBatch(int n) {
        for (int i = 0; i < n; i++) {
            if ((int)popups.size() >= MAX_POPUPS) break;
            Popup p;
            float ang  = (float)(rand() % 628) * 0.01f;
            float dist = 40.0f + (float)(rand() % (phase2 ? 260 : 190));
            p.x = worldX + cosf(ang) * dist;
            p.y = worldY + sinf(ang) * dist;
            clampToArena(p.x, p.y, screenW, screenH);
            p.maxLife = (phase2 ? 4.0f : 5.6f) + (float)(rand() % 100) * 0.01f * 1.6f;
            popups.push_back(p);
        }
    }

    void updateSpawn(float dt) {
        spawnCd -= dt;
        if (spawnCd > 0.0f) return;
        int n = phase2 ? (2 + rand() % 2) : (1 + rand() % 2);
        spawnBatch(n);
        spawnCd = (phase2 ? 0.8f : 1.3f) + (float)(rand() % 40) * 0.01f;
    }

    void updatePopups(float dt, std::vector<Bullet>& bullets) {
        for (auto& p : popups) {
            if (!p.alive) continue;
            if (p.spawnT < 1.0f) p.spawnT = std::min(1.0f, p.spawnT + dt * 6.0f);
            p.life += dt;
            if (p.life >= p.maxLife) {
                p.alive = false;
                explodePopup(p, bullets);
            }
        }
        popups.erase(std::remove_if(popups.begin(), popups.end(),
            [](const Popup& p) { return !p.alive; }), popups.end());
    }

    // ── 코어 자체 견제(약함) — 원본 컨셉대로 본체 공격력은 낮게 ──
    void updatePoke(float px, float py, float dt, std::vector<Bullet>& bullets) {
        pokeCd -= dt;
        if (pokeCd > 0.0f) return;
        float dx = px - worldX, dy = py - worldY;
        float d = sqrtf(dx * dx + dy * dy);
        if (d > 1.0f) fireDir(bullets, worldX, worldY, dx / d, dy / d,
                              195.0f, glm::vec3(1.0f, 0.55f, 0.75f), 0.75f);
        pokeCd = (phase2 ? 2.4f : 3.4f) + (float)(rand() % 50) * 0.02f;
    }

    // ── 2페이즈: 업데이트 설치 다운로드 게이지 ──
    void updateDownload(float dt, std::vector<Bullet>& bullets) {
        if (!phase2) return;
        switch (dlState) {
        case DlState::Idle:
            dlCd -= dt;
            if (dlCd <= 0.0f) { dlState = DlState::Charging; dlT = 0.0f; }
            break;
        case DlState::Charging:
            dlT += dt;
            if (dlT >= DL_CHARGE_DUR) {
                int n = 10 + (int)popups.size() * 2;
                if (n > 34) n = 34;
                float spin = (float)(rand() % 628) * 0.01f;
                for (int i = 0; i < n; i++) {
                    float a = spin + (float)i * (6.2831853f / (float)n);
                    fireDir(bullets, worldX, worldY, cosf(a), sinf(a),
                            250.0f + (float)(rand() % 60), glm::vec3(1.0f, 0.4f, 0.7f), 0.9f);
                }
                shakePulse = true;
                dlBurstFx  = true;
                spawnBatch(5);
                dlState = DlState::Idle; dlT = 0.0f;
                dlCd = DL_CD_MIN + (float)(rand() % 100) * 0.01f * DL_CD_JIT;
            }
            break;
        }
    }

    void Update(float px, float py, float dt, float& playerHP, std::vector<Bullet>& bullets) {
        if (!alive) return;
        if (!phase2 && hp <= maxHp * 0.5f) phase2 = true;
        if (hullFlash > 0.0f) hullFlash = std::max(0.0f, hullFlash - dt * 5.5f);

        updatePopups(dt, bullets);
        if (!charging()) updateSpawn(dt);   // 충전 중엔 스폰 멈춰 플레이어가 걷어낼 틈을 줌
        updatePoke(px, py, dt, bullets);
        updateDownload(dt, bullets);

        float ddx = px - worldX, ddy = py - worldY;
        if (ddx * ddx + ddy * ddy < BODY * BODY)
            HurtPlayer(playerHP, 8.0f * dt);
    }

    // ──────────────────────── 렌더링 ────────────────────────
    static bool inWinPt(float px, float py, float wx, float wy, float ww, float wh) {
        return px >= wx && px <= wx + ww && py >= wy && py <= wy + wh;
    }

    // 얇은 두께의 선분(닫기 버튼 X, 느낌표 등에 사용)
    static void drawLineSeg(float x0, float y0, float x1, float y1, float th,
                            float r, float g, float b, float a) {
        float dx = x1 - x0, dy = y1 - y0;
        float len = sqrtf(dx * dx + dy * dy);
        if (len < 0.001f) return;
        float nx = -dy / len * th * 0.5f, ny = dx / len * th * 0.5f;
        BatchTri(x0 + nx, y0 + ny, x1 + nx, y1 + ny, x1 - nx, y1 - ny, r, g, b, a);
        BatchTri(x0 + nx, y0 + ny, x1 - nx, y1 - ny, x0 - nx, y0 - ny, r, g, b, a);
    }

    // 보스 본체 — 보스 소유 창에서만 (오류 코드 핵심 — 경고 삼각형 아이콘)
    void renderCore(float t) const {
        float flash = (hullFlash > 0.0f) ? std::min(1.0f, hullFlash / 0.18f) : 0.0f;
        bool  shield = shielded();
        bool  chg = charging();
        float pulse = 0.5f + 0.5f * sinf(t * 3.0f);
        float r = BODY * 0.44f;

        float rr = chg ? 0.55f : (1.0f);
        float gg = chg ? 0.65f : (0.22f + flash * 0.75f);
        float bb = chg ? 0.75f : (0.28f + flash * 0.75f);

        drawCircle(worldX, worldY, r + (shield ? 5.0f : 0.0f), 0.10f, 0.10f, 0.12f, 0.85f);
        drawCircle(worldX, worldY, r * 0.9f, rr, gg, bb, 0.55f + flash * 0.4f);

        // 경고 삼각형 + 느낌표
        drawTriangle(worldX, worldY - 1.0f, r * 0.95f, 0.95f, 0.95f, 0.95f, 0.9f);
        drawLineSeg(worldX, worldY - r * 0.28f, worldX, worldY + r * 0.08f, r * 0.11f, 0.85f, 0.15f, 0.2f, 0.9f);
        drawCircle(worldX, worldY + r * 0.30f, r * 0.075f, 0.85f, 0.15f, 0.2f, 0.9f);

        if (shield) {
            for (int i = 0; i < 4; i++) {
                float a = t * 0.6f + (float)i * 1.5708f;
                float bx = worldX + cosf(a) * (r + 16.0f), by = worldY + sinf(a) * (r + 16.0f);
                drawRect(bx - 4.0f, by - 1.5f, 8.0f, 3.0f, 0.4f, 0.85f, 1.0f, 0.5f + 0.3f * pulse);
            }
        }

        if (chg) {
            float prog = dlT / DL_CHARGE_DUR;
            float bw = 130.0f, bh = 15.0f;
            float bx = worldX - bw * 0.5f, by = worldY - r - 40.0f;
            drawRect(bx - 3.0f, by - 3.0f, bw + 6.0f, bh + 6.0f, 0.06f, 0.06f, 0.08f, 0.9f);
            drawRect(bx, by, bw, bh, 0.22f, 0.22f, 0.26f, 0.85f);
            drawRect(bx, by, bw * prog, bh, 0.3f, 0.85f, 1.0f, 0.92f);
            const wchar_t* lbl = L"UPDATE INSTALLING...";
            float lw = g_TextS.Width(lbl, 0.32f);
            g_TextS.Draw(lbl, worldX - lw * 0.5f, by - 15.0f, 0.32f, 0.85f, 0.92f, 1.0f, 0.92f);
        }
    }

    // 오류 팝업 — 코어 창 밖(플레이어 창 등)까지 흩어질 수 있어 창별로 개별 호출
    void renderPopupsInWin(float wx, float wy, float ww, float wh) const {
        for (const auto& p : popups) {
            if (!p.alive) continue;
            if (!inWinPt(p.x, p.y, wx, wy, ww, wh)) continue;
            float u = p.spawnT;
            float lifeFrac = 1.0f - p.life / p.maxLife;
            float danger = 1.0f - lifeFrac;
            float w = 42.0f * u, h = 30.0f * u;

            drawRect(p.x - w * 0.5f, p.y - h * 0.5f, w, h, 0.87f, 0.87f, 0.90f, 0.92f * u);
            drawRect(p.x - w * 0.5f, p.y - h * 0.5f, w, h * 0.30f, 0.8f, 0.15f, 0.2f, 0.95f * u);

            float xcx = p.x + w * 0.36f, xcy = p.y - h * 0.35f;
            drawLineSeg(xcx - 3.0f * u, xcy - 3.0f * u, xcx + 3.0f * u, xcy + 3.0f * u, 1.6f * u, 1, 1, 1, 0.9f * u);
            drawLineSeg(xcx - 3.0f * u, xcy + 3.0f * u, xcx + 3.0f * u, xcy - 3.0f * u, 1.6f * u, 1, 1, 1, 0.9f * u);

            drawTriangle(p.x - w * 0.26f, p.y + h * 0.16f, 13.0f * u, 0.85f, 0.2f, 0.25f, 0.85f * u);
            drawLineSeg(p.x - w * 0.26f, p.y + h * 0.03f, p.x - w * 0.26f, p.y + h * 0.18f, 1.6f * u, 1, 1, 1, 0.9f * u);
            drawCircle(p.x - w * 0.26f, p.y + h * 0.27f, 1.3f * u, 1, 1, 1, 0.9f * u);

            float barMaxW = w - 6.0f * u;
            float barR = 0.3f + 0.6f * danger, barG = 0.78f - 0.55f * danger;
            drawRect(p.x - barMaxW * 0.5f, p.y + h * 0.5f - 4.5f * u, barMaxW * lifeFrac, 3.0f * u,
                     barR, barG, 0.25f, 0.92f * u);

            if (danger > 0.72f)
                drawCircle(p.x, p.y, w * 0.72f, 1.0f, 0.3f, 0.25f, (danger - 0.72f) * 0.65f);
        }
    }

    // ──────────────────────── HUD ────────────────────────
    const wchar_t* dlLabel() const {
        if (charging()) return L"UPDATE!!";
        return phase2 ? L"UPDATE CD" : L"LOCKED";
    }
    float dlDisplayCd() const {
        if (charging()) return DL_CHARGE_DUR - dlT;
        return dlCd;
    }
    int popupCount() const { return (int)popups.size(); }
};
