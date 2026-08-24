#pragma once
#include "GameManager.h"

// onedow.exe 흐름(난이도·직업·무기) — 세 페이지 공통 패널 크기
constexpr float FLOW_PANEL_W = 1440.0f;
constexpr float FLOW_PANEL_H = 800.0f;
constexpr float FLOW_CHROME_TOP = 10.0f;   // 타이틀바 제거 후 상단 여백
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
    case GS::MAIN_MENU:         return L"wallpaper.gif";
    case GS::RUN_CONFIG:        return L"runconfig.odw";
    case GS::CREATIVE_CONFIG:   return L"debug.odw";
    case GS::SHOP:              return L"shop.odw";
    case GS::CODEX:             return L"codex.odw";
    case GS::SETTINGS:          return L"setting.odw";
    case GS::GAMEOVER:          return L"gameover.log";
    case GS::VICTORY:           return L"victory.log";
    case GS::AUG_SELECT:
    case GS::DEBUFF_SELECT:     return L"augselect.odw";
    case GS::RUN_SHOP:          return L"runshop.odw";
    default:                    return L"onedow.exe";
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
// onedow.exe 흐름(난이도·직업·무기 등) — 화면 전체가 아닌 컴팩트 패널
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
