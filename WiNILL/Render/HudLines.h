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

// Closed polyline through n corners with mitred joins: each corner is one
// shared vertex pair, so no part of the stroke is drawn twice and a faded
// (alpha < 1) outline shows no darker overlap at the corners. The corners
// stay sharp. Keyline pass first when requested.
inline void ClosedOutline(const float* px, const float* py, int n, float thick,
                          Rgb c, float a, float keyline) {
    constexpr int kMaxCorners = 16;
    if (n < 3 || n > kMaxCorners) return;
    for (int pass = keyline > 0.0f ? 0 : 1; pass < 2; ++pass) {
        const float w = pass == 0 ? thick + keyline : thick;
        const Rgb col = pass == 0 ? KeyCol(c) : c;
        const float alpha = pass == 0 ? kKeyA * a : a;
        float ox[kMaxCorners], oy[kMaxCorners], ix[kMaxCorners], iy[kMaxCorners];
        for (int i = 0; i < n; ++i) {
            const int h = (i + n - 1) % n, j = (i + 1) % n;
            float d1x = px[i] - px[h], d1y = py[i] - py[h];
            float d2x = px[j] - px[i], d2y = py[j] - py[i];
            const float l1 = std::max(0.001f, std::sqrt(d1x * d1x + d1y * d1y));
            const float l2 = std::max(0.001f, std::sqrt(d2x * d2x + d2y * d2y));
            d1x /= l1; d1y /= l1; d2x /= l2; d2y /= l2;
            // Mitre direction bisects the two edge normals.
            float mx = -d1y - d2y, my = d1x + d2x;
            const float ml = std::max(0.001f, std::sqrt(mx * mx + my * my));
            mx /= ml; my /= ml;
            const float cosHalf = std::max(0.25f, mx * -d1y + my * d1x);
            const float reach = w * 0.5f / cosHalf;
            ox[i] = px[i] + mx * reach; oy[i] = py[i] + my * reach;
            ix[i] = px[i] - mx * reach; iy[i] = py[i] - my * reach;
        }
        for (int i = 0; i < n; ++i) {
            const int j = (i + 1) % n;
            BatchTri(ox[i], oy[i], ox[j], oy[j], ix[j], iy[j], col.r, col.g, col.b, alpha);
            BatchTri(ox[i], oy[i], ix[j], iy[j], ix[i], iy[i], col.r, col.g, col.b, alpha);
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

// Equilateral triangle pointing down, centred on its centroid, with
// circumradius r (top edge at y - r/2, bottom vertex at y + r).
inline void TriangleDownOutline(float cx, float cy, float r, float thick, Rgb c,
                                float a, float keyline) {
    const float hx = r * 0.8660254f;
    const float px[3] = { cx - hx, cx + hx, cx };
    const float py[3] = { cy - r * 0.5f, cy - r * 0.5f, cy + r };
    ClosedOutline(px, py, 3, thick, c, a, keyline);
}

enum class FillShape { Circle, Diamond, Square, TriangleDown };

// Clockwise wedge from 12 o'clock covering frac of a node's interior.
// Circle: radius r. Diamond: vertices at top/right/bottom/left
// (|x| + |y| = r). Square: half side r (max(|x|, |y|) = r).
// TriangleDown: circumradius r, as TriangleDownOutline.
inline void SectorFill(float cx, float cy, float r, FillShape shape, float frac,
                       Rgb c, float a) {
    frac = std::clamp(frac, 0.0f, 1.0f);
    if (frac <= 0.0f) return;
    // 16 steps per quadrant land exactly on the diamond's corners; the
    // square's corners sit at 45°, which 64 steps also hit. The triangle's
    // corners (90°, -30°, 210°) need 96 steps (3.75°).
    const int kSteps = shape == FillShape::TriangleDown ? 96 : 64;
    const float step = 2.0f * kPi / kSteps;
    const float end = -0.5f * kPi + 2.0f * kPi * frac;
    auto edge = [&](float t, float& x, float& y) {
        const float ct = std::cos(t), st = std::sin(t);
        float d = r;
        if (shape == FillShape::Diamond) d = r / (std::fabs(ct) + std::fabs(st));
        else if (shape == FillShape::Square) d = r / std::max(std::fabs(ct), std::fabs(st));
        else if (shape == FillShape::TriangleDown) {
            // Edge normals at -90° (top), 30° and 150°; inradius r/2.
            const float n = std::max({ -st,
                                       ct * 0.8660254f + st * 0.5f,
                                       -ct * 0.8660254f + st * 0.5f });
            d = r * 0.5f / n;
        }
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

// Prefix of an open polyline (px/py, n points) covering frac of its length.
// Writes up to n points to ox/oy and returns how many; the last one is the
// interpolated end point.
inline int PathPrefix(const float* px, const float* py, int n, float frac,
                      float* ox, float* oy) {
    frac = std::clamp(frac, 0.0f, 1.0f);
    float total = 0.0f;
    for (int i = 0; i + 1 < n; ++i)
        total += std::hypot(px[i + 1] - px[i], py[i + 1] - py[i]);
    float left = total * frac;
    int m = 0;
    ox[m] = px[0]; oy[m] = py[0]; ++m;
    for (int i = 0; i + 1 < n && left > 0.0f; ++i) {
        const float len = std::hypot(px[i + 1] - px[i], py[i + 1] - py[i]);
        if (len <= 0.0f) continue;
        const float t = std::min(1.0f, left / len);
        ox[m] = px[i] + (px[i + 1] - px[i]) * t;
        oy[m] = py[i] + (py[i + 1] - py[i]) * t;
        ++m;
        left -= len;
    }
    return m;
}

// Open polyline stroke; keyline pass first, joints extended by half the
// stroke so corners stay sharp.
inline void Polyline(const float* px, const float* py, int n, float thick, Rgb c,
                     float a, float keyline) {
    for (int pass = keyline > 0.0f ? 0 : 1; pass < 2; ++pass) {
        const float w = pass == 0 ? thick + keyline : thick;
        const Rgb col = pass == 0 ? KeyCol(c) : c;
        const float alpha = pass == 0 ? kKeyA * a : a;
        for (int i = 0; i + 1 < n; ++i) {
            const float dx = px[i + 1] - px[i], dy = py[i + 1] - py[i];
            const float len = std::sqrt(dx * dx + dy * dy);
            if (len < 0.001f) continue;
            const float ex = dx / len * w * 0.5f, ey = dy / len * w * 0.5f;
            Segment(px[i] - ex, py[i] - ey, px[i + 1] + ex, py[i + 1] + ey, w, col, alpha);
        }
    }
}

// Fan from (cx, cy) over an open polyline: the wedge a perimeter progress
// sweeps, so a polygon's fill and its progress stroke always end together.
inline void FanFill(float cx, float cy, const float* px, const float* py, int n,
                    Rgb c, float a) {
    for (int i = 0; i + 1 < n; ++i)
        BatchTri(cx, cy, px[i], py[i], px[i + 1], py[i + 1], c.r, c.g, c.b, a);
}

// Clockwise perimeter path of TriangleDownOutline starting at 12 o'clock
// (the top edge's midpoint) and returning there: 5 points.
inline void TriangleDownPath(float cx, float cy, float r, float* px, float* py) {
    const float hx = r * 0.8660254f, top = cy - r * 0.5f;
    const float xs[5] = { cx, cx + hx, cx, cx - hx, cx };
    const float ys[5] = { top, top, cy + r, top, top };
    for (int i = 0; i < 5; ++i) { px[i] = xs[i]; py[i] = ys[i]; }
}

}  // namespace hud_lines
