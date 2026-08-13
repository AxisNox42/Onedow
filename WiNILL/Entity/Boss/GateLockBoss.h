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
#include "Settings.h"

extern TextRenderer g_TextS;

// ─────────────────────────────────────────────────────────────
// GATE.lock — 데이터 격리 시스템 (창 조작 보스)
//   3페이즈:
//     P1 SCAN  (100% → 60%) : 중앙 정박, 격자 스캔탄막, 창 압축
//     P2 PUSH  ( 60% → 25%) : 순찰 이동, 링 회전탄막, 창 드리프트
//     P3 LOCK  ( 25% →  0%) : 플레이어 창 주위 공전, 반사탄, 데미지 바닥
// ─────────────────────────────────────────────────────────────

class GateLockBoss {
public:
    static constexpr const wchar_t* BOSS_NAME = L"GATE.lock";

    float worldX = 0.0f, worldY = 0.0f;
    float hp = 0.0f, maxHp = 0.0f;
    bool  alive    = true;
    bool  exploded = false;
    int   screenW  = 0, screenH = 0;

    bool phase2 = false;
    bool phase3 = false;

    // ── 시각 타이머 ──────────────────────────────────────────
    float pulseT   = 0.0f;   // 범용 맥동 축적값
    float glitchT  = 0.0f;   // P3 글리치 강도 누적
    float flashT   = 0.0f;   // 피격 플래시 잔량

    // 링 각도 (0: 코어, 1: 중간 링, 2: 외곽 링)
    float ringAng[3] = { 0.0f, 0.0f, 0.0f };

    // ── P1 이동: 중앙 정박 + 미세 부유 ──────────────────────
    //   (외부 드리프트 포스가 없을 때 보스 자신은 거의 안 움직임)

    // ── P2 이동: 화면 내 순찰 ─────────────────────────────────
    struct PatrolPoint { float x, y; };
    static constexpr int PATROL_N = 6;
    PatrolPoint patrol[PATROL_N];
    int   patrolIdx   = 0;
    float patrolWaitT = 0.0f;    // >0 = 현재 지점에서 대기 중
    static constexpr float PATROL_WAIT    = 2.0f;
    static constexpr float PATROL_SPEED   = 150.0f;

    // ── P3 이동: 플레이어 창 중심 주위 공전 ──────────────────
    float orbitAng  = 0.0f;
    static constexpr float ORBIT_RADIUS   = 220.0f;
    static constexpr float ORBIT_SPEED    = 0.85f;   // rad/s

    // ── VFX ───────────────────────────────────────────────────
    struct Spark { float x, y, vx, vy, life; };
    std::vector<Spark> sparks;

    // ── 상수 ─────────────────────────────────────────────────
    static constexpr float CORE_HALF  = 42.0f;   // 코어 사각형 반변장
    static constexpr float HIT_RADIUS = 48.0f;   // 총알 히트박스 반지름
    static constexpr float RING_W[3]  = { 140.0f, 100.0f, 68.0f };  // 링 반변장
    static constexpr float RING_SPD[3]= { 0.28f,  -0.52f,  0.88f }; // 링 회전 속도 (음수=역방향)
    static constexpr float PI         = 3.1415926535f;

    // ─────────────────────────────────────────────────────────
    GateLockBoss(int sw, int sh, float hpInit)
        : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = (float)sw * 0.5f;
        worldY = (float)sh * 0.30f;

        // 링 초기 각도를 조금씩 틀어서 시작부터 생동감 있게
        ringAng[0] = randf(0.0f, PI * 2.0f);
        ringAng[1] = ringAng[0] + PI * 0.33f;
        ringAng[2] = ringAng[0] - PI * 0.17f;

        orbitAng = randf(0.0f, PI * 2.0f);

        buildPatrol();
    }

    // ─────────────────────────────────────────────────────────
    // 정적 헬퍼
    // ─────────────────────────────────────────────────────────
    static float clampf(float v, float lo, float hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    }
    static float randf(float lo, float hi) {
        return lo + (hi - lo) * ((float)(rand() % 10000) / 10000.0f);
    }
    static float lenf(float x, float y) {
        return std::sqrt(x * x + y * y);
    }

    // 회전 사각형 (4개의 삼각형으로 채움)
    static void drawRotRect(float cx, float cy, float w, float h, float ang,
                            float r, float g, float b, float a) {
        float hx = w * 0.5f, hy = h * 0.5f;
        float c = std::cos(ang), s = std::sin(ang);
        auto tx = [&](float x, float y) { return cx + x * c - y * s; };
        auto ty = [&](float x, float y) { return cy + x * s + y * c; };
        float x0 = tx(-hx,-hy), y0 = ty(-hx,-hy);
        float x1 = tx( hx,-hy), y1 = ty( hx,-hy);
        float x2 = tx( hx, hy), y2 = ty( hx, hy);
        float x3 = tx(-hx, hy), y3 = ty(-hx, hy);
        BatchTri(x0,y0, x1,y1, x2,y2, r,g,b,a);
        BatchTri(x0,y0, x2,y2, x3,y3, r,g,b,a);
    }

    // 선분 (두꺼운 회전사각형으로 근사)
    static void drawLine(float x0, float y0, float x1, float y1, float thick,
                         float r, float g, float b, float a) {
        float dx = x1-x0, dy = y1-y0;
        float l = std::sqrt(dx*dx + dy*dy);
        if (l < 0.5f) return;
        drawRotRect((x0+x1)*0.5f, (y0+y1)*0.5f, l, thick,
                    std::atan2(dy, dx), r, g, b, a);
    }

    // 사각형 테두리 (4선분으로 구성된 사각 프레임)
    static void drawRectFrame(float cx, float cy, float hw, float hh, float ang,
                               float r, float g, float b, float a, float thick = 3.0f) {
        float c = std::cos(ang), s = std::sin(ang);
        auto tx = [&](float x, float y) { return cx + x * c - y * s; };
        auto ty = [&](float x, float y) { return cy + x * s + y * c; };
        float xs[4] = {-hw,  hw,  hw, -hw};
        float ys[4] = {-hh, -hh,  hh,  hh};
        for (int i = 0; i < 4; i++) {
            int j = (i + 1) & 3;
            drawLine(tx(xs[i],ys[i]), ty(xs[i],ys[i]),
                     tx(xs[j],ys[j]), ty(xs[j],ys[j]),
                     thick, r, g, b, a);
        }
    }

    // ─────────────────────────────────────────────────────────
    // VFX 헬퍼
    // ─────────────────────────────────────────────────────────
    void pushSpark(float x, float y, float r, float g, float b, int n = 8) {
        for (int i = 0; i < n; i++) {
            float ang  = randf(0.0f, PI * 2.0f);
            float spd  = randf(80.0f, 320.0f);
            sparks.push_back({ x, y, std::cos(ang)*spd, std::sin(ang)*spd, randf(0.18f, 0.42f) });
        }
        if ((int)sparks.size() > 160)
            sparks.erase(sparks.begin(), sparks.begin() + (int)sparks.size() - 160);
    }

    // ─────────────────────────────────────────────────────────
    // 페이즈별 색상 (P1: 초록, P2: 황색, P3: 빨강)
    // ─────────────────────────────────────────────────────────
    glm::vec3 phaseColor() const {
        if (phase3) return { 1.00f, 0.18f, 0.22f };
        if (phase2) return { 0.92f, 0.72f, 0.18f };
        return              { 0.22f, 0.85f, 0.48f };
    }

    const wchar_t* stateTag() const {
        if (phase3) return L"P3 LOCK.mode";
        if (phase2) return L"P2 PUSH.mode";
        return              L"P1 SCAN.mode";
    }

    // ─────────────────────────────────────────────────────────
    // 순찰 지점 생성 (P2 시작 시 / 생성자에서도 미리 준비)
    // ─────────────────────────────────────────────────────────
    void buildPatrol() {
        // 화면 상단 60% + 가장자리에서 200px 안쪽 범위에 6개 분산 배치
        float mx = (float)screenW, my = (float)screenH;
        float mx0 = 200.0f, mx1 = mx - 200.0f;
        float my0 = 150.0f, my1 = my * 0.60f;
        // 2×3 그리드 배치 + 소량 랜덤 오프셋
        float gw = (mx1 - mx0) / 2.0f, gh = (my1 - my0) / 2.0f;
        for (int i = 0; i < PATROL_N; i++) {
            float gx = (float)(i % 3) * gw;
            float gy = (float)(i / 3) * gh;
            patrol[i] = {
                mx0 + gx + gw * 0.5f + randf(-gw * 0.25f, gw * 0.25f),
                my0 + gy + gh * 0.5f + randf(-gh * 0.25f, gh * 0.25f)
            };
        }
        // 첫 순찰 목적지: 현재 위치와 제일 먼 지점부터
        int best = 0;
        float bestD = 0.0f;
        for (int i = 0; i < PATROL_N; i++) {
            float d = lenf(patrol[i].x - worldX, patrol[i].y - worldY);
            if (d > bestD) { bestD = d; best = i; }
        }
        patrolIdx = best;
    }

    // ─────────────────────────────────────────────────────────
    // 난이도 배율
    // ─────────────────────────────────────────────────────────
    float damageMul(Difficulty d) const {
        if (d == Difficulty::EASY) return 0.80f;
        if (d == Difficulty::HARD) return 1.14f;
        return 1.0f;
    }

    // ─────────────────────────────────────────────────────────
    // 업데이트 (이동 + 링 회전 + 스파크 감쇠)
    //   공격 패턴(스캔탄막, 드리프트, 반사탄)은 별도 구현 예정.
    // ─────────────────────────────────────────────────────────
    void Update(float px, float py, float dt, float& playerHP,
                std::vector<Bullet>& bullets, Difficulty difficulty,
                bool dashInvuln) {
        (void)bullets; (void)difficulty; (void)dashInvuln;
        if (!alive) return;

        // ── 페이즈 전환 ────────────────────────────────────
        if (!phase2 && hp <= maxHp * 0.60f) {
            phase2 = true;
            buildPatrol();   // 순찰 지점 새로 뽑기
            patrolWaitT = 0.0f;
            pushSpark(worldX, worldY, 0.92f, 0.72f, 0.18f, 16);
        }
        if (!phase3 && hp <= maxHp * 0.25f) {
            phase3 = true;
            orbitAng = std::atan2(worldY - py, worldX - px); // 현재 상대 각도 유지
            pushSpark(worldX, worldY, 1.0f, 0.18f, 0.22f, 24);
        }

        // ── 시각 타이머 ────────────────────────────────────
        pulseT += dt;
        if (phase3) glitchT += dt;
        if (flashT > 0.0f) flashT -= dt;

        // ── 링 회전 (페이즈 진행에 따라 가속) ────────────
        float phaseMult = phase3 ? 2.4f : (phase2 ? 1.55f : 1.0f);
        for (int i = 0; i < 3; i++)
            ringAng[i] += dt * RING_SPD[i] * phaseMult;

        // ── 이동 ──────────────────────────────────────────
        if (!phase2) {
            moveHover(dt);
        } else if (!phase3) {
            movePatrol(dt);
        } else {
            moveOrbit(dt, px, py);
        }

        // 화면 경계 클램프
        worldX = clampf(worldX, 140.0f, (float)screenW - 140.0f);
        worldY = clampf(worldY, 100.0f, (float)screenH - 200.0f);

        // ── 근접 데미지 (코어에 직접 닿았을 때) ──────────
        {
            float dx = px - worldX, dy = py - worldY;
            if (!dashInvuln && dx*dx + dy*dy < HIT_RADIUS * HIT_RADIUS)
                HurtPlayer(playerHP, 9.0f * dt);
        }

        // ── 스파크 감쇠 ───────────────────────────────────
        for (auto& s : sparks) {
            s.x += s.vx * dt; s.y += s.vy * dt;
            s.vx *= 0.87f; s.vy *= 0.87f;
            s.life -= dt;
        }
        sparks.erase(std::remove_if(sparks.begin(), sparks.end(),
            [](const Spark& s) { return s.life <= 0.0f; }), sparks.end());
    }

    // ─────────────────────────────────────────────────────────
    // 이동 구현 — P1: 미세 부유
    // ─────────────────────────────────────────────────────────
    void moveHover(float dt) {
        // 화면 중앙 기준으로 sin/cos 경로를 천천히 부유
        float targX = (float)screenW * 0.5f
                    + std::sin(pulseT * 0.55f) * 28.0f;
        float targY = (float)screenH * 0.28f
                    + std::cos(pulseT * 0.42f) * 16.0f;
        worldX += (targX - worldX) * 1.2f * dt;
        worldY += (targY - worldY) * 1.2f * dt;
    }

    // ─────────────────────────────────────────────────────────
    // 이동 구현 — P2: 6점 순찰
    // ─────────────────────────────────────────────────────────
    void movePatrol(float dt) {
        if (patrolWaitT > 0.0f) {
            // 현재 지점에서 대기
            patrolWaitT -= dt;
            // 대기 중에도 미세 부유 적용
            float bx = patrol[patrolIdx].x, by = patrol[patrolIdx].y;
            worldX += (bx - worldX) * 3.0f * dt;
            worldY += (by - worldY) * 3.0f * dt;
            return;
        }

        // 목적지로 이동
        float tx = patrol[patrolIdx].x, ty = patrol[patrolIdx].y;
        float dx = tx - worldX, dy = ty - worldY;
        float dist = lenf(dx, dy);
        if (dist < 6.0f) {
            // 도착 → 대기 시작 + 다음 목적지 선택
            patrolWaitT = PATROL_WAIT + randf(-0.3f, 0.5f);
            pickNextPatrol();
        } else {
            float spd = PATROL_SPEED * (phase3 ? 1.0f : 1.0f); // P3은 orbit으로 대체
            float move = std::min(spd * dt, dist);
            worldX += (dx / dist) * move;
            worldY += (dy / dist) * move;
        }
    }

    // 다음 순찰 지점: 현재 지점과 최소 거리 이상인 곳을 랜덤 선택
    void pickNextPatrol() {
        int attempts = 0;
        int next = patrolIdx;
        while (next == patrolIdx && attempts < 12) {
            next = rand() % PATROL_N;
            attempts++;
        }
        patrolIdx = next;
    }

    // ─────────────────────────────────────────────────────────
    // 이동 구현 — P3: 플레이어 창 중심 공전
    // ─────────────────────────────────────────────────────────
    void moveOrbit(float dt, float px, float py) {
        orbitAng += ORBIT_SPEED * dt;
        float targX = px + std::cos(orbitAng) * ORBIT_RADIUS;
        float targY = py + std::sin(orbitAng) * ORBIT_RADIUS;
        // 빠르게 추적 (공전 궤도에 즉시 스냅되게)
        worldX += (targX - worldX) * 6.0f * dt;
        worldY += (targY - worldY) * 6.0f * dt;
    }

    // ─────────────────────────────────────────────────────────
    // 렌더 — 배경 이펙트 (스캔라인 노이즈 등, 현재는 스파크만)
    // ─────────────────────────────────────────────────────────
    void renderFx(float t) const {
        glm::vec3 col = phaseColor();

        // 스파크 잔불
        for (const auto& s : sparks) {
            float a = clampf(s.life / 0.40f, 0.0f, 1.0f);
            float sz = 5.0f + 6.0f * a;
            drawRotRect(s.x, s.y, sz, sz,
                        t * 4.5f + s.x * 0.012f,
                        col.r, col.g, col.b, a * 0.85f);
        }
    }

    // ─────────────────────────────────────────────────────────
    // 렌더 — 보스 본체
    //   구조: 외곽 링 → 중간 링 → 내부 링 → 코어 패널 → 자물쇠 아이콘
    // ─────────────────────────────────────────────────────────
    void renderBody(float t) const {
        glm::vec3 col  = phaseColor();
        float pulse    = 0.5f + 0.5f * std::sin(t * (phase3 ? 11.0f : 6.5f));
        float flash    = clampf(flashT / 0.16f, 0.0f, 1.0f);

        // P3 글리치 오프셋 (보스 본체가 흔들림)
        float gx = 0.0f, gy = 0.0f;
        if (phase3) {
            gx = std::sin(glitchT * 31.0f) * 5.0f;
            gy = std::cos(glitchT * 23.0f) * 3.5f;
        }
        float cx = worldX + gx, cy = worldY + gy;

        // ── 외곽 글로우 (반지름 크게, 투명하게) ──────────
        float glowR = RING_W[0] * (1.1f + pulse * 0.08f);
        drawRect(cx - glowR, cy - glowR, glowR * 2.0f, glowR * 2.0f,
                 col.r * 0.12f, col.g * 0.12f, col.b * 0.12f, 0.45f);

        // ── 링 0: 외곽 링 (가장 크고 느림) ───────────────
        {
            float thick = phase3 ? 4.5f : 3.0f;
            float alpha = 0.45f + pulse * 0.12f;
            drawRectFrame(cx, cy, RING_W[0], RING_W[0], ringAng[2],
                          col.r * 0.6f, col.g * 0.6f, col.b * 0.6f, alpha, thick);
            // 코너 액센트 (각 꼭짓점에 작은 사각형)
            float c2 = std::cos(ringAng[2]), s2 = std::sin(ringAng[2]);
            for (int i = 0; i < 4; i++) {
                float ai = ringAng[2] + PI * 0.5f * (float)i + PI * 0.25f;
                float ex = cx + std::cos(ai) * RING_W[0] * 1.41f;
                float ey = cy + std::sin(ai) * RING_W[0] * 1.41f;
                drawRotRect(ex, ey, 9.0f, 9.0f, ringAng[2] + PI * 0.25f,
                            col.r, col.g, col.b, 0.60f + pulse * 0.20f);
            }
        }

        // ── 링 1: 중간 링 (역방향 회전) ──────────────────
        {
            float thick = phase3 ? 5.0f : 3.5f;
            float alpha = 0.60f + pulse * 0.15f;
            drawRectFrame(cx, cy, RING_W[1], RING_W[1], ringAng[1],
                          col.r * 0.80f, col.g * 0.80f, col.b * 0.80f, alpha, thick);
            // 사변 중앙에 작은 노치 (연결 포인트 느낌)
            for (int i = 0; i < 4; i++) {
                float ai = ringAng[1] + PI * 0.5f * (float)i;
                float ex = cx + std::cos(ai) * RING_W[1];
                float ey = cy + std::sin(ai) * RING_W[1];
                drawRotRect(ex, ey, 12.0f, 5.0f, ringAng[1],
                            col.r, col.g, col.b, 0.70f);
            }
        }

        // ── 링 2: 내부 링 (빠르고 밝음) ─────────────────
        {
            float thick = phase3 ? 6.0f : (phase2 ? 4.5f : 3.5f);
            float alpha = 0.75f + pulse * 0.18f;
            drawRectFrame(cx, cy, RING_W[2], RING_W[2], ringAng[0],
                          col.r, col.g, col.b, alpha, thick);
        }

        // ── 코어 패널 (Win32 다이얼로그 스타일) ──────────
        // 다크 배경 패널
        drawRotRect(cx, cy, CORE_HALF * 2.0f, CORE_HALF * 2.0f, 0.0f,
                    0.04f, 0.05f, 0.06f, 0.96f);
        // 패널 테두리 (얇고 밝음)
        drawRectFrame(cx, cy, CORE_HALF, CORE_HALF, 0.0f,
                      col.r, col.g, col.b, 0.85f + flash * 0.15f, 2.0f);
        // 타이틀바 (패널 상단의 얇은 색띠)
        float tbH = 8.0f;
        drawRect(cx - CORE_HALF, cy - CORE_HALF - tbH, CORE_HALF * 2.0f, tbH,
                 col.r * 0.9f, col.g * 0.9f, col.b * 0.9f, 0.80f);

        // 피격 플래시 (코어 전체를 하얗게)
        if (flash > 0.0f)
            drawRotRect(cx, cy, CORE_HALF * 2.2f, CORE_HALF * 2.2f, 0.0f,
                        1.0f, 1.0f, 1.0f, flash * 0.55f);

        // ── 자물쇠 아이콘 ─────────────────────────────────
        drawLockIcon(cx, cy + 4.0f, col, pulse, t);

        // ── 보스 이름 태그 (코어 아래) ───────────────────
        const wchar_t* tag = stateTag();
        float tw = g_TextS.Width(tag, 0.46f);
        g_TextS.Draw(tag,
                     cx - tw * 0.5f,
                     cy + CORE_HALF + 14.0f,
                     0.46f,
                     col.r, col.g, col.b, 0.90f);

        // ── HP 바 (보스 이름 아래) ───────────────────────
        {
            float bw = CORE_HALF * 2.4f, bh = 4.0f;
            float bx = cx - bw * 0.5f;
            float by = cy + CORE_HALF + 36.0f;
            float frac = clampf(hp / maxHp, 0.0f, 1.0f);
            drawRect(bx, by, bw, bh, 0.12f, 0.12f, 0.14f, 0.90f);
            drawRect(bx, by, bw * frac, bh, col.r, col.g, col.b, 0.95f);
        }
    }

private:
    // ── 자물쇠 아이콘 드로우 ─────────────────────────────────
    //   락 바디: 작은 사각형 (cx, cy+4 기준)
    //   걸쇠(shackle): 역 U자 — 두 수직선 + 상단 연결선
    void drawLockIcon(float cx, float cy, glm::vec3 col, float pulse, float t) const {
        (void)t;
        float br  = col.r, bg = col.g, bb  = col.b;
        float alpha = 0.80f + pulse * 0.14f;

        // 락 바디 (채워진 사각형)
        float bw = 18.0f, bh = 14.0f;
        drawRotRect(cx, cy + 6.0f, bw, bh, 0.0f, br, bg, bb, alpha);

        // 락 걸쇠 — 두 수직선 + 상단 수평선
        float shW = 11.0f, shH = 12.0f, shThick = 3.5f;
        float lx = cx - shW * 0.5f, rx = cx + shW * 0.5f;
        float top = cy - 4.0f, bot = cy;
        drawLine(lx, bot, lx, top,        shThick, br, bg, bb, alpha); // 왼쪽
        drawLine(rx, bot, rx, top,        shThick, br, bg, bb, alpha); // 오른쪽
        drawLine(lx, top, rx, top,        shThick, br, bg, bb, alpha); // 상단 연결
        (void)shH;

        // 키홀 (락 바디 중앙 작은 구멍 — 어두운 원형 근사)
        drawRotRect(cx, cy + 6.0f, 5.5f, 7.0f, 0.0f,
                    0.04f, 0.05f, 0.06f, 0.92f);
    }
};
