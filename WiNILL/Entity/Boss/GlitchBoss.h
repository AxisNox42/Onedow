#pragma once
// ─────────────────────────────────────────────────────────────
// CORRUPT.dll — v3.1 근접·점멸·잔상 + 글리치 파동·블링크
//
//   STALK → (LUNGE | WAVE | BLINK) → STUTTER → …
//   P1: LUNGE만  P2+: 3종 로테이션  P3: 연속기·파동↑
// ─────────────────────────────────────────────────────────────

#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include "DrawPrim.h"
#include "TextRenderer.h"

extern TextRenderer g_TextS;

enum class BossState {
    BOOT, STALK,
    LUNGE_WARN, LUNGE,
    WAVE_WARN, WAVE,
    BLINK,
    STUTTER
};

enum class GAttack { LUNGE, WAVE, BLINK };

struct GlitchAfterimage {
    float x = 0, y = 0;
    float life = 0.0f;
    float maxLife = 0.55f;
    bool  harmful = true;
    float rgbOff = 0.0f;
};

struct GlitchWave {
    float x = 0, y = 0;
    float r = 0.0f;
    float maxR = 380.0f;
    float spd = 260.0f;
    float thick = 22.0f;
    float life = 1.0f;
    bool  alive = true;
};

class GlitchBoss {
public:
    float worldX = 0, worldY = 0;
    float hp = 9000.0f, maxHp = 9000.0f;
    bool  alive = true;
    bool  exploded = false;
    int   screenW = 0, screenH = 0;

    BossState state = BossState::BOOT;
    float stateTimer = 0.0f;

    bool  phase2 = false;
    bool  phase3 = false;

    std::vector<GlitchAfterimage> afterimages;
    std::vector<GlitchWave> waves;

    float glitchAmount = 0.0f;
    float textNoise = 0.0f;
    float lungeFlash = 0.0f;
    float attackBanner = 0.0f;

    static constexpr float BODY = 44.0f;
    static constexpr float WIN_TB = 22.0f;
    static constexpr float GLITCH_WIN_TB = 22.0f;

    static constexpr float T_BOOT = 1.8f;
    static constexpr float T_STALK = 3.6f;
    static constexpr float T_LUNGE_WARN = 0.42f;
    static constexpr float T_LUNGE = 0.28f;
    static constexpr float T_WAVE_WARN = 0.55f;
    static constexpr float T_WAVE = 1.35f;
    static constexpr float T_BLINK = 0.72f;
    static constexpr float T_STUTTER = 1.85f;

    GlitchBoss(int sw, int sh, float hpInit) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.72f;
        worldY = sh * 0.28f;
    }

    static const wchar_t* BossName() { return L"CORRUPT.dll"; }

    float combinedHp() const { return hp; }
    float combinedMaxHp() const { return maxHp; }

    bool suppressScreenGlitch() const { return false; }

    const wchar_t* stateTag() const {
        switch (state) {
        case BossState::BOOT:       return L"BOOT";
        case BossState::STALK:      return L"STALK";
        case BossState::LUNGE_WARN: return L"◉ LOCK";
        case BossState::LUNGE:      return L"DASH";
        case BossState::WAVE_WARN:  return L"WAVE◉";
        case BossState::WAVE:       return L"WAVE";
        case BossState::BLINK:      return L"BLINK";
        case BossState::STUTTER:    return L"STUTTER";
        default: return L"?";
        }
    }

    const wchar_t* attackName() const {
        switch (nextAttack_) {
        case GAttack::LUNGE: return L"RUSH";
        case GAttack::WAVE:  return L"WAVE";
        case GAttack::BLINK: return L"BLINK";
        default: return L"?";
        }
    }

    bool isVisible() const {
        if (state == BossState::STUTTER) return true;
        if (state == BossState::WAVE_WARN || state == BossState::WAVE) return true;
        if (state == BossState::LUNGE_WARN && stateTimer > T_LUNGE_WARN * 0.55f) return true;
        if (state == BossState::BLINK) return blinkVisible_;
        return flickerOn_;
    }

    bool isHittable() const {
        if (hp <= 0.0f) return false;
        if (state == BossState::STUTTER) return true;
        if (state == BossState::LUNGE || state == BossState::LUNGE_WARN) return false;
        if (state == BossState::BLINK) return false;
        if (state == BossState::WAVE) return false;
        return isVisible();
    }

    void syncAlive() {
        if (hp <= 0.0f) alive = false;
    }

    void Update(float playerCX, float playerCY, float dt, float& playerHP) {
        if (!alive) return;

        stateTimer += dt;
        lungeFlash = std::max(0.0f, lungeFlash - dt * 2.8f);
        attackBanner = std::max(0.0f, attackBanner - dt * 1.2f);
        textNoise = std::max(0.0f, textNoise - dt * 1.8f);

        if (!phase2 && hp <= maxHp * 0.5f) phase2 = true;
        if (!phase3 && hp <= maxHp * 0.25f) phase3 = true;

        tickFlicker(dt);
        tickAfterimages(dt, playerCX, playerCY, playerHP);
        tickWaves(dt, playerCX, playerCY, playerHP);
        tickContact(playerCX, playerCY, dt, playerHP);

        switch (state) {
        case BossState::BOOT:
            glitchAmount = 0.28f + 0.12f * sinf(stateTimer * 9.0f);
            if (stateTimer < 0.5f) textNoise = 0.65f;
            if (stateTimer >= T_BOOT) enterStalk();
            break;

        case BossState::STALK:
            glitchAmount = 0.06f + (phase2 ? 0.05f : 0.0f);
            chasePlayer(playerCX, playerCY, dt);
            if (stateTimer >= stalkDuration()) pickAttack(playerCX, playerCY);
            break;

        case BossState::LUNGE_WARN:
            glitchAmount = 0.14f + 0.08f * sinf(stateTimer * 24.0f);
            textNoise = 0.35f;
            if (stateTimer >= T_LUNGE_WARN) enterLunge();
            break;

        case BossState::LUNGE:
            glitchAmount = 0.1f;
            tickLunge(dt);
            if (stateTimer >= T_LUNGE) enterStutter();
            break;

        case BossState::WAVE_WARN:
            glitchAmount = 0.12f + 0.06f * sinf(stateTimer * 16.0f);
            textNoise = 0.3f;
            if (stateTimer >= T_WAVE_WARN) enterWave();
            break;

        case BossState::WAVE:
            glitchAmount = 0.09f;
            tickWaveSpawn(dt);
            if (stateTimer >= T_WAVE) enterStutter();
            break;

        case BossState::BLINK:
            glitchAmount = 0.18f;
            tickBlink(dt, playerCX, playerCY);
            if (stateTimer >= T_BLINK) enterStutter();
            break;

        case BossState::STUTTER:
            glitchAmount = 0.02f;
            driftIdle(dt);
            if (stateTimer >= T_STUTTER) {
                if (phase3 && chainExtra_ > 0) {
                    chainExtra_--;
                    pickAttack(playerCX, playerCY);
                } else {
                    enterStalk();
                }
            }
            break;
        }

        syncAlive();
    }

    bool tryHitAfterimage(float bx, float by, float pbx, float pby) {
        for (auto& g : afterimages) {
            if (g.life <= 0.0f) continue;
            if (segDist(bx, by, pbx, pby, g.x, g.y) < BODY * 0.65f) {
                g.life = 0.0f;
                glitchAmount = 0.35f;
                textNoise = 0.5f;
                return true;
            }
        }
        return false;
    }

    float tryHitBody(float bx, float by, float pbx, float pby, float dmg) {
        if (!isHittable()) return 0.0f;
        if (segDist(bx, by, pbx, pby, worldX, worldY) < BODY * 0.72f) {
            float dealt = (dmg < hp) ? dmg : hp;
            hp -= dealt;
            spawnAfterimage(worldX, worldY, true, 0.35f);
            glitchAmount = 0.25f;
            syncAlive();
            return dealt;
        }
        return 0.0f;
    }

    void renderWaves(float gt) const {
        for (auto& w : waves) {
            if (!w.alive) continue;
            float fade = w.life;
            float blink = 0.55f + 0.45f * sinf(gt * 20.0f + w.r * 0.04f);
            float a = fade * blink;
            drawRing(w.x + 5.0f, w.y, w.r, w.thick, 1.0f, 0.12f, 0.35f, a * 0.55f);
            drawRing(w.x - 4.0f, w.y, w.r, w.thick * 0.92f, 0.12f, 0.92f, 0.38f, a * 0.5f);
            drawRing(w.x, w.y, w.r, w.thick * 0.85f, 0.42f, 0.18f, 0.98f, a * 0.48f);
            for (int i = 0; i < 6; i++) {
                float ang = gt * 1.8f + (float)i * 1.047f;
                float sx = w.x + cosf(ang) * w.r;
                float sy = w.y + sinf(ang) * w.r * 0.75f;
                drawRect(sx - 8.0f, sy - 2.0f, 16.0f, 4.0f, 0.2f, 0.95f, 1.0f, a * 0.35f);
            }
        }
    }

    void renderAfterimages(float gt) const {
        for (auto& g : afterimages) {
            if (g.life <= 0.0f) continue;
            float t = g.life / g.maxLife;
            float blink = 0.45f + 0.55f * sinf(gt * 18.0f + g.x * 0.05f);
            float a = t * blink * 0.82f;
            drawPhantomCore(g.x + g.rgbOff, g.y, gt, a, false);
            drawPhantomCore(g.x - g.rgbOff * 0.7f, g.y, gt, a * 0.75f, false);
        }
    }

    void renderBody(float gt) const {
        if (!isVisible() && state != BossState::LUNGE) {
            float ghost = 0.18f + 0.08f * sinf(gt * 30.0f);
            drawPhantomCore(worldX, worldY, gt, ghost, false);
            return;
        }
        float pulse = (state == BossState::LUNGE) ? 1.15f : 1.0f;
        if (lungeFlash > 0.02f)
            drawCircle(worldX, worldY, BODY * 1.35f, 1.0f, 0.3f, 0.65f, lungeFlash * 0.25f);
        drawMonitorCore(worldX, worldY, gt, pulse,
                        (maxHp > 0.0f) ? hp / maxHp : 1.0f);
        wchar_t tag[32];
        swprintf_s(tag, L"[%ls]", stateTag());
        float tw = g_TextS.Width(tag, 0.44f);
        g_TextS.Draw(tag, worldX - tw * 0.5f, worldY - BODY - 28.0f, 0.44f,
                     0.95f, 0.35f, 0.72f, isVisible() ? 0.88f : 0.45f);
    }

    void renderTelegraphs(float px, float py, float gt) const {
        if (state == BossState::LUNGE_WARN) {
            float prog = stateTimer / T_LUNGE_WARN;
            if (prog > 1.0f) prog = 1.0f;
            float blink = 0.5f + 0.5f * sinf(gt * 26.0f);
            drawCircle(px, py, 16.0f + prog * 22.0f, 0.35f, 0.95f, 1.0f,
                       0.22f + prog * 0.38f * blink);
            drawCircle(worldX, worldY, BODY * (0.9f + prog * 0.35f),
                       0.95f, 0.22f, 0.58f, 0.18f + prog * 0.28f);
        }
        if (state == BossState::WAVE_WARN) {
            float prog = stateTimer / T_WAVE_WARN;
            if (prog > 1.0f) prog = 1.0f;
            for (int i = 1; i <= 3; i++) {
                float pr = 40.0f + (float)i * 35.0f * prog;
                float blink = 0.5f + 0.5f * sinf(gt * 18.0f + (float)i);
                drawRing(worldX, worldY, pr, 10.0f, 0.35f, 0.92f, 1.0f, 0.15f * blink * prog);
            }
        }
        if (state == BossState::STUTTER) {
            float pulse = 0.5f + 0.5f * sinf(gt * 9.0f);
            drawCircle(worldX, worldY, BODY * 1.35f, 0.95f, 0.35f, 0.7f, 0.12f + pulse * 0.14f);
        }
        if (state == BossState::BLINK && blinkVisible_) {
            drawCircle(worldX, worldY, BODY * 1.1f, 0.85f, 0.3f, 0.95f, 0.35f);
        }
    }

    void renderTelegraphLabels(float px, float py, float gt) const {
        (void)gt;
        if (state == BossState::LUNGE_WARN) {
            float prog = stateTimer / T_LUNGE_WARN;
            if (prog > 1.0f) prog = 1.0f;
            wchar_t tw[] = L"▶ PHANTOM RUSH";
            float tw_w = g_TextS.Width(tw, 0.52f);
            g_TextS.Draw(tw, px - tw_w * 0.5f, py - 58.0f, 0.52f,
                         0.35f, 0.95f, 1.0f, 0.75f + prog * 0.2f);
        }
        if (state == BossState::WAVE_WARN || state == BossState::WAVE) {
            wchar_t ww[] = L"≈ GLITCH WAVE ≈";
            float ww_w = g_TextS.Width(ww, 0.5f);
            g_TextS.Draw(ww, worldX - ww_w * 0.5f, worldY - BODY - 42.0f, 0.5f,
                         0.35f, 0.95f, 1.0f, 0.82f);
        }
        if (state == BossState::BLINK) {
            wchar_t bw[] = L"▣ BLINK CHAIN";
            float bw_w = g_TextS.Width(bw, 0.48f);
            g_TextS.Draw(bw, worldX - bw_w * 0.5f, worldY - BODY - 40.0f, 0.48f,
                         0.85f, 0.35f, 0.95f, 0.85f);
        }
        if (state == BossState::STUTTER) {
            wchar_t sw[] = L"◆ STUTTER — HIT NOW";
            float sw_w = g_TextS.Width(sw, 0.48f);
            g_TextS.Draw(sw, worldX - sw_w * 0.5f, worldY - BODY - 38.0f, 0.48f,
                         0.95f, 0.35f, 0.72f, 0.92f);
        }
        if (attackBanner > 0.04f) {
            wchar_t ab[16];
            swprintf_s(ab, L"%ls", attackName());
            float sc = 1.05f + attackBanner * 0.25f;
            float ab_w = g_TextS.Width(ab, sc);
            g_TextS.Draw(ab, px - ab_w * 0.5f, py - 90.0f, sc,
                         1.0f, 0.15f, 0.4f, attackBanner * 0.88f);
        }
    }

private:
    float flickerT_ = 0.0f;
    bool  flickerOn_ = true;
    float lungeDirX_ = 1.0f, lungeDirY_ = 0.0f;
    float lungeSpd_ = 520.0f;
    float afterimageCd_ = 0.0f;
    float moveTrail_ = 0.0f;
    int   chainExtra_ = 0;
    float lastX_ = 0.0f, lastY_ = 0.0f;
    GAttack nextAttack_ = GAttack::LUNGE;
    float waveSpawnCd_ = 0.0f;
    int   waveSpawned_ = 0;
    int   blinkStep_ = 0;
    float blinkStepT_ = 0.0f;
    bool  blinkVisible_ = true;

    float stalkDuration() const {
        float base = T_STALK;
        if (phase3) return base * 0.72f;
        if (phase2) return base * 0.85f;
        return base;
    }

    float flickerInterval() const {
        if (phase3) return 0.07f;
        if (phase2) return 0.11f;
        return 0.16f;
    }

    int afterimageCap() const {
        if (phase3) return 14;
        if (phase2) return 10;
        return 6;
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

    static void drawRing(float cx, float cy, float r, float thick,
                         float cr, float cg, float cb, float a) {
        if (a <= 0.01f || r <= 1.0f) return;
        const int SEG = 32;
        for (int i = 0; i < SEG; i++) {
            float a0 = (float)i / (float)SEG * 6.283185f;
            float a1 = (float)(i + 1) / (float)SEG * 6.283185f;
            float ox = cosf(a0), oy = sinf(a0);
            float ix = cosf(a1), iy = sinf(a1);
            float ro = r + thick * 0.5f, ri = r - thick * 0.5f;
            if (ri < 2.0f) ri = 2.0f;
            float v[12] = {
                cx + ox * ro, cy + oy * ro,
                cx + ox * ri, cy + oy * ri,
                cx + ix * ro, cy + iy * ro,
                cx + ox * ri, cy + oy * ri,
                cx + ix * ri, cy + iy * ri,
                cx + ix * ro, cy + iy * ro
            };
            BatchVerts(v, 6, cr, cg, cb, a);
        }
    }

    void tickFlicker(float dt) {
        if (state == BossState::STUTTER || state == BossState::WAVE ||
            state == BossState::WAVE_WARN) {
            flickerOn_ = true;
            return;
        }
        flickerT_ -= dt;
        if (flickerT_ <= 0.0f) {
            flickerOn_ = !flickerOn_;
            flickerT_ = flickerInterval() * (0.65f + (float)(rand() % 35) * 0.01f);
            if (state == BossState::LUNGE_WARN) flickerOn_ = true;
        }
    }

    void spawnAfterimage(float x, float y, bool harmful, float life) {
        if ((int)afterimages.size() >= afterimageCap()) {
            auto it = std::min_element(afterimages.begin(), afterimages.end(),
                [](const GlitchAfterimage& a, const GlitchAfterimage& b) {
                    return a.life < b.life;
                });
            if (it != afterimages.end()) *it = GlitchAfterimage{};
        }
        GlitchAfterimage g;
        g.x = x; g.y = y;
        g.maxLife = g.life = life;
        g.harmful = harmful;
        g.rgbOff = 4.0f + (float)(rand() % 8);
        afterimages.push_back(g);
    }

    void spawnWave() {
        GlitchWave w;
        w.x = worldX;
        w.y = worldY;
        w.r = BODY * 0.5f;
        w.maxR = phase3 ? 440.0f : (phase2 ? 400.0f : 340.0f);
        w.spd = phase3 ? 310.0f : (phase2 ? 270.0f : 240.0f);
        w.thick = phase3 ? 26.0f : 22.0f;
        w.life = 1.0f;
        waves.push_back(w);
    }

    void tickWaves(float dt, float px, float py, float& playerHP) {
        for (auto& w : waves) {
            if (!w.alive) continue;
            w.r += w.spd * dt;
            w.life = 1.0f - w.r / w.maxR;
            if (w.r >= w.maxR) { w.alive = false; continue; }
            float dx = px - w.x, dy = py - w.y;
            float dist = sqrtf(dx * dx + dy * dy);
            float half = w.thick * 0.55f;
            if (dist > w.r - half && dist < w.r + half) {
                float rate = phase3 ? 22.0f : (phase2 ? 16.0f : 12.0f);
                playerHP -= rate * dt;
            }
        }
        waves.erase(std::remove_if(waves.begin(), waves.end(),
            [](const GlitchWave& w) { return !w.alive; }), waves.end());
    }

    void tickAfterimages(float dt, float px, float py, float& playerHP) {
        for (auto& g : afterimages) {
            if (g.life <= 0.0f) continue;
            g.life -= dt;
            if (!g.harmful) continue;
            float dx = px - g.x, dy = py - g.y;
            if (dx * dx + dy * dy < 900.0f)
                playerHP -= (phase3 ? 9.0f : (phase2 ? 6.5f : 4.0f)) * dt;
        }
        afterimages.erase(std::remove_if(afterimages.begin(), afterimages.end(),
            [](const GlitchAfterimage& g) { return g.life <= 0.0f; }), afterimages.end());
    }

    void tickContact(float px, float py, float dt, float& playerHP) {
        (void)px; (void)py;
        if (state != BossState::LUNGE && state != BossState::STALK) return;
        float dx = px - worldX, dy = py - worldY;
        float d2 = dx * dx + dy * dy;
        float r = BODY * 0.85f;
        if (d2 < r * r) {
            float rate = (state == BossState::LUNGE) ? 48.0f : 18.0f;
            if (phase3) rate *= 1.15f;
            playerHP -= rate * dt;
        }
    }

    void chasePlayer(float px, float py, float dt) {
        float dx = px - worldX, dy = py - worldY;
        float d = sqrtf(dx * dx + dy * dy) + 1e-3f;
        float spd = phase3 ? 118.0f : (phase2 ? 98.0f : 82.0f);
        float step = spd * dt;
        if (step > d) step = d;
        lastX_ = worldX; lastY_ = worldY;
        worldX += dx / d * step;
        worldY += dy / d * step;

        moveTrail_ += step;
        afterimageCd_ -= dt;
        float trailNeed = phase3 ? 14.0f : (phase2 ? 18.0f : 28.0f);
        if (afterimageCd_ <= 0.0f || moveTrail_ >= trailNeed) {
            moveTrail_ = 0.0f;
            afterimageCd_ = phase3 ? 0.08f : (phase2 ? 0.10f : 0.15f);
            spawnAfterimage(lastX_, lastY_, true, phase3 ? 0.75f : 0.55f);
            if (phase2) spawnAfterimage(lastX_ + 6.0f, lastY_ - 4.0f, true, 0.45f);
        }
    }

    void driftIdle(float dt) {
        float margin = BODY + 70.0f;
        float cx = screenW * 0.5f, cy = screenH * 0.5f;
        float dx = cx - worldX, dy = cy - worldY;
        float d = sqrtf(dx * dx + dy * dy) + 1e-3f;
        float step = 28.0f * dt;
        if (step > d) step = d;
        lastX_ = worldX; lastY_ = worldY;
        worldX += dx / d * step;
        worldY += dy / d * step;
        worldX = std::max(margin, std::min((float)screenW - margin, worldX));
        worldY = std::max(margin, std::min((float)screenH - margin, worldY));
    }

    void pickAttack(float px, float py) {
        if (!phase2) {
            nextAttack_ = GAttack::LUNGE;
            beginLungeWarn(px, py);
            return;
        }
        int roll = rand() % 100;
        if (phase3) {
            if (roll < 34) nextAttack_ = GAttack::LUNGE;
            else if (roll < 67) nextAttack_ = GAttack::WAVE;
            else nextAttack_ = GAttack::BLINK;
        } else {
            if (roll < 45) nextAttack_ = GAttack::LUNGE;
            else if (roll < 75) nextAttack_ = GAttack::WAVE;
            else nextAttack_ = GAttack::BLINK;
        }
        switch (nextAttack_) {
        case GAttack::LUNGE: beginLungeWarn(px, py); break;
        case GAttack::WAVE:  beginWaveWarn(); break;
        case GAttack::BLINK: beginBlink(px, py); break;
        }
    }

    void beginLungeWarn(float px, float py) {
        state = BossState::LUNGE_WARN;
        stateTimer = 0.0f;
        float dx = px - worldX, dy = py - worldY;
        float d = sqrtf(dx * dx + dy * dy) + 1e-3f;
        lungeDirX_ = dx / d;
        lungeDirY_ = dy / d;
        flickerOn_ = true;
        textNoise = 0.4f;
    }

    void enterLunge() {
        state = BossState::LUNGE;
        stateTimer = 0.0f;
        lungeFlash = 1.0f;
        attackBanner = 0.9f;
        lungeSpd_ = phase3 ? 680.0f : (phase2 ? 580.0f : 500.0f);
        spawnAfterimage(worldX, worldY, true, 0.65f);
        spawnAfterimage(worldX - lungeDirX_ * 24.0f, worldY - lungeDirY_ * 24.0f, true, 0.5f);
    }

    void tickLunge(float dt) {
        lastX_ = worldX; lastY_ = worldY;
        worldX += lungeDirX_ * lungeSpd_ * dt;
        worldY += lungeDirY_ * lungeSpd_ * dt;
        spawnAfterimage(lastX_, lastY_, true, 0.4f);
    }

    void beginWaveWarn() {
        state = BossState::WAVE_WARN;
        stateTimer = 0.0f;
        attackBanner = 0.75f;
        textNoise = 0.45f;
        flickerOn_ = true;
    }

    void enterWave() {
        state = BossState::WAVE;
        stateTimer = 0.0f;
        waveSpawnCd_ = 0.0f;
        waveSpawned_ = 0;
        lungeFlash = 0.6f;
        attackBanner = 0.85f;
        spawnWave();
    }

    void tickWaveSpawn(float dt) {
        waveSpawnCd_ -= dt;
        int maxWaves = phase3 ? 4 : (phase2 ? 3 : 2);
        if (waveSpawnCd_ <= 0.0f && waveSpawned_ < maxWaves) {
            waveSpawnCd_ = phase3 ? 0.28f : 0.36f;
            spawnWave();
            waveSpawned_++;
            spawnAfterimage(worldX, worldY, false, 0.3f);
        }
    }

    void beginBlink(float px, float py) {
        state = BossState::BLINK;
        stateTimer = 0.0f;
        blinkStep_ = 0;
        blinkStepT_ = 0.0f;
        blinkVisible_ = true;
        attackBanner = 0.8f;
        textNoise = 0.5f;
        float dx = px - worldX, dy = py - worldY;
        float d = sqrtf(dx * dx + dy * dy) + 1e-3f;
        lungeDirX_ = dx / d;
        lungeDirY_ = dy / d;
    }

    void tickBlink(float dt, float px, float py) {
        blinkStepT_ -= dt;
        if (blinkStepT_ <= 0.0f && blinkStep_ < (phase3 ? 5 : 4)) {
            blinkStepT_ = phase3 ? 0.11f : 0.14f;
            blinkVisible_ = false;
            lastX_ = worldX; lastY_ = worldY;
            float jump = phase3 ? 95.0f : 78.0f;
            float dx = px - worldX, dy = py - worldY;
            float d = sqrtf(dx * dx + dy * dy) + 1e-3f;
            worldX += dx / d * jump;
            worldY += dy / d * jump;
            spawnAfterimage(lastX_, lastY_, true, 0.65f);
            spawnAfterimage(worldX, worldY, true, 0.5f);
            if (phase2) spawnAfterimage(lastX_ + 8.0f, lastY_ - 6.0f, true, 0.4f);
            blinkVisible_ = true;
            blinkStep_++;
            glitchAmount = 0.3f;
        }
    }

    void enterStutter() {
        state = BossState::STUTTER;
        stateTimer = 0.0f;
        flickerOn_ = true;
        blinkVisible_ = true;
        glitchAmount = 0.05f;
        waves.clear();
        if (phase3 && chainExtra_ == 0) chainExtra_ = 1;
    }

    void enterStalk() {
        state = BossState::STALK;
        stateTimer = 0.0f;
        chainExtra_ = 0;
        glitchAmount = 0.05f;
    }

    void drawPhantomCore(float cx, float cy, float gt, float alpha, bool hpBar) const {
        float pulse = 0.5f + 0.5f * sinf(gt * 6.0f + cx * 0.02f);
        drawRect(cx - BODY * 0.7f, cy - BODY * 0.55f, BODY * 1.4f, BODY * 1.1f,
                 0.05f, 0.06f, 0.1f, 0.85f * alpha);
        drawCircle(cx + 4.0f, cy, BODY * 0.65f, 1.0f, 0.1f, 0.35f, 0.35f * alpha);
        drawCircle(cx - 4.0f, cy, BODY * 0.65f, 0.1f, 0.82f, 1.0f, 0.35f * alpha);
        drawCircle(cx, cy, BODY * 0.6f, 0.08f, 0.07f, 0.11f, 0.9f * alpha);
        drawNeonBorder(cx - BODY * 0.65f, cy - BODY * 0.65f, BODY * 1.3f, BODY * 1.3f,
                       0.85f, 0.25f, 0.62f);
        if (hpBar && state == BossState::STUTTER)
            drawCircle(cx, cy, BODY * 0.32f, 1.0f, 0.4f, 0.72f, (0.25f + pulse * 0.15f) * alpha);
    }

    void drawMonitorCore(float cx, float cy, float gt, float scale, float hpFrac) const {
        float pulse = 0.5f + 0.5f * sinf(gt * 5.5f + cx * 0.015f);
        float a = isVisible() ? 1.0f : 0.55f;
        drawRect(cx - BODY * 0.76f * scale, cy - BODY * 0.6f * scale,
                 BODY * 1.52f * scale, BODY * 1.2f * scale,
                 0.04f, 0.05f, 0.09f, 0.94f * a);
        for (int i = 0; i < 5; i++) {
            float sy = cy - BODY * 0.5f * scale + (float)i * BODY * 0.24f * scale;
            drawRect(cx - BODY * 0.7f * scale, sy, BODY * 1.4f * scale, 2.0f,
                     0.15f, 0.82f, 1.0f, 0.22f * a);
        }
        drawCircle(cx + 5.0f * scale, cy, BODY * 0.72f * scale, 1.0f, 0.08f, 0.35f, 0.42f * a);
        drawCircle(cx - 5.0f * scale, cy, BODY * 0.72f * scale, 0.08f, 0.85f, 1.0f, 0.42f * a);
        drawCircle(cx, cy, BODY * 0.68f * scale, 0.07f, 0.06f, 0.1f, 0.92f * a);
        drawNeonBorder(cx - BODY * 0.7f * scale, cy - BODY * 0.7f * scale,
                       BODY * 1.4f * scale, BODY * 1.4f * scale, 0.95f, 0.22f, 0.62f);
        drawRect(cx - 11.0f * scale, cy - 3.0f * scale, 22.0f * scale, 6.0f * scale,
                 0.12f, 0.95f, 1.0f, 0.68f * a);
        if (state == BossState::STUTTER)
            drawCircle(cx, cy, BODY * 0.38f * scale, 1.0f, 0.45f, 0.75f, 0.35f + pulse * 0.2f);
        if (hpFrac < 1.0f) {
            float bw = BODY * 1.28f;
            drawRect(cx - bw * 0.5f, cy - BODY - 11.0f, bw, 4.0f, 0.1f, 0.1f, 0.14f, 0.85f * a);
            drawRect(cx - bw * 0.5f, cy - BODY - 11.0f, bw * hpFrac, 4.0f,
                     0.95f, 0.25f, 0.55f, 0.9f * a);
        }
    }
};
