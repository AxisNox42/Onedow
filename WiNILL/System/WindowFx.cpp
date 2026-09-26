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

namespace {
enum AccentState {
    ACCENT_DISABLED = 0,
    ACCENT_ENABLE_BLURBEHIND = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,
};

enum WindowCompositionAttribute {
    WCA_ACCENT_POLICY = 19,
};

struct AccentPolicy {
    int accentState;
    int accentFlags;
    unsigned long gradientColor;
    int animationId;
};

struct WindowCompositionAttributeData {
    WindowCompositionAttribute attribute;
    void* data;
    size_t sizeOfData;
};

using SetWindowCompositionAttributeProc = BOOL (WINAPI *)(
    HWND, WindowCompositionAttributeData*);

static HWND s_blurHwnd = nullptr;
static bool s_blurEnabled = false;
static bool s_blurApplied = false;

static int ReadTransparencySetting() {
    DWORD value = 0, size = sizeof(value);
    const LSTATUS result = RegGetValueW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"EnableTransparency", RRF_RT_REG_DWORD, nullptr, &value, &size);
    return result == ERROR_SUCCESS ? (value != 0 ? 1 : 0) : -1;
}

static void NotifyTransparencyChanged() {
    DWORD_PTR ignored = 0;
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
        reinterpret_cast<LPARAM>(L"ImmersiveColorSet"),
        SMTO_ABORTIFHUNG, 200, &ignored);
}

struct TemporaryTransparency {
    bool restorePending = false;
    bool originalExists = false;
    DWORD originalValue = 0;

    bool Restore() {
        if (!restorePending) return true;
        HKEY key = nullptr;
        LSTATUS result = RegOpenKeyExW(HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
            0, KEY_SET_VALUE, &key);
        if (result == ERROR_FILE_NOT_FOUND && !originalExists) {
            restorePending = false;
            return true;
        }
        if (result != ERROR_SUCCESS) return false;
        if (originalExists)
            result = RegSetValueExW(key, L"EnableTransparency", 0, REG_DWORD,
                reinterpret_cast<const BYTE*>(&originalValue), sizeof(originalValue));
        else
            result = RegDeleteValueW(key, L"EnableTransparency");
        RegCloseKey(key);
        if (result != ERROR_SUCCESS &&
            !(result == ERROR_FILE_NOT_FOUND && !originalExists)) return false;
        restorePending = false;
        NotifyTransparencyChanged();
        return true;
    }

    // Fallback for normal process exit; forced termination cannot run cleanup.
    ~TemporaryTransparency() { Restore(); }
};

static TemporaryTransparency s_transparency;

static bool EnableSystemTransparency() {
    DWORD original = 0, size = sizeof(original);
    const LSTATUS readResult = RegGetValueW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"EnableTransparency", RRF_RT_REG_DWORD, nullptr, &original, &size);
    // Do not overwrite a setting we cannot read and later restore exactly.
    if (readResult != ERROR_SUCCESS && readResult != ERROR_FILE_NOT_FOUND)
        return false;
    if (readResult == ERROR_SUCCESS && original != 0) return true;
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS)
        return false;
    const DWORD enabled = 1;
    const LSTATUS result = RegSetValueExW(key, L"EnableTransparency", 0,
        REG_DWORD, reinterpret_cast<const BYTE*>(&enabled), sizeof(enabled));
    RegCloseKey(key);
    if (result != ERROR_SUCCESS) return false;
    // Preserve the first snapshot if an earlier restore is still pending.
    if (!s_transparency.restorePending) {
        s_transparency.originalExists = readResult == ERROR_SUCCESS;
        s_transparency.originalValue = original;
        s_transparency.restorePending = true;
    }
    NotifyTransparencyChanged();
    return ReadTransparencySetting() == 1;
}

static void OpenBlurSettings(HWND hwnd, const wchar_t* uri) {
    const HINSTANCE result = ShellExecuteW(hwnd, L"open", uri,
                                           nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(result) <= 32)
        MessageBoxW(hwnd,
            L"Windows 설정을 열지 못했습니다. 설정에서 투명 효과를 켜고 "
            L"배터리/에너지 절약 모드를 꺼주세요.",
            L"ONEDOW 블러 설정", MB_OK | MB_ICONINFORMATION);
}

// Check environmental changes even after the composition API reported success.
// A successful policy call does not mean Windows currently permits acrylic.
static bool RefreshBlurEnvironment(HWND hwnd, bool enabled) {
    static bool wasEnabled = false;
    static double nextCheck = 0.0;
    static int lastTransparency = -2, lastSaver = -2;
    const bool enabling = enabled && !wasEnabled;
    const bool disabling = !enabled && wasEnabled;
    wasEnabled = enabled;
    const double now = glfwGetTime();
    if (!enabled) {
        if (disabling || now >= nextCheck) {
            nextCheck = now + 1.0;
            if (!s_transparency.Restore() && disabling)
                MessageBoxW(hwnd,
                    L"Windows 투명 효과를 원래 설정으로 복원하지 못했습니다.\n"
                    L"Windows 설정의 투명 효과 항목을 확인해주세요.",
                    L"ONEDOW 블러 설정", MB_OK | MB_ICONWARNING);
        }
        return disabling;
    }
    if (!enabling && now < nextCheck) return false;
    nextCheck = now + 1.0;

    if (enabling && !EnableSystemTransparency()) {
        if (MessageBoxW(hwnd,
            L"Windows 투명 효과를 자동으로 켜지 못했습니다.\n"
            L"설정에서 투명 효과를 켜주세요.\n\n설정을 여시겠습니까?",
            L"ONEDOW 블러 설정", MB_YESNO | MB_ICONINFORMATION) == IDYES)
            OpenBlurSettings(hwnd, L"ms-settings:colors");
    }
    SYSTEM_POWER_STATUS power = {};
    const int saver = GetSystemPowerStatus(&power)
        ? (power.SystemStatusFlag == 1 ? 1 : 0) : -1;
    const int transparency = ReadTransparencySetting();
    const bool changed = enabling || transparency != lastTransparency ||
                         saver != lastSaver;
    lastTransparency = transparency;
    lastSaver = saver;
    // Prompt only on launch/explicit enable, never interrupt gameplay on a timer.
    if (enabling && saver == 1) {
        MessageBoxW(hwnd,
            L"배터리/에너지 절약 모드가 켜져 있어 배경 블러가 제한됩니다.\n"
            L"Windows 전원 설정에서 절약 모드를 꺼주세요.\n"
            L"변경 후 게임으로 돌아오면 블러가 다시 적용됩니다.",
            L"ONEDOW 블러 안내", MB_OK | MB_ICONINFORMATION);
    }
    return changed;
}

static SetWindowCompositionAttributeProc ResolveBackdropProc() {
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (!user32) return nullptr;
    return reinterpret_cast<SetWindowCompositionAttributeProc>(
        GetProcAddress(user32, "SetWindowCompositionAttribute"));
}


}

bool ConfigureWindowBackdropBlur(GLFWwindow* window, bool enabled) {
    if (!window) return false;
    HWND hwnd = glfwGetWin32Window(window);
    if (!hwnd) return false;

    static double nextRetry = 0.0;
    if (RefreshBlurEnvironment(hwnd, enabled)) {
        s_blurApplied = false;
        nextRetry = 0.0;
    }
    const double now = glfwGetTime();
    if (hwnd == s_blurHwnd && enabled == s_blurEnabled &&
        (!enabled || s_blurApplied || now < nextRetry)) {
        return enabled && s_blurApplied;
    }

    const SetWindowCompositionAttributeProc setComposition = ResolveBackdropProc();
    BOOL accentApplied = FALSE;
    if (setComposition) {
        AccentPolicy policy = {};
        if (enabled) {
            // Prefer acrylic: recent Windows builds can accept legacy blur
            // without visibly blurring the desktop. Both paths use DWM.
            policy.accentState = ACCENT_ENABLE_ACRYLICBLURBEHIND;
            policy.accentFlags = 0;
            // AABBGGRR. Keep the fixed veil subtle; ON/OFF controls the effect.
            policy.gradientColor = (0x18ul << 24)
                                 | (0x18ul << 16)
                                 | (0x10ul << 8)
                                 | 0x08ul;
        } else {
            policy.accentState = ACCENT_DISABLED;
            policy.accentFlags = 0;
            policy.gradientColor = 0;
        }

        WindowCompositionAttributeData data = {};
        data.attribute = WCA_ACCENT_POLICY;
        data.data = &policy;
        data.sizeOfData = sizeof(policy);
        accentApplied = setComposition(hwnd, &data);
        if (enabled && !accentApplied) {
            policy.accentState = ACCENT_ENABLE_BLURBEHIND;
            accentApplied = setComposition(hwnd, &data);
        }
    }

    // DwmEnableBlurBehindWindow is not a blur fallback on Windows 8+.
    // Do not disable it either: GLFW uses it for framebuffer transparency.
    if (!accentApplied && enabled) {
        static bool reported = false;
        if (!reported) std::fprintf(stderr, "[Backdrop] Windows rejected blur policies\n");
        reported = true;
    }
    nextRetry = now + 2.0;
    if (accentApplied) {
        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }

    s_blurHwnd = hwnd;
    s_blurEnabled = enabled;
    s_blurApplied = enabled && accentApplied == TRUE;
    return enabled && s_blurApplied;
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

bool ConfigureWindowBackdropBlur(GLFWwindow* /*window*/, bool /*enabled*/) {
    return false;
}
#endif
