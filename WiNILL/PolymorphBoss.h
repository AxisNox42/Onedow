#pragma once
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#include "Bullet.h"

// ─────────────────────────────────────────────────────────────
// 폴리모프 보스 (보라색 · 큼 · 희귀 등장) — 폼 변환형
//   폼: TRIANGLE / RHOMBUS / DIAMOND 를 일정 시간마다 무작위 전환
//     TRIANGLE : 화면 한쪽 끝 → 반대쪽 끝으로 가로지르는 '고속 세모 쇄도'
//                먼저 시작 모서리에 경고 표시(텔레그래프) → 이후 3초간 매우 빠른
//                세모들이 줄줄이 쏟아져 구역을 가득 채우며 이동 (잡으면 EXP, 추적 X)
//     RHOMBUS  : 랜덤 방향 레이저 — 먼저 경고선 표시 후 발사. 주변 어두운 시야 밴드.
//     DIAMOND  : 총알 반사(무적). 반사탄은 플레이어 원래 위력 그대로.
//   페이즈2 (HP 50% 이하): 화면 2배 확장(main 이 g_ViewZoom 처리) + 폼 강화 +
//     주변 차크람 3개(각 1000HP, 제거 전까지 본체 무적)
// ─────────────────────────────────────────────────────────────

enum class PForm { TRIANGLE, RHOMBUS, DIAMOND };

struct PSwarm  { float x, y, vx, vy; float life; bool alive; };
struct PChakram{ float angle; float hp; bool alive; };

class PolymorphBoss {
public:
    float worldX, worldY;
    float hp, maxHp;
    bool  alive = true, exploded = false;
    int   screenW, screenH;

    PForm form = PForm::TRIANGLE;
    float formTimer = 0.0f;
    float formDuration = 6.0f;
    bool  phase2 = false;

    // wander
    float targetX, targetY, wanderTimer = 0.0f;

    // TRIANGLE — 끝에서 끝으로 가로지르는 고속 쇄도
    std::vector<PSwarm> swarm;
    bool  triWarn   = false;   // 경고(텔레그래프) 중
    bool  triActive = false;   // 쇄도 진행 중
    float triWarnTimer = 0.0f;
    float triTimer     = 0.0f; // 쇄도 경과 (3초)
    float triCd        = 0.4f; // 쇄도 사이 쿨다운
    float triSpawnAccum = 0.0f;
    float triDirX = 1.0f, triDirY = 0.0f;  // 진행 방향 (상하/좌우)

    // RHOMBUS (laser)
    bool  laserActive = false;
    bool  laserWarn   = false;   // 발사 전 경고선
    float laserWarnTimer = 0.0f;
    float laserX = 0, laserY = 0, laserDirX = 1, laserDirY = 0;
    float laserTimer = 0.0f, laserCd = 0.0f;
    float laserHalf = 20.0f;     // 시야 밴드 반폭 (페이즈2 +10)

    // CHAKRAM (phase2 방어막)
    std::vector<PChakram> chakrams;

    static constexpr float BODY = 105.0f;         // 큼 (1.5배)
    static constexpr float SWARM_DMG = 4.0f;      // 세모 접촉(잡몹 절반급)
    static constexpr float LASER_DPS = 12.0f;     // 루시드식: 자주 쏘되 딜은 아주 낮게

    PolymorphBoss(int sw, int sh, float hpInit) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.5f;  worldY = sh * 0.30f;
        targetX = worldX; targetY = worldY;
        pickForm(true);
    }

    // ── 상태 질의 ──
    bool shielded()   const { return phase2 && !chakrams.empty(); } // 차크람 남으면 무적
    bool reflecting() const { return form == PForm::DIAMOND; }       // 반사
    bool damageable() const { return !reflecting() && !shielded(); }

    void pickNewTarget() {
        float m = 160.0f;
        targetX = m + (float)(rand() % std::max(1, screenW - 2*(int)m));
        targetY = m + (float)(rand() % std::max(1, screenH - 2*(int)m));
    }

    void pickForm(bool first) {
        PForm prev = form;
        do { form = (PForm)(rand() % 3); } while (!first && form == prev);
        formTimer = 0.0f;
        // 폼 상태 초기화
        triWarn = triActive = false;
        triCd = 0.4f; triSpawnAccum = 0.0f; triTimer = 0.0f;
        laserActive = laserWarn = false;
        laserCd = 0.6f;
    }

    void enterPhase2() {
        phase2 = true;
        laserHalf = 30.0f;                 // 마름모 범위 +10
        formDuration = 5.0f;               // 폼 재사용 쿨 감소
        chakrams.clear();
        for (int i = 0; i < 3; i++) {
            PChakram c; c.angle = (float)i / 3.0f * 6.2831853f;
            c.hp = 1000.0f; c.alive = true;
            chakrams.push_back(c);
        }
    }

    // ── TRIANGLE: 끝에서 끝으로 가로지르는 쇄도 ──
    void startTriWarn() {
        triWarn = true;
        triWarnTimer = phase2 ? 0.8f : 1.0f;   // 경고 시간
        switch (rand() % 4) {
        case 0: triDirX =  1; triDirY =  0; break;   // 좌 → 우
        case 1: triDirX = -1; triDirY =  0; break;   // 우 → 좌
        case 2: triDirX =  0; triDirY =  1; break;   // 상 → 하
        default:triDirX =  0; triDirY = -1; break;   // 하 → 상
        }
    }

    void spawnTriRow() {
        // 시작 모서리 전체에 걸쳐 무작위로 한 줄 분량 spawn (고속, 직선 이동)
        // 페이즈2: 화면이 2배로 확장되므로 확장된 영역 전체를 덮도록 spawn 범위/수량 증가
        float sp = (phase2 ? 920.0f : 800.0f) + (float)(rand() % 160);  // 개빠름
        int   perRow = phase2 ? 6 : 3;
        float life   = phase2 ? 5.5f : 3.5f;          // 더 넓은 영역 가로질러야 함
        float exX = phase2 ? (float)screenW * 0.5f : 0.0f;   // 각 변 확장량
        float exY = phase2 ? (float)screenH * 0.5f : 0.0f;
        float minX = -exX, maxX = (float)screenW + exX;
        float minY = -exY, maxY = (float)screenH + exY;
        int   spanX = std::max(1, (int)(maxX - minX));
        int   spanY = std::max(1, (int)(maxY - minY));
        for (int i = 0; i < perRow; i++) {
            PSwarm s; s.life = life; s.alive = true;
            if (triDirX != 0.0f) {                    // 가로 이동
                s.x  = (triDirX > 0) ? minX - 20.0f : maxX + 20.0f;
                s.y  = minY + (float)(rand() % spanY);
                s.vx = triDirX * sp; s.vy = 0.0f;
            } else {                                  // 세로 이동
                s.x  = minX + (float)(rand() % spanX);
                s.y  = (triDirY > 0) ? minY - 20.0f : maxY + 20.0f;
                s.vx = 0.0f; s.vy = triDirY * sp;
            }
            swarm.push_back(s);
        }
    }

    // ── RHOMBUS: 경고선 조준만(발사 X) ──
    void aimLaser(float px, float py) {
        float m = 20.0f;
        switch (rand() % 4) {
        case 0: laserX = (float)(rand()%screenW); laserY = -m;          break;
        case 1: laserX = (float)(rand()%screenW); laserY = screenH + m; break;
        case 2: laserX = -m;            laserY = (float)(rand()%screenH);break;
        default:laserX = screenW + m;   laserY = (float)(rand()%screenH);break;
        }
        float dx = px - laserX, dy = py - laserY;
        float d  = std::sqrt(dx*dx + dy*dy) + 1e-3f;
        laserDirX = dx/d; laserDirY = dy/d;
    }

    static float segDist(float px, float py, float ax, float ay, float bx, float by) {
        float abx=bx-ax, aby=by-ay, l2=abx*abx+aby*aby, t=0.0f;
        if (l2>1e-6f){ t=((px-ax)*abx+(py-ay)*aby)/l2; t=t<0?0:(t>1?1:t);}
        float cx=ax+abx*t, cy=ay+aby*t, dx=px-cx, dy=py-cy;
        return std::sqrt(dx*dx+dy*dy);
    }

    void Update(float px, float py, float dt, float& playerHP,
                std::vector<Bullet>& bullets) {
        if (!alive) return;

        // 페이즈2 진입
        if (!phase2 && hp <= maxHp * 0.5f) enterPhase2();

        // 본체 wander
        wanderTimer += dt;
        if (wanderTimer >= 2.5f) { pickNewTarget(); wanderTimer = 0.0f; }
        worldX += (targetX - worldX) * 1.2f * dt;
        worldY += (targetY - worldY) * 1.2f * dt;

        // 차크람 공전
        for (auto& c : chakrams) { if (c.alive) c.angle += 2.0f * dt; }
        chakrams.erase(std::remove_if(chakrams.begin(), chakrams.end(),
            [](const PChakram& c){ return !c.alive; }), chakrams.end());

        // 폼 전환 (쇄도/레이저 진행 중에는 끊지 않음)
        formTimer += dt;
        if (formTimer >= formDuration && !triActive && !triWarn &&
            !laserActive && !laserWarn)
            pickForm(false);

        // ── 폼별 행동 ──
        switch (form) {
        case PForm::TRIANGLE: {
            if (!triActive && !triWarn) {
                triCd -= dt;
                if (triCd <= 0.0f) startTriWarn();
            }
            if (triWarn) {
                triWarnTimer -= dt;
                if (triWarnTimer <= 0.0f) {
                    triWarn = false; triActive = true;
                    triTimer = 0.0f; triSpawnAccum = 0.0f;
                }
            }
            if (triActive) {
                triTimer += dt;
                triSpawnAccum += dt;
                float rowInterval = phase2 ? 0.09f : 0.13f;
                if (triSpawnAccum >= rowInterval) {
                    triSpawnAccum = 0.0f;
                    spawnTriRow();
                }
                if (triTimer >= 3.0f) {           // 3초간 쇄도
                    triActive = false;
                    triCd = phase2 ? 1.0f : 1.8f;
                }
            }
            break;
        }
        case PForm::RHOMBUS: {
            // 루시드식: 짧은 경고 → 짧은 빔 → 짧은 쿨 을 빠르게 반복
            if (!laserActive && !laserWarn) {
                laserCd -= dt;
                if (laserCd <= 0.0f) {
                    aimLaser(px, py);
                    laserWarn = true;
                    laserWarnTimer = phase2 ? 0.35f : 0.45f;
                }
            }
            if (laserWarn) {
                laserWarnTimer -= dt;
                if (laserWarnTimer <= 0.0f) {
                    laserWarn = false; laserActive = true; laserTimer = 0.0f;
                }
            }
            if (laserActive) {
                laserTimer += dt;
                float ex = laserX + laserDirX * (float)(screenW + screenH);
                float ey = laserY + laserDirY * (float)(screenW + screenH);
                if (segDist(px, py, laserX, laserY, ex, ey) < 14.0f)
                    playerHP -= LASER_DPS * dt;
                if (laserTimer >= 0.7f) {            // 빔 지속 짧게
                    laserActive = false;
                    laserCd = phase2 ? 0.35f : 0.6f;  // 다음 빔까지 쿨 짧게
                }
            }
            break;
        }
        case PForm::DIAMOND:
            break;  // 반사는 main 충돌에서 처리
        }

        // 세모 무리 (직선 이동, 추적 안 함)
        for (auto& s : swarm) {
            if (!s.alive) continue;
            s.x += s.vx * dt; s.y += s.vy * dt;
            s.life -= dt;
            float dx = px - s.x, dy = py - s.y;
            if (dx*dx + dy*dy < 15.0f*15.0f) { playerHP -= SWARM_DMG; s.alive = false; }
            // 페이즈2 확장 영역까지 가로질러야 하므로 despawn 경계도 확장
            float mxB = (phase2 ? (float)screenW * 0.5f : 0.0f) + 160.0f;
            float myB = (phase2 ? (float)screenH * 0.5f : 0.0f) + 160.0f;
            if (s.life <= 0.0f ||
                s.x < -mxB || s.x > screenW + mxB ||
                s.y < -myB || s.y > screenH + myB)
                s.alive = false;
        }
        swarm.erase(std::remove_if(swarm.begin(), swarm.end(),
            [](const PSwarm& s){ return !s.alive; }), swarm.end());
    }
};
