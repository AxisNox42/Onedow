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
    static constexpr const wchar_t* BOSS_NAME = L"TESSERACT";

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
    float gravityDt = 1.0f / 60.0f;
    int   patternStep = 0;

    // Compatibility only. TESS.glitch no longer uses walls or add orbs.
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
        bool  alive;
    };
    std::vector<Orb> orbs;

    struct Spark {
        float x, y, vx, vy, life;
    };
    std::vector<Spark> sparks;

    struct BreathBullet {
        float x = 0.0f, y = 0.0f;
        float prevX = 0.0f, prevY = 0.0f;
        float dirX = 0.0f, dirY = 0.0f;
        float speed = 0.0f;
        float traveled = 0.0f;
        float maxRange = 0.0f;
        float sizeScale = 0.68f;
        float enemyDmg = 1.0f;
        bool active = true;
        glm::vec3 color = glm::vec3(0.68f, 0.32f, 1.0f);
    };
    std::vector<BreathBullet> breathBullets;

    enum class SkillType { BlackHole, ReverseBurst, GravityBreath, GravityPulse };
    struct Skill {
        SkillType type = SkillType::BlackHole;
        float x = 0.0f, y = 0.0f;
        float radius = 0.0f;
        float width = 0.0f;
        float len = 0.0f;
        float ang = 0.0f;
        float t = 0.0f;
        float warn = 0.6f;
        float dur = 1.0f;
        float dmg = 8.0f;
        float force = 0.0f;
        int   mode = 0;
        bool  hit = false;
        float fireT = 0.0f;
        int   volleys = 0;
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

    static float segDist(float px, float py, float ax, float ay, float bx, float by) {
        float vx = bx - ax;
        float vy = by - ay;
        float wx = px - ax;
        float wy = py - ay;
        float len2 = vx * vx + vy * vy;
        float t = (len2 > 1e-5f) ? (wx * vx + wy * vy) / len2 : 0.0f;
        t = clampf(t, 0.0f, 1.0f);
        float cx = ax + vx * t;
        float cy = ay + vy * t;
        return dist(px, py, cx, cy);
    }

    static float angleDelta(float a, float b) {
        float d = std::fmod(a - b + PI, PI * 2.0f);
        if (d < 0.0f) d += PI * 2.0f;
        return d - PI;
    }

    static bool skillActive(const Skill& s) {
        return s.t >= s.warn && s.t <= s.warn + s.dur;
    }

    static float skillProg(const Skill& s) {
        if (s.t < 0.0f) return 0.0f;
        return clampf(s.t / std::max(0.01f, s.warn), 0.0f, 1.0f);
    }

    bool hitWall(float x0, float y0, float x1, float y1) const {
        (void)x0; (void)y0; (void)x1; (void)y1;
        return false;
    }

    void blockMove(float& pcx, float& pcy, float pr) const {
        (void)pr;
        for (const auto& s : skills) {
            if (!skillActive(s)) continue;
            float dx = s.x - pcx;
            float dy = s.y - pcy;
            float d = std::sqrt(dx * dx + dy * dy) + 1e-3f;

            if (s.type == SkillType::BlackHole && d < s.radius) {
                float pull = (1.0f - d / s.radius) * s.force * gravityDt;
                pcx += dx / d * pull;
                pcy += dy / d * pull;
            } else if (s.type == SkillType::ReverseBurst && d < s.radius + s.width * 1.7f) {
                float band = 1.0f - clampf(std::fabs(d - s.radius) / (s.width * 1.7f), 0.0f, 1.0f);
                float push = band * s.force * gravityDt;
                pcx -= dx / d * push;
                pcy -= dy / d * push;
            } else if (s.type == SkillType::GravityPulse && d < s.radius + s.width * 1.4f) {
                float band = 1.0f - clampf(std::fabs(d - s.radius) / (s.width * 1.4f), 0.0f, 1.0f);
                float push = band * s.force * gravityDt;
                pcx -= dx / d * push;
                pcy -= dy / d * push;
            }
        }
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
        if ((int)skills.size() > 18)
            skills.erase(skills.begin(), skills.begin() + ((int)skills.size() - 18));
    }

    int breathBulletLimit(Difficulty difficulty) const {
        int limit = (difficulty == Difficulty::EASY ? 120 :
                    difficulty == Difficulty::HARD ? 220 : 170);
        if (phase3) limit += 36;
        return limit;
    }

    bool breathQueuedOrActive() const {
        for (const auto& s : skills) {
            if (s.type == SkillType::GravityBreath && s.t <= s.warn + s.dur)
                return true;
        }
        return false;
    }

    void spawnBlackHole(float px, float py, Difficulty difficulty,
                        float delay = 0.0f, bool nearPlayer = true, bool withBurst = true) {
        Skill s{};
        s.type = SkillType::BlackHole;
        if (nearPlayer) {
            s.x = clampf(px + randf(-145.0f, 145.0f), 100.0f, (float)screenW - 100.0f);
            s.y = clampf(py + randf(-115.0f, 115.0f), 110.0f, (float)screenH - 110.0f);
        } else {
            s.x = clampf(worldX + randf(-190.0f, 190.0f), 100.0f, (float)screenW - 100.0f);
            s.y = clampf(worldY + randf(80.0f, 270.0f), 110.0f, (float)screenH - 110.0f);
        }
        s.radius = phase3 ? randf(92.0f, 120.0f) : phase2 ? randf(82.0f, 106.0f) : randf(72.0f, 96.0f);
        if (difficulty == Difficulty::EASY) s.radius *= 0.88f;
        if (difficulty == Difficulty::HARD) s.radius *= 1.05f;
        s.warn = (phase3 ? 0.46f : phase2 ? 0.56f : 0.72f) * warnMul(difficulty);
        s.dur = phase3 ? 1.22f : phase2 ? 1.10f : 0.98f;
        s.dmg = (phase3 ? 11.0f : phase2 ? 9.0f : 7.0f) * damageMul(difficulty);
        s.force = (phase3 ? 185.0f : phase2 ? 155.0f : 125.0f);
        s.width = (phase3 ? 82.0f : phase2 ? 68.0f : 54.0f);
        if (difficulty == Difficulty::EASY) s.width *= 0.86f;
        if (difficulty == Difficulty::HARD) s.width *= 1.12f;
        s.t = -delay;
        addSkill(s);

        if (withBurst) {
            spawnReverseBurst(s.x, s.y, s.radius + randf(18.0f, 36.0f),
                              difficulty, delay + s.warn + s.dur - 0.18f,
                              phase3);
        }
    }

    void spawnReverseBurst(float x, float y, float radius, Difficulty difficulty,
                           float delay = 0.0f, bool strong = false) {
        Skill s{};
        s.type = SkillType::ReverseBurst;
        s.x = clampf(x, 70.0f, (float)screenW - 70.0f);
        s.y = clampf(y, 80.0f, (float)screenH - 80.0f);
        s.radius = radius;
        s.width = strong ? 34.0f : 28.0f;
        if (difficulty == Difficulty::EASY) s.width *= 0.90f;
        if (difficulty == Difficulty::HARD) s.width *= 1.12f;
        s.warn = (strong ? 0.34f : 0.42f) * warnMul(difficulty);
        s.dur = strong ? 0.18f : 0.16f;
        s.dmg = (strong ? 17.0f : 13.0f) * damageMul(difficulty);
        s.force = strong ? 420.0f : 320.0f;
        s.t = -delay;
        addSkill(s);
    }

    void spawnGravityBreath(float px, float py, Difficulty difficulty,
                            float delay = 0.0f, bool wide = false) {
        (void)px; (void)py;
        int cap = breathBulletLimit(difficulty);
        if ((phase3 && breathQueuedOrActive()) ||
            (int)breathBullets.size() > cap * 3 / 4)
            return;

        Skill s{};
        s.type = SkillType::GravityBreath;
        s.x = (float)screenW * 0.5f;
        s.y = (float)screenH * 0.5f;
        s.radius = (float)std::max(screenW, screenH) * 0.48f;
        s.len = (float)std::max(screenW, screenH) * 0.86f;
        s.ang = randf(0.0f, PI * 2.0f);
        s.width = (difficulty == Difficulty::EASY ? 0.145f :
                   difficulty == Difficulty::HARD ? 0.102f : 0.122f);
        if (wide) s.width *= 0.92f;
        s.warn = (phase3 ? 0.74f : 0.92f) * warnMul(difficulty);
        s.dur = phase3 ? 1.08f : (wide ? 1.02f : 0.92f);
        s.dmg = 0.0f;
        s.force = (difficulty == Difficulty::EASY ? 0.175f :
                   difficulty == Difficulty::HARD ? 0.128f : 0.152f);
        s.mode = (difficulty == Difficulty::EASY ? 24 :
                  difficulty == Difficulty::HARD ? 34 : 29);
        if (wide) s.mode += 3;
        s.t = -delay;
        addSkill(s);
    }

    void spawnGravityPulse(float px, float py, Difficulty difficulty,
                           float delay = 0.0f, bool nearPlayer = true) {
        Skill s{};
        s.type = SkillType::GravityPulse;
        if (nearPlayer) {
            s.x = clampf(px + randf(-95.0f, 95.0f), 80.0f, (float)screenW - 80.0f);
            s.y = clampf(py + randf(-80.0f, 80.0f), 90.0f, (float)screenH - 90.0f);
        } else {
            s.x = clampf(worldX + randf(-130.0f, 130.0f), 80.0f, (float)screenW - 80.0f);
            s.y = clampf(worldY + randf(45.0f, 190.0f), 90.0f, (float)screenH - 90.0f);
        }
        s.radius = phase3 ? randf(108.0f, 142.0f) : phase2 ? randf(96.0f, 126.0f) : randf(82.0f, 112.0f);
        s.width = phase3 ? 30.0f : 24.0f;
        if (difficulty == Difficulty::EASY) {
            s.radius *= 0.90f;
            s.width *= 0.86f;
        }
        if (difficulty == Difficulty::HARD) {
            s.radius *= 1.07f;
            s.width *= 1.12f;
        }
        s.warn = (phase3 ? 0.34f : phase2 ? 0.42f : 0.54f) * warnMul(difficulty);
        s.dur = 0.13f;
        s.dmg = (phase3 ? 11.0f : phase2 ? 9.0f : 7.0f) * damageMul(difficulty);
        s.force = phase3 ? 265.0f : 210.0f;
        s.t = -delay;
        addSkill(s);
    }

    void spawnBasicSkill(float px, float py, Difficulty difficulty,
                         float delay = 0.0f) {
        int r = rand() % 3;
        if (r == 0) {
            spawnGravityPulse(px, py, difficulty, delay, true);
        } else if (r == 1) {
            spawnReverseBurst(px + randf(-80.0f, 80.0f), py + randf(-65.0f, 65.0f),
                              randf(82.0f, phase3 ? 136.0f : 118.0f),
                              difficulty, delay, phase3);
        } else {
            spawnBlackHole(px, py, difficulty, delay, true, false);
        }
    }

    bool gravityBreathCueActive() const {
        for (const auto& s : skills) {
            if (s.type == SkillType::GravityBreath &&
                s.t >= -0.04f && s.t <= s.warn + s.dur)
                return true;
        }
        return false;
    }

    bool inBreathGap(const Skill& s, float a) const {
        for (int i = 0; i < 3; i++) {
            float gap = s.ang + (PI * 2.0f / 3.0f) * (float)i;
            if (std::fabs(angleDelta(a, gap)) <= s.width)
                return true;
        }
        return false;
    }

    void fireGravityBreathVolley(Skill& s, Difficulty difficulty) {
        int allowed = breathBulletLimit(difficulty) - (int)breathBullets.size();
        if (allowed <= 0) return;

        int dirs = std::max(12, s.mode);
        float jitter = ((s.volleys & 1) ? 0.5f : 0.0f) * (PI * 2.0f / (float)dirs);
        float spd = (difficulty == Difficulty::EASY ? 248.0f :
                    difficulty == Difficulty::HARD ? 326.0f : 288.0f);
        if (phase3) spd += 22.0f;
        float dmg = (difficulty == Difficulty::EASY ? 0.95f :
                    difficulty == Difficulty::HARD ? 1.55f : 1.22f);
        if (phase3) dmg += 0.15f;

        for (int i = 0; i < dirs; i++) {
            float a = (float)i / (float)dirs * PI * 2.0f + jitter;
            if (inBreathGap(s, a)) continue;

            BreathBullet b{};
            b.x = b.prevX = s.x;
            b.y = b.prevY = s.y;
            b.dirX = std::cos(a);
            b.dirY = std::sin(a);
            b.speed = spd + randf(-12.0f, 18.0f);
            b.sizeScale = 0.68f;
            b.enemyDmg = dmg * damageMul(difficulty);
            b.maxRange = s.len;
            b.color = phase3 ? glm::vec3(1.0f, 0.18f, 0.38f)
                              : glm::vec3(0.68f, 0.32f, 1.0f);
            breathBullets.push_back(b);
            if (--allowed <= 0) break;
        }
        s.volleys++;
    }

    void updateBreathBullets(float dt, float px, float py, float& playerHP,
                             std::vector<Bullet>& playerBullets, bool dashInvuln,
                             Difficulty difficulty) {
        if (breathBullets.empty()) return;

        const float margin = 180.0f;
        for (auto& eb : breathBullets) {
            if (!eb.active) continue;

            eb.prevX = eb.x;
            eb.prevY = eb.y;
            float step = eb.speed * dt;
            eb.x += eb.dirX * step;
            eb.y += eb.dirY * step;
            eb.traveled += step;

            if (eb.traveled > eb.maxRange ||
                eb.x < -margin || eb.x > (float)screenW + margin ||
                eb.y < -margin || eb.y > (float)screenH + margin) {
                eb.active = false;
                continue;
            }

            float playerHitR = 10.0f + 7.0f * eb.sizeScale;
            if (!dashInvuln &&
                segDist(px, py, eb.prevX, eb.prevY, eb.x, eb.y) < playerHitR) {
                HurtPlayer(playerHP, eb.enemyDmg);
                eb.active = false;
                pushSpark(eb.x, eb.y, eb.color.r, eb.color.g, eb.color.b);
            }
        }

        bool hasPlayerShots = false;
        for (const auto& b : playerBullets) {
            if (b.active && !b.isEnemy) {
                hasPlayerShots = true;
                break;
            }
        }

        if (hasPlayerShots) {
            const float cell = 96.0f;
            int cols = std::max(1, (int)(((float)screenW + margin * 2.0f) / cell) + 1);
            int rows = std::max(1, (int)(((float)screenH + margin * 2.0f) / cell) + 1);
            std::vector<int> head(cols * rows, -1);
            std::vector<int> links;
            std::vector<int> refs;
            links.reserve(playerBullets.size() * 2);
            refs.reserve(playerBullets.size() * 2);

            auto clampi = [](int v, int lo, int hi) {
                return v < lo ? lo : (v > hi ? hi : v);
            };
            auto cellMin = [&](float v, int maxCell) {
                return clampi((int)std::floor((v + margin) / cell), 0, maxCell);
            };
            auto addToCell = [&](int bulletIdx, int cellIdx) {
                refs.push_back(bulletIdx);
                links.push_back(head[cellIdx]);
                head[cellIdx] = (int)refs.size() - 1;
            };

            for (int pi = 0; pi < (int)playerBullets.size(); ++pi) {
                const Bullet& pb = playerBullets[pi];
                if (!pb.active || pb.isEnemy) continue;

                float pad = 16.0f * pb.sizeScale + 12.0f;
                int x0 = cellMin(std::min(pb.prevX, pb.x) - pad, cols - 1);
                int x1 = cellMin(std::max(pb.prevX, pb.x) + pad, cols - 1);
                int y0 = cellMin(std::min(pb.prevY, pb.y) - pad, rows - 1);
                int y1 = cellMin(std::max(pb.prevY, pb.y) + pad, rows - 1);
                for (int cy = y0; cy <= y1; ++cy) {
                    int row = cy * cols;
                    for (int cx = x0; cx <= x1; ++cx)
                        addToCell(pi, row + cx);
                }
            }

            std::vector<int> seen(playerBullets.size(), 0);
            int stamp = 1;
            for (auto& eb : breathBullets) {
                if (!eb.active) continue;

                float enemyHitR = 7.0f * eb.sizeScale + 6.0f;
                int x0 = cellMin(std::min(eb.prevX, eb.x) - enemyHitR, cols - 1);
                int x1 = cellMin(std::max(eb.prevX, eb.x) + enemyHitR, cols - 1);
                int y0 = cellMin(std::min(eb.prevY, eb.y) - enemyHitR, rows - 1);
                int y1 = cellMin(std::max(eb.prevY, eb.y) + enemyHitR, rows - 1);

                ++stamp;
                for (int cy = y0; cy <= y1 && eb.active; ++cy) {
                    int row = cy * cols;
                    for (int cx = x0; cx <= x1 && eb.active; ++cx) {
                        for (int entry = head[row + cx]; entry >= 0; entry = links[entry]) {
                            int pi = refs[entry];
                            if (seen[pi] == stamp) continue;
                            seen[pi] = stamp;

                            Bullet& pb = playerBullets[pi];
                            if (!pb.active || pb.isEnemy) continue;

                            float playerHitR = 6.0f * pb.sizeScale;
                            float d1 = segDist(eb.x, eb.y, pb.prevX, pb.prevY, pb.x, pb.y);
                            float d2 = segDist(pb.x, pb.y, eb.prevX, eb.prevY, eb.x, eb.y);
                            if (std::min(d1, d2) >= enemyHitR + playerHitR) continue;

                            eb.active = false;
                            if (pb.pierceRemaining > 0.001f) {
                                pb.pierceRemaining -= 1.0f;
                                if (pb.pierceRemaining <= 0.001f) pb.active = false;
                            } else {
                                pb.active = false;
                            }
                            pushSpark(eb.x, eb.y, eb.color.r, eb.color.g, eb.color.b);
                            break;
                        }
                    }
                }
            }
        }

        int cap = breathBulletLimit(difficulty);
        if ((int)breathBullets.size() > cap) {
            int overflow = (int)breathBullets.size() - cap;
            breathBullets.erase(breathBullets.begin(), breathBullets.begin() + overflow);
        }
        breathBullets.erase(std::remove_if(breathBullets.begin(), breathBullets.end(),
            [](const BreathBullet& b) { return !b.active; }), breathBullets.end());
    }

    void spawnPattern(float px, float py, Difficulty difficulty) {
        ++patternStep;
        if (!phase2) {
            if ((patternStep % 4) == 0) {
                spawnBasicSkill(px, py, difficulty);
                spawnGravityPulse(px, py, difficulty, 0.36f, false);
            } else if ((patternStep % 3) == 0) {
                spawnBlackHole(px, py, difficulty, 0.0f, true, true);
            } else {
                spawnBasicSkill(px, py, difficulty);
                spawnBlackHole(px, py, difficulty, 0.0f, patternStep & 1, false);
            }
            skillCd = randf(0.96f, 1.25f) * tempoMul(difficulty);
            return;
        }

        if (!phase3) {
            int p = patternStep % 4;
            if (p == 0) {
                spawnBasicSkill(px, py, difficulty);
                spawnBlackHole(px, py, difficulty, 0.0f, true, true);
                spawnGravityBreath(px, py, difficulty, 0.58f, false);
            } else if (p == 1) {
                spawnBasicSkill(px, py, difficulty);
                spawnGravityBreath(px, py, difficulty);
                spawnReverseBurst(px, py, randf(118.0f, 158.0f), difficulty, 0.70f);
            } else if (p == 2) {
                spawnGravityPulse(px, py, difficulty, 0.0f, true);
                spawnBlackHole(px, py, difficulty, 0.0f, true, false);
                spawnBlackHole(px, py, difficulty, 0.38f, false, true);
            } else {
                spawnBasicSkill(px, py, difficulty);
                spawnGravityBreath(px, py, difficulty, 0.0f, false);
            }
            skillCd = randf(0.82f, 1.08f) * tempoMul(difficulty);
            return;
        }

        int p = patternStep % 5;
        if (p == 0) {
            spawnBasicSkill(px, py, difficulty);
            spawnBlackHole(px, py, difficulty, 0.0f, true, true);
            spawnBlackHole(px, py, difficulty, 0.22f, false, true);
        } else if (p == 1) {
            spawnGravityPulse(px, py, difficulty, 0.0f, true);
            spawnBlackHole(px, py, difficulty, 0.0f, true, false);
            spawnGravityBreath(px, py, difficulty, 0.48f, true);
            spawnReverseBurst(px, py, randf(150.0f, 205.0f), difficulty, 1.02f, true);
        } else if (p == 2) {
            spawnBasicSkill(px, py, difficulty);
            spawnGravityBreath(px, py, difficulty, 0.0f, false);
            spawnGravityBreath(px, py, difficulty, 0.62f, false);
        } else if (p == 3) {
            spawnGravityPulse(px, py, difficulty, 0.0f, false);
            spawnBlackHole(px, py, difficulty, 0.0f, false, true);
            spawnGravityBreath(px, py, difficulty, 0.42f, false);
        } else {
            spawnReverseBurst(worldX, worldY, randf(130.0f, 180.0f), difficulty, 0.0f, true);
            spawnBlackHole(px, py, difficulty, 0.32f, true, false);
            spawnBasicSkill(px, py, difficulty, 0.58f);
        }
        skillCd = randf(0.56f, 0.78f) * tempoMul(difficulty);
    }

    bool skillHitsPlayer(const Skill& s, float px, float py) const {
        if (s.type == SkillType::BlackHole)
            return dist(px, py, s.x, s.y) <= s.radius;
        if (s.type == SkillType::ReverseBurst || s.type == SkillType::GravityPulse)
            return std::fabs(dist(px, py, s.x, s.y) - s.radius) <= s.width;
        return false;
    }

    void updateBlackHoleMotion(Skill& s, float px, float py, float dt) {
        if (s.type != SkillType::BlackHole || !skillActive(s)) return;

        float dx = px - s.x;
        float dy = py - s.y;
        float d = std::sqrt(dx * dx + dy * dy);
        if (d < 1e-3f) return;

        float step = std::min(d, s.width * dt);
        s.x += dx / d * step;
        s.y += dy / d * step;
        s.x = clampf(s.x, 70.0f, (float)screenW - 70.0f);
        s.y = clampf(s.y, 80.0f, (float)screenH - 80.0f);
    }

    void Update(float px, float py, float dt, float& playerHP,
                std::vector<Bullet>& bullets, Difficulty difficulty,
                bool dashInvuln) {
        if (!alive) return;
        gravityDt = dt;

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
        coreAng += dt * (phase3 ? 5.75f : (phase2 ? 2.85f : 1.25f));
        if (faceFlash > 0.0f) faceFlash -= dt;

        float hoverY = phase3 ? (float)screenH * 0.26f : (float)screenH * 0.32f;
        float targetX = (float)screenW * 0.5f + std::sin(glitchT * 1.4f) * (phase3 ? 86.0f : 118.0f);
        targetX += (px - (float)screenW * 0.5f) * (phase3 ? 0.16f : 0.10f);
        float targetY = hoverY + std::cos(glitchT * 1.05f) * (phase3 ? 22.0f : 34.0f);
        float follow = phase3 ? 1.2f : (phase2 ? 0.96f : 0.68f);
        if (gravityBreathCueActive()) {
            targetX = (float)screenW * 0.5f;
            targetY = (float)screenH * 0.5f;
            follow = phase3 ? 3.65f : 3.15f;
        }
        worldX += (targetX - worldX) * follow * dt;
        worldY += (targetY - worldY) * follow * dt;
        worldX = clampf(worldX, 140.0f, (float)screenW - 140.0f);
        worldY = clampf(worldY, 100.0f, (float)screenH - 90.0f);

        skillCd -= dt;
        if (skillCd <= 0.0f)
            spawnPattern(px, py, difficulty);

        for (auto& s : skills) {
            s.t += dt;
            if (s.t < 0.0f) continue;

            bool active = skillActive(s);
            updateBlackHoleMotion(s, px, py, dt);
            if (s.type == SkillType::GravityBreath && active) {
                s.fireT -= dt;
                while (s.fireT <= 0.0f) {
                    fireGravityBreathVolley(s, difficulty);
                    s.fireT += std::max(0.035f, s.force);
                }
            }
            if (!active || dashInvuln) continue;

            if (s.type == SkillType::BlackHole) {
                if (skillHitsPlayer(s, px, py))
                    HurtPlayer(playerHP, s.dmg * dt);
            } else if (!s.hit && skillHitsPlayer(s, px, py)) {
                HurtPlayer(playerHP, s.dmg);
                s.hit = true;
                pushSpark(px, py, 1.0f, 0.22f, 0.38f);
            }
        }
        skills.erase(std::remove_if(skills.begin(), skills.end(),
            [](const Skill& s) { return s.t > s.warn + s.dur + 0.20f; }), skills.end());

        updateBreathBullets(dt, px, py, playerHP, bullets, dashInvuln, difficulty);

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

    void drawBlackHoleSkill(const Skill& s, float t) const {
        float p = skillProg(s);
        bool active = skillActive(s);
        float activeT = active ? std::max(0.0f, s.t - s.warn) : 0.0f;
        float pulse = 0.5f + 0.5f * std::sin(t * 13.0f + s.x * 0.01f);
        float r = phase3 ? 1.0f : 0.55f;
        float g = phase3 ? 0.16f : 0.82f;
        float b = 1.0f;

        if (!active) {
            float closingR = s.radius * (1.34f - p * 0.34f);
            float tickA = 0.20f + p * 0.58f;
            drawCircle(s.x, s.y, s.radius, 0.02f, 0.005f, 0.04f, 0.16f + p * 0.12f);
            drawArcRing(s.x, s.y, s.radius, r, g, b, 0.28f + p * 0.28f, 34);
            drawArcRing(s.x, s.y, closingR, 1.0f, 0.92f, 1.0f, tickA, 38);
            for (int i = 0; i < 8; i++) {
                float a = (float)i / 8.0f * PI * 2.0f;
                float ca = std::cos(a), sa = std::sin(a);
                float inner = s.radius * (0.82f + p * 0.05f);
                float outer = s.radius * (1.08f + pulse * 0.03f);
                drawLine(s.x + ca * inner, s.y + sa * inner,
                         s.x + ca * outer, s.y + sa * outer,
                         3.0f + p * 1.4f, 1.0f, 0.90f, 1.0f, tickA);
            }
            drawCircle(s.x, s.y, 9.0f + p * 10.0f, 0.0f, 0.0f, 0.01f, 0.82f);
            return;
        }

        float drawR = s.radius * (0.98f + pulse * 0.05f);
        drawCircle(s.x, s.y, drawR, 0.01f, 0.005f, 0.025f, 0.72f);
        drawCircle(s.x, s.y, s.radius * (0.58f + pulse * 0.03f),
                   0.0f, 0.0f, 0.01f, 0.60f);
        drawArcRing(s.x, s.y, s.radius, r, g, b, 0.60f + pulse * 0.16f, 36);
        drawArcRing(s.x, s.y, s.radius * (0.66f + pulse * 0.02f),
                    0.92f, 1.0f, 0.96f, 0.30f + pulse * 0.20f, 24);
        drawArcRing(s.x, s.y, s.radius * (0.36f + pulse * 0.02f),
                    1.0f, 0.9f, 1.0f, 0.26f + pulse * 0.18f, 18);
        for (int i = 0; i < 10; i++) {
            float a = (float)i / 10.0f * PI * 2.0f + (phase3 ? PI * 0.10f : 0.0f);
            float ca = std::cos(a), sa = std::sin(a);
            float longTick = (i % 5 == 0) ? 1.0f : 0.0f;
            float inner = s.radius * (0.28f + longTick * 0.06f);
            float outer = s.radius * (0.92f + pulse * 0.04f);
            drawLine(s.x + ca * inner, s.y + sa * inner,
                     s.x + ca * outer, s.y + sa * outer,
                     2.8f + longTick * 1.6f, 0.88f, 1.0f, 0.96f, 0.48f);
        }
        drawCircle(s.x, s.y, 11.0f + pulse * 7.0f + activeT * 2.0f,
                   0.0f, 0.0f, 0.01f, 0.96f);
    }

    void drawReverseBurstSkill(const Skill& s, float t) const {
        float p = skillProg(s);
        bool active = skillActive(s);
        float flash = active ? 0.85f : 0.24f + p * 0.35f;
        drawArcRing(s.x, s.y, s.radius - s.width, 1.0f, 0.9f, 1.0f, flash * 0.55f, 36);
        drawArcRing(s.x, s.y, s.radius, phase3 ? 1.0f : 0.85f, phase3 ? 0.18f : 0.42f,
                    1.0f, flash, 46);
        drawArcRing(s.x, s.y, s.radius + s.width, 1.0f, 0.9f, 1.0f, flash * 0.55f, 36);
        if (active)
            drawCircle(s.x, s.y, s.radius + s.width * 1.6f, 1.0f, 0.25f, 0.72f, 0.08f);
    }

    void drawGravityPulseSkill(const Skill& s, float t) const {
        float p = skillProg(s);
        bool active = skillActive(s);
        float pulse = 0.5f + 0.5f * std::sin(t * 24.0f + s.x * 0.02f);
        float a = active ? 0.74f : 0.18f + p * 0.32f;
        drawArcRing(s.x, s.y, s.radius - s.width, 0.70f, 1.0f, 0.92f, a * 0.42f, 30);
        drawArcRing(s.x, s.y, s.radius, phase3 ? 1.0f : 0.42f,
                    phase3 ? 0.24f : 0.92f, 1.0f, a, 42);
        drawArcRing(s.x, s.y, s.radius + s.width, 1.0f, 0.92f, 1.0f, a * 0.48f, 30);
        if (active)
            drawCircle(s.x, s.y, s.radius + pulse * s.width, 0.45f, 0.96f, 1.0f, 0.10f);
    }

    void drawBreathSkill(const Skill& s, float t) const {
        float p = skillProg(s);
        bool active = skillActive(s);
        float pulse = 0.5f + 0.5f * std::sin(t * 20.0f);
        float r = phase3 ? 1.0f : 0.76f;
        float g = phase3 ? 0.18f : 0.34f;
        float b = phase3 ? 0.36f : 1.0f;
        float dangerA = active ? 0.34f + pulse * 0.10f : 0.08f + p * 0.18f;
        float safeA = active ? 0.58f : 0.20f + p * 0.24f;

        drawCircle(s.x, s.y, s.radius * (0.42f + pulse * 0.08f),
                   0.02f, 0.00f, 0.04f, active ? 0.30f : 0.12f + p * 0.10f);
        drawArcRing(s.x, s.y, s.radius * 0.34f, 1.0f, 0.92f, 1.0f, 0.18f + p * 0.22f, 22);
        drawArcRing(s.x, s.y, s.radius * 0.58f, r, g, b, 0.18f + p * 0.24f, 30);

        int spokes = active ? 30 : 42;
        float inner = 34.0f;
        float outer = s.len;
        for (int i = 0; i < spokes; i++) {
            float a = (float)i / (float)spokes * PI * 2.0f + t * 0.04f;
            if (inBreathGap(s, a)) continue;
            float ca = std::cos(a), sa = std::sin(a);
            drawLine(s.x + ca * inner, s.y + sa * inner,
                     s.x + ca * outer, s.y + sa * outer,
                     active ? 2.2f : 1.6f, r, g, b, dangerA);
        }

        for (int i = 0; i < 3; i++) {
            float gap = s.ang + (PI * 2.0f / 3.0f) * (float)i;
            for (int edge = -1; edge <= 1; edge += 2) {
                float a = gap + s.width * (float)edge;
                float ca = std::cos(a), sa = std::sin(a);
                drawLine(s.x + ca * 28.0f, s.y + sa * 28.0f,
                         s.x + ca * outer, s.y + sa * outer,
                         4.0f, 0.36f, 1.0f, 0.82f, safeA);
            }
            float ca = std::cos(gap), sa = std::sin(gap);
            drawLine(s.x + ca * 38.0f, s.y + sa * 38.0f,
                     s.x + ca * outer, s.y + sa * outer,
                     2.0f, 0.80f, 1.0f, 0.92f, safeA * 0.72f);
        }

        drawCircle(s.x, s.y, 18.0f + pulse * 12.0f, 0.0f, 0.0f, 0.01f, 0.94f);
    }

    void drawBreathBullet(const BreathBullet& b, float t) const {
        if (!b.active) return;

        float pulse = 0.5f + 0.5f * std::sin(t * 26.0f + b.x * 0.035f);
        float coreR = 5.6f * b.sizeScale;
        float tail = 18.0f + pulse * 5.0f;
        drawLine(b.x - b.dirX * tail, b.y - b.dirY * tail, b.x, b.y,
                 3.0f, b.color.r, b.color.g, b.color.b, 0.34f);
        drawCircle(b.x, b.y, coreR * 1.85f, 1.0f, 1.0f, 1.0f, 0.16f + pulse * 0.06f);
        drawCircle(b.x, b.y, coreR * 1.20f, b.color.r, b.color.g, b.color.b, 0.78f);
        drawCircle(b.x, b.y, coreR * 0.58f, 0.05f, 0.01f, 0.08f, 0.86f);
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

    void renderSingularityField(float t) const {
        float power = phase3 ? 1.0f : (phase2 ? 0.58f : 0.28f);
        float cr = phase3 ? 1.0f : (phase2 ? 0.76f : 0.36f);
        float cg = phase3 ? 0.16f : (phase2 ? 0.34f : 0.86f);
        float cb = phase3 ? 0.34f : 1.0f;
        float maxR = (float)std::max(screenW, screenH);

        drawCircle(worldX, worldY, maxR * (0.30f + power * 0.05f),
                   cr, cg, cb, 0.020f + power * 0.030f);
        drawArcRing(worldX, worldY, CORE_R * (2.95f + power * 0.70f),
                    cr, cg, cb, 0.10f + power * 0.14f, phase3 ? 48 : 34);
        drawArcRing(worldX, worldY, CORE_R * (3.85f + std::sin(t * 1.4f) * 0.16f),
                    0.34f, 1.0f, 0.82f, 0.07f + power * 0.09f, phase3 ? 52 : 36);

        int rays = phase3 ? 26 : (phase2 ? 16 : 9);
        for (int i = 0; i < rays; i++) {
            float u = (float)i / (float)rays;
            float a = u * PI * 2.0f + std::sin(t * 0.44f + (float)i * 1.91f) * (0.12f + power * 0.10f);
            float edge = maxR * (0.58f + 0.10f * std::sin(t * 0.7f + (float)i));
            float sx = (float)screenW * 0.5f + std::cos(a) * edge;
            float sy = (float)screenH * 0.5f + std::sin(a) * edge;
            float ex = worldX + std::cos(a + PI) * CORE_R * (1.20f + power * 0.25f);
            float ey = worldY + std::sin(a + PI) * CORE_R * (1.20f + power * 0.25f);
            float alt = (i % 3 == 0) ? 1.0f : 0.0f;
            drawLine(sx, sy, ex, ey, 1.2f + power * 1.0f + alt,
                     alt ? 0.34f : cr, alt ? 1.0f : cg, alt ? 0.82f : cb,
                     0.045f + power * 0.070f);
        }

        if (phase3) {
            for (int i = 0; i < 7; i++) {
                float y = ((float)i + 0.5f) / 7.0f * (float)screenH;
                float xOff = std::sin(t * (2.2f + i * 0.17f) + (float)i * 2.0f) * 42.0f;
                drawRect(xOff, y, (float)screenW, 2.0f,
                         1.0f, 0.16f, 0.34f, 0.040f);
            }
        }
    }

    void renderFx(float t, float aimX, float aimY) const {
        (void)aimX; (void)aimY;
        renderSingularityField(t);
        renderNoise(t);
        for (const auto& s : skills) {
            if (s.t < 0.0f) continue;
            if (s.type == SkillType::BlackHole) drawBlackHoleSkill(s, t);
            else if (s.type == SkillType::ReverseBurst) drawReverseBurstSkill(s, t);
            else if (s.type == SkillType::GravityBreath) drawBreathSkill(s, t);
            else if (s.type == SkillType::GravityPulse) drawGravityPulseSkill(s, t);
        }
        for (const auto& b : breathBullets)
            drawBreathBullet(b, t);

        for (const auto& s : sparks) {
            float a = clampf(s.life / 0.34f, 0.0f, 1.0f);
            drawRotRect(s.x, s.y, 5.0f + 7.0f * a, 5.0f + 7.0f * a,
                        t * 5.0f + s.x * 0.01f, 1.0f, phase3 ? 0.22f : 0.52f,
                        phase3 ? 0.38f : 1.0f, a);
        }
    }

    void drawOrbit(float t, float rad, float tilt, float r, float g, float b, float a,
                   float spinMul = 1.0f, float wobble = 0.0f) const {
        int n = 40;
        float prevX = 0.0f, prevY = 0.0f;
        float axis = coreAng * 0.55f * spinMul + tilt +
                     std::sin(glitchT * (2.2f + tilt * 1.7f)) * wobble;
        for (int i = 0; i <= n; i++) {
            float u = (float)i / (float)n * PI * 2.0f;
            float x = std::cos(u) * rad;
            float y = std::sin(u) * rad * tilt;
            float ca = std::cos(axis), sa = std::sin(axis);
            float wx = worldX + x * ca - y * sa;
            float wy = worldY + x * sa + y * ca;
            bool brokenGap = wobble > 0.3f && ((i + (int)(glitchT * 11.0f)) % 9) == 0;
            if (i > 0 && (i % 3) != 0 && !brokenGap)
                drawLine(prevX, prevY, wx, wy, 2.0f + wobble, r, g, b, a);
            prevX = wx; prevY = wy;
        }
        float p = t * (0.75f + tilt) * spinMul + tilt * 4.0f;
        float px = std::cos(p) * rad;
        float py = std::sin(p) * rad * tilt;
        float ca = std::cos(axis), sa = std::sin(axis);
        drawCircle(worldX + px * ca - py * sa, worldY + px * sa + py * ca,
                   4.0f + wobble * 1.5f, r, g, b, 0.75f);
    }

    void drawCubeFace(float cx, float cy, float size, float ang,
                      float sx, float sy, float r, float g, float b, float a,
                      float collapse = 0.0f) const {
        float pts[4][2] = {
            {-size, -size}, { size, -size}, { size, size}, {-size, size}
        };
        float wx[4], wy[4];
        float c = std::cos(ang), s = std::sin(ang);
        for (int i = 0; i < 4; i++) {
            float x = pts[i][0] * sx;
            float y = pts[i][1] * sy;
            float shear = std::sin(glitchT * (4.0f + (float)i) + (float)i * 1.7f) *
                          collapse * 0.22f;
            float jitter = std::sin(glitchT * (24.0f + collapse * 18.0f) + (float)i * 2.4f) *
                           (2.0f + collapse * 12.0f);
            jitter += std::sin(glitchT * (43.0f + (float)i * 3.0f)) * collapse * 5.0f;
            float sx2 = x + y * shear;
            float sy2 = y - x * shear * 0.55f;
            wx[i] = cx + sx2 * c - sy2 * s + jitter;
            wy[i] = cy + sx2 * s + sy2 * c - jitter * (0.45f + collapse * 0.20f);
        }
        for (int i = 0; i < 4; i++) {
            int j = (i + 1) & 3;
            float thick = 4.0f + collapse * ((i & 1) ? 1.0f : 2.5f);
            drawLine(wx[i], wy[i], wx[j], wy[j], thick, r, g, b, a);
        }
        if (collapse > 0.01f) {
            drawLine(wx[0], wy[0], wx[2], wy[2], 2.0f + collapse * 1.5f,
                     1.0f, 0.92f, 1.0f, a * collapse * 0.46f);
            drawLine(wx[1], wy[1], wx[3], wy[3], 1.5f + collapse,
                     r, g, b, a * collapse * 0.34f);
        }
    }

    void renderBody(float t) const {
        float collapse = phase3 ? 1.0f : (phase2 ? 0.42f : 0.0f);
        float pulse = 0.5f + 0.5f * std::sin(t * (phase3 ? 15.5f : phase2 ? 9.0f : 7.5f));
        float warp = std::sin(t * (phase3 ? 6.7f : 4.1f)) * collapse;
        float cr = phase3 ? 1.0f : (phase2 ? 0.84f : 0.40f);
        float cg = phase3 ? 0.18f : (phase2 ? 0.36f : 0.92f);
        float cb = phase3 ? 0.34f : 1.0f;
        float sx = 1.0f + pulse * 0.06f - collapse * 0.18f + warp * 0.26f;
        float sy = 1.0f - pulse * 0.04f + collapse * 0.16f - warp * 0.19f;
        float flash = clampf(faceFlash / 0.20f, 0.0f, 1.0f);
        float orbitSpin = phase3 ? 2.45f : (phase2 ? 1.45f : 1.0f);
        float orbitBreak = phase3 ? 0.62f : (phase2 ? 0.22f : 0.0f);

        drawOrbit(t, CORE_R * 1.72f, 0.36f, cr, cg, cb, 0.32f + flash * 0.18f,
                  orbitSpin, orbitBreak);
        drawOrbit(t, CORE_R * 2.10f, 0.62f, 0.26f, 1.0f, 0.82f, 0.24f,
                  orbitSpin * 0.86f, orbitBreak * 0.7f);
        if (phase2) drawOrbit(t, CORE_R * 2.42f, 0.48f, 0.88f, 0.34f, 1.0f, 0.26f,
                              orbitSpin * 1.18f, orbitBreak);
        if (phase3) {
            drawOrbit(t, CORE_R * 2.72f, 0.74f, 1.0f, 0.18f, 0.36f, 0.28f,
                      orbitSpin * 1.36f, orbitBreak * 1.1f);
            drawOrbit(t, CORE_R * 3.06f, 0.42f, 0.42f, 1.0f, 0.72f, 0.16f,
                      orbitSpin * 1.62f, orbitBreak * 1.25f);
            drawOrbit(t, CORE_R * 3.46f, 0.58f, 1.0f, 0.86f, 0.42f, 0.14f,
                      orbitSpin * 1.92f, orbitBreak * 1.35f);
        }

        drawCircle(worldX, worldY, CORE_R * (1.45f + pulse * 0.18f),
                   cr, cg, cb, phase3 ? 0.18f : 0.10f);
        if (flash > 0.0f)
            drawCircle(worldX, worldY, CORE_R * (1.85f + flash * 0.3f),
                       1.0f, 0.88f, 1.0f, 0.18f * flash);

        int gates = phase3 ? 8 : (phase2 ? 5 : 3);
        for (int i = 0; i < gates; i++) {
            float a = coreAng * (phase3 ? -0.72f : -0.36f) + (float)i / (float)gates * PI * 2.0f;
            float wob = std::sin(t * (3.4f + i * 0.23f) + (float)i * 1.21f);
            float rad = CORE_R * (2.18f + collapse * 0.92f + wob * 0.10f);
            float gx = worldX + std::cos(a) * rad;
            float gy = worldY + std::sin(a) * rad * (0.58f + collapse * 0.10f);
            float size = CORE_R * (0.18f + collapse * 0.08f);
            drawRotRect(gx, gy, size * 1.8f, size * 0.72f,
                        -coreAng + a, cr, cg, cb, 0.26f + collapse * 0.16f);
            drawLine(worldX, worldY, gx, gy, 1.4f + collapse * 1.0f,
                     cr, cg, cb, 0.10f + collapse * 0.08f);
        }

        float splitX = 20.0f + collapse * std::sin(t * 8.2f) * 13.0f;
        float splitY = 16.0f + collapse * std::cos(t * 7.6f) * 10.0f;
        drawCubeFace(worldX - splitX, worldY - splitY, CORE_R * (0.62f + collapse * 0.04f),
                     coreAng * (1.0f + collapse * 0.18f), sx, sy,
                     cr * 0.55f, cg * 0.55f, cb * 0.55f, 0.72f, collapse);
        drawCubeFace(worldX + splitX, worldY + splitY, CORE_R * (0.62f - collapse * 0.03f),
                     coreAng * (1.0f - collapse * 0.10f) + 0.2f, sx, sy,
                     cr, cg, cb, 0.95f, collapse);
        if (phase3) {
            drawCubeFace(worldX + std::sin(t * 5.7f) * 18.0f,
                         worldY - std::cos(t * 6.1f) * 14.0f,
                         CORE_R * 0.43f, -coreAng * 1.35f,
                         sy * 0.82f, sx * 1.12f, 1.0f, 0.18f, 0.36f, 0.34f, 1.0f);
        }

        float ca = std::cos(coreAng), sa = std::sin(coreAng);
        for (int i = 0; i < 4; i++) {
            float a = coreAng + PI * 0.25f + (float)i * PI * 0.5f;
            float x = std::cos(a) * CORE_R * 0.72f * sx;
            float y = std::sin(a) * CORE_R * 0.72f * sy;
            float x0 = worldX - splitX + x * ca - y * sa;
            float y0 = worldY - splitY + x * sa + y * ca;
            float x1 = worldX + splitX + x * ca - y * sa;
            float y1 = worldY + splitY + x * sa + y * ca;
            drawLine(x0, y0, x1, y1, 3.0f + collapse * 1.2f, cr, cg, cb, 0.58f);
        }

        if (collapse > 0.0f) {
            int shards = phase3 ? 22 : 9;
            for (int i = 0; i < shards; i++) {
                float a = coreAng * (phase3 ? 1.35f : 0.9f) + (float)i / (float)shards * PI * 2.0f;
                float wob = std::sin(t * (7.0f + (float)i) + (float)i * 1.31f);
                float inner = CORE_R * (1.05f + collapse * 0.30f + wob * 0.05f);
                float outer = CORE_R * (1.42f + collapse * 0.92f + wob * 0.14f);
                float tx = std::cos(a), ty = std::sin(a);
                drawLine(worldX + tx * inner, worldY + ty * inner,
                         worldX + tx * outer + std::sin(t * 11.0f + i) * collapse * 8.0f,
                         worldY + ty * outer + std::cos(t * 9.0f + i) * collapse * 8.0f,
                         2.0f + collapse * 2.4f, cr, cg, cb, 0.18f + collapse * 0.20f);
            }
        }

        drawRotRect(worldX, worldY, CORE_R * 1.10f * sx, CORE_R * 0.92f * sy,
                    coreAng * -0.65f, 0.04f, 0.025f, 0.06f, 0.9f);
        drawRotRect(worldX, worldY, CORE_R * (0.48f + flash * 0.26f), CORE_R * 0.44f,
                    coreAng * 1.4f, 1.0f, 0.92f, 1.0f, 0.72f + pulse * 0.18f);

        wchar_t tag[32];
        if (phase3) swprintf_s(tag, L"P3 SINGULARITY BREAK");
        else if (phase2) swprintf_s(tag, L"P2 GRAVITY BREATH");
        else swprintf_s(tag, L"P1 GRAVITY WAKE");
        float tw = g_TextS.Width(tag, 0.48f);
        g_TextS.Draw(tag, worldX - tw * 0.5f, worldY - CORE_R - 38.0f,
                     0.48f, cr, cg, cb, 0.9f);
    }

    const wchar_t* stateTag() const {
        if (phase3) return L"SINGULARITY BREAK";
        if (phase2) return L"GRAVITY BREATH";
        return L"GRAVITY WAKE";
    }
};
