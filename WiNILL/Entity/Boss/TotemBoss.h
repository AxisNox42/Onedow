#pragma once
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <glm/glm.hpp>
#include "Bullet.h"
#include "Monster.h"
#include "DrawPrim.h"
#include "TextRenderer.h"
#include "PlayerStats.h"

extern TextRenderer g_TextS;

// HANGAR.sys — 항공 공장 보스 (Mindustry 스타일 생산 라인)
//   · 코어 = 메인 격납고 (항상 피격 가능)
//   · 생산 베이 3개 — 근접 / 원거리 / 완성(연성 실패↓)
//   · 2초마다 1회 조립 시도 — 티어·근/원 랜덤, 연성 실패 확률
//   · 보스 HP 낮을수록 제작 가능 최고 티어 상승
class TotemBoss {
public:
    enum class BayRole { MeleeLine, RangedLine, Finisher };

    struct ProdBay {
        float x = 0.0f, y = 0.0f;
        float hp = 0.0f, maxHp = 0.0f;
        bool  alive = false;
        BayRole role = BayRole::MeleeLine;
        float hitFlash = 0.0f;
        float craftPulse = 0.0f;
    };

    struct SpawnRequest {
        float   x = 0.0f, y = 0.0f;
        MobKind kind = MobKind::NORMAL;
        int     tier = 1;
        float   hpMul = 0.55f;
        float   spdMul = 0.85f;
        float   scale = 1.0f;
    };

    float worldX, worldY;
    float hp, maxHp;
    bool  alive = true;
    bool  exploded = false;
    int   screenW, screenH;

    static constexpr int   N_BAY           = 3;
    ProdBay bays[N_BAY];

    std::vector<SpawnRequest> mobSpawnQueue;

    float craftTimer    = 0.0f;
    float craftFxTimer  = 0.0f;
    float craftFailFx   = 0.0f;
    int   lastCraftTier = 0;
    bool  lastCraftFail = false;
    bool  lastCraftMelee = false;
    int   totalCrafted  = 0;
    int   totalFailed   = 0;

    float skillCd  = 4.0f;
    float strafeT  = 0.0f;
    bool  strafing = false;

    static constexpr float BODY          = 62.0f;
    static constexpr float BAY_HIT       = 30.0f;
    static constexpr float TOTEM_HIT     = BAY_HIT;
    static constexpr float WIN_W         = 200.0f;
    static constexpr float WIN_H         = 168.0f;
    static constexpr float WIN_TB        = 11.0f;
    static constexpr float CRAFT_INT     = 2.0f;
    static constexpr float BAY_HP0       = 400.0f;
    static constexpr float MAP_PAD       = 72.0f;

    TotemBoss(int sw, int sh, float hpInit) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.5f;
        worldY = sh * 0.38f;
        initBays(true);
        craftTimer = 0.4f;
    }

    // ── main.cpp 호환 ──
    static constexpr int N_TOTEM = N_BAY;
    bool vulnerable() const { return false; }
    float vulnTimer = 0.0f;
    int wave = 1;
    bool isSkillSealed(int) const { return false; }
    float statDamageMult() const { return 1.0f; }

    int aliveBays() const {
        int n = 0;
        for (int i = 0; i < N_BAY; i++) if (bays[i].alive) ++n;
        return n;
    }
    int aliveTotems() const { return aliveBays(); }

    bool bayAlive(BayRole r) const {
        for (int i = 0; i < N_BAY; i++)
            if (bays[i].alive && bays[i].role == r) return true;
        return false;
    }

    int maxCraftTier() const {
        float r = (maxHp > 0.0f) ? hp / maxHp : 1.0f;
        if (r > 0.82f) return 2;
        if (r > 0.58f) return 3;
        if (r > 0.34f) return 4;
        return 5;
    }

    static float failChancePct(int tier) {
        switch (tier) {
        case 1: return 8.0f;
        case 2: return 14.0f;
        case 3: return 24.0f;
        case 4: return 36.0f;
        case 5: return 50.0f;
        default: return 20.0f;
        }
    }

    static int meleeRollPct(int tier) {
        switch (tier) {
        case 1: return 72;
        case 2: return 62;
        case 3: return 52;
        case 4: return 45;
        case 5: return 40;
        default: return 50;
        }
    }

    static int rollTier(int maxT) {
        static const int w[6] = { 0, 38, 30, 18, 9, 5 };
        int sum = 0;
        for (int i = 1; i <= maxT; i++) sum += w[i];
        if (sum <= 0) return 1;
        int r = rand() % sum;
        for (int i = 1; i <= maxT; i++) {
            r -= w[i];
            if (r < 0) return i;
        }
        return 1;
    }

    static MobKind kindFor(int tier, bool melee) {
        if (melee) {
            switch (tier) {
            case 1: return MobKind::NORMAL;
            case 2: return MobKind::CHARGER;
            case 3: return MobKind::BRUTE;
            case 4: return MobKind::SPLITTER;
            default: return MobKind::SHIELDED;
            }
        }
        switch (tier) {
        case 1: return MobKind::DDOS;
        case 2: return MobKind::WEAVER;
        case 3: return MobKind::ORBITER;
        case 4: return MobKind::SPAWNER;
        default: return MobKind::BLINKER;
        }
    }

    void initBays(bool fresh = false) {
        const BayRole roles[N_BAY] = {
            BayRole::MeleeLine, BayRole::RangedLine, BayRole::Finisher
        };
        float cx = worldX, cy = worldY;
        float spread = 118.0f;
        for (int i = 0; i < N_BAY; i++) {
            float t = (N_BAY == 1) ? 0.5f : (float)i / (float)(N_BAY - 1);
            if (fresh || bays[i].maxHp <= 0.0f) {
                bays[i].role = roles[i];
                bays[i].maxHp = BAY_HP0 * (roles[i] == BayRole::Finisher ? 1.15f : 1.0f);
                bays[i].hp = bays[i].maxHp;
                bays[i].alive = true;
                bays[i].hitFlash = 0.0f;
                bays[i].craftPulse = 0.0f;
            }
            bays[i].x = cx + (t - 0.5f) * spread * 2.0f;
            bays[i].y = cy + 92.0f;
        }
    }

    void repositionBays() { initBays(false); }

    int pickCraftBay(bool wantMelee) const {
        int idx[N_BAY], n = 0;
        for (int i = 0; i < N_BAY; i++) {
            if (!bays[i].alive) continue;
            if (bays[i].role == BayRole::Finisher) { idx[n++] = i; idx[n++] = i; }
            else if (wantMelee && bays[i].role == BayRole::MeleeLine) idx[n++] = i;
            else if (!wantMelee && bays[i].role == BayRole::RangedLine) idx[n++] = i;
        }
        if (n <= 0) {
            n = 0;
            for (int i = 0; i < N_BAY; i++) if (bays[i].alive) idx[n++] = i;
        }
        if (n <= 0) return -1;
        return idx[rand() % n];
    }

    void attemptCraft() {
        if (aliveBays() <= 0) return;

        int maxT = maxCraftTier();
        int tier = rollTier(maxT);
        bool wantMelee = (rand() % 100) < meleeRollPct(tier);

        if (wantMelee && !bayAlive(BayRole::MeleeLine)) {
            if (bayAlive(BayRole::RangedLine)) wantMelee = false;
            else { registerFail(tier, wantMelee); return; }
        }
        if (!wantMelee && !bayAlive(BayRole::RangedLine)) {
            if (bayAlive(BayRole::MeleeLine)) wantMelee = true;
            else { registerFail(tier, wantMelee); return; }
        }

        float fail = failChancePct(tier);
        if (bayAlive(BayRole::Finisher)) fail *= 0.72f;
        if ((float)(rand() % 1000) < fail * 10.0f) {
            registerFail(tier, wantMelee);
            return;
        }

        int bi = pickCraftBay(wantMelee);
        if (bi < 0) { registerFail(tier, wantMelee); return; }

        ProdBay& bay = bays[bi];
        MobKind kind = kindFor(tier, wantMelee);
        float hpMul  = 0.42f + (float)tier * 0.11f;
        float spdMul = 0.78f + (float)tier * 0.05f;
        float scale  = 0.88f + (float)tier * 0.09f;

        float ang = (float)(rand() % 628) * 0.01f;
        float dist = 36.0f + (float)(rand() % 40);
        SpawnRequest req;
        req.x = bay.x + cosf(ang) * dist;
        req.y = bay.y + sinf(ang) * dist * 0.55f + 28.0f;
        req.kind = kind;
        req.tier = tier;
        req.hpMul = hpMul;
        req.spdMul = spdMul;
        req.scale = scale;
        mobSpawnQueue.push_back(req);

        bay.craftPulse = 0.55f;
        lastCraftTier = tier;
        lastCraftMelee = wantMelee;
        lastCraftFail = false;
        craftFxTimer = 0.45f;
        totalCrafted++;
    }

    void registerFail(int tier, bool melee) {
        lastCraftTier = tier;
        lastCraftMelee = melee;
        lastCraftFail = true;
        craftFailFx = 0.55f;
        totalFailed++;
    }

    void onBayDamaged(int idx) {
        if (idx < 0 || idx >= N_BAY) return;
        if (!bays[idx].alive) return;
        bays[idx].hitFlash = 0.16f;
    }

    void onBayDestroyed(int idx) {
        if (idx < 0 || idx >= N_BAY) return;
        bays[idx].alive = false;
        bays[idx].hp = 0.0f;
    }

    void onTotemDamaged(int idx) { onBayDamaged(idx); }
    void onTotemKilled(int idx)  { onBayDestroyed(idx); }

    void fireDir(std::vector<Bullet>& b, float ox, float oy, float dx, float dy,
                 float sp, glm::vec3 col) {
        Bullet bb(ox, oy, ox + dx * 100.0f, oy + dy * 100.0f);
        bb.isEnemy = true; bb.speed = sp; bb.color = col;
        b.push_back(bb);
    }

    void startStrafe(std::vector<Bullet>& bullets) {
        strafing = true;
        strafeT = 0.0f;
        float base = (float)(rand() % 628) * 0.01f;
        for (int i = 0; i < 14; i++) {
            float a = base + (float)i * 0.42f;
            fireDir(bullets, worldX, worldY, cosf(a), sinf(a),
                    360.0f + (float)(rand() % 80), glm::vec3(1.0f, 0.62f, 0.18f));
        }
    }

    void Update(float px, float py, float dt, float& playerHP,
                std::vector<Bullet>& bullets,
                float& /*pullX*/, float& /*pullY*/) {
        if (!alive) return;

        craftTimer -= dt;
        if (craftTimer <= 0.0f) {
            craftTimer = CRAFT_INT;
            attemptCraft();
        }
        if (craftFxTimer > 0.0f) craftFxTimer -= dt;
        if (craftFailFx > 0.0f) craftFailFx -= dt;

        for (int i = 0; i < N_BAY; i++) {
            if (bays[i].hitFlash > 0.0f) bays[i].hitFlash -= dt;
            if (bays[i].craftPulse > 0.0f) bays[i].craftPulse -= dt;
        }

        float ddx = px - worldX, ddy = py - worldY;
        if (ddx * ddx + ddy * ddy < BODY * BODY * 1.1f)
            HurtPlayer(playerHP, 11.0f * dt);

        skillCd -= dt;
        if (strafing) {
            strafeT += dt;
            if (strafeT >= 0.35f) strafing = false;
        } else if (skillCd <= 0.0f) {
            startStrafe(bullets);
            skillCd = 5.5f + (float)(rand() % 40) * 0.05f;
        }
    }

    static const wchar_t* bayWinTitle(BayRole r) {
        switch (r) {
        case BayRole::MeleeLine:  return L"ASM.melee";
        case BayRole::RangedLine: return L"ASM.range";
        case BayRole::Finisher:   return L"ASM.finish";
        default: return L"ASM.line";
        }
    }

    static const wchar_t* totemWinTitle(BayRole r) { return bayWinTitle(r); }

    static void bayColor(BayRole r, float& cr, float& cg, float& cb) {
        switch (r) {
        case BayRole::MeleeLine:  cr = 1.0f;  cg = 0.55f; cb = 0.15f; break;
        case BayRole::RangedLine: cr = 0.35f; cg = 0.82f; cb = 1.0f;  break;
        case BayRole::Finisher:   cr = 0.95f; cg = 0.82f; cb = 0.22f; break;
        default: cr = 0.7f; cg = 0.7f; cb = 0.7f; break;
        }
    }

    void renderRunway(float gt) const {
        float cx = worldX, cy = worldY + 42.0f;
        float rw = 280.0f, rh = 18.0f;
        drawRect(cx - rw * 0.5f, cy, rw, rh, 0.12f, 0.10f, 0.08f, 0.75f);
        for (int i = -4; i <= 4; i++) {
            float pulse = 0.35f + 0.25f * sinf(gt * 3.0f + (float)i * 0.7f);
            drawRect(cx + (float)i * 28.0f - 4.0f, cy + 4.0f, 8.0f, rh - 8.0f,
                     0.95f, 0.78f, 0.2f, 0.25f + pulse * 0.35f);
        }
    }

    void renderHangarCore(float t) const {
        renderRunway(t);
        float pulse = 0.5f + 0.5f * sinf(t * 3.2f);
        float hw = BODY * 1.15f;
        float hh = BODY * 0.95f;

        drawRect(worldX - hw, worldY - hh * 0.35f, hw * 2.0f, hh * 1.35f,
                 0.10f, 0.08f, 0.06f, 0.92f);
        drawNeonBorder(worldX - hw, worldY - hh * 0.35f, hw * 2.0f, hh * 1.35f,
                       0.95f, 0.62f, 0.15f);

        float doorW = hw * 1.1f, doorH = hh * 0.75f;
        drawRect(worldX - doorW * 0.5f, worldY - doorH * 0.15f, doorW, doorH,
                 0.06f, 0.05f, 0.04f, 0.95f);
        for (int i = 0; i < 5; i++) {
            float lx = worldX - doorW * 0.42f + (float)i * doorW * 0.21f;
            drawRect(lx, worldY - doorH * 0.1f, 3.0f, doorH * 0.85f,
                     0.18f, 0.14f, 0.10f, 0.85f);
        }

        float beacon = 0.55f + pulse * 0.45f;
        drawCircle(worldX, worldY - hh * 0.55f, 7.0f, 1.0f, 0.25f, 0.12f, beacon);
        drawTriangle(worldX, worldY - hh * 0.2f, 16.0f, 0.9f, 0.75f, 0.25f, 0.85f);

        if (craftFxTimer > 0.0f && !lastCraftFail) {
            wchar_t ok[24];
            swprintf_s(ok, L"T%d %ls OK", lastCraftTier, lastCraftMelee ? L"M" : L"R");
            g_TextS.Draw(ok, worldX - 34.0f, worldY - hh - 18.0f, 0.48f,
                         0.4f, 1.0f, 0.55f, craftFxTimer * 1.8f);
        }
        if (craftFailFx > 0.0f) {
            g_TextS.Draw(L"FAIL", worldX - 16.0f, worldY - hh - 18.0f, 0.52f,
                         1.0f, 0.25f, 0.2f, craftFailFx * 1.8f);
            drawCircle(worldX, worldY, 28.0f, 1.0f, 0.2f, 0.12f, craftFailFx * 0.35f);
        }

        int maxT = maxCraftTier();
        wchar_t tierBuf[32];
        swprintf_s(tierBuf, L"MAX T%d", maxT);
        g_TextS.Draw(tierBuf, worldX - 22.0f, worldY + hh * 0.55f, 0.44f,
                     1.0f, 0.72f, 0.22f, 0.88f);
    }

    void renderConveyor(const ProdBay& bay, float gt) const {
        if (!bay.alive) return;
        float cr, cg, cb;
        bayColor(bay.role, cr, cg, cb);
        const int SEG = 6;
        for (int s = 0; s < SEG; s++) {
            float u = fmodf(gt * 1.4f + (float)s * 0.17f, 1.0f);
            float x = bay.x + (worldX - bay.x) * u;
            float y = bay.y + (worldY + 20.0f - bay.y) * u;
            drawRect(x - 2.0f, y - 2.0f, 4.0f, 4.0f, cr, cg, cb, 0.35f);
        }
    }

    void renderBay(const ProdBay& bay, float gt) const {
        if (!bay.alive) return;
        float cr, cg, cb;
        bayColor(bay.role, cr, cg, cb);
        float flash = (bay.hitFlash > 0.0f) ? bay.hitFlash / 0.16f : 0.0f;
        float pulse = 0.5f + 0.5f * sinf(gt * 4.0f);
        if (bay.craftPulse > 0.0f) pulse = 0.85f;

        float bw = 54.0f, bh = 44.0f;
        float bx = bay.x - bw * 0.5f, by = bay.y - bh * 0.5f;
        drawRect(bx, by, bw, bh, cr * 0.12f, cg * 0.12f, cb * 0.12f, 0.92f);
        drawNeonBorder(bx, by, bw, bh, cr + flash * 0.4f, cg + flash * 0.3f, cb + flash * 0.2f);

        float armW = 8.0f, armH = 22.0f;
        drawRect(bx + 8.0f, by - armH + 6.0f, armW, armH, cr * 0.35f, cg * 0.35f, cb * 0.35f, 0.9f);
        drawRect(bx + bw - 8.0f - armW, by - armH + 6.0f, armW, armH,
                 cr * 0.35f, cg * 0.35f, cb * 0.35f, 0.9f);

        drawRect(bx + 10.0f, by + 10.0f, bw - 20.0f, 10.0f,
                 cr, cg, cb, 0.35f + pulse * 0.35f);
        for (int i = 0; i < 3; i++) {
            float lx = bx + 14.0f + (float)i * 14.0f;
            float lit = (maxCraftTier() > i) ? (0.5f + pulse * 0.4f) : 0.12f;
            drawCircle(lx, by + 28.0f, 4.0f, cr, cg, cb, lit);
        }

        float hf = (bay.maxHp > 0.0f) ? bay.hp / bay.maxHp : 0.0f;
        drawRect(bx + 4.0f, by + bh - 5.0f, bw - 8.0f, 3.0f, 0.08f, 0.08f, 0.08f, 0.9f);
        drawRect(bx + 4.0f, by + bh - 5.0f, (bw - 8.0f) * hf, 3.0f, cr, cg, cb, 1.0f);

        renderConveyor(bay, gt);
    }

    void renderTotem(const ProdBay& bay, float gt) const { renderBay(bay, gt); }

    // legacy stubs — 렌더 루프 호환
    void renderOrbitGuide(float) const {}
    void renderRiteWeb(float) const {}
    void renderLinks(float) const {}
    void renderLaser() const {}
    void renderCore(float t) const { renderHangarCore(t); }

    void damageExpandShield(float) {}

private:
    static float SegDist(float px, float py, float ax, float ay, float bx, float by) {
        float abx = bx - ax, aby = by - ay;
        float apx = px - ax, apy = py - ay;
        float ab2 = abx * abx + aby * aby;
        float t = (ab2 > 1e-6f) ? (apx * abx + apy * aby) / ab2 : 0.0f;
        if (t < 0.0f) t = 0.0f; else if (t > 1.0f) t = 1.0f;
        float cx = ax + abx * t, cy = ay + aby * t;
        float dx = px - cx, dy = py - cy;
        return sqrtf(dx * dx + dy * dy);
    }
};
