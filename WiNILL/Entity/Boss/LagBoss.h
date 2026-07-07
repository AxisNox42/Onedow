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
//   본체 = 회전하는 "로딩 스피너" 코어(고스트 잔상 트레일 동반, 텔레포트-스타터 이동)
//   핵심 훅: 디싱크 스플릿 — 주기적으로 가짜 분신(스피너 클론)을 흩뿌려 "진짜가 어디냐"를 묻는 페이크형 압박
//     (플레이어 조작/탄속은 절대 건드리지 않음 — 판별력을 요구할 뿐, 컨트롤을 뺏지 않음)
//   + 견제 사격 3종(펄스샷 / 패킷로스 스트림 / 핑 스파이크) + 2페이즈 버퍼오버플로우
class LagBoss {
public:
    static constexpr const wchar_t* BOSS_NAME = L"LAG.exe";

    struct Ghost {
        float x = 0.0f, y = 0.0f;
        float age = 0.0f;
    };
    struct Decoy {
        float x = 0.0f, y = 0.0f;
        float seed = 0.0f;
    };

    float worldX, worldY;
    float hp, maxHp;
    bool  alive = true;
    bool  exploded = false;
    int   screenW, screenH;
    float facing = 0.0f;
    bool  shakePulse = false;    // 버퍼오버플로우 발사 순간 (main.cpp 히트스탑/쉐이크)
    bool  phase2 = false;

    // ── 이펙트 트리거(main.cpp가 소비 후 false로 되돌림) ──
    bool  teleFx = false;
    float teleFxFromX = 0.0f, teleFxFromY = 0.0f;
    bool  splitFx = false;      // 디싱크 스플릿 발생(분신 등장) 순간
    bool  collapseFx = false;   // 디싱크 스플릿 종료(분신 소멸) 순간

    float hullFlash = 0.0f;     // 피격 시 흰 플래시 (main.cpp가 세팅)

    std::vector<Ghost> ghosts;
    std::vector<Decoy> decoys;

    static constexpr float BODY       = 56.0f;
    static constexpr float MAP_PAD    = 110.0f;
    static constexpr float GHOST_LIFE = 0.5f;

    float teleCd  = 1.0f;
    float pulseCd = 2.2f;
    float freezeGlow = 0.0f;   // 렌더용 (버퍼오버플로우 진행도)

    float pulseFlash  = 0.0f;  // 펄스샷 발사 순간 코어 번쩍임
    float streamFlash = 0.0f;  // 스트림 발사 순간 코어 번쩍임

    // ── 디싱크 스플릿: 예고 → 분신 등장(진짜를 섞어서 헷갈리게) → 수렴 ──
    enum class DesyncState { Idle, Telegraph, Split, Collapse };
    DesyncState dsState = DesyncState::Idle;
    float dsCd = 5.0f;
    float dsT  = 0.0f;

    static constexpr float DS_TELEGRAPH_DUR = 0.55f;
    static constexpr float DS_SPLIT_DUR     = 2.6f;
    static constexpr float DS_COLLAPSE_DUR  = 0.35f;
    static constexpr float DS_CD_MIN_P1 = 6.5f, DS_CD_JIT_P1 = 2.2f;
    static constexpr float DS_CD_MIN_P2 = 4.4f, DS_CD_JIT_P2 = 1.8f;
    static constexpr int   DS_DECOY_N_P1 = 1, DS_DECOY_N_P2 = 2;

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
    bool  desyncLocked() const { return dsState != DesyncState::Idle; }
    bool  desyncActive() const { return dsState == DesyncState::Split; }

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

        teleFx = true; teleFxFromX = fromX; teleFxFromY = fromY;

        float base = phase2 ? 0.55f : 0.95f;
        float jit  = phase2 ? 0.55f : 0.85f;
        teleCd = base + (float)(rand() % 100) * 0.01f * jit;
    }

    static void clampToArena(float& x, float& y, int sw, int sh) {
        float m = MAP_PAD;
        if (x < m) x = m; if (x > sw - m) x = sw - m;
        if (y < m) y = m; if (y > sh - m) y = sh - m;
    }

    // ── 디싱크 스플릿: 예고(신호 흔들림) → 분신 등장(진짜와 뒤섞임) → 수렴 ──
    void updateDesync(float dt) {
        switch (dsState) {
        case DesyncState::Idle:
            dsCd -= dt;
            if (dsCd <= 0.0f) { dsState = DesyncState::Telegraph; dsT = 0.0f; }
            break;
        case DesyncState::Telegraph:
            dsT += dt;
            if (dsT >= DS_TELEGRAPH_DUR) {
                dsState = DesyncState::Split; dsT = 0.0f;
                decoys.clear();
                int n = phase2 ? DS_DECOY_N_P2 : DS_DECOY_N_P1;
                for (int i = 0; i < n; i++) {
                    float ang  = (float)(rand() % 628) * 0.01f;
                    float dist = 150.0f + (float)(rand() % 130);
                    Decoy d;
                    d.x = worldX + cosf(ang) * dist;
                    d.y = worldY + sinf(ang) * dist;
                    clampToArena(d.x, d.y, screenW, screenH);
                    d.seed = (float)(rand() % 100) * 0.01f;
                    decoys.push_back(d);
                }
                splitFx = true;
            }
            break;
        case DesyncState::Split:
            dsT += dt;
            if (dsT >= DS_SPLIT_DUR) { dsState = DesyncState::Collapse; dsT = 0.0f; collapseFx = true; }
            break;
        case DesyncState::Collapse:
            dsT += dt;
            if (dsT >= DS_COLLAPSE_DUR) {
                dsState = DesyncState::Idle; dsT = 0.0f;
                decoys.clear();
                float base = phase2 ? DS_CD_MIN_P2 : DS_CD_MIN_P1;
                float jit  = phase2 ? DS_CD_JIT_P2 : DS_CD_JIT_P1;
                dsCd = base + (float)(rand() % 100) * 0.01f * jit;
            }
            break;
        }
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
            pulseFlash = 1.0f;
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
            streamFlash = 1.0f;
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

        if (hullFlash > 0.0f)  hullFlash  = std::max(0.0f, hullFlash  - dt * 5.5f);
        if (pulseFlash > 0.0f) pulseFlash = std::max(0.0f, pulseFlash - dt * 4.5f);
        if (streamFlash > 0.0f) streamFlash = std::max(0.0f, streamFlash - dt * 4.5f);

        updateBufferOverflow(px, py, dt, bullets);

        if (!frozen()) {
            if (!desyncLocked()) updateMovement(px, py, dt);
            updatePulse(px, py, dt, bullets);
            updateStream(px, py, dt, bullets);
            updatePing(px, py, dt, bullets);
            updateDesync(dt);
        }

        float ddx = px - worldX, ddy = py - worldY;
        if (ddx * ddx + ddy * ddy < BODY * BODY)
            HurtPlayer(playerHP, 12.0f * dt);
    }

    // ──────────────────────── 렌더링 ────────────────────────
    static bool inWinPt(float px, float py, float wx, float wy, float ww, float wh) {
        return px >= wx && px <= wx + ww && py >= wy && py <= wy + wh;
    }

    // 로딩 스피너 세그먼트 링 — 회전 위치에 따라 밝은 헤드 → 어두운 테일 그라데이션
    static void spinnerSegs(float cx, float cy, float radius, float spin,
                            float r, float g, float b, float baseA, int segN = 10) {
        const float slot = 6.2831853f / (float)segN;
        const float halfArc = slot * 0.32f;
        for (int i = 0; i < segN; i++) {
            float bright = 1.0f - (float)i / (float)segN * 0.85f;
            float angCenter = spin + (float)i * slot + halfArc;
            drawConeFan(cx, cy, radius, angCenter, halfArc, r, g, b, baseA * bright);
        }
    }

    // 보스 본체 — 보스 소유 창에서만 (로딩 스피너 코어)
    void renderCore(float t) const {
        float telegraphU = (dsState == DesyncState::Telegraph) ? (dsT / DS_TELEGRAPH_DUR) : 0.0f;
        float flash = (hullFlash > 0.0f) ? std::min(1.0f, hullFlash / 0.18f) : 0.0f;
        bool  erroring = frozen();   // 버퍼오버플로우 충전/방출 중엔 코어가 "에러" 색으로

        float baseR = 30.0f;
        float spin  = t * 3.4f;

        float jit = telegraphU * 3.5f;
        float ccx = worldX + (telegraphU > 0.001f ? sinf(t * 53.0f) * jit : 0.0f);
        float ccy = worldY + (telegraphU > 0.001f ? cosf(t * 47.0f) * jit : 0.0f);

        float rr, gg, bb;
        if (erroring)      { rr = 1.0f; gg = 0.25f + flash * 0.5f; bb = 0.35f + flash * 0.5f; }
        else                { rr = 0.30f + flash * 0.7f; gg = 0.85f + flash * 0.15f; bb = 1.0f; }

        spinnerSegs(ccx, ccy, baseR, spin, rr, gg, bb, 0.82f);
        drawCircle(ccx, ccy, baseR * 0.42f, 0.85f + flash * 0.15f, 0.95f, 1.0f, 0.72f + flash * 0.28f);
        drawCircle(ccx, ccy, baseR * 0.16f, 1.0f, 1.0f, 1.0f, 0.9f);

        // 발사 순간 — 코어 번쩍임(펄스=시안 / 스트림=그린) 링
        if (pulseFlash > 0.001f)
            drawCircle(ccx, ccy, baseR * 1.15f, 0.4f, 0.95f, 1.0f, pulseFlash * 0.22f);
        if (streamFlash > 0.001f)
            drawCircle(ccx, ccy, baseR * 1.15f, 0.45f, 0.9f, 0.55f, streamFlash * 0.20f);

        // 디싱크 예고 — 곧 갈라질 것처럼 신호가 흔들림(경고 링)
        if (telegraphU > 0.001f)
            drawCircle(ccx, ccy, baseR * 1.35f, 1.0f, 0.3f, 0.4f, telegraphU * 0.20f);

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

    // 디싱크 분신 — 가짜 스피너 클론(항상 크로매틱 흔들림 = "이건 가짜다" 신호). 창별로 개별 호출
    void renderDecoysInWin(float wx, float wy, float ww, float wh, float t) const {
        if (dsState != DesyncState::Split && dsState != DesyncState::Collapse) return;
        float u = (dsState == DesyncState::Collapse) ? std::max(0.0f, 1.0f - dsT / DS_COLLAPSE_DUR) : 1.0f;
        for (const auto& d : decoys) {
            if (!inWinPt(d.x, d.y, wx, wy, ww, wh)) continue;
            float dspin = t * (2.4f + d.seed * 3.2f);
            float djit  = 3.0f + d.seed * 4.0f;
            float dx1 = d.x + sinf(t * 47.0f + d.seed * 10.0f) * djit;
            float dy1 = d.y + cosf(t * 39.0f + d.seed * 10.0f) * djit;
            // 크로매틱 애버레이션: 살짝 어긋난 두 색 링이 겹쳐 보임 → 흔들리는 가짜 신호
            spinnerSegs(dx1 - djit * 0.6f, dy1, 30.0f * u, dspin, 1.0f, 0.2f, 0.5f, 0.42f * u);
            spinnerSegs(dx1 + djit * 0.6f, dy1, 30.0f * u, dspin, 0.2f, 0.85f, 1.0f, 0.42f * u);
            drawCircle(dx1, dy1, 30.0f * 0.42f * u, 0.75f, 0.8f, 0.85f, 0.45f * u);
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
    const wchar_t* dsLabel() const {
        switch (dsState) {
        case DesyncState::Telegraph: return L"DESYNC..";
        case DesyncState::Split:     return L"DESYNC!!";
        case DesyncState::Collapse:  return L"SYNC";
        default: return L"sync cd";
        }
    }
    float dsDisplayCd() const {
        if (dsState == DesyncState::Telegraph) return DS_TELEGRAPH_DUR - dsT;
        if (dsState == DesyncState::Split)      return DS_SPLIT_DUR - dsT;
        if (dsState == DesyncState::Collapse)   return DS_COLLAPSE_DUR - dsT;
        return dsCd;
    }
    int decoyCount() const { return (int)decoys.size(); }
};
