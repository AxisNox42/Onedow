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

// 전투 VFX 역할색 — 아군/필드/함정/적 구분 (후반 눈 피로↓)
namespace RoleCol {
inline constexpr UiColor3 ALLY      = {0.28f, 0.95f, 0.58f};  // 백신·아군 창
inline constexpr UiColor3 FIELD    = {0.55f, 1.00f, 0.28f};  // 정전기장
inline constexpr UiColor3 EMP      = {1.00f, 0.88f, 0.18f};  // EMP
inline constexpr UiColor3 DROP     = {0.22f, 0.82f, 0.32f};  // 패치
inline constexpr UiColor3 TRAP     = {1.00f, 0.38f, 0.22f};  // trap.exe
inline constexpr UiColor3 ENEMY_UI = {0.85f, 0.22f, 0.95f}; // 적 popup 계열
}

inline void drawRectCol(float x, float y, float w, float h, UiColor4 c) {
    drawRect(x, y, w, h, c.r, c.g, c.b, c.a);
}

inline void drawRectCol3(float x, float y, float w, float h, UiColor3 c, float a = 1.0f) {
    drawRect(x, y, w, h, c.r, c.g, c.b, a);
}
