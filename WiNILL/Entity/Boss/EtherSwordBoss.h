#pragma once
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <glm/glm.hpp>
#include "Bullet.h"
#include "DrawPrim.h"
#include "PlayerStats.h"
#include "Settings.h"

enum class ESState {
    IDLE,
    BASIC_ATTACK,
    CHARGE,
    DASH,
    PUNISH
};

enum class ESBasicType {
    EDGE_SLASH,
    ORBIT_CUT,
    CROSS_CHECK
};

class EtherSwordBoss {
public:
    static constexpr int ARENA_SWORD_MAX = 20;

    float worldX = 0.0f, worldY = 0.0f;
    float hp = 1.0f, maxHp = 1.0f;
    bool  alive = true;
    bool  exploded = false;
    int   screenW = 0, screenH = 0;

    bool phase2 = false;
    bool phase3 = false;
    bool enrage = false;

    float arenaRadius = 0.0f;
    float barrierRot = 0.0f;
    bool  arenaActive = false;
    bool  arenaForming = false;
    float arenaFormT = 0.0f;
    int   arenaSwordCount = 0;
    float arenaLaunchCd = 0.0f;
    float arenaRebuildTimer = 0.0f;
    static constexpr float ARENA_FORM_DUR = 5.0f;
    static constexpr float ARENA_FORM_MAXR = 0.66f;

    ESState swordState = ESState::IDLE;
    float   swordAngle = 0.0f;
    float   swordLen = 150.0f;
    float   idleTimer = 2.5f;
    float   chargeTimer = 0.0f;
    float   dashTimer = 0.0f;
    float   dashVX = 0.0f, dashVY = 0.0f;
    float   dashTravelLeft = 0.0f;
    float   dashPreviewX = 0.0f, dashPreviewY = 0.0f;
    float   punishTimer = 0.0f;
    float   chargeTotal = 0.0f;
    float   chargeSpinDir = 1.0f;
    float   dashStartX = 0.0f, dashStartY = 0.0f;
    float   dashEndX = 0.0f, dashEndY = 0.0f;
    float   dashTrailT = 0.0f;
    float   dashShockT = 0.0f;
    bool    dashHit = false;
    int     dashCycle = 0;

    float lastPX = 0.0f, lastPY = 0.0f;
    float prevPX = 0.0f, prevPY = 0.0f;
    bool  hasPlayerPos = false;

    struct BasicAttack {
        ESBasicType type = ESBasicType::EDGE_SLASH;
        float t = 0.0f;
        float warn = 0.55f;
        float dur = 0.20f;
        float x = 0.0f, y = 0.0f;
        float angle = 0.0f;
        float radius = 0.0f;
        bool fired = false;
        bool hitA = false;
        bool hitB = false;
    };
    BasicAttack basic;

    bool  bgActive = false;
    int   bgMode = 0;
    int   bgCycle = 0;
    float bgCd = 8.0f;
    float bgTimer = 0.0f;
    float bgOrbAng = 0.0f;
    float bgOrbDir = 1.0f;
    float bgX = 0.0f, bgY = 0.0f;
    float bgSlashCd = 0.0f;
    float bgFadeAlpha = 0.0f;
    int   bgSpawned = 0;

    struct ArcSlash {
        float cx = 0.0f, cy = 0.0f;
        float radius = 0.0f;
        float angle = 0.0f;
        float halfArc = 1.0f;
        float t = 0.0f;
        float warn = 0.72f;
        float activeDur = 0.18f;
        float thickness = 30.0f;
        float damage = 13.0f;
        bool active = true;
        bool hit = false;
    };
    std::vector<ArcSlash> arcSlashes;

    struct CrescentSlash {
        float x = 0.0f, y = 0.0f;
        float prevX = 0.0f, prevY = 0.0f;
        float dirX = 1.0f, dirY = 0.0f;
        float speed = 620.0f;
        float accel = 0.0f;
        float life = 1.35f;
        float maxLife = 1.35f;
        float damage = 14.0f;
        float radius = 44.0f;
        float maxRadius = 44.0f;
        float radiusGrow = 0.0f;
        bool active = true;
        bool hit = false;
    };
    std::vector<CrescentSlash> crescentSlashes;

    struct ArenaSwordStrike {
        int slot = -1;
        float sx = 0.0f, sy = 0.0f;
        float x = 0.0f, y = 0.0f;
        float prevX = 0.0f, prevY = 0.0f;
        float dirX = 1.0f, dirY = 0.0f;
        float t = 0.0f;
        float warn = 0.62f;
        float speed = 900.0f;
        float damage = 12.0f;
        float life = 1.8f;
        bool launched = false;
        bool active = true;
        bool hit = false;
    };
    std::vector<ArenaSwordStrike> arenaSwordStrikes;

    float shadowX = 0.0f, shadowY = 0.0f;
    float shadowOrbAng = 0.0f;
    float shadowSlashCd = 0.0f;

    struct BadSector { float x = 0.0f, y = 0.0f; bool active = false; };
    static constexpr int MAX_BAD = 12;
    BadSector badSectors[MAX_BAD] = {};
    int badIdx = 0;

    float flashT = 0.0f;
    float flashR = 0.0f, flashG = 0.0f, flashB = 0.0f;

    bool  taskKillActive = false;
    float taskKillT = 0.0f;
    float safeX = 0.0f, safeY = 0.0f;
    float sweepY = 0.0f;
    bool  taskKillDone = false;

    static constexpr float PI = 3.1415926535f;
    static constexpr float TAU = 6.283185307f;
    static constexpr float DASH_SPEED = 1960.0f;
    static constexpr float SWORD_LEN = 150.0f;
    static constexpr float HIT_RADIUS = 60.0f;
    static constexpr float CONTACT_DPS = 12.0f;
    static constexpr float ARENA_FACTOR = 0.42f;
    static constexpr float TASK_KILL_HP_FRAC = 0.01f;
    static constexpr const wchar_t* BOSS_NAME = L"ETHER_SWORD_MASTER.sys";

    EtherSwordBoss(int sw, int sh, float hpInit)
        : screenW(sw), screenH(sh)
    {
        hp = maxHp = hpInit;
        worldX = sw * 0.5f;
        worldY = sh * 0.32f;
        arenaRadius = std::min(sw, sh) * ARENA_FACTOR;
        idleTimer = 1.15f;
        bgCd = 8.0f;
    }

    float taskKillHp() const {
        return maxHp * TASK_KILL_HP_FRAC;
    }

    bool BodyInvulnerable() const { return taskKillActive; }

    bool IsOutsideArena(float px, float py) const {
        if (!phase2 || !arenaActive || arenaForming || taskKillActive) return false;
        float acx = screenW * 0.5f, acy = screenH * 0.5f;
        float dx = px - acx, dy = py - acy;
        return sqrtf(dx * dx + dy * dy) > arenaRadius;
    }

    float PlayerDamageToBossMul(float px, float py) const {
        return IsOutsideArena(px, py) ? 0.50f : 1.0f;
    }

    bool IsBadSector(float px, float py) const {
        if (!enrage || taskKillActive) return false;
        for (const auto& b : badSectors) {
            if (!b.active) continue;
            if (fabsf(px - b.x) <= 40.0f && fabsf(py - b.y) <= 40.0f)
                return true;
        }
        return false;
    }

    void BeginTaskKill(float px, float py, std::vector<Bullet>& bullets) {
        if (taskKillActive || taskKillDone || !alive) return;

        hp = taskKillHp();
        taskKillActive = true;
        taskKillDone = false;
        taskKillT = 0.0f;
        sweepY = 0.0f;
        bgActive = false;
        bgFadeAlpha = 0.0f;
        arcSlashes.clear();
        crescentSlashes.clear();
        arenaSwordStrikes.clear();
        shadowSlashCd = 999.0f;
        swordState = ESState::PUNISH;

        for (auto& b : bullets) {
            if (b.isEnemy) b.active = false;
        }

        safeX = clampf(px + (float)(rand() % 181 - 90), 90.0f, (float)screenW - 90.0f);
        safeY = clampf(py + (float)(rand() % 181 - 90), 90.0f, (float)screenH - 90.0f);
    }

    void Update(float px, float py, float arenaR,
                float dt, float& playerHP,
                std::vector<Bullet>& bullets,
                Difficulty difficulty,
                float playerMaxHP = 100.0f) {
        if (!alive) return;
        (void)playerMaxHP;

        if (!hasPlayerPos) {
            lastPX = prevPX = px;
            lastPY = prevPY = py;
            hasPlayerPos = true;
        } else {
            prevPX = lastPX;
            prevPY = lastPY;
            lastPX = px;
            lastPY = py;
        }

        if (taskKillActive) {
            updateTaskKill(px, py, dt, playerHP);
            return;
        }

        updatePhases(bullets, px, py, dt, difficulty);
        if (taskKillActive) return;

        if (flashT > 0.0f) flashT -= dt;
        if (dashTrailT > 0.0f) dashTrailT = std::max(0.0f, dashTrailT - dt);
        if (dashShockT > 0.0f) dashShockT = std::max(0.0f, dashShockT - dt);
        float barrierSpd = enrage ? 7.5f : phase3 ? 5.5f : phase2 ? 4.0f : 2.5f;
        barrierRot += dt * barrierSpd;

        if (arenaForming) {
            updateArenaOpeningMotion(dt);
        }
        updateArenaSwords(px, py, dt, playerHP, difficulty);

        if (phase2 && arenaActive && !arenaForming) {
            float acx = screenW * 0.5f, acy = screenH * 0.5f;
            float dx = px - acx, dy = py - acy;
            if (sqrtf(dx * dx + dy * dy) > arenaR)
                HurtPlayer(playerHP, 10.0f * dt);
        }

        if (phase2 && arenaActive) updatePatternB(px, py, dt, playerHP, bullets, difficulty);
        updateArcSlashes(px, py, dt, playerHP);
        updateCrescentSlashes(px, py, dt, playerHP);

        if (!arenaForming && !(phase2 && !phase3 && bgActive))
            updatePatternA(px, py, dt, playerHP, bullets, difficulty);

        if (enrage) updateShadow(px, py, dt, bullets, difficulty);

        if (!BodyInvulnerable()) {
            float dx = px - worldX, dy = py - worldY;
            if (sqrtf(dx * dx + dy * dy) < HIT_RADIUS) {
                HurtPlayer(playerHP, CONTACT_DPS * dt * damageMul(difficulty));
            }
        }
    }

    void updatePatternA(float px, float py, float dt, float& playerHP,
                        std::vector<Bullet>& bullets,
                        Difficulty difficulty) {
        switch (swordState) {
        case ESState::IDLE:
            aimSwordAt(px, py, dt * 9.5f);
            swordLen = SWORD_LEN;
            idleTimer -= dt;
            if (idleTimer <= 0.0f) {
                if (shouldUseBasic(difficulty))
                    startBasicAttack(px, py, difficulty);
                else
                    startCharge(px, py, difficulty);
            }
            break;

        case ESState::BASIC_ATTACK:
            updateBasicAttack(px, py, dt, playerHP, bullets, difficulty);
            break;

        case ESState::CHARGE:
            updateCharge(px, py, dt, difficulty);
            break;

        case ESState::DASH:
            updateDash(px, py, dt, playerHP, difficulty);
            break;

        case ESState::PUNISH:
            swordLen = SWORD_LEN;
            punishTimer -= dt;
            if (punishTimer <= 0.0f) {
                ++dashCycle;
                swordState = ESState::IDLE;
                idleTimer = idleTime(difficulty);
                dashHit = false;
            }
            break;
        }
    }

    void updatePatternB(float px, float py, float dt, float& playerHP,
                        std::vector<Bullet>& bullets,
                        Difficulty difficulty) {
        (void)playerHP;
        (void)bullets;

        if (arenaForming) return;

        if (enrage && !bgActive) {
            bgCd = std::min(bgCd, 0.65f);
        }

        if (!bgActive) {
            if (!motionReadyForPhaseSkill() || hasActiveArenaSwordStrike()) return;
            bgCd -= dt;
            if (bgCd <= 0.0f) startBackgroundRoutine(px, py, difficulty);
            return;
        }

        bgFadeAlpha += dt / 0.28f;
        if (bgFadeAlpha > 1.0f) bgFadeAlpha = 1.0f;

        if (bgMode == 0) {
            bgOrbAng += bgOrbDir * dt * 1.35f;
            bgSlashCd -= dt;
            if (bgSlashCd <= 0.0f && bgSpawned < 9) {
                spawnLargeArcSlash(px, py, difficulty);
                bgSlashCd = (difficulty == Difficulty::HARD) ? 0.30f :
                            (difficulty == Difficulty::EASY) ? 0.43f : 0.36f;
            }
        } else {
            bgSlashCd -= dt;
            if (bgSlashCd <= 0.0f && bgSpawned < 6) {
                spawnOuterCrescent(px, py, difficulty);
                bgSlashCd = (difficulty == Difficulty::HARD) ? 0.48f :
                            (difficulty == Difficulty::EASY) ? 0.68f : 0.56f;
            }
        }

        bgTimer -= dt;
        if (bgTimer <= 0.0f) {
            bgActive = false;
            bgFadeAlpha = 0.0f;
            bgCd = enrage ? 0.80f : bgCdBase(difficulty);
            if (!phase3) {
                worldX = screenW * 0.5f + (float)(rand() % 181 - 90);
                worldY = screenH * 0.5f + (float)(rand() % 141 - 70);
                clampBossToScreen();
            }
        }
    }

    void updateArcSlashes(float px, float py, float dt, float& playerHP) {
        for (auto& s : arcSlashes) {
            if (!s.active) continue;
            s.t += dt;
            bool striking = s.t >= s.warn && s.t <= s.warn + s.activeDur;
            if (striking && !s.hit) {
                float dx = px - s.cx, dy = py - s.cy;
                float dist = sqrtf(dx * dx + dy * dy);
                float ang = atan2f(dy, dx);
                if (fabsf(dist - s.radius) <= s.thickness * 0.5f &&
                    fabsf(angleDelta(ang, s.angle)) <= s.halfArc) {
                    HurtPlayer(playerHP, s.damage);
                    s.hit = true;
                }
            }
            if (s.t >= s.warn + s.activeDur + 0.32f)
                s.active = false;
        }
        arcSlashes.erase(std::remove_if(arcSlashes.begin(), arcSlashes.end(),
            [](const ArcSlash& s) { return !s.active; }), arcSlashes.end());
    }

    void updateCrescentSlashes(float px, float py, float dt, float& playerHP) {
        for (auto& s : crescentSlashes) {
            if (!s.active) continue;
            s.prevX = s.x;
            s.prevY = s.y;
            s.speed += s.accel * dt;
            s.radius = std::min(s.maxRadius, s.radius + s.radiusGrow * dt);
            s.x += s.dirX * s.speed * dt;
            s.y += s.dirY * s.speed * dt;
            s.life -= dt;
            if (!s.hit && pointSegmentDist(px, py, s.prevX, s.prevY, s.x, s.y) <= s.radius * 0.55f) {
                HurtPlayer(playerHP, s.damage);
                s.hit = true;
                s.active = false;
            }
            float margin = 180.0f;
            if (s.life <= 0.0f || s.x < -margin || s.x > screenW + margin ||
                s.y < -margin || s.y > screenH + margin) {
                s.active = false;
            }
        }
        crescentSlashes.erase(std::remove_if(crescentSlashes.begin(), crescentSlashes.end(),
            [](const CrescentSlash& s) { return !s.active; }), crescentSlashes.end());
    }

    void updateShadow(float px, float py, float dt,
                      std::vector<Bullet>& bullets,
                      Difficulty difficulty) {
        float acx = screenW * 0.5f, acy = screenH * 0.5f;
        shadowOrbAng -= 320.0f / std::max(120.0f, arenaRadius) * dt;
        shadowX = acx + cosf(shadowOrbAng) * arenaRadius * 0.82f;
        shadowY = acy + sinf(shadowOrbAng) * arenaRadius * 0.82f;

        shadowSlashCd -= dt;
        if (shadowSlashCd <= 0.0f) {
            shadowSlashCd = 0.24f * tempoMul(difficulty);
            float dx = px - shadowX, dy = py - shadowY;
            float d = sqrtf(dx * dx + dy * dy) + 1e-3f;
            spawnEnemyBullet(shadowX, shadowY, dx / d, dy / d,
                             560.0f * speedMul(difficulty),
                             12.0f * damageMul(difficulty),
                             1.16f, glm::vec3(0.30f, 0.50f, 0.86f),
                             bullets);
        }
    }

    void updateTaskKill(float px, float py, float dt, float& playerHP) {
        if (taskKillDone) return;
        taskKillT += dt;

        if (taskKillT >= 3.0f && taskKillT < 3.25f) {
            float prog = (taskKillT - 3.0f) / 0.25f;
            sweepY = prog * (float)screenH;
        } else if (taskKillT >= 3.25f) {
            sweepY = (float)screenH;
        }

        if (taskKillT >= 3.25f) {
            float dx = px - safeX, dy = py - safeY;
            bool inSafe = sqrtf(dx * dx + dy * dy) < 55.0f;
            if (inSafe) {
                alive = false;
                exploded = false;
            } else {
                g_PlayerShield = 0.0f;
                playerHP = 0.0f;
            }
            taskKillDone = true;
        }
    }

    void renderBody(float gt) const {
        float pulse = 0.5f + 0.5f * sinf(gt * 4.0f);

        if (flashT > 0.0f) {
            float fa = std::min(0.48f, flashT * 0.55f);
            drawRect(0.0f, 0.0f, (float)screenW, (float)screenH,
                     flashR, flashG, flashB, fa);
        }

        renderBadSectors();

        if (taskKillActive) {
            renderTaskKill(gt);
            return;
        }

        renderArena(pulse);
        renderArenaOpeningMotion(pulse);
        renderArenaSwordStrikes();
        renderBackgroundCopy(pulse);
        renderArcSlashes();
        renderCrescentSlashes();
        renderDashTrail(pulse);

        if (BodyInvulnerable())
            return;

        renderShadow(pulse, gt);
        renderCore(pulse, gt);
        renderBasicWarning(gt);
        renderSwordState(pulse);
    }

    void renderTaskKill(float gt) const {
        float t = taskKillT;
        drawRect(0.0f, 0.0f, (float)screenW, (float)screenH, 0.0f, 0.0f, 0.0f, 0.34f);

        float charge = clampf(t / 1.4f, 0.0f, 1.0f);
        float cx = screenW * 0.5f;
        float cy = screenH * 0.08f;
        float len = (t >= 1.5f) ? (float)screenW * 2.0f : 160.0f + charge * 160.0f;
        drawRotRect(cx, cy, len, 24.0f + charge * 10.0f, PI * 0.5f,
                    0.48f, 0.78f, 1.0f, 0.88f);
        drawRotRect(cx, cy, len, 5.0f, PI * 0.5f,
                    1.0f, 1.0f, 1.0f, 0.72f);

        if (t >= 2.0f) {
            float pulse = 0.5f + 0.5f * sinf(gt * 6.0f);
            drawCircle(safeX, safeY, 55.0f, 0.10f, 0.88f, 0.28f, 0.34f);
            drawCircle(safeX, safeY, 61.0f, 0.30f, 1.0f, 0.42f, 0.12f + pulse * 0.12f);
            drawLine(safeX - 22.0f, safeY, safeX + 22.0f, safeY,
                     3.5f, 0.30f, 1.0f, 0.42f, 0.86f);
            drawLine(safeX, safeY - 22.0f, safeX, safeY + 22.0f,
                     3.5f, 0.30f, 1.0f, 0.42f, 0.86f);
        }

        if (t >= 3.0f && !taskKillDone) {
            drawRect(0.0f, 0.0f, (float)screenW, sweepY,
                     0.55f, 0.82f, 1.0f, 0.72f);
            drawRect(0.0f, sweepY - 10.0f, (float)screenW, 20.0f,
                     1.0f, 1.0f, 1.0f, 0.52f);
        }
    }

private:
    static float clampf(float v, float lo, float hi) {
        if (hi < lo) return lo;
        return std::max(lo, std::min(v, hi));
    }

    static float wrapPi(float a) {
        while (a > PI) a -= TAU;
        while (a < -PI) a += TAU;
        return a;
    }

    static float angleDelta(float a, float b) {
        return wrapPi(a - b);
    }

    static float pointSegmentDist(float px, float py,
                                  float ax, float ay,
                                  float bx, float by) {
        float vx = bx - ax, vy = by - ay;
        float wx = px - ax, wy = py - ay;
        float len2 = vx * vx + vy * vy;
        if (len2 < 0.001f) {
            float dx = px - ax, dy = py - ay;
            return sqrtf(dx * dx + dy * dy);
        }
        float t = (wx * vx + wy * vy) / len2;
        t = clampf(t, 0.0f, 1.0f);
        float cx = ax + vx * t, cy = ay + vy * t;
        float dx = px - cx, dy = py - cy;
        return sqrtf(dx * dx + dy * dy);
    }

    bool motionReadyForPhaseSkill() const {
        return swordState == ESState::IDLE || swordState == ESState::PUNISH;
    }

    bool hasActiveArenaSwordStrike() const {
        for (const auto& s : arenaSwordStrikes) {
            if (s.active) return true;
        }
        return false;
    }

    bool hasPendingArenaSwordStrike() const {
        for (const auto& s : arenaSwordStrikes) {
            if (s.active && !s.launched) return true;
        }
        return false;
    }

    float arenaLaunchDelay(Difficulty difficulty) const {
        float base = 0.95f + (float)(rand() % 120) * 0.01f;
        if (phase3) base *= 0.86f;
        if (enrage) base *= 0.64f;
        return std::max(0.42f, base * tempoMul(difficulty));
    }

    void resetArenaSwords() {
        arenaSwordCount = ARENA_SWORD_MAX;
    }

    void beginArenaFormation(Difficulty difficulty) {
        arenaActive = true;
        arenaForming = true;
        arenaFormT = 0.0f;
        arenaRebuildTimer = 0.0f;
        resetArenaSwords();
        arenaSwordStrikes.clear();
        arenaLaunchCd = arenaLaunchDelay(difficulty);
        bgActive = false;
        bgFadeAlpha = 0.0f;
        bgCd = std::max(1.8f, std::min(bgCd, bgCdBase(difficulty)));
        arcSlashes.clear();
        crescentSlashes.clear();
        swordState = ESState::IDLE;
        idleTimer = ARENA_FORM_DUR + 0.20f;
        chargeTimer = 0.0f;
        dashTimer = 0.0f;
        dashTravelLeft = 0.0f;
        dashTrailT = 0.0f;
        dashShockT = 0.0f;
        dashHit = false;
    }

    void breakArena() {
        arenaActive = false;
        arenaForming = false;
        arenaFormT = ARENA_FORM_DUR;
        arenaSwordCount = 0;
        arenaLaunchCd = 0.0f;
        arenaRebuildTimer = 10.0f;
        bgActive = false;
        bgFadeAlpha = 0.0f;
        arcSlashes.clear();
        crescentSlashes.clear();
        flashT = std::max(flashT, 0.22f);
        flashR = 0.30f; flashG = 0.58f; flashB = 1.0f;
    }

    void updateArenaOpeningMotion(float dt) {
        float acx = screenW * 0.5f;
        float acy = screenH * 0.5f;
        float p = clampf(arenaFormT / ARENA_FORM_DUR, 0.0f, 1.0f);
        float targetX = acx;
        float targetY = acy - arenaRadius * (0.18f + 0.14f * (1.0f - p));
        float pull = clampf(dt * (1.8f + p * 1.6f), 0.0f, 1.0f);
        worldX += (targetX - worldX) * pull;
        worldY += (targetY - worldY) * pull;
        swordAngle = -PI * 0.5f + sinf(barrierRot * 1.4f) * 0.08f;
        swordLen = SWORD_LEN;
        dashTravelLeft = 0.0f;
        dashTrailT = 0.0f;
        dashShockT = 0.0f;
    }

    void arenaSwordPos(int slot, int count, float radius, float& x, float& y) const {
        float acx = screenW * 0.5f, acy = screenH * 0.5f;
        int safeCount = std::max(1, count);
        float ang = (float)slot / (float)safeCount * TAU + barrierRot;
        x = acx + cosf(ang) * radius;
        y = acy + sinf(ang) * radius;
    }

    int pickArenaSwordSlot() const {
        if (arenaSwordCount <= 0) return -1;
        return rand() % arenaSwordCount;
    }

    void spendArenaSwordSlot(int slot) {
        (void)slot;
        if (arenaSwordCount <= 0) return;
        arenaSwordCount = std::max(0, arenaSwordCount - 1);
        if (arenaSwordCount <= 0)
            breakArena();
    }

    void spawnArenaSwordStrike(float px, float py, Difficulty difficulty) {
        int slot = pickArenaSwordSlot();
        if (slot < 0) {
            breakArena();
            return;
        }

        float sx = 0.0f, sy = 0.0f;
        arenaSwordPos(slot, arenaSwordCount, arenaRadius, sx, sy);
        float dx = px - sx, dy = py - sy;
        float dist = sqrtf(dx * dx + dy * dy) + 1e-3f;

        ArenaSwordStrike s{};
        s.slot = slot;
        s.sx = s.x = s.prevX = sx;
        s.sy = s.y = s.prevY = sy;
        s.dirX = dx / dist;
        s.dirY = dy / dist;
        s.warn = 0.58f * warnMul(difficulty);
        s.speed = (phase3 ? 1040.0f : 920.0f) * speedMul(difficulty);
        s.damage = (phase3 ? 14.0f : 12.0f) * damageMul(difficulty);
        s.life = 1.95f;
        arenaSwordStrikes.push_back(s);

        if (swordState == ESState::IDLE)
            idleTimer = std::max(idleTimer, s.warn + 0.14f);
    }

    void updateArenaSwords(float px, float py, float dt, float& playerHP,
                           Difficulty difficulty) {
        for (auto& s : arenaSwordStrikes) {
            if (!s.active) continue;
            s.t += dt;

            if (!s.launched && s.t >= s.warn) {
                s.launched = true;
                s.x = s.prevX = s.sx;
                s.y = s.prevY = s.sy;
                spendArenaSwordSlot(s.slot);
            }

            if (!s.launched) continue;

            s.prevX = s.x;
            s.prevY = s.y;
            s.x += s.dirX * s.speed * dt;
            s.y += s.dirY * s.speed * dt;
            s.life -= dt;

            if (!s.hit && pointSegmentDist(px, py, s.prevX, s.prevY, s.x, s.y) <= 25.0f) {
                HurtPlayer(playerHP, s.damage);
                s.hit = true;
            }

            float margin = 220.0f;
            if (s.life <= 0.0f || s.x < -margin || s.x > screenW + margin ||
                s.y < -margin || s.y > screenH + margin) {
                s.active = false;
            }
        }

        arenaSwordStrikes.erase(std::remove_if(arenaSwordStrikes.begin(), arenaSwordStrikes.end(),
            [](const ArenaSwordStrike& s) { return !s.active; }), arenaSwordStrikes.end());

        if (!phase2 || taskKillActive) return;

        if (!arenaActive && !arenaForming) {
            arenaRebuildTimer -= dt;
            if (arenaRebuildTimer <= 0.0f && motionReadyForPhaseSkill() && !bgActive) {
                beginArenaFormation(difficulty);
                flashT = std::max(flashT, 0.30f);
                flashR = 0.52f; flashG = 0.76f; flashB = 1.0f;
            }
            return;
        }

        if (!arenaActive || arenaForming || bgActive ||
            !motionReadyForPhaseSkill() || hasPendingArenaSwordStrike())
            return;

        arenaLaunchCd -= dt;
        if (arenaLaunchCd <= 0.0f && arenaSwordCount > 0) {
            spawnArenaSwordStrike(px, py, difficulty);
            arenaLaunchCd = arenaLaunchDelay(difficulty);
        }
    }

    float tempoMul(Difficulty difficulty) const {
        if (difficulty == Difficulty::EASY) return 1.18f;
        if (difficulty == Difficulty::HARD) return 0.86f;
        return 1.0f;
    }

    float warnMul(Difficulty difficulty) const {
        if (difficulty == Difficulty::EASY) return 1.22f;
        if (difficulty == Difficulty::HARD) return 0.88f;
        return 1.0f;
    }

    float damageMul(Difficulty difficulty) const {
        if (difficulty == Difficulty::EASY) return 0.82f;
        if (difficulty == Difficulty::HARD) return 1.12f;
        return 1.0f;
    }

    float speedMul(Difficulty difficulty) const {
        if (difficulty == Difficulty::EASY) return 0.92f;
        if (difficulty == Difficulty::HARD) return 1.08f;
        return 1.0f;
    }

    float idleTime(Difficulty difficulty) const {
        float base = enrage ? 0.10f : phase3 ? 0.26f : phase2 ? 0.34f : 0.46f;
        return base * tempoMul(difficulty);
    }

    float chargeTime(Difficulty difficulty) const {
        float base = enrage ? 0.40f : phase3 ? 0.50f : phase2 ? 0.60f : 0.72f;
        return base * warnMul(difficulty);
    }

    float punishTime(Difficulty difficulty) const {
        float base = enrage ? 0.52f : phase3 ? 0.68f : phase2 ? 0.82f : 1.00f;
        if (difficulty == Difficulty::EASY) base += 0.08f;
        if (difficulty == Difficulty::HARD) base -= 0.06f;
        return std::max(0.42f, base);
    }

    float bgCdBase(Difficulty difficulty) const {
        float base = phase3 ? 5.0f : 7.0f;
        return base * tempoMul(difficulty);
    }

    float bgDuration() const {
        return phase3 ? 6.0f : 4.5f;
    }

    float bgFireInterval(Difficulty difficulty) const {
        float base = enrage ? 0.24f : phase3 ? 0.20f : 0.30f;
        return base * tempoMul(difficulty);
    }

    float bgBulletDamage(Difficulty difficulty) const {
        float base = enrage ? 10.0f : phase3 ? 12.0f : 9.5f;
        return base * damageMul(difficulty);
    }

    float bgBulletSpeed(Difficulty difficulty) const {
        float base = enrage ? 560.0f : phase3 ? 610.0f : 540.0f;
        return base * speedMul(difficulty);
    }

    float orbitHalfArc() const {
        return phase3 ? 0.60f : 0.52f;
    }

    bool shouldUseBasic(Difficulty difficulty) const {
        if (enrage) return difficulty == Difficulty::HARD && (dashCycle % 2 == 0);
        if (difficulty == Difficulty::EASY && phase2 && !phase3) return (dashCycle % 2 == 0);
        return true;
    }

    void updatePhases(std::vector<Bullet>& bullets, float px, float py, float dt,
                      Difficulty difficulty) {
        if (!phase2 && hp <= maxHp * 0.65f) {
            phase2 = true;
            flashT = 0.4f; flashR = 0.55f; flashG = 0.80f; flashB = 1.0f;
            beginArenaFormation(difficulty);
        }
        if (arenaForming) {
            arenaFormT += dt;
            if (arenaFormT >= ARENA_FORM_DUR) arenaForming = false;
        }
        if (!phase3 && hp <= maxHp * 0.35f) {
            phase3 = true;
            flashT = 0.6f; flashR = 0.55f; flashG = 0.80f; flashB = 1.0f;
        }
        if (!enrage && hp <= maxHp * 0.10f) {
            enrage = true;
            flashT = 1.0f; flashR = 1.0f; flashG = 0.15f; flashB = 0.15f;
            shadowOrbAng = 0.0f;
            shadowSlashCd = 0.20f;
        }
        if (hp <= taskKillHp()) {
            BeginTaskKill(px, py, bullets);
        }
    }

    void aimSwordAt(float px, float py, float turn) {
        float target = atan2f(py - worldY, px - worldX);
        swordAngle += angleDelta(target, swordAngle) * clampf(turn, 0.0f, 1.0f);
    }

    void clampBossToScreen() {
        float m = HIT_RADIUS + 6.0f;
        worldX = clampf(worldX, m, (float)screenW - m);
        worldY = clampf(worldY, m, (float)screenH - m);
    }

    float dashOvershoot(Difficulty difficulty) const {
        float base = phase2 ? 76.0f : 68.0f;
        if (difficulty == Difficulty::EASY) base -= 10.0f;
        if (difficulty == Difficulty::HARD) base += 14.0f;
        return base;
    }

    float dashTravelToPlayerBack(float px, float py, Difficulty difficulty) const {
        float dx = cosf(swordAngle), dy = sinf(swordAngle);
        float forward = (px - worldX) * dx + (py - worldY) * dy;
        float diag = sqrtf((float)screenW * screenW + (float)screenH * screenH);
        float minTravel = phase2 ? 120.0f : 145.0f;
        return clampf(forward + dashOvershoot(difficulty), minTravel, diag);
    }

    void updateDashPreview(float px, float py, Difficulty difficulty) {
        float travel = dashTravelToPlayerBack(px, py, difficulty);
        dashPreviewX = worldX + cosf(swordAngle) * travel;
        dashPreviewY = worldY + sinf(swordAngle) * travel;
    }

    void startCharge(float px, float py, Difficulty difficulty) {
        swordState = ESState::CHARGE;
        chargeTimer = chargeTime(difficulty);
        chargeTotal = chargeTimer;
        chargeSpinDir = (rand() % 2) ? 1.0f : -1.0f;
        dashHit = false;
        aimSwordAt(px, py, 1.0f);
        updateDashPreview(px, py, difficulty);
        swordLen = SWORD_LEN;
    }

    void startBasicAttack(float px, float py, Difficulty difficulty) {
        basic = BasicAttack{};
        if (!phase2) {
            int pick = dashCycle % ((difficulty == Difficulty::EASY) ? 3 : 4);
            basic.type = (pick == 0 || pick == 2) ? ESBasicType::EDGE_SLASH :
                         (pick == 1) ? ESBasicType::ORBIT_CUT :
                                       ESBasicType::CROSS_CHECK;
        } else {
            int pick = dashCycle % 3;
            if (difficulty == Difficulty::EASY && !phase3 && pick == 2) pick = 0;
            basic.type = (pick == 0) ? ESBasicType::EDGE_SLASH :
                         (pick == 1) ? ESBasicType::ORBIT_CUT :
                                       ESBasicType::CROSS_CHECK;
        }

        basic.x = worldX;
        basic.y = worldY;
        basic.angle = atan2f(py - worldY, px - worldX);
        basic.warn = 0.55f * warnMul(difficulty);
        basic.dur = phase3 ? 0.34f : phase2 ? 0.28f : 0.30f;

        if (basic.type == ESBasicType::ORBIT_CUT) {
            float acx = screenW * 0.5f, acy = screenH * 0.5f;
            float rx = px - acx, ry = py - acy;
            float vx = px - prevPX, vy = py - prevPY;
            float cross = rx * vy - ry * vx;
            float dir = (fabsf(vx) + fabsf(vy) > 0.5f)
                      ? (cross >= 0.0f ? 1.0f : -1.0f)
                      : ((rand() % 2) ? 1.0f : -1.0f);
            basic.x = acx;
            basic.y = acy;
            basic.radius = clampf(sqrtf(rx * rx + ry * ry), 120.0f,
                                  std::max(140.0f, arenaRadius - 36.0f));
            basic.angle = atan2f(ry, rx) + dir * 0.52f;
            basic.warn = 0.65f * warnMul(difficulty);
            basic.dur = 0.28f;
        } else if (basic.type == ESBasicType::CROSS_CHECK) {
            basic.x = px;
            basic.y = py;
            basic.warn = 0.75f * warnMul(difficulty);
            basic.dur = 0.44f;
        }

        comboVisualLock();
        swordState = ESState::BASIC_ATTACK;
    }

    void comboVisualLock() {
        swordAngle = basic.angle;
        swordLen = SWORD_LEN;
    }

    void updateBasicAttack(float px, float py, float dt, float& playerHP,
                           std::vector<Bullet>& bullets,
                           Difficulty difficulty) {
        basic.t += dt;
        swordLen = SWORD_LEN;

        if (basic.type == ESBasicType::EDGE_SLASH) {
            swordAngle = basic.angle;
            if (!basic.fired && basic.t >= basic.warn) {
                basic.fired = true;
                int sideLanes = (!phase2 || difficulty == Difficulty::EASY) ? 0 : 1;
                for (int i = -sideLanes; i <= sideLanes; ++i) {
                    float a = basic.angle + (float)i * 0.11f;
                    spawnEnemyBullet(worldX, worldY, cosf(a), sinf(a),
                                     710.0f * speedMul(difficulty),
                                     13.0f * damageMul(difficulty),
                                     1.26f, glm::vec3(0.72f, 0.92f, 1.0f),
                                     bullets);
                }
            }
            if (!basic.hitA && basic.t >= basic.warn + 0.11f) {
                basic.hitA = true;
                spawnEdgeFollowShots(px, py, difficulty, bullets);
            }
        } else if (basic.type == ESBasicType::ORBIT_CUT) {
            swordAngle += dt * 2.5f;
            if (basic.t >= basic.warn && basic.t <= basic.warn + basic.dur && !basic.hitA) {
                float dx = px - basic.x, dy = py - basic.y;
                float dist = sqrtf(dx * dx + dy * dy);
                float a = atan2f(dy, dx);
                if (fabsf(dist - basic.radius) <= 38.0f &&
                    fabsf(angleDelta(a, basic.angle)) <= orbitHalfArc()) {
                    HurtPlayer(playerHP, 16.0f * damageMul(difficulty));
                    basic.hitA = true;
                }
            }
        } else {
            swordAngle = basic.angle;
            float lineLen = sqrtf((float)screenW * screenW + (float)screenH * screenH);
            float ca = cosf(basic.angle), sa = sinf(basic.angle);
            if (basic.t >= basic.warn && basic.t <= basic.warn + 0.10f && !basic.hitA) {
                float d = pointSegmentDist(px, py,
                                           basic.x - ca * lineLen, basic.y - sa * lineLen,
                                           basic.x + ca * lineLen, basic.y + sa * lineLen);
                if (d <= 30.0f) {
                    HurtPlayer(playerHP, 14.0f * damageMul(difficulty));
                    basic.hitA = true;
                }
            }
            if (basic.t >= basic.warn + 0.18f && basic.t <= basic.warn + 0.30f && !basic.hitB) {
                float ca2 = -sa, sa2 = ca;
                float d = pointSegmentDist(px, py,
                                           basic.x - ca2 * lineLen, basic.y - sa2 * lineLen,
                                           basic.x + ca2 * lineLen, basic.y + sa2 * lineLen);
                if (d <= 27.0f) {
                    HurtPlayer(playerHP, 10.0f * damageMul(difficulty));
                    basic.hitB = true;
                }
            }
        }

        if (basic.t >= basic.warn + basic.dur) {
            startCharge(px, py, difficulty);
        }
    }

    void updateCharge(float px, float py, float dt, Difficulty difficulty) {
        float total = std::max(0.001f, chargeTotal);
        float lockWindow = 0.16f * warnMul(difficulty);
        if (chargeTimer > lockWindow) {
            aimSwordAt(px, py, dt * 9.0f);
        }
        updateDashPreview(px, py, difficulty);
        float cp = 1.0f - clampf(chargeTimer / total, 0.0f, 1.0f);
        swordLen = SWORD_LEN;
        chargeTimer -= dt;
        if (chargeTimer <= 0.0f) {
            float dx = cosf(swordAngle), dy = sinf(swordAngle);
            float dashSpd = DASH_SPEED * speedMul(difficulty);
            dashVX = dx * dashSpd;
            dashVY = dy * dashSpd;
            dashTravelLeft = dashTravelToPlayerBack(px, py, difficulty);
            dashPreviewX = worldX + dx * dashTravelLeft;
            dashPreviewY = worldY + dy * dashTravelLeft;
            dashTimer = dashTravelLeft / std::max(1.0f, dashSpd) + 0.08f;
            dashStartX = worldX;
            dashStartY = worldY;
            dashEndX = worldX;
            dashEndY = worldY;
            dashTrailT = 0.30f;
            dashShockT = 0.0f;
            swordLen = SWORD_LEN;
            dashHit = false;
            swordState = ESState::DASH;
        }
    }

    void updateDash(float px, float py, float dt, float& playerHP, Difficulty difficulty) {
        dashTimer -= dt;
        float oldX = worldX, oldY = worldY;
        float dashSpd = sqrtf(dashVX * dashVX + dashVY * dashVY);
        float step = std::min(dashTravelLeft, dashSpd * dt);
        float nx = dashSpd > 1.0f ? dashVX / dashSpd : cosf(swordAngle);
        float ny = dashSpd > 1.0f ? dashVY / dashSpd : sinf(swordAngle);
        worldX += nx * step;
        worldY += ny * step;
        dashTravelLeft = std::max(0.0f, dashTravelLeft - step);

        bool hitWall = false;
        float m = HIT_RADIUS + 4.0f;
        if (worldX < m) { worldX = m; hitWall = true; }
        if (worldX > screenW - m) { worldX = (float)screenW - m; hitWall = true; }
        if (worldY < m) { worldY = m; hitWall = true; }
        if (worldY > screenH - m) { worldY = (float)screenH - m; hitWall = true; }

        dashEndX = worldX;
        dashEndY = worldY;

        float ca = cosf(swordAngle), sa = sinf(swordAngle);
        float ax = worldX - ca * swordLen * 0.34f;
        float ay = worldY - sa * swordLen * 0.34f;
        float bx = worldX + ca * swordLen * 0.56f;
        float by = worldY + sa * swordLen * 0.56f;
        float bodyPath = pointSegmentDist(px, py, oldX, oldY, worldX, worldY);
        float bladePath = pointSegmentDist(px, py, ax, ay, bx, by);
        if (!dashHit && std::min(bodyPath, bladePath) <= 36.0f) {
            HurtPlayer(playerHP, 30.0f * damageMul(difficulty));
            dashHit = true;
            dashTravelLeft = 0.0f;
            dashTimer = 0.0f;
        }

        if (hitWall || dashTravelLeft <= 0.5f || dashTimer <= 0.0f)
            enterPunish(difficulty);
    }

    void enterPunish(Difficulty difficulty) {
        if (enrage) {
            float tx = clampf(worldX + cosf(swordAngle) * 70.0f, 40.0f, (float)screenW - 40.0f);
            float ty = clampf(worldY + sinf(swordAngle) * 70.0f, 40.0f, (float)screenH - 40.0f);
            badSectors[badIdx] = { tx, ty, true };
            badIdx = (badIdx + 1) % MAX_BAD;
        }
        swordState = ESState::PUNISH;
        punishTimer = punishTime(difficulty);
        dashEndX = worldX;
        dashEndY = worldY;
        dashTrailT = std::max(dashTrailT, 0.24f);
        dashShockT = 0.18f;
        dashTravelLeft = 0.0f;
        swordLen = SWORD_LEN;
    }

    void spawnLargeArcSlash(float px, float py, Difficulty difficulty) {
        float acx = screenW * 0.5f, acy = screenH * 0.5f;
        if (bgSpawned == 0 && !phase3) {
            worldX = acx;
            worldY = acy;
        }

        ArcSlash s{};
        s.cx = worldX;
        s.cy = worldY;
        float baseAim = atan2f(py - s.cy, px - s.cx);
        int step = bgSpawned++;
        s.radius = clampf(arenaRadius * (0.43f + 0.10f * (float)(step % 4)),
                          150.0f, std::max(160.0f, arenaRadius - 54.0f));
        s.angle = baseAim + bgOrbDir * (0.72f * (float)step + ((step % 2) ? 0.20f : -0.16f));
        s.halfArc = phase3 ? 1.12f : 1.02f;
        s.warn = (phase3 ? 0.64f : 0.76f) * warnMul(difficulty);
        s.activeDur = 0.16f;
        s.thickness = (phase3 ? 32.0f : 28.0f) * (difficulty == Difficulty::EASY ? 0.88f : 1.0f);
        s.damage = (phase3 ? 16.0f : 13.0f) * damageMul(difficulty);
        arcSlashes.push_back(s);
        if (arcSlashes.size() > 18)
            arcSlashes.erase(arcSlashes.begin());
    }

    void spawnOuterCrescent(float px, float py, Difficulty difficulty) {
        float acx = screenW * 0.5f, acy = screenH * 0.5f;
        float a = ((float)(rand() % 6283) * 0.001f) + (float)bgSpawned * 0.43f;
        float r = arenaRadius * (1.08f + (float)(rand() % 12) * 0.01f);
        worldX = clampf(acx + cosf(a) * r, 72.0f, (float)screenW - 72.0f);
        worldY = clampf(acy + sinf(a) * r, 72.0f, (float)screenH - 72.0f);
        bgX = worldX;
        bgY = worldY;

        float dx = px - worldX, dy = py - worldY;
        float d = sqrtf(dx * dx + dy * dy) + 1e-3f;
        CrescentSlash s{};
        s.x = s.prevX = worldX;
        s.y = s.prevY = worldY;
        s.dirX = dx / d;
        s.dirY = dy / d;
        s.speed = (phase3 ? 580.0f : 520.0f) * speedMul(difficulty);
        s.accel = (phase3 ? 360.0f : 285.0f) * speedMul(difficulty);
        s.maxLife = s.life = 1.35f;
        s.damage = (phase3 ? 17.0f : 14.0f) * damageMul(difficulty);
        s.radius = phase3 ? 40.0f : 34.0f;
        s.maxRadius = phase3 ? 72.0f : 60.0f;
        s.radiusGrow = (s.maxRadius - s.radius) / std::max(0.1f, s.maxLife);
        crescentSlashes.push_back(s);
        if (crescentSlashes.size() > 16)
            crescentSlashes.erase(crescentSlashes.begin());
        ++bgSpawned;
    }

    void startBackgroundRoutine(float px, float py, Difficulty difficulty) {
        bgActive = true;
        bgMode = bgCycle++ % 2;
        if (enrage && (bgCycle % 3) == 0) bgMode = 0;
        bgTimer = (bgMode == 0) ? (phase3 ? 4.2f : 3.8f) : (phase3 ? 3.7f : 3.2f);
        bgSlashCd = 0.05f;
        bgFadeAlpha = 0.0f;
        bgOrbDir = (rand() % 2) ? 1.0f : -1.0f;
        bgOrbAng = (float)(rand() % 628) * 0.01f;
        bgSpawned = 0;
        if (bgMode == 0) {
            bgX = worldX;
            bgY = worldY;
            spawnLargeArcSlash(px, py, difficulty);
            bgSlashCd = 0.30f;
        } else {
            spawnOuterCrescent(px, py, difficulty);
            bgSlashCd = 0.48f;
        }
    }

    void spawnEdgeFollowShots(float px, float py, Difficulty difficulty,
                              std::vector<Bullet>& bullets) {
        float vx = px - prevPX;
        float vy = py - prevPY;
        float lead = (difficulty == Difficulty::HARD) ? 0.36f :
                     (difficulty == Difficulty::EASY) ? 0.16f : 0.26f;
        float tx = clampf(px + vx * lead, 24.0f, (float)screenW - 24.0f);
        float ty = clampf(py + vy * lead, 24.0f, (float)screenH - 24.0f);
        float baseAim = atan2f(ty - worldY, tx - worldX);
        float perX = -sinf(baseAim), perY = cosf(baseAim);
        float offset = phase3 ? 64.0f : phase2 ? 56.0f : 46.0f;
        float speed = (phase3 ? 790.0f : phase2 ? 750.0f : 720.0f) * speedMul(difficulty);
        float damage = (phase3 ? 9.5f : 8.5f) * damageMul(difficulty);

        for (int side = -1; side <= 1; side += 2) {
            float ox = worldX + perX * offset * (float)side;
            float oy = worldY + perY * offset * (float)side;
            float dx = tx - ox;
            float dy = ty - oy;
            float d = sqrtf(dx * dx + dy * dy) + 1e-3f;
            spawnEnemyBullet(ox, oy, dx / d, dy / d,
                             speed, damage, 1.08f,
                             glm::vec3(0.58f, 0.82f, 1.0f),
                             bullets);
        }

        if (phase2 && difficulty != Difficulty::EASY) {
            spawnEnemyBullet(worldX, worldY, cosf(baseAim), sinf(baseAim),
                             speed * 0.82f, damage * 0.78f, 0.96f,
                             glm::vec3(0.42f, 0.68f, 1.0f),
                             bullets);
        }
    }

    static void spawnEnemyBullet(float x, float y, float dx, float dy,
                                 float speed, float damage, float sizeScale,
                                 const glm::vec3& color,
                                 std::vector<Bullet>& bullets) {
        Bullet b(x, y, x + dx * 100.0f, y + dy * 100.0f);
        b.isEnemy = true;
        b.speed = speed;
        b.enemyDmg = damage;
        b.sizeScale = sizeScale;
        b.color = color;
        bullets.push_back(b);
    }

    static void drawRotRect(float cx, float cy, float w, float h, float ang,
                            float r, float g, float b, float a) {
        float hx = w * 0.5f, hy = h * 0.5f;
        float ca = cosf(ang), sa = sinf(ang);
        float xs[4] = { -hx, -hx, hx, hx };
        float ys[4] = { -hy, hy, hy, -hy };
        float vx[4], vy[4];
        for (int i = 0; i < 4; i++) {
            vx[i] = cx + xs[i] * ca - ys[i] * sa;
            vy[i] = cy + xs[i] * sa + ys[i] * ca;
        }
        BatchTri(vx[0], vy[0], vx[1], vy[1], vx[2], vy[2], r, g, b, a);
        BatchTri(vx[0], vy[0], vx[2], vy[2], vx[3], vy[3], r, g, b, a);
    }

    static void drawLine(float x0, float y0, float x1, float y1, float thick,
                         float r, float g, float b, float a) {
        float dx = x1 - x0, dy = y1 - y0;
        float l = sqrtf(dx * dx + dy * dy);
        if (l < 0.5f) return;
        drawRotRect((x0 + x1) * 0.5f, (y0 + y1) * 0.5f, l, thick,
                    atan2f(dy, dx), r, g, b, a);
    }

    static void drawOctagon(float cx, float cy, float rad,
                            float r, float g, float b, float a) {
        for (int i = 0; i < 8; i++) {
            float a0 = (float)i / 8.0f * TAU;
            float a1 = (float)(i + 1) / 8.0f * TAU;
            BatchTri(cx, cy,
                     cx + cosf(a0) * rad, cy + sinf(a0) * rad,
                     cx + cosf(a1) * rad, cy + sinf(a1) * rad,
                     r, g, b, a);
        }
    }

    static void drawArcLine(float cx, float cy, float radius,
                            float center, float halfArc, float thick,
                            float r, float g, float b, float a) {
        const int segs = 18;
        float lastA = center - halfArc;
        float lastX = cx + cosf(lastA) * radius;
        float lastY = cy + sinf(lastA) * radius;
        for (int i = 1; i <= segs; ++i) {
            float t = (float)i / (float)segs;
            float ang = center - halfArc + halfArc * 2.0f * t;
            float nx = cx + cosf(ang) * radius;
            float ny = cy + sinf(ang) * radius;
            drawLine(lastX, lastY, nx, ny, thick, r, g, b, a);
            lastX = nx;
            lastY = ny;
        }
    }

    static void drawSword(float cx, float cy, float len, float ang,
                          float r, float g, float b, float a) {
        float cosA = cosf(ang), sinA = sinf(ang);
        float perX = -sinA, perY = cosA;

        float guardOff = len * 0.05f;
        float hiltLen = len * 0.5f - guardOff;
        float bladeLen = len * 0.5f + guardOff;
        float bladeMid = len * 0.25f - guardOff * 0.5f;
        float tipExt = 16.0f;
        float tipBW = 4.0f;

        drawRotRect(cx, cy, len + 20.0f, 30.0f, ang, r * 0.25f, g * 0.25f, b, 0.12f);

        float bladeX = cx + cosA * bladeMid;
        float bladeY = cy + sinA * bladeMid;
        drawRotRect(bladeX, bladeY, bladeLen, 9.0f, ang, r, g, b, a);
        drawRotRect(bladeX, bladeY, bladeLen, 2.2f, ang, 1.0f, 1.0f, 1.0f, a * 0.52f);

        float tipX = cx + cosA * (len * 0.5f);
        float tipY = cy + sinA * (len * 0.5f);
        BatchTri(tipX + perX * tipBW, tipY + perY * tipBW,
                 tipX - perX * tipBW, tipY - perY * tipBW,
                 tipX + cosA * tipExt, tipY + sinA * tipExt,
                 r, g, b, a * 0.85f);

        float gx = cx - cosA * guardOff;
        float gy = cy - sinA * guardOff;
        drawRotRect(gx, gy, 5.5f, 38.0f, ang, 0.55f, 0.72f, 0.95f, a * 0.80f);
        drawRotRect(gx, gy, 4.0f, 26.0f, ang, 0.90f, 0.95f, 1.0f, a * 0.95f);

        float hx = cx - cosA * (guardOff + hiltLen * 0.5f);
        float hy = cy - sinA * (guardOff + hiltLen * 0.5f);
        drawRotRect(hx, hy, hiltLen, 5.5f, ang, 0.32f, 0.46f, 0.72f, a * 0.90f);

        float pomX = cx - cosA * (guardOff + hiltLen);
        float pomY = cy - sinA * (guardOff + hiltLen);
        drawCircle(pomX, pomY, 7.5f, r, g, b, a * 0.80f);
    }

    void renderBadSectors() const {
        if (!enrage) return;
        for (const auto& bs : badSectors) {
            if (!bs.active) continue;
            drawRect(bs.x - 40.0f, bs.y - 40.0f, 80.0f, 80.0f,
                     0.85f, 0.10f, 0.12f, 0.28f);
            drawNeonBorder(bs.x - 40.0f, bs.y - 40.0f, 80.0f, 80.0f,
                           1.0f, 0.22f, 0.22f);
        }
    }

    void renderArena(float pulse) const {
        if (!arenaActive && !arenaForming) return;

        float acx = screenW * 0.5f, acy = screenH * 0.5f;
        float barrierR = arenaRadius;
        if (arenaForming) {
            float p = clampf(arenaFormT / ARENA_FORM_DUR, 0.0f, 1.0f);
            float smooth = p * p * (3.0f - 2.0f * p);
            float startR = arenaRadius / ARENA_FACTOR * ARENA_FORM_MAXR;
            barrierR = startR + (arenaRadius - startR) * smooth;
        }
        int visibleSwords = std::max(0, std::min(ARENA_SWORD_MAX, arenaSwordCount));
        for (int i = 0; i < visibleSwords; i++) {
            float ang = (float)i / (float)std::max(1, visibleSwords) * TAU + barrierRot;
            float bx = acx + cosf(ang) * barrierR;
            float by = acy + sinf(ang) * barrierR;
            drawRotRect(bx, by, 70.0f, 18.0f, ang + PI * 0.5f,
                        0.28f, 0.52f, 0.90f, 0.16f);
            drawRotRect(bx, by, 52.0f, 11.0f, ang + PI * 0.5f,
                        0.55f, 0.80f, 1.0f, 0.78f);
            drawCircle(acx + cosf(ang) * (barrierR - 30.0f),
                       acy + sinf(ang) * (barrierR - 30.0f),
                       4.0f, 0.80f, 0.95f, 1.0f, 0.55f + pulse * 0.30f);
        }
        drawCircle(acx, acy, barrierR - 6.0f, 0.35f, 0.58f, 0.90f, 0.08f);
    }

    void renderArenaOpeningMotion(float pulse) const {
        if (!arenaForming) return;

        float p = clampf(arenaFormT / ARENA_FORM_DUR, 0.0f, 1.0f);
        float acx = screenW * 0.5f;
        float acy = screenH * 0.5f;
        float ringR = arenaRadius * (0.24f + 0.76f * p);
        float alpha = 1.0f - clampf((p - 0.78f) / 0.22f, 0.0f, 1.0f);

        drawLine(worldX, worldY, acx, acy, 18.0f,
                 0.20f, 0.46f, 0.95f, 0.08f + 0.12f * alpha);
        drawLine(worldX, worldY, acx, acy, 3.0f,
                 0.86f, 0.96f, 1.0f, 0.34f * alpha);
        drawCircle(worldX, worldY, 42.0f + p * 58.0f,
                   0.38f, 0.66f, 1.0f, 0.08f + pulse * 0.05f);
        drawArcLine(acx, acy, ringR, barrierRot, PI,
                    5.0f + p * 6.0f, 0.62f, 0.84f, 1.0f, 0.22f * alpha);
        drawArcLine(acx, acy, ringR * 0.82f, -barrierRot * 0.7f, PI,
                    2.5f, 1.0f, 1.0f, 1.0f, 0.18f * alpha);

        for (int i = 0; i < 6; ++i) {
            float a = barrierRot * 1.55f + (float)i * TAU / 6.0f;
            float rr = 48.0f + p * 34.0f + pulse * 4.0f;
            float sx = worldX + cosf(a) * rr;
            float sy = worldY + sinf(a) * rr;
            drawSword(sx, sy, 82.0f + p * 22.0f, a + PI * 0.5f,
                      0.56f, 0.80f, 1.0f, 0.34f * alpha);
        }
    }

    void renderArenaSwordStrikes() const {
        if (arenaSwordStrikes.empty()) return;
        float diag = sqrtf((float)screenW * screenW + (float)screenH * screenH);
        for (const auto& s : arenaSwordStrikes) {
            if (!s.active) continue;
            float laneX = s.sx + s.dirX * diag;
            float laneY = s.sy + s.dirY * diag;
            float ang = atan2f(s.dirY, s.dirX);

            if (!s.launched) {
                float p = clampf(s.t / std::max(0.001f, s.warn), 0.0f, 1.0f);
                drawLine(s.sx, s.sy, laneX, laneY, 24.0f + p * 8.0f,
                         0.95f, 0.42f, 0.22f, 0.12f + p * 0.16f);
                drawLine(s.sx, s.sy, laneX, laneY, 3.5f,
                         1.0f, 0.92f, 0.72f, 0.36f + p * 0.34f);
                drawSword(s.sx, s.sy, SWORD_LEN * (0.88f + p * 0.10f), ang,
                          0.75f, 0.90f, 1.0f, 0.70f + p * 0.22f);
            } else {
                float fade = clampf(s.life / 1.95f, 0.0f, 1.0f);
                drawLine(s.x - s.dirX * 120.0f, s.y - s.dirY * 120.0f,
                         s.x, s.y, 19.0f, 0.30f, 0.58f, 1.0f, 0.18f * fade);
                drawLine(s.x - s.dirX * 76.0f, s.y - s.dirY * 76.0f,
                         s.x, s.y, 5.0f, 0.95f, 0.98f, 1.0f, 0.52f * fade);
                drawSword(s.x, s.y, SWORD_LEN * 0.94f, ang,
                          0.58f, 0.82f, 1.0f, 0.86f * fade);
            }
        }
    }

    void renderBackgroundCopy(float pulse) const {
        if (!bgActive || bgFadeAlpha <= 0.01f) return;
        float a = bgFadeAlpha * 0.56f;
        if (bgMode == 0) {
            drawCircle(worldX, worldY, arenaRadius * 0.58f,
                       0.42f, 0.72f, 1.0f, a * 0.06f);
            drawCircle(worldX, worldY, arenaRadius * 0.72f,
                       0.42f, 0.72f, 1.0f, a * 0.04f);
            drawSword(worldX, worldY, 138.0f, bgOrbAng + PI * 0.5f,
                      0.55f, 0.80f, 1.0f, a * 0.70f);
        } else {
            drawLine(bgX, bgY, lastPX, lastPY, 2.5f,
                     0.62f, 0.84f, 1.0f, a * 0.50f);
            drawCircle(bgX, bgY, 40.0f + pulse * 8.0f, 0.20f, 0.48f, 0.95f, a * 0.26f);
            drawCircle(bgX, bgY, 22.0f + pulse * 3.0f, 0.48f, 0.76f, 1.0f, a * 0.34f);
            drawOctagon(bgX, bgY, 18.0f, 0.55f, 0.80f, 1.0f, a);
            drawSword(bgX, bgY, 150.0f, atan2f(lastPY - bgY, lastPX - bgX),
                      0.45f, 0.70f, 1.0f, a * 0.82f);
        }
    }

    void renderArcSlashes() const {
        for (const auto& s : arcSlashes) {
            if (!s.active) continue;
            float warnP = clampf(s.t / std::max(0.001f, s.warn), 0.0f, 1.0f);
            bool striking = s.t >= s.warn && s.t <= s.warn + s.activeDur;
            float fade = 1.0f;
            if (s.t > s.warn + s.activeDur) {
                fade = 1.0f - clampf((s.t - s.warn - s.activeDur) / 0.32f, 0.0f, 1.0f);
            }
            if (striking) {
                drawArcLine(s.cx, s.cy, s.radius, s.angle, s.halfArc,
                            s.thickness, 0.70f, 0.90f, 1.0f, 0.50f * fade);
                drawArcLine(s.cx, s.cy, s.radius, s.angle, s.halfArc,
                            4.0f, 1.0f, 1.0f, 1.0f, 0.72f * fade);
            } else {
                float tickA = 0.18f + warnP * 0.34f;
                drawArcLine(s.cx, s.cy, s.radius, s.angle, s.halfArc,
                            4.0f + warnP * 7.0f,
                            0.95f, 0.45f, 0.18f, tickA * fade);
                drawArcLine(s.cx, s.cy, s.radius, s.angle, s.halfArc,
                            1.7f, 1.0f, 0.95f, 0.72f, 0.28f * fade);
            }
        }
    }

    static void drawCrescentShape(float cx, float cy, float radius,
                                  float dirX, float dirY,
                                  float r, float g, float b, float a) {
        float spinAng = atan2f(dirY, dirX);
        float outerR = radius;
        float innerR = radius * 0.46f;
        float sweep = 2.35f;
        const int segs = 14;
        for (int i = 0; i < segs; i++) {
            float a0 = spinAng - sweep * 0.5f + sweep * (float)i / (float)segs;
            float a1 = spinAng - sweep * 0.5f + sweep * (float)(i + 1) / (float)segs;
            BatchTri(cx + cosf(a0) * innerR, cy + sinf(a0) * innerR,
                     cx + cosf(a0) * outerR, cy + sinf(a0) * outerR,
                     cx + cosf(a1) * outerR, cy + sinf(a1) * outerR,
                     r, g, b, a);
            BatchTri(cx + cosf(a0) * innerR, cy + sinf(a0) * innerR,
                     cx + cosf(a1) * outerR, cy + sinf(a1) * outerR,
                     cx + cosf(a1) * innerR, cy + sinf(a1) * innerR,
                     r, g, b, a);
        }
        for (int i = 0; i < segs; i++) {
            float a0 = spinAng - sweep * 0.5f + sweep * (float)i / (float)segs;
            float a1 = spinAng - sweep * 0.5f + sweep * (float)(i + 1) / (float)segs;
            BatchTri(cx + cosf(a0) * outerR, cy + sinf(a0) * outerR,
                     cx + cosf(a0) * (outerR + 3.5f), cy + sinf(a0) * (outerR + 3.5f),
                     cx + cosf(a1) * (outerR + 3.5f), cy + sinf(a1) * (outerR + 3.5f),
                     1.0f, 1.0f, 1.0f, a * 0.58f);
            BatchTri(cx + cosf(a0) * outerR, cy + sinf(a0) * outerR,
                     cx + cosf(a1) * (outerR + 3.5f), cy + sinf(a1) * (outerR + 3.5f),
                     cx + cosf(a1) * outerR, cy + sinf(a1) * outerR,
                     1.0f, 1.0f, 1.0f, a * 0.58f);
        }
    }

    void renderCrescentSlashes() const {
        for (const auto& s : crescentSlashes) {
            if (!s.active) continue;
            float lifeP = clampf(s.life / std::max(0.001f, s.maxLife), 0.0f, 1.0f);
            drawLine(s.x - s.dirX * 72.0f, s.y - s.dirY * 72.0f,
                     s.x, s.y, 14.0f, 0.32f, 0.58f, 1.0f, 0.16f * lifeP);
            drawCrescentShape(s.x, s.y, s.radius, s.dirX, s.dirY,
                              0.55f, 0.82f, 1.0f, 0.88f * lifeP);
        }
    }

    void renderDashTrail(float pulse) const {
        if (swordState != ESState::DASH && dashTrailT <= 0.0f && dashShockT <= 0.0f)
            return;

        float ex = (swordState == ESState::DASH) ? worldX : dashEndX;
        float ey = (swordState == ESState::DASH) ? worldY : dashEndY;
        float fade = (swordState == ESState::DASH) ? 1.0f : clampf(dashTrailT / 0.30f, 0.0f, 1.0f);
        if (fade > 0.01f) {
            float dx = ex - dashStartX, dy = ey - dashStartY;
            float len = sqrtf(dx * dx + dy * dy);
            if (len > 2.0f) {
                float nx = -dy / len, ny = dx / len;
                drawLine(dashStartX, dashStartY, ex, ey, 48.0f,
                         0.18f, 0.44f, 0.95f, 0.10f * fade);
                drawLine(dashStartX, dashStartY, ex, ey, 20.0f,
                         0.46f, 0.74f, 1.0f, 0.28f * fade);
                drawLine(dashStartX, dashStartY, ex, ey, 4.0f,
                         1.0f, 1.0f, 1.0f, 0.54f * fade);
                for (int i = 1; i <= 3; ++i) {
                    float off = (float)i * 9.0f;
                    float a = (0.24f - (float)i * 0.05f) * fade;
                    drawLine(dashStartX + nx * off, dashStartY + ny * off,
                             ex + nx * off, ey + ny * off,
                             2.0f, 0.60f, 0.86f, 1.0f, a);
                    drawLine(dashStartX - nx * off, dashStartY - ny * off,
                             ex - nx * off, ey - ny * off,
                             2.0f, 0.60f, 0.86f, 1.0f, a);
                }
            }
        }

        if (dashShockT > 0.0f) {
            float p = 1.0f - clampf(dashShockT / 0.18f, 0.0f, 1.0f);
            drawCircle(dashEndX, dashEndY, 30.0f + p * 62.0f,
                       0.55f, 0.80f, 1.0f, (1.0f - p) * 0.18f);
            drawLine(dashEndX - cosf(swordAngle) * (46.0f + p * 34.0f),
                     dashEndY - sinf(swordAngle) * (46.0f + p * 34.0f),
                     dashEndX + cosf(swordAngle) * (46.0f + p * 34.0f),
                     dashEndY + sinf(swordAngle) * (46.0f + p * 34.0f),
                     7.0f, 0.80f, 0.95f, 1.0f, (1.0f - p) * 0.28f);
        }
    }

    void renderShadow(float pulse, float gt) const {
        if (!enrage) return;
        drawCircle(shadowX, shadowY, 26.0f + pulse * 3.0f,
                   0.12f, 0.28f, 0.70f, 0.20f);
        drawOctagon(shadowX, shadowY, 19.0f, 0.38f, 0.62f, 1.0f, 0.42f);
        drawSword(shadowX, shadowY, 105.0f, -gt * 1.3f,
                  0.28f, 0.48f, 0.88f, 0.42f);
    }

    void renderCore(float pulse, float gt) const {
        float phasePulse = phase3 ? 1.08f : phase2 ? 1.02f : 1.0f;
        float coreR = (28.0f + pulse * 4.0f) * phasePulse;
        drawCircle(worldX, worldY, coreR * 1.55f, 0.30f, 0.55f, 0.90f, 0.06f + pulse * 0.04f);
        drawOctagon(worldX, worldY, coreR, 0.04f, 0.06f, 0.12f, 0.95f);
        drawNeonBorder(worldX - coreR, worldY - coreR, coreR * 2.0f, coreR * 2.0f,
                       enrage ? 1.0f : 0.55f, enrage ? 0.25f : 0.80f, enrage ? 0.25f : 1.0f);
        drawOctagon(worldX, worldY, coreR * 0.60f, 0.08f, 0.12f, 0.22f, 0.78f);
        int fins = enrage ? 8 : phase3 ? 6 : 4;
        float spin = enrage ? 3.2f : phase3 ? 2.1f : 0.8f;
        for (int fi = 0; fi < fins; fi++) {
            float fAng = gt * spin + fi * TAU / (float)fins;
            float fx = worldX + cosf(fAng) * coreR * 1.22f;
            float fy = worldY + sinf(fAng) * coreR * 1.22f;
            drawRotRect(fx, fy, 24.0f, 6.5f, fAng,
                        0.38f, 0.58f, 0.88f, 0.48f);
            drawRotRect(fx, fy, 16.0f, 3.5f, fAng,
                        0.65f, 0.82f, 1.0f, 0.66f);
        }
        drawCircle(worldX, worldY, coreR * 0.23f, 0.55f, 0.80f, 1.0f, 0.52f + pulse * 0.28f);
        drawCircle(worldX, worldY, coreR * 0.08f, 1.0f, 1.0f, 1.0f, 0.78f + pulse * 0.18f);
    }

    void renderBasicWarning(float gt) const {
        if (swordState != ESState::BASIC_ATTACK) return;
        float warnP = clampf(basic.t / std::max(0.001f, basic.warn), 0.0f, 1.0f);
        float a = 0.24f + warnP * 0.34f;
        if (basic.type == ESBasicType::EDGE_SLASH) {
            float len = sqrtf((float)screenW * screenW + (float)screenH * screenH);
            int sideLanes = (!phase2) ? 0 : 1;
            for (int i = -sideLanes; i <= sideLanes; ++i) {
                float ang = basic.angle + (float)i * 0.11f;
                float ex = worldX + cosf(ang) * len;
                float ey = worldY + sinf(ang) * len;
                drawLine(worldX, worldY, ex, ey, basic.fired ? 6.0f : 3.5f,
                         0.70f, 0.90f, 1.0f, basic.fired ? 0.24f : a);
            }
            if (!basic.hitA) {
                float aim = atan2f(lastPY - worldY, lastPX - worldX);
                float perX = -sinf(aim), perY = cosf(aim);
                float offset = phase3 ? 64.0f : phase2 ? 56.0f : 46.0f;
                for (int side = -1; side <= 1; side += 2) {
                    float ox = worldX + perX * offset * (float)side;
                    float oy = worldY + perY * offset * (float)side;
                    drawLine(ox, oy,
                             lastPX,
                             lastPY,
                             2.4f, 0.45f, 0.72f, 1.0f, a * 0.50f);
                }
                if (phase2) {
                    drawLine(worldX, worldY,
                             worldX + cosf(aim) * len,
                             worldY + sinf(aim) * len,
                             1.7f, 0.35f, 0.60f, 1.0f, a * 0.24f);
                }
            }
        } else if (basic.type == ESBasicType::ORBIT_CUT) {
            float thick = basic.t >= basic.warn ? 22.0f : 9.0f + warnP * 10.0f;
            drawArcLine(basic.x, basic.y, basic.radius, basic.angle, orbitHalfArc(), thick,
                        1.0f, 0.58f, 0.20f, basic.t >= basic.warn ? 0.58f : a);
            drawArcLine(basic.x, basic.y, basic.radius, basic.angle, orbitHalfArc(), 2.4f,
                        1.0f, 1.0f, 1.0f, 0.45f);
        } else {
            float lineLen = sqrtf((float)screenW * screenW + (float)screenH * screenH);
            float ca = cosf(basic.angle), sa = sinf(basic.angle);
            float ca2 = -sa, sa2 = ca;
            float a1 = (basic.t >= basic.warn) ? 0.56f : a;
            float a2 = (basic.t >= basic.warn + 0.18f) ? 0.46f : a * 0.65f;
            drawLine(basic.x - ca * lineLen, basic.y - sa * lineLen,
                     basic.x + ca * lineLen, basic.y + sa * lineLen,
                     10.0f, 1.0f, 0.55f, 0.18f, a1);
            drawLine(basic.x - ca2 * lineLen, basic.y - sa2 * lineLen,
                     basic.x + ca2 * lineLen, basic.y + sa2 * lineLen,
                     8.0f, 0.70f, 0.90f, 1.0f, a2);
        }
    }

    void renderSwordState(float pulse) const {
        if (arenaForming) {
            float p = clampf(arenaFormT / ARENA_FORM_DUR, 0.0f, 1.0f);
            float castAng = -PI * 0.5f + sinf(barrierRot * 1.4f) * 0.08f;
            for (int i = 3; i >= 0; --i) {
                float trail = (float)i * 0.16f;
                drawSword(worldX, worldY, SWORD_LEN * (0.94f + p * 0.12f),
                          castAng + trail,
                          0.46f, 0.74f, 1.0f, (0.18f + p * 0.22f) * (1.0f - i * 0.18f));
            }
            drawLine(worldX, worldY,
                     worldX + cosf(castAng) * (SWORD_LEN * 0.88f),
                     worldY + sinf(castAng) * (SWORD_LEN * 0.88f),
                     7.0f, 1.0f, 1.0f, 1.0f, 0.46f + pulse * 0.18f);
            return;
        }

        if (swordState == ESState::CHARGE) {
            float total = std::max(0.001f, chargeTotal);
            float cp = clampf(1.0f - chargeTimer / total, 0.0f, 1.0f);
            float spinAng = swordAngle + chargeSpinDir * cp * TAU * 3.0f;
            float spinA = 0.22f + cp * 0.48f;

            drawLine(worldX - cosf(swordAngle) * 120.0f,
                     worldY - sinf(swordAngle) * 120.0f,
                     dashPreviewX,
                     dashPreviewY,
                     14.0f + cp * 10.0f, 0.22f, 0.50f, 0.95f, 0.07f + cp * 0.10f);
            drawLine(worldX, worldY,
                     dashPreviewX,
                     dashPreviewY,
                     3.0f + cp * 2.5f, 0.82f, 0.94f, 1.0f, 0.38f + cp * 0.24f);
            for (int i = 0; i < 3; ++i) {
                float rp = clampf(cp - (float)i * 0.16f, 0.0f, 1.0f);
                if (rp <= 0.0f) continue;
                drawCircle(worldX, worldY, 28.0f + rp * (42.0f + i * 22.0f),
                           0.70f, 0.88f, 1.0f, (0.08f + rp * 0.15f) * (1.0f - i * 0.18f));
            }
            for (int i = 0; i < 4; ++i) {
                float trailAng = spinAng - chargeSpinDir * (float)i * 0.34f;
                float trailAlpha = spinA * (1.0f - (float)i * 0.20f);
                drawSword(worldX, worldY, 150.0f, trailAng,
                          0.52f, 0.78f, 1.0f, trailAlpha);
            }
            for (int i = 0; i < 8; ++i) {
                float a = spinAng + (float)i * TAU / 8.0f;
                float rr = 42.0f + cp * 28.0f;
                drawLine(worldX + cosf(a) * rr, worldY + sinf(a) * rr,
                         worldX + cosf(a + chargeSpinDir * 0.42f) * (rr + 18.0f),
                         worldY + sinf(a + chargeSpinDir * 0.42f) * (rr + 18.0f),
                         2.5f, 0.60f, 0.86f, 1.0f, 0.14f + cp * 0.18f);
            }
            return;
        }

        if (swordState == ESState::DASH) {
            drawRotRect(worldX, worldY, swordLen * 1.08f, 46.0f, swordAngle,
                        0.18f, 0.40f, 0.85f, 0.08f);
            drawRotRect(worldX, worldY, swordLen, 22.0f, swordAngle,
                        0.35f, 0.62f, 1.0f, 0.22f);
            drawRotRect(worldX, worldY, swordLen, 10.0f, swordAngle,
                        0.55f, 0.80f, 1.0f, 0.82f);
            drawRotRect(worldX, worldY, swordLen, 2.5f, swordAngle,
                        1.0f, 1.0f, 1.0f, 0.88f);
            return;
        }

        float sr = enrage ? 1.0f : swordState == ESState::PUNISH ? 0.42f : 0.55f;
        float sg = enrage ? 0.36f : swordState == ESState::PUNISH ? 0.66f : 0.80f;
        float sb = 1.0f;
        float sa = swordState == ESState::PUNISH ? 0.58f : 0.90f;
        if (swordState == ESState::PUNISH) {
            drawCircle(worldX, worldY, 42.0f + pulse * 4.0f, 0.32f, 0.58f, 1.0f, 0.08f);
        }
        drawSword(worldX, worldY, swordLen, swordAngle, sr, sg, sb, sa);
    }
};
