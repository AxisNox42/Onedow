// Windows ?�더�?glad보다 먼�? ??APIENTRY 매크�?중복 ?�의 방�?
#ifdef _WIN32
  #include <windows.h>
  #include <dwmapi.h>   // DwmIsCompositionEnabled (진단??
  #include <timeapi.h>  // timeBeginPeriod / timeEndPeriod (FPS �??��???
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#ifdef _WIN32
  #define GLFW_EXPOSE_NATIVE_WIN32
  #include <GLFW/glfw3native.h>
#endif

#include "Platform.h"
#include "CrashHandler.h"

#include <glm/glm.hpp>
#include <iostream>
#include <vector>
#include <deque>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <cstring>
#include <cwctype>
#include <algorithm>
#include <functional>

#include "FakeWindow.h"
#include "GameManager.h"
#include "GameVersion.h"
#include "Monster.h"
#include "GravisField.h"
#include "Bullet.h"
#include "MonsterManager.h"
#include "EnemyProgression.h"
#include "EnemySpawner.h"
#include "ProjectileSystem.h"
#include "AugmentSlots.h"
#include "GameplayTime.h"
#include "Codex.h"
#include "CollisionSystem.h"
#include "KillRewards.h"      // main.cpp kill paths (common collection, laser)
#include "RunStats.h"         // stat builds: new run, rebuild, restart, Chaos
#include "RunResult.h"        // run result commit: abandon, game over
#include "Augment.h"
#include "PlayerStats.h"
#include "TextRenderer.h"
#include "PlanetShader.h"
#include "NebulaGlowShader.h"
#include "Settings.h"
#include "UiColors.h"
#include "ExpSystem.h"
#include "DrawPrim.h"
#include "WindowFx.h"
#include "Translations.h"
#include "Weapons.h"
#include "SaveSystem.h"
#include "IconSystem.h"
#include "Camera.h"
#include "MainShader.h"
#include "EntityDraw.h"
#include "WaveHud.h"
#include "SkillHud.h"
#include "VitalsHud.h"
#include "PlayerWeaponShell.h"
#include "GameFonts.h"
#include "PlayerRuntime.h"
#include "Scenes.h"
#include "SceneContext.h"
#include "SceneInternal.h"
#include "SceneSkills.h"
#include "Input.h"
#include "DebugToolkit.h"
#include "SmokeCapture.h"
#include "OsPane.h"
#include "UiLayout.h"
#include "SceneUI.h"
#include "WindowChrome.h"
#include "GameplayTelemetry.h"

#ifdef _MSC_VER
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib")   // RegOpenKeyExA / RegQueryValueExA / RegCloseKey
#pragma comment(lib, "winmm.lib")      // timeBeginPeriod / timeEndPeriod (FPS �??��???
// dwmapi.lib ?�?WindowFx.cpp ?�서 링크
#endif

#ifdef _MSC_VER
extern "C++" {
    __declspec(dllexport) DWORD NvOptimusEnablement = 0;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 0;
}
#endif

// ?�명???�퍼??WindowFx.h/cpp �??�동
// (EnableWindowTransparency ?�?TransparencyLog 가 ?�일 기능)
#define DBG TransparencyLog

GameManager    g_GameManager;
MonsterManager g_MonsterManager;
std::vector<Bullet> g_Bullets;

// ?�?�?개발???�리?�이?�브) 모드 ???�감 검?�창 ?�스?�에그로�??�금 ?�?�?
//    출시 빌드???�리?�이?�브 진입?�이 ?�겨???�고, ?�감 검?�에 ?�크�?코드�?//    ?�력?�면 ?�금?�어 ?�이???�면???��????�장?�다. (?�반 ?�레?�어??�?�?
bool    g_DevUnlocked   = false;
float   g_DevToastTimer = 0.0f;            // ?�금 ?�인 ?�스??(�?
// ?�정�?볼륨 ?�자 직접?�력 ?�태
bool    g_VolEdit = false;
wchar_t g_VolBuf[8] = {0};
int     g_VolLen = 0;
PlayerStats    g_Stats;
GameplayTelemetry g_GameplayTelemetry;
bool g_aug1Released = true, g_aug2Released = true, g_aug3Released = true;
float g_AugExitT    = -1.0f;
int   g_AugExitSlot = -1;
float g_RepExitT    = -1.0f;
int   g_RepExitSlot = -3;
TextRenderer   g_TextL;   // ??글??(증강 ?�름, ?�태 ?�?��?)
TextRenderer   g_TextS;   // ?��? 글??(?�명, ?�트)
TextRenderer   g_TextXL;  // 초�????�?��?(?�작�?로고) ?�용 ??고해?�도 ?�스??
#ifdef _WIN32
HANDLE         g_FontMemHandle   = nullptr;
#endif

struct ApproachOrb {
    float x = 0, y = 0;
};
std::vector<ApproachOrb> g_ApproachOrbs;

struct DroneState {
    float angle     = 0.0f;
    float fireTimer = 0.0f;
};
// Game speed while Ctrl-arranging windows mid-fight.
constexpr float kArrangeSlow = 0.18f;

// Defined further down; the pane and boss helpers below read them.
extern float g_GameTime;
extern float g_ShakeTime;
extern float g_ShakeMag;

// NORMA OS-pane prototype, driven by ONEDOW_SMOKE "PANE=..." steps.
static bool g_PaneDebug = false;         // outline panes/desktop windows, opaque backdrop
static bool g_PaneArrangeForce = false;   // smoke stand-in for holding Ctrl while paused
static int  g_PaneTestId = 0;

// Rectangles of the panes in front of the one being decorated: its brackets,
// glow and marks are not drawn where a front pane covers them.
static std::vector<os_pane::Rect> g_PaneCover;

// hud_lines::Segment minus the parts inside g_PaneCover.
static void PaneSeg(float x0, float y0, float x1, float y1, float w, hud_lines::Rgb c, float a) {
    if (g_PaneCover.empty()) { hud_lines::Segment(x0, y0, x1, y1, w, c, a); return; }
    // Pieces of [0, 1] along the segment still in sight.
    std::vector<std::pair<float, float>> keep{ { 0.0f, 1.0f } };
    const float dx = x1 - x0, dy = y1 - y0;
    for (const os_pane::Rect& r : g_PaneCover) {
        // Where the segment runs inside r (Liang-Barsky).
        float t0 = 0.0f, t1 = 1.0f;
        const float ps[4] = { -dx, dx, -dy, dy };
        const float qs[4] = { x0 - r.x, r.x + r.w - x0, y0 - r.y, r.y + r.h - y0 };
        bool hit = true;
        for (int i = 0; i < 4 && hit; ++i) {
            if (std::fabs(ps[i]) < 1e-6f) { if (qs[i] < 0.0f) hit = false; continue; }
            const float t = qs[i] / ps[i];
            if (ps[i] < 0.0f) t0 = std::max(t0, t); else t1 = std::min(t1, t);
            if (t0 > t1) hit = false;
        }
        if (!hit) continue;
        std::vector<std::pair<float, float>> next;
        for (const auto& k : keep) {
            if (k.second <= t0 || k.first >= t1) { next.push_back(k); continue; }
            if (k.first < t0) next.push_back({ k.first, t0 });
            if (k.second > t1) next.push_back({ t1, k.second });
        }
        keep.swap(next);
        if (keep.empty()) return;
    }
    for (const auto& k : keep)
        hud_lines::Segment(x0 + dx * k.first, y0 + dy * k.first, x0 + dx * k.second,
                           y0 + dy * k.second, w, c, a);
}

static void DrawPaneOutline(const os_pane::Rect& r, hud_lines::Rgb c, float w, float a) {
    PaneSeg(r.x, r.y, r.x + r.w, r.y, w, c, a);
    PaneSeg(r.x + r.w, r.y, r.x + r.w, r.y + r.h, w, c, a);
    PaneSeg(r.x + r.w, r.y + r.h, r.x, r.y + r.h, w, c, a);
    PaneSeg(r.x, r.y + r.h, r.x, r.y, w, c, a);
}

// A gradient made of lines: outlines stepping away from (outward > 0) or into
// the frame, each fainter than the last.
static void DrawPaneGlow(const os_pane::Rect& r, hud_lines::Rgb c, float alpha,
                         int rings, float step, float outward) {
    for (int i = 1; i <= rings; ++i) {
        const float d = step * (float)i * outward;
        const os_pane::Rect ring{ r.x - d, r.y - d, r.w + d * 2.0f, r.h + d * 2.0f };
        const float fade = 1.0f - (float)(i - 1) / (float)rings;
        DrawPaneOutline(ring, c, 2.0f, alpha * fade * fade);
    }
}

// Screen-space debug view: desktop windows grey, pane frame ice, pane client
// cyan, plus the client's world extent under the current camera zoom.
static void DrawPaneDebug() {
    BindMainShader();
    for (const os_pane::Rect& r : os_pane::DesktopWindows())
        DrawPaneOutline(r, { 0.54f, 0.56f, 0.66f }, 1.5f, 0.85f);
    for (const os_pane::Pane& p : os_pane::Panes()) {
        if (!p.open || p.minimized) continue;
        DrawPaneOutline(p.frame, { 0.72f, 0.94f, 1.0f }, 2.0f, 0.95f);
        DrawPaneOutline(p.client, { 0.20f, 0.88f, 0.94f }, 1.2f, 0.90f);
        const float zoom = g_ViewZoom < 0.01f ? 0.01f : g_ViewZoom;
        wchar_t label[160];
        swprintf_s(label, L"pane %d  world %.0f,%.0f %.0fx%.0f  zoom %.3f  moves %d  refused %d",
                   p.id, ScreenToWorldX(p.client.x), ScreenToWorldY(p.client.y),
                   p.client.w / zoom, p.client.h / zoom, g_ViewZoom, p.moves, p.refusals);
        g_TextS.Draw(label, p.frame.x, p.frame.y + p.frame.h + 8.0f, 0.5f,
                     0.72f, 0.94f, 1.0f, 0.95f);
    }
    BatchFlush();
}

// NORMA's verdict. When the boss wave's clock runs out with NORMA still
// standing, a countdown starts; at zero the player is struck dead and the
// run is lost. Felling NORMA first wins as usual. Seconds left, -1 = idle.
static float g_NormaJudgment = -1.0f;
static bool  g_NormaDoom = false;   // the coming death is the verdict
static constexpr float kNormaJudgmentSeconds = 10.0f;

// NORMA's health under the wave track: a dim full-length line, the bright
// remaining share over it, and a notch where phase 2 begins. Lines only.
static void DrawNormaBossBar(float sw, float scale, float topY) {
    const Monster* boss = nullptr;
    for (const Monster* m : g_MonsterManager.monsters)
        if (m->kind == MobKind::NORMA && m->alive) { boss = m; break; }
    if (!boss) return;
    const float ratio = std::clamp(boss->hp / std::max(1.0f, boss->normaMaxHp), 0.0f, 1.0f);
    const float w = sw * 0.36f, x0 = (sw - w) * 0.5f;
    const float y = topY + 92.0f * scale;
    const hud_lines::Rgb ice{ 0.72f, 0.94f, 1.0f };
    BindMainShader();
    hud_lines::Segment(x0, y, x0 + w, y, 2.0f * scale, ice, 0.22f);
    if (ratio > 0.0f)
        hud_lines::Segment(x0, y, x0 + w * ratio, y, 3.0f * scale, ice, 0.95f);
    const float notch = x0 + w * norma::kPhase2Ratio;
    hud_lines::Segment(notch, y - 6.0f * scale, notch, y + 6.0f * scale, 1.4f * scale, ice, 0.7f);
    BatchFlush();
    const wchar_t* name = boss->normaPhase == 2 ? L"NORMA  II" : L"NORMA";
    const float ts = 0.55f * scale;
    g_TextS.Draw(name, x0, y - 10.0f * scale - g_TextS.Height(name, ts), ts,
                 0.72f, 0.94f, 1.0f, 0.95f);

    // The verdict countdown: a red line that drains under the health bar.
    if (g_NormaJudgment >= 0.0f) {
        const float left = std::clamp(g_NormaJudgment / kNormaJudgmentSeconds, 0.0f, 1.0f);
        const float pulse = g_NormaJudgment < 3.0f
            ? 0.65f + 0.35f * sinf(g_NormaJudgment * 14.0f) : 1.0f;
        const hud_lines::Rgb red{ 1.0f, 0.30f, 0.32f };
        const float y2 = y + 16.0f * scale;
        BindMainShader();
        hud_lines::Segment(x0, y2, x0 + w, y2, 2.0f * scale, red, 0.22f);
        hud_lines::Segment(x0, y2, x0 + w * left, y2, 3.0f * scale, red, 0.95f * pulse);
        BatchFlush();
        static const wchar_t* const FMT[3] = {
            L"심판까지  %.1f", L"Judgment in  %.1f", L"裁きまで  %.1f" };
        wchar_t line[64];
        swprintf_s(line, FMT[std::max(0, std::min(2, LangIndex()))], g_NormaJudgment);
        g_TextS.Draw(line, x0, y2 + 8.0f * scale, ts, 1.0f, 0.36f, 0.38f, 0.95f * pulse);
    }
}

// Ice corner brackets on every open pane, drawn by the game above the pane's
// own frame. They thin and dim as the pane's durability wears down. A pane
// NORMA just claimed gets the claim laser and brackets that close in from
// 1.6x; the pane under a CloseWindow count shows the time left along its top.
static float PaneDurabilityLeft(const os_pane::Pane& p);
struct NormaFlash;
static const NormaFlash* FlashFor(int id);
static float FlashAge(const NormaFlash& f);
static bool CountdownFor(int id, float& left);
static void DrawSummonStars(const os_pane::Pane& p);
static float RamRedFor(int id);
static void DrawPaneHop(const os_pane::Pane& p);
static void DrawRamLane(const os_pane::Pane& p);
static bool ThrowPayloadFor(int id, int& count, float& turn);
static void DrawThrowAppear();
static bool FuseFor(int id, float& share, os_pane::Rect& range);
struct PaneCollapse;
static void DrawPaneCollapses(float dt);
static void DrawPaneBrackets() {
    DrawPaneCollapses(1.0f / 60.0f);
    DrawThrowAppear();
    bool any = false;
    const hud_lines::Rgb ice{ 0.72f, 0.94f, 1.0f };
    for (const os_pane::Pane& p : os_pane::Panes()) {
        if (!p.open || p.minimized || p.covered) continue;
        if (!any) { BindMainShader(); any = true; }
        g_PaneCover.clear();
        for (const os_pane::Pane& q : os_pane::Panes())
            if (q.open && !q.minimized && q.zRank < p.zRank) g_PaneCover.push_back(q.shown);
        os_pane::Rect box = p.shown;
        if (const NormaFlash* flash = FlashFor(p.id)) {
            const float age = FlashAge(*flash);
            if (age < norma::kSnapSeconds) {
                const float t = 1.0f - age / norma::kSnapSeconds;
                const float grow = 1.0f + norma::kSnapGrow * t * t;
                box = { box.x + box.w * 0.5f * (1.0f - grow), box.y + box.h * 0.5f * (1.0f - grow),
                        box.w * grow, box.h * grow };
            }
        }
        DrawSummonStars(p);
        int payload = 0;
        float payloadTurn = 0.0f;
        if (ThrowPayloadFor(p.id, payload, payloadTurn)) {
            // The shots the box holds, circling its middle.
            const float cx = p.shown.x + p.shown.w * 0.5f, cy = p.shown.y + p.shown.h * 0.55f;
            const float rad = std::min(p.shown.w, p.shown.h) * 0.30f;
            for (int k = 0; k < payload; ++k) {
                const float a = payloadTurn + 6.2831853f * (float)k / (float)std::max(1, payload);
                const float r2 = rad * (k % 2 == 0 ? 1.0f : 0.6f);
                const float qx = cx + cosf(a) * r2, qy = cy + sinf(a) * r2, q = 5.0f;
                const os_pane::Rect sq{ qx - q, qy - q, q * 2.0f, q * 2.0f };
                DrawPaneOutline(sq, ice, 1.6f, 0.9f);
            }
        }
        float fuseShare = 0.0f;
        os_pane::Rect blast{};
        if (FuseFor(p.id, fuseShare, blast)) {
            // The blast range, firming up as the fuse burns, and the frame
            // throbbing faster toward the blast.
            const hud_lines::Rgb warn{ 0.86f, 0.80f, 1.0f };
            DrawPaneOutline(blast, warn, 1.2f + 1.8f * fuseShare, 0.30f + 0.55f * fuseShare);
            const float beat = 0.5f + 0.5f * sinf(g_GameTime * (10.0f + 30.0f * fuseShare));
            DrawPaneOutline(p.shown, { 0.92f, 0.99f, 1.0f }, 1.5f + 2.5f * beat, 0.4f + 0.5f * beat);
            PaneSeg(blast.x, blast.y - 6.0f,
                               blast.x + blast.w * (1.0f - fuseShare), blast.y - 6.0f, 3.0f, warn, 0.9f);
        }
        float countdown = 0.0f;
        if (CountdownFor(p.id, countdown)) {
            const float share = std::clamp(countdown / norma::kCloseCountdown, 0.0f, 1.0f);
            PaneSeg(p.shown.x, p.shown.y - 7.0f, p.shown.x + p.shown.w * share,
                               p.shown.y - 7.0f, 3.0f, { 0.86f, 0.80f, 1.0f }, 0.95f);
            // The border throbs faster as the count runs out.
            const float beat = 0.5f + 0.5f * sinf(g_GameTime * (6.0f + 18.0f * (1.0f - share)));
            DrawPaneOutline(p.shown, { 0.86f, 0.80f, 1.0f }, 1.5f + 2.0f * beat, 0.35f + 0.5f * beat);
        }
        // Afterimages of the in-game rectangle flying after its window.
        for (const os_pane::Ghost& g : p.trail) {
            const float fade = 1.0f - g.age / os_pane::kGhostSeconds;
            DrawPaneOutline(g.frame, ice, 1.4f, 0.45f * fade * fade);
        }
        const float left = PaneDurabilityLeft(p);
        // A pane the boss is ramming with glows red, and stronger.
        const float red = RamRedFor(p.id);
        const hud_lines::Rgb tint{ ice.r + (1.0f - ice.r) * red, ice.g + (0.30f - ice.g) * red,
                                   ice.b + (0.26f - ice.b) * red };
        // A held space breathes: a soft gradient out from and into the frame.
        const float breathe = 0.75f + 0.25f * sinf(g_GameTime * 2.2f + (float)p.id);
        const float glow = (0.5f + 0.5f * left) * (1.0f + 1.6f * red);
        DrawPaneGlow(box, tint, 0.30f * breathe * glow, 5 + (int)(3.0f * red), 4.0f, 1.0f);
        DrawPaneGlow(box, tint, 0.18f * breathe * glow, 3, 5.0f, -1.0f);
        const float width = 2.0f + 2.6f * left;
        const float alpha = 0.45f + 0.55f * left;
        const float arm = std::min(90.0f, std::min(box.w, box.h) * 0.22f);
        const float xs[2] = { box.x, box.x + box.w };
        const float ys[2] = { box.y, box.y + box.h };
        for (int cx = 0; cx < 2; ++cx) {
            for (int cy = 0; cy < 2; ++cy) {
                const float x = xs[cx], y = ys[cy];
                const float sx = cx == 0 ? 1.0f : -1.0f, sy = cy == 0 ? 1.0f : -1.0f;
                PaneSeg(x, y, x + sx * arm, y, width, tint, alpha);
                PaneSeg(x, y, x, y + sy * arm, width, tint, alpha);
            }
        }
        DrawRamLane(p);
        DrawPaneHop(p);
    }
    g_PaneCover.clear();
    // While arranging: the window a drag would grab glows, pulsing.
    os_pane::Rect target;
    if (os_pane::ArrangeTarget(target)) {
        if (!any) { BindMainShader(); any = true; }
        const float pulse = 0.6f + 0.4f * sinf((float)glfwGetTime() * 7.0f);
        DrawPaneGlow(target, { 0.92f, 0.99f, 1.0f }, 0.85f * pulse, 6, 3.5f, 1.0f);
        DrawPaneOutline(target, { 0.92f, 0.99f, 1.0f }, 2.5f, 0.95f);
    }
    if (any) BatchFlush();
}

static void DumpPaneState(const char* label, GLFWwindow* window) {
    std::filesystem::create_directories("Screenshots");
    FILE* f = nullptr;
#ifdef _MSC_VER
    if (fopen_s(&f, "Screenshots/pane_state.txt", "a") != 0) f = nullptr;
#else
    f = std::fopen("Screenshots/pane_state.txt", "a");
#endif
    if (!f) return;
    std::fprintf(f, "%s fg=%d focused=%d arrange=%d passthrough=%d state=%d desktop=%zu",
                 label, os_pane::OverlayIsForeground() ? 1 : 0,
                 glfwGetWindowAttrib(window, GLFW_FOCUSED),
                 os_pane::ArrangeMode() ? 1 : 0,
                 glfwGetWindowAttrib(window, GLFW_MOUSE_PASSTHROUGH),
                 (int)g_GameManager.currentState, os_pane::DesktopWindows().size());
    for (const os_pane::Pane& p : os_pane::Panes())
        std::fprintf(f, " [id=%d open=%d min=%d locked=%d frame=%.0f,%.0f,%.0fx%.0f "
                        "client=%.0f,%.0f,%.0fx%.0f moves=%d refused=%d]",
                     p.id, p.open ? 1 : 0, p.minimized ? 1 : 0, p.locked ? 1 : 0,
                     p.frame.x, p.frame.y, p.frame.w, p.frame.h,
                     p.client.x, p.client.y, p.client.w, p.client.h,
                     p.moves, p.refusals);
    std::fputc('\n', f);
    std::fclose(f);
}

static const int MAX_DRONES = 4;
DroneState g_Drones[MAX_DRONES] = {};

//   1초마???�레?�어 ?�치??1개씩 배치, �?5�?지????맵에 ~5�??�시
// Window sizes for active ranged and signal frames.
float GENESIS_WIN_W = 300.0f;
float SWARM_WIN_W = 210.0f;
float g_RfwW = 500.0f, g_RfwH = 500.0f;

struct ChakramState {
    float angle        = 0.0f;
    float hp           = 150.0f;
    float maxHp        = 150.0f;
    bool  alive        = false;
    float respawnTimer = 0.0f;
};
static const int   MAX_CHAKRAMS    = 3;
ChakramState g_Chakrams[MAX_CHAKRAMS] = {};
static const float CHAKRAM_RADIUS = 130.0f;   // 버프: ?��? 공전 (공전�??�격)
static const float CHAKRAM_SIZE   = 30.0f;    // 버프: ??칼날

float g_BulletRainTimer = 0.0f;

// LIGHT_STEP ?�격 감�????�전 HP 기록
float g_PrevHP = 100.0f;

// 초당 EXP ?�적??(?�버?????��??�는 죽음, ?�몹 가??

static void GrantExperience(XpAmount amount) {
    if (amount <= 0) return;
    g_GameManager.xp = XpAdd(g_GameManager.xp, amount);
    g_GameplayTelemetry.RecordExperience(amount);
}

// 게임 ?�작 ??경과 ?�간 (?�폭�?보스 ?�장 ?�?�밍)
float g_GameTime         = 0.0f;

// ??�� 충격??(?�폭�??�폭 / 보스 ?�폰)
struct ShockWave {
    float x, y;
    float life, maxLife;
    float maxRadius;
    float r, g, b;
    bool  active  = false;
    bool  needsBg = false;
};
static const int MAX_SHOCKS = 16;
ShockWave g_ShockWaves[MAX_SHOCKS] = {};

static void SpawnShockWave(float x, float y, float maxR, float life,
                           float r, float g, float b,
                           bool needsBg = false) {
    for (int i = 0; i < MAX_SHOCKS; i++) {
        if (!g_ShockWaves[i].active) {
            g_ShockWaves[i] = { x, y, life, life, maxR, r, g, b, true, needsBg };
            return;
        }
    }
}

// 검�?근접 ?�윙 ?�상 (조�? 방향 ??
float g_MuzzleX = 0.0f, g_MuzzleY = 0.0f, g_MuzzleAng = 0.0f, g_MuzzleTimer = 0.0f;

// NORMA window spaces (SpaceRules.h): open panes become world regions each
// frame, front-most first, and the player's space is resolved from its centre.
static void RebuildPaneSpaces(const SpatialBounds& player) {
    std::vector<const os_pane::Pane*> order;
    for (const os_pane::Pane& p : os_pane::Panes())
        if (p.open && !p.minimized && !p.covered) order.push_back(&p);
    std::sort(order.begin(), order.end(),
              [](const os_pane::Pane* a, const os_pane::Pane* b) { return a->zRank < b->zRank; });
    space::g_Regions.clear();
    for (const os_pane::Pane* p : order)
        space::g_Regions.push_back({ p->id,
            ScreenToWorldX(p->shown.x), ScreenToWorldY(p->shown.y),
            ScreenToWorldX(p->shown.x + p->shown.w), ScreenToWorldY(p->shown.y + p->shown.h) });
    space::g_PlayerSpace = space::SpaceAt(player.x + player.width * 0.5f,
                                          player.y + player.height * 0.5f);
}

// Pane durability: every shot a border stops wears that pane down; a worn
// out pane self-destructs. What the blast does is still undecided, so for
// now it only closes the pane with a shockwave.
struct PaneWear { int id = 0; float hits = 0.0f; };
static std::vector<PaneWear> g_PaneWear;

static PaneWear& WearOf(int id) {
    for (PaneWear& w : g_PaneWear) if (w.id == id) return w;
    g_PaneWear.push_back({ id, 0.0f });
    return g_PaneWear.back();
}

static float PaneMaxDurability(const os_pane::Pane& p) {
    const float zoom = g_ViewZoom < 0.01f ? 0.01f : g_ViewZoom;
    return space::MaxDurability((p.shown.w / zoom) * (p.shown.h / zoom));
}

// 0..1 durability left, 1 for panes nothing has hit yet.
static float PaneDurabilityLeft(const os_pane::Pane& p) {
    for (const PaneWear& w : g_PaneWear)
        if (w.id == p.id) return std::max(0.0f, 1.0f - w.hits / PaneMaxDurability(p));
    return 1.0f;
}

// NORMA's windows (NormaBoss.h). Panes the boss claimed or opened are
// released when its pattern ends or it falls; every claim also leaves a
// flash record for the laser and the bracket snap.
struct NormaFlash { int id = 0; float bornAt = 0.0f; float fromX = 0.0f, fromY = 0.0f; };
static std::vector<int> g_NormaPanes;
static std::vector<NormaFlash> g_NormaFlashes;
struct NormaCountdown { int id = 0; float left = 0.0f; };
static NormaCountdown g_NormaCountdown;

static Monster* FindNorma() {
    for (Monster* m : g_MonsterManager.monsters)
        if (m->kind == MobKind::NORMA && m->alive) return m;
    return nullptr;
}

static const os_pane::Pane* PaneById(int id) {
    for (const os_pane::Pane& p : os_pane::Panes())
        if (p.id == id && p.open) return &p;
    return nullptr;
}

static void FlashNormaPane(int id, const Monster& boss) {
    g_NormaFlashes.push_back({ id, g_GameTime, boss.worldX, boss.worldY });
}

struct PaneCollapse;
static void CollapsePane(const os_pane::Pane& pane);

// Lets go of the real windows the boss claimed. Panes it opened itself stay
// until the player closes them or they wear out.
static void ReleaseNormaClaims() {
    std::vector<int> kept;
    for (int id : g_NormaPanes) {
        const os_pane::Pane* pane = PaneById(id);
        if (!pane) continue;
        if (pane->adopted) os_pane::Close(id);
        else kept.push_back(id);
    }
    g_NormaPanes.swap(kept);
}

// The boss fell: every pane it opened folds shut with it.
static void ReleaseNormaPanes() {
    for (int id : g_NormaPanes)
        if (const os_pane::Pane* pane = PaneById(id)) {
            if (!pane->adopted) CollapsePane(*pane);
            os_pane::Close(id);
        }
    g_NormaPanes.clear();
}

// Wandering of the panes the boss opened (screen pixels).
struct PaneDrift {
    int id = 0;
    int stage = 0;             // 0 holding, 1 marking the next spot, 2 moving
    float timer = 0.0f;
    float holdFor = 6.0f;
    // Sub-pixel position: windows sit on whole pixels.
    float x = 0.0f, y = 0.0f;
    bool placed = false;
    float hopFromX = 0.0f, hopFromY = 0.0f, hopToX = 0.0f, hopToY = 0.0f;
    // Refuge pull: glides from (fromX, fromY) to (toX, toY) while pullLeft > 0.
    float pullLeft = 0.0f;
    float fromX = 0.0f, fromY = 0.0f, toX = 0.0f, toY = 0.0f;
};
static std::vector<PaneDrift> g_PaneDrifts;

static float RandRange(float lo, float hi) {
    return lo + (hi - lo) * (float)(rand() % 1000) / 999.0f;
}

static void StartPaneDrift(int id) {
    PaneDrift d;
    d.id = id;
    d.holdFor = RandRange(norma::kPaneHoldMin, norma::kPaneHoldMax);
    g_PaneDrifts.push_back(d);
}

static bool PaneRamBusy(int id);
static void PickPaneSpot(float w, float h, float psx, float psy, float& x, float& y);

// The boss's panes hold still, then every few seconds hop: the next spot is
// marked for a moment, then the pane glides there (smoothstep). A refuge
// pull or a ram takes the pane over meanwhile. psx, psy: the player on screen.
static void DriftNormaPanes(float dt, float psx, float psy) {
    for (PaneDrift& d : g_PaneDrifts) {
        const os_pane::Pane* pane = PaneById(d.id);
        if (!pane || pane->flying || pane->minimized || PaneRamBusy(d.id)) continue;
        // Resync when the window was put elsewhere (a snap, a ram); that
        // also cancels a pull and restarts the hold.
        if (!d.placed || std::fabs(d.x - pane->frame.x) > 2.0f ||
            std::fabs(d.y - pane->frame.y) > 2.0f) {
            d.x = pane->frame.x;
            d.y = pane->frame.y;
            d.placed = true;
            d.pullLeft = 0.0f;
            d.stage = 0;
            d.timer = 0.0f;
        }
        if (d.pullLeft > 0.0f) {
            d.pullLeft = std::max(0.0f, d.pullLeft - dt);
            const float t = 1.0f - d.pullLeft / norma::kRefugePull;
            const float s = t * t * (3.0f - 2.0f * t);
            d.x = d.fromX + (d.toX - d.fromX) * s;
            d.y = d.fromY + (d.toY - d.fromY) * s;
            os_pane::Place(d.id, std::round(d.x), std::round(d.y));
            if (d.pullLeft <= 0.0f) { d.stage = 0; d.timer = 0.0f; }
            continue;
        }
        d.timer += dt;
        if (d.stage == 0) {
            if (d.timer < d.holdFor) continue;
            float tx = d.x, ty = d.y;
            PickPaneSpot(pane->frame.w, pane->frame.h, psx, psy, tx, ty);
            d.hopFromX = d.x;
            d.hopFromY = d.y;
            d.hopToX = tx;
            d.hopToY = ty;
            d.stage = 1;
            d.timer = 0.0f;
        } else if (d.stage == 1) {
            if (d.timer < norma::kPaneHopWarn) continue;
            d.stage = 2;
            d.timer = 0.0f;
        } else {
            const float t = std::min(1.0f, d.timer / norma::kPaneHopMove);
            const float s = t * t * (3.0f - 2.0f * t);
            d.x = d.hopFromX + (d.hopToX - d.hopFromX) * s;
            d.y = d.hopFromY + (d.hopToY - d.hopFromY) * s;
            os_pane::Place(d.id, std::round(d.x), std::round(d.y));
            if (t >= 1.0f) {
                d.stage = 0;
                d.timer = 0.0f;
                d.holdFor = RandRange(norma::kPaneHoldMin, norma::kPaneHoldMax);
            }
        }
    }
    g_PaneDrifts.erase(std::remove_if(g_PaneDrifts.begin(), g_PaneDrifts.end(),
        [](const PaneDrift& d) { return PaneById(d.id) == nullptr; }), g_PaneDrifts.end());
}

// The next spot of a pane about to hop: its brackets, fading in.
static void DrawPaneHop(const os_pane::Pane& p) {
    for (const PaneDrift& d : g_PaneDrifts) {
        if (d.id != p.id || d.stage != 1) continue;
        const float k = std::clamp(d.timer / norma::kPaneHopWarn, 0.0f, 1.0f);
        const hud_lines::Rgb ice{ 0.72f, 0.94f, 1.0f };
        const float arm = std::min(40.0f, std::min(p.shown.w, p.shown.h) * 0.18f);
        const float xs[2] = { d.hopToX, d.hopToX + p.shown.w };
        const float ys[2] = { d.hopToY, d.hopToY + p.shown.h };
        for (int cx = 0; cx < 2; ++cx)
            for (int cy = 0; cy < 2; ++cy) {
                const float x = xs[cx], y = ys[cy];
                const float sx = cx == 0 ? 1.0f : -1.0f, sy = cy == 0 ? 1.0f : -1.0f;
                PaneSeg(x, y, x + sx * arm, y, 1.6f, ice, 0.25f + 0.55f * k);
                PaneSeg(x, y, x, y + sy * arm, 1.6f, ice, 0.25f + 0.55f * k);
            }
    }
}

// The boss's own open panes (not claimed real windows).
static std::vector<const os_pane::Pane*> NormaOwnPanes() {
    std::vector<const os_pane::Pane*> own;
    for (int id : g_NormaPanes)
        if (const os_pane::Pane* pane = PaneById(id))
            if (!pane->adopted) own.push_back(pane);
    return own;
}

// The player's centre in world space, kept current by the main loop for the
// pane patterns that aim at it.
static float g_NormaPlayerX = 0.0f, g_NormaPlayerY = 0.0f;
// Smoothed player velocity (world px/s) for the boss's predictive aim.
static float g_NormaPlayerVX = 0.0f, g_NormaPlayerVY = 0.0f;
static float playerCenterXForNorma() { return g_NormaPlayerX; }
static float playerCenterYForNorma() { return g_NormaPlayerY; }

// 8 Throw: the box the boss hurls (screen pixels).
struct ThrowBox {
    bool active = false;
    int id = 0;
    int stage = 0;             // -1 growing out of the boss, 0 filling, 1 flying, 2 fuse
    float timer = 0.0f;
    float x = 0.0f, y = 0.0f;  // sub-pixel top-left
    float tx = 0.0f, ty = 0.0f;
    os_pane::Rect spot;        // where the box opens
};
static ThrowBox g_ThrowBox;

// 10 Ram: the pane the boss hurls (screen pixels).
struct PaneRam {
    int id = 0;
    int stage = 0;               // 0 aim, 1 flying, 2 settling
    float timer = 0.0f;
    float fromX = 0.0f, fromY = 0.0f, fromW = 0.0f, fromH = 0.0f;
    float x = 0.0f, y = 0.0f;    // sub-pixel top-left
    float w = 0.0f, h = 0.0f;    // current size (squeezing during the aim)
    float dirX = 1.0f, dirY = 0.0f;
    bool playerInside = false;   // inside when last checked (no hit then)
    bool hitPlayer = false;
    std::vector<const void*> hitMobs;
};
static PaneRam g_PaneRam;

static bool PaneRamBusy(int id) { return id != 0 && g_PaneRam.id == id; }

// 0..1: how red the pane glows (control by the boss).
static float RamRedFor(int id) {
    if (!PaneRamBusy(id)) return 0.0f;
    if (g_PaneRam.stage == 0) {
        const Monster* boss = FindNorma();
        const float aim = norma::RamAimFor(boss ? boss->normaPhase : 1);
        return 0.4f + 0.6f * std::clamp(g_PaneRam.timer / aim, 0.0f, 1.0f);
    }
    if (g_PaneRam.stage == 1) return 1.0f;
    return 1.0f - std::clamp(g_PaneRam.timer / norma::kRamSettle, 0.0f, 1.0f);
}

// 9 Summon: panes loading mobs.
// A summon gate "compiles" its mobs as a constellation: one star per mob at
// the spot it will appear (u, v: shares of the pane, so it rides along with
// a drifting pane), linked in a loop by a bright head over the load time.
struct SummonGate {
    int id = 0;
    float timer = 0.0f;
    int count = 0;
    std::vector<float> u, v;
};
static std::vector<SummonGate> g_SummonGates;


// Star spots for a gate: spread over the pane below the title bar, kept
// apart, and ordered around the middle so the loop never crosses itself.
static void PlanGateStars(SummonGate& gate) {
    gate.u.clear();
    gate.v.clear();
    for (int k = 0; k < gate.count; ++k) {
        float bu = 0.5f, bv = 0.6f, bestGap = -1.0f;
        for (int tries = 0; tries < 16; ++tries) {
            const float cu = RandRange(0.14f, 0.86f), cv = RandRange(0.30f, 0.86f);
            float gap = 10.0f;
            for (size_t j = 0; j < gate.u.size(); ++j)
                gap = std::min(gap, std::hypot(cu - gate.u[j], cv - gate.v[j]));
            if (gap > bestGap) { bestGap = gap; bu = cu; bv = cv; }
        }
        gate.u.push_back(bu);
        gate.v.push_back(bv);
    }
    std::vector<int> order(gate.u.size());
    for (size_t k = 0; k < order.size(); ++k) order[k] = (int)k;
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        return std::atan2(gate.v[a] - 0.58f, gate.u[a] - 0.5f) <
               std::atan2(gate.v[b] - 0.58f, gate.u[b] - 0.5f);
    });
    std::vector<float> u2, v2;
    for (int k : order) { u2.push_back(gate.u[k]); v2.push_back(gate.v[k]); }
    gate.u.swap(u2);
    gate.v.swap(v2);
}

static void SpawnNormaShot(float x, float y, float angle, float speed) {
    Bullet shot(x, y, x + cosf(angle) * 100.0f, y + sinf(angle) * 100.0f);
    shot.isEnemy = true;
    const Monster* boss = FindNorma();
    shot.speed = speed * (boss && boss->normaPhase == 2 ? norma::kPhase2ShotSpeed : 1.0f);
    shot.enemyDmg = norma::kShotDamage;
    shot.sizeScale = 1.05f;
    shot.color = glm::vec3(0.72f, 0.94f, 1.0f);
    shot.normaShot = true;
    g_Bullets.push_back(shot);
}

// A random spot anywhere on screen for a new boss pane, preferring one that
// overlaps the open panes least, so panes do not pile up (and blow up in a
// chain). A spot that would open right over the player scores worse too.
static void PickPaneSpot(float w, float h, float psx, float psy, float& x, float& y) {
    const float sw = (float)screenWidth, sh = (float)screenHeight;
    float best = -1.0f;
    for (int tries = 0; tries < 40; ++tries) {
        const float cx = RandRange(0.0f, std::max(0.0f, sw - w));
        const float cy = RandRange(0.0f, std::max(0.0f, sh - h));
        float score = 0.0f;
        for (const os_pane::Pane& p : os_pane::Panes()) {
            if (!p.open || p.minimized) continue;
            const float ox = std::max(0.0f, std::min(cx + w, p.frame.x + p.frame.w) - std::max(cx, p.frame.x));
            const float oy = std::max(0.0f, std::min(cy + h, p.frame.y + p.frame.h) - std::max(cy, p.frame.y));
            score += ox * oy;
        }
        constexpr float kPlayerMargin = 80.0f;
        if (psx > cx - kPlayerMargin && psx < cx + w + kPlayerMargin &&
            psy > cy - kPlayerMargin && psy < cy + h + kPlayerMargin)
            score += w * h;
        if (best < 0.0f || score < best) {
            best = score;
            x = cx;
            y = cy;
            if (score <= 0.0f) return;
        }
    }
}

// Serves the boss's Asks and keeps its Hide target on its pane.
// Seconds until the boss opens its next pane (see norma::kPaneSpawn*).
static float g_NormaPaneSpawnTimer = norma::kPaneSpawnFirst;

// A new boss pane of random size at a random spot (see PickPaneSpot).
static int OpenNormaPane(Monster& boss, float psx, float psy) {
    const float sw = (float)screenWidth, sh = (float)screenHeight;
    const float w = std::min(sw * norma::kPaneMaxShareW, RandRange(norma::kRefugeMinW, norma::kRefugeMaxW));
    const float h = std::min(sh * norma::kPaneMaxShareH, RandRange(norma::kRefugeMinH, norma::kRefugeMaxH));
    float x = 0.0f, y = 0.0f;
    PickPaneSpot(w, h, psx, psy, x, y);
    const int id = os_pane::Open(x, y, w, h);
    if (id <= 0) return 0;
    g_NormaPanes.push_back(id);
    StartPaneDrift(id);
    (void)boss;   // opens quietly: no claim laser for a timed pane
    return id;
}

static void ServeNormaAsks(const SpatialBounds& player) {
    Monster* boss = FindNorma();
    std::vector<norma::Ask> asks;
    asks.swap(norma::g_Asks);
    if (!boss) {
        ReleaseNormaPanes();
        g_NormaCountdown = {};
        if (g_ThrowBox.id != 0) os_pane::Close(g_ThrowBox.id);
        g_ThrowBox = {};
        g_SummonGates.clear();
        g_PaneRam = {};
        g_NormaPaneSpawnTimer = norma::kPaneSpawnFirst;
        return;
    }
    // Forget panes the player closed or that wore out.
    g_NormaPanes.erase(std::remove_if(g_NormaPanes.begin(), g_NormaPanes.end(),
        [](int id) { return PaneById(id) == nullptr; }), g_NormaPanes.end());
    const float sw = (float)screenWidth, sh = (float)screenHeight;
    const float psx = W2SX(player.x + player.width * 0.5f);
    const float psy = W2SY(player.y + player.height * 0.5f);
    for (norma::Ask ask : asks) {
        switch (ask) {
        case norma::Ask::ClaimFarWindow: {
            // The real window farthest from the player, big enough to fight
            // in and not covering most of the screen; else one of our own.
            const std::vector<os_pane::Rect>& desk = os_pane::DesktopWindows();
            int best = -1;
            float bestDist = -1.0f;
            for (int i = 0; i < (int)desk.size(); ++i) {
                const os_pane::Rect& r = desk[i];
                if (r.w < 360.0f || r.h < 260.0f || r.w * r.h > sw * sh * 0.6f) continue;
                const float ddx = r.x + r.w * 0.5f - psx, ddy = r.y + r.h * 0.5f - psy;
                const float dist = ddx * ddx + ddy * ddy;
                if (dist > bestDist) { bestDist = dist; best = i; }
            }
            int id = best >= 0 ? os_pane::Adopt(best) : 0;
            bool fresh = id > 0;
            if (id == 0) {
                // No real window: hide in the farthest pane the boss already
                // opened. It never opens a new pane around itself for this;
                // with none to use, the boss falls back on a direct attack.
                const os_pane::Pane* farthest = nullptr;
                float farDist = -1.0f;
                for (const os_pane::Pane* pane : NormaOwnPanes()) {
                    const float ddx = pane->shown.x + pane->shown.w * 0.5f - psx;
                    const float ddy = pane->shown.y + pane->shown.h * 0.5f - psy;
                    if (ddx * ddx + ddy * ddy > farDist) { farDist = ddx * ddx + ddy * ddy; farthest = pane; }
                }
                if (farthest) id = farthest->id;
            }
            if (id > 0) {
                if (fresh) {
                    g_NormaPanes.push_back(id);
                    if (const os_pane::Pane* pane = PaneById(id))
                        if (!pane->adopted) StartPaneDrift(id);
                }
                FlashNormaPane(id, *boss);
                boss->normaPaneId = id;
            } else {
                boss->normaPaneId = -1;
            }
            break;
        }
        case norma::Ask::OpenRefuges: {
            // The shelters are the panes already open (opened on a timer,
            // not by this pattern): they light up before the flood. One must
            // be reachable in the telegraph, so when none is within reach
            // the nearest glides over (or, with none open, one opens there).
            const float reach = std::clamp(norma::kRefugeReach * g_Stats.moveSpeedMult,
                                           norma::kRefugeReachMin, norma::kRefugeReachMax) * g_ViewZoom;
            const os_pane::Pane* nearest = nullptr;
            float nearestGap = 0.0f;
            for (const os_pane::Pane* pane : NormaOwnPanes()) {
                FlashNormaPane(pane->id, *boss);
                if (pane->minimized || pane->flying) continue;
                const os_pane::Rect& r = pane->frame;
                const float gx = std::max({ r.x - psx, 0.0f, psx - (r.x + r.w) });
                const float gy = std::max({ r.y - psy, 0.0f, psy - (r.y + r.h) });
                const float gap = std::sqrt(gx * gx + gy * gy);
                if (!nearest || gap < nearestGap) { nearest = pane; nearestGap = gap; }
            }
            if (nearest && nearestGap <= reach) break;
            // Where a w x h pane sits with its near edge kRefugeGap of reach
            // from the player, toward (ux, uy).
            auto spotToward = [&](float w, float h, float ux, float uy, float& x, float& y) {
                const float along = reach * norma::kRefugeGap +
                                    std::fabs(ux) * w * 0.5f + std::fabs(uy) * h * 0.5f;
                x = std::clamp(psx + ux * along - w * 0.5f, 0.0f, std::max(0.0f, sw - w));
                y = std::clamp(psy + uy * along - h * 0.5f, 0.0f, std::max(0.0f, sh - h));
            };
            if (nearest) {
                const os_pane::Rect& r = nearest->frame;
                float ux = r.x + r.w * 0.5f - psx, uy = r.y + r.h * 0.5f - psy;
                const float len = std::max(1.0f, std::sqrt(ux * ux + uy * uy));
                ux /= len;
                uy /= len;
                for (PaneDrift& d : g_PaneDrifts) {
                    if (d.id != nearest->id) continue;
                    d.fromX = r.x;
                    d.fromY = r.y;
                    d.x = r.x;
                    d.y = r.y;
                    d.placed = true;
                    spotToward(r.w, r.h, ux, uy, d.toX, d.toY);
                    d.pullLeft = norma::kRefugePull;
                }
            } else if ((int)NormaOwnPanes().size() < norma::MaxBossPanesFor(boss->normaPhase)) {
                const float w = std::min(sw * norma::kPaneMaxShareW, RandRange(norma::kRefugeMinW, norma::kRefugeMaxW));
                const float h = std::min(sh * norma::kPaneMaxShareH, RandRange(norma::kRefugeMinH, norma::kRefugeMaxH));
                const float a = RandRange(0.0f, 6.2831853f);
                float x = 0.0f, y = 0.0f;
                spotToward(w, h, cosf(a), sinf(a), x, y);
                const int id = os_pane::Open(x, y, w, h);
                if (id > 0) {
                    g_NormaPanes.push_back(id);
                    StartPaneDrift(id);
                    FlashNormaPane(id, *boss);
                }
            }
            break;
        }
        case norma::Ask::ClosePlayerWindow: {
            const int id = space::g_PlayerSpace;
            if (id <= 0 || !PaneById(id)) {
                boss->normaPaneId = -1;
                break;
            }
            os_pane::SetLocked(id, true);
            g_NormaCountdown = { id, norma::kCloseCountdown };
            FlashNormaPane(id, *boss);
            boss->normaPaneId = id;
            break;
        }
        case norma::Ask::ReleaseWindows:
            ReleaseNormaClaims();
            break;
        case norma::Ask::ThrowBox: {
            if (g_ThrowBox.active) break;
            // The box opens beside the boss, on the player's side.
            const float bsx = W2SX(boss->worldX), bsy = W2SY(boss->worldY);
            const float ddx = psx - bsx, ddy = psy - bsy;
            const float len = std::max(1.0f, std::sqrt(ddx * ddx + ddy * ddy));
            const float w = norma::kThrowW, h = norma::kThrowH;
            const float cx = std::clamp(bsx + ddx / len * 220.0f, w * 0.5f, sw - w * 0.5f);
            const float cy = std::clamp(bsy + ddy / len * 220.0f, h * 0.5f, sh - h * 0.5f);
            // It opens after growing out of the boss (TickThrowBox).
            g_ThrowBox = {};
            g_ThrowBox.active = true;
            g_ThrowBox.stage = -1;
            g_ThrowBox.spot = { cx - w * 0.5f, cy - h * 0.5f, w, h };
            break;
        }
        case norma::Ask::RamWindow: {
            // The pane farthest from the player gets the longest run at it.
            // Gates, fusing and minimized panes are left alone.
            const os_pane::Pane* pick = nullptr;
            float pickDist = -1.0f;
            for (const os_pane::Pane* pane : NormaOwnPanes()) {
                float fuse = 0.0f;
                os_pane::Rect blast{};
                if (pane->minimized || pane->flying || FuseFor(pane->id, fuse, blast)) continue;
                bool gate = false;
                for (const SummonGate& g : g_SummonGates) gate = gate || g.id == pane->id;
                if (gate || pane->id == g_NormaCountdown.id) continue;
                const float ddx = pane->frame.x + pane->frame.w * 0.5f - psx;
                const float ddy = pane->frame.y + pane->frame.h * 0.5f - psy;
                if (ddx * ddx + ddy * ddy > pickDist) { pickDist = ddx * ddx + ddy * ddy; pick = pane; }
            }
            if (!pick || g_PaneRam.id != 0) {
                boss->normaPaneId = -1;   // falls back on a direct attack
                break;
            }
            g_PaneRam = {};
            g_PaneRam.id = pick->id;
            g_PaneRam.fromX = g_PaneRam.x = pick->frame.x;
            g_PaneRam.fromY = g_PaneRam.y = pick->frame.y;
            g_PaneRam.fromW = g_PaneRam.w = pick->frame.w;
            g_PaneRam.fromH = g_PaneRam.h = pick->frame.h;
            FlashNormaPane(pick->id, *boss);
            boss->normaPaneId = pick->id;
            break;
        }
        case norma::Ask::OpenGates: {
            // Gates are the boss's open panes, up to the gate count; it opens
            // none for this (with none open, only the aimed bursts remain).
            const int perGate = boss->normaPhase >= 2 ? norma::kSummonPhase2 : norma::kSummonPhase1;
            std::vector<int> gates;
            for (const os_pane::Pane* pane : NormaOwnPanes())
                if ((int)gates.size() < norma::kSummonGates) gates.push_back(pane->id);
            for (int id : gates) {
                bool already = false;
                for (const SummonGate& g : g_SummonGates) already = already || g.id == id;
                if (already) continue;
                SummonGate gate;
                gate.id = id;
                gate.count = perGate;
                PlanGateStars(gate);
                g_SummonGates.push_back(gate);
                FlashNormaPane(id, *boss);
            }
            break;
        }
        }
    }
    if (boss->normaPattern == norma::Pattern::Hide && boss->normaPaneId > 0) {
        const os_pane::Pane* pane = PaneById(boss->normaPaneId);
        if (!pane || pane->minimized) {
            boss->normaPaneLost = true;
        } else {
            boss->normaTargetX = ScreenToWorldX(pane->shown.x + pane->shown.w * 0.5f);
            boss->normaTargetY = ScreenToWorldY(pane->shown.y + pane->shown.h * 0.5f);
        }
    }
    g_NormaFlashes.erase(std::remove_if(g_NormaFlashes.begin(), g_NormaFlashes.end(),
        [](const NormaFlash& f) { return g_GameTime - f.bornAt > 1.0f; }), g_NormaFlashes.end());
}

// The boss opens a pane every few seconds while under the cap; at the cap the
// turn is skipped (the timer still restarts).
static void TickNormaPaneSpawn(float dt, const SpatialBounds& player) {
    Monster* boss = FindNorma();
    if (!boss) return;
    g_NormaPaneSpawnTimer -= dt;
    if (g_NormaPaneSpawnTimer > 0.0f) return;
    g_NormaPaneSpawnTimer = boss->normaPhase >= 2 ? norma::kPaneSpawnPhase2 : norma::kPaneSpawnPhase1;
    if ((int)NormaOwnPanes().size() >= norma::MaxBossPanesFor(boss->normaPhase)) return;
    OpenNormaPane(*boss, W2SX(player.x + player.width * 0.5f), W2SY(player.y + player.height * 0.5f));
}

// A pane folding shut: its rectangle drawn shrinking to the centre.
struct PaneCollapse { os_pane::Rect rect; float age = 0.0f; };
static std::vector<PaneCollapse> g_PaneCollapses;
constexpr float kPaneCollapseSeconds = 0.35f;

static void Shake(float seconds, float magnitude) {
    g_ShakeTime = std::max(g_ShakeTime, seconds);
    g_ShakeMag = std::max(g_ShakeMag, magnitude);
}

// Turns the boss's effect cues into shockwaves, sparks and shake; and gives
// its fall a send-off.
static float g_NormaLastX = 0.0f, g_NormaLastY = 0.0f;
static bool g_NormaWasAlive = false;
static void PlayNormaEvents() {
    const float ir = 0.72f, ig = 0.94f, ib = 1.0f;
    for (const norma::Event& e : norma::g_Events) {
        switch (e.fx) {
        case norma::Fx::Volley:
            for (int k = 0; k < 4; ++k) {
                const float a = e.a + 1.5707963f * (float)k;
                SpawnSparks(e.x + cosf(a) * 62.0f, e.y + sinf(a) * 62.0f, 2, ir, ig, ib, 220.0f);
            }
            break;
        case norma::Fx::Muzzle:
            SpawnSparks(e.x + cosf(e.a) * 70.0f, e.y + sinf(e.a) * 70.0f, 2, ir, ig, ib, 260.0f);
            if (g_ShakeTime < 0.05f) Shake(0.05f, 2.5f);
            break;
        case norma::Fx::Burst:
            SpawnShockWave(e.x, e.y, 180.0f + 160.0f * e.a, 0.45f, ir, ig, ib);
            SpawnSparks(e.x, e.y, (int)(10.0f + 12.0f * e.a), 0.92f, 0.99f, 1.0f, 380.0f);
            Shake(0.18f + 0.10f * e.a, 6.0f + 6.0f * e.a);
            break;
        case norma::Fx::StarOut:
            // A health star went dark: a short flare and a beat of hit stop.
            SpawnSparks(e.x, e.y, 16, ir, ig, ib, 420.0f);
            SpawnShockWave(e.x, e.y, 150.0f, 0.3f, ir, ig, ib);
            Shake(0.16f, 7.0f);
            TriggerHitStop(0.05f);
            break;
        case norma::Fx::Wave:
            SpawnShockWave(e.x, e.y, e.a * 1.6f, 1.6f, 0.76f, 0.80f, 1.0f);
            SpawnSparks(e.x, e.y, 6, ir, ig, ib, 260.0f);
            Shake(0.10f, 5.0f);
            break;
        case norma::Fx::EnterPane:
        case norma::Fx::LeavePane:
            SpawnShockWave(e.x, e.y, 150.0f, 0.35f, 0.92f, 0.99f, 1.0f);
            SpawnEnemyExplosion(e.x, e.y, ir, ig, ib, false);
            Shake(0.20f, 9.0f);
            break;
        case norma::Fx::PhaseShift:
            SpawnShockWave(e.x, e.y, 520.0f, 0.70f, 0.92f, 0.99f, 1.0f);
            SpawnShockWave(e.x, e.y, 320.0f, 0.50f, 0.76f, 0.70f, 1.0f);
            SpawnShockWave(e.x, e.y, 160.0f, 0.35f, 1.0f, 1.0f, 1.0f);
            SpawnEnemyExplosion(e.x, e.y, ir, ig, ib, true);
            SpawnSparks(e.x, e.y, 40, 0.92f, 0.99f, 1.0f, 520.0f);
            Shake(0.6f, 24.0f);
            TriggerHitStop(0.12f);
            TriggerFlash(0.72f, 0.94f, 1.0f, 0.22f);
            break;
        }
    }
    norma::g_Events.clear();

    const Monster* boss = FindNorma();
    if (boss) {
        g_NormaLastX = boss->worldX;
        g_NormaLastY = boss->worldY;
        g_NormaWasAlive = true;
    } else if (g_NormaWasAlive) {
        g_NormaWasAlive = false;
        for (int i = 0; i < 3; ++i)
            SpawnShockWave(g_NormaLastX, g_NormaLastY, 300.0f + 260.0f * (float)i,
                           0.5f + 0.25f * (float)i, 0.92f, 0.99f, 1.0f);
        SpawnEnemyExplosion(g_NormaLastX, g_NormaLastY, 0.72f, 0.94f, 1.0f, true);
        SpawnSparks(g_NormaLastX, g_NormaLastY, 60, 0.92f, 0.99f, 1.0f, 640.0f);
        Shake(0.8f, 30.0f);
        TriggerHitStop(0.18f);
        TriggerFlash(0.92f, 0.99f, 1.0f, 0.30f);
    }
}

// 8 Throw: fill, fly, fuse, burst.
static void TickThrowBox(float dt) {
    if (!g_ThrowBox.active) return;
    if (g_ThrowBox.stage == -1) {
        g_ThrowBox.timer += dt;
        if (g_ThrowBox.timer < norma::kThrowAppear) return;
        const os_pane::Rect& r = g_ThrowBox.spot;
        const int id = os_pane::Open(r.x, r.y, r.w, r.h);
        const Monster* boss = FindNorma();
        if (id <= 0 || !boss) { g_ThrowBox = {}; return; }
        g_ThrowBox.id = id;
        g_ThrowBox.stage = 0;
        g_ThrowBox.timer = 0.0f;
        FlashNormaPane(id, *boss);
        SpawnSparks(ScreenToWorldX(r.x + r.w * 0.5f), ScreenToWorldY(r.y + r.h * 0.5f),
                    14, 0.92f, 0.99f, 1.0f, 320.0f);
        Shake(0.12f, 6.0f);
        return;
    }
    const os_pane::Pane* pane = PaneById(g_ThrowBox.id);
    if (!pane || pane->minimized) {           // closed in time: no burst
        if (pane) os_pane::Close(g_ThrowBox.id);
        g_ThrowBox = {};
        return;
    }
    ThrowBox& box = g_ThrowBox;
    box.timer += dt;
    if (box.stage == 0) {
        if (box.timer < norma::kThrowFill) return;
        // Aim where the player will be when the box lands (about 0.8 s of
        // flight), keeping the box on screen.
        const float pcx = W2SX(playerCenterXForNorma() + g_NormaPlayerVX * 0.8f);
        const float pcy = W2SY(playerCenterYForNorma() + g_NormaPlayerVY * 0.8f);
        box.tx = std::clamp(pcx - pane->frame.w * 0.5f, 0.0f, (float)screenWidth - pane->frame.w);
        box.ty = std::clamp(pcy - pane->frame.h * 0.5f, 0.0f, (float)screenHeight - pane->frame.h);
        box.x = pane->frame.x;
        box.y = pane->frame.y;
        box.stage = 1;
        box.timer = 0.0f;
        return;
    }
    if (box.stage == 1) {
        const float ddx = box.tx - box.x, ddy = box.ty - box.y;
        const float remaining = std::sqrt(ddx * ddx + ddy * ddy);
        const float step = norma::kThrowStartSpeed * std::exp(box.timer / norma::kThrowRamp) * dt;
        if (step >= remaining) {
            box.x = box.tx;
            box.y = box.ty;
            box.stage = 2;
            box.timer = 0.0f;
            Shake(0.15f, 8.0f);
        } else {
            box.x += ddx / remaining * step;
            box.y += ddy / remaining * step;
        }
        os_pane::Place(box.id, std::round(box.x), std::round(box.y));
        return;
    }
    if (box.timer < norma::kThrowFuse) return;
    // Burst: the shots it held fly out from where it landed.
    const float cx = ScreenToWorldX(pane->shown.x + pane->shown.w * 0.5f);
    const float cy = ScreenToWorldY(pane->shown.y + pane->shown.h * 0.5f);
    CollapsePane(*pane);
    os_pane::Close(box.id);
    g_ThrowBox = {};
    const float spin = RandRange(0.0f, 6.2831853f);
    for (int i = 0; i < norma::kThrowRing; ++i)
        SpawnNormaShot(cx, cy, spin + 6.2831853f * (float)i / norma::kThrowRing,
                       norma::kThrowRingSpeed);
    for (int i = 0; i < norma::kThrowInner; ++i)
        SpawnNormaShot(cx, cy, spin + 6.2831853f * ((float)i + 0.5f) / norma::kThrowInner,
                       norma::kThrowInnerSpeed);
    SpawnShockWave(cx, cy, 360.0f, 0.5f, 0.92f, 0.99f, 1.0f);
    SpawnEnemyExplosion(cx, cy, 0.72f, 0.94f, 1.0f, true);
    SpawnSparks(cx, cy, 30, 0.92f, 0.99f, 1.0f, 460.0f);
    Shake(0.35f, 16.0f);
}

// 9 Summon: gates load, then mobs step out inside them.
static void TickSummonGates(float dt) {
    const float hpMul = g_Stats.monsterHpMult * GetDifficultyParams(g_Difficulty).enemyHpMultiplier;
    for (SummonGate& gate : g_SummonGates) {
        const os_pane::Pane* pane = PaneById(gate.id);
        if (!pane || pane->minimized) { gate.timer = -1.0f; continue; }   // load broken
        gate.timer += dt;
        if (gate.timer < norma::kSummonLoad) continue;
        // Each mob appears on its star.
        const Monster* boss = FindNorma();
        for (size_t k = 0; k < gate.u.size(); ++k) {
            const float mx = ScreenToWorldX(pane->shown.x + pane->shown.w * gate.u[k]);
            const float my = ScreenToWorldY(pane->shown.y + pane->shown.h * gate.v[k]);
            Monster* mob = new Monster(mx, my, hpMul);
            if (boss && boss->normaPhase >= 2 && rand() % 10 < 3) mob->MakeKind(MobKind::GIMBAL);
            g_MonsterManager.monsters.push_back(mob);
            SpawnSparks(mx, my, 8, 0.92f, 0.99f, 1.0f, 260.0f);
            SpawnShockWave(mx, my, 70.0f, 0.3f, 0.72f, 0.94f, 1.0f);
        }
        Shake(0.2f, 8.0f);
        gate.timer = -1.0f;   // done
    }
    g_SummonGates.erase(std::remove_if(g_SummonGates.begin(), g_SummonGates.end(),
        [](const SummonGate& g) { return g.timer < 0.0f; }), g_SummonGates.end());
}

// Once the ram's aim locks, red ghost brackets of the pane march ahead of
// it along the flight: where it will pass, as wide as it is.
static void DrawRamLane(const os_pane::Pane& p) {
    const PaneRam& ram = g_PaneRam;
    if (!PaneRamBusy(p.id) || ram.stage != 0) return;
    const Monster* boss = FindNorma();
    const float aim = norma::RamAimFor(boss ? boss->normaPhase : 1);
    const float k = std::clamp(ram.timer / aim, 0.0f, 1.0f);
    if (k < norma::kRamLock) return;
    const float show = (k - norma::kRamLock) / (1.0f - norma::kRamLock);
    const hud_lines::Rgb red{ 1.0f, 0.30f, 0.26f };
    // The pane's length along the flight; ghosts march ahead by that much.
    const float len = std::fabs(ram.dirX) * p.shown.w + std::fabs(ram.dirY) * p.shown.h;
    const float march = std::fmod(g_GameTime * 2.2f, 1.0f);
    const float arm = std::min(22.0f, std::min(p.shown.w, p.shown.h) * 0.2f);
    for (int j = 0; j < 3; ++j) {
        const float u = (float)j + march;   // 0..3 ghosts ahead
        const float fade = 1.0f - u / 3.0f;
        const float d = len * 0.75f * (u + 0.6f);
        const float gx = p.shown.x + ram.dirX * d, gy = p.shown.y + ram.dirY * d;
        const float xs[2] = { gx, gx + p.shown.w }, ys[2] = { gy, gy + p.shown.h };
        const float al = (0.35f + 0.6f * show) * fade * fade;
        for (int cx = 0; cx < 2; ++cx)
            for (int cy = 0; cy < 2; ++cy) {
                const float x = xs[cx], y = ys[cy];
                const float sx = cx == 0 ? 1.0f : -1.0f, sy = cy == 0 ? 1.0f : -1.0f;
                PaneSeg(x, y, x + sx * arm, y, 2.4f, red, al);
                PaneSeg(x, y, x, y + sy * arm, 2.4f, red, al);
            }
    }
}

// 10 Ram: draw the pane back while aiming at the player, then hurl it until
// it meets the screen edge. Its frame sweeping over the player or a mob
// hurts and shoves it; whoever was already inside when it set off is safe.
static void TickPaneRam(float dt) {
    PaneRam& ram = g_PaneRam;
    if (ram.id == 0) return;
    const os_pane::Pane* pane = PaneById(ram.id);
    if (!pane || pane->minimized) { ram = {}; return; }
    if (pane->flying && ram.stage < 2) {   // dragged away: control broken
        ram.stage = 2;
        ram.timer = 0.0f;
    }
    ram.timer += dt;
    const float sw = (float)screenWidth, sh = (float)screenHeight;
    const float psx = W2SX(g_NormaPlayerX), psy = W2SY(g_NormaPlayerY);
    const auto inside = [&](float x, float y) {
        return x >= ram.x && x <= ram.x + ram.w && y >= ram.y && y <= ram.y + ram.h;
    };
    if (ram.stage == 0) {
        const Monster* boss = FindNorma();
        const float aim = norma::RamAimFor(boss ? boss->normaPhase : 1);
        const float k = std::clamp(ram.timer / aim, 0.0f, 1.0f);
        const float ease = 1.0f - (1.0f - k) * (1.0f - k);
        // Squeezes down to a small block around its middle in the first
        // half of the aim, so its lane is narrow enough to sidestep.
        const float squeeze = std::min(1.0f, ease * 2.0f);
        ram.w = ram.fromW + (std::min(ram.fromW, norma::kRamW) - ram.fromW) * squeeze;
        ram.h = ram.fromH + (std::min(ram.fromH, norma::kRamH) - ram.fromH) * squeeze;
        const float mx = ram.fromX + ram.fromW * 0.5f, my = ram.fromY + ram.fromH * 0.5f;
        if (k < norma::kRamLock) {
            const float ddx = psx - mx, ddy = psy - my;
            const float len = std::max(1.0f, std::sqrt(ddx * ddx + ddy * ddy));
            ram.dirX = ddx / len;
            ram.dirY = ddy / len;
        }
        // Draws back away from the player, easing out.
        const float back = norma::kRamPullBack * ease;
        ram.x = std::clamp(mx - ram.w * 0.5f - ram.dirX * back, 0.0f, std::max(0.0f, sw - ram.w));
        ram.y = std::clamp(my - ram.h * 0.5f - ram.dirY * back, 0.0f, std::max(0.0f, sh - ram.h));
        os_pane::Resize(ram.id, std::round(ram.x), std::round(ram.y), std::round(ram.w), std::round(ram.h));
        if (k >= 1.0f) {
            ram.stage = 1;
            ram.timer = 0.0f;
            ram.playerInside = inside(psx, psy);
        }
        return;
    }
    if (ram.stage == 1) {
        const float speed = std::min(norma::kRamMaxSpeed,
                                     norma::kRamStartSpeed * std::exp(ram.timer / norma::kRamRamp));
        ram.x += ram.dirX * speed * dt;
        ram.y += ram.dirY * speed * dt;
        bool hitEdge = false;
        if (ram.x < 0.0f) { ram.x = 0.0f; hitEdge = true; }
        if (ram.y < 0.0f) { ram.y = 0.0f; hitEdge = true; }
        if (ram.x + ram.w > sw) { ram.x = std::max(0.0f, sw - ram.w); hitEdge = true; }
        if (ram.y + ram.h > sh) { ram.y = std::max(0.0f, sh - ram.h); hitEdge = true; }
        os_pane::Place(ram.id, std::round(ram.x), std::round(ram.y));
        // The frame sweeping over the player.
        const bool now = inside(psx, psy);
        if (now && !ram.playerInside && !ram.hitPlayer) {
            ram.hitPlayer = true;
            {
                space::HurtSourceScope source(space::kUntracked);
                HurtPlayer(g_GameManager.playerHP, norma::kRamDamage);
            }
            ApplyPlayerKnockback(ram.dirX * norma::kRamKnock, ram.dirY * norma::kRamKnock);
            SpawnSparks(g_NormaPlayerX, g_NormaPlayerY, 14, 1.0f, 0.40f, 0.34f, 380.0f);
            Shake(0.25f, 12.0f);
            TriggerHitStop(0.05f);
        }
        ram.playerInside = now;
        // ...and over mobs: they are crushed once each.
        const auto crush = [&](auto& m) {
            const void* key = &m;
            if (std::find(ram.hitMobs.begin(), ram.hitMobs.end(), key) != ram.hitMobs.end()) return;
            if (!inside(W2SX(m.worldX), W2SY(m.worldY))) return;
            ram.hitMobs.push_back(key);
            g_MonsterManager.ApplyDamage(m, norma::kRamMobDamage, space::kUntracked);
            SpawnSparks(m.worldX, m.worldY, 6, 1.0f, 0.40f, 0.34f, 260.0f);
        };
        for (Monster* m : g_MonsterManager.monsters)
            if (m->alive && m->kind != MobKind::NORMA) crush(*m);
        for (RangedMob* r : g_MonsterManager.rangedMobs)
            if (r->alive) crush(*r);
        if (hitEdge || ram.timer > 3.0f) {
            // Impact at the leading edge.
            const float ex = ram.x + ram.w * 0.5f + ram.dirX * ram.w * 0.5f;
            const float ey = ram.y + ram.h * 0.5f + ram.dirY * ram.h * 0.5f;
            const float wx = ScreenToWorldX(ex), wy = ScreenToWorldY(ey);
            SpawnShockWave(wx, wy, 220.0f, 0.35f, 1.0f, 0.36f, 0.30f);
            SpawnSparks(wx, wy, 22, 1.0f, 0.45f, 0.38f, 460.0f);
            Shake(0.35f, 16.0f);
            if (Monster* boss = FindNorma()) boss->normaExposed = norma::kExposeAfterRam;
            ram.stage = 2;
            ram.timer = 0.0f;
        }
        return;
    }
    if (ram.timer >= norma::kRamSettle) ram = {};
}

// CloseWindow: when the count runs out, everything left in the pane is
// crushed and the pane closes.
static void TickNormaCountdown(float dt) {
    if (g_NormaCountdown.id == 0) return;
    const os_pane::Pane* pane = PaneById(g_NormaCountdown.id);
    if (!pane) { g_NormaCountdown = {}; return; }
    g_NormaCountdown.left -= dt;
    if (g_NormaCountdown.left > 0.0f) return;
    const int id = g_NormaCountdown.id;
    {
        space::HurtSourceScope source(id);
        HurtPlayer(g_GameManager.playerHP, norma::kCloseDamage);
    }
    for (Monster* m : g_MonsterManager.monsters)
        if (m->alive && m->kind != MobKind::NORMA && space::SpaceAt(m->worldX, m->worldY) == id)
            g_MonsterManager.ApplyDamage(*m, norma::kCloseMobDamage, id);
    for (RangedMob* r : g_MonsterManager.rangedMobs)
        if (r->alive && space::SpaceAt(r->worldX, r->worldY) == id)
            g_MonsterManager.ApplyDamage(*r, norma::kCloseMobDamage, id);
    const float cx = ScreenToWorldX(pane->shown.x + pane->shown.w * 0.5f);
    const float cy = ScreenToWorldY(pane->shown.y + pane->shown.h * 0.5f);
    SpawnShockWave(cx, cy, std::max(pane->shown.w, pane->shown.h) * 0.55f / g_ViewZoom, 0.45f,
                   0.86f, 0.80f, 1.0f);
    SpawnEnemyExplosion(cx, cy, 0.86f, 0.80f, 1.0f, true);
    SpawnSparks(cx, cy, 30, 0.92f, 0.99f, 1.0f, 480.0f);
    Shake(0.35f, 18.0f);
    TriggerHitStop(0.08f);
    g_PaneCollapses.push_back({ pane->shown, 0.0f });
    os_pane::SetLocked(id, false);
    os_pane::Close(id);
    g_NormaPanes.erase(std::remove(g_NormaPanes.begin(), g_NormaPanes.end(), id), g_NormaPanes.end());
    g_NormaCountdown = {};
}

static const NormaFlash* FlashFor(int id) {
    for (auto it = g_NormaFlashes.rbegin(); it != g_NormaFlashes.rend(); ++it)
        if (it->id == id) return &*it;
    return nullptr;
}
static float FlashAge(const NormaFlash& f) { return g_GameTime - f.bornAt; }
// The gate's constellation over its pane: stars light up as the head reaches
// them, the links behind it stay, and the loop closes as the load ends.
static void DrawSummonStars(const os_pane::Pane& p) {
    const SummonGate* gate = nullptr;
    for (const SummonGate& g : g_SummonGates)
        if (g.id == p.id && g.timer >= 0.0f) gate = &g;
    if (!gate || gate->u.empty()) return;
    const hud_lines::Rgb ice{ 0.72f, 0.94f, 1.0f }, white{ 0.92f, 0.99f, 1.0f };
    const hud_lines::Rgb warn{ 0.86f, 0.80f, 1.0f };
    const int n = (int)gate->u.size();
    std::vector<float> xs(n), ys(n);
    for (int k = 0; k < n; ++k) {
        xs[k] = p.shown.x + p.shown.w * gate->u[k];
        ys[k] = p.shown.y + p.shown.h * gate->v[k];
    }
    // n links make the loop; the head walks them over the load time.
    const float walk = std::clamp(gate->timer / norma::kSummonLoad, 0.0f, 1.0f) * (float)n;
    const int done = std::min(n, (int)walk);
    const float part = walk - (float)done;
    for (int k = 0; k < done; ++k) {
        const int j = (k + 1) % n;
        PaneSeg(xs[k], ys[k], xs[j], ys[j], 1.6f, warn, 0.75f);
    }
    if (done < n) {
        const int j = (done + 1) % n;
        const float hx = xs[done] + (xs[j] - xs[done]) * part;
        const float hy = ys[done] + (ys[j] - ys[done]) * part;
        PaneSeg(xs[done], ys[done], hx, hy, 2.4f, white, 0.95f);
        // the bright head: a small cross
        PaneSeg(hx - 5.0f, hy, hx + 5.0f, hy, 2.0f, white, 0.95f);
        PaneSeg(hx, hy - 5.0f, hx, hy + 5.0f, 2.0f, white, 0.95f);
    }
    for (int k = 0; k < n; ++k) {
        const bool lit = k <= done;
        const float r = lit ? 6.0f : 3.0f;
        const float w = lit ? 1.8f : 1.0f;
        const float a = lit ? 0.95f : 0.35f;
        const hud_lines::Rgb c = lit ? ice : warn;
        PaneSeg(xs[k] - r, ys[k], xs[k], ys[k] - r, w, c, a);
        PaneSeg(xs[k], ys[k] - r, xs[k] + r, ys[k], w, c, a);
        PaneSeg(xs[k] + r, ys[k], xs[k], ys[k] + r, w, c, a);
        PaneSeg(xs[k], ys[k] + r, xs[k] - r, ys[k], w, c, a);
    }
}
static bool ThrowPayloadFor(int id, int& count, float& turn) {
    if (g_ThrowBox.id != id || id == 0) return false;
    const int total = norma::kThrowRing / 2;   // drawn squares stand for the load
    count = g_ThrowBox.stage == 0
        ? std::max(1, (int)(total * std::min(1.0f, g_ThrowBox.timer / norma::kThrowFill)))
        : total;
    turn = g_GameTime * (g_ThrowBox.stage == 2 ? 9.0f : 3.0f);
    return true;
}
// The box growing out of the boss: a square leaves the body, swells and
// slides to its spot, trailing two fainter copies and lines back to the boss.
static void DrawThrowAppear() {
    if (!g_ThrowBox.active || g_ThrowBox.stage != -1) return;
    const Monster* boss = FindNorma();
    if (!boss) return;
    const float bx = W2SX(boss->worldX), by = W2SY(boss->worldY);
    const float bh = 18.0f * boss->sizeScale * 1.3f * g_ViewZoom;
    const os_pane::Rect from{ bx - bh, by - bh, bh * 2.0f, bh * 2.0f };
    const os_pane::Rect& to = g_ThrowBox.spot;
    const hud_lines::Rgb ice{ 0.72f, 0.94f, 1.0f };
    const auto at = [&](float t) {
        const float e = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
        return os_pane::Rect{ from.x + (to.x - from.x) * e, from.y + (to.y - from.y) * e,
                              from.w + (to.w - from.w) * e, from.h + (to.h - from.h) * e };
    };
    const float t = std::clamp(g_ThrowBox.timer / norma::kThrowAppear, 0.0f, 1.0f);
    BindMainShader();
    for (int g = 2; g >= 1; --g) {
        const float tg = std::max(0.0f, t - 0.12f * (float)g);
        DrawPaneOutline(at(tg), ice, 1.2f, 0.25f / (float)g);
    }
    const os_pane::Rect r = at(t);
    DrawPaneOutline(r, ice, 2.4f, 0.95f);
    const float cxs[2] = { r.x, r.x + r.w }, cys[2] = { r.y, r.y + r.h };
    for (float cx : cxs)
        for (float cy : cys)
            hud_lines::Segment(bx, by, cx, cy, 1.0f, ice, 0.35f * (1.0f - t));
    BatchFlush();
}

static bool CountdownFor(int id, float& left) {
    if (g_NormaCountdown.id != id || id == 0) return false;
    left = g_NormaCountdown.left;
    return true;
}

static void CollapsePane(const os_pane::Pane& pane) {
    g_PaneCollapses.push_back({ pane.shown, 0.0f });
}

static void DrawPaneCollapses(float dt) {
    if (g_PaneCollapses.empty()) return;
    BindMainShader();
    for (PaneCollapse& c : g_PaneCollapses) {
        c.age += dt;
        const float t = std::clamp(c.age / kPaneCollapseSeconds, 0.0f, 1.0f);
        const float k = 1.0f - t * t;
        const float cx = c.rect.x + c.rect.w * 0.5f, cy = c.rect.y + c.rect.h * 0.5f;
        const os_pane::Rect r{ cx - c.rect.w * 0.5f * k, cy - c.rect.h * 0.5f * k,
                               c.rect.w * k, c.rect.h * k };
        DrawPaneOutline(r, { 0.92f, 0.99f, 1.0f }, 2.5f, 1.0f - t);
        hud_lines::Segment(r.x, r.y, cx, cy, 1.2f, { 0.72f, 0.94f, 1.0f }, 0.6f * (1.0f - t));
        hud_lines::Segment(r.x + r.w, r.y, cx, cy, 1.2f, { 0.72f, 0.94f, 1.0f }, 0.6f * (1.0f - t));
        hud_lines::Segment(r.x, r.y + r.h, cx, cy, 1.2f, { 0.72f, 0.94f, 1.0f }, 0.6f * (1.0f - t));
        hud_lines::Segment(r.x + r.w, r.y + r.h, cx, cy, 1.2f, { 0.72f, 0.94f, 1.0f }, 0.6f * (1.0f - t));
    }
    g_PaneCollapses.erase(std::remove_if(g_PaneCollapses.begin(), g_PaneCollapses.end(),
        [](const PaneCollapse& c) { return c.age >= kPaneCollapseSeconds; }), g_PaneCollapses.end());
    BatchFlush();
}

// Worn-out panes counting down to their blast.
struct PaneFuse { int id = 0; float timer = 0.0f; };
static std::vector<PaneFuse> g_PaneFuses;

static const PaneFuse* FuseOf(int id) {
    for (const PaneFuse& f : g_PaneFuses) if (f.id == id) return &f;
    return nullptr;
}

// The blast range around a pane, in screen pixels.
static os_pane::Rect BlastRange(const os_pane::Pane& p) {
    const float m = space::kBlastMargin * g_ViewZoom;
    return { p.shown.x - m, p.shown.y - m, p.shown.w + m * 2.0f, p.shown.h + m * 2.0f };
}

static void ApplyPaneAbsorbs() {
    for (const space::Absorb& a : space::g_Absorbed) {
        SpawnSparks(a.x, a.y, 3, 0.76f, 0.70f, 1.0f, 160.0f);
        if (a.paneId > 0) WearOf(a.paneId).hits += 1.0f;
    }
    space::g_Absorbed.clear();
    for (const os_pane::Pane& p : os_pane::Panes()) {
        if (!p.open) continue;
        const float left = PaneDurabilityLeft(p);
        // Worn out: light the fuse once. Low: the contents start to waver.
        if (left <= 0.0f && !FuseOf(p.id)) {
            g_PaneFuses.push_back({ p.id, 0.0f });
            // A lit fuse cannot be put out: closing or minimizing the pane
            // is refused ("not responding") until it blows.
            os_pane::SetLocked(p.id, true);
            SpawnSparks(ScreenToWorldX(p.shown.x + p.shown.w * 0.5f),
                        ScreenToWorldY(p.shown.y + p.shown.h * 0.5f), 10, 0.86f, 0.80f, 1.0f, 300.0f);
        }
        float unsteady = 0.0f;
        if (const PaneFuse* f = FuseOf(p.id))
            unsteady = 0.6f + 0.4f * std::min(1.0f, f->timer / space::kFuseSeconds);
        else if (left < space::kUnsteadyShare)
            unsteady = 0.35f * (1.0f - left / space::kUnsteadyShare);
        os_pane::SetUnstable(p.id, unsteady);
    }
    g_PaneWear.erase(std::remove_if(g_PaneWear.begin(), g_PaneWear.end(),
        [](const PaneWear& w) {
            for (const os_pane::Pane& p : os_pane::Panes())
                if (p.id == w.id && p.open) return false;
            return true;
        }), g_PaneWear.end());
}

// A lit fuse burns for space::kFuseSeconds of play, then the pane explodes:
// everything in the blast range is hit, whatever space it stands in, and the
// pane closes. A pane closed in the meantime (in pause) never blows.
static void TickPaneFuses(float dt) {
    for (PaneFuse& fuse : g_PaneFuses) {
        const os_pane::Pane* pane = PaneById(fuse.id);
        if (!pane) { fuse.timer = -1.0f; continue; }
        fuse.timer += dt;
        if (fuse.timer < space::kFuseSeconds) continue;
        const os_pane::Rect range = BlastRange(*pane);
        const float x0 = ScreenToWorldX(range.x), y0 = ScreenToWorldY(range.y);
        const float x1 = ScreenToWorldX(range.x + range.w), y1 = ScreenToWorldY(range.y + range.h);
        const auto inside = [&](float x, float y) { return x >= x0 && x <= x1 && y >= y0 && y <= y1; };
        if (inside(g_NormaPlayerX, g_NormaPlayerY)) {
            space::HurtSourceScope blast(space::kUntracked);
            HurtPlayer(g_GameManager.playerHP, space::kBlastPlayerDamage);
        }
        for (Monster* m : g_MonsterManager.monsters)
            if (m->alive && m->kind != MobKind::NORMA && inside(m->worldX, m->worldY))
                g_MonsterManager.ApplyDamage(*m, space::kBlastMobDamage, space::kUntracked);
        for (RangedMob* r : g_MonsterManager.rangedMobs)
            if (r->alive && inside(r->worldX, r->worldY))
                g_MonsterManager.ApplyDamage(*r, space::kBlastMobDamage, space::kUntracked);
        const float cx = (x0 + x1) * 0.5f, cy = (y0 + y1) * 0.5f;
        const float spin = RandRange(0.0f, 6.2831853f);
        for (int i = 0; i < space::kBlastRing; ++i)
            SpawnNormaShot(cx, cy, spin + 6.2831853f * (float)i / space::kBlastRing,
                           space::kBlastRingSpeed);
        for (int i = 0; i < space::kBlastInner; ++i)
            SpawnNormaShot(cx, cy, spin + 6.2831853f * ((float)i + 0.5f) / space::kBlastInner,
                           space::kBlastInnerSpeed);
        SpawnShockWave(cx, cy, std::max(x1 - x0, y1 - y0) * 0.6f, 0.5f, 0.86f, 0.80f, 1.0f);
        SpawnShockWave(cx, cy, std::max(x1 - x0, y1 - y0) * 0.35f, 0.35f, 1.0f, 1.0f, 1.0f);
        SpawnEnemyExplosion(cx, cy, 0.86f, 0.80f, 1.0f, true);
        SpawnSparks(cx, cy, 36, 0.92f, 0.99f, 1.0f, 520.0f);
        Shake(0.4f, 20.0f);
        TriggerHitStop(0.06f);
        g_PaneCollapses.push_back({ pane->shown, 0.0f });
        os_pane::Close(fuse.id);
        fuse.timer = -1.0f;
    }
    g_PaneFuses.erase(std::remove_if(g_PaneFuses.begin(), g_PaneFuses.end(),
        [](const PaneFuse& f) { return f.timer < 0.0f; }), g_PaneFuses.end());
}

static bool FuseFor(int id, float& share, os_pane::Rect& range) {
    const PaneFuse* fuse = FuseOf(id);
    const os_pane::Pane* pane = PaneById(id);
    if (!fuse || !pane) return false;
    share = std::clamp(fuse->timer / space::kFuseSeconds, 0.0f, 1.0f);
    range = BlastRange(*pane);
    return true;
}
// Static Field's weapon pulse is gameplay state, not just a player-shell
// visual.  Keeping its timer outside the render path makes the area weapon
// behave consistently at every frame rate and during fixed-step simulation.
float g_StaticFieldPulseTimer = 0.0f;
inline void TriggerMuzzle(float x, float y, float ang) {
    g_MuzzleX = x; g_MuzzleY = y; g_MuzzleAng = ang; g_MuzzleTimer = 0.05f;
}

// ?�면 ?�들�?(보스 ?�폰, 충격??
float g_ShakeTime = 0.0f;
float g_ShakeMag  = 0.0f;

// 보스 보상 ???��? 버프 ????(?�버???�이지 skip)
// ?�?�?배드 ?�터 ?�망 ?�류�????�시 감속 구역(?�상 ?�역). ?�에 ?�으�??�동?�도 -10% ?�?�?
//   즉시 ?�기지 ?�고 ZONE_OPEN(0.7�???걸쳐 ?�점 부?�되???�짐(grow factor = age/OPEN).
struct LaserBeam { float ox, oy, ex, ey, life, maxLife; float width = 1.0f; };
std::vector<LaserBeam> g_LaserBeams;
float          g_LaserTimer = 0.0f;
constexpr float LASER_INT   = 0.85f;  // 발사 주기(�? ???�프: 0.7


// ?�?�?백신 ?�캔 (증강) ??주기?�으�??�레?�어 주�????�화 ?�스(범위 ?�소) ?�?�?
//   ?�각 링�? 기존 SpawnShockWave(?�창 �? ?�사??

// ?�?�?보스 ?�장 ?�조(증상) ?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?
//   보스 ?�폰??"결정 ??2.5�??�조(?�마 증상 + 경고 배너) ???�제 ?�성" ?�로 분리.
//   ?�조 ?�안 게임?�레?�는 계속(?�레그래??. 만료 ??결정??보스�??�제�??�성.
bool  g_LastRunRecord   = false;  // 직전 ?�이 ?�기록이?�는지 (GAMEOVER ?�시??

float g_WindowSizeCur = 0.0f;

static float EffectivePlayerWinSize() {
    if (g_WindowSizeCur >= 64.0f) return g_WindowSizeCur;
    if (g_Stats.windowSize >= 64.0f) return g_Stats.windowSize;
    return std::max(400.0f * g_Scale, 64.0f);
}

static float ClampPlayerWinSize(float sz) {
    if (sz != sz || sz < 64.0f) sz = EffectivePlayerWinSize();
    return sz;
}

float g_WinPrevHP     = -1.0f;    // Previous HP used to detect damage.
float g_HurtVignette  = 0.0f;     // Remaining hit vignette time.
float g_HpBarPop      = 0.0f;     // Remaining time to show the circular HP ring.

// Level-up signal: two thin rings in the XP colour expand from the
// player while LevelUpTransition runs (lines only, no fill or veil).
static void DrawLevelUpRings(float cx, float cy, float playerSize,
                             float progress) {
    if (progress <= 0.0f) return;
    constexpr float kTwoPi = 6.2831853f;
    for (int ring = 0; ring < 2; ++ring) {
        const float delay = ring * 0.18f;
        const float t = std::clamp((progress - delay) / (1.0f - delay), 0.0f, 1.0f);
        if (t <= 0.0f) continue;
        const float radius = playerSize * (1.2f + (ring == 0 ? 5.2f : 3.4f) * EaseOutCubic(t));
        const float alpha = (ring == 0 ? 0.85f : 0.60f) * (1.0f - t);
        DrawSettingsOrbitArc(cx, cy, radius, radius, 0.0f, kTwoPi,
                             0.30f, 0.82f, 1.0f,
                             ring == 0 ? 2.2f : 1.4f, alpha);
    }
}

static void DrawEdgeVignette(float width, float intensity,
                             float r, float g, float b) {
    if (width <= 0.0f || intensity <= 0.0f) return;
    constexpr int kBands = 10;
    const float bandWidth = width / kBands;
    const float sw = (float)screenWidth, sh = (float)screenHeight;
    BindMainShader();
    for (int band = 0; band < kBands; ++band) {
        const float inset = band * bandWidth;
        const float alpha = intensity * (1.0f -
            ((float)band + 0.5f) / kBands);
        drawRect(0.0f, inset, sw, bandWidth, r, g, b, alpha);
        drawRect(0.0f, sh - inset - bandWidth,
                 sw, bandWidth, r, g, b, alpha);
        const float sideY = inset + bandWidth;
        const float sideH = std::max(0.0f, sh - 2.0f * sideY);
        drawRect(inset, sideY, bandWidth, sideH, r, g, b, alpha);
        drawRect(sw - inset - bandWidth, sideY,
                 bandWidth, sideH, r, g, b, alpha);
    }
}

static void DrawGameplayCrosshair(float x, float y) {
    // A hollow cyan reticle. Each stroke carries only a thin keyline in a
    // dark shade of the same cyan, so it reads on bright wallpapers without
    // the black disc that used to sit behind the whole reticle.
    const UiColor3 accent = UiCol::ACCENT_CYAN;
    const float kr = accent.r * 0.22f, kg = accent.g * 0.22f, kb = accent.b * 0.22f;
    const float ka = 0.55f * CombatKeylineStrength() + 0.15f;
    auto ring = [&](float rOuter, float rInner, float r, float g, float b, float a) {
        constexpr int kSegs = 40;
        for (int i = 0; i < kSegs; ++i) {
            const float a0 = 6.2831853f * (float)i / kSegs;
            const float a1 = 6.2831853f * (float)(i + 1) / kSegs;
            const float c0 = cosf(a0), s0 = sinf(a0), c1 = cosf(a1), s1 = sinf(a1);
            BatchTri(x + c0 * rOuter, y + s0 * rOuter, x + c0 * rInner, y + s0 * rInner,
                     x + c1 * rInner, y + s1 * rInner, r, g, b, a);
            BatchTri(x + c0 * rOuter, y + s0 * rOuter, x + c1 * rInner, y + s1 * rInner,
                     x + c1 * rOuter, y + s1 * rOuter, r, g, b, a);
        }
    };
    auto tick = [&](float rx, float ry, float w, float h) {
        drawRect(rx - 0.8f, ry - 0.8f, w + 1.6f, h + 1.6f, kr, kg, kb, ka);
        drawRect(rx, ry, w, h, accent.r, accent.g, accent.b, 0.96f);
    };
    ring(9.3f, 6.0f, kr, kg, kb, ka);
    ring(8.5f, 6.8f, accent.r, accent.g, accent.b, 0.92f);
    drawCircle(x, y, 2.6f, kr, kg, kb, ka);
    drawCircle(x, y, 1.8f, 0.96f, 0.99f, 1.0f, 1.0f);
    tick(x - 15.0f, y - 1.0f, 6.0f, 2.0f);
    tick(x + 9.0f, y - 1.0f, 6.0f, 2.0f);
    tick(x - 1.0f, y - 15.0f, 2.0f, 6.0f);
    tick(x - 1.0f, y + 9.0f, 2.0f, 6.0f);
}

void SyncPlayerBoundsSize(SpatialBounds& pw, float delta, bool animate) {
    if (g_Stats.windowSize < 64.0f)
        g_Stats.windowSize = std::max(400.0f * g_Scale, 64.0f);
    float targetWin = g_Stats.windowSize *
        ((g_PlayerRuntime.hyperFocusTimer > 0.0f) ? 1.5f : 1.0f);
    if (g_WindowSizeCur < 64.0f) g_WindowSizeCur = g_Stats.windowSize;
    if (animate) {
        float winStep = std::min(1.0f, delta * 5.5f);
        g_WindowSizeCur += (targetWin - g_WindowSizeCur) * winStep;
    } else {
        g_WindowSizeCur = targetWin;
    }
    float pwSz = ClampPlayerWinSize(std::max(g_WindowSizeCur, 64.0f));
    float cx = pw.x + pw.width * 0.5f;
    float cy = pw.y + pw.height * 0.5f;
    if (cx != cx || cy != cy || pw.width < 64.0f || pw.height < 64.0f ||
        pw.width != pw.width || pw.height != pw.height) {
        cx = (float)screenWidth * 0.5f;
        cy = (float)screenHeight * 0.5f;
    }
    pw.width = pw.height = pwSz;
    pw.x = cx - pwSz * 0.5f;
    pw.y = cy - pwSz * 0.5f;
}

static void EnsurePlayerBounds(SpatialBounds& pw) {
    SyncPlayerBoundsSize(pw, 1.0f, false);
}

#if defined(_DEBUG)
static constexpr bool kCompileDebugBuild = true;
#else
static constexpr bool kCompileDebugBuild = false;
#endif

// ?�?�??�티�??�킬 ?�스?????�??기본) + ?�롯 3�?증강 ?�득, �?차면 교체) ?�?�?
// HUD: ?�재 측정 FPS (?�단 ?�측 ?�시)
int    g_CurrentFPS  = 0;
double g_FpsLastTime = 0.0;
int    g_FpsFrames   = 0;

// ���� ���� �ε��� ���?(���� �������? �ߺ� ���� ����)
std::vector<int> g_OwnedAugs;

// ũ������Ƽ�� ���?���� ���� ���� ���� (�ε���). ���� ���� �� �ϰ� ����.
bool g_CreativeStartPending = false;   // ?�용 ?��?(main 루프가 applyByIdx �?처리)
std::deque<int> g_AugEffectQueue;
bool g_AugEffectProcessing = false;

// 증강 ?�택 hover state (-1 = 미선?? 0/1/2 = 카드 ?�덱??
int  g_HoveredAug    = -1;

// 마우???�릭 edge 감�? (?�전 ?�레??left button ?�태)
bool g_LmbPrev = false;
bool g_RmbPrev = false;

// PAUSED ?�태 ??보유 증강 ?�릭 ???�명 ?�시 (-1 = ?�음, 0..AUG_TOTAL-1 = ?�덱??
int g_PauseSelectedAug = -1;
bool g_ShowPauseInventory = false;

// ?�정 ?�면 진입 ???�전 ?�태 (?�로 가�???복�?)
GameState g_SettingsReturnTo = GameState::MAIN_MENU;

int g_CurrentWeapon = -1;

// 변??카드 ??AUG_SELECT ??25% ?�률�?4번째 카드 ?�장
// ?�재 무기�??�른 StartWeapon ?�로 ?�환 (기존 무기 ?�과 ?�거 ????무기 ?�용)
// �?= ?�환??StartWeapon ?�덱?? -1 = ?�번 ?�운?�는 변??카드 ?�음
int g_ConversionWeapon = -1;

// DYING ?�망 ?�출 ?�태
float g_DyingTimer    = 0.0f;
bool  g_DeathBoomDone = false;
float g_DeathWinW0    = 0.0f;
float g_GameOverFade  = 0.0f;    // 게임?�버 메뉴 ?�이?�인 (0??, ?�?�크�???2.5�?
// ?�망 ?�출 = 즉시 ?�??��(?�티?? ????�� ?�파 ??GAMEOVER (�??�네마틱 ?�음, ?�순)
static const float DYING_DUR      = 0.8f;   // ??�� ?�파가 ?�생?�는 ?�간 (?�이???�까지)
static const float GAMEOVER_FADE  = 2.5f;   // �޴� 100%���� �ɸ��� �ð�

struct DeathParticle {
    float x, y, vx, vy;
    float size;       // ??변 길이(px)
    float r, g, b;    // ?�상
    bool  active = false;
};
static const int MAX_DEBRIS = 48;
DeathParticle g_Debris[MAX_DEBRIS] = {};
float g_DeathCX = 0, g_DeathCY = 0;
float g_DeathFlash = 0.0f; // ??�� ?�광 (1.0 ??0.0)
wchar_t g_DeathReason[96] = {0};   // ?�망 ?�인 ("?�○ ???�해 종료??)

static SpatialBounds* s_PlayerBoundsRef = nullptr;

static void InitChakramsFromStats() {
    if (!g_Stats.chakram || g_Stats.chakramCount <= 0) return;
    for (int c = 0; c < g_Stats.chakramCount && c < MAX_CHAKRAMS; c++) {
        g_Chakrams[c].maxHp = 150.0f;
        if (!g_Chakrams[c].alive && g_Chakrams[c].respawnTimer <= 0) {
            g_Chakrams[c].alive        = true;
            g_Chakrams[c].hp           = 150.0f;
            g_Chakrams[c].respawnTimer = 0.0f;
        }
        g_Chakrams[c].angle =
            (float)c / (float)g_Stats.chakramCount * 6.2831853f;
    }
}

void ApplyAugmentSideEffects(AugType atype, int scrW, int scrH) {
    float oldWS  = s_PlayerBoundsRef ? s_PlayerBoundsRef->width : g_Stats.windowSize;
    float oldPCX = s_PlayerBoundsRef
        ? s_PlayerBoundsRef->x + s_PlayerBoundsRef->width  * 0.5f
        : (float)scrW * 0.5f;
    float oldPCY = s_PlayerBoundsRef
        ? s_PlayerBoundsRef->y + s_PlayerBoundsRef->height * 0.5f
        : (float)scrH * 0.5f;

    if (atype == AugType::CHAKRAM || atype == AugType::CHAKRAM_2 ||
        atype == AugType::CHAKRAM_3 || atype == AugType::CHAKRAM_SINGULARITY) {
        InitChakramsFromStats();
    }
    if (atype == AugType::D_APPROACH && g_ApproachOrbs.empty()) {
        ApproachOrb orb;
        int edge = rand() % 4;
        if      (edge == 0) { orb.x = (float)(rand() % scrW);  orb.y = -40.0f; }
        else if (edge == 1) { orb.x = (float)(rand() % scrW);  orb.y = scrH + 40.0f; }
        else if (edge == 2) { orb.x = -40.0f;                  orb.y = (float)(rand() % scrH); }
        else                { orb.x = (float)scrW + 40.0f;     orb.y = (float)(rand() % scrH); }
        g_ApproachOrbs.push_back(orb);
    }
    if (g_Stats.windowSize != oldWS && s_PlayerBoundsRef) {
        s_PlayerBoundsRef->width  = g_Stats.windowSize;
        s_PlayerBoundsRef->height = g_Stats.windowSize;
        s_PlayerBoundsRef->x = oldPCX - g_Stats.windowSize * 0.5f;
        s_PlayerBoundsRef->y = oldPCY - g_Stats.windowSize * 0.5f;
        g_WindowSizeCur = g_Stats.windowSize;
    }
}

static void RebuildPlayerStatsFromOwned(int scrW, int scrH, bool preserveRuntime = true) {
    int weapon = g_CurrentWeapon;
    std::vector<int> owned = g_OwnedAugs;
    // Stats first (RunStats.h), then the runtime objects. The loop below
    // reads g_Stats only through ApplyAugmentSideEffects (window size,
    // chakram count), which ends in the same state with the final stats.
    const PlayerStats before = g_Stats;
    const PlayerRuntimeState runtimeBefore = g_PlayerRuntime;
    const SkillSlot skillsBefore[3] = { g_Skills[0], g_Skills[1], g_Skills[2] };
    const float shieldBefore = g_PlayerShield;
    const float shieldTimerBefore = g_PlayerShieldTimer;
    g_Stats = RebuildStats(weapon, owned, g_Scale, preserveRuntime ? &before : nullptr);

    memset(g_TypeOwned, 0, sizeof(g_TypeOwned));
    // takenOnce is a per-run acquisition ledger.  Rebuilding derived stats
    // must not reopen cards that were already selected earlier in this run.

    g_OwnedAugs.clear();
    for (int d = 0; d < MAX_DRONES;   d++) g_Drones[d]   = DroneState{};
    for (int c = 0; c < MAX_CHAKRAMS; c++) g_Chakrams[c] = ChakramState{};
    g_LaserBeams.clear();
    g_ApproachOrbs.clear();
    ResetPlayerSkills();

    for (int idx : owned) {
        if (idx < 0 || idx >= AUG_TOTAL) continue;
        AugType atype = ALL_AUGS[idx].type;
        g_OwnedAugs.push_back(idx);
        g_TypeOwned[(int)atype] = true;
        EquipSkill(SkillForAug(atype));
        if (AugOnceOnly(atype, ALL_AUGS[idx].rarity))
            g_GameManager.takenOnce[idx] = true;
        ApplyAugmentSideEffects(atype, scrW, scrH);
    }
    if (g_Stats.chakram) InitChakramsFromStats();
    ReequipSkillsFromOwned(g_OwnedAugs.data(), (int)g_OwnedAugs.size());
    if (preserveRuntime) {
        g_PlayerRuntime = runtimeBefore;
        g_PlayerShield = shieldBefore;
        g_PlayerShieldTimer = shieldTimerBefore;
        bool keepTimeStop = false, keepHyperFocus = false;
        for (SkillSlot& skill : g_Skills) {
            if (skill.type == SkillType::TIME_STOP) keepTimeStop = true;
            if (skill.type == SkillType::HYPER_FOCUS) keepHyperFocus = true;
            for (const SkillSlot& old : skillsBefore)
                if (skill.type != SkillType::NONE && skill.type == old.type)
                    skill.cd = old.cd;
        }
        if (!keepTimeStop) g_PlayerRuntime.timeStopTimer = 0.0f;
        if (!keepHyperFocus) g_PlayerRuntime.hyperFocusTimer = 0.0f;
        if (!g_Stats.dashUpgrade) g_PlayerRuntime.dashBoostShotsLeft = 0;
    }

    g_GameManager.maxHP = g_Stats.maxHP;
    if (g_GameManager.playerHP > g_Stats.maxHP)
        g_GameManager.playerHP = g_Stats.maxHP;
}

static void ApplySingleAugIdx(int idx, int scrW, int scrH) {
    if (idx < 0 || idx >= AUG_TOTAL) return;
    AugType atype = ALL_AUGS[idx].type;
    const PlayerStats statsBefore = g_Stats;
    ApplyAugStats(g_Stats, idx);
    g_OwnedAugs.push_back(idx);
    g_TypeOwned[(int)atype] = true;
    MarkAugSeen(idx);
    EquipSkill(SkillForAug(atype));
    if (AugOnceOnly(atype, ALL_AUGS[idx].rarity))
        g_GameManager.takenOnce[idx] = true;
    if (g_GameManager.playerHP > g_Stats.maxHP)
        g_GameManager.playerHP = g_Stats.maxHP;
    ApplyAugmentSideEffects(atype, scrW, scrH);
    g_GameplayTelemetry.RecordAugment(g_GameTime,
                                      g_GameManager.playerLevel,
                                      idx,
                                      statsBefore,
                                      g_Stats);
}

static void ApplyWaveDebuff(int scrW, int scrH) {
    // The wave HUD lists this wave's debuff until the wave ends, so a wave
    // that draws none must not keep showing the previous one.
    g_GameManager.lastWaveDebuff = -1;
    // The preview rolled this wave's debuff ahead of time; use it so what was
    // announced is what arrives.
    int idx = g_GameManager.nextWaveDebuff;
    g_GameManager.nextWaveDebuff = -1;
    if (idx < 0 && g_GameManager.PickRandomDebuffIndices(&idx, 1) != 1) return;
    ApplySingleAugIdx(idx, scrW, scrH);
    g_GameManager.maxHP = g_Stats.maxHP;
    g_GameManager.lastWaveDebuff = idx;
    g_GameManager.waveDebuffToast = 4.0f;
    const int enteredWave = g_GameManager.currentWave;
    if (enteredWave >= 0 && enteredWave < 64)
        g_GameManager.waveDebuffLog[enteredWave] = idx + 1;
}

// Ends the run as a win. reason holds the KR/EN/JP line shown with the
// final wave number.
static void FinishRunVictory(int finalWave, const wchar_t* const (&reason)[3],
                             const char* telemetryTag) {
    if (g_GameManager.waveVictory || g_GameManager.currentState != GameState::RUNNING)
        return;
    g_GameManager.waveVictory = true;
    g_GameplayTelemetry.EndRun(g_GameTime, g_GameManager.playerLevel,
                               g_GameManager.xp, g_Stats, telemetryTag);
    const int li = std::max(0, std::min(2, LangIndex()));
    swprintf_s(g_DeathReason, L"%ls %d", reason[li], finalWave);
    Audio::StopBgm();
    g_GameOverFade = 0.0f;
    g_LastRunRecord = CommitRunResult(
        !PlayRecordsProgress(), static_cast<int>(g_Difficulty), g_GameManager.score,
        g_Stats.killCount, (g_SelectedJob == JOB_STATIC_FIELD ? 1 : 0),
        g_GameManager.currentWave, true, g_GameTime);
    // The battle is over: the report sits on an empty field, not on a frozen
    // snapshot of the last wave's mobs, shots and summons.
    g_MonsterManager.Clear();
    g_Bullets.clear();
    g_ApproachOrbs.clear();
    g_LaserBeams.clear();
    g_StardustPickups.clear();
    for (int d = 0; d < MAX_DRONES;   d++) g_Drones[d]   = DroneState{};
    for (int c = 0; c < MAX_CHAKRAMS; c++) g_Chakrams[c] = ChakramState{};
    for (int i = 0; i < MAX_SHOCKS; i++) g_ShockWaves[i].active = false;
    g_GameManager.currentState = GameState::GAMEOVER;
}

// Felling NORMA, the final wave's boss, wins the run at once.
static void CompleteBossRun(int finalWave) {
    static const wchar_t* const reason[3] = {
        L"NORMA 처치", L"NORMA Defeated", L"NORMA撃破"
    };
    FinishRunVictory(finalWave, reason, "boss_defeated");
}

// Time-out verdict: starts when the boss wave's clock hits zero with NORMA
// alive, and kills the player (no revive) when the countdown ends.
static void TickNormaJudgment(float dt) {
    bool bossAlive = false;
    for (const Monster* m : g_MonsterManager.monsters)
        if (m->kind == MobKind::NORMA && m->alive) { bossAlive = true; break; }
    const bool timeOut = WaveRunComplete(g_GameTime, g_Difficulty);
    if (!bossAlive || !timeOut || NormaDefeated()) {
        g_NormaJudgment = -1.0f;
        g_NormaDoom = false;
        return;
    }
    if (g_NormaJudgment < 0.0f) {
        g_NormaJudgment = kNormaJudgmentSeconds;
        g_ShakeTime = 0.5f; g_ShakeMag = 8.0f;
        return;
    }
    g_NormaJudgment -= dt;
    if (g_NormaJudgment > 0.0f) return;
    g_NormaJudgment = -1.0f;
    g_NormaDoom = true;
    g_Stats.mk2Used = true;                    // the verdict cannot be revived
    g_GameManager.playerHP = 0.0f;
    g_ShakeTime = 0.6f; g_ShakeMag = 26.0f;
    TriggerFlash(1.0f, 0.3f, 0.32f, 0.9f);
}

static void ProcessWaveTransitions(int scrW, int scrH) {
    if (g_GameManager.currentState != GameState::RUNNING) return;
    const int reachedWave = CurrentWaveNumber(g_GameTime, g_Difficulty);
    while (g_GameManager.currentWave < reachedWave) {
        g_GameManager.waveTransitionFrom = g_GameManager.currentWave;
        ++g_GameManager.currentWave;
        g_GameManager.waveTransitionTimer = 2.0f;
        ApplyWaveDebuff(scrW, scrH);
    }
    // Roll the next wave's debuff once the current wave is nearly over, so the
    // HUD can announce it and the boundary applies exactly that one.
    if (g_GameManager.nextWaveDebuff < 0 &&
        g_GameManager.currentWave < FinalWaveCount(g_Difficulty) &&
        CurrentWaveSecondsRemaining(g_GameTime, g_Difficulty) <= wave_hud::kDebuffPreviewSeconds) {
        int idx = -1;
        if (g_GameManager.PickRandomDebuffIndices(&idx, 1) == 1)
            g_GameManager.nextWaveDebuff = idx;
    }
    // No timed clear: the run is won only by felling the boss (CompleteBossRun).
    // When the boss wave's clock runs out the fight simply goes on.
}

static void BeginAugReplaceFlow(int newIdx) {
    g_GameManager.pendingAugIdx      = newIdx;
    g_GameManager.replaceChoiceCount = 0;
    for (int oi : g_OwnedAugs) {
        if (!AugIdxUsesIdentitySlot(oi)) continue;
        if (g_GameManager.replaceChoiceCount >= 32) break;
        g_GameManager.replaceChoices[g_GameManager.replaceChoiceCount++] = oi;
    }
    g_HoveredAug = -1;
    g_GameManager.augmentKeyboardFocus = false;
    g_GameManager.currentState = GameState::AUG_REPLACE;
    ++g_GameManager.augmentSelectionSerial;
}

static void ProcessAugEffectQueue(int scrW, int scrH);
static void FinishAugmentEffects();

static void PushAugEffectsFront(const int* effects, int count) {
    for (int i = count - 1; i >= 0; --i) {
        if (effects[i] >= 0 && effects[i] < AUG_TOTAL)
            g_AugEffectQueue.push_front(effects[i]);
    }
}

// Plain values the skill HUD reads each frame (SkillHudModel.h).
static skill_hud::SkillHudInputs GatherSkillHudInputs() {
    using skill_hud::SkillGlyph;
    skill_hud::SkillHudInputs in{};
    for (int i = 0; i < 3; ++i) {
        const SkillSlot& s = g_Skills[i];
        in.equipped[i] = s.type != SkillType::NONE;
        in.glyph[i] = s.type == SkillType::HYPER_FOCUS ? SkillGlyph::Focus
                    : s.type == SkillType::TIME_STOP   ? SkillGlyph::TimeStop
                                                       : SkillGlyph::Burst;
        in.cd[i] = s.cd;
        in.cdMax[i] = PlayerSkillCooldownMax(s.type);
    }
    in.dashCd = g_PlayerRuntime.dashCooldown;
    in.dashCdMax = g_PlayerRuntime.dashCooldownMax;
    in.dashActive = g_PlayerRuntime.dashActive;
    in.dashProgress = g_PlayerRuntime.dashProgress;
    in.dashUpgrade = g_Stats.dashUpgrade;
    in.hyperFocusTimer = g_PlayerRuntime.hyperFocusTimer;
    in.hyperFocusDur = HYPER_FOCUS_DUR;
    in.timeStopTimer = g_PlayerRuntime.timeStopTimer;
    in.timeStopDur = TIMESTOP_DUR;
    in.rainOwned = g_Stats.bulletRain;
    in.rainTimer = g_BulletRainTimer;
    in.rainCooldown = g_Stats.bulletRainCooldown;
    return in;
}

static void ResetAugmentDerivedRuntime() {
    g_ApproachOrbs.clear();
    for (int d = 0; d < MAX_DRONES; d++) g_Drones[d] = DroneState{};
    for (int c = 0; c < MAX_CHAKRAMS; c++) g_Chakrams[c] = ChakramState{};
    g_LaserBeams.clear();
    ResetPlayerSkills();
}

enum class LevelUpSource {
    EXPERIENCE,   // XP crossed a threshold in the fixed step
    MANUAL,       // creative/debug F key
};

// Records one level gained (playerLevel/xp already advanced) and queues a
// three-card buff selection. Wave boundaries own automatic debuffs.
static void GrantLevelUpReward(int oldLevel, XpAmount need, XpAmount oldXp,
                               LevelUpSource source) {
    const bool manual = source == LevelUpSource::MANUAL;
    g_GameplayTelemetry.RecordLevelUp(g_GameTime, oldLevel,
                                      g_GameManager.playerLevel, need,
                                      oldXp, g_GameManager.xp, manual);
    // Level-ups grant buffs only; wave boundaries own automatic debuffs.
    g_GameManager.QueueAugmentReward();

    // XP level-ups play a short full-speed signal and then open the
    // selection on their own. Extra levels gained meanwhile only queue;
    // they open back to back once the first selection finishes.
    LevelUpTransition& transition = g_GameManager.levelUpTransition;
    if (!manual && g_GameManager.currentState == GameState::RUNNING &&
        !g_GameManager.augmentRewardActive && !transition.Active())
        transition.Begin();
}

// Inputs for GameplayTime.h from the current run state.
static GameplayTimeInputs CurrentGameplayTime() {
    GameplayTimeInputs in;
    in.hitStop = g_HitStopTimer > 0.0f;
    in.dying = g_GameManager.currentState == GameState::DYING;
    return in;
}

// Hits taken while this is true are undone at the end of the fixed step.
static bool IsPlayerDamageProtected() {
    return g_PlayerRuntime.dashInvulnerability > 0.0f ||
           g_PlayerRuntime.postPickGrace > 0.0f ||
           g_GameManager.levelUpTransition.Active();
}

// Every augment flow (buff, debuff, chained rewards) returns to combat here.
static void ReturnToRunningAfterPick() {
    g_GameManager.currentState = GameState::RUNNING;
    g_PlayerRuntime.postPickGrace = POST_PICK_GRACE;
}

static void FinishCurrentAugmentReward() {
    if (g_GameManager.augmentRewardActive &&
        g_GameManager.pendingAugmentRewards > 0) {
        --g_GameManager.pendingAugmentRewards;
    }
    g_GameManager.augmentRewardActive = false;
    if (!g_GameManager.ActivateNextAugmentReward())
        ReturnToRunningAfterPick();
}

static void FinishAugmentEffects() {
    if (g_GameManager.augmentRewardActive &&
        g_GameManager.pendingAugmentRewards > 0) {
        FinishCurrentAugmentReward();
        return;
    }

    ReturnToRunningAfterPick();
}

static void ProcessAugEffectQueue(int scrW, int scrH) {
    while (!g_AugEffectQueue.empty()) {
        const int idx = g_AugEffectQueue.front();
        g_AugEffectQueue.pop_front();
        if (idx < 0 || idx >= AUG_TOTAL) continue;

        const AugType atype = ALL_AUGS[idx].type;
        MarkAugSeen(idx);

        if (atype == AugType::S_CHAOS) {
            const int prevAugs = std::min((int)g_OwnedAugs.size(), 60);
            const PlayerStats before = g_Stats;
            const float oldWindowSize = g_Stats.windowSize;

            ResetAugmentDerivedRuntime();
            g_Stats = ChaosStats(before, g_CurrentWeapon, g_Scale);

            g_OwnedAugs.clear();
            // Preserve takenOnce: this is a run-wide acquisition ledger.
            memset(g_TypeOwned, 0, sizeof(g_TypeOwned));
            g_GameManager.maxHP = g_Stats.maxHP;
            if (g_GameManager.playerHP > g_Stats.maxHP)
                g_GameManager.playerHP = g_Stats.maxHP;
            if (s_PlayerBoundsRef && oldWindowSize != g_Stats.windowSize) {
                const float cx = s_PlayerBoundsRef->x + s_PlayerBoundsRef->width * 0.5f;
                const float cy = s_PlayerBoundsRef->y + s_PlayerBoundsRef->height * 0.5f;
                s_PlayerBoundsRef->width = g_Stats.windowSize;
                s_PlayerBoundsRef->height = g_Stats.windowSize;
                s_PlayerBoundsRef->x = cx - g_Stats.windowSize * 0.5f;
                s_PlayerBoundsRef->y = cy - g_Stats.windowSize * 0.5f;
            }
            g_WindowSizeCur = g_Stats.windowSize;

            const int nDebuffs = std::max(0, (prevAugs * 2 + 2) / 5);
            const int nBuffs = prevAugs - nDebuffs;
            int buffs[64] = {}, debuffs[64] = {};
            const int gotBuffs = g_GameManager.PickRandomAugIndices(
                buffs, nBuffs, false, true, false, false);
            const int gotDebuffs = g_GameManager.PickRandomDebuffIndices(
                debuffs, nDebuffs);
            PushAugEffectsFront(debuffs, gotDebuffs);
            PushAugEffectsFront(buffs, gotBuffs);
            continue;
        }

        if (atype == AugType::S_PANDORA) {
            int buffs[3] = {}, debuffs[2] = {};
            const int gotBuffs = g_GameManager.PickRandomAugIndices(
                buffs, 3, g_Stats.sizeAugTaken,
                true, false, false);
            const int gotDebuffs = g_GameManager.PickRandomDebuffIndices(
                debuffs, 2);
            PushAugEffectsFront(debuffs, gotDebuffs);
            PushAugEffectsFront(buffs, gotBuffs);
            continue;
        }

        if (atype == AugType::RANDOM_AUG) {
            int picks[3] = {};
            const int got = g_GameManager.PickRandomAugIndices(
                picks, 3, g_Stats.sizeAugTaken,
                true, false, false);
            PushAugEffectsFront(picks, got);
            continue;
        }

        if (NeedsReplaceForAug(idx)) {
            BeginAugReplaceFlow(idx);
            return;
        }
        ApplySingleAugIdx(idx, scrW, scrH);
    }
}

static void CompleteAugReplaceFlow(int replaceSlot, int scrW, int scrH) {
    if (replaceSlot < 0 || replaceSlot >= g_GameManager.replaceChoiceCount) return;
    int oldIdx = g_GameManager.replaceChoices[replaceSlot];
    int newIdx = g_GameManager.pendingAugIdx;

    for (auto it = g_OwnedAugs.begin(); it != g_OwnedAugs.end(); ++it) {
        if (*it == oldIdx) { g_OwnedAugs.erase(it); break; }
    }
    // Keep the acquisition ledger latched.  Replacing a card removes its
    // current effect, but a one-time card must never return in this run.
    g_TypeOwned[(int)ALL_AUGS[oldIdx].type] = false;

    RebuildPlayerStatsFromOwned(scrW, scrH);
    ApplySingleAugIdx(newIdx, scrW, scrH);
    g_GameManager.maxHP = g_Stats.maxHP;
    g_GameManager.pendingAugIdx = -1;
    g_GameManager.replaceChoiceCount = 0;

    if (g_AugEffectProcessing) {
        ProcessAugEffectQueue(scrW, scrH);
        // The scene state still names the completed picker. Only a new
        // pending card means the queue opened another replacement.
        if (g_GameManager.pendingAugIdx >= 0) return;
        g_AugEffectProcessing = false;
    }
    FinishAugmentEffects();
}

static void CancelAugReplaceFlow() {
    g_GameManager.pendingAugIdx = -1;
    g_GameManager.replaceChoiceCount = 0;
    if (g_AugEffectProcessing) {
        ProcessAugEffectQueue(g_GameManager.screenW, g_GameManager.screenH);
        if (g_GameManager.pendingAugIdx >= 0) return;
        g_AugEffectProcessing = false;
    }
    FinishAugmentEffects();
}

static bool DebugEditAugment(DebugAugmentAction action, int idx) {
    const GameState state = g_GameManager.currentState;
    if (state != GameState::RUNNING && state != GameState::PAUSED &&
        state != GameState::READY) return false;
    if (action == DebugAugmentAction::OpenSelection) {
        g_GameManager.QueueAugmentReward();
        g_GameManager.ActivateNextAugmentReward();
        return true;
    }
    if (action == DebugAugmentAction::Clear) {
        g_OwnedAugs.clear();
        memset(g_GameManager.takenOnce, 0, sizeof(g_GameManager.takenOnce));
        RebuildPlayerStatsFromOwned(g_GameManager.screenW, g_GameManager.screenH);
        return true;
    }
    if (action == DebugAugmentAction::FillSlots) {
        for (int i = 0; i < AUG_TOTAL && !IdentitySlotsFull(); ++i) {
            if (!AugIdxUsesIdentitySlot(i) || AugRemoved(ALL_AUGS[i].type) ||
                ALL_AUGS[i].type == AugType::RANDOM_AUG ||
                std::find(g_OwnedAugs.begin(), g_OwnedAugs.end(), i) != g_OwnedAugs.end())
                continue;
            ApplySingleAugIdx(i, g_GameManager.screenW, g_GameManager.screenH);
        }
        g_GameManager.maxHP = g_Stats.maxHP;
        return IdentitySlotsFull();
    }
    if (idx < 0 || idx >= AUG_TOTAL || AugRemoved(ALL_AUGS[idx].type)) return false;
    const AugType type = ALL_AUGS[idx].type;
    const bool instant = type == AugType::RANDOM_AUG || type == AugType::S_CHAOS ||
                         type == AugType::S_PANDORA;
    if (action == DebugAugmentAction::Draw) {
        g_GameManager.QueueAugmentReward();
        if (!g_GameManager.ActivateNextAugmentReward()) {
            // A forced debug card remains testable even when no random
            // candidates are eligible (once-only/prerequisite restrictions).
            g_GameManager.QueueAugmentReward();
            g_GameManager.augmentRewardActive = true;
            g_GameManager.currentState = GameState::AUG_SELECT;
            ++g_GameManager.augmentSelectionSerial;
            g_AugExitT = g_RepExitT = -1.0f;
            g_AugExitSlot = -1;
            g_RepExitSlot = -3;
        }
        // Offer the selected augment in slot 1, retaining the other random
        // choices and the normal confirmation/animation/replacement path.
        for (int i = 1; i < g_GameManager.augChoiceCount; ++i)
            if (g_GameManager.augChoices[i] == idx)
                std::swap(g_GameManager.augChoices[0], g_GameManager.augChoices[i]);
        g_GameManager.augChoices[0] = idx;
        g_GameManager.augChoiceCount = std::max(1, g_GameManager.augChoiceCount);
        g_HoveredAug = -1;
        return true;
    }
    if (action == DebugAugmentAction::Grant && instant) {
        g_GameManager.QueueAugmentReward();
        g_GameManager.augmentRewardActive = true;
        g_AugEffectQueue.push_back(idx);
        g_AugEffectProcessing = true;
        ProcessAugEffectQueue(g_GameManager.screenW, g_GameManager.screenH);
        if (g_GameManager.pendingAugIdx < 0) {
            g_AugEffectProcessing = false;
            g_GameManager.maxHP = g_Stats.maxHP;
            FinishAugmentEffects();
        }
        return true;
    }
    auto first = std::find(g_OwnedAugs.begin(), g_OwnedAugs.end(), idx);
    if (action == DebugAugmentAction::Remove ||
        (action == DebugAugmentAction::Toggle && first != g_OwnedAugs.end())) {
        if (action == DebugAugmentAction::Toggle)
            g_OwnedAugs.erase(std::remove(g_OwnedAugs.begin(), g_OwnedAugs.end(), idx), g_OwnedAugs.end());
        else if (first != g_OwnedAugs.end()) g_OwnedAugs.erase(first);
        else return false;
        // Debug OFF must allow the same once-only card to be tested again.
        if (std::find(g_OwnedAugs.begin(), g_OwnedAugs.end(), idx) == g_OwnedAugs.end())
            g_GameManager.takenOnce[idx] = false;
        RebuildPlayerStatsFromOwned(g_GameManager.screenW, g_GameManager.screenH);
    } else {
        if (instant) return false; // One-shot cards use Grant and the effect queue.
        ApplySingleAugIdx(idx, g_GameManager.screenW, g_GameManager.screenH);
        g_GameManager.maxHP = g_Stats.maxHP;
    }
    return true;
}


static void BeginGameplayTelemetryIfNeeded() {
    if (!g_DebugMode || !g_BalanceTestMode) return;
    if (g_GameplayTelemetry.IsActive()) return;

    const GameState state = g_GameManager.currentState;
    const bool inRunFlow = state == GameState::READY ||
                           state == GameState::RUNNING ||
                           state == GameState::PAUSED ||
                           state == GameState::AUG_SELECT ||
                           state == GameState::DEBUFF_SELECT;
    if (!inRunFlow) return;

    // FinalizeLoadout() runs from the scene layer.
    const bool hasLoadout = g_CurrentWeapon >= 0 || !g_OwnedAugs.empty();
    if (!hasLoadout) return;

    g_GameplayTelemetry.BeginRun(
        g_GameTime,
        g_GameManager.playerLevel,
        g_GameManager.xp,
        g_Stats,
        g_CreativeMode ? "creative" : "normal");
}

static void SyncGameplayTelemetry() {
    if (!g_DebugMode || !g_BalanceTestMode) {
        if (g_GameplayTelemetry.IsActive()) {
            g_GameplayTelemetry.EndRun(
                g_GameTime,
                g_GameManager.playerLevel,
                g_GameManager.xp,
                g_Stats,
                g_DebugMode ? "balance_test_disabled" : "debug_mode_disabled");
        }
        return;
    }
    BeginGameplayTelemetryIfNeeded();
}

// Experiment (Docs/QA/Onedow도파민 시스템 변경 계획.md 3-1): a short hit stop
// when a Genesis or Quasar dies while running. Runtime toggle only (Debug
// Toolkit Stats tab), on at launch and never saved; it does not affect any
// other hit stop. Remove this block, its call and the toolkit binding to
// revert.
static bool g_EliteKillHitStop = true;
// 0.03s alone was not felt; paired with the kill distortion (a slight ring
// warp in the death recipes) it is raised to 0.06s.
static constexpr float ELITE_KILL_HIT_STOP = 0.06f;   // boss: revisit once bosses exist

static void TriggerEliteKillHitStop(Monster& m) {
    if (!g_EliteKillHitStop) return;
    if (g_GameManager.currentState != GameState::RUNNING) return;
    if (m.kind != MobKind::GENESIS && m.kind != MobKind::QUASAR) return;
    TriggerHitStop(ELITE_KILL_HIT_STOP);   // longest remaining wins; no stacking
    m.eliteKillFx = true;                  // kill distortion (EntityDraw drawMob)
}

int main() {
    CrashHandler::Install();   // 강종(E23) 추적 ??처리 ?????�외 ??로그+미니?�프
    srand((unsigned)time(NULL));

    // ?�행 ?�일 ?�더�??�업 ?�렉?�리 ?�동 (Resource/ ?��?경로 로드 보장)
    PlatformChdirToExeDir();
#if defined(_WIN32)
    // One game at a time: two copies would each save over the other's
    // progress. Smoke captures never save, so they may run alongside.
    if (SmokeEnv("ONEDOW_SMOKE").empty()) {
        CreateMutexW(nullptr, TRUE, L"Local\\ASTERIA_SingleInstance");
        if (GetLastError() == ERROR_ALREADY_EXISTS) return 0;
    }
#endif
    LoadGame();   // �����?����/���?�ҷ����� (������ �⺻�� ����)
    if (g_SmokeRun.Init()) os_pane::SetCoverCheck(false);   // ONEDOW_SMOKE 스모크 캡처 세션 설정
    if (g_SmokeCapture && SmokeEnv("ONEDOW_SMOKE_RECORDS") == "1") {
        // Smoke-only PLAY panel records (never saved): easy/hard cleared, normal in progress.
        g_BestWave[0] = 10; g_BestClearSec[0] = 19 * 60 + 42;
        g_BestWave[1] = 12; g_BestClearSec[1] = 0;
        g_BestWave[2] = 20; g_BestClearSec[2] = 41 * 60 + 7;
    }
#if !defined(_DEBUG)
    // DEBUG MODE is intentionally available in Release packages for QA.
    // Keep balance telemetry opt-in to debug builds so the QA toggle does not
    // enable release telemetry by itself.
    g_BalanceTestMode = false;
#endif
#if defined(__APPLE__)
    // ���� �� ���� 1ȸ: CRT OFF + VFX �淮 (�������� �ٽ� �� �� ����)
    if (!g_MacOptV1) {
        g_MacOptV1 = true;
        g_ShaderFx = false;
        if (g_VfxDensity == VfxDensity::FULL)
            g_VfxDensity = VfxDensity::REDUCED;
        SaveGame();
    }
#endif
    ApplyAccentTheme();   // �����?�׼�Ʈ �׸� �� g_Accent* �ݿ�

    if (!glfwInit()) return -1;
    // Sleep ?�상??1ms �?(FPS �??��??�용). Windows �??��? ?�음
    PlatformTimerBegin();

    GLFWmonitor*       monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode    = glfwGetVideoMode(monitor);
    screenWidth  = mode->width;
#ifdef _WIN32
    // Avoid an exact monitor-size surface without exposing the desktop.
    // The extra row extends below the monitor instead of leaving a clickable
    // one-pixel strip at its bottom edge.
    screenHeight = mode->height + 1;
#else
    screenHeight = mode->height;
#endif
    if (g_SmokeRun.Active() && g_SmokeRun.Width() > 0 && g_SmokeRun.Height() > 0) {
        screenWidth  = g_SmokeRun.Width();
        screenHeight = g_SmokeRun.Height();
    }

#if defined(__APPLE__)
    {
        int wx = 0, wy = 0, ww = 0, wh = 0;
        glfwGetMonitorWorkarea(monitor, &wx, &wy, &ww, &wh);
        if (ww > 0 && wh > 0) {
            screenWidth  = ww;
            screenHeight = wh;
            if (mode->height > wh)
                g_TaskbarH = mode->height - (wy + wh);
        }
    }
#endif

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,        GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    // macOS ??3.2+ Core ?�서 forward-compatible 컨텍?�트 ?�수
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    // ?�티??HiDPI) 2× 백버???�기 ???�레?�버??= �??�기(?? 1:1
    //   좌표/?�크issor/마우?��? ?��? ???�위�??�치 ??Windows ?�??�일?�게 ?�작
    //   (???�면 게임???�면 좌하??1/4 ?�만 그려지�??�릭 ?�치가 ?�긋??
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
#endif
    glfwWindowHint(GLFW_DECORATED,             GLFW_FALSE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    // GLFW_FLOATING ?�거: WS_EX_TOPMOST + ?�체?�면 ?�기 조합??DWM ?�립 모드�??�발
    // ??DWM 컴포지???�회 ???�파 ?�명??무효?? TOPMOST ?�이 ?�성 ???�동?�로 ?�정.
    glfwWindowHint(GLFW_RESIZABLE,             GLFW_FALSE);
    glfwWindowHint(GLFW_ALPHA_BITS,            8);
    // Text and thin constellation rules are rendered from glyph textures and
    // line primitives.  Request a modest MSAA surface so the same UI path
    // remains readable on integrated and discrete adapters alike; drivers
    // that cannot provide it simply fall back to the default framebuffer.
    // The game is a full-screen desktop overlay. 2x MSAA keeps thin UI and
    // circular VFX clean while avoiding the 4x full-screen sample cost.
    glfwWindowHint(GLFW_SAMPLES,                2);

    // Windows overscans below the monitor to retain a non-exact window size.
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight,
                                          "Onedow", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }

#if defined(__APPLE__)
    {
        int wx = 0, wy = 0, ww = 0, wh = 0;
        glfwGetMonitorWorkarea(monitor, &wx, &wy, &ww, &wh);
        if (ww > 0 && wh > 0)
            glfwSetWindowPos(window, wx, wy);
        else
            glfwSetWindowPos(window, 0, 0);
    }
#else
    glfwSetWindowPos(window, 0, 0);
#endif

    // ?�단 ?�업?�시�??�이 계산 ???�?�크�??�에 ???�는 ?�업?�시줄에 ?�단 UI 가
    //   가?��?지 ?�도�? (?�업 ?�역???�면보다 ?�으�?�?차이가 ?�업?�시�??�이)
#ifdef _WIN32
    // The Shell keeps the taskbar behind our active borderless window.
    g_TaskbarH = 0;
#endif

    // ?�상??기�? ?��??????��? ?�면?�서 �??�티?��? 비�? 축소?�도�?계산 ???�괄 ?�용.
    //   (?��? ?��??�라 ?��? ?�면?�수�??��??�으�?컸던 문제 ?�결 + ?�체?�으�?�?축소)
    g_Scale = (float)screenHeight / SCALE_REF_H;
    if (g_Scale > 1.0f) g_Scale = 1.0f;
    if (g_Scale < 0.5f) g_Scale = 0.5f;
    GENESIS_WIN_W *= g_Scale;
    SWARM_WIN_W    *= g_Scale;
    g_RfwW *= g_Scale; g_RfwH *= g_Scale;
    glfwMakeContextCurrent(window);
    InputRegisterCallbacks(window);
    glfwSwapInterval((g_FpsCap == 0) ? 1 : 0);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;
    GLint msaaSamples = 0;
    glGetIntegerv(GL_SAMPLES, &msaaSamples);
    if (msaaSamples > 0)
        glEnable(GL_MULTISAMPLE);

    // ��Ʈ: exe ��ġ�� ���� Resource ���?��ΰ�?�޶��� �� �־� ���� �����ϴ� ��θ�?������.
    InitializeGameFonts(screenWidth, screenHeight);

    BatchFlush(); glEnable(GL_BLEND);
    // RGB: ?��? ?�파블렌??/ Alpha: ?�레?�버???�파�??�바르게 ?�적
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                        GL_ONE,       GL_ONE_MINUS_SRC_ALPHA);

    // ?�이�??�토그램) ?�스�??�이?�라??+ 증강 ?�이�?로드
    InitIconGL();
    LoadIcons();
    InitRainMissileTex();   // ?�환 ?��? 로켓 ?�프?�이???�배�??�거+?�트 가??

    EnableWindowTransparency(window);
    os_pane::Init(window);

    // --- �?GL/?��??�트�?종합 진단 (Windows ?�용) ---
#ifdef _WIN32
    {
        HWND hWnd = glfwGetWin32Window(window);
        // GL 3.3 Core Profile: GL_ALPHA_BITS deprecated ??glGetFramebufferAttachmentParameteriv ?�용
        GLint alphaBits = 0, rBits = 0, gBits = 0, bBits = 0;
        glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_BACK_LEFT,
            GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE, &alphaBits);
        glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_BACK_LEFT,
            GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE,   &rBits);
        glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_BACK_LEFT,
            GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE, &gBits);
        glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_BACK_LEFT,
            GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE,  &bBits);
        const char* renderer = (const char*)glGetString(GL_RENDERER);
        const char* glVer    = (const char*)glGetString(GL_VERSION);
        const char* vendor   = (const char*)glGetString(GL_VENDOR);
        // AMD/ATI 감�? ??AMD ?�라?�버??GLSL 컴파?�이 ???�격?�고 ?�명 FBO 처리가
        //   NVIDIA ?�??�라, 벤더�?로그�??�겨 검?�?�면/?�명?�패 ?�인 추적???�용.
        bool isAMD = false;
        if (vendor) {
            std::string vlow = vendor;
            for (auto& ch : vlow) ch = (char)tolower((unsigned char)ch);
            isAMD = (vlow.find("amd") != std::string::npos) ||
                    (vlow.find("ati") != std::string::npos) ||
                    (vlow.find("advanced micro") != std::string::npos);
        }

        // Windows ?�명???�과 ?�정 (?��??�트�?
        DWORD enableTrans = 1;
        HKEY hKey = nullptr;
        if (RegOpenKeyExA(HKEY_CURRENT_USER,
            "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
            0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD sz = sizeof(DWORD);
            RegQueryValueExA(hKey, "EnableTransparency", nullptr, nullptr,
                             (LPBYTE)&enableTrans, &sz);
            RegCloseKey(hKey);
        }

        BOOL dwmOn = FALSE;
        DwmIsCompositionEnabled(&dwmOn);
        LONG ex    = GetWindowLong(hWnd, GWL_EXSTYLE);
        int  trans = glfwGetWindowAttrib(window, GLFW_TRANSPARENT_FRAMEBUFFER);

        DBG("=== 종합 진단 ===\n");
        DBG("  [GL] Vendor           : %s%s\n", vendor ? vendor : "NULL",
            isAMD ? "  (AMD 감�? ???�격 GLSL/?�명 FBO 경로)" : "");
        DBG("  [GL] Renderer         : %s\n", renderer ? renderer : "NULL");
        DBG("  [GL] Version          : %s\n", glVer    ? glVer    : "NULL");
        DBG("  [GL] Framebuffer bits : R=%d G=%d B=%d A=%d\n",
            rBits, gBits, bBits, alphaBits);
        DBG("       ??(Core Profile ?�확??쿼리. A=0?�면 ?�라?�버가 ?�파 채널 거�?)\n");
        DBG("\n");
        DBG("  [GLFW] transparent_framebuffer : %d [%s]\n",
            trans, trans ? "SUPPORTED" : "NOT SUPPORTED");
        DBG("  [DWM]  Composition enabled     : %s\n", dwmOn ? "YES" : "NO");
        DBG("  [Win]  GWL_EXSTYLE             : 0x%08lX\n", ex);
        DBG("  [Win]  WS_EX_LAYERED           : %s\n",
            (ex & WS_EX_LAYERED)     ? "SET"           : "NOT SET");
        DBG("  [Win]  WS_EX_TRANSPARENT       : %s\n",
            (ex & WS_EX_TRANSPARENT) ? "SET (?�릭?�과!)" : "NOT SET (?�상)");
        DBG("\n");
        DBG("  [REG]  EnableTransparency      : %lu [%s]\n",
            enableTrans,
            enableTrans ? "ON (?�상)"
                        : "OFF ???�게 문제! ?�정>개인?�정>???�명???�과 켜기");
        DBG("=================\n\n");
    }
#endif // _WIN32 (진단 블록)

    InitMainShaderPipeline(screenWidth, screenHeight);
    InitMainBatchGeometry(screenWidth, screenHeight);
    InitPlanetShader(screenWidth, screenHeight);
    InitNebulaGlowShader(screenWidth, screenHeight);

    // --- 게임 ?�브?�트 초기??---
    SpatialBounds playerWin(
        (screenWidth  - g_Stats.windowSize) * 0.5f,
        (screenHeight - g_Stats.windowSize) * 0.5f,
        g_Stats.windowSize, g_Stats.windowSize);

    const float PLAYER_SIZE = 25.0f;
    const float MOVE_SPEED  = 400.0f;

    g_GameManager.Init(screenWidth, screenHeight);

    // Runtime debug toolkit bindings. The toolkit is inert unless the
    // persisted DEBUG MODE setting is enabled, but its callbacks are wired
    // once here so the normal game loop remains the single owner of state.
    g_DebugToolkit.Configure({
        &g_Stats,
        &g_GameManager,
        &g_MonsterManager,
        &playerWin,
        &g_WindowSizeCur,
        screenWidth,
        screenHeight,
        &g_BalanceTestMode,
        [&]() {
            if (!std::isfinite(g_Stats.maxHP) || g_Stats.maxHP < 1.0f)
                g_Stats.maxHP = 1.0f;
            if (!std::isfinite(g_Stats.windowSize) || g_Stats.windowSize < 64.0f)
                g_Stats.windowSize = 64.0f;
            g_GameManager.maxHP = g_Stats.maxHP;
            if (!std::isfinite(g_GameManager.playerHP))
                g_GameManager.playerHP = g_Stats.maxHP;
            g_GameManager.playerHP = std::max(0.0f,
                                               std::min(g_Stats.maxHP,
                                                        g_GameManager.playerHP));
            g_GameManager.scoreAccum = (float)g_GameManager.score;
            const float cx = playerWin.x + playerWin.width * 0.5f;
            const float cy = playerWin.y + playerWin.height * 0.5f;
            playerWin.width  = g_Stats.windowSize;
            playerWin.height = g_Stats.windowSize;
            playerWin.x = cx - playerWin.width * 0.5f;
            playerWin.y = cy - playerWin.height * 0.5f;
            g_WindowSizeCur = g_Stats.windowSize;
            if (g_Stats.chakram) InitChakramsFromStats();
            else for (int i = 0; i < MAX_CHAKRAMS; ++i)
                g_Chakrams[i] = ChakramState{};
        },
        [&](int mobKind, int count) {
            if (mobKind < 0 || mobKind > (int)MobKind::MAGNETAR) return 0;
            int spawned = 0;
            for (int i = 0; i < count; ++i) {
                const size_t before = g_MonsterManager.monsters.size();
                g_MonsterManager.SpawnMob(screenWidth, screenHeight, 160,
                                          std::max(0.01f, g_Stats.monsterHpMult));
                if (g_MonsterManager.monsters.size() <= before) break;
                Monster* mob = g_MonsterManager.monsters.back();
                mob->MakeKind((MobKind)mobKind);
                if (mob->kind == MobKind::ROTOR)
                    mob->hp *= g_Stats.rotorHpMult;
                if (mob->kind == MobKind::REGULUS) {
                    const float playerX = playerWin.x + playerWin.width * 0.5f;
                    const float playerY = playerWin.y + playerWin.height * 0.5f;
                    const float angle = (float)i * 2.39996323f;
                    mob->worldX = playerX + std::cos(angle) * 360.0f;
                    mob->worldY = playerY + std::sin(angle) * 360.0f;
                }
                ++spawned;
            }
            return spawned;
        },
        [&](int count) {
            int spawned = 0;
            for (int i = 0; i < count; ++i)
            {
                const size_t before = g_MonsterManager.rangedMobs.size();
                g_MonsterManager.SpawnRangedMob(
                    screenWidth, screenHeight,
                    std::max(0.01f, g_Stats.rmobHpMult), 160,
                    0.0f, 0.0f, -1, -1);
                if (g_MonsterManager.rangedMobs.size() <= before) break;
                ++spawned;
            }
            return spawned;
        },
        &g_EliteKillHitStop,
        &g_PlayConstellationOrbits,
        [&]() {
            if (g_GameManager.currentState != GameState::RUNNING) return;
            g_GameTime = WaveStartTimeSeconds(
                g_GameManager.currentWave + 1, g_Difficulty);
            ProcessWaveTransitions(screenWidth, screenHeight);
        },
        &g_OwnedAugs,
        DebugEditAugment
    });

    float lastFrame        = 0.0f;
    float fireTimer        = g_Stats.fireInterval;  // ready to fire immediately
    float accumulator      = 0.0f;
    const float FIXED_DT = 1.0f / 60.0f;

    Audio::Init(g_AudioMonoOutput);   // Uses the saved stereo/mono output preference.
    g_AudioMonoOutput = Audio::IsMonoOutput();
    Audio::SetBgmVolume(g_BgmVol / 100.0f);
    Audio::SetSfxVolume(g_SfxVol / 100.0f);
    Audio::SetBgmEnabled(g_BgmEnabled && g_BgmVol > 0);
    Audio::SetSfxEnabled(g_SfxEnabled && g_SfxVol > 0);

    // ============================================================
    // 메인 루프
    // ============================================================
    while (!glfwWindowShouldClose(window)) {
        s_PlayerBoundsRef = &playerWin;
        float now   = (float)glfwGetTime();
        float delta = now - lastFrame;
        if (delta > 0.1f) delta = 0.1f;
        const float realDelta = delta;   // panes run on wall time
        if (g_DebugMode) {
            const GameState st = g_GameManager.currentState;
            if (st == GameState::READY || st == GameState::RUNNING ||
                st == GameState::PAUSED || st == GameState::AUG_SELECT ||
                st == GameState::AUG_REPLACE || st == GameState::DEBUFF_SELECT ||
                st == GameState::DYING)
                g_PlayDebugTainted = true;
        }
        delta *= g_SmokeTimeScale;   // 1 outside smoke captures
        // Arranging windows mid-fight slows the game down.
        if (os_pane::ArrangeMode() && g_GameManager.currentState == GameState::RUNNING)
            delta *= kArrangeSlow;
        lastFrame = now;

        // ?�래??추적 브레?�크????마�?�??�태�??�겨 강종(E23) ??로그�??�치 ?�정
        {
            char bc[200];
            std::snprintf(bc, sizeof(bc),
                "st=%d score=%lld lv=%d mobs=%u",
                (int)g_GameManager.currentState,
                (long long)g_GameManager.score,
                g_GameManager.playerLevel,
                (unsigned)g_MonsterManager.monsters.size());
            CrashHandler::SetBreadcrumb(bc);
        }

        // �??�커?��? ?�으�??�른 ?�으�??�환) ?�동 ?�시?��? ??        //   ?�버?�이???�커???�어??루프가 계속 ?��?�? ??막으�?게임??        //   백그?�운?�에??계속 진행??콤보 3�?창이 ?�러가 리셋?�는 버그 ??.
        const InputFocusTransition focus = InputUpdateWindowFocus(window);
        const bool inputFocusChanged = focus.lost || focus.gained;
        if (inputFocusChanged) {
            ResetPlayerInputEdges();
            g_aug1Released = g_aug2Released = g_aug3Released = true;
        }
        if (focus.lost) {
            g_GameManager.spaceReleased = true;
            g_GameManager.escReleased = true;
            // Smoke captures run unattended; a stray focus change must not
            // pause them mid-sequence.
            // Clicking another program while arranging windows is part of
            // the fight, not a reason to pause.
            if (g_GameManager.currentState == GameState::RUNNING && !g_SmokeRun.Active() &&
                !os_pane::ArrangeMode()) {
                g_GameManager.pauseResumeState = GameState::RUNNING;
                g_GameManager.currentState = GameState::PAUSED;
            }
        }
        if (focus.gained) {
            g_GameManager.spaceReleased = true;
            g_GameManager.escReleased = true;
        }
        // Away from the game (Alt-Tab), its panes hide; back, they return
        // under it. Smoke captures run unfocused, so they keep theirs.
        if (!g_SmokeRun.Active()) {
            if (focus.lost) os_pane::SetAway(true);
            if (focus.gained) os_pane::SetAway(false);
        }

        // FPS 측정 (1�??�위)
        ++g_FpsFrames;
        if (now - g_FpsLastTime >= 1.0f) {
            g_CurrentFPS  = g_FpsFrames;
            g_FpsFrames   = 0;
            g_FpsLastTime = now;
        }

        InputUpdateTextInput(window);
        glfwPollEvents();
        UpdateWindowTaskbarPolicy(window);
        {
            // Moved windows' in-game rectangles wait for the game to resume.
            const GameState hs = g_GameManager.currentState;
            os_pane::SetHold(hs != GameState::RUNNING && hs != GameState::DYING);
        }
        os_pane::Tick(realDelta);
        // Panes belong to a run: leaving it (game over, menus, a fresh start)
        // closes them all.
        {
            const GameState ps = g_GameManager.currentState;
            if (ps == GameState::GAMEOVER || ps == GameState::READY ||
                ps == GameState::MAIN_MENU || ps == GameState::CREATIVE_CONFIG ||
                ps == GameState::CODEX || ps == GameState::TUTORIAL)
                os_pane::CloseAll();
        }
        {
            const float nx = playerWin.x + playerWin.width * 0.5f;
            const float ny = playerWin.y + playerWin.height * 0.5f;
            if (g_GameManager.currentState == GameState::RUNNING && delta > 0.0f) {
                // Short smoothing: a turn shows up within about 0.15 s.
                const float k = std::min(1.0f, delta / 0.15f);
                g_NormaPlayerVX += ((nx - g_NormaPlayerX) / delta - g_NormaPlayerVX) * k;
                g_NormaPlayerVY += ((ny - g_NormaPlayerY) / delta - g_NormaPlayerVY) * k;
                // A teleport (new run, dash reset) is not a velocity.
                const float sp = std::sqrt(g_NormaPlayerVX * g_NormaPlayerVX +
                                           g_NormaPlayerVY * g_NormaPlayerVY);
                if (sp > 1500.0f) {
                    g_NormaPlayerVX *= 1500.0f / sp;
                    g_NormaPlayerVY *= 1500.0f / sp;
                }
            }
            g_NormaPlayerX = nx;
            g_NormaPlayerY = ny;
            if (Monster* boss = FindNorma()) {
                boss->normaPlayerVX = g_NormaPlayerVX;
                boss->normaPlayerVY = g_NormaPlayerVY;
            }
        }
        ServeNormaAsks(playerWin);
        PlayNormaEvents();
        if (g_GameManager.currentState == GameState::RUNNING) {
            TickNormaCountdown(delta);
            TickPaneFuses(delta);
            TickThrowBox(delta);
            TickSummonGates(delta);
            // While a charge winds up the panes hold still and none open, so
            // the boss is the one thing moving.
            const Monster* winding = FindNorma();
            const bool hold = winding && winding->NormaDashWindShare() > 0.0f;
            if (!hold) TickNormaPaneSpawn(delta, playerWin);
            TickPaneRam(delta);
            DriftNormaPanes(hold ? 0.0f : delta, W2SX(playerWin.x + playerWin.width * 0.5f),
                            W2SY(playerWin.y + playerWin.height * 0.5f));
        }
        ApplyPaneAbsorbs();
        RebuildPaneSpaces(playerWin);
        // Hand testing: ONEDOW_PANE_TEST=1 keeps one test pane open during runs.
        {
            static const bool paneHandTest = !SmokeEnv("ONEDOW_PANE_TEST").empty();
            if (paneHandTest && g_GameManager.currentState == GameState::RUNNING) {
                bool anyOpen = false;
                for (const os_pane::Pane& p : os_pane::Panes()) anyOpen = anyOpen || p.open;
                if (!anyOpen)
                    g_PaneTestId = os_pane::Open(screenWidth * 0.5f + 80.0f,
                                                 screenHeight * 0.5f - 180.0f, 520.0f, 360.0f);
            }
        }
        // Hand testing: F9 brings NORMA in now during a run.
        {
            static bool f9Down = false;
            const bool f9 = glfwGetKey(window, GLFW_KEY_F9) == GLFW_PRESS;
            if (f9 && !f9Down && g_GameManager.currentState == GameState::RUNNING)
                DebugSpawnNorma(g_MonsterManager, screenWidth,
                                g_Stats.monsterHpMult *
                                    GetDifficultyParams(g_Difficulty).enemyHpMultiplier,
                                g_Difficulty);
            f9Down = f9;
        }
        // Hand testing: F8 opens another test pane during a run.
        {
            static bool f8Down = false;
            const bool f8 = glfwGetKey(window, GLFW_KEY_F8) == GLFW_PRESS;
            const GameState ps = g_GameManager.currentState;
            if (f8 && !f8Down && (ps == GameState::RUNNING || ps == GameState::PAUSED))
                g_PaneTestId = os_pane::Open(screenWidth * 0.5f - 260.0f + 60.0f * (g_PaneTestId % 5),
                                             screenHeight * 0.5f - 180.0f + 40.0f * (g_PaneTestId % 5),
                                             520.0f, 360.0f);
            f8Down = f8;
        }
        // Window arranging is no longer part of play: the boss alone moves its
        // panes. Only smoke captures still force it on.
        os_pane::SetArrangeMode(g_PaneArrangeForce);
        if (inputFocusChanged) {
            g_GameManager.spaceReleased =
                glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE;
            g_GameManager.escReleased =
                glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_RELEASE;
            g_aug1Released = glfwGetKey(window, GLFW_KEY_1) == GLFW_RELEASE;
            g_aug2Released = glfwGetKey(window, GLFW_KEY_2) == GLFW_RELEASE;
            g_aug3Released = glfwGetKey(window, GLFW_KEY_3) == GLFW_RELEASE;
            g_PlayerRuntime.dashInputHeld =
                keys[GLFW_KEY_LEFT_SHIFT] || keys[GLFW_KEY_RIGHT_SHIFT];
            g_PlayerRuntime.skillInputHeld[0] = keys[GLFW_KEY_Q];
            g_PlayerRuntime.skillInputHeld[1] = keys[GLFW_KEY_E];
            g_PlayerRuntime.skillInputHeld[2] = keys[GLFW_KEY_R];
        }

        // �?부?�럽�?보간 (?�이�? ?�면 ?�장 ??
        //   ?�투/?�투?�버?�이(RUNNING/DYING/PAUSED/증강·?�버???�택)?�선 �??��? ??        //   ?�시?��? ??줌인?�는 �?부?�연?�럽?�는 ?�드�? �??�제???�작�???        // Restore the base view after leaving the active combat states.
        {
            GameState zs = g_GameManager.currentState;
            bool inFight = (zs == GameState::RUNNING || zs == GameState::DYING ||
                            zs == GameState::PAUSED  || zs == GameState::AUG_SELECT ||
                            zs == GameState::DEBUFF_SELECT);
            if (!inFight) { g_ViewZoom = g_ViewZoomTarget = 1.0f; }
            else {
                // ?�수 비�? 줌아?????�장???�서???�어�?(?�한 1.4�?= zoom 0.714).
                //   200�� ������ �ִ�ġ ����.
                float t = std::min(1.0f, (float)g_GameManager.score / 2000000.0f);
                g_ViewZoomTarget = 1.0f - 0.286f * t;   // 1.0 �� 0.714
            }
        }
        g_ViewZoom += (g_ViewZoomTarget - g_ViewZoom) * std::min(1.0f, delta * 4.0f);

        // ?�장 ?�레????줌아?�된 만큼 보이???�역???�어지므�??�티??배회/?�폰 ?�역???�장
        //   (?�수 줌아?�·폴리모??줌아??공통 처리)
        {
            float zb = (g_ViewZoom < 0.01f) ? 0.01f : g_ViewZoom;
            g_ArenaExX = (float)screenWidth  * 0.5f * (1.0f / zb - 1.0f);
            g_ArenaExY = (float)screenHeight * 0.5f * (1.0f / zb - 1.0f);
        }

        // 마우???�태 (mx,my = ?�면 ?��? / wmx,wmy = �?보정???�드 좌표 = 조�???
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        const bool rawLmb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        const bool rawRmb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
        if (inputFocusChanged) {
            g_LmbPrev = rawLmb;
            g_RmbPrev = rawRmb;
        }
        bool lmb = rawLmb;
        float wmx = ScreenToWorldX((float)mx);
        float wmy = ScreenToWorldY((float)my);

        // --- ?�력 처리 ---
        GameState prevState = g_GameManager.currentState;
        const bool debugInputCaptured = g_DebugToolkit.BeginInput(
            window, (float)screenWidth, (float)screenHeight, mx, my,
            lmb, g_LmbPrev);
        if (!debugInputCaptured) {
            g_GameManager.HandleInput(window);
        } else {
            // Opening the toolkit also pauses simulation and consumes the
            // pointer so gameplay and scene controls cannot leak through.
            delta = 0.0f;
            lmb = false;
            wmx = ScreenToWorldX(-10000.0f);
            wmy = ScreenToWorldY(-10000.0f);
        }

        // Keep the OS pointer out of the combat view while the custom
        // crosshair is active. Pause/settings screens use normal input, so
        // the pointer must remain available for their clickable controls.
        InputUpdateGameplayCursor(window, g_GameManager.currentState,
                                  g_ShowCrosshair, g_DebugToolkit.IsVisible());

        // ??게임 리셋 ?�다 (GAMEOVER ??READY, ?�이???�택 ?? ?�시?�기 버튼 ?�에???�출)
        auto ResetForNewGame = [&]() {
            g_GameplayTelemetry.EndRun(g_GameTime,
                                       g_GameManager.playerLevel,
                                       g_GameManager.xp,
                                       g_Stats,
                                       "reset_or_restart");
            g_Stats        = ResetRunStats(g_Scale);   // weapon/trial: FinalizeLoadout
            g_ApproachOrbs.clear();
            for (int d = 0; d < MAX_DRONES;   d++) g_Drones[d]   = DroneState{};
            for (int c = 0; c < MAX_CHAKRAMS; c++) g_Chakrams[c] = ChakramState{};
                                    g_BulletRainTimer = 0.0f;
            g_PrevHP          = g_Stats.maxHP;
            g_OwnedAugs.clear();
            memset(g_TypeOwned, 0, sizeof(g_TypeOwned));
            g_CreativeGodmode  = false;
            g_CreativeFreeGrab = false;
            g_HoveredAug       = -1;
            g_ConversionWeapon = -1;
            g_CurrentWeapon    = -1;
            g_PauseSelectedAug = -1;
            g_ShowPauseInventory = false;
            g_GameTime         = 0.0f;
            g_GameManager.ClearAugmentRewardQueue();
            g_AugEffectQueue.clear();
            g_AugEffectProcessing = false;
            for (int i = 0; i < MAX_SHOCKS; i++) g_ShockWaves[i].active = false;
            g_MuzzleTimer = 0.0f;
            g_StaticFieldPulseTimer = 0.0f;
            g_ShakeTime = 0.0f; g_ShakeMag = 0.0f;
            ResetTrials();
            g_RunStardust       = 0;
            g_LaserBeams.clear(); g_LaserTimer = 0.0f;
            g_StardustPickups.clear(); g_StardustHudPulse = 0.0f;

            ResetJuice();
            ResetPlayerSkills();
            g_SkillHudPulse.Reset();
            ResetPlayerInputEdges();
            g_PlayerRuntime.postPickGrace = 0.0f;
            g_WindowSizeCur = g_Stats.windowSize;
            g_WinPrevHP = -1.0f;
            g_PlayerDamagePulse = 0.0f;
            g_HurtVignette = 0.0f; g_HpBarPop = 0.0f;
            g_HpGhost.Reset(1.0f);
            g_ViewZoom = g_ViewZoomTarget = 1.0f;   // �??�복
            g_ZoomCX = g_ZoomCY = 0.0f;
            g_Difficulty = Difficulty::NORMAL;
            // Reset wave spawn timers and scheduled elite state for this run.
            ResetEnemySpawnState();
            g_DyingTimer      = 0.0f;
            g_DeathBoomDone   = false;
            g_GameOverFade    = 0.0f;
            g_DeathFlash      = 0.0f;
            for (int i = 0; i < MAX_DEBRIS; i++) g_Debris[i].active = false;
            fireTimer = g_Stats.fireInterval;
            playerWin.width  = g_Stats.windowSize;
            playerWin.height = g_Stats.windowSize;
            playerWin.x = (screenWidth  - g_Stats.windowSize) * 0.5f;
            playerWin.y = (screenHeight - g_Stats.windowSize) * 0.5f;
            const long long initialScore = g_CreativeMode
                ? g_CreativeStartScore : 0;
            g_GameManager.ResetRunProgress(g_MonsterManager, g_Bullets,
                                           100.0f, initialScore);
        };

        // Restart the current run without returning to the PLAY screen.
        // Keep the selected loadout and run modifiers, then rebuild the
        // combat state so the next frame starts from a clean arena.
        auto RestartCurrentRun = [&]() {
            const int savedWeapon = g_CurrentWeapon;
            const bool savedGodmode = g_CreativeGodmode;
            const Difficulty savedDifficulty = g_Difficulty;
            const std::vector<int> savedOwnedAugs = g_OwnedAugs;
            const bool savedTrialPoolReady = g_TrialPoolReady;
            int savedTrialPool[TRIAL_SLOT_COUNT] = {};
            bool savedTrialSelected[TRIAL_SLOT_COUNT] = {};
            for (int i = 0; i < TRIAL_SLOT_COUNT; ++i) {
                savedTrialPool[i] = g_TrialPool[i];
                savedTrialSelected[i] = g_TrialSelected[i];
            }

            // This clears enemies, projectiles, drops,
            // timers, run currencies, score/XP and the death presentation.
            ResetForNewGame();

            // Restore the setup that ResetForNewGame intentionally clears.
            g_CurrentWeapon = savedWeapon;
            g_Difficulty = savedDifficulty;
            g_CreativeGodmode = savedGodmode;
            g_OwnedAugs = savedOwnedAugs;
            g_TrialPoolReady = savedTrialPoolReady;
            for (int i = 0; i < TRIAL_SLOT_COUNT; ++i) {
                g_TrialPool[i] = savedTrialPool[i];
                g_TrialSelected[i] = savedTrialSelected[i];
            }

            // Re-applying the owned augment list restores the exact build,
            // including class/creative augments and their side effects.
            RebuildPlayerStatsFromOwned(screenWidth, screenHeight, false);
            g_GameManager.maxHP = g_Stats.maxHP;
            g_GameManager.playerHP = g_Stats.maxHP;

            g_CreativeStartPending = false;
            g_GameManager.conversionAug = -1;
            g_GameManager.hoveredCard = -1;
            g_GameManager.pendingAugIdx = -1;
            g_GameManager.replaceChoiceCount = 0;
            g_GameManager.augChoiceCount = 0;
            for (int i = 0; i < 3; ++i) g_GameManager.augChoices[i] = -1;
            g_AugExitT = -1.0f;
            g_AugExitSlot = -1;
            g_RepExitT = -1.0f;
            g_RepExitSlot = -3;
            g_GameManager.ClearAugmentRewardQueue();
            g_AugEffectQueue.clear();
            g_AugEffectProcessing = false;
            g_GameManager.spaceReleased = true;
            g_GameManager.escReleased = true;
            g_GameManager.pauseResumeState = GameState::RUNNING;
            g_GameManager.currentState = GameState::RUNNING;
            g_GameManager.lastState = GameState::RUNNING;
        };

        // End the current run without rebuilding it. Abandoning from pause or
        // settings should use the same report screen as a normal death while
        // preserving the score, build, level, kills, and earned stardust.
        auto AbandonCurrentRun = [&]() {
            if (g_GameManager.currentState == GameState::GAMEOVER)
                return;

            g_GameplayTelemetry.EndRun(g_GameTime,
                                       g_GameManager.playerLevel,
                                       g_GameManager.xp,
                                       g_Stats,
                                       "abandoned");

            g_MonsterManager.Clear();
            g_Bullets.clear();
                    g_LaserBeams.clear();

            g_ApproachOrbs.clear();
            g_StardustPickups.clear();
            g_MuzzleTimer = 0.0f;
            g_ShakeTime = 0.0f;
            g_ShakeMag = 0.0f;
            for (int d = 0; d < MAX_DRONES;   ++d) g_Drones[d]   = DroneState{};
            for (int c = 0; c < MAX_CHAKRAMS; ++c) g_Chakrams[c] = ChakramState{};
            for (int i = 0; i < MAX_SHOCKS; ++i) g_ShockWaves[i].active = false;
            ResetJuice();

            g_GameManager.playerHP = 0.0f;
            g_GameManager.pauseResumeState = GameState::RUNNING;
            g_DyingTimer = 0.0f;
            g_DeathBoomDone = true;
            g_DeathFlash = 0.0f;
            g_GameOverFade = 0.0f;
            Audio::StopBgm();

            const int li = std::max(0, std::min(2, LangIndex()));
            const wchar_t* abandoned[3] = {
                L"플레이 포기", L"PLAY ABANDONED", L"プレイを放棄"
            };
            wcscpy_s(g_DeathReason, abandoned[li]);

            g_LastRunRecord = CommitRunResult(!PlayRecordsProgress(), (int)g_Difficulty,
                                              g_GameManager.score, g_Stats.killCount,
                                              (g_SelectedJob == JOB_STATIC_FIELD ? 1 : 0),
                                              g_GameManager.currentWave, false, g_GameTime);

            g_GameManager.currentState = GameState::GAMEOVER;
        };

        // Scripted UI smoke capture (ONEDOW_SMOKE). Each step opens a scene
        // through the same entry points the menus use, then the frame is
        // captured once the scene has had time to settle.
        if (g_SmokeRun.Active()) {
            g_SmokeRun.Frame(window, screenWidth, screenHeight, mx, my, lmb,
                [&](const std::string& step) {
                    GameManager& gm = g_GameManager;
                    if (step == "MENU" || step == "PLAY" || step == "SHOP" ||
                        step == "CODEX" || step == "SETTINGS") {
                        const int panel = step == "PLAY" ? 0
                                        : step == "SHOP" ? 1
                                        : step == "CODEX" ? 2
                                        : step == "SETTINGS" ? 3 : -1;
                        gm.currentState = GameState::MAIN_MENU;
                        SmokeOpenMenuPanel(panel);
                    } else if (step == "RUN") {
                        RestartCurrentRun();
                    } else if (step == "DEBUG") {
                        if (gm.currentState != GameState::RUNNING &&
                            gm.currentState != GameState::READY &&
                            gm.currentState != GameState::PAUSED &&
                            gm.currentState != GameState::AUG_SELECT &&
                            gm.currentState != GameState::DEBUFF_SELECT &&
                            gm.currentState != GameState::AUG_REPLACE)
                            RestartCurrentRun();
                        g_DebugToolkit.SetVisible(true);
                    } else if (step.rfind("DEBUGSEARCH=", 0) == 0) {
                        const std::string query = step.substr(12);
                        // Smoke uses ASCII IDs; interactive input also accepts Korean IME.
                        g_DebugToolkit.InsertAugmentSearchText(std::wstring(query.begin(), query.end()));
                    } else if (step.rfind("STARDUST_NEAR", 0) == 0) {
                        // Simulate a contact-range loot drop for the smoke
                        // capture without depending on combat input timing.
                        RestartCurrentRun();
                        const float cx = playerWin.x + playerWin.width * 0.5f;
                        const float cy = playerWin.y + playerWin.height * 0.5f;
                        const size_t equals = step.find('=');
                        const XpAmount smokeXp = equals == std::string::npos
                            ? XpFromWhole(1) : XpFromReward(std::atof(step.c_str() + equals + 1));
                        SpawnStardust(cx + 42.0f, cy, 1, cx, cy, smokeXp);
                    } else if (step.rfind("PANE=", 0) == 0) {
                        // NORMA OS-pane prototype:
                        //   PANE=OPEN|OVERPLAYER|LOCK|UNLOCK|CLOSE|MIN|RESTORE|MOVE|ARRANGE|STEAL|CALM
                        //   PANE=SHOT=<name>  save the composed screen as pane_<name>.bmp
                        // Every step appends a line to Screenshots/pane_state.txt.
                        const std::string action = step.substr(5);
                        if (action == "OPEN") {
                            if (gm.currentState != GameState::RUNNING) RestartCurrentRun();
                            g_PaneDebug = true;
                            const float pw = 520.0f, ph = 360.0f;
                            g_PaneTestId = os_pane::Open(screenWidth * 0.5f + 80.0f,
                                                         screenHeight * 0.5f - ph * 0.5f, pw, ph);
                        } else if (action == "OVERPLAYER") {
                            // Title bar straight through the player at screen centre.
                            if (gm.currentState != GameState::RUNNING) RestartCurrentRun();
                            g_PaneDebug = true;
                            g_PaneTestId = os_pane::Open(screenWidth * 0.5f - 260.0f,
                                                         screenHeight * 0.5f - 14.0f, 520.0f, 360.0f);
                        } else if (action == "LOCK") {
                            os_pane::SetLocked(g_PaneTestId, true);
                        } else if (action == "UNLOCK") {
                            os_pane::SetLocked(g_PaneTestId, false);
                        } else if (action == "CLOSE") {
                            os_pane::DebugRequestClose(g_PaneTestId);
                        } else if (action == "MIN") {
                            os_pane::DebugRequestMinimize(g_PaneTestId);
                        } else if (action == "RESTORE") {
                            os_pane::DebugRestore(g_PaneTestId);
                        } else if (action == "MOVE") {
                            os_pane::DebugMoveBy(g_PaneTestId, -360.0f, 120.0f);
                        } else if (action == "ARRANGE") {
                            g_PaneArrangeForce = true;
                        } else if (action == "STEAL") {
                            os_pane::DebugActivateProbe();
                        } else if (action == "CALM") {
                            g_PaneArrangeForce = false;
                        } else if (action.rfind("PROBE=", 0) == 0) {
                            // PANE=PROBE=x,y  who receives a click at that overlay point
                            float px = 0.0f, py = 0.0f;
                            sscanf_s(action.c_str() + 6, "%f,%f", &px, &py);
                            bool overlayHit = false;
                            const unsigned long pid = os_pane::DebugProcessAt(px, py, &overlayHit);
                            std::filesystem::create_directories("Screenshots");
                            FILE* pf = nullptr;
                            if (fopen_s(&pf, "Screenshots/pane_state.txt", "a") != 0) pf = nullptr;
                            if (pf) {
                                std::fprintf(pf, "PROBE %.0f,%.0f pid=%lu overlay=%d exstyle=0x%08lX\n",
                                             px, py, pid, overlayHit ? 1 : 0,
                                             os_pane::DebugOverlayExStyle());
                                std::fclose(pf);
                            }
                        } else if (action.rfind("SHOT=", 0) == 0) {
                            const std::string name = action.substr(5);
                            const std::wstring path = L"Screenshots/pane_" +
                                std::wstring(name.begin(), name.end()) + L".bmp";
                            os_pane::DebugCaptureScreen(path.c_str());
                        }
                        DumpPaneState(action.c_str(), window);
                    } else if (step == "PAUSED") {
                        if (gm.currentState != GameState::RUNNING)
                            RestartCurrentRun();
                        gm.pauseResumeState = GameState::RUNNING;
                        gm.currentState = GameState::PAUSED;
                    } else if (step == "GAMEOVER") {
                        if (gm.currentState != GameState::RUNNING)
                            RestartCurrentRun();
                        AbandonCurrentRun();
                    } else if (step.rfind("REG04", 0) == 0) {
                        // REG-04 (scripts/test_reg04_run_reset.ps1): fill every
                        // piece of run state, run one reset path, and append a
                        // snapshot line to Screenshots/reg04_state.txt.
                        //   REG04=NEWRUN|RESTART|REBUILD|ABANDON  fill, run, dump
                        //   REG04=READY   fill, then enter READY; when the frame
                        //                 started in GAMEOVER the frame's own
                        //                 GAMEOVER->READY handling resets
                        //   REG04DUMP=<label>  dump only
                        auto augIdx = [](AugType t) {
                            for (int i = 0; i < AUG_TOTAL; ++i)
                                if (ALL_AUGS[i].type == t) return i;
                            return -1;
                        };
                        auto fill = [&]() {
                            for (int d = 0; d < MAX_DRONES; ++d) g_Drones[d] = { 1.0f + d, 0.5f };
                            for (int c = 0; c < 2; ++c) {
                                g_Chakrams[c].alive = true; g_Chakrams[c].hp = 80.0f;
                            }
                            g_LaserBeams.assign(3, LaserBeam{ 0, 0, 10, 10, 0.1f, 0.1f, 1.0f });
                            g_ApproachOrbs.assign(2, ApproachOrb{ 5.0f, 5.0f });
                            g_Skills[0] = { SkillType::CLOSE_WINDOW, 3.0f };
                            g_Skills[1] = { SkillType::HYPER_FOCUS, 7.0f };
                            g_Skills[2] = { SkillType::TIME_STOP, 11.0f };
                            g_SkillReplaceIdx = 2;
                            g_PlayerRuntime.dashCooldown = 1.5f;
                            g_PlayerRuntime.timeStopTimer = 0.8f;
                            g_PlayerRuntime.hyperFocusTimer = 0.6f;
                            g_PlayerShield = 25.0f; g_PlayerShieldTimer = 4.0f;
                            g_PlayerRuntime.dashInputHeld = true;
                            g_PlayerRuntime.skillInputHeld[0] = true;
                            g_PlayerRuntime.postPickGrace = 0.4f;
                            for (int i = 0; i < 5; ++i)
                                g_Bullets.push_back(Bullet(0.0f, 0.0f, 10.0f, 0.0f));
                            for (int i = 0; i < 4; ++i)
                                g_MonsterManager.monsters.push_back(new Monster(-5000.0f, -5000.0f, 1.0f));
                            g_MonsterManager.rangedMobs.push_back(
                                new RangedMob(-5000.0f, -5000.0f, screenWidth, screenHeight));
                            for (int i = 0; i < 6; ++i) g_StardustPickups.push_back(StardustPickup{});
                            for (int i = 0; i < 2; ++i) g_ShockWaves[i].active = true;
                            for (int i = 0; i < 3; ++i) g_Debris[i].active = true;
                            g_AugEffectQueue.assign(2, augIdx(AugType::RATE_UP));
                            const int owned[] = { augIdx(AugType::RATE_UP), augIdx(AugType::HP_UP),
                                                  augIdx(AugType::SKILL_TIMESTOP) };
                            g_OwnedAugs.assign(owned, owned + 3);
                            for (int idx : owned) {
                                g_TypeOwned[(int)ALL_AUGS[idx].type] = true;
                                gm.takenOnce[idx] = true;
                            }
                            gm.score = 4321; gm.scoreAccum = 4321.0f; gm.xp = XpFromWhole(55); gm.playerLevel = 3;
                            gm.playerHP = 42.0f;
                            g_Stats.killCount = 777;
                            g_CurrentWeapon = (int)StartWeapon::SMG;
                            g_GameTime = 99.0f;
                            // Match the wave to the filled time; otherwise the next
                            // frame enters wave 2 and applies a random wave debuff,
                            // which leaks into the following snapshots.
                            gm.currentWave = CurrentWaveNumber(g_GameTime, g_Difficulty);
                        };
                        auto dump = [&](const std::string& label) {
                            int drones = 0, chakrams = 0, skills = 0, shocks = 0, debris = 0,
                                typeOwned = 0, taken = 0, tier3 = 0, tier4 = 0;
                            for (const auto& d : g_Drones) drones += (d.angle != 0.0f || d.fireTimer != 0.0f);
                            for (const auto& c : g_Chakrams) chakrams += c.alive;
                            float timeCd = 0.0f;
                            for (const auto& s : g_Skills) {
                                skills += (s.type != SkillType::NONE);
                                if (s.type == SkillType::TIME_STOP) timeCd = s.cd;
                            }
                            for (const auto& s : g_ShockWaves) shocks += s.active;
                            for (const auto& p : g_Debris) debris += p.active;
                            for (bool t : g_TypeOwned) typeOwned += t;
                            for (bool t : gm.takenOnce) taken += t;
                            for (const Monster* mob : g_MonsterManager.monsters) {
                                if (!mob->alive) continue;
                                tier3 += mob->kind == MobKind::GRAVIS ||
                                         mob->kind == MobKind::REGULUS ||
                                         mob->kind == MobKind::MAGNETAR;
                                tier4 += mob->kind == MobKind::GENESIS ||
                                         mob->kind == MobKind::QUASAR;
                            }
                            std::filesystem::create_directories("Screenshots");
                            FILE* f = nullptr;
#ifdef _MSC_VER
                            if (fopen_s(&f, "Screenshots/reg04_state.txt", "a") != 0) f = nullptr;
#else
                            f = std::fopen("Screenshots/reg04_state.txt", "a");
#endif
                            if (!f) return;
                            std::fprintf(f,
                                "%s state=%d drones=%d chakrams=%d laser=%d orbs=%d skills=%d replaceIdx=%d "
                                "dashCd=%.2f timeStop=%.2f focus=%.2f timeCd=%.2f shield=%.1f dashHeld=%d skillHeld=%d grace=%.2f "
                                "bullets=%d monsters=%d ranged=%d stardust=%d shocks=%d debris=%d queue=%d "
                                "owned=%d typeOwned=%d takenOnce=%d score=%lld xp=%.3f level=%d hp=%.1f "
                                "kills=%lld weapon=%d time=%.1f rewards=%d levelup=%d "
                                "wave=%d lastDebuff=%d waveVictory=%d tier3=%d tier4=%d "
                                "twin=%d damage=%.5f slots=%d pendingAug=%d choice0=%s\n",
                                label.c_str(), (int)gm.currentState, drones, chakrams,
                                (int)g_LaserBeams.size(), (int)g_ApproachOrbs.size(), skills,
                                g_SkillReplaceIdx, g_PlayerRuntime.dashCooldown,
                                g_PlayerRuntime.timeStopTimer, g_PlayerRuntime.hyperFocusTimer,
                                timeCd, g_PlayerShield,
                                (int)g_PlayerRuntime.dashInputHeld, (int)g_PlayerRuntime.skillInputHeld[0],
                                g_PlayerRuntime.postPickGrace, (int)g_Bullets.size(),
                                (int)g_MonsterManager.monsters.size(),
                                (int)g_MonsterManager.rangedMobs.size(),
                                (int)g_StardustPickups.size(), shocks, debris,
                                (int)g_AugEffectQueue.size(), (int)g_OwnedAugs.size(), typeOwned, taken,
                                gm.score, XpToDouble(gm.xp), gm.playerLevel, gm.playerHP,
                                g_Stats.killCount, g_CurrentWeapon, g_GameTime,
                                gm.pendingAugmentRewards,
                                (int)gm.levelUpTransition.Active(),
                                gm.currentWave, gm.lastWaveDebuff,
                                (int)gm.waveVictory, tier3, tier4,
                                (int)g_Stats.twin, g_Stats.damageMultiplier,
                                CountIdentitySlotsUsed(), gm.pendingAugIdx,
                                gm.augChoiceCount > 0 && gm.augChoices[0] >= 0 && gm.augChoices[0] < AUG_TOTAL
                                    ? ALL_AUGS[gm.augChoices[0]].name : "NONE");
                            std::fclose(f);
                        };
                        const size_t eq = step.find('=');
                        const std::string arg = eq == std::string::npos ? "" : step.substr(eq + 1);
                        if (step.rfind("REG04DUMP=", 0) == 0) {
                            dump(arg);
                        } else if (arg.rfind("PICK_", 0) == 0) {
                            RestartCurrentRun();
                            g_OwnedAugs.assign(IdentitySlotMax(), augIdx(AugType::PROB_CHAIN));
                            RebuildPlayerStatsFromOwned(screenWidth, screenHeight);
                            gm.QueueAugmentReward();
                            if (arg == "PICK_REWARD") gm.QueueAugmentReward();
                            gm.ActivateNextAugmentReward();
                            g_AugEffectProcessing = true;
                            g_AugEffectQueue.push_back(augIdx(AugType::TWIN));
                            ProcessAugEffectQueue(screenWidth, screenHeight);
                            dump("PICK_OPEN");
                            if (arg == "PICK_CHAIN")
                                g_AugEffectQueue.push_back(augIdx(AugType::PIERCE));
                            if (arg == "PICK_CANCEL") CancelAugReplaceFlow();
                            else CompleteAugReplaceFlow(0, screenWidth, screenHeight);
                            if (arg == "PICK_CHAIN") {
                                dump("PICK_CHAIN_WAIT");
                                CompleteAugReplaceFlow(0, screenWidth, screenHeight);
                            }
                            dump(arg);
                        } else if (arg == "READY") {
                            fill();
                            gm.currentState = GameState::READY;
                        } else {
                            fill();
                            if (arg == "NEWRUN") ResetForNewGame();
                            else if (arg == "RESTART") RestartCurrentRun();
                            else if (arg == "REBUILD") RebuildPlayerStatsFromOwned(screenWidth, screenHeight);
                            else if (arg == "REBUILD_NO_TIME") {
                                g_OwnedAugs.pop_back();
                                RebuildPlayerStatsFromOwned(screenWidth, screenHeight);
                            }
                            else if (arg == "ABANDON") AbandonCurrentRun();
                            dump(arg);
                        }
                    } else if (step.rfind("AUGSEL", 0) == 0) {
                        // AUGSEL or AUGSEL=<AugDef::name>: the named card is
                        // placed in slot 1 and keyboard-focused so its
                        // detail panel is visible in the capture.
                        if (gm.currentState != GameState::RUNNING)
                            RestartCurrentRun();
                        gm.QueueAugmentReward();
                        gm.ActivateNextAugmentReward();
                        const size_t eq = step.find('=');
                        if (eq != std::string::npos && gm.augChoiceCount > 0) {
                            const std::string want = step.substr(eq + 1);
                            for (int i = 0; i < AUG_TOTAL; ++i) {
                                if (want == ALL_AUGS[i].name) {
                                    gm.augChoices[0] = i;
                                    break;
                                }
                            }
                            g_SmokeAugFocus = 0;
                        }
                    } else if (step.rfind("GIVE=", 0) == 0) {
                        // GIVE=<AugDef::name>: grant an augment to the run.
                        const std::string want = step.substr(5);
                        for (int i = 0; i < AUG_TOTAL; ++i) {
                            if (want == ALL_AUGS[i].name) {
                                ApplySingleAugIdx(i, screenWidth, screenHeight);
                                break;
                            }
                        }
                    }
                    if (step.rfind("LEVEL=", 0) == 0) {
                        gm.playerLevel = std::max(1, std::atoi(step.c_str() + 6));
                    } else if (step.rfind("WAVETIME=", 0) == 0) {
                        // HUD-only smoke fixture: jump to a wave without replaying combat.
                        g_GameTime = std::clamp(std::strtof(step.c_str() + 9, nullptr),
                            0.0f, TotalWaveDurationSeconds(g_Difficulty) - 1.0f);
                        gm.currentWave = CurrentWaveNumber(g_GameTime, g_Difficulty);
                    } else if (step.rfind("XP=", 0) == 0) {
                        // XP=<n>: one large XP gain, levelled by the next tick.
                        GrantExperience(XpFromReward(std::atof(step.c_str() + 3)));
                    } else if (step.rfind("DUST=", 0) == 0) {
                        // DUST=<n>: stardust picked up this run (wallet too,
                        // as a real pickup does). Never saved.
                        const long long n = std::atoll(step.c_str() + 5);
                        AddSaveValue(g_RunStardust, n);
                        AddSaveValue(g_Coins, n);
                    } else if (step.rfind("COINS=", 0) == 0) {
                        // COINS=<n>: wallet for shop states (never saved).
                        g_Coins = std::atoll(step.c_str() + 6);
                    } else if (step.rfind("WHEEL=", 0) == 0) {
                        // WHEEL=<notches>: same sign as the GLFW wheel
                        // callback (yoffset); negative scrolls down.
                        g_ScrollAccum += (float)std::atof(step.c_str() + 6);
                    } else if (step == "HURT") {
                        // Same timers a real hit sets (red edge + HP pop).
                        g_HurtVignette = 0.5f;
                        g_HpBarPop = 0.35f;
                    } else if (step.rfind("SLOW=", 0) == 0) {
                        // SLOW=x: scale game time (e.g. 0.2) to capture fast
                        // animations frame by frame.
                        g_SmokeTimeScale = std::clamp((float)atof(step.c_str() + 5), 0.02f, 1.0f);
                    } else if (step.rfind("GENESISROTOR=", 0) == 0 || step.rfind("GENESISCYCLE=",0)==0) {
                        const bool fullCycle=step.rfind("GENESISCYCLE=",0)==0;
                        if (gm.currentState != GameState::RUNNING) RestartCurrentRun();
                        g_MonsterManager.Clear();
                        g_Bullets.clear();
                        std::srand(1234);
                        const float px = screenWidth * 0.58f, py = screenHeight * 0.57f;
                        playerWin.x = px - playerWin.width*0.5f;
                        playerWin.y = py - playerWin.height*0.5f;
                        auto* g = new Monster(screenWidth*0.30f,screenHeight*0.35f);
                        g->MakeKind(MobKind::GENESIS);
                        g->hp = 1000000.0f; g->speed = 0.0f;
                        g->genesisNextRotor = !fullCycle;
                        g->hiveOrbitAngle=atan2f(py-g->worldY,px-g->worldX);
                        g->spawnTimer = Monster::GENESIS_TRACK_TIME;
                        g_MonsterManager.monsters.push_back(g);
                        const glm::vec4 bounds(0,BROWSER_CHROME_H,screenWidth,
                                               screenHeight-g_GameBarH-g_TaskbarH);
                        float qaHp = 10000.0f;
                        g_MonsterManager.UpdateAll(px,py,0.0f,qaHp,g_Bullets,1,1,1,-1,-1,-1,-1,bounds);
                        const float age = std::clamp(std::strtof(step.c_str()+13,nullptr),0.0f,fullCycle ? 10.0f : 3.8f);
                        for (float t=0;t<age;) {
                            const float dt = std::min(1.0f/120.0f,age-t); t += dt;
                            g_MonsterManager.UpdateAll(px+sinf(t*2.0f)*100.0f,py+sinf(t*1.5f)*80.0f,
                                                      dt,qaHp,g_Bullets,1,1,1,-1,-1,-1,-1,bounds);
                        }
                        playerWin.x+=sinf(age*2.0f)*100.0f;
                        playerWin.y+=sinf(age*1.5f)*80.0f;
                        g_SmokeTimeScale = 0.0f;
                    } else if (step.rfind("GRAVISGLIDE=", 0) == 0 || step.rfind("GRAVISSLING=", 0) == 0 ||
                               step.rfind("GRAVISTEST=", 0) == 0) {
                        // GRAVISGLIDE=<sec> / GRAVISSLING=<sec>: replay a Gravis
                        // elite pattern from its start to the given time and
                        // freeze for the capture (like QUASARFALL).
                        // GRAVISTEST=GLIDE|SLING: start the pattern on the live
                        // game loop (full speed) with the player standing still
                        // at the destination; pair with GRAVISDUMP=<label>.
                        if (gm.currentState != GameState::RUNNING) RestartCurrentRun();
                        g_MonsterManager.Clear();
                        g_Bullets.clear();
                        g_PlayerRuntime.knockbackVX = g_PlayerRuntime.knockbackVY = 0.0f;
                        g_PlayerRuntime.moveSlowTimer = 0.0f;
                        const bool live = step.rfind("GRAVISTEST=", 0) == 0;
                        const std::string gArg = step.substr(step.find('=') + 1);
                        const bool wantGlide = live ? gArg == "GLIDE" : step.rfind("GRAVISGLIDE=", 0) == 0;
                        const float gpx = screenWidth * 0.61f, gpy = screenHeight * 0.64f;
                        playerWin.x = gpx - playerWin.width * 0.5f;
                        playerWin.y = gpy - playerWin.height * 0.5f;
                        std::srand(1234);
                        auto* gv = new Monster(screenWidth * 0.30f, screenHeight * 0.42f);
                        gv->MakeKind(MobKind::GRAVIS);
                        gv->hp = 1000000.0f;
                        gv->speed = 0.0f;
                        // The two machines are independent: park the other one so the
                        // capture shows only the pattern under test.
                        if (wantGlide) { gv->BeginGravisGlide(); gv->gravisSlingTimer = -1.0e6f; }
                        else { gv->BeginGravisSling(); gv->gravisTimer = -1.0e6f; }
                        const glm::vec4 gBounds(0, BROWSER_CHROME_H, screenWidth,
                                                screenHeight - g_GameBarH - g_TaskbarH);
                        if (live) {
                            gm.playerHP = g_Stats.maxHP;
                            g_SmokeTimeScale = 1.0f;
                        } else {
                            const float age = std::clamp(std::strtof(step.c_str() + 12, nullptr),
                                                        0.0f, 14.0f);
                            float qaHp = 10000.0f;
                            for (float t = 0.0f; t < age; ) {
                                const float dt = std::min(1.0f / 120.0f, age - t);
                                t += dt;
                                gv->Update(gpx, gpy, dt, qaHp, 1.0f, -1, -1, -1, -1, nullptr, gBounds);
                            }
                            g_SmokeTimeScale = 0.02f;
                        }
                        g_MonsterManager.monsters.push_back(gv);
                    } else if (step.rfind("GRAVISDUMP=", 0) == 0) {
                        // Append one state line to Screenshots/gravis_state.txt
                        // (read by scripts/test_gravis_elite_smoke.ps1).
                        const Monster* gv = nullptr;
                        for (const Monster* mob : g_MonsterManager.monsters)
                            if (mob->alive && mob->kind == MobKind::GRAVIS) { gv = mob; break; }
                        std::filesystem::create_directories("Screenshots");
                        FILE* gf = nullptr;
#ifdef _MSC_VER
                        if (fopen_s(&gf, "Screenshots/gravis_state.txt", "a") != 0) gf = nullptr;
#else
                        gf = std::fopen("Screenshots/gravis_state.txt", "a");
#endif
                        if (gf) {
                            const float gcx = playerWin.x + playerWin.width * 0.5f;
                            const float gcy = playerWin.y + playerWin.height * 0.5f;
                            const float gdx = gv ? gcx - gv->worldX : 0.0f;
                            const float gdy = gv ? gcy - gv->worldY : 0.0f;
                            std::fprintf(gf,
                                "%s state=%d sling=%d field=%d hp=%.2f maxhp=%.2f slow=%.2f dist=%.1f influence=%.3f\n",
                                step.c_str() + 11, gv ? gv->gravisState : -1, gv ? gv->gravisSling : -1,
                                gv ? (int)gv->GravisFieldActive() : -1, gm.playerHP, g_Stats.maxHP,
                                g_PlayerRuntime.moveSlowTimer, std::sqrt(gdx * gdx + gdy * gdy),
                                GravisField::g_playerInfluence);
                            std::fclose(gf);
                        }
                    } else if (step.rfind("QUASARFALL=", 0) == 0 || step.rfind("QUASARBEAM=", 0) == 0) {
                        // Deterministic motion QA for both charge animations.
                        if (gm.currentState != GameState::RUNNING) RestartCurrentRun();
                        g_MonsterManager.Clear();
                        playerWin.x = screenWidth * 0.61f - playerWin.width * 0.5f;
                        playerWin.y = screenHeight * 0.64f - playerWin.height * 0.5f;
                        std::srand(1234); // repeatable motion replay, before random initial angles
                        auto* q = new Monster(screenWidth * 0.30f, screenHeight * 0.42f);
                        q->MakeKind(MobKind::QUASAR);
                        q->hp = 1000000.0f;
                        q->speed = 0.0f;
                        if (step.rfind("QUASARFALL=", 0) == 0) q->BeginQuasarFall();
                        else q->quasarTimer = Monster::QUASAR_COOLDOWN_TIME - Monster::QUASAR_PRECHARGE_TIME;
                        const float age = std::clamp(std::strtof(step.c_str() + 11, nullptr),
                                                    0.0f, 6.35f);
                        float qaHp = 10000.0f;
                        const glm::vec4 bounds(0, BROWSER_CHROME_H, screenWidth,
                                               screenHeight - g_GameBarH - g_TaskbarH);
                        for (float t = 0.0f; t < age; ) {
                            const float dt = std::min(1.0f / 120.0f, age - t);
                            t += dt;
                            q->Update(screenWidth * 0.61f + sinf(t * 1.5f) * 90.0f,
                                      screenHeight * 0.64f + cosf(t * 1.5f) * 55.0f,
                                      dt, qaHp, 1.0f, -1, -1, -1, -1, nullptr, bounds);
                        }
                        playerWin.x += sinf(age * 1.5f) * 90.0f;
                        playerWin.y += cosf(age * 1.5f) * 55.0f;
                        g_MonsterManager.monsters.push_back(q);
                        g_SmokeTimeScale = 0.02f;
                    } else if (step == "TRIAL10") {
                        // TRIAL10: the max HP trial is on, to see trial values in the report.
                        g_TrialPool[0] = 10;
                        g_TrialSelected[0] = true;
                    } else if (step == "DEATH") {
                        // DEATH: the player dies now through the normal
                        // HP <= 0 path, so the game-over report is a real one.
                        gm.playerHP = 0.0f;
                    } else if (step == "KILL") {
                        // KILL: every living enemy dies now through the normal
                        // death pass (rewards, death animations).
                        for (auto* mob : g_MonsterManager.monsters)
                            if (mob->alive) { mob->hp = 0.0f; mob->alive = false; }
                    } else if (step == "SHIELD") {
                        // SHIELD: give every living non-Regulus enemy a full
                        // Regulus shield, to check the shield mark on each kind.
                        auto grant = [](EnemyShieldState& shield) {
                            shield = {};
                            shield.acquiredOnce = true;
                            shield.hp = shield.maxHp = 200.0f;
                        };
                        for (auto* mob : g_MonsterManager.monsters)
                            if (mob->alive && mob->kind != MobKind::REGULUS)
                                grant(mob->regulusShield);
                        for (auto* mob : g_MonsterManager.rangedMobs)
                            if (mob->alive) grant(mob->regulusShield);
                    } else if (step == "SPAWN=SCOPE") {
                        // SPAWN=SCOPE: one ranged Scope 600px right of the player.
                        const size_t before = g_MonsterManager.rangedMobs.size();
                        g_MonsterManager.SpawnRangedMob(screenWidth, screenHeight,
                                                        1.0f, (int)before + 1);
                        if (g_MonsterManager.rangedMobs.size() > before) {
                            RangedMob* mob = g_MonsterManager.rangedMobs.back();
                            mob->worldX = playerWin.x + playerWin.width * 0.5f + 600.0f;
                            mob->worldY = playerWin.y + playerWin.height * 0.5f;
                        }
                    } else if (step.rfind("SPAWN=", 0) == 0) {
                        // SPAWN=<MobKind index>: one mob 600px right of the
                        // player, for capturing a specific enemy's visuals.
                        const int kind = std::atoi(step.c_str() + 6);
                        const size_t before = g_MonsterManager.monsters.size();
                        if (kind >= 0 && kind <= (int)MobKind::MAGNETAR)
                            g_MonsterManager.SpawnMob(screenWidth, screenHeight, 160);
                        if (g_MonsterManager.monsters.size() > before) {
                            Monster* mob = g_MonsterManager.monsters.back();
                            mob->worldX = playerWin.x + playerWin.width * 0.5f + 600.0f;
                            mob->worldY = playerWin.y + playerWin.height * 0.5f;
                            mob->MakeKind((MobKind)kind);
                        }
                    } else if (step.rfind("SKILLHUD=", 0) == 0) {
                        // SKILLHUD=MIX|EMPTY|PULSE: skill HUD states in one
                        // frame (cooldowns, effects, passives) on a fresh run.
                        const std::string preset = step.substr(9);
                        g_OwnedAugs.clear();   // RestartCurrentRun keeps owned cards
                        RestartCurrentRun();
                        auto give = [&](AugType t) {
                            for (int i = 0; i < AUG_TOTAL; ++i)
                                if (ALL_AUGS[i].type == t) {
                                    ApplySingleAugIdx(i, screenWidth, screenHeight);
                                    return;
                                }
                        };
                        ClearEquippedSkills();
                        if (preset == "MIX") {
                            EquipSkill(SkillType::CLOSE_WINDOW);
                            EquipSkill(SkillType::HYPER_FOCUS);
                            EquipSkill(SkillType::TIME_STOP);
                            g_Skills[0].cd = PlayerSkillCooldownMax(SkillType::CLOSE_WINDOW) * 0.6f;
                            g_Skills[1].cd = 12.0f;
                            g_Skills[2].cd = 0.0f;
                            g_PlayerRuntime.hyperFocusTimer = 3.0f;
                            g_PlayerRuntime.dashCooldown = DASH_CD;   // ~half left at the ~1.5 s capture
                            g_PlayerRuntime.dashCooldownMax = DASH_CD;
                            give(AugType::BULLET_RAIN);
                            g_BulletRainTimer = g_Stats.bulletRainCooldown * 0.6f;
                            give(AugType::LIGHT_STEP);
                            g_Stats.lightStepDisableTimer = 7.0f;
                            for (int k = 0; k < 3; ++k) give(AugType::D_APPROACH);
                        } else if (preset == "PULSE") {
                            EquipSkill(SkillType::TIME_STOP);
                            g_Skills[0].cd = 0.05f;
                            g_SmokeTimeScale = 0.2f;
                        }
                    } else if (step.rfind("NORMATEST=", 0) == 0) {
                        // NORMATEST=AIM|DASH|RAM|HIDE|P2: bring NORMA in (to the
                        // player's right) and start that pattern now; P2 drops it
                        // into phase 2. Motion captures follow as plain steps.
                        const std::string arg = step.substr(10);
                        // A new run opens on its starting pick: take the first
                        // card so the fight actually runs.
                        if (gm.currentState == GameState::AUG_SELECT && gm.augChoiceCount > 0) {
                            g_AugEffectQueue.push_back(gm.augChoices[0]);
                            g_AugEffectProcessing = true;
                            ProcessAugEffectQueue(screenWidth, screenHeight);
                            g_AugEffectProcessing = false;
                            gm.maxHP = g_Stats.maxHP;
                            FinishAugmentEffects();
                        }
                        if (!FindNorma())
                            DebugSpawnNorma(g_MonsterManager, screenWidth,
                                            g_Stats.monsterHpMult *
                                            GetDifficultyParams(g_Difficulty).enemyHpMultiplier,
                                            g_Difficulty);
                        // Clear the stage so the capture shows NORMA alone.
                        for (Monster* m : g_MonsterManager.monsters)
                            if (m->alive && m->kind != MobKind::NORMA) m->alive = false;
                        if (g_ThrowBox.id != 0) os_pane::Close(g_ThrowBox.id);
                        g_ThrowBox = {};
                        if (Monster* boss = FindNorma()) {
                            if (arg == "P2") {
                                boss->hp = boss->normaMaxHp * 0.35f;
                            } else {
                                boss->worldX = playerWin.x + playerWin.width * 0.5f + 380.0f;
                                boss->worldY = playerWin.y + playerWin.height * 0.5f - 60.0f;
                                const norma::Pattern p = arg == "AIM" ? norma::Pattern::Aim
                                                       : arg == "DASH" ? norma::Pattern::Dash
                                                       : arg == "RAM" ? norma::Pattern::Ram
                                                       : arg == "HIDE" ? norma::Pattern::Hide
                                                       : norma::Pattern::None;
                                if (p != norma::Pattern::None) boss->StartNormaPattern(p);
                            }
                        }
                    } else if (step.rfind("HP=", 0) == 0) {
                        // HP=<0..1>: set the player's health fraction.
                        const float f = std::clamp(
                            (float)std::atof(step.c_str() + 3), 0.01f, 1.0f);
                        gm.playerHP = g_Stats.maxHP * f;
                    } else if (step.rfind("DIFF=", 0) == 0) {
                        // DIFF=EASY|NORMAL|HARD: run difficulty (wave count,
                        // elite schedule) for wave HUD captures.
                        const std::string d = step.substr(5);
                        g_Difficulty = d == "EASY" ? Difficulty::EASY
                                     : d == "HARD" ? Difficulty::HARD
                                     : Difficulty::NORMAL;
                    } else if (step.rfind("CODEXMOB=", 0) == 0) {
                        // CODEXMOB=UNSEEN|SEEN|KNOWN: Astral Log state of
                        // every enemy, for the elite name tag. Never saved.
                        const std::string st = step.substr(9);
                        for (int id = 0; id < CM_COUNT; ++id) {
                            g_MobSeen[id] = st != "UNSEEN";
                            g_MobKillCounts[id] = st == "KNOWN" ? 1 : 0;
                            g_RunMobKillCounts[id] = 0;
                        }
                    }
                    if (step.rfind("SEARCH=", 0) == 0) {
                        // SEARCH=<ascii text>: fill the Codex search field
                        // as if typed (keyboard input is not scripted).
                        const std::string q = step.substr(7);
                        CodexSearchClear();
                        for (char ch : q) {
                            if (g_CodexSearchLen >= 63) break;
                            g_CodexSearch[g_CodexSearchLen++] = (wchar_t)ch;
                        }
                        g_CodexSearch[g_CodexSearchLen] = L'\0';
                        g_CodexSearchCaret = g_CodexSearchLen;
                    }
                    // "WAIT" (or any other name) keeps the current scene.
                });
            // Scripted pointer events arrive after the normal input pass.
            // Route them through the same toolkit hitboxes as physical clicks.
            if (g_DebugToolkit.IsVisible() && lmb)
                g_DebugToolkit.BeginInput(nullptr, (float)screenWidth,
                    (float)screenHeight, mx, my, lmb, false);
            wmx = ScreenToWorldX((float)mx);
            wmy = ScreenToWorldY((float)my);
        }

        // GAMEOVER?�READY ?�동 감�? (ESC ?�으�?직접 ?�환??경우)
        if (prevState == GameState::GAMEOVER &&
            g_GameManager.currentState == GameState::READY) {
            ResetForNewGame();
        }
        if (prevState == GameState::READY &&
            g_GameManager.currentState == GameState::RUNNING) {
        }
        // PAUSED ???�른 ?�태: 증강 ?�명 박스 ?�기
        if (prevState == GameState::PAUSED &&
            g_GameManager.currentState != GameState::PAUSED) {
            g_PauseSelectedAug = -1;
            g_ShowPauseInventory = false;
        }
        if (!debugInputCaptured)
            g_GameManager.UpdateStateSystem(g_MonsterManager, g_Bullets);
        SyncGameplayTelemetry();

        // Keep the native window title synchronized with the active scene and
        // the live run counters.  This is intentionally throttled so title
        // updates do not compete with rendering, while still making the
        // borderless window useful in task switching and diagnostics.
        static double lastTitleUpdate = -1.0;
        const double titleNow = glfwGetTime();
        if (g_GameManager.currentState != prevState ||
            lastTitleUpdate < 0.0 || titleNow - lastTitleUpdate >= 0.25) {
            lastTitleUpdate = titleNow;
            const char* scene = "Menu";
            switch (g_GameManager.currentState) {
            case GameState::READY:          scene = "Ready"; break;
            case GameState::RUNNING:        scene = "In Game"; break;
            case GameState::PAUSED:         scene = "Paused"; break;
            case GameState::DYING:          scene = "Defeated"; break;
            case GameState::AUG_SELECT:     scene = "Augment Select"; break;
            case GameState::AUG_REPLACE:    scene = "Augment Replace"; break;
            case GameState::DEBUFF_SELECT:  scene = "Modifier Select"; break;
            case GameState::GAMEOVER:       scene = "Game Over"; break;
            case GameState::CODEX:          scene = "Codex"; break;
            case GameState::TUTORIAL:       scene = "Tutorial"; break;
            case GameState::SETTINGS:       scene = "Settings"; break;
            case GameState::CREATIVE_CONFIG:scene = "Creative Config"; break;
            default:                         break;
            }
            std::string title = std::string("Onedow - ") + scene;
            if (g_GameManager.currentState == GameState::RUNNING ||
                g_GameManager.currentState == GameState::DYING ||
                g_GameManager.currentState == GameState::PAUSED ||
                g_GameManager.currentState == GameState::AUG_SELECT ||
                g_GameManager.currentState == GameState::DEBUFF_SELECT) {
                title += " | FPS " + std::to_string(g_CurrentFPS);
                title += " | HP " + std::to_string((int)std::max(0.0f, g_GameManager.playerHP));
                title += " | XP " + FormatXp(g_GameManager.xp);
                if (g_GameManager.currentState == GameState::AUG_SELECT ||
                    g_GameManager.currentState == GameState::DEBUFF_SELECT)
                    title += " | Choices " + std::to_string(g_GameManager.augChoiceCount);
            }
            glfwSetWindowTitle(window, title.c_str());
        }

        // Keep the main loop music active during gameplay and stop it in menus.
        {
            Audio::SetEnabled(g_AudioEngineEnabled && g_SoundVol > 0);
            g_BgmEnabled = g_BgmVol > 0;
            g_SfxEnabled = g_SfxVol > 0;
            Audio::SetBgmVolume(g_BgmVol / 100.0f);
            Audio::SetSfxVolume(g_SfxVol / 100.0f);
            Audio::SetBgmEnabled(g_BgmEnabled);
            Audio::SetSfxEnabled(g_SfxEnabled);
            Audio::SetVolume(g_SoundVol / 100.0f);        // 마스??볼륨
            GameState cs = g_GameManager.currentState;
            bool bgmOn = (cs == GameState::RUNNING || cs == GameState::PAUSED ||
                          cs == GameState::AUG_SELECT || cs == GameState::DEBUFF_SELECT ||
                          cs == GameState::READY);
            if (bgmOn) Audio::PlayBgmMain();
            else       Audio::StopBgm();
        }

        // ?�리?�이?�브 무적 ??�??�레??체력 ?�?고정 (?��? 죽�? ?�음)
        if (g_CreativeGodmode && (g_CreativeMode || g_DebugMode) &&
            g_GameManager.currentState == GameState::RUNNING) {
            g_GameManager.playerHP = g_Stats.maxHP;
        }

        // HP 0 ??MK2 부??OR DYING (1�??�로??모션 ??GAMEOVER)
        if (g_GameManager.playerHP <= 0.0f &&
            g_GameManager.currentState == GameState::RUNNING) {
            // MK2: 1??부????공격??비�? ??�� + ?�?HP (?�널???�음)
            if (g_Stats.mk2 && !g_Stats.mk2Used) {
                g_Stats.mk2Used = true;
                float pCX = playerWin.x + playerWin.width  * 0.5f;
                float pCY = playerWin.y + playerWin.height * 0.5f;
                float blastDmg = g_Stats.GetBaseDamage()
                               * g_Stats.GetDamageMultiplier() * 6.0f;
                float blastRad = 280.0f;
                // 주�? ?�에�??��?지
                for (auto m : g_MonsterManager.monsters) {
                    if (!m->alive) continue;
                    float dx = m->worldX - pCX, dy = m->worldY - pCY;
                    if (dx*dx + dy*dy < blastRad * blastRad) {
                        g_MonsterManager.ApplyDamage(*m, blastDmg);
                    }
                }
                for (auto rm : g_MonsterManager.rangedMobs) {
                    if (!rm->alive) continue;
                    float dx = rm->worldX - pCX, dy = rm->worldY - pCY;
                    if (dx*dx + dy*dy < blastRad * blastRad) {
                        g_MonsterManager.ApplyDamage(*rm, blastDmg);
                    }
                }
                // �ð� ȿ�� ? ���� + �����?+ ȭ�� ����
                SpawnEnemyExplosion(pCX, pCY, 1.0f, 0.8f, 0.3f, true);
                SpawnEnemyExplosion(pCX, pCY, 0.4f, 1.0f, 0.8f, true);
                SpawnShockWave(pCX, pCY, blastRad * 1.4f, 0.55f,
                               1.0f, 0.85f, 0.3f);
                g_ShakeTime = 0.45f; g_ShakeMag = 22.0f;
                TriggerFlash(1.0f, 0.9f, 0.4f, 0.8f);   // 부????강한 번쩍
                TriggerHitStop(0.14f);                  // ?�팩???��?

                // 부?????�력�??�널???�이 ?�?HP �?(?�버???�음)
                g_GameManager.maxHP    = g_Stats.maxHP;
                g_GameManager.playerHP = g_Stats.maxHP;
                g_PrevHP               = g_GameManager.playerHP;
                // RUNNING ?�태 ?��? (DYING ?�이 X)
            } else {
                // ?�반 ?�망 ???�레?�어 기점 ?�??��(모든 ???�짐) ??메뉴 ?�이?�인
                g_GameManager.currentState = GameState::DYING;
                g_GameManager.playerHP     = 0.0f;
                Audio::StopBgm();
                float pCX = playerWin.x + playerWin.width  * 0.5f;
                float pCY = playerWin.y + playerWin.height * 0.5f;
                g_DeathCX = pCX; g_DeathCY = pCY;
                g_DyingTimer    = DYING_DUR;
                g_DeathBoomDone = false;
                g_DeathFlash    = 0.0f;
                // ?�리모프 ??�??�복 (?�망 ?�출??�??�음)
                g_ZoomCX = g_ZoomCY = 0.0f;
                g_ViewZoom = 1.0f; g_ViewZoomTarget = 1.0f;
                // ?�망 ?�인 ???�티????�� ?? 가??가까운 ?�협(?�로?�스)??기록 (결과창에 ?�시??
                {
                    float best = 1e18f; const wchar_t* nm = nullptr;
                    auto consider = [&](float ex, float ey, const wchar_t* n) {
                        float dx = ex-pCX, dy = ey-pCY, d = dx*dx+dy*dy;
                        if (d < best) { best = d; nm = n; }
                    };
                    for (auto m  : g_MonsterManager.monsters)   if (m->alive)  consider(m->worldX,  m->worldY,  MobName(CodexMobIdForKind(m->kind)));
                    for (auto r  : g_MonsterManager.rangedMobs) if (r->alive)  consider(r->worldX,  r->worldY,  MobName(CM_SCOPE));
                    int li = LangIndex();
                    const wchar_t* FMT[3] = { L"%ls: signal dispersed", L"Dispersed by %ls", L"%ls dispersed" };
                    const wchar_t* UNK[3] = { L"Unknown error", L"Terminated by unknown error", L"Unknown error" };
                    if (nm) swprintf_s(g_DeathReason, FMT[li], nm);
                    else    wcscpy_s(g_DeathReason, UNK[li]);
                    if (g_NormaDoom) {
                        static const wchar_t* const DOOM[3] = {
                            L"NORMA의 심판: 시간 초과", L"Judged by NORMA: time ran out",
                            L"NORMAの裁き: 時間切れ" };
                        wcscpy_s(g_DeathReason, DOOM[li]);
                    }
                }
            }   // close else (MK2 분기 ??
        }       // close outer if (HP <= 0)

        // DYING ?�망 ?�출 ???�레?�어 기점 ?�??��(모든 ???�짐) ??GAMEOVER (�?변???�음)
        if (g_GameManager.currentState == GameState::DYING) {
            g_DyingTimer -= delta;
            g_DeathFlash -= delta * 4.0f;
            if (g_DeathFlash < 0.0f) g_DeathFlash = 0.0f;

            // ?�??�� ???�레?�어 기점, 모든 ?�이 ?�짐 (즉시, �??�네마틱 ?�음)
            if (!g_DeathBoomDone) {
                g_DeathBoomDone = true;
                float pCX = g_DeathCX, pCY = g_DeathCY;
                // 모든 ????��?�키�??�거 (scored/noBlast ?�시 ???�수/?�쇄 ?�산 ????
                for (auto m : g_MonsterManager.monsters) if (m->alive) {
                    m->SpawnDeathEffect((float)now, false, false);
                    m->alive = false; m->scored = true; m->noBlast = true;
                }
                for (auto r : g_MonsterManager.rangedMobs) if (r->alive) {
                    r->SpawnDeathEffect();
                    r->alive = false; r->scored = true;
                }
                // 모든 ?�·보?�·분?�체·총알·?�탑??즉시 ?�전 ??�� ??UpdateAll(RUNNING ?�용)??                //   맡기�?DYING/GAMEOVER ?�안 ?�거�?�?�??�이 ?��? ?�태�??�으므�??�기???�거.
                g_MonsterManager.Clear();   // monsters, ranged mobs, and projectiles are deleted and cleared
                g_Bullets.clear();
                g_LaserBeams.clear();   // ?�캔 ?�이?�?�??�리
                   // 배드 ?�터 감속 구역/출혈 ?�리
                // ?�레?�어 중심 ?�??�� + 충격??+ ?�광 + ?�들�?+ 방사???�편
                for (int k = 0; k < 4; k++)
                    SpawnEnemyExplosion(pCX, pCY, 1.0f, 0.85f - (k%2)*0.4f, 0.3f, true);
                SpawnShockWave(pCX, pCY, 900.0f, 0.7f, 0.4f, 0.9f, 1.0f);
                SpawnShockWave(pCX, pCY, 560.0f, 0.55f, 1.0f, 0.9f, 0.4f);
                g_DeathFlash = 1.0f;
                g_ShakeTime = 0.5f; g_ShakeMag = 30.0f;
                TriggerFlash(0.6f, 0.85f, 1.0f, 0.8f);
                for (int i = 0; i < MAX_DEBRIS; i++) {
                    float angle = (float)i / (float)MAX_DEBRIS * 2.0f * (float)M_PI
                                + ((float)(rand() % 100) - 50.0f) * 0.01f;
                    float spd = 450.0f + (float)(rand() % 650);
                    float sz  = 8.0f + (float)(rand() % 24);
                    int   tint = rand() % 10;
                    float cr, cg, cb;
                    if      (tint < 5) { cr = 0.10f; cg = 0.85f; cb = 1.00f; }
                    else if (tint < 8) { cr = 1.00f; cg = 1.00f; cb = 1.00f; }
                    else               { cr = 1.00f; cg = 0.85f; cb = 0.20f; }
                    g_Debris[i] = { pCX, pCY, cosf(angle)*spd, sinf(angle)*spd, sz, cr, cg, cb, true };
                }
            }

            float sd = delta;
            for (int i = 0; i < MAX_DEBRIS; i++) {
                if (!g_Debris[i].active) continue;
                g_Debris[i].x  += g_Debris[i].vx * sd;
                g_Debris[i].y  += g_Debris[i].vy * sd;
                g_Debris[i].vx *= (1.0f - 1.6f * sd);
                g_Debris[i].vy *= (1.0f - 1.6f * sd);
            }
            if (g_DyingTimer <= 0.0f) {
                g_GameplayTelemetry.EndRun(g_GameTime,
                                           g_GameManager.playerLevel,
                                           g_GameManager.xp,
                                           g_Stats,
                                           "gameover");
                g_ViewZoom = g_ViewZoomTarget = 1.0f;   // �??�복 (메뉴 ?�상??
                g_ZoomCX = g_ZoomCY = 0.0f;             // �?중심 ?�면 중앙?�로
                g_GameManager.currentState = GameState::GAMEOVER;
                g_DyingTimer = 0.0f;
                g_GameOverFade = 0.0f;   // 결과 메뉴 ?�이?�인 ?�작 (2.5�?
                // 기록 ?�?????�이?�별 최고???�적/코인 갱신 (?�기록이�??�시)
                //   ?�리?�이?�브 모드(?�드박스)??코인·기록 ?�외 (?�밍 방�?)
                g_LastRunRecord = CommitRunResult(!PlayRecordsProgress(), (int)g_Difficulty,
                                                  g_GameManager.score, g_Stats.killCount,
                                                  (g_SelectedJob == JOB_STATIC_FIELD ? 1 : 0),
                                                  g_GameManager.currentWave, false, g_GameTime);
            }
        }

        if (g_GameManager.currentState == GameState::GAMEOVER && g_GameOverFade < 1.0f) {
            g_GameOverFade += delta / GAMEOVER_FADE;
            if (g_GameOverFade > 1.0f) g_GameOverFade = 1.0f;
        }

        // ?�망 ??�� ?�여�??�티?�·파?�·충격파·?�파????게임?�버 ?�면??굳어버리지 ?�도�?        //   DYING/GAMEOVER ?�안?�도 계속 갱신???�연?�럽�??�라지�??�다.
        {
            GameState gvs = g_GameManager.currentState;
            if (gvs == GameState::DYING || gvs == GameState::GAMEOVER) {
                UpdateEnemyParticles(delta);
                for (auto& sw : g_ShockWaves) { if (!sw.active) continue; sw.life -= delta; if (sw.life <= 0.0f) sw.active = false; }
                                for (auto& sp : g_Sparks) {
                    sp.x += sp.vx * delta; sp.y += sp.vy * delta;
                    float drag = std::max(0.0f, 1.0f - 6.0f * delta);
                    sp.vx *= drag; sp.vy *= drag; sp.life -= delta;
                }
                g_Sparks.erase(std::remove_if(g_Sparks.begin(), g_Sparks.end(),
                    [](const Spark& s){ return s.life <= 0.0f; }), g_Sparks.end());
                if (gvs == GameState::GAMEOVER) {
                    for (int i = 0; i < MAX_DEBRIS; i++) { if (!g_Debris[i].active) continue;
                        g_Debris[i].x += g_Debris[i].vx * delta; g_Debris[i].y += g_Debris[i].vy * delta;
                        g_Debris[i].vx *= (1.0f - 1.6f * delta); g_Debris[i].vy *= (1.0f - 1.6f * delta);
                    }
                }
            }
        }

        // --- AUG_SELECT / DEBUFF_SELECT: 1/2/3 focus, SPACE confirm ---
        static bool s_augSpaceReleased = true;
        if (inputFocusChanged)
            s_augSpaceReleased = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE;
        {
            const int spaceState = glfwGetKey(window, GLFW_KEY_SPACE);
            if (spaceState == GLFW_RELEASE) s_augSpaceReleased = true;
        }
        if (!debugInputCaptured &&
            (g_GameManager.currentState == GameState::AUG_SELECT ||
             g_GameManager.currentState == GameState::DEBUFF_SELECT)) {
            const int k1 = glfwGetKey(window, GLFW_KEY_1);
            const int k2 = glfwGetKey(window, GLFW_KEY_2);
            const int k3 = glfwGetKey(window, GLFW_KEY_3);            // Special effects are processed by ProcessAugEffectQueue below.
            auto applyAug = [&](int slot) {
                if (slot < 0 || slot >= g_GameManager.augChoiceCount) return;
                const bool wasBuff = (g_GameManager.currentState == GameState::AUG_SELECT);
                const int selectedIdx = g_GameManager.augChoices[slot];
                if (selectedIdx < 0 || selectedIdx >= AUG_TOTAL) return;
                g_AugEffectQueue.push_back(selectedIdx);
                g_AugEffectProcessing = true;
                ProcessAugEffectQueue(screenWidth, screenHeight);
                if (g_GameManager.currentState == GameState::AUG_REPLACE)
                    return;
                g_AugEffectProcessing = false;
                g_GameManager.maxHP = g_Stats.maxHP;
                if (wasBuff) {
                    FinishAugmentEffects();
                } else {
                    ReturnToRunningAfterPick();
                }
            };

            constexpr float AUG_EXIT_DUR = 1.70f;  // 공전 흡수(0~0.65) + 슈퍼노바(0.65~1.20) + 여유

            // exit anim timer -- fires applyAug when done
            bool augExitFired = false;
            if (g_AugExitT >= 0.0f) {
                g_AugExitT += delta;
                if (g_AugExitT >= AUG_EXIT_DUR) {
                    applyAug(g_AugExitSlot);
                    g_HoveredAug  = -1;
                    g_AugExitT    = -1.0f;
                    g_AugExitSlot = -1;
                    s_augSpaceReleased      = false;
                    g_GameManager.spaceReleased = false;
                    augExitFired = true;
                }
            }

            if (!augExitFired) {
                // 1/2/3 focuses a card; SPACE starts the confirmation exit.
                if (g_AugExitT < 0.0f) {
                    if (k1 == GLFW_PRESS && g_aug1Released && g_GameManager.augChoiceCount > 0) {
                        g_HoveredAug = 0; g_GameManager.augmentKeyboardFocus = true;
                        g_aug1Released = false;
                    }
                    if (k2 == GLFW_PRESS && g_aug2Released && g_GameManager.augChoiceCount > 1) {
                        g_HoveredAug = 1; g_GameManager.augmentKeyboardFocus = true;
                        g_aug2Released = false;
                    }
                    if (k3 == GLFW_PRESS && g_aug3Released && g_GameManager.augChoiceCount > 2) {
                        g_HoveredAug = 2; g_GameManager.augmentKeyboardFocus = true;
                        g_aug3Released = false;
                    }
                }
                if (k1 == GLFW_RELEASE) g_aug1Released = true;
                if (k2 == GLFW_RELEASE) g_aug2Released = true;
                if (k3 == GLFW_RELEASE) g_aug3Released = true;

                // SPACE = confirm the focused constellation.
                int kSp = glfwGetKey(window, GLFW_KEY_SPACE);
                if (kSp == GLFW_PRESS && s_augSpaceReleased &&
                    g_HoveredAug >= 0 && g_HoveredAug < g_GameManager.augChoiceCount &&
                    g_AugExitT < 0.0f) {
                    g_AugExitT    = 0.0f;
                    g_AugExitSlot = g_HoveredAug;
                    s_augSpaceReleased = false;
                    g_GameManager.spaceReleased = false;
                }


            }
        }

        if (!debugInputCaptured && g_GameManager.currentState == GameState::AUG_REPLACE) {
            constexpr float REP_EXIT_DUR = 0.28f;
            static bool s_repEsc   = true;
            static bool s_repSpace = true;
            static bool s_repKeys[9] = { true,true,true,true,true,true,true,true,true };
            if (inputFocusChanged) {
                s_repEsc = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_RELEASE;
                s_repSpace = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE;
                for (int i = 0; i < 9; ++i)
                    s_repKeys[i] = glfwGetKey(window, GLFW_KEY_1 + i) == GLFW_RELEASE;
            }

            int kEsc   = glfwGetKey(window, GLFW_KEY_ESCAPE);
            int kSpRep = glfwGetKey(window, GLFW_KEY_SPACE);
            if (kEsc   == GLFW_RELEASE) s_repEsc   = true;
            if (kSpRep == GLFW_RELEASE) s_repSpace  = true;
            for (int ki = 0; ki < 9; ki++)
                if (glfwGetKey(window, GLFW_KEY_1 + ki) == GLFW_RELEASE) s_repKeys[ki] = true;

            bool repExitFired = false;
            if (g_RepExitT >= 0.0f) {
                g_RepExitT += delta;
                if (g_RepExitT >= REP_EXIT_DUR) {
                    if (g_RepExitSlot == -2)
                        CancelAugReplaceFlow();
                    else {
                        CompleteAugReplaceFlow(g_RepExitSlot, screenWidth, screenHeight);
                        g_HoveredAug = -1;
                    }
                    g_RepExitT    = -1.0f;
                    g_RepExitSlot = -3;
                    repExitFired  = true;
                }
            }

            if (!repExitFired && g_RepExitT < 0.0f) {
                if (kEsc == GLFW_PRESS && s_repEsc) {
                    g_RepExitT    = 0.0f;
                    g_RepExitSlot = -2;
                    s_repEsc = false;
                }
                if (kSpRep == GLFW_PRESS && s_repSpace && g_HoveredAug >= 0 &&
                    g_HoveredAug < g_GameManager.replaceChoiceCount) {
                    g_RepExitT    = 0.0f;
                    g_RepExitSlot = g_HoveredAug;
                    s_repSpace = false;
                }
                for (int ki = 0; ki < 9; ki++) {
                    if (ki >= g_GameManager.replaceChoiceCount) break;
                    if (glfwGetKey(window, GLFW_KEY_1 + ki) == GLFW_PRESS && s_repKeys[ki]) {
                        g_HoveredAug = ki;
                        g_GameManager.augmentKeyboardFocus = true;
                        s_repKeys[ki] = false;
                    }
                }
            }
        }

        // --- �� ���� (���� Ŭ���� ��) ---
        // --- ?�리?�이?�브 모드: F = 증강 그랩(?�버???�함 ?�드박스), G = 무적 ?��? ---
        if (!debugInputCaptured && (g_CreativeMode || g_DebugMode || kCompileDebugBuild)) {
            static bool s_fkeyReleased = true;
            int kF = glfwGetKey(window, GLFW_KEY_F);
            if (inputFocusChanged) s_fkeyReleased = kF == GLFW_RELEASE;
            if (kF == GLFW_RELEASE) s_fkeyReleased = true;
            if (kF == GLFW_PRESS && s_fkeyReleased &&
                g_GameManager.currentState == GameState::RUNNING) {
                // ?�드박스: ?�버?�도 카드 ?�???�어??무엇?�든 집을 ???�게
                const int oldLevel = g_GameManager.playerLevel;
                const XpAmount oldXp = g_GameManager.xp;
                ++g_GameManager.playerLevel;
                g_GameManager.xp = 0;
                GrantLevelUpReward(oldLevel, 0, oldXp, LevelUpSource::MANUAL);
                g_GameManager.ActivateNextAugmentReward();
                g_CreativeFreeGrab = false;
                s_fkeyReleased = false;
            }
            // G ??무적 ON/OFF ?��?
            static bool s_gkeyReleased = true;
            int kG = glfwGetKey(window, GLFW_KEY_G);
            if (inputFocusChanged) s_gkeyReleased = kG == GLFW_RELEASE;
            if (kG == GLFW_RELEASE) s_gkeyReleased = true;
            if (kG == GLFW_PRESS && s_gkeyReleased &&
                g_GameManager.currentState == GameState::RUNNING) {
                g_CreativeGodmode = !g_CreativeGodmode;
                s_gkeyReleased = false;
            }
        }

        // --- ?�트?�톱: ???�벤??직후 ?�깐 ?��??�이???��? (?�더??계속) ---
        if (g_HitStopTimer > 0.0f) g_HitStopTimer -= delta;

        // --- Level-up signal: real time; opens the selection when done ---
        {
            LevelUpTransition& transition = g_GameManager.levelUpTransition;
            const GameState st = g_GameManager.currentState;
            if (st == GameState::RUNNING) {
                if (transition.Update(delta))
                    g_GameManager.ActivateNextAugmentReward();  // none left: keep playing
            } else if (st != GameState::PAUSED) {
                transition.Cancel();   // death or leaving the run wins
            }
        }

        // --- Fixed timestep: speed comes from GameplayTime.h ---
        const float physDelta = ComputeGameplayDelta(delta, CurrentGameplayTime());
        accumulator += physDelta;
        while (accumulator >= FIXED_DT) {
            if (g_GameManager.ShouldUpdate()) {
                const bool playerControl = g_GameManager.currentState == GameState::RUNNING;
                if (!playerControl) {
                    g_PlayerRuntime.dashActive = false;
                }
                g_PlayerDmgMult = g_Stats.GetDamageTakenMult();
                const float zoomNow = (g_ViewZoom < 0.01f) ? 0.01f : g_ViewZoom;
                float mvX = 0.0f, mvY = 0.0f;
                const float mlen = UpdatePlayerMovement(
                    playerWin.x, playerWin.y,
                    keys[GLFW_KEY_W], keys[GLFW_KEY_S],
                    keys[GLFW_KEY_A], keys[GLFW_KEY_D],
                    playerControl, MOVE_SPEED * g_Stats.GetMoveMultiplier(lmb) *
                        (g_PlayerRuntime.moveSlowTimer > 0.0f ? GRAVIS_SLOW_MOVE_MULT : 1.0f),
                    FIXED_DT, g_PlayerRuntime.dashActive, mvX, mvY);
                if (playerControl && mlen > 0.001f && !g_PlayerRuntime.dashActive) {
                    // ?�동 ?�상(afterimage) ???�정 간격?�로 ?�레?�어 중심???��? ?�안 ?�상
                    static float s_trailAcc = 0.0f;
                    s_trailAcc += FIXED_DT;
                    if (s_trailAcc >= 0.028f) {
                        s_trailAcc = 0.0f;
                        SpawnTrail(playerWin.x + playerWin.width  * 0.5f,
                                   playerWin.y + playerWin.height * 0.5f,
                                   24.0f, 0.35f, 0.85f, 1.0f);
                    }
                }

                const PlayerViewportBounds viewport = GetPlayerViewportBounds(
                    (float)screenWidth, (float)screenHeight, zoomNow,
                    BROWSER_CHROME_H, g_GameBarH + (float)g_TaskbarH);
                ClampPlayerToViewport(playerWin.x, playerWin.y,
                                      playerWin.width, playerWin.height,
                                      viewport);
                const float ccX = viewport.centerX;
                const float ccY = viewport.centerY;
                const float halfW = viewport.halfWidth;
                const float halfH = viewport.halfHeight;
                const float topLimit = viewport.top;
                const float bottomLimit = viewport.bottom;
                float pCX = playerWin.x + playerWin.width * 0.5f;
                float pCY = playerWin.y + playerWin.height * 0.5f;

                // ?�?�??�티�??�킬 ??쿨다??지??갱신 + ?�력(Shift ?�??/ Q·E·R ?�롯) ?�?�?
                UpdatePlayerRuntimeTimers(
                    FIXED_DT, GravisField::DashRecoveryMult(GravisField::g_playerInfluence));
                g_SkillHudPulse.Update(GatherSkillHudInputs(), FIXED_DT);


                // �??�기 ???�레?�어 중심 ??�� (?�백 + ?�해)
                auto closeWindowBlast = [&](float cx, float cy) {
                    float dmg = g_Stats.GetBaseDamage() * g_Stats.GetDamageMultiplier() * 8.0f;
                    float rad = 380.0f, r2 = rad * rad, knock = 130.0f;
                    auto hitKB = [&](float& ex, float& ey, auto applyDamage) {
                        float dx = ex - cx, dy = ey - cy, d2 = dx*dx + dy*dy;
                        if (d2 < r2 && space::Reaches(space::kPlayerSource, ex, ey)) {
                            applyDamage();
                            float d = std::sqrt(d2)+1e-3f;
                            ex += dx/d*knock; ey += dy/d*knock;
                        }
                    };
                    for (auto m : g_MonsterManager.monsters) {
                        if (m->alive)
                            hitKB(m->worldX, m->worldY,
                                  [&]() { g_MonsterManager.ApplyDamage(*m, dmg); });
                    }
                    for (auto r : g_MonsterManager.rangedMobs) {
                        if (r->alive)
                            hitKB(r->worldX, r->worldY,
                                  [&]() { g_MonsterManager.ApplyDamage(*r, dmg); });
                    }
                    SpawnShockWave(cx, cy, rad*1.3f, 0.6f, 0.5f, 0.8f, 1.0f);
                    SpawnEnemyExplosion(cx, cy, 0.5f, 0.8f, 1.0f, true);
                    g_ShakeTime = 0.4f; g_ShakeMag = 20.0f;
                    TriggerFlash(0.5f, 0.8f, 1.0f, 0.45f); TriggerHitStop(0.07f);
                };

                // ?�롯 ?�킬 발동 ?�퍼
                auto useSkill = [&](int slot) {
                    if (slot < 0 || slot >= 3) return;
                    SkillSlot& s = g_Skills[slot];
                    if (s.type == SkillType::NONE || s.cd > 0.0f) return;
                    switch (s.type) {
                    case SkillType::CLOSE_WINDOW: closeWindowBlast(pCX, pCY); break;
                    case SkillType::HYPER_FOCUS:
                        g_PlayerRuntime.hyperFocusTimer = HYPER_FOCUS_DUR;
                        TriggerFlash(0.5f, 0.75f, 1.0f, 0.35f);
                        break;
                    case SkillType::TIME_STOP:    g_PlayerRuntime.timeStopTimer  = TIMESTOP_DUR;
                                                  TriggerFlash(0.4f,0.9f,1.0f,0.4f); break;
                    default: break;
                    }
                    s.cd = PlayerSkillCooldownMax(s.type);
                };

                // �÷��� ���?? ���� �̵� + ª�� ����
                if (playerControl && g_PlayerRuntime.dashActive) {
                    AdvancePlayerDash(FIXED_DT, pCX, pCY);
                    playerWin.x = pCX - playerWin.width * 0.5f;
                    playerWin.y = pCY - playerWin.height * 0.5f;
                }
                bool cDash = keys[GLFW_KEY_LEFT_SHIFT] || keys[GLFW_KEY_RIGHT_SHIFT];
                if (playerControl && cDash && !g_PlayerRuntime.dashInputHeld && g_PlayerRuntime.dashCooldown <= 0.0f && !g_PlayerRuntime.dashActive) {
                    float ddx = mvX, ddy = mvY;
                    if (mlen <= 0.001f) {
                        float ax = wmx - pCX, ay = wmy - pCY;
                        float al = std::sqrt(ax*ax+ay*ay)+1e-3f; ddx = ax/al; ddy = ay/al;
                    }
                    const float topLimit = ccY + (BROWSER_CHROME_H - ccY) / zoomNow;
                    BeginPlayerDash(pCX, pCY, ddx, ddy,
                                    ccX - halfW, ccX + halfW,
                                    topLimit, ccY + halfH);
                    TriggerFlash(0.35f, 0.85f, 1.0f, 0.12f);
                    SpawnShockWave(g_PlayerRuntime.dashFromX, g_PlayerRuntime.dashFromY, 70.0f, 0.25f, 0.4f, 1.0f, 1.0f);
                    if (g_Stats.dashUpgrade) {
                        g_PlayerRuntime.dashBoostShotsLeft = 3;
                        const int burstN = 3 + rand() % 3;
                        for (int bi = 0; bi < burstN; bi++) {
                            float a = (float)bi / (float)burstN * 2.0f * (float)M_PI
                                    + ((float)(rand() % 100) - 50.0f) * 0.01f;
                            Bullet nb(pCX, pCY,
                                      pCX + cosf(a) * 120.0f,
                                      pCY + sinf(a) * 120.0f);
                            nb.speed      = g_Stats.bulletSpeed * 1.1f;
                            nb.color      = glm::vec3(0.45f, 0.95f, 1.0f);
                            nb.homing     = true;
                            nb.homingTurn = 6.0f;
                            nb.dmgMult    = 0.55f;
                            nb.launchRamp  = 0.08f;
                            nb.launchAccel = 1.4f;
                            g_Bullets.push_back(nb);
                        }
                    }
                }
                g_PlayerRuntime.dashInputHeld = cDash;
                const int skillKeys[3] = { GLFW_KEY_Q, GLFW_KEY_E, GLFW_KEY_R };
                for (int i = 0; i < 3; ++i) {
                    const bool held = keys[skillKeys[i]];
                    if (playerControl && held && !g_PlayerRuntime.skillInputHeld[i])
                        useSkill(i);
                    g_PlayerRuntime.skillInputHeld[i] = held;
                }
                // C16: ?�티�??�킬 ?�동 ?�용 ??쿨다???�난 ?�롯???�동 발동
                if (playerControl && g_AutoSkill) for (int i = 0; i < 3; i++) useSkill(i);

                // Apply each station once per fixed step.
                GravisField::Apply(g_MonsterManager, g_Bullets, pCX, pCY,
                                   playerControl, g_PlayerRuntime.dashActive,
                                   MOVE_SPEED * g_Stats.GetMoveMultiplier(lmb), FIXED_DT);
                AdvancePlayerKnockback(FIXED_DT, pCX, pCY);
                pCX = std::max(ccX - halfW, std::min(ccX + halfW, pCX));
                pCY = std::max(ccY + (BROWSER_CHROME_H - ccY) / zoomNow,
                               std::min(bottomLimit, pCY));
                playerWin.x = pCX - playerWin.width * 0.5f;
                playerWin.y = pCY - playerWin.height * 0.5f;

                UpdateProjectiles(g_Bullets, g_MonsterManager, playerWin,
                                  FIXED_DT, g_PlayerRuntime.timeStopTimer,
                                  g_PlayerRuntime.hyperFocusTimer,
                                  (float)screenWidth, (float)screenHeight,
                                  g_ViewZoom);

                // ?�??무적 ???�번 ?�텝 ?�작 HP ?�??(???�해??무효, ?�복?�??��?)
                float hpAtStep = g_GameManager.playerHP;
                TickPlayerShield(FIXED_DT);
                //   ???��? = ?�간?��? ?�킬 OR 증강 ??직후 ~0.05s("?�이 ?�?, 짧게)
                bool  timeStopped = (g_PlayerRuntime.timeStopTimer > 0.0f) || (g_PlayerRuntime.postPickGrace > 0.45f);
                float enemyDt = FIXED_DT;
                if (!timeStopped && g_PlayerRuntime.hyperFocusTimer > 0.0f) enemyDt *= 0.7f;
                float focusSlow = (g_PlayerRuntime.hyperFocusTimer > 0.0f) ? 0.7f : 1.0f;

                // 몬스???�데?�트 (?�버??multiplier ?�용) ???�간 ?��? 중엔 ??멈춤
                float rmobMoveMult = 1.0f / g_Stats.rmobDelayMult; // <1 ????빠름
                // Time-based speed ramp. Density is lower now, so enemy pressure should not depend
                // only on score gained from kills.
                float mobSpdRamp = EnemySpeedRamp(g_GameTime);
                // Ư���� ? �� AI ���� �����衤�Ұ�, AI �� �÷��̾� �о��
                if (!timeStopped && g_Stats.chakram && g_Stats.chakramSingularity) {
                    const float pullR2 = 220.0f * 220.0f;
                    const float winSz = g_WindowSizeCur > 1.0f ? g_WindowSizeCur : g_Stats.windowSize;
                    const float safeR  = winSz * 0.52f;
                    const float safeR2 = safeR * safeR;
                    for (int c = 0; c < g_Stats.chakramCount && c < MAX_CHAKRAMS; c++) {
                        if (!g_Chakrams[c].alive) continue;
                        float chx = pCX + cosf(g_Chakrams[c].angle) * CHAKRAM_RADIUS;
                        float chy = pCY + sinf(g_Chakrams[c].angle) * CHAKRAM_RADIUS;
                        for (auto m : g_MonsterManager.monsters) {
                            if (!m->alive) continue;
                            if (!space::Reaches(space::kPlayerSource, m->worldX, m->worldY)) continue;
                            float ddx = chx - m->worldX, ddy = chy - m->worldY;
                            float d2 = ddx * ddx + ddy * ddy;
                            float pdx = m->worldX - pCX, pdy = m->worldY - pCY;
                            float pd2 = pdx * pdx + pdy * pdy;
                            if (d2 < pullR2) {
                                m->singularityGrace = 0.14f;
                                if (d2 > 900.0f && pd2 > safeR2) {
                                    float d = std::sqrt(d2) + 1e-3f;
                                    m->worldX += (ddx / d) * 300.0f * FIXED_DT;
                                    m->worldY += (ddy / d) * 300.0f * FIXED_DT;
                                }
                                g_MonsterManager.ApplyDamage(
                                    *m, 100.0f * FIXED_DT);
                            }
                        }
                    }
                }
                if (!timeStopped) {
                    g_MonsterManager.UpdateAll(pCX, pCY, enemyDt,
                                               g_GameManager.playerHP, g_Bullets,
                                               g_Stats.mobSpeedMult * mobSpdRamp * focusSlow,
                                               rmobMoveMult * mobSpdRamp * focusSlow,
                                               g_Stats.rotorHpMult,
                                               -1.0f, -1.0f, -1.0f, -1.0f,
                                               glm::vec4(ccX - halfW, topLimit,
                                                         ccX + halfW, bottomLimit));
                    // Held kill stardust (Quasar supernova) comes out at the blast.
                    ReleasePendingStardust(g_MonsterManager, pCX, pCY);

                    // Gravis' arrival burst slows the player; dash i-frames ignore it.
                    for (auto* m : g_MonsterManager.monsters) {
                        if (!m || m->kind != MobKind::GRAVIS || m->gravisSlowRequest <= 0.0f) continue;
                        if (g_PlayerRuntime.dashInvulnerability <= 0.0f)
                            g_PlayerRuntime.moveSlowTimer = std::max(
                                g_PlayerRuntime.moveSlowTimer, m->gravisSlowRequest);
                        m->gravisSlowRequest = 0.0f;
                    }

                    // Antares rams shove the player; dash i-frames ignore the shove.
                    // A run that hits the map edge stuns it there instead.
                    for (auto* m : g_MonsterManager.monsters) {
                        if (m) m->ConfineAntares(viewport.left, viewport.top,
                                                 viewport.right, viewport.bottom);
                        if (m) m->ConfineNormaDash(viewport.left, viewport.top,
                                                   viewport.right, viewport.bottom);
                        if (!m || m->kind != MobKind::ANTARES ||
                            (m->antaresKnockX == 0.0f && m->antaresKnockY == 0.0f))
                            continue;
                        if (g_PlayerRuntime.dashInvulnerability <= 0.0f)
                            ApplyPlayerKnockback(m->antaresKnockX, m->antaresKnockY);
                        m->antaresKnockX = m->antaresKnockY = 0.0f;
                    }
                    // NORMA's charges shove the same way.
                    for (auto* m : g_MonsterManager.monsters) {
                        if (!m || m->kind != MobKind::NORMA ||
                            (m->normaKnockX == 0.0f && m->normaKnockY == 0.0f))
                            continue;
                        if (g_PlayerRuntime.dashInvulnerability <= 0.0f)
                            ApplyPlayerKnockback(m->normaKnockX, m->normaKnockY);
                        m->normaKnockX = m->normaKnockY = 0.0f;
                    }

                    // Large controller enemies must remain readable on-screen.
                    // Reflect their free-drift heading when the safe viewport is
                    // reached so they do not idle beyond the visible arena.
                    for (auto* m : g_MonsterManager.monsters) {
                        if (!m || !m->alive ||
                            (m->kind != MobKind::GRAVIS && m->kind != MobKind::QUASAR)) continue;
                        const float margin = m->kind == MobKind::GRAVIS ? 92.0f : 76.0f;
                        const float minX = margin;
                        const float maxX = std::max(minX, (float)screenWidth - margin);
                        const float minY = margin;
                        const float maxY = std::max(minY, (float)screenHeight - margin);
                        const bool hitX = m->worldX < minX || m->worldX > maxX;
                        const bool hitY = m->worldY < minY || m->worldY > maxY;
                        m->worldX = std::max(minX, std::min(maxX, m->worldX));
                        m->worldY = std::max(minY, std::min(maxY, m->worldY));
                        if (m->kind == MobKind::GRAVIS) {
                            if (hitX) m->gravisDriftAngle = 3.14159265f - m->gravisDriftAngle;
                            if (hitY) m->gravisDriftAngle = -m->gravisDriftAngle;
                        } else {
                            if (hitX) m->quasarMoveAngle = 3.14159265f - m->quasarMoveAngle;
                            if (hitY) m->quasarMoveAngle = -m->quasarMoveAngle;
                        }
                    }
                }

                // Static Field is an area weapon with a real fixed-step pulse.
                // It supplements the loadout's short-range field identity and
                // gives the weapon a reliable combat effect instead of leaving
                // the field geometry as a purely cosmetic shell.
                if (g_CurrentWeapon == (int)StartWeapon::SMG && playerControl) {
                    g_StaticFieldPulseTimer -= FIXED_DT;
                    if (g_StaticFieldPulseTimer <= 0.0f && !timeStopped) {
                        const float radius2 = STATIC_FIELD_PULSE_RADIUS * STATIC_FIELD_PULSE_RADIUS;
                        const float pulseDamage = g_Stats.GetBaseDamage()
                            * g_Stats.GetDamageMultiplier() * STATIC_FIELD_PULSE_DAMAGE_MULT;
                        auto pulseTarget = [&](float tx, float ty, auto& target) {
                            if (!target.alive) return;
                            const float dx = tx - pCX, dy = ty - pCY;
                            if (dx * dx + dy * dy > radius2) return;
                            const float dealt = g_MonsterManager.ApplyDamage(
                                target, pulseDamage);
                            SpawnDamageNumber(tx, ty, dealt, false);
                        };
                        for (auto* m : g_MonsterManager.monsters)
                            pulseTarget(m->worldX, m->worldY, *m);
                        for (auto* r : g_MonsterManager.rangedMobs)
                            pulseTarget(r->worldX, r->worldY, *r);
                        g_StaticFieldPulseTimer += STATIC_FIELD_PULSE_INTERVAL;
                    }
                } else {
                    g_StaticFieldPulseTimer = 0.0f;
                }
                if (!timeStopped && g_Stats.chakram && g_Stats.chakramSingularity) {
                    const float winSz = g_WindowSizeCur > 1.0f ? g_WindowSizeCur : g_Stats.windowSize;
                    const float safeR  = winSz * 0.52f;
                    const float safeR2 = safeR * safeR;
                    for (auto m : g_MonsterManager.monsters) {
                        if (!m->alive) continue;
                        float pdx = m->worldX - pCX, pdy = m->worldY - pCY;
                        float pd2 = pdx * pdx + pdy * pdy;
                        if (pd2 >= safeR2) continue;
                        float pd = std::sqrt(pd2) + 1e-3f;
                        float push = (safeR - pd + 20.0f) * 16.0f;
                        m->worldX += (pdx / pd) * push * FIXED_DT;
                        m->worldY += (pdy / pd) * push * FIXED_DT;
                        m->singularityGrace = 0.16f;
                    }
                }

                // 리로???�너 ?�데?�트 (무기 ?�태머신 + ?�전 질주, ??총알 push)
CollisionSystem::Update(pCX, pCY,
                    g_MonsterManager, g_Bullets,
                    g_GameManager.playerHP,
                    g_GameManager.scoreAccum, g_GameManager.score,
                    g_Stats);
                // Apply after crowding, pulls and weapon knockback so these
                // enemies remain inside the camera's zoom-adjusted viewport.
                const float viewportLeft = ScreenToWorldX(0.0f);
                const float viewportTop = ScreenToWorldY(0.0f);
                const float viewportRight = ScreenToWorldX((float)screenWidth);
                const float viewportBottom = ScreenToWorldY((float)screenHeight);
                for (auto* m : g_MonsterManager.monsters)
                    if (m->alive) {
                        m->ConstrainGenesisToViewport(viewportLeft, viewportTop,
                                                      viewportRight, viewportBottom);
                        m->ConstrainGimbalToViewport(viewportLeft, viewportTop,
                                                     viewportRight, viewportBottom);
                        m->ConstrainRegulusToViewport(viewportLeft, viewportTop,
                                                      viewportRight, viewportBottom);
                    }
                if (g_Bullets.size() > 2800) {
                    g_Bullets.erase(
                        std::remove_if(g_Bullets.begin(), g_Bullets.end(),
                            [](const Bullet& b){ return !b.active; }),
                        g_Bullets.end());
                    if (g_Bullets.size() > 2800)
                        g_Bullets.erase(g_Bullets.begin() + 2800, g_Bullets.end());
                }
                // ?�??무적 / 증강???�예(C14) ???�번 ?�텝?????�해 무효 (?�복?�??��?)
                ApplyPlayerDamageProtection(
                    hpAtStep, g_GameManager.playerHP,
                    IsPlayerDamageProtected(),
                    g_Stats.lightStep, g_Stats.lightStepHitLock,
                    g_Stats.lightStepDisableTimer);

                // Regular enemy kill rewards (KillRewards.h).
                KillRewardSink killSink{ g_GameManager.playerHP, g_GameManager.scoreAccum,
                                         g_GameManager.score, g_Stats, pCX, pCY };

                // ?�망 ??�� spawn
                //   + 분열�??�망 ???��? ?�식 2마리 (�?2?��?까�?)
                for (auto m : g_MonsterManager.monsters) {
                    if (!m->alive && !m->exploded) {
                        CollectMonsterKill(killSink, *m);
                        m->SpawnDeathEffect((float)now);
                        TriggerEliteKillHitStop(*m);
                        // ??��???�리????죽을 ???�져 ?�레?�어?�게 광역 ?�해
                        // ?�쇄 ??��(DEATH_BLAST) ???�망 ?�치?�서 주�? ?�에�?AoE
                        //   ?�프: ?��?지 0.8??.3 + ??���?죽�? 몹�? ?�시 ???�짐(무한?�쇄 차단)
                        // Genesis deaths emit particles only, including when
                        // the player's chain-explosion augment is equipped.
                        if (g_Stats.deathBlast && !m->noBlast &&
                            m->kind != MobKind::GENESIS) {
                            float blastDmg = g_Stats.GetBaseDamage()
                                           * g_Stats.GetDamageMultiplier() * g_Stats.deathBlastDmgPct;
                            float blastR = 130.0f * g_Stats.deathBlastMult;
                            float bx = m->worldX, by = m->worldY;
                            SpawnShockWave(bx, by, blastR, 0.35f, 1.0f, 0.55f, 0.15f);
                            for (auto m2 : g_MonsterManager.monsters) {
                                if (!m2->alive || m2 == m) continue;
                                float ddx = m2->worldX - bx, ddy = m2->worldY - by;
                                if (ddx*ddx + ddy*ddy < blastR*blastR) {
                                    g_MonsterManager.ApplyDamage(*m2, blastDmg);
                                    if (!m2->alive) m2->noBlast = true;
                                }
                            }
                            for (auto r2 : g_MonsterManager.rangedMobs) {
                                if (!r2->alive) continue;
                                float ddx = r2->worldX - bx, ddy = r2->worldY - by;
                                if (ddx*ddx + ddy*ddy < blastR*blastR) {
                                    g_MonsterManager.ApplyDamage(*r2, blastDmg);
                                }
                            }
                        }
                    }
                }
                for (auto r : g_MonsterManager.rangedMobs) {
                    if (!r->alive && !r->exploded) {
                        CollectScopeKill(killSink, *r);
                        r->SpawnDeathEffect();
                    }
                }
                    const bool atMainCap = !g_CreativeMode
                                          && g_GameManager.playerLevel >= MAIN_LEVEL_CAP;
                    if (atMainCap) {
                        g_GameManager.xp = 0;
                    } else {
                    // Consume every level threshold available in the same
                    // fixed tick.  A large pickup must not leave the player
                    // permanently one level behind or discard XP.
                    constexpr int kMaxLevelUpsPerTick = 64;
                    int levelUps = 0;
                    // atMainCap is only the state before this tick; a large
                    // XP gain must still stop at the cap on the way up.
                    while (levelUps++ < kMaxLevelUpsPerTick &&
                           (g_CreativeMode ||
                            g_GameManager.playerLevel < MAIN_LEVEL_CAP)) {
                        const int oldLevel = g_GameManager.playerLevel;
                        const XpAmount oldXp = g_GameManager.xp;
                        if (!g_ExpSystem.ConsumeNextLevel(g_GameManager.xp,
                                                          g_GameManager.playerLevel)) break;
                        GrantLevelUpReward(oldLevel, oldXp - g_GameManager.xp,
                                           oldXp, LevelUpSource::EXPERIENCE);
                    }
                    } // !atMainCap

                // �ð� ���� (�޽ġ����� �߿��� ����)
                g_GameManager.AddScore(FIXED_DT * 100.0f);
            }
            accumulator -= FIXED_DT;
        }

        g_GameplayTelemetry.Update(
            g_GameTime,
            g_GameManager.playerLevel,
            g_GameManager.xp,
            g_ExpSystem.RequiredAmount(g_GameManager.playerLevel),
            g_Stats);

        // ?�?�??�맛: ?��?지 ?�자 / 콤보 / ?�래??�??�레??갱신 (게임 진행 중에�?감쇠) ?�?�?
        if (g_GameManager.currentState == GameState::RUNNING ||
            g_GameManager.currentState == GameState::DYING) {
            for (auto& d : g_DmgNumbers) {
                d.x  += d.vx * delta;
                d.y  += d.vy * delta;
                d.vy += 70.0f * delta;        // ?�한 중력 (?�구쳤다 처짐)
                d.life -= delta;
            }
            g_DmgNumbers.erase(std::remove_if(g_DmgNumbers.begin(), g_DmgNumbers.end(),
                [](const DamageNumber& d){ return d.life <= 0.0f; }), g_DmgNumbers.end());
            if (g_ComboTimer > 0.0f) {
                g_ComboTimer -= delta;
                if (g_ComboTimer <= 0.0f) g_Combo = 0;
            }
            g_ComboPulse -= delta * 4.0f;
            if (g_ComboPulse < 0.0f) g_ComboPulse = 0.0f;
            if (g_ComboMilestone > 0.0f) g_ComboMilestone -= delta;
        }
        if (g_FlashIntensity > 0.0f) {
            g_FlashIntensity -= delta * 3.5f;
            if (g_FlashIntensity < 0.0f) g_FlashIntensity = 0.0f;
        }
        if (g_GameManager.waveDebuffToast > 0.0f)
            g_GameManager.waveDebuffToast = std::max(
                0.0f, g_GameManager.waveDebuffToast - delta);
        if (g_GameManager.currentState == GameState::RUNNING &&
            g_GameManager.waveTransitionTimer > 0.0f) {
            g_GameManager.waveTransitionTimer = std::max(
                0.0f, g_GameManager.waveTransitionTimer - delta);
        }

        {
            GameState ws = g_GameManager.currentState;
            if (ws == GameState::RUNNING || ws == GameState::PAUSED ||
                ws == GameState::READY  || ws == GameState::AUG_SELECT ||
                ws == GameState::DEBUFF_SELECT || ws == GameState::DYING) {
                SyncPlayerBoundsSize(playerWin, delta, ws == GameState::RUNNING);
            }
        }

        // Detect damage for the hit feedback. Non-combat screens synchronize
        // immediately so upgrades or resets cannot be mistaken for damage.
        {
            const GameState hpState = g_GameManager.currentState;
            const bool hpAnimActive = hpState == GameState::RUNNING ||
                                      hpState == GameState::DYING;
            const float hpNow = std::max(0.0f, g_GameManager.playerHP);
            const float hpNowFrac = std::clamp(hpNow / std::max(1.0f, g_Stats.maxHP), 0.0f, 1.0f);
            if (!hpAnimActive) {
                g_WinPrevHP = hpNow;
                g_HpGhost.Reset(hpNowFrac);
            } else {
                g_HpGhost.Update(hpNowFrac, delta);
                if (g_WinPrevHP < 0.0f) {
                    g_WinPrevHP = hpNow;
                }

                const float hpBefore = g_WinPrevHP;
                const float hpDelta = hpNow - hpBefore;
                if (g_PlayerDamagePulse > 0.0f) {
                    g_HpBarPop = std::max(g_HpBarPop, g_PlayerDamagePulse);
                    g_PlayerDamagePulse = 0.0f;
                }
                if (hpDelta < -0.0001f) {
                    g_HurtVignette = 0.5f;
                    g_HpBarPop = 2.2f;
                }
                g_WinPrevHP = hpNow;

                if (g_HurtVignette > 0.0f) {
                    g_HurtVignette -= delta * 1.6f;
                    if (g_HurtVignette < 0.0f) g_HurtVignette = 0.0f;
                }
                if (g_HpBarPop > 0.0f) {
                    g_HpBarPop -= delta;
                    if (g_HpBarPop < 0.0f) g_HpBarPop = 0.0f;
                }
            }
        }

        if (g_GameManager.currentState == GameState::RUNNING) {
            float pCX = playerWin.x + playerWin.width  * 0.5f;
            float pCY = playerWin.y + playerWin.height * 0.5f;
            bool  moving = keys[GLFW_KEY_W] || keys[GLFW_KEY_S] ||
                           keys[GLFW_KEY_A] || keys[GLFW_KEY_D];

            // ?�?�??�격 ?��???HP 감소 ??빨간 비네?�만 (?�야 변???�거, �??�기 고정) ?�?�?
            // HP ���?(REGEN_UP, �Ŵ�ȭ, ���?II �� regenPerSec �ջ�)
            if (g_Stats.regenPerSec > 0.0f) {
                g_GameManager.playerHP += g_Stats.GetRegenRate(g_GameManager.playerHP) * delta;
                if (g_GameManager.playerHP > g_Stats.maxHP)
                    g_GameManager.playerHP = g_Stats.maxHP;
            }
            // 배드 ?�터 출혈 ??감속 구역 ?�이�?출혈 ?�?�머 2초로 갱신, 빠져?��????�류


            // ?�감 발견 ???�거�??�폭�?(존재?�면 발견 처리)
            for (const auto* ranged : g_MonsterManager.rangedMobs)
                if (ranged->alive)
                    MarkMobSeenId(CM_SCOPE);

            // ?�적 조건 체크 (???�수/??보유 증강 기�? ??보스 ?�적?�?처치 ?�점?�서 처리)
            if (PlayRecordsProgress()) {
                long long runScore = g_GameManager.score;
                long long runKills = g_Stats.killCount;
                if (runScore >= 300000)  TryUnlockAch(ACH_SCORE_300K);
                if (runScore >= 500000)  TryUnlockAch(ACH_SCORE_500K);
                if (runScore >= 1000000) TryUnlockAch(ACH_SCORE_1M);
                if (runKills >= 500)     TryUnlockAch(ACH_KILLS_500);
                if (g_Stats.critChance > 0 && runScore >= 200000)
                    TryUnlockAch(ACH_CRIT_SCORE);
                if (g_Stats.deathBlast && runKills >= 300)
                    TryUnlockAch(ACH_DEATHBLAST_KILLS);
                int debuffCnt = 0;
                for (int oi : g_OwnedAugs)
                    if (ALL_AUGS[oi].rarity == AugRarity::DEBUFF) ++debuffCnt;
                if (debuffCnt >= 5) TryUnlockAch(ACH_DEBUFF_5);
            }

            UpdateEnemyFx(delta);
            g_MonsterManager.UpdateDeathEffects(delta);

            // 충격???�데?�트
            for (auto& sw : g_ShockWaves) {
                if (!sw.active) continue;
                sw.life -= delta;
                if (sw.life <= 0.0f) sw.active = false;
            }

            // 검�??�윙 ?�상 ?�데?�트

            for (auto& lb : g_LaserBeams) lb.life -= delta;
            g_LaserBeams.erase(std::remove_if(g_LaserBeams.begin(), g_LaserBeams.end(),
                [](const LaserBeam& b){ return b.life <= 0.0f; }), g_LaserBeams.end());

            // 별가루: 스폰 직후 약간 퍼진 뒤 매 프레임 플레이어 방향으로 재조향 — 무조건 수집.
            g_StardustHudPulse = std::max(0.0f, g_StardustHudPulse - delta * 3.8f);
            constexpr float STARDUST_SCATTER_SECONDS = 0.40f;
            constexpr float STARDUST_MIN_HOMING_SECONDS = 0.18f;
            constexpr float STARDUST_PICKUP_RADIUS = 24.0f;
            for (auto& dust : g_StardustPickups) {
                if (!dust.alive) continue;
                dust.age += delta;

                // 초기 0.4s: 퍼짐 (초기 vx/vy 방향 유지하되 감속)
                if (dust.age < STARDUST_SCATTER_SECONDS) {
                    const float drag = 1.0f - delta * 4.5f;
                    dust.vx *= drag;
                    dust.vy *= drag;
                } else {
                    // 이후: 매 프레임 플레이어 방향으로 속도 재조향 (무조건 추적)
                    const float speed = 520.0f + std::min(280.0f, dust.age * 400.0f);
                    const float dx = pCX - dust.x, dy = pCY - dust.y;
                    const float dist = sqrtf(dx * dx + dy * dy);
                    if (dist > 0.001f) {
                        dust.vx = (dx / dist) * speed;
                        dust.vy = (dy / dist) * speed;
                    }
                }

                dust.x += dust.vx * delta;
                dust.y += dust.vy * delta;

                // Don't collect contact-range drops during scatter or on the
                // first homing frame; let the return flight remain visible.
                const float dx = pCX - dust.x, dy = pCY - dust.y;
                const float dist = sqrtf(dx * dx + dy * dy);
                if (dust.age >= STARDUST_SCATTER_SECONDS +
                                STARDUST_MIN_HOMING_SECONDS &&
                    dist < STARDUST_PICKUP_RADIUS) {
                    GrantExperience(dust.xpValue);
                    if (PlayRecordsProgress()) {
                        AddSaveValue(g_Coins, dust.value);
                        AddSaveValue(g_RunStardust, dust.value);
                    }
                    g_StardustHudPulse = 1.0f;
                    SpawnSparks(dust.x, dust.y, dust.value >= 5 ? 6 : 3,
                                1.0f, 0.85f, 0.30f, 180.0f);
                    dust.alive = false;
                }
            }
            g_StardustPickups.erase(std::remove_if(g_StardustPickups.begin(), g_StardustPickups.end(),
                [](const StardustPickup& d) { return !d.alive || d.age > 6.0f; }),
                g_StardustPickups.end());

            // ?��??�파???�데?�트 (?�동 + 감속 + ?�명)
            for (auto& sp : g_Sparks) {
                sp.x += sp.vx * delta;
                sp.y += sp.vy * delta;
                float drag = std::max(0.0f, 1.0f - 6.0f * delta);
                sp.vx *= drag; sp.vy *= drag;
                sp.life -= delta;
            }
            g_Sparks.erase(std::remove_if(g_Sparks.begin(), g_Sparks.end(),
                [](const Spark& s){ return s.life <= 0.0f; }), g_Sparks.end());
            // ?�동 ?�상 ?�명 갱신
            for (auto& tr : g_Trail) tr.life -= delta;
            g_Trail.erase(std::remove_if(g_Trail.begin(), g_Trail.end(),
                [](const Trail& t){ return t.life <= 0.0f; }), g_Trail.end());
            if (g_MuzzleTimer > 0.0f) g_MuzzleTimer -= delta;

            if (g_ShakeTime > 0.0f) g_ShakeTime -= delta;

            if (g_Stats.lightStepDisableTimer > 0.0f)
                g_Stats.lightStepDisableTimer -= delta;


            UpdateEnemySpawns(g_MonsterManager, g_Stats,
                              screenWidth, screenHeight, delta,
                              g_GameTime, g_GameManager.score, g_Difficulty);
            g_GameTime += delta;
            ProcessWaveTransitions(screenWidth, screenHeight);
            TickNormaJudgment(delta);
            if (NormaDefeated()) CompleteBossRun(FinalWaveCount(g_Difficulty));
            float approachSpeed = 120.0f *
                (1.0f + 0.20f * (float)(g_Stats.approachStacks > 0 ? g_Stats.approachStacks - 1 : 0));
            for (auto& orb : g_ApproachOrbs) {
                float dx = pCX - orb.x;
                float dy = pCY - orb.y;
                float d  = sqrtf(dx*dx + dy*dy);
                if (d > 1.0f) {
                    orb.x += (dx/d) * approachSpeed * delta;
                    orb.y += (dy/d) * approachSpeed * delta;
                }
                if (d < 25.0f) {
                    g_GameManager.playerHP -= 20.0f * delta;
                }
            }

            // ?�론: ?�레?�어 주위 공전 + ?�동 발사 (?�력�?50%), 1~2�?            // ?�탑 모드 ?�성 ???�론?�?공전/발사 ?�고 ?�탑?�로 ?�체??
            if (g_Stats.drone) {
                for (int d = 0; d < g_Stats.droneCount && d < MAX_DRONES; d++) {
                    auto& dr = g_Drones[d];
                    dr.angle += 1.5f * delta;
                    // 균등 각도 ?�프??(각자 ?�른 ?�치)
                    float ang = dr.angle + (float)d * 6.2831853f / (float)g_Stats.droneCount;
                    float droneX = pCX + cosf(ang) * 80.0f;
                    float droneY = pCY + sinf(ang) * 80.0f;
                    dr.fireTimer += delta;
                    // 군집 지???�화) ??발사 간격 2.0× ??0.6× (초고??
                    float droneInt = g_Stats.fireInterval * (g_Stats.droneRapid ? 0.6f : 2.0f);
                    if (dr.fireTimer >= droneInt) {
                        float tx = 0.0f, ty = 0.0f;
                        if (g_MonsterManager.FindNearestEnemy(
                                droneX, droneY, tx, ty)) {
                            Bullet nb(droneX, droneY, tx, ty);
                            nb.speed = g_Stats.bulletSpeed * 0.5f;
                            // Hive keeps four orbiters, but each projectile
                            // contributes half of the normal rifle damage.
                            nb.dmgMult = 0.5f;
                            nb.color = glm::vec3(0.2f, 0.9f, 1.0f);
                            g_Bullets.push_back(nb);
                        }
                        dr.fireTimer = 0.0f;
                    }
                }
            }

            //   1초마???�레?�어 ?�치???�탑 1�?배치, �?5�?지????맵에 ~5�??�시

            // 차크?? 공전 + ?�몹 즉사 + ??총알 충돌 + ?�생??(n �?
            if (g_Stats.chakram) {
                for (int c = 0; c < g_Stats.chakramCount && c < MAX_CHAKRAMS; c++) {
                    auto& ch = g_Chakrams[c];
                    if (ch.alive) {
                        ch.angle += 5.5f * delta;
                        float chx = pCX + cosf(ch.angle) * CHAKRAM_RADIUS;
                        float chy = pCY + sinf(ch.angle) * CHAKRAM_RADIUS;
                        float hitR2 = CHAKRAM_SIZE * CHAKRAM_SIZE;
                        // Ư���� ? ������/�Ұ��� ����ƽ(�� AI ����). ���⼱ ��ũ�� ������.
                        // ���?���� ���?(Ư���� ���� ��)
                        if (!g_Stats.chakramSingularity) {
                        for (auto m : g_MonsterManager.monsters) {
                            if (!m->alive) continue;
                            float ddx = m->worldX - chx, ddy = m->worldY - chy;
                            if (ddx*ddx + ddy*ddy < hitR2) {
                                // The boss takes steady damage instead of the
                                // instant kill ordinary mobs get.
                                g_MonsterManager.ApplyDamage(*m, m->kind == MobKind::NORMA
                                    ? g_Stats.GetBaseDamage() * g_Stats.GetDamageMultiplier() *
                                          3.0f * delta
                                    : m->hp);
                                ch.hp -= 2.0f;
                            }
                        }
                        }
                        // ������
                        // ?�거�?�??�촉 ?????�해
                        for (auto rr : g_MonsterManager.rangedMobs) {
                            if (!rr->alive) continue;
                            float ddx = rr->worldX - chx, ddy = rr->worldY - chy;
                            if (ddx*ddx + ddy*ddy < hitR2) {
                                g_MonsterManager.ApplyDamage(*rr, 120.0f);
                                ch.hp -= 3.0f;
                            }
                        }
                        // ??총알 막기
                        for (auto& bb : g_Bullets) {
                            if (!bb.active || !bb.isEnemy) continue;
                            float ddx = bb.x - chx, ddy = bb.y - chy;
                            if (ddx*ddx + ddy*ddy < hitR2) {
                                ch.hp -= 6.0f;
                                bb.active = false;
                            }
                        }
                        if (ch.hp <= 0.0f) {
                            ch.alive = false;
                            ch.respawnTimer = 6.0f;
                        }
                    } else {
                        ch.respawnTimer -= delta;
                        if (ch.respawnTimer <= 0.0f) {
                            ch.alive = true;
                            if (ch.maxHp < 1.0f) ch.maxHp = 150.0f;
                            ch.hp    = ch.maxHp;
                        }
                    }
                }
            }

            if (g_Stats.bulletRain) {
                g_BulletRainTimer += delta;
                // ���� ����(��ȭ): óġ���� ��ٿ�?���� 0.2�� ����. �ּ� 3�� ���� ����.
                if (g_Stats.rainKillReduce && g_RainKillAccum > 0.0f) {
                    float killBoost = g_RainKillAccum * 0.2f;
                    float maxBoost = g_Stats.bulletRainCooldown - 3.0f - g_BulletRainTimer;
                    if (maxBoost < 0.0f) maxBoost = 0.0f;
                    if (killBoost > maxBoost) killBoost = maxBoost;
                    g_BulletRainTimer += killBoost;
                }
                g_RainKillAccum = 0.0f;
                if (g_BulletRainTimer >= g_Stats.bulletRainCooldown) {
                    g_BulletRainTimer = 0.0f;
                    const int N = 20;
                    for (int i = 0; i < N; i++) {
                        float a = (float)i / (float)N * 2.0f * (float)M_PI
                                + ((float)(rand() % 100) - 50.0f) * 0.005f;
                        Bullet nb(pCX, pCY,
                                  pCX + cosf(a) * 100.0f,
                                  pCY + sinf(a) * 100.0f);
                        nb.speed      = g_Stats.bulletSpeed * 1.05f;  // 최고??가???? ??SAM 미사?�식
                        nb.color      = glm::vec3(1.0f, 0.5f, 0.2f);
                        nb.homing     = true;
                        nb.homingTurn = 5.0f;   // rad/s
                        nb.dmgMult    = 0.5f;
                        // ���� �̻��� ? �߻� ���� 5%���� ������ ~0.7�ʿ� 100% ����
                        nb.launchRamp  = 0.05f;
                        nb.launchAccel = 1.35f;
                        // (?�환?��? ??기존 ?�반 ?�환 ?�더�?복원. 로켓 ?�프?�이??미사??
                        g_Bullets.push_back(nb);
                    }
                }
            }



            // Fire bullets and cache the active damage modifiers.
            // ?�탑 모드: ?�레?�어 ?�동 발사 비활??(?�탑???�??발사)
            float effInterval = g_Stats.fireInterval * g_Stats.GetFireIntervalMult() *
                                GravisField::FireIntervalMult(GravisField::g_playerInfluence);
            float effSpeed    = g_Stats.bulletSpeed + g_Stats.GetBulletSpeedBonus();
            fireTimer += delta;
            auto spawnOne = [&](float tx, float ty) {
                float ftx = tx, fty = ty;
                if (g_Stats.bulletSpread > 0.0f) {
                    float dx = tx - pCX, dy = ty - pCY;
                    float ang = atan2f(dy, dx);
                    float jitter = ((float)(rand() % 200) - 100.0f) / 100.0f
                                 * g_Stats.bulletSpread;
                    ang += jitter;
                    ftx = pCX + cosf(ang) * 200.0f;
                    fty = pCY + sinf(ang) * 200.0f;
                }
                Bullet nb(pCX, pCY, ftx, fty);
                nb.speed = effSpeed;
                if (g_Stats.ricochetMax > 0) {
                    nb.bouncesLeft = g_Stats.ricochetMax;
                    nb.dmgMult *= g_Stats.ricochetDmgMult;
                }
                if (g_Stats.berserk) {
                    float hpFrac = g_Stats.maxHP > 0.0f
                                 ? g_GameManager.playerHP / g_Stats.maxHP : 1.0f;
                    hpFrac = std::max(0.0f, std::min(1.0f, hpFrac));
                    nb.dmgMult *= 1.0f + (1.0f - hpFrac) * 0.6f;
                }
                if (g_PlayerRuntime.dashBoostShotsLeft > 0) {
                    --g_PlayerRuntime.dashBoostShotsLeft;
                    nb.dmgMult *= 2.0f;
                }
                g_Bullets.push_back(nb);
            };

            auto spawnAimed = [&](float tx, float ty) {
                TriggerMuzzle(pCX, pCY, atan2f(ty - pCY, tx - pCX));
                Audio::PlaySfx(Audio::Sfx::Shoot);
                if (g_Stats.twin) {
                    float ang = atan2f(ty - pCY, tx - pCX);
                    const float off = 0.087f;
                    const float r = 200.0f;
                    const int n = (g_Stats.twinCount < 2) ? 2 : g_Stats.twinCount;
                    for (int shot = 0; shot < n; ++shot) {
                        float a = (n > 1)
                            ? ang + (((float)shot / (float)(n - 1)) - 0.5f) * (2.0f * off)
                            : ang;
                        spawnOne(pCX + cosf(a) * r, pCY + sinf(a) * r);
                    }
                } else {
                    spawnOne(tx, ty);
                }
            };

            auto aimTarget = [&](float& tx, float& ty) -> bool {
                if (lmb) { tx = wmx; ty = wmy; return true; }
                return g_MonsterManager.FindNearestEnemy(pCX, pCY, tx, ty);
            };

            // ?�?�??�캔 ?�이?�?(증강) ??0.7초마??조�? 방향 관??�?(군중?�어) ?�?�?
            // Laser sweep uses the shared segment collision path.
            if (g_Stats.laser) {
                float laserInt = (g_Stats.laserTier >= 3) ? 0.26f   // 수렴: 빠르지만 화면을 잠식하지 않음
                               : (g_Stats.laserTier >= 2) ? 0.55f : LASER_INT;
                g_LaserTimer += delta;
                if (g_LaserTimer >= laserInt) {
                    g_LaserTimer -= laserInt;
                    float lang  = atan2f(wmy - pCY, wmx - pCX);   // ?�이?�????�� 커서 방향
                    // ?�거리는 II(760)?�서 ???�리지 ?�음. ?�화 ?�렴?�?'?�비'�?강화.
                    float LASER_RANGE = (g_Stats.laserTier >= 3) ? 700.0f
                                      : (g_Stats.laserTier >= 2) ? 760.0f : 560.0f;
                    float lex = pCX + cosf(lang) * LASER_RANGE, ley = pCY + sinf(lang) * LASER_RANGE;
                    // ?�화 ?�렴(tier3): �??�비 2�???광폭 관??(?�몹 ?�인 ?�소)
                    const float beamW    = (g_Stats.laserTier >= 3) ? 1.5f : 1.0f;
                    const float BEAM_HALF = 24.0f * beamW;
                    bool lcrit = false; float lcm = 1.0f;
                    if (g_Stats.critChance > 0 && (rand()%100) < g_Stats.critChance) { lcrit = true; lcm = g_Stats.critMult; }
                    float lbm = 1.0f;
                    if (g_Stats.berserk) {
                        float hf = g_Stats.maxHP > 0.0f ? g_GameManager.playerHP / g_Stats.maxHP : 1.0f;
                        if (hf < 0.0f) hf = 0.0f; else if (hf > 1.0f) hf = 1.0f;
                        lbm = 1.0f + (1.0f - hf) * 0.6f;
                    }
                    float laserDmgMult = (g_Stats.laserTier >= 3) ? 1.15f : 1.4f;
                    float ldmg = g_Stats.GetBaseDamage() * g_Stats.GetDamageMultiplier()
                               * laserDmgMult * lcm * lbm;
                    KillRewardSink killSink{ g_GameManager.playerHP, g_GameManager.scoreAccum,
                                             g_GameManager.score, g_Stats, pCX, pCY };
                    auto inLine = [&](float qx, float qy) -> bool {
                        return SegDist(qx, qy, pCX, pCY, lex, ley) < BEAM_HALF;
                    };
                    for (auto m : g_MonsterManager.monsters) {
                        if (!m->alive || !inLine(m->worldX, m->worldY)) continue;
                        const float dealt = g_MonsterManager.ApplyDamage(*m, ldmg);
                        SpawnDamageNumber(m->worldX, m->worldY, dealt, lcrit);
                        CreditLaserMonsterKill(killSink, *m);
                    }
                    for (auto rr : g_MonsterManager.rangedMobs) {
                        if (!rr->alive || !inLine(rr->worldX, rr->worldY)) continue;
                        const float dealt = g_MonsterManager.ApplyDamage(*rr, ldmg);
                        SpawnDamageNumber(rr->worldX, rr->worldY, dealt, lcrit);
                        CreditLaserScopeKill(killSink, *rr);
                    }
                    g_LaserBeams.push_back({ pCX, pCY, lex, ley, 0.13f, 0.13f, beamW });
                    TriggerMuzzle(pCX, pCY, lang);
                }
            }

            bool fireHeld = lmb && (g_PlayerRuntime.postPickGrace <= 0.0f);
            if (fireHeld && fireTimer >= effInterval) {
                float tx, ty;
                if (aimTarget(tx, ty)) {
                    spawnAimed(tx, ty);
                    fireTimer = 0.0f;
                }
            }
        }

        // GameManager 가 ?�버/변???�태�??????�게 ?�기??(Render ?�서 ?�용)


        g_GameManager.hoveredCard = g_HoveredAug;
        const bool debugCaptureOverride = g_DebugToolkit.CaptureInProgress();
        const GameState debugRestoreState = g_GameManager.currentState;
        if (debugCaptureOverride) {
            // Render each requested scene through the normal dispatch while
            // keeping the real run state untouched between capture frames.
            g_GameManager.currentState =
                g_DebugToolkit.CaptureRenderState(debugRestoreState);
            lmb = false;
            mx = -10000.0;
            my = -10000.0;
        }
        // ============================================================
        // ?�더�?        // ============================================================
        glViewport(0, 0, screenWidth, screenHeight);
        // The pane prototype hides the desktop so its screen captures stay private.
        glClearColor(0.0f, 0.0f, 0.0f, g_PaneDebug ? 1.0f : 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(g_MainShader);
        glUniform1i(g_MainFxLoc, g_ShaderFx ? 1 : 0);   // CRT ?�이???�과 ?��? (G)
        // ?�면 ?�들�?+ �??�용 ??game world �? HUD/text(별도 ortho)???�향 ?�음
        float orthoShake[16];
        memcpy(orthoShake, g_BaseOrtho, sizeof(g_BaseOrtho));
        // �?중심 = ZCX/ZCY, 기본 ?�면 중앙). z=1 ?�면 base ortho ?�??�일(identity)
        {
            float z = g_ViewZoom;
            float zcx = ZCX(), zcy = ZCY();
            orthoShake[0]  =  2.0f * z / (float)screenWidth;
            orthoShake[5]  = -2.0f * z / (float)screenHeight;
            orthoShake[12] =  2.0f * zcx * (1.0f - z) / (float)screenWidth  - 1.0f;
            orthoShake[13] =  1.0f - 2.0f * zcy * (1.0f - z) / (float)screenHeight;
        }
        // 게임?�레???�망?�출 ???�태(메뉴·게임?�버 ???�선 ?�면 ?�들�??�여 ?�거
        //   ???�망 ???�작창으�?갔을 ???�면??계속 ?�리??문제 방�?
        {
            GameState gs = g_GameManager.currentState;
            if (gs != GameState::RUNNING && gs != GameState::DYING &&
                gs != GameState::PAUSED && gs != GameState::AUG_SELECT &&
                gs != GameState::DEBUFF_SELECT) {
                g_ShakeTime = 0.0f; g_ShakeMag = 0.0f;
            }
        }
        if (g_ShakeTime > 0.0f) {
            float intensity = g_ShakeMag * (g_ShakeTime / 0.6f);
            if (intensity > g_ShakeMag) intensity = g_ShakeMag;
            float sx = ((float)(rand() % 200) - 100.0f) / 100.0f * intensity;
            float sy = ((float)(rand() % 200) - 100.0f) / 100.0f * intensity;
            orthoShake[12] -= 2.0f * sx / (float)screenWidth;
            orthoShake[13] += 2.0f * sy / (float)screenHeight;
        }
        BatchFlush();   // ortho(�? 바꾸�??????�전 매트�?���??�인 ?�형 먼�? 그림
        glUniformMatrix4fv(g_MainProjLoc, 1, GL_FALSE, orthoShake);
        // 글로벌?�도 ?�기????BindMainShader() 가 ??�??�용
        memcpy(g_MainOrtho, orthoShake, sizeof(orthoShake));
        glBindVertexArray(g_MainVAO);

        // ?�드/?�티???�더??게임?�레???�태?�서�?그린????메뉴(?�작�?????        //   직전 게임???�레?�어 창·잔???�티?��? ?��? ?�태�?비치??문제 방�?.
        //   (g_MainOrtho ???�에???��? 갱신?�으므�?메뉴 ?�스???�모 ?�더???�상)
        {
        GameState wgs = g_GameManager.currentState;
        bool inWorldRender = (wgs == GameState::RUNNING || wgs == GameState::DYING ||
                              wgs == GameState::PAUSED  || wgs == GameState::READY  ||
                              wgs == GameState::AUG_SELECT || wgs == GameState::DEBUFF_SELECT ||
                              wgs == GameState::AUG_REPLACE ||

                              wgs == GameState::GAMEOVER ||
                              (wgs == GameState::SETTINGS &&
                               g_SettingsReturnTo == GameState::PAUSED));
        if (inWorldRender) {

        EnsurePlayerBounds(playerWin);

        // Screen-edge contrast for the HUD. Keep the combat center clear and
        // darken only the perimeter beneath entities, projectiles, and HUD.
        BatchFlush();
        glDisable(GL_SCISSOR_TEST);
        glEnable(GL_BLEND);
        glUniformMatrix4fv(g_MainProjLoc, 1, GL_FALSE, g_BaseOrtho);
        memcpy(g_MainOrtho, g_BaseOrtho, sizeof(g_BaseOrtho));

        constexpr int kVignetteSteps = 24;
        constexpr float kEdgeAlpha = 0.16f;
        const float sideW = std::min(180.0f, std::max(110.0f, screenWidth * 0.09f));
        const float topH = std::min(220.0f, std::max(140.0f, screenHeight * 0.17f));
        for (int i = 0; i < kVignetteSteps; ++i) {
            const float t = (i + 0.5f) / (float)kVignetteSteps;
            const float fade = 1.0f - t * t * (3.0f - 2.0f * t);
            const float a = kEdgeAlpha * fade;
            const float x0 = sideW * i / (float)kVignetteSteps;
            const float x1 = sideW * (i + 1) / (float)kVignetteSteps;
            const float y0 = topH * i / (float)kVignetteSteps;
            const float y1 = topH * (i + 1) / (float)kVignetteSteps;
            drawRect(x0, 0.0f, x1 - x0, (float)screenHeight, 0.0f, 0.0f, 0.0f, a);
            drawRect((float)screenWidth - x1, 0.0f, x1 - x0,
                     (float)screenHeight, 0.0f, 0.0f, 0.0f, a);
            drawRect(0.0f, y0, (float)screenWidth, y1 - y0, 0.0f, 0.0f, 0.0f, a);
            drawRect(0.0f, (float)screenHeight - y1, (float)screenWidth,
                     y1 - y0, 0.0f, 0.0f, 0.0f, a);
        }
        if (g_StrongMenuDim) {
            drawRect(0.0f, 0.0f, (float)screenWidth, (float)screenHeight,
                     0.0f, 0.0f, 0.0f, 0.58f);
        }
        BatchFlush();
        glUniformMatrix4fv(g_MainProjLoc, 1, GL_FALSE, orthoShake);
        memcpy(g_MainOrtho, orthoShake, sizeof(orthoShake));

        // ?�거�?�?FakeWindow ?�기 ?�수 (?�더·?�리??공용)
        const float RFW_W = g_RfwW;
        const float RFW_H = g_RfwH;

        // ============================================================
        // ?�더 z-order (?�래?�위)
        //  (a) ?�거�?�?FakeWindow 배경  ??가???�래
        //  (b) ?�거�?�?�??��????�몹·총알·?�이?�몬??(scissor ?�리??
        //  (c) ?�레?�어 FakeWindow 배경 ???�에????��
        // ============================================================

        // (a0) ?�명 배경 가리기 ??충격??배경 + ?�레그래??배경
        //      glDisable(GL_BLEND) + 불투�??�두???�형 ???�후 FakeWindow �???��?�?
        BatchFlush(); glDisable(GL_BLEND);
        BindMainShader();

        // (a0-1) ?�폭�?충격??배경 ??needsBg ?�래그�? ?�정??충격?�에 ?�해
        for (auto& sw : g_ShockWaves) {
            if (!sw.active || !sw.needsBg) continue;
            float t   = 1.0f - sw.life / sw.maxLife;  // 0??
            float bgR = sw.maxRadius * t + 30.0f;      // 링보??조금 ?�게
            drawCircle(sw.x, sw.y, bgR, 0.08f, 0.08f, 0.10f, 1.0f);
        }

        // (a) FakeWindow 영역은 아래의 스크리저를 위해 유지한다.
        //     시각적으로는 불투명한 창 대신 얇은 신호 프레임만 사용한다.
        // ?�?�?가짜창 ?�합 z-리스??(??��?�높?? 봇넷 < ?�거�?< 보스/?�라??. 같�? ?�?��?
        //    ?�환(벡터) ?�서 = 먼�? ?�환???��? ?�래. 배경+?�온보더�????�서�?'�??�위'
        //    �?그려, ?��? 창의 불투�?배경????? 창의 배경·?�곽?�을 ?�연????��(?�선?�위 가�?.
        //    (?�레?�어 창�? ???�에 ?�로 그려 ??�� 최상??)
        struct FWin { float x, y, w, h; const wchar_t* name;
                      float br, bgc, bbc, nr, ngc, nbc;
                      float gaugePct = -1.0f; float tb = 0.0f; };
        std::vector<FWin> zwins;
        auto addW = [&](float cx, float cy, float w, float h, const wchar_t* nm,
                        float br, float bgc, float bbc, float nr, float ngc, float nbc,
                        float tb = 0.0f, float gaugePct = -1.0f) {
            zwins.push_back({ cx - w*0.5f, cy - h*0.5f, w, h, nm, br,bgc,bbc, nr,ngc,nbc, gaugePct, tb });
        };
        // 봇넷 ?�드 (최하?? ?�환 ?�서)
        for (auto m : g_MonsterManager.monsters) {
            if (!m->alive || m->kind != MobKind::GENESIS) continue;
            float w = GENESIS_WIN_W * m->sizeScale;
            addW(m->worldX, m->worldY, w, w, L"HIVE", 0.10f,0.04f,0.05f, 0.85f,0.20f,0.24f);
        }
        // Swarm app window composite pass.
        // ?�거�?�?(?�환 ?�서)
        for (auto r : g_MonsterManager.rangedMobs) {
            if (r->deathScale <= 0.0f) continue;
            float sc = r->deathScale;
            addW(r->worldX, r->worldY, RFW_W*sc, RFW_H*sc,
                 L"LENS",
                 0.05f,0.07f,0.12f,
                 0.20f, 0.75f, 0.88f);
        }
        // 터렛 신호 프레임. 실제 터렛 영역/스크리저는 그대로 유지한다.
                // z-order signal layer. Scissor/collision geometry is intentionally
        // kept below; only the heavy visual window treatment is removed.
        BatchFlush(); glDisable(GL_SCISSOR_TEST); glEnable(GL_BLEND);
        for (auto& fw : zwins) {
            if (fw.w < 64.0f || fw.h < 64.0f || fw.w != fw.w || fw.h != fw.h) continue;
            const float signalA = (fw.tb > 0.0f) ? 0.26f : 0.22f;
            DrawInGameSignalFrame(fw.x, fw.y, fw.w, fw.h,
                                  fw.nr, fw.ngc, fw.nbc, signalA);
            if (fw.tb > 0.0f) {
                if (fw.gaugePct >= 0.0f) {
                    float pct = fw.gaugePct < 0.0f ? 0.0f : fw.gaugePct > 1.0f ? 1.0f : fw.gaugePct;
                    float gy = fw.y + 4.0f;
                    drawRect(fw.x + 10.0f, gy, fw.w - 20.0f, 2.0f,
                             0.08f, 0.10f, 0.14f, 0.55f);
                    drawRect(fw.x + 10.0f, gy, (fw.w - 20.0f) * pct, 2.0f,
                             fw.nr, fw.ngc, fw.nbc, 0.90f);
                }
            }
        }
        BatchFlush(); glEnable(GL_BLEND);  // ?�후 ?�반 ?�파 블렌??보장

        // (b) Global combat pass.
        // The old implementation rendered the same entities through each
        // fake window's WorldScissor. That made enemies and bullets disappear
        // whenever they crossed a window boundary, and also multiplied draw
        // calls. Fake windows remain visual/collision metadata only.
        BatchFlush();
        glDisable(GL_SCISSOR_TEST);

        // Rear and front sight fields stay before entity bodies to preserve
        // the existing combat render order.
        const float sightPlayerX = playerWin.x + playerWin.width * 0.5f;
        const float sightPlayerY = playerWin.y + playerWin.height * 0.5f;
        const float sightPlayerSize = PLAYER_SIZE * g_Stats.playerSizeMult;
        DrawCombatSightFields(
            g_MonsterManager, sightPlayerX, sightPlayerY, sightPlayerSize,
            g_GameManager.currentState != GameState::DYING &&
                g_GameManager.currentState != GameState::GAMEOVER);

        DrawRegulusBeams(g_MonsterManager, g_GameTime);

        // Mobs in another window space than the player read dim (lines
        // stay, only brightness drops), so the player sees who can reach them.
        const float spaceBaseAlpha = g_BatchAlpha;
        const auto dimIfElsewhere = [&](float x, float y) {
            g_BatchAlpha = space::Active() && space::SpaceAt(x, y) != space::g_PlayerSpace
                ? spaceBaseAlpha * 0.35f : spaceBaseAlpha;
        };
        for (auto m : g_MonsterManager.monsters) {
            if (!(m->alive || m->DeathEffectVisible())) continue;
            dimIfElsewhere(m->worldX, m->worldY);
            drawMob(m, g_GameTime);
        }
        for (auto r : g_MonsterManager.rangedMobs) {
            dimIfElsewhere(r->worldX, r->worldY);
            drawRangedMob(r, g_GameTime);
        }
        g_BatchAlpha = spaceBaseAlpha;
        DrawBulletHalos(g_Bullets);
        for (auto& b : g_Bullets) {
            if (b.active) drawBullet(b);
        }
        for (int i = 0; i < EnemyParticleCap(); ++i) {
            auto& p = g_EnemyParts[i];
            if (!p.active || p.delay > 0.0f) continue;
            float a = p.life / p.maxLife;
            float hs = p.size * 0.5f;
            switch (p.shape) {
            case EnemyParticleShape::CIRCLE:
                drawCircle(p.x, p.y, hs, p.r, p.g, p.b, a);
                break;
            case EnemyParticleShape::TRIANGLE: {
                const float heading = std::atan2(p.vy, p.vx);
                const float radius = p.size * 0.58f;
                const float a0 = heading - 1.5707963f;
                const float a1 = a0 + 2.0943951f;
                const float a2 = a1 + 2.0943951f;
                BatchTri(p.x + std::cos(a0) * radius,
                         p.y + std::sin(a0) * radius,
                         p.x + std::cos(a1) * radius,
                         p.y + std::sin(a1) * radius,
                         p.x + std::cos(a2) * radius,
                         p.y + std::sin(a2) * radius, p.r, p.g, p.b, a);
                break;
            }
            case EnemyParticleShape::SQUARE:
            default:
                drawRect(p.x - hs, p.y - hs, p.size, p.size,
                         p.r, p.g, p.b, a);
                break;
            }
        }
        DrawStardustPickups();
        for (auto& orb : g_ApproachOrbs) {
            DrawApproachOrb(orb.x, orb.y, g_GameTime);
        }
        BatchFlush();

        if (g_GameManager.currentState != GameState::DYING &&
            g_GameManager.currentState != GameState::GAMEOVER) {
            const float pCX = playerWin.x + playerWin.width * 0.5f;
            const float pCY = playerWin.y + playerWin.height * 0.5f;
            const float playerSize = PLAYER_SIZE * g_Stats.playerSizeMult;
            BindMainShader();
            const GameState healthState = g_GameManager.currentState;
            if (healthState == GameState::RUNNING ||
                (healthState == GameState::PAUSED && !g_ShowPauseInventory) ||
                healthState == GameState::AUG_SELECT ||
                healthState == GameState::DEBUFF_SELECT) {
                const float hpFraction = std::clamp(g_GameManager.playerHP /
                    std::max(1.0f, g_Stats.maxHP), 0.0f, 1.0f);
                const float hpHit = std::min(1.0f, g_HpBarPop / 0.35f);
                // Keep the blue health arc readable even at full HP.
                const float hpRest = hpFraction >= 1.0f ? 0.85f : 0.92f;
                vitals_hud::DrawHpArc(pCX, pCY, playerSize, hpFraction,
                                      g_HpGhost.frac,
                                      std::min(1.0f, hpRest + 0.30f * hpHit),
                                      g_GameTime);
            }
            DrawPlayerWeaponShell(pCX, pCY, playerSize,
                                  atan2f(wmy - pCY, wmx - pCX), g_GameTime);
            DrawLevelUpRings(pCX, pCY, playerSize,
                             g_GameManager.levelUpTransition.Progress());
            BatchFlush();
        }

        // (c) player playfield signal. The rectangular region itself remains
        // available to gameplay/scissor code, but no opaque fake window is drawn.
        BatchFlush(); glDisable(GL_SCISSOR_TEST); glEnable(GL_BLEND);
        EnsurePlayerBounds(playerWin);



        // (c2) HP/EXP �????�레?�어 �??�단 ?�쪽??부�?(창과 ?�께 ?�동) ?�?�?
        // ?�?�?(h3) 가�?OS �??�롬 ???�?��?�?+ [X] ?�기 (?�스?�톱 ?�계관) ?�?�?
        //    "??= ?�로?�스, 창을 ?�아 종료?�다" ?�체?? ?�드 좌표(�?반영)�?그림.
        {
            GameState gst = g_GameManager.currentState;
            bool inGame = (gst == GameState::RUNNING || gst == GameState::PAUSED ||
                           gst == GameState::DYING || gst == GameState::AUG_SELECT ||
                           gst == GameState::DEBUFF_SELECT);
            if (inGame) {
                auto winChrome = [&](float x, float y, float w, float h,
                                     const wchar_t* title, float tr, float tg, float tb,
                                     float gaugePct = -1.0f) {
                    (void)x; (void)y; (void)w; (void)h;
                    (void)title; (void)tr; (void)tg; (void)tb; (void)gaugePct;
                };
                // 가짜창 ?�?��?�????�에??만든 z-리스????��?�높?? ?�서�?그림.
                //   '?�기보다 ?��? �? ?�는 '?�레?�어 �????�?��?바�? ??���? ?�체�?                //   ?�기??�??�니??**겹친 가�?구간�?* ?�라?�다(부�??�리??.
                //   ?�?��?바의 ?�로 ??y,y+TB]?�?겹치??가림창??x구간??가?�구간에??빼고,
                //   ?��? 구간?�만 glScissor �??�립??그린?? (봇넷<?�거�?보스<?�레?�어,
                //   같�? ?�?��? ?�환?�서)
                const float TBH = 22.0f;
                glEnable(GL_SCISSOR_TEST);
                auto drawBarClipped = [&](float x, float y, float w, float h,
                                          const wchar_t* nm, float nr, float ng, float nb,
                                          size_t selfIdx, bool isPlayer, float gaugePct = -1.0f) {
                    // 가??x구간 리스??(�?vec2: x=?�작, y=??
                    std::vector<glm::vec2> vis{ glm::vec2(x, x + w) };
                    auto subtract = [&](float c0, float c1) {
                        if (c1 <= c0) return;
                        std::vector<glm::vec2> out;
                        for (auto& iv : vis) {
                            float a = iv.x, b = iv.y;
                            if (c1 <= a || c0 >= b) { out.push_back(iv); continue; }
                            if (c0 > a) out.push_back(glm::vec2(a, c0));
                            if (c1 < b) out.push_back(glm::vec2(c1, b));
                        }
                        vis.swap(out);
                    };
                    auto consider = [&](float ox, float oy, float ow, float oh) {
                        if (oy < y + TBH && oy + oh > y)   // 가림창???�?��?�??�로 ?��? 겹침
                            subtract(std::max(x, ox), std::min(x + w, ox + ow));
                    };
                    if (!isPlayer)
                        consider(playerWin.x, playerWin.y, playerWin.width, playerWin.height);
                    for (size_t j = selfIdx + 1; j < zwins.size(); j++)
                        consider(zwins[j].x, zwins[j].y, zwins[j].w, zwins[j].h);
                    // ?��? 구간�??�립??그림
                    for (auto& iv : vis) {
                        float a = iv.x, b = iv.y;
                        if (b - a < 0.5f) continue;
                        WorldScissor(a, y, b - a, TBH);
                        winChrome(x, y, w, h, nm, nr, ng, nb, gaugePct);
                    }
                };
                for (size_t i = 0; i < zwins.size(); i++) {
                    const FWin& fw = zwins[i];
                    drawBarClipped(fw.x, fw.y, fw.w, fw.h, fw.name,
                                   fw.nr, fw.ngc, fw.nbc, i, false, fw.gaugePct);
                }
                BatchFlush();
                // ?�레?�어 �?????�� 최상?? ?�립 ?�이 ?�체
                BatchFlush();
                glDisable(GL_SCISSOR_TEST);
            }
        }

        BatchFlush();   // Restore the base orthographic projection for the HUD.
        glUniformMatrix4fv(g_MainProjLoc, 1, GL_FALSE, g_BaseOrtho);
        memcpy(g_MainOrtho, g_BaseOrtho, sizeof(g_BaseOrtho));









        // (h3) ?�면 ?�래?????�벨??보스처치/부???�간 번쩍
        if (g_FlashIntensity > 0.001f) {
            BindMainShader();
            float a = g_FlashIntensity; if (a > 0.85f) a = 0.85f;
            drawRect(0, 0, (float)screenWidth, (float)screenHeight,
                     g_FlashColor.r, g_FlashColor.g, g_FlashColor.b, a);
        }
        // FORK.worm ?�피 ???��???RGB 분리 글리치


        // (h4) ?�간 ?��? ???�면 가?�자�??�안 비네??(?��? ?�출)
        if (g_PlayerRuntime.timeStopTimer > 0.0f) {
            BindMainShader();
            float a = 0.10f + 0.05f * sinf((float)glfwGetTime() * 10.0f);
            float bw = 18.0f;
            drawRect(0, 0, (float)screenWidth, bw, 0.3f, 0.9f, 1.0f, a);
            drawRect(0, (float)screenHeight - bw, (float)screenWidth, bw, 0.3f, 0.9f, 1.0f, a);
            drawRect(0, 0, bw, (float)screenHeight, 0.3f, 0.9f, 1.0f, a);
            drawRect((float)screenWidth - bw, 0, bw, (float)screenHeight, 0.3f, 0.9f, 1.0f, a);
        }

        // (h5) ?�격 ??빨간 가?�자�?비네??(?�격 ?��?
        if (g_HurtVignette > 0.001f) {
            const float t = std::clamp(g_HurtVignette / 0.5f, 0.0f, 1.0f);
            DrawEdgeVignette(52.0f * t + 8.0f, 0.20f * t,
                             0.95f, 0.10f, 0.10f);
        }


        BatchFlush();
        glDisable(GL_SCISSOR_TEST);

        }   // if (inWorldRender)
        }   // world render game block

        // [6] HUD
        g_GameManager.Render();

        // ?�?�?[7] ?�국???�스??+ 메뉴 ?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?
        {
            float sw = (float)screenWidth, sh = (float)screenHeight;
            auto  st = g_GameManager.currentState;

            // ���� �� ���̵� ������Ʈ ��������������������������������������������������������������������������������
            if (g_FadeDir == 1) {               // ���̵� �ƿ� (��������)
                g_FadeAlpha += delta / FADE_OUT_DUR;
                if (g_FadeAlpha >= 1.0f) {
                    g_FadeAlpha = 1.0f;
                    g_GameManager.currentState = g_FadeTarget;
                    st = g_FadeTarget;
                    g_AppOpen = 0.0f;
                    g_FadeDir = -1;
                }
            } else if (g_FadeDir == -1) {       // ���̵� �� (���� �� ����)
                g_FadeAlpha -= delta / FADE_IN_DUR;
                if (g_FadeAlpha <= 0.0f) {
                    g_FadeAlpha = 0.0f;
                    g_FadeDir   = 0;
                }
            }


            // ??�?shop/codex/config) 진입 ???�림 ?�니메이???�작 ??�??�레??진행
            {
                static GameState s_prevWinSt = GameState::MAIN_MENU;
                bool isApp = (st == GameState::CODEX ||
                              st == GameState::TUTORIAL || st == GameState::SETTINGS);
                if (st != s_prevWinSt) {
                    if (isApp) g_AppOpen = 0.0f;   // ??�????�기 0?�서 ?�기
                    s_prevWinSt = st;
                }
                if (isApp && g_AppOpen < 1.0f) {
                    g_AppOpen += delta / APP_OPEN_DUR;
                    if (g_AppOpen > 1.0f) g_AppOpen = 1.0f;
                }
            }

            // ?�?�??�게???�업?�시�???메뉴?�??�일??OS ?�레???��? (?�스?�톱 방어 ?��??? ?�?�?
            // ?�?�??�적 ?�금 ?�스??(?�단 중앙 배너, 4�??�시 ???�이?? ?�?�?
            if (g_AchToastTimer > 0.0f && g_AchToastId >= 0 &&
                g_AchToastId < ACH_COUNT) {
                g_AchToastTimer -= delta;
                float a = g_AchToastTimer > 3.0f ? (4.0f - g_AchToastTimer)
                        : std::min(1.0f, g_AchToastTimer);
                if (a < 0.0f) a = 0.0f; if (a > 1.0f) a = 1.0f;
                int li2 = LangIndex();
                const wchar_t* LBL[3] = { L"\uC5C5\uC801 \uB2EC\uC131!", L"Achievement!", L"\u5B9F\u7E3E\u9054\u6210!" };
                wchar_t tb[160];
                wchar_t rewardText[32];
                FormatGrouped(ACH_DEFS[g_AchToastId].coinReward, rewardText, 32);
                swprintf_s(tb, L"%ls  %ls  (+%ls)", LBL[li2],
                           AchName(g_AchToastId), rewardText);
                const float toastScale = UiTextScale(g_TextS, UiTextLevel::Description,
                                                     UiScale(sw, sh));
                float bw = g_TextS.Width(tb, toastScale) + 48.0f;
                float bx0 = (sw - bw) * 0.5f, by0 = sh * 0.10f;
                BindMainShader();
                drawRect(bx0, by0, bw, 52.0f, 0.12f, 0.10f, 0.02f, 0.85f * a);
                drawRect(bx0, by0, bw, 4.0f, 1.0f, 0.85f, 0.25f, 0.95f * a);
                g_TextS.Draw(tb, bx0 + 24.0f, by0 + 16.0f, toastScale,
                             1.0f, 0.9f, 0.4f, a);
            }

            // ?�?�??�리?�이?�브 HUD (게임 �? ??F:증강  G:무적 ?�?�?
            if (g_CreativeMode &&
                (st == GameState::RUNNING || st == GameState::READY ||
                 st == GameState::AUG_SELECT || st == GameState::DEBUFF_SELECT)) {
                int li3 = LangIndex();
                const wchar_t* CH[3] = {
                    L"CREATIVE   F: \uC99D\uAC15   G: \uBB34\uC801",
                    L"CREATIVE   F: Augment   G: Godmode",
                    L"CREATIVE   F: ?\u5316   G: \u30B4\u30C3\u30C9" };
                g_TextS.Draw(CH[li3], 20.0f, HudY(sh, Hud::CREATIVE_LABEL),
                             UiTextScale(g_TextS, UiTextLevel::Supporting, UiScale(sw, sh)),
                             0.7f, 0.85f, 1.0f, 0.85f);
                if (g_CreativeGodmode) {
                    const wchar_t* GOD[3] = { L"* \uBB34\uC801 ON", L"* GODMODE ON", L"* \u30B4\u30C3\u30C9 ON" };
                    float blink = 0.65f + 0.35f * sinf((float)glfwGetTime() * 5.0f);
                    g_TextL.Draw(GOD[li3], 20.0f, 24.0f,
                                 UiTextScale(g_TextL, UiTextLevel::Description, UiScale(sw, sh)),
                                 1.0f, 0.85f, 0.2f, blink);
                }
            }
#if defined(_DEBUG)
            if (g_DebugMode && !g_CreativeMode &&
                (st == GameState::RUNNING || st == GameState::READY ||
                 st == GameState::AUG_SELECT || st == GameState::DEBUFF_SELECT)) {
                int li3 = LangIndex();
                const wchar_t* DBG[3] = {
                    L"\uB514\uBC84\uADF8   F: \uB808\uBCA8\uC5C5   G: \uBB34\uC801",
                    L"DEBUG   F: LEVEL UP   G: GODMODE",
                    L"\u30C7\u30D0\u30C3\u30B0   F: \u30EC\u30D9\u30EB\u30A2\u30C3\u30D7   G: \u7121\u6575"
                };
                g_TextS.Draw(DBG[li3], 20.0f, HudY(sh, Hud::CREATIVE_LABEL),
                             UiTextScale(g_TextS, UiTextLevel::Supporting, UiScale(sw, sh)),
                             0.7f, 0.85f, 1.0f, 0.85f);
                if (g_CreativeGodmode) {
                    const wchar_t* GOD[3] = { L"* \uBB34\uC801 ON", L"* GODMODE ON", L"* \u30B4\u30C3\u30C9 ON" };
                    float blink = 0.65f + 0.35f * sinf((float)glfwGetTime() * 5.0f);
                    g_TextL.Draw(GOD[li3], 20.0f, 24.0f,
                                 UiTextScale(g_TextL, UiTextLevel::Description, UiScale(sw, sh)),
                                 1.0f, 0.85f, 0.2f, blink);
                }
            }
#endif

            // ?�?�?[7b] UI ???�스?�치 ??메뉴/�??�태??Scene_* ?�수�?분리 ?�?�?
            //    RUNNING/DYING(?�수 ?�게?????�이 ?�으??컨텍?�트 구성 ?�체�?건너?�?
            UpdateGameTextHalo(screenHeight);

            // Scene_* dispatch still includes READY and the in-run menus;
            // this flag only controls which state gets the scene composition.
            const bool outgameBackdrop =
                st != GameState::RUNNING && st != GameState::DYING;
            if (outgameBackdrop) {
                std::function<void()> resetFn = ResetForNewGame;
                std::function<void()> restartRunFn = RestartCurrentRun;
                std::function<void()> abandonRunFn = AbandonCurrentRun;
                // ������ ũ�� ����(���?64px) ���� Ŭ���� ���� �������� ����
                bool sceneLmb = (!g_DebugToolkit.IsInputCaptured() &&
                                 my >= BROWSER_CHROME_H) ? lmb : false;
                SceneCtx ctx{ sw, sh, mx, my, sceneLmb, delta, window, &fireTimer,
                              resetFn, restartRunFn, abandonRunFn,
                              inputFocusChanged };
                const SceneTextContext sceneText{ g_TextL, g_TextS };
                switch (st) {
                case GameState::MAIN_MENU:         Scene_MainMenu(ctx);         break;
                case GameState::CODEX:             Scene_Codex(ctx);            break;
                case GameState::TUTORIAL: {
                    const TutorialSceneContext tutorialState{
                        g_GameManager.currentState,
                        g_LmbPrev,
                        g_AppOpen,
                        sceneText
                    };
                    Scene_Tutorial(ctx, tutorialState);
                    break;
                }
                case GameState::CREATIVE_CONFIG:   Scene_CreativeConfig(ctx);   break;
                case GameState::SETTINGS:          Scene_Settings(ctx);         break;
                case GameState::READY:             Scene_Ready(ctx, sceneText); break;
                // Pause is drawn after the HUD and version so its veil dims
                // the entire gameplay view, not just the world geometry.
                case GameState::PAUSED: break;
                case GameState::GAMEOVER: {
                    const GameOverSceneContext reportState{
                        g_GameManager.currentState,
                        g_GameOverFade,
                        g_LmbPrev,
                        g_DeathReason,
                        g_GameManager.waveVictory,
                        g_LastRunRecord,
                        g_GameManager.score,
                        g_GameManager.playerLevel,
                        g_Stats.killCount,
                        g_OwnedAugs,
                        sceneText
                    };
                    Scene_GameOver(ctx, reportState);
                    break;
                }
                case GameState::AUG_SELECT:
                case GameState::DEBUFF_SELECT:     Scene_AugSelect(ctx);        break;
                case GameState::AUG_REPLACE:       Scene_AugReplace(ctx);       break;
                default: break;
                }
            }
            // Refresh the cursor after scene actions change the state.
            InputUpdateGameplayCursor(window, g_GameManager.currentState,
                                      g_ShowCrosshair, g_DebugToolkit.IsVisible());

            // ?�단 HUD ???�제 게임 진행 ?�태?�서�?(메뉴/?�감/?�점?????�게)
            if (st == GameState::RUNNING || st == GameState::PAUSED ||
                st == GameState::DYING   || st == GameState::AUG_SELECT ||
                st == GameState::DEBUFF_SELECT) {
                const float hudScale = UiScale(sw, sh);
                if (st == GameState::RUNNING) {
                    const float hpFrac = g_Stats.maxHP > 0.0f
                        ? g_GameManager.playerHP / g_Stats.maxHP : 1.0f;
                    if (hpFrac > 0.0f && hpFrac < 0.25f) {
                        const float pulse = 0.5f + 0.5f * sinf(g_GameTime * 6.0f);
                        const float severity = 1.0f - hpFrac / 0.25f;
                        const float alpha = (0.12f + 0.22f * pulse)
                                          * (0.6f + 0.4f * severity);
                        DrawEdgeVignette(72.0f * hudScale, alpha * 0.85f,
                                         0.92f, 0.08f, 0.10f);
                    }
                }
#ifdef __APPLE__
                const float hudTopY = BROWSER_CHROME_H + 8.0f + 30.0f;
#else
                const float hudTopY = BROWSER_CHROME_H + 4.0f;
#endif
                // Level + XP: one line along the bottom edge, stopping short
                // of the version label in the bottom-right corner.
                if (st != GameState::DYING &&
                    !(st == GameState::PAUSED && g_ShowPauseInventory)) {
                    wchar_t lvBuf2[64];
                    swprintf_s(lvBuf2, L"%ls%d",
                               T(StrId::LV_PREFIX), g_GameManager.playerLevel);
                    const bool xpAtCap = !g_CreativeMode &&
                        g_GameManager.playerLevel >= MAIN_LEVEL_CAP;
                    const float xpFrac = vitals_hud::XpFill(g_GameManager.xp,
                        g_ExpSystem.RequiredAmount(g_GameManager.playerLevel), xpAtCap);
                    static const std::wstring xpVersionLabel = std::wstring(L"v") + ONEDOW_VERSION;
                    const float versionScale = vitals_hud::BottomHudVersionScale(hudScale);
                    const float xpRightX = sw - vitals_hud::BottomHudPadding(hudScale)
                        - g_TextS.Width(xpVersionLabel.c_str(), versionScale) - vitals_hud::BottomHudGap(hudScale);
                    vitals_hud::DrawXpEdgeLine(sh, hudScale, xpFrac, xpAtCap, lvBuf2,
                        g_GameManager.levelUpTransition.Progress(), xpRightX);

                    // HP arc text: LOW HP beside the arc, numbers after a hit.
                    const float hpFrac = std::clamp(g_GameManager.playerHP /
                        std::max(1.0f, g_Stats.maxHP), 0.0f, 1.0f);
                    vitals_hud::DrawHpArcText(playerWin.x + playerWin.width * 0.5f,
                        playerWin.y + playerWin.height * 0.5f,
                        PLAYER_SIZE * g_Stats.playerSizeMult, hpFrac,
                        g_GameManager.playerHP, g_Stats.maxHP,
                        std::min(1.0f, g_HpBarPop / 0.6f) * 0.9f, hudScale, g_GameTime,
                        st == GameState::RUNNING || st == GameState::PAUSED);
                }
                DrawWaveHud(sw, hudScale, hudTopY, g_GameTime, g_Difficulty, g_GameManager);
                DrawNormaBossBar(sw, hudScale, hudTopY);

                // ?�상?? FPS
                wchar_t fpsBuf[32];
                swprintf_s(fpsBuf, L"%ls  %d", T(StrId::FPS), g_CurrentFPS);
                const float fpsScale = UiTextScale(g_TextS, UiTextLevel::Supporting, hudScale);
                float fpsW = g_TextS.Width(fpsBuf, fpsScale);
                g_TextS.Draw(fpsBuf, sw - fpsW - 12.0f, hudTopY, fpsScale,
                             0.7f, 0.9f, 1.0f, 0.85f);



                // (HP/EXP ?�각 바는 ?�레?�어 �??�단??부착됨 ????(c2) 참고)
                // ?�단 조작 ?�내 (?��??�게 ??��) ?????�레?�어가 HP/?�킬 ?�치�??�게
            }

            // ?�?�??�맛: ?��?지 ?�자 ?�업 + 콤보 카운??(?�제 ?�레??중에�? ?�?�?
            //    메뉴/?�시?��??�선 ?��? ???�스?��? 메뉴 ?�에 ?�던 버그 fix
            if (st == GameState::RUNNING || st == GameState::DYING) {
                // ?��?지 ?�자 (?�드 ???�크�?변?????�스?? ???�정 ?��?
                for (auto& d : g_DmgNumbers) {
                    float t  = d.life / d.maxLife;                   // 1 ??0
                    float sx = roundf(W2SX(d.x)), sy = W2SY(d.y);
                    wchar_t nb[32]; FormatGrouped(d.amount, nb, 32);
                    float sc = UiTextScale(g_TextS, UiTextLevel::Description,
                        (d.crit ? 1.05f : 0.72f) * g_ViewZoom
                        * (0.7f + 0.3f * t) * UiScale(sw, sh));
                    float a  = (t > 0.55f) ? 1.0f : (t / 0.55f);
                    float w  = g_TextS.Width(nb, sc);
                    if (d.crit) {
                        const int critLang = LangIndex();
                        const wchar_t* critLabel = critLang == 0 ? L"치명타"
                            : (critLang == 2 ? L"クリティカル" : L"CRIT");
                        const float critSc = UiTextScale(g_TextS, UiTextLevel::Supporting,
                                                         0.80f + 0.20f * t);
                        const float critW = g_TextS.Width(critLabel, critSc);
                        g_TextS.Draw(critLabel, sx - critW * 0.5f,
                                     sy - g_TextS.Height(critLabel, critSc) - 3.0f,
                                     critSc, 1.0f, 0.74f, 0.18f, a * 0.92f);
                        g_TextS.Draw(nb, sx - w * 0.5f, sy, sc,
                                     1.0f, 0.85f, 0.2f, a);
                    } else {
                        g_TextS.Draw(nb, sx - w*0.5f, sy, sc,
                                     1.0f, 1.0f, 1.0f, a*0.9f);
                    }
                }
                // 콤보 카운??(5콤보 ?�상부?? ?�이 콤보???�라 강해�? ???�정 ?��?
                if (g_ShowCombo && st == GameState::RUNNING && g_Combo >= 5) {
                    bool  ms      = (g_ComboMilestone > 0.0f);
                    float msBoost = ms ? (g_ComboMilestone / 0.7f) : 0.0f;
                    wchar_t cb[48], comboText[32]; FormatGrouped(g_Combo, comboText, 32);
                    swprintf_s(cb, L"%ls COMBO", comboText);
                    float sc = UiTextScale(g_TextS, UiTextLevel::Subtitle,
                        (1.0f + g_ComboPulse * 0.4f + msBoost * 0.55f) * UiScale(sw, sh));
                    sc *= 1.0f + (g_Combo > 30 ? 0.12f : 0.0f);
                    float cr = 1.0f, cg = 1.0f, cbl = 1.0f;
                    if      (g_Combo >= 50) { cg = 0.25f; cbl = 0.2f; }
                    else if (g_Combo >= 25) { cg = 0.55f; cbl = 0.15f; }
                    else if (g_Combo >= 12) { cg = 0.9f;  cbl = 0.3f; }
                    if (ms) { cr = 1.0f; cg = 0.85f; cbl = 0.30f; }   // 마일?�톤 = 골드 ?�스
                    float w = g_TextS.Width(cb, sc);
                    g_TextS.Draw(cb, (sw - w) * 0.5f, sh * 0.115f, sc, cr, cg, cbl, 0.95f);
                }
            }

            // Skill HUD (bottom centre): key cooldowns and cooldown passives.
            if (st == GameState::RUNNING ||
                (st == GameState::PAUSED && !g_ShowPauseInventory)) {
                DrawSkillHud(skill_hud::BuildSkillHudView(GatherSkillHudInputs(), g_SkillHudPulse),
                             UiScale(sw, sh), sw, sh);
            }

        }

        if (!g_DebugToolkit.SuppressOverlayForCapture()) {
            BindMainShader();
            static const std::wstring versionLabel = std::wstring(L"v") + ONEDOW_VERSION;
            const float versionUiScale = UiScale((float)screenWidth, (float)screenHeight);
            const float versionScale = vitals_hud::BottomHudVersionScale(versionUiScale);
            const float versionPad = vitals_hud::BottomHudPadding(versionUiScale);
            g_TextS.Draw(versionLabel.c_str(),
                         screenWidth - versionPad - g_TextS.Width(versionLabel.c_str(), versionScale),
                         HudY((float)screenHeight, versionPad) - g_TextS.Height(versionLabel.c_str(), versionScale),
                         versionScale, 0.62f, 0.76f, 0.88f, 0.92f);
        }

        // Pane marks stay off the augment screens, which cover the field.
        {
            const GameState bs = g_GameManager.currentState;
            if (bs != GameState::AUG_SELECT && bs != GameState::DEBUFF_SELECT &&
                bs != GameState::AUG_REPLACE)
                DrawPaneBrackets();
        }
        if (g_PaneDebug) DrawPaneDebug();

        if (g_GameManager.currentState == GameState::PAUSED) {
            const bool sceneLmb = !g_DebugToolkit.IsInputCaptured() &&
                                  my >= BROWSER_CHROME_H && lmb;
            const SceneCtx ctx{ (float)screenWidth, (float)screenHeight, mx, my,
                                sceneLmb, delta, window, &fireTimer,
                                ResetForNewGame, RestartCurrentRun, AbandonCurrentRun,
                                inputFocusChanged };
            const SceneTextContext sceneText{ g_TextL, g_TextS };
            const PauseSceneContext pauseState{
                g_GameManager.currentState, g_GameManager.pauseResumeState,
                g_SettingsReturnTo, g_ShowPauseInventory, g_LmbPrev,
                sceneText, ResetSettingsUi
            };
            Scene_Paused(ctx, pauseState);
            if (g_ShowPauseInventory) Scene_OwnedAugPanel(ctx);
            InputUpdateGameplayCursor(window, g_GameManager.currentState,
                                      g_ShowCrosshair, g_DebugToolkit.IsVisible());
        }

        if (!g_DebugToolkit.SuppressOverlayForCapture())
            g_DebugToolkit.Render((float)screenWidth, (float)screenHeight,
                                  g_GameManager.currentState);

        const GameState crosshairState = g_GameManager.currentState;
        if (g_ShowCrosshair && !g_DebugToolkit.IsVisible() &&
            (crosshairState == GameState::RUNNING ||
             crosshairState == GameState::DYING) &&
            glfwGetWindowAttrib(window, GLFW_FOCUSED) &&
            mx >= 0.0 && my >= 0.0 && mx < screenWidth && my < screenHeight) {
            BindMainShader();
            DrawGameplayCrosshair((float)mx, (float)my);
        }

        // Keep the physical mouse state for edge detection. `lmb` may be
        // cleared above when the debug toolkit captures input; storing that
        // cleared value makes a held button look like a new click every frame.
        g_LmbPrev = rawLmb;
        g_RmbPrev = rawRmb;

        // ?�적 ?�금 / ?�감 발견 발생 ???�??(게임 �?즉시 ?�구??
        if (g_AchSaveNeeded || g_CodexDirty) {
            SaveGame(); g_AchSaveNeeded = false; g_CodexDirty = false;
        }

        BatchFlush();   // ?�레??마�?�????��? ?�형 모두 그림
        if (g_DebugToolkit.SuppressOverlayForCapture())
            g_DebugToolkit.CaptureFrameAfterRender(
                screenWidth, screenHeight, g_GameManager.currentState);
        glfwSwapBuffers(window);
        if (debugCaptureOverride)
            g_GameManager.currentState = debugRestoreState;

        // ?�?�?FPS �?(g_FpsCap > 0 ???�만) ?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?
        // timeBeginPeriod(1) �?Sleep ?�상??1ms. 마�?�?~1ms ??busy-wait
        // C18: 과거 '무제??(-1) ?�이브는 300 ?�로 ?�램??(진짜 무제???�거).
        int capFps = (g_FpsCap < 0) ? 300 : g_FpsCap;
        if (capFps > 0) {
            double target = 1.0 / (double)capFps;
            double frameStart = (double)now;
            double remain = target - (glfwGetTime() - frameStart);
            if (remain > 0.001) {
                unsigned ms = (unsigned)((remain - 0.001) * 1000.0);
                PlatformSleepMs(ms);
            }
#ifndef _WIN32
            // macOS/Linux: ������ ���� busy-spin�� CPU���߿��� �� ª�� sleep
            while (glfwGetTime() - frameStart < target) {
                double left = target - (glfwGetTime() - frameStart);
                if (left > 0.003)
                    PlatformSleepMs(1);
                else
                    break;
            }
#else
            while (glfwGetTime() - frameStart < target) { /* spin */ }
#endif
        }
    }

    g_GameplayTelemetry.EndRun(g_GameTime,
                               g_GameManager.playerLevel,
                               g_GameManager.xp,
                               g_Stats,
                               "application_exit");
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glDeleteVertexArrays(1, &g_MainVAO);
    glDeleteBuffers(1, &g_VBO);
    glDeleteProgram(g_MainShader);
    ShutdownGameFonts();
    g_GameManager.ShutdownGlResources();
#ifdef _WIN32
    if (g_FontMemHandle) {
        RemoveFontMemResourceEx(g_FontMemHandle);
        g_FontMemHandle = nullptr;
    }
#endif
    Audio::Shutdown();
    PlatformTimerEnd();
    os_pane::Shutdown();
    ReleaseWindowTaskbarPolicy(window);
    InputUnregisterCallbacks(window);
    glfwTerminate();
    return 0;
}
