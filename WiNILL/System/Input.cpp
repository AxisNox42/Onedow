#include "Input.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "GameManager.h"
#include "Codex.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string>
#include <vector>
#include <cwctype>
#include <cstring>

extern GameManager g_GameManager;
extern float g_DevToastTimer;
extern bool g_VolEdit;
extern wchar_t g_VolBuf[8];
extern int g_VolLen;

bool  keys[1024] = {};
float g_ScrollAccum = 0.0f;

wchar_t g_CodexSearch[64] = {0};
int     g_CodexSearchLen  = 0;

static const wchar_t* DEV_CODE = L"develop_mod";

static std::string CodexWideToUtf8(const std::wstring& text) {
    if (text.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, text.data(), (int)text.size(),
                                         nullptr, 0, nullptr, nullptr);
    if (size <= 0) return {};
    std::string result((size_t)size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), (int)text.size(),
                        result.data(), size, nullptr, nullptr);
    return result;
}

static std::wstring CodexUtf8ToWide(const char* text) {
    if (!text || !text[0]) return {};
    const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1,
                                         nullptr, 0);
    if (size <= 1) return {};
    std::vector<wchar_t> converted((size_t)size);
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1,
                             converted.data(), size)) return {};
    return std::wstring(converted.data(), (size_t)size - 1);
}

static void CodexCheckDevPhrase() {
    if (g_DevUnlocked) return;
    std::wstring value(g_CodexSearch);
    for (auto& ch : value) if (ch < 128) ch = (wchar_t)towlower(ch);
    if (value == DEV_CODE) {
        g_DevUnlocked = true;
        g_DevToastTimer = 3.0f;
        CodexSearchClear();
    }
}

static void CodexCopySelection(GLFWwindow* window, bool cut) {
    if (!CodexSearchHasSelection()) return;
    const std::string utf8 = CodexWideToUtf8(CodexSearchSelectedText());
    glfwSetClipboardString(window, utf8.c_str());
    if (cut) CodexSearchDeleteSelection();
}

static void CodexPaste(GLFWwindow* window) {
    const std::wstring text = CodexUtf8ToWide(glfwGetClipboardString(window));
    CodexSearchInsertText(text.c_str());
    CodexCheckDevPhrase();
}

static void InputKeyCallback(GLFWwindow* window, int key, int, int action, int mods) {
    if (key >= 0 && key < 1024) {
        if      (action == GLFW_PRESS)   keys[key] = true;
        else if (action == GLFW_RELEASE) keys[key] = false;
    }
    if (!g_CodexSearchInputEnabled || (action != GLFW_PRESS && action != GLFW_REPEAT)) return;

    const bool ctrl = (mods & GLFW_MOD_CONTROL) != 0;
    const bool shift = (mods & GLFW_MOD_SHIFT) != 0;
    if (ctrl && action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_A: CodexSearchSelectAll(); return;
        case GLFW_KEY_C: CodexCopySelection(window, false); return;
        case GLFW_KEY_X: CodexCopySelection(window, true); return;
        case GLFW_KEY_V: CodexPaste(window); return;
        default: break;
        }
    }
    switch (key) {
    case GLFW_KEY_LEFT:      CodexSearchMoveLeft(shift); break;
    case GLFW_KEY_RIGHT:     CodexSearchMoveRight(shift); break;
    case GLFW_KEY_HOME:      CodexSearchMoveHome(shift); break;
    case GLFW_KEY_END:       CodexSearchMoveEnd(shift); break;
    case GLFW_KEY_BACKSPACE: CodexSearchBackspace(); break;
    case GLFW_KEY_DELETE:    CodexSearchDeleteForward(); break;
    default: break;
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
    if (!g_CodexSearchInputEnabled) return;
    CodexSearchInsertCodepoint(cp);
    CodexCheckDevPhrase();
}

static void InputScrollCallback(GLFWwindow*, double /*xoff*/, double yoff) {
    g_ScrollAccum += (float)yoff;
}

void InputRegisterCallbacks(GLFWwindow* window) {
    glfwSetKeyCallback(window, InputKeyCallback);
    glfwSetCharCallback(window, InputCharCallback);
    glfwSetScrollCallback(window, InputScrollCallback);
}

void InputClearState() {
    std::memset(keys, 0, sizeof(keys));
    g_ScrollAccum = 0.0f;
}

InputFocusTransition InputUpdateWindowFocus(GLFWwindow* window) {
    static bool wasFocused = true;
    const bool focused = glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0;
    const InputFocusTransition transition{ !focused && wasFocused,
                                           focused && !wasFocused };
    if (transition.lost || transition.gained) InputClearState();
    wasFocused = focused;
    return transition;
}

void InputUpdateGameplayCursor(GLFWwindow* window, GameState state,
                               bool crosshairEnabled, bool debugVisible) {
    const bool hideCursor = crosshairEnabled && !debugVisible &&
        (state == GameState::RUNNING || state == GameState::DYING);
    static bool cursorModeInitialized = false;
    static bool cursorHidden = false;
    if (!cursorModeInitialized || cursorHidden != hideCursor) {
        glfwSetInputMode(window, GLFW_CURSOR,
                         hideCursor ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);
        cursorHidden = hideCursor;
        cursorModeInitialized = true;
    }
}
