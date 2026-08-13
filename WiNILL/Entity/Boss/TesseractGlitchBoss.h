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

class TesseractGlitchBoss {
public:
    static constexpr const wchar_t* BOSS_NAME = L"TESS.glitch";

    float worldX, worldY;
    float hp, maxHp;
    bool  alive = true;
    bool  exploded = false;
    int   screenW, screenH;

    bool phase2 = false;
    bool phase3 = false;

    float coreAng = 0.0f;
    float glitchT = 0.0f;
    float skillCd = 0.55f;
    float faceFlash = 0.0f;
    int   patternStep = 0;

    // Compatibility only. TESS.glitch currently has no projectile bars/add orbs.
    struct Wall {
        float cx, cy, len, thick, ang;
        float vx, vy, spin;
        float life, maxLife;
        float warm, maxWarm;
        bool active, launched, heavy, hitPlayer, fake;
    };
    std::vector<Wall> walls;

    struct Orb {
        float x, y;
        float vx, vy;
        float hp;
        float fireT;
        float life;
        bool  bomber;
        bool  alive;
    };
    std::vector<Orb> orbs;

    struct Spark {
        float x, y, vx, vy, life;
    };
    std::vector<Spark> sparks;

    enum class SkillType { CorePulse, QuadrantCrash, GravityWell, MirrorFault, DimLock };
    struct Skill {
        SkillType type = SkillType::CorePulse;
        float x = 0.0f, y = 0.0f;
        float w = 0.0f, h = 0.0f;
        float ang = 0.0f;
        float t = 0.0f;
        float warn = 0.6f;
        float dur = 0.16f;
        float dmg = 10.0f;
        int   mode = 0;
        bool  hit = false;
    };
    std::vector<Skill> skills;

    static constexpr float CORE_R = 58.0f;
    static constexpr float ORB_R = 18.0f;
    static constexpr float WALL_THICK = 0.0f;
    static constexpr float WALL_SPEED = 0.0f;
    static constexpr float WALL_DAMAGE = 0.0f;
    static constexpr float ORB_HP = 1.0f;
    static constexpr float ORB_DMG = 0.0f;
    static constexpr float PI = 3.1415926535f;

    TesseractGlitchBoss(int sw, int sh, float hpInit)
        : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.5f;
        worldY = sh * 0.32f;
        coreAng = (float)(rand() % 628) * 0.01f;
    }

    static float clampf(float v, float lo, float hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    }

    static float randf(float lo, float hi) {
        float t = (float)(rand() % 10000) / 10000.0f;
        return lo + (hi - lo) * t;
    }

    static float dist(float ax, float ay, float bx, float by) {
        float dx = ax - bx, dy = ay - by;
        return std::sqrt(dx * dx + dy * dy);
    }

    static bool pointRect(float px, float py, float cx, float cy, float w, float h) {
        return std::fabs(px - cx) <= w * 0.5f && std::fabs(py - cy) <= h * 0.5f;
    }

    bool hitWall(float x0, float y0, float x1, float y1) const {
        (void)x0; (void)y0; (void)x1; (void)y1;
        return false;
    }

    void blockMove(float& pcx, float& pcy, float pr) const {
        (void)pcx; (void)pcy; (void)pr;
    }

    void pushSpark(float x, float y, float r, float g, float b) {
        (void)r; (void)g; (void)b;
        for (int i = 0; i < 7; i++) {
            float a = randf(0.0f, PI * 2.0f);
            float sp = randf(90.0f, 280.0f);
            sparks.push_back({ x, y, std::cos(a) * sp, std::sin(a) * sp, randf(0.18f, 0.34f) });
        }
        if ((int)sparks.size() > 180)
            sparks.erase(sparks.begin(), sparks.begin() + ((int)sparks.size() - 180));
    }

    float warnMul(Difficulty difficulty) const {
        if (difficulty == Difficulty::EASY) return 1.16f;
        if (difficulty == Difficulty::HARD) return 0.86f;
        return 1.0f;
    }

    float tempoMul(Difficulty difficulty) const {
        if (difficulty == Difficulty::EASY) return 1.16f;
        if (difficulty == Difficulty::HARD) return 0.82f;
        return 1.0f;
    }

    float damageMul(Difficulty difficulty) const {
        if (difficulty == Difficulty::EASY) return 0.82f;
        if (difficulty == Difficulty::HARD) return 1.12f;
        return 1.0f;
    }

    void addSkill(const Skill& s) {
        skills.push_back(s);
        faceFlash = 0.20f;
        if ((int)skills.size() > 22)
            skills.erase(skills.begin(), skills.begin() + ((int)skills.size() - 22));
    }

    void spawnCorePulse(float px, float py, Difficulty difficulty, float delay = 0.0f, int mode = -1) {
        Skill s{};
        s.type = SkillType::CorePulse;
        s.mode = (mode >= 0) ? mode : rand() % 2; // 0 = ring, 1 = disk
        if (s.mode == 0) {
            s.x = worldX;
            s.y = worldY;
            s.w = phase3 ? randf(155.0f, 215.0f) : phase2 ? randf(145.0f, 205.0f) : randf(135.0f, 185.0f);
            s.h = difficulty == Difficulty::EASY ? 30.0f : difficulty == Difficulty::HARD ? 45.0f : 38.0f;
        } else {
            s.x = clampf(px + randf(-90.0f, 90.0f), 80.0f, (float)screenW - 80.0f);
            s.y = clampf(py + randf(-75.0f, 75.0f), 90.0f, (float)screenH - 90.0f);
            s.w = phase3 ? 116.0f : phase2 ? 106.0f : 96.0f;
            s.h = s.w;
        }
        s.warn = (phase3 ? 0.46f : phase2 ? 0.56f : 0.68f) * warnMul(difficulty);
        s.dur = phase3 ? 0.14f : 0.17f;
        s.dmg = (phase3 ? 15.0f : phase2 ? 12.0f : 10.0f) * damageMul(difficulty);
        s.t = -delay;
        addSkill(s);
    }

    void spawnQuadrantCrash(float px, float py, Difficulty difficulty, float delay = 0.0f) {
        Skill s{};
        s.type = SkillType::QuadrantCrash;
        bool right = px >= (float)screenW * 0.5f;
        bool bottom = py >= (float)screenH * 0.5f;
        int playerQ = (right ? 1 : 0) + (bottom ? 2 : 0);
        int offset = 1 + (rand() % 3);
        s.mode = (playerQ + offset) & 3;
        if (phase3 && rand() % 2 == 0) s.mode = playerQ;
        s.warn = (phase3 ? 0.48f : phase2 ? 0.60f : 0.76f) * warnMul(difficulty);
        s.dur = 0.18f;
        s.dmg = (phase3 ? 16.0f : phase2 ? 13.0f : 10.0f) * damageMul(difficulty);
        s.t = -delay;
        addSkill(s);
    }

    void spawnGravityWell(float px, float py, Difficulty difficulty, float delay = 0.0f) {
        Skill s{};
        s.type = SkillType::GravityWell;
        s.x = clampf(px + randf(-145.0f, 145.0f), 90.0f, (float)screenW - 90.0f);
        s.y = clampf(py + randf(-115.0f, 115.0f), 100.0f, (float)screenH - 100.0f);
        s.w = phase3 ? randf(98.0f, 132.0f) : phase2 ? randf(92.0f, 122.0f) : randf(84.0f, 108.0f);
        if (difficulty == Difficulty::EASY) s.w *= 0.9f;
        if (difficulty == Difficulty::HARD) s.w *= 1.08f;
        s.warn = (phase3 ? 0.34f : 0.46f) * warnMul(difficulty);
        s.dur = phase3 ? 1.08f : 0.86f;
        s.dmg = (phase3 ? 13.0f : 9.5f) * damageMul(difficulty);
        s.t = -delay;
        addSkill(s);
    }

    void spawnMirrorFault(float px, float py, Difficulty difficulty, float delay = 0.0f) {
        Skill s{};
        s.type = SkillType::MirrorFault;
        float ox = randf(-150.0f, 150.0f);
        float oy = randf(-110.0f, 110.0f);
        s.x = clampf(px + ox, 72.0f, (float)screenW - 72.0f);
        s.y = clampf(py + oy, 84.0f, (float)screenH - 84.0f);
        s.w = clampf((float)screenW - s.x + randf(-45.0f, 45.0f), 72.0f, (float)screenW - 72.0f);
        s.h = clampf((float)screenH - s.y + randf(-45.0f, 45.0f), 84.0f, (float)screenH - 84.0f);
        s.ang = difficulty == Difficulty::EASY ? 54.0f : difficulty == Difficulty::HARD ? 68.0f : 60.0f;
        if (phase3) s.ang += 8.0f;
        s.warn = (phase3 ? 0.42f : 0.58f) * warnMul(difficulty);
        s.dur = 0.16f;
        s.dmg = (phase3 ? 15.0f : 12.0f) * damageMul(difficulty);
        s.t = -delay;
        addSkill(s);
    }

    void spawnDimLock(float px, float py, Difficulty difficulty, float delay = 0.0f) {
        Skill s{};
        s.type = SkillType::DimLock;
        s.mode = rand() % 2;
        s.x = clampf(px, 160.0f, (float)screenW - 160.0f);
        s.y = clampf(py, 140.0f, (float)screenH - 140.0f);
        float corridor = difficulty == Difficulty::EASY ? 270.0f : difficulty == Difficulty::HARD ? 190.0f : 225.0f;
        s.w = s.h = corridor;
        s.warn = 0.58f * warnMul(difficulty);
        s.dur = difficulty == Difficulty::EASY ? 1.0f : 1.22f;
        s.dmg = 10.0f * damageMul(difficulty);
        s.t = -delay;
        addSkill(s);
    }

    void spawnPattern(float px, float py, Difficulty difficulty) {
        ++patternStep;
        if (!phase2) {
            int p = patternStep % 3;
            if (p == 0) spawnCorePulse(px, py, difficulty, 0.0f, 0);
            else if (p == 1) spawnQuadrantCrash(px, py, difficulty);
            else spawnGravityWell(px, py, difficulty);
            skillCd = randf(1.08f, 1.36f) * tempoMul(difficulty);
            return;
        }

        if (!phase3) {
            int p = patternStep % 4;
            if (p == 0) {
                spawnMirrorFault(px, py, difficulty);
            } else if (p == 1) {
                spawnCorePulse(px, py, difficulty, 0.0f, 1);
                spawnQuadrantCrash(px, py, difficulty, 0.18f);
            } else if (p == 2) {
                spawnGravityWell(px, py, difficulty);
                spawnCorePulse(px, py, difficulty, 0.26f, 0);
            } else {
                spawnMirrorFault(px, py, difficulty);
                spawnGravityWell(px, py, difficulty, 0.22f);
            }
            skillCd = randf(0.72f, 0.96f) * tempoMul(difficulty);
            return;
        }

        int p = patternStep % 5;
        if (p == 0) {
            spawnDimLock(px, py, difficulty);
            spawnCorePulse(px, py, difficulty, 0.34f, 0);
            spawnMirrorFault(px, py, difficulty, 0.58f);
        } else if (p == 1) {
            spawnGravityWell(px, py, difficulty);
            spawnGravityWell(px, py, difficulty, 0.18f);
            spawnQuadrantCrash(px, py, difficulty, 0.42f);
        } else if (p == 2) {
            spawnMirrorFault(px, py, difficulty);
            spawnMirrorFault(px, py, difficulty, 0.22f);
            spawnCorePulse(px, py, difficulty, 0.42f, 1);
        } else if (p == 3) {
            spawnDimLock(px, py, difficulty);
            spawnQuadrantCrash(px, py, difficulty, 0.30f);
            spawnGravityWell(px, py, difficulty, 0.55f);
        } else {
            spawnCorePulse(px, py, difficulty, 0.0f, 0);
            spawnCorePulse(px, py, difficulty, 0.22f, 1);
            spawnMirrorFault(px, py, difficulty, 0.44f);
        }
        skillCd = randf(0.46f, 0.68f) * tempoMul(difficulty);
    }

    bool skillHitsPlayer(const Skill& s, float px, float py) const {
        if (s.type == SkillType::CorePulse) {
            float d = dist(px, py, s.x, s.y);
            if (s.mode == 0) return std::fabs(d - s.w) <= s.h;
            return d <= s.w;
        }
        if (s.type == SkillType::QuadrantCrash) {
            bool right = px >= (float)screenW * 0.5f;
            bool bottom = py >= (float)screenH * 0.5f;
            int q = (right ? 1 : 0) + (bottom ? 2 : 0);
            return q == s.mode;
        }
        if (s.type == SkillType::GravityWell) {
            return dist(px, py, s.x, s.y) <= s.w;
        }
        if (s.type == SkillType::MirrorFault) {
            return dist(px, py, s.x, s.y) <= s.ang ||
                   dist(px, py, s.w, s.h) <= s.ang;
        }
        if (s.type == SkillType::DimLock) {
            if (s.mode == 0) return std::fabs(px - s.x) > s.w * 0.5f;
            return std::fabs(py - s.y) > s.h * 0.5f;
        }
        return false;
    }

    void Update(float px, float py, float dt, float& playerHP,
                std::vector<Bullet>& bullets, Difficulty difficulty,
                bool dashInvuln) {
        (void)bullets;
        if (!alive) return;

        if (!phase2 && hp <= maxHp * 0.70f) {
            phase2 = true;
            skillCd = 0.20f;
            pushSpark(worldX, worldY, 0.86f, 0.36f, 1.0f);
        }
        if (!phase3 && hp <= maxHp * 0.36f) {
            phase3 = true;
            skillCd = 0.10f;
            pushSpark(worldX, worldY, 1.0f, 0.18f, 0.32f);
        }

        glitchT += dt;
        coreAng += dt * (phase3 ? 3.6f : (phase2 ? 2.35f : 1.25f));
        if (faceFlash > 0.0f) faceFlash -= dt;

        float hoverY = phase3 ? (float)screenH * 0.26f : (float)screenH * 0.32f;
        float targetX = (float)screenW * 0.5f + std::sin(glitchT * 1.4f) * (phase3 ? 86.0f : 118.0f);
        targetX += (px - (float)screenW * 0.5f) * (phase3 ? 0.16f : 0.10f);
        float targetY = hoverY + std::cos(glitchT * 1.05f) * (phase3 ? 22.0f : 34.0f);
        float follow = phase3 ? 1.2f : (phase2 ? 0.96f : 0.68f);
        worldX += (targetX - worldX) * follow * dt;
        worldY += (targetY - worldY) * follow * dt;
        worldX = clampf(worldX, 140.0f, (float)screenW - 140.0f);
        worldY = clampf(worldY, 100.0f, (float)screenH - 220.0f);

        skillCd -= dt;
        if (skillCd <= 0.0f)
            spawnPattern(px, py, difficulty);

        for (auto& s : skills) {
            s.t += dt;
            if (s.t < 0.0f) continue;

            bool active = s.t >= s.warn && s.t <= s.warn + s.dur;
            if (!active || dashInvuln) continue;

            if (s.type == SkillType::GravityWell || s.type == SkillType::DimLock) {
                if (skillHitsPlayer(s, px, py))
                    HurtPlayer(playerHP, s.dmg * dt);
            } else if (!s.hit && skillHitsPlayer(s, px, py)) {
                HurtPlayer(playerHP, s.dmg);
                s.hit = true;
                pushSpark(px, py, 1.0f, 0.22f, 0.38f);
            }
        }
        skills.erase(std::remove_if(skills.begin(), skills.end(),
            [](const Skill& s) { return s.t > s.warn + s.dur + 0.18f; }), skills.end());

        for (auto& s : sparks) {
            s.life -= dt;
            s.x += s.vx * dt;
            s.y += s.vy * dt;
            s.vx *= 0.88f;
            s.vy *= 0.88f;
        }
        sparks.erase(std::remove_if(sparks.begin(), sparks.end(),
            [](const Spark& s) { return s.life <= 0.0f; }), sparks.end());

        float cdx = px - worldX, cdy = py - worldY;
        if (!dashInvuln && cdx * cdx + cdy * cdy < CORE_R * CORE_R)
            HurtPlayer(playerHP, 10.0f * dt);
    }

    static void drawRotRect(float cx, float cy, float w, float h, float ang,
                            float r, float g, float b, float a) {
        float hx = w * 0.5f, hy = h * 0.5f;
        float c = std::cos(ang), s = std::sin(ang);
        float x0 = -hx, y0 = -hy;
        float x1 =  hx, y1 = -hy;
        float x2 =  hx, y2 =  hy;
        float x3 = -hx, y3 =  hy;
        auto tx = [&](float x, float y) { return cx + x * c - y * s; };
        auto ty = [&](float x, float y) { return cy + x * s + y * c; };
        BatchTri(tx(x0,y0), ty(x0,y0), tx(x1,y1), ty(x1,y1), tx(x2,y2), ty(x2,y2), r,g,b,a);
        BatchTri(tx(x0,y0), ty(x0,y0), tx(x2,y2), ty(x2,y2), tx(x3,y3), ty(x3,y3), r,g,b,a);
    }

    static void drawLine(float x0, float y0, float x1, float y1, float thick,
                         float r, float g, float b, float a) {
        float dx = x1 - x0, dy = y1 - y0;
        float l = std::sqrt(dx * dx + dy * dy);
        if (l < 0.5f) return;
        drawRotRect((x0 + x1) * 0.5f, (y0 + y1) * 0.5f, l, thick,
                    std::atan2(dy, dx), r, g, b, a);
    }

    static void drawArcRing(float cx, float cy, float rad,
                            float r, float g, float b, float a, int n = 32) {
        for (int i = 0; i < n; i++) {
            if ((i & 1) == 0) continue;
            float a0 = (float)i / (float)n * PI * 2.0f;
            float a1 = (float)(i + 1) / (float)n * PI * 2.0f;
            float x0 = cx + std::cos(a0) * rad, y0 = cy + std::sin(a0) * rad;
            float x1 = cx + std::cos(a1) * rad, y1 = cy + std::sin(a1) * rad;
            drawLine(x0, y0, x1, y1, 3.0f, r, g, b, a);
        }
    }

    static float skillProg(const Skill& s) {
        if (s.t < 0.0f) return 0.0f;
        return clampf(s.t / std::max(0.01f, s.warn), 0.0f, 1.0f);
    }

    static bool skillActive(const Skill& s) {
        return s.t >= s.warn && s.t <= s.warn + s.dur;
    }

    void drawCorePulseSkill(const Skill& s, float t) const {
        float p = skillProg(s);
        bool a = skillActive(s);
        float cr = phase3 ? 1.0f : phase2 ? 0.84f : 0.40f;
        float cg = phase3 ? 0.18f : phase2 ? 0.36f : 0.92f;
        float cb = phase3 ? 0.34f : 1.0f;
        float pulse = 0.5f + 0.5f * std::sin(t * 20.0f + s.x * 0.01f);
        if (s.mode == 0) {
            drawArcRing(s.x, s.y, s.w, cr, cg, cb, a ? 0.74f : 0.25f + p * 0.24f, 42);
            drawArcRing(s.x, s.y, s.w - s.h, 1.0f, 0.9f, 1.0f, 0.12f + p * 0.20f, 34);
            drawArcRing(s.x, s.y, s.w + s.h, 1.0f, 0.9f, 1.0f, 0.12f + p * 0.20f, 34);
        } else {
            drawCircle(s.x, s.y, s.w * (a ? 0.96f + pulse * 0.05f : 0.72f + p * 0.28f),
                       cr, cg, cb, a ? 0.62f : 0.12f + p * 0.30f);
            drawArcRing(s.x, s.y, s.w, 1.0f, 0.9f, 1.0f, 0.20f + p * 0.28f, 34);
        }
    }

    void drawQuadrantSkill(const Skill& s, float t) const {
        float p = skillProg(s);
        bool a = skillActive(s);
        float x = (s.mode & 1) ? (float)screenW * 0.5f : 0.0f;
        float y = (s.mode & 2) ? (float)screenH * 0.5f : 0.0f;
        float w = (float)screenW * 0.5f;
        float h = (float)screenH * 0.5f;
        float flick = 0.5f + 0.5f * std::sin(t * 26.0f);
        drawRect(x, y, w, h, phase3 ? 1.0f : 0.78f,
                 phase3 ? 0.18f : 0.36f, phase3 ? 0.34f : 1.0f,
                 a ? 0.56f : 0.10f + p * 0.24f);
        drawRect(x, y, w, 4.0f, 1.0f, 0.9f, 1.0f, 0.35f + flick * 0.25f);
        drawRect(x, y + h - 4.0f, w, 4.0f, 1.0f, 0.9f, 1.0f, 0.35f + flick * 0.25f);
        drawRect(x, y, 4.0f, h, 1.0f, 0.9f, 1.0f, 0.35f + flick * 0.25f);
        drawRect(x + w - 4.0f, y, 4.0f, h, 1.0f, 0.9f, 1.0f, 0.35f + flick * 0.25f);
    }

    void drawGravityWellSkill(const Skill& s, float t) const {
        float p = skillProg(s);
        bool a = skillActive(s);
        float pulse = 0.5f + 0.5f * std::sin(t * 15.0f + s.x * 0.02f);
        drawCircle(s.x, s.y, s.w * (0.82f + p * 0.18f), 0.03f, 0.02f, 0.06f, a ? 0.58f : 0.18f + p * 0.18f);
        drawArcRing(s.x, s.y, s.w * (0.92f + pulse * 0.04f),
                    phase3 ? 1.0f : 0.55f, phase3 ? 0.18f : 0.82f, 1.0f,
                    0.32f + p * 0.28f, 36);
        drawCircle(s.x, s.y, 9.0f + pulse * 7.0f, 1.0f, 0.9f, 1.0f, 0.42f + p * 0.24f);
    }

    void drawMirrorFaultSkill(const Skill& s, float t) const {
        float p = skillProg(s);
        bool a = skillActive(s);
        float cr = phase3 ? 1.0f : 0.84f;
        float cg = phase3 ? 0.18f : 0.36f;
        float cb = phase3 ? 0.34f : 1.0f;
        float alpha = a ? 0.64f : 0.14f + p * 0.24f;
        drawCircle(s.x, s.y, s.ang, cr, cg, cb, alpha);
        drawCircle(s.w, s.h, s.ang, cr, cg, cb, alpha);
        drawLine(s.x, s.y, s.w, s.h, 3.0f, 1.0f, 0.9f, 1.0f, 0.26f + p * 0.28f);
        drawArcRing(s.x, s.y, s.ang + 8.0f, 1.0f, 0.9f, 1.0f, 0.24f + p * 0.20f, 26);
        drawArcRing(s.w, s.h, s.ang + 8.0f, 1.0f, 0.9f, 1.0f, 0.24f + p * 0.20f, 26);
    }

    void drawDimLockSkill(const Skill& s, float t) const {
        float p = skillProg(s);
        bool a = skillActive(s);
        float alpha = a ? 0.32f : 0.08f + p * 0.14f;
        float r = phase3 ? 1.0f : 0.82f;
        float g = phase3 ? 0.16f : 0.36f;
        float b = phase3 ? 0.30f : 1.0f;
        if (s.mode == 0) {
            float leftW = std::max(0.0f, s.x - s.w * 0.5f);
            float rightX = s.x + s.w * 0.5f;
            drawRect(0.0f, 0.0f, leftW, (float)screenH, r, g, b, alpha);
            drawRect(rightX, 0.0f, (float)screenW - rightX, (float)screenH, r, g, b, alpha);
            drawRect(s.x - s.w * 0.5f, 0.0f, 4.0f, (float)screenH, 0.3f, 1.0f, 0.82f, 0.42f + p * 0.28f);
            drawRect(s.x + s.w * 0.5f - 4.0f, 0.0f, 4.0f, (float)screenH, 0.3f, 1.0f, 0.82f, 0.42f + p * 0.28f);
        } else {
            float topH = std::max(0.0f, s.y - s.h * 0.5f);
            float botY = s.y + s.h * 0.5f;
            drawRect(0.0f, 0.0f, (float)screenW, topH, r, g, b, alpha);
            drawRect(0.0f, botY, (float)screenW, (float)screenH - botY, r, g, b, alpha);
            drawRect(0.0f, s.y - s.h * 0.5f, (float)screenW, 4.0f, 0.3f, 1.0f, 0.82f, 0.42f + p * 0.28f);
            drawRect(0.0f, s.y + s.h * 0.5f - 4.0f, (float)screenW, 4.0f, 0.3f, 1.0f, 0.82f, 0.42f + p * 0.28f);
        }
    }

    void renderNoise(float t) const {
        int n = phase3 ? 32 : (phase2 ? 18 : 8);
        float aBase = phase3 ? 0.075f : (phase2 ? 0.045f : 0.025f);
        for (int i = 0; i < n; i++) {
            float s = std::sin(t * (19.0f + i) + (float)i * 11.73f) * 43758.0f;
            float u = s - std::floor(s);
            float v = std::sin(t * (13.0f + i * 0.42f) + (float)i * 7.19f) * 21917.0f;
            v = v - std::floor(v);
            float x = u * (float)screenW;
            float y = v * (float)screenH;
            float w = 14.0f + (float)((i * 17) % 74);
            drawRect(x, y, w, 2.0f, phase3 ? 1.0f : 0.55f,
                     phase3 ? 0.16f : 0.78f, phase3 ? 0.28f : 1.0f, aBase);
        }
    }

    void renderFx(float t, float aimX, float aimY) const {
        (void)aimX; (void)aimY;
        renderNoise(t);
        for (const auto& s : skills) {
            if (s.t < 0.0f) continue;
            if (s.type == SkillType::CorePulse) drawCorePulseSkill(s, t);
            else if (s.type == SkillType::QuadrantCrash) drawQuadrantSkill(s, t);
            else if (s.type == SkillType::GravityWell) drawGravityWellSkill(s, t);
            else if (s.type == SkillType::MirrorFault) drawMirrorFaultSkill(s, t);
            else if (s.type == SkillType::DimLock) drawDimLockSkill(s, t);
        }

        for (const auto& s : sparks) {
            float a = clampf(s.life / 0.34f, 0.0f, 1.0f);
            drawRotRect(s.x, s.y, 5.0f + 7.0f * a, 5.0f + 7.0f * a,
                        t * 5.0f + s.x * 0.01f, 1.0f, phase3 ? 0.22f : 0.52f,
                        phase3 ? 0.38f : 1.0f, a);
        }
    }

    void drawCubeFace(float cx, float cy, float size, float ang,
                      float sx, float sy, float r, float g, float b, float a) const {
        float pts[4][2] = {
            {-size, -size}, { size, -size}, { size, size}, {-size, size}
        };
        float wx[4], wy[4];
        float c = std::cos(ang), s = std::sin(ang);
        for (int i = 0; i < 4; i++) {
            float x = pts[i][0] * sx;
            float y = pts[i][1] * sy;
            float jitter = std::sin(glitchT * 24.0f + (float)i * 2.4f) *
                           (phase3 ? 9.0f : phase2 ? 5.0f : 2.0f);
            wx[i] = cx + x * c - y * s + jitter;
            wy[i] = cy + x * s + y * c - jitter * 0.55f;
        }
        for (int i = 0; i < 4; i++) {
            int j = (i + 1) & 3;
            drawLine(wx[i], wy[i], wx[j], wy[j], 4.0f, r, g, b, a);
        }
    }

    void renderBody(float t) const {
        float pulse = 0.5f + 0.5f * std::sin(t * (phase3 ? 13.0f : 7.5f));
        float cr = phase3 ? 1.0f : (phase2 ? 0.84f : 0.40f);
        float cg = phase3 ? 0.18f : (phase2 ? 0.36f : 0.92f);
        float cb = phase3 ? 0.34f : 1.0f;
        float sx = phase3 ? (0.78f + pulse * 0.52f) : 1.0f + pulse * 0.06f;
        float sy = phase3 ? (1.26f - pulse * 0.34f) : 1.0f - pulse * 0.04f;
        float flash = clampf(faceFlash / 0.20f, 0.0f, 1.0f);

        drawCircle(worldX, worldY, CORE_R * (1.45f + pulse * 0.18f),
                   cr, cg, cb, phase3 ? 0.18f : 0.10f);
        if (flash > 0.0f)
            drawCircle(worldX, worldY, CORE_R * (1.85f + flash * 0.3f),
                       1.0f, 0.88f, 1.0f, 0.18f * flash);

        drawCubeFace(worldX - 20.0f, worldY - 16.0f, CORE_R * 0.62f,
                     coreAng, sx, sy, cr * 0.55f, cg * 0.55f, cb * 0.55f, 0.72f);
        drawCubeFace(worldX + 20.0f, worldY + 16.0f, CORE_R * 0.62f,
                     coreAng + 0.2f, sx, sy, cr, cg, cb, 0.95f);

        float ca = std::cos(coreAng), sa = std::sin(coreAng);
        for (int i = 0; i < 4; i++) {
            float a = coreAng + PI * 0.25f + (float)i * PI * 0.5f;
            float x = std::cos(a) * CORE_R * 0.72f * sx;
            float y = std::sin(a) * CORE_R * 0.72f * sy;
            float x0 = worldX - 20.0f + x * ca - y * sa;
            float y0 = worldY - 16.0f + x * sa + y * ca;
            float x1 = worldX + 20.0f + x * ca - y * sa;
            float y1 = worldY + 16.0f + x * sa + y * ca;
            drawLine(x0, y0, x1, y1, 3.0f, cr, cg, cb, 0.58f);
        }

        drawRotRect(worldX, worldY, CORE_R * 1.10f * sx, CORE_R * 0.92f * sy,
                    coreAng * -0.65f, 0.04f, 0.025f, 0.06f, 0.9f);
        drawRotRect(worldX, worldY, CORE_R * (0.48f + flash * 0.26f), CORE_R * 0.44f,
                    coreAng * 1.4f, 1.0f, 0.92f, 1.0f, 0.72f + pulse * 0.18f);

        if (phase3)
            drawArcRing(worldX, worldY, CORE_R * 2.08f, 1.0f, 0.16f, 0.32f, 0.62f, 38);
        else if (phase2)
            drawArcRing(worldX, worldY, CORE_R * 1.82f, 0.86f, 0.38f, 1.0f, 0.38f, 32);

        wchar_t tag[32];
        if (phase3) swprintf_s(tag, L"P3 DIMENSION LOCK");
        else if (phase2) swprintf_s(tag, L"P2 MIRROR FAULT");
        else swprintf_s(tag, L"P1 CORE PULSE");
        float tw = g_TextS.Width(tag, 0.48f);
        g_TextS.Draw(tag, worldX - tw * 0.5f, worldY - CORE_R - 38.0f,
                     0.48f, cr, cg, cb, 0.9f);
    }

    const wchar_t* stateTag() const {
        if (phase3) return L"DIMENSION LOCK";
        if (phase2) return L"MIRROR FAULT";
        return L"CORE PULSE";
    }
};
