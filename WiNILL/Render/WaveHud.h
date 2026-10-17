#pragma once
// ─────────────────────────────────────────────────────────────
// Wave HUD (top centre): "WAVE 04 / 15" and the time left in the wave,
// and a track with one node per wave (symbols only, no labels).
// Line glyphs only; text keeps the renderer halo so
// it reads on a bright wallpaper without a plate.
//   ○ plain wave   ◇ elite wave   ◈ final wave
// A fifth wave counts as elite only when its scheduled enemy is an elite
// kind (IsEliteMob); EASY's Regulus/Antares waves stay plain.
// ─────────────────────────────────────────────────────────────
#include <algorithm>
#include <cmath>
#include <cstdio>

#include "DrawPrim.h"
#include "EnemySpawner.h"
#include "GameManager.h"
#include "Monster.h"
#include "SceneUI.h"
#include "Settings.h"
#include "TextRenderer.h"
#include "WaveSystem.h"

extern TextRenderer g_TextL;
extern TextRenderer g_TextS;

namespace wave_hud {

struct Rgb { float r, g, b; };
inline constexpr Rgb kPast   = { 0.47f, 0.87f, 1.00f };
inline constexpr Rgb kNext   = { 0.66f, 0.70f, 0.80f };
inline constexpr Rgb kTrack  = { 0.36f, 0.41f, 0.53f };
inline constexpr Rgb kElite  = { 1.00f, 0.57f, 0.25f };
inline constexpr Rgb kFinal  = { 1.00f, 0.81f, 0.38f };
inline constexpr Rgb kDanger = { 1.00f, 0.36f, 0.38f };
inline constexpr Rgb kText   = { 0.86f, 0.91f, 1.00f };
// Keyline under every line glyph: a dark shade of the glyph's own colour,
// like the text halo. On a dark wallpaper it sinks into the background; on
// a bright or saturated one it outlines the bright core, so one of the two
// always contrasts without a plate behind the track.
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

inline void DiamondOutline(float cx, float cy, float r, float thick, Rgb c,
                           float a, float keyline) {
    const float px[5] = { cx, cx + r, cx, cx - r, cx };
    const float py[5] = { cy - r, cy, cy + r, cy, cy - r };
    // Extend each edge by half the stroke so the corners stay sharp.
    for (int pass = keyline > 0.0f ? 0 : 1; pass < 2; ++pass) {
        const float w = pass == 0 ? thick + keyline : thick;
        const Rgb col = pass == 0 ? KeyCol(c) : c;
        const float alpha = pass == 0 ? kKeyA * a : a;
        for (int i = 0; i < 4; ++i) {
            const float dx = px[i + 1] - px[i], dy = py[i + 1] - py[i];
            const float len = std::sqrt(dx * dx + dy * dy);
            const float ex = dx / len * w * 0.5f, ey = dy / len * w * 0.5f;
            Segment(px[i] - ex, py[i] - ey, px[i + 1] + ex, py[i + 1] + ey, w, col, alpha);
        }
    }
}

// Clockwise wedge from 12 o'clock covering frac of the node's interior:
// the elapsed part of the wave. A circle, or a diamond with its vertices
// at top/right/bottom/left (|x| + |y| = r).
inline void SectorFill(float cx, float cy, float r, bool diamond, float frac,
                       Rgb c, float a) {
    frac = std::clamp(frac, 0.0f, 1.0f);
    if (frac <= 0.0f) return;
    // 16 steps per quadrant land exactly on the diamond's corners.
    constexpr int kSteps = 64;
    const float step = 2.0f * kPi / kSteps;
    const float end = -0.5f * kPi + 2.0f * kPi * frac;
    auto edge = [&](float t, float& x, float& y) {
        const float ct = std::cos(t), st = std::sin(t);
        const float d = diamond ? r / (std::fabs(ct) + std::fabs(st)) : r;
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

inline bool IsEliteWave(int wave, Difficulty difficulty) {
    const ScheduledEliteInfo info = GetScheduledElite(wave, difficulty);
    return info.scheduled && IsEliteMob(info.kind);
}

inline void HudText(TextRenderer& font, const wchar_t* text, float x,
                     float baseline, float scale, Rgb c, float a) {
    font.Draw(text, x, baseline - font.BaselineOffset(scale), scale, c.r, c.g, c.b, a);
}

}  // namespace wave_hud

inline void DrawWaveHud(float sw, float hudScale, float hudTopY, float gameTime,
                        Difficulty difficulty, const GameManager& gm) {
    using namespace wave_hud;
    const float hs = hudScale;
    const int li = std::max(0, std::min(2, LangIndex()));
    const int finalWave = FinalWaveCount(difficulty);
    const int wave = CurrentWaveNumber(gameTime, difficulty);
    const float remain = CurrentWaveSecondsRemaining(gameTime, difficulty);
    const bool lastSeconds = remain > 0.0f && remain <= 10.0f;
    const float keyline = 3.2f * hs;
    const float cx = sw * 0.5f;

    // ── header: WAVE 04 / 15   02:10 ──
    static const wchar_t* kWaveLabel[3] = { L"WAVE", L"WAVE", L"WAVE" };
    wchar_t number[8], total[16], timer[16];
    swprintf_s(number, L"%02d", wave);
    swprintf_s(total, L"/ %d", finalWave);
    const int remainWhole = (int)std::ceil(remain);
    swprintf_s(timer, L"%02d:%02d", remainWhole / 60, remainWhole % 60);
    const float labelSc = UiTextScale(g_TextS, UiTextLevel::Supporting, hs * 0.80f);
    const float numberSc = UiTextScale(g_TextL, UiTextLevel::Subtitle, hs * 0.74f);
    const float totalSc = UiTextScale(g_TextS, UiTextLevel::Description, hs * 0.78f);
    const float timerSc = UiTextScale(g_TextL, UiTextLevel::Subtitle,
                                      hs * (lastSeconds ? 0.76f : 0.60f));
    const float labelW = g_TextS.Width(kWaveLabel[li], labelSc);
    const float numberW = g_TextL.Width(number, numberSc);
    const float totalW = g_TextS.Width(total, totalSc);
    const float timerW = g_TextL.Width(timer, timerSc);
    const float gapS = 6.0f * hs, gapL = 20.0f * hs;
    const float headerW = labelW + gapS + numberW + gapS + totalW + gapL + timerW;
    const float baseline = hudTopY + g_TextL.BaselineOffset(numberSc) + 2.0f * hs;
    float x = cx - headerW * 0.5f;
    HudText(g_TextS, kWaveLabel[li], x, baseline, labelSc, kNext, 0.92f);
    x += labelW + gapS;
    HudText(g_TextL, number, x, baseline, numberSc, kText, 1.0f);
    x += numberW + gapS;
    HudText(g_TextS, total, x, baseline, totalSc, kNext, 0.92f);
    x += totalW + gapL;
    HudText(g_TextL, timer, x, baseline, timerSc,
             lastSeconds ? kDanger : kPast, 1.0f);

    // ── track ──
    BindMainShader();
    const float trackY = baseline + 22.0f * hs;
    const float gap = finalWave > 1
        ? std::min(26.0f * hs, sw * 0.34f / (float)(finalWave - 1)) : 0.0f;
    const float trackW = gap * (float)(finalWave - 1);
    const float x0 = cx - trackW * 0.5f;
    const float lineW = 1.5f * hs;
    // Half-width of each node along the track, so the links stop at the
    // node's edge instead of running through it.
    auto nodeHalf = [&](int w) {
        const bool isFinal = w == finalWave;
        const bool isElite = !isFinal && IsEliteWave(w, difficulty);
        if (w == wave) return (isFinal || isElite ? 13.0f : 10.0f) * hs + 1.2f * hs;
        if (isFinal) return 9.0f * hs + 0.9f * hs;
        if (isElite) return 7.0f * hs + 0.9f * hs;
        return (w < wave ? 3.0f : 4.0f) * hs + 0.7f * hs;
    };
    const float linkPad = 3.0f * hs;
    for (int w = 1; w < finalWave; ++w) {
        const float a = x0 + gap * (float)(w - 1) + nodeHalf(w) + linkPad;
        const float b = x0 + gap * (float)w - nodeHalf(w + 1) - linkPad;
        if (b <= a) continue;
        const bool passed = w + 1 <= wave;
        Segment(a, trackY, b, trackY, lineW + keyline, KeyCol(passed ? kPast : kTrack), kKeyA);
        Segment(a, trackY, b, trackY, passed ? lineW : lineW * 0.8f,
                passed ? kPast : kTrack, passed ? 0.85f : 0.80f);
    }

    const float frac = std::clamp(remain / WAVE_DURATION_SECONDS, 0.0f, 1.0f);
    for (int w = 1; w <= finalWave; ++w) {
        const float nx = x0 + gap * (float)(w - 1);
        const bool isFinal = w == finalWave;
        const bool isElite = !isFinal && IsEliteWave(w, difficulty);
        if (w == wave) {
            // Current wave: a steady outline in the node's own colour, and
            // its interior filling clockwise from 12 o'clock as the wave
            // runs - a disc on plain waves, the diamond on elite/final ones.
            const bool special = isFinal || isElite;
            const Rgb own = isFinal ? kFinal : (isElite ? kElite : kPast);
            const float r = special ? 13.0f * hs : 10.0f * hs;
            const float stroke = 2.0f * hs;
            const float pulse = gm.waveTransitionTimer > 0.0f
                ? std::clamp((1.0f - gm.waveTransitionTimer / 2.0f) / 0.6f, 0.0f, 1.0f)
                : 1.0f;
            constexpr float kFillA = 0.85f;
            if (special) {
                DiamondOutline(nx, trackY, r, stroke, own, 1.0f, keyline);
                // The final wave keeps its double frame; the fill sits inside.
                float fillR = r - stroke * 1.4f - 1.5f * hs;
                if (isFinal) {
                    const float innerR = r - 5.0f * hs;
                    DiamondOutline(nx, trackY, innerR, 1.1f * hs, own, 0.9f, 0.0f);
                    fillR = innerR - 1.1f * hs * 1.4f - 1.5f * hs;
                }
                SectorFill(nx, trackY, fillR, true, 1.0f - frac, own, kFillA);
                if (pulse < 1.0f)
                    DiamondOutline(nx, trackY, r + 16.0f * hs * pulse, 1.4f * hs, own,
                                   0.9f * (1.0f - pulse), 0.0f);
            } else {
                Ring(nx, trackY, r, stroke, own, 1.0f, keyline);
                SectorFill(nx, trackY, r - stroke * 0.5f - 1.5f * hs, false,
                           1.0f - frac, own, kFillA);
                if (pulse < 1.0f)
                    Ring(nx, trackY, r + 18.0f * hs * pulse, 1.4f * hs, kPast,
                         0.9f * (1.0f - pulse), 0.0f);
            }
        } else if (isFinal) {
            DiamondOutline(nx, trackY, 9.0f * hs, 1.7f * hs, kFinal, 1.0f, keyline);
            DiamondOutline(nx, trackY, 4.5f * hs, 1.1f * hs, kFinal, 0.9f, 0.0f);
        } else if (isElite) {
            DiamondOutline(nx, trackY, 7.0f * hs, 1.7f * hs,
                           w < wave ? kPast : kElite, w < wave ? 0.85f : 1.0f, keyline);
        } else if (w < wave) {
            Dot(nx, trackY, 3.0f * hs, kPast, 0.95f, keyline);
        } else {
            Ring(nx, trackY, 4.0f * hs, 1.3f * hs, kNext, 0.85f, keyline);
        }
    }
    BatchFlush();
}
