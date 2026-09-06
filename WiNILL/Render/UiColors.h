#pragma once
#include "DrawPrim.h"

struct UiColor4 { float r, g, b, a; };
struct UiColor3 { float r, g, b; };

enum class UiThemeMode : uint8_t { Dark = 0, Light = 1 };

// Semantic UI palette. Keep gameplay/VFX colors outside this palette so a theme
// switch cannot accidentally recolor combat feedback.
struct UiThemePalette {
    UiColor4 TextPrimary;
    UiColor4 TextMuted;
    UiColor4 Frame;
    UiColor4 Panel;
    UiColor4 Accent;
    UiColor4 NodeGlow;
    UiColor4 Dim;
};

inline constexpr UiThemePalette DARK_THEME = {
    {0.92f, 0.96f, 1.00f, 1.00f},
    {0.58f, 0.70f, 0.84f, 0.82f},
    {0.18f, 0.62f, 0.96f, 0.90f},
    {0.035f, 0.050f, 0.080f, 0.72f},
    {0.10f, 0.68f, 1.00f, 1.00f},
    {0.24f, 0.76f, 1.00f, 0.82f},
    {0.002f, 0.006f, 0.016f, 0.68f}
};

// High-contrast light preset: silver/gray background assumptions, never pure
// white. It is defined now and can be exposed through Settings in a later phase.
inline constexpr UiThemePalette LIGHT_THEME = {
    {0.08f, 0.11f, 0.15f, 1.00f},
    {0.28f, 0.34f, 0.40f, 0.86f},
    {0.05f, 0.36f, 0.52f, 0.92f},
    {0.72f, 0.75f, 0.79f, 0.78f},
    {0.02f, 0.36f, 0.56f, 1.00f},
    {0.00f, 0.25f, 0.42f, 0.78f},
    {0.18f, 0.21f, 0.25f, 0.42f}
};

inline UiThemeMode g_UiThemeMode = UiThemeMode::Dark;

inline const UiThemePalette& GetUiTheme() {
    return g_UiThemeMode == UiThemeMode::Light ? LIGHT_THEME : DARK_THEME;
}

inline void SetUiTheme(UiThemeMode mode) {
    g_UiThemeMode = mode;
}

namespace UiCol {
inline constexpr UiColor4 VFX_BG     = {0.08f, 0.08f, 0.10f, 1.0f};
inline constexpr UiColor4 WIN_BODY   = {0.07f, 0.08f, 0.11f, 0.99f};
inline constexpr UiColor4 PANEL_BG   = {0.06f, 0.08f, 0.10f, 1.0f};
inline constexpr UiColor4 TASKBAR_BG = {0.01f, 0.015f, 0.03f, 0.94f};
inline constexpr UiColor3 ACCENT_CYAN = {0.30f, 0.80f, 1.00f};
inline constexpr UiColor3 TASKBAR_TOP = {0.35f, 0.62f, 1.00f};
inline constexpr UiColor3 APPROACH_ORB_OUTER = {0.95f, 0.00f, 0.00f};
inline constexpr UiColor3 APPROACH_ORB_INNER = {0.95f, 0.05f, 0.05f};
}

// 전투 VFX 역할색 — 아군/필드/함정/적 구분 (후반 눈 피로↓)
namespace RoleCol {
inline constexpr UiColor3 ALLY      = {0.28f, 0.95f, 0.58f};  // 백신·아군 창
inline constexpr UiColor3 FIELD    = {0.55f, 1.00f, 0.28f};  // 정전기장
inline constexpr UiColor3 EMP      = {1.00f, 0.88f, 0.18f};  // EMP
inline constexpr UiColor3 DROP     = {0.22f, 0.82f, 0.32f};  // 패치
inline constexpr UiColor3 TRAP     = {1.00f, 0.38f, 0.22f};  // trap.exe
inline constexpr UiColor3 ENEMY_UI = {0.85f, 0.22f, 0.95f}; // 적 popup 계열
}

// Visual depth tiers. Rules:
//   - Assign every UI element to exactly one tier based on its settled state
//     (Idle, Hover-settled, Selected-settled, Disabled).
//   - Tween/Fade/Pulse may pass through inter-tier values during transition.
//   - After transition, elements must converge to their tier's representative value.
//   - Decorative Pulse must not settle in Interactive or above.
//   - Each scene has at most one primary Ambient system.
namespace UiDepth {
    constexpr float Ambient        = 0.055f;  // dust, nebula, inactive decoration
    constexpr float AmbientMax     = 0.08f;
    constexpr float Structural     = 0.14f;   // dividers, tree lines, inactive frames
    constexpr float StructuralMax  = 0.18f;
    constexpr float Interactive    = 0.58f;   // buttons, selectable rows, key values
    constexpr float Focus          = 0.94f;   // active selection, titles, commands
    constexpr float DisabledControl = 0.38f;  // button visible but not clickable
    constexpr float HiddenOption   = 0.14f;   // locked/unrevealed item (structural presence only)
}

inline float ClampUiAlpha(float alpha, float maxAlpha) {
    if (alpha < 0.0f) return 0.0f;
    if (alpha > maxAlpha) return maxAlpha;
    return alpha;
}

// Use for any decoration that pulses. Automatically caps at the given tier max
// so decoration cannot accidentally drift into a higher tier.
inline float PulsedAmbientAlpha(float base, float amplitude, float phase) {
    return ClampUiAlpha(base + amplitude * sinf(phase), UiDepth::AmbientMax);
}

inline float PulsedStructuralAlpha(float base, float amplitude, float phase) {
    return ClampUiAlpha(base + amplitude * sinf(phase), UiDepth::StructuralMax);
}

inline void drawRectCol(float x, float y, float w, float h, UiColor4 c) {
    drawRect(x, y, w, h, c.r, c.g, c.b, c.a);
}

inline void drawRectCol3(float x, float y, float w, float h, UiColor3 c, float a = 1.0f) {
    drawRect(x, y, w, h, c.r, c.g, c.b, a);
}
