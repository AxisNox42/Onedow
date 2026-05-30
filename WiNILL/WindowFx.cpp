#include "WindowFx.h"
#include <cstdio>
#include <cstdarg>

// --- 디버그 로그 (transparency_debug.txt 에 기록) — 크로스플랫폼 ---
void TransparencyLog(const char* fmt, ...) {
    static bool first = true;
    FILE* f = nullptr;
#ifdef _WIN32
    fopen_s(&f, "transparency_debug.txt", first ? "w" : "a");
#else
    f = std::fopen("transparency_debug.txt", first ? "w" : "a");
#endif
    if (!f) return;
    first = false;
    va_list a; va_start(a, fmt); vfprintf(f, fmt, a); va_end(a);
    fclose(f);
}

#ifdef _WIN32
// ═══════════════════════ Windows: DWM 합성 ═════════════════════════════════════
#include <windows.h>
#include <dwmapi.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#pragma comment(lib, "dwmapi.lib")

void EnableWindowTransparency(GLFWwindow* window) {
    HWND hwnd = glfwGetWin32Window(window);
    TransparencyLog("=== EnableWindowTransparency ===\n");

    BOOL dwmOn = FALSE;
    DwmIsCompositionEnabled(&dwmOn);
    TransparencyLog("  DwmIsCompositionEnabled = %s\n", dwmOn ? "YES" : "NO");

    if (dwmOn) {
        // 클라이언트 영역 전체를 유리 프레임으로 → per-pixel 알파가 데스크탑에 합성됨
        MARGINS m = { -1, -1, -1, -1 };
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
// ═══════════════════════ macOS / Linux ═════════════════════════════════════════
//   투명도는 glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE) 가 처리.
//   (Cocoa: NSWindow opaque=NO / X11: 32bit visual). 추가 OS 호출 불필요.
void EnableWindowTransparency(GLFWwindow* /*window*/) {
    TransparencyLog("=== EnableWindowTransparency (non-Windows: GLFW hint) ===\n");
}
#endif
