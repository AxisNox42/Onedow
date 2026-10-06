#pragma once

#include <vector>
#include "SceneTextContext.h"

enum class GameState;
struct SceneCtx;

struct GameOverSceneContext {
    GameState& currentState;
    float& fade;
    bool previousLeftMouseDown;
    const wchar_t* deathReason;
    bool lastRunRecord;
    long long score;
    int playerLevel;
    long long killCount;
    const std::vector<int>& ownedAugments;
    SceneTextContext text;
};

void Scene_GameOver(const SceneCtx& scene, const GameOverSceneContext& state);
