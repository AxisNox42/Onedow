#pragma once

// ─────────────────────────────────────────────────────────────
// Real OS windows for the NORMA boss prototype.
//   A pane is a Win32 window kept just below the game window: the OS draws
//   its title bar and border (matching the user's light/dark theme), its
//   client area is see-through, and combat drawn by the game covers both, so
//   the title bar never hides play and every click reaches the game. In pause,
//   holding Ctrl lifts panes and other windows above the game so the player
//   can drag, minimize and close them; a locked pane refuses minimize/close
//   and says so in its title.
//   Real desktop windows (other programs) are listed by rectangle only.
// macOS/Linux: every call is a no-op and no panes exist.
// ─────────────────────────────────────────────────────────────
#include <vector>

struct GLFWwindow;

namespace os_pane {

// Rectangles are in overlay client pixels (the game's screen space).
struct Rect {
    float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
};

// One afterimage of a pane's in-game rectangle in flight, and how long ago.
// It fades out over kGhostSeconds.
constexpr float kGhostSeconds = 0.18f;
// The in-game rectangle chases a moved window at a speed that starts at
// kStartSpeed (px/s) and grows e-fold every kSpeedRamp seconds, with no set
// arrival time: near moves land soon, far ones end in a fast dash.
constexpr float kStartSpeed = 20.0f;
constexpr float kSpeedRamp = 0.2f;
struct Ghost {
    Rect frame;
    float age = 0.0f;
};

struct Pane {
    int  id = 0;
    bool open = false;        // false once closed by the player or the game
    bool minimized = false;
    bool locked = false;      // minimize/close requests are refused
    Rect frame;               // visible bounds without the drop shadow
    Rect client;              // see-through area below the title bar
    int  moves = 0;           // position/size changes seen so far
    int  refusals = 0;        // refused minimize/close requests
    int  zRank = 0;           // 0 = front-most of the panes
    // The window itself moves with the player's drag, but its in-game
    // rectangle (space, brackets) stays put while held, then flies after it.
    Rect shown;               // in-game rectangle used by combat
    bool flying = false;
    float flightTime = 0.0f;  // seconds since the chase started
    std::vector<Ghost> trail; // afterimages left while flying
    float refusalTimer = 0.0f;// > 0 while the title shows the refusal
};

void Init(GLFWwindow* overlay);
void Shutdown();

// Opens a pane whose outer frame sits at (x, y, w, h) in overlay pixels.
int  Open(float x, float y, float w, float h);
void Close(int id);           // game-side close, never refused
void CloseAll();              // every pane, e.g. when a run ends
void SetLocked(int id, bool locked);

// Per frame: refresh rectangles, restore titles after a refusal, and rescan
// real desktop windows at a low rate.
void Tick(float dt);

// While held (the game is paused) a moved window's in-game rectangle stays
// where it was; releasing the hold sends it after the window.
void SetHold(bool hold);

// Pause-time arrangement: the game sinks just above the desktop so real
// windows show on top and the player can drag them. Turning it off raises
// the game again without taking focus.
void SetArrangeMode(bool on);
bool ArrangeMode();
bool AnyOpen();

const std::vector<Pane>& Panes();
const std::vector<Rect>& DesktopWindows();
bool OverlayIsForeground();

// Test hooks for smoke captures: send the same requests the title-bar
// buttons and a drag would.
void DebugRequestClose(int id);
void DebugRequestMinimize(int id);
void DebugRestore(int id);
void DebugMoveBy(int id, float dx, float dy);
// True while the arrange key (Ctrl) is held, read globally so it still
// works after another program took focus.
bool ArrangeKeyHeld();
// Saves what the screen shows under the overlay (game, panes and anything
// composed above) as a 32-bit BMP. Smoke captures read the GL framebuffer, which
// never contains the OS-drawn panes.
bool DebugCaptureScreen(const wchar_t* path);
// Diagnostic: process owning the top-level window under an overlay point,
// and the overlay's extended style bits.
unsigned long DebugProcessAt(float x, float y, bool* isOverlay);
unsigned long DebugOverlayExStyle();
// Diagnostic: hand the foreground to the test window titled "PaneProbe",
// as if the player had clicked another program.
bool DebugActivateProbe();

} // namespace os_pane
