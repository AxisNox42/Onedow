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
#include "Monster.h"
#include "AugmentSlots.h"
#include "../../Render/Juice.h"
#include <algorithm>
#include <vector>
#include <string>
#include <cmath>
#include <functional>

#include "SceneInternal.h"

void Scene_Tutorial(const SceneCtx& c, const TutorialSceneContext& state) {
    const SceneTextContext& text = state.text;
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    GLFWwindow* window = c.window;
    const GameState st = state.currentState;
    (void)window;
    const float uiScale = UiScale(sw, sh);

    static int s_page = 0;
    static GameState s_prevSt = GameState::MAIN_MENU;
    if (st != s_prevSt) {
        if (st == GameState::TUTORIAL) s_page = 0;
        s_prevSt = st;
    }

    auto drawSlashText = [&](const wchar_t* content, float x, float y, float maxW,
                             float sc, float r, float g, float b, float a) -> float {
        if (!content || !content[0]) return y;
        std::vector<std::wstring> lines;
        std::wstring cur;
        for (const wchar_t* p = content; *p; ++p) {
            // A slash surrounded by spaces is a section separator.  Keep
            // compact rates such as "0.28/s" intact so the unit never wraps
            // onto a line by itself.
            if (*p == L'/' && (p == content || p[-1] == L' ' || p[1] == L' ')) {
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
        float lineH = 30.0f * uiScale;
        float cy = y;
        for (int li = 0; li < (int)lines.size(); li++) {
            float factor = sc * uiScale;
            float lsc = UiTextScale(text.body, UiTextLevel::Description, factor);
            while (factor > 0.55f && text.body.Width(lines[li].c_str(), lsc) > maxW) {
                factor -= 0.04f;
                lsc = UiTextScale(text.body, UiTextLevel::Description, factor);
            }
            text.body.Draw(lines[li].c_str(), x, cy + li * lineH, lsc, r, g, b, a);
        }
        return y + (float)lines.size() * lineH;
    };

    const float WW = std::min(sw * 0.92f, 1180.0f * uiScale);
    const float WH = std::min(sh * 0.88f, 880.0f * uiScale);
    float wx, wy;
    DrawMenuBackground(sw, sh, c.delta);
    SceneAppWindow(sw, sh, WW, WH, TutorialWinTitle(), 0.35f, 0.85f, 1.0f, wx, wy, false, 0.0f);
    if (state.appOpen < 0.999f) return;

    int li = LangIndexTutorial();
    const float sideW = 210.0f * uiScale;
    const float pad = 24.0f * uiScale;
    float sideX = wx + pad;
    float sideY = wy + 16.0f;
    float contentX = wx + sideW + pad * 2.0f;
    float contentW = WW - sideW - pad * 3.0f;
    float contentY = wy + 16.0f;

    BindMainShader();
    drawRect(sideX, sideY, sideW, WH - 120.0f * uiScale, 0.10f, 0.12f, 0.16f, 0.95f);

    const wchar_t* SIDE[LANG_COUNT] = { L"목차", L"Contents", L"目次" };
    text.body.Draw(SIDE[li], sideX + 14.0f * uiScale, sideY + 10.0f * uiScale,
                 UiTextScale(text.body, UiTextLevel::Subtitle, uiScale),
                 0.55f, 0.85f, 1.0f, 0.95f);

    const float rowH = 44.0f * uiScale;
    for (int i = 0; i < TUTORIAL_PAGE_COUNT; i++) {
        float ry = sideY + 38.0f * uiScale + i * rowH;
        bool sel = (i == s_page);
        bool hov = (mx >= sideX + 6.0f && mx <= sideX + sideW - 6.0f &&
                    my >= ry && my <= ry + rowH - 4.0f);
        if (sel) drawRect(sideX + 6.0f, ry, sideW - 12.0f, rowH - 4.0f, 0.25f, 0.55f, 0.95f, 0.35f);
        else if (hov) drawRect(sideX + 6.0f, ry, sideW - 12.0f, rowH - 4.0f, 0.18f, 0.22f, 0.28f, 0.85f);
        const wchar_t* pt = TutorialPageTitle(i);
        const wchar_t* titleVariants[LANG_COUNT] = {
            kTutorialPageTitle[0][i], kTutorialPageTitle[1][i],
            kTutorialPageTitle[2][i]
        };
        float titleScale = UiTextScale(text.title, UiTextLevel::Title, uiScale);
        const float maxTitleW = MaxLocalizedTextWidth(
            text.title, titleVariants, LANG_COUNT, titleScale);
        const float maxTitleH = text.title.Height(pt, titleScale);
        const float titleFit = std::min(1.0f, std::min(
            (sideW - 30.0f * uiScale) / std::max(1.0f, maxTitleW),
            (rowH - 8.0f * uiScale) / std::max(1.0f, maxTitleH)));
        titleScale *= std::max(0.0f, titleFit);
        const float titleH = text.title.Height(pt, titleScale);
        text.title.Draw(pt, sideX + 16.0f * uiScale,
                     ry + (rowH - titleH) * 0.5f, titleScale,
                     sel ? 0.95f : 0.82f, sel ? 0.98f : 0.88f, 1.0f, 1.0f);
        if (hov && lmb && !state.previousLeftMouseDown) s_page = i;
    }

    const wchar_t* pageTitle = TutorialPageTitle(s_page);
    text.title.Draw(pageTitle, contentX, contentY,
                 UiTextScale(text.title, UiTextLevel::Title, uiScale),
                 0.55f, 0.90f, 1.0f, 1.0f);
    float bodyY = contentY + 58.0f * uiScale;
    bodyY = drawSlashText(TutorialPageBody(s_page), contentX, bodyY, contentW,
                          1.0f, 0.90f, 0.94f, 1.0f, 0.95f);

    const wchar_t* ctrl = TutorialControls();
    const float ctrlScale = UiTextScale(text.body, UiTextLevel::Supporting, uiScale);
    float ctrlW = text.body.Width(ctrl, ctrlScale);
    text.body.Draw(ctrl, contentX + (contentW - ctrlW) * 0.5f,
                 wy + WH - 108.0f * uiScale, ctrlScale,
                 0.50f, 0.80f, 1.0f, 0.90f);

    const wchar_t* PREV[LANG_COUNT] = { L"< 이전", L"< Prev", L"< 前へ" };
    const wchar_t* NEXT[LANG_COUNT] = { L"다음 >", L"Next >", L"次へ >" };
    float navY = wy + WH - 62.0f * uiScale;
    if (s_page > 0 && UIButton(wx + pad, navY, 130.0f * uiScale, 46.0f * uiScale,
                               PREV[li], mx, my, lmb, state.previousLeftMouseDown, false,
                               PanelButtonSlideSide::Both, uiScale, PREV, LANG_COUNT))
        s_page--;
    if (s_page < TUTORIAL_PAGE_COUNT - 1 &&
        UIButton(wx + pad + 140.0f * uiScale, navY,
                 130.0f * uiScale, 46.0f * uiScale, NEXT[li], mx, my,
                 lmb, state.previousLeftMouseDown, false, PanelButtonSlideSide::Both,
                 uiScale, NEXT, LANG_COUNT))
        s_page++;

    wchar_t pg[32];
    swprintf_s(pg, L"%d / %d", s_page + 1, TUTORIAL_PAGE_COUNT);
    const float pageScale = UiTextScale(text.body, UiTextLevel::Supporting, uiScale);
    float pgW = text.body.Width(pg, pageScale);
    text.body.Draw(pg, wx + WW * 0.5f - pgW * 0.5f,
                 navY + 14.0f * uiScale, pageScale, 0.7f, 0.8f, 0.9f, 0.95f);

    {
        static bool s_lPrev = false, s_rPrev = false;
        bool lk = (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS);
        bool rk = (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS);
        if (c.inputFocusChanged) {
            s_lPrev = lk;
            s_rPrev = rk;
        }
        if (lk && !s_lPrev && s_page > 0) s_page--;
        if (rk && !s_rPrev && s_page < TUTORIAL_PAGE_COUNT - 1) s_page++;
        s_lPrev = lk;
        s_rPrev = rk;
    }

    static const wchar_t* backVariants[] = { L"뒤로", L"BACK", L"戻る" };
    if (UIButton(wx + WW - pad - 180.0f * uiScale, navY,
                 180.0f * uiScale, 46.0f * uiScale, T(StrId::BTN_BACK),
                 mx, my, lmb, state.previousLeftMouseDown, false,
                 PanelButtonSlideSide::Both, uiScale, backVariants, LANG_COUNT))
        state.currentState = GameState::MAIN_MENU;
}

// Apply the selected fixed weapon and transition directly to READY.
//   직업제 개편: 총기 3택1 화면 없이 직업 = 고정 무기 1:1 매핑.
void Scene_Ready(const SceneCtx& c, const SceneTextContext& text) {
    const float sw = c.sw, sh = c.sh;
    const float uiScale = UiScale(sw, sh);
    (void)c;

    auto drawSlashText = [&](const wchar_t* content, float y, float sc, float r, float g, float b, float a) {
        if (!content || !content[0]) return;
        std::vector<std::wstring> lines;
        std::wstring cur;
        for (const wchar_t* p = content; *p; ++p) {
            if (*p == L'/' && (p == content || p[-1] == L' ' || p[1] == L' ')) {
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
        float lineH = 30.0f * uiScale;
        float maxW = sw * 0.78f;
        for (int li = 0; li < (int)lines.size(); li++) {
            float factor = sc * uiScale;
            float lsc = UiTextScale(text.body, UiTextLevel::Description, factor);
            while (factor > 0.55f && text.body.Width(lines[li].c_str(), lsc) > maxW) {
                factor -= 0.04f;
                lsc = UiTextScale(text.body, UiTextLevel::Description, factor);
            }
            float lw = text.body.Width(lines[li].c_str(), lsc);
            text.body.Draw(lines[li].c_str(), (sw - lw) * 0.5f, y + li * lineH, lsc, r, g, b, a);
        }
    };

    const wchar_t* title = ReadyTitle();
    const float titleScale = UiTextScale(text.title, UiTextLevel::Title, uiScale);
    text.title.Draw(title, CenterTextX(sw, text.title, title, titleScale), sh * 0.22f,
                 titleScale,
                 0.55f, 0.90f, 1.0f, 0.98f);

    drawSlashText(ReadyBrief(), sh * 0.36f, 1.0f, 0.92f, 0.95f, 1.0f, 0.92f);

    const wchar_t* hint = ReadyHint();
    const float hintScale = UiTextScale(text.body, UiTextLevel::Supporting, uiScale);
    text.body.Draw(hint, CenterTextX(sw, text.body, hint, hintScale), sh * 0.52f, hintScale,
                 0.55f, 0.82f, 1.0f, 0.88f);

    const wchar_t* T1 = T(StrId::PRESS_SPACE_TO_START);
    const wchar_t* T2 = T(StrId::ESC_QUIT);
    const float startScale = UiTextScale(text.title, UiTextLevel::Title, uiScale);
    const float quitScale = UiTextScale(text.body, UiTextLevel::Supporting, uiScale);
    text.title.Draw(T1, CenterTextX(sw, text.title, T1, startScale),
                 sh * 0.72f, startScale, 1, 1, 1, 0.95f);
    text.body.Draw(T2, CenterTextX(sw, text.body, T2, quitScale),
                 sh * 0.78f, quitScale, 0.8f, 0.8f, 0.8f, 0.8f);

    const wchar_t* ctrl = TutorialControls();
    text.body.Draw(ctrl, CenterTextX(sw, text.body, ctrl, quitScale), sh * 0.88f, quitScale,
                 0.55f, 0.85f, 1.0f, 0.95f);
}

