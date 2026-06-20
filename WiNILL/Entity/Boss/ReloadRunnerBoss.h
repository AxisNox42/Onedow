#pragma once
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <glm/glm.hpp>
#include "Bullet.h"
#include "DrawPrim.h"
#include "TextRenderer.h"

extern TextRenderer g_TextS;

// ─────────────────────────────────────────────────────────────
// RELOADER.exe — 기동 화력 플랫폼 (전면전 보스)
//   3종 무기 로테이션 + 장전 질주 + 화면 가장자리 포격 + ASSAULT 돌격
//   · 근접(110px 이내): 탄막·ASSAULT·살보 OFF → 도주·장전 = 딜 타임
//   · 중거리~(210px+): 전면전 화력 유지
// ─────────────────────────────────────────────────────────────

enum class RRWeapon { SHOTGUN, SNIPER, MACHINEGUN };
enum class RRState  { ACTIVE, RELOAD_SPRINT, ASSAULT };

class ReloadRunnerBoss {
public:
    float worldX, worldY;
    float hp, maxHp;
    bool  alive = true;
    bool  exploded = false;
    int   screenW, screenH;

    RRState  state  = RRState::ACTIVE;
    RRWeapon weapon = RRWeapon::SHOTGUN;

    float moveSpeed = 295.0f;
    int   ammo = 4;

    float fireTimer   = 0.0f;
    float stopTimer   = 0.0f;
    float warmUpTimer = 0.0f;
    float reloadTimer = 0.0f;
    float aimDelay    = 0.0f;
    float assaultT    = 0.0f;
    float assaultCd   = 8.0f;
    float salvoCd     = 4.5f;
    float strafeT     = 0.0f;
    float strafeSign  = 1.0f;
    float spinAng     = 0.0f;

    bool  aiming       = false;
    bool  mgTelegraph  = false;
    bool  mgFiring     = false;
    bool  mgZoneLocked = false;
    float zoneDirX = 1.0f, zoneDirY = 0.0f;
    float zoneHalfAngle = 0.52f;
    float zoneLen = 1100.0f;

    bool  phase2 = false;
    bool  phase3 = false;
    float spamTimer = 0.0f;

    float sprintFireTimer = 0.0f;
    struct Trail { float x, y, life; };
    std::vector<Trail> trails;

    static constexpr float BODY = 48.0f;
    static constexpr float MELEE_NEAR  = 112.0f;   // 붙으면 화력 OFF, 도주·장전
    static constexpr float MELEE_MID   = 215.0f;   // 이 안쪽이면 살보·스팸·ASSAULT 제한
    static constexpr float PANIC_DIST  = 88.0f;    // 강제 패닉 장전
    static constexpr float SG_FLEE_DIST  = 155.0f;   // SG 근접 시 후퇴 구간

    static constexpr float SG_RANGE    = 500.0f;
    static constexpr float SG_INTERVAL = 0.20f;
    static constexpr int   SG_AMMO     = 6;
    static constexpr int   SG_PELLETS  = 9;
    static constexpr float SG_SPREAD   = 0.72f;
    static constexpr float SG_BSPEED   = 680.0f;
    static constexpr float SG_DMG      = 11.0f;

    static constexpr float SN_KITE_RANGE = 480.0f;
    static constexpr float SN_AIM_DELAY  = 0.38f;
    static constexpr float SN_FREEZE     = 0.48f;
    static constexpr float SN_BSPEED     = 1900.0f;
    static constexpr float SN_DMG        = 34.0f;

    static constexpr float MG_WARMUP   = 1.35f;
    static constexpr int   MG_AMMO     = 52;
    static constexpr float MG_INTERVAL = 0.038f;
    static constexpr float MG_BSPEED   = 620.0f;
    static constexpr float MG_DMG      = 8.5f;
    static constexpr float MG_ZONE_ROT = 0.55f;

    static constexpr float RELOAD_TIME   = 0.58f;
    static constexpr float SPRINT_MULT = 4.2f;
    static constexpr float SPRINT_FIRE_INT = 0.14f;
    static constexpr float SPRINT_BSPEED   = 560.0f;

    static constexpr float SPAM_INT    = 0.85f;
    static constexpr int   SPAM_N      = 18;
    static constexpr float SPAM_BSPEED = 400.0f;

    static constexpr float ASSAULT_WARM = 0.72f;   // 붉은 예고 — 이 동안 사격 없음
    static constexpr float ASSAULT_DUR  = 1.25f;   // 예고 포함 총 길이 (사격 ~0.5초)
    static constexpr float ASSAULT_CD   = 15.0f;
    static constexpr float ASSAULT_MIN  = 255.0f;  // 이 거리 밖에서만 발동
    static constexpr float ASSAULT_MAX  = 460.0f;
    static constexpr float ASSAULT_FIRE = 0.40f;     // 사격 간격
    static constexpr float SALVO_CD    = 4.0f;

    ReloadRunnerBoss(int sw, int sh, float hpInit)
        : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.5f;
        worldY = sh * 0.28f;
        equip(RRWeapon::SHOTGUN);
    }

    void equip(RRWeapon w) {
        weapon = w;
        state  = RRState::ACTIVE;
        fireTimer = stopTimer = warmUpTimer = aimDelay = 0.0f;
        aiming = mgTelegraph = mgFiring = mgZoneLocked = false;
        switch (w) {
        case RRWeapon::SHOTGUN:    ammo = SG_AMMO; break;
        case RRWeapon::SNIPER:     ammo = phase3 ? 2 : 1; break;
        case RRWeapon::MACHINEGUN: ammo = MG_AMMO; mgTelegraph = true; break;
        }
    }

    void enterReload(std::vector<Bullet>& bullets, bool panic = false) {
        int ringN = panic ? 8 : 14;
        float ringDmg = panic ? 5.5f : 7.5f;
        float ringSpd = panic ? 340.0f : 400.0f;
        for (int i = 0; i < ringN; i++) {
            float a = (float)i / (float)ringN * 6.2831853f;
            fireDir(bullets, cosf(a), sinf(a), ringSpd,
                    glm::vec3(1.0f, 0.55f, 0.15f), ringDmg);
        }
        state = RRState::RELOAD_SPRINT;
        reloadTimer = 0.0f;
        sprintFireTimer = 0.0f;
        aiming = mgTelegraph = mgFiring = false;
    }

    void clampToScreen() {
        float m = BODY + 8.0f;
        if (worldX < m) worldX = m; else if (worldX > screenW - m) worldX = screenW - m;
        if (worldY < m) worldY = m; else if (worldY > screenH - m) worldY = screenH - m;
    }

    void fireDir(std::vector<Bullet>& bullets, float dirX, float dirY,
                 float speed, glm::vec3 col, float dmg = 0.0f) {
        Bullet b(worldX, worldY, worldX + dirX * 100.0f, worldY + dirY * 100.0f);
        b.isEnemy = true;
        b.speed   = speed;
        b.color   = col;
        if (dmg > 0.0f) b.enemyDmg = dmg;
        bullets.push_back(b);
    }

    void fleeFrom(float nx, float ny, float dt, float mul = 1.0f) {
        worldX -= nx * moveSpeed * mul * dt;
        worldY -= ny * moveSpeed * mul * dt;
        clampToScreen();
    }

    void fireShotgun(std::vector<Bullet>& bullets, float nx, float ny, float dist) {
        float base = atan2f(ny, nx);
        int n = SG_PELLETS + (phase2 ? 2 : 0);
        if (dist < SG_FLEE_DIST) n = (n > 4) ? 4 : n;
        float spread = (dist < SG_FLEE_DIST) ? SG_SPREAD * 0.55f : SG_SPREAD;
        float dmg = (dist < MELEE_NEAR) ? SG_DMG * 0.65f : SG_DMG;
        for (int i = 0; i < n; i++) {
            float t = (n > 1) ? (float)i / (float)(n - 1) : 0.5f;
            float a = base + (t - 0.5f) * spread;
            fireDir(bullets, cosf(a), sinf(a), SG_BSPEED,
                    glm::vec3(1.0f, 0.48f, 0.12f), dmg);
        }
    }

    void fireAssaultBurst(std::vector<Bullet>& bullets, float nx, float ny) {
        float base = atan2f(ny, nx);
        int n = phase3 ? 5 : 4;
        float spread = 0.42f;
        float dmg = SG_DMG * 0.68f;
        for (int i = 0; i < n; i++) {
            float t = (n > 1) ? (float)i / (float)(n - 1) : 0.5f;
            float a = base + (t - 0.5f) * spread;
            fireDir(bullets, cosf(a), sinf(a), SG_BSPEED * 0.92f,
                    glm::vec3(1.0f, 0.35f, 0.12f), dmg);
        }
    }

    void fireSpamBurst(std::vector<Bullet>& bullets, float nx, float ny) {
        float off = (float)(rand() % 100) * 0.01f;
        int n = SPAM_N + (phase3 ? 6 : (phase2 ? 4 : 0));
        for (int i = 0; i < n; i++) {
            float a = (off + (float)i / (float)n) * 6.2831853f;
            fireDir(bullets, cosf(a), sinf(a), SPAM_BSPEED,
                    glm::vec3(1.0f, 0.72f, 0.18f), 9.0f);
        }
        for (int i = -2; i <= 2; i++) {
            float a = atan2f(ny, nx) + (float)i * 0.14f;
            fireDir(bullets, cosf(a), sinf(a), SPAM_BSPEED * 1.65f,
                    glm::vec3(1.0f, 0.38f, 0.12f), 11.0f);
        }
    }

    void fireEdgeSalvo(std::vector<Bullet>& bullets, float px, float py) {
        const float M = 24.0f;
        struct Spawn { float x, y; } pts[8] = {
            { M, py }, { screenW - M, py },
            { px, M }, { px, screenH - M },
            { M, M }, { screenW - M, M },
            { M, screenH - M }, { screenW - M, screenH - M }
        };
        for (int i = 0; i < 8; i++) {
            float dx = px - pts[i].x, dy = py - pts[i].y;
            float d = sqrtf(dx * dx + dy * dy) + 1e-3f;
            float sp = 520.0f + (float)(rand() % 80);
            fireDir(bullets, dx / d, dy / d, sp, glm::vec3(1.0f, 0.35f, 0.2f), 10.0f);
            if (phase3) {
                float a = atan2f(dy, dx) + 0.12f;
                Bullet b(pts[i].x, pts[i].y, pts[i].x + cosf(a) * 100.0f, pts[i].y + sinf(a) * 100.0f);
                b.isEnemy = true; b.speed = sp * 0.85f; b.color = glm::vec3(1.0f, 0.5f, 0.25f);
                b.enemyDmg = 8.0f;
                bullets.push_back(b);
            }
        }
    }

    void strafeMove(float nx, float ny, float dt) {
        strafeT -= dt;
        if (strafeT <= 0.0f) {
            strafeT = 0.55f + (float)(rand() % 40) * 0.01f;
            strafeSign = (rand() % 2) ? 1.0f : -1.0f;
        }
        float px = -ny * strafeSign, py = nx * strafeSign;
        worldX += (nx * 0.55f + px * 0.85f) * moveSpeed * dt;
        worldY += (ny * 0.55f + py * 0.85f) * moveSpeed * dt;
    }

    void pushTrail() {
        trails.push_back({ worldX, worldY, 0.28f });
        if ((int)trails.size() > 10) trails.erase(trails.begin());
    }

    void Update(float px, float py, float dt, float& playerHP,
                std::vector<Bullet>& bullets) {
        if (!alive) return;

        if (!phase2 && hp <= maxHp * 0.5f) phase2 = true;
        if (!phase3 && hp <= maxHp * 0.25f) phase3 = true;

        float dx = px - worldX, dy = py - worldY;
        float dist = sqrtf(dx * dx + dy * dy) + 1e-3f;
        float nx = dx / dist, ny = dy / dist;
        spinAng += dt * (phase3 ? 4.5f : (phase2 ? 3.0f : 2.0f));

        if (dist < BODY + 10.0f)
            playerHP -= (phase3 ? 12.0f : (phase2 ? 10.0f : 8.0f)) * dt;

        bool nearMelee = dist < MELEE_NEAR;
        bool midOrFar  = dist >= MELEE_MID;

        float spamInt = phase3 ? SPAM_INT * 0.65f : (phase2 ? SPAM_INT * 0.85f : SPAM_INT * 1.15f);
        if (nearMelee) spamInt *= 2.8f;
        else if (!midOrFar) spamInt *= 1.6f;
        spamTimer += dt;
        if (spamTimer >= spamInt && midOrFar && state != RRState::ASSAULT) {
            spamTimer = 0.0f;
            fireSpamBurst(bullets, nx, ny);
        }

        salvoCd -= dt;
        if (salvoCd <= 0.0f && midOrFar && state != RRState::ASSAULT) {
            salvoCd = phase3 ? SALVO_CD * 0.55f : (phase2 ? SALVO_CD * 0.75f : SALVO_CD);
            fireEdgeSalvo(bullets, px, py);
        }

        if (nearMelee && state == RRState::ACTIVE &&
            (weapon == RRWeapon::SNIPER || weapon == RRWeapon::MACHINEGUN)) {
            enterReload(bullets, true);
            return;
        }
        if (dist < PANIC_DIST && state == RRState::ACTIVE) {
            enterReload(bullets, true);
            return;
        }

        assaultCd -= dt;
        bool assaultRange = dist >= ASSAULT_MIN && dist <= ASSAULT_MAX;
        if (state == RRState::ACTIVE && assaultCd <= 0.0f && midOrFar && assaultRange) {
            state = RRState::ASSAULT;
            assaultT = 0.0f;
            fireTimer = 0.0f;
            float cd = ASSAULT_CD;
            if (phase2) cd *= 0.88f;
            if (phase3) cd *= 0.78f;
            assaultCd = cd;
        }

        if (state == RRState::ASSAULT) {
            if (nearMelee || dist < MELEE_MID) {
                state = RRState::ACTIVE;
                enterReload(bullets, true);
                return;
            }
            assaultT += dt;
            float strafeMul = (assaultT < ASSAULT_WARM) ? 0.45f : 0.7f;
            strafeT -= dt;
            if (strafeT <= 0.0f) {
                strafeT = 0.7f;
                strafeSign = (rand() % 2) ? 1.0f : -1.0f;
            }
            float px = -ny * strafeSign, py = nx * strafeSign;
            worldX += px * moveSpeed * strafeMul * dt;
            worldY += py * moveSpeed * strafeMul * dt;
            clampToScreen();

            if (assaultT >= ASSAULT_WARM) {
                fireTimer += dt;
                if (fireTimer >= ASSAULT_FIRE) {
                    fireTimer = 0.0f;
                    fireAssaultBurst(bullets, nx, ny);
                }
            }
            if (assaultT >= ASSAULT_DUR) {
                state = RRState::ACTIVE;
                enterReload(bullets, false);
            }
            return;
        }

        if (state == RRState::RELOAD_SPRINT) {
            pushTrail();
            float sp = moveSpeed * SPRINT_MULT;
            worldX -= nx * sp * dt;
            worldY -= ny * sp * dt;
            if (!nearMelee) strafeMove(nx, ny, dt * 0.6f);
            clampToScreen();
            sprintFireTimer += dt;
            float sfInt = nearMelee ? 0.32f : SPRINT_FIRE_INT;
            if (sprintFireTimer >= sfInt) {
                sprintFireTimer = 0.0f;
                float base = atan2f(ny, nx);
                int spread = nearMelee ? 2 : 3;
                for (int i = -spread; i <= spread; i++) {
                    float a = base + (float)i * 0.16f;
                    fireDir(bullets, cosf(a), sinf(a), SPRINT_BSPEED,
                            glm::vec3(1.0f, 0.42f, 0.28f), nearMelee ? 5.5f : 7.5f);
                }
            }
            reloadTimer += dt;
            float rt = phase3 ? RELOAD_TIME * 0.38f : (phase2 ? RELOAD_TIME * 0.48f : RELOAD_TIME);
            if (reloadTimer >= rt) {
                if (phase3 && (rand() % 100) < 35) {
                    state = RRState::ACTIVE;
                    weapon = (RRWeapon)(rand() % 3);
                    switch (weapon) {
                    case RRWeapon::SHOTGUN: ammo = 2; break;
                    case RRWeapon::SNIPER:  ammo = 1; break;
                    default: ammo = 12; mgFiring = true; mgTelegraph = false; break;
                    }
                    aiming = false;
                } else {
                    equip((RRWeapon)(rand() % 3));
                }
            }
            return;
        }

        switch (weapon) {
        case RRWeapon::SHOTGUN: {
            if (dist < SG_FLEE_DIST) {
                fleeFrom(nx, ny, dt, 1.35f);
            } else if (dist > SG_RANGE * 0.92f) {
                worldX += nx * moveSpeed * dt;
                worldY += ny * moveSpeed * dt;
                clampToScreen();
            } else {
                strafeMove(nx, ny, dt);
                clampToScreen();
            }
            if (dist < SG_RANGE && dist >= MELEE_NEAR * 0.85f) {
                fireTimer += dt;
                float interval = (dist < SG_FLEE_DIST) ? 0.36f : SG_INTERVAL;
                if (fireTimer >= interval) {
                    fireTimer = 0.0f;
                    fireShotgun(bullets, nx, ny, dist);
                    if (--ammo <= 0) enterReload(bullets, dist < SG_FLEE_DIST);
                }
            } else if (dist < MELEE_NEAR && ammo <= 1) {
                enterReload(bullets, true);
            }
            break;
        }
        case RRWeapon::SNIPER: {
            if (!aiming) {
                if (dist < SN_KITE_RANGE) {
                    worldX -= nx * moveSpeed * 1.15f * dt;
                    worldY -= ny * moveSpeed * 1.15f * dt;
                    strafeMove(nx, ny, dt * 0.5f);
                    clampToScreen();
                }
                aimDelay += dt;
                if (aimDelay >= SN_AIM_DELAY) { aiming = true; stopTimer = 0.0f; }
            } else {
                stopTimer += dt;
                if (stopTimer >= SN_FREEZE) {
                    fireDir(bullets, nx, ny, SN_BSPEED, glm::vec3(0.25f, 1.0f, 1.0f), SN_DMG);
                    if (phase2) {
                        float a = atan2f(ny, nx) + 0.09f;
                        fireDir(bullets, cosf(a), sinf(a), SN_BSPEED * 0.92f,
                                glm::vec3(0.35f, 0.85f, 1.0f), SN_DMG * 0.75f);
                    }
                    if (phase3) {
                        float a = atan2f(ny, nx) - 0.09f;
                        fireDir(bullets, cosf(a), sinf(a), SN_BSPEED * 0.88f,
                                glm::vec3(0.2f, 0.95f, 1.0f), SN_DMG * 0.65f);
                    }
                    aiming = false;
                    aimDelay = 0.0f;
                    if (--ammo <= 0) enterReload(bullets, false);
                }
            }
            break;
        }
        case RRWeapon::MACHINEGUN: {
            if (dist < SG_FLEE_DIST && (mgTelegraph || mgFiring)) {
                mgTelegraph = mgFiring = mgZoneLocked = false;
                enterReload(bullets, true);
                break;
            }
            if (mgTelegraph) {
                if (!mgZoneLocked) { zoneDirX = nx; zoneDirY = ny; mgZoneLocked = true; }
                warmUpTimer += dt;
                if (warmUpTimer >= MG_WARMUP) {
                    mgTelegraph = false;
                    mgFiring = true;
                    fireTimer = 0.0f;
                }
            } else if (mgFiring) {
                float rot = MG_ZONE_ROT * dt * (phase3 ? 1.9f : (phase2 ? 1.3f : 1.0f));
                float cs = cosf(rot), sn = sinf(rot);
                float zx = zoneDirX * cs - zoneDirY * sn;
                float zy = zoneDirX * sn + zoneDirY * cs;
                zoneDirX = zx; zoneDirY = zy;
                fireTimer += dt;
                if (fireTimer >= MG_INTERVAL) {
                    fireTimer = 0.0f;
                    float base = atan2f(zoneDirY, zoneDirX);
                    float r = (float)(rand() % 200 - 100) / 100.0f;
                    float a = base + r * zoneHalfAngle;
                    fireDir(bullets, cosf(a), sinf(a), MG_BSPEED,
                            glm::vec3(1.0f, 0.82f, 0.18f), MG_DMG);
                    if (phase2 && (rand() % 4) == 0) {
                        float a2 = base + r * zoneHalfAngle * 1.4f;
                        fireDir(bullets, cosf(a2), sinf(a2), MG_BSPEED * 0.9f,
                                glm::vec3(1.0f, 0.55f, 0.15f), MG_DMG * 0.7f);
                    }
                    if (--ammo <= 0) { mgFiring = false; enterReload(bullets, false); }
                }
            }
            break;
        }
        }

        for (auto& tr : trails) tr.life -= dt;
        trails.erase(std::remove_if(trails.begin(), trails.end(),
                     [](const Trail& t) { return t.life <= 0.0f; }), trails.end());
    }

    static const wchar_t* weaponTag(RRWeapon w) {
        switch (w) {
        case RRWeapon::SHOTGUN:    return L"SG";
        case RRWeapon::SNIPER:     return L"SR";
        case RRWeapon::MACHINEGUN: return L"MG";
        default: return L"??";
        }
    }

    void renderTelegraphs(float px, float py, float gt) const {
        if (aiming) {
            float adx = px - worldX, ady = py - worldY;
            float ad = sqrtf(adx * adx + ady * ady) + 1e-3f;
            float dxn = adx / ad, dyn = ady / ad;
            float ex = worldX + dxn * (float)(screenW + screenH);
            float ey = worldY + dyn * (float)(screenW + screenH);
            float blink = 0.55f + 0.4f * sinf(gt * 38.0f);
            const int SEG = 18;
            for (int s = 0; s < SEG; s++) {
                if ((s & 1) == 0) continue;
                float u0 = (float)s / (float)SEG, u1 = (float)(s + 1) / (float)SEG;
                float x0 = worldX + (ex - worldX) * u0, y0 = worldY + (ey - worldY) * u0;
                float x1 = worldX + (ex - worldX) * u1, y1 = worldY + (ey - worldY) * u1;
                float th = phase3 ? 4.0f : 3.0f;
                drawRect((x0 + x1) * 0.5f - th, (y0 + y1) * 0.5f - th,
                         th * 2, th * 2, 0.25f, 0.95f, 1.0f, blink);
            }
        }
        if (mgTelegraph) {
            float base = atan2f(zoneDirY, zoneDirX);
            float a0 = base - zoneHalfAngle, a1 = base + zoneHalfAngle;
            float prog = warmUpTimer / MG_WARMUP;
            float alpha = 0.14f + 0.38f * prog;
            const int SEG = 22;
            for (int s = 0; s < SEG; s++) {
                float aa = a0 + (a1 - a0) * (float)s / (float)SEG;
                float ab = a0 + (a1 - a0) * (float)(s + 1) / (float)SEG;
                float L = zoneLen;
                float mx = (worldX + cosf(aa) * L + worldX + cosf(ab) * L) * 0.5f;
                float my = (worldY + sinf(aa) * L + worldY + sinf(ab) * L) * 0.5f;
                drawRect(mx - 2, my - 2, 4, 4, 1.0f, 0.55f, 0.12f, alpha);
            }
            drawCircle(worldX, worldY, 18.0f + prog * 24.0f, 1.0f, 0.45f, 0.1f, 0.12f + prog * 0.15f);
        }
        if (state == RRState::ACTIVE && weapon == RRWeapon::SHOTGUN) {
            float adx = px - worldX, ady = py - worldY;
            float ad = sqrtf(adx * adx + ady * ady) + 1e-3f;
            if (ad < SG_RANGE) {
                float base = atan2f(ady, adx);
                float ha = SG_SPREAD * 0.5f;
                drawCircle(worldX + cosf(base) * SG_RANGE * 0.5f,
                           worldY + sinf(base) * SG_RANGE * 0.5f,
                           SG_RANGE * 0.35f, 1.0f, 0.42f, 0.12f, 0.06f);
            }
        }
        if (state == RRState::ASSAULT) {
            float prog = (ASSAULT_WARM > 0.0f) ? assaultT / ASSAULT_WARM : 1.0f;
            if (prog > 1.0f) prog = 1.0f;
            float pulse = 0.5f + 0.5f * sinf(gt * (assaultT < ASSAULT_WARM ? 18.0f : 10.0f));
            drawCircle(worldX, worldY, BODY * (1.35f + prog * 0.45f + pulse * 0.15f),
                       1.0f, 0.25f, 0.08f, 0.12f + prog * 0.2f);
            if (assaultT < ASSAULT_WARM) {
                wchar_t warn[] = L"ASSAULT —";
                float ww = g_TextS.Width(warn, 0.5f);
                g_TextS.Draw(warn, worldX - ww * 0.5f, worldY - BODY - 36.0f, 0.5f,
                             1.0f, 0.3f, 0.1f, 0.7f + prog * 0.25f);
            }
        }
    }

    void renderBody(float gt, float px, float py) const {
        float aim = atan2f(py - worldY, px - worldX);
        float pulse = 0.5f + 0.5f * sinf(gt * 5.0f);

        for (auto& tr : trails) {
            float a = (tr.life > 0.0f) ? tr.life / 0.28f : 0.0f;
            drawRect(tr.x - BODY * 0.4f, tr.y - BODY * 0.25f,
                     BODY * 0.8f, BODY * 0.5f, 1.0f, 0.55f, 0.15f, 0.14f * a);
        }

        if (phase3) {
            drawCircle(worldX, worldY, BODY * 1.55f, 1.0f, 0.15f, 0.08f, 0.12f + pulse * 0.1f);
        } else if (phase2) {
            drawCircle(worldX, worldY, BODY * 1.35f, 1.0f, 0.45f, 0.12f, 0.08f + pulse * 0.08f);
        }

        float cr, cg, cb;
        if (state == RRState::RELOAD_SPRINT)      { cr = 1.0f; cg = 0.88f; cb = 0.25f; }
        else if (state == RRState::ASSAULT)       { cr = 1.0f; cg = 0.28f; cb = 0.12f; }
        else if (weapon == RRWeapon::SHOTGUN)     { cr = 1.0f; cg = 0.48f; cb = 0.14f; }
        else if (weapon == RRWeapon::SNIPER)      { cr = 0.22f; cg = 0.92f; cb = 1.0f; }
        else                                      { cr = 1.0f; cg = 0.78f; cb = 0.18f; }

        for (int i = 0; i < 4; i++) {
            float a = spinAng * 0.5f + (float)i * 1.571f;
            float lx = worldX + cosf(a) * (BODY * 0.95f);
            float ly = worldY + sinf(a) * (BODY * 0.72f);
            drawRect(lx - 7, ly - 5, 14, 10, cr * 0.15f, cg * 0.15f, cb * 0.15f, 0.85f);
            drawRect(lx - 5, ly - 3, 10, 6, cr * 0.35f, cg * 0.35f, cb * 0.35f, 0.9f);
        }

        drawRect(worldX - BODY * 0.78f, worldY - BODY * 0.55f,
                 BODY * 1.56f, BODY * 1.1f, 0.07f, 0.06f, 0.08f, 0.94f);
        drawNeonBorder(worldX - BODY * 0.78f, worldY - BODY * 0.55f,
                       BODY * 1.56f, BODY * 1.1f, cr, cg, cb);

        for (int i = 0; i < 6; i++) {
            float a = -spinAng + (float)i * 1.047f;
            float rx = worldX + cosf(a) * (BODY * 0.42f);
            float ry = worldY + sinf(a) * (BODY * 0.32f);
            drawRect(rx - 3, ry - 3, 6, 6, cr, cg, cb, 0.45f + pulse * 0.35f);
        }

        float bx = worldX + cosf(aim) * (BODY * 0.55f);
        float by = worldY + sinf(aim) * (BODY * 0.55f);
        if (weapon == RRWeapon::SHOTGUN || state == RRState::ASSAULT) {
            drawRect(bx - 4, by - 3, 22, 6, 0.12f, 0.12f, 0.14f, 0.95f);
            drawRect(bx, by - 2, 18, 4, cr, cg, cb, 0.95f);
            drawRect(bx + 14, by - 5, 8, 10, cr * 0.8f, cg * 0.8f, cb * 0.8f, 0.85f);
        } else if (weapon == RRWeapon::SNIPER) {
            drawRect(bx - 2, by - 2, 32, 4, 0.1f, 0.12f, 0.14f, 0.95f);
            drawRect(bx, by - 1.5f, 28, 3, cr, cg, cb, 1.0f);
            drawCircle(bx + 26, by, 5.0f, cr, cg, cb, 0.7f);
        } else {
            drawRect(bx - 3, by - 4, 16, 8, 0.1f, 0.1f, 0.12f, 0.95f);
            for (int i = 0; i < 3; i++)
                drawRect(bx + (float)i * 5.0f, by - 2, 4, 4, cr, cg, cb, 0.85f);
        }

        drawCircle(worldX, worldY, BODY * 0.22f, 1.0f, 0.92f, 0.75f, 0.65f + pulse * 0.25f);

        if (state == RRState::RELOAD_SPRINT &&
            ((int)(gt * 6.0f) % 2 == 0)) {
            const wchar_t* rl = L"RELOAD>>";
            float rw = g_TextS.Width(rl, 0.55f);
            g_TextS.Draw(rl, worldX - rw * 0.5f, worldY - BODY - 38.0f, 0.55f,
                         1.0f, 0.85f, 0.25f, 0.95f);
        } else if (state != RRState::RELOAD_SPRINT) {
            wchar_t tag[24];
            float adx = px - worldX, ady = py - worldY;
            float ad = sqrtf(adx * adx + ady * ady);
            if (state == RRState::ASSAULT)
                swprintf_s(tag, L"ASSAULT");
            else if (ad < MELEE_NEAR)
                swprintf_s(tag, L"PANIC");
            else
                swprintf_s(tag, L"[%ls %d]", weaponTag(weapon), ammo);
            float tw = g_TextS.Width(tag, 0.48f);
            g_TextS.Draw(tag, worldX - tw * 0.5f, worldY - BODY - 34.0f, 0.48f,
                         cr, cg, cb, 0.88f);
        }
    }
};
