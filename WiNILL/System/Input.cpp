#include "Input.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "GameManager.h"
#include "Codex.h"
#include "GameContext.h"
#include <string>
#include <cwctype>

bool  keys[1024] = {};
float g_ScrollAccum = 0.0f;

wchar_t g_CodexSearch[64] = {0};
int     g_CodexSearchLen  = 0;

static const wchar_t* DEV_CODE = L"develop_mod";

static void InputKeyCallback(GLFWwindow*, int key, int, int action, int) {
    if (key >= 0 && key < 1024) {
        if      (action == GLFW_PRESS)   keys[key] = true;
        else if (action == GLFW_RELEASE) keys[key] = false;
    }
}

static void InputCharCallback(GLFWwindow*, unsigned int cp) {
    if (g_VolEdit && g_GameManager.currentState == GameState::SETTINGS) {
        if (cp >= L'0' && cp <= L'9' && g_VolLen < 3) {
            g_VolBuf[g_VolLen++] = (wchar_t)cp;
            g_VolBuf[g_VolLen] = 0;
        }
        return;
    }
    if (g_GameManager.currentState != GameState::CODEX &&
        !g_CodexSearchInputEnabled) return;
    if (cp >= 32 && g_CodexSearchLen < 63) {
        g_CodexSearch[g_CodexSearchLen++] = (wchar_t)cp;
        g_CodexSearch[g_CodexSearchLen]   = 0;
        if (!g_DevUnlocked) {
            std::wstring s(g_CodexSearch);
            for (auto& ch : s) if (ch < 128) ch = (wchar_t)towlower(ch);
            if (s == DEV_CODE) {
                g_DevUnlocked   = true;
                g_DevToastTimer = 3.0f;
                g_CodexSearch[0] = 0;
                g_CodexSearchLen = 0;
            }
        }
    }
}

static void InputScrollCallback(GLFWwindow*, double /*xoff*/, double yoff) {
    g_ScrollAccum += (float)yoff;
}

void InputRegisterCallbacks(GLFWwindow* window) {
    glfwSetKeyCallback(window, InputKeyCallback);
    glfwSetCharCallback(window, InputCharCallback);
    glfwSetScrollCallback(window, InputScrollCallback);
}
