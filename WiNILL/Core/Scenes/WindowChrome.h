#pragma once
#include "GameManager.h"

// onedow.exe 흐름(난이도·직업·무기) — 세 페이지 공통 패널 크기
constexpr float FLOW_PANEL_W = 1440.0f;
constexpr float FLOW_PANEL_H = 800.0f;
constexpr float FLOW_CHROME_TOP = 40.0f;   // 타이틀바+강조선 아래 여백
constexpr float FLOW_FOOT_H     = 76.0f;   // 하단(뒤로) 예약 높이

inline float FlowContentH(float panelH) {
    return panelH - FLOW_CHROME_TOP - FLOW_FOOT_H;
}
inline float FlowBackY(float panelY, float panelH) {
    return panelY + panelH - FLOW_FOOT_H + 14.0f;
}

void SceneDeskWindow(float sw, float sh, const wchar_t* fname,
                     float ar, float ag, float ab);
// onedow.exe 흐름(난이도·직업·무기 등) — 화면 전체가 아닌 컴팩트 패널
void SceneFlowWindow(float sw, float sh, float WW, float WH,
                     const wchar_t* fname, float ar, float ag, float ab,
                     float& outX, float& outY, float& outContentY);
void SceneAppWindow(float sw, float sh, float WW, float WH,
                    const wchar_t* fname, float ar, float ag, float ab,
                    float& outX, float& outY, bool gameOverlay = false);
void DrawIngameTaskbar(float sw, float sh, GameState st);
