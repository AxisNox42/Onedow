#include "Scenes.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "GameManager.h"
#include "GameContext.h"
#include "SceneSkills.h"
#include "SceneUI.h"
#include "SceneInternal.h"
#include "UiLayout.h"
#include "WindowChrome.h"
#include "Settings.h"
#include "Translations.h"
#include "TutorialText.h"
#include "Meta.h"
#include "Achievements.h"
#include "Codex.h"
#include "Augment.h"
#include "PlayerStats.h"
#include "Weapons.h"
#include "ExpSystem.h"
#include "SaveSystem.h"
#include "DrawPrim.h"
#include "../../Render/UiColors.h"
#include "../../Render/PlanetShader.h"
#include "../../Render/NebulaGlowShader.h"
#include "MainShader.h"
#include "../../Render/BlurShader.h"
#include "../../System/Audio.h"
#include "TextRenderer.h"
#include "IconSystem.h"
#include "../System/SystemInfo.h"
#include "Camera.h"
#include "EntityDraw.h"
#include "Monster.h"
#include "AugmentSlots.h"
#include "../../System/Input.h"
#include "../../Render/Juice.h"
#include <algorithm>
#include <vector>
#include <string>
#include <cmath>
#include <functional>

constexpr float SCENE_BG_ALPHA = 0.0f;
float g_MainMenuEntryT  = 0.0f;
static float g_CodexEntryT     = 0.0f;
static float g_ShopEntryT      = 0.0f;
// Page textures use one reveal gate so CircleTexture layers do not pop in as
// independent draw calls during a scene handoff.  The gate is changed only
// by page entry/exit animation, never by list selection or hover state.
static float g_SceneTextureReveal = 1.0f;
static bool  s_ShopBackRequested = false;
static bool  s_CodexBackRequested = false;
static bool  s_ShopNeedsOpenInit = true;
static bool  s_MainMenuArmoryPanel = false;
static bool  s_MainMenuCodexPanel = false;
bool  s_MainMenuSettingsPanel = false;
bool  s_MainMenuRunConfigPanel = false;
bool  s_MainMenuResumeFromPanel = false;
// PLAY owns the current trial catalogue. The gameplay system still has four
// slots; this UI state packs enabled definitions into those slots on PLAY.

float UiApproach(float v, float target, float dt, float speed) {
    float k = dt * speed;
    if (k > 1.0f) k = 1.0f;
    return v + (target - v) * k;
}

struct ConstellationMotionProfile {
    float rotationSpeed;
    float radialPull;
    float edgeAlpha;
    float nodePulse;
    float alignmentStrength;
};

struct ConstellationMotionState {
    ConstellationMotionProfile current{};
    ConstellationMotionProfile target{};
    float phase = 0.0f;
    bool initialized = false;
};

static ConstellationMotionState s_MainMenuConstellationMotion;

// One source of truth for labels that remain visible while a page is being
// entered or exited.  Transition overlays used to carry their own legacy
// English route IDs, which caused names such as ASTRAL_LOG to flash before the
// localized page title appeared.
const wchar_t* MainMenuRouteLabel(int language, int index) {
    static const wchar_t* kRoutes[3][5] = {
        { L"플레이", L"상점", L"도감", L"설정", L"종료" },
        { L"PLAY", L"SHOP", L"ASTRAL LOG", L"SETTINGS", L"EXIT" },
        { L"スタート", L"ショップ", L"星界記録", L"設定", L"終了" },
    };
    const int li = std::max(0, std::min(2, language));
    const int mi = std::max(0, std::min(4, index));
    return kRoutes[li][mi];
}

static ConstellationMotionProfile MainMenuMotionProfile(int menuIndex) {
    static constexpr ConstellationMotionProfile kIdle =
        { 1.35f,  0.00f, 0.74f, 0.64f, 0.10f };
    static constexpr ConstellationMotionProfile kProfiles[5] = {
        { 1.90f, -0.08f, 0.90f, 1.00f, 0.12f }, // PLAY(STAR CHART): core convergence
        { 1.72f,  0.10f, 1.00f, 0.92f, 0.08f }, // ARMORY: radial expansion
        { 0.92f,  0.01f, 0.82f, 0.62f, 0.20f }, // ASTRAL_LOG: stable observation
        { 1.12f,  0.00f, 0.88f, 0.72f, 0.78f }, // CALIBRATION: orbital alignment
        { 0.34f, -0.06f, 0.46f, 0.28f, 0.42f }, // SHUTDOWN: motion decay
    };
    return (menuIndex >= 0 && menuIndex < 5) ? kProfiles[menuIndex] : kIdle;
}

static bool MotionProfileSettled(const ConstellationMotionProfile& a,
                                 const ConstellationMotionProfile& b) {
    return std::abs(a.rotationSpeed - b.rotationSpeed) < 0.005f
        && std::abs(a.radialPull - b.radialPull) < 0.005f
        && std::abs(a.edgeAlpha - b.edgeAlpha) < 0.003f
        && std::abs(a.nodePulse - b.nodePulse) < 0.003f
        && std::abs(a.alignmentStrength - b.alignmentStrength) < 0.005f;
}

static void UpdateMainMenuConstellationMotion(int menuIndex, float dt) {
    ConstellationMotionState& state = s_MainMenuConstellationMotion;
    state.target = MainMenuMotionProfile(menuIndex);
    if (!state.initialized) {
        state.current = state.target;
        state.initialized = true;
    } else {
        const float clampedDt = std::min(dt, 0.05f);
        const float follow = 1.0f - expf(-7.5f * clampedDt);
        auto approach = [follow](float current, float target) {
            return current + (target - current) * follow;
        };
        state.current.rotationSpeed = approach(state.current.rotationSpeed, state.target.rotationSpeed);
        state.current.radialPull = approach(state.current.radialPull, state.target.radialPull);
        state.current.edgeAlpha = approach(state.current.edgeAlpha, state.target.edgeAlpha);
        state.current.nodePulse = approach(state.current.nodePulse, state.target.nodePulse);
        state.current.alignmentStrength = approach(state.current.alignmentStrength,
                                                   state.target.alignmentStrength);
        if (MotionProfileSettled(state.current, state.target))
            state.current = state.target;
    }

    constexpr float kTau = 6.28318530717958647692f;
    state.phase = fmodf(state.phase
                        + state.current.rotationSpeed * std::min(dt, 0.05f), kTau);
}



static void ResetCodexUi(float entryStart = 0.0f) {
    g_CodexEntryT = std::max(0.0f, std::min(1.0f, entryStart));
    s_CodexBackRequested = false;
    CodexSearchClear();
    g_CodexSearchInputEnabled = false;
}

static void ResetShopUi(float entryStart = 0.0f) {
    g_ShopEntryT = std::max(0.0f, std::min(1.0f, entryStart));
    s_ShopBackRequested = false;
    s_ShopNeedsOpenInit = true;
}

void SmokeOpenMenuPanel(int panel) {
    s_MainMenuArmoryPanel = false;
    s_MainMenuCodexPanel = false;
    s_MainMenuSettingsPanel = false;
    s_MainMenuRunConfigPanel = false;
    switch (panel) {
    case 0: ResetRunConfigUi(1.0f); s_MainMenuRunConfigPanel = true; break;
    case 1: ResetShopUi(1.0f);      s_MainMenuArmoryPanel = true;    break;
    case 2: ResetCodexUi(1.0f);     s_MainMenuCodexPanel = true;     break;
    case 3: ResetSettingsUi(1.0f);  s_MainMenuSettingsPanel = true;  break;
    default: break;
    }
}
// 메인메뉴·난이도선택 공용 앰비언트 배경 (파티클 + 스캔라인 + 선택적 비네트)
void DrawMenuBackground(float sw, float sh, float delta, float darkenAmount) {
    float dtp = delta; if (dtp > 0.05f) dtp = 0.05f;
    BindMainShader();
    if (darkenAmount < 0.0f) darkenAmount = 0.0f;
    if (darkenAmount > 1.0f) darkenAmount = 1.0f;

    if (darkenAmount > 0.0f) {
        // 딥 네이비 반투명 — 배경화면이 비치도록
        drawRect(0, 0, sw, sh, 0.04f, 0.06f, 0.14f, 0.16f * darkenAmount);
    }

    struct AP { float x, y, vx, vy, sz, tw; };
    constexpr int kParticleCount = 30;
    static AP a_ps[kParticleCount]; static bool ap_init = false;
    if (!ap_init) { ap_init = true;
        for (int i = 0; i < kParticleCount; i++) {
            a_ps[i].x = (float)(rand()%(int)sw); a_ps[i].y = (float)(rand()%(int)sh);
            float ang = (rand()%628)*0.01f, spd = 5.0f + (rand()%16);
            a_ps[i].vx = cosf(ang)*spd; a_ps[i].vy = sinf(ang)*spd;
            a_ps[i].sz = 1.2f + (rand()%26)*0.1f; a_ps[i].tw = (rand()%628)*0.01f;
        }
    }
    // 성운 3색 파티클 (바이올렛 / 마젠타 / 차가운 별빛)
    static const float kNebR[3] = { 0.42f, 0.92f, 0.72f };
    static const float kNebG[3] = { 0.28f, 0.22f, 0.82f };
    static const float kNebB[3] = { 1.00f, 0.78f, 1.00f };
    const int visibleParticles = (g_VfxDensity == VfxDensity::REDUCED) ? 18 : kParticleCount;
    for (int i = 0; i < visibleParticles; i++) { AP& p = a_ps[i];
        p.x += p.vx*dtp; p.y += p.vy*dtp;
        if (p.x < -8) p.x = sw+8; if (p.x > sw+8) p.x = -8;
        if (p.y < -8) p.y = sh+8; if (p.y > sh+8) p.y = -8;
        p.tw += dtp*1.6f;
        int ci = i % 3;
        drawCircle(p.x, p.y, p.sz, kNebR[ci], kNebG[ci], kNebB[ci],
                   PulsedAmbientAlpha(0.050f, 0.025f, p.tw));
    }
    // The post-process CRT shader already supplies scanlines. Keep this
    // decorative pass only when CRT is off so thin menu geometry is not doubled.
    if (!g_ShaderFx) {
        const float scanlineA = (g_VfxDensity == VfxDensity::REDUCED) ? 0.008f : 0.012f;
        for (float yy = 0.0f; yy < sh; yy += 4.0f)
            drawRect(0.0f, yy, sw, 1.0f, 0.35f, 0.20f, 0.85f, scanlineA);
    }
    if (darkenAmount > 0.0f) {
        // 상하 비네트
        drawRect(0, 0, sw, 110.0f, 0.0f, 0.0f, 0.0f, 0.22f * darkenAmount);
        drawRect(0, sh - 90.0f, sw, 90.0f, 0.0f, 0.0f, 0.0f, 0.22f * darkenAmount);
    }
}

static float MainLogoTop(float sh) {
    return sh * 0.105f + 50.0f;
}

static float MainLogoHeight(float sh) {
    float h = sh * 0.118f;
    if (h < 86.0f) h = 86.0f;
    if (h > 132.0f) h = 132.0f;
    return h;
}

float LogoClamp01(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

void SetSceneTextureReveal(float reveal) {
    g_SceneTextureReveal = LogoClamp01(reveal);
}

void LogoLine(float x1, float y1, float x2, float y2,
                     float thick, float r, float g, float b, float a) {
    float dx = x2 - x1, dy = y2 - y1;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.5f || thick <= 0.0f || a <= 0.0f) return;
    float nx = -dy / len * thick * 0.5f;
    float ny =  dx / len * thick * 0.5f;
    BatchTri(x1 + nx, y1 + ny, x1 - nx, y1 - ny, x2 + nx, y2 + ny, r, g, b, a);
    BatchTri(x1 - nx, y1 - ny, x2 - nx, y2 - ny, x2 + nx, y2 + ny, r, g, b, a);
}

void DrawSceneLeftVignette(float sw, float sh, float alpha) {
    if (alpha <= 0.001f) return;
    const UiThemePalette& theme = GetUiTheme();
    DrawLinearGradient(0.0f, 0.0f, sw * 0.60f, sh,
                       theme.Dim.r, theme.Dim.g, theme.Dim.b, alpha * theme.Dim.a);
}

// bg_linear.png is the page's persistent left-side contrast field.  Keep it
// outside the content reveal so a page transition never exposes a bright
// flash or removes the static left rail for a frame.
static void DrawPersistentSceneLeftVignette(float sw, float sh, float alpha) {
    DrawSceneLeftVignette(sw, sh, alpha);
}

// Wide paired side fields for settings and other information-heavy pages.
// The mirrored texture keeps the center open while both edges receive the
// same deep contrast treatment used by the PLAY composition.
// Shared settings layout constants are declared in SceneInternal.h.

void DrawPersistentSceneSideVignettes(float sw, float sh, float alpha) {
    if (alpha <= 0.001f) return;
    const UiThemePalette& theme = GetUiTheme();
    const float fieldW = sw * kWideSceneLinearWidth;
    // bg_linear is already a darkening mask; applying the theme alpha again
    // made it disappear against the menu background.
    const float fieldA = alpha;
    DrawLinearGradient(0.0f, 0.0f, fieldW, sh,
                       theme.Dim.r, theme.Dim.g, theme.Dim.b,
                       fieldA, false);
    DrawLinearGradient(sw - fieldW, 0.0f, fieldW, sh,
                       theme.Dim.r, theme.Dim.g, theme.Dim.b,
                       fieldA, true);
}

void DrawSceneRadialVignette(float cx, float cy, float radius, float alpha) {
    if (alpha <= 0.001f || radius <= 1.0f) return;
    const UiThemePalette& theme = GetUiTheme();
    DrawRadialGradient(cx, cy, radius, theme.Dim.r, theme.Dim.g, theme.Dim.b,
                       alpha * theme.Dim.a * 0.96f);
}

void DrawSettingsOrbitArc(float cx, float cy, float radiusX,
                                  float radiusY, float startAngle,
                                  float endAngle, float r, float g, float b,
                                  float thickness, float alpha) {
    if (alpha <= 0.001f || radiusX <= 1.0f || radiusY <= 1.0f) return;
    constexpr int kSegments = 48;
    float prevX = cx + cosf(startAngle) * radiusX;
    float prevY = cy + sinf(startAngle) * radiusY;
    for (int i = 1; i <= kSegments; ++i) {
        const float t = (float)i / (float)kSegments;
        const float a = startAngle + (endAngle - startAngle) * t;
        const float x = cx + cosf(a) * radiusX;
        const float y = cy + sinf(a) * radiusY;
        LogoLine(prevX, prevY, x, y, thickness, r, g, b, alpha);
        prevX = x;
        prevY = y;
    }
}

float MainMenuCommandStartY(float sh, float uiScale) {
    const float kRowH = 70.0f * uiScale;
    const float kGap = 15.0f * uiScale;
    constexpr float kRowCount = 5.0f;
    const float totalH = kRowCount * kRowH + (kRowCount - 1.0f) * kGap;
    float y = sh * 0.48f;
    if (y + totalH > sh - 54.0f) y = sh - totalH - 54.0f;
    const float logoFloor = MainLogoTop(sh) + MainLogoHeight(sh) * 0.72f;
    if (y < logoFloor) y = logoFloor;
    return y;
}

// Outgame navigation rails share one lower-left bottom anchor across pages.
// Scene transition ghosts use the same lobby calculation as the live buttons.
float OutgameButtonRailStartY(float sh, float uiScale, int rowCount,
                              float rowH, float rowGap) {
    const float totalH = rowCount * rowH + std::max(0, rowCount - 1) * rowGap;
    float y = sh - totalH - std::max(52.0f * uiScale, sh * 0.075f);
    const float logoFloor = MainLogoTop(sh) + MainLogoHeight(sh) * 0.82f;
    if (y < logoFloor) y = logoFloor;
    return y;
}

float MainMenuButtonRailStartY(float sh, float uiScale) {
    return OutgameButtonRailStartY(sh, uiScale, 5,
                                   70.0f * uiScale, 8.0f * uiScale);
}

static void DrawSharedMenuDim(float sw, float sh,
                              float commandX, float commandY,
                              float commandW, float commandH,
                              float alpha,
                              bool persistentLeft = false) {
    if (alpha <= 0.001f && !persistentLeft) return;
    if (persistentLeft) {
        DrawPersistentSceneLeftVignette(sw, sh, std::max(alpha, 0.72f));
    } else {
        DrawSceneLeftVignette(sw, sh, alpha);
    }
    if (alpha <= 0.001f) return;
    DrawSceneRadialVignette(commandX + commandW * 0.32f,
                            commandY + commandH * 0.50f,
                            std::max(commandW, commandH) * 0.72f,
                            0.36f * alpha);
}
static void DrawAstralDataPlate(float x, float y, float w, float h,
                                float r, float g, float b, float alpha) {
    if (alpha <= 0.001f || w <= 1.0f || h <= 1.0f) return;
    const float cut = std::min(34.0f, std::min(w, h) * 0.08f);
    // Data surfaces stay neutral black in every theme. Accent color belongs
    // to text, markers, and brackets; tinting the fill reduces contrast.
    const float baseR = 0.004f;
    const float baseG = 0.006f;
    const float baseB = 0.010f;
    const float innerR = 0.002f;
    const float innerG = 0.004f;
    const float innerB = 0.008f;
    BindMainShader();
    // Keep the plate readable on bright backgrounds. The texture is only an
    // edge treatment; using its opaque center as the panel fill creates a
    // large pale glow when stretched over wide detail regions.
    drawRect(x, y, w, h, baseR, baseG, baseB, 0.54f * alpha);
    if (g_ConfigPanelTex) {
        DrawIcon(g_ConfigPanelTex, x, y, w, h,
                 innerR, innerG, innerB, 0.13f * alpha);
    } else {
        drawRect(x + 5.0f, y + 5.0f, w - 10.0f, h - 10.0f,
                 innerR, innerG, innerB, 0.18f * alpha);
    }
    drawRect(x + cut, y, w * 0.42f, 1.4f, r, g, b, 0.38f * alpha);
    drawRect(x + w - cut - w * 0.20f, y + h - 1.4f,
             w * 0.20f, 1.4f, r, g, b, 0.22f * alpha);
    drawRect(x, y + cut, 1.4f, h * 0.30f, r, g, b, 0.34f * alpha);
    drawRect(x + w - 1.4f, y + h * 0.60f,
             1.4f, h * 0.20f, r, g, b, 0.20f * alpha);
    LogoLine(x, y + cut, x + cut, y, 1.2f, r, g, b, 0.46f * alpha);
    LogoLine(x + w - cut, y + h, x + w, y + h - cut,
             1.2f, r, g, b, 0.30f * alpha);
    drawDiamond(x + cut, y, 2.8f, r, g, b, 0.62f * alpha);
    drawDiamond(x + w - cut, y + h, 2.4f, r, g, b, 0.40f * alpha);
}

// Constellation geometry sits directly on the desktop (no plate behind it).
// Faint lines and nodes are tuned for a dark or blurred backdrop, so with a
// sharp desktop they need their own dark keyline, the same idea as the text
// halo. A blurred desktop only gets a light touch to keep the original look.
static float ConstellGeometryHalo() {
    return g_BackdropBlurEnabled ? 0.25f : 1.0f;
}

void DrawVisibleConstellLine(float x1, float y1, float x2, float y2,
                                    float thick, float r, float g, float b, float a) {
    if (a <= 0.001f) return;
    // sqrt(a) lifts the keyline of deliberately faint lines more than that of
    // already strong ones, so a 0.15-alpha edge still separates from white.
    const float halo = ConstellGeometryHalo();
    const float liftA = sqrtf(a);
    const float keyA = std::min(0.85f, a * 0.30f + halo * 0.80f * liftA);
    // On a sharp desktop the line itself also needs to win against wallpaper
    // detail: it gets brighter and up to ~60% thicker, faint lines the most.
    const float fgA = std::min(1.0f, a + halo * 0.60f * liftA * (1.0f - a));
    thick *= 1.0f + 0.60f * halo;
    const float keyPad = 1.2f * halo;
    // Keyline in a dark shade of the line's own colour, not neutral black.
    const float kr = r * 0.22f, kg = g * 0.22f, kb = b * 0.22f;
    if (g_ConstellationLineTex) {
        const float dx = x2 - x1, dy = y2 - y1;
        const float len = sqrtf(dx * dx + dy * dy);
        if (len > 0.5f) {
            const float ang = atan2f(dy, dx);
            DrawIconRot(g_ConstellationLineTex,
                        (x1 + x2) * 0.5f, (y1 + y2) * 0.5f,
                        len * 0.5f, thick * 2.4f + keyPad, ang,
                        kr, kg, kb, keyA);
            DrawIconRot(g_ConstellationLineTex,
                        (x1 + x2) * 0.5f, (y1 + y2) * 0.5f,
                        len * 0.5f, thick * 0.72f, ang,
                        r, g, b, fgA);
            return;
        }
    }
    LogoLine(x1, y1, x2, y2, thick * 2.75f + keyPad, kr, kg, kb,
             std::max(0.42f * a, keyA));
    LogoLine(x1, y1, x2, y2, thick, r, g, b, fgA);
}

// DrawVisibleConstellLine 과 같은 키라인 계산이지만 텍스처 호출 없이 LogoLine(배치 지오메트리)만
// 쓴다. 한 프레임에 수백 개 선분을 그리는 장면(PLAY 별자리)은 이쪽을 쓴다.
void DrawConstellLineBatched(float x1, float y1, float x2, float y2,
                             float thick, float r, float g, float b, float a) {
    if (a <= 0.001f) return;
    const float halo = ConstellGeometryHalo();
    const float liftA = sqrtf(a);
    const float keyA = std::min(0.85f, a * 0.30f + halo * 0.80f * liftA);
    const float fgA = std::min(1.0f, a + halo * 0.60f * liftA * (1.0f - a));
    thick *= 1.0f + 0.60f * halo;
    const float keyPad = 1.2f * halo;
    // 텍스처 선과 달리 LogoLine 은 가장자리가 단단한 사각형이라, 같은 값이면 테두리가 훨씬
    // 두껍고 짙어 보인다. 키라인을 가늘고 연하게, 본선은 약간 가늘게 해 시각적 두께를 맞춘다.
    LogoLine(x1, y1, x2, y2, thick * 1.7f + keyPad * 0.5f, r * 0.22f, g * 0.22f, b * 0.22f,
             std::max(0.25f * a, keyA * 0.55f));
    LogoLine(x1, y1, x2, y2, thick * 0.85f, r, g, b, fgA);
}

void DrawConstellationDiscEllipse(float x, float y, float rx, float ry,
                                  float r, float g, float b, float a,
                                  bool darkenBlend) {
    a *= g_SceneTextureReveal;
    if (a <= 0.001f || rx <= 0.1f || ry <= 0.1f) return;
    if (g_ConstellationCircleTex) {
        const bool darken = darkenBlend
                         && r <= 0.02f && g <= 0.02f && b <= 0.02f;
        if (darken) {
            DrawIconDarken(g_ConstellationCircleTex,
                           x - rx, y - ry, rx * 2.0f, ry * 2.0f,
                           r, g, b, a);
        } else {
            DrawIcon(g_ConstellationCircleTex,
                     x - rx, y - ry, rx * 2.0f, ry * 2.0f,
                     r, g, b, a);
        }
    } else {
        drawCircle(x, y, rx, r, g, b, a);
    }
}

void DrawConstellationDisc(float x, float y, float radius,
                                  float r, float g, float b, float a,
                                  bool darkenBlend) {
    DrawConstellationDiscEllipse(x, y, radius, radius, r, g, b, a, darkenBlend);
}

static void ShopStaticFieldPalette(int tab, float& r, float& g, float& b) {
    switch (tab) {
    case 0: // CORE UNLOCKS: cold ice
        r = 0.38f; g = 0.74f; b = 1.00f;
        break;
    case 1: // RIFLE: restrained green-gold
        r = 0.72f; g = 0.88f; b = 0.48f;
        break;
    case 2: // FIELD: violet signal
        r = 0.58f; g = 0.50f; b = 1.00f;
        break;
    default: // Fallback palette
        r = 0.30f; g = 0.86f; b = 1.00f;
        break;
    }
}

struct ShopDetailLayout {
    float left = 0.0f;
    float right = 0.0f;
    float top = 0.0f;
    float bottom = 0.0f;
    float titleY = 0.0f;
    float copyY = 0.0f;
    float tradeY = 0.0f;
    float actionX = 0.0f;
    float actionW = 0.0f;
    float actionY = 0.0f;
    float fieldY = 0.0f;
};

static ShopDetailLayout MakeShopDetailLayout(float x, float top,
                                             float width, float bottom,
                                             float uiS) {
    ShopDetailLayout layout;
    layout.left = x;
    layout.right = x + width;
    layout.top = top;
    layout.bottom = bottom;
    layout.titleY = top + 10.0f * uiS;
    // Detail content starts at the title and ends at the lower-right action.
    // The purchase control owns the bottom edge of the readout rather than
    // floating beside the level/cost rows.
    layout.copyY = layout.titleY + 78.0f * uiS;
    layout.actionY = bottom - 36.0f * uiS;
    // Keep the purchase readout close to the action control. The order inside
    // the block is LEVEL -> COST -> BALANCE -> verdict -> button.
    layout.tradeY = layout.actionY - 158.0f * uiS;
    // Keep the action anchored to the right edge, but give its label a wider
    // reading lane so long states do not collapse into a narrow ribbon.
    layout.actionX = x + width * 0.30f;
    layout.actionW = width * 0.70f;
    layout.fieldY = layout.copyY + (layout.actionY - layout.copyY) * 0.54f;

    // On a short viewport, pull the middle rows upward while preserving the
    // title and the button's bottom-edge ownership.
    const float minTradeY = layout.copyY + 96.0f * uiS;
    if (layout.tradeY < minTradeY) {
        layout.tradeY = minTradeY;
        layout.fieldY = layout.copyY + (layout.actionY - layout.copyY) * 0.54f;
    }
    return layout;
}

void DrawVisibleConstellNode(float x, float y, float size,
                                    float r, float g, float b, float a,
                                    bool whiteSpark,
                                    bool drawField) {
    if (a <= 0.001f) return;
    if (drawField) {
        // Keep the optical halo subordinate to the constellation geometry.
        // The node diamond is the signal; these fields only separate it from
        // the background and should not read as a second, larger star.
        if (g_ConstellationCircleTex && g_IconBatchProg) {
            DrawConstellationDisc(x, y, size * 1.85f,
                                  0.0f, 0.0f, 0.012f, 0.34f * a);
            const float reveal = g_SceneTextureReveal;
            if (reveal > 0.001f) {
                static std::vector<IconBatchQuad> fields;
                fields.clear();
                fields.push_back({
                    x - size * 1.15f, y - size * 1.15f,
                    size * 2.30f, size * 2.30f,
                    r * 0.08f, g * 0.08f, b * 0.10f,
                    0.26f * a * reveal
                });
                fields.push_back({
                    x - size * 0.88f, y - size * 0.88f,
                    size * 1.76f, size * 1.76f,
                    r, g, b, 0.58f * a * reveal
                });
                // The first layer already established the icon pass. Keep
                // pending diamonds queued so later node fields preserve the
                // same painter order as the former per-layer draws.
                DrawIconBatch(g_ConstellationCircleTex, fields, false, false);
            }
        } else {
            DrawConstellationDisc(x, y, size * 1.85f,
                                  0.0f, 0.0f, 0.012f, 0.34f * a);
            DrawConstellationDisc(x, y, size * 1.15f,
                                  r * 0.08f, g * 0.08f, b * 0.10f, 0.26f * a);
            DrawConstellationDisc(x, y, size * 0.88f,
                                  r, g, b, 0.58f * a);
        }
    }
    const float halo = ConstellGeometryHalo();
    if (halo > 0.0f)
        drawDiamond(x, y, size * 1.20f + 2.0f * halo,
                    r * 0.22f, g * 0.22f, b * 0.22f,
                    std::min(0.70f, halo * (0.30f + 0.45f * a)));
    // The node body and spark are lifted the same way as faint lines.
    const float nodeA = std::min(1.0f, a + halo * 0.45f * sqrtf(a) * (1.0f - a));
    drawDiamond(x, y, size * 1.20f, r, g, b, 0.74f * nodeA);
    drawDiamond(x, y, size * (0.42f + 0.18f * halo),
                whiteSpark ? 0.96f : r,
                whiteSpark ? 1.0f  : g,
                whiteSpark ? 1.0f  : b,
                0.88f * nodeA);
}

void DrawArchiveConstellation(float cx, float cy, float radius,
                                     int category, int key, float now,
                                     float r, float g, float b,
                                     float alpha, float uiS,
                                     bool drawNodeFields) {
    if (alpha <= 0.001f || radius <= 4.0f) return;

    // Entity entries reuse the gameplay renderer so the archive miniature is
    // the same ROTOR/GENESIS/SCOPE/SWARM/GRAVIS silhouette seen in a run.
    if (category == 0) {
        float previewScale = std::max(1.8f, radius / 18.0f);
        // Genesis has a larger station footprint than compact process forms.
        if (key == CM_GENESIS) previewScale *= 1.18f;
        drawCodexMobPreview(key, cx, cy, previewScale);
        return;
    }

    constexpr int kNodeCount = 9;
    float px[kNodeCount] = {};
    float py[kNodeCount] = {};
    float depth[kNodeCount] = {};
    const unsigned seed = 0x9E3779B9u ^ (unsigned)(key + 17) * 2654435761u
                        ^ (unsigned)(category + 3) * 2246822519u;
    const float drift = now * (category == 2 ? 0.055f
                               : category == 1 ? 0.095f : 0.045f);

    for (int i = 0; i < kNodeCount; ++i) {
        const unsigned h = seed ^ (unsigned)(i + 1) * 3266489917u;
        const float jitter = (float)((h >> 9) & 1023u) / 1023.0f;
        const float ring = 0.48f + 0.46f * (float)((h >> 20) & 255u) / 255.0f;
        float angle = drift + (6.2831853f * (float)i / (float)kNodeCount);
        if (category == 0) angle += (jitter - 0.5f) * 0.48f;
        if (category == 1) angle += (i & 1) ? 0.08f : -0.08f;
        if (category == 2) angle += sinf(now * 0.18f + (float)i) * 0.035f;
        if (category == 3) angle += ((i & 1) ? 0.035f : -0.035f);
        const float z = sinf(angle * (category == 1 ? 2.0f : 1.5f) + jitter * 3.2f);
        const float perspective = 0.88f + 0.12f * z;
        const float rr = radius * ring * perspective;
        px[i] = cx + cosf(angle) * rr;
        py[i] = cy + sinf(angle) * rr * (0.66f + 0.08f * z);
        depth[i] = z;
    }

    // A restrained orbital scaffold gives the constellation depth without
    // turning the archive readout into another radar chart.
    const int scaffoldRings = category == 3 ? 3 : 2;
    for (int ring = 0; ring < scaffoldRings; ++ring) {
        const float rr = category == 3
            ? radius * (0.46f + 0.21f * (float)ring)
            : radius * (0.52f + 0.34f * (float)ring);
        const float phase = drift * (ring == 0 ? -0.8f : 0.55f);
        float prevX = cx + cosf(phase) * rr;
        float prevY = cy + sinf(phase) * rr * 0.68f;
        for (int s = 1; s <= 40; ++s) {
            const float a = phase + 6.2831853f * (float)s / 40.0f;
            const float x = cx + cosf(a) * rr;
            const float y = cy + sinf(a) * rr * 0.68f;
            if ((s + ring) % (category == 3 ? 4 : 3) != 0)
                LogoLine(prevX, prevY, x, y, 0.55f * uiS,
                         r, g, b,
                         (category == 3
                             ? (ring == 0 ? 0.18f : ring == 1 ? 0.12f : 0.08f)
                             : (ring == 0 ? 0.14f : 0.10f)) * alpha);
            prevX = x; prevY = y;
        }
    }

    for (int i = 0; i < kNodeCount; ++i) {
        const int next = (i + 1) % kNodeCount;
        DrawVisibleConstellLine(px[i], py[i], px[next], py[next],
                                0.92f * uiS, r, g, b, 0.56f * alpha);
        if ((i + category) % 3 == 0) {
            const int cross = (i + 4 + category) % kNodeCount;
            DrawVisibleConstellLine(px[i], py[i], px[cross], py[cross],
                                    0.68f * uiS, r, g, b, 0.30f * alpha);
        }
    }

    if (category == 2) {
        for (int i = 0; i < kNodeCount; i += 2)
            DrawVisibleConstellLine(cx, cy, px[i], py[i], 0.64f * uiS,
                                    r, g, b, 0.20f * alpha);
        DrawConstellationDisc(cx, cy, 18.0f * uiS, 0.0f, 0.0f, 0.01f, 0.28f * alpha);
        DrawVisibleConstellNode(cx, cy, 5.0f * uiS, r, g, b,
                                0.92f * alpha, false, drawNodeFields);
    }
    if (category == 3) {
        // Core records carry a central anchor and an extra orbital tier so
        // they read as gameplay systems rather than another colour profile.
        DrawConstellationDisc(cx, cy, 24.0f * uiS,
                              0.0f, 0.0f, 0.012f, 0.24f * alpha);
        DrawVisibleConstellNode(cx, cy, 7.0f * uiS, r, g, b,
                                0.92f * alpha, false, drawNodeFields);
    }

    for (int i = 0; i < kNodeCount; ++i) {
        const float twinkle = 0.80f + 0.20f * sinf(now * 2.1f + (float)i * 1.7f);
        const float size = (3.0f + 1.15f * (depth[i] + 1.0f))
                         * uiS * (category == 3 ? 1.12f : 1.0f);
        const float nodeAlpha = twinkle * alpha
            * (depth[i] > 0.55f ? 1.0f : 0.86f);
        // Node fields are the expensive part of a constellation preview. The
        // shop keeps them for the settled/selected entry, while a moving
        // neighbour only draws its star core during list transitions.
        if (drawNodeFields) {
            DrawConstellationDisc(px[i], py[i], size * 2.20f,
                                  0.0f, 0.0f, 0.012f, 0.10f * nodeAlpha);
            DrawConstellationDisc(px[i], py[i], size * 1.35f,
                                  r, g, b, 0.035f * nodeAlpha);
        }
        DrawVisibleConstellNode(px[i], py[i], size, r, g, b,
                                nodeAlpha, false, drawNodeFields);
    }
}

// A deliberately non-identifying placeholder for records that have not been
// encountered yet. It must not reuse the gameplay silhouette: even a dimmed
// version would leak the entity's shape through the archive.
static void DrawUnknownConstellation(float cx, float cy, float radius,
                                     float now, float alpha, float uiS,
                                     int seed) {
    if (alpha <= 0.001f || radius <= 4.0f) return;

    static const float kX[6] = { -0.82f, -0.24f, 0.52f, 0.80f, 0.12f, -0.58f };
    static const float kY[6] = { -0.08f, -0.70f, -0.44f,  0.34f, 0.76f,  0.42f };
    static const int kEdges[7][2] = {
        { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 4 },
        { 4, 5 }, { 5, 0 }, { 1, 4 }
    };
    const float phase = (float)(seed % 7) * 0.17f + now * 0.035f;
    // Dark enough to read as unresolved, but bright enough that the viewer
    // can immediately tell a constellation is occupying this slot.
    const float cr = 0.42f, cg = 0.50f, cb = 0.62f;

    DrawConstellationDisc(cx, cy, radius * 1.16f,
                          0.0f, 0.0f, 0.006f, 0.42f * alpha);
    DrawConstellationDisc(cx, cy, radius * 0.54f,
                          0.02f, 0.028f, 0.045f, 0.34f * alpha);

    float px[6] = {}, py[6] = {};
    for (int i = 0; i < 6; ++i) {
        const float a = phase + (float)i * 0.055f;
        const float x = kX[i] * radius;
        const float y = kY[i] * radius;
        px[i] = cx + x * cosf(a) - y * sinf(a);
        py[i] = cy + x * sinf(a) + y * cosf(a);
    }
    for (const auto& edge : kEdges) {
        DrawVisibleConstellLine(px[edge[0]], py[edge[0]],
                                px[edge[1]], py[edge[1]],
                                0.52f * uiS, cr, cg, cb,
                                0.58f * alpha);
    }

    // Broken orbital fragments keep the placeholder alive without forming a
    // recognizable category-specific shape.
    for (int i = 0; i < 3; ++i) {
        const float start = phase + 0.65f + (float)i * 2.08f;
        const float end = start + 0.32f + 0.05f * (float)(seed % 3);
        const float x0 = cx + cosf(start) * radius * 0.92f;
        const float y0 = cy + sinf(start) * radius * 0.92f;
        const float x1 = cx + cosf(end) * radius * 0.92f;
        const float y1 = cy + sinf(end) * radius * 0.92f;
        DrawVisibleConstellLine(x0, y0, x1, y1,
                                0.42f * uiS, cr, cg, cb,
                                0.42f * alpha);
    }

    for (int i = 0; i < 6; ++i) {
        const float pulse = 0.78f + 0.22f * sinf(now * 1.7f + (float)i * 1.4f);
        DrawVisibleConstellNode(px[i], py[i],
                                (3.0f + 0.70f * (float)(i & 1)) * uiS,
                                cr, cg, cb, pulse * 0.82f * alpha,
                                false, true);
    }
    DrawConstellationDisc(cx, cy, radius * 0.18f,
                          0.02f, 0.028f, 0.045f, 0.42f * alpha);
    DrawVisibleConstellNode(cx, cy, 4.0f * uiS, cr, cg, cb,
                            0.88f * alpha, false, true);
    drawDiamond(cx, cy, 2.2f * uiS, 0.68f, 0.76f, 0.88f,
                0.64f * alpha);
}

// AESTERIA title mark: thin polygonal letters with small solid star dots and
// one slim orbit that joins the first A's apex to the last A's lower-right
// foot. Lines only; the orbit passes behind the letters except at those two
// joins, which carry the focal stars.
static void LogoRing(float cx, float cy, float radius, float thick,
                     float r, float g, float b, float a) {
    constexpr int kSeg = 14;
    float px = cx + radius, py = cy;
    for (int i = 1; i <= kSeg; ++i) {
        const float t = 6.2831853f * (float)i / (float)kSeg;
        const float x = cx + cosf(t) * radius;
        const float y = cy + sinf(t) * radius;
        LogoLine(px, py, x, y, thick, r, g, b, a);
        px = x; py = y;
    }
}

void DrawMainOnedowLogo(float sw, float sh, float alpha, float reveal,
                               float centerXOverride, float heightMul) {
    if (alpha <= 0.001f) return;

    struct Pt { float x, y; };
    struct Stroke { int first, count; bool curve; };
    struct Glyph { int firstStroke, strokeCount; float w; };

    // Letter strokes as polylines on a unit cap height (y down).
    static const Pt kPts[] = {
        // A
        {0.00f, 1.00f}, {0.42f, 0.00f}, {0.84f, 1.00f},
        {0.20f, 0.62f}, {0.64f, 0.62f},
        // E
        {0.66f, 0.00f}, {0.00f, 0.00f}, {0.00f, 1.00f}, {0.66f, 1.00f},
        {0.00f, 0.50f}, {0.54f, 0.50f},
        // T
        {0.00f, 0.00f}, {0.72f, 0.00f}, {0.36f, 0.00f}, {0.36f, 1.00f},
        // R leg (the bowl is curved, built below)
        {0.40f, 0.50f}, {0.72f, 1.00f},
        // I
        {0.00f, 0.00f}, {0.00f, 1.00f},
    };

    // Curved strokes, sampled finely enough to read as smooth curves. They
    // carry star nodes only where the curve starts, ends, turns at a corner
    // or reaches the middle of a bowl.
    constexpr int kArc = 39;                       // segments per S bowl
    constexpr int kRArc = 24;                      // segments in the R bowl
    constexpr int kSFirst = 0, kSCount = kArc * 2 + 1;
    constexpr int kRFirst = kSFirst + kSCount, kRCount = kRArc + 4;
    constexpr int kCurveCount = kRFirst + kRCount;
    static Pt sCurve[kCurveCount];
    static bool sCurveNode[kCurveCount];
    static bool sCurveBuilt = false;
    if (!sCurveBuilt) {
        constexpr float kDeg = 0.0174533f;
        // S: two stacked elliptical bowls meeting at the centre.
        for (int i = 0; i <= kArc; ++i) {
            const float a = (-35.0f - 235.0f * (float)i / (float)kArc) * kDeg;
            sCurve[kSFirst + i] = { 0.33f + 0.33f * cosf(a), 0.25f + 0.25f * sinf(a) };
        }
        for (int i = 1; i <= kArc; ++i) {
            const float a = (-90.0f + 235.0f * (float)i / (float)kArc) * kDeg;
            sCurve[kSFirst + kArc + i] = { 0.33f + 0.33f * cosf(a), 0.75f + 0.25f * sinf(a) };
        }
        sCurveNode[kSFirst] = sCurveNode[kSFirst + kArc] = sCurveNode[kSFirst + kSCount - 1] = true;
        // R: spine up, then a half-ellipse bowl back to the spine.
        sCurve[kRFirst + 0] = { 0.00f, 1.00f };
        sCurve[kRFirst + 1] = { 0.00f, 0.00f };
        for (int i = 0; i <= kRArc; ++i) {
            const float a = (-90.0f + 180.0f * (float)i / (float)kRArc) * kDeg;
            sCurve[kRFirst + 2 + i] = { 0.40f + 0.28f * cosf(a), 0.25f + 0.25f * sinf(a) };
        }
        sCurve[kRFirst + kRCount - 1] = { 0.00f, 0.50f };
        sCurveNode[kRFirst + 0] = sCurveNode[kRFirst + 1] = true;
        sCurveNode[kRFirst + 2 + kRArc / 2] = true;
        sCurveNode[kRFirst + kRCount - 1] = true;
        sCurveBuilt = true;
    }

    static const Stroke kStrokes[] = {
        {0, 3, false}, {3, 2, false},                  // A
        {5, 4, false}, {9, 2, false},                  // E
        {kSFirst, kSCount, true},                      // S
        {11, 2, false}, {13, 2, false},                // T
        {kRFirst, kRCount, true}, {15, 2, false},      // R
        {17, 2, false},                                // I
    };
    static const Glyph kA = {0, 2, 0.84f};
    static const Glyph kE = {2, 2, 0.66f};
    static const Glyph kS = {4, 1, 0.66f};
    static const Glyph kT = {5, 2, 0.72f};
    static const Glyph kR = {7, 2, 0.72f};
    static const Glyph kI = {9, 1, 0.00f};
    static const Glyph* const kWord[] = { &kA, &kE, &kS, &kT, &kE, &kR, &kI, &kA };
    constexpr int kGlyphCount = (int)(sizeof(kWord) / sizeof(kWord[0]));
    constexpr float kGap = 0.30f;

    float unitW = -kGap;
    for (int i = 0; i < kGlyphCount; ++i) unitW += kWord[i]->w + kGap;

    float logoH = MainLogoHeight(sh) * heightMul;
    float logoW = unitW * logoH;
    const float maxW = sw * 0.66f;
    if (logoW > maxW) {
        logoW = maxW;
        logoH = logoW / unitW;
    }
    const float logoCenterX = (centerXOverride > 0.0f) ? centerXOverride : sw * 0.5f;
    float x0 = logoCenterX - logoW * 0.5f;
    if (x0 < 28.0f) x0 = 28.0f;
    if (x0 + logoW > sw - 28.0f) x0 = sw - logoW - 28.0f;
    const float y0 = MainLogoTop(sh);
    const float now = (float)glfwGetTime();

    // Build the word in screen space.
    constexpr int kMaxSegs = 128;
    constexpr int kMaxNodes = 72;
    struct Seg { float ax, ay, bx, by, len, start; };
    Seg segs[kMaxSegs];
    float nodeX[kMaxNodes], nodeY[kMaxNodes], nodeAt[kMaxNodes];
    int segCount = 0, nodeCount = 0;
    float totalLen = 0.0f;
    float cursor = 0.0f;
    float firstApexX = 0.0f, lastFootX = 0.0f;
    for (int gi = 0; gi < kGlyphCount; ++gi) {
        const Glyph& gl = *kWord[gi];
        const float gx = x0 + cursor * logoH;
        if (gi == 0) firstApexX = gx + 0.42f * logoH;
        if (gi == kGlyphCount - 1) lastFootX = gx + 0.84f * logoH;
        for (int si = 0; si < gl.strokeCount; ++si) {
            const Stroke& st = kStrokes[gl.firstStroke + si];
            const Pt* pts = (st.curve ? sCurve : kPts) + st.first;
            const bool* nodeMask = st.curve ? sCurveNode + st.first : nullptr;
            const int count = st.count;
            float prevX = 0.0f, prevY = 0.0f;
            for (int pi = 0; pi < count; ++pi) {
                const float x = gx + pts[pi].x * logoH;
                const float y = y0 + pts[pi].y * logoH;
                if (pi > 0 && segCount < kMaxSegs) {
                    Seg& s = segs[segCount++];
                    s.ax = prevX; s.ay = prevY;
                    s.bx = x; s.by = y;
                    s.len = sqrtf((s.bx - s.ax) * (s.bx - s.ax) + (s.by - s.ay) * (s.by - s.ay));
                    s.start = totalLen;
                    totalLen += s.len;
                }
                // Straight strokes carry a node at every vertex; curved ones
                // only where their mask marks one.
                const bool node = !nodeMask || nodeMask[pi];
                if (node && nodeCount < kMaxNodes) {
                    nodeX[nodeCount] = x;
                    nodeY[nodeCount] = y;
                    nodeAt[nodeCount] = totalLen;   // trace length that reveals it
                    ++nodeCount;
                }
                prevX = x; prevY = y;
            }
        }
        cursor += gl.w + kGap;
    }

    // Timeline: orbit sweeps in, strokes trace left to right, then the two
    // focal stars ignite (first A, then last A).
    const float orbitT = Smoothstep(LogoClamp01(reveal / 0.40f));
    const float traceT = Smoothstep(LogoClamp01((reveal - 0.14f) / 0.62f));
    const float igniteP = Smoothstep(LogoClamp01((reveal - 0.74f) / 0.14f));
    const float igniteQ = Smoothstep(LogoClamp01((reveal - 0.86f) / 0.14f));
    const float settle = Smoothstep(LogoClamp01((reveal - 0.90f) / 0.10f));
    const float traceLen = totalLen * traceT;

    const float core = std::max(1.8f, logoH * 0.030f);
    const float key = core * 2.6f;
    const float nodeR = std::max(1.6f, logoH * 0.020f);
    const float nodeW = std::max(0.9f, core * 0.45f);
    const float focalR = nodeR * 1.45f;
    const float focalRing = logoH * 0.075f + 0.4f * logoH * 0.01f * sinf(now * 1.6f) * settle;
    const float pX = firstApexX, pY = y0;
    const float qX = lastFootX, qY = y0 + logoH;

    // Line colours (cold star blue). Instead of a hard dark keyline, every
    // stroke sits in a soft deep-blue falloff: wider layers are fainter, so
    // the edge reads as a gradient while still separating from a bright
    // desktop.
    const float lr = 0.32f, lg = 0.72f, lb = 1.00f;
    const float hr = 0.08f, hg = 0.26f, hb = 0.52f;
    const float orr = 0.28f, org = 0.56f, orb = 0.84f;
    constexpr int kHaloSteps = 4;
    auto haloLine = [&](float x1, float y1, float x2, float y2,
                        float width, float a) {
        for (int h = kHaloSteps; h >= 1; --h) {
            const float k = (float)h / (float)kHaloSteps;    // 1 = outermost
            LogoLine(x1, y1, x2, y2, width * (1.0f + 2.4f * k),
                     hr, hg, hb, (0.09f + 0.12f * (1.0f - k)) * a);
        }
    };

    auto nodeVisible = [&](int i) { return nodeAt[i] <= traceLen + 0.5f; };
    auto isFocal = [&](float x, float y) {
        return (fabsf(x - pX) < 0.5f && fabsf(y - pY) < 0.5f)
            || (fabsf(x - qX) < 0.5f && fabsf(y - qY) < 0.5f);
    };

    BindMainShader();

    // Orbit ellipse through P and Q: centred between them, tilted along the
    // diagonal (minus a small offset) and reaching a little past the word.
    const float ocx = (pX + qX) * 0.5f, ocy = (pY + qY) * 0.5f;
    const float halfD = 0.5f * sqrtf((qX - pX) * (qX - pX) + (qY - pY) * (qY - pY));
    const float th = atan2f(qY - pY, qX - pX) - 3.0f * 0.0174533f;
    const float rx = halfD + logoH * 0.714f;
    const float du = pX - ocx, dv = pY - ocy;
    const float lu = du * cosf(-th) - dv * sinf(-th);
    const float lv = du * sinf(-th) + dv * cosf(-th);
    const float ry = fabsf(lv) / sqrtf(std::max(0.05f, 1.0f - (lu * lu) / (rx * rx)));
    const float cth = cosf(th), sth = sinf(th);
    auto orbitPoint = [&](float a, float& ox, float& oy) {
        const float ex = cosf(a) * rx, ey = sinf(a) * ry;
        ox = ocx + ex * cth - ey * sth;
        oy = ocy + ex * sth + ey * cth;
    };
    auto distToWord = [&](float x, float y) {
        float best = 1e9f;
        for (int i = 0; i < segCount; ++i) {
            const Seg& s = segs[i];
            // Only strokes that have started tracing can hide the orbit, so
            // the orbit never shows gaps for letters that are not drawn yet.
            if (s.start >= traceLen) continue;
            const float vx = s.bx - s.ax, vy = s.by - s.ay;
            const float l2 = vx * vx + vy * vy;
            float t = l2 > 0.0f ? ((x - s.ax) * vx + (y - s.ay) * vy) / l2 : 0.0f;
            t = LogoClamp01(t);
            const float ex = s.ax + vx * t - x, ey = s.ay + vy * t - y;
            best = std::min(best, ex * ex + ey * ey);
        }
        return sqrtf(best);
    };

    // A soft night-sky field shaped like the orbit, fading to nothing well
    // past it, so the title stays readable over a bright desktop. This is
    // the one deliberate backdrop exception: it has no edge and only sits
    // behind the title mark.
    DrawRadialGradientRotated(ocx, ocy, rx + logoH * 1.15f, ry + logoH * 1.25f, th,
                              0.024f, 0.047f, 0.11f, 0.85f * alpha * orbitT);
    BindMainShader();

    const float orbitA = alpha * orbitT;
    if (orbitA > 0.001f) {
        constexpr int kOrbitSeg = 160;
        const int drawn = (int)(kOrbitSeg * orbitT + 0.5f);
        const float orbitW = std::max(1.0f, core * 0.48f);
        const float behindGap = key * 0.75f;
        const float joinR = logoH * 0.21f;
        float px, py;
        orbitPoint(3.1415927f, px, py);
        for (int i = 1; i <= drawn; ++i) {
            float x, y;
            orbitPoint(3.1415927f + 6.2831853f * (float)i / (float)kOrbitSeg, x, y);
            const float mx = (px + x) * 0.5f, my = (py + y) * 0.5f;
            const float dP = sqrtf((mx - pX) * (mx - pX) + (my - pY) * (my - pY));
            const float dQ = sqrtf((mx - qX) * (mx - qX) + (my - qY) * (my - qY));
            const bool nearJoin = dP < joinR || dQ < joinR;
            const bool insideFocal = dP < focalR + nodeW || dQ < focalR + nodeW;
            const bool behind = !nearJoin && distToWord(mx, my) < behindGap;
            if (!insideFocal && !behind) {
                haloLine(px, py, x, y, orbitW, 0.80f * orbitA);
                LogoLine(px, py, x, y, orbitW, orr, org, orb, 0.80f * orbitA);
            }
            px = x; py = y;
        }
    }

    // Letter strokes, traced by cumulative length, coloured with a vertical
    // gradient: near-white at the cap line down to deep star blue at the
    // baseline.
    auto gradAt = [&](float y, float& r, float& g, float& b) {
        const float t = LogoClamp01((y - y0) / logoH);
        r = 0.86f + (0.20f - 0.86f) * t;
        g = 0.97f + (0.48f - 0.97f) * t;
        b = 1.00f;
    };
    const float pieceLen = std::max(2.0f, logoH * 0.06f);
    float acc = 0.0f;
    for (int si = 0; si < segCount; ++si) {
        const Seg& s = segs[si];
        const float f = LogoClamp01((traceLen - acc) / std::max(0.001f, s.len));
        acc += s.len;
        if (f <= 0.0f) continue;
        const float ex = s.ax + (s.bx - s.ax) * f;
        const float ey = s.ay + (s.by - s.ay) * f;
        haloLine(s.ax, s.ay, ex, ey, core, alpha);
        // Short pieces so the vertical gradient reads as continuous.
        const int pieces = std::max(1, (int)ceilf(s.len * f / pieceLen));
        float px = s.ax, py = s.ay;
        for (int k = 1; k <= pieces; ++k) {
            const float t = (float)k / (float)pieces;
            const float qx = s.ax + (ex - s.ax) * t, qy = s.ay + (ey - s.ay) * t;
            float r, g, b;
            gradAt((py + qy) * 0.5f, r, g, b);
            LogoLine(px, py, qx, qy, core, r, g, b, 0.98f * alpha);
            px = qx; py = qy;
        }
    }

    // Small solid star dots at every revealed vertex, each in the same soft
    // blue falloff as the strokes. The two orbit joins are drawn separately
    // as focal stars.
    auto starDot = [&](float x, float y, float radius, float a) {
        for (int h = kHaloSteps; h >= 1; --h) {
            const float k = (float)h / (float)kHaloSteps;
            drawCircle(x, y, radius + nodeW * 2.4f * k,
                       hr, hg, hb, (0.09f + 0.12f * (1.0f - k)) * a);
        }
        float r, g, b;
        gradAt(y, r, g, b);
        drawCircle(x, y, radius, r + (1.0f - r) * 0.55f, g + (1.0f - g) * 0.55f,
                   b + (1.0f - b) * 0.55f, a);
    };
    for (int ni = 0; ni < nodeCount; ++ni) {
        if (!nodeVisible(ni) || isFocal(nodeX[ni], nodeY[ni])) continue;
        starDot(nodeX[ni], nodeY[ni], nodeR, 0.95f * alpha);
    }

    const float ign[2] = { igniteP, igniteQ };
    const float fx[2] = { pX, qX }, fy[2] = { pY, qY };
    for (int i = 0; i < 2; ++i) {
        const float a = alpha * ign[i];
        if (a <= 0.001f) continue;
        starDot(fx[i], fy[i], focalR * (0.4f + 0.6f * ign[i]), a);
        const float ring = focalRing * ign[i];
        LogoRing(fx[i], fy[i], ring, nodeW * 0.8f, lr, lg, lb, 0.75f * a);
    }

    // Idle: two small stars drift along the orbit.
    const float driftA = alpha * settle;
    if (driftA > 0.001f) {
        const float a0 = now * 0.22f;
        float sx, sy;
        orbitPoint(3.1415927f + a0, sx, sy);
        starDot(sx, sy, nodeR * 0.85f, 0.80f * driftA);
        orbitPoint(a0, sx, sy);
        starDot(sx, sy, nodeR * 0.70f, 0.50f * driftA);
    }
}

static float MenuCommandReveal(float timeline, int row,
                               float start = 0.04f, float stagger = 0.04f,
                               float duration = 0.28f) {
    return SceneTransitionEase((timeline - start - (float)row * stagger) / duration);
}

static void DrawMainMenuAstralVeil(float sw, float sh,
                                   float menuX, float menuY,
                                   float menuW, float menuH,
                                   const float focusWeights[5],
                                   float alpha) {
    if (alpha <= 0.001f) return;
    const UiThemePalette& theme = GetUiTheme();
    float focus = 0.0f;
    for (int i = 0; i < 5; ++i) focus = std::max(focus, focusWeights[i]);

    // These oversized soft fields extend beyond the menu bounds, so the
    // result reads as a translucent celestial medium rather than a panel.
    DrawRadialGradientRect(menuX - sw * 0.16f, menuY - sh * 0.22f,
                           menuW + sw * 0.38f, menuH + sh * 0.42f,
                           theme.Dim.r, theme.Dim.g, theme.Dim.b,
                           (0.28f + focus * 0.06f) * alpha);
    DrawRadialGradientRect(menuX + menuW * 0.10f, menuY - sh * 0.12f,
                           sw * 0.56f, menuH + sh * 0.26f,
                           theme.Accent.r * 0.10f,
                           theme.Accent.g * 0.08f,
                           theme.Accent.b * 0.13f,
                           (0.12f + focus * 0.04f) * alpha);

}

struct InlineThreeColumnLayout {
    float rootX, rootY, rootW, rootH;
    float contentX, contentY, contentW, contentH;
    float detailX, detailY, detailW, detailH;
    float bottomY;
};

static InlineThreeColumnLayout BuildInlineThreeColumnLayout(float sw, float sh, float uiS,
                                                             float rootX, float rootY,
                                                             float rootW, float rootH) {
    const float sideMargin = std::max(54.0f, 66.0f * uiS);
    const float columnGap = std::max(30.0f, 38.0f * uiS);
    const float contentX = std::max(sw * 0.29f, rootX + rootW + columnGap);
    const float detailX = std::max(sw * 0.65f, contentX + 430.0f * uiS);
    const float topY = rootY - 12.0f * uiS;
    const float bottomY = sh - std::max(48.0f, 58.0f * uiS);

    InlineThreeColumnLayout out{};
    out.rootX = rootX;
    out.rootY = rootY;
    out.rootW = rootW;
    out.rootH = rootH;
    out.contentX = contentX;
    out.contentY = topY;
    out.contentW = std::max(280.0f * uiS, detailX - contentX - columnGap);
    out.contentH = std::max(260.0f * uiS, bottomY - topY);
    out.detailX = detailX;
    out.detailY = topY;
    out.detailW = std::max(280.0f * uiS, sw - sideMargin - detailX);
    out.detailH = out.contentH;
    out.bottomY = bottomY;
    return out;
}

struct ProjectedOrbitPoint {
    float x, y, z;
};

static ProjectedOrbitPoint ProjectOrbitalPoint(float cx, float cy, float radius,
                                               float angle, float tilt, float roll,
                                               float phase) {
    const float a = angle + phase;
    const float lx = cosf(a) * radius;
    const float ly = sinf(a) * radius * sinf(tilt);
    const float lz = sinf(a) * radius * cosf(tilt);
    const float cr = cosf(roll), sr = sinf(roll);
    const float rx = lx * cr - ly * sr;
    const float ry = lx * sr + ly * cr;
    const float perspective = 1.0f + lz / std::max(1.0f, radius * 5.5f);
    return { cx + rx * perspective, cy + ry * perspective, lz };
}

static void DrawMainMenuOrbitalInstrument(float cx, float cy, float radius,
                                          int focusIndex, const float focusWeights[5],
                                          float hoverT,
                                          const ConstellationMotionProfile& motion,
                                          float motionT, float alpha, float uiS,
                                          float r, float g, float b) {
    if (alpha <= 0.001f) return;
    constexpr float kTau = 6.28318530717958647692f;
    radius *= 1.0f + motion.radialPull;
    const int activeOrbit = std::max(0, std::min(focusIndex, 4));

    // A restrained particle shell establishes volume before the brighter
    // armillary paths are drawn over it.
    constexpr float kGolden = 2.39996322972865332f;
    for (int i = 0; i < 84; ++i) {
        const float v = -1.0f + 2.0f * ((float)i + 0.5f) / 84.0f;
        const float shell = sqrtf(std::max(0.0f, 1.0f - v * v));
        const float a = (float)i * kGolden + motionT * (0.035f + (i % 3) * 0.008f);
        const float jitter = 0.88f + 0.18f * sinf((float)i * 7.31f);
        const float rr = radius * 1.34f * jitter;
        const float z = sinf(a) * shell;
        const float px = cx + cosf(a) * shell * rr + z * radius * 0.10f;
        const float py = cy + v * rr * 0.88f + z * radius * 0.045f;
        const float depth = LogoClamp01(0.5f + z * 0.5f);
        const float dustA = (0.040f + depth * 0.040f)
                          * motion.edgeAlpha * alpha;
        DrawConstellationDisc(px, py, (0.65f + depth * 1.10f) * uiS,
                              r, g, b, dustA);
    }

    struct MenuOrbit {
        float scale, tilt, roll, speed, phase;
    };
    const MenuOrbit orbits[5] = {
        {0.92f, 0.24f,  0.08f,  0.34f, 0.20f},
        {1.05f, 0.50f, -0.72f, -0.27f, 1.35f},
        {1.17f, 0.78f,  0.84f,  0.22f, 2.45f},
        {1.28f, 1.02f, -1.08f, -0.18f, 3.65f},
        {1.39f, 0.64f,  1.42f,  0.15f, 4.80f},
    };

    for (int o = 0; o < 5; ++o) {
        const MenuOrbit& od = orbits[o];
        float activeT = LogoClamp01(focusWeights[o]);
        if (o == activeOrbit) activeT = std::max(activeT, hoverT);
        activeT = Smoothstep(activeT);
        const float rr = radius * od.scale * (1.0f + 0.025f * activeT);
        const float alignedRoll = -0.90f + (float)o * 0.45f;
        const float tilt = od.tilt + ((0.62f + (float)(o - 2) * 0.07f) - od.tilt)
                                     * motion.alignmentStrength;
        const float roll = od.roll + (alignedRoll - od.roll)
                                     * motion.alignmentStrength;
        const float phase = od.phase + motionT * od.speed
                          * (1.0f + activeT * 0.38f);
        ProjectedOrbitPoint prev = ProjectOrbitalPoint(cx, cy, rr, 0.0f,
                                                       tilt, roll, phase);
        for (int s = 1; s <= 42; ++s) {
            const float a = kTau * (float)s / 42.0f;
            const ProjectedOrbitPoint p = ProjectOrbitalPoint(cx, cy, rr, a,
                                                              tilt, roll, phase);
            const float depth = LogoClamp01(0.5f + p.z / std::max(1.0f, rr * 2.0f));
            const float pathA = (0.065f + activeT * 0.42f)
                              * (0.62f + depth * 0.60f)
                              * motion.edgeAlpha * alpha;
            const float midX = (prev.x + p.x) * 0.5f - cx;
            const float midY = (prev.y + p.y) * 0.5f - cy;
            const bool behindCore = depth < 0.46f
                                 && midX * midX + midY * midY < radius * radius * 0.092f;
            if (!behindCore)
                DrawVisibleConstellLine(prev.x, prev.y, p.x, p.y,
                                        (0.56f + activeT * 0.62f) * uiS,
                                        r, g, b, pathA);
            prev = p;
        }

        const float starA = phase + 0.42f + (float)o * 0.61f;
        const ProjectedOrbitPoint star = ProjectOrbitalPoint(cx, cy, rr, starA,
                                                             tilt, roll, 0.0f);
        const float pulseAmount = 0.18f * motion.nodePulse;
        const float pulse = 1.0f - pulseAmount
                          + pulseAmount * sinf(motionT * 2.2f + o * 1.4f);
        if (activeT > 0.04f) {
            for (int trail = 4; trail >= 1; --trail) {
                const float trailAngle = starA - od.speed * (float)trail * 0.11f;
                const ProjectedOrbitPoint tail = ProjectOrbitalPoint(
                    cx, cy, rr, trailAngle, tilt, roll, 0.0f);
                const float tailA = activeT * alpha * (0.045f + (4 - trail) * 0.035f);
                DrawConstellationDisc(tail.x, tail.y,
                                      (0.9f + (4 - trail) * 0.35f) * uiS,
                                      r, g, b, tailA);
            }
        }
        const float starSize = (3.0f + activeT * 7.2f) * pulse * uiS;
        DrawConstellationDisc(star.x, star.y, starSize * 2.05f,
                               0.0f, 0.0f, 0.014f, (0.13f + activeT * 0.12f) * alpha);
        DrawConstellationDisc(star.x, star.y, starSize * 1.28f,
                               r, g, b, (0.12f + activeT * 0.20f)
                               * motion.edgeAlpha * alpha);
        DrawConstellationDisc(star.x, star.y, std::max(1.4f, starSize * 0.34f),
                              0.98f, 1.0f, 1.0f, (0.54f + activeT * 0.42f) * alpha);
    }

    // The compact inner body gives the five orbital planes a clear anchor.
    const float coreR = radius * 0.27f;
    // Three-layer CircleTexture treatment: broad dark contrast, a restrained
    // field for the surrounding small constellations, and a focused center.
    DrawConstellationDisc(cx, cy, coreR * 2.45f,
                          0.0f, 0.0f, 0.014f, 0.34f * alpha);
    DrawConstellationDisc(cx, cy, coreR * 1.58f,
                          r, g, b, 0.075f * alpha);
    DrawConstellationDisc(cx, cy, coreR * 0.88f,
                          r, g, b, 0.21f * alpha);
    for (int lat = -3; lat <= 3; ++lat) {
        const float n = (float)lat / 4.0f;
        const float y = cy + n * coreR;
        const float bandR = coreR * sqrtf(std::max(0.0f, 1.0f - n * n));
        float lastX = 0.0f, lastY = 0.0f;
        for (int s = 0; s <= 28; ++s) {
            const float a = kTau * (float)s / 28.0f + motionT * 0.21f;
            const float z = sinf(a) * bandR;
            const float x = cx + cosf(a) * bandR;
            const float py = y + z * 0.16f;
            if (s > 0) {
                LogoLine(lastX, lastY, x, py, 1.20f * uiS,
                         0.0f, 0.0f, 0.014f, 0.18f * alpha);
                LogoLine(lastX, lastY, x, py, 0.52f * uiS,
                         r, g, b, (0.16f + 0.11f * (z / coreR + 1.0f)) * alpha);
            }
            lastX = x;
            lastY = py;
        }
    }
    for (int lon = 0; lon < 7; ++lon) {
        const float roll = (float)lon * kTau / 7.0f + motionT * 0.13f;
        float lastX = 0.0f, lastY = 0.0f;
        for (int s = 0; s <= 24; ++s) {
            const float a = -1.5707963f + 3.1415926f * (float)s / 24.0f;
            const float ringR = cosf(a) * coreR;
            const float z = sinf(roll) * ringR;
            const float x = cx + cosf(roll) * ringR;
            const float y = cy + sinf(a) * coreR + z * 0.16f;
            if (s > 0) {
                LogoLine(lastX, lastY, x, y, 1.10f * uiS,
                         0.0f, 0.0f, 0.014f, 0.16f * alpha);
                LogoLine(lastX, lastY, x, y, 0.48f * uiS,
                         r, g, b, (0.12f + 0.10f * (z / coreR + 1.0f)) * alpha);
            }
            lastX = x;
            lastY = y;
        }
    }
    DrawConstellationDisc(cx, cy, (5.8f + 1.4f * sinf(motionT * 2.6f)) * uiS,
                          r, g, b, 0.62f * motion.nodePulse * alpha);
    DrawConstellationDisc(cx, cy, 2.1f * uiS,
                          1.0f, 1.0f, 1.0f, 0.96f * alpha);
}
#include "SceneMainMenu.inl"

#include "SceneShop.inl"

#include "SceneCodexInline.inl"

#include "SceneCodex.inl"

bool TryEscNavigateBack() {
    using GS = GameState;
    GameState& st = g_GameManager.currentState;
    switch (st) {
    case GS::AUG_SELECT:
    case GS::DEBUFF_SELECT:
    case GS::AUG_REPLACE:
        // Keep the selection screen as the resume target. ESC pauses the
        // run here; it must not cancel or consume the pending choice.
        g_GameManager.pauseResumeState = st;
        st = GS::PAUSED;
        return true;
    case GS::CODEX:
        CodexSearchClear();
        st = GS::MAIN_MENU;
        return true;
    case GS::TUTORIAL:
        st = GS::MAIN_MENU;
        return true;
    case GS::CREATIVE_CONFIG:
        st = GS::MAIN_MENU;
        return true;
    case GS::SETTINGS:
        SaveGame();
        st = g_SettingsReturnTo;
        return true;
    default:
        return false;
    }
}
