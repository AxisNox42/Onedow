#pragma once
#include "GameManager.h"

void SceneDeskWindow(float sw, float sh, const wchar_t* fname,
                     float ar, float ag, float ab);
// onedow.exe 흐름(난이도·직업·무기 등) — 화면 전체가 아닌 컴팩트 패널
void SceneFlowWindow(float sw, float sh, float WW, float WH,
                     const wchar_t* fname, float ar, float ag, float ab,
                     float& outX, float& outContentY);
void SceneAppWindow(float sw, float sh, float WW, float WH,
                    const wchar_t* fname, float ar, float ag, float ab,
                    float& outX, float& outY, bool gameOverlay = false);
void DrawIngameTaskbar(float sw, float sh, GameState st);
