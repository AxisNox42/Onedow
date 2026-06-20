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
// TOTEM.sys — 이름 미정 · 토템 봉인 보스
//   · 시작 시 토템 4기 랜덤 배치 + 보스 등장
//   · 토템 생존 중: 보스 무적 / 토템마다 스킬 봉인·스탯 흡수·잡몹 방해
//   · 토템 전멸 → 일정 시간 보스 본체 공격 가능 → 토템 재생성
//   · 보스: 순간이동 + 팽창(블랙홀·총으로 깨는 보호막) → 수축(360° 탄막)
//           + 회전 레이저(가끔 역회전)
//   · 토템 공격 시: 주변 잡몹 소환 + 보스가 체력 낮은 토템 쪽으로 이동
// ─────────────────────────────────────────────────────────────
class TotemBoss {
public:
    enum class TotemKind { SealQ, SealE, SealR, StatDrain, MobSpawn };

    struct Totem {
        float x = 0.0f, y = 0.0f;
        float targetX = 0.0f, targetY = 0.0f;
        int   anchorSlot = 0;
        float hp = 0.0f, maxHp = 0.0f;
        bool  alive = false;
        TotemKind kind = TotemKind::StatDrain;
        float mobSpawnCd = 0.0f;
        float bobPhase = 0.0f;
    };

    float worldX, worldY;
    float hp, maxHp;
    bool  alive = true;
    bool  exploded = false;
    int   screenW, screenH;

    Totem totems[4];
    std::vector<std::pair<float, float>> mobSpawnQueue;

    // ── 공격 FSM ──
    enum class Skill { Idle, Teleport, Expand, Contract, Laser };
    Skill skill = Skill::Idle;
    float skillT = 0.0f;
    float skillCd = 2.5f;

    float expandShield = 0.0f, expandShieldMax = 0.0f;
    float expandPull = 0.0f;          // 0..1 팽창 강도 (렌더·당김)
    float laserAng = 0.0f;
    float laserSpin = 1.35f;          // rad/s
    float laserFlipCd = 0.0f;
    bool  protectTotems = false;
    float vulnTimer = 0.0f;
    float rotateCd = 6.0f;

    static constexpr int   N_TOTEM        = 4;
    static constexpr float BODY           = 62.0f;
    static constexpr float TOTEM_HIT      = 28.0f;
    static constexpr float WIN_W          = 200.0f;
    static constexpr float WIN_H          = 168.0f;
    static constexpr float WIN_TB         = 12.0f;
    static constexpr float ANCHOR_RX      = 0.34f;   // 화면 폭 대비
    static constexpr float ANCHOR_RY      = 0.30f;
    static constexpr float ROTATE_INT     = 7.5f;
    static constexpr float DRIFT_SPD      = 145.0f;
    static constexpr float VULN_WINDOW    = 14.0f;
    static constexpr float TOTEM_HP_BASE  = 420.0f;
    static constexpr float STAT_DRAIN     = 0.22f;   // 토템 1기당 피해 −22%
    static constexpr float EXPAND_T       = 2.8f;
    static constexpr float EXPAND_TEL     = 0.65f;
    static constexpr float CONTRACT_T     = 0.45f;
    static constexpr float LASER_T        = 3.2f;
    static constexpr float TELEPORT_T     = 0.35f;
    static constexpr float LASER_LEN      = 920.0f;
    static constexpr float LASER_HALF     = 14.0f;
    static constexpr int   BURST_N        = 32;
    static constexpr float BURST_SPD      = 480.0f;
    static constexpr float MAP_PAD        = 80.0f;

    TotemBoss(int sw, int sh, float hpInit) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.5f;
        worldY = sh * 0.42f;
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
        return (m < 0.15f) ? 0.15f : m;
    }

    void anchorPos(int slot, float& ax, float& ay) const {
        float cx = screenW * 0.5f;
        float cy = screenH * 0.40f;
        float rx = screenW * ANCHOR_RX;
        float ry = screenH * ANCHOR_RY;
        float pad = WIN_W * 0.65f;
        if (rx < pad + 40.0f) rx = pad + 40.0f;
        if (ry < pad + 40.0f) ry = pad + 40.0f;
        switch (slot & 3) {
        case 0: ax = cx;       ay = cy - ry; break;
        case 1: ax = cx + rx;  ay = cy;       break;
        case 2: ax = cx;       ay = cy + ry;  break;
        default: ax = cx - rx; ay = cy;       break;
        }
        if (ax < pad) ax = pad;
        if (ax > screenW - pad) ax = screenW - pad;
        if (ay < pad + 20.0f) ay = pad + 20.0f;
        if (ay > screenH - pad - 60.0f) ay = screenH - pad - 60.0f;
    }

    void assignAnchors(bool snap) {
        int slots[N_TOTEM] = { 0, 1, 2, 3 };
        for (int i = N_TOTEM - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            int t = slots[i]; slots[i] = slots[j]; slots[j] = t;
        }
        int si = 0;
        for (int i = 0; i < N_TOTEM; i++) {
            if (!totems[i].alive) continue;
            totems[i].anchorSlot = slots[si++];
            anchorPos(totems[i].anchorSlot, totems[i].targetX, totems[i].targetY);
            if (snap) { totems[i].x = totems[i].targetX; totems[i].y = totems[i].targetY; }
        }
    }

    void shuffleAnchors() {
        int aliveIdx[N_TOTEM], na = 0;
        for (int i = 0; i < N_TOTEM; i++)
            if (totems[i].alive) aliveIdx[na++] = i;
        if (na < 2) return;
        int a = aliveIdx[rand() % na], b = aliveIdx[rand() % na];
        while (b == a && na > 1) b = aliveIdx[rand() % na];
        if (a == b) return;
        int tmp = totems[a].anchorSlot;
        totems[a].anchorSlot = totems[b].anchorSlot;
        totems[b].anchorSlot = tmp;
        anchorPos(totems[a].anchorSlot, totems[a].targetX, totems[a].targetY);
        anchorPos(totems[b].anchorSlot, totems[b].targetX, totems[b].targetY);
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
        for (int k = 0; k < N_TOTEM; k++) {
            totems[k].maxHp = totems[k].hp = TOTEM_HP_BASE * (0.85f + (float)(rand() % 30) * 0.01f);
            totems[k].alive = true;
            totems[k].kind = pool[k];
            totems[k].mobSpawnCd = 0.0f;
            totems[k].bobPhase = (float)(rand() % 628) * 0.01f;
        }
        assignAnchors(true);
        vulnTimer = 0.0f;
        protectTotems = false;
        rotateCd = ROTATE_INT;
    }

    void onTotemDamaged(int idx) {
        if (idx < 0 || idx >= N_TOTEM) return;
        Totem& t = totems[idx];
        if (!t.alive) return;
        protectTotems = true;
        t.mobSpawnCd -= 0.35f;
        if (t.mobSpawnCd <= 0.0f) {
            t.mobSpawnCd = 1.1f;
            for (int i = 0; i < 2; i++) {
                float a = (float)(rand() % 628) * 0.01f;
                float d = 60.0f + (float)(rand() % 90);
                mobSpawnQueue.push_back({ t.x + cosf(a) * d, t.y + sinf(a) * d });
            }
        }
    }

    void onTotemKilled(int idx) {
        if (idx < 0 || idx >= N_TOTEM) return;
        totems[idx].alive = false;
        if (aliveTotems() > 0) assignAnchors(false);
        if (aliveTotems() <= 0) {
            vulnTimer = VULN_WINDOW;
            protectTotems = false;
        }
    }

    void fireDir(std::vector<Bullet>& b, float ox, float oy, float dx, float dy,
                 float sp, glm::vec3 col) {
        Bullet bb(ox, oy, ox + dx * 100.0f, oy + dy * 100.0f);
        bb.isEnemy = true; bb.speed = sp; bb.color = col;
        b.push_back(bb);
    }

    void teleportRandom() {
        float m = MAP_PAD + BODY;
        float rx = screenW - 2.0f * m; if (rx < 1.0f) rx = 1.0f;
        float ry = screenH - 2.0f * m; if (ry < 1.0f) ry = 1.0f;
        worldX = m + (float)(rand() % (int)rx);
        worldY = m + (float)(rand() % (int)ry);
    }

    void moveTowardLowestTotem(float dt) {
        int best = -1;
        float bestR = 2.0f;
        for (int i = 0; i < N_TOTEM; i++) {
            if (!totems[i].alive) continue;
            float r = (totems[i].maxHp > 0.0f) ? totems[i].hp / totems[i].maxHp : 0.0f;
            if (r < bestR) { bestR = r; best = i; }
        }
        if (best < 0) return;
        float tx = totems[best].x, ty = totems[best].y;
        float dx = tx - worldX, dy = ty - worldY;
        float d = sqrtf(dx * dx + dy * dy);
        if (d < 1.0f) return;
        float spd = 210.0f * dt;
        if (spd > d) spd = d;
        worldX += dx / d * spd;
        worldY += dy / d * spd;
    }

    void startSkill(Skill s) {
        skill = s;
        skillT = 0.0f;
        if (s == Skill::Expand) {
            expandShieldMax = expandShield = maxHp * 0.18f + 800.0f;
            expandPull = 0.0f;
        }
        if (s == Skill::Laser) {
            laserAng = (float)(rand() % 628) * 0.01f;
            laserSpin = (rand() % 2) ? 1.35f : -1.35f;
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

        // 토템 앵커 교대 — 살아있는 토템끼리 자리 스왑
        if (aliveTotems() >= 2 && vulnTimer <= 0.0f) {
            rotateCd -= dt;
            if (rotateCd <= 0.0f) {
                rotateCd = ROTATE_INT + (float)(rand() % 50) * 0.04f;
                shuffleAnchors();
            }
        }
        for (int i = 0; i < N_TOTEM; i++) {
            if (!totems[i].alive) continue;
            float dx = totems[i].targetX - totems[i].x;
            float dy = totems[i].targetY - totems[i].y;
            float d = sqrtf(dx * dx + dy * dy);
            if (d > 0.5f) {
                float step = DRIFT_SPD * dt;
                if (step > d) step = d;
                totems[i].x += dx / d * step;
                totems[i].y += dy / d * step;
            }
        }

        if (protectTotems && aliveTotems() > 0)
            moveTowardLowestTotem(dt);
        else
            protectTotems = false;

        laserFlipCd -= dt;

        float ddx = px - worldX, ddy = py - worldY;
        if (vulnerable() && ddx * ddx + ddy * ddy < BODY * BODY)
            playerHP -= 12.0f * dt;

        skillCd -= dt;
        skillT += dt;

        switch (skill) {
        case Skill::Idle:
            if (skillCd <= 0.0f) {
                int roll = rand() % 100;
                if (roll < 22)      startSkill(Skill::Teleport);
                else if (roll < 48) startSkill(Skill::Expand);
                else if (roll < 72) startSkill(Skill::Laser);
                else                startSkill(Skill::Contract);
                skillCd = 2.0f + (float)(rand() % 80) * 0.025f;
            }
            break;
        case Skill::Teleport:
            if (skillT >= TELEPORT_T * 0.5f && skillT - dt < TELEPORT_T * 0.5f)
                teleportRandom();
            if (skillT >= TELEPORT_T) { skill = Skill::Idle; skillT = 0.0f; }
            break;
        case Skill::Expand:
            if (skillT < EXPAND_TEL) {
                expandPull = skillT / EXPAND_TEL;
            } else {
                expandPull = 1.0f;
                float dx = worldX - px, dy = worldY - py;
                float d = sqrtf(dx * dx + dy * dy);
                if (d > 40.0f && d < 520.0f) {
                    float pull = 280.0f * dt * (1.0f - d / 520.0f);
                    pullX = dx / d * pull;
                    pullY = dy / d * pull;
                }
                if (skillT >= EXPAND_TEL + EXPAND_T || expandShield <= 0.0f) {
                    startSkill(Skill::Contract);
                }
            }
            break;
        case Skill::Contract:
            if (skillT >= CONTRACT_T * 0.5f && skillT - dt < CONTRACT_T * 0.5f) {
                float off = (float)(rand() % 100) * 0.01f;
                for (int i = 0; i < BURST_N; i++) {
                    float a = off + (float)i / (float)BURST_N * 6.2831853f;
                    fireDir(bullets, worldX, worldY, cosf(a), sinf(a),
                            BURST_SPD, glm::vec3(0.95f, 0.35f, 1.0f));
                }
            }
            expandPull = 0.0f;
            if (skillT >= CONTRACT_T) { skill = Skill::Idle; skillT = 0.0f; }
            break;
        case Skill::Laser:
            laserAng += laserSpin * dt;
            if (laserFlipCd <= 0.0f && (rand() % 180) == 0) {
                laserSpin = -laserSpin;
                laserFlipCd = 1.8f;
            }
            {
                float lx = cosf(laserAng), ly = sinf(laserAng);
                float ex = worldX + lx * LASER_LEN, ey = worldY + ly * LASER_LEN;
                float dist = SegDist(px, py, worldX, worldY, ex, ey);
                if (dist < LASER_HALF + 18.0f) playerHP -= 22.0f * dt;
            }
            if (skillT >= LASER_T) { skill = Skill::Idle; skillT = 0.0f; }
            break;
        }

        for (int i = 0; i < N_TOTEM; i++)
            if (totems[i].alive) totems[i].mobSpawnCd -= dt;
    }

    void damageExpandShield(float dmg) {
        if (skill != Skill::Expand || expandShield <= 0.0f) return;
        expandShield -= dmg;
        if (expandShield < 0.0f) expandShield = 0.0f;
    }

    bool expanding() const { return skill == Skill::Expand && skillT >= EXPAND_TEL; }
    float expandShieldFrac() const {
        if (expandShieldMax <= 0.0f) return 0.0f;
        float f = expandShield / expandShieldMax;
        return (f < 0.0f) ? 0.0f : ((f > 1.0f) ? 1.0f : f);
    }

    static const wchar_t* totemLabel(TotemKind k) {
        switch (k) {
        case TotemKind::SealQ:      return L"Q";
        case TotemKind::SealE:      return L"E";
        case TotemKind::SealR:      return L"R";
        case TotemKind::StatDrain:  return L"DRN";
        case TotemKind::MobSpawn:   return L"SPN";
        default: return L"?";
        }
    }

    static const wchar_t* totemWinTitle(TotemKind k) {
        switch (k) {
        case TotemKind::SealQ:      return L"SEAL.Q";
        case TotemKind::SealE:      return L"SEAL.E";
        case TotemKind::SealR:      return L"SEAL.R";
        case TotemKind::StatDrain:  return L"DRAIN";
        case TotemKind::MobSpawn:   return L"SPAWN";
        default: return L"TOTEM";
        }
    }

    static void totemColor(TotemKind k, float& r, float& g, float& b) {
        r = 0.35f; g = 0.75f; b = 0.95f;
        switch (k) {
        case TotemKind::SealQ:     r = 0.5f; g = 0.8f; b = 1.0f; break;
        case TotemKind::SealE:     r = 1.0f; g = 0.6f; b = 0.2f; break;
        case TotemKind::SealR:     r = 0.4f; g = 0.95f; b = 1.0f; break;
        case TotemKind::StatDrain: r = 0.95f; g = 0.3f; b = 0.55f; break;
        case TotemKind::MobSpawn:  r = 0.55f; g = 0.95f; b = 0.45f; break;
        default: break;
        }
    }

    void renderCore(float t) const {
        float pulse = 0.5f + 0.5f * sinf(t * 3.6f);
        float spin  = t * 0.85f;

        // 외곽 오라 — 보라·마젠타 맥동
        for (int i = 0; i < 3; i++) {
            float rr = BODY * (1.55f + (float)i * 0.22f + pulse * 0.12f);
            float a  = 0.10f - (float)i * 0.025f;
            drawCircle(worldX, worldY, rr, 0.72f, 0.22f, 0.95f, a + pulse * 0.06f);
        }

        // 순간이동 잔상
        if (skill == Skill::Teleport && skillT > TELEPORT_T * 0.35f) {
            float fade = 1.0f - (skillT - TELEPORT_T * 0.35f) / (TELEPORT_T * 0.65f);
            if (fade < 0.0f) fade = 0.0f;
            drawCircle(worldX, worldY, BODY * 1.1f, 0.9f, 0.5f, 1.0f, 0.18f * fade);
            drawRect(worldX - BODY, worldY - BODY, BODY * 2, BODY * 2,
                     0.9f, 0.4f, 1.0f, 0.12f * fade);
        }

        // 회전 룬 링
        const int RUNE = 8;
        for (int i = 0; i < RUNE; i++) {
            float a = spin + (float)i * 6.2831853f / (float)RUNE;
            float rx = worldX + cosf(a) * (BODY * 1.05f);
            float ry = worldY + sinf(a) * (BODY * 1.05f);
            drawRect(rx - 4.0f, ry - 4.0f, 8.0f, 8.0f,
                     0.85f, 0.35f, 1.0f, 0.55f + pulse * 0.3f);
        }

        // 역회전 내환
        for (int i = 0; i < 6; i++) {
            float a = -spin * 1.4f + (float)i * 1.047f;
            float ix = worldX + cosf(a) * (BODY * 0.62f);
            float iy = worldY + sinf(a) * (BODY * 0.62f);
            drawTriangle(ix, iy, 10.0f, 0.55f, 0.15f, 0.95f, 0.65f);
        }

        // 코어 — 어두운 베이스 + 네온 심장
        drawRect(worldX - BODY * 0.92f, worldY - BODY * 0.92f,
                 BODY * 1.84f, BODY * 1.84f, 0.08f, 0.03f, 0.12f, 0.94f);
        drawNeonBorder(worldX - BODY * 0.92f, worldY - BODY * 0.92f,
                       BODY * 1.84f, BODY * 1.84f, 0.75f, 0.25f, 0.98f);
        drawCircle(worldX, worldY, BODY * 0.55f,
                   0.45f + pulse * 0.35f, 0.12f, 0.55f, 0.88f);
        drawCircle(worldX, worldY, BODY * 0.28f,
                   1.0f, 0.55f, 0.95f, 0.75f + pulse * 0.2f);
        // 십자 슬릿 — 글리치 느낌
        float sl = BODY * (0.35f + pulse * 0.08f);
        drawRect(worldX - sl * 0.5f, worldY - 2.5f, sl, 5.0f, 1.0f, 0.9f, 1.0f, 0.7f);
        drawRect(worldX - 2.5f, worldY - sl * 0.5f, 5.0f, sl, 1.0f, 0.9f, 1.0f, 0.7f);

        if (expanding() && expandShield > 0.0f) {
            float sf = expandPull * (0.85f + expandShieldFrac() * 0.35f);
            float rr = BODY * (1.35f + sf * 2.0f);
            drawCircle(worldX, worldY, rr, 0.12f, 0.04f, 0.18f, 0.14f + expandShieldFrac() * 0.22f);
            drawNeonBorder(worldX - rr, worldY - rr, rr * 2, rr * 2, 0.85f, 0.15f, 1.0f);
            // 보호막 arc
            int segs = 16;
            for (int i = 0; i < segs; i++) {
                float u = (float)i / (float)segs * expandShieldFrac();
                float a0 = -spin + u * 6.2831853f;
                float r0 = rr * 0.92f;
                drawRect(worldX + cosf(a0) * r0 - 3, worldY + sinf(a0) * r0 - 3,
                         6, 6, 0.9f, 0.3f, 1.0f, 0.5f);
            }
        }
        if (!vulnerable() && aliveTotems() > 0) {
            float lockW = BODY * 0.9f;
            drawRect(worldX - lockW * 0.5f, worldY - BODY - 20.0f,
                     lockW, 7.0f, 0.95f, 0.2f, 0.35f, 0.85f);
            drawRect(worldX - 5.0f, worldY - BODY - 28.0f, 10.0f, 10.0f,
                     0.95f, 0.25f, 0.35f, 0.9f);
        }
    }

    void renderLinks(float gt) const {
        if (vulnerable() || aliveTotems() <= 0) return;
        float pulse = 0.45f + 0.35f * sinf(gt * 5.0f);
        for (int i = 0; i < N_TOTEM; i++) {
            if (!totems[i].alive) continue;
            float r, g, b; totemColor(totems[i].kind, r, g, b);
            float tx = totems[i].x, ty = totems[i].y;
            const int SEG = 10;
            for (int s = 0; s < SEG; s++) {
                if ((s & 1) == 0) continue;
                float u0 = (float)s / (float)SEG, u1 = (float)(s + 1) / (float)SEG;
                float x0 = worldX + (tx - worldX) * u0, y0 = worldY + (ty - worldY) * u0;
                float x1 = worldX + (tx - worldX) * u1, y1 = worldY + (ty - worldY) * u1;
                drawRect((x0 + x1) * 0.5f - 1.5f, (y0 + y1) * 0.5f - 1.5f,
                         3.0f, 3.0f, r, g, b, pulse * 0.55f);
            }
        }
    }

    void renderTotem(const Totem& tot, float gt) const {
        if (!tot.alive) return;
        float bob = sinf(gt * 2.8f + tot.bobPhase) * 3.0f;
        float cx = tot.x, cy = tot.y + bob;
        float pulse = 0.5f + 0.5f * sinf(gt * 4.0f + tot.bobPhase);
        float r, g, b;
        totemColor(tot.kind, r, g, b);

        // ── 네온 오벨리스크 (창 안에만, 오버플로 없음) ──
        float coreH = 72.0f, coreW = 14.0f;
        float baseW = 52.0f;

        // 바닥 육각 발광
        drawCircle(cx, cy + coreH * 0.38f, baseW * 0.55f,
                   r * 0.15f, g * 0.15f, b * 0.2f, 0.22f + pulse * 0.1f);
        for (int i = 0; i < 6; i++) {
            float a = gt * 0.45f + (float)i * 1.047f;
            float px = cx + cosf(a) * baseW * 0.48f;
            float py = cy + coreH * 0.38f + sinf(a) * baseW * 0.28f;
            drawTriangle(px, py, 7.0f, r, g, b, 0.35f + pulse * 0.25f);
        }

        // 회전 룬 링 (좁게)
        for (int i = 0; i < 5; i++) {
            float a = -gt * 1.1f + (float)i * 1.257f;
            float rx = cx + cosf(a) * 28.0f;
            float ry = cy + sinf(a) * 18.0f;
            drawRect(rx - 2.0f, ry - 2.0f, 4.0f, 4.0f, r, g, b, 0.5f + pulse * 0.35f);
        }

        // 기둥 그림자 + 코어
        drawRect(cx - coreW * 0.5f - 2, cy - coreH * 0.42f, coreW + 4, coreH,
                 0.03f, 0.03f, 0.06f, 0.75f);
        drawRect(cx - coreW * 0.5f, cy - coreH * 0.42f, coreW, coreH,
                 r * 0.25f, g * 0.25f, b * 0.3f, 0.85f);
        drawRect(cx - 2.0f, cy - coreH * 0.38f, 4.0f, coreH * 0.82f,
                 r, g, b, 0.55f + pulse * 0.4f);

        // 상단 크리스탈 캡
        drawTriangle(cx, cy - coreH * 0.48f, 16.0f, r, g, b, 0.9f);
        drawTriangle(cx, cy - coreH * 0.44f, 9.0f, 1.0f, 1.0f, 1.0f, 0.55f);

        // 종류별 글리프 (텍스트 대신 도형)
        switch (tot.kind) {
        case TotemKind::SealQ:
        case TotemKind::SealE:
        case TotemKind::SealR: {
            const wchar_t* ch = totemLabel(tot.kind);
            float sc = 0.62f;
            float tw = g_TextS.Width(ch, sc);
            g_TextS.Draw(ch, cx - tw * 0.5f, cy - 6.0f, sc, r, g, b, 0.95f);
            drawRect(cx - 11.0f, cy + 6.0f, 22.0f, 14.0f, 0.06f, 0.06f, 0.1f, 0.9f);
            drawRect(cx - 7.0f, cy + 2.0f, 14.0f, 10.0f, r, g, b, 0.75f);
            break;
        }
        case TotemKind::StatDrain:
            drawCircle(cx, cy, 9.0f, r, g, b, 0.45f);
            for (int i = 0; i < 3; i++) {
                float a = gt * 2.0f + (float)i * 2.094f;
                drawRect(cx + cosf(a) * 16.0f - 2, cy + sinf(a) * 10.0f - 2,
                         4, 4, r, g, b, 0.8f);
            }
            break;
        case TotemKind::MobSpawn:
            for (int i = 0; i < 3; i++) {
                float ox = cx + ((float)i - 1.0f) * 11.0f;
                drawCircle(ox, cy + 2.0f, 4.5f, r, g, b, 0.65f + pulse * 0.3f);
            }
            break;
        default: break;
        }

        // HP — 기둥 우측 세로 바
        float hf = (tot.maxHp > 0.0f) ? tot.hp / tot.maxHp : 0.0f;
        float barX = cx + coreW * 0.5f + 8.0f;
        float barH = coreH * 0.7f;
        float barY = cy - barH * 0.5f;
        drawRect(barX, barY, 4.0f, barH, 0.1f, 0.1f, 0.14f, 0.85f);
        drawRect(barX, barY + barH * (1.0f - hf), 4.0f, barH * hf, r, g, b, 1.0f);
    }

    void renderLaser() const {
        if (skill != Skill::Laser) return;
        float lx = cosf(laserAng), ly = sinf(laserAng);
        float ex = worldX + lx * LASER_LEN, ey = worldY + ly * LASER_LEN;
        const int N = 24;
        for (int i = 0; i < N; i++) {
            float u0 = (float)i / (float)N;
            float u1 = (float)(i + 1) / (float)N;
            float x0 = worldX + lx * LASER_LEN * u0, y0 = worldY + ly * LASER_LEN * u0;
            float x1 = worldX + lx * LASER_LEN * u1, y1 = worldY + ly * LASER_LEN * u1;
            float w = LASER_HALF * (1.0f - u0 * 0.4f);
            drawRect((x0 + x1) * 0.5f - w, (y0 + y1) * 0.5f - w, w * 2, w * 2,
                     1.0f, 0.25f, 0.85f, 0.35f);
        }
        drawRect(ex - 10, ey - 10, 20, 20, 1.0f, 0.4f, 1.0f, 0.7f);
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
