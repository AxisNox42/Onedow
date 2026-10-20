#pragma once
// ─────────────────────────────────────────────────────────────
// Vitals HUD: HP stays next to the player, XP moves to the screen edge.
//   HP arc  — world space, around the player. 240° open at the top so the
//             aim line and weapon shell keep the upper side. Always one solid
//             line (no gaps: no dashed lines in the UI); the 10% steps are 9
//             short ticks just outside the arc. One colour; red only below
//             25%, with "LOW HP" beside the arc.
//             The HP just lost stays red for a moment, then drains.
//   XP line — screen space, bottom edge. Level label at its left end.
// Model (fills, ghost, thresholds): VitalsHudModel.h.
// ─────────────────────────────────────────────────────────────
#include <algorithm>
#include <cmath>
#include <cwchar>

#include "Camera.h"
#include "DrawPrim.h"
#include "HudLines.h"
#include "SceneUI.h"
#include "Settings.h"
#include "TextRenderer.h"
#include "UiLayout.h"
#include "VitalsHudModel.h"

extern TextRenderer g_TextS;

inline vitals_hud::HpGhost g_HpGhost;

namespace vitals_hud {

using hud_lines::Rgb;

inline constexpr Rgb kHpCol     = { 0.22f, 0.65f, 1.00f };
inline constexpr Rgb kDangerCol = { 1.00f, 0.36f, 0.38f };
inline constexpr Rgb kTrackCol  = { 0.55f, 0.62f, 0.72f };
inline constexpr Rgb kXpCol     = { 0.30f, 0.82f, 1.00f };
inline constexpr Rgb kXpCapCol  = { 1.00f, 0.82f, 0.30f };

inline float HpArcRadius(float playerSize) { return std::max(66.0f, playerSize * 2.72f); }
inline float HpArcThick(float playerSize) { return std::max(3.6f, playerSize * 0.144f); }

// World space; call inside the world pass with the player centre.
// alpha: resting strength (the caller raises it on a hit).
inline void DrawHpArc(float cx, float cy, float playerSize, float hpFrac,
                      float ghostFrac, float alpha, float time) {
    using hud_lines::Arc;
    using hud_lines::KeyCol;
    using hud_lines::kKeyA;
    const float radius = HpArcRadius(playerSize);
    const float thick = HpArcThick(playerSize);
    const float keyline = 3.0f * CombatKeylineStrength();
    const bool danger = IsDanger(hpFrac);
    const Rgb col = danger ? kDangerCol : kHpCol;
    const float a = danger
        ? std::min(1.0f, 0.80f + 0.20f * (0.5f + 0.5f * std::sin(time * 6.0f)))
        : alpha;
    const float trackA = 0.40f * std::max(alpha, 0.6f);
    hpFrac = std::clamp(hpFrac, 0.0f, 1.0f);
    ghostFrac = std::clamp(std::max(ghostFrac, hpFrac), 0.0f, 1.0f);

    // Track, ghost and fill sit end to end on one continuous arc.
    struct Piece { float a0, sweep; Rgb c; float alpha; };
    Piece pieces[3];
    int n = 0;
    if (ghostFrac < 1.0f)
        pieces[n++] = { kArcStart + kArcSweep * ghostFrac, kArcSweep * (1.0f - ghostFrac),
                        kTrackCol, trackA };
    if (ghostFrac > hpFrac)
        pieces[n++] = { kArcStart + kArcSweep * hpFrac, kArcSweep * (ghostFrac - hpFrac),
                        kDangerCol, 0.75f * a };
    if (hpFrac > 0.0f)
        pieces[n++] = { kArcStart, kArcSweep * hpFrac, col, a };

    // 10% ticks outside the arc; the 50% one is a little longer.
    const float tickA = 0.80f * std::max(alpha, 0.6f);
    const float tickW = std::max(1.2f, thick * 0.45f);
    const float t0 = radius + thick * 0.5f + 2.0f;
    auto ticks = [&](float w, Rgb c, float ta) {
        for (int i = 1; i < kHpSegments; ++i) {
            const float ang = kArcStart + kArcSweep * i / kHpSegments;
            const float t1 = t0 + (i == kHpSegments / 2 ? 7.0f : 4.5f);
            const float ca = std::cos(ang), sa = std::sin(ang);
            hud_lines::Segment(cx + ca * t0, cy + sa * t0, cx + ca * t1, cy + sa * t1, w, c, ta);
        }
    };

    if (keyline > 0.0f) {
        for (int i = 0; i < n; ++i)
            Arc(cx, cy, radius, thick + keyline, pieces[i].a0, pieces[i].sweep,
                KeyCol(pieces[i].c), kKeyA * pieces[i].alpha);
        ticks(tickW + keyline, KeyCol(kTrackCol), kKeyA * tickA);
    }
    for (int i = 0; i < n; ++i)
        Arc(cx, cy, radius, thick, pieces[i].a0, pieces[i].sweep, pieces[i].c, pieces[i].alpha);
    ticks(tickW, kTrackCol, tickA);
}

// Screen space text for the arc: "LOW HP" at the arc's right end while in
// danger, and "hp / max" under the arc while a hit is fresh (numberA > 0).
// visible = false on reward screens: time is frozen there, so a fresh-hit
// number would otherwise sit on top of the cards until the screen closes.
inline void DrawHpArcText(float cx, float cy, float playerSize, float hpFrac,
                          float hp, float maxHp, float numberA, float hudScale,
                          float time, bool visible) {
    if (!visible) return;
    const float radius = HpArcRadius(playerSize) + HpArcThick(playerSize);
    const float sc = UiTextScale(g_TextS, UiTextLevel::Supporting, hudScale);
    if (IsDanger(hpFrac)) {
        static const wchar_t* kLow[3] = { L"위험", L"LOW HP", L"危険" };
        const wchar_t* s = kLow[std::clamp(LangIndex(), 0, 2)];
        const float ang = kArcStart + kArcSweep;           // right end (-30°)
        const float x = W2SX(cx + std::cos(ang) * radius) + 8.0f * hudScale;
        const float y = W2SY(cy + std::sin(ang) * radius) - g_TextS.Height(s, sc) * 0.5f;
        const float pulse = 0.5f + 0.5f * std::sin(time * 6.0f);
        g_TextS.Draw(s, x, y, sc, kDangerCol.r, kDangerCol.g, kDangerCol.b, 0.70f + 0.30f * pulse);
    }
    if (numberA > 0.0f && maxHp > 0.0f) {
        wchar_t buf[32];
        std::swprintf(buf, 32, L"%d / %d", (int)std::ceil(std::max(0.0f, hp)), (int)std::ceil(maxHp));
        const float w = g_TextS.Width(buf, sc);
        const Rgb c = IsDanger(hpFrac) ? kDangerCol : kHpCol;
        g_TextS.Draw(buf, W2SX(cx) - w * 0.5f, W2SY(cy + radius) + 4.0f * hudScale, sc,
                     c.r, c.g, c.b, numberA);
    }
}

// Keep the version compact while sharing the bottom margin and bar gap.
inline float BottomHudTextScale(float hs) {
    return UiTextScale(g_TextS, UiTextLevel::Supporting, 1.65f * hs);
}
inline float BottomHudVersionScale(float hs) {
    return UiTextScale(g_TextS, UiTextLevel::Supporting, 1.20f * hs);
}
inline float BottomHudPadding(float hs) { return 20.0f * hs; }
inline float BottomHudGap(float hs) { return 16.0f * hs; }
inline float BottomHudTextY(float sh, float hs) {
    return HudY(sh, BottomHudPadding(hs)) -
        g_TextS.Height(L"Ag0123456789", BottomHudTextScale(hs));
}

// Screen space. Line along the bottom edge from just after the level label
// to rightX. levelUp: LevelUpTransition progress (0 = idle) flashes the line.
inline void DrawXpEdgeLine(float sh, float hudScale, float xpFrac, bool atCap,
                           const wchar_t* levelLabel, float levelUp, float rightX) {
    using hud_lines::KeyCol;
    using hud_lines::kKeyA;
    using hud_lines::Segment;
    const float hs = hudScale;
    const float keyline = 3.2f * hs * CombatKeylineStrength();
    const Rgb col = atCap ? kXpCapCol : kXpCol;
    const float sc = BottomHudTextScale(hs);
    const float labelW = g_TextS.Width(levelLabel, sc);
    const float labelH = g_TextS.Height(levelLabel, sc);
    const float textY = BottomHudTextY(sh, hs);
    const float y = textY + labelH * 0.5f;
    const float x0 = BottomHudPadding(hs);
    const float lx = x0 + labelW + BottomHudGap(hs);
    if (rightX <= lx + 40.0f * hs) return;
    const float fx = lx + (rightX - lx) * std::clamp(xpFrac, 0.0f, 1.0f);
    const float flash = levelUp > 0.0f ? 1.0f - levelUp : 0.0f;

    struct Piece { float x0, x1, w; Rgb c; float a; };
    Piece pieces[3];
    int n = 0;
    if (fx < rightX) pieces[n++] = { fx, rightX, 3.0f * hs, kTrackCol, 0.65f };
    if (fx > lx)     pieces[n++] = { lx, fx, 4.0f * hs, col, 1.0f };
    if (flash > 0.0f) pieces[n++] = { lx, rightX, (4.0f + 1.6f * flash) * hs, col, 0.90f * flash };

    BindMainShader();
    if (keyline > 0.0f)
        for (int i = 0; i < n; ++i)
            Segment(pieces[i].x0, y, pieces[i].x1, y, pieces[i].w + keyline,
                    KeyCol(pieces[i].c), kKeyA * pieces[i].a);
    for (int i = 0; i < n; ++i)
        Segment(pieces[i].x0, y, pieces[i].x1, y, pieces[i].w, pieces[i].c, pieces[i].a);
    BatchFlush();
    g_TextS.Draw(levelLabel, x0, textY, sc, col.r, col.g, col.b,
                 0.85f + 0.15f * flash);
}

}  // namespace vitals_hud
