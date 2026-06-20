#pragma once
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include "DrawPrim.h"
#include "TextRenderer.h"

extern TextRenderer g_TextS;

// CORRUPT.dll — 깨진 디스플레이 / PHANTOM 가짜창 보스
//   CORRUPT → FRAGMENT → TEAR(짧은 RGB tear) → SYNC(약점)

enum class BossState { CORRUPT, FRAGMENT, TEAR, SYNC };

struct GlitchShard {
    float x = 0, y = 0;
    float vx = 0, vy = 0;
    float orbitAng = 0.0f;
    float orbitR = 48.0f;
    bool  homing = false;
    float homingDelay = 0.0f;
    bool  alive = true;
};

struct CorruptPulse {
    float x = 0, y = 0;
    float r = 0.0f;
    float maxR = 180.0f;
    float life = 0.0f;
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

    std::vector<GlitchShard> shards;
    float shardCd = 0.0f;

    bool  laserActive = false;
    bool  laser2Active = false;
    bool  laser3Active = false;
    bool  laserWarn = false;
    float laserWarnT = 0.0f;
    float laserDirX = 1.0f, laserDirY = 0.0f;
    float laser2DirX = 0.0f, laser2DirY = 0.0f;
    float laser2Delay = 0.0f;
    float laser3Delay = 999.0f;

    bool  phase2 = false;
    bool  phase3 = false;

    float driftTX = 0, driftTY = 0, driftTimer = 0.0f;

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
        float barRain = 0.0f;
        float cursorX = 0, cursorY = 0;
        float cursorFlash = 0.0f;
        int   winSeed = 0;
    };
    std::vector<Decoy> decoys;
    int   decoyCountLast = 0;
    float decoyFlash = 0.0f;
    float swapCd = 0.0f;
    float swapFlash = 0.0f;

    std::vector<CorruptPulse> pulses;
    float pulseCd = 0.0f;

    float rollbackCd = 6.0f;
    float prevPx = 0.0f, prevPy = 0.0f;

    float glitchAmount = 0.0f;
    float textNoise = 0.0f;
    float spinAng = 0.0f;

    float hopCd = 0.0f;
    float hopWarn = 0.0f;
    float hopTX = 0, hopTY = 0;
    float slideCd = 0.0f;
    float slideT = 0.0f;
    float slideVX = 0, slideVY = 0;
    float forkCd = 0.0f;

    static constexpr float BODY = 44.0f;
    static constexpr float MELEE_NEAR = 105.0f;
    static constexpr float PHANTOM_WIN_W = 300.0f;
    static constexpr float PHANTOM_WIN_H = 220.0f;
    static constexpr float PHANTOM_WIN_TB = 16.0f;

    static constexpr float T_CORRUPT = 2.8f;
    static constexpr float T_FRAGMENT = 6.0f;
    static constexpr float T_TEAR = 0.95f;
    static constexpr float T_SYNC = 2.8f;
    static constexpr float LASER_WARN = 1.05f;
    static constexpr float T_LASER1 = 0.62f;
    static constexpr float LASER2_STAGGER = 0.38f;

    static constexpr float DRIFT_SPD = 82.0f;
    static constexpr float SHARD_SPEED = 340.0f;
    static constexpr float SHARD_ACCEL = 980.0f;
    static constexpr float SHARD_DMG = 4.5f;
    static constexpr float SHARD_HIT_R = 12.0f;
    static constexpr float HOMING_DELAY = 0.72f;
    static constexpr int   SHARD_CAP = 10;
    static constexpr int   SHARD_CAP_P2 = 14;

    static constexpr float SWAP_INT = 8.5f;
    static constexpr float DECOY_HP_SHARE = 0.10f;

    GlitchBoss(int sw, int sh, float hpInit = 9000.0f) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.85f;
        worldY = sh * 0.15f;
    }

    static const wchar_t* BossName() { return L"CORRUPT.dll"; }

    static void phantomWinTitle(int seed, wchar_t* out, int cap) {
        swprintf_s(out, cap, L"PHANTOM_%04X.exe", (seed & 0xFFFF));
    }

    int shardCap() const { return phase2 ? SHARD_CAP_P2 : SHARD_CAP; }

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

    void trimShards() {
        while ((int)shards.size() > shardCap()) shards.erase(shards.begin());
    }

    void spawnShard(float ox, float oy, float tx, float ty, bool orbitFirst = true) {
        if ((int)shards.size() >= shardCap()) return;
        GlitchShard s;
        float a = (float)(rand() % 628) * 0.01f;
        s.x = ox + cosf(a) * 28.0f;
        s.y = oy + sinf(a) * 28.0f;
        s.orbitAng = a;
        s.orbitR = 36.0f + (float)(rand() % 30);
        if (orbitFirst) {
            s.homing = true;
            s.homingDelay = HOMING_DELAY + (float)(rand() % 40) * 0.01f;
        } else {
            float dx = tx - s.x, dy = ty - s.y;
            float d = sqrtf(dx * dx + dy * dy) + 1e-3f;
            s.vx = dx / d * SHARD_SPEED;
            s.vy = dy / d * SHARD_SPEED;
        }
        shards.push_back(s);
    }

    void spawnShardRing(float tx, float ty, int n) {
        for (int i = 0; i < n; i++) {
            if ((int)shards.size() >= shardCap()) break;
            spawnShard(worldX, worldY, tx, ty, true);
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
        d.atkKind = rand() % 3;
        d.winSeed = rand() & 0xFFFF;
        d.cursorX = d.x + (float)(rand() % 80 - 40);
        d.cursorY = d.y + (float)(rand() % 60 - 30);
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

    void driftEntity(float& x, float& y, float& dtx, float& dty, float& dtimer, float spd, float dt) {
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
        warn = 0.34f;
        float jump = 110.0f + (float)(rand() % 80);
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
            x = htx; y = hty;
            glitchAmount = std::max(glitchAmount, 0.35f);
        }
    }

    void startScreenSlide() {
        slideT = 0.48f;
        int dir = rand() % 4;
        float mag = 140.0f;
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
        worldX += slideVX * dt;
        worldY += slideVY * dt;
        for (auto& d : decoys) {
            if (!d.alive) continue;
            d.x += slideVX * dt;
            d.y += slideVY * dt;
        }
    }

    void emitPulse(float ox, float oy, float maxR) {
        CorruptPulse p;
        p.x = ox; p.y = oy; p.maxR = maxR; p.r = 24.0f; p.life = 0.55f; p.alive = true;
        pulses.push_back(p);
    }

    void swapPhantom() {
        if (decoys.empty()) return;
        int i = rand() % (int)decoys.size();
        if (!decoys[i].alive) return;
        float tx = worldX, ty = worldY, thp = hp;
        worldX = decoys[i].x; worldY = decoys[i].y; hp = decoys[i].hp;
        decoys[i].x = tx; decoys[i].y = ty; decoys[i].hp = thp;
        swapFlash = 1.0f;
        glitchAmount = std::max(glitchAmount, 0.45f);
        textNoise = std::max(textNoise, 0.5f);
    }

    void onDecoyDestroyed(Decoy& d) {
        decoyFlash = 1.0f;
        glitchAmount = 0.55f;
        textNoise = 0.75f;
        emitPulse(d.x, d.y, 120.0f);
        d.alive = false;
        d.respawn = 2.4f;
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
        float pxn = -laserDirY, pyn = laserDirX;
        laser2DirX = laserDirX * 0.92f + pxn * 0.12f;
        laser2DirY = laserDirY * 0.92f + pyn * 0.12f;
    }

    void enterTear() {
        state = BossState::TEAR;
        stateTimer = 0.0f;
        textNoise = 0.85f;
        glitchAmount = 0.42f;
        laserWarn = false;
        laserActive = true;
        laser2Active = laser3Active = false;
        laser2Delay = LASER2_STAGGER;
        laser3Delay = LASER2_STAGGER * 1.55f;
        shards.clear();
    }

    const wchar_t* stateTag() const {
        switch (state) {
        case BossState::CORRUPT:   return L"CORRUPT";
        case BossState::FRAGMENT:  return laserWarn ? L"TEAR SOON" : (hopWarn > 0.05f ? L"HOP" : L"FRAGMENT");
        case BossState::TEAR:      return L"RGB TEAR";
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
            driftEntity(d.x, d.y, d.driftTX, d.driftTY, d.driftTimer, DRIFT_SPD * 1.1f, dt);
            d.atkCd -= dt;
            if (d.atkCd <= 0.0f) {
                d.atkCd = phase3 ? 2.6f : 3.4f;
                if (d.atkKind == 0) {
                    d.barRain = 0.45f;
                } else if (d.atkKind == 1) {
                    startGlitchHop(d.x, d.y, d.hopWarn, d.hopTX, d.hopTY);
                } else {
                    d.cursorFlash = 0.55f;
                    d.cursorX = playerCX + (float)(rand() % 60 - 30);
                    d.cursorY = playerCY + (float)(rand() % 60 - 30);
                }
            }
            if (d.barRain > 0.0f) d.barRain -= dt;
            if (d.cursorFlash > 0.0f) d.cursorFlash -= dt;
            tickGlitchHop(d.x, d.y, d.hopWarn, d.hopTX, d.hopTY, dt);
        }
    }

    void Update(float playerCX, float playerCY, float dt, float& playerHP) {
        if (!alive) return;
        stateTimer += dt;
        spinAng += dt * 2.4f;

        if (!phase2 && combinedHp() <= combinedMaxHp() * 0.5f) phase2 = true;
        if (!phase3 && combinedHp() <= combinedMaxHp() * 0.25f) phase3 = true;

        textNoise = std::max(0.0f, textNoise - dt * 1.8f);
        decoyFlash = std::max(0.0f, decoyFlash - dt * 2.2f);
        swapFlash = std::max(0.0f, swapFlash - dt * 2.5f);

        hopCd -= dt; slideCd -= dt; pulseCd -= dt; shardCd -= dt; forkCd -= dt;

        float pdist = sqrtf((playerCX - worldX) * (playerCX - worldX) +
                            (playerCY - worldY) * (playerCY - worldY)) + 1e-3f;

        if (phase2 || phase3) {
            ensureDecoys();
            swapCd -= dt;
            if (swapCd <= 0.0f) {
                swapCd = phase3 ? SWAP_INT * 0.65f : SWAP_INT;
                swapPhantom();
            }
            updateDecoys(playerCX, playerCY, dt);
        } else {
            decoys.clear();
            decoyCountLast = 0;
        }

        tickScreenSlide(dt);
        tickGlitchHop(worldX, worldY, hopWarn, hopTX, hopTY, dt);
        driftEntity(worldX, worldY, driftTX, driftTY, driftTimer,
                    DRIFT_SPD * (state == BossState::SYNC ? 0.55f : (phase2 ? 1.2f : 1.0f)), dt);

        if (laserWarn && pdist < MELEE_NEAR) {
            hideInCorner();
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
            glitchAmount = phase2 ? 0.10f : 0.04f;

            if (shardCd <= 0.0f) {
                shardCd = phase2 ? 2.2f : 2.8f;
                spawnShardRing(playerCX, playerCY, phase2 ? 3 : 2);
            }
            trimShards();

            if (hopCd <= 0.0f && hopWarn <= 0.0f) {
                hopCd = phase3 ? 3.0f : 4.2f;
                startGlitchHop(worldX, worldY, hopWarn, hopTX, hopTY);
            }
            if (slideCd <= 0.0f && slideT <= 0.0f) {
                slideCd = phase2 ? 9.0f : 12.0f;
                startScreenSlide();
            }
            if (pulseCd <= 0.0f) {
                pulseCd = phase2 ? 5.5f : 7.0f;
                emitPulse(worldX, worldY, phase2 ? 200.0f : 165.0f);
            }
            if (forkCd <= 0.0f && !decoys.empty()) {
                forkCd = 8.0f;
                for (auto& d : decoys) {
                    if (!d.alive) continue;
                    emitPulse(d.x, d.y, 100.0f);
                }
            }

            if (!laserWarn && stateTimer >= T_FRAGMENT - LASER_WARN) {
                lockLaser(playerCX, playerCY);
                laserWarn = true;
                laserWarnT = 0.0f;
            }
            if (laserWarn) laserWarnT += dt;

            rollbackCd -= dt;
            if (rollbackCd <= 0.0f) {
                rollbackCd = phase2 ? 8.0f : 10.0f;
                spawnShard(prevPx, prevPy, prevPx, prevPy, false);
            }

            if (stateTimer >= T_FRAGMENT) enterTear();
            break;
        }

        case BossState::TEAR:
            glitchAmount = 0.38f * (1.0f - stateTimer / T_TEAR);
            if (stateTimer >= T_LASER1) laserActive = false;
            laser2Delay -= dt;
            laser3Delay -= dt;
            if (laser2Delay <= 0.0f && !laser2Active) laser2Active = true;
            if (laser3Delay <= 0.0f && !laser3Active) laser3Active = true;
            if (stateTimer >= T_TEAR) {
                state = BossState::SYNC;
                stateTimer = 0.0f;
                laserActive = laser2Active = laser3Active = false;
            }
            break;

        case BossState::SYNC:
            glitchAmount = 0.0f;
            textNoise = 0.12f;
            shards.clear();
            if (stateTimer >= T_SYNC) {
                state = BossState::FRAGMENT;
                stateTimer = 0.0f;
                laserWarn = false;
            }
            break;
        }

        for (auto& p : pulses) {
            if (!p.alive) continue;
            p.life -= dt;
            p.r += p.maxR * 1.6f * dt;
            if (p.life <= 0.0f || p.r > p.maxR) p.alive = false;
            else {
                float dx = playerCX - p.x, dy = playerCY - p.y;
                float dist = sqrtf(dx * dx + dy * dy);
                if (dist > p.r - 18.0f && dist < p.r + 8.0f)
                    playerHP -= 12.0f * dt;
            }
        }
        pulses.erase(std::remove_if(pulses.begin(), pulses.end(),
                      [](const CorruptPulse& p) { return !p.alive; }), pulses.end());

        for (auto& s : shards) {
            if (!s.alive) continue;
            if (s.homing) {
                if (s.homingDelay > 0.0f) {
                    s.homingDelay -= dt;
                    s.orbitAng += dt * 2.8f;
                    s.x = worldX + cosf(s.orbitAng) * s.orbitR;
                    s.y = worldY + sinf(s.orbitAng) * s.orbitR * 0.75f;
                } else {
                    float dx = playerCX - s.x, dy = playerCY - s.y;
                    float d = sqrtf(dx * dx + dy * dy) + 1e-3f;
                    s.vx += dx / d * SHARD_ACCEL * dt;
                    s.vy += dy / d * SHARD_ACCEL * dt;
                    s.x += s.vx * dt;
                    s.y += s.vy * dt;
                }
            } else {
                s.x += s.vx * dt;
                s.y += s.vy * dt;
            }
            float dx = playerCX - s.x, dy = playerCY - s.y;
            if (dx * dx + dy * dy < SHARD_HIT_R * SHARD_HIT_R) {
                playerHP -= SHARD_DMG;
                s.alive = false;
            }
        }
        shards.erase(std::remove_if(shards.begin(), shards.end(),
                      [](const GlitchShard& s) { return !s.alive; }), shards.end());

        float reach = (float)(screenW + screenH);
        float pxn = -laserDirY, pyn = laserDirX;
        if (laserActive) {
            if (segDist(playerCX, playerCY, worldX, worldY, worldX + laserDirX * reach,
                        worldY + laserDirY * reach) < 24.0f)
                playerHP -= 32.0f * dt;
        }
        if (laser2Active) {
            if (segDist(playerCX, playerCY, worldX, worldY,
                        worldX + laser2DirX * reach, worldY + laser2DirY * reach) < 22.0f)
                playerHP -= 26.0f * dt;
        }
        if (laser3Active) {
            float d3x = laserDirX * 0.88f - pxn * 0.14f;
            float d3y = laserDirY * 0.88f - pyn * 0.14f;
            if (segDist(playerCX, playerCY, worldX, worldY,
                        worldX + d3x * reach, worldY + d3y * reach) < 20.0f)
                playerHP -= 24.0f * dt;
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
            float hp = hopWarn / 0.34f;
            drawCircle(hopTX, hopTY, BODY * 0.85f, 0.95f, 0.25f, 0.65f, 0.15f + hp * 0.35f);
        }

        for (auto& p : pulses) {
            if (!p.alive) continue;
            drawArcRing(p.x, p.y, p.r, 0.95f, 0.25f, 0.65f, 0.22f);
        }

        if (laserWarn) {
            float reach = (float)(screenW + screenH);
            float prog = laserWarnT / LASER_WARN;
            if (prog > 1.0f) prog = 1.0f;
            float blink = 0.55f + 0.45f * sinf(gt * 24.0f);
            drawLaserLine(worldX, worldY, laserDirX, laserDirY, reach,
                          0.3f, 0.95f, 1.0f, blink * (0.5f + prog * 0.5f), blink * 0.35f);
            drawCircle(px, py, 14.0f + prog * 12.0f, 0.25f, 0.95f, 1.0f, 0.2f + prog * 0.35f);
            wchar_t tw[] = L"RGB TEAR";
            float tw_w = g_TextS.Width(tw, 0.48f);
            g_TextS.Draw(tw, worldX - tw_w * 0.5f, worldY - BODY - 38.0f, 0.48f,
                         0.35f, 0.95f, 1.0f, 0.85f);
        }

        if (state == BossState::SYNC) {
            drawCircle(worldX, worldY, BODY * 1.35f, 0.95f, 0.25f, 0.65f, 0.12f + pulse * 0.1f);
            wchar_t sw[] = L"SYNC — CORE OPEN";
            float sw_w = g_TextS.Width(sw, 0.5f);
            g_TextS.Draw(sw, worldX - sw_w * 0.5f, worldY - BODY - 36.0f, 0.5f,
                         0.95f, 0.35f, 0.72f, 0.9f);
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

    void renderShards(float gt) const {
        for (auto& s : shards) {
            if (!s.alive) continue;
            float flick = 0.5f + 0.5f * sinf(gt * 22.0f + s.x * 0.1f);
            drawRect(s.x - 6.0f, s.y - 1.5f, 12.0f, 3.0f, 0.1f, 0.95f, 1.0f, 0.85f);
            drawRect(s.x - 4.0f + flick * 2.0f, s.y - 2.0f, 8.0f, 4.0f, 0.95f, 0.2f, 0.55f, 0.7f);
        }
    }

    void renderLasers() const {
        float reach = (float)(screenW + screenH);
        float pxn = -laserDirY, pyn = laserDirX;
        if (laserActive)
            drawLaserLine(worldX, worldY, laserDirX, laserDirY, reach, 1.0f, 0.2f, 0.45f, 0.9f, 0.4f);
        if (laser2Active)
            drawLaserLine(worldX, worldY, laser2DirX, laser2DirY, reach, 0.2f, 0.95f, 0.55f, 0.88f, 0.38f);
        if (laser3Active) {
            float d3x = laserDirX * 0.88f - pxn * 0.14f;
            float d3y = laserDirY * 0.88f - pyn * 0.14f;
            drawLaserLine(worldX, worldY, d3x, d3y, reach, 0.55f, 0.2f, 0.95f, 0.85f, 0.35f);
        }
    }

    void renderCore(float cx, float cy, float gt, bool real, float alpha, float hpFrac = 1.0f) const {
        float pulse = 0.5f + 0.5f * sinf(gt * 6.0f + cx * 0.02f);
        float a = alpha * (real ? 1.0f : 0.65f);
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
        if (hpFrac < 1.0f) {
            float bw = BODY * 1.3f;
            drawRect(cx - bw * 0.5f, cy - BODY - 12.0f, bw, 4.0f, 0.12f, 0.1f, 0.14f, 0.85f * a);
            drawRect(cx - bw * 0.5f, cy - BODY - 12.0f, bw * hpFrac, 4.0f, 0.95f, 0.25f, 0.55f, 0.9f * a);
        }
    }

    void renderDecoyWindow(const Decoy& d, float gt, float wx, float wy, float ww, float wh) const {
        if (!d.alive) return;
        if (d.barRain > 0.0f) {
            for (int i = 0; i < 5; i++) {
                float by = wy + 24.0f + (float)(i * 17) + fmodf(gt * 90.0f + (float)i * 13.0f, wh - 40.0f);
                drawRect(wx + 8.0f, by, ww - 16.0f, 2.0f, 0.95f, 0.15f, 0.55f, 0.35f);
            }
        }
        if (d.cursorFlash > 0.0f) {
            float cx = wx + ww * 0.5f + (d.cursorX - d.x) * 0.3f;
            float cy = wy + wh * 0.5f + (d.cursorY - d.y) * 0.3f;
            drawRect(cx - 1.0f, cy - 10.0f, 2.0f, 20.0f, 1.0f, 0.95f, 0.95f, d.cursorFlash);
            drawRect(cx - 10.0f, cy - 1.0f, 20.0f, 2.0f, 1.0f, 0.95f, 0.95f, d.cursorFlash);
        }
        if (d.hopWarn > 0.05f) {
            float hx = wx + ww * 0.5f + (d.hopTX - d.x) * 0.25f;
            float hy = wy + wh * 0.5f + (d.hopTY - d.y) * 0.25f;
            drawCircle(hx, hy, 16.0f, 0.95f, 0.2f, 0.55f, 0.25f);
        }
        float hf = (d.maxHp > 0.0f) ? d.hp / d.maxHp : 0.0f;
        renderCore(wx + ww * 0.5f, wy + wh * 0.52f, gt, false, 0.92f, hf);
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
    }
};
