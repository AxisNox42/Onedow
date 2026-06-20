#pragma once
// ─────────────────────────────────────────────────────────────
// CORRUPT.dll — 깨진 디스플레이 보스 (v2 전면 재작성)
//
//   컨셉: CORRUPT.dll 단일 창 + 내부 PHANTOM 분신 + RGB TEAR 예고 레이저
//
//   사이클 (반복):
//     CORRUPT  → 부팅 글리치 (2s)
//     FRAGMENT → 이동·샤드·펄스 (스캔)
//     (LOCK)   → FRAGMENT 말미, 고정 방향 전조 빔 (1.2s)
//     TEAR     → RGB 관통 빔 (1.4s)
//     SYNC     → 코어 노출 · 딜 타임 (2.2s)
//
//   페이즈:
//     P2 50% — PHANTOM 분신 2 (각 10% HP, CORRUPT 창 내부)
//     P3 25% — 분신 3, 사이클 가속, SWAP
// ─────────────────────────────────────────────────────────────

#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include "DrawPrim.h"
#include "TextRenderer.h"

extern TextRenderer g_TextS;

enum class BossState { CORRUPT, FRAGMENT, TEAR, SYNC };

struct GlitchShard {
    float x = 0, y = 0;
    float vx = 0, vy = 0;
    float orbitAng = 0.0f;
    float orbitR = 40.0f;
    float homingDelay = 0.0f;
    bool  alive = true;
};

class GlitchBoss {
public:
    float worldX = 0, worldY = 0;
    float hp = 9000.0f, maxHp = 9000.0f;
    bool  alive = true;
    bool  exploded = false;
    int   screenW = 0, screenH = 0;

    BossState state = BossState::CORRUPT;
    float stateTimer = 0.0f;

    bool  phase2 = false;
    bool  phase3 = false;

    std::vector<GlitchShard> shards;

    // RGB TEAR
    bool  laserWarn = false;
    float laserWarnT = 0.0f;
    float beamDirX = 1.0f, beamDirY = 0.0f;
    float beam2DirX = 0.0f, beam2DirY = 0.0f;
    bool  beam1 = false, beam2 = false, beam3 = false;
    float beam2T = 0.0f, beam3T = 0.0f;

    // 연출 (main UI 글리치용)
    float glitchAmount = 0.0f;
    float textNoise = 0.0f;
    float tearFlash = 0.0f;
    float attackBanner = 0.0f;
    float swapFlash = 0.0f;
    float spinAng = 0.0f;

    struct Decoy {
        float offX = 0, offY = 0;
        float hp = 0, maxHp = 0;
        bool  alive = true;
        float respawn = 0.0f;
        int   winSeed = 0;
        float driftTX = 0, driftTY = 0, driftTimer = 0.0f;
    };
    std::vector<Decoy> decoys;

    static constexpr float BODY = 44.0f;
    static constexpr float WIN_TB = 22.0f;
    static constexpr float GLITCH_WIN_TB = 22.0f;

    static constexpr float T_BOOT = 2.0f;
    static constexpr float T_SCAN = 4.5f;
    static constexpr float T_LOCK = 1.25f;
    static constexpr float T_TEAR = 1.45f;
    static constexpr float T_SYNC = 2.2f;
    static constexpr float T_BEAM1 = 1.15f;
    static constexpr float BEAM_STAGGER = 0.38f;
    static constexpr float BEAM_HALF = 30.0f;
    static constexpr float BEAM_HALF_LOCK = 20.0f;
    static constexpr float DECOY_HP_SHARE = 0.10f;
    static constexpr float SWAP_INT = 7.5f;

    GlitchBoss(int sw, int sh, float hpInit) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.72f;
        worldY = sh * 0.28f;
    }

    static const wchar_t* BossName() { return L"CORRUPT.dll"; }

    static void phantomLabel(int seed, wchar_t* out, int cap) {
        swprintf_s(out, cap, L"PHANTOM_%04X", seed & 0xFFFF);
    }

    float decoyWorldX(const Decoy& d) const { return worldX + d.offX; }
    float decoyWorldY(const Decoy& d) const { return worldY + d.offY; }
    float coreWorldX() const { return worldX + coreOffX_; }
    float coreWorldY() const { return worldY + coreOffY_; }

    float combinedHp() const {
        float t = hp;
        for (auto& d : decoys) if (d.alive) t += d.hp;
        return t;
    }

    float combinedMaxHp() const {
        int n = 0;
        for (auto& d : decoys) if (d.alive) n++;
        return maxHp + maxHp * DECOY_HP_SHARE * (float)n;
    }

    bool hasBeamVisual() const {
        return laserWarn || beam1 || beam2 || beam3;
    }

    bool suppressScreenGlitch() const {
        return state == BossState::TEAR || hasBeamVisual();
    }

    const wchar_t* stateTag() const {
        switch (state) {
        case BossState::CORRUPT:  return laserWarn ? L"LOCK" : L"BOOT";
        case BossState::FRAGMENT: return laserWarn ? L"LOCK" : L"SCAN";
        case BossState::TEAR:     return L"RGB TEAR";
        case BossState::SYNC:     return L"SYNC";
        default: return L"?";
        }
    }

    void syncAlive() {
        if (hp <= 0.0f) {
            bool any = false;
            for (auto& d : decoys) if (d.alive && d.hp > 0.0f) any = true;
            if (!any) alive = false;
        }
    }

    // ── Update ────────────────────────────────────────────────

    void Update(float playerCX, float playerCY, float dt, float& playerHP) {
        if (!alive) return;

        stateTimer += dt;
        spinAng += dt * 2.2f;
        tearFlash = std::max(0.0f, tearFlash - dt * 2.2f);
        attackBanner = std::max(0.0f, attackBanner - dt * 1.0f);
        swapFlash = std::max(0.0f, swapFlash - dt * 2.4f);
        textNoise = std::max(0.0f, textNoise - dt * 1.6f);

        if (!phase2 && combinedHp() <= combinedMaxHp() * 0.5f) phase2 = true;
        if (!phase3 && combinedHp() <= combinedMaxHp() * 0.25f) phase3 = true;

        if (phase2 || phase3) {
            ensureDecoys();
            tickDecoys(dt);
            tickSwap(dt);
        } else {
            decoys.clear();
            coreOffX_ = coreOffY_ = 0.0f;
        }

        driftBoss(dt);
        tickShards(playerCX, playerCY, dt, playerHP);
        tickBeamDamage(playerCX, playerCY, dt, playerHP);

        switch (state) {
        case BossState::CORRUPT:
            glitchAmount = 0.22f + 0.1f * sinf(stateTimer * 8.0f);
            if (stateTimer < 0.4f) textNoise = 0.5f;
            if (stateTimer >= T_BOOT) {
                state = BossState::FRAGMENT;
                stateTimer = 0.0f;
                glitchAmount = 0.05f;
            }
            break;

        case BossState::FRAGMENT:
            glitchAmount = 0.04f;
            tickScan(playerCX, playerCY, dt);
            if (laserWarn) {
                laserWarnT += dt;
                if (laserWarnT >= T_LOCK) enterTear();
            } else if (stateTimer >= scanDuration()) {
                beginLock(playerCX, playerCY);
            }
            break;

        case BossState::TEAR:
            glitchAmount = 0.08f;
            beam2T += dt;
            beam3T += dt;
            if (stateTimer >= 0.0f && !beam1) beam1 = true;
            if (beam2T >= BEAM_STAGGER) beam2 = true;
            if (beam3T >= BEAM_STAGGER * 1.4f) beam3 = true;
            if (stateTimer >= T_BEAM1) beam1 = false;
            if (stateTimer >= T_TEAR) {
                state = BossState::SYNC;
                stateTimer = 0.0f;
                beam1 = beam2 = beam3 = false;
            }
            break;

        case BossState::SYNC:
            glitchAmount = 0.0f;
            textNoise = 0.04f;
            shards.clear();
            if (stateTimer >= T_SYNC) {
                state = BossState::FRAGMENT;
                stateTimer = 0.0f;
                laserWarn = false;
                laserWarnT = 0.0f;
            }
            break;
        }

        syncAlive();
    }

    // ── Hit ───────────────────────────────────────────────────

    float tryHitDecoy(float bx, float by, float pbx, float pby, float dmg) {
        for (auto& d : decoys) {
            if (!d.alive || d.hp <= 0.0f) continue;
            if (segDist(bx, by, pbx, pby, decoyWorldX(d), decoyWorldY(d)) < BODY * 0.72f) {
                float dealt = (dmg < d.hp) ? dmg : d.hp;
                d.hp -= dealt;
                if (d.hp <= 0.0f) onDecoyDead(d);
                return dealt;
            }
        }
        return 0.0f;
    }

    // ── Render (창 내부 — scissor 안) ─────────────────────────

    void renderShards(float gt) const {
        for (auto& s : shards) {
            if (!s.alive) continue;
            float flick = 0.5f + 0.5f * sinf(gt * 20.0f + s.x * 0.08f);
            drawRect(s.x - 11.0f, s.y - 2.5f, 22.0f, 5.0f, 0.1f, 0.92f, 1.0f, 0.92f);
            drawRect(s.x - 7.0f + flick * 2.0f, s.y - 3.5f, 14.0f, 7.0f,
                     0.95f, 0.22f, 0.58f, 0.85f);
            drawCircle(s.x, s.y, 5.5f, 0.4f, 0.95f, 1.0f, 0.7f);
        }
    }

    void renderBody(float gt) const {
        if (swapFlash > 0.02f)
            drawCircle(worldX, worldY, BODY * 1.4f, 1.0f, 0.35f, 0.7f, swapFlash * 0.22f);
        drawMonitorCore(worldX + coreOffX_, worldY + coreOffY_, gt, true,
                        (maxHp > 0.0f) ? hp / maxHp : 1.0f);
        wchar_t tag[32];
        swprintf_s(tag, L"[%ls]", stateTag());
        float tw = g_TextS.Width(tag, 0.44f);
        g_TextS.Draw(tag, worldX + coreOffX_ - tw * 0.5f, worldY + coreOffY_ - BODY - 30.0f, 0.44f,
                     0.95f, 0.35f, 0.72f, 0.88f);
    }

    void renderDecoys(float gt) const {
        for (auto& d : decoys) {
            if (!d.alive) continue;
            float cx = decoyWorldX(d), cy = decoyWorldY(d);
            float hf = (d.maxHp > 0.0f) ? d.hp / d.maxHp : 0.0f;
            drawMonitorCore(cx, cy, gt, false, hf);
            wchar_t tag[24];
            phantomLabel(d.winSeed, tag, 24);
            float tw = g_TextS.Width(tag, 0.34f);
            g_TextS.Draw(tag, cx - tw * 0.5f, cy - BODY - 18.0f, 0.34f,
                         0.55f, 0.75f, 0.95f, 0.72f);
        }
    }

    // ── Render (월드 FX — UI ortho 전, caller가 additive 처리) ─

    void renderTelegraphs(float px, float py, float gt) const {
        if (state == BossState::SYNC) {
            float pulse = 0.5f + 0.5f * sinf(gt * 10.0f);
            drawCircle(worldX, worldY, BODY * 1.5f, 0.95f, 0.3f, 0.65f, 0.14f + pulse * 0.12f);
            drawCircle(worldX, worldY, BODY * 1.0f, 1.0f, 0.55f, 0.85f, 0.1f + pulse * 0.08f);
        }

        if (laserWarn) {
            float prog = laserWarnT / T_LOCK;
            if (prog > 1.0f) prog = 1.0f;
            drawCircle(px, py, 18.0f + prog * 16.0f, 0.3f, 0.95f, 1.0f, 0.25f + prog * 0.35f);
            drawCircle(coreWorldX(), coreWorldY(), BODY * (0.95f + prog * 0.3f), 0.95f, 0.25f, 0.55f, 0.2f + prog * 0.25f);
        }

        for (auto& s : shards) {
            if (!s.alive || s.homingDelay <= 0.0f) continue;
            drawCircle(s.x, s.y, 8.0f, 0.35f, 0.95f, 1.0f, 0.35f);
        }
    }

    void renderBeams(float gt) const {
        float reach = (float)(screenW + screenH) * 1.2f;
        float perpX = -beamDirY, perpY = beamDirX;

        if (laserWarn) {
            float prog = laserWarnT / T_LOCK;
            if (prog > 1.0f) prog = 1.0f;
            float blink = 0.5f + 0.5f * sinf(gt * 22.0f);
            drawRgbBeam(coreWorldX(), coreWorldY(), beamDirX, beamDirY, reach,
                        BEAM_HALF_LOCK * (0.8f + prog * 0.25f),
                        blink * (0.4f + prog * 0.45f), blink * (0.55f + prog * 0.4f), 3.0f);
        }

        if (beam1) {
            float f = 0.85f + 0.15f * sinf(gt * 30.0f);
            drawRgbBeam(coreWorldX(), coreWorldY(), beamDirX, beamDirY, reach, BEAM_HALF, 0.95f * f, 0.98f * f, 5.0f);
            drawCircle(coreWorldX(), coreWorldY(), BODY, 1.0f, 0.25f, 0.45f, 0.5f * f);
        }
        if (beam2) {
            float f = 0.82f + 0.18f * sinf(gt * 28.0f + 1.0f);
            drawBeam(coreWorldX(), coreWorldY(), beam2DirX, beam2DirY, reach,
                     0.15f, 0.98f, 0.4f, BEAM_HALF * 0.9f, 0.88f * f, 0.95f * f);
        }
        if (beam3) {
            float d3x = beamDirX * 0.9f - perpX * 0.12f;
            float d3y = beamDirY * 0.9f - perpY * 0.12f;
            float f = 0.82f + 0.18f * sinf(gt * 28.0f + 2.0f);
            drawBeam(coreWorldX(), coreWorldY(), d3x, d3y, reach,
                     0.45f, 0.18f, 0.98f, BEAM_HALF * 0.88f, 0.85f * f, 0.92f * f);
        }
        (void)perpX;
    }

    void renderTelegraphLabels(float px, float py, float gt) const {
        (void)gt;
        if (laserWarn) {
            float prog = laserWarnT / T_LOCK;
            if (prog > 1.0f) prog = 1.0f;
            wchar_t tw[] = L"▶ RGB TEAR";
            float tw_w = g_TextS.Width(tw, 0.54f);
            g_TextS.Draw(tw, px - tw_w * 0.5f, py - 62.0f, 0.54f,
                         0.35f, 0.95f, 1.0f, 0.78f + prog * 0.2f);
        }
        if (state == BossState::SYNC) {
            wchar_t sw[] = L"◆ CORE EXPOSED";
            float sw_w = g_TextS.Width(sw, 0.5f);
            g_TextS.Draw(sw, coreWorldX() - sw_w * 0.5f, coreWorldY() - BODY - 34.0f, 0.5f,
                         0.95f, 0.35f, 0.72f, 0.92f);
        }
        if (attackBanner > 0.04f) {
            wchar_t bw[] = L"TEAR";
            float sc = 1.1f + attackBanner * 0.3f;
            float bw_w = g_TextS.Width(bw, sc);
            g_TextS.Draw(bw, px - bw_w * 0.5f, py - 96.0f, sc,
                         1.0f, 0.15f, 0.4f, attackBanner * 0.9f);
        }
        if (state == BossState::CORRUPT && stateTimer < 1.5f) {
            wchar_t iw[] = L"CORRUPT.dll";
            float iw_w = g_TextS.Width(iw, 0.5f);
            g_TextS.Draw(iw, worldX - iw_w * 0.5f, worldY - BODY - 46.0f, 0.5f,
                         0.95f, 0.25f, 0.55f, 0.88f);
        }
    }

private:
    float scanCd_ = 0.0f;
    float shardCd_ = 0.0f;
    float swapCd_ = 0.0f;
    float driftTX_ = 0.0f, driftTY_ = 0.0f, driftTimer_ = 0.0f;
    float coreOffX_ = 0.0f, coreOffY_ = 0.0f;
    float decoyCountLast_ = 0;

    float scanDuration() const {
        float base = T_SCAN;
        if (phase3) return base * 0.72f;
        if (phase2) return base * 0.85f;
        return base;
    }

    static float segDist(float px, float py, float ax, float ay, float bx, float by) {
        float abx = bx - ax, aby = by - ay;
        float l2 = abx * abx + aby * aby, t = 0.0f;
        if (l2 > 1e-6f) {
            t = ((px - ax) * abx + (py - ay) * aby) / l2;
            if (t < 0.0f) t = 0.0f;
            else if (t > 1.0f) t = 1.0f;
        }
        float cx = ax + abx * t, cy = ay + aby * t;
        float dx = px - cx, dy = py - cy;
        return sqrtf(dx * dx + dy * dy);
    }

    static void drawBeam(float ox, float oy, float dx, float dy, float reach,
                         float cr, float cg, float cb, float halfW,
                         float glowA, float coreA) {
        float ex = ox + dx * reach, ey = oy + dy * reach;
        float px = -dy, py = dx;
        for (int pass = 0; pass < 3; pass++) {
            float th, ca, rr, gg, bb;
            if (pass == 0) {
                th = halfW * 2.8f; ca = glowA * 0.5f;
                rr = cr * 0.4f; gg = cg * 0.4f; bb = cb * 0.4f;
            } else if (pass == 1) {
                th = halfW * 1.4f; ca = glowA * 0.78f;
                rr = cr * 0.85f; gg = cg * 0.85f; bb = cb * 0.85f;
            } else {
                th = halfW * 0.45f; ca = coreA;
                rr = cr; gg = cg; bb = cb;
            }
            float p1x = ox + px * th, p1y = oy + py * th;
            float p2x = ox - px * th, p2y = oy - py * th;
            float p3x = ex + px * th, p3y = ey + py * th;
            float p4x = ex - px * th, p4y = ey - py * th;
            float v[12] = { p1x,p1y,p2x,p2y,p3x,p3y, p2x,p2y,p4x,p4y,p3x,p3y };
            BatchVerts(v, 6, rr, gg, bb, ca);
        }
    }

    static void drawRgbBeam(float ox, float oy, float dx, float dy, float reach,
                            float halfW, float glowA, float coreA, float chroma) {
        float px = -dy, py = dx;
        drawBeam(ox + px * chroma, oy + py * chroma, dx, dy, reach,
                 1.0f, 0.12f, 0.28f, halfW, glowA, coreA);
        drawBeam(ox - px * chroma * 0.6f, oy - py * chroma * 0.6f, dx, dy, reach,
                 0.12f, 0.95f, 0.35f, halfW * 0.92f, glowA * 0.9f, coreA * 0.95f);
        drawBeam(ox - px * chroma * 1.1f, oy - py * chroma * 1.1f, dx, dy, reach,
                 0.4f, 0.18f, 0.98f, halfW * 0.88f, glowA * 0.85f, coreA * 0.9f);
    }

    void drawMonitorCore(float cx, float cy, float gt, bool real, float hpFrac) const {
        float pulse = 0.5f + 0.5f * sinf(gt * 5.5f + cx * 0.015f);
        float a = real ? 1.0f : 0.68f;
        drawRect(cx - BODY * 0.76f, cy - BODY * 0.6f, BODY * 1.52f, BODY * 1.2f,
                 0.04f, 0.05f, 0.09f, 0.94f * a);
        for (int i = 0; i < 5; i++) {
            float sy = cy - BODY * 0.5f + (float)i * BODY * 0.24f;
            drawRect(cx - BODY * 0.7f, sy, BODY * 1.4f, 2.0f,
                     0.15f, 0.82f, 1.0f, 0.2f * a);
        }
        drawCircle(cx + 5.0f, cy, BODY * 0.72f, 1.0f, 0.08f, 0.35f, 0.4f * a);
        drawCircle(cx - 5.0f, cy, BODY * 0.72f, 0.08f, 0.85f, 1.0f, 0.4f * a);
        drawCircle(cx, cy, BODY * 0.68f, 0.07f, 0.06f, 0.1f, 0.92f * a);
        drawNeonBorder(cx - BODY * 0.7f, cy - BODY * 0.7f, BODY * 1.4f, BODY * 1.4f,
                         0.95f, 0.22f, 0.62f);
        drawRect(cx - 11.0f, cy - 3.0f, 22.0f, 6.0f, 0.12f, 0.95f, 1.0f, 0.68f * a);
        if (state == BossState::SYNC && real)
            drawCircle(cx, cy, BODY * 0.38f, 1.0f, 0.45f, 0.75f, 0.35f + pulse * 0.2f);
        if (hpFrac < 1.0f) {
            float bw = BODY * 1.28f;
            drawRect(cx - bw * 0.5f, cy - BODY - 11.0f, bw, 4.0f, 0.1f, 0.1f, 0.14f, 0.85f * a);
            drawRect(cx - bw * 0.5f, cy - BODY - 11.0f, bw * hpFrac, 4.0f,
                     0.95f, 0.25f, 0.55f, 0.9f * a);
        }
    }

    void beginLock(float px, float py) {
        float cx = coreWorldX(), cy = coreWorldY();
        float dx = px - cx, dy = py - cy;
        float d = sqrtf(dx * dx + dy * dy) + 1e-3f;
        beamDirX = dx / d;
        beamDirY = dy / d;
        float pxn = -beamDirY, pyn = beamDirX;
        beam2DirX = beamDirX * 0.93f + pxn * 0.11f;
        beam2DirY = beamDirY * 0.93f + pyn * 0.11f;
        laserWarn = true;
        laserWarnT = 0.0f;
    }

    void enterTear() {
        state = BossState::TEAR;
        stateTimer = 0.0f;
        laserWarn = false;
        laserWarnT = 0.0f;
        beam1 = true;
        beam2 = beam3 = false;
        beam2T = beam3T = 0.0f;
        tearFlash = 1.0f;
        attackBanner = 0.85f;
        textNoise = 0.15f;
        shards.clear();
    }

    void tickScan(float px, float py, float dt) {
        shardCd_ -= dt;
        if (shardCd_ <= 0.0f) {
            shardCd_ = phase2 ? 2.0f : 2.6f;
            spawnShard(px, py);
            if (phase2) spawnShard(px, py);
        }
        while ((int)shards.size() > shardCap()) shards.erase(shards.begin());
    }

    int shardCap() const { return phase3 ? 12 : (phase2 ? 9 : 6); }

    void spawnShard(float tx, float ty) {
        if ((int)shards.size() >= shardCap()) return;
        GlitchShard s;
        float a = (float)(rand() % 628) * 0.01f;
        s.x = worldX + cosf(a) * 32.0f;
        s.y = worldY + sinf(a) * 32.0f;
        s.orbitAng = a;
        s.orbitR = 38.0f + (float)(rand() % 24);
        s.homingDelay = 0.65f + (float)(rand() % 30) * 0.01f;
        s.alive = true;
        (void)tx; (void)ty;
        shards.push_back(s);
    }

    void tickShards(float px, float py, float dt, float& playerHP) {
        for (auto& s : shards) {
            if (!s.alive) continue;
            if (s.homingDelay > 0.0f) {
                s.homingDelay -= dt;
                s.orbitAng += dt * 2.6f;
                s.x = worldX + cosf(s.orbitAng) * s.orbitR;
                s.y = worldY + sinf(s.orbitAng) * s.orbitR * 0.78f;
            } else {
                float dx = px - s.x, dy = py - s.y;
                float d = sqrtf(dx * dx + dy * dy) + 1e-3f;
                s.vx += dx / d * 920.0f * dt;
                s.vy += dy / d * 920.0f * dt;
                s.x += s.vx * dt;
                s.y += s.vy * dt;
            }
            float dx = px - s.x, dy = py - s.y;
            if (dx * dx + dy * dy < 144.0f) {
                playerHP -= 4.0f;
                s.alive = false;
            }
        }
        shards.erase(std::remove_if(shards.begin(), shards.end(),
                      [](const GlitchShard& s) { return !s.alive; }), shards.end());
    }

    void tickBeamDamage(float px, float py, float dt, float& playerHP) {
        if (!beam1 && !beam2 && !beam3) return;
        float reach = (float)(screenW + screenH);
        float hitR = 26.0f;
        float cx = coreWorldX(), cy = coreWorldY();
        if (beam1 && segDist(px, py, cx, cy, cx + beamDirX * reach,
                             cy + beamDirY * reach) < hitR)
            playerHP -= 34.0f * dt;
        if (beam2 && segDist(px, py, cx, cy, cx + beam2DirX * reach,
                             cy + beam2DirY * reach) < hitR - 2.0f)
            playerHP -= 28.0f * dt;
        if (beam3) {
            float pxn = -beamDirY, pyn = beamDirX;
            float d3x = beamDirX * 0.9f - pxn * 0.12f;
            float d3y = beamDirY * 0.9f - pyn * 0.12f;
            if (segDist(px, py, cx, cy, cx + d3x * reach,
                        cy + d3y * reach) < hitR - 3.0f)
                playerHP -= 26.0f * dt;
        }
    }

    void driftBoss(float dt) {
        float margin = BODY + 80.0f;
        driftTimer_ -= dt;
        if (driftTimer_ <= 0.0f) {
            driftTimer_ = 1.4f + (float)(rand() % 80) * 0.01f;
            int rx = screenW - (int)(2.0f * margin); if (rx < 1) rx = 1;
            int ry = screenH - (int)(2.0f * margin); if (ry < 1) ry = 1;
            driftTX_ = margin + (float)(rand() % rx);
            driftTY_ = margin + (float)(rand() % ry);
        }
        float mdx = driftTX_ - worldX, mdy = driftTY_ - worldY;
        float md = sqrtf(mdx * mdx + mdy * mdy);
        if (md > 2.0f) {
            float spd = (state == BossState::SYNC) ? 48.0f : (phase2 ? 95.0f : 78.0f);
            float step = spd * dt;
            if (step > md) step = md;
            worldX += mdx / md * step;
            worldY += mdy / md * step;
        }
    }

    Decoy makeDecoy(int slot) {
        Decoy d;
        float ang = (float)slot * 2.094395f + (float)(rand() % 60) * 0.008f;
        float dist = 90.0f + (float)(rand() % 80);
        d.offX = cosf(ang) * dist;
        d.offY = sinf(ang) * dist * 0.72f;
        clampDecoyOffset(d);
        d.alive = true;
        d.maxHp = d.hp = maxHp * DECOY_HP_SHARE;
        d.winSeed = rand() & 0xFFFF;
        return d;
    }

    void clampDecoyOffset(Decoy& d) const {
        float maxR = (float)std::min(screenW, screenH) * 0.14f;
        if (maxR < 70.0f) maxR = 70.0f;
        float r = sqrtf(d.offX * d.offX + d.offY * d.offY);
        if (r > maxR && r > 1e-3f) {
            d.offX = d.offX / r * maxR;
            d.offY = d.offY / r * maxR;
        }
    }

    void ensureDecoys() {
        int want = phase3 ? 3 : (phase2 ? 2 : 0);
        while ((int)decoys.size() < want) decoys.push_back(makeDecoy((int)decoys.size()));
        while ((int)decoys.size() > want) decoys.pop_back();
        if (want != decoyCountLast_) {
            decoyCountLast_ = want;
            float per = maxHp * DECOY_HP_SHARE;
            for (auto& d : decoys) {
                if (!d.alive) continue;
                d.maxHp = per;
                if (d.hp <= 0.0f || d.hp > per) d.hp = per;
            }
            float coreCap = maxHp - per * (float)want;
            if (coreCap < maxHp * 0.38f) coreCap = maxHp * 0.38f;
            if (hp > coreCap) hp = coreCap;
        }
    }

    void tickDecoys(float dt) {
        for (size_t i = 0; i < decoys.size(); i++) {
            auto& d = decoys[i];
            if (!d.alive) {
                d.respawn -= dt;
                if (d.respawn <= 0.0f) {
                    d = makeDecoy((int)i);
                    d.maxHp = d.hp = maxHp * DECOY_HP_SHARE;
                }
                continue;
            }
            d.driftTimer -= dt;
            if (d.driftTimer <= 0.0f) {
                d.driftTimer = 1.6f;
                float maxR = (float)std::min(screenW, screenH) * 0.14f;
                if (maxR < 70.0f) maxR = 70.0f;
                float a = (float)(rand() % 628) * 0.01f;
                float r = maxR * (0.35f + (float)(rand() % 55) * 0.01f);
                d.driftTX = cosf(a) * r;
                d.driftTY = sinf(a) * r * 0.72f;
            }
            float mdx = d.driftTX - d.offX, mdy = d.driftTY - d.offY;
            float md = sqrtf(mdx * mdx + mdy * mdy);
            if (md > 1.0f) {
                float step = 55.0f * dt;
                if (step > md) step = md;
                d.offX += mdx / md * step;
                d.offY += mdy / md * step;
                clampDecoyOffset(d);
            }
        }
    }

    void tickSwap(float dt) {
        if (decoys.empty()) return;
        swapCd_ -= dt;
        if (swapCd_ <= 0.0f) {
            swapCd_ = phase3 ? SWAP_INT * 0.65f : SWAP_INT;
            int i = rand() % (int)decoys.size();
            if (!decoys[i].alive) return;
            float tx = decoys[i].offX, ty = decoys[i].offY, th = decoys[i].hp;
            decoys[i].offX = coreOffX_; decoys[i].offY = coreOffY_; decoys[i].hp = hp;
            coreOffX_ = tx; coreOffY_ = ty; hp = th;
            swapFlash = 1.0f;
            glitchAmount = 0.35f;
        }
    }

    void onDecoyDead(Decoy& d) {
        d.alive = false;
        d.respawn = 2.8f;
        d.hp = 0.0f;
        glitchAmount = 0.4f;
        textNoise = 0.55f;
        syncAlive();
    }
};
