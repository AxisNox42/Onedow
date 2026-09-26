#include "Scenes.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "GameManager.h"
#include "GameContext.h"
#include "SceneSkills.h"
#include "SceneUI.h"
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
#include "TextRenderer.h"
#include "IconSystem.h"
#include "../System/SystemInfo.h"
#include "Camera.h"
#include "EntityDraw.h"
#include "Monster.h"
#include "RunIntermission.h"
#include "BossDirector.h"
#include "AugmentSlots.h"
#include "../../Render/Juice.h"
#include <algorithm>
#include <vector>
#include <string>
#include <cmath>
#include <functional>

constexpr float SCENE_BG_ALPHA = 0.0f;
static float g_MainMenuEntryT  = 0.0f;
static float g_SettingsEntryT  = 0.0f;
static float g_CodexEntryT     = 0.0f;
static float g_ShopEntryT      = 0.0f;
// Page textures use one reveal gate so CircleTexture layers do not pop in as
// independent draw calls during a scene handoff.  The gate is changed only
// by page entry/exit animation, never by list selection or hover state.
static float g_SceneTextureReveal = 1.0f;
static bool  s_ShopBackRequested = false;
static bool  s_CodexBackRequested = false;
static bool  s_MainMenuArmoryPanel = false;
static bool  s_MainMenuCodexPanel = false;
static bool  s_MainMenuSettingsPanel = false;
static bool  s_SettingsInlineNeedsReset = false;
static bool  s_MainMenuRunConfigPanel = false;
static bool  s_MainMenuResumeFromPanel = false;
static void Scene_RunConfigInline(const SceneCtx& c);
static void Scene_SettingsInline(const SceneCtx& c);
static int   s_RcWeapon        = 0;
// PLAY owns the current trial catalogue. The gameplay system still has four
// slots; this UI state packs enabled definitions into those slots on PLAY.
static bool  s_RcTrialEnabled[TRIAL_DEF_COUNT] = {};
static bool  s_RcTrialStateLoaded = false;

static float UiApproach(float v, float target, float dt, float speed) {
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
static const wchar_t* MainMenuRouteLabel(int language, int index) {
    static const wchar_t* kRoutes[3][5] = {
        { L"플레이", L"상점", L"도감", L"설정", L"종료" },
        { L"PLAY", L"SHOP", L"ASTRAL LOG", L"SETTINGS", L"EXIT" },
        { L"スタート", L"ショップ", L"星界記録", L"設定", L"終了" },
    };
    const int li = std::max(0, std::min(2, language));
    const int mi = std::max(0, std::min(4, index));
    return kRoutes[li][mi];
}

static const wchar_t* MainMenuSubtitleLabel(int language, int index) {
    static const wchar_t* kSubtitles[3][5] = {
        { L"전투 시작", L"무기고", L"도감 기록", L"설정", L"게임 종료" },
        { L"Start", L"Armory", L"Astral Log", L"Setting", L"Exit" },
        { L"スタート", L"武器庫", L"星界記録", L"設定", L"終了" },
    };
    const int li = std::max(0, std::min(2, language));
    const int mi = std::max(0, std::min(4, index));
    return kSubtitles[li][mi];
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

static void ResetRunConfigUi() {
    g_Difficulty = Difficulty::NORMAL;
    ResetTrials();
    s_RcWeapon = 0;
    s_RcTrialStateLoaded = false;
    for (int i = 0; i < TRIAL_DEF_COUNT; ++i) s_RcTrialEnabled[i] = false;
}

static void ResetSettingsUi() {
    g_SettingsEntryT = 0.0f;
    s_SettingsInlineNeedsReset = true;
}

static void ResetCodexUi() {
    g_CodexEntryT = 0.0f;
    s_CodexBackRequested = false;
    CodexSearchClear();
    g_CodexSearchInputEnabled = false;
}

static void ResetShopUi() {
    g_ShopEntryT = 0.0f;
    s_ShopBackRequested = false;
}

static void ResetMainMenuUi() {
    g_MainMenuEntryT = 0.0f;
    s_MainMenuArmoryPanel = false;
    s_MainMenuCodexPanel = false;
    s_MainMenuSettingsPanel = false;
    s_MainMenuRunConfigPanel = false;
    s_MainMenuResumeFromPanel = false;
    s_MainMenuConstellationMotion = {};
}

// 메인메뉴·난이도선택 공용 앰비언트 배경 (파티클 + 스캔라인 + 선택적 비네트)
static void DrawMenuBackground(float sw, float sh, float delta, float darkenAmount = 1.0f) {
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

static float LogoClamp01(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

static void SetSceneTextureReveal(float reveal) {
    g_SceneTextureReveal = LogoClamp01(reveal);
}

static void LogoLine(float x1, float y1, float x2, float y2,
                     float thick, float r, float g, float b, float a) {
    float dx = x2 - x1, dy = y2 - y1;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.5f || thick <= 0.0f || a <= 0.0f) return;
    float nx = -dy / len * thick * 0.5f;
    float ny =  dx / len * thick * 0.5f;
    BatchTri(x1 + nx, y1 + ny, x1 - nx, y1 - ny, x2 + nx, y2 + ny, r, g, b, a);
    BatchTri(x1 - nx, y1 - ny, x2 - nx, y2 - ny, x2 + nx, y2 + ny, r, g, b, a);
}

static void DrawSceneLeftVignette(float sw, float sh, float alpha) {
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
static constexpr float kWideSceneLinearWidth = 0.58f;
static constexpr float kWideSceneLinearAlpha = 0.68f;

static void DrawPersistentSceneSideVignettes(float sw, float sh, float alpha) {
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

static void DrawSceneRadialVignette(float cx, float cy, float radius, float alpha) {
    if (alpha <= 0.001f || radius <= 1.0f) return;
    const UiThemePalette& theme = GetUiTheme();
    DrawRadialGradient(cx, cy, radius, theme.Dim.r, theme.Dim.g, theme.Dim.b,
                       alpha * theme.Dim.a * 0.96f);
}

static void DrawSettingsOrbitArc(float cx, float cy, float radiusX,
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

static float MainMenuCommandStartY(float sh) {
    constexpr float kRowH = 70.0f;
    constexpr float kGap = 15.0f;
    constexpr float kRowCount = 5.0f;
    const float totalH = kRowCount * kRowH + (kRowCount - 1.0f) * kGap;
    float y = sh * 0.48f;
    if (y + totalH > sh - 54.0f) y = sh - totalH - 54.0f;
    const float logoFloor = MainLogoTop(sh) + MainLogoHeight(sh) * 0.72f;
    if (y < logoFloor) y = logoFloor;
    return y;
}

// The lobby command rail is the canonical home for the main-menu buttons.
// Scene transition ghosts must use this same anchor; keeping the old centered
// command Y here makes the previous button stack flash before it exits.
static float MainMenuButtonRailStartY(float sh) {
    constexpr float kRowH = 70.0f;
    constexpr float kGap = 15.0f;
    constexpr float kRowCount = 5.0f;
    const float totalH = kRowCount * kRowH + (kRowCount - 1.0f) * kGap;
    float y = sh - totalH - std::max(52.0f, sh * 0.075f);
    const float logoFloor = MainLogoTop(sh) + MainLogoHeight(sh) * 0.82f;
    if (y < logoFloor) y = logoFloor;
    return y;
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

static void DrawReadableSurface(float x, float y, float w, float h, float alpha) {
    if (alpha <= 0.001f || w <= 1.0f || h <= 1.0f) return;
    const UiThemePalette& theme = GetUiTheme();
    const float featherX = std::min(40.0f, w * 0.08f);
    const float featherY = std::min(28.0f, h * 0.10f);

    // Keep a restrained opacity floor so labels near the panel edges do not
    // fall back onto the scene background as the radial mask fades out.
    BindMainShader();
    drawRect(x, y, w, h,
             theme.Panel.r, theme.Panel.g, theme.Panel.b,
             alpha * theme.Panel.a * 0.52f);
    DrawRadialGradientRect(x - featherX, y - featherY,
                           w + featherX * 2.0f, h + featherY * 2.0f,
                           theme.Panel.r, theme.Panel.g, theme.Panel.b,
                           alpha * theme.Panel.a * 0.92f);
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

struct SceneTextCommand {
    TextRenderer* renderer = nullptr;
    std::wstring text;
    float x = 0.0f, y = 0.0f, scale = 1.0f;
    float r = 1.0f, g = 1.0f, b = 1.0f, alpha = 1.0f;
    float shadowAlpha = 0.68f;
};

// SHOP has a large amount of late-drawn CircleTexture geometry.  Queueing its
// text lets the page finish all background/icon work first, then emits one
// final unlit text pass above that geometry.
static bool g_DeferSceneText = false;
static std::vector<SceneTextCommand> g_SceneTextCommands;

static void DrawShadowedTextImmediate(TextRenderer& renderer, const wchar_t* text,
                                      float x, float y, float scale,
                                      float r, float g, float b, float alpha,
                                      float shadowAlpha) {
    if (!text || alpha <= 0.001f) return;

    // Text is a UI signal, not part of the scene's light field.  The old
    // four-direction keyline drew almost a complete second glyph around every
    // label, which made small text look dirty and noticeably darker over the
    // page textures.  Keep only a restrained offset shadow for separation;
    // the actual label is still emitted last at its requested colour/alpha.
    const float shadowOffset = std::max(0.60f, std::min(1.00f, scale * 0.72f));
    const float readableShadowAlpha = std::min(0.16f,
                                               alpha * shadowAlpha * 0.24f);
    renderer.Draw(text, x + shadowOffset, y + shadowOffset, scale,
                  0.0f, 0.0f, 0.012f, readableShadowAlpha);
    renderer.Draw(text, x, y, scale, r, g, b, alpha);
}

static void DrawShadowedText(TextRenderer& renderer, const wchar_t* text,
                             float x, float y, float scale,
                             float r, float g, float b, float alpha,
                             float shadowAlpha = 0.68f) {
    if (!text || alpha <= 0.001f) return;
    if (g_DeferSceneText) {
        SceneTextCommand cmd;
        cmd.renderer = &renderer;
        cmd.text = text;
        cmd.x = x; cmd.y = y; cmd.scale = scale;
        cmd.r = r; cmd.g = g; cmd.b = b;
        cmd.alpha = alpha;
        cmd.shadowAlpha = shadowAlpha;
        g_SceneTextCommands.emplace_back(std::move(cmd));
        return;
    }
    DrawShadowedTextImmediate(renderer, text, x, y, scale,
                              r, g, b, alpha, shadowAlpha);
}

static void FlushSceneTextCommands() {
    if (g_SceneTextCommands.empty()) return;
    const bool wasDeferred = g_DeferSceneText;
    g_DeferSceneText = false;
    for (const SceneTextCommand& cmd : g_SceneTextCommands) {
        if (cmd.renderer && !cmd.text.empty()) {
            DrawShadowedTextImmediate(*cmd.renderer, cmd.text.c_str(),
                                      cmd.x, cmd.y, cmd.scale,
                                      cmd.r, cmd.g, cmd.b,
                                      cmd.alpha, cmd.shadowAlpha);
        }
    }
    g_SceneTextCommands.clear();
    g_DeferSceneText = wasDeferred;
}

static void DrawVisibleConstellLine(float x1, float y1, float x2, float y2,
                                    float thick, float r, float g, float b, float a) {
    if (a <= 0.001f) return;
    if (g_ConstellationLineTex) {
        const float dx = x2 - x1, dy = y2 - y1;
        const float len = sqrtf(dx * dx + dy * dy);
        if (len > 0.5f) {
            const float ang = atan2f(dy, dx);
            DrawIconRot(g_ConstellationLineTex,
                        (x1 + x2) * 0.5f, (y1 + y2) * 0.5f,
                        len * 0.5f, thick * 2.4f, ang,
                        0.0f, 0.0f, 0.012f, 0.30f * a);
            DrawIconRot(g_ConstellationLineTex,
                        (x1 + x2) * 0.5f, (y1 + y2) * 0.5f,
                        len * 0.5f, thick * 0.72f, ang,
                        r, g, b, a);
            return;
        }
    }
    LogoLine(x1, y1, x2, y2, thick * 2.75f, 0.0f, 0.0f, 0.012f, 0.42f * a);
    LogoLine(x1, y1, x2, y2, thick, r, g, b, a);
}

static void DrawConstellationDisc(float x, float y, float radius,
                                  float r, float g, float b, float a,
                                  bool darkenBlend = true) {
    a *= g_SceneTextureReveal;
    if (a <= 0.001f || radius <= 0.1f) return;
    if (g_ConstellationCircleTex) {
        const bool darken = darkenBlend
                         && r <= 0.02f && g <= 0.02f && b <= 0.02f;
        if (darken) {
            DrawIconDarken(g_ConstellationCircleTex,
                           x - radius, y - radius, radius * 2.0f, radius * 2.0f,
                           r, g, b, a);
        } else {
            DrawIcon(g_ConstellationCircleTex,
                     x - radius, y - radius, radius * 2.0f, radius * 2.0f,
                     r, g, b, a);
        }
    } else {
        drawCircle(x, y, radius, r, g, b, a);
    }
}

static void ShopStaticFieldPalette(int tab, float& r, float& g, float& b) {
    switch (tab) {
    case 1: // CORE UNLOCKS: cold ice
        r = 0.38f; g = 0.74f; b = 1.00f;
        break;
    case 2: // RIFLE: restrained green-gold
        r = 0.72f; g = 0.88f; b = 0.48f;
        break;
    case 3: // FIELD: violet signal
        r = 0.58f; g = 0.50f; b = 1.00f;
        break;
    default: // START: cyan origin
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

static void DrawVisibleConstellNode(float x, float y, float size,
                                    float r, float g, float b, float a,
                                    bool whiteSpark = true,
                                    bool drawField = true) {
    if (a <= 0.001f) return;
    if (drawField) {
        // Keep the optical halo subordinate to the constellation geometry.
        // The node diamond is the signal; these fields only separate it from
        // the background and should not read as a second, larger star.
        DrawConstellationDisc(x, y, size * 1.85f, 0.0f, 0.0f, 0.012f, 0.34f * a);
        DrawConstellationDisc(x, y, size * 1.15f, r * 0.08f, g * 0.08f, b * 0.10f, 0.26f * a);
        DrawConstellationDisc(x, y, size * 0.88f, r, g, b, 0.58f * a);
    }
    drawDiamond(x, y, size * 1.20f, r, g, b, 0.74f * a);
    drawDiamond(x, y, size * 0.42f,
                whiteSpark ? 0.96f : r,
                whiteSpark ? 1.0f  : g,
                whiteSpark ? 1.0f  : b,
                0.88f * a);
}

static void DrawArchiveConstellation(float cx, float cy, float radius,
                                     int category, int key, float now,
                                     float r, float g, float b,
                                     float alpha, float uiS,
                                     bool drawNodeFields = true) {
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

// Fixed ASTRAL_LOG signature constellation. Unlike record previews this shape
// never changes with selection; only its restrained phase/pulse animates.
static void DrawAstralCoreConstellation(float cx, float cy, float radius,
                                        float r, float g, float b,
                                        float now, float alpha, float uiS) {
    if (alpha <= 0.001f || radius <= 4.0f) return;
    static const float kNodes[8][2] = {
        { 0.00f, -0.92f }, { 0.58f, -0.54f }, { 0.84f, 0.08f },
        { 0.46f,  0.68f }, { -0.12f, 0.88f }, { -0.72f, 0.52f },
        { -0.86f,-0.14f }, { -0.48f,-0.66f }
    };
    static const int kEdges[10][2] = {
        {0,1},{1,2},{2,3},{3,4},{4,5},{5,6},{6,7},{7,0},{1,6},{2,5}
    };
    const float phase = now * 0.045f;
    const float pulse = 0.92f + 0.08f * sinf(now * 1.7f);
    float px[8], py[8];
    const float cs = cosf(phase), sn = sinf(phase);
    for (int i = 0; i < 8; ++i) {
        const float lx = kNodes[i][0] * radius;
        const float ly = kNodes[i][1] * radius;
        px[i] = cx + lx * cs - ly * sn;
        py[i] = cy + lx * sn + ly * cs;
    }
    for (int i = 0; i < 10; ++i) {
        const int a = kEdges[i][0], d = kEdges[i][1];
        DrawVisibleConstellLine(px[a], py[a], px[d], py[d],
                                1.15f * uiS, r, g, b, 0.42f * alpha);
    }
    // Two broken orbital rings keep the core legible without radial spokes.
    for (int ring = 0; ring < 2; ++ring) {
        const float rr = radius * (0.44f + 0.22f * (float)ring);
        float ox = cx + cosf(phase + 0.42f) * rr;
        float oy = cy + sinf(phase + 0.42f) * rr * 0.72f;
        for (int s = 1; s <= 36; ++s) {
            const float a = phase + 0.42f + 6.2831853f * (float)s / 36.0f;
            const float nx = cx + cosf(a) * rr;
            const float ny = cy + sinf(a) * rr * 0.72f;
            if ((s + ring) % 4 != 1)
                LogoLine(ox, oy, nx, ny, 0.62f * uiS, r, g, b,
                         (0.12f - 0.025f * (float)ring) * alpha);
            ox = nx; oy = ny;
        }
    }
    for (int i = 0; i < 8; ++i) {
        const float size = (i == 0 || i == 4 ? 6.2f : 4.3f) * uiS;
        DrawVisibleConstellNode(px[i], py[i], size, r, g, b,
                                pulse * 0.72f * alpha, false);
    }
    DrawConstellationDisc(cx, cy, radius * 0.42f,
                          r, g, b, 0.16f * pulse * alpha);
    DrawConstellationDisc(cx, cy, radius * 0.17f,
                          r, g, b, 0.42f * pulse * alpha);
    DrawVisibleConstellNode(cx, cy, 8.0f * uiS, r, g, b,
                            0.96f * pulse * alpha, false);
    drawDiamond(cx, cy, 2.5f * uiS, 0.86f, 0.92f, 0.98f,
                0.72f * pulse * alpha);
}

static void DrawMainOnedowLogo(float sw, float sh, float alpha, float reveal,
                               float centerXOverride = -1.0f, float heightMul = 1.0f) {
    if (alpha <= 0.001f) return;

    struct Pt { float x, y; };
    struct Edge { int a, b; };
    struct Glyph {
        const Pt* pts;
        int ptCount;
        const Edge* edges;
        int edgeCount;
        float w;
    };

    static const Pt O_P[] = {
        {0.16f, 0.00f}, {0.74f, 0.00f}, {0.92f, 0.18f}, {0.92f, 0.82f},
        {0.74f, 1.00f}, {0.16f, 1.00f}, {0.00f, 0.82f}, {0.00f, 0.18f}
    };
    static const Edge O_E[] = {
        {0,1}, {1,2}, {2,3}, {3,4}, {4,5}, {5,6}, {6,7}, {7,0}
    };
    static const Pt N_P[] = {
        {0.00f, 1.00f}, {0.00f, 0.00f}, {0.82f, 1.00f}, {0.82f, 0.00f}
    };
    static const Edge N_E[] = { {0,1}, {1,2}, {2,3} };
    static const Pt E_P[] = {
        {0.00f, 0.00f}, {0.00f, 0.50f}, {0.00f, 1.00f},
        {0.78f, 0.00f}, {0.64f, 0.50f}, {0.78f, 1.00f}
    };
    static const Edge E_E[] = { {0,2}, {0,3}, {1,4}, {2,5} };
    static const Pt D_P[] = {
        {0.00f, 0.00f}, {0.58f, 0.00f}, {0.88f, 0.26f},
        {0.88f, 0.74f}, {0.58f, 1.00f}, {0.00f, 1.00f}
    };
    static const Edge D_E[] = { {0,1}, {1,2}, {2,3}, {3,4}, {4,5}, {5,0} };
    static const Pt W_P[] = {
        {0.00f, 0.00f}, {0.18f, 1.00f}, {0.52f, 0.48f},
        {0.86f, 1.00f}, {1.08f, 0.00f}
    };
    static const Edge W_E[] = { {0,1}, {1,2}, {2,3}, {3,4} };
    static const Glyph glyphs[] = {
        { O_P, 8, O_E, 8, 0.92f },
        { N_P, 4, N_E, 3, 0.82f },
        { E_P, 6, E_E, 4, 0.78f },
        { D_P, 6, D_E, 6, 0.88f },
        { O_P, 8, O_E, 8, 0.92f },
        { W_P, 5, W_E, 4, 1.08f },
    };

    const int glyphCount = (int)(sizeof(glyphs) / sizeof(glyphs[0]));
    const float gap = 0.19f;
    float unitW = -gap;
    int totalEdges = 0;
    for (int i = 0; i < glyphCount; ++i) {
        unitW += glyphs[i].w + gap;
        totalEdges += glyphs[i].edgeCount;
    }

    float logoH = MainLogoHeight(sh) * heightMul;
    float logoW = unitW * logoH;
    const float maxW = sw * 0.66f;
    if (logoW > maxW) {
        logoW = maxW;
        logoH = logoW / unitW;
    }

    float logoCenterX = (centerXOverride > 0.0f) ? centerXOverride : sw * 0.5f;
    float x0 = logoCenterX - logoW * 0.5f;
    if (x0 < 28.0f) x0 = 28.0f;
    if (x0 + logoW > sw - 28.0f) x0 = sw - logoW - 28.0f;
    const float y0 = MainLogoTop(sh);
    const float cx = x0 + logoW * 0.5f;
    const float cy = y0 + logoH * 0.52f;
    const float now = (float)glfwGetTime();
    const float trace = Smoothstep(LogoClamp01(reveal * 1.10f));
    const float settle = Smoothstep(LogoClamp01((reveal - 0.42f) / 0.58f));
    const float stroke = std::max(6.0f, logoH * 0.082f);

    BindMainShader();

    int edgeIndex = 0;
    float cursor = 0.0f;
    for (int gi = 0; gi < glyphCount; ++gi) {
        const Glyph& gl = glyphs[gi];
        for (int ei = 0; ei < gl.edgeCount; ++ei, ++edgeIndex) {
            const Pt& pa = gl.pts[gl.edges[ei].a];
            const Pt& pb = gl.pts[gl.edges[ei].b];
            float et = Smoothstep(LogoClamp01(trace * (float)totalEdges - (float)edgeIndex));
            if (et <= 0.001f) continue;
            float ax = x0 + (cursor + pa.x) * logoH;
            float ay = y0 + pa.y * logoH;
            float bx = x0 + (cursor + pb.x) * logoH;
            float by = y0 + pb.y * logoH;
            bx = ax + (bx - ax) * et;
            by = ay + (by - ay) * et;

            LogoLine(ax, ay, bx, by, stroke * 1.55f, 0.03f, 0.06f, 0.10f, 0.82f * alpha);
            LogoLine(ax, ay, bx, by, stroke * 1.08f, 0.04f, 0.18f, 0.34f, 0.94f * alpha);
            LogoLine(ax, ay, bx, by, stroke * 0.54f, 0.32f, 0.72f, 1.00f, 0.98f * alpha);
        }
        cursor += gl.w + gap;
    }

    // Sparse orbital constellation around the solid logo.
    const float orbitA = alpha * settle;
    if (orbitA > 0.001f) {
        const float rx0 = logoW * 0.60f;
        const float ry0 = logoH * 0.84f;
        const float rot0 = now * 0.22f;
        const float rot1 = -now * 0.15f + 1.35f;

        auto orbitPoint = [&](float ang, float rx, float ry, float rot, float& ox, float& oy) {
            float ca = cosf(ang), sa = sinf(ang);
            float cr = cosf(rot), sr = sinf(rot);
            float lx = ca * rx;
            float ly = sa * ry;
            ox = cx + lx * cr - ly * sr;
            oy = cy + lx * sr + ly * cr;
        };

        const int arcSteps = 42;
        for (int i = 0; i < arcSteps; ++i) {
            if ((i % 4) == 1) continue;
            float a0 = (float)i / (float)arcSteps * 6.2831853f;
            float a1 = ((float)i + 0.55f) / (float)arcSteps * 6.2831853f;
            float x1, y1, x2, y2;
            orbitPoint(a0, rx0, ry0, rot0, x1, y1);
            orbitPoint(a1, rx0, ry0, rot0, x2, y2);
            LogoLine(x1, y1, x2, y2, 1.1f, 0.18f, 0.62f, 0.96f, 0.28f * orbitA);
        }

        const int nodeCount = 7;
        float px[nodeCount], py[nodeCount];
        for (int i = 0; i < nodeCount; ++i) {
            float ang = (float)i / (float)nodeCount * 6.2831853f + now * 0.18f;
            float rx = rx0 * (0.86f + 0.08f * sinf((float)i * 1.7f));
            float ry = ry0 * (0.88f + 0.07f * cosf((float)i * 1.3f));
            orbitPoint(ang, rx, ry, rot1, px[i], py[i]);
        }
        for (int i = 0; i < nodeCount; ++i) {
            int j = (i + 2) % nodeCount;
            if ((i % 2) == 0)
                LogoLine(px[i], py[i], px[j], py[j], 0.95f, 0.42f, 0.62f, 1.0f, 0.12f * orbitA);
        }
        for (int i = 0; i < nodeCount; ++i) {
            float tw = 0.78f + 0.22f * sinf(now * 2.5f + (float)i * 0.9f);
            float sz = (i == 0 || i == 3) ? 7.0f : 4.6f;
            DrawConstellationDisc(px[i], py[i], sz * 1.75f,
                                  0.0f, 0.018f, 0.050f, 0.30f * orbitA);
            DrawConstellationDisc(px[i], py[i], sz,
                                  0.42f, 0.82f, 1.0f,
                                  (0.58f + 0.18f * tw) * orbitA);
            DrawConstellationDisc(px[i], py[i], sz * 0.34f,
                                  1.0f, 1.0f, 1.0f, 0.62f * orbitA);
        }

        float lY = y0 + logoH + 18.0f;
        LogoLine(x0 + logoW * 0.08f, lY, x0 + logoW * 0.43f, lY,
                 1.4f, 0.38f, 0.82f, 1.0f, 0.28f * orbitA);
        LogoLine(x0 + logoW * 0.57f, lY, x0 + logoW * 0.92f, lY,
                 1.4f, 0.38f, 0.82f, 1.0f, 0.28f * orbitA);
        drawDiamond(cx, lY, 5.5f, 0.72f, 0.48f, 1.0f, 0.54f * orbitA);
    }
}

static void DrawMenuCommandFeedback(float x, float y, float w, float h,
                                    float r, float g, float b, float alpha,
                                    float active, float pulse, float now,
                                    PanelButtonSlideSide slideSide =
                                        PanelButtonSlideSide::Both) {
    if (active <= 0.01f || alpha <= 0.001f) return;
    BindMainShader();
    const float lineH = 20.0f + 42.0f * active + 10.0f * pulse;
    const float barA = (0.24f + 0.55f * active + 0.12f * pulse) * alpha;
    const bool showLeft = slideSide == PanelButtonSlideSide::Left
                       || slideSide == PanelButtonSlideSide::Both;
    const bool showRight = slideSide == PanelButtonSlideSide::Right
                        || slideSide == PanelButtonSlideSide::Both;
    if (showLeft || showRight) {
        // Bottom commands use paired slidebars. Their offset grows with the
        // eased hover value, making the bars visibly travel outward instead
        // of appearing as static brackets.
        const float slide = 10.0f + 14.0f * active + 3.0f * pulse;
        if (showLeft) {
            drawRect(x - slide, y + h * 0.5f - lineH * 0.5f,
                     2.2f, lineH, r, g, b, barA);
            drawDiamond(x - slide - 6.0f, y + h * 0.5f,
                        3.0f + 2.6f * active + 1.8f * pulse,
                        r, g, b,
                        (0.40f + 0.34f * active + 0.16f * pulse) * alpha);
        }
        if (showRight) {
            drawRect(x + w + slide - 2.2f, y + h * 0.5f - lineH * 0.5f,
                     2.2f, lineH, r, g, b, barA);
            drawDiamond(x + w + slide + 4.0f, y + h * 0.5f,
                        3.0f + 2.6f * active + 1.8f * pulse,
                        r, g, b,
                        (0.40f + 0.34f * active + 0.16f * pulse) * alpha);
        }
    }
    if (slideSide != PanelButtonSlideSide::None) {
        const float scanY = y + h * (0.34f + 0.32f * fmodf(now * 0.55f, 1.0f));
        drawRect(x - 4.0f, scanY, w * (0.58f + 0.24f * active), 1.1f,
                 r, g, b, (0.050f + 0.090f * active) * alpha);
    }
}

static float UpdateMenuCommandHover(float current, bool hovered, float dt) {
    const float step = std::min(1.0f, dt * 10.0f);
    return current + ((hovered ? 1.0f : 0.0f) - current) * step;
}

static float MenuCommandReveal(float timeline, int row,
                               float start = 0.20f, float stagger = 0.14f,
                               float duration = 0.32f) {
    return Smoothstep(LogoClamp01((timeline - start - (float)row * stagger) / duration));
}

static void DrawPanelButtonCore(const wchar_t* route, const wchar_t* sub,
                                float x, float y, float w, float h,
                                float r, float g, float b, float alpha,
                                float hover, bool selected, float pulse, float now,
                                float routeScale = 1.04f, float subScale = 0.48f,
                                bool keepTextReadable = false,
                                PanelButtonSlideSide slideSide =
                                    PanelButtonSlideSide::Both,
                                bool centerText = false,
                                bool centerSingleLine = false) {
    const float active = std::max(hover, selected ? 1.0f : 0.0f);
    DrawMenuCommandFeedback(x, y, w, h, r, g, b, alpha,
                            active, pulse, now, slideSide);

    while (routeScale > 0.72f && g_TextL.Width(route, routeScale) > w)
        routeScale -= 0.04f;
    const bool hasSub = sub && sub[0] != L'\0';
    const float routeY = centerSingleLine && !hasSub
        ? y + (h - g_TextL.Height(route, routeScale)) * 0.5f
        : y + 2.0f;
    const float subY = routeY + g_TextL.Height(route, routeScale) - 3.0f;
    float tr = 1.0f + (r - 1.0f) * active;
    float tg = 1.0f + (g - 1.0f) * active;
    float tb = 1.0f + (b - 1.0f) * active;
    if (keepTextReadable && !selected) {
        // SHOP keeps every route legible at once. Its accent still survives,
        // but inactive tabs no longer read like disabled/transparent rows.
        constexpr float kInactiveTextLift = 0.72f;
        tr = r + (1.0f - r) * kInactiveTextLift;
        tg = g + (1.0f - g) * kInactiveTextLift;
        tb = b + (1.0f - b) * kInactiveTextLift;
    }
    if (selected) tr = tg = tb = 1.0f;

    const float routeTextW = g_TextL.Width(route, routeScale);
    const float routeX = centerText
        ? x + (w - routeTextW) * 0.5f : x;
    DrawShadowedText(g_TextL, route, routeX, routeY, routeScale,
                     tr, tg, tb,
                     (keepTextReadable
                          ? (0.98f + 0.06f * active + 0.10f * pulse)
                          : (0.90f + 0.10f * active + 0.16f * pulse)) * alpha,
                     0.86f);
    if (sub && sub[0] != L'\0') {
        const float subTextW = g_TextS.Width(sub, subScale);
        const float subX = centerText
            ? x + (w - subTextW) * 0.5f : x + 4.0f;
        DrawShadowedText(g_TextS, sub, subX, subY, subScale,
                         keepTextReadable
                             ? (0.84f + 0.10f * active)
                             : (0.70f + 0.18f * active),
                         keepTextReadable
                             ? (0.88f + 0.08f * active)
                             : (0.75f + 0.16f * active),
                         keepTextReadable
                             ? (0.94f + 0.06f * active)
                             : (0.82f + 0.12f * active),
                         (keepTextReadable
                              ? (0.92f + 0.06f * active)
                              : (0.76f + 0.18f * active)) * alpha,
                         0.70f);
    }
}

// Canonical panel-button surface. Scene code should use this entry point
// instead of assembling a frame, feedback bars, and text independently.
static void DrawPanelButton(const wchar_t* route, const wchar_t* sub,
                            float x, float y, float w, float h,
                            float r, float g, float b, float alpha,
                            float hover, bool selected, float pulse, float now,
                            float routeScale = 1.04f, float subScale = 0.48f,
                            bool keepTextReadable = false,
                            PanelButtonSlideSide slideSide =
                                PanelButtonSlideSide::Both,
                            bool centerText = false,
                            bool centerSingleLine = false) {
    DrawPanelButtonCore(route, sub, x, y, w, h, r, g, b, alpha,
                        hover, selected, pulse, now,
                        routeScale, subScale, keepTextReadable,
                        slideSide, centerText, centerSingleLine);
}

// Compatibility alias for older scene paths. All calls still flow through
// DrawPanelButton, so there is only one visual implementation to maintain.
static void DrawUnifiedMenuCommand(const wchar_t* route, const wchar_t* sub,
                                   float x, float y, float w, float h,
                                   float r, float g, float b, float alpha,
                                   float hover, bool selected, float pulse, float now,
                                   float routeScale = 1.04f, float subScale = 0.48f,
                                   bool keepTextReadable = false,
                                   bool bothSides = true,
                                   bool centerText = false,
                                   bool centerSingleLine = false) {
    const PanelButtonSlideSide slideSide = bothSides
        ? PanelButtonSlideSide::Both : PanelButtonSlideSide::Left;
    DrawPanelButton(route, sub, x, y, w, h, r, g, b, alpha,
                    hover, selected, pulse, now,
                    routeScale, subScale, keepTextReadable,
                    slideSide, centerText, centerSingleLine);
}

static bool PanelButtonHit(double mx, double my,
                           float x, float y, float w, float h,
                           float hitPadding = 0.0f) {
    return mx >= x - hitPadding && mx <= x + w + hitPadding
        && my >= y - hitPadding && my <= y + h + hitPadding;
}

static bool UpdatePanelButtonHover(float& hover, bool enabled,
                                   double mx, double my,
                                   float x, float y, float w, float h,
                                   float dt, float hitPadding = 0.0f) {
    const bool hovered = enabled && PanelButtonHit(mx, my, x, y, w, h,
                                                    hitPadding);
    hover = UpdateMenuCommandHover(hover, hovered, dt);
    return hovered;
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

static void DrawOrbitalPowerSphere(float cx, float cy, float radius,
                                   int weapon, float now, float alpha, float uiS,
                                   float r, float g, float b) {
    if (alpha <= 0.001f) return;
    constexpr float kPi = 3.14159265358979323846f;
    constexpr float kOrbitSpeedScale = 1.80f;
    const float motionT = now * kOrbitSpeedScale;

    // Dense inner globe. These fine lines stay batched while the prominent
    // orbital paths and stars use the supplied texture masks.
    const float globeR = radius * 0.36f;
    const float globeSpin = motionT * (weapon == 0 ? 0.22f : -0.14f);
    for (int lat = -3; lat <= 3; ++lat) {
        const float latN = (float)lat / 4.0f;
        const float bandY = latN * globeR;
        const float bandR = globeR * sqrtf(std::max(0.0f, 1.0f - latN * latN));
        float prevX = 0.0f, prevY = 0.0f;
        for (int s = 0; s <= 28; ++s) {
            const float a = 2.0f * kPi * (float)s / 28.0f + globeSpin;
            const float z = sinf(a) * bandR;
            const float x = cx + cosf(a) * bandR;
            const float y = cy + bandY + z * 0.18f;
            if (s > 0) {
                const float lineA = (0.12f + 0.09f * (z / globeR + 1.0f)) * alpha;
                LogoLine(prevX, prevY, x, y, 1.35f * uiS,
                         0.0f, 0.0f, 0.018f, 0.42f * lineA);
                LogoLine(prevX, prevY, x, y, 0.58f * uiS,
                         r, g, b, lineA);
            }
            prevX = x; prevY = y;
        }
    }
    for (int lon = 0; lon < 9; ++lon) {
        const float az = 2.0f * kPi * (float)lon / 9.0f + globeSpin;
        float prevX = 0.0f, prevY = 0.0f;
        for (int s = 0; s <= 24; ++s) {
            const float a = -kPi * 0.5f + kPi * (float)s / 24.0f;
            const float ringR = cosf(a) * globeR;
            const float z = sinf(az) * ringR;
            const float x = cx + cosf(az) * ringR;
            const float y = cy + sinf(a) * globeR + z * 0.18f;
            if (s > 0) {
                const float lineA = (0.11f + 0.08f * (z / globeR + 1.0f)) * alpha;
                LogoLine(prevX, prevY, x, y, 1.28f * uiS,
                         0.0f, 0.0f, 0.018f, 0.40f * lineA);
                LogoLine(prevX, prevY, x, y, 0.54f * uiS,
                         r, g, b, lineA);
            }
            prevX = x; prevY = y;
        }
    }

    struct OrbitDef { float radius, tilt, roll, speed, phase; };
    const OrbitDef orbits[3] = {
        { 1.05f, 0.30f,  0.18f,  0.15f, 0.0f },
        { 1.18f, 0.72f, -0.86f, -0.10f, 1.7f },
        { 1.27f, 1.04f,  0.94f,  0.07f, 3.4f },
    };
    for (int o = 0; o < 3; ++o) {
        const OrbitDef& od = orbits[o];
        const float rr = radius * od.radius;
        const float phase = od.phase + motionT * od.speed * (weapon == 0 ? 1.0f : 0.72f);
        ProjectedOrbitPoint prev = ProjectOrbitalPoint(cx, cy, rr, 0.0f,
                                                       od.tilt, od.roll, phase);
        for (int s = 1; s <= 32; ++s) {
            const float a = 2.0f * kPi * (float)s / 32.0f;
            const ProjectedOrbitPoint p = ProjectOrbitalPoint(cx, cy, rr, a,
                                                              od.tilt, od.roll, phase);
            const float depth = LogoClamp01(0.5f + p.z / (rr * 2.0f));
            DrawVisibleConstellLine(prev.x, prev.y, p.x, p.y, 0.66f * uiS,
                                   r, g, b, (0.10f + 0.22f * depth) * alpha);
            prev = p;
        }
    }

    // Fixed samples avoid per-frame noise flicker while the slow phase shift
    // gives the dust cloud the same orbital motion as the reference.
    for (int i = 0; i < 54; ++i) {
        const int orbitIndex = i % 3;
        const OrbitDef& od = orbits[orbitIndex];
        const float jitter = 1.0f + 0.055f * sinf((float)i * 12.9898f);
        const float rr = radius * od.radius * jitter;
        const float a = (float)i * 2.399963f + motionT * od.speed * 0.52f;
        const ProjectedOrbitPoint p = ProjectOrbitalPoint(cx, cy, rr, a,
                                                          od.tilt, od.roll,
                                                          od.phase);
        const float depth = LogoClamp01(0.5f + p.z / (rr * 2.0f));
        const float size = (0.75f + depth * 1.15f) * uiS;
        DrawConstellationDisc(p.x, p.y, size, r, g, b,
                              (0.08f + depth * 0.18f) * alpha);
    }

    const int starOrbit[4] = { 0, 1, 2, 1 };
    const float starAngle[4] = { 0.25f, 1.82f, 3.72f, 5.18f };
    for (int i = 0; i < 4; ++i) {
        const OrbitDef& od = orbits[starOrbit[i]];
        const float rr = radius * od.radius;
        const ProjectedOrbitPoint p = ProjectOrbitalPoint(
            cx, cy, rr, starAngle[i] + motionT * od.speed,
            od.tilt, od.roll, od.phase);
        const float twinkle = 0.72f + 0.28f * sinf(motionT * (2.7f + i * 0.31f) + i * 1.7f);
        DrawConstellationDisc(p.x, p.y, (13.0f + 4.0f * twinkle) * uiS,
                              0.0f, 0.0f, 0.012f, 0.34f * alpha);
        DrawConstellationDisc(p.x, p.y, (7.0f + 3.0f * twinkle) * uiS,
                              r, g, b, 0.22f * alpha);
        DrawConstellationDisc(p.x, p.y, (2.8f + 1.8f * twinkle) * uiS,
                              0.96f, 1.0f, 1.0f, 0.92f * alpha);
    }
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

static void DrawWeaponPowerConstellation(float cx, float cy, float radius,
                                         const float values[6],
                                         const wchar_t* const names[6],
                                         const wchar_t* const labels[6],
                                         int weapon, float now, float alpha, float uiS,
                                         float r, float g, float b) {
    if (alpha <= 0.001f) return;
    constexpr float kPi = 3.14159265358979323846f;
    constexpr int kAxes = 6;
    float ux[kAxes], uy[kAxes];
    float px[kAxes], py[kAxes];

    // A local optical well keeps the power diagram readable without turning
    // the center of the screen into another rectangular application panel.
    DrawSceneRadialVignette(cx, cy, radius * 1.72f, 0.36f * alpha);
    DrawOrbitalPowerSphere(cx, cy, radius, weapon, now, 0.82f * alpha, uiS,
                           r, g, b);

    for (int ring = 1; ring <= 3; ++ring) {
        const float rr = radius * ((float)ring / 3.0f);
        const float ringA = (0.24f + 0.10f * (float)ring) * alpha;
        for (int i = 0; i < kAxes; ++i) {
            const float a0 = -kPi * 0.5f + (2.0f * kPi * (float)i / (float)kAxes);
            const float a1 = -kPi * 0.5f + (2.0f * kPi * (float)(i + 1) / (float)kAxes);
            LogoLine(cx + cosf(a0) * rr, cy + sinf(a0) * rr,
                     cx + cosf(a1) * rr, cy + sinf(a1) * rr,
                     1.1f * uiS, r, g, b, ringA);
        }
    }

    // Weapon-specific background constellation — identifies the weapon by shape
    if (weapon == 0) {
        // RIFLE — LANCE: narrow spear, 6 nodes, apex-base axis, precision feel
        constexpr int nL = 6;
        const float lAng[nL] = {
            -kPi * 0.50f,                    // apex (top)
            -kPi * 0.50f - 0.52f,            // upper-left wing
            -kPi * 0.50f + 0.52f,            // upper-right wing
            -kPi * 0.50f + kPi - 0.55f,      // lower-left base
            -kPi * 0.50f + kPi + 0.55f,      // lower-right base
            -kPi * 0.50f + kPi               // base anchor (bottom)
        };
        const float lRad[nL] = { 1.26f, 1.16f, 1.16f, 1.22f, 1.22f, 1.28f };
        const float lSpin = now * 0.05f;
        float lx[nL], ly[nL];
        for (int i = 0; i < nL; ++i) {
            lx[i] = cx + cosf(lAng[i] + lSpin) * radius * lRad[i];
            ly[i] = cy + sinf(lAng[i] + lSpin) * radius * lRad[i];
        }
        constexpr int lEdgeCount = 7;
        const int lEdges[lEdgeCount][2] = { {0,1},{0,2},{1,3},{2,4},{3,5},{4,5},{1,2} };
        for (int e = 0; e < lEdgeCount; ++e)
            DrawVisibleConstellLine(lx[lEdges[e][0]], ly[lEdges[e][0]],
                                    lx[lEdges[e][1]], ly[lEdges[e][1]],
                                    0.85f * uiS, r, g, b, 0.34f * alpha);
        const float lp = 0.5f + 0.5f * sinf(now * 6.8f);
        for (int i = 0; i < nL; ++i) {
            const bool isKey = (i == 0 || i == 5);
            DrawVisibleConstellNode(lx[i], ly[i],
                                    (isKey ? 4.2f + lp * 1.8f : 3.4f) * uiS,
                                    r, g, b, (isKey ? 0.68f + lp * 0.22f : 0.44f) * alpha);
        }
    } else {
        // STATIC FIELD — CORONA: 7-node web ring, slow breathing, area feel
        constexpr int nC = 7;
        const float cRad[nC] = { 1.28f, 1.16f, 1.30f, 1.14f, 1.25f, 1.20f, 1.32f };
        const float cSpin = now * -0.055f;
        float cnx[nC], cny[nC];
        for (int i = 0; i < nC; ++i) {
            const float a = cSpin - kPi * 0.5f + 2.0f * kPi * (float)i / (float)nC;
            cnx[i] = cx + cosf(a) * radius * cRad[i];
            cny[i] = cy + sinf(a) * radius * cRad[i];
        }
        for (int i = 0; i < nC; ++i) {
            const int j = (i + 1) % nC;
            DrawVisibleConstellLine(cnx[i], cny[i], cnx[j], cny[j],
                                    0.85f * uiS, r, g, b, 0.32f * alpha);
        }
        for (int i = 0; i < nC; ++i) {
            const int k = (i + 3) % nC;
            DrawVisibleConstellLine(cnx[i], cny[i], cnx[k], cny[k],
                                    0.60f * uiS, r, g, b, 0.18f * alpha);
        }
        const float cp = 0.5f + 0.5f * sinf(now * 3.2f);
        for (int i = 0; i < nC; ++i)
            DrawVisibleConstellNode(cnx[i], cny[i],
                                    (3.2f + cp * 1.4f) * uiS,
                                    r, g, b, (0.40f + cp * 0.16f) * alpha);
    }

    for (int i = 0; i < kAxes; ++i) {
        const float ang = -kPi * 0.5f + (2.0f * kPi * (float)i / (float)kAxes);
        ux[i] = cosf(ang);
        uy[i] = sinf(ang);
        const float valueRadius = radius * (0.22f + 0.78f * LogoClamp01(values[i]));
        px[i] = cx + ux[i] * valueRadius;
        py[i] = cy + uy[i] * valueRadius;
        LogoLine(cx, cy, cx + ux[i] * radius, cy + uy[i] * radius,
                 0.90f * uiS, r, g, b, 0.38f * alpha);
        for (int tick = 1; tick <= 4; ++tick) {
            const float tr = radius * (float)tick / 4.0f;
            const float tx = cx + ux[i] * tr;
            const float ty = cy + uy[i] * tr;
            const float tickHalf = (tick == 4 ? 5.0f : 3.2f) * uiS;
            LogoLine(tx - uy[i] * tickHalf, ty + ux[i] * tickHalf,
                     tx + uy[i] * tickHalf, ty - ux[i] * tickHalf,
                     0.90f * uiS, r, g, b,
                     (tick == 4 ? 0.46f : 0.26f) * alpha);
        }
    }

    // Filled polygon — triangle fan from center so data shape is readable
    for (int i = 0; i < kAxes; ++i) {
        const int j = (i + 1) % kAxes;
        BatchTri(cx, cy, px[i], py[i], px[j], py[j], r, g, b, 0.22f * alpha);
    }

    for (int i = 0; i < kAxes; ++i) {
        const int j = (i + 1) % kAxes;
        DrawVisibleConstellLine(px[i], py[i], px[j], py[j], 1.6f * uiS,
                               r, g, b, 0.96f * alpha);
        DrawVisibleConstellNode(px[i], py[i], 5.4f * uiS,
                               r, g, b, 1.00f * alpha);
    }

    const float pulse = 0.5f + 0.5f * sinf(now * (weapon == 0 ? 7.0f : 3.4f));
    const float spin = now * (weapon == 0 ? 0.72f : -0.38f);
    const float orbitR = 25.0f * uiS + pulse * 4.0f * uiS;
    for (int i = 0; i < 6; ++i) {
        const float a0 = spin + 2.0f * kPi * (float)i / 6.0f;
        const float a1 = a0 + 0.34f;
        LogoLine(cx + cosf(a0) * orbitR, cy + sinf(a0) * orbitR,
                 cx + cosf(a1) * orbitR, cy + sinf(a1) * orbitR,
                 1.6f * uiS, r, g, b, (0.46f + 0.30f * pulse) * alpha);
    }
    DrawConstellationDisc(cx, cy, (29.0f + 6.0f * pulse) * uiS,
                          0.0f, 0.0f, 0.012f, 0.32f * alpha);
    DrawConstellationDisc(cx, cy, (20.0f + 5.0f * pulse) * uiS,
                          r, g, b, 0.12f * alpha);
    DrawConstellationDisc(cx, cy, 7.0f * uiS,
                          r, g, b, 0.70f * alpha);
    drawDiamond(cx, cy, 11.0f * uiS, r, g, b, 0.86f * alpha);
    drawDiamond(cx, cy, 4.2f * uiS, 0.96f, 1.0f, 1.0f, 0.96f * alpha);

    for (int i = 0; i < kAxes; ++i) {
        const float labelR = radius + 34.0f * uiS;
        const float anchor = cx + ux[i] * labelR;
        const float ty = cy + uy[i] * labelR - 8.0f * uiS;
        const float nameScale = 0.43f * uiS;
        const float valueScale = 0.52f * uiS;
        const float nameW = g_TextS.Width(names[i], nameScale);
        const float valueW = g_TextS.Width(labels[i], valueScale);
        // Each string aligns to anchor independently — value width cannot shift name position
        float nameX, valX;
        if (ux[i] < -0.25f) {
            const float re = anchor - 4.0f * uiS;
            nameX = re - nameW;
            valX  = re - valueW;
        } else if (fabsf(ux[i]) <= 0.25f) {
            nameX = anchor - nameW  * 0.5f;
            valX  = anchor - valueW * 0.5f;
        } else {
            nameX = anchor + 4.0f * uiS;
            valX  = anchor + 4.0f * uiS;
        }
        DrawShadowedText(g_TextS, names[i], nameX, ty, nameScale,
                         0.66f, 0.76f, 0.86f, 0.76f * alpha, 0.70f);
        DrawShadowedText(g_TextS, labels[i], valX, ty + 18.0f * uiS, valueScale,
                         0.94f, 0.98f, 1.0f, 0.96f * alpha, 0.76f);
    }
}

static void DrawExpandingBracket(float x, float y, float w, float h, float expandT,
                                 float r, float g, float b, float alpha) {
    if (alpha <= 0.001f) return;
    const float e = std::max(0.0f, std::min(1.0f, expandT));
    const float push = 42.0f * e;
    const float len = 26.0f + 18.0f * e;
    const float t = 1.6f;
    const float x0 = x - push, y0 = y - push * 0.55f;
    const float x1 = x + w + push, y1 = y + h + push * 0.55f;
    BindMainShader();
    drawRect(x0, y0, len, t, r, g, b, alpha);
    drawRect(x0, y0, t, len, r, g, b, alpha);
    drawRect(x1 - len, y0, len, t, r, g, b, alpha);
    drawRect(x1 - t, y0, t, len, r, g, b, alpha);
    drawRect(x0, y1 - t, len, t, r, g, b, alpha);
    drawRect(x0, y1 - len, t, len, r, g, b, alpha);
    drawRect(x1 - len, y1 - t, len, t, r, g, b, alpha);
    drawRect(x1 - t, y1 - len, t, len, r, g, b, alpha);
}

void Scene_MainMenu(const SceneCtx& c) {
    // The main menu owns no page-entry texture fade. Inline pages set their
    // own reveal immediately before drawing, so returning here can never
    // inherit a partially faded CircleTexture from the previous page.
    SetSceneTextureReveal(1.0f);
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    GLFWwindow* window = c.window;
    const GameState st = g_GameManager.currentState; (void)st;
    (void)*c.fireTimer;
    int li2 = LangIndex();
    bool booting = (g_BootAnim > 0.0f);
    float uiA = 1.0f - g_FadeAlpha; if (uiA < 0.0f) uiA = 0.0f;

    // Inline panels own their local reveal; keep the shared background stable
    // during the handoff so the old fade cannot create a brightness flash.
    DrawMenuBackground(sw, sh, delta, 1.0f);
    if (s_MainMenuArmoryPanel) {
        Scene_Shop(c);
        return;
    }
    if (s_MainMenuCodexPanel) {
        Scene_Codex(c);
        return;
    }
    if (s_MainMenuSettingsPanel) {
        Scene_SettingsInline(c);
        return;
    }
    if (s_MainMenuRunConfigPanel) {
        Scene_RunConfigInline(c);
        return;
    }

    // Intro animation (original)
    static float s_introT = 0.0f;
    static int   s_menuSelect = -1;
    static float s_menuExitT  = 0.0f;
    static float kHoverT[5] = {};
    constexpr float kTitleDur  = 0.55f;
    constexpr float kBtnDur    = 0.45f;
    constexpr float kBtnStag   = 0.14f;
    constexpr float kTextStart = kTitleDur + 4.0f * kBtnStag + kBtnDur;
    constexpr float kTextDur   = 0.35f;
    constexpr float kIntroDone = kTextStart + kTextDur;

    if (g_MainMenuEntryT <= 0.0f) {
        s_introT = 0.0f;
        s_menuSelect = -1;
        s_menuExitT = 0.0f;
        for (int i = 0; i < 5; ++i) kHoverT[i] = 0.0f;
        g_MainMenuEntryT = 0.001f;
    } else {
        g_MainMenuEntryT += delta;
    }
    if (s_MainMenuResumeFromPanel) {
        s_menuSelect = -1;
        s_menuExitT = 0.0f;
        for (int i = 0; i < 5; ++i) kHoverT[i] = 0.0f;
        s_MainMenuResumeFromPanel = false;
    }
    const bool exitActive = (s_menuSelect >= 0);
    if (exitActive) {
        s_menuExitT += delta;
        if (s_menuExitT > 0.70f) s_menuExitT = 0.70f;
    }

    bool introWasActive = (s_introT < kIntroDone);
    if (!booting && lmb && !g_LmbPrev && introWasActive)
        s_introT = kIntroDone;
    if (!booting && s_introT < kIntroDone)
        s_introT = std::min(s_introT + delta, kIntroDone);
    const bool introActive = (s_introT < kIntroDone);

    float titlePhA = std::min(s_introT / kTitleDur, 1.0f);
    const float titleA = Smoothstep(titlePhA) * uiA;
    float textPhA  = (s_introT > kTextStart)
        ? std::min((s_introT - kTextStart) / kTextDur, 1.0f) : 0.0f;
    const float textA = Smoothstep(textPhA) * uiA;

    // Button layout constants
    struct SBtnDef {
        const wchar_t* label[3];
        const wchar_t* route[3];
        float ir, ig, ib;
    };
    static const SBtnDef kBtns[] = {
        { { L"시작",      L"Start",    L"スタート"  }, { L"PLAY",        L"PLAY",        L"PLAY"        }, 0.18f, 0.62f, 0.96f },
        { { L"상점",      L"Armory",   L"武器庫"    }, { L"ARMORY",      L"ARMORY",      L"ARMORY"      }, 0.18f, 0.62f, 0.96f },
        { { L"도감",      L"Astral Log", L"星界記録" }, { L"ASTRAL_LOG",  L"ASTRAL_LOG",  L"ASTRAL_LOG"  }, 0.18f, 0.62f, 0.96f },
        { { L"설정",      L"Setting",  L"設定"      }, { L"SETTING",     L"SETTING",     L"SETTING"     }, 0.18f, 0.62f, 0.96f },
        { { L"게임 종료", L"Exit",     L"終了"      }, { L"EXIT",        L"EXIT",        L"EXIT"        }, 0.18f, 0.62f, 0.96f },
    };
    // The lobby uses a large route label plus a smaller descriptor. Keep
    // both layers in the active language; route IDs must not leak through
    // as English-only text when Korean or Japanese is selected.
    const int   kBtnCount = 5;
    const float BW = std::min(560.0f, std::max(420.0f, sw * 0.34f));
    const float BH = 70.0f;
    const float BGAP = 15.0f;
    float totalBH = kBtnCount * BH + (kBtnCount - 1) * BGAP;
    float btnX0   = std::max(58.0f, sw * 0.075f);
    // The lobby command rail belongs to the lower-left corner, matching the
    // navigation language used by the other pages. Keep a responsive bottom
    // margin, but never let the rail climb into the logo on short windows.
    float btnY0 = MainMenuButtonRailStartY(sh);

    const float now = (float)glfwGetTime();
    const float menuFieldA = Smoothstep(LogoClamp01((s_introT - 0.10f) / 0.62f)) * uiA;
    DrawMainMenuAstralVeil(sw, sh, btnX0, btnY0, BW, totalBH,
                           kHoverT, menuFieldA);
    DrawSharedMenuDim(sw, sh, btnX0, btnY0, BW, totalBH,
                      Smoothstep(LogoClamp01((s_introT - 0.12f) / 0.58f)) * uiA);
    {
        const UiThemePalette& theme = GetUiTheme();
        const float anchorX = btnX0 - 36.0f;
        const float anchorY = std::max(22.0f, MainLogoTop(sh) - 20.0f);
        const float anchorH = std::min(sh - anchorY - 26.0f, totalBH + 170.0f);
        const float anchorA = Smoothstep(LogoClamp01((s_introT - 0.28f) / 0.48f)) * uiA;
        BindMainShader();
        drawRect(anchorX, anchorY, 1.2f, anchorH,
                 theme.Frame.r, theme.Frame.g, theme.Frame.b, 0.17f * anchorA);
        drawDiamond(anchorX + 0.6f, anchorY + 4.0f, 2.8f,
                    theme.Frame.r, theme.Frame.g, theme.Frame.b, 0.22f * anchorA);
        drawDiamond(anchorX + 0.6f, anchorY + anchorH - 4.0f, 2.8f,
                    theme.Frame.r, theme.Frame.g, theme.Frame.b, 0.18f * anchorA);
    }
    DrawMainOnedowLogo(sw, sh, titleA, titlePhA, sw * 0.30f, 0.82f);
    for (int i = 0; i < kBtnCount; i++) {
        float baseBx = btnX0;
        float baseBy = btnY0 + (float)i * (BH + BGAP);
        const bool buttonReady = !booting && !introActive && !introWasActive
                              && !exitActive && g_FadeDir == 0;
        bool hov = UpdatePanelButtonHover(kHoverT[i], buttonReady,
                                          mx, my,
                                          baseBx, baseBy, BW, BH,
                                          delta, 12.0f);
        float t = kHoverT[i];
        float rawBtnPh = (s_introT - kTitleDur - (float)i * kBtnStag) / kBtnDur;
        rawBtnPh = std::max(0.0f, std::min(rawBtnPh, 1.0f));
        float reveal = Smoothstep(rawBtnPh);
        bool selected = (s_menuSelect == i);
        float exitP = Smoothstep(std::min(1.0f, s_menuExitT / 0.50f));
        float rowA = reveal * uiA;
        if (exitActive && !selected) rowA *= (1.0f - exitP * 0.86f);
        float slide = (1.0f - reveal) * 34.0f;
        float bx = baseBx + slide - 10.0f * t;
        float by = baseBy + (exitActive && !selected ? exitP * 28.0f : 0.0f);
        float ar = kBtns[i].ir, ag = kBtns[i].ig, ab = kBtns[i].ib;
        if (i == 4) {
            // Keep the idle command in the shared family color. The warmer
            // warning hue appears only while SHUTDOWN is actively targeted.
            const float warnT = std::max(t, selected ? exitP : 0.0f);
            ar += (0.96f - ar) * warnT;
            ag += (0.72f - ag) * warnT;
            ab += (0.28f - ab) * warnT;
        }
        float selectPulse = selected ? (0.50f + 0.50f * sinf(now * 18.0f)) * exitP : 0.0f;
        const wchar_t* route = MainMenuRouteLabel(li2, i);
        const wchar_t* sub = MainMenuSubtitleLabel(li2, i);
        DrawPanelButton(route, sub, bx, by, BW, BH,
                        ar, ag, ab, rowA, t, selected, selectPulse,
                        now + (float)i * 0.17f,
                        1.04f, 0.48f, false,
                        PanelButtonSlideSide::Left);

        if (hov && lmb && !g_LmbPrev) {
            s_menuSelect = i;
            s_menuExitT = 0.0f;
        }
    }

    {
        int morphIdx = -1;
        float bestHover = 0.18f;
        for (int i = 0; i < kBtnCount; ++i) {
            if (kHoverT[i] > bestHover) { bestHover = kHoverT[i]; morphIdx = i; }
        }
        if (s_menuSelect >= 0) morphIdx = s_menuSelect;

        UpdateMainMenuConstellationMotion(morphIdx, delta);
        const int focusIdx = morphIdx >= 0 ? morphIdx : 0;
        const float palette[5][3] = {
            {0.18f, 0.62f, 0.96f}, {0.08f, 0.70f, 0.82f},
            {0.70f, 0.62f, 1.00f}, {0.96f, 0.78f, 0.34f},
            {0.52f, 0.64f, 0.78f}
        };
        const int colorIdx = focusIdx % 5;
        const float colR = palette[colorIdx][0];
        const float colG = palette[colorIdx][1];
        const float colB = palette[colorIdx][2];
        // Preserve the shared cyan family while allowing the focused command
        // to tint the constellation's active geometry.
        const float paletteMix = 0.12f + 0.20f * bestHover;
        const float lineR = 0.18f + (colR - 0.18f) * paletteMix;
        const float lineG = 0.62f + (colG - 0.62f) * paletteMix;
        const float lineB = 1.00f + (colB - 1.00f) * paletteMix;
        const float canvasReveal = Smoothstep(LogoClamp01((s_introT - kTitleDur - 0.18f) / 0.70f)) * uiA;
        const float collapse = exitActive ? Smoothstep(std::min(1.0f, s_menuExitT / 0.50f)) : 0.0f;
        const float canvasLeft = btnX0 + BW + 40.0f;
        const float canvasRight = std::max(canvasLeft + 280.0f, sw - 30.0f);
        const float availableCanvasW = canvasRight - canvasLeft;
        const float cw = std::min(sw * 0.48f, availableCanvasW);
        const float ch = std::min(sh * 0.66f, cw * 0.78f);
        const float parallaxX = ((float)mx - sw * 0.5f) * -0.012f;
        const float parallaxY = ((float)my - sh * 0.5f) * -0.009f;
        const float cx = canvasLeft + availableCanvasW * 0.5f + parallaxX;
        const float cy = sh * 0.53f + parallaxY;
        DrawRadialGradient(cx, cy, cw * 0.54f, 0.0f, 0.0f, 0.012f,
                          0.52f * canvasReveal * (1.0f - collapse * 0.35f));
        const float sphereRadius = std::min(cw, ch) * 0.345f
                                 * (1.0f + 0.045f * bestHover);
        const float sphereA = (0.72f + 0.20f * bestHover)
                            * canvasReveal * (1.0f - 0.84f * collapse);
        DrawMainMenuOrbitalInstrument(cx, cy, sphereRadius, focusIdx,
                                      kHoverT, bestHover,
                                      s_MainMenuConstellationMotion.current,
                                      s_MainMenuConstellationMotion.phase,
                                      sphereA, 1.0f,
                                      lineR, lineG, lineB);
        if (collapse > 0.01f)
            drawCircle(cx, cy, 18.0f + 52.0f * collapse, lineR, lineG, lineB, 0.080f * collapse * canvasReveal);
    }

    const float kExitDelay = (s_menuSelect == 4) ? 0.52f : 0.18f;
    if (s_menuSelect >= 0 && s_menuExitT >= kExitDelay && g_FadeDir == 0) {
        int selectedMenu = s_menuSelect;
        s_menuSelect = -1;
        s_menuExitT = 0.0f;
        switch (selectedMenu) {
        case 0:
            ResetRunConfigUi();
            s_MainMenuRunConfigPanel = true;
            break;
        case 1:
            ResetShopUi();
            s_MainMenuArmoryPanel = true;
            break;
        case 2:
            ResetCodexUi();
            s_MainMenuCodexPanel = true;
            break;
        case 3:
            ResetSettingsUi();
            s_MainMenuSettingsPanel = true;
            break;
        case 4:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        }
    }



                        if (g_BootAnim > 0.0f) {
                    g_BootAnim -= delta;
                    float prog = 1.0f - g_BootAnim / BOOT_DUR;        // 0→1
                    if (prog < 0.0f) prog = 0.0f; if (prog > 1.0f) prog = 1.0f;
                    float ease = prog < 0.25f ? (prog / 0.25f) : 1.0f; // 창 열림 0~25%
                    float ar = g_BootAr, ag = g_BootAg, ab = g_BootAb;
                    float WW = 520.0f, WH = 300.0f;
                    float cw = WW * ease;              // 크기 0 → 지정 크기
                    float chh= WH * ease;
                    float wx = sw * 0.5f - cw * 0.5f;
                    float wy = sh * 0.5f - chh * 0.5f;
                    BindMainShader();
                    drawRect(0, 0, sw, sh, 0.0f, 0.0f, 0.0f, 0.45f * ease);  // 배경 딤
                    // 시네마틱 — 아래로 훑는 스캔 스윕 라인 (화이트 플래시는 눈 아파서 제거)
                    {
                        float sweepY = fmodf(prog * 1.3f, 1.0f) * sh;
                        drawRect(0, sweepY, sw, 2.0f, ar, ag, ab, 0.30f * ease);
                        drawRect(0, sweepY - 40.0f, sw, 40.0f, ar, ag, ab, 0.05f * ease);
                    }
                    drawRect(wx, wy, cw, chh, 0.06f, 0.07f, 0.10f, 0.99f);   // 창 본체
                    if (ease > 0.9f) {
                        // 부팅 로그 — 진행도에 따라 한 줄씩 나타남
                        static const wchar_t* LOG[3][6] = {
                            {
                                L"> 모듈 마운트 중 ...",
                                L"> 애셋 로드             [ OK ]",
                                L"> 렌더러 초기화         [ OK ]",
                                L"> 별자리 격자 연결     [ OK ]",
                                L"> 저장 데이터 검증      [ OK ]",
                                L"> 준비 완료."
                            },
                            {
                                L"> mounting modules ...",
                                L"> loading assets        [ OK ]",
                                L"> init renderer         [ OK ]",
                                L"> linking star lattice    [ OK ]",
                                L"> verify save data      [ OK ]",
                                L"> ready."
                            },
                            {
                                L"> モジュールをマウント ...",
                                L"> アセット読込          [ OK ]",
                                L"> レンダラー初期化      [ OK ]",
                                L"> 星座ラティス接続       [ OK ]",
                                L"> セーブデータ検証      [ OK ]",
                                L"> 準備完了。"
                            }
                        };
                        int shown = (int)(prog * 6.5f); if (shown > 6) shown = 6;
                        for (int i = 0; i < shown; i++) {
                            bool last = (i == 5);
                            DrawShadowedText(g_TextS, LOG[li2][i],
                                             wx + 22.0f, wy + 14.0f + i * 24.0f, 0.6f,
                                             last ? ag : 0.65f, last ? 1.0f : 0.78f,
                                             last ? ag : 0.7f, 0.95f, 0.70f);
                        }
                        // 진행 바 (창 하단)
                        float barW = cw - 44.0f, barX = wx + 22.0f, barY = wy + chh - 30.0f;
                        BindMainShader();
                        drawRect(barX, barY, barW, 14.0f, 0.12f, 0.14f, 0.20f, 1.0f);
                        drawRect(barX, barY, barW * prog, 14.0f, ar, ag, ab, 1.0f);
                        wchar_t pct[16]; swprintf_s(pct, L"%d%%", (int)(prog * 100.0f));
                        DrawShadowedText(g_TextS, pct,
                                         barX + barW - 44.0f, barY - 22.0f, 0.6f,
                                         0.8f, 0.9f, 1.0f, 1.0f, 0.70f);
                    }
                    if (g_BootAnim <= 0.0f) {
                        g_BootAnim = 0.0f;
                        g_GameManager.currentState = g_BootTarget;
                    }
                }
}

void Scene_Shop(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    // ── State ──────────────────────────────────────────────────────────
    static constexpr int SHOP_TAB_COUNT = 4;
    static int   s_tab        = 0;
    static float s_tabHov[SHOP_TAB_COUNT] = {};
    static float s_itemHov[300] = {};
    static float s_backHov = 0.0f;
    static int   s_selKey     = -1;
    static float s_scroll     = 0.0f;
    static float s_detailT    = 0.0f;
    static int   s_prevKey    = -2;
    static int   s_prevTab    = -1;
    static bool  s_backExit   = false;
    static float s_backOutT   = 0.0f;
    static float s_listTransitionDir = 1.0f;
    static int   s_listFromKey = -2;
    static float s_listTransitionT = 1.0f;
    static bool  s_prevUp      = false;
    static bool  s_prevDown    = false;
    static int   s_navHoldDir  = 0;
    static float s_navHoldT    = 0.0f;
    static float s_navRepeatT  = 0.0f;
    static bool  s_dragging    = false;
    static bool  s_dragMoved   = false;
    static float s_dragAccum   = 0.0f;
    static double s_dragLastY  = 0.0;
    static bool  s_prevRmb    = false;
    static bool  s_prevEsc    = false;
    static int   s_prevTagKey = -2;
    static float s_tagFlickerT = 1.0f;
    static bool  s_browseItems = false;
    static float s_browseT = 0.0f;
    static bool  s_browseInputLock = false;
    static int   s_fieldColorTab = -1;
    static float s_fieldColorR = 0.30f;
    static float s_fieldColorG = 0.86f;
    static float s_fieldColorB = 1.00f;
    static float s_fieldTargetR = 0.30f;
    static float s_fieldTargetG = 0.86f;
    static float s_fieldTargetB = 1.00f;

    struct SLItem {
        bool           isGroup;
        int            key;
        const wchar_t* label;
        float          r, g, b;
    };
    static SLItem s_items[300];
    static int    s_itemCount = 0;

    const float now = (float)glfwGetTime();
    if (!s_MainMenuArmoryPanel)
        DrawMenuBackground(sw, sh, delta, false);

    float dt = delta; if (dt > 0.05f) dt = 0.05f;
    if (g_ShopEntryT <= 0.001f) {
        // Open ARMORY directly on the first category. The start-module node
        // should be visible as soon as the shop finishes entering; requiring
        // an extra click on the already-selected tab made it look missing.
        s_browseItems = true;
        s_browseT = 0.0f;
        s_browseInputLock = true;
    }
    g_ShopEntryT += dt;
    if (g_ShopEntryT > 1.0f) g_ShopEntryT = 1.0f;

    const bool rawRmb = c.window && glfwGetMouseButton(c.window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    const bool rmbClick = rawRmb && !s_prevRmb;
    s_prevRmb = rawRmb;
    const bool rawEsc = c.window && glfwGetKey(c.window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    const bool escClick = rawEsc && !s_prevEsc;
    s_prevEsc = rawEsc;

    const float browseTarget = s_browseItems ? 1.0f : 0.0f;
    s_browseT = UiApproach(s_browseT, browseTarget, dt, 9.0f);
    if (s_browseInputLock && std::abs(s_browseT - browseTarget) < 0.035f)
        s_browseInputLock = false;
    const bool inputReady = !s_backExit && !s_browseInputLock && g_ShopEntryT >= 0.55f;
    auto beginBack = [&]() {
        if (!inputReady) return;
        s_backExit = true;
        s_backOutT = 0.0f;
        s_ShopBackRequested = false;
    };
    if (s_ShopBackRequested) {
        if (inputReady) beginBack();
        else s_ShopBackRequested = false;
    }
    if (rmbClick || escClick) {
        if (inputReady && s_browseItems) {
            s_browseItems = false;
            s_browseInputLock = true;
        } else {
            beginBack();
        }
    }
    bool finishBackAfterRender = false;
    if (s_backExit) {
        s_backOutT += dt;
        if (s_backOutT >= 0.42f) {
            s_backOutT = 0.42f;
            finishBackAfterRender = true;
        }
    }

    const float entryOldOut = Smoothstep(LogoClamp01(g_ShopEntryT / 0.35f));
    const float entryTreeIn = Smoothstep(LogoClamp01((g_ShopEntryT - 0.20f) / 0.35f));
    const float entryDetailIn = Smoothstep(LogoClamp01((g_ShopEntryT - 0.34f) / 0.30f));
    const float backP = s_backExit ? Smoothstep(LogoClamp01(s_backOutT / 0.42f)) : 0.0f;
    const float oldMenuA = s_backExit ? backP : (1.0f - entryOldOut);
    const float wake = s_backExit ? (1.0f - backP) : entryTreeIn;
    const float rightWake = s_backExit ? (1.0f - backP) : entryDetailIn;
    int li = LangIndex(); if (li < 0 || li >= 3) li = 0;
    const int nli = (li == 0) ? 0 : 1;
    if (s_tab < 0 || s_tab >= SHOP_TAB_COUNT) { s_tab = 0; s_prevTab = -1; }

    // ── Layout ─────────────────────────────────────────────────────────
    const float TARGET_W = 1640.0f, TARGET_H = 910.0f;
    const float uiS = std::max(0.70f, std::min(sw * 0.94f / TARGET_W, sh * 0.90f / TARGET_H));
    const float panelW = TARGET_W * uiS;
    const float panelH = TARGET_H * uiS;
    const float panelX = (sw - panelW) * 0.5f;
    const float panelY = (sh - panelH) * 0.5f + (1.0f - wake) * 24.0f * uiS;
    const float headerH = 82.0f * uiS;
    const float footerH = 74.0f * uiS;
    const float gap = 20.0f * uiS;
    const float colY = panelY + headerH;
    const float colH = panelH - headerH - footerH;
    const float leftW = panelW * 0.260f;
    const float midW = panelW * 0.460f;
    const float runRightW = panelW - leftW - midW - gap * 2.0f;
    const float leftX = panelX;
    const float midX = leftX + leftW + gap;
    const float runRightX = midX + midW + gap;
    (void)runRightW;
    (void)runRightX;
    const float mainBW = std::min(560.0f, std::max(420.0f, sw * 0.34f));
    const float mainBH = 70.0f;
    const float mainGap = 15.0f;
    const float mainTotalH = 5.0f * mainBH + 4.0f * mainGap;
    const float mainX = std::max(58.0f, sw * 0.075f);
    const float mainY = MainMenuCommandStartY(sh);
    const float armoryY = mainY + (mainBH + mainGap);
    const float armoryMidY = armoryY + mainBH * 0.5f;

    const float rootX = mainX + (s_backExit ? backP * 180.0f * uiS : -(1.0f - wake) * 82.0f * uiS);
    const float rootY = mainY;
    const float rootW = std::min(340.0f * uiS, mainBW * 0.66f);
    const float rootH = mainBH;
    const float rootGap = mainGap;
    // SHOP keeps its content layout anchored to the original rootY, but the
    // category rail itself lives in the lower-left corner.
    const float shopRailY = sh - mainTotalH - 48.0f * uiS;
    DrawSharedMenuDim(sw, sh, mainX, shopRailY, mainBW, mainTotalH,
                      std::max(oldMenuA, wake), true);

    // SHOP keeps the live background visible, but an optional soft backdrop
    // blur removes the high-frequency text/windows that compete with the
    // constellation labels. Capture only after the persistent dim layer and
    // before any SHOP UI is drawn, so the UI itself always stays sharp.
    if (g_BackdropBlurEnabled) {
        InitBlurSystem((int)sw, (int)sh);
        CaptureBackdrop();
        DrawBlurPanel(0.0f, 0.0f, sw, sh,
                      0.16f * std::max(oldMenuA, wake),
                      0.004f, 0.010f, 0.020f);
    }
    const InlineThreeColumnLayout columns = BuildInlineThreeColumnLayout(
        sw, sh, uiS, rootX, rootY, rootW, rootH);
    // ARMORY keeps the main-menu command rail, then opens one RUN_CONFIG-sized
    // work surface. The item tree and transaction detail are columns inside
    // that surface rather than two unrelated floating cards.
    const float workY = std::max(74.0f, sh * 0.12f);
    const float workBottom = sh - std::max(88.0f, 104.0f * uiS);
    const float workH = std::max(520.0f * uiS, workBottom - workY);
    const float workX = columns.contentX - 18.0f * uiS;
    const float workRight = sw - std::max(32.0f, 38.0f * uiS);
    const float workW = std::max(780.0f * uiS, workRight - workX);
    const float workHeaderH = 76.0f * uiS;
    const float listColumnW = std::max(330.0f * uiS,
        std::min(430.0f * uiS, workW * 0.34f));
    const float depth2X = workX + 22.0f * uiS;
    const float depth2Y = workY + workHeaderH;
    const float depth2W = listColumnW - 44.0f * uiS;
    const float depth2H = workH - workHeaderH - 20.0f * uiS;
    const float rightX = workX + listColumnW + 18.0f * uiS;
    const float rightW = std::max(440.0f * uiS, workRight - rightX);
    const float rightAreaY = workY;
    const float rightAreaH = workH;
    const float tabH = 54.0f * uiS;
    const float tabY = columns.contentY;
    const float listAreaY = columns.contentY + 64.0f * uiS;
    const float listAreaH = columns.contentH - 64.0f * uiS;
    const float footY = columns.bottomY;
    // These surfaces are intentionally borderless and bounded to readable
    // content. A real fill is required on bright desktop/game backgrounds;
    // text shadows alone cannot establish enough local contrast.
    // ARMORY uses the codex-style persistent category rail: categories stay
    // visible on the left while the selected tab's nodes remain visible beside
    // it. The first tab is selected by default, but is not a separate screen.
    const float categoryA = 1.0f;
    const float itemA = Smoothstep(LogoClamp01((g_ShopEntryT - 0.20f) / 0.55f));
    const float detailWake = rightWake * itemA;
    if (detailWake > 0.002f) {
        // Keep ARMORY surface-free like the codex: only the connecting
        // constellation rules remain, with no opaque work plate behind them.
        DrawAstralDataPlate(workX, workY, workW, workH,
                            0.38f, 0.82f, 1.0f, 0.0f);
    }
    static constexpr int KEY_META  = 0;
    static constexpr int KEY_THEME = 100;
    static constexpr int KEY_AUG   = 1000;

    // SHOP ownership is independent from PLAY loadout state. A core module
    // becomes owned after its first level; profiles use their save bit.
    auto shopItemPurchased = [&](int key) {
        if (key >= KEY_AUG) return false;
        if (key >= KEY_THEME) {
            const int themeId = key - KEY_THEME;
            return themeId >= 0 && themeId < ACCENT_COUNT
                && ThemeOwned(themeId);
        }
        const int metaId = key - KEY_META;
        return metaId >= 0 && metaId < META_COUNT && g_MetaLv[metaId] > 0;
    };

    auto augWeapCat = [](AugType t) -> int {
        switch (t) {
        case AugType::RIFLE_STABILITY:
            return 0;
        case AugType::STATIC_FIELD:
        case AugType::STATIC_FIELD_2:
            return 1;
        default:
            return -1;
        }
    };

    if (s_prevTab != s_tab) {
        ShopStaticFieldPalette(s_tab, s_fieldTargetR,
                               s_fieldTargetG, s_fieldTargetB);
        if (s_fieldColorTab < 0) {
            s_fieldColorR = s_fieldTargetR;
            s_fieldColorG = s_fieldTargetG;
            s_fieldColorB = s_fieldTargetB;
        }
        s_fieldColorTab = s_tab;
        s_prevTab   = s_tab;
        s_itemCount = 0;
        s_scroll    = 0.0f;
        for (float& hover : s_itemHov) hover = 0.0f;

        auto addGroup = [&](const wchar_t* lbl, float r, float g2, float b) {
            if (s_itemCount < 300) s_items[s_itemCount++] = { true, -1, lbl, r, g2, b };
        };
        auto addItem = [&](int key, const wchar_t* lbl, float r, float g2, float b) {
            if (s_itemCount < 300) s_items[s_itemCount++] = { false, key, lbl, r, g2, b };
        };

        // Keep the first shop category focused on the special run-start
        // modules. General permanent upgrades live in the separate UNLOCK
        // category so the first start module can never be hidden in the list.
        if (s_tab == 0) {
            addGroup(nli == 0 ? L"\xC2DC\xC791 \xBAA8\xB4C8" : L"START MODULES",
                     0.38f, 0.82f, 1.00f);
            addItem(KEY_META + META_STARTAUG, MetaName(META_STARTAUG),
                    0.38f, 0.82f, 1.00f);
        }
        if (s_tab == 1) {
            addGroup(nli == 0 ? L"\xCF54\xC5B4 \xD574\xAE08" : L"CORE UNLOCKS",
                     0.38f, 0.82f, 1.00f);
            for (int i = 0; i < META_COUNT; i++)
                if (i != META_STARTAUG)
                    addItem(KEY_META + i, MetaName(i), 0.38f, 0.82f, 1.00f);
            addGroup(nli == 0 ? L"\xD654\xBA74 \xD504\xB85C\xD30C\xC77C" : L"DISPLAY PROFILES",
                     1.00f, 0.84f, 0.28f);
            for (int i = 1; i < ACCENT_COUNT; ++i)
                addItem(KEY_THEME + i, AccentName(i),
                        ACCENT_THEMES[i].r, ACCENT_THEMES[i].g, ACCENT_THEMES[i].b);
        }
        if (s_tab == 2) {
            addGroup(nli == 0 ? L"\xC18C\xCD1D \xC2DC\xC2A4\xD15C" : L"RIFLE CONSTELLATION", 0.72f, 0.92f, 0.42f);
            for (int i = 0; i < AUG_TOTAL; i++)
                if (!AugRemoved(ALL_AUGS[i].type) && augWeapCat(ALL_AUGS[i].type) == 0)
                    addItem(KEY_AUG + i, ALL_AUGS[i].locName[li], 0.72f, 0.92f, 0.42f);
        }
        if (s_tab == 3) {
            addGroup(nli == 0 ? L"\xC704\xC131 \xBC30\xC5F4" : L"FIELD ARRAY", 0.62f, 0.48f, 1.00f);
            for (int i = 0; i < AUG_TOTAL; i++)
                if (!AugRemoved(ALL_AUGS[i].type) && augWeapCat(ALL_AUGS[i].type) == 1)
                    addItem(KEY_AUG + i, ALL_AUGS[i].locName[li], 0.62f, 0.48f, 1.00f);
        }

        s_listTransitionDir = 1.0f;
        s_listFromKey = s_selKey;
        s_listTransitionT = 0.0f;
        s_selKey = -1;
        for (int i = 0; i < s_itemCount; i++)
            if (!s_items[i].isGroup) { s_selKey = s_items[i].key; break; }
    }

    // Keep the static field stable between tabs; only a category change
    // eases it toward the next palette.
    s_fieldColorR = UiApproach(s_fieldColorR, s_fieldTargetR, dt, 3.8f);
    s_fieldColorG = UiApproach(s_fieldColorG, s_fieldTargetG, dt, 3.8f);
    s_fieldColorB = UiApproach(s_fieldColorB, s_fieldTargetB, dt, 3.8f);

    if (s_selKey != s_prevKey) {
        s_listFromKey = s_prevKey;
        s_listTransitionT = 0.0f;
        s_prevKey = s_selKey;
        s_detailT = 0.0f;
    }
    if (s_selKey != s_prevTagKey) { s_prevTagKey = s_selKey; s_tagFlickerT = 0.0f; }
    s_detailT = UiApproach(s_detailT, 1.0f, dt, 6.0f);
    s_listTransitionT = UiApproach(s_listTransitionT, 1.0f, dt, 8.5f);
    s_tagFlickerT = UiApproach(s_tagFlickerT, 1.0f, dt, 16.0f);

    // SHOP_ARCHIVE_RENDERER
    // This intentionally follows the Codex renderer's composition: one large
    // orbital field, a curved observation rail of entries, and a fixed record
    // block. SHOP only changes where that archive field lives and what the
    // record block can purchase.
    {
        g_SceneTextCommands.clear();
        g_DeferSceneText = true;
        const float designA = std::max(0.0f, std::min(1.0f, wake));
        const float detailA = std::max(0.0f, std::min(1.0f, rightWake * s_detailT));
        // CircleTexture layers are page-owned: they reveal once with the page
        // and then remain stable while records move along the rail.  This is
        // deliberately independent from designA/detailA so list selection
        // cannot make the static fields disappear and re-render.
        const float textureReveal = s_backExit
            ? std::max(0.0f, 1.0f - backP)
            : Smoothstep(LogoClamp01(g_ShopEntryT / 0.78f));
        SetSceneTextureReveal(textureReveal);
        const float staticFieldA = 1.0f;
        const float staticGeometryA = textureReveal;
        const float ui = std::max(0.62f, std::min(sw * 0.90f / 1800.0f,
                                                  sh * 0.86f / 1020.0f));
        const float cyanR = 0.30f, cyanG = 0.86f, cyanB = 1.00f;
        // Static field lights use the selected category palette.  Their
        // colour is eased in only when the category changes, never animated
        // continuously while the user is browsing one category.
        const float fieldR = s_fieldColorR;
        const float fieldG = s_fieldColorG;
        const float fieldB = s_fieldColorB;
        const float goldR = 1.00f, goldG = 0.76f, goldB = 0.24f;
        const float whiteR = 0.88f, whiteG = 0.94f, whiteB = 1.00f;
        // Let the item explanation lead the hierarchy. Purchase/status copy
        // stays readable but subordinate, while constellation labels use the
        // same larger explanatory scale.
        const float detailDescriptionSc = 0.96f * ui;
        const float purchaseInfoSc = 0.82f * ui;
        const float purchaseButtonSc = 0.70f * ui;
        const bool lmbClick = lmb && !g_LmbPrev;

        auto drawFit = [&](const wchar_t* text, float x, float y, float scale,
                           float maxW, float r, float g2, float b, float a) {
            if (!text || !text[0] || maxW <= 2.0f) return;
            float sc = scale;
            const float tw = g_TextS.Width(text, sc);
            if (tw > maxW && tw > 0.0f) sc *= maxW / tw;
            DrawShadowedText(g_TextS, text, x, y, sc, r, g2, b,
                             a * designA, 0.84f);
        };
        auto drawRightFit = [&](TextRenderer& renderer, const wchar_t* text,
                                float rightX, float y, float scale, float maxW,
                                float r, float g2, float b, float a,
                                float shadow = 0.84f) {
            if (!text || !text[0] || maxW <= 2.0f) return;
            float sc = scale;
            float tw = renderer.Width(text, sc);
            if (tw > maxW && tw > 0.0f) {
                sc *= maxW / tw;
                tw = renderer.Width(text, sc);
            }
            DrawShadowedText(renderer, text, rightX - tw, y, sc,
                             r, g2, b, a * designA, shadow);
        };
        auto drawModuleGlyph = [&](int key, float cx, float cy, float size,
                                   float r, float g2, float b, float a) {
            if (key >= KEY_AUG && key - KEY_AUG >= 0 && key - KEY_AUG < AUG_TOTAL) {
                DrawAugIcon(ALL_AUGS[key - KEY_AUG].type,
                            cx - size * 0.5f, cy - size * 0.5f, size,
                            r, g2, b, a * designA);
                return;
            }
            BindMainShader();
            const float h = size * 0.5f;
            const float t = std::max(1.0f * ui, size * 0.045f);
            drawRect(cx - h, cy - h, size, t, r, g2, b, a * designA);
            drawRect(cx - h, cy + h - t, size, t, r, g2, b, a * designA);
            drawRect(cx - h, cy - h, t, size, r, g2, b, a * designA);
            drawRect(cx + h - t, cy - h, t, size, r, g2, b, a * designA);
            LogoLine(cx - size * 0.22f, cy, cx + size * 0.22f, cy,
                     t, r, g2, b, a * designA);
            LogoLine(cx, cy - size * 0.22f, cx, cy + size * 0.22f,
                     t, r, g2, b, a * designA);
            drawDiamond(cx, cy, size * 0.10f, r, g2, b, a * designA);
        };
        enum ShopActionState {
            SHOP_ACTION_READY = 0,
            SHOP_ACTION_ACTIVE,
            SHOP_ACTION_BLOCKED,
            SHOP_ACTION_CATALOGUE
        };
        auto drawFixedAction = [&](float x, float y, float w, const wchar_t* label,
                                   float r, float g2, float b,
                                   bool enabled, bool hovered,
                                   ShopActionState state) {
            const bool hot = enabled && hovered;
            static float actionColorR = 0.28f;
            static float actionColorG = 0.88f;
            static float actionColorB = 1.00f;
            static float actionHoverT = 0.0f;
            static float actionEnabledT = 1.0f;
            static bool actionColorReady = false;
            // Give the action a little more vertical breathing room. The
            // matching hitbox is expanded below so the larger ribbon remains
            // comfortable to click.
            const float top = y - 54.0f * ui;
            const float bottom = y + 40.0f * ui;
            float buttonR = r, buttonG = g2, buttonB = b;
            if (state == SHOP_ACTION_READY) {
                buttonR = 0.28f; buttonG = 0.88f; buttonB = 1.00f;
            } else if (state == SHOP_ACTION_ACTIVE) {
                buttonR = 0.72f; buttonG = 0.98f; buttonB = 0.96f;
            } else if (state == SHOP_ACTION_BLOCKED) {
                buttonR = 0.96f; buttonG = 0.30f; buttonB = 0.34f;
            } else if (state == SHOP_ACTION_CATALOGUE) {
                buttonR = 0.42f; buttonG = 0.52f; buttonB = 0.66f;
            }
            if (!actionColorReady) {
                actionColorR = buttonR;
                actionColorG = buttonG;
                actionColorB = buttonB;
                actionColorReady = true;
            }
            // State colour, hover glow and disabled fade all settle through
            // the same short UI interpolation so changing records never
            // causes the action ribbon to snap between colours.
            actionColorR = UiApproach(actionColorR, buttonR, dt, 8.0f);
            actionColorG = UiApproach(actionColorG, buttonG, dt, 8.0f);
            actionColorB = UiApproach(actionColorB, buttonB, dt, 8.0f);
            actionHoverT = UiApproach(actionHoverT, hot ? 1.0f : 0.0f,
                                      dt, 10.0f);
            actionEnabledT = UiApproach(actionEnabledT,
                                        enabled ? 1.0f : 0.0f, dt, 8.0f);
            const float aa = 0.28f + 0.44f * actionEnabledT
                           + 0.28f * actionEnabledT * actionHoverT;
            const float cut = std::min(56.0f * ui, w * 0.24f);
            const float fillA = (0.34f + 0.18f * actionHoverT)
                              * aa * designA;
            // A soft dark ribbon beneath the coloured one separates the
            // action from nearby constellation labels without introducing a
            // rectangular panel or a new opaque layout surface.
            const float dimX = x - 12.0f * ui;
            const float dimTop = top - 7.0f * ui;
            const float dimW = w + 12.0f * ui;
            const float dimH = (bottom - top) + 14.0f * ui;
            const float dimCut = std::min(56.0f * ui, dimW * 0.24f);
            DrawLinearGradientRibbon(dimX, dimTop, dimW, dimH, dimCut,
                                     0.0f, 0.0f, 0.012f,
                                     (0.34f + 0.12f * actionEnabledT)
                                         * designA,
                                     true);
            // The action is a directional ribbon. Its slanted end points back
            // into the detail record without adding a rigid button frame.
            DrawLinearGradientRibbon(x, top, w, bottom - top, cut,
                                     actionColorR, actionColorG, actionColorB,
                                     fillA, true);
            // A small lock-on node gives the action endpoint a game-like
            // interaction cue without changing the ribbon geometry.
            const float actionNodeX = x + w - cut * 0.42f;
            const float actionNodePulse = 0.30f + 0.26f * actionHoverT;
            DrawVisibleConstellNode(actionNodeX, y + 1.0f * ui,
                                    (2.6f + 1.0f * actionHoverT) * ui,
                                    actionColorR, actionColorG, actionColorB,
                                    actionNodePulse * aa * designA, false);
            // The action label uses one stable high-contrast colour. State
            // colours belong to the ribbon itself; changing them on the text
            // makes the control look disabled or errored at a glance.
            float textSc = purchaseButtonSc;
            float textW = g_TextS.Width(label, textSc);
            const float textRight = x + w - cut * 0.56f - 18.0f * ui;
            const float textMaxW = std::max(48.0f * ui,
                                            textRight - x - 18.0f * ui);
            if (textW > textMaxW && textW > 0.0f) {
                textSc *= textMaxW / textW;
                textW = g_TextS.Width(label, textSc);
            }
            const float textR = 0.94f;
            const float textG = 0.98f;
            const float textB = 1.00f;
            DrawShadowedText(g_TextS, label, textRight - textW,
                             y - 20.0f * ui, textSc, textR, textG, textB,
                             0.56f + 0.42f * actionEnabledT, 0.84f);
        };

        DrawPersistentSceneLeftVignette(sw, sh, 0.055f);

        // Keep this loop byte-for-byte in the same visual family as the other
        // pages' shared root commands.
        static const wchar_t* kShopRoute[SHOP_TAB_COUNT][2] = {
            { L"\xC2DC\xC791 \xBAA8\xB4C8", L"START" },
            { L"\xD574\xAE08", L"UNLOCK" },
            { L"\xC18C\xCD1D", L"RIFLE" },
            { L"\xC704\xC131", L"FIELD" },
        };
        static const float kShopR[SHOP_TAB_COUNT] = { 0.70f, 0.38f, 0.72f, 0.62f };
        static const float kShopG[SHOP_TAB_COUNT] = { 0.70f, 0.82f, 0.92f, 0.48f };
        static const float kShopB[SHOP_TAB_COUNT] = { 0.70f, 1.00f, 0.42f, 1.00f };
        const float railX = rootX;
        const float railY = shopRailY;
        const float railW = rootW;
        const float railH = rootH;
        const float categoryWake = wake * categoryA;
        for (int i = 0; i < SHOP_TAB_COUNT + 1; ++i) {
            const bool isBack = i == SHOP_TAB_COUNT;
            const float tx = railX - 58.0f * itemA;
            const float ty = railY + (float)i * (railH + rootGap);
            const bool hov = inputReady
                          && mx >= tx - 26.0f
                          && mx < tx + railW + 18.0f * ui
                          && my >= ty && my < ty + railH;
            float& hovT = isBack ? s_backHov : s_tabHov[i];
            hovT = UpdateMenuCommandHover(hovT, hov, dt);
            if (hov && lmbClick) {
                if (isBack) beginBack();
                else {
                    s_tab = i;
                    s_prevTab = -1;
                    s_browseItems = true;
                    s_browseInputLock = true;
                }
            }
            const bool selected = !isBack && s_tab == i;
            const float rr = isBack ? 0.42f : kShopR[i];
            const float gg = isBack ? 0.62f : kShopG[i];
            const float bb = isBack ? 0.78f : kShopB[i];
            const wchar_t* route = isBack ? L"BACK" : kShopRoute[i][1];
            const wchar_t* sub = isBack ? L"BACK" : kShopRoute[i][0];
            const float reveal = s_backExit ? wake : MenuCommandReveal(g_ShopEntryT, i);
            const float rowA = reveal * categoryWake;
            // Once SHOP is interactive, every category label remains fully
            // readable. Only the page exit is allowed to lower the rail.
            const float tabRowA = (!s_backExit && g_ShopEntryT >= 0.55f)
                ? 1.0f : rowA;
            const float selectPulse = selected
                ? 0.16f + 0.16f * sinf(now * 4.0f) : 0.0f;
            const float slide = (1.0f - reveal) * 34.0f;
            DrawUnifiedMenuCommand(route, sub,
                                   tx + slide - 10.0f * hovT, ty,
                                   railW, railH, rr, gg, bb, tabRowA, hovT,
                                   selected, selectPulse,
                                   now + (float)i * 0.17f,
                                   1.04f, 0.48f, true);
        }

        float detailR = cyanR, detailG = cyanG, detailB = cyanB;
        const wchar_t* detailTitle = L"모듈 없음";
        const wchar_t* detailType = L"상점 항목";
        bool detailIsMeta = false, detailIsTheme = false, detailIsAug = false;
        int detailId = -1;
        if (s_selKey >= 0) {
            if (s_selKey < KEY_THEME) {
                detailIsMeta = true; detailId = s_selKey - KEY_META;
                detailType = L"코어 모듈";
                if (detailId >= 0 && detailId < META_COUNT) {
                    detailTitle = MetaName(detailId);
                }
            } else if (s_selKey < KEY_AUG) {
                detailIsTheme = true; detailId = s_selKey - KEY_THEME;
                detailType = L"화면 프로필";
                if (detailId >= 0 && detailId < ACCENT_COUNT) {
                    detailTitle = AccentName(detailId);
                    detailR = ACCENT_THEMES[detailId].r;
                    detailG = ACCENT_THEMES[detailId].g;
                    detailB = ACCENT_THEMES[detailId].b;
                }
            } else {
                detailIsAug = true; detailId = s_selKey - KEY_AUG;
                detailType = L"페이로드 모듈";
                if (detailId >= 0 && detailId < AUG_TOTAL) {
                    detailTitle = ALL_AUGS[detailId].locName[li];
                    GetRarityColor(ALL_AUGS[detailId].rarity,
                                   detailR, detailG, detailB);
                }
            }
        }

        // The shop constellation is intentionally oversized and right-biased,
        // like the Codex's corner-anchored chart. Its origin sits near the
        // upper-right edge while the visible product rail falls inward as a
        // giant lower semicircle. This makes the constellation the stage
        // instead of a small illustration floating between the UI columns.
        const float chartCX = sw * 0.30f;
        const float chartCY = sh * 0.10f;
        const float chartR = std::min(sw * 0.52f, sh * 0.70f);
        const float itemR = chartR * 0.94f;
        const float rowStep = std::min(250.0f * ui, sh * 0.24f);
        // The catalogue follows one clean lower semicircle. The screen-space
        // Y axis grows downward, so 0 -> PI draws only the lower half.
        const float railRadius = chartR * 0.68f;
        const float railStartAngle = 0.02f;
        const float railSweep = 3.10f;
        const auto railXAt = [&](float row) {
            const float angle = railStartAngle + railSweep * (row / 4.0f);
            return chartCX + cosf(angle) * railRadius;
        };
        const auto railYAt = [&](float row) {
            const float angle = railStartAngle + railSweep * (row / 4.0f);
            return chartCY + sinf(angle) * railRadius;
        };
        // The record readout is a right-center observation block. Keeping its
        // anchor above the lower HUD leaves the lower-right CircleTexture as
        // atmosphere instead of forcing every line of copy into the corner.
        const float detailX = sw * 0.69f;
        const float detailW = std::max(360.0f * ui,
                                       std::min(sw * 0.26f,
                                                sw - detailX - 42.0f * ui));
        // Lift the complete readout slightly so the wallet/status rows keep
        // clear air above the bottom action ribbon.
        const ShopDetailLayout detailLayout = MakeShopDetailLayout(
            detailX, sh * 0.43f, detailW, sh * 0.91f, ui);
        const float detailRight = detailLayout.right;
        const float infoY = detailLayout.titleY;

        if (g_ConstellationCircleTex && textureReveal > 0.001f) {
            // Normalized world-space corner: (1, 1) is the lower-right
            // background corner. The radius is sized from the farthest
            // upper-left detail text so the whole record stays in the dark
            // texture field without adding an opaque panel.
            const float detailTextLeft = detailX - 32.0f * ui;
            const float detailTextTop = detailLayout.top - 28.0f * ui;
            const float detailReach = sqrtf(
                (sw - detailTextLeft) * (sw - detailTextLeft)
                + (sh - detailTextTop) * (sh - detailTextTop));
            // The readout owns the entire lower-right field.  Grow the halo
            // beyond the measured text reach so the title, copy, price and
            // action all sit inside one continuous contrast well.
            const float circleR = std::max(std::min(sw, sh) * 1.04f,
                                           detailReach * 1.28f);
            const float circleCX = sw * 0.95f;
            const float circleCY = sh * 0.95f;
            BatchFlush();
            DrawIcon(g_ConstellationCircleTex,
                     circleCX - circleR, circleCY - circleR,
                     circleR * 2.0f, circleR * 2.0f,
                     0.0f, 0.0f, 0.012f, 0.80f * textureReveal);
            // Inner chromatic field shares the exact (1, 1) corner origin and
            // remains inside the black visibility halo.
            const float colorCircleR = circleR * 0.85f;
            DrawIcon(g_ConstellationCircleTex,
                     circleCX - colorCircleR, circleCY - colorCircleR,
                     colorCircleR * 2.0f, colorCircleR * 2.0f,
                     fieldR, fieldG, fieldB, 0.055f * textureReveal);
        }

        int itemIndices[300] = {};
        int itemTotal = 0;
        int selectedSlot = 0;
        for (int i = 0; i < s_itemCount; ++i) {
            if (s_items[i].isGroup) continue;
            if (itemTotal < 300) {
                itemIndices[itemTotal] = i;
                if (s_items[i].key == s_selKey) selectedSlot = itemTotal;
                ++itemTotal;
            }
        }

        if (itemTotal > 0) {
            const bool overOrbit = inputReady
                && mx >= chartCX - itemR - 100.0f * ui
                && mx <= sw
                && my >= chartCY - rowStep * 4.6f
                && my <= chartCY + rowStep * 4.6f;

            // Match the Codex list controller: W/S (and arrow keys) use an
            // immediate first step followed by a held-key repeat, while the
            // pointer can scrub the same vertical rail by dragging. All input
            // paths feed one step request so the orbit transition stays
            // identical regardless of how the item was changed.
            const bool keyUpNow = c.window &&
                (glfwGetKey(c.window, GLFW_KEY_UP) == GLFW_PRESS ||
                 glfwGetKey(c.window, GLFW_KEY_W) == GLFW_PRESS);
            const bool keyDownNow = c.window &&
                (glfwGetKey(c.window, GLFW_KEY_DOWN) == GLFW_PRESS ||
                 glfwGetKey(c.window, GLFW_KEY_S) == GLFW_PRESS);
            int stepRequest = 0;
            const int navDir = keyUpNow == keyDownNow ? 0 : (keyUpNow ? -1 : 1);
            if (inputReady && navDir != 0) {
                if (navDir != s_navHoldDir) {
                    s_navHoldDir = navDir;
                    s_navHoldT = 0.0f;
                    s_navRepeatT = 0.0f;
                    stepRequest = navDir;
                } else {
                    s_navHoldT += dt;
                    if (s_navHoldT >= 0.25f) {
                        s_navRepeatT += dt;
                        const float repeatInterval = std::max(0.05f,
                            0.26f - (s_navHoldT - 0.25f) * 0.075f);
                        if (s_navRepeatT >= repeatInterval) {
                            s_navRepeatT = 0.0f;
                            stepRequest = navDir;
                        }
                    }
                }
            } else {
                s_navHoldDir = 0;
                s_navHoldT = 0.0f;
                s_navRepeatT = 0.0f;
            }
            s_prevUp = keyUpNow;
            s_prevDown = keyDownNow;

            if (overOrbit && lmb && !g_LmbPrev) {
                s_dragging = true;
                s_dragMoved = false;
                s_dragAccum = 0.0f;
                s_dragLastY = my;
            } else if (!lmb) {
                s_dragging = false;
                s_dragAccum = 0.0f;
            }
            if (s_dragging && lmb) {
                s_dragAccum += (float)(my - s_dragLastY);
                s_dragLastY = my;
                const float dragStepThreshold = rowStep * 0.88f;
                if (fabsf(s_dragAccum) >= dragStepThreshold) {
                    stepRequest = s_dragAccum > 0.0f ? -1 : 1;
                    s_dragAccum += s_dragAccum > 0.0f
                        ? -dragStepThreshold : dragStepThreshold;
                    s_dragMoved = true;
                }
            }

            if (overOrbit && g_ScrollAccum != 0.0f)
                stepRequest = g_ScrollAccum > 0.0f ? -1 : 1;
            g_ScrollAccum = 0.0f;

            if (stepRequest != 0) {
                selectedSlot = (selectedSlot + stepRequest + itemTotal) % itemTotal;
                s_listTransitionDir = (float)stepRequest;
                s_listFromKey = s_selKey;
                s_listTransitionT = 0.0f;
                s_selKey = s_items[itemIndices[selectedSlot]].key;
                s_detailT = 0.0f;
            }
            selectedSlot = 0;
            for (int i = 0; i < itemTotal; ++i)
                if (s_items[itemIndices[i]].key == s_selKey) selectedSlot = i;

            int fromSlot = -1;
            for (int i = 0; i < itemTotal; ++i)
                if (s_items[itemIndices[i]].key == s_listFromKey) fromSlot = i;
            const bool hasFrom = fromSlot >= 0 && fromSlot != selectedSlot;
            const float listEase = Smoothstep(s_listTransitionT);
            const auto listRelFor = [&](int slot, int focusSlot) {
                if (itemTotal <= 1) return 2.0f;
                int offset = slot - focusSlot;
                while (offset > itemTotal / 2) offset -= itemTotal;
                while (offset < -itemTotal / 2) offset += itemTotal;
                // Five positions: two before, selected middle, two after.
                if (offset < -2 || offset > 2) return -1000.0f;
                return (float)(offset + 2);
            };
            const bool listTransitionActive = hasFrom && listEase < 0.995f;
            // Input remains live while records travel.  Only the candidate
            // resolution is consolidated so overlapping animated hit regions
            // cannot let render-loop order change the selected item.
            const bool listClickReady = inputReady;
            int clickSlot = -1;
            float clickDist2 = 1.0e30f;
            // These fields belong to the SHOP viewport itself. They must not
            // inherit s_detailT, because selecting another record should not
            // make the fixed chart/background CircleTextures fade and redraw
            // as if they were part of the record being replaced.
            // Static chart field: this anchor belongs to the constellation
            // chart, not to the selected record, so it never follows list
            // transitions. Record nodes travel over this fixed field.
            const float staticChartCX = chartCX;
            const float staticChartCY = chartCY;
            const float codexFieldCX = staticChartCX - chartR * 0.10f;
            // Chromatic origin marker for the chart's fixed local (0, 0).
            DrawConstellationDisc(staticChartCX, staticChartCY, chartR * 0.82f,
                                  fieldR, fieldG, fieldB,
                                  0.11f * staticFieldA);
            DrawConstellationDisc(staticChartCX, staticChartCY,
                                  chartR * 0.56f, fieldR, fieldG, fieldB,
                                  0.04f * staticFieldA);
            // Draw the black contrast field after the chromatic origin so a
            // dark overlap remains dark instead of being lightened by it.
            DrawConstellationDisc(codexFieldCX, staticChartCY,
                                  chartR * 1.22f, 0.0f, 0.0f, 0.0f,
                                  0.16f * staticFieldA, false);
            // A large local black field behind the focused middle entry gives
            // the active constellation the same layered depth as the Codex
            // record viewer, without introducing an opaque panel.
            const float focusCX = railXAt(2.0f);
            const float focusCY = railYAt(2.0f);
            DrawConstellationDisc(focusCX, focusCY, chartR * 0.16f,
                                  fieldR, fieldG, fieldB,
                                  0.09f * staticFieldA);
            DrawConstellationDisc(focusCX - chartR * 0.08f, focusCY,
                                  chartR * 0.26f,
                                  0.0f, 0.0f, 0.012f,
                                  0.30f * staticFieldA);
            BindMainShader();
            for (int ring = 0; ring < 4; ++ring) {
                const float rr = chartR * (0.525f + 0.125f * (float)ring);
                const int ringSegments = 30;
                for (int j = 0; j < ringSegments; ++j) {
                    if ((j + ring * 2) % 5 == 3) continue;
                    const float a0 = railStartAngle + (float)j * railSweep / (float)ringSegments;
                    const float a1 = railStartAngle + ((float)j + 0.66f) * railSweep / (float)ringSegments;
                    LogoLine(chartCX + cosf(a0) * rr, chartCY + sinf(a0) * rr,
                             chartCX + cosf(a1) * rr, chartCY + sinf(a1) * rr,
                             (ring == 3 ? 1.0f : 0.65f) * ui,
                             fieldR, fieldG, fieldB,
                             (ring == 3 ? 0.095f : 0.042f) * staticGeometryA);
                }
            }
            // Keep one faint, unbroken rail underneath the segmented Codex
            // rings. It belongs to the static chart, so it never vanishes or
            // breaks while records are travelling between list positions.
            const int continuousRailSegments = 96;
            for (int j = 0; j < continuousRailSegments; ++j) {
                const float a0 = railStartAngle
                    + railSweep * (float)j / (float)continuousRailSegments;
                const float a1 = railStartAngle
                    + railSweep * ((float)j + 1.0f)
                        / (float)continuousRailSegments;
                LogoLine(chartCX + cosf(a0) * railRadius,
                         chartCY + sinf(a0) * railRadius,
                         chartCX + cosf(a1) * railRadius,
                         chartCY + sinf(a1) * railRadius,
                         0.46f * ui,
                         fieldR, fieldG, fieldB,
                         0.052f * staticGeometryA);
            }

            // Secondary Codex-style orbit system: three smaller elliptical
            // tracks that repeat the same lower-half silhouette, plus anchor
            // lights. There is deliberately no upper-half track.
            for (int track = 0; track < 3; ++track) {
                const float rx = chartR * (0.28f + 0.105f * (float)track);
                const float ry = rx * (0.54f + 0.07f * (float)track);
                const float start = 0.08f + 0.05f * (float)track;
                const float sweep = 2.96f - 0.10f * (float)track;
                const int segments = 24;
                for (int seg = 0; seg < segments; ++seg) {
                    if ((seg + track * 2) % 5 == 2) continue;
                    const float a0 = start + sweep * (float)seg / (float)segments;
                    const float a1 = start + sweep * ((float)seg + 0.62f) / (float)segments;
                    LogoLine(chartCX + cosf(a0) * rx,
                             chartCY + sinf(a0) * ry,
                             chartCX + cosf(a1) * rx,
                             chartCY + sinf(a1) * ry,
                             (0.52f + 0.12f * (float)track) * ui,
                             fieldR, fieldG, fieldB,
                             (0.072f - 0.012f * (float)track) * staticGeometryA);
                }
                const float anchorA = start + sweep * 0.86f;
                DrawVisibleConstellNode(chartCX + cosf(anchorA) * rx,
                                        chartCY + sinf(anchorA) * ry,
                                        (2.0f - 0.18f * (float)track) * ui,
                                        fieldR, fieldG, fieldB,
                                        (0.36f - 0.05f * (float)track) * staticGeometryA,
                                        false);
            }
            float chainX[5] = {}, chainY[5] = {};
            float chainR[5] = {}, chainG[5] = {}, chainB[5] = {};
            float chainA[5] = {};
            bool chainValid[5] = {};
            float selectedNodeX = railXAt(2.0f);
            float selectedNodeY = railYAt(2.0f);
            for (int slot = 0; slot < itemTotal; ++slot) {
                const int itemIndex = itemIndices[slot];
                const SLItem& item = s_items[itemIndex];
                const float newRel = listRelFor(slot, selectedSlot);
                const float oldRel = hasFrom ? listRelFor(slot, fromSlot) : newRel;
                const bool oldVisible = oldRel > -999.0f;
                const bool newVisible = newRel > -999.0f;
                if (!oldVisible && !newVisible) continue;
                const float sourceRel = oldVisible
                    ? oldRel : newRel + s_listTransitionDir;
                const float targetRel = newVisible
                    ? newRel : oldRel - s_listTransitionDir;
                const float drawRel = sourceRel + (targetRel - sourceRel) * listEase;
                const float itemFade =
                    (oldVisible ? (1.0f - listEase) : 0.0f)
                    + (newVisible ? listEase : 0.0f);
                const bool oldSelected = hasFrom && slot == fromSlot;
                const bool newSelected = slot == selectedSlot;
                const float focus = std::max(
                    oldSelected ? (1.0f - listEase) : 0.0f,
                    newSelected ? listEase : 0.0f);
                const float hover = s_itemHov[itemIndex];
                const float active = std::max(focus, hover);
                const bool isCoreNode = item.key < KEY_THEME;
                const bool isProfileNode = item.key >= KEY_THEME
                                        && item.key < KEY_AUG;
                // Core unlocks are the gameplay-critical records. The whole
                // shop chart is large, so give its product constellations a
                // matching scale instead of leaving tiny nodes on a giant
                // field. Profiles and payloads remain slightly lighter.
                // Reduce the constellation body itself, including the
                // selected middle entry. Hover only changes emphasis/alpha;
                // it must never make a constellation grow again.
                const float baseMiniR = (isCoreNode ? 88.0f : 72.0f)
                                      + (isCoreNode ? 78.0f : 58.0f) * focus;
                // Hover is a readability state, not a scale-up state. Keep a
                // small shrink while the pointer rests on a record so the
                // constellation never swells against its label.
                const float hoverPreview = s_itemHov[itemIndex];
                const float miniR = baseMiniR * (1.0f - 0.15f * hoverPreview);
                const float miniRadius = miniR * ui;
                const float drawAx = railXAt(drawRel);
                const float drawAy = railYAt(drawRel);
                const bool itemPurchased = shopItemPurchased(item.key);
                // Purchased entries retain their own palette. Unpurchased
                // entries stay neutral; the selected one receives only a
                // restrained cyan focus so it does not read as owned.
                const float visualR = itemPurchased ? item.r
                    : (newSelected ? cyanR : 0.42f);
                const float visualG = itemPurchased ? item.g
                    : (newSelected ? cyanG : 0.52f);
                const float visualB = itemPurchased ? item.b
                    : (newSelected ? cyanB : 0.62f);
                const float labelW = std::max(180.0f * ui,
                                              std::min(300.0f * ui,
                                                       sw * 0.18f));
                // Hit testing belongs to the constellation node only. The
                // right-side label is intentionally excluded; including its
                // width made neighbouring records share one hover region.
                const float hoverPad = 16.0f * ui;
                const float hitRadiusX = baseMiniR * ui * 0.82f + hoverPad;
                const float hitRadiusY = baseMiniR * ui * 0.72f + hoverPad;
                const bool hov = inputReady
                              && mx >= drawAx - hitRadiusX
                              && mx <= drawAx + hitRadiusX
                              && my >= drawAy - hitRadiusY
                              && my <= drawAy + hitRadiusY;
                s_itemHov[itemIndex] = UpdateMenuCommandHover(s_itemHov[itemIndex], hov, dt);
                if (hov && lmbClick && listClickReady && !s_dragMoved) {
                    // Multiple large constellations may touch at their edge.
                    // Resolve the press once, by nearest node, instead of
                    // allowing iteration order to decide which item wins.
                    const float dx = (float)mx - drawAx;
                    const float dy = (float)my - drawAy;
                    const float dist2 = dx * dx + dy * dy;
                    if (dist2 < clickDist2) {
                        clickDist2 = dist2;
                        clickSlot = slot;
                    }
                }
                const bool fullItemFields = !listTransitionActive
                                          || oldSelected || newSelected;
                // The node field follows the travelling constellation itself.
                // Records that remain in the five visible slots keep their
                // field at full strength throughout the move; only records
                // entering or leaving the rail use the edge fade. This avoids
                // the CircleTexture appearing only after the list settles.
                const float nodeFieldA = designA
                    * ((oldVisible && newVisible) ? 1.0f : itemFade);
                DrawConstellationDisc(drawAx, drawAy, miniRadius * 0.90f,
                                      visualR, visualG, visualB,
                                      (itemPurchased
                                           ? (0.100f + 0.060f * focus)
                                           : (0.018f + 0.052f * focus))
                                          * nodeFieldA);
                DrawConstellationDisc(drawAx, drawAy, miniRadius * 1.32f,
                                      0.0f, 0.0f, 0.0f,
                                      (0.14f + 0.06f * active) * nodeFieldA,
                                      false);
                DrawConstellationDisc(drawAx, drawAy, miniRadius * 1.08f,
                                      0.0f, 0.0f, 0.0f,
                                      (0.26f + 0.20f * focus) * nodeFieldA);
                DrawArchiveConstellation(drawAx, drawAy, miniRadius,
                                         isCoreNode ? 3 : (item.key >= KEY_AUG ? 2 : 1),
                                         item.key, now * 0.18f,
                                         visualR, visualG, visualB,
                                         (itemPurchased
                                              ? (0.34f + 0.48f * focus + 0.28f * hover)
                                              : (0.20f + 0.34f * focus + 0.16f * hover))
                                             * designA * itemFade,
                                         ui, fullItemFields);
                DrawVisibleConstellLine(drawAx + miniRadius * 0.74f,
                                        drawAy,
                                        drawAx + miniRadius + 15.0f * ui,
                                        drawAy,
                                        (0.68f + 0.72f * focus) * ui,
                                        visualR, visualG, visualB,
                                        (itemPurchased
                                             ? (0.18f + 0.44f * focus + 0.18f * hover)
                                             : (0.10f + 0.30f * focus + 0.12f * hover))
                                            * designA * itemFade);
                DrawVisibleConstellNode(drawAx + miniRadius + 15.0f * ui, drawAy,
                                        (2.6f + 1.4f * focus) * ui,
                                        visualR, visualG, visualB,
                                        (itemPurchased
                                             ? (0.34f + 0.52f * focus + 0.22f * hover)
                                             : (0.18f + 0.38f * focus + 0.14f * hover))
                                            * designA * itemFade,
                                        false);
                const float labelContrast = isCoreNode
                    ? (0.66f + 0.34f * focus)
                    : (0.54f + 0.46f * focus);
                const float labelR = visualR + (whiteR - visualR) * labelContrast;
                const float labelG = visualG + (whiteG - visualG) * labelContrast;
                const float labelB = visualB + (whiteB - visualB) * labelContrast;
                drawFit(item.label, drawAx + miniRadius + 25.0f * ui,
                        drawAy - 12.0f * ui,
                        (isCoreNode ? 0.68f : 0.62f) * ui
                            + 0.22f * focus * ui,
                        labelW, labelR, labelG, labelB,
                        (0.64f + 0.38f * focus + 0.18f * hover) * itemFade);
                const float typeR = visualR + (whiteR - visualR) * 0.34f;
                const float typeG = visualG + (whiteG - visualG) * 0.34f;
                const float typeB = visualB + (whiteB - visualB) * 0.34f;
                DrawShadowedText(g_TextS,
                                 isCoreNode ? L"코어"
                                 : (isProfileNode ? L"프로필" : L"페이로드"),
                                 drawAx + miniRadius + 25.0f * ui,
                                 drawAy + 13.0f * ui,
                                 (isCoreNode ? 0.42f : 0.38f) * ui
                                     + 0.06f * focus * ui,
                                  typeR, typeG, typeB,
                                  (0.48f + 0.38f * focus + 0.20f * hover)
                                      * designA * itemFade,
                                  0.58f);
                if (newSelected) {
                    selectedNodeX = drawAx;
                    selectedNodeY = drawAy;
                }
                if (newVisible) {
                    const int chainSlot = (int)(newRel + 0.5f);
                    if (chainSlot >= 0 && chainSlot < 5) {
                        chainX[chainSlot] = drawAx;
                        chainY[chainSlot] = drawAy;
                        chainR[chainSlot] = visualR;
                        chainG[chainSlot] = visualG;
                        chainB[chainSlot] = visualB;
                        chainA[chainSlot] = itemFade;
                        chainValid[chainSlot] = true;
                    }
                }
            }
            if (clickSlot >= 0 && clickSlot != selectedSlot) {
                int deltaSlot = clickSlot - selectedSlot;
                // Follow the shortest direction around the circular list.
                // Without wrapping this sign, clicking an item near either
                // end can animate through the wrong side of the catalogue.
                if (deltaSlot > itemTotal / 2) deltaSlot -= itemTotal;
                if (deltaSlot < -itemTotal / 2) deltaSlot += itemTotal;
                s_listTransitionDir = (deltaSlot >= 0) ? 1.0f : -1.0f;
                s_listFromKey = s_selKey;
                s_listTransitionT = 0.0f;
                s_selKey = s_items[itemIndices[clickSlot]].key;
                s_detailT = 0.0f;
            }
            for (int chainSlot = 1; chainSlot < 5; ++chainSlot) {
                if (!chainValid[chainSlot - 1] || !chainValid[chainSlot]) continue;
                DrawVisibleConstellLine(chainX[chainSlot - 1], chainY[chainSlot - 1],
                                        chainX[chainSlot], chainY[chainSlot],
                                        0.72f * ui,
                                        chainR[chainSlot], chainG[chainSlot], chainB[chainSlot],
                                        0.16f * designA * chainA[chainSlot]);
            }
        }

        // Fixed right-center record block. Keep the field borderless; the
        // CircleTexture supplies contrast while typography carries the read.
        // The detail glow is a fixed UI light, independent of the selected
        // item colour, and is anchored around the action button.
        const float detailFieldRCol = cyanR;
        const float detailFieldGCol = cyanG;
        const float detailFieldBCol = cyanB;
        const float detailFieldCX = detailLayout.actionX
                                  + detailLayout.actionW * 0.50f;
        const float detailFieldCY = detailLayout.actionY;
        const float detailFieldR = std::max(300.0f * ui, detailW * 0.98f);
        // Give the lower-right readout the same chromatic depth as the
        // constellation field. Keep the light behind the dark contrast layer
        // so the copy remains readable over bright gameplay backgrounds.
        DrawConstellationDisc(detailFieldCX, detailFieldCY, detailFieldR * 0.92f,
                              detailFieldRCol, detailFieldGCol, detailFieldBCol,
                              0.040f * staticFieldA);
        DrawConstellationDisc(detailFieldCX, detailFieldCY, detailFieldR * 0.78f,
                              detailFieldRCol, detailFieldGCol, detailFieldBCol,
                              0.032f * staticFieldA);
        DrawConstellationDisc(detailFieldCX, detailFieldCY, detailFieldR * 0.44f,
                              detailFieldRCol, detailFieldGCol, detailFieldBCol,
                              0.022f * staticFieldA);
        DrawConstellationDisc(detailFieldCX, detailFieldCY, detailFieldR,
                              0.0f, 0.0f, 0.012f, 0.14f * staticFieldA,
                              false);

        // The detail readout intentionally stays typographic. Its orbital
        // lines and marker nodes compete with the information, especially on
        // the bright background, so the right edge is now the only anchor.
        drawRightFit(g_TextS, detailType, detailRight,
                     detailLayout.titleY - 38.0f * ui,
                     0.44f * ui, detailW, detailR, detailG, detailB,
                     0.96f * detailA, 0.72f);
        float titleSc = 1.08f * ui;
        while (titleSc > 0.74f * ui && g_TextL.Width(detailTitle, titleSc) > detailW)
            titleSc -= 0.04f * ui;
        drawRightFit(g_TextL, detailTitle, detailRight, detailLayout.titleY,
                     titleSc, detailW, 0.96f, 0.98f, 1.00f,
                     0.99f * detailA, 0.92f);

        const float copyY = detailLayout.copyY;
        if (detailIsMeta) {
            drawRightFit(g_TextS, L"출격 전 적용",
                         detailRight, copyY, detailDescriptionSc, detailW,
                         0.82f, 0.90f, 0.98f, 0.90f * detailA, 0.88f);
        } else if (detailIsTheme) {
            drawRightFit(g_TextS, L"아웃게임 색상 적용",
                         detailRight, copyY, detailDescriptionSc, detailW,
                         detailR, detailG, detailB, 0.88f * detailA, 0.86f);
        } else if (detailIsAug && detailId >= 0 && detailId < AUG_TOTAL) {
            const wchar_t* desc = AugDesc(ALL_AUGS[detailId]);
            if (desc && desc[0]) {
                std::wstring text(desc);
                size_t pos = 0;
                int line = 0;
                while (pos < text.size() && line < 3) {
                    size_t slash = text.find(L" / ", pos);
                    std::wstring segment = slash == std::wstring::npos
                        ? text.substr(pos) : text.substr(pos, slash - pos);
                    pos = slash == std::wstring::npos ? text.size() : slash + 3;
                    while (!segment.empty() && segment.front() == L' ') segment.erase(0, 1);
                    while (!segment.empty() && segment.back() == L' ') segment.pop_back();
                    drawRightFit(g_TextS, segment.c_str(), detailRight,
                                 copyY + line * 32.0f * ui,
                                 detailDescriptionSc, detailW,
                                 0.76f, 0.84f, 0.94f, 0.82f, 0.86f);
                    ++line;
                }
            }
        }

        const float tradeY = detailLayout.tradeY;
        const float actionX = detailLayout.actionX;
        const float actionW = detailLayout.actionW;
        const float actionY = detailLayout.actionY;
        // PLAY's readout uses a much larger information scale. Keep the
        // transaction/status copy at that same visual weight instead of
        // leaving level, balance, and purchase-result lines in tiny helper
        // text.
        const float detailInfoSc = purchaseInfoSc;
        auto drawWalletSummary = [&](float y, long long cost,
                                     bool canAfford, bool owned,
                                     bool finished) {
            auto drawStatusMarker = [&](const wchar_t* label, float markerY) {
                const float labelW = g_TextS.Width(label, detailInfoSc);
                DrawVisibleConstellNode(detailRight - labelW - 15.0f * ui,
                                        markerY + 6.0f * ui, 2.3f * ui,
                                        detailFieldRCol, detailFieldGCol,
                                        detailFieldBCol,
                                        0.58f * detailA, false);
            };
            wchar_t balanceBuf[64];
            swprintf_s(balanceBuf, L"보유 별가루  %lld", g_Coins);
            drawRightFit(g_TextS, balanceBuf, detailRight, y + 12.0f * ui,
                         detailInfoSc, detailW, whiteR, whiteG, whiteB,
                         0.78f * detailA, 0.60f);

            if (finished) {
                drawStatusMarker(L"최대 레벨", y - 30.0f * ui);
                drawRightFit(g_TextS, L"최대 레벨", detailRight,
                             y - 30.0f * ui, detailInfoSc, detailW,
                             detailR, detailG, detailB,
                             0.92f * detailA, 0.64f);
                return;
            }
            if (owned) {
                drawStatusMarker(L"구매 완료", y - 30.0f * ui);
                drawRightFit(g_TextS, L"구매 완료",
                             detailRight, y - 30.0f * ui, detailInfoSc,
                             detailW, detailR, detailG, detailB,
                             0.92f * detailA, 0.64f);
                return;
            }

            wchar_t costBuf[64];
            swprintf_s(costBuf, L"비용  %lld 별가루", cost);
            drawRightFit(g_TextS, costBuf, detailRight, y - 30.0f * ui,
                         detailInfoSc, detailW, goldR, goldG, goldB,
                         0.98f * detailA, 0.64f);

            if (canAfford) {
                wchar_t verdictBuf[64];
                swprintf_s(verdictBuf, L"구매 후  %lld 별가루",
                           g_Coins - cost);
                drawRightFit(g_TextS, verdictBuf, detailRight,
                             y + 52.0f * ui, detailInfoSc, detailW,
                             detailR, detailG, detailB,
                             0.86f * detailA, 0.62f);
            }
        };
        if (detailIsMeta && detailId >= 0 && detailId < META_COUNT) {
            const MetaDef& md = META_DEFS[detailId];
            const int curLv = g_MetaLv[detailId];
            const long long cost = MetaNextCost(detailId);
            wchar_t levelBuf[48]; swprintf_s(levelBuf, L"레벨  %d / %d", curLv, md.maxLv);
            drawRightFit(g_TextS, levelBuf, detailRight, tradeY - 60.0f * ui,
                         detailInfoSc, detailW, whiteR, whiteG, whiteB,
                         0.82f * detailA, 0.60f);
            const bool canBuy = cost >= 0 && g_Coins >= cost && curLv < md.maxLv;
            drawWalletSummary(tradeY, cost, canBuy, false,
                              curLv >= md.maxLv || cost < 0);
            const bool actionHover = inputReady && mx >= actionX
                                   && mx <= actionX + actionW
                                   && my >= actionY - 52.0f * ui
                                   && my <= actionY + 52.0f * ui;
            if (actionHover && lmbClick && canBuy) {
                g_Coins -= cost; ++g_MetaLv[detailId]; SaveGame();
            }
            wchar_t actionBuf[64];
            if (curLv >= md.maxLv) swprintf_s(actionBuf, L"최대 레벨");
            else if (!canBuy) swprintf_s(actionBuf, L"별가루 부족");
            else if (curLv <= 0) swprintf_s(actionBuf, L"구매");
            else swprintf_s(actionBuf, L"모듈 강화");
            drawFixedAction(actionX, actionY, actionW, actionBuf,
                            detailR, detailG, detailB,
                            canBuy || curLv >= md.maxLv, actionHover,
                            curLv >= md.maxLv ? SHOP_ACTION_ACTIVE
                                : (canBuy ? SHOP_ACTION_READY
                                          : SHOP_ACTION_BLOCKED));
        } else if (detailIsTheme && detailId >= 0 && detailId < ACCENT_COUNT) {
            const AccentTheme& theme = ACCENT_THEMES[detailId];
            const bool owned = ThemeOwned(detailId);
            const long long cost = theme.cost;
            const bool canBuy = !owned && g_Coins >= cost;
            // Shop owns purchase only. Loadout/equip remains a PLAY action.
            drawWalletSummary(tradeY, cost, canBuy, owned, false);
            const bool actionHover = inputReady && mx >= actionX
                                   && mx <= actionX + actionW
                                   && my >= actionY - 52.0f * ui
                                   && my <= actionY + 52.0f * ui;
            if (actionHover && lmbClick && canBuy) {
                g_Coins -= cost;
                g_ThemeOwned |= (1 << detailId);
                SaveGame();
            }
            drawFixedAction(actionX, actionY, actionW,
                            owned ? L"구매 완료"
                                  : (canBuy ? L"구매" : L"별가루 부족"),
                            detailR, detailG, detailB,
                            canBuy, actionHover,
                            owned ? SHOP_ACTION_ACTIVE
                                  : (canBuy ? SHOP_ACTION_READY
                                            : SHOP_ACTION_BLOCKED));
        } else if (detailIsAug) {
            drawRightFit(g_TextS, L"패시브 효과",
                         detailRight, tradeY - 20.0f * ui,
                         detailDescriptionSc, detailW, whiteR, whiteG, whiteB,
                         0.82f * detailA, 0.60f);
            drawFixedAction(actionX, actionY, actionW, L"패시브",
                            detailR, detailG, detailB, false, false,
                            SHOP_ACTION_CATALOGUE);
        }

        DrawShadowedText(g_TextS, L"ESC / RMB  뒤로가기",
                         sw - std::max(74.0f, sw * 0.075f) -
                         g_TextS.Width(L"ESC / RMB  뒤로가기", 0.31f * ui),
                         sh - 48.0f * ui, 0.31f * ui,
                         0.52f, 0.68f, 0.82f, 0.60f * designA, 0.58f);

        // All CircleTexture/icon work for the SHOP frame is complete here.
        // Release the final text pass only now so no late glow can cover a
        // tab, list label, price, or action label.
        FlushSceneTextCommands();
        g_DeferSceneText = false;

        if (finishBackAfterRender) {
            s_backExit = false;
            s_backOutT = 0.0f;
            s_ShopBackRequested = false;
            s_MainMenuArmoryPanel = false;
            s_MainMenuResumeFromPanel = true;
            g_MainMenuEntryT = 1.0f;
            g_GameManager.currentState = GameState::MAIN_MENU;
        }
        return;
    }

}

static void Scene_CodexInline(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    (void)c.fireTimer; (void)c.reset;

    static constexpr int CAT_COUNT = 3;
    static int   s_cat = 0;
    static float s_rootHover[CAT_COUNT + 1] = {};
    static int   s_sel[CAT_COUNT] = { -1, -1, -1 };
    static float s_scroll[CAT_COUNT] = {};
    static bool  s_backExit = false;
    static float s_backOutT = 0.0f;
    static bool  s_prevRmb = false;
    static bool  s_prevEsc = false;
    static int   s_prevCat = -1;
    static int   s_prevSel = -9999;
    static float s_decryptT = 1.0f;
    static float s_itemHover[512] = {};
    static float s_itemReveal[512] = {};
    static float s_orbitAngle[CAT_COUNT] = {};
    static float s_orbitTarget[CAT_COUNT] = {};
    static float s_orbitVelocity[CAT_COUNT] = {};
    static float s_displaySlot[CAT_COUNT] = {};
    static float s_displayTarget[CAT_COUNT] = {};
    static bool  s_prevUp = false;
    static bool  s_prevDown = false;
    static bool  s_prevEnter = false;
    static int   s_navHoldDir = 0;
    static float s_navHoldT = 0.0f;
    static float s_navRepeatT = 0.0f;
    static bool  s_dragging = false;
    static bool  s_dragMoved = false;
    static float s_dragAccum = 0.0f;
    static double s_dragLastY = 0.0;
    static bool  s_prevSearchBackspace = false;
    static float s_searchBackspaceT = 0.0f;
    static bool  s_searchBackspaceRepeating = false;
    static float s_searchHover = 0.0f;
    static bool  s_searchFocused = false;
    static std::wstring s_prevSearch;

    g_CodexSearchInputEnabled = s_searchFocused && !s_backExit;

    float dt = delta; if (dt > 0.05f) dt = 0.05f;
    g_CodexEntryT += dt;
    if (g_CodexEntryT > 1.0f) g_CodexEntryT = 1.0f;
    const float now = (float)glfwGetTime();

    const bool rawRmb = c.window && glfwGetMouseButton(c.window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    const bool rmbClick = rawRmb && !s_prevRmb;
    s_prevRmb = rawRmb;
    const bool rawEsc = c.window && glfwGetKey(c.window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    const bool escClick = rawEsc && !s_prevEsc;
    s_prevEsc = rawEsc;

    const bool rawSearchBackspace = c.window &&
        glfwGetKey(c.window, GLFW_KEY_BACKSPACE) == GLFW_PRESS;
    if (!s_searchFocused || !rawSearchBackspace) {
        s_searchBackspaceT = 0.0f;
        s_searchBackspaceRepeating = false;
    } else if (!s_prevSearchBackspace) {
        if (g_CodexSearchLen > 0)
            g_CodexSearch[--g_CodexSearchLen] = 0;
        s_searchBackspaceT = 0.0f;
        s_searchBackspaceRepeating = false;
    } else if (g_CodexSearchLen > 0) {
        s_searchBackspaceT += dt;
        const float repeatDelay = s_searchBackspaceRepeating ? 0.055f : 0.30f;
        if (s_searchBackspaceT >= repeatDelay) {
            g_CodexSearch[--g_CodexSearchLen] = 0;
            s_searchBackspaceT = 0.0f;
            s_searchBackspaceRepeating = true;
        }
    }
    s_prevSearchBackspace = rawSearchBackspace;

    const std::wstring currentSearch(g_CodexSearch);
    if (currentSearch != s_prevSearch) {
        s_prevSearch = currentSearch;
        for (int i = 0; i < CAT_COUNT; ++i) {
            s_displaySlot[i] = 0.0f;
            s_displayTarget[i] = 0.0f;
        }
        s_prevSel = -9999;
        s_decryptT = 0.0f;
    }

    const bool inputReady = !s_backExit && g_CodexEntryT >= 0.55f;
    auto beginBack = [&]() {
        if (!inputReady) return;
        s_backExit = true;
        s_backOutT = 0.0f;
        s_CodexBackRequested = false;
    };
    if (s_CodexBackRequested) {
        if (inputReady) beginBack();
        else s_CodexBackRequested = false;
    }
    if (rmbClick || escClick) beginBack();

    bool finishBackAfterRender = false;
    if (s_backExit) {
        s_backOutT += dt;
        if (s_backOutT >= 0.42f) {
            s_backOutT = 0.42f;
            finishBackAfterRender = true;
        }
    }

    const float entryOldOut = Smoothstep(LogoClamp01(g_CodexEntryT / 0.35f));
    const float entryTreeIn = Smoothstep(LogoClamp01((g_CodexEntryT - 0.20f) / 0.35f));
    const float entryDetailIn = Smoothstep(LogoClamp01((g_CodexEntryT - 0.34f) / 0.30f));
    const float backP = s_backExit ? Smoothstep(LogoClamp01(s_backOutT / 0.42f)) : 0.0f;
    const float oldMenuA = s_backExit ? backP : (1.0f - entryOldOut);
    const float wake = s_backExit ? (1.0f - backP) : entryTreeIn;
    const float rightWake = s_backExit ? (1.0f - backP) : entryDetailIn;

    int li = LangIndex(); if (li < 0 || li >= 3) li = 0;
    const int nli = (li == 0) ? 0 : 1;

    // All archive CircleTextures share one page-level reveal. It is not tied
    // to the selected record, so scrolling/dragging only moves existing
    // nodes instead of making every texture pop back in.
    const float textureReveal = s_backExit
        ? std::max(0.0f, 1.0f - backP)
        : Smoothstep(LogoClamp01(g_CodexEntryT / 0.72f));
    SetSceneTextureReveal(textureReveal);

    const float uiS = std::max(0.70f, std::min(sw * 0.94f / 1640.0f, sh * 0.90f / 910.0f));
    const float mainBW = std::min(560.0f, std::max(420.0f, sw * 0.34f));
    const float mainBH = 70.0f;
    const float mainGap = 15.0f;
    const float mainTotalH = 5.0f * mainBH + 4.0f * mainGap;
    const float mainX = std::max(58.0f, sw * 0.075f);
    const float mainY = MainMenuCommandStartY(sh);
    const float astralY = mainY + 2.0f * (mainBH + mainGap);
    const float astralMidY = astralY + mainBH * 0.5f;

    const float rootX = mainX + (s_backExit ? backP * 180.0f * uiS : -(1.0f - wake) * 82.0f * uiS);
    const float rootY = mainY;
    const float rootW = std::min(360.0f * uiS, mainBW * 0.70f);
    const float rootH = mainBH;
    const float rootGap = mainGap;
    // Keep the archive columns anchored to the existing canvas, while the
    // left command rail itself sits in the lower-left corner.
    const float rootTotalH = rootH * (CAT_COUNT + 1) + rootGap * CAT_COUNT;
    const float railY = sh - rootTotalH - 48.0f * uiS;
    const InlineThreeColumnLayout columns = BuildInlineThreeColumnLayout(
        sw, sh, uiS, rootX, rootY, rootW, rootH);
    // Match the RUN_CONFIG canvas bounds so both inline screens carry the
    // same visual weight instead of growing from the lower root-menu anchor.
    const float archivePanelY = std::max(74.0f, sh * 0.12f);
    const float archivePanelBottom = sh - std::max(88.0f, 104.0f * uiS);
    const float archivePanelH = std::max(260.0f * uiS,
                                         archivePanelBottom - archivePanelY);
    const float depth2X = columns.contentX;
    const float depth2Y = archivePanelY;
    const float depth2W = columns.contentW;
    const float depth2H = archivePanelH;
    const float rightX = columns.detailX;
    const float rightW = columns.detailW;
    const float rightY = archivePanelY;
    const float rightH = archivePanelH;
    const float archiveHeaderH = 68.0f * uiS;
    const float listX = depth2X;
    const float listY = depth2Y + archiveHeaderH;
    const float listW = depth2W;
    const float listH = std::max(180.0f * uiS, depth2H - archiveHeaderH - 10.0f * uiS);
    const float archiveX = depth2X - 18.0f * uiS;
    const float archiveY = depth2Y - 4.0f * uiS;
    const float archiveRight = rightX + rightW;
    const float archiveW = archiveRight - archiveX;
    const float archiveH = depth2H + 8.0f * uiS;

    DrawSharedMenuDim(sw, sh, mainX, railY, mainBW, mainTotalH,
                      std::max(oldMenuA, wake), true);

    struct RootDef { const wchar_t* route; const wchar_t* sub; float r, g, b; };
    static const RootDef ROOTS[CAT_COUNT + 1] = {
        { L"ENTITIES", L"\uAD00\uCE21\uCCB4", 0.48f, 0.82f, 1.00f },
        { L"MODULES",  L"\uBAA8\uB4C8",       0.62f, 0.52f, 1.00f },
        { L"APEX",     L"\uC815\uC810",       0.96f, 0.76f, 0.30f },
        { L"BACK",     L"\uB4A4\uB85C",       0.42f, 0.62f, 0.78f },
    };
    const RootDef& curRoot = ROOTS[s_cat];

    // Keep the archive on the main-menu canvas.  The hierarchy is carried by
    // the root rail, orbital chart and typography instead of a window-shaped
    // data plate.
    BindMainShader();

    // Main-menu expansion ghost.
    const float oldOut = 1.0f - oldMenuA;
    const float returnLogoA = s_backExit ? (0.42f + 0.58f * backP) : 0.42f;
    DrawMainOnedowLogo(sw, sh,
                       returnLogoA * oldMenuA,
                       LogoClamp01(oldMenuA + 0.22f * wake),
                       sw * 0.30f - 160.0f * oldOut,
                       0.82f);
    {
        const int ghostLang = LangIndex();
        const float ghostSlide = 250.0f * oldOut;
        const float ghostRailY = MainMenuButtonRailStartY(sh);
        const float anchorX = mainX - ghostSlide - 36.0f;
        BindMainShader();
        drawRect(anchorX, std::max(22.0f, MainLogoTop(sh) - 20.0f),
                 1.2f, mainTotalH + 170.0f,
                 0.48f, 0.82f, 1.0f, 0.12f * oldMenuA);
        for (int i = 0; i < 5; ++i) {
            const bool focus = (!s_backExit && i == 2);
            const float rowY = ghostRailY + (float)i * (mainBH + mainGap);
            const float rowX = mainX - ghostSlide - (focus ? 0.0f : 26.0f * oldOut);
            const float rowA = (s_backExit ? 0.82f : focus ? 0.76f : 0.18f) * oldMenuA;
            const float active = focus ? 1.0f : 0.0f;
            if (focus) {
                drawRect(rowX - 18.0f, rowY + mainBH * 0.5f - 31.0f,
                         2.0f, 62.0f, 0.48f, 0.82f, 1.0f, 0.56f * oldMenuA);
                drawRect(rowX - 4.0f, rowY + mainBH * 0.50f,
                         mainBW * 0.42f, 1.1f, 0.48f, 0.82f, 1.0f, 0.12f * oldMenuA);
                drawDiamond(rowX - 17.0f, rowY + mainBH * 0.5f,
                            5.0f, 0.48f, 0.82f, 1.0f, 0.72f * oldMenuA);
            }
            float routeSc = 1.04f;
            while (routeSc > 0.82f &&
                   g_TextL.Width(MainMenuRouteLabel(ghostLang, i), routeSc) > mainBW)
                routeSc -= 0.04f;
            const float routeY2 = rowY + 2.0f;
            const float subY = routeY2 +
                               g_TextL.Height(MainMenuRouteLabel(ghostLang, i), routeSc) - 3.0f;
            const float gr = s_backExit ? 1.0f : 1.0f - 0.52f * (1.0f - active);
            const float gg = s_backExit ? 1.0f : 1.0f - 0.18f * (1.0f - active);
            g_TextL.Draw(MainMenuRouteLabel(ghostLang, i), rowX, routeY2, routeSc,
                         gr, gg, 1.0f, rowA);
            g_TextS.Draw(MainMenuSubtitleLabel(ghostLang, i), rowX + 4.0f, subY, 0.48f,
                         0.70f + 0.16f * active,
                         0.75f + 0.14f * active,
                         0.82f + 0.10f * active,
                         (s_backExit ? 0.70f : focus ? 0.82f : 0.26f) * oldMenuA);
        }
        const float bridgeA = std::min(oldMenuA, wake);
        LogoLine(mainX - ghostSlide + mainBW * 0.42f, astralMidY,
                 rootX - 26.0f * uiS, astralMidY,
                 1.0f * uiS, 0.48f, 0.82f, 1.0f, 0.22f * bridgeA);
        drawDiamond(rootX - 26.0f * uiS, astralMidY,
                    3.3f * uiS, 0.48f, 0.82f, 1.0f, 0.46f * bridgeA);
    }

    if (s_prevCat != s_cat) {
        s_prevCat = s_cat;
        s_decryptT = 0.0f;
        for (float& hover : s_itemHover) hover = 0.0f;
        for (float& reveal : s_itemReveal) reveal = 0.0f;
    }

    // Depth 1 root.
    BindMainShader();
    drawRect(rootX - 36.0f, railY - 20.0f,
             1.2f, rootH * 4.0f + rootGap * 3.0f + 40.0f,
             0.48f, 0.82f, 1.00f, 0.13f * wake);
    for (int i = 0; i < CAT_COUNT + 1; ++i) {
        const bool isBack = (i == CAT_COUNT);
        const float tx = rootX;
        const float ty = railY + (float)i * (rootH + rootGap);
        const float hitX = tx - 26.0f;
        const float hitRight = std::min(tx + rootW + 18.0f * uiS, depth2X - 34.0f * uiS);
        const bool hov = inputReady
            && (mx >= hitX && mx < hitRight && my >= ty && my < ty + rootH);
        s_rootHover[i] = UpdateMenuCommandHover(s_rootHover[i], hov, dt);

        if (hov && lmb && !g_LmbPrev) {
            if (isBack) beginBack();
            else if (s_cat != i) s_cat = i;
        }

        const bool sel = (!isBack && s_cat == i);
        const RootDef& rd = ROOTS[i];
        const float reveal = s_backExit ? wake : MenuCommandReveal(g_CodexEntryT, i);
        const float rowA = reveal * wake;
        const float selectPulse = sel ? (0.12f + 0.12f * sinf(now * 3.3f)) : 0.0f;
        const float bx = tx + (1.0f - reveal) * 34.0f - 10.0f * s_rootHover[i];
        const float by = ty;

        DrawUnifiedMenuCommand(rd.route, rd.sub, bx, by, rootW, rootH,
                               rd.r, rd.g, rd.b, rowA, s_rootHover[i], sel,
                               selectPulse, now + (float)i * 0.17f, 1.02f, 0.48f);
    }

    struct CItem {
        bool isGroup;
        int key;
        const wchar_t* label;
        bool seen;
        float r, g, b;
    };
    CItem items[512];
    int itemCount = 0;
    static const int kCodexMobIds[] = {
        CM_ROTOR, CM_SCOPE, CM_SWARM, CM_GENESIS, CM_GRAVIS, CM_QUASAR
    };
    auto addGroup = [&](const wchar_t* label, float r, float g, float b) {
        if (itemCount < 512) items[itemCount++] = { true, -1, label, true, r, g, b };
    };
    auto addItem = [&](int key, const wchar_t* label, bool seen,
                       const wchar_t* const* searchNames, int searchNameCount,
                       float r, float g, float b) {
        if (g_CodexSearchLen > 0 && (!seen ||
            !CodexMatchAny(searchNames, searchNameCount))) return;
        if (itemCount < 512) items[itemCount++] = { false, key, label, seen, r, g, b };
    };

    if (s_cat == 0) {
        addGroup(L"SIGNAL CLASS", 0.48f, 0.82f, 1.00f);
        for (int i : kCodexMobIds)
            addItem(i, CodexMobSeen(i) ? MobName(i) : L"???", CodexMobSeen(i),
                    MobLocalizedNames(i), LANG_COUNT,
                    0.48f, 0.82f, 1.00f);
    } else if (s_cat == 1) {
        int sorted[AUG_TOTAL], n = 0;
        for (int i = 0; i < AUG_TOTAL; ++i)
            if (!AugRemoved(ALL_AUGS[i].type)) sorted[n++] = i;
        std::sort(sorted, sorted + n, [](int a, int b) { return AugTierIndexLess(a, b); });
        AugRarity prevR = (AugRarity)-1;
        for (int k = 0; k < n; ++k) {
            int i = sorted[k];
            AugRarity rar = ALL_AUGS[i].rarity;
            float rr, rg, rb; GetRarityColor(rar, rr, rg, rb);
            if (rar != prevR) {
                addGroup(GetRarityKR(rar), rr, rg, rb);
                prevR = rar;
            }
            addItem(i, CodexAugSeen(i) ? AugName(ALL_AUGS[i]) : L"???",
                    CodexAugSeen(i), ALL_AUGS[i].locName, LANG_COUNT, rr, rg, rb);
        }
    } else {
        addGroup(L"APEX SIGNAL", 0.96f, 0.76f, 0.30f);
        for (int i = 0; i < BOSS_CODEX_COUNT; ++i)
            addItem(i, BossCodexSeen(i) ? BossCodexName(i) : L"???",
                     BossCodexSeen(i), BossCodexLocalizedNames(i), LANG_COUNT,
                     0.96f, 0.76f, 0.30f);
    }

    int unlockTotal[CAT_COUNT] = {
        (int)(sizeof(kCodexMobIds) / sizeof(kCodexMobIds[0])), 0, BOSS_CODEX_COUNT
    };
    int unlockSeen[CAT_COUNT] = {};
    for (int id : kCodexMobIds)
        if (CodexMobSeen(id)) ++unlockSeen[0];
    for (int i = 0; i < AUG_TOTAL; ++i) {
        if (AugRemoved(ALL_AUGS[i].type)) continue;
        ++unlockTotal[1];
        if (CodexAugSeen(i)) ++unlockSeen[1];
    }
    for (int i = 0; i < BOSS_CODEX_COUNT; ++i)
        if (BossCodexSeen(i)) ++unlockSeen[2];

    int observedCount = 0;
    int recordCount = 0;
    for (int i = 0; i < itemCount; ++i) {
        if (items[i].isGroup) continue;
        ++recordCount;
        if (items[i].seen) ++observedCount;
    }
    wchar_t archivePath[96];
    wchar_t archiveProgress[64];
    swprintf_s(archivePath, L"ASTRAL ARCHIVE / %ls", ROOTS[s_cat].route);
    swprintf_s(archiveProgress, L"%02d / %02d OBSERVED", observedCount, recordCount);
    // The restored layout deliberately carries no page header or enclosing
    // archive panel: the root commands, orbit, and record itself are enough.
    (void)archivePath;

    // Use the open upper-left canvas for search and a compact global unlock
    // summary. Search filters only discovered records in the active category.
    auto codexText = [&](const wchar_t* kr, const wchar_t* en) -> const wchar_t* {
        return nli == 0 ? kr : en;
    };
    const float utilityX = mainX;
    const float utilityW = std::min(sw * 0.40f, rootW + 120.0f * uiS);
    const float searchW = std::max(180.0f * uiS,
                                   std::min(420.0f * uiS,
                                            utilityW - 24.0f * uiS));
    const float utilityY = std::max(76.0f, sh * 0.11f);
    const float searchY = utilityY + 30.0f * uiS;
    const float searchH = 46.0f * uiS;
    const bool searchHover = inputReady && mx >= utilityX && mx <= utilityX + searchW
        && my >= searchY && my <= searchY + searchH;
    s_searchHover = UiApproach(s_searchHover, searchHover ? 1.0f : 0.0f, dt, 10.0f);
    const float clearW = 32.0f * uiS;
    const bool hasSearch = g_CodexSearchLen > 0;
    const bool clearHover = hasSearch && inputReady
        && mx >= utilityX + searchW - clearW && mx <= utilityX + searchW
        && my >= searchY && my <= searchY + searchH;
    const bool searchClick = (searchHover || clearHover) && lmb && !g_LmbPrev;
    if (searchClick)
        s_searchFocused = true;
    else if (lmb && !g_LmbPrev)
        s_searchFocused = false;
    if (clearHover && searchClick)
        CodexSearchClear();
    g_CodexSearchInputEnabled = s_searchFocused && !s_backExit;

    BindMainShader();
    const wchar_t* searchTitle = s_searchFocused
        ? codexText(L"\uAC80\uC0C9 / \uC785\uB825\uC911", L"SEARCH / ACTIVE")
        : codexText(L"\uAC80\uC0C9", L"SEARCH");
    DrawShadowedText(g_TextS, searchTitle,
                     utilityX, utilityY, 0.46f * uiS,
                     curRoot.r, curRoot.g, curRoot.b,
                     (0.72f + 0.18f * s_searchFocused) * wake, 0.58f);
    drawRect(utilityX, searchY, searchW, searchH,
             0.010f, 0.020f, 0.038f,
             (0.24f + 0.05f * s_searchHover + 0.06f * s_searchFocused) * wake);
    drawConstellFrame(utilityX, searchY, searchW, searchH,
                      curRoot.r, curRoot.g, curRoot.b,
                      (0.28f + 0.22f * s_searchHover +
                       0.18f * s_searchFocused) * wake,
                      12.0f * uiS, 2.2f * uiS,
                      0.025f * s_searchFocused * wake);
    const float searchMidY = searchY + searchH * 0.50f;
    const float searchIconX = utilityX + 20.0f * uiS;
    DrawVisibleConstellNode(searchIconX, searchMidY, 4.0f * uiS,
                            curRoot.r, curRoot.g, curRoot.b,
                            (0.56f + 0.24f * s_searchFocused) * wake,
                            false);
    LogoLine(searchIconX + 3.0f * uiS, searchMidY + 3.0f * uiS,
             searchIconX + 9.0f * uiS, searchMidY + 9.0f * uiS,
             1.2f * uiS, curRoot.r, curRoot.g, curRoot.b,
             (0.54f + 0.22f * s_searchFocused) * wake);
    std::wstring searchDisplay;
    if (hasSearch)
        searchDisplay = std::wstring(g_CodexSearch);
    else if (!s_searchFocused)
        searchDisplay = std::wstring(codexText(L"\uC785\uB825\uD558\uC5EC \uAC80\uC0C9", L"TYPE TO FILTER"));
    const bool searchCaretOn = s_searchFocused && (((int)(now * 2.0f)) & 1) == 0;
    if (searchCaretOn)
        searchDisplay += L"|";
    float searchSc = 0.48f * uiS;
    const float searchMaxW = std::max(30.0f * uiS, searchW - clearW - 58.0f * uiS);
    while (searchSc > 0.30f * uiS &&
           g_TextS.Width(searchDisplay.c_str(), searchSc) > searchMaxW)
        searchSc -= 0.025f * uiS;
    const float searchTextX = utilityX + 40.0f * uiS;
    const float searchTextY = searchMidY
        - g_TextS.Height(searchDisplay.c_str(), searchSc) * 0.50f;
    DrawShadowedText(g_TextS, searchDisplay.c_str(),
                     searchTextX, searchTextY,
                     searchSc,
                     hasSearch ? 0.92f : 0.42f,
                     hasSearch ? 0.96f : 0.56f,
                     hasSearch ? 1.00f : 0.70f,
                     (hasSearch ? 0.92f : 0.70f) * wake, 0.62f);
    if (hasSearch) {
        const float clearSc = 0.42f * uiS;
        DrawShadowedText(g_TextS, L"X",
                         utilityX + searchW - clearW * 0.68f,
                         searchMidY - g_TextS.Height(L"X", clearSc) * 0.50f,
                         clearSc,
                         clearHover ? 1.0f : 0.58f,
                         clearHover ? 1.0f : 0.70f,
                         clearHover ? 1.0f : 0.82f,
                         (clearHover ? 0.98f : 0.62f) * wake, 0.60f);
    }

    const float progressTitleY = searchY + searchH + 24.0f * uiS;
    DrawShadowedText(g_TextS,
                     codexText(L"\uB3C4\uAC10 \uD574\uAE08 \uBAA9\uB85D", L"UNLOCK PROGRESS"),
                     utilityX, progressTitleY, 0.48f * uiS,
                     curRoot.r, curRoot.g, curRoot.b, 0.84f * wake, 0.58f);
    const float progressBarX = utilityX;
    const float progressBarW = std::max(48.0f * uiS,
                                        std::min(320.0f * uiS,
                                                 utilityW - 16.0f * uiS));
    const float resultSc = 0.32f * uiS;
    const float resultW = g_TextS.Width(archiveProgress, resultSc);
    DrawShadowedText(g_TextS, archiveProgress,
                     progressBarX + progressBarW - resultW, progressTitleY + 2.0f * uiS,
                     resultSc, 0.62f, 0.72f, 0.86f, 0.66f * wake, 0.52f);
    // Give each unlock entry its own two-line block: a large readout first,
    // then the archive line directly underneath it.  This keeps the progress
    // data legible without competing with the search field above.
    const float progressRowY = progressTitleY + 32.0f * uiS;
    const float progressRowStep = 68.0f * uiS;
    for (int i = 0; i < CAT_COUNT; ++i) {
        const float rowY = progressRowY + (float)i * progressRowStep;
        const RootDef& progressRoot = ROOTS[i];
        DrawShadowedText(g_TextS, progressRoot.route,
                         utilityX, rowY, 0.62f * uiS,
                         progressRoot.r, progressRoot.g, progressRoot.b,
                         (s_cat == i ? 0.98f : 0.72f) * wake, 0.54f);
        wchar_t progressBuf[32];
        swprintf_s(progressBuf, L"%02d / %02d", unlockSeen[i], unlockTotal[i]);
        const float progressSc = 0.52f * uiS;
        const float progressW = g_TextS.Width(progressBuf, progressSc);
        DrawShadowedText(g_TextS, progressBuf,
                         progressBarX + progressBarW - progressW, rowY, progressSc,
                         0.86f, 0.92f, 1.00f, 0.88f * wake, 0.54f);
        const float trackY = rowY + 32.0f * uiS;
        const float fillW = progressBarW * (unlockTotal[i] > 0
            ? (float)unlockSeen[i] / (float)unlockTotal[i] : 0.0f);
        DrawVisibleConstellLine(progressBarX, trackY,
                                progressBarX + progressBarW, trackY,
                                0.75f * uiS, progressRoot.r, progressRoot.g, progressRoot.b,
                                0.18f * wake);
        if (fillW > 0.0f)
            DrawVisibleConstellLine(progressBarX, trackY,
                                    progressBarX + fillW, trackY,
                                    1.7f * uiS, progressRoot.r, progressRoot.g, progressRoot.b,
                                    0.76f * wake);
        DrawVisibleConstellNode(progressBarX + fillW, trackY, 2.4f * uiS,
                                progressRoot.r, progressRoot.g, progressRoot.b,
                                0.64f * wake, false);
    }

    bool selectedVisible = false;
    for (int i = 0; i < itemCount; ++i) {
        if (!items[i].isGroup && items[i].key == s_sel[s_cat]) {
            selectedVisible = true;
            break;
        }
    }
    if (!selectedVisible) s_sel[s_cat] = -1;

    if (s_sel[s_cat] < 0) {
        for (int i = 0; i < itemCount; ++i) {
            if (!items[i].isGroup) { s_sel[s_cat] = items[i].key; break; }
        }
    }
    if (s_sel[s_cat] != s_prevSel) {
        s_prevSel = s_sel[s_cat];
        s_decryptT = 0.0f;
    }
    s_decryptT = UiApproach(s_decryptT, 1.0f, dt, 8.0f);

    // Orbital record list.  Selection owns one angular target; rendering reads
    // only the damped angle so labels and miniature constellations never apply
    // a second, conflicting interpolation.
    int recordSlots[512] = {};
    int recordSlotCount = 0;
    int selectedSlot = 0;
    for (int i = 0; i < itemCount; ++i) {
        if (items[i].isGroup) continue;
        if (items[i].key == s_sel[s_cat]) selectedSlot = recordSlotCount;
        recordSlots[recordSlotCount++] = i;
    }
    if (recordSlotCount > 0 && s_sel[s_cat] < 0) {
        s_sel[s_cat] = items[recordSlots[0]].key;
        selectedSlot = 0;
    }
    if (s_displaySlot[s_cat] == 0.0f && s_displayTarget[s_cat] == 0.0f && selectedSlot != 0) {
        s_displaySlot[s_cat] = s_displayTarget[s_cat] = (float)selectedSlot;
    }

    const bool keyUpNow = c.window &&
        (glfwGetKey(c.window, GLFW_KEY_UP) == GLFW_PRESS ||
         glfwGetKey(c.window, GLFW_KEY_W) == GLFW_PRESS);
    const bool keyDownNow = c.window &&
        (glfwGetKey(c.window, GLFW_KEY_DOWN) == GLFW_PRESS ||
         glfwGetKey(c.window, GLFW_KEY_S) == GLFW_PRESS);
    const bool keyEnterNow = c.window && glfwGetKey(c.window, GLFW_KEY_ENTER) == GLFW_PRESS;
    int stepRequest = 0;
    const int navDir = keyUpNow == keyDownNow ? 0 : (keyUpNow ? -1 : 1);
    if (inputReady && navDir != 0) {
        if (navDir != s_navHoldDir) {
            s_navHoldDir = navDir;
            s_navHoldT = 0.0f;
            s_navRepeatT = 0.0f;
            stepRequest = navDir; // immediate first move
        } else {
            s_navHoldT += dt;
            // Hold for ~0.25s, then accelerate by shrinking the repeat interval.
            if (s_navHoldT >= 0.25f) {
                s_navRepeatT += dt;
                // Cap the navigation rate so long holds never skip too fast.
                const float repeatInterval = std::max(0.05f,
                    0.26f - (s_navHoldT - 0.25f) * 0.075f);
                if (s_navRepeatT >= repeatInterval) {
                    s_navRepeatT = 0.0f;
                    stepRequest = navDir;
                }
            }
        }
    } else {
        s_navHoldDir = 0;
        s_navHoldT = 0.0f;
        s_navRepeatT = 0.0f;
    }
    s_prevUp = keyUpNow;
    s_prevDown = keyDownNow;
    s_prevEnter = keyEnterNow;

    const float chartCX = sw;
    const float chartCY = sh * 0.51f;
    const float chartR = std::max(sh * 0.58f, sw * 0.44f);
    // Entity silhouettes use their gameplay footprint (Gravis includes a
    // wide gravity field), so give that category more vertical breathing room.
    const float itemR = sw * 0.70f;
    const float rowStep = 245.0f * uiS;
    const bool overOrbit = inputReady && mx >= depth2X - 30.0f * uiS &&
        mx <= sw && my >= archivePanelY && my <= archivePanelBottom;
    if (overOrbit && lmb && !g_LmbPrev) {
        s_dragging = true;
        s_dragMoved = false;
        s_dragAccum = 0.0f;
        s_dragLastY = my;
    } else if (!lmb) {
        s_dragging = false;
        s_dragAccum = 0.0f;
    }
    if (s_dragging && lmb) {
        s_dragAccum += (float)(my - s_dragLastY);
        s_dragLastY = my;
        // Dragging is intentionally less sensitive than wheel/keyboard input:
        // require almost a full visual row before advancing one record.
        const float dragStepThreshold = rowStep * 0.88f;
        if (fabsf(s_dragAccum) >= dragStepThreshold) {
            // Drag direction follows the reversed wheel convention.
            stepRequest = s_dragAccum > 0.0f ? -1 : 1;
            s_dragAccum += s_dragAccum > 0.0f ? -dragStepThreshold : dragStepThreshold;
            s_dragMoved = true;
        }
    }
    if (overOrbit && g_ScrollAccum != 0.0f)
        stepRequest = g_ScrollAccum > 0.0f ? -1 : 1; // reversed wheel direction
    g_ScrollAccum = 0.0f;
    if (recordSlotCount > 0 && stepRequest != 0) {
        selectedSlot = (selectedSlot + stepRequest + recordSlotCount) % recordSlotCount;
        s_sel[s_cat] = items[recordSlots[selectedSlot]].key;
        s_displayTarget[s_cat] += (float)stepRequest;
    }
    {
        const float diff = s_displayTarget[s_cat] - s_displaySlot[s_cat];
        const float speed = 10.0f;
        s_displaySlot[s_cat] += diff * std::min(1.0f, dt * speed);
        if (fabsf(diff) < 0.002f) {
            s_displaySlot[s_cat] = s_displayTarget[s_cat];
        }
    }

    // The real CircleTexture pair is deliberately behind chart and copy.
    DrawConstellationDisc(chartCX - chartR * 0.12f, chartCY, chartR * 1.48f,
                          0.0f, 0.0f, 0.0f, 0.68f * rightWake);
    DrawConstellationDisc(chartCX - chartR * 0.30f, chartCY, chartR * 0.72f,
                          0.22f, 0.30f, 0.38f, 0.42f * rightWake);
    // A dedicated contrast field keeps the wrapped record rail readable on
    // white/bright gameplay backgrounds. It overlaps the main sight field so
    // the list feels embedded in the constellation instead of boxed in.
    const float listFieldCX = chartCX - itemR + 180.0f * uiS;
    DrawConstellationDisc(listFieldCX, chartCY, 700.0f * uiS,
                          0.0f, 0.0f, 0.0f, 0.22f * rightWake);
    DrawConstellationDisc(listFieldCX + 90.0f * uiS, chartCY, 510.0f * uiS,
                          curRoot.r * 0.10f, curRoot.g * 0.10f, curRoot.b * 0.12f,
                          0.075f * rightWake);
    BindMainShader();
    for (int ring = 0; ring < 4; ++ring) {
        const float rr = chartR * (0.56f + 0.145f * (float)ring);
        const int segments = 72;
        for (int j = 0; j < segments; ++j) {
            if ((j + ring * 2) % 6 == 3) continue;
            const float a0 = (float)j * 6.2831853f / (float)segments;
            const float a1 = ((float)j + 0.64f) * 6.2831853f / (float)segments;
            LogoLine(chartCX + cosf(a0) * rr, chartCY + sinf(a0) * rr,
                     chartCX + cosf(a1) * rr, chartCY + sinf(a1) * rr,
                     (ring == 3 ? 1.0f : 0.65f) * uiS,
                     curRoot.r, curRoot.g, curRoot.b,
                     (ring == 3 ? 0.16f : 0.075f) * wake);
        }
    }
    // Selection datum: no radial spokes across the chart interior.
    LogoLine(chartCX - chartR, chartCY, chartCX - chartR * 0.72f, chartCY,
             1.1f * uiS, curRoot.r, curRoot.g, curRoot.b, 0.42f * wake);
    drawDiamond(chartCX - chartR, chartCY, 4.0f * uiS,
                curRoot.r, curRoot.g, curRoot.b, 0.78f * wake);

    // Connected observation rail. Entries slide one row at a time along this
    // datum, including the wrapped first↔last transition.
    auto railXAt = [&](float row) {
        const float dy = row * rowStep;
        const float span = std::max(40.0f, itemR * itemR - dy * dy);
        return chartCX - sqrtf(span);
    };
    for (int r = -5; r < 5; ++r) {
        LogoLine(railXAt((float)r), chartCY + (float)r * rowStep,
                 railXAt((float)(r + 1)), chartCY + (float)(r + 1) * rowStep,
                 0.72f * uiS, curRoot.r, curRoot.g, curRoot.b, 0.12f * wake);
    }

    for (int slot = 0; slot < recordSlotCount; ++slot) {
        float relF = (float)slot - s_displaySlot[s_cat];
        while (relF > (float)recordSlotCount * 0.5f) relF -= (float)recordSlotCount;
        while (relF < -(float)recordSlotCount * 0.5f) relF += (float)recordSlotCount;
        const int rel = (int)std::round(relF);
        if (rel < -6 || rel > 6) continue;
        const CItem& itm = items[recordSlots[slot]];
        const float ax = railXAt(relF);
        const float ay = chartCY + relF * rowStep;
        const float distanceFade = std::max(0.0f, 1.0f - fabsf((float)rel) / 6.0f);
        const bool isSel = (rel == 0);
        const float hitW = 440.0f * uiS;
        const float hitH = 100.0f * uiS;
        const bool hov = inputReady && mx >= ax - 80.0f * uiS && mx <= ax + hitW &&
                         my >= ay - hitH * 0.5f && my <= ay + hitH * 0.5f;
        if (hov && lmb && !g_LmbPrev && !s_dragMoved) {
            float jump = (float)slot - (float)selectedSlot;
            while (jump > (float)recordSlotCount * 0.5f) jump -= (float)recordSlotCount;
            while (jump < -(float)recordSlotCount * 0.5f) jump += (float)recordSlotCount;
            s_sel[s_cat] = itm.key;
            selectedSlot = slot;
            s_displayTarget[s_cat] += jump;
        }
        const float targetFocus = isSel ? 1.0f : (hov ? 0.72f : 0.0f);
        // Per-record focus smoothing makes the horizontal datum grow/shrink
        // instead of snapping when the active record changes.
        s_itemHover[slot] = UiApproach(s_itemHover[slot], targetFocus, dt, 8.0f);
        const float targetReveal = (fabsf(relF) <= 5.0f) ? 1.0f : 0.0f;
        s_itemReveal[slot] = UiApproach(s_itemReveal[slot], targetReveal, dt, 6.5f);
        const float reveal = s_itemReveal[slot] * wake;
        if (reveal <= 0.005f) continue;
        const float active = std::max(s_itemHover[slot], distanceFade * 0.38f);
        const float miniX = ax;
        // Keep the gameplay silhouette readable, but compact enough that the
        // wider entity row spacing still shows roughly 3-4 records per page.
        const float focusScale = std::max(0.0f, std::min(1.0f, s_itemHover[slot]));
        const float miniBase = (s_cat == 1) ? 34.0f : 18.0f;
        const float miniFocus = (s_cat == 1) ? 38.0f : 24.0f;
        const float miniR = (miniBase + miniFocus * focusScale) *
                           (0.58f + 0.42f * reveal) * uiS;
        const float signalR = itm.seen ? itm.r : 0.26f;
        const float signalG = itm.seen ? itm.g : 0.32f;
        const float signalB = itm.seen ? itm.b : 0.40f;
        // Local black halo around each record keeps the constellation core
        // legible without darkening the entire archive surface.
        DrawConstellationDisc(miniX, ay, miniR * 2.80f,
                              0.0f, 0.0f, 0.0f,
                              (0.16f + 0.10f * active) * reveal);
        // Category-colored core light: every record gets a restrained glow,
        // while the selected record naturally becomes brighter via `active`.
        DrawConstellationDisc(miniX, ay, miniR * 1.14f,
                              signalR, signalG, signalB,
                              (itm.seen ? (0.055f + 0.095f * active)
                                        : (0.018f + 0.030f * active)) * reveal);
        if (s_cat == 0 && !itm.seen) {
            DrawUnknownConstellation(miniX, ay, miniR * 1.12f,
                                     now * 0.18f, (0.58f + 0.30f * active)
                                     * reveal, uiS, itm.key);
        } else {
            DrawArchiveConstellation(miniX, ay, miniR, s_cat, itm.key,
                                     now * 0.18f, signalR, signalG, signalB,
                                     (0.28f + 0.66f * active) * reveal, uiS);
        }
        const float lineLength = (250.0f + 130.0f * s_itemHover[slot]) * uiS;
        const float lineEndX = ax + lineLength;
        LogoLine(miniX + miniR * 0.68f, ay, lineEndX, ay,
                 (0.96f + 0.56f * s_itemHover[slot]) * uiS,
                 signalR, signalG, signalB,
                 (itm.seen ? 0.12f : 0.06f
                     + 0.12f * active) * reveal);
        DrawVisibleConstellNode(lineEndX, ay, (isSel ? 4.0f : 2.8f) * uiS,
                                signalR, signalG, signalB,
                                (itm.seen ? 0.20f : 0.10f
                                    + 0.24f * active) * reveal, false);
        if (isSel)
            // The vertical selection datum is a stable anchor; hover only
            // changes the horizontal line length and must not shift this bar.
            drawRect(ax + 118.0f * uiS,
                     ay - 22.0f * uiS, 2.0f * uiS, 44.0f * uiS,
                     signalR, signalG, signalB,
                     (itm.seen ? 0.76f : 0.42f) * reveal);
        float tsc = (isSel ? 0.68f : 0.58f) * uiS;
        DrawShadowedText(g_TextS, itm.label, ax + 132.0f * uiS,
                         ay - g_TextS.Height(itm.label, tsc) * 0.5f,
                         tsc,
                         itm.seen ? 0.84f + 0.16f * active : 0.58f,
                         itm.seen ? 0.88f + 0.12f * active : 0.62f,
                         itm.seen ? 0.94f + 0.06f * active : 0.70f,
                         (0.38f + 0.60f * active) * reveal, 0.68f);
    }

    // A search can legitimately hide every record.  Keep this state explicit
    // instead of rendering an "UNASSIGNED" detail panel over an empty list.
    // The chart field is already drawn above, so the empty state sits in the
    // same archive surface and remains readable on every background.
    if (recordSlotCount == 0 && hasSearch) {
        const float emptyCX = rightX + rightW * 0.50f;
        const float emptyY = rightY + rightH * 0.46f;
        const wchar_t* emptyTitle = codexText(L"검색 결과 없음", L"NO MATCHES");
        const wchar_t* emptyHint = codexText(L"다른 검색어를 입력하세요", L"TRY A DIFFERENT SEARCH");
        DrawConstellationDisc(emptyCX, emptyY, 34.0f * uiS,
                              curRoot.r, curRoot.g, curRoot.b,
                              0.12f * rightWake);
        DrawVisibleConstellNode(emptyCX, emptyY, 7.0f * uiS,
                                curRoot.r, curRoot.g, curRoot.b,
                                0.62f * rightWake, false);
        const float titleSc = 0.78f * uiS;
        const float hintSc = 0.46f * uiS;
        DrawShadowedText(g_TextL, emptyTitle,
                         emptyCX - g_TextL.Width(emptyTitle, titleSc) * 0.5f,
                         emptyY + 30.0f * uiS, titleSc,
                         0.92f, 0.96f, 1.0f, 0.94f * rightWake, 0.66f);
        DrawShadowedText(g_TextS, emptyHint,
                         emptyCX - g_TextS.Width(emptyHint, hintSc) * 0.5f,
                         emptyY + 62.0f * uiS, hintSc,
                         0.64f, 0.76f, 0.88f, 0.78f * rightWake, 0.56f);
        if (finishBackAfterRender) {
            s_backExit = false;
            s_backOutT = 0.0f;
            s_CodexBackRequested = false;
            s_searchFocused = false;
            g_CodexSearchInputEnabled = false;
            s_MainMenuCodexPanel = false;
            s_MainMenuResumeFromPanel = true;
            g_MainMenuEntryT = 1.0f;
            g_GameManager.currentState = GameState::MAIN_MENU;
        }
        return;
    }


    auto scramble = [&](const wchar_t* src, int seed) -> std::wstring {
        static const wchar_t* glyphs = L"01/\\#*@+-=_";
        if (!src) return L"";
        std::wstring out(src);
        int gCount = 10;
        int tick = (int)(now * 90.0f);
        for (size_t i = 0; i < out.size(); ++i) {
            if (out[i] == L' ' || out[i] == L':' || out[i] == L'|' || out[i] == L'[' || out[i] == L']')
                continue;
            out[i] = glyphs[(seed + tick + (int)i * 7) % gCount];
        }
        return out;
    };
    auto drawScanTextS = [&](const wchar_t* text, float x, float y, float sc,
                             float r, float g, float b, float a, int seed) {
        if (s_decryptT < 0.36f) {
            std::wstring t = scramble(text, seed);
            DrawShadowedText(g_TextS, t.c_str(), x, y, sc, r, g, b,
                             a * (0.72f + 0.28f * s_decryptT), 0.62f);
        } else {
            DrawShadowedText(g_TextS, text, x, y, sc, r, g, b, a, 0.66f);
        }
    };
    auto drawScanTextL = [&](const wchar_t* text, float x, float y, float sc,
                             float r, float g, float b, float a, int seed) {
        if (s_decryptT < 0.36f) {
            std::wstring t = scramble(text, seed);
            DrawShadowedText(g_TextL, t.c_str(), x, y, sc, r, g, b,
                             a * (0.72f + 0.28f * s_decryptT), 0.68f);
        } else {
            DrawShadowedText(g_TextL, text, x, y, sc, r, g, b, a, 0.74f);
        }
    };

    auto selectedSeen = [&]() -> bool {
        int key = s_sel[s_cat];
        if (key < 0) return false;
        if (s_cat == 0) return CodexMobSeen(key);
        if (s_cat == 1) return key >= 0 && key < AUG_TOTAL && CodexAugSeen(key);
        return key >= 0 && key < BOSS_CODEX_COUNT && BossCodexSeen(key);
    };
    auto selectedTitle = [&]() -> const wchar_t* {
        int key = s_sel[s_cat];
        if (key < 0) return L"UNASSIGNED";
        if (s_cat == 0) return selectedSeen() ? MobName(key) : L"???";
        if (s_cat == 1) return selectedSeen() ? AugName(ALL_AUGS[key]) : L"???";
        return selectedSeen() ? BossCodexName(key) : L"???";
    };
    auto selectedDesc = [&]() -> const wchar_t* {
        int key = s_sel[s_cat];
        if (!selectedSeen()) {
            static const wchar_t* kUnknownDesc[3] = {
                L"\uBBF8\uD655\uC778 \uC2E0\uD638",
                L"UNDISCOVERED SIGNAL",
                L"\u672A\u78BA\u8A8D\u30B7\u30B0\u30CA\u30EB"
            };
            return kUnknownDesc[std::max(0, std::min(2, li))];
        }
        if (s_cat == 0) return MobDesc(key);
        if (s_cat == 1) return AugDesc(ALL_AUGS[key]);
        return BossCodexDesc(key);
    };

    const int selKey = s_sel[s_cat] < 0 ? 0 : s_sel[s_cat];
    const bool seen = selectedSeen();
    float cr = curRoot.r, cg = curRoot.g, cb = curRoot.b;
    if (s_cat == 1 && selKey >= 0 && selKey < AUG_TOTAL)
        GetRarityColor(ALL_AUGS[selKey].rarity, cr, cg, cb);

    // Fixed record block inside the chart, matching the original wide-open
    // composition.  It does not move with the orbit or label animation.
    const float detailX = sw * 0.70f;
    const float detailW = std::max(320.0f * uiS,
                                   std::min(sw * 0.25f, sw - detailX - 54.0f * uiS));
    const float infoY = sh * 0.42f;
    BindMainShader();
    drawRect(detailX, infoY, detailW, 1.1f * uiS,
             cr, cg, cb, 0.28f * rightWake);
    drawDiamond(detailX, infoY, 3.0f * uiS,
                cr, cg, cb, 0.62f * rightWake);
    if (s_decryptT < 0.96f) {
        const float scanY = infoY + 10.0f * uiS
            + (rightY + rightH - infoY - 28.0f * uiS) * Smoothstep(s_decryptT);
        drawRect(detailX, scanY, detailW, 1.0f * uiS,
                 cr, cg, cb, 0.22f * rightWake * (1.0f - Smoothstep(s_decryptT)));
    }

    wchar_t idBuf[96];
    if (s_cat == 0) {
        if (seen) swprintf_s(idBuf, L"ID : ENTITY_%02d", selKey);
        else      swprintf_s(idBuf, L"ID : ENTITY_??");
    }
    else if (s_cat == 1) swprintf_s(idBuf, L"ID : MODULE_%03d", selKey);
    else swprintf_s(idBuf, L"ID : APEX_%02d", selKey);
    wchar_t statBuf[128];
    if (s_cat == 0)
        swprintf_s(statBuf, seen
            ? L"CLASS : HOSTILE_SIGNAL | STATUS : CATALOGUED"
            : L"CLASS : UNKNOWN_SIGNAL | STATUS : UNRESOLVED");
    else if (s_cat == 1)
        swprintf_s(statBuf, L"CLASS : MODULE_ARCHIVE | STATUS : %ls", seen ? L"ACQUIRED" : L"NO_DATA");
    else
        swprintf_s(statBuf, L"CLASS : APEX_ENTITY | STATUS : %ls", seen ? L"OBSERVED" : L"NO_DATA");

    float titleSc = 0.92f * uiS;
    while (titleSc > 0.66f * uiS
           && g_TextL.Width(selectedTitle(), titleSc) > detailW)
        titleSc -= 0.04f * uiS;
    float descSc = 0.50f * uiS;
    while (descSc > 0.40f * uiS
           && g_TextS.Width(selectedDesc(), descSc) > detailW)
        descSc -= 0.025f * uiS;

    drawScanTextS(idBuf, detailX, infoY + 16.0f * uiS,
                  0.46f * uiS, 0.62f, 0.72f, 0.86f,
                  0.74f * rightWake, selKey + s_cat * 31);
    drawScanTextL(selectedTitle(), detailX, infoY + 44.0f * uiS,
                  titleSc, 0.94f, 0.98f, 1.0f,
                  0.98f * rightWake, selKey + 7);

    const float descY = infoY + 44.0f * uiS
        + g_TextL.Height(selectedTitle(), titleSc) + 12.0f * uiS;
    DrawShadowedText(g_TextS, L"RECORD SUMMARY", detailX, descY,
                     0.42f * uiS, cr, cg, cb, 0.72f * rightWake, 0.54f);
    drawScanTextS(selectedDesc(), detailX, descY + 24.0f * uiS,
                  descSc, 0.76f, 0.84f, 0.94f,
                  0.86f * rightWake, selKey + 13);

    const float metaY = descY + 66.0f * uiS;
    LogoLine(detailX, metaY - 10.0f * uiS,
             detailX + detailW, metaY - 10.0f * uiS,
             0.7f * uiS, cr, cg, cb, 0.14f * rightWake);
    drawScanTextS(statBuf, detailX, metaY,
                  0.46f * uiS, 0.84f, 0.90f, 0.98f,
                  0.88f * rightWake, selKey + 19);

    wchar_t recordBuf[128];
    if (s_cat == 0) {
        if (seen) swprintf_s(recordBuf,
                             L"RECORD TYPE : HOSTILE ENTITY | ACCESS : READ ONLY");
        else swprintf_s(recordBuf,
                        L"RECORD TYPE : UNKNOWN SIGNAL | ACCESS : LOCKED");
    }
    else if (s_cat == 1)
        swprintf_s(recordBuf, L"RECORD TYPE : AUGMENT MODULE | SOURCE : IN-RUN");
    else
        swprintf_s(recordBuf, L"RECORD TYPE : APEX ENCOUNTER | ACCESS : READ ONLY");
        drawScanTextS(recordBuf, detailX, metaY + 25.0f * uiS,
                  0.42f * uiS, 0.62f, 0.70f, 0.82f,
                  0.70f * rightWake, selKey + 23);
    g_TextS.Draw(L"[ ARCHIVE_READ_ONLY ]", detailX, metaY + 52.0f * uiS,
                 0.46f * uiS, cr, cg, cb, 0.82f * rightWake);

    if (finishBackAfterRender) {
        s_backExit = false;
        s_backOutT = 0.0f;
        s_CodexBackRequested = false;
        s_searchFocused = false;
        g_CodexSearchInputEnabled = false;
        s_MainMenuCodexPanel = false;
        s_MainMenuResumeFromPanel = true;
        g_MainMenuEntryT = 1.0f;
        g_GameManager.currentState = GameState::MAIN_MENU;
    }
}

void Scene_Codex(const SceneCtx& c) {
    if (s_MainMenuCodexPanel) {
        Scene_CodexInline(c);
        return;
    }

    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    GLFWwindow* window = c.window;
    (void)c.fireTimer; (void)c.reset;

    // ── State ─────────────────────────────────────────────────────────────
    static int   s_cat         = 0;
    static int   s_prevCat     = -1;
    static float s_catHover[3] = {};
    static float s_catFocus[3] = {};
    static float s_panelT      = 1.0f;
    static bool  s_catLoaded[3]= {};
    static int   s_sel[3]      = { -1, -1, -1 };
    static float s_scroll[3]   = {};
    static float s_detailFadeT = 0.0f;
    static int   s_prevSel     = -2;

    DrawMenuBackground(sw, sh, delta, false);

    float dt = delta; if (dt > 0.05f) dt = 0.05f;
    g_CodexEntryT += dt;
    if (g_CodexEntryT > 1.0f) g_CodexEntryT = 1.0f;
    const float now = (float)glfwGetTime();

    const float wake      = Smoothstep(std::min(1.0f, g_CodexEntryT / 0.50f));
    const float rightWake = Smoothstep(std::min(1.0f,
        std::max(0.0f, g_CodexEntryT - 0.10f) / 0.44f));
    SetSceneTextureReveal(Smoothstep(LogoClamp01(g_CodexEntryT / 0.72f)));

    int li = LangIndex(); if (li < 0 || li >= 3) li = 0;
    const int nli = (li == 0) ? 0 : 1;

    // ── Layout ────────────────────────────────────────────────────────────
    const float TARGET_W = 1760.0f, TARGET_H = 930.0f;
    float uiS = std::max(0.72f, std::min(sw * 0.975f / TARGET_W, sh * 0.920f / TARGET_H));
    if (uiS > 1.30f) uiS = 1.30f;
    const float panelW  = TARGET_W * uiS;
    const float panelH  = TARGET_H * uiS;
    const float panelX  = (sw - panelW) * 0.5f;
    const float panelY  = (sh - panelH) * 0.5f;
    const float headerH = 82.0f * uiS;
    const float footerH = 74.0f * uiS;
    const float bodyY   = panelY + headerH;
    const float bodyH   = panelH - headerH - footerH;
    const float footY   = panelY + panelH - footerH + 12.0f * uiS;
    const float leftW   = panelW * 0.260f;
    const float colGap  = 14.0f * uiS;
    const float leftX   = panelX;
    const float rightX  = leftX + leftW + colGap;
    const float rightW  = (panelX + panelW) - rightX - 8.0f * uiS;
    const float tabH    = 54.0f * uiS;
    const float listAreaY = bodyY + 64.0f * uiS;
    const float listAreaH = bodyH - 64.0f * uiS;
    DrawPersistentSceneLeftVignette(sw, sh, 0.64f);
    DrawSceneRadialVignette(rightX + rightW * 0.44f, bodyY + bodyH * 0.50f,
                            std::min(rightW, bodyH) * 0.60f, 0.44f * rightWake);

    // ── Category definitions ──────────────────────────────────────────────
    struct CatDef { const wchar_t* id; const wchar_t* name[2]; float r, g, b; };
    static const CatDef kCats[] = {
        { L"ENTITIES", { L"\xAD00\xCE21\xCCB4", L"ENTITIES" }, 0.48f, 0.82f, 1.00f },
        { L"MODULES",  { L"\xBAA8\xB4C8",  L"MODULES"  }, 0.62f, 0.52f, 1.00f },
        { L"APEX",     { L"\xC815\xC810",  L"APEX"     }, 0.96f, 0.76f, 0.30f },
    };
    const int kCatCount = 3;
    const CatDef& cat = kCats[s_cat];
    const float uiR = cat.r, uiG = cat.g, uiB = cat.b;
    const float inkR = 0.88f, inkG = 0.94f, inkB = 1.00f;
    const float idleR = 0.50f, idleG = 0.62f, idleB = 0.76f;

    auto hit = [&](float x, float y, float w, float h) -> bool {
        return mx >= x && mx <= x + w && my >= y && my <= y + h;
    };

    // ── Transitions ───────────────────────────────────────────────────────
    if (s_prevCat != s_cat) {
        s_prevCat = s_cat;
        s_panelT = s_catLoaded[s_cat] ? 1.0f : 0.0f;
    }
    s_panelT = UiApproach(s_panelT, 1.0f, delta, 10.0f);
    if (s_panelT > 0.995f)
        s_catLoaded[s_cat] = true;

    int curSel = s_sel[s_cat];
    if (curSel != s_prevSel) { s_prevSel = curSel; s_detailFadeT = 0.0f; }
    s_detailFadeT = UiApproach(s_detailFadeT, 1.0f, delta, 8.0f);

    // ── Helper lambdas ────────────────────────────────────────────────────
    auto drawCenterS = [&](const wchar_t* text, float x, float y, float w,
                           float sc, float r, float g, float b, float a) {
        while (sc > 0.36f * uiS && g_TextS.Width(text, sc) > w - 12.0f * uiS)
            sc -= 0.025f * uiS;
        float tw = g_TextS.Width(text, sc);
        g_TextS.Draw(text, x + (w - tw) * 0.5f, y, sc, r, g, b, a);
    };
    auto drawFitS = [&](const wchar_t* text, float x, float y, float maxW,
                        float sc, float minSc, float r, float g, float b, float a) {
        while (sc > minSc && g_TextS.Width(text, sc) > maxW)
            sc -= 0.025f * uiS;
        g_TextS.Draw(text, x, y, sc, r, g, b, a);
    };
    auto drawCBkt = [&](float x, float y, float w, float h,
                        float r, float g, float b, float a) {
        float cL = std::min(w * 0.24f, 10.0f * uiS), ct = 1.3f * uiS;
        drawRect(x,       y,       cL, ct, r, g, b, a);
        drawRect(x,       y,       ct, cL, r, g, b, a);
        drawRect(x+w-cL,  y,       cL, ct, r, g, b, a);
        drawRect(x+w-ct,  y,       ct, cL, r, g, b, a);
        drawRect(x,       y+h-ct,  cL, ct, r, g, b, a);
        drawRect(x,       y+h-cL,  ct, cL, r, g, b, a);
        drawRect(x+w-cL,  y+h-ct,  cL, ct, r, g, b, a);
        drawRect(x+w-ct,  y+h-cL,  ct, cL, r, g, b, a);
        float ns = 3.0f * uiS;
        drawDiamond(x,   y,   ns, r, g, b, a);
        drawDiamond(x+w, y,   ns, r, g, b, a);
        drawDiamond(x,   y+h, ns, r, g, b, a);
        drawDiamond(x+w, y+h, ns, r, g, b, a);
    };
    auto drawAstrolabeGrid = [&](float x, float y, float w, float h,
                                 float r, float g, float b, float a) {
        if (a <= 0.001f) return;
        const float cx = x + w * 0.50f;
        const float cy = y + h * 0.50f;
        const float rad = std::min(w, h) * 0.31f;
        const float tLine = std::max(0.70f, 0.82f * uiS);
        auto ep = [&](float ang, float rx, float ry, float rot, float& ox, float& oy) {
            float ca = cosf(ang), sa = sinf(ang);
            float cr = cosf(rot), sr2 = sinf(rot);
            float lx = ca * rx;
            float ly = sa * ry;
            ox = cx + lx * cr - ly * sr2;
            oy = cy + lx * sr2 + ly * cr;
        };

        DrawSceneRadialVignette(cx, cy, rad * 1.95f, 0.34f * a);
        BindMainShader();
        for (int axis = 0; axis < 8; ++axis) {
            float a0 = now * 0.035f + (float)axis * 0.7853982f;
            DrawVisibleConstellLine(cx, cy,
                                    cx + cosf(a0) * rad * 0.82f,
                                    cy + sinf(a0) * rad * 0.82f,
                                    tLine, r, g, b, 0.050f * a);
        }

        for (int ring = 0; ring < 4; ++ring) {
            const float rx = rad * (0.66f + 0.13f * (float)ring);
            const float ry = rad * ((ring == 0) ? 0.66f : (0.23f + 0.10f * (float)ring));
            const float rot = now * (0.07f + 0.018f * (float)ring) + (float)ring * 0.72f;
            const int steps = 72;
            for (int s = 0; s < steps; ++s) {
                if (((s + ring) % 5) == 2) continue;
                float p0 = (float)s / (float)steps;
                float p1 = ((float)s + 0.56f) / (float)steps;
                float x0, y0, x1, y1;
                ep(p0 * 6.2831853f, rx, ry, rot, x0, y0);
                ep(p1 * 6.2831853f, rx, ry, rot, x1, y1);
                DrawVisibleConstellLine(x0, y0, x1, y1, tLine, r, g, b,
                                        (ring == 0 ? 0.155f : 0.105f) * a);
            }
        }

        for (int n = 0; n < 12; ++n) {
            float ang = now * 0.11f + (float)n * 0.5235988f;
            float nr = rad * (0.48f + 0.42f * ((n % 3) / 2.0f));
            DrawVisibleConstellNode(cx + cosf(ang) * nr,
                                    cy + sinf(ang) * nr,
                                    ((n % 4) == 0 ? 3.7f : 2.5f) * uiS,
                                    r, g, b, ((n % 4) == 0 ? 0.30f : 0.20f) * a);
        }
        DrawVisibleConstellNode(cx, cy, 7.0f * uiS, r, g, b, 0.36f * a);
    };
    auto drawInfoQuad = [&](float x, float y, float w, float h, float a) {
        if (a <= 0.001f) return;
        BindMainShader();
        drawRect(x + 5.0f*uiS, y + 6.0f*uiS, w, h, 0.0f, 0.0f, 0.0f, 0.12f * a);
        drawRect(x, y, w, h, 0.038f, 0.054f, 0.078f, 0.38f * a);
        drawRect(x, y, w, h, uiR, uiG, uiB, 0.052f * a);
        drawCBkt(x, y, w, h, uiR, uiG, uiB, 0.26f * a);
    };

    // ── HEADER ────────────────────────────────────────────────────────────
    const float hSlide = (1.0f - wake) * 30.0f * uiS;
    const wchar_t* archiveTitle = (li == 0) ? L"도감"
        : (li == 1) ? L"ASTRAL LOG" : L"星界記録";
    BindMainShader();
    g_TextL.Draw(archiveTitle,
                 leftX, panelY + 4.0f * uiS - hSlide,
                 1.16f * uiS, 1.0f, 1.0f, 1.0f, 0.98f * wake);

    drawRect(leftX, panelY + headerH - 2.0f * uiS, panelW, 1.5f * uiS,
             uiR, uiG, uiB, 0.30f * wake);

    // Category tabs: same quiet strip language as RUN CONFIG / ARMORY.
    const float tabGp = 0.0f;
    const float tabW  = (leftW - tabGp * (float)(kCatCount - 1)) / (float)kCatCount;
    const float tabsX = leftX;
    const float tabsY = bodyY;

    for (int i = 0; i < kCatCount; ++i) {
        float tx = tabsX + (float)i * (tabW + tabGp);
        float ty = tabsY;
        bool hov = hit(tx, ty, tabW, tabH);
        s_catHover[i] = UiApproach(s_catHover[i], hov ? 1.0f : 0.0f, delta, 10.0f);
        s_catFocus[i] = UiApproach(s_catFocus[i], (i == s_cat) ? 1.0f : 0.0f, delta, 8.5f);
        if (hov && lmb && !g_LmbPrev && i != s_cat) s_cat = i;
        bool sel = (i == s_cat);
        const CatDef& cd = kCats[i];
        BindMainShader();
        if (sel || hov) {
            drawRect(tx + 8.0f * uiS, ty + tabH * 0.52f, tabW - 16.0f * uiS,
                     1.1f * uiS, cd.r, cd.g, cd.b,
                     (sel ? 0.070f : 0.035f + s_catHover[i] * 0.030f) * wake);
        }
        if (sel) {
            drawRect(tx, ty + tabH - 2.0f * uiS, tabW, 2.0f * uiS,
                     cd.r, cd.g, cd.b, 0.56f * wake);
            drawDiamond(tx + 15.0f * uiS, ty + tabH * 0.5f,
                        3.0f * uiS, cd.r, cd.g, cd.b, 0.72f * wake);
        }
        const wchar_t* tabLabel = kCats[i].name[nli];
        const float idSc = 0.56f * uiS;
        const float idY  = ty + (tabH - g_TextS.Height(tabLabel, idSc)) * 0.5f;
        drawCenterS(tabLabel, tx, idY, tabW, idSc,
                    sel ? inkR : idleR,
                    sel ? inkG : idleG,
                    sel ? inkB : idleB,
                    (sel ? 0.92f : 0.48f + s_catHover[i] * 0.20f) * wake);
    }

    // ── LEFT PANEL ────────────────────────────────────────────────────────
    float panelE = Smoothstep(s_panelT);

    BindMainShader();
    {
        const float masterX = leftX - 8.0f * uiS;
        const float masterY = bodyY - 6.0f * uiS;
        const float masterW = (rightX + rightW) - masterX;
        const float masterH = bodyH + 10.0f * uiS;
        drawRect(masterX + 12.0f*uiS, masterY + 13.0f*uiS, masterW, masterH,
                 0.0f, 0.0f, 0.0f, 0.035f * wake);
        drawConstellFrame(masterX, masterY, masterW, masterH,
                          uiR, uiG, uiB, 0.070f * panelE * wake,
                          24.0f * uiS, 2.4f * uiS, 0.010f * wake, 0.34f);
    }
    drawRect(leftX + 8.0f*uiS, listAreaY + 10.0f*uiS, leftW, listAreaH,
             0.0f, 0.0f, 0.0f, 0.030f * wake);
    drawRect(leftX, listAreaY, leftW, listAreaH,
             0.030f, 0.042f, 0.062f, 0.055f * wake);
    drawRect(leftX + 8.0f*uiS, listAreaY + 8.0f*uiS,
             leftW - 16.0f*uiS, listAreaH - 16.0f*uiS,
             0.052f, 0.072f, 0.098f, 0.050f * wake);
    drawRect(leftX + 2.0f * uiS, listAreaY, 1.2f * uiS, listAreaH,
             uiR, uiG, uiB, 0.14f * wake);
    if (wake > 0.02f) {
        BatchFlush(); SetGlowFx(true);
        drawConstellFrame(leftX, listAreaY, leftW, listAreaH,
                          uiR, uiG, uiB, 0.070f * panelE * wake,
                          18.0f*uiS, 2.4f*uiS, 0.010f * wake, 0.42f);
        BatchFlush(); SetGlowFx(false);
    }
    // List layout constants
    const float listX     = leftX + 6.0f * uiS;
    const float listW2    = leftW  - 12.0f * uiS;
    const float listTopY  = listAreaY + 8.0f * uiS;
    const float listBotY  = listAreaY + listAreaH - 8.0f * uiS;
    const float listViewH = listBotY - listTopY;
    const float itemH     = 58.0f * uiS;
    const float grpH      = 54.0f * uiS;

    // Build item list for current category
    struct ListItem {
        bool           isGroup;
        int            dataIdx;
        const wchar_t* label;
        bool           seen;
        float          r, g, b;
    };
    static ListItem s_items[512];
    int nItems = 0;
    float contentH = 0.0f;

    if (s_cat == 0) {
        struct MobGroup { const wchar_t* name; int ids[9]; int count; float r, g, b; };
        static const MobGroup MGRPS[] = {
            { L"ACTIVE SIGNALS",
              { CM_ROTOR, CM_SCOPE, CM_SWARM, CM_GENESIS, CM_GRAVIS, CM_QUASAR, 0, 0, 0 },
              6, 0.48f, 0.82f, 1.00f },
        };
        for (int gi = 0; gi < 1; ++gi) {
            const MobGroup& mg = MGRPS[gi];
            ListItem& lh = s_items[nItems++];
            lh.isGroup = true; lh.dataIdx = -1; lh.seen = true;
            lh.label = mg.name; lh.r = mg.r; lh.g = mg.g; lh.b = mg.b;
            contentH += grpH;
            for (int j = 0; j < mg.count; ++j) {
                int id = mg.ids[j];
                ListItem& litem = s_items[nItems++];
                litem.isGroup = false; litem.dataIdx = id;
                litem.seen = CodexMobSeen(id);
                litem.label = litem.seen ? MobName(id) : L"???";
                litem.r = mg.r; litem.g = mg.g; litem.b = mg.b;
                contentH += itemH;
            }
        }
    } else if (s_cat == 1) {
        int sorted[AUG_TOTAL], ns_aug = 0;
        for (int i = 0; i < AUG_TOTAL; ++i)
            if (!AugRemoved(ALL_AUGS[i].type)) sorted[ns_aug++] = i;
        std::sort(sorted, sorted + ns_aug,
                  [](int a, int b) { return AugTierIndexLess(a, b); });
        AugRarity prevR = (AugRarity)-1;
        for (int k = 0; k < ns_aug; ++k) {
            int i = sorted[k];
            AugRarity rar = ALL_AUGS[i].rarity;
            if (rar != prevR) {
                float rr, rg, rb; GetRarityColor(rar, rr, rg, rb);
                ListItem& lh = s_items[nItems++];
                lh.isGroup = true; lh.dataIdx = -1; lh.seen = true;
                lh.label = GetRarityKR(rar); lh.r = rr; lh.g = rg; lh.b = rb;
                prevR = rar; contentH += grpH;
            }
            float rr, rg, rb; GetRarityColor(ALL_AUGS[i].rarity, rr, rg, rb);
            ListItem& litem = s_items[nItems++];
            litem.isGroup = false; litem.dataIdx = i;
            litem.seen = CodexAugSeen(i);
            litem.label = litem.seen ? AugName(ALL_AUGS[i]) : L"???";
            litem.r = rr; litem.g = rg; litem.b = rb;
            contentH += itemH;
        }
    } else {
        ListItem& lh = s_items[nItems++];
        lh.isGroup = true; lh.dataIdx = -1; lh.seen = true;
        lh.label = L"DETECTED";
        lh.r = cat.r; lh.g = cat.g; lh.b = cat.b;
        contentH += grpH;
        for (int i = 0; i < BOSS_CODEX_COUNT; ++i) {
            ListItem& litem = s_items[nItems++];
            litem.isGroup = false; litem.dataIdx = i;
            litem.seen = BossCodexSeen(i);
            litem.label = litem.seen ? BossCodexName(i) : L"???";
            litem.r = uiR; litem.g = uiG; litem.b = uiB;
            contentH += itemH;
        }
    }

    // Scroll
    float& scroll = s_scroll[s_cat];
    float maxScroll = std::max(0.0f, contentH - listViewH);
    bool overList = (mx >= listX && mx <= listX + listW2 &&
                     my >= listTopY && my <= listBotY);
    if (overList && g_ScrollAccum != 0.0f)
        scroll -= g_ScrollAccum * itemH * 1.3f;
    g_ScrollAccum = 0.0f;
    scroll = std::max(0.0f, std::min(scroll, maxScroll));

    // Render list (scissored)
    BatchFlush();
    glEnable(GL_SCISSOR_TEST);
    glScissor((GLint)listX, (GLint)(sh - listBotY), (GLint)listW2, (GLint)listViewH);

    float ry = listTopY - scroll;
    for (int k = 0; k < nItems; ++k) {
        const ListItem& litem = s_items[k];
        float ih = litem.isGroup ? grpH : itemH;
        if (ry + ih < listTopY) { ry += ih; continue; }
        if (ry > listBotY)      break;

        if (litem.isGroup) {
            BindMainShader();
            const float gy = ry + 3.0f * uiS;
            const float gh = grpH - 6.0f * uiS;
            const float mid = gy + gh * 0.5f;
            drawRect(listX + 9.0f*uiS, mid - 0.5f*uiS,
                     listW2 - 28.0f*uiS, 1.0f*uiS,
                     uiR, uiG, uiB, 0.12f * wake);
            drawRect(listX + 4.0f*uiS, gy + 7.0f*uiS, 1.6f*uiS, gh - 14.0f*uiS,
                     litem.r, litem.g, litem.b, 0.48f * wake);
            drawDiamond(listX + 5.0f*uiS, mid, 3.4f*uiS,
                        litem.r, litem.g, litem.b, 0.62f * wake);
            const float groupSc = 0.50f * uiS;
            g_TextS.Draw(litem.label, listX + 22.0f*uiS,
                         gy + (gh - g_TextS.Height(litem.label, groupSc)) * 0.5f,
                         groupSc, 0.82f, 0.92f, 1.0f, 0.90f * wake);
        } else {
            bool isSel = (s_sel[s_cat] == litem.dataIdx);
            const float cardY = ry + 4.0f * uiS;
            const float cardH = itemH - 8.0f * uiS;
            bool hov2  = overList && hit(listX, cardY, listW2, cardH);
            if (hov2 && lmb && !g_LmbPrev && litem.seen)
                s_sel[s_cat] = litem.dataIdx;

            float ia = wake * (0.65f + 0.35f * panelE);
            BindMainShader();
            const float focus = isSel ? 1.0f : (hov2 ? 0.55f : 0.0f);
            if (isSel || hov2) {
                drawRect(listX + 22.0f * uiS, cardY + cardH * 0.5f,
                         listW2 - 30.0f * uiS, 1.1f * uiS,
                         uiR, uiG, uiB, (isSel ? 0.115f : 0.055f) * ia);
            }
            if (isSel) {
                drawRect(listX + 4.0f * uiS, cardY + 7.0f * uiS,
                         2.0f * uiS, cardH - 14.0f * uiS,
                         uiR, uiG, uiB, 0.74f * ia);
                g_TextS.Draw(L">", listX + 12.0f * uiS,
                             cardY + (cardH - g_TextS.Height(L">", 0.60f * uiS)) * 0.5f,
                             0.60f * uiS, uiR, uiG, uiB, 0.96f * ia);
            } else if (hov2) {
                drawRect(listX + 5.0f * uiS, cardY + 10.0f * uiS,
                         1.4f * uiS, cardH - 20.0f * uiS,
                         uiR, uiG, uiB, 0.34f * ia);
            }

            const float ndy = cardY + cardH * 0.5f;
            const float ndx = listX + 6.0f*uiS;
            drawDiamond(ndx, ndy, (isSel ? 4.4f : 3.0f)*uiS,
                        litem.r, litem.g, litem.b,
                        (isSel ? 0.92f : hov2 ? 0.62f : 0.32f) * ia);

            float itemSc = 0.57f * uiS;
            while (itemSc > 0.46f * uiS && g_TextS.Width(litem.label, itemSc) > listW2 - 54.0f * uiS)
                itemSc -= 0.025f * uiS;
            const float textY = cardY + (cardH - g_TextS.Height(litem.label, itemSc)) * 0.5f;
            g_TextS.Draw(litem.label, listX + (isSel ? 34.0f : 22.0f)*uiS, textY, itemSc,
                         litem.seen ? (isSel ? 1.0f : hov2 ? 0.94f : 0.68f) : 0.46f,
                         litem.seen ? (isSel ? 1.0f : hov2 ? 0.97f : 0.76f) : 0.48f,
                         litem.seen ? (isSel ? 1.0f : hov2 ? 1.00f : 0.86f) : 0.54f,
                         (litem.seen ? (isSel ? 0.98f : hov2 ? 0.86f : 0.64f) : 0.52f) * ia);
        }
        ry += ih;
    }

    BatchFlush();
    glDisable(GL_SCISSOR_TEST);

    // Scrollbar
    if (maxScroll > 0.0f) {
        BindMainShader();
        float tkX = panelX + leftW - 5.0f*uiS;
        drawRect(tkX, listTopY, 3.5f*uiS, listViewH,
                 0.10f, 0.10f, 0.14f, 0.50f * wake);
        float thumbH = listViewH * (listViewH / contentH);
        if (thumbH < 18.0f*uiS) thumbH = 18.0f*uiS;
        float thumbY = listTopY + (listViewH - thumbH) * (scroll / maxScroll);
        drawRect(tkX, thumbY, 3.5f*uiS, thumbH, uiR, uiG, uiB, 0.80f * wake);
    }

    // ── RIGHT PANEL ───────────────────────────────────────────────────────
    float detailA    = Smoothstep(s_detailFadeT) * rightWake;
    float entryShift = (1.0f - rightWake) * 70.0f * uiS;
    float rpx        = rightX + entryShift;

    BindMainShader();
    drawRect(rpx + 9.0f*uiS, bodyY + 11.0f*uiS, rightW, bodyH,
             0.0f, 0.0f, 0.0f, (0.045f + 0.035f*panelE)*rightWake);
    drawRect(rpx, bodyY, rightW, bodyH,
             0.026f, 0.038f, 0.058f, (0.26f + 0.08f*panelE)*rightWake);
    drawRect(rpx + 14.0f*uiS, bodyY + 14.0f*uiS,
             rightW - 28.0f*uiS, bodyH - 28.0f*uiS,
             0.050f, 0.070f, 0.096f, 0.070f * rightWake);
    float sweepW = rightW * 0.28f;
    float sweepX = rpx + fmodf(now * 180.0f, rightW + sweepW) - sweepW;
    drawRect(sweepX, bodyY + 1.0f*uiS, sweepW, 1.5f*uiS,
             uiR, uiG, uiB, 0.12f*panelE*rightWake);
    float scanY2 = bodyY + 18.0f*uiS + fmodf(now * 110.0f,
                   std::max(1.0f, bodyH - 30.0f*uiS));
    drawRect(rpx + 2.0f*uiS, scanY2, rightW - 4.0f*uiS, 1.0f*uiS,
             uiR, uiG, uiB, 0.040f*panelE*rightWake);
    if (rightWake > 0.02f) {
        BatchFlush(); SetGlowFx(true);
        drawConstellFrame(rpx, bodyY, rightW, bodyH,
                          uiR, uiG, uiB, 0.22f*panelE*rightWake,
                          24.0f*uiS, 5.5f*uiS, 0.040f * rightWake, 0.40f);
        BatchFlush(); SetGlowFx(false);
    }
    // Content area
    int selItem = s_sel[s_cat];
    const float cntX = rpx + 22.0f*uiS;
    const float cntY = bodyY + 34.0f*uiS;
    const float cntW = rightW - 44.0f*uiS;

    if (selItem < 0) {
        drawAstrolabeGrid(rpx, bodyY, rightW, bodyH, uiR, uiG, uiB,
                          panelE * rightWake);
    } else {
        // Layout: left viz panel (38%) | right info block (62%)
        const float cntH   = bodyH - 70.0f*uiS;
        const float vizW   = std::min(cntW * 0.46f, 560.0f * uiS);
        const float vizH   = cntH;
        const float vizX   = cntX;
        const float vizCX  = vizX + vizW * 0.5f;
        const float vizCY  = cntY + vizH * 0.5f;
        const float infoX  = cntX + vizW + 24.0f*uiS;
        const float infoW  = cntW - vizW - 24.0f*uiS;

        // Viz panel background
        BindMainShader();
        drawRect(vizX, cntY, vizW, vizH,
                 0.014f, 0.022f, 0.036f,
                 0.18f * detailA);
        drawRect(vizX + 8.0f*uiS, cntY + 8.0f*uiS, vizW - 16.0f*uiS, vizH - 16.0f*uiS,
                 uiR, uiG, uiB, 0.018f * detailA);
        drawCBkt(vizX, cntY, vizW, vizH, uiR, uiG, uiB, 0.40f * detailA);
        float vzScan = cntY + fmodf(now * 55.0f, std::max(1.0f, vizH));
        drawRect(vizX + 2.0f*uiS, vzScan, vizW - 4.0f*uiS, 1.0f*uiS,
                 uiR, uiG, uiB, 0.038f * detailA);

        if (s_cat == 0) {
            // Entity previews use the exact gameplay silhouettes (ROTOR,
            // GENESIS, SCOPE, SWARM and GRAVIS).
            const float previewScale = std::max(2.4f, std::min(vizW, vizH) / 72.0f);
            drawCodexMobPreview(selItem, vizCX, vizCY, previewScale);
        } else {
        // Rotating constellation node viewer
        {
            int nNodes = (s_cat == 0) ? 5 : (s_cat == 1) ? 6 : 4;
            float orbR  = std::min(vizW, vizH) * 0.36f;
            float spd   = (s_cat == 0) ? 0.28f : (s_cat == 1) ? 0.18f : 0.14f;
            float baseA = now * spd + (float)(selItem % 13) * 0.61f;
            float innerR = orbR * 0.42f;
            float baseA2 = -now * spd * 0.55f + (float)(selItem % 7) * 1.13f;

            float px[8], py[8];
            for (int ni = 0; ni < nNodes; ++ni) {
                float ang = baseA + (float)ni * (6.2832f / (float)nNodes);
                px[ni] = vizCX + cosf(ang) * orbR;
                py[ni] = vizCY + sinf(ang) * orbR;
            }
            // Outer ring edges (constellation dashes)
            BindMainShader();
            for (int ni = 0; ni < nNodes; ++ni) {
                int nj = (ni + 1) % nNodes;
                float dx2 = px[nj]-px[ni], dy2 = py[nj]-py[ni];
                float len2 = sqrtf(dx2*dx2+dy2*dy2);
                int nd = std::max(2,(int)(len2/(6.0f*uiS)));
                for (int d=1; d<nd; ++d) {
                    float t = (float)d/(float)nd;
                    drawDiamond(px[ni]+dx2*t, py[ni]+dy2*t, 1.1f*uiS,
                                uiR, uiG, uiB, 0.44f*detailA);
                }
            }
            // Center spokes (every other node)
            for (int ni = 0; ni < nNodes; ni += 2) {
                float dx2 = px[ni]-vizCX, dy2 = py[ni]-vizCY;
                float len2 = sqrtf(dx2*dx2+dy2*dy2);
                int nd = std::max(2,(int)(len2/(7.0f*uiS)));
                for (int d=1; d<nd; ++d) {
                    float t = (float)d/(float)nd;
                    drawDiamond(vizCX+dx2*t, vizCY+dy2*t, 0.9f*uiS,
                                uiR, uiG, uiB, 0.26f*detailA);
                }
            }
            // Inner counter-rotating ring
            int nInner = (s_cat == 1) ? 3 : 2;
            for (int ni = 0; ni < nInner; ++ni) {
                float ang = baseA2 + (float)ni*(6.2832f/(float)nInner);
                float ipx = vizCX + cosf(ang)*innerR;
                float ipy = vizCY + sinf(ang)*innerR;
                float dx2 = ipx-vizCX, dy2 = ipy-vizCY;
                float len2 = sqrtf(dx2*dx2+dy2*dy2);
                int nd = std::max(2,(int)(len2/(6.0f*uiS)));
                for (int d=1; d<nd; ++d) {
                    float t = (float)d/(float)nd;
                    drawDiamond(vizCX+dx2*t, vizCY+dy2*t, 1.0f*uiS,
                                uiR, uiG, uiB, 0.20f*detailA);
                }
                BindMainShader();
                drawDiamond(ipx, ipy, 2.5f*uiS, uiR, uiG, uiB, 0.55f*detailA);
            }
            // Outer node diamonds
            BindMainShader();
            for (int ni = 0; ni < nNodes; ++ni) {
                float nsz = (ni == 0) ? 5.0f : 3.5f;
                drawDiamond(px[ni], py[ni], nsz*uiS, uiR, uiG, uiB,
                            (ni==0 ? 0.90f : 0.68f)*detailA);
            }
            // Center node
            drawDiamond(vizCX, vizCY, 4.0f*uiS, uiR, uiG, uiB, 0.80f*detailA);
            drawDiamond(vizCX, vizCY, 2.0f*uiS, 1.0f, 1.0f, 1.0f, 0.42f*detailA);
        }
        }

        const float infoGap   = 18.0f * uiS;
        const float titleBoxY = cntY;
        const float titleBoxH = 104.0f * uiS;
        const float descBoxY  = titleBoxY + titleBoxH + infoGap;
        const float descBoxH  = 150.0f * uiS;
        const float logBoxY   = descBoxY + descBoxH + infoGap;
        const float logBoxH   = std::max(132.0f * uiS, cntY + cntH - logBoxY);

        if (s_cat == 0) {
            // ── Entity detail ─────────────────────────────────────────────
            bool seen = CodexMobSeen(selItem);
            BindMainShader();
            drawInfoQuad(infoX, titleBoxY, infoW, titleBoxH, detailA);
            g_TextS.Draw(L"ENTITY", infoX + 18.0f*uiS, titleBoxY + 10.0f*uiS,
                         0.44f*uiS, uiR, uiG, uiB, 0.78f*detailA);
            g_TextL.Draw(MobName(selItem), infoX + 18.0f*uiS, titleBoxY + 34.0f*uiS,
                         0.90f*uiS, 1.0f, 1.0f, 1.0f, 0.96f*detailA);
            drawRect(infoX + 18.0f*uiS, titleBoxY + titleBoxH - 16.0f*uiS, infoW - 36.0f*uiS, 1.5f*uiS,
                     uiR, uiG, uiB, 0.28f*detailA);
            drawInfoQuad(infoX, descBoxY, infoW, descBoxH, detailA);
            drawFitS(seen ? MobDesc(selItem) : L"???",
                     infoX + 18.0f*uiS, descBoxY + 24.0f*uiS, infoW - 36.0f*uiS,
                     0.60f*uiS, 0.44f*uiS, 0.86f, 0.92f, 1.0f, 0.92f*detailA);
            // ── 시스템 로그 블록 ──
            if (seen) {
                drawInfoQuad(infoX, logBoxY, infoW, logBoxH, detailA);
                const wchar_t* threatLv = MobThreatLabel(selItem);
                wchar_t pidBuf[16]; swprintf_s(pidBuf, L"0x%02X", (selItem * 17 + 0x40) & 0xFF);
                struct { const wchar_t* k; const wchar_t* v; } logR[] = {
                    { L"THREAT_LV  ", threatLv },
                    { L"PATTERN_ID ", pidBuf   },
                    { L"STATUS     ", L"CATALOGUED" },
                };
                for (int ll = 0; ll < 3; ++ll) {
                    float ly = logBoxY + 28.0f*uiS + ll * 38.0f * uiS;
                    g_TextS.Draw(logR[ll].k, infoX + 18.0f*uiS,              ly, 0.46f*uiS, uiR, uiG, uiB, 0.72f*detailA);
                    g_TextS.Draw(L": ",       infoX + 150.0f*uiS,            ly, 0.46f*uiS, uiR, uiG, uiB, 0.54f*detailA);
                    g_TextS.Draw(logR[ll].v,  infoX + 172.0f*uiS,            ly, 0.48f*uiS, 1.0f, 1.0f, 1.0f, 0.88f*detailA);
                }
            }

        } else if (s_cat == 1) {
            // ── Module detail ─────────────────────────────────────────────
            bool seen = (selItem >= 0 && selItem < AUG_TOTAL && CodexAugSeen(selItem));
            if (seen) {
                const AugDef& d = ALL_AUGS[selItem];
                float rr, rg, rb; GetRarityColor(d.rarity, rr, rg, rb);
                BindMainShader();
                drawInfoQuad(infoX, titleBoxY, infoW, titleBoxH, detailA);
                g_TextS.Draw(L"MODULE", infoX + 18.0f*uiS, titleBoxY + 10.0f*uiS,
                             0.44f*uiS, uiR, uiG, uiB, 0.78f*detailA);
                g_TextL.Draw(AugName(d), infoX + 18.0f*uiS, titleBoxY + 34.0f*uiS,
                             0.90f*uiS, 1.0f, 1.0f, 1.0f, 0.96f*detailA);
                drawRect(infoX + 18.0f*uiS, titleBoxY + titleBoxH - 16.0f*uiS, infoW - 36.0f*uiS, 1.5f*uiS,
                         uiR, uiG, uiB, 0.28f*detailA);
                drawInfoQuad(infoX, descBoxY, infoW, descBoxH, detailA);
                drawFitS(AugDesc(d), infoX + 18.0f*uiS, descBoxY + 24.0f*uiS, infoW - 36.0f*uiS,
                         0.60f*uiS, 0.44f*uiS, 0.86f, 0.92f, 1.0f, 0.92f*detailA);
                // ── 모듈 데이터 블록 ──
                {
                    drawInfoQuad(infoX, logBoxY, infoW, logBoxH, detailA);
                    wchar_t tidBuf[24]; swprintf_s(tidBuf, L"0x%03X", (int)d.type & 0xFFF);
                    struct { const wchar_t* k; const wchar_t* v; } mlogR[] = {
                        { L"MODULE_CLASS", GetRarityKR(d.rarity) },
                        { L"TYPE_ID     ", tidBuf },
                        { L"ACQUISITION ", L"IN-RUN PICK" },
                    };
                    for (int ml = 0; ml < 3; ++ml) {
                        float mly = logBoxY + 28.0f*uiS + ml * 38.0f*uiS;
                        g_TextS.Draw(mlogR[ml].k, infoX + 18.0f*uiS,  mly, 0.46f*uiS, uiR, uiG, uiB, 0.72f*detailA);
                        g_TextS.Draw(L": ", infoX + 164.0f*uiS,       mly, 0.46f*uiS, uiR, uiG, uiB, 0.54f*detailA);
                        g_TextS.Draw(mlogR[ml].v, infoX + 188.0f*uiS, mly, 0.48f*uiS, 1.0f, 1.0f, 1.0f, 0.88f*detailA);
                    }
                }
                if (d.rarity == AugRarity::COMBO) {
                    for (int ci = 0; ci < COMBO_COUNT; ++ci) {
                        if (COMBO_DEFS[ci].result != d.type) continue;
                        const ComboDef& cd2 = COMBO_DEFS[ci];
                        int ia = AugIndexOfType(cd2.reqs[0]);
                        int ib = AugIndexOfType(cd2.reqs[1]);
                        int ic = cd2.reqCount >= 3 ? AugIndexOfType(cd2.reqs[2]) : -1;
                        wchar_t rc[192];
                        if (cd2.reqCount >= 3)
                            swprintf_s(rc, L"RECIPE: %ls + %ls + %ls",
                                       ia>=0 ? AugName(ALL_AUGS[ia]) : L"?",
                                       ib>=0 ? AugName(ALL_AUGS[ib]) : L"?",
                                       ic>=0 ? AugName(ALL_AUGS[ic]) : L"?");
                        else
                            swprintf_s(rc, L"RECIPE: %ls + %ls",
                                       ia>=0 ? AugName(ALL_AUGS[ia]) : L"?",
                                       ib>=0 ? AugName(ALL_AUGS[ib]) : L"?");
                        BindMainShader();
                        drawFitS(rc, infoX + 18.0f*uiS, logBoxY + 150.0f*uiS, infoW - 36.0f*uiS,
                                 0.48f*uiS, 0.34f*uiS,
                                 0.82f, 0.94f, 1.0f, 0.90f*detailA);
                        break;
                    }
                }
            } else {
                BindMainShader();
                const wchar_t* q[2] = { L"\xBBF8\xBC1C\xACAC \xAE4C\xC9C0 \xBE44\xACF5\xAC1C",
                                         L"Undiscovered — unlock by acquiring" };
                drawInfoQuad(infoX, titleBoxY, infoW, titleBoxH, detailA);
                g_TextS.Draw(q[nli], infoX + 18.0f*uiS, titleBoxY + 32.0f*uiS,
                             0.60f*uiS, 0.62f, 0.66f, 0.76f, 0.88f*detailA);
            }
        } else {
            // ── Apex detail ───────────────────────────────────────────────
            bool seen = (selItem >= 0 && selItem < BOSS_CODEX_COUNT &&
                         BossCodexSeen(selItem));
            if (seen) {
                BindMainShader();
                drawInfoQuad(infoX, titleBoxY, infoW, titleBoxH, detailA);
                g_TextS.Draw(L"APEX", infoX + 18.0f*uiS, titleBoxY + 10.0f*uiS,
                             0.44f*uiS, uiR, uiG, uiB, 0.78f*detailA);
                g_TextL.Draw(BossCodexName(selItem), infoX + 18.0f*uiS, titleBoxY + 34.0f*uiS,
                             0.90f*uiS, 1.0f, 1.0f, 1.0f, 0.96f*detailA);
                drawRect(infoX + 18.0f*uiS, titleBoxY + titleBoxH - 16.0f*uiS, infoW - 36.0f*uiS, 1.5f*uiS,
                         uiR, uiG, uiB, 0.28f*detailA);
                drawInfoQuad(infoX, descBoxY, infoW, descBoxH, detailA);
                drawFitS(BossCodexDesc(selItem), infoX + 18.0f*uiS, descBoxY + 24.0f*uiS, infoW - 36.0f*uiS,
                         0.60f*uiS, 0.44f*uiS, 0.86f, 0.92f, 1.0f, 0.92f*detailA);
                // ── APEX 위협 데이터 ──
                {
                    drawInfoQuad(infoX, logBoxY, infoW, logBoxH, detailA);
                    wchar_t apidBuf[24]; swprintf_s(apidBuf, L"0x%02X", (selItem * 31 + 0x70) & 0xFF);
                    struct { const wchar_t* k; const wchar_t* v; } alogR[] = {
                        { L"CLASS      ", L"APEX / BOSS" },
                        { L"SIGNAL_ID  ", apidBuf },
                        { L"THREAT_LV  ", L"CRITICAL" },
                    };
                    for (int al = 0; al < 3; ++al) {
                        float aly = logBoxY + 28.0f*uiS + al * 38.0f*uiS;
                        g_TextS.Draw(alogR[al].k, infoX + 18.0f*uiS,  aly, 0.46f*uiS, uiR, uiG, uiB, 0.72f*detailA);
                        g_TextS.Draw(L": ", infoX + 150.0f*uiS,       aly, 0.46f*uiS, uiR, uiG, uiB, 0.54f*detailA);
                        g_TextS.Draw(alogR[al].v, infoX + 172.0f*uiS, aly, 0.48f*uiS, 1.0f, 1.0f, 1.0f, 0.88f*detailA);
                    }
                }
            } else {
                BindMainShader();
                const wchar_t* q[2] = {
                    L"\xBBF8\xBC1C\xACAC \x2014 \xBCF4\xC2A4 \xC870\xC6B0 \xC2DC \xACF5\xAC1C",
                    L"Undiscovered — encounter the boss" };
                drawInfoQuad(infoX, titleBoxY, infoW, titleBoxH, detailA);
                g_TextS.Draw(q[nli], infoX + 18.0f*uiS, titleBoxY + 32.0f*uiS,
                             0.60f*uiS, 0.62f, 0.66f, 0.76f, 0.88f*detailA);
            }
        }
    }

    // ── FOOTER: BACK button ───────────────────────────────────────────────
    float bw = 160.0f*uiS, bh = 40.0f*uiS;
    float bx = panelX, by = footY;
    bool backHov = hit(bx, by, bw, bh);
    BindMainShader();
    drawRect(bx, by, bw, bh,
             0.020f + uiR*(backHov ? 0.045f : 0.018f),
             0.030f + uiG*(backHov ? 0.035f : 0.014f),
             0.046f + uiB*(backHov ? 0.030f : 0.012f), 0.94f * wake);
    drawCBkt(bx, by, bw, bh, uiR, uiG, uiB, (backHov ? 0.68f : 0.28f) * wake);
    drawCenterS(T(StrId::BTN_BACK), bx, by + 10.0f*uiS, bw, 0.46f*uiS,
                backHov ? 1.0f : 0.78f, backHov ? 1.0f : 0.84f, backHov ? 1.0f : 0.96f,
                0.96f * wake);
    if (backHov && lmb && !g_LmbPrev) {
        CodexSearchClear();
        g_CodexSearchInputEnabled = false;
        g_GameManager.currentState = GameState::MAIN_MENU;
    }
}


void Scene_Tutorial(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    GLFWwindow* window = c.window;
    const GameState st = g_GameManager.currentState;
    (void)window;

    static int s_page = 0;
    static GameState s_prevSt = GameState::MAIN_MENU;
    if (st != s_prevSt) {
        if (st == GameState::TUTORIAL) s_page = 0;
        s_prevSt = st;
    }

    auto drawSlashText = [&](const wchar_t* text, float x, float y, float maxW,
                             float sc, float r, float g, float b, float a) -> float {
        if (!text || !text[0]) return y;
        std::vector<std::wstring> lines;
        std::wstring cur;
        for (const wchar_t* p = text; *p; ++p) {
            // A slash surrounded by spaces is a section separator.  Keep
            // compact rates such as "0.28/s" intact so the unit never wraps
            // onto a line by itself.
            if (*p == L'/' && (p == text || p[-1] == L' ' || p[1] == L' ')) {
                if (!cur.empty()) lines.push_back(cur);
                cur.clear();
            }
            else cur += *p;
        }
        if (!cur.empty()) lines.push_back(cur);
        for (auto& s : lines) {
            while (!s.empty() && s.front() == L' ') s.erase(0, 1);
            while (!s.empty() && s.back() == L' ') s.pop_back();
        }
        float lineH = 26.0f * sc;
        float cy = y;
        for (int li = 0; li < (int)lines.size(); li++) {
            float lsc = sc;
            while (lsc > 0.55f && g_TextS.Width(lines[li].c_str(), lsc) > maxW) lsc -= 0.04f;
            g_TextS.Draw(lines[li].c_str(), x, cy + li * lineH, lsc, r, g, b, a);
        }
        return y + (float)lines.size() * lineH;
    };

    const float WW = std::min(sw * 0.92f, 1180.0f);
    const float WH = std::min(sh * 0.88f, 880.0f);
    float wx, wy;
    DrawMenuBackground(sw, sh, c.delta);
    SceneAppWindow(sw, sh, WW, WH, TutorialWinTitle(), 0.35f, 0.85f, 1.0f, wx, wy, false, 0.0f);
    if (g_AppOpen < 0.999f) return;

    int li = LangIndexTutorial();
    const float sideW = 210.0f;
    const float pad = 24.0f;
    float sideX = wx + pad;
    float sideY = wy + 16.0f;
    float contentX = wx + sideW + pad * 2.0f;
    float contentW = WW - sideW - pad * 3.0f;
    float contentY = wy + 16.0f;

    BindMainShader();
    drawRect(sideX, sideY, sideW, WH - 120.0f, 0.10f, 0.12f, 0.16f, 0.95f);

    const wchar_t* SIDE[3] = { L"목차", L"Contents", L"目次" };
    g_TextS.Draw(SIDE[li], sideX + 14.0f, sideY + 10.0f, 0.82f, 0.55f, 0.85f, 1.0f, 0.95f);

    const float rowH = 44.0f;
    for (int i = 0; i < TUTORIAL_PAGE_COUNT; i++) {
        float ry = sideY + 38.0f + i * rowH;
        bool sel = (i == s_page);
        bool hov = (mx >= sideX + 6.0f && mx <= sideX + sideW - 6.0f &&
                    my >= ry && my <= ry + rowH - 4.0f);
        if (sel) drawRect(sideX + 6.0f, ry, sideW - 12.0f, rowH - 4.0f, 0.25f, 0.55f, 0.95f, 0.35f);
        else if (hov) drawRect(sideX + 6.0f, ry, sideW - 12.0f, rowH - 4.0f, 0.18f, 0.22f, 0.28f, 0.85f);
        const wchar_t* pt = TutorialPageTitle(i);
        g_TextS.Draw(pt, sideX + 16.0f, ry + 12.0f, 0.78f,
                     sel ? 0.95f : 0.82f, sel ? 0.98f : 0.88f, 1.0f, 1.0f);
        if (hov && lmb && !g_LmbPrev) s_page = i;
    }

    const wchar_t* pageTitle = TutorialPageTitle(s_page);
    g_TextL.Draw(pageTitle, contentX, contentY, 1.15f, 0.55f, 0.90f, 1.0f, 1.0f);
    float bodyY = contentY + 52.0f;
    bodyY = drawSlashText(TutorialPageBody(s_page), contentX, bodyY, contentW,
                          0.88f, 0.90f, 0.94f, 1.0f, 0.95f);

    const wchar_t* ctrl = TutorialControls();
    float ctrlW = g_TextS.Width(ctrl, 0.72f);
    g_TextS.Draw(ctrl, contentX + (contentW - ctrlW) * 0.5f, wy + WH - 108.0f, 0.72f,
                 0.50f, 0.80f, 1.0f, 0.90f);

    const wchar_t* PREV[3] = { L"< 이전", L"< Prev", L"< 前へ" };
    const wchar_t* NEXT[3] = { L"다음 >", L"Next >", L"次へ >" };
    float navY = wy + WH - 62.0f;
    if (s_page > 0 && UIButton(wx + pad, navY, 130.0f, 46.0f, PREV[li], mx, my, lmb, g_LmbPrev))
        s_page--;
    if (s_page < TUTORIAL_PAGE_COUNT - 1 &&
        UIButton(wx + pad + 140.0f, navY, 130.0f, 46.0f, NEXT[li], mx, my, lmb, g_LmbPrev))
        s_page++;

    wchar_t pg[32];
    swprintf_s(pg, L"%d / %d", s_page + 1, TUTORIAL_PAGE_COUNT);
    float pgW = g_TextS.Width(pg, 0.85f);
    g_TextS.Draw(pg, wx + WW * 0.5f - pgW * 0.5f, navY + 14.0f, 0.85f, 0.7f, 0.8f, 0.9f, 0.95f);

    {
        static bool s_lPrev = false, s_rPrev = false;
        bool lk = (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS);
        bool rk = (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS);
        if (lk && !s_lPrev && s_page > 0) s_page--;
        if (rk && !s_rPrev && s_page < TUTORIAL_PAGE_COUNT - 1) s_page++;
        s_lPrev = lk;
        s_rPrev = rk;
    }

    if (UIButton(wx + WW - pad - 180.0f, navY, 180.0f, 46.0f, T(StrId::BTN_BACK),
                 mx, my, lmb, g_LmbPrev))
        g_GameManager.currentState = GameState::MAIN_MENU;
}

// 직업 확정 → 고정 무기/조작 적용 → 시작증강 or READY 로 전이.
//   직업제 개편: 총기 3택1 화면 없이 직업 = 고정 무기 1:1 매핑.
void FinalizeLoadout(const SceneCtx& c, int wIdx) {
    const float sw = c.sw, sh = c.sh;
    float& fireTimer = *c.fireTimer;

    g_Difficulty = Difficulty::NORMAL;
    g_Stats.baseFireInterval = g_Stats.fireInterval;
    ApplyWeapon(g_Stats, (StartWeapon)wIdx);
    g_CurrentWeapon = wIdx;
    MarkStartWeaponOwnedType((StartWeapon)wIdx);
    fireTimer = g_Stats.fireInterval;
    if (g_CreativeMode) {
        for (int aidx : g_CreativeStartAugList) {
            if (aidx < 0 || aidx >= AUG_TOTAL) continue;
            AugType atype = ALL_AUGS[aidx].type;
            g_Stats.Apply(atype);
            g_OwnedAugs.push_back(aidx);
            g_TypeOwned[(int)atype] = true;
            MarkAugSeen(aidx);
            EquipSkill(SkillForAug(atype));
            if (AugOnceOnly(atype, ALL_AUGS[aidx].rarity))
                g_GameManager.takenOnce[aidx] = true;
            ApplyAugmentSideEffects(atype, (int)sw, (int)sh);
        }
        SyncPlayerBoundsAfterLoadout();
    }
    float trialHpMul = TrialPlayerMaxHpMult();
    if (trialHpMul < 0.999f)
        g_Stats.maxHP *= trialHpMul;
    g_GameManager.maxHP    = g_Stats.maxHP;
    g_GameManager.playerHP = g_Stats.maxHP;
    g_PrevHP               = g_Stats.maxHP;
    int startAugs = g_MetaStartAugs + ((g_CreativeMode) ? g_CreativeStartAugs : 0);
    if (startAugs > 0) {
        g_GameManager.ClearAugmentRewardQueue();
        for (int i = 0; i < startAugs; i++)
            g_GameManager.QueueAugmentReward(false, g_CreativeMode);
        g_BossRewardPicksLeft = 0;
        if (!g_GameManager.ActivateNextAugmentReward())
            g_GameManager.currentState = GameState::READY;
    } else {
        g_GameManager.currentState = GameState::READY;
    }
}

// 선택된 직업의 고정 무기 인덱스.  현재 플레이 가능한 직업은 소총과
// 전기장 두 가지이며, 예약 슬롯은 항상 기본 소총으로 폴백한다.
static int FixedWeaponForSelectedJob() {
    if (IsPlayableJob(g_SelectedJob) && g_SelectedJob != JOB_NONE &&
        JOB_DEFS[g_SelectedJob].fixedWeapon >= 0)
        return JOB_DEFS[g_SelectedJob].fixedWeapon;
    return (int)StartWeapon::RIFLE;
}


void Scene_CreativeConfig(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    GLFWwindow* window = c.window;
    const GameState st = g_GameManager.currentState;
    float& fireTimer = *c.fireTimer;
    const std::function<void()>& ResetForNewGame = c.reset;
                DrawMenuBackground(sw, sh, delta);
                BindMainShader();

                const float topY = 44.0f;
                const wchar_t* TIT = L"CREATIVE";
                g_TextL.Draw(TIT, (sw - g_TextL.Width(TIT, 1.4f)) * 0.5f,
                             topY, 1.4f, 0.85f, 0.95f, 0.6f, 1.0f);

                const float OBW = 130.0f, OBH = 44.0f, OBG = 10.0f;
                const float leftX = 40.0f;
                const float contentTop = topY + 52.0f;
                const float footY = sh - 64.0f;

                g_TextS.Draw(L"Start Score", leftX, contentTop, 0.95f, 1, 1, 1, 0.9f);
                struct ScoreOpt { const wchar_t* l; long long v; };
                ScoreOpt sOpts[5] = { {L"0",0},{L"200k",200000},{L"400k",400000},{L"500k",500000} };
                for (int i = 0; i < 4; i++) {
                    float ox = leftX + i * (OBW + OBG);
                    bool sel = (g_CreativeStartScore == sOpts[i].v);
                    if (UIButton(ox, contentTop + 28.0f, OBW, OBH, sOpts[i].l,
                                 mx, my, lmb, g_LmbPrev, sel))
                        g_CreativeStartScore = sOpts[i].v;
                }

                if (kBossEncountersEnabled) {
                g_TextS.Draw(L"Boss", leftX, contentTop + 90.0f, 1.0f, 1, 1, 1, 0.9f);
                g_TextS.Draw(L"(런 중 B = 선택 보스 즉시 스폰)", leftX, contentTop + 108.0f,
                             0.52f, 0.72f, 0.78f, 0.88f, 0.85f);
                g_TextS.Draw(L"* 일반 런 미포함 (개발용)", leftX, contentTop + 124.0f,
                             0.48f, 0.68f, 0.75f, 0.82f, 0.75f);
                struct BossOpt { const wchar_t* l; int v; };
                BossOpt bOpts[6] = {
                    {L"None",-1}, {L"LEVIATHAN",30}, {L"VOLLEY",2},
                    {L"TESSERACT",10}, {L"FORK",8}, {L"ETHER SWORD",20}
                };
                const float BBW = 112.0f;
                for (int i = 0; i < 6; i++) {
                    int col = i % 3, row = i / 3;
                    float ox = leftX + col * (BBW + OBG);
                    float oy = contentTop + 144.0f + row * (OBH + 8.0f);
                    bool sel = (g_CreativeBossPick == bOpts[i].v);
                    if (UIButton(ox, oy, BBW, OBH, bOpts[i].l,
                                 mx, my, lmb, g_LmbPrev, sel))
                        g_CreativeBossPick = bOpts[i].v;
                }
                } else {
                    g_CreativeBossPick = -1;
                }

                const float augTop = contentTop + (kBossEncountersEnabled ? 236.0f : 90.0f);
                g_TextS.Draw(L"Start Augments", leftX, augTop, 1.0f, 1, 1, 1, 0.9f);
                int aOpts[4] = { 0, 3, 5, 10 };
                for (int i = 0; i < 4; i++) {
                    float ox = leftX + i * (OBW + OBG);
                    wchar_t lb[8]; swprintf_s(lb, L"%d", aOpts[i]);
                    bool sel = (g_CreativeStartAugs == aOpts[i]);
                    if (UIButton(ox, augTop + 28.0f, OBW, OBH, lb,
                                 mx, my, lmb, g_LmbPrev, sel))
                        g_CreativeStartAugs = aOpts[i];
                }

                {
                    const float LIST_W = std::min(820.0f, sw * 0.46f);
                    float gx = sw - LIST_W - 40.0f;
                    if (gx < leftX + 440.0f) gx = leftX + 440.0f;
                    g_TextS.Draw(L"Pick Start Augments (click)", gx, contentTop, 0.95f, 1, 1, 1, 0.9f);
                    int avail[AUG_TOTAL], na = 0;
                    for (int i = 0; i < AUG_TOTAL; i++) {
                        if (AugRemoved(ALL_AUGS[i].type)) continue;
                        avail[na++] = i;
                    }
                    std::sort(avail, avail + na, [](int a, int b) {
                        return AugListIndexLess(a, b);
                    });
                    const float ROW_H = 24.0f, HDR_H = 22.0f;
                    float gTop = contentTop + 28.0f;
                    float gBottom = footY - 12.0f;
                    float viewH = gBottom - gTop;
                    int hdrCount = 0;
                    AugListGroup prevGrp = (AugListGroup)-1;
                    for (int k = 0; k < na; k++) {
                        AugListGroup g = AugListGroupOf(ALL_AUGS[avail[k]]);
                        if (g != prevGrp) { hdrCount++; prevGrp = g; }
                    }
                    float contentH = (float)na * ROW_H + (float)hdrCount * HDR_H;
                    static float s_caScroll = 0.0f;
                    bool over = (mx >= gx && mx <= gx + LIST_W && my >= gTop && my <= gBottom);
                    if (over && g_ScrollAccum != 0.0f) s_caScroll -= g_ScrollAccum * ROW_H * 1.5f;
                    g_ScrollAccum = 0.0f;
                    float maxS = (contentH > viewH) ? (contentH - viewH) : 0.0f;
                    if (s_caScroll < 0) s_caScroll = 0; if (s_caScroll > maxS) s_caScroll = maxS;
                    BatchFlush(); glEnable(GL_SCISSOR_TEST);
                    glScissor((GLint)gx, (GLint)(sh - gBottom), (GLint)(LIST_W + 4), (GLint)viewH);
                    prevGrp = (AugListGroup)-1;
                    float ry = gTop - s_caScroll;
                    for (int k = 0; k < na; k++) {
                        int i = avail[k];
                        AugListGroup grp = AugListGroupOf(ALL_AUGS[i]);
                        if (grp != prevGrp) {
                            if (ry >= gTop - HDR_H && ry <= gBottom)
                                g_TextS.Draw(AugListGroupLabel(grp), gx, ry, 0.72f,
                                             0.55f, 0.75f, 0.95f, 0.88f);
                            ry += HDR_H;
                            prevGrp = grp;
                        }
                        if (ry >= gTop - ROW_H && ry <= gBottom) {
                            int selPos = -1;
                            for (int s = 0; s < (int)g_CreativeStartAugList.size(); s++)
                                if (g_CreativeStartAugList[s] == i) { selPos = s; break; }
                            bool selected = (selPos >= 0);
                            bool hv = (over && my >= ry - 2.0f && my < ry + ROW_H - 4.0f);
                            float rr, rg, rb;
                            GetRarityColor(ALL_AUGS[i].rarity, rr, rg, rb);
                            if (selected || hv) {
                                BindMainShader();
                                drawRect(gx, ry - 2.0f, LIST_W, ROW_H,
                                         selected ? 0.18f : 0.12f,
                                         selected ? 0.22f : 0.14f,
                                         selected ? 0.30f : 0.18f, 0.75f);
                                if (selected)
                                    drawRect(gx, ry - 2.0f, 3.0f, ROW_H, rr, rg, rb, 1.0f);
                            }
                            wchar_t line[128];
                            swprintf_s(line, L"%ls [%ls] %ls",
                                       selected ? L"v" : L"-",
                                       GetAugBadge(ALL_AUGS[i]), AugName(ALL_AUGS[i]));
                            float rowSc = 0.76f;
                            while (rowSc > 0.60f &&
                                   g_TextS.Width(line, rowSc) > LIST_W - 8.0f) rowSc -= 0.03f;
                            g_TextS.Draw(line, gx + 4.0f, ry, rowSc, rr, rg, rb, selected ? 1.0f : 0.88f);
                            if (hv && lmb && !g_LmbPrev) {
                                if (selected) g_CreativeStartAugList.erase(
                                    g_CreativeStartAugList.begin() + selPos);
                                else          g_CreativeStartAugList.push_back(i);
                            }
                        }
                        ry += ROW_H;
                    }
                    BatchFlush(); glDisable(GL_SCISSOR_TEST);
                }

                {
                    static float s_creStartHov = 0.0f, s_creBackHov = 0.0f;
                    const float fnow = (float)glfwGetTime();
                    const float fdt  = std::min(delta, 0.05f);

                    const float SW = 280.0f, SH = 52.0f, SX = (sw - SW) * 0.5f;
                    const bool sHov = (mx >= SX - 12.0f && mx <= SX + SW + 12.0f &&
                                       my >= footY && my <= footY + SH);
                    s_creStartHov += ((sHov ? 1.0f : 0.0f) - s_creStartHov) * std::min(1.0f, fdt * 10.0f);
                    DrawUnifiedMenuCommand(L"START", nullptr, SX, footY, SW, SH,
                                          0.38f, 0.95f, 0.62f, 1.0f,
                                          s_creStartHov, false, 0.0f, fnow);
                    if (sHov && lmb && !g_LmbPrev) {
                        ResetForNewGame();
                        FinalizeLoadout(c, FixedWeaponForSelectedJob());
                    }

                    const float BW = 160.0f, BH = 48.0f, BX = 40.0f;
                    const bool bHov = (mx >= BX - 12.0f && mx <= BX + BW + 12.0f &&
                                       my >= footY && my <= footY + BH);
                    s_creBackHov += ((bHov ? 1.0f : 0.0f) - s_creBackHov) * std::min(1.0f, fdt * 10.0f);
                    DrawUnifiedMenuCommand(T(StrId::BTN_BACK), nullptr, BX, footY, BW, BH,
                                          0.48f, 0.82f, 1.0f, 1.0f,
                                          s_creBackHov, false, 0.0f, fnow + 0.17f);
                    if (bHov && lmb && !g_LmbPrev)
                        g_GameManager.currentState = GameState::MAIN_MENU;
                }
}

// PLAY 패널 — "STAR CHART": 무기 성좌(좌) + 굴레 궤도 띠(중) + 런 요약/LAUNCH(우).
// 모든 선택지는 별 노드로 표현하고, 글로우는 선택된 별에만 제한한다.
static void Scene_RunConfigInline(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh, dt = std::min(c.delta, 0.05f);
    const double mx = c.mx, my = c.my;
    const bool ko = LangIndex() == 0;
    auto PlayText = [&](const wchar_t* kr, const wchar_t* en) -> const wchar_t* {
        return ko ? kr : en;
    };
    const bool lmb = c.lmb;
    const bool lmbClick = lmb && !g_LmbPrev;

    static float entry = 0.0f;
    static float exitT = 0.0f;
    static bool exiting = false;
    static bool launchExit = false;
    static float launchT = 0.0f;
    static bool prevEsc = false, prevRmb = false;
    static int weapon = 0;
    static int trialFocus = 0;
    static int trialSelectedSlot = 0;
    static float trialDisplaySlot = 0.0f;
    static float trialTargetSlot = 0.0f;
    static int trialNavHoldDir = 0;
    static float trialNavHoldT = 0.0f;
    static float trialNavRepeatT = 0.0f;
    static bool trialDragging = false;
    static bool trialDragMoved = false;
    static float trialDragLastY = 0.0f;
    static float trialDragLastX = 0.0f;
    static int trialPressedId = -1;
    static float detailScroll = 0.0f;
    static float detailScrollTarget = 0.0f;
    static bool detailDragging = false;
    static bool detailDragMoved = false;
    static float detailDragLastY = 0.0f;
    static float trialHover[TRIAL_DEF_COUNT] = {};
    static float weaponHover[2] = {};
    static float backHover = 0.0f;
    static float playHover = 0.0f;
    static float resetHover = 0.0f;
    static float statT[6] = { 0.50f, 0.78f, 0.78f, 0.72f, 0.68f, 0.72f };
    static const float kStatTargets[2][6] = {
        { 0.50f, 0.78f, 0.78f, 0.72f, 0.68f, 0.72f },
        { 0.22f, 0.86f, 0.94f, 1.00f, 0.50f, 0.28f }
    };

    static constexpr int kTrialCatalogCount = TRIAL_DEF_COUNT - 1;
    static const int kTrialOrder[kTrialCatalogCount] = {
        // EARLY
        0, 1, 3, 4, 5, 8, 9, 10,
        // MID
        11, 13,
        // LATE
        6, 7, 14, 15, 16,
        // BOSS
        2, 17, 18, 19
    };
    static const wchar_t* kTrialTags[TRIAL_DEF_COUNT] = {
        L"SPEED UP  /  SCORE +15%", L"SURVIVAL DOWN  /  SCORE +15%",
        L"BOSS UP  /  SCORE +15%",  L"ECONOMY UP  /  SCORE +15%",
        L"BUILD DOWN  /  SCORE +15%",L"COOLING DOWN  /  SCORE +15%",
        L"DURABILITY UP  /  SCORE +15%",L"PRESSURE UP  /  SCORE +15%",
        L"EARLY  /  SCORE +14%",     L"EARLY  /  SCORE +16%",
        L"OPENING DOWN  /  SCORE +18%",L"MID  /  SCORE +18%",
        L"REMOVED",                    L"MID  /  SCORE +17%",
        L"LATE  /  SCORE +22%",       L"LATE  /  SCORE +24%",
        L"LATE  /  SCORE +21%",       L"BOSS  /  SCORE +24%",
        L"BOSS  /  SCORE +22%",       L"BOSS  /  SCORE +20%"
    };
    static const wchar_t* kTrialTagsKR[TRIAL_DEF_COUNT] = {
        L"속도 증가  /  점수 +15%", L"생존력 감소  /  점수 +15%",
        L"보스 강화  /  점수 +15%", L"경제 악화  /  점수 +15%",
        L"빌드 제약  /  점수 +15%", L"쿨다운 증가  /  점수 +15%",
        L"내구도 증가  /  점수 +15%", L"압박 증가  /  점수 +15%",
        L"초반  /  점수 +14%", L"초반  /  점수 +16%",
        L"시작 제약  /  점수 +18%", L"중반  /  점수 +18%",
        L"REMOVED",           L"중반  /  점수 +17%",
        L"후반  /  점수 +22%", L"후반  /  점수 +24%",
        L"후반  /  점수 +21%", L"보스  /  점수 +24%",
        L"보스  /  점수 +22%", L"보스  /  점수 +20%"
    };
    static const wchar_t* kTrialDetail[TRIAL_DEF_COUNT] = {
        L"Hostile movement speed is increased by 20 percent from the start of the run.",
        L"Your maximum HP is reduced by 25 percent. Recovery cannot restore the missing capacity.",
        L"Boss encounters gain 25 percent more HP, extending the final damage window.",
        L"All run shop prices are increased by 30 percent. Economy upgrades become a real trade-off.",
        L"Augment choices are limited to two cards whenever a selection is generated.",
        L"All skill cooldowns are increased by 25 percent. Timing becomes part of the build.",
        L"All enemies gain 30 percent HP, making sustained damage and target priority matter more.",
        L"Enemy speed and HP both rise by 20 percent. The whole arena becomes less forgiving.",
        L"Ranged enemies begin appearing earlier than normal, before the build is fully online.",
        L"The early normal spawn pressure is increased, compressing the opening economy window.",
        L"Starting maximum HP is reduced. The first few rooms become the cost of entry.",
        L"Midgame enemy pressure rises by 8 percent through spawn rate and HP.",
        L"This trial has been retired.",
        L"The midgame ranged enemy cap is raised, creating more simultaneous firing lanes.",
        L"The late-game spawn ramp is strengthened. Pressure keeps climbing after the build stabilizes.",
        L"Late enemies receive a stronger HP ramp, so damage scaling must keep pace.",
        L"Late ranged pressure is increased, reducing the value of standing still or holding one lane.",
        L"Boss HP is greatly increased. This is a deliberate long-fight score multiplier.",
        L"The spawn relief around bosses is reduced, so the arena remains dangerous during the encounter.",
        L"Boss warning time is shorter. Read the signal early or enter the encounter unprepared."
    };
    static const wchar_t* kTrialDetailKR[TRIAL_DEF_COUNT] = {
        L"런 시작부터 적의 이동 속도가 20퍼센트 증가합니다.",
        L"최대 체력이 25퍼센트 감소합니다. 회복으로 잃은 최대치를 되돌릴 수 없습니다.",
        L"보스의 체력이 25퍼센트 증가해 최종 피해 구간이 길어집니다.",
        L"모든 런 상점의 가격이 30퍼센트 증가합니다. 경제 강화의 선택이 더 까다로워집니다.",
        L"증강 선택이 생성될 때 선택지가 2장으로 제한됩니다.",
        L"모든 스킬의 쿨다운이 25퍼센트 증가합니다. 빌드에 타이밍 관리가 필요해집니다.",
        L"모든 적의 체력이 30퍼센트 증가해 지속 피해와 우선 처치가 중요해집니다.",
        L"적의 속도와 체력이 모두 20퍼센트 증가합니다. 전장이 전반적으로 더 가혹해집니다.",
        L"원거리 적이 평소보다 일찍 등장해 빌드가 완성되기 전부터 압박을 줍니다.",
        L"초반 일반 적의 스폰 압력이 증가해 초반 경제를 정비할 시간이 줄어듭니다.",
        L"시작 최대 체력이 감소합니다. 첫 방들이 입장 비용이 됩니다.",
        L"중반 적 스폰 속도와 체력이 8% 증가합니다.",
        L"현재 버전에서 제거된 시련입니다.",
        L"중반 원거리 적 수 제한이 증가해 동시에 유지해야 할 사선이 많아집니다.",
        L"후반 스폰 증가 폭이 커집니다. 빌드가 안정된 뒤에도 압박이 계속 상승합니다.",
        L"후반 적의 체력 증가 폭이 커지므로 피해량 성장도 발맞춰야 합니다.",
        L"후반 원거리 압박이 증가해 한 자리에 머물거나 한 방향만 지키기 어려워집니다.",
        L"보스의 체력이 크게 증가합니다. 장기전을 감수하고 점수 배율을 얻는 시련입니다.",
        L"보스 주변의 스폰 완화가 줄어들어 전투 중에도 전장이 위험하게 유지됩니다.",
        L"보스 경고 시간이 짧아집니다. 신호를 일찍 읽지 않으면 준비되지 않은 채 전투에 들어갑니다."
    };
    static const wchar_t* kStageLabels[4] = { L"EARLY", L"MID", L"LATE", L"BOSS" };
    static const wchar_t* kStageLabelsKR[4] = { L"초반", L"중반", L"후반", L"보스" };
    static const wchar_t* kTrialNamesKR[TRIAL_DEF_COUNT] = {
        L"오버클럭", L"메모리 누수", L"방화벽", L"부패한 드롭",
        L"프로세스 제한", L"낮은 대역폭", L"강화", L"급증",
        L"조기 돌입", L"패킷 폭풍", L"콜드 부트", L"중반 압박",
        L"REMOVED", L"프로세스 노이즈", L"후반 초과", L"강화 코어",
        L"신호 변위", L"방화벽 코어", L"소음 경기장", L"신호 손실"
    };
    static const wchar_t* kWeaponNames[2] = { L"RIFLE", L"FIELD" };
    static const wchar_t* kWeaponNamesKR[2] = { L"소총", L"전기장" };
    static const wchar_t* kWeaponSub[2] = {
        L"PRECISION / SINGLE TARGET", L"AREA CONTROL / SUSTAINED"
    };
    static const wchar_t* kWeaponSubKR[2] = {
        L"정밀 / 단일 대상", L"범위 제어 / 지속"
    };
    static const wchar_t* kStatNames[6] = {
        L"DAMAGE", L"FIRE RATE", L"INTERVAL", L"SPEED", L"SPREAD", L"RANGE"
    };
    static const wchar_t* kStatNamesKR[6] = {
        L"공격력", L"연사 속도", L"간격", L"탄속", L"퍼짐", L"사거리"
    };
    static const wchar_t* kStatValues[2][6] = {
        { L"50", L"5.0/s", L"0.20s", L"1200", L"0.04", L"900" },
        { L"18", L"CONT.", L"0.08s", L"320", L"0.15", L"180" }
    };
    static const wchar_t* kStatValuesKR[2][6] = {
        { L"50", L"5.0/s", L"0.20초", L"1200", L"0.04", L"900" },
        { L"18", L"연속", L"0.08초", L"320", L"0.15", L"180" }
    };
    auto trialName = [&](int id) -> const wchar_t* {
        return ko ? kTrialNamesKR[id] : TRIAL_DEFS[id].id;
    };
    auto trialCompact = [&](int id) -> const wchar_t* {
        return TRIAL_DEFS[id].desc[ko ? 0 : 1];
    };
    auto trialTag = [&](int id) -> const wchar_t* {
        return ko ? kTrialTagsKR[id] : kTrialTags[id];
    };
    auto trialDetail = [&](int id) -> const wchar_t* {
        return ko ? kTrialDetailKR[id] : kTrialDetail[id];
    };
    auto stageLabel = [&](int stage) -> const wchar_t* {
        return ko ? kStageLabelsKR[stage] : kStageLabels[stage];
    };
    auto weaponName = [&](int id) -> const wchar_t* {
        return ko ? kWeaponNamesKR[id] : kWeaponNames[id];
    };
    auto weaponSub = [&](int id) -> const wchar_t* {
        return ko ? kWeaponSubKR[id] : kWeaponSub[id];
    };
    auto statName = [&](int id) -> const wchar_t* {
        return ko ? kStatNamesKR[id] : kStatNames[id];
    };
    auto statValue = [&](int weaponId, int statId) -> const wchar_t* {
        return ko ? kStatValuesKR[weaponId][statId] : kStatValues[weaponId][statId];
    };

    auto countEnabled = [&]() {
        int count = 0;
        for (int i = 0; i < TRIAL_DEF_COUNT; ++i)
            if (i != TRIAL_REMOVED_ID && s_RcTrialEnabled[i]) ++count;
        return count;
    };
    auto stageIndex = [&](int defId) {
        switch (TrialStageForDef(defId)) {
        case TrialStage::EARLY: return 0;
        case TrialStage::MID:   return 1;
        case TrialStage::LATE:  return 2;
        case TrialStage::BOSS:  return 3;
        }
        return 0;
    };
    auto syncTrialsToGame = [&]() {
        int slot = 0;
        for (int id = 0; id < TRIAL_DEF_COUNT; ++id) {
            if (id == TRIAL_REMOVED_ID || !s_RcTrialEnabled[id]
                || slot >= TRIAL_SLOT_COUNT) continue;
            g_TrialPool[slot] = id;
            g_TrialSelected[slot] = true;
            ++slot;
        }
        for (; slot < TRIAL_SLOT_COUNT; ++slot)
            g_TrialSelected[slot] = false;
        g_TrialPoolReady = true;
    };

    if (entry <= 0.0f && !exiting) {
        weapon = std::max(0, std::min(1, s_RcWeapon));
        trialFocus = 0;
        trialSelectedSlot = 0;
        trialDisplaySlot = 0.0f;
        trialTargetSlot = 0.0f;
        trialNavHoldDir = 0;
        trialNavHoldT = 0.0f;
        trialNavRepeatT = 0.0f;
        trialDragging = false;
        trialDragMoved = false;
        trialDragLastY = 0.0f;
        trialDragLastX = 0.0f;
        trialPressedId = -1;
        detailScroll = 0.0f;
        detailScrollTarget = 0.0f;
        detailDragging = false;
        detailDragMoved = false;
        detailDragLastY = 0.0f;
        for (int i = 0; i < 2; ++i) weaponHover[i] = 0.0f;
        for (int i = 0; i < TRIAL_DEF_COUNT; ++i) trialHover[i] = 0.0f;
        backHover = playHover = 0.0f;
        resetHover = 0.0f;
        if (!s_RcTrialStateLoaded) {
            for (int i = 0; i < TRIAL_DEF_COUNT; ++i) s_RcTrialEnabled[i] = false;
            for (int slot = 0; slot < TRIAL_SLOT_COUNT; ++slot) {
                const int id = g_TrialPool[slot];
                if (g_TrialSelected[slot] && id >= 0 && id < TRIAL_DEF_COUNT
                    && id != TRIAL_REMOVED_ID)
                    s_RcTrialEnabled[id] = true;
            }
            s_RcTrialStateLoaded = true;
        }
        // A retired trial may still exist in an older saved selection.
        s_RcTrialEnabled[TRIAL_REMOVED_ID] = false;
        for (int id = 0; id < TRIAL_DEF_COUNT; ++id) {
            if (id != TRIAL_REMOVED_ID && s_RcTrialEnabled[id]) {
                trialFocus = id;
                break;
            }
        }
        for (int i = 0; i < kTrialCatalogCount; ++i) {
            if (kTrialOrder[i] == trialFocus) {
                trialSelectedSlot = i;
                break;
            }
        }
        trialDisplaySlot = trialTargetSlot = (float)trialSelectedSlot;
        launchExit = false;
        launchT = 0.0f;
    }

    entry = std::min(1.0f, entry + dt);
    if (launchExit) launchT = std::min(0.50f, launchT + dt);

    const bool esc = c.window && glfwGetKey(c.window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    const bool rmb = c.window && glfwGetMouseButton(c.window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    const bool backInput = (esc && !prevEsc) || (rmb && !prevRmb);
    prevEsc = esc;
    prevRmb = rmb;

    const float entryIn = Smoothstep(LogoClamp01((entry - 0.12f) / 0.42f));
    const float entryHeader = Smoothstep(LogoClamp01((entry - 0.04f) / 0.34f));
    if (backInput && !exiting && !launchExit && entry >= 0.55f)
        exiting = true;
    if (exiting) exitT = std::min(0.42f, exitT + dt);
    const float backP = exiting ? Smoothstep(LogoClamp01(exitT / 0.42f)) : 0.0f;
    const float launchP = Smoothstep(LogoClamp01((launchT - 0.28f) / 0.22f));
    const float contentA = entryIn * (1.0f - backP) * (1.0f - launchP);
    const bool ready = !exiting && !launchExit && entry >= 0.55f;

    SetSceneTextureReveal(exiting
        ? std::max(0.0f, 1.0f - backP)
        : Smoothstep(LogoClamp01((entry - 0.02f) / 0.72f)));

    const float uiS = std::max(0.70f,
        std::min(sw * 0.94f / 1640.0f, sh * 0.90f / 910.0f));
    const float pageL = std::max(42.0f, 72.0f * uiS);
    const float pageR = sw - std::max(42.0f, 72.0f * uiS);
    const float pageW = std::max(420.0f, pageR - pageL);
    const float headerY = std::max(30.0f, 48.0f * uiS);
    const float footerY = sh - std::max(62.0f, 84.0f * uiS);
    const float footerH = 42.0f * uiS;
    const float bodyTop = headerY + 76.0f * uiS;
    const float bodyBottom = footerY - 26.0f * uiS;
    const float detailW = std::min(286.0f * uiS, pageW * 0.23f);
    // PLAY uses a denser information layout than the other menu pages, so
    // keep its text comfortably above the small renderer's minimum scale and
    // give each catalogue row a clear title/effect/tag hierarchy.
    const float playMetaScale = 0.60f * uiS;
    const float playBodyScale = 0.70f * uiS;
    const float trialTitleScale = 0.68f * uiS;
    const float trialEffectScale = 0.62f * uiS;
    const float trialTagScale = 0.56f * uiS;
    const float trialListShiftLeft = 54.0f * uiS;
    const float trialListX = pageR - std::max(330.0f * uiS, pageW * 0.245f)
                           - trialListShiftLeft;
    const float trialListRight = pageR - 6.0f * uiS - trialListShiftLeft;
    const float centerL = pageL + detailW + 34.0f * uiS;
    const float centerR = trialListX - 38.0f * uiS;
    const float centerW = std::max(330.0f * uiS, centerR - centerL);
    const float chartCX = centerL + centerW * 0.50f;
    const float chartCY = bodyTop + (bodyBottom - bodyTop) * 0.44f;
    const float chartR = std::min(244.0f * uiS,
        std::min(centerW * 0.34f, (bodyBottom - bodyTop) * 0.29f));
    const float trialListW = std::max(230.0f * uiS, trialListRight - trialListX);
    // Keep all three bottom commands on one shared baseline. The trial reset
    // uses the catalogue's left X anchor, so it reads as the action belonging
    // to the list instead of another header control.
    const float routeX = pageL;
    const float routeW = std::min(188.0f * uiS, detailW * 0.88f);
    const float routeH = 58.0f * uiS;
    const float routeBottom = footerY + footerH * 0.18f;
    const float routeY = routeBottom - routeH;
    const float playW = 270.0f * uiS;
    const float playH = 62.0f * uiS;
    const float playX = chartCX - playW * 0.5f;
    const float playY = routeY - 1.0f * uiS;
    const wchar_t* resetLabel = PlayText(L"시련 초기화", L"TRIAL RESET");
    const float resetW = std::min(230.0f * uiS, trialListW);
    const float resetX = trialListRight - resetW;
    const float resetY = routeY;
    const float now = (float)glfwGetTime();
    const float weaponR = weapon == 0 ? 0.28f : 1.00f;
    const float weaponG = weapon == 0 ? 0.90f : 0.72f;
    const float weaponB = weapon == 0 ? 1.00f : 0.24f;
    const float trialR = 1.00f, trialG = 0.60f, trialB = 0.22f;
    const int activeCount = countEnabled();

    for (int i = 0; i < 6; ++i)
        statT[i] = UiApproach(statT[i], kStatTargets[weapon][i], dt, 8.0f);

    // The trial catalogue is a finite list on a restrained rail. The list
    // owns the full right column and keeps its first/last records visible.
    const float trialRowStep = 108.0f * uiS;
    // The visual row is shorter than the carousel step. Let adjacent rows'
    // hit regions meet at their centers' midpoint so there is no dead strip
    // between two trial entries.
    const float trialRowHitHalf = trialRowStep * 0.50f;
    const float trialViewTop = bodyTop + 58.0f * uiS;
    // Stop the catalogue above the shared bottom action rail. This leaves the
    // third command visibly owned by the list and prevents rows from entering
    // the BACK / PLAY / TRIAL RESET area.
    const float trialViewBottom = std::max(trialViewTop,
                                           resetY - 18.0f * uiS);
    const float trialViewH = std::max(0.0f, trialViewBottom - trialViewTop);
    const float trialViewCenterY = trialViewTop + trialViewH * 0.50f;
    const float trialDiagonal = std::min(72.0f * uiS, trialListW * 0.18f);
    const float trialRailTopX = trialListX - 4.0f * uiS;
    const float trialRailLeft = trialRailTopX - 30.0f * uiS;
    const int trialVisibleRadius = std::max(3,
        (int)(trialViewH / (trialRowStep * 2.0f)) + 2);
    auto trialRailXAt = [&](float y) {
        const float t = std::max(0.0f,
            std::min(1.0f, (y - trialViewTop) / std::max(1.0f, trialViewH)));
        return trialRailTopX + trialDiagonal * t;
    };
    // Keep the catalogue in a finite range.  The previous wrapped carousel
    // made the title/count feel inconsistent and allowed the user to scroll
    // forever through the same trials.
    auto normalizeTrialSlots = [&]() {
        const float last = (float)std::max(0, kTrialCatalogCount - 1);
        trialTargetSlot = std::max(0.0f, std::min(last, trialTargetSlot));
        trialDisplaySlot = std::max(0.0f, std::min(last, trialDisplaySlot));
    };
    const bool resetHov = ready
        && mx >= resetX - 22.0f * uiS
        && mx <= resetX + resetW + 22.0f * uiS
        && my >= resetY - 8.0f * uiS
        && my <= resetY + routeH + 10.0f * uiS;
    resetHover = UpdateMenuCommandHover(resetHover, resetHov, dt);
    if (resetHov && lmbClick) {
        for (int id = 0; id < TRIAL_DEF_COUNT; ++id)
            s_RcTrialEnabled[id] = false;
        trialFocus = 0;
        detailScroll = 0.0f;
        detailScrollTarget = 0.0f;
        detailDragging = false;
        detailDragMoved = false;
    }
    const float detailX = pageL + 8.0f * uiS;
    const float detailTop = bodyTop + 150.0f * uiS;
    const float detailViewTop = detailTop + 92.0f * uiS;
    // Leave a clear dead zone above the shared BACK/PLAY action rail. The
    // active-trial copy can scroll, but it must never appear underneath the
    // bottom commands.
    const float detailViewBottom = std::min(bodyBottom - 34.0f * uiS,
                                            footerY - 150.0f * uiS);
    const bool overTrialList = ready && mx >= trialRailLeft - 26.0f * uiS
        && mx <= trialListRight + 8.0f * uiS && my >= trialViewTop && my <= trialViewBottom;
    const bool overDetail = ready && mx >= detailX - 16.0f * uiS
        && mx <= detailX + detailW + 16.0f * uiS
        && my >= detailViewTop && my <= detailViewBottom;

    // The explanation column is a real scroll surface as well as a readable
    // list.  Keep the press/drag state separate from the trial catalogue so
    // grabbing the copy never toggles a trial by accident.
    if (lmbClick && overDetail && !trialDragging) {
        detailDragging = true;
        detailDragMoved = false;
        detailDragLastY = (float)my;
        detailScrollTarget = detailScroll;
    }
    if (!lmb && detailDragging) {
        detailDragging = false;
        detailDragMoved = false;
    }
    if (detailDragging && lmb) {
        const float dy = (float)my - detailDragLastY;
        const float dragThreshold = 3.5f * uiS;
        if (!detailDragMoved && fabsf(dy) >= dragThreshold)
            detailDragMoved = true;
        if (detailDragMoved) {
            detailScrollTarget -= dy;
            detailDragLastY = (float)my;
        }
    }

    const bool keyUpNow = c.window &&
        (glfwGetKey(c.window, GLFW_KEY_UP) == GLFW_PRESS ||
         glfwGetKey(c.window, GLFW_KEY_W) == GLFW_PRESS);
    const bool keyDownNow = c.window &&
        (glfwGetKey(c.window, GLFW_KEY_DOWN) == GLFW_PRESS ||
         glfwGetKey(c.window, GLFW_KEY_S) == GLFW_PRESS);
    int trialStepRequest = 0;
    if (!lmb && trialDragging) {
        // A press/release without a meaningful pointer movement is the
        // toggle gesture. Dragging the same row never reaches this branch.
        if (!trialDragMoved && trialPressedId >= 0
            && trialPressedId < TRIAL_DEF_COUNT) {
            const bool wasOn = s_RcTrialEnabled[trialPressedId];
            if (!wasOn && countEnabled() >= TRIAL_SLOT_COUNT) {
            } else {
                s_RcTrialEnabled[trialPressedId] = !wasOn;
                detailScroll = 0.0f;
                detailScrollTarget = 0.0f;
            }
        }
        trialDragging = false;
        trialDragMoved = false;
        trialPressedId = -1;
    }
    if (trialDragging && lmb) {
        const float dx = (float)mx - trialDragLastX;
        const float dy = (float)my - trialDragLastY;
        const float dragThreshold = 3.5f * uiS;
        if (!trialDragMoved && dx * dx + dy * dy
            >= dragThreshold * dragThreshold)
            trialDragMoved = true;
        if (trialDragMoved && fabsf(dy) > 0.0001f) {
            // Keep the press point until the gesture is confirmed. That way
            // the few pixels used to cross the threshold are not discarded,
            // which makes the rail feel responsive instead of sticky.
            const float dragDelta = -(dy / trialRowStep) * 1.12f;
            // Move the display directly with the pointer and clamp it to the
            // first/last catalogue entry.
            trialTargetSlot += dragDelta;
            trialDisplaySlot += dragDelta;
            normalizeTrialSlots();
            trialDragMoved = true;
            int nearestSlot = (int)std::round(trialTargetSlot);
            nearestSlot = std::max(0, std::min(kTrialCatalogCount - 1,
                                                nearestSlot));
            if (nearestSlot != trialSelectedSlot) {
                trialSelectedSlot = nearestSlot;
                trialFocus = kTrialOrder[trialSelectedSlot];
            }
            normalizeTrialSlots();
        }
        if (trialDragMoved) {
            trialDragLastY = (float)my;
            trialDragLastX = (float)mx;
        }
    }
    const int trialNavDir = keyUpNow == keyDownNow
        ? 0 : (keyUpNow ? -1 : 1);
    if (!trialDragging && ready && trialNavDir != 0) {
        if (trialNavDir != trialNavHoldDir) {
            trialNavHoldDir = trialNavDir;
            trialNavHoldT = 0.0f;
            trialNavRepeatT = 0.0f;
            trialStepRequest = trialNavDir;
        } else {
            trialNavHoldT += dt;
            if (trialNavHoldT >= 0.25f) {
                trialNavRepeatT += dt;
                const float repeatInterval = std::max(0.055f,
                    0.24f - (trialNavHoldT - 0.25f) * 0.070f);
                if (trialNavRepeatT >= repeatInterval) {
                    trialNavRepeatT = 0.0f;
                    trialStepRequest = trialNavDir;
                }
            }
        }
    } else {
        trialNavHoldDir = 0;
        trialNavHoldT = 0.0f;
        trialNavRepeatT = 0.0f;
    }
    if (g_ScrollAccum != 0.0f) {
        if (!trialDragging && overTrialList)
            trialStepRequest = g_ScrollAccum > 0.0f ? -1 : 1;
        else if (overDetail)
            detailScrollTarget -= g_ScrollAccum * 34.0f * uiS;
        g_ScrollAccum = 0.0f;
    }
    if (trialStepRequest != 0) {
        trialSelectedSlot = std::max(0, std::min(kTrialCatalogCount - 1,
                            trialSelectedSlot + trialStepRequest));
        trialTargetSlot += (float)trialStepRequest;
        trialFocus = kTrialOrder[trialSelectedSlot];
        normalizeTrialSlots();
    }
    {
        const float diff = trialTargetSlot - trialDisplaySlot;
        trialDisplaySlot += diff * std::min(1.0f, dt * 10.0f);
        if (fabsf(diff) < 0.002f)
            trialDisplaySlot = trialTargetSlot;
    }
    // PLAY shares the same wide, mirrored side fields as Settings. The
    // transparent scene remains visible through the center while the edges
    // receive a consistent dark contrast treatment.
    BatchFlush();
    DrawPersistentSceneSideVignettes(sw, sh,
                                     kWideSceneLinearAlpha * contentA);
    // The radial texture is the hero's light source: a colored outer corona
    // makes the constellation read as a focal object, while a small dark core
    // keeps the weapon title and node geometry crisp over bright backdrops.
    DrawRadialGradient(chartCX, chartCY, chartR * 1.88f,
                       weaponR * 0.56f, weaponG * 0.56f, weaponB * 0.56f,
                       0.34f * contentA);
    DrawRadialGradient(chartCX, chartCY, chartR * 1.08f,
                       0.0f, 0.0f, 0.008f,
                       0.26f * contentA);
    // CircleTexture carries the constellation's depth, while the broad edge
    // linear fields and hero radial fields provide the page contrast.
    DrawConstellationDisc(chartCX, chartCY, chartR * 1.74f,
                          0.0f, 0.0f, 0.012f, 0.28f * contentA);
    DrawConstellationDisc(chartCX, chartCY, chartR * 1.34f,
                          weaponR * 0.08f, weaponG * 0.08f, weaponB * 0.10f,
                          (0.10f + 0.025f * sinf(now * 1.2f)) * contentA, false);
    DrawConstellationDisc(trialListX + trialListW * 0.62f,
                          (trialViewTop + trialViewBottom) * 0.52f,
                          trialListW * 0.72f, 0.0f, 0.0f, 0.012f,
                          0.38f * contentA);

    DrawShadowedText(g_TextL, PlayText(L"플레이", L"PLAY"), pageL, headerY,
                     1.52f * uiS, 1.0f, 1.0f, 1.0f,
                     0.98f * entryHeader, 0.74f);
    const float escW = g_TextS.Width(L"[ESC]", playMetaScale);
    DrawShadowedText(g_TextS, L"[ESC]", pageR - escW, headerY + 8.0f * uiS,
                     playMetaScale, 0.68f, 0.78f, 0.88f,
                     0.72f * entryHeader, 0.54f);
    LogoLine(pageL, headerY + 58.0f * uiS, pageR, headerY + 58.0f * uiS,
             0.7f * uiS, weaponR, weaponG, weaponB, 0.12f * contentA);

    // Weapon choice is a small editorial tab in the upper-left. This is the
    // only weapon selection surface; the bottom rail is reserved for BACK.
    const float tabsY = bodyTop + 12.0f * uiS;
    const float tabScale = 0.86f * uiS;
    float tabX[2] = { detailX, detailX + 170.0f * uiS };
    for (int i = 0; i < 2; ++i) {
        const float tabW = g_TextL.Width(weaponName(i), tabScale);
        const bool hov = ready && mx >= tabX[i] - 8.0f * uiS
            && mx <= tabX[i] + tabW + 12.0f * uiS
            && my >= tabsY - 16.0f * uiS && my <= tabsY + 42.0f * uiS;
        weaponHover[i] = UpdateMenuCommandHover(weaponHover[i], hov, dt);
        const bool selected = weapon == i;
        DrawShadowedText(g_TextL, weaponName(i), tabX[i], tabsY,
                         tabScale,
                         selected ? weaponR : 0.58f,
                         selected ? weaponG : 0.68f,
                         selected ? weaponB : 0.78f,
                         (selected ? 0.96f : 0.64f + 0.18f * weaponHover[i]) * contentA,
                         0.50f);
        if (hov && lmbClick) { weapon = i; s_RcWeapon = weapon; }
    }
    LogoLine(detailX, tabsY + 48.0f * uiS, detailX + detailW,
             tabsY + 48.0f * uiS, 0.8f * uiS,
             weaponR, weaponG, weaponB, 0.18f * contentA);

    // Hero copy is attached to the constellation, keeping the selected
    // weapon readable before the player reads either side column.
    const float heroNameY = chartCY - chartR - 58.0f * uiS;
    const float heroNameScale = 0.98f * uiS;
    const float heroNameW = g_TextL.Width(weaponName(weapon), heroNameScale);
    const wchar_t* heroSystemLabel = PlayText(L"무기 별자리", L"WEAPON CONSTELLATION");
    DrawShadowedText(g_TextS, heroSystemLabel,
                     chartCX - g_TextS.Width(heroSystemLabel, playMetaScale) * 0.5f,
                     heroNameY - 32.0f * uiS, playMetaScale,
                     weaponR, weaponG, weaponB, 0.78f * contentA, 0.52f);
    DrawShadowedText(g_TextL, weaponName(weapon), chartCX - heroNameW * 0.5f,
                     heroNameY, heroNameScale,
                     1.0f, 1.0f, 1.0f, 0.96f * contentA, 0.72f);
    const float subW = g_TextS.Width(weaponSub(weapon), playMetaScale);
    DrawShadowedText(g_TextS, weaponSub(weapon), chartCX - subW * 0.5f,
                     heroNameY + 58.0f * uiS, playMetaScale,
                     weaponR, weaponG, weaponB, 0.76f * contentA, 0.50f);

    // A shallow perspective network gives the hero a dimensional read. The
    // nodes are depth-scaled and the ellipses rotate at different speeds, so
    // RIFLE and FIELD share the language but retain distinct silhouettes.
    auto drawHeroNetwork = [&]() {
        constexpr int kNodeCount = 18;
        float px[kNodeCount] = {}, py[kNodeCount] = {}, depth[kNodeCount] = {};
        const float spin = now * (weapon == 0 ? 0.045f : -0.032f);
        const float yScale = weapon == 0 ? 0.70f : 0.52f;
        for (int i = 0; i < kNodeCount; ++i) {
            const float a = spin + 6.2831853f * (float)i / (float)kNodeCount;
            const float wave = sinf(now * 0.34f + i * 1.73f);
            const float ring = 0.64f + 0.24f * (0.5f + 0.5f * sinf(i * 2.11f + 0.7f));
            depth[i] = wave;
            px[i] = chartCX + cosf(a) * chartR * ring;
            py[i] = chartCY + sinf(a) * chartR * ring * yScale
                   + wave * chartR * 0.055f;
        }
        for (int ring = 0; ring < 3; ++ring) {
            const float rx = chartR * (0.55f + ring * 0.18f);
            const float ry = chartR * yScale * (0.36f + ring * 0.16f);
            const float phase = spin * (ring == 1 ? -0.8f : 0.55f)
                              + (ring == 2 ? 0.22f : 0.0f);
            float lastX = chartCX + cosf(phase) * rx;
            float lastY = chartCY + sinf(phase) * ry;
            for (int n = 1; n <= 32; ++n) {
                const float a = phase + 6.2831853f * (float)n / 32.0f;
                const float x = chartCX + cosf(a) * rx;
                const float y = chartCY + sinf(a) * ry;
                if ((n + ring) % (ring == 2 ? 4 : 3) != 0)
                    DrawVisibleConstellLine(lastX, lastY, x, y,
                                            (0.52f + ring * 0.10f) * uiS,
                                            weaponR, weaponG, weaponB,
                                            (0.08f + ring * 0.025f) * contentA);
                lastX = x; lastY = y;
            }
        }
        for (int i = 0; i < kNodeCount; ++i) {
            const int next = (i + 1) % kNodeCount;
            const int cross = (i + 5 + weapon) % kNodeCount;
            const float edgeA = (0.15f + 0.12f * (depth[i] + 1.0f) * 0.5f) * contentA;
            DrawVisibleConstellLine(px[i], py[i], px[next], py[next],
                                    0.78f * uiS, weaponR, weaponG, weaponB, edgeA);
            if ((i + weapon) % 2 == 0)
                DrawVisibleConstellLine(px[i], py[i], px[cross], py[cross],
                                        0.56f * uiS, weaponR, weaponG, weaponB,
                                        edgeA * 0.62f);
            const float nodeSize = (2.3f + 1.7f * (depth[i] + 1.0f) * 0.5f) * uiS;
            if (depth[i] > 0.12f)
                DrawConstellationDisc(px[i], py[i], nodeSize * 2.2f,
                                      weaponR, weaponG, weaponB, 0.08f * contentA);
            DrawVisibleConstellNode(px[i], py[i], nodeSize,
                                    weaponR, weaponG, weaponB,
                                    (0.42f + 0.24f * (depth[i] + 1.0f) * 0.5f) * contentA,
                                    false, depth[i] > -0.35f);
        }
        // FIELD keeps its wider corona. RIFLE uses only the constellation
        // network; the former targeting spine read as unrelated stray lines.
        if (weapon != 0) {
            for (int i = 0; i < 8; ++i) {
                const float a = now * -0.08f + 6.2831853f * i / 8.0f;
                DrawVisibleConstellLine(chartCX, chartCY,
                                        chartCX + cosf(a) * chartR * 0.72f,
                                        chartCY + sinf(a) * chartR * 0.72f * yScale,
                                        0.62f * uiS, weaponR, weaponG, weaponB,
                                        0.18f * contentA);
            }
        }
        for (int id = 0; id < TRIAL_DEF_COUNT; ++id) {
            if (id == TRIAL_REMOVED_ID || !s_RcTrialEnabled[id]) continue;
            const int node = (id * 7 + weapon * 3) % kNodeCount;
            DrawVisibleConstellLine(chartCX, chartCY, px[node], py[node],
                                    1.10f * uiS, trialR, trialG, trialB,
                                    0.24f * contentA);
            DrawConstellationDisc(px[node], py[node], 13.0f * uiS,
                                  trialR, trialG, trialB, 0.10f * contentA);
            DrawVisibleConstellNode(px[node], py[node], 4.2f * uiS,
                                    trialR, trialG, trialB, 0.86f * contentA,
                                    true, true);
        }
        const float pulse = 0.5f + 0.5f * sinf(now * (weapon == 0 ? 5.6f : 3.0f));
        DrawConstellationDisc(chartCX, chartCY, (32.0f + pulse * 8.0f) * uiS,
                              0.0f, 0.0f, 0.012f, 0.36f * contentA);
        DrawConstellationDisc(chartCX, chartCY, (17.0f + pulse * 4.0f) * uiS,
                              weaponR, weaponG, weaponB, 0.18f * contentA, false);
        drawDiamond(chartCX, chartCY, (9.0f + pulse * 2.0f) * uiS,
                    weaponR, weaponG, weaponB, 0.86f * contentA);
        drawDiamond(chartCX, chartCY, 3.8f * uiS,
                    0.96f, 1.0f, 1.0f, 0.94f * contentA);
    };
    drawHeroNetwork();

    // Compact telemetry stays under the hero as a 3 x 2 readout. The larger
    // type makes the values scannable without creating a second information
    // panel beside the weapon hero.
    const float statY = chartCY + chartR + 26.0f * uiS;
    const float statRowStep = 60.0f * uiS;
    const float statColumnGap = 30.0f * uiS;
    const float statLabelScale = 0.72f * uiS;
    const float statValueScale = 0.76f * uiS;
    const float statW = std::max(96.0f * uiS,
        (centerW - statColumnGap * 2.0f) / 3.0f);
    for (int i = 0; i < 6; ++i) {
        const int col = i % 3;
        const int row = i / 3;
        const float x = centerL + col * (statW + statColumnGap);
        const float y = statY + row * statRowStep;
        const float valueW = g_TextS.Width(statValue(weapon, i), statValueScale);
        const float textInset = 4.0f * uiS;
        // The track follows the same left/right bounds as the metric text:
        // label starts at barX and the value ends at barRight.
        const float barX = x + textInset;
        const float barRight = x + statW - textInset;
        DrawShadowedText(g_TextS, statName(i), x + textInset, y,
                         statLabelScale, 0.72f, 0.82f, 0.92f,
                         0.78f * contentA, 0.46f);
        DrawShadowedText(g_TextS, statValue(weapon, i),
                         x + statW - valueW - textInset, y,
                         statValueScale, 0.92f, 0.96f, 1.0f,
                         0.90f * contentA, 0.52f);
        // Give the telemetry a little air: the track is inset and sits below
        // the labels instead of competing with the enlarged text.
        LogoLine(barX, y + 31.0f * uiS, barRight,
                 y + 31.0f * uiS, 0.95f * uiS,
                 0.34f, 0.44f, 0.56f, 0.22f * contentA);
        LogoLine(barX, y + 31.0f * uiS,
                 barX + (barRight - barX) * LogoClamp01(statT[i]),
                 y + 31.0f * uiS, 1.75f * uiS,
                 weaponR, weaponG, weaponB, 0.70f * contentA);
    }

    // Left active-trial readout: this is intentionally independent of the
    // hovered catalogue row. It reports every enabled trial, while the
    // catalogue remains free to browse without rewriting the explanation.
    DrawShadowedText(g_TextS,
                     activeCount > 0 ? PlayText(L"활성 시련", L"ACTIVE TRIALS")
                                     : PlayText(L"활성 시련 없음", L"NO ACTIVE TRIALS"),
                     detailX, detailTop, playBodyScale,
                     trialR, trialG, trialB, 0.82f * contentA, 0.52f);
    wchar_t activeReadout[32];
    swprintf_s(activeReadout, ko ? L"%d / %d 활성화" : L"%d / %d ENABLED",
               activeCount, TRIAL_SLOT_COUNT);
    const float activeReadoutScale = playMetaScale;
    DrawShadowedText(g_TextS, activeReadout,
                     detailX,
                     detailTop + 34.0f * uiS, activeReadoutScale,
                     0.64f, 0.74f, 0.84f, 0.72f * contentA, 0.42f);
    LogoLine(detailX, detailTop + 70.0f * uiS,
             detailX + detailW, detailTop + 70.0f * uiS,
             0.75f * uiS, trialR, trialG, trialB, 0.26f * contentA);
    const wchar_t* detailHint = PlayText(L"마우스로 잡고 이동  //  휠 스크롤",
                                         L"DRAG TO SCROLL  //  MOUSE WHEEL");
    float detailHintScale = 0.34f * uiS;
    while (detailHintScale > 0.26f * uiS
           && g_TextS.Width(detailHint, detailHintScale) > detailW)
        detailHintScale -= 0.02f * uiS;
    const float detailHintW = g_TextS.Width(detailHint, detailHintScale);
    DrawShadowedText(g_TextS, detailHint,
                     detailX + detailW - detailHintW,
                     detailViewTop - 12.0f * uiS, detailHintScale,
                     0.42f, 0.70f, 0.80f, 0.68f * contentA, 0.40f);
    // Give the active build an objective, compact readout before the prose
    // descriptions.  These values mirror the multipliers used by the run
    // code; conditional late-game effects are shown as start -> peak rather
    // than pretending they are active from the first room.
    struct ActiveSummaryRow {
        std::wstring label;
        std::wstring value;
        bool buff = false;
    };
    std::vector<ActiveSummaryRow> summaryRows;
    const auto selected = [&](int id) {
        return id >= 0 && id < TRIAL_DEF_COUNT && id != TRIAL_REMOVED_ID
            && s_RcTrialEnabled[id];
    };
    auto formatPercent = [](float mult) {
        wchar_t buf[32] = {};
        swprintf_s(buf, L"%+.1f%%", (mult - 1.0f) * 100.0f);
        return std::wstring(buf);
    };
    auto formatPercentRange = [&](float startMult, float peakMult) {
        if (fabsf(startMult - peakMult) < 0.0005f)
            return formatPercent(startMult);
        wchar_t buf[64] = {};
        swprintf_s(buf, L"%+.1f%% → %+.1f%%",
                   (startMult - 1.0f) * 100.0f,
                   (peakMult - 1.0f) * 100.0f);
        return std::wstring(buf);
    };
    auto formatCountRange = [&](int startCount, int peakCount) {
        wchar_t buf[32] = {};
        if (startCount == peakCount)
            swprintf_s(buf, L"+%d", startCount);
        else
            swprintf_s(buf, L"+%d → +%d", startCount, peakCount);
        return std::wstring(buf);
    };
    auto addPercentMetric = [&](const wchar_t* label, float startMult,
                                float peakMult, bool buff) {
        if (fabsf(startMult - 1.0f) < 0.0005f
            && fabsf(peakMult - 1.0f) < 0.0005f)
            return;
        summaryRows.push_back({ label,
                                formatPercentRange(startMult, peakMult),
                                buff });
    };

    float scoreMult = 1.0f;
    for (int id = 0; id < TRIAL_DEF_COUNT; ++id) {
        if (selected(id)) scoreMult += TrialScoreBonusForDef(id);
    }
    if (activeCount > 0) {
        wchar_t scoreBuf[48] = {};
        swprintf_s(scoreBuf, L"x%.2f  (%+.1f%%)", scoreMult,
                   (scoreMult - 1.0f) * 100.0f);
        summaryRows.push_back({ PlayText(L"점수 배율", L"SCORE MULTIPLIER"),
                                scoreBuf, true });
    }

    float playerHpMult = 1.0f;
    if (selected(10)) playerHpMult *= 0.82f;
    addPercentMetric(PlayText(L"플레이어 최대 HP", L"PLAYER MAX HP"),
                     1.0f, playerHpMult, false);

    float enemyHpStart = 1.0f, enemyHpPeak = 1.0f;
    if (selected(11)) enemyHpStart *= 1.08f;
    if (selected(15)) enemyHpPeak *= 1.28f;
    enemyHpPeak *= enemyHpStart;
    addPercentMetric(PlayText(L"일반 적 체력", L"ENEMY HP"),
                     enemyHpStart, enemyHpPeak, false);

    float spawnStart = 1.0f, spawnPeak = 1.0f;
    if (selected(9))  spawnStart *= 1.18f;
    if (selected(11)) spawnStart *= 1.08f;
    spawnPeak = spawnStart;
    if (selected(14)) spawnPeak *= 1.24f;
    if (selected(16)) spawnPeak *= 1.12f;
    addPercentMetric(PlayText(L"일반 스폰 빈도", L"SPAWN FREQUENCY"),
                     spawnStart, spawnPeak, false);

    float rangedStart = 1.0f, rangedPeak = 1.0f;
    if (selected(8))  rangedStart *= 0.78f;
    if (selected(18)) rangedStart *= 0.90f;
    rangedPeak = rangedStart;
    if (selected(13)) rangedPeak *= 0.86f;
    if (selected(16)) rangedPeak *= 0.82f;
    addPercentMetric(PlayText(L"원거리 스폰 간격", L"RANGED INTERVAL"),
                     rangedStart, rangedPeak, false);

    int rangedMaxStart = selected(18) ? 1 : 0;
    int rangedMaxPeak = rangedMaxStart
                      + (selected(13) ? 2 : 0)
                      + (selected(16) ? 2 : 0);
    if (rangedMaxStart > 0 || rangedMaxPeak > 0)
        summaryRows.push_back({ PlayText(L"원거리 최대 수", L"RANGED MAX"),
                                formatCountRange(rangedMaxStart, rangedMaxPeak),
                                false });

    float bossHpMult = 1.0f;
    if (selected(17)) bossHpMult *= 1.35f;
    if (selected(18)) bossHpMult *= 1.15f;
    addPercentMetric(PlayText(L"보스 체력", L"BOSS HP"),
                     bossHpMult, bossHpMult, false);

    if (selected(19))
        summaryRows.push_back({ PlayText(L"보스 경고 시간", L"BOSS WARNING"),
                                PlayText(L"-28.0%", L"-28.0%"), false });

    const float summaryTitleScale = 0.66f * uiS;
    const float summaryTitleH = 29.0f * uiS;
    const float summaryRowH = 32.0f * uiS;
    const float summaryBottomGap = 15.0f * uiS;
    const float summaryBlockH = summaryTitleH
                              + std::max<size_t>(1, summaryRows.size()) * summaryRowH
                              + summaryBottomGap;
    struct ActiveDetailLine {
        std::wstring text;
        int kind;
        int tone;
    };
    std::vector<ActiveDetailLine> detailLines;
    auto appendLine = [&](const std::wstring& text, int kind, int tone) {
        detailLines.push_back({ text, kind, tone });
    };
    auto appendWrapped = [&](const std::wstring& text, int kind, int tone) {
        std::wstring line;
        std::wstring word;
        for (size_t i = 0; i <= text.size(); ++i) {
            const bool end = i == text.size();
            const wchar_t ch = end ? L' ' : text[i];
            if (ch != L' ') { word.push_back(ch); continue; }
            std::wstring candidate = line.empty() ? word : line + L" " + word;
            if (!line.empty() && g_TextS.Width(candidate.c_str(), playBodyScale) > detailW) {
                appendLine(line, kind, tone);
                line = word;
            } else if (!word.empty()) {
                line = candidate;
            }
            word.clear();
        }
        if (!line.empty()) appendLine(line, kind, tone);
    };
    if (activeCount <= 0) {
        appendWrapped(PlayText(L"시련 목록에서 시련을 활성화해 런 설정을 구성하세요.",
                               L"Enable trials from the catalogue to build the run modifier set."),
                      0, 0);
    } else {
        for (int order = 0; order < kTrialCatalogCount; ++order) {
            const int id = kTrialOrder[order];
            if (!s_RcTrialEnabled[id]) continue;
            appendLine(std::wstring(trialName(id)), 1, 0);
            std::wstring stageLine = ko ? L"단계  //  " : L"STAGE  //  ";
            stageLine += stageLabel(stageIndex(id));
            appendLine(stageLine, 2, 0);
            appendLine(PlayText(L"효과", L"EFFECT"), 2, 0);
            appendWrapped(std::wstring(trialDetail(id)), 0, -1);
            appendLine(PlayText(L"간단 요약", L"COMPACT READOUT"), 2, 0);
            appendWrapped(std::wstring(trialCompact(id)), 0, -1);
            appendLine(PlayText(L"보상 / 압박", L"PAYOFF / PRESSURE"), 2, 0);
            appendWrapped(std::wstring(trialTag(id)), 0, 1);
            appendLine(L"", 3, 0);
        }
    }
    const float detailLineH = 31.0f * uiS;
    const float detailContentH = summaryBlockH + detailLines.size() * detailLineH;
    const float detailMaxScroll = std::max(0.0f,
        detailContentH - (detailViewBottom - detailViewTop) + 6.0f * uiS);
    detailScrollTarget = std::max(0.0f,
                                  std::min(detailScrollTarget, detailMaxScroll));
    const float detailScrollEase = std::min(1.0f, dt * 14.0f);
    detailScroll += (detailScrollTarget - detailScroll) * detailScrollEase;
    if (fabsf(detailScrollTarget - detailScroll) < 0.08f)
        detailScroll = detailScrollTarget;
    BatchFlush();
    glEnable(GL_SCISSOR_TEST);
    glScissor((GLint)(detailX - 6.0f * uiS),
              (GLint)(sh - detailViewBottom),
              (GLint)(detailW + 18.0f * uiS),
              (GLint)(detailViewBottom - detailViewTop));
    float detailY = detailViewTop - detailScroll;
    DrawShadowedText(g_TextS,
                     PlayText(L"현재 적용 합계", L"ACTIVE EFFECT TOTAL"),
                     detailX, detailY, summaryTitleScale,
                     0.68f, 0.88f, 0.96f, 0.86f * contentA, 0.50f);
    detailY += summaryTitleH;
    if (summaryRows.empty()) {
        DrawShadowedText(g_TextS,
                         PlayText(L"선택된 시련 없음", L"NO ACTIVE MODIFIERS"),
                         detailX, detailY, 0.58f * uiS,
                         0.72f, 0.78f, 0.84f, 0.72f * contentA, 0.46f);
        detailY += summaryRowH;
    } else {
        for (const ActiveSummaryRow& row : summaryRows) {
            float valueScale = 0.58f * uiS;
            const float maxValueW = detailW * 0.48f;
            while (valueScale > 0.42f * uiS
                   && g_TextS.Width(row.value.c_str(), valueScale) > maxValueW)
                valueScale -= 0.02f * uiS;
            const float valueW = g_TextS.Width(row.value.c_str(), valueScale);
            const float labelMaxW = std::max(20.0f * uiS,
                detailW - valueW - 11.0f * uiS);
            float labelScale = 0.56f * uiS;
            while (labelScale > 0.40f * uiS
                   && g_TextS.Width(row.label.c_str(), labelScale) > labelMaxW)
                labelScale -= 0.02f * uiS;
            DrawShadowedText(g_TextS, row.label.c_str(), detailX, detailY,
                             labelScale,
                             0.70f, 0.78f, 0.86f, 0.86f * contentA, 0.44f);
            DrawShadowedText(g_TextS, row.value.c_str(),
                             detailX + detailW - valueW, detailY,
                             valueScale,
                             row.buff ? 0.20f : 1.0f,
                             row.buff ? 0.92f : 0.28f,
                             row.buff ? 1.0f : 0.28f,
                             0.94f * contentA, 0.50f);
            LogoLine(detailX, detailY + 19.0f * uiS,
                     detailX + detailW, detailY + 19.0f * uiS,
                     0.45f * uiS, 0.18f, 0.28f, 0.34f,
                     0.28f * contentA);
            detailY += summaryRowH;
        }
    }
    detailY += summaryBottomGap;
    for (size_t i = 0; i < detailLines.size(); ++i) {
        const ActiveDetailLine& detailLine = detailLines[i];
        if (detailLine.kind == 3 && detailLine.text.empty()) {
            detailY += detailLineH;
            continue;
        }
        const bool title = detailLine.kind == 1;
        const bool heading = detailLine.kind == 2;
        const bool buff = detailLine.tone > 0;
        const bool nerf = detailLine.tone < 0;
        DrawShadowedText(g_TextS, detailLine.text.c_str(), detailX, detailY,
                         title ? 0.68f * uiS : heading ? playMetaScale : playBodyScale,
                         title ? 1.0f : heading ? trialR : buff ? 0.20f : nerf ? 1.0f : 0.80f,
                         title ? 1.0f : heading ? trialG : buff ? 0.92f : nerf ? 0.28f : 0.87f,
                         title ? 1.0f : heading ? trialB : buff ? 1.0f : nerf ? 0.28f : 0.94f,
                         (title ? 0.92f : heading ? 0.72f : 0.86f) * contentA,
                         title ? 0.64f : heading ? 0.50f : 0.54f);
        detailY += detailLineH;
    }
    BatchFlush();
    glDisable(GL_SCISSOR_TEST);
    if (detailMaxScroll > 0.0f) {
        const float barX = detailX + detailW + 5.0f * uiS;
        const float thumbH = std::max(24.0f * uiS,
            (detailViewBottom - detailViewTop)
            * ((detailViewBottom - detailViewTop) / detailContentH));
        const float thumbY = detailViewTop
            + (detailViewBottom - detailViewTop - thumbH)
            * (detailScroll / detailMaxScroll);
        drawRect(barX, detailViewTop, 2.0f * uiS,
                 detailViewBottom - detailViewTop,
                 0.10f, 0.16f, 0.20f, 0.38f * contentA);
        drawRect(barX, thumbY, 2.0f * uiS, thumbH,
                 trialR, trialG, trialB, 0.66f * contentA);
    }

    // Right codex list: compact rows communicate the decision; the full
    // sentence is reserved for the scrollable detail column on the left.
    DrawShadowedText(g_TextS, PlayText(L"시련 목록", L"TRIAL CATALOGUE"), trialListX, bodyTop,
                     playBodyScale, trialR, trialG, trialB,
                     1.0f * contentA, 0.56f);
    wchar_t activeLabel[32];
    swprintf_s(activeLabel, ko ? L"%d / %d 활성" : L"%d / %d ACTIVE",
               activeCount, TRIAL_SLOT_COUNT);
    const float activeW = g_TextS.Width(activeLabel, 0.76f * uiS);
    DrawShadowedText(g_TextL, activeLabel, trialListRight - activeW,
                     bodyTop - 7.0f * uiS, 0.76f * uiS,
                     trialR, trialG, trialB, 1.0f * contentA, 0.66f);
    LogoLine(trialListX, bodyTop + 35.0f * uiS,
             trialListRight, bodyTop + 35.0f * uiS,
             0.8f * uiS, trialR, trialG, trialB, 0.30f * contentA);
    // A quiet cyan linear field follows the catalogue's \ rail. The slanted
    // texture keeps the list's motion language intact without becoming a
    // heavy card behind the readable rows.
    BatchFlush();
    const float trialFieldX = trialListX - 16.0f * uiS;
    const float trialFieldTop = trialViewTop - 14.0f * uiS;
    const float trialFieldW = trialListW + 32.0f * uiS;
    const float trialFieldH = trialViewBottom - trialViewTop + 28.0f * uiS;
    DrawLinearGradientRibbon(trialFieldX, trialFieldTop,
                             trialFieldW, trialFieldH,
                             std::min((trialDiagonal + 18.0f * uiS),
                                      trialFieldW * 0.18f),
                             0.06f, 0.48f, 0.62f,
                             0.075f * contentA, false);
    // The catalogue is a finite diagonal rail. Visible slots stay evenly
    // spaced and stop at the first/last real definition.
    const float railTopY = trialViewTop + 6.0f * uiS;
    const float railBottomY = trialViewBottom - 6.0f * uiS;
    const float railBottomX = trialRailTopX + trialDiagonal;
    LogoLine(trialRailTopX, railTopY, railBottomX, railBottomY,
             0.78f * uiS, weaponR, weaponG, weaponB,
             0.15f * contentA);
    LogoLine(trialRailTopX + 13.0f * uiS, railTopY,
             railBottomX + 13.0f * uiS, railBottomY,
             0.42f * uiS, weaponR, weaponG, weaponB,
             0.065f * contentA);
    DrawVisibleConstellNode(trialRailTopX, railTopY, 2.2f * uiS,
                            weaponR, weaponG, weaponB, 0.46f * contentA,
                            false, false);
    DrawVisibleConstellNode(railBottomX, railBottomY, 2.2f * uiS,
                            weaponR, weaponG, weaponB, 0.46f * contentA,
                            false, false);

    BatchFlush();
    glEnable(GL_SCISSOR_TEST);
    glScissor((GLint)(trialRailLeft - 26.0f * uiS),
              (GLint)(sh - trialViewBottom),
              (GLint)(trialListRight - trialRailLeft + 44.0f * uiS),
              (GLint)trialViewH);
    for (int slot = 0; slot < kTrialCatalogCount; ++slot) {
        float relF = (float)slot - trialDisplaySlot;
        const int rel = (int)std::round(relF);
        if (rel < -trialVisibleRadius || rel > trialVisibleRadius) continue;
        const int id = kTrialOrder[slot];
        const float rowCenter = trialViewCenterY + relF * trialRowStep;
        const float rowY = rowCenter - 43.0f * uiS;
        if (rowCenter + trialRowHitHalf < trialViewTop - 8.0f * uiS
            || rowCenter - trialRowHitHalf > trialViewBottom + 8.0f * uiS) {
            trialHover[id] = UpdateMenuCommandHover(trialHover[id], false, dt);
            continue;
        }
        const float rowX = trialRailXAt(rowCenter);
        const bool on = s_RcTrialEnabled[id];
        const bool rowHov = ready && overTrialList
            && mx >= rowX - 34.0f * uiS
            && mx <= trialListRight + 8.0f * uiS
            && my >= rowCenter - trialRowHitHalf
            && my <= rowCenter + trialRowHitHalf;
        trialHover[id] = UpdateMenuCommandHover(trialHover[id], rowHov, dt);
        if (rowHov && trialFocus != id) {
            trialFocus = id;
        }
        const float hover = trialHover[id];
        // PLAY keeps each trial anchored to the diagonal rail. Hover nudges
        // the readable row to the right, never across the rail into the hero.
        // An enabled trial intentionally keeps that same visual hover state
        // even when the pointer leaves the catalogue, so the active build is
        // scannable at a glance.
        const float visualHover = std::max(hover, on ? 1.0f : 0.0f);
        const float itemX = rowX + 22.0f * visualHover;
        const float rowA = (on ? 0.84f : 0.68f) + 0.16f * visualHover;
        if (visualHover > 0.02f) {
            const float hoverA = visualHover;
            const float hoverR = on ? trialR : weaponR;
            const float hoverG = on ? trialG : weaponG;
            const float hoverB = on ? trialB : weaponB;
            DrawConstellationDisc(itemX - 2.0f * uiS,
                                  rowY + 39.0f * uiS,
                                  (13.0f + 8.0f * hoverA) * uiS,
                                  hoverR, hoverG, hoverB,
                                  0.07f * hoverA * contentA);
        }
        if (visualHover > 0.02f)
            DrawVisibleConstellLine(rowX, rowCenter,
                                    itemX + 5.0f * uiS, rowCenter,
                                    0.58f * uiS,
                                    on ? trialR : weaponR, on ? trialG : weaponG,
                                    on ? trialB : weaponB,
                                    (0.10f + 0.24f * visualHover) * contentA);
        DrawVisibleConstellNode(itemX + 5.0f * uiS, rowY + 32.0f * uiS,
                                (on ? 4.2f : 3.0f + visualHover) * uiS,
                                on ? trialR : weaponR, on ? trialG : weaponG,
                                on ? trialB : weaponB,
                                rowA * contentA, on, on);
        const float textLift = std::min(1.0f, visualHover * 1.25f);
        const float titleR = on ? 1.0f : 0.94f + 0.06f * textLift;
        const float titleG = on ? 0.78f : 0.97f + 0.03f * textLift;
        const float titleB = on ? 0.42f : 1.0f;
        const float titleMaxW = std::max(2.0f,
            trialListRight - (itemX + 20.0f * uiS) - 16.0f * uiS);
        float titleScale = trialTitleScale;
        while (titleScale > 0.58f * uiS
               && g_TextS.Width(trialName(id), titleScale) > titleMaxW)
            titleScale -= 0.025f * uiS;
        DrawShadowedText(g_TextS, trialName(id),
                         itemX + 20.0f * uiS, rowY + 1.0f * uiS,
                         titleScale, titleR, titleG, titleB,
                         ((on ? 1.0f : 0.94f) + 0.06f * visualHover) * contentA,
                         0.56f);
        // The effect line is the challenge/debuff, so keep it red even when
        // the row is not selected.  The score payoff below is the player
        // benefit and is always cyan for quick scanning.
        const float compactR = 1.0f;
        const float compactG = 0.28f + 0.08f * textLift;
        const float compactB = 0.28f + 0.08f * textLift;
        DrawShadowedText(g_TextS, trialCompact(id),
                         itemX + 20.0f * uiS, rowY + 30.0f * uiS,
                         trialEffectScale, compactR, compactG, compactB,
                         ((on ? 1.0f : 0.90f) + 0.10f * visualHover) * contentA,
                         0.46f);
        const float tagY = rowY + 60.0f * uiS;
        const float tagAlpha = ((on ? 1.0f : 0.88f) + 0.12f * visualHover)
                             * contentA;
        std::wstring stageText = std::wstring(stageLabel(stageIndex(id)))
                                + L"  //";
        DrawShadowedText(g_TextS, stageText.c_str(),
                         itemX + 20.0f * uiS, tagY, trialTagScale,
                         0.66f, 0.76f, 0.84f, tagAlpha, 0.42f);
        wchar_t scoreBuf[32] = {};
        const int scorePct = (int)(TrialScoreBonusForDef(id) * 100.0f + 0.5f);
        swprintf_s(scoreBuf, ko ? L"점수 +%d%%" : L"SCORE +%d%%", scorePct);
        const float stageW = g_TextS.Width(stageText.c_str(), trialTagScale);
        DrawShadowedText(g_TextS, scoreBuf,
                         itemX + 20.0f * uiS + stageW + 9.0f * uiS,
                         tagY, trialTagScale,
                         0.20f, 0.92f, 1.0f, tagAlpha, 0.46f);
        LogoLine(itemX, rowY + 88.0f * uiS,
                 trialListRight, rowY + 88.0f * uiS,
                 0.55f * uiS, trialR, trialG, trialB,
                 (on ? 0.20f : 0.10f) * contentA);
        if (rowHov && lmbClick) {
            // The whole row is one gesture surface. Release without movement
            // toggles this trial; movement turns the same press into a drag.
            trialDragging = true;
            trialDragMoved = false;
            trialPressedId = id;
            trialDragLastY = (float)my;
            trialDragLastX = (float)mx;
            trialFocus = id;
            detailScroll = 0.0f;
        }
    }
    BatchFlush();
    glDisable(GL_SCISSOR_TEST);

    // Shared bottom rail: BACK | PLAY | TRIAL RESET. Each command keeps its
    // own column anchor, while the third command is aligned with the trial
    // catalogue above it.
    const float routeHitX = routeX - 22.0f * uiS;
    const float routeHitW = routeW + 44.0f * uiS;
    const bool backHov = ready && mx >= routeHitX && mx <= routeHitX + routeHitW
        && my >= routeY && my <= routeY + routeH;
    const bool playHov = ready && mx >= playX && mx <= playX + playW
        && my >= playY - 12.0f * uiS && my <= playY + playH + 10.0f * uiS;
    backHover = UpdateMenuCommandHover(backHover, backHov, dt);
    playHover = UpdateMenuCommandHover(playHover, playHov, dt);
    DrawUnifiedMenuCommand(PlayText(L"뒤로", L"BACK"), PlayText(L"돌아가기", L"RETURN"),
                           routeX, routeY, routeW, routeH,
                           0.48f, 0.82f, 1.0f, contentA,
                           backHover, false, 0.0f, now,
                           0.92f, 0.58f, true, true, true);
    DrawUnifiedMenuCommand(PlayText(L"플레이", L"PLAY"), nullptr,
                           playX, playY, playW, playH,
                           weaponR, weaponG, weaponB, contentA,
                           playHover, true,
                           0.12f + 0.10f * sinf(now * 2.2f), now + 0.51f,
                           0.92f, 0.58f, true, true, true, true);
    DrawUnifiedMenuCommand(resetLabel, PlayText(L"활성 시련 해제", L"CLEAR ACTIVE"),
                           resetX, resetY, resetW, routeH,
                           trialR, trialG, trialB, contentA,
                           resetHover, false,
                           0.08f + 0.08f * sinf(now * 1.8f), now + 0.86f,
                           0.92f, 0.58f, true, true, true);

    if (backHov && lmbClick && ready) exiting = true;
    if (playHov && lmbClick && ready) {
        syncTrialsToGame();
        launchExit = true;
        launchT = 0.0f;
    }

    g_BatchAlpha = 1.0f;
    if (launchExit && launchT >= 0.50f) {
        g_SelectedJob = weapon == 0 ? JOB_NONE : JOB_STATIC_FIELD;
        s_RcWeapon = weapon;
        s_MainMenuRunConfigPanel = false;
        if (g_CreativeMode) g_GameManager.currentState = GameState::CREATIVE_CONFIG;
        else { c.reset(); FinalizeLoadout(c, FixedWeaponForSelectedJob()); }
        launchExit = false;
        launchT = 0.0f;
    }
    if (exiting && exitT >= 0.42f) {
        s_MainMenuRunConfigPanel = false;
        s_MainMenuResumeFromPanel = true;
        ResetRunConfigUi();
        g_MainMenuEntryT = 1.0f;
        entry = 0.0f;
        exitT = 0.0f;
        exiting = false;
        launchExit = false;
        launchT = 0.0f;
        g_GameManager.currentState = GameState::MAIN_MENU;
    }
}
static void Scene_SettingsInline(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const float dt = std::min(c.delta, 0.05f);
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float now = (float)glfwGetTime();
    const bool inGameSettings = (g_SettingsReturnTo == GameState::PAUSED);
    static int   tab = 0;          // 0=DISPLAY 1=AUDIO 2=CONTROL 3=SYSTEM
    static float entry = 0.0f;
    static float exitT = 0.0f;
    static bool  exiting = false;
    static bool  prevEsc = false;
    static bool  prevRmb = false;
    static float pulse = 0.0f;
    static int   pulseRow = -1;
    static float tabSwitchT   = 0.0f;
    static bool  s_volDrag    = false;
    static float listHover[32] = {};
    static float rowHover[4][6]  = {};
    static float optHover[4][6][8] = {};
    static float holdT = 0.0f;
    static int listCursor = 1;
    // Shared normalized scroll position for the left index and right board.
    // Each pane maps the same 0..1 value to its own row geometry.
    static float settingsScroll = 0.0f;
    static float settingsScrollTarget = 0.0f;
    static bool prevListW = false;
    static bool prevListS = false;
    static int detailRow = 0;
    struct SettingsSnapshot {
        int fps = 60;
        VfxDensity density = VfxDensity::FULL;
        bool shaderFx = false;
        MobVisualStyle mobStyle = MobVisualStyle::CLASSIC;
        bool combo = true;
        bool backdropBlur = true;
        int soundVol = 100;
        bool autoFire = true;
        bool autoSkill = false;
        bool crosshair = true;
        bool debugMode = false;
        Language language = Language::KR;
    };
    static SettingsSnapshot savedSettings = {};
    static bool savedSettingsValid = false;
    static bool confirmBack = false;
    static bool settingsDirty = false;
    static bool  wasInline = false;
    if (!wasInline || s_SettingsInlineNeedsReset) {
        entry = 0.0f;
        exitT = 0.0f;
        exiting = false;
        prevEsc = false;
        prevRmb = false;
        pulse = 0.0f;
        pulseRow = -1;
        tabSwitchT = 0.0f;
        s_volDrag  = false;
        holdT = 0.0f;
        listCursor = 1;
        settingsScroll = 0.0f;
        settingsScrollTarget = 0.0f;
        prevListW = false;
        prevListS = false;
        detailRow = 0;
        savedSettings = { g_FpsCap, g_VfxDensity, g_ShaderFx, g_MobVisualStyle,
                          g_ShowCombo, g_BackdropBlurEnabled, g_SoundVol,
                          g_AutoFire, g_AutoSkill, g_ShowCrosshair, g_DebugMode,
                          g_Language };
        savedSettingsValid = true;
        confirmBack = false;
        settingsDirty = false;
        for (auto& h : listHover) h = 0.0f;
        for (auto& category : rowHover)
            for (auto& h : category) h = 0.0f;
        for (auto& category : optHover)
            for (auto& row : category)
                for (auto& h : row) h = 0.0f;
        wasInline = true;
        s_SettingsInlineNeedsReset = false;
    }
    entry = std::min(1.0f, entry + dt);
    pulse = std::max(0.0f, pulse - dt * 2.0f);
    tabSwitchT += dt;

    auto restoreSettings = [&]() {
        if (!savedSettingsValid) return;
        g_FpsCap = savedSettings.fps;
        g_VfxDensity = savedSettings.density;
        g_ShaderFx = savedSettings.shaderFx;
        g_MobVisualStyle = savedSettings.mobStyle;
        g_ShowCombo = savedSettings.combo;
        g_BackdropBlurEnabled = savedSettings.backdropBlur;
        g_SoundVol = savedSettings.soundVol;
        g_AutoFire = savedSettings.autoFire;
        g_AutoSkill = savedSettings.autoSkill;
        g_ShowCrosshair = savedSettings.crosshair;
        g_DebugMode = savedSettings.debugMode;
        g_Language = savedSettings.language;
    };
    auto settingsChanged = [&]() {
        if (!savedSettingsValid) return false;
        return g_FpsCap != savedSettings.fps
            || g_VfxDensity != savedSettings.density
            || g_ShaderFx != savedSettings.shaderFx
            || g_MobVisualStyle != savedSettings.mobStyle
            || g_ShowCombo != savedSettings.combo
            || g_BackdropBlurEnabled != savedSettings.backdropBlur
            || g_SoundVol != savedSettings.soundVol
            || g_AutoFire != savedSettings.autoFire
            || g_AutoSkill != savedSettings.autoSkill
            || g_ShowCrosshair != savedSettings.crosshair
            || g_DebugMode != savedSettings.debugMode
            || g_Language != savedSettings.language;
    };

    const bool esc = c.window && glfwGetKey(c.window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    const bool rmb = c.window && glfwGetMouseButton(c.window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    const bool backInput = (esc && !prevEsc) || (rmb && !prevRmb);
    prevEsc = esc; prevRmb = rmb;
    const float entryOldOut = Smoothstep(LogoClamp01(entry / 0.35f));
    const float entryTreeIn = Smoothstep(LogoClamp01((entry - 0.20f) / 0.35f));
    if (backInput && !exiting && entry >= 0.55f) {
        if (settingsDirty) confirmBack = true;
        else exiting = true;
    }
    if (exiting) exitT = std::min(0.42f, exitT + dt);
    const float outP = exiting ? Smoothstep(LogoClamp01(exitT / 0.42f)) : 0.0f;
    const float treeA = entryTreeIn * (1.0f - outP);
    const float oldA = exiting ? outP : (1.0f - entryOldOut);
    const bool ready = !exiting && entry >= 0.55f;
    const bool inputReady = ready && !confirmBack;
    SetSceneTextureReveal(exiting
        ? std::max(0.0f, 1.0f - outP)
        : Smoothstep(LogoClamp01((entry - 0.04f) / 0.74f)));

    const float uiS = std::max(0.70f, std::min(sw * 0.94f / 1640.0f, sh * 0.90f / 910.0f));
    const float mainBW = std::min(560.0f, std::max(420.0f, sw * 0.34f));
    const float mainBH = 70.0f, mainGap = 15.0f;
    const float mainTotal = mainBH * 5.0f + mainGap * 4.0f;
    const float mainX = std::max(58.0f, sw * 0.075f);
    const float mainY = MainMenuCommandStartY(sh);
    // Settings uses its own full-canvas composition. The generic inline
    // layout is intentionally not used here because it compresses the page
    // into a small center panel.
    const float depthX = sw * 0.30f;
    const float depthY = std::max(112.0f * uiS, sh * 0.13f);
    const float detailX = sw * 0.55f;
    const float detailY = depthY;
    const float detailW = std::max(300.0f * uiS, sw - detailX - 64.0f * uiS);
    const float detailH = sh - depthY - 112.0f * uiS;

    // ── Large atmospheric backdrop ─────────────────────────────────
    // The field is intentionally left open; the live background is the page.

    // Canvas panel — solid core + feathered edges
    // Tab accent color — shared by sphere motif and option chips
    const float tabR = tab == 0 ? 0.35f : tab == 1 ? 1.00f : tab == 2 ? 0.35f : 0.72f;
    const float tabG = tab == 0 ? 0.72f : tab == 1 ? 0.72f : tab == 2 ? 0.92f : 0.52f;
    const float tabB = tab == 0 ? 1.00f : tab == 1 ? 0.30f : tab == 2 ? 0.55f : 1.00f;
    const bool korean = LangIndex() == 0;

    DrawPersistentSceneSideVignettes(sw, sh, kWideSceneLinearAlpha);
    {
        const float cW  = detailX + detailW - depthX;
        const float cH  = detailH;
        // Center at 78% of the panel width — large enough radius that rings bleed outside
        const float mCX = depthX + cW * 0.24f;
        const float mCY = depthY + cH * 0.50f;
        const float mR  = std::min(cH * 0.28f, cW * 0.17f);

        // Settings is a calibration screen, not a constellation browser.
        // Keep only a very soft focal field behind the values; constellation
        // nodes and the globe are intentionally omitted here.
        DrawSceneRadialVignette(mCX, mCY, mR * 2.2f, 0.035f * treeA);
    }

    if (s_MainMenuSettingsPanel)
        DrawMainOnedowLogo(sw, sh, 0.42f * oldA,
                           LogoClamp01(oldA + 0.20f * treeA), sw * 0.30f - 160.0f * (1.0f - oldA), 0.82f);

    DrawShadowedText(g_TextL, korean ? L"설정" : L"SETTINGS", mainX, 48.0f * uiS,
                     1.16f, 1.0f, 1.0f, 1.0f, 0.94f * treeA, 0.72f);
    DrawShadowedText(g_TextS, korean ? L"시스템 보정" : L"SYSTEM CALIBRATION",
                     mainX + 4.0f * uiS, 86.0f * uiS,
                     0.52f, 0.35f, 0.76f, 1.0f, 0.72f * treeA, 0.58f);

    // \uC88C\uCE21 ghost \uBC84\uD2BC \u2014 \uCEE8\uD14D\uC2A4\uD2B8\uC5D0 \uB530\uB77C \uBA54\uC778\uBA54\uB274 vs \uC77C\uC2DC\uC815\uC9C0 \uBA54\uB274
    if (s_MainMenuSettingsPanel) {
        const wchar_t* menu[5] = { korean ? L"플레이" : L"PLAY",
                                   korean ? L"상점" : L"SHOP",
                                   korean ? L"성도 기록" : L"ASTRAL_LOG",
                                   korean ? L"설정" : L"SETTING",
                                   korean ? L"종료" : L"EXIT" };
        static const wchar_t* menuSub[5] = { L"\uC2DC\uC791", L"\uC0C1\uC810", L"\uB3C4\uAC10", L"\uC124\uC815", L"\uAC8C\uC784 \uC885\uB8CC" };
        for (int i = 0; i < 5; ++i) {
            const float y = MainMenuButtonRailStartY(sh) + i * (mainBH + mainGap);
            const bool focus = i == 3;
            const float x = mainX - 250.0f * (1.0f - oldA) - (focus ? 0.0f : 24.0f * (1.0f - oldA));
            const float a = (focus ? 0.82f : 0.34f) * oldA;
            DrawShadowedText(g_TextL, MainMenuRouteLabel(LangIndex(), i), x, y + 2.0f, 1.04f,
                             1.0f, 1.0f, 1.0f, a, 0.70f);
            DrawShadowedText(g_TextS, MainMenuSubtitleLabel(LangIndex(), i), x + 4.0f, y + 43.0f, 0.48f,
                             0.72f, 0.77f, 0.84f, a * 0.9f, 0.62f);
        }
    } else if (!inGameSettings) {
        const wchar_t* menu[4] = { korean ? L"재개" : L"RESUME",
                                   korean ? L"설정" : L"CALIBRATION",
                                   korean ? L"\uD3EC\uAE30\uD558\uAE30" : L"ABANDON RUN",
                                   korean ? L"종료" : L"TERMINATE" };
        static const wchar_t* menuSub[4] = { L"\uC7AC\uAC1C", L"\uC124\uC815", L"\uB7F0 \uD3EC\uAE30", L"\uAC8C\uC784 \uC885\uB8CC" };
        for (int i = 0; i < 4; ++i) {
            const float y = MainMenuButtonRailStartY(sh) + i * (mainBH + mainGap);
            const bool focus = i == 1;
            const float x = mainX - 250.0f * (1.0f - oldA) - (focus ? 0.0f : 24.0f * (1.0f - oldA));
            const float a = (focus ? 0.82f : 0.34f) * oldA;
            DrawShadowedText(g_TextL, menu[i], x, y + 2.0f, 1.04f,
                             1.0f, 1.0f, 1.0f, a, 0.70f);
            DrawShadowedText(g_TextS, menuSub[i], x + 4.0f, y + 43.0f, 0.48f,
                             0.72f, 0.77f, 0.84f, a * 0.9f, 0.62f);
        }
    }

    // \u2550\u2550\u2550 TAB RAIL (DISPLAY / AUDIO / CONTROL / SYSTEM / BACK) \u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550
    struct SettingsTab { const wchar_t* en; const wchar_t* kr; };
    static const SettingsTab tabs[5] = {
        { L"DISPLAY",  L"\uD654\uBA74"       },
        { L"AUDIO",    L"\uC18C\uB9AC"       },
        { L"GAMEPLAY", L"\uAC8C\uC784\uD50C\uB808\uC774" },
        { L"ARCHIVE",  L"\uAE30\uB85D" },
        { L"BACK",     L"\uB4A4\uB85C"       },
    };
    if (tab >= 4) tab = 0;

    // The integrated list below is the only settings navigation surface.
    // The former orbit-tab controls were non-interactive and duplicated it.
    // The open field intentionally has no active-tab bridge line.

    // \u2550\u2550\u2550 RIGHT PANEL \u2014 SETTINGS ROWS \u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550
    const float dA   = treeA * (0.78f + 0.22f * (1.0f - pulse));
    // The settings body owns the full area to the right of the category
    // rail, split into a readable list column and a dedicated readout column.
    const float rpX  = depthX;
    const float rpW  = detailX + detailW - rpX;

    // Header: category context stays quiet so the selected setting can own
    // the visual hierarchy below it.
    DrawShadowedText(g_TextS, korean ? L"관측소 보정" : L"OBSERVATORY CALIBRATION", rpX, depthY, 0.56f,
                     0.48f, 0.82f, 1.0f, 0.70f * dA, 0.66f);
    DrawShadowedText(g_TextS, korean ? L"전체 설정" : L"ALL SETTINGS", rpX, depthY + 34.0f, 0.72f,
                     tabR, tabG, tabB, 0.90f * dA, 0.68f);
    const float sepY = depthY + 68.0f;
    LogoLine(rpX, sepY, rpX + rpW * 0.92f, sepY,
             0.76f * uiS, tabR, tabG, tabB, 0.22f * dA);

    // Row layout constants (5행 탭은 높이 줄임)
    // Use the available vertical field instead of compressing every tab into
    // the same short block. Sparse tabs therefore breathe, while DISPLAY
    // still keeps enough rows visible without creating a second duplicate UI.
    const float rRowH   = 220.0f;
    const float rFirstY = detailY + 92.0f;
    const float rLabelX = rpX;
    // Let the control column use the full available width. The old 900px cap
    // forced FPS options to shrink and made read-only values collide with the
    // description column on wider layouts.
    // Keep the interaction/readout panel inside its own right-side bounds.
    // The old minimum width could push the last option past the canvas on
    // narrow windows.
    const float rClickW = std::max(1.0f, rpW - 20.0f * uiS);

    if (pulse <= 0.01f) pulseRow = -1;

    // \u2500\u2500 Row data per tab \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500
    struct SRow {
        const wchar_t* label     = nullptr;
        const wchar_t* value     = nullptr; // nullptr = opts or hold bar
        bool readOnly  = false;
        bool isVolume  = false;             // left-half=-10, right-half=+10
        bool isDanger  = false;
        bool isHold    = false;
        const wchar_t* opts[8]  = {};       // 선택지 전체 목록 (없으면 optCount=0)
        int optCount   = 0;
        int optCur     = 0;                 // 현재 선택된 인덱스
    };
    SRow settingsRows[4][6] = {};
    const int rowCounts[4] = { 4, 5, kDebugSettingsVisible ? 4 : 3, 2 };

    static wchar_t s_volBuf[8];

    // \u2500\u2500 Render rows \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500
    // Build the same setting rows for every category so the right side can
    // render one continuous board instead of replacing the view per tab.
    swprintf_s(s_volBuf, L"%d", g_SoundVol);
    const int allLangCur = g_Language == Language::KR ? 0 :
                           g_Language == Language::EN ? 1 : 2;
    const int allFpsCur =
        g_FpsCap == 0 ? 0 : g_FpsCap == 30 ? 1 : g_FpsCap == 60 ? 2 :
        g_FpsCap == 144 ? 3 : g_FpsCap == 300 ? 4 : 5;
    settingsRows[0][0] = { korean ? L"FPS 제한" : L"FPS CAP", nullptr, false, false, false, false,
                           { korean ? L"동기화" : L"VSYNC", L"30", L"60", L"144", L"300",
                             korean ? L"무제한" : L"UNLIM" }, 6, allFpsCur };
    settingsRows[0][1] = { korean ? L"그래픽 품질" : L"GRAPHICS", nullptr, false, false, false, false,
                           { korean ? L"전체" : L"FULL", korean ? L"감소" : L"REDUCED" },
                           2, g_VfxDensity == VfxDensity::FULL ? 0 : 1 };
    settingsRows[0][2] = { korean ? L"콤보 HUD" : L"COMBO HUD", nullptr, false, false, false, false,
                           { korean ? L"켜짐" : L"ON", korean ? L"꺼짐" : L"OFF" }, 2, g_ShowCombo ? 0 : 1 };
    settingsRows[0][3] = { korean ? L"배경 블러" : L"BACKDROP BLUR", nullptr, false, false, false, false,
                           { korean ? L"꺼짐" : L"OFF", korean ? L"켜짐" : L"ON" },
                           2, g_BackdropBlurEnabled ? 1 : 0 };
    settingsRows[1][0] = { korean ? L"마스터 볼륨" : L"MASTER VOL", s_volBuf, false, true, false, false };
    settingsRows[1][1] = { korean ? L"BGM 채널" : L"BGM BUS", korean ? L"활성 · 고정" : L"ACTIVE · FIXED", true, false, false, false };
    settingsRows[1][2] = { korean ? L"효과음 채널" : L"SFX BUS", korean ? L"활성 · 고정" : L"ACTIVE · FIXED", true, false, false, false };
    settingsRows[1][3] = { korean ? L"출력" : L"OUTPUT", korean ? L"스테레오 · 고정" : L"STEREO · FIXED", true, false, false, false };
    settingsRows[1][4] = { korean ? L"오디오 엔진" : L"AUDIO ENGINE", L"MINIAUDIO · FIXED", true, false, false, false };
    settingsRows[2][0] = { korean ? L"자동 발사" : L"AUTO FIRE", nullptr, false, false, false, false,
                           { korean ? L"켜짐" : L"ON", korean ? L"꺼짐" : L"OFF" }, 2, g_AutoFire ? 0 : 1 };
    settingsRows[2][1] = { korean ? L"자동 스킬" : L"AUTO SKILL", nullptr, false, false, false, false,
                           { korean ? L"켜짐" : L"ON", korean ? L"꺼짐" : L"OFF" }, 2, g_AutoSkill ? 0 : 1 };
    settingsRows[2][2] = { korean ? L"조준선" : L"CROSSHAIR", nullptr, false, false, false, false,
                           { korean ? L"켜짐" : L"ON", korean ? L"꺼짐" : L"OFF" }, 2, g_ShowCrosshair ? 0 : 1 };
    settingsRows[2][3] = { korean ? L"\uB514\uBC84\uAE45 \uBAA8\uB4DC" : L"DEBUG MODE", nullptr, false, false, false, false,
                           { korean ? L"\uCF1C\uC9D0" : L"ON", korean ? L"\uB04C\uC9D0" : L"OFF" }, 2, g_DebugMode ? 0 : 1 };
    settingsRows[3][0] = { korean ? L"언어" : L"LANGUAGE", nullptr, false, false, false, false,
                           { korean ? L"한국어" : L"KOR", korean ? L"영어" : L"ENG", korean ? L"일본어" : L"JPN" },
                           3, allLangCur };
    settingsRows[3][1] = { korean ? L"데이터 초기화" : L"RESET DATA", nullptr, false, false, true, true };

    const wchar_t* allDescriptions[4][6] = {};
    const wchar_t* allDisplayDesc[4] = {
        korean ? L"프레임 동기화 기준" : L"Frame pacing target",
        korean ? L"장면 디테일 밀도" : L"Scene detail density",
        korean ? L"전투 콤보 표시" : L"Combat combo signal",
        korean ? L"배경 디테일 필터" : L"Background detail filter" };
    const wchar_t* allAudioDesc[5] = {
        korean ? L"전체 출력 음량" : L"Master output level",
        korean ? L"배경 음악 채널" : L"Background music routing",
        korean ? L"전투 효과음 채널" : L"Combat sound routing",
        korean ? L"출력 채널 구성" : L"Output channel layout",
        korean ? L"사용 중인 오디오 엔진" : L"Active audio runtime" };
    const wchar_t* allGameplayDesc[4] = {
        korean ? L"자동 조준 발사" : L"Automatic target fire",
        korean ? L"자동 스킬 발동" : L"Automatic skill trigger",
        korean ? L"플레이 중 조준선" : L"In-run aiming reticle",
        korean ? L"F 레벨업 / G 무적 단축키" : L"F level-up / G godmode hotkeys" };
    for (int i = 0; i < 4; ++i) allDescriptions[0][i] = allDisplayDesc[i];
    for (int i = 0; i < 5; ++i) allDescriptions[1][i] = allAudioDesc[i];
    for (int i = 0; i < 4; ++i) allDescriptions[2][i] = allGameplayDesc[i];
    allDescriptions[3][0] = korean ? L"인터페이스 언어" : L"Interface language";
    allDescriptions[3][1] = korean ? L"로컬 진행도 삭제" : L"Erase local progress";

    auto settingChanged = [&](int category, int row) {
        if (!savedSettingsValid) return false;
        if (category == 0) {
            if (row == 0) return g_FpsCap != savedSettings.fps;
            if (row == 1) return g_VfxDensity != savedSettings.density;
            if (row == 2) return g_ShowCombo != savedSettings.combo;
            if (row == 3) return g_BackdropBlurEnabled != savedSettings.backdropBlur;
        } else if (category == 1) {
            return row == 0 && g_SoundVol != savedSettings.soundVol;
        } else if (category == 2) {
            if (row == 0) return g_AutoFire != savedSettings.autoFire;
            if (row == 1) return g_AutoSkill != savedSettings.autoSkill;
            if (row == 2) return g_ShowCrosshair != savedSettings.crosshair;
            if (row == 3) return g_DebugMode != savedSettings.debugMode;
        } else if (category == 3) {
            return row == 0 && g_Language != savedSettings.language;
        }
        return false;
    };

    // Integrated settings catalogue: category headers and setting entries
    // share one scrollable stream in the diagonal left pane.
    struct SettingsListEntry { int category; int row; bool header; const wchar_t* label; };
    SettingsListEntry list[32] = {};
    int listCount = 0;
    auto addHeader = [&](int cat, const wchar_t* label) {
        list[listCount++] = { cat, -1, true, label };
    };
    auto addItem = [&](int cat, int row, const wchar_t* label) {
        list[listCount++] = { cat, row, false, label };
    };
    const wchar_t* displayLabels[4] = {
        korean ? L"FPS 제한" : L"FPS CAP",
        korean ? L"그래픽 품질" : L"GRAPHICS",
        korean ? L"콤보 HUD" : L"COMBO HUD",
        korean ? L"배경 블러" : L"BACKDROP BLUR" };
    const wchar_t* audioLabels[5] = {
        korean ? L"마스터 볼륨" : L"MASTER VOL",
        korean ? L"BGM 채널" : L"BGM BUS",
        korean ? L"효과음 채널" : L"SFX BUS",
        korean ? L"출력" : L"OUTPUT",
        korean ? L"오디오 엔진" : L"AUDIO ENGINE" };
    const wchar_t* gameplayLabels[4] = {
        korean ? L"자동 발사" : L"AUTO FIRE",
        korean ? L"자동 스킬" : L"AUTO SKILL",
        korean ? L"조준선" : L"CROSSHAIR",
        korean ? L"\uB514\uBC84\uAE45 \uBAA8\uB4DC" : L"DEBUG MODE" };
    const wchar_t* archiveLabels[2] = {
        korean ? L"언어" : L"LANGUAGE",
        korean ? L"데이터 초기화" : L"RESET DATA" };
    // Accordion model: every category keeps a visible header, while only
    // the selected category expands its controls.  The old board rendered
    // every category at once and made the settings page feel permanently
    // expanded, especially at 1280px-wide resolutions.
    addHeader(0, korean ? L"화면" : L"DISPLAY");
    if (tab == 0) for (int i = 0; i < 4; ++i) addItem(0, i, displayLabels[i]);
    addHeader(1, korean ? L"소리" : L"AUDIO");
    if (tab == 1) for (int i = 0; i < 5; ++i) addItem(1, i, audioLabels[i]);
    addHeader(2, korean ? L"게임플레이" : L"GAMEPLAY");
    if (tab == 2) for (int i = 0; i < (kDebugSettingsVisible ? 4 : 3); ++i)
        addItem(2, i, gameplayLabels[i]);
    addHeader(3, korean ? L"기록" : L"ARCHIVE");
    if (tab == 3) for (int i = 0; i < 2; ++i) addItem(3, i, archiveLabels[i]);

    bool focusChanged = false;

    auto isListItem = [&](int index) {
        return index >= 0 && index < listCount && !list[index].header;
    };
    auto stepListCursor = [&](int direction) {
        int next = listCursor;
        do {
            next += direction;
            if (next < 0) next = listCount - 1;
            if (next >= listCount) next = 0;
        } while (!isListItem(next));
        listCursor = next;
    };
    if (!isListItem(listCursor) || list[listCursor].category != tab) {
        listCursor = -1;
        for (int i = 0; i < listCount; ++i) {
            if (isListItem(i) && list[i].category == tab) {
                listCursor = i;
                break;
            }
        }
        if (listCursor < 0) listCursor = 0;
    }

    const float listX = mainX + 14.0f * uiS;
    const float listTop = depthY + 8.0f * uiS;
    const float listW = std::max(250.0f * uiS, depthX - listX - 38.0f * uiS);
    const float listBottom = sh - 154.0f * uiS;
    const float listH = std::max(220.0f * uiS, listBottom - listTop);
    const float listRowH = 52.0f * uiS;
    // The rendered y positions are pixels, so their scroll range must be
    // pixels too.  Row-count values here capped the previous list movement
    // to only a handful of pixels.
    const float leftMaxScroll = std::max(0.0f,
        (float)listCount * listRowH - listH);
    const float rightListTop = sepY + 34.0f * uiS;
    const float rightListBottom = sh - 206.0f * uiS;
    const float rightListH = std::max(260.0f * uiS, rightListBottom - rightListTop);
    const float rightRowH = 112.0f * uiS;
    const float rightMaxScroll = std::max(0.0f,
        (float)listCount * rightRowH - rightListH);
    settingsScrollTarget = std::max(0.0f, std::min(1.0f, settingsScrollTarget));
    // The current scroll value deliberately trails the requested position.
    // This single shared easing drives the left index, right board and both
    // scroll thumbs together.
    settingsScroll += (settingsScrollTarget - settingsScroll)
                    * std::min(1.0f, dt * 15.0f);
    settingsScroll = std::max(0.0f, std::min(1.0f, settingsScroll));
    const float leftScroll = settingsScroll * leftMaxScroll;
    const float rightScroll = settingsScroll * rightMaxScroll;
    const float listRailTopX = listX - 4.0f * uiS;
    const float listRailDiagonal = std::min(72.0f * uiS, listW * 0.18f);
    const float listRailBottomX = listRailTopX + listRailDiagonal;
    auto listRailXAt = [&](float y) {
        const float t = std::max(0.0f,
            std::min(1.0f, (y - listTop) / std::max(1.0f, listH)));
        return listRailTopX + listRailDiagonal * t;
    };
    const bool overList = inputReady && mx >= listX - 24.0f * uiS
                       && mx < listX + listW + 28.0f * uiS
                       && my >= listTop && my < listBottom;
    const bool overRightList = inputReady && mx >= rpX - 10.0f * uiS
                            && mx < rpX + rClickW
                            && my >= rightListTop && my < rightListBottom;
    // The board is one continuous document.  Let the wheel work anywhere in
    // its central canvas, including category separators, so AUDIO/GAMEPLAY/
    // ARCHIVE never depend on a narrow hit strip.
    const bool overSettingsCanvas = inputReady
                                 && mx >= listX - 30.0f * uiS
                                 && mx < rpX + rpW + 24.0f * uiS
                                 && my >= listTop && my < rightListBottom;
    if (overSettingsCanvas && g_ScrollAccum != 0.0f) {
        settingsScrollTarget -= g_ScrollAccum * 0.085f;
        settingsScrollTarget = std::max(0.0f, std::min(1.0f, settingsScrollTarget));
        g_ScrollAccum = 0.0f;
    }
    const bool listWKey = c.window && glfwGetKey(c.window, GLFW_KEY_W) == GLFW_PRESS;
    const bool listSKey = c.window && glfwGetKey(c.window, GLFW_KEY_S) == GLFW_PRESS;
    const int cursorBeforeKeys = listCursor;
    if ((overList || overRightList) && listWKey && !prevListW) stepListCursor(-1);
    if ((overList || overRightList) && listSKey && !prevListS) stepListCursor(1);
    focusChanged = listCursor != cursorBeforeKeys;
    prevListW = listWKey;
    prevListS = listSKey;
    auto revealFocus = [&]() {
        if (rightMaxScroll <= 0.0f) {
            settingsScrollTarget = 0.0f;
            return;
        }
        const float target = std::max(0.0f, std::min(rightMaxScroll,
            (float)listCursor * rightRowH - rightListH * 0.42f));
        settingsScrollTarget = target / rightMaxScroll;
    };
    if (focusChanged) revealFocus();
    if (isListItem(listCursor)) {
        const SettingsListEntry& selectedEntry = list[listCursor];
        if (selectedEntry.category != tab || selectedEntry.row != detailRow) {
            tab = selectedEntry.category;
            detailRow = selectedEntry.row;
            focusChanged = true;
            pulse = 1.0f;
            tabSwitchT = 0.0f;
            for (auto& category : rowHover)
                for (auto& h : category) h = 0.0f;
            for (auto& category : optHover)
                for (auto& row : category)
                    for (auto& h : row) h = 0.0f;
        }
    }
    if (focusChanged) revealFocus();

    // The left catalogue uses the same diagonal constellation language as the
    // PLAY trial list. Rows grow from one restrained  rail instead of sitting
    // on a rigid text column.
    BatchFlush();
    glEnable(GL_SCISSOR_TEST);
    glScissor((GLint)(listX - 34.0f * uiS),
              (GLint)(sh - listBottom),
              (GLint)(listW + 72.0f * uiS),
              (GLint)(listBottom - listTop));
    LogoLine(listRailTopX, listTop - 8.0f * uiS,
             listRailBottomX, listBottom + 8.0f * uiS,
             0.82f * uiS, 0.34f, 0.72f, 1.0f, 0.24f * dA);
    LogoLine(listRailTopX + 13.0f * uiS, listTop - 8.0f * uiS,
             listRailBottomX + 13.0f * uiS, listBottom + 8.0f * uiS,
             0.40f * uiS, 0.34f, 0.72f, 1.0f, 0.08f * dA);
    DrawVisibleConstellNode(listRailTopX, listTop - 8.0f * uiS,
                            2.2f * uiS, 0.34f, 0.72f, 1.0f,
                            0.48f * dA, false, false);
    DrawVisibleConstellNode(listRailBottomX, listBottom + 8.0f * uiS,
                            2.2f * uiS, 0.34f, 0.72f, 1.0f,
                            0.48f * dA, false, false);
    for (int i = 0; i < listCount; ++i) {
        const float y = listTop + i * listRowH - leftScroll;
        if (y < listTop - listRowH || y > listBottom) {
            listHover[i] = UpdateMenuCommandHover(listHover[i], false, dt);
            continue;
        }
        const float rowCenter = y + listRowH * 0.50f;
        const float railX = listRailXAt(rowCenter);
        const bool hov = overList && mx >= listX - 10.0f * uiS
                      && mx < listX + listW + 18.0f * uiS
                      && my >= y && my < y + listRowH;
        if (hov && lmb && !g_LmbPrev) {
            int targetIndex = i;
            if (list[i].header) {
                // Clicking a collapsed category expands it on the next
                // frame; the first row becomes the keyboard/detail focus.
                tab = list[i].category;
                detailRow = 0;
                listCursor = i;
                focusChanged = true;
                pulse = 1.0f;
                tabSwitchT = 0.0f;
                continue;
            }
            if (!list[targetIndex].header) {
                listCursor = targetIndex;
                detailRow = list[targetIndex].row;
                focusChanged = true;
                if (tab != list[targetIndex].category) {
                    tab = list[targetIndex].category;
                    pulse = 1.0f;
                    tabSwitchT = 0.0f;
                    detailRow = list[targetIndex].row;
                    for (auto& category : rowHover)
                        for (auto& h : category) h = 0.0f;
                    for (auto& category : optHover)
                        for (auto& row : category)
                            for (auto& h : row) h = 0.0f;
                }
            }
        }
        listHover[i] = UpdateMenuCommandHover(listHover[i], hov && !list[i].header, dt);
        if (list[i].header) {
            const float headerX = railX + 20.0f * uiS;
            DrawShadowedText(g_TextS, list[i].label, headerX,
                             y + 3.0f * uiS, 1.10f,
                             0.34f, 0.72f, 1.0f, 0.78f * dA, 0.56f);
            LogoLine(headerX, y + listRowH - 2.0f * uiS,
                     listX + listW * 0.78f, y + listRowH - 2.0f * uiS,
                     0.7f * uiS, 0.34f, 0.72f, 1.0f, 0.16f * dA);
        } else {
            const bool changed = settingChanged(list[i].category, list[i].row);
            // Clicking is navigation only.  Persistent colour is reserved
            // for an actually changed value; hover remains transient.
            const float visual = std::max(listHover[i], changed ? 0.72f : 0.0f);
            const float itemX = railX + 22.0f * visual;
            const float nodeX = itemX + 5.0f * uiS;
            const float a = changed ? (0.86f + 0.14f * visual)
                                    : (0.42f + 0.20f * visual);
            const float r = changed ? 1.0f : 0.68f + 0.10f * visual;
            const float g = changed ? 0.68f : 0.76f + 0.12f * visual;
            const float b = changed ? 0.32f : 1.0f;
            if (visual > 0.02f)
                DrawConstellationDisc(nodeX, rowCenter,
                                      (11.0f + 7.0f * visual) * uiS,
                                      r, g, b, 0.07f * visual * dA);
            DrawVisibleConstellLine(railX, rowCenter, nodeX, rowCenter,
                                    0.56f * uiS, r, g, b,
                                    (0.08f + 0.22f * visual) * dA);
            DrawVisibleConstellNode(nodeX, rowCenter,
                                    (2.8f + 1.0f * visual) * uiS,
                                    r, g, b, a * dA,
                                    false, false);
            const float textA = changed
                ? (0.98f + 0.02f * visual)
                : (0.92f + 0.08f * visual);
            DrawShadowedText(g_TextL, list[i].label,
                             itemX + 20.0f * uiS, y + 8.0f * uiS, 0.66f,
                             changed ? 1.0f : 1.0f,
                             changed ? 0.68f : 1.0f,
                             changed ? 0.32f : 1.0f, textA * dA, 0.62f);
        }
    }
    BatchFlush();
    glDisable(GL_SCISSOR_TEST);
    if (leftMaxScroll > 0.0f) {
        const float trackX = listX + listW + 12.0f * uiS;
        const float leftTotalH = listCount * listRowH;
        const float thumbH = std::max(28.0f * uiS, listH * (listH / leftTotalH));
        const float thumbY = listTop + (listH - thumbH) * settingsScroll;
        LogoLine(trackX, listTop, trackX, listBottom,
                 0.7f * uiS, 0.34f, 0.72f, 1.0f, 0.12f * dA);
        LogoLine(trackX, thumbY, trackX, thumbY + thumbH,
                 1.8f * uiS, 0.34f, 0.82f, 1.0f, 0.55f * dA);
    }


    // Right settings board: every category remains present in one continuous
    // scroll stream. Its logical rows use the same cursor/scroll as the left
    // catalogue, while the larger row geometry keeps the existing type scale.
    int hoverCategory = -1;
    int hoverSettingRow = -1;
    int hoverSettingOpt = -1;
    bool resetPressed = false;
    const float rightRowX = rpX + 18.0f * uiS;
    const float rightRowW = rClickW - 18.0f * uiS;
    // Controls are a left-anchored column inside the settings panel. This
    // leaves a comfortable readout area while avoiding the old far-right,
    // centered-looking option placement.
    const float rightControlInset = std::min(
        rpW * 0.34f, std::max(0.0f, rClickW - 160.0f * uiS));
    const float rightControlX = rpX + rightControlInset;
    const float rightControlEnd = rpX + rClickW - 18.0f * uiS;
    const float rightControlW = std::max(1.0f,
                                         rightControlEnd - rightControlX);
    auto categoryColor = [&](int category, float& r, float& g, float& b) {
        if (category == 0) { r = 0.35f; g = 0.72f; b = 1.00f; }
        else if (category == 1) { r = 1.00f; g = 0.72f; b = 0.30f; }
        else if (category == 2) { r = 0.35f; g = 0.92f; b = 0.55f; }
        else { r = 0.72f; g = 0.52f; b = 1.00f; }
    };

    // Every option row shares one control grid. Measuring each row on its own
    // made the second option drift whenever a label such as "부드럽게" was
    // wider than ON/OFF. Measure the longest localized label per slot once,
    // then reuse those columns for FPS, toggles, and language choices.
    auto optionLabelForLanguage = [&](int category, int row,
                                      int optionLanguage, int optionIndex)
                                      -> const wchar_t* {
        if (category == 0) {
            if (row == 0) {
                static const wchar_t* fps[3][6] = {
                    { L"동기화", L"30", L"60", L"144", L"300", L"무제한" },
                    { L"VSYNC", L"30", L"60", L"144", L"300", L"UNLIM" },
                    { L"同期", L"30", L"60", L"144", L"300", L"無制限" }
                };
                return fps[optionLanguage][optionIndex];
            }
            if (row == 1) {
                static const wchar_t* graphics[3][2] = {
                    { L"전체", L"감소" }, { L"FULL", L"REDUCED" }, { L"全体", L"削減" }
                };
                return graphics[optionLanguage][optionIndex];
            }
            if (row == 2) {
                static const wchar_t* toggle[3][2] = {
                    { L"켜짐", L"꺼짐" }, { L"ON", L"OFF" }, { L"オン", L"オフ" }
                };
                return toggle[optionLanguage][optionIndex];
            }
            if (row == 3) {
                static const wchar_t* blur[3][2] = {
                    { L"꺼짐", L"켜짐" }, { L"OFF", L"ON" }, { L"オフ", L"オン" }
                };
                return blur[optionLanguage][optionIndex];
            }
        } else if (category == 2 && row < 4) {
            static const wchar_t* toggle[3][2] = {
                { L"켜짐", L"꺼짐" }, { L"ON", L"OFF" }, { L"オン", L"オフ" }
            };
            return toggle[optionLanguage][optionIndex];
        } else if (category == 3 && row == 0) {
            static const wchar_t* language[3][3] = {
                { L"한국어", L"영어", L"일본어" },
                { L"KOR", L"ENG", L"JPN" },
                { L"韓国語", L"英語", L"日本語" }
            };
            return language[optionLanguage][optionIndex];
        }
        return nullptr;
    };

    const float commonOptionSc = 0.96f;
    const float commonChipPadX = 8.0f * uiS;
    const float commonChipGap = 12.0f * uiS;
    float commonOptionCellW[8] = {};
    int commonOptionCount = 0;
    for (int category = 0; category < 4; ++category) {
        for (int row = 0; row < 6; ++row) {
            const SRow& setting = settingsRows[category][row];
            if (setting.optCount <= 0) continue;
            commonOptionCount = std::max(commonOptionCount, setting.optCount);
            for (int j = 0; j < setting.optCount; ++j) {
                float widest = g_TextL.Width(setting.opts[j], commonOptionSc);
                for (int language = 0; language < 3; ++language) {
                    const wchar_t* localized = optionLabelForLanguage(
                        category, row, language, j);
                    if (localized)
                        widest = std::max(widest,
                                          g_TextL.Width(localized, commonOptionSc));
                }
                commonOptionCellW[j] = std::max(
                    commonOptionCellW[j], widest + commonChipPadX * 2.0f);
            }
        }
    }
    float commonOptionTotalW = 0.0f;
    for (int j = 0; j < commonOptionCount; ++j)
        commonOptionTotalW += commonOptionCellW[j];
    if (commonOptionCount > 1)
        commonOptionTotalW += commonChipGap * (commonOptionCount - 1);
    float commonOptionScale = commonOptionSc;
    if (commonOptionTotalW > rightControlW && commonOptionTotalW > 1.0f) {
        commonOptionScale *= std::max(0.55f, rightControlW / commonOptionTotalW);
        for (int j = 0; j < commonOptionCount; ++j)
            commonOptionCellW[j] = commonOptionCellW[j]
                * (commonOptionScale / commonOptionSc);
    }

    BatchFlush();
    glEnable(GL_SCISSOR_TEST);
    glScissor((GLint)rpX,
              (GLint)(sh - rightListBottom),
              (GLint)rClickW,
              (GLint)(rightListBottom - rightListTop));
    for (int i = 0; i < listCount; ++i) {
        const float y = rightListTop + i * rightRowH - rightScroll;
        if (y < rightListTop - rightRowH || y > rightListBottom) continue;

        const SettingsListEntry& entry = list[i];
        float catR = 0.35f, catG = 0.72f, catB = 1.0f;
        categoryColor(entry.category, catR, catG, catB);
        if (entry.header) {
            const bool headerHover = overRightList
                                  && mx >= rightRowX - 12.0f * uiS
                                  && mx < rightRowX + rightRowW
                                  && my >= y && my < y + rightRowH;
            if (headerHover && lmb && !g_LmbPrev) {
                for (int k = 0; k < listCount; ++k) {
                    if (!list[k].header && list[k].category == entry.category) {
                        listCursor = k;
                        detailRow = list[k].row;
                        break;
                    }
                }
                tab = entry.category;
                focusChanged = true;
                pulse = 1.0f;
                tabSwitchT = 0.0f;
            }
            DrawShadowedText(g_TextS, entry.label, rightRowX,
                             y + 1.0f * uiS, 1.65f,
                             catR, catG, catB,
                             1.0f * dA, 0.56f);
            const float headerRuleY = y + 62.0f * uiS;
            LogoLine(rightRowX, headerRuleY,
                     rightControlEnd, headerRuleY,
                     1.05f * uiS, catR, catG, catB, 0.34f * dA);
            DrawVisibleConstellNode(rightRowX, headerRuleY,
                                    2.6f * uiS, catR, catG, catB,
                                    0.62f * dA, false, false);
            continue;
        }

        const int category = entry.category;
        const int row = entry.row;
        SRow& setting = settingsRows[category][row];
        const bool changed = settingChanged(category, row);
        const bool itemHover = overRightList
                            && mx >= rightRowX - 12.0f * uiS
                            && mx < rightRowX + rightRowW
                            && my >= y && my < y + rightRowH;
        if (itemHover) {
            hoverCategory = category;
            hoverSettingRow = row;
            if (lmb && !g_LmbPrev) {
                listCursor = i;
                tab = category;
                detailRow = row;
                focusChanged = true;
            }
        }
        rowHover[category][row] = UpdateMenuCommandHover(
            rowHover[category][row], itemHover && !setting.readOnly, dt);
        const float focus = rowHover[category][row];
        const float rowA = dA * (0.72f + 0.28f * Smoothstep(
            LogoClamp01((rightListTop + rightListH - y) / std::max(1.0f, rightListH))));

        if (changed || focus > 0.01f) {
            const float bgA = changed ? 0.12f : 0.035f;
            drawRect(rightRowX, y + 4.0f * uiS, rightRowW,
                     rightRowH - 14.0f * uiS,
                     changed ? 0.52f : 0.20f,
                     changed ? 0.30f : 0.34f,
                     changed ? 0.08f : 0.44f,
                     bgA * rowA);
        }
        if (focus > 0.01f) {
            drawRect(rightRowX, y + rightRowH - 10.0f * uiS,
                     rightRowW, 1.0f,
                     0.34f, 0.72f, 1.0f, 0.16f * focus * rowA);
        }

        const float titleR = changed ? 1.0f : 0.82f;
        const float titleG = changed ? 0.68f : 0.86f;
        const float titleB = changed ? 0.32f : 1.0f;
        DrawShadowedText(g_TextL, setting.label,
                         rightRowX + 20.0f * uiS, y + 12.0f * uiS, 1.18f,
                         titleR, titleG, titleB,
                         1.0f * rowA,
                         0.66f);
        if (allDescriptions[category][row]) {
            DrawShadowedText(g_TextS, allDescriptions[category][row],
                             rightRowX + 20.0f * uiS, y + 57.0f * uiS, 0.72f,
                             0.72f, 0.80f, 0.90f, 1.0f * rowA, 0.58f);
        }
        if (changed) {
            DrawConstellationDisc(rightRowX + 5.0f * uiS,
                                  y + rightRowH * 0.50f,
                                  13.0f * uiS, 1.0f, 0.56f, 0.18f,
                                  0.10f * rowA);
            DrawVisibleConstellNode(rightRowX + 5.0f * uiS,
                                    y + rightRowH * 0.50f, 3.6f * uiS,
                                    1.0f, 0.64f, 0.26f,
                                    0.88f * rowA, false, true);
        }

        const float controlY = y + 47.0f * uiS;
        const float controlHitY = controlY - 30.0f * uiS;
        const float controlHitH = 60.0f * uiS;
        if (setting.isVolume) {
            const float slX = rightControlX;
            const float slW = std::max(120.0f * uiS, rightControlW - 62.0f * uiS);
            const float vol01 = g_SoundVol / 100.0f;
            const bool inBar = inputReady
                && mx >= slX - 8.0f * uiS && mx <= slX + slW + 46.0f * uiS
                && my >= controlY - 18.0f * uiS && my <= controlY + 18.0f * uiS;
            if (inputReady && lmb && (inBar || s_volDrag)) {
                s_volDrag = true;
                const float t = std::max(0.0f, std::min(1.0f,
                    ((float)mx - slX) / std::max(1.0f, slW)));
                g_SoundVol = (int)(t * 100.0f + 0.5f);
            }
            const float glow = s_volDrag ? 1.0f : focus;
            drawRect(slX, controlY - 4.0f * uiS, slW, 8.0f * uiS,
                     0.18f, 0.24f, 0.34f, 0.50f * rowA);
            drawRect(slX, controlY - 4.0f * uiS, slW * vol01, 8.0f * uiS,
                     0.48f, 0.82f, 1.0f, (0.60f + 0.28f * glow) * rowA);
            drawDiamond(slX + slW * vol01, controlY,
                        (6.0f + 3.5f * glow) * uiS,
                        0.76f + 0.24f * glow, 0.90f + 0.10f * glow,
                        1.0f, (0.86f + 0.14f * glow) * rowA);
            wchar_t vbuf[8];
            swprintf_s(vbuf, L"%d", g_SoundVol);
            DrawShadowedText(g_TextL, vbuf, slX + slW + 16.0f * uiS,
                             controlY - 14.0f * uiS, 0.88f,
                             0.48f, 0.82f, 1.0f,
                             (0.72f + 0.24f * glow) * rowA, 0.58f);
        } else if (setting.isHold) {
            const float barW = std::min(250.0f * uiS, rightControlW);
            const float barH = 30.0f * uiS;
            const float barX = rightControlX;
            const float barY = controlY - barH * 0.5f;
            const bool resetHover = inputReady
                && mx >= barX && mx < barX + barW
                && my >= controlHitY && my < controlHitY + controlHitH;
            if (resetHover) {
                hoverCategory = category;
                hoverSettingRow = row;
                resetPressed = lmb;
            }
            const float showA = std::max(rowHover[category][row],
                                        holdT > 0.01f ? 1.0f : 0.0f);
            drawRect(barX, barY, barW, barH,
                     0.18f, 0.04f, 0.04f, 0.55f * showA * rowA);
            if (holdT > 0.01f) {
                drawRect(barX + 2.0f, barY + 2.0f,
                         (barW - 4.0f) * (holdT / 1.5f), barH - 4.0f,
                         1.0f, 0.22f, 0.14f, 0.92f * rowA);
            }
            DrawShadowedText(g_TextS,
                             holdT > 0.01f
                             ? (korean ? L"삭제 중..." : L"ERASING...")
                                 : (korean ? L"길게 눌러 초기화" : L"HOLD TO RESET"),
                             barX + 12.0f * uiS, barY + 6.0f * uiS, 0.74f,
                             1.0f, 0.36f, 0.30f,
                             1.0f * rowA,
                             0.58f);
        } else if (setting.optCount > 0) {
            // Use the shared option grid measured above. Each row now starts
            // and continues at the same x positions regardless of whether its
            // labels are ON/OFF, CLASSIC/SOFT, or longer localized strings.
            const float optSc = commonOptionScale;
            const float chipPadX = commonChipPadX;
            const float chipGap = commonChipGap;
            float ox = rightControlX;
            for (int j = 0; j < setting.optCount; ++j) {
                const float cellW = commonOptionCellW[j];
                const bool optionHover = inputReady
                    && mx >= ox && mx < ox + cellW
                    && my >= controlHitY && my < controlHitY + controlHitH;
                optHover[category][row][j] = UpdateMenuCommandHover(
                    optHover[category][row][j], optionHover, dt);
                if (optionHover) {
                    hoverCategory = category;
                    hoverSettingRow = row;
                    hoverSettingOpt = j;
                }
                const bool current = j == setting.optCur;
                const float optionHoverT = optHover[category][row][j];
                // The selected value must read as state, not just as a tiny
                // marker. Keep it near-white at full alpha; non-selected
                // values remain visible but recede until hovered.
                const float optionA = current
                    ? 1.0f
                    : (0.32f + 0.44f * optionHoverT);
                const float optionR = current
                    ? catR * 0.42f + 0.58f
                    : catR * (0.68f + 0.18f * optionHoverT);
                const float optionG = current
                    ? catG * 0.42f + 0.58f
                    : catG * (0.68f + 0.18f * optionHoverT);
                const float optionB = current
                    ? catB * 0.42f + 0.58f
                    : catB * (0.68f + 0.18f * optionHoverT);
                DrawShadowedText(g_TextL, setting.opts[j],
                                 ox + chipPadX,
                                 controlY, optSc,
                                 optionR, optionG, optionB,
                                 optionA * rowA, current ? 0.44f : 0.30f);
                if (current) {
                    drawDiamond(ox - 8.0f * uiS, controlY,
                                (3.8f + 0.8f * optionHoverT) * uiS,
                                optionR, optionG, optionB,
                                0.96f * rowA);
                }
                ox += cellW + chipGap;
            }
            if (category == 0 && row == 3) {
#ifdef _WIN32
                const wchar_t* blurHelp[] = {
                    korean ? L"Windows 투명 효과가 필요합니다."
                           : L"Requires Windows transparency effects.",
                    korean ? L"배터리 절약 모드에서는 블러가 제한됩니다."
                           : L"Battery saver can restrict background blur.",
                    korean ? L"블러가 안 보이면 절약 모드를 해제해주세요."
                           : L"Disable energy saver if blur is unavailable."
                };
#else
                const wchar_t* blurHelp[] = {
                    korean ? L"게임 배경을 흐리게 하여 메뉴 가독성을 높입니다."
                           : L"Blurs the game background to improve menu readability.",
                    korean ? L"바탕화면 블러 지원은 운영체제에 따라 다릅니다."
                           : L"Desktop blur support depends on the operating system."
                };
#endif
                const float helpX = ox + 8.0f * uiS;
                const float helpW = std::max(1.0f, rightControlEnd - helpX);
                float helpScale = 0.84f;
                for (const wchar_t* line : blurHelp) {
                    const float width = g_TextS.Width(line, 0.84f);
                    if (width > helpW)
                        helpScale = std::min(helpScale, 0.84f * helpW / width);
                }
                float helpY = y + 20.0f * uiS;
                for (const wchar_t* line : blurHelp) {
                    DrawShadowedText(g_TextS, line, helpX, helpY, helpScale,
                                     0.72f, 0.82f, 0.92f, rowA, 0.58f);
                    helpY += std::max(22.0f * uiS,
                                     g_TextS.Height(line, helpScale) + 3.0f * uiS);
                }
            }
        } else if (setting.value) {
            DrawShadowedText(g_TextL, setting.value,
                             rightControlX, controlY, 0.88f,
                             0.70f, 0.78f, 0.88f, 1.0f * rowA, 0.58f);
        }
    }
    if (!lmb) s_volDrag = false;
    BatchFlush();
    glDisable(GL_SCISSOR_TEST);
    if (rightMaxScroll > 0.0f) {
        const float rightTotalH = listCount * rightRowH;
        const float thumbH = std::max(32.0f * uiS,
                                      rightListH * (rightListH / rightTotalH));
        const float thumbY = rightListTop
            + (rightListH - thumbH) * settingsScroll;
        const float trackX = rightControlEnd + 12.0f * uiS;
        LogoLine(trackX, rightListTop, trackX, rightListBottom,
                 0.7f * uiS, 0.34f, 0.72f, 1.0f, 0.12f * dA);
        LogoLine(trackX, thumbY, trackX, thumbY + thumbH,
                 1.8f * uiS, 0.34f, 0.82f, 1.0f, 0.55f * dA);
    }

    if (focusChanged) revealFocus();

    // Commit controls live below the detail readout. Changes are applied
    // provisionally while browsing, but only this button writes them.
    settingsDirty = settingsChanged();
    const float backX = mainX;
    const float backY = sh - 126.0f * uiS;
    const float backW = 286.0f * uiS;
    const float backH = 64.0f * uiS;
    const float saveX = detailX + 42.0f * uiS;
    const float saveY = sh - 162.0f * uiS;
    const float saveW = std::min(486.0f * uiS, detailW * 0.82f);
    const float saveH = 64.0f * uiS;
    const bool backHit = ready && !confirmBack
                       && mx >= backX && mx < backX + backW
                       && my >= backY && my < backY + backH;
    const bool saveHit = ready && !confirmBack
                       && mx >= saveX && mx < saveX + saveW
                       && my >= saveY && my < saveY + saveH;

    DrawUnifiedMenuCommand(korean ? L"뒤로" : L"BACK",
                           korean ? L"설정" : L"SETTINGS",
                           backX, backY, backW, backH,
                           0.34f, 0.72f, 1.0f, treeA,
                           0.0f, backHit, 0.0f, now,
                           0.88f, 0.42f, true, true, true);
    DrawUnifiedMenuCommand(korean ? L"변경 저장" : L"SAVE CHANGES",
                           settingsDirty
                               ? (korean ? L"설정 기록" : L"WRITE CONFIGURATION")
                               : (korean ? L"변경 없음" : L"NO CHANGES"),
                           saveX, saveY, saveW, saveH,
                           0.34f, 0.82f, 1.0f,
                           treeA * (settingsDirty ? 1.0f : 0.24f),
                           0.0f, saveHit && settingsDirty, 0.0f, now + 0.2f,
                            0.78f, 0.38f, true, true, true);

    if (ready && !confirmBack && lmb && !g_LmbPrev && backHit) {
        if (settingsDirty) confirmBack = true;
        else exiting = true;
    }
    if (ready && !confirmBack && lmb && !g_LmbPrev && saveHit && settingsDirty) {
        SaveGame();
        savedSettings = { g_FpsCap, g_VfxDensity, g_ShaderFx, g_MobVisualStyle,
                          g_ShowCombo, g_BackdropBlurEnabled, g_SoundVol,
                          g_AutoFire, g_AutoSkill, g_ShowCrosshair, g_DebugMode,
                          g_Language };
        savedSettingsValid = true;
        settingsDirty = false;
        pulse = 1.0f;
    }

    if (confirmBack) {
        const float cx = sw * 0.50f;
        const float cy = sh * 0.52f;
        drawRect(0.0f, 0.0f, sw, sh, 0.01f, 0.02f, 0.04f, 0.56f * treeA);
        DrawShadowedText(g_TextL, L"UNSAVED CHANGES", cx - 168.0f * uiS,
                         cy - 66.0f * uiS, 0.96f, 1.0f, 1.0f, 1.0f,
                         0.98f * treeA, 0.72f);
        DrawShadowedText(g_TextS, L"SAVE SETTINGS BEFORE EXIT?",
                         cx - 132.0f * uiS, cy - 28.0f * uiS, 0.58f,
                         0.60f, 0.72f, 0.86f, 0.82f * treeA, 0.58f);
        const float dialogY = cy + 26.0f * uiS;
        const float dialogW = 210.0f * uiS;
        const float dialogH = 48.0f * uiS;
        const bool saveBackHit = mx >= cx - dialogW - 12.0f * uiS
                              && mx < cx - 12.0f * uiS
                              && my >= dialogY && my < dialogY + dialogH;
        const bool discardHit = mx >= cx + 12.0f * uiS
                              && mx < cx + dialogW + 12.0f * uiS
                              && my >= dialogY && my < dialogY + dialogH;
        DrawUnifiedMenuCommand(L"SAVE & BACK", nullptr,
                               cx - dialogW - 12.0f * uiS, dialogY,
                               dialogW, dialogH, 0.34f, 0.82f, 1.0f,
                               treeA, 0.0f, saveBackHit, 0.0f, now,
                               0.70f, 0.36f, true, true, true);
        DrawUnifiedMenuCommand(L"DISCARD", nullptr,
                               cx + 12.0f * uiS, dialogY,
                               dialogW, dialogH, 1.0f, 0.32f, 0.28f,
                               treeA, 0.0f, discardHit, 0.0f, now + 0.2f,
                               0.76f, 0.36f, true, true, true);
        if (lmb && !g_LmbPrev && saveBackHit) {
            SaveGame();
            savedSettings = { g_FpsCap, g_VfxDensity, g_ShaderFx, g_MobVisualStyle,
                              g_ShowCombo, g_BackdropBlurEnabled, g_SoundVol,
                              g_AutoFire, g_AutoSkill, g_ShowCrosshair, g_DebugMode,
                              g_Language };
            settingsDirty = false;
            confirmBack = false;
            exiting = true;
        } else if (lmb && !g_LmbPrev && discardHit) {
            restoreSettings();
            glfwSwapInterval(g_FpsCap == 0 ? 1 : 0);
            settingsDirty = false;
            confirmBack = false;
            exiting = true;
        }
    }

    // \u2500\u2500 Click handling \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500
     // Apply option clicks using the category/row that owns the hovered
     // control. The right board is no longer filtered by the current tab.
     if (inputReady && lmb && !g_LmbPrev
         && hoverCategory >= 0 && hoverSettingRow >= 0
         && hoverSettingOpt >= 0) {
         const int category = hoverCategory;
         const int row = hoverSettingRow;
         const int option = hoverSettingOpt;
         pulseRow = row;
         pulse = 1.0f;
         tab = category;
         detailRow = row;
         if (category == 0) {
             if (row == 0) {
                 const int fps[] = { 0, 30, 60, 144, 300, -1 };
                 g_FpsCap = fps[option];
                 glfwSwapInterval(g_FpsCap == 0 ? 1 : 0);
             } else if (row == 1) {
                 g_VfxDensity = (option == 0) ? VfxDensity::FULL : VfxDensity::REDUCED;
            } else if (row == 2) {
                g_ShowCombo = option == 0;
            } else if (row == 3) {
                g_BackdropBlurEnabled = option != 0;
            }
         } else if (category == 2) {
             if (row == 0) g_AutoFire = option == 0;
             else if (row == 1) g_AutoSkill = option == 0;
             else if (row == 2) g_ShowCrosshair = option == 0;
             else if (row == 3) g_DebugMode = option == 0;
         } else if (category == 3 && row == 0) {
             g_Language = option == 0 ? Language::KR :
                          option == 1 ? Language::EN : Language::JP;
         }
     }

     // RESET DATA is armed only while the visible reset control is held.
     if (inputReady && resetPressed) {
         holdT = std::min(holdT + dt, 1.5f);
         if (holdT >= 1.5f) {
             ResetSaveProgress();
             SaveGame();
             holdT = 0.0f;
         }
     } else if (inputReady) {
         holdT = std::max(0.0f, holdT - dt * 3.0f);
     } else {
         holdT = std::max(0.0f, holdT - dt * 4.0f);
     }

    DrawShadowedText(g_TextS, korean ? L"ESC / RMB  뒤로" : L"ESC / RMB  BACK", backX, sh - 48.0f * uiS,
                     0.44f, 0.52f, 0.62f, 0.72f, 0.58f * dA, 0.56f);

    if (exiting && exitT >= 0.42f) {
        // Settings are persisted only by SAVE CHANGES or SAVE & BACK.
        if (s_MainMenuSettingsPanel) {
            s_MainMenuSettingsPanel = false;
            s_MainMenuResumeFromPanel = true;
            g_MainMenuEntryT = 1.0f;
        } else {
            g_GameManager.currentState = GameState::PAUSED;
        }
        ResetSettingsUi();
        wasInline = false;
    }
}

void Scene_Settings(const SceneCtx& c) {
    Scene_SettingsInline(c);
}

void Scene_Ready(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    (void)c;

    auto drawSlashText = [&](const wchar_t* text, float y, float sc, float r, float g, float b, float a) {
        if (!text || !text[0]) return;
        std::vector<std::wstring> lines;
        std::wstring cur;
        for (const wchar_t* p = text; *p; ++p) {
            if (*p == L'/' && (p == text || p[-1] == L' ' || p[1] == L' ')) {
                if (!cur.empty()) lines.push_back(cur);
                cur.clear();
            }
            else cur += *p;
        }
        if (!cur.empty()) lines.push_back(cur);
        for (auto& s : lines) {
            while (!s.empty() && s.front() == L' ') s.erase(0, 1);
            while (!s.empty() && s.back() == L' ') s.pop_back();
        }
        float lineH = 30.0f * sc;
        float maxW = sw * 0.78f;
        for (int li = 0; li < (int)lines.size(); li++) {
            float lsc = sc;
            while (lsc > 0.55f && g_TextS.Width(lines[li].c_str(), lsc) > maxW) lsc -= 0.04f;
            float lw = g_TextS.Width(lines[li].c_str(), lsc);
            g_TextS.Draw(lines[li].c_str(), (sw - lw) * 0.5f, y + li * lineH, lsc, r, g, b, a);
        }
    };

    const wchar_t* title = ReadyTitle();
    g_TextL.Draw(title, CenterTextX(sw, g_TextL, title, 1.2f), sh * 0.22f, 1.2f,
                 0.55f, 0.90f, 1.0f, 0.98f);

    drawSlashText(ReadyBrief(), sh * 0.36f, 0.95f, 0.92f, 0.95f, 1.0f, 0.92f);

    const wchar_t* hint = ReadyHint();
    g_TextS.Draw(hint, CenterTextX(sw, g_TextS, hint, 0.88f), sh * 0.52f, 0.88f,
                 0.55f, 0.82f, 1.0f, 0.88f);

    const wchar_t* T1 = T(StrId::PRESS_SPACE_TO_START);
    const wchar_t* T2 = T(StrId::ESC_QUIT);
    g_TextL.Draw(T1, CenterTextX(sw, g_TextL, T1, 1.0f), sh * 0.72f, 1.0f, 1, 1, 1, 0.95f);
    g_TextS.Draw(T2, CenterTextX(sw, g_TextS, T2, 1.0f), sh * 0.78f, 1.0f, 0.8f, 0.8f, 0.8f, 0.8f);

    const wchar_t* ctrl = TutorialControls();
    g_TextS.Draw(ctrl, CenterTextX(sw, g_TextS, ctrl, 0.88f), sh * 0.88f, 0.88f,
                 0.55f, 0.85f, 1.0f, 0.95f);
}

void Scene_Paused(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    GLFWwindow* window = c.window;
    (void)*c.fireTimer;
    (void)c.reset;
    const std::function<void()>& RestartCurrentRun = c.restartRun;

    static float  s_EntryT    = 0.0f;
    static float  s_HoverT[5] = {};
    static int    s_ExitSel   = -1;
    static float  s_ExitT     = 0.0f;
    static double s_LastCall  = 0.0;

    const double curTime = glfwGetTime();
    if (curTime - s_LastCall > 0.12) {
        s_EntryT = 0.0f;
        for (int i = 0; i < 5; ++i) s_HoverT[i] = 0.0f;
        s_ExitSel = -1;
        s_ExitT   = 0.0f;
    }
    s_LastCall  = curTime;
    s_EntryT   += delta;
    if (s_ExitSel >= 0) {
        s_ExitT += delta;
        if (s_ExitT > 0.65f) s_ExitT = 0.65f;
    }

    const float entryFade  = Smoothstep(std::min(s_EntryT / 0.38f, 1.0f));
    const float fnow       = (float)curTime;
    const bool  exitActive = (s_ExitSel >= 0);
    const float exitP      = exitActive ? Smoothstep(std::min(s_ExitT / 0.45f, 1.0f)) : 0.0f;

    BatchFlush();
    glDisable(GL_SCISSOR_TEST);
    glUniformMatrix4fv(g_MainProjLoc, 1, GL_FALSE, g_BaseOrtho);
    memcpy(g_MainOrtho, g_BaseOrtho, sizeof(g_BaseOrtho));
    BindMainShader();

    // 전체 미세 딤 — 게임 일시정지 인식용
    drawRect(0, 0, sw, sh, 0.01f, 0.01f, 0.02f, 0.44f * entryFade);

    // 좌측 비네트 — 메인메뉴와 동일 스타일, 패널 없이 어둠으로 가시성 확보
    DrawSceneLeftVignette(sw, sh, 0.68f * entryFade);

    // 레이아웃 — 메인메뉴 기준 좌측 정렬
    const float BW      = std::min(520.0f, sw * 0.38f);
    const float BH      = 70.0f;
    const float BGAP    = 15.0f;
    const int   NBTN    = 5;
    const float totalBH = (float)NBTN * BH + (float)(NBTN - 1) * BGAP;
    const float btnX0   = std::max(58.0f, sw * 0.075f);
    const float btnY0   = sh * 0.42f;
    const float titleY  = sh * 0.24f;

    // 좌측 앵커 라인 + 다이아몬드 (메인메뉴와 동일)
    {
        const float anchorX = btnX0 - 36.0f;
        const float anchorY = titleY - 12.0f;
        const float anchorH = std::min(sh - anchorY - 30.0f, totalBH + 130.0f);
        const float anchorA = Smoothstep(std::min(std::max(0.0f, (s_EntryT - 0.10f) / 0.42f), 1.0f)) * entryFade;
        BindMainShader();
        drawRect(anchorX, anchorY, 1.2f, anchorH, 0.48f, 0.82f, 1.0f, 0.17f * anchorA);
        drawDiamond(anchorX + 0.6f, anchorY + 4.0f, 2.8f, 0.48f, 0.82f, 1.0f, 0.22f * anchorA);
        drawDiamond(anchorX + 0.6f, anchorY + anchorH - 4.0f, 2.8f, 0.48f, 0.82f, 1.0f, 0.18f * anchorA);
    }

    // 타이틀은 로비의 RUN/PLAY 용어와 같은 어휘를 사용한다.
    {
        float titleA = Smoothstep(std::min(s_EntryT / 0.28f, 1.0f)) * entryFade;
        const int li = std::max(0, std::min(2, LangIndex()));
        static const wchar_t* kPauseHeader[3] = {
            L"런 일시정지", L"RUN PAUSED", L"ラン一時停止"
        };
        static const wchar_t* kPauseStatus[3] = {
            L"런 상태 : 일시정지", L"RUN STATE : PAUSED", L"ラン状態 : 一時停止"
        };
        const wchar_t* hdr = kPauseHeader[li];
        float hdrSc = 1.2f;
        g_TextL.Draw(hdr, btnX0, titleY, hdrSc, 0.50f, 0.82f, 1.0f, 0.92f * titleA);
        const wchar_t* sub = kPauseStatus[li];
        g_TextS.Draw(sub, btnX0, titleY + g_TextL.Height(hdr, hdrSc) + 2.0f, 0.50f,
                     0.48f, 0.72f, 0.90f, 0.80f * titleA);
        BindMainShader();
        drawRect(btnX0, titleY + g_TextL.Height(hdr, hdrSc) + 26.0f,
                 BW * 0.52f, 1.0f, 0.30f, 0.78f, 1.0f, 0.22f * titleA);
    }

    // 버튼
    struct PBtnDef { const wchar_t* route; const wchar_t* sub; };
    static const wchar_t* kPauseRoutes[3][5] = {
        { L"재개", L"설정", L"재시작", L"\uD3EC\uAE30\uD558\uAE30", L"종료" },
        { L"RESUME", L"SETTINGS", L"RESTART", L"ABANDON RUN", L"EXIT" },
        { L"再開", L"設定", L"再起動", L"\u30E9\u30F3\u3092\u653E\u68C4", L"終了" },
    };
    static const wchar_t* kPauseSubs[3][5] = {
        { L"재개", L"설정", L"현재 런 재시작", L"현재 런 포기", L"게임 종료" },
        { L"Resume", L"Settings", L"Restart current run", L"Abandon current run", L"Exit game" },
        { L"再開", L"設定", L"現在のランを再起動", L"現在のランを放棄", L"ゲーム終了" },
    };
    const int pauseLang = std::max(0, std::min(2, LangIndex()));

    const float ar = 0.48f, ag = 0.82f, ab = 1.0f;

    for (int i = 0; i < NBTN; ++i) {
        float rawPh = (s_EntryT - 0.18f - (float)i * 0.08f) / 0.32f;
        float reveal = Smoothstep(std::max(0.0f, std::min(rawPh, 1.0f)));

        float baseBx = btnX0;
        float baseBy = btnY0 + (float)i * (BH + BGAP);
        const PBtnDef pbtn = { kPauseRoutes[pauseLang][i],
                               kPauseSubs[pauseLang][i] };
        bool hov = UpdatePanelButtonHover(s_HoverT[i], !exitActive,
                                          mx, my,
                                          baseBx, baseBy, BW, BH,
                                          // Keep adjacent pause rows distinct.  The
                                          // old 26px slop made a click near a row
                                          // boundary activate its neighbour.
                                          delta, 4.0f);
        float t = s_HoverT[i];

        bool  selected    = (s_ExitSel == i);
        float selectPulse = selected ? (0.50f + 0.50f * sinf(fnow * 18.0f)) * exitP : 0.0f;
        float activeT     = std::max(t, selected ? exitP : 0.0f);

        float rowA = reveal * entryFade;
        if (exitActive && !selected) rowA *= (1.0f - exitP * 0.86f);
        float slide = (1.0f - reveal) * 34.0f;
        float bx    = baseBx + slide - 10.0f * t;
        float by    = baseBy + (exitActive && !selected ? exitP * 28.0f : 0.0f);

        float routeSc = 1.04f;
        while (routeSc > 0.82f && g_TextL.Width(pbtn.route, routeSc) > BW)
            routeSc -= 0.04f;
        DrawPanelButton(pbtn.route, pbtn.sub, bx, by, BW, BH,
                        ar, ag, ab, rowA,
                        activeT, selected, selectPulse,
                        fnow + (float)i * 0.17f,
                        routeSc, 0.48f, false,
                        PanelButtonSlideSide::Both);

        if (hov && lmb && !g_LmbPrev && s_ExitSel < 0) {
            s_ExitSel = i;
            s_ExitT   = 0.0f;
        }
    }

    if (s_ExitSel >= 0 && s_ExitT >= 0.50f) {
        int sel   = s_ExitSel;
        s_ExitSel = -1;
        s_ExitT   = 0.0f;
        switch (sel) {
        case 0:
            g_GameManager.currentState = g_GameManager.pauseResumeState;
            g_GameManager.pauseResumeState = GameState::RUNNING;
            break;
        case 1:
            g_SettingsReturnTo = GameState::PAUSED;
            ResetSettingsUi();
            g_GameManager.currentState = GameState::SETTINGS;
            break;
        case 2:
            if (RestartCurrentRun) RestartCurrentRun();
            break;
        case 3:
            if (c.abandonRun) c.abandonRun();
            break;
        case 4: glfwSetWindowShouldClose(window, GLFW_TRUE); break;
        }
        return;
    }

    {
        float hintA = Smoothstep(std::min(std::max(0.0f, s_EntryT - 0.50f) / 0.30f, 1.0f)) * entryFade;
        const wchar_t* hint = L"[SPACE / ESC]  재개";
        g_TextS.Draw(hint, btnX0, sh * 0.88f, 0.58f,
                     0.50f, 0.70f, 0.90f, 0.72f * hintA);
    }
}
void Scene_GameOver(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    GLFWwindow* window = c.window;
    const std::function<void()>& ResetForNewGame = c.reset;
    static int s_focusedModule = -1;
    float gof = std::max(0.0f, std::min(1.0f, g_GameOverFade));
    if (gof < 0.05f) s_focusedModule = -1;
    bool skippedCinematic = false;
    if (gof < 0.999f && lmb && !g_LmbPrev) {
        // A click during the collapse is a skip request, never a menu action.
        g_GameOverFade = 1.0f;
        gof = 1.0f;
        skippedCinematic = true;
    }
    const float ge = Smoothstep(gof);
    const float now = (float)glfwGetTime();
    const int li = std::max(0, std::min(2, LangIndex()));
    const float uiS = std::max(0.72f, std::min(sw / 1640.0f, sh / 910.0f));

    static const wchar_t* kRunTitle[3] = {
        L"\uB7F0 \uC885\uB8CC", L"RUN ENDED", L"\u30E9\u30F3\u7D42\u4E86"
    };
    static const wchar_t* kDeathLabel[3] = {
        L"\uC0AC\uB9DD \uC6D0\uC778", L"DEATH SIGNAL", L"\u6B7B\u56E0"
    };
    static const wchar_t* kReportLabel[3] = {
        L"\uB7F0 \uB9AC\uD3EC\uD2B8", L"RUN REPORT", L"\u30E9\u30F3 \u30EC\u30DD\u30FC\u30C8"
    };
    static const wchar_t* kBestLabel[3] = {
        L"\uCD5C\uACE0 \uAE30\uB85D", L"BEST SCORE", L"\u30D9\u30B9\u30C8\u30B9\u30B3\u30A2"
    };
    static const wchar_t* kStardustLabel[3] = {
        L"\uD68D\uB4DD \uBCC4\uAC00\uB8E8", L"STARDUST GAINED", L"\u7372\u5F97\u30B9\u30BF\u30FC\u30C0\u30B9\u30C8"
    };
    static const wchar_t* kCoinLabel[3] = {
        L"\uD68D\uB4DD \uCF54\uC778", L"COINS EARNED", L"\u7372\u5F97\u30B3\u30A4\u30F3"
    };
    static const wchar_t* kTotalLabel[3] = {
        L"\uBCF4\uC720 \uCF54\uC778", L"TOTAL COINS", L"\u6240\u6301\u30B3\u30A4\u30F3"
    };
    static const wchar_t* kBuildLabel[3] = {
        L"\uBE4C\uB4DC \uBCC4\uC790\uB9AC", L"BUILD CONSTELLATION", L"\u30D3\u30EB\u30C9\u661F\u5EA7"
    };
    static const wchar_t* kModulesLabel[3] = {
        L"\uAE30\uB85D\uB41C \uBAA8\uB4C8", L"MODULES RECORDED", L"\u8A18\u9332\u30E2\u30B8\u30E5\u30FC\u30EB"
    };
    static const wchar_t* kNoModules[3] = {
        L"\uAE30\uB85D\uB41C \uBAA8\uB4C8 \uC5C6\uC74C", L"NO MODULES RECORDED", L"\u8A18\u9332\u30E2\u30B8\u30E5\u30FC\u306A\u3057"
    };
    static const wchar_t* kSelectedModule[3] = {
        L"\uC120\uD0DD\uB41C \uBCC4\uC790\uB9AC", L"SELECTED CONSTELLATION", L"\u9078\u629E\u3057\u305F\u661F\u5EA7"
    };
    static const wchar_t* kNewRecord[3] = {
        L"* \uC2E0\uAE30\uB85D *", L"* NEW RECORD *", L"* \u65B0\u8A18\u9332 *"
    };
    static const wchar_t* kRoute[3][3] = {
        { L"\uC7AC\uC2DC\uC791", L"\uB4A4\uB85C", L"\uC885\uB8CC" },
        { L"RESTART", L"BACK", L"EXIT" },
        { L"\u518D\u8D77\u52D5", L"\u623B\u308B", L"\u7D42\u4E86" }
    };
    static const wchar_t* kSub[3][3] = {
        { L"\uD604\uC7AC \uB7F0 \uC720\uC9C0", L"\uB85C\uBE44\uB85C", L"\uAC8C\uC784 \uC885\uB8CC" },
        { L"KEEP CURRENT RUN SETUP", L"RETURN TO LOBBY", L"EXIT GAME" },
        { L"\u73FE\u5728\u306E\u30E9\u30F3\u8A2D\u5B9A\u3092\u7DAD\u6301", L"\u30ED\u30D3\u30FC\u3078", L"\u30B2\u30FC\u30E0\u7D42\u4E86" }
    };

    // The report owns the full frame, so the build constellation is always
    // visible even when the page texture reveal was left at a low value.
    SetSceneTextureReveal(1.0f);
    BindMainShader();
    drawRect(0.0f, 0.0f, sw, sh, 0.012f, 0.026f, 0.060f, 0.52f * ge);
    DrawSceneLeftVignette(sw, sh, 0.34f * ge);
    DrawSceneRadialVignette(sw * 0.72f, sh * 0.44f,
                            std::max(sw, sh) * 0.76f, 0.12f * ge);

    const float leftX = std::max(42.0f, sw * 0.085f);
    const float leftW = std::min(680.0f, std::max(300.0f, sw * 0.49f));
    const float titleY = std::max(58.0f, sh * 0.14f);
    const float deathY = titleY + 62.0f;
    const float reportY = deathY + 58.0f;
    // Keep the metric grid below the report rule.  The previous 28px offset
    // put the first metric label directly on that rule at 1280px height.
    const float metricY = reportY + 52.0f;
    const float metricRowH = std::max(76.0f, std::min(96.0f, sh * 0.102f));
    const float metricGap = leftW * 0.52f;
    const float accentR = 0.32f, accentG = 0.82f, accentB = 1.0f;

    g_TextL.Draw(kRunTitle[li], leftX, titleY, 1.62f,
                 1.0f, 0.30f, 0.32f, 0.96f * ge);
    g_TextS.Draw(kDeathLabel[li], leftX, deathY, 0.68f,
                 0.72f, 0.76f, 0.86f, 0.82f * ge);
    if (g_DeathReason[0]) {
        float reasonScale = 0.92f;
        while (reasonScale > 0.54f &&
               g_TextS.Width(g_DeathReason, reasonScale) > leftW)
            reasonScale -= 0.04f;
        g_TextS.Draw(g_DeathReason, leftX, deathY + 25.0f, reasonScale,
                     1.0f, 0.46f, 0.43f, 0.90f * ge);
    }

    g_TextS.Draw(kReportLabel[li], leftX, reportY, 0.86f,
                 accentR, accentG, accentB, 0.92f * ge);
    LogoLine(leftX, reportY + 27.0f, leftX + leftW, reportY + 27.0f,
             1.0f, accentR, accentG, accentB, 0.20f * ge);
    if (g_LastRunRecord) {
        const float blink = 0.62f + 0.38f * sinf(now * 6.0f);
        g_TextS.Draw(kNewRecord[li], leftX + leftW - 198.0f, reportY,
                     0.62f, 1.0f, 0.84f, 0.28f, blink * ge);
    }

    float countF = std::min(1.0f, gof / 0.72f);
    countF = Smoothstep(countF);
    const long long score = (long long)(g_GameManager.score * countF);
    const long long bestValue = g_BestScore[(int)g_Difficulty];
    const long long best = g_LastRunRecord
        ? (long long)(bestValue * countF) : bestValue;
    const long long stardust = (long long)(g_RunStardust * countF);
    const long long coins = (long long)(g_LastRunCoins * countF);

    auto drawMetric = [&](const wchar_t* label, long long value,
                          float x, float y, float valueScale,
                          float rr, float gg, float bb) {
        g_TextS.Draw(label, x, y, 0.68f,
                     0.64f, 0.76f, 0.88f, 0.88f * ge);
        wchar_t valueBuf[64];
        swprintf_s(valueBuf, L"%lld", value);
        g_TextL.Draw(valueBuf, x, y + 30.0f, valueScale,
                     rr, gg, bb, 0.94f * ge);
    };

    drawMetric(T(StrId::FINAL_SCORE), score, leftX, metricY, 1.08f,
               1.0f, 0.94f, 0.58f);
    drawMetric(kBestLabel[li], best, leftX + metricGap, metricY, 0.98f,
               1.0f, 0.78f, 0.34f);
    drawMetric(T(StrId::REACHED_LEVEL), g_GameManager.playerLevel,
               leftX, metricY + metricRowH, 0.90f,
               0.68f, 0.92f, 1.0f);
    drawMetric(T(StrId::KILL_COUNT), (long long)g_Stats.killCount,
               leftX + metricGap, metricY + metricRowH, 0.90f,
               0.70f, 1.0f, 0.78f);
    drawMetric(kStardustLabel[li], stardust, leftX,
               metricY + metricRowH * 2.0f, 0.90f,
               0.72f, 0.88f, 1.0f);
    drawMetric(kCoinLabel[li], coins, leftX + metricGap,
               metricY + metricRowH * 2.0f, 0.90f,
               1.0f, 0.86f, 0.30f);
    wchar_t totalCoinBuf[64];
    swprintf_s(totalCoinBuf, L"%ls  %lld", kTotalLabel[li], g_Coins);
    g_TextS.Draw(totalCoinBuf, leftX + metricGap,
                 metricY + metricRowH * 3.0f - 12.0f, 0.50f,
                 0.55f, 0.62f, 0.72f, 0.72f * ge);

    // The owned augment list becomes a compact, rarity-colored constellation.
    const float chartCX = sw * 0.76f;
    const float chartCY = sh * 0.43f;
    const float chartR = std::max(88.0f, std::min(sw * 0.18f, sh * 0.25f));
    const float chartA = Smoothstep(std::min(1.0f,
        std::max(0.0f, (gof - 0.12f) / 0.64f)));
    const float pi = 3.1415927f;
    const float chartSpin = now * 0.075f;

    DrawSceneRadialVignette(chartCX, chartCY, chartR * 1.70f,
                            0.24f * chartA);
    DrawConstellationDisc(chartCX, chartCY, chartR * 1.18f,
                          0.0f, 0.0f, 0.012f, 0.32f * chartA);
    DrawSettingsOrbitArc(chartCX, chartCY, chartR, chartR * 0.68f,
                         chartSpin - pi * 0.92f, chartSpin + pi * 0.92f,
                         0.28f, 0.72f, 1.0f, 0.72f * uiS,
                         0.24f * chartA);
    DrawSettingsOrbitArc(chartCX, chartCY, chartR * 0.70f, chartR * 0.44f,
                         -chartSpin + 0.24f, -chartSpin + pi * 1.76f,
                         0.30f, 0.62f, 0.92f, 0.52f * uiS,
                         0.14f * chartA);

    // Keep the selected character as the constellation's identity anchor.
    // Job icons are the same visual designs used on the loadout screen.
    const int playerJob = IsPlayableJob(g_SelectedJob)
                        ? g_SelectedJob : JOB_NONE;
    const GLuint playerIcon = JobIcon(playerJob);
    const float playerCoreR = chartR * 0.25f;
    DrawConstellationDisc(chartCX, chartCY, playerCoreR * 1.30f,
                          0.015f, 0.055f, 0.095f, 0.58f * chartA);
    DrawSettingsOrbitArc(chartCX, chartCY, playerCoreR * 1.18f,
                         playerCoreR * 0.82f, chartSpin,
                         chartSpin + pi * 2.0f,
                         0.44f, 0.86f, 1.0f, 0.85f * uiS,
                         0.42f * chartA);
    if (playerIcon) {
        BindMainShader();
        DrawIcon(playerIcon, chartCX - playerCoreR, chartCY - playerCoreR,
                 playerCoreR * 2.0f, playerCoreR * 2.0f,
                 1.0f, 1.0f, 1.0f, 0.96f * chartA);
    } else {
        BindMainShader();
        DrawPlayerWeaponShell(chartCX, chartCY,
                              std::max(12.0f, chartR * 0.075f),
                              chartSpin + pi * 0.5f);
    }

    const wchar_t* chartTitle = kBuildLabel[li];
    g_TextS.Draw(chartTitle,
                 chartCX - g_TextS.Width(chartTitle, 0.66f) * 0.5f,
                 chartCY - chartR * 1.34f, 0.66f,
                 accentR, accentG, accentB, 0.94f * chartA);

    std::vector<int> modules;
    modules.reserve(64);
    for (int idx : g_OwnedAugs) {
        if (idx >= 0 && idx < AUG_TOTAL && modules.size() < 64)
            modules.push_back(idx);
    }
    const int moduleCount = (int)modules.size();
    const int visibleModules = std::min(moduleCount, 8);
    if (s_focusedModule < 0 || s_focusedModule >= visibleModules)
        s_focusedModule = -1;
    wchar_t moduleCountBuf[64];
    swprintf_s(moduleCountBuf, L"%ls  %d", kModulesLabel[li], moduleCount);
    g_TextS.Draw(moduleCountBuf,
                 chartCX - g_TextS.Width(moduleCountBuf, 0.42f) * 0.5f,
                 chartCY + chartR * 1.22f, 0.48f,
                 0.58f, 0.68f, 0.80f, 0.76f * chartA);

    int hoveredModule = -1;
    if (visibleModules > 0) {
        float px[8] = {}, py[8] = {};
        const float kAngles[8] = {
            -1.56f, -0.83f, -0.12f, 0.58f,
             1.42f,  2.18f,  2.88f, 3.72f
        };
        const float kRadii[8] = {
            0.62f, 0.78f, 0.56f, 0.76f,
            0.66f, 0.82f, 0.55f, 0.74f
        };
        for (int i = 0; i < visibleModules; ++i) {
            const float angle = chartSpin * 0.72f + kAngles[i];
            const float radius = chartR * kRadii[i];
            px[i] = chartCX + cosf(angle) * radius;
            py[i] = chartCY + sinf(angle) * radius * (0.64f + 0.04f * (i & 1));
        }

        const bool allowConstellationInput = gof >= 0.999f && !skippedCinematic;
        if (allowConstellationInput) {
            const float hitR = std::max(15.0f, 13.0f * uiS);
            for (int i = 0; i < visibleModules; ++i) {
                const float dx = (float)mx - px[i];
                const float dy = (float)my - py[i];
                if (dx * dx + dy * dy <= hitR * hitR) {
                    hoveredModule = i;
                    break;
                }
            }
            if (hoveredModule >= 0 && lmb && !g_LmbPrev)
                s_focusedModule = hoveredModule;
        }
        const int detailModule = hoveredModule >= 0
                               ? hoveredModule : s_focusedModule;

        // Avoid a regular polygon: the uneven ring and selective chords make
        // each build read as a different constellation instead of a wheel.
        for (int i = 0; i < visibleModules; ++i) {
            const int next = (i + 1) % visibleModules;
            float rr, gg, bb;
            GetRarityColor(ALL_AUGS[modules[i]].rarity, rr, gg, bb);
            DrawVisibleConstellLine(px[i], py[i], px[next], py[next],
                                    1.35f * uiS, rr, gg, bb,
                                    0.62f * chartA);
        }
        const int chordPairs[][2] = {
            { 0, 3 }, { 2, 5 }, { 4, 7 }, { 6, 1 }
        };
        const int chordCount = (visibleModules >= 6) ? 4
                             : (visibleModules >= 4) ? 2 : 0;
        for (int i = 0; i < chordCount; ++i) {
            const int a = chordPairs[i][0] % visibleModules;
            const int b = chordPairs[i][1] % visibleModules;
            float rr, gg, bb;
            GetRarityColor(ALL_AUGS[modules[a]].rarity, rr, gg, bb);
            DrawVisibleConstellLine(px[a], py[a], px[b], py[b],
                                    0.85f * uiS, rr, gg, bb,
                                    0.30f * chartA);
        }
        for (int i = 0; i < visibleModules; ++i) {
            float rr, gg, bb;
            GetRarityColor(ALL_AUGS[modules[i]].rarity, rr, gg, bb);
            const bool focused = i == detailModule;
            DrawVisibleConstellNode(px[i], py[i],
                                    (focused ? 9.0f : 7.0f) * uiS,
                                    rr, gg, bb, chartA, focused, true);
            if (focused) {
                DrawSettingsOrbitArc(px[i], py[i], 15.0f * uiS,
                                     9.0f * uiS, chartSpin,
                                     chartSpin + pi * 2.0f,
                                     rr, gg, bb, 0.85f * uiS,
                                     0.72f * chartA);
            }
        }
        const int overflow = moduleCount - visibleModules;
        if (overflow > 0) {
            wchar_t moreBuf[48];
            swprintf_s(moreBuf, L"+%d MORE", overflow);
            g_TextS.Draw(moreBuf,
                         chartCX + chartR * 0.48f,
                         chartCY + chartR * 1.22f, 0.44f,
                         0.62f, 0.76f, 0.90f, 0.82f * chartA);
        }

        if (detailModule >= 0 && detailModule < visibleModules) {
            const AugDef& selected = ALL_AUGS[modules[detailModule]];
            const wchar_t* moduleName = selected.locName[li];
            const wchar_t* moduleDesc = selected.locDesc[li];
            const float detailY = chartCY + chartR * 0.82f;
            const float detailW = chartR * 1.92f;
            LogoLine(chartCX - detailW * 0.5f, detailY - 10.0f,
                     chartCX + detailW * 0.5f, detailY - 10.0f,
                     0.8f, accentR, accentG, accentB, 0.20f * chartA);
            g_TextS.Draw(kSelectedModule[li],
                         chartCX - g_TextS.Width(kSelectedModule[li], 0.48f) * 0.5f,
                         detailY, 0.48f,
                         accentR, accentG, accentB, 0.88f * chartA);
            float nameScale = 0.68f;
            while (nameScale > 0.48f &&
                   g_TextL.Width(moduleName, nameScale) > detailW)
                nameScale -= 0.04f;
            g_TextL.Draw(moduleName,
                         chartCX - g_TextL.Width(moduleName, nameScale) * 0.5f,
                         detailY + 22.0f, nameScale,
                         0.92f, 0.98f, 1.0f, 0.94f * chartA);
            if (moduleDesc && moduleDesc[0]) {
                float descScale = 0.42f;
                while (descScale > 0.30f &&
                       g_TextS.Width(moduleDesc, descScale) > detailW)
                    descScale -= 0.03f;
                g_TextS.Draw(moduleDesc,
                             chartCX - g_TextS.Width(moduleDesc, descScale) * 0.5f,
                             detailY + 53.0f, descScale,
                             0.62f, 0.74f, 0.84f, 0.78f * chartA);
            }
        }
    } else {
        g_TextS.Draw(kNoModules[li],
                     chartCX - g_TextS.Width(kNoModules[li], 0.44f) * 0.5f,
                     chartCY + chartR * 0.78f, 0.44f,
                     0.62f, 0.76f, 0.90f, 0.82f * chartA);
    }

    static float s_hovGO[3] = {};
    const float BW = std::min(520.0f, std::max(280.0f, sw * 0.38f));
    const float BH = std::max(48.0f, std::min(68.0f, sh * 0.075f));
    const float BGAP = std::max(10.0f, sh * 0.013f);
    const float totalBH = 3.0f * BH + 2.0f * BGAP;
    const float bX0 = leftX;
    // Reserve the full metric block, including the total-coin line.  On
    // compact windows the old three-row estimate put the first command on
    // top of that line and produced the QA overlap seen in GAMEOVER.
    const float statsBottom = metricY + metricRowH * 3.0f + 18.0f;
    const float bY0 = std::max(statsBottom + 30.0f,
                               sh - totalBH - std::max(44.0f, sh * 0.06f));
    const bool showButtons = gof >= 0.999f && !skippedCinematic;

    if (showButtons) {
        const float ancX = bX0 - 36.0f;
        const float ancY = bY0 - 8.0f;
        const float ancH = totalBH + 16.0f;
        BindMainShader();
        drawRect(ancX, ancY, 1.2f, ancH, accentR, accentG, accentB,
                 0.15f * ge);
        drawDiamond(ancX + 0.6f, ancY + 4.0f, 2.8f,
                    accentR, accentG, accentB, 0.20f * ge);
        drawDiamond(ancX + 0.6f, ancY + ancH - 4.0f, 2.8f,
                    accentR, accentG, accentB, 0.16f * ge);
    }
    for (int i = 0; i < 3; ++i) {
        const float by = bY0 + (float)i * (BH + BGAP);
        const bool hov = showButtons &&
            mx >= bX0 - 26.0f && mx <= bX0 + BW + 32.0f &&
            my >= by && my <= by + BH;
        s_hovGO[i] = UpdateMenuCommandHover(s_hovGO[i], hov, delta);
        const float rowA = (showButtons ? 1.0f : 0.0f) * ge;
        DrawUnifiedMenuCommand(kRoute[li][i], kSub[li][i],
                               bX0 - 10.0f * s_hovGO[i], by, BW, BH,
                               accentR, accentG, accentB, rowA,
                               s_hovGO[i], false, 0.0f,
                               now + (float)i * 0.17f);
        if (hov && lmb && !g_LmbPrev) {
            switch (i) {
            case 0:
                ResetForNewGame();
                FinalizeLoadout(c, FixedWeaponForSelectedJob());
                break;
            case 1:
                g_GameManager.currentState = GameState::MAIN_MENU;
                break;
            case 2:
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                break;
            }
        }
    }
}

void Scene_Victory(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    GLFWwindow* window = c.window;
    const std::function<void()>& ResetForNewGame = c.reset;

    float vf = g_VictoryFade;
    float ve = Smoothstep(vf);

    BindMainShader();
    drawRect(0, 0, sw, sh, 0.02f, 0.06f, 0.04f, 0.88f * ve);

    static const wchar_t* T1[3] = { L"런 클리어!", L"RUN CLEARED!", L"ランクリア!" };
    int li = LangIndex();
    if (li < 0 || li > 2) li = 0;
    g_TextL.Draw(T1[li], CenterTextX(sw, g_TextL, T1[li], 1.7f), sh * 0.26f, 1.7f,
                 0.35f, 1.0f, 0.55f, 0.98f * ve);

    wchar_t scoreBuf[64], lvBuf[64], killBuf[64], coinBuf[64];
    swprintf_s(scoreBuf, L"%ls   %lld", T(StrId::FINAL_SCORE), g_GameManager.score);
    swprintf_s(lvBuf,   L"%ls   Lv. %d", T(StrId::REACHED_LEVEL), g_GameManager.playerLevel);
    swprintf_s(killBuf, L"%ls   %lld", T(StrId::KILL_COUNT), g_Stats.killCount);
    swprintf_s(coinBuf, L"+%lld COIN  (total %lld)", g_LastRunCoins, g_Coins);
    g_TextL.Draw(scoreBuf, CenterTextX(sw, g_TextL, scoreBuf, 1.25f), sh * 0.46f, 1.25f,
                 1.0f, 1.0f, 0.85f, 0.95f * ve);
    g_TextS.Draw(lvBuf,   CenterTextX(sw, g_TextS, lvBuf, 1.0f), sh * 0.54f, 1.0f,
                 0.85f, 0.95f, 0.85f, 0.95f * ve);
    g_TextS.Draw(killBuf, CenterTextX(sw, g_TextS, killBuf, 1.0f), sh * 0.59f, 1.0f,
                 0.85f, 0.95f, 0.85f, 0.95f * ve);
    g_TextS.Draw(coinBuf, CenterTextX(sw, g_TextS, coinBuf, 1.0f), sh * 0.64f, 1.0f,
                 1.0f, 0.9f, 0.35f, 0.95f * ve);

    // 버튼 3개 — 메인메뉴/일시정지와 동일한 별자리 좌정렬 스타일
    {
        static float s_hovVC[3] = {};
        const float BW  = std::min(480.0f, sw * 0.36f);
        const float BH  = 68.0f, BGAP = 12.0f;
        const float bX0 = std::max(58.0f, sw * 0.075f);
        const float bY0 = sh * 0.715f;
        const float ar  = 0.38f, ag = 1.0f, ab = 0.62f;
        const float fnow = (float)glfwGetTime();
        const float showT = vf >= 0.999f ? 1.0f : 0.0f;

        static const wchar_t* kRoute[3] = { L"RESTART",      L"RETURN_TO_HUB", L"TERMINATE" };
        const wchar_t*        kSub[3]   = { T(StrId::BTN_RESTART), T(StrId::BTN_MAIN_MENU), T(StrId::BTN_QUIT) };

        if (showT > 0.0f) {
            const float ancX = bX0 - 36.0f, ancY = bY0 - 8.0f;
            const float ancH = (float)(3 - 1) * (BH + BGAP) + BH + 16.0f;
            BindMainShader();
            drawRect(ancX, ancY, 1.2f, ancH, ar,ag,ab, 0.15f * ve);
            drawDiamond(ancX + 0.6f, ancY + 4.0f,        2.8f, ar,ag,ab, 0.20f * ve);
            drawDiamond(ancX + 0.6f, ancY + ancH - 4.0f, 2.8f, ar,ag,ab, 0.16f * ve);
        }

        for (int i = 0; i < 3; ++i) {
            float by2 = bY0 + (float)i * (BH + BGAP);
            bool hov = (showT > 0.0f &&
                        mx >= bX0 - 26.0f && mx <= bX0 + BW + 32.0f &&
                        my >= by2 && my <= by2 + BH);
            s_hovVC[i] = UpdateMenuCommandHover(s_hovVC[i], hov, delta);
            float t    = s_hovVC[i];
            float rowA = showT * ve;
            float bx2  = bX0 - 10.0f * t;

            DrawUnifiedMenuCommand(kRoute[i], kSub[i], bx2, by2, BW, BH,
                                   ar, ag, ab, rowA, t, false, 0.0f,
                                   fnow + (float)i * 0.17f);

            if (hov && lmb && !g_LmbPrev) {
                switch (i) {
                case 0: ResetForNewGame(); FinalizeLoadout(c, FixedWeaponForSelectedJob()); break;
                case 1: g_GameManager.currentState = GameState::MAIN_MENU; break;
                case 2: glfwSetWindowShouldClose(window, GLFW_TRUE); break;
                }
            }
        }
    }
}

struct TarotCardPose {
    float cx = 0.0f;
    float cy = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    float angle = 0.0f;
};

static float TarotClamp01(float v) {
    return std::max(0.0f, std::min(1.0f, v));
}

static float TarotEaseOut(float t) {
    const float inv = 1.0f - TarotClamp01(t);
    return 1.0f - inv * inv * inv;
}

static TarotCardPose TarotBasePose(int index, int count, float sw, float sh,
                                   float cardW, float cardH, float centerX,
                                   float centerY) {
    TarotCardPose p;
    const float middle = (float)(count - 1) * 0.5f;
    const float gap = count <= 3 ? cardW * 0.14f : cardW * 0.08f;
    const float spread = cardW + gap;
    const float offset = (float)index - middle;
    p.cx = centerX + offset * spread;
    p.cy = centerY;
    p.w = cardW;
    p.h = cardH;
    p.angle = 0.0f;
    (void)sw;
    (void)sh;
    return p;
}

static void TarotPoint(const TarotCardPose& p, float lx, float ly,
                       float& x, float& y) {
    const float cs = cosf(p.angle);
    const float sn = sinf(p.angle);
    x = p.cx + lx * cs - ly * sn;
    y = p.cy + lx * sn + ly * cs;
}

static void TarotFill(const TarotCardPose& p, float w, float h,
                      float r, float g, float b, float a) {
    float x0, y0, x1, y1, x2, y2, x3, y3;
    TarotPoint(p, -w * 0.5f, -h * 0.5f, x0, y0);
    TarotPoint(p,  w * 0.5f, -h * 0.5f, x1, y1);
    TarotPoint(p,  w * 0.5f,  h * 0.5f, x2, y2);
    TarotPoint(p, -w * 0.5f,  h * 0.5f, x3, y3);
    BatchTri(x0, y0, x1, y1, x2, y2, r, g, b, a);
    BatchTri(x0, y0, x2, y2, x3, y3, r, g, b, a);
}

static void TarotSegment(float x1, float y1, float x2, float y2, float width,
                         float r, float g, float b, float a) {
    const float dx = x2 - x1;
    const float dy = y2 - y1;
    const float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.5f) return;
    const float nx = -dy / len * width * 0.5f;
    const float ny =  dx / len * width * 0.5f;
    BatchTri(x1 + nx, y1 + ny, x1 - nx, y1 - ny,
             x2 - nx, y2 - ny, r, g, b, a);
    BatchTri(x1 + nx, y1 + ny, x2 - nx, y2 - ny,
             x2 + nx, y2 + ny, r, g, b, a);
}

static void TarotStroke(const TarotCardPose& p, float w, float h, float width,
                        float r, float g, float b, float a) {
    float x0, y0, x1, y1, x2, y2, x3, y3;
    TarotPoint(p, -w * 0.5f, -h * 0.5f, x0, y0);
    TarotPoint(p,  w * 0.5f, -h * 0.5f, x1, y1);
    TarotPoint(p,  w * 0.5f,  h * 0.5f, x2, y2);
    TarotPoint(p, -w * 0.5f,  h * 0.5f, x3, y3);
    TarotSegment(x0, y0, x1, y1, width, r, g, b, a);
    TarotSegment(x1, y1, x2, y2, width, r, g, b, a);
    TarotSegment(x2, y2, x3, y3, width, r, g, b, a);
    TarotSegment(x3, y3, x0, y0, width, r, g, b, a);
}

static bool TarotHit(const TarotCardPose& p, double mx, double my, float pad) {
    const float dx = (float)mx - p.cx;
    const float dy = (float)my - p.cy;
    const float cs = cosf(p.angle);
    const float sn = sinf(p.angle);
    const float lx = dx * cs + dy * sn;
    const float ly = -dx * sn + dy * cs;
    return fabsf(lx) <= p.w * 0.5f + pad &&
           fabsf(ly) <= p.h * 0.5f + pad;
}

static void DrawAugmentConstellation(const AugDef& def, float cx, float cy,
                                     float radius, float r, float g, float b,
                                     float alpha, float now, float uiS) {
    const AugListGroup group = AugListGroupOf(def);
    int category = 1;
    switch (group) {
    case AugListGroup::SKILL:
    case AugListGroup::WEAPON:
        category = 2;
        break;
    case AugListGroup::ORBIT:
    case AugListGroup::COMBO:
    case AugListGroup::MYTHIC:
    case AugListGroup::SPECIAL:
    case AugListGroup::DEBUFF:
        category = 3;
        break;
    default:
        category = 1;
        break;
    }

    // Reuse the game's constellation language, but seed each augment with its
    // own type so two cards never look like a generic repeated icon.
    DrawArchiveConstellation(cx, cy, radius, category,
                              static_cast<int>(def.type), now,
                              r, g, b, alpha, uiS, true);
}

static void DrawAugmentOrbitRing(float cx, float cy, float rx, float ry,
                                 float phase, float r, float g, float b,
                                 float alpha, float uiS,
                                 float lineScale = 1.0f,
                                 float glowScale = 0.0f) {
    if (alpha <= 0.001f || rx <= 2.0f || ry <= 2.0f) return;
    constexpr int kSegments = 48;
    float prevX = cx + cosf(phase) * rx;
    float prevY = cy + sinf(phase) * ry;
    for (int i = 1; i <= kSegments; ++i) {
        const float a = phase + 2.0f * (float)M_PI *
                        (float)i / (float)kSegments;
            const float x = cx + cosf(a) * rx;
            const float y = cy + sinf(a) * ry;
            if ((i % 8) != 0) {
                if (glowScale > 0.001f) {
                    DrawVisibleConstellLine(
                        prevX, prevY, x, y,
                        2.40f * uiS * glowScale,
                        r, g, b, alpha * 0.16f);
                }
                DrawVisibleConstellLine(prevX, prevY, x, y,
                                        0.72f * uiS * lineScale,
                                        r, g, b, alpha);
        }
        prevX = x;
        prevY = y;
    }
    for (int i = 0; i < 4; ++i) {
        const float a = phase + (float)i * (float)M_PI * 0.5f;
        DrawVisibleConstellNode(cx + cosf(a) * rx,
                                cy + sinf(a) * ry,
                                2.8f * uiS * lineScale, r, g, b,
                                alpha * 0.76f, false, true);
    }
}

static void DrawAugmentShieldOrbit(float cx, float cy, float radius,
                                   float phase, float satellitePhase,
                                   float r, float g, float b,
                                   float alpha, float pulse, float uiS) {
    if (alpha <= 0.001f || radius <= 2.0f) return;

    constexpr int kSegments = 64;
    for (int ring = 0; ring < 2; ++ring) {
        const float rx = radius * (ring == 0 ? 1.58f : 1.26f);
        const float ry = radius * (ring == 0 ? 0.84f : 0.64f);
        const float ringPhase = phase + (ring == 0 ? 0.0f : 0.24f);
        const int gapEvery = ring == 0 ? 9 : 11;
        const float thickness = (0.82f + pulse * 0.66f) * uiS;
        const float ringAlpha = alpha * (ring == 0 ? 0.82f : 0.58f);

        float prevX = cx + cosf(ringPhase) * rx;
        float prevY = cy + sinf(ringPhase) * ry;
        for (int i = 1; i <= kSegments; ++i) {
            const float a = ringPhase + 2.0f * (float)M_PI *
                            (float)i / (float)kSegments;
            const float x = cx + cosf(a) * rx;
            const float y = cy + sinf(a) * ry;
            if ((i % gapEvery) != 0) {
                DrawVisibleConstellLine(prevX, prevY, x, y,
                                        thickness, r, g, b, ringAlpha);
            }
            prevX = x;
            prevY = y;
        }
    }

    const float satelliteRx = radius * 1.58f;
    const float satelliteRy = radius * 0.84f;
    const float satelliteX = cx + cosf(satellitePhase) * satelliteRx;
    const float satelliteY = cy + sinf(satellitePhase) * satelliteRy;
    const float satelliteSize = (3.8f + pulse * 2.2f) * uiS;
    DrawConstellationDisc(satelliteX, satelliteY,
                          satelliteSize * (3.0f + pulse * 1.6f),
                          r, g, b, alpha * (0.14f + pulse * 0.16f));
    DrawVisibleConstellNode(satelliteX, satelliteY, satelliteSize,
                            r, g, b, alpha * (0.72f + pulse * 0.24f),
                            true, true);
}

static void DrawAugmentCylinderField(float x, float y, float w, float h,
                                     float r, float g, float b, float alpha,
                                     float reveal, float phase) {
    if (alpha <= 0.001f || w <= 8.0f || h <= 8.0f) return;
    reveal = TarotClamp01(reveal);
    const float midX = x + w * 0.50f;
    const float midY = y + h * 0.52f;
    const float fieldA = alpha * (0.72f + reveal * 0.28f);

    BindMainShader();
    drawRect(x + 12.0f, y + 14.0f, w, h,
             0.0f, 0.0f, 0.018f, alpha * 0.36f);
    drawRect(x, y, w, h,
             0.006f, 0.010f, 0.028f, alpha * 0.74f);
    drawRect(x + w * 0.08f, y + h * 0.08f,
             w * 0.84f, h * 0.84f,
             r * 0.018f, g * 0.022f, b * 0.034f,
             alpha * 0.20f);
    BatchFlush();

    // Curved side rails are the silhouette of the rotating cylinder. They
    // stay dark at the edges and expose more of the colored field as reveal
    // advances, matching the "촤라락" opening in the reference sketch.
    const int railSteps = 36;
    const float leftRail = x + w * 0.15f;
    const float rightRail = x + w * 0.85f;
    float prevLX = leftRail, prevRX = rightRail;
    float prevY = y;
    for (int i = 1; i <= railSteps; ++i) {
        const float t = (float)i / (float)railSteps;
        const float yy = y + h * t;
        const float curve = sinf(t * (float)M_PI) * w * 0.065f;
        const float lx = leftRail + curve;
        const float rx = rightRail - curve;
        const float railReveal = TarotClamp01((reveal - t * 0.18f) / 0.82f);
        DrawVisibleConstellLine(prevLX, prevY, lx, yy,
                                2.4f, 0.0f, 0.0f, 0.012f,
                                alpha * (0.62f + 0.24f * railReveal));
        DrawVisibleConstellLine(prevRX, prevY, rx, yy,
                                2.4f, 0.0f, 0.0f, 0.012f,
                                alpha * (0.62f + 0.24f * railReveal));
        DrawVisibleConstellLine(prevLX + 3.5f, prevY, lx + 3.5f, yy,
                                0.85f, r, g, b, alpha * 0.18f * railReveal);
        DrawVisibleConstellLine(prevRX - 3.5f, prevY, rx - 3.5f, yy,
                                0.85f, r, g, b, alpha * 0.18f * railReveal);
        prevLX = lx; prevRX = rx; prevY = yy;
    }

    // Elliptical orbit bands establish the cylinder depth. The phase is
    // intentionally slow; the field should feel alive without rotating the
    // labels or making the choices hard to read.
    const int orbitSteps = 48;
    for (int ring = 0; ring < 3; ++ring) {
        const float rx = w * (0.22f + 0.075f * (float)ring);
        const float ry = h * (0.16f + 0.065f * (float)ring);
        const float ringPhase = phase * (ring == 1 ? -0.65f : 0.42f)
                              + (float)ring * 0.78f;
        float prevX = midX + cosf(ringPhase) * rx;
        float prevY2 = midY + sinf(ringPhase) * ry;
        for (int i = 1; i <= orbitSteps; ++i) {
            const float a = ringPhase + 2.0f * (float)M_PI *
                            (float)i / (float)orbitSteps;
            const float nx = midX + cosf(a) * rx;
            const float ny = midY + sinf(a) * ry;
            const bool gap = ((i + ring * 3) % 7) == 0;
            if (!gap) {
                DrawVisibleConstellLine(prevX, prevY2, nx, ny,
                                        0.62f + 0.18f * (float)ring,
                                        r, g, b,
                                        alpha * (0.12f - 0.018f * (float)ring));
            }
            prevX = nx; prevY2 = ny;
        }
    }

    const float sweep = phase * 0.8f;
    for (int i = 0; i < 5; ++i) {
        const float a = sweep + (float)i * 1.256637f;
        const float px = midX + cosf(a) * w * 0.34f;
        const float py = midY + sinf(a) * h * 0.37f;
        DrawVisibleConstellNode(px, py, 2.0f + (i & 1),
                                r, g, b,
                                alpha * (0.24f + 0.08f *
                                         sinf(phase * 2.0f + (float)i)),
                                false, true);
    }
    DrawConstellationDisc(midX, midY, h * 0.15f,
                          r, g, b, alpha * 0.10f);
    DrawVisibleConstellNode(midX, midY, h * 0.035f,
                            r, g, b, fieldA * 0.82f, false, true);
    BatchFlush();
}

static void DrawTarotEmblem(const AugDef& def, float cx, float cy, float size,
                            float r, float g, float b, float a, float now) {
    const AugListGroup group = AugListGroupOf(def);
    const float pulse = 1.0f + sinf(now * 2.7f) * 0.035f;
    const float outer = size * 0.50f * pulse;
    const float inner = size * 0.34f;
    drawCircle(cx, cy, outer, r * 0.18f, g * 0.18f, b * 0.18f, a * 0.56f);
    TarotSegment(cx - outer, cy, cx + outer, cy, 1.0f, r, g, b, a * 0.34f);
    TarotSegment(cx, cy - outer, cx, cy + outer, 1.0f, r, g, b, a * 0.34f);

    if (def.rarity == AugRarity::COMBO) {
        drawDiamond(cx - size * 0.20f, cy, size * 0.38f, r, g, b, a * 0.82f);
        drawDiamond(cx + size * 0.20f, cy, size * 0.38f, r, g, b, a * 0.82f);
        TarotSegment(cx - size * 0.10f, cy, cx + size * 0.10f, cy,
                     2.5f, r, g, b, a);
    } else if (def.rarity == AugRarity::DEBUFF) {
        const float q = inner * 0.92f;
        BatchTri(cx, cy + q, cx - q, cy - q * 0.72f,
                 cx + q, cy - q * 0.72f, r, g, b, a * 0.78f);
        drawDiamond(cx, cy - size * 0.04f, size * 0.28f,
                    1.0f, 0.28f, 0.28f, a);
    } else {
        const int points = (group == AugListGroup::SKILL ||
                            group == AugListGroup::COMBO) ? 6 : 4;
        for (int i = 0; i < points; ++i) {
            const float ang = -M_PI * 0.5f + (float)i * 2.0f * M_PI / points;
            const float x = cx + cosf(ang) * inner;
            const float y = cy + sinf(ang) * inner;
            TarotSegment(cx, cy, x, y, 2.0f, r, g, b, a * 0.72f);
            drawDiamond(x, y, size * 0.14f, r, g, b, a * 0.90f);
        }
        if (group == AugListGroup::ORBIT || group == AugListGroup::MYTHIC)
            drawCircle(cx, cy, inner * 0.52f, r, g, b, a * 0.60f);
        drawDiamond(cx, cy, size * 0.30f, r, g, b, a);
    }
    BatchFlush();
}

static void DrawTarotCardSurface(const TarotCardPose& p, float r, float g,
                                 float b, float alpha, float hover,
                                 float flash, bool selected, float now) {
    // Historical augment cards were square to the screen: a dark body, a
    // rarity-colored title bar, full neon border, and a small constellation
    // corner frame. Keep the pose parameter so the animation code can still
    // own movement/scale, but render the card in this deliberately flat style.
    const float left = p.cx - p.w * 0.5f;
    const float top = p.cy - p.h * 0.5f;
    const float topBar = std::max(38.0f, p.w * 0.16f);
    const float inner = std::max(4.0f, p.w * 0.018f);
    const float shimmer = 0.15f + 0.10f * (sinf(now * 3.2f) * 0.5f + 0.5f);
    const float glow = 0.78f + hover * 0.26f + flash * 0.25f;

    BindMainShader();
    drawRect(left + 8.0f, top + 10.0f, p.w, p.h,
             0.0f, 0.0f, 0.02f, alpha * 0.44f);
    drawRect(left, top, p.w, p.h,
             0.018f, 0.024f, 0.052f, alpha * 0.98f);
    drawRect(left + inner, top + inner, p.w - inner * 2.0f,
             p.h - inner * 2.0f,
             0.035f + r * 0.06f, 0.045f + g * 0.04f,
             0.082f + b * 0.04f, alpha * 0.92f);
    drawRect(left, top, p.w, topBar,
             r * 0.38f, g * 0.38f, b * 0.38f,
             alpha * (0.46f + hover * 0.16f + flash * 0.12f));

    const float borderR = std::min(1.0f, r * (1.4f + shimmer) + flash * 0.55f);
    const float borderG = std::min(1.0f, g * (1.4f + shimmer) + flash * 0.55f);
    const float borderB = std::min(1.0f, b * (1.4f + shimmer) + flash * 0.55f);
    const float borderA = alpha * glow;
    drawRect(left, top, p.w, 1.8f, borderR, borderG, borderB, borderA);
    drawRect(left, top + p.h - 1.8f, p.w, 1.8f,
             borderR, borderG, borderB, borderA);
    drawRect(left, top, 1.8f, p.h, borderR, borderG, borderB, borderA);
    drawRect(left + p.w - 1.8f, top, 1.8f, p.h,
             borderR, borderG, borderB, borderA);
    drawConstellFrame(left + inner, top + inner, p.w - inner * 2.0f,
                      p.h - inner * 2.0f, r, g, b,
                      alpha * (0.26f + hover * 0.24f),
                      14.0f, 4.0f, alpha * 0.12f);

    // The old card reserves the upper half for the icon. This matches the
    // icon slot used by the content pass below, so the divider never cuts
    // through the glyph at smaller window scales.
    const float dividerY = top + topBar + p.w * 0.54f;
    drawRect(left + p.w * 0.10f, dividerY,
             p.w * 0.80f, 1.5f, r, g, b,
             alpha * (0.26f + hover * 0.18f));
    drawRect(left + p.w * 0.10f, top + p.h - p.w * 0.24f,
             p.w * 0.80f, 1.0f, r, g, b, alpha * 0.18f);

    drawDiamond(left + 10.0f, top + 10.0f, 6.0f + hover * 2.0f,
                borderR, borderG, borderB, alpha * 0.88f);
    drawDiamond(left + p.w - 10.0f, top + 10.0f,
                6.0f + hover * 2.0f, borderR, borderG, borderB,
                alpha * 0.88f);
    drawDiamond(left + 10.0f, top + p.h - 10.0f, 4.5f,
                r, g, b, alpha * 0.52f);
    drawDiamond(left + p.w - 10.0f, top + p.h - 10.0f, 4.5f,
                r, g, b, alpha * 0.52f);

    if (selected) {
        drawRect(left - 5.0f, top - 5.0f, p.w + 10.0f, 2.6f,
                 1.0f, 1.0f, 1.0f, alpha * 0.78f);
        drawRect(left - 5.0f, top + p.h + 2.4f, p.w + 10.0f, 2.6f,
                 1.0f, 1.0f, 1.0f, alpha * 0.78f);
        drawRect(left - 5.0f, top - 5.0f, 2.6f, p.h + 10.0f,
                 1.0f, 1.0f, 1.0f, alpha * 0.78f);
        drawRect(left + p.w + 2.4f, top - 5.0f, 2.6f, p.h + 10.0f,
                 1.0f, 1.0f, 1.0f, alpha * 0.78f);
    }
    BatchFlush();
}

static std::vector<std::wstring> TarotWrap(const wchar_t* src, float scale,
                                            float maxWidth) {
    std::vector<std::wstring> lines;
    std::wstring segment;
    const wchar_t* text = src ? src : L"";
    auto flush = [&]() {
        while (!segment.empty() && segment.front() == L' ') segment.erase(segment.begin());
        if (!segment.empty()) lines.push_back(segment);
        segment.clear();
    };
    for (const wchar_t* p = text; *p; ++p) {
        if (*p == L'/') { flush(); continue; }
        std::wstring test = segment;
        test.push_back(*p);
        if (!segment.empty() && g_TextS.Width(test.c_str(), scale) > maxWidth)
            flush();
        segment.push_back(*p);
    }
    flush();
    return lines;
}

static void DrawTarotDetailPanel(const AugDef& def, float x, float y, float w,
                                 float h, float alpha) {
    float r, g, b;
    GetRarityColor(def.rarity, r, g, b);
    BindMainShader();
    drawRect(x + 7.0f, y + 8.0f, w, h, 0.0f, 0.0f, 0.02f, alpha * 0.36f);
    drawRect(x, y, w, h, 0.018f, 0.024f, 0.052f, alpha * 0.94f);
    drawRect(x, y, w, 4.0f, r, g, b, alpha * 0.95f);
    drawConstellFrame(x, y, w, h, r, g, b, alpha * 0.52f,
                      16.0f, 5.0f, alpha * 0.20f);
    BatchFlush();

    g_TextS.Draw(GetAugBadge(def), x + 18.0f, y + 16.0f, 0.64f,
                 std::min(1.0f, r * 1.35f + 0.20f),
                 std::min(1.0f, g * 1.35f + 0.20f),
                 std::min(1.0f, b * 1.35f + 0.20f), alpha * 0.95f);
    const wchar_t* name = AugName(def);
    float nameScale = 0.90f;
    while (nameScale > 0.54f && g_TextL.Width(name, nameScale) > w - 36.0f)
        nameScale -= 0.04f;
    g_TextL.Draw(name, x + 18.0f, y + 45.0f, nameScale,
                 1.0f, 1.0f, 1.0f, alpha * 0.98f);
    BatchFlush();

    const wchar_t* stat = AugStat(def);
    g_TextS.Draw(stat, x + 18.0f, y + 82.0f, 0.62f,
                 0.72f, 1.0f, 0.82f, alpha * 0.92f);
    BatchFlush();

    const float descScale = 0.58f;
    const float maxWidth = w - 36.0f;
    const std::vector<std::wstring> lines = TarotWrap(AugDesc(def), descScale, maxWidth);
    float dy = y + 116.0f;
    for (const std::wstring& line : lines) {
        if (dy > y + h - 22.0f) break;
        g_TextS.Draw(line.c_str(), x + 18.0f, dy, descScale,
                     0.80f, 0.88f, 0.98f, alpha * 0.84f);
        dy += 22.0f;
    }
    BatchFlush();
}

enum class AugPreviewMetric {
    ATTACK,
    FIRE_RATE,
    BULLET_SPEED,
    MOVE_SPEED,
    MAX_HP,
    VISION,
    COUNT
};

enum class AugVisualFamily {
    STRIKE,
    WARD,
    MOTION,
    ORBITAL,
    UTILITY,
    FRACTURED,
    DUAL
};

struct AugSelectionLayout {
    float ui = 1.0f;
    float textUi = 1.0f;
    float orbitCX = 0.0f;
    float orbitCY = 0.0f;
    float orbitR = 0.0f;
    float baseR = 0.0f;
    float panelX = 0.0f;
    float panelY = 0.0f;
    float panelW = 0.0f;
    float panelH = 0.0f;
};

static PlayerStats MakeAugPreviewBaseStats() {
    PlayerStats base;
    ApplyMeta(base);
    base.windowSize *= g_Scale;

    if (g_CurrentWeapon >= 0 &&
        g_CurrentWeapon < (int)StartWeapon::_COUNT) {
        ApplyWeapon(base, (StartWeapon)g_CurrentWeapon);
    }
    return base;
}

static PlayerStats MakeAugPreviewTrialStats() {
    PlayerStats trial = MakeAugPreviewBaseStats();
    const float trialHp = TrialPlayerMaxHpMult();
    if (trialHp < 0.999f)
        trial.maxHP *= trialHp;
    return trial;
}

static PlayerStats MakeAugPreviewOwnedStats() {
    PlayerStats owned = MakeAugPreviewTrialStats();
    for (int idx : g_OwnedAugs) {
        if (idx < 0 || idx >= AUG_TOTAL) continue;
        owned.Apply(ALL_AUGS[idx].type);
        if (ALL_AUGS[idx].rarity == AugRarity::COMMON)
            owned.ApplyCommonMultBoost();
    }
    return owned;
}

static PlayerStats MakeAugPreviewCandidateStats(const PlayerStats& owned,
                                                const AugDef& candidate) {
    PlayerStats preview = owned;
    preview.Apply(candidate.type);
    if (candidate.rarity == AugRarity::COMMON)
        preview.ApplyCommonMultBoost();
    return preview;
}

static float AugPreviewMetricValue(const PlayerStats& stats,
                                   AugPreviewMetric metric) {
    switch (metric) {
    case AugPreviewMetric::ATTACK:
        return stats.GetBaseDamage() * stats.damageMultiplier;
    case AugPreviewMetric::FIRE_RATE:
        return stats.fireInterval > 0.0001f
            ? 1.0f / stats.fireInterval : 0.0f;
    case AugPreviewMetric::BULLET_SPEED:
        return stats.bulletSpeed;
    case AugPreviewMetric::MOVE_SPEED:
        return stats.moveSpeedMult * 100.0f;
    case AugPreviewMetric::MAX_HP:
        return stats.maxHP;
    case AugPreviewMetric::VISION:
        return stats.windowSize;
    default:
        return 0.0f;
    }
}

struct TarotBurstParticle {
    float x = 0.0f, y = 0.0f;
    float vx = 0.0f, vy = 0.0f;
    float life = 0.0f;
    float r = 1.0f, g = 1.0f, b = 1.0f;
};

struct AugShapeVertex {
    float angleDeg;
    float radius;
};

struct AugShapeEdge {
    int a;
    int b;
};

struct AugImpactRow {
    AugPreviewMetric metric = AugPreviewMetric::ATTACK;
    float before = 0.0f;
    float after = 0.0f;
};

static float AugSelectionClamp(float v) {
    return std::max(0.0f, std::min(1.0f, v));
}

static float AugSelectionEase(float v) {
    const float t = AugSelectionClamp(v);
    const float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
}

static float AugSelectionSmooth(float speed, float delta) {
    return 1.0f - expf(-std::max(0.0f, speed) *
                       std::max(0.0f, delta));
}

static AugSelectionLayout MakeAugSelectionLayout(float sw, float sh) {
    AugSelectionLayout layout;
    layout.ui = std::max(0.72f, std::min(1.15f,
        std::min(sw / 1920.0f, sh / 1080.0f)));
    layout.textUi = std::max(0.86f, layout.ui);
    const float aspect = sh > 1.0f ? sw / sh : 1.777f;
    layout.orbitCX = sw * (aspect < 1.50f ? 0.34f : 0.36f);
    layout.orbitCY = sh * 0.50f;
    layout.panelX = sw * (aspect < 1.50f ? 0.66f : 0.69f);
    layout.panelY = sh * 0.19f;
    layout.panelW = std::max(230.0f * layout.ui,
                             sw - layout.panelX - 42.0f * layout.ui);
    layout.panelH = sh * 0.67f;
    // Keep the selector's large circular structure dominant while leaving a
    // deliberate breathing gap before the detail panel.
    layout.orbitR = std::min(sw * 0.21f, sh * 0.32f);
    layout.orbitR = std::min(layout.orbitR,
        std::max(140.0f * layout.ui,
                 layout.panelX - layout.orbitCX - 132.0f * layout.ui));
    const float baseR = std::max(42.0f * layout.ui,
        std::min(64.0f * layout.ui, std::min(sw, sh) * 0.060f));
    layout.baseR = baseR * 1.25f;
    return layout;
}

static bool AugIsWardVisual(AugType type) {
    switch (type) {
    case AugType::REGEN_UP:
    case AugType::LIFESTEAL:
    case AugType::VAMPIRE:
    case AugType::MK2:
    case AugType::HP_UP:
    case AugType::FIREWALL:
    case AugType::REGEN_2:
    case AugType::CB_BASTION:
    case AugType::CB_LIFEBUOY:
        return true;
    default:
        return false;
    }
}

static bool AugIsMotionVisual(AugType type) {
    switch (type) {
    case AugType::MOVE_UP:
    case AugType::VISION_UP:
    case AugType::LIGHT_AMMO:
    case AugType::LIGHT_STEP:
    case AugType::MINIATURIZE:
        return true;
    default:
        return false;
    }
}

static bool AugIsStrikeVisual(AugType type) {
    switch (type) {
    case AugType::DMG_UP:
    case AugType::RATE_UP:
    case AugType::SPD_UP:
    case AugType::CRIT:
    case AugType::OVERDRIVE:
    case AugType::CORE_OVERLOAD:
    case AugType::PIERCE:
    case AugType::PIERCE_2:
    case AugType::TWIN:
    case AugType::TWIN_2:
    case AugType::CHAIN:
    case AugType::CHAIN_2:
    case AugType::DEATH_BLAST:
    case AugType::DEATH_BLAST_2:
    case AugType::POWER_SURGE:
        return true;
    default:
        return false;
    }
}

static AugVisualFamily AugVisualFamilyOf(const AugDef& def) {
    if (def.rarity == AugRarity::DEBUFF) return AugVisualFamily::FRACTURED;
    if (def.rarity == AugRarity::COMBO) return AugVisualFamily::DUAL;
    const AugListGroup group = AugListGroupOf(def);
    if (group == AugListGroup::ORBIT) return AugVisualFamily::ORBITAL;
    if (group == AugListGroup::WEAPON) return AugVisualFamily::STRIKE;
    if (group == AugListGroup::SKILL || group == AugListGroup::SPECIAL)
        return AugVisualFamily::UTILITY;
    if (AugIsWardVisual(def.type)) return AugVisualFamily::WARD;
    if (AugIsMotionVisual(def.type)) return AugVisualFamily::MOTION;
    if (AugIsStrikeVisual(def.type)) return AugVisualFamily::STRIKE;
    return AugVisualFamily::UTILITY;
}

static const wchar_t* AugVisualFamilyLabel(AugVisualFamily family) {
    switch (family) {
    case AugVisualFamily::STRIKE:    return L"STRIKE";
    case AugVisualFamily::WARD:      return L"WARD";
    case AugVisualFamily::MOTION:    return L"MOTION";
    case AugVisualFamily::ORBITAL:   return L"ORBITAL";
    case AugVisualFamily::UTILITY:   return L"UTILITY";
    case AugVisualFamily::FRACTURED: return L"FRACTURED";
    case AugVisualFamily::DUAL:      return L"DUAL";
    default:                         return L"UNKNOWN";
    }
}

static void DrawAugSelectionConstellation(const AugDef& def,
                                          AugVisualFamily family,
                                          float cx, float cy, float radius,
                                          float lineR, float lineG, float lineB,
                                          float accentR, float accentG,
                                          float accentB, float alpha,
                                          float reveal, float rotation,
                                          float ui, bool focused,
                                          float now) {
    static const AugShapeVertex strikeV[] = {
        {-90,1.00f},{-54,0.42f},{-18,0.96f},{18,0.40f},{54,1.00f},
        {90,0.42f},{126,0.94f},{162,0.40f},{198,0.98f},{234,0.42f}
    };
    static const AugShapeEdge strikeE[] = {
        {0,1},{1,2},{2,3},{3,4},{4,5},{5,6},{6,7},{7,8},{8,9},{9,0},
        {1,5},{5,9},{9,3},{3,7},{7,1}
    };
    static const AugShapeVertex wardV[] = {
        {-90,1.00f},{-30,1.00f},{30,1.00f},{90,1.00f},{150,1.00f},{210,1.00f},
        {-60,0.50f},{60,0.50f},{180,0.50f},{0,0.00f}
    };
    static const AugShapeEdge wardE[] = {
        {0,1},{1,2},{2,3},{3,4},{4,5},{5,0},{6,7},{7,8},{8,6},
        {0,8},{1,6},{2,6},{3,7},{4,7},{5,8},{6,9},{7,9},{8,9}
    };
    static const AugShapeVertex motionV[] = {
        {-72,0.96f},{-24,1.00f},{25,0.82f},{-38,0.55f},{18,0.48f},
        {72,0.62f},{126,0.78f},{176,0.58f},{220,0.88f}
    };
    static const AugShapeEdge motionE[] = {
        {0,1},{1,2},{2,4},{4,3},{3,0},{0,4},{1,3},{4,5},{5,6},{6,7},{7,8}
    };
    static const AugShapeVertex orbitalV[] = {
        {0,0.00f},{-90,1.00f},{-45,0.88f},{0,1.00f},{45,0.88f},
        {90,1.00f},{135,0.88f},{180,1.00f},{225,0.88f}
    };
    static const AugShapeEdge orbitalE[] = {
        {1,2},{2,3},{3,4},{4,5},{5,6},{6,7},{7,8},{8,1},
        {0,1},{0,3},{0,5},{0,7}
    };
    static const AugShapeVertex utilityV[] = {
        {-90,0.94f},{-45,0.58f},{0,0.84f},{45,0.54f},{90,0.92f},
        {150,0.62f},{205,0.98f},{232,0.52f},{268,0.82f}
    };
    static const AugShapeEdge utilityE[] = {
        {0,1},{1,2},{2,3},{3,4},{1,5},{5,6},{3,7},{7,8},{5,7},{2,7}
    };
    static const AugShapeVertex fracturedV[] = {
        {-92,1.00f},{-48,0.55f},{-8,0.92f},{38,0.48f},{78,1.00f},
        {126,0.56f},{168,0.90f},{210,0.44f},{252,0.86f}
    };
    static const AugShapeEdge fracturedE[] = {
        {0,1},{1,2},{3,4},{4,5},{5,6},{7,8},{8,0},{1,5},{2,7},{4,7}
    };

    if (family == AugVisualFamily::DUAL) {
        DrawAugSelectionConstellation(def, AugVisualFamily::WARD,
            cx - radius * 0.28f, cy, radius * 0.70f,
            lineR, lineG, lineB, accentR, accentG, accentB,
            alpha, reveal, rotation - 0.18f, ui, focused, now);
        DrawAugSelectionConstellation(def, AugVisualFamily::STRIKE,
            cx + radius * 0.28f, cy, radius * 0.70f,
            lineR, lineG, lineB, accentR, accentG, accentB,
            alpha, reveal, rotation + 0.18f, ui, focused, now);
        DrawVisibleConstellLine(cx - radius * 0.24f, cy,
                                cx + radius * 0.24f, cy,
                                (focused ? 2.4f : 1.7f) * ui,
                                accentR, accentG, accentB,
                                alpha * 0.62f * reveal);
        BatchFlush();
        return;
    }

    const AugShapeVertex* vertices = utilityV;
    const AugShapeEdge* edges = utilityE;
    int vertexCount = (int)(sizeof(utilityV) / sizeof(utilityV[0]));
    int edgeCount = (int)(sizeof(utilityE) / sizeof(utilityE[0]));
    switch (family) {
    case AugVisualFamily::STRIKE:
        vertices = strikeV; edges = strikeE;
        vertexCount = (int)(sizeof(strikeV) / sizeof(strikeV[0]));
        edgeCount = (int)(sizeof(strikeE) / sizeof(strikeE[0]));
        break;
    case AugVisualFamily::WARD:
        vertices = wardV; edges = wardE;
        vertexCount = (int)(sizeof(wardV) / sizeof(wardV[0]));
        edgeCount = (int)(sizeof(wardE) / sizeof(wardE[0]));
        break;
    case AugVisualFamily::MOTION:
        vertices = motionV; edges = motionE;
        vertexCount = (int)(sizeof(motionV) / sizeof(motionV[0]));
        edgeCount = (int)(sizeof(motionE) / sizeof(motionE[0]));
        break;
    case AugVisualFamily::ORBITAL:
        vertices = orbitalV; edges = orbitalE;
        vertexCount = (int)(sizeof(orbitalV) / sizeof(orbitalV[0]));
        edgeCount = (int)(sizeof(orbitalE) / sizeof(orbitalE[0]));
        break;
    case AugVisualFamily::FRACTURED:
        vertices = fracturedV; edges = fracturedE;
        vertexCount = (int)(sizeof(fracturedV) / sizeof(fracturedV[0]));
        edgeCount = (int)(sizeof(fracturedE) / sizeof(fracturedE[0]));
        break;
    default:
        break;
    }

    DrawConstellationDisc(cx, cy, radius * 1.48f,
                          0.0f, 0.0f, 0.008f, alpha * 0.38f);
    DrawNebulaGlow(cx, cy, radius * (focused ? 2.08f : 1.58f),
                   lineR, lineG, lineB,
                   alpha * (focused ? 0.54f : 0.22f), now,
                   def.rarity == AugRarity::SPECIAL);

    float px[16] = {};
    float py[16] = {};
    const unsigned seed = ((unsigned)((int)def.type + 1) * 2654435761u);
    const float baseJitter = (((seed >> 4) & 255u) / 255.0f - 0.5f) * 0.12f;
    for (int i = 0; i < vertexCount && i < 16; ++i) {
        const unsigned bits = (seed >> ((i * 3) & 15)) ^ (seed * (unsigned)(i + 3));
        const float angleJitter = (((bits >> 5) & 15u) / 15.0f - 0.5f) * 0.10f;
        const float radialJitter = 0.94f + ((bits >> 9) & 15u) / 15.0f * 0.12f;
        const float angle = vertices[i].angleDeg * (float)M_PI / 180.0f +
                            rotation + baseJitter + angleJitter;
        const float vr = radius * vertices[i].radius * radialJitter;
        px[i] = cx + cosf(angle) * vr;
        py[i] = cy + sinf(angle) * vr;
    }

    const float edgeProgress = AugSelectionClamp(reveal) * (float)edgeCount;
    for (int e = 0; e < edgeCount; ++e) {
        const float local = AugSelectionClamp(edgeProgress - (float)e);
        if (local <= 0.001f) continue;
        const int a = edges[e].a;
        const int b = edges[e].b;
        const float ex = px[a] + (px[b] - px[a]) * local;
        const float ey = py[a] + (py[b] - py[a]) * local;
        DrawVisibleConstellLine(px[a], py[a], ex, ey,
                                (focused ? 2.35f : 1.72f) * ui,
                                lineR, lineG, lineB,
                                alpha * (focused ? 0.90f : 0.68f));
    }

    const float nodeProgress = AugSelectionClamp(reveal * 1.20f) *
                               (float)vertexCount;
    for (int i = 0; i < vertexCount; ++i) {
        const float local = AugSelectionClamp(nodeProgress - (float)i);
        if (local <= 0.001f) continue;
        const bool centerNode = vertices[i].radius < 0.10f;
        const float size = (centerNode ? 5.8f : (focused ? 4.8f : 3.6f)) *
                           ui * (0.70f + local * 0.30f);
        DrawVisibleConstellNode(px[i], py[i], size,
                                centerNode ? accentR : lineR,
                                centerNode ? accentG : lineG,
                                centerNode ? accentB : lineB,
                                alpha * (0.72f + local * 0.24f),
                                focused || centerNode, true);
    }

    const float pulse = 0.5f + 0.5f * sinf(now * 3.2f + baseJitter * 8.0f);
    DrawVisibleConstellNode(cx, cy,
        (focused ? 10.5f + pulse * 2.8f : 6.5f + pulse * 1.0f) * ui,
        accentR, accentG, accentB,
        alpha * AugSelectionClamp(reveal * 1.35f), true, true);
    BatchFlush();
}

static int BuildAugImpactRows(const AugDef& def, AugImpactRow* rows,
                              int maxRows) {
    if (!rows || maxRows <= 0) return 0;
    const PlayerStats owned = MakeAugPreviewOwnedStats();
    const PlayerStats result = MakeAugPreviewCandidateStats(owned, def);
    int count = 0;
    for (int i = 0; i < (int)AugPreviewMetric::COUNT && count < maxRows; ++i) {
        const AugPreviewMetric metric = (AugPreviewMetric)i;
        const float before = AugPreviewMetricValue(owned, metric);
        const float after = AugPreviewMetricValue(result, metric);
        if (fabsf(after - before) <= 0.05f) continue;
        rows[count++] = { metric, before, after };
    }
    return count;
}

static const wchar_t* AugMetricLabel(AugPreviewMetric metric) {
    static const wchar_t* labels[3][(int)AugPreviewMetric::COUNT] = {
        { L"공격", L"연사", L"탄속", L"이동", L"최대 HP", L"시야" },
        { L"ATTACK", L"FIRE RATE", L"BULLET SPD", L"MOVE", L"MAX HP", L"VISION" },
        { L"攻撃", L"連射", L"弾速", L"移動", L"最大HP", L"視界" }
    };
    int lang = CurLangIdx();
    if (lang < 0 || lang > 2) lang = 0;
    int idx = (int)metric;
    if (idx < 0 || idx >= (int)AugPreviewMetric::COUNT) idx = 0;
    return labels[lang][idx];
}

static void FormatAugMetricValue(wchar_t* out, size_t outCount,
                                 AugPreviewMetric metric, float value,
                                 bool signedValue) {
    if (!out || outCount == 0) return;
    if (metric == AugPreviewMetric::FIRE_RATE) {
        swprintf_s(out, outCount, signedValue ? L"%+.1f/s" : L"%.1f/s", value);
    } else if (metric == AugPreviewMetric::MOVE_SPEED) {
        swprintf_s(out, outCount, signedValue ? L"%+.1f%%" : L"%.1f%%", value);
    } else {
        swprintf_s(out, outCount, signedValue ? L"%+.0f" : L"%.0f", value);
    }
}

static void Scene_AugSelectConstellationPolished(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh, delta = c.delta;
    const bool isDebuff = g_GameManager.currentState == GameState::DEBUFF_SELECT;
    const int count = std::max(0, std::min(3, g_GameManager.augChoiceCount));
    const bool inExit = g_AugExitT >= 0.0f;
    const float now = (float)glfwGetTime();
    const AugSelectionLayout layout = MakeAugSelectionLayout(sw, sh);

    static int seenSerial = -1;
    static float enterT = 0.0f;
    static float orbitPhase = 0.0f;
    static float orbitSpeed = 0.0f;
    static float ringPhase = 0.0f;
    static float hoverMotionT = 0.0f;
    static float shapePhase[3] = {};
    static float shieldCycleT[3] = {};
    static float shieldPhase[3] = {};
    static float shieldT[3] = {};
    static int shieldFocus = -1;
    static float focusT[3] = {};
    static float dimT[3] = { 1.0f, 1.0f, 1.0f };
    static float flashT[3] = {};
    static float panelT = 0.0f;
    static int previousFocus = -1;
    static bool lmbPrev = false;
    static double previousMx = -1.0;
    static double previousMy = -1.0;
    struct Particle { float x,y,vx,vy,life,r,g,b; };
    static Particle particles[72];
    static bool particlesSpawned = false;

    if (seenSerial != g_GameManager.augmentSelectionSerial) {
        seenSerial = g_GameManager.augmentSelectionSerial;
        enterT = 0.0f;
        orbitSpeed = 0.0f;
        ringPhase = 0.0f;
        hoverMotionT = 0.0f;
        for (int i = 0; i < 3; ++i)
            shapePhase[i] = (float)i * 2.0944f;
        for (int i = 0; i < 3; ++i) {
            shieldCycleT[i] = 0.0f;
            shieldPhase[i] = 0.0f;
            shieldT[i] = 0.0f;
        }
        shieldFocus = -1;
        panelT = 0.0f;
        previousFocus = -1;
        lmbPrev = false;
        previousMx = c.mx;
        previousMy = c.my;
        particlesSpawned = false;
        g_HoveredAug = -1;
        for (int i = 0; i < 3; ++i) {
            focusT[i] = 0.0f;
            dimT[i] = 1.0f;
            flashT[i] = 0.0f;
        }
        for (auto& p : particles) p.life = 0.0f;
    }

    if (!inExit) enterT = std::min(1.0f, enterT + delta / 1.20f);
    const float enterE = AugSelectionEase(enterT);
    const bool inputReady = enterT >= 0.66f && !inExit;
    const int focus = g_HoveredAug >= 0 && g_HoveredAug < count
        ? g_HoveredAug : -1;

    if (focus != previousFocus && focus >= 0) flashT[focus] = 1.0f;
    previousFocus = focus;
    for (int i = 0; i < 3; ++i) {
        const float focusTarget = inputReady && focus == i ? 1.0f : 0.0f;
        // Keep every candidate's own hue. Non-focused constellations only lose
        // light, so focus reads as depth instead of a palette/state change.
        const float dimTarget = focus >= 0 && focus != i ? 0.46f : 1.0f;
        focusT[i] += (focusTarget - focusT[i]) *
                     AugSelectionSmooth(8.5f, delta);
        dimT[i] += (dimTarget - dimT[i]) *
                   AugSelectionSmooth(9.0f, delta);
        flashT[i] = std::max(0.0f, flashT[i] - delta / 0.22f);
    }
    panelT += ((focus >= 0 && !inExit ? 1.0f : 0.0f) - panelT) *
              AugSelectionSmooth(10.0f, delta);

    const float decel = AugSelectionEase(
        AugSelectionClamp((enterT - 0.12f) / 0.78f));
    const float entrySpeed = 0.90f + (0.065f - 0.90f) * decel;
    // Keep one shared angular velocity for the whole orbit. Hovering ramps
    // that velocity up smoothly instead of assigning a different speed to
    // each candidate based on its position.
    const float hoverTarget = focus >= 0 ? 1.0f : 0.0f;
    hoverMotionT += (hoverTarget - hoverMotionT) *
                    AugSelectionSmooth(focus >= 0 ? 11.0f : 8.0f, delta);
    const float hoverOrbitSpeed = 0.065f +
                                  0.075f * AugSelectionEase(hoverMotionT);
    const float targetSpeed = focus >= 0 ? hoverOrbitSpeed : entrySpeed;
    orbitSpeed += (targetSpeed - orbitSpeed) *
                  AugSelectionSmooth(focus >= 0 ? 8.5f : 4.5f, delta);
    if (!inExit) {
        orbitPhase += orbitSpeed * delta;
        ringPhase += orbitSpeed * delta;
        if (ringPhase > 2.0f * (float)M_PI)
            ringPhase -= 2.0f * (float)M_PI;
    }
    for (int i = 0; i < count; ++i) {
        // The constellation's local rotation is continuous across hover
        // changes. Focus may scale/pull it during confirmation, but never
        // re-seed its angle from a different time-based multiplier.
        // Keep the constellation's own spin independent from the shared
        // orbit. The previous value was too subtle to read during play.
        const float localSpeed = 0.24f;
        if (!inExit) shapePhase[i] += localSpeed * delta;
        if (shapePhase[i] > 2.0f * (float)M_PI)
            shapePhase[i] -= 2.0f * (float)M_PI;
    }

    float orbitX[3] = {};
    float orbitY[3] = {};
    float drawX[3] = {};
    float drawY[3] = {};
    float drawR[3] = {};
    float reveal[3] = {};
    for (int i = 0; i < count; ++i) {
        float angle = -(float)M_PI * 0.50f;
        if (count > 1)
            angle += orbitPhase + (float)i * (2.0f * (float)M_PI / (float)count);
        const float radiusScale = count == 1 ? 0.72f : 1.0f;
        orbitX[i] = layout.orbitCX + cosf(angle) * layout.orbitR * radiusScale;
        orbitY[i] = layout.orbitCY + sinf(angle) * layout.orbitR * radiusScale;
        reveal[i] = AugSelectionEase(
            AugSelectionClamp((enterT - (float)i * 0.07f) / 0.62f));
        const float grow = AugSelectionEase(focusT[i]);
        // Hover magnifies the constellation exactly where it lives on the
        // orbit. Only confirmation is allowed to pull it into the centre.
        drawX[i] = layout.orbitCX + (orbitX[i] - layout.orbitCX) * reveal[i];
        drawY[i] = layout.orbitCY + (orbitY[i] - layout.orbitCY) * reveal[i];
        drawR[i] = layout.baseR * (1.0f + grow * 0.64f + flashT[i] * 0.05f);
    }

    const bool pointerMoved = previousMx < 0.0 ||
        fabs(c.mx - previousMx) > 3.0 || fabs(c.my - previousMy) > 3.0;
    if (pointerMoved) g_GameManager.augmentKeyboardFocus = false;
    previousMx = c.mx;
    previousMy = c.my;
    auto hitCandidate = [&]() -> int {
        int best = -1;
        float bestD2 = 1e30f;
        for (int i = 0; i < count; ++i) {
            const float dx = (float)c.mx - drawX[i];
            const float dy = (float)c.my - drawY[i];
            const float d2 = dx * dx + dy * dy;
            const float hitR = std::max(layout.baseR * 1.55f,
                                        drawR[i] * 1.15f);
            if (d2 <= hitR * hitR && d2 < bestD2) {
                best = i;
                bestD2 = d2;
            }
        }
        return best;
    };
    if (inputReady && pointerMoved &&
        !g_GameManager.augmentKeyboardFocus) {
        const int hovered = hitCandidate();
        if (hovered >= 0) g_HoveredAug = hovered;
    }
    const bool lmbClick = c.lmb && !lmbPrev;
    if (inputReady && lmbClick) {
        const int clicked = hitCandidate();
        if (clicked >= 0) {
            g_HoveredAug = clicked;
            g_GameManager.augmentKeyboardFocus = false;
        }
    }
    lmbPrev = c.lmb;

    const int activeFocus = g_HoveredAug >= 0 && g_HoveredAug < count
        ? g_HoveredAug : -1;
    for (int i = 0; i < 3; ++i) {
        const float shieldTarget = !inExit && activeFocus == i ? 1.0f : 0.0f;
        shieldT[i] += (shieldTarget - shieldT[i]) *
                      AugSelectionSmooth(13.0f, delta);
    }
    const float stateR = isDebuff ? 0.96f : 0.16f;
    const float stateG = isDebuff ? 0.14f : 0.84f;
    const float stateB = isDebuff ? 0.18f : 0.98f;
    const float collapseT = inExit
        ? AugSelectionEase(AugSelectionClamp(g_AugExitT / 0.55f)) : 0.0f;
    const float novaT = inExit
        ? AugSelectionEase(AugSelectionClamp((g_AugExitT - 0.55f) / 0.40f)) : 0.0f;
    const float sceneAlpha = inExit
        ? 1.0f - AugSelectionClamp((g_AugExitT - 1.12f) / 0.50f) : 1.0f;

    if (activeFocus != shieldFocus) {
        shieldFocus = activeFocus;
        if (activeFocus >= 0) {
            shieldCycleT[activeFocus] = 0.0f;
            shieldPhase[activeFocus] = 0.0f;
        }
    }

    float activeShieldPulse = 0.0f;
    if (!inExit && activeFocus >= 0) {
        float& cycle = shieldCycleT[activeFocus];
        cycle += delta;
        constexpr float kShieldAccel = 0.40f;
        constexpr float kShieldDecel = 0.70f;
        constexpr float kShieldRest = 0.45f;
        constexpr float kShieldCycle = kShieldAccel + kShieldDecel +
                                       kShieldRest;
        while (cycle >= kShieldCycle) cycle -= kShieldCycle;

        float speedScale = 0.72f;
        if (cycle < kShieldAccel) {
            activeShieldPulse = AugSelectionEase(cycle / kShieldAccel);
            speedScale = 0.72f + activeShieldPulse * 3.10f;
        } else if (cycle < kShieldAccel + kShieldDecel) {
            const float t = (cycle - kShieldAccel) / kShieldDecel;
            const float ease = AugSelectionEase(t);
            activeShieldPulse = 1.0f - ease * 0.78f;
            speedScale = 3.82f - ease * 3.10f;
        } else {
            activeShieldPulse = 0.10f;
        }

        shieldPhase[activeFocus] += 0.92f * speedScale * delta;
        if (shieldPhase[activeFocus] > 2.0f * (float)M_PI)
            shieldPhase[activeFocus] -= 2.0f * (float)M_PI;
    }

    BindMainShader();
    // Keep the gameplay scene readable underneath the selector. The former
    // near-black plate made the constellation field and surrounding context
    // disappear on darker displays.
    drawRect(0.0f, 0.0f, sw, sh,
             isDebuff ? 0.026f : 0.008f,
             isDebuff ? 0.008f : 0.018f,
             isDebuff ? 0.014f : 0.040f,
             enterE * (isDebuff ? 0.58f : 0.52f) * sceneAlpha);
    DrawNebulaGlow(layout.orbitCX, layout.orbitCY,
                   layout.orbitR * 1.02f,
                   stateR, stateG, stateB,
                   enterE * sceneAlpha * 0.34f,
                   now, false);
    drawRect(layout.panelX - 30.0f * layout.ui,
             layout.panelY - 28.0f * layout.ui,
             layout.panelW + 18.0f * layout.ui,
             layout.panelH + 38.0f * layout.ui,
             0.0f, 0.006f, 0.016f,
             enterE * sceneAlpha * 0.20f);
    DrawRadialGradientRect(
        layout.panelX - 80.0f * layout.ui,
        layout.panelY - 20.0f * layout.ui,
        layout.panelW + 160.0f * layout.ui,
        layout.panelH + 40.0f * layout.ui,
        stateR * 0.20f, stateG * 0.20f, stateB * 0.20f,
        enterE * sceneAlpha * 0.17f);
    BatchFlush();

    const float frameA = enterE * sceneAlpha;
    drawConstellFrame(18.0f * layout.ui, 18.0f * layout.ui,
                      sw - 36.0f * layout.ui,
                      sh - 36.0f * layout.ui,
                      stateR, stateG, stateB, frameA * 0.16f,
                      24.0f * layout.ui, 7.0f * layout.ui,
                      frameA * 0.10f, enterE);
    BatchFlush();

    if (!inExit && count > 0) {
        // The candidates share one circular orbit. It replaces the old
        // candidate-to-candidate triangle; the constellation shapes remain
        // independent and unchanged.
        const float orbitScale = count == 1 ? 0.72f : 1.0f;
        const float orbitReveal = reveal[count - 1];
        DrawAugmentOrbitRing(
            layout.orbitCX, layout.orbitCY,
            layout.orbitR * orbitScale * orbitReveal,
            layout.orbitR * orbitScale * orbitReveal,
            ringPhase - (float)M_PI * 0.5f,
            stateR, stateG, stateB,
            enterE * sceneAlpha * orbitReveal * 0.44f,
            layout.ui, 1.45f, 1.0f);
        // Inner field orbits give the empty center a layered, illuminated
        // structure without changing the candidates' path.
        DrawAugmentOrbitRing(
            layout.orbitCX, layout.orbitCY,
            layout.orbitR * orbitScale * orbitReveal * 0.84f,
            layout.orbitR * orbitScale * orbitReveal * 0.84f,
            ringPhase + 0.72f,
            stateR, stateG, stateB,
            enterE * sceneAlpha * orbitReveal * 0.22f,
            layout.ui, 1.16f, 0.42f);
        DrawAugmentOrbitRing(
            layout.orbitCX, layout.orbitCY,
            layout.orbitR * orbitScale * orbitReveal * 0.66f,
            layout.orbitR * orbitScale * orbitReveal * 0.66f,
            ringPhase + 1.46f,
            stateR, stateG, stateB,
            enterE * sceneAlpha * orbitReveal * 0.14f,
            layout.ui, 1.05f, 0.30f);
        DrawAugmentOrbitRing(
            layout.orbitCX, layout.orbitCY,
            layout.orbitR * orbitScale * orbitReveal * 0.48f,
            layout.orbitR * orbitScale * orbitReveal * 0.48f,
            ringPhase + 2.10f,
            stateR, stateG, stateB,
            enterE * sceneAlpha * orbitReveal * 0.10f,
            layout.ui, 0.96f, 0.22f);
        BatchFlush();

        const wchar_t* orbitLabel = isDebuff ? L"DEBUFF SELECT"
                                             : L"AUG SELECT";
        const float orbitLabelScale = 1.38f * layout.textUi;
        const float orbitLabelW = g_TextS.Width(orbitLabel, orbitLabelScale);
        g_TextS.Draw(orbitLabel,
                     layout.orbitCX - orbitLabelW * 0.5f,
                     layout.orbitCY - 10.0f * layout.ui,
                     orbitLabelScale,
                     0.95f, 0.98f, 1.0f,
                     enterE * sceneAlpha * 0.72f);
        const wchar_t* orbitSubLabel = L"CONSTELLATION FIELD";
        const float orbitSubScale = 0.28f * layout.textUi;
        const float orbitSubW = g_TextS.Width(orbitSubLabel, orbitSubScale);
        g_TextS.Draw(orbitSubLabel,
                     layout.orbitCX - orbitSubW * 0.5f,
                     layout.orbitCY + 18.0f * layout.ui,
                     orbitSubScale,
                     0.52f, 0.68f, 0.80f,
                     enterE * sceneAlpha * 0.34f);
        BatchFlush();
    }

    for (int i = 0; i < count; ++i) {
        const int augIdx = g_GameManager.augChoices[i];
        if (augIdx < 0 || augIdx >= AUG_TOTAL) continue;
        const AugDef& def = ALL_AUGS[augIdx];
        const bool focused = activeFocus == i && !inExit;
        const bool selected = inExit && g_AugExitSlot == i;
        const bool other = inExit && g_AugExitSlot != i;
        float px = drawX[i];
        float py = drawY[i];
        float radius = drawR[i];
        float alpha = reveal[i] * dimT[i] * sceneAlpha;
        if (selected) {
            px += (layout.orbitCX - px) * collapseT;
            py += (layout.orbitCY - py) * collapseT;
            radius = layout.baseR * (1.0f + collapseT * 0.72f + novaT * 3.4f);
            alpha *= 1.0f - novaT * novaT;
        } else if (other) {
            int selectedSlot = std::max(0, std::min(count - 1, g_AugExitSlot));
            const float centerX = drawX[selectedSlot] +
                                  (layout.orbitCX - drawX[selectedSlot]) * collapseT;
            const float centerY = drawY[selectedSlot] +
                                  (layout.orbitCY - drawY[selectedSlot]) * collapseT;
            const float initialAngle = atan2f(drawY[i] - drawY[selectedSlot],
                                              drawX[i] - drawX[selectedSlot]);
            const float initialDist = sqrtf(
                (drawX[i] - drawX[selectedSlot]) * (drawX[i] - drawX[selectedSlot]) +
                (drawY[i] - drawY[selectedSlot]) * (drawY[i] - drawY[selectedSlot]));
            const float spiralR = initialDist * (1.0f - collapseT);
            const float spiralA = initialAngle + g_AugExitT * 10.5f;
            px = centerX + cosf(spiralA) * spiralR;
            py = centerY + sinf(spiralA) * spiralR;
            radius *= 1.0f - collapseT * 0.92f;
            alpha *= 1.0f - collapseT;
        }
        if (alpha <= 0.004f || radius <= 2.0f) continue;

        float rarityR, rarityG, rarityB;
        GetRarityColor(def.rarity, rarityR, rarityG, rarityB);
        // Rarity is the constellation's material identity. Keep a small state
        // tint for buff/debuff context, but let the tier color drive the whole
        // silhouette: lines, outer nodes, nebula glow, and hover shield.
        const float lineR = stateR * 0.22f + rarityR * 0.78f;
        const float lineG = stateG * 0.22f + rarityG * 0.78f;
        const float lineB = stateB * 0.22f + rarityB * 0.78f;
        const AugVisualFamily family = AugVisualFamilyOf(def);
        DrawAugSelectionConstellation(def, family, px, py, radius,
            selected && novaT > 0.0f ? 1.0f : lineR,
            selected && novaT > 0.0f ? 1.0f : lineG,
            selected && novaT > 0.0f ? 1.0f : lineB,
            rarityR, rarityG, rarityB,
            alpha, reveal[i], shapePhase[i], layout.ui,
            focused || selected, now);

        if (!inExit && shieldT[i] > 0.01f) {
            const bool shieldActive = i == activeFocus;
            const float shieldPulse = shieldActive ? activeShieldPulse : 0.10f;
            const float shieldAlpha = alpha * shieldT[i] *
                (shieldActive ? 0.36f + shieldPulse * 0.62f : 0.20f);
            DrawAugmentShieldOrbit(
                px, py, radius,
                shieldPhase[i], shieldPhase[i] + 0.38f,
                lineR, lineG, lineB,
                shieldAlpha, shieldPulse, layout.ui);
            BatchFlush();
        }

        if (!inExit && reveal[i] > 0.42f) {
            wchar_t keyLabel[16];
            swprintf_s(keyLabel, L"[%d]", i + 1);
            const float keyScale = (focused ? 0.86f : 0.72f) * layout.textUi;
            const float keyW = g_TextS.Width(keyLabel, keyScale);
            const float labelGap = 10.0f * layout.ui;
            float nameScale = 0.48f * layout.textUi;
            const float maxGroupW = layout.orbitR * 1.12f;
            while (nameScale > 0.34f &&
                   keyW + labelGap +
                       g_TextS.Width(AugName(def), nameScale) > maxGroupW)
                nameScale -= 0.025f;
            const float nameW = g_TextS.Width(AugName(def), nameScale);
            const float groupW = keyW + labelGap + nameW;
            const float labelY = focused
                ? py + (py < layout.orbitCY ? radius + 21.0f * layout.ui
                                             : -radius - 29.0f * layout.ui)
                : py + (py < layout.orbitCY ? -radius - 26.0f * layout.ui
                                             : radius + 14.0f * layout.ui);
            const float groupX = px - groupW * 0.5f;
            const float nameY = labelY +
                                (keyScale - nameScale) * 0.28f;
            DrawShadowedText(g_TextS, keyLabel,
                groupX, labelY, keyScale,
                focused ? 1.0f : 0.88f,
                focused ? 1.0f : 0.93f,
                focused ? 1.0f : 0.98f,
                alpha * (focused ? 1.0f : 0.90f), 0.70f);
            DrawShadowedText(g_TextS, AugName(def),
                groupX + keyW + labelGap, nameY, nameScale,
                focused ? 1.0f : 0.82f,
                focused ? 1.0f : 0.90f,
                focused ? 1.0f : 0.98f,
                alpha * (focused ? 0.98f : 0.82f), 0.60f);
            BatchFlush();
        }
    }

    const float panelBaseA = enterE * sceneAlpha;
    DrawVisibleConstellLine(layout.panelX - 14.0f * layout.ui,
                            layout.panelY - 8.0f * layout.ui,
                            layout.panelX - 14.0f * layout.ui,
                            layout.panelY + layout.panelH,
                            1.35f * layout.ui,
                            stateR, stateG, stateB,
                            panelBaseA * (activeFocus >= 0 ? 0.50f : 0.20f));
    DrawVisibleConstellNode(layout.panelX - 14.0f * layout.ui,
                            layout.panelY - 8.0f * layout.ui,
                            3.2f * layout.ui,
                            stateR, stateG, stateB,
                            panelBaseA * 0.56f, true, true);
    BatchFlush();

    if (!inExit && activeFocus >= 0) {
        const int augIdx = g_GameManager.augChoices[activeFocus];
        if (augIdx >= 0 && augIdx < AUG_TOTAL) {
            const AugDef& def = ALL_AUGS[augIdx];
            const AugVisualFamily family = AugVisualFamilyOf(def);
            float rarityR, rarityG, rarityB;
            GetRarityColor(def.rarity, rarityR, rarityG, rarityB);
            const float a = panelT * panelBaseA;
            float y = layout.panelY;
            const wchar_t* panelKind = isDebuff ? L"TRIAL COST" : L"BUFF AUGMENT";
            g_TextS.Draw(panelKind, layout.panelX, y,
                         0.96f * layout.textUi,
                         stateR, stateG, stateB, a * 0.90f);
            wchar_t classText[128];
            swprintf_s(classText, L"%ls  //  %ls",
                       GetAugBadge(def), AugVisualFamilyLabel(family));
            const float classScale = 0.72f * layout.textUi;
            const float classW = g_TextS.Width(classText, classScale);
            g_TextS.Draw(classText,
                         layout.panelX + layout.panelW - classW, y,
                         classScale, rarityR, rarityG, rarityB, a * 0.84f);
            y += 56.0f * layout.ui;

            const wchar_t* name = AugName(def);
            float nameScale = 1.04f * layout.textUi;
            while (nameScale > 0.70f &&
                   g_TextL.Width(name, nameScale) > layout.panelW)
                nameScale -= 0.04f;
            DrawShadowedText(g_TextL, name, layout.panelX, y,
                             nameScale, 0.98f, 0.99f, 1.0f, a, 0.62f);
            y += 62.0f * layout.ui;

            // Keep the measurable effect in its own upper block. This is the
            // quick, objective answer to "what changes if I take it?".
            g_TextS.Draw(L"OBJECTIVE CHANGE", layout.panelX, y,
                         0.72f * layout.textUi,
                         0.56f, 0.68f, 0.82f, a * 0.78f);
            y += 38.0f * layout.ui;

            const wchar_t* stat = AugStat(def);
            float statScale = 0.98f * layout.textUi;
            while (statScale > 0.70f &&
                   g_TextS.Width(stat, statScale) > layout.panelW)
                statScale -= 0.03f;
            g_TextS.Draw(stat, layout.panelX, y, statScale,
                         stateR, stateG, stateB, a * 0.95f);
            y += 54.0f * layout.ui;

            DrawVisibleConstellLine(layout.panelX, y,
                                    layout.panelX + layout.panelW, y,
                                    0.75f * layout.ui,
                                    stateR, stateG, stateB, a * 0.28f);
            BatchFlush();
            y += 18.0f * layout.ui;

            AugImpactRow rows[3];
            const int rowCount = BuildAugImpactRows(def, rows, 3);
            if (rowCount > 0) {
                g_TextS.Draw(L"STAT DELTA", layout.panelX, y,
                             0.78f * layout.textUi,
                             0.56f, 0.68f, 0.82f, a * 0.72f);
                y += 46.0f * layout.ui;
                for (int row = 0; row < rowCount; ++row) {
                    wchar_t before[32], after[32], deltaText[32];
                    FormatAugMetricValue(before, 32, rows[row].metric,
                                         rows[row].before, false);
                    FormatAugMetricValue(after, 32, rows[row].metric,
                                         rows[row].after, false);
                    const float diff = rows[row].after - rows[row].before;
                    FormatAugMetricValue(deltaText, 32, rows[row].metric,
                                         diff, true);
                    const wchar_t* label = AugMetricLabel(rows[row].metric);
                    const float metricScale = 0.82f * layout.textUi;
                    g_TextS.Draw(label, layout.panelX, y, metricScale,
                                 0.70f, 0.80f, 0.90f, a * 0.86f);
                    wchar_t values[96];
                    swprintf_s(values, L"%ls  →  %ls", before, after);
                    const float valuesW = g_TextS.Width(values, metricScale);
                    g_TextS.Draw(values,
                        layout.panelX + layout.panelW * 0.70f - valuesW,
                        y, metricScale,
                        0.88f, 0.94f, 1.0f, a * 0.92f);
                    const float deltaW = g_TextS.Width(deltaText, metricScale);
                    const bool positive = diff > 0.0f;
                    g_TextS.Draw(deltaText,
                        layout.panelX + layout.panelW - deltaW,
                        y, metricScale,
                        positive ? 0.14f : 0.98f,
                        positive ? 0.84f : 0.16f,
                        positive ? 0.98f : 0.20f,
                        a * 0.98f);
                    y += 48.0f * layout.ui;
                }
            } else {
                g_TextS.Draw(L"NO DIRECT STAT CHANGE", layout.panelX, y,
                             0.72f * layout.textUi,
                             0.70f, 0.78f, 0.88f, a * 0.72f);
                y += 48.0f * layout.ui;
            }

            // The lower block is deliberately separated from the numbers.
            // It is the explanatory layer: mechanics, conditions, and flavor.
            const float lowerSectionY = layout.panelY + layout.panelH * 0.53f;
            float descriptionY = std::max(y + 22.0f * layout.ui,
                                          lowerSectionY);
            const float descriptionFloor =
                layout.panelY + layout.panelH - 150.0f * layout.ui;
            if (descriptionY > descriptionFloor)
                descriptionY = y + 22.0f * layout.ui;

            DrawVisibleConstellLine(layout.panelX,
                                    descriptionY - 18.0f * layout.ui,
                                    layout.panelX + layout.panelW,
                                    descriptionY - 18.0f * layout.ui,
                                    0.85f * layout.ui,
                                    stateR, stateG, stateB, a * 0.34f);
            DrawVisibleConstellNode(layout.panelX,
                                    descriptionY - 18.0f * layout.ui,
                                    2.4f * layout.ui,
                                    stateR, stateG, stateB,
                                    a * 0.46f, true, true);
            BatchFlush();

            const wchar_t* descriptionHeader =
                isDebuff ? L"ENEMY MODIFIER" : L"MECHANIC";
            g_TextS.Draw(descriptionHeader, layout.panelX, descriptionY,
                         0.82f * layout.textUi,
                         stateR, stateG, stateB, a * 0.86f);
            descriptionY += 48.0f * layout.ui;

            const float descScale = 0.86f * layout.textUi;
            const std::vector<std::wstring> lines =
                TarotWrap(def.locDesc[CurLangIdx()], descScale, layout.panelW);
            int drawn = 0;
            for (const std::wstring& line : lines) {
                if (drawn >= 4 ||
                    descriptionY > layout.panelY + layout.panelH - 64.0f * layout.ui)
                    break;
                g_TextS.Draw(line.c_str(), layout.panelX, descriptionY, descScale,
                             0.76f, 0.84f, 0.94f, a * 0.82f);
                descriptionY += 40.0f * layout.ui;
                ++drawn;
            }
            BatchFlush();
        }
    } else if (!inExit) {
        const wchar_t* guide = CurLangIdx() == 0
            ? L"1 / 2 / 3 또는 별자리를 선택"
            : (CurLangIdx() == 2
                ? L"1 / 2 / 3 または星座を選択"
                : L"SELECT A CONSTELLATION OR PRESS 1 / 2 / 3");
        g_TextS.Draw(guide, layout.panelX, layout.panelY,
                     0.48f * layout.textUi,
                     0.54f, 0.66f, 0.80f, panelBaseA * 0.72f);
        BatchFlush();
    }

    if (!inExit) {
        const wchar_t* controls = L"1 / 2 / 3  FOCUS     SPACE  CONFIRM";
        g_TextS.Draw(controls, layout.panelX, sh * 0.925f,
                     0.47f * layout.textUi,
                     0.54f, 0.66f, 0.82f, panelBaseA * 0.76f);
        BatchFlush();
    }

    if (inExit && g_AugExitT >= 0.55f && !particlesSpawned &&
        g_AugExitSlot >= 0 && g_AugExitSlot < count) {
        particlesSpawned = true;
        const AugDef& def = ALL_AUGS[g_GameManager.augChoices[g_AugExitSlot]];
        float r, g, b;
        GetRarityColor(def.rarity, r, g, b);
        for (int i = 0; i < 72; ++i) {
            const float angle = (float)i / 72.0f * 2.0f * (float)M_PI;
            const float speed = 120.0f + (float)(rand() % 300);
            particles[i] = { layout.orbitCX, layout.orbitCY,
                cosf(angle) * speed, sinf(angle) * speed,
                0.66f, r, g, b };
        }
        TriggerFlash(r, g, b, 0.78f);
    }
    if (particlesSpawned) {
        BindMainShader();
        for (auto& p : particles) {
            if (p.life <= 0.0f) continue;
            p.life -= delta;
            p.x += p.vx * delta;
            p.y += p.vy * delta;
            const float drag = std::max(0.0f, 1.0f - delta * 2.2f);
            p.vx *= drag;
            p.vy *= drag;
            const float a = AugSelectionClamp(p.life / 0.66f);
            drawDiamond(p.x, p.y, 4.2f * a + 0.8f,
                        p.r, p.g, p.b, a * 0.86f);
        }
        BatchFlush();
    }
}

void Scene_AugSelect(const SceneCtx& c) {
    Scene_AugSelectConstellationPolished(c);
}
static void Scene_AugReplaceCards(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh, delta = c.delta;
    const int newIdx = g_GameManager.pendingAugIdx;
    if (newIdx < 0 || newIdx >= AUG_TOTAL) return;

    static int seenSerial = -1;
    static float enterT = 0.0f;
    static float hoverT[32] = {};
    static float flashT[32] = {};
    static bool lmbPrev = false;
    static double prevMx = -1.0, prevMy = -1.0;

    if (seenSerial != g_GameManager.augmentSelectionSerial) {
        seenSerial = g_GameManager.augmentSelectionSerial;
        enterT = 0.0f;
        lmbPrev = false;
        prevMx = c.mx;
        prevMy = c.my;
        g_HoveredAug = -1;
        for (int i = 0; i < 32; ++i) { hoverT[i] = 0.0f; flashT[i] = 0.0f; }
    }
    enterT = std::min(1.0f, enterT + delta / 0.36f);
    const bool exiting = g_RepExitT >= 0.0f;
    const float exitT = exiting ? TarotEaseOut(TarotClamp01(g_RepExitT / 0.28f)) : 0.0f;
    const int n = std::max(0, std::min(32, g_GameManager.replaceChoiceCount));
    const int visible = std::min(n, 9);
    const bool pointerMoved = prevMx < 0.0 ||
        fabs(c.mx - prevMx) > 0.5 || fabs(c.my - prevMy) > 0.5;
    if (pointerMoved) g_GameManager.augmentKeyboardFocus = false;
    prevMx = c.mx;
    prevMy = c.my;

    const float ui = std::min(1.0f, std::min(sw / 1600.0f, sh / 900.0f));
    const float panelX = sw * 0.705f;
    const float panelW = std::max(230.0f, sw - panelX - 24.0f);
    const float cardAreaW = std::max(420.0f, panelX - 34.0f);
    const int cols = std::max(1, std::min(3, visible));
    const int rows = std::max(1, (visible + cols - 1) / cols);
    const float gap = 16.0f * ui;
    const float cardW = std::min(225.0f * ui, (cardAreaW - gap * (cols - 1)) / cols);
    const float cardH = std::min(302.0f * ui, (sh * 0.64f - gap * (rows - 1)) / rows);
    const float gridW = cardW * cols + gap * (cols - 1);
    const float baseX = (cardAreaW - gridW) * 0.5f;
    const float baseY = sh * 0.255f;

    auto PoseFor = [&](int i, float yOffset = 0.0f) {
        const int col = i % cols;
        const int row = i / cols;
        TarotCardPose p;
        p.cx = baseX + col * (cardW + gap) + cardW * 0.5f;
        p.cy = baseY + row * (cardH + gap) + cardH * 0.5f + yOffset;
        p.w = cardW;
        p.h = cardH;
        p.angle = 0.0f;
        return p;
    };

    if (!exiting && enterT > 0.95f &&
        (!g_GameManager.augmentKeyboardFocus || pointerMoved)) {
        int hover = -1;
        for (int i = visible - 1; i >= 0; --i)
            if (TarotHit(PoseFor(i), c.mx, c.my, 10.0f)) { hover = i; break; }
        g_HoveredAug = hover;
    }
    if (g_HoveredAug >= 0 && g_HoveredAug < 32 &&
        g_HoveredAug < g_GameManager.replaceChoiceCount)
        flashT[g_HoveredAug] = std::max(flashT[g_HoveredAug], 0.18f);
    for (int i = 0; i < 32; ++i) {
        const float target = (!exiting && g_HoveredAug == i) ? 1.0f : 0.0f;
        hoverT[i] += (target - hoverT[i]) * std::min(1.0f, delta * 13.0f);
        flashT[i] = std::max(0.0f, flashT[i] - delta);
    }
    if (!exiting && c.lmb && !lmbPrev && g_HoveredAug >= 0 &&
        g_HoveredAug < visible) {
        g_RepExitT = 0.0f;
        g_RepExitSlot = g_HoveredAug;
    }
    lmbPrev = c.lmb;

    const float enterE = TarotEaseOut(enterT);
    const float overlay = enterE * (1.0f - exitT);
    BindMainShader();
    drawRect(0, 0, sw, sh, 0.025f, 0.010f, 0.045f, overlay * 0.78f);
    drawConstellFrame(16.0f, 16.0f, sw - 32.0f, sh - 32.0f,
                      0.72f, 0.30f, 0.96f, overlay * 0.60f,
                      24.0f, 8.0f, overlay * 0.24f, enterT);
    BatchFlush();

    const wchar_t* title = L"IDENTITY SLOT // REPLACE";
    g_TextL.Draw(title, 24.0f, sh * 0.075f, 0.92f,
                 1.0f, 0.96f, 1.0f, overlay * 0.96f);
    wchar_t newLine[160];
    swprintf_s(newLine, L"NEW  [%ls] %ls", GetAugBadge(ALL_AUGS[newIdx]),
               AugName(ALL_AUGS[newIdx]));
    g_TextS.Draw(newLine, 26.0f, sh * 0.125f, 0.66f,
                 0.82f, 0.92f, 1.0f, overlay * 0.88f);
    BatchFlush();

    for (int i = 0; i < visible; ++i) {
        const int idx = g_GameManager.replaceChoices[i];
        if (idx < 0 || idx >= AUG_TOTAL) continue;
        const AugDef& def = ALL_AUGS[idx];
        float r, g, b;
        GetRarityColor(def.rarity, r, g, b);
        const float stagger = TarotEaseOut(TarotClamp01((enterT - i * 0.05f) / 0.72f));
        TarotCardPose p = PoseFor(i, (1.0f - stagger) * 54.0f);
        const float hT = TarotEaseOut(hoverT[i]);
        p.cy -= hT * 14.0f;
        p.w *= 1.0f + hT * 0.045f;
        p.h *= 1.0f + hT * 0.045f;
        const bool selected = exiting && g_RepExitSlot == i;
        const bool other = exiting && !selected;
        float alpha = stagger * (g_HoveredAug >= 0 && g_HoveredAug != i ? 0.42f : 1.0f);
        if (selected) {
            p.cx += (sw * 0.50f - p.cx) * exitT;
            p.cy += (sh * 0.49f - p.cy) * exitT;
            p.w *= 1.0f + exitT * 0.25f;
            p.h *= 1.0f + exitT * 0.25f;
            alpha *= 1.0f - exitT * 0.78f;
        } else if (other) {
            p.cy += (i & 1 ? -1.0f : 1.0f) * exitT * 30.0f;
            alpha *= 1.0f - exitT;
        }
        DrawTarotCardSurface(p, r, g, b, alpha, hT, flashT[i], selected,
                             (float)glfwGetTime());
        const float left = p.cx - p.w * 0.5f;
        const float top = p.cy - p.h * 0.5f;
        wchar_t key[16];
        swprintf_s(key, L"[%d]", i + 1);
        g_TextS.Draw(key, left + 14.0f, top + 11.0f, 0.54f,
                     0.80f, 0.86f, 1.0f, alpha * 0.86f);
        const wchar_t* name = AugName(def);
        float nameScale = 0.68f;
        while (nameScale > 0.44f && g_TextL.Width(name, nameScale) > p.w - 26.0f)
            nameScale -= 0.04f;
        const float nameW = g_TextL.Width(name, nameScale);
        g_TextL.Draw(name, p.cx - nameW * 0.5f, top + 38.0f,
                     nameScale, 1.0f, 1.0f, 1.0f, alpha * 0.96f);
        BatchFlush();
        DrawTarotEmblem(def, p.cx, p.cy - 8.0f, p.w * 0.40f,
                        r, g, b, alpha * 0.86f, (float)glfwGetTime());
        const wchar_t* stat = AugStat(def);
        const float statW = g_TextS.Width(stat, 0.46f);
        g_TextS.Draw(stat, p.cx - statW * 0.5f, top + p.h - 46.0f,
                     0.46f, 0.72f, 1.0f, 0.82f, alpha * 0.84f);
        BatchFlush();
    }

    const float detailY = sh * 0.245f;
    DrawTarotDetailPanel(ALL_AUGS[newIdx], panelX, detailY, panelW,
                         std::min(sh * 0.48f, 390.0f), overlay * 0.96f);
    const wchar_t* hint = L"1-9 SELECT   SPACE REPLACE   ESC CANCEL";
    g_TextS.Draw(hint, 24.0f, sh * 0.92f, 0.64f,
                 0.62f, 0.72f, 0.88f, overlay * 0.80f);
    BatchFlush();
}

void Scene_AugReplace(const SceneCtx& c) {
    Scene_AugReplaceCards(c);
}
void Scene_OwnedAugPanel(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    GLFWwindow* window = c.window;
    const GameState st = g_GameManager.currentState;
    float& fireTimer = *c.fireTimer;
    const std::function<void()>& ResetForNewGame = c.reset;
                // 같은 인덱스 카운트 (스택)
                int counts[AUG_TOTAL] = {};
                for (int idx : g_OwnedAugs) counts[idx]++;
                // 보유 증강을 티어(등급)순으로 정렬
                int ord[AUG_TOTAL], nord = 0;
                for (int i = 0; i < AUG_TOTAL; i++) if (counts[i] > 0) ord[nord++] = i;
                std::sort(ord, ord + nord, [](int a, int b) {
                    return AugTierIndexLess(a, b);
                });

                const float PX  = 16.0f;
                const float ROW_H = 24.0f;
                const float HDR_H = 20.0f;
                const float COLW  = 320.0f;          // 리스트 클릭/호버 가로 범위
                const wchar_t* TITLE = T(StrId::OWNED_AUGS);
                g_TextS.Draw(TITLE, PX, 60.0f, 1.1f, 1, 1, 1, 0.95f);
                // 현재 무기 (항상 표시)
                {
                    const wchar_t* wPrefix = (g_Language==Language::EN)?L"Weapon:":
                                             (g_Language==Language::JP)?L"武器:":
                                             L"현재 무기:";
                    wchar_t wLine[160];
                    swprintf_s(wLine, L"%ls %ls", wPrefix, CurrentWeaponLabel());
                    float wSc = 0.78f;
                    while (wSc > 0.62f && g_TextS.Width(wLine, wSc) > COLW - 4.0f) wSc -= 0.03f;
                    g_TextS.Draw(wLine, PX + 2.0f, 90.0f, wSc,
                                 0.55f, 0.85f, 1.0f, 0.92f);
                }

                // 리스트 뷰 영역 — 하단 스킬/HP HUD 바로 위까지 (넘치면 스크롤)
                const float listTop    = 110.0f;
                float listBottom = sh - 175.0f;               // 스킬 슬롯/HP 패널 위까지
                if (listBottom < listTop + 4.0f * ROW_H) listBottom = listTop + 4.0f * ROW_H;
                const float viewH      = listBottom - listTop;
                int hdrCount = 0;
                AugRarity prevR = (AugRarity)-1;
                for (int oi = 0; oi < nord; oi++) {
                    AugRarity r = ALL_AUGS[ord[oi]].rarity;
                    if (r != prevR) { hdrCount++; prevR = r; }
                }
                const float contentH   = (float)nord * ROW_H + (float)hdrCount * HDR_H;

                // 마우스 휠 스크롤 (리스트 위에서만 소비)
                static float s_ownScroll = 0.0f;
                bool overList = (mx >= 0 && mx <= COLW && my >= listTop && my <= listBottom);
                if (overList && g_ScrollAccum != 0.0f)
                    s_ownScroll -= g_ScrollAccum * ROW_H * 1.5f;
                g_ScrollAccum = 0.0f;   // 매 프레임 소비 (다른 곳에서 안 쓰면 무시)
                float maxScroll = (contentH > viewH) ? (contentH - viewH) : 0.0f;
                if (s_ownScroll < 0.0f)        s_ownScroll = 0.0f;
                if (s_ownScroll > maxScroll)   s_ownScroll = maxScroll;

                // 리스트 (scissor 클립 + 스크롤)
                int   hoverAug = -1;
                float hoverRowY = 0.0f;
                BatchFlush(); glEnable(GL_SCISSOR_TEST);
                glScissor(0, (GLint)(sh - listBottom), (GLint)(COLW + 10.0f), (GLint)viewH);
                prevR = (AugRarity)-1;
                float ry = listTop - s_ownScroll;
                for (int oi = 0; oi < nord; oi++) {
                    int i = ord[oi];
                    AugRarity rar = ALL_AUGS[i].rarity;
                    if (rar != prevR) {
                        if (ry >= listTop - HDR_H && ry <= listBottom) {
                            wchar_t rh[48];
                            swprintf_s(rh, L"-- %ls --", GetRarityKR(rar));
                            g_TextS.Draw(rh, PX, ry, 0.72f, 0.55f, 0.75f, 0.95f, 0.88f);
                        }
                        ry += HDR_H;
                        prevR = rar;
                    }
                    if (ry < listTop - ROW_H || ry > listBottom) { ry += ROW_H; continue; }
                    const AugDef& def = ALL_AUGS[i];
                    float cr, cg, cb;
                    GetRarityColor(def.rarity, cr, cg, cb);
                    cr = std::min(1.0f, cr * 1.3f + 0.25f);
                    cg = std::min(1.0f, cg * 1.3f + 0.25f);
                    cb = std::min(1.0f, cb * 1.3f + 0.25f);
                    bool rowHover = (overList && my >= ry - 2.0f && my < ry + ROW_H - 4.0f);
                    if (rowHover) {
                        hoverAug = i; hoverRowY = ry;
                        BindMainShader();
                        drawRect(0, ry - 2.0f, COLW, ROW_H, 0.15f, 0.16f, 0.26f, 0.6f);
                        drawRect(0, ry - 2.0f, 3.0f, ROW_H, cr, cg, cb, 1.0f);
                    }
                    wchar_t line[128];
                    if (counts[i] > 1)
                        swprintf_s(line, L"· [%ls] %ls  ×%d", GetAugBadge(def), AugName(def), counts[i]);
                    else
                        swprintf_s(line, L"· [%ls] %ls", GetAugBadge(def), AugName(def));
                    float rowSc = 0.82f;
                    while (rowSc > 0.62f && g_TextS.Width(line, rowSc) > COLW - PX - 10.0f) rowSc -= 0.03f;
                    g_TextS.Draw(line, PX, ry, rowSc, cr, cg, cb, 0.9f);
                    ry += ROW_H;
                }
                BatchFlush(); glDisable(GL_SCISSOR_TEST);

                // 스크롤바 (내용이 넘칠 때만)
                if (maxScroll > 0.0f) {
                    BindMainShader();
                    float trackX = COLW + 2.0f;
                    drawRect(trackX, listTop, 4.0f, viewH, 0.12f, 0.12f, 0.16f, 0.6f);
                    float thumbH = viewH * (viewH / contentH);
                    float thumbY = listTop + (viewH - thumbH) * (s_ownScroll / maxScroll);
                    drawRect(trackX, thumbY, 4.0f, thumbH, 0.5f, 0.6f, 0.8f, 0.9f);
                }

                // 무기 줄 호버 (리스트 y=110 이전 — 증강 행과 분리)
                const float WEAPON_Y = 90.0f;
                bool overWeapon = (mx >= 0 && mx <= COLW &&
                                   my >= WEAPON_Y - 4.0f && my <= WEAPON_Y + 18.0f);

                // 우측 상세 패널 — 증강 행: 증강 설명 / 무기 줄: 무기 설명
                auto wrapDescLines = [&](const wchar_t* src, float dsc, float dWmax,
                                         std::vector<std::wstring>& out) {
                    std::vector<std::wstring> dl; std::wstring cur2;
                    for (const wchar_t* p = src; *p; ++p) {
                        if (*p == L'/') { if (!cur2.empty()) dl.push_back(cur2); cur2.clear(); }
                        else cur2 += *p;
                    }
                    if (!cur2.empty()) dl.push_back(cur2);
                    out.clear();
                    for (auto& ln : dl) {
                        while (!ln.empty() && ln.front() == L' ') ln.erase(0, 1);
                        if (g_TextS.Width(ln.c_str(), dsc) <= dWmax) { out.push_back(ln); continue; }
                        std::wstring acc, word;
                        auto fw = [&]() {
                            if (word.empty()) return;
                            std::wstring tr = acc.empty() ? word : acc + L" " + word;
                            if (g_TextS.Width(tr.c_str(), dsc) > dWmax && !acc.empty()) {
                                out.push_back(acc); acc = word;
                            } else acc = tr;
                            word.clear();
                        };
                        for (wchar_t ch : ln) { if (ch == L' ') fw(); else word += ch; }
                        fw();
                        if (!acc.empty()) out.push_back(acc);
                    }
                };

                auto drawSidePanel = [&](float BY, float br, float bg, float bb,
                                         const wchar_t* title, const wchar_t* descSrc) {
                    const float BW = std::min(380.0f, sw - (COLW + 30.0f));
                    const float BH = 138.0f;
                    float BX = COLW + 18.0f;
                    if (BY + BH > sh - 20.0f) BY = sh - 20.0f - BH;
                    if (BY < 20.0f) BY = 20.0f;
                    BindMainShader();
                    drawRect(BX, BY, BW, BH, 0.03f, 0.03f, 0.06f, 0.95f);
                    drawRect(BX, BY, BW, 4.0f, br, bg, bb, 1.0f);
                    g_TextS.Draw(title, BX + 12.0f, BY + 12.0f, 0.90f,
                                 std::min(1.0f, br * 1.4f + 0.3f),
                                 std::min(1.0f, bg * 1.4f + 0.3f),
                                 std::min(1.0f, bb * 1.4f + 0.3f), 1.0f);
                    const float dsc = 0.76f, dWmax = BW - 24.0f;
                    std::vector<std::wstring> wrapped;
                    wrapDescLines(descSrc, dsc, dWmax, wrapped);
                    float dy = BY + 40.0f;
                    for (auto& w : wrapped) {
                        if (dy > BY + BH - 14.0f) break;
                        g_TextS.Draw(w.c_str(), BX + 12.0f, dy, dsc, 1.0f, 1.0f, 0.95f, 0.90f);
                        dy += 22.0f;
                    }
                };

                if (hoverAug >= 0) {
                    const AugDef& sd = ALL_AUGS[hoverAug];
                    float hr, hg, hb;
                    GetRarityColor(sd.rarity, hr, hg, hb);
                    wchar_t hd[128];
                    swprintf_s(hd, L"[%ls] %ls", GetAugBadge(sd), AugName(sd));
                    drawSidePanel(hoverRowY - 6.0f, hr, hg, hb, hd, AugDesc(sd));
                } else if (overWeapon) {
                    drawSidePanel(WEAPON_Y - 4.0f, 0.35f, 0.75f, 1.0f,
                                  CurrentWeaponLabel(), CurrentWeaponDescText());
                }
}

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
    case GS::SHOP:
        s_ShopBackRequested = true;
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
