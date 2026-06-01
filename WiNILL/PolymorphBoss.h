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
//     TRIANGLE : 맵 곳곳에 예고 마커 → 3초 뒤 한 방향으로 이동하는 세모 무리
//                (잡을 수 있음, 마리당 EXP, 플레이어를 쫓지 않음)
//     RHOMBUS  : 랜덤 방향에서 레이저(주변 어두운 시야 밴드) — 닿으면 피해
//     DIAMOND  : 총알 반사(무적). 반사탄은 플레이어 원래 위력 그대로 → 쏠지 고민
//   페이즈2 (HP 50% 이하): 화면 2배 확장(main 이 g_ViewZoom 처리) + 폼 강화 +
//     주변 차크람 3개(각 1000HP, 제거 전까지 본체 무적)
// ─────────────────────────────────────────────────────────────

enum class PForm { TRIANGLE, RHOMBUS, DIAMOND };

struct PMarker { float x, y, dirX, dirY; float timer; bool fired; };
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

    // TRIANGLE
    std::vector<PMarker> markers;
    std::vector<PSwarm>  swarm;
    float markerAccum = 0.0f;

    // RHOMBUS (laser)
    bool  laserActive = false;
    float laserX = 0, laserY = 0, laserDirX = 1, laserDirY = 0;
    float laserTimer = 0.0f, laserCd = 0.0f;
    float laserHalf = 20.0f;     // 시야 밴드 반폭 (페이즈2 +10)

    // CHAKRAM (phase2 방어막)
    std::vector<PChakram> chakrams;

    static constexpr float BODY = 70.0f;          // 큼
    static constexpr float SWARM_DMG = 4.0f;      // 세모 접촉(잡몹 절반급)
    static constexpr float LASER_DPS = 50.0f;

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
        markerAccum = 0.0f;
        laserCd = 0.6f;
        laserActive = false;
    }

    void enterPhase2() {
        phase2 = true;
        laserHalf = 30.0f;                 // 마름모 범위 +10
        formDuration = 5.0f;               // 삼각형 등 재사용 쿨 감소
        chakrams.clear();
        for (int i = 0; i < 3; i++) {
            PChakram c; c.angle = (float)i / 3.0f * 6.2831853f;
            c.hp = 1000.0f; c.alive = true;
            chakrams.push_back(c);
        }
    }

    void spawnMarker(float px, float py) {
        // 맵 임의 위치 + 임의 직선 방향
        PMarker m;
        float mar = 100.0f;
        m.x = mar + (float)(rand() % std::max(1, screenW - 2*(int)mar));
        m.y = mar + (float)(rand() % std::max(1, screenH - 2*(int)mar));
        float a = (float)(rand() % 628) * 0.01f;
        m.dirX = cosf(a); m.dirY = sinf(a);
        m.timer = 3.0f;   // 3초 뒤 발생
        m.fired = false;
        markers.push_back(m);
    }

    void fireSwarm(const PMarker& m) {
        // 마커 위치에서 한 방향으로 이동하는 세모 무리 (범위 = 마커 주변)
        int n = phase2 ? 14 : 9;
        float span = phase2 ? 220.0f : 150.0f;   // 페이즈2 범위 증가
        float perpX = -m.dirY, perpY = m.dirX;
        for (int i = 0; i < n; i++) {
            float t = (n > 1) ? ((float)i / (n - 1) - 0.5f) : 0.0f;
            PSwarm s;
            s.x = m.x + perpX * t * span;
            s.y = m.y + perpY * t * span;
            float sp = 360.0f + (float)(rand() % 120);
            s.vx = m.dirX * sp;  s.vy = m.dirY * sp;
            s.life = 4.0f; s.alive = true;
            swarm.push_back(s);
        }
    }

    void fireLaser(float px, float py) {
        // 랜덤 화면 가장자리에서 시작 → 플레이어 근처 가로지르는 직선
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
        laserActive = true; laserTimer = 0.0f;
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

        // 폼 전환
        formTimer += dt;
        if (formTimer >= formDuration) pickForm(false);

        // ── 폼별 행동 ──
        switch (form) {
        case PForm::TRIANGLE: {
            markerAccum += dt;
            float interval = phase2 ? 1.4f : 2.0f;
            if (markerAccum >= interval) {
                markerAccum = 0.0f;
                int cnt = phase2 ? 2 : 1;   // 페이즈2 한번에 2개
                for (int i = 0; i < cnt; i++) spawnMarker(px, py);
            }
            break;
        }
        case PForm::RHOMBUS: {
            laserCd -= dt;
            if (!laserActive && laserCd <= 0.0f) { fireLaser(px, py); laserCd = phase2 ? 1.4f : 2.0f; }
            if (laserActive) {
                laserTimer += dt;
                float ex = laserX + laserDirX * (float)(screenW + screenH);
                float ey = laserY + laserDirY * (float)(screenW + screenH);
                if (segDist(px, py, laserX, laserY, ex, ey) < 14.0f)
                    playerHP -= LASER_DPS * dt;
                if (laserTimer >= 1.4f) laserActive = false;
            }
            break;
        }
        case PForm::DIAMOND:
            break;  // 반사는 main 충돌에서 처리
        }

        // 마커 카운트다운 → 세모 무리
        for (auto& m : markers) {
            if (m.fired) continue;
            m.timer -= dt;
            if (m.timer <= 0.0f) { fireSwarm(m); m.fired = true; }
        }
        markers.erase(std::remove_if(markers.begin(), markers.end(),
            [](const PMarker& m){ return m.fired; }), markers.end());

        // 세모 무리 (직선 이동, 추적 안 함)
        for (auto& s : swarm) {
            if (!s.alive) continue;
            s.x += s.vx * dt; s.y += s.vy * dt;
            s.life -= dt;
            float dx = px - s.x, dy = py - s.y;
            if (dx*dx + dy*dy < 15.0f*15.0f) { playerHP -= SWARM_DMG; s.alive = false; }
            if (s.life <= 0.0f ||
                s.x < -120 || s.x > screenW+120 || s.y < -120 || s.y > screenH+120)
                s.alive = false;
        }
        swarm.erase(std::remove_if(swarm.begin(), swarm.end(),
            [](const PSwarm& s){ return !s.alive; }), swarm.end());
    }
};
