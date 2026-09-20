#pragma once
#include "GameManager.h"

// Astral flow (difficulty, career, weapon) — shared panel dimensions
constexpr float FLOW_PANEL_W = 1440.0f;
constexpr float FLOW_PANEL_H = 800.0f;
constexpr float FLOW_CHROME_TOP = 0.0f;
constexpr float FLOW_FOOT_H     = 76.0f;   // 하단(뒤로) 예약 높이

inline float FlowContentH(float panelH) {
    return panelH - FLOW_CHROME_TOP - FLOW_FOOT_H;
}
inline float FlowBackY(float panelY, float panelH) {
    return panelY + panelH - FLOW_FOOT_H + 14.0f;
}

// ── 브라우저 크롬 ──────────────────────────────────────────────────
constexpr float BROWSER_CHROME_H = 0.0f;    // 타이틀바 제거

inline const wchar_t* BrowserUrl(GameState st) {
    using GS = GameState;
    switch (st) {
    case GS::MAIN_MENU:         return L"STAR MAP";
    case GS::CREATIVE_CONFIG:   return L"GENESIS CONFIG";
    case GS::SHOP:              return L"ARMORY";
    case GS::CODEX:             return L"ASTRAL ARCHIVE";
    case GS::SETTINGS:          return L"OBSERVATORY";
    case GS::GAMEOVER:          return L"SIGNAL COLLAPSE";
    case GS::VICTORY:           return L"STARFALL REPORT";
    case GS::AUG_SELECT:
    case GS::DEBUFF_SELECT:     return L"AUGMENT SELECTION";
    case GS::RUN_SHOP:          return L"RUN INTERMISSION";
    default:                    return L"ORBITAL CORE";
    }
}

// 씬 전환 결과를 outSt 에 반환 (탭 클릭 시 변경됨)
void DrawBrowserChrome(float sw, float sh, GameState st,
                       double mx, double my, bool lmb, bool lmbPrev,
                       GameState& outSt,
                       float barRatio = 0.0f,
                       float br = 0.35f, float bg = 0.72f, float bb = 1.0f);

void SceneDeskWindow(float sw, float sh, const wchar_t* fname,
                     float ar, float ag, float ab);
// Astral flow panel — compact constellation surface
void SceneFlowWindow(float sw, float sh, float WW, float WH,
                     const wchar_t* fname, float ar, float ag, float ab,
                     float& outX, float& outY, float& outContentY,
                     float dimAlpha = 0.0f,
                     float bodyAlpha = 0.97f,
                     float shadowAlpha = 0.32f);
void SceneAppWindow(float sw, float sh, float WW, float WH,
                    const wchar_t* fname, float ar, float ag, float ab,
                    float& outX, float& outY, bool gameOverlay = false,
                    float dimAlpha = 0.0f);
void DrawIngameTaskbar(float sw, float sh, GameState st);
