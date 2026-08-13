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
// VOLLEY.sys — 기동 화력 드론 (전면전 보스)
//   3종 무기 로테이션 + 장전 질주 + 화면 가장자리 포격 + ASSAULT 돌격
//   · 근접(110px 이내): 탄막·ASSAULT·살보 OFF → 도주·장전 = 딜 타임
//   · 중거리~(210px+): 전면전 화력 유지
// ─────────────────────────────────────────────────────────────

enum class RRWeapon { RIFLE, SNIPER, GRENADE, MACHINEGUN };
enum class RRState  { ACTIVE, RELOAD_STEP, RELOAD_SPRINT, ASSAULT, OVERHEAT };

class ReloadRunnerBoss {
public:
    float worldX, worldY;
    float hp, maxHp;
    bool  alive = true;
    bool  exploded = false;
    int   screenW, screenH;

    RRState  state  = RRState::ACTIVE;
    RRWeapon weapon = RRWeapon::SNIPER;

    float moveSpeed = 330.0f;
    int   ammo = 4;

    float fireTimer   = 0.0f;
    float stopTimer   = 0.0f;
    float warmUpTimer = 0.0f;
    float reloadTimer = 0.0f;
    float aimDelay    = 0.0f;
    float assaultT    = 0.0f;
    float assaultCd   = 8.0f;
    float salvoCd     = 4.5f;
    float panicCd     = 0.0f;
    float overheatT   = 0.0f;
    float curveCd     = 6.0f;
    float curveShotT  = 0.0f;
    int   curveShotsLeft = 0;
    float dashCd      = 1.2f;
    float dashT       = 0.0f;
    float dashVelX    = 0.0f;
    float dashVelY    = 0.0f;
    float glRecoilT   = 0.0f;
    float glRecoilDur = 0.0f;
    float glRecoilX   = 0.0f;
    float glRecoilY   = 0.0f;
    float glRecoilAppliedX = 0.0f;
    float glRecoilAppliedY = 0.0f;
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

    struct GLShell {
        float x, y;
        float prevX, prevY;
        float dirX, dirY;
        float speed;
        float age, life;
        float wavePhase;
        float waveAmp, waveFreq;
        float damage;
        float hitRadius;
        float bendX, bendY;
        float lastBendDist;
        bool  tracking;
    };
    std::vector<GLShell> glShells;
    struct GLPop {
        float x, y;
        float t, life;
    };
    std::vector<GLPop> glPops;

    static constexpr float BODY = 48.0f;
    static constexpr float MELEE_NEAR  = 112.0f;   // 붙으면 화력 OFF, 도주·장전
    static constexpr float MELEE_MID   = 215.0f;   // 이 안쪽이면 살보·스팸·ASSAULT 제한
    static constexpr float PANIC_DIST  = 88.0f;    // 강제 패닉 장전
    static constexpr float RF_MIN_DIST = 180.0f;
    static constexpr float RF_RANGE    = 620.0f;
    static constexpr float RF_INTERVAL = 0.125f;
    static constexpr int   RF_AMMO     = 6;
    static constexpr float RF_BSPEED   = 880.0f;
    static constexpr float RF_DMG      = 9.0f;
    static constexpr float RF_JITTER   = 0.035f;

    static constexpr float SN_KITE_RANGE = 480.0f;
    static constexpr float SN_AIM_DELAY  = 0.30f;
    static constexpr float SN_FREEZE     = 0.36f;
    static constexpr float SN_BSPEED     = 2050.0f;
    static constexpr float SN_DMG        = 36.0f;

    static constexpr float MG_WARMUP   = 1.35f;
    static constexpr int   MG_AMMO     = 42;
    static constexpr float MG_INTERVAL = 0.038f;
    static constexpr float MG_BSPEED   = 620.0f;
    static constexpr float MG_DMG      = 8.5f;
    static constexpr float MG_ZONE_ROT = 0.55f;

    static constexpr float RELOAD_TIME = 0.78f;
    static constexpr float OVERHEAT_TIME = 1.15f;
    static constexpr float SPRINT_MULT = 4.2f;
    static constexpr float SPRINT_FIRE_INT = 0.14f;
    static constexpr float SPRINT_BSPEED   = 560.0f;

    static constexpr int   GL_AMMO       = 3;
    static constexpr float GL_INTERVAL   = 0.58f;
    static constexpr float GL_BSPEED     = 275.0f;
    static constexpr float GL_ACCEL      = 245.0f;
    static constexpr float GL_DMG        = 23.0f;
    static constexpr float GL_HIT_RADIUS = 18.0f;
    static constexpr float GL_LIFE       = 4.75f;
    static constexpr float GL_WAVE_AMP   = 92.0f;
    static constexpr float GL_WAVE_FREQ  = 6.2f;
    static constexpr float GL_TURN_RATE  = 2.7f;
    static constexpr float GL_RECOIL     = 24.0f;
    static constexpr float GL_RECOIL_DUR = 0.18f;

    static constexpr float CURVE_CD       = 8.5f;
    static constexpr float CURVE_INTERVAL = 0.8f;
    static constexpr int   CURVE_COUNT    = 5;
    static constexpr float CURVE_BSPEED   = 430.0f;
    static constexpr float CURVE_TURN     = 0.85f;

    static constexpr float SPAM_INT    = 1.4f;
    static constexpr int   SPAM_N      = 13;
    static constexpr float SPAM_BSPEED = 400.0f;

    static constexpr float ASSAULT_WARM = 0.72f;   // 붉은 예고 — 이 동안 사격 없음
    static constexpr float ASSAULT_DUR  = 1.25f;   // 예고 포함 총 길이 (사격 ~0.5초)
    static constexpr float ASSAULT_CD   = 15.0f;
    static constexpr float ASSAULT_MIN  = 255.0f;  // 이 거리 밖에서만 발동
    static constexpr float ASSAULT_MAX  = 460.0f;
    static constexpr float ASSAULT_FIRE = 0.40f;     // 사격 간격
    static constexpr float SALVO_CD    = 7.0f;

    ReloadRunnerBoss(int sw, int sh, float hpInit)
        : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.5f;
        worldY = sh * 0.28f;
        equip(RRWeapon::RIFLE);
    }

    int rifleAmmo(Difficulty difficulty) const {
        if (difficulty == Difficulty::EASY) return 4;
        if (difficulty == Difficulty::HARD) return 7;
        return RF_AMMO;
    }

    int grenadeAmmo(Difficulty difficulty) const {
        if (difficulty == Difficulty::EASY) return 2;
        if (difficulty == Difficulty::HARD) return 4;
        return GL_AMMO;
    }

    int machinegunAmmo(Difficulty difficulty) const {
        if (difficulty == Difficulty::EASY) return 34;
        if (difficulty == Difficulty::HARD) return 48;
        return MG_AMMO;
    }

    RRWeapon pickNextWeapon(Difficulty difficulty) const {
        int r = rand() % 100;
        if (!phase2) {
            if (r < 52) return RRWeapon::RIFLE;
            if (r < 76) return RRWeapon::GRENADE;
            return RRWeapon::SNIPER;
        }
        if (!phase3) {
            if (r < 42) return RRWeapon::RIFLE;
            if (r < 72) return RRWeapon::GRENADE;
            return RRWeapon::SNIPER;
        }
        if (difficulty == Difficulty::EASY)
            return (r < 48) ? RRWeapon::MACHINEGUN : ((r < 80) ? RRWeapon::RIFLE : RRWeapon::GRENADE);
        if (r < 40) return RRWeapon::MACHINEGUN;
        if (r < 68) return RRWeapon::RIFLE;
        if (r < 90) return RRWeapon::GRENADE;
        return RRWeapon::SNIPER;
    }

    bool allowEdgeWarning(Difficulty difficulty) const {
        if (difficulty == Difficulty::EASY) return false;
        if (difficulty == Difficulty::NORMAL) return phase3;
        return phase2;
    }

    bool allowAssault(Difficulty difficulty) const {
        return difficulty == Difficulty::HARD && phase3;
    }

    void equip(RRWeapon w, Difficulty difficulty = Difficulty::NORMAL) {
        weapon = w;
        state  = RRState::ACTIVE;
        fireTimer = stopTimer = warmUpTimer = aimDelay = 0.0f;
        aiming = mgTelegraph = mgFiring = mgZoneLocked = false;
        switch (w) {
        case RRWeapon::RIFLE:      ammo = rifleAmmo(difficulty); break;
        case RRWeapon::SNIPER:     ammo = (difficulty == Difficulty::HARD && phase3) ? 2 : 1; break;
        case RRWeapon::GRENADE:    ammo = grenadeAmmo(difficulty); break;
        case RRWeapon::MACHINEGUN: ammo = machinegunAmmo(difficulty); mgTelegraph = true; break;
        }
    }

    void firePanicPulse(std::vector<Bullet>& bullets, Difficulty difficulty) {
        int ringN = (difficulty == Difficulty::EASY) ? 6 : ((difficulty == Difficulty::HARD) ? 10 : 8);
        float ringDmg = (difficulty == Difficulty::EASY) ? 4.5f : 5.5f;
        float ringSpd = (difficulty == Difficulty::EASY) ? 300.0f : 340.0f;
        for (int i = 0; i < ringN; i++) {
            float a = (float)i / (float)ringN * 6.2831853f;
            fireDir(bullets, cosf(a), sinf(a), ringSpd,
                    glm::vec3(0.55f, 1.0f, 0.9f), ringDmg);
        }
    }

    void enterReload(std::vector<Bullet>& bullets, bool panic = false,
                     Difficulty difficulty = Difficulty::NORMAL) {
        if (panic) firePanicPulse(bullets, difficulty);
        bool sprint = (difficulty == Difficulty::HARD && phase3 && !panic);
        state = sprint ? RRState::RELOAD_SPRINT : RRState::RELOAD_STEP;
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

    void fireRifleShot(std::vector<Bullet>& bullets, float nx, float ny,
                       Difficulty difficulty) {
        float base = atan2f(ny, nx);
        float jitter = ((float)(rand() % 200 - 100) / 100.0f) * RF_JITTER;
        float spd = RF_BSPEED * ((difficulty == Difficulty::HARD) ? 1.08f : 1.0f);
        float dmg = RF_DMG * ((difficulty == Difficulty::EASY) ? 0.82f : 1.0f);
        fireDir(bullets, cosf(base + jitter), sinf(base + jitter), spd,
                glm::vec3(0.6f, 1.0f, 0.35f), dmg);
    }

    void fireAssaultBurst(std::vector<Bullet>& bullets, float nx, float ny) {
        float base = atan2f(ny, nx);
        int n = phase3 ? 5 : 4;
        float spread = 0.42f;
        float dmg = RF_DMG * 0.72f;
        for (int i = 0; i < n; i++) {
            float t = (n > 1) ? (float)i / (float)(n - 1) : 0.5f;
            float a = base + (t - 0.5f) * spread;
            fireDir(bullets, cosf(a), sinf(a), RF_BSPEED * 0.86f,
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

    static float segDist(float ax, float ay, float bx, float by, float px, float py) {
        float vx = bx - ax, vy = by - ay;
        float wx = px - ax, wy = py - ay;
        float len2 = vx * vx + vy * vy;
        float t = (len2 > 1e-4f) ? (wx * vx + wy * vy) / len2 : 0.0f;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        float cx = ax + vx * t, cy = ay + vy * t;
        float dx = px - cx, dy = py - cy;
        return sqrtf(dx * dx + dy * dy);
    }

    static float easeOutCubic(float t) {
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        float inv = 1.0f - t;
        return 1.0f - inv * inv * inv;
    }

    void startGLRecoil(float dirX, float dirY, float dist) {
        glRecoilT = 0.0f;
        glRecoilDur = GL_RECOIL_DUR;
        glRecoilX = -dirX * dist;
        glRecoilY = -dirY * dist;
        glRecoilAppliedX = 0.0f;
        glRecoilAppliedY = 0.0f;
        pushTrail();
    }

    void tickGLRecoil(float dt) {
        if (glRecoilDur <= 0.0f || glRecoilT >= glRecoilDur) return;
        glRecoilT += dt;
        float u = glRecoilT / glRecoilDur;
        float e = easeOutCubic(u);
        float targetX = glRecoilX * e;
        float targetY = glRecoilY * e;
        worldX += targetX - glRecoilAppliedX;
        worldY += targetY - glRecoilAppliedY;
        glRecoilAppliedX = targetX;
        glRecoilAppliedY = targetY;
        if (glRecoilT >= glRecoilDur) {
            glRecoilT = glRecoilDur;
            glRecoilDur = 0.0f;
        }
        clampToScreen();
    }

    void spawnGLPop(float x, float y) {
        glPops.push_back({ x, y, 0.0f, 0.24f });
        if ((int)glPops.size() > 8) glPops.erase(glPops.begin());
    }

    void fireGrenadeLauncher(float nx, float ny, float px, float py,
                             Difficulty difficulty, bool panicShot = false) {
        float side = (rand() % 2) ? 1.0f : -1.0f;
        float sx = worldX + nx * BODY * 0.62f + (-ny * side) * BODY * 0.45f;
        float sy = worldY + ny * BODY * 0.62f + ( nx * side) * BODY * 0.45f;
        float jitter = ((float)(rand() % 200 - 100) / 100.0f) * 0.07f;
        float base = atan2f(ny, nx) + jitter;

        float speed = GL_BSPEED;
        float damage = GL_DMG;
        float amp = GL_WAVE_AMP;
        float freq = GL_WAVE_FREQ;
        float hit = GL_HIT_RADIUS;
        float life = GL_LIFE;
        float recoil = GL_RECOIL;
        if (difficulty == Difficulty::EASY) {
            speed *= 0.88f; damage *= 0.78f;
            amp *= 0.72f; freq *= 0.9f; recoil *= 0.72f; hit *= 0.92f;
        } else if (difficulty == Difficulty::HARD) {
            speed *= 1.08f; damage *= 1.08f;
            amp *= 1.18f; freq *= 1.08f; recoil *= 1.12f;
        }
        if (panicShot) {
            damage *= 0.75f;
            amp *= 0.82f;
            life *= 0.82f;
            recoil *= 0.95f;
        }

        float dirX = cosf(base);
        float dirY = sinf(base);
        float phase = (float)(rand() % 628) * 0.01f;
        float bdx = px - sx, bdy = py - sy;
        float bendDist = sqrtf(bdx * bdx + bdy * bdy) + 1e-3f;
        glShells.push_back({ sx, sy, sx, sy, dirX, dirY, speed, 0.0f, life,
                             phase, amp, freq, damage, hit, px, py, bendDist, true });
        if ((int)glShells.size() > 10) glShells.erase(glShells.begin());
        startGLRecoil(dirX, dirY, recoil);
    }

    void tickGLShells(float px, float py, float dt, float& playerHP,
                      std::vector<Bullet>& bullets, Difficulty difficulty) {
        (void)bullets;
        float accel = GL_ACCEL;
        float turnRate = GL_TURN_RATE;
        if (difficulty == Difficulty::EASY) { accel *= 0.78f; turnRate *= 0.72f; }
        if (difficulty == Difficulty::HARD) { accel *= 1.18f; turnRate *= 1.12f; }

        for (auto& s : glShells) {
            s.prevX = s.x;
            s.prevY = s.y;
            s.age += dt;
            s.speed += accel * dt;

            if (s.tracking && s.age > 0.18f) {
                float tx = s.bendX - s.x, ty = s.bendY - s.y;
                float td = sqrtf(tx * tx + ty * ty) + 1e-3f;
                if (td < s.lastBendDist) s.lastBendDist = td;
                float ahead = tx * s.dirX + ty * s.dirY;
                bool closeToBend = td <= 150.0f;
                bool passedBend = ahead < -10.0f || td > s.lastBendDist + 28.0f;
                bool timeout = s.age > 1.15f;
                if (closeToBend || passedBend || timeout) {
                    s.tracking = false;
                } else {
                    float curA = atan2f(s.dirY, s.dirX);
                    float wantA = atan2f(ty / td, tx / td);
                    float diff = wantA - curA;
                    while (diff >  3.14159265f) diff -= 6.2831853f;
                    while (diff < -3.14159265f) diff += 6.2831853f;
                    float maxStep = turnRate * dt;
                    if (diff >  maxStep) diff =  maxStep;
                    if (diff < -maxStep) diff = -maxStep;
                    float newA = curA + diff;
                    s.dirX = cosf(newA);
                    s.dirY = sinf(newA);
                }
            }

            float wave = s.tracking ? sinf(s.age * s.waveFreq + s.wavePhase) * s.waveAmp : 0.0f;
            float perpX = -s.dirY;
            float perpY =  s.dirX;
            s.x += (s.dirX * s.speed + perpX * wave) * dt;
            s.y += (s.dirY * s.speed + perpY * wave) * dt;

            if (segDist(s.prevX, s.prevY, s.x, s.y, px, py) <= s.hitRadius + 10.0f) {
                HurtPlayer(playerHP, s.damage);
                spawnGLPop(s.x, s.y);
                s.life = -1.0f;
            }
            bool wallHit = false;
            if (s.x < 12.0f) { s.x = 12.0f; wallHit = true; }
            if (s.x > screenW - 12.0f) { s.x = screenW - 12.0f; wallHit = true; }
            if (s.y < 12.0f) { s.y = 12.0f; wallHit = true; }
            if (s.y > screenH - 12.0f) { s.y = screenH - 12.0f; wallHit = true; }
            if (wallHit) {
                spawnGLPop(s.x, s.y);
                s.life = -1.0f;
            }
            if (s.age >= s.life) {
                s.life = -1.0f;
            }
        }
        glShells.erase(std::remove_if(glShells.begin(), glShells.end(),
            [](const GLShell& s) { return s.life < 0.0f; }), glShells.end());
        for (auto& p : glPops) p.t += dt;
        glPops.erase(std::remove_if(glPops.begin(), glPops.end(),
            [](const GLPop& p) { return p.t >= p.life; }), glPops.end());
    }

    void fireCurveMissile(std::vector<Bullet>& bullets, float px, float py, int shotIndex) {
        float side = (shotIndex & 1) ? 1.0f : -1.0f;
        float sx = worldX + side * BODY * 0.72f;
        float sy = worldY - BODY * 0.18f;
        float dx = px - sx, dy = py - sy;
        float d = sqrtf(dx * dx + dy * dy) + 1e-3f;
        float base = atan2f(dy / d, dx / d) + side * 0.22f;
        Bullet b(sx, sy, sx + cosf(base) * 100.0f, sy + sinf(base) * 100.0f);
        b.isEnemy = true;
        b.speed = CURVE_BSPEED;
        b.color = glm::vec3(1.0f, 0.25f, 0.95f);
        b.enemyDmg = 9.0f;
        b.sizeScale = 1.35f;
        b.homing = true;
        b.homingTurn = CURVE_TURN;
        b.launchRamp = 0.28f;
        b.launchAccel = 1.0f;
        bullets.push_back(b);
    }

    void fireEdgeSalvo(std::vector<Bullet>& bullets, float px, float py,
                       Difficulty difficulty) {
        const float M = 24.0f;
        struct Spawn { float x, y; } pts[8] = {
            { M, py }, { screenW - M, py },
            { px, M }, { px, screenH - M },
            { M, M }, { screenW - M, M },
            { M, screenH - M }, { screenW - M, screenH - M }
        };
        int count = (difficulty == Difficulty::HARD && phase3) ? 8 : 4;
        for (int i = 0; i < count; i++) {
            float dx = px - pts[i].x, dy = py - pts[i].y;
            float d = sqrtf(dx * dx + dy * dy) + 1e-3f;
            float sp = 520.0f + (float)(rand() % 80);
            fireDir(bullets, dx / d, dy / d, sp, glm::vec3(1.0f, 0.35f, 0.2f), 10.0f);
            if (difficulty == Difficulty::HARD && phase3) {
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

    void startCombatDash(float nx, float ny, float dist, Difficulty difficulty) {
        float side = (rand() % 2) ? 1.0f : -1.0f;
        float awayBias = (dist < 300.0f) ? 0.72f : 0.25f;
        float vx = -ny * side - nx * awayBias;
        float vy =  nx * side - ny * awayBias;
        float len = sqrtf(vx * vx + vy * vy) + 1e-3f;
        float sp = moveSpeed * (phase3 ? 2.45f : (phase2 ? 2.15f : 1.85f));
        if (difficulty == Difficulty::EASY) sp *= 0.82f;
        if (difficulty == Difficulty::HARD) sp *= 1.12f;
        dashVelX = vx / len * sp;
        dashVelY = vy / len * sp;
        dashT = (difficulty == Difficulty::EASY) ? 0.12f : 0.16f;
        dashCd = phase3 ? 1.05f : 1.28f;
        if (difficulty == Difficulty::EASY) dashCd *= 1.55f;
        if (difficulty == Difficulty::HARD) dashCd *= 0.72f;
        pushTrail();
    }

    void tickCombatDash(float dt) {
        if (dashT <= 0.0f) return;
        pushTrail();
        worldX += dashVelX * dt;
        worldY += dashVelY * dt;
        dashT -= dt;
        if (dashT < 0.0f) dashT = 0.0f;
        clampToScreen();
    }

    void Update(float px, float py, float dt, float& playerHP,
                std::vector<Bullet>& bullets,
                Difficulty difficulty = Difficulty::NORMAL) {
        if (!alive) return;

        if (!phase2 && hp <= maxHp * 0.70f) phase2 = true;
        if (!phase3 && hp <= maxHp * 0.40f) phase3 = true;

        float dx = px - worldX, dy = py - worldY;
        float dist = sqrtf(dx * dx + dy * dy) + 1e-3f;
        float nx = dx / dist, ny = dy / dist;
        spinAng += dt * (phase3 ? 4.5f : (phase2 ? 3.0f : 2.0f));
        panicCd -= dt;
        curveCd -= dt;
        dashCd -= dt;
        tickGLShells(px, py, dt, playerHP, bullets, difficulty);
        tickGLRecoil(dt);
        if (dashT > 0.0f) tickCombatDash(dt);
        if (dashT <= 0.0f && dashCd <= 0.0f && state == RRState::ACTIVE &&
            !aiming && !mgTelegraph && !mgFiring) {
            startCombatDash(nx, ny, dist, difficulty);
        }

        if (dist < BODY + 10.0f)
            HurtPlayer(playerHP, (phase3 ? 9.0f : (phase2 ? 7.5f : 6.0f)) * dt);

        bool nearMelee = dist < MELEE_NEAR;
        bool midOrFar  = dist >= MELEE_MID;

        for (auto& tr : trails) tr.life -= dt;
        trails.erase(std::remove_if(trails.begin(), trails.end(),
                     [](const Trail& t) { return t.life <= 0.0f; }), trails.end());

        if (difficulty == Difficulty::HARD && phase3 && state == RRState::ACTIVE &&
            curveShotsLeft <= 0 && curveCd <= 0.0f && !mgTelegraph && !mgFiring) {
            curveShotsLeft = CURVE_COUNT;
            curveShotT = 0.0f;
            curveCd = CURVE_CD;
        }
        if (curveShotsLeft > 0 && state == RRState::ACTIVE) {
            curveShotT -= dt;
            if (curveShotT <= 0.0f) {
                int shotIndex = CURVE_COUNT - curveShotsLeft;
                fireCurveMissile(bullets, px, py, shotIndex);
                curveShotsLeft--;
                curveShotT = CURVE_INTERVAL;
            }
        }

        if (difficulty == Difficulty::HARD && phase3) {
            float spamInt = SPAM_INT * 1.25f;
            spamTimer += dt;
            if (spamTimer >= spamInt && midOrFar && state == RRState::ACTIVE &&
                weapon != RRWeapon::MACHINEGUN && weapon != RRWeapon::GRENADE &&
                curveShotsLeft <= 0) {
                spamTimer = 0.0f;
                fireSpamBurst(bullets, nx, ny);
            }
        }

        salvoCd -= dt;
        if (allowEdgeWarning(difficulty) && salvoCd <= 0.0f && midOrFar &&
            state == RRState::ACTIVE && curveShotsLeft <= 0) {
            salvoCd = (difficulty == Difficulty::HARD) ? SALVO_CD * 0.82f : SALVO_CD * 1.2f;
            fireEdgeSalvo(bullets, px, py, difficulty);
        }

        if (dist < PANIC_DIST && state == RRState::ACTIVE && panicCd <= 0.0f) {
            enterReload(bullets, true, difficulty);
            panicCd = (difficulty == Difficulty::EASY) ? 3.2f : 2.4f;
            return;
        }
        if (dist < RF_MIN_DIST && state == RRState::ACTIVE && panicCd <= 0.0f) {
            if (weapon != RRWeapon::MACHINEGUN) fireGrenadeLauncher(nx, ny, px, py, difficulty, true);
            fleeFrom(nx, ny, dt, 1.9f);
            enterReload(bullets, false, difficulty);
            panicCd = (difficulty == Difficulty::EASY) ? 4.2f : 3.0f;
            return;
        }

        assaultCd -= dt;
        bool assaultRange = dist >= ASSAULT_MIN && dist <= ASSAULT_MAX;
        if (allowAssault(difficulty) && state == RRState::ACTIVE &&
            assaultCd <= 0.0f && midOrFar && assaultRange) {
            state = RRState::ASSAULT;
            assaultT = 0.0f;
            fireTimer = 0.0f;
            assaultCd = ASSAULT_CD;
        }

        if (state == RRState::ASSAULT) {
            if (nearMelee || dist < MELEE_MID) {
                state = RRState::ACTIVE;
                enterReload(bullets, true, difficulty);
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
                enterReload(bullets, false, difficulty);
            }
            return;
        }

        if (state == RRState::OVERHEAT) {
            overheatT += dt;
            strafeMove(nx, ny, dt * 0.22f);
            clampToScreen();
            float wait = OVERHEAT_TIME;
            if (difficulty == Difficulty::EASY) wait *= 1.25f;
            if (difficulty == Difficulty::HARD) wait *= 0.9f;
            if (overheatT >= wait) {
                overheatT = 0.0f;
                equip(pickNextWeapon(difficulty), difficulty);
            }
            return;
        }

        if (state == RRState::RELOAD_STEP) {
            worldX -= nx * moveSpeed * 0.72f * dt;
            worldY -= ny * moveSpeed * 0.72f * dt;
            if (!nearMelee) strafeMove(nx, ny, dt * 0.35f);
            clampToScreen();
            reloadTimer += dt;
            float rt = RELOAD_TIME;
            if (difficulty == Difficulty::EASY) rt *= 1.25f;
            if (difficulty == Difficulty::HARD) rt *= 0.82f;
            if (reloadTimer >= rt) {
                equip(pickNextWeapon(difficulty), difficulty);
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
            float rt = RELOAD_TIME * 0.58f;
            if (reloadTimer >= rt) {
                equip(pickNextWeapon(difficulty), difficulty);
            }
            return;
        }

        switch (weapon) {
        case RRWeapon::RIFLE: {
            if (dist < RF_MIN_DIST) {
                fleeFrom(nx, ny, dt, 1.2f);
            } else if (dist > RF_RANGE * 0.92f) {
                worldX += nx * moveSpeed * dt;
                worldY += ny * moveSpeed * dt;
                clampToScreen();
            } else {
                strafeMove(nx, ny, dt);
                clampToScreen();
            }
            if (dist < RF_RANGE && dist >= MELEE_NEAR * 0.95f) {
                fireTimer += dt;
                float interval = RF_INTERVAL;
                if (difficulty == Difficulty::EASY) interval *= 1.45f;
                if (difficulty == Difficulty::HARD) interval *= 0.86f;
                if (fireTimer >= interval) {
                    fireTimer = 0.0f;
                    fireRifleShot(bullets, nx, ny, difficulty);
                    if (--ammo <= 0) enterReload(bullets, false, difficulty);
                }
            } else if (dist < MELEE_NEAR && ammo <= 1) {
                enterReload(bullets, true, difficulty);
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
                float aimNeed = SN_AIM_DELAY;
                if (difficulty == Difficulty::EASY) aimNeed *= 1.35f;
                if (difficulty == Difficulty::HARD) aimNeed *= 0.9f;
                if (aimDelay >= aimNeed) { aiming = true; stopTimer = 0.0f; }
            } else {
                stopTimer += dt;
                float freezeNeed = SN_FREEZE;
                if (difficulty == Difficulty::EASY) freezeNeed *= 1.22f;
                if (difficulty == Difficulty::HARD) freezeNeed *= 0.9f;
                if (stopTimer >= freezeNeed) {
                    float dmg = SN_DMG * ((difficulty == Difficulty::EASY) ? 0.82f : 1.0f);
                    fireDir(bullets, nx, ny, SN_BSPEED, glm::vec3(0.25f, 1.0f, 1.0f), dmg);
                    int echoShots = 0;
                    if (difficulty == Difficulty::NORMAL && phase3) echoShots = 1;
                    if (difficulty == Difficulty::HARD && phase2) echoShots = 1;
                    if (difficulty == Difficulty::HARD && phase3) echoShots = 2;
                    float base = atan2f(ny, nx);
                    if (echoShots >= 1) {
                        float a = base + 0.085f;
                        fireDir(bullets, cosf(a), sinf(a), SN_BSPEED * 0.9f,
                                glm::vec3(0.35f, 0.85f, 1.0f), SN_DMG * 0.68f);
                    }
                    if (echoShots >= 2) {
                        float a = base - 0.085f;
                        fireDir(bullets, cosf(a), sinf(a), SN_BSPEED * 0.86f,
                                glm::vec3(0.2f, 0.95f, 1.0f), SN_DMG * 0.58f);
                    }
                    aiming = false;
                    aimDelay = 0.0f;
                    if (--ammo <= 0) enterReload(bullets, false, difficulty);
                }
            }
            break;
        }
        case RRWeapon::GRENADE: {
            if (dist < RF_MIN_DIST * 1.22f) {
                fleeFrom(nx, ny, dt, 1.35f);
            } else if (dist > RF_RANGE + 130.0f) {
                worldX += nx * moveSpeed * 0.74f * dt;
                worldY += ny * moveSpeed * 0.74f * dt;
                clampToScreen();
            } else {
                strafeMove(nx, ny, dt * 0.82f);
                clampToScreen();
            }

            if (dist < RF_RANGE + 160.0f && dist >= MELEE_NEAR * 0.92f) {
                fireTimer += dt;
                float interval = GL_INTERVAL;
                if (difficulty == Difficulty::EASY) interval *= 1.32f;
                if (difficulty == Difficulty::HARD) interval *= 0.86f;
                if (fireTimer >= interval) {
                    fireTimer = 0.0f;
                    fireGrenadeLauncher(nx, ny, px, py, difficulty);
                    if (--ammo <= 0) enterReload(bullets, false, difficulty);
                }
            } else if (dist < MELEE_NEAR && ammo <= 1) {
                enterReload(bullets, true, difficulty);
            }
            break;
        }
        case RRWeapon::MACHINEGUN: {
            if (dist < RF_MIN_DIST && (mgTelegraph || mgFiring)) {
                mgTelegraph = mgFiring = mgZoneLocked = false;
                enterReload(bullets, true, difficulty);
                break;
            }
            if (mgTelegraph) {
                if (!mgZoneLocked) { zoneDirX = nx; zoneDirY = ny; mgZoneLocked = true; }
                warmUpTimer += dt;
                float warm = MG_WARMUP;
                if (difficulty == Difficulty::EASY) warm *= 1.35f;
                if (difficulty == Difficulty::HARD) warm *= 0.9f;
                if (warmUpTimer >= warm) {
                    mgTelegraph = false;
                    mgFiring = true;
                    fireTimer = 0.0f;
                }
            } else if (mgFiring) {
                float rotMul = phase3 ? 1.15f : 0.9f;
                if (difficulty == Difficulty::EASY) rotMul *= 0.55f;
                if (difficulty == Difficulty::HARD) rotMul *= 1.35f;
                float rot = MG_ZONE_ROT * dt * rotMul;
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
                    if (difficulty == Difficulty::HARD && phase3 && (rand() % 4) == 0) {
                        float a2 = base + r * zoneHalfAngle * 1.4f;
                        fireDir(bullets, cosf(a2), sinf(a2), MG_BSPEED * 0.9f,
                                glm::vec3(1.0f, 0.55f, 0.15f), MG_DMG * 0.7f);
                    }
                    if (--ammo <= 0) {
                        mgFiring = false;
                        overheatT = 0.0f;
                        state = RRState::OVERHEAT;
                    }
                }
            }
            break;
        }
        }
    }

    static const wchar_t* weaponTag(RRWeapon w) {
        switch (w) {
        case RRWeapon::RIFLE:      return L"RF";
        case RRWeapon::SNIPER:     return L"SR";
        case RRWeapon::GRENADE:    return L"GL";
        case RRWeapon::MACHINEGUN: return L"MG";
        default: return L"??";
        }
    }

    static const wchar_t* BossName() { return L"VOLLEY.sys"; }

    static void drawFan(float ox, float oy, float a0, float a1, float len,
                        float r, float g, float b, float fillA, float edgeA, int segs = 24) {
        for (int s = 0; s < segs; s++) {
            float u0 = (float)s / (float)segs, u1 = (float)(s + 1) / (float)segs;
            float aa = a0 + (a1 - a0) * u0, ab = a0 + (a1 - a0) * u1;
            float x0 = ox + cosf(aa) * len, y0 = oy + sinf(aa) * len;
            float x1 = ox + cosf(ab) * len, y1 = oy + sinf(ab) * len;
            for (int layer = 1; layer <= 4; layer++) {
                float t = (float)layer / 4.0f;
                float sx = (x0 + x1) * 0.5f, sy = (y0 + y1) * 0.5f;
                float mx = ox + (sx - ox) * t;
                float my = oy + (sy - oy) * t;
                drawRect(mx - 2.5f, my - 2.5f, 5.0f, 5.0f, r, g, b, fillA * (0.25f + t * 0.75f));
            }
            drawRect(x0 - 3.0f, y0 - 3.0f, 6.0f, 6.0f, r, g, b, edgeA);
            drawRect(x1 - 3.0f, y1 - 3.0f, 6.0f, 6.0f, r, g, b, edgeA);
        }
        float ex0 = ox + cosf(a0) * len, ey0 = oy + sinf(a0) * len;
        float ex1 = ox + cosf(a1) * len, ey1 = oy + sinf(a1) * len;
        const int RAY = 14;
        for (int s = 0; s < RAY; s++) {
            if ((s & 1) == 0) continue;
            float u0 = (float)s / (float)RAY, u1 = (float)(s + 1) / (float)RAY;
            auto dot = [&](float x0, float y0, float x1, float y1) {
                float px = x0 + (x1 - x0) * u0, py = y0 + (y1 - y0) * u0;
                float qx = x0 + (x1 - x0) * u1, qy = y0 + (y1 - y0) * u1;
                drawRect((px + qx) * 0.5f - 2.0f, (py + qy) * 0.5f - 2.0f,
                         4.0f, 4.0f, r, g, b, edgeA);
            };
            dot(ox, oy, ex0, ey0);
            dot(ox, oy, ex1, ey1);
        }
    }

    static void drawArcRing(float cx, float cy, float rad,
                            float r, float g, float b, float a, int n = 32) {
        for (int i = 0; i < n; i++) {
            if ((i & 1) == 0) continue;
            float a0 = (float)i / (float)n * 6.2831853f;
            float a1 = (float)(i + 1) / (float)n * 6.2831853f;
            float x0 = cx + cosf(a0) * rad, y0 = cy + sinf(a0) * rad;
            float x1 = cx + cosf(a1) * rad, y1 = cy + sinf(a1) * rad;
            drawRect((x0 + x1) * 0.5f - 2.0f, (y0 + y1) * 0.5f - 2.0f,
                     4.0f, 4.0f, r, g, b, a);
        }
    }

    void renderGLShells(float gt) const {
        for (const auto& p : glPops) {
            float u = (p.life > 0.0f) ? p.t / p.life : 1.0f;
            if (u < 0.0f) u = 0.0f;
            if (u > 1.0f) u = 1.0f;
            float a = 1.0f - u;
            drawCircle(p.x, p.y, 14.0f + 34.0f * u, 0.35f, 1.0f, 0.55f, 0.18f * a);
            drawArcRing(p.x, p.y, 20.0f + 42.0f * u, 1.0f, 0.34f, 0.9f, 0.55f * a, 22);
            drawCircle(p.x, p.y, 6.0f + 8.0f * u, 1.0f, 0.42f, 0.92f, 0.75f * a);
        }

        for (const auto& s : glShells) {
            float lifeT = (s.life > 0.0f) ? s.age / s.life : 1.0f;
            if (lifeT < 0.0f) lifeT = 0.0f;
            if (lifeT > 1.0f) lifeT = 1.0f;
            float pulse = 0.55f + 0.45f * sinf(gt * 18.0f + s.wavePhase);
            drawRect(s.prevX - 4.0f, s.prevY - 4.0f, 8.0f, 8.0f,
                     0.18f, 1.0f, 0.58f, 0.28f * (1.0f - lifeT));
            drawCircle(s.x, s.y, s.hitRadius + 8.0f + pulse * 3.0f,
                       0.3f, 1.0f, 0.55f, 0.18f + pulse * 0.12f);
            drawArcRing(s.x, s.y, s.hitRadius + 13.0f,
                        1.0f, 0.34f, 0.9f, 0.42f + pulse * 0.22f, 18);
            drawCircle(s.x, s.y, 8.0f + pulse * 2.0f, 0.3f, 1.0f, 0.55f, 0.9f);
            drawCircle(s.x, s.y, 4.0f, 1.0f, 0.36f, 0.9f, 0.95f);
        }
    }

    void renderWeaponBackdrop(float gt) const {
        float cx = screenW * 0.5f;
        float cy = screenH * 0.52f;
        float pulse = 0.5f + 0.5f * sinf(gt * 1.7f);
        float a = 0.055f + pulse * 0.025f;
        if (weapon == RRWeapon::SNIPER) {
            drawRect(cx - 260.0f, cy - 9.0f, 520.0f, 18.0f, 0.15f, 0.95f, 1.0f, a);
            drawRect(cx + 150.0f, cy - 26.0f, 60.0f, 52.0f, 0.15f, 0.95f, 1.0f, a * 0.75f);
            drawCircle(cx - 35.0f, cy, 46.0f, 0.15f, 0.95f, 1.0f, a * 0.55f);
        } else if (weapon == RRWeapon::RIFLE) {
            drawRect(cx - 190.0f, cy - 16.0f, 360.0f, 32.0f, 0.55f, 1.0f, 0.25f, a);
            drawRect(cx - 30.0f, cy + 14.0f, 54.0f, 90.0f, 0.55f, 1.0f, 0.25f, a * 0.8f);
            drawRect(cx - 260.0f, cy - 34.0f, 82.0f, 68.0f, 0.55f, 1.0f, 0.25f, a * 0.65f);
        } else if (weapon == RRWeapon::GRENADE) {
            drawRect(cx - 160.0f, cy - 28.0f, 270.0f, 56.0f, 0.3f, 1.0f, 0.55f, a * 0.85f);
            drawCircle(cx + 110.0f, cy, 52.0f, 0.3f, 1.0f, 0.55f, a * 0.72f);
            drawCircle(cx + 110.0f, cy, 26.0f, 1.0f, 0.38f, 0.92f, a * 0.48f);
            drawRect(cx - 238.0f, cy + 20.0f, 74.0f, 42.0f, 0.3f, 1.0f, 0.55f, a * 0.6f);
        } else {
            drawCircle(cx - 80.0f, cy, 58.0f, 1.0f, 0.72f, 0.12f, a * 0.75f);
            drawCircle(cx,        cy, 58.0f, 1.0f, 0.72f, 0.12f, a * 0.75f);
            drawCircle(cx + 80.0f, cy, 58.0f, 1.0f, 0.72f, 0.12f, a * 0.75f);
            drawRect(cx - 170.0f, cy - 16.0f, 340.0f, 32.0f, 1.0f, 0.72f, 0.12f, a);
        }
    }

    void renderTelegraphs(float px, float py, float gt, bool drawGL = true) const {
        float adx = px - worldX, ady = py - worldY;
        float ad = sqrtf(adx * adx + ady * ady) + 1e-3f;
        float baseA = atan2f(ady, adx);

        renderWeaponBackdrop(gt);

        if (drawGL) renderGLShells(gt);

        drawArcRing(worldX, worldY, MELEE_NEAR, 0.25f, 0.95f, 0.85f, 0.22f);
        drawNeonBorder(worldX - MELEE_NEAR, worldY - MELEE_NEAR,
                       MELEE_NEAR * 2, MELEE_NEAR * 2, 0.3f, 1.0f, 0.75f);

        if (salvoCd < 0.45f && state != RRState::ASSAULT) {
            float flash = 0.35f + 0.45f * sinf(gt * 22.0f);
            float m = 28.0f;
            drawRect(0, 0, (float)screenW, m, 1.0f, 0.35f, 0.15f, flash * 0.35f);
            drawRect(0, (float)screenH - m, (float)screenW, m, 1.0f, 0.35f, 0.15f, flash * 0.35f);
            drawRect(0, 0, m, (float)screenH, 1.0f, 0.35f, 0.15f, flash * 0.35f);
            drawRect((float)screenW - m, 0, m, (float)screenH, 1.0f, 0.35f, 0.15f, flash * 0.35f);
        }

        if (aiming) {
            float dxn = adx / ad, dyn = ady / ad;
            float ex = worldX + dxn * (float)(screenW + screenH);
            float ey = worldY + dyn * (float)(screenW + screenH);
            float blink = 0.65f + 0.35f * sinf(gt * 38.0f);
            float prog = (SN_FREEZE > 0.0f) ? stopTimer / SN_FREEZE : 0.0f;
            if (prog > 1.0f) prog = 1.0f;
            const int SEG = 22;
            for (int s = 0; s < SEG; s++) {
                if ((s & 1) == 0) continue;
                float u0 = (float)s / (float)SEG, u1 = (float)(s + 1) / (float)SEG;
                float x0 = worldX + (ex - worldX) * u0, y0 = worldY + (ey - worldY) * u0;
                float x1 = worldX + (ex - worldX) * u1, y1 = worldY + (ey - worldY) * u1;
                float th = 4.0f + prog * 4.0f;
                drawRect((x0 + x1) * 0.5f - th, (y0 + y1) * 0.5f - th,
                         th * 2, th * 2, 0.15f, 0.95f, 1.0f, blink);
            }
            drawCircle(px, py, 16.0f + prog * 10.0f, 0.2f, 0.95f, 1.0f, 0.25f + prog * 0.35f);
            drawNeonBorder(px - 18.0f, py - 18.0f, 36.0f, 36.0f, 0.3f, 0.95f, 1.0f);
        }

        if (weapon == RRWeapon::SNIPER && !aiming && ad < SN_KITE_RANGE) {
            drawArcRing(worldX, worldY, SN_KITE_RANGE, 0.2f, 0.85f, 1.0f, 0.14f);
        }

        if (mgTelegraph || mgFiring) {
            float base = atan2f(zoneDirY, zoneDirX);
            float a0 = base - zoneHalfAngle, a1 = base + zoneHalfAngle;
            float prog = mgTelegraph ? (warmUpTimer / MG_WARMUP) : 1.0f;
            if (prog > 1.0f) prog = 1.0f;
            float fillA = 0.18f + 0.32f * prog;
            float edgeA = 0.45f + 0.4f * prog;
            drawFan(worldX, worldY, a0, a1, zoneLen * (0.55f + prog * 0.45f),
                    1.0f, 0.55f, 0.12f, fillA, edgeA, 28);
            drawCircle(worldX, worldY, 22.0f + prog * 18.0f, 1.0f, 0.5f, 0.1f, 0.2f + prog * 0.2f);
            wchar_t mgw[] = L"MG ZONE";
            float mw = g_TextS.Width(mgw, 0.48f);
            g_TextS.Draw(mgw, worldX - mw * 0.5f, worldY + BODY + 8.0f, 0.48f,
                         1.0f, 0.65f, 0.15f, 0.75f + prog * 0.2f);
        }

        if (state == RRState::ACTIVE && weapon == RRWeapon::RIFLE && ad < RF_RANGE + 40.0f) {
            float ha = 0.09f;
            float inRange = (ad < RF_RANGE) ? 1.0f : 0.55f;
            drawFan(worldX, worldY, baseA - ha, baseA + ha, RF_RANGE,
                    0.55f, 1.0f, 0.25f, 0.12f * inRange, 0.42f * inRange, 12);
            drawArcRing(worldX, worldY, RF_RANGE, 0.55f, 1.0f, 0.25f, 0.22f * inRange);
        }

        if (drawGL && state == RRState::ACTIVE && weapon == RRWeapon::GRENADE && ad < RF_RANGE + 190.0f) {
            float ha = 0.16f;
            float inRange = (ad < RF_RANGE + 160.0f) ? 1.0f : 0.55f;
            drawFan(worldX, worldY, baseA - ha, baseA + ha, RF_RANGE + 160.0f,
                    0.3f, 1.0f, 0.55f, 0.09f * inRange, 0.36f * inRange, 14);
            drawArcRing(worldX, worldY, RF_RANGE + 160.0f, 0.3f, 1.0f, 0.55f, 0.17f * inRange);
        }

        if (state == RRState::OVERHEAT) {
            float p = 0.55f + 0.45f * sinf(gt * 18.0f);
            drawCircle(worldX, worldY, BODY * (1.55f + p * 0.2f), 1.0f, 0.18f, 0.08f, 0.24f);
            wchar_t oh[] = L"OVERHEAT";
            float ow = g_TextS.Width(oh, 0.54f);
            g_TextS.Draw(oh, worldX - ow * 0.5f, worldY + BODY + 10.0f, 0.54f,
                         1.0f, 0.35f, 0.12f, 0.92f);
        }

        if (state == RRState::ASSAULT) {
            float prog = (ASSAULT_WARM > 0.0f) ? assaultT / ASSAULT_WARM : 1.0f;
            if (prog > 1.0f) prog = 1.0f;
            float pulse = 0.5f + 0.5f * sinf(gt * (assaultT < ASSAULT_WARM ? 18.0f : 10.0f));
            drawArcRing(worldX, worldY, ASSAULT_MIN, 1.0f, 0.35f, 0.12f, 0.2f + prog * 0.15f);
            if (assaultT < ASSAULT_WARM) {
                float ha = 0.28f;
                drawFan(worldX, worldY, baseA - ha, baseA + ha, ASSAULT_MAX * 0.55f,
                        1.0f, 0.22f, 0.08f, 0.15f + prog * 0.2f, 0.5f + prog * 0.3f, 18);
                wchar_t warn[] = L"! ASSAULT !";
                float ww = g_TextS.Width(warn, 0.58f);
                g_TextS.Draw(warn, worldX - ww * 0.5f, worldY - BODY - 40.0f, 0.58f,
                             1.0f, 0.28f, 0.1f, 0.85f + prog * 0.15f);
            } else {
                float ha = 0.26f;
                drawFan(worldX, worldY, baseA - ha, baseA + ha, 280.0f,
                        1.0f, 0.28f, 0.1f, 0.25f, 0.65f, 14);
            }
            drawCircle(worldX, worldY, BODY * (1.3f + prog * 0.35f + pulse * 0.12f),
                       1.0f, 0.25f, 0.08f, 0.15f + prog * 0.25f);
        }
    }

    void renderBody(float gt, float px, float py) const {
        float aim = atan2f(py - worldY, px - worldX);
        float pulse = 0.5f + 0.5f * sinf(gt * 5.0f);
        float ca = cosf(aim), sa = sinf(aim);

        for (auto& tr : trails) {
            float a = (tr.life > 0.0f) ? tr.life / 0.28f : 0.0f;
            drawRect(tr.x - BODY * 0.35f, tr.y - BODY * 0.2f,
                     BODY * 0.7f, BODY * 0.4f, 1.0f, 0.55f, 0.15f, 0.16f * a);
        }

        if (phase3) {
            drawCircle(worldX, worldY, BODY * 1.5f, 1.0f, 0.15f, 0.08f, 0.1f + pulse * 0.08f);
        } else if (phase2) {
            drawCircle(worldX, worldY, BODY * 1.28f, 1.0f, 0.45f, 0.12f, 0.07f + pulse * 0.06f);
        }

        float cr, cg, cb;
        if (state == RRState::RELOAD_SPRINT)      { cr = 1.0f; cg = 0.88f; cb = 0.25f; }
        else if (state == RRState::RELOAD_STEP)   { cr = 0.65f; cg = 1.0f; cb = 0.75f; }
        else if (state == RRState::OVERHEAT)      { cr = 1.0f; cg = 0.22f; cb = 0.08f; }
        else if (state == RRState::ASSAULT)       { cr = 1.0f; cg = 0.28f; cb = 0.12f; }
        else if (weapon == RRWeapon::RIFLE)       { cr = 0.55f; cg = 1.0f; cb = 0.25f; }
        else if (weapon == RRWeapon::SNIPER)      { cr = 0.22f; cg = 0.92f; cb = 1.0f; }
        else if (weapon == RRWeapon::GRENADE)     { cr = 0.32f; cg = 1.0f; cb = 0.58f; }
        else                                      { cr = 1.0f; cg = 0.78f; cb = 0.18f; }

        drawCircle(worldX, worldY, BODY * 0.92f, 0.01f, 0.015f, 0.025f, 0.54f);
        drawRect(worldX - BODY * 0.98f, worldY - BODY * 0.78f,
                 BODY * 1.96f, BODY * 0.18f, 0.02f, 0.025f, 0.04f, 0.92f);
        drawNeonBorder(worldX - BODY * 0.98f, worldY - BODY * 0.78f,
                       BODY * 1.96f, BODY * 0.18f, cr, cg, cb);
        drawRect(worldX - BODY * 0.72f, worldY + BODY * 0.63f,
                 BODY * 1.44f, BODY * 0.16f, 0.02f, 0.025f, 0.04f, 0.84f);
        drawNeonBorder(worldX - BODY * 0.72f, worldY + BODY * 0.63f,
                       BODY * 1.44f, BODY * 0.16f, cr * 0.85f, cg * 0.85f, cb * 0.85f);

        drawRect(worldX - BODY * 1.55f, worldY - BODY * 0.28f,
                 BODY * 0.72f, BODY * 0.56f, 0.05f, 0.06f, 0.09f, 0.88f);
        drawRect(worldX + BODY * 0.83f, worldY - BODY * 0.28f,
                 BODY * 0.72f, BODY * 0.56f, 0.05f, 0.06f, 0.09f, 0.88f);
        drawNeonBorder(worldX - BODY * 1.55f, worldY - BODY * 0.28f,
                       BODY * 0.72f, BODY * 0.56f, cr, cg, cb);
        drawNeonBorder(worldX + BODY * 0.83f, worldY - BODY * 0.28f,
                       BODY * 0.72f, BODY * 0.56f, cr, cg, cb);
        drawRect(worldX - BODY * 0.28f, worldY + BODY * 0.48f,
                 BODY * 0.22f, BODY * 0.55f, 1.0f, 0.42f, 0.12f, 0.25f + pulse * 0.22f);
        drawRect(worldX + BODY * 0.06f, worldY + BODY * 0.48f,
                 BODY * 0.22f, BODY * 0.55f, 1.0f, 0.42f, 0.12f, 0.25f + pulse * 0.22f);
        drawCircle(worldX - BODY * 1.2f, worldY, 8.0f + pulse * 2.0f, cr, cg, cb, 0.82f);
        drawCircle(worldX + BODY * 1.2f, worldY, 8.0f + pulse * 2.0f, cr, cg, cb, 0.82f);
        if (curveShotsLeft > 0) {
            drawCircle(worldX - BODY * 1.2f, worldY, 17.0f, 1.0f, 0.22f, 0.92f, 0.35f + pulse * 0.22f);
            drawCircle(worldX + BODY * 1.2f, worldY, 17.0f, 1.0f, 0.22f, 0.92f, 0.35f + pulse * 0.22f);
        }

        for (int i = 0; i < 2; i++) {
            float side = (i == 0) ? 1.0f : -1.0f;
            float pxp = worldX + (-sa * side) * (BODY * 0.72f);
            float pyp = worldY + ( ca * side) * (BODY * 0.72f);
            drawCircle(pxp, pyp, 11.0f, cr * 0.12f, cg * 0.12f, cb * 0.12f, 0.9f);
            drawCircle(pxp, pyp, 7.0f, cr * 0.35f, cg * 0.35f, cb * 0.35f, 0.85f);
            float gx = pxp + ca * 10.0f, gy = pyp + sa * 10.0f;
            drawRect(gx - 2, gy - 2, 12.0f, 4.0f, cr, cg, cb, 0.9f);
        }

        drawCircle(worldX, worldY, BODY * 0.62f, 0.06f, 0.05f, 0.08f, 0.92f);
        drawNeonBorder(worldX - BODY * 0.62f, worldY - BODY * 0.62f,
                       BODY * 1.24f, BODY * 1.24f, cr, cg, cb);
        for (int i = 0; i < 3; i++) {
            float a = spinAng * 0.8f + (float)i * 2.094f;
            drawRect(worldX + cosf(a) * BODY * 0.5f - 2.0f,
                     worldY + sinf(a) * BODY * 0.38f - 2.0f,
                     4.0f, 4.0f, cr, cg, cb, 0.4f + pulse * 0.3f);
        }

        float prowX = worldX + ca * (BODY * 0.38f);
        float prowY = worldY + sa * (BODY * 0.38f);
        drawTriangle(prowX, prowY, 16.0f, cr, cg, cb, 0.88f);

        float bx = worldX + ca * (BODY * 0.62f);
        float by = worldY + sa * (BODY * 0.62f);
        if (weapon == RRWeapon::RIFLE || state == RRState::ASSAULT) {
            drawRect(bx - 3, by - 3, 26, 6, 0.1f, 0.1f, 0.12f, 0.95f);
            drawRect(bx + 4, by - 2, 22, 4, cr, cg, cb, 0.95f);
            drawRect(bx + 2, by + 3, 7, 12, cr * 0.75f, cg * 0.75f, cb * 0.75f, 0.85f);
        } else if (weapon == RRWeapon::SNIPER) {
            drawRect(bx - 1, by - 1.5f, 30, 3, cr, cg, cb, 1.0f);
            drawCircle(bx + 24, by, 4.0f, cr, cg, cb, 0.75f);
        } else if (weapon == RRWeapon::GRENADE) {
            drawRect(bx - 8, by - 8, 34, 16, 0.06f, 0.07f, 0.09f, 0.96f);
            drawRect(bx + 18, by - 11, 18, 22, cr, cg, cb, 0.78f);
            drawCircle(bx + 34, by, 13.0f, cr, cg, cb, 0.45f);
            drawCircle(bx + 34, by, 6.0f, 1.0f, 0.36f, 0.9f, 0.82f);
            drawRect(bx - 1, by + 8, 9, 15, cr * 0.72f, cg * 0.72f, cb * 0.72f, 0.82f);
        } else {
            for (int i = 0; i < 4; i++)
                drawRect(bx + (float)i * 4.0f, by - 2, 3, 4, cr, cg, cb, 0.85f);
        }

        drawRect(worldX - 8.0f, worldY - BODY * 0.55f, 16.0f, 5.0f,
                 0.15f, 0.95f, 1.0f, 0.55f + pulse * 0.25f);
        drawCircle(worldX, worldY, 6.0f, 1.0f, 0.95f, 0.8f, 0.7f);

        if (state == RRState::OVERHEAT) {
            const wchar_t* oh = L"OVERHEAT";
            float ow = g_TextS.Width(oh, 0.52f);
            g_TextS.Draw(oh, worldX - ow * 0.5f, worldY - BODY - 38.0f, 0.52f,
                         1.0f, 0.32f, 0.1f, 0.95f);
        } else if (state == RRState::RELOAD_STEP) {
            const wchar_t* rl = L"RELOAD";
            float rw = g_TextS.Width(rl, 0.52f);
            g_TextS.Draw(rl, worldX - rw * 0.5f, worldY - BODY - 38.0f, 0.52f,
                         0.65f, 1.0f, 0.75f, 0.95f);
        } else if (state == RRState::RELOAD_SPRINT &&
            ((int)(gt * 6.0f) % 2 == 0)) {
            const wchar_t* rl = L"SPRINT>>";
            float rw = g_TextS.Width(rl, 0.52f);
            g_TextS.Draw(rl, worldX - rw * 0.5f, worldY - BODY - 38.0f, 0.52f,
                         1.0f, 0.85f, 0.25f, 0.95f);
        } else if (state != RRState::RELOAD_SPRINT) {
            wchar_t tag[24];
            float adx = px - worldX, ady = py - worldY;
            float dist = sqrtf(adx * adx + ady * ady);
            if (state == RRState::ASSAULT)
                swprintf_s(tag, L"ASSAULT");
            else if (dist < MELEE_NEAR)
                swprintf_s(tag, L"PANIC");
            else
                swprintf_s(tag, L"[%ls %d]", weaponTag(weapon), ammo);
            float tw = g_TextS.Width(tag, 0.48f);
            g_TextS.Draw(tag, worldX - tw * 0.5f, worldY - BODY - 34.0f, 0.48f,
                         cr, cg, cb, 0.88f);
        }
    }
};
