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
#include "../../System/WindowFx.h"
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
static bool  s_MainMenuTrialSelectPanel = false;
static bool  s_MainMenuResumeFromPanel = false;
static void Scene_RunConfigInline(const SceneCtx& c);
static void Scene_TrialSelectInline(const SceneCtx& c);
static void Scene_SettingsInline(const SceneCtx& c);
static int   s_RcWeapon        = 0;
static bool  s_RcTrialNodes[6] = {};
static float s_RcNodeHover[6]  = {};
static float s_RcTrialTypeT[6] = {};
static int   s_TrialTargetCount = 1;
// PLAY owns a full trial catalogue.  The gameplay system still has four
// slots; this UI state is the 20-item catalogue selection before PLAY packs
// the enabled definitions into those slots.
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
    s_TrialTargetCount = 1;
    ResetTrials();
    s_RcWeapon = 0;
    s_MainMenuTrialSelectPanel = false;
    s_RcTrialStateLoaded = false;
    for (int i = 0; i < TRIAL_DEF_COUNT; ++i) s_RcTrialEnabled[i] = false;
    for (int i = 0; i < 6; ++i) { s_RcTrialNodes[i] = false; s_RcNodeHover[i] = 0.0f; s_RcTrialTypeT[i] = 0.0f; }
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
        if (key == CM_SPAWNER) previewScale *= 1.18f;
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
    if (s_MainMenuTrialSelectPanel) {
        Scene_TrialSelectInline(c);
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
    static const wchar_t* kMenuRoutes[3][5] = {
        { L"플레이", L"상점", L"도감", L"설정", L"종료" },
        { L"PLAY", L"SHOP", L"ASTRAL LOG", L"SETTINGS", L"EXIT" },
        { L"スタート", L"ショップ", L"星界記録", L"設定", L"終了" },
    };
    const int   kBtnCount = 5;
    const float BW = std::min(560.0f, std::max(420.0f, sw * 0.34f));
    const float BH = 70.0f;
    const float BGAP = 15.0f;
    float totalBH = kBtnCount * BH + (kBtnCount - 1) * BGAP;
    float btnX0   = std::max(58.0f, sw * 0.075f);
    // The lobby command rail belongs to the lower-left corner, matching the
    // navigation language used by the other pages. Keep a responsive bottom
    // margin, but never let the rail climb into the logo on short windows.
    float btnY0 = sh - totalBH - std::max(52.0f, sh * 0.075f);
    const float logoFloor = MainLogoTop(sh) + MainLogoHeight(sh) * 0.82f;
    if (btnY0 < logoFloor) btnY0 = logoFloor;

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
        const wchar_t* route = kMenuRoutes[li2][i];
        // Lobby commands do not need a translated explanatory subtitle.
        // Keep the primary command localized while using the compact English
        // identifier as its consistent secondary label in Korean mode.
        const wchar_t* sub = (li2 == 0) ? kBtns[i].label[1] : kBtns[i].label[li2];
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
    const bool systemBackdrop = ConfigureWindowBackdropBlur(
        c.window, g_BackdropBlurEnabled);
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
    static int s_blurW = 0;
    static int s_blurH = 0;
    static int s_blurCadence = 0;
    static bool s_blurCacheValid = false;
    static bool s_blurWasEnabled = false;
    if (g_BackdropBlurEnabled && !systemBackdrop) {
        const int blurW = std::max(1, (int)sw);
        const int blurH = std::max(1, (int)sh);
        const bool sizeChanged = s_blurW != blurW || s_blurH != blurH;
        if (sizeChanged) {
            InitBlurSystem(blurW, blurH);
            s_blurW = blurW;
            s_blurH = blurH;
            s_blurCacheValid = false;
        }
        const bool transitionActive = wake < 0.995f
                                   || oldMenuA < 0.995f
                                   || s_backExit;
        const bool refresh = !s_blurCacheValid
                          || !s_blurWasEnabled
                          || transitionActive
                          || ((s_blurCadence++ & 3) == 0);
        if (refresh) {
            CaptureBackdrop();
            s_blurCacheValid = true;
        }
        s_blurWasEnabled = true;
        DrawBlurPanel(0.0f, 0.0f, sw, sh,
                      0.16f * std::max(oldMenuA, wake),
                      0.004f, 0.010f, 0.020f);
    } else {
        s_blurCacheValid = false;
        s_blurWasEnabled = false;
        s_blurCadence = 0;
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
        case AugType::SNIPER:
        case AugType::SMG_COMPRESSOR:
        case AugType::SNIPER_AMPLIFIER:
        case AugType::SKILL_FOCUS:
        case AugType::REVOLVER_OVERLOAD:
        case AugType::REVOLVER_SILVER:
            return 0;
        case AugType::STATIC_FIELD:
        case AugType::STATIC_FIELD_2:
        case AugType::EMP_PULSE:
        case AugType::PATCH_MINE:
        case AugType::TRAP_EXE:
        case AugType::POPUP_ALLY:
        case AugType::GLUE_SYNC:
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

    // SHOP_CODEX_RENDERER
    // SHOP keeps the shared left command rail. Its catalogue constellation
    // occupies the upper middle, while the purchase copy stays in the lower
    // right so the two jobs never compete for the same space.
    {
        const float designA = std::max(0.0f, std::min(1.0f, wake));
        const float detailA = std::max(0.0f, std::min(1.0f, rightWake * s_detailT));
        const float ui = std::max(0.62f, std::min(sw * 0.90f / 1800.0f,
                                                  sh * 0.86f / 1020.0f));
        const float cyanR = 0.30f, cyanG = 0.86f, cyanB = 1.00f;
        const float goldR = 1.00f, goldG = 0.76f, goldB = 0.24f;
        const float whiteR = 0.88f, whiteG = 0.94f, whiteB = 1.00f;
        const bool lmbClick = lmb && !g_LmbPrev;

        auto drawFit = [&](const wchar_t* text, float x, float y, float scale,
                           float maxW, float r, float g2, float b, float a) {
            if (!text || !text[0] || maxW <= 2.0f) return;
            float sc = scale;
            const float tw = g_TextS.Width(text, sc);
            if (tw > maxW && tw > 0.0f) sc *= maxW / tw;
            DrawShadowedText(g_TextS, text, x, y, sc, r, g2, b,
                             a * designA, 0.72f);
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
        auto drawFixedAction = [&](float x, float y, float w, const wchar_t* label,
                                   float r, float g2, float b,
                                   bool enabled, bool hovered) {
            const float aa = enabled ? (hovered ? 1.0f : 0.72f) : 0.28f;
            drawFit(label, x, y - 25.0f * ui, 0.76f * ui, w,
                    0.94f, 0.98f, 1.00f, enabled ? 0.98f : 0.44f);
            DrawVisibleConstellLine(x, y + 14.0f * ui,
                                    x + w * 0.70f, y + 14.0f * ui,
                                    2.2f * ui, r, g2, b, aa * designA);
            DrawVisibleConstellNode(x + w * 0.70f, y + 14.0f * ui,
                                    5.2f * ui, r, g2, b, aa * designA, false);
        };

        DrawSceneLeftVignette(sw, sh, 0.06f * designA);

        // Exact shared command rail used by the other outgame pages.
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
        const float railY = rootY;
        const float railW = rootW;
        const float railH = rootH;
        const float categoryWake = wake * categoryA;
        const int rootCount = SHOP_TAB_COUNT + 1;
        for (int i = 0; i < rootCount; ++i) {
            const bool isBack = i == SHOP_TAB_COUNT;
            const float tx = railX - 58.0f * itemA;
            const float ty = railY + (float)i * (railH + rootGap);
            const bool hov = inputReady
                          && mx >= tx - 26.0f * ui
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
            const float selectPulse = selected
                ? 0.16f + 0.16f * sinf(now * 4.0f) : 0.0f;
            const float slide = (1.0f - reveal) * 34.0f;
            DrawUnifiedMenuCommand(route, sub,
                                   tx + slide - 10.0f * hovT, ty,
                                   railW, railH, rr, gg, bb, rowA, hovT,
                                   selected, selectPulse,
                                   now + (float)i * 0.17f);
        }

        // The chart sits above the fold, between the shared rail and the
        // detail copy. This deliberately follows the archive constellation
        // language rather than the previous SHOP carousel.
        const float chartCX = sw * 0.58f;
        const float chartCY = sh * 0.315f;
        const float chartR = std::min(sw * 0.245f, sh * 0.285f);
        const float detailX = sw * 0.68f;
        const float detailW = std::max(360.0f * ui,
                                       std::min(sw * 0.27f,
                                                sw - detailX - 42.0f * ui));
        const float infoY = sh * 0.675f;

        // Contrast is local to the lower-right copy, never a full page plate.
        if (g_ConstellationCircleTex && designA > 0.001f) {
            const float circleR = std::min(sw, sh) * 0.39f;
            const float circleCX = sw * 0.91f;
            const float circleCY = sh * 0.92f;
            BatchFlush();
            DrawIcon(g_ConstellationCircleTex,
                     circleCX - circleR, circleCY - circleR,
                     circleR * 2.0f, circleR * 2.0f,
                     0.0f, 0.0f, 0.012f, 0.80f * designA);
        }

        float detailR = cyanR, detailG = cyanG, detailB = cyanB;
        const wchar_t* detailTitle = L"NO MODULE";
        const wchar_t* detailType = L"EMPTY CATALOGUE";
        bool detailIsMeta = false, detailIsTheme = false, detailIsAug = false;
        int detailId = -1;
        if (s_selKey >= 0) {
            if (s_selKey < KEY_THEME) {
                detailIsMeta = true; detailId = s_selKey - KEY_META;
                if (detailId >= 0 && detailId < META_COUNT) {
                    detailTitle = MetaName(detailId);
                    detailType = L"CORE MODULE";
                }
            } else if (s_selKey < KEY_AUG) {
                detailIsTheme = true; detailId = s_selKey - KEY_THEME;
                if (detailId >= 0 && detailId < ACCENT_COUNT) {
                    detailTitle = AccentName(detailId);
                    detailType = L"SIGNAL PROFILE";
                    detailR = ACCENT_THEMES[detailId].r;
                    detailG = ACCENT_THEMES[detailId].g;
                    detailB = ACCENT_THEMES[detailId].b;
                }
            } else {
                detailIsAug = true; detailId = s_selKey - KEY_AUG;
                if (detailId >= 0 && detailId < AUG_TOTAL) {
                    detailTitle = ALL_AUGS[detailId].locName[li];
                    detailType = L"FIELD PAYLOAD";
                    GetRarityColor(ALL_AUGS[detailId].rarity,
                                   detailR, detailG, detailB);
                }
            }
        }

        int itemIndices[300] = {};
        int itemTotal = 0;
        int selectedPos = 0;
        for (int i = 0; i < s_itemCount; ++i) {
            if (s_items[i].isGroup) continue;
            if (itemTotal < 300) {
                itemIndices[itemTotal] = i;
                if (s_items[i].key == s_selKey) selectedPos = itemTotal;
                ++itemTotal;
            }
        }

        float selectedNodeX = chartCX;
        float selectedNodeY = chartCY;
        if (itemTotal > 0) {
            const int maxVisible = 7;
            const float rowStep = std::min(72.0f * ui, sh * 0.066f);
            const bool overChart = inputReady
                && mx >= chartCX - chartR * 1.45f
                && mx <= chartCX + chartR * 0.80f
                && my >= chartCY - rowStep * 4.5f
                && my <= chartCY + rowStep * 4.5f;
            if (overChart && g_ScrollAccum != 0.0f) {
                const int direction = g_ScrollAccum < 0.0f ? 1 : -1;
                selectedPos = (selectedPos + direction + itemTotal) % itemTotal;
                s_selKey = s_items[itemIndices[selectedPos]].key;
                s_detailT = 0.0f;
            }
            g_ScrollAccum = 0.0f;
            int currentSelectedPos = 0;
            for (int i = 0; i < itemTotal; ++i)
                if (s_items[itemIndices[i]].key == s_selKey) currentSelectedPos = i;

            const int constellationKind = s_selKey >= KEY_AUG ? 2 : 1;
            DrawConstellationDisc(chartCX, chartCY, chartR * 1.10f,
                                  0.0f, 0.0f, 0.012f, 0.22f * detailA);
            DrawConstellationDisc(chartCX, chartCY, chartR * 0.70f,
                                  0.10f, 0.17f, 0.22f, 0.08f * detailA);
            DrawArchiveConstellation(chartCX, chartCY, chartR * 0.64f,
                                     constellationKind, s_selKey,
                                     now * 0.18f, detailR, detailG, detailB,
                                     0.94f * detailA, ui);
            drawModuleGlyph(s_selKey, chartCX, chartCY,
                            58.0f * ui, detailR, detailG, detailB, 0.92f);
            drawDiamond(chartCX, chartCY, 4.0f * ui,
                        whiteR, whiteG, whiteB, 0.86f * detailA);

            // Product entries rise along the chart's left edge, like the
            // archive's observation rail, but use shop item names and colour.
            float lastX = 0.0f, lastY = 0.0f;
            bool hasLast = false;
            for (int slot = 0; slot < itemTotal; ++slot) {
                float rel = (float)slot - (float)currentSelectedPos;
                while (rel > (float)itemTotal * 0.5f) rel -= (float)itemTotal;
                while (rel < -(float)itemTotal * 0.5f) rel += (float)itemTotal;
                if (std::fabs(rel) > (float)(maxVisible / 2)) continue;
                const int itemIndex = itemIndices[slot];
                const SLItem& item = s_items[itemIndex];
                const bool selected = item.key == s_selKey;
                if (selected) {
                    selectedNodeX = chartCX;
                    selectedNodeY = chartCY;
                    continue;
                }
                const float nodeY = chartCY + rel * rowStep;
                const float nodeX = chartCX - chartR * (0.74f + 0.035f * std::fabs(rel));
                const float miniR = (selected ? 38.0f : 22.0f) * ui;
                const float labelX = nodeX + miniR + 20.0f * ui;
                const float labelW = std::min(260.0f * ui, sw * 0.16f);
                const bool hov = inputReady
                              && mx >= nodeX - 34.0f * ui
                              && mx <= labelX + labelW
                              && my >= nodeY - 38.0f * ui
                              && my <= nodeY + 38.0f * ui;
                s_itemHov[itemIndex] = UpdateMenuCommandHover(s_itemHov[itemIndex], hov, dt);
                if (hov && lmbClick) {
                    s_selKey = item.key;
                    s_detailT = 0.0f;
                }
                const float active = s_itemHov[itemIndex];
                if (hasLast)
                    DrawVisibleConstellLine(lastX, lastY, nodeX, nodeY,
                                            0.58f * ui, item.r, item.g, item.b,
                                            0.18f * designA);
                DrawConstellationDisc(nodeX, nodeY, miniR * 2.5f,
                                      0.0f, 0.0f, 0.012f,
                                      (0.10f + 0.10f * active) * designA);
                DrawArchiveConstellation(nodeX, nodeY, miniR, 1, item.key,
                                         now * 0.18f, item.r, item.g, item.b,
                                         (0.34f + 0.30f * active) * designA, ui);
                drawModuleGlyph(item.key, nodeX, nodeY, 17.0f * ui,
                                item.r, item.g, item.b,
                                0.38f + 0.28f * active);
                const float lineEnd = labelX - 9.0f * ui;
                DrawVisibleConstellLine(nodeX + miniR * 0.70f, nodeY,
                                        lineEnd, nodeY, 0.65f * ui,
                                        item.r, item.g, item.b,
                                        (0.18f + 0.18f * active) * designA);
                DrawVisibleConstellNode(lineEnd, nodeY, 2.6f * ui,
                                        item.r, item.g, item.b,
                                        (0.34f + 0.22f * active) * designA, false);
                drawFit(item.label, labelX, nodeY - 11.0f * ui,
                        0.43f * ui, labelW,
                        item.r, item.g, item.b,
                        0.48f + 0.26f * active);
                const wchar_t* kind = item.key < KEY_THEME ? L"CORE"
                                     : (item.key < KEY_AUG ? L"PROFILE" : L"PAYLOAD");
                DrawShadowedText(g_TextS, kind, labelX, nodeY + 12.0f * ui,
                                 0.27f * ui, item.r, item.g, item.b,
                                 (0.30f + 0.22f * active) * designA, 0.58f);
                lastX = nodeX; lastY = nodeY; hasLast = true;
            }
            DrawVisibleConstellLine(chartCX - chartR * 0.74f, chartCY,
                                    chartCX - chartR * 0.18f, chartCY,
                                    0.82f * ui, detailR, detailG, detailB,
                                    0.30f * detailA);
        }

        // Selected constellation to lower-right detail copy.
        DrawVisibleConstellLine(selectedNodeX + chartR * 0.45f,
                                selectedNodeY + chartR * 0.30f,
                                detailX - 24.0f * ui, infoY - 18.0f * ui,
                                0.78f * ui, detailR, detailG, detailB,
                                0.30f * detailA);
        DrawVisibleConstellNode(detailX - 24.0f * ui, infoY - 18.0f * ui,
                                3.6f * ui, detailR, detailG, detailB,
                                0.72f * detailA, false);
        DrawShadowedText(g_TextS, detailType, detailX, infoY,
                         0.38f * ui, detailR, detailG, detailB,
                         0.88f * detailA, 0.62f);
        float titleSc = 0.86f * ui;
        while (titleSc > 0.62f * ui && g_TextL.Width(detailTitle, titleSc) > detailW)
            titleSc -= 0.04f * ui;
        DrawShadowedText(g_TextL, detailTitle, detailX, infoY + 29.0f * ui,
                         titleSc, whiteR, whiteG, whiteB, 0.98f * detailA, 0.72f);
        DrawVisibleConstellLine(detailX, infoY + 68.0f * ui,
                                detailX + detailW * 0.78f, infoY + 68.0f * ui,
                                0.72f * ui, detailR, detailG, detailB,
                                0.32f * detailA);

        const float copyY = infoY + 91.0f * ui;
        if (detailIsMeta) {
            DrawShadowedText(g_TextS, L"PERMANENT MODULE / APPLIES BEFORE DEPLOYMENT",
                             detailX, copyY, 0.37f * ui,
                             0.76f, 0.84f, 0.92f, 0.76f * detailA, 0.58f);
        } else if (detailIsTheme) {
            DrawShadowedText(g_TextS, L"SIGNAL PROFILE / ROUTES THE OUTGAME PALETTE",
                             detailX, copyY, 0.37f * ui,
                             0.76f, 0.84f, 0.92f, 0.76f * detailA, 0.58f);
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
                    drawFit(segment.c_str(), detailX, copyY + line * 22.0f * ui,
                            0.40f * ui, detailW - 20.0f * ui,
                            0.76f, 0.84f, 0.94f, 0.82f);
                    ++line;
                }
            }
        }

        const float tradeY = infoY + 174.0f * ui;
        const float priceX = detailX;
        const float actionX = detailX + detailW * 0.46f;
        const float actionW = std::max(170.0f * ui, detailW * 0.50f);
        if (detailIsMeta && detailId >= 0 && detailId < META_COUNT) {
            const MetaDef& md = META_DEFS[detailId];
            const int curLv = g_MetaLv[detailId];
            const long long cost = MetaNextCost(detailId);
            wchar_t levelBuf[48]; swprintf_s(levelBuf, L"LEVEL  %d / %d", curLv, md.maxLv);
            DrawShadowedText(g_TextS, levelBuf, priceX, tradeY - 31.0f * ui,
                             0.40f * ui, whiteR, whiteG, whiteB,
                             0.80f * detailA, 0.60f);
            wchar_t costBuf[64];
            if (cost < 0) swprintf_s(costBuf, L"MAX LEVEL");
            else swprintf_s(costBuf, L"%lld  STARDUST", cost);
            DrawShadowedText(g_TextS, costBuf, priceX, tradeY + 3.0f * ui,
                             0.43f * ui, goldR, goldG, goldB,
                             0.96f * detailA, 0.62f);
            const bool canBuy = cost >= 0 && g_Coins >= cost && curLv < md.maxLv;
            const bool actionHover = inputReady && mx >= actionX
                                  && mx <= actionX + actionW
                                  && my >= tradeY - 52.0f * ui
                                  && my <= tradeY + 52.0f * ui;
            if (actionHover && lmbClick && canBuy) {
                g_Coins -= cost; ++g_MetaLv[detailId]; SaveGame();
            }
            wchar_t actionBuf[64];
            if (curLv >= md.maxLv) swprintf_s(actionBuf, L"MAX LEVEL");
            else swprintf_s(actionBuf, L"UPGRADE MODULE  %lld", cost);
            drawFixedAction(actionX, tradeY, actionW, actionBuf,
                            detailR, detailG, detailB,
                            canBuy || curLv >= md.maxLv, actionHover);
        } else if (detailIsTheme && detailId >= 0 && detailId < ACCENT_COUNT) {
            const AccentTheme& theme = ACCENT_THEMES[detailId];
            const bool owned = ThemeOwned(detailId);
            const bool equipped = owned && g_ThemeSel == detailId;
            const long long cost = theme.cost;
            DrawShadowedText(g_TextS, equipped ? L"EQUIPPED"
                              : (owned ? L"OWNED" : L"LOCKED"),
                             priceX, tradeY - 31.0f * ui, 0.40f * ui,
                             whiteR, whiteG, whiteB, 0.80f * detailA, 0.60f);
            wchar_t priceBuf[64];
            if (owned) swprintf_s(priceBuf, L"PROFILE OWNED");
            else swprintf_s(priceBuf, L"%lld  STARDUST", cost);
            DrawShadowedText(g_TextS, priceBuf, priceX, tradeY + 3.0f * ui,
                             0.43f * ui, goldR, goldG, goldB,
                             0.96f * detailA, 0.62f);
            const bool canUse = !equipped && (owned || g_Coins >= cost);
            const bool actionHover = inputReady && mx >= actionX
                                  && mx <= actionX + actionW
                                  && my >= tradeY - 52.0f * ui
                                  && my <= tradeY + 52.0f * ui;
            if (actionHover && lmbClick && canUse) {
                if (!owned) { g_Coins -= cost; g_ThemeOwned |= (1 << detailId); }
                g_ThemeSel = detailId; ApplyAccentTheme(); SaveGame();
            }
            drawFixedAction(actionX, tradeY,
                            actionW, equipped ? L"EQUIPPED"
                                : (owned ? L"EQUIP" : L"UNLOCK"),
                            detailR, detailG, detailB,
                            canUse || equipped, actionHover);
        } else if (detailIsAug) {
            DrawShadowedText(g_TextS, L"PASSIVE EFFECT / FIELD PAYLOAD",
                             priceX, tradeY - 31.0f * ui, 0.40f * ui,
                             whiteR, whiteG, whiteB, 0.80f * detailA, 0.60f);
            drawFixedAction(actionX, tradeY, actionW, L"PASSIVE",
                            detailR, detailG, detailB, false, false);
        }

        wchar_t dustBuf[64]; swprintf_s(dustBuf, L"STARDUST  %06lld", g_Coins);
        const float dustW = g_TextS.Width(dustBuf, 0.52f * ui);
        DrawVisibleConstellNode(sw - std::max(74.0f, sw * 0.075f) - dustW - 22.0f * ui,
                                std::max(72.0f, sh * 0.105f), 5.0f * ui,
                                goldR, goldG, goldB, 0.94f * designA, false);
        DrawShadowedText(g_TextS, dustBuf,
                         sw - std::max(74.0f, sw * 0.075f) - dustW,
                         std::max(72.0f, sh * 0.105f) - 7.0f * ui,
                         0.52f * ui, goldR, goldG, goldB,
                         0.98f * designA, 0.72f);
        DrawShadowedText(g_TextS, L"ESC / RMB  RETURN",
                         sw - std::max(74.0f, sw * 0.075f) -
                         g_TextS.Width(L"ESC / RMB  RETURN", 0.31f * ui),
                         sh - 48.0f * ui, 0.31f * ui,
                         0.52f, 0.68f, 0.82f, 0.60f * designA, 0.58f);

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

    // SHOP_MARKET_RENDERER
    // Products live in the upper-left constellation field. The lower-right
    // copy and transaction line describe the constellation currently selected.
    {
        const float designA = std::max(0.0f, std::min(1.0f, wake));
        const float detailA = std::max(0.0f, std::min(1.0f, rightWake * s_detailT));
        const float ui = std::max(0.62f, std::min(sw * 0.90f / 1800.0f,
                                                  sh * 0.86f / 1020.0f));
        const float cyanR = 0.28f, cyanG = 0.86f, cyanB = 1.00f;
        const float goldR = 1.00f, goldG = 0.77f, goldB = 0.24f;
        const float whiteR = 0.88f, whiteG = 0.94f, whiteB = 1.00f;
        const float topY = std::max(72.0f, sh * 0.105f);
        const float marketCX = sw * 0.35f;
        const float carouselY = sh * 0.36f;
        const float detailCX = sw * 0.73f;
        const float detailY = sh * 0.675f;
        const float lmbClick = lmb && !g_LmbPrev;

        auto drawFit = [&](const wchar_t* text, float x, float y, float scale,
                           float maxW, float r, float g2, float b, float a) {
            if (!text || !text[0] || maxW <= 2.0f) return;
            float sc = scale;
            const float tw = g_TextS.Width(text, sc);
            if (tw > maxW && tw > 0.0f) sc *= maxW / tw;
            DrawShadowedText(g_TextS, text, x, y, sc, r, g2, b, a * designA, 0.72f);
        };
        auto drawFitCentered = [&](const wchar_t* text, float centerX, float y,
                                   float scale, float maxW,
                                   float r, float g2, float b, float a) {
            if (!text || !text[0] || maxW <= 2.0f) return;
            float sc = scale;
            const float originalW = g_TextS.Width(text, sc);
            if (originalW > maxW && originalW > 0.0f) sc *= maxW / originalW;
            const float textW = g_TextS.Width(text, sc);
            DrawShadowedText(g_TextS, text, centerX - textW * 0.5f, y, sc,
                             r, g2, b, a * designA, 0.72f);
        };
        auto drawArc = [&](float cx, float cy, float rx, float ry,
                           float a0, float a1, float r, float g2, float b,
                           float thick, float a) {
            const int segments = 30;
            float px = cx + cosf(a0) * rx;
            float py = cy + sinf(a0) * ry;
            for (int i = 1; i <= segments; ++i) {
                const float t = (float)i / (float)segments;
                const float ang = a0 + (a1 - a0) * t;
                const float nx = cx + cosf(ang) * rx;
                const float ny = cy + sinf(ang) * ry;
                DrawVisibleConstellLine(px, py, nx, ny, thick, r, g2, b,
                                        a * designA);
                px = nx; py = ny;
            }
            DrawVisibleConstellNode(px, py, 3.0f * ui, r, g2, b,
                                    0.70f * a * designA, false);
        };
        auto drawModuleGlyph = [&](int key, float cx, float cy,
                                   float size, float r, float g2, float b, float a) {
            if (key >= KEY_AUG && key - KEY_AUG >= 0 && key - KEY_AUG < AUG_TOTAL) {
                DrawAugIcon(ALL_AUGS[key - KEY_AUG].type,
                            cx - size * 0.5f, cy - size * 0.5f,
                            size, r, g2, b, a * designA);
                return;
            }
            BindMainShader();
            const float half = size * 0.5f;
            const float line = std::max(1.0f * ui, size * 0.045f);
            drawRect(cx - half, cy - half, size, line, r, g2, b, a * designA);
            drawRect(cx - half, cy + half - line, size, line, r, g2, b, a * designA);
            drawRect(cx - half, cy - half, line, size, r, g2, b, a * designA);
            drawRect(cx + half - line, cy - half, line, size, r, g2, b, a * designA);
            LogoLine(cx - size * 0.23f, cy, cx + size * 0.23f, cy,
                     line, r, g2, b, a * designA);
            LogoLine(cx, cy - size * 0.23f, cx, cy + size * 0.23f,
                     line, r, g2, b, a * designA);
            drawDiamond(cx, cy, size * 0.10f, r, g2, b, a * designA);
        };
        auto drawTradeAction = [&](float x, float y, float w, const wchar_t* label,
                                   float r, float g2, float b,
                                   bool enabled, bool hovered) {
            const float aa = enabled ? (hovered ? 1.0f : 0.72f) : 0.28f;
            drawFit(label, x, y - 20.0f * ui, 0.54f * ui, w,
                    r, g2, b, enabled ? 0.96f : 0.44f);
            DrawVisibleConstellLine(x, y + 14.0f * ui,
                                    x + w * 0.68f, y + 14.0f * ui,
                                    1.8f * ui, r, g2, b, aa * designA);
            DrawVisibleConstellNode(x + w * 0.68f, y + 14.0f * ui,
                                    4.4f * ui, r, g2, b, aa * designA, false);
        };

        DrawSceneLeftVignette(sw, sh, 0.045f * designA);
        DrawSceneRadialVignette(detailCX, detailY,
                                std::max(320.0f * ui, sw * 0.28f),
                                0.13f * detailA);

        // The lower-right CircleTexture is a soft black contrast field. It is
        // deliberately clipped by the screen edge so it reads as part of the
        // background scene rather than as another rectangular UI surface.
        if (g_ConstellationCircleTex && designA > 0.001f) {
            const float circleR = std::min(sw, sh) * 0.38f;
            const float circleCX = sw * 0.90f;
            const float circleCY = sh * 0.91f;
            BatchFlush();
            DrawIcon(g_ConstellationCircleTex,
                     circleCX - circleR, circleCY - circleR,
                     circleR * 2.0f, circleR * 2.0f,
                     0.0f, 0.0f, 0.012f, 0.78f * designA);
        }

        // A small route marker replaces the old page title. It gives the
        // screen a clear way back without creating another vertical menu.
        const float backX = sw * 0.075f;
        const bool backHover = inputReady && mx >= backX - 18.0f * ui
                            && mx <= backX + 118.0f * ui
                            && my >= topY - 18.0f * ui
                            && my <= topY + 28.0f * ui;
        s_backHov = UpdateMenuCommandHover(s_backHov, backHover, dt);
        if (backHover && lmbClick) beginBack();
        DrawVisibleConstellNode(backX, topY, 4.5f * ui,
                                0.46f, 0.78f, 0.96f,
                                (0.72f + 0.22f * s_backHov) * designA, false);
        DrawVisibleConstellLine(backX + 10.0f * ui, topY,
                                backX + 30.0f * ui, topY,
                                1.1f * ui, 0.46f, 0.78f, 0.96f,
                                (0.48f + 0.32f * s_backHov) * designA);
        DrawShadowedText(g_TextS, L"BACK", backX + 39.0f * ui,
                         topY - 7.0f * ui, 0.42f * ui,
                         0.58f, 0.78f, 0.92f,
                         (0.66f + 0.24f * s_backHov) * designA, 0.70f);

        // Categories are now a single horizontal constellation instead of a
        // heavy left rail. Their labels remain the navigation affordance.
        static const wchar_t* kMarketRoute[SHOP_TAB_COUNT] = {
            L"START", L"UNLOCK", L"RIFLE", L"FIELD"
        };
        static const wchar_t* kMarketSub[SHOP_TAB_COUNT] = {
            L"MODULES", L"CORE", L"WEAPON", L"ARRAY"
        };
        static const float kMarketR[SHOP_TAB_COUNT] = { 0.38f, 0.30f, 0.72f, 0.60f };
        static const float kMarketG[SHOP_TAB_COUNT] = { 0.84f, 0.82f, 0.92f, 0.52f };
        static const float kMarketB[SHOP_TAB_COUNT] = { 1.00f, 1.00f, 0.42f, 1.00f };
        const float catStartX = sw * 0.29f;
        const float catGap = std::min(160.0f * ui, sw * 0.115f);
        for (int i = 0; i < SHOP_TAB_COUNT; ++i) {
            const float x = catStartX + (float)i * catGap;
            const float labelW = g_TextS.Width(kMarketRoute[i], 0.47f * ui);
            const bool hov = inputReady && mx >= x - 14.0f * ui
                          && mx <= x + labelW + 18.0f * ui
                          && my >= topY - 20.0f * ui
                          && my <= topY + 30.0f * ui;
            s_tabHov[i] = UpdateMenuCommandHover(s_tabHov[i], hov, dt);
            if (hov && lmbClick) {
                s_tab = i;
                s_prevTab = -1;
                s_browseItems = true;
                s_browseInputLock = true;
            }
            const bool selected = s_tab == i;
            const float a = (selected ? 0.98f : 0.48f + 0.36f * s_tabHov[i]) * designA;
            DrawVisibleConstellNode(x, topY, (selected ? 5.2f : 3.2f) * ui,
                                    kMarketR[i], kMarketG[i], kMarketB[i], a, false);
            DrawShadowedText(g_TextS, kMarketRoute[i], x + 13.0f * ui,
                             topY - 14.0f * ui, 0.47f * ui,
                             selected ? whiteR : kMarketR[i],
                             selected ? whiteG : kMarketG[i],
                             selected ? whiteB : kMarketB[i], a, 0.72f);
            DrawShadowedText(g_TextS, kMarketSub[i], x + 14.0f * ui,
                             topY + 10.0f * ui, 0.29f * ui,
                             kMarketR[i], kMarketG[i], kMarketB[i],
                             (selected ? 0.78f : 0.38f + 0.24f * s_tabHov[i]) * designA,
                             0.58f);
            DrawVisibleConstellLine(x, topY + 27.0f * ui,
                                    x + (selected ? labelW + 18.0f * ui : labelW * 0.52f),
                                    topY + 27.0f * ui,
                                    (selected ? 1.8f : 0.7f) * ui,
                                    kMarketR[i], kMarketG[i], kMarketB[i],
                                    (selected ? 0.78f : 0.18f) * designA);
        }

        // Resource readout is intentionally a floating purchase signal, not a
        // page heading.
        wchar_t dustBuf[80];
        swprintf_s(dustBuf, L"STARDUST  %06lld", g_Coins);
        const float dustSc = 0.52f * ui;
        const float dustW = g_TextS.Width(dustBuf, dustSc);
        const float dustX = sw - std::max(74.0f, sw * 0.075f) - dustW;
        drawArc(dustX - 20.0f * ui, topY, 20.0f * ui, 8.0f * ui,
                3.36f, 5.90f, goldR, goldG, goldB, 1.5f * ui, 0.75f);
        DrawVisibleConstellNode(dustX - 20.0f * ui, topY, 5.0f * ui,
                                goldR, goldG, goldB, 0.94f * designA, false);
        DrawShadowedText(g_TextS, dustBuf, dustX, topY - 7.0f * ui,
                         dustSc, goldR, goldG, goldB, 0.98f * designA, 0.72f);

        int itemIndices[300];
        int itemTotal = 0;
        int selectedPos = 0;
        for (int i = 0; i < s_itemCount; ++i) {
            if (s_items[i].isGroup) continue;
            if (itemTotal < 300) {
                itemIndices[itemTotal] = i;
                if (s_items[i].key == s_selKey) selectedPos = itemTotal;
                ++itemTotal;
            }
        }

        float selectedNodeX = marketCX;
        float selectedNodeY = carouselY;
        if (itemTotal > 0) {
            const float chartR = std::min(sw * 0.25f, sh * 0.255f);
            const float rowStep = std::min(78.0f * ui, sh * 0.070f);
            const bool overChart = inputReady && mx >= marketCX - chartR * 1.30f
                                && mx <= marketCX + chartR * 0.95f
                                && my >= carouselY - rowStep * 4.8f
                                && my <= carouselY + rowStep * 4.8f;
            if (overChart && g_ScrollAccum != 0.0f) {
                const int direction = g_ScrollAccum < 0.0f ? 1 : -1;
                selectedPos = (selectedPos + direction + itemTotal) % itemTotal;
                s_selKey = s_items[itemIndices[selectedPos]].key;
                s_detailT = 0.0f;
            }
            g_ScrollAccum = 0.0f;

            int currentSelectedPos = 0;
            for (int i = 0; i < itemTotal; ++i)
                if (s_items[itemIndices[i]].key == s_selKey) currentSelectedPos = i;

            // The shop list is a curved upper-left star map. The line of
            // products bends toward the centre instead of becoming a row of
            // conventional cards.
            drawArc(marketCX, carouselY, chartR * 0.92f, chartR * 0.66f,
                    3.36f, 5.78f, cyanR, cyanG, cyanB, 0.85f * ui, 0.30f);
            drawArc(marketCX, carouselY, chartR * 0.72f, chartR * 0.48f,
                    3.72f, 5.38f, cyanR, cyanG, cyanB, 0.62f * ui, 0.17f);

            float previousX = 0.0f, previousY = 0.0f;
            bool hasPrevious = false;
            for (int slot = 0; slot < itemTotal; ++slot) {
                float relF = (float)slot - (float)currentSelectedPos;
                while (relF > (float)itemTotal * 0.5f) relF -= (float)itemTotal;
                while (relF < -(float)itemTotal * 0.5f) relF += (float)itemTotal;
                if (std::fabs(relF) > 4.0f) continue;

                const int itemIndex = itemIndices[slot];
                const SLItem& item = s_items[itemIndex];
                const float dy = relF * rowStep;
                const float span = std::max(42.0f * ui,
                                            chartR * chartR - dy * dy);
                const float nodeX = marketCX - sqrtf(span) * 0.78f;
                const float nodeY = carouselY + dy;
                const bool selected = item.key == s_selKey;
                const float hoverPreview = s_itemHov[itemIndex];
                const float miniR = (selected ? 36.0f : 21.0f)
                                  * (1.0f - 0.15f * hoverPreview) * ui;
                const float labelX = nodeX + miniR + 26.0f * ui;
                const float labelW = std::max(180.0f * ui,
                                              std::min(300.0f * ui,
                                                       sw * 0.18f));
                const bool hov = inputReady && mx >= nodeX - 38.0f * ui
                              && mx <= labelX + labelW
                              && my >= nodeY - 42.0f * ui
                              && my <= nodeY + 42.0f * ui;
                s_itemHov[itemIndex] = UpdateMenuCommandHover(s_itemHov[itemIndex], hov, dt);
                if (hov && lmbClick) {
                    s_selKey = item.key;
                    s_detailT = 0.0f;
                }
                const float active = selected ? 1.0f : s_itemHov[itemIndex];
                if (hasPrevious) {
                    DrawVisibleConstellLine(previousX, previousY, nodeX, nodeY,
                                            0.62f * ui, item.r, item.g, item.b,
                                            (selected ? 0.36f : 0.15f) * designA);
                }
                const int constellationKind = item.key >= KEY_AUG ? 2 : 1;
                DrawConstellationDisc(nodeX, nodeY, miniR * 1.24f,
                                      0.0f, 0.0f, 0.012f,
                                      (selected ? 0.26f : 0.12f) * designA);
                DrawArchiveConstellation(nodeX, nodeY, miniR,
                                         constellationKind, item.key,
                                         now * 0.18f, item.r, item.g, item.b,
                                         (selected ? 0.84f : 0.38f + 0.28f * active) * designA,
                                         ui);
                DrawVisibleConstellNode(nodeX, nodeY,
                                        (selected ? 4.4f : 2.4f + 0.45f * active) * ui,
                                        item.r, item.g, item.b,
                                        (selected ? 0.96f : 0.44f + 0.24f * active) * designA,
                                        false);
                drawModuleGlyph(item.key, nodeX, nodeY,
                                (selected ? 30.0f : 18.0f) * ui,
                                item.r, item.g, item.b,
                                selected ? 0.88f : 0.34f + 0.28f * active);
                const float lineEnd = labelX - 12.0f * ui;
                DrawVisibleConstellLine(nodeX + miniR * 0.70f, nodeY,
                                        lineEnd, nodeY, (selected ? 1.5f : 0.72f) * ui,
                                        item.r, item.g, item.b,
                                        (selected ? 0.62f : 0.18f + 0.18f * active) * designA);
                DrawVisibleConstellNode(lineEnd, nodeY,
                                        (selected ? 4.0f : 2.5f) * ui,
                                        item.r, item.g, item.b,
                                        (selected ? 0.88f : 0.34f + 0.20f * active) * designA,
                                        false);
                drawFit(item.label, labelX, nodeY - 13.0f * ui,
                        selected ? 0.56f * ui : 0.42f * ui, labelW,
                        selected ? whiteR : item.r,
                        selected ? whiteG : item.g,
                        selected ? whiteB : item.b,
                        selected ? 0.98f : 0.48f + 0.26f * active);
                const wchar_t* kind = item.key < KEY_THEME ? L"CORE MODULE"
                                     : (item.key < KEY_AUG ? L"SIGNAL PROFILE" : L"FIELD PAYLOAD");
                DrawShadowedText(g_TextS, kind, labelX, nodeY + 12.0f * ui,
                                 0.29f * ui, item.r, item.g, item.b,
                                 (selected ? 0.72f : 0.32f + 0.20f * active) * designA,
                                 0.58f);
                if (selected) {
                    selectedNodeX = nodeX;
                    selectedNodeY = nodeY;
                }
                previousX = nodeX;
                previousY = nodeY;
                hasPrevious = true;
            }
        }

        float detailR = cyanR, detailG = cyanG, detailB = cyanB;
        const wchar_t* detailTitle = L"NO MODULE";
        const wchar_t* detailType = L"EMPTY CATALOGUE";
        bool detailIsMeta = false, detailIsTheme = false, detailIsAug = false;
        int detailId = -1;
        if (s_selKey >= 0) {
            if (s_selKey < KEY_THEME) {
                detailIsMeta = true; detailId = s_selKey - KEY_META;
                if (detailId >= 0 && detailId < META_COUNT) {
                    detailTitle = MetaName(detailId);
                    detailType = L"CORE MODULE";
                }
            } else if (s_selKey < KEY_AUG) {
                detailIsTheme = true; detailId = s_selKey - KEY_THEME;
                if (detailId >= 0 && detailId < ACCENT_COUNT) {
                    detailTitle = AccentName(detailId);
                    detailType = L"SIGNAL PROFILE";
                    detailR = ACCENT_THEMES[detailId].r;
                    detailG = ACCENT_THEMES[detailId].g;
                    detailB = ACCENT_THEMES[detailId].b;
                }
            } else {
                detailIsAug = true; detailId = s_selKey - KEY_AUG;
                if (detailId >= 0 && detailId < AUG_TOTAL) {
                    detailTitle = ALL_AUGS[detailId].locName[li];
                    detailType = L"FIELD PAYLOAD";
                    GetRarityColor(ALL_AUGS[detailId].rarity,
                                   detailR, detailG, detailB);
                }
            }
        }

        // The selected constellation sends one quiet route line into the
        // lower-right title/detail field.
        DrawVisibleConstellLine(selectedNodeX + 22.0f * ui, selectedNodeY,
                                detailCX - 250.0f * ui, detailY - 20.0f * ui,
                                0.9f * ui, detailR, detailG, detailB,
                                0.34f * detailA);
        DrawVisibleConstellNode(detailCX - 250.0f * ui, detailY - 20.0f * ui,
                                3.8f * ui,
                                detailR, detailG, detailB, 0.70f * detailA, false);
        const float titleW = std::min(520.0f * ui, sw * 0.40f);
        const float titleSc = 0.86f * ui;
        drawFitCentered(detailType, detailCX, detailY, 0.38f * ui,
                        titleW, detailR, detailG, detailB, 0.84f * detailA);
        drawFitCentered(detailTitle, detailCX, detailY + 28.0f * ui,
                        titleSc, titleW, whiteR, whiteG, whiteB, 0.98f);

        const float tradeY = detailY + 93.0f * ui;
        const float tradeL = detailCX - std::min(410.0f * ui, sw * 0.25f);
        const float tradeR = detailCX + std::min(410.0f * ui, sw * 0.25f);
        DrawVisibleConstellLine(tradeL, tradeY, tradeR, tradeY,
                                0.72f * ui, detailR, detailG, detailB,
                                0.26f * detailA);

        const float infoX = tradeL;
        const float actionX = marketCX + 42.0f * ui;
        const float actionW = std::min(300.0f * ui, tradeR - actionX);
        if (detailIsMeta && detailId >= 0 && detailId < META_COUNT) {
            const MetaDef& md = META_DEFS[detailId];
            const int curLv = g_MetaLv[detailId];
            const long long cost = MetaNextCost(detailId);
            wchar_t levelBuf[48];
            swprintf_s(levelBuf, L"LEVEL  %d / %d", curLv, md.maxLv);
            DrawShadowedText(g_TextS, levelBuf, infoX, tradeY - 30.0f * ui,
                             0.42f * ui, whiteR, whiteG, whiteB,
                             0.82f * detailA, 0.62f);
            wchar_t costBuf[64];
            if (cost < 0) swprintf_s(costBuf, L"MAX LEVEL");
            else swprintf_s(costBuf, L"%lld  STARDUST", cost);
            DrawShadowedText(g_TextS, costBuf, infoX, tradeY + 22.0f * ui,
                             0.43f * ui, goldR, goldG, goldB,
                             0.92f * detailA, 0.62f);
            const bool canBuy = cost >= 0 && g_Coins >= cost && curLv < md.maxLv;
            const bool actionHover = inputReady && mx >= actionX
                                  && mx <= actionX + actionW
                                  && my >= tradeY - 52.0f * ui
                                  && my <= tradeY + 52.0f * ui;
            if (actionHover && lmbClick && canBuy) {
                g_Coins -= cost;
                ++g_MetaLv[detailId];
                SaveGame();
            }
            wchar_t actionBuf[64];
            if (curLv >= md.maxLv) swprintf_s(actionBuf, L"MAX LEVEL");
            else swprintf_s(actionBuf, L"UPGRADE MODULE  %lld", cost);
            drawTradeAction(actionX, tradeY, actionW, actionBuf,
                            detailR, detailG, detailB,
                            canBuy || curLv >= md.maxLv, actionHover);
        } else if (detailIsTheme && detailId >= 0 && detailId < ACCENT_COUNT) {
            const AccentTheme& theme = ACCENT_THEMES[detailId];
            const bool owned = ThemeOwned(detailId);
            const bool equipped = owned && g_ThemeSel == detailId;
            const long long cost = theme.cost;
            const wchar_t* status = equipped ? L"EQUIPPED"
                                : (owned ? L"OWNED" : L"LOCKED");
            DrawShadowedText(g_TextS, status, infoX, tradeY - 30.0f * ui,
                             0.42f * ui, whiteR, whiteG, whiteB,
                             0.82f * detailA, 0.62f);
            wchar_t costBuf[64];
            if (owned) swprintf_s(costBuf, L"PROFILE OWNED");
            else swprintf_s(costBuf, L"%lld  STARDUST", cost);
            DrawShadowedText(g_TextS, costBuf, infoX, tradeY + 22.0f * ui,
                             0.43f * ui, goldR, goldG, goldB,
                             0.92f * detailA, 0.62f);
            const bool actionEnabled = !equipped && (owned || g_Coins >= cost);
            const bool actionHover = inputReady && mx >= actionX
                                  && mx <= actionX + actionW
                                  && my >= tradeY - 52.0f * ui
                                  && my <= tradeY + 52.0f * ui;
            if (actionHover && lmbClick && actionEnabled) {
                if (!owned) {
                    g_Coins -= cost;
                    g_ThemeOwned |= (1 << detailId);
                }
                g_ThemeSel = detailId;
                ApplyAccentTheme();
                SaveGame();
            }
            const wchar_t* actionLabel = equipped ? L"EQUIPPED"
                                    : (owned ? L"EQUIP" : L"UNLOCK");
            drawTradeAction(actionX, tradeY, actionW, actionLabel,
                            detailR, detailG, detailB,
                            actionEnabled || equipped, actionHover);
        } else if (detailIsAug && detailId >= 0 && detailId < AUG_TOTAL) {
            const AugDef& ad = ALL_AUGS[detailId];
            const wchar_t* desc = AugDesc(ad);
            DrawShadowedText(g_TextS, L"CATALOGUE / FIELD READY", infoX,
                             tradeY - 30.0f * ui, 0.42f * ui,
                             whiteR, whiteG, whiteB, 0.82f * detailA, 0.62f);
            if (desc && desc[0]) {
                std::wstring text(desc);
                size_t pos = 0;
                int line = 0;
                while (pos < text.size() && line < 2) {
                    size_t slash = text.find(L" / ", pos);
                    std::wstring segment = slash == std::wstring::npos
                        ? text.substr(pos) : text.substr(pos, slash - pos);
                    pos = slash == std::wstring::npos ? text.size() : slash + 3;
                    while (!segment.empty() && segment.front() == L' ') segment.erase(0, 1);
                    while (!segment.empty() && segment.back() == L' ') segment.pop_back();
                    drawFit(segment.c_str(), infoX, tradeY + (4.0f + line * 22.0f) * ui,
                            0.38f * ui, actionX - infoX - 30.0f * ui,
                            0.72f, 0.82f, 0.90f, 0.80f);
                    ++line;
                }
            }
            drawTradeAction(actionX, tradeY, actionW, L"PASSIVE",
                            detailR, detailG, detailB, false, false);
        }

        DrawShadowedText(g_TextS, L"SCROLL / CLICK TO ROTATE CATALOGUE",
                         marketCX - g_TextS.Width(L"SCROLL / CLICK TO ROTATE CATALOGUE", 0.31f * ui) * 0.5f,
                         sh - 48.0f * ui, 0.31f * ui,
                         0.52f, 0.68f, 0.82f, 0.60f * designA, 0.58f);
        DrawShadowedText(g_TextS, L"ESC / RMB  RETURN",
                         sw - std::max(74.0f, sw * 0.075f) -
                         g_TextS.Width(L"ESC / RMB  RETURN", 0.31f * ui),
                         sh - 48.0f * ui, 0.31f * ui,
                         0.52f, 0.68f, 0.82f, 0.60f * designA, 0.58f);

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

    // SHOP_DECK_RENDERER
    {
        const float designA = std::max(0.0f, std::min(1.0f, wake));
        const float detailA = std::max(0.0f, std::min(1.0f, rightWake * s_detailT));
        const float ui = std::max(0.62f, std::min(sw * 0.90f / 1800.0f,
                                                  sh * 0.86f / 1020.0f));
        // Anchor the shop canvas to the same root rail as the other pages.
        const float deckX = rootX;
        const float deckY = std::max(58.0f, rootY - 150.0f * ui);
        const float deckR = sw - std::max(36.0f, sw * 0.045f);
        const float deckB = sh - std::max(58.0f, sh * 0.075f);
        const float deckW = std::max(760.0f, deckR - deckX);
        const float deckH = std::max(540.0f, deckB - deckY);
        // The shop follows the same rule as the codex: no filled surfaces.
        // Contrast comes from hierarchy, rules, corner marks and active rails
        // so the live background remains part of the page.
        const float primaryInkA = 0.98f;
        const float railW = rootW;
        const float gap = 34.0f * ui;
        const float listX = deckX + railW + gap;
        const float listW = std::max(320.0f * ui,
                                     std::min(520.0f * ui, deckW * 0.34f));
        const float detailX = listX + listW + gap;
        const float detailW = std::max(300.0f * ui, deckR - detailX);
        const float contentTop = rootY + 16.0f * ui;
        const float contentBottom = deckB - 34.0f * ui;
        const float contentH = std::max(280.0f * ui, contentBottom - contentTop);
        const bool lmbClick = lmb && !g_LmbPrev;

        auto drawRule = [&](float x, float y, float w, float r, float g2, float b,
                            float a) {
            BindMainShader();
            drawRect(x, y, w, std::max(1.0f, 1.0f * ui), r, g2, b,
                     a * designA);
        };
        auto drawOrbitArc = [&](float cx, float cy, float rx, float ry,
                                float a0, float a1, float r, float g2, float b,
                                float thick, float a) {
            const int segments = 24;
            float px = cx + cosf(a0) * rx;
            float py = cy + sinf(a0) * ry;
            for (int i = 1; i <= segments; ++i) {
                const float t = (float)i / (float)segments;
                const float ang = a0 + (a1 - a0) * t;
                const float nx = cx + cosf(ang) * rx;
                const float ny = cy + sinf(ang) * ry;
                DrawVisibleConstellLine(px, py, nx, ny, thick, r, g2, b,
                                        a * designA);
                px = nx;
                py = ny;
            }
            DrawVisibleConstellNode(px, py, 2.8f * ui, r, g2, b, 0.54f * a * designA);
        };
        auto drawFit = [&](const wchar_t* text, float x, float y, float scale,
                           float maxW, float r, float g2, float b, float a) {
            if (!text || !text[0] || maxW <= 2.0f) return;
            float sc = scale;
            const float tw = g_TextS.Width(text, sc);
            if (tw > maxW && tw > 0.0f) sc *= maxW / tw;
            DrawShadowedText(g_TextS, text, x, y, sc, r, g2, b, a * designA, 0.64f);
        };
        auto drawModuleGlyph = [&](int key, float cx, float cy,
                                   float r, float g2, float b, float a) {
            if (key >= KEY_AUG && key - KEY_AUG >= 0 && key - KEY_AUG < AUG_TOTAL) {
                DrawAugIcon(ALL_AUGS[key - KEY_AUG].type, cx - 15.0f * ui,
                            cy - 15.0f * ui, 30.0f * ui, r, g2, b, a * designA);
                return;
            }
            BindMainShader();
            const float s = 15.0f * ui;
            drawRect(cx - s, cy - s, s * 2.0f, 1.0f * ui, r, g2, b, a * designA);
            drawRect(cx - s, cy + s - 1.0f * ui, s * 2.0f, 1.0f * ui,
                     r, g2, b, a * designA);
            drawRect(cx - s, cy - s, 1.0f * ui, s * 2.0f,
                     r, g2, b, a * designA);
            drawRect(cx + s - 1.0f * ui, cy - s, 1.0f * ui, s * 2.0f,
                     r, g2, b, a * designA);
            LogoLine(cx - 7.0f * ui, cy, cx + 7.0f * ui, cy,
                     1.5f * ui, r, g2, b, a * designA);
            LogoLine(cx, cy - 7.0f * ui, cx, cy + 7.0f * ui,
                     1.5f * ui, r, g2, b, a * designA);
            drawDiamond(cx, cy, 3.0f * ui, r, g2, b, a * designA);
        };
        auto drawAction = [&](float x, float y, float w, const wchar_t* label,
                              float r, float g2, float b, bool enabled, bool hov) {
            const float aa = enabled ? (hov ? 0.95f : 0.68f) : 0.25f;
            const float pulse = hov && enabled ? (0.12f + 0.05f * sinf(now * 5.0f)) : 0.0f;
            const float lineY = y + 37.0f * ui;
            const float lineW = w * (0.64f + pulse);
            LogoLine(x + 8.0f * ui, lineY, x + lineW, lineY,
                     1.4f * ui, r, g2, b, aa * designA);
            DrawVisibleConstellNode(x + lineW, lineY, 4.0f * ui,
                                    r, g2, b, aa * designA, false);
            drawFit(label, x + 8.0f * ui, y + 4.0f * ui, 0.52f * ui,
                    w - 16.0f * ui, r, g2, b, enabled ? 0.92f : 0.42f);
        };

        DrawSceneLeftVignette(sw, sh, 0.06f * designA);
        DrawSceneRadialVignette(detailX + detailW * 0.48f,
                                deckY + deckH * 0.54f,
                                std::max(220.0f * ui, detailW * 0.48f),
                                0.06f * detailA);

        const float cyanR = 0.32f, cyanG = 0.86f, cyanB = 1.00f;
        const float warmR = 1.00f, warmG = 0.78f, warmB = 0.28f;
        const float paleR = 0.78f, paleG = 0.88f, paleB = 0.96f;

        wchar_t dustBuf[80];
        swprintf_s(dustBuf, L"STARDUST  %06lld", g_Coins);
        const float dustSc = 0.56f * ui;
        const float dustW = g_TextS.Width(dustBuf, dustSc);
        const float dustCX = deckR - dustW - 28.0f * ui;
        const float dustCY = deckY + 29.0f * ui;
        drawOrbitArc(dustCX, dustCY, 19.0f * ui, 8.0f * ui,
                     3.35f, 5.85f, warmR, warmG, warmB, 1.7f * ui, 0.72f);
        drawOrbitArc(dustCX, dustCY, 12.0f * ui, 5.0f * ui,
                     0.15f, 2.10f, warmR, warmG, warmB, 1.0f * ui, 0.46f);
        DrawVisibleConstellNode(dustCX, dustCY, 5.0f * ui,
                                warmR, warmG, warmB, 0.95f * designA, false);
        DrawShadowedText(g_TextS, dustBuf, deckR - dustW, deckY + 22.0f * ui,
                         dustSc, warmR, warmG, warmB, 0.96f * designA, 0.72f);
        DrawVisibleConstellNode(deckR - 3.0f * ui, deckY + 23.0f * ui,
                                2.4f * ui, warmR, warmG, warmB,
                                0.58f * designA, false);

        static const wchar_t* kTabLbl[SHOP_TAB_COUNT][2] = {
            { L"\xC2DC\xC791 \xBAA8\xB4C8", L"START" },
            { L"\xD574\xAE08", L"UNLOCK" },
            { L"\xC18C\xCD1D", L"RIFLE"  },
            { L"\xC704\xC131", L"FIELD"  },
        };
        static const float kTR[SHOP_TAB_COUNT] = { 0.70f, 0.38f, 0.72f, 0.62f };
        static const float kTG[SHOP_TAB_COUNT] = { 0.70f, 0.82f, 0.92f, 0.48f };
        static const float kTB[SHOP_TAB_COUNT] = { 0.70f, 1.00f, 0.42f, 1.00f };
        const float railX = rootX;
        const float railY = rootY;
        // The left rail uses the same command renderer as the codex, armory
        // and run-config pages. It is intentionally not a shop-specific card.
        const float routeH = rootH;
        const float routeGap = rootGap;
        const float categoryWake = wake * categoryA;
        const int ROOT_COUNT = SHOP_TAB_COUNT + 1;
        for (int i = 0; i < ROOT_COUNT; ++i) {
            const bool isBack = (i == SHOP_TAB_COUNT);
            const float tx = railX - 58.0f * itemA;
            const float ty = railY + (float)i * (routeH + routeGap);
            const float hitX = tx - 26.0f;
            const float hitRight = tx + railW + 18.0f * ui;
            const bool hov = inputReady
                           && (mx >= hitX && mx < hitRight
                           && my >= ty && my < ty + routeH);
            float& hovT = isBack ? s_backHov : s_tabHov[i];
            hovT = UpdateMenuCommandHover(hovT, hov, dt);

            if (hov && lmbClick) {
                if (isBack) {
                    beginBack();
                } else {
                    s_tab = i;
                    s_prevTab = -1;
                    s_browseItems = true;
                    s_browseInputLock = true;
                }
            }

            const bool sel = (!isBack && s_tab == i);
            const float rr = isBack ? 0.42f : kTR[i];
            const float gg = isBack ? 0.62f : kTG[i];
            const float bb = isBack ? 0.78f : kTB[i];
            const wchar_t* route = isBack ? L"BACK" : kTabLbl[i][1];
            const wchar_t* sub = isBack ? L"BACK" : kTabLbl[i][0];
            const float reveal = s_backExit ? wake : MenuCommandReveal(g_ShopEntryT, i);
            const float rowA = reveal * categoryWake;
            const float selectPulse = sel ? (0.16f + 0.16f * sinf(now * 4.0f)) : 0.0f;
            const float slide = (1.0f - reveal) * 34.0f;
            const float bx = tx + slide - 10.0f * hovT;
            const float by = ty;

            DrawUnifiedMenuCommand(route, sub, bx, by, railW, routeH,
                                   rr, gg, bb, rowA, hovT, sel, selectPulse,
                                   now + (float)i * 0.17f);
        }

        // Open information fields: drifting arcs define zones without turning
        // either column into a panel.
        drawOrbitArc(listX + listW * 0.48f, contentTop + contentH * 0.54f,
                     listW * 0.58f, contentH * 0.58f,
                     3.72f, 5.22f, cyanR, cyanG, cyanB, 0.78f * ui, 0.28f);
        drawOrbitArc(detailX + detailW * 0.42f, contentTop + contentH * 0.46f,
                     detailW * 0.64f, contentH * 0.54f,
                     5.55f, 7.05f, cyanR, cyanG, cyanB, 0.72f * ui, 0.22f);

        int visibleCount = 0;
        for (int i = 0; i < s_itemCount; ++i)
            if (!s_items[i].isGroup) ++visibleCount;
        wchar_t countBuf[48]; swprintf_s(countBuf, L"%02d AVAILABLE", visibleCount);
        const float countSc = 0.44f * ui;
        DrawVisibleConstellNode(listX + 4.0f * ui, contentTop - 11.0f * ui,
                                3.8f * ui, cyanR, cyanG, cyanB,
                                0.74f * designA, false);
        DrawShadowedText(g_TextS, countBuf, listX + 17.0f * ui,
                         contentTop - 18.0f * ui, countSc,
                         cyanR, cyanG, cyanB, 0.86f * designA, 0.76f);
        DrawVisibleConstellNode(listX + listW - 5.0f * ui, contentTop - 11.0f * ui,
                                2.5f * ui, cyanR, cyanG, cyanB,
                                0.42f * designA, false);

        const float rowH = 74.0f * ui;
        const float groupH = 42.0f * ui;
        float listTotalH = 0.0f;
        for (int i = 0; i < s_itemCount; ++i)
            listTotalH += s_items[i].isGroup ? groupH : rowH;
        const float maxScroll = std::max(0.0f, listTotalH - contentH + 14.0f * ui);
        const bool overList = inputReady && mx >= listX - 20.0f * ui
                           && mx < listX + listW + 12.0f * ui
                           && my >= contentTop && my < contentTop + contentH;
        if (overList && g_ScrollAccum != 0.0f)
            s_scroll -= g_ScrollAccum * 34.0f * ui;
        g_ScrollAccum = 0.0f;
        s_scroll = std::max(0.0f, std::min(s_scroll, maxScroll));

        BatchFlush();
        glEnable(GL_SCISSOR_TEST);
        int scX = (int)(listX - 25.0f * ui);
        int scY = (int)(sh - contentTop - contentH);
        int scW = (int)(listW + 55.0f * ui);
        int scH = (int)contentH;
        if (scX < 0) scX = 0; if (scY < 0) scY = 0;
        glScissor(scX, scY, scW, scH);

        float rowY = contentTop - s_scroll;
        int displayIndex = 0;
        float prevNodeX = listX + 18.0f * ui;
        float prevNodeY = contentTop;
        bool hasPrevNode = false;
        for (int i = 0; i < s_itemCount; ++i) {
            const SLItem& item = s_items[i];
            const float h = item.isGroup ? groupH : rowH;
            const float y = rowY;
            rowY += h;
            if (y + h < contentTop || y > contentTop + contentH) {
                s_itemHov[i] = UpdateMenuCommandHover(s_itemHov[i], false, dt);
                continue;
            }
            if (item.isGroup) {
                DrawVisibleConstellNode(listX + 18.0f * ui, y + 19.0f * ui,
                                        3.6f * ui, item.r, item.g, item.b,
                                        0.62f * designA, false);
                DrawShadowedText(g_TextS, item.label, listX + 42.0f * ui,
                                 y + 8.0f * ui, 0.42f * ui,
                                 item.r, item.g, item.b, 0.80f * designA, 0.62f);
                continue;
            }
            const bool selected = s_selKey == item.key;
            const bool hovered = inputReady && s_browseItems
                              && mx >= listX - 18.0f * ui
                              && mx < listX + listW + 12.0f * ui
                              && my >= y && my < y + h;
            s_itemHov[i] = UpdateMenuCommandHover(s_itemHov[i], hovered, dt);
            if (hovered && lmbClick) s_selKey = item.key;
            const int rowNumber = displayIndex++;
            const float active = selected ? 1.0f : s_itemHov[i];
            const float rowMid = y + h * 0.5f;
            const float nodeX = listX + 18.0f * ui;
            if (hasPrevNode)
                DrawVisibleConstellLine(prevNodeX, prevNodeY, nodeX, rowMid,
                                        0.60f * ui, item.r, item.g, item.b,
                                        0.18f * designA);
            DrawVisibleConstellNode(nodeX, rowMid,
                                    (selected ? 7.0f : 3.8f) * ui,
                                    item.r, item.g, item.b,
                                    (selected ? 0.96f : 0.46f + 0.28f * active) * designA,
                                    false);
            if (selected)
                drawCircle(nodeX, rowMid, (14.0f + 2.0f * sinf(now * 3.0f)) * ui,
                           item.r, item.g, item.b, 0.08f * designA);
            drawModuleGlyph(item.key, nodeX, rowMid,
                            item.r, item.g, item.b,
                            selected ? 0.82f : 0.24f + 0.48f * active);
            prevNodeX = nodeX;
            prevNodeY = rowMid;
            hasPrevNode = true;
            wchar_t idxBuf[12]; swprintf_s(idxBuf, L"%02d", rowNumber + 1);
            DrawShadowedText(g_TextS, idxBuf, listX + 48.0f * ui,
                             y + 12.0f * ui, 0.32f * ui,
                             item.r, item.g, item.b,
                             (selected ? primaryInkA : 0.56f + 0.28f * active) * designA, 0.78f);
            drawFit(item.label, listX + 82.0f * ui, y + 10.0f * ui,
                    0.62f * ui, listW - 118.0f * ui,
                    selected ? primaryInkA : 0.86f,
                    selected ? primaryInkA : 0.90f,
                    selected ? primaryInkA : 0.96f,
                    selected ? primaryInkA : 0.84f + 0.14f * active);
            const wchar_t* itemKind = item.key < KEY_THEME ? L"META"
                                    : (item.key < KEY_AUG ? L"SIGNAL" : L"FIELD");
            DrawShadowedText(g_TextS, itemKind, listX + 82.0f * ui,
                             y + 38.0f * ui, 0.31f * ui,
                             item.r, item.g, item.b,
                             (selected ? 0.92f : 0.62f + 0.25f * active) * designA, 0.76f);
            if (selected) {
                DrawVisibleConstellLine(listX + 82.0f * ui, y + h - 4.0f * ui,
                                        listX + 82.0f * ui + (listW - 112.0f * ui) * 0.42f,
                                        y + h - 4.0f * ui, 0.72f * ui,
                                        item.r, item.g, item.b, 0.28f * designA);
            }
        }
        BatchFlush();
        glDisable(GL_SCISSOR_TEST);

        // A visible scrollbar makes the catalogue state legible without
        // relying on hidden mouse-wheel affordances.
        if (maxScroll > 0.5f) {
            const float trackX = listX + listW + 9.0f * ui;
            const float trackY = contentTop + 3.0f * ui;
            const float trackH = contentH - 6.0f * ui;
            const float thumbH = std::max(34.0f * ui,
                                          trackH * contentH / listTotalH);
            const float thumbY = trackY + (trackH - thumbH) * (s_scroll / maxScroll);
            BindMainShader();
            drawRect(trackX, trackY, 2.0f * ui, trackH,
                     0.22f, 0.34f, 0.46f, 0.44f * designA);
            drawRect(trackX - 1.0f * ui, thumbY, 4.0f * ui, thumbH,
                     cyanR, cyanG, cyanB, 0.86f * designA);
        }

        DrawVisibleConstellNode(detailX - 21.0f * ui, contentTop - 12.0f * ui,
                                3.0f * ui, cyanR, cyanG, cyanB, 0.38f * detailA);
        DrawVisibleConstellLine(detailX - 13.0f * ui, contentTop - 12.0f * ui,
                                detailX + 58.0f * ui, contentTop - 12.0f * ui,
                                0.7f * ui, cyanR, cyanG, cyanB, 0.24f * detailA);
        DrawVisibleConstellNode(detailX + 63.0f * ui, contentTop - 12.0f * ui,
                                2.4f * ui, cyanR, cyanG, cyanB, 0.48f * detailA);

        float detailR = cyanR, detailG = cyanG, detailB = cyanB;
        const wchar_t* detailTitle = L"NO PAYLOAD";
        const wchar_t* detailType = L"EMPTY SLOT";
        bool detailIsMeta = false, detailIsTheme = false, detailIsAug = false;
        int detailId = -1;
        if (s_selKey >= 0) {
            if (s_selKey < KEY_THEME) {
                detailIsMeta = true; detailId = s_selKey - KEY_META;
                if (detailId >= 0 && detailId < META_COUNT) {
                    detailTitle = MetaName(detailId);
                    detailType = L"PERMANENT MODULE";
                }
            } else if (s_selKey < KEY_AUG) {
                detailIsTheme = true; detailId = s_selKey - KEY_THEME;
                if (detailId >= 0 && detailId < ACCENT_COUNT) {
                    detailTitle = AccentName(detailId);
                    detailType = L"SIGNAL PROFILE";
                    detailR = ACCENT_THEMES[detailId].r;
                    detailG = ACCENT_THEMES[detailId].g;
                    detailB = ACCENT_THEMES[detailId].b;
                }
            } else {
                detailIsAug = true; detailId = s_selKey - KEY_AUG;
                if (detailId >= 0 && detailId < AUG_TOTAL) {
                    detailTitle = ALL_AUGS[detailId].locName[li];
                    detailType = L"IN-RUN FIELD PAYLOAD";
                    GetRarityColor(ALL_AUGS[detailId].rarity,
                                   detailR, detailG, detailB);
                }
            }
        }

        const float heroY = contentTop + 32.0f * ui;
        drawModuleGlyph(s_selKey, detailX + 22.0f * ui, heroY + 26.0f * ui,
                        detailR, detailG, detailB, 0.92f);
        DrawShadowedText(g_TextS, detailType, detailX + 54.0f * ui,
                         heroY + 5.0f * ui, 0.38f * ui,
                         detailR, detailG, detailB, 0.82f * detailA, 0.60f);
        drawFit(detailTitle, detailX + 54.0f * ui, heroY + 23.0f * ui,
                0.82f * ui, detailW - 72.0f * ui,
                paleR, paleG, paleB, 0.98f);
        drawRule(detailX, heroY + 67.0f * ui, detailW,
                 detailR, detailG, detailB, 0.28f);
        drawRule(detailX, heroY + 67.0f * ui, detailW * 0.26f,
                 detailR, detailG, detailB, 0.76f);

        const float infoY = heroY + 98.0f * ui;
        const float infoW = std::max(200.0f * ui, detailW * 0.46f);
        const float exhibitX = detailX + detailW * 0.55f;
        const float exhibitY = heroY + 91.0f * ui;
        const float exhibitW = std::max(180.0f * ui, detailW * 0.40f);
        const float exhibitH = std::max(170.0f * ui,
                                        contentBottom - exhibitY - 78.0f * ui);
        auto drawExhibitFrame = [&](const wchar_t* label, float r, float g2, float b,
                                     float a) {
            BindMainShader();
            LogoLine(exhibitX + 16.0f * ui, exhibitY + 42.0f * ui,
                     exhibitX + exhibitW - 16.0f * ui, exhibitY + 42.0f * ui,
                     0.72f * ui, r, g2, b, 0.30f * a);
            drawOrbitArc(exhibitX + exhibitW * 0.52f,
                         exhibitY + exhibitH * 0.53f,
                         exhibitW * 0.48f, exhibitH * 0.40f,
                         3.58f, 5.02f, r, g2, b, 0.92f * ui, 0.62f * a);
            drawOrbitArc(exhibitX + exhibitW * 0.52f,
                         exhibitY + exhibitH * 0.53f,
                         exhibitW * 0.41f, exhibitH * 0.33f,
                         5.22f, 6.62f, r, g2, b, 0.62f * ui, 0.28f * a);
            DrawAstralCoreConstellation(exhibitX + exhibitW * 0.52f,
                                        exhibitY + exhibitH * 0.53f,
                                        std::min(exhibitW, exhibitH) * 0.22f,
                                        r, g2, b, now, 0.34f * a, ui);
            DrawVisibleConstellNode(exhibitX + exhibitW * 0.05f,
                                    exhibitY + exhibitH * 0.53f,
                                    3.0f * ui, r, g2, b, 0.52f * a);
            DrawShadowedText(g_TextS, label, exhibitX + 18.0f * ui,
                             exhibitY + 14.0f * ui, 0.42f * ui,
                             r, g2, b, 0.92f * a, 0.70f);
        };
        if (detailIsMeta && detailId >= 0 && detailId < META_COUNT) {
            const MetaDef& md = META_DEFS[detailId];
            const int curLv = g_MetaLv[detailId];
            const long long cost = MetaNextCost(detailId);
            drawExhibitFrame(L"MODULE TUNING RACK", detailR, detailG, detailB, detailA);
            {
                const float rackY = exhibitY + exhibitH * 0.56f;
                const float rackL = exhibitX + 30.0f * ui;
                const float rackR = exhibitX + exhibitW - 30.0f * ui;
                const int rackCount = std::max(3, std::min(md.maxLv, 8));
                drawRule(rackL, rackY, rackR - rackL, detailR, detailG, detailB, 0.42f);
                for (int n = 0; n < rackCount; ++n) {
                    const float x = rackL + (rackR - rackL) *
                        ((rackCount <= 1) ? 0.0f : (float)n / (float)(rackCount - 1));
                    const bool filled = n < curLv;
                    const bool next = n == curLv && curLv < md.maxLv;
                    drawRect(x - 0.7f * ui, rackY - 22.0f * ui,
                             1.4f * ui, 44.0f * ui, detailR, detailG, detailB,
                             (filled ? 0.58f : 0.18f) * detailA);
                    drawDiamond(x, rackY,
                                (filled ? 9.0f : next ? 7.0f : 5.0f) * ui,
                                detailR, detailG, detailB,
                                (filled ? 0.92f : next ? 0.58f : 0.24f) * detailA);
                    if (filled)
                        drawDiamond(x, rackY, 3.0f * ui, 1.0f, 1.0f, 1.0f,
                                    0.55f * detailA);
                }
                wchar_t rackBuf[48]; swprintf_s(rackBuf, L"TUNE  %02d / %02d", curLv, md.maxLv);
                DrawShadowedText(g_TextS, rackBuf,
                                 exhibitX + 30.0f * ui, exhibitY + exhibitH - 37.0f * ui,
                                 0.42f * ui, paleR, paleG, paleB, 0.88f * detailA, 0.70f);
            }
            wchar_t levelBuf[64];
            swprintf_s(levelBuf, L"LEVEL  %d / %d", curLv, md.maxLv);
            DrawShadowedText(g_TextS, levelBuf, detailX, infoY,
                             0.42f * ui, paleR, paleG, paleB, 0.82f * detailA, 0.62f);
            const float tickY = infoY + 29.0f * ui;
            for (int lv = 0; lv < md.maxLv; ++lv) {
                const bool filled = lv < curLv;
                BindMainShader();
                drawRect(detailX + lv * 31.0f * ui, tickY,
                         23.0f * ui, 4.0f * ui,
                         filled ? detailR : 0.24f,
                         filled ? detailG : 0.30f,
                         filled ? detailB : 0.38f,
                         (filled ? 0.92f : 0.34f) * detailA);
            }
            DrawShadowedText(g_TextS, L"PERMANENT / APPLIES BEFORE DEPLOYMENT",
                             detailX, infoY + 58.0f * ui, 0.36f * ui,
                             0.58f, 0.72f, 0.84f, 0.68f * detailA, 0.58f);
            wchar_t costBuf[64];
            if (cost < 0) swprintf_s(costBuf, L"CORE MODULE MAX LEVEL");
            else swprintf_s(costBuf, L"NEXT TUNE   %lld STARDUST", cost);
            DrawShadowedText(g_TextS, costBuf, detailX, infoY + 90.0f * ui,
                             0.44f * ui, detailR, detailG, detailB,
                             0.82f * detailA, 0.62f);

            const float actionY = contentBottom - 48.0f * ui;
            const bool canBuy = cost >= 0 && g_Coins >= cost && curLv < md.maxLv;
            const bool actionHover = inputReady && mx >= detailX
                                  && mx < detailX + infoW
                                  && my >= actionY - 5.0f * ui && my < actionY + 52.0f * ui;
            if (actionHover && lmbClick && canBuy) {
                g_Coins -= cost;
                ++g_MetaLv[detailId];
                SaveGame();
            }
            wchar_t actionBuf[64];
            if (curLv >= md.maxLv) swprintf_s(actionBuf, L"MAX LEVEL");
            else swprintf_s(actionBuf, L"UPGRADE MODULE   %lld", cost);
            drawAction(detailX, actionY, std::min(310.0f * ui, infoW),
                       actionBuf, detailR, detailG, detailB,
                       canBuy || curLv >= md.maxLv, actionHover);
        } else if (detailIsTheme && detailId >= 0 && detailId < ACCENT_COUNT) {
            const AccentTheme& theme = ACCENT_THEMES[detailId];
            const bool owned = ThemeOwned(detailId);
            const bool equipped = owned && g_ThemeSel == detailId;
            const long long cost = theme.cost;
            drawExhibitFrame(L"CHROMA PROFILE", theme.r, theme.g, theme.b, detailA);
            for (int bar = 0; bar < 9; ++bar) {
                const float x = exhibitX + 28.0f * ui + bar *
                    ((exhibitW - 56.0f * ui) / 9.0f);
                const float wave = 0.46f + 0.54f * sinf(now * 2.1f + bar * 0.52f);
                BindMainShader();
                drawRect(x, exhibitY + exhibitH * 0.50f - wave * 38.0f * ui,
                         11.0f * ui, wave * 76.0f * ui,
                         theme.r, theme.g, theme.b,
                         (owned ? 0.62f : 0.20f) * detailA);
            }
            drawDiamond(exhibitX + exhibitW * 0.5f, exhibitY + exhibitH * 0.50f,
                        14.0f * ui, theme.r, theme.g, theme.b, 0.72f * detailA);
            drawDiamond(exhibitX + exhibitW * 0.5f, exhibitY + exhibitH * 0.50f,
                        5.0f * ui, 0.01f, 0.02f, 0.04f, 0.92f * detailA);
            DrawShadowedText(g_TextS, L"SIGNAL PROFILE / UI TELEMETRY",
                             detailX, infoY, 0.40f * ui,
                             paleR, paleG, paleB, 0.78f * detailA, 0.60f);
            const float swatchY = infoY + 31.0f * ui;
            for (int s = 0; s < 8; ++s) {
                BindMainShader();
                const float wave = 0.55f + 0.45f * sinf(now * 2.0f + s * 0.70f);
                drawRect(detailX + s * 28.0f * ui, swatchY,
                         18.0f * ui, (8.0f + 18.0f * wave) * ui,
                         theme.r, theme.g, theme.b,
                         (owned ? 0.25f + 0.07f * wave : 0.08f) * detailA);
            }
            drawRule(detailX, swatchY + 25.0f * ui,
                     std::min(280.0f * ui, detailW * 0.72f),
                     theme.r, theme.g, theme.b, 0.76f);
            const wchar_t* ownership = equipped ? L"CURRENTLY ROUTED"
                                  : (owned ? L"OWNED / READY" : L"LOCKED / PURCHASE TO UNLOCK");
            DrawShadowedText(g_TextS, ownership, detailX, infoY + 90.0f * ui,
                             0.40f * ui, theme.r, theme.g, theme.b,
                             0.88f * detailA, 0.60f);
            wchar_t priceBuf[64];
            if (owned) swprintf_s(priceBuf, L"SIGNAL COST   --");
            else swprintf_s(priceBuf, L"SIGNAL COST   %lld STARDUST", cost);
            DrawShadowedText(g_TextS, priceBuf, detailX, infoY + 121.0f * ui,
                             0.44f * ui, theme.r, theme.g, theme.b,
                             0.80f * detailA, 0.60f);

            const float actionY = contentBottom - 48.0f * ui;
            const bool actionEnabled = !equipped && (owned || g_Coins >= cost);
            const bool actionHover = inputReady && mx >= detailX
                                  && mx < detailX + infoW
                                  && my >= actionY - 5.0f * ui && my < actionY + 52.0f * ui;
            if (actionHover && lmbClick && actionEnabled) {
                if (!owned) {
                    g_Coins -= cost;
                    g_ThemeOwned |= (1 << detailId);
                }
                g_ThemeSel = detailId;
                ApplyAccentTheme();
                SaveGame();
            }
            const wchar_t* actionLabel = equipped ? L"EQUIPPED"
                                    : (owned ? L"EQUIP" : L"UNLOCK");
            drawAction(detailX, actionY, std::min(310.0f * ui, infoW),
                       actionLabel, theme.r, theme.g, theme.b,
                       actionEnabled || equipped, actionHover);
        } else if (detailIsAug && detailId >= 0 && detailId < AUG_TOTAL) {
            const AugDef& ad = ALL_AUGS[detailId];
            const wchar_t* desc = AugDesc(ad);
            drawExhibitFrame(L"FIELD PAYLOAD SPECIMEN", detailR, detailG, detailB, detailA);
            {
                const float cx = exhibitX + exhibitW * 0.50f;
                const float cy = exhibitY + exhibitH * 0.53f;
                const float outer = std::min(exhibitW, exhibitH) * 0.27f;
                const float inner = outer * 0.54f;
                for (int ring = 0; ring < 2; ++ring) {
                    const float rr = ring == 0 ? inner : outer;
                    float px = cx + cosf(now * (ring == 0 ? 0.34f : -0.22f)) * rr;
                    float py = cy + sinf(now * (ring == 0 ? 0.34f : -0.22f)) * rr * 0.62f;
                    for (int seg = 1; seg <= 24; ++seg) {
                        const float ang = now * (ring == 0 ? 0.34f : -0.22f) +
                            6.2831853f * (float)seg / 24.0f;
                        const float nx = cx + cosf(ang) * rr;
                        const float ny = cy + sinf(ang) * rr * 0.62f;
                        if ((seg + ring) % 3 != 0)
                            LogoLine(px, py, nx, ny, 0.65f * ui,
                                     detailR, detailG, detailB,
                                     (ring == 0 ? 0.30f : 0.20f) * detailA);
                        px = nx; py = ny;
                    }
                }
                for (int node = 0; node < 6; ++node) {
                    const float ang = now * -0.22f + node * 1.0472f;
                    drawDiamond(cx + cosf(ang) * outer,
                                cy + sinf(ang) * outer * 0.62f,
                                4.8f * ui, detailR, detailG, detailB,
                                0.62f * detailA);
                }
                drawModuleGlyph(s_selKey, cx, cy, detailR, detailG, detailB, 0.94f);
            }
            DrawShadowedText(g_TextS, L"FIELD PAYLOAD / CATALOGUE",
                             detailX, infoY, 0.40f * ui,
                             paleR, paleG, paleB, 0.78f * detailA, 0.60f);
            if (desc && desc[0]) {
                std::wstring text(desc);
                size_t pos = 0;
                int line = 0;
                while (pos < text.size() && line < 5) {
                    size_t slash = text.find(L" / ", pos);
                    std::wstring segment = slash == std::wstring::npos
                        ? text.substr(pos) : text.substr(pos, slash - pos);
                    pos = slash == std::wstring::npos ? text.size() : slash + 3;
                    while (!segment.empty() && segment.front() == L' ') segment.erase(0, 1);
                    while (!segment.empty() && segment.back() == L' ') segment.pop_back();
                    drawFit(segment.c_str(), detailX, infoY + (31.0f + line * 27.0f) * ui,
                            0.44f * ui, infoW - 18.0f * ui,
                            0.82f, 0.88f, 0.94f, 0.82f);
                    ++line;
                }
            }
            drawRule(detailX, infoY + 181.0f * ui,
                     std::min(300.0f * ui, infoW),
                     detailR, detailG, detailB, 0.52f);
            DrawShadowedText(g_TextS, L"SELECTED HERE / APPLIED DURING RUN",
                             detailX, infoY + 199.0f * ui, 0.38f * ui,
                             detailR, detailG, detailB, 0.78f * detailA, 0.60f);
            drawAction(detailX, contentBottom - 48.0f * ui,
                       std::min(310.0f * ui, infoW),
                       L"PASSIVE", detailR, detailG, detailB, false, false);
        } else {
            DrawShadowedText(g_TextS, L"SELECT A MODULE TO PREPARE THE RUN",
                             detailX, infoY, 0.44f * ui,
                             0.72f, 0.82f, 0.90f, 0.72f * detailA, 0.62f);
        }

        drawRule(deckX, deckB + 12.0f * ui, deckW, cyanR, cyanG, cyanB, 0.18f);
        DrawShadowedText(g_TextS, L"READY CHECK  /  MODULES ROUTED BEFORE LAUNCH",
                         deckX, deckB + 25.0f * ui, 0.34f * ui,
                         0.52f, 0.68f, 0.80f, 0.62f * designA, 0.58f);
        DrawShadowedText(g_TextS, L"ESC / RMB  RETURN",
                         deckR - g_TextS.Width(L"ESC / RMB  RETURN", 0.34f * ui),
                         deckB + 25.0f * ui, 0.34f * ui,
                         0.52f, 0.68f, 0.80f, 0.62f * designA, 0.58f);
        drawOrbitArc(deckX + deckW * 0.50f, deckY + deckH * 0.52f,
                     deckW * 0.56f, deckH * 0.58f,
                     3.86f, 4.48f, cyanR, cyanG, cyanB,
                     0.72f * ui, 0.24f);
        drawOrbitArc(deckX + deckW * 0.50f, deckY + deckH * 0.52f,
                     deckW * 0.56f, deckH * 0.58f,
                     5.72f, 6.44f, cyanR, cyanG, cyanB,
                     0.72f * ui, 0.20f);

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

    // ── Main-menu expansion ghost ──────────────────────────────────────
    const float oldOut = 1.0f - oldMenuA;
    const float returnLogoA = s_backExit ? (0.42f + 0.58f * backP) : 0.42f;
    DrawMainOnedowLogo(sw, sh,
                       returnLogoA * oldMenuA,
                       LogoClamp01(oldMenuA + 0.22f * wake),
                       sw * 0.30f - 160.0f * oldOut,
                       0.82f);
    {
        struct GhostDef { const wchar_t* route; const wchar_t* sub; };
        static const GhostDef kGhostMenu[5] = {
            { L"PLAY",        L"\uC2DC\uC791" },
            { L"SHOP",        L"\uC0C1\uC810" },
            { L"ASTRAL_LOG",  L"\uB3C4\uAC10" },
            { L"SETTING",     L"\uC124\uC815" },
            { L"EXIT",        L"\uAC8C\uC784 \uC885\uB8CC" },
        };
        const float ghostSlide = 250.0f * oldOut;
        const float anchorX = mainX - ghostSlide - 36.0f;
        BindMainShader();
        drawRect(anchorX, std::max(22.0f, MainLogoTop(sh) - 20.0f),
                 1.2f, mainTotalH + 170.0f,
                 0.48f, 0.82f, 1.0f, 0.12f * oldMenuA);
        for (int i = 0; i < 5; ++i) {
            const bool focus = (!s_backExit && i == 1);
            const float rowY = mainY + (float)i * (mainBH + mainGap);
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
            while (routeSc > 0.82f && g_TextL.Width(kGhostMenu[i].route, routeSc) > mainBW)
                routeSc -= 0.04f;
            const float subSc = 0.48f;
            const float routeY = rowY + 2.0f;
            const float subY = routeY + g_TextL.Height(kGhostMenu[i].route, routeSc) - 3.0f;
            const float gr = s_backExit ? 1.0f : 1.0f - 0.52f * (1.0f - active);
            const float gg = s_backExit ? 1.0f : 1.0f - 0.18f * (1.0f - active);
            const float gb = 1.0f;
            g_TextL.Draw(kGhostMenu[i].route, rowX, routeY, routeSc,
                         gr, gg, gb, rowA);
            g_TextS.Draw(kGhostMenu[i].sub, rowX + 4.0f, subY, subSc,
                         0.70f + 0.16f * active,
                         0.75f + 0.14f * active,
                         0.82f + 0.10f * active,
                         (s_backExit ? 0.70f : focus ? 0.82f : 0.26f) * oldMenuA);
        }
        const float bridgeA = std::min(oldMenuA, wake);
        LogoLine(mainX - ghostSlide + mainBW * 0.42f, armoryMidY,
                 rootX - 26.0f * uiS, armoryMidY,
                 1.1f * uiS, 0.48f, 0.82f, 1.0f, 0.28f * bridgeA);
        drawDiamond(rootX - 26.0f * uiS, armoryMidY,
                    3.5f * uiS, 0.48f, 0.82f, 1.0f, 0.52f * bridgeA);
    }

    // ── Cascading text tree: root commands ─────────────────────────────
    static const wchar_t* kTabLbl[SHOP_TAB_COUNT][2] = {
        { L"\xC2DC\xC791 \xBAA8\xB4C8", L"START" },
        { L"\xD574\xAE08", L"UNLOCK" },
        { L"\xC18C\xCD1D", L"RIFLE"  },
        { L"\xC704\xC131", L"FIELD"  },
    };
    static const float kTR[SHOP_TAB_COUNT] = { 0.70f, 0.38f, 0.72f, 0.62f };
    static const float kTG[SHOP_TAB_COUNT] = { 0.70f, 0.82f, 0.92f, 0.48f };
    static const float kTB[SHOP_TAB_COUNT] = { 0.70f, 1.00f, 0.42f, 1.00f };

    const float categoryWake = wake * categoryA;
    BindMainShader();
    drawRect(rootX - 36.0f, rootY - 20.0f,
             1.2f, rootH * 5.0f + rootGap * 4.0f + 40.0f,
             0.48f, 0.82f, 1.00f, 0.16f * categoryWake);

    const int ROOT_COUNT = SHOP_TAB_COUNT + 1;
    for (int i = 0; i < ROOT_COUNT; i++) {
        const bool isBack = (i == SHOP_TAB_COUNT);
        const float tx = rootX - 58.0f * itemA;
        const float ty = rootY + (float)i * (rootH + rootGap);
        const float hitX = tx - 26.0f;
        const float hitRight = tx + rootW + 18.0f * uiS;
        const bool hov = inputReady
                       && (mx >= hitX && mx < hitRight
                       && my >= ty && my < ty + rootH);
        float& hovT = isBack ? s_backHov : s_tabHov[i];
        hovT = UpdateMenuCommandHover(hovT, hov, dt);

        if (hov && lmb && !g_LmbPrev) {
            if (isBack) {
                beginBack();
            } else {
                s_tab = i;
                s_prevTab = -1;
                s_browseItems = true;
                s_browseInputLock = true;
            }
        }

        const bool sel = (!isBack && s_tab == i);
        const float rr = isBack ? 0.42f : kTR[i];
        const float gg = isBack ? 0.62f : kTG[i];
        const float bb = isBack ? 0.78f : kTB[i];
        const wchar_t* route = isBack ? L"BACK" : kTabLbl[i][1];
        const wchar_t* sub = isBack ? L"\uB4A4\uB85C" : kTabLbl[i][0];
        const float reveal = s_backExit ? wake : MenuCommandReveal(g_ShopEntryT, i);
        const float rowA = reveal * categoryWake;
        const float selectPulse = sel ? (0.16f + 0.16f * sinf(now * 4.0f)) : 0.0f;
        const float slide = (1.0f - reveal) * 34.0f;
        const float bx = tx + slide - 10.0f * hovT;
        const float by = ty;

        DrawUnifiedMenuCommand(route, sub, bx, by, rootW, rootH,
                               rr, gg, bb, rowA, hovT, sel, selectPulse,
                               now + (float)i * 0.17f);
    }

    // ── Depth 2: item branches ─────────────────────────────────────────
    const float itemWake = wake * itemA;
    const float itemSlideX = (1.0f - itemA) * 54.0f * uiS;
    const float itemListX = depth2X + itemSlideX;
    const float itemBackY = rootY;
    const bool itemBackHov = inputReady && !s_browseItems
        && mx >= rootX - 24.0f * uiS && mx < rootX + rootW + 28.0f * uiS
        && my >= itemBackY && my < itemBackY + rootH;
    s_backHov = UpdateMenuCommandHover(s_backHov, itemBackHov, dt);
    if (itemBackHov && lmb && !g_LmbPrev) {
        s_browseItems = false;
        s_browseInputLock = true;
    }
    wchar_t armoryPath[80];
    swprintf_s(armoryPath, L"SHOP / %ls", kTabLbl[s_tab][1]);
    if (!s_browseItems) {
        DrawUnifiedMenuCommand(armoryPath, nli == 0 ? L"\xCE74\xD14C\xACE0\xB9AC" : L"CATEGORY",
                               rootX - 10.0f * s_backHov, itemBackY, rootW, rootH,
                               kTR[s_tab], kTG[s_tab], kTB[s_tab], itemWake,
                               s_backHov, true, 0.10f + 0.08f * sinf(now * 4.0f), now);
    }

    if (detailWake > 0.002f) {
        int visibleItems = 0;
        for (int i = 0; i < s_itemCount; ++i)
            if (!s_items[i].isGroup) ++visibleItems;
        wchar_t indexBuf[64];
        swprintf_s(indexBuf, nli == 0 ? L"%02d \uAC1C \uB178\uB4DC" : L"%02d NODES", visibleItems);
        DrawShadowedText(g_TextS,
                         nli == 0 ? L"SHOP \uC870\uB9BD \uBAA9\uB85D" : L"SHOP ASSEMBLY INDEX",
                         depth2X, workY + 22.0f * uiS,
                         0.52f * uiS, 0.68f, 0.80f, 0.94f,
                         0.78f * detailWake, 0.62f);
        const float countSc = 0.46f * uiS;
        DrawShadowedText(g_TextS, indexBuf,
                         rightX - 28.0f * uiS - g_TextS.Width(indexBuf, countSc),
                         workY + 24.0f * uiS, countSc,
                         0.62f, 0.72f, 0.86f, 0.72f * detailWake, 0.58f);
    }

    BindMainShader();
    const float parentY = rootY + (float)s_tab * (rootH + rootGap) + rootH * 0.5f;
    const float trunkX = depth2X - 26.0f * uiS;
    LogoLine(rootX + rootW + 18.0f * uiS, parentY, trunkX, parentY,
             1.0f * uiS, kTR[s_tab], kTG[s_tab], kTB[s_tab], 0.34f * wake);

    const float itemH  = 50.0f * uiS;
    const float grpH   = 40.0f * uiS;
    const float listX  = itemListX;
    const float listW2 = depth2W;

    float totalH = 0.0f;
    for (int i = 0; i < s_itemCount; i++)
        totalH += s_items[i].isGroup ? grpH : itemH;
    float maxScroll = std::max(0.0f, totalH - depth2H + 16.0f * uiS);

    bool overList = inputReady && s_browseItems
                 && (mx >= depth2X - 38.0f * uiS && mx < depth2X + depth2W + 26.0f * uiS
                  && my >= depth2Y && my < depth2Y + depth2H);
    if (overList && g_ScrollAccum != 0.0f)
        s_scroll -= g_ScrollAccum * 36.0f * uiS;
    g_ScrollAccum = 0.0f;
    if (s_scroll < 0.0f) s_scroll = 0.0f;
    if (s_scroll > maxScroll) s_scroll = maxScroll;

    // The shared work plate provides the contrast. Keep only a faint list-zone
    // tint so scrolling rows remain grouped without becoming another card.
    BindMainShader();
    drawRect(depth2X - 6.0f * uiS, depth2Y,
             depth2W + 12.0f * uiS, depth2H,
             0.030f, 0.040f, 0.062f, 0.0f);

    BatchFlush();
    glEnable(GL_SCISSOR_TEST);
    {
        int scX = (int)(depth2X - 42.0f * uiS), scY = (int)(sh - (depth2Y + depth2H));
        int scW = (int)(depth2W + 70.0f * uiS), scH = (int)depth2H;
        if (scX < 0) scX = 0; if (scY < 0) scY = 0;
        glScissor(scX, scY, scW, scH);
    }

    float curY = depth2Y + 6.0f * uiS - s_scroll;
    const float treeTop = curY;
    const float treeBot = curY + totalH - 8.0f * uiS;
    BindMainShader();
    LogoLine(trunkX, std::min(parentY, treeTop + 12.0f * uiS),
             trunkX, std::max(parentY, treeBot),
             0.9f * uiS, kTR[s_tab], kTG[s_tab], kTB[s_tab], 0.22f * itemWake);
    for (int i = 0; i < s_itemCount; i++) {
        const SLItem& itm = s_items[i];
        float rh = itm.isGroup ? grpH : itemH;
        float ry = curY; curY += rh;
        if (ry + rh < depth2Y || ry > depth2Y + depth2H) {
            s_itemHov[i] = UpdateMenuCommandHover(s_itemHov[i], false, dt);
            continue;
        }

        if (itm.isGroup) {
            BindMainShader();
            float mid = ry + grpH * 0.5f;
            float grpPulse = 0.72f + 0.28f * sinf(now * 2.2f + (float)i * 0.41f);
            LogoLine(trunkX, mid, listX - 8.0f * uiS, mid,
                     0.9f * uiS, itm.r, itm.g, itm.b, 0.28f * itemWake);
            drawRect(listX + 4.0f*uiS, mid - 0.5f*uiS,
                     listW2 * 0.96f, 1.0f*uiS,
                     itm.r, itm.g, itm.b, (0.11f + 0.030f * grpPulse) * itemWake);
            drawDiamond(listX - 8.0f*uiS, mid,
                        (3.8f + 0.6f * grpPulse) * uiS,
                        itm.r, itm.g, itm.b, 0.58f * itemWake);
            const float gsc = 0.70f * uiS;
            DrawShadowedText(g_TextS, itm.label, listX + 14.0f*uiS,
                             ry + (grpH - g_TextS.Height(itm.label, gsc)) * 0.5f,
                             gsc, 0.88f, 0.95f, 1.0f, 0.86f * itemWake, 0.64f);
        } else {
            bool isSel = (s_selKey == itm.key);
            bool hov2  = inputReady && s_browseItems
                       && (mx >= listX - 24.0f * uiS && mx < listX + listW2 + 18.0f * uiS
                       && my >= ry - 4.0f * uiS && my < ry + itemH + 5.0f * uiS);
            s_itemHov[i] = UpdateMenuCommandHover(s_itemHov[i], hov2, dt);
            const float itemActive = std::max(s_itemHov[i], isSel ? 1.0f : 0.0f);
            if (hov2 && lmb && !g_LmbPrev) s_selKey = itm.key;
            BindMainShader();
            const float cardY = ry + 2.0f * uiS;
            const float cardH = itemH - 4.0f * uiS;
            const float cardPulse = isSel ? (0.60f + 0.40f * sinf(now * 3.2f + (float)i * 0.31f)) : 0.0f;
            const float ndy = cardY + cardH * 0.5f;

            // Faint background — selected only, Ambient tier
            // Selection is carried by the rail and scan line only.  The shop
            // should read as an overlay on the live scene, not as a stack of
            // item cards that hides the background.
            if (isSel) {
                drawRect(listX - 1.0f * uiS, cardY + 5.0f * uiS,
                         2.4f * uiS, cardH - 10.0f * uiS,
                         itm.r, itm.g, itm.b,
                         (0.52f + 0.14f * cardPulse) * itemWake);
                drawRect(listX + 18.0f * uiS, cardY + cardH - 1.0f * uiS,
                         listW2 - 34.0f * uiS, 1.0f * uiS,
                         itm.r, itm.g, itm.b,
                         (0.18f + 0.06f * cardPulse) * itemWake);
            }

            // Branch connector
            LogoLine(trunkX, ndy, listX - 12.0f * uiS, ndy,
                     0.8f * uiS, itm.r, itm.g, itm.b, (isSel ? 0.40f : 0.16f) * itemWake);

            // Drifting scan line — Structural tier, no static centering
            if (itemActive > 0.01f) {
                const float scanY = cardY + cardH * (0.34f + 0.32f * fmodf(now * 0.55f + (float)i * 0.23f, 1.0f));
                drawRect(listX + 20.0f * uiS, scanY, listW2 - 22.0f * uiS, 1.0f * uiS,
                         itm.r, itm.g, itm.b,
                         PulsedStructuralAlpha(0.055f * itemActive, 0.030f * itemActive,
                                               now * 1.1f + (float)i) * itemWake);
            }

            // Left accent bar — grows with active state
            {
                const float barH = cardH * (0.40f + 0.35f * itemActive) - 8.0f * uiS;
                const float barA = isSel ? (0.68f + 0.14f * cardPulse)
                                         : 0.28f * s_itemHov[i];
                if (barA > 0.01f)
                    drawRect(listX - 2.0f * uiS, ndy - barH * 0.5f,
                             1.8f * uiS, barH, itm.r, itm.g, itm.b, barA * itemWake);
            }

            // Left connector diamond
            drawDiamond(listX - 12.0f * uiS, ndy,
                        (2.2f + 1.3f * itemActive) * uiS,
                        itm.r, itm.g, itm.b,
                        (0.16f + 0.50f * itemActive) * itemWake);

            // Right-edge corner diamonds — item color (not white)
            if (isSel) {
                drawDiamond(listX + listW2 - 13.0f * uiS, cardY + 13.0f * uiS,
                            (3.0f + 0.8f * cardPulse) * uiS,
                            itm.r, itm.g, itm.b, (0.42f + 0.12f * cardPulse) * itemWake);
                drawDiamond(listX + listW2 - 13.0f * uiS, cardY + cardH - 13.0f * uiS,
                            (2.6f + 0.6f * cardPulse) * uiS,
                            itm.r, itm.g, itm.b, (0.28f + 0.10f * cardPulse) * itemWake);
            }

            // Label text
            float tsc = 0.68f * uiS;
            DrawShadowedText(g_TextS, itm.label, listX + (isSel ? 20.0f : 14.0f) * uiS,
                             cardY + (cardH - g_TextS.Height(itm.label, tsc)) * 0.5f,
                             tsc,
                             0.66f + 0.34f * itemActive,
                             0.72f + 0.28f * itemActive,
                             0.80f + 0.20f * itemActive,
                             (0.64f + 0.36f * itemActive) * itemWake, 0.68f);
        }
    }

    BatchFlush();
    glDisable(GL_SCISSOR_TEST);

    // ── Right panel metrics ─────────────────────────────────────────────
    const float rPad = 28.0f * uiS;
    const float rInX = rightX + rPad;
    const float rInY = rightAreaY + rPad;
    const float rInH = rightAreaH - rPad * 2.0f;

    if (false && s_selKey >= 0 && rightWake > 0.02f) {
        float da = s_detailT * rightWake;

        if (s_selKey < KEY_THEME) {
            // ── META upgrade ──────────────────────────────────────────────
            int mi = s_selKey - KEY_META;
            if (mi >= 0 && mi < META_COUNT) {
                const MetaDef& md = META_DEFS[mi];
                int curLv = g_MetaLv[mi];
                long long cost = MetaNextCost(mi);

                BindMainShader();
                g_TextS.Draw(L"UNLOCK", rInX, rInY, 0.42f*uiS,
                             0.38f, 0.82f, 1.00f, 0.80f * da);
                g_TextL.Draw(MetaName(mi), rInX, rInY + 22.0f*uiS,
                             0.90f*uiS, 1.0f, 1.0f, 1.0f, 0.95f * da);

                float barY = rInY + 82.0f*uiS;
                float barW = rightW * 0.42f;
                float prog = (md.maxLv > 0) ? (float)curLv / (float)md.maxLv : 1.0f;
                wchar_t lvBuf[32]; swprintf_s(lvBuf, L"Lv %d / %d", curLv, md.maxLv);
                g_TextS.Draw(lvBuf, rInX, barY - 22.0f*uiS, 0.43f*uiS,
                             0.70f, 0.80f, 0.95f, 0.80f * da);
                BindMainShader();
                drawRect(rInX, barY, barW, 8.0f*uiS, 0.10f, 0.12f, 0.18f, 0.90f * da);
                drawRect(rInX, barY, barW * prog, 8.0f*uiS, 0.38f, 0.82f, 1.00f, da);

                float btnX = rInX, btnY = barY + 20.0f*uiS;
                float btnW = 200.0f*uiS, btnH = 42.0f*uiS;

                if (curLv >= md.maxLv) {
                    drawRect(btnX, btnY, btnW, btnH, 0.10f, 0.20f, 0.12f, 0.80f * da);
                    float mw = g_TextS.Width(L"MAX", 0.48f*uiS);
                    g_TextS.Draw(L"MAX", btnX + (btnW - mw)*0.5f, btnY + 10.0f*uiS,
                                 0.48f*uiS, 0.40f, 0.90f, 0.45f, da);
                } else {
                    bool canBuy = (g_Coins >= cost);
                    bool bHov   = (mx >= btnX && mx < btnX+btnW && my >= btnY && my < btnY+btnH);
                    bool bClick = bHov && lmb && !g_LmbPrev;
                    BindMainShader();
                    drawRect(btnX, btnY, btnW, btnH,
                             canBuy ? (bHov ? 0.18f : 0.12f) : 0.12f,
                             canBuy ? (bHov ? 0.28f : 0.18f) : 0.10f,
                             canBuy ? (bHov ? 0.48f : 0.34f) : 0.10f,
                             0.90f * da);
                    drawConstellFrame(btnX, btnY, btnW, btnH,
                                      0.38f, 0.82f, 1.00f,
                                      (canBuy ? (bHov ? 0.90f : 0.50f) : 0.22f) * da,
                                      10.0f*uiS, 2.5f*uiS);
                    wchar_t btnLbl[64]; swprintf_s(btnLbl, L"UPGRADE  %lld", cost);
                    float bw2 = g_TextS.Width(btnLbl, 0.48f*uiS);
                    g_TextS.Draw(btnLbl, btnX + (btnW - bw2)*0.5f, btnY + 11.0f*uiS,
                                 0.48f*uiS,
                                 canBuy ? 0.70f : 0.38f,
                                 canBuy ? 0.90f : 0.38f,
                                 canBuy ? 1.00f : 0.42f, da);
                    if (bClick && canBuy) {
                        g_Coins -= cost; g_MetaLv[mi]++; SaveGame();
                    }
                }

                wchar_t hintBuf[80];
                swprintf_s(hintBuf, L"Base cost: %lld  x1.6 per level", md.baseCost);
                g_TextS.Draw(hintBuf, rInX, btnY + btnH + 12.0f*uiS,
                             0.38f*uiS, 0.50f, 0.60f, 0.74f, 0.62f * da);
                // ── 티어 시각화 (diamond 5칸) ──
                {
                    float tierY = btnY + btnH + 36.0f*uiS;
                    drawRect(rInX, tierY - 6.0f*uiS, rightW - rPad*2.0f, 1.0f*uiS,
                             0.38f, 0.82f, 1.00f, 0.16f * da);
                    for (int di = 0; di < md.maxLv && di < 8; ++di) {
                        bool filled = (di < curLv);
                        float dA = filled ? 0.80f : 0.18f;
                        drawDiamond(rInX + di * 20.0f*uiS + 8.0f*uiS, tierY + 8.0f*uiS,
                                    (filled ? 7.0f : 5.5f)*uiS, 0.38f, 0.82f, 1.00f, dA * da);
                    }
                    // next tier highlight
                    if (curLv < md.maxLv) {
                        float nhX = rInX + curLv * 20.0f*uiS + 8.0f*uiS;
                        float nhPulse = 0.6f + 0.4f * sinf(now * 3.0f);
                        drawDiamond(nhX, tierY + 8.0f*uiS, 7.0f*uiS,
                                    0.38f, 0.82f, 1.00f, 0.45f * nhPulse * da);
                    }
                }
            }

        } else if (s_selKey < KEY_AUG) {
            // ── THEME cosmetic ────────────────────────────────────────────
            int ti = s_selKey - KEY_THEME;
            if (ti >= 0 && ti < ACCENT_COUNT) {
                const AccentTheme& th = ACCENT_THEMES[ti];
                bool owned = ThemeOwned(ti);
                bool isCur = (g_ThemeSel == ti);

                BindMainShader();
                g_TextS.Draw(L"COSMETIC", rInX, rInY, 0.34f*uiS,
                             0.92f, 0.72f, 0.22f, 0.80f * da);
                g_TextL.Draw(AccentName(ti), rInX, rInY + 22.0f*uiS,
                             0.78f*uiS, th.r, th.g, th.b, 0.95f * da);

                float swY = rInY + 82.0f*uiS;
                float swW = 130.0f*uiS, swH = 64.0f*uiS;
                BindMainShader();
                drawRect(rInX, swY, swW, swH,
                         th.r * (owned ? 1.0f : 0.28f),
                         th.g * (owned ? 1.0f : 0.28f),
                         th.b * (owned ? 1.0f : 0.28f), da);
                drawConstellFrame(rInX, swY, swW, swH, th.r, th.g, th.b,
                                  0.65f * da, 12.0f*uiS, 3.0f*uiS);

                float btnX = rInX, btnY = swY + swH + 16.0f*uiS;
                float btnW = 200.0f*uiS, btnH = 42.0f*uiS;

                if (isCur && owned) {
                    drawRect(btnX, btnY, btnW, btnH, 0.10f, 0.20f, 0.12f, 0.80f * da);
                    drawConstellFrame(btnX, btnY, btnW, btnH,
                                      th.r, th.g, th.b, 0.55f * da, 10.0f*uiS, 2.5f*uiS);
                    float ew = g_TextS.Width(L"EQUIPPED", 0.42f*uiS);
                    g_TextS.Draw(L"EQUIPPED", btnX + (btnW - ew)*0.5f, btnY + 11.0f*uiS,
                                 0.42f*uiS, th.r, th.g, th.b, da);
                } else if (owned) {
                    bool bHov   = (mx >= btnX && mx < btnX+btnW && my >= btnY && my < btnY+btnH);
                    bool bClick = bHov && lmb && !g_LmbPrev;
                    BindMainShader();
                    drawRect(btnX, btnY, btnW, btnH,
                             bHov ? 0.14f : 0.08f, bHov ? 0.20f : 0.12f,
                             bHov ? 0.16f : 0.10f, 0.90f * da);
                    drawConstellFrame(btnX, btnY, btnW, btnH, th.r, th.g, th.b,
                                      (bHov ? 0.85f : 0.48f) * da, 10.0f*uiS, 2.5f*uiS);
                    float ew = g_TextS.Width(L"EQUIP", 0.42f*uiS);
                    g_TextS.Draw(L"EQUIP", btnX + (btnW - ew)*0.5f, btnY + 11.0f*uiS,
                                 0.42f*uiS, th.r, th.g, th.b, da);
                    if (bClick) { g_ThemeSel = ti; ApplyAccentTheme(); SaveGame(); }
                } else {
                    bool canBuy = (g_Coins >= th.cost);
                    bool bHov   = (mx >= btnX && mx < btnX+btnW && my >= btnY && my < btnY+btnH);
                    bool bClick = bHov && lmb && !g_LmbPrev;
                    BindMainShader();
                    drawRect(btnX, btnY, btnW, btnH,
                             canBuy ? (bHov ? 0.18f : 0.11f) : 0.09f,
                             canBuy ? (bHov ? 0.14f : 0.08f) : 0.06f,
                             canBuy ? (bHov ? 0.08f : 0.05f) : 0.04f,
                             0.90f * da);
                    drawConstellFrame(btnX, btnY, btnW, btnH, th.r, th.g, th.b,
                                      (canBuy ? (bHov ? 0.90f : 0.48f) : 0.20f) * da,
                                      10.0f*uiS, 2.5f*uiS);
                    wchar_t buyLbl[48]; swprintf_s(buyLbl, L"BUY  %lld", th.cost);
                    float bw3 = g_TextS.Width(buyLbl, 0.42f*uiS);
                    g_TextS.Draw(buyLbl, btnX + (btnW - bw3)*0.5f, btnY + 11.0f*uiS,
                                 0.42f*uiS, th.r, th.g, th.b, (canBuy ? 1.0f : 0.38f) * da);
                    if (bClick && canBuy) {
                        g_Coins -= th.cost;
                        g_ThemeOwned |= (1 << ti);
                        g_ThemeSel = ti; ApplyAccentTheme(); SaveGame();
                    }
                }
            }

        } else {
            // ── AUG catalog ───────────────────────────────────────────────
            int ai = s_selKey - KEY_AUG;
            if (ai >= 0 && ai < AUG_TOTAL) {
                const AugDef& ad = ALL_AUGS[ai];
                float cr2, cg2, cb2;
                GetRarityColor(ad.rarity, cr2, cg2, cb2);

                BindMainShader();
                g_TextS.Draw(GetAugBadge(ad), rInX, rInY, 0.42f*uiS,
                             cr2, cg2, cb2, 0.85f * da);
                const wchar_t* augName = ad.locName[li];
                g_TextL.Draw(augName, rInX + 6.0f*uiS, rInY + 22.0f*uiS,
                             0.90f*uiS, 1.0f, 1.0f, 1.0f, 0.95f * da);
                drawRect(rInX, rInY + 22.0f*uiS,
                         3.0f*uiS, g_TextL.Height(augName, 0.90f*uiS),
                         cr2, cg2, cb2, 0.70f * da);

                const wchar_t* desc = AugDesc(ad);
                if (desc && desc[0]) {
                    float dY = rInY + 92.0f*uiS;
                    float dsc = 0.48f * uiS;
                    float lineH = dsc * 19.0f;
                    std::wstring ds(desc);
                    size_t pos = 0; int ln = 0;
                    while (pos < ds.size() && ln < 8) {
                        size_t sl = ds.find(L" / ", pos);
                        std::wstring seg = (sl == std::wstring::npos)
                            ? ds.substr(pos) : ds.substr(pos, sl - pos);
                        pos = (sl == std::wstring::npos) ? ds.size() : sl + 3;
                        g_TextS.Draw(seg.c_str(), rInX + 6.0f*uiS, dY + ln * lineH,
                                     dsc, 0.80f, 0.85f, 0.90f, 0.82f * da);
                        ln++;
                    }
                }

                // ── 기하 미리보기 장식 ──
                {
                    float geoX = rInX + (rightW - rPad * 2.0f) * 0.5f;
                    float geoY = rInY + rInH * 0.68f;
                    float geoR  = 32.0f * uiS;
                    float geoR2 = 54.0f * uiS;
                    float geoPulse = 0.75f + 0.25f * sinf(now * 1.9f);
                    float ang1 = now * 0.52f, ang2 = -now * 0.36f + 0.785f;
                    BindMainShader();
                    for (int n = 0; n < 4; ++n) {
                        float a = ang1 + n * 1.5708f;
                        drawDiamond(geoX + cosf(a)*geoR, geoY + sinf(a)*geoR,
                                    5.5f*uiS, cr2, cg2, cb2, 0.70f * geoPulse * da);
                    }
                    for (int n = 0; n < 6; ++n) {
                        float a = ang2 + n * 1.0472f;
                        drawDiamond(geoX + cosf(a)*geoR2, geoY + sinf(a)*geoR2,
                                    3.5f*uiS, cr2, cg2, cb2, 0.38f * da);
                    }
                    drawDiamond(geoX, geoY, 12.0f*uiS, cr2, cg2, cb2, 0.85f * geoPulse * da);
                    drawDiamond(geoX, geoY,  5.0f*uiS, 0.04f, 0.05f, 0.08f, da);
                }
                g_TextS.Draw(L"CATALOG \x2014 in-run pick only",
                             rInX, rInY + rInH - 26.0f*uiS,
                             0.38f*uiS, 0.48f, 0.56f, 0.66f, 0.62f * da);
            }
        }
    }

    // Right panel: readable ARMORY detail view.
    {
        float da = std::max(0.0f, std::min(1.0f, s_detailT * detailWake));
        float sr = 0.38f, sg = 0.82f, sb = 1.00f;
        if (s_selKey >= KEY_THEME && s_selKey < KEY_AUG) {
            int ti = s_selKey - KEY_THEME;
            if (ti >= 0 && ti < ACCENT_COUNT) {
                sr = ACCENT_THEMES[ti].r; sg = ACCENT_THEMES[ti].g; sb = ACCENT_THEMES[ti].b;
            }
        } else if (s_selKey >= KEY_AUG) {
            int ai = s_selKey - KEY_AUG;
            if (ai >= 0 && ai < AUG_TOTAL)
                GetRarityColor(ALL_AUGS[ai].rarity, sr, sg, sb);
        }

        const float rInW = rightW - rPad * 2.0f;
        const float headerY = rInY + 10.0f * uiS;
        const float titleY = rInY + 10.0f * uiS;
        const float bodyY = rInY + 110.0f * uiS;
        const float bodyH = rInH - 150.0f * uiS;

        // ARMORY reads top-to-bottom inside the detail column: the constellation
        // owns the full upper bar and the selected node's explanation owns the
        // full lower bar. Keeping both bars on the same x-span prevents the
        // detail view from splitting into two unrelated floating cards.
        const float vizX = rInX;
        const float vizY = rInY + 58.0f * uiS;
        const float vizW = rInW;
        const float tagDockY = rInY + rInH - 198.0f * uiS;
        const float vizH = std::max(250.0f * uiS,
            std::min(430.0f * uiS, tagDockY - vizY - 22.0f * uiS));
        const float infoX = rInX;
        const float infoW = rInW * 0.42f;

        // Keep the scene visible, but give the readout a soft local contrast
        // pocket so it remains legible over bright world geometry.
        DrawSceneRadialVignette(rightX + rightW * 0.50f,
                                rightAreaY + rightAreaH * 0.52f,
                                std::max(260.0f * uiS, rightW * 0.56f),
                                0.16f * detailWake);

        auto L10n = [&](const wchar_t* kr, const wchar_t* en) -> const wchar_t* {
            return nli == 0 ? kr : en;
        };

        auto drawOrbitCornerArcs = [&](float x, float y, float w, float h,
                                       float r, float g, float b, float a) {
            if (a <= 0.001f) return;
            const float arcR = std::max(18.0f * uiS,
                std::min(std::min(w, h) * 0.10f, 44.0f * uiS));
            const float thick = std::max(0.75f, 0.90f * uiS);
            struct ArcDef { float cx, cy, a0, a1; };
            const ArcDef arcs[4] = {
                { x + arcR,     y + arcR,     3.1415926f, 4.7123890f },
                { x + w - arcR, y + arcR,     4.7123890f, 6.2831853f },
                { x + w - arcR, y + h - arcR, 0.0000000f, 1.5707963f },
                { x + arcR,     y + h - arcR, 1.5707963f, 3.1415926f },
            };
            for (int k = 0; k < 4; ++k) {
                for (int s = 0; s < 7; ++s) {
                    if ((s % 3) == 1) continue;
                    float t0 = (float)s / 7.0f;
                    float t1 = ((float)s + 0.62f) / 7.0f;
                    float aa0 = arcs[k].a0 + (arcs[k].a1 - arcs[k].a0) * t0;
                    float aa1 = arcs[k].a0 + (arcs[k].a1 - arcs[k].a0) * t1;
                    LogoLine(arcs[k].cx + cosf(aa0) * arcR,
                             arcs[k].cy + sinf(aa0) * arcR,
                             arcs[k].cx + cosf(aa1) * arcR,
                             arcs[k].cy + sinf(aa1) * arcR,
                             thick, r, g, b, a * 0.55f);
                }
            }
        };

        auto drawAstralMeta = [&](int seed, float r, float g, float b, float a) {
            if (a <= 0.001f) return;
            if (seed < 0) seed = 0;
            const int raH = (seed * 7 + 14) % 24;
            const int raM = (seed * 13 + 29) % 60;
            const int dec = ((seed * 19 + 62) % 125) - 62;
            const int decM = (seed * 11 + 40) % 60;
            const float lum = 0.62f + (float)((seed * 17 + 27) % 78) * 0.01f;
            const float mass = 0.84f + (float)((seed * 23 + 51) % 96) * 0.01f;
            wchar_t coordBuf[96];
            wchar_t physBuf[96];
            swprintf_s(coordBuf, L"RA %02dh %02dm / DEC %+03d\x00B0 %02d'", raH, raM, dec, decM);
            if (nli == 0)
                swprintf_s(physBuf, L"\uAD11\uB3C4 %.2f / \uC9C8\uB7C9 %.2f", lum, mass);
            else
                swprintf_s(physBuf, L"LUMINOSITY %.2f / MASS %.2f", lum, mass);
            const float sc = 0.34f * uiS;
            const float x = rInX + 18.0f * uiS;
            const float y = rightAreaY + rightAreaH - 52.0f * uiS;
            BindMainShader();
            drawRect(x - 10.0f * uiS, y - 8.0f * uiS,
                     430.0f * uiS, 42.0f * uiS,
                     0.014f, 0.020f, 0.034f, 0.36f * a);
            drawDiamond(x - 2.0f * uiS, y + 7.0f * uiS,
                        2.6f * uiS, r, g, b, 0.38f * a);
            g_TextS.Draw(coordBuf, x + 10.0f * uiS, y,
                         sc, 0.58f, 0.66f, 0.80f, 0.68f * a);
            g_TextS.Draw(physBuf, x + 10.0f * uiS, y + 18.0f * uiS,
                         sc, 0.58f, 0.66f, 0.80f, 0.56f * a);
        };

        auto drawPanelBase = [&]() {
            BindMainShader();
            drawRect(rInX, headerY + 34.0f * uiS, rInW, 2.0f * uiS,
                     sr, sg, sb, 0.22f * rightWake);
            LogoLine(rInX, rightAreaY + rightAreaH - 28.0f * uiS,
                     rInX + rInW * 0.34f, rightAreaY + rightAreaH - 28.0f * uiS,
                     1.0f * uiS, sr, sg, sb, 0.11f * rightWake);
            LogoLine(vizX - 26.0f * uiS, vizY + vizH * 0.5f,
                     vizX - 4.0f * uiS, vizY + vizH * 0.5f,
                     1.0f * uiS, sr, sg, sb, 0.18f * rightWake);
        };

        auto drawCredits = [&]() {
            wchar_t creditBuf[64];
            swprintf_s(creditBuf, nli == 0 ? L"\uBCC4\uAC00\uB8E8 :: %06lld" : L"STARDUST :: %06lld", g_Coins);
            const float sc = 0.50f * uiS;
            const float w = g_TextS.Width(creditBuf, sc);
            const float x = rightX + rightW - rPad - w;
            const float y = rightAreaY + 20.0f * uiS;
            BindMainShader();
            drawRect(x - 18.0f * uiS, y + 18.0f * uiS,
                     w + 18.0f * uiS, 1.0f * uiS,
                     1.00f, 0.84f, 0.20f, 0.28f * rightWake);
            drawDiamond(x - 8.0f * uiS, y + 18.0f * uiS,
                        4.0f * uiS, 1.00f, 0.84f, 0.20f, 0.82f * rightWake);
            DrawShadowedText(g_TextS, creditBuf, x, y,
                             sc, 1.0f, 0.88f, 0.36f, 0.98f * rightWake, 0.74f);
        };

        auto drawDataBox = [&](float x, float y, float w, const wchar_t* label,
                               const wchar_t* value, float r, float g, float b, float a) {
            const float h = 62.0f * uiS;
            BindMainShader();
            drawRect(x, y, w, h, 0.028f, 0.036f, 0.056f, 0.10f * a);
            drawRect(x, y, 2.0f * uiS, h, r, g, b, 0.70f * a);
            drawRect(x + 10.0f * uiS, y + h - 1.5f * uiS, w - 20.0f * uiS, 1.5f * uiS,
                     r, g, b, 0.22f * a);
            DrawShadowedText(g_TextS, label, x + 14.0f * uiS, y + 8.0f * uiS,
                             0.38f * uiS, 0.78f, 0.83f, 0.96f, 0.90f * a, 0.60f);
            DrawShadowedText(g_TextS, value, x + 14.0f * uiS, y + 31.0f * uiS,
                             0.54f * uiS, 0.92f, 0.96f, 1.0f, 0.98f * a, 0.70f);
        };
        (void)drawDataBox;

        auto drawHoloCross = [&](float x, float y, float s, float r, float g, float b, float a) {
            if (a <= 0.001f) return;
            BindMainShader();
            drawRect(x - s, y - 0.5f * uiS, s * 2.0f, 1.0f * uiS, r, g, b, a);
            drawRect(x - 0.5f * uiS, y - s, 1.0f * uiS, s * 2.0f, r, g, b, a);
        };

        auto drawProductHeader = [&](const wchar_t* code, const wchar_t* title,
                                     float r, float g, float b, float a) {
            const float x = infoX;
            const float y = titleY;
            const float w = infoW;
            const float h = 92.0f * uiS;
            BindMainShader();
            drawRect(x, y + 55.0f * uiS, w, 1.5f * uiS, r, g, b, 0.30f * a);
            drawRect(x, y + 57.0f * uiS, w * 0.28f, 1.0f * uiS, 1.0f, 1.0f, 1.0f, 0.18f * a);
            drawHoloCross(x, y + 2.0f * uiS, 5.0f * uiS, r, g, b, 0.34f * a);
            drawHoloCross(x + w, y + 2.0f * uiS, 5.0f * uiS, r, g, b, 0.26f * a);
            drawHoloCross(x, y + h, 5.0f * uiS, r, g, b, 0.22f * a);
            drawHoloCross(x + w, y + h, 5.0f * uiS, r, g, b, 0.30f * a);
            DrawShadowedText(g_TextS, code, x, y, 0.40f * uiS,
                             0.58f, 0.66f, 0.78f, 0.72f * a, 0.58f);
            float titleSc = 1.10f * uiS;
            while (titleSc > 0.82f * uiS && g_TextL.Width(title, titleSc) > w - 16.0f * uiS)
                titleSc -= 0.04f * uiS;
            DrawShadowedText(g_TextL, title, x, y + 28.0f * uiS,
                             titleSc, 1.0f, 1.0f, 1.0f, 0.98f * a, 0.78f);
        };

        auto drawTerminalRow = [&](float x, float y, float w,
                                   const wchar_t* label, const wchar_t* value,
                                   float r, float g, float b, float a,
                                   bool costValue = false) {
            if (a <= 0.001f) return;
            const float rowH = 34.0f * uiS;
            const float labelSc = 0.46f * uiS;
            const float valueSc = 0.54f * uiS;
            const float valueX = x + 190.0f * uiS;
            BindMainShader();
            drawRect(x, y + rowH - 3.0f * uiS, w, 1.0f * uiS,
                     r, g, b, 0.13f * a);
            drawRect(x, y + 7.0f * uiS, 2.0f * uiS, rowH - 13.0f * uiS,
                     r, g, b, 0.52f * a);
            drawDiamond(x + 9.0f * uiS, y + rowH * 0.5f,
                        2.6f * uiS, r, g, b, 0.46f * a);
            DrawShadowedText(g_TextS, label, x + 22.0f * uiS, y + 5.0f * uiS,
                             labelSc, 0.68f, 0.76f, 0.88f, 0.88f * a, 0.60f);
            DrawShadowedText(g_TextS, value, valueX, y + 2.0f * uiS,
                             valueSc,
                             costValue ? 1.0f : 0.92f,
                             costValue ? 0.86f : 0.96f,
                             costValue ? 0.36f : 1.0f,
                             0.98f * a, 0.70f);
        };

        auto metaNodeCode = [&](int id) -> const wchar_t* {
            switch (id) {
            case META_HP:       return L"STAR NODE : HP UPLINK";
            case META_DMG:      return L"STAR NODE : DAMAGE CORE";
            case META_MOVE:     return L"STAR NODE : MOBILITY DRIVE";
            case META_VISION:   return L"STAR NODE : SIGHT ARRAY";
            case META_STARTAUG: return L"STAR NODE : START MODULE";
            case META_ID_SLOT:  return L"STAR NODE : IDENTITY SOCKET";
            default:            return L"STAR NODE : UNKNOWN";
            }
        };

        auto drawNodeMap = [&](int filled, int total, const wchar_t* label,
                               float r, float g, float b, float a,
                               float nmX = -1.0f, float nmY = -1.0f,
                               float nmW = -1.0f, float nmH = -1.0f) {
            const float nx = nmX > 0.0f ? nmX : vizX;
            const float ny = nmY > 0.0f ? nmY : vizY;
            const float nw = nmW > 0.0f ? nmW : vizW;
            const float nh = nmH > 0.0f ? nmH : vizH;
            if (a <= 0.001f) return;

            // Shop-specific visual language: a horizontal purchase trace.
            // Codex can afford a specimen/map viewer; the shop gets a compact
            // stock-to-upgrade readout that leaves the world visible around it.
            BindMainShader();
            g_TextS.Draw(label, nx + 18.0f * uiS, ny + 16.0f * uiS,
                         0.52f * uiS, 0.78f, 0.83f, 0.96f, 0.62f * a);
            drawRect(nx + 18.0f * uiS, ny + 46.0f * uiS, nw * 0.34f,
                     1.0f * uiS, r, g, b, 0.34f * a);

            int nodes = std::max(3, std::min(total, 8));
            const float lineL = nx + nw * 0.17f;
            const float lineR = nx + nw * 0.83f;
            const float cy = ny + nh * 0.56f;
            const float lineW = std::max(1.0f, lineR - lineL);
            const float baseA = 0.18f * a;
            drawRect(lineL, cy - 1.0f * uiS, lineW, 2.0f * uiS,
                     r, g, b, baseA);
            drawRect(lineL, cy - 4.0f * uiS,
                     lineW * std::min(1.0f, std::max(0.0f, (float)filled / (float)std::max(1, total))),
                     8.0f * uiS, r, g, b, 0.045f * a);

            for (int i = 0; i < nodes; ++i) {
                const float t = nodes <= 1 ? 0.0f : (float)i / (float)(nodes - 1);
                const float x = lineL + lineW * t;
                const bool on = i < filled;
                const bool next = i == filled && filled < total;
                const float pulse = next ? 0.72f + 0.28f * sinf(now * 4.0f) : 1.0f;
                drawRect(x - 0.6f * uiS, cy - 11.0f * uiS,
                         1.2f * uiS, 22.0f * uiS, r, g, b,
                         (on ? 0.54f : 0.16f) * a);
                drawDiamond(x, cy, (on ? 6.0f : next ? 5.5f : 3.8f) * uiS,
                            r, g, b,
                            (on ? 0.86f : next ? 0.72f * pulse : 0.26f) * a);
                if (on)
                    drawDiamond(x, cy, 2.0f * uiS, 1.0f, 1.0f, 1.0f, 0.50f * a);
            }

            wchar_t progressBuf[32];
            swprintf_s(progressBuf, L"%02d / %02d", std::min(filled, total), total);
            float progressSc = 0.58f * uiS;
            g_TextS.Draw(progressBuf, lineR - g_TextS.Width(progressBuf, progressSc),
                         cy + 34.0f * uiS, progressSc,
                         0.92f, 0.96f, 1.0f, 0.90f * a);
        };

        auto drawActionButton = [&](float x, float y, float w, float h, const wchar_t* label,
                                    bool enabled, float r, float g, float b, float a) -> bool {
            bool hov = inputReady && (mx >= x && mx < x + w && my >= y && my < y + h);
            BindMainShader();
            drawRect(x, y, w, h,
                     r * (hov && enabled ? 0.12f : 0.04f),
                     g * (hov && enabled ? 0.10f : 0.035f),
                     b * (hov && enabled ? 0.08f : 0.030f),
                     (hov && enabled ? 0.52f : 0.18f) * a);
            if (hov && enabled) {
                const float slashGap = 18.0f * uiS;
                for (float sx = x - h; sx < x + w + h; sx += slashGap) {
                    LogoLine(sx, y + h - 5.0f * uiS,
                             sx + 22.0f * uiS, y + 5.0f * uiS,
                             1.0f * uiS, r, g, b, 0.18f * a);
                }
                float scanX = x + fmodf(now * 260.0f, w + 80.0f * uiS) - 40.0f * uiS;
                drawRect(scanX, y + 3.0f * uiS, 24.0f * uiS, h - 6.0f * uiS,
                         r, g, b, 0.12f * a);
            }
            drawConstellFrame(x, y, w, h, r, g, b,
                              (enabled ? (hov ? 0.92f : 0.62f) : 0.38f) * a,
                              15.0f * uiS, 3.5f * uiS, 0.04f * a);
            float sc = 0.60f * uiS;
            while (sc > 0.42f * uiS && g_TextS.Width(label, sc) > w - 28.0f * uiS)
                sc -= 0.025f * uiS;
            float tw = g_TextS.Width(label, sc);
            DrawShadowedText(g_TextS, label, x + (w - tw) * 0.5f,
                             y + (h - g_TextS.Height(label, sc)) * 0.5f,
                             sc,
                             enabled ? 0.92f : 0.64f,
                             enabled ? 0.98f : 0.70f,
                             enabled ? 1.00f : 0.78f,
                             0.96f * a, 0.70f);
            return enabled && hov && lmb && !g_LmbPrev;
        };

        // The detail column lives inside the shared ARMORY surface. Retain the
        // scan-on reveal, but do not stack another opaque panel behind it.
        {
            BindMainShader();
            drawRect(rightX, rightAreaY + workHeaderH,
                     rightW, rightAreaH - workHeaderH,
                     0.038f, 0.050f, 0.075f, 0.0f);
            const float scanReveal = Smoothstep(LogoClamp01((g_ShopEntryT - 0.34f) / 0.32f));
            if (scanReveal > 0.004f && scanReveal < 0.998f) {
                const float scanY = rightAreaY + workHeaderH
                    + (rightAreaH - workHeaderH) * scanReveal;
                drawRect(rightX, scanY - 1.5f * uiS, rightW, 3.0f * uiS,
                         sr, sg, sb, 0.48f * (1.0f - scanReveal) * detailWake);
            }
        }

        drawPanelBase();
        drawCredits();

        auto drawMiniTagFrame = [&](float x, float y, float w, float h,
                                    float r, float g, float b, float a) {
            if (a <= 0.001f) return;
            // A transaction readout, rather than a second information card:
            // only a spine and a few registration marks separate it from the
            // scene behind it.
            BindMainShader();
            drawRect(x, y + 8.0f * uiS, 2.0f * uiS, h - 16.0f * uiS,
                     r, g, b, 0.58f * a);
            drawRect(x + 14.0f * uiS, y, w * 0.30f, 1.0f * uiS,
                     r, g, b, 0.26f * a);
            drawRect(x + w * 0.72f, y + h - 1.0f * uiS,
                     w * 0.28f - 14.0f * uiS, 1.0f * uiS,
                     r, g, b, 0.18f * a);
            drawDiamond(x, y + 8.0f * uiS, 3.0f * uiS, r, g, b, 0.72f * a);
            drawDiamond(x, y + h - 8.0f * uiS, 2.4f * uiS, r, g, b, 0.42f * a);
        };

        auto drawDataTag = [&](const wchar_t* id, const wchar_t* title,
                               const wchar_t* desc, const wchar_t* stat,
                               const wchar_t* cmd, bool enabled,
                               float r, float g, float b, float a,
                               float tagX0 = -1.0f, float tagY0 = -1.0f,
                               float tagW0 = -1.0f) -> bool {
            const float tagW = tagW0 > 0.0f ? tagW0 : std::min(760.0f * uiS, rInW - 48.0f * uiS);
            const float tagH = 168.0f * uiS;
            const float tagX = tagX0 > 0.0f ? tagX0 : rInX + 24.0f * uiS;
            const float tagY = tagY0 > 0.0f ? tagY0 : rInY + 104.0f * uiS;
            const float flick = Smoothstep(s_tagFlickerT);
            const float tagA = a * (0.58f + 0.42f * flick);
            const float sc0 = 0.60f * uiS;
            const float sc1 = 0.78f * uiS;
            bool cmdHov = inputReady && enabled
                && mx >= tagX && mx < tagX + tagW
                && my >= tagY + 126.0f * uiS && my < tagY + tagH + 8.0f * uiS;

            drawMiniTagFrame(tagX, tagY, tagW, tagH, r, g, b, tagA);
            BindMainShader();
            if (s_tagFlickerT < 0.98f) {
                const float scanY = tagY + 10.0f * uiS + (tagH - 22.0f * uiS) * flick;
                drawRect(tagX + 8.0f * uiS, scanY, tagW - 18.0f * uiS, 1.0f * uiS,
                         r, g, b, 0.34f * a * (1.0f - flick));
            }
            drawDiamond(tagX + 10.0f * uiS, tagY + 17.0f * uiS,
                        2.8f * uiS, r, g, b, 0.70f * tagA);
            DrawShadowedText(g_TextS, id, tagX + 24.0f * uiS, tagY + 8.0f * uiS,
                             sc0, 0.62f, 0.72f, 0.86f, 0.72f * tagA, 0.58f);
            DrawShadowedText(g_TextS, title, tagX + 24.0f * uiS, tagY + 35.0f * uiS,
                             sc1, 0.92f, 0.96f, 1.0f, 0.94f * tagA, 0.72f);
            if (desc && desc[0]) {
                DrawShadowedText(g_TextS, desc, tagX + 24.0f * uiS, tagY + 70.0f * uiS,
                                   0.60f * uiS, 0.66f, 0.75f, 0.88f, 0.78f * tagA, 0.58f);
            }
            DrawShadowedText(g_TextS, stat, tagX + 24.0f * uiS, tagY + 103.0f * uiS,
                               0.64f * uiS, 0.86f, 0.91f, 0.98f, 0.90f * tagA, 0.66f);
            if (cmd) {
                if (cmdHov) {
                    drawRect(tagX + 22.0f * uiS, tagY + 136.0f * uiS,
                               std::min(tagW - 44.0f * uiS, g_TextS.Width(cmd, 0.60f * uiS) + 24.0f * uiS),
                             1.2f * uiS, r, g, b, 0.54f * tagA);
                }
                DrawShadowedText(g_TextS, cmd, tagX + 24.0f * uiS, tagY + 137.0f * uiS,
                              0.60f * uiS,
                                 enabled ? (cmdHov ? 1.0f : 0.86f) : 0.48f,
                                 enabled ? (cmdHov ? 1.0f : 0.92f) : 0.54f,
                                 enabled ? 1.0f : 0.60f,
                                 (enabled ? 0.92f : 0.48f) * tagA, 0.68f);
            }
            return cmdHov && lmb && !g_LmbPrev;
        };

        {
            int filled = 0, total = 5;
            const wchar_t* mapLabel = nli == 0 ? L"\uD574\uAE08 \uC9C4\uD589" : L"UPGRADE TRACE";
            const wchar_t* id = L"ID: EMPTY";
            const wchar_t* title = L"UNASSIGNED NODE";
            const wchar_t* desc = L"DESC: SELECT TREE BRANCH";
            wchar_t statBuf[128]; swprintf_s(statBuf, L"STAT: [ - ]");
            const wchar_t* cmd = nullptr;
            bool cmdEnabled = false;

            // The map owns the upper half; keep the data tag readable without
            // pinning it to the extreme bottom-right corner.
            const float nmColX = vizX;
            const float nmColY = vizY;
            const float nmColW = vizW;
            const float nmColH = vizH;
            const float tagW0 = rInW - 16.0f * uiS;
            const float tagH0 = 168.0f * uiS;
            const float tagX0 = rInX + 4.0f * uiS;
            const float tagY0 = tagDockY;

            if (s_selKey >= 0 && s_selKey < KEY_THEME) {
                int mi = s_selKey - KEY_META;
                if (mi >= 0 && mi < META_COUNT) {
                    const MetaDef& md = META_DEFS[mi];
                    int curLv = g_MetaLv[mi];
                    long long cost = MetaNextCost(mi);
                    bool maxed = curLv >= md.maxLv;
                    filled = curLv; total = md.maxLv;
                    id = metaNodeCode(mi);
                    title = MetaName(mi);
                    desc = nli == 0 ? L"\uC601\uAD6C \uD574\uAE08 \uB178\uB4DC" : L"PERMANENT UNLOCK NODE";
                    if (maxed)
                        swprintf_s(statBuf, L"LV %d/%d  |  COST MAX", curLv, md.maxLv);
                    else
                        swprintf_s(statBuf, L"LV %d/%d  |  COST %lld SD", curLv, md.maxLv, cost);
                    cmd = maxed ? L"CMD : [ MAXED ]" : L"CMD : [> INITIATE_UNLOCK ]";
                    cmdEnabled = !maxed && g_Coins >= cost;
                    if (drawDataTag(id, title, desc, statBuf, cmd, cmdEnabled,
                                    sr, sg, sb, da, tagX0, tagY0, tagW0)) {
                        g_Coins -= cost;
                        g_MetaLv[mi]++;
                        SaveGame();
                    }
                    drawNodeMap(filled, total, mapLabel, sr, sg, sb, da,
                                nmColX, rInY, nmColW, nmColH);
                }
            } else if (s_selKey >= KEY_THEME && s_selKey < KEY_AUG) {
                int ti = s_selKey - KEY_THEME;
                if (ti >= 0 && ti < ACCENT_COUNT) {
                    const AccentTheme& th = ACCENT_THEMES[ti];
                    bool owned = ThemeOwned(ti);
                    bool isCur = (g_ThemeSel == ti);
                    filled = owned ? 5 : 2; total = 5;
                    id = L"CHROMA_NODE : PALETTE";
                    title = AccentName(ti);
                    desc = L"VISUAL ACCENT PALETTE";
                    swprintf_s(statBuf, L"%ls  |  COST %lld SD",
                               owned ? L"OWNED" : L"LOCKED", th.cost);
                    cmd = isCur ? L"CMD : [ EQUIPPED ]"
                        : owned ? L"CMD : [> EQUIP_SET ]" : L"CMD : [> PURCHASE_INITIATE ]";
                    cmdEnabled = owned || g_Coins >= th.cost;
                    if (drawDataTag(id, title, desc, statBuf, cmd, cmdEnabled,
                                    th.r, th.g, th.b, da, tagX0, tagY0, tagW0)) {
                        if (owned && !isCur) {
                            g_ThemeSel = ti;
                            ApplyAccentTheme();
                            SaveGame();
                        } else if (!owned && g_Coins >= th.cost) {
                            g_Coins -= th.cost;
                            g_ThemeOwned |= (1 << ti);
                            g_ThemeSel = ti;
                            ApplyAccentTheme();
                            SaveGame();
                        }
                    }
                    sr = th.r; sg = th.g; sb = th.b;
                    mapLabel = nli == 0 ? L"\uC0C9\uC0C1 \uD504\uB85C\uD30C\uC77C" : L"COLOR PROFILE";
                    drawNodeMap(filled, total, mapLabel, sr, sg, sb, da,
                                nmColX, rInY, nmColW, nmColH);
                }
            } else if (s_selKey >= KEY_AUG) {
                int ai = s_selKey - KEY_AUG;
                if (ai >= 0 && ai < AUG_TOTAL) {
                    const AugDef& ad = ALL_AUGS[ai];
                    GetRarityColor(ad.rarity, sr, sg, sb);
                    filled = 4; total = 6;
                    id = L"STAR NODE : MODULE CATALOG";
                    title = ad.locName[li];
                    desc = AugDesc(ad);
                    if (!desc || !desc[0]) desc = L"IN-RUN MODULE";
                    swprintf_s(statBuf, L"RUN MODULE  |  ARCHIVE READ ONLY");
                    cmd = L"CMD : [ CHARTED_ONLY ]";
                    cmdEnabled = false;
                    drawDataTag(id, title, desc, statBuf, cmd, cmdEnabled,
                                sr, sg, sb, da, tagX0, tagY0, tagW0);
                    mapLabel = nli == 0 ? L"\uB7F0 \uC804\uC6A9 \uBAA8\uB4C8" : L"RUN-ONLY MODULE";
                    drawNodeMap(filled, total, mapLabel, sr, sg, sb, da,
                                nmColX, rInY, nmColW, nmColH);
                }
            }
        }

        if (false) {
        if (s_selKey < 0) {
            float a = rightWake;
            drawProductHeader(L"STAR NODE : EMPTY", L10n(L"\uBE48 \uBCC4\uC790\uB9AC \uC9C0\uB3C4", L"EMPTY STAR FIELD"),
                              sr, sg, sb, 0.92f * a);
            g_TextS.Draw(L10n(L"\uB2E4\uB978 \uADA4\uB3C4\uB97C \uC120\uD0DD", L"SELECT ANOTHER ORBIT"), rInX, bodyY + 28.0f * uiS,
                         0.58f * uiS, 0.64f, 0.74f, 0.88f, 0.72f * a);
            drawNodeMap(0, 5, L10n(L"\uBE48 \uD558\uB298", L"EMPTY SKY"), sr, sg, sb, 0.75f * a);
        } else if (s_selKey < KEY_THEME) {
            int mi = s_selKey - KEY_META;
            if (mi >= 0 && mi < META_COUNT) {
                const MetaDef& md = META_DEFS[mi];
                int curLv = g_MetaLv[mi];
                long long cost = MetaNextCost(mi);
                float prog = (md.maxLv > 0) ? (float)curLv / (float)md.maxLv : 1.0f;
                bool maxed = curLv >= md.maxLv;
                bool canBuy = !maxed && g_Coins >= cost;

                drawProductHeader(metaNodeCode(mi), MetaName(mi), sr, sg, sb, da);

                wchar_t lvBuf[32]; swprintf_s(lvBuf, L"Lv %d / %d", curLv, md.maxLv);
                wchar_t costBuf[48];
                if (maxed) swprintf_s(costBuf, L"%ls", L10n(L"\uCD5C\uB300", L"MAX"));
                else swprintf_s(costBuf, L"%lld SD", cost);
                const float gridX = infoX + 18.0f * uiS;
                const float gridW = infoW - 36.0f * uiS;
                drawTerminalRow(gridX, bodyY + 2.0f * uiS, gridW,
                                L"[ CAPACITY ]", lvBuf, sr, sg, sb, da);
                drawTerminalRow(gridX, bodyY + 42.0f * uiS, gridW,
                                L"[ COST     ]", costBuf, sr, sg, sb, da, true);
                drawTerminalRow(gridX, bodyY + 82.0f * uiS, gridW,
                                L"[ STATUS   ]",
                                maxed ? L"COMPLETE" : L"READY", sr, sg, sb, da);

                float barX = infoX + 18.0f * uiS;
                float barY = bodyY + 132.0f * uiS;
                float barW = infoW - 36.0f * uiS;
                float barH = 12.0f * uiS;
                BindMainShader();
                drawRect(barX, barY, barW, barH, 0.10f, 0.14f, 0.20f, 0.96f * da);
                drawRect(barX, barY, barW * prog, barH, sr, sg, sb, 0.82f * da);
                drawDiamond(barX + barW * prog, barY + barH * 0.5f,
                            4.5f * uiS, sr, sg, sb, 0.90f * da);

                float btnX = infoX + 18.0f * uiS;
                float btnY = bodyY + 178.0f * uiS;
                float btnW = std::min(430.0f * uiS, infoW - 36.0f * uiS);
                float btnH = 62.0f * uiS;
                if (drawActionButton(btnX, btnY, btnW, btnH,
                                     maxed ? L"[ MAXED ]" : L10n(L"[> \uD574\uAE08 \uC2E4\uD589 ]", L"[> UNLOCK_INITIATE ]"),
                                     canBuy || maxed, sr, sg, sb, da) && canBuy) {
                    g_Coins -= cost;
                    g_MetaLv[mi]++;
                    SaveGame();
                }

                wchar_t hintBuf[96];
                swprintf_s(hintBuf, nli == 0 ? L"\uAE30\uC900 %lld  /  \uBCC4\uAC00\uB8E8 x1.6" : L"BASE %lld  /  STARDUST x1.6", md.baseCost);
                g_TextS.Draw(hintBuf, btnX, btnY + btnH + 18.0f * uiS,
                             0.46f * uiS, 0.66f, 0.76f, 0.90f, 0.80f * da);
                drawNodeMap(curLv, md.maxLv, L10n(L"\uC131\uB3C4", L"CONSTELLATION MAP"), sr, sg, sb, da);
            }
        } else if (s_selKey < KEY_AUG) {
            int ti = s_selKey - KEY_THEME;
            if (ti >= 0 && ti < ACCENT_COUNT) {
                const AccentTheme& th = ACCENT_THEMES[ti];
                bool owned = ThemeOwned(ti);
                bool isCur = (g_ThemeSel == ti);
                bool canBuy = !owned && g_Coins >= th.cost;

                drawProductHeader(L"CHROMA_NODE : PALETTE", AccentName(ti), th.r, th.g, th.b, da);

                wchar_t costBuf[48]; swprintf_s(costBuf, L"%lld SD", th.cost);
                const float gridX = infoX + 18.0f * uiS;
                const float gridW = infoW - 36.0f * uiS;
                drawTerminalRow(gridX, bodyY + 2.0f * uiS, gridW,
                                L"[ STATUS   ]", owned ? L"OWNED" : L"LOCKED", th.r, th.g, th.b, da);
                drawTerminalRow(gridX, bodyY + 42.0f * uiS, gridW,
                                L"[ COST     ]", owned ? L"-" : costBuf, th.r, th.g, th.b, da, true);
                drawTerminalRow(gridX, bodyY + 82.0f * uiS, gridW,
                                L"[ EQUIP    ]", isCur ? L"ACTIVE" : L"READY", th.r, th.g, th.b, da);

                const wchar_t* btnLabel = isCur ? L"[ EQUIPPED ]"
                    : owned ? L"[> EQUIP_SET ]" : L10n(L"[> \uAD6C\uC785 \uC2E4\uD589 ]", L"[> PURCHASE_INITIATE ]");
                if (drawActionButton(infoX + 18.0f * uiS, bodyY + 178.0f * uiS,
                                     std::min(430.0f * uiS, infoW - 36.0f * uiS),
                                     62.0f * uiS, btnLabel,
                                     owned || canBuy, th.r, th.g, th.b, da)) {
                    if (owned && !isCur) {
                        g_ThemeSel = ti;
                        ApplyAccentTheme();
                        SaveGame();
                    } else if (!owned && canBuy) {
                        g_Coins -= th.cost;
                        g_ThemeOwned |= (1 << ti);
                        g_ThemeSel = ti;
                        ApplyAccentTheme();
                        SaveGame();
                    }
                }
                drawNodeMap(owned ? 5 : 2, 5, L10n(L"\uC131\uC6B4 \uD314\uB808\uD2B8", L"ASTRAL PALETTE"), th.r, th.g, th.b, da);
            }
        } else {
            int ai = s_selKey - KEY_AUG;
            if (ai >= 0 && ai < AUG_TOTAL) {
                const AugDef& ad = ALL_AUGS[ai];
                float cr2, cg2, cb2;
                GetRarityColor(ad.rarity, cr2, cg2, cb2);

                const wchar_t* augName = ad.locName[li];
                drawProductHeader(L"STAR NODE : MODULE CATALOG", augName, cr2, cg2, cb2, da);

                const float gridX = infoX + 18.0f * uiS;
                const float gridW = infoW - 36.0f * uiS;
                drawTerminalRow(gridX, bodyY + 2.0f * uiS, gridW,
                                L"[ TYPE     ]", L"MODULE", cr2, cg2, cb2, da);
                drawTerminalRow(gridX, bodyY + 42.0f * uiS, gridW,
                                L"[ SOURCE   ]", L"RUN ORBIT", cr2, cg2, cb2, da);
                drawTerminalRow(gridX, bodyY + 82.0f * uiS, gridW,
                                L"[ STATUS   ]", L"MAPPED", cr2, cg2, cb2, da);

                const wchar_t* desc = AugDesc(ad);
                if (desc && desc[0]) {
                    std::wstring ds(desc);
                    size_t pos = 0;
                    float descX = infoX + 18.0f * uiS;
                    float descY = bodyY + 132.0f * uiS;
                    float descW = infoW - 36.0f * uiS;
                    float descH = 178.0f * uiS;
                    float dY = descY + 22.0f * uiS;
                    int ln = 0;
                    BindMainShader();
                    drawRect(descX, descY, descW, descH,
                             0.028f, 0.038f, 0.058f, 0.92f * da);
                    drawRect(descX, descY, descW, descH,
                             cr2, cg2, cb2, 0.040f * da);
                    drawConstellFrame(descX, descY, descW, descH,
                                      cr2, cg2, cb2, 0.22f * da,
                                      14.0f * uiS, 3.0f * uiS, 0.0f, 1.0f);
                    drawOrbitCornerArcs(descX, descY, descW, descH,
                                        cr2, cg2, cb2, 0.16f * da);
                    while (pos < ds.size() && ln < 6) {
                        size_t sl = ds.find(L" / ", pos);
                        std::wstring seg = (sl == std::wstring::npos)
                            ? ds.substr(pos) : ds.substr(pos, sl - pos);
                        pos = (sl == std::wstring::npos) ? ds.size() : sl + 3;
                        while (!seg.empty() && seg.front() == L' ') seg.erase(0, 1);
                        while (!seg.empty() && seg.back() == L' ') seg.pop_back();
                        float lineY = dY + ln * 26.0f * uiS;
                        BindMainShader();
                        drawDiamond(descX + 16.0f * uiS, lineY + 9.0f * uiS,
                                    2.8f * uiS, cr2, cg2, cb2, 0.50f * da);
                        g_TextS.Draw(seg.c_str(), descX + 30.0f * uiS,
                                     lineY,
                                     0.50f * uiS, 0.84f, 0.90f, 0.98f, 0.88f * da);
                        ++ln;
                    }
                }

                drawActionButton(infoX + 18.0f * uiS, bodyY + 300.0f * uiS,
                                 std::min(430.0f * uiS, infoW - 36.0f * uiS),
                                 58.0f * uiS, L"[ CHARTED_ONLY ]",
                                 false, cr2, cg2, cb2, da);
            }
        }
        }
    }

    if (false)
    {
        float da = s_detailT * rightWake;
        float sr = 0.38f, sg = 0.82f, sb = 1.00f;
        if (s_selKey >= KEY_THEME && s_selKey < KEY_AUG) {
            int ti = s_selKey - KEY_THEME;
            if (ti >= 0 && ti < ACCENT_COUNT) {
                sr = ACCENT_THEMES[ti].r; sg = ACCENT_THEMES[ti].g; sb = ACCENT_THEMES[ti].b;
            }
        } else if (s_selKey >= KEY_AUG) {
            int ai = s_selKey - KEY_AUG;
            if (ai >= 0 && ai < AUG_TOTAL)
                GetRarityColor(ALL_AUGS[ai].rarity, sr, sg, sb);
        }

        const float rInW = rightW - rPad * 2.0f;
        const float topH = 56.0f * uiS;
        const float contentY = rInY + topH + 18.0f * uiS;
        const float vizW = std::min(430.0f * uiS, rInW * 0.38f);
        const float vizH = std::min(410.0f * uiS, rInH - topH - 68.0f * uiS);
        const float vizX = rInX + rInW - vizW;
        const float vizY = contentY + 8.0f * uiS;
        const float infoX = rInX;
        const float infoW = rInW - vizW - 34.0f * uiS;

        auto drawMiniStat = [&](float x, float y, float w, const wchar_t* label,
                                const wchar_t* value, float r, float g, float b, float a) {
            const float h = 54.0f * uiS;
            BindMainShader();
            drawRect(x, y, w, h, 0.018f, 0.024f, 0.038f, 0.92f * a);
            drawRect(x, y, 2.5f * uiS, h, r, g, b, 0.70f * a);
            drawConstellFrame(x, y, w, h, r, g, b, 0.22f * a, 9.0f * uiS, 2.0f * uiS);
            g_TextS.Draw(label, x + 12.0f * uiS, y + 7.0f * uiS, 0.34f * uiS,
                         r, g, b, 0.66f * a);
            g_TextS.Draw(value, x + 12.0f * uiS, y + 27.0f * uiS, 0.46f * uiS,
                         0.92f, 0.96f, 1.0f, 0.95f * a);
        };

        auto drawVizBase = [&](float x, float y, float w, float h,
                               const wchar_t* title, float r, float g, float b, float a) {
            BindMainShader();
            drawRect(x + 8.0f * uiS, y + 10.0f * uiS, w, h,
                     0.0f, 0.0f, 0.0f, 0.28f * a);
            drawRect(x, y, w, h, 0.006f, 0.012f, 0.024f, 0.96f * a);
            drawRect(x + 14.0f * uiS, y + 46.0f * uiS, w - 28.0f * uiS, 1.2f * uiS,
                     r, g, b, 0.22f * a);
            BatchFlush();
            SetGlowFx(true);
            drawConstellFrame(x, y, w, h, r, g, b, 0.50f * a,
                              18.0f * uiS, 4.0f * uiS, 0.07f * a);
            SetGlowFx(false);
            g_TextS.Draw(title, x + 18.0f * uiS, y + 16.0f * uiS,
                         0.42f * uiS, r, g, b, 0.82f * a);
        };

        auto drawLevelConstellation = [&](int curLv, int maxLv, float r, float g, float b, float a) {
            drawVizBase(vizX, vizY, vizW, vizH, L"UNLOCK MAP", r, g, b, a);
            BindMainShader();
            int nodes = std::max(3, maxLv);
            float cx = vizX + vizW * 0.5f;
            float cy = vizY + vizH * 0.54f;
            float rad = std::min(vizW, vizH) * 0.29f;
            float px[8] = {}, py[8] = {};
            for (int i = 0; i < nodes && i < 8; ++i) {
                float ang = -1.5708f + (float)i / (float)nodes * 6.2831853f + now * 0.12f;
                px[i] = cx + cosf(ang) * rad;
                py[i] = cy + sinf(ang) * rad;
            }
            for (int i = 0; i < nodes && i < 8; ++i) {
                int j = (i + 1) % nodes;
                LogoLine(px[i], py[i], px[j], py[j], 1.0f * uiS,
                         r, g, b, (i < curLv || j < curLv ? 0.42f : 0.13f) * a);
                LogoLine(cx, cy, px[i], py[i], 0.8f * uiS,
                         r, g, b, (i < curLv ? 0.22f : 0.07f) * a);
            }
            drawDiamond(cx, cy, 12.0f * uiS, r, g, b, 0.78f * a);
            drawDiamond(cx, cy, 5.0f * uiS, 0.02f, 0.04f, 0.08f, 0.92f * a);
            for (int i = 0; i < nodes && i < 8; ++i) {
                bool filled = i < curLv;
                bool next = i == curLv && curLv < maxLv;
                float pulse = next ? (0.68f + 0.32f * sinf(now * 3.8f)) : 1.0f;
                drawDiamond(px[i], py[i], (filled ? 9.0f : 6.0f) * uiS,
                            r, g, b, (filled ? 0.86f : next ? 0.56f * pulse : 0.22f) * a);
                drawDiamond(px[i], py[i], (filled ? 3.4f : 2.4f) * uiS,
                            1.0f, 1.0f, 1.0f, (filled ? 0.42f : 0.14f) * a);
            }
            wchar_t lvText[32]; swprintf_s(lvText, L"%d / %d", curLv, maxLv);
            float lvSc = 0.70f * uiS;
            g_TextS.Draw(lvText, cx - g_TextS.Width(lvText, lvSc) * 0.5f,
                         vizY + vizH - 54.0f * uiS, lvSc,
                         0.92f, 0.96f, 1.0f, 0.90f * a);
        };

        auto drawAugConstellation = [&](const wchar_t* title, float r, float g, float b, float a) {
            drawVizBase(vizX, vizY, vizW, vizH, title, r, g, b, a);
            BindMainShader();
            float cx = vizX + vizW * 0.5f;
            float cy = vizY + vizH * 0.55f;
            float inner = std::min(vizW, vizH) * 0.18f;
            float outer = std::min(vizW, vizH) * 0.31f;
            float pulse = 0.72f + 0.28f * sinf(now * 2.0f);
            for (int i = 0; i < 6; ++i) {
                float a0 = now * 0.36f + (float)i * 1.0472f;
                float a1 = now * 0.36f + (float)((i + 1) % 6) * 1.0472f;
                float x0 = cx + cosf(a0) * outer, y0 = cy + sinf(a0) * outer;
                float x1 = cx + cosf(a1) * outer, y1 = cy + sinf(a1) * outer;
                LogoLine(x0, y0, x1, y1, 0.9f * uiS, r, g, b, 0.20f * a);
                if ((i % 2) == 0)
                    LogoLine(cx, cy, x0, y0, 0.8f * uiS, r, g, b, 0.13f * a);
                drawDiamond(x0, y0, 4.8f * uiS, r, g, b, 0.48f * a);
            }
            for (int i = 0; i < 4; ++i) {
                float a0 = -now * 0.52f + (float)i * 1.5708f + 0.785f;
                drawDiamond(cx + cosf(a0) * inner, cy + sinf(a0) * inner,
                            6.5f * uiS, r, g, b, 0.62f * pulse * a);
            }
            drawDiamond(cx, cy, 14.0f * uiS, r, g, b, 0.88f * pulse * a);
            drawDiamond(cx, cy, 5.5f * uiS, 0.02f, 0.04f, 0.08f, 0.94f * a);
        };

        BindMainShader();
        drawRect(rightX + 9.0f * uiS, listAreaY + 11.0f * uiS, rightW, listAreaH,
                 0.0f, 0.0f, 0.0f, 0.0f);
        drawRect(rightX, listAreaY, rightW, listAreaH,
                 0.006f + sr * 0.006f, 0.010f + sg * 0.005f, 0.020f + sb * 0.005f,
                 0.0f);
        drawRect(rightX + rPad, listAreaY + topH + 2.0f * uiS, rightW - rPad * 2.0f,
                 1.4f * uiS, sr, sg, sb, 0.22f * rightWake);
        BatchFlush();
        SetGlowFx(true);
        drawConstellFrame(rightX, listAreaY, rightW, listAreaH,
                          sr, sg, sb, 0.50f * rightWake,
                          28.0f * uiS, 5.0f * uiS, 0.08f * rightWake);
        SetGlowFx(false);

        wchar_t creditBuf[64]; swprintf_s(creditBuf, L"CREDITS :: %06lld", g_Coins);
        float creditSc = 0.48f * uiS;
        float creditH = 42.0f * uiS;
        float creditW = g_TextS.Width(creditBuf, creditSc) + 52.0f * uiS;
        float creditX = rightX + rightW - rPad - creditW;
        float creditY = rInY;
        BindMainShader();
        drawRect(creditX, creditY, creditW, creditH,
                 0.038f, 0.036f, 0.020f, 0.94f * rightWake);
        drawRect(creditX, creditY, creditW, creditH,
                 1.0f, 0.84f, 0.22f, 0.070f * rightWake);
        BatchFlush();
        SetGlowFx(true);
        drawConstellFrame(creditX, creditY, creditW, creditH,
                          1.0f, 0.84f, 0.22f, 0.60f * rightWake,
                          10.0f * uiS, 2.6f * uiS, 0.04f * rightWake);
        SetGlowFx(false);
        BindMainShader();
        drawDiamond(creditX + 18.0f * uiS, creditY + creditH * 0.5f,
                    4.2f * uiS, 1.0f, 0.84f, 0.22f, 0.86f * rightWake);
        g_TextS.Draw(creditBuf, creditX + 32.0f * uiS,
                     creditY + (creditH - g_TextS.Height(creditBuf, creditSc)) * 0.5f,
                     creditSc, 1.0f, 0.88f, 0.34f, 0.96f * rightWake);

        if (s_selKey >= 0 && rightWake > 0.02f) {
            if (s_selKey < KEY_THEME) {
                int mi = s_selKey - KEY_META;
                if (mi >= 0 && mi < META_COUNT) {
                    const MetaDef& md = META_DEFS[mi];
                    int curLv = g_MetaLv[mi];
                    long long cost = MetaNextCost(mi);
                    float prog = (md.maxLv > 0) ? (float)curLv / (float)md.maxLv : 1.0f;
                    drawLevelConstellation(curLv, md.maxLv, sr, sg, sb, da);

                    g_TextS.Draw(L"UNLOCK NODE", infoX, contentY, 0.46f * uiS,
                                 sr, sg, sb, 0.82f * da);
                    g_TextL.Draw(MetaName(mi), infoX, contentY + 28.0f * uiS,
                                 1.02f * uiS, 1.0f, 1.0f, 1.0f, 0.96f * da);

                    wchar_t lvBuf[32]; swprintf_s(lvBuf, L"Lv %d / %d", curLv, md.maxLv);
                    wchar_t costBuf[48];
                    if (curLv >= md.maxLv) swprintf_s(costBuf, L"MAX");
                    else swprintf_s(costBuf, L"%lld C", cost);
                    drawMiniStat(infoX, contentY + 98.0f * uiS, infoW * 0.31f,
                                 L"LEVEL", lvBuf, sr, sg, sb, da);
                    drawMiniStat(infoX + infoW * 0.34f, contentY + 98.0f * uiS, infoW * 0.31f,
                                 L"COST", costBuf, sr, sg, sb, da);
                    drawMiniStat(infoX + infoW * 0.68f, contentY + 98.0f * uiS, infoW * 0.32f,
                                 L"STATE", curLv >= md.maxLv ? L"COMPLETE" : L"AVAILABLE", sr, sg, sb, da);

                    float barX = infoX;
                    float barY = contentY + 176.0f * uiS;
                    float barW = infoW;
                    float barH = 10.0f * uiS;
                    BindMainShader();
                    drawRect(barX, barY, barW, barH, 0.09f, 0.12f, 0.18f, 0.92f * da);
                    drawRect(barX, barY, barW * prog, barH, sr, sg, sb, 0.78f * da);
                    drawDiamond(barX + barW * prog, barY + barH * 0.5f,
                                4.2f * uiS, sr, sg, sb, 0.80f * da);

                    float btnX = infoX;
                    float btnY = contentY + 220.0f * uiS;
                    float btnW = std::min(380.0f * uiS, infoW * 0.68f);
                    float btnH = 58.0f * uiS;
                    bool maxed = curLv >= md.maxLv;
                    bool canBuy = !maxed && g_Coins >= cost;
                    bool bHov = (mx >= btnX && mx < btnX + btnW && my >= btnY && my < btnY + btnH);
                    BindMainShader();
                    drawRect(btnX, btnY, btnW, btnH,
                             canBuy ? (bHov ? 0.075f : 0.045f) : 0.030f,
                             canBuy ? (bHov ? 0.130f : 0.085f) : 0.034f,
                             canBuy ? (bHov ? 0.190f : 0.135f) : 0.045f,
                             0.96f * da);
                    BatchFlush();
                    SetGlowFx(true);
                    drawConstellFrame(btnX, btnY, btnW, btnH,
                                      sr, sg, sb, (canBuy ? (bHov ? 0.96f : 0.62f) : 0.28f) * da,
                                      14.0f * uiS, 3.4f * uiS, (canBuy ? 0.07f : 0.0f) * da);
                    SetGlowFx(false);
                    const wchar_t* btnLabel = maxed ? L"MAXED" : L"UNLOCK";
                    float bSc = 0.58f * uiS;
                    float bTw = g_TextS.Width(btnLabel, bSc);
                    g_TextS.Draw(btnLabel, btnX + (btnW - bTw) * 0.5f,
                                 btnY + (btnH - g_TextS.Height(btnLabel, bSc)) * 0.5f,
                                 bSc, canBuy || maxed ? 0.92f : 0.42f,
                                 canBuy || maxed ? 0.98f : 0.46f,
                                 canBuy || maxed ? 1.0f : 0.52f, 0.94f * da);
                    if (bHov && lmb && !g_LmbPrev && canBuy) {
                        g_Coins -= cost;
                        g_MetaLv[mi]++;
                        SaveGame();
                    }

                    wchar_t hintBuf[96];
                    swprintf_s(hintBuf, L"base %lld  /  cost grows x1.6 per level", md.baseCost);
                    g_TextS.Draw(hintBuf, infoX, btnY + btnH + 16.0f * uiS,
                                 0.42f * uiS, 0.58f, 0.66f, 0.78f, 0.72f * da);
                }
            } else if (s_selKey < KEY_AUG) {
                int ti = s_selKey - KEY_THEME;
                if (ti >= 0 && ti < ACCENT_COUNT) {
                    const AccentTheme& th = ACCENT_THEMES[ti];
                    bool owned = ThemeOwned(ti);
                    bool isCur = (g_ThemeSel == ti);
                    drawAugConstellation(L"THEME ARRAY", th.r, th.g, th.b, da);

                    g_TextS.Draw(L"COSMETIC NODE", infoX, contentY, 0.46f * uiS,
                                 th.r, th.g, th.b, 0.82f * da);
                    g_TextL.Draw(AccentName(ti), infoX, contentY + 28.0f * uiS,
                                 1.02f * uiS, th.r, th.g, th.b, 0.96f * da);

                    wchar_t costBuf[48]; swprintf_s(costBuf, L"%lld C", th.cost);
                    drawMiniStat(infoX, contentY + 98.0f * uiS, infoW * 0.31f,
                                 L"STATE", owned ? L"OWNED" : L"LOCKED", th.r, th.g, th.b, da);
                    drawMiniStat(infoX + infoW * 0.34f, contentY + 98.0f * uiS, infoW * 0.31f,
                                 L"COST", owned ? L"-" : costBuf, th.r, th.g, th.b, da);
                    drawMiniStat(infoX + infoW * 0.68f, contentY + 98.0f * uiS, infoW * 0.32f,
                                 L"EQUIP", isCur ? L"ACTIVE" : L"READY", th.r, th.g, th.b, da);

                    float btnX = infoX;
                    float btnY = contentY + 220.0f * uiS;
                    float btnW = std::min(380.0f * uiS, infoW * 0.68f);
                    float btnH = 58.0f * uiS;
                    bool canBuy = !owned && g_Coins >= th.cost;
                    bool bHov = (mx >= btnX && mx < btnX + btnW && my >= btnY && my < btnY + btnH);
                    const wchar_t* btnLabel = isCur ? L"EQUIPPED" : owned ? L"EQUIP" : L"BUY";
                    BindMainShader();
                    drawRect(btnX, btnY, btnW, btnH,
                             th.r * (bHov ? 0.13f : 0.08f),
                             th.g * (bHov ? 0.11f : 0.07f),
                             th.b * (bHov ? 0.09f : 0.06f), 0.94f * da);
                    drawConstellFrame(btnX, btnY, btnW, btnH, th.r, th.g, th.b,
                                      (bHov ? 0.82f : 0.52f) * da, 14.0f * uiS, 3.4f * uiS);
                    float bSc = 0.58f * uiS;
                    float bTw = g_TextS.Width(btnLabel, bSc);
                    g_TextS.Draw(btnLabel, btnX + (btnW - bTw) * 0.5f,
                                 btnY + (btnH - g_TextS.Height(btnLabel, bSc)) * 0.5f,
                                 bSc, th.r, th.g, th.b, (!owned && !canBuy ? 0.42f : 0.96f) * da);
                    if (bHov && lmb && !g_LmbPrev) {
                        if (owned && !isCur) {
                            g_ThemeSel = ti;
                            ApplyAccentTheme();
                            SaveGame();
                        } else if (!owned && canBuy) {
                            g_Coins -= th.cost;
                            g_ThemeOwned |= (1 << ti);
                            g_ThemeSel = ti;
                            ApplyAccentTheme();
                            SaveGame();
                        }
                    }
                }
            } else {
                int ai = s_selKey - KEY_AUG;
                if (ai >= 0 && ai < AUG_TOTAL) {
                    const AugDef& ad = ALL_AUGS[ai];
                    float cr2, cg2, cb2;
                    GetRarityColor(ad.rarity, cr2, cg2, cb2);
                    drawAugConstellation(L"MODULE TRACE", cr2, cg2, cb2, da);

                    g_TextS.Draw(GetAugBadge(ad), infoX, contentY, 0.46f * uiS,
                                 cr2, cg2, cb2, 0.86f * da);
                    const wchar_t* augName = ad.locName[li];
                    g_TextL.Draw(augName, infoX, contentY + 28.0f * uiS,
                                 1.02f * uiS, 1.0f, 1.0f, 1.0f, 0.96f * da);

                    drawMiniStat(infoX, contentY + 98.0f * uiS, infoW * 0.31f,
                                 L"TYPE", L"AUGMENT", cr2, cg2, cb2, da);
                    drawMiniStat(infoX + infoW * 0.34f, contentY + 98.0f * uiS, infoW * 0.31f,
                                 L"SOURCE", L"IN RUN", cr2, cg2, cb2, da);
                    drawMiniStat(infoX + infoW * 0.68f, contentY + 98.0f * uiS, infoW * 0.32f,
                                 L"STATE", L"CATALOG", cr2, cg2, cb2, da);

                    const wchar_t* desc = AugDesc(ad);
                    if (desc && desc[0]) {
                        std::wstring ds(desc);
                        size_t pos = 0;
                        float dY = contentY + 184.0f * uiS;
                        float dSc = 0.48f * uiS;
                        int ln = 0;
                        while (pos < ds.size() && ln < 5) {
                            size_t sl = ds.find(L" / ", pos);
                            std::wstring seg = (sl == std::wstring::npos)
                                ? ds.substr(pos) : ds.substr(pos, sl - pos);
                            pos = (sl == std::wstring::npos) ? ds.size() : sl + 3;
                            while (!seg.empty() && seg.front() == L' ') seg.erase(0, 1);
                            while (!seg.empty() && seg.back() == L' ') seg.pop_back();
                            g_TextS.Draw(seg.c_str(), infoX, dY + ln * 27.0f * uiS,
                                         dSc, 0.82f, 0.88f, 0.96f, 0.82f * da);
                            ++ln;
                        }
                    }

                    float btnX = infoX;
                    float btnY = contentY + 344.0f * uiS;
                    float btnW = std::min(380.0f * uiS, infoW * 0.68f);
                    float btnH = 50.0f * uiS;
                    BindMainShader();
                    drawRect(btnX, btnY, btnW, btnH,
                             0.026f, 0.032f, 0.048f, 0.88f * da);
                    drawConstellFrame(btnX, btnY, btnW, btnH,
                                      cr2, cg2, cb2, 0.36f * da,
                                      12.0f * uiS, 3.0f * uiS);
                    const wchar_t* note = L"CATALOG ONLY";
                    float nSc = 0.48f * uiS;
                    float nW = g_TextS.Width(note, nSc);
                    g_TextS.Draw(note, btnX + (btnW - nW) * 0.5f,
                                 btnY + (btnH - g_TextS.Height(note, nSc)) * 0.5f,
                                 nSc, cr2, cg2, cb2, 0.74f * da);
                }
            }
        }
    }

    // ── Footer ──────────────────────────────────────────────────────────
    (void)leftX;
    (void)footY;
    if (finishBackAfterRender) {
        s_backExit = false;
        s_backOutT = 0.0f;
        s_ShopBackRequested = false;
        s_MainMenuArmoryPanel = false;
        s_MainMenuResumeFromPanel = true;
        g_MainMenuEntryT = 1.0f;
        g_GameManager.currentState = GameState::MAIN_MENU;
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
        struct GhostDef { const wchar_t* route; const wchar_t* sub; };
        static const GhostDef kGhostMenu[5] = {
            { L"PLAY",        L"\uC2DC\uC791" },
            { L"SHOP",        L"\uC0C1\uC810" },
            { L"ASTRAL_LOG",  L"\uB3C4\uAC10" },
            { L"SETTING",     L"\uC124\uC815" },
            { L"EXIT",        L"\uAC8C\uC784 \uC885\uB8CC" },
        };
        const float ghostSlide = 250.0f * oldOut;
        const float anchorX = mainX - ghostSlide - 36.0f;
        BindMainShader();
        drawRect(anchorX, std::max(22.0f, MainLogoTop(sh) - 20.0f),
                 1.2f, mainTotalH + 170.0f,
                 0.48f, 0.82f, 1.0f, 0.12f * oldMenuA);
        for (int i = 0; i < 5; ++i) {
            const bool focus = (!s_backExit && i == 2);
            const float rowY = mainY + (float)i * (mainBH + mainGap);
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
            while (routeSc > 0.82f && g_TextL.Width(kGhostMenu[i].route, routeSc) > mainBW)
                routeSc -= 0.04f;
            const float routeY2 = rowY + 2.0f;
            const float subY = routeY2 + g_TextL.Height(kGhostMenu[i].route, routeSc) - 3.0f;
            const float gr = s_backExit ? 1.0f : 1.0f - 0.52f * (1.0f - active);
            const float gg = s_backExit ? 1.0f : 1.0f - 0.18f * (1.0f - active);
            g_TextL.Draw(kGhostMenu[i].route, rowX, routeY2, routeSc, gr, gg, 1.0f, rowA);
            g_TextS.Draw(kGhostMenu[i].sub, rowX + 4.0f, subY, 0.48f,
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
        CM_NORMAL, CM_RANGED, CM_DDOS, CM_SPAWNER, CM_GRAVIS, CM_QUASAR
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

#if 0
    // Legacy vertical tree retained temporarily for reference while the
    // archive data is shared with the orbital renderer above.
    // Depth 2 list.
    BindMainShader();
    const float parentY = rootY + (float)s_cat * (rootH + rootGap) + rootH * 0.5f;
    const float trunkX = depth2X - 26.0f * uiS;
    LogoLine(rootX + rootW + 14.0f * uiS, parentY, trunkX, parentY,
             0.80f * uiS, curRoot.r, curRoot.g, curRoot.b, 0.22f * wake);

    const float itemH = 48.0f * uiS;
    const float grpH = 36.0f * uiS;
    float totalH = 0.0f;
    for (int i = 0; i < itemCount; ++i)
        totalH += items[i].isGroup ? grpH : itemH;
    float& scroll = s_scroll[s_cat];
    const float maxScroll = std::max(0.0f, totalH - listH + 16.0f * uiS);
    const bool overList = inputReady
        && (mx >= listX - 36.0f * uiS && mx < listX + listW + 22.0f * uiS
         && my >= listY && my < listY + listH);
    if (overList && g_ScrollAccum != 0.0f)
        scroll -= g_ScrollAccum * 36.0f * uiS;
    g_ScrollAccum = 0.0f;
    scroll = std::max(0.0f, std::min(scroll, maxScroll));

    BatchFlush();
    glEnable(GL_SCISSOR_TEST);
    {
        int scX = (int)(listX - 42.0f * uiS);
        int scY = (int)(sh - (listY + listH));
        int scW = (int)(listW + 70.0f * uiS);
        int scH = (int)listH;
        if (scX < 0) scX = 0; if (scY < 0) scY = 0;
        glScissor(scX, scY, scW, scH);
    }
    float curY = listY + 6.0f * uiS - scroll;
    const float treeTop = curY;
    const float treeBot = curY + totalH - 8.0f * uiS;
    BindMainShader();
    LogoLine(trunkX, std::min(parentY, treeTop + 12.0f * uiS),
             trunkX, std::max(parentY, treeBot),
             0.72f * uiS, curRoot.r, curRoot.g, curRoot.b, 0.16f * wake);

    for (int i = 0; i < itemCount; ++i) {
        const CItem& itm = items[i];
        const float rh = itm.isGroup ? grpH : itemH;
        const float ry = curY;
        curY += rh;
        if (ry + rh < listY || ry > listY + listH) {
            s_itemHover[i] = UpdateMenuCommandHover(s_itemHover[i], false, dt);
            continue;
        }

        if (itm.isGroup) {
            const float mid = ry + grpH * 0.5f;
            BindMainShader();
            LogoLine(trunkX, mid, depth2X - 8.0f * uiS, mid,
                     0.72f * uiS, itm.r, itm.g, itm.b, 0.20f * wake);
            drawRect(depth2X + 4.0f * uiS, mid - 0.5f * uiS,
                     depth2W * 0.92f, 1.0f * uiS,
                     itm.r, itm.g, itm.b, 0.080f * wake);
            drawDiamond(depth2X - 8.0f * uiS, mid,
                        3.3f * uiS, itm.r, itm.g, itm.b, 0.46f * wake);
            DrawShadowedText(g_TextS, itm.label, depth2X + 14.0f * uiS,
                             ry + (grpH - g_TextS.Height(itm.label, 0.60f * uiS)) * 0.5f,
                             0.60f * uiS, 0.80f, 0.90f, 1.0f, 0.76f * wake, 0.62f);
        } else {
            const bool isSel = (s_sel[s_cat] == itm.key);
            const bool hov = inputReady
                && (mx >= depth2X - 24.0f * uiS && mx < depth2X + depth2W + 18.0f * uiS
                 && my >= ry - 4.0f * uiS && my < ry + itemH + 5.0f * uiS);
            s_itemHover[i] = UpdateMenuCommandHover(s_itemHover[i], hov, dt);
            const float itemActive = std::max(s_itemHover[i], isSel ? 1.0f : 0.0f);
            if (hov && lmb && !g_LmbPrev)
                s_sel[s_cat] = itm.key;

            const float cardY = ry + 2.0f * uiS;
            const float cardH = itemH - 4.0f * uiS;
            const float ndy = cardY + cardH * 0.5f;
            BindMainShader();
            LogoLine(trunkX, ndy, depth2X - 12.0f * uiS, ndy,
                     0.68f * uiS, itm.r, itm.g, itm.b,
                     (0.10f + 0.20f * itemActive) * wake);
            if (itemActive > 0.01f) {
                drawRect(depth2X + 24.0f * uiS, cardY + cardH * 0.5f,
                         depth2W - 26.0f * uiS, 1.0f * uiS,
                         itm.r, itm.g, itm.b,
                         (0.050f * itemActive + (isSel ? 0.045f : 0.0f)) * wake);
            }
            if (isSel) {
                drawRect(depth2X - 2.0f * uiS, cardY + 8.0f * uiS,
                         2.0f * uiS, cardH - 16.0f * uiS,
                         itm.r, itm.g, itm.b, 0.58f * wake);
                g_TextS.Draw(L">", depth2X + 7.0f * uiS,
                             cardY + (cardH - g_TextS.Height(L">", 0.62f * uiS)) * 0.5f,
                             0.62f * uiS, itm.r, itm.g, itm.b, 0.90f * wake);
            }
            drawDiamond(depth2X - 12.0f * uiS, ndy,
                        (2.0f + 1.3f * itemActive) * uiS,
                        itm.r, itm.g, itm.b, (0.14f + 0.44f * itemActive) * wake);
            float tsc = 0.62f * uiS;
            while (tsc > 0.48f * uiS && g_TextS.Width(itm.label, tsc) > depth2W - 54.0f * uiS)
                tsc -= 0.025f * uiS;
            DrawShadowedText(g_TextS, itm.label,
                             depth2X + (isSel ? 27.0f : 16.0f) * uiS,
                             cardY + (cardH - g_TextS.Height(itm.label, tsc)) * 0.5f,
                             tsc,
                             (itm.seen ? 0.70f : 0.54f) + (1.0f - (itm.seen ? 0.70f : 0.54f)) * itemActive,
                             (itm.seen ? 0.77f : 0.59f) + (1.0f - (itm.seen ? 0.77f : 0.59f)) * itemActive,
                             (itm.seen ? 0.87f : 0.68f) + (1.0f - (itm.seen ? 0.87f : 0.68f)) * itemActive,
                             (0.74f + 0.24f * itemActive) * wake, 0.66f);
        }
    }
    BatchFlush();
    glDisable(GL_SCISSOR_TEST);
#endif

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
    BindMainShader();
    g_TextL.Draw(L"ASTRAL_LOG",
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
              { CM_NORMAL, CM_RANGED, CM_DDOS, CM_SPAWNER, CM_GRAVIS, CM_QUASAR, 0, 0, 0 },
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
                const wchar_t* threatLv = (selItem >= CM_BRUTE) ? L"HIGH" : L"MODERATE";
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
            if (*p == L'/') { if (!cur.empty()) lines.push_back(cur); cur.clear(); }
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
    bool classJob = false;
    if (g_SelectedJob > 0 && g_SelectedJob < JOB_COUNT) {
        const JobDef& jd = JOB_DEFS[g_SelectedJob];
        for (int a = 0; a < jd.startAugCount; a++) {
            int ji = AugIndexOf(jd.startAugs[a]);
            if (ji < 0) continue;
            g_Stats.Apply(jd.startAugs[a]);
            g_OwnedAugs.push_back(ji);
            // 조합 레시피(g_TypeOwned)와 분리 — 런 중 획득한 증강만 조합에 사용
            MarkAugSeen(ji);
            if (AugOnceOnly(jd.startAugs[a], ALL_AUGS[ji].rarity))
                g_GameManager.takenOnce[ji] = true;
            EquipSkill(SkillForAug(jd.startAugs[a]));
            ApplyAugmentSideEffects(jd.startAugs[a], (int)sw, (int)sh);
        }
        if (jd.weaponMode == 1) {           // 검객: 근접 호 스윙
            g_Stats.meleeWeapon  = true;
            g_Stats.fireInterval = 0.26f;
            g_Stats.baseFireInterval = g_Stats.fireInterval;
            g_RunMelee = true; classJob = true;
        } else if (jd.weaponMode == 2) {    // 궁수: 차징 화살
            g_Stats.bowWeapon    = true;
            g_Stats.bulletSpeed *= 1.4f;
            g_RunBow = true; classJob = true;
        }
        fireTimer = g_Stats.fireInterval;
    }
    // 검객/궁수는 총기 표기가 무의미 → 변환/게임오버 표시용 무기 제거
    if (classJob) g_CurrentWeapon = -1;
    // 크리에이티브: 직접 고른 시작 증강 즉시 적용 (스탯+보유목록 직접)
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

// 선택된 직업(g_SelectedJob)의 고정 무기 인덱스. 근접/활 직업은 내부적으로
// RIFLE 을 더미 베이스로 쓰고 weaponMode 가 이후 덮어쓴다.
static int FixedWeaponForSelectedJob() {
    if (g_SelectedJob > 0 && g_SelectedJob < JOB_COUNT &&
        JOB_DEFS[g_SelectedJob].fixedWeapon >= 0)
        return JOB_DEFS[g_SelectedJob].fixedWeapon;
    return (int)StartWeapon::RIFLE;
}

void Scene_JobSelect(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    GLFWwindow* window = c.window;
    const GameState st = g_GameManager.currentState;
    float& fireTimer = *c.fireTimer;
    const std::function<void()>& ResetForNewGame = c.reset;
                BindMainShader();
                const float FW = FLOW_PANEL_W, FH = FLOW_PANEL_H;
                float fx, fy, fcy;
                DrawMenuBackground(sw, sh, delta);
                SceneFlowWindow(sw, sh, FW, FH, L"ASTRAL CAREER", 0.55f, 0.7f, 1.0f, fx, fy, fcy, 0.0f);
                const float backY = FlowBackY(fy, FH);

                int li = LangIndex();
                const wchar_t* JTIT[3] = { L"직업 선택", L"Choose a Class", L"職業を選択" };
                const wchar_t* JHINT[3] = {
                    L"업적을 달성하면 새 직업이 해금됩니다",
                    L"Complete achievements to unlock more classes",
                    L"実績達成で新しい職業が解放されます" };
                const wchar_t* LOCKED[3] = { L"잠김 — ", L"Locked — ", L"未解放 — " };
                const wchar_t* TIT = JTIT[li];
                float titSc = 1.35f;
                float titleY = fcy + 16.0f;
                float titW = g_TextL.Width(TIT, titSc);
                g_TextL.Draw(TIT, fx + (FW - titW) * 0.5f, titleY, titSc, 1, 1, 1, 1);
                const wchar_t* HN = JHINT[li];
                float hintSc = 0.88f;
                float hintY = titleY + g_TextL.Height(TIT, titSc) + 20.0f;
                float hintW = g_TextS.Width(HN, hintSc);
                g_TextS.Draw(HN, fx + (FW - hintW) * 0.5f, hintY, hintSc,
                             0.7f, 0.8f, 0.9f, 0.9f);

                const float BW = 920.0f, BH = 84.0f, BG = 14.0f;
                const float ICON_W = 58.0f;
                const float TX = 14.0f;
                float bx = fx + (FW - BW) * 0.5f;
                float by = hintY + g_TextS.Height(HN, hintSc) + 36.0f;
                static const int kShownJobs[] = { JOB_NONE, JOB_SWORDSMAN, JOB_ARCHER };
                for (int jRow = 0; jRow < 3; jRow++) {
                    int j = kShownJobs[jRow];
                    float y = by + jRow * (BH + BG);
                    bool unlocked = JobUnlocked(j);
                    bool sel = (g_SelectedJob == j);
                    bool clicked = false;
                    if (unlocked) {
                        clicked = UIButton(bx, y, BW, BH, L"", mx, my, lmb, g_LmbPrev, sel);
                    } else {
                        BindMainShader();
                        drawRect(bx, y, BW, BH, 0.08f, 0.06f, 0.06f, 0.9f);
                        drawRect(bx, y, BW, 2.0f, 0.4f,0.3f,0.3f,0.8f);
                        drawRect(bx, y+BH-2, BW, 2.0f, 0.4f,0.3f,0.3f,0.8f);
                    }
                    GLuint ji = JobIcon(j);
                    if (ji) {
                        float isz = BH - 24.0f;
                        DrawIcon(ji, bx + 10.0f, y + (BH - isz)*0.5f, isz, isz,
                                 unlocked?1.0f:0.45f, unlocked?1.0f:0.45f, unlocked?1.0f:0.5f, 0.95f);
                    }
                    BindMainShader();
                    float textX = bx + ICON_W + TX;
                    float textW = BW - ICON_W - TX - 12.0f;
                    float nsc = 0.82f;
                    while (nsc > 0.52f && g_TextL.Width(JobName(j), nsc) > textW) nsc -= 0.04f;
                    g_TextL.Draw(JobName(j), textX, y + 12.0f, nsc,
                                 unlocked?1.0f:0.55f, unlocked?1.0f:0.55f, unlocked?1.0f:0.6f, 0.98f);
                    wchar_t line[200];
                    float lr=0.85f, lg=0.95f, lb=1.0f;
                    if (unlocked) {
                        swprintf_s(line, L"%ls", JobDesc(j));
                    } else {
                        int a = JOB_DEFS[j].unlockAch;
                        swprintf_s(line, L"%ls%ls", LOCKED[li],
                                   (a>=0 && a<ACH_COUNT) ? AchName(a) : L"???");
                        lr=0.9f; lg=0.45f; lb=0.45f;
                    }
                    float dsc = 0.70f;
                    while (dsc > 0.48f && g_TextS.Width(line, dsc) > textW) dsc -= 0.03f;
                    g_TextS.Draw(line, textX, y + 46.0f, dsc, lr, lg, lb, 0.92f);

                    if (clicked) {
                        g_SelectedJob = j;
                        ResetForNewGame();
                        FinalizeLoadout(c, FixedWeaponForSelectedJob());
                    }
                }
                if (UIButton(fx + 32.0f, backY, 160.0f, 48.0f, T(StrId::BTN_BACK),
                             mx, my, lmb, g_LmbPrev)) {
                    g_GameManager.currentState = g_CreativeMode
                        ? GameState::CREATIVE_CONFIG : GameState::MAIN_MENU;
                }
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

                g_TextS.Draw(L"Boss", leftX, contentTop + 90.0f, 1.0f, 1, 1, 1, 0.9f);
                g_TextS.Draw(L"(런 중 B = 선택 보스 즉시 스폰)", leftX, contentTop + 108.0f,
                             0.52f, 0.72f, 0.78f, 0.88f, 0.85f);
                g_TextS.Draw(L"* 일반 런 미포함 (개발용)", leftX, contentTop + 124.0f,
                             0.48f, 0.68f, 0.75f, 0.82f, 0.75f);
                struct BossOpt { const wchar_t* l; int v; };
                BossOpt bOpts[5] = {
                    {L"None",-1}, {L"VOLLEY",2}, {L"TESSERACT",10},
                    {L"FORK",8}, {L"ETHER SWORD",20}
                };
                const float BBW = 112.0f;
                for (int i = 0; i < 5; i++) {
                    int col = i % 3, row = i / 3;
                    float ox = leftX + col * (BBW + OBG);
                    float oy = contentTop + 144.0f + row * (OBH + 8.0f);
                    bool sel = (g_CreativeBossPick == bOpts[i].v);
                    if (UIButton(ox, oy, BBW, OBH, bOpts[i].l,
                                 mx, my, lmb, g_LmbPrev, sel))
                        g_CreativeBossPick = bOpts[i].v;
                }

                g_TextS.Draw(L"Start Augments", leftX, contentTop + 236.0f, 1.0f, 1, 1, 1, 0.9f);
                int aOpts[4] = { 0, 3, 5, 10 };
                for (int i = 0; i < 4; i++) {
                    float ox = leftX + i * (OBW + OBG);
                    wchar_t lb[8]; swprintf_s(lb, L"%d", aOpts[i]);
                    bool sel = (g_CreativeStartAugs == aOpts[i]);
                    if (UIButton(ox, contentTop + 264.0f, OBW, OBH, lb,
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
static void Scene_RunConfigInlineOld(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh, dt = std::min(c.delta, 0.05f);
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const bool lmbClick = lmb && !g_LmbPrev;

    static float entry = 0.0f;
    static float exitT = 0.0f;
    static bool exiting = false;
    static bool launchExit = false;
    static float launchT = 0.0f;
    static bool prevEsc = false, prevRmb = false;
    static int weapon = 0;
    static float weaponHover[2] = {};
    static float trialHover[6] = {};
    static float backHover = 0.0f;
    static float launchHover = 0.0f;
    static float statT[6] = { 0.50f, 0.78f, 0.78f, 0.72f, 0.68f, 0.72f };
    static const float kStatTargets[2][6] = {
        { 0.50f, 0.78f, 0.78f, 0.72f, 0.68f, 0.72f },
        { 0.22f, 0.86f, 0.94f, 1.00f, 0.50f, 0.28f }
    };

    if (entry <= 0.0f && !exiting) {
        weapon = std::max(0, std::min(1, s_RcWeapon));
        for (int i = 0; i < 2; ++i) weaponHover[i] = 0.0f;
        for (int i = 0; i < 6; ++i) trialHover[i] = 0.0f;
        backHover = launchHover = 0.0f;
        int activeTrialCount = 0;
        for (int i = 0; i < 6; ++i)
            if (s_RcTrialNodes[i]) ++activeTrialCount;
        if (activeTrialCount == 0 && s_TrialTargetCount > 0) {
            s_RcTrialNodes[0] = true;
            activeTrialCount = 1;
        }
        s_TrialTargetCount = activeTrialCount;
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

    const float entryIn = Smoothstep(LogoClamp01((entry - 0.16f) / 0.42f));
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

    const int li = std::max(0, std::min(2, LangIndex()));
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
    const float infoX = pageL + pageW * 0.75f;
    const float infoW = std::max(210.0f, pageR - infoX);
    const float centerL = pageL;
    const float centerR = infoX - 46.0f * uiS;
    const float centerW = std::max(320.0f, centerR - centerL);
    const float centerY = bodyTop + (bodyBottom - bodyTop) * 0.50f;
    // One central hero chart replaces the previous left constellation column.
    // The right column remains reserved for the secondary trial controls.
    const float chartCX = centerL + centerW * 0.46f;
    const float chartCY = bodyTop + (bodyBottom - bodyTop) * 0.38f;
    const float chartR = std::min(310.0f * uiS,
        std::min(centerW * 0.24f, (bodyBottom - bodyTop) * 0.30f));
    const float now = (float)glfwGetTime();
    constexpr float kPi = 3.14159265358979323846f;
    constexpr float kTau = 6.28318530717958647692f;

    static const wchar_t* kWeaponNames[2][3] = {
        { L"소총", L"RIFLE", L"ライフル" },
        { L"정전기장", L"STATIC FIELD", L"静電場" }
    };
    static const wchar_t* kWeaponRoutes[2] = { L"RIFLE", L"FIELD" };
    static const wchar_t* kWeaponSub[2][3] = {
        { L"정밀 단일 대상", L"PRECISION / SINGLE TARGET", L"精密 / 単体" },
        { L"지속 범위 제압", L"AREA CONTROL / SUSTAINED", L"範囲制圧 / 継続" }
    };
    static const wchar_t* kStatNames[3][6] = {
        { L"공격력", L"연사", L"간격", L"탄속", L"퍼짐", L"사거리" },
        { L"DAMAGE", L"FIRE RATE", L"INTERVAL", L"SPEED", L"SPREAD", L"RANGE" },
        { L"攻撃力", L"連射", L"間隔", L"弾速", L"拡散", L"射程" }
    };
    static const wchar_t* kStatValues[2][6] = {
        { L"50", L"5.0/s", L"0.20s", L"1200", L"0.04", L"900" },
        { L"18", L"CONT.", L"0.08s", L"320", L"0.15", L"180" }
    };
    static const wchar_t* kTrialNames[6][3] = {
        { L"압도", L"OVERWHELMING", L"圧倒" },
        { L"끈질김", L"RELENTLESS", L"執拗" },
        { L"취약", L"FRAGILE", L"脆弱" },
        { L"실명", L"BLIND", L"盲目" },
        { L"가속", L"ACCELERATE", L"加速" },
        { L"엘리트", L"ELITE", L"エリート" }
    };
    static const wchar_t* kTrialDesc[6][3] = {
        { L"적 수 증가", L"MORE HOSTILES", L"敵数増加" },
        { L"체력 회복 없음", L"NO RECOVERY", L"回復なし" },
        { L"받는 피해 증가", L"DAMAGE TAKEN UP", L"被ダメージ増加" },
        { L"시야 축소", L"REDUCED SIGHT", L"視界縮小" },
        { L"적 이동속도 증가", L"FASTER HOSTILES", L"敵速度上昇" },
        { L"엘리트만 등장", L"ELITES ONLY", L"エリートのみ" }
    };
    const float weaponR = weapon == 0 ? 0.28f : 1.00f;
    const float weaponG = weapon == 0 ? 0.90f : 0.72f;
    const float weaponB = weapon == 0 ? 1.00f : 0.24f;
    const float trialR = 1.00f, trialG = 0.60f, trialB = 0.22f;
    const int trialCount = std::max(0, std::min(6, s_TrialTargetCount));

    for (int i = 0; i < 6; ++i)
        statT[i] = UiApproach(statT[i], kStatTargets[weapon][i], dt, 8.0f);

    // The persistent left field is the only page-wide contrast treatment.
    // All other fields are local CircleTexture layers owned by this page.
    DrawPersistentSceneLeftVignette(sw, sh, 0.26f);
    BatchFlush();
    DrawConstellationDisc(chartCX, chartCY, chartR * 1.42f,
                          0.0f, 0.0f, 0.012f, 0.23f * contentA);
    DrawConstellationDisc(chartCX, chartCY, chartR * 0.84f,
                          weaponR * 0.10f, weaponG * 0.10f, weaponB * 0.12f,
                          0.065f * contentA);
    const float infoCX = infoX + infoW * 0.54f;
    const float infoCY = bodyTop + (bodyBottom - bodyTop) * 0.52f;
    const float infoFieldR = std::max(infoW * 0.92f, (bodyBottom - bodyTop) * 0.62f);
    DrawConstellationDisc(infoCX, infoCY, infoFieldR,
                          0.0f, 0.0f, 0.012f, 0.60f * contentA);
    DrawConstellationDisc(infoCX - infoW * 0.05f, infoCY - 4.0f * uiS,
                          infoFieldR * 0.58f,
                          weaponR * 0.13f, weaponG * 0.13f, weaponB * 0.16f,
                          0.10f * contentA);

    // Header: text and one quiet datum line, with no enclosing surface.
    DrawShadowedText(g_TextL, L"PLAY", pageL, headerY,
                     1.16f * uiS, 1.0f, 1.0f, 1.0f, 0.98f * entryHeader, 0.74f);
    DrawShadowedText(g_TextS, L"// RUN PREPARATION", pageL + 78.0f * uiS,
                     headerY + 9.0f * uiS, 0.50f * uiS,
                     weaponR, weaponG, weaponB, 0.78f * entryHeader, 0.58f);
    const wchar_t* escLabel[3] = { L"[ESC]", L"[ESC]", L"[ESC]" };
    const float escW = g_TextS.Width(escLabel[li], 0.46f * uiS);
    DrawShadowedText(g_TextS, escLabel[li], pageR - escW, headerY + 8.0f * uiS,
                     0.46f * uiS, 0.68f, 0.78f, 0.88f, 0.72f * entryHeader, 0.54f);
    LogoLine(pageL, headerY + 42.0f * uiS, pageR, headerY + 42.0f * uiS,
             0.7f * uiS, weaponR, weaponG, weaponB, 0.12f * contentA);

    // The constellation is the single central hero. Weapon selection is
    // intentionally owned by the lower-left route, so this visual remains
    // decorative and never becomes a second clickable control.
    // CircleTexture is the hero's local contrast field: a soft dark core
    // keeps the constellation readable on bright desktop content, while the
    // colored passes make the selected weapon feel illuminated.
    DrawConstellationDisc(chartCX, chartCY, chartR * 1.62f,
                          0.0f, 0.0f, 0.012f, 0.30f * contentA);
    DrawConstellationDisc(chartCX, chartCY, chartR * 1.38f,
                          weaponR * 0.10f, weaponG * 0.10f, weaponB * 0.12f,
                          (0.10f + 0.025f * sinf(now * 1.4f)) * contentA,
                          false);
    DrawConstellationDisc(chartCX, chartCY, chartR * 1.08f,
                          weaponR * 0.12f, weaponG * 0.12f, weaponB * 0.15f,
                          0.14f * contentA, false);
    static const wchar_t* kNoLabel[6] = { L"", L"", L"", L"", L"", L"" };
    const float heroBrightness = 0.52f + 0.12f * (0.5f + 0.5f * sinf(now * 1.7f));
    DrawWeaponPowerConstellation(chartCX, chartCY, chartR,
                                  statT, kNoLabel, kNoLabel, weapon,
                                  now, heroBrightness * contentA, uiS,
                                  weaponR, weaponG, weaponB);

    // Legacy orbit/list layout retained as source fallback only; the active
    // PLAY layout uses the right-side ON/OFF list below.
    if (false) {
    // Legacy shackle orbit. It is intentionally unreachable; the right-side
    // trial rows below own the preparation input in the current layout.
    const float orbitCX = chartCX + centerW * 0.27f;
    const float orbitCY = chartCY;
    const float orbitRX = std::min(184.0f * uiS, centerW * 0.18f);
    const float orbitRY = std::min(96.0f * uiS, (bodyBottom - bodyTop) * 0.15f);
    for (int ring = 0; ring < 2; ++ring) {
        const float rx = orbitRX * (0.78f + 0.22f * ring);
        const float ry = orbitRY * (0.78f + 0.22f * ring);
        float px = orbitCX + cosf(now * (ring ? -0.04f : 0.05f)) * rx;
        float py = orbitCY + sinf(now * (ring ? -0.04f : 0.05f)) * ry;
        for (int s = 1; s <= 48; ++s) {
            const float a = kTau * (float)s / 48.0f + now * (ring ? -0.04f : 0.05f);
            const float nx = orbitCX + cosf(a) * rx;
            const float ny = orbitCY + sinf(a) * ry;
            if ((s + ring) % 5 != 2)
                DrawVisibleConstellLine(px, py, nx, ny, 0.58f * uiS,
                                        trialR, trialG, trialB,
                                        (ring ? 0.09f : 0.15f) * contentA);
            px = nx;
            py = ny;
        }
    }
    float trialX[6] = {}, trialY[6] = {};
    for (int i = 0; i < 6; ++i) {
        const float a = -kPi * 0.5f + kTau * (float)i / 6.0f + now * 0.045f;
        trialX[i] = orbitCX + cosf(a) * orbitRX * 0.92f;
        trialY[i] = orbitCY + sinf(a) * orbitRY * 0.92f;
        const bool selected = i < trialCount;
        const float nodeA = (selected ? 0.76f : 0.36f + 0.24f * trialHover[i]) * contentA;
        if (selected)
            DrawConstellationDisc(trialX[i], trialY[i],
                                  (11.0f + 2.5f * sinf(now * 2.2f + i)) * uiS,
                                  trialR, trialG, trialB, 0.09f * nodeA);
        DrawVisibleConstellNode(trialX[i], trialY[i],
                                (selected ? 5.2f : 3.6f + trialHover[i]) * uiS,
                                trialR, trialG, trialB, nodeA,
                                selected, selected);
        wchar_t slotLabel[8];
        swprintf_s(slotLabel, L"S%d", i + 1);
        const float labelW = g_TextS.Width(slotLabel, 0.34f * uiS);
        DrawShadowedText(g_TextS, slotLabel, trialX[i] - labelW * 0.5f,
                         trialY[i] + 17.0f * uiS, 0.34f * uiS,
                         selected ? trialR : 0.60f, selected ? trialG : 0.68f,
                         selected ? trialB : 0.78f,
                          (selected ? 0.72f : 0.40f + trialHover[i] * 0.18f) * contentA, 0.46f);
        if (selected && i > 0)
            DrawVisibleConstellLine(trialX[i - 1], trialY[i - 1], trialX[i], trialY[i],
                                    0.7f * uiS, trialR, trialG, trialB, 0.20f * contentA);
    }
    DrawShadowedText(g_TextS, li == 0 ? L"클릭 또는 휠로 개수 조정" :
                     li == 2 ? L"クリックまたはホイールで変更" :
                               L"CLICK OR SCROLL TO SET COUNT",
                     orbitCX - orbitRX * 0.66f, orbitCY + orbitRY + 35.0f * uiS,
                     0.34f * uiS, 0.68f, 0.76f, 0.86f,
                      0.54f * contentA, 0.46f);

    const bool overOrbit = ready && mx >= orbitCX - orbitRX - 28.0f * uiS
        && mx <= orbitCX + orbitRX + 28.0f * uiS
        && my >= orbitCY - orbitRY - 28.0f * uiS
        && my <= orbitCY + orbitRY + 28.0f * uiS;
    if (overOrbit && g_ScrollAccum != 0.0f) {
        s_TrialTargetCount += g_ScrollAccum > 0.0f ? -1 : 1;
        s_TrialTargetCount = std::max(1, std::min(6, s_TrialTargetCount));
    }
    g_ScrollAccum = 0.0f;
    for (int i = 0; i < 6; ++i) {
        const bool hov = ready && mx >= trialX[i] - 26.0f * uiS && mx <= trialX[i] + 26.0f * uiS
            && my >= trialY[i] - 26.0f * uiS && my <= trialY[i] + 26.0f * uiS;
        trialHover[i] = UpdateMenuCommandHover(trialHover[i], hov, dt);
        if (hov && lmbClick) {
            s_TrialTargetCount = i + 1;
            s_TrialTargetCount = std::max(1, std::min(6, s_TrialTargetCount));
        }
    }

    // Right information readout. CircleTexture is already behind this text;
    // there are no card fills or frame rectangles in this region.
    const float infoTextX = infoX + 18.0f * uiS;
    const float infoRight = pageR - 6.0f * uiS;
    DrawShadowedText(g_TextS, L"WEAPON", infoTextX, bodyTop,
                     0.42f * uiS, weaponR, weaponG, weaponB, 0.78f * contentA, 0.54f);
    const wchar_t* weaponName = kWeaponNames[weapon][li];
    const float weaponNameScale = 0.86f * uiS;
    const float weaponNameW = g_TextL.Width(weaponName, weaponNameScale);
    DrawShadowedText(g_TextL, weaponName, infoRight - weaponNameW,
                     bodyTop - 5.0f * uiS, weaponNameScale,
                     1.0f, 1.0f, 1.0f, 0.96f * contentA, 0.70f);
    LogoLine(infoTextX, bodyTop + 35.0f * uiS, infoRight,
             bodyTop + 35.0f * uiS, 0.8f * uiS,
             weaponR, weaponG, weaponB, 0.28f * contentA);

    const float statY = bodyTop + 58.0f * uiS;
    const float statStep = 31.0f * uiS;
    const float statBarX = infoTextX + infoW * 0.43f;
    const float statBarW = std::max(60.0f * uiS, infoW * 0.28f);
    for (int i = 0; i < 6; ++i) {
        const float y = statY + i * statStep;
        DrawShadowedText(g_TextS, kStatNames[li][i], infoTextX, y,
                         0.38f * uiS, 0.70f, 0.80f, 0.90f, 0.76f * contentA, 0.48f);
        LogoLine(statBarX, y + 12.0f * uiS,
                 statBarX + statBarW, y + 12.0f * uiS,
                 0.7f * uiS, 0.42f, 0.52f, 0.66f, 0.24f * contentA);
        LogoLine(statBarX, y + 12.0f * uiS,
                 statBarX + statBarW * LogoClamp01(statT[i]), y + 12.0f * uiS,
                 1.4f * uiS, weaponR, weaponG, weaponB, 0.72f * contentA);
        DrawVisibleConstellNode(statBarX + statBarW * LogoClamp01(statT[i]),
                                y + 12.0f * uiS, 2.7f * uiS,
                                weaponR, weaponG, weaponB, 0.82f * contentA, false, false);
        const float valueScale = 0.43f * uiS;
        const float valueW = g_TextS.Width(kStatValues[weapon][i], valueScale);
        DrawShadowedText(g_TextS, kStatValues[weapon][i], infoRight - valueW, y,
                         valueScale, 0.92f, 0.96f, 1.0f, 0.90f * contentA, 0.54f);
    }

    const float shackY = statY + 6.0f * statStep + 25.0f * uiS;
    LogoLine(infoTextX, shackY - 11.0f * uiS, infoRight, shackY - 11.0f * uiS,
             0.8f * uiS, trialR, trialG, trialB, 0.24f * contentA);
    wchar_t shackTitle[32];
    swprintf_s(shackTitle, L"SHACKLES  +%d", trialCount);
    DrawShadowedText(g_TextS, shackTitle, infoTextX, shackY,
                     0.48f * uiS, trialR, trialG, trialB, 0.90f * contentA, 0.58f);
    for (int i = 0; i < trialCount; ++i) {
        const float y = shackY + 29.0f * uiS + i * 30.0f * uiS;
        DrawVisibleConstellNode(infoTextX + 5.0f * uiS, y + 5.0f * uiS,
                                3.1f * uiS, trialR, trialG, trialB,
                                0.84f * contentA, false, false);
        wchar_t slot[8];
        swprintf_s(slot, L"S%d", i + 1);
        DrawShadowedText(g_TextS, slot, infoTextX + 15.0f * uiS, y,
                         0.34f * uiS, trialR, trialG, trialB, 0.82f * contentA, 0.46f);
        DrawShadowedText(g_TextS, kTrialNames[i][li], infoTextX + 43.0f * uiS, y,
                         0.40f * uiS, 0.94f, 0.96f, 1.0f, 0.88f * contentA, 0.52f);
        DrawShadowedText(g_TextS, kTrialDesc[i][li], infoTextX + 43.0f * uiS,
                         y + 14.0f * uiS, 0.31f * uiS,
                         trialR, trialG, trialB, 0.62f * contentA, 0.42f);
    }

    const float summaryY = bodyBottom - 68.0f * uiS;
    LogoLine(infoTextX, summaryY - 14.0f * uiS, infoRight, summaryY - 14.0f * uiS,
             0.8f * uiS, weaponR, weaponG, weaponB, 0.22f * contentA);
    DrawShadowedText(g_TextS, L"RUN SUMMARY", infoTextX, summaryY,
                     0.38f * uiS, weaponR, weaponG, weaponB, 0.72f * contentA, 0.48f);
    DrawShadowedText(g_TextS, L"WEAPON", infoTextX, summaryY + 23.0f * uiS,
                     0.34f * uiS, 0.66f, 0.76f, 0.86f, 0.62f * contentA, 0.42f);
    DrawShadowedText(g_TextS, weaponName, infoRight - weaponNameW * 0.78f,
                     summaryY + 19.0f * uiS, 0.42f * uiS,
                     0.92f, 0.96f, 1.0f, 0.84f * contentA, 0.50f);
    DrawShadowedText(g_TextS, L"SHACKLES", infoTextX, summaryY + 47.0f * uiS,
                     0.34f * uiS, 0.66f, 0.76f, 0.86f, 0.62f * contentA, 0.42f);
    DrawShadowedText(g_TextS, shackTitle, infoRight - g_TextS.Width(shackTitle, 0.42f * uiS),
                     summaryY + 43.0f * uiS, 0.42f * uiS,
                     trialR, trialG, trialB, 0.90f * contentA, 0.50f);

    }

    // The title and values belong to the hero chart instead of living in a
    // detached center column. This keeps the weapon readout as one visual
    // object, matching the constellation's selected silhouette.
    const float heroTextL = chartCX - chartR * 0.68f;
    const float heroTextR = chartCX + chartR * 0.68f;
    const wchar_t* heroName = kWeaponNames[weapon][li];
    const float heroSystemScale = 0.42f * uiS;
    const float heroSystemW = g_TextS.Width(L"WEAPON CONSTELLATION", heroSystemScale);
    DrawShadowedText(g_TextS, L"WEAPON CONSTELLATION", chartCX - heroSystemW * 0.5f,
                     bodyTop + 10.0f * uiS, heroSystemScale,
                     weaponR, weaponG, weaponB, 0.80f * contentA, 0.54f);
    const float heroNameScale = 0.92f * uiS;
    const float heroNameW = g_TextL.Width(heroName, heroNameScale);
    DrawShadowedText(g_TextL, heroName, chartCX - heroNameW * 0.5f,
                     bodyTop + 43.0f * uiS, heroNameScale,
                     1.0f, 1.0f, 1.0f, 0.96f * contentA, 0.72f);
    const float heroSubScale = 0.42f * uiS;
    const float heroSubW = g_TextS.Width(kWeaponSub[weapon][li], heroSubScale);
    DrawShadowedText(g_TextS, kWeaponSub[weapon][li], chartCX - heroSubW * 0.5f,
                     bodyTop + 78.0f * uiS, heroSubScale,
                     weaponR, weaponG, weaponB, 0.82f * contentA, 0.52f);
    LogoLine(heroTextL, bodyTop + 103.0f * uiS, heroTextR,
             bodyTop + 103.0f * uiS, 0.8f * uiS,
             weaponR, weaponG, weaponB, 0.24f * contentA);

    // Six values form a compact telemetry strip under the constellation.
    // Three columns keep the readout attached to the hero without creating a
    // second panel or competing with the right-side trials list.
    const float statAreaL = centerL + centerW * 0.27f;
    const float statAreaW = centerW * 0.54f;
    const float statColW = statAreaW / 3.0f;
    const float detailStatY = chartCY + chartR + 18.0f * uiS;
    const float detailStatStep = 36.0f * uiS;
    for (int i = 0; i < 6; ++i) {
        const int col = i % 3;
        const int row = i / 3;
        const float x = statAreaL + col * statColW;
        const float y = detailStatY + row * detailStatStep;
        const float valueScale = 0.42f * uiS;
        const float valueW = g_TextS.Width(kStatValues[weapon][i], valueScale);
        const float valueX = x + statColW - valueW - 8.0f * uiS;
        const float barX = x + 78.0f * uiS;
        const float barW = std::max(52.0f * uiS, valueX - barX - 12.0f * uiS);
        DrawShadowedText(g_TextS, kStatNames[li][i], x, y,
                         0.36f * uiS, 0.70f, 0.80f, 0.90f,
                         0.82f * contentA, 0.50f);
        LogoLine(barX, y + 12.0f * uiS,
                 barX + barW, y + 12.0f * uiS,
                 0.8f * uiS, 0.38f, 0.48f, 0.60f, 0.24f * contentA);
        LogoLine(barX, y + 12.0f * uiS,
                 barX + barW * LogoClamp01(statT[i]), y + 12.0f * uiS,
                 1.5f * uiS, weaponR, weaponG, weaponB, 0.76f * contentA);
        DrawVisibleConstellNode(barX + barW * LogoClamp01(statT[i]),
                                y + 12.0f * uiS, 2.8f * uiS,
                                weaponR, weaponG, weaponB, 0.84f * contentA,
                                false, false);
        DrawShadowedText(g_TextS, kStatValues[weapon][i], valueX, y,
                         valueScale, 0.94f, 0.97f, 1.0f,
                         0.94f * contentA, 0.56f);
    }

    // Right-side trial list. Each modifier is an independent ON/OFF control;
    // the list is the only trial selection surface on PLAY.
    const float trialListX = infoX + 18.0f * uiS;
    const float trialListRight = pageR - 6.0f * uiS;
    const float trialListW = std::max(190.0f * uiS, trialListRight - trialListX);
    DrawShadowedText(g_TextS, L"TRIALS", trialListX, bodyTop,
                     0.44f * uiS, trialR, trialG, trialB,
                     0.84f * contentA, 0.56f);
    wchar_t activeTrialLabel[24];
    swprintf_s(activeTrialLabel, L"%d / 6 ACTIVE", trialCount);
    const float activeW = g_TextS.Width(activeTrialLabel, 0.52f * uiS);
    DrawShadowedText(g_TextL, activeTrialLabel, trialListRight - activeW,
                     bodyTop - 5.0f * uiS, 0.52f * uiS,
                     trialR, trialG, trialB, 0.92f * contentA, 0.68f);
    LogoLine(trialListX, bodyTop + 35.0f * uiS, trialListRight,
             bodyTop + 35.0f * uiS, 0.8f * uiS,
             trialR, trialG, trialB, 0.30f * contentA);

    const float trialRowY = bodyTop + 58.0f * uiS;
    const float trialRowH = 58.0f * uiS;
    const float trialHitX = trialListX - 12.0f * uiS;
    const float trialHitW = trialListW + 24.0f * uiS;
    for (int i = 0; i < 6; ++i) {
        const float y = trialRowY + i * trialRowH;
        const bool on = s_RcTrialNodes[i];
        const bool rowHov = ready && mx >= trialHitX && mx <= trialHitX + trialHitW
            && my >= y && my <= y + trialRowH - 5.0f * uiS;
        trialHover[i] = UpdateMenuCommandHover(trialHover[i], rowHov, dt);
        const float rowA = (on ? 0.76f : 0.44f) + 0.16f * trialHover[i];
        if (on || trialHover[i] > 0.02f)
            drawRect(trialListX - 5.0f * uiS, y - 4.0f * uiS,
                     trialListW + 10.0f * uiS, trialRowH - 7.0f * uiS,
                     trialR, trialG, trialB,
                     (on ? 0.045f : 0.020f) + 0.035f * trialHover[i]);
        DrawVisibleConstellNode(trialListX + 4.0f * uiS, y + 16.0f * uiS,
                                (on ? 4.6f : 3.4f + trialHover[i]) * uiS,
                                trialR, trialG, trialB, rowA * contentA,
                                on, on);
        DrawShadowedText(g_TextS, kTrialNames[i][li], trialListX + 20.0f * uiS, y,
                         0.43f * uiS, on ? 0.96f : 0.68f, on ? 0.96f : 0.72f,
                         on ? 1.0f : 0.82f, rowA * contentA, 0.52f);
        DrawShadowedText(g_TextS, kTrialDesc[i][li], trialListX + 20.0f * uiS,
                         y + 18.0f * uiS, 0.32f * uiS,
                         trialR, trialG, trialB, (on ? 0.68f : 0.42f) * contentA, 0.42f);
        const wchar_t* toggleLabel = on ? L"ON" : L"OFF";
        const float toggleScale = 0.44f * uiS;
        const float toggleW = g_TextS.Width(toggleLabel, toggleScale);
        DrawShadowedText(g_TextS, toggleLabel, trialListRight - toggleW, y + 5.0f * uiS,
                         toggleScale, on ? trialR : 0.56f, on ? trialG : 0.64f,
                         on ? trialB : 0.74f, (on ? 0.94f : 0.56f) * contentA, 0.56f);
        LogoLine(trialListX, y + trialRowH - 8.0f * uiS,
                 trialListRight, y + trialRowH - 8.0f * uiS,
                 0.65f * uiS, trialR, trialG, trialB,
                 (on ? 0.22f : 0.11f) * contentA);
        if (rowHov && lmbClick) {
            s_RcTrialNodes[i] = !s_RcTrialNodes[i];
            int activeCount = 0;
            for (int j = 0; j < 6; ++j)
                if (s_RcTrialNodes[j]) ++activeCount;
            s_TrialTargetCount = activeCount;
        }
    }
    g_ScrollAccum = 0.0f;

    // Other pages keep their route controls as a vertical lower-left rail.
    // Play follows that same grammar: only this rail owns weapon selection.
    const float routeX = pageL;
    const float routeW = std::min(300.0f * uiS, centerW * 0.30f);
    const float routeH = 64.0f * uiS;
    const float routeGap = 9.0f * uiS;
    const float routeTotalH = routeH * 3.0f + routeGap * 2.0f;
    const float routeBottom = footerY + footerH * 0.18f;
    const float routeY = routeBottom - routeTotalH;
    const float routeHitX = routeX - 28.0f * uiS;
    const float routeHitW = routeW + 44.0f * uiS;
    const float playW = 270.0f * uiS;
    const float playH = 66.0f * uiS;
    const float playX = chartCX - playW * 0.5f;
    const float playY = bodyBottom - playH - 4.0f * uiS;

    // Keep the weapon route's local runway, but leave the central PLAY action
    // free of the paired bg_linear side treatment.
    const float railPulse = 0.72f + 0.28f * (0.5f + 0.5f * sinf(now * 1.15f));
    const float railLightA = 0.11f * railPulse * (1.0f - backP) * (1.0f - launchP);
    DrawLinearGradientRibbon(routeX - 24.0f * uiS, routeY - 18.0f * uiS,
                             routeW + 48.0f * uiS, routeTotalH + 30.0f * uiS,
                             24.0f * uiS, weaponR, weaponG, weaponB,
                             railLightA, false);
    DrawVisibleConstellLine(routeX - 19.0f * uiS, routeY - 8.0f * uiS,
                            routeX - 19.0f * uiS, routeBottom + 4.0f * uiS,
                            0.9f * uiS, weaponR, weaponG, weaponB,
                            (0.16f + 0.04f * sinf(now * 1.1f)) * contentA);
    DrawShadowedText(g_TextS, L"WEAPONS", routeX, routeY - 30.0f * uiS,
                     0.40f * uiS, weaponR, weaponG, weaponB,
                     0.76f * contentA, 0.50f);

    const bool rifleHov = ready && mx >= routeHitX && mx <= routeHitX + routeHitW
        && my >= routeY && my <= routeY + routeH;
    const bool fieldHov = ready && mx >= routeHitX && mx <= routeHitX + routeHitW
        && my >= routeY + routeH + routeGap
        && my <= routeY + routeH * 2.0f + routeGap;
    const bool backHov = ready && mx >= routeHitX && mx <= routeHitX + routeHitW
        && my >= routeY + (routeH + routeGap) * 2.0f
        && my <= routeY + (routeH + routeGap) * 2.0f + routeH;
    const bool playHov = ready && mx >= playX && mx <= playX + playW
        && my >= playY - 12.0f * uiS && my <= playY + playH + 8.0f * uiS;
    weaponHover[0] = UpdateMenuCommandHover(weaponHover[0], rifleHov, dt);
    weaponHover[1] = UpdateMenuCommandHover(weaponHover[1], fieldHov, dt);
    backHover = UpdateMenuCommandHover(backHover, backHov, dt);
    launchHover = UpdateMenuCommandHover(launchHover, playHov, dt);

    const wchar_t* backSub[3] = { L"뒤로", L"RETURN", L"戻る" };
    DrawUnifiedMenuCommand(kWeaponRoutes[0], kWeaponSub[0][li],
                           routeX, routeY, routeW, routeH,
                           0.28f, 0.90f, 1.0f, contentA,
                           weaponHover[0], weapon == 0,
                           weapon == 0 ? 0.10f + 0.08f * sinf(now * 2.0f) : 0.0f,
                           now, 0.78f, 0.40f, true);
    DrawUnifiedMenuCommand(kWeaponRoutes[1], kWeaponSub[1][li],
                           routeX, routeY + routeH + routeGap, routeW, routeH,
                           1.0f, 0.72f, 0.24f, contentA,
                           weaponHover[1], weapon == 1,
                           weapon == 1 ? 0.10f + 0.08f * sinf(now * 2.0f) : 0.0f,
                           now + 0.17f, 0.78f, 0.40f, true);
    DrawUnifiedMenuCommand(L"BACK", backSub[li],
                           routeX, routeY + (routeH + routeGap) * 2.0f,
                           routeW, routeH, 0.48f, 0.82f, 1.0f, contentA,
                           backHover, false, 0.0f, now + 0.34f,
                           0.78f, 0.40f, true);
    wchar_t readyLabel[64];
    swprintf_s(readyLabel, L"READY  //  %d TRIAL%s ACTIVE",
                trialCount, trialCount == 1 ? L"" : L"S");
    const float readyScale = 0.36f * uiS;
    const float readyW = g_TextS.Width(readyLabel, readyScale);
    DrawShadowedText(g_TextS, readyLabel, chartCX - readyW * 0.5f,
                     playY - 22.0f * uiS, readyScale,
                     weaponR, weaponG, weaponB, 0.76f * contentA, 0.48f);
    DrawUnifiedMenuCommand(L"PLAY", L"START RUN",
                           playX, playY, playW, playH,
                           weaponR, weaponG, weaponB, contentA,
                           launchHover, true,
                           0.12f + 0.10f * sinf(now * 2.2f), now + 0.51f,
                           0.78f, 0.40f, true);

    if (backHov && lmbClick && ready)
        exiting = true;
    if ((rifleHov || fieldHov) && lmbClick && ready) {
        weapon = fieldHov ? 1 : 0;
        s_RcWeapon = weapon;
    }
    if (playHov && lmbClick && ready) {
        launchExit = true;
        launchT = 0.0f;
    }

    g_BatchAlpha = 1.0f;
    if (launchExit && launchT >= 0.50f) {
        g_SelectedJob = weapon == 0 ? JOB_NONE : JOB_VAMPIRE;
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

// PLAY v2: the catalogue is intentionally separate from the legacy six-node
// screen above.  It keeps the page interaction model small: weapon tabs at
// the top-left, a hero constellation in the middle, a scrollable trial
// catalogue on the right, and the common BACK rail at the bottom-left.
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

    static const int kTrialOrder[TRIAL_DEF_COUNT] = {
        // EARLY
        0, 1, 3, 4, 5, 8, 9, 10,
        // MID
        11, 12, 13,
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
        L"MID  /  SCORE +20%",        L"MID  /  SCORE +17%",
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
        L"중반  /  점수 +20%", L"중반  /  점수 +17%",
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
        L"Special and elite enemies receive a stronger midgame appearance bias.",
        L"Bombers arrive earlier and appear more frequently, changing safe movement routes.",
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
        L"특수 및 정예 적이 중반에 등장할 확률이 증가합니다.",
        L"봄버가 더 일찍, 더 자주 등장해 안전한 이동 경로가 달라집니다.",
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
        L"조기 돌입", L"패킷 폭풍", L"콜드 부트", L"엘리트 개화",
        L"봄버 추적", L"프로세스 노이즈", L"후반 초과", L"강화 코어",
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
            if (s_RcTrialEnabled[i]) ++count;
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
            if (!s_RcTrialEnabled[id] || slot >= TRIAL_SLOT_COUNT) continue;
            g_TrialPool[slot] = id;
            g_TrialSelected[slot] = true;
            ++slot;
        }
        const int selectedCount = slot;
        for (; slot < TRIAL_SLOT_COUNT; ++slot)
            g_TrialSelected[slot] = false;
        s_TrialTargetCount = selectedCount;
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
                if (g_TrialSelected[slot] && id >= 0 && id < TRIAL_DEF_COUNT)
                    s_RcTrialEnabled[id] = true;
            }
            s_RcTrialStateLoaded = true;
        }
        for (int id = 0; id < TRIAL_DEF_COUNT; ++id) {
            if (s_RcTrialEnabled[id]) { trialFocus = id; break; }
        }
        for (int i = 0; i < TRIAL_DEF_COUNT; ++i) {
            if (kTrialOrder[i] == trialFocus) {
                trialSelectedSlot = i;
                break;
            }
        }
        trialDisplaySlot = trialTargetSlot = (float)trialSelectedSlot;
        s_TrialTargetCount = countEnabled();
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

    // The trial catalogue is an infinite list on a restrained \ rail. The
    // list owns the full right column, so records keep flowing through the
    // available space instead of being re-centered when one is clicked.
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
            std::min(1.0f, (y - trialViewTop) / trialViewH));
        return trialRailTopX + trialDiagonal * t;
    };
    // Keep the infinite carousel's internal coordinates close to the active
    // slot. The visible result still wraps forever, but a long drag can never
    // grow the float values until the wrapping loops become expensive.
    auto normalizeTrialSlots = [&]() {
        const float relativeTarget = trialTargetSlot
                                   - (float)trialSelectedSlot;
        const float cycles = std::round(
            relativeTarget / (float)TRIAL_DEF_COUNT);
        if (fabsf(cycles) > 0.0f) {
            const float offset = cycles * (float)TRIAL_DEF_COUNT;
            trialTargetSlot -= offset;
            trialDisplaySlot -= offset;
        }
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
        s_TrialTargetCount = 0;
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
                s_TrialTargetCount = countEnabled();
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
            // Move the display directly with the pointer. The target is kept
            // in the same unbounded coordinate space so wrapping remains
            // seamless when the drag crosses the first/last trial.
            trialTargetSlot += dragDelta;
            trialDisplaySlot += dragDelta;
            trialDragMoved = true;
            int nearestSlot = (int)std::round(trialTargetSlot)
                            % TRIAL_DEF_COUNT;
            if (nearestSlot < 0) nearestSlot += TRIAL_DEF_COUNT;
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
        trialSelectedSlot = (trialSelectedSlot + trialStepRequest
                             + TRIAL_DEF_COUNT) % TRIAL_DEF_COUNT;
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
            if (!s_RcTrialEnabled[id]) continue;
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
        return id >= 0 && id < TRIAL_DEF_COUNT && s_RcTrialEnabled[id];
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
        swprintf_s(buf, L"%+.1f%% !92 %+.1f%%",
                   (startMult - 1.0f) * 100.0f,
                   (peakMult - 1.0f) * 100.0f);
        return std::wstring(buf);
    };
    auto formatCountRange = [&](int startCount, int peakCount) {
        wchar_t buf[32] = {};
        if (startCount == peakCount)
            swprintf_s(buf, L"+%d", startCount);
        else
            swprintf_s(buf, L"+%d !92 +%d", startCount, peakCount);
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

    float bomberStart = 1.0f, bomberPeak = 1.0f;
    if (selected(12)) bomberStart *= 0.76f;
    bomberPeak = bomberStart;
    if (selected(14)) bomberPeak *= 0.88f;
    if (selected(12))
        summaryRows.push_back({ PlayText(L"자폭병 첫 등장", L"BOMBER FIRST SPAWN"),
                                PlayText(L"-10.0초", L"-10.0s"), false });
    addPercentMetric(PlayText(L"자폭병 스폰 간격", L"BOMBER INTERVAL"),
                     bomberStart, bomberPeak, false);

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
        for (int order = 0; order < TRIAL_DEF_COUNT; ++order) {
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
    // The catalogue is an infinite diagonal rail. Visible slots stay evenly
    // spaced while their data wraps through the 20 definitions. This keeps
    // the motion legible and gives the right side the requested \ silhouette
    // without sending a curved orbit through the hero.
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
    for (int slot = 0; slot < TRIAL_DEF_COUNT; ++slot) {
        float relF = (float)slot - trialDisplaySlot;
        while (relF > (float)TRIAL_DEF_COUNT * 0.5f)
            relF -= (float)TRIAL_DEF_COUNT;
        while (relF < -(float)TRIAL_DEF_COUNT * 0.5f)
            relF += (float)TRIAL_DEF_COUNT;
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
        g_SelectedJob = weapon == 0 ? JOB_NONE : JOB_VAMPIRE;
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

static void Scene_RunConfigInlineLegacy(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh, dt = std::min(c.delta, 0.05f);
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    static float entry = 0.0f;
    static float exitT = 0.0f;
    static bool exiting = false;
    static bool playExit = false;
    static float playT = 0.0f;
    static bool prevEsc = false, prevRmb = false;
    static int weapon = 0;
    static float weaponHover[2] = {};
    static float backHover    = 0.0f;
    static float playHover    = 0.0f;
    static float chipHover[6] = {};
    static float arrowHover[2]= {};
    static float stripOffset  = 0.0f;
    static bool  stripDrag    = false;
    static float dragStartX   = 0.0f;
    static float dragStartOff = 0.0f;
    static float dragMoved    = 0.0f;
    static float statT[6] = { 0.50f, 0.78f, 0.78f, 0.72f, 0.68f, 0.72f };
    static const float barTargets[2][6] = {
        { 0.50f, 0.78f, 0.78f, 0.72f, 0.68f, 0.72f },
        { 0.22f, 0.86f, 0.94f, 1.00f, 0.50f, 0.28f }
    };

    if (entry <= 0.0f && !exiting) {
        weapon = std::max(0, std::min(1, weapon));
        weaponHover[0] = weaponHover[1] = 0.0f;
        backHover = playHover = 0.0f;
        for (int i = 0; i < 6; ++i) chipHover[i] = 0.0f;
        arrowHover[0] = arrowHover[1] = 0.0f;
        stripOffset = 0.0f; stripDrag = false; dragMoved = 0.0f;
    }
    entry = std::min(1.0f, entry + dt);
    if (playExit) playT = std::min(0.50f, playT + dt);
    const bool esc = c.window && glfwGetKey(c.window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    const bool rmb = c.window && glfwGetMouseButton(c.window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    const bool backInput = (esc && !prevEsc) || (rmb && !prevRmb);
    prevEsc = esc; prevRmb = rmb;
    const float oldOut = Smoothstep(LogoClamp01(entry / 0.35f));
    const float contentIn = Smoothstep(LogoClamp01((entry - 0.20f) / 0.35f));
    if (backInput && !exiting && entry >= 0.55f) exiting = true;
    if (exiting) exitT = std::min(0.42f, exitT + dt);
    const float backP = exiting ? Smoothstep(LogoClamp01(exitT / 0.42f)) : 0.0f;
    const float oldA = exiting ? backP : 1.0f - oldOut;
    const float playCollapse = Smoothstep(LogoClamp01((playT - 0.30f) / 0.20f));
    const float contentA = contentIn * (1.0f - backP) * (1.0f - playCollapse);
    const bool ready = !exiting && !playExit && entry >= 0.55f;
    SetSceneTextureReveal(exiting
        ? std::max(0.0f, 1.0f - backP)
        : Smoothstep(LogoClamp01((entry - 0.04f) / 0.74f)));
    const float uiS = std::max(0.70f, std::min(sw * 0.94f / 1640.0f, sh * 0.90f / 910.0f));
    const float mainX = std::max(58.0f, sw * 0.075f);
    const float mainY = MainMenuCommandStartY(sh);
    const float mainBW = std::min(560.0f, std::max(420.0f, sw * 0.34f));
    const float mainBH = 70.0f, mainGap = 15.0f;
    const float leftX = mainX + 82.0f * uiS * (1.0f - contentIn) + 180.0f * uiS * backP;
    const float leftY = mainY;
    const float leftW = std::min(340.0f * uiS, mainBW * 0.70f);
    const float canvasLeft = std::max(sw * 0.28f, leftX + leftW + 18.0f * uiS);
    const float canvasRight = sw - std::max(32.0f, 36.0f * uiS);
    const float canvasTop = std::max(74.0f, sh * 0.12f);
    const float canvasBottom = sh - std::max(88.0f, 104.0f * uiS);
    const float cw = canvasRight - canvasLeft;
    const float cH = canvasBottom - canvasTop;
    const float contentTop = canvasTop + 58.0f * uiS;
    const float centerY = contentTop + (canvasBottom - contentTop) * 0.48f;
    const float now = (float)glfwGetTime();
    const float kPi = 3.14159265f;

    // 3-column anchors: 무기 성좌 / 굴레 궤도 띠 / 런 요약
    const float starX  = canvasLeft + cw * 0.145f;
    const float stripX = canvasLeft + cw * 0.295f;
    const float stripW = cw * 0.415f;
    const float sumX   = canvasLeft + cw * 0.765f;
    const float sumW   = canvasRight - 8.0f * uiS - sumX;

    const float wr = weapon == 0 ? 0.35f : 0.55f;
    const float wg = weapon == 0 ? 0.72f : 0.90f;
    const float wb = 1.0f;
    // 무기별 고정 별자리 색상: RIFLE=시안, STATIC FIELD=주황
    const float conR = weapon == 0 ? 0.22f : 1.00f;
    const float conG = weapon == 0 ? 0.90f : 0.72f;
    const float conB = weapon == 0 ? 1.00f : 0.22f;
    const int trialCount = s_TrialTargetCount;

    for (int i = 0; i < 6; ++i) {
        statT[i] = UiApproach(statT[i], barTargets[weapon][i], dt, 8.0f);
    }

    DrawPersistentSceneLeftVignette(sw, sh, 0.86f);

    static const wchar_t* menu[5] = { L"PLAY", L"SHOP", L"ASTRAL_LOG", L"SETTING", L"EXIT" };
    static const wchar_t* sub[5] = { L"시작", L"상점", L"도감", L"설정", L"게임 종료" };
    for (int i = 0; i < 5; ++i) {
        const float y = mainY + i * (mainBH + mainGap);
        const float x = mainX - 250.0f * (1.0f - oldA) - 24.0f * (i != 0) * (1.0f - oldA);
        DrawShadowedText(g_TextL, menu[i], x, y + 2.0f, 1.04f,
                         1.0f, 1.0f, 1.0f, (i == 0 ? 0.82f : 0.18f) * oldA, 0.70f);
        DrawShadowedText(g_TextS, sub[i], x + 4.0f, y + 43.0f, 0.48f,
                         0.72f, 0.77f, 0.84f, (i == 0 ? 0.74f : 0.22f) * oldA, 0.62f);
    }

    static const wchar_t* weapons[2] = { L"RIFLE", L"STATIC FIELD" };
    static const wchar_t* weaponSub[2] = { L"PRECISION FIRE / SINGLE TARGET", L"AREA CONTROL / SUSTAINED" };
    static const wchar_t* statNames[6] = { L"DAMAGE", L"FIRE RATE", L"INTERVAL", L"SPEED", L"SPREAD", L"RANGE" };
    static const wchar_t* statVals[2][6] = {
        { L"50", L"5.0/s", L"0.20s", L"1200", L"0.04", L"??" },
        { L"18", L"CONT.", L"0.08s", L"??", L"??", L"180" }
    };
    static const wchar_t* trialNames[6] = {
        L"OVERWHELMING", L"RELENTLESS", L"FRAGILE",
        L"BLIND",        L"ACCELERATE", L"ELITE"
    };
    static const wchar_t* trialDescs[6] = {
        L"적 수 증가", L"체력 회복 없음", L"받는 피해 증가",
        L"시야 축소",  L"적 이동속도 증가", L"엘리트만 등장"
    };
    const float tr = 1.0f, tg = 0.60f, tb = 0.22f; // shackle amber

    // 아주 얕은 어두운 패널 + 코너 브라켓 (과도한 글로우 금지)
    {
        const float la = Smoothstep(LogoClamp01((entry - 0.22f) / 0.42f)) * contentA;
        drawRect(canvasLeft, canvasTop, cw, cH, 0.04f, 0.055f, 0.08f, 0.52f * la);
        const float cwr = 28.0f * uiS, lw = 1.0f * uiS;
        const float cR = canvasRight, cB = canvasBottom;
        LogoLine(canvasLeft, canvasTop, canvasLeft+cwr, canvasTop, lw, wr,wg,wb, 0.50f*la);
        LogoLine(canvasLeft, canvasTop, canvasLeft, canvasTop+cwr, lw, wr,wg,wb, 0.50f*la);
        LogoLine(cR, canvasTop, cR-cwr, canvasTop, lw, wr,wg,wb, 0.50f*la);
        LogoLine(cR, canvasTop, cR, canvasTop+cwr, lw, wr,wg,wb, 0.50f*la);
        LogoLine(canvasLeft, cB, canvasLeft+cwr, cB, lw, wr,wg,wb, 0.50f*la);
        LogoLine(canvasLeft, cB, canvasLeft, cB-cwr, lw, wr,wg,wb, 0.50f*la);
        LogoLine(cR, cB, cR-cwr, cB, lw, wr,wg,wb, 0.50f*la);
        LogoLine(cR, cB, cR, cB-cwr, lw, wr,wg,wb, 0.50f*la);
        drawDiamond(canvasLeft, canvasTop, 3.0f*uiS, wr,wg,wb, 0.65f*la);
        drawDiamond(cR,         canvasTop, 3.0f*uiS, wr,wg,wb, 0.65f*la);
        drawDiamond(canvasLeft, cB,        3.0f*uiS, wr,wg,wb, 0.65f*la);
        drawDiamond(cR,         cB,        3.0f*uiS, wr,wg,wb, 0.65f*la);
    }

    // 헤더 라인
    DrawShadowedText(g_TextS, L"STAR CHART // RUN PREPARATION",
                     canvasLeft + 8.0f * uiS, canvasTop + 12.0f * uiS, 0.50f * uiS,
                     wr, wg, wb, 0.80f * contentA, 0.60f);
    {
        const wchar_t* escL = L"[ ESC / RMB ] BACK";
        const float ew = g_TextS.Width(escL, 0.44f * uiS);
        DrawShadowedText(g_TextS, escL, canvasRight - ew - 10.0f * uiS,
                         canvasTop + 14.0f * uiS, 0.44f * uiS,
                         0.66f, 0.75f, 0.84f, 0.68f * contentA, 0.55f);
    }
    LogoLine(canvasLeft + 8.0f * uiS, contentTop - 12.0f * uiS,
             canvasRight - 8.0f * uiS, contentTop - 12.0f * uiS,
             0.8f * uiS, wr, wg, wb, 0.16f * contentA);

    // 컬럼 라벨
    DrawShadowedText(g_TextS, L"WEAPON STARS", canvasLeft + 8.0f * uiS, contentTop,
                     0.42f * uiS, wr, wg, wb, 0.62f * contentA, 0.50f);
    DrawShadowedText(g_TextS, L"SHACKLE ORBIT", stripX, contentTop,
                     0.42f * uiS, tr, tg, tb, 0.62f * contentA, 0.50f);
    DrawShadowedText(g_TextS, L"RUN SUMMARY", sumX, contentTop,
                     0.42f * uiS, wr, wg, wb, 0.62f * contentA, 0.50f);

    // ── 좌 컬럼: 무기 성좌 ────────────────────────────────────────────
    const float starY[2] = { centerY - 165.0f * uiS, centerY + 165.0f * uiS };
    // 별 사이 연결선 — 얇은 1px, 느린 파동 알파
    {
        const float linkA = (0.14f + 0.08f * sinf(now * 1.15f)) * contentA;
        LogoLine(starX, starY[0], starX, starY[1], 1.0f * uiS,
                 0.48f, 0.82f, 1.0f, linkA);
    }
    for (int i = 0; i < 2; ++i) {
        const bool isSel = (weapon == i);
        const bool hov = ready && mx >= starX - 26.0f * uiS && mx <= starX + 200.0f * uiS
            && my >= starY[i] - 26.0f * uiS && my <= starY[i] + 26.0f * uiS;
        weaponHover[i] = UpdateMenuCommandHover(weaponHover[i], hov, dt);
        if (hov && lmb && !g_LmbPrev && weapon != i) weapon = i;
        const float nr = i == 0 ? 0.22f : 1.00f;
        const float ng = i == 0 ? 0.90f : 0.72f;
        const float nb = i == 0 ? 1.00f : 0.22f;
        const float reveal = MenuCommandReveal(entry, i) * contentA;
        if (isSel) {
            // 글로우는 선택된 별에만
            DrawConstellationDisc(starX, starY[i], (30.0f + 4.0f * weaponHover[i]) * uiS,
                                  nr, ng, nb, 0.10f * reveal);
            DrawConstellationDisc(starX, starY[i], 16.0f * uiS, nr, ng, nb, 0.20f * reveal);
            DrawVisibleConstellNode(starX, starY[i], (7.5f + weaponHover[i]) * uiS,
                                    nr, ng, nb, 0.95f * reveal);
        } else {
            DrawVisibleConstellNode(starX, starY[i], (5.5f + 0.8f * weaponHover[i]) * uiS,
                                    0.55f, 0.65f, 0.80f,
                                    (0.45f + 0.25f * weaponHover[i]) * reveal, false);
        }
        const float nameA = isSel ? 0.96f : (0.42f + 0.30f * weaponHover[i]);
        const float subA  = isSel ? 0.70f : (0.30f + 0.22f * weaponHover[i]);
        DrawShadowedText(g_TextL, weapons[i], starX + 30.0f * uiS, starY[i] - 18.0f * uiS,
                         0.72f * uiS, 1.0f, 1.0f, 1.0f, nameA * reveal, 0.70f);
        DrawShadowedText(g_TextS, weaponSub[i], starX + 30.0f * uiS, starY[i] + 10.0f * uiS,
                         0.38f * uiS, 0.70f, 0.78f, 0.88f, subA * reveal, 0.55f);
    }

    // 선택된 무기 별 주변 — 6축 스탯 방사형 (기존 statT 모핑 유지)
    {
        const float detailA = contentA;
        const float chartCX = starX;
        const float chartCY = starY[weapon];
        const float chartR  = std::min(cw * 0.068f, cH * 0.110f);
        const float nodeOrbit = chartR + 30.0f * uiS;
        static const wchar_t* kNL[6] = { L"", L"", L"", L"", L"", L"" };
        DrawWeaponPowerConstellation(chartCX, chartCY, chartR,
                                     statT, kNL, kNL, weapon,
                                     now, 0.45f * detailA, uiS, conR, conG, conB);
        for (int i = 0; i < 6; ++i) {
            const float ang     = -kPi * 0.5f + (2.0f * kPi * (float)i / 6.0f);
            const float twinkle = 0.86f + 0.14f * sinf(now * 2.2f + (float)i * 1.3f);
            const float sv      = LogoClamp01(statT[i]);
            const float nx      = chartCX + cosf(ang) * nodeOrbit;
            const float ny      = chartCY + sinf(ang) * nodeOrbit;
            const float sz      = (2.8f + sv * 3.8f) * uiS * twinkle;
            const float innerX  = chartCX + cosf(ang) * chartR * (0.22f + 0.78f * sv);
            const float innerY  = chartCY + sinf(ang) * chartR * (0.22f + 0.78f * sv);
            LogoLine(innerX, innerY, nx, ny, 0.5f * uiS, conR, conG, conB, 0.14f * detailA);
            drawDiamond(nx, ny, sz, conR, conG, conB, (0.68f + 0.30f * sv) * twinkle * detailA);
            drawDiamond(nx, ny, sz * 0.38f, 0.95f, 1.0f, 1.0f, 0.78f * detailA);
            const float ld  = sz * 2.2f + 6.0f * uiS;
            const float lax = nx + cosf(ang) * ld;
            const float lay = ny + sinf(ang) * ld;
            const float nscl = 0.48f * uiS;
            const float vscl = 0.58f * uiS;
            const float nw   = g_TextS.Width(statNames[i], nscl);
            const float vw   = g_TextS.Width(statVals[weapon][i], vscl);
            float tx;
            if      (cosf(ang) < -0.28f) tx = lax - std::max(nw, vw) - 2.0f * uiS;
            else if (cosf(ang) >  0.28f) tx = lax + 2.0f * uiS;
            else                          tx = lax - nw * 0.5f;
            float ty = lay - 12.0f * uiS;
            if      (sinf(ang) >  0.28f) ty = lay + 2.0f * uiS;
            else if (sinf(ang) < -0.28f) ty = lay - 24.0f * uiS;
            const float vtx = (cosf(ang) < -0.28f) ? lax - vw - 2.0f * uiS
                            : (cosf(ang) >  0.28f) ? lax + 2.0f * uiS
                            :                         lax - vw * 0.5f;
            DrawShadowedText(g_TextS, statNames[i], tx, ty, nscl,
                             0.72f, 0.84f, 0.96f, 0.85f * detailA, 0.55f);
            DrawShadowedText(g_TextS, statVals[weapon][i], vtx, ty + 16.0f * uiS, vscl,
                             conR * 0.25f + 0.75f, conG * 0.25f + 0.75f, conB * 0.25f + 0.75f,
                             0.96f * detailA, 0.60f);
        }
    }

    // BACK (좌측 커맨드 레일)
    const float weaponH = 64.0f * uiS;
    const bool backHov = ready && mx >= leftX - 18.0f && mx < leftX + leftW + 18.0f
        && my >= leftY && my < leftY + weaponH;
    backHover = UpdateMenuCommandHover(backHover, backHov, dt);
    DrawUnifiedMenuCommand(L"BACK", L"", leftX - 10.0f * backHover, leftY,
                           leftW, weaponH, 0.48f, 0.82f, 1.0f,
                           MenuCommandReveal(entry, 2) * contentA,
                           backHover, false, 0.0f, now + 0.46f, 0.90f, 0.42f);
    if (backHov && lmb && !g_LmbPrev) exiting = true;

    // ── 중앙: 굴레 궤도 띠 (가로 스크롤) ──────────────────────────────
    const float stripY  = centerY - 75.0f * uiS;
    const float stripH  = 150.0f * uiS;
    const float stripMid= stripY + stripH * 0.5f;
    const float chipW   = 148.0f * uiS, chipGap = 16.0f * uiS, chipH = 116.0f * uiS;
    const float chipY   = stripY + (stripH - chipH) * 0.5f;
    const float totalW  = 6.0f * chipW + 5.0f * chipGap;
    const float maxOff  = std::max(0.0f, totalW - stripW);

    // 휠 스크롤
    const bool overStrip = ready && mx >= stripX && mx <= stripX + stripW
                        && my >= stripY - 8.0f * uiS && my <= stripY + stripH + 8.0f * uiS;
    if (overStrip && g_ScrollAccum != 0.0f)
        stripOffset -= g_ScrollAccum * 46.0f * uiS;
    g_ScrollAccum = 0.0f;

    // 좌우 화살표
    const float arrowW = 30.0f * uiS;
    const float arY  = stripMid - arrowW * 0.5f;
    const float arX0 = stripX - arrowW - 10.0f * uiS;
    const float arX1 = stripX + stripW + 10.0f * uiS;
    const bool canL = stripOffset > 0.5f;
    const bool canR = stripOffset < maxOff - 0.5f;
    const bool hovAL = ready && canL && mx >= arX0 && mx <= arX0 + arrowW
                    && my >= arY && my <= arY + arrowW;
    const bool hovAR = ready && canR && mx >= arX1 && mx <= arX1 + arrowW
                    && my >= arY && my <= arY + arrowW;
    arrowHover[0] = UpdateMenuCommandHover(arrowHover[0], hovAL, dt);
    arrowHover[1] = UpdateMenuCommandHover(arrowHover[1], hovAR, dt);
    if (hovAL && lmb && !g_LmbPrev) stripOffset -= (chipW + chipGap);
    if (hovAR && lmb && !g_LmbPrev) stripOffset += (chipW + chipGap);

    // 드래그 스크롤 (+ 짧은 클릭은 칩 토글)
    if (ready && lmb && !g_LmbPrev && overStrip) {
        stripDrag = true; dragStartX = (float)mx; dragStartOff = stripOffset; dragMoved = 0.0f;
    }
    int clickChip = -1;
    if (stripDrag) {
        if (lmb) {
            const float dx = (float)mx - dragStartX;
            dragMoved = std::max(dragMoved, fabsf(dx));
            stripOffset = dragStartOff - dx;
        } else {
            stripDrag = false;
            if (dragMoved < 6.0f) {
                for (int i = 0; i < 6; ++i) {
                    const float cx0 = stripX + i * (chipW + chipGap) - stripOffset;
                    if (mx >= cx0 && mx <= cx0 + chipW && my >= chipY && my <= chipY + chipH)
                        clickChip = i;
                }
            }
        }
    }
    if (clickChip >= 0) {
        if (clickChip + 1 == s_TrialTargetCount) s_TrialTargetCount = clickChip;
        else                                     s_TrialTargetCount = clickChip + 1;
    }
    stripOffset = std::max(0.0f, std::min(stripOffset, maxOff));

    // 궤도 가이드 라인 — 느린 파동 알파
    LogoLine(stripX, stripMid, stripX + stripW, stripMid, 1.0f * uiS,
             tr, tg, tb, (0.13f + 0.07f * sinf(now * 1.05f)) * contentA);
    {
        wchar_t cntBuf[16];
        swprintf_s(cntBuf, L"%d / 6", s_TrialTargetCount);
        const float cw2 = g_TextS.Width(cntBuf, 0.46f * uiS);
        DrawShadowedText(g_TextS, cntBuf, stripX + stripW - cw2, contentTop,
                         0.46f * uiS, 1.0f, 1.0f, 1.0f, 0.80f * contentA, 0.60f);
        DrawShadowedText(g_TextS, L"클릭으로 개수 선택 · 시작 시 무작위 부여",
                         stripX, stripY + stripH + 12.0f * uiS, 0.38f * uiS,
                         0.70f, 0.78f, 0.88f, 0.55f * contentA, 0.50f);
    }

    // 칩 렌더링 — 띠 영역 클리핑
    for (int i = 0; i < 6; ++i) {
        const float cx0 = stripX + i * (chipW + chipGap) - stripOffset;
        const bool hov = ready && !stripDrag
            && mx >= std::max(cx0, stripX) && mx <= std::min(cx0 + chipW, stripX + stripW)
            && my >= chipY && my <= chipY + chipH;
        chipHover[i] = UpdateMenuCommandHover(chipHover[i], hov, dt);
    }
    BatchFlush();
    glEnable(GL_SCISSOR_TEST);
    {
        int scX = (int)stripX;
        int scY = (int)(sh - stripY - stripH);
        int scW = (int)stripW;
        int scH = (int)stripH;
        if (scX < 0) scX = 0; if (scY < 0) scY = 0;
        glScissor(scX, scY, scW, scH);
    }
    // 점등된 칩 사이 연결선
    for (int i = 1; i < 6; ++i) {
        if (i >= trialCount) break;
        const float nx0 = stripX + (i - 1) * (chipW + chipGap) + chipW * 0.5f - stripOffset;
        const float nx1 = stripX + i * (chipW + chipGap) + chipW * 0.5f - stripOffset;
        LogoLine(nx0, chipY + 24.0f * uiS, nx1, chipY + 24.0f * uiS, 1.0f * uiS,
                 tr, tg, tb, (0.28f + 0.10f * sinf(now * 1.3f + (float)i)) * contentA);
    }
    for (int i = 0; i < 6; ++i) {
        const float cx0 = stripX + i * (chipW + chipGap) - stripOffset;
        const float hov = chipHover[i];
        const bool lit = i < trialCount;
        const float nx = cx0 + chipW * 0.5f;
        const float ny = chipY + 24.0f * uiS;
        drawRect(cx0, chipY, chipW, chipH, tr, tg, tb,
                 (lit ? 0.09f : 0.02f + 0.05f * hov) * contentA);
        const float bA = (lit ? 0.48f : 0.14f + 0.22f * hov) * contentA;
        LogoLine(cx0,        chipY,         cx0 + chipW, chipY,         0.7f * uiS, tr, tg, tb, bA);
        LogoLine(cx0,        chipY + chipH, cx0 + chipW, chipY + chipH, 0.7f * uiS, tr, tg, tb, bA);
        LogoLine(cx0,        chipY,         cx0,         chipY + chipH, 0.7f * uiS, tr, tg, tb, bA);
        LogoLine(cx0 + chipW, chipY,        cx0 + chipW, chipY + chipH, 0.7f * uiS, tr, tg, tb, bA);
        if (lit) {
            const float pulse = 0.5f + 0.5f * sinf(now * 2.6f + (float)i * 1.1f);
            DrawConstellationDisc(nx, ny, (16.0f + 4.0f * pulse) * uiS, tr, tg, tb, 0.14f * contentA);
            drawDiamond(nx, ny, (7.0f + pulse) * uiS, tr, tg, tb, 0.92f * contentA);
            drawDiamond(nx, ny, 2.6f * uiS, 1.0f, 1.0f, 0.9f, 0.96f * contentA);
        } else {
            drawDiamond(nx, ny, (5.5f + hov) * uiS, 0.65f, 0.74f, 0.86f,
                        (0.40f + 0.30f * hov) * contentA);
            drawDiamond(nx, ny, 2.0f * uiS, 0.90f, 0.95f, 1.0f, 0.34f * contentA);
        }
        const float nameSc = 0.44f * uiS;
        const float nameW  = g_TextS.Width(trialNames[i], nameSc);
        DrawShadowedText(g_TextS, trialNames[i], nx - nameW * 0.5f, chipY + 52.0f * uiS, nameSc,
                         1.0f, 1.0f, 1.0f,
                         (lit ? 0.92f : 0.40f + 0.30f * hov) * contentA, 0.60f);
        const float descSc = 0.36f * uiS;
        const float descW  = g_TextS.Width(trialDescs[i], descSc);
        DrawShadowedText(g_TextS, trialDescs[i], nx - descW * 0.5f, chipY + 78.0f * uiS, descSc,
                         lit ? tr : 0.60f, lit ? tg : 0.68f, lit ? tb : 0.78f,
                         (lit ? 0.78f : 0.44f) * contentA, 0.55f);
    }
    BatchFlush(); glDisable(GL_SCISSOR_TEST);

    // 화살표 렌더링
    for (int a = 0; a < 2; ++a) {
        const float ax = a == 0 ? arX0 : arX1;
        const bool can = a == 0 ? canL : canR;
        const float hov = arrowHover[a];
        const float aA = can ? (0.45f + 0.40f * hov) * contentA : 0.14f * contentA;
        drawRect(ax, arY, arrowW, arrowW, tr, tg, tb, (0.04f + 0.08f * hov) * contentA * (can ? 1.0f : 0.4f));
        const wchar_t* glyph = a == 0 ? L"<" : L">";
        const float gSc = 0.72f * uiS;
        const float gw = g_TextL.Width(glyph, gSc);
        const float gh = g_TextL.Height(glyph, gSc);
        DrawShadowedText(g_TextL, glyph, ax + (arrowW - gw) * 0.5f, arY + (arrowW - gh) * 0.5f,
                         gSc, tr, tg, tb, aA, 0.55f);
    }

    // 선택 별 → 궤도 띠 → LAUNCH 별 수렴 연결선
    const float playW = std::min(280.0f * uiS, sumW);
    const float playH = 82.0f * uiS;
    const float playX = sumX + (sumW - playW) * 0.5f;
    const float playY = canvasBottom - playH - 14.0f * uiS;
    {
        const float convA = (0.13f + 0.07f * sinf(now * 1.05f)) * contentA;
        LogoLine(starX + 20.0f * uiS, starY[weapon], stripX, stripMid,
                 1.0f * uiS, conR, conG, conB, convA);
        const float convB = trialCount > 0
            ? (0.20f + 0.09f * sinf(now * 1.05f + 1.3f)) * contentA
            : 0.08f * contentA;
        LogoLine(stripX + stripW, stripMid, playX + playW * 0.5f, playY + playH * 0.5f,
                 1.0f * uiS,
                 trialCount > 0 ? tr : 0.48f,
                 trialCount > 0 ? tg : 0.82f,
                 trialCount > 0 ? tb : 1.0f, convB);
    }

    // ── 우 컬럼: 런 요약 + LAUNCH ──────────────────────────────────────
    {
        const float rowY0 = contentTop + 30.0f * uiS;
        const float rowStep = 52.0f * uiS;
        wchar_t shackBuf[16];
        if (trialCount > 0) swprintf_s(shackBuf, L"+%d", trialCount);
        else                wcscpy_s (shackBuf, L"—");
        const wchar_t* sumLabel[3] = { L"WEAPON", L"SHACKLES", L"DIFFICULTY" };
        const wchar_t* sumValue[3] = { weapons[weapon], shackBuf, L"NORMAL" };
        for (int i = 0; i < 3; ++i) {
            const float ry = rowY0 + i * rowStep;
            DrawShadowedText(g_TextS, sumLabel[i], sumX, ry, 0.38f * uiS,
                             0.62f, 0.74f, 0.86f, 0.58f * contentA, 0.45f);
            const float vSc = 0.56f * uiS;
            const float vW = g_TextS.Width(sumValue[i], vSc);
            const float vR = i == 0 ? conR * 0.25f + 0.75f : (i == 1 && trialCount > 0 ? tr : 0.92f);
            const float vG = i == 0 ? conG * 0.25f + 0.75f : (i == 1 && trialCount > 0 ? tg : 0.96f);
            const float vB = i == 0 ? conB * 0.25f + 0.75f : (i == 1 && trialCount > 0 ? tb : 1.0f);
            DrawShadowedText(g_TextS, sumValue[i], sumX + sumW - vW, ry - 4.0f * uiS, vSc,
                             vR, vG, vB, 0.92f * contentA, 0.62f);
            LogoLine(sumX, ry + rowStep - 16.0f * uiS, sumX + sumW, ry + rowStep - 16.0f * uiS,
                     0.6f * uiS, wr, wg, wb, 0.12f * contentA);
        }
    }
    // 기록 — 요약 아래, LAUNCH 위
    {
        static const wchar_t* recLabels[4] = { L"BEST SCORE", L"BEST KILLS", L"TOTAL KILLS", L"TOTAL RUNS" };
        const long long recData[4] = {
            g_WeaponBestScore[weapon], g_WeaponBestKills[weapon],
            g_WeaponTotalKills[weapon], g_WeaponRunCount[weapon]
        };
        const float recY0 = playY - 116.0f * uiS;
        LogoLine(sumX, recY0 - 6.0f * uiS, sumX + sumW, recY0 - 6.0f * uiS,
                 0.6f * uiS, conR, conG, conB, 0.17f * contentA);
        for (int i = 0; i < 4; ++i) {
            const float ry = recY0 + (float)i * 23.0f * uiS;
            DrawShadowedText(g_TextS, recLabels[i], sumX, ry, 0.36f * uiS,
                             0.62f, 0.74f, 0.86f, 0.60f * contentA, 0.40f);
            wchar_t vbuf[32];
            if (recData[i] == 0) wcscpy_s(vbuf, L"—");
            else                 swprintf_s(vbuf, L"%lld", recData[i]);
            const float vw2 = g_TextS.Width(vbuf, 0.42f * uiS);
            DrawShadowedText(g_TextS, vbuf, sumX + sumW - vw2, ry,
                             0.42f * uiS, 0.92f, 0.96f, 1.0f, 0.80f * contentA, 0.50f);
        }
    }

    // LAUNCH 버튼 — 기존 코너라인 + 다이아몬드 장식 계승
    const bool playHov = ready && mx >= playX && mx < playX + playW && my >= playY && my < playY + playH;
    playHover = UpdateMenuCommandHover(playHover, playHov, dt);
    {
        const float fa = (0.56f + 0.28f * playHover) * contentA;
        const float fi = (0.10f + 0.12f * playHover) * contentA;
        const float cw3 = 22.0f * uiS;
        drawRect(playX, playY, playW, playH, wr, wg, wb, fi);
        LogoLine(playX,           playY,          playX + cw3,        playY,          1.1f*uiS, wr,wg,wb, fa);
        LogoLine(playX,           playY,          playX,              playY + cw3,    1.1f*uiS, wr,wg,wb, fa);
        LogoLine(playX + playW,   playY,          playX+playW - cw3,  playY,          1.1f*uiS, wr,wg,wb, fa);
        LogoLine(playX + playW,   playY,          playX+playW,        playY + cw3,    1.1f*uiS, wr,wg,wb, fa);
        LogoLine(playX,           playY + playH,  playX + cw3,        playY + playH,  1.1f*uiS, wr,wg,wb, fa);
        LogoLine(playX,           playY + playH,  playX,              playY+playH-cw3,1.1f*uiS, wr,wg,wb, fa);
        LogoLine(playX + playW,   playY + playH,  playX+playW - cw3,  playY + playH,  1.1f*uiS, wr,wg,wb, fa);
        LogoLine(playX + playW,   playY + playH,  playX+playW,        playY+playH-cw3,1.1f*uiS, wr,wg,wb, fa);
        drawDiamond(playX + playW * 0.5f, playY,         3.5f*uiS, wr,wg,wb, fa * 0.72f);
        drawDiamond(playX + playW * 0.5f, playY + playH, 3.5f*uiS, wr,wg,wb, fa * 0.72f);
    }
    {
        wchar_t playLabel[32];
        if (trialCount > 0) swprintf_s(playLabel, L"LAUNCH  +%d", trialCount);
        else                wcscpy_s (playLabel, L"LAUNCH");
        const float tScale = 0.92f * uiS;
        const float tW = g_TextL.Width(playLabel, tScale);
        const float tH = g_TextL.Height(playLabel, tScale);
        DrawShadowedText(g_TextL, playLabel,
                         playX + (playW - tW) * 0.5f, playY + (playH - tH) * 0.5f, tScale,
                         1.0f + (wr - 1.0f) * playHover, 1.0f + (wg - 1.0f) * playHover, 1.0f,
                         (0.92f + 0.08f * playHover) * contentA, 0.86f);
    }
    if (playHov && lmb && !g_LmbPrev && ready) {
        playExit = true;
        playT = 0.0f;
    }
    g_BatchAlpha = 1.0f;
    if (playExit && playT >= 0.50f) {
        g_SelectedJob = weapon == 0 ? JOB_NONE : JOB_VAMPIRE;
        s_RcWeapon = weapon;
        if (s_TrialTargetCount > 0) {
            // 무작위 굴레 배정
            int idx[6] = {0,1,2,3,4,5};
            for (int i = 5; i > 0; --i) {
                int j = rand() % (i + 1);
                std::swap(idx[i], idx[j]);
            }
            for (int i = 0; i < 6; ++i) s_RcTrialNodes[i] = false;
            for (int i = 0; i < s_TrialTargetCount; ++i) s_RcTrialNodes[idx[i]] = true;
            for (int i = 0; i < 6; ++i) s_RcTrialTypeT[i] = 0.0f;
            s_MainMenuRunConfigPanel   = false;
            s_MainMenuTrialSelectPanel = true;
        } else {
            if (g_CreativeMode) g_GameManager.currentState = GameState::CREATIVE_CONFIG;
            else { c.reset(); FinalizeLoadout(c, FixedWeaponForSelectedJob()); }
        }
        playExit = false;
        playT = 0.0f;
    }
    if (exiting && exitT >= 0.42f) {
        s_MainMenuRunConfigPanel = false;
        s_MainMenuResumeFromPanel = true;
        ResetRunConfigUi();
        g_MainMenuEntryT = 1.0f;
        entry = 0.0f;
        exiting = false;
        playExit = false;
        playT = 0.0f;
        g_GameManager.currentState = GameState::MAIN_MENU;
    }
}

static void Scene_TrialSelectInline(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh, dt = std::min(c.delta, 0.05f);
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    static float entry    = 0.0f;
    static float exitT    = 0.0f;
    static bool  exiting  = false;
    static bool  playExit = false;
    static float playT    = 0.0f;
    static bool  prevEsc  = false, prevRmb = false;
    static float backHover   = 0.0f;
    static float startHover  = 0.0f;
    static float rerollHover = 0.0f;
    static bool  wasActive   = false;
    static float flipT       = 0.0f;   // 0→1: constellation slide 진행
    static float slideFlipT  = 0.0f;  // lags behind flipT → ease-in
    static float statT[6]    = { 0.50f, 0.78f, 0.78f, 0.72f, 0.68f, 0.72f };
    static const float barTargets[2][6] = {
        { 0.50f, 0.78f, 0.78f, 0.72f, 0.68f, 0.72f },
        { 0.22f, 0.86f, 0.94f, 1.00f, 0.50f, 0.28f }
    };

    if (!wasActive) {
        entry = 0.0f; exitT = 0.0f; exiting = false;
        playExit = false; playT = 0.0f;
        prevEsc = false; prevRmb = false;
        backHover = startHover = rerollHover = 0.0f;
        flipT = 0.0f; slideFlipT = 0.0f;
        for (int i = 0; i < 6; ++i) s_RcNodeHover[i] = 0.0f;
        wasActive = true;
    }

    entry = std::min(1.0f, entry + dt);
    if (playExit) playT = std::min(0.50f, playT + dt);
    const bool esc = c.window && glfwGetKey(c.window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    const bool rmb = c.window && glfwGetMouseButton(c.window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    const bool backInput = (esc && !prevEsc) || (rmb && !prevRmb);
    prevEsc = esc; prevRmb = rmb;
    if (backInput && !exiting && entry >= 0.55f) exiting = true;
    if (exiting) exitT = std::min(0.42f, exitT + dt);
    const float oldOut      = Smoothstep(LogoClamp01(entry / 0.35f));
    const float contentIn   = Smoothstep(LogoClamp01((entry - 0.20f) / 0.35f));
    const float backP       = exiting ? Smoothstep(LogoClamp01(exitT / 0.42f)) : 0.0f;
    const float oldA        = exiting ? backP : 1.0f - oldOut;
    const float playCollapse= Smoothstep(LogoClamp01((playT - 0.30f) / 0.20f));
    const float contentA    = contentIn * (1.0f - backP) * (1.0f - playCollapse);
    const bool  ready       = !exiting && !playExit && entry >= 0.55f;
    SetSceneTextureReveal(exiting
        ? std::max(0.0f, 1.0f - backP)
        : Smoothstep(LogoClamp01((entry - 0.04f) / 0.74f)));
    const float now         = (float)glfwGetTime();

    const float uiS     = std::max(0.70f, std::min(sw * 0.94f / 1640.0f, sh * 0.90f / 910.0f));
    const float mainX   = std::max(58.0f, sw * 0.075f);
    const float mainY   = MainMenuCommandStartY(sh);
    const float mainBW  = std::min(560.0f, std::max(420.0f, sw * 0.34f));
    const float mainBH  = 70.0f, mainGap = 15.0f;
    const float leftX   = mainX + 82.0f * uiS * (1.0f - contentIn) + 180.0f * uiS * backP;
    const float leftW   = std::min(340.0f * uiS, mainBW * 0.70f);
    const float canvasLeft   = std::max(sw * 0.28f, leftX + leftW + 18.0f * uiS);
    const float canvasRight  = sw - std::max(32.0f, 36.0f * uiS);
    const float canvasTop    = std::max(74.0f, sh * 0.12f);
    const float canvasBottom = sh - std::max(88.0f, 104.0f * uiS);

    // Amber color for trial mode
    const float tr = 1.0f, tg = 0.60f, tb = 0.22f;
    // Weapon color (RunConfig 위치 기준)
    const float wr = s_RcWeapon == 0 ? 0.35f : 0.55f;
    const float wg = s_RcWeapon == 0 ? 0.72f : 0.90f;
    const float wb = 1.0f;
    // 무기별 고정 별자리 색상: RIFLE=시안, STATIC FIELD=주황
    const float conR = s_RcWeapon == 0 ? 0.22f : 1.00f;
    const float conG = s_RcWeapon == 0 ? 0.90f : 0.72f;
    const float conB = s_RcWeapon == 0 ? 1.00f : 0.22f;

    // Constellation: RunConfig 우측 → TrialSelect 좌측으로 ease-in 슬라이드
    flipT      = UiApproach(flipT,      1.0f, dt, 7.0f);
    slideFlipT = UiApproach(slideFlipT, flipT, dt, 3.5f); // lags → slow start, fast catch-up
    const float fromCX  = canvasLeft + (canvasRight - canvasLeft) * 0.76f; // RunConfig 위치
    const float consCX  = canvasLeft + (canvasRight - canvasLeft) * 0.34f; // 목표 위치
    const float animCX  = fromCX + (consCX - fromCX) * Smoothstep(slideFlipT);
    const float consCY  = canvasTop + (canvasBottom - canvasTop) * 0.44f;
    const float consR   = std::min((consCX - canvasLeft + (canvasRight - canvasLeft) * 0.12f),
                                    (canvasBottom - canvasTop) * 0.28f);
    // 색상도 무기 컬러 → 앰버로 함께 전환
    const float animR   = wr + (tr - wr) * Smoothstep(flipT);
    const float animG   = wg + (tg - wg) * Smoothstep(flipT);
    const float animB   = wb + (tb - wb) * Smoothstep(flipT);

    DrawPersistentSceneLeftVignette(sw, sh, 0.86f);

    // Ghost menu list
    static const wchar_t* menu[5] = { L"PLAY", L"SHOP", L"ASTRAL_LOG", L"SETTING", L"EXIT" };
    static const wchar_t* sub[5]  = { L"시작", L"상점", L"도감", L"설정", L"게임 종료" };
    for (int i = 0; i < 5; ++i) {
        const float y = mainY + i * (mainBH + mainGap);
        const float x = mainX - 250.0f * (1.0f - oldA) - 24.0f * (i != 0) * (1.0f - oldA);
        DrawShadowedText(g_TextL, menu[i], x, y + 2.0f, 1.04f,
                         1.0f, 1.0f, 1.0f, (i == 0 ? 0.82f : 0.18f) * oldA, 0.70f);
        DrawShadowedText(g_TextS, sub[i],  x + 4.0f, y + 43.0f, 0.48f,
                         0.72f, 0.77f, 0.84f, (i == 0 ? 0.74f : 0.22f) * oldA, 0.62f);
    }

    // Canvas scan-reveal
    {
        const float panelReveal = Smoothstep(LogoClamp01((entry - 0.22f) / 0.42f));
        const float cW = canvasRight - canvasLeft;
        const float cH = canvasBottom - canvasTop;
        drawRect(canvasLeft, canvasTop, cW, cH * panelReveal, 0.04f, 0.055f, 0.08f, 0.62f * contentA);
        if (panelReveal > 0.004f && panelReveal < 0.998f) {
            const float scanY    = canvasTop + cH * panelReveal;
            const float scanFade = 1.0f - Smoothstep(LogoClamp01((panelReveal - 0.80f) / 0.20f));
            drawRect(canvasLeft, scanY - 1.5f * uiS, cW, 3.0f * uiS,
                     tr, tg, tb, 0.58f * scanFade * contentA);
        }
        LogoLine(canvasLeft, canvasTop, canvasRight, canvasTop,
                 0.8f * uiS, tr, tg, tb, 0.28f * panelReveal * contentA);
    }

    // Header
    const float headerX = canvasLeft + 8.0f * uiS;
    const float headerY = canvasTop;
    DrawShadowedText(g_TextS, L"SHACKLE CONFIGURATION", headerX, headerY, 0.50f * uiS,
                     tr, tg, tb, 0.78f * contentA, 0.66f);
    DrawShadowedText(g_TextL, L"굴레", headerX, headerY + 30.0f * uiS, 1.05f * uiS,
                     1.0f, 1.0f, 1.0f, 0.98f * contentA, 0.78f);
    DrawShadowedText(g_TextS, L"부여된 굴레를 확인하세요  ·  리롤로 재배정 가능",
                     headerX, headerY + 72.0f * uiS, 0.46f * uiS,
                     0.70f, 0.78f, 0.88f, 0.82f * contentA, 0.66f);
    LogoLine(headerX, headerY + 98.0f * uiS,
             canvasLeft + (canvasRight - canvasLeft) * 0.60f, headerY + 98.0f * uiS,
             1.0f * uiS, tr, tg, tb, 0.22f * contentA);

    // Trial typewriter list — right portion
    static const wchar_t* trialNames[6] = {
        L"OVERWHELMING", L"RELENTLESS", L"FRAGILE",
        L"BLIND",        L"ACCELERATE", L"ELITE"
    };
    static const wchar_t* trialDescs[6] = {
        L"적 수 증가",       L"체력 회복 없음",    L"받는 피해 증가",
        L"시야 축소",        L"적 이동속도 증가",  L"엘리트만 등장"
    };
    const float listX  = canvasLeft + (canvasRight - canvasLeft) * 0.52f;
    const float listY0 = headerY + 110.0f * uiS;
    const float lineH  = 48.0f * uiS;
    int trialCount = 0, row = 0;
    for (int i = 0; i < 6; ++i) {
        if (s_RcTrialNodes[i]) {
            s_RcTrialTypeT[i] = std::min(s_RcTrialTypeT[i] + dt * 60.0f, 40.0f);
            ++trialCount;
        } else {
            s_RcTrialTypeT[i] = 0.0f;
        }
        if (!s_RcTrialNodes[i]) continue;
        wchar_t fullLine[64];
        swprintf_s(fullLine, L"%s : %s", trialNames[i], trialDescs[i]);
        int  showChars = std::min((int)wcslen(fullLine), (int)s_RcTrialTypeT[i]);
        wchar_t dispLine[64] = {};
        wcsncpy_s(dispLine, fullLine, showChars);
        float ly = listY0 + row * lineH;
        LogoLine(listX, ly + lineH - 5.0f * uiS,
                 listX + (canvasRight - listX) * 0.88f, ly + lineH - 5.0f * uiS,
                 0.7f * uiS, tr, tg, tb, 0.18f * contentA);
        wchar_t idxBuf[8]; swprintf_s(idxBuf, L"[%d]", row + 1);
        DrawShadowedText(g_TextS, idxBuf,
                         listX, ly + 4.0f * uiS, 0.50f * uiS, tr, tg, tb, 0.68f * contentA, 0.50f);
        DrawShadowedText(g_TextS, dispLine,
                         listX + 36.0f * uiS, ly + 4.0f * uiS, 0.60f * uiS,
                         1.0f, 1.0f, 1.0f, 0.94f * contentA, 0.62f);
        if (showChars < (int)wcslen(fullLine)) {
            float blinkA = 0.5f + 0.5f * sinf(now * 20.0f);
            float curX   = listX + 36.0f * uiS + g_TextS.Width(dispLine, 0.60f * uiS);
            DrawShadowedText(g_TextS, L"_", curX, ly + 4.0f * uiS, 0.60f * uiS,
                             0.48f, 0.82f, 1.0f, blinkA * contentA, 0.40f);
        }
        ++row;
    }
    if (trialCount == 0)
        DrawShadowedText(g_TextS, L"— 별자리 노드를 클릭해 굴레 추가 —",
                         listX, listY0 + 10.0f * uiS, 0.40f * uiS,
                         0.50f, 0.58f, 0.68f, 0.55f * contentA, 0.42f);

    // 별자리 + 노드 표시 (읽기 전용 — 클릭 불가)
    for (int i = 0; i < 6; ++i)
        statT[i] = UiApproach(statT[i], barTargets[s_RcWeapon][i], dt, 8.0f);
    DrawSceneRadialVignette(animCX, consCY,
                            std::max(260.0f * uiS, consR * 1.60f), 0.32f * contentA);
    {
        const float brightness = 0.35f + 0.50f * (trialCount / 6.0f);
        static const wchar_t* kNoLabel[6] = { L"", L"", L"", L"", L"", L"" };
        DrawWeaponPowerConstellation(animCX, consCY, consR,
                                     statT, kNoLabel, kNoLabel, s_RcWeapon,
                                     now, brightness * contentA, uiS, conR, conG, conB);
        const float kPi = 3.14159265f;
        for (int i = 0; i < 6; ++i) {
            const float angle = -kPi * 0.5f + i * kPi / 3.0f;
            const float nx = animCX + cosf(angle) * consR * 0.88f;
            const float ny = consCY  + sinf(angle) * consR * 0.88f;
            const float pulse = 0.5f + 0.5f * sinf(now * 3.2f + (float)i * 1.1f);
            if (s_RcTrialNodes[i]) {
                // 활성 노드 — 앰버 glow + 다이아
                DrawConstellationDisc(nx, ny, (28.0f + pulse * 10.0f) * uiS,
                                      tr, tg, tb, 0.10f * contentA);
                DrawConstellationDisc(nx, ny, 14.0f * uiS, tr, tg, tb, 0.20f * contentA);
                drawDiamond(nx, ny, (7.0f + pulse * 1.2f) * uiS, tr, tg, tb, 0.92f * contentA);
                drawDiamond(nx, ny, 2.8f * uiS, 1.0f, 1.0f, 0.9f, 0.96f * contentA);
            } else {
                // 비활성 노드 — 흰 윤곽선
                DrawConstellationDisc(nx, ny, 16.0f * uiS, 0.55f, 0.65f, 0.80f, 0.05f * contentA);
                drawDiamond(nx, ny, 5.5f * uiS, 0.65f, 0.74f, 0.86f, 0.52f * contentA);
                drawDiamond(nx, ny, 2.0f * uiS, 0.90f, 0.95f, 1.0f,  0.38f * contentA);
            }
        }
    }

    // BACK button (left command rail)
    {
        const bool backHov = ready && mx >= leftX - 18.0f && mx < leftX + leftW + 18.0f
                          && my >= mainY && my < mainY + mainBH;
        backHover = UpdateMenuCommandHover(backHover, backHov, dt);
        DrawUnifiedMenuCommand(L"BACK", L"무기 선택",
                               leftX - 10.0f * backHover, mainY, leftW, mainBH,
                               0.48f, 0.82f, 1.0f,
                               MenuCommandReveal(entry, 0) * contentA,
                               backHover, false, 0.0f, now, 0.90f, 0.42f);
        if (backHov && lmb && !g_LmbPrev) exiting = true;
    }

    // REROLL + START 수평 배치
    const float btnRowH  = 82.0f * uiS;
    const float btnRowY  = canvasBottom - btnRowH - 14.0f * uiS;
    const float btnRowCX = (canvasLeft + canvasRight) * 0.5f;
    const float gap3     = 12.0f * uiS;
    const float kStartW  = 240.0f * uiS;
    const float rrW2     = 160.0f * uiS;
    // 두 버튼 합산 너비로 중앙 정렬
    const float totalBW  = rrW2 + gap3 + kStartW;
    const float rrX2     = btnRowCX - totalBW * 0.5f;
    const float startX2  = rrX2 + rrW2 + gap3;

    // REROLL 버튼
    {
        const bool rrHov = UpdatePanelButtonHover(rerollHover, ready,
                                                  mx, my,
                                                  rrX2, btnRowY,
                                                  rrW2, btnRowH, dt,
                                                  8.0f * uiS);
        DrawPanelButton(L"REROLL", nullptr,
                        rrX2, btnRowY, rrW2, btnRowH,
                        tr, tg, tb, contentA,
                        rerollHover, false, 0.0f, now + 0.18f,
                        0.62f * uiS, 0.48f * uiS,
                        false, PanelButtonSlideSide::Both, true, true);
        if (rrHov && lmb && !g_LmbPrev && ready) {
            int idx[6] = {0,1,2,3,4,5};
            for (int i = 5; i > 0; --i) { int j = rand() % (i+1); std::swap(idx[i], idx[j]); }
            for (int i = 0; i < 6; ++i) s_RcTrialNodes[i] = false;
            for (int i = 0; i < s_TrialTargetCount; ++i) s_RcTrialNodes[idx[i]] = true;
            for (int i = 0; i < 6; ++i) s_RcTrialTypeT[i] = 0.0f;
        }
    }

    // START button
    const float startW  = 240.0f * uiS;
    const float startH  = btnRowH;
    const float startX  = startX2;
    const float startY  = btnRowY;
    const float startCX = startX + startW * 0.5f;
    {
        wchar_t trialSum[32];
        swprintf_s(trialSum, L"굴레 %d개 부여됨", s_TrialTargetCount);
        const float tl1 = g_TextS.Width(L"SHACKLE MODE", 0.36f * uiS);
        const float tl2 = g_TextS.Width(trialSum, 0.46f * uiS);
        DrawShadowedText(g_TextS, L"SHACKLE MODE",
                         btnRowCX - tl1 * 0.5f, btnRowY - 42.0f * uiS,
                         0.36f * uiS, tr, tg, tb, 0.60f * contentA, 0.42f);
        DrawShadowedText(g_TextS, trialSum,
                         btnRowCX - tl2 * 0.5f, btnRowY - 22.0f * uiS,
                         0.46f * uiS, 1.0f, 1.0f, 1.0f, 0.92f * contentA, 0.72f);
    }
    const bool startHov = UpdatePanelButtonHover(startHover, ready,
                                                 mx, my,
                                                 startX, startY,
                                                 startW, startH, dt,
                                                 8.0f * uiS);
    wchar_t startLabel[32];
    swprintf_s(startLabel, L"START  +%d", s_TrialTargetCount);
    DrawPanelButton(startLabel, nullptr,
                    startX, startY, startW, startH,
                    tr, tg, tb, contentA,
                    startHover, false,
                    startHov ? 0.22f : 0.0f, now + 0.42f,
                    0.92f * uiS, 0.48f * uiS,
                    false, PanelButtonSlideSide::Both, true, true);
    if (startHov && lmb && !g_LmbPrev && ready) {
        playExit = true;
        playT    = 0.0f;
    }
    g_BatchAlpha = 1.0f;
    if (playExit && playT >= 0.50f) {
        g_SelectedJob = s_RcWeapon == 0 ? JOB_NONE : JOB_VAMPIRE;
        wasActive = false;
        s_MainMenuTrialSelectPanel = false;
        if (g_CreativeMode) g_GameManager.currentState = GameState::CREATIVE_CONFIG;
        else { c.reset(); FinalizeLoadout(c, FixedWeaponForSelectedJob()); }
        playExit = false;
        playT    = 0.0f;
    }
    if (exiting && exitT >= 0.42f) {
        wasActive = false;
        s_MainMenuTrialSelectPanel = false;
        s_MainMenuRunConfigPanel   = true;
        exiting = false;
        exitT   = 0.0f;
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
        Language language = Language::KR;
    };
    static SettingsSnapshot savedSettings = {};
    static bool savedSettingsValid = false;
    static bool confirmBack = false;
    static bool confirmAbandon = false;
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
                          g_AutoFire, g_AutoSkill, g_ShowCrosshair, g_Language };
        savedSettingsValid = true;
        confirmBack = false;
        confirmAbandon = false;
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
            || g_Language != savedSettings.language;
    };

    const bool esc = c.window && glfwGetKey(c.window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    const bool rmb = c.window && glfwGetMouseButton(c.window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    const bool backInput = (esc && !prevEsc) || (rmb && !prevRmb);
    prevEsc = esc; prevRmb = rmb;
    const float entryOldOut = Smoothstep(LogoClamp01(entry / 0.35f));
    const float entryTreeIn = Smoothstep(LogoClamp01((entry - 0.20f) / 0.35f));
    if (backInput && !exiting && !confirmAbandon && entry >= 0.55f) {
        if (settingsDirty) confirmBack = true;
        else exiting = true;
    }
    if (exiting) exitT = std::min(0.42f, exitT + dt);
    const float outP = exiting ? Smoothstep(LogoClamp01(exitT / 0.42f)) : 0.0f;
    const float treeA = entryTreeIn * (1.0f - outP);
    const float oldA = exiting ? outP : (1.0f - entryOldOut);
    const bool ready = !exiting && entry >= 0.55f;
    const bool inputReady = ready && !confirmBack && !confirmAbandon;
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
            const float y = mainY + i * (mainBH + mainGap);
            const bool focus = i == 3;
            const float x = mainX - 250.0f * (1.0f - oldA) - (focus ? 0.0f : 24.0f * (1.0f - oldA));
            const float a = (focus ? 0.82f : 0.34f) * oldA;
            DrawShadowedText(g_TextL, menu[i], x, y + 2.0f, 1.04f,
                             1.0f, 1.0f, 1.0f, a, 0.70f);
            DrawShadowedText(g_TextS, menuSub[i], x + 4.0f, y + 43.0f, 0.48f,
                             0.72f, 0.77f, 0.84f, a * 0.9f, 0.62f);
        }
    } else {
        const wchar_t* menu[4] = { korean ? L"재개" : L"RESUME",
                                   korean ? L"설정" : L"CALIBRATION",
                                   korean ? L"\uD3EC\uAE30\uD558\uAE30" : L"ABANDON RUN",
                                   korean ? L"종료" : L"TERMINATE" };
        static const wchar_t* menuSub[4] = { L"\uC7AC\uAC1C", L"\uC124\uC815", L"\uB7F0 \uD3EC\uAE30", L"\uAC8C\uC784 \uC885\uB8CC" };
        for (int i = 0; i < 4; ++i) {
            const float y = mainY + i * (mainBH + mainGap);
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
#if 0
    const float orbitCX = mainX - 68.0f * uiS;
    const float orbitCY = sh * 0.56f;
    const float orbitRX = std::min(236.0f * uiS, sw * 0.145f);
    const float orbitRY = sh * 0.39f;
    const float orbitStart = -1.16f;
    const float orbitEnd = 1.16f;
    const float orbitR = 0.34f;
    const float orbitG = 0.72f;
    const float orbitB = 1.00f;
    DrawSettingsOrbitArc(orbitCX, orbitCY, orbitRX, orbitRY,
                 orbitStart, orbitEnd, orbitR, orbitG, orbitB,
                 0.78f * uiS, 0.0f);
    DrawSettingsOrbitArc(orbitCX, orbitCY, orbitRX * 0.87f, orbitRY * 0.87f,
                 orbitStart + 0.08f, orbitEnd - 0.08f,
                 orbitR, orbitG, orbitB,
                 0.48f * uiS, 0.0f);

    for (int i = 0; i < 5; ++i) {
        const float t = (float)i / 4.0f;
        const float angle = orbitStart + (orbitEnd - orbitStart) * t;
        const float nodeX = orbitCX + cosf(angle) * orbitRX;
        const float nodeY = orbitCY + sinf(angle) * orbitRY;
        const float nodeSize = 5.4f * uiS;
        const bool hov = false;
        rootHover[i] = UpdateMenuCommandHover(rootHover[i], hov, dt);
        if (hov && lmb && !g_LmbPrev) {
            if (i == 4) {
                exiting = true;
            } else if (i != tab) {
                tab = i;
                holdT = 0.0f;
                pulse = 1.0f;
                pulseRow = -1;
                tabSwitchT = 0.0f;
                for (auto& h : rowHover) h = 0.0f;
                for (auto& row : optHover) for (auto& h : row) h = 0.0f;
            }
        }
        const bool selected = (i == tab && i < 4);
        const float revealT = MenuCommandReveal(entry, i);
        const float rowA    = 0.0f;
        const float x       = nodeX + 18.0f * uiS - 8.0f * rootHover[i];
        const float cFlash  = (i == tab && pulse > 0.0f && i < 4) ? pulse : 0.0f;
        const float nodeR = selected ? 0.34f : orbitR;
        const float nodeG = selected ? 0.92f : orbitG;
        const float nodeB = selected ? 1.00f : orbitB;
        DrawVisibleConstellNode(nodeX, nodeY,
                                nodeSize * (selected ? 1.24f : 0.92f),
                                nodeR, nodeG, nodeB,
                                rowA * (selected ? 1.0f : 0.64f),
                                true, true);
        DrawUnifiedMenuCommand(tabs[i].en, tabs[i].kr, x,
                               nodeY - rootH * 0.42f, rootW, rootH,
                               nodeR, nodeG, nodeB, rowA,
                               rootHover[i], selected,
                               cFlash, now + (float)i * 0.17f,
                               0.94f, 0.46f, true);
    }

    // Connector: active tab \u2192 right panel header
    // The former orbit block is kept disabled while the integrated catalogue
    // below owns all category navigation.
#endif
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
    const int rowCounts[4] = { 4, 5, 3, 2 };
    // Kept as a compatibility buffer for the legacy single-row block below;
    // the integrated board reads settingsRows instead.
    SRow rows[6] = {};
    int rowCount = 0;

    static wchar_t s_volBuf[8];
    static wchar_t s_scoreBuf[24];

    if (tab == 0) { // DISPLAY (5행)
        const int fpsCur =
            g_FpsCap ==   0 ? 0 : g_FpsCap ==  30 ? 1 :
            g_FpsCap ==  60 ? 2 : g_FpsCap == 144 ? 3 :
            g_FpsCap == 300 ? 4 : 5;
        rows[0] = { L"FPS CAP",    nullptr, false, false, false, false,
                    { L"VSYNC", L"30", L"60", L"144", L"300", L"UNLIM" }, 6, fpsCur };
        rows[1] = { L"GRAPHICS",   nullptr, false, false, false, false,
                    { L"FULL", L"REDUCED" }, 2, g_VfxDensity == VfxDensity::FULL ? 0 : 1 };
        rows[2] = { L"COMBO HUD",  nullptr, false, false, false, false,
                    { L"ON", L"OFF" }, 2, g_ShowCombo ? 0 : 1 };
        rows[3] = { L"BACKDROP BLUR", nullptr, false, false, false, false,
                    { L"OFF", L"ON" }, 2, g_BackdropBlurEnabled ? 1 : 0 };
        rowCount = 4;
    } else if (tab == 1) { // AUDIO
        swprintf_s(s_volBuf, L"%d", g_SoundVol);
        rows[0] = { L"MASTER VOL", s_volBuf, false, true, false, false };
        rows[1] = { L"BGM BUS",     L"ACTIVE",          true, false, false, false };
        rows[2] = { L"SFX BUS",     L"ACTIVE",          true, false, false, false };
        rows[3] = { L"OUTPUT",      L"STEREO",          true, false, false, false };
        rows[4] = { L"AUDIO ENGINE",L"MINIAUDIO",       true, false, false, false };
        rowCount = 5;
    } else if (tab == 2) { // GAMEPLAY
        rows[0] = { L"AUTO FIRE",  nullptr, false, false, false, false,
                    { L"ON", L"OFF" }, 2, g_AutoFire ? 0 : 1 };
        rows[1] = { L"AUTO SKILL", nullptr, false, false, false, false,
                    { L"ON", L"OFF" }, 2, g_AutoSkill ? 0 : 1 };
        rows[2] = { L"CROSSHAIR",  nullptr, false, false, false, false,
                    { L"ON", L"OFF" }, 2, g_ShowCrosshair ? 0 : 1 };
        rowCount = 3;
    } else { // SYSTEM (tab == 3)
        const int langCur =
            g_Language == Language::KR ? 0 :
            g_Language == Language::EN ? 1 : 2;
        long long bestAny = g_BestScore[0];
        if (g_BestScore[1] > bestAny) bestAny = g_BestScore[1];
        if (g_BestScore[2] > bestAny) bestAny = g_BestScore[2];
        swprintf_s(s_scoreBuf, L"%lld", bestAny);
        rows[0] = { L"LANGUAGE",   nullptr, false, false, false, false,
                    { L"KOR", L"ENG", L"JPN" }, 3, langCur };
        rows[1] = { L"RESET DATA", nullptr, false, false, true, true };
        rowCount = 2;
    }

    const float optionStartX = rpX + 28.0f * uiS;
    const wchar_t* rowDescription[6] = {};
    if (tab == 0) {
        static const wchar_t* d[4] = {
            L"Frame pacing target",
            L"Scene detail density",
            L"Combat combo signal",
            L"Background detail filter"
        };
        for (int i = 0; i < 4; ++i) rowDescription[i] = d[i];
    } else if (tab == 1) {
        static const wchar_t* d[5] = {
            L"Master output level",
            L"Background music routing",
            L"Combat sound routing",
            L"Output channel layout",
            L"Active audio runtime"
        };
        for (int i = 0; i < 5; ++i) rowDescription[i] = d[i];
    } else if (tab == 2) {
        static const wchar_t* d[3] = {
            L"Automatic target fire",
            L"Automatic skill trigger",
            L"In-run aiming reticle"
        };
        for (int i = 0; i < 3; ++i) rowDescription[i] = d[i];
    } else {
        rowDescription[0] = L"Interface language";
        rowDescription[1] = L"Erase local progress";
    }

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
    settingsRows[1][1] = { korean ? L"BGM 채널" : L"BGM BUS", korean ? L"활성" : L"ACTIVE", true, false, false, false };
    settingsRows[1][2] = { korean ? L"효과음 채널" : L"SFX BUS", korean ? L"활성" : L"ACTIVE", true, false, false, false };
    settingsRows[1][3] = { korean ? L"출력" : L"OUTPUT", korean ? L"스테레오" : L"STEREO", true, false, false, false };
    settingsRows[1][4] = { korean ? L"오디오 엔진" : L"AUDIO ENGINE", L"MINIAUDIO", true, false, false, false };
    settingsRows[2][0] = { korean ? L"자동 발사" : L"AUTO FIRE", nullptr, false, false, false, false,
                           { korean ? L"켜짐" : L"ON", korean ? L"꺼짐" : L"OFF" }, 2, g_AutoFire ? 0 : 1 };
    settingsRows[2][1] = { korean ? L"자동 스킬" : L"AUTO SKILL", nullptr, false, false, false, false,
                           { korean ? L"켜짐" : L"ON", korean ? L"꺼짐" : L"OFF" }, 2, g_AutoSkill ? 0 : 1 };
    settingsRows[2][2] = { korean ? L"조준선" : L"CROSSHAIR", nullptr, false, false, false, false,
                           { korean ? L"켜짐" : L"ON", korean ? L"꺼짐" : L"OFF" }, 2, g_ShowCrosshair ? 0 : 1 };
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
    const wchar_t* allGameplayDesc[3] = {
        korean ? L"자동 조준 발사" : L"Automatic target fire",
        korean ? L"자동 스킬 발동" : L"Automatic skill trigger",
        korean ? L"플레이 중 조준선" : L"In-run aiming reticle" };
    for (int i = 0; i < 4; ++i) allDescriptions[0][i] = allDisplayDesc[i];
    for (int i = 0; i < 5; ++i) allDescriptions[1][i] = allAudioDesc[i];
    for (int i = 0; i < 3; ++i) allDescriptions[2][i] = allGameplayDesc[i];
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
    const wchar_t* gameplayLabels[3] = {
        korean ? L"자동 발사" : L"AUTO FIRE",
        korean ? L"자동 스킬" : L"AUTO SKILL",
        korean ? L"조준선" : L"CROSSHAIR" };
    const wchar_t* archiveLabels[2] = {
        korean ? L"언어" : L"LANGUAGE",
        korean ? L"데이터 초기화" : L"RESET DATA" };
    addHeader(0, korean ? L"화면" : L"DISPLAY");
    for (int i = 0; i < 4; ++i) addItem(0, i, displayLabels[i]);
    addHeader(1, korean ? L"소리" : L"AUDIO");
    for (int i = 0; i < 5; ++i) addItem(1, i, audioLabels[i]);
    addHeader(2, korean ? L"게임플레이" : L"GAMEPLAY");
    for (int i = 0; i < 3; ++i) addItem(2, i, gameplayLabels[i]);
    addHeader(3, korean ? L"기록" : L"ARCHIVE");
    for (int i = 0; i < 2; ++i) addItem(3, i, archiveLabels[i]);

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
    if (!isListItem(listCursor)) listCursor = 1;

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
                for (int k = i + 1; k < listCount; ++k) {
                    if (!list[k].header && list[k].category == list[i].category) {
                        targetIndex = k;
                        break;
                    }
                }
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

 #if 0
    int curHoverRow = -1;
    int curHoverOpt = -1; // which option chip is hovered within curHoverRow
    for (int i = 0; i < rowCount; ++i) {
        if (i != detailRow) {
            rowHover[i] = 0.0f;
            continue;
        }
        // Detail is a fixed readout slot. The selected row index belongs to
        // the left catalogue only and must never move the right-side design.
        const float ry = rFirstY;
        // 행마다 stagger: 탭 전환 후 순차 페이드인
        const float rReveal = Smoothstep(LogoClamp01(tabSwitchT / 0.18f));
        const float rA = dA * rReveal;
        // Every setting follows the same vertical grammar: title, description,
        // then the actual control. Keep the control band comfortably below the
        // description so long labels never collide with their values.
        const float rowControlY = ry + 132.0f * uiS;
        const float controlHitY = rowControlY - 30.0f * uiS;
        const float controlHitH = 60.0f * uiS;

        const bool hov = inputReady && !rows[i].readOnly
                       && mx >= rpX && mx < rpX + rClickW
                       && my >= controlHitY && my < controlHitY + controlHitH;
        if (hov) curHoverRow = i;
        rowHover[i] = UpdateMenuCommandHover(rowHover[i], hov, dt);

        // A restrained interaction rail replaces heavy cards. The row stays
        // transparent, but the hovered control gains a soft cyan trace so
        // the three columns read as one connected setting.
        const float rowFocus = rowHover[i];
        if (rowFocus > 0.001f) {
            drawRect(rpX, ry + rRowH - 10.0f,
                     rClickW, 1.0f,
                     0.34f, 0.72f, 1.0f, 0.12f * rowFocus * rA);
            drawDiamond(rLabelX - 16.0f, rowControlY,
                        2.8f + 1.8f * rowFocus,
                        0.34f, 0.72f, 1.0f,
                        0.42f * rowFocus * rA);
        }

        // Fixed satellite slot: rows remain broad hit targets, while their
        // visual identity is a constellation node rather than a card.
        // Label
        const float lA  = rows[i].readOnly ? 0.68f : (0.84f + 0.14f * rowHover[i]);
        const float lR  = rows[i].isDanger ? 1.0f  : 1.0f;
        const float lG  = rows[i].isDanger ? 0.36f : 1.0f;
        const float lB  = rows[i].isDanger ? 0.30f : 1.0f;
        DrawShadowedText(g_TextL, rows[i].label,
                         rLabelX, ry + 6.0f, 1.18f,
                         lR, lG, lB, lA * rA, 0.66f);
        if (rowDescription[i]) {
            DrawShadowedText(g_TextS, rowDescription[i],
                             rLabelX, ry + 56.0f, 0.72f,
                             0.66f, 0.74f, 0.84f,
                             0.76f * rA, 0.58f);
        }
        LogoLine(rLabelX, ry + 92.0f * uiS,
                 rpX + rClickW, ry + 92.0f * uiS,
                 0.62f * uiS, tabR, tabG, tabB, 0.16f * rA);

        const float flash = (pulseRow == i) ? pulse : 0.0f;

        if (rows[i].isVolume) {
            const float slX  = rpX + 28.0f * uiS;
            const float slW  = std::max(180.0f * uiS,
                                       rpX + rClickW - slX - 96.0f * uiS);
            const float slCY = rowControlY;
            const float slH  = 8.0f;
            const float vol01 = g_SoundVol / 100.0f;

            // 드래그 처리
            const bool inBar = inputReady
                && (double)mx >= slX - 6.0 && (double)mx <= slX + slW + 6.0
                && (double)my >= slCY - 14.0 && (double)my <= slCY + 14.0;
            if (inputReady && lmb && (inBar || s_volDrag)) {
                s_volDrag = true;
                const float t = std::max(0.0f, std::min(1.0f, (float)((double)mx - slX) / slW));
                g_SoundVol = (int)(t * 100.0f + 0.5f);
            }
            if (!lmb) s_volDrag = false;

            const float glow = s_volDrag ? 1.0f : rowHover[i];

            // 트랙 배경
            drawRect(slX, slCY - slH * 0.5f, slW, slH,
                     0.18f, 0.24f, 0.34f, 0.50f * rA);
            // 채워진 부분
            if (vol01 > 0.0f)
                drawRect(slX, slCY - slH * 0.5f, slW * vol01, slH,
                         0.48f, 0.82f, 1.0f, (0.60f + 0.28f * glow) * rA);
            // 핸들
            const float kx = slX + slW * vol01;
            const float ks = (6.0f + 3.5f * glow);
            drawDiamond(kx, slCY, ks,
                        0.76f + 0.24f * glow, 0.90f + 0.10f * glow, 1.0f,
                        (0.86f + 0.14f * glow) * rA);

            // 수치 (슬라이더 오른쪽)
            wchar_t vbuf[8];
            swprintf_s(vbuf, L"%d", g_SoundVol);
            DrawShadowedText(g_TextL, vbuf, slX + slW + 16.0f,
                             rowControlY - 14.0f, 0.88f,
                             0.48f, 0.82f, 1.0f, (0.72f + 0.24f * glow) * rA, 0.58f);
        } else if (rows[i].isHold) {
            // Hold-to-confirm bar (RESET DATA)
            const float barH = 30.0f;
            const float barW = 240.0f;
            const float barX = rpX + rClickW - barW - 12.0f;
            const float barY = rowControlY - barH * 0.5f;
            const float showA = std::max(rowHover[i], holdT > 0.01f ? 1.0f : 0.0f);
            drawRect(barX, barY, barW, barH,
                     0.18f, 0.04f, 0.04f, 0.55f * showA * rA);
            if (holdT > 0.01f) {
                const float prog = holdT / 1.5f;
                drawRect(barX + 2.0f, barY + 2.0f,
                         (barW - 4.0f) * prog, barH - 4.0f,
                         1.0f, 0.22f, 0.14f, 0.92f * rA);
            }
            DrawShadowedText(g_TextS, holdT > 0.01f ? L"ERASING..." : L"HOLD TO RESET",
                             barX + 12.0f, barY + 8.0f, 0.62f,
                             1.0f, rows[i].isDanger ? 0.36f : 0.42f, 0.30f,
                             (0.72f + 0.26f * rowHover[i]) * rA, 0.58f);
        } else if (rows[i].optCount > 0) {
            // 각 옵션을 독립적으로 클릭 가능한 칩으로 렌더링
            // Option values are the actual controls, so they must read larger
            // than the helper descriptions while still fitting long rows.
            // FPS has six values and needs a compact rail; short binary
            // options keep the larger control scale for readability.
            float optSc = 0.96f;
            const float chipPadX = 8.0f;
            const float chipGap  = (i == 0) ? 9.0f : 14.0f;
            // 전체 너비 계산 (우→좌)
            const float optionStart = optionStartX;
            const float optionEnd = rpX + rClickW - 12.0f;
            float longestW = 0.0f;
            for (int j = 0; j < rows[i].optCount; ++j)
                longestW = std::max(longestW, g_TextL.Width(rows[i].opts[j], optSc));
            float cellW = longestW + chipPadX * 2.0f;
            float totalW = cellW * rows[i].optCount + chipGap * (rows[i].optCount - 1);
            const float availableW = std::max(80.0f, optionEnd - optionStart);
            if (totalW > availableW) {
                const float fit = std::max(i == 0 ? 0.55f : 0.68f,
                                           availableW / totalW);
                optSc *= fit;
                longestW = 0.0f;
                for (int j = 0; j < rows[i].optCount; ++j)
                    longestW = std::max(longestW, g_TextL.Width(rows[i].opts[j], optSc));
                cellW = longestW + chipPadX * 2.0f;
                totalW = cellW * rows[i].optCount + chipGap * (rows[i].optCount - 1);
            }
            float ox = optionStart;
            // 좌→우 렌더링
            for (int j = 0; j < rows[i].optCount; ++j) {
                const float ow  = g_TextL.Width(rows[i].opts[j], optSc);
                const float chipX = ox;
                const float chipW = cellW;

                const bool isCur = (j == rows[i].optCur);
                const bool ohov  = inputReady
                                 && mx >= chipX && mx < chipX + chipW
                                 && my >= controlHitY
                                 && my < controlHitY + controlHitH;
                optHover[i][j] = UpdateMenuCommandHover(optHover[i][j], ohov, dt);
                if (ohov) { curHoverRow = i; curHoverOpt = j; }

                float oR = tabR, oG = tabG, oB = tabB;

                // 칩 배경
                const float chipBgA = 0.0f;
                (void)chipBgA;
                // 칩 테두리 (선택된 경우)
                if (isCur) {
                    drawDiamond(chipX - 8.0f * uiS, rowControlY,
                                3.4f * uiS, oR, oG, oB, 0.78f * rA);
                    LogoLine(chipX, rowControlY + 17.0f * uiS,
                             chipX + chipW, rowControlY + 17.0f * uiS,
                             0.75f * uiS, oR, oG, oB, 0.34f * rA);
                }

                const float oA = isCur
                    ? (0.90f + 0.10f * optHover[i][j] + 0.14f * flash)
                    : (0.45f + 0.35f * optHover[i][j]);
                DrawShadowedText(g_TextL, rows[i].opts[j],
                                 chipX + (chipW - ow) * 0.5f, rowControlY, optSc,
                                 oR, oG, oB, oA * rA, 0.58f);
                ox += chipW + chipGap;
            }
        } else if (rows[i].value) {
            // 단순 값 표시 (read-only 또는 볼륨 수치)
            const float vAlpha = rows[i].readOnly
                               ? 0.68f : (0.76f + 0.22f * rowHover[i] + 0.16f * flash);
            const float dvR = rows[i].readOnly ? 0.62f : 0.48f;
            const float dvG = rows[i].readOnly ? 0.70f : 0.82f;
            const float dvB = rows[i].readOnly ? 0.78f : 1.0f;
            DrawShadowedText(g_TextL, rows[i].value,
                              rpX + 28.0f * uiS,
                             rowControlY, 0.88f,
                             dvR, dvG, dvB, vAlpha * rA, 0.58f);
        }
    }
    for (int i = rowCount; i < 6; ++i) rowHover[i] = 0.0f;
 #endif

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
        } else if (category == 2 && row < 3) {
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
    const bool backHit = ready && !confirmBack && !confirmAbandon
                       && mx >= backX && mx < backX + backW
                       && my >= backY && my < backY + backH;
    const bool saveHit = ready && !confirmBack && !confirmAbandon
                       && mx >= saveX && mx < saveX + saveW
                       && my >= saveY && my < saveY + saveH;

    const float restartX = mainX;
    const float restartY = backY - (backH + 14.0f * uiS);
    const float restartW = backW;
    const float restartH = backH;
    const bool restartHit = inGameSettings && ready && !confirmBack && !confirmAbandon
                          && mx >= restartX && mx < restartX + restartW
                          && my >= restartY && my < restartY + restartH;
    const float abandonX = mainX;
    const float abandonY = restartY - (restartH + 14.0f * uiS);
    const float abandonW = restartW;
    const float abandonH = restartH;
    const bool abandonHit = inGameSettings && ready && !confirmBack && !confirmAbandon
                          && mx >= abandonX && mx < abandonX + abandonW
                          && my >= abandonY && my < abandonY + abandonH;

    if (inGameSettings) {
        DrawUnifiedMenuCommand(
            korean ? L"\uD3EC\uAE30\uD558\uAE30" : L"ABANDON RUN",
            korean ? L"\uACB0\uACFC \uD654\uBA74\uC73C\uB85C \uC774\uB3D9" : L"OPEN RUN REPORT",
            abandonX, abandonY, abandonW, abandonH,
            1.0f, 0.30f, 0.28f, treeA,
            0.0f, abandonHit, 0.0f, now + 0.04f,
            0.82f, 0.40f, true, true, true);
        DrawUnifiedMenuCommand(
            korean ? L"런 재시작" : L"RESTART RUN",
            korean ? L"현재 장비 유지" : L"KEEP CURRENT LOADOUT",
            restartX, restartY, restartW, restartH,
            1.0f, 0.48f, 0.32f, treeA,
            0.0f, restartHit, 0.0f, now + 0.12f,
            0.82f, 0.40f, true, true, true);
    }

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

    if (abandonHit && lmb && !g_LmbPrev) {
        confirmAbandon = true;
    }
    if (restartHit && lmb && !g_LmbPrev) {
        if (c.restartRun) c.restartRun();
        ResetSettingsUi();
        wasInline = false;
        return;
    }
    if (ready && !confirmBack && lmb && !g_LmbPrev && backHit) {
        if (settingsDirty) confirmBack = true;
        else exiting = true;
    }
    if (ready && !confirmBack && lmb && !g_LmbPrev && saveHit && settingsDirty) {
        SaveGame();
        savedSettings = { g_FpsCap, g_VfxDensity, g_ShaderFx, g_MobVisualStyle,
                          g_ShowCombo, g_BackdropBlurEnabled, g_SoundVol,
                          g_AutoFire, g_AutoSkill, g_ShowCrosshair, g_Language };
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
                              g_AutoFire, g_AutoSkill, g_ShowCrosshair, g_Language };
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

    if (confirmAbandon) {
        const float cx = sw * 0.50f;
        const float cy = sh * 0.52f;
        drawRect(0.0f, 0.0f, sw, sh, 0.01f, 0.02f, 0.04f,
                 0.62f * treeA);
        DrawShadowedText(g_TextL,
                         korean ? L"\uB7F0 \uD3EC\uAE30" : L"ABANDON RUN?",
                         cx - 150.0f * uiS, cy - 66.0f * uiS,
                         0.96f, 1.0f, 0.42f, 0.38f,
                         0.98f * treeA, 0.72f);
        DrawShadowedText(g_TextS,
                         korean ? L"\uD604\uC7AC \uB7F0\uC744 \uC885\uB8CC\uD558\uACE0 \uACB0\uACFC \uD654\uBA74\uC73C\uB85C \uC774\uB3D9\uD569\uB2C8\uB2E4"
                                : L"END THE CURRENT RUN AND OPEN THE RESULT SCREEN",
                         cx - 174.0f * uiS, cy - 28.0f * uiS,
                         0.58f, 0.76f, 0.82f, 0.90f,
                         0.84f * treeA, 0.58f);
        const float dialogY = cy + 26.0f * uiS;
        const float dialogW = 220.0f * uiS;
        const float dialogH = 50.0f * uiS;
        const bool cancelHit = mx >= cx - dialogW - 12.0f * uiS
                            && mx < cx - 12.0f * uiS
                            && my >= dialogY && my < dialogY + dialogH;
        const bool abandonConfirmHit = mx >= cx + 12.0f * uiS
                                    && mx < cx + dialogW + 12.0f * uiS
                                    && my >= dialogY && my < dialogY + dialogH;
        DrawUnifiedMenuCommand(korean ? L"\uCDE8\uC18C" : L"CANCEL", nullptr,
                               cx - dialogW - 12.0f * uiS, dialogY,
                               dialogW, dialogH, 0.34f, 0.72f, 1.0f,
                               treeA, 0.0f, cancelHit, 0.0f, now,
                               0.72f, 0.36f, true, true, true);
        DrawUnifiedMenuCommand(korean ? L"\uD3EC\uAE30\uD558\uAE30" : L"ABANDON",
                               nullptr, cx + 12.0f * uiS, dialogY,
                               dialogW, dialogH, 1.0f, 0.30f, 0.28f,
                               treeA, 0.0f, abandonConfirmHit, 0.0f, now + 0.2f,
                               0.72f, 0.36f, true, true, true);
        if (lmb && !g_LmbPrev && cancelHit) {
            confirmAbandon = false;
        } else if (lmb && !g_LmbPrev && abandonConfirmHit) {
            if (c.abandonRun) c.abandonRun();
            confirmAbandon = false;
            ResetSettingsUi();
            wasInline = false;
            return;
        }
    }

    // Context readout: the lower-right area is reserved for the setting that
    // currently owns the user's attention. This gives short tabs such as
    // AUDIO and SYSTEM useful structure without inventing more controls.
    // SYSTEM \ud0ed \ud558\ub2e8 \uc815\ubcf4 \uc601\uc5ed (\uc77d\uae30 \uc804\uc6a9, \ud589 \ub8e8\ud504 \ubc16)
    if (false && tab == 3) {
        const float infoY = rFirstY + rowCount * rRowH + 20.0f;
        // \uad6c\ubd84\uc120
        LogoLine(rLabelX, infoY, rpX + rClickW * 0.78f, infoY,
                 0.8f, 0.48f, 0.82f, 1.0f, 0.10f * dA);
        // BEST SCORE
        DrawShadowedText(g_TextS, L"BEST SCORE", rLabelX, infoY + 10.0f, 0.52f,
                         0.55f, 0.62f, 0.72f, 0.52f * dA, 0.55f);
        DrawShadowedText(g_TextL, s_scoreBuf, rpX + rClickW - 180.0f, infoY + 8.0f, 0.76f,
                         0.55f, 0.62f, 0.72f, 0.52f * dA, 0.55f);
        // BUILD
        DrawShadowedText(g_TextS, L"BUILD", rLabelX, infoY + 48.0f, 0.52f,
                         0.55f, 0.62f, 0.72f, 0.52f * dA, 0.55f);
        DrawShadowedText(g_TextL, L"DEVELOPER SIGNAL", rpX + rClickW - 180.0f, infoY + 46.0f, 0.76f,
                         0.55f, 0.62f, 0.72f, 0.52f * dA, 0.55f);
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
    if (s_MainMenuSettingsPanel || g_SettingsReturnTo == GameState::PAUSED) {
        Scene_SettingsInline(c);
        return;
    }
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    GLFWwindow* window = c.window;
    (void)c.fireTimer;
    (void)c.reset;

    static int   s_tab         = 0;
    static int   s_prevTab     = 0;
    static float s_tabHover[3] = {};
    static float s_tabFocus[3] = {};
    static float s_panelT      = 1.0f;
    static bool  s_showCredits = false;
    static bool  s_bsPrev      = false;
    static bool  s_enPrev      = false;
    static bool  s_resetDialog          = false;
    static bool  s_resetDialogJustOpened = false;
    static bool  s_resetItems[4] = {};
    static bool  s_resetDone   = false;
    static int   s_crtFxLevel  = 100;
    static float s_nodePulse   = 0.0f;

    struct SettingsTabDef {
        const wchar_t* id;
        const wchar_t* name[2];
        const wchar_t* brief[2];
        float r, g, b;
    };
    static const SettingsTabDef kTabs[] = {
        { L"GAME",   { L"게임",   L"GAME"   },
          { L"화면, 언어, 전투 보조, HUD",    L"Screen, language, combat assist, HUD" },
          0.42f, 0.62f, 1.00f },
        { L"AUDIO",  { L"오디오", L"AUDIO"  },
          { L"음량과 사운드 채널",          L"Volume and sound channels" },
          1.00f, 0.76f, 0.30f },
        { L"ARCHIVE", { L"기록", L"ARCHIVE" },
          { L"저장 기록, 리셋, 크레딧", L"Save, records, reset, credits" },
          1.00f, 0.45f, 0.42f },
    };
    constexpr int kTabCount = 3;
    if (s_tab < 0 || s_tab >= kTabCount) s_tab = 0;

    const bool settingsOverlay = (g_SettingsReturnTo == GameState::PAUSED);
    if (!settingsOverlay) DrawMenuBackground(sw, sh, delta, false);

    // Entry animation timer
    float dt = delta; if (dt > 0.05f) dt = 0.05f;
    g_SettingsEntryT += dt;
    if (g_SettingsEntryT > 1.0f) g_SettingsEntryT = 1.0f;
    const float now  = (float)glfwGetTime();

    const float wake = Smoothstep(std::min(1.0f, g_SettingsEntryT / 0.52f));
    const float rightWake = Smoothstep(std::min(1.0f,
        std::max(0.0f, g_SettingsEntryT - 0.10f) / 0.44f));
    SetSceneTextureReveal(Smoothstep(LogoClamp01(g_SettingsEntryT / 0.72f)));

    int li = LangIndex();
    if (li < 0 || li >= 3) li = 0;
    const int nli = (li == 0) ? 0 : 1;
    auto SettingsText = [&](const wchar_t* kr, const wchar_t* en) -> const wchar_t* {
        return (nli == 0) ? kr : en;
    };

    // Main-menu family layout: floating left commands and an open detail field.
    const float TARGET_W = 1640.0f;
    const float TARGET_H = 910.0f;
    float uiS = std::max(0.72f, std::min(sw * 0.985f / TARGET_W, sh * 0.965f / TARGET_H));
    if (uiS > 1.30f) uiS = 1.30f;
    const float panelW  = TARGET_W * uiS;
    const float panelH  = TARGET_H * uiS;
    const float panelX  = (sw - panelW) * 0.5f;
    const float panelY  = (sh - panelH) * 0.5f;
    const float headerH = 86.0f * uiS;
    const float footerH = 72.0f * uiS;
    const float bodyY   = panelY + headerH;
    const float bodyH   = panelH - headerH - footerH;
    const float leftW   = panelW * 0.255f;
    const float colGap  = 34.0f * uiS;
    const float leftX   = panelX;
    const float rightX  = leftX + leftW + colGap;
    const float rightW  = panelW - leftW - colGap;
    const float footY   = panelY + panelH - footerH + 14.0f * uiS;
    if (!settingsOverlay) {
        DrawPersistentSceneLeftVignette(sw, sh, 0.08f);
        DrawSceneRadialVignette(rightX + rightW * 0.52f, bodyY + bodyH * 0.52f,
                                std::min(rightW, bodyH) * 0.56f, 0.08f * rightWake);
    }

    const SettingsTabDef& tab = kTabs[s_tab];
    const UiThemePalette& uiTheme = GetUiTheme();
    const float baseR = uiTheme.Frame.r, baseG = uiTheme.Frame.g, baseB = uiTheme.Frame.b;
    // Normalize legacy language saves before laying out translated controls.
    if (s_tab == 0 && (int)g_Language > 1) g_Language = Language::KR;

    auto hit = [&](float x, float y, float w, float h) {
        return mx >= x && mx <= x + w && my >= y && my <= y + h;
    };
    const bool modalActive = s_resetDialog || s_resetDone;
    auto clampVol = [](int v) { return v < 0 ? 0 : (v > 100 ? 100 : v); };
    auto commitVol = [&]() {
        int v = 0;
        for (int i = 0; i < g_VolLen; ++i) v = v * 10 + (g_VolBuf[i] - L'0');
        if (g_VolLen > 0) g_SoundVol = clampVol(v);
        g_VolEdit = false;
    };
    auto drawBorder = [](float x, float y, float w, float h,
                         float r, float g, float b, float a, float t) {
        drawRect(x, y, w, t, r, g, b, a);
        drawRect(x, y + h - t, w, t, r, g, b, a);
        drawRect(x, y, t, h, r, g, b, a);
        drawRect(x + w - t, y, t, h, r, g, b, a);
    };
    auto drawFitS = [&](const wchar_t* text, float x, float y, float maxW,
                        float sc, float minSc, float r, float g, float b, float a) {
        while (sc > minSc && g_TextS.Width(text, sc) > maxW)
            sc -= 0.025f * uiS;
        g_TextS.Draw(text, x, y, sc, r, g, b, a);
    };
    auto drawCenterS = [&](const wchar_t* text, float x, float y, float w,
                           float sc, float r, float g, float b, float a) {
        while (sc > 0.36f * uiS && g_TextS.Width(text, sc) > w - 12.0f * uiS)
            sc -= 0.025f * uiS;
        float tw = g_TextS.Width(text, sc);
        g_TextS.Draw(text, x + (w - tw) * 0.5f, y, sc, r, g, b, a);
    };
    auto drawOrbitArc = [&](float cx, float cy, float rx, float ry,
                            float a0, float a1, float r, float g, float b,
                            float thick, float a) {
        const int segments = 22;
        float px = cx + cosf(a0) * rx;
        float py = cy + sinf(a0) * ry;
        for (int i = 1; i <= segments; ++i) {
            const float t = (float)i / (float)segments;
            const float ang = a0 + (a1 - a0) * t;
            const float nx = cx + cosf(ang) * rx;
            const float ny = cy + sinf(ang) * ry;
            DrawVisibleConstellLine(px, py, nx, ny, thick, r, g, b, a);
            px = nx;
            py = ny;
        }
        DrawVisibleConstellNode(px, py, 2.4f * uiS, r, g, b, 0.50f * a);
    };
    auto segment = [&](float x, float y, float w, float h, const wchar_t* label,
                       bool selected, float r, float g, float b, bool enabled = true) {
        bool hov = enabled && hit(x, y, w, h);
        float ctlA = rightWake * (0.58f + 0.42f * Smoothstep(s_panelT));
        BindMainShader();
        // A soft baseline replaces the old box-shaped control. The endpoint
        // nodes make the hit target legible without turning it into a card.
        const float ba = (enabled ? (selected ? 0.94f : (hov ? 0.64f : 0.24f)) : 0.12f) * ctlA;
        const float lineY = y + h - 4.0f * uiS;
        LogoLine(x + 10.0f * uiS, lineY, x + w - 10.0f * uiS, lineY,
                 (selected ? 1.7f : 0.9f) * uiS, r, g, b, ba);
        DrawVisibleConstellNode(x + 10.0f * uiS, lineY,
                                (selected ? 3.2f : 2.2f) * uiS,
                                r, g, b, ba, false);
        DrawVisibleConstellNode(x + w - 10.0f * uiS, lineY,
                                (selected ? 3.2f : 2.2f) * uiS,
                                r, g, b, ba, false);
        drawCenterS(label, x, y + 7.0f * uiS, w, 0.76f * uiS,
                    enabled ? (selected ? 1.0f : 0.70f) : 0.40f,
                    enabled ? (selected ? 1.0f : 0.78f) : 0.44f,
                    enabled ? (selected ? 1.0f : 0.90f) : 0.52f,
                    (enabled ? 0.96f : 0.52f) * ctlA);
        return hov && lmb && !g_LmbPrev && !modalActive;
    };

    if (s_prevTab != s_tab) {
        s_prevTab = s_tab;
        s_panelT  = 0.0f;
    }
    s_panelT = UiApproach(s_panelT, 1.0f, delta, 10.0f);
    if (s_nodePulse > 0.0f) s_nodePulse = std::max(0.0f, s_nodePulse - dt);
    if (s_tab != 1 && g_VolEdit) commitVol();

    // === HEADER ===
    const wchar_t* langShort[3] = { L"KR", L"EN", L"JP" };
    wchar_t fpsBuf[24];
    if (g_FpsCap == 0) swprintf_s(fpsBuf, L"VSYNC");
    else swprintf_s(fpsBuf, L"%d FPS", g_FpsCap);
    wchar_t profileBuf[96];
    swprintf_s(profileBuf, L"%ls / %ls / SOUND %d", langShort[li], fpsBuf, g_SoundVol);

    const float hSlide = (1.0f - wake) * 16.0f * uiS;
    BindMainShader();
    g_TextL.Draw(L"ASTRAL SETTINGS",
                 leftX, panelY + 8.0f * uiS - hSlide,
                 1.16f * uiS, 1.0f, 1.0f, 1.0f, 0.98f * wake);
    float profileW = 330.0f * uiS;
    float profileX = panelX + panelW - profileW + (1.0f - wake) * 50.0f * uiS;
    drawCenterS(profileBuf, profileX, panelY + 16.0f * uiS, profileW,
                0.44f * uiS, 0.72f, 0.84f, 1.0f, 0.82f * wake);
    LogoLine(profileX + 12.0f*uiS, panelY + 46.0f*uiS,
             profileX + profileW - 12.0f*uiS, panelY + 46.0f*uiS,
             1.0f*uiS, baseR, baseG, baseB, 0.20f * wake);

    // === LEFT COLUMN: shared command rail ===
    // This is intentionally the same root-tab loop used by the codex and
    // armory pages: same hitbox, reveal, slide and command renderer.
    const float tabCardGap = 15.0f * uiS;
    const float tabCardH   = 70.0f * uiS;
    const float rootX = leftX;
    const float rootY = bodyY;
    const float rootW = leftW;
    const float rootH = tabCardH;
    const float rootGap = tabCardGap;
    const bool inputReady = !modalActive && g_SettingsEntryT >= 0.55f;
    for (int i = 0; i < kTabCount; ++i) {
        const SettingsTabDef& td = kTabs[i];
        const float tx = rootX;
        const float ty = rootY + (float)i * (rootH + rootGap);
        const float hitX = tx - 26.0f * uiS;
        const float hitRight = tx + rootW + 18.0f * uiS;
        const bool hov = inputReady
                       && (mx >= hitX && mx < hitRight
                       && my >= ty && my < ty + rootH);
        s_tabHover[i] = UpdateMenuCommandHover(s_tabHover[i], hov, dt);
        const bool sel = s_tab == i;
        s_tabFocus[i] = UiApproach(s_tabFocus[i], sel ? 1.0f : 0.0f, delta, 8.5f);
        if (hov && lmb && !g_LmbPrev && !sel) {
            if (g_VolEdit) commitVol();
            s_tab = i;
            s_prevTab = i;
            s_panelT = 0.0f;
            s_nodePulse = 0.5f;
            s_resetDialog = false;
        }

        const float reveal = MenuCommandReveal(g_SettingsEntryT, i);
        const float rowA = reveal * wake;
        const float selectPulse = sel ? (0.12f + 0.12f * sinf(now * 3.3f)) : 0.0f;
        const float bx = tx + (1.0f - reveal) * 34.0f * uiS
                          - 10.0f * s_tabHover[i];
        DrawUnifiedMenuCommand(td.id, td.name[0], bx, ty, rootW, rootH,
                               td.r, td.g, td.b, rowA, s_tabHover[i], sel,
                               selectPulse, now + (float)i * 0.17f,
                               1.04f, 0.48f);
    }

    // Non-interactive calibration node decoration under compact tabs.
    {
        float decoY = bodyY + (tabCardH + tabCardGap) * kTabCount + 36.0f * uiS;
        float decoH = std::max(0.0f, bodyH - (decoY - bodyY) - 18.0f * uiS);
        if (decoH > 90.0f * uiS) {
            const float cx = leftX + leftW * 0.50f;
            const float cy = decoY + decoH * 0.46f;
            const float rad = std::min(leftW, decoH) * 0.34f;
            const float pulseT = Smoothstep(std::min(1.0f, s_nodePulse / 0.5f));
            const float pulseA = 1.0f + pulseT * 2.4f;
            const float pulseSize = 1.0f + pulseT * 0.35f;
            float px[7], py[7];
            BindMainShader();
            drawRect(leftX + 18.0f*uiS, decoY, leftW - 36.0f*uiS, 1.0f*uiS,
                     baseR, baseG, baseB, 0.085f * pulseA * wake);
            for (int i = 0; i < 7; ++i) {
                float a = now * 0.055f + (float)i * 0.8975979f;
                float rr = rad * (0.70f + 0.24f * ((i % 3) / 2.0f));
                px[i] = cx + cosf(a) * rr;
                py[i] = cy + sinf(a) * rr;
            }
            for (int i = 0; i < 7; ++i) {
                int j = (i + 2) % 7;
                LogoLine(px[i], py[i], px[j], py[j], 0.80f*uiS,
                         baseR, baseG, baseB, 0.070f * pulseA * wake);
            }
            for (int i = 0; i < 7; ++i)
                drawDiamond(px[i], py[i], ((i % 3) == 0 ? 3.2f : 2.2f) * uiS,
                            baseR, baseG, baseB,
                            ((i % 3) == 0 ? 0.18f : 0.11f) * pulseA * wake);
            if (pulseT > 0.01f) {
                drawCircle(cx, cy, rad * pulseSize, baseR, baseG, baseB, 0.035f * pulseT * wake);
                drawDiamond(cx, cy, 5.0f * pulseSize * uiS, baseR, baseG, baseB, 0.32f * pulseT * wake);
            }
            g_TextS.Draw(L"ORBIT NODE", leftX + 22.0f*uiS, decoY + decoH - 34.0f*uiS,
                         0.34f*uiS, 0.42f, 0.55f, 0.68f, (0.30f + 0.22f * pulseT) * wake);
        }
    }

    // === RIGHT PANEL: Content window ===
    float panelE = Smoothstep(s_panelT);
    float entryShift = (1.0f - rightWake) * 70.0f * uiS;
    float panelShift = (1.0f - panelE) * 40.0f * uiS;
    float rpx = rightX + entryShift + panelShift;

    // Tab → panel connector: 선택된 탭 우측 끝 → 우측 패널 좌측 끝 가이드라인
    {
        float connFoc = s_tabFocus[s_tab];
        float connCwi = MenuCommandReveal(g_SettingsEntryT, s_tab);
        float connA   = connFoc * rightWake * connCwi;
        if (connA > 0.01f) {
            float selCardY = bodyY + (float)s_tab * (tabCardH + tabCardGap);
            float connY    = selCardY + tabCardH * 0.5f;
            float cx0      = leftX + leftW;
            float cx1      = rpx;
            float connH    = 1.5f * uiS;
            BindMainShader();
            LogoLine(cx0, connY, cx0 + (cx1 - cx0) * 0.42f, connY,
                     0.9f * uiS, baseR, baseG, baseB, 0.24f * connA);
            drawOrbitArc(cx1 - 32.0f * uiS, connY,
                         34.0f * uiS, 18.0f * uiS,
                         3.85f, 5.35f, baseR, baseG, baseB,
                         0.75f * uiS, 0.28f * connA);
            DrawVisibleConstellNode(cx0, connY, 3.6f * uiS,
                                    baseR, baseG, baseB, 0.62f * connA);
        }
    }

    BindMainShader();
    LogoLine(rpx, bodyY, rpx + rightW, bodyY,
             0.8f*uiS, baseR, baseG, baseB, 0.24f*panelE*rightWake);
    if (rightWake > 0.02f) {
        drawOrbitArc(rpx + rightW * 0.48f, bodyY + bodyH * 0.50f,
                     rightW * 0.55f, bodyH * 0.58f,
                     5.55f, 7.10f, baseR, baseG, baseB,
                     0.90f * uiS, 0.22f * panelE * rightWake);
        drawOrbitArc(rpx + rightW * 0.48f, bodyY + bodyH * 0.50f,
                     rightW * 0.48f, bodyH * 0.48f,
                     2.95f, 4.18f, baseR, baseG, baseB,
                     0.65f * uiS, 0.12f * panelE * rightWake);
    }
    BindMainShader();
    // Master detail title
    g_TextL.Draw(L"TUNING FIELD", rpx + 20.0f*uiS, bodyY + 12.0f*uiS,
                 0.94f*uiS, 1.0f, 1.0f, 1.0f, 0.96f*panelE*rightWake);
    wchar_t tabTag[32];
    swprintf_s(tabTag, L"[ %ls ]", (nli == 0) ? tab.name[0] : tab.id);
    const float tagSc = 0.54f * uiS;
    const float tagW = g_TextS.Width(tabTag, tagSc);
    g_TextS.Draw(tabTag, rpx + rightW - 30.0f*uiS - tagW, bodyY + 28.0f*uiS,
                 tagSc, tab.r, tab.g, tab.b, 0.88f*panelE*rightWake);

    // === ROW LAYOUT ===
    const float rowX    = rpx + 22.0f * uiS;
    const float rowW    = rightW - 44.0f * uiS;
    const float rowH    = 64.0f * uiS;
    const float rowGap  = 8.0f * uiS;
    const float rowStart = bodyY + 92.0f * uiS;
    const float ctlW    = 420.0f * uiS;
    const float ctlH    = 42.0f * uiS;

    auto groupBox = [&](float y, float h, const wchar_t* title,
                        float r, float g, float b, bool danger = false) {
        float a = rightWake * (0.62f + 0.38f * panelE);
        BindMainShader();
        DrawVisibleConstellNode(rowX + 9.0f * uiS, y + 20.0f * uiS,
                                (danger ? 3.8f : 2.8f) * uiS,
                                r, g, b, (danger ? 0.86f : 0.60f) * a);
        LogoLine(rowX + 28.0f*uiS, y + 42.0f*uiS,
                 rowX + rowW * (danger ? 0.58f : 0.40f), y + 42.0f*uiS,
                 1.1f*uiS, r, g, b, (danger ? 0.30f : 0.18f) * a);
        g_TextS.Draw(title, rowX + 20.0f*uiS, y + 13.0f*uiS,
                     0.52f*uiS,
                     danger ? 1.0f : 0.82f,
                     danger ? 0.56f : 0.90f,
                     danger ? 0.56f : 1.0f,
                     0.90f * a);
    };

    auto rowShell = [&](float y, const wchar_t* label, const wchar_t* desc,
                        bool disabled, float h) {
        bool hov = !disabled && !modalActive && hit(rowX, y, rowW, h);
        float rowIdx = (y - rowStart) / (rowH + rowGap);
        if (rowIdx < 0.0f) rowIdx = 0.0f;
        float revealRaw = panelE * 1.22f - rowIdx * 0.075f;
        if (revealRaw < 0.0f) revealRaw = 0.0f;
        if (revealRaw > 1.0f) revealRaw = 1.0f;
        float rowA = Smoothstep(revealRaw) * rightWake;
        const float hovF = (hov && !disabled) ? 1.0f : 0.0f;
        BindMainShader();
        LogoLine(rowX + 24.0f*uiS, y + h - 1.0f*uiS,
                 rowX + 24.0f*uiS + (rowW - 48.0f*uiS) *
                     (0.36f + 0.20f * hovF), y + h - 1.0f*uiS,
                 0.9f*uiS, baseR, baseG, baseB,
                 (disabled ? 0.030f : 0.070f + 0.100f * hovF) * rowA);
        float labelMax = rowW - ctlW - 54.0f*uiS;
        const float codeSc = 0.34f * uiS;
        if (desc && desc[0]) {
            g_TextS.Draw(desc, rowX + 24.0f*uiS, y + 9.0f*uiS,
                         codeSc, baseR, baseG, baseB,
                         (disabled ? 0.34f : 0.62f) * rowA);
        }
        const float labelSc = 0.70f * uiS;
        const float labelY = y + 28.0f * uiS;
        drawFitS(label, rowX + 24.0f*uiS, labelY, labelMax,
                 labelSc, 0.54f*uiS,
                 disabled ? 0.45f : 0.92f, disabled ? 0.48f : 0.96f, disabled ? 0.56f : 1.0f,
                 (disabled ? 0.55f : 0.94f)*rowA);
    };
    auto rowShellStd = [&](float y, const wchar_t* label, const wchar_t* desc,
                           bool disabled = false) {
        rowShell(y, label, desc, disabled, rowH);
    };
    auto boolRow = [&](float y, const wchar_t* label, const wchar_t* desc, bool& value) {
        rowShellStd(y, label, desc);
        float bx   = rowX + rowW - ctlW - 16.0f*uiS;
        float by   = y + (rowH - ctlH)*0.5f;
        float bw   = (ctlW - 10.0f*uiS) * 0.5f;
        float offX = bx + bw + 10.0f*uiS;
        float ctlA = rightWake * (0.58f + 0.42f * Smoothstep(s_panelT));
        bool hovOn  = !modalActive && hit(bx,   by, bw, ctlH);
        bool hovOff = !modalActive && hit(offX, by, bw, ctlH);
        if (hovOn  && lmb && !g_LmbPrev) value = true;
        if (hovOff && lmb && !g_LmbPrev) value = false;

        // Corner-bracket highlight helper (inline lambda ok in C++17)
        auto drawBkt = [&](float ox, bool active, bool hov) {
            float ba = active ? 0.86f * ctlA : (hov ? 0.30f * ctlA : 0.0f);
            if (ba < 0.004f) return;
            const float lineY = by + ctlH - 4.0f * uiS;
            LogoLine(ox + 10.0f * uiS, lineY, ox + bw - 10.0f * uiS, lineY,
                     (active ? 1.6f : 0.8f) * uiS, baseR, baseG, baseB, ba);
            DrawVisibleConstellNode(ox + 10.0f * uiS, lineY,
                                    (active ? 3.0f : 2.0f) * uiS,
                                    baseR, baseG, baseB, ba, false);
            DrawVisibleConstellNode(ox + bw - 10.0f * uiS, lineY,
                                    (active ? 3.0f : 2.0f) * uiS,
                                    baseR, baseG, baseB, ba, false);
        };

        BindMainShader();
        drawBkt(bx,   value,  hovOn);
        drawBkt(offX, !value, hovOff);
        drawCenterS(SettingsText(L"활성", L"ACTIVE"), bx, by + 7.0f*uiS, bw, 0.64f*uiS,
                    value  ? 1.0f : 0.44f, value  ? 1.0f : 0.48f, value  ? 1.0f : 0.58f,
                    (value  ? 0.96f : 0.50f) * ctlA);
        drawCenterS(SettingsText(L"대기", L"IDLE"), offX, by + 7.0f*uiS, bw, 0.64f*uiS,
                    !value ? 1.0f : 0.44f, !value ? 1.0f : 0.48f, !value ? 1.0f : 0.58f,
                    (!value ? 0.96f : 0.50f) * ctlA);
    };
    auto choiceRow = [&](float y, const wchar_t* label, const wchar_t* desc,
                         const wchar_t* const* labels, const int* values, int count,
                         int current, auto setValue) {
        rowShellStd(y, label, desc);
        float bx = rowX + rowW - ctlW - 16.0f*uiS;
        float by = y + (rowH - ctlH)*0.5f;
        float sg = 8.0f*uiS;
        float bw = (ctlW - sg*(count-1)) / (float)count;
        for (int i = 0; i < count; ++i) {
            if (segment(bx + i*(bw+sg), by, bw, ctlH, labels[i],
                        current == values[i], baseR, baseG, baseB))
                setValue(values[i]);
        }
    };
    auto infoRow = [&](float y, const wchar_t* label, const wchar_t* desc,
                       const wchar_t* value, bool disabled = false) {
        rowShellStd(y, label, desc, disabled);
        float infoA = rightWake * (0.58f + 0.42f*panelE);
        float bx = rowX + rowW - ctlW - 16.0f*uiS;
        float by = y + (rowH - ctlH)*0.5f;
        BindMainShader();
        {   // Corner brackets
            float ba = (disabled ? 0.10f : 0.26f) * infoA;
            const float lineY = by + ctlH - 4.0f * uiS;
            LogoLine(bx + 12.0f * uiS, lineY, bx + ctlW - 12.0f * uiS, lineY,
                     0.9f * uiS, baseR, baseG, baseB, ba);
            DrawVisibleConstellNode(bx + ctlW * 0.5f, lineY, 2.2f * uiS,
                                    baseR, baseG, baseB, ba, false);
        }
        drawCenterS(value, bx, by + 7.0f*uiS, ctlW, 0.76f*uiS,
                    disabled ? 0.48f : 0.82f, disabled ? 0.50f : 0.90f, disabled ? 0.56f : 1.0f,
                    (disabled ? 0.52f : 0.90f)*infoA);
    };
    auto systemLogRow = [&](float y, const wchar_t* text) {
        float rowA = rightWake * (0.58f + 0.42f * panelE);
        BindMainShader();
        drawRect(rowX + 24.0f*uiS, y + rowH - 1.0f*uiS,
                 rowW - 48.0f*uiS, 1.0f*uiS, baseR, baseG, baseB, 0.040f * rowA);
        g_TextS.Draw(L"[STAR_STATUS]", rowX + 24.0f*uiS, y + 9.0f*uiS,
                     0.34f*uiS, baseR, baseG, baseB, 0.36f * rowA);
        g_TextS.Draw(text, rowX + 24.0f*uiS, y + 30.0f*uiS,
                     0.46f*uiS, 0.42f, 0.48f, 0.58f, 0.72f * rowA);
    };
    auto volumeRow = [&](float y, const wchar_t* label, const wchar_t* desc, int& vol) {
        rowShellStd(y, label, desc);
        float volA = rightWake * (0.58f + 0.42f*panelE);
        float bx = rowX + rowW - ctlW - 16.0f*uiS;
        float by = y + (rowH - ctlH)*0.5f;
        float smallW = 42.0f*uiS;
        if (segment(bx, by, smallW, ctlH, L"-", false, baseR, baseG, baseB)) {
            g_VolEdit = false; vol = clampVol(vol - 5);
        }
        float barX  = bx + smallW + 12.0f*uiS;
        float fieldW = 76.0f*uiS;
        float plusX  = bx + ctlW - fieldW - smallW - 16.0f*uiS;
        float barW   = plusX - barX - 12.0f*uiS;
        float trackY = by + ctlH*0.5f;
        float fillW  = barW * (vol / 100.0f);
        BindMainShader();
        LogoLine(barX, trackY, barX + barW, trackY, 1.2f*uiS,
                 0.22f, 0.28f, 0.38f, 0.74f*volA);
        if (fillW > 0.0f)
            LogoLine(barX, trackY, barX + fillW, trackY, 2.0f*uiS,
                     baseR, baseG, baseB, 0.90f*volA);
        float knobX = barX + fillW;
        drawCircle(knobX, trackY, 10.0f*uiS, baseR,baseG,baseB, 0.92f*volA);
        drawCircle(knobX, trackY, 5.2f*uiS, 0.96f,0.98f,1.0f, 0.96f*volA);
        bool barHover = !modalActive && hit(barX, by, barW, ctlH);
        if (lmb && barHover) {
            g_VolEdit = false;
            vol = clampVol((int)(((float)mx - barX) / barW * 100.0f + 0.5f));
        }
        if (segment(plusX, by, smallW, ctlH, L"+", false, baseR, baseG, baseB)) {
            g_VolEdit = false; vol = clampVol(vol + 5);
        }
        float fieldX = bx + ctlW - fieldW;
        bool fieldHover = hit(fieldX, by, fieldW, ctlH);
        BindMainShader();
        LogoLine(fieldX + 8.0f * uiS, by + ctlH - 4.0f * uiS,
                 fieldX + fieldW - 8.0f * uiS, by + ctlH - 4.0f * uiS,
                 (g_VolEdit ? 1.6f : 0.8f) * uiS, baseR, baseG, baseB,
                 (g_VolEdit ? 0.72f : (fieldHover ? 0.38f : 0.18f)) * volA);
        DrawVisibleConstellNode(fieldX + fieldW * 0.5f, by + ctlH - 4.0f * uiS,
                                (g_VolEdit ? 3.0f : 2.0f) * uiS,
                                baseR, baseG, baseB,
                                (g_VolEdit ? 0.72f : (fieldHover ? 0.38f : 0.18f)) * volA,
                                false);
        if (lmb && !g_LmbPrev && !modalActive) {
            if (fieldHover) { g_VolEdit = true; g_VolLen = 0; g_VolBuf[0] = 0; }
            else if (g_VolEdit) commitVol();
        }
        wchar_t shown[16];
        if (g_VolEdit) {
            bool caret = (((int)(glfwGetTime()*2.0)) & 1) == 0;
            swprintf_s(shown, L"%ls%ls", g_VolLen ? g_VolBuf : L"", caret ? L"|" : L"");
        } else {
            swprintf_s(shown, L"%d", vol);
        }
        drawCenterS(shown, fieldX, by + 7.0f*uiS, fieldW, 0.74f*uiS, 1.0f,1.0f,1.0f, 0.94f*volA);
    };
    auto disabledMeterRow = [&](float y, const wchar_t* label, const wchar_t* desc) {
        rowShellStd(y, label, desc, true);
        float meterA = rightWake * (0.58f + 0.42f*panelE);
        float bx = rowX + rowW - ctlW - 16.0f*uiS;
        float by = y + (rowH - ctlH)*0.5f;
        BindMainShader();
        {   // Corner brackets (disabled style)
            float ba = 0.12f * meterA;
            LogoLine(bx + 14.0f * uiS, by + ctlH - 4.0f * uiS,
                     bx + ctlW - 14.0f * uiS, by + ctlH - 4.0f * uiS,
                     0.8f * uiS, baseR, baseG, baseB, ba);
            DrawVisibleConstellNode(bx + ctlW * 0.5f, by + ctlH - 4.0f * uiS,
                                    2.0f * uiS, baseR, baseG, baseB, ba, false);
        }
        drawCenterS(SettingsText(L"마스터 연동", L"MASTER LINKED"),
                    bx, by+8.0f*uiS, ctlW, 0.64f*uiS, 0.48f,0.52f,0.62f, 0.62f*meterA);
    };

    if (g_VolEdit) {
        bool bs = (glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS);
        if (bs && !s_bsPrev && g_VolLen > 0) g_VolBuf[--g_VolLen] = 0;
        s_bsPrev = bs;
        bool en = (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS);
        if (en && !s_enPrev) commitVol();
        s_enPrev = en;
    } else {
        s_bsPrev = false;
        s_enPrev = false;
    }

    const float groupHead = 52.0f * uiS;
    const float groupPadB = 12.0f * uiS;
    const float groupGap = 16.0f * uiS;
    float y = rowStart;
    if (s_tab == 0) {
        const float displayH = groupHead + rowH * 5.0f + rowGap * 4.0f + groupPadB;
        const float controlH = groupHead + rowH * 2.0f + rowGap + groupPadB;
        const float langH    = groupHead + rowH + groupPadB;

        const wchar_t* fpsLabels[5] = { L"VSYNC", L"30", L"60", L"144", L"300" };
        const int fpsVals[5] = { 0, 30, 60, 144, 300 };
        groupBox(y, displayH, SettingsText(L"화면", L"DISPLAY"), baseR, baseG, baseB);
        y += groupHead;
        choiceRow(y, SettingsText(L"FPS 제한", L"FPS LIMIT"),
                  L"[FRAME_CAP]",
                  fpsLabels, fpsVals, 5, g_FpsCap, [&](int v) {
                      g_FpsCap = v;
                      glfwSwapInterval((g_FpsCap == 0) ? 1 : 0);
                  });
        y += rowH + rowGap;

        const wchar_t* crtLabelsKr[3] = { L"꺼짐", L"50", L"100" };
        const wchar_t* crtLabelsEn[3] = { L"OFF", L"50", L"100" };
        // Four fixed presets keep the shader output in known-good visual ranges.
        const wchar_t* crtPresetKr[4] = { L"\xAFBC\xC9D0", L"\xB0AE\xC74C", L"\xC911\xAC04", L"\xB192\xC74C" };
        const wchar_t* crtPresetEn[4] = { L"OFF", L"LOW", L"MID", L"HIGH" };
        const wchar_t* const* crtLabels = (nli == 0) ? crtPresetKr : crtPresetEn;
        const int crtVals[4] = { 0, 30, 60, 85 };
        int crtCurrent = g_ShaderFx ? s_crtFxLevel : 0;
        if (crtCurrent != 0 && crtCurrent != 30 && crtCurrent != 60 && crtCurrent != 85) crtCurrent = 85;
        choiceRow(y, SettingsText(L"CRT 셰이더", L"CRT_FX"),
                  L"[CRT_FX]",
                  crtLabels, crtVals, 4, crtCurrent, [&](int v) {
                      s_crtFxLevel = v;
                      g_ShaderFx = (v > 0);
                  });
        y += rowH + rowGap;

        const int twoVals[2] = { 0, 1 };
        const wchar_t* vfxLabelsKr[2] = { L"전체", L"감소" };
        const wchar_t* vfxLabelsEn[2] = { L"FULL", L"REDUCED" };
        const wchar_t* const* vfxLabels = (nli == 0) ? vfxLabelsKr : vfxLabelsEn;
        choiceRow(y, SettingsText(L"VFX 밀도", L"VFX_DENS"),
                  L"[VFX_DENS]",
                  vfxLabels, twoVals, 2, (int)g_VfxDensity, [&](int v) {
                      g_VfxDensity = (v == 0) ? VfxDensity::FULL : VfxDensity::REDUCED;
                  });

        y += rowH + rowGap;
        const wchar_t* themeLabelsKr[2] = { L"\xAE30\xBCF8", L"\xBC1D\xC74C" };
        const wchar_t* themeLabelsEn[2] = { L"DARK", L"LIGHT" };
        const wchar_t* const* themeLabels = (nli == 0) ? themeLabelsKr : themeLabelsEn;
        const int themeVals[2] = { 0, 1 };
        choiceRow(y, SettingsText(L"UI \xD14C\xB9C8", L"UI THEME"),
                  L"[UI_THEME]", themeLabels, themeVals, 2,
                  (int)g_UiThemeMode, [&](int v) {
                      SetUiTheme(v == 1 ? UiThemeMode::Light : UiThemeMode::Dark);
                  });
        y += rowH + rowGap;
        const wchar_t* blurLabelsKr[2] = {
            L"OFF", L"ON"
        };
        const wchar_t* blurLabelsEn[2] = {
            L"OFF", L"ON"
        };
        const wchar_t* const* blurLabels = (nli == 0) ? blurLabelsKr : blurLabelsEn;
        const int blurVals[2] = { 0, 1 };
        const int blurCurrent = g_BackdropBlurEnabled ? 1 : 0;
        choiceRow(y, SettingsText(L"배경 블러", L"BACKDROP BLUR"),
                  L"[BACKDROP_BLUR]", blurLabels, blurVals, 2, blurCurrent,
                  [&](int v) {
                      g_BackdropBlurEnabled = (v != 0);
                  });

        y = rowStart + displayH + groupGap;
        groupBox(y, controlH, SettingsText(L"조작", L"CONTROL"), baseR, baseG, baseB);
        y += groupHead;
        boolRow(y, SettingsText(L"자동 발사", L"AUTO_FIRE"),
                L"[AUTO_FIRE]",
                g_AutoFire);
        y += rowH + rowGap;
        boolRow(y, SettingsText(L"조준선", L"CROSSHAIR"),
                L"[AIM_CURSOR]",
                g_ShowCrosshair);

        y = rowStart + displayH + groupGap + controlH + groupGap;
        groupBox(y, langH, SettingsText(L"언어", L"LANGUAGE"), baseR, baseG, baseB);
        y += groupHead;
        const wchar_t* langLabelsKr[2] = { L"한국어", L"영어" };
        const wchar_t* langLabelsEn[2] = { L"KOR", L"ENG" };
        const wchar_t* const* langLabels = (nli == 0) ? langLabelsKr : langLabelsEn;
        const int langVals[2] = { 0, 1 };
        choiceRow(y, SettingsText(L"언어팩", L"LANG_PACK"),
                  L"[LANG_PACK]",
                  langLabels, langVals, 2, std::min((int)g_Language, 1), [&](int v) {
                      if (v >= 0 && v < 2) g_Language = (Language)v;
                  });
    } else if (s_tab == 1) {
        const float masterH = groupHead + rowH + groupPadB;
        const float channelH = groupHead + rowH * 3.0f + rowGap * 2.0f + groupPadB;
        groupBox(y, masterH, SettingsText(L"마스터", L"MASTER"), baseR, baseG, baseB);
        y += groupHead;
        volumeRow(y,
                  SettingsText(L"마스터 볼륨", L"MASTER VOL"),
                  L"[MASTER_VOL]",
                  g_SoundVol);

        y = rowStart + masterH + groupGap;
        groupBox(y, channelH, SettingsText(L"채널", L"CHANNELS"), baseR, baseG, baseB);
        y += groupHead;
        disabledMeterRow(y,
                         SettingsText(L"BGM 볼륨", L"BGM VOL"),
                         L"[BGM_VOL]");
        y += rowH + rowGap;
        disabledMeterRow(y,
                         SettingsText(L"효과음 볼륨", L"SFX VOL"),
                         L"[SFX_VOL]");
        y += rowH + rowGap;
        disabledMeterRow(y,
                         SettingsText(L"UI 볼륨", L"UI VOL"),
                         L"[UI_VOL]");
    } else {
        const float saveH = groupHead + rowH * 3.0f + rowGap * 2.0f + groupPadB;
        const float infoH = groupHead + rowH * 2.0f + rowGap + groupPadB;
        const float dangerH = groupHead + rowH + groupPadB;
        wchar_t bestBuf[128];
        swprintf_s(bestBuf, L"E %lld / N %lld / H %lld", g_BestScore[0], g_BestScore[1], g_BestScore[2]);
        wchar_t recordBuf[96];
        swprintf_s(recordBuf, L"KILLS %lld / RUNS %lld", g_TotalKills, g_TotalGames);
        wchar_t coinBuf[64];
        swprintf_s(coinBuf, L"%lld G", g_Coins);
        groupBox(y, saveH, SettingsText(L"저장 기록", L"SAVE DATA"), baseR, baseG, baseB);
        y += groupHead;
        systemLogRow(y, SettingsText(L"[ 별자리 상태 : 관측 기록 자동 저장 ]", L"[ STAR_STATUS : OBSERVATION AUTO-SAVE ACTIVE ]"));
        y += rowH + rowGap;
        infoRow(y, SettingsText(L"최고 기록", L"BEST RECORD"),
                L"[BEST_SCORE]",
                bestBuf);
        y += rowH + rowGap;
        infoRow(y, SettingsText(L"누적 기록", L"RUN RECORD"),
                L"[RUN_DATA]",
                recordBuf);

        y = rowStart + saveH + groupGap;
        groupBox(y, infoH, SettingsText(L"정보", L"INFO"), baseR, baseG, baseB);
        y += groupHead;
        infoRow(y, SettingsText(L"별가루", L"STARDUST"),
                L"[CREDIT_POOL]",
                coinBuf);
        y += rowH + rowGap;
        rowShellStd(y, SettingsText(L"제작진", L"CREDITS"),
                    L"[BUILD_INFO]");
        {
            float bx = rowX + rowW - ctlW - 16.0f * uiS;
            float by2 = y + (rowH - ctlH) * 0.5f;
            if (segment(bx, by2, ctlW, ctlH,
                        nli == 0 ? L"\xBCF4\xAE30" : L"View", false, baseR, baseG, baseB))
                s_showCredits = true;
        }

        y = rowStart + saveH + groupGap + infoH + groupGap;
        groupBox(y, dangerH, SettingsText(L"위험 구역", L"DANGER ZONE"), 1.0f, 0.35f, 0.35f, true);
        y += groupHead;
        rowShellStd(y, SettingsText(L"데이터 초기화", L"RESET DATA"),
                    L"[DANGER_ZONE]");
        {
            float bx = rowX + rowW - ctlW - 16.0f * uiS;
            float by2 = y + (rowH - ctlH) * 0.5f;
            bool rHov = hit(bx, by2, ctlW, ctlH);
            BindMainShader();
            BatchFlush(); SetGlowFx(true);
            drawConstellFrame(bx, by2, ctlW, ctlH, 1.0f, 0.35f, 0.35f,
                              rHov ? 0.90f : 0.52f, 10.0f*uiS, 3.0f*uiS);
            BatchFlush(); SetGlowFx(false);
            drawCenterS(nli == 0 ? L"\xCD08\xAE30\xD654 \xC120\xD0DD..." : L"Select Range...",
                        bx, by2 + 8.0f*uiS, ctlW, 0.70f*uiS, 1.0f, 0.55f, 0.55f, 0.96f);
            if (rHov && lmb && !g_LmbPrev) {
                s_resetDialog           = true;
                s_resetDialogJustOpened = true;
                s_resetDone   = false;
                s_resetItems[0] = s_resetItems[1] = s_resetItems[2] = s_resetItems[3] = false;
            }
        }
    }

    const float backW = 178.0f * uiS;
    const float footX = panelX + (panelW - backW) * 0.5f;
    const bool lmbMain = lmb && !modalActive;
    const float backH = 48.0f * uiS;
    const bool backHover = hit(footX, footY, backW, backH);
    if (backHover && lmbMain && !g_LmbPrev) {
        if (g_VolEdit) commitVol();
        SaveGame();
        g_GameManager.currentState = g_SettingsReturnTo;
    }
    // Main-menu style command text: retain the wide hitbox without a solid button.
    const float backA = (backHover ? 1.0f : 0.72f) * wake;
    g_TextL.Draw(L"<", footX + 8.0f*uiS, footY + 7.0f*uiS,
                 0.78f*uiS, baseR, baseG, baseB, backA);
    g_TextS.Draw(T(StrId::BTN_BACK), footX + 30.0f*uiS, footY + 13.0f*uiS,
                  0.62f*uiS, 0.82f, 0.92f, 1.0f, backA);
    if (backHover) {
        BindMainShader();
        drawRect(footX + 28.0f*uiS, footY + backH - 4.0f*uiS,
                 backW - 34.0f*uiS, 1.0f*uiS, baseR, baseG, baseB, 0.42f*backA);
    }

    if (s_resetDialog) {
        const float dW = 560.0f * uiS, dH = 380.0f * uiS;
        const float dX = (sw - dW) * 0.5f, dY = (sh - dH) * 0.5f;
        BindMainShader();
        BatchFlush(); SetGlowFx(true);
        drawConstellFrame(dX, dY, dW, dH, 1.0f, 0.35f, 0.35f, 0.72f, 16.0f*uiS, 5.0f*uiS, 0.22f);
        BatchFlush(); SetGlowFx(false);

        const wchar_t* dlgTitle = nli == 0 ? L"\xCD08\xAE30\xD654 \xBC94\xC704 \xC120\xD0DD" : L"SELECT RESET SCOPE";
        float tw = g_TextL.Width(dlgTitle, 0.88f*uiS);
        BindMainShader();
        g_TextL.Draw(dlgTitle, dX + (dW - tw) * 0.5f, dY + 18.0f*uiS, 0.88f*uiS, 1.0f, 0.55f, 0.55f, 0.98f);
        drawRect(dX + 20.0f*uiS, dY + 54.0f*uiS, dW - 40.0f*uiS, 1.0f*uiS, 1.0f, 0.4f, 0.4f, 0.30f);

        const wchar_t* itemLabels[2][4] = {
            { L"\xCD5C\xACE0 \xAE30\xB85D (\xB09C\xC774\xB3C4\xBCC4)", L"\xB204\xC801 \xAE30\xB85D (\xCD1D\xD569/\xD310)", L"\xBCF4\xC720 \xCF54\xC778", L"\xD574\xAE08 \xD56D\xBAA9" },
            { L"Best Score",        L"Run Records",        L"Coins",         L"Unlocks"   },
        };
        for (int i = 0; i < 4; ++i) {
            float iy = dY + 72.0f*uiS + (float)i * 56.0f*uiS;
            float cbX = dX + 28.0f*uiS, cbY = iy + 8.0f*uiS;
            float cbS = 26.0f*uiS;
            bool cbHov = hit(cbX, cbY, cbS, cbS);
            BindMainShader();
            BatchFlush(); SetGlowFx(true);
            drawConstellFrame(cbX, cbY, cbS, cbS, 1.0f, 0.40f, 0.40f,
                              s_resetItems[i] ? 0.90f : (cbHov ? 0.55f : 0.28f),
                              7.0f*uiS, 2.5f*uiS);
            BatchFlush(); SetGlowFx(false);
            if (s_resetItems[i]) {
                drawDiamond(cbX + cbS*0.5f, cbY + cbS*0.5f, cbS*0.45f, 1.0f, 0.55f, 0.55f, 0.92f);
            }
            if (cbHov && lmb && !g_LmbPrev) s_resetItems[i] = !s_resetItems[i];
            g_TextS.Draw(itemLabels[nli][i], cbX + cbS + 14.0f*uiS, iy + 12.0f*uiS,
                         0.52f*uiS, 0.92f, 0.92f, 1.0f, 0.90f);
        }

        const float btnY = dY + dH - 62.0f*uiS;
        const float btnH = 42.0f*uiS, btnW = 180.0f*uiS;
        const float cancelX = dX + dW * 0.5f - btnW - 12.0f*uiS;
        const float execX   = dX + dW * 0.5f + 12.0f*uiS;
        bool anyItem = s_resetItems[0] || s_resetItems[1] || s_resetItems[2] || s_resetItems[3];

        if (UIButton(cancelX, btnY, btnW, btnH, nli == 0 ? L"\xCDE8\xC18C" : L"Cancel",
                     mx, my, lmb, g_LmbPrev)) {
            s_resetDialog = false;
        }

        bool execHov = anyItem && hit(execX, btnY, btnW, btnH);
        BindMainShader();
        BatchFlush(); SetGlowFx(true);
        drawConstellFrame(execX, btnY, btnW, btnH, 1.0f, 0.35f, 0.35f,
                          anyItem ? (execHov ? 0.90f : 0.55f) : 0.18f,
                          10.0f*uiS, 3.0f*uiS);
        BatchFlush(); SetGlowFx(false);
        drawCenterS(nli == 0 ? L"\xCD08\xAE30\xD654 \xC2E4\xD589" : L"Confirm Reset",
                    execX, btnY + 12.0f*uiS, btnW, 0.46f*uiS,
                    1.0f, anyItem ? 0.50f : 0.35f, anyItem ? 0.50f : 0.35f,
                    anyItem ? 0.96f : 0.42f);

        if (execHov && anyItem && lmb && !g_LmbPrev) {
            if (s_resetItems[0]) { g_BestScore[0] = g_BestScore[1] = g_BestScore[2] = 0; }
            if (s_resetItems[1]) { g_TotalKills = 0; g_TotalGames = 0; }
            if (s_resetItems[2]) { g_Coins = 0; }
            if (s_resetItems[3]) { ResetSaveProgress(); }
            SaveGame();
            s_resetDialog = false;
            s_resetDone   = true;
        }

        bool justOpened = s_resetDialogJustOpened;
        s_resetDialogJustOpened = false;
        if (!justOpened && lmb && !g_LmbPrev && !hit(dX, dY, dW, dH))
            s_resetDialog = false;
    }

    if (s_resetDone) {
        const float nW = 460.0f*uiS, nH = 210.0f*uiS;
        const float nX = (sw - nW) * 0.5f, nY = (sh - nH) * 0.5f;
        BindMainShader();
        BatchFlush(); SetGlowFx(true);
        drawConstellFrame(nX, nY, nW, nH, 1.0f, 0.35f, 0.35f, 0.72f, 16.0f*uiS, 5.0f*uiS, 0.22f);
        BatchFlush(); SetGlowFx(false);
        const wchar_t* doneTitle = nli == 0 ? L"\xCD08\xAE30\xD654 \xC644\xB8CC" : L"Reset Complete";
        float dtw = g_TextL.Width(doneTitle, 0.92f*uiS);
        BindMainShader();
        g_TextL.Draw(doneTitle, nX + (nW - dtw)*0.5f, nY + 18.0f*uiS,
                     0.92f*uiS, 1.0f, 0.55f, 0.55f, 0.98f);
        drawRect(nX + 20.0f*uiS, nY + 60.0f*uiS, nW - 40.0f*uiS, 1.5f*uiS,
                 1.0f, 0.4f, 0.4f, 0.36f);
        const wchar_t* line1 = nli==0
            ? L"\xC120\xD0DD\xD55C \xAE30\xB85D\xC774 \xCD08\xAE30\xD654\xB418\xC5C8\xC2B5\xB2C8\xB2E4."
            : L"Selected records have been reset.";
        const wchar_t* line2 = nli==0
            ? L"\xBCC0\xACBD \xC0AC\xD56D\xC744 \xC801\xC6A9\xD558\xB824\xBA74 \xAC8C\xC784\xC744 \xC7AC\xC2DC\xC791\xD558\xC138\xC694."
            : L"Restart the game to apply changes.";
        float sc1 = 0.46f*uiS;
        while (sc1 > 0.32f*uiS && g_TextS.Width(line1, sc1) > nW - 60.0f*uiS) sc1 -= 0.02f*uiS;
        float sc2 = 0.40f*uiS;
        while (sc2 > 0.28f*uiS && g_TextS.Width(line2, sc2) > nW - 60.0f*uiS) sc2 -= 0.02f*uiS;
        g_TextS.Draw(line1, nX + (nW - g_TextS.Width(line1, sc1))*0.5f,
                     nY + 74.0f*uiS, sc1, 0.88f, 0.92f, 1.0f, 0.86f);
        g_TextS.Draw(line2, nX + (nW - g_TextS.Width(line2, sc2))*0.5f,
                     nY + 104.0f*uiS, sc2, 0.50f, 0.58f, 0.72f, 0.62f);
        const float qW = 160.0f*uiS, qH = 44.0f*uiS;
        const float qX = nX + (nW - qW)*0.5f, qY = nY + nH - 60.0f*uiS;
        bool qHov = hit(qX, qY, qW, qH);
        BindMainShader();
        BatchFlush(); SetGlowFx(true);
        drawConstellFrame(qX, qY, qW, qH, 1.0f, 0.35f, 0.35f,
                          qHov ? 0.90f : 0.55f, 10.0f*uiS, 3.0f*uiS);
        BatchFlush(); SetGlowFx(false);
        drawCenterS(nli==0 ? L"\xC885\xB8CC" : L"Quit", qX, qY + 12.0f*uiS, qW,
                    0.48f*uiS, 1.0f, 0.60f, 0.60f, 0.96f);
        if (qHov && lmb && !g_LmbPrev)
            glfwSetWindowShouldClose(window, 1);
    }

    if (s_showCredits) {
        BindMainShader();
        float cw = 660.0f * uiS;
        float ch = 456.0f * uiS;
        float cx = (sw - cw) * 0.5f;
        float cy = (sh - ch) * 0.5f;
        drawBorder(cx, cy, cw, ch, 0.35f, 0.82f, 1.0f, 0.46f, 1.5f * uiS);
        const wchar_t* ct = L"CREDITS";
        g_TextL.Draw(ct, cx + (cw - g_TextL.Width(ct, 1.04f * uiS)) * 0.5f,
                     cy + 28.0f * uiS, 1.04f * uiS, 1.0f, 1.0f, 1.0f, 0.98f);
        const wchar_t* lines[] = {
            L"ONEDOW - ASTRAL DEFENSE",
            L"Fonts: Jua / Kosugi Maru / Chakra Petch / Orbit",
            L"Icons: game-icons.net",
            L"Audio engine: miniaudio",
            L"Built with OpenGL, GLFW, GLAD, glm, stb",
            L"Made with Claude Code",
        };
        float ly = cy + 88.0f * uiS;
        for (auto* ln : lines) {
            drawFitS(ln, cx + 42.0f * uiS, ly, cw - 84.0f * uiS,
                     0.54f * uiS, 0.40f * uiS, 0.82f, 0.90f, 1.0f, 0.90f);
            ly += 38.0f * uiS;
        }
        if (UIButton(cx + (cw - 180.0f * uiS) * 0.5f, cy + ch - 62.0f * uiS,
                     180.0f * uiS, 44.0f * uiS, T(StrId::BTN_BACK),
                     mx, my, lmb, g_LmbPrev))
            s_showCredits = false;
    }
}

static void Scene_Settings_Legacy_UNUSED(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    GLFWwindow* window = c.window;
    const GameState st = g_GameManager.currentState;
    float& fireTimer = *c.fireTimer;
    const std::function<void()>& ResetForNewGame = c.reset;
                const float WW = std::min(sw * 0.88f, 1220.0f);
                const float WH = std::min(sh * 0.86f, 780.0f);
                float wx, wy;
                const bool settingsOverlay = (g_SettingsReturnTo == GameState::PAUSED);
                if (!settingsOverlay) DrawMenuBackground(sw, sh, delta);
                SceneAppWindow(sw, sh, WW, WH, L"OBSERVATORY", 0.70f, 0.75f, 0.88f, wx, wy,
                               settingsOverlay, 0.0f);
                if (g_AppOpen >= 0.999f) {           // 완전히 열린 뒤에만 콘텐츠
                // ── 중앙 정렬 2열 그리드 (좌: 표시·그래픽 / 우: 조작·효과) ──
                const float OW = 112.0f, OH = 44.0f, OG = 8.0f;
                const float rowH = 58.0f;
                const float contentW = 980.0f;
                const float colW = (contentW - 48.0f) * 0.5f;
                const float cx0 = wx + (WW - contentW) * 0.5f;
                const float colL = cx0;
                const float colR = cx0 + colW + 48.0f;
                const float btnXL = 236.0f;   // 좌열 라벨 폭 (긴 한글 라벨용)
                const float btnXR = 188.0f;

                g_TextL.Draw(T(StrId::SET_TITLE), cx0, wy + 44.0f, 1.05f, 1, 1, 1, 1);

                auto toggleAt = [&](float labX, float btnBase, float ly,
                                    const wchar_t* label, bool& val, float labMaxW) {
                    float lsc = 0.85f;
                    while (lsc > 0.62f && g_TextS.Width(label, lsc) > labMaxW) lsc -= 0.03f;
                    g_TextS.Draw(label, labX, ly + 14.0f, lsc, 1, 1, 1, 0.9f);
                    if (UIButton(btnBase, ly, OW, OH, T(StrId::OPT_ON),
                                 mx, my, lmb, g_LmbPrev, val)) val = true;
                    if (UIButton(btnBase + OW + OG, ly, OW, OH, T(StrId::OPT_OFF),
                                 mx, my, lmb, g_LmbPrev, !val)) val = false;
                };

                // FPS (전체 폭)
                float lineY = wy + 96.0f;
                g_TextS.Draw(T(StrId::SET_FPS), colL, lineY + 12.0f, 0.85f, 1, 1, 1, 0.9f);
                struct FpsOpt { const wchar_t* label; int val; };
                FpsOpt fpsOpts[4] = {
                    { L"30", 30 }, { L"60", 60 }, { L"144", 144 }, { L"300", 300 }
                };
                for (int i = 0; i < 4; i++) {
                    float bx = colL + btnXL + i * (OW + OG);
                    bool sel = (g_FpsCap == fpsOpts[i].val);
                    if (UIButton(bx, lineY, OW, OH, fpsOpts[i].label,
                                 mx, my, lmb, g_LmbPrev, sel)) {
                        g_FpsCap = fpsOpts[i].val;
                        glfwSwapInterval((g_FpsCap == 0) ? 1 : 0);
                    }
                }

                // 언어 (전체 폭)
                lineY = wy + 96.0f + rowH;
                g_TextS.Draw(T(StrId::SET_LANG), colL, lineY + 12.0f, 0.85f, 1, 1, 1, 0.9f);
                struct LangOpt { const wchar_t* label; Language lang; };
                LangOpt langOpts[LANG_COUNT] = {
                    { L"한국어",  Language::KR },
                    { L"English", Language::EN },
                    { L"日本語",  Language::JP },
                };
                for (int i = 0; i < LANG_COUNT; i++) {
                    float bx = colL + btnXL + i * (OW + OG);
                    bool sel = (g_Language == langOpts[i].lang);
                    if (UIButton(bx, lineY, OW, OH, langOpts[i].label,
                                 mx, my, lmb, g_LmbPrev, sel)) {
                        g_Language = langOpts[i].lang;
                    }
                }

                const wchar_t* afLabel = (g_Language==Language::EN)?L"Auto-Fire":
                                         (g_Language==Language::JP)?L"自動発射":L"자동 발사";
                const wchar_t* asLabel = (g_Language==Language::EN)?L"Auto-Skill":
                                         (g_Language==Language::JP)?L"自動スキル":L"자동 스킬";
                const wchar_t* sfLabel = (g_Language==Language::EN)?L"CRT Shader":
                                         (g_Language==Language::JP)?L"CRTシェーダー":L"CRT 셰이더";
                float tY0 = wy + 96.0f + rowH * 2;
                toggleAt(colL, colL + btnXL, tY0,            T(StrId::SET_CROSSHAIR), g_ShowCrosshair, btnXL - 8.0f);
                toggleAt(colL, colL + btnXL, tY0 + rowH,     T(StrId::SET_COMBO),     g_ShowCombo, btnXL - 8.0f);
                toggleAt(colR, colR + btnXR, tY0,            afLabel, g_AutoFire, btnXR - 8.0f);
                toggleAt(colR, colR + btnXR, tY0 + rowH,     asLabel, g_AutoSkill, btnXR - 8.0f);
                toggleAt(colR, colR + btnXR, tY0 + rowH * 2, sfLabel, g_ShaderFx, btnXR - 8.0f);

                // 몹 외형 · 위성 VFX
                float gfxY = tY0 + rowH * 3;
                const wchar_t* mobLabel = (g_Language==Language::EN)?L"Mob look":
                                          (g_Language==Language::JP)?L"敵見た目":L"몹 외형";
                const wchar_t* mobA = (g_Language==Language::EN)?L"Classic":
                                      (g_Language==Language::JP)?L"クラシック":L"기본";
                const wchar_t* mobB = (g_Language==Language::EN)?L"Soft":
                                      (g_Language==Language::JP)?L"ソフト":L"부드럽";
                g_TextS.Draw(mobLabel, colL, gfxY + 12.0f, 0.85f, 1, 1, 1, 0.9f);
                if (UIButton(colL + btnXL, gfxY, OW, OH, mobA, mx, my, lmb, g_LmbPrev,
                             g_MobVisualStyle == MobVisualStyle::CLASSIC))
                    g_MobVisualStyle = MobVisualStyle::CLASSIC;
                if (UIButton(colL + btnXL + OW + OG, gfxY, OW, OH, mobB, mx, my, lmb, g_LmbPrev,
                             g_MobVisualStyle == MobVisualStyle::SOFT))
                    g_MobVisualStyle = MobVisualStyle::SOFT;

                const wchar_t* vfxLabel = (g_Language==Language::EN)?L"Satellite VFX":
                                          (g_Language==Language::JP)?L"衛星VFX":L"위성 VFX";
                const wchar_t* vfxA = (g_Language==Language::EN)?L"Full":
                                      (g_Language==Language::JP)?L"通常":L"보통";
                const wchar_t* vfxB = (g_Language==Language::EN)?L"Reduced":
                                      (g_Language==Language::JP)?L"節約":L"절약";
                g_TextS.Draw(vfxLabel, colR, gfxY + 12.0f, 0.85f, 1, 1, 1, 0.9f);
                if (UIButton(colR + btnXR, gfxY, OW, OH, vfxA, mx, my, lmb, g_LmbPrev,
                             g_VfxDensity == VfxDensity::FULL))
                    g_VfxDensity = VfxDensity::FULL;
                if (UIButton(colR + btnXR + OW + OG, gfxY, OW, OH, vfxB, mx, my, lmb, g_LmbPrev,
                             g_VfxDensity == VfxDensity::REDUCED))
                    g_VfxDensity = VfxDensity::REDUCED;

                // 사운드 볼륨 — 전체 폭 한 줄 (그래픽 행과 분리)
                {
                    float vy = gfxY + rowH + 8.0f;
                    g_TextS.Draw(T(StrId::SET_SOUND), colL, vy + 12.0f, 0.85f, 1, 1, 1, 0.9f);
                    auto clampVol = [](int v){ return v < 0 ? 0 : (v > 100 ? 100 : v); };
                    float bx0 = colL + btnXL;

                    // [−]
                    if (UIButton(bx0, vy, 44.0f, OH, L"−", mx, my, lmb, g_LmbPrev)) {
                        g_VolEdit = false; g_SoundVol = clampVol(g_SoundVol - 5);
                    }
                    // 슬라이더 (트랙 + 손잡이) — 클릭/드래그로 직접 설정
                    float barX = bx0 + 56.0f, barW = 300.0f;
                    float trackY = vy + OH * 0.5f, trackH = 6.0f;
                    float kx = barX + barW * (g_SoundVol / 100.0f);
                    drawRect(barX, trackY - trackH*0.5f, barW, trackH, 0.10f, 0.12f, 0.16f, 1.0f);     // 트랙
                    drawRect(barX, trackY - trackH*0.5f, kx - barX, trackH, 0.35f, 0.75f, 1.0f, 0.95f); // 채움
                    drawCircle(kx, trackY, 11.0f, 0.35f, 0.8f, 1.0f, 1.0f);    // 손잡이 외곽
                    drawCircle(kx, trackY,  6.0f, 0.95f, 0.98f, 1.0f, 1.0f);   // 손잡이 코어
                    bool barHover = (mx >= barX && mx <= barX + barW && my >= vy && my <= vy + OH);
                    if (lmb && barHover) {   // 누르는 동안(드래그) 마우스 X 로 값 설정
                        g_VolEdit = false;
                        g_SoundVol = clampVol((int)((float)(mx - barX) / barW * 100.0f + 0.5f));
                    }
                    // [+]
                    float plusX = barX + barW + 8.0f;
                    if (UIButton(plusX, vy, 44.0f, OH, L"+", mx, my, lmb, g_LmbPrev)) {
                        g_VolEdit = false; g_SoundVol = clampVol(g_SoundVol + 5);
                    }
                    // 숫자 직접입력 필드
                    float fldX = plusX + 44.0f + 14.0f, fldW = 92.0f;
                    bool fldHover = (mx >= fldX && mx <= fldX + fldW && my >= vy && my <= vy + OH);
                    BindMainShader();
                    drawRect(fldX, vy, fldW, OH, g_VolEdit ? 0.16f : 0.09f,
                             g_VolEdit ? 0.18f : 0.10f, 0.22f, 1.0f);
                    drawRect(fldX, vy, fldW, 2.0f, 0.4f, 0.9f, 0.6f, 0.8f);
                    auto commitVol = [&]() {
                        int v = 0; for (int i = 0; i < g_VolLen; i++) v = v*10 + (g_VolBuf[i]-L'0');
                        if (g_VolLen > 0) g_SoundVol = clampVol(v);
                        g_VolEdit = false;
                    };
                    if (lmb && !g_LmbPrev) {
                        if (fldHover) { g_VolEdit = true; g_VolLen = 0; g_VolBuf[0] = 0; }
                        else if (g_VolEdit) commitVol();   // 다른 곳 클릭 = 확정
                    }
                    // 백스페이스 / 엔터 (엣지 감지)
                    {
                        static bool bsPrev = false, enPrev = false;
                        bool bs = (glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS);
                        if (g_VolEdit && bs && !bsPrev && g_VolLen > 0) g_VolBuf[--g_VolLen] = 0;
                        bsPrev = bs;
                        bool en = (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS);
                        if (g_VolEdit && en && !enPrev) commitVol();
                        enPrev = en;
                    }
                    wchar_t shown[16];
                    if (g_VolEdit) {
                        bool caret = (((int)(glfwGetTime()*2.0)) & 1) == 0;
                        swprintf_s(shown, L"%ls%ls", g_VolLen ? g_VolBuf : L"", caret ? L"|" : L"");
                    } else swprintf_s(shown, L"%d", g_SoundVol);
                    g_TextS.Draw(shown, fldX + 12.0f, vy + 12.0f, 0.9f, 1,1,1,0.95f);
                }

                // PC 사양 — 베타 피드백용 (설정 하단)
                {
                    const wchar_t* specTitle = (g_Language==Language::EN)?L"Telemetry profile":
                                               (g_Language==Language::JP)?L"観測テレメトリ":
                                               L"관측 텔레메트리";
                    float sy = wy + WH - 118.0f;
                    g_TextS.Draw(specTitle, cx0, sy, 0.72f, 0.62f, 0.72f, 0.82f, 0.88f);
                    int fw = 0, fh = 0;
                    glfwGetFramebufferSize(window, &fw, &fh);
                    std::wstring s1 = GetSystemSpecLine1();
                    std::wstring s2 = GetSystemSpecLine2(fw, fh);
                    float specSc = 0.60f;
                    float maxW = contentW;
                    while (specSc > 0.48f && g_TextS.Width(s1.c_str(), specSc) > maxW) specSc -= 0.02f;
                    g_TextS.Draw(s1.c_str(), cx0, sy + 20.0f, specSc, 0.78f, 0.88f, 0.98f, 0.90f);
                    g_TextS.Draw(s2.c_str(), cx0, sy + 38.0f, specSc, 0.72f, 0.82f, 0.92f, 0.85f);
                }

                // 하단 버튼 — 중앙 정렬
                const float footY = wy + WH - 64.0f;
                const float backW = 180.0f, resetW = 210.0f, credW = 150.0f;
                const float footGap = 16.0f;
                const float footTotal = backW + resetW + credW + footGap * 2.0f;
                const float footX = wx + (WW - footTotal) * 0.5f;
                if (UIButton(footX, footY, backW, 48.0f, T(StrId::BTN_BACK),
                             mx, my, lmb, g_LmbPrev)) {
                    SaveGame();
                    g_GameManager.currentState = g_SettingsReturnTo;
                }
                // 세이브 초기화 (2단계 확인) — 점수/코인/메타/업적/도감/테마 전부 리셋
                {
                    static bool s_resetConfirm = false;
                    const wchar_t* rl = s_resetConfirm
                        ? ((g_Language==Language::EN)?L"Sure? (click again)":
                           (g_Language==Language::JP)?L"本当に？(再クリック)":L"정말? (다시 클릭)")
                        : ((g_Language==Language::EN)?L"Reset Save":
                           (g_Language==Language::JP)?L"セーブ初期化":L"세이브 초기화");
                    float rx = footX + backW + footGap, ry = footY, rwid = resetW;
                    bool rh = (mx>=rx && mx<=rx+rwid && my>=ry && my<=ry+48.0f);
                    BindMainShader();
                    drawRect(rx, ry, rwid, 48.0f, s_resetConfirm?0.40f:0.18f, 0.06f, 0.06f, rh?1.0f:0.9f);
                    drawRect(rx, ry, rwid, 2.0f, 0.95f, 0.3f, 0.3f, 0.9f);
                    float rtw = g_TextS.Width(rl, 0.82f);
                    g_TextS.Draw(rl, rx+(rwid-rtw)*0.5f, ry+15.0f, 0.82f, 1.0f, 0.65f, 0.6f, 1.0f);
                    if (lmb && !g_LmbPrev) {
                        if (rh) {
                            if (!s_resetConfirm) s_resetConfirm = true;
                            else { ResetSaveProgress(); s_resetConfirm = false; }
                        } else s_resetConfirm = false;   // 딴 곳 클릭 = 확인 취소
                    }
                }

                // 크레딧 (오픈소스 에셋 출처) — 버튼 → 오버레이
                static bool s_showCredits = false;
                {
                    const wchar_t* cl = (g_Language==Language::EN)?L"Credits":
                                        (g_Language==Language::JP)?L"クレジット":L"크레딧";
                    float cxp = footX + backW + resetW + footGap * 2.0f;
                    if (UIButton(cxp, footY, credW, 48.0f, cl, mx, my, lmb, g_LmbPrev))
                        s_showCredits = true;
                }
                if (s_showCredits) {
                    BindMainShader();
                    drawRect(0, 0, sw, sh, 0.0f, 0.0f, 0.0f, 0.78f * SCENE_BG_ALPHA);   // 딤
                    float CW = 620.0f, CH = 440.0f;
                    float CX = (sw - CW) * 0.5f, CY = (sh - CH) * 0.5f;
                    drawRect(CX, CY, CW, CH, 0.05f, 0.06f, 0.10f, 0.98f);
                    drawRect(CX, CY, CW, 4.0f, 0.3f, 0.8f, 1.0f, 1.0f);
                    const wchar_t* CT = L"CREDITS";
                    g_TextL.Draw(CT, CX + (CW - g_TextL.Width(CT,1.1f))*0.5f, CY + 24.0f, 1.1f, 1,1,1,1);
                    const wchar_t* lines[] = {
                        L"ONEDOW  —  ASTRAL DEFENSE",
                        L"",
                        L"Fonts:  Jua / Kosugi Maru  (SIL OFL)",
                        L"Icons:  game-icons.net  (CC BY 3.0)",
                        L"         Lorc · Delapouite · Skoll",
                        L"Missile sprite:  Saepul Nahwan  (Noun Project)",
                        L"Audio engine:  miniaudio  (public domain)",
                        L"Built with:  OpenGL · GLFW · GLAD · glm · stb",
                        L"",
                        L"Made with Claude Code",
                    };
                    float ly = CY + 78.0f;
                    for (auto* ln : lines) {
                        g_TextS.Draw(ln, CX + 36.0f, ly, 0.82f, 0.85f, 0.92f, 1.0f, 0.95f);
                        ly += 32.0f;
                    }
                    if (UIButton(CX + (CW-180.0f)*0.5f, CY + CH - 60.0f, 180.0f, 44.0f,
                                 T(StrId::BTN_BACK), mx, my, lmb, g_LmbPrev))
                        s_showCredits = false;
                }
                }   // close: g_AppOpen open guard
}

void Scene_Ready(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    (void)c;

    auto drawSlashText = [&](const wchar_t* text, float y, float sc, float r, float g, float b, float a) {
        if (!text || !text[0]) return;
        std::vector<std::wstring> lines;
        std::wstring cur;
        for (const wchar_t* p = text; *p; ++p) {
            if (*p == L'/') { if (!cur.empty()) lines.push_back(cur); cur.clear(); }
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
                                          delta, 26.0f);
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
void Scene_GameOverLegacy(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    GLFWwindow* window = c.window;
    const GameState st = g_GameManager.currentState;
    float& fireTimer = *c.fireTimer;
    const std::function<void()>& ResetForNewGame = c.reset;
                // ── 메뉴 페이드인: 폭발 직후 결과 메뉴가 서서히 떠오름 (0→1, 2.5초) ──
                float gof = g_GameOverFade;          // 0..1
                float ge  = Smoothstep(gof);  // smoothstep (부드럽게)

                // 전체 화면 딤 — 보스 창/엔티티가 결과창 뒤로 비치지 않게 (페이드)
                BindMainShader();
                drawRect(0, 0, sw, sh, 0.02f, 0.02f, 0.06f, 0.88f * ge);
                const wchar_t* T1 = T(StrId::GAMEOVER);
                g_TextL.Draw(T1, CenterTextX(sw, g_TextL, T1, 1.6f), sh*0.30f, 1.6f,
                             1, 0.25f, 0.25f, 0.95f * ge);
                // 사망 원인 (프로세스 종료 사유)
                if (g_DeathReason[0]) {
                    g_TextS.Draw(g_DeathReason, CenterTextX(sw, g_TextS, g_DeathReason, 1.0f), sh*0.385f, 1.0f,
                                 1.0f, 0.55f, 0.45f, 0.92f * ge);
                }

                // 결과 — 점수 카운트업(띠리릭) + Best(통계 바로 위) + 레벨/처치/코인
                float cu = gof / 0.75f; if (cu > 1.0f) cu = 1.0f;
                cu = Smoothstep(cu);          // smoothstep → 숫자 롤업 느낌
                long long curScore = (long long)(g_GameManager.score * cu);
                long long bestVal  = g_BestScore[(int)g_Difficulty];
                long long curBest  = g_LastRunRecord ? (long long)(bestVal * cu) : bestVal;  // 신기록이면 같이 롤업

                wchar_t bestBuf[64], scoreBuf[64], lvBuf[64], killBuf[64], coinBuf[64];
                // BEST — 통계(최종점수) 바로 위
                swprintf_s(bestBuf, L"BEST   %lld", curBest);
                g_TextS.Draw(bestBuf, CenterTextX(sw, g_TextS, bestBuf, 1.1f), sh*0.405f, 1.1f,
                             1.0f, 0.85f, 0.4f, 0.95f * ge);
                // 최종 점수 (카운트업, 대)
                swprintf_s(scoreBuf, L"%ls   %lld", T(StrId::FINAL_SCORE), curScore);
                g_TextL.Draw(scoreBuf, CenterTextX(sw, g_TextL, scoreBuf, 1.3f), sh*0.455f, 1.3f,
                             1.0f, 1.0f, 0.7f, 0.95f * ge);
                // 도달 레벨 / 처치 수
                swprintf_s(lvBuf,   L"%ls   Lv. %d", T(StrId::REACHED_LEVEL), g_GameManager.playerLevel);
                swprintf_s(killBuf, L"%ls   %lld",   T(StrId::KILL_COUNT),    g_Stats.killCount);
                g_TextS.Draw(lvBuf,   CenterTextX(sw, g_TextS, lvBuf, 1.05f), sh*0.545f, 1.05f, 0.85f,0.95f,0.85f, 0.95f*ge);
                g_TextS.Draw(killBuf, CenterTextX(sw, g_TextS, killBuf, 1.05f), sh*0.59f,  1.05f, 0.85f,0.95f,0.85f, 0.95f*ge);
                // 코인
                swprintf_s(coinBuf, L"+%lld COIN  (total %lld)", g_LastRunCoins, g_Coins);
                g_TextS.Draw(coinBuf, CenterTextX(sw, g_TextS, coinBuf, 1.0f), sh*0.645f, 1.0f, 1.0f,0.9f,0.3f, 0.95f*ge);
                if (g_LastRunRecord) {
                    const wchar_t* rec = L"* NEW RECORD *";
                    float blink = 0.6f + 0.4f * sinf((float)glfwGetTime() * 6.0f);
                    g_TextL.Draw(rec, CenterTextX(sw, g_TextL, rec, 1.0f), sh*0.355f, 1.0f,
                                 1.0f, 0.9f, 0.2f, blink * ge);
                }

                // (사망 시점 보유 증강은 Scene_OwnedAugPanel — 일시정지와 동일한 호버 패널로 표시.
                //  기존 다열 목록은 제거: 패널과 겹쳐 이중 표시되던 문제 fix)

                // 버튼 3개 — 메인메뉴/일시정지와 동일한 별자리 좌정렬 스타일
                {
                    static float s_hovGO[3] = {};
                    const float BW  = std::min(480.0f, sw * 0.36f);
                    const float BH  = 68.0f, BGAP = 12.0f;
                    const float bX0 = std::max(58.0f, sw * 0.075f);
                    const float bY0 = sh * 0.715f;
                    const float ar  = 0.48f, ag = 0.82f, ab = 1.0f;
                    const float fnow = (float)glfwGetTime();
                    const float showT = gof >= 0.999f ? 1.0f : 0.0f;

                    static const wchar_t* kRoute[3] = { L"RESTART",      L"RETURN_TO_HUB", L"TERMINATE" };
                    const wchar_t*        kSub[3]   = { T(StrId::BTN_RESTART), T(StrId::BTN_MAIN_MENU), T(StrId::BTN_QUIT) };

                    // 앵커 라인
                    if (showT > 0.0f) {
                        const float ancX = bX0 - 36.0f, ancY = bY0 - 8.0f;
                        const float ancH = (float)(3 - 1) * (BH + BGAP) + BH + 16.0f;
                        BindMainShader();
                        drawRect(ancX, ancY, 1.2f, ancH, ar,ag,ab, 0.15f * ge);
                        drawDiamond(ancX + 0.6f, ancY + 4.0f,       2.8f, ar,ag,ab, 0.20f * ge);
                        drawDiamond(ancX + 0.6f, ancY + ancH - 4.0f, 2.8f, ar,ag,ab, 0.16f * ge);
                    }

                    for (int i = 0; i < 3; ++i) {
                        float by2 = bY0 + (float)i * (BH + BGAP);
                        bool hov = (showT > 0.0f &&
                                    mx >= bX0 - 26.0f && mx <= bX0 + BW + 32.0f &&
                                    my >= by2 && my <= by2 + BH);
                        s_hovGO[i] = UpdateMenuCommandHover(s_hovGO[i], hov, delta);
                        float t   = s_hovGO[i];
                        float rowA = showT * ge;
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
    const float metricY = reportY + 28.0f;
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
                 metricY + metricRowH * 2.0f + 60.0f, 0.50f,
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
    const int playerJob = (g_SelectedJob >= 0 && g_SelectedJob < JOB_COUNT)
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
    const float statsBottom = metricY + metricRowH * 3.0f + 8.0f;
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

struct TarotBurstParticle {
    float x = 0.0f, y = 0.0f;
    float vx = 0.0f, vy = 0.0f;
    float life = 0.0f;
    float r = 1.0f, g = 1.0f, b = 1.0f;
};

static void Scene_AugSelectCards(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh, delta = c.delta;
    const GameState state = g_GameManager.currentState;
    const int nCards = std::max(0, std::min(3, g_GameManager.augChoiceCount));
    const float now = (float)glfwGetTime();
    const bool isDebuff = state == GameState::DEBUFF_SELECT;
    const bool inExit = g_AugExitT >= 0.0f;

    static int seenSerial = -1;
    static float enterT = 0.0f;
    static float hoverT[3] = {};
    static float flashT[3] = {};
    static bool lmbPrev = false;
    static int prevHover = -1;
    static double prevMx = -1.0, prevMy = -1.0;
    static bool burstSpawned = false;
    static TarotBurstParticle burst[96];

    if (seenSerial != g_GameManager.augmentSelectionSerial) {
        seenSerial = g_GameManager.augmentSelectionSerial;
        enterT = 0.0f;
        prevHover = -1;
        prevMx = c.mx;
        prevMy = c.my;
        lmbPrev = false;
        burstSpawned = false;
        for (int i = 0; i < 3; ++i) { hoverT[i] = 0.0f; flashT[i] = 0.0f; }
        for (auto& p : burst) p.life = 0.0f;
    }

    if (!inExit) enterT = std::min(1.0f, enterT + delta / 0.48f);
    const float enterE = TarotEaseOut(enterT);
    const bool inputReady = enterT >= 0.98f && !inExit;
    const bool pointerMoved = prevMx < 0.0 ||
        fabs(c.mx - prevMx) > 0.5 || fabs(c.my - prevMy) > 0.5;
    if (pointerMoved) g_GameManager.augmentKeyboardFocus = false;
    prevMx = c.mx;
    prevMy = c.my;

    const float ui = std::min(1.0f, std::min(sw / 1800.0f, sh / 1000.0f));
    const float cardW = 340.0f * ui;
    const float cardH = 500.0f * ui;
    const float centerX = sw * 0.50f;
    const float centerY = sh * 0.535f;
    const float panelW = std::min(sw * 0.86f, 1040.0f * ui);
    const float panelX = (sw - panelW) * 0.5f;
    const float panelY = centerY + cardH * 0.5f + 24.0f * ui;
    const float panelAvailable = sh - panelY - 34.0f * ui;
    const float panelH = std::max(100.0f, std::min(222.0f * ui, panelAvailable));

    TarotCardPose base[3];
    for (int i = 0; i < nCards; ++i)
        base[i] = TarotBasePose(i, nCards, sw, sh, cardW, cardH, centerX, centerY);

    if (inputReady && (!g_GameManager.augmentKeyboardFocus || pointerMoved)) {
        int hover = -1;
        for (int i = nCards - 1; i >= 0; --i) {
            if (TarotHit(base[i], c.mx, c.my, 12.0f)) { hover = i; break; }
        }
        g_HoveredAug = hover;
    }
    if (g_HoveredAug != prevHover && g_HoveredAug >= 0 && g_HoveredAug < nCards)
        flashT[g_HoveredAug] = 1.0f;
    prevHover = g_HoveredAug;

    for (int i = 0; i < 3; ++i) {
        const float target = inputReady && g_HoveredAug == i ? 1.0f : 0.0f;
        hoverT[i] += (target - hoverT[i]) * std::min(1.0f, delta * 11.0f);
        flashT[i] = std::max(0.0f, flashT[i] - delta / 0.22f);
    }

    if (inputReady && c.lmb && !lmbPrev &&
        g_HoveredAug >= 0 && g_HoveredAug < nCards) {
        g_GameManager.augmentKeyboardFocus = false;
        g_AugExitT = 0.0f;
        g_AugExitSlot = g_HoveredAug;
    }
    lmbPrev = c.lmb;

    float overlay = TarotEaseOut(std::min(1.0f, enterT * 1.35f));
    if (inExit) overlay *= std::max(0.0f, 1.0f - TarotClamp01(g_AugExitT / 0.72f));
    BindMainShader();
    drawRect(0, 0, sw, sh, 0.004f, 0.006f, 0.016f, overlay * 0.72f);
    drawConstellFrame(16.0f, 16.0f, sw - 32.0f, sh - 32.0f,
                      isDebuff ? 0.90f : 0.30f, isDebuff ? 0.20f : 0.62f,
                      isDebuff ? 0.20f : 0.92f, overlay * 0.66f,
                      24.0f, 8.0f, overlay * 0.30f, enterT);
    BatchFlush();

    const wchar_t* title = isDebuff ? T(StrId::CHOOSE_DEBUFF) : T(StrId::CHOOSE_AUG);
    float titleAlpha = overlay * std::max(0.0f, 1.0f - (inExit ? g_AugExitT * 2.0f : 0.0f));
    g_TextL.Draw(title, CenterTextX(sw, g_TextL, title, 1.0f), sh * 0.065f,
                 1.0f, 1.0f, 1.0f, 1.0f, titleAlpha);
    if (!isDebuff) {
        wchar_t slots[64];
        swprintf_s(slots, L"ID %d / %d", CountIdentitySlotsUsed(), IdentitySlotMax());
        g_TextS.Draw(slots, CenterTextX(sw, g_TextS, slots, 0.70f),
                     sh * 0.065f + 34.0f, 0.70f,
                     0.72f, 0.92f, 1.0f, titleAlpha * 0.84f);
    }
    BatchFlush();

    for (int i = 0; i < nCards; ++i) {
        const AugDef& def = ALL_AUGS[g_GameManager.augChoices[i]];
        float r, g, b;
        GetRarityColor(def.rarity, r, g, b);
        const bool hovered = inputReady && g_HoveredAug == i;
        const bool selected = inExit && g_AugExitSlot == i;
        const bool other = inExit && !selected;
        const float hT = TarotEaseOut(hoverT[i]);
        const float stagger = TarotEaseOut(TarotClamp01((enterT - i * 0.08f) / 0.78f));
        TarotCardPose p = base[i];
        p.cx = centerX + (p.cx - centerX) * stagger;
        p.cy = centerY + (p.cy - centerY) * stagger + (1.0f - stagger) * 90.0f;
        p.cy -= hT * 18.0f;
        p.w *= 1.0f + hT * 0.045f;
        p.h *= 1.0f + hT * 0.045f;
        p.angle *= 1.0f - hT * 0.20f;
        float alpha = stagger * (g_HoveredAug >= 0 && g_HoveredAug != i ? 0.42f : 1.0f);

        if (selected) {
            const float t = TarotEaseOut(TarotClamp01(g_AugExitT / 0.72f));
            p.cx += (sw * 0.50f - p.cx) * t;
            p.cy += (sh * 0.49f - p.cy) * t;
            p.w *= 1.0f + t * 0.42f;
            p.h *= 1.0f + t * 0.42f;
            p.angle *= 1.0f - t;
            const float fade = TarotClamp01((g_AugExitT - 0.56f) / 0.54f);
            alpha *= 1.0f - fade * 0.92f;
        } else if (other) {
            const float t = TarotEaseOut(TarotClamp01(g_AugExitT / 0.42f));
            p.cy += (i % 2 == 0 ? 1.0f : -1.0f) * t * 44.0f;
            p.cx += (i - g_AugExitSlot) * t * 20.0f;
            alpha *= 1.0f - t;
        }
        if (alpha <= 0.01f) continue;

        DrawTarotCardSurface(p, r, g, b, alpha, hT, flashT[i], selected, now);
        const float left = p.cx - p.w * 0.5f;
        const float top = p.cy - p.h * 0.5f;
        const float topBar = std::max(38.0f, p.w * 0.16f);
        const wchar_t* badge = GetAugBadge(def);
        const float badgeScale = 0.62f;
        const float badgeW = g_TextS.Width(badge, badgeScale);
        g_TextS.Draw(badge, p.cx - badgeW * 0.5f, top + topBar * 0.28f,
                     badgeScale,
                     std::min(1.0f, r * 1.45f + 0.20f),
                     std::min(1.0f, g * 1.45f + 0.20f),
                     std::min(1.0f, b * 1.45f + 0.20f), alpha * 0.94f);
        wchar_t key[16];
        swprintf_s(key, L"[%d]", i + 1);
        const float keyW = g_TextS.Width(key, 0.60f);
        g_TextS.Draw(key, p.cx - keyW * 0.5f, top + p.h - 38.0f,
                     0.60f, 0.72f, 0.82f, 0.96f, alpha * 0.84f);
        BatchFlush();

        const float iconSize = std::min(p.w * 0.44f, 150.0f * ui);
        const float iconX = p.cx - iconSize * 0.5f;
        const float iconY = top + topBar + 18.0f * ui;
        GLuint icon = IconFor(def.type);
        if (icon) {
            BatchFlush();
            DrawIcon(icon, iconX, iconY, iconSize, iconSize,
                     1.0f, 1.0f, 1.0f,
                     alpha * (0.90f + hT * 0.10f));
            BindMainShader();
        } else {
            DrawTarotEmblem(def, p.cx, iconY + iconSize * 0.5f,
                            iconSize, r, g, b,
                            alpha * (0.88f + hT * 0.12f), now);
        }
        const float dividerY = iconY + iconSize + 16.0f * ui;
        const float nameY = dividerY + 14.0f * ui;
        const wchar_t* name = AugName(def);
        float nameScale = 0.82f;
        while (nameScale > 0.54f && g_TextL.Width(name, nameScale) > p.w - 34.0f)
            nameScale -= 0.04f;
        const float nameW = g_TextL.Width(name, nameScale);
        g_TextL.Draw(name, p.cx - nameW * 0.5f, nameY,
                     nameScale, 1.0f, 1.0f, 1.0f, alpha * 0.98f);
        const wchar_t* stat = AugStat(def);
        float statScale = 0.56f;
        while (statScale > 0.40f && g_TextS.Width(stat, statScale) > p.w - 28.0f)
            statScale -= 0.04f;
        const float statW = g_TextS.Width(stat, statScale);
        g_TextS.Draw(stat, p.cx - statW * 0.5f, nameY + 36.0f * ui,
                     statScale, 0.72f, 1.0f, 0.82f, alpha * 0.92f);
        BatchFlush();

        std::vector<std::wstring> shortLines = TarotWrap(AugDesc(def), 0.48f,
                                                         p.w - 30.0f);
        if (!shortLines.empty()) {
            const std::wstring& line = shortLines.front();
            g_TextS.Draw(line.c_str(), p.cx - g_TextS.Width(line.c_str(), 0.48f) * 0.5f,
                         top + p.h - 54.0f, 0.48f,
                         0.76f, 0.84f, 0.94f, alpha * 0.72f);
        }
        BatchFlush();
    }

    if (!inExit && inputReady && nCards > 0) {
        const int detailSlot = g_HoveredAug >= 0 && g_HoveredAug < nCards
            ? g_HoveredAug : 0;
        const AugDef& def = ALL_AUGS[g_GameManager.augChoices[detailSlot]];
        DrawTarotDetailPanel(def, panelX, panelY, panelW, panelH, enterE);
        if (g_HoveredAug < 0) {
            const wchar_t* hint = T(StrId::KEY_HINT_NO_HOVER);
            g_TextS.Draw(hint, CenterTextX(sw, g_TextS, hint, 0.68f),
                         sh * 0.965f, 0.68f,
                         0.52f, 0.64f, 0.80f, enterE * 0.68f);
            BatchFlush();
        }
    } else if (!inExit && enterT > 0.68f) {
        const wchar_t* hint = T(StrId::KEY_HINT_NO_HOVER);
        g_TextS.Draw(hint, CenterTextX(sw, g_TextS, hint, 0.68f), sh * 0.965f,
                     0.68f, 0.52f, 0.64f, 0.80f, enterE * 0.68f);
        BatchFlush();
    }

    if (nCards > 0 && inExit && g_AugExitT >= 0.54f && !burstSpawned) {
        burstSpawned = true;
        const int slot = std::max(0, std::min(nCards - 1, g_AugExitSlot));
        float r, g, b;
        GetRarityColor(ALL_AUGS[g_GameManager.augChoices[slot]].rarity, r, g, b);
        for (int i = 0; i < 96; ++i) {
            const float ang = (float)i / 96.0f * 2.0f * M_PI;
            const float speed = 100.0f + (float)(rand() % 260);
            burst[i] = { sw * 0.50f, sh * 0.49f, cosf(ang) * speed,
                         sinf(ang) * speed, 0.72f, r, g, b };
        }
        TriggerFlash(r, g, b, 0.78f);
    }
    if (nCards > 0 && inExit && g_AugExitT >= 0.54f) {
        for (auto& particle : burst) {
            if (particle.life <= 0.0f) continue;
            particle.life -= delta;
            particle.x += particle.vx * delta;
            particle.y += particle.vy * delta;
            particle.vx *= 1.0f - std::min(1.0f, delta * 2.0f);
            particle.vy *= 1.0f - std::min(1.0f, delta * 2.0f);
            const float a = TarotClamp01(particle.life / 0.72f);
            drawDiamond(particle.x, particle.y, 5.0f * a + 1.0f,
                        particle.r, particle.g, particle.b, a * 0.9f);
        }
        BatchFlush();
        TarotSegment(sw * 0.50f - 80.0f, sh * 0.49f,
                     sw * 0.50f + 80.0f, sh * 0.49f, 2.0f,
                     1.0f, 1.0f, 1.0f,
                     TarotClamp01((g_AugExitT - 0.54f) / 0.18f) * 0.72f);
        BatchFlush();
    }
}

void Scene_AugSelect(const SceneCtx& c) {
    Scene_AugSelectCards(c);
    return;
    const float sw = c.sw, sh = c.sh;
    const float delta = c.delta;
    const GameState st = g_GameManager.currentState;
    const int nCards = std::max(0, std::min(3, g_GameManager.augChoiceCount));

    auto easeOut = [](float t) -> float { float i=1.f-t; return 1.f-i*i*i; };
    auto easeIn  = [](float t) -> float { return t*t*t; };
    auto drawSeg = [](float x1,float y1,float x2,float y2,float t,
                      float lr,float lg,float lb,float la) {
        float dx=x2-x1, dy=y2-y1, len=sqrtf(dx*dx+dy*dy);
        if (len < 0.5f) return;
        float nx=-dy/len*t*0.5f, ny=dx/len*t*0.5f;
        BatchTri(x1+nx,y1+ny, x1-nx,y1-ny, x2-nx,y2-ny, lr,lg,lb,la);
        BatchTri(x1+nx,y1+ny, x2-nx,y2-ny, x2+nx,y2+ny, lr,lg,lb,la);
    };

    // ── 정적 상태 ──
    static float s_spawnT    = 0.0f;
    static float s_dimT[3]   = {1,1,1};
    static float s_flashT[3] = {};
    static float s_pullT[3]  = {};
    static bool  s_lmbPrev   = false;
    static float s_panelA    = 0.0f;
    static int   s_prevHov   = -1;
    static int   s_seenSerial = -1;
    static double s_prevMx = -1.0, s_prevMy = -1.0;

    struct SNPart { float x,y,vx,vy,life; float r,g,b; bool active; };
    static constexpr int SN_MAX = 72;
    static SNPart s_sn[SN_MAX];
    static bool   s_snSpawned = false;

    // 진입 리셋
    {
        if (s_seenSerial != g_GameManager.augmentSelectionSerial) {
            s_seenSerial  = g_GameManager.augmentSelectionSerial;
            s_spawnT    = 0.0f;
            s_prevHov   = -1;
            s_panelA    = 0.0f;
            s_snSpawned = false;
            for (int i=0; i<3; i++) { s_dimT[i]=1.f; s_flashT[i]=0.f; s_pullT[i]=0.f; }
            s_lmbPrev = false;
            s_prevMx = c.mx;
            s_prevMy = c.my;
            for (auto& p: s_sn) p.active=false;
        }
    }

    const bool  isDebuff = (st == GameState::DEBUFF_SELECT);
    const float now      = (float)glfwGetTime();

    constexpr float COLLAPSE_END = 0.65f;
    constexpr float SN_START     = 0.65f;
    constexpr float SN_END       = 1.20f;

    const bool  inExit     = (g_AugExitT >= 0.0f);
    const float collapseT  = inExit ? std::min(g_AugExitT / COLLAPSE_END, 1.0f) : 0.0f;
    const float collapseE  = easeOut(collapseT);
    const float supernovaT = inExit
        ? std::max(0.0f, std::min((g_AugExitT - SN_START)/(SN_END - SN_START), 1.0f))
        : 0.0f;

    if (!inExit)
        s_spawnT = std::min(s_spawnT + delta / 0.55f, 1.0f);
    const bool inFocus = (s_spawnT >= 1.0f && !inExit);

    if (g_HoveredAug != s_prevHov && g_HoveredAug >= 0 && g_HoveredAug < nCards)
        s_flashT[g_HoveredAug] = 1.0f;
    s_prevHov = g_HoveredAug;

    for (int i=0; i<3; i++) {
        float dimTgt  = (g_HoveredAug >= 0 && g_HoveredAug != i && inFocus) ? 0.38f : 1.0f;
        float pullTgt = (g_HoveredAug == i && inFocus) ? 1.0f : 0.0f;
        s_dimT[i]  += (dimTgt  - s_dimT[i])  * std::min(1.0f, delta * 10.0f);
        s_pullT[i] += (pullTgt - s_pullT[i])  * std::min(1.0f, delta * 5.0f);
        if (s_flashT[i] > 0.f) s_flashT[i] = std::max(0.f, s_flashT[i] - delta / 0.20f);
    }
    {
        float panelTgt = (inFocus && g_HoveredAug >= 0) ? 1.0f : 0.0f;
        s_panelA += (panelTgt - s_panelA) * std::min(1.0f, delta * 12.0f);
    }

    // ── 공전 궤도 ──
    static float s_orbitT = 0.0f;
    if (!inExit) s_orbitT += delta * 0.22f;
    const float ORB_CX  = sw * 0.50f;
    const float ORB_CY  = sh * 0.46f;
    const float ORBIT_R = std::min(sw, sh) * 0.210f;
    const float BASE_R  = 46.0f;
    auto OrbPos = [&](int idx, float& ox, float& oy) {
        float a = s_orbitT + (float)idx * (6.2832f / 3.0f);
        ox = ORB_CX + cosf(a) * ORBIT_R;
        oy = ORB_CY + sinf(a) * ORBIT_R;
    };

    // Continuous pointer hover keeps the visual focus and click target in sync.
    const bool pointerMoved = (s_prevMx < 0.0 ||
        fabs(c.mx - s_prevMx) > 0.5 || fabs(c.my - s_prevMy) > 0.5);
    if (pointerMoved) g_GameManager.augmentKeyboardFocus = false;
    s_prevMx = c.mx;
    s_prevMy = c.my;
    if (!inExit && inFocus && (!g_GameManager.augmentKeyboardFocus || pointerMoved)) {
        int best = -1;
        float bestD2 = 1e9f;
        for (int i = 0; i < nCards; i++) {
            float ox, oy; OrbPos(i, ox, oy);
            float pe = easeOut(s_pullT[i]);
            float px = ox + (ORB_CX - ox) * pe;
            float py = oy + (ORB_CY - oy) * pe;
            float dx = (float)c.mx - px;
            float dy = (float)c.my - py;
            float d2 = dx * dx + dy * dy;
            if (d2 < bestD2) { bestD2 = d2; best = i; }
        }
        if (best >= 0) {
            float hitR = BASE_R * (1.0f + easeOut(s_pullT[best]) * 1.5f) * 1.4f;
            g_HoveredAug = (bestD2 < hitR * hitR) ? best : -1;
        } else {
            g_HoveredAug = -1;
        }
    }

    // ── 마우스 클릭 판정 (궤도 기반) ──
    {
        bool lmbClick = c.lmb && !s_lmbPrev;
        if (lmbClick && !inExit && inFocus) {
            g_GameManager.augmentKeyboardFocus = false;
            int   best  = -1;
            float bestD2 = 1e9f;
            for (int i = 0; i < nCards; i++) {
                float ox, oy; OrbPos(i, ox, oy);
                float pe = easeOut(s_pullT[i]);
                float rpx = ox + (ORB_CX - ox) * pe;
                float rpy = oy + (ORB_CY - oy) * pe;
                float dx = (float)c.mx - rpx;
                float dy = (float)c.my - rpy;
                float d2 = dx*dx + dy*dy;
                if (d2 < bestD2) { bestD2 = d2; best = i; }
            }
            float hitR = BASE_R * (1.0f + easeOut(s_pullT[best >= 0 ? best : 0]) * 1.5f) * 1.4f;
            if (best >= 0 && bestD2 < hitR * hitR) {
                g_HoveredAug = best;
                if (g_HoveredAug == best && g_AugExitT < 0.0f) {
                    // 항성(이미 선택) 재클릭 → 확정
                    g_AugExitT    = 0.0f;
                    g_AugExitSlot = best;
                }
            }
        }
        s_lmbPrev = c.lmb;
    }

    // ── 배경 Dim ──
    {
        float dimA = easeOut(std::min(s_spawnT * 1.8f, 1.0f));
        if (inExit) dimA *= std::max(0.f, 1.0f - collapseT * 2.0f);
        drawRect(0, 0, sw, sh, 0.0f, 0.0f, 0.0f, dimA * 0.62f);
        BatchFlush();
    }

    // ── 헤더 ──
    {
        float titA = easeOut(std::min(s_spawnT * 2.0f, 1.0f));
        if (inExit) titA *= std::max(0.f, 1.0f - collapseT * 3.0f);
        const wchar_t* TIT = isDebuff ? T(StrId::CHOOSE_DEBUFF) : T(StrId::CHOOSE_AUG);
        BatchFlush();
        g_TextL.Draw(TIT, CenterTextX(sw, g_TextL, TIT, 1.0f),
                     sh * 0.07f, 1.0f, 1.f, 1.f, 1.f, 0.92f * titA);
        if (st == GameState::AUG_SELECT) {
            wchar_t slotBuf[64];
            swprintf_s(slotBuf, L"ID %d / %d", CountIdentitySlotsUsed(), IdentitySlotMax());
            g_TextS.Draw(slotBuf, CenterTextX(sw, g_TextS, slotBuf, 0.76f),
                         sh * 0.07f + 34.0f, 0.76f, 0.72f, 0.92f, 1.f, 0.78f * titA);
        }
        BatchFlush();
    }

    // ── 별자리 외곽 프레임 ──
    {
        float frameA = easeOut(std::min(s_spawnT, 1.0f));
        if (inExit) frameA *= std::max(0.f, 1.0f - collapseT * 1.5f);
        if (frameA > 0.01f) {
            drawConstellFrame(16.f, 16.f, sw - 32.f, sh - 32.f,
                              0.30f, 0.62f, 0.92f, frameA * 0.72f,
                              24.f, 8.f, frameA * 0.40f, s_spawnT);
            BatchFlush();
        }
    }

    // ── 등급별 색상 (마스터 기획서 고정값) ──
    auto RarColor = [&](AugRarity r, float& cr, float& cg, float& cb) {
        switch (r) {
        case AugRarity::COMMON:    cr=1.0f; cg=1.0f; cb=1.0f; return;
        case AugRarity::RARE:      cr=0.2f; cg=0.9f; cb=0.3f; return;
        case AugRarity::EPIC:      cr=0.7f; cg=0.2f; cb=1.0f; return;
        case AugRarity::LEGENDARY: { float p=sinf(now*3.5f)*0.5f+0.5f;
                                     cr=1.0f; cg=0.80f+p*0.05f; cb=p*0.15f; return; }
        case AugRarity::MYTHIC:    cr=1.0f; cg=0.2f; cb=0.8f; return;
        case AugRarity::COMBO:     cr=0.0f; cg=0.9f; cb=1.0f; return;
        case AugRarity::SPECIAL:   cr=1.0f; cg=1.0f; cb=1.0f; return;
        case AugRarity::DEBUFF:    cr=1.0f; cg=0.1f; cb=0.1f; return;
        default:                   cr=1.0f; cg=1.0f; cb=1.0f; return;
        }
    };

    // ── 별자리 기하 데이터 ──
    struct CsVtx  { float ang, frac; };
    struct CsEdge { int a, b; };

    // OFFENSE: 별 윤곽 + 내부 오각형(pentagram) — 이중 별 구조
    static const CsVtx kOffV[] = {
        {-90.f,1.00f},{-18.f,0.96f},{ 54.f,1.00f},{128.f,0.93f},{200.f,0.97f},  // outer tips
        {-54.f,0.40f},{ 18.f,0.38f},{ 91.f,0.42f},{163.f,0.38f},{235.f,0.40f},  // inner concave
    };
    static const CsEdge kOffE[] = {
        {0,5},{5,1},{1,6},{6,2},{2,7},{7,3},{3,8},{8,4},{4,9},{9,0},  // 별 윤곽
        {5,7},{7,9},{9,6},{6,8},{8,5},                                  // 내부 pentagram
    };
    // DEFENSE: 헥사그램(다윗의 별) + 중심 — 요새 구조
    static const CsVtx kDefV[] = {
        {-90.f,1.00f},{-30.f,1.00f},{ 30.f,1.00f},{ 90.f,1.00f},{150.f,1.00f},{210.f,1.00f}, // outer hex (0-5)
        {-60.f,0.52f},{ 60.f,0.52f},{180.f,0.52f},  // inner triangle (6-8)
        {  0.f,0.00f},                               // center (9)
    };
    static const CsEdge kDefE[] = {
        {0,1},{1,2},{2,3},{3,4},{4,5},{5,0},          // outer hex
        {6,7},{7,8},{8,6},                            // inner triangle
        {0,8},{1,6},{2,6},{3,7},{4,7},{5,8},          // spokes hex→triangle
        {6,9},{7,9},{8,9},                            // center spokes
    };
    // UTILITY: Dipper(국자) — 비대칭 머리+굽은 손잡이
    static const CsVtx kUtlV[] = {
        {-65.f,0.92f},{-15.f,1.00f},{ 38.f,0.84f},  // bowl top (0-2)
        {-32.f,0.52f},{ 22.f,0.50f},               // bowl bottom (3-4)
        { 90.f,0.64f},{145.f,0.76f},               // handle mid (5-6)
        {190.f,0.58f},{222.f,0.82f},               // handle tip (7-8)
    };
    static const CsEdge kUtlE[] = {
        {0,1},{1,2},{2,4},{4,3},{3,0},  // bowl outline
        {0,4},{1,3},                     // bowl diagonals
        {4,5},{5,6},{6,7},{7,8},         // handle chain
    };

    enum class CS : uint8_t { OFFENSE, DEFENSE, UTILITY };
    auto ShapeOf = [](AugRarity r) -> CS {
        switch(r) {
        case AugRarity::RARE:
        case AugRarity::LEGENDARY:
        case AugRarity::DEBUFF:  return CS::OFFENSE;
        case AugRarity::EPIC:
        case AugRarity::MYTHIC:  return CS::DEFENSE;
        default:                 return CS::UTILITY;
        }
    };

    auto DrawConstell = [&](float cx, float cy, float radius,
                             float cr, float cg, float cb,
                             float alpha, CS shape, AugRarity rar, float selfRot) {
        DrawNebulaGlow(cx, cy, radius*1.65f, cr,cg,cb,
                       alpha*0.68f, now, rar == AugRarity::SPECIAL);

        const CsVtx*  vt  = kUtlV; const CsEdge* ed = kUtlE;
        int nvt = (int)(sizeof(kUtlV)/sizeof(kUtlV[0]));
        int ned = (int)(sizeof(kUtlE)/sizeof(kUtlE[0]));
        if (shape == CS::OFFENSE) {
            vt=kOffV; ed=kOffE;
            nvt=(int)(sizeof(kOffV)/sizeof(kOffV[0]));
            ned=(int)(sizeof(kOffE)/sizeof(kOffE[0]));
        } else if (shape == CS::DEFENSE) {
            vt=kDefV; ed=kDefE;
            nvt=(int)(sizeof(kDefV)/sizeof(kDefV[0]));
            ned=(int)(sizeof(kDefE)/sizeof(kDefE[0]));
        }

        float vx[12], vy[12];
        for (int j=0; j<nvt && j<12; j++) {
            float rad = vt[j].ang * (float)M_PI / 180.0f + selfRot;
            vx[j] = cx + cosf(rad) * radius * vt[j].frac;
            vy[j] = cy + sinf(rad) * radius * vt[j].frac;
        }
        for (int j=0; j<ned; j++)
            drawSeg(vx[ed[j].a],vy[ed[j].a], vx[ed[j].b],vy[ed[j].b], 2.2f, cr,cg,cb, alpha*0.82f);

        for (int j=0; j<nvt; j++)
            if (vt[j].frac >= 0.45f)
                drawDiamond(vx[j], vy[j], 8.0f, cr,cg,cb, alpha*0.90f);

        float coreSz = 20.0f;
        if      (rar == AugRarity::LEGENDARY) coreSz = 26.0f + sinf(now*3.5f)*3.0f;
        else if (rar == AugRarity::MYTHIC)    coreSz = 24.0f;
        else if (rar == AugRarity::COMBO)     coreSz = 16.0f;
        drawDiamond(cx, cy, coreSz, cr,cg,cb, alpha);

        BatchFlush();
    };

    // ── 궤도 연결선 (삼각형) ──
    {
        float lineA = easeOut(std::min(s_spawnT * 1.5f, 1.0f));
        if (inExit) lineA *= std::max(0.f, 1.0f - collapseT * 2.0f);
        if (lineA > 0.01f) {
            float ox[3], oy[3];
            for (int _i=0; _i<nCards; _i++) OrbPos(_i, ox[_i], oy[_i]);
            if (nCards >= 2)
                drawSeg(ox[0],oy[0], ox[1],oy[1], 1.4f, 0.30f,0.62f,0.92f, lineA*0.20f);
            if (nCards >= 3) {
                drawSeg(ox[1],oy[1], ox[2],oy[2], 1.4f, 0.30f,0.62f,0.92f, lineA*0.20f);
                drawSeg(ox[2],oy[2], ox[0],oy[0], 1.4f, 0.30f,0.62f,0.92f, lineA*0.20f);
            }
            BatchFlush();
        }
    }

    // ── 별자리 3개 렌더 ──
    static const wchar_t* KEY_LABELS[3] = { L"[ 1 ]", L"[ 2 ]", L"[ 3 ]" };

    for (int i=0; i<nCards; i++) {
        const AugDef& def = ALL_AUGS[g_GameManager.augChoices[i]];
        float cr, cg, cb;
        RarColor(def.rarity, cr, cg, cb);
        if (inFocus && g_HoveredAug >= 0 && g_HoveredAug != i) {
            float bt = ((1.0f - s_dimT[i]) / (1.0f - 0.38f)) * 0.75f;
            cr = cr * (1.0f - bt) + 0.20f * bt;
            cg = cg * (1.0f - bt) + 0.55f * bt;
            cb = cb * (1.0f - bt) + 0.85f * bt;
        }

        const CS   shape   = ShapeOf(def.rarity);
        const bool hov     = (g_HoveredAug == i && inFocus);
        const bool isConf  = (inExit && g_AugExitSlot == i);
        const bool isOther = (inExit && g_AugExitSlot != i);
        const float flash  = s_flashT[i];

        float spawnStag = std::max(0.0f, std::min((s_spawnT-(float)i*0.10f)/0.75f, 1.0f));
        float spawnE    = easeOut(spawnStag);

        float opx, opy;
        OrbPos(i, opx, opy);
        float pullE = easeOut(s_pullT[i]);
        // 항성 풀: 궤도 위치 → 중앙으로
        float tpx = opx + (ORB_CX - opx) * pullE;
        float tpy = opy + (ORB_CY - opy) * pullE;
        // SPAWN: 중심에서 팽창
        float px = ORB_CX + (tpx - ORB_CX) * spawnE;
        float py = ORB_CY + (tpy - ORB_CY) * spawnE;
        float alpha = spawnE * s_dimT[i];

        float radius = BASE_R;
        if (def.rarity == AugRarity::LEGENDARY)
            radius *= 1.0f + (sinf(now*3.5f)*0.5f+0.5f)*0.08f;
        radius *= (1.0f + flash*0.04f);
        radius *= (1.0f + pullE * 1.5f);   // 항성화: 최대 2.5x

        // selfRot 먼저 계산 (isOther 내부에서 가속 추가 가능)
        float selfRot = now * 0.45f + (float)i * (6.2832f / 3.0f);

        if (isConf) {
            px     = opx + (sw*0.5f - opx) * collapseE;
            py     = opy + (sh*0.5f - opy) * collapseE;
            radius = BASE_R * (1.0f + collapseE * 0.55f);
            alpha  = spawnE;
            if (supernovaT > 0.0f) {
                float snE = easeOut(supernovaT);
                radius = BASE_R * (1.0f + snE * 4.2f);
                alpha  = spawnE * (1.0f - snE*snE);
                cr=1.f; cg=1.f; cb=1.f;
            }
        } else if (isOther) {
            int ci = g_AugExitSlot;
            // 확정 별자리 초기 궤도 위치 (고정) → 공전 기준각 계산
            float confOx = ORB_CX, confOy = ORB_CY;
            if (ci>=0&&ci<3) OrbPos(ci, confOx, confOy);
            // 확정 별자리 현재 위치 (중앙으로 이동 중) → 공전 중심 추적
            float cpx = confOx + (sw*0.5f - confOx) * collapseE;
            float cpy = confOy + (sh*0.5f - confOy) * collapseE;
            float absorb = easeIn(std::min(g_AugExitT / COLLAPSE_END, 1.0f));
            // 초기 각도: other 위치 → 확정 위치 방향
            float initAng  = atan2f(opy - confOy, opx - confOx);
            float initDist = sqrtf((opx-confOx)*(opx-confOx)+(opy-confOy)*(opy-confOy));
            // 빠른 공전 + 나선형 수렴
            float orbitAng = initAng + g_AugExitT * 12.0f;
            float spiralR  = initDist * (1.0f - absorb);
            px     = cpx + cosf(orbitAng) * spiralR;
            py     = cpy + sinf(orbitAng) * spiralR;
            radius = BASE_R * (1.0f - absorb * 0.95f);
            alpha  = spawnE * (1.0f - absorb * absorb);
        }

        if (alpha < 0.005f || radius < 2.0f) continue;
        if (def.rarity == AugRarity::COMBO) {
            float off = radius * 0.36f;
            DrawConstell(px-off, py, radius*0.70f, cr,cg,cb, alpha, CS::DEFENSE, def.rarity, selfRot);
            DrawConstell(px+off, py, radius*0.70f, cr,cg,cb, alpha, CS::OFFENSE, def.rarity, selfRot);
            drawSeg(px-off, py, px+off, py, 2.8f, cr,cg,cb, alpha*0.62f);
            BatchFlush();
        } else {
            DrawConstell(px, py, radius, cr, cg, cb, alpha, shape, def.rarity, selfRot);
        }

        if (!inExit && spawnE > 0.5f) {
            float kA  = spawnE * 0.82f;
            float klW = g_TextS.Width(KEY_LABELS[i], 0.76f);
            BatchFlush();
            g_TextS.Draw(KEY_LABELS[i], px - klW*0.5f, py + radius + 30.0f,
                         0.76f, 0.58f,0.80f,1.00f, kA);
            BatchFlush();
        }
    }

    // ── 우측 데이터 태그 패널 ──
    {
        if (s_panelA > 0.01f && !inExit && nCards > 0) {
            int idx = (g_HoveredAug>=0 && g_HoveredAug<nCards) ? g_HoveredAug : 0;
            const AugDef& hovDef = ALL_AUGS[g_GameManager.augChoices[idx]];
            float hr, hg, hb;
            RarColor(hovDef.rarity, hr, hg, hb);
            float pA = s_panelA;

            const float PX = sw * 0.680f;
            const float PY = sh * 0.220f;
            const float PW = sw - PX - 18.0f;

            drawSeg(PX - 8.0f, PY, PX - 8.0f, PY + sh * 0.42f, 2.0f, hr,hg,hb, pA * 0.65f);
            BatchFlush();

            float ty = PY;

            // [ CLASS : badge ]
            const wchar_t* badge = GetAugBadge(hovDef);
            wchar_t classLine[64];
            swprintf_s(classLine, L"[ CLASS : %ls ]", badge);
            g_TextS.Draw(classLine, PX, ty, 0.52f, hr,hg,hb, 0.90f*pA);
            ty += 26.0f;
            BatchFlush();

            // > 증강명
            const wchar_t* name = AugName(hovDef);
            float nSc = 0.84f;
            while (nSc > 0.52f && g_TextL.Width(name, nSc) > PW) nSc -= 0.04f;
            wchar_t nameLine[256];
            swprintf_s(nameLine, L"> %ls", name);
            g_TextL.Draw(nameLine, PX, ty, nSc, 1.f,1.f,1.f, 0.96f*pA);
            ty += nSc * 32.0f + 10.0f;
            BatchFlush();

            drawSeg(PX, ty, PX + PW * 0.65f, ty, 1.5f, 0.30f,0.62f,0.92f, pA * 0.30f);
            ty += 14.0f;
            BatchFlush();

            // STAT: 핵심 수치
            {
                const wchar_t* statVal = AugStat(hovDef);
                wchar_t statLine[128];
                swprintf_s(statLine, L"STAT: %ls", statVal);
                float ssc = 0.60f;
                while (ssc > 0.44f && g_TextS.Width(statLine, ssc) > PW) ssc -= 0.04f;
                g_TextS.Draw(statLine, PX, ty, ssc, 0.70f,0.95f,0.72f, 0.92f*pA);
                ty += ssc * 22.0f + 8.0f;
                BatchFlush();
            }

            // DESC: 간단 설명 (locDesc 직접 — AugDescKR 장문 제외)
            {
                const wchar_t* desc = hovDef.locDesc[CurLangIdx()];
                float dsc = 0.60f;
                std::wstring text = desc ? desc : L"";
                if (text.rfind(L"DESC:", 0) == 0) text.erase(0, 5);
                std::vector<std::wstring> descLines;
                std::wstring line;
                auto flushLine = [&]() {
                    if (!line.empty()) descLines.push_back(line);
                    line.clear();
                };
                for (wchar_t ch : text) {
                    if (ch == L'/') { flushLine(); continue; }
                    std::wstring test = line;
                    test += ch;
                    while (dsc > 0.44f && g_TextS.Width(test.c_str(), dsc) > PW)
                        dsc -= 0.04f;
                    if (!line.empty() && g_TextS.Width(test.c_str(), dsc) > PW) {
                        flushLine();
                    }
                    line += ch;
                }
                flushLine();
                const float descLineH = dsc * 22.0f;
                for (const std::wstring& descLine : descLines) {
                    g_TextS.Draw(descLine.c_str(), PX, ty, dsc,
                                 0.78f,0.88f,0.96f, 0.82f*pA);
                    ty += descLineH;
                }
                ty += 4.0f;
                BatchFlush();
            }

            ty += 8.0f;
            drawSeg(PX, ty, PX + PW * 0.65f, ty, 1.5f, 0.30f,0.62f,0.92f, pA * 0.18f);
            ty += 12.0f;
            BatchFlush();

            const wchar_t* HINT = T(StrId::KEY_HINT_HOVER);
            g_TextS.Draw(HINT, PX, ty, 0.58f, 0.48f,0.68f,0.80f, 0.72f*pA);
            BatchFlush();

        } else if (!inExit && s_spawnT > 0.4f) {
            const wchar_t* HINT = T(StrId::KEY_HINT_NO_HOVER);
            BatchFlush();
            g_TextS.Draw(HINT, CenterTextX(sw, g_TextS, HINT, 0.72f),
                         sh*0.88f, 0.72f, 0.52f,0.60f,0.72f, 0.62f*easeOut(s_spawnT));
            BatchFlush();
        }
    }

    // ── SUPERNOVA 파티클 ──
    if (g_AugExitT >= SN_START) {
        if (!s_snSpawned) {
            s_snSpawned = true;
            int confSlot = (g_AugExitSlot >= 0 && g_AugExitSlot < nCards)
                         ? g_AugExitSlot : 0;
            int confIdx = (nCards > 0) ? g_GameManager.augChoices[confSlot] : 0;
            float pr, pg2, pb;
            RarColor(ALL_AUGS[confIdx].rarity, pr, pg2, pb);
            float spawnX = ORB_CX, spawnY = ORB_CY;
            if (g_AugExitSlot>=0&&g_AugExitSlot<nCards) OrbPos(g_AugExitSlot, spawnX, spawnY);
            for (int k=0; k<SN_MAX; k++) {
                float ang = (float)k/SN_MAX*6.2832f + now*0.1f;
                float spd = 140.0f + (float)(rand()%340);
                s_sn[k] = { spawnX, spawnY,
                             cosf(ang)*spd, sinf(ang)*spd,
                             0.60f, pr, pg2, pb, true };
            }
            TriggerFlash(pr, pg2, pb, 0.90f);
        }
        BindMainShader();
        for (auto& p : s_sn) {
            if (!p.active) continue;
            p.life -= delta;
            if (p.life <= 0.f) { p.active=false; continue; }
            p.x  += p.vx * delta;
            p.y  += p.vy * delta;
            p.vx *= (1.0f - 2.2f*delta);
            p.vy *= (1.0f - 2.2f*delta);
            float fr = p.life / 0.60f;
            drawDiamond(p.x, p.y, 4.5f*fr+0.5f, p.r,p.g,p.b, fr*0.90f);
        }
        BatchFlush();
    }
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
    return;
    const float sw = c.sw, sh = c.sh;
    const float delta = c.delta;
    int newIdx = g_GameManager.pendingAugIdx;
    if (newIdx < 0 || newIdx >= AUG_TOTAL) return;

    // ── 애니메이션 상태 ──
    static float s_enterT      = 0.0f;
    static float s_hovY[32]    = {};
    static float s_pickFlash[32] = {};
    static int   s_prevHov     = -1;
    static int   s_seenSerial  = -1;
    static double s_prevMx = -1.0, s_prevMy = -1.0;

    auto easeOut = [](float t) -> float {
        float inv = 1.0f - t; return 1.0f - inv * inv * inv;
    };

    // 새 진입 감지
    if (s_seenSerial != g_GameManager.augmentSelectionSerial) {
        s_seenSerial = g_GameManager.augmentSelectionSerial;
        s_enterT = 0.0f;
        std::fill(std::begin(s_hovY),      std::end(s_hovY),      0.f);
        std::fill(std::begin(s_pickFlash), std::end(s_pickFlash), 0.f);
        s_prevHov = -1;
        s_prevMx = c.mx;
        s_prevMy = c.my;
        g_HoveredAug = -1;
    }
    s_enterT = std::min(s_enterT + delta / 0.32f, 1.0f);

    // pick flash 트리거
    if (g_HoveredAug != s_prevHov && g_HoveredAug >= 0 && g_HoveredAug < 32)
        s_pickFlash[g_HoveredAug] = 1.0f;
    s_prevHov = g_HoveredAug;

    // hover spring + flash decay
    int n = g_GameManager.replaceChoiceCount;
    for (int i = 0; i < n && i < 32; i++) {
        float targetY = (g_HoveredAug == i) ? -10.0f : 0.0f;
        s_hovY[i] += (targetY - s_hovY[i]) * std::min(1.0f, delta * 16.0f);
        if (s_pickFlash[i] > 0.f) s_pickFlash[i] = std::max(0.f, s_pickFlash[i] - delta / 0.20f);
    }

    // exit anim
    const float EXIT_DUR = 0.28f;
    float exitPct = (g_RepExitT >= 0.f) ? std::min(g_RepExitT / EXIT_DUR, 1.0f) : -1.0f;
    float exitE   = (exitPct >= 0.f) ? easeOut(exitPct) : 0.f;
    bool  isCancelExit = (g_RepExitSlot == -2);

    float enterE = easeOut(s_enterT);

    int li = LangIndex();
    if (li < 0 || li > 2) li = 0;

    // ── 오버레이 ──
    float overlayA = enterE * (exitPct >= 0.f ? (1.0f - exitE) : 1.0f);
    BindMainShader();
    drawRect(0, 0, sw, sh, 0.03f, 0.02f, 0.08f, overlayA * 0.68f * SCENE_BG_ALPHA);
    BatchFlush();

    // ── 타이틀 ──
    float titA = enterE;
    if (exitPct >= 0.f) titA *= std::max(0.f, 1.0f - exitPct * 2.5f);

    const wchar_t* TIT[3] = {
        L"식별 슬롯 가득 — 교체할 증강 선택",
        L"Identity slots full — pick one to replace",
        L"識別スロット満杯 — 交換する強化を選択" };
    g_TextL.Draw(TIT[li], CenterTextX(sw, g_TextL, TIT[li], 0.95f), sh * 0.12f,
                 0.95f, 1, 1, 1, 0.95f * titA);

    // NEW 증강 표시
    wchar_t newLine[160];
    swprintf_s(newLine, L"NEW: [%ls] %ls",
               GetAugBadge(ALL_AUGS[newIdx]), AugName(ALL_AUGS[newIdx]));
    g_TextS.Draw(newLine, CenterTextX(sw, g_TextS, newLine, 0.88f), sh * 0.18f,
                 0.88f, 0.9f, 1.0f, 0.7f, 0.95f * titA);
    BatchFlush();

    // ── 카드 목록 ──
    const float CARD_W = 260.0f, CARD_H = 120.0f, GAP = 16.0f;
    float totalW = (float)n * CARD_W + (float)(n > 0 ? n - 1 : 0) * GAP;
    if (totalW > sw - 40.0f) totalW = sw - 40.0f;
    float baseX = (sw - totalW) * 0.5f;
    float baseY = sh * 0.32f;

    static const wchar_t* KEYS[9] = { L"[1]",L"[2]",L"[3]",L"[4]",L"[5]",L"[6]",L"[7]",L"[8]",L"[9]" };

    const bool pointerMoved = (s_prevMx < 0.0 ||
        fabs(c.mx - s_prevMx) > 0.5 || fabs(c.my - s_prevMy) > 0.5);
    if (pointerMoved) g_GameManager.augmentKeyboardFocus = false;
    s_prevMx = c.mx;
    s_prevMy = c.my;
    if (g_RepExitT < 0.0f && (!g_GameManager.augmentKeyboardFocus || pointerMoved)) {
        int hover = -1;
        for (int i = 0; i < n && i < 32; i++) {
            float stag = (float)i * 0.04f;
            float ct = std::max(0.f, std::min((s_enterT - stag) /
                                               (1.0f - stag + 0.001f), 1.0f));
            float cy = baseY + (1.0f - easeOut(ct)) * 60.0f + s_hovY[i];
            float cx = baseX + i * (CARD_W + GAP);
            if (c.mx >= cx && c.mx <= cx + CARD_W &&
                c.my >= cy && c.my <= cy + CARD_H) {
                hover = i;
                break;
            }
        }
        g_HoveredAug = hover;
    }
    if (c.lmb && !g_LmbPrev && g_RepExitT < 0.0f &&
        g_HoveredAug >= 0 && g_HoveredAug < n && g_HoveredAug < 32) {
        g_GameManager.augmentKeyboardFocus = false;
        g_RepExitT = 0.0f;
        g_RepExitSlot = g_HoveredAug;
    }

    for (int i = 0; i < n && i < 32; i++) {
        int idx = g_GameManager.replaceChoices[i];
        if (idx < 0 || idx >= AUG_TOTAL) continue;

        // 슬라이드인 스태거
        float stag = (float)i * 0.04f;
        float ct   = std::max(0.f, std::min((s_enterT - stag) / (1.0f - stag + 0.001f), 1.0f));
        float cardA = easeOut(ct);
        float slideY = (1.0f - easeOut(ct)) * 60.0f;

        bool  isConf = (exitPct >= 0.f && g_RepExitSlot == i);
        float flash  = s_pickFlash[i];

        // exit 오프셋
        float exitOffY = 0.f;
        if (exitPct >= 0.f) {
            if (isCancelExit) {
                exitOffY = exitE * 50.0f;
                cardA   *= std::max(0.f, 1.0f - exitE * 1.6f);
            } else if (isConf) {
                exitOffY = -exitE * 50.0f;
                cardA   *= std::max(0.f, 1.0f - exitE * 1.2f);
            } else {
                exitOffY = exitE * 40.0f;
                cardA   *= std::max(0.f, 1.0f - exitE * 1.8f);
            }
        }

        float cx = baseX + i * (CARD_W + GAP);
        float cy = baseY + slideY + s_hovY[i] + exitOffY;

        const AugDef& def = ALL_AUGS[idx];
        float tr, tg, tb;
        GetRarityColor(def.rarity, tr, tg, tb);
        float flashBoost = flash * 0.5f + (isConf ? exitE * (1.0f - exitE) * 3.0f * (1.0f - exitPct) : 0.0f);
        bool  hov = (g_HoveredAug == i);

        BindMainShader();
        float bgMul = (hov ? 0.42f : 0.28f) + flash * 0.12f;
        drawRect(cx, cy, CARD_W, CARD_H, tr * bgMul, tg * bgMul, tb * bgMul, 0.92f * cardA);

        BatchFlush(); glEnable(GL_BLEND);
        float bord = (hov ? 1.0f : 0.55f) + flash * 0.5f + flashBoost;
        float nr = std::min(1.0f, tr * (1.4f + flashBoost) + 0.08f);
        float ng = std::min(1.0f, tg * (1.4f + flashBoost) + 0.08f);
        float nb = std::min(1.0f, tb * (1.4f + flashBoost) + 0.08f);
        drawNeonBorder(cx, cy, CARD_W, CARD_H, nr * bord, ng * bord, nb * bord);

        wchar_t lbl[128];
        swprintf_s(lbl, L"[%d] %ls", i + 1, AugName(def));
        float lsc = 0.78f;
        while (lsc > 0.52f && g_TextL.Width(lbl, lsc) > CARD_W - 16.0f) lsc -= 0.04f;
        BatchFlush();
        g_TextL.Draw(lbl, cx + 10.0f, cy + 16.0f, lsc, 1, 1, 1, 0.95f * cardA);
        if (i < 9) {
            BatchFlush();
            g_TextS.Draw(KEYS[i], cx + 10.0f, cy + CARD_H - 32.0f, 0.85f,
                         nr, ng, nb, 0.90f * cardA);
        }
        BatchFlush();
    }

    // ── 키 힌트 ──
    const wchar_t* HINT[3] = {
        L"1~9: 선택 · Space: 교체 · Esc: 새 증강 포기",
        L"1~9: select · Space: replace · Esc: skip new augment",
        L"1~9: 選択 · Space: 交換 · Esc: 新規強化を見送り" };
    BindMainShader();
    g_TextS.Draw(HINT[li], CenterTextX(sw, g_TextS, HINT[li], 0.82f), sh * 0.78f,
                 0.82f, 0.75f, 0.75f, 0.75f, 0.85f * overlayA);
    BatchFlush();
}

static void Scene_RunShopCards(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh, delta = c.delta;
    constexpr int nCards = 4;
    static int seenShop = -1;
    static float enterT = 0.0f;
    static float hoverT[nCards] = {};
    static float flashT[nCards] = {};
    static double prevMx = -1.0, prevMy = -1.0;
    static bool lmbPrev = false;

    int stockSerial = 17;
    for (int i = 0; i < nCards; ++i)
        stockSerial = stockSerial * 131 + g_RunShopStock[i] + 2;
    if (seenShop != stockSerial) {
        seenShop = stockSerial;
        enterT = 0.0f;
        prevMx = c.mx;
        prevMy = c.my;
        lmbPrev = false;
        for (int i = 0; i < nCards; ++i) { hoverT[i] = 0.0f; flashT[i] = 0.0f; }
    }
    enterT = std::min(1.0f, enterT + delta / 0.42f);
    const float enterE = TarotEaseOut(enterT);
    const bool pointerMoved = prevMx < 0.0 ||
        fabs(c.mx - prevMx) > 0.5 || fabs(c.my - prevMy) > 0.5;
    prevMx = c.mx;
    prevMy = c.my;

    const float ui = std::min(1.0f, std::min(sw / 1600.0f, sh / 900.0f));
    const float cardW = 242.0f * ui;
    const float cardH = 368.0f * ui;
    const float centerX = sw * 0.50f;
    const float centerY = sh * 0.49f;
    TarotCardPose base[nCards];
    for (int i = 0; i < nCards; ++i)
        base[i] = TarotBasePose(i, nCards, sw, sh, cardW, cardH, centerX, centerY);

    if (pointerMoved || g_HoveredAug < 0 || g_HoveredAug >= nCards) {
        int hover = -1;
        for (int i = nCards - 1; i >= 0; --i)
            if (TarotHit(base[i], c.mx, c.my, 12.0f)) { hover = i; break; }
        if (pointerMoved || hover >= 0) g_HoveredAug = hover;
    }
    if (c.lmb && !lmbPrev) {
        for (int i = nCards - 1; i >= 0; --i) {
            if (TarotHit(base[i], c.mx, c.my, 12.0f)) {
                g_HoveredAug = i;
                break;
            }
        }
    }
    lmbPrev = c.lmb;

    for (int i = 0; i < nCards; ++i) {
        const float target = g_HoveredAug == i ? 1.0f : 0.0f;
        hoverT[i] += (target - hoverT[i]) * std::min(1.0f, delta * 11.0f);
        flashT[i] = std::max(0.0f, flashT[i] - delta / 0.22f);
    }

    BindMainShader();
    drawRect(0, 0, sw, sh, 0.006f, 0.008f, 0.018f, enterE * 0.55f);
    drawConstellFrame(16.0f, 16.0f, sw - 32.0f, sh - 32.0f,
                      0.92f, 0.64f, 0.24f, enterE * 0.52f,
                      24.0f, 8.0f, enterE * 0.20f, enterT);
    BatchFlush();

    const wchar_t* title = L"RUN SHOP // AUGMENT RELICS";
    g_TextL.Draw(title, CenterTextX(sw, g_TextL, title, 0.94f), sh * 0.065f,
                 0.94f, 1.0f, 0.98f, 0.90f, enterE * 0.96f);
    wchar_t gold[48];
    swprintf_s(gold, L"G  %lld", g_RunGold);
    const float goldW = g_TextL.Width(gold, 0.84f);
    g_TextL.Draw(gold, sw - goldW - 28.0f, sh * 0.065f, 0.84f,
                 1.0f, 0.84f, 0.30f, enterE * 0.96f);
    BatchFlush();

    for (int i = 0; i < nCards; ++i) {
        const int idx = g_RunShopStock[i];
        TarotCardPose p = base[i];
        const float hT = TarotEaseOut(hoverT[i]);
        p.cy -= hT * 18.0f;
        p.w *= 1.0f + hT * 0.045f;
        p.h *= 1.0f + hT * 0.045f;
        const bool sold = idx < 0 || idx >= AUG_TOTAL;
        float r = sold ? 0.35f : 0.0f;
        float g = sold ? 0.35f : 0.0f;
        float b = sold ? 0.40f : 0.0f;
        if (!sold) GetRarityColor(ALL_AUGS[idx].rarity, r, g, b);
        const float alpha = enterE * (g_HoveredAug >= 0 && g_HoveredAug != i ? 0.42f : 1.0f);
        DrawTarotCardSurface(p, r, g, b, alpha, hT, flashT[i], false,
                             (float)glfwGetTime());
        const float top = p.cy - p.h * 0.5f;
        wchar_t key[16];
        swprintf_s(key, L"[%d]", i + 1);
        g_TextS.Draw(key, p.cx - g_TextS.Width(key, 0.56f) * 0.5f,
                     top + 13.0f, 0.56f, 0.75f, 0.84f, 0.96f, alpha * 0.86f);
        if (sold) {
            const wchar_t* soldText = L"SOLD";
            g_TextL.Draw(soldText, p.cx - g_TextL.Width(soldText, 0.94f) * 0.5f,
                         p.cy - 16.0f, 0.94f, 0.55f, 0.55f, 0.62f, alpha * 0.78f);
            BatchFlush();
            continue;
        }

        const AugDef& def = ALL_AUGS[idx];
        const wchar_t* badge = GetAugBadge(def);
        g_TextS.Draw(badge, p.cx - g_TextS.Width(badge, 0.56f) * 0.5f,
                     top + 40.0f, 0.56f,
                     std::min(1.0f, r * 1.45f + 0.20f),
                     std::min(1.0f, g * 1.45f + 0.20f),
                     std::min(1.0f, b * 1.45f + 0.20f), alpha * 0.92f);
        const wchar_t* name = AugName(def);
        float nameScale = 0.72f;
        while (nameScale > 0.46f && g_TextL.Width(name, nameScale) > p.w - 26.0f)
            nameScale -= 0.04f;
        g_TextL.Draw(name, p.cx - g_TextL.Width(name, nameScale) * 0.5f,
                     top + 69.0f, nameScale, 1.0f, 1.0f, 1.0f, alpha * 0.96f);
        BatchFlush();
        DrawTarotEmblem(def, p.cx, p.cy - 22.0f, p.w * 0.40f,
                        r, g, b, alpha * 0.88f, (float)glfwGetTime());

        wchar_t price[32];
        swprintf_s(price, L"G  %d", g_RunShopPrice[i]);
        const bool afford = g_RunGold >= g_RunShopPrice[i];
        g_TextS.Draw(price, p.cx - g_TextS.Width(price, 0.72f) * 0.5f,
                     top + p.h - 62.0f, 0.72f,
                     afford ? 1.0f : 0.55f, afford ? 0.84f : 0.42f,
                     0.30f, alpha * 0.98f);
        const wchar_t* buy = L"SPACE  BUY";
        g_TextS.Draw(buy, p.cx - g_TextS.Width(buy, 0.50f) * 0.5f,
                     top + p.h - 35.0f, 0.50f,
                     0.70f, 0.80f, 0.94f, alpha * 0.78f);
        BatchFlush();
    }

    if (g_HoveredAug >= 0 && g_HoveredAug < nCards &&
        g_RunShopStock[g_HoveredAug] >= 0 &&
        g_RunShopStock[g_HoveredAug] < AUG_TOTAL) {
        const int idx = g_RunShopStock[g_HoveredAug];
        const float detailW = std::min(760.0f, sw * 0.70f);
        const float detailX = (sw - detailW) * 0.5f;
        DrawTarotDetailPanel(ALL_AUGS[idx], detailX, sh * 0.805f,
                             detailW, std::min(116.0f, sh * 0.14f), enterE);
    }
    const wchar_t* hint = L"1-4 SELECT   SPACE BUY   ESC LEAVE";
    g_TextS.Draw(hint, CenterTextX(sw, g_TextS, hint, 0.62f), sh * 0.965f,
                 0.62f, 0.60f, 0.68f, 0.82f, enterE * 0.76f);
    BatchFlush();
}

void Scene_RunShop(const SceneCtx& c) {
    Scene_RunShopCards(c);
    return;
    const float sw = c.sw, sh = c.sh;
    const GameState st = g_GameManager.currentState;

    const int  nCards  = 4;
    const float CARD_W = 240.0f;
    const float CARD_H = 360.0f;
    const float GAP    = 32.0f;
    const float TOTAL_W = (float)nCards * CARD_W + (float)(nCards - 1) * GAP;
    float baseX = (sw - TOTAL_W) * 0.5f;
    float baseY = (sh - CARD_H) * 0.38f;

    const wchar_t* TIT[3] = {
        L"런 상점 — 골드로 증강 구매",
        L"Run Shop — buy augments with gold",
        L"ランショップ — ゴールドで強化購入" };
    int li = LangIndex();
    if (li < 0 || li > 2) li = 0;
    g_TextL.Draw(TIT[li], CenterTextX(sw, g_TextL, TIT[li], 0.95f), baseY - 72.0f,
                 0.95f, 1, 1, 1, 0.95f);

    wchar_t goldBuf[48];
    swprintf_s(goldBuf, L"G  %lld", g_RunGold);
    float gw = g_TextL.Width(goldBuf, 0.9f);
    g_TextL.Draw(goldBuf, sw - gw - 24.0f, 18.0f, 0.9f, 1.0f, 0.88f, 0.35f, 0.95f);

    const wchar_t* HINT[3] = {
        L"1~4 선택 · Space 구매 · ESC 나가기 (구역 밖으로 나갔다가 재입장)",
        L"1~4 select · Space buy · ESC leave (re-enter zone after exit)",
        L"1~4 選択 · Space 購入 · ESC 退出 (再入場は一度離れる)" };
    g_TextS.Draw(HINT[li], CenterTextX(sw, g_TextS, HINT[li], 0.82f),
                 baseY + CARD_H + 168.0f, 0.82f, 0.75f, 0.75f, 0.75f, 0.85f);

    static const wchar_t* KEY_LABELS[4] = { L"[ 1 ]", L"[ 2 ]", L"[ 3 ]", L"[ 4 ]" };
    for (int i = 0; i < nCards; i++) {
        float cardX = baseX + i * (CARD_W + GAP);
        float yOff  = (g_HoveredAug == i) ? -14.0f : 0.0f;
        int idx = g_RunShopStock[i];
        bool sold = (idx < 0);

        drawRect(cardX, baseY + yOff, CARD_W, CARD_H,
                 0.04f, 0.05f, 0.07f, sold ? 0.55f : 0.88f);

        if (sold) {
            const wchar_t* SOLD[3] = { L"매진", L"SOLD", L"売切" };
            float tw = g_TextL.Width(SOLD[li], 1.1f);
            g_TextL.Draw(SOLD[li], cardX + (CARD_W - tw) * 0.5f,
                         baseY + yOff + CARD_H * 0.45f, 1.1f,
                         0.5f, 0.5f, 0.55f, 0.7f);
            continue;
        }

        const AugDef& def = ALL_AUGS[idx];
        const wchar_t* cardName = AugName(def);
        float tr, tg, tb;
        GetRarityColor(def.rarity, tr, tg, tb);
        tr = std::min(1.0f, tr * 1.4f + 0.25f);
        tg = std::min(1.0f, tg * 1.4f + 0.25f);
        tb = std::min(1.0f, tb * 1.4f + 0.25f);
        const wchar_t* topLabel = GetAugBadge(def);
        GLuint icon = IconFor(def.type);
        if (icon) {
            float isz = 120.0f;
            DrawIcon(icon, cardX + (CARD_W - isz) * 0.5f,
                     baseY + yOff + CARD_H * 0.12f, isz, isz,
                     1.0f, 1.0f, 1.0f, 0.97f);
        }
        float rw = g_TextS.Width(topLabel, 1.0f);
        g_TextS.Draw(topLabel, cardX + (CARD_W - rw) * 0.5f,
                     baseY + yOff + 18.0f, 1.0f, tr, tg, tb, 0.95f);

        float nameSc = 1.05f;
        while (nameSc > 0.65f &&
               g_TextL.Width(cardName, nameSc) > CARD_W - 16.0f)
            nameSc -= 0.05f;
        float nw = g_TextL.Width(cardName, nameSc);
        g_TextL.Draw(cardName, cardX + (CARD_W - nw) * 0.5f,
                     baseY + yOff + CARD_H * 0.52f, nameSc, 1, 1, 1, 0.95f);

        wchar_t priceBuf[32];
        swprintf_s(priceBuf, L"G %d", g_RunShopPrice[i]);
        bool afford = (g_RunGold >= g_RunShopPrice[i]);
        float pw = g_TextS.Width(priceBuf, 1.0f);
        g_TextS.Draw(priceBuf, cardX + (CARD_W - pw) * 0.5f,
                     baseY + yOff + CARD_H - 72.0f, 1.0f,
                     afford ? 1.0f : 0.55f,
                     afford ? 0.85f : 0.4f,
                     afford ? 0.35f : 0.35f, 0.95f);

        float kw = g_TextS.Width(KEY_LABELS[i], 1.0f);
        g_TextS.Draw(KEY_LABELS[i], cardX + (CARD_W - kw) * 0.5f,
                     baseY + yOff + CARD_H - 40.0f, 1.0f, tr, tg, tb, 0.95f);
    }

    if (g_HoveredAug >= 0 && g_HoveredAug < nCards &&
        g_RunShopStock[g_HoveredAug] >= 0) {
        int hIdx = g_RunShopStock[g_HoveredAug];
        const AugDef& hDef = ALL_AUGS[hIdx];
        const wchar_t* hDesc = AugDesc(hDef);
        float hr, hg, hb;
        GetRarityColor(hDef.rarity, hr, hg, hb);
        float boxY = baseY + CARD_H + 20.0f;
        float boxW = TOTAL_W;
        float boxH = 130.0f;
        float boxX = (sw - boxW) * 0.5f;
        drawRect(boxX, boxY, boxW, boxH, 0.03f, 0.03f, 0.05f, 0.85f);
        drawRect(boxX, boxY, boxW, 4.0f, hr, hg, hb, 1.0f);
        std::vector<std::wstring> lines;
        std::wstring cur;
        for (const wchar_t* p = hDesc; *p; ++p) {
            if (*p == L'/') {
                if (!cur.empty()) lines.push_back(cur);
                cur.clear();
            } else cur += *p;
        }
        if (!cur.empty()) lines.push_back(cur);
        int n = (int)lines.size();
        if (n < 1) n = 1;
        float maxw = 1.0f;
        for (auto& ln : lines) {
            float w = g_TextS.Width(ln.c_str(), 1.0f);
            if (w > maxw) maxw = w;
        }
        float sc = 0.9f;
        if (maxw * sc > boxW - 40.0f) sc = (boxW - 40.0f) / maxw;
        if (sc < 0.6f) sc = 0.6f;
        float lineH = 30.0f * sc;
        float startY = boxY + 18.0f + (boxH - 18.0f - lineH * n) * 0.5f;
        for (int li2 = 0; li2 < n; li2++) {
            const wchar_t* s = lines[li2].c_str();
            float lw = g_TextS.Width(s, sc);
            g_TextS.Draw(s, boxX + (boxW - lw) * 0.5f,
                         startY + lineH * (float)li2, sc,
                         1.0f, 1.0f, 1.0f, 0.95f);
        }
    }
    (void)st;
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
    case GS::JOB_SELECT:
        st = g_CreativeMode ? GS::CREATIVE_CONFIG : GS::MAIN_MENU;
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
