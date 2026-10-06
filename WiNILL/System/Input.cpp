#include "Input.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include "GameManager.h"
#include "Codex.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <imm.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cwctype>
#include <cstring>
#include <cmath>

#ifdef _MSC_VER
#pragma comment(lib, "imm32.lib")
#endif

extern GameManager g_GameManager;
extern float g_DevToastTimer;
extern bool g_VolEdit;
extern wchar_t g_VolBuf[8];
extern int g_VolLen;

bool  keys[1024] = {};
float g_ScrollAccum = 0.0f;

wchar_t g_CodexSearch[64] = {0};
int     g_CodexSearchLen  = 0;

static HWND s_imeHwnd = nullptr;
static WNDPROC s_glfwWndProc = nullptr;
static POINT s_imeCompositionPoint = { 16, 16 };
static POINT s_imeCandidatePoint = { 16, 40 };
static bool s_imeComposing = false;

static void ApplyCodexImeAnchor(HWND hwnd) {
    if (!hwnd || !s_imeComposing) return;
    HIMC context = ImmGetContext(hwnd);
    if (!context) return;
    COMPOSITIONFORM composition = {};
    composition.dwStyle = CFS_POINT;
    composition.ptCurrentPos = s_imeCompositionPoint;
    ImmSetCompositionWindow(context, &composition);
    CANDIDATEFORM candidate = {};
    candidate.dwIndex = 0;
    candidate.dwStyle = CFS_CANDIDATEPOS;
    candidate.ptCurrentPos = s_imeCandidatePoint;
    ImmSetCandidateWindow(context, &candidate);
    ImmReleaseContext(hwnd, context);
}

static void ReadCodexImeComposition(HWND hwnd, LPARAM flags) {
    if (!g_CodexSearchInputEnabled || (flags & GCS_RESULTSTR)) {
        CodexSearchClearComposition();
        return;
    }
    if (!(flags & GCS_COMPSTR)) return;
    HIMC context = ImmGetContext(hwnd);
    if (!context) return;
    wchar_t composition[64] = {};
    const LONG bytes = ImmGetCompositionStringW(context, GCS_COMPSTR, nullptr, 0);
    const LONG cursorPos = (flags & GCS_CURSORPOS)
        ? ImmGetCompositionStringW(context, GCS_CURSORPOS, nullptr, 0) : -1;
    if (bytes > 0) {
        const LONG capacity = (LONG)(sizeof(composition) - sizeof(wchar_t));
        const LONG readBytes = ImmGetCompositionStringW(
            context, GCS_COMPSTR, composition, std::min(bytes, capacity));
        if (readBytes > 0)
            composition[std::min<LONG>(readBytes / (LONG)sizeof(wchar_t), 63)] = 0;
    }
    ImmReleaseContext(hwnd, context);
    CodexSearchSetComposition(composition, cursorPos >= 0 ? (int)cursorPos : -1);
}

static LRESULT CALLBACK InputWindowProc(HWND hwnd, UINT message,
                                        WPARAM wParam, LPARAM lParam) {
    if (message == WM_IME_COMPOSITION)
        ReadCodexImeComposition(hwnd, lParam);

    const LRESULT result = s_glfwWndProc
        ? CallWindowProcW(s_glfwWndProc, hwnd, message, wParam, lParam)
        : DefWindowProcW(hwnd, message, wParam, lParam);

    if (message == WM_IME_STARTCOMPOSITION) {
        s_imeComposing = true;
        ApplyCodexImeAnchor(hwnd);
    } else if (message == WM_IME_ENDCOMPOSITION) {
        s_imeComposing = false;
        CodexSearchClearComposition();
    }
    return result;
}

void InputSetCodexImeAnchor(float compositionX, float caretX,
                            float y, float textHeight) {
    const POINT compositionPoint = {
        (LONG)std::lround(compositionX), (LONG)std::lround(y)
    };
    const POINT candidatePoint = {
        (LONG)std::lround(caretX), (LONG)std::lround(y + textHeight)
    };
    if (compositionPoint.x == s_imeCompositionPoint.x &&
        compositionPoint.y == s_imeCompositionPoint.y &&
        candidatePoint.x == s_imeCandidatePoint.x &&
        candidatePoint.y == s_imeCandidatePoint.y) return;
    s_imeCompositionPoint = compositionPoint;
    s_imeCandidatePoint = candidatePoint;
    ApplyCodexImeAnchor(s_imeHwnd);
}

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
    if (g_CodexSearchCompositionLen > 0) return;

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
    CodexSearchClearComposition();
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
    InputUnregisterCallbacks(window);
    s_imeHwnd = glfwGetWin32Window(window);
    if (s_imeHwnd) {
        SetLastError(0);
        const LONG_PTR previous = SetWindowLongPtrW(
            s_imeHwnd, GWLP_WNDPROC, (LONG_PTR)InputWindowProc);
        if (previous || GetLastError() == 0)
            s_glfwWndProc = (WNDPROC)previous;
        else
            s_imeHwnd = nullptr;
    }
}

void InputUnregisterCallbacks(GLFWwindow*) {
    if (s_imeHwnd && s_glfwWndProc &&
        (WNDPROC)GetWindowLongPtrW(s_imeHwnd, GWLP_WNDPROC) == InputWindowProc)
        SetWindowLongPtrW(s_imeHwnd, GWLP_WNDPROC, (LONG_PTR)s_glfwWndProc);
    s_imeHwnd = nullptr;
    s_glfwWndProc = nullptr;
    s_imeComposing = false;
    CodexSearchClearComposition();
}

void InputClearState() {
    std::memset(keys, 0, sizeof(keys));
    g_ScrollAccum = 0.0f;
    CodexSearchClearComposition();
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
