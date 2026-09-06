#include "WindowChrome.h"
#include "SceneContext.h"
#include "DrawPrim.h"
#include "TextRenderer.h"
#include "Settings.h"
#include "GameManager.h"
#include "Platform.h"
#include "UiLayout.h"
#include "UiColors.h"
#include <ctime>
#include <cmath>

extern TextRenderer g_TextS;

void SceneDeskWindow(float sw, float sh, const wchar_t* fname,
                     float ar, float ag, float ab) {
    (void)fname;
    BindMainShader();
    drawRect(0, 0,        sw, 1.5f, ar, ag, ab, 0.5f);
    drawRect(0, sh-1.5f,  sw, 1.5f, ar, ag, ab, 0.5f);
    drawRect(0, 0,        1.5f, sh, ar, ag, ab, 0.5f);
    drawRect(sw-1.5f, 0,  1.5f, sh, ar, ag, ab, 0.5f);
}

void SceneFlowWindow(float sw, float sh, float WW, float WH,
                     const wchar_t* fname, float ar, float ag, float ab,
                     float& outX, float& outY, float& outContentY,
                     float dimAlpha, float bodyAlpha, float shadowAlpha) {
    (void)fname;
    (void)dimAlpha;
    float wx = (sw - WW) * 0.5f, wy = (sh - WH) * 0.5f;
    outX = wx;
    outY = wy;
    outContentY = wy + FLOW_CHROME_TOP;
    BindMainShader();
    drawRect(wx + 6.0f, wy + 8.0f, WW, WH, 0.0f, 0.0f, 0.0f, shadowAlpha);
    drawRect(wx, wy, WW, WH, 0.07f, 0.08f, 0.11f, bodyAlpha);
    drawRect(wx,           wy,        WW,   1.5f, ar, ag, ab, 0.5f);
    drawRect(wx,           wy+WH-1.5f,WW,   1.5f, ar, ag, ab, 0.5f);
    drawRect(wx,           wy,        1.5f, WH,   ar, ag, ab, 0.5f);
    drawRect(wx+WW-1.5f,   wy,        1.5f, WH,   ar, ag, ab, 0.5f);
}

void SceneAppWindow(float sw, float sh, float WW, float WH,
                    const wchar_t* fname, float ar, float ag, float ab,
                    float& outX, float& outY, bool gameOverlay,
                    float dimAlpha) {
    (void)fname;
    (void)dimAlpha;
    float wx = (sw - WW) * 0.5f, wy = (sh - WH) * 0.5f;
    outX = wx; outY = wy;
    float op = g_AppOpen; if (op < 0.0f) op = 0.0f; if (op > 1.0f) op = 1.0f;
    float e  = Smoothstep(op);
    const float bodyA = gameOverlay ? 0.86f : 0.99f;
    BindMainShader();
    if (e < 0.999f) {
        float dw = WW * e, dh = WH * e;
        float dx = sw*0.5f - dw*0.5f, dy = sh*0.5f - dh*0.5f;
        drawRect(dx+5, dy+6, dw, dh, 0.0f, 0.0f, 0.0f, 0.30f);
        drawRect(dx, dy, dw, dh, 0.07f, 0.08f, 0.11f, bodyA);
        drawRect(dx,       dy,       dw,   1.5f, ar, ag, ab, 0.5f);
        drawRect(dx,       dy+dh-1.5f,dw,  1.5f, ar, ag, ab, 0.5f);
        drawRect(dx,       dy,       1.5f, dh,   ar, ag, ab, 0.5f);
        drawRect(dx+dw-1.5f,dy,      1.5f, dh,   ar, ag, ab, 0.5f);
        return;
    }
    drawRect(wx+7, wy+9, WW, WH, 0.0f, 0.0f, 0.0f, 0.35f);
    drawRect(wx, wy, WW, WH, 0.07f, 0.08f, 0.11f, bodyA);
    drawRect(wx,         wy,        WW,   1.5f, ar, ag, ab, 0.5f);
    drawRect(wx,         wy+WH-1.5f,WW,   1.5f, ar, ag, ab, 0.5f);
    drawRect(wx,         wy,        1.5f, WH,   ar, ag, ab, 0.5f);
    drawRect(wx+WW-1.5f, wy,        1.5f, WH,   ar, ag, ab, 0.5f);
    outX = wx; outY = wy;
}

void DrawBrowserChrome(float sw, float sh, GameState st,
                       double mx, double my, bool lmb, bool lmbPrev,
                       GameState& outSt,
                       float barRatio, float br, float bg, float bb)
{
    (void)sw; (void)sh; (void)mx; (void)my; (void)lmb; (void)lmbPrev;
    (void)barRatio; (void)br; (void)bg; (void)bb;
    outSt = st;
}

void DrawIngameTaskbar(float sw, float sh, GameState st) {
    const int li = LangIndex();
    BindMainShader();
    const float tbH = g_GameBarH;
    const float tbY = sh - (float)g_TaskbarH - tbH;
    drawRectCol(0, tbY, sw, tbH, UiCol::TASKBAR_BG);
    drawRectCol3(0, tbY, sw, 2.0f, UiCol::TASKBAR_TOP, 0.9f);
    drawRectCol3(9.0f, tbY + (tbH - 22.0f) * 0.5f, 22.0f, 22.0f, UiCol::ACCENT_CYAN, 0.95f);
    g_TextS.Draw(L"onedow.exe", 40.0f, tbY + (tbH - 15.0f) * 0.5f, 0.68f,
                 0.9f, 0.96f, 1.0f, 1.0f);
    const wchar_t* STAT_RUN[3] = { L"* 데스크톱 방어 중", L"* Defending desktop", L"* デスクトップ防衛中" };
    const wchar_t* STAT_PAU[3] = { L"|| 일시정지", L"|| Paused", L"|| 一時停止" };
    const wchar_t* stat = (st == GameState::PAUSED) ? STAT_PAU[li] : STAT_RUN[li];
    float stw = g_TextS.Width(stat, 0.66f);
    g_TextS.Draw(stat, CenterX(sw, stw), tbY + (tbH - 14.0f) * 0.5f, 0.66f,
                 0.6f, 0.85f, 1.0f, 0.9f);
    time_t tt = time(nullptr); struct tm lt; localtime_s(&lt, &tt);
    wchar_t clk[16]; swprintf_s(clk, L"%02d:%02d", lt.tm_hour, lt.tm_min);
    float clw = g_TextS.Width(clk, 0.72f);
    g_TextS.Draw(clk, sw - clw - 16.0f, tbY + (tbH - 15.0f) * 0.5f, 0.72f,
                 0.85f, 0.92f, 1.0f, 1.0f);
}
