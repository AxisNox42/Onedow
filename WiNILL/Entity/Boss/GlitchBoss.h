#pragma once
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include "DrawPrim.h"
#include "TextRenderer.h"

extern TextRenderer g_TextS;

// ─────────────────────────────────────────────────────────────
// CORRUPT.dll — 깨진 디스플레이 / PHANTOM 디코이 보스
//   CORRUPT → FRAGMENT → TEAR → SYNC 루프
// ─────────────────────────────────────────────────────────────

enum class BossState { CORRUPT, FRAGMENT, TEAR, SYNC };

struct MiniTri {
    float x = 0, y = 0;
    float vx = 0, vy = 0;
    float angle = 0;
    bool  homing = false;
    float homingDelay = 0.0f;
    bool  alive = true;
};

class GlitchBoss {
public:
    float worldX, worldY;
    float hp = 9000.0f, maxHp = 9000.0f;
    bool  alive = true;
    bool  exploded = false;
    int   screenW, screenH;

    BossState state = BossState::CORRUPT;
    float stateTimer = 0.0f;

    std::vector<MiniTri> minis;
    float spawnAccum = 0.0f;
    int   spawnEdgeHint = 0;

    bool  laserActive = false;
    bool  laser2Active = false;
    bool  laserWarn = false;
    float laserWarnT = 0.0f;
    float laserDirX = 1.0f, laserDirY = 0.0f;
    float laser2DirX = 0.0f, laser2DirY = 0.0f;
    float laser2Delay = 0.0f;

    bool  phase2 = false;
    bool  phase3 = false;

    float driftTX = 0, driftTY = 0, driftTimer = 0.0f;
    bool  driftInit = false;

    struct Decoy {
        float x = 0, y = 0;
        float hp = 0, maxHp = 0;
        float respawn = 0.0f;
        bool  alive = true;
        float driftTX = 0, driftTY = 0, driftTimer = 0.0f;
        float hopWarn = 0.0f;
        float hopTX = 0, hopTY = 0;
        float atkCd = 0.0f;
        int   atkKind = 0;
        bool  miniLaser = false;
        float miniLaserT = 0.0f;
        float laserDirX = 1.0f, laserDirY = 0.0f;
    };
    std::vector<Decoy> decoys;
    int   decoyCountLast = 0;
    float decoyFlash = 0.0f;
    float swapCd = 0.0f;
    float swapFlash = 0.0f;

    float novaTimer = 0.0f;
    float novaWarn = 0.0f;

    float rollbackCd = 6.0f;
    float prevPx = 0.0f, prevPy = 0.0f;

    float glitchAmount = 0.0f;
    float burstFlash = 0.0f;
    float textNoise = 0.0f;
    float spinAng = 0.0f;

    // 추가 패턴
    float hopCd = 0.0f;
    float hopWarn = 0.0f;
    float hopTX = 0, hopTY = 0;
    float slideCd = 0.0f;
    float slideT = 0.0f;
    float slideVX = 0, slideVY = 0;
    float skewCd = 0.0f;
    float mirrorCd = 0.0f;

    static constexpr float BODY = 44.0f;
    static constexpr float MELEE_NEAR = 105.0f;
    static constexpr float PHANTOM_WIN_W = 340.0f;

    static constexpr float T_CORRUPT = 3.2f;
    static constexpr float T_FRAGMENT = 5.5f;
    static constexpr float T_TEAR = 1.15f;
    static constexpr float T_SYNC = 2.6f;
    static constexpr float LASER_WARN = 1.2f;
    static constexpr float T_LASER1 = 0.95f;
    static constexpr float LASER2_STAGGER = 0.42f;

    static constexpr float DRIFT_SPD = 78.0f;
    static constexpr float MINI_SPEED = 500.0f;
    static constexpr float MINI_BURST_ACCEL = 1350.0f;
    static constexpr float MINI_DMG = 5.5f;
    static constexpr float MINI_HIT_R = 15.0f;
    static constexpr float HOMING_DELAY = 0.48f;
    static constexpr int   MINI_CAP = 28;
    static constexpr int   MINI_CAP_P2 = 34;

    static constexpr float NOVA_INT = 8.0f;
    static constexpr int   NOVA_N = 12;
    static constexpr float NOVA_WARN_T = 0.55f;
    static constexpr float SWAP_INT = 9.0f;
    static constexpr float DECOY_HP_SHARE = 0.10f;

    GlitchBoss(int sw, int sh, float hpInit = 9000.0f) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.85f;
        worldY = sh * 0.15f;
    }

    static const wchar_t* BossName() { return L"CORRUPT.dll"; }

    int miniCap() const { return phase2 ? MINI_CAP_P2 : MINI_CAP; }

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

    void syncAlive() {
        if (hp <= 0.0f) {
            bool any = false;
            for (auto& d : decoys) if (d.alive && d.hp > 0.0f) any = true;
            if (!any) alive = false;
        }
    }

    void hideInCorner() {
        int c = rand() % 4;
        float m = 0.13f;
        worldX = (c & 1) ? screenW * (1.0f - m) : screenW * m;
        worldY = (c & 2) ? screenH * (1.0f - m) : screenH * m;
    }

    void trimMinis() {
        while ((int)minis.size() > miniCap()) minis.erase(minis.begin());
    }

    void spawnMiniAt(float sx, float sy, float tx, float ty) {
        if ((int)minis.size() >= miniCap()) return;
        MiniTri t;
        t.x = sx;
        t.y = sy;
        float dx = tx - sx, dy = ty - sy;
        float d = sqrtf(dx * dx + dy * dy) + 1e-3f;
        float sp = MINI_SPEED * (0.62f + (float)(rand() % 50) * 0.01f);
        t.vx = dx / d * sp;
        t.vy = dy / d * sp;
        t.angle = atan2f(dy, dx);
        minis.push_back(t);
    }

    void spawnMini(float px, float py, float tx, float ty) {
        float m = 55.0f;
        float sx, sy;
        int edge = rand() % 4;
        spawnEdgeHint = edge;
        switch (edge) {
        case 0: sx = (float)(rand() % screenW); sy = -m; break;
        case 1: sx = (float)(rand() % screenW); sy = screenH + m; break;
        case 2: sx = -m; sy = (float)(rand() % screenH); break;
        default: sx = screenW + m; sy = (float)(rand() % screenH); break;
        }
        spawnMiniAt(sx, sy, tx, ty);
    }

    void burstMiniAt(float cx, float cy, int n) {
        for (int i = 0; i < n; i++) {
            if ((int)minis.size() >= miniCap()) break;
            float a = (float)i / (float)n * 6.2831853f + (float)(rand() % 100) * 0.01f;
            MiniTri t;
            t.x = cx;
            t.y = cy;
            float sp = MINI_SPEED * 0.65f;
            t.vx = cosf(a) * sp;
            t.vy = sinf(a) * sp;
            t.angle = a;
            minis.push_back(t);
        }
    }

    void skewBurst(float ox, float oy, float tx, float ty, int n = 5) {
        float base = atan2f(ty - oy, tx - ox);
        for (int i = 0; i < n; i++) {
            if ((int)minis.size() >= miniCap()) break;
            float ha = base + ((float)i - (float)(n - 1) * 0.5f) * 0.22f;
            MiniTri t;
            t.x = ox;
            t.y = oy;
            float sp = MINI_SPEED * 0.72f;
            t.vx = cosf(ha) * sp;
            t.vy = sinf(ha) * sp;
            t.angle = ha;
            minis.push_back(t);
        }
    }

    Decoy makeDecoy() {
        Decoy d;
        float m = 0.16f;
        d.x = screenW * (m + (float)(rand() % 1000) * 0.001f * (1.0f - 2.0f * m));
        d.y = screenH * (m + (float)(rand() % 1000) * 0.001f * (1.0f - 2.0f * m));
        d.alive = true;
        d.respawn = 0.0f;
        d.maxHp = d.hp = maxHp * DECOY_HP_SHARE;
        d.atkKind = rand() % 4;
        return d;
    }

    void splitHpToDecoys() {
        int n = 0;
        for (auto& d : decoys) if (d.alive) n++;
        if (n <= 0) return;
        float per = maxHp * DECOY_HP_SHARE;
        float reserve = per * (float)n;
        for (auto& d : decoys) {
            if (!d.alive) continue;
            d.maxHp = per;
            if (d.hp <= 0.0f || d.hp > per) d.hp = per;
        }
        float coreCap = maxHp - reserve;
        if (coreCap < maxHp * 0.35f) coreCap = maxHp * 0.35f;
        if (hp > coreCap) hp = coreCap;
    }

    void ensureDecoys() {
        int want = phase3 ? 3 : (phase2 ? 2 : 0);
        while ((int)decoys.size() < want) decoys.push_back(makeDecoy());
        while ((int)decoys.size() > want) decoys.pop_back();
        if (want != decoyCountLast) {
            decoyCountLast = want;
            splitHpToDecoys();
        }
    }

    void pickDriftTarget(float& tx, float& ty, float margin) const {
        int rx = screenW - (int)(2.0f * margin); if (rx < 1) rx = 1;
        int ry = screenH - (int)(2.0f * margin); if (ry < 1) ry = 1;
        tx = margin + (float)(rand() % rx);
        ty = margin + (float)(rand() % ry);
    }

    void driftEntity(float& x, float& y, float& dtx, float& dty, float& dtimer,
                     float spd, float dt) {
        float dmarg = BODY + 72.0f;
        dtimer -= dt;
        if (dtimer <= 0.0f) {
            dtimer = 1.2f + (float)(rand() % 100) * 0.012f;
            pickDriftTarget(dtx, dty, dmarg);
        }
        float mdx = dtx - x, mdy = dty - y;
        float md = sqrtf(mdx * mdx + mdy * mdy);
        if (md > 1.0f) {
            float step = spd * dt;
            if (step > md) step = md;
            x += mdx / md * step;
            y += mdy / md * step;
        }
    }

    void startGlitchHop(float& x, float& y, float& warn, float& htx, float& hty) {
        warn = 0.38f;
        float jump = 130.0f + (float)(rand() % 90);
        float ang = (float)(rand() % 628) * 0.01f;
        htx = x + cosf(ang) * jump;
        hty = y + sinf(ang) * jump;
        float m = BODY + 60.0f;
        if (htx < m) htx = m; if (htx > screenW - m) htx = screenW - m;
        if (hty < m) hty = m; if (hty > screenH - m) hty = screenH - m;
    }

    void tickGlitchHop(float& x, float& y, float& warn, float htx, float hty, float dt) {
        if (warn <= 0.0f) return;
        warn -= dt;
        if (warn <= 0.0f) {
            x = htx;
            y = hty;
            glitchAmount = std::max(glitchAmount, 0.35f);
        }
    }

    void startScreenSlide() {
        slideT = 0.55f;
        int dir = rand() % 4;
        float mag = 165.0f;
        switch (dir) {
        case 0: slideVX = mag;  slideVY = 0; break;
        case 1: slideVX = -mag; slideVY = 0; break;
        case 2: slideVX = 0; slideVY = mag; break;
        default: slideVX = 0; slideVY = -mag; break;
        }
        glitchAmount = std::max(glitchAmount, 0.28f);
        textNoise = std::max(textNoise, 0.45f);
    }

    void tickScreenSlide(float dt) {
        if (slideT <= 0.0f) return;
        slideT -= dt;
        float step = 420.0f * dt;
        worldX += slideVX * dt;
        worldY += slideVY * dt;
        for (auto& d : decoys) {
            if (!d.alive) continue;
            d.x += slideVX * dt;
            d.y += slideVY * dt;
        }
        for (auto& t : minis) {
            t.x += slideVX * dt * 0.35f;
            t.y += slideVY * dt * 0.35f;
        }
        (void)step;
    }

    void swapPhantom() {
        if (decoys.empty()) return;
        int i = rand() % (int)decoys.size();
        if (!decoys[i].alive) return;
        float tx = worldX, ty = worldY;
        float thp = hp;
        worldX = decoys[i].x;
        worldY = decoys[i].y;
        hp = decoys[i].hp;
        decoys[i].x = tx;
        decoys[i].y = ty;
        decoys[i].hp = thp;
        swapFlash = 1.0f;
        glitchAmount = std::max(glitchAmount, 0.45f);
        textNoise = std::max(textNoise, 0.5f);
    }

    void onDecoyDestroyed(Decoy& d) {
        decoyFlash = 1.0f;
        glitchAmount = 0.55f;
        textNoise = 0.75f;
        burstMiniAt(d.x, d.y, 6);
        d.alive = false;
        d.respawn = 2.0f;
        d.hp = 0.0f;
        syncAlive();
    }

    float tryHitDecoy(float bx, float by, float pbx, float pby, float dmg) {
        for (auto& d : decoys) {
            if (!d.alive || d.hp <= 0.0f) continue;
            if (segDist(bx, by, pbx, pby, d.x, d.y) < BODY * 0.75f) {
                float dealt = (dmg < d.hp) ? dmg : d.hp;
                d.hp -= dealt;
                if (d.hp <= 0.0f) onDecoyDestroyed(d);
                return dealt;
            }
        }
        return 0.0f;
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

    void lockLaser(float px, float py) {
        float dx = px - worldX, dy = py - worldY;
        float d = sqrtf(dx * dx + dy * dy) + 1e-3f;
        laserDirX = dx / d;
        laserDirY = dy / d;
        laser2DirX = laserDirX;
        laser2DirY = laserDirY;
    }

    void enterTear() {
        state = BossState::TEAR;
        stateTimer = 0.0f;
        textNoise = 0.85f;
        glitchAmount = 0.42f;
        laserWarn = false;
        laserActive = true;
        laser2Active = false;
        laser2Delay = (phase2 || phase3) ? LASER2_STAGGER : 999.0f;
        for (auto& t : minis) {
            t.homing = true;
            t.homingDelay = HOMING_DELAY;
        }
    }

    const wchar_t* stateTag() const {
        switch (state) {
        case BossState::CORRUPT:   return L"CORRUPT";
        case BossState::FRAGMENT:  return laserWarn ? L"TEAR SOON" : (hopWarn > 0.05f ? L"HOP" : L"FRAGMENT");
        case BossState::TEAR:      return L"TEAR";
        case BossState::SYNC:      return L"SYNC";
        default: return L"?";
        }
    }

    void updateDecoys(float playerCX, float playerCY, float dt) {
        for (auto& d : decoys) {
            if (!d.alive) {
                d.respawn -= dt;
                if (d.respawn <= 0.0f) {
                    d = makeDecoy();
                    splitHpToDecoys();
                }
                continue;
            }
            driftEntity(d.x, d.y, d.driftTX, d.driftTY, d.driftTimer,
                        DRIFT_SPD * 1.15f, dt);

            d.atkCd -= dt;
            if (d.atkCd <= 0.0f) {
                d.atkCd = phase3 ? 2.2f : 3.0f;
                if (d.atkKind == 0) {
                    spawnMiniAt(d.x, d.y, playerCX, playerCY);
                    spawnMiniAt(d.x, d.y, playerCX, playerCY);
                } else if (d.atkKind == 1) {
                    skewBurst(d.x, d.y, playerCX, playerCY, 4);
                } else if (d.atkKind == 2) {
                    startGlitchHop(d.x, d.y, d.hopWarn, d.hopTX, d.hopTY);
                } else {
                    float dx = playerCX - d.x, dy = playerCY - d.y;
                    float dist = sqrtf(dx * dx + dy * dy) + 1e-3f;
                    d.laserDirX = dx / dist;
                    d.laserDirY = dy / dist;
                    d.miniLaser = true;
                    d.miniLaserT = 0.38f;
                }
            }
            tickGlitchHop(d.x, d.y, d.hopWarn, d.hopTX, d.hopTY, dt);
            if (d.miniLaser) d.miniLaserT -= dt;
            if (d.miniLaserT <= 0.0f) d.miniLaser = false;
        }
    }

    void Update(float playerCX, float playerCY, float dt, float& playerHP) {
        if (!alive) return;
        stateTimer += dt;
        spinAng += dt * 2.4f;

        if (!phase2 && combinedHp() <= combinedMaxHp() * 0.5f) phase2 = true;
        if (!phase3 && combinedHp() <= combinedMaxHp() * 0.25f) phase3 = true;

        burstFlash = std::max(0.0f, burstFlash - dt * 3.5f);
        textNoise = std::max(0.0f, textNoise - dt * 1.8f);
        decoyFlash = std::max(0.0f, decoyFlash - dt * 2.2f);
        swapFlash = std::max(0.0f, swapFlash - dt * 2.5f);
        if (novaWarn > 0.0f) novaWarn -= dt;

        hopCd -= dt;
        slideCd -= dt;
        skewCd -= dt;
        mirrorCd -= dt;

        float pdx = playerCX - worldX, pdy = playerCY - worldY;
        float pdist = sqrtf(pdx * pdx + pdy * pdy) + 1e-3f;

        if (phase2 || phase3) {
            ensureDecoys();
            swapCd -= dt;
            if (swapCd <= 0.0f) {
                swapCd = phase3 ? SWAP_INT * 0.65f : SWAP_INT;
                swapPhantom();
            }
            updateDecoys(playerCX, playerCY, dt);
            for (auto& d : decoys) {
                if (!d.alive || !d.miniLaser) continue;
                float reach = 300.0f;
                float ex = d.x + d.laserDirX * reach, ey = d.y + d.laserDirY * reach;
                if (segDist(playerCX, playerCY, d.x, d.y, ex, ey) < 16.0f)
                    playerHP -= 22.0f * dt;
            }
        } else {
            decoys.clear();
            decoyCountLast = 0;
        }

        tickScreenSlide(dt);
        tickGlitchHop(worldX, worldY, hopWarn, hopTX, hopTY, dt);

        driftEntity(worldX, worldY, driftTX, driftTY, driftTimer,
                    DRIFT_SPD * (state == BossState::SYNC ? 0.55f : (phase2 ? 1.25f : 1.0f)), dt);

        if (laserWarn && pdist < MELEE_NEAR) {
            hideInCorner();
            driftInit = false;
            laserWarn = false;
            laserWarnT = 0.0f;
            stateTimer = std::max(0.0f, stateTimer - 0.8f);
        }

        switch (state) {
        case BossState::CORRUPT:
            glitchAmount = 0.28f + 0.22f * sinf(stateTimer * 12.0f);
            textNoise = 0.65f;
            if (stateTimer >= T_CORRUPT) {
                glitchAmount = 0.08f;
                hideInCorner();
                state = BossState::FRAGMENT;
                stateTimer = 0.0f;
            }
            break;

        case BossState::FRAGMENT: {
            glitchAmount = phase2 ? 0.12f : 0.04f;
            spawnAccum += dt;
            if (spawnAccum >= (phase2 ? 0.65f : 0.82f)) {
                spawnAccum = 0.0f;
                int n = phase2 ? 3 : 2;
                for (int i = 0; i < n; i++) spawnMini(playerCX, playerCY, playerCX, playerCY);
            }
            trimMinis();

            if (hopCd <= 0.0f && hopWarn <= 0.0f) {
                hopCd = phase3 ? 3.2f : 4.5f;
                startGlitchHop(worldX, worldY, hopWarn, hopTX, hopTY);
            }

            if (slideCd <= 0.0f && slideT <= 0.0f) {
                slideCd = phase2 ? 10.0f : 13.0f;
                startScreenSlide();
            }

            if (skewCd <= 0.0f) {
                skewCd = phase2 ? 4.5f : 6.0f;
                skewBurst(worldX, worldY, playerCX, playerCY, phase2 ? 7 : 5);
            }

            if (mirrorCd <= 0.0f && !decoys.empty()) {
                mirrorCd = 7.5f;
                for (auto& d : decoys) {
                    if (!d.alive) continue;
                    skewBurst(d.x, d.y, playerCX, playerCY, 3);
                }
            }

            if (!laserWarn && stateTimer >= T_FRAGMENT - LASER_WARN) {
                lockLaser(playerCX, playerCY);
                laserWarn = true;
                laserWarnT = 0.0f;
            }
            if (laserWarn) {
                laserWarnT += dt;
                if (phase2 && laserWarnT > LASER_WARN * 0.45f) {
                    for (auto& d : decoys) {
                        if (!d.alive) continue;
                        float reach = (float)(screenW + screenH);
                        float ex = d.x + laserDirX * reach, ey = d.y + laserDirY * reach;
                        if (segDist(playerCX, playerCY, d.x, d.y, ex, ey) < 20.0f)
                            playerHP -= 8.0f * dt;
                    }
                }
            }

            novaTimer += dt;
            float nInt = phase2 ? NOVA_INT * 0.75f : NOVA_INT;
            if (novaTimer >= nInt - NOVA_WARN_T && novaWarn <= 0.0f) novaWarn = NOVA_WARN_T;
            if (novaTimer >= nInt && !laserWarn) {
                novaTimer = 0.0f;
                novaWarn = 0.0f;
                glitchAmount = std::max(glitchAmount, 0.32f);
                for (int i = 0; i < NOVA_N; i++) {
                    if ((int)minis.size() >= miniCap()) break;
                    float a = (float)i / (float)NOVA_N * 6.2831853f;
                    MiniTri t;
                    t.x = worldX;
                    t.y = worldY;
                    t.vx = cosf(a) * MINI_SPEED * 0.5f;
                    t.vy = sinf(a) * MINI_SPEED * 0.5f;
                    t.angle = a;
                    minis.push_back(t);
                }
            }

            rollbackCd -= dt;
            if (rollbackCd <= 0.0f) {
                rollbackCd = phase2 ? 7.0f : 9.0f;
                for (int i = 0; i < 3; i++)
                    spawnMini(prevPx, prevPy, prevPx, prevPy);
            }

            if (stateTimer >= T_FRAGMENT) enterTear();
            break;
        }

        case BossState::TEAR:
            glitchAmount = 0.38f * (1.0f - stateTimer / T_TEAR);
            if (stateTimer >= T_LASER1) laserActive = false;
            if (laser2Delay < 900.0f) {
                laser2Delay -= dt;
                if (laser2Delay <= 0.0f && !laser2Active) laser2Active = true;
            }
            if (stateTimer >= T_TEAR) {
                state = BossState::SYNC;
                stateTimer = 0.0f;
                laserActive = laser2Active = false;
                for (auto& t : minis) { t.homing = false; t.homingDelay = 0.0f; }
            }
            break;

        case BossState::SYNC:
            glitchAmount = phase2 ? 0.05f : 0.0f;
            textNoise = 0.15f;
            if (stateTimer >= T_SYNC) {
                state = BossState::FRAGMENT;
                stateTimer = 0.0f;
                spawnAccum = 0.0f;
                laserWarn = false;
            }
            break;
        }

        for (auto& t : minis) {
            if (!t.alive) continue;
            if (t.homing) {
                if (t.homingDelay > 0.0f) t.homingDelay -= dt;
                else {
                    float dx = playerCX - t.x, dy = playerCY - t.y;
                    float d = sqrtf(dx * dx + dy * dy) + 1e-3f;
                    t.vx += dx / d * MINI_BURST_ACCEL * dt;
                    t.vy += dy / d * MINI_BURST_ACCEL * dt;
                }
            }
            t.x += t.vx * dt;
            t.y += t.vy * dt;
            t.angle += dt * 8.0f;
            float dx = playerCX - t.x, dy = playerCY - t.y;
            if (dx * dx + dy * dy < MINI_HIT_R * MINI_HIT_R) {
                playerHP -= MINI_DMG;
                t.alive = false;
            }
            if (t.x < -150 || t.x > screenW + 150 || t.y < -150 || t.y > screenH + 150)
                t.alive = false;
        }
        minis.erase(std::remove_if(minis.begin(), minis.end(),
                     [](const MiniTri& t) { return !t.alive; }), minis.end());

        float reach = (float)(screenW + screenH);
        if (laserActive) {
            float ex = worldX + laserDirX * reach, ey = worldY + laserDirY * reach;
            if (segDist(playerCX, playerCY, worldX, worldY, ex, ey) < 28.0f)
                playerHP -= 38.0f * dt;
        }
        if (laser2Active) {
            float ex2 = worldX + laser2DirX * reach, ey2 = worldY + laser2DirY * reach;
            if (segDist(playerCX, playerCY, worldX, worldY, ex2, ey2) < 26.0f)
                playerHP -= 34.0f * dt;
        }

        prevPx = playerCX;
        prevPy = playerCY;
        syncAlive();
    }

    static void drawLaserLine(float ox, float oy, float dx, float dy, float reach,
                              float cr, float cg, float cb, float coreA, float outerA) {
        float ex = ox + dx * reach, ey = oy + dy * reach;
        const int SEG = 20;
        for (int s = 0; s < SEG; s++) {
            if ((s & 1) == 0) continue;
            float u0 = (float)s / (float)SEG, u1 = (float)(s + 1) / (float)SEG;
            float x0 = ox + (ex - ox) * u0, y0 = oy + (ey - oy) * u0;
            float x1 = ox + (ex - ox) * u1, y1 = oy + (ey - oy) * u1;
            drawRect((x0 + x1) * 0.5f - 5.0f, (y0 + y1) * 0.5f - 5.0f,
                     10.0f, 10.0f, cr * 0.4f, cg * 0.4f, cb * 0.4f, outerA);
            drawRect((x0 + x1) * 0.5f - 2.5f, (y0 + y1) * 0.5f - 2.5f,
                     5.0f, 5.0f, cr, cg, cb, coreA);
        }
    }

    void renderTelegraphs(float px, float py, float gt) const {
        float pulse = 0.5f + 0.5f * sinf(gt * 14.0f);

        drawArcRing(worldX, worldY, MELEE_NEAR, 0.3f, 1.0f, 0.78f, 0.18f);

        if (hopWarn > 0.05f) {
            float hp = hopWarn / 0.38f;
            drawCircle(hopTX, hopTY, BODY * 0.9f, 0.95f, 0.25f, 0.65f, 0.15f + hp * 0.35f);
            drawCircle(worldX, worldY, BODY * 1.1f, 1.0f, 0.4f, 0.7f, 0.25f * hp);
        }

        if (slideT > 0.0f) {
            float sa = slideT / 0.55f;
            drawRect(worldX - 80.0f, worldY - 80.0f, 160.0f, 160.0f,
                     0.2f, 0.95f, 1.0f, 0.08f + sa * 0.15f);
        }

        if (novaWarn > 0.0f) {
            float np = novaWarn / NOVA_WARN_T;
            drawCircle(worldX, worldY, BODY * (1.2f + (1.0f - np) * 0.8f),
                       1.0f, 0.35f, 0.65f, 0.15f + np * 0.25f);
            wchar_t nw[] = L"NOVA";
            float nw_w = g_TextS.Width(nw, 0.5f);
            g_TextS.Draw(nw, worldX - nw_w * 0.5f, worldY + BODY + 6.0f, 0.5f,
                         1.0f, 0.4f, 0.7f, 0.8f);
        }

        if (laserWarn) {
            float reach = (float)(screenW + screenH);
            float prog = (LASER_WARN > 0.0f) ? laserWarnT / LASER_WARN : 1.0f;
            if (prog > 1.0f) prog = 1.0f;
            float blink = 0.55f + 0.45f * sinf(gt * 24.0f);
            drawLaserLine(worldX, worldY, laserDirX, laserDirY, reach,
                          0.3f, 0.95f, 1.0f, blink * (0.5f + prog * 0.5f), blink * 0.35f);
            for (auto& d : decoys) {
                if (!d.alive) continue;
                drawLaserLine(d.x, d.y, laserDirX, laserDirY, reach * 0.55f,
                              0.95f, 0.2f, 0.55f, blink * 0.35f, blink * 0.2f);
            }
            drawCircle(px, py, 14.0f + prog * 12.0f, 0.25f, 0.95f, 1.0f, 0.2f + prog * 0.35f);
            wchar_t tw[] = L"TEAR LINE";
            float tw_w = g_TextS.Width(tw, 0.48f);
            g_TextS.Draw(tw, worldX - tw_w * 0.5f, worldY - BODY - 38.0f, 0.48f,
                         0.35f, 0.95f, 1.0f, 0.85f);
            float sec = LASER_WARN - laserWarnT;
            if (sec < 0.0f) sec = 0.0f;
            wchar_t cd[16];
            swprintf_s(cd, L"%.1fs", sec);
            g_TextS.Draw(cd, worldX - 16.0f, worldY - BODY - 54.0f, 0.55f,
                         1.0f, 0.5f, 0.85f, 0.9f);
        }

        if (state == BossState::TEAR) {
            float ha = 0.32f;
            float base = atan2f(py - worldY, px - worldX);
            drawFan(worldX, worldY, base - ha, base + ha, 320.0f,
                    1.0f, 0.25f, 0.35f, 0.2f, 0.55f, 16);
        }

        if (state == BossState::SYNC) {
            drawCircle(worldX, worldY, BODY * 1.35f, 0.95f, 0.25f, 0.65f, 0.12f + pulse * 0.1f);
        }

        const float m = 22.0f;
        if (state == BossState::FRAGMENT && spawnEdgeHint >= 0 && spawnEdgeHint < 4) {
            float flash = 0.25f + 0.2f * pulse;
            switch (spawnEdgeHint) {
            case 0: drawRect(0, 0, (float)screenW, m, 0.95f, 0.2f, 0.55f, flash); break;
            case 1: drawRect(0, (float)screenH - m, (float)screenW, m, 0.95f, 0.2f, 0.55f, flash); break;
            case 2: drawRect(0, 0, m, (float)screenH, 0.95f, 0.2f, 0.55f, flash); break;
            default: drawRect((float)screenW - m, 0, m, (float)screenH, 0.95f, 0.2f, 0.55f, flash); break;
            }
        }
    }

    static void drawArcRing(float cx, float cy, float rad,
                            float r, float g, float b, float a, int n = 28) {
        for (int i = 0; i < n; i++) {
            if ((i & 1) == 0) continue;
            float a0 = (float)i / (float)n * 6.2831853f;
            float a1 = (float)(i + 1) / (float)n * 6.2831853f;
            float x0 = cx + cosf(a0) * rad, y0 = cy + sinf(a0) * rad;
            float x1 = cx + cosf(a1) * rad, y1 = cy + sinf(a1) * rad;
            drawRect((x0 + x1) * 0.5f - 2.0f, (y0 + y1) * 0.5f - 2.0f,
                     4.0f, 4.0f, r, g, b, a);
        }
    }

    static void drawFan(float ox, float oy, float a0, float a1, float len,
                        float r, float g, float b, float fillA, float edgeA, int segs = 20) {
        for (int s = 0; s < segs; s++) {
            float u0 = (float)s / (float)segs, u1 = (float)(s + 1) / (float)segs;
            float aa = a0 + (a1 - a0) * u0, ab = a0 + (a1 - a0) * u1;
            float x0 = ox + cosf(aa) * len, y0 = oy + sinf(aa) * len;
            float x1 = ox + cosf(ab) * len, y1 = oy + sinf(ab) * len;
            for (int layer = 1; layer <= 3; layer++) {
                float t = (float)layer / 3.0f;
                float mx = ox + ((x0 + x1) * 0.5f - ox) * t;
                float my = oy + ((y0 + y1) * 0.5f - oy) * t;
                drawRect(mx - 2.0f, my - 2.0f, 4.0f, 4.0f, r, g, b, fillA * t);
            }
            drawRect(x0 - 2.5f, y0 - 2.5f, 5.0f, 5.0f, r, g, b, edgeA);
            drawRect(x1 - 2.5f, y1 - 2.5f, 5.0f, 5.0f, r, g, b, edgeA);
        }
    }

    void renderMinis(float gt) const {
        for (auto& t : minis) {
            if (!t.alive) continue;
            if (t.homing && t.homingDelay <= 0.0f) {
                drawRect(t.x - 4, t.y - 4, 8, 8, 1.0f, 0.25f, 0.35f, 0.95f);
                drawRect(t.x - 2, t.y - 2, 4, 4, 1.0f, 0.5f, 0.55f, 0.85f);
            } else if (t.homing) {
                float p = t.homingDelay / HOMING_DELAY;
                drawRect(t.x - 3, t.y - 3, 6, 6, 1.0f, 0.55f, 0.2f, 0.5f + (1.0f - p) * 0.4f);
            } else {
                drawRect(t.x - 3, t.y - 2, 6, 4, 0.85f, 0.35f, 0.95f, 0.9f);
                drawRect(t.x - 1.5f, t.y - 1, 3, 2, 0.95f, 0.55f, 1.0f, 0.75f);
            }
        }
    }

    void renderLasers() const {
        float reach = (float)(screenW + screenH);
        if (laserActive)
            drawLaserLine(worldX, worldY, laserDirX, laserDirY, reach,
                          1.0f, 0.25f, 0.75f, 0.92f, 0.45f);
        if (laser2Active)
            drawLaserLine(worldX, worldY, laser2DirX, laser2DirY, reach,
                          0.95f, 0.2f, 0.85f, 0.88f, 0.4f);
        for (auto& d : decoys) {
            if (!d.alive || !d.miniLaser) continue;
            drawLaserLine(d.x, d.y, d.laserDirX, d.laserDirY, 260.0f,
                          0.95f, 0.15f, 0.55f, 0.75f, 0.35f);
        }
    }

    void renderCore(float cx, float cy, float gt, bool real, float alpha, float hpFrac = 1.0f) const {
        float pulse = 0.5f + 0.5f * sinf(gt * 6.0f + cx * 0.02f);
        float a = alpha * (real ? 1.0f : 0.55f);

        drawCircle(cx + 4.0f, cy, BODY * 0.72f, 1.0f, 0.08f, 0.35f, 0.35f * a);
        drawCircle(cx - 4.0f, cy, BODY * 0.72f, 0.08f, 0.85f, 1.0f, 0.35f * a);
        drawCircle(cx, cy, BODY * 0.68f, 0.08f, 0.06f, 0.1f, 0.88f * a);
        drawNeonBorder(cx - BODY * 0.68f, cy - BODY * 0.68f,
                         BODY * 1.36f, BODY * 1.36f, 0.95f, 0.22f, 0.62f);

        for (int i = 0; i < 6; i++) {
            float ang = spinAng + (float)i * 1.047f;
            float px = cx + cosf(ang) * (BODY * 0.55f);
            float py = cy + sinf(ang) * (BODY * 0.42f);
            drawRect(px - 2, py - 2, 4, 4, 0.9f, 0.3f, 0.75f, 0.45f * a + pulse * 0.25f);
        }

        drawRect(cx - 10.0f, cy - 3.0f, 20.0f, 6.0f, 0.12f, 0.95f, 1.0f, 0.65f * a);
        drawRect(cx - 6.0f, cy - 1.5f, 12.0f, 3.0f, 0.95f, 0.25f, 0.55f, 0.85f * a);

        if (!real) {
            wchar_t q[] = L"PHANTOM";
            float qw = g_TextS.Width(q, 0.38f);
            g_TextS.Draw(q, cx - qw * 0.5f, cy + BODY * 0.28f, 0.38f, 1.0f, 0.4f, 0.65f, 0.7f * a);
        }

        if (hpFrac < 1.0f) {
            float bw = BODY * 1.4f;
            drawRect(cx - bw * 0.5f, cy - BODY - 14.0f, bw, 5.0f, 0.12f, 0.1f, 0.14f, 0.85f * a);
            drawRect(cx - bw * 0.5f, cy - BODY - 14.0f, bw * hpFrac, 5.0f, 0.95f, 0.25f, 0.55f, 0.9f * a);
        }
    }

    void renderDecoyBody(const Decoy& d, float gt) const {
        if (!d.alive) return;
        float hf = (d.maxHp > 0.0f) ? d.hp / d.maxHp : 0.0f;
        if (d.hopWarn > 0.05f) {
            float hp = d.hopWarn / 0.38f;
            drawCircle(d.hopTX, d.hopTY, BODY * 0.7f, 0.95f, 0.2f, 0.55f, 0.2f * hp);
        }
        renderCore(d.x, d.y, gt, false, 0.9f, hf);
    }

    void renderBody(float gt) const {
        if (swapFlash > 0.0f)
            drawCircle(worldX, worldY, BODY * 1.5f, 1.0f, 0.3f, 0.65f, swapFlash * 0.25f);

        renderCore(worldX, worldY, gt, true, 1.0f, (maxHp > 0.0f) ? hp / maxHp : 1.0f);

        wchar_t tag[32];
        swprintf_s(tag, L"[%ls]", stateTag());
        float tw = g_TextS.Width(tag, 0.46f);
        g_TextS.Draw(tag, worldX - tw * 0.5f, worldY - BODY - 32.0f, 0.46f,
                     0.95f, 0.35f, 0.72f, 0.88f);

        if ((int)minis.size() > 0) {
            wchar_t mb[24];
            swprintf_s(mb, L"FRAG %d", (int)minis.size());
            float mw = g_TextS.Width(mb, 0.42f);
            g_TextS.Draw(mb, worldX - mw * 0.5f, worldY + BODY + 8.0f, 0.42f,
                         0.85f, 0.45f, 0.95f, 0.75f);
        }
    }
};
