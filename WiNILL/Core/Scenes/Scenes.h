#pragma once
#include "SceneContext.h"
#include "ScenePause.h"
#include "SceneRunEnd.h"
#include "SceneStart.h"

void Scene_MainMenu(const SceneCtx& c);
void Scene_Shop(const SceneCtx& c);
void Scene_Codex(const SceneCtx& c);
void Scene_CreativeConfig(const SceneCtx& c);
void Scene_Settings(const SceneCtx& c);
void Scene_Paused(const SceneCtx& c, const PauseSceneContext& state);
void Scene_AugSelect(const SceneCtx& c);
void Scene_AugReplace(const SceneCtx& c);
void Scene_OwnedAugPanel(const SceneCtx& c);
// Smoke capture: open a main-menu panel directly
// (-1 none, 0 play, 1 shop, 2 codex, 3 settings).
void SmokeOpenMenuPanel(int panel);

// ESC — 뒤로가기 버튼이 있는 메뉴 화면에서만 (전투 중 제외). 처리했으면 true.
bool TryEscNavigateBack();
