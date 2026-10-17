#pragma once
// ─────────────────────────────────────────────────────────────
// Line primitives shared by the combat HUD (wave track, skill keys).
// Every glyph is drawn as solid strokes on the batch renderer; the only
// fill is SectorFill, the clockwise wedge that shows a timer inside a node.
// ─────────────────────────────────────────────────────────────
#include <algorithm>
#include <cmath>

#include "DrawPrim.h"

namespace hud_lines {

struct Rgb { float r, g, b; };
// Keyline under every line glyph: a dark shade of the glyph's own colour,
// like the text halo. On a dark wallpaper it sinks into the background; on
// a bright or saturated one it outlines the bright core, so one of the two
// always contrasts without a plate behind the glyph.
inline constexpr float kKeyA = 0.88f;
inline constexpr float kKeyShade = 0.18f;
inline Rgb KeyCol(Rgb c) { return { c.r * kKeyShade, c.g * kKeyShade, c.b * kKeyShade }; }
inline constexpr float kPi = 3.14159265f;

inline void Segment(float x0, float y0, float x1, float y1, float w,
                    Rgb c, float a) {
    const float dx = x1 - x0, dy = y1 - y0;
    const float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.001f) return;
    const float nx = -dy / len * w * 0.5f, ny = dx / len * w * 0.5f;
    BatchTri(x0 + nx, y0 + ny, x1 + nx, y1 + ny, x1 - nx, y1 - ny, c.r, c.g, c.b, a);
    BatchTri(x0 + nx, y0 + ny, x1 - nx, y1 - ny, x0 - nx, y0 - ny, c.r, c.g, c.b, a);
}

// Arc from angle a0 sweeping `sweep` radians (screen space, clockwise).
inline void Arc(float cx, float cy, float radius, float thick, float a0,
                float sweep, Rgb c, float a) {
    const int segments = std::max(2, (int)(std::fabs(sweep) / (2.0f * kPi) * 40.0f));
    const float inner = radius - thick * 0.5f, outer = radius + thick * 0.5f;
    for (int i = 0; i < segments; ++i) {
        const float t0 = a0 + sweep * i / segments;
        const float t1 = a0 + sweep * (i + 1) / segments;
        const float c0 = std::cos(t0), s0 = std::sin(t0);
        const float c1 = std::cos(t1), s1 = std::sin(t1);
        BatchTri(cx + c0 * outer, cy + s0 * outer, cx + c1 * outer, cy + s1 * outer,
                 cx + c1 * inner, cy + s1 * inner, c.r, c.g, c.b, a);
        BatchTri(cx + c0 * outer, cy + s0 * outer, cx + c1 * inner, cy + s1 * inner,
                 cx + c0 * inner, cy + s0 * inner, c.r, c.g, c.b, a);
    }
}

inline void Ring(float cx, float cy, float radius, float thick, Rgb c, float a,
                 float keyline) {
    if (keyline > 0.0f)
        Arc(cx, cy, radius, thick + keyline, 0.0f, 2.0f * kPi, KeyCol(c), kKeyA * a);
    Arc(cx, cy, radius, thick, 0.0f, 2.0f * kPi, c, a);
}

// Closed polyline through n corners; each edge is extended by half the
// stroke so the corners stay sharp. Keyline pass first when requested.
inline void ClosedOutline(const float* px, const float* py, int n, float thick,
                          Rgb c, float a, float keyline) {
    for (int pass = keyline > 0.0f ? 0 : 1; pass < 2; ++pass) {
        const float w = pass == 0 ? thick + keyline : thick;
        const Rgb col = pass == 0 ? KeyCol(c) : c;
        const float alpha = pass == 0 ? kKeyA * a : a;
        for (int i = 0; i < n; ++i) {
            const int j = (i + 1) % n;
            const float dx = px[j] - px[i], dy = py[j] - py[i];
            const float len = std::sqrt(dx * dx + dy * dy);
            if (len < 0.001f) continue;
            const float ex = dx / len * w * 0.5f, ey = dy / len * w * 0.5f;
            Segment(px[i] - ex, py[i] - ey, px[j] + ex, py[j] + ey, w, col, alpha);
        }
    }
}

inline void DiamondOutline(float cx, float cy, float r, float thick, Rgb c,
                           float a, float keyline) {
    const float px[4] = { cx, cx + r, cx, cx - r };
    const float py[4] = { cy - r, cy, cy + r, cy };
    ClosedOutline(px, py, 4, thick, c, a, keyline);
}

// Axis-aligned square with half side `half`.
inline void SquareOutline(float cx, float cy, float half, float thick, Rgb c,
                          float a, float keyline) {
    const float px[4] = { cx - half, cx + half, cx + half, cx - half };
    const float py[4] = { cy - half, cy - half, cy + half, cy + half };
    ClosedOutline(px, py, 4, thick, c, a, keyline);
}

enum class FillShape { Circle, Diamond, Square };

// Clockwise wedge from 12 o'clock covering frac of a node's interior.
// Circle: radius r. Diamond: vertices at top/right/bottom/left
// (|x| + |y| = r). Square: half side r (max(|x|, |y|) = r).
inline void SectorFill(float cx, float cy, float r, FillShape shape, float frac,
                       Rgb c, float a) {
    frac = std::clamp(frac, 0.0f, 1.0f);
    if (frac <= 0.0f) return;
    // 16 steps per quadrant land exactly on the diamond's corners; the
    // square's corners sit at 45°, which 64 steps also hit.
    constexpr int kSteps = 64;
    const float step = 2.0f * kPi / kSteps;
    const float end = -0.5f * kPi + 2.0f * kPi * frac;
    auto edge = [&](float t, float& x, float& y) {
        const float ct = std::cos(t), st = std::sin(t);
        float d = r;
        if (shape == FillShape::Diamond) d = r / (std::fabs(ct) + std::fabs(st));
        else if (shape == FillShape::Square) d = r / std::max(std::fabs(ct), std::fabs(st));
        x = cx + ct * d; y = cy + st * d;
    };
    float px, py;
    edge(-0.5f * kPi, px, py);
    for (int i = 1; i <= kSteps; ++i) {
        const float t = std::min(end, -0.5f * kPi + step * i);
        float qx, qy;
        edge(t, qx, qy);
        BatchTri(cx, cy, px, py, qx, qy, c.r, c.g, c.b, a);
        if (t >= end) break;
        px = qx; py = qy;
    }
}

inline void Dot(float cx, float cy, float r, Rgb c, float a, float keyline) {
    if (keyline > 0.0f) {
        const Rgb k = KeyCol(c);
        drawCircle(cx, cy, r + keyline * 0.5f, k.r, k.g, k.b, kKeyA * a);
    }
    drawCircle(cx, cy, r, c.r, c.g, c.b, a);
}

}  // namespace hud_lines
