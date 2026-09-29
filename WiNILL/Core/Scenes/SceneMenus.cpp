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

// The lobby command rail is the canonical home for the main-menu buttons.
// Scene transition ghosts must use this same anchor; keeping the old centered
// command Y here makes the previous button stack flash before it exits.
float MainMenuButtonRailStartY(float sh, float uiScale) {
    const float kRowH = 70.0f * uiScale;
    const float kGap = 10.0f * uiScale;
    constexpr float kRowCount = 5.0f;
    const float totalH = kRowCount * kRowH + (kRowCount - 1.0f) * kGap;
    float y = sh - totalH - std::max(52.0f * uiScale, sh * 0.075f);
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

void DrawVisibleConstellLine(float x1, float y1, float x2, float y2,
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

void DrawConstellationDisc(float x, float y, float radius,
                                  float r, float g, float b, float a,
                                  bool darkenBlend) {
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
    drawDiamond(x, y, size * 1.20f, r, g, b, 0.74f * a);
    drawDiamond(x, y, size * 0.42f,
                whiteSpark ? 0.96f : r,
                whiteSpark ? 1.0f  : g,
                whiteSpark ? 1.0f  : b,
                0.88f * a);
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

// Fixed ASTRAL_LOG signature constellation. Unlike record previews this shape
// never changes with selection; only its restrained phase/pulse animates.
void DrawMainOnedowLogo(float sw, float sh, float alpha, float reveal,
                               float centerXOverride, float heightMul) {
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

static float MenuCommandReveal(float timeline, int row,
                               float start = 0.20f, float stagger = 0.14f,
                               float duration = 0.32f) {
    return Smoothstep(LogoClamp01((timeline - start - (float)row * stagger) / duration));
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
}void Scene_MainMenu(const SceneCtx& c) {
    // The main menu owns no page-entry texture fade. Inline pages set their
    // own reveal immediately before drawing, so returning here can never
    // inherit a partially faded CircleTexture from the previous page.
    SetSceneTextureReveal(1.0f);
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    GLFWwindow* window = c.window;
    const float uiScale = UiScale(sw, sh);
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
        s_menuExitT += std::min(delta, 0.05f);
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
        float ir, ig, ib;
    };
    static const SBtnDef kBtns[] = {
        { 0.18f, 0.62f, 0.96f },
        { 0.18f, 0.62f, 0.96f },
        { 0.18f, 0.62f, 0.96f },
        { 0.18f, 0.62f, 0.96f },
        { 0.18f, 0.62f, 0.96f },
    };
    // Lobby buttons use one localized route label; route IDs must not leak
    // through as English-only text when Korean or Japanese is selected.
    const int   kBtnCount = 5;
    const float BW = std::min(560.0f * uiScale,
                              std::max(420.0f * uiScale, sw * 0.34f));
    const float BH = 70.0f * uiScale;
    const float BGAP = 10.0f * uiScale;
    float totalBH = kBtnCount * BH + (kBtnCount - 1) * BGAP;
    float btnX0   = std::max(58.0f * uiScale, sw * 0.075f);
    // The lobby command rail belongs to the lower-left corner, matching the
    // navigation language used by the other pages. Keep a responsive bottom
    // margin, but never let the rail climb into the logo on short windows.
    float btnY0 = MainMenuButtonRailStartY(sh, uiScale);

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
        float rawBtnPh = (s_introT - kTitleDur - (float)i * kBtnStag) / kBtnDur;
        rawBtnPh = std::max(0.0f, std::min(rawBtnPh, 1.0f));
        float reveal = Smoothstep(rawBtnPh);
        bool selected = (s_menuSelect == i);
        float exitP = Smoothstep(std::min(1.0f, s_menuExitT / 0.50f));
        const bool buttonReady = !booting && !introActive && !introWasActive
                              && !exitActive && g_FadeDir == 0;
        const float priorBx = baseBx + (1.0f - reveal) * 34.0f - 10.0f * kHoverT[i];
        const float priorBy = baseBy + (exitActive && !selected ? exitP * 28.0f : 0.0f);
        bool hov = UpdatePanelButtonHover(kHoverT[i], buttonReady,
                                          mx, my,
                                          priorBx, priorBy, BW, BH,
                                          delta, 4.0f * uiScale);
        float t = kHoverT[i];
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
        const wchar_t* routeVariants[] = {
            MainMenuRouteLabel(0, i),
            MainMenuRouteLabel(1, i),
            MainMenuRouteLabel(2, i),
        };
        DrawPanelButton(route, bx, by, BW, BH,
                        ar, ag, ab, rowA, t, selected, selectPulse,
                        now + (float)i * 0.17f,
                        uiScale, false, PanelButtonSlideSide::Left, false,
                        routeVariants, 3);

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
        const float handoffT = s_menuExitT;
        s_menuSelect = -1;
        s_menuExitT = 0.0f;
        switch (selectedMenu) {
        case 0:
            ResetRunConfigUi(handoffT);
            s_MainMenuRunConfigPanel = true;
            break;
        case 1:
            ResetShopUi(handoffT);
            s_MainMenuArmoryPanel = true;
            break;
        case 2:
            ResetCodexUi(handoffT);
            s_MainMenuCodexPanel = true;
            break;
        case 3:
            ResetSettingsUi(handoffT);
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
    static constexpr int SHOP_TAB_COUNT = 3;
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
    static int   s_navHoldDir  = 0;
    static float s_navHoldT    = 0.0f;
    static float s_navRepeatT  = 0.0f;
    static bool  s_dragging    = false;
    static bool  s_dragMoved   = false;
    static float s_dragAccum   = 0.0f;
    static double s_dragLastY  = 0.0;
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
    if (s_ShopNeedsOpenInit) {
        // Open ARMORY directly on the first category. The start-module node
        // should be visible as soon as the shop finishes entering; requiring
        // an extra click on the already-selected tab made it look missing.
        s_browseItems = true;
        s_browseT = 0.0f;
        s_browseInputLock = true;
        s_ShopNeedsOpenInit = false;
    }
    g_ShopEntryT += dt;
    if (g_ShopEntryT > 1.0f) g_ShopEntryT = 1.0f;

    const bool rawRmb = c.window && glfwGetMouseButton(c.window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    const bool rmbClick = rawRmb && !g_RmbPrev;
    const bool rawEsc = c.window && glfwGetKey(c.window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    if (c.inputFocusChanged) s_prevEsc = rawEsc;
    const bool escClick = rawEsc && !s_prevEsc;
    s_prevEsc = rawEsc;
    if (c.inputFocusChanged) {
        s_navHoldDir = 0;
        s_navHoldT = 0.0f;
        s_navRepeatT = 0.0f;
        s_dragging = false;
        s_dragAccum = 0.0f;
    }

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
    const float uiS = UiScale(sw, sh);
    const float mainBW = std::min(560.0f * uiS, std::max(420.0f * uiS, sw * 0.34f));
    const float mainBH = 70.0f * uiS;
    const float mainGap = 15.0f * uiS;
    const float mainTotalH = 5.0f * mainBH + 4.0f * mainGap;
    const float mainX = std::max(58.0f * uiS, sw * 0.075f);
    const float mainY = MainMenuCommandStartY(sh, uiS);

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
        CaptureBackdrop(BackdropBlurCaptureIntervalSeconds());
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

        if (s_tab == 0) {
            addGroup(nli == 0 ? L"\xCF54\xC5B4 \xD574\xAE08" : L"CORE UNLOCKS",
                     0.38f, 0.82f, 1.00f);
            for (int i = 0; i < META_COUNT; i++)
                if (i != META_RESERVED_STARTAUG)
                    addItem(KEY_META + i, MetaName(i), 0.38f, 0.82f, 1.00f);
            addGroup(nli == 0 ? L"\xD654\xBA74 \xD504\xB85C\xD30C\xC77C" : L"DISPLAY PROFILES",
                     1.00f, 0.84f, 0.28f);
            for (int i = 1; i < ACCENT_COUNT; ++i)
                addItem(KEY_THEME + i, AccentName(i),
                        ACCENT_THEMES[i].r, ACCENT_THEMES[i].g, ACCENT_THEMES[i].b);
        }
        if (s_tab == 1) {
            addGroup(nli == 0 ? L"\xC18C\xCD1D \xC2DC\xC2A4\xD15C" : L"RIFLE CONSTELLATION", 0.72f, 0.92f, 0.42f);
            for (int i = 0; i < AUG_TOTAL; i++)
                if (!AugRemoved(ALL_AUGS[i].type) && augWeapCat(ALL_AUGS[i].type) == 0)
                    addItem(KEY_AUG + i, ALL_AUGS[i].locName[li], 0.72f, 0.92f, 0.42f);
        }
        if (s_tab == 2) {
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
        BeginDeferredSceneText();
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
        const float purchaseButtonSc = UiTextScale(
            g_TextL, UiTextLevel::Title, ui);
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
            SHOP_ACTION_BLOCKED
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
            float textW = g_TextL.Width(label, textSc);
            const float textRight = x + w - cut * 0.56f - 18.0f * ui;
            const float textMaxW = std::max(48.0f * ui,
                                            textRight - x - 18.0f * ui);
            if (textW > textMaxW && textW > 0.0f) {
                textSc *= textMaxW / textW;
                textW = g_TextL.Width(label, textSc);
            }
            const float textR = 0.94f;
            const float textG = 0.98f;
            const float textB = 1.00f;
            DrawShadowedText(g_TextL, label, textRight - textW,
                             y - 20.0f * ui, textSc, textR, textG, textB,
                             0.56f + 0.42f * actionEnabledT, 0.84f);
        };

        DrawPersistentSceneLeftVignette(sw, sh, 0.055f);

        // Keep this loop byte-for-byte in the same visual family as the other
        // pages' shared root commands.
        static const wchar_t* kShopRoute[SHOP_TAB_COUNT][3] = {
            { L"\xD574\xAE08", L"UNLOCK", L"\u89E3\u653E" },
            { L"\xC18C\xCD1D", L"RIFLE", L"\u30E9\u30A4\u30D5\u30EB" },
            { L"\xC704\xC131", L"FIELD", L"\u30D5\u30A3\u30FC\u30EB\u30C9" },
        };
        static const wchar_t* kShopBack[3] = { L"\uB4A4\uB85C", L"BACK", L"\u623B\u308B" };
        const int shopLanguage = std::max(0, std::min(2, LangIndex()));
        static const float kShopR[SHOP_TAB_COUNT] = { 0.38f, 0.72f, 0.62f };
        static const float kShopG[SHOP_TAB_COUNT] = { 0.82f, 0.92f, 0.48f };
        static const float kShopB[SHOP_TAB_COUNT] = { 1.00f, 0.42f, 1.00f };
        const float railX = rootX;
        const float railY = shopRailY;
        const float railW = rootW;
        const float railH = rootH;
        const float categoryWake = wake * categoryA;
        for (int i = 0; i < SHOP_TAB_COUNT + 1; ++i) {
            const bool isBack = i == SHOP_TAB_COUNT;
            const float tx = railX - 58.0f * itemA;
            const float ty = railY + (float)i * (railH + rootGap);
            const float reveal = s_backExit ? wake : MenuCommandReveal(g_ShopEntryT, i);
            float& hovT = isBack ? s_backHov : s_tabHov[i];
            const float hitX = tx + (1.0f - reveal) * 34.0f - 10.0f * hovT;
            const bool hov = inputReady
                          && PanelButtonHit(mx, my, hitX, ty, railW, railH,
                                            18.0f * ui);
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
            const wchar_t* route = isBack ? kShopBack[shopLanguage]
                                          : kShopRoute[i][shopLanguage];
            const wchar_t* routeVariants[3] = {
                isBack ? kShopBack[0] : kShopRoute[i][0],
                isBack ? kShopBack[1] : kShopRoute[i][1],
                isBack ? kShopBack[2] : kShopRoute[i][2]
            };
            const float rowA = reveal * categoryWake;
            // Once SHOP is interactive, every category label remains fully
            // readable. Only the page exit is allowed to lower the rail.
            const float tabRowA = (!s_backExit && g_ShopEntryT >= 0.55f)
                ? 1.0f : rowA;
            const float selectPulse = selected
                ? 0.16f + 0.16f * sinf(now * 4.0f) : 0.0f;
            const float slide = (1.0f - reveal) * 34.0f;
            DrawUnifiedMenuCommand(route,
                                   tx + slide - 10.0f * hovT, ty,
                                   railW, railH, rr, gg, bb, tabRowA, hovT,
                                   selected, selectPulse,
                                   now + (float)i * 0.17f,
                                   uiS, true, true, false,
                                   routeVariants, 3);
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
        const float detailX = sw * 0.63f;
        const float detailW = std::max(360.0f * ui,
                                       std::min(sw * 0.29f,
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
            if (c.inputFocusChanged) {
                s_navHoldDir = navDir;
                s_navHoldT = 0.0f;
                s_navRepeatT = 0.0f;
            }
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
                const std::vector<std::wstring> segments = TarotWrap(
                    desc, detailDescriptionSc, detailW);
                int line = 0;
                for (const std::wstring& segment : segments) {
                    if (line >= 3) break;
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
            drawRightFit(g_TextS, L"플레이 중 획득 효과 · 도감 전용",
                         detailRight, tradeY - 20.0f * ui,
                         detailDescriptionSc, detailW, whiteR, whiteG, whiteB,
                         0.82f * detailA, 0.60f);
        }

        DrawShadowedText(g_TextS, L"ESC / RMB  뒤로가기",
                         sw - std::max(74.0f, sw * 0.075f) -
                         g_TextS.Width(L"ESC / RMB  뒤로가기", 0.31f * ui),
                         sh - 48.0f * ui, 0.31f * ui,
                         0.52f, 0.68f, 0.82f, 0.60f * designA, 0.58f);

        // All CircleTexture/icon work for the SHOP frame is complete here.
        // Release the final text pass only now so no late glow can cover a
        // tab, list label, price, or action label.
        EndDeferredSceneText();

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

    static constexpr int CAT_COUNT = 2;
    static int   s_cat = 0;
    static float s_rootHover[CAT_COUNT + 1] = {};
    static int   s_sel[CAT_COUNT] = { -1, -1 };
    static float s_scroll[CAT_COUNT] = {};
    static bool  s_backExit = false;
    static float s_backOutT = 0.0f;
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
    const bool rmbClick = rawRmb && !g_RmbPrev;
    const bool rawEsc = c.window && glfwGetKey(c.window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    if (c.inputFocusChanged) s_prevEsc = rawEsc;
    const bool escClick = rawEsc && !s_prevEsc;
    s_prevEsc = rawEsc;

    const bool rawSearchBackspace = c.window &&
        glfwGetKey(c.window, GLFW_KEY_BACKSPACE) == GLFW_PRESS;
    if (c.inputFocusChanged) {
        s_navHoldDir = 0;
        s_navHoldT = 0.0f;
        s_navRepeatT = 0.0f;
        s_dragging = false;
        s_dragAccum = 0.0f;
        s_prevSearchBackspace = rawSearchBackspace;
        s_searchBackspaceT = 0.0f;
        s_searchBackspaceRepeating = false;
    }
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

    const float uiS = UiScale(sw, sh);
    const float mainBW = std::min(560.0f * uiS, std::max(420.0f * uiS, sw * 0.34f));
    const float mainBH = 70.0f * uiS;
    const float mainGap = 15.0f * uiS;
    const float mainTotalH = 5.0f * mainBH + 4.0f * mainGap;
    const float mainX = std::max(58.0f * uiS, sw * 0.075f);
    const float mainY = MainMenuCommandStartY(sh, uiS);
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

    struct RootDef { const wchar_t* label[3]; float r, g, b; };
    static const RootDef ROOTS[CAT_COUNT + 1] = {
        { { L"\uAD00\uCE21\uCCB4", L"ENTITIES", L"\u89B3\u6E2C\u5BFE\u8C61" },
          0.48f, 0.82f, 1.00f },
        { { L"\uBAA8\uB4C8", L"MODULES", L"\u30E2\u30B8\u30E5\u30FC\u30EB" },
          0.62f, 0.52f, 1.00f },
        { { L"\uB4A4\uB85C", L"BACK", L"\u623B\u308B" },
          0.42f, 0.62f, 0.78f },
    };
    const int rootLanguage = std::max(0, std::min(2, LangIndex()));
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
        const float ghostRailY = MainMenuButtonRailStartY(sh, uiS);
        const float ghostGap = 10.0f * uiS;
        const float anchorX = mainX - ghostSlide - 36.0f;
        BindMainShader();
        drawRect(anchorX, std::max(22.0f, MainLogoTop(sh) - 20.0f),
                 1.2f, mainTotalH + 170.0f,
                 0.48f, 0.82f, 1.0f, 0.12f * oldMenuA);
        for (int i = 0; i < 5; ++i) {
            const bool focus = (!s_backExit && i == 2);
            const float rowY = ghostRailY + (float)i * (mainBH + ghostGap);
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
            const wchar_t* route = MainMenuRouteLabel(ghostLang, i);
            const wchar_t* routeVariants[] = {
                MainMenuRouteLabel(0, i),
                MainMenuRouteLabel(1, i),
                MainMenuRouteLabel(2, i),
            };
            float routeSc = UiTextScale(g_TextL, UiTextLevel::Title, uiS);
            const float availableW = std::max(1.0f, mainBW - 16.0f);
            const float availableH = std::max(1.0f, mainBH - 8.0f);
            const float widestRoute = MaxLocalizedTextWidth(
                g_TextL, routeVariants, 3, routeSc);
            const float routeH = g_TextL.Height(route, routeSc);
            const float textFit = std::min(1.0f,
                std::min(widestRoute > 0.0f ? availableW / widestRoute : 1.0f,
                         routeH > 0.0f ? availableH / routeH : 1.0f));
            routeSc *= textFit;
            const float routeY2 = rowY
                + std::max(0.0f, (mainBH - g_TextL.Height(route, routeSc)) * 0.5f);
            const float gr = s_backExit ? 1.0f : 1.0f - 0.52f * (1.0f - active);
            const float gg = s_backExit ? 1.0f : 1.0f - 0.18f * (1.0f - active);
            g_TextL.Draw(route, rowX + 8.0f, routeY2, routeSc,
                         gr, gg, 1.0f, rowA);
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
        const float reveal = s_backExit ? wake : MenuCommandReveal(g_CodexEntryT, i);
        const float priorBx = tx + (1.0f - reveal) * 34.0f - 10.0f * s_rootHover[i];
        const float hitX = priorBx - 26.0f;
        const float hitRight = std::min(priorBx + rootW + 18.0f * uiS,
                                        depth2X - 34.0f * uiS);
        const bool hov = inputReady
            && (mx >= hitX && mx < hitRight && my >= ty && my < ty + rootH);
        s_rootHover[i] = UpdateMenuCommandHover(s_rootHover[i], hov, dt);

        if (hov && lmb && !g_LmbPrev) {
            if (isBack) beginBack();
            else if (s_cat != i) s_cat = i;
        }

        const bool sel = (!isBack && s_cat == i);
        const RootDef& rd = ROOTS[i];
        const float rowA = reveal * wake;
        const float selectPulse = sel ? (0.12f + 0.12f * sinf(now * 3.3f)) : 0.0f;
        const float bx = tx + (1.0f - reveal) * 34.0f - 10.0f * s_rootHover[i];
        const float by = ty;

        DrawUnifiedMenuCommand(rd.label[rootLanguage], bx, by, rootW, rootH,
                               rd.r, rd.g, rd.b, rowA, s_rootHover[i], sel,
                               selectPulse, now + (float)i * 0.17f,
                               uiS, false, true, false,
                               rd.label, 3);
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
    }

    int unlockTotal[CAT_COUNT] = {
        (int)(sizeof(kCodexMobIds) / sizeof(kCodexMobIds[0])), 0
    };
    int unlockSeen[CAT_COUNT] = {};
    for (int id : kCodexMobIds)
        if (CodexMobSeen(id)) ++unlockSeen[0];
    for (int i = 0; i < AUG_TOTAL; ++i) {
        if (AugRemoved(ALL_AUGS[i].type)) continue;
        ++unlockTotal[1];
        if (CodexAugSeen(i)) ++unlockSeen[1];
    }

    int observedCount = 0;
    int recordCount = 0;
    for (int i = 0; i < itemCount; ++i) {
        if (items[i].isGroup) continue;
        ++recordCount;
        if (items[i].seen) ++observedCount;
    }
    wchar_t archivePath[96];
    wchar_t archiveProgress[64];
    swprintf_s(archivePath, L"ASTRAL ARCHIVE / %ls",
               ROOTS[s_cat].label[rootLanguage]);
    swprintf_s(archiveProgress, L"%02d / %02d OBSERVED", observedCount, recordCount);
    // The restored layout deliberately carries no page header or enclosing
    // archive panel: the root commands, orbit, and record itself are enough.
    (void)archivePath;

    // Use the open upper-left canvas for search and a compact global unlock
    // summary. Search filters only discovered records in the active category.
    auto codexText = [&](const wchar_t* kr, const wchar_t* en,
                         const wchar_t* jp = nullptr) -> const wchar_t* {
        if (li == 0) return kr;
        if (li == 2 && jp) return jp;
        return en;
    };
    const float utilityX = mainX;
    const float utilityW = std::min(sw * 0.40f, rootW + 120.0f * uiS);
    const float searchW = std::max(180.0f * uiS,
                                   std::min(420.0f * uiS,
                                            utilityW - 24.0f * uiS));
    const float utilityY = std::max(76.0f, sh * 0.11f);
    const float searchY = utilityY + 44.0f * uiS;
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
        ? codexText(L"\uAC80\uC0C9 / \uC785\uB825\uC911", L"SEARCH / ACTIVE", L"\u691C\u7D22\u4E2D")
        : codexText(L"\uAC80\uC0C9", L"SEARCH", L"\u691C\u7D22");
    const float searchTitleScale = UiTextScale(g_TextS, UiTextLevel::Subtitle, uiS);
    DrawShadowedText(g_TextS, searchTitle,
                     utilityX, utilityY, searchTitleScale,
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
    std::wstring searchDisplay;
    if (hasSearch)
        searchDisplay = std::wstring(g_CodexSearch);
    else if (!s_searchFocused)
        searchDisplay = std::wstring(codexText(L"\uC785\uB825\uD558\uC5EC \uAC80\uC0C9", L"TYPE TO FILTER", L"\u5165\u529B\u3057\u3066\u691C\u7D22"));
    const bool searchCaretOn = s_searchFocused && (((int)(now * 2.0f)) & 1) == 0;
    if (searchCaretOn)
        searchDisplay += L"|";
    float searchSc = UiTextScale(g_TextS, hasSearch
        ? UiTextLevel::Description : UiTextLevel::Supporting, uiS);
    const float searchMaxW = std::max(30.0f * uiS, searchW - clearW - 58.0f * uiS);
    const float searchTextW0 = g_TextS.Width(searchDisplay.c_str(), searchSc);
    if (searchTextW0 > searchMaxW && searchTextW0 > 0.0f)
        searchSc *= searchMaxW / searchTextW0;
    const float searchTextX = utilityX + 16.0f * uiS;
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
    const wchar_t* progressTitle = codexText(
        L"\uB3C4\uAC10 \uD574\uAE08 \uBAA9\uB85D", L"UNLOCK PROGRESS", L"\u56F3\u9451\u89E3\u653E\u9032\u6357");
    const float progressTitleScale = UiTextScale(g_TextS, UiTextLevel::Subtitle, uiS);
    DrawShadowedText(g_TextS,
                     progressTitle,
                     utilityX, progressTitleY, progressTitleScale,
                     curRoot.r, curRoot.g, curRoot.b, 0.84f * wake, 0.58f);
    const float progressBarX = utilityX;
    const float progressBarW = std::max(48.0f * uiS,
                                        std::min(320.0f * uiS,
                                                 utilityW - 16.0f * uiS));
    const float resultSc = UiTextScale(g_TextS, UiTextLevel::Supporting, uiS);
    const float resultW = g_TextS.Width(archiveProgress, resultSc);
    DrawShadowedText(g_TextS, archiveProgress,
                     progressBarX + progressBarW - resultW,
                     progressTitleY + (g_TextS.Height(progressTitle, progressTitleScale)
                                      - g_TextS.Height(archiveProgress, resultSc)) * 0.5f,
                     resultSc, 0.62f, 0.72f, 0.86f, 0.66f * wake, 0.52f);
    // Give each unlock entry its own two-line block: a large readout first,
    // then the archive line directly underneath it.  This keeps the progress
    // data legible without competing with the search field above.
    const float progressRowY = progressTitleY
        + g_TextS.Height(progressTitle, progressTitleScale) + 10.0f * uiS;
    const float progressRowStep = 68.0f * uiS;
    for (int i = 0; i < CAT_COUNT; ++i) {
        const float rowY = progressRowY + (float)i * progressRowStep;
        const RootDef& progressRoot = ROOTS[i];
        const float progressRootScale = UiTextScale(g_TextS, UiTextLevel::Subtitle, uiS);
        DrawShadowedText(g_TextS, progressRoot.label[rootLanguage],
                         utilityX, rowY, progressRootScale,
                         progressRoot.r, progressRoot.g, progressRoot.b,
                         (s_cat == i ? 0.98f : 0.72f) * wake, 0.54f);
        wchar_t progressBuf[32];
        swprintf_s(progressBuf, L"%02d / %02d", unlockSeen[i], unlockTotal[i]);
        const float progressSc = UiTextScale(g_TextS, UiTextLevel::Description, uiS);
        const float progressW = g_TextS.Width(progressBuf, progressSc);
        DrawShadowedText(g_TextS, progressBuf,
                         progressBarX + progressBarW - progressW,
                         rowY + (g_TextS.Height(progressRoot.label[rootLanguage], progressRootScale)
                               - g_TextS.Height(progressBuf, progressSc)) * 0.5f,
                         progressSc,
                         0.86f, 0.92f, 1.00f, 0.88f * wake, 0.54f);
        const float trackY = rowY + g_TextS.Height(progressRoot.label[rootLanguage], progressRootScale)
                           + 8.0f * uiS;
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
    int stepRequest = 0;
    const int navDir = keyUpNow == keyDownNow ? 0 : (keyUpNow ? -1 : 1);
    if (c.inputFocusChanged) {
        s_navHoldDir = navDir;
        s_navHoldT = 0.0f;
        s_navRepeatT = 0.0f;
    }
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
    // Keep the selected record's orbital node on the screen center while
    // preserving the existing arc, row spacing and list presentation.
    const float itemR = sw * 0.70f;
    const float chartCX = sw * 0.50f + itemR;
    const float chartShiftX = chartCX - sw;
    const float chartCY = sh * 0.51f;
    const float chartR = std::max(sh * 0.58f, sw * 0.44f);
    // Entity silhouettes use their gameplay footprint (Gravis includes a
    // wide gravity field), so give that category more vertical breathing room.
    const float rowStep = 245.0f * uiS;
    const bool overOrbit = inputReady &&
        mx >= depth2X - 30.0f * uiS + chartShiftX &&
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
        const float lineEndX = isSel
            ? std::min(ax + lineLength, rightX - 18.0f * uiS)
            : ax + lineLength;
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
        const wchar_t* emptyTitle = codexText(L"검색 결과 없음", L"NO MATCHES", L"検索結果なし");
        const wchar_t* emptyHint = codexText(L"다른 검색어를 입력하세요", L"TRY A DIFFERENT SEARCH", L"別の語句を入力してください");
        DrawConstellationDisc(emptyCX, emptyY, 34.0f * uiS,
                              curRoot.r, curRoot.g, curRoot.b,
                              0.12f * rightWake);
        DrawVisibleConstellNode(emptyCX, emptyY, 7.0f * uiS,
                                curRoot.r, curRoot.g, curRoot.b,
                                0.62f * rightWake, false);
        const float titleSc = UiTextScale(g_TextL, UiTextLevel::Title, uiS);
        const float hintSc = UiTextScale(g_TextS, UiTextLevel::Supporting, uiS);
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
    auto drawScanDescription = [&](const wchar_t* text, float x, float y,
                                   float maxW, float sc,
                                   float r, float g, float b, float a,
                                   int seed) {
        std::vector<std::wstring> lines;
        std::wstring line;
        if (text) {
            for (const wchar_t* p = text; *p; ++p) {
                if (*p == L'\n') {
                    lines.push_back(line);
                    line.clear();
                    continue;
                }
                std::wstring candidate = line;
                candidate += *p;
                if (!line.empty() && g_TextS.Width(candidate.c_str(), sc) > maxW) {
                    const size_t split = line.find_last_of(L" \t");
                    if (split != std::wstring::npos) {
                        std::wstring carry = line.substr(split + 1);
                        line.resize(split);
                        if (!line.empty()) lines.push_back(line);
                        line = std::move(carry);
                        if (*p != L' ') line.push_back(*p);
                    } else {
                        lines.push_back(line);
                        line.clear();
                        if (*p != L' ') line.push_back(*p);
                    }
                } else {
                    line = std::move(candidate);
                }
            }
        }
        if (!line.empty()) lines.push_back(line);
        const float lineH = std::max(22.0f * uiS,
            g_TextS.Height(L"Ag", sc) + 5.0f * uiS);
        for (size_t i = 0; i < lines.size(); ++i)
            drawScanTextS(lines[i].c_str(), x, y + (float)i * lineH,
                          sc, r, g, b, a, seed + (int)i);
        return y + (float)lines.size() * lineH;
    };

    auto selectedSeen = [&]() -> bool {
        int key = s_sel[s_cat];
        if (key < 0) return false;
        if (s_cat == 0) return CodexMobSeen(key);
        return s_cat == 1 && key >= 0 && key < AUG_TOTAL && CodexAugSeen(key);
    };
    auto selectedTitle = [&]() -> const wchar_t* {
        int key = s_sel[s_cat];
        if (key < 0) return L"UNASSIGNED";
        if (s_cat == 0) return selectedSeen() ? MobName(key) : L"???";
        return selectedSeen() ? AugName(ALL_AUGS[key]) : L"???";
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
        return AugDesc(ALL_AUGS[key]);
    };

    const int selKey = s_sel[s_cat] < 0 ? 0 : s_sel[s_cat];
    const bool seen = selectedSeen();
    float cr = curRoot.r, cg = curRoot.g, cb = curRoot.b;
    if (s_cat == 1 && selKey >= 0 && selKey < AUG_TOTAL)
        GetRarityColor(ALL_AUGS[selKey].rarity, cr, cg, cb);

    // Fixed record block inside the chart, matching the original wide-open
    // composition.  It does not move with the orbit or label animation.
    const float detailX = sw * 0.68f;
    const float detailW = std::max(320.0f * uiS,
                                   std::min(sw * 0.26f, sw - detailX - 54.0f * uiS));
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

    float titleSc = UiTextScale(g_TextL, UiTextLevel::Title, uiS);
    const float selectedTitleW = g_TextL.Width(selectedTitle(), titleSc);
    if (selectedTitleW > detailW && selectedTitleW > 0.0f)
        titleSc *= detailW / selectedTitleW;
    const float idSc = UiTextScale(g_TextS, UiTextLevel::Supporting, uiS);
    const float descSc = UiTextScale(g_TextS, UiTextLevel::Description, uiS);

    drawScanTextS(idBuf, detailX, infoY + 16.0f * uiS,
                  idSc, 0.62f, 0.72f, 0.86f,
                  0.74f * rightWake, selKey + s_cat * 31);
    drawScanTextL(selectedTitle(), detailX, infoY + 44.0f * uiS,
                  titleSc, 0.94f, 0.98f, 1.0f,
                  0.98f * rightWake, selKey + 7);

    const float descY = infoY + 44.0f * uiS
        + g_TextL.Height(selectedTitle(), titleSc) + 12.0f * uiS;
    const float summarySc = UiTextScale(g_TextS, UiTextLevel::Subtitle, uiS);
    const wchar_t* summaryText = codexText(L"기록 요약", L"RECORD SUMMARY", L"記録の概要");
    DrawShadowedText(g_TextS, summaryText, detailX, descY,
                     summarySc, cr, cg, cb, 0.72f * rightWake, 0.54f);
    const float descriptionY = descY + g_TextS.Height(summaryText, summarySc)
                             + 8.0f * uiS;
    const float descriptionEndY = drawScanDescription(
        selectedDesc(), detailX, descriptionY, detailW, descSc,
        0.76f, 0.84f, 0.94f, 0.86f * rightWake, selKey + 13);

    const float metaY = descriptionEndY + 14.0f * uiS;
    LogoLine(detailX, metaY - 10.0f * uiS,
             detailX + detailW, metaY - 10.0f * uiS,
             0.7f * uiS, cr, cg, cb, 0.14f * rightWake);
    drawScanTextS(statBuf, detailX, metaY,
                  idSc, 0.84f, 0.90f, 0.98f,
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
                  idSc, 0.62f, 0.70f, 0.82f,
                  0.70f * rightWake, selKey + 23);
    g_TextS.Draw(L"[ ARCHIVE_READ_ONLY ]", detailX, metaY + 52.0f * uiS,
                 idSc, cr, cg, cb, 0.82f * rightWake);

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
    static float s_catHover[2] = {};
    static float s_catFocus[2] = {};
    static float s_panelT      = 1.0f;
    static bool  s_catLoaded[2]= {};
    static int   s_sel[2]      = { -1, -1 };
    static float s_scroll[2]   = {};
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
    float uiS = UiScale(sw, sh, 0.975f, 0.920f, 0.72f, 1.30f);
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
    struct CatDef { const wchar_t* id; const wchar_t* name[3]; float r, g, b; };
    static const CatDef kCats[] = {
        { L"ENTITIES", { L"관측체", L"ENTITIES", L"観測対象" }, 0.48f, 0.82f, 1.00f },
        { L"MODULES",  { L"모듈", L"MODULES", L"モジュール" }, 0.62f, 0.52f, 1.00f },
    };
    const int kCatCount = 2;
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
    auto drawWrappedS = [&](const wchar_t* text, float x, float y,
                            float maxW, float maxH, float sc,
                            float r, float g, float b, float a) {
        std::vector<std::wstring> lines;
        std::wstring line;
        if (text) {
            for (const wchar_t* p = text; *p; ++p) {
                if (*p == L'\n') {
                    lines.push_back(line);
                    line.clear();
                    continue;
                }
                std::wstring candidate = line;
                candidate += *p;
                if (!line.empty() && g_TextS.Width(candidate.c_str(), sc) > maxW) {
                    const size_t split = line.find_last_of(L" \t");
                    if (split != std::wstring::npos) {
                        std::wstring carry = line.substr(split + 1);
                        line.resize(split);
                        if (!line.empty()) lines.push_back(line);
                        line = std::move(carry);
                        if (*p != L' ') line.push_back(*p);
                    } else {
                        lines.push_back(line);
                        line.clear();
                        if (*p != L' ') line.push_back(*p);
                    }
                } else {
                    line = std::move(candidate);
                }
            }
        }
        if (!line.empty()) lines.push_back(line);
        const float lineH = std::max(22.0f * uiS,
            g_TextS.Height(L"Ag", sc) + 4.0f * uiS);
        for (size_t i = 0; i < lines.size() && (i + 1) * lineH <= maxH; ++i)
            g_TextS.Draw(lines[i].c_str(), x, y + (float)i * lineH,
                         sc, r, g, b, a);
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
                 UiTextScale(g_TextL, UiTextLevel::Title, uiS),
                 1.0f, 1.0f, 1.0f, 0.98f * wake);

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
        const wchar_t* tabLabel = kCats[i].name[li];
        const float idSc = UiTextScale(g_TextS, UiTextLevel::Title, uiS);
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
            const float groupSc = UiTextScale(g_TextS, UiTextLevel::Subtitle, uiS);
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

            float itemSc = UiTextScale(g_TextS, UiTextLevel::Title, uiS);
            while (itemSc > UiTextScale(g_TextS, UiTextLevel::Supporting, uiS)
                   && g_TextS.Width(litem.label, itemSc) > listW2 - 54.0f * uiS)
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
                         UiTextScale(g_TextS, UiTextLevel::Supporting, uiS),
                         uiR, uiG, uiB, 0.78f*detailA);
            g_TextL.Draw(MobName(selItem), infoX + 18.0f*uiS, titleBoxY + 34.0f*uiS,
                         UiTextScale(g_TextL, UiTextLevel::Title, uiS),
                         1.0f, 1.0f, 1.0f, 0.96f*detailA);
            drawRect(infoX + 18.0f*uiS, titleBoxY + titleBoxH - 16.0f*uiS, infoW - 36.0f*uiS, 1.5f*uiS,
                     uiR, uiG, uiB, 0.28f*detailA);
            drawInfoQuad(infoX, descBoxY, infoW, descBoxH, detailA);
            drawWrappedS(seen ? MobDesc(selItem) : L"???",
                         infoX + 18.0f*uiS, descBoxY + 24.0f*uiS,
                         infoW - 36.0f*uiS, descBoxH - 42.0f*uiS,
                         UiTextScale(g_TextS, UiTextLevel::Description, uiS),
                         0.86f, 0.92f, 1.0f, 0.92f*detailA);
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
                    const float logScale = UiTextScale(g_TextS, UiTextLevel::Supporting, uiS);
                    g_TextS.Draw(logR[ll].k, infoX + 18.0f*uiS,              ly, logScale, uiR, uiG, uiB, 0.72f*detailA);
                    g_TextS.Draw(L": ",       infoX + 150.0f*uiS,            ly, logScale, uiR, uiG, uiB, 0.54f*detailA);
                    g_TextS.Draw(logR[ll].v,  infoX + 172.0f*uiS,            ly, logScale, 1.0f, 1.0f, 1.0f, 0.88f*detailA);
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
                             UiTextScale(g_TextS, UiTextLevel::Supporting, uiS),
                             uiR, uiG, uiB, 0.78f*detailA);
                g_TextL.Draw(AugName(d), infoX + 18.0f*uiS, titleBoxY + 34.0f*uiS,
                             UiTextScale(g_TextL, UiTextLevel::Title, uiS),
                             1.0f, 1.0f, 1.0f, 0.96f*detailA);
                drawRect(infoX + 18.0f*uiS, titleBoxY + titleBoxH - 16.0f*uiS, infoW - 36.0f*uiS, 1.5f*uiS,
                         uiR, uiG, uiB, 0.28f*detailA);
                drawInfoQuad(infoX, descBoxY, infoW, descBoxH, detailA);
                drawWrappedS(AugDesc(d), infoX + 18.0f*uiS,
                             descBoxY + 24.0f*uiS, infoW - 36.0f*uiS,
                             descBoxH - 42.0f*uiS,
                             UiTextScale(g_TextS, UiTextLevel::Description, uiS),
                             0.86f, 0.92f, 1.0f, 0.92f*detailA);
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
                        const float logScale = UiTextScale(g_TextS, UiTextLevel::Supporting, uiS);
                        g_TextS.Draw(mlogR[ml].k, infoX + 18.0f*uiS,  mly, logScale, uiR, uiG, uiB, 0.72f*detailA);
                        g_TextS.Draw(L": ", infoX + 164.0f*uiS,       mly, logScale, uiR, uiG, uiB, 0.54f*detailA);
                        g_TextS.Draw(mlogR[ml].v, infoX + 188.0f*uiS, mly, logScale, 1.0f, 1.0f, 1.0f, 0.88f*detailA);
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
                const wchar_t* q[3] = { L"수집까지 비공개",
                                         L"Undiscovered — unlock by acquiring",
                                         L"入手すると記録が解放されます" };
                drawInfoQuad(infoX, titleBoxY, infoW, titleBoxH, detailA);
                g_TextS.Draw(q[li], infoX + 18.0f*uiS, titleBoxY + 32.0f*uiS,
                             UiTextScale(g_TextS, UiTextLevel::Description, uiS),
                             0.62f, 0.66f, 0.76f, 0.88f*detailA);
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
    const float backTextScale = UiTextScale(g_TextS, UiTextLevel::Title, uiS);
    drawCenterS(T(StrId::BTN_BACK), bx,
                by + (bh - g_TextS.Height(T(StrId::BTN_BACK), backTextScale)) * 0.5f,
                bw, backTextScale,
                backHov ? 1.0f : 0.78f, backHov ? 1.0f : 0.84f, backHov ? 1.0f : 0.96f,
                0.96f * wake);
    if (backHov && lmb && !g_LmbPrev) {
        CodexSearchClear();
        g_CodexSearchInputEnabled = false;
        g_GameManager.currentState = GameState::MAIN_MENU;
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
