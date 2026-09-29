#include "WindowChrome.h"
#include "SceneContext.h"
#include "DrawPrim.h"
#include "GameManager.h"
#include "UiLayout.h"

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
