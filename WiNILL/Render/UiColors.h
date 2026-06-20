#pragma once
#include "DrawPrim.h"

struct UiColor4 { float r, g, b, a; };
struct UiColor3 { float r, g, b; };

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

inline void drawRectCol(float x, float y, float w, float h, UiColor4 c) {
    drawRect(x, y, w, h, c.r, c.g, c.b, c.a);
}

inline void drawRectCol3(float x, float y, float w, float h, UiColor3 c, float a = 1.0f) {
    drawRect(x, y, w, h, c.r, c.g, c.b, a);
}
