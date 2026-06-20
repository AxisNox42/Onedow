#pragma once
#include "Settings.h"

// ── 하단 HUD / 작업표시줄 오프셋 ──
inline float BottomInset() { return g_GameBarH + (float)g_TaskbarH; }
inline float HudY(float sh, float offsetAboveBars) {
    return sh - offsetAboveBars - BottomInset();
}

namespace Hud {
inline constexpr float CREATIVE_LABEL = 36.0f;
inline constexpr float COMBO_TEXT     = 106.0f;
inline constexpr float LOW_HP_WARN    = 94.0f;
inline constexpr float SKILL_KEYS_Y   = 40.0f + 56.0f + 8.0f;   // HP 바 위 스킬 키
inline constexpr float SLOT_BAR_BASE  = 40.0f;
}

// ── 레이아웃 / 이징 ──
inline float Smoothstep(float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    return t * t * (3.0f - 2.0f * t);
}

inline float CenterX(float parentW, float childW) { return (parentW - childW) * 0.5f; }

// 중심 좌표 + 창 크기 → 좌상단 (가짜 창 scissor/draw 공용)
inline float WinOrigin(float center, float size) { return center - size * 0.5f; }
