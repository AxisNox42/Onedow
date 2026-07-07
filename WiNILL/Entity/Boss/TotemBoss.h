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

// ADUN.relay — 열린 격납 프레임(코어) + 유기적 무리(flock) 스웜
//   + 유인·저격 페어 협공 + 착탄식 야마토 낙하 + 회전-플래시 십자 표적
class TotemBoss {
public:
    struct Interceptor {
        float x = 0.0f, y = 0.0f;
        float vx = 0.0f, vy = 0.0f;
        float burstAng = 0.0f;   // 스폰 슬롯 각도 (펄스 시드로도 사용)
        float aimAng = 0.0f;
        float hp = 0.0f, maxHp = 0.0f;
        bool  alive = false;
        float hitFlash = 0.0f;
        float shootCd = 0.0f;    // 솔로(미페어) 낙오 개체용 사격 타이머
        int   pairIdx = -1;      // 협공 짝 인덱스 (-1 = 미페어)
        bool  isLure = false;    // true = 유인(다이브), false = 저격
        float pairCd = 0.0f;     // 유인 기체의 다음 돌진까지 남은 시간
        float divePhase = 0.0f;  // >0 이면 돌진 중 (남은 시간)
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
    float yamTargetX = 0.0f, yamTargetY = 0.0f;

    static constexpr float BODY              = 72.0f;
    static constexpr float INT_HIT           = 14.0f;
    static constexpr float MAP_PAD           = 96.0f;
    static constexpr float INT_HP_RATIO      = 0.05f;
    static constexpr float INT_SPAWN_INT     = 1.0f;
    static constexpr int   MAX_INT_ALIVE     = 26;

    static constexpr float HANGAR_R          = 52.0f;   // 격납 프레임 반경 (스폰 슬롯)
    static constexpr float LAUNCH_SPD_MIN    = 240.0f;

    static constexpr float FLOCK_SEP_R       = 34.0f;
    static constexpr float FLOCK_COH_R       = 170.0f;
    static constexpr float FLOCK_SEP_STR     = 1500.0f;
    static constexpr float FLOCK_ALI_STR     = 2.2f;
    static constexpr float FLOCK_COH_STR     = 2.6f;
    static constexpr float FLOCK_MAX_SPEED   = 480.0f;

    static constexpr float ORBIT_MIN         = 90.0f;
    static constexpr float ORBIT_MAX         = 240.0f;
    static constexpr float ORBIT_PUSH        = 560.0f;
    static constexpr float ORBIT_PULL        = 170.0f;

    static constexpr float PAIR_CYCLE_MIN    = 2.0f;
    static constexpr float DIVE_DUR          = 0.5f;
    static constexpr float DIVE_ACCEL        = 1050.0f;
    static constexpr float DIVE_MAX_SPEED    = 640.0f;

    static constexpr float YAMATO_CHARGE     = 0.42f;
    static constexpr float YAMATO_IMPACT_R   = 92.0f;
    static constexpr float YAMATO_DMG        = 32.0f;

    static constexpr float STRIKE_SPIN_DUR   = 0.22f;
    static constexpr float STRIKE_FLASH_DUR  = 0.08f;

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

    // ── 페어링: 살아있는 미페어 개체를 둘씩 묶어 유인/저격 역할 배정 ──
    void refreshPairs() {
        int n = (int)ints.size();
        for (int i = 0; i < n; i++) {
            auto& ic = ints[i];
            if (!ic.alive) { ic.pairIdx = -1; continue; }
            if (ic.pairIdx >= 0) {
                if (ic.pairIdx >= n || !ints[ic.pairIdx].alive || ints[ic.pairIdx].pairIdx != i)
                    ic.pairIdx = -1;
            }
        }
        int pending = -1;
        for (int i = 0; i < n; i++) {
            auto& ic = ints[i];
            if (!ic.alive || ic.pairIdx >= 0) continue;
            if (pending < 0) { pending = i; continue; }
            ints[pending].pairIdx = i;
            ints[i].pairIdx = pending;
            ints[pending].isLure = true;
            ints[i].isLure = false;
            ints[pending].pairCd = PAIR_CYCLE_MIN * 0.5f + (float)(rand() % 100) * 0.01f;
            ints[i].pairCd = 0.0f;
            ints[pending].divePhase = 0.0f;
            pending = -1;
        }
    }

    void initInterceptor(Interceptor& ic, float px, float py) {
        ic.alive = true;
        ic.maxHp = ic.hp = intHpMax();
        ic.hitFlash = 0.0f;
        ic.pairIdx = -1; ic.isLure = false; ic.pairCd = 0.0f; ic.divePhase = 0.0f;
        ic.shootCd = 1.1f + (float)(rand() % 50) * 0.02f;

        float baseAng = atan2f(py - worldY, px - worldX);
        float slotAng = baseAng + ((float)(rand() % 200 - 100)) * 0.01f;
        ic.burstAng = slotAng;
        ic.x = worldX + cosf(slotAng) * HANGAR_R;
        ic.y = worldY + sinf(slotAng) * HANGAR_R;
        float launchSpd = LAUNCH_SPD_MIN + (float)(rand() % 140);
        ic.vx = cosf(slotAng) * launchSpd;
        ic.vy = sinf(slotAng) * launchSpd;
        ic.aimAng = slotAng;
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

    // ── 무리(flock) 스티어링 + 페어 다이브/사격 ──
    void updateInterceptors(float px, float py, float dt, std::vector<Bullet>& bullets) {
        refreshPairs();

        int n = (int)ints.size();
        for (int i = 0; i < n; i++) {
            auto& a = ints[i];
            if (!a.alive) continue;
            if (a.hitFlash > 0.0f) a.hitFlash -= dt;

            float sepX = 0.0f, sepY = 0.0f; int nSep = 0;
            float aliVX = 0.0f, aliVY = 0.0f; int nAli = 0;
            float cohX = 0.0f, cohY = 0.0f; int nCoh = 0;
            for (int j = 0; j < n; j++) {
                if (j == i) continue;
                auto& b = ints[j];
                if (!b.alive) continue;
                float dx = a.x - b.x, dy = a.y - b.y;
                float d2 = dx * dx + dy * dy;
                if (d2 > FLOCK_COH_R * FLOCK_COH_R) continue;
                nCoh++; cohX += b.x; cohY += b.y;
                nAli++; aliVX += b.vx; aliVY += b.vy;
                if (d2 < FLOCK_SEP_R * FLOCK_SEP_R && d2 > 0.01f) {
                    float d = sqrtf(d2);
                    sepX += dx / d; sepY += dy / d;
                    nSep++;
                }
            }

            float stx = 0.0f, sty = 0.0f;
            if (nSep > 0) { stx += (sepX / nSep) * FLOCK_SEP_STR; sty += (sepY / nSep) * FLOCK_SEP_STR; }
            if (nAli > 0) { stx += (aliVX / nAli - a.vx) * FLOCK_ALI_STR; sty += (aliVY / nAli - a.vy) * FLOCK_ALI_STR; }
            if (nCoh > 0) { stx += (cohX / nCoh - a.x) * FLOCK_COH_STR; sty += (cohY / nCoh - a.y) * FLOCK_COH_STR; }

            float pdx = a.x - px, pdy = a.y - py;
            float pdist = sqrtf(pdx * pdx + pdy * pdy);
            float pnx = 0.0f, pny = 0.0f;
            if (pdist > 1.0f) {
                pnx = pdx / pdist; pny = pdy / pdist;
                if (pdist < ORBIT_MIN)      { stx += pnx * ORBIT_PUSH; sty += pny * ORBIT_PUSH; }
                else if (pdist > ORBIT_MAX) { stx -= pnx * ORBIT_PULL; sty -= pny * ORBIT_PULL; }
            }

            float maxSpd = FLOCK_MAX_SPEED;
            if (a.pairIdx >= 0 && a.isLure && ints[a.pairIdx].alive) {
                if (a.divePhase > 0.0f) {
                    a.divePhase -= dt;
                    if (pdist > 1.0f) { stx -= pnx * DIVE_ACCEL; sty -= pny * DIVE_ACCEL; }
                    maxSpd = DIVE_MAX_SPEED;
                    if (a.divePhase <= 0.0f) {
                        int sIdx = a.pairIdx;
                        if (sIdx >= 0 && sIdx < n && ints[sIdx].alive) {
                            auto& sn = ints[sIdx];
                            float bx = px - sn.x, by = py - sn.y;
                            float bd = sqrtf(bx * bx + by * by);
                            if (bd > 1.0f) {
                                sn.aimAng = atan2f(by, bx);
                                fireDir(bullets, sn.x, sn.y, bx / bd, by / bd,
                                        300.0f + (float)(rand() % 60),
                                        glm::vec3(0.55f, 0.95f, 1.0f), 0.85f);
                            }
                        }
                        a.pairCd = PAIR_CYCLE_MIN + (float)(rand() % 140) * 0.01f;
                    }
                } else {
                    a.pairCd -= dt;
                    if (a.pairCd <= 0.0f) a.divePhase = DIVE_DUR;
                }
            } else if (a.pairIdx < 0) {
                a.shootCd -= dt;
                if (a.shootCd <= 0.0f) {
                    float bx = px - a.x, by = py - a.y;
                    float bd = sqrtf(bx * bx + by * by);
                    if (bd > ORBIT_MIN * 0.6f) {
                        a.aimAng = atan2f(by, bx);
                        fireDir(bullets, a.x, a.y, bx / bd, by / bd,
                                300.0f + (float)(rand() % 60),
                                glm::vec3(0.55f, 0.95f, 1.0f), 0.85f);
                    }
                    a.shootCd = 1.8f + (float)(rand() % 60) * 0.02f;
                }
            }

            a.vx += stx * dt;
            a.vy += sty * dt;
            a.vx *= 0.985f;
            a.vy *= 0.985f;
            float spd = sqrtf(a.vx * a.vx + a.vy * a.vy);
            if (spd > maxSpd) {
                a.vx *= maxSpd / spd;
                a.vy *= maxSpd / spd;
            }
            a.x += a.vx * dt;
            a.y += a.vy * dt;
            if (spd > 6.0f) a.aimAng = atan2f(a.vy, a.vx);
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

    // ── 십자 표적: 등장부터 회전+축소 → 사라지는 순간 잔광 플래시 → 폭발 ──
    void updateStrikes(float px, float py, float dt,
                       float& playerHP, std::vector<Bullet>& bullets) {
        for (auto& s : strikes) {
            if (s.done) continue;
            s.age += dt;
            if (s.age < STRIKE_SPIN_DUR) {
                float u = s.age / STRIKE_SPIN_DUR;
                s.rot += dt * (6.0f + u * 16.0f);
                s.size = 28.0f * (1.0f - u);
            } else if (s.age < STRIKE_SPIN_DUR + STRIKE_FLASH_DUR) {
                // 플래시 구간 — 렌더에서만 처리
            } else if (!s.done) {
                s.done = true;
                explodeStrike(s, px, py, playerHP, bullets);
            }
        }
        strikes.erase(std::remove_if(strikes.begin(), strikes.end(),
            [](const StrikeMark& s) { return s.done && s.age > STRIKE_SPIN_DUR + STRIKE_FLASH_DUR + 0.5f; }),
            strikes.end());
    }

    // ── 야마토: 짧은 예고 후 예고 시점 위치에 즉시 낙하(착탄) ──
    void startYamato(float px, float py) {
        yamPhase = YamPhase::Charge;
        yamTimer = YAMATO_CHARGE;
        yamTargetX = px;
        yamTargetY = py;
    }

    void detonateYamato(float px, float py, float& playerHP, std::vector<Bullet>& bullets) {
        float dx = px - yamTargetX, dy = py - yamTargetY;
        if (dx * dx + dy * dy < YAMATO_IMPACT_R * YAMATO_IMPACT_R)
            HurtPlayer(playerHP, YAMATO_DMG);
        for (int i = 0; i < 14; i++) {
            float a = (float)i * (6.283f / 14.0f);
            fireDir(bullets, yamTargetX, yamTargetY, cosf(a), sinf(a),
                    230.0f + (float)(rand() % 60),
                    glm::vec3(1.0f, 0.42f, 0.15f), 0.8f);
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
        ints[idx].pairIdx = -1;
    }

    void Update(float px, float py, float dt, float& playerHP,
                std::vector<Bullet>& bullets,
                float& /*pullX*/, float& /*pullY*/) {
        if (!alive) return;

        if (hullFlash > 0.0f) hullFlash -= dt;

        updateMovement(px, py, dt);
        updateInterceptors(px, py, dt, bullets);
        updateStrikes(px, py, dt, playerHP, bullets);

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
                detonateYamato(px, py, playerHP, bullets);
                yamPhase = YamPhase::Idle;
                yamCd = 8.5f + (float)(rand() % 45) * 0.05f;
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

    // ── 헐: 열린 격납 프레임(코어) — 배 실루엣 없이 링 + 슬롯 틱 + 회전 애퍼처 ──
    void renderHull(float t) const {
        float pulse = 0.5f + 0.5f * sinf(t * 3.0f);
        float flash = (hullFlash > 0.0f) ? hullFlash / 0.18f : 0.0f;
        float cr = 0.55f + flash * 0.35f;
        float cg = 0.46f + flash * 0.2f;
        float cb = 1.0f;

        renderWireRing(worldX, worldY, HANGAR_R + pulse * 4.0f, 20, 2.2f,
                       cr, cg, cb, 0.9f, t * 0.3f);
        renderWireRing(worldX, worldY, HANGAR_R * 0.62f, 14, 1.6f,
                       cr * 0.75f, cg * 0.75f, cb * 0.75f, 0.7f, -t * 0.5f);

        const int SLOTS = 10;
        for (int i = 0; i < SLOTS; i++) {
            float a = (float)i / (float)SLOTS * 6.283f + t * 0.3f;
            float x0 = worldX + cosf(a) * (HANGAR_R - 4.0f);
            float y0 = worldY + sinf(a) * (HANGAR_R - 4.0f);
            float x1 = worldX + cosf(a) * (HANGAR_R + 10.0f);
            float y1 = worldY + sinf(a) * (HANGAR_R + 10.0f);
            drawWireSeg(x0, y0, x1, y1, 1.6f, cr, cg, cb, 0.55f);
        }

        float ax = t * 0.7f;
        for (int i = 0; i < 3; i++) {
            int j = (i + 1) % 3;
            float a0 = ax + (float)i * 2.094f, a1 = ax + (float)j * 2.094f;
            drawWireSeg(worldX + cosf(a0) * HANGAR_R * 0.34f, worldY + sinf(a0) * HANGAR_R * 0.34f,
                        worldX + cosf(a1) * HANGAR_R * 0.34f, worldY + sinf(a1) * HANGAR_R * 0.34f,
                        1.6f, cr, cg, cb, 0.8f);
        }
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
            drawWireSeg(wx[i], wy[i], wx[j], wy[j], 1.7f, r, g, b, a);
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
            float sz = 12.0f + pulse * 2.5f;
            float rr = 0.45f + flash * 0.45f, gg = 0.92f + flash * 0.08f, bb = 1.0f;
            if (ic.divePhase > 0.0f) {
                rr = 1.0f; gg = 0.55f + flash * 0.3f; bb = 0.32f;
                sz += 3.0f;
            }
            drawCursorShard(ic.x, ic.y, sz, ic.aimAng, rr, gg, bb, 0.98f);
        }
        (void)t;
    }

    void renderStrikesInWin(float wx, float wy, float ww, float wh) const {
        for (const auto& s : strikes) {
            if (s.done) continue;
            if (!inWinPt(s.x, s.y, wx, wy, ww, wh)) continue;
            if (s.age < STRIKE_SPIN_DUR) {
                float u = s.age / STRIKE_SPIN_DUR;
                float a = 0.5f + 0.45f * u;
                drawWireCross(s.x, s.y, s.size, s.rot, 2.0f + u * 1.2f, 1.0f, 0.88f, 0.22f, a);
            } else if (s.age < STRIKE_SPIN_DUR + STRIKE_FLASH_DUR) {
                float u = (s.age - STRIKE_SPIN_DUR) / STRIKE_FLASH_DUR;
                float a = 1.0f - u;
                float r = 6.0f + u * 40.0f;
                drawCircle(s.x, s.y, r, 1.0f, 0.95f, 0.6f, a * 0.8f);
            }
        }
    }

    void renderTelegraphs(float t) const {
        if (yamPhase == YamPhase::Charge) {
            float prog = 1.0f - yamTimer / YAMATO_CHARGE;
            if (prog < 0.0f) prog = 0.0f;
            if (prog > 1.0f) prog = 1.0f;
            float ringR = 58.0f * (1.0f - prog) + 8.0f;
            float glow = 0.4f + 0.5f * prog;
            renderWireRing(yamTargetX, yamTargetY, ringR, 14, 2.0f + prog * 2.0f,
                           1.0f, 0.3f + prog * 0.25f, 0.12f, glow, t * 2.2f);
            drawWireCross(yamTargetX, yamTargetY, 10.0f + prog * 6.0f, t * 3.0f,
                          1.8f, 1.0f, 0.4f, 0.15f, glow * 0.8f);
            wchar_t lbl[] = L"IMPACT";
            float lw = g_TextS.Width(lbl, 0.42f);
            g_TextS.Draw(lbl, yamTargetX - lw * 0.5f, yamTargetY - ringR - 20.0f, 0.42f,
                         1.0f, 0.4f, 0.15f, 0.5f + 0.5f * prog);
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
