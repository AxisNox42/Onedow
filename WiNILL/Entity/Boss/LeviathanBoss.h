#pragma once

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

#include "DrawPrim.h"

// LEVIATHAN is deliberately a spectacle boss.  The body never becomes a
// damage target; the only gameplay target is the heart during HEART_EXPOSED.
// Keeping the encounter here, instead of turning every constellation node
// into a Monster, lets the large node budget stay purely visual and batched.
enum class LeviathanState : int {
    ARRIVAL = 0,
    MIGRATION,
    DIVE_TO_HEART,
    HEART_EXPOSED,
    RESURFACE,
};

class LeviathanBoss {
public:
    static constexpr const wchar_t* BOSS_NAME = L"LEVIATHAN";
    static constexpr float HEART_RADIUS = 112.0f;

    float worldX = 0.0f;
    float worldY = 0.0f;
    float hp = 1.0f;
    float maxHp = 1.0f;
    bool  alive = true;
    bool  exploded = false;

    int screenW = 1280;
    int screenH = 720;
    LeviathanState state = LeviathanState::ARRIVAL;
    float stateTimer = 0.0f;
    float elapsed = 0.0f;
    float heading = 0.0f;
    float heartCX = 0.0f;
    float heartCY = 0.0f;
    float heartPulse = 0.0f;
    float hitFlash = 0.0f;
    int   heartBand = 0;          // four 25% windows: 100-75-50-25-0
    float heartFloor = 0.0f;      // HP floor for the current exposure window
    bool  heartOpenedEvent = false;
    bool  heartClosedEvent = false;

    LeviathanBoss(int w, int h, float hpIn)
        : screenW(std::max(640, w)), screenH(std::max(360, h)),
          hp(std::max(1.0f, hpIn)), maxHp(std::max(1.0f, hpIn)) {
        heartCX = screenW * 0.5f;
        heartCY = screenH * 0.48f;
        worldX = -bodyLength() * 0.60f;
        worldY = screenH * 0.30f;
        heartFloor = heartFloorForBand();
    }

    bool heartVulnerable() const {
        return alive && state == LeviathanState::HEART_EXPOSED;
    }

    bool bodyInvulnerable() const { return alive; }

    float heartHitRadius() const {
        const float p = 0.5f + 0.5f * std::sinf(heartPulse * 5.2f);
        return HEART_RADIUS + p * 10.0f;
    }

    float hpRatio() const {
        return maxHp > 0.0f ? std::max(0.0f, hp / maxHp) : 0.0f;
    }

    const wchar_t* stateTag() const {
        switch (state) {
        case LeviathanState::ARRIVAL:       return L"ARRIVAL // THE SKY MOVES";
        case LeviathanState::MIGRATION:     return L"MIGRATION // STAR CURRENT";
        case LeviathanState::DIVE_TO_HEART: return L"DIVE // HEART SIGNAL FOUND";
        case LeviathanState::HEART_EXPOSED: return L"HEART EXPOSED // STRIKE NOW";
        case LeviathanState::RESURFACE:     return L"RESURFACE // THE CURRENT TURNS";
        default:                            return L"UNKNOWN CURRENT";
        }
    }

    // Returns the damage actually accepted.  Each exposure can remove only
    // one 25% HP band, so a missed heart window is a retry rather than a free
    // skip through the entire encounter.
    float applyHeartDamage(float incoming) {
        if (!heartVulnerable() || incoming <= 0.0f) return 0.0f;
        float allowed = std::max(0.0f, hp - heartFloor);
        float dealt = std::min(incoming, allowed);
        if (dealt <= 0.0f) return 0.0f;
        hp -= dealt;
        if (hp < heartFloor) hp = heartFloor;
        heartPulse = 0.0f;
        hitFlash = 1.0f;
        if (heartBand >= 3 && hp <= 0.001f) {
            hp = 0.0f;
            alive = false;
        }
        return dealt;
    }

    bool heartHitPoint(float x, float y, float extra = 0.0f) const {
        if (!heartVulnerable()) return false;
        const float r = heartHitRadius() + extra;
        const float dx = x - heartCX, dy = y - heartCY;
        return dx * dx + dy * dy <= r * r;
    }

    bool heartHitSegment(float x0, float y0, float x1, float y1,
                         float extra = 0.0f) const {
        if (!heartVulnerable()) return false;
        const float vx = x1 - x0, vy = y1 - y0;
        const float wx = heartCX - x0, wy = heartCY - y0;
        const float vv = vx * vx + vy * vy;
        float u = vv > 0.0001f ? (wx * vx + wy * vy) / vv : 0.0f;
        u = std::max(0.0f, std::min(1.0f, u));
        const float px = x0 + vx * u, py = y0 + vy * u;
        const float r = heartHitRadius() + extra;
        const float dx = px - heartCX, dy = py - heartCY;
        return dx * dx + dy * dy <= r * r;
    }

    bool heartHitRadius(float x, float y, float radius) const {
        if (!heartVulnerable()) return false;
        const float dx = x - heartCX, dy = y - heartCY;
        const float r = heartHitRadius() + std::max(0.0f, radius);
        return dx * dx + dy * dy <= r * r;
    }

    void Update(float playerX, float playerY, float dt) {
        (void)playerX;
        (void)playerY;
        if (!alive || dt <= 0.0f) return;

        elapsed += dt;
        stateTimer += dt;
        heartPulse += dt;
        hitFlash = std::max(0.0f, hitFlash - dt * 3.8f);
        heartOpenedEvent = false;
        heartClosedEvent = false;

        switch (state) {
        case LeviathanState::ARRIVAL: {
            const float u = std::min(1.0f, stateTimer / 3.20f);
            worldX = -bodyLength() * 0.62f +
                     (screenW * 0.34f + bodyLength() * 0.62f) * u;
            worldY = screenH * 0.30f + std::sinf(elapsed * 1.1f) * 34.0f;
            heading = 0.10f + std::sinf(elapsed * 0.8f) * 0.06f;
            if (stateTimer >= 3.20f) changeState(LeviathanState::MIGRATION);
        } break;

        case LeviathanState::MIGRATION: {
            const float phase = elapsed * 0.46f;
            worldX = screenW * 0.50f + std::sinf(phase) * (screenW * 0.66f);
            worldY = screenH * 0.27f + std::sinf(phase * 1.71f) * screenH * 0.18f;
            const float targetHeading = 0.16f + std::sinf(phase * 0.73f) * 0.075f;
            heading += (targetHeading - heading) * std::min(1.0f, dt * 2.2f);
            if (stateTimer >= 4.80f) {
                changeState(LeviathanState::DIVE_TO_HEART);
            }
        } break;

        case LeviathanState::DIVE_TO_HEART: {
            const float u = std::min(1.0f, stateTimer / 1.15f);
            const float eased = u * u * (3.0f - 2.0f * u);
            const float sx = worldX, sy = worldY;
            worldX = sx + (heartCX - sx) * eased;
            worldY = sy + (screenH * 0.08f - sy) * eased;
            heading = 0.16f - 0.08f * eased;
            if (stateTimer >= 1.15f) {
                heartFloor = heartFloorForBand();
                changeState(LeviathanState::HEART_EXPOSED);
                heartOpenedEvent = true;
            }
        } break;

        case LeviathanState::HEART_EXPOSED:
            worldX = screenW * 0.50f;
            worldY = screenH * 0.08f;
            heading = 0.12f;
            // Give the player a readable target window even after the floor
            // for this band has been reached.
            if (stateTimer >= 4.60f && alive) {
                heartClosedEvent = true;
                changeState(LeviathanState::RESURFACE);
            }
            break;

        case LeviathanState::RESURFACE:
            worldY -= 170.0f * dt;
            worldX += 95.0f * dt;
            heading = 0.12f;
            if (stateTimer >= 1.20f) {
                if (hp <= heartFloor + 0.01f) {
                    if (heartBand < 3) ++heartBand;
                }
                if (hp <= 0.001f) {
                    alive = false;
                    break;
                }
                changeState(LeviathanState::MIGRATION);
            }
            break;
        }
    }

    void renderBody(float t) const {
        const float L = bodyLength();
        const float W = bodyWidth();
        const float pulse = 0.5f + 0.5f * std::sinf(t * 1.8f);
        const float stateAlpha = state == LeviathanState::HEART_EXPOSED ? 0.22f :
                                 state == LeviathanState::DIVE_TO_HEART ? 0.52f : 0.90f;
        const glm::vec3 ink(0.82f, 0.94f, 1.0f);
        const glm::vec3 white(1.0f, 1.0f, 1.0f);
        const glm::vec3 shadow(0.003f, 0.008f, 0.018f);

        struct N { float x, y; };
        auto local = [&](N n) {
            return localPoint(n.x * L, n.y * W);
        };
        auto xy = [&](float x, float y) {
            return localPoint(x * L, y * W);
        };
        auto dot = [&](const glm::vec2& p, float size, float alpha,
                       bool bright = false) {
            const float glow = bright ? 0.20f : 0.10f;
            drawCircle(p.x, p.y, size * 2.4f, ink.r, ink.g, ink.b,
                       alpha * glow);
            drawDiamond(p.x, p.y, size, bright ? white.r : ink.r,
                        bright ? white.g : ink.g, bright ? white.b : ink.b,
                        alpha);
        };
        auto fill = [&](const N* q, int count, float alpha) {
            glm::vec2 c(0.0f);
            for (int i = 0; i < count; ++i) c += local(q[i]);
            c /= (float)count;
            for (int i = 0; i < count; ++i) {
                const glm::vec2 a = local(q[i]);
                const glm::vec2 b = local(q[(i + 1) % count]);
                BatchTri(c.x, c.y, a.x, a.y, b.x, b.y,
                         shadow.r, shadow.g, shadow.b, alpha);
            }
        };
        auto chain = [&](const N* q, int count, float width, float alpha,
                         bool close = false, bool brightNodes = false) {
            for (int i = 1; i < count; ++i)
                drawLine(local(q[i - 1]), local(q[i]), width, ink, alpha);
            if (close && count > 2)
                drawLine(local(q[count - 1]), local(q[0]), width, ink, alpha);
            for (int i = 0; i < count; ++i)
                dot(local(q[i]), width * (brightNodes ? 1.15f : 0.85f),
                    alpha * 0.88f, brightNodes && (i % 3 == 0));
        };

        // One fixed side profile: head at x+, raised tail at x-, and the
        // oversized near-side fin hanging below the belly.
        const N outline[] = {
            {-0.48f,-0.04f}, {-0.42f,-0.14f}, {-0.32f,-0.23f},
            {-0.18f,-0.30f}, {-0.02f,-0.33f}, { 0.16f,-0.31f},
            { 0.32f,-0.25f}, { 0.45f,-0.16f}, { 0.56f,-0.06f},
            { 0.64f, 0.01f}, { 0.59f, 0.08f}, { 0.48f, 0.13f},
            { 0.34f, 0.17f}, { 0.23f, 0.22f}, { 0.06f, 0.28f},
            {-0.12f, 0.30f}, {-0.28f, 0.25f}, {-0.40f, 0.14f},
            {-0.48f, 0.05f}
        };
        const N tailA[] = {
            {-0.45f,0.01f}, {-0.55f,-0.10f}, {-0.62f,-0.27f},
            {-0.74f,-0.45f}, {-0.68f,-0.53f}, {-0.54f,-0.49f},
            {-0.42f,-0.36f}, {-0.37f,-0.17f}
        };
        const N tailB[] = {
            {-0.45f,0.03f}, {-0.57f,0.09f}, {-0.70f,0.18f},
            {-0.82f,0.31f}, {-0.76f,0.39f}, {-0.62f,0.34f},
            {-0.48f,0.23f}, {-0.37f,0.10f}
        };
        const N farFin[] = {
            {-0.04f,-0.17f}, {-0.01f,-0.34f}, {0.04f,-0.53f},
            {0.13f,-0.66f}, {0.20f,-0.54f}, {0.15f,-0.34f},
            {0.10f,-0.17f}
        };
        const N nearFin[] = {
            {0.00f,0.14f}, {0.08f,0.28f}, {0.16f,0.47f},
            {0.23f,0.66f}, {0.31f,0.76f}, {0.37f,0.68f},
            {0.29f,0.47f}, {0.18f,0.23f}
        };

        fill(tailA, 8, stateAlpha * 0.38f);
        fill(tailB, 8, stateAlpha * 0.38f);
        fill(farFin, 7, stateAlpha * 0.28f);
        fill(outline, 19, stateAlpha * 0.56f);
        fill(nearFin, 8, stateAlpha * 0.42f);

        chain(tailA, 8, 2.5f, stateAlpha * 0.90f, true, true);
        chain(tailB, 8, 2.5f, stateAlpha * 0.90f, true, true);
        chain(farFin, 7, 2.0f, stateAlpha * 0.65f, true, false);
        chain(outline, 19, 4.0f, stateAlpha * 0.98f, true, true);
        chain(nearFin, 8, 3.6f, stateAlpha * 0.90f, true, true);

        // Keep interior information sparse so the contour remains the boss's
        // identity instead of turning into another noisy constellation.
        const N back[] = {
            {-0.38f,-0.10f}, {-0.22f,-0.18f}, {-0.03f,-0.22f},
            { 0.16f,-0.20f}, { 0.34f,-0.14f}, { 0.50f,-0.06f}
        };
        const N belly[] = {
            {-0.35f,0.10f}, {-0.19f,0.18f}, {0.0f,0.23f},
            {0.18f,0.20f}, {0.34f,0.14f}, {0.49f,0.07f}
        };
        const N jaw[] = {
            {0.61f,0.045f}, {0.53f,0.11f}, {0.40f,0.15f},
            {0.27f,0.17f}, {0.14f,0.14f}
        };
        chain(back, 6, 1.5f, stateAlpha * 0.62f, false, false);
        chain(belly, 6, 1.4f, stateAlpha * 0.58f, false, false);
        chain(jaw, 5, 2.1f, stateAlpha * 0.85f, false, true);

        for (int i = 0; i < 6; ++i) {
            const float x = -0.17f + (float)i * 0.070f;
            drawLine(xy(x, -0.20f), xy(x + 0.045f, 0.18f),
                     1.2f, ink, stateAlpha * 0.42f);
            dot(xy(x + 0.02f, 0.02f), 2.0f, stateAlpha * 0.58f, false);
        }

        const glm::vec2 eye = xy(0.43f, -0.14f);
        dot(eye, 5.5f + pulse * 1.2f, stateAlpha, true);
        drawCircle(eye.x, eye.y, 14.0f + pulse * 3.0f,
                   0.18f, 0.76f, 1.0f, stateAlpha * 0.20f);
        dot(xy(0.28f, -0.22f), 3.0f, stateAlpha * 0.70f, false);
        for (int i = 0; i < 4; ++i) {
            const float x = 0.49f + (float)i * 0.018f;
            drawLine(xy(x, 0.09f), xy(x + 0.03f, 0.14f),
                     1.0f, white, stateAlpha * 0.50f);
        }

        // Five quiet wake curves echo the reference's orbit lines without
        // competing with the whale's silhouette.
        for (int k = 0; k < 5; ++k) {
            const float off = -0.14f + (float)k * 0.07f;
            glm::vec2 prev = xy(-0.44f, off);
            for (int j = 1; j <= 14; ++j) {
                const float u = (float)j / 14.0f;
                const float x = -0.44f - u * (0.52f + k * 0.02f);
                const float y = off + std::sinf(u * 4.0f + t * 0.35f + k) *
                                 (0.035f + u * 0.065f);
                const glm::vec2 cur = xy(x, y);
                drawLine(prev, cur, 1.2f, ink,
                         stateAlpha * (0.26f - u * 0.10f));
                if ((j % 5) == 0)
                    dot(cur, 1.8f, stateAlpha * 0.42f, false);
                prev = cur;
            }
        }
    }

    void renderTelegraph(float t) const {
        if (state != LeviathanState::DIVE_TO_HEART &&
            state != LeviathanState::HEART_EXPOSED &&
            state != LeviathanState::RESURFACE) return;

        const float open = state == LeviathanState::HEART_EXPOSED ? 1.0f :
                           std::min(1.0f, stateTimer / 1.15f);
        const float pulse = 0.5f + 0.5f * std::sinf(t * 4.5f);
        drawCircle(heartCX, heartCY, (170.0f + pulse * 28.0f) * open,
                   0.08f, 0.62f, 0.82f, 0.07f * open);
        for (int ring = 0; ring < 3; ++ring) {
            const float r = (140.0f + ring * 42.0f) * (0.80f + open * 0.20f);
            for (int i = 0; i < 24; ++i) {
                if (((i + ring) & 1) != 0) continue;
                const float a0 = t * (0.35f + ring * 0.1f) +
                                 (float)i / 24.0f * 6.2831853f;
                const float a1 = a0 + 0.16f;
                drawLine(glm::vec2(heartCX + std::cos(a0) * r,
                                   heartCY + std::sin(a0) * r),
                         glm::vec2(heartCX + std::cos(a1) * r,
                                   heartCY + std::sin(a1) * r),
                         2.2f, glm::vec3(0.25f, 0.82f, 1.0f),
                         0.34f * open);
            }
        }
    }

    void renderHeart(float t) const {
        if (state != LeviathanState::DIVE_TO_HEART &&
            state != LeviathanState::HEART_EXPOSED &&
            state != LeviathanState::RESURFACE) return;

        const bool exposed = state == LeviathanState::HEART_EXPOSED;
        const float fade = state == LeviathanState::RESURFACE ?
                           std::max(0.0f, 1.0f - stateTimer / 1.20f) :
                           state == LeviathanState::DIVE_TO_HEART ?
                           std::min(1.0f, stateTimer / 1.15f) : 1.0f;
        const float beat = 0.5f + 0.5f * std::sinf(heartPulse * 5.2f);
        const float r = heartHitRadius();
        const float alpha = fade * (exposed ? 1.0f : 0.64f);
        const glm::vec3 core(1.0f, 0.28f, 0.46f);
        const glm::vec3 edge(0.30f, 0.82f, 1.0f);

        drawCircle(heartCX, heartCY, r * (2.1f + beat * 0.35f),
                   core.r, core.g, core.b, 0.08f * alpha);
        drawCircle(heartCX, heartCY, r * 1.34f,
                   edge.r, edge.g, edge.b, 0.12f * alpha);
        for (int i = 0; i < 28; ++i) {
            const float a = t * 0.7f + (float)i / 28.0f * 6.2831853f;
            const float rr = r * (1.08f + 0.16f * std::sinf(i * 2.3f));
            drawDiamond(heartCX + std::cos(a) * rr,
                        heartCY + std::sin(a) * rr,
                        4.0f + (i % 3), edge.r, edge.g, edge.b,
                        alpha * (0.35f + 0.35f * beat));
        }
        const glm::vec2 p0(heartCX - r * 0.47f, heartCY - r * 0.20f);
        const glm::vec2 p1(heartCX - r * 0.13f, heartCY - r * 0.50f);
        const glm::vec2 p2(heartCX + r * 0.22f, heartCY - r * 0.17f);
        const glm::vec2 p3(heartCX + r * 0.03f, heartCY + r * 0.50f);
        const glm::vec2 p4(heartCX - r * 0.28f, heartCY + r * 0.12f);
        drawLine(p0, p1, 8.0f, core, alpha * 0.92f);
        drawLine(p1, p2, 8.0f, core, alpha * 0.92f);
        drawLine(p2, p3, 8.0f, core, alpha * 0.92f);
        drawLine(p3, p4, 8.0f, core, alpha * 0.92f);
        drawLine(p4, p0, 8.0f, core, alpha * 0.92f);
        drawCircle(heartCX, heartCY, r * (0.40f + beat * 0.08f),
                   0.22f, 0.04f, 0.10f, alpha * 0.88f);
        drawDiamond(heartCX, heartCY, r * (0.30f + beat * 0.08f),
                    1.0f, 0.72f, 0.82f, alpha);
        drawCircle(heartCX, heartCY, 8.0f + beat * 4.0f,
                   1.0f, 1.0f, 1.0f, alpha);
        if (exposed) {
            drawCircle(heartCX, heartCY, r * (1.25f + beat * 0.16f),
                       1.0f, 0.35f, 0.55f, 0.16f + beat * 0.12f);
        }
    }

private:
    float bodyLength() const {
        return std::max(960.0f, (float)screenW * 1.38f);
    }

    float bodyWidth() const {
        return std::max(300.0f, (float)screenH * 0.48f);
    }

    float heartFloorForBand() const {
        const float f = 0.75f - 0.25f * (float)heartBand;
        return maxHp * std::max(0.0f, f);
    }

    void changeState(LeviathanState next) {
        state = next;
        stateTimer = 0.0f;
        if (next == LeviathanState::HEART_EXPOSED)
            heartFloor = heartFloorForBand();
    }

    glm::vec2 localPoint(float x, float y) const {
        const float c = std::cos(heading), s = std::sin(heading);
        return glm::vec2(worldX + c * x - s * y,
                         worldY + s * x + c * y);
    }

    static void drawLine(const glm::vec2& a, const glm::vec2& b,
                         float width, const glm::vec3& color, float alpha) {
        const glm::vec2 d = b - a;
        const float len = std::sqrt(d.x * d.x + d.y * d.y);
        if (len <= 0.001f) return;
        const glm::vec2 n(-d.y / len * width * 0.5f,
                         d.x / len * width * 0.5f);
        BatchTri(a.x - n.x, a.y - n.y, a.x + n.x, a.y + n.y,
                 b.x + n.x, b.y + n.y, color.r, color.g, color.b, alpha);
        BatchTri(a.x - n.x, a.y - n.y, b.x + n.x, b.y + n.y,
                 b.x - n.x, b.y - n.y, color.r, color.g, color.b, alpha);
    }
};
