#include "WindowFx.h"
#include <cstdio>
#include <cstdarg>

// --- 디버그 로그 — 비활성화(no-op). 과거 transparency_debug.txt 를 만들던 기능 제거.
//     (F26: 배포본에 디버그 파일이 생기지 않도록. 호출부는 그대로 두고 함수만 비움.)
void TransparencyLog(const char* /*fmt*/, ...) {
    // intentionally empty
}

#ifdef _WIN32
// ═══════════════════════ Windows: DWM 합성 ═════════════════════════════════════
#include <windows.h>
#include <dwmapi.h>
#include <shobjidl.h>
#include <shellapi.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include "Settings.h"

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")

namespace {
// Shell fullscreen classification is independent of the swapchain dimensions.
// This preserves the 1px DWM-composition workaround without forcing TOPMOST
// or changing/hiding Explorer's taskbar windows globally.
bool MarkShellFullscreen(HWND hwnd, bool fullscreen) {
    const HRESULT initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(initialized) && initialized != RPC_E_CHANGED_MODE) return false;
    ITaskbarList2* taskbar = nullptr;
    HRESULT result = CoCreateInstance(CLSID_TaskbarList, nullptr,
                                     CLSCTX_INPROC_SERVER,
                                     IID_PPV_ARGS(&taskbar));
    if (SUCCEEDED(result)) {
        result = taskbar->HrInit();
        if (SUCCEEDED(result))
            result = taskbar->MarkFullscreenWindow(hwnd, fullscreen ? TRUE : FALSE);
        taskbar->Release();
    }
    if (SUCCEEDED(initialized)) CoUninitialize();
    return SUCCEEDED(result);
}
}

void UpdateWindowTaskbarPolicy(GLFWwindow* window) {
    if (!window) return;
    HWND hwnd = glfwGetWin32Window(window);
    const bool active = GetForegroundWindow() == hwnd && !IsIconic(hwnd);
    // Reapply on focus changes and Explorer restart; retry failures without
    // issuing a COM request every rendered frame.
    static HWND lastWindow = nullptr, lastShell = nullptr;
    static bool lastActive = false;
    static bool applied = false;
    static double nextRetry = 0.0;
    HWND shell = FindWindowW(L"Shell_TrayWnd", nullptr);
    const double now = glfwGetTime();
    if (hwnd == lastWindow && shell == lastShell && active == lastActive &&
        (applied || now < nextRetry)) return;
    applied = MarkShellFullscreen(hwnd, active);
    lastWindow = hwnd;
    lastShell = shell;
    lastActive = active;
    nextRetry = now + 2.0;
}

void ReleaseWindowTaskbarPolicy(GLFWwindow* window) {
    if (window) MarkShellFullscreen(glfwGetWin32Window(window), false);
}

void EnableWindowTransparency(GLFWwindow* window) {
    HWND hwnd = glfwGetWin32Window(window);
    TransparencyLog("=== EnableWindowTransparency ===\n");

    BOOL dwmOn = FALSE;
    DwmIsCompositionEnabled(&dwmOn);
    TransparencyLog("  DwmIsCompositionEnabled = %s\n", dwmOn ? "YES" : "NO");
    // GLFW_TRANSPARENT_FRAMEBUFFER already handles per-pixel alpha.
    // Extending DWM glass over the whole client area darkens the desktop behind UI.

    if (dwmOn) {
        // 클라이언트 영역 전체를 유리 프레임으로 → per-pixel 알파가 데스크탑에 합성됨
        MARGINS m = { 0, 0, 0, 0 };
        HRESULT hrM = DwmExtendFrameIntoClientArea(hwnd, &m);
        TransparencyLog("  DwmExtendFrameIntoClientArea → 0x%08lX (%s)\n",
            (unsigned long)hrM, SUCCEEDED(hrM) ? "OK" : "FAILED");
    }

    LONG ex = GetWindowLong(hwnd, GWL_EXSTYLE);
    TransparencyLog("  GWL_EXSTYLE   = 0x%08lX\n", ex);
    TransparencyLog("  WS_EX_LAYERED = %s\n", (ex & WS_EX_LAYERED) ? "SET" : "NOT SET");
    TransparencyLog("================================\n\n");
}

#else
void UpdateWindowTaskbarPolicy(GLFWwindow*) {}
void ReleaseWindowTaskbarPolicy(GLFWwindow*) {}
// ═══════════════════════ macOS / Linux ═════════════════════════════════════════
//   투명도는 glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE) 가 처리.
//   (Cocoa: NSWindow opaque=NO / X11: 32bit visual). 추가 OS 호출 불필요.
void EnableWindowTransparency(GLFWwindow* /*window*/) {
    TransparencyLog("=== EnableWindowTransparency (non-Windows: GLFW hint) ===\n");
}
#endif
