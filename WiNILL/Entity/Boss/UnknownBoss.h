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
#include "CollisionSystem.h"

extern TextRenderer g_TextS;

static inline void ubDrawThickLine(float x1, float y1, float x2, float y2, float thick,
                                   float r, float g, float b, float a) {
    float dx = x2 - x1, dy = y2 - y1;
    float len = sqrtf(dx * dx + dy * dy) + 1e-6f;
    float px = -dy / len * thick * 0.5f;
    float py =  dx / len * thick * 0.5f;
    BatchTri(x1 + px, y1 + py, x2 + px, y2 + py, x2 - px, y2 - py, r, g, b, a);
    BatchTri(x1 + px, y1 + py, x2 - px, y2 - py, x1 - px, y1 - py, r, g, b, a);
}

// ─────────────────────────────────────────────────────────────
// UNKNOWN.sys — 창연(窗緣) 검 보스
//   검 투척 → 가짜창 가장자리 pin (영구) → 전부 소진 시 2초 전조 → 회수 경로 피격
// ─────────────────────────────────────────────────────────────

enum class UBEdge { TOP, RIGHT, BOTTOM, LEFT };

enum class UBState {
    MOVE, WINDUP, FLYING, RECALL_TEL, RECALLING, PAUSE
};

enum class UBSkill {
    EDGE, CHAIN, CROSS, SKEW
};

struct UBPin {
    float wx, wy, ww, wh;
    UBEdge edge;
    float along;
    float pinX, pinY;
    bool  ownedWin;
    float scar;          // 0=검 박힘, 1=회수 후 흔적만
};

struct UBFly {
    float x, y, tx, ty;
    float t;
    bool  active;
    int   pinSlot;
};

struct UBRecall {
    float x, y, vx, vy;
    float fromX, fromY;
    bool  active;
};

struct UBExtraWin {
    float x, y, w, h;
};

class UnknownBoss {
public:
    static constexpr const wchar_t* BOSS_NAME = L"UNKNOWN.sys";

    float worldX, worldY;
    float hp, maxHp;
    bool  alive = true;
    bool  exploded = false;
    int   screenW, screenH;

    int   quiver = 3;
    int   maxQuiver = 3;
    bool  phase2 = false;
    bool  phase3 = false;

    UBState state = UBState::MOVE;
    UBSkill nextSkill = UBSkill::EDGE;
    float stateT = 0.0f;
    float actionCd = 1.2f;
    float moveSpeed = 175.0f;
    float orbitT = 0.0f;
    float sheathCd = 0.0f;
    float shakePulse = 0.0f;

    int   chainLeft = 0;
    int   crossLeft = 0;
    float throwTargetWx = 0, throwTargetWy = 0, throwTargetWw = 0, throwTargetWh = 0;
    UBEdge throwEdge = UBEdge::TOP;
    float throwAlong = 0.5f;
    bool  throwOwnedWin = false;

    std::vector<UBPin>   pins;
    std::vector<UBFly>   flies;
    std::vector<UBRecall> recalls;
    std::vector<UBExtraWin> extraWins;

    static constexpr float BODY = 44.0f;
    static constexpr float WIN_W = 420.0f;
    static constexpr float WIN_H = 520.0f;
    static constexpr float RECALL_TEL = 2.0f;
    static constexpr float EDGE_HAZ = 11.0f;
    static constexpr int   MAX_PINS = 14;
    static constexpr int   MAX_EXTRA = 4;
    static constexpr float FLY_SPD = 920.0f;
    static constexpr float RECALL_SPD = 780.0f;
    static constexpr float RECALL_DMG = 14.0f;
    static constexpr float EDGE_DPS = 9.0f;

    UnknownBoss(int sw, int sh, float hpInit)
        : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.5f;
        worldY = sh * 0.30f;
        refreshQuiver();
    }

    bool vulnerable() const {
        return state == UBState::RECALL_TEL || state == UBState::RECALLING;
    }

    float damageTakenMult() const {
        if (state == UBState::RECALLING) return 1.22f;
        if (state == UBState::RECALL_TEL) return 1.12f;
        if (orbitT > 0.0f) return 0.55f;
        return 1.0f;
    }

    void refreshQuiver() {
        maxQuiver = phase3 ? 5 : (phase2 ? 4 : 3);
        if (quiver > maxQuiver) quiver = maxQuiver;
    }

    static void edgePoint(float wx, float wy, float ww, float wh,
                          UBEdge e, float along, float& ox, float& oy) {
        if (along < 0.05f) along = 0.05f;
        if (along > 0.95f) along = 0.95f;
        switch (e) {
        case UBEdge::TOP:    ox = wx + ww * along; oy = wy; break;
        case UBEdge::BOTTOM: ox = wx + ww * along; oy = wy + wh; break;
        case UBEdge::LEFT:   ox = wx; oy = wy + wh * along; break;
        case UBEdge::RIGHT:  ox = wx + ww; oy = wy + wh * along; break;
        }
    }

    static UBEdge pickEdge(float px, float py, float wx, float wy, float ww, float wh) {
        float cx = wx + ww * 0.5f, cy = wy + wh * 0.5f;
        float dx = px - cx, dy = py - cy;
        if (std::fabs(dx) > std::fabs(dy))
            return dx > 0 ? UBEdge::LEFT : UBEdge::RIGHT;
        return dy > 0 ? UBEdge::TOP : UBEdge::BOTTOM;
    }

    void clampPos() {
        float m = BODY + 24.0f;
        if (worldX < m) worldX = m;
        else if (worldX > screenW - m) worldX = screenW - m;
        if (worldY < m) worldY = m;
        else if (worldY > screenH - m) worldY = screenH - m;
    }

    bool pickTarget(float px, float py,
                    float pwx, float pwy, float pww, float pwh,
                    bool preferPlayer) {
        throwOwnedWin = false;
        if (preferPlayer || (rand() % 100) < 72) {
            throwTargetWx = pwx; throwTargetWy = pwy;
            throwTargetWw = pww; throwTargetWh = pwh;
            throwEdge = pickEdge(px, py, pwx, pwy, pww, pwh);
            throwAlong = 0.18f + (float)(rand() % 65) * 0.01f;
            return true;
        }
        if ((int)extraWins.size() < MAX_EXTRA && (rand() % 100) < 40) {
            float ew = 180.0f + (float)(rand() % 80);
            float eh = 120.0f + (float)(rand() % 50);
            float ex = 80.0f + (float)(rand() % (int)std::max(1.0f, screenW - ew - 160.0f));
            float ey = 80.0f + (float)(rand() % (int)std::max(1.0f, screenH - eh - 160.0f));
            extraWins.push_back({ ex, ey, ew, eh });
            throwTargetWx = ex; throwTargetWy = ey;
            throwTargetWw = ew; throwTargetWh = eh;
            throwEdge = (UBEdge)(rand() % 4);
            throwAlong = 0.2f + (float)(rand() % 60) * 0.01f;
            throwOwnedWin = true;
            return true;
        }
        throwTargetWx = pwx; throwTargetWy = pwy;
        throwTargetWw = pww; throwTargetWh = pwh;
        throwEdge = pickEdge(px, py, pwx, pwy, pww, pwh);
        throwAlong = 0.5f;
        return true;
    }

    void beginThrow() {
        float tx, ty;
        edgePoint(throwTargetWx, throwTargetWy, throwTargetWw, throwTargetWh,
                  throwEdge, throwAlong, tx, ty);
        UBFly f;
        f.x = worldX; f.y = worldY;
        f.tx = tx; f.ty = ty;
        f.t = 0.0f; f.active = true;
        f.pinSlot = (int)pins.size();
        flies.push_back(f);
        quiver--;
        state = UBState::FLYING;
        stateT = 0.0f;
    }

    void commitPin(const UBFly& f) {
        if ((int)pins.size() >= MAX_PINS) {
            pins.erase(pins.begin());
        }
        UBPin p;
        p.wx = throwTargetWx; p.wy = throwTargetWy;
        p.ww = throwTargetWw; p.wh = throwTargetWh;
        p.edge = throwEdge;
        p.along = throwAlong;
        p.pinX = f.tx; p.pinY = f.ty;
        p.ownedWin = throwOwnedWin;
        p.scar = 0.0f;
        pins.push_back(p);
    }

    void startRecallTelegraph() {
        state = UBState::RECALL_TEL;
        stateT = RECALL_TEL;
        shakePulse = 1.0f;
    }

    void launchRecalls() {
        recalls.clear();
        for (auto& p : pins) {
            if (p.scar > 0.5f) continue;
            UBRecall r;
            r.fromX = p.pinX; r.fromY = p.pinY;
            r.x = p.pinX; r.y = p.pinY;
            float dx = worldX - p.pinX, dy = worldY - p.pinY;
            float d = sqrtf(dx * dx + dy * dy) + 1e-3f;
            r.vx = dx / d * RECALL_SPD;
            r.vy = dy / d * RECALL_SPD;
            r.active = true;
            recalls.push_back(r);
            p.scar = 1.0f;
        }
        state = UBState::RECALLING;
        stateT = 0.0f;
    }

    void pickNextSkill() {
        if (phase3 && quiver >= 3 && (rand() % 100) < 35) {
            nextSkill = UBSkill::CHAIN;
            chainLeft = std::min(3, quiver);
            return;
        }
        if (phase2 && quiver >= 2 && (rand() % 100) < 40) {
            nextSkill = UBSkill::CROSS;
            crossLeft = 2;
            return;
        }
        if (phase2 && (int)extraWins.size() < MAX_EXTRA && (rand() % 100) < 28) {
            nextSkill = UBSkill::SKEW;
            return;
        }
        nextSkill = UBSkill::EDGE;
    }

    void sheathBurst(std::vector<Bullet>& bullets) {
        for (auto& p : pins) {
            for (int i = 0; i < 6; i++) {
                float a = (float)i / 6.0f * 6.2831853f + (float)(rand() % 100) * 0.01f;
                Bullet b(p.pinX, p.pinY,
                         p.pinX + cosf(a) * 80.0f, p.pinY + sinf(a) * 80.0f);
                b.isEnemy = true;
                b.speed = 340.0f + (float)(rand() % 60);
                b.color = glm::vec3(0.95f, 0.25f, 0.55f);
                b.enemyDmg = 8.0f;
                bullets.push_back(b);
            }
        }
        shakePulse = 1.0f;
    }

    void hurtEdge(float px, float py, float& playerHP, float dt) {
        for (auto& p : pins) {
            float strip = EDGE_HAZ;
            bool hit = false;
            switch (p.edge) {
            case UBEdge::TOP:
                hit = px >= p.wx && px <= p.wx + p.ww &&
                      py >= p.wy - strip && py <= p.wy + strip;
                break;
            case UBEdge::BOTTOM:
                hit = px >= p.wx && px <= p.wx + p.ww &&
                      py >= p.wy + p.wh - strip && py <= p.wy + p.wh + strip;
                break;
            case UBEdge::LEFT:
                hit = py >= p.wy && py <= p.wy + p.wh &&
                      px >= p.wx - strip && px <= p.wx + strip;
                break;
            case UBEdge::RIGHT:
                hit = py >= p.wy && py <= p.wy + p.wh &&
                      px >= p.wx + p.ww - strip && px <= p.wx + p.ww + strip;
                break;
            }
            if (hit) HurtPlayer(playerHP, EDGE_DPS * dt);
        }
    }

    void Update(float px, float py,
                float pwx, float pwy, float pww, float pwh,
                float dt, float& playerHP,
                std::vector<Bullet>& bullets) {
        if (!alive) return;

        if (!phase2 && hp <= maxHp * 0.55f) { phase2 = true; refreshQuiver(); }
        if (!phase3 && hp <= maxHp * 0.28f) { phase3 = true; refreshQuiver(); }

        float dx = px - worldX, dy = py - worldY;
        float dist = sqrtf(dx * dx + dy * dy) + 1e-3f;
        float nx = dx / dist, ny = dy / dist;

        if (dist < BODY + 8.0f)
            HurtPlayer(playerHP, (phase3 ? 11.0f : 9.0f) * dt);

        hurtEdge(px, py, playerHP, dt);

        if (orbitT > 0.0f) orbitT -= dt;
        if (dist < 130.0f && quiver > 0 && orbitT <= 0.0f) {
            orbitT = 2.2f;
        }

        sheathCd -= dt;
        if (phase2 && sheathCd <= 0.0f && hp < maxHp * 0.72f) {
            sheathCd = 18.0f;
            sheathBurst(bullets);
        }

        actionCd -= dt;

        switch (state) {
        case UBState::MOVE:
            if (dist > 340.0f) {
                worldX += nx * moveSpeed * dt;
                worldY += ny * moveSpeed * dt;
            } else if (dist < 200.0f) {
                worldX -= nx * moveSpeed * 0.85f * dt;
                worldY -= ny * moveSpeed * 0.85f * dt;
            } else {
                worldX += -ny * moveSpeed * 0.45f * dt;
                worldY += nx * moveSpeed * 0.45f * dt;
            }
            clampPos();

            if (quiver <= 0 && flies.empty()) {
                bool anyLivePin = false;
                for (auto& p : pins) if (p.scar < 0.5f) { anyLivePin = true; break; }
                if (anyLivePin) startRecallTelegraph();
                else {
                    quiver = maxQuiver;
                    actionCd = 0.8f;
                }
                break;
            }

            if (quiver > 0 && actionCd <= 0.0f) {
                pickNextSkill();
                if (nextSkill == UBSkill::SKEW)
                    pickTarget(px, py, pwx, pwy, pww, pwh, false);
                else if (nextSkill == UBSkill::CROSS)
                    pickTarget(px, py, pwx, pwy, pww, pwh, true);
                else
                    pickTarget(px, py, pwx, pwy, pww, pwh, true);
                state = UBState::WINDUP;
                stateT = 0.42f;
            }
            break;

        case UBState::WINDUP:
            stateT -= dt;
            if (stateT <= 0.0f) beginThrow();
            break;

        case UBState::FLYING: {
            stateT += dt;
            bool anyFly = false;
            for (auto& f : flies) {
                if (!f.active) continue;
                anyFly = true;
                float fdx = f.tx - f.x, fdy = f.ty - f.y;
                float fl = sqrtf(fdx * fdx + fdy * fdy);
                float step = FLY_SPD * dt;
                if (fl <= step + 2.0f) {
                    f.x = f.tx; f.y = f.ty;
                    f.active = false;
                    commitPin(f);
                    shakePulse = 0.6f;
                } else {
                    f.x += fdx / fl * step;
                    f.y += fdy / fl * step;
                }
            }
            if (!anyFly) {
                flies.erase(std::remove_if(flies.begin(), flies.end(),
                    [](const UBFly& f) { return !f.active; }), flies.end());
                if (chainLeft > 1) {
                    chainLeft--;
                    actionCd = 0.22f;
                    state = UBState::MOVE;
                } else if (crossLeft > 1) {
                    crossLeft--;
                    if (throwEdge == UBEdge::TOP) throwEdge = UBEdge::BOTTOM;
                    else if (throwEdge == UBEdge::BOTTOM) throwEdge = UBEdge::TOP;
                    else if (throwEdge == UBEdge::LEFT) throwEdge = UBEdge::RIGHT;
                    else throwEdge = UBEdge::LEFT;
                    state = UBState::WINDUP;
                    stateT = 0.32f;
                } else {
                    state = UBState::PAUSE;
                    stateT = 0.35f;
                }
            }
            break;
        }

        case UBState::PAUSE:
            stateT -= dt;
            if (stateT <= 0.0f) {
                state = UBState::MOVE;
                actionCd = phase3 ? 0.55f : (phase2 ? 0.72f : 0.95f);
            }
            break;

        case UBState::RECALL_TEL:
            stateT -= dt;
            if (stateT <= 0.0f) launchRecalls();
            break;

        case UBState::RECALLING: {
            stateT += dt;
            int activeN = 0;
            for (auto& r : recalls) {
                if (!r.active) continue;
                activeN++;
                float ox = r.x, oy = r.y;
                r.x += r.vx * dt;
                r.y += r.vy * dt;
                if (SegDist(px, py, ox, oy, r.x, r.y) < 16.0f)
                    HurtPlayer(playerHP, RECALL_DMG * dt * 8.0f);
                float bdx = worldX - r.x, bdy = worldY - r.y;
                if (bdx * bdx + bdy * bdy < BODY * BODY) {
                    r.active = false;
                }
            }
            if (activeN == 0) {
                recalls.clear();
                quiver = maxQuiver;
                state = UBState::MOVE;
                actionCd = 1.0f;
            } else if (phase2 && stateT > 0.45f) {
                // Stagger: phase2+ recalls don't all need to finish same frame
            }
            break;
        }
        }

        // Recall blades vs player point
        for (auto& r : recalls) {
            if (!r.active) continue;
            if (SegDist(px, py, r.x - r.vx * dt, r.y - r.vy * dt, r.x, r.y) < 14.0f)
                HurtPlayer(playerHP, RECALL_DMG);
        }
    }

    void renderBody(float gt, float aimX, float aimY) const {
        float cx = WIN_W * 0.5f, cy = WIN_H * 0.52f;
        drawRect(cx - 28.0f, cy - 36.0f, 56.0f, 72.0f, 0.92f, 0.92f, 0.95f, 0.95f);
        drawRect(cx - 10.0f, cy - 8.0f, 20.0f, 20.0f, 0.95f, 0.22f, 0.58f, 1.0f);
        g_TextS.Draw(L"?", cx - 6.0f, cy - 10.0f, 1.1f, 1.0f, 1.0f, 1.0f, 1.0f);

        int show = quiver;
        for (int i = 0; i < show; i++) {
            float ang = gt * 1.8f + (float)i * (6.2831853f / (float)std::max(1, maxQuiver));
            float ox = cx + cosf(ang) * 72.0f;
            float oy = cy + sinf(ang) * 52.0f;
            float bladeAng = ang + 1.57f;
            float ex = ox + cosf(bladeAng) * 28.0f;
            float ey = oy + sinf(bladeAng) * 28.0f;
            ubDrawThickLine(ox, oy, ex, ey, 3.0f, 0.95f, 0.30f, 0.62f, 0.95f);
        }

        if (orbitT > 0.0f) {
            float pulse = 0.5f + 0.5f * sinf(gt * 12.0f);
            drawCircle(cx, cy, 88.0f, 0.95f, 0.35f, 0.65f, 0.12f * pulse);
        }

        wchar_t tag[32];
        swprintf_s(tag, L"blades: %d", quiver);
        g_TextS.Draw(tag, 14.0f, WIN_H - 36.0f, 0.55f, 0.95f, 0.45f, 0.70f, 0.9f);
    }

    void renderPinOnWindow(float wx, float wy, float ww, float wh, float gt) const {
        for (auto& p : pins) {
            if (std::fabs(p.wx - wx) > 1.0f || std::fabs(p.wy - wy) > 1.0f ||
                std::fabs(p.ww - ww) > 1.0f || std::fabs(p.wh - wh) > 1.0f)
                continue;
            float ox, oy;
            edgePoint(p.wx, p.wy, p.ww, p.wh, p.edge, p.along, ox, oy);
            float lx = ox - wx, ly = oy - wy;
            float alpha = (p.scar > 0.5f) ? 0.35f : 1.0f;
            float inset = 18.0f;
            float ex = lx, ey = ly;
            switch (p.edge) {
            case UBEdge::TOP:    ey = ly + inset; break;
            case UBEdge::BOTTOM: ey = ly - inset; break;
            case UBEdge::LEFT:   ex = lx + inset; break;
            case UBEdge::RIGHT:  ex = lx - inset; break;
            }
            ubDrawThickLine(lx, ly, ex, ey, 3.5f, 0.95f, 0.28f, 0.60f, alpha);
            drawCircle(lx, ly, 6.0f, 0.95f, 0.40f, 0.70f, alpha * 0.85f);

            float strip = EDGE_HAZ;
            switch (p.edge) {
            case UBEdge::TOP:
                drawRect(lx - 20.0f, ly - strip, 40.0f, strip * 2.0f,
                         0.95f, 0.20f, 0.50f, 0.10f * alpha);
                break;
            case UBEdge::BOTTOM:
                drawRect(lx - 20.0f, ly - strip, 40.0f, strip * 2.0f,
                         0.95f, 0.20f, 0.50f, 0.10f * alpha);
                break;
            case UBEdge::LEFT:
                drawRect(lx - strip, ly - 20.0f, strip * 2.0f, 40.0f,
                         0.95f, 0.20f, 0.50f, 0.10f * alpha);
                break;
            case UBEdge::RIGHT:
                drawRect(lx - strip, ly - 20.0f, strip * 2.0f, 40.0f,
                         0.95f, 0.20f, 0.50f, 0.10f * alpha);
                break;
            }
        }
    }

    void renderWorld(float gt, float pwx, float pwy, float pww, float pwh) const {
        for (auto& f : flies) {
            if (!f.active) continue;
            ubDrawThickLine(f.x, f.y, f.tx, f.ty, 2.0f, 0.95f, 0.35f, 0.55f, 0.35f);
            float ang = atan2f(f.ty - f.y, f.tx - f.x);
            float ex = f.x + cosf(ang) * 22.0f;
            float ey = f.y + sinf(ang) * 22.0f;
            ubDrawThickLine(f.x, f.y, ex, ey, 4.0f, 0.95f, 0.30f, 0.65f, 0.95f);
        }

        if (state == UBState::RECALL_TEL) {
            float blink = 0.45f + 0.45f * (0.5f + 0.5f * sinf(gt * 14.0f));
            float prog = 1.0f - stateT / RECALL_TEL;
            for (auto& p : pins) {
                if (p.scar > 0.5f) continue;
                ubDrawThickLine(p.pinX, p.pinY, worldX, worldY, 2.0f,
                                1.0f, 0.25f, 0.45f, blink * (0.35f + 0.65f * prog));
            }
        }

        for (auto& r : recalls) {
            if (!r.active) continue;
            ubDrawThickLine(r.fromX, r.fromY, r.x, r.y, 2.5f, 1.0f, 0.35f, 0.55f, 0.75f);
            float ang = atan2f(r.vy, r.vx);
            ubDrawThickLine(r.x, r.y, r.x - cosf(ang) * 20.0f, r.y - sinf(ang) * 20.0f,
                            4.0f, 0.95f, 0.28f, 0.62f, 1.0f);
        }

        if (state == UBState::WINDUP) {
            float tx, ty;
            edgePoint(throwTargetWx, throwTargetWy, throwTargetWw, throwTargetWh,
                      throwEdge, throwAlong, tx, ty);
            float pulse = 0.5f + 0.5f * sinf(gt * 18.0f);
            drawCircle(tx, ty, 16.0f + pulse * 6.0f, 1.0f, 0.45f, 0.65f, 0.55f * pulse);
            ubDrawThickLine(worldX, worldY, tx, ty, 1.5f, 0.95f, 0.40f, 0.70f, 0.25f * pulse);
        }
    }

    static const wchar_t* stateTag(UBState s) {
        switch (s) {
        case UBState::WINDUP:      return L"AIM";
        case UBState::FLYING:      return L"THROW";
        case UBState::RECALL_TEL:  return L"RECALL…";
        case UBState::RECALLING:   return L"PULL";
        default:                   return L"EDGE";
        }
    }
};
