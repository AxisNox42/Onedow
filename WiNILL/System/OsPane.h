#pragma once

// ─────────────────────────────────────────────────────────────
// Real OS windows for the NORMA boss prototype.
//   A pane is a Win32 window kept just below the game window: the OS draws
//   its title bar and border (matching the user's light/dark theme), its
//   client reads like a small notepad or image viewer in the same theme
//   (random lines from the programmed sky, or a star chart), and combat drawn
//   by the game covers all of it, so the pane never hides play and every
//   click reaches the game. In pause,
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
    // A desktop window of another program claimed as a space. The game never
    // moves, restacks, retitles or closes it; closing the pane only lets go.
    bool adopted = false;
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
    // Mostly hidden behind another program's window (one the game cannot
    // stack under, e.g. an always-on-top one): it is no space meanwhile.
    bool covered = false;
};

void Init(GLFWwindow* overlay);
void Shutdown();

// Opens a pane whose outer frame sits at (x, y, w, h) in overlay pixels.
int  Open(float x, float y, float w, float h);
// Claims DesktopWindows()[index] as a pane (see Pane::adopted). Returns the
// pane id, or 0 when the window is gone.
int  Adopt(int desktopIndex);
void Close(int id);           // game-side close, never refused
// Game-driven move: puts the pane's visible frame's top-left at (x, y) and
// its in-game rectangle with it. Ignored for adopted windows and while the
// in-game rectangle is still chasing a player's drag.
void Place(int id, float x, float y);
// Game-driven move and resize: the visible frame to (x, y, w, h). Same
// conditions as Place.
void Resize(int id, float x, float y, float w, float h);
void CloseAll();              // every pane, e.g. when a run ends
void SetLocked(int id, bool locked);
// 0..1: how unsteady the pane's contents paint (jittering lines, corrupted
// characters, a tint toward violet). Repaints every frame while above 0.
void SetUnstable(int id, float level);

// Per frame: refresh rectangles, restore titles after a refusal, and rescan
// real desktop windows at a low rate.
void Tick(float dt);

// While held (the game is paused) a moved window's in-game rectangle stays
// where it was; releasing the hold sends it after the window.
void SetHold(bool hold);

// The player switched away from the game (Alt-Tab and the like): the game's
// own panes hide until it comes back, then show again (un-minimized) right
// under the game in their order. Real windows claimed as spaces are left be.
void SetAway(bool away);
// Smoke captures run behind other windows: their panes must not count as
// covered (Pane::covered stays false while this is off).
void SetCoverCheck(bool on);

// Arrangement (Ctrl held, in combat or paused): the game sinks as far back
// as it can while staying in sight (above any window filling the screen,
// else above the desktop) so real windows show on top and can be dragged.
// Turning it off raises the game again and takes focus back.
void SetArrangeMode(bool on);
bool ArrangeMode();
// While arranging: the frame of the window under the cursor (a pane or a
// real window), the one a drag would pick up. False over the game or the
// desktop.
bool ArrangeTarget(Rect& frame);
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
