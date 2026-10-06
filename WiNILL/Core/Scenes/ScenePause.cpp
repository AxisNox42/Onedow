#include "Scenes.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "GameManager.h"
#include "ScenePause.h"
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

#include "SceneInternal.h"

namespace {
enum class PauseAction : int { Resume, OpenSettings, ViewAugments, Abandon };
constexpr int kPauseActionCount = static_cast<int>(PauseAction::Abandon) + 1;

void ApplyPauseAction(PauseAction action, const SceneCtx& scene,
                      const PauseSceneContext& state) {
    switch (action) {
    case PauseAction::Resume:
        state.showOwnedAugments = false;
        state.currentState = state.resumeState;
        state.resumeState = GameState::RUNNING;
        break;
    case PauseAction::OpenSettings:
        state.showOwnedAugments = false;
        state.settingsReturnState = GameState::PAUSED;
        if (state.resetSettingsUi) state.resetSettingsUi(0.0f);
        state.currentState = GameState::SETTINGS;
        break;
    case PauseAction::ViewAugments:
        state.showOwnedAugments = true;
        break;
    case PauseAction::Abandon:
        state.showOwnedAugments = false;
        if (scene.abandonRun) scene.abandonRun();
        break;
    }
}
}

void Scene_Paused(const SceneCtx& c, const PauseSceneContext& state) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    (void)*c.fireTimer;
    (void)c.reset;

    static float  s_EntryT    = 0.0f;
    static float  s_HoverT[kPauseActionCount] = {};
    static int    s_ExitSel   = -1;
    static float  s_ExitT     = 0.0f;
    static double s_LastCall  = 0.0;

    const double curTime = glfwGetTime();
    if (curTime - s_LastCall > 0.12) {
        s_EntryT = 0.0f;
        for (int i = 0; i < kPauseActionCount; ++i) s_HoverT[i] = 0.0f;
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
    const float uiScale = UiScale(sw, sh);
    const float BW      = std::min(520.0f * uiScale, sw * 0.38f);
    const float BH      = 70.0f * uiScale;
    const float BGAP    = 15.0f * uiScale;
    const float totalBH = (float)kPauseActionCount * BH + (float)(kPauseActionCount - 1) * BGAP;
    const float btnX0   = std::max(58.0f, sw * 0.075f);
    const float btnY0   = sh * 0.42f;
    const float titleY  = sh * 0.24f;

    static bool s_InventoryEscArmed = true;
    if (state.showOwnedAugments) {
        const int esc = c.window ? glfwGetKey(c.window, GLFW_KEY_ESCAPE)
                                 : GLFW_RELEASE;
        if (esc == GLFW_RELEASE) s_InventoryEscArmed = true;
        else if (s_InventoryEscArmed) {
            state.showOwnedAugments = false;
            s_InventoryEscArmed = false;
        }
        const int li = std::clamp(LangIndex(), 0, LANG_COUNT - 1);
        static const wchar_t* kInventoryBack[LANG_COUNT] = {
            L"ESC  뒤로", L"ESC  BACK", L"ESC  戻る"
        };
        const float backScale = UiTextScale(
            state.text.body, UiTextLevel::Supporting, uiScale);
        const wchar_t* back = kInventoryBack[li];
        state.text.body.Draw(back,
            sw - 32.0f * uiScale - state.text.body.Width(back, backScale),
            sh - 42.0f * uiScale, backScale,
            0.48f, 0.78f, 0.96f, 0.88f * entryFade);
        return;
    }
    s_InventoryEscArmed = false;
    if (!c.window || glfwGetKey(c.window, GLFW_KEY_ESCAPE) == GLFW_RELEASE)
        s_InventoryEscArmed = true;

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
        const int li = std::clamp(LangIndex(), 0, LANG_COUNT - 1);
        static const wchar_t* kPauseHeader[LANG_COUNT] = {
            L"플레이 일시정지", L"PLAY PAUSED", L"プレイ一時停止"
        };
        const wchar_t* hdr = kPauseHeader[li];
        float hdrSc = UiTextScale(state.text.title, UiTextLevel::Title, uiScale);
        state.text.title.Draw(hdr, btnX0, titleY, hdrSc, 0.50f, 0.82f, 1.0f, 0.92f * titleA);
        BindMainShader();
        drawRect(btnX0, titleY + state.text.title.Height(hdr, hdrSc) + 14.0f * uiScale,
                 BW * 0.52f, 1.0f, 0.30f, 0.78f, 1.0f, 0.22f * titleA);
    }

    // 버튼
    static const wchar_t* kPauseRoutes[LANG_COUNT][kPauseActionCount] = {
        { L"계속하기", L"설정", L"보유 증강", L"\uD3EC\uAE30\uD558\uAE30" },
        { L"CONTINUE", L"SETTINGS", L"AUGMENTS", L"ABANDON" },
        { L"続ける", L"設定", L"所持強化", L"\u653E\u68C4\u3059\u308B" },
    };
    const int pauseLang = std::clamp(LangIndex(), 0, LANG_COUNT - 1);

    const float ar = 0.48f, ag = 0.82f, ab = 1.0f;

    for (int i = 0; i < kPauseActionCount; ++i) {
        float rawPh = (s_EntryT - 0.18f - (float)i * 0.08f) / 0.32f;
        float reveal = Smoothstep(std::max(0.0f, std::min(rawPh, 1.0f)));

        float baseBx = btnX0;
        float baseBy = btnY0 + (float)i * (BH + BGAP);
        const wchar_t* route = kPauseRoutes[pauseLang][i];
        const bool selected = (s_ExitSel == i);
        const float slide = (1.0f - reveal) * 34.0f;
        const float priorBx = baseBx + slide - 10.0f * s_HoverT[i];
        const float priorBy = baseBy + (exitActive && !selected ? exitP * 28.0f : 0.0f);
        bool hov = UpdatePanelButtonHover(s_HoverT[i], !exitActive,
                                          mx, my,
                                          priorBx, priorBy, BW, BH,
                                          // Keep adjacent pause rows distinct.  The
                                          // old 26px slop made a click near a row
                                          // boundary activate its neighbour.
                                          delta, 4.0f);
        float t = s_HoverT[i];

        float selectPulse = selected ? (0.50f + 0.50f * sinf(fnow * 18.0f)) * exitP : 0.0f;
        float activeT     = std::max(t, selected ? exitP : 0.0f);

        float rowA = reveal * entryFade;
        if (exitActive && !selected) rowA *= (1.0f - exitP * 0.86f);
        float bx    = baseBx + slide - 10.0f * t;
        float by    = baseBy + (exitActive && !selected ? exitP * 28.0f : 0.0f);

        const wchar_t* routeVariants[LANG_COUNT] = {
            kPauseRoutes[0][i], kPauseRoutes[1][i], kPauseRoutes[2][i]
        };
        DrawPanelButton(route, bx, by, BW, BH,
                        ar, ag, ab, rowA,
                        activeT, selected, selectPulse,
                        fnow + (float)i * 0.17f,
                        uiScale, false, PanelButtonSlideSide::Both, false,
                        routeVariants, LANG_COUNT);

        if (hov && lmb && !state.previousLeftMouseDown && s_ExitSel < 0) {
            s_ExitSel = i;
            s_ExitT   = 0.0f;
        }
    }

    if (s_ExitSel >= 0 && s_ExitT >= 0.50f) {
        int sel   = s_ExitSel;
        s_ExitSel = -1;
        s_ExitT   = 0.0f;
        ApplyPauseAction(static_cast<PauseAction>(sel), c, state);
        return;
    }

    {
        float hintA = Smoothstep(std::min(std::max(0.0f, s_EntryT - 0.50f) / 0.30f, 1.0f)) * entryFade;
        const int li = std::clamp(LangIndex(), 0, LANG_COUNT - 1);
        static const wchar_t* kPauseHint[LANG_COUNT] = {
            L"[SPACE / ESC]  계속하기", L"[SPACE / ESC]  RESUME",
            L"[SPACE / ESC]  続ける"
        };
        const wchar_t* hint = kPauseHint[li];
        const float hintScale = UiTextScale(state.text.body, UiTextLevel::Supporting, uiScale);
        state.text.body.Draw(hint, btnX0, sh * 0.88f, hintScale,
                     0.50f, 0.70f, 0.90f, 0.72f * hintA);
    }
}
