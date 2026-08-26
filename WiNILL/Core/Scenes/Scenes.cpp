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
#include "../../Render/PlanetShader.h"
#include "../../Render/NebulaGlowShader.h"
#include "MainShader.h"
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
static int   g_RunConfigStep   = 0;   // 0=loadout, 1=trials/summary
static float g_RunConfigEntryT = 0.0f;
static float g_RunConfigFocusT[3] = { 0.0f, 0.0f, 0.0f };
static float g_MainMenuEntryT  = 0.0f;
static float g_SettingsEntryT  = 0.0f;
static float g_CodexEntryT     = 0.0f;
static float g_ShopEntryT      = 0.0f;
static bool  s_ShopBackRequested = false;
static bool  s_CodexBackRequested = false;
static bool  s_MainMenuArmoryPanel = false;
static bool  s_MainMenuCodexPanel = false;
static bool  s_MainMenuSettingsPanel = false;
static bool  s_MainMenuRunConfigPanel = false;
static bool  s_MainMenuResumeFromPanel = false;
static void Scene_RunConfigInline(const SceneCtx& c);
static void Scene_SettingsInline(const SceneCtx& c);
static float s_RadarCur[6]     = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
static float s_StatGlitchT     = 1.0f;
static bool  s_TrialsEnabled   = false;
static bool  s_TrialSelectPage = false;
static int   s_TrialTargetCount = 1;
static float s_TrialHover[TRIAL_SLOT_COUNT] = {};
static float s_PanelFadeT      = 1.0f;   // 0→1: 우측 패널 콘텐츠 페이드인
static int   s_PanelPrevSel    = -1;     // 마지막으로 표시된 무기 인덱스

static float UiApproach(float v, float target, float dt, float speed) {
    float k = dt * speed;
    if (k > 1.0f) k = 1.0f;
    return v + (target - v) * k;
}

static void ResetRunConfigUi() {
    g_RunConfigStep = 0;
    g_Difficulty = Difficulty::NORMAL;
    g_RunConfigEntryT = 0.0f;
    g_RunConfigFocusT[0] = g_RunConfigFocusT[1] = g_RunConfigFocusT[2] = 0.0f;
    for (int i = 0; i < 6; ++i) s_RadarCur[i] = 0.0f;
    s_StatGlitchT = 1.0f;
    s_TrialsEnabled = false;
    s_TrialSelectPage = false;
    s_TrialTargetCount = 1;
    for (int i = 0; i < TRIAL_SLOT_COUNT; ++i) s_TrialHover[i] = 0.0f;
    s_PanelFadeT   = 1.0f;
    s_PanelPrevSel = -1;
    ResetTrials();
}

static void ResetSettingsUi() {
    g_SettingsEntryT = 0.0f;
}

static void ResetCodexUi() {
    g_CodexEntryT = 0.0f;
    s_CodexBackRequested = false;
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
}

// 메인메뉴·난이도선택 공용 앰비언트 배경 (파티클 + 스캔라인 + 선택적 비네트)
static void DrawMenuBackground(float sw, float sh, float delta, float darkenAmount = 1.0f) {
    float dtp = delta; if (dtp > 0.05f) dtp = 0.05f;
    BindMainShader();
    if (darkenAmount < 0.0f) darkenAmount = 0.0f;
    if (darkenAmount > 1.0f) darkenAmount = 1.0f;

    if (darkenAmount > 0.0f) {
        // 딥 네이비 반투명 — 배경화면이 비치도록
        drawRect(0, 0, sw, sh, 0.04f, 0.06f, 0.14f, 0.20f * darkenAmount);
    }

    struct AP { float x, y, vx, vy, sz, tw; };
    static AP a_ps[55]; static bool ap_init = false;
    if (!ap_init) { ap_init = true;
        for (int i = 0; i < 55; i++) {
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
    for (int i = 0; i < 55; i++) { AP& p = a_ps[i];
        p.x += p.vx*dtp; p.y += p.vy*dtp;
        if (p.x < -8) p.x = sw+8; if (p.x > sw+8) p.x = -8;
        if (p.y < -8) p.y = sh+8; if (p.y > sh+8) p.y = -8;
        p.tw += dtp*1.6f;
        int ci = i % 3;
        drawCircle(p.x, p.y, p.sz, kNebR[ci], kNebG[ci], kNebB[ci], 0.12f + 0.10f*sinf(p.tw));
    }
    // 성운 보라 스캔라인
    for (float yy = 0.0f; yy < sh; yy += 4.0f)
        drawRect(0.0f, yy, sw, 1.0f, 0.35f, 0.20f, 0.85f, 0.018f);
    if (darkenAmount > 0.0f) {
        // 상하 비네트
        drawRect(0, 0, sw, 110.0f, 0.0f, 0.0f, 0.0f, 0.28f * darkenAmount);
        drawRect(0, sh - 90.0f, sw, 90.0f, 0.0f, 0.0f, 0.0f, 0.28f * darkenAmount);
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
    DrawLinearGradient(0.0f, 0.0f, sw * 0.60f, sh,
                       0.002f, 0.006f, 0.016f, alpha);
}

static void DrawSceneRadialVignette(float cx, float cy, float radius, float alpha) {
    if (alpha <= 0.001f || radius <= 1.0f) return;
    DrawRadialGradient(cx, cy, radius, 0.0f, 0.0f, 0.012f, alpha * 0.65f);
}

static void DrawVisibleConstellLine(float x1, float y1, float x2, float y2,
                                    float thick, float r, float g, float b, float a) {
    if (a <= 0.001f) return;
    LogoLine(x1, y1, x2, y2, thick * 2.75f, 0.0f, 0.0f, 0.012f, 0.42f * a);
    LogoLine(x1, y1, x2, y2, thick, r, g, b, a);
}

static void DrawVisibleConstellNode(float x, float y, float size,
                                    float r, float g, float b, float a) {
    if (a <= 0.001f) return;
    drawCircle(x, y, size * 3.25f, 0.0f, 0.0f, 0.012f, 0.34f * a);
    drawCircle(x, y, size * 2.05f, r * 0.08f, g * 0.08f, b * 0.10f, 0.26f * a);
    drawDiamond(x, y, size * 1.20f, r, g, b, 0.74f * a);
    drawDiamond(x, y, size * 0.42f, 0.96f, 1.0f, 1.0f, 0.88f * a);
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
            LogoLine(ax, ay, bx, by, stroke * 1.08f, 0.07f, 0.21f, 0.34f, 0.92f * alpha);
            LogoLine(ax, ay, bx, by, stroke * 0.54f, 0.70f, 0.92f, 1.00f, 0.94f * alpha);
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
            LogoLine(x1, y1, x2, y2, 1.1f, 0.38f, 0.82f, 1.0f, 0.22f * orbitA);
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
            drawDiamond(px[i], py[i], sz, 0.42f, 0.82f, 1.0f, (0.58f + 0.18f * tw) * orbitA);
            drawDiamond(px[i], py[i], sz * 0.42f, 1.0f, 1.0f, 1.0f, 0.55f * orbitA);
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
                                    float active, float pulse, float now) {
    if (active <= 0.01f || alpha <= 0.001f) return;
    BindMainShader();
    const float lineH = 20.0f + 42.0f * active + 10.0f * pulse;
    drawRect(x - 18.0f, y + h * 0.5f - lineH * 0.5f,
             2.2f, lineH, r, g, b, (0.24f + 0.55f * active + 0.12f * pulse) * alpha);
    const float scanY = y + h * (0.34f + 0.32f * fmodf(now * 0.55f, 1.0f));
    drawRect(x - 4.0f, scanY, w * (0.58f + 0.24f * active), 1.1f,
             r, g, b, (0.050f + 0.090f * active) * alpha);
    drawDiamond(x - 17.0f, y + h * 0.5f,
                3.0f + 2.6f * active + 1.8f * pulse,
                r, g, b, (0.40f + 0.34f * active + 0.16f * pulse) * alpha);
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

    float menuBgDarken = 1.0f;
    if (g_FadeDir == 1 && g_FadeTarget == GameState::RUN_CONFIG)
        menuBgDarken = 1.0f - Smoothstep(g_FadeAlpha);
    DrawMenuBackground(sw, sh, delta, menuBgDarken);
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
    static float s_canvasX[10] = {};
    static float s_canvasY[10] = {};
    static bool  s_canvasInit = false;
    static float kHoverT[5] = {};
    constexpr float kTitleDur  = 0.55f;
    constexpr float kBtnDur    = 0.45f;
    constexpr float kBtnStag   = 0.09f;
    constexpr float kTextStart = kTitleDur + 4.0f * kBtnStag + kBtnDur;
    constexpr float kTextDur   = 0.35f;
    constexpr float kIntroDone = kTextStart + kTextDur;

    if (g_MainMenuEntryT <= 0.0f) {
        s_introT = 0.0f;
        s_menuSelect = -1;
        s_menuExitT = 0.0f;
        s_canvasInit = false;
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
        { { L"시작",      L"Start",    L"スタート"  }, { L"RUN_CONFIG",  L"RUN_CONFIG",  L"RUN_CONFIG"  }, 0.48f, 0.82f, 1.00f },
        { { L"상점",      L"Armory",   L"武器庫"    }, { L"ARMORY",      L"ARMORY",      L"ARMORY"      }, 0.48f, 0.82f, 1.00f },
        { { L"도감",      L"Astral Log", L"星界記録" }, { L"ASTRAL_LOG",  L"ASTRAL_LOG",  L"ASTRAL_LOG"  }, 0.48f, 0.82f, 1.00f },
        { { L"설정",      L"Setting",  L"設定"      }, { L"CALIBRATION", L"CALIBRATION", L"CALIBRATION" }, 0.48f, 0.82f, 1.00f },
        { { L"게임 종료", L"Exit",     L"終了"      }, { L"SHUTDOWN",    L"SHUTDOWN",    L"SHUTDOWN"    }, 0.48f, 0.82f, 1.00f },
    };
    const int   kBtnCount = 5;
    const float BW = std::min(560.0f, std::max(420.0f, sw * 0.34f));
    const float BH = 70.0f;
    const float BGAP = 15.0f;
    float totalBH = kBtnCount * BH + (kBtnCount - 1) * BGAP;
    float btnX0   = std::max(58.0f, sw * 0.075f);
    float btnY0   = sh * 0.48f;
    if (btnY0 + totalBH > sh - 54.0f) btnY0 = sh - totalBH - 54.0f;
    if (btnY0 < MainLogoTop(sh) + MainLogoHeight(sh) * 0.72f) btnY0 = MainLogoTop(sh) + MainLogoHeight(sh) * 0.72f;

    auto rectHit = [&](float hpx, float hpy, float rx, float ry, float rw, float rh) -> bool {
        return hpx >= rx && hpx <= rx + rw && hpy >= ry && hpy <= ry + rh;
    };
    auto drawRunBorder = [&](float x, float y, float w, float h,
                             float r, float g, float b, float a, float thick) {
        drawRect(x, y, w, thick, r, g, b, a);
        drawRect(x, y + h - thick, w, thick, r, g, b, a);
        drawRect(x, y, thick, h, r, g, b, a);
        drawRect(x + w - thick, y, thick, h, r, g, b, a);
    };
    auto drawRunCorners = [&](float x, float y, float w, float h,
                              float r, float g, float b, float a, float len, float thick) {
        drawRect(x, y, len, thick, r, g, b, a);
        drawRect(x, y, thick, len, r, g, b, a);
        drawRect(x + w - len, y, len, thick, r, g, b, a);
        drawRect(x + w - thick, y, thick, len, r, g, b, a);
        drawRect(x, y + h - thick, len, thick, r, g, b, a);
        drawRect(x, y + h - len, thick, len, r, g, b, a);
        drawRect(x + w - len, y + h - thick, len, thick, r, g, b, a);
        drawRect(x + w - thick, y + h - len, thick, len, r, g, b, a);
    };
    DrawSceneLeftVignette(sw, sh, Smoothstep(LogoClamp01((s_introT - 0.12f) / 0.58f)) * uiA);
    {
        const float anchorX = btnX0 - 36.0f;
        const float anchorY = std::max(22.0f, MainLogoTop(sh) - 20.0f);
        const float anchorH = std::min(sh - anchorY - 26.0f, totalBH + 170.0f);
        const float anchorA = Smoothstep(LogoClamp01((s_introT - 0.28f) / 0.48f)) * uiA;
        BindMainShader();
        drawRect(anchorX, anchorY, 1.2f, anchorH,
                 0.48f, 0.82f, 1.0f, 0.17f * anchorA);
        drawDiamond(anchorX + 0.6f, anchorY + 4.0f, 2.8f,
                    0.48f, 0.82f, 1.0f, 0.22f * anchorA);
        drawDiamond(anchorX + 0.6f, anchorY + anchorH - 4.0f, 2.8f,
                    0.48f, 0.82f, 1.0f, 0.18f * anchorA);
    }
    DrawMainOnedowLogo(sw, sh, titleA, titlePhA, sw * 0.30f, 0.82f);
    const float now = (float)glfwGetTime();
    for (int i = 0; i < kBtnCount; i++) {
        float baseBx = btnX0;
        float baseBy = btnY0 + (float)i * (BH + BGAP);
        bool hov = (!booting && !introActive && !introWasActive && !exitActive && g_FadeDir == 0
                    && rectHit((float)mx, (float)my, baseBx - 26.0f, baseBy, BW + 58.0f, BH));
        float spd = delta * 10.0f; if (spd > 1.0f) spd = 1.0f;
        kHoverT[i] += ((hov ? 1.0f : 0.0f) - kHoverT[i]) * spd;
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
        float selectPulse = selected ? (0.50f + 0.50f * sinf(now * 18.0f)) * exitP : 0.0f;
        float activeT = std::max(t, selected ? exitP : 0.0f);

        DrawMenuCommandFeedback(bx, by, BW, BH, ar, ag, ab,
                                rowA, activeT, selectPulse, now + (float)i * 0.17f);

        const wchar_t* route = kBtns[i].route[li2];
        const wchar_t* sub = kBtns[i].label[0];
        float routeSc = 1.04f;
        while (routeSc > 0.82f && g_TextL.Width(route, routeSc) > BW)
            routeSc -= 0.04f;
        float subSc = 0.48f;
        float routeY = by + 2.0f;
        float subY = routeY + g_TextL.Height(route, routeSc) - 3.0f;
        float tr = 1.0f + (ar - 1.0f) * activeT;
        float tg = 1.0f + (ag - 1.0f) * activeT;
        float tb = 1.0f + (ab - 1.0f) * activeT;
        if (selected) { tr = 1.0f; tg = 1.0f; tb = 1.0f; }
        g_TextL.Draw(route, bx, routeY, routeSc,
                     tr, tg, tb, (0.82f + 0.16f * activeT + 0.18f * selectPulse) * rowA);
        g_TextS.Draw(sub, bx + 4.0f, subY, subSc,
                     0.70f + 0.18f * activeT, 0.75f + 0.16f * activeT, 0.82f + 0.12f * activeT,
                     (0.70f + 0.20f * activeT) * rowA);

        if (hov && lmb && !g_LmbPrev) {
            if (i == 0) {
                ResetRunConfigUi();
                s_MainMenuRunConfigPanel = true;
                return;
            }
            if (i == 1) {
                ResetShopUi();
                s_MainMenuArmoryPanel = true;
                return;
            }
            if (i == 2) {
                ResetCodexUi();
                s_MainMenuCodexPanel = true;
                return;
            }
            if (i == 3) {
                ResetSettingsUi();
                s_MainMenuSettingsPanel = true;
                return;
            }
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

        struct NodeDef { float x, y; };
        static const NodeDef shapes[6][10] = {
            { {0.50f,0.18f},{0.68f,0.27f},{0.78f,0.47f},{0.66f,0.68f},{0.47f,0.78f},{0.28f,0.63f},{0.22f,0.40f},{0.36f,0.24f},{0.53f,0.49f},{0.58f,0.58f} },
            { {0.50f,0.16f},{0.68f,0.26f},{0.71f,0.48f},{0.62f,0.68f},{0.50f,0.82f},{0.38f,0.68f},{0.29f,0.48f},{0.32f,0.26f},{0.50f,0.43f},{0.50f,0.55f} },
            { {0.32f,0.24f},{0.50f,0.16f},{0.69f,0.25f},{0.78f,0.47f},{0.62f,0.72f},{0.38f,0.75f},{0.25f,0.56f},{0.44f,0.42f},{0.56f,0.42f},{0.50f,0.58f} },
            { {0.50f,0.14f},{0.76f,0.32f},{0.79f,0.63f},{0.54f,0.82f},{0.24f,0.70f},{0.18f,0.40f},{0.37f,0.20f},{0.50f,0.36f},{0.50f,0.58f},{0.64f,0.49f} },
            { {0.50f,0.18f},{0.72f,0.22f},{0.82f,0.44f},{0.74f,0.68f},{0.52f,0.78f},{0.28f,0.72f},{0.18f,0.50f},{0.28f,0.28f},{0.50f,0.48f},{0.64f,0.48f} },
            { {0.50f,0.10f},{0.64f,0.30f},{0.88f,0.34f},{0.70f,0.52f},{0.78f,0.80f},{0.50f,0.64f},{0.22f,0.80f},{0.30f,0.52f},{0.12f,0.34f},{0.36f,0.30f} },
        };
        static const int edges[6][14][2] = {
            { {0,1},{1,2},{2,3},{3,4},{4,5},{5,6},{6,7},{7,0},{0,8},{8,4},{2,8},{5,9},{9,1},{-1,-1} },
            { {0,8},{8,4},{1,8},{8,7},{2,9},{9,6},{3,4},{5,6},{0,1},{1,2},{6,7},{7,0},{-1,-1},{-1,-1} },
            { {0,1},{1,2},{2,3},{3,4},{4,5},{5,6},{6,0},{0,7},{7,8},{8,2},{7,9},{9,3},{-1,-1},{-1,-1} },
            { {0,7},{7,8},{8,3},{1,7},{7,5},{2,9},{9,6},{3,4},{4,5},{5,6},{6,0},{1,2},{-1,-1},{-1,-1} },
            { {0,1},{1,2},{2,3},{3,4},{4,5},{5,6},{6,7},{7,0},{0,8},{8,4},{2,9},{9,6},{7,8},{1,9} },
            { {0,5},{1,5},{2,5},{3,5},{4,5},{5,6},{5,7},{5,8},{5,9},{1,2},{3,4},{6,7},{8,9},{-1,-1} },
        };
        int shapeIdx = (morphIdx >= 0) ? (morphIdx + 1) : 0;
        const float canvasReveal = Smoothstep(LogoClamp01((s_introT - kTitleDur - 0.18f) / 0.70f)) * uiA;
        const float collapse = exitActive ? Smoothstep(std::min(1.0f, s_menuExitT / 0.50f)) : 0.0f;
        const float cx = sw * 0.69f;
        const float cy = sh * 0.53f;
        const float cw = sw * 0.48f;
        const float ch = sh * 0.66f;
        auto drawNodeGlow = [&](float x, float y, float size, float a) {
            if (a <= 0.001f) return;
            drawCircle(x, y, size * 3.4f, 0.0f, 0.0f, 0.010f, 0.30f * a);
            drawCircle(x, y, size * 2.15f, 0.020f, 0.060f, 0.100f, 0.24f * a);
            drawDiamond(x, y, size * 1.28f, 0.48f, 0.82f, 1.0f, 0.70f * a);
            drawDiamond(x, y, size * 0.46f, 0.96f, 1.0f, 1.0f, 0.88f * a);
        };
        if (!s_canvasInit) {
            for (int i = 0; i < 10; ++i) {
                s_canvasX[i] = shapes[shapeIdx][i].x;
                s_canvasY[i] = shapes[shapeIdx][i].y;
            }
            s_canvasInit = true;
        }
        for (int i = 0; i < 10; ++i) {
            s_canvasX[i] = UiApproach(s_canvasX[i], shapes[shapeIdx][i].x, delta, 3.6f + bestHover * 4.5f);
            s_canvasY[i] = UiApproach(s_canvasY[i], shapes[shapeIdx][i].y, delta, 3.6f + bestHover * 4.5f);
        }

        float px[10], py[10];
        for (int i = 0; i < 10; ++i) {
            float ox = cx + (s_canvasX[i] - 0.5f) * cw;
            float oy = cy + (s_canvasY[i] - 0.5f) * ch;
            float spin = now * (0.035f + 0.020f * bestHover);
            float dx = ox - cx, dy = oy - cy;
            float cr = cosf(spin), sr = sinf(spin);
            ox = cx + dx * cr - dy * sr;
            oy = cy + dx * sr + dy * cr;
            px[i] = ox + (cx - ox) * collapse;
            py[i] = oy + (cy - oy) * collapse;
        }

        DrawRadialGradient(cx, cy, cw * 0.50f, 0.0f, 0.0f, 0.012f,
                          0.52f * canvasReveal * (1.0f - collapse * 0.35f));
        for (int i = 0; i < 36; ++i) {
            if ((i % 4) == 2) continue;
            float a0 = (float)i / 36.0f * 6.2831853f + now * 0.05f;
            float a1 = a0 + 0.080f;
            float rx = cw * (0.36f + 0.04f * sinf(now * 0.4f));
            float ry = ch * 0.30f;
            float x0 = cx + cosf(a0) * rx;
            float y0 = cy + sinf(a0) * ry;
            float x1 = cx + cosf(a1) * rx;
            float y1 = cy + sinf(a1) * ry;
            LogoLine(x0, y0, x1, y1,
                     2.6f, 0.0f, 0.0f, 0.012f, 0.20f * canvasReveal * (1.0f - collapse));
            LogoLine(cx + cosf(a0) * rx, cy + sinf(a0) * ry,
                     cx + cosf(a1) * rx, cy + sinf(a1) * ry,
                     0.95f, 0.42f, 0.82f, 1.0f, 0.080f * canvasReveal * (1.0f - collapse));
        }
        for (int e = 0; e < 14; ++e) {
            int a = edges[shapeIdx][e][0], b = edges[shapeIdx][e][1];
            if (a < 0 || b < 0) break;
            float edgeA = (0.18f + 0.20f * bestHover + 0.38f * collapse) * canvasReveal;
            LogoLine(px[a], py[a], px[b], py[b],
                     4.0f + collapse * 3.4f, 0.0f, 0.0f, 0.012f, 0.42f * edgeA);
            LogoLine(px[a], py[a], px[b], py[b],
                     1.45f + collapse * 2.4f, 0.48f, 0.82f, 1.0f, edgeA);
        }
        for (int i = 0; i < 10; ++i) {
            float ns = ((i == 0 || i == 4 || i == 8) ? 5.2f : 3.5f) * (1.0f + collapse * 1.6f);
            drawNodeGlow(px[i], py[i], ns,
                         (0.48f + 0.22f * bestHover + 0.46f * collapse) * canvasReveal);
        }
        if (collapse > 0.01f)
            drawCircle(cx, cy, 18.0f + 52.0f * collapse, 0.48f, 0.82f, 1.0f, 0.080f * collapse * canvasReveal);
    }

    if (s_menuSelect >= 0 && s_menuExitT >= 0.52f && g_FadeDir == 0) {
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
                    drawRect(0, 0, sw, sh, 0.0f, 0.0f, 0.0f, 0.45f * ease * SCENE_BG_ALPHA);  // 배경 딤
                    // 시네마틱 — 아래로 훑는 스캔 스윕 라인 (화이트 플래시는 눈 아파서 제거)
                    {
                        float sweepY = fmodf(prog * 1.3f, 1.0f) * sh;
                        drawRect(0, sweepY, sw, 2.0f, ar, ag, ab, 0.30f * ease);
                        drawRect(0, sweepY - 40.0f, sw, 40.0f, ar, ag, ab, 0.05f * ease);
                    }
                    drawRect(wx, wy, cw, chh, 0.06f, 0.07f, 0.10f, 0.99f);   // 창 본체
                    if (ease > 0.9f) {
                        // 부팅 로그 — 진행도에 따라 한 줄씩 나타남
                        static const wchar_t* LOG[6] = {
                            L"> mounting modules ...",
                            L"> loading assets        [ OK ]",
                            L"> init renderer         [ OK ]",
                            L"> linking onedow.dll    [ OK ]",
                            L"> verify save data      [ OK ]",
                            L"> ready." };
                        int shown = (int)(prog * 6.5f); if (shown > 6) shown = 6;
                        for (int i = 0; i < shown; i++) {
                            bool last = (i == 5);
                            g_TextS.Draw(LOG[i], wx + 22.0f, wy + 14.0f + i * 24.0f, 0.6f,
                                         last ? ag : 0.65f, last ? 1.0f : 0.78f,
                                         last ? ag : 0.7f, 0.95f);
                        }
                        // 진행 바 (창 하단)
                        float barW = cw - 44.0f, barX = wx + 22.0f, barY = wy + chh - 30.0f;
                        BindMainShader();
                        drawRect(barX, barY, barW, 14.0f, 0.12f, 0.14f, 0.20f, 1.0f);
                        drawRect(barX, barY, barW * prog, 14.0f, ar, ag, ab, 1.0f);
                        wchar_t pct[16]; swprintf_s(pct, L"%d%%", (int)(prog * 100.0f));
                        g_TextS.Draw(pct, barX + barW - 44.0f, barY - 22.0f, 0.6f,
                                     0.8f, 0.9f, 1.0f, 1.0f);
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
    static float s_backHov = 0.0f;
    static int   s_selKey     = -1;
    static float s_scroll     = 0.0f;
    static float s_detailT    = 0.0f;
    static int   s_prevKey    = -2;
    static int   s_prevTab    = -1;
    static bool  s_backExit   = false;
    static float s_backOutT   = 0.0f;
    static bool  s_prevRmb    = false;
    static bool  s_prevEsc    = false;
    static int   s_prevTagKey = -2;
    static float s_tagFlickerT = 1.0f;

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
    g_ShopEntryT += dt;
    if (g_ShopEntryT > 1.0f) g_ShopEntryT = 1.0f;

    const bool rawRmb = c.window && glfwGetMouseButton(c.window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    const bool rmbClick = rawRmb && !s_prevRmb;
    s_prevRmb = rawRmb;
    const bool rawEsc = c.window && glfwGetKey(c.window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    const bool escClick = rawEsc && !s_prevEsc;
    s_prevEsc = rawEsc;

    const bool inputReady = !s_backExit && g_ShopEntryT >= 0.55f;
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
    if (rmbClick || escClick) beginBack();
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
    const float rightX = std::max(sw * 0.56f, panelX + panelW * 0.56f);
    const float rightW = (panelX + panelW) - rightX - 8.0f * uiS;
    const float rightAreaY = colY;
    const float rightAreaH = colH;
    const float tabH = 54.0f * uiS;
    const float tabY = colY;
    const float listAreaY = colY + 64.0f * uiS;
    const float listAreaH = colH - 64.0f * uiS;
    const float footY = panelY + panelH - footerH + 12.0f * uiS;
    (void)runRightW;
    (void)runRightX;
    const float leftVignetteA = s_backExit
        ? (0.74f + 0.26f * backP)
        : (s_MainMenuArmoryPanel ? 0.74f : 0.64f * wake);
    DrawSceneLeftVignette(sw, sh, leftVignetteA);
    if (s_MainMenuArmoryPanel)
        DrawSceneLeftVignette(sw, sh, 0.18f * wake);
    DrawSceneRadialVignette(rightX + rightW * 0.56f, rightAreaY + rightAreaH * 0.52f,
                            std::min(rightW, rightAreaH) * 0.62f, 0.46f * rightWake);

    const float mainBW = std::min(560.0f, std::max(420.0f, sw * 0.34f));
    const float mainBH = 70.0f;
    const float mainGap = 15.0f;
    const float mainTotalH = 5.0f * mainBH + 4.0f * mainGap;
    const float mainX = std::max(58.0f, sw * 0.075f);
    float mainY = sh * 0.48f;
    if (mainY + mainTotalH > sh - 54.0f) mainY = sh - mainTotalH - 54.0f;
    if (mainY < MainLogoTop(sh) + MainLogoHeight(sh) * 0.72f)
        mainY = MainLogoTop(sh) + MainLogoHeight(sh) * 0.72f;
    const float armoryY = mainY + (mainBH + mainGap);
    const float armoryMidY = armoryY + mainBH * 0.5f;

    const float rootX = mainX + (s_backExit ? backP * 180.0f * uiS : -(1.0f - wake) * 82.0f * uiS);
    const float rootY = mainY + (1.0f - wake) * 18.0f * uiS;
    const float rootW = std::min(340.0f * uiS, mainBW * 0.66f);
    const float rootH = mainBH;
    const float rootGap = mainGap;
    const float depth2X = rootX + std::min(345.0f * uiS, mainBW * 0.64f);
    const float depth2Y = std::max(panelY + headerH + 34.0f * uiS,
        rootY + (float)s_tab * (rootH + rootGap) - 34.0f * uiS);
    const float depth2W = std::max(300.0f * uiS,
        std::min(470.0f * uiS, rightX - depth2X - 32.0f * uiS));
    const float depth2H = std::max(280.0f * uiS,
        std::min(panelY + panelH - footerH - depth2Y - 18.0f * uiS, 620.0f * uiS));

    static constexpr int KEY_META  = 0;
    static constexpr int KEY_THEME = 100;
    static constexpr int KEY_AUG   = 1000;

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
        s_prevTab   = s_tab;
        s_itemCount = 0;
        s_scroll    = 0.0f;

        auto addGroup = [&](const wchar_t* lbl, float r, float g2, float b) {
            if (s_itemCount < 300) s_items[s_itemCount++] = { true, -1, lbl, r, g2, b };
        };
        auto addItem = [&](int key, const wchar_t* lbl, float r, float g2, float b) {
            if (s_itemCount < 300) s_items[s_itemCount++] = { false, key, lbl, r, g2, b };
        };

        if (s_tab == 0 || s_tab == 1) {
            addGroup(nli == 0 ? L"\xCF54\xC5B4 \xD574\xAE08" : L"CORE UNLOCKS", 0.38f, 0.82f, 1.00f);
            for (int i = 0; i < META_COUNT; i++)
                addItem(KEY_META + i, MetaName(i), 0.38f, 0.82f, 1.00f);
        }
        if (s_tab == 0 || s_tab == 2) {
            addGroup(nli == 0 ? L"\xC18C\xCD1D \xC2DC\xC2A4\xD15C" : L"RIFLE SYSTEMS", 0.72f, 0.92f, 0.42f);
            for (int i = 0; i < AUG_TOTAL; i++)
                if (!AugRemoved(ALL_AUGS[i].type) && augWeapCat(ALL_AUGS[i].type) == 0)
                    addItem(KEY_AUG + i, ALL_AUGS[i].locName[li], 0.72f, 0.92f, 0.42f);
        }
        if (s_tab == 0 || s_tab == 3) {
            addGroup(nli == 0 ? L"\xC704\xC131 \xBC30\xC5F4" : L"FIELD ARRAY", 0.62f, 0.48f, 1.00f);
            for (int i = 0; i < AUG_TOTAL; i++)
                if (!AugRemoved(ALL_AUGS[i].type) && augWeapCat(ALL_AUGS[i].type) == 1)
                    addItem(KEY_AUG + i, ALL_AUGS[i].locName[li], 0.62f, 0.48f, 1.00f);
        }

        s_selKey = -1;
        for (int i = 0; i < s_itemCount; i++)
            if (!s_items[i].isGroup) { s_selKey = s_items[i].key; break; }
    }

    if (s_selKey != s_prevKey) { s_prevKey = s_selKey; s_detailT = 0.0f; }
    if (s_selKey != s_prevTagKey) { s_prevTagKey = s_selKey; s_tagFlickerT = 0.0f; }
    s_detailT = UiApproach(s_detailT, 1.0f, dt, 6.0f);
    s_tagFlickerT = UiApproach(s_tagFlickerT, 1.0f, dt, 16.0f);

    // ── Main-menu expansion ghost ──────────────────────────────────────
    const float oldOut = 1.0f - oldMenuA;
    const float returnLogoA = s_backExit ? (0.42f + 0.58f * backP) : 0.42f;
    DrawMainOnedowLogo(sw, sh,
                       returnLogoA * oldMenuA + 0.08f * wake,
                       LogoClamp01(oldMenuA + 0.22f * wake),
                       sw * 0.30f - 160.0f * oldOut,
                       0.82f);
    {
        struct GhostDef { const wchar_t* route; const wchar_t* sub; };
        static const GhostDef kGhostMenu[5] = {
            { L"RUN_CONFIG",  L"\uC2DC\uC791" },
            { L"ARMORY",      L"\uC0C1\uC810" },
            { L"ASTRAL_LOG",  L"\uB3C4\uAC10" },
            { L"CALIBRATION", L"\uC124\uC815" },
            { L"SHUTDOWN",    L"\uAC8C\uC784 \uC885\uB8CC" },
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
        { L"\xC804\xCCB4", L"ALL"    },
        { L"\xD574\xAE08", L"UNLOCK" },
        { L"\xC18C\xCD1D", L"RIFLE"  },
        { L"\xC704\xC131", L"FIELD"  },
    };
    static const float kTR[SHOP_TAB_COUNT] = { 0.70f, 0.38f, 0.72f, 0.62f };
    static const float kTG[SHOP_TAB_COUNT] = { 0.70f, 0.82f, 0.92f, 0.48f };
    static const float kTB[SHOP_TAB_COUNT] = { 0.70f, 1.00f, 0.42f, 1.00f };

    BindMainShader();
    drawRect(rootX - 36.0f, rootY - 20.0f,
             1.2f, rootH * 5.0f + rootGap * 4.0f + 40.0f,
             0.48f, 0.82f, 1.00f, 0.16f * wake);

    const int ROOT_COUNT = SHOP_TAB_COUNT + 1;
    for (int i = 0; i < ROOT_COUNT; i++) {
        const bool isBack = (i == SHOP_TAB_COUNT);
        const float tx = rootX;
        const float ty = rootY + (float)i * (rootH + rootGap);
        const float hitX = tx - 26.0f;
        const float hitRight = std::min(tx + rootW + 18.0f * uiS, depth2X - 34.0f * uiS);
        const bool hov = inputReady
                       && (mx >= hitX && mx < hitRight
                       && my >= ty && my < ty + rootH);
        float& hovT = isBack ? s_backHov : s_tabHov[i];
        float rootSpd = dt * 10.0f; if (rootSpd > 1.0f) rootSpd = 1.0f;
        hovT += ((hov ? 1.0f : 0.0f) - hovT) * rootSpd;

        if (hov && lmb && !g_LmbPrev) {
            if (isBack) {
                beginBack();
            } else {
                s_tab = i;
                s_prevTab = -1;
            }
        }

        const bool sel = (!isBack && s_tab == i);
        const float rr = isBack ? 0.42f : kTR[i];
        const float gg = isBack ? 0.62f : kTG[i];
        const float bb = isBack ? 0.78f : kTB[i];
        const wchar_t* route = isBack ? L"BACK" : kTabLbl[i][1];
        const wchar_t* sub = isBack ? L"\uB4A4\uB85C" : kTabLbl[i][0];
        const float rawRootPh = s_backExit
            ? wake
            : LogoClamp01((g_ShopEntryT - 0.20f - (float)i * 0.09f) / 0.45f);
        const float reveal = Smoothstep(rawRootPh);
        const float rowA = reveal * wake;
        const float activeT = std::max(hovT, sel ? 1.0f : 0.0f);
        const float selectPulse = sel ? (0.16f + 0.16f * sinf(now * 4.0f)) : 0.0f;
        const float slide = (1.0f - reveal) * 34.0f;
        const float bx = tx + slide - 10.0f * hovT;
        const float by = ty;

        BindMainShader();
        if (activeT > 0.01f) {
            const float lineH = 20.0f + 42.0f * activeT + 10.0f * selectPulse;
            drawRect(bx - 18.0f, by + rootH * 0.5f - lineH * 0.5f,
                     2.2f, lineH, rr, gg, bb, (0.24f + 0.55f * activeT) * rowA);
            const float scanY = by + rootH * (0.34f + 0.32f * fmodf(now * 0.55f + (float)i * 0.17f, 1.0f));
            drawRect(bx - 4.0f, scanY, mainBW * (0.58f + 0.24f * activeT), 1.1f,
                     rr, gg, bb, (0.050f + 0.090f * activeT) * rowA);
            drawDiamond(bx - 17.0f, by + rootH * 0.5f,
                        3.0f + 2.6f * activeT, rr, gg, bb, (0.40f + 0.34f * activeT) * rowA);
        }

        float routeSc = 1.04f;
        while (routeSc > 0.82f && g_TextL.Width(route, routeSc) > mainBW)
            routeSc -= 0.04f;
        const float subSc = 0.48f;
        const float routeY = by + 2.0f;
        const float subY = routeY + g_TextL.Height(route, routeSc) - 3.0f;
        float tr = 1.0f + (rr - 1.0f) * activeT;
        float tg = 1.0f + (gg - 1.0f) * activeT;
        float tb = 1.0f + (bb - 1.0f) * activeT;
        if (sel) { tr = 1.0f; tg = 1.0f; tb = 1.0f; }
        g_TextL.Draw(route, bx, routeY, routeSc,
                     tr, tg, tb, (0.82f + 0.16f * activeT + 0.18f * selectPulse) * rowA);
        g_TextS.Draw(sub, bx + 4.0f, subY, subSc,
                     0.70f + 0.18f * activeT,
                     0.75f + 0.16f * activeT,
                     0.82f + 0.12f * activeT,
                     (0.70f + 0.20f * activeT) * rowA);
    }

    // ── Depth 2: item branches ─────────────────────────────────────────
    BindMainShader();
    const float parentY = rootY + (float)s_tab * (rootH + rootGap) + rootH * 0.5f;
    const float trunkX = depth2X - 26.0f * uiS;
    LogoLine(rootX + rootW + 18.0f * uiS, parentY, trunkX, parentY,
             1.0f * uiS, kTR[s_tab], kTG[s_tab], kTB[s_tab], 0.34f * wake);

    const float itemH  = 50.0f * uiS;
    const float grpH   = 40.0f * uiS;
    const float listX  = depth2X;
    const float listW2 = depth2W;

    float totalH = 0.0f;
    for (int i = 0; i < s_itemCount; i++)
        totalH += s_items[i].isGroup ? grpH : itemH;
    float maxScroll = std::max(0.0f, totalH - depth2H + 16.0f * uiS);

    bool overList = inputReady
                 && (mx >= depth2X - 38.0f * uiS && mx < depth2X + depth2W + 26.0f * uiS
                  && my >= depth2Y && my < depth2Y + depth2H);
    if (overList && g_ScrollAccum != 0.0f)
        s_scroll -= g_ScrollAccum * 36.0f * uiS;
    g_ScrollAccum = 0.0f;
    if (s_scroll < 0.0f) s_scroll = 0.0f;
    if (s_scroll > maxScroll) s_scroll = maxScroll;

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
             0.9f * uiS, kTR[s_tab], kTG[s_tab], kTB[s_tab], 0.22f * wake);
    for (int i = 0; i < s_itemCount; i++) {
        const SLItem& itm = s_items[i];
        float rh = itm.isGroup ? grpH : itemH;
        float ry = curY; curY += rh;
        if (ry + rh < depth2Y || ry > depth2Y + depth2H) continue;

        if (itm.isGroup) {
            BindMainShader();
            float mid = ry + grpH * 0.5f;
            float grpPulse = 0.72f + 0.28f * sinf(now * 2.2f + (float)i * 0.41f);
            LogoLine(trunkX, mid, listX - 8.0f * uiS, mid,
                     0.9f * uiS, itm.r, itm.g, itm.b, 0.28f * wake);
            drawRect(listX + 4.0f*uiS, mid - 0.5f*uiS,
                     listW2 * 0.96f, 1.0f*uiS,
                     itm.r, itm.g, itm.b, (0.11f + 0.030f * grpPulse) * wake);
            drawDiamond(listX - 8.0f*uiS, mid,
                        (3.8f + 0.6f * grpPulse) * uiS,
                        itm.r, itm.g, itm.b, 0.58f * wake);
            const float gsc = 0.58f * uiS;
            g_TextS.Draw(itm.label, listX + 14.0f*uiS,
                         ry + (grpH - g_TextS.Height(itm.label, gsc)) * 0.5f,
                         gsc, 0.86f, 0.94f, 1.0f, 0.74f * wake);
        } else {
            bool isSel = (s_selKey == itm.key);
            bool hov2  = inputReady
                       && (mx >= listX - 24.0f * uiS && mx < listX + listW2 + 18.0f * uiS
                       && my >= ry - 4.0f * uiS && my < ry + itemH + 5.0f * uiS);
            if (hov2 && lmb && !g_LmbPrev) s_selKey = itm.key;
            BindMainShader();
            const float cardY = ry + 2.0f * uiS;
            const float cardH = itemH - 4.0f * uiS;
            const float cardPulse = isSel ? (0.60f + 0.40f * sinf(now * 3.2f + (float)i * 0.31f)) : 0.0f;
            const float ndy = cardY + cardH * 0.5f;
            LogoLine(trunkX, ndy, listX - 12.0f * uiS, ndy,
                     0.8f * uiS, itm.r, itm.g, itm.b, (isSel ? 0.40f : 0.16f) * wake);
            if (isSel || hov2) {
                drawRect(listX + 24.0f * uiS, cardY + cardH * 0.5f,
                         listW2 - 26.0f * uiS, 1.1f * uiS,
                         itm.r, itm.g, itm.b, (isSel ? 0.15f + 0.040f * cardPulse : 0.070f) * wake);
            }
            if (isSel) {
                drawRect(listX - 2.0f * uiS, cardY + 7.0f * uiS,
                         2.0f * uiS, cardH - 14.0f * uiS,
                         itm.r, itm.g, itm.b, (0.72f + 0.16f * cardPulse) * wake);
                const float cursorSc = 0.66f * uiS;
                g_TextS.Draw(L">", listX + 7.0f * uiS,
                             cardY + (cardH - g_TextS.Height(L">", cursorSc)) * 0.5f,
                             cursorSc, itm.r, itm.g, itm.b, 0.96f * wake);
            } else if (hov2) {
                drawRect(listX - 1.0f * uiS, cardY + 10.0f * uiS,
                         1.4f * uiS, cardH - 20.0f * uiS,
                         itm.r, itm.g, itm.b, 0.34f * wake);
            }
            if (isSel) {
                drawDiamond(listX + listW2 - 13.0f * uiS, cardY + 13.0f * uiS,
                    (3.2f + 1.0f * cardPulse) * uiS,
                            1.0f, 1.0f, 1.0f, 0.44f * wake);
                drawDiamond(listX + listW2 - 13.0f * uiS, cardY + cardH - 13.0f * uiS,
                    (2.8f + 0.8f * cardPulse) * uiS,
                            1.0f, 1.0f, 1.0f, 0.30f * wake);
            }
            float ndx = listX - 12.0f*uiS;
            drawDiamond(ndx, ndy, (isSel ? 3.5f : 2.2f)*uiS, itm.r, itm.g, itm.b,
                        (isSel ? 0.66f : hov2 ? 0.34f : 0.16f) * wake);
            float tsc = 0.56f * uiS;
            g_TextS.Draw(itm.label, listX + (isSel ? 27.0f : 16.0f)*uiS,
                         cardY + (cardH - g_TextS.Height(itm.label, tsc)) * 0.5f,
                         tsc,
                         isSel ? 1.0f : hov2 ? 0.82f : 0.54f,
                         isSel ? 1.0f : hov2 ? 0.88f : 0.62f,
                         isSel ? 1.0f : hov2 ? 0.98f : 0.72f,
                         (isSel ? 1.0f : hov2 ? 0.78f : 0.48f) * wake);
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
        float da = std::max(0.0f, std::min(1.0f, s_detailT * rightWake));
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
        const float vizW = std::min(600.0f * uiS, rInW * 0.58f);
        const float vizH = std::min(500.0f * uiS, bodyH * 0.78f);
        const float vizX = rInX + rInW * 0.42f;
        const float vizY = bodyY + 18.0f * uiS;
        const float infoX = rInX;
        const float infoW = rInW - vizW - 32.0f * uiS;

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
            const float h = 42.0f * uiS;
            const float w = g_TextS.Width(creditBuf, sc) + 52.0f * uiS;
            const float x = rightX + rightW - rPad - w;
            const float y = rightAreaY + 16.0f * uiS;
            BindMainShader();
            drawRect(x, y + h * 0.50f, w, 1.0f * uiS,
                     1.00f, 0.84f, 0.20f, 0.18f * rightWake);
            drawConstellFrame(x, y, w, h, 1.00f, 0.84f, 0.20f,
                              0.38f * rightWake, 10.0f * uiS, 2.5f * uiS, 0.0f, 1.0f);
            drawDiamond(x + 18.0f * uiS, y + h * 0.5f,
                        4.2f * uiS, 1.00f, 0.84f, 0.20f, 0.90f * rightWake);
            g_TextS.Draw(creditBuf, x + 32.0f * uiS,
                         y + (h - g_TextS.Height(creditBuf, sc)) * 0.5f,
                         sc, 1.0f, 0.88f, 0.36f, 0.98f * rightWake);
        };

        auto drawDataBox = [&](float x, float y, float w, const wchar_t* label,
                               const wchar_t* value, float r, float g, float b, float a) {
            const float h = 62.0f * uiS;
            BindMainShader();
            drawRect(x, y, w, h, 0.028f, 0.036f, 0.056f, 0.10f * a);
            drawRect(x, y, 2.0f * uiS, h, r, g, b, 0.70f * a);
            drawRect(x + 10.0f * uiS, y + h - 1.5f * uiS, w - 20.0f * uiS, 1.5f * uiS,
                     r, g, b, 0.22f * a);
            g_TextS.Draw(label, x + 14.0f * uiS, y + 8.0f * uiS,
                         0.38f * uiS, 0.78f, 0.83f, 0.96f, 0.90f * a);
            g_TextS.Draw(value, x + 14.0f * uiS, y + 31.0f * uiS,
                         0.54f * uiS, 0.92f, 0.96f, 1.0f, 0.98f * a);
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
            g_TextS.Draw(code, x, y, 0.40f * uiS,
                         0.58f, 0.66f, 0.78f, 0.72f * a);
            float titleSc = 1.10f * uiS;
            while (titleSc > 0.82f * uiS && g_TextL.Width(title, titleSc) > w - 16.0f * uiS)
                titleSc -= 0.04f * uiS;
            g_TextL.Draw(title, x, y + 28.0f * uiS,
                         titleSc, 1.0f, 1.0f, 1.0f, 0.98f * a);
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
            g_TextS.Draw(label, x + 22.0f * uiS, y + 5.0f * uiS,
                         labelSc, 0.68f, 0.76f, 0.88f, 0.88f * a);
            g_TextS.Draw(value, valueX, y + 2.0f * uiS,
                         valueSc,
                         costValue ? 1.0f : 0.92f,
                         costValue ? 0.86f : 0.96f,
                         costValue ? 0.36f : 1.0f,
                         0.98f * a);
        };

        auto metaNodeCode = [&](int id) -> const wchar_t* {
            switch (id) {
            case META_HP:       return L"SYS_NODE : HP_UPGRADE";
            case META_DMG:      return L"SYS_NODE : DAMAGE_CORE";
            case META_MOVE:     return L"SYS_NODE : MOBILITY_DRIVE";
            case META_VISION:   return L"SYS_NODE : SIGHT_ARRAY";
            case META_STARTAUG: return L"SYS_NODE : START_MODULE";
            case META_ID_SLOT:  return L"SYS_NODE : IDENTITY_SOCKET";
            default:            return L"SYS_NODE : UNKNOWN";
            }
        };

        auto drawNodeMap = [&](int filled, int total, const wchar_t* label,
                               float r, float g, float b, float a) {
            BindMainShader();
            g_TextS.Draw(label, vizX + 18.0f * uiS, vizY + 16.0f * uiS,
                         0.42f * uiS, 0.78f, 0.83f, 0.96f, 0.56f * a);
            drawRect(vizX + 18.0f * uiS, vizY + 46.0f * uiS, vizW - 36.0f * uiS,
                     1.0f * uiS, r, g, b, 0.18f * a);

            int nodes = std::max(3, std::min(total, 8));
            float cx = vizX + vizW * 0.50f;
            float cy = vizY + vizH * 0.56f;
            float rad = std::min(vizW, vizH) * 0.34f;
            float px[8] = {}, py[8] = {};
            DrawSceneRadialVignette(cx, cy, rad * 2.10f, 0.42f * a);
            BindMainShader();
            for (int i = 0; i < nodes; ++i) {
                float ang = -1.5708f + (float)i / (float)nodes * 6.2831853f + now * 0.10f;
                px[i] = cx + cosf(ang) * rad;
                py[i] = cy + sinf(ang) * rad;
            }
            for (int i = 0; i < nodes; ++i) {
                int j = (i + 1) % nodes;
                bool on = (i < filled || j < filled);
                DrawVisibleConstellLine(px[i], py[i], px[j], py[j], 1.20f * uiS, r, g, b,
                                        (on ? 0.58f : 0.20f) * a);
                DrawVisibleConstellLine(cx, cy, px[i], py[i], 0.82f * uiS, r, g, b,
                                        (i < filled ? 0.30f : 0.10f) * a);
            }
            DrawVisibleConstellNode(cx, cy, 10.5f * uiS, r, g, b, 0.88f * a);
            for (int i = 0; i < nodes; ++i) {
                bool on = i < filled;
                bool next = i == filled && filled < total;
                float pulse = next ? 0.74f + 0.26f * sinf(now * 4.0f) : 1.0f;
                DrawVisibleConstellNode(px[i], py[i], (on ? 7.2f : 5.2f) * uiS,
                                        r, g, b, (on ? 0.92f : next ? 0.62f * pulse : 0.28f) * a);
            }
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
            g_TextS.Draw(label, x + (w - tw) * 0.5f,
                         y + (h - g_TextS.Height(label, sc)) * 0.5f,
                         sc,
                         enabled ? 0.92f : 0.64f,
                         enabled ? 0.98f : 0.70f,
                         enabled ? 1.00f : 0.78f,
                         0.96f * a);
            return enabled && hov && lmb && !g_LmbPrev;
        };

        drawPanelBase();
        drawCredits();

        auto drawMiniTagFrame = [&](float x, float y, float w, float h,
                                    float r, float g, float b, float a) {
            if (a <= 0.001f) return;
            const float l = 26.0f * uiS;
            const float t = 1.0f * uiS;
            BindMainShader();
            drawRect(x, y, l, t, r, g, b, 0.42f * a);
            drawRect(x, y, t, h, r, g, b, 0.34f * a);
            drawRect(x, y + h - t, l, t, r, g, b, 0.36f * a);
            drawRect(x + w - l, y + h - t, l, t, r, g, b, 0.26f * a);
        };

        auto drawDataTag = [&](const wchar_t* id, const wchar_t* title,
                               const wchar_t* desc, const wchar_t* stat,
                               const wchar_t* cmd, bool enabled,
                               float r, float g, float b, float a) -> bool {
            const float tagW = std::min(610.0f * uiS, rightW * 0.48f);
            const float tagH = 128.0f * uiS;
            const float tagX = vizX + 20.0f * uiS;
            const float tagY = vizY + vizH + 16.0f * uiS;
            const float flick = Smoothstep(s_tagFlickerT);
            const float tagA = a * (0.58f + 0.42f * flick);
            const float sc0 = 0.40f * uiS;
            const float sc1 = 0.48f * uiS;
            bool cmdHov = inputReady && enabled
                && mx >= tagX && mx < tagX + tagW
                && my >= tagY + 84.0f * uiS && my < tagY + tagH + 14.0f * uiS;

            drawMiniTagFrame(tagX, tagY, tagW, tagH, r, g, b, tagA);
            BindMainShader();
            if (s_tagFlickerT < 0.98f) {
                const float scanY = tagY + 10.0f * uiS + (tagH - 22.0f * uiS) * flick;
                drawRect(tagX + 8.0f * uiS, scanY, tagW - 18.0f * uiS, 1.0f * uiS,
                         r, g, b, 0.34f * a * (1.0f - flick));
            }
            drawDiamond(tagX + 10.0f * uiS, tagY + 17.0f * uiS,
                        2.8f * uiS, r, g, b, 0.70f * tagA);
            g_TextS.Draw(id, tagX + 24.0f * uiS, tagY + 6.0f * uiS,
                         sc0, 0.62f, 0.72f, 0.86f, 0.72f * tagA);
            g_TextS.Draw(title, tagX + 24.0f * uiS, tagY + 28.0f * uiS,
                         sc1, 0.92f, 0.96f, 1.0f, 0.94f * tagA);
            if (desc && desc[0]) {
                g_TextS.Draw(desc, tagX + 24.0f * uiS, tagY + 56.0f * uiS,
                             0.40f * uiS, 0.66f, 0.75f, 0.88f, 0.70f * tagA);
            }
            g_TextS.Draw(stat, tagX + 24.0f * uiS, tagY + 83.0f * uiS,
                         0.42f * uiS,
                         0.86f, 0.91f, 0.98f, 0.86f * tagA);
            if (cmd) {
                if (cmdHov) {
                    drawRect(tagX + 22.0f * uiS, tagY + 112.0f * uiS,
                             std::min(tagW - 44.0f * uiS, g_TextS.Width(cmd, 0.42f * uiS) + 24.0f * uiS),
                             1.2f * uiS, r, g, b, 0.54f * tagA);
                }
                g_TextS.Draw(cmd, tagX + 24.0f * uiS, tagY + 108.0f * uiS,
                             0.42f * uiS,
                             enabled ? (cmdHov ? 1.0f : 0.86f) : 0.48f,
                             enabled ? (cmdHov ? 1.0f : 0.92f) : 0.54f,
                             enabled ? 1.0f : 0.60f,
                             (enabled ? 0.92f : 0.48f) * tagA);
            }
            return cmdHov && lmb && !g_LmbPrev;
        };

        {
            int filled = 0, total = 5;
            const wchar_t* mapLabel = L"CONSTELLATION MAP";
            const wchar_t* id = L"ID: EMPTY";
            const wchar_t* title = L"UNASSIGNED NODE";
            const wchar_t* desc = L"DESC: SELECT TREE BRANCH";
            wchar_t statBuf[128]; swprintf_s(statBuf, L"STAT: [ - ]");
            const wchar_t* cmd = nullptr;
            bool cmdEnabled = false;

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
                    desc = nli == 0 ? L"DESC: \uC601\uAD6C \uD574\uAE08 \uB178\uB4DC" : L"DESC: PERMANENT UNLOCK NODE";
                    if (maxed)
                        swprintf_s(statBuf, L"STAT: [ LV %d/%d ] | [ COST: MAX ]", curLv, md.maxLv);
                    else
                        swprintf_s(statBuf, L"STAT: [ LV %d/%d ] | [ COST: %lld SD ]", curLv, md.maxLv, cost);
                    cmd = maxed ? L"CMD : [ MAXED ]" : L"CMD : [> INITIATE_UNLOCK ]";
                    cmdEnabled = !maxed && g_Coins >= cost;
                    if (drawDataTag(id, title, desc, statBuf, cmd, cmdEnabled, sr, sg, sb, da)) {
                        g_Coins -= cost;
                        g_MetaLv[mi]++;
                        SaveGame();
                    }
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
                    desc = L"DESC: VISUAL ACCENT PALETTE";
                    swprintf_s(statBuf, L"STAT: [ %ls ] | [ COST: %lld SD ]",
                               owned ? L"OWNED" : L"LOCKED", th.cost);
                    cmd = isCur ? L"CMD : [ EQUIPPED ]"
                        : owned ? L"CMD : [> EQUIP_SET ]" : L"CMD : [> PURCHASE_INITIATE ]";
                    cmdEnabled = owned || g_Coins >= th.cost;
                    if (drawDataTag(id, title, desc, statBuf, cmd, cmdEnabled, th.r, th.g, th.b, da)) {
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
                    mapLabel = L"ASTRAL PALETTE";
                }
            } else if (s_selKey >= KEY_AUG) {
                int ai = s_selKey - KEY_AUG;
                if (ai >= 0 && ai < AUG_TOTAL) {
                    const AugDef& ad = ALL_AUGS[ai];
                    GetRarityColor(ad.rarity, sr, sg, sb);
                    filled = 4; total = 6;
                    id = L"SYS_NODE : MODULE_CATALOG";
                    title = ad.locName[li];
                    desc = AugDesc(ad);
                    if (!desc || !desc[0]) desc = L"DESC: IN-RUN MODULE";
                    swprintf_s(statBuf, L"STAT: [ TYPE: MODULE ] | [ SOURCE: RUN ORBIT ]");
                    cmd = L"CMD : [ CHARTED_ONLY ]";
                    cmdEnabled = false;
                    drawDataTag(id, title, desc, statBuf, cmd, cmdEnabled, sr, sg, sb, da);
                    mapLabel = L"MODULE TRACE";
                }
            }

            drawNodeMap(filled, total, mapLabel, sr, sg, sb, s_selKey < 0 ? rightWake * 0.62f : da);
            drawAstralMeta(s_selKey, sr, sg, sb, rightWake * 0.70f);
        }

        if (false) {
        if (s_selKey < 0) {
            float a = rightWake;
            drawProductHeader(L"SYS_NODE : EMPTY", L10n(L"\uBE48 \uBCC4\uC790\uB9AC \uC9C0\uB3C4", L"EMPTY STAR FIELD"),
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
                drawProductHeader(L"SYS_NODE : MODULE_CATALOG", augName, cr2, cg2, cb2, da);

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
                drawNodeMap(4, 6, L10n(L"\uC131\uB3C4", L"CONSTELLATION MAP"), cr2, cg2, cb2, da);
            }
        }
        drawAstralMeta(s_selKey, sr, sg, sb, rightWake * 0.92f);
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
                 0.0f, 0.0f, 0.0f, 0.34f * rightWake);
        drawRect(rightX, listAreaY, rightW, listAreaH,
                 0.006f + sr * 0.006f, 0.010f + sg * 0.005f, 0.020f + sb * 0.005f,
                 0.985f * rightWake);
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

    const float uiS = std::max(0.70f, std::min(sw * 0.94f / 1640.0f, sh * 0.90f / 910.0f));
    const float mainBW = std::min(560.0f, std::max(420.0f, sw * 0.34f));
    const float mainBH = 70.0f;
    const float mainGap = 15.0f;
    const float mainTotalH = 5.0f * mainBH + 4.0f * mainGap;
    const float mainX = std::max(58.0f, sw * 0.075f);
    float mainY = sh * 0.48f;
    if (mainY + mainTotalH > sh - 54.0f) mainY = sh - mainTotalH - 54.0f;
    if (mainY < MainLogoTop(sh) + MainLogoHeight(sh) * 0.72f)
        mainY = MainLogoTop(sh) + MainLogoHeight(sh) * 0.72f;
    const float astralY = mainY + 2.0f * (mainBH + mainGap);
    const float astralMidY = astralY + mainBH * 0.5f;

    const float rootX = mainX + (s_backExit ? backP * 180.0f * uiS : -(1.0f - wake) * 82.0f * uiS);
    const float rootY = mainY;
    const float rootW = std::min(360.0f * uiS, mainBW * 0.70f);
    const float rootH = mainBH;
    const float rootGap = mainGap;
    const float depth2X = rootX + std::min(365.0f * uiS, mainBW * 0.68f);
    const float depth2Y = std::max(110.0f * uiS, rootY + (float)s_cat * (rootH + rootGap) - 28.0f * uiS);
    const float depth2W = std::max(330.0f * uiS, std::min(500.0f * uiS, sw * 0.50f - depth2X));
    const float depth2H = std::min(620.0f * uiS, sh - depth2Y - 98.0f * uiS);
    const float rightX = std::max(sw * 0.55f, depth2X + depth2W + 52.0f * uiS);
    const float rightW = sw - rightX - 72.0f * uiS;
    const float rightY = MainLogoTop(sh) + MainLogoHeight(sh) * 0.72f + 30.0f * uiS;
    const float rightH = sh - rightY - 92.0f * uiS;

    const float leftVignetteA = s_backExit ? (0.76f + 0.22f * backP) : 0.76f;
    DrawSceneLeftVignette(sw, sh, leftVignetteA);
    DrawSceneRadialVignette(rightX + rightW * 0.52f, rightY + rightH * 0.48f,
                            std::min(rightW, rightH) * 0.70f, 0.48f * rightWake);

    struct RootDef { const wchar_t* route; const wchar_t* sub; float r, g, b; };
    static const RootDef ROOTS[CAT_COUNT + 1] = {
        { L"ENTITIES", L"\uAD00\uCE21\uCCB4", 0.48f, 0.82f, 1.00f },
        { L"MODULES",  L"\uBAA8\uB4C8",       0.62f, 0.52f, 1.00f },
        { L"APEX",     L"\uC815\uC810",       0.96f, 0.76f, 0.30f },
        { L"BACK",     L"\uB4A4\uB85C",       0.42f, 0.62f, 0.78f },
    };

    // Main-menu expansion ghost.
    const float oldOut = 1.0f - oldMenuA;
    const float returnLogoA = s_backExit ? (0.42f + 0.58f * backP) : 0.42f;
    DrawMainOnedowLogo(sw, sh,
                       returnLogoA * oldMenuA + 0.08f * wake,
                       LogoClamp01(oldMenuA + 0.22f * wake),
                       sw * 0.30f - 160.0f * oldOut,
                       0.82f);
    {
        struct GhostDef { const wchar_t* route; const wchar_t* sub; };
        static const GhostDef kGhostMenu[5] = {
            { L"RUN_CONFIG",  L"\uC2DC\uC791" },
            { L"ARMORY",      L"\uC0C1\uC810" },
            { L"ASTRAL_LOG",  L"\uB3C4\uAC10" },
            { L"CALIBRATION", L"\uC124\uC815" },
            { L"SHUTDOWN",    L"\uAC8C\uC784 \uC885\uB8CC" },
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
    }

    // Depth 1 root.
    BindMainShader();
    drawRect(rootX - 36.0f, rootY - 20.0f,
             1.2f, rootH * 4.0f + rootGap * 3.0f + 40.0f,
             0.48f, 0.82f, 1.00f, 0.13f * wake);
    for (int i = 0; i < CAT_COUNT + 1; ++i) {
        const bool isBack = (i == CAT_COUNT);
        const float tx = rootX;
        const float ty = rootY + (float)i * (rootH + rootGap);
        const float hitX = tx - 26.0f;
        const float hitRight = std::min(tx + rootW + 18.0f * uiS, depth2X - 34.0f * uiS);
        const bool hov = inputReady
            && (mx >= hitX && mx < hitRight && my >= ty && my < ty + rootH);
        float spd = dt * 10.0f; if (spd > 1.0f) spd = 1.0f;
        s_rootHover[i] += ((hov ? 1.0f : 0.0f) - s_rootHover[i]) * spd;

        if (hov && lmb && !g_LmbPrev) {
            if (isBack) beginBack();
            else if (s_cat != i) s_cat = i;
        }

        const bool sel = (!isBack && s_cat == i);
        const RootDef& rd = ROOTS[i];
        const float rawRootPh = s_backExit
            ? wake
            : LogoClamp01((g_CodexEntryT - 0.20f - (float)i * 0.09f) / 0.45f);
        const float reveal = Smoothstep(rawRootPh);
        const float rowA = reveal * wake;
        const float activeT = std::max(s_rootHover[i], sel ? 1.0f : 0.0f);
        const float selectPulse = sel ? (0.12f + 0.12f * sinf(now * 3.3f)) : 0.0f;
        const float bx = tx + (1.0f - reveal) * 34.0f - 10.0f * s_rootHover[i];
        const float by = ty;

        BindMainShader();
        if (activeT > 0.01f) {
            const float lineH = 20.0f + 42.0f * activeT + 8.0f * selectPulse;
            drawRect(bx - 18.0f, by + rootH * 0.5f - lineH * 0.5f,
                     2.0f, lineH, rd.r, rd.g, rd.b, (0.20f + 0.48f * activeT) * rowA);
            drawRect(bx - 4.0f, by + rootH * 0.50f,
                     rootW * (0.46f + 0.16f * activeT), 1.0f * uiS,
                     rd.r, rd.g, rd.b, (0.040f + 0.055f * activeT) * rowA);
            drawDiamond(bx - 17.0f, by + rootH * 0.5f,
                        3.0f + 2.2f * activeT, rd.r, rd.g, rd.b,
                        (0.34f + 0.30f * activeT) * rowA);
        }

        float routeSc = 1.02f;
        while (routeSc > 0.80f && g_TextL.Width(rd.route, routeSc) > rootW)
            routeSc -= 0.04f;
        const float routeY = by + 2.0f;
        const float subY = routeY + g_TextL.Height(rd.route, routeSc) - 3.0f;
        float tr = 1.0f + (rd.r - 1.0f) * activeT;
        float tg = 1.0f + (rd.g - 1.0f) * activeT;
        float tb = 1.0f + (rd.b - 1.0f) * activeT;
        if (sel) { tr = 1.0f; tg = 1.0f; tb = 1.0f; }
        g_TextL.Draw(rd.route, bx, routeY, routeSc, tr, tg, tb,
                     (0.80f + 0.15f * activeT + selectPulse) * rowA);
        g_TextS.Draw(rd.sub, bx + 4.0f, subY, 0.48f,
                     0.70f + 0.16f * activeT,
                     0.75f + 0.14f * activeT,
                     0.82f + 0.10f * activeT,
                     (0.68f + 0.18f * activeT) * rowA);
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
    auto addGroup = [&](const wchar_t* label, float r, float g, float b) {
        if (itemCount < 512) items[itemCount++] = { true, -1, label, true, r, g, b };
    };
    auto addItem = [&](int key, const wchar_t* label, bool seen, float r, float g, float b) {
        if (itemCount < 512) items[itemCount++] = { false, key, label, seen, r, g, b };
    };

    if (s_cat == 0) {
        addGroup(L"SIGNAL CLASS", 0.48f, 0.82f, 1.00f);
        for (int i = 0; i < CM_COUNT; ++i)
            addItem(i, CodexMobSeen(i) ? MobName(i) : L"???", CodexMobSeen(i),
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
                    CodexAugSeen(i), rr, rg, rb);
        }
    } else {
        addGroup(L"APEX SIGNAL", 0.96f, 0.76f, 0.30f);
        for (int i = 0; i < BOSS_CODEX_COUNT; ++i)
            addItem(i, BossCodexSeen(i) ? BossCodexName(i) : L"???",
                    BossCodexSeen(i), 0.96f, 0.76f, 0.30f);
    }

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

    // Depth 2 list.
    BindMainShader();
    const RootDef& curRoot = ROOTS[s_cat];
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
    const float maxScroll = std::max(0.0f, totalH - depth2H + 16.0f * uiS);
    const bool overList = inputReady
        && (mx >= depth2X - 36.0f * uiS && mx < depth2X + depth2W + 22.0f * uiS
         && my >= depth2Y && my < depth2Y + depth2H);
    if (overList && g_ScrollAccum != 0.0f)
        scroll -= g_ScrollAccum * 36.0f * uiS;
    g_ScrollAccum = 0.0f;
    scroll = std::max(0.0f, std::min(scroll, maxScroll));

    BatchFlush();
    glEnable(GL_SCISSOR_TEST);
    {
        int scX = (int)(depth2X - 42.0f * uiS);
        int scY = (int)(sh - (depth2Y + depth2H));
        int scW = (int)(depth2W + 70.0f * uiS);
        int scH = (int)depth2H;
        if (scX < 0) scX = 0; if (scY < 0) scY = 0;
        glScissor(scX, scY, scW, scH);
    }
    float curY = depth2Y + 6.0f * uiS - scroll;
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
        if (ry + rh < depth2Y || ry > depth2Y + depth2H) continue;

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
            g_TextS.Draw(itm.label, depth2X + 14.0f * uiS,
                         ry + (grpH - g_TextS.Height(itm.label, 0.54f * uiS)) * 0.5f,
                         0.54f * uiS, 0.80f, 0.90f, 1.0f, 0.64f * wake);
        } else {
            const bool isSel = (s_sel[s_cat] == itm.key);
            const bool hov = inputReady
                && (mx >= depth2X - 24.0f * uiS && mx < depth2X + depth2W + 18.0f * uiS
                 && my >= ry - 4.0f * uiS && my < ry + itemH + 5.0f * uiS);
            if (hov && lmb && !g_LmbPrev)
                s_sel[s_cat] = itm.key;

            const float cardY = ry + 2.0f * uiS;
            const float cardH = itemH - 4.0f * uiS;
            const float ndy = cardY + cardH * 0.5f;
            BindMainShader();
            LogoLine(trunkX, ndy, depth2X - 12.0f * uiS, ndy,
                     0.68f * uiS, itm.r, itm.g, itm.b, (isSel ? 0.30f : 0.10f) * wake);
            if (isSel || hov) {
                drawRect(depth2X + 24.0f * uiS, cardY + cardH * 0.5f,
                         depth2W - 26.0f * uiS, 1.0f * uiS,
                         itm.r, itm.g, itm.b, (isSel ? 0.095f : 0.050f) * wake);
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
                        (isSel ? 3.3f : 2.0f) * uiS,
                        itm.r, itm.g, itm.b, (isSel ? 0.58f : hov ? 0.32f : 0.14f) * wake);
            float tsc = 0.54f * uiS;
            while (tsc > 0.43f * uiS && g_TextS.Width(itm.label, tsc) > depth2W - 54.0f * uiS)
                tsc -= 0.025f * uiS;
            g_TextS.Draw(itm.label, depth2X + (isSel ? 27.0f : 16.0f) * uiS,
                         cardY + (cardH - g_TextS.Height(itm.label, tsc)) * 0.5f,
                         tsc,
                         isSel ? 1.0f : hov ? 0.84f : itm.seen ? 0.60f : 0.42f,
                         isSel ? 1.0f : hov ? 0.90f : itm.seen ? 0.68f : 0.46f,
                         isSel ? 1.0f : hov ? 1.00f : itm.seen ? 0.78f : 0.52f,
                         (isSel ? 0.96f : hov ? 0.78f : 0.50f) * wake);
        }
    }
    BatchFlush();
    glDisable(GL_SCISSOR_TEST);

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
            g_TextS.Draw(t.c_str(), x, y, sc, r, g, b, a * (0.72f + 0.28f * s_decryptT));
        } else {
            g_TextS.Draw(text, x, y, sc, r, g, b, a);
        }
    };
    auto drawScanTextL = [&](const wchar_t* text, float x, float y, float sc,
                             float r, float g, float b, float a, int seed) {
        if (s_decryptT < 0.36f) {
            std::wstring t = scramble(text, seed);
            g_TextL.Draw(t.c_str(), x, y, sc, r, g, b, a * (0.72f + 0.28f * s_decryptT));
        } else {
            g_TextL.Draw(text, x, y, sc, r, g, b, a);
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
        if (!selectedSeen()) return nli == 0 ? L"\uBBF8\uBC1C\uACAC \uAE30\uB85D" : L"UNDISCOVERED SIGNAL";
        if (s_cat == 0) return MobDesc(key);
        if (s_cat == 1) return AugDesc(ALL_AUGS[key]);
        return BossCodexDesc(key);
    };

    // Depth 3 constellation canvas.
    const int selKey = s_sel[s_cat] < 0 ? 0 : s_sel[s_cat];
    const bool seen = selectedSeen();
    float cr = curRoot.r, cg = curRoot.g, cb = curRoot.b;
    if (s_cat == 1 && selKey >= 0 && selKey < AUG_TOTAL)
        GetRarityColor(ALL_AUGS[selKey].rarity, cr, cg, cb);

    BindMainShader();
    const float cx = rightX + rightW * 0.50f;
    const float cy = rightY + rightH * 0.45f;
    const float rad = std::min(rightW, rightH) * (s_cat == 2 ? 0.34f : 0.30f);
    const float canvasA = rightWake * (seen ? 1.0f : 0.55f);
    DrawSceneRadialVignette(cx, cy, rad * 2.15f, 0.46f * rightWake);
    BindMainShader();

    if (s_cat == 0) {
        const int n = 7;
        float px[8], py[8];
        for (int i = 0; i < n; ++i) {
            float a = now * 0.10f + (float)i / (float)n * 6.2831853f;
            float jitter = 0.78f + 0.20f * sinf(now * 2.0f + (float)(i + selKey) * 1.17f);
            px[i] = cx + cosf(a) * rad * jitter;
            py[i] = cy + sinf(a) * rad * (0.66f + 0.10f * cosf((float)i * 0.9f));
        }
        for (int i = 0; i < n; ++i) {
            int j = (i + 2 + (selKey % 2)) % n;
            DrawVisibleConstellLine(px[i], py[i], px[j], py[j],
                                    1.0f * uiS, cr, cg, cb, 0.20f * canvasA);
        }
        for (int i = 0; i < n; ++i)
            DrawVisibleConstellNode(px[i], py[i], (i == 0 ? 5.2f : 3.4f) * uiS,
                                    cr, cg, cb, (i == 0 ? 0.66f : 0.42f) * canvasA);
    } else if (s_cat == 1) {
        const int n = 6;
        float px[6], py[6];
        for (int i = 0; i < n; ++i) {
            float a = now * 0.06f + (float)i / (float)n * 6.2831853f;
            px[i] = cx + cosf(a) * rad;
            py[i] = cy + sinf(a) * rad;
        }
        for (int i = 0; i < n; ++i) {
            DrawVisibleConstellLine(px[i], py[i], px[(i + 1) % n], py[(i + 1) % n],
                                    1.1f * uiS, cr, cg, cb, 0.28f * canvasA);
            if ((i % 2) == 0)
                DrawVisibleConstellLine(cx, cy, px[i], py[i],
                                        0.82f * uiS, cr, cg, cb, 0.16f * canvasA);
        }
        for (int i = 0; i < 3; ++i) {
            float a = -now * 0.16f + (float)i / 3.0f * 6.2831853f;
            DrawVisibleConstellNode(cx + cosf(a) * rad * 0.42f,
                                    cy + sinf(a) * rad * 0.42f,
                                    3.4f * uiS, cr, cg, cb, 0.46f * canvasA);
        }
        DrawVisibleConstellNode(cx, cy, 7.0f * uiS, cr, cg, cb, 0.72f * canvasA);
        for (int i = 0; i < n; ++i)
            DrawVisibleConstellNode(px[i], py[i], 4.0f * uiS, cr, cg, cb, 0.56f * canvasA);
    } else {
        const int n = 8;
        for (int r = 0; r < 3; ++r) {
            float rr = rad * (0.54f + 0.20f * r);
            for (int i = 0; i < n; ++i) {
                if ((i + r) % 3 == 1) continue;
                float a0 = now * (0.025f + 0.010f * r) + (float)i / (float)n * 6.2831853f;
                float a1 = a0 + 0.18f;
                DrawVisibleConstellLine(cx + cosf(a0) * rr, cy + sinf(a0) * rr * 0.72f,
                                        cx + cosf(a1) * rr, cy + sinf(a1) * rr * 0.72f,
                                        1.2f * uiS, cr, cg, cb, 0.25f * canvasA);
            }
        }
        DrawVisibleConstellNode(cx, cy, 12.0f * uiS, cr, cg, cb, 0.82f * canvasA);
        for (int i = 0; i < 5; ++i) {
            float a = now * 0.04f + (float)i / 5.0f * 6.2831853f;
            DrawVisibleConstellNode(cx + cosf(a) * rad * 0.84f,
                                    cy + sinf(a) * rad * 0.58f,
                                    5.2f * uiS, cr, cg, cb, 0.48f * canvasA);
        }
    }

    // Compact read-only data tag.
    const float tagW = std::min(640.0f * uiS, rightW * 0.58f);
    const float tagH = 132.0f * uiS;
    const float tagX = cx - tagW * 0.32f;
    const float tagY = cy + rad * 0.88f;
    BindMainShader();
    drawRect(tagX, tagY, tagW, 1.1f * uiS, cr, cg, cb, 0.28f * rightWake);
    drawRect(tagX, tagY, 1.1f * uiS, tagH, cr, cg, cb, 0.22f * rightWake);
    drawRect(tagX, tagY + tagH, tagW * 0.42f, 1.1f * uiS, cr, cg, cb, 0.18f * rightWake);
    if (s_decryptT < 0.96f) {
        float scanY = tagY + 8.0f * uiS + (tagH - 18.0f * uiS) * Smoothstep(s_decryptT);
        drawRect(tagX + 10.0f * uiS, scanY, tagW - 18.0f * uiS, 1.0f * uiS,
                 cr, cg, cb, 0.22f * rightWake * (1.0f - Smoothstep(s_decryptT)));
    }

    wchar_t idBuf[96];
    if (s_cat == 0) swprintf_s(idBuf, L"ID : ENTITY_%02d", selKey);
    else if (s_cat == 1) swprintf_s(idBuf, L"ID : MODULE_%03d", selKey);
    else swprintf_s(idBuf, L"ID : APEX_%02d", selKey);
    wchar_t statBuf[128];
    if (s_cat == 0)
        swprintf_s(statBuf, L"CLASS : HOSTILE_SIGNAL | STATUS : %ls", seen ? L"CATALOGUED" : L"NO_DATA");
    else if (s_cat == 1)
        swprintf_s(statBuf, L"CLASS : MODULE_ARCHIVE | STATUS : %ls", seen ? L"ACQUIRED" : L"NO_DATA");
    else
        swprintf_s(statBuf, L"CLASS : APEX_ENTITY | STATUS : %ls", seen ? L"OBSERVED" : L"NO_DATA");

    drawScanTextS(idBuf, tagX + 24.0f * uiS, tagY + 9.0f * uiS,
                  0.42f * uiS, 0.62f, 0.72f, 0.86f, 0.72f * rightWake, selKey + s_cat * 31);
    drawScanTextL(selectedTitle(), tagX + 24.0f * uiS, tagY + 34.0f * uiS,
                  0.62f * uiS, 0.94f, 0.98f, 1.0f, 0.96f * rightWake, selKey + 7);
    drawScanTextS(selectedDesc(), tagX + 24.0f * uiS, tagY + 66.0f * uiS,
                  0.38f * uiS, 0.70f, 0.78f, 0.90f, 0.70f * rightWake, selKey + 13);
    drawScanTextS(statBuf, tagX + 24.0f * uiS, tagY + 92.0f * uiS,
                  0.40f * uiS, 0.84f, 0.90f, 0.98f, 0.86f * rightWake, selKey + 19);
    g_TextS.Draw(L"[ ARCHIVE_READ_ONLY ]", tagX + 24.0f * uiS, tagY + 114.0f * uiS,
                 0.40f * uiS, cr, cg, cb, 0.72f * rightWake);

    if (finishBackAfterRender) {
        s_backExit = false;
        s_backOutT = 0.0f;
        s_CodexBackRequested = false;
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
    DrawSceneLeftVignette(sw, sh, 0.64f * wake);
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
            { L"CLASS-I",
              { CM_NORMAL, CM_SPLITTER, CM_BLINKER, CM_CHARGER,
                CM_WEAVER, CM_BRUTE, CM_ORBITER, CM_SPAWNER, CM_SHIELDED },
              9, 0.48f, 0.82f, 1.00f },
            { L"CLASS-II",
              { CM_RANGED, CM_BOMBER, CM_DDOS, CM_BADSECTOR, CM_REGERROR, 0, 0, 0, 0 },
              5, 0.62f, 0.80f, 0.96f },
        };
        for (int gi = 0; gi < 2; ++gi) {
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
        SyncPlayerWindowAfterLoadout();
    }
    float trialHpMul = TrialPlayerMaxHpMult();
    if (trialHpMul < 0.999f)
        g_Stats.maxHP *= trialHpMul;
    g_GameManager.maxHP    = g_Stats.maxHP;
    g_GameManager.playerHP = g_Stats.maxHP;
    g_PrevHP               = g_Stats.maxHP;
    int startAugs = g_MetaStartAugs + ((g_CreativeMode) ? g_CreativeStartAugs : 0);
    if (startAugs > 0) {
        g_BossRewardPicksLeft = startAugs;
        g_GameManager.PickAugChoices(g_Stats.sizeAugTaken,
                                     g_Stats.distAugTaken, g_CreativeMode);
        g_GameManager.currentState = GameState::AUG_SELECT;
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
                SceneFlowWindow(sw, sh, FW, FH, L"career.exe", 0.55f, 0.7f, 1.0f, fx, fy, fcy, 0.0f);
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
                        ? GameState::CREATIVE_CONFIG : GameState::RUN_CONFIG;
                }
}

void Scene_RunConfig(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    (void)c.window;
    const std::function<void()>& ResetForNewGame = c.reset;

    if (!g_TrialPoolReady) RerollTrialPool();

    float dt = c.delta;
    if (dt > 0.05f) dt = 0.05f;
    g_RunConfigEntryT += dt;
    if (g_RunConfigEntryT > 1.0f) g_RunConfigEntryT = 1.0f;
    const float dimWake = Smoothstep(std::min(1.0f, g_RunConfigEntryT / 0.42f));
    const float wake = Smoothstep(std::min(1.0f, (g_RunConfigEntryT - 0.08f) / 0.50f));
    const float now = (float)glfwGetTime();

    DrawMenuBackground(sw, sh, c.delta, dimWake);

    { float ua = 1.0f - g_FadeAlpha; if (ua < 0.0f) ua = 0.0f; g_BatchAlpha = ua; }

    const float TARGET_W = 1640.0f;
    const float TARGET_H = 910.0f;
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
    const float rightW = panelW - leftW - midW - gap * 2.0f;
    const float leftX = panelX;
    const float midX = leftX + leftW + gap;
    const float rightX = midX + midW + gap;
    const float footY = panelY + panelH - footerH + 12.0f * uiS;
    DrawSceneLeftVignette(sw, sh, 0.34f * wake);
    DrawSceneRadialVignette(midX + (panelW - leftW - gap) * 0.50f, colY + colH * 0.54f,
                            std::min(panelW - leftW, colH) * 0.42f, 0.36f * wake);

    int li = LangIndex();
    int nli = (li == 0) ? 0 : 1;

    auto hit = [&](float x, float y, float w, float h) {
        return mx >= x && mx <= x + w && my >= y && my <= y + h;
    };

    auto drawBorder = [](float x, float y, float w, float h,
                         float r, float g, float b, float a, float thick) {
        drawRect(x, y, w, thick, r, g, b, a);
        drawRect(x, y + h - thick, w, thick, r, g, b, a);
        drawRect(x, y, thick, h, r, g, b, a);
        drawRect(x + w - thick, y, thick, h, r, g, b, a);
    };

    auto drawFitS = [&](const wchar_t* text, float x, float y, float maxW,
                        float sc, float minSc, float r, float g, float b, float a) {
        while (sc > minSc && g_TextS.Width(text, sc) > maxW)
            sc -= 0.025f * uiS;
        g_TextS.Draw(text, x, y, sc, r, g, b, a);
    };

    auto drawCenterS = [&](const wchar_t* text, float x, float y, float w,
                           float sc, float r, float g, float b, float a) {
        while (sc > 0.42f * uiS && g_TextS.Width(text, sc) > w - 16.0f * uiS)
            sc -= 0.025f * uiS;
        float tw = g_TextS.Width(text, sc);
        g_TextS.Draw(text, x + (w - tw) * 0.5f, y, sc, r, g, b, a);
    };

    auto drawCenterL = [&](const wchar_t* text, float x, float y, float w,
                           float sc, float r, float g, float b, float a) {
        while (sc > 0.48f * uiS && g_TextL.Width(text, sc) > w - 20.0f * uiS)
            sc -= 0.035f * uiS;
        float tw = g_TextL.Width(text, sc);
        g_TextL.Draw(text, x + (w - tw) * 0.5f, y, sc, r, g, b, a);
    };

    auto drawWindow = [&](float x, float y, float w, float h, const wchar_t* title,
                          float r, float g, float b, float focus, bool current) {
        BindMainShader();
        float bodyA = 0.62f + 0.28f * focus;
        drawRect(x + 8.0f * uiS, y + 10.0f * uiS, w, h, 0.0f, 0.0f, 0.0f, 0.20f + 0.10f * focus);
        drawRect(x, y, w, h,
                 0.025f + r * 0.016f * focus,
                 0.032f + g * 0.014f * focus,
                 0.052f + b * 0.014f * focus, bodyA);
        drawRect(x, y, w, 30.0f * uiS,
                 r * (0.20f + 0.38f * focus),
                 g * (0.20f + 0.38f * focus),
                 b * (0.22f + 0.38f * focus), 0.94f);
        drawRect(x, y + 30.0f * uiS, w, 2.0f * uiS, r, g, b, 0.22f + 0.55f * focus);
        drawBorder(x, y, w, h, r, g, b, 0.15f + 0.55f * focus, 1.5f * uiS);
        float bs = 10.0f * uiS;
        float by = y + 10.0f * uiS;
        float bx = x + w - 18.0f * uiS;
        drawRect(bx - 2.0f * (bs + 7.0f * uiS), by, bs, bs, 1, 1, 1, 0.12f + 0.15f * focus);
        drawRect(bx - (bs + 7.0f * uiS), by, bs, bs, 1, 1, 1, 0.12f + 0.15f * focus);
        drawRect(bx, by, bs, bs, 0.9f, 0.25f, 0.25f, 0.22f + 0.45f * focus);
        g_TextS.Draw(title, x + 12.0f * uiS, y + 7.0f * uiS, 0.56f * uiS,
                     0.72f + 0.28f * focus, 0.82f + 0.16f * focus, 1.0f, 0.52f + 0.46f * focus);
        if (current && focus > 0.25f) {
            float sweepY = y + 35.0f * uiS + fmodf(now * 95.0f, h - 48.0f * uiS);
            drawRect(x + 1.5f * uiS, sweepY, w - 3.0f * uiS, 1.0f * uiS, r, g, b, 0.045f * focus);
        }
    };

    auto drawDisabledVeil = [&](float x, float y, float w, float h,
                                const wchar_t* msg, float a) {
        BindMainShader();
        drawRect(x, y + 30.0f * uiS, w, h - 30.0f * uiS, 0.0f, 0.0f, 0.0f, a);
        drawCenterS(msg, x, y + h * 0.50f - 9.0f * uiS, w,
                    0.60f * uiS, 0.48f, 0.58f, 0.72f, 0.70f);
    };

    struct WCardDef {
        int job;
        const wchar_t* name[2];
        const wchar_t* desc[2];
        long long cost;
    };
    static const WCardDef kWC[] = {
        { JOB_NONE,      { L"소총",     L"RIFLE"        },
          { L"표준 연사 / 균형 범용",          L"Standard auto fire / balanced"   }, 0 },
        { JOB_VAMPIRE,   { L"정전기장", L"STATIC FIELD" },
          { L"범위 전기장 지속 피해",          L"Area electric field / sustained" }, 0 },
    };
    static const int kWCCount = 2;
    static const wchar_t* kWeaponStatLabels[6][2] = {
        { L"피해량",   L"DAMAGE"    },
        { L"공격속도", L"FIRE RATE" },
        { L"연사간격", L"INTERVAL"  },
        { L"탄속",     L"SPEED"     },
        { L"흔들림",   L"SPREAD"    },
        { L"사거리",   L"RANGE"     },
    };
    struct WDetailDef {
        const wchar_t* profile[2];
        const wchar_t* notes[2][3];
        const wchar_t* stat[2][6];
        float r, g, b;
        float norm[6]; // 레이더 축 [0-1]: 피해량, 공격속도, 연사간격, 탄속, 정밀도, 사거리
    };
    static const WDetailDef kWeaponDetail[] = {
        {   // 소총 — 균형형, 전 축 중상위
            { L"안정적인 표준 화력", L"Stable standard fire" },
            {
                { L"기본 조작과 성장 효율이 가장 안정적",
                  L"거리 유지와 보스전 대응이 무난함",
                  L"큰 약점이 적은 입문 기준 무기" },
                { L"Most stable handling and growth curve",
                  L"Reliable range control and boss response",
                  L"Baseline weapon with few sharp weaknesses" }
            },
            {
                { L"50", L"5.00/s", L"0.20s", L"1200", L"0.04 rad", L"표준" },
                { L"50", L"5.00/s", L"0.20s", L"1200", L"0.04 rad", L"Standard" }
            },
            0.35f, 0.72f, 1.00f,
            { 0.50f, 0.78f, 0.78f, 0.72f, 0.68f, 0.72f }
        },
        {   // 정전기장 — 속도·연사 극대, 피해·사거리 극소
            { L"탄환 없이 전장을 전기로 지배한다", L"Control the field with no ammunition" },
            {
                { L"플레이어 주변 반경에 전기장을 지속 전개",
                  L"범위 내 모든 적에게 틱 피해 — 탄약 없음",
                  L"원거리 적에게 무력, 근접 통제에 특화" },
                { L"Sustains an electric field around the player",
                  L"Tick damage to all enemies in range — no ammo",
                  L"Ineffective at range, dominates close control" }
            },
            {
                { L"18/틱", L"연속", L"0.08s", L"즉시", L"-", L"180 px" },
                { L"18/tick", L"Continuous", L"0.08s", L"Instant", L"-", L"180 px" }
            },
            0.55f, 0.90f, 1.00f,
            { 0.22f, 1.00f, 0.94f, 1.00f, 0.50f, 0.28f }
        },
    };
    static int   s_WeaponSel      = 0;
    static float s_WeaponHover[kWCCount] = {};
    if (s_WeaponSel < 0 || s_WeaponSel >= kWCCount) s_WeaponSel = 0;

    // 레이더 차트 애니메이션: 선택된 무기 norm 값으로 매 프레임 접근
    {
        const float* tgt = kWeaponDetail[s_WeaponSel].norm;
        float spd = (g_RunConfigStep >= 1) ? 12.0f : 0.0f;
        for (int i = 0; i < 6; ++i)
            s_RadarCur[i] = UiApproach(s_RadarCur[i], tgt[i], dt, spd);
    }

    // 무기 전환 트랜지션: 선택이 바뀌면 패널 콘텐츠를 페이드아웃 후 인
    if (g_RunConfigStep >= 1 && s_WeaponSel != s_PanelPrevSel) {
        s_PanelFadeT   = 0.0f;
        s_StatGlitchT  = 0.0f;
        s_PanelPrevSel = s_WeaponSel;
    }
    s_PanelFadeT = std::min(1.0f, s_PanelFadeT + dt * 5.5f);
    s_StatGlitchT = std::min(1.0f, s_StatGlitchT + dt * 8.0f);
    const float panelContentA = Smoothstep(s_PanelFadeT);

    const float accentR = (g_RunConfigStep >= 1) ? kWeaponDetail[s_WeaponSel].r : 0.42f;
    const float accentG = (g_RunConfigStep >= 1) ? kWeaponDetail[s_WeaponSel].g : 0.28f;
    const float accentB = (g_RunConfigStep >= 1) ? kWeaponDetail[s_WeaponSel].b : 1.00f;

    auto clearTrialSelection = [&]() {
        for (int i = 0; i < TRIAL_SLOT_COUNT; ++i)
            g_TrialSelected[i] = false;
    };

    auto startConfiguredRun = [&]() {
        if (!s_TrialsEnabled)
            clearTrialSelection();
        g_SelectedJob = kWC[s_WeaponSel].job;
        g_Difficulty = Difficulty::NORMAL;
        s_TrialSelectPage = false;
        if (g_CreativeMode) {
            g_GameManager.currentState = GameState::CREATIVE_CONFIG;
        } else {
            ResetForNewGame();
            FinalizeLoadout(c, FixedWeaponForSelectedJob());
        }
    };

    if (s_TrialTargetCount < 1) s_TrialTargetCount = 1;
    if (s_TrialTargetCount > TRIAL_SLOT_COUNT) s_TrialTargetCount = TRIAL_SLOT_COUNT;

    if (s_TrialSelectPage) {
        s_TrialsEnabled = true;

        const float pageW = std::min(panelW * 0.72f, 1040.0f * uiS);
        const float pageH = 620.0f * uiS;
        const float pageX = (sw - pageW) * 0.5f;
        const float pageY = (sh - pageH) * 0.5f;
        const float pagePad = 30.0f * uiS;
        const float rowH = 82.0f * uiS;
        const float rowG = 12.0f * uiS;

        BindMainShader();
        drawRect(pageX + 10.0f*uiS, pageY + 12.0f*uiS, pageW, pageH,
                 0.0f, 0.0f, 0.0f, 0.30f);
        drawRect(pageX, pageY, pageW, pageH,
                 0.020f + accentR * 0.012f,
                 0.026f + accentG * 0.010f,
                 0.042f + accentB * 0.012f, 0.94f);
        drawRect(pageX, pageY, pageW, 3.0f*uiS, accentR, accentG, accentB, 0.82f);
        BatchFlush();
        SetGlowFx(true);
        drawConstellFrame(pageX, pageY, pageW, pageH, accentR, accentG, accentB,
                          0.70f, 22.0f*uiS, 6.0f*uiS, 0.12f);
        BatchFlush();
        SetGlowFx(false);

        g_TextL.Draw(L"TRIAL SELECT", pageX + pagePad, pageY + 24.0f*uiS,
                     0.96f*uiS, 0.92f, 0.96f, 1.0f, 0.98f);

        wchar_t needBuf[48];
        swprintf_s(needBuf, L"%d / %d", TrialCount(), s_TrialTargetCount);
        const float needSc = 0.66f*uiS;
        g_TextL.Draw(needBuf, pageX + pageW - pagePad - g_TextL.Width(needBuf, needSc),
                     pageY + 29.0f*uiS, needSc,
                     TrialCount() == s_TrialTargetCount ? 0.60f : 1.0f,
                     TrialCount() == s_TrialTargetCount ? 0.96f : 0.64f,
                     TrialCount() == s_TrialTargetCount ? 0.54f : 0.42f, 0.96f);

        const float listX = pageX + pagePad;
        const float listY = pageY + 110.0f*uiS;
        const float listW = pageW - pagePad * 2.0f;
        for (int slot = 0; slot < TRIAL_SLOT_COUNT; ++slot) {
            const float sx = listX;
            const float sy = listY + (float)slot * (rowH + rowG);
            const int defIdx = std::max(0, std::min(TRIAL_DEF_COUNT - 1, g_TrialPool[slot]));
            const bool hovSlot = hit(sx, sy, listW, rowH);
            if (hovSlot && lmb && !g_LmbPrev) {
                if (g_TrialSelected[slot]) {
                    g_TrialSelected[slot] = false;
                } else if (TrialCount() < s_TrialTargetCount) {
                    g_TrialSelected[slot] = true;
                }
            }

            s_TrialHover[slot] = UiApproach(s_TrialHover[slot], hovSlot ? 1.0f : 0.0f, dt, 12.0f);
            const float hvSlot = s_TrialHover[slot];
            const bool picked = g_TrialSelected[slot];
            const bool locked = !picked && TrialCount() >= s_TrialTargetCount;
            const float rowA = locked ? 0.20f : (picked ? 0.58f : 0.34f + 0.10f * hvSlot);
            const float lineA = locked ? 0.10f : (picked ? 0.82f : 0.22f + 0.24f * hvSlot);

            BindMainShader();
            drawRect(sx, sy, listW, rowH,
                     0.014f + accentR * (picked ? 0.050f : 0.012f),
                     0.018f + accentG * (picked ? 0.044f : 0.010f),
                     0.030f + accentB * (picked ? 0.038f : 0.010f), rowA);
            drawRect(sx, sy, 4.0f*uiS, rowH, accentR, accentG, accentB, lineA);
            drawRect(sx, sy + rowH - 1.0f*uiS, listW, 1.0f*uiS,
                     accentR, accentG, accentB, lineA * 0.55f);

            const float cbx = sx + 22.0f*uiS;
            const float cby = sy + 14.0f*uiS;
            g_TextS.Draw(L"[   ]", cbx, cby, 0.50f*uiS,
                         locked ? 0.36f : 0.78f,
                         locked ? 0.40f : 0.86f,
                         locked ? 0.48f : 0.94f,
                         locked ? 0.34f : 0.88f);
            if (picked) {
                BindMainShader();
                drawRect(cbx + 8.0f*uiS, cby + 7.0f*uiS,
                         7.0f*uiS, 7.0f*uiS, accentR, accentG, accentB, 0.94f);
            }

            wchar_t titleBuf[96];
            swprintf_s(titleBuf, L"%ls / %ls",
                       TrialStageLabel(TrialStageForDef(defIdx), nli),
                       TRIAL_DEFS[defIdx].id);
            const int bonusPct = (int)(TrialScoreBonusForDef(defIdx) * 100.0f + 0.5f);
            wchar_t bonusBuf[24];
            swprintf_s(bonusBuf, L"+%02d%%", bonusPct);

            const float textX = sx + 86.0f*uiS;
            const float bonusSc = 0.62f*uiS;
            const float bonusW = g_TextL.Width(bonusBuf, bonusSc);
            const float bonusX = sx + listW - bonusW - 24.0f*uiS;
            drawFitS(titleBuf, textX, sy + 13.0f*uiS,
                     bonusX - textX - 24.0f*uiS, 0.56f*uiS, 0.40f*uiS,
                     picked ? accentR : 0.74f,
                     picked ? accentG : 0.84f,
                     picked ? accentB : 0.98f,
                     locked ? 0.36f : (picked ? 0.98f : 0.82f));
            drawFitS(TRIAL_DEFS[defIdx].desc[nli], textX, sy + 45.0f*uiS,
                     bonusX - textX - 24.0f*uiS, 0.42f*uiS, 0.30f*uiS,
                     0.48f, 0.58f, 0.76f,
                     locked ? 0.26f : (picked ? 0.72f : 0.56f));
            g_TextL.Draw(bonusBuf, bonusX, sy + 24.0f*uiS, bonusSc,
                         picked ? 0.72f : 0.48f,
                         picked ? 1.0f  : 0.62f,
                         picked ? 0.58f : 0.78f,
                         locked ? 0.34f : 0.88f);
        }

        const float backW = 168.0f*uiS;
        const float btnH = 50.0f*uiS;
        const float btnY = pageY + pageH - pagePad - btnH;
        const float confirmW = 292.0f*uiS;
        const float confirmX = pageX + pageW - pagePad - confirmW;
        const bool backHov = hit(pageX + pagePad, btnY, backW, btnH);
        const bool ready = TrialCount() == s_TrialTargetCount;
        const bool confirmHov = ready && hit(confirmX, btnY, confirmW, btnH);
        if (backHov && lmb && !g_LmbPrev) {
            clearTrialSelection();
            s_TrialSelectPage = false;
        }
        if (confirmHov && g_LmbPrev && !lmb)
            startConfiguredRun();

        BindMainShader();
        drawRect(pageX + pagePad, btnY, backW, btnH, 0.028f, 0.034f, 0.052f, 0.72f);
        drawBorder(pageX + pagePad, btnY, backW, btnH,
                   0.50f, 0.60f, 0.78f, backHov ? 0.70f : 0.34f, 1.3f*uiS);
        drawCenterS(T(StrId::BTN_BACK), pageX + pagePad, btnY + 15.0f*uiS, backW, 0.48f*uiS,
                    0.78f, 0.86f, 1.0f, 0.88f);

        drawRect(confirmX, btnY, confirmW, btnH,
                 (0.028f + accentR * 0.026f) * (ready ? 1.0f : 0.34f),
                 (0.034f + accentG * 0.022f) * (ready ? 1.0f : 0.34f),
                 (0.052f + accentB * 0.020f) * (ready ? 1.0f : 0.34f),
                 ready ? 0.84f : 0.46f);
        BatchFlush();
        SetGlowFx(true);
        drawConstellFrame(confirmX, btnY, confirmW, btnH, accentR, accentG, accentB,
                          ready ? (confirmHov ? 1.0f : 0.78f) : 0.18f,
                          16.0f*uiS, 5.0f*uiS, ready ? 0.16f : 0.0f);
        BatchFlush();
        SetGlowFx(false);
        const wchar_t* confirmLabel = ready
            ? ((li == 0) ? L"시작" : L"PLAY")
            : ((li == 0) ? L"시련 선택 필요" : L"SELECT REQUIRED");
        drawCenterL(confirmLabel, confirmX,
                    btnY + (btnH - g_TextL.Height(confirmLabel, 0.70f*uiS)) * 0.5f,
                    confirmW, 0.70f*uiS, 1.0f, 1.0f, 1.0f, ready ? 0.98f : 0.42f);

        BatchFlush();
        g_BatchAlpha = 1.0f;
        return;
    }

    BindMainShader();
    g_TextL.Draw(L"RUN CONFIG", panelX, panelY + 4.0f * uiS, 1.16f * uiS,
                 0.72f, 0.82f, 1.0f, 0.98f);
    BatchFlush();
    SetGlowFx(true);
    drawRect(panelX, panelY + headerH - 10.0f * uiS, panelW, 1.0f * uiS,
             accentR, accentG, accentB, 0.18f);
    BatchFlush();
    SetGlowFx(false);

    // ── 좌측: 무기 선택 카드 목록 (고정 높이, 여유 간격) ─────────
    const float cH      = 118.0f * uiS;
    const float cGap    = 28.0f * uiS;
    const float cTopPad = 34.0f * uiS;
    const float cPad    = 22.0f * uiS;

    for (int i = 0; i < kWCCount; ++i) {
        float cx = leftX;
        float cy = colY + cTopPad + (float)i * (cH + cGap);
        bool hov = hit(cx, cy, leftW, cH);
        bool selected = (g_RunConfigStep >= 1 && s_WeaponSel == i);
        s_WeaponHover[i] = UiApproach(s_WeaponHover[i], hov ? 1.0f : 0.0f, dt, 10.0f);
        if (hov && lmb && !g_LmbPrev) { s_WeaponSel = i; g_RunConfigStep = 1; }

        const WDetailDef& wd = kWeaponDetail[i];
        float hv = s_WeaponHover[i];
        const float focus = selected ? 1.0f : hv;
        const float pulse = selected ? (0.55f + 0.45f * sinf(now * 3.4f + (float)i * 0.8f)) : hv;
        const float inactiveA = selected ? 1.0f : (0.56f + 0.18f * hv);
        const float inactiveR = selected ? 1.0f : 0.72f;
        const float inactiveG = selected ? 1.0f : 0.78f;
        const float inactiveB = selected ? 1.0f : 0.88f;

        // 카드 바디 (어두운 배경 — non-glow)
        BindMainShader();
        drawRect(cx - 7.0f*uiS, cy - 7.0f*uiS, leftW + 14.0f*uiS, cH + 14.0f*uiS,
                 0.0f, 0.0f, 0.0f, selected ? 0.11f : 0.045f + 0.030f * hv);
        drawRect(cx - 3.0f*uiS, cy - 3.0f*uiS, leftW + 6.0f*uiS, cH + 6.0f*uiS,
                 wd.r, wd.g, wd.b, selected ? 0.080f + 0.040f * pulse : 0.025f + 0.045f * hv);
        drawRect(cx, cy, leftW, cH,
                 0.050f + wd.r * (selected ? 0.045f : 0.020f + 0.018f * hv),
                 0.058f + wd.g * (selected ? 0.036f : 0.016f + 0.016f * hv),
                 0.082f + wd.b * (selected ? 0.032f : 0.014f + 0.014f * hv),
                 selected ? 0.24f : (0.10f + 0.08f * hv));

        // 컬러 스트라이프 + 별자리 프레임 (glow on)
        BatchFlush();
        SetGlowFx(true);
        drawDiamond(cx + 14.0f*uiS, cy + 15.0f*uiS, (4.5f + 1.2f * pulse) * uiS,
                    wd.r, wd.g, wd.b, selected ? 0.90f : 0.42f + 0.30f * hv);
        drawDiamond(cx + leftW - 14.0f*uiS, cy + cH - 14.0f*uiS, (4.0f + 1.0f * pulse) * uiS,
                    wd.r, wd.g, wd.b, selected ? 0.76f : 0.28f + 0.28f * hv);
        drawConstellFrame(cx, cy, leftW, cH, wd.r, wd.g, wd.b,
                          selected ? 0.58f : 0.10f + 0.22f * hv,
                          16.0f * uiS, 3.2f * uiS,
                          selected ? 0.070f + 0.035f * pulse : 0.020f * hv);
        BatchFlush();
        SetGlowFx(false);

        // 텍스트
        const wchar_t* stateTag = selected ? L"ACTIVE" : (hov ? L"READY" : L"IDLE");
        float tagSc = 0.44f * uiS;
        float tagW = g_TextS.Width(stateTag, tagSc);
        g_TextS.Draw(stateTag, cx + leftW - tagW - cPad, cy + 12.0f*uiS,
                     tagSc, selected ? wd.r : 0.58f, selected ? wd.g : 0.66f, selected ? wd.b : 0.82f,
                     selected ? 0.92f : 0.42f + 0.28f * hv);
        g_TextL.Draw(kWC[i].name[nli], cx + cPad, cy + 14.0f*uiS,
                     1.08f * uiS, inactiveR, inactiveG, inactiveB,
                     selected ? 0.98f : inactiveA);
        if (selected) {
            g_TextS.Draw(L">", cx + 7.0f * uiS, cy + 47.0f * uiS,
                         0.64f * uiS, wd.r, wd.g, wd.b, 0.90f);
            BindMainShader();
            drawRect(cx + 28.0f * uiS, cy + cH - 20.0f * uiS,
                     leftW - 42.0f * uiS, 1.1f * uiS,
                     wd.r, wd.g, wd.b, (0.11f + 0.04f * pulse));
        }
        drawFitS(kWC[i].desc[nli], cx + cPad, cy + 84.0f*uiS,
                 leftW - cPad * 2.0f, 0.58f*uiS, 0.42f*uiS,
                 selected ? 0.72f : 0.56f,
                 selected ? 0.84f : 0.64f,
                 selected ? 1.00f : 0.76f,
                 selected ? 0.88f : 0.58f + 0.22f * hv);
    }

    // ── 우측: 무기 상세 패널 (배경 카드 통합) ─────────────────────
    const float rDX  = midX + 12.0f * uiS;
    const float rDW  = (panelX + panelW) - rDX - 8.0f * uiS;
    const float rPad = 22.0f * uiS;

    // 전체 배경 카드 (반투명 — 배경화면 살짝 비침)
    BindMainShader();
    drawRect(rDX, colY, rDW, colH, 0.038f, 0.044f, 0.068f, 0.68f);
    BatchFlush();
    SetGlowFx(true);
    drawConstellFrame(rDX, colY, rDW, colH, accentR, accentG, accentB,
                      (g_RunConfigStep >= 1) ? 0.60f : 0.22f,
                      20.0f * uiS, 6.0f * uiS,
                      (g_RunConfigStep >= 1) ? 0.18f : 0.06f);
    BatchFlush();
    SetGlowFx(false);

    if (g_RunConfigStep < 1) {
        const wchar_t* ph = (li == 0) ? L"무기를 선택하세요" : L"SELECT A WEAPON";
        float phSc = 0.94f * uiS;
        float phW  = g_TextL.Width(ph, phSc);
        g_TextL.Draw(ph, rDX + (rDW - phW) * 0.5f, colY + colH * 0.44f,
                     phSc, 0.32f, 0.38f, 0.58f, 0.48f);
    } else {
        const WCardDef&   wc = kWC[s_WeaponSel];
        const WDetailDef& wd = kWeaponDetail[s_WeaponSel];
        const float wr = wd.r, wg = wd.g, wb = wd.b;
        float ry = colY + rPad;

        // 전환 페이드 — 이 블록 내 모든 드로에 panelContentA 적용
        BatchFlush();
        g_BatchAlpha = panelContentA;

        // 무기명 대형
        BindMainShader();
        g_TextL.Draw(wc.name[nli], rDX + rPad, ry, 1.24f*uiS, wr, wg, wb, 0.98f);

        // 프로파일 서브카드
        const float profY = ry + 74.0f * uiS;
        const float profH = 52.0f * uiS;
        const float profW = rDW - rPad * 2.0f;
        BindMainShader();
        drawRect(rDX + rPad, profY, profW, profH, 0.028f, 0.034f, 0.052f, 0.72f);
        BatchFlush();
        SetGlowFx(true);
        drawConstellFrame(rDX + rPad, profY, profW, profH, wr, wg, wb, 0.36f,
                          10.0f*uiS, 3.0f*uiS);
        BatchFlush();
        SetGlowFx(false);
        const float profileSc = 0.66f * uiS;
        drawFitS(wd.profile[nli],
                 rDX + rPad + 16.0f*uiS,
                 profY + (profH - g_TextS.Height(wd.profile[nli], profileSc)) * 0.5f,
                 profW - 26.0f*uiS, profileSc, 0.46f*uiS,
                 0.68f, 0.80f, 0.96f, 0.84f);

        // 구분선 (glow)
        float divY1 = profY + profH + 24.0f * uiS;
        BatchFlush();
        SetGlowFx(true);
        drawRect(rDX + rPad, divY1, rDW - rPad * 2.0f, 1.0f*uiS, wr, wg, wb, 0.22f);
        BatchFlush();
        SetGlowFx(false);

        // 노트 서브카드
        const float noteCardY = divY1 + 16.0f * uiS;
        const float noteLineH = 38.0f * uiS;
        const float noteCardH = 3.0f * noteLineH + 24.0f * uiS;
        const float noteW     = rDW - rPad * 2.0f;
        BindMainShader();
        drawRect(rDX + rPad, noteCardY, noteW, noteCardH, 0.028f, 0.034f, 0.052f, 0.68f);
        BatchFlush();
        SetGlowFx(true);
        drawConstellFrame(rDX + rPad, noteCardY, noteW, noteCardH, wr, wg, wb, 0.28f,
                          10.0f*uiS, 3.0f*uiS);
        // bullet 다이아몬드
        for (int i = 0; i < 3; ++i) {
            float ny = noteCardY + 12.0f*uiS + (float)i * noteLineH;
            drawDiamond(rDX + rPad + 14.0f*uiS, ny + noteLineH * 0.5f - 2.0f*uiS,
                        5.5f*uiS, wr, wg, wb, 0.62f);
        }
        BatchFlush();
        SetGlowFx(false);
        for (int i = 0; i < 3; ++i) {
            float ny = noteCardY + 12.0f*uiS + (float)i * noteLineH;
            drawFitS(wd.notes[nli][i], rDX + rPad + 28.0f*uiS, ny + 6.0f*uiS,
                     noteW - 40.0f*uiS, 0.62f*uiS, 0.46f*uiS,
                     0.68f, 0.78f, 0.96f, 0.84f);
        }

        // 구분선 (glow)
        float divY2 = noteCardY + noteCardH + 24.0f * uiS;
        BatchFlush();
        SetGlowFx(true);
        drawRect(rDX + rPad, divY2, rDW - rPad * 2.0f, 1.0f*uiS, wr, wg, wb, 0.18f);
        BatchFlush();
        SetGlowFx(false);

        // ── 레이더 차트 + 우측 수치 리스트 ────────────────────────
        const float statsY  = divY2 + 18.0f * uiS;
        const float fullW   = rDW - rPad * 2.0f;
        const float radarSW = fullW * 0.46f;
        const float radarH  = 238.0f * uiS;
        const float radarCX = rDX + rPad + radarSW * 0.5f;
        const float radarCY = statsY + radarH * 0.5f;
        const float radarR  = 84.0f * uiS;

        // 6축 각도 + 최대점 좌표
        const int RN = 6;
        float axAng[RN], axTX[RN], axTY[RN];
        for (int i = 0; i < RN; ++i) {
            axAng[i] = -(float)M_PI * 0.5f + (float)i * 2.0f * (float)M_PI / (float)RN;
            axTX[i]  = radarCX + cosf(axAng[i]) * radarR;
            axTY[i]  = radarCY + sinf(axAng[i]) * radarR;
        }

        // 임의 각도 선 그리기 (BatchTri 기반)
        auto drawLine2 = [](float x1, float y1, float x2, float y2,
                            float r, float g, float b, float a, float thick) {
            float dx = x2-x1, dy = y2-y1;
            float len = sqrtf(dx*dx + dy*dy); if (len < 0.5f) return;
            float nx = -dy/len*thick*0.5f, ny = dx/len*thick*0.5f;
            BatchTri(x1+nx, y1+ny, x1-nx, y1-ny, x2+nx, y2+ny, r, g, b, a);
            BatchTri(x1-nx, y1-ny, x2-nx, y2-ny, x2+nx, y2+ny, r, g, b, a);
        };

        // 그리드 링 4개 (어두운 육각형)
        BindMainShader();
        for (int ring = 1; ring <= 4; ++ring) {
            float rr = radarR * (float)ring * 0.25f;
            float ga = 0.07f + 0.05f * (float)ring;
            for (int i = 0; i < RN; ++i) {
                int j = (i+1) % RN;
                float px = radarCX + cosf(axAng[i])*rr, py = radarCY + sinf(axAng[i])*rr;
                float qx = radarCX + cosf(axAng[j])*rr, qy = radarCY + sinf(axAng[j])*rr;
                drawLine2(px, py, qx, qy, wr, wg, wb, ga, 1.0f);
            }
        }
        // 6개 축 선
        for (int i = 0; i < RN; ++i)
            drawLine2(radarCX, radarCY, axTX[i], axTY[i], wr, wg, wb, 0.18f, 1.0f);

        // 스탯 폴리곤 채움 + 외곽선 + 버텍스 노드 (glow)
        BatchFlush();
        SetGlowFx(true);
        for (int i = 0; i < RN; ++i) {
            int j = (i+1) % RN;
            float px = radarCX + cosf(axAng[i])*radarR*s_RadarCur[i];
            float py = radarCY + sinf(axAng[i])*radarR*s_RadarCur[i];
            float qx = radarCX + cosf(axAng[j])*radarR*s_RadarCur[j];
            float qy = radarCY + sinf(axAng[j])*radarR*s_RadarCur[j];
            BatchTri(radarCX, radarCY, px, py, qx, qy, wr, wg, wb, 0.24f);
        }
        for (int i = 0; i < RN; ++i) {
            int j = (i+1) % RN;
            float px = radarCX + cosf(axAng[i])*radarR*s_RadarCur[i];
            float py = radarCY + sinf(axAng[i])*radarR*s_RadarCur[i];
            float qx = radarCX + cosf(axAng[j])*radarR*s_RadarCur[j];
            float qy = radarCY + sinf(axAng[j])*radarR*s_RadarCur[j];
            drawLine2(px, py, qx, qy, wr, wg, wb, 0.88f, 1.8f);
        }
        for (int i = 0; i < RN; ++i) {
            float px = radarCX + cosf(axAng[i])*radarR*s_RadarCur[i];
            float py = radarCY + sinf(axAng[i])*radarR*s_RadarCur[i];
            drawDiamond(px, py, 5.0f*uiS, wr, wg, wb, 0.92f);
        }
        BatchFlush();
        SetGlowFx(false);

        // 축 레이블 (각 꼭짓점 바깥)
        for (int i = 0; i < RN; ++i) {
            float tox = cosf(axAng[i]), toy = sinf(axAng[i]);
            float lx = radarCX + tox * (radarR + 28.0f*uiS);
            float ly = radarCY + toy * (radarR + 28.0f*uiS);
            float sc = 0.62f * uiS;
            float lw = g_TextS.Width(kWeaponStatLabels[i][nli], sc);
            float lh = g_TextS.Height(kWeaponStatLabels[i][nli], sc);
            float tx = lx - lw * (0.5f - tox * 0.5f);
            float ty = ly - lh * (0.5f - toy * 0.5f);
            g_TextS.Draw(kWeaponStatLabels[i][nli], tx, ty, sc, 0.94f, 0.98f, 1.0f, 0.92f);
        }

        // 우측 수치 리스트 (레이더와 동기화되는 막대 그래프)
        const float listX  = rDX + rPad + radarSW + 18.0f*uiS;
        const float listW  = fullW - radarSW - 18.0f*uiS;
        const float listRH = radarH / 6.0f;
        BindMainShader();
        drawRect(listX - 10.0f*uiS, statsY + 8.0f*uiS, 3.0f*uiS, radarH - 16.0f*uiS,
                 wr, wg, wb, 0.44f);
        for (int i = 0; i < 6; ++i) {
            float lry  = statsY + (float)i * listRH;
            float rowY = lry + 3.0f*uiS;
            float rowH = listRH - 6.0f*uiS;
            BindMainShader();
            drawRect(listX, rowY, listW, rowH,
                     0.018f + wr*0.014f, 0.024f + wg*0.010f, 0.040f + wb*0.010f, 0.58f);
            g_TextS.Draw(kWeaponStatLabels[i][nli], listX + 12.0f*uiS, rowY + 2.0f*uiS,
                         0.60f*uiS, 0.84f, 0.92f, 1.0f, 0.92f);
            const wchar_t* statText = wd.stat[nli][i];
            wchar_t glitchBuf[64];
            float statValueA = 0.90f;
            if (s_StatGlitchT < 1.0f) {
                const wchar_t* glyphs = L"0123456789ABCDEF";
                int tick = (int)(now * 54.0f);
                int p = 0;
                for (; statText[p] && p < 63; ++p) {
                    wchar_t ch = statText[p];
                    bool digit = (ch >= L'0' && ch <= L'9');
                    bool upper = (ch >= L'A' && ch <= L'Z');
                    bool lower = (ch >= L'a' && ch <= L'z');
                    if (digit || ((upper || lower) && s_StatGlitchT < 0.72f)) {
                        glitchBuf[p] = glyphs[(tick + s_WeaponSel * 17 + i * 11 + p * 5) & 15];
                    } else {
                        glitchBuf[p] = ch;
                    }
                }
                glitchBuf[p] = 0;
                statText = glitchBuf;
                statValueA = 0.76f + 0.18f * (float)((tick + i) & 1);
            }

            float valueSc = 0.64f * uiS;
            float valueW = g_TextS.Width(statText, valueSc);
            const float valueMaxW = listW * 0.42f;
            while (valueW > valueMaxW && valueSc > 0.36f*uiS) {
                valueSc *= 0.94f;
                valueW = g_TextS.Width(statText, valueSc);
            }
            g_TextS.Draw(statText, listX + listW - valueW - 12.0f*uiS, rowY + 3.0f*uiS,
                         valueSc, 0.94f, 0.98f, 1.0f, statValueA);

            const float barX = listX + 12.0f * uiS;
            const float barW = listW - 24.0f * uiS;
            const float barH = 6.0f * uiS;
            const float barY = rowY + rowH - 9.0f * uiS;
            float barT = s_RadarCur[i];
            if (barT < 0.0f) barT = 0.0f;
            if (barT > 1.0f) barT = 1.0f;
            barT = Smoothstep(barT);

            BindMainShader();
            drawRect(barX, barY, barW, barH, wr, wg, wb, 0.08f);
            drawRect(barX, barY, barW * 0.35f, barH, wr, wg, wb, 0.08f);
            drawRect(barX + barW * 0.70f, barY, 1.0f*uiS, barH, wr, wg, wb, 0.18f);
            BatchFlush();
            SetGlowFx(true);
            drawRect(barX, barY, barW * barT, barH, wr, wg, wb, 0.70f);
            if (barT > 0.02f) {
                drawDiamond(barX + barW * barT, barY + barH * 0.5f,
                            3.8f*uiS, wr, wg, wb, 0.88f);
            }
            BatchFlush();
            SetGlowFx(false);
        }

        // ── 시련 토글 서브카드 ────────────────────────────────────
        const float trialCardY = statsY + radarH + 16.0f * uiS;
        const float trialCardH = 66.0f * uiS;
        const float trialX     = rDX + rPad;
        const float actionGap  = 18.0f * uiS;
        const float actionW    = 286.0f * uiS;
        const float actionX    = rDX + rDW - rPad - actionW;
        const float actionY    = trialCardY;
        const float actionH    = trialCardH;
        const float trialW     = actionX - actionGap - trialX;
        const float stepW      = 154.0f * uiS;
        const float stepH      = 38.0f * uiS;
        const float stepX      = trialX + trialW - stepW - 18.0f * uiS;
        const float stepY      = trialCardY + (trialCardH - stepH) * 0.5f;
        const float arrowW     = 36.0f * uiS;
        const bool decHov      = s_TrialsEnabled && hit(stepX, stepY, arrowW, stepH);
        const bool incHov      = s_TrialsEnabled && hit(stepX + stepW - arrowW, stepY, arrowW, stepH);
        const bool stepHov     = s_TrialsEnabled && hit(stepX, stepY, stepW, stepH);
        const float toggleW    = trialW - (s_TrialsEnabled ? (stepW + 34.0f * uiS) : 0.0f);
        bool trialToggleHov = hit(trialX, trialCardY, toggleW, trialCardH);
        bool trialHov = trialToggleHov || stepHov;
        if (trialToggleHov && lmb && !g_LmbPrev) {
            s_TrialsEnabled = !s_TrialsEnabled;
            if (!s_TrialsEnabled) {
                clearTrialSelection();
                s_TrialSelectPage = false;
            }
        }
        if (s_TrialsEnabled && lmb && !g_LmbPrev) {
            if (decHov) {
                s_TrialTargetCount = std::max(1, s_TrialTargetCount - 1);
                clearTrialSelection();
            } else if (incHov) {
                s_TrialTargetCount = std::min(TRIAL_SLOT_COUNT, s_TrialTargetCount + 1);
                clearTrialSelection();
            }
        }

        float scoreMult = 1.0f;
        wchar_t rateBuf[32];
        swprintf_s(rateBuf, L"x%.2f", scoreMult);
        float rateSc = 0.62f*uiS;

        BindMainShader();
        drawRect(trialX, trialCardY, trialW, trialCardH,
                 0.018f, 0.022f, 0.036f, trialHov ? 0.46f : 0.34f);
        drawRect(trialX, trialCardY, trialW, 1.0f*uiS,
                 wr, wg, wb, s_TrialsEnabled ? 0.34f : 0.10f);
        drawRect(trialX, trialCardY + trialCardH - 1.0f*uiS, trialW, 1.0f*uiS,
                 wr, wg, wb, s_TrialsEnabled ? 0.22f : 0.06f);
        if (s_TrialsEnabled) {
            drawRect(trialX, trialCardY, 4.0f*uiS, trialCardH,
                     wr, wg, wb, 0.52f);
        }
        const float checkSc = 0.66f * uiS;
        const float checkX = trialX + 18.0f * uiS;
        const wchar_t* checkText = L"[   ]";
        const float checkY = trialCardY + (trialCardH - g_TextS.Height(checkText, checkSc)) * 0.5f;
        g_TextS.Draw(checkText, checkX, checkY, checkSc,
                     s_TrialsEnabled ? 0.94f : 0.46f,
                     s_TrialsEnabled ? 1.00f : 0.50f,
                     s_TrialsEnabled ? 1.00f : 0.58f,
                     s_TrialsEnabled ? 0.96f : 0.54f);
        if (s_TrialsEnabled) {
            BindMainShader();
            drawRect(checkX + 9.0f*uiS, checkY + 8.0f*uiS,
                     8.0f*uiS, 8.0f*uiS, wr, wg, wb, 0.92f);
        }

        const wchar_t* stateText = s_TrialsEnabled
            ? L"TRIAL MODULE : ACTIVE"
            : L"TRIAL MODULE : DISABLED";
        const float stateSc = 0.66f * uiS;
        const float stateY = trialCardY + (trialCardH - g_TextS.Height(stateText, stateSc)) * 0.5f;
        const float stateX = checkX + 78.0f * uiS;
        const float rateW = g_TextL.Width(rateBuf, rateSc);
        const float rateX = trialX + trialW - rateW - 64.0f*uiS;
        const float textLimitX = s_TrialsEnabled ? stepX : rateX;
        drawFitS(stateText, stateX, stateY,
                 textLimitX - stateX - 18.0f*uiS, stateSc, 0.38f*uiS,
                 s_TrialsEnabled ? wr : 0.52f,
                 s_TrialsEnabled ? wg : 0.58f,
                 s_TrialsEnabled ? wb : 0.68f,
                 s_TrialsEnabled ? 0.96f : 0.62f);

        BindMainShader();
        if (s_TrialsEnabled) {
            drawRect(stepX, stepY, stepW, stepH, 0.018f, 0.024f, 0.038f, stepHov ? 0.64f : 0.46f);
            drawRect(stepX, stepY, stepW, 1.0f*uiS, wr, wg, wb, 0.34f);
            drawRect(stepX, stepY + stepH - 1.0f*uiS, stepW, 1.0f*uiS, wr, wg, wb, 0.22f);
            drawCenterL(L"<", stepX, stepY + 4.0f*uiS, arrowW, 0.64f*uiS,
                        decHov ? 1.0f : 0.58f, decHov ? 1.0f : 0.70f, decHov ? 1.0f : 0.90f, 0.92f);
            drawCenterL(L">", stepX + stepW - arrowW, stepY + 4.0f*uiS, arrowW, 0.64f*uiS,
                        incHov ? 1.0f : 0.58f, incHov ? 1.0f : 0.70f, incHov ? 1.0f : 0.90f, 0.92f);
            wchar_t countBuf[16];
            swprintf_s(countBuf, L"%02d", s_TrialTargetCount);
            drawCenterL(countBuf, stepX + arrowW, stepY + 3.0f*uiS, stepW - arrowW * 2.0f,
                        0.68f*uiS, wr, wg, wb, 0.98f);
        } else {
            g_TextL.Draw(rateBuf, rateX,
                         trialCardY + (trialCardH - g_TextL.Height(rateBuf, rateSc)) * 0.5f, rateSc,
                         0.38f, 0.54f, 0.72f, 0.92f);
        }

        const bool actionHov = hit(actionX, actionY, actionW, actionH);
        const bool actionClick = actionHov && g_LmbPrev && !lmb;
        const wchar_t* actionLabel = s_TrialsEnabled
            ? ((li == 0) ? L"시련 선택" : L"SELECT TRIALS")
            : ((li == 0) ? L"시작" : L"PLAY");
        BindMainShader();
        drawRect(actionX, actionY, actionW, actionH,
                 0.014f + wr * 0.020f,
                 0.018f + wg * 0.018f,
                 0.028f + wb * 0.018f, actionHov ? 1.0f : 0.96f);
        if (actionHov) {
            drawRect(actionX + 4.0f*uiS, actionY + 4.0f*uiS,
                     actionW - 8.0f*uiS, actionH - 8.0f*uiS,
                     wr, wg, wb, 0.14f);
        }
        drawBorder(actionX, actionY, actionW, actionH, wr, wg, wb,
                   actionHov ? 0.86f : 0.66f, 1.4f * uiS);
        const float cornerL = 28.0f * uiS;
        const float cornerT = 2.4f * uiS;
        drawRect(actionX, actionY, cornerL, cornerT, wr, wg, wb, actionHov ? 0.96f : 0.78f);
        drawRect(actionX, actionY, cornerT, cornerL, wr, wg, wb, actionHov ? 0.96f : 0.78f);
        drawRect(actionX + actionW - cornerL, actionY, cornerL, cornerT, wr, wg, wb, actionHov ? 0.96f : 0.78f);
        drawRect(actionX + actionW - cornerT, actionY, cornerT, cornerL, wr, wg, wb, actionHov ? 0.96f : 0.78f);
        drawRect(actionX, actionY + actionH - cornerT, cornerL, cornerT, wr, wg, wb, actionHov ? 0.96f : 0.78f);
        drawRect(actionX, actionY + actionH - cornerL, cornerT, cornerL, wr, wg, wb, actionHov ? 0.96f : 0.78f);
        drawRect(actionX + actionW - cornerL, actionY + actionH - cornerT, cornerL, cornerT, wr, wg, wb, actionHov ? 0.96f : 0.78f);
        drawRect(actionX + actionW - cornerT, actionY + actionH - cornerL, cornerT, cornerL, wr, wg, wb, actionHov ? 0.96f : 0.78f);
        const float actionSc = 0.92f * uiS;
        drawCenterL(actionLabel,
                    actionX, actionY + (actionH - g_TextL.Height(actionLabel, actionSc)) * 0.5f,
                    actionW, actionSc,
                    1.0f, 1.0f, 1.0f, 0.99f);
        if (actionClick) {
            if (s_TrialsEnabled) {
                clearTrialSelection();
                s_TrialSelectPage = true;
            } else {
                startConfiguredRun();
            }
        }

        // 전환 페이드 복구
        BatchFlush();
        g_BatchAlpha = 1.0f;
    }


    if (UIButton(panelX, footY, 168.0f * uiS, 50.0f * uiS, T(StrId::BTN_BACK),
                 mx, my, lmb, g_LmbPrev)) {
        ResetRunConfigUi();
        ResetMainMenuUi();
        g_GameManager.currentState = GameState::MAIN_MENU;
    }

    const float devW = 128.0f * uiS;
    const float devX = panelX + panelW - devW;
    bool devHov = hit(devX, footY, devW, 50.0f * uiS);
    if (devHov && lmb && !g_LmbPrev)
        g_CreativeMode = !g_CreativeMode;
    BindMainShader();
    bool devOn = g_CreativeMode;
    drawRect(devX, footY, devW, 50.0f * uiS,
             devOn ? 0.13f : 0.05f,
             devOn ? 0.08f : 0.06f,
             devOn ? 0.03f : 0.08f, 0.92f);
    drawBorder(devX, footY, devW, 50.0f * uiS,
               devOn ? 1.0f : 0.30f,
               devOn ? 0.55f : 0.40f,
               devOn ? 0.10f : 0.55f,
               devOn ? 0.86f : (devHov ? 0.34f : 0.20f), 1.5f * uiS);
    drawCenterS(devOn ? L"DEV ON" : L"DEV OFF", devX, footY + 14.0f * uiS, devW,
                0.60f * uiS,
                devOn ? 1.0f : 0.55f,
                devOn ? 0.68f : 0.62f,
                devOn ? 0.18f : 0.78f, 0.90f);

    BatchFlush();
    g_BatchAlpha = 1.0f;
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
                    {L"None",-1}, {L"VOLLEY.sys",2}, {L"TESS.glitch",10},
                    {L"FORK.worm",8}, {L"ETHER_SWORD",20}
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
                    wchar_t cb[48]; swprintf_s(cb, L"selected: %d", (int)g_CreativeStartAugList.size());
                    g_TextS.Draw(cb, gx, gBottom + 10.0f, 0.85f, 1.0f, 0.9f, 0.4f, 0.95f);
                }

                if (UIButton((sw - 280.0f) * 0.5f, footY, 280.0f, 52.0f,
                             L"START", mx, my, lmb, g_LmbPrev)) {
                    ResetForNewGame();
                    FinalizeLoadout(c, FixedWeaponForSelectedJob());
                }

                if (UIButton(40.0f, footY, 160.0f, 48.0f, T(StrId::BTN_BACK),
                             mx, my, lmb, g_LmbPrev)) {
                    g_GameManager.currentState = GameState::RUN_CONFIG;
                }
}

static void Scene_RunConfigInline(const SceneCtx& c) {
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
    static float statT[6] = { 0.50f, 0.78f, 0.78f, 0.72f, 0.68f, 0.72f };
    static float radarT[6] = { 0.50f, 0.78f, 0.78f, 0.72f, 0.68f, 0.72f };
    static const float barTargets[2][6] = {
        { 0.50f, 0.78f, 0.78f, 0.72f, 0.68f, 0.72f },
        { 0.22f, 0.86f, 0.94f, 1.00f, 0.50f, 0.28f }
    };
    static float constellationT = 0.0f;

    if (entry <= 0.0f && !exiting) weapon = std::max(0, std::min(1, weapon));
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
    const float playGlow = Smoothstep(LogoClamp01((playT - 0.10f) / 0.20f))
        * (1.0f - Smoothstep(LogoClamp01((playT - 0.30f) / 0.20f)));
    const float contentA = contentIn * (1.0f - backP) * (1.0f - playCollapse);
    const bool ready = !exiting && !playExit && entry >= 0.55f;
    const float uiS = std::max(0.70f, std::min(sw * 0.94f / 1640.0f, sh * 0.90f / 910.0f));
    const float mainX = std::max(58.0f, sw * 0.075f);
    const float mainY = sh * 0.48f;
    const float mainBW = std::min(560.0f, std::max(420.0f, sw * 0.34f));
    const float mainBH = 70.0f, mainGap = 15.0f;
    const float leftX = mainX + 82.0f * uiS * (1.0f - contentIn) + 180.0f * uiS * backP;
    const float leftY = mainY;
    const float leftW = std::min(340.0f * uiS, mainBW * 0.70f);
    const float centerX = leftX + 390.0f * uiS;
    const float centerW = std::max(360.0f * uiS, sw * 0.30f);
    const float rightX = std::max(sw * 0.70f, centerX + centerW + 30.0f * uiS);
    const float rightW = sw - rightX - 70.0f * uiS;
    const float topY = MainLogoTop(sh) + MainLogoHeight(sh) * 0.72f + 32.0f * uiS;
    const float now = (float)glfwGetTime();

    DrawSceneLeftVignette(sw, sh, 0.76f * std::max(oldA, contentA));
    DrawSceneRadialVignette(rightX + rightW * 0.5f, topY + 210.0f * uiS,
                            std::max(160.0f * uiS, rightW * 0.72f), 0.44f * contentA);

    static const wchar_t* menu[5] = { L"RUN_CONFIG", L"ARMORY", L"ASTRAL_LOG", L"CALIBRATION", L"SHUTDOWN" };
    static const wchar_t* sub[5] = { L"\uC2DC\uC791", L"\uC0C1\uC810", L"\uB3C4\uAC10", L"\uC124\uC815", L"\uAC8C\uC784 \uC885\uB8CC" };
    for (int i = 0; i < 5; ++i) {
        const float y = mainY + i * (mainBH + mainGap);
        const float x = mainX - 250.0f * (1.0f - oldA) - 24.0f * (i != 0) * (1.0f - oldA);
        g_TextL.Draw(menu[i], x, y + 2.0f, 1.04f, 1.0f, 1.0f, 1.0f,
                     (i == 0 ? 0.82f : 0.18f) * oldA);
        g_TextS.Draw(sub[i], x + 4.0f, y + 43.0f, 0.48f, 0.72f, 0.77f, 0.84f,
                     (i == 0 ? 0.74f : 0.22f) * oldA);
    }

    static const wchar_t* weapons[2] = { L"RIFLE", L"STATIC FIELD" };
    static const wchar_t* weaponSub[2] = { L"PRECISION FIRE / SINGLE TARGET", L"AREA CONTROL / SUSTAINED" };
    static const wchar_t* statNames[6] = { L"DAMAGE", L"FIRE RATE", L"INTERVAL", L"SPEED", L"SPREAD", L"RANGE" };
    static const wchar_t* statVals[2][6] = {
        { L"50", L"5.00/s", L"0.20s", L"1200", L"0.04 rad", L"STANDARD" },
        { L"18/tick", L"CONTINUOUS", L"0.08s", L"INSTANT", L"-", L"180 px" }
    };
    const float wr = weapon == 0 ? 0.35f : 0.55f;
    const float wg = weapon == 0 ? 0.72f : 0.90f;
    const float wb = 1.0f;
    const float weaponH = 64.0f * uiS;
    for (int i = 0; i < 2; ++i) {
        const float y = leftY + i * (weaponH + 16.0f * uiS);
        const bool hov = ready && mx >= leftX - 28.0f && mx < centerX - 30.0f && my >= y && my < y + weaponH;
        weaponHover[i] = UiApproach(weaponHover[i], hov ? 1.0f : 0.0f, dt, 11.0f);
        if (hov && lmb && !g_LmbPrev) weapon = i;
        const float active = std::max(weaponHover[i], weapon == i ? 1.0f : 0.0f);
        const float a = Smoothstep(LogoClamp01((entry - 0.20f - i * 0.14f) / 0.32f)) * contentA;
        DrawMenuCommandFeedback(leftX - 10.0f * weaponHover[i], y, leftW, weaponH,
                                wr, wg, wb, a, active, weapon == i ? 0.18f : 0.0f, now + i * 0.17f);
        const float cr = 1.0f + (wr - 1.0f) * active;
        const float cg = 1.0f + (wg - 1.0f) * active;
        g_TextL.Draw(weapons[i], leftX - 10.0f * weaponHover[i], y + 2.0f, 0.90f,
                     cr, cg, 1.0f, (0.84f + 0.16f * active) * a);
        g_TextS.Draw(weaponSub[i], leftX + 4.0f - 10.0f * weaponHover[i], y + 39.0f, 0.42f,
                     0.70f, 0.78f, 0.88f, 0.76f * a);
    }

    const float backY = leftY + 2.0f * (weaponH + 16.0f * uiS) + 18.0f * uiS;
    const bool backHov = ready && mx >= leftX - 28.0f && mx < centerX - 30.0f
        && my >= backY && my < backY + weaponH;
    DrawMenuCommandFeedback(leftX, backY, leftW, weaponH, 0.48f, 0.82f, 1.0f,
                            contentA, backHov ? 1.0f : 0.0f, 0.0f, now + 0.46f);
    g_TextL.Draw(L"BACK", leftX, backY + 2.0f, 0.90f,
                 backHov ? 0.48f : 0.86f, backHov ? 0.82f : 0.90f, 1.0f,
                 0.90f * contentA);
    if (backHov && lmb && !g_LmbPrev) exiting = true;

    const float detailA = contentA * (0.72f + 0.28f * Smoothstep(s_PanelFadeT));
    const float configFrameX = centerX - 24.0f * uiS;
    const float configFrameY = topY - 18.0f * uiS;
    const float configFrameW = centerW + 104.0f * uiS;
    const float configFrameH = 492.0f * uiS;
    if (g_ConfigPanelTex) {
        // Use the supplied alpha-mask as a dark contrast plate behind the stat rows.
        DrawIcon(g_ConfigPanelTex, centerX - 52.0f * uiS, topY - 32.0f * uiS,
                 centerW + 160.0f * uiS, 548.0f * uiS,
                 0.0f, 0.0f, 0.0f, 0.95f * detailA);
    }
    DrawExpandingBracket(configFrameX, configFrameY, configFrameW, configFrameH,
                         Smoothstep(LogoClamp01(playT / 0.15f)), wr, wg, wb,
                         (0.28f + 0.72f * playGlow) * (detailA + playCollapse * 0.45f));
    g_TextS.Draw(L"RUN CONFIG / LOADOUT", centerX, topY, 0.54f,
                 0.48f, 0.82f, 1.0f, 0.74f * contentA);
    g_TextL.Draw(weapons[weapon], centerX, topY + 38.0f, 1.12f,
                 1.0f, 1.0f, 1.0f, 0.98f * contentA);
    g_TextS.Draw(weaponSub[weapon], centerX, topY + 78.0f, 0.48f,
                 0.70f, 0.78f, 0.88f, 0.84f * detailA);
    LogoLine(centerX, topY + 108.0f, centerX + centerW * 0.84f, topY + 108.0f,
             1.0f, wr, wg, wb, 0.30f * detailA);
    const float barW = centerW * 0.78f;
    for (int i = 0; i < 6; ++i) {
        const float y = topY + 142.0f * uiS + i * 55.0f * uiS;
        statT[i] = UiApproach(statT[i], barTargets[weapon][i], dt, 8.0f);
        g_TextS.Draw(statNames[i], centerX, y, 0.58f * uiS,
                     0.92f, 0.96f, 1.0f, 0.90f * detailA);
        g_TextS.Draw(statVals[weapon][i], centerX + barW + 16.0f * uiS, y,
                     0.54f * uiS, 0.92f, 0.96f, 1.0f, 0.90f * detailA);
        drawRect(centerX, y + 25.0f * uiS, barW, 6.0f * uiS, wr, wg, wb, 0.10f * detailA);
        drawRect(centerX, y + 25.0f * uiS, barW * std::max(0.0f, std::min(1.0f, statT[i])), 6.0f * uiS,
                 wr, wg, wb, 0.76f * detailA);
    }

    const float radarTargets[2][6] = {
        { 0.50f, 0.78f, 0.78f, 0.72f, 0.68f, 0.72f },
        { 0.22f, 0.86f, 0.94f, 1.00f, 0.50f, 0.28f }
    };
    for (int i = 0; i < 6; ++i)
        radarT[i] = UiApproach(radarT[i], radarTargets[weapon][i], dt, 9.0f);
    const float targetShape[6][2] = {
        { 0.50f, 0.12f }, { 0.84f, 0.30f }, { 0.76f, 0.78f },
        { 0.50f, 0.92f }, { 0.24f, 0.78f }, { 0.16f, 0.30f }
    };
    const float rcx = rightX + rightW * 0.5f;
    const float rcy = topY + 255.0f * uiS;
    const float rr = std::min(rightW * 0.38f, 150.0f * uiS);
    constellationT = UiApproach(constellationT, 1.0f, dt, 4.0f);
    for (int i = 0; i < 6; ++i) {
        const float px = rcx + (targetShape[i][0] - 0.5f) * rr * 2.0f * radarT[i];
        const float py = rcy + (targetShape[i][1] - 0.5f) * rr * 2.0f * radarT[i];
        const int j = (i + 1) % 6;
        const float qx = rcx + (targetShape[j][0] - 0.5f) * rr * 2.0f * radarT[j];
        const float qy = rcy + (targetShape[j][1] - 0.5f) * rr * 2.0f * radarT[j];
        const float dx = qx - px, dy = qy - py;
        const float edgeLen = sqrtf(dx * dx + dy * dy);
        const float edgeAng = atan2f(dy, dx);
        if (g_ConstellationLineTex && edgeLen > 1.0f) {
            DrawIconRot(g_ConstellationLineTex, (px + qx) * 0.5f, (py + qy) * 0.5f,
                        edgeLen * 0.5f, 7.0f * uiS, edgeAng,
                        wr, wg, wb, 0.78f * detailA);
        }
        LogoLine(px, py, qx, qy, 1.0f * uiS, wr, wg, wb,
                 (g_ConstellationLineTex ? 0.22f : 0.64f) * detailA);
        LogoLine(rcx, rcy, px, py, 0.8f * uiS, wr, wg, wb, 0.28f * detailA);
        if (g_ConstellationCircleTex) {
            DrawIcon(g_ConstellationCircleTex, px - 12.0f * uiS, py - 12.0f * uiS,
                     24.0f * uiS, 24.0f * uiS, wr, wg, wb, 0.82f * detailA);
        }
        drawDiamond(px, py, 4.0f * uiS, 1.0f, 1.0f, 1.0f, 0.94f * detailA);
    }
    for (int ring = 1; ring <= 4; ++ring) {
        const float ringR = rr * (float)ring * 0.25f;
        for (int i = 0; i < 6; ++i) {
            const int j = (i + 1) % 6;
            const float a0 = -1.5707963f + i * 1.0471976f;
            const float a1 = -1.5707963f + j * 1.0471976f;
            LogoLine(rcx + cosf(a0) * ringR, rcy + sinf(a0) * ringR,
                     rcx + cosf(a1) * ringR, rcy + sinf(a1) * ringR,
                     0.75f * uiS, wr, wg, wb, (0.10f + ring * 0.025f) * detailA);
        }
    }
    if (g_ConstellationCircleTex)
        DrawIcon(g_ConstellationCircleTex, rcx - 18.0f * uiS, rcy - 18.0f * uiS,
                 36.0f * uiS, 36.0f * uiS, 1.0f, 1.0f, 1.0f, 0.92f * detailA);
    drawDiamond(rcx, rcy, 7.0f * uiS, 1.0f, 1.0f, 1.0f, 0.98f * detailA);
    g_TextS.Draw(L"POWER PROFILE", rightX, rcy + rr + 46.0f * uiS, 0.54f * uiS,
                 0.48f, 0.82f, 1.0f, 0.74f * detailA);
    g_TextS.Draw(weapon == 0 ? L"PRECISION FIRE" : L"FIELD CONTROL",
                 rightX, rcy + rr + 72.0f * uiS, 0.62f * uiS,
                 0.92f, 0.96f, 1.0f, 0.90f * detailA);

    const float playW = 260.0f * uiS, playH = 58.0f * uiS;
    const float playX = sw - 70.0f * uiS - playW;
    const float playY = sh - 92.0f * uiS;
    const bool playHov = ready && mx >= playX && mx < playX + playW && my >= playY && my < playY + playH;
    DrawMenuCommandFeedback(playX + 18.0f, playY, playW, playH, wr, wg, wb,
                            contentA, playHov ? 1.0f : 0.0f, playHov ? 0.22f : 0.0f, now);
    g_TextL.Draw(L"PLAY", playX + 18.0f, playY + 5.0f, 0.92f,
                 playHov ? wr : 1.0f, playHov ? wg : 1.0f, 1.0f, 0.98f * contentA);
    if (playHov && lmb && !g_LmbPrev && ready) {
        playExit = true;
        playT = 0.0f;
    }
    g_BatchAlpha = 1.0f;
    if (playExit && playT >= 0.50f) {
        g_SelectedJob = weapon == 0 ? JOB_NONE : JOB_VAMPIRE;
        if (g_CreativeMode) g_GameManager.currentState = GameState::CREATIVE_CONFIG;
        else { c.reset(); FinalizeLoadout(c, FixedWeaponForSelectedJob()); }
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
    }
}

static void Scene_SettingsInline(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const float dt = std::min(c.delta, 0.05f);
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float now = (float)glfwGetTime();
    static int tab = 0;
    static int group = 0;
    static float entry = 0.0f;
    static float exitT = 0.0f;
    static bool exiting = false;
    static bool prevEsc = false;
    static bool prevRmb = false;
    static float pulse = 0.0f;
    static float rootHover[4] = {};
    static float groupHover[3] = {};
    static bool wasInline = false;
    if (!wasInline) {
        entry = 0.0f;
        exitT = 0.0f;
        exiting = false;
        prevEsc = false;
        prevRmb = false;
        wasInline = true;
    }
    entry = std::min(1.0f, entry + dt);
    pulse = std::max(0.0f, pulse - dt * 2.0f);

    const bool esc = c.window && glfwGetKey(c.window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    const bool rmb = c.window && glfwGetMouseButton(c.window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    const bool backInput = (esc && !prevEsc) || (rmb && !prevRmb);
    prevEsc = esc; prevRmb = rmb;
    const float entryOldOut = Smoothstep(LogoClamp01(entry / 0.35f));
    const float entryTreeIn = Smoothstep(LogoClamp01((entry - 0.20f) / 0.35f));
    if (backInput && !exiting && entry >= 0.55f) exiting = true;
    if (exiting) exitT = std::min(0.42f, exitT + dt);
    const float outP = exiting ? Smoothstep(LogoClamp01(exitT / 0.42f)) : 0.0f;
    const float treeA = entryTreeIn * (1.0f - outP);
    const float oldA = exiting ? outP : (1.0f - entryOldOut);
    const bool ready = !exiting && entry >= 0.55f;

    // Main-menu inline panels return before Scene_MainMenu's vignette pass.
    // Keep the same left-side contrast veil used by ARMORY and ASTRAL_LOG.
    DrawSceneLeftVignette(sw, sh, 0.76f * std::max(treeA, oldA));

    const float uiS = std::max(0.70f, std::min(sw * 0.94f / 1640.0f, sh * 0.90f / 910.0f));
    const float mainBW = std::min(560.0f, std::max(420.0f, sw * 0.34f));
    const float mainBH = 70.0f, mainGap = 15.0f;
    const float mainTotal = mainBH * 5.0f + mainGap * 4.0f;
    const float mainX = std::max(58.0f, sw * 0.075f);
    float mainY = sh * 0.48f;
    if (mainY + mainTotal > sh - 54.0f) mainY = sh - mainTotal - 54.0f;
    // Settings rows enter from the right, matching the inline ARMORY/CODEX motion.
    const float rootX = mainX + 82.0f * uiS * (1.0f - treeA) + 180.0f * uiS * outP;
    // Inline tabs occupy the same left command rail as the other main-menu panels.
    const float rootY = mainY;
    const float rootW = std::min(360.0f * uiS, mainBW * 0.70f);
    const float rootH = mainBH;
    const float depthX = rootX + std::min(365.0f * uiS, mainBW * 0.68f);
    const float depthY = std::max(110.0f * uiS, rootY - 28.0f * uiS);
    const float detailX = std::max(sw * 0.58f, depthX + 390.0f * uiS);
    const float detailY = MainLogoTop(sh) + MainLogoHeight(sh) * 0.72f + 30.0f * uiS;
    const float detailW = sw - detailX - 70.0f * uiS;

    DrawSceneRadialVignette(detailX + detailW * 0.5f, detailY + sh * 0.23f,
                            std::min(detailW, sh * 0.65f) * 0.72f, 0.34f * treeA);
    if (s_MainMenuSettingsPanel)
        DrawMainOnedowLogo(sw, sh, 0.42f * oldA + 0.08f * treeA,
                           LogoClamp01(oldA + 0.20f * treeA), sw * 0.30f - 160.0f * (1.0f - oldA), 0.82f);

    // \uC88C\uCE21 ghost \uBC84\uD2BC \u2014 \uCEE8\uD14D\uC2A4\uD2B8\uC5D0 \uB530\uB77C \uBA54\uC778\uBA54\uB274 vs \uC77C\uC2DC\uC815\uC9C0 \uBA54\uB274
    if (s_MainMenuSettingsPanel) {
        static const wchar_t* menu[5] = { L"RUN_CONFIG", L"ARMORY", L"ASTRAL_LOG", L"CALIBRATION", L"SHUTDOWN" };
        static const wchar_t* menuSub[5] = { L"\uC2DC\uC791", L"\uC0C1\uC810", L"\uB3C4\uAC10", L"\uC124\uC815", L"\uAC8C\uC784 \uC885\uB8CC" };
        for (int i = 0; i < 5; ++i) {
            const float y = mainY + i * (mainBH + mainGap);
            const bool focus = i == 3;
            const float x = mainX - 250.0f * (1.0f - oldA) - (focus ? 0.0f : 24.0f * (1.0f - oldA));
            const float a = (focus ? 0.82f : 0.34f) * oldA;
            g_TextL.Draw(menu[i], x, y + 2.0f, 1.04f, 1.0f, 1.0f, 1.0f, a);
            g_TextS.Draw(menuSub[i], x + 4.0f, y + 43.0f, 0.48f, 0.72f, 0.77f, 0.84f, a * 0.9f);
        }
    } else {
        static const wchar_t* menu[4] = { L"RESUME", L"CALIBRATION", L"RETURN_TO_HUB", L"TERMINATE" };
        static const wchar_t* menuSub[4] = { L"\uC7AC\uAC1C", L"\uC124\uC815", L"\uBA54\uC778 \uBA54\uB274", L"\uAC8C\uC784 \uC885\uB8CC" };
        for (int i = 0; i < 4; ++i) {
            const float y = mainY + i * (mainBH + mainGap);
            const bool focus = i == 1;
            const float x = mainX - 250.0f * (1.0f - oldA) - (focus ? 0.0f : 24.0f * (1.0f - oldA));
            const float a = (focus ? 0.82f : 0.34f) * oldA;
            g_TextL.Draw(menu[i], x, y + 2.0f, 1.04f, 1.0f, 1.0f, 1.0f, a);
            g_TextS.Draw(menuSub[i], x + 4.0f, y + 43.0f, 0.48f, 0.72f, 0.77f, 0.84f, a * 0.9f);
        }
    }

    struct Root { const wchar_t* en; const wchar_t* kr; };
    static const Root roots[4] = {
        { L"GAME", L"\uAC8C\uC784" }, { L"AUDIO", L"\uC624\uB514\uC624" },
        { L"SYSTEM", L"\uC2DC\uC2A4\uD15C" }, { L"BACK", L"\uB4A4\uB85C" }
    };
    for (int i = 0; i < 4; ++i) {
        const float y = rootY + i * (rootH + mainGap);
        const bool hov = ready && mx >= rootX - 28.0f && mx < depthX - 36.0f && my >= y && my < y + rootH;
        const float hoverSpeed = std::min(1.0f, dt * 11.0f);
        rootHover[i] += ((hov ? 1.0f : 0.0f) - rootHover[i]) * hoverSpeed;
        if (hov && lmb && !g_LmbPrev) {
            if (i == 3) exiting = true;
            else { tab = i; group = 0; pulse = 1.0f; }
        }
        const bool selected = i == tab && i < 3;
        const float active = std::max(rootHover[i], selected ? 1.0f : 0.0f);
        // Keep the shared entry start, but give each settings row enough air to read.
        // Explicit top-to-bottom order: GAME -> AUDIO -> SYSTEM -> BACK.
        const float rowDelay = 0.20f + (float)i * 0.14f;
        const float revealT = Smoothstep(LogoClamp01((entry - rowDelay) / 0.32f));
        const float rowA = revealT * treeA;
        const float x = rootX + (1.0f - revealT) * 34.0f - 10.0f * rootHover[i];
        const float clickFlash = (i == tab && pulse > 0.0f) ? pulse : 0.0f;
        DrawMenuCommandFeedback(x, y, rootW, rootH,
                                0.48f, 0.82f, 1.0f, rowA,
                                active, clickFlash, now + (float)i * 0.17f);
        const float colorT = std::max(rootHover[i], selected ? 0.82f : 0.0f);
        const float textR = 1.0f + (0.48f - 1.0f) * colorT;
        const float textG = 1.0f + (0.82f - 1.0f) * colorT;
        const float textB = 1.0f;
        g_TextL.Draw(roots[i].en, x, y + 2.0f, 1.02f,
                     textR, textG, textB, (0.86f + 0.14f * active) * rowA);
        g_TextS.Draw(roots[i].kr, x + 4.0f, y + 43.0f, 0.48f,
                     0.70f + 0.14f * active, 0.75f + 0.13f * active, 0.82f + 0.10f * active,
                     (0.78f + 0.18f * active) * rowA);
    }

    static const wchar_t* gameGroups[] = { L"DISPLAY", L"CONTROL", L"LANGUAGE" };
    static const wchar_t* audioGroups[] = { L"MASTER", L"CHANNELS" };
    static const wchar_t* systemGroups[] = { L"SAVE DATA", L"INFO", L"DANGER ZONE" };
    const wchar_t** groups = tab == 0 ? gameGroups : (tab == 1 ? audioGroups : systemGroups);
    const int groupCount = tab == 0 ? 3 : (tab == 1 ? 2 : 3);
    if (group >= groupCount) group = 0;
    const float branchX = depthX - 25.0f * uiS;
    LogoLine(rootX + rootW + 12.0f, rootY + tab * (rootH + mainGap) + rootH * 0.5f,
             branchX, depthY + rootH * 0.5f, 1.0f, 0.48f, 0.82f, 1.0f, 0.25f * treeA);
    for (int i = 0; i < groupCount; ++i) {
        const float y = depthY + i * 62.0f * uiS;
        const bool hov = ready && mx >= depthX - 34.0f && mx < detailX - 30.0f && my >= y && my < y + 50.0f * uiS;
        const float groupSpeed = std::min(1.0f, dt * 11.0f);
        groupHover[i] += ((hov ? 1.0f : 0.0f) - groupHover[i]) * groupSpeed;
        if (hov && lmb && !g_LmbPrev) { group = i; pulse = 1.0f; }
        const bool sel = group == i;
        const float groupActive = std::max(groupHover[i], sel ? 0.82f : 0.0f);
        LogoLine(branchX, y + 24.0f * uiS, depthX + 4.0f, y + 24.0f * uiS,
                 0.8f, 0.48f, 0.82f, 1.0f, 0.18f * treeA);
        const float groupR = 1.0f + (0.48f - 1.0f) * groupActive;
        const float groupG = 1.0f + (0.82f - 1.0f) * groupActive;
        g_TextL.Draw((sel ? L"> " : L"  "), depthX - 8.0f * groupHover[i], y + 2.0f, 0.78f,
                     groupR, groupG, 1.0f, (0.76f + 0.20f * groupActive) * treeA);
        g_TextL.Draw(groups[i], depthX + 25.0f - 8.0f * groupHover[i], y + 2.0f, 0.82f,
                     groupR, groupG, 1.0f, (0.78f + 0.18f * groupActive) * treeA);
        if (groupActive > 0.01f) {
            drawRect(depthX + 9.0f - 8.0f * groupHover[i], y + 41.0f * uiS,
                     170.0f * uiS * groupActive, 1.0f,
                     0.48f, 0.82f, 1.0f, 0.16f * groupActive * treeA);
        }
    }

    const float dA = treeA * (0.78f + 0.22f * (1.0f - pulse));
    drawRect(detailX - 12.0f, detailY + 56.0f, 2.0f, 420.0f,
             0.48f, 0.82f, 1.0f, 0.40f * dA);
    const wchar_t* tabName = roots[tab].en;
    g_TextS.Draw(L"SYS_CALIBRATION", detailX, detailY, 0.56f,
                 0.48f, 0.82f, 1.0f, 0.70f * dA);
    g_TextL.Draw(tabName, detailX, detailY + 38.0f, 1.08f,
                     1.0f, 1.0f, 1.0f, 0.98f * dA);
    const float lineY = detailY + 98.0f;
    LogoLine(detailX, lineY, detailX + detailW * 0.84f, lineY,
             1.0f, 0.48f, 0.82f, 1.0f, 0.30f * dA);

    const wchar_t* labels[6] = {};
    int labelCount = 0;
    if (tab == 0) {
        if (group == 0) { labels[0] = L"FPS LIMIT     VSYNC / 30 / 60 / 144 / 300"; labels[1] = L"CRT FX       OFF / LOW / MID / HIGH"; labels[2] = L"VFX DENSITY   FULL / REDUCED"; labelCount = 3; }
        else if (group == 1) { labels[0] = L"AUTO FIRE    ACTIVE / IDLE"; labels[1] = L"CROSSHAIR    ACTIVE / IDLE"; labelCount = 2; }
        else { labels[0] = L"LANG PACK     KOR / ENG"; labelCount = 1; }
    } else if (tab == 1) {
        labels[0] = L"MASTER VOL    0 - 100"; labels[1] = L"BGM / SFX / UI  MASTER LINKED"; labelCount = 2;
    } else {
        if (group == 0) { labels[0] = L"BEST RECORD   READ ONLY"; labels[1] = L"RUN RECORD    READ ONLY"; labelCount = 2; }
        else if (group == 1) { labels[0] = L"BUILD         DEBUG x64"; labels[1] = L"LANGUAGE      ACTIVE"; labelCount = 2; }
        else { labels[0] = L"RESET DATA    HOLD TO CONFIRM"; labelCount = 1; }
    }
    if (ready && lmb && !g_LmbPrev && mx >= detailX && mx < detailX + detailW * 0.86f) {
        const int row = (int)((my - (lineY + 42.0f)) / 58.0f);
        if (row >= 0 && row < labelCount) {
            if (tab == 0 && group == 0 && row == 0) {
                const int fps[] = { 0, 30, 60, 144, 300 };
                int next = 0;
                for (int i = 0; i < 5; ++i) if (g_FpsCap == fps[i]) next = (i + 1) % 5;
                g_FpsCap = fps[next];
                glfwSwapInterval(g_FpsCap == 0 ? 1 : 0);
            } else if (tab == 0 && group == 0 && row == 1) {
                g_ShaderFx = !g_ShaderFx;
            } else if (tab == 0 && group == 0 && row == 2) {
                g_VfxDensity = (g_VfxDensity == VfxDensity::FULL)
                    ? VfxDensity::REDUCED : VfxDensity::FULL;
            } else if (tab == 0 && group == 1 && row == 0) {
                g_AutoFire = !g_AutoFire;
            } else if (tab == 0 && group == 1 && row == 1) {
                g_ShowCrosshair = !g_ShowCrosshair;
            } else if (tab == 0 && group == 2) {
                g_Language = (g_Language == Language::EN) ? Language::KR : Language::EN;
            } else if (tab == 1 && group == 0 && row == 0) {
                g_SoundVol = (g_SoundVol >= 100) ? 0 : std::min(100, g_SoundVol + 10);
            }
        }
    }
    for (int i = 0; i < labelCount; ++i) {
        const float y = lineY + 48.0f + i * 58.0f;
        g_TextL.Draw(labels[i], detailX + 18.0f, y, 0.66f,
                     0.92f, 0.95f, 1.0f, 0.88f * dA);
        LogoLine(detailX + 18.0f, y + 35.0f, detailX + detailW * 0.78f, y + 35.0f,
                 0.8f, 0.48f, 0.82f, 1.0f, 0.16f * dA);
    }
    g_TextS.Draw(L"[ ESC / RMB ] BACK", detailX, sh - 72.0f, 0.50f,
                 0.66f, 0.75f, 0.84f, 0.72f * dA);

    if (exiting && exitT >= 0.42f) {
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
        { L"SYSTEM", { L"시스템", L"SYSTEM" },
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
    float cardWake[3];
    for (int i = 0; i < 3; ++i)
        cardWake[i] = Smoothstep(std::min(1.0f,
            std::max(0.0f, g_SettingsEntryT - (float)i * 0.09f) / 0.40f));
    const float rightWake = Smoothstep(std::min(1.0f,
        std::max(0.0f, g_SettingsEntryT - 0.10f) / 0.44f));

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
        DrawSceneLeftVignette(sw, sh, 0.62f * wake);
        DrawSceneRadialVignette(rightX + rightW * 0.52f, bodyY + bodyH * 0.52f,
                                std::min(rightW, bodyH) * 0.56f, 0.30f * rightWake);
    }

    const SettingsTabDef& tab = kTabs[s_tab];
    const float baseR = 0.48f, baseG = 0.82f, baseB = 1.00f;
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
    auto drawAngularTabFrame = [&](float x, float y, float w, float h,
                                   float r, float g, float b,
                                   float a, float active, float hover) {
        const float t = (0.8f + 0.35f * active + 0.25f * hover) * uiS;
        const float capW = (30.0f + 5.0f * active) * uiS;
        const float capH = (22.0f + 3.0f * hover) * uiS;
        const float inset = (8.0f - 2.0f * active) * uiS;
        const float notch = (14.0f + 8.0f * hover + 8.0f * active) * uiS;
        const float sweep = fmodf(now * (0.34f + 0.38f * hover + 0.20f * active), 1.0f);
        const float sx = x + inset + (w - inset * 2.0f - notch) * sweep;

        BindMainShader();
        drawRect(x, y, w, h,
                 0.050f + r * (0.030f + 0.050f * active),
                 0.060f + g * (0.026f + 0.042f * active),
                 0.085f + b * (0.024f + 0.038f * active),
                 (0.82f + 0.10f * active + 0.04f * hover) * a);
        drawRect(x, y + inset, capW, t * 1.35f, r, g, b, (0.62f + 0.26f * active + 0.12f * hover) * a);
        drawRect(x, y + inset, t * 1.55f, capH, r, g, b, (0.62f + 0.26f * active + 0.12f * hover) * a);
        drawRect(x, y + h - inset - t * 1.35f, capW, t * 1.35f, r, g, b, (0.58f + 0.22f * active + 0.12f * hover) * a);
        drawRect(x, y + h - inset - capH, t * 1.55f, capH, r, g, b, (0.58f + 0.22f * active + 0.12f * hover) * a);

        drawRect(x + w - capW, y + inset, capW, t * 1.35f, r, g, b, (0.62f + 0.26f * active + 0.12f * hover) * a);
        drawRect(x + w - t * 1.55f, y + inset, t * 1.55f, capH, r, g, b, (0.62f + 0.26f * active + 0.12f * hover) * a);
        drawRect(x + w - capW, y + h - inset - t * 1.35f, capW, t * 1.35f, r, g, b, (0.58f + 0.22f * active + 0.12f * hover) * a);
        drawRect(x + w - t * 1.55f, y + h - inset - capH, t * 1.55f, capH, r, g, b, (0.58f + 0.22f * active + 0.12f * hover) * a);

        if (hover > 0.01f) {
            drawRect(sx, y + inset, notch, t * 1.35f, r, g, b, (0.34f * hover) * a);
            drawRect(x + w - inset - notch - (sx - x - inset), y + h - inset - t * 1.35f,
                     notch, t * 1.35f, r, g, b, (0.28f * hover) * a);
        }
    };
    auto segment = [&](float x, float y, float w, float h, const wchar_t* label,
                       bool selected, float r, float g, float b, bool enabled = true) {
        bool hov = enabled && hit(x, y, w, h);
        float dim = enabled ? 1.0f : 0.36f;
        float ctlA = rightWake * (0.58f + 0.42f * Smoothstep(s_panelT));
        BindMainShader();
        drawRect(x, y, w, h,
                 (0.058f + r * (selected ? 0.125f : 0.040f)) * dim,
                 (0.066f + g * (selected ? 0.102f : 0.034f)) * dim,
                 (0.088f + b * (selected ? 0.086f : 0.030f)) * dim,
                 (enabled ? 0.96f : 0.68f) * ctlA);
        // Corner brackets + diamond nodes instead of full border
        float ba = (enabled ? (selected ? 0.94f : (hov ? 0.64f : 0.30f)) : 0.16f) * ctlA;
        float cL = std::min(w * 0.28f, 9.0f * uiS);
        float ct = 1.3f * uiS;
        drawRect(x,       y,       cL, ct, r, g, b, ba);
        drawRect(x,       y,       ct, cL, r, g, b, ba);
        drawRect(x+w-cL,  y,       cL, ct, r, g, b, ba);
        drawRect(x+w-ct,  y,       ct, cL, r, g, b, ba);
        drawRect(x,       y+h-ct,  cL, ct, r, g, b, ba);
        drawRect(x,       y+h-cL,  ct, cL, r, g, b, ba);
        drawRect(x+w-cL,  y+h-ct,  cL, ct, r, g, b, ba);
        drawRect(x+w-ct,  y+h-cL,  ct, cL, r, g, b, ba);
        float ns = (selected ? 4.5f : 3.0f) * uiS;
        drawDiamond(x,   y,   ns, r, g, b, ba);
        drawDiamond(x+w, y,   ns, r, g, b, ba);
        drawDiamond(x,   y+h, ns, r, g, b, ba);
        drawDiamond(x+w, y+h, ns, r, g, b, ba);
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
    g_TextL.Draw(L"CALIBRATION",
                 leftX, panelY + 8.0f * uiS - hSlide,
                 1.16f * uiS, 1.0f, 1.0f, 1.0f, 0.98f * wake);
    float profileW = 330.0f * uiS;
    float profileX = panelX + panelW - profileW + (1.0f - wake) * 50.0f * uiS;
    drawRect(profileX, panelY + 8.0f * uiS, profileW, 44.0f * uiS,
             0.040f, 0.050f, 0.070f, 0.82f * wake);
    drawConstellFrame(profileX, panelY + 8.0f * uiS, profileW, 44.0f * uiS,
                      baseR, baseG, baseB,
                      (0.24f + 0.08f * sinf(now * 2.2f)) * wake,
                      10.0f * uiS, 2.0f * uiS);
    drawCenterS(profileBuf, profileX, panelY + 20.0f * uiS, profileW,
                0.44f * uiS, 0.76f, 0.86f, 1.0f, 0.88f * wake);

    // === LEFT COLUMN: Tab cards ===
    const float tabCardGap = 16.0f * uiS;
    const float tabCardH   = 66.0f * uiS;

    for (int i = 0; i < kTabCount; ++i) {
        const SettingsTabDef& td = kTabs[i];
        float cardY  = bodyY + (float)i * (tabCardH + tabCardGap);
        float cwi    = cardWake[i];
        float cxOff  = (1.0f - cwi) * 62.0f * uiS;
        float cxBase = leftX - cxOff;
        bool sel = (i == s_tab);
        bool hov = hit(leftX, cardY, leftW, tabCardH);
        s_tabHover[i] = UiApproach(s_tabHover[i], hov ? 1.0f : 0.0f, delta, 10.0f);
        s_tabFocus[i] = UiApproach(s_tabFocus[i], sel ? 1.0f : 0.0f, delta, 8.5f);
        float foc = s_tabFocus[i];
        float hv  = s_tabHover[i];
        if (hov && lmb && !g_LmbPrev && !sel && !modalActive) {
            if (g_VolEdit) commitVol();
            s_tab = i; s_prevTab = i; s_panelT = 0.0f;
            s_nodePulse = 0.5f;
            s_resetDialog = false;
        }
        BindMainShader();
        if (i == 0)
            drawRect(leftX + 2.0f * uiS, bodyY, 1.2f * uiS, bodyH,
                     baseR, baseG, baseB, 0.13f * wake);
        // The old card remains a logical hitbox only; the visible UI is floating.
        if (sel || hov) {
            drawRect(cxBase + 28.0f * uiS, cardY + tabCardH * 0.78f,
                     leftW - 54.0f * uiS, 1.0f * uiS,
                     baseR, baseG, baseB, (0.06f + 0.09f*foc + 0.04f*hv) * cwi);
            drawRect(cxBase + 6.0f*uiS, cardY + 10.0f*uiS,
                     1.4f*uiS, tabCardH - 20.0f*uiS,
                     td.r, td.g, td.b, (0.24f + 0.50f*foc + 0.18f*hv) * cwi);
        }
        if (sel)
            g_TextS.Draw(L">", cxBase + 13.0f * uiS,
                         cardY + (tabCardH - g_TextS.Height(L">", 0.62f * uiS)) * 0.5f,
                         0.62f * uiS, td.r, td.g, td.b, 0.94f * cwi);
        drawDiamond(cxBase + 6.0f*uiS, cardY + tabCardH * 0.5f,
                    (3.0f + 1.5f*foc) * uiS, td.r, td.g, td.b,
                    (0.30f + 0.48f*foc + 0.18f*hv) * cwi);
        const wchar_t* tabTitle = (nli == 0) ? td.name[0] : td.id;
        const float tabIdSc = 0.98f * uiS;
        const float tabIdY  = cardY + (tabCardH - g_TextL.Height(tabTitle, tabIdSc)) * 0.5f;
        g_TextL.Draw(tabTitle, cxBase + (sel ? 34.0f : 22.0f)*uiS, tabIdY,
                     tabIdSc,
                     0.86f + baseR*0.14f*foc, 0.90f + baseG*0.10f*foc, 1.0f,
                     (0.78f+0.20f*foc)*cwi);
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
            g_TextS.Draw(L"CALIBRATION NODE", leftX + 22.0f*uiS, decoY + decoH - 34.0f*uiS,
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
        float connCwi = cardWake[s_tab];
        float connA   = connFoc * rightWake * connCwi;
        if (connA > 0.01f) {
            float selCardY = bodyY + (float)s_tab * (tabCardH + tabCardGap);
            float connY    = selCardY + tabCardH * 0.5f;
            float cx0      = leftX + leftW;
            float cx1      = rpx;
            float connH    = 1.5f * uiS;
            BindMainShader();
            drawRect(cx0, connY - connH*0.5f, cx1 - cx0, connH,
                     baseR, baseG, baseB, 0.34f * connA);
            drawDiamond(cx0, connY, 4.5f*uiS, baseR, baseG, baseB, 0.62f * connA);
            drawDiamond(cx1, connY, 4.5f*uiS, baseR, baseG, baseB, 0.62f * connA);
        }
    }

    BindMainShader();
    drawRect(rpx + 9.0f*uiS, bodyY + 11.0f*uiS, rightW, bodyH,
             0.0f, 0.0f, 0.0f, 0.035f*panelE*rightWake);
    drawRect(rpx, bodyY, rightW, bodyH,
             0.044f + baseR*0.018f,
             0.052f + baseG*0.015f,
             0.074f + baseB*0.014f, 0.10f*panelE*rightWake);
    if (rightWake > 0.02f) {
        BatchFlush(); SetGlowFx(true);
        drawConstellFrame(rpx, bodyY, rightW, bodyH,
                           baseR, baseG, baseB, 0.20f*panelE*rightWake,
                          22.0f*uiS, 6.0f*uiS, 0.14f, rightWake);
        BatchFlush(); SetGlowFx(false);
    }
    BindMainShader();
    // Master detail title
    g_TextL.Draw(L"CALIBRATION", rpx + 20.0f*uiS, bodyY + 12.0f*uiS,
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
        drawRect(rowX, y, rowW, h,
                 danger ? 0.070f : 0.040f,
                 danger ? 0.026f : 0.052f,
                 danger ? 0.030f : 0.074f,
                 0.14f * a);
        drawRect(rowX, y, rowW, h,
                 r, g, b, danger ? 0.026f * a : 0.045f * a);
        drawRect(rowX, y, 4.0f * uiS, h, r, g, b,
                 (danger ? 0.86f : 0.72f) * a);
        drawRect(rowX + 18.0f*uiS, y + 42.0f*uiS,
                 rowW - 36.0f*uiS, 1.2f*uiS, r, g, b,
                 (danger ? 0.24f : 0.20f) * a);
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
        float dim = disabled ? 0.48f : 1.0f;
        const float hovF = (hov && !disabled) ? 1.0f : 0.0f;
        BindMainShader();
        drawRect(rowX + 10.0f*uiS, y, rowW - 20.0f*uiS, h,
                 0.018f + baseR * (hovF ? 0.030f : 0.010f),
                 0.024f + baseG * (hovF ? 0.024f : 0.008f),
                 0.036f + baseB * (hovF ? 0.020f : 0.007f),
                 (disabled ? 0.10f : 0.12f + 0.10f * hovF) * rowA * dim);
        drawRect(rowX + 20.0f*uiS, y + h - 1.0f*uiS,
                 rowW - 40.0f*uiS, 1.0f*uiS, baseR, baseG, baseB,
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
            float cL = std::min(bw * 0.28f, 9.0f * uiS), ct = 1.3f * uiS;
            drawRect(ox,      by,        cL, ct, baseR, baseG, baseB, ba);
            drawRect(ox,      by,        ct, cL, baseR, baseG, baseB, ba);
            drawRect(ox+bw-cL,by,        cL, ct, baseR, baseG, baseB, ba);
            drawRect(ox+bw-ct,by,        ct, cL, baseR, baseG, baseB, ba);
            drawRect(ox,      by+ctlH-ct,cL, ct, baseR, baseG, baseB, ba);
            drawRect(ox,      by+ctlH-cL,ct, cL, baseR, baseG, baseB, ba);
            drawRect(ox+bw-cL,by+ctlH-ct,cL, ct, baseR, baseG, baseB, ba);
            drawRect(ox+bw-ct,by+ctlH-cL,ct, cL, baseR, baseG, baseB, ba);
            float ns = (active ? 3.5f : 2.5f) * uiS;
            drawDiamond(ox,    by,      ns, baseR, baseG, baseB, ba);
            drawDiamond(ox+bw, by,      ns, baseR, baseG, baseB, ba);
            drawDiamond(ox,    by+ctlH, ns, baseR, baseG, baseB, ba);
            drawDiamond(ox+bw, by+ctlH, ns, baseR, baseG, baseB, ba);
        };

        BindMainShader();
        if (value)
            drawRect(bx,   by, bw, ctlH, baseR*0.08f, baseG*0.07f, baseB*0.07f, 0.90f * ctlA);
        else if (!value)
            drawRect(offX, by, bw, ctlH, baseR*0.08f, baseG*0.07f, baseB*0.07f, 0.90f * ctlA);
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
        drawRect(bx, by, ctlW, ctlH, 0.050f, 0.058f, 0.075f, (disabled ? 0.50f : 0.82f)*infoA);
        {   // Corner brackets
            float ba = (disabled ? 0.10f : 0.26f) * infoA;
            float cL = std::min(ctlW * 0.22f, 10.0f * uiS), ct = 1.3f * uiS;
            drawRect(bx,        by,        cL, ct, baseR,baseG,baseB, ba);
            drawRect(bx,        by,        ct, cL, baseR,baseG,baseB, ba);
            drawRect(bx+ctlW-cL,by,        cL, ct, baseR,baseG,baseB, ba);
            drawRect(bx+ctlW-ct,by,        ct, cL, baseR,baseG,baseB, ba);
            drawRect(bx,        by+ctlH-ct,cL, ct, baseR,baseG,baseB, ba);
            drawRect(bx,        by+ctlH-cL,ct, cL, baseR,baseG,baseB, ba);
            drawRect(bx+ctlW-cL,by+ctlH-ct,cL, ct, baseR,baseG,baseB, ba);
            drawRect(bx+ctlW-ct,by+ctlH-cL,ct, cL, baseR,baseG,baseB, ba);
            float ns = 3.0f * uiS;
            drawDiamond(bx,       by,       ns, baseR,baseG,baseB, ba);
            drawDiamond(bx+ctlW,  by,       ns, baseR,baseG,baseB, ba);
            drawDiamond(bx,       by+ctlH,  ns, baseR,baseG,baseB, ba);
            drawDiamond(bx+ctlW,  by+ctlH,  ns, baseR,baseG,baseB, ba);
        }
        drawCenterS(value, bx, by + 7.0f*uiS, ctlW, 0.76f*uiS,
                    disabled ? 0.48f : 0.82f, disabled ? 0.50f : 0.90f, disabled ? 0.56f : 1.0f,
                    (disabled ? 0.52f : 0.90f)*infoA);
    };
    auto systemLogRow = [&](float y, const wchar_t* text) {
        float rowA = rightWake * (0.58f + 0.42f * panelE);
        BindMainShader();
        drawRect(rowX + 10.0f*uiS, y, rowW - 20.0f*uiS, rowH,
                 0.018f, 0.024f, 0.036f, 0.24f * rowA);
        drawRect(rowX + 24.0f*uiS, y + rowH - 1.0f*uiS,
                 rowW - 48.0f*uiS, 1.0f*uiS, baseR, baseG, baseB, 0.040f * rowA);
        g_TextS.Draw(L"[SYS_STATUS]", rowX + 24.0f*uiS, y + 9.0f*uiS,
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
        drawRect(barX, trackY-4.0f*uiS, barW, 8.0f*uiS, 0.11f,0.12f,0.16f, 0.96f*volA);
        drawRect(barX, trackY-4.0f*uiS, fillW, 8.0f*uiS, baseR,baseG,baseB, 0.90f*volA);
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
        drawRect(fieldX, by, fieldW, ctlH,
                 g_VolEdit?0.15f:0.060f, g_VolEdit?0.17f:0.068f,
                 g_VolEdit?0.22f:0.090f, 0.92f*volA);
        drawBorder(fieldX, by, fieldW, ctlH, baseR, baseG, baseB,
                   (g_VolEdit?0.72f:(fieldHover?0.38f:0.18f))*volA, 1.2f*uiS);
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
        drawRect(bx, by, ctlW, ctlH, 0.045f,0.048f,0.060f, 0.50f*meterA);
        {   // Corner brackets (disabled style)
            float ba = 0.12f * meterA;
            float cL = std::min(ctlW * 0.22f, 10.0f * uiS), ct = 1.3f * uiS;
            drawRect(bx,        by,        cL, ct, baseR,baseG,baseB, ba);
            drawRect(bx,        by,        ct, cL, baseR,baseG,baseB, ba);
            drawRect(bx+ctlW-cL,by,        cL, ct, baseR,baseG,baseB, ba);
            drawRect(bx+ctlW-ct,by,        ct, cL, baseR,baseG,baseB, ba);
            drawRect(bx,        by+ctlH-ct,cL, ct, baseR,baseG,baseB, ba);
            drawRect(bx,        by+ctlH-cL,ct, cL, baseR,baseG,baseB, ba);
            drawRect(bx+ctlW-cL,by+ctlH-ct,cL, ct, baseR,baseG,baseB, ba);
            drawRect(bx+ctlW-ct,by+ctlH-cL,ct, cL, baseR,baseG,baseB, ba);
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
        const float displayH = groupHead + rowH * 3.0f + rowGap * 2.0f + groupPadB;
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
        systemLogRow(y, SettingsText(L"[ 시스템 상태 : 자동 저장 활성화 ]", L"[ SYS_STATUS : CLOUD AUTO-SYNC ACTIVE ]"));
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
            drawRect(bx, by2, ctlW, ctlH,
                     0.28f + (rHov ? 0.08f : 0.0f), 0.04f, 0.04f, 0.92f);
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

    if (modalActive) {
        BindMainShader();
        drawRect(panelX, panelY, panelW, panelH, 0.0f, 0.0f, 0.0f, 0.52f);
    }

    if (s_resetDialog) {
        const float dW = 560.0f * uiS, dH = 380.0f * uiS;
        const float dX = (sw - dW) * 0.5f, dY = (sh - dH) * 0.5f;
        BindMainShader();
        drawRect(dX + 8.0f*uiS, dY + 10.0f*uiS, dW, dH, 0.0f, 0.0f, 0.0f, 0.32f);
        drawRect(dX, dY, dW, dH, 0.032f, 0.040f, 0.060f, 0.97f);
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
            drawRect(cbX, cbY, cbS, cbS,
                     s_resetItems[i] ? 0.60f : 0.06f,
                     s_resetItems[i] ? 0.10f : 0.06f,
                     s_resetItems[i] ? 0.10f : 0.06f, 0.94f);
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
        drawRect(execX, btnY, btnW, btnH,
                 anyItem ? (0.30f + (execHov ? 0.08f : 0.0f)) : 0.08f,
                 0.03f, 0.03f, anyItem ? 0.94f : 0.50f);
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
        drawRect(nX + 7.0f*uiS, nY + 9.0f*uiS, nW, nH, 0.0f, 0.0f, 0.0f, 0.32f);
        drawRect(nX, nY, nW, nH, 0.032f, 0.040f, 0.060f, 0.97f);
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
        drawRect(qX, qY, qW, qH, 0.28f+(qHov?0.08f:0.0f), 0.04f, 0.04f, 0.92f);
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
        drawRect(cx + 8.0f * uiS, cy + 10.0f * uiS, cw, ch, 0.0f, 0.0f, 0.0f, 0.28f);
        drawRect(cx, cy, cw, ch, 0.050f, 0.060f, 0.085f, 0.98f);
        drawRect(cx, cy, cw, 5.0f * uiS, 0.35f, 0.82f, 1.0f, 0.95f);
        drawBorder(cx, cy, cw, ch, 0.35f, 0.82f, 1.0f, 0.46f, 1.5f * uiS);
        const wchar_t* ct = L"CREDITS";
        g_TextL.Draw(ct, cx + (cw - g_TextL.Width(ct, 1.04f * uiS)) * 0.5f,
                     cy + 28.0f * uiS, 1.04f * uiS, 1.0f, 1.0f, 1.0f, 0.98f);
        const wchar_t* lines[] = {
            L"ONEDOW - Desktop Defense",
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
                SceneAppWindow(sw, sh, WW, WH, L"setting.odw", 0.70f, 0.75f, 0.88f, wx, wy,
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
                    const wchar_t* specTitle = (g_Language==Language::EN)?L"System specs (for feedback)":
                                               (g_Language==Language::JP)?L"PCスペック (フィードバック用)":
                                               L"PC 사양 (피드백용)";
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
                        L"ONEDOW  —  Desktop Defense",
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

    static float  s_EntryT    = 0.0f;
    static float  s_HoverT[4] = {};
    static int    s_ExitSel   = -1;
    static float  s_ExitT     = 0.0f;
    static double s_LastCall  = 0.0;

    const double curTime = glfwGetTime();
    if (curTime - s_LastCall > 0.12) {
        s_EntryT = 0.0f;
        for (int i = 0; i < 4; ++i) s_HoverT[i] = 0.0f;
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
    const int   NBTN    = 4;
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

    // 타이틀
    {
        float titleA = Smoothstep(std::min(s_EntryT / 0.28f, 1.0f)) * entryFade;
        const wchar_t* hdr = L"SYSTEM  PAUSED";
        float hdrSc = 1.2f;
        g_TextL.Draw(hdr, btnX0, titleY, hdrSc, 0.50f, 0.82f, 1.0f, 0.92f * titleA);
        const wchar_t* sub = L"PROCESS 0x004F : SUSPENDED";
        g_TextS.Draw(sub, btnX0, titleY + g_TextL.Height(hdr, hdrSc) + 2.0f, 0.50f,
                     0.38f, 0.60f, 0.80f, 0.60f * titleA);
        BindMainShader();
        drawRect(btnX0, titleY + g_TextL.Height(hdr, hdrSc) + 26.0f,
                 BW * 0.52f, 1.0f, 0.30f, 0.78f, 1.0f, 0.22f * titleA);
    }

    // 버튼
    struct PBtnDef { const wchar_t* route; const wchar_t* sub; };
    static const PBtnDef kPBtns[4] = {
        { L"RESUME",        L"재개"      },
        { L"CALIBRATION",   L"설정"      },
        { L"RETURN_TO_HUB", L"메인 메뉴" },
        { L"TERMINATE",     L"게임 종료"  },
    };

    const float ar = 0.48f, ag = 0.82f, ab = 1.0f;

    auto rectHit = [](float hpx, float hpy,
                      float rx, float ry, float rw, float rh) -> bool {
        return hpx >= rx && hpx <= rx + rw && hpy >= ry && hpy <= ry + rh;
    };

    for (int i = 0; i < NBTN; ++i) {
        float rawPh = (s_EntryT - 0.18f - (float)i * 0.08f) / 0.32f;
        float reveal = Smoothstep(std::max(0.0f, std::min(rawPh, 1.0f)));

        float baseBx = btnX0;
        float baseBy = btnY0 + (float)i * (BH + BGAP);
        bool hov = (!exitActive &&
                    (bool)rectHit((float)mx, (float)my, baseBx - 26.0f, baseBy, BW + 58.0f, BH));
        float spd = delta * 10.0f; if (spd > 1.0f) spd = 1.0f;
        s_HoverT[i] += ((hov ? 1.0f : 0.0f) - s_HoverT[i]) * spd;
        float t = s_HoverT[i];

        bool  selected    = (s_ExitSel == i);
        float selectPulse = selected ? (0.50f + 0.50f * sinf(fnow * 18.0f)) * exitP : 0.0f;
        float activeT     = std::max(t, selected ? exitP : 0.0f);

        float rowA = reveal * entryFade;
        if (exitActive && !selected) rowA *= (1.0f - exitP * 0.86f);
        float slide = (1.0f - reveal) * 34.0f;
        float bx    = baseBx + slide - 10.0f * t;
        float by    = baseBy + (exitActive && !selected ? exitP * 28.0f : 0.0f);

        BindMainShader();
        DrawMenuCommandFeedback(bx, by, BW, BH, ar, ag, ab,
                                rowA, activeT, selectPulse, fnow + (float)i * 0.17f);

        float routeSc = 1.04f;
        while (routeSc > 0.82f && g_TextL.Width(kPBtns[i].route, routeSc) > BW)
            routeSc -= 0.04f;
        float routeY = by + 2.0f;
        float subY   = routeY + g_TextL.Height(kPBtns[i].route, routeSc) - 3.0f;
        float tr = 1.0f + (ar - 1.0f) * activeT;
        float tg = 1.0f + (ag - 1.0f) * activeT;
        float tb = 1.0f + (ab - 1.0f) * activeT;
        if (selected) { tr = 1.0f; tg = 1.0f; tb = 1.0f; }
        g_TextL.Draw(kPBtns[i].route, bx, routeY, routeSc,
                     tr, tg, tb, (0.82f + 0.16f * activeT + 0.18f * selectPulse) * rowA);
        g_TextS.Draw(kPBtns[i].sub, bx + 4.0f, subY, 0.48f,
                     0.70f + 0.18f * activeT, 0.75f + 0.16f * activeT, 0.82f + 0.12f * activeT,
                     (0.70f + 0.20f * activeT) * rowA);

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
        case 0: g_GameManager.currentState = GameState::RUNNING; break;
        case 1:
            g_SettingsReturnTo = GameState::PAUSED;
            ResetSettingsUi();
            g_GameManager.currentState = GameState::SETTINGS;
            break;
        case 2: g_GameManager.currentState = GameState::MAIN_MENU; break;
        case 3: glfwSetWindowShouldClose(window, GLFW_TRUE); break;
        }
        return;
    }

    {
        float hintA = Smoothstep(std::min(std::max(0.0f, s_EntryT - 0.50f) / 0.30f, 1.0f)) * entryFade;
        const wchar_t* hint = L"[SPACE / ESC]  재개";
        g_TextS.Draw(hint, btnX0, sh * 0.88f, 0.58f,
                     0.42f, 0.62f, 0.82f, 0.55f * hintA);
    }
}
void Scene_GameOver(const SceneCtx& c) {
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
                drawRect(0, 0, sw, sh, 0.02f, 0.02f, 0.06f, 0.88f * ge * SCENE_BG_ALPHA);
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

                // 버튼 3개: 다시하기 / 메뉴로 / 종료 — 페이드 완료 후에만 표시/활성
                if (gof >= 0.999f) {
                    const float BW = 240.0f, BH = 56.0f, BG = 16.0f;
                    float totalW = 3 * BW + 2 * BG;
                    float bx = (sw - totalW) * 0.5f;
                    float by = sh * 0.72f;

                    if (UIButton(bx + 0*(BW+BG), by, BW, BH, T(StrId::BTN_RESTART),
                                 mx, my, lmb, g_LmbPrev)) {
                        ResetForNewGame();
                        FinalizeLoadout(c, FixedWeaponForSelectedJob());
                    }
                    if (UIButton(bx + 1*(BW+BG), by, BW, BH, T(StrId::BTN_MAIN_MENU),
                                 mx, my, lmb, g_LmbPrev)) {
                        g_GameManager.currentState = GameState::MAIN_MENU;
                    }
                    if (UIButton(bx + 2*(BW+BG), by, BW, BH, T(StrId::BTN_QUIT),
                                 mx, my, lmb, g_LmbPrev)) {
                        glfwSetWindowShouldClose(window, GLFW_TRUE);
                    }
                }
}

void Scene_Victory(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    GLFWwindow* window = c.window;
    const std::function<void()>& ResetForNewGame = c.reset;

    float vf = g_VictoryFade;
    float ve = Smoothstep(vf);

    BindMainShader();
    drawRect(0, 0, sw, sh, 0.02f, 0.06f, 0.04f, 0.88f * ve * SCENE_BG_ALPHA);

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

    if (vf >= 0.999f) {
        const float BW = 240.0f, BH = 56.0f, BG = 16.0f;
        float totalW = 3 * BW + 2 * BG;
        float bx = (sw - totalW) * 0.5f;
        float by = sh * 0.72f;
        if (UIButton(bx + 0*(BW+BG), by, BW, BH, T(StrId::BTN_RESTART),
                     mx, my, lmb, g_LmbPrev)) {
            ResetForNewGame();
            FinalizeLoadout(c, FixedWeaponForSelectedJob());
        }
        if (UIButton(bx + 1*(BW+BG), by, BW, BH, T(StrId::BTN_MAIN_MENU),
                     mx, my, lmb, g_LmbPrev)) {
            g_GameManager.currentState = GameState::MAIN_MENU;
        }
        if (UIButton(bx + 2*(BW+BG), by, BW, BH, T(StrId::BTN_QUIT),
                     mx, my, lmb, g_LmbPrev)) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }
}

void Scene_AugSelect(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const float delta = c.delta;
    const GameState st = g_GameManager.currentState;

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

    struct SNPart { float x,y,vx,vy,life; float r,g,b; bool active; };
    static constexpr int SN_MAX = 72;
    static SNPart s_sn[SN_MAX];
    static bool   s_snSpawned = false;

    // 진입 리셋
    {
        const GameState prev = g_GameManager.lastState;
        if (prev != GameState::AUG_SELECT && prev != GameState::DEBUFF_SELECT) {
            s_spawnT    = 0.0f;
            s_prevHov   = -1;
            s_panelA    = 0.0f;
            s_snSpawned = false;
            for (int i=0; i<3; i++) { s_dimT[i]=1.f; s_flashT[i]=0.f; s_pullT[i]=0.f; }
            s_lmbPrev = false;
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

    if (g_HoveredAug != s_prevHov && g_HoveredAug >= 0 && g_HoveredAug < 3)
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

    // ── 마우스 클릭 판정 (궤도 기반) ──
    {
        bool lmbClick = c.lmb && !s_lmbPrev;
        if (lmbClick && !inExit && inFocus) {
            int   best  = -1;
            float bestD2 = 1e9f;
            for (int i = 0; i < 3; i++) {
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
                if (g_HoveredAug == best && g_AugExitT < 0.0f) {
                    // 항성(이미 선택) 재클릭 → 확정
                    g_AugExitT    = 0.0f;
                    g_AugExitSlot = best;
                } else {
                    g_HoveredAug = best;
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
            for (int _i=0; _i<3; _i++) OrbPos(_i, ox[_i], oy[_i]);
            drawSeg(ox[0],oy[0], ox[1],oy[1], 1.4f, 0.30f,0.62f,0.92f, lineA*0.20f);
            drawSeg(ox[1],oy[1], ox[2],oy[2], 1.4f, 0.30f,0.62f,0.92f, lineA*0.20f);
            drawSeg(ox[2],oy[2], ox[0],oy[0], 1.4f, 0.30f,0.62f,0.92f, lineA*0.20f);
            BatchFlush();
        }
    }

    // ── 별자리 3개 렌더 ──
    static const wchar_t* KEY_LABELS[3] = { L"[ 1 ]", L"[ 2 ]", L"[ 3 ]" };

    for (int i=0; i<3; i++) {
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
            g_TextS.Draw(KEY_LABELS[i], px - klW*0.5f, py + BASE_R + 30.0f,
                         0.76f, 0.58f,0.80f,1.00f, kA);
            BatchFlush();
        }
    }

    // ── 우측 데이터 태그 패널 ──
    {
        if (s_panelA > 0.01f && !inExit) {
            int idx = (g_HoveredAug>=0 && g_HoveredAug<3) ? g_HoveredAug : 0;
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
                while (dsc > 0.44f && g_TextS.Width(desc, dsc) > PW) dsc -= 0.04f;
                wchar_t descLine[256];
                swprintf_s(descLine, L"DESC: %ls", desc);
                g_TextS.Draw(descLine, PX, ty, dsc, 0.78f,0.88f,0.96f, 0.82f*pA);
                ty += dsc * 22.0f + 4.0f;
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
            int confIdx = (g_AugExitSlot>=0) ? g_GameManager.augChoices[g_AugExitSlot] : 0;
            float pr, pg2, pb;
            RarColor(ALL_AUGS[confIdx].rarity, pr, pg2, pb);
            float spawnX = ORB_CX, spawnY = ORB_CY;
            if (g_AugExitSlot>=0&&g_AugExitSlot<3) OrbPos(g_AugExitSlot, spawnX, spawnY);
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
void Scene_AugReplace(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const float delta = c.delta;
    int newIdx = g_GameManager.pendingAugIdx;
    if (newIdx < 0 || newIdx >= AUG_TOTAL) return;

    // ── 애니메이션 상태 ──
    static float s_enterT      = 0.0f;
    static float s_hovY[32]    = {};
    static float s_pickFlash[32] = {};
    static int   s_prevHov     = -1;

    auto easeOut = [](float t) -> float {
        float inv = 1.0f - t; return 1.0f - inv * inv * inv;
    };

    // 새 진입 감지
    const GameState prev = g_GameManager.lastState;
    if (prev != GameState::AUG_REPLACE) {
        s_enterT = 0.0f;
        std::fill(std::begin(s_hovY),      std::end(s_hovY),      0.f);
        std::fill(std::begin(s_pickFlash), std::end(s_pickFlash), 0.f);
        s_prevHov = -1;
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

void Scene_RunShop(const SceneCtx& c) {
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
        st = g_CreativeMode ? GS::CREATIVE_CONFIG : GS::RUN_CONFIG;
        return true;
    case GS::RUN_CONFIG:
        st = GS::MAIN_MENU;
        return true;
    case GS::CREATIVE_CONFIG:
        st = GS::RUN_CONFIG;
        return true;
    case GS::SETTINGS:
        SaveGame();
        st = g_SettingsReturnTo;
        return true;
    default:
        return false;
    }
}
