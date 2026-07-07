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

extern TextRenderer g_TextS;

// FLAGSHIP.sys — 추상 와이어 코어 + 플레이어 주변 고속 스웜 + 십자 포격
class TotemBoss {
public:
    struct Interceptor {
        float x = 0.0f, y = 0.0f;
        float vx = 0.0f, vy = 0.0f;
        float burstAng = 0.0f;
        float dashCd = 0.0f;
        float hp = 0.0f, maxHp = 0.0f;
        bool  alive = false;
        float shootCd = 0.0f;
        float hitFlash = 0.0f;
        float aimAng = 0.0f;
    };

    struct StrikeMark {
        float x = 0.0f, y = 0.0f;
        float age = 0.0f;
        float rot = 0.0f;
        float size = 28.0f;
        float blastR = 64.0f;
        float blastDmg = 16.0f;
        bool  done = false;
    };

    float worldX, worldY;
    float hp, maxHp;
    bool  alive = true;
    bool  exploded = false;
    int   screenW, screenH;
    float facing = 0.0f;

    std::vector<Interceptor> ints;
    std::vector<StrikeMark> strikes;

    float moveSpeed = 52.0f;
    float preferDist = 300.0f;
    float hullFlash = 0.0f;

    float strikeCd = 2.4f;
    float bigStrikeCd = 7.5f;
    float intSpawnCd = 0.35f;

    enum class YamPhase { Idle, Charge, Fire };
    YamPhase yamPhase = YamPhase::Idle;
    float yamCd = 6.0f;
    float yamTimer = 0.0f;
    float yamAim = 0.0f;

    static constexpr float BODY              = 72.0f;
    static constexpr float INT_HIT           = 14.0f;
    static constexpr float MAP_PAD           = 96.0f;
    static constexpr float INT_HP_RATIO      = 0.05f;
    static constexpr float INT_SPAWN_INT     = 1.0f;
    static constexpr int   MAX_INT_ALIVE     = 26;
    static constexpr float YAMATO_CHARGE     = 0.42f;
    static constexpr float STRIKE_HOLD       = 0.12f;
    static constexpr float STRIKE_SHRINK_DUR = 0.20f;
    static constexpr float INT_BURST_SPEED   = 920.0f;
    static constexpr float INT_MAX_SPEED     = 1050.0f;

    float intHpMax() const {
        return (maxHp > 0.0f) ? maxHp * INT_HP_RATIO : 1.0f;
    }

    bool vulnerable() const { return false; }
    float vulnTimer = 0.0f;
    int wave = 1;
    bool isSkillSealed(int) const { return false; }
    float statDamageMult() const { return 1.0f; }
    int aliveTotems() const { return aliveInterceptors(); }
    void onTotemDamaged(int) {}
    void onTotemKilled(int) {}
    void damageExpandShield(float) {}
    void renderOrbitGuide(float) const {}
    void renderRiteWeb(float) const {}
    void renderLinks(float) const {}
    void renderLaser() const {}
    void repositionBays() {}

    TotemBoss(int sw, int sh, float hpInit) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.5f;
        worldY = sh * 0.36f;
        ints.reserve(MAX_INT_ALIVE);
    }

    int aliveInterceptors() const {
        int n = 0;
        for (const auto& ic : ints) if (ic.alive) ++n;
        return n;
    }

    void clampPos() {
        float m = MAP_PAD;
        if (worldX < m) worldX = m;
        if (worldX > screenW - m) worldX = screenW - m;
        if (worldY < m) worldY = m;
        if (worldY > screenH - m) worldY = screenH - m;
    }

    void localToWorld(float lx, float ly, float& wx, float& wy) const {
        float c = cosf(facing), s = sinf(facing);
        wx = worldX + lx * c - ly * s;
        wy = worldY + lx * s + ly * c;
    }

    static void drawWireSeg(float x0, float y0, float x1, float y1, float thick,
                            float r, float g, float b, float a) {
        float dx = x1 - x0, dy = y1 - y0;
        float len = sqrtf(dx * dx + dy * dy);
        if (len < 0.5f) return;
        float nx = -dy / len * thick * 0.5f;
        float ny =  dx / len * thick * 0.5f;
        BatchTri(x0 + nx, y0 + ny, x1 + nx, y1 + ny, x1 - nx, y1 - ny, r, g, b, a);
        BatchTri(x0 + nx, y0 + ny, x1 - nx, y1 - ny, x0 - nx, y0 - ny, r, g, b, a);
    }

    static void drawWireCross(float cx, float cy, float halfLen, float rot,
                              float thick, float r, float g, float b, float a) {
        float c = cosf(rot), s = sinf(rot);
        float dx = c * halfLen, dy = s * halfLen;
        float px = -s * halfLen, py = c * halfLen;
        drawWireSeg(cx - dx, cy - dy, cx + dx, cy + dy, thick, r, g, b, a);
        drawWireSeg(cx - px, cy - py, cx + px, cy + py, thick, r, g, b, a);
    }

    void fireDir(std::vector<Bullet>& b, float ox, float oy, float dx, float dy,
                 float sp, glm::vec3 col, float sz = 1.0f) {
        Bullet bb(ox, oy, ox + dx * 100.0f, oy + dy * 100.0f);
        bb.isEnemy = true; bb.speed = sp; bb.color = col; bb.sizeScale = sz;
        b.push_back(bb);
    }

    void burstFromPlayer(Interceptor& ic, float px, float py, bool randomRadial) {
        if (randomRadial)
            ic.burstAng = (float)(rand() % 628) * 0.01f;
        else
            ic.burstAng = atan2f(ic.y - py, ic.x - px);
        float spd = INT_BURST_SPEED + (float)(rand() % 320);
        ic.x = px + cosf(ic.burstAng) * (6.0f + (float)(rand() % 18));
        ic.y = py + sinf(ic.burstAng) * (6.0f + (float)(rand() % 18));
        ic.vx = cosf(ic.burstAng) * spd;
        ic.vy = sinf(ic.burstAng) * spd;
        ic.dashCd = 0.18f + (float)(rand() % 35) * 0.01f;
        ic.aimAng = atan2f(ic.vy, ic.vx);
    }

    void initInterceptor(Interceptor& ic, float px, float py) {
        ic.alive = true;
        ic.maxHp = ic.hp = intHpMax();
        ic.shootCd = 0.2f + (float)(rand() % 30) * 0.01f;
        ic.hitFlash = 0.0f;
        burstFromPlayer(ic, px, py, true);
    }

    bool spawnInterceptor(float px, float py) {
        for (auto& ic : ints) {
            if (!ic.alive) {
                initInterceptor(ic, px, py);
                return true;
            }
        }
        if ((int)ints.size() >= MAX_INT_ALIVE) return false;
        ints.emplace_back();
        initInterceptor(ints.back(), px, py);
        return true;
    }

    void updateMovement(float px, float py, float dt) {
        float dx = px - worldX, dy = py - worldY;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist > 4.0f) {
            float nx = dx / dist, ny = dy / dist;
            facing = atan2f(dy, dx);
            if (dist > preferDist + 50.0f) {
                worldX += nx * moveSpeed * dt;
                worldY += ny * moveSpeed * dt;
            } else if (dist < preferDist - 50.0f) {
                worldX -= nx * moveSpeed * 0.65f * dt;
                worldY -= ny * moveSpeed * 0.65f * dt;
            } else {
                worldX += -ny * moveSpeed * 0.5f * dt;
                worldY += nx * moveSpeed * 0.5f * dt;
            }
        }
        clampPos();
    }

    void updateInterceptors(float px, float py, float dt, std::vector<Bullet>& bullets) {
        for (auto& ic : ints) {
            if (!ic.alive) continue;
            if (ic.hitFlash > 0.0f) ic.hitFlash -= dt;

            ic.dashCd -= dt;
            if (ic.dashCd <= 0.0f)
                burstFromPlayer(ic, px, py, true);

            ic.x += ic.vx * dt;
            ic.y += ic.vy * dt;
            ic.vx *= 0.994f;
            ic.vy *= 0.994f;
            float spd = sqrtf(ic.vx * ic.vx + ic.vy * ic.vy);
            if (spd > INT_MAX_SPEED) {
                ic.vx *= INT_MAX_SPEED / spd;
                ic.vy *= INT_MAX_SPEED / spd;
            }
            if (spd > 8.0f) ic.aimAng = atan2f(ic.vy, ic.vx);

            ic.shootCd -= dt;
            if (ic.shootCd <= 0.0f) {
                burstFromPlayer(ic, px, py, false);
                float bx = px - ic.x, by = py - ic.y;
                float bd = sqrtf(bx * bx + by * by);
                if (bd > 1.0f) {
                    ic.aimAng = atan2f(by, bx);
                    fireDir(bullets, ic.x, ic.y, bx / bd, by / bd,
                            340.0f + (float)(rand() % 80),
                            glm::vec3(0.55f, 0.95f, 1.0f), 0.75f);
                }
                ic.shootCd = 0.55f + (float)(rand() % 40) * 0.01f;
            }
        }
    }

    void addStrike(float x, float y, float blastR, float dmg) {
        StrikeMark s;
        s.x = x; s.y = y;
        s.age = 0.0f;
        s.rot = (float)(rand() % 628) * 0.01f;
        s.size = 26.0f + (float)(rand() % 12);
        s.blastR = blastR;
        s.blastDmg = dmg;
        s.done = false;
        strikes.push_back(s);
    }

    void explodeStrike(StrikeMark& s, float px, float py,
                       float& playerHP, std::vector<Bullet>& bullets) {
        float dx = px - s.x, dy = py - s.y;
        if (dx * dx + dy * dy < s.blastR * s.blastR)
            HurtPlayer(playerHP, s.blastDmg);
        for (int i = 0; i < 10; i++) {
            float a = (float)i * 0.628f + s.rot;
            fireDir(bullets, s.x, s.y, cosf(a), sinf(a),
                    240.0f + (float)(rand() % 50),
                    glm::vec3(1.0f, 0.82f, 0.18f), 0.85f);
        }
    }

    void updateStrikes(float px, float py, float dt,
                       float& playerHP, std::vector<Bullet>& bullets) {
        for (auto& s : strikes) {
            if (s.done) continue;
            s.age += dt;
            if (s.age < STRIKE_HOLD) {
                s.size = 26.0f + (float)(rand() % 8) * 0.0f;
            } else if (s.age < STRIKE_HOLD + STRIKE_SHRINK_DUR) {
                float u = (s.age - STRIKE_HOLD) / STRIKE_SHRINK_DUR;
                s.rot += dt * (5.0f + u * 9.0f);
                s.size = 28.0f * (1.0f - u);
            } else if (!s.done) {
                s.done = true;
                explodeStrike(s, px, py, playerHP, bullets);
            }
        }
        strikes.erase(std::remove_if(strikes.begin(), strikes.end(),
            [](const StrikeMark& s) { return s.done && s.age > STRIKE_HOLD + STRIKE_SHRINK_DUR + 0.5f; }),
            strikes.end());
    }

    void startYamato(float px, float py) {
        yamPhase = YamPhase::Charge;
        yamTimer = YAMATO_CHARGE;
        yamAim = atan2f(py - worldY, px - worldX);
    }

    void fireYamato(std::vector<Bullet>& bullets) {
        float dx = cosf(yamAim), dy = sinf(yamAim);
        float ox, oy;
        localToWorld(58.0f, 0.0f, ox, oy);
        Bullet bb(ox, oy, ox + dx * 200.0f, oy + dy * 200.0f);
        bb.isEnemy = true;
        bb.speed = 720.0f;
        bb.sizeScale = 2.1f;
        bb.color = glm::vec3(1.0f, 0.32f, 0.12f);
        bb.maxRange = 300.0f;
        bb.shellKaboom = true;
        bb.shellRadius = 82.0f;
        bb.shellDmg = 30.0f;
        bullets.push_back(bb);
    }

    void processShellBlasts(float px, float py, float& playerHP,
                            std::vector<Bullet>& bullets) {
        for (auto& b : bullets) {
            if (b.active || !b.isEnemy || !b.shellKaboom || b.shellHandled) continue;
            if (b.maxRange > 0.0f && b.traveled < b.maxRange * 0.92f) continue;
            b.shellHandled = true;
            float dx = px - b.x, dy = py - b.y;
            if (dx * dx + dy * dy < b.shellRadius * b.shellRadius)
                HurtPlayer(playerHP, b.shellDmg);
            for (int i = 0; i < 12; i++) {
                float a = (float)i * 0.524f;
                fireDir(bullets, b.x, b.y, cosf(a), sinf(a),
                        200.0f, glm::vec3(1.0f, 0.4f, 0.12f), 0.7f);
            }
        }
    }

    void onIntDamaged(int idx) {
        if (idx < 0 || idx >= (int)ints.size()) return;
        if (!ints[idx].alive) return;
        ints[idx].hitFlash = 0.14f;
    }

    void onIntKilled(int idx) {
        if (idx < 0 || idx >= (int)ints.size()) return;
        ints[idx].alive = false;
        ints[idx].hp = 0.0f;
    }

    void Update(float px, float py, float dt, float& playerHP,
                std::vector<Bullet>& bullets,
                float& /*pullX*/, float& /*pullY*/) {
        if (!alive) return;

        if (hullFlash > 0.0f) hullFlash -= dt;

        updateMovement(px, py, dt);
        updateInterceptors(px, py, dt, bullets);
        updateStrikes(px, py, dt, playerHP, bullets);
        processShellBlasts(px, py, playerHP, bullets);

        intSpawnCd -= dt;
        if (intSpawnCd <= 0.0f) {
            spawnInterceptor(px, py);
            intSpawnCd = INT_SPAWN_INT;
        }

        float ddx = px - worldX, ddy = py - worldY;
        if (ddx * ddx + ddy * ddy < (BODY * 0.85f) * (BODY * 0.85f))
            HurtPlayer(playerHP, 14.0f * dt);

        strikeCd -= dt;
        if (strikeCd <= 0.0f) {
            float ox = px + (float)(rand() % 160 - 80);
            float oy = py + (float)(rand() % 160 - 80);
            addStrike(ox, oy, 58.0f, 14.0f);
            strikeCd = 2.0f + (float)(rand() % 30) * 0.04f;
        }

        bigStrikeCd -= dt;
        if (bigStrikeCd <= 0.0f) {
            addStrike(px, py, 78.0f, 22.0f);
            bigStrikeCd = 7.0f + (float)(rand() % 40) * 0.05f;
        }

        if (yamPhase == YamPhase::Idle) {
            yamCd -= dt;
            if (yamCd <= 0.0f) startYamato(px, py);
        } else if (yamPhase == YamPhase::Charge) {
            yamTimer -= dt;
            if (yamTimer <= 0.0f) {
                fireYamato(bullets);
                yamPhase = YamPhase::Idle;
                yamCd = 10.0f + (float)(rand() % 50) * 0.06f;
            }
        }
    }

    void renderWireRing(float cx, float cy, float rad, int seg, float thick,
                        float r, float g, float b, float a, float spin) const {
        float prevX = cx + cosf(spin) * rad;
        float prevY = cy + sinf(spin) * rad;
        for (int i = 1; i <= seg; i++) {
            float ang = spin + (float)i / (float)seg * 6.283f;
            float x = cx + cosf(ang) * rad;
            float y = cy + sinf(ang) * rad;
            drawWireSeg(prevX, prevY, x, y, thick, r, g, b, a);
            prevX = x; prevY = y;
        }
    }

    void renderHull(float t) const {
        float pulse = 0.5f + 0.5f * sinf(t * 3.0f);
        float flash = (hullFlash > 0.0f) ? hullFlash / 0.18f : 0.0f;
        float cr = 0.35f + flash * 0.35f;
        float cg = 0.82f + flash * 0.1f;
        float cb = 1.0f;

        renderWireRing(worldX, worldY, 46.0f + pulse * 5.0f, 16, 2.0f,
                       cr, cg, cb, 0.9f, t * 0.35f);
        renderWireRing(worldX, worldY, 28.0f, 10, 1.6f,
                       cr * 0.7f, cg * 0.7f, cb * 0.7f, 0.75f, -t * 0.55f);

        for (int i = 0; i < 8; i++) {
            float a = (float)i * 0.785f + t * 0.25f;
            float x1 = worldX + cosf(a) * 22.0f;
            float y1 = worldY + sinf(a) * 22.0f;
            float x2 = worldX + cosf(a) * (58.0f + pulse * 8.0f);
            float y2 = worldY + sinf(a) * (58.0f + pulse * 8.0f);
            drawWireSeg(x1, y1, x2, y2, 1.5f, cr * 0.6f, cg * 0.6f, cb * 0.6f, 0.7f);
        }

        float hx[6], hy[6];
        for (int i = 0; i < 6; i++) {
            float a = (float)i * 1.047f + t * 0.15f;
            hx[i] = worldX + cosf(a) * 18.0f;
            hy[i] = worldY + sinf(a) * 18.0f;
        }
        for (int i = 0; i < 6; i++) {
            int j = (i + 1) % 6;
            drawWireSeg(hx[i], hy[i], hx[j], hy[j], 1.8f, cr, cg, cb, 0.85f);
        }
        drawWireSeg(worldX - 10.0f, worldY, worldX + 10.0f, worldY, 1.4f, cr, cg, cb, 0.65f);
        drawWireSeg(worldX, worldY - 10.0f, worldX, worldY + 10.0f, 1.4f, cr, cg, cb, 0.65f);
    }

    static void drawCursorShard(float cx, float cy, float size, float ang,
                                float r, float g, float b, float a) {
        static const float lx[] = { 0.0f,  0.62f,  0.34f,  0.0f, -0.34f, -0.62f };
        static const float ly[] = { -1.0f, -0.18f, 0.62f, 0.38f, 0.62f, -0.18f };
        float wx[6], wy[6];
        float c = cosf(ang), s = sinf(ang);
        for (int i = 0; i < 6; i++) {
            float px = lx[i] * size, py = ly[i] * size;
            wx[i] = cx + px * c - py * s;
            wy[i] = cy + px * s + py * c;
        }
        for (int i = 0; i < 6; i++) {
            int j = (i + 1) % 6;
            drawWireSeg(wx[i], wy[i], wx[j], wy[j], 1.3f, r, g, b, a);
        }
    }

    static bool inWinPt(float px, float py, float wx, float wy, float ww, float wh) {
        return px >= wx && px <= wx + ww && py >= wy && py <= wy + wh;
    }

    void renderInterceptorsInWin(float t, float wx, float wy, float ww, float wh) const {
        for (const auto& ic : ints) {
            if (!ic.alive) continue;
            if (!inWinPt(ic.x, ic.y, wx, wy, ww, wh)) continue;
            float flash = (ic.hitFlash > 0.0f) ? ic.hitFlash / 0.14f : 0.0f;
            float pulse = 0.5f + 0.5f * sinf(t * 9.0f + ic.burstAng);
            float sz = 9.0f + pulse * 2.0f;
            drawCursorShard(ic.x, ic.y, sz, ic.aimAng,
                            0.35f + flash * 0.45f, 0.88f + flash * 0.1f, 1.0f, 0.92f);
        }
        (void)t;
    }

    void renderStrikesInWin(float wx, float wy, float ww, float wh) const {
        for (const auto& s : strikes) {
            if (s.done) continue;
            if (s.age >= STRIKE_HOLD + STRIKE_SHRINK_DUR) continue;
            if (!inWinPt(s.x, s.y, wx, wy, ww, wh)) continue;
            float a = (s.age < STRIKE_HOLD) ? 0.95f : (1.0f - (s.age - STRIKE_HOLD) / STRIKE_SHRINK_DUR);
            drawWireCross(s.x, s.y, s.size, s.rot, 2.2f, 1.0f, 0.88f, 0.22f, 0.55f * a);
        }
    }

    void renderTelegraphs(float t) const {
        if (yamPhase == YamPhase::Charge) {
            float prog = 1.0f - yamTimer / YAMATO_CHARGE;
            if (prog < 0.0f) prog = 0.0f;
            if (prog > 1.0f) prog = 1.0f;
            float ox, oy;
            localToWorld(52.0f, 0.0f, ox, oy);
            float dx = cosf(yamAim), dy = sinf(yamAim);
            float len = 120.0f + prog * 280.0f;
            drawWireSeg(ox, oy, ox + dx * len, oy + dy * len,
                        2.0f + prog * 2.0f, 1.0f, 0.28f + prog * 0.2f, 0.1f, 0.35f + prog * 0.5f);
            g_TextS.Draw(L"YAMATO", ox - 28.0f, oy - 36.0f, 0.48f,
                         1.0f, 0.35f, 0.12f, 0.45f + prog * 0.55f);
        }
        (void)t;
    }

    void renderCore(float t) const {
        renderHull(t);
        renderTelegraphs(t);
    }

    float yamatoDisplayCd() const {
        if (yamPhase == YamPhase::Charge) return yamTimer;
        return yamCd;
    }

    const wchar_t* yamatoLabel() const {
        return (yamPhase == YamPhase::Charge) ? L"YAMATO" : L"YAMATO CD";
    }

    float swarmSpawnCd() const { return intSpawnCd; }
};
