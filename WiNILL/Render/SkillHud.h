#pragma once
// ─────────────────────────────────────────────────────────────
// Skill HUD (bottom left): four keys (SHIFT/Q/E/R) as square frames with a
// line icon, and a row of owned passives (RAIN/LSTP/DEATH) as circles above.
//   square = pressed by the player, circle = runs by itself
//   cooldown / charge = clockwise wedge filling the frame from 12 o'clock
// Icons are line pictograms with constellation dots on their ends and
// joints. Text is only the key letter and the seconds left on a cooldown.
// State comes from SkillHudModel.h (BuildSkillHudView).
// ─────────────────────────────────────────────────────────────
#include <cmath>
#include <cstdio>

#include "DrawPrim.h"
#include "HudLines.h"
#include "SceneUI.h"
#include "Settings.h"
#include "SkillHudModel.h"
#include "TextRenderer.h"
#include "UiLayout.h"

extern TextRenderer g_TextL;
extern TextRenderer g_TextS;

inline skill_hud::SkillHudPulse g_SkillHudPulse;

namespace skill_hud {

using hud_lines::Rgb;

inline Rgb ColorRgb(HudColor c) {
    switch (c) {
    case HudColor::Dash:      return { 0.40f, 1.00f, 1.00f };
    case HudColor::Time:      return { 0.40f, 0.90f, 1.00f };
    case HudColor::Close:     return { 0.50f, 0.80f, 1.00f };
    case HudColor::Focus:     return { 0.59f, 0.94f, 0.86f };
    case HudColor::Rain:      return { 1.00f, 0.50f, 0.20f };
    case HudColor::LightStep: return { 0.70f, 0.70f, 0.85f };
    case HudColor::Death:     return { 1.00f, 0.36f, 0.38f };
    default:                  return { 0.36f, 0.41f, 0.53f };
    }
}
inline constexpr Rgb kKeyText = { 0.86f, 0.91f, 1.00f };

// One stroke style, drawn in two passes so every keyline sits under every
// bright stroke of the glyph (no keyline crossing a joint).
struct Pen {
    Rgb c;
    float a, w, node, keyline;
    bool keyPass;

    Rgb Col() const { return keyPass ? hud_lines::KeyCol(c) : c; }
    float Alpha() const { return keyPass ? hud_lines::kKeyA * a : a; }
    float Width() const { return keyPass ? w + keyline : w; }

    void Line(float x0, float y0, float x1, float y1) const {
        // Extend by half the stroke so joints close without gaps.
        const float dx = x1 - x0, dy = y1 - y0;
        const float len = std::sqrt(dx * dx + dy * dy);
        if (len < 0.001f) return;
        const float ex = dx / len * Width() * 0.5f, ey = dy / len * Width() * 0.5f;
        hud_lines::Segment(x0 - ex, y0 - ey, x1 + ex, y1 + ey, Width(), Col(), Alpha());
    }
    void Arc(float cx, float cy, float r, float a0, float sweep) const {
        hud_lines::Arc(cx, cy, r, Width(), a0, sweep, Col(), Alpha());
    }
    void Circle(float cx, float cy, float r, float alphaMul = 1.0f, float wMul = 1.0f) const {
        const float wv = keyPass ? w * wMul + keyline : w * wMul;
        hud_lines::Arc(cx, cy, r, wv, 0.0f, 2.0f * hud_lines::kPi, Col(), Alpha() * alphaMul);
    }
    void Node(float x, float y, float scale = 1.0f) const {
        const float r = node * scale + (keyPass ? keyline * 0.5f : 0.0f);
        const Rgb col = Col();
        drawCircle(x, y, r, col.r, col.g, col.b, Alpha());
    }
};

inline void GlyphDash(const Pen& p, float x, float y, float r, bool flash) {
    for (float dx : { -0.45f, 0.15f }) {
        const float tipX = x + dx * r + 0.25f * r;
        p.Line(x + dx * r - 0.30f * r, y - 0.55f * r, tipX, y);
        p.Line(tipX, y, x + dx * r - 0.30f * r, y + 0.55f * r);
        p.Node(tipX, y);
    }
    p.Line(x - 0.95f * r, y, x - 0.62f * r, y);
    if (flash) {
        p.Line(x + 0.40f * r, y, x + 0.80f * r, y);
        p.Node(x + 0.92f * r, y, 1.5f);
    }
}

inline void GlyphBurst(const Pen& p, float x, float y, float r) {
    p.Circle(x, y, 0.42f * r);
    p.Node(x, y);
    for (int k = 0; k < 4; ++k) {
        const float t = hud_lines::kPi * (0.25f + 0.5f * k);
        const float c = std::cos(t), s = std::sin(t);
        p.Line(x + 0.62f * r * c, y + 0.62f * r * s, x + 0.92f * r * c, y + 0.92f * r * s);
        p.Node(x + 0.98f * r * c, y + 0.98f * r * s);
    }
}

inline void GlyphFocus(const Pen& p, float x, float y, float r) {
    p.Circle(x, y, 0.62f * r);
    p.Circle(x, y, 0.20f * r, 1.0f, 0.8f);
    for (int k = 0; k < 4; ++k) {
        const float t = hud_lines::kPi * 0.5f * k;
        const float c = std::cos(t), s = std::sin(t);
        p.Line(x + 0.78f * r * c, y + 0.78f * r * s, x + 1.0f * r * c, y + 1.0f * r * s);
        p.Node(x + 1.0f * r * c, y + 1.0f * r * s);
    }
}

inline void GlyphTimeStop(const Pen& p, float x, float y, float r) {
    const float h = 0.85f * r, s = 0.55f * r;
    p.Line(x - s, y - h, x + s, y - h);
    p.Line(x - s, y + h, x + s, y + h);
    p.Line(x - s * 0.8f, y - h, x + s * 0.8f, y + h);
    p.Line(x + s * 0.8f, y - h, x - s * 0.8f, y + h);
    p.Line(x - s * 0.35f, y + h * 0.62f, x + s * 0.35f, y + h * 0.62f);
    p.Node(x - s, y - h); p.Node(x + s, y - h);
    p.Node(x - s, y + h); p.Node(x + s, y + h);
}

inline void GlyphRain(const Pen& p, float x, float y, float r) {
    for (float dx : { -0.45f, 0.05f, 0.55f }) {
        const float x0 = x + dx * r - 0.25f * r, y0 = y - 0.60f * r;
        const float x1 = x + dx * r + 0.15f * r, y1 = y + 0.45f * r;
        p.Line(x0, y0, x1, y1);
        const float ang = std::atan2(y1 - y0, x1 - x0);
        for (float d : { 2.6f, -2.6f })
            p.Line(x1, y1, x1 + 0.24f * r * std::cos(ang + d), y1 + 0.24f * r * std::sin(ang + d));
        p.Node(x0, y0);
    }
}

inline void GlyphLightStep(const Pen& p, float x, float y, float r) {
    const float cx = x + 0.35f * r, cy = y + 0.35f * r;
    for (int k = 0; k < 3; ++k) {
        const float rr = r * (0.55f + 0.22f * k);
        p.Arc(cx, cy, rr, hud_lines::kPi, hud_lines::kPi * 0.5f);
        p.Node(cx, cy - rr);
    }
}

inline void GlyphDeath(const Pen& p, float x, float y, float r, int stacks) {
    p.Circle(x, y, 0.32f * r);
    p.Circle(x, y, 0.72f * r, 0.45f, 0.6f);
    for (int k = 0; k < stacks; ++k) {
        const float t = -0.5f * hud_lines::kPi + k * 2.0f * hud_lines::kPi / 3.0f;
        p.Node(x + 0.72f * r * std::cos(t), y + 0.72f * r * std::sin(t), 1.4f);
    }
}

inline void DrawSkillGlyph(Pen p, SkillGlyph g, float x, float y, float r) {
    for (int pass = p.keyline > 0.0f ? 0 : 1; pass < 2; ++pass) {
        p.keyPass = pass == 0;
        switch (g) {
        case SkillGlyph::Dash:     GlyphDash(p, x, y, r, false); break;
        case SkillGlyph::Flash:    GlyphDash(p, x, y, r, true); break;
        case SkillGlyph::Burst:    GlyphBurst(p, x, y, r); break;
        case SkillGlyph::Focus:    GlyphFocus(p, x, y, r); break;
        case SkillGlyph::TimeStop: GlyphTimeStop(p, x, y, r); break;
        }
    }
}

inline void DrawPassiveGlyph(Pen p, const PassiveView& v, float x, float y, float r) {
    for (int pass = p.keyline > 0.0f ? 0 : 1; pass < 2; ++pass) {
        p.keyPass = pass == 0;
        switch (v.glyph) {
        case PassiveGlyph::Rain:      GlyphRain(p, x, y, r); break;
        case PassiveGlyph::LightStep: GlyphLightStep(p, x, y, r); break;
        case PassiveGlyph::Death:     GlyphDeath(p, x, y, r, v.stacks); break;
        }
    }
}

inline float PulseT(float pulse) {
    return pulse > 0.0f ? 1.0f - pulse / kPulseDur : 1.0f;
}

}  // namespace skill_hud

inline void DrawSkillHud(const skill_hud::SkillHudView& view, float hudScale, float sh) {
    using namespace skill_hud;
    using hud_lines::FillShape;
    const float hs = hudScale;
    const float keyline = 3.2f * hs * CombatKeylineStrength();
    const float side = 60.0f * hs, gap = 10.0f * hs, half = side * 0.5f;
    const float x0 = BottomLeftActionX(hs);
    const float top = HudY(sh, side + Hud::SKILL_KEYS_Y * hs);
    const float cy = top + half;
    const float keySc = UiTextScale(g_TextS, UiTextLevel::Supporting, hs);
    const float secSc = UiTextScale(g_TextL, UiTextLevel::Description, hs);
    const float keyH = g_TextS.Height(L"SHIFT", keySc);

    // ── shapes ──
    BindMainShader();
    for (int i = 0; i < 4; ++i) {
        const SkillSlotView& s = view.slots[i];
        const float cx = x0 + half + i * (side + gap);
        const Rgb col = ColorRgb(s.color);
        if (s.empty) {
            hud_lines::SquareOutline(cx, cy, half, 1.0f * hs, col, 0.35f, keyline);
            const Pen plus{ col, 0.55f, 1.2f * hs, 0.0f, 0.0f, false };
            plus.Line(cx - 7.0f * hs, cy, cx + 7.0f * hs, cy);
            plus.Line(cx, cy - 7.0f * hs, cx, cy + 7.0f * hs);
            continue;
        }
        const bool ready = s.elapsedFrac >= 1.0f;
        const float frameW = (ready ? 2.0f : 1.4f) * hs;
        hud_lines::SquareOutline(cx, cy, half, frameW, col, ready ? 1.0f : 0.55f, keyline);
        if (!ready)
            hud_lines::SectorFill(cx, cy, half - frameW * 0.5f - 1.5f * hs, FillShape::Square,
                                  s.elapsedFrac, col, 0.30f);
        if (s.effectFrac > 0.0f)
            hud_lines::SquareOutline(cx, cy, half + 5.0f * hs, 1.1f * hs, col, s.effectFrac, keyline);
        if (s.pulse > 0.0f) {
            const float t = PulseT(s.pulse);
            hud_lines::SquareOutline(cx, cy, half + 14.0f * hs * t, 1.4f * hs, col,
                                     0.9f * (1.0f - t), 0.0f);
        }
        const Pen pen{ col, s.iconBright ? 1.0f : 0.30f, 1.8f * hs, 1.8f * hs, keyline, false };
        DrawSkillGlyph(pen, s.glyph, cx, cy, half * 0.55f);
    }

    const float pr = 17.0f * hs;
    const float pcy = top - keyH - 12.0f * hs - pr;
    for (int i = 0; i < view.passiveCount; ++i) {
        const PassiveView& p = view.passives[i];
        const float pcx = x0 + pr + i * (2.0f * pr + 12.0f * hs);
        const Rgb col = ColorRgb(p.color);
        const bool off = p.glyph == PassiveGlyph::LightStep && !p.on;
        hud_lines::Ring(pcx, pcy, pr, 1.4f * hs, col, off ? 0.55f : 1.0f, keyline);
        const bool charging = (p.glyph == PassiveGlyph::Rain && p.chargeFrac < 1.0f) || off;
        if (charging)
            hud_lines::SectorFill(pcx, pcy, pr - 0.7f * hs - 1.5f * hs, FillShape::Circle,
                                  p.chargeFrac, col, 0.30f);
        if (p.pulse > 0.0f) {
            const float t = PulseT(p.pulse);
            hud_lines::Ring(pcx, pcy, pr + 14.0f * hs * t, 1.4f * hs, col, 0.9f * (1.0f - t), 0.0f);
        }
        float alpha = 1.0f, w = 1.8f * hs;
        if (off) { alpha = 0.35f; w = 1.0f * hs; }
        else if (p.glyph == PassiveGlyph::Rain && p.chargeFrac < 1.0f) alpha = 0.55f;
        const Pen pen{ col, alpha, w, 1.6f * hs, keyline, false };
        DrawPassiveGlyph(pen, p, pcx, pcy, pr * 0.62f);
    }
    BatchFlush();

    // ── text: key letters outside each frame, seconds inside ──
    for (int i = 0; i < 4; ++i) {
        const SkillSlotView& s = view.slots[i];
        const float left = x0 + i * (side + gap);
        const bool ready = !s.empty && s.elapsedFrac >= 1.0f;
        g_TextS.Draw(s.key, left + 1.0f * hs, top - 5.0f * hs - keyH, keySc,
                     kKeyText.r, kKeyText.g, kKeyText.b, ready ? 0.95f : 0.55f);
        if (s.showSeconds) {
            wchar_t buf[8];
            swprintf_s(buf, L"%d", (int)std::ceil(s.cdRemain));
            const float w = g_TextL.Width(buf, secSc);
            const float h = g_TextL.Height(buf, secSc);
            g_TextL.Draw(buf, left + half - w * 0.5f, cy - h * 0.5f, secSc,
                         kKeyText.r, kKeyText.g, kKeyText.b, 0.95f);
        }
    }
}
