#include "SceneContext.h"
#include "GameManager.h"

float          g_BootAnim   = 0.0f;
GameState      g_BootTarget = GameState::DIFFICULTY_SELECT;
const wchar_t* g_BootName   = L"onedow.exe";
float          g_BootAr     = 0.30f;
float          g_BootAg     = 0.80f;
float          g_BootAb     = 1.00f;
float          g_AppOpen    = 1.0f;

void LaunchApp(GameState target, const wchar_t* name,
               float ar, float ag, float ab) {
    g_BootAnim   = BOOT_DUR;
    g_BootTarget = target;
    g_BootName   = name;
    g_BootAr     = ar;
    g_BootAg     = ag;
    g_BootAb     = ab;
}
