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

#include "SceneInternal.h"

void Scene_Paused(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    (void)*c.fireTimer;
    (void)c.reset;

    static float  s_EntryT    = 0.0f;
    static float  s_HoverT[3] = {};
    static int    s_ExitSel   = -1;
    static float  s_ExitT     = 0.0f;
    static double s_LastCall  = 0.0;

    const double curTime = glfwGetTime();
    if (curTime - s_LastCall > 0.12) {
        s_EntryT = 0.0f;
        for (int i = 0; i < 3; ++i) s_HoverT[i] = 0.0f;
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
    const int   NBTN    = 3;
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
            L"플레이 일시정지", L"PLAY PAUSED", L"プレイ一時停止"
        };
        static const wchar_t* kPauseStatus[3] = {
            L"플레이 상태 : 일시정지", L"PLAY STATE : PAUSED", L"プレイ状態 : 一時停止"
        };
        const wchar_t* hdr = kPauseHeader[li];
        float hdrSc = UiTextScale(g_TextL, UiTextLevel::Title, uiScale);
        g_TextL.Draw(hdr, btnX0, titleY, hdrSc, 0.50f, 0.82f, 1.0f, 0.92f * titleA);
        const wchar_t* sub = kPauseStatus[li];
        const float subSc = UiTextScale(g_TextS, UiTextLevel::Subtitle, uiScale);
        g_TextS.Draw(sub, btnX0, titleY + g_TextL.Height(hdr, hdrSc) + 2.0f * uiScale, subSc,
                     0.48f, 0.72f, 0.90f, 0.80f * titleA);
        BindMainShader();
        drawRect(btnX0, titleY + g_TextL.Height(hdr, hdrSc) + 26.0f,
                 BW * 0.52f, 1.0f, 0.30f, 0.78f, 1.0f, 0.22f * titleA);
    }

    // 버튼
    static const wchar_t* kPauseRoutes[3][3] = {
        { L"계속하기", L"설정", L"\uD3EC\uAE30\uD558\uAE30" },
        { L"CONTINUE", L"SETTINGS", L"ABANDON" },
        { L"続ける", L"設定", L"\u653E\u68C4\u3059\u308B" },
    };
    const int pauseLang = std::max(0, std::min(2, LangIndex()));

    const float ar = 0.48f, ag = 0.82f, ab = 1.0f;

    for (int i = 0; i < NBTN; ++i) {
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

        const wchar_t* routeVariants[] = {
            kPauseRoutes[0][i], kPauseRoutes[1][i], kPauseRoutes[2][i]
        };
        DrawPanelButton(route, bx, by, BW, BH,
                        ar, ag, ab, rowA,
                        activeT, selected, selectPulse,
                        fnow + (float)i * 0.17f,
                        uiScale, false, PanelButtonSlideSide::Both, false,
                        routeVariants, 3);

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
            if (c.abandonRun) c.abandonRun();
            break;
        }
        return;
    }

    {
        float hintA = Smoothstep(std::min(std::max(0.0f, s_EntryT - 0.50f) / 0.30f, 1.0f)) * entryFade;
        const int li = std::max(0, std::min(2, LangIndex()));
        static const wchar_t* kPauseHint[3] = {
            L"[SPACE / ESC]  계속하기", L"[SPACE / ESC]  RESUME",
            L"[SPACE / ESC]  続ける"
        };
        const wchar_t* hint = kPauseHint[li];
        const float hintScale = UiTextScale(g_TextS, UiTextLevel::Supporting, uiScale);
        g_TextS.Draw(hint, btnX0, sh * 0.88f, hintScale,
                     0.50f, 0.70f, 0.90f, 0.72f * hintA);
    }
}
