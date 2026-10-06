#pragma once

#include "SceneTextContext.h"

enum class GameState;

// Dependencies the pause scene needs from mutable app state. Passing individual
// fields keeps the scene independent of the global-state declaration hub.
struct PauseSceneContext {
    GameState& currentState;
    GameState& resumeState;
    GameState& settingsReturnState;
    bool& showOwnedAugments;
    bool previousLeftMouseDown;
    SceneTextContext text;
    void (*resetSettingsUi)(float);
};
