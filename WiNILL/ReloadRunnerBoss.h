#pragma once
#include <vector>
#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#include "Bullet.h"

// ─────────────────────────────────────────────────────────────
// 리로드 러너 (Reload_Runner.exe) — 원거리 변칙 보스
//   장착 무기에 따라 이동/사격 전조가 완전히 변하고,
//   탄창이 비면 초고속으로 질주하며 장전 → 무작위 무기 교체.
//
//   SHOTGUN    : 플레이어로 접근 + 부채꼴 산탄 (3발 소모)
//   SNIPER     : 거리 벌리며 도망 + 1초 정지 조준선 → 고속 저격 (1발)
//   MACHINEGUN : 2초 범위 예고 → 고정 자세로 예고 구역에 30발 난사
//   RELOAD_SPRINT: 어떤 무기든 탄 소진 시 진입. ×3.5 질주로 도주 + 1.5초 장전
// ─────────────────────────────────────────────────────────────

enum class RRWeapon { SHOTGUN, SNIPER, MACHINEGUN };
enum class RRState  { ACTIVE, RELOAD_SPRINT };

class ReloadRunnerBoss {
public:
    float worldX, worldY;
    float hp, maxHp;
    bool  alive = true;
    bool  exploded = false;
    int   screenW, screenH;

    RRState  state  = RRState::ACTIVE;
    RRWeapon weapon = RRWeapon::SHOTGUN;

    float moveSpeed = 140.0f;
    int   ammo = 3;

    // ── AI 타이머 ──
    float fireTimer   = 0.0f;   // 발사 간격
    float stopTimer   = 0.0f;   // SNIPER 정지 조준(1초)
    float warmUpTimer = 0.0f;   // MG 범위 예고(2초)
    float reloadTimer = 0.0f;   // 장전(1.5초)
    float aimDelay    = 0.0f;   // SNIPER 조준 시작 전 이동

    // ── 서브 상태 / 락 (렌더가 읽음) ──
    bool  aiming      = false;          // SNIPER 정지 조준 중 → main 이 조준선 렌더
    bool  mgTelegraph = false;          // MG 예고 중 → main 이 부채꼴 렌더
    bool  mgFiring    = false;          // MG 고정 난사 중
    bool  mgZoneLocked= false;
    float zoneDirX = 1.0f, zoneDirY = 0.0f;   // MG 고정 사격 방향(락)
    float zoneHalfAngle = 0.42f;              // 부채꼴 반각(rad)
    float zoneLen = 1000.0f;

    static constexpr float BODY = 42.0f;

    // 무기별 상수
    static constexpr float SG_RANGE    = 360.0f;
    static constexpr float SG_INTERVAL = 0.42f;
    static constexpr int   SG_AMMO     = 3;
    static constexpr int   SG_PELLETS  = 7;
    static constexpr float SG_SPREAD   = 0.62f;
    static constexpr float SG_BSPEED   = 640.0f;

    static constexpr float SN_KITE_RANGE = 560.0f;  // 이보다 가까우면 도망
    static constexpr float SN_AIM_DELAY  = 0.9f;    // 조준 시작 전 이동 시간
    static constexpr float SN_FREEZE     = 1.0f;    // 정지 조준 1초
    static constexpr float SN_BSPEED     = 1600.0f; // 고속 저격탄

    static constexpr float MG_WARMUP   = 2.0f;
    static constexpr int   MG_AMMO     = 30;
    static constexpr float MG_INTERVAL = 0.065f;
    static constexpr float MG_BSPEED   = 580.0f;

    static constexpr float RELOAD_TIME = 1.5f;
    static constexpr float SPRINT_MULT = 3.5f;

    ReloadRunnerBoss(int sw, int sh, float hpInit)
        : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.5f;  worldY = sh * 0.25f;
        equip(RRWeapon::SHOTGUN);
    }

    void equip(RRWeapon w) {
        weapon = w;
        state  = RRState::ACTIVE;
        fireTimer = stopTimer = warmUpTimer = aimDelay = 0.0f;
        aiming = mgTelegraph = mgFiring = mgZoneLocked = false;
        switch (w) {
        case RRWeapon::SHOTGUN:    ammo = SG_AMMO; break;
        case RRWeapon::SNIPER:     ammo = 1;       break;
        case RRWeapon::MACHINEGUN: ammo = MG_AMMO; mgTelegraph = true; break;
        }
    }

    void enterReload() {
        state = RRState::RELOAD_SPRINT;
        reloadTimer = 0.0f;
        aiming = mgTelegraph = mgFiring = false;
    }

    void clampToScreen() {
        float m = BODY;
        if (worldX < m) worldX = m;  else if (worldX > screenW - m) worldX = screenW - m;
        if (worldY < m) worldY = m;  else if (worldY > screenH - m) worldY = screenH - m;
    }

    void fireDir(std::vector<Bullet>& bullets, float dirX, float dirY,
                 float speed, glm::vec3 col) {
        Bullet b(worldX, worldY, worldX + dirX * 100.0f, worldY + dirY * 100.0f);
        b.isEnemy = true;
        b.speed   = speed;
        b.color   = col;
        bullets.push_back(b);
    }

    void Update(float px, float py, float dt, float& playerHP,
                std::vector<Bullet>& bullets) {
        if (!alive) return;

        float dx = px - worldX, dy = py - worldY;
        float dist = std::sqrt(dx*dx + dy*dy) + 1e-3f;
        float nx = dx / dist, ny = dy / dist;

        // 본체 접촉 약한 데미지
        if (dist < BODY) playerHP -= 12.0f * dt;

        // ── 장전 질주 ──
        if (state == RRState::RELOAD_SPRINT) {
            float sp = moveSpeed * SPRINT_MULT;
            worldX -= nx * sp * dt;       // 플레이어 반대 방향
            worldY -= ny * sp * dt;
            clampToScreen();
            reloadTimer += dt;
            if (reloadTimer >= RELOAD_TIME)
                equip((RRWeapon)(rand() % 3));   // 무작위 교체
            return;
        }

        // ── ACTIVE: 무기별 분기 ──
        switch (weapon) {

        case RRWeapon::SHOTGUN: {
            // 플레이어로 접근하며 무빙
            worldX += nx * moveSpeed * dt;
            worldY += ny * moveSpeed * dt;
            clampToScreen();
            if (dist < SG_RANGE) {
                fireTimer += dt;
                if (fireTimer >= SG_INTERVAL) {
                    fireTimer = 0.0f;
                    float base = atan2f(ny, nx);
                    for (int i = 0; i < SG_PELLETS; i++) {
                        float t = (SG_PELLETS > 1) ? (float)i / (SG_PELLETS - 1) : 0.5f;
                        float a = base + (t - 0.5f) * SG_SPREAD;
                        fireDir(bullets, cosf(a), sinf(a), SG_BSPEED,
                                glm::vec3(1.0f, 0.5f, 0.1f));
                    }
                    if (--ammo <= 0) enterReload();
                }
            }
            break;
        }

        case RRWeapon::SNIPER: {
            if (!aiming) {
                // 거리 벌리며 도망 + 조준 준비
                if (dist < SN_KITE_RANGE) {
                    worldX -= nx * moveSpeed * dt;
                    worldY -= ny * moveSpeed * dt;
                    clampToScreen();
                }
                aimDelay += dt;
                if (aimDelay >= SN_AIM_DELAY) { aiming = true; stopTimer = 0.0f; }
            } else {
                // 완전 정지 조준 1초 → 발사 (현재 플레이어 추적해 발사)
                stopTimer += dt;
                if (stopTimer >= SN_FREEZE) {
                    fireDir(bullets, nx, ny, SN_BSPEED, glm::vec3(0.3f, 1.0f, 1.0f));
                    aiming = false; aimDelay = 0.0f;
                    if (--ammo <= 0) enterReload();
                }
            }
            break;
        }

        case RRWeapon::MACHINEGUN: {
            if (mgTelegraph) {
                // 첫 프레임에 방향 락 (플레이어 방향) → 이후 정지 예고
                if (!mgZoneLocked) { zoneDirX = nx; zoneDirY = ny; mgZoneLocked = true; }
                warmUpTimer += dt;
                if (warmUpTimer >= MG_WARMUP) { mgTelegraph = false; mgFiring = true; fireTimer = 0.0f; }
            } else if (mgFiring) {
                // 고정 자세로 락된 부채꼴 구역에 난사 (플레이어 추적 X)
                fireTimer += dt;
                if (fireTimer >= MG_INTERVAL) {
                    fireTimer = 0.0f;
                    float base = atan2f(zoneDirY, zoneDirX);
                    float r    = (float)(rand() % 200 - 100) / 100.0f;  // -1..1
                    float a    = base + r * zoneHalfAngle;
                    fireDir(bullets, cosf(a), sinf(a), MG_BSPEED,
                            glm::vec3(1.0f, 0.85f, 0.2f));
                    if (--ammo <= 0) { mgFiring = false; enterReload(); }
                }
            }
            break;
        }
        }
    }
};
