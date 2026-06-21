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

// UNKNOWN.sys — 30검: 20 연속 투척(게임창 끝 pin) + 10 예비(플레이어 주위 EDGE CAGE)

enum class UBEdge { TOP, RIGHT, BOTTOM, LEFT };
enum class UBState { MOVE, RESERVE_TEL, RESERVE, RECALL_TEL, RECALLING };
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
    bool  telegraph = false;
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

struct UBReserveTarget {
    UBEdge edge;
    float along;
    float x, y;
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

    int   quiver = 30;
    int   maxQuiver = 30;
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
    bool  reservePending = false;
    int   reserveIdx = 0;
    float reserveCd = 0.0f;
    int   windupTelIdx = -1;
    float telTargetX = 0.0f, telTargetY = 0.0f;

    float throwTargetWx = 0, throwTargetWy = 0, throwTargetWw = 0, throwTargetWh = 0;
    UBEdge throwEdge = UBEdge::TOP;
    float throwAlong = 0.5f;

    std::vector<UBPin>            pins;
    std::vector<UBFly>            flies;
    std::vector<UBRecall>         recalls;
    std::vector<UBSwordWin>       bladeWins;
    std::vector<UBReserveTarget>  reserveTargets;

    static constexpr float BODY = 58.0f;
    static constexpr float WIN_W = 500.0f;
    static constexpr float WIN_H = 580.0f;
    static constexpr float BLADE_WIN_W = 132.0f;
    static constexpr float BLADE_WIN_H = 100.0f;
    static constexpr int   TOTAL_QUIVER = 30;
    static constexpr int   EDGE_THROW_COUNT = 20;
    static constexpr int   RESERVE_COUNT = 10;
    static constexpr float RECALL_TEL = 1.85f;
    static constexpr float RESERVE_TEL_DUR = 1.45f;
    static constexpr float EDGE_HAZ = 14.0f;
    static constexpr int   MAX_PINS = 34;
    static constexpr int   MAX_FLY  = 20;
    static constexpr float FLY_SPD = 2400.0f;
    static constexpr float RECALL_SPD = 980.0f;
    static constexpr float RECALL_DMG = 15.0f;
    static constexpr float EDGE_DPS = 11.0f;
    static constexpr float THROW_CD_P1 = 0.078f;
    static constexpr float THROW_CD_P2 = 0.058f;
    static constexpr float THROW_CD_P3 = 0.040f;
    static constexpr float RESERVE_THROW_GAP = 0.105f;

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
        maxQuiver = TOTAL_QUIVER;
        if (quiver > maxQuiver) quiver = maxQuiver;
    }

    void resetCycle() {
        quiver = maxQuiver;
        reservePending = false;
        reserveIdx = 0;
        reserveCd = 0.0f;
        reserveTargets.clear();
        clearWindupTel();
        state = UBState::MOVE;
        actionCd = 0.30f;
        windupT = 0.0f;
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

    void pickThrowAtPlayerEdge(float px, float py, float jitter = 0.04f) {
        float sw = (float)screenW, sh = (float)screenH;
        throwTargetWx = 0.0f;
        throwTargetWy = 0.0f;
        throwTargetWw = sw;
        throwTargetWh = sh;
        throwEdge = pickEdge(px, py, 0.0f, 0.0f, sw, sh);
        float j = ((float)(rand() % 100) - 50.0f) * 0.001f * jitter * 10.0f;
        if (throwEdge == UBEdge::TOP || throwEdge == UBEdge::BOTTOM)
            throwAlong = px / sw + j;
        else
            throwAlong = py / sh + j;
        throwAlong = std::max(0.06f, std::min(0.94f, throwAlong));
        edgePoint(throwTargetWx, throwTargetWy, throwTargetWw, throwTargetWh,
                  throwEdge, throwAlong, telTargetX, telTargetY);
    }

    void buildReserveFan(float px, float py) {
        reserveTargets.clear();
        float sw = (float)screenW, sh = (float)screenH;
        UBEdge main = pickEdge(px, py, 0.0f, 0.0f, sw, sh);
        float base = (main == UBEdge::TOP || main == UBEdge::BOTTOM) ? px / sw : py / sh;
        for (int i = 0; i < RESERVE_COUNT; i++) {
            UBReserveTarget rt;
            rt.edge = main;
            if (i == 3 || i == 6) {
                if (main == UBEdge::TOP) rt.edge = UBEdge::LEFT;
                else if (main == UBEdge::BOTTOM) rt.edge = UBEdge::RIGHT;
                else if (main == UBEdge::LEFT) rt.edge = UBEdge::TOP;
                else rt.edge = UBEdge::BOTTOM;
            }
            float spread = ((float)i - 4.5f) * 0.034f;
            if (rt.edge == UBEdge::TOP || rt.edge == UBEdge::BOTTOM)
                rt.along = base + spread;
            else
                rt.along = base + spread * (sh / sw);
            rt.along = std::max(0.05f, std::min(0.95f, rt.along));
            edgePoint(0.0f, 0.0f, sw, sh, rt.edge, rt.along, rt.x, rt.y);
            reserveTargets.push_back(rt);
        }
    }

    int allocBladeWin(float cx, float cy, bool recall, bool telegraph = false) {
        UBSwordWin w;
        w.w = BLADE_WIN_W;
        w.h = BLADE_WIN_H;
        w.recall = recall;
        w.telegraph = telegraph;
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

    void clearWindupTel() {
        if (windupTelIdx >= 0) {
            freeBladeWin(windupTelIdx);
            windupTelIdx = -1;
        }
    }

    void beginWindupTelegraph() {
        clearWindupTel();
        windupTelIdx = allocBladeWin(telTargetX, telTargetY, false, true);
    }

    static void drawBlade(float x, float y, float ang, float len, float thick,
                          float r, float g, float b, float a) {
        float ex = x + cosf(ang) * len;
        float ey = y + sinf(ang) * len;
        ubDrawThickLine(x, y, ex, ey, thick * 2.2f, r * 0.4f, g * 0.4f, b * 0.4f, a * 0.24f);
        ubDrawThickLine(x, y, ex, ey, thick, r, g, b, a);
        drawCircle(ex, ey, thick * 0.68f, r, g * 1.1f, b, a);
    }

    void launchFlyTo(float tx, float ty, UBEdge edge, float along) {
        if ((int)flies.size() >= MAX_FLY || quiver <= 0) return;
        UBFly f;
        f.fx = worldX; f.fy = worldY;
        f.x = worldX; f.y = worldY;
        f.tx = tx; f.ty = ty;
        f.twx = 0.0f; f.twy = 0.0f;
        f.tww = (float)screenW; f.twh = (float)screenH;
        f.edge = edge;
        f.along = along;
        f.active = true;
        f.winIdx = allocBladeWin(f.x, f.y, false);
        flies.push_back(f);
        quiver--;
        if (quiver == RESERVE_COUNT)
            reservePending = true;
    }

    void beginThrow() {
        clearWindupTel();
        launchFlyTo(telTargetX, telTargetY, throwEdge, throwAlong);

        if (chainLeft > 1) {
            chainLeft--;
            actionCd = 0.025f;
        } else if (crossLeft > 1) {
            crossLeft--;
            if (throwEdge == UBEdge::TOP) throwEdge = UBEdge::BOTTOM;
            else if (throwEdge == UBEdge::BOTTOM) throwEdge = UBEdge::TOP;
            else if (throwEdge == UBEdge::LEFT) throwEdge = UBEdge::RIGHT;
            else throwEdge = UBEdge::LEFT;
            edgePoint(throwTargetWx, throwTargetWy, throwTargetWw, throwTargetWh,
                      throwEdge, throwAlong, telTargetX, telTargetY);
            actionCd = 0.032f;
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
        clearWindupTel();
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
        if (quiver <= RESERVE_COUNT) return;
        if (phase3 && quiver > RESERVE_COUNT + 2 && (rand() % 100) < 40) {
            nextSkill = UBSkill::CHAIN;
            chainLeft = std::min(4, quiver - RESERVE_COUNT);
            return;
        }
        if (phase2 && quiver > RESERVE_COUNT + 1 && (rand() % 100) < 42) {
            nextSkill = UBSkill::CROSS;
            crossLeft = 2;
            return;
        }
        nextSkill = UBSkill::EDGE;
    }

    float windupTime() const {
        if (phase3) return 0.075f;
        if (phase2) return 0.095f;
        return 0.115f;
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
                shakePulse = 0.40f;
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
            resetCycle();
        }
    }

    void tryStartThrow(float px, float py) {
        if (state != UBState::MOVE) return;
        if (quiver <= RESERVE_COUNT) return;
        if (actionCd > 0.0f || windupT > 0.0f) return;
        if (activeFlyCount() >= MAX_FLY) return;
        pickNextSkill();
        pickThrowAtPlayerEdge(px, py);
        beginWindupTelegraph();
        windupT = windupTime();
    }

    void startReservePhase(float px, float py) {
        buildReserveFan(px, py);
        reservePending = false;
        reserveIdx = 0;
        reserveCd = 0.0f;
        clearWindupTel();
        state = UBState::RESERVE_TEL;
        stateT = RESERVE_TEL_DUR;
        shakePulse = 0.8f;
    }

    void tickReserveThrows() {
        if (reserveIdx >= (int)reserveTargets.size()) return;
        if (activeFlyCount() >= MAX_FLY) return;
        auto& rt = reserveTargets[reserveIdx];
        launchFlyTo(rt.x, rt.y, rt.edge, rt.along);
        reserveIdx++;
        reserveCd = RESERVE_THROW_GAP;
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
        if (dist < 160.0f && quiver > RESERVE_COUNT && orbitT <= 0.0f)
            orbitT = 1.4f;

        actionCd -= dt;

        if (state == UBState::MOVE || state == UBState::RESERVE_TEL || state == UBState::RESERVE) {
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
        }

        updateFlies(dt);

        if (state == UBState::MOVE) {
            if (reservePending && quiver == RESERVE_COUNT && activeFlyCount() == 0)
                startReservePhase(px, py);

            if (quiver <= 0 && activeFlyCount() == 0) {
                bool anyLivePin = false;
                for (auto& p : pins) if (p.scar < 0.5f) { anyLivePin = true; break; }
                if (anyLivePin) startRecallTelegraph();
                else resetCycle();
            }

            tryStartThrow(px, py);
            if (windupT > 0.0f) {
                windupT -= dt;
                if (windupT <= 0.0f) beginThrow();
            }
        }

        switch (state) {
        case UBState::RESERVE_TEL:
            stateT -= dt;
            if (stateT <= 0.0f) {
                state = UBState::RESERVE;
                reserveCd = 0.0f;
            }
            break;

        case UBState::RESERVE:
            reserveCd -= dt;
            if (reserveCd <= 0.0f && reserveIdx < (int)reserveTargets.size())
                tickReserveThrows();
            if (reserveIdx >= (int)reserveTargets.size() &&
                quiver <= 0 && activeFlyCount() == 0) {
                bool anyLivePin = false;
                for (auto& p : pins) if (p.scar < 0.5f) { anyLivePin = true; break; }
                if (anyLivePin) startRecallTelegraph();
                else resetCycle();
            }
            break;

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

    static float pulse(float gt) { return 0.5f + 0.5f * sinf(gt * 18.0f); }

    void renderWindupTelegraph(float gt, float wx, float wy, float ww, float wh) const {
        if (windupT <= 0.0f || state != UBState::MOVE) return;
        if (!visibleInRect(telTargetX, telTargetY, wx, wy, ww, wh, 90.0f) &&
            !visibleInRect(worldX, worldY, wx, wy, ww, wh))
            return;

        float prog = 1.0f - windupT / std::max(0.01f, windupTime());
        float blink = 0.45f + 0.45f * (0.5f + 0.5f * sinf(gt * 14.0f));
        float ang = atan2f(telTargetY - worldY, telTargetX - worldX);

        ubDrawThickLine(worldX, worldY, telTargetX, telTargetY, 3.5f,
                        0.95f, 0.35f, 0.65f, (0.25f + 0.45f * prog) * blink);
        int segs = 10;
        for (int i = 0; i <= segs; i++) {
            if ((i & 1) == 0) continue;
            float u = (float)i / (float)segs;
            float sx = worldX + (telTargetX - worldX) * u;
            float sy = worldY + (telTargetY - worldY) * u;
            drawCircle(sx, sy, 5.0f, 0.95f, 0.40f, 0.70f, 0.35f * blink);
        }
        drawBlade(worldX, worldY, ang, 38.0f, 4.5f, 0.95f, 0.32f, 0.65f, 0.75f * blink);
        float p = pulse(gt);
        drawCircle(telTargetX, telTargetY, 14.0f + p * 10.0f, 1.0f, 0.55f, 0.85f, 0.70f * blink);
        drawNeonBorder(telTargetX - 22.0f, telTargetY - 22.0f, 44.0f, 44.0f,
                       0.95f, 0.35f, 0.68f);
    }

    void renderReserveTelegraph(float gt, float wx, float wy, float ww, float wh) const {
        if (state != UBState::RESERVE_TEL && state != UBState::RESERVE) return;
        float blink = 0.40f + 0.40f * (0.5f + 0.5f * sinf(gt * 13.0f));
        float prog = (state == UBState::RESERVE_TEL)
            ? (1.0f - stateT / RESERVE_TEL_DUR) : 1.0f;
        for (auto& rt : reserveTargets) {
            if (!visibleInRect(rt.x, rt.y, wx, wy, ww, wh, 90.0f)) continue;
            ubDrawThickLine(worldX, worldY, rt.x, rt.y, 3.0f,
                            1.0f, 0.28f, 0.50f, blink * (0.30f + 0.50f * prog));
            drawCircle(rt.x, rt.y, 12.0f + pulse(gt) * 8.0f, 1.0f, 0.40f, 0.65f, 0.65f * blink);
            float nx, ny;
            edgeNormal(rt.edge, nx, ny);
            drawBlade(rt.x + nx * 28.0f, rt.y + ny * 28.0f,
                      atan2f(-ny, -nx), 32.0f, 4.0f, 0.98f, 0.25f, 0.55f, 0.55f * blink);
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
            drawRect(fx - 13.0f, fy - 9.0f, 26.0f, 18.0f, 0.03f, 0.02f, 0.05f, 0.55f);
            drawNeonBorder(fx - 13.0f, fy - 9.0f, 26.0f, 18.0f, cr * 0.55f, cg * 0.55f, cb * 0.85f);
        }

        drawCircle(cx, cy, BODY * 0.82f, cr, cg, cb, 0.05f + pulse * 0.04f);
        drawNeonBorder(cx - BODY * 0.58f, cy - BODY * 0.58f,
                       BODY * 1.16f, BODY * 1.16f, cr, cg, cb);

        drawCircle(cx, cy, BODY * 0.34f, 0.04f, 0.03f, 0.07f, 0.94f);
        drawCircle(cx, cy, BODY * 0.14f, 1.0f, 0.88f, 0.96f, 0.65f + pulse * 0.30f);

        int shown = std::min(quiver, 22);
        for (int i = 0; i < shown; i++) {
            float ring = (float)(i % 4);
            float rad = BODY + 24.0f + ring * 14.0f;
            float ang = gt * 2.2f + (float)i * (6.2831853f / (float)std::max(1, shown));
            drawBlade(cx + cosf(ang) * rad, cy + sinf(ang) * (rad * 0.62f),
                      ang + 1.57f, 28.0f, 3.2f, cr, cg, cb, 0.85f);
        }

        if (state == UBState::RESERVE_TEL || state == UBState::RESERVE) {
            float blink = 0.5f + 0.5f * sinf(gt * 18.0f);
            drawCircle(cx, cy, BODY * 1.05f, 1.0f, 0.30f, 0.50f, 0.20f * blink);
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
            drawBlade(sx, sy, ang, sqrtf((ex - sx) * (ex - sx) + (ey - sy) * (ey - sy)),
                      6.0f, 0.98f, 0.28f, 0.62f, alpha);
            drawCircle(ox, oy, 8.0f, 1.0f, 0.55f, 0.85f, alpha * 0.9f);
        }
    }

    void renderFlyBlade(float gt, const UBFly& f) const {
        float rdx = f.tx - f.x, rdy = f.ty - f.y;
        float rem = sqrtf(rdx * rdx + rdy * rdy);
        float ang = atan2f(rdy, rdx);
        ubDrawThickLine(f.fx, f.fy, f.x, f.y, 3.0f, 0.95f, 0.38f, 0.62f, 0.38f);
        drawBlade(f.x, f.y, ang, std::min(72.0f, rem + 20.0f), 7.0f,
                  0.98f, 0.32f, 0.68f, 1.0f);
        drawCircle(f.tx, f.ty, 10.0f + pulse(gt) * 4.0f, 1.0f, 0.55f, 0.85f, 0.50f);
    }

    void renderRecallBlade(float gt, const UBRecall& r) const {
        ubDrawThickLine(r.fromX, r.fromY, r.x, r.y, 4.5f, 1.0f, 0.28f, 0.48f, 0.88f);
        drawBlade(r.x, r.y, atan2f(r.vy, r.vx), 44.0f, 6.0f, 0.98f, 0.22f, 0.52f, 1.0f);
        (void)gt;
    }

    void renderWorld(float gt, float wx, float wy, float ww, float wh) const {
        float bx = worldX - WIN_W * 0.5f, by = worldY - WIN_H * 0.5f;
        bool isBossWin = sameWindow(wx, wy, ww, wh, bx, by, WIN_W, WIN_H);

        renderWindupTelegraph(gt, wx, wy, ww, wh);
        renderReserveTelegraph(gt, wx, wy, ww, wh);

        for (auto& f : flies) {
            if (!f.active) continue;
            if (f.winIdx >= 0 && f.winIdx < (int)bladeWins.size()) {
                auto& bw = bladeWins[f.winIdx];
                if (bw.active && rectOverlap(bw.x, bw.y, bw.w, bw.h, wx, wy, ww, wh))
                    renderFlyBlade(gt, f);
            } else if (isBossWin)
                renderFlyBlade(gt, f);
        }

        for (auto& r : recalls) {
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
            }
        }
    }

    void renderWindowPass(float gt, float wx, float wy, float ww, float wh) const {
        renderWorld(gt, wx, wy, ww, wh);
        renderPinOnWindow(wx, wy, ww, wh, gt);
    }

    static const wchar_t* stateTag(UBState s) {
        switch (s) {
        case UBState::RESERVE_TEL: return L"CAGE…";
        case UBState::RESERVE:     return L"CAGE";
        case UBState::RECALL_TEL:  return L"PULL…";
        case UBState::RECALLING:   return L"PULL";
        default:                   return L"EDGE";
        }
    }

    int edgeBladesLeft() const {
        return std::max(0, quiver - RESERVE_COUNT);
    }
};
