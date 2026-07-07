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
//   + 글로벌 오버로드(공간 제한 없이 주기적으로 게임 전체가 버벅임 — 플레이어 이동/탄속에 스터터)
//   + 견제 사격 3종(펄스샷 / 패킷로스 스트림 / 핑 스파이크) + 2페이즈 버퍼오버플로우
class LagBoss {
public:
    static constexpr const wchar_t* BOSS_NAME = L"LAG.exe";

    struct Ghost {
        float x = 0.0f, y = 0.0f;
        float age = 0.0f;
    };

    float worldX, worldY;
    float hp, maxHp;
    bool  alive = true;
    bool  exploded = false;
    int   screenW, screenH;
    float facing = 0.0f;
    bool  shakePulse = false;    // 버퍼오버플로우 발사 순간 (main.cpp 히트스탑/쉐이크)
    bool  stutterPulse = false;  // 오버로드 프리즈 펄스 시작 순간 (main.cpp 짧은 히트스탑/글리치 플래시)
    bool  phase2 = false;

    std::vector<Ghost> ghosts;

    static constexpr float BODY       = 56.0f;
    static constexpr float MAP_PAD    = 110.0f;
    static constexpr float GHOST_LIFE = 0.5f;

    float teleCd  = 1.0f;
    float pulseCd = 2.2f;
    float freezeGlow = 0.0f;   // 렌더용 (버퍼오버플로우 진행도)

    // ── 글로벌 오버로드: 공간 제한 없이 주기적으로 "게임이 버벅임" ──
    enum class OverloadState { Idle, Telegraph, Active };
    OverloadState ovState = OverloadState::Idle;
    float ovCd   = 4.5f;
    float ovT    = 0.0f;
    float ovPhaseSeed = 0.0f;
    float ovPrevMult  = 1.0f;

    static constexpr float OV_TELEGRAPH_DUR = 0.4f;
    static constexpr float OV_ACTIVE_DUR    = 1.15f;
    static constexpr float OV_STUTTER_PERIOD = 0.32f;   // 프리즈 펄스 반복 주기
    static constexpr float OV_STUTTER_FREEZE = 0.10f;   // 주기 중 실제로 얼어있는 구간
    static constexpr float OV_CD_MIN_P1 = 5.5f, OV_CD_JIT_P1 = 2.2f;
    static constexpr float OV_CD_MIN_P2 = 3.4f, OV_CD_JIT_P2 = 1.6f;

    // ── 핑 스파이크: 플레이어의 과거 위치를 찍어두고, 지연 후 그 자리를 타격 ──
    bool  pingArmed = false;
    float pingCd = 3.2f;
    float pingT  = 0.0f;
    float pingX = 0.0f, pingY = 0.0f;
    static constexpr float PING_DELAY  = 0.85f;
    static constexpr int   PING_N      = 10;
    static constexpr float PING_RADIUS = 66.0f;

    // ── 패킷로스 스트림: 속도가 들쑥날쑥한 견제 사격 (스트림 중간이 끊기는 느낌) ──
    float streamCd = 3.6f;
    static constexpr int STREAM_N = 7;

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
        for (int i = 1; i <= 3; i++) pushGhost(fromX, fromY);

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

    // ── 글로벌 오버로드: 예고(글리치 강화) → 활성(스터터 프리즈 펄스 반복) ──
    float stutterMultAt(float t) const {
        float phase = fmodf(t + ovPhaseSeed, OV_STUTTER_PERIOD);
        return (phase < OV_STUTTER_FREEZE) ? 0.03f : 0.88f;
    }

    void updateOverload(float dt) {
        switch (ovState) {
        case OverloadState::Idle:
            ovCd -= dt;
            if (ovCd <= 0.0f) {
                ovState = OverloadState::Telegraph;
                ovT = 0.0f;
                ovPhaseSeed = (float)(rand() % 100) * 0.01f * OV_STUTTER_PERIOD;
            }
            break;
        case OverloadState::Telegraph:
            ovT += dt;
            if (ovT >= OV_TELEGRAPH_DUR) { ovState = OverloadState::Active; ovT = 0.0f; ovPrevMult = 1.0f; }
            break;
        case OverloadState::Active: {
            ovT += dt;
            float m = stutterMultAt(ovT);
            if (m < 0.3f && ovPrevMult >= 0.3f) stutterPulse = true;   // 프리즈 펄스 진입 순간
            ovPrevMult = m;
            if (ovT >= OV_ACTIVE_DUR) {
                ovState = OverloadState::Idle;
                ovT = 0.0f;
                float base = phase2 ? OV_CD_MIN_P2 : OV_CD_MIN_P1;
                float jit  = phase2 ? OV_CD_JIT_P2 : OV_CD_JIT_P1;
                ovCd = base + (float)(rand() % 100) * 0.01f * jit;
            }
        } break;
        }
    }

    bool overloadActive() const { return ovState == OverloadState::Active; }

    // 공간 제한 없이 항상 적용 — 플레이어 이동속도/플레이어 탄속에 곱해서 사용
    float globalSlowMult() const {
        if (ovState != OverloadState::Active) return 1.0f;
        return stutterMultAt(ovT);
    }

    // 화면 전체 글리치 오버레이 강도 (0~1) — main.cpp 풀스크린 이펙트용
    float overloadGlitchStrength() const {
        if (ovState == OverloadState::Telegraph) return (ovT / OV_TELEGRAPH_DUR) * 0.6f;
        if (ovState == OverloadState::Active) {
            bool freezing = stutterMultAt(ovT) < 0.3f;
            return freezing ? 1.0f : 0.55f;
        }
        return 0.0f;
    }

    // ── 기본 견제 사격: 3-way 펄스 ──
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

    // ── 패킷로스 스트림: 속도가 들쑥날쑥해 리듬을 읽기 힘든 견제 ──
    void updateStream(float px, float py, float dt, std::vector<Bullet>& bullets) {
        streamCd -= dt;
        if (streamCd > 0.0f) return;
        float dx = px - worldX, dy = py - worldY;
        float d = sqrtf(dx * dx + dy * dy);
        if (d > 1.0f) {
            float baseAng = atan2f(dy, dx);
            static const float SPD[3] = { 460.0f, 190.0f, 330.0f };
            for (int i = 0; i < STREAM_N; i++) {
                float spread = (float)(i - STREAM_N / 2) * 0.06f;
                float spd = SPD[i % 3] + (float)(rand() % 30);
                fireDir(bullets, worldX, worldY, cosf(baseAng + spread), sinf(baseAng + spread),
                        spd, glm::vec3(0.4f, 0.9f, 0.55f), 0.8f);
            }
        }
        streamCd = (phase2 ? 2.6f : 3.9f) + (float)(rand() % 50) * 0.02f;
    }

    // ── 핑 스파이크: 플레이어의 과거 위치를 기록 → 지연 후 그 자리를 타격 ──
    void updatePing(float px, float py, float dt, std::vector<Bullet>& bullets) {
        if (pingArmed) {
            pingT -= dt;
            if (pingT <= 0.0f) {
                for (int i = 0; i < PING_N; i++) {
                    float a = (float)i * (6.2831853f / (float)PING_N);
                    fireDir(bullets, pingX, pingY, cosf(a), sinf(a),
                            220.0f + (float)(rand() % 50), glm::vec3(1.0f, 0.6f, 0.2f), 0.85f);
                }
                pingArmed = false;
                pingCd = (phase2 ? 2.1f : 2.9f) + (float)(rand() % 60) * 0.02f;
            }
        } else {
            pingCd -= dt;
            if (pingCd <= 0.0f) {
                pingX = px; pingY = py;
                pingArmed = true;
                pingT = PING_DELAY;
            }
        }
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
            updateStream(px, py, dt, bullets);
            updatePing(px, py, dt, bullets);
            updateOverload(dt);
        }

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

    // 핑 스파이크 텔레그래프 — 기록된 위치에 카운트다운 링 (창별로 개별 호출)
    void renderPingInWin(float wx, float wy, float ww, float wh) const {
        if (!pingArmed) return;
        if (!inWinPt(pingX, pingY, wx, wy, ww, wh)) return;
        float u = 1.0f - pingT / PING_DELAY;   // 0→1 진행
        if (u < 0.0f) u = 0.0f; if (u > 1.0f) u = 1.0f;
        float r = PING_RADIUS * (1.0f - u) + 6.0f;
        drawCircle(pingX, pingY, r, 1.0f, 0.6f, 0.2f, 0.10f + 0.12f * u);
        for (int i = 0; i < 12; i++) {
            float a = (float)i / 12.0f * 6.2831853f;
            drawCircle(pingX + cosf(a) * r, pingY + sinf(a) * r, 2.0f,
                       1.0f, 0.65f, 0.25f, 0.4f + 0.4f * u);
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
    const wchar_t* overloadLabel() const {
        switch (ovState) {
        case OverloadState::Telegraph: return L"OVERLOAD..";
        case OverloadState::Active:    return L"OVERLOAD!!";
        default: return L"ov cd";
        }
    }
    float overloadDisplayCd() const {
        if (ovState == OverloadState::Telegraph) return OV_TELEGRAPH_DUR - ovT;
        if (ovState == OverloadState::Active)     return OV_ACTIVE_DUR - ovT;
        return ovCd;
    }
};
