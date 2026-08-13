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
//   P1 RING  (100→60%) : 링 탄막(6발) + 창 압축 + 잠금 노드
//   P2 PUSH  ( 60→25%) : 링 탄막(10발) + 창 드리프트
//   P3 LOCK  ( 25→ 0%) : 창 주위 공전 + 반사탄 + 격리 피해
//
//   main.cpp 역할:
//     - 압축(compressPauseT) / 드리프트(driftVX,driftVY) 적용
//     - 잠금 노드 총알 히트 체크 (onNodeKill)
//     - P3 창 크기 220px 고정
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
    float pulseT  = 0.0f;
    float glitchT = 0.0f;
    float flashT  = 0.0f;

    float ringAng[3] = { 0.0f, 0.0f, 0.0f };

    // last-known player window center (stored each Update, used in renderFx)
    float lastPx = 0.0f, lastPy = 0.0f, lastWinHalf = 200.0f;

    // ─────────────────────────────────────────────────────────
    // P1: 잠금 노드 (4 모서리)
    //   main.cpp가 총알 히트 체크 후 onNodeKill() 호출
    // ─────────────────────────────────────────────────────────
    struct LockNode {
        int   cx, cy;  // corner sign ±1 (world pos = px ± cx*winHalf)
        float life;    // auto-expire timer
        bool  alive;
    };
    LockNode nodes[4];
    float    nodeCd         = 15.0f;  // time until next node batch
    float    compressPauseT = 0.0f;   // >0 = compression paused
    float    p3EntrySize    = 400.0f; // windowSize when P3 started, for restore on death

    static constexpr float NODE_LIFE        = 15.0f;
    static constexpr float NODE_PENALTY_HP  = 18.0f;  // damage if node expires un-hit
    static constexpr float COMPRESS_RATE    = 12.0f;  // px/s shrink
    static constexpr float COMPRESS_MIN_P1  = 280.0f;
    static constexpr float COMPRESS_MIN_P3  = 220.0f;
    static constexpr float NODE_EXPAND      = 28.0f;  // window expand on node kill
    static constexpr float NODE_PAUSE       = 3.5f;   // compress pause duration

    // ─────────────────────────────────────────────────────────
    // P2: 링 탄막
    // ─────────────────────────────────────────────────────────
    float ringFireCd  = 2.5f;   // P1 첫 발사까지 여유
    float ringBaseAng = 0.0f;
    int   ringStep    = 0;

    static constexpr float RING_BSPEED = 420.0f;
    static constexpr float RING_DMG    = 16.0f;

    // ─────────────────────────────────────────────────────────
    // P2: 창 드리프트 (main.cpp가 playerWin에 적용)
    // ─────────────────────────────────────────────────────────
    float driftVX       = 0.0f;
    float driftVY       = 0.0f;
    float driftSwitchT  = 0.0f;

    static constexpr float DRIFT_SPEED    = 125.0f;
    static constexpr float DRIFT_INTERVAL = 4.8f;

    // ─────────────────────────────────────────────────────────
    // P3: 반사탄
    // ─────────────────────────────────────────────────────────
    struct BounceBullet {
        float x, y, vx, vy;
        int   bounces;
        float life;
        bool  alive;
    };
    std::vector<BounceBullet> bounceBullets;
    float bounceFireCd = 1.8f;

    static constexpr int   BOUNCE_MAX   = 4;
    static constexpr float BOUNCE_SPD   = 380.0f;
    static constexpr float BOUNCE_DMG   = 13.0f;
    static constexpr float BOUNCE_LIFE  = 6.0f;
    static constexpr float BOUNCE_R     = 7.0f;

    // P3: 격리 피해 (패시브 틱)
    static constexpr float ISOLATE_DMG_BASE = 2.5f;   // HP/s
    static constexpr float ISOLATE_DMG_ENRAGE = 5.5f; // HP/s when boss < 10% HP

    // ─────────────────────────────────────────────────────────
    // 이동
    // ─────────────────────────────────────────────────────────
    struct PatrolPoint { float x, y; };
    static constexpr int PATROL_N = 6;
    PatrolPoint patrol[PATROL_N];
    int   patrolIdx   = 0;
    float patrolWaitT = 0.0f;
    static constexpr float PATROL_WAIT  = 2.0f;
    static constexpr float PATROL_SPEED = 150.0f;

    float orbitAng = 0.0f;
    static constexpr float ORBIT_RADIUS = 220.0f;
    static constexpr float ORBIT_SPEED  = 0.85f;

    // ─────────────────────────────────────────────────────────
    // VFX
    // ─────────────────────────────────────────────────────────
    struct Spark { float x, y, vx, vy, life; };
    std::vector<Spark> sparks;

    // ─────────────────────────────────────────────────────────
    // 상수
    // ─────────────────────────────────────────────────────────
    static constexpr float CORE_HALF  = 42.0f;
    static constexpr float HIT_RADIUS = 48.0f;
    static constexpr float RING_W[3]  = { 140.0f, 100.0f, 68.0f };
    static constexpr float RING_SPD[3]= { 0.28f, -0.52f,  0.88f };
    static constexpr float PI         = 3.1415926535f;

    // ─────────────────────────────────────────────────────────
    GateLockBoss(int sw, int sh, float hpInit)
        : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = (float)sw * 0.5f;
        worldY = (float)sh * 0.30f;
        ringAng[0] = randf(0.0f, PI * 2.0f);
        ringAng[1] = ringAng[0] + PI * 0.33f;
        ringAng[2] = ringAng[0] - PI * 0.17f;
        orbitAng   = randf(0.0f, PI * 2.0f);
        for (int i = 0; i < 4; i++) nodes[i] = { (i<2?-1:1), (i==0||i==3?-1:1), 0.0f, false };
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
    static float lenf(float x, float y) { return std::sqrt(x*x + y*y); }

    static void drawRotRect(float cx, float cy, float w, float h, float ang,
                            float r, float g, float b, float a) {
        float hx = w * 0.5f, hy = h * 0.5f;
        float c = std::cos(ang), s = std::sin(ang);
        auto tx = [&](float x, float y) { return cx + x*c - y*s; };
        auto ty = [&](float x, float y) { return cy + x*s + y*c; };
        float x0=tx(-hx,-hy),y0=ty(-hx,-hy), x1=tx(hx,-hy),y1=ty(hx,-hy);
        float x2=tx(hx,hy),  y2=ty(hx,hy),   x3=tx(-hx,hy),y3=ty(-hx,hy);
        BatchTri(x0,y0,x1,y1,x2,y2, r,g,b,a);
        BatchTri(x0,y0,x2,y2,x3,y3, r,g,b,a);
    }

    static void drawLine(float x0, float y0, float x1, float y1, float thick,
                         float r, float g, float b, float a) {
        float dx=x1-x0, dy=y1-y0, l=lenf(dx,dy);
        if (l < 0.5f) return;
        drawRotRect((x0+x1)*0.5f,(y0+y1)*0.5f, l, thick, std::atan2(dy,dx), r,g,b,a);
    }

    static void drawRectFrame(float cx, float cy, float hw, float hh, float ang,
                               float r, float g, float b, float a, float thick = 3.0f) {
        float c=std::cos(ang), s=std::sin(ang);
        auto tx=[&](float x,float y){return cx+x*c-y*s;};
        auto ty=[&](float x,float y){return cy+x*s+y*c;};
        float xs[4]={-hw,hw,hw,-hw}, ys[4]={-hh,-hh,hh,hh};
        for (int i=0;i<4;i++){int j=(i+1)&3;
            drawLine(tx(xs[i],ys[i]),ty(xs[i],ys[i]),tx(xs[j],ys[j]),ty(xs[j],ys[j]),thick,r,g,b,a);}
    }

    // ─────────────────────────────────────────────────────────
    // VFX 헬퍼
    // ─────────────────────────────────────────────────────────
    void pushSpark(float x, float y, float r, float g, float b, int n = 8) {
        for (int i = 0; i < n; i++) {
            float a = randf(0.0f, PI*2.0f), sp = randf(80.0f, 320.0f);
            sparks.push_back({x, y, std::cos(a)*sp, std::sin(a)*sp, randf(0.18f, 0.42f)});
        }
        if ((int)sparks.size() > 180)
            sparks.erase(sparks.begin(), sparks.begin() + (int)sparks.size() - 180);
    }

    // ─────────────────────────────────────────────────────────
    // 페이즈 색상
    // ─────────────────────────────────────────────────────────
    glm::vec3 phaseColor() const {
        if (phase3) return {1.00f, 0.18f, 0.22f};
        if (phase2) return {0.92f, 0.72f, 0.18f};
        return              {0.22f, 0.85f, 0.48f};
    }
    const wchar_t* stateTag() const {
        if (phase3) return L"P3 LOCK.mode";
        if (phase2) return L"P2 PUSH.mode";
        return              L"P1 RING.fire";
    }

    float damageMul(Difficulty d) const {
        return d == Difficulty::EASY ? 0.80f : d == Difficulty::HARD ? 1.14f : 1.0f;
    }

    // ─────────────────────────────────────────────────────────
    // 순찰 지점 생성
    // ─────────────────────────────────────────────────────────
    void buildPatrol() {
        float mx0=200.0f, mx1=(float)screenW-200.0f;
        float my0=150.0f, my1=(float)screenH*0.60f;
        float gw=(mx1-mx0)/2.0f, gh=(my1-my0)/2.0f;
        for (int i=0;i<PATROL_N;i++) {
            patrol[i]={mx0+(float)(i%3)*gw+gw*0.5f+randf(-gw*0.24f,gw*0.24f),
                       my0+(float)(i/3)*gh+gh*0.5f+randf(-gh*0.24f,gh*0.24f)};
        }
        int best=0; float bd=0;
        for (int i=0;i<PATROL_N;i++){float d=lenf(patrol[i].x-worldX,patrol[i].y-worldY);if(d>bd){bd=d;best=i;}}
        patrolIdx=best;
    }
    void pickNextPatrol() {
        int next=patrolIdx, tries=0;
        while (next==patrolIdx && tries<12) { next=rand()%PATROL_N; tries++; }
        patrolIdx=next;
    }

    // ─────────────────────────────────────────────────────────
    // 잠금 노드 — main.cpp가 bullet hit 확인 후 호출
    // ─────────────────────────────────────────────────────────
    void onNodeKill(int i) {
        nodes[i].alive    = false;
        compressPauseT    = NODE_PAUSE;
        pushSpark(lastPx + nodes[i].cx * lastWinHalf,
                  lastPy + nodes[i].cy * lastWinHalf,
                  0.22f, 0.85f, 0.48f, 14);
    }

    // 만료된 노드의 데미지는 main.cpp에서 직접 처리

    // ─────────────────────────────────────────────────────────
    // P1: 잠금 노드 업데이트
    // ─────────────────────────────────────────────────────────
    void updateNodes(float dt, float& playerHP, Difficulty diff) {
        nodeCd -= dt;
        if (nodeCd <= 0.0f) {
            for (int i=0;i<4;i++) if (!nodes[i].alive) {
                nodes[i].alive = true;
                nodes[i].life  = NODE_LIFE;
            }
            nodeCd = NODE_LIFE + randf(-1.0f, 2.0f);
        }
        for (int i=0;i<4;i++) {
            if (!nodes[i].alive) continue;
            nodes[i].life -= dt;
            if (nodes[i].life <= 0.0f) {
                nodes[i].alive = false;
                HurtPlayer(playerHP, NODE_PENALTY_HP * damageMul(diff));
                pushSpark(lastPx + nodes[i].cx*lastWinHalf,
                          lastPy + nodes[i].cy*lastWinHalf, 1.0f,0.18f,0.22f, 10);
            }
        }
    }

    // ─────────────────────────────────────────────────────────
    // P2: 링 탄막
    // ─────────────────────────────────────────────────────────
    void updateRingFire(float dt, float px, float py,
                        std::vector<Bullet>& bullets, Difficulty diff) {
        ringFireCd -= dt;
        if (ringFireCd > 0.0f) return;

        ++ringStep;
        float dmg  = RING_DMG * damageMul(diff);
        float spd  = RING_BSPEED;
        bool  fan  = (ringStep % 3 == 2);

        if (!fan) {
            // 원형 링 — P1:6, P2:10, P3:12
            int   n   = phase3 ? 12 : (phase2 ? 10 : 6);
            float off = ringBaseAng;
            for (int i=0;i<n;i++) {
                float a = off + (float)i / (float)n * PI * 2.0f;
                Bullet b(worldX, worldY, worldX + std::cos(a), worldY + std::sin(a));
                b.speed = spd; b.isEnemy = true; b.enemyDmg = dmg;
                b.color = phase3 ? glm::vec3(1.0f,0.18f,0.22f) : glm::vec3(0.92f,0.72f,0.18f);
                bullets.push_back(b);
            }
            ringBaseAng += 0.38f;
        } else {
            // 플레이어 조준 부채꼴 (5발)
            float base = std::atan2(py - worldY, px - worldX);
            for (int i=-2;i<=2;i++) {
                float a = base + (float)i * 0.20f;
                Bullet b(worldX, worldY, worldX + std::cos(a), worldY + std::sin(a));
                b.speed = spd * 1.15f; b.isEnemy = true; b.enemyDmg = dmg;
                b.color = phase3 ? glm::vec3(1.0f,0.18f,0.22f) : glm::vec3(0.92f,0.72f,0.18f);
                bullets.push_back(b);
            }
        }
        ringFireCd = phase3 ? 1.1f : (phase2 ? 1.4f : 2.0f);
        ringFireCd += randf(-0.12f, 0.18f);
    }

    // ─────────────────────────────────────────────────────────
    // P2: 드리프트 방향 업데이트 (벡터만 계산, 적용은 main.cpp)
    // ─────────────────────────────────────────────────────────
    void updateDrift(float dt, float px, float py) {
        driftSwitchT -= dt;
        if (driftSwitchT > 0.0f) return;
        driftSwitchT = DRIFT_INTERVAL + randf(-0.6f, 1.0f);

        // 현재 플레이어 창과 화면 엣지 중 가장 먼 방향으로 밀기
        float toL = px, toR = (float)screenW - px;
        float toT = py, toB = (float)screenH - py;
        // 각 방향 중 가장 '멀리' 밀 수 있는 방향 선택 (공간이 남아있는 쪽)
        float mx = (toR > toL) ? 1.0f : -1.0f;
        float my = (toB > toT) ? 1.0f : -1.0f;
        // 수평/수직 중 랜덤 선택
        bool useH = (rand() % 2) == 0;
        float spd = DRIFT_SPEED * (phase3 ? 1.0f : 1.0f);  // P3은 orbit으로 대체
        driftVX = useH ? mx * spd : 0.0f;
        driftVY = useH ? 0.0f    : my * spd;
    }

    // ─────────────────────────────────────────────────────────
    // P3: 반사탄 발사
    // ─────────────────────────────────────────────────────────
    void updateBounceFire(float dt, float px, float py) {
        bounceFireCd -= dt;
        if (bounceFireCd > 0.0f) return;

        // 3발을 플레이어 방향 ±스프레드로 발사
        float base = std::atan2(py - worldY, px - worldX);
        int n = phase3 ? 5 : 4;
        for (int i=0;i<n;i++) {
            float spread = (float)(i - n/2) * 0.28f;
            float a = base + spread;
            BounceBullet bb{};
            bb.x  = worldX; bb.y = worldY;
            bb.vx = std::cos(a) * BOUNCE_SPD;
            bb.vy = std::sin(a) * BOUNCE_SPD;
            bb.bounces = 0;
            bb.life    = BOUNCE_LIFE;
            bb.alive   = true;
            bounceBullets.push_back(bb);
        }
        bounceFireCd = phase3 ? 1.15f : 1.55f;
        bounceFireCd += randf(-0.1f, 0.2f);
    }

    // P3: 반사탄 업데이트 (이동 + 반사 + 플레이어 히트)
    void updateBounceBullets(float dt, float px, float py, float winHalf,
                             float& playerHP, bool dashInvuln, Difficulty diff) {
        float wx0 = px - winHalf, wx1 = px + winHalf;
        float wy0 = py - winHalf, wy1 = py + winHalf;
        float dmg = BOUNCE_DMG * damageMul(diff);

        for (auto& bb : bounceBullets) {
            if (!bb.alive) continue;
            bb.life -= dt;
            if (bb.life <= 0.0f) { bb.alive = false; continue; }

            bb.x += bb.vx * dt;
            bb.y += bb.vy * dt;

            // playerWin 경계 반사
            if (bb.bounces < BOUNCE_MAX) {
                if (bb.x < wx0 && bb.vx < 0.0f) { bb.vx = -bb.vx; bb.x = wx0; bb.bounces++; }
                if (bb.x > wx1 && bb.vx > 0.0f) { bb.vx = -bb.vx; bb.x = wx1; bb.bounces++; }
                if (bb.y < wy0 && bb.vy < 0.0f) { bb.vy = -bb.vy; bb.y = wy0; bb.bounces++; }
                if (bb.y > wy1 && bb.vy > 0.0f) { bb.vy = -bb.vy; bb.y = wy1; bb.bounces++; }
            } else {
                bb.alive = false; continue;
            }

            // 플레이어 히트 (창 중심 기준)
            if (!dashInvuln) {
                float dx = bb.x - px, dy = bb.y - py;
                if (dx*dx + dy*dy < (BOUNCE_R + 14.0f)*(BOUNCE_R + 14.0f)) {
                    HurtPlayer(playerHP, dmg);
                    bb.alive = false;
                    pushSpark(bb.x, bb.y, 1.0f, 0.18f, 0.22f, 6);
                }
            }
        }
        bounceBullets.erase(std::remove_if(bounceBullets.begin(), bounceBullets.end(),
            [](const BounceBullet& b){ return !b.alive; }), bounceBullets.end());
    }

    // ─────────────────────────────────────────────────────────
    // P3: 격리 피해 (패시브)
    // ─────────────────────────────────────────────────────────
    void applyIsolationDamage(float dt, float& playerHP, Difficulty diff) {
        float rate = (hp / maxHp < 0.10f) ? ISOLATE_DMG_ENRAGE : ISOLATE_DMG_BASE;
        HurtPlayer(playerHP, rate * damageMul(diff) * dt);
    }

    // ─────────────────────────────────────────────────────────
    // 업데이트
    //   winHalf = playerWin.width * 0.5f  (main.cpp에서 전달)
    // ─────────────────────────────────────────────────────────
    void Update(float px, float py, float winHalf, float dt, float& playerHP,
                std::vector<Bullet>& bullets, Difficulty difficulty, bool dashInvuln) {
        if (!alive) return;

        lastPx = px; lastPy = py; lastWinHalf = winHalf;

        // ── 페이즈 전환 ────────────────────────────────────
        if (!phase2 && hp <= maxHp * 0.60f) {
            phase2 = true;
            buildPatrol();
            patrolWaitT = 0.0f;
            nodeCd = 14.0f;   // 잠금 노드 유지 (P2에서도 작동)
            driftSwitchT = 0.0f;
            pushSpark(worldX, worldY, 0.92f, 0.72f, 0.18f, 16);
        }
        if (!phase3 && hp <= maxHp * 0.25f) {
            phase3 = true;
            bounceBullets.clear();
            for (int i=0;i<4;i++) nodes[i].alive = false;
            orbitAng = std::atan2(worldY - py, worldX - px);
            driftVX = driftVY = 0.0f;
            pushSpark(worldX, worldY, 1.0f, 0.18f, 0.22f, 24);
        }

        // ── 시각 ──────────────────────────────────────────
        pulseT += dt;
        if (phase3) glitchT += dt;
        if (flashT > 0.0f) flashT -= dt;

        float phaseMult = phase3 ? 2.4f : (phase2 ? 1.55f : 1.0f);
        for (int i=0;i<3;i++) ringAng[i] += dt * RING_SPD[i] * phaseMult;

        // ── 이동 ──────────────────────────────────────────
        if (!phase2) moveHover(dt);
        else if (!phase3) movePatrol(dt);
        else moveOrbit(dt, px, py);

        worldX = clampf(worldX, 140.0f, (float)screenW - 140.0f);
        worldY = clampf(worldY, 100.0f, (float)screenH - 200.0f);

        // ── 공격 패턴 ─────────────────────────────────────
        if (!phase3) updateNodes(dt, playerHP, difficulty);
        updateRingFire(dt, px, py, bullets, difficulty);   // 전 페이즈
        if (phase2 && !phase3) updateDrift(dt, px, py);
        if (phase3) {
            updateBounceFire(dt, px, py);
            updateBounceBullets(dt, px, py, winHalf, playerHP, dashInvuln, difficulty);
            applyIsolationDamage(dt, playerHP, difficulty);
        }

        // ── 근접 접촉 피해 ────────────────────────────────
        {
            float dx=px-worldX, dy=py-worldY;
            if (!dashInvuln && dx*dx+dy*dy < HIT_RADIUS*HIT_RADIUS)
                HurtPlayer(playerHP, 9.0f * dt);
        }

        // ── 스파크 ────────────────────────────────────────
        for (auto& s : sparks) {
            s.x+=s.vx*dt; s.y+=s.vy*dt; s.vx*=0.87f; s.vy*=0.87f; s.life-=dt;
        }
        sparks.erase(std::remove_if(sparks.begin(), sparks.end(),
            [](const Spark& s){ return s.life<=0.0f; }), sparks.end());
    }

    // ─────────────────────────────────────────────────────────
    // 이동 구현
    // ─────────────────────────────────────────────────────────
    void moveHover(float dt) {
        float tx = (float)screenW*0.5f + std::sin(pulseT*0.55f)*28.0f;
        float ty = (float)screenH*0.28f + std::cos(pulseT*0.42f)*16.0f;
        worldX += (tx-worldX)*1.2f*dt;
        worldY += (ty-worldY)*1.2f*dt;
    }

    void movePatrol(float dt) {
        if (patrolWaitT > 0.0f) {
            patrolWaitT -= dt;
            worldX += (patrol[patrolIdx].x-worldX)*3.0f*dt;
            worldY += (patrol[patrolIdx].y-worldY)*3.0f*dt;
            return;
        }
        float tx=patrol[patrolIdx].x, ty=patrol[patrolIdx].y;
        float dx=tx-worldX, dy=ty-worldY, dist=lenf(dx,dy);
        if (dist < 6.0f) {
            patrolWaitT = PATROL_WAIT + randf(-0.3f, 0.5f);
            pickNextPatrol();
        } else {
            float mv = std::min(PATROL_SPEED*dt, dist);
            worldX += (dx/dist)*mv; worldY += (dy/dist)*mv;
        }
    }

    void moveOrbit(float dt, float px, float py) {
        orbitAng += ORBIT_SPEED * dt;
        float tx = px + std::cos(orbitAng)*ORBIT_RADIUS;
        float ty = py + std::sin(orbitAng)*ORBIT_RADIUS;
        worldX += (tx-worldX)*6.0f*dt;
        worldY += (ty-worldY)*6.0f*dt;
    }

    // ─────────────────────────────────────────────────────────
    // 렌더 — 이펙트 (스캔 빔 + 반사탄 + 노드 + 스파크)
    // ─────────────────────────────────────────────────────────
    void renderFx(float t) const {
        glm::vec3 col = phaseColor();

        // ── 반사탄 ──────────────────────────────────────────
        for (const auto& bb : bounceBullets) {
            if (!bb.alive) continue;
            float a = clampf(bb.life / BOUNCE_LIFE, 0.0f, 1.0f);
            float pulse2 = 0.6f + 0.4f * std::sin(t * 18.0f + bb.x * 0.01f);
            drawCircle(bb.x, bb.y, BOUNCE_R * pulse2,  1.0f, 0.18f, 0.22f, a * 0.9f);
            drawCircle(bb.x, bb.y, BOUNCE_R * 0.5f,   1.0f, 0.88f, 0.92f, a * 0.75f);
        }

        // ── 잠금 노드 (lastPx/Py/WinHalf 사용) ──────────────
        for (int i=0;i<4;i++) {
            const auto& nd = nodes[i];
            if (!nd.alive) continue;
            float nx = lastPx + nd.cx * lastWinHalf;
            float ny = lastPy + nd.cy * lastWinHalf;
            float lifeFrac = nd.life / NODE_LIFE;
            float flicker = 0.55f + 0.45f * std::sin(t * 12.0f + (float)i * 1.57f);
            float urgency = 1.0f - lifeFrac;            // 0 = fresh, 1 = about to expire
            float nr = 0.22f + urgency * 0.78f;
            float ng = 0.85f - urgency * 0.67f;
            float nb = 0.48f - urgency * 0.30f;
            float na = 0.75f + flicker * 0.15f;
            // 외곽 글로우
            drawCircle(nx, ny, 18.0f * flicker, nr, ng, nb, 0.20f);
            // 다이아몬드
            drawRotRect(nx, ny, 14.0f, 14.0f, PI*0.25f, nr, ng, nb, na);
            drawRectFrame(nx, ny, 8.0f, 8.0f, PI*0.25f, 1.0f, 1.0f, 1.0f, 0.55f, 1.5f);
        }

        // ── 스파크 ──────────────────────────────────────────
        for (const auto& s : sparks) {
            float a = clampf(s.life / 0.40f, 0.0f, 1.0f);
            float sz = 5.0f + 6.0f * a;
            drawRotRect(s.x, s.y, sz, sz, t*4.5f+s.x*0.012f, col.r,col.g,col.b, a*0.85f);
        }
    }

    // ─────────────────────────────────────────────────────────
    // 렌더 — 본체
    // ─────────────────────────────────────────────────────────
    void renderBody(float t) const {
        glm::vec3 col = phaseColor();
        float pulse = 0.5f + 0.5f * std::sin(t * (phase3 ? 11.0f : 6.5f));
        float flash = clampf(flashT / 0.16f, 0.0f, 1.0f);

        float gx = 0.0f, gy = 0.0f;
        if (phase3) {
            gx = std::sin(glitchT*31.0f)*5.0f;
            gy = std::cos(glitchT*23.0f)*3.5f;
        }
        float cx = worldX+gx, cy = worldY+gy;

        // 외곽 글로우
        float glowR = RING_W[0] * (1.1f + pulse * 0.08f);
        drawRect(cx-glowR, cy-glowR, glowR*2.0f, glowR*2.0f,
                 col.r*0.12f, col.g*0.12f, col.b*0.12f, 0.45f);

        // 링 2: 외곽
        {
            float tk = phase3 ? 4.5f : 3.0f;
            float al = 0.45f + pulse*0.12f;
            drawRectFrame(cx,cy, RING_W[0],RING_W[0], ringAng[2], col.r*0.6f,col.g*0.6f,col.b*0.6f, al, tk);
            for (int i=0;i<4;i++) {
                float ai = ringAng[2] + PI*0.5f*(float)i + PI*0.25f;
                float ex = cx + std::cos(ai)*RING_W[0]*1.41f;
                float ey = cy + std::sin(ai)*RING_W[0]*1.41f;
                drawRotRect(ex,ey, 9.0f,9.0f, ringAng[2]+PI*0.25f, col.r,col.g,col.b, 0.60f+pulse*0.20f);
            }
        }
        // 링 1: 중간
        {
            float tk = phase3 ? 5.0f : 3.5f;
            drawRectFrame(cx,cy, RING_W[1],RING_W[1], ringAng[1], col.r*0.80f,col.g*0.80f,col.b*0.80f, 0.60f+pulse*0.15f, tk);
            for (int i=0;i<4;i++) {
                float ai = ringAng[1] + PI*0.5f*(float)i;
                float ex = cx + std::cos(ai)*RING_W[1], ey = cy + std::sin(ai)*RING_W[1];
                drawRotRect(ex,ey, 12.0f,5.0f, ringAng[1], col.r,col.g,col.b, 0.70f);
            }
        }
        // 링 0: 내부
        {
            float tk = phase3?6.0f:(phase2?4.5f:3.5f);
            drawRectFrame(cx,cy, RING_W[2],RING_W[2], ringAng[0], col.r,col.g,col.b, 0.75f+pulse*0.18f, tk);
        }

        // 코어 패널
        drawRotRect(cx,cy, CORE_HALF*2.0f,CORE_HALF*2.0f, 0.0f, 0.04f,0.05f,0.06f, 0.96f);
        drawRectFrame(cx,cy, CORE_HALF,CORE_HALF, 0.0f, col.r,col.g,col.b, 0.85f+flash*0.15f, 2.0f);
        float tbH = 8.0f;
        drawRect(cx-CORE_HALF, cy-CORE_HALF-tbH, CORE_HALF*2.0f, tbH, col.r*0.9f,col.g*0.9f,col.b*0.9f, 0.80f);
        if (flash > 0.0f)
            drawRotRect(cx,cy, CORE_HALF*2.2f,CORE_HALF*2.2f, 0.0f, 1.0f,1.0f,1.0f, flash*0.55f);

        // 자물쇠 아이콘
        drawLockIcon(cx, cy+4.0f, col, pulse);

        // 상태 태그
        const wchar_t* tag = stateTag();
        float tw = g_TextS.Width(tag, 0.46f);
        g_TextS.Draw(tag, cx-tw*0.5f, cy+CORE_HALF+14.0f, 0.46f, col.r,col.g,col.b, 0.90f);

        // HP 바
        {
            float bw=CORE_HALF*2.4f, bh=4.0f;
            float bx=cx-bw*0.5f, by=cy+CORE_HALF+36.0f;
            float frac=clampf(hp/maxHp, 0.0f, 1.0f);
            drawRect(bx, by, bw, bh, 0.12f,0.12f,0.14f, 0.90f);
            drawRect(bx, by, bw*frac, bh, col.r,col.g,col.b, 0.95f);
        }
    }

private:
    void drawLockIcon(float cx, float cy, glm::vec3 col, float pulse) const {
        float al = 0.80f + pulse*0.14f;
        float bw=18.0f, bh=14.0f;
        drawRotRect(cx, cy+6.0f, bw, bh, 0.0f, col.r,col.g,col.b, al);
        float shW=11.0f, shThick=3.5f;
        float lx=cx-shW*0.5f, rx=cx+shW*0.5f, top=cy-4.0f, bot=cy;
        drawLine(lx,bot,lx,top, shThick, col.r,col.g,col.b, al);
        drawLine(rx,bot,rx,top, shThick, col.r,col.g,col.b, al);
        drawLine(lx,top,rx,top, shThick, col.r,col.g,col.b, al);
        drawRotRect(cx, cy+6.0f, 5.5f,7.0f, 0.0f, 0.04f,0.05f,0.06f, 0.92f);
    }
};
