#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "GameManager.h"
#include "PlayerStats.h"
#include <cstdio>
#include <cstdlib>

// Unrelated GameManager entry points are linked but unused by this GL test.
PlayerStats g_Stats;
int g_CurrentWeapon = 0;
GameState g_SettingsReturnTo = GameState::MAIN_MENU;
int g_HoveredAug = -1, g_AugExitSlot = -1, g_RepExitSlot = -1;
float g_AugExitT = 0.0f, g_RepExitT = 0.0f;
bool TryEscNavigateBack() { return false; }

static int lateDeletes = 0;
static void APIENTRY CountVertexArrays(GLsizei, const GLuint*) { ++lateDeletes; }
static void APIENTRY CountBuffers(GLsizei, const GLuint*) { ++lateDeletes; }
static void APIENTRY CountProgram(GLuint) { ++lateDeletes; }

static void Check(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}

int main() {
    Check(glfwInit() == GLFW_TRUE, "GLFW init");
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(256, 256, "GameManager shutdown",
                                         nullptr, nullptr);
    Check(window != nullptr, "window");
    glfwMakeContextCurrent(window);
    Check(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) != 0, "GL loader");

    GameManager manager;
    manager.Init(256, 256);
    manager.ShutdownGlResources();
    manager.ShutdownGlResources();  // idempotent before context shutdown

    glfwDestroyWindow(window);
    glfwTerminate();
    glad_glDeleteVertexArrays = CountVertexArrays;
    glad_glDeleteBuffers = CountBuffers;
    glad_glDeleteProgram = CountProgram;
    manager.ShutdownGlResources();
    Check(lateDeletes == 0, "no GL delete after context shutdown");
    std::puts("GameManager GL shutdown passed");
}
