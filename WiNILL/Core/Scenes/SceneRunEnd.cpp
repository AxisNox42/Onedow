#include "Scenes.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "GameManager.h"

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
#include "PlayerWeaponShell.h"
#include "Monster.h"
#include "AugmentSlots.h"
#include "../../Render/Juice.h"
#include <algorithm>
#include <vector>
#include <string>
#include <cmath>
#include <functional>

#include "SceneInternal.h"

namespace {
enum class GameOverAction : int { Restart, ReturnToMenu, Quit };
constexpr int kGameOverActionCount = static_cast<int>(GameOverAction::Quit) + 1;

void ApplyGameOverAction(GameOverAction action, GLFWwindow* window,
                         const SceneCtx& scene,
                         const GameOverSceneContext& state) {
    switch (action) {
    case GameOverAction::Restart:
        if (scene.reset) scene.reset();
        FinalizeLoadout(scene, FixedWeaponForSelectedJob());
        break;
    case GameOverAction::ReturnToMenu:
        state.currentState = GameState::MAIN_MENU;
        break;
    case GameOverAction::Quit:
        if (window) glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
    }
}
}

void Scene_GameOver(const SceneCtx& c, const GameOverSceneContext& state) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    GLFWwindow* window = c.window;
    static int s_focusedModule = -1;
    float gof = std::max(0.0f, std::min(1.0f, state.fade));
    if (gof < 0.05f) s_focusedModule = -1;
    bool skippedCinematic = false;
    if (gof < 0.999f && lmb && !state.previousLeftMouseDown) {
        // A click during the collapse is a skip request, never a menu action.
        state.fade = 1.0f;
        gof = 1.0f;
        skippedCinematic = true;
    }
    const float ge = Smoothstep(gof);
    const float now = (float)glfwGetTime();
    const int li = std::clamp(LangIndex(), 0, LANG_COUNT - 1);
    const float uiS = UiScale(sw, sh, 1.0f, 1.0f, 0.72f);

    static const wchar_t* kDeathLabel[LANG_COUNT] = {
        L"\uC0AC\uB9DD\uC6D0\uC778", L"DEATH CAUSE", L"\u6B7B\u56E0"
    };
    static const wchar_t* kReportLabel[LANG_COUNT] = {
        L"\uD50C\uB808\uC774 \uB9AC\uD3EC\uD2B8", L"PLAY REPORT", L"\u30D7\u30EC\u30A4 \u30EC\u30DD\u30FC\u30C8"
    };
    static const wchar_t* kBestLabel[LANG_COUNT] = {
        L"\uCD5C\uACE0 \uAE30\uB85D", L"BEST SCORE", L"\u30D9\u30B9\u30C8\u30B9\u30B3\u30A2"
    };
    static const wchar_t* kStardustLabel[LANG_COUNT] = {
        L"\uD68D\uB4DD \uBCC4\uAC00\uB8E8", L"STARDUST GAINED", L"\u7372\u5F97\u30B9\u30BF\u30FC\u30C0\u30B9\u30C8"
    };
    static const wchar_t* kCoinLabel[LANG_COUNT] = {
        L"\uD68D\uB4DD \uCF54\uC778", L"COINS EARNED", L"\u7372\u5F97\u30B3\u30A4\u30F3"
    };
    static const wchar_t* kTotalLabel[LANG_COUNT] = {
        L"\uBCF4\uC720 \uBCC4\uC790\uB9AC", L"OWNED CONSTELLATIONS", L"\u6240\u6301\u661F\u5EA7"
    };
    static const wchar_t* kBuildLabel[LANG_COUNT] = {
        L"\uBE4C\uB4DC \uBCC4\uC790\uB9AC", L"BUILD CONSTELLATION", L"\u30D3\u30EB\u30C9\u661F\u5EA7"
    };
    static const wchar_t* kModulesLabel[LANG_COUNT] = {
        L"\uAE30\uB85D\uB41C \uBAA8\uB4C8", L"MODULES RECORDED", L"\u8A18\u9332\u30E2\u30B8\u30E5\u30FC\u30EB"
    };
    static const wchar_t* kNoModules[LANG_COUNT] = {
        L"\uAE30\uB85D\uB41C \uBAA8\uB4C8 \uC5C6\uC74C", L"NO MODULES RECORDED", L"\u8A18\u9332\u30E2\u30B8\u30E5\u30FC\u306A\u3057"
    };
    static const wchar_t* kSelectedModule[LANG_COUNT] = {
        L"\uC120\uD0DD\uB41C \uBCC4\uC790\uB9AC", L"SELECTED CONSTELLATION", L"\u9078\u629E\u3057\u305F\u661F\u5EA7"
    };
    static const wchar_t* kNewRecord[LANG_COUNT] = {
        L"* \uC2E0\uAE30\uB85D *", L"* NEW RECORD *", L"* \u65B0\u8A18\u9332 *"
    };
    static const wchar_t* kRoute[LANG_COUNT][kGameOverActionCount] = {
        { L"\uC7AC\uC2DC\uC791", L"\uB4A4\uB85C", L"\uC885\uB8CC" },
        { L"RESTART", L"BACK", L"EXIT" },
        { L"\u518D\u8D77\u52D5", L"\u623B\u308B", L"\u7D42\u4E86" }
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
    const float deathY = std::max(58.0f, sh * 0.14f);
    const float accentR = 0.32f, accentG = 0.82f, accentB = 1.0f;

    const wchar_t* leftMetricVariants[LANG_COUNT * 3];
    const wchar_t* rightMetricVariants[LANG_COUNT * 3];
    for (int language = 0; language < LANG_COUNT; ++language) {
        leftMetricVariants[language * 3] = kStrings[(int)StrId::FINAL_SCORE][language];
        leftMetricVariants[language * 3 + 1] = kStrings[(int)StrId::REACHED_LEVEL][language];
        leftMetricVariants[language * 3 + 2] = kStardustLabel[language];
        rightMetricVariants[language * 3] = kBestLabel[language];
        rightMetricVariants[language * 3 + 1] = kStrings[(int)StrId::KILL_COUNT][language];
        rightMetricVariants[language * 3 + 2] = kCoinLabel[language];
    }

    float deathScaleFactor = uiS;
    float deathScale = UiTextScale(state.text.body, UiTextLevel::Subtitle,
                                   deathScaleFactor);
    const float reportScale = UiTextScale(state.text.body, UiTextLevel::Subtitle, uiS);
    float metricLabelFactor = uiS;
    float metricLabelScale = UiTextScale(state.text.body, UiTextLevel::Subtitle,
                                         metricLabelFactor);
    float maxLeftMetricLabelW = MaxLocalizedTextWidth(
        state.text.body, leftMetricVariants, LANG_COUNT * 3, metricLabelScale);
    float maxRightMetricLabelW = MaxLocalizedTextWidth(
        state.text.body, rightMetricVariants, LANG_COUNT * 3, metricLabelScale);
    while (metricLabelFactor > 0.55f &&
           maxLeftMetricLabelW + maxRightMetricLabelW + 32.0f * uiS > leftW) {
        metricLabelFactor -= 0.04f;
        metricLabelScale = UiTextScale(state.text.body, UiTextLevel::Subtitle,
                                       metricLabelFactor);
        maxLeftMetricLabelW = MaxLocalizedTextWidth(
            state.text.body, leftMetricVariants, LANG_COUNT * 3, metricLabelScale);
        maxRightMetricLabelW = MaxLocalizedTextWidth(
            state.text.body, rightMetricVariants, LANG_COUNT * 3, metricLabelScale);
    }
    const float metricGap = std::min(
        std::max(leftW * 0.44f, maxLeftMetricLabelW + 24.0f * uiS),
        leftW - maxRightMetricLabelW);
    const float metricValueScale = UiTextScale(
        state.text.title, UiTextLevel::Description, 1.4f * uiS);
    const float recordScale = UiTextScale(state.text.body, UiTextLevel::Supporting, uiS);
    const float metricValueY = state.text.body.Height(L"A", metricLabelScale)
                             + 8.0f * uiS;
    const float metricRowH = std::max(
        std::max(76.0f, std::min(96.0f, sh * 0.102f)),
        metricValueY + state.text.title.Height(L"A", metricValueScale)
                     + 14.0f * uiS);
    const bool hasDeathReason = state.deathReason && state.deathReason[0];
    constexpr const wchar_t* kDeathSeparator = L" : ";
    wchar_t deathLine[512];
    if (hasDeathReason)
        swprintf_s(deathLine, L"%ls%ls%ls", kDeathLabel[li],
                   kDeathSeparator, state.deathReason);
    else
        swprintf_s(deathLine, L"%ls", kDeathLabel[li]);
    while (deathScaleFactor > 0.55f &&
           state.text.body.Width(deathLine, deathScale) > leftW) {
        deathScaleFactor = std::max(0.55f, deathScaleFactor - 0.04f);
        deathScale = UiTextScale(state.text.body, UiTextLevel::Subtitle,
                                 deathScaleFactor);
    }
    const float deathH = state.text.body.Height(deathLine, deathScale);
    const float reportH = state.text.body.Height(kReportLabel[li], reportScale);
    const float recordH = state.text.body.Height(kNewRecord[li], recordScale);
    const float reportW = MaxLocalizedTextWidth(state.text.body, kReportLabel,
                                                LANG_COUNT, reportScale);
    const float recordW = MaxLocalizedTextWidth(state.text.body, kNewRecord,
                                                LANG_COUNT, recordScale);
    const bool recordInline = state.lastRunRecord &&
        reportW + recordW + 24.0f * uiS <= leftW;
    const float reportY = deathY + deathH + 14.0f * uiS;
    const float recordY = reportY + (recordInline ? 0.0f : reportH + 5.0f * uiS);
    const float reportBottomY = std::max(reportY + reportH,
        state.lastRunRecord ? recordY + recordH : reportY);
    const float ruleY = reportBottomY + 8.0f * uiS;
    const float metricY = ruleY + 18.0f * uiS;

    state.text.body.Draw(kDeathLabel[li], leftX, deathY,
                 deathScale,
                 0.72f, 0.76f, 0.86f, 0.82f * ge);
    if (hasDeathReason) {
        const float separatorX = leftX + state.text.body.Width(
            kDeathLabel[li], deathScale);
        const float reasonX = separatorX + state.text.body.Width(
            kDeathSeparator, deathScale);
        state.text.body.Draw(kDeathSeparator, separatorX, deathY, deathScale,
                     0.72f, 0.76f, 0.86f, 0.82f * ge);
        state.text.body.Draw(state.deathReason, reasonX, deathY, deathScale,
                     1.0f, 0.46f, 0.43f, 0.90f * ge);
    }

    state.text.body.Draw(kReportLabel[li], leftX, reportY,
                 reportScale,
                 accentR, accentG, accentB, 0.92f * ge);
    LogoLine(leftX, ruleY, leftX + leftW, ruleY,
             1.0f, accentR, accentG, accentB, 0.20f * ge);
    if (state.lastRunRecord) {
        const float blink = 0.62f + 0.38f * sinf(now * 6.0f);
        const float recordX = recordInline ? leftX + leftW - recordW : leftX;
        state.text.body.Draw(kNewRecord[li], recordX, recordY, recordScale,
                     1.0f, 0.84f, 0.28f, blink * ge);
    }

    float countF = std::min(1.0f, gof / 0.72f);
    countF = Smoothstep(countF);
    const long long score = (long long)(state.score * countF);
    const long long bestValue = g_BestScore[(int)g_Difficulty];
    const long long best = state.lastRunRecord
        ? (long long)(bestValue * countF) : bestValue;
    const long long stardust = (long long)(g_RunStardust * countF);
    const long long coins = (long long)(g_LastRunCoins * countF);

    auto drawMetric = [&](const wchar_t* label, long long value,
                          float x, float y,
                          float rr, float gg, float bb) {
        state.text.body.Draw(label, x, y, metricLabelScale,
                     0.64f, 0.76f, 0.88f, 0.88f * ge);
        wchar_t valueBuf[64];
        swprintf_s(valueBuf, L"%lld", value);
        state.text.title.Draw(valueBuf, x, y + metricValueY, metricValueScale,
                     rr, gg, bb, 0.94f * ge);
    };

    drawMetric(T(StrId::FINAL_SCORE), score, leftX, metricY,
               1.0f, 0.94f, 0.58f);
    drawMetric(kBestLabel[li], best, leftX + metricGap, metricY,
               1.0f, 0.78f, 0.34f);
    drawMetric(T(StrId::REACHED_LEVEL), state.playerLevel,
               leftX, metricY + metricRowH,
               0.68f, 0.92f, 1.0f);
    drawMetric(T(StrId::KILL_COUNT), (long long)state.killCount,
               leftX + metricGap, metricY + metricRowH,
               0.70f, 1.0f, 0.78f);
    drawMetric(kStardustLabel[li], stardust, leftX,
               metricY + metricRowH * 2.0f,
               0.72f, 0.88f, 1.0f);
    drawMetric(kCoinLabel[li], coins, leftX + metricGap,
               metricY + metricRowH * 2.0f,
               1.0f, 0.86f, 0.30f);
    wchar_t totalCoinBuf[64];
    swprintf_s(totalCoinBuf, L"%ls  %lld", kTotalLabel[li], g_Coins);
    state.text.body.Draw(totalCoinBuf, leftX + metricGap,
                 metricY + metricRowH * 3.0f - 12.0f * uiS,
                 UiTextScale(state.text.body, UiTextLevel::Supporting, uiS),
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
    state.text.body.Draw(chartTitle,
                 chartCX - state.text.body.Width(chartTitle,
                     UiTextScale(state.text.body, UiTextLevel::Subtitle, uiS)) * 0.5f,
                 chartCY - chartR * 1.34f,
                 UiTextScale(state.text.body, UiTextLevel::Subtitle, uiS),
                 accentR, accentG, accentB, 0.94f * chartA);

    std::vector<int> modules;
    modules.reserve(64);
    for (int idx : state.ownedAugments) {
        if (idx >= 0 && idx < AUG_TOTAL && !AugRemoved(ALL_AUGS[idx].type) &&
            modules.size() < 64)
            modules.push_back(idx);
    }
    const int moduleCount = (int)modules.size();
    const int visibleModules = std::min(moduleCount, 8);
    if (s_focusedModule < 0 || s_focusedModule >= visibleModules)
        s_focusedModule = -1;
    wchar_t moduleCountBuf[64];
    swprintf_s(moduleCountBuf, L"%ls  %d", kModulesLabel[li], moduleCount);
    state.text.body.Draw(moduleCountBuf,
                 chartCX - state.text.body.Width(moduleCountBuf, 0.42f) * 0.5f,
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
            if (hoveredModule >= 0 && lmb && !state.previousLeftMouseDown)
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
            state.text.body.Draw(moreBuf,
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
            const float selectedScale = UiTextScale(state.text.body, UiTextLevel::Supporting, uiS);
            state.text.body.Draw(kSelectedModule[li],
                         chartCX - state.text.body.Width(kSelectedModule[li], selectedScale) * 0.5f,
                         detailY, selectedScale,
                         accentR, accentG, accentB, 0.88f * chartA);
            const wchar_t* nameVariants[] = {
                selected.locName[0], selected.locName[1], selected.locName[2]
            };
            float nameFactor = uiS;
            float nameScale = UiTextScale(state.text.title, UiTextLevel::Description, nameFactor);
            while (nameFactor > 0.55f &&
                   MaxLocalizedTextWidth(state.text.title, nameVariants, LANG_COUNT, nameScale) > detailW) {
                nameFactor -= 0.04f;
                nameScale = UiTextScale(state.text.title, UiTextLevel::Description, nameFactor);
            }
            state.text.title.Draw(moduleName,
                         chartCX - state.text.title.Width(moduleName, nameScale) * 0.5f,
                         detailY + 22.0f * uiS, nameScale,
                         0.92f, 0.98f, 1.0f, 0.94f * chartA);
            if (moduleDesc && moduleDesc[0]) {
                const float descScale = UiTextScale(state.text.body, UiTextLevel::Supporting, uiS);
                const std::vector<std::wstring> descLines = TarotWrap(moduleDesc, descScale, detailW);
                float descY = detailY + 53.0f * uiS;
                for (const std::wstring& line : descLines) {
                    const float lineW = state.text.body.Width(line.c_str(), descScale);
                    state.text.body.Draw(line.c_str(), chartCX - lineW * 0.5f, descY,
                                 descScale, 0.62f, 0.74f, 0.84f, 0.78f * chartA);
                    descY += state.text.body.Height(line.c_str(), descScale) + 2.0f * uiS;
                }
            }
        }
    } else {
        const float noModulesScale = UiTextScale(state.text.body, UiTextLevel::Description, uiS);
        state.text.body.Draw(kNoModules[li],
                     chartCX - state.text.body.Width(kNoModules[li], noModulesScale) * 0.5f,
                     chartCY + chartR * 0.78f, noModulesScale,
                     0.62f, 0.76f, 0.90f, 0.82f * chartA);
    }

    static float s_hovGO[kGameOverActionCount] = {};
    const float BW = std::min(520.0f, std::max(280.0f, sw * 0.38f));
    const float BH = std::max(48.0f, std::min(68.0f, sh * 0.075f));
    const float BGAP = std::max(10.0f, sh * 0.013f);
    const float totalBH = (float)kGameOverActionCount * BH + (float)(kGameOverActionCount - 1) * BGAP;
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
    for (int i = 0; i < kGameOverActionCount; ++i) {
        const float by = bY0 + (float)i * (BH + BGAP);
        const bool hov = showButtons &&
            mx >= bX0 - 26.0f && mx <= bX0 + BW + 32.0f &&
            my >= by && my <= by + BH;
        s_hovGO[i] = UpdateMenuCommandHover(s_hovGO[i], hov, delta);
        const float rowA = (showButtons ? 1.0f : 0.0f) * ge;
        const wchar_t* routeVariants[LANG_COUNT] = {};
        for (int language = 0; language < LANG_COUNT; ++language)
            routeVariants[language] = kRoute[language][i];
        DrawUnifiedMenuCommand(kRoute[li][i],
                               bX0 - 10.0f * s_hovGO[i], by, BW, BH,
                               accentR, accentG, accentB, rowA,
                               s_hovGO[i], false, 0.0f,
                               now + (float)i * 0.17f,
                               uiS, false, true, false,
                               routeVariants, LANG_COUNT);
        if (hov && lmb && !state.previousLeftMouseDown) {
            ApplyGameOverAction(static_cast<GameOverAction>(i), window, c, state);
        }
    }
}



