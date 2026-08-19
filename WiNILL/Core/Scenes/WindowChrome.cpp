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
    BindMainShader();
    const float TB = 30.0f;
    drawRect(0, 0, sw, TB, ar*0.5f, ag*0.5f, ab*0.5f, 0.96f);
    drawRect(0, TB, sw, 2.0f, ar, ag, ab, 0.9f);
    drawRect(0, 0, sw, 1.5f, ar, ag, ab, 0.5f);
    drawRect(0, sh-1.5f, sw, 1.5f, ar, ag, ab, 0.5f);
    drawRect(0, 0, 1.5f, sh, ar, ag, ab, 0.5f);
    drawRect(sw-1.5f, 0, 1.5f, sh, ar, ag, ab, 0.5f);
    float bs = 14.0f, byc = (TB-bs)*0.5f, bxc = sw - 24.0f;
    drawRect(bxc - 2*(bs+8), byc, bs, bs, 1,1,1, 0.25f);
    drawRect(bxc - (bs+8),   byc, bs, bs, 1,1,1, 0.25f);
    drawRect(bxc, byc, bs, bs, 0.9f, 0.25f, 0.25f, 0.9f);
    g_TextS.Draw(fname, 14.0f, 5.0f, 0.62f, 0.95f, 0.97f, 1.0f, 1.0f);
}

void SceneFlowWindow(float sw, float sh, float WW, float WH,
                     const wchar_t* fname, float ar, float ag, float ab,
                     float& outX, float& outY, float& outContentY,
                     float dimAlpha, float bodyAlpha, float shadowAlpha) {
    float wx = (sw - WW) * 0.5f, wy = (sh - WH) * 0.5f;
    outX = wx;
    outY = wy;
    outContentY = wy + FLOW_CHROME_TOP;
    const float TB = 30.0f;
    BindMainShader();
    if (dimAlpha > 0.0f) drawRect(0, 0, sw, sh, 0.0f, 0.0f, 0.0f, dimAlpha);
    drawRect(wx + 6.0f, wy + 8.0f, WW, WH, 0.0f, 0.0f, 0.0f, shadowAlpha);
    drawRect(wx, wy, WW, WH, 0.07f, 0.08f, 0.11f, bodyAlpha);
    drawRect(wx, wy, WW, TB, ar * 0.5f, ag * 0.5f, ab * 0.55f, 1.0f);
    drawRect(wx, wy + TB, WW, 2.0f, ar, ag, ab, 0.9f);
    float bs = 13.0f, byc = wy + (TB - bs) * 0.5f, bxc = wx + WW - 22.0f;
    drawRect(bxc - 2 * (bs + 7), byc, bs, bs, 1, 1, 1, 0.25f);
    drawRect(bxc - (bs + 7), byc, bs, bs, 1, 1, 1, 0.25f);
    drawRect(bxc, byc, bs, bs, 0.9f, 0.25f, 0.25f, 0.9f);
    drawRect(wx, wy, WW, 1.5f, ar, ag, ab, 0.5f);
    drawRect(wx, wy + WH - 1.5f, WW, 1.5f, ar, ag, ab, 0.5f);
    drawRect(wx, wy, 1.5f, WH, ar, ag, ab, 0.5f);
    drawRect(wx + WW - 1.5f, wy, 1.5f, WH, ar, ag, ab, 0.5f);
    g_TextS.Draw(fname, wx + 12.0f, wy + 5.0f, 0.6f, 0.95f, 0.97f, 1.0f, 1.0f);
}

void SceneAppWindow(float sw, float sh, float WW, float WH,
                    const wchar_t* fname, float ar, float ag, float ab,
                    float& outX, float& outY, bool gameOverlay,
                    float dimAlpha) {
    float wx = (sw - WW) * 0.5f, wy = (sh - WH) * 0.5f;
    outX = wx; outY = wy;
    float op = g_AppOpen; if (op < 0.0f) op = 0.0f; if (op > 1.0f) op = 1.0f;
    float e  = Smoothstep(op);
    const float bodyA = gameOverlay ? 0.86f : 0.99f;
    BindMainShader();
    if (dimAlpha > 0.0f) drawRect(0, 0, sw, sh, 0.0f, 0.0f, 0.0f, dimAlpha * e);
    if (e < 0.999f) {
        float dw = WW * e, dh = WH * e;
        float dx = sw*0.5f - dw*0.5f, dy = sh*0.5f - dh*0.5f;
        drawRect(dx+5, dy+6, dw, dh, 0.0f,0.0f,0.0f, 0.30f);
        drawRect(dx, dy, dw, dh, 0.07f, 0.08f, 0.11f, bodyA);
        drawRect(dx, dy, dw, 4.0f, ar, ag, ab, 1.0f);
        drawRect(dx, dy, dw, 1.5f, ar,ag,ab,0.5f);
        drawRect(dx, dy+dh-1.5f, dw, 1.5f, ar,ag,ab,0.5f);
        drawRect(dx, dy, 1.5f, dh, ar,ag,ab,0.5f);
        drawRect(dx+dw-1.5f, dy, 1.5f, dh, ar,ag,ab,0.5f);
        return;
    }
    drawRect(wx+7, wy+9, WW, WH, 0.0f, 0.0f, 0.0f, 0.35f);
    drawRect(wx, wy, WW, WH, 0.07f, 0.08f, 0.11f, bodyA);
    const float TB = 30.0f;
    drawRect(wx, wy, WW, TB, ar*0.5f, ag*0.5f, ab*0.55f, 1.0f);
    drawRect(wx, wy+TB, WW, 2.0f, ar, ag, ab, 0.9f);
    float bs=13.0f, byc=wy+(TB-bs)*0.5f, bxc=wx+WW-22.0f;
    drawRect(bxc-2*(bs+7), byc, bs,bs, 1,1,1,0.25f);
    drawRect(bxc-(bs+7),   byc, bs,bs, 1,1,1,0.25f);
    drawRect(bxc, byc, bs,bs, 0.9f,0.25f,0.25f,0.9f);
    drawRect(wx, wy, WW, 1.5f, ar,ag,ab,0.5f);
    drawRect(wx, wy+WH-1.5f, WW, 1.5f, ar,ag,ab,0.5f);
    drawRect(wx, wy, 1.5f, WH, ar,ag,ab,0.5f);
    drawRect(wx+WW-1.5f, wy, 1.5f, WH, ar,ag,ab,0.5f);
    g_TextS.Draw(fname, wx+12.0f, wy+5.0f, 0.6f, 0.95f,0.97f,1.0f,1.0f);
    outX = wx; outY = wy;
}

void DrawBrowserChrome(float sw, float sh, GameState st,
                       double mx, double my, bool lmb, bool lmbPrev,
                       GameState& outSt,
                       float barRatio, float br, float bg, float bb)
{
    using GS = GameState;
    outSt = st;
    (void)sh; (void)mx; (void)my; (void)lmb; (void)lmbPrev;

    const float H = BROWSER_CHROME_H;  // 31px

    // ── 배경 + 하단 강조선 ────────────────────────────────────────
    BindMainShader();
    drawRect(0, 0, sw, H, 0.052f, 0.062f, 0.082f, 1.0f);
    drawRect(0, H - 1.5f, sw, 1.5f, 0.18f, 0.28f, 0.46f, 0.55f);

    // ── 주소창 pill ───────────────────────────────────────────────
    const float ADDR_L = 6.0f;
    const float ADDR_R = sw - 6.0f;
    const float ADDR_W = ADDR_R - ADDR_L;
    const float ADDR_H = 19.0f;
    const float ADDR_Y = (H - ADDR_H) * 0.5f;
    drawRect(ADDR_L, ADDR_Y, ADDR_W, ADDR_H, 0.088f, 0.108f, 0.155f, 0.92f);
    // 진행 바 fill (보스 HP or 점수 진행)
    if (barRatio > 0.0f) {
        float fw = ADDR_W * (barRatio > 1.0f ? 1.0f : barRatio);
        drawRect(ADDR_L, ADDR_Y, fw, ADDR_H, br, bg, bb, 0.38f);
    }
    drawRect(ADDR_L, ADDR_Y,                  ADDR_W, 1.0f, 0.22f, 0.32f, 0.52f, 0.45f);
    drawRect(ADDR_L, ADDR_Y + ADDR_H - 1.0f, ADDR_W, 1.0f, 0.22f, 0.32f, 0.52f, 0.45f);
    drawRect(ADDR_L, ADDR_Y,                  1.0f, ADDR_H, 0.22f, 0.32f, 0.52f, 0.45f);
    drawRect(ADDR_R - 1.0f, ADDR_Y,           1.0f, ADDR_H, 0.22f, 0.32f, 0.52f, 0.45f);

    // ── 자물쇠 아이콘 (12×15px) ──────────────────────────────────
    const float LX = ADDR_L + 8.0f;
    const float LY = ADDR_Y + (ADDR_H - 15.0f) * 0.5f;
    bool gameMode = (st == GS::RUNNING || st == GS::DYING ||
                     st == GS::GAMEOVER || st == GS::VICTORY ||
                     st == GS::AUG_SELECT || st == GS::DEBUFF_SELECT ||
                     st == GS::RUN_SHOP);
    float lr = gameMode ? 0.95f : 0.28f;
    float lg = gameMode ? 0.65f : 0.88f;
    float lb = gameMode ? 0.20f : 0.48f;
    drawRect(LX + 2.0f, LY,        2.0f, 8.0f, lr, lg, lb, 0.88f);  // shackle left
    drawRect(LX + 8.0f, LY,        2.0f, 8.0f, lr, lg, lb, 0.88f);  // shackle right
    drawRect(LX + 2.0f, LY,        8.0f, 2.0f, lr, lg, lb, 0.88f);  // shackle top
    drawRect(LX,        LY + 6.0f, 12.0f, 9.0f, lr, lg, lb, 0.88f); // body
    drawRect(LX + 5.0f, LY + 9.0f,  2.0f, 4.0f, 0.05f, 0.06f, 0.09f, 0.85f); // keyhole

    // ── URL 텍스트 ────────────────────────────────────────────────
    const wchar_t* url = BrowserUrl(st);
    const float URL_X  = LX + 16.0f;
    const float URL_Y  = ADDR_Y + (ADDR_H - g_TextS.Height(url, 0.70f)) * 0.5f;
    g_TextS.Draw(url, URL_X, URL_Y, 0.70f,
                 gameMode ? 0.92f : 0.68f,
                 gameMode ? 0.96f : 0.80f,
                 gameMode ? 1.00f : 1.00f, 0.95f);
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
