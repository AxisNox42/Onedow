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

#include "SceneInternal.h"

float g_SettingsEntryT = 0.0f;
bool s_SettingsInlineNeedsReset = false;

void ResetSettingsUi(float entryStart) {
    g_SettingsEntryT = std::max(0.0f, std::min(1.0f, entryStart));
    s_SettingsInlineNeedsReset = true;
}


void Scene_SettingsInline(const SceneCtx& c) {
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
        int backdropBlurCaptureHz = 20;
        int soundVol = 100;
        bool bgmEnabled = true;
        bool sfxEnabled = true;
        bool audioMonoOutput = false;
        bool audioEngineEnabled = true;
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
        entry = g_SettingsEntryT;
        exitT = 0.0f;
        exiting = false;
        prevEsc = false;
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
                          g_ShowCombo, g_BackdropBlurEnabled, g_BackdropBlurCaptureHz,
                          g_SoundVol, g_BgmEnabled, g_SfxEnabled,
                          g_AudioMonoOutput, g_AudioEngineEnabled,
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
    g_SettingsEntryT = entry;
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
        g_BackdropBlurCaptureHz = savedSettings.backdropBlurCaptureHz;
        g_SoundVol = savedSettings.soundVol;
        g_BgmEnabled = savedSettings.bgmEnabled;
        g_SfxEnabled = savedSettings.sfxEnabled;
        g_AudioMonoOutput = savedSettings.audioMonoOutput;
        g_AudioEngineEnabled = savedSettings.audioEngineEnabled;
        Audio::SetBgmEnabled(g_BgmEnabled);
        Audio::SetSfxEnabled(g_SfxEnabled);
        Audio::SetEnabled(g_AudioEngineEnabled && g_SoundVol > 0);
        Audio::SetVolume(g_SoundVol / 100.0f);
        Audio::SetMonoOutput(g_AudioMonoOutput);
        g_AudioMonoOutput = Audio::IsMonoOutput();
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
            || g_BackdropBlurCaptureHz != savedSettings.backdropBlurCaptureHz
            || g_SoundVol != savedSettings.soundVol
            || g_BgmEnabled != savedSettings.bgmEnabled
            || g_SfxEnabled != savedSettings.sfxEnabled
            || g_AudioMonoOutput != savedSettings.audioMonoOutput
            || g_AudioEngineEnabled != savedSettings.audioEngineEnabled
            || g_AutoFire != savedSettings.autoFire
            || g_AutoSkill != savedSettings.autoSkill
            || g_ShowCrosshair != savedSettings.crosshair
            || g_DebugMode != savedSettings.debugMode
            || g_Language != savedSettings.language;
    };

    const bool esc = c.window && glfwGetKey(c.window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    const bool rmb = c.window && glfwGetMouseButton(c.window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    if (c.inputFocusChanged) prevEsc = esc;
    if (c.inputFocusChanged) s_volDrag = false;
    const bool backInput = (esc && !prevEsc) || (rmb && !g_RmbPrev);
    prevEsc = esc;
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

    const float uiS = UiScale(sw, sh);
    auto settingsLevel = [&](TextRenderer& renderer) {
        return &renderer == &g_TextL
            ? UiTextLevel::Description : UiTextLevel::Supporting;
    };
    auto settingsScale = [&](TextRenderer& renderer, float scale,
                             UiTextLevel level) {
        return UiTextScale(renderer, level, scale * uiS);
    };
    auto drawSettingsText = [&](TextRenderer& renderer, const wchar_t* text,
                                float x, float y, float scale,
                                float r, float g, float b, float alpha,
                                float shadowAlpha = 0.68f) {
        DrawShadowedText(renderer, text, x, y,
                         settingsScale(renderer, scale, settingsLevel(renderer)),
                         r, g, b, alpha, shadowAlpha);
    };
    auto settingsButtonTextWidth = [&](const wchar_t* text, float scale,
                                       UiTextLevel level = UiTextLevel::Title) {
        return g_TextL.Width(text,
            settingsScale(g_TextL, scale, level));
    };
    auto drawSettingsTextAtLevel = [&](TextRenderer& renderer, const wchar_t* text,
                                       float x, float y, float scale,
                                       float r, float g, float b, float alpha,
                                       UiTextLevel level, float shadowAlpha) {
        DrawShadowedText(renderer, text, x, y,
                         settingsScale(renderer, scale, level),
                         r, g, b, alpha, shadowAlpha);
    };
    const float mainBH = 70.0f * uiS, mainGap = 15.0f * uiS;
    const float mainX = BottomLeftActionX(uiS);
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

    const float settingsTitleY = 42.0f * uiS;
    drawSettingsTextAtLevel(g_TextL, korean ? L"설정" : L"SETTINGS", mainX, settingsTitleY,
                            1.0f, 1.0f, 1.0f, 1.0f, 0.94f * treeA,
                            UiTextLevel::Title, 0.72f);
    drawSettingsTextAtLevel(g_TextS, korean ? L"시스템 보정" : L"SYSTEM CALIBRATION",
                            mainX + 4.0f * uiS,
                            settingsTitleY + g_TextL.Height(
                                korean ? L"설정" : L"SETTINGS",
                                settingsScale(g_TextL, 1.0f, UiTextLevel::Title)) + 8.0f * uiS,
                            1.0f, 0.35f, 0.76f, 1.0f, 0.72f * treeA,
                            UiTextLevel::Subtitle, 1.0f);

    // \uC88C\uCE21 ghost \uBC84\uD2BC \u2014 \uCEE8\uD14D\uC2A4\uD2B8\uC5D0 \uB530\uB77C \uBA54\uC778\uBA54\uB274 vs \uC77C\uC2DC\uC815\uC9C0 \uBA54\uB274
    if (s_MainMenuSettingsPanel) {
        const float mainMenuGap = 10.0f * uiS;
        const wchar_t* menu[5] = { korean ? L"플레이" : L"PLAY",
                                   korean ? L"상점" : L"SHOP",
                                   korean ? L"성도 기록" : L"ASTRAL_LOG",
                                   korean ? L"설정" : L"SETTING",
                                   korean ? L"종료" : L"EXIT" };
        for (int i = 0; i < 5; ++i) {
            const float y = MainMenuButtonRailStartY(sh, uiS) + i * (mainBH + mainMenuGap);
            const bool focus = i == 3;
            const float x = mainX - 250.0f * (1.0f - oldA) - (focus ? 0.0f : 24.0f * (1.0f - oldA));
            const float a = (focus ? 0.82f : 0.34f) * oldA;
            const wchar_t* route = MainMenuRouteLabel(LangIndex(), i);
            const float routeScale = settingsScale(
                g_TextL, 1.0f, UiTextLevel::Title);
            const float routeY = y + (mainBH - g_TextL.Height(route, routeScale)) * 0.5f;
            drawSettingsTextAtLevel(g_TextL, route, x, routeY, 1.0f,
                                    1.0f, 1.0f, 1.0f, a,
                                    UiTextLevel::Title, 0.70f);
        }
    } else if (!inGameSettings) {
        const wchar_t* menu[4] = { korean ? L"계속하기" : L"CONTINUE",
                                   korean ? L"설정" : L"CALIBRATION",
                                   korean ? L"\uD3EC\uAE30\uD558\uAE30" : L"ABANDON RUN",
                                   korean ? L"종료" : L"TERMINATE" };
        for (int i = 0; i < 4; ++i) {
            const float y = MainMenuButtonRailStartY(sh, uiS) + i * (mainBH + mainGap);
            const bool focus = i == 1;
            const float x = mainX - 250.0f * (1.0f - oldA) - (focus ? 0.0f : 24.0f * (1.0f - oldA));
            const float a = (focus ? 0.82f : 0.34f) * oldA;
            const float routeScale = settingsScale(
                g_TextL, 1.0f, UiTextLevel::Title);
            const float routeH = g_TextL.Height(menu[i], routeScale);
            drawSettingsTextAtLevel(g_TextL, menu[i], x,
                                    y + (mainBH - routeH) * 0.5f, 1.0f,
                                    1.0f, 1.0f, 1.0f, a,
                                    UiTextLevel::Title, 0.70f);
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

    auto activateSettingsCategory = [&](int nextCategory, int nextRow) {
        if (nextCategory < 0 || nextCategory >= 4) return;
        const bool categoryChanged = tab != nextCategory;
        tab = nextCategory;
        detailRow = std::max(0, nextRow);
        if (!categoryChanged) return;

        settingsScroll = 0.0f;
        settingsScrollTarget = 0.0f;
        pulse = 1.0f;
        pulseRow = -1;
        tabSwitchT = 0.0f;
        for (auto& category : rowHover)
            for (auto& h : category) h = 0.0f;
        for (auto& category : optHover)
            for (auto& row : category)
                for (auto& h : row) h = 0.0f;
    };

    // The integrated list below is the only settings navigation surface.
    // The former orbit-tab controls were non-interactive and duplicated it.
    // The open field intentionally has no active-tab bridge line.

    // \u2550\u2550\u2550 RIGHT PANEL \u2014 SETTINGS ROWS \u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550
    const float dA   = treeA * (0.78f + 0.22f * (1.0f - pulse));
    // The settings body owns the full area to the right of the category
    // rail, split into a readable list column and a dedicated readout column.
    const float rpX  = depthX;
    const float rpW  = detailX + detailW - rpX;
    const float categoryReveal = Smoothstep(
        LogoClamp01(tabSwitchT / 0.22f));

    // Keep the context line and category title vertically centered as one
    // measured block between the top of the right tab and its separator.
    const wchar_t* settingsContext = korean
        ? L"관측소 보정" : L"OBSERVATORY CALIBRATION";
    const wchar_t* categoryTitle = korean ? tabs[tab].kr : tabs[tab].en;
    const float contextScale = settingsScale(
        g_TextS, 1.0f, UiTextLevel::Supporting);
    const float categoryScale = settingsScale(
        g_TextL, 1.0f, UiTextLevel::Subtitle);
    const float contextH = g_TextS.Height(settingsContext, contextScale);
    const float categoryH = g_TextL.Height(categoryTitle, categoryScale);
    const float headerGap = 4.0f * uiS;
    const float headerBlockH = contextH + headerGap + categoryH;
    const float headerAreaH = 68.0f * uiS;
    const float headerBlockY = depthY
        + std::max(0.0f, (headerAreaH - headerBlockH) * 0.5f);
    drawSettingsTextAtLevel(g_TextS, settingsContext, rpX, headerBlockY,
                            1.0f, 0.48f, 0.82f, 1.0f, 0.70f * dA,
                            UiTextLevel::Supporting, 0.66f);
    drawSettingsTextAtLevel(g_TextL, categoryTitle, rpX,
                            headerBlockY + contextH + headerGap,
                            1.0f, tabR, tabG, tabB,
                            0.90f * dA * categoryReveal,
                            UiTextLevel::Subtitle, 0.68f);
    const float sepY = depthY + headerAreaH;
    LogoLine(rpX, sepY, rpX + rpW * 0.92f, sepY,
             0.76f * uiS, tabR, tabG, tabB,
             0.22f * dA * categoryReveal);

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
    const int rowCounts[4] = { 5, 5, kDebugSettingsVisible ? 4 : 3, 2 };

    static wchar_t s_volBuf[8];

    // \u2500\u2500 Render rows \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500
    // Keep the setting state for every category; the right panel renders only
    // the rows belonging to the currently selected category.
    swprintf_s(s_volBuf, L"%d", g_SoundVol);
    const int allLangCur = g_Language == Language::KR ? 0 :
                           g_Language == Language::EN ? 1 : 2;
    const int allFpsCur =
        g_FpsCap == 0 ? 0 : g_FpsCap == 30 ? 1 : g_FpsCap == 60 ? 2 :
        g_FpsCap == 144 ? 3 : g_FpsCap == 300 ? 4 : 5;
    static const wchar_t* const kToggleLabels[3][2] = {
        { L"켜짐", L"꺼짐" }, { L"ON", L"OFF" }, { L"オン", L"オフ" }
    };
    const int toggleLanguage = std::max(0, std::min(2, LangIndex()));
    const wchar_t* const toggleOn = kToggleLabels[toggleLanguage][0];
    const wchar_t* const toggleOff = kToggleLabels[toggleLanguage][1];
    settingsRows[0][0] = { korean ? L"FPS 제한" : L"FPS CAP", nullptr, false, false, false, false,
                           { korean ? L"동기화" : L"VSYNC", L"30", L"60", L"144", L"300",
                             korean ? L"무제한" : L"UNLIM" }, 6, allFpsCur };
    settingsRows[0][1] = { korean ? L"그래픽 품질" : L"GRAPHICS", nullptr, false, false, false, false,
                           { korean ? L"전체" : L"FULL", korean ? L"감소" : L"REDUCED" },
                           2, g_VfxDensity == VfxDensity::FULL ? 0 : 1 };
    settingsRows[0][2] = { korean ? L"콤보 HUD" : L"COMBO HUD", nullptr, false, false, false, false,
                           { toggleOn, toggleOff }, 2, g_ShowCombo ? 0 : 1 };
    settingsRows[0][3] = { korean ? L"배경 블러" : L"BACKDROP BLUR", nullptr, false, false, false, false,
                           { toggleOff, toggleOn },
                           2, g_BackdropBlurEnabled ? 1 : 0 };
    settingsRows[0][4] = { korean ? L"블러 갱신 주기" : L"BLUR REFRESH RATE", nullptr, false, false, false, false,
                           { L"1", L"20", L"60" }, 3,
                           g_BackdropBlurCaptureHz == 1 ? 0 :
                           g_BackdropBlurCaptureHz == 60 ? 2 : 1 };
    settingsRows[1][0] = { korean ? L"마스터 볼륨" : L"MASTER VOL", s_volBuf, false, true, false, false };
    settingsRows[1][1] = { korean ? L"BGM 채널" : L"BGM BUS", nullptr, false, false, false, false,
                           { toggleOn, toggleOff }, 2, g_BgmEnabled ? 0 : 1 };
    settingsRows[1][2] = { korean ? L"효과음 채널" : L"SFX BUS", nullptr, false, false, false, false,
                           { toggleOn, toggleOff }, 2, g_SfxEnabled ? 0 : 1 };
    settingsRows[1][3] = { korean ? L"출력" : L"OUTPUT", nullptr, false, false, false, false,
                           { korean ? L"스테레오" : L"STEREO",
                             korean ? L"모노" : L"MONO" },
                           2, g_AudioMonoOutput ? 1 : 0 };
    settingsRows[1][4] = { korean ? L"오디오 엔진" : L"AUDIO ENGINE", nullptr, false, false, false, false,
                           { toggleOn, toggleOff }, 2,
                           g_AudioEngineEnabled ? 0 : 1 };
    settingsRows[2][0] = { korean ? L"자동 발사" : L"AUTO FIRE", nullptr, false, false, false, false,
                           { toggleOn, toggleOff }, 2, g_AutoFire ? 0 : 1 };
    settingsRows[2][1] = { korean ? L"자동 스킬" : L"AUTO SKILL", nullptr, false, false, false, false,
                           { toggleOn, toggleOff }, 2, g_AutoSkill ? 0 : 1 };
    settingsRows[2][2] = { korean ? L"조준선" : L"CROSSHAIR", nullptr, false, false, false, false,
                           { toggleOn, toggleOff }, 2, g_ShowCrosshair ? 0 : 1 };
    settingsRows[2][3] = { korean ? L"\uB514\uBC84\uAE45 \uBAA8\uB4DC" : L"DEBUG MODE", nullptr, false, false, false, false,
                           { toggleOn, toggleOff }, 2, g_DebugMode ? 0 : 1 };
    settingsRows[3][0] = { korean ? L"언어" : L"LANGUAGE", nullptr, false, false, false, false,
                           { korean ? L"한국어" : L"KOR", korean ? L"영어" : L"ENG", korean ? L"일본어" : L"JPN" },
                           3, allLangCur };
    settingsRows[3][1] = { korean ? L"데이터 초기화" : L"RESET DATA", nullptr, false, false, true, true };

    auto settingsOptionTextLevel = [&](int category, int row) {
        const SRow& setting = settingsRows[category][row];
        const bool isToggle = setting.optCount == 2
            && ((setting.opts[0] == toggleOn && setting.opts[1] == toggleOff)
                || (setting.opts[0] == toggleOff && setting.opts[1] == toggleOn));
        return isToggle ? UiTextLevel::Subtitle : UiTextLevel::Title;
    };

    auto settingChanged = [&](int category, int row) {
        if (!savedSettingsValid) return false;
        if (category == 0) {
            if (row == 0) return g_FpsCap != savedSettings.fps;
            if (row == 1) return g_VfxDensity != savedSettings.density;
            if (row == 2) return g_ShowCombo != savedSettings.combo;
            if (row == 3) return g_BackdropBlurEnabled != savedSettings.backdropBlur;
            if (row == 4) return g_BackdropBlurCaptureHz != savedSettings.backdropBlurCaptureHz;
        } else if (category == 1) {
            if (row == 0) return g_SoundVol != savedSettings.soundVol;
            if (row == 1) return g_BgmEnabled != savedSettings.bgmEnabled;
            if (row == 2) return g_SfxEnabled != savedSettings.sfxEnabled;
            if (row == 3) return g_AudioMonoOutput != savedSettings.audioMonoOutput;
            if (row == 4) return g_AudioEngineEnabled != savedSettings.audioEngineEnabled;
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

    // The left catalogue keeps every category visible and expands only the
    // selected one. The right panel uses that same selection.
    struct SettingsListEntry { int category; int row; bool header; const wchar_t* label; };
    SettingsListEntry list[32] = {};
    int listCount = 0;
    auto addHeader = [&](int cat, const wchar_t* label) {
        list[listCount++] = { cat, -1, true, label };
    };
    auto addItem = [&](int cat, int row, const wchar_t* label) {
        list[listCount++] = { cat, row, false, label };
    };
    const wchar_t* displayLabels[5] = {
        korean ? L"FPS 제한" : L"FPS CAP",
        korean ? L"그래픽 품질" : L"GRAPHICS",
        korean ? L"콤보 HUD" : L"COMBO HUD",
        korean ? L"배경 블러" : L"BACKDROP BLUR",
        korean ? L"블러 갱신 주기" : L"BLUR REFRESH RATE" };
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
    // the selected category expands its controls. This keeps navigation
    // stable while the right panel changes to the selected category.
    addHeader(0, korean ? L"화면" : L"DISPLAY");
    if (tab == 0) for (int i = 0; i < 5; ++i) addItem(0, i, displayLabels[i]);
    addHeader(1, korean ? L"소리" : L"AUDIO");
    if (tab == 1) for (int i = 0; i < 5; ++i) addItem(1, i, audioLabels[i]);
    addHeader(2, korean ? L"게임플레이" : L"GAMEPLAY");
    if (tab == 2) for (int i = 0; i < (kDebugSettingsVisible ? 4 : 3); ++i)
        addItem(2, i, gameplayLabels[i]);
    addHeader(3, korean ? L"기록" : L"ARCHIVE");
    if (tab == 3) for (int i = 0; i < 2; ++i) addItem(3, i, archiveLabels[i]);

    SettingsListEntry rightList[32] = {};
    int rightListCount = 0;
    auto addRightItem = [&](int cat, int row, const wchar_t* label) {
        rightList[rightListCount++] = { cat, row, false, label };
    };
    const wchar_t* const* activeLabels = displayLabels;
    if (tab == 1) activeLabels = audioLabels;
    else if (tab == 2) activeLabels = gameplayLabels;
    else if (tab == 3) activeLabels = archiveLabels;
    for (int i = 0; i < rowCounts[tab]; ++i)
        addRightItem(tab, i, activeLabels[i]);

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
    const float listH = std::max(1.0f, listBottom - listTop);
    const float listRowH = 52.0f * uiS;
    // The rendered y positions are pixels, so their scroll range must be
    // pixels too.  Row-count values here capped the previous list movement
    // to only a handful of pixels.
    const float leftMaxScroll = std::max(0.0f,
        (float)listCount * listRowH - listH);
    const float rightListTop = sepY + 34.0f * uiS;
    const float rightListBottom = sh - 206.0f * uiS;
    const float rightListH = std::max(1.0f, rightListBottom - rightListTop);
    const float rightRowH = 112.0f * uiS;
    const float rightMaxScroll = std::max(0.0f,
        (float)rightListCount * rightRowH - rightListH);
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
    const bool categoryInteractionReady = tabSwitchT >= 0.22f;
    const bool overRightList = inputReady && categoryInteractionReady
                            && mx >= rpX - 10.0f * uiS
                            && mx < rpX + rClickW
                            && my >= rightListTop && my < rightListBottom;
    // Let the wheel scroll the visible settings area, including the left
    // catalogue and empty space around shorter categories.
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
    if (c.inputFocusChanged) {
        prevListW = listWKey;
        prevListS = listSKey;
    }
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
        int rightFocus = -1;
        if (isListItem(listCursor)) {
            const SettingsListEntry& focused = list[listCursor];
            for (int i = 0; i < rightListCount; ++i) {
                if (!rightList[i].header && rightList[i].category == focused.category
                    && rightList[i].row == focused.row) {
                    rightFocus = i;
                    break;
                }
            }
        }
        if (rightFocus < 0) return;
        const float target = std::max(0.0f, std::min(rightMaxScroll,
            (float)rightFocus * rightRowH - rightListH * 0.42f));
        settingsScrollTarget = target / rightMaxScroll;
    };
    if (focusChanged) revealFocus();
    if (isListItem(listCursor)) {
        const SettingsListEntry& selectedEntry = list[listCursor];
        if (selectedEntry.category != tab || selectedEntry.row != detailRow) {
            activateSettingsCategory(selectedEntry.category,
                                      selectedEntry.row);
            focusChanged = true;
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
                activateSettingsCategory(list[i].category, 0);
                listCursor = -1;
                focusChanged = true;
                continue;
            }
            if (!list[targetIndex].header) {
                listCursor = targetIndex;
                detailRow = list[targetIndex].row;
                focusChanged = true;
                if (tab != list[targetIndex].category) {
                    activateSettingsCategory(list[targetIndex].category,
                                             list[targetIndex].row);
                }
            }
        }
        listHover[i] = UpdateMenuCommandHover(listHover[i], hov && !list[i].header, dt);
        if (list[i].header) {
            const float headerX = railX + 20.0f * uiS;
            const bool activeCategory = list[i].category == tab;
            const bool categoryHover = hov;
            if (activeCategory || categoryHover)
                drawRect(headerX - 10.0f * uiS,
                         y + 2.0f * uiS,
                         listW * 0.78f,
                         listRowH - 5.0f * uiS,
                         0.12f, 0.42f, 0.72f,
                         (activeCategory ? 0.10f : 0.045f) * dA);
            const float categoryScale = settingsScale(
                g_TextS, 1.0f, UiTextLevel::Subtitle);
            const float categoryH = g_TextS.Height(list[i].label, categoryScale);
            drawSettingsTextAtLevel(
                g_TextS, list[i].label, headerX,
                y + (listRowH - categoryH) * 0.5f, 1.0f,
                activeCategory || categoryHover ? 0.58f : 0.34f,
                activeCategory || categoryHover ? 0.86f : 0.72f,
                1.0f,
                (activeCategory ? 1.0f :
                 categoryHover ? 0.92f : 0.68f) * dA,
                UiTextLevel::Subtitle, 0.56f);
            LogoLine(headerX, y + listRowH - 2.0f * uiS,
                     listX + listW * 0.78f, y + listRowH - 2.0f * uiS,
                     activeCategory ? 1.15f * uiS : 0.7f * uiS,
                     0.34f, 0.72f, 1.0f,
                     (activeCategory ? 0.38f : 0.16f) * dA);
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
            const float itemTextX = itemX + 20.0f * uiS;
            float itemTextScale = settingsScale(
                g_TextL, 1.0f, UiTextLevel::Description);
            const float itemTextMaxW = std::max(
                1.0f, listX + listW - itemTextX - 8.0f * uiS);
            const float itemTextW = g_TextL.Width(list[i].label, itemTextScale);
            if (itemTextW > itemTextMaxW && itemTextW > 0.0f)
                itemTextScale *= itemTextMaxW / itemTextW;
            const float itemTextH = g_TextL.Height(list[i].label, itemTextScale);
            drawSettingsTextAtLevel(
                g_TextL, list[i].label, itemTextX,
                y + (listRowH - itemTextH) * 0.5f, 1.0f,
                1.0f, changed ? 0.68f : 1.0f,
                changed ? 0.32f : 1.0f, textA * dA,
                UiTextLevel::Description, 0.62f);
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


    // The right settings board is scoped to the category selected on the left.
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
                return kToggleLabels[optionLanguage][optionIndex];
            }
            if (row == 3) {
                return kToggleLabels[optionLanguage][1 - optionIndex];
            }
            if (row == 4) {
                static const wchar_t* rates[3][3] = {
                    { L"1", L"20", L"60" },
                    { L"1", L"20", L"60" },
                    { L"1", L"20", L"60" }
                };
                return rates[optionLanguage][optionIndex];
            }
        } else if (category == 1) {
            if (row == 1 || row == 2 || row == 4)
                return kToggleLabels[optionLanguage][optionIndex];
            if (row == 3) {
                static const wchar_t* output[3][2] = {
                    { L"스테레오", L"모노" },
                    { L"STEREO", L"MONO" },
                    { L"ステレオ", L"モノ" }
                };
                return output[optionLanguage][optionIndex];
            }
        } else if (category == 2 && row < 4) {
            return kToggleLabels[optionLanguage][optionIndex];
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

    auto localizedOptionWidth = [&](int category, int row, int optionIndex,
                                    float scale, const wchar_t* fallback) {
        const UiTextLevel level = settingsOptionTextLevel(category, row);
        float widest = settingsButtonTextWidth(fallback, scale, level);
        for (int language = 0; language < 3; ++language) {
            const wchar_t* localized = optionLabelForLanguage(
                category, row, language, optionIndex);
            if (localized)
                widest = std::max(widest,
                                  settingsButtonTextWidth(localized, scale,
                                                          level));
        }
        return widest;
    };

    const float commonOptionSc = 1.0f;
    const float commonChipPadX = 8.0f * uiS;
    const float commonChipGap = 12.0f * uiS;
    float commonOptionCellW[8] = {};
    int commonOptionCount = 0;
    for (int category = 0; category < 4; ++category) {
        for (int row = 0; row < 6; ++row) {
            if (category == 0 && row == 0) continue;
            const SRow& setting = settingsRows[category][row];
            if (setting.optCount <= 0) continue;
            commonOptionCount = std::max(commonOptionCount, setting.optCount);
            for (int j = 0; j < setting.optCount; ++j) {
                const float widest = localizedOptionWidth(
                    category, row, j, commonOptionSc, setting.opts[j]);
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
        std::fill(std::begin(commonOptionCellW), std::end(commonOptionCellW), 0.0f);
        for (int category = 0; category < 4; ++category) {
            for (int row = 0; row < 6; ++row) {
                if (category == 0 && row == 0) continue;
                const SRow& setting = settingsRows[category][row];
                for (int j = 0; j < setting.optCount; ++j) {
                    const float widest = localizedOptionWidth(
                        category, row, j, commonOptionScale, setting.opts[j]);
                    commonOptionCellW[j] = std::max(
                        commonOptionCellW[j], widest + commonChipPadX * 2.0f);
                }
            }
        }
    }

    float fpsOptionScale = commonOptionSc;
    const float fpsChipPadX = 4.0f * uiS;
    const float fpsChipGap = 16.0f * uiS;
    float fpsOptionCellW[8] = {};
    float fpsOptionTotalW = 0.0f;
    for (int j = 0; j < settingsRows[0][0].optCount; ++j) {
        fpsOptionCellW[j] = localizedOptionWidth(
                                0, 0, j, fpsOptionScale,
                                settingsRows[0][0].opts[j])
                          + fpsChipPadX * 2.0f;
        fpsOptionTotalW += fpsOptionCellW[j];
    }
    if (settingsRows[0][0].optCount > 1)
        fpsOptionTotalW += fpsChipGap * (settingsRows[0][0].optCount - 1);
    if (fpsOptionTotalW > rightControlW && fpsOptionTotalW > 1.0f) {
        fpsOptionScale *= std::max(0.55f, rightControlW / fpsOptionTotalW);
        for (int j = 0; j < settingsRows[0][0].optCount; ++j) {
            fpsOptionCellW[j] = localizedOptionWidth(
                                    0, 0, j, fpsOptionScale,
                                    settingsRows[0][0].opts[j])
                              + fpsChipPadX * 2.0f;
        }
    }

    float languageOptionScale = commonOptionSc;
    float languageOptionCellW[8] = {};
    float languageOptionTotalW = 0.0f;
    for (int j = 0; j < settingsRows[3][0].optCount; ++j) {
        const float widest = localizedOptionWidth(
            3, 0, j, languageOptionScale, settingsRows[3][0].opts[j]);
        languageOptionCellW[j] = widest + fpsChipPadX * 2.0f;
        languageOptionTotalW += languageOptionCellW[j];
    }
    if (settingsRows[3][0].optCount > 1)
        languageOptionTotalW += fpsChipGap * (settingsRows[3][0].optCount - 1);
    if (languageOptionTotalW > rightControlW && languageOptionTotalW > 1.0f) {
        languageOptionScale *= std::max(0.55f,
                                        rightControlW / languageOptionTotalW);
        for (int j = 0; j < settingsRows[3][0].optCount; ++j)
            languageOptionCellW[j] = localizedOptionWidth(
                3, 0, j, languageOptionScale, settingsRows[3][0].opts[j])
                + fpsChipPadX * 2.0f;
    }

    BatchFlush();
    glEnable(GL_SCISSOR_TEST);
    glScissor((GLint)rpX,
              (GLint)(sh - rightListBottom),
              (GLint)rClickW,
              (GLint)(rightListBottom - rightListTop));
    for (int i = 0; i < rightListCount; ++i) {
        const float categoryOffset = (1.0f - categoryReveal) * 14.0f * uiS;
        const float y = rightListTop + i * rightRowH - rightScroll
                      + categoryOffset;
        if (y < rightListTop - rightRowH || y > rightListBottom) continue;

        const SettingsListEntry& entry = rightList[i];
        // A category may change after the list was assembled earlier this
        // frame. Hide the old category immediately; the new list arrives on
        // the next frame and fades into the same panel.
        if (entry.category != tab) continue;
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
            const float categoryHeaderScale = settingsScale(
                g_TextL, 1.0f, UiTextLevel::Subtitle);
            const float categoryHeaderH = g_TextL.Height(
                entry.label, categoryHeaderScale);
            drawSettingsTextAtLevel(g_TextL, entry.label, rightRowX,
                                    y + (rightRowH - categoryHeaderH) * 0.5f,
                                    1.0f,
                                    catR, catG, catB, 1.0f * dA,
                                    UiTextLevel::Subtitle, 0.56f);
            const float headerRuleY = y + rightRowH - 10.0f * uiS;
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
                tab = category;
                detailRow = row;
                listCursor = -1;
                for (int k = 0; k < listCount; ++k) {
                    if (!list[k].header && list[k].category == category
                        && list[k].row == row) {
                        listCursor = k;
                        break;
                    }
                }
                focusChanged = true;
            }
        }
        rowHover[category][row] = UpdateMenuCommandHover(
            rowHover[category][row], itemHover && !setting.readOnly, dt);
        const float focus = rowHover[category][row];
        const float rowA = dA * categoryReveal * (0.72f + 0.28f * Smoothstep(
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
        const bool compactFpsRow = category == 0 && row == 0;
        const bool compactLanguageRow = category == 3 && row == 0;
        const bool compactAlignedRow = compactFpsRow || compactLanguageRow;
        const float optSc = compactFpsRow ? fpsOptionScale
            : compactLanguageRow ? languageOptionScale : commonOptionScale;
        const UiTextLevel optionTextLevel = setting.optCount > 0
            ? settingsOptionTextLevel(category, row) : UiTextLevel::Title;
        const float settingLabelScale = settingsScale(
            g_TextL, 1.0f, UiTextLevel::Title);
        const float settingLabelH = g_TextL.Height(
            setting.label, settingLabelScale);
        // The label and its control occupy separate columns of the same row.
        // Center each text/control in that row rather than stacking the
        // control below the label inside a shared vertical content block.
        const float settingLabelY = y + (rightRowH - settingLabelH) * 0.5f;
        const float controlHitY = y;
        const float controlHitH = rightRowH;
        const float controlY = y + rightRowH * 0.5f;
        drawSettingsTextAtLevel(
            g_TextL, setting.label, rightRowX + 20.0f * uiS,
            settingLabelY, 1.0f, titleR, titleG, titleB,
            1.0f * rowA, UiTextLevel::Title, 0.66f);
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
            const float volumeScale = settingsScale(
                g_TextL, 1.0f, UiTextLevel::Title);
            drawSettingsTextAtLevel(
                g_TextL, vbuf, slX + slW + 16.0f * uiS,
                controlY - g_TextL.Height(vbuf, volumeScale) * 0.5f,
                1.0f, 0.48f, 0.82f, 1.0f,
                (0.72f + 0.24f * glow) * rowA,
                UiTextLevel::Title, 0.58f);
        } else if (setting.isHold) {
            const float barW = std::min(250.0f * uiS, rightControlW);
            const float barH = 46.0f * uiS;
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
            static const wchar_t* resetLabelVariants[] = {
                L"길게 눌러 초기화", L"HOLD TO RESET", L"長押しでリセット"
            };
            static const wchar_t* erasingLabelVariants[] = {
                L"삭제 중...", L"ERASING...", L"削除中..."
            };
            const wchar_t* resetLabel = holdT > 0.01f
                ? (korean ? erasingLabelVariants[0] : erasingLabelVariants[1])
                : (korean ? resetLabelVariants[0] : resetLabelVariants[1]);
            const wchar_t* const* labelVariants = holdT > 0.01f
                ? erasingLabelVariants : resetLabelVariants;
            float resetTextScale = settingsScale(
                g_TextL, 1.0f, UiTextLevel::Title);
            const float widestLabel = MaxLocalizedTextWidth(
                g_TextL, labelVariants, 3, resetTextScale);
            const float resetTextMaxW = std::max(1.0f, barW - 24.0f * uiS);
            if (widestLabel > resetTextMaxW && widestLabel > 0.0f)
                resetTextScale *= resetTextMaxW / widestLabel;
            const float resetTextW = g_TextL.Width(resetLabel, resetTextScale);
            const float resetTextH = g_TextL.Height(resetLabel, resetTextScale);
            DrawShadowedText(g_TextL, resetLabel,
                             barX + (barW - resetTextW) * 0.5f,
                             controlY - resetTextH * 0.5f,
                             resetTextScale, 1.0f, 0.36f, 0.30f,
                             1.0f * rowA, 0.58f);
        } else if (setting.optCount > 0) {
            // FPS and language share one compact, measured rhythm; other
            // controls retain the wider common option cells.
            const float chipPadX = compactAlignedRow ? fpsChipPadX : commonChipPadX;
            const float chipGap = compactAlignedRow ? fpsChipGap : commonChipGap;
            float ox = rightControlX;
            for (int j = 0; j < setting.optCount; ++j) {
                const float cellW = compactFpsRow ? fpsOptionCellW[j]
                    : compactLanguageRow ? languageOptionCellW[j]
                    : commonOptionCellW[j];
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
                const bool blurRateDim = category == 0 && row == 4
                                      && !g_BackdropBlurEnabled;
                const float optionA = current
                    ? (blurRateDim ? 0.62f : 1.0f)
                    : (blurRateDim ? 0.20f : 0.32f) + 0.44f * optionHoverT;
                const float optionR = current
                    ? catR * 0.42f + 0.58f
                    : catR * (0.68f + 0.18f * optionHoverT);
                const float optionG = current
                    ? catG * 0.42f + 0.58f
                    : catG * (0.68f + 0.18f * optionHoverT);
                const float optionB = current
                    ? catB * 0.42f + 0.58f
                    : catB * (0.68f + 0.18f * optionHoverT);
                // DrawShadowedText takes the top of the glyph box. Lift the
                // labels to align their visual centers with the option hit row.
                const float optionScale = settingsScale(
                    g_TextL, optSc, optionTextLevel);
                const float optionTextY = controlY
                    - g_TextL.Height(setting.opts[j], optionScale) * 0.5f;
                drawSettingsTextAtLevel(g_TextL, setting.opts[j],
                                        ox + chipPadX,
                                        optionTextY, optSc,
                                        optionR, optionG, optionB,
                                        optionA * rowA,
                                        optionTextLevel,
                                        current ? 0.44f : 0.30f);
                ox += cellW + chipGap;
            }
        } else if (setting.value) {
            const float valueScale = settingsScale(
                g_TextL, 1.0f, UiTextLevel::Title);
            drawSettingsTextAtLevel(
                g_TextL, setting.value, rightControlX,
                controlY - g_TextL.Height(setting.value, valueScale) * 0.5f,
                1.0f, 0.70f, 0.78f, 0.88f, 1.0f * rowA,
                UiTextLevel::Title, 0.58f);
        }
    }
    if (!lmb) s_volDrag = false;
    BatchFlush();
    glDisable(GL_SCISSOR_TEST);
    if (rightMaxScroll > 0.0f && rightListCount > 0
        && rightList[0].category == tab) {
        const float rightTotalH = rightListCount * rightRowH;
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
    const float backX = BottomLeftActionX(uiS);
    const float backY = BottomLeftActionY(sh, 64.0f * uiS, uiS);
    const float backW = 286.0f * uiS;
    const float backH = 64.0f * uiS;
    const float saveW = std::min(486.0f * uiS, detailW * 0.82f);
    const float saveX = detailX + detailW - saveW - 24.0f * uiS;
    const float saveY = backY;
    const float saveH = 64.0f * uiS;
    const bool backHit = ready && !confirmBack
                       && mx >= backX && mx < backX + backW
                       && my >= backY && my < backY + backH;
    const bool saveHit = ready && !confirmBack
                       && mx >= saveX && mx < saveX + saveW
                       && my >= saveY && my < saveY + saveH;

    static const wchar_t* settingsBackVariants[] = { L"뒤로", L"BACK", L"戻る" };
    static const wchar_t* settingsSaveVariants[] = {
        L"변경 저장", L"SAVE CHANGES", L"変更を保存"
    };
    const int settingsLanguage = std::max(0, std::min(2, LangIndex()));
    DrawUnifiedMenuCommand(settingsBackVariants[settingsLanguage],
                           backX, backY, backW, backH,
                           0.34f, 0.72f, 1.0f, treeA,
                           0.0f, backHit, 0.0f, now,
                           uiS, true, true, true,
                           settingsBackVariants, 3);
    DrawUnifiedMenuCommand(settingsSaveVariants[settingsLanguage],
                           saveX, saveY, saveW, saveH,
                           0.34f, 0.82f, 1.0f,
                           treeA * (settingsDirty ? 1.0f : 0.24f),
                           0.0f, saveHit && settingsDirty, 0.0f, now + 0.2f,
                           uiS, true, true, true,
                           settingsSaveVariants, 3);

    if (ready && !confirmBack && lmb && !g_LmbPrev && backHit) {
        if (settingsDirty) confirmBack = true;
        else exiting = true;
    }
    if (ready && !confirmBack && lmb && !g_LmbPrev && saveHit && settingsDirty) {
        SaveGame();
        savedSettings = { g_FpsCap, g_VfxDensity, g_ShaderFx, g_MobVisualStyle,
                          g_ShowCombo, g_BackdropBlurEnabled, g_BackdropBlurCaptureHz,
                          g_SoundVol, g_BgmEnabled, g_SfxEnabled,
                          g_AudioMonoOutput, g_AudioEngineEnabled,
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
        const float dialogTitleScale = settingsScale(
            g_TextL, 1.0f, UiTextLevel::Title);
        const wchar_t* dialogTitle = L"UNSAVED CHANGES";
        drawSettingsTextAtLevel(
            g_TextL, dialogTitle,
            cx - g_TextL.Width(dialogTitle, dialogTitleScale) * 0.5f,
            cy - 66.0f * uiS, 1.0f,
            1.0f, 1.0f, 1.0f, 0.98f * treeA,
            UiTextLevel::Title, 0.72f);
        const wchar_t* dialogPrompt = L"SAVE SETTINGS BEFORE EXIT?";
        const float dialogPromptScale = settingsScale(
            g_TextL, 1.0f, UiTextLevel::Description);
        drawSettingsTextAtLevel(
            g_TextL, dialogPrompt,
            cx - g_TextL.Width(dialogPrompt, dialogPromptScale) * 0.5f,
            cy - 28.0f * uiS, 1.0f,
            0.60f, 0.72f, 0.86f, 0.82f * treeA,
            UiTextLevel::Description, 0.58f);
        const float dialogY = cy + 26.0f * uiS;
        const float dialogW = 210.0f * uiS;
        const float dialogH = 48.0f * uiS;
        const bool saveBackHit = mx >= cx - dialogW - 12.0f * uiS
                              && mx < cx - 12.0f * uiS
                              && my >= dialogY && my < dialogY + dialogH;
        const bool discardHit = mx >= cx + 12.0f * uiS
                              && mx < cx + dialogW + 12.0f * uiS
                              && my >= dialogY && my < dialogY + dialogH;
        static const wchar_t* saveBackVariants[] = {
            L"저장 후 뒤로", L"SAVE & BACK", L"保存して戻る"
        };
        static const wchar_t* discardVariants[] = { L"폐기", L"DISCARD", L"破棄" };
        DrawUnifiedMenuCommand(saveBackVariants[settingsLanguage],
                               cx - dialogW - 12.0f * uiS, dialogY,
                               dialogW, dialogH, 0.34f, 0.82f, 1.0f,
                               treeA, 0.0f, saveBackHit, 0.0f, now,
                               uiS, true, true, true,
                               saveBackVariants, 3);
        DrawUnifiedMenuCommand(discardVariants[settingsLanguage],
                               cx + 12.0f * uiS, dialogY,
                               dialogW, dialogH, 1.0f, 0.32f, 0.28f,
                               treeA, 0.0f, discardHit, 0.0f, now + 0.2f,
                               uiS, true, true, true,
                               discardVariants, 3);
        if (lmb && !g_LmbPrev && saveBackHit) {
            SaveGame();
            savedSettings = { g_FpsCap, g_VfxDensity, g_ShaderFx, g_MobVisualStyle,
                              g_ShowCombo, g_BackdropBlurEnabled, g_BackdropBlurCaptureHz,
                              g_SoundVol, g_BgmEnabled, g_SfxEnabled,
                              g_AudioMonoOutput, g_AudioEngineEnabled,
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
     // control. Only the selected category is rendered on the right panel.
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
                InvalidateBackdropCapture();
             } else if (row == 4) {
                 static const int captureRates[] = { 1, 20, 60 };
                 g_BackdropBlurCaptureHz = captureRates[option];
                 InvalidateBackdropCapture();
             }
         } else if (category == 1) {
             if (row == 1) {
                 g_BgmEnabled = option == 0;
                 Audio::SetBgmEnabled(g_BgmEnabled);
             } else if (row == 2) {
                 g_SfxEnabled = option == 0;
                 Audio::SetSfxEnabled(g_SfxEnabled);
             } else if (row == 3) {
                 Audio::SetMonoOutput(option == 1);
                 g_AudioMonoOutput = Audio::IsMonoOutput();
             } else if (row == 4) {
                 g_AudioEngineEnabled = option == 0;
                 Audio::SetEnabled(g_AudioEngineEnabled && g_SoundVol > 0);
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

    drawSettingsTextAtLevel(g_TextS,
                            korean ? L"ESC / RMB  뒤로" : L"ESC / RMB  BACK",
                            backX, sh - 48.0f * uiS, 1.0f,
                            0.52f, 0.62f, 0.72f, 0.58f * dA,
                            UiTextLevel::Supporting, 0.56f);

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

