#include "PlayerWeaponShell.h"

#include "../Entity/MonsterManager.h"
#include "../Entity/Weapons.h"
#include "Camera.h"
#include "DrawPrim.h"
#include "IconSystem.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>

extern MonsterManager g_MonsterManager;
extern int g_CurrentWeapon;
extern float g_MuzzleTimer;
enum class PlayerShellKind { Rifle, StaticField };

static PlayerShellKind CurrentPlayerShellKind() {
    return g_CurrentWeapon == (int)StartWeapon::SMG
        ? PlayerShellKind::StaticField : PlayerShellKind::Rifle;
}

static void DrawPlayerRotRect(float cx, float cy, float w, float h, float ang,
                              float r, float g, float b, float a) {
    float hx = w * 0.5f, hy = h * 0.5f;
    float c = cosf(ang), s = sinf(ang);
    float x0 = -hx, y0 = -hy;
    float x1 =  hx, y1 = -hy;
    float x2 =  hx, y2 =  hy;
    float x3 = -hx, y3 =  hy;
    auto tx = [&](float x, float y) { return cx + x * c - y * s; };
    auto ty = [&](float x, float y) { return cy + x * s + y * c; };
    BatchTri(tx(x0,y0), ty(x0,y0), tx(x1,y1), ty(x1,y1), tx(x2,y2), ty(x2,y2), r,g,b,a);
    BatchTri(tx(x0,y0), ty(x0,y0), tx(x2,y2), ty(x2,y2), tx(x3,y3), ty(x3,y3), r,g,b,a);
}

static void DrawPlayerLine(float x0, float y0, float x1, float y1, float thick,
                           float r, float g, float b, float a) {
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.5f) return;
    DrawPlayerRotRect((x0 + x1) * 0.5f, (y0 + y1) * 0.5f, len, thick,
                      atan2f(dy, dx), r, g, b, a);
}
static void DrawPlayerHexFrame(float cx, float cy, float rad, float ang,
                               float r, float g, float b, float a, float thick,
                               bool nodes) {
    float vx[6], vy[6];
    for (int i = 0; i < 6; ++i) {
        float th = ang + (float)i * 1.04719755f;
        vx[i] = cx + cosf(th) * rad;
        vy[i] = cy + sinf(th) * rad;
    }
    for (int i = 0; i < 6; ++i) {
        int n = (i + 1) % 6;
        DrawPlayerLine(vx[i], vy[i], vx[n], vy[n], thick, r, g, b, a);
    }
    if (nodes) {
        float ns = std::max(4.0f, thick * 3.1f);
        for (int i = 0; i < 6; ++i)
            drawDiamond(vx[i], vy[i], ns, r, g, b, a * 1.12f);
    }
}

static void DrawPlayerTriangleFrame(float cx, float cy, float rad, float ang,
                                    float r, float g, float b, float a,
                                    float thick, bool nodes) {
    float vx[3], vy[3];
    for (int i = 0; i < 3; ++i) {
        float th = ang - 1.5707963f + (float)i * 2.0943951f;
        vx[i] = cx + cosf(th) * rad;
        vy[i] = cy + sinf(th) * rad;
    }
    for (int i = 0; i < 3; ++i) {
        int n = (i + 1) % 3;
        DrawPlayerLine(vx[i], vy[i], vx[n], vy[n], thick, r, g, b, a);
    }
    if (nodes) {
        float ns = std::max(4.0f, thick * 3.0f);
        for (int i = 0; i < 3; ++i)
            drawDiamond(vx[i], vy[i], ns, r, g, b, a * 1.08f);
    }
}

static void DrawPlayerCrossStarFrame(float cx, float cy, float outerRad, float innerRad,
                                     float ang, float r, float g, float b, float a,
                                     float thick, bool cross, bool nodes) {
    float vx[8], vy[8];
    for (int i = 0; i < 8; ++i) {
        float rad = (i & 1) ? innerRad : outerRad;
        float th = ang - 1.5707963f + (float)i * 0.78539816f;
        vx[i] = cx + cosf(th) * rad;
        vy[i] = cy + sinf(th) * rad;
    }

    for (int i = 0; i < 8; ++i) {
        int n = (i + 1) & 7;
        DrawPlayerLine(vx[i], vy[i], vx[n], vy[n], thick, r, g, b, a);
    }

    if (cross) {
        for (int i = 0; i < 8; i += 2)
            DrawPlayerLine(cx, cy, vx[i], vy[i], std::max(0.8f, thick * 0.42f),
                           r, g, b, a * 0.34f);
    }

    if (nodes) {
        float tipSz = std::max(4.0f, thick * 3.0f);
        for (int i = 0; i < 8; i += 2)
            drawDiamond(vx[i], vy[i], tipSz, r, g, b, a * 1.16f);
    }
}
static void DrawPlayerZigzagLine(float x0, float y0, float x1, float y1,
                                 int steps, float amp, float thick,
                                 float r, float g, float b, float a) {
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1.0f) return;
    float nx = -dy / len, ny = dx / len;
    float px = x0, py = y0;
    for (int i = 1; i <= steps; ++i) {
        float t = (float)i / (float)steps;
        float ox = (i == steps) ? 0.0f : (((i & 1) ? 1.0f : -1.0f) * amp);
        float cx2 = x0 + dx * t + nx * ox;
        float cy2 = y0 + dy * t + ny * ox;
        DrawPlayerLine(px, py, cx2, cy2, thick, r, g, b, a);
        px = cx2; py = cy2;
    }
}

// In-game regions remain rectangular for scissor/collision logic, but their
// visual identity is reduced to a light signal frame instead of a fake window.
void DrawInGameSignalFrame(float x, float y, float w, float h,
                                  float r, float g, float b, float alpha) {
    if (w < 24.0f || h < 24.0f || w != w || h != h) return;
    const float corner = std::max(10.0f, std::min(22.0f, std::min(w, h) * 0.12f));
    drawConstellFrame(x, y, w, h, r, g, b, alpha, corner, 3.0f, alpha * 0.12f, 1.0f);
}

void DrawPlayerWeaponShell(float cx, float cy, float sz, float aimAng) {
    if (!(aimAng == aimAng)) aimAng = 0.0f;

    const float r = 0.34f, g = 1.0f, b = 1.0f;
    const float wr = 0.92f, wg = 1.0f, wb = 1.0f;
    const float ar = 1.0f, ag = 0.62f, ab = 0.16f;
    float t = (float)glfwGetTime();
    float kick = (g_MuzzleTimer > 0.0f) ? (g_MuzzleTimer / 0.05f) : 0.0f;
    float pulse = 0.5f + 0.5f * sinf(t * 4.0f);
    auto rotX = [&](float lx, float ly, float ang) {
        return cx + lx * cosf(ang) - ly * sinf(ang);
    };
    auto rotY = [&](float lx, float ly, float ang) {
        return cy + lx * sinf(ang) + ly * cosf(ang);
    };
    auto triRot = [&](float ax, float ay, float bx, float by, float cx2, float cy2,
                      float ang, float rr, float gg, float bb, float aa) {
        BatchTri(rotX(ax, ay, ang), rotY(ax, ay, ang),
                 rotX(bx, by, ang), rotY(bx, by, ang),
                 rotX(cx2, cy2, ang), rotY(cx2, cy2, ang),
                 rr, gg, bb, aa);
    };
    auto triEdge = [&](float ax, float ay, float bx, float by, float cx2, float cy2,
                       float ang, float thick, float rr, float gg, float bb, float aa) {
        float x0 = rotX(ax, ay, ang), y0 = rotY(ax, ay, ang);
        float x1 = rotX(bx, by, ang), y1 = rotY(bx, by, ang);
        float x2 = rotX(cx2, cy2, ang), y2 = rotY(cx2, cy2, ang);
        DrawPlayerLine(x0, y0, x1, y1, thick, rr, gg, bb, aa);
        DrawPlayerLine(x1, y1, x2, y2, thick, rr, gg, bb, aa);
        DrawPlayerLine(x2, y2, x0, y0, thick, rr, gg, bb, aa);
    };

    PlayerShellKind kind = CurrentPlayerShellKind();
    switch (kind) {
    case PlayerShellKind::StaticField: {
        struct ZapTarget { float x, y, d2; };
        ZapTarget targets[4];
        int targetCount = 0;
        float fieldRad = std::max(150.0f, sz * 4.3f);
        float fieldR2 = fieldRad * fieldRad;
        auto addTarget = [&](float tx, float ty) {
            float dx = tx - cx, dy = ty - cy;
            float d2 = dx * dx + dy * dy;
            if (d2 > fieldR2) return;
            if (targetCount < 4) {
                targets[targetCount++] = { tx, ty, d2 };
            } else {
                int farIdx = 0;
                for (int i = 1; i < 4; ++i)
                    if (targets[i].d2 > targets[farIdx].d2) farIdx = i;
                if (d2 < targets[farIdx].d2)
                    targets[farIdx] = { tx, ty, d2 };
            }
        };
        for (auto m : g_MonsterManager.monsters)    if (m->alive)  addTarget(m->worldX,  m->worldY);
        for (auto rm : g_MonsterManager.rangedMobs) if (rm->alive) addTarget(rm->worldX, rm->worldY);

        float hitPulse = (targetCount > 0) ? (0.55f + 0.45f * sinf(t * 24.0f)) : 0.0f;
        float bodyPulse = pulse * 0.18f + hitPulse * 0.58f;
        float triAng = t * 1.90f;
        float hexAng = 0.5235988f - t * 0.72f;

        DrawPlayerHexFrame(cx, cy, fieldRad,
                           0.5235988f + t * 0.045f, r, g, b,
                           0.060f + hitPulse * 0.050f, 1.0f + hitPulse * 0.6f, false);
        DrawPlayerHexFrame(cx, cy, fieldRad * 0.72f,
                           0.5235988f - t * 0.030f, r, g, b,
                           0.035f + hitPulse * 0.040f, 0.8f, false);

        DrawPlayerTriangleFrame(cx, cy, sz * (0.82f + bodyPulse * 0.08f),
                                triAng, r, g, b, 0.78f + hitPulse * 0.20f,
                                2.0f + hitPulse * 1.2f, true);
        DrawPlayerHexFrame(cx, cy, sz * (1.17f + bodyPulse * 0.08f),
                           hexAng, r, g, b, 0.48f + hitPulse * 0.32f,
                           1.55f + hitPulse * 1.0f, true);
        DrawPlayerHexFrame(cx, cy, sz * 1.45f,
                           hexAng * 0.70f + 0.30f, r, g, b,
                           0.14f + hitPulse * 0.12f, 1.0f, false);

        drawCircle(cx, cy, sz * (0.34f + bodyPulse * 0.05f), 0.0f, 0.0f, 0.0f, 0.58f);
        drawCircle(cx, cy, sz * (0.27f + bodyPulse * 0.04f), r, g, b, 0.48f + bodyPulse * 0.28f);
        drawDiamond(cx, cy, sz * (0.22f + bodyPulse * 0.02f), wr, wg, wb, 0.78f + hitPulse * 0.18f);

        for (int i = 0; i < 6; ++i) {
            float va = hexAng + (float)i * 1.04719755f;
            float vx = cx + cosf(va) * sz * 1.17f;
            float vy = cy + sinf(va) * sz * 1.17f;
            DrawPlayerLine(cx + cosf(va) * sz * 0.40f, cy + sinf(va) * sz * 0.40f,
                           vx, vy, 0.9f + hitPulse * 0.8f, r, g, b,
                           0.16f + hitPulse * 0.18f);
        }

        for (int i = 0; i < targetCount; ++i) {
            float srcAng = hexAng + (float)(i % 6) * 1.04719755f;
            float sx = cx + cosf(srcAng) * sz * 1.18f;
            float sy = cy + sinf(srcAng) * sz * 1.18f;
            float flicker = 0.70f + 0.30f * sinf(t * 38.0f + (float)i * 1.7f);
            DrawPlayerZigzagLine(sx, sy, targets[i].x, targets[i].y,
                                 5, 5.0f + hitPulse * 4.0f,
                                 1.25f + hitPulse * 0.9f,
                                 wr, wg, wb, (0.28f + hitPulse * 0.36f) * flicker);
            drawDiamond(targets[i].x, targets[i].y, 5.0f + hitPulse * 2.5f,
                        r, g, b, 0.34f + hitPulse * 0.28f);
        }
        break;
    }
    case PlayerShellKind::Rifle:
    default: {
        float bounce = sinf((1.0f - kick) * 3.1415926f) * 0.035f;
        float outerScale = std::max(0.66f, 1.0f - kick * 0.24f + bounce);
        float innerScale = std::max(0.70f, 1.0f - kick * 0.18f + bounce * 0.65f);
        float outerAng = t * 2.25f;
        float innerAng = -t * 1.06f + 0.78539816f;
        float scopeAng = -t * 0.30f;

        auto rx = [&](float lx, float ly) {
            return cx + lx * cosf(scopeAng) - ly * sinf(scopeAng);
        };
        auto ry = [&](float lx, float ly) {
            return cy + lx * sinf(scopeAng) + ly * cosf(scopeAng);
        };

        drawCircle(cx, cy, sz * 0.54f, 0.0f, 0.0f, 0.0f, 0.26f);
        {
            float orbitR = sz * 1.86f;
            const int DASHES = 32;
            for (int i = 0; i < DASHES; ++i) {
                if ((i & 1) != 0) continue;
                float a0 = scopeAng + (float)i / (float)DASHES * 6.2831853f;
                float a1 = scopeAng + ((float)i + 0.42f) / (float)DASHES * 6.2831853f;
                DrawPlayerLine(cx + cosf(a0) * orbitR, cy + sinf(a0) * orbitR,
                               cx + cosf(a1) * orbitR, cy + sinf(a1) * orbitR,
                               0.85f, r, g, b, 0.22f);
            }
            for (int i = 0; i < 2; ++i) {
                float na = scopeAng * 1.55f + (float)i * 3.1415926f;
                drawDiamond(cx + cosf(na) * orbitR, cy + sinf(na) * orbitR,
                            sz * 0.095f, wr, wg, wb, 0.52f + kick * 0.18f);
            }

            float corner = sz * 1.45f;
            float leg = sz * 0.34f;
            for (int ix = -1; ix <= 1; ix += 2) {
                for (int iy = -1; iy <= 1; iy += 2) {
                    float x0 = (float)ix * corner;
                    float y0 = (float)iy * corner;
                    DrawPlayerLine(rx(x0, y0), ry(x0, y0),
                                   rx(x0 - (float)ix * leg, y0), ry(x0 - (float)ix * leg, y0),
                                   1.35f + kick * 0.35f, r, g, b, 0.34f + kick * 0.14f);
                    DrawPlayerLine(rx(x0, y0), ry(x0, y0),
                                   rx(x0, y0 - (float)iy * leg), ry(x0, y0 - (float)iy * leg),
                                   1.35f + kick * 0.35f, r, g, b, 0.34f + kick * 0.14f);
                    drawDiamond(rx(x0, y0), ry(x0, y0),
                                sz * 0.060f, wr, wg, wb, 0.36f + kick * 0.22f);
                }
            }
        }
        DrawPlayerCrossStarFrame(cx, cy, sz * 1.24f * outerScale, sz * 0.33f * outerScale,
                                 outerAng, r, g, b, 0.42f + kick * 0.18f,
                                 1.35f + kick * 0.45f, true, true);
        DrawPlayerCrossStarFrame(cx, cy, sz * 0.76f * innerScale, sz * 0.22f * innerScale,
                                 innerAng, wr, wg, wb, 0.74f + kick * 0.22f,
                                 2.35f + kick * 1.10f, true, true);

        for (int i = 0; i < 4; ++i) {
            float oa = outerAng - 1.5707963f + (float)i * 1.5707963f;
            float ia = innerAng - 1.5707963f + (float)i * 1.5707963f;
            drawDiamond(cx + cosf(oa) * sz * 1.24f * outerScale,
                        cy + sinf(oa) * sz * 1.24f * outerScale,
                        sz * (0.080f + kick * 0.050f), wr, wg, wb,
                        0.24f + kick * 0.66f);
            drawDiamond(cx + cosf(ia) * sz * 0.76f * innerScale,
                        cy + sinf(ia) * sz * 0.76f * innerScale,
                        sz * (0.070f + kick * 0.045f), r, g, b,
                        0.38f + kick * 0.52f);
        }

        float core = sz * (0.30f + kick * 0.025f);
        drawRect(cx - core * 0.52f, cy - core * 0.52f,
                 core * 1.04f, core * 1.04f, 0.0f, 0.0f, 0.0f, 0.66f);
        drawDiamond(cx, cy, core * 0.92f, r, g, b, 0.72f + kick * 0.18f);
        drawDiamond(cx, cy, core * 0.46f, wr, wg, wb, 0.84f + kick * 0.12f);
        break;
    }
    }
}
