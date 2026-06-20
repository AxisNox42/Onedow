#pragma once
#include "GameManager.h"

void SceneDeskWindow(float sw, float sh, const wchar_t* fname,
                     float ar, float ag, float ab);
void SceneAppWindow(float sw, float sh, float WW, float WH,
                    const wchar_t* fname, float ar, float ag, float ab,
                    float& outX, float& outY);
void DrawIngameTaskbar(float sw, float sh, GameState st);
