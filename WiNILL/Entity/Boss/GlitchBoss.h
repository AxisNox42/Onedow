#pragma once
// ─────────────────────────────────────────────────────────────
// CORRUPT.dll — v3 근접·점멸·잔상
//
//   레이저/소환 제거. CORRUPT.dll 창 하나 안에서:
//     · 플레이어 추격 (STALK)
//     · 고속 점멸 → 무적 구간 / STUTTER 때만 확실히 맞음
//     · 이동·돌진 궤적에 RGB 잔상 (PHANTOM) — 접촉 피해
//     · LUNGE — 짧은 예고 후 근접 돌진
//
//   사이클: BOOT → STALK → LUNGE_WARN → LUNGE → STUTTER → …
//   P2 50%: 잔상↑·점멸↑   P3 25%: 연속 돌진·잔상 지속↑
// ─────────────────────────────────────────────────────────────

#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include "DrawPrim.h"
#include "TextRenderer.h"

extern TextRenderer g_TextS;

enum class BossState { BOOT, STALK, LUNGE_WARN, LUNGE, STUTTER };

struct GlitchAfterimage {
    float x = 0, y = 0;
    float life = 0.0f;
    float maxLife = 0.55f;
    bool  harmful = true;
    float rgbOff = 0.0f;
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

    float glitchAmount = 0.0f;
    float textNoise = 0.0f;
    float lungeFlash = 0.0f;
    float attackBanner = 0.0f;

    static constexpr float BODY = 44.0f;
    static constexpr float WIN_TB = 22.0f;
    static constexpr float GLITCH_WIN_TB = 22.0f;

    static constexpr float T_BOOT = 1.8f;
    static constexpr float T_STALK = 3.8f;
    static constexpr float T_LUNGE_WARN = 0.42f;
    static constexpr float T_LUNGE = 0.28f;
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
        case BossState::STUTTER:    return L"STUTTER";
        default: return L"?";
        }
    }

    bool isVisible() const {
        if (state == BossState::STUTTER) return true;
        if (state == BossState::LUNGE_WARN && stateTimer > T_LUNGE_WARN * 0.55f) return true;
        return flickerOn_;
    }

    bool isHittable() const {
        if (hp <= 0.0f) return false;
        if (state == BossState::STUTTER) return true;
        if (state == BossState::LUNGE || state == BossState::LUNGE_WARN) return false;
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
        tickContact(playerCX, playerCY, dt, playerHP);

        switch (state) {
        case BossState::BOOT:
            glitchAmount = 0.28f + 0.12f * sinf(stateTimer * 9.0f);
            if (stateTimer < 0.5f) textNoise = 0.65f;
            if (stateTimer >= T_BOOT) enterStalk();
            break;

        case BossState::STALK:
            glitchAmount = 0.06f + (phase2 ? 0.04f : 0.0f);
            chasePlayer(playerCX, playerCY, dt);
            if (stateTimer >= stalkDuration()) beginLungeWarn(playerCX, playerCY);
            break;

        case BossState::LUNGE_WARN:
            glitchAmount = 0.14f + 0.08f * sinf(stateTimer * 24.0f);
            textNoise = 0.35f;
            if (stateTimer >= T_LUNGE_WARN) enterLunge();
            break;

        case BossState::LUNGE:
            glitchAmount = 0.1f;
            tickLunge(dt, playerCX, playerCY, playerHP);
            if (stateTimer >= T_LUNGE) enterStutter();
            break;

        case BossState::STUTTER:
            glitchAmount = 0.02f;
            driftIdle(dt);
            if (stateTimer >= T_STUTTER) {
                if (phase3 && chainLunges_ > 0) {
                    chainLunges_--;
                    beginLungeWarn(playerCX, playerCY);
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
            float ex = worldX + lungeDirX_ * (80.0f + prog * 120.0f);
            float ey = worldY + lungeDirY_ * (80.0f + prog * 120.0f);
            drawCircle(ex, ey, 12.0f + prog * 8.0f, 1.0f, 0.15f, 0.45f, 0.35f * blink);
        }
        if (state == BossState::STUTTER) {
            float pulse = 0.5f + 0.5f * sinf(gt * 9.0f);
            drawCircle(worldX, worldY, BODY * 1.35f, 0.95f, 0.35f, 0.7f, 0.12f + pulse * 0.14f);
        }
        if (state == BossState::LUNGE) {
            float ex = worldX - lungeDirX_ * 40.0f;
            float ey = worldY - lungeDirY_ * 40.0f;
            drawRect(ex - 18.0f, ey - 8.0f, 36.0f, 16.0f, 1.0f, 0.12f, 0.42f, 0.55f);
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
        if (state == BossState::STUTTER) {
            wchar_t sw[] = L"◆ STUTTER — HIT NOW";
            float sw_w = g_TextS.Width(sw, 0.48f);
            g_TextS.Draw(sw, worldX - sw_w * 0.5f, worldY - BODY - 38.0f, 0.48f,
                         0.95f, 0.35f, 0.72f, 0.92f);
        }
        if (attackBanner > 0.04f) {
            wchar_t bw[] = L"DASH";
            float sc = 1.05f + attackBanner * 0.25f;
            float bw_w = g_TextS.Width(bw, sc);
            g_TextS.Draw(bw, px - bw_w * 0.5f, py - 90.0f, sc,
                         1.0f, 0.15f, 0.4f, attackBanner * 0.88f);
        }
        if (state == BossState::BOOT && stateTimer < 1.2f) {
            wchar_t iw[] = L"CORRUPT.dll";
            float iw_w = g_TextS.Width(iw, 0.5f);
            g_TextS.Draw(iw, worldX - iw_w * 0.5f, worldY - BODY - 44.0f, 0.5f,
                         0.95f, 0.25f, 0.55f, 0.88f);
        }
    }

private:
    float flickerT_ = 0.0f;
    bool  flickerOn_ = true;
    float lungeDirX_ = 1.0f, lungeDirY_ = 0.0f;
    float lungeSpd_ = 520.0f;
    float afterimageCd_ = 0.0f;
    float moveTrail_ = 0.0f;
    int   chainLunges_ = 0;
    float lastX_ = 0.0f, lastY_ = 0.0f;

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

    void tickFlicker(float dt) {
        if (state == BossState::STUTTER) {
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
        float trailNeed = phase3 ? 14.0f : (phase2 ? 20.0f : 28.0f);
        if (afterimageCd_ <= 0.0f || moveTrail_ >= trailNeed) {
            moveTrail_ = 0.0f;
            afterimageCd_ = phase3 ? 0.08f : (phase2 ? 0.11f : 0.15f);
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
        (void)dt;
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

    void tickLunge(float dt, float px, float py, float& playerHP) {
        (void)px; (void)py; (void)playerHP;
        lastX_ = worldX; lastY_ = worldY;
        worldX += lungeDirX_ * lungeSpd_ * dt;
        worldY += lungeDirY_ * lungeSpd_ * dt;
        spawnAfterimage(lastX_, lastY_, true, 0.4f);
    }

    void enterStutter() {
        state = BossState::STUTTER;
        stateTimer = 0.0f;
        flickerOn_ = true;
        glitchAmount = 0.05f;
        if (phase3 && chainLunges_ == 0) chainLunges_ = 1;
    }

    void enterStalk() {
        state = BossState::STALK;
        stateTimer = 0.0f;
        chainLunges_ = 0;
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
