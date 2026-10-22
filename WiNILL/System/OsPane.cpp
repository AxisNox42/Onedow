#include "OsPane.h"

#ifdef _WIN32
#include <windows.h>
#include <dwmapi.h>
#include <algorithm>
#include <cmath>
#include <string>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <cstdio>
#include <filesystem>
#include "Settings.h"

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "advapi32.lib")

namespace os_pane {
namespace {

constexpr wchar_t kClassName[] = L"AsteriaPane";
// The client is painted in this colour and keyed out, so the overlay shows
// through and clicks pass to whatever lies below.
constexpr COLORREF kSeeThrough = RGB(255, 0, 255);
constexpr float kRefusalSeconds = 1.6f;
constexpr float kDesktopScanSeconds = 0.5f;
// Windows smaller than this, or covering nearly the whole game, are not
// candidates for a combat space.
constexpr float kMinDesktopW = 200.0f, kMinDesktopH = 120.0f;
constexpr float kMaxDesktopCover = 0.90f;
constexpr DWORD kDarkModeAttribute = 20;   // DWMWA_USE_IMMERSIVE_DARK_MODE

GLFWwindow* s_overlay = nullptr;
HWND s_overlayHwnd = nullptr;
// Hidden owner for every pane: keeps panes off the taskbar without tying
// their z-order to the game the way owning them by the overlay would.
HWND s_ownerHwnd = nullptr;
std::vector<Pane> s_panes;
std::vector<HWND> s_handles;               // parallel to s_panes

std::vector<Rect> s_desktop;
float s_scanTimer = 0.0f;
bool s_arrange = false;
bool s_hold = false;
bool s_sunk = false;          // arrange mode lowered the game below other windows
bool s_passthrough = false;   // arrange mode fell back to mouse pass-through
bool s_classReady = false;
int s_nextId = 1;

int IndexOfHandle(HWND hwnd) {
    for (size_t i = 0; i < s_handles.size(); ++i)
        if (s_handles[i] == hwnd) return (int)i;
    return -1;
}

int IndexOfId(int id) {
    for (size_t i = 0; i < s_panes.size(); ++i)
        if (s_panes[i].id == id && s_panes[i].open) return (int)i;
    return -1;
}

const wchar_t* BaseTitle() { return L"NORMA"; }

const wchar_t* RefusalSuffix() {
    const int li = LangIndex();
    return li == 0 ? L" (응답 없음)" : (li == 2 ? L" (応答なし)" : L" (Not Responding)");
}

POINT OverlayOrigin() {
    POINT origin{ 0, 0 };
    if (s_overlayHwnd) ClientToScreen(s_overlayHwnd, &origin);
    return origin;
}

Rect ToOverlay(const RECT& r) {
    const POINT o = OverlayOrigin();
    return { (float)(r.left - o.x), (float)(r.top - o.y),
             (float)(r.right - r.left), (float)(r.bottom - r.top) };
}

// Visible bounds: GetWindowRect includes the invisible resize border.
RECT FrameBounds(HWND hwnd) {
    RECT r{};
    if (FAILED(DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &r, sizeof(r))))
        GetWindowRect(hwnd, &r);
    return r;
}

void RefreshRects(int i) {
    HWND hwnd = s_handles[i];
    if (!hwnd) return;
    Pane& p = s_panes[i];
    p.frame = ToOverlay(FrameBounds(hwnd));
    RECT cr{};
    GetClientRect(hwnd, &cr);
    POINT tl{ cr.left, cr.top }, br{ cr.right, cr.bottom };
    ClientToScreen(hwnd, &tl);
    ClientToScreen(hwnd, &br);
    p.client = ToOverlay(RECT{ tl.x, tl.y, br.x, br.y });
    p.minimized = IsIconic(hwnd) != FALSE;
}

void Refuse(int i) {
    Pane& p = s_panes[i];
    ++p.refusals;
    p.refusalTimer = kRefusalSeconds;
    const std::wstring title = std::wstring(BaseTitle()) + RefusalSuffix();
    SetWindowTextW(s_handles[i], title.c_str());
}

// Win32 title bars stay light unless asked; follow the user's app theme.
void ApplyTheme(HWND hwnd) {
    DWORD light = 1, size = sizeof(light);
    RegGetValueW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &light, &size);
    const BOOL dark = light == 0 ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, kDarkModeAttribute, &dark, sizeof(dark));
}

void PaintSeeThrough(HWND hwnd, HDC dc) {
    RECT r{};
    GetClientRect(hwnd, &r);
    HBRUSH brush = CreateSolidBrush(kSeeThrough);
    FillRect(dc, &r, brush);
    DeleteObject(brush);
}

LRESULT CALLBACK PaneProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    const int i = IndexOfHandle(hwnd);
    switch (msg) {
    case WM_MOUSEACTIVATE:
        // Dragging a pane must not take keyboard focus from the game.
        return MA_NOACTIVATE;
    case WM_NCHITTEST: {
        const LRESULT hit = DefWindowProcW(hwnd, msg, wp, lp);
        return hit == HTCLIENT ? HTTRANSPARENT : hit;
    }
    case WM_ERASEBKGND:
        PaintSeeThrough(hwnd, (HDC)wp);
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        PaintSeeThrough(hwnd, dc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_SYSCOMMAND: {
        const WPARAM cmd = wp & 0xFFF0;
        if (i >= 0 && s_panes[i].locked && (cmd == SC_MINIMIZE || cmd == SC_CLOSE)) {
            Refuse(i);
            return 0;
        }
        break;
    }
    case WM_CLOSE:
        if (i >= 0 && s_panes[i].locked) {
            Refuse(i);
            return 0;
        }
        DestroyWindow(hwnd);
        return 0;
    case WM_WINDOWPOSCHANGED:
        if (i >= 0) {
            // Restacking below the game changes z-order only; not a move.
            const WINDOWPOS* pos = reinterpret_cast<const WINDOWPOS*>(lp);
            if ((pos->flags & (SWP_NOMOVE | SWP_NOSIZE)) != (SWP_NOMOVE | SWP_NOSIZE))
                ++s_panes[i].moves;
            RefreshRects(i);
        }
        break;
    case WM_SIZE:
        if (i >= 0) RefreshRects(i);
        break;
    case WM_DESTROY:
        if (i >= 0) {
            s_panes[i].open = false;
            s_handles[i] = nullptr;
        }
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

bool EnsureClass() {
    if (s_classReady) return true;
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = PaneProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kClassName;
    s_classReady = RegisterClassExW(&wc) != 0;
    return s_classReady;
}

// Panes live just below the game so combat draws over their title bars and
// clicks reach the game. Only arrange mode lets them rise above it.
bool AboveGame(HWND pane) {
    for (HWND h = GetWindow(s_overlayHwnd, GW_HWNDPREV); h; h = GetWindow(h, GW_HWNDPREV))
        if (h == pane) return true;
    return false;
}

void StackBelowGame(HWND pane) {
    SetWindowPos(pane, s_overlayHwnd, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

bool SameRect(const Rect& a, const Rect& b) {
    return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
}

// Moves a pane's in-game rectangle after its window once nothing holds it.
void UpdateShown(Pane& p, float dt) {
    if (!p.flying) {
        if (s_hold || SameRect(p.shown, p.frame)) return;
        p.flying = true;
        p.flightTime = 0.0f;
    }
    // Afterimages only where the rectangle really travelled, so the slow
    // creep does not pile them on one spot.
    const bool travelled = p.trail.empty() ||
        std::fabs(p.trail.back().frame.x - p.shown.x) +
        std::fabs(p.trail.back().frame.y - p.shown.y) >= 6.0f;
    if (travelled) p.trail.push_back({ p.shown, 0.0f });
    p.flightTime += dt;
    // Step toward the window's current rectangle (position and size as one
    // vector), so a window moved again mid-flight is still chased.
    const float dx = p.frame.x - p.shown.x, dy = p.frame.y - p.shown.y;
    const float dw = p.frame.w - p.shown.w, dh = p.frame.h - p.shown.h;
    const float remaining = std::sqrt(dx * dx + dy * dy + dw * dw + dh * dh);
    const float step = kStartSpeed * std::exp(p.flightTime / kSpeedRamp) * dt;
    if (step >= remaining) {
        p.shown = p.frame;
        p.flying = false;
        return;
    }
    const float k = step / remaining;
    p.shown = { p.shown.x + dx * k, p.shown.y + dy * k,
                p.shown.w + dw * k, p.shown.h + dh * k };
}

bool IsShellWindow(HWND hwnd) {
    wchar_t cls[64] = {};
    GetClassNameW(hwnd, cls, 64);
    return lstrcmpW(cls, L"Progman") == 0 || lstrcmpW(cls, L"WorkerW") == 0 ||
           lstrcmpW(cls, L"Shell_TrayWnd") == 0 ||
           lstrcmpW(cls, L"Shell_SecondaryTrayWnd") == 0;
}

// Only rectangles are kept: titles and contents of other programs are never read.
BOOL CALLBACK CollectDesktopWindow(HWND hwnd, LPARAM) {
    if (!IsWindowVisible(hwnd) || IsIconic(hwnd)) return TRUE;
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == GetCurrentProcessId()) return TRUE;
    if (GetWindowLongW(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) return TRUE;
    if (IsShellWindow(hwnd)) return TRUE;
    BOOL cloaked = FALSE;
    DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
    if (cloaked) return TRUE;

    const Rect r = ToOverlay(FrameBounds(hwnd));
    if (r.w < kMinDesktopW || r.h < kMinDesktopH) return TRUE;
    RECT oc{};
    GetClientRect(s_overlayHwnd, &oc);
    const float ow = (float)(oc.right - oc.left), oh = (float)(oc.bottom - oc.top);
    const float ix = (std::max)(0.0f, (std::min)(r.x + r.w, ow) - (std::max)(r.x, 0.0f));
    const float iy = (std::max)(0.0f, (std::min)(r.y + r.h, oh) - (std::max)(r.y, 0.0f));
    if (ix * iy <= 0.0f) return TRUE;
    if (ix * iy >= ow * oh * kMaxDesktopCover) return TRUE;
    s_desktop.push_back(r);   // EnumWindows walks front to back
    return TRUE;
}

// The top-level window hosting the desktop icons (Progman, or a WorkerW
// when a wallpaper engine reparents it).
BOOL CALLBACK FindShellView(HWND hwnd, LPARAM out) {
    if (FindWindowExW(hwnd, nullptr, L"SHELLDLL_DefView", nullptr)) {
        *reinterpret_cast<HWND*>(out) = hwnd;
        return FALSE;
    }
    return TRUE;
}

HWND DesktopShellWindow() {
    HWND shell = nullptr;
    EnumWindows(FindShellView, reinterpret_cast<LPARAM>(&shell));
    return shell;
}

// Brings the game back on top and gives it keyboard focus after the player
// clicked another program during arrange mode. Plain HWND_TOP is ignored
// while that program owns the foreground, so pass through the topmost band
// (z-order only) and borrow the foreground thread's input to take focus.
void RaiseOverlay() {
    const UINT keep = SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE;
    SetWindowPos(s_overlayHwnd, HWND_TOPMOST, 0, 0, 0, 0, keep);
    SetWindowPos(s_overlayHwnd, HWND_NOTOPMOST, 0, 0, 0, 0, keep);
    HWND foreground = GetForegroundWindow();
    if (foreground == s_overlayHwnd) return;
    const DWORD self = GetCurrentThreadId();
    const DWORD other = foreground ? GetWindowThreadProcessId(foreground, nullptr) : 0;
    const bool attached = other && other != self && AttachThreadInput(self, other, TRUE);
    SetForegroundWindow(s_overlayHwnd);
    SetFocus(s_overlayHwnd);
    if (attached) AttachThreadInput(self, other, FALSE);
}

void ScanDesktop() {
    s_desktop.clear();
    if (s_overlayHwnd) EnumWindows(CollectDesktopWindow, 0);
}

} // namespace

void Init(GLFWwindow* overlay) {
    s_overlay = overlay;
    s_overlayHwnd = overlay ? glfwGetWin32Window(overlay) : nullptr;
}

void Shutdown() {
    SetArrangeMode(false);
    for (size_t i = 0; i < s_handles.size(); ++i)
        if (s_handles[i]) DestroyWindow(s_handles[i]);
    if (s_ownerHwnd) DestroyWindow(s_ownerHwnd);
    s_ownerHwnd = nullptr;
    s_panes.clear();
    s_handles.clear();
    s_desktop.clear();
}

int Open(float x, float y, float w, float h) {
    if (!s_overlayHwnd || !EnsureClass()) return 0;
    if (!s_ownerHwnd)
        s_ownerHwnd = CreateWindowExW(WS_EX_TOOLWINDOW, kClassName, L"", WS_POPUP,
                                      0, 0, 0, 0, nullptr, nullptr,
                                      GetModuleHandleW(nullptr), nullptr);
    const POINT o = OverlayOrigin();
    const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
                        WS_MINIMIZEBOX | WS_THICKFRAME;
    const DWORD exStyle = WS_EX_LAYERED | WS_EX_NOACTIVATE;
    HWND hwnd = CreateWindowExW(exStyle, kClassName, BaseTitle(), style,
                                o.x + (int)x, o.y + (int)y, (int)w, (int)h,
                                s_ownerHwnd, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!hwnd) return 0;
    SetLayeredWindowAttributes(hwnd, kSeeThrough, 0, LWA_COLORKEY);
    ApplyTheme(hwnd);
    ShowWindow(hwnd, SW_SHOWNOACTIVATE);

    // Place the visible frame, not the invisible resize border, at (x, y, w, h).
    RECT wr{};
    GetWindowRect(hwnd, &wr);
    const RECT fr = FrameBounds(hwnd);
    const int left = fr.left - wr.left, top = fr.top - wr.top;
    const int right = wr.right - fr.right, bottom = wr.bottom - fr.bottom;
    SetWindowPos(hwnd, nullptr, o.x + (int)x - left, o.y + (int)y - top,
                 (int)w + left + right, (int)h + top + bottom,
                 SWP_NOZORDER | SWP_NOACTIVATE);
    if (!s_arrange) StackBelowGame(hwnd);

    Pane pane;
    pane.id = s_nextId++;
    pane.open = true;
    s_panes.push_back(pane);
    s_handles.push_back(hwnd);
    RefreshRects((int)s_panes.size() - 1);
    s_panes.back().shown = s_panes.back().frame;
    return pane.id;
}

void Close(int id) {
    const int i = IndexOfId(id);
    if (i < 0) return;
    s_panes[i].locked = false;
    DestroyWindow(s_handles[i]);
}

void CloseAll() {
    for (const Pane& p : s_panes)
        if (p.open) Close(p.id);
}

void SetLocked(int id, bool locked) {
    const int i = IndexOfId(id);
    if (i >= 0) s_panes[i].locked = locked;
}

void SetHold(bool hold) { s_hold = hold; }

void Tick(float dt) {
    // Front-to-back order among panes, for overlapping spaces.
    int rank = 0;
    for (HWND h = GetTopWindow(nullptr); h; h = GetWindow(h, GW_HWNDNEXT)) {
        const int i = IndexOfHandle(h);
        if (i >= 0) s_panes[i].zRank = rank++;
    }
    for (size_t i = 0; i < s_panes.size(); ++i) {
        Pane& p = s_panes[i];
        for (Ghost& g : p.trail) g.age += dt;
        p.trail.erase(std::remove_if(p.trail.begin(), p.trail.end(),
                                     [](const Ghost& g) { return g.age >= kGhostSeconds; }),
                      p.trail.end());
        if (!p.open) continue;
        if (!s_arrange && AboveGame(s_handles[i])) StackBelowGame(s_handles[i]);
        RefreshRects((int)i);
        UpdateShown(p, dt);
        if (p.refusalTimer > 0.0f) {
            p.refusalTimer -= dt;
            if (p.refusalTimer <= 0.0f) {
                p.refusalTimer = 0.0f;
                SetWindowTextW(s_handles[i], BaseTitle());
            }
        }
    }
    s_scanTimer -= dt;
    if (s_scanTimer <= 0.0f) {
        s_scanTimer = kDesktopScanSeconds;
        ScanDesktop();
    }
}

void SetArrangeMode(bool on) {
    if (on == s_arrange || !s_overlayHwnd) return;
    s_arrange = on;
    const UINT keep = SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE;
    if (on) {
        // Sink the game just above the desktop so real windows sit on top,
        // fully visible and draggable. The pause screen veils whatever lies
        // behind the game, so mouse pass-through alone left them unseen.
        HWND shell = DesktopShellWindow();
        HWND above = shell ? GetWindow(shell, GW_HWNDPREV) : nullptr;
        if (above && above != s_overlayHwnd) {
            SetWindowPos(s_overlayHwnd, above, 0, 0, 0, 0, keep);
            s_sunk = true;
        } else {
            glfwSetWindowAttrib(s_overlay, GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);
            s_passthrough = true;
        }
    } else {
        if (s_sunk) RaiseOverlay();
        if (s_passthrough)
            glfwSetWindowAttrib(s_overlay, GLFW_MOUSE_PASSTHROUGH, GLFW_FALSE);
        s_sunk = s_passthrough = false;
    }
}

bool ArrangeMode() { return s_arrange; }

bool AnyOpen() {
    for (const Pane& p : s_panes)
        if (p.open) return true;
    return false;
}
const std::vector<Pane>& Panes() { return s_panes; }
const std::vector<Rect>& DesktopWindows() { return s_desktop; }

bool OverlayIsForeground() {
    return s_overlayHwnd && GetForegroundWindow() == s_overlayHwnd;
}

void DebugRequestClose(int id) {
    const int i = IndexOfId(id);
    if (i >= 0) SendMessageW(s_handles[i], WM_SYSCOMMAND, SC_CLOSE, 0);
}

void DebugRequestMinimize(int id) {
    const int i = IndexOfId(id);
    if (i >= 0) SendMessageW(s_handles[i], WM_SYSCOMMAND, SC_MINIMIZE, 0);
}

void DebugRestore(int id) {
    const int i = IndexOfId(id);
    if (i >= 0) ShowWindow(s_handles[i], SW_SHOWNOACTIVATE);
}

void DebugMoveBy(int id, float dx, float dy) {
    const int i = IndexOfId(id);
    if (i < 0) return;
    RECT wr{};
    GetWindowRect(s_handles[i], &wr);
    SetWindowPos(s_handles[i], nullptr, wr.left + (int)dx, wr.top + (int)dy, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

unsigned long DebugProcessAt(float x, float y, bool* isOverlay) {
    const POINT o = OverlayOrigin();
    POINT pt{ o.x + (LONG)x, o.y + (LONG)y };
    HWND root = GetAncestor(WindowFromPoint(pt), GA_ROOT);
    if (isOverlay) *isOverlay = root == s_overlayHwnd;
    DWORD pid = 0;
    if (root) GetWindowThreadProcessId(root, &pid);
    return pid;
}

unsigned long DebugOverlayExStyle() {
    return s_overlayHwnd ? (unsigned long)GetWindowLongW(s_overlayHwnd, GWL_EXSTYLE) : 0;
}

bool DebugActivateProbe() {
    HWND probe = FindWindowW(nullptr, L"PaneProbe");
    return probe && SetForegroundWindow(probe);
}

bool ArrangeKeyHeld() {
    return (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
}

bool DebugCaptureScreen(const wchar_t* path) {
    if (!s_overlayHwnd) return false;
    RECT oc{};
    GetClientRect(s_overlayHwnd, &oc);
    const int w = oc.right - oc.left, h = oc.bottom - oc.top;
    if (w <= 0 || h <= 0) return false;
    const POINT o = OverlayOrigin();

    HDC screen = GetDC(nullptr);
    HDC mem = CreateCompatibleDC(screen);
    BITMAPINFO bi{};
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = h;              // bottom-up, as BMP files store rows
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    HBITMAP dib = CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    bool ok = false;
    if (dib && bits) {
        HGDIOBJ old = SelectObject(mem, dib);
        ok = BitBlt(mem, 0, 0, w, h, screen, o.x, o.y, SRCCOPY | CAPTUREBLT) != FALSE;
        SelectObject(mem, old);
        if (ok) {
            std::error_code ec;
            std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);
            BITMAPFILEHEADER fh{};
            const DWORD imageBytes = (DWORD)w * (DWORD)h * 4u;
            fh.bfType = 0x4D42;   // "BM"
            fh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
            fh.bfSize = fh.bfOffBits + imageBytes;
            FILE* f = nullptr;
            ok = _wfopen_s(&f, path, L"wb") == 0 && f;
            if (ok) {
                std::fwrite(&fh, sizeof(fh), 1, f);
                std::fwrite(&bi.bmiHeader, sizeof(bi.bmiHeader), 1, f);
                std::fwrite(bits, 1, imageBytes, f);
                std::fclose(f);
            }
        }
    }
    if (dib) DeleteObject(dib);
    DeleteDC(mem);
    ReleaseDC(nullptr, screen);
    return ok;
}

} // namespace os_pane

#else

namespace os_pane {
namespace { std::vector<Pane> s_none; std::vector<Rect> s_noRects; }
void Init(GLFWwindow*) {}
void Shutdown() {}
int  Open(float, float, float, float) { return 0; }
void Close(int) {}
void CloseAll() {}
void SetLocked(int, bool) {}
void SetHold(bool) {}
void Tick(float) {}
void SetArrangeMode(bool) {}
bool ArrangeMode() { return false; }
bool AnyOpen() { return false; }
const std::vector<Pane>& Panes() { return s_none; }
const std::vector<Rect>& DesktopWindows() { return s_noRects; }
bool OverlayIsForeground() { return true; }
void DebugRequestClose(int) {}
void DebugRequestMinimize(int) {}
void DebugRestore(int) {}
void DebugMoveBy(int, float, float) {}
bool ArrangeKeyHeld() { return false; }
bool DebugCaptureScreen(const wchar_t*) { return false; }
unsigned long DebugProcessAt(float, float, bool* isOverlay) { if (isOverlay) *isOverlay = true; return 0; }
unsigned long DebugOverlayExStyle() { return 0; }
bool DebugActivateProbe() { return false; }
} // namespace os_pane

#endif
