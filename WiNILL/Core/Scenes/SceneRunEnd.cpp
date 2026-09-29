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
    const float uiS = UiScale(sw, sh, 1.0f, 1.0f, 0.72f);

    static const wchar_t* kRunTitle[3] = {
        L"\uD50C\uB808\uC774 \uC885\uB8CC", L"PLAY ENDED", L"\u30D7\u30EC\u30A4\u7D42\u4E86"
    };
    static const wchar_t* kDeathLabel[3] = {
        L"\uC0AC\uB9DD \uC6D0\uC778", L"DEATH SIGNAL", L"\u6B7B\u56E0"
    };
    static const wchar_t* kReportLabel[3] = {
        L"\uD50C\uB808\uC774 \uB9AC\uD3EC\uD2B8", L"PLAY REPORT", L"\u30D7\u30EC\u30A4 \u30EC\u30DD\u30FC\u30C8"
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

    const float titleScale = UiTextScale(g_TextL, UiTextLevel::Title, uiS);
    g_TextL.Draw(kRunTitle[li], leftX, titleY, titleScale,
                 1.0f, 0.30f, 0.32f, 0.96f * ge);
    g_TextS.Draw(kDeathLabel[li], leftX, deathY,
                 UiTextScale(g_TextS, UiTextLevel::Subtitle, uiS),
                 0.72f, 0.76f, 0.86f, 0.82f * ge);
    if (g_DeathReason[0]) {
        float reasonFactor = uiS;
        float reasonScale = UiTextScale(g_TextS, UiTextLevel::Supporting, reasonFactor);
        while (reasonFactor > 0.55f && g_TextS.Width(g_DeathReason, reasonScale) > leftW) {
            reasonFactor -= 0.04f;
            reasonScale = UiTextScale(g_TextS, UiTextLevel::Supporting, reasonFactor);
        }
        g_TextS.Draw(g_DeathReason, leftX, deathY + 25.0f, reasonScale,
                     1.0f, 0.46f, 0.43f, 0.90f * ge);
    }

    g_TextS.Draw(kReportLabel[li], leftX, reportY,
                 UiTextScale(g_TextS, UiTextLevel::Subtitle, uiS),
                 accentR, accentG, accentB, 0.92f * ge);
    LogoLine(leftX, reportY + 27.0f, leftX + leftW, reportY + 27.0f,
             1.0f, accentR, accentG, accentB, 0.20f * ge);
    if (g_LastRunRecord) {
        const float blink = 0.62f + 0.38f * sinf(now * 6.0f);
        g_TextS.Draw(kNewRecord[li], leftX + leftW - 198.0f, reportY,
                     UiTextScale(g_TextS, UiTextLevel::Supporting, uiS),
                     1.0f, 0.84f, 0.28f, blink * ge);
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
        g_TextS.Draw(label, x, y,
                     UiTextScale(g_TextS, UiTextLevel::Supporting, uiS),
                     0.64f, 0.76f, 0.88f, 0.88f * ge);
        wchar_t valueBuf[64];
        swprintf_s(valueBuf, L"%lld", value);
        g_TextL.Draw(valueBuf, x, y + 30.0f * uiS,
                     UiTextScale(g_TextL, UiTextLevel::Description, valueScale * uiS),
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
                 metricY + metricRowH * 3.0f - 12.0f * uiS,
                 UiTextScale(g_TextS, UiTextLevel::Supporting, uiS),
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
                 chartCX - g_TextS.Width(chartTitle,
                     UiTextScale(g_TextS, UiTextLevel::Subtitle, uiS)) * 0.5f,
                 chartCY - chartR * 1.34f,
                 UiTextScale(g_TextS, UiTextLevel::Subtitle, uiS),
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
            const float selectedScale = UiTextScale(g_TextS, UiTextLevel::Supporting, uiS);
            g_TextS.Draw(kSelectedModule[li],
                         chartCX - g_TextS.Width(kSelectedModule[li], selectedScale) * 0.5f,
                         detailY, selectedScale,
                         accentR, accentG, accentB, 0.88f * chartA);
            const wchar_t* nameVariants[] = {
                selected.locName[0], selected.locName[1], selected.locName[2]
            };
            float nameFactor = uiS;
            float nameScale = UiTextScale(g_TextL, UiTextLevel::Description, nameFactor);
            while (nameFactor > 0.55f &&
                   MaxLocalizedTextWidth(g_TextL, nameVariants, 3, nameScale) > detailW) {
                nameFactor -= 0.04f;
                nameScale = UiTextScale(g_TextL, UiTextLevel::Description, nameFactor);
            }
            g_TextL.Draw(moduleName,
                         chartCX - g_TextL.Width(moduleName, nameScale) * 0.5f,
                         detailY + 22.0f * uiS, nameScale,
                         0.92f, 0.98f, 1.0f, 0.94f * chartA);
            if (moduleDesc && moduleDesc[0]) {
                const float descScale = UiTextScale(g_TextS, UiTextLevel::Supporting, uiS);
                const std::vector<std::wstring> descLines = TarotWrap(moduleDesc, descScale, detailW);
                float descY = detailY + 53.0f * uiS;
                for (const std::wstring& line : descLines) {
                    const float lineW = g_TextS.Width(line.c_str(), descScale);
                    g_TextS.Draw(line.c_str(), chartCX - lineW * 0.5f, descY,
                                 descScale, 0.62f, 0.74f, 0.84f, 0.78f * chartA);
                    descY += g_TextS.Height(line.c_str(), descScale) + 2.0f * uiS;
                }
            }
        }
    } else {
        const float noModulesScale = UiTextScale(g_TextS, UiTextLevel::Description, uiS);
        g_TextS.Draw(kNoModules[li],
                     chartCX - g_TextS.Width(kNoModules[li], noModulesScale) * 0.5f,
                     chartCY + chartR * 0.78f, noModulesScale,
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
        const wchar_t* routeVariants[] = {
            kRoute[0][i], kRoute[1][i], kRoute[2][i]
        };
        DrawUnifiedMenuCommand(kRoute[li][i],
                               bX0 - 10.0f * s_hovGO[i], by, BW, BH,
                               accentR, accentG, accentB, rowA,
                               s_hovGO[i], false, 0.0f,
                               now + (float)i * 0.17f,
                               uiS, false, true, false,
                               routeVariants, 3);
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



