#pragma once
#include <functional>
#include "GameManager.h"

struct GLFWwindow;

struct SceneCtx {
    float  sw, sh;
    double mx, my;
    bool   lmb;
    float  delta;
    GLFWwindow* window;
    float* fireTimer;
    std::function<void()> reset;
    std::function<void()> restartRun;
    std::function<void()> abandonRun;
};

// 앱 부팅 / 창 열림 애니메이션
extern float          g_BootAnim;
extern GameState      g_BootTarget;
extern const wchar_t* g_BootName;
extern float          g_BootAr, g_BootAg, g_BootAb;
extern float          g_AppOpen;
constexpr float BOOT_DUR      = 1.15f;
constexpr float APP_OPEN_DUR  = 0.22f;

void LaunchApp(GameState target, const wchar_t* name,
               float ar, float ag, float ab);

// ── 씬 페이드 전환 ───────────────────────────────────────────────
inline float     g_FadeAlpha  = 0.0f;
inline int       g_FadeDir    = 0;          // 1=fadeout, -1=fadein, 0=idle
inline GameState g_FadeTarget = GameState::MAIN_MENU;
constexpr float  FADE_OUT_DUR = 0.35f;
constexpr float  FADE_IN_DUR  = 0.35f;

inline void StartSceneFade(GameState target) {
    if (g_FadeDir != 0) return;
    g_FadeTarget = target;
    g_FadeDir    = 1;
    g_FadeAlpha  = 0.0f;
}
