#include "WindowChrome.h"
#include "SceneContext.h"
#include "DrawPrim.h"
#include "TextRenderer.h"
#include "Settings.h"
#include "GameManager.h"
#include "Platform.h"
#include "UiLayout.h"
#include "UiColors.h"
#include <cmath>

extern TextRenderer g_TextS;

void SceneDeskWindow(float sw, float sh, const wchar_t* fname,
                     float ar, float ag, float ab) {
    (void)fname;
    BindMainShader();
    drawConstellFrame(0.0f, 0.0f, sw, sh, ar, ag, ab, 0.28f,
                      30.0f, 7.0f, 0.10f, 1.0f);
}

void SceneFlowWindow(float sw, float sh, float WW, float WH,
                     const wchar_t* fname, float ar, float ag, float ab,
                     float& outX, float& outY, float& outContentY,
                     float dimAlpha, float bodyAlpha, float shadowAlpha) {
    (void)fname;
    (void)dimAlpha;
    (void)bodyAlpha;
    (void)shadowAlpha;
    float wx = (sw - WW) * 0.5f, wy = (sh - WH) * 0.5f;
    outX = wx;
    outY = wy;
    outContentY = wy + FLOW_CHROME_TOP;
    BindMainShader();
    drawConstellFrame(wx, wy, WW, WH, ar, ag, ab, 0.42f,
                      26.0f, 6.0f, 0.12f, 1.0f);
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
    BindMainShader();
    if (e < 0.999f) {
        float dw = WW * e, dh = WH * e;
        float dx = sw*0.5f - dw*0.5f, dy = sh*0.5f - dh*0.5f;
        if (dw > 24.0f && dh > 24.0f) {
            const float frameA = (gameOverlay ? 0.32f : 0.42f) * e;
            drawConstellFrame(dx, dy, dw, dh, ar, ag, ab, frameA,
                              22.0f, 5.0f, 0.10f * e, e);
        }
        return;
    }
    drawConstellFrame(wx, wy, WW, WH, ar, ag, ab,
                      gameOverlay ? 0.32f : 0.42f,
                      22.0f, 5.0f, 0.10f, 1.0f);
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
    BindMainShader();
    const float tbH = g_GameBarH;
    if (tbH <= 0.0f) return;
    const float tbY = sh - (float)g_TaskbarH - tbH;
    drawRect(0.0f, tbY, sw, tbH, 0.03f, 0.07f, 0.12f, 0.72f);
    drawRectCol3(0.0f, tbY, sw, 1.3f, UiCol::TASKBAR_TOP, 0.72f);
    drawDiamond(18.0f, tbY + tbH * 0.5f, 6.0f,
                UiCol::ACCENT_CYAN.r, UiCol::ACCENT_CYAN.g,
                UiCol::ACCENT_CYAN.b, 0.92f);
    g_TextS.Draw(L"ORBITAL TELEMETRY", 34.0f, tbY + (tbH - 15.0f) * 0.5f, 0.64f,
                 0.72f, 0.92f, 1.0f, 0.96f);
    const wchar_t* stat = (st == GameState::PAUSED)
        ? L"|| SIGNAL SUSPENDED"
        : L"* STELLAR FIELD ACTIVE";
    float stw = g_TextS.Width(stat, 0.66f);
    g_TextS.Draw(stat, CenterX(sw, stw), tbY + (tbH - 14.0f) * 0.5f, 0.66f,
                 0.6f, 0.85f, 1.0f, 0.9f);
}
