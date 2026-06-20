#pragma once
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <glm/glm.hpp>
#include "Bullet.h"
#include "DrawPrim.h"
#include "TextRenderer.h"

extern TextRenderer g_TextS;

// ─────────────────────────────────────────────────────────────
// RITE.CORE (TOTEM.sys) — 능력 봉인 의식 보스
//   · 4 기둥이 화면 중심 링 위를 공전 — 절대 겹치지 않는 궤도 배치
//   · 기둥 생존 = 보스 무적 / Q·E·R·DRAIN·SPAWN 디버프
//   · 기둥 전멸 → RITE DOWN(취약) → 웨이브+1 후 재의식
//   · 보스: 위상이동 · 특이점(보호막) · 수축탄 · 회전 레이저 · 의식망(기둥 연결선)
// ─────────────────────────────────────────────────────────────
class TotemBoss {
public:
    enum class TotemKind { SealQ, SealE, SealR, StatDrain, MobSpawn };

    struct Totem {
        float x = 0.0f, y = 0.0f;
        float targetX = 0.0f, targetY = 0.0f;
        int   ringSlot = 0;
        float hp = 0.0f, maxHp = 0.0f;
        bool  alive = false;
        TotemKind kind = TotemKind::StatDrain;
        float mobSpawnCd = 0.0f;
        float bobPhase = 0.0f;
        float hitFlash = 0.0f;
    };

    float worldX, worldY;
    float hp, maxHp;
    bool  alive = true;
    bool  exploded = false;
    int   screenW, screenH;
    int   wave = 1;

    Totem totems[4];
    std::vector<std::pair<float, float>> mobSpawnQueue;

    enum class Skill { Idle, PhaseShift, Singularity, Collapse, OrbitBeam, RitePulse };
    Skill skill = Skill::Idle;
    float skillT = 0.0f;
    float skillCd = 2.2f;

    float expandShield = 0.0f, expandShieldMax = 0.0f;
    float expandPull = 0.0f;
    float laserAng = 0.0f;
    float laserSpin = 1.45f;
    float laserFlipCd = 0.0f;
    bool  protectTotems = false;
    float vulnTimer = 0.0f;
    float shuffleCd = 5.5f;
    float ringAngle = 0.0f;
    float ringSpin = 0.22f;
    float ritePulse = 0.0f;
    struct Ghost { float x, y, life; };
    std::vector<Ghost> ghosts;

    static constexpr int   N_TOTEM     = 4;
    static constexpr float BODY        = 58.0f;
    static constexpr float TOTEM_HIT   = 26.0f;
    static constexpr float WIN_W       = 188.0f;
    static constexpr float WIN_H       = 158.0f;
    static constexpr float WIN_TB      = 11.0f;
    static constexpr float ORBIT_R     = 0.31f;
    static constexpr float SHUFFLE_INT = 5.5f;
    static constexpr float DRIFT_SPD   = 165.0f;
    static constexpr float VULN_BASE   = 11.0f;
    static constexpr float TOTEM_HP0   = 380.0f;
    static constexpr float STAT_DRAIN  = 0.20f;
    static constexpr float EXPAND_T    = 2.6f;
    static constexpr float EXPAND_TEL  = 0.72f;
    static constexpr float COLLAPSE_T  = 0.42f;
    static constexpr float BEAM_T      = 3.0f;
    static constexpr float SHIFT_T     = 0.42f;
    static constexpr float PULSE_T     = 0.55f;
    static constexpr float LASER_LEN   = 880.0f;
    static constexpr float LASER_HALF  = 13.0f;
    static constexpr int   BURST_N     = 36;
    static constexpr float BURST_SPD   = 500.0f;
    static constexpr float MAP_PAD     = 72.0f;

    TotemBoss(int sw, int sh, float hpInit) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.5f;
        worldY = sh * 0.40f;
        ringAngle = (float)(rand() % 628) * 0.01f;
        spawnTotems();
    }

    int aliveTotems() const {
        int n = 0;
        for (int i = 0; i < N_TOTEM; i++) if (totems[i].alive) ++n;
        return n;
    }

    bool vulnerable() const { return vulnTimer > 0.0f; }

    bool isSkillSealed(int slot) const {
        if (slot < 0 || slot >= 3) return false;
        TotemKind want = (slot == 0) ? TotemKind::SealQ
                       : (slot == 1) ? TotemKind::SealE : TotemKind::SealR;
        for (int i = 0; i < N_TOTEM; i++)
            if (totems[i].alive && totems[i].kind == want) return true;
        return false;
    }

    float statDamageMult() const {
        int n = 0;
        for (int i = 0; i < N_TOTEM; i++)
            if (totems[i].alive && totems[i].kind == TotemKind::StatDrain) ++n;
        float m = 1.0f - STAT_DRAIN * (float)n;
        return (m < 0.18f) ? 0.18f : m;
    }

    float orbitRadius() const {
        float r = (screenW < screenH ? screenW : screenH) * ORBIT_R;
        float pad = WIN_W * 0.72f;
        if (r < pad) r = pad;
        return r;
    }

    void ringPos(int slot, float ang, float& ax, float& ay) const {
        float cx = screenW * 0.5f;
        float cy = screenH * 0.40f;
        float R = orbitRadius();
        float a = ang + (float)(slot & 3) * 1.5707963f;
        ax = cx + cosf(a) * R;
        ay = cy + sinf(a) * R * 0.88f;
        float pad = WIN_W * 0.55f;
        if (ax < pad) ax = pad;
        if (ax > screenW - pad) ax = screenW - pad;
        if (ay < pad + 16.0f) ay = pad + 16.0f;
        if (ay > screenH - pad - 72.0f) ay = screenH - pad - 72.0f;
    }

    void assignRingSlots(bool snap) {
        int order[N_TOTEM] = { 0, 1, 2, 3 };
        for (int i = N_TOTEM - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            int t = order[i]; order[i] = order[j]; order[j] = t;
        }
        int si = 0;
        for (int i = 0; i < N_TOTEM; i++) {
            if (!totems[i].alive) continue;
            totems[i].ringSlot = order[si++];
            ringPos(totems[i].ringSlot, ringAngle, totems[i].targetX, totems[i].targetY);
            if (snap) { totems[i].x = totems[i].targetX; totems[i].y = totems[i].targetY; }
        }
    }

    void shuffleRingSlots() {
        int idx[N_TOTEM], na = 0;
        for (int i = 0; i < N_TOTEM; i++) if (totems[i].alive) idx[na++] = i;
        if (na < 2) return;
        int a = idx[rand() % na], b = idx[rand() % na];
        while (b == a) b = idx[rand() % na];
        int tmp = totems[a].ringSlot;
        totems[a].ringSlot = totems[b].ringSlot;
        totems[b].ringSlot = tmp;
        ringPos(totems[a].ringSlot, ringAngle, totems[a].targetX, totems[a].targetY);
        ringPos(totems[b].ringSlot, ringAngle, totems[b].targetX, totems[b].targetY);
        ringSpin = (rand() % 2) ? 0.22f : -0.22f;
    }

    void spawnTotems() {
        TotemKind pool[5] = {
            TotemKind::SealQ, TotemKind::SealE, TotemKind::SealR,
            TotemKind::StatDrain, TotemKind::MobSpawn
        };
        for (int i = 4; i > 1; i--) {
            int j = rand() % i;
            TotemKind t = pool[i]; pool[i] = pool[j]; pool[j] = t;
        }
        float hpMul = 1.0f + (float)(wave - 1) * 0.08f;
        for (int k = 0; k < N_TOTEM; k++) {
            totems[k].maxHp = totems[k].hp = TOTEM_HP0 * hpMul * (0.9f + (float)(rand() % 20) * 0.01f);
            totems[k].alive = true;
            totems[k].kind = pool[k];
            totems[k].mobSpawnCd = 0.0f;
            totems[k].bobPhase = (float)(rand() % 628) * 0.01f;
            totems[k].hitFlash = 0.0f;
        }
        assignRingSlots(true);
        vulnTimer = 0.0f;
        protectTotems = false;
        shuffleCd = SHUFFLE_INT;
    }

    void onTotemDamaged(int idx) {
        if (idx < 0 || idx >= N_TOTEM) return;
        Totem& t = totems[idx];
        if (!t.alive) return;
        protectTotems = true;
        t.hitFlash = 0.18f;
        t.mobSpawnCd -= 0.4f;
        if (t.mobSpawnCd <= 0.0f) {
            t.mobSpawnCd = 1.0f + (float)(rand() % 40) * 0.02f;
            int n = (t.kind == TotemKind::MobSpawn) ? 3 : 1;
            for (int i = 0; i < n; i++) {
                float a = (float)(rand() % 628) * 0.01f;
                float d = 48.0f + (float)(rand() % 70);
                mobSpawnQueue.push_back({ t.x + cosf(a) * d, t.y + sinf(a) * d });
            }
        }
    }

    void onTotemKilled(int idx) {
        if (idx < 0 || idx >= N_TOTEM) return;
        totems[idx].alive = false;
        ritePulse = 0.35f;
        if (aliveTotems() > 0) assignRingSlots(false);
        if (aliveTotems() <= 0) {
            vulnTimer = VULN_BASE + (float)wave * 0.6f;
            if (vulnTimer > 16.0f) vulnTimer = 16.0f;
            protectTotems = false;
            wave++;
        }
    }

    void fireDir(std::vector<Bullet>& b, float ox, float oy, float dx, float dy,
                 float sp, glm::vec3 col) {
        Bullet bb(ox, oy, ox + dx * 100.0f, oy + dy * 100.0f);
        bb.isEnemy = true; bb.speed = sp; bb.color = col;
        b.push_back(bb);
    }

    void phaseShift() {
        ghosts.push_back({ worldX, worldY, 0.35f });
        if ((int)ghosts.size() > 8) ghosts.erase(ghosts.begin());
        float m = MAP_PAD + BODY;
        float rx = screenW - 2.0f * m; if (rx < 1.0f) rx = 1.0f;
        float ry = screenH - 2.0f * m; if (ry < 1.0f) ry = 1.0f;
        worldX = m + (float)(rand() % (int)rx);
        worldY = m + (float)(rand() % (int)ry);
    }

    void moveTowardThreatenedTotem(float dt) {
        int best = -1;
        float bestR = 2.0f;
        for (int i = 0; i < N_TOTEM; i++) {
            if (!totems[i].alive) continue;
            float r = (totems[i].maxHp > 0.0f) ? totems[i].hp / totems[i].maxHp : 0.0f;
            if (r < bestR) { bestR = r; best = i; }
        }
        if (best < 0) return;
        float dx = totems[best].x - worldX, dy = totems[best].y - worldY;
        float d = sqrtf(dx * dx + dy * dy);
        if (d < 1.0f) return;
        float spd = 240.0f * dt;
        if (spd > d) spd = d;
        worldX += dx / d * spd;
        worldY += dy / d * spd;
    }

    void startSkill(Skill s) {
        skill = s;
        skillT = 0.0f;
        if (s == Skill::Singularity) {
            float boost = 1.0f + 0.12f * (float)aliveTotems();
            expandShieldMax = expandShield = (maxHp * 0.15f + 720.0f) * boost;
            expandPull = 0.0f;
        }
        if (s == Skill::OrbitBeam) {
            laserAng = ringAngle + (float)(rand() % 628) * 0.01f;
            laserSpin = (rand() % 2) ? 1.45f : -1.45f;
        }
    }

    void damageRiteWeb(float px, float py, float dt, float& playerHP) {
        if (vulnerable() || aliveTotems() < 2) return;
        int alive[N_TOTEM], na = 0;
        for (int i = 0; i < N_TOTEM; i++) if (totems[i].alive) alive[na++] = i;
        for (int a = 0; a < na; a++) {
            int b = (a + 1) % na;
            float ax = totems[alive[a]].x, ay = totems[alive[a]].y;
            float bx = totems[alive[b]].x, by = totems[alive[b]].y;
            float d = SegDist(px, py, ax, ay, bx, by);
            if (d < 14.0f) playerHP -= 14.0f * dt;
        }
        if (aliveTotems() >= 3) {
            for (int a = 0; a < na; a++)
                for (int b = a + 2; b < na; b++) {
                    float ax = totems[alive[a]].x, ay = totems[alive[a]].y;
                    float bx = totems[alive[b]].x, by = totems[alive[b]].y;
                    float d = SegDist(px, py, ax, ay, bx, by);
                    if (d < 10.0f) playerHP -= 8.0f * dt;
                }
        }
    }

    void Update(float px, float py, float dt, float& playerHP,
                std::vector<Bullet>& bullets,
                float& pullX, float& pullY) {
        if (!alive) return;
        pullX = pullY = 0.0f;

        if (vulnTimer > 0.0f) {
            vulnTimer -= dt;
            if (vulnTimer <= 0.0f) {
                vulnTimer = 0.0f;
                spawnTotems();
            }
        }

        ringAngle += ringSpin * dt;
        if (aliveTotems() >= 2 && !vulnerable()) {
            shuffleCd -= dt;
            if (shuffleCd <= 0.0f) {
                shuffleCd = SHUFFLE_INT + (float)(rand() % 30) * 0.05f;
                shuffleRingSlots();
            }
        }
        for (int i = 0; i < N_TOTEM; i++) {
            if (!totems[i].alive) continue;
            ringPos(totems[i].ringSlot, ringAngle, totems[i].targetX, totems[i].targetY);
            float dx = totems[i].targetX - totems[i].x;
            float dy = totems[i].targetY - totems[i].y;
            float d = sqrtf(dx * dx + dy * dy);
            if (d > 0.5f) {
                float step = DRIFT_SPD * dt;
                if (step > d) step = d;
                totems[i].x += dx / d * step;
                totems[i].y += dy / d * step;
            }
            if (totems[i].hitFlash > 0.0f) totems[i].hitFlash -= dt;
            totems[i].mobSpawnCd -= dt;
        }

        if (protectTotems && aliveTotems() > 0) moveTowardThreatenedTotem(dt);
        else protectTotems = false;

        if (ritePulse > 0.0f) ritePulse -= dt;
        laserFlipCd -= dt;
        for (auto& g : ghosts) g.life -= dt;
        ghosts.erase(std::remove_if(ghosts.begin(), ghosts.end(),
                     [](const Ghost& g) { return g.life <= 0.0f; }), ghosts.end());

        damageRiteWeb(px, py, dt, playerHP);
        if (skill == Skill::RitePulse && ritePulse > 0.4f) {
            float dx = px - worldX, dy = py - worldY;
            if (dx * dx + dy * dy < (BODY * 3.2f) * (BODY * 3.2f))
                playerHP -= 28.0f * dt;
        }

        float ddx = px - worldX, ddy = py - worldY;
        if (vulnerable() && ddx * ddx + ddy * ddy < BODY * BODY)
            playerHP -= 14.0f * dt;

        skillCd -= dt;
        skillT += dt;

        switch (skill) {
        case Skill::Idle:
            if (skillCd <= 0.0f) {
                int roll = rand() % 100;
                int n = aliveTotems();
                if (roll < 18)                    startSkill(Skill::PhaseShift);
                else if (roll < 40)               startSkill(Skill::Singularity);
                else if (roll < 62)               startSkill(Skill::OrbitBeam);
                else if (roll < 78 && n >= 2)     startSkill(Skill::RitePulse);
                else                              startSkill(Skill::Collapse);
                skillCd = 1.9f + (float)(rand() % 60) * 0.03f;
            }
            break;
        case Skill::PhaseShift:
            if (skillT >= SHIFT_T * 0.45f && skillT - dt < SHIFT_T * 0.45f)
                phaseShift();
            if (skillT >= SHIFT_T) { skill = Skill::Idle; skillT = 0.0f; }
            break;
        case Skill::Singularity:
            if (skillT < EXPAND_TEL) {
                expandPull = skillT / EXPAND_TEL;
            } else {
                expandPull = 1.0f;
                float dx = worldX - px, dy = worldY - py;
                float d = sqrtf(dx * dx + dy * dy);
                if (d > 36.0f && d < 500.0f) {
                    float str = 300.0f * dt * (1.0f - d / 500.0f);
                    pullX = dx / d * str;
                    pullY = dy / d * str;
                }
                if (skillT >= EXPAND_TEL + EXPAND_T || expandShield <= 0.0f)
                    startSkill(Skill::Collapse);
            }
            break;
        case Skill::Collapse:
            if (skillT >= COLLAPSE_T * 0.45f && skillT - dt < COLLAPSE_T * 0.45f) {
                float off = (float)(rand() % 100) * 0.01f;
                for (int i = 0; i < BURST_N; i++) {
                    float a = off + (float)i / (float)BURST_N * 6.2831853f;
                    fireDir(bullets, worldX, worldY, cosf(a), sinf(a),
                            BURST_SPD, glm::vec3(0.92f, 0.28f, 1.0f));
                }
            }
            expandPull = 0.0f;
            if (skillT >= COLLAPSE_T) { skill = Skill::Idle; skillT = 0.0f; }
            break;
        case Skill::OrbitBeam:
            laserAng += laserSpin * dt;
            if (laserFlipCd <= 0.0f && (rand() % 140) == 0) {
                laserSpin = -laserSpin;
                laserFlipCd = 1.6f;
            }
            {
                float lx = cosf(laserAng), ly = sinf(laserAng);
                float ex = worldX + lx * LASER_LEN, ey = worldY + ly * LASER_LEN;
                if (SegDist(px, py, worldX, worldY, ex, ey) < LASER_HALF + 16.0f)
                    playerHP -= 24.0f * dt;
            }
            if (skillT >= BEAM_T) { skill = Skill::Idle; skillT = 0.0f; }
            break;
        case Skill::RitePulse:
            if (skillT >= PULSE_T * 0.5f && skillT - dt < PULSE_T * 0.5f)
                ritePulse = 1.0f;
            if (skillT >= PULSE_T) { skill = Skill::Idle; skillT = 0.0f; }
            break;
        }
    }

    void damageExpandShield(float dmg) {
        if (skill != Skill::Singularity || expandShield <= 0.0f) return;
        expandShield -= dmg;
        if (expandShield < 0.0f) expandShield = 0.0f;
    }

    bool expanding() const { return skill == Skill::Singularity && skillT >= EXPAND_TEL; }
    float expandShieldFrac() const {
        if (expandShieldMax <= 0.0f) return 0.0f;
        float f = expandShield / expandShieldMax;
        return (f < 0.0f) ? 0.0f : ((f > 1.0f) ? 1.0f : f);
    }

    static const wchar_t* totemLabel(TotemKind k) {
        switch (k) {
        case TotemKind::SealQ:     return L"Q";
        case TotemKind::SealE:     return L"E";
        case TotemKind::SealR:     return L"R";
        case TotemKind::StatDrain: return L"DR";
        case TotemKind::MobSpawn:  return L"SP";
        default: return L"?";
        }
    }

    static const wchar_t* totemWinTitle(TotemKind k) {
        switch (k) {
        case TotemKind::SealQ:     return L":: BIND.Q";
        case TotemKind::SealE:     return L":: BIND.E";
        case TotemKind::SealR:     return L":: BIND.R";
        case TotemKind::StatDrain: return L":: DRAIN";
        case TotemKind::MobSpawn:  return L":: SPAWN";
        default: return L":: RITE";
        }
    }

    static void totemColor(TotemKind k, float& r, float& g, float& b) {
        switch (k) {
        case TotemKind::SealQ:     r = 0.45f; g = 0.82f; b = 1.0f;  break;
        case TotemKind::SealE:     r = 1.0f;  g = 0.55f; b = 0.18f; break;
        case TotemKind::SealR:     r = 0.35f; g = 0.95f; b = 1.0f;  break;
        case TotemKind::StatDrain: r = 0.95f; g = 0.28f; b = 0.62f; break;
        case TotemKind::MobSpawn:  r = 0.48f; g = 0.98f; b = 0.42f; break;
        default: r = 0.7f; g = 0.4f; b = 0.95f; break;
        }
    }

    void renderOrbitGuide(float gt) const {
        if (vulnerable()) return;
        float cx = screenW * 0.5f, cy = screenH * 0.40f;
        float R = orbitRadius();
        float pulse = 0.5f + 0.5f * sinf(gt * 2.2f);
        const int N = 24;
        for (int i = 0; i < N; i++) {
            if ((i & 1) == 0) continue;
            float a0 = ringAngle + (float)i / (float)N * 6.2831853f;
            float a1 = ringAngle + (float)(i + 1) / (float)N * 6.2831853f;
            float x0 = cx + cosf(a0) * R, y0 = cy + sinf(a0) * R * 0.88f;
            float x1 = cx + cosf(a1) * R, y1 = cy + sinf(a1) * R * 0.88f;
            drawRect((x0 + x1) * 0.5f - 1.0f, (y0 + y1) * 0.5f - 1.0f,
                     2.0f, 2.0f, 0.55f, 0.2f, 0.85f, 0.08f + pulse * 0.06f);
        }
    }

    void renderRiteWeb(float gt) const {
        if (vulnerable() || aliveTotems() < 2) return;
        int alive[N_TOTEM], na = 0;
        for (int i = 0; i < N_TOTEM; i++) if (totems[i].alive) alive[na++] = i;
        float pulse = 0.35f + 0.45f * sinf(gt * 6.0f);
        if (ritePulse > 0.0f) pulse = 0.85f;
        auto drawEdge = [&](int ia, int ib, float alpha) {
            float ax = totems[alive[ia]].x, ay = totems[alive[ia]].y;
            float bx = totems[alive[ib]].x, by = totems[alive[ib]].y;
            const int SEG = 12;
            for (int s = 0; s < SEG; s++) {
                if ((s & 1) == 0) continue;
                float u0 = (float)s / (float)SEG, u1 = (float)(s + 1) / (float)SEG;
                float x0 = ax + (bx - ax) * u0, y0 = ay + (by - ay) * u0;
                float x1 = ax + (bx - ax) * u1, y1 = ay + (by - ay) * u1;
                drawRect((x0 + x1) * 0.5f - 1.5f, (y0 + y1) * 0.5f - 1.5f,
                         3.0f, 3.0f, 0.75f, 0.25f, 0.98f, alpha * pulse);
            }
        };
        for (int a = 0; a < na; a++)
            drawEdge(a, (a + 1) % na, 0.22f);
        if (na >= 3) {
            for (int a = 0; a < na; a++)
                for (int b = a + 2; b < na; b++)
                    drawEdge(a, b, 0.10f);
        }
    }

    void renderCore(float t) const {
        float pulse = 0.5f + 0.5f * sinf(t * 4.0f);
        float spin = t * 1.1f;

        for (auto& g : ghosts) {
            float a = (g.life > 0.0f) ? g.life / 0.35f : 0.0f;
            drawCircle(g.x, g.y, BODY * 0.7f, 0.85f, 0.35f, 1.0f, 0.12f * a);
        }

        if (vulnerable()) {
            drawCircle(worldX, worldY, BODY * 1.2f, 1.0f, 0.35f, 0.55f, 0.15f + pulse * 0.1f);
        }

        for (int i = 0; i < 6; i++) {
            float a = spin + (float)i * 1.047f;
            float hx = worldX + cosf(a) * BODY * 0.95f;
            float hy = worldY + sinf(a) * BODY * 0.95f;
            drawTriangle(hx, hy, 11.0f, 0.65f, 0.18f, 0.95f, 0.5f + pulse * 0.25f);
        }

        drawCircle(worldX, worldY, BODY * 0.72f, 0.08f, 0.04f, 0.14f, 0.9f);
        drawNeonBorder(worldX - BODY * 0.72f, worldY - BODY * 0.72f,
                       BODY * 1.44f, BODY * 1.44f, 0.78f, 0.22f, 0.98f);
        drawCircle(worldX, worldY, BODY * 0.38f,
                   0.5f + pulse * 0.3f, 0.1f, 0.55f, 0.85f);
        drawCircle(worldX, worldY, BODY * 0.16f, 1.0f, 0.92f, 1.0f, 0.7f);

        float tri = BODY * 0.42f;
        for (int i = 0; i < 3; i++) {
            float a = -spin * 1.6f + (float)i * 2.094f;
            float tx = worldX + cosf(a) * tri * 0.55f;
            float ty = worldY + sinf(a) * tri * 0.55f;
            drawTriangle(tx, ty, 9.0f, 1.0f, 0.5f, 0.95f, 0.55f);
        }

        if (expanding() && expandShield > 0.0f) {
            float sf = expandPull * (0.9f + expandShieldFrac() * 0.4f);
            float rr = BODY * (1.25f + sf * 2.2f);
            drawCircle(worldX, worldY, rr, 0.1f, 0.03f, 0.16f, 0.16f + expandShieldFrac() * 0.2f);
            drawNeonBorder(worldX - rr, worldY - rr, rr * 2, rr * 2, 0.9f, 0.15f, 1.0f);
        }

        if (!vulnerable() && aliveTotems() > 0) {
            wchar_t wbuf[16]; swprintf_s(wbuf, L"W%d", wave);
            float ws = 0.42f;
            g_TextS.Draw(wbuf, worldX - 10.0f, worldY - BODY - 22.0f, ws, 0.95f, 0.3f, 0.4f, 0.85f);
        }
    }

    void renderLinks(float gt) const {
        if (vulnerable() || aliveTotems() <= 0) return;
        float pulse = 0.4f + 0.35f * sinf(gt * 5.5f);
        for (int i = 0; i < N_TOTEM; i++) {
            if (!totems[i].alive) continue;
            float r, g, b; totemColor(totems[i].kind, r, g, b);
            float tx = totems[i].x, ty = totems[i].y;
            const int SEG = 8;
            for (int s = 0; s < SEG; s++) {
                if ((s & 1) == 0) continue;
                float u0 = (float)s / (float)SEG, u1 = (float)(s + 1) / (float)SEG;
                float x0 = worldX + (tx - worldX) * u0, y0 = worldY + (ty - worldY) * u0;
                float x1 = worldX + (tx - worldX) * u1, y1 = worldY + (ty - worldY) * u1;
                drawRect((x0 + x1) * 0.5f - 1.0f, (y0 + y1) * 0.5f - 1.0f,
                         2.0f, 2.0f, r, g, b, pulse * 0.45f);
            }
        }
    }

    void renderTotem(const Totem& tot, float gt) const {
        if (!tot.alive) return;
        float bob = sinf(gt * 3.0f + tot.bobPhase) * 2.5f;
        float cx = tot.x, cy = tot.y + bob;
        float pulse = 0.5f + 0.5f * sinf(gt * 4.5f + tot.bobPhase);
        float r, g, b;
        totemColor(tot.kind, r, g, b);
        float flash = (tot.hitFlash > 0.0f) ? tot.hitFlash / 0.18f : 0.0f;

        float plat = 38.0f;
        drawRect(cx - plat * 0.5f, cy + 22.0f, plat, 6.0f, r * 0.12f, g * 0.12f, b * 0.15f, 0.85f);
        drawNeonBorder(cx - plat * 0.5f, cy + 22.0f, plat, 6.0f, r, g, b);

        for (int i = 0; i < 4; i++) {
            float a = gt * 0.9f + (float)i * 1.571f;
            drawRect(cx + cosf(a) * 22.0f - 1.5f, cy + sinf(a) * 14.0f - 1.5f,
                     3.0f, 3.0f, r, g, b, 0.35f + pulse * 0.3f);
        }

        float coreH = 58.0f, coreW = 12.0f;
        drawRect(cx - coreW * 0.5f, cy - coreH * 0.35f, coreW, coreH,
                 r * 0.2f, g * 0.2f, b * 0.25f, 0.92f);
        drawRect(cx - 1.5f, cy - coreH * 0.32f, 3.0f, coreH * 0.78f,
                 r + flash * 0.5f, g + flash * 0.4f, b + flash * 0.3f,
                 0.6f + pulse * 0.35f);
        drawTriangle(cx, cy - coreH * 0.38f, 12.0f, r, g, b, 0.92f);

        switch (tot.kind) {
        case TotemKind::SealQ:
        case TotemKind::SealE:
        case TotemKind::SealR: {
            const wchar_t* ch = totemLabel(tot.kind);
            float sc = 0.58f;
            float tw = g_TextS.Width(ch, sc);
            g_TextS.Draw(ch, cx - tw * 0.5f, cy - 4.0f, sc, 1, 1, 1, 0.92f);
            drawRect(cx - 8.0f, cy + 8.0f, 16.0f, 10.0f, 0.05f, 0.05f, 0.08f, 0.9f);
            drawRect(cx - 5.0f, cy + 10.0f, 10.0f, 6.0f, r, g, b, 0.8f);
            break;
        }
        case TotemKind::StatDrain:
            for (int i = 0; i < 3; i++) {
                float a = gt * 1.8f + (float)i * 2.094f;
                drawRect(cx + cosf(a) * 14.0f - 2, cy + sinf(a) * 9.0f - 2,
                         4, 4, r, g, b, 0.75f);
            }
            break;
        case TotemKind::MobSpawn:
            for (int i = 0; i < 3; i++)
                drawCircle(cx + ((float)i - 1.0f) * 9.0f, cy + 4.0f, 3.5f,
                           r, g, b, 0.55f + pulse * 0.35f);
            break;
        default: break;
        }

        float hf = (tot.maxHp > 0.0f) ? tot.hp / tot.maxHp : 0.0f;
        drawRect(cx - plat * 0.42f, cy + 30.0f, plat * 0.84f, 3.0f, 0.08f, 0.08f, 0.1f, 0.9f);
        drawRect(cx - plat * 0.42f, cy + 30.0f, plat * 0.84f * hf, 3.0f, r, g, b, 1.0f);
    }

    void renderLaser() const {
        if (skill != Skill::OrbitBeam) return;
        float lx = cosf(laserAng), ly = sinf(laserAng);
        float ex = worldX + lx * LASER_LEN, ey = worldY + ly * LASER_LEN;
        const int N = 20;
        for (int i = 0; i < N; i++) {
            float u0 = (float)i / (float)N, u1 = (float)(i + 1) / (float)N;
            float x0 = worldX + lx * LASER_LEN * u0, y0 = worldY + ly * LASER_LEN * u0;
            float x1 = worldX + lx * LASER_LEN * u1, y1 = worldY + ly * LASER_LEN * u1;
            float w = LASER_HALF * (1.0f - u0 * 0.45f);
            drawRect((x0 + x1) * 0.5f - w, (y0 + y1) * 0.5f - w, w * 2, w * 2,
                     0.95f, 0.22f, 0.88f, 0.32f);
        }
    }

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
