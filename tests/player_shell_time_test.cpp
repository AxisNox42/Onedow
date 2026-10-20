#include "PlayerWeaponShell.h"
#include "DrawPrim.h"
#include "MonsterManager.h"
#include "Weapons.h"
#include <GLFW/glfw3.h>
#include <cassert>
#include <cstdio>

MonsterManager g_MonsterManager;
int g_CurrentWeapon = (int)StartWeapon::SMG;
float g_MuzzleTimer = 0.0f;

static std::vector<float> Geometry(float gameTime) {
    g_Batch.clear();
    DrawPlayerWeaponShell(300.0f, 300.0f, 25.0f, 0.0f, gameTime);
    return g_Batch;
}

int main() {
    assert(glfwInit());
    const auto idle = Geometry(3.0f);
    g_MonsterManager.monsters.push_back(new Monster(380.0f, 300.0f));
    glfwSetTime(10.0);
    const auto firing = Geometry(3.0f);
    assert(firing.size() > idle.size()); // Includes a nearby target's zap.
    glfwSetTime(100.0);
    assert(Geometry(3.0f) == firing);    // Paused: wall time cannot animate it.
    assert(Geometry(3.1f) != firing);    // Resume: gameplay time animates it.
    g_MonsterManager.Clear();
    glfwTerminate();
    std::puts("player_shell_time_test: OK");
}
