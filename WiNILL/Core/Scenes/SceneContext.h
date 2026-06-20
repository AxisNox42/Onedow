#pragma once
#include <functional>

struct GLFWwindow;

enum class GameState;

struct SceneCtx {
    float  sw, sh;
    double mx, my;
    bool   lmb;
    float  delta;
    GLFWwindow* window;
    float* fireTimer;
    std::function<void()> reset;
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
