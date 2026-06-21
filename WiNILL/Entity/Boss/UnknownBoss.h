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

// UNKNOWN.sys — 창연(窗緣) 검 보스
//   연속 투척 → 게임창 끝 pin → blade.sys 미니창(비행/회수) → 2초 전조 → 회수

enum class UBEdge { TOP, RIGHT, BOTTOM, LEFT };
enum class UBState { MOVE, RECALL_TEL, RECALLING };
enum class UBSkill { EDGE, CHAIN, CROSS };

struct UBPin {
    float wx, wy, ww, wh;
    UBEdge edge;
    float along;
    float pinX, pinY;
    float scar;
};

struct UBSwordWin {
    float x, y, w, h;
    bool  active = false;
    bool  recall = false;
};

struct UBFly {
    float fx, fy;
    float x, y, tx, ty;
    float twx, twy, tww, twh;
    UBEdge edge;
    float along;
    bool  active;
    int   winIdx;
};

struct UBRecall {
    float x, y, vx, vy;
    float fromX, fromY;
    bool  active;
    int   winIdx;
};

class UnknownBoss {
public:
    static constexpr const wchar_t* BOSS_NAME  = L"UNKNOWN.sys";
    static constexpr const wchar_t* BLADE_NAME = L"blade.sys";

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
    float actionCd = 0.35f;
    float windupT = 0.0f;
    float moveSpeed = 155.0f;
    float orbitT = 0.0f;
    float shakePulse = 0.0f;
    float spinAng = 0.0f;

    int   chainLeft = 0;
    int   crossLeft = 0;
    float throwTargetWx = 0, throwTargetWy = 0, throwTargetWw = 0, throwTargetWh = 0;
    UBEdge throwEdge = UBEdge::TOP;
    float throwAlong = 0.5f;

    std::vector<UBPin>     pins;
    std::vector<UBFly>     flies;
    std::vector<UBRecall>  recalls;
    std::vector<UBSwordWin> bladeWins;

    static constexpr float BODY = 58.0f;
    static constexpr float WIN_W = 500.0f;
    static constexpr float WIN_H = 580.0f;
    static constexpr float BLADE_WIN_W = 132.0f;
    static constexpr float BLADE_WIN_H = 100.0f;
    static constexpr float RECALL_TEL = 1.85f;
    static constexpr float EDGE_HAZ = 14.0f;
    static constexpr int   MAX_PINS = 28;
    static constexpr int   MAX_FLY  = 14;
    static constexpr float FLY_SPD = 2400.0f;
    static constexpr float RECALL_SPD = 980.0f;
    static constexpr float RECALL_DMG = 15.0f;
    static constexpr float EDGE_DPS = 11.0f;
    static constexpr float THROW_CD_P1 = 0.045f;
    static constexpr float THROW_CD_P2 = 0.028f;
    static constexpr float THROW_CD_P3 = 0.016f;

    UnknownBoss(int sw, int sh, float hpInit)
        : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.5f;
        worldY = sh * 0.28f;
        refreshQuiver();
        quiver = maxQuiver;
    }

    bool vulnerable() const {
        return state == UBState::RECALL_TEL || state == UBState::RECALLING;
    }

    float damageTakenMult() const {
        if (state == UBState::RECALLING) return 1.24f;
        if (state == UBState::RECALL_TEL) return 1.14f;
        if (orbitT > 0.0f) return 0.68f;
        return 1.0f;
    }

    void refreshQuiver() {
        maxQuiver = phase3 ? 18 : (phase2 ? 14 : 10);
        if (quiver > maxQuiver) quiver = maxQuiver;
    }

    static bool windowMatch(float a, float b) { return std::fabs(a - b) < 3.0f; }

    static bool sameWindow(float wx, float wy, float ww, float wh,
                           float ox, float oy, float ow, float oh) {
        return windowMatch(wx, ox) && windowMatch(wy, oy) &&
               windowMatch(ww, ow) && windowMatch(wh, oh);
    }

    static void edgePoint(float wx, float wy, float ww, float wh,
                          UBEdge e, float along, float& ox, float& oy) {
        along = std::max(0.03f, std::min(0.97f, along));
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
        if (std::fabs(dx) * wh > std::fabs(dy) * ww)
            return dx > 0 ? UBEdge::LEFT : UBEdge::RIGHT;
        return dy > 0 ? UBEdge::TOP : UBEdge::BOTTOM;
    }

    static void edgeNormal(UBEdge e, float& nx, float& ny) {
        switch (e) {
        case UBEdge::TOP:    nx = 0.0f;  ny = -1.0f; break;
        case UBEdge::BOTTOM: nx = 0.0f;  ny =  1.0f; break;
        case UBEdge::LEFT:   nx = -1.0f; ny =  0.0f; break;
        case UBEdge::RIGHT:  nx =  1.0f; ny =  0.0f; break;
        }
    }

    static bool visibleInRect(float px, float py,
                              float wx, float wy, float ww, float wh, float pad = 80.0f) {
        return px >= wx - pad && px <= wx + ww + pad &&
               py >= wy - pad && py <= wy + wh + pad;
    }

    static bool rectOverlap(float ax, float ay, float aw, float ah,
                            float bx, float by, float bw, float bh) {
        return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
    }

    void clampPos() {
        float m = BODY + 20.0f;
        worldX = std::max(m, std::min(worldX, (float)screenW - m));
        worldY = std::max(m, std::min(worldY, (float)screenH - m));
    }

    void pickThrowAtScreenEdge(float px, float py) {
        throwTargetWx = 0.0f;
        throwTargetWy = 0.0f;
        throwTargetWw = (float)screenW;
        throwTargetWh = (float)screenH;
        throwEdge = pickEdge(px, py, 0.0f, 0.0f, (float)screenW, (float)screenH);
        throwAlong = 0.02f + (float)(rand() % 96) * 0.01f;
    }

    int allocBladeWin(float cx, float cy, bool recall) {
        UBSwordWin w;
        w.w = BLADE_WIN_W;
        w.h = BLADE_WIN_H;
        w.recall = recall;
        w.active = true;
        setBladeWinPos(w, cx, cy);
        bladeWins.push_back(w);
        return (int)bladeWins.size() - 1;
    }

    static void setBladeWinPos(UBSwordWin& w, float cx, float cy) {
        w.x = cx - w.w * 0.5f;
        w.y = cy - w.h * 0.5f;
    }

    void syncBladeWin(int idx, float cx, float cy) {
        if (idx < 0 || idx >= (int)bladeWins.size()) return;
        if (!bladeWins[idx].active) return;
        setBladeWinPos(bladeWins[idx], cx, cy);
    }

    void freeBladeWin(int idx) {
        if (idx < 0 || idx >= (int)bladeWins.size()) return;
        bladeWins[idx].active = false;
    }

    static void drawBlade(float x, float y, float ang, float len, float thick,
                          float r, float g, float b, float a) {
        float ex = x + cosf(ang) * len;
        float ey = y + sinf(ang) * len;
        ubDrawThickLine(x, y, ex, ey, thick * 2.2f, r * 0.4f, g * 0.4f, b * 0.4f, a * 0.24f);
        ubDrawThickLine(x, y, ex, ey, thick, r, g, b, a);
        drawCircle(ex, ey, thick * 0.68f, r, g * 1.1f, b, a);
    }

    void beginThrow() {
        if ((int)flies.size() >= MAX_FLY) return;
        float tx, ty;
        edgePoint(throwTargetWx, throwTargetWy, throwTargetWw, throwTargetWh,
                  throwEdge, throwAlong, tx, ty);
        UBFly f;
        f.fx = worldX; f.fy = worldY;
        f.x = worldX; f.y = worldY;
        f.tx = tx; f.ty = ty;
        f.twx = throwTargetWx; f.twy = throwTargetWy;
        f.tww = throwTargetWw; f.twh = throwTargetWh;
        f.edge = throwEdge;
        f.along = throwAlong;
        f.active = true;
        f.winIdx = allocBladeWin(f.x, f.y, false);
        flies.push_back(f);
        quiver--;

        if (chainLeft > 1) {
            chainLeft--;
            actionCd = 0.012f;
        } else if (crossLeft > 1) {
            crossLeft--;
            if (throwEdge == UBEdge::TOP) throwEdge = UBEdge::BOTTOM;
            else if (throwEdge == UBEdge::BOTTOM) throwEdge = UBEdge::TOP;
            else if (throwEdge == UBEdge::LEFT) throwEdge = UBEdge::RIGHT;
            else throwEdge = UBEdge::LEFT;
            actionCd = 0.018f;
        } else {
            actionCd = phase3 ? THROW_CD_P3 : (phase2 ? THROW_CD_P2 : THROW_CD_P1);
        }
    }

    void commitPin(const UBFly& f) {
        if ((int)pins.size() >= MAX_PINS)
            pins.erase(pins.begin());
        UBPin p;
        p.wx = f.twx; p.wy = f.twy;
        p.ww = f.tww; p.wh = f.twh;
        p.edge = f.edge;
        p.along = f.along;
        p.pinX = f.tx; p.pinY = f.ty;
        p.scar = 0.0f;
        pins.push_back(p);
    }

    void buildRecallBladeWins() {
        for (auto& w : bladeWins) w.active = false;
        bladeWins.clear();
        for (auto& p : pins) {
            if (p.scar > 0.5f) continue;
            allocBladeWin(p.pinX, p.pinY, true);
        }
    }

    void startRecallTelegraph() {
        buildRecallBladeWins();
        state = UBState::RECALL_TEL;
        stateT = RECALL_TEL;
        shakePulse = 1.0f;
    }

    void launchRecalls() {
        recalls.clear();
        int wi = 0;
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
            r.winIdx = wi;
            if (wi < (int)bladeWins.size()) {
                bladeWins[wi].recall = true;
                bladeWins[wi].active = true;
                setBladeWinPos(bladeWins[wi], r.x, r.y);
            }
            recalls.push_back(r);
            wi++;
            p.scar = 1.0f;
        }
        state = UBState::RECALLING;
        stateT = 0.0f;
    }

    void pickNextSkill() {
        if (phase3 && quiver >= 2 && (rand() % 100) < 48) {
            nextSkill = UBSkill::CHAIN;
            chainLeft = std::min(5, quiver);
            return;
        }
        if (phase2 && quiver >= 2 && (rand() % 100) < 50) {
            nextSkill = UBSkill::CROSS;
            crossLeft = 2;
            return;
        }
        nextSkill = UBSkill::EDGE;
    }

    float windupTime() const {
        if (phase3) return 0.035f;
        if (phase2) return 0.055f;
        return 0.075f;
    }

    int activeFlyCount() const {
        int n = 0;
        for (auto& f : flies) if (f.active) n++;
        return n;
    }

    void hurtEdge(float px, float py, float& playerHP, float dt) {
        for (auto& p : pins) {
            if (p.scar > 0.5f) continue;
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

    void updateFlies(float dt) {
        for (auto& f : flies) {
            if (!f.active) continue;
            float fdx = f.tx - f.x, fdy = f.ty - f.y;
            float fl = sqrtf(fdx * fdx + fdy * fdy);
            float step = FLY_SPD * dt;
            if (fl <= step + 1.0f) {
                f.x = f.tx; f.y = f.ty;
                f.active = false;
                commitPin(f);
                freeBladeWin(f.winIdx);
                shakePulse = 0.45f;
            } else {
                f.x += fdx / fl * step;
                f.y += fdy / fl * step;
            }
            syncBladeWin(f.winIdx, f.x, f.y);
        }
        flies.erase(std::remove_if(flies.begin(), flies.end(),
            [](const UBFly& f) { return !f.active; }), flies.end());
    }

    void updateRecalls(float px, float py, float dt, float& playerHP) {
        int activeN = 0;
        for (auto& r : recalls) {
            if (!r.active) continue;
            activeN++;
            float ox = r.x, oy = r.y;
            r.x += r.vx * dt;
            r.y += r.vy * dt;
            syncBladeWin(r.winIdx, r.x, r.y);
            if (SegDist(px, py, ox, oy, r.x, r.y) < 16.0f)
                HurtPlayer(playerHP, RECALL_DMG * dt * 8.0f);
            float bdx = worldX - r.x, bdy = worldY - r.y;
            if (bdx * bdx + bdy * bdy < BODY * BODY) {
                r.active = false;
                freeBladeWin(r.winIdx);
            }
        }
        if (state == UBState::RECALLING && activeN == 0) {
            recalls.clear();
            bladeWins.clear();
            pins.clear();
            quiver = maxQuiver;
            state = UBState::MOVE;
            actionCd = 0.25f;
            windupT = 0.0f;
        }
    }

    void tryStartThrow(float px, float py) {
        if (state != UBState::MOVE) return;
        if (quiver <= 0 || actionCd > 0.0f || windupT > 0.0f) return;
        if (activeFlyCount() >= MAX_FLY) return;
        pickNextSkill();
        pickThrowAtScreenEdge(px, py);
        windupT = windupTime();
    }

    void Update(float px, float py,
                float /*pwx*/, float /*pwy*/, float /*pww*/, float /*pwh*/,
                float dt, float& playerHP,
                std::vector<Bullet>& /*bullets*/) {
        if (!alive) return;

        spinAng += dt * (phase3 ? 3.4f : 2.2f);

        if (!phase2 && hp <= maxHp * 0.55f) { phase2 = true; refreshQuiver(); }
        if (!phase3 && hp <= maxHp * 0.28f) { phase3 = true; refreshQuiver(); }

        float dx = px - worldX, dy = py - worldY;
        float dist = sqrtf(dx * dx + dy * dy) + 1e-3f;
        float nx = dx / dist, ny = dy / dist;

        hurtEdge(px, py, playerHP, dt);

        if (orbitT > 0.0f) orbitT -= dt;
        if (dist < 160.0f && quiver > 0 && orbitT <= 0.0f)
            orbitT = 1.4f;

        actionCd -= dt;

        if (state == UBState::MOVE) {
            if (dist > 380.0f) {
                worldX += nx * moveSpeed * dt;
                worldY += ny * moveSpeed * dt;
            } else if (dist < 260.0f) {
                worldX -= nx * moveSpeed * 0.7f * dt;
                worldY -= ny * moveSpeed * 0.7f * dt;
            } else {
                worldX += -ny * moveSpeed * 0.35f * dt;
                worldY += nx * moveSpeed * 0.35f * dt;
            }
            clampPos();

            if (quiver <= 0 && activeFlyCount() == 0) {
                bool anyLivePin = false;
                for (auto& p : pins) if (p.scar < 0.5f) { anyLivePin = true; break; }
                if (anyLivePin) startRecallTelegraph();
                else {
                    quiver = maxQuiver;
                    actionCd = 0.30f;
                }
            }
        }

        updateFlies(dt);

        if (state == UBState::MOVE) {
            tryStartThrow(px, py);
            if (windupT > 0.0f) {
                windupT -= dt;
                if (windupT <= 0.0f) beginThrow();
            }
        }

        switch (state) {
        case UBState::RECALL_TEL:
            stateT -= dt;
            if (stateT <= 0.0f) launchRecalls();
            break;
        case UBState::RECALLING:
            updateRecalls(px, py, dt, playerHP);
            break;
        default: break;
        }

        for (auto& r : recalls) {
            if (!r.active) continue;
            if (SegDist(px, py, r.x - r.vx * dt, r.y - r.vy * dt, r.x, r.y) < 13.0f)
                HurtPlayer(playerHP, RECALL_DMG);
        }
    }

    void renderBody(float gt, float /*aimX*/, float /*aimY*/) const {
        float cx = worldX, cy = worldY;
        float pulse = 0.5f + 0.5f * sinf(gt * 6.5f);
        const float cr = 0.95f, cg = 0.20f, cb = 0.58f;

        if (phase3)
            drawCircle(cx, cy, BODY * 1.45f, cr, cg, cb, 0.09f + pulse * 0.07f);
        else if (phase2)
            drawCircle(cx, cy, BODY * 1.22f, cr, cg, cb, 0.06f + pulse * 0.05f);

        for (int i = 0; i < 6; i++) {
            float a = spinAng * 0.9f + (float)i * 1.047f;
            float fx = cx + cosf(a) * (BODY * 0.95f);
            float fy = cy + sinf(a) * (BODY * 0.62f);
            float fw = 26.0f, fh = 18.0f;
            drawRect(fx - fw * 0.5f, fy - fh * 0.5f, fw, fh, 0.03f, 0.02f, 0.05f, 0.55f);
            drawNeonBorder(fx - fw * 0.5f, fy - fh * 0.5f, fw, fh,
                           cr * 0.55f, cg * 0.55f, cb * 0.85f);
        }

        drawCircle(cx, cy, BODY * 0.82f, cr, cg, cb, 0.05f + pulse * 0.04f);
        drawNeonBorder(cx - BODY * 0.58f, cy - BODY * 0.58f,
                       BODY * 1.16f, BODY * 1.16f, cr, cg, cb);

        for (int i = 0; i < 4; i++) {
            float a = -spinAng * 1.4f + (float)i * 1.571f;
            float tx = cx + cosf(a) * BODY * 0.28f;
            float ty = cy + sinf(a) * BODY * 0.28f;
            drawTriangle(tx, ty, 11.0f, 0.35f, 0.95f, 1.0f, 0.55f + pulse * 0.25f);
        }

        drawCircle(cx, cy, BODY * 0.34f, 0.04f, 0.03f, 0.07f, 0.94f);
        drawCircle(cx, cy, BODY * 0.14f, 1.0f, 0.88f, 0.96f, 0.65f + pulse * 0.30f);

        int shown = std::min(quiver, 18);
        for (int i = 0; i < shown; i++) {
            float ring = (float)(i % 3);
            float rad = BODY + 28.0f + ring * 16.0f;
            float ang = gt * 2.4f + (float)i * (6.2831853f / (float)shown);
            float ox = cx + cosf(ang) * rad;
            float oy = cy + sinf(ang) * (rad * 0.62f);
            drawBlade(ox, oy, ang + 1.57f, 30.0f, 3.5f, cr, cg, cb, 0.88f);
        }

        if (orbitT > 0.0f) {
            float op = 0.5f + 0.5f * sinf(gt * 14.0f);
            drawNeonBorder(cx - 100.0f, cy - 100.0f, 200.0f, 200.0f, cr, cg, cb);
            drawCircle(cx, cy, 100.0f, cr, cg, cb, 0.10f * op);
        }

        if (state == UBState::RECALL_TEL) {
            float blink = 0.5f + 0.5f * sinf(gt * 16.0f);
            drawCircle(cx, cy, BODY * 0.9f, 1.0f, 0.22f, 0.42f, 0.22f * blink);
        }
    }

    void renderPinOnWindow(float wx, float wy, float ww, float wh, float /*gt*/) const {
        for (auto& p : pins) {
            float ox, oy;
            edgePoint(p.wx, p.wy, p.ww, p.wh, p.edge, p.along, ox, oy);
            if (!visibleInRect(ox, oy, wx, wy, ww, wh)) continue;
            float nx, ny;
            edgeNormal(p.edge, nx, ny);
            float alpha = (p.scar > 0.5f) ? 0.35f : 1.0f;
            const float outLen = 52.0f, inLen = 44.0f;
            float sx = ox + nx * outLen, sy = oy + ny * outLen;
            float ex = ox - nx * inLen,  ey = oy - ny * inLen;
            float ang = atan2f(ey - sy, ex - sx);
            float blen = sqrtf((ex - sx) * (ex - sx) + (ey - sy) * (ey - sy));
            drawBlade(sx, sy, ang, blen, 6.0f, 0.98f, 0.28f, 0.62f, alpha);
            drawCircle(ox, oy, 8.0f, 1.0f, 0.55f, 0.85f, alpha * 0.9f);

            float strip = EDGE_HAZ, hz = strip * 2.0f;
            if (p.edge == UBEdge::TOP || p.edge == UBEdge::BOTTOM)
                drawRect(ox - 28.0f, oy - strip, 56.0f, hz, 0.95f, 0.18f, 0.48f, 0.18f * alpha);
            else
                drawRect(ox - strip, oy - 28.0f, hz, 56.0f, 0.95f, 0.18f, 0.48f, 0.18f * alpha);
        }
    }

    static float pulse(float gt) { return 0.5f + 0.5f * sinf(gt * 18.0f); }

    void renderFlyBlade(float gt, const UBFly& f) const {
        float rdx = f.tx - f.x, rdy = f.ty - f.y;
        float rem = sqrtf(rdx * rdx + rdy * rdy);
        float ang = atan2f(rdy, rdx);
        ubDrawThickLine(f.fx, f.fy, f.x, f.y, 3.0f, 0.95f, 0.38f, 0.62f, 0.38f);
        drawBlade(f.x, f.y, ang, std::min(72.0f, rem + 20.0f), 7.0f,
                  0.98f, 0.32f, 0.68f, 1.0f);
        float p = pulse(gt);
        drawCircle(f.tx, f.ty, 12.0f + p * 5.0f, 1.0f, 0.55f, 0.85f, 0.55f * p);
    }

    void renderRecallBlade(float gt, const UBRecall& r) const {
        ubDrawThickLine(r.fromX, r.fromY, r.x, r.y, 4.5f, 1.0f, 0.28f, 0.48f, 0.88f);
        drawBlade(r.x, r.y, atan2f(r.vy, r.vx), 44.0f, 6.0f, 0.98f, 0.22f, 0.52f, 1.0f);
        float blink = 0.45f + 0.45f * sinf(gt * 14.0f);
        if (state == UBState::RECALL_TEL)
            drawCircle(r.fromX, r.fromY, 12.0f + blink * 8.0f, 1.0f, 0.35f, 0.55f, 0.65f * blink);
    }

    void renderWorld(float gt, float wx, float wy, float ww, float wh) const {
        float bx = worldX - WIN_W * 0.5f, by = worldY - WIN_H * 0.5f;
        bool isBossWin = sameWindow(wx, wy, ww, wh, bx, by, WIN_W, WIN_H);
        auto inWinPt = [&](float px, float py) {
            return px >= wx && px <= wx + ww && py >= wy && py <= wy + wh;
        };

        for (auto& f : flies) {
            if (!f.active) continue;
            if (f.winIdx >= 0 && f.winIdx < (int)bladeWins.size()) {
                auto& bw = bladeWins[f.winIdx];
                if (bw.active && rectOverlap(bw.x, bw.y, bw.w, bw.h, wx, wy, ww, wh))
                    renderFlyBlade(gt, f);
            } else if (isBossWin || inWinPt(f.x, f.y) || inWinPt(f.tx, f.ty))
                renderFlyBlade(gt, f);
        }

        if (windupT > 0.0f && state == UBState::MOVE) {
            float tx, ty;
            edgePoint(throwTargetWx, throwTargetWy, throwTargetWw, throwTargetWh,
                      throwEdge, throwAlong, tx, ty);
            if (visibleInRect(tx, ty, wx, wy, ww, wh, 24.0f)) {
                float p = pulse(gt);
                drawCircle(tx, ty, 10.0f + p * 7.0f, 1.0f, 0.50f, 0.80f, 0.75f * p);
            }
        }

        for (size_t i = 0; i < recalls.size(); i++) {
            auto& r = recalls[i];
            if (!r.active) continue;
            if (r.winIdx >= 0 && r.winIdx < (int)bladeWins.size()) {
                auto& bw = bladeWins[r.winIdx];
                if (bw.active && rectOverlap(bw.x, bw.y, bw.w, bw.h, wx, wy, ww, wh))
                    renderRecallBlade(gt, r);
            }
        }

        if (state == UBState::RECALL_TEL) {
            float blink = 0.40f + 0.40f * (0.5f + 0.5f * sinf(gt * 12.0f));
            float prog = 1.0f - stateT / RECALL_TEL;
            for (auto& bw : bladeWins) {
                if (!bw.active || !bw.recall) continue;
                if (!rectOverlap(bw.x, bw.y, bw.w, bw.h, wx, wy, ww, wh)) continue;
                float cx = bw.x + bw.w * 0.5f, cy = bw.y + bw.h * 0.5f;
                ubDrawThickLine(cx, cy, worldX, worldY, 4.0f,
                                1.0f, 0.22f, 0.42f, blink * (0.35f + 0.55f * prog));
                drawCircle(cx, cy, 14.0f + pulse(gt) * 7.0f, 1.0f, 0.35f, 0.55f, 0.70f * blink);
            }
        }
    }

    void renderWindowPass(float gt, float wx, float wy, float ww, float wh) const {
        renderWorld(gt, wx, wy, ww, wh);
        renderPinOnWindow(wx, wy, ww, wh, gt);
    }

    static const wchar_t* stateTag(UBState s) {
        switch (s) {
        case UBState::RECALL_TEL:  return L"PULL…";
        case UBState::RECALLING:   return L"PULL";
        default:                   return L"EDGE";
        }
    }
};
