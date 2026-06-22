#pragma once
#include "SceneContext.h"

void Scene_MainMenu(const SceneCtx& c);
void Scene_Shop(const SceneCtx& c);
void Scene_Codex(const SceneCtx& c);
void Scene_JobSelect(const SceneCtx& c);
void Scene_WeaponSelect(const SceneCtx& c);
void Scene_DifficultySelect(const SceneCtx& c);
void Scene_CreativeConfig(const SceneCtx& c);
void Scene_Settings(const SceneCtx& c);
void Scene_Ready(const SceneCtx& c);
void Scene_Paused(const SceneCtx& c);
void Scene_GameOver(const SceneCtx& c);
void Scene_AugSelect(const SceneCtx& c);
void Scene_RunShop(const SceneCtx& c);
void Scene_OwnedAugPanel(const SceneCtx& c);

// ESC — 뒤로가기 버튼이 있는 메뉴 화면에서만 (전투 중 제외). 처리했으면 true.
bool TryEscNavigateBack();
