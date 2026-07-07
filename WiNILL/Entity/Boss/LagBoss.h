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

// LAG.exe — 렉 스파이크 프로세스
//   고스트 잔상 트레일(히트박스는 항상 현재 진짜 몸통에만) + 텔레포트-스타터 이동
//   + 시간 왜곡 포켓(플레이어·플레이어 탄만 감속, 조작감 손상 없음)
//   + 2페이즈: 버퍼오버플로우(완전 정지 → 그 동안 움직인 만큼 방사형 버스트)
class LagBoss {
public:
    static constexpr const wchar_t* BOSS_NAME = L"LAG.exe";

    struct Ghost {
        float x = 0.0f, y = 0.0f;
        float age = 0.0f;
    };
    struct Pocket {
        float x = 0.0f, y = 0.0f;
        float radius = 0.0f;
        float age = 0.0f;
    };

    float worldX, worldY;
    float hp, maxHp;
    bool  alive = true;
    bool  exploded = false;
    int   screenW, screenH;
    float facing = 0.0f;
    bool  shakePulse = false;
    bool  phase2 = false;

    std::vector<Ghost>  ghosts;
    std::vector<Pocket> pockets;

    static constexpr float BODY          = 56.0f;
    static constexpr float MAP_PAD       = 110.0f;
    static constexpr float GHOST_LIFE    = 0.5f;
    static constexpr float POCKET_RADIUS = 92.0f;
    static constexpr float POCKET_FORM   = 0.4f;   // 형성(예고, 아직 안 느려짐)
    static constexpr float POCKET_ACTIVE = 3.0f;   // 활성(실제 감속)
    static constexpr float POCKET_FADE   = 0.4f;   // 소멸
    static constexpr float POCKET_SLOW   = 0.4f;   // 포켓 안 이동/탄속 배율

    float teleCd   = 1.0f;
    float pocketCd = 2.6f;
    float pulseCd  = 2.2f;
    float freezeGlow = 0.0f;   // 렌더용 (버퍼오버플로우 진행도)

    enum class BofState { Idle, Freeze, Recover };
    BofState bofState = BofState::Idle;
    float bofCd  = 9.0f;   // phase2 진입 후부터 카운트
    float bofT   = 0.0f;
    float bofTrackedDist = 0.0f;
    float bofPrevPX = 0.0f, bofPrevPY = 0.0f;

    static constexpr float BOF_FREEZE_DUR    = 0.75f;
    static constexpr float BOF_RECOVER_DUR   = 0.35f;
    static constexpr int   BOF_BASE_BURST    = 10;
    static constexpr int   BOF_MAX_BURST     = 30;
    static constexpr float BOF_DIST_PER_SHOT = 13.0f;
    static constexpr float BOF_CD_MIN        = 8.0f;
    static constexpr float BOF_CD_JIT        = 3.0f;

    float dmgTakenMult = 1.0f;

    LagBoss(int sw, int sh, float hpInit) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.5f;
        worldY = sh * 0.4f;
    }

    float statDamageMult() const { return 1.0f; }
    bool  frozen() const { return bofState == BofState::Freeze || bofState == BofState::Recover; }

    void clampPos() {
        float m = MAP_PAD;
        if (worldX < m) worldX = m;
        if (worldX > screenW - m) worldX = screenW - m;
        if (worldY < m) worldY = m;
        if (worldY > screenH - m) worldY = screenH - m;
    }

    void fireDir(std::vector<Bullet>& b, float ox, float oy, float dx, float dy,
                 float sp, glm::vec3 col, float sz = 1.0f) {
        Bullet bb(ox, oy, ox + dx * 100.0f, oy + dy * 100.0f);
        bb.isEnemy = true; bb.speed = sp; bb.color = col; bb.sizeScale = sz;
        b.push_back(bb);
    }

    void pushGhost(float x, float y) {
        Ghost g; g.x = x; g.y = y; g.age = 0.0f;
        ghosts.push_back(g);
        if (ghosts.size() > 24) ghosts.erase(ghosts.begin());
    }

    // ── 텔레포트-스타터: 불규칙한 간격으로 플레이어 인근에 순간 도약(프레임 스킵) ──
    void updateMovement(float px, float py, float dt) {
        teleCd -= dt;
        if (teleCd > 0.0f) return;

        float fromX = worldX, fromY = worldY;
        for (int i = 1; i <= 3; i++) {
            float u = (float)i / 4.0f;
            pushGhost(fromX, fromY);
            (void)u;
        }

        float ring = 190.0f + (float)(rand() % 220);
        float ang  = (float)(rand() % 628) * 0.01f;
        worldX = px + cosf(ang) * ring;
        worldY = py + sinf(ang) * ring;
        clampPos();
        pushGhost((fromX + worldX) * 0.5f, (fromY + worldY) * 0.5f);
        facing = atan2f(py - worldY, px - worldX);

        float base = phase2 ? 0.55f : 0.95f;
        float jit  = phase2 ? 0.55f : 0.85f;
        teleCd = base + (float)(rand() % 100) * 0.01f * jit;
    }

    void spawnPocket(float px, float py) {
        Pocket p;
        p.x = px + (float)(rand() % 240 - 120);
        p.y = py + (float)(rand() % 240 - 120);
        p.radius = POCKET_RADIUS;
        p.age = 0.0f;
        pockets.push_back(p);
    }

    // ── 시간 왜곡 포켓: 항상 노화/소멸 처리, 얼어있지 않을 때만 새로 생성 ──
    void updatePockets(float px, float py, float dt) {
        for (auto& p : pockets) p.age += dt;
        pockets.erase(std::remove_if(pockets.begin(), pockets.end(), [](const Pocket& p) {
            return p.age > POCKET_FORM + POCKET_ACTIVE + POCKET_FADE;
        }), pockets.end());

        if (frozen()) return;
        pocketCd -= dt;
        int cap = phase2 ? 4 : 2;
        if (pocketCd <= 0.0f && (int)pockets.size() < cap) {
            spawnPocket(px, py);
            float base = phase2 ? 2.1f : 3.4f;
            pocketCd = base + (float)(rand() % 100) * 0.012f;
        }
    }

    // (x,y) 위치가 활성 포켓 안이면 <1 반환 — 플레이어 이동/플레이어 탄 속도에 곱해서 사용
    float speedMultAt(float x, float y) const {
        float mult = 1.0f;
        for (const auto& p : pockets) {
            if (p.age < POCKET_FORM || p.age > POCKET_FORM + POCKET_ACTIVE) continue;
            float dx = x - p.x, dy = y - p.y;
            if (dx * dx + dy * dy < p.radius * p.radius)
                mult = std::min(mult, POCKET_SLOW);
        }
        return mult;
    }

    // ── 기본 견제 사격 (얼어있지 않을 때만) ──
    void updatePulse(float px, float py, float dt, std::vector<Bullet>& bullets) {
        pulseCd -= dt;
        if (pulseCd > 0.0f) return;
        float dx = px - worldX, dy = py - worldY;
        float d = sqrtf(dx * dx + dy * dy);
        if (d > 1.0f) {
            float baseAng = atan2f(dy, dx);
            for (int i = -1; i <= 1; i++) {
                float a = baseAng + (float)i * 0.16f;
                fireDir(bullets, worldX, worldY, cosf(a), sinf(a),
                        260.0f + (float)(rand() % 50), glm::vec3(0.35f, 0.95f, 1.0f), 0.85f);
            }
        }
        pulseCd = (phase2 ? 1.6f : 2.3f) + (float)(rand() % 40) * 0.02f;
    }

    // ── 버퍼오버플로우 (phase2 전용): 완전 정지 → 그 동안 플레이어가 움직인 만큼 방사형 버스트 ──
    void updateBufferOverflow(float px, float py, float dt, std::vector<Bullet>& bullets) {
        if (!phase2) return;
        switch (bofState) {
        case BofState::Idle:
            bofCd -= dt;
            freezeGlow = std::max(0.0f, freezeGlow - dt * 3.0f);
            if (bofCd <= 0.0f) {
                bofState = BofState::Freeze;
                bofT = 0.0f;
                bofTrackedDist = 0.0f;
                bofPrevPX = px; bofPrevPY = py;
            }
            break;
        case BofState::Freeze: {
            bofT += dt;
            freezeGlow = std::min(1.0f, bofT / BOF_FREEZE_DUR);
            float dx = px - bofPrevPX, dy = py - bofPrevPY;
            bofTrackedDist += sqrtf(dx * dx + dy * dy);
            bofPrevPX = px; bofPrevPY = py;
            if (bofT >= BOF_FREEZE_DUR) {
                int n = BOF_BASE_BURST + (int)(bofTrackedDist / BOF_DIST_PER_SHOT);
                if (n > BOF_MAX_BURST) n = BOF_MAX_BURST;
                if (n < BOF_BASE_BURST) n = BOF_BASE_BURST;
                float spin = (float)(rand() % 628) * 0.01f;
                for (int i = 0; i < n; i++) {
                    float a = spin + (float)i * (6.2831853f / (float)n);
                    fireDir(bullets, worldX, worldY, cosf(a), sinf(a),
                            270.0f + (float)(rand() % 70), glm::vec3(1.0f, 0.25f, 0.85f), 0.9f);
                }
                shakePulse = true;
                bofState = BofState::Recover;
                bofT = 0.0f;
            }
        } break;
        case BofState::Recover:
            bofT += dt;
            freezeGlow = std::max(0.0f, 1.0f - bofT / BOF_RECOVER_DUR);
            if (bofT >= BOF_RECOVER_DUR) {
                bofState = BofState::Idle;
                bofCd = BOF_CD_MIN + (float)(rand() % 100) * 0.01f * BOF_CD_JIT;
            }
            break;
        }
    }

    void Update(float px, float py, float dt, float& playerHP, std::vector<Bullet>& bullets) {
        if (!alive) return;
        if (!phase2 && hp <= maxHp * 0.5f) phase2 = true;

        for (auto& g : ghosts) g.age += dt;
        ghosts.erase(std::remove_if(ghosts.begin(), ghosts.end(),
            [](const Ghost& g) { return g.age > GHOST_LIFE; }), ghosts.end());

        updateBufferOverflow(px, py, dt, bullets);

        if (!frozen()) {
            updateMovement(px, py, dt);
            updatePulse(px, py, dt, bullets);
        }
        updatePockets(px, py, dt);

        float ddx = px - worldX, ddy = py - worldY;
        if (ddx * ddx + ddy * ddy < BODY * BODY)
            HurtPlayer(playerHP, 12.0f * dt);
    }

    // ──────────────────────── 렌더링 ────────────────────────
    static bool inWinPt(float px, float py, float wx, float wy, float ww, float wh) {
        return px >= wx && px <= wx + ww && py >= wy && py <= wy + wh;
    }

    // 보스 본체 — 보스 소유 창에서만 (크로매틱 애버레이션 글리치 다이아몬드)
    void renderCore(float t) const {
        float jit = (0.5f + 0.5f * sinf(t * 37.0f)) * 3.0f;
        float sz = 34.0f;
        drawDiamond(worldX - jit, worldY, sz, 1.0f, 0.15f, 0.55f, 0.55f);
        drawDiamond(worldX + jit, worldY, sz, 0.15f, 0.85f, 1.0f, 0.55f);
        drawDiamond(worldX, worldY, sz * 0.9f, 0.85f, 0.95f, 1.0f, 0.95f);

        if (freezeGlow > 0.001f) {
            float ringR = 40.0f + freezeGlow * 70.0f;
            drawCircle(worldX, worldY, ringR, 1.0f, 0.25f, 0.85f, freezeGlow * 0.22f);
            const wchar_t* lbl = (bofState == BofState::Recover) ? L"DUMP" : L"BUFFER";
            float lw = g_TextS.Width(lbl, 0.42f);
            g_TextS.Draw(lbl, worldX - lw * 0.5f, worldY - ringR - 20.0f, 0.42f,
                         1.0f, 0.3f, 0.85f, freezeGlow);
        }
    }

    // 고스트 잔상 — 보스 창 밖(플레이어 창 등)까지 번질 수 있어 창별로 개별 호출
    void renderGhostsInWin(float wx, float wy, float ww, float wh) const {
        for (const auto& g : ghosts) {
            if (!inWinPt(g.x, g.y, wx, wy, ww, wh)) continue;
            float u = 1.0f - g.age / GHOST_LIFE;
            if (u < 0.0f) continue;
            drawDiamond(g.x, g.y, 30.0f * (0.7f + 0.3f * u), 0.55f, 0.85f, 1.0f, u * 0.4f);
        }
    }

    // 시간 왜곡 포켓 — 형성(예고 링) → 활성(짙은 셰이드) → 소멸
    void renderPocketsInWin(float wx, float wy, float ww, float wh) const {
        for (const auto& p : pockets) {
            if (!inWinPt(p.x, p.y, wx, wy, ww, wh)) continue;
            if (p.age < POCKET_FORM) {
                float u = p.age / POCKET_FORM;
                for (int i = 0; i < 20; i++) {
                    float a = (float)i / 20.0f * 6.2831853f;
                    drawCircle(p.x + cosf(a) * p.radius, p.y + sinf(a) * p.radius,
                               2.0f, 0.6f, 0.9f, 1.0f, 0.55f * u);
                }
            } else if (p.age < POCKET_FORM + POCKET_ACTIVE) {
                drawCircle(p.x, p.y, p.radius, 0.35f, 0.8f, 1.0f, 0.16f);
                for (int i = 0; i < 20; i++) {
                    float a = (float)i / 20.0f * 6.2831853f;
                    drawCircle(p.x + cosf(a) * p.radius, p.y + sinf(a) * p.radius,
                               2.2f, 0.55f, 0.9f, 1.0f, 0.85f);
                }
            } else {
                float u = 1.0f - (p.age - POCKET_FORM - POCKET_ACTIVE) / POCKET_FADE;
                if (u < 0.0f) u = 0.0f;
                drawCircle(p.x, p.y, p.radius * (0.6f + 0.4f * u), 0.35f, 0.8f, 1.0f, 0.1f * u);
            }
        }
    }

    // ──────────────────────── HUD ────────────────────────
    const wchar_t* bofLabel() const {
        switch (bofState) {
        case BofState::Freeze:  return L"BUFFER!!";
        case BofState::Recover: return L"DUMP";
        default: return phase2 ? L"BOF CD" : L"LOCKED";
        }
    }
    float bofDisplayCd() const {
        if (bofState == BofState::Freeze)  return BOF_FREEZE_DUR - bofT;
        if (bofState == BofState::Recover) return BOF_RECOVER_DUR - bofT;
        return bofCd;
    }
    int pocketCount() const { return (int)pockets.size(); }
};
