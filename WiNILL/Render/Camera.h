#pragma once
#include <glad/glad.h>
#include "DrawPrim.h"

// ── 디스플레이 / 줌 카메라 (월드 ↔ 스크린, 가짜창 scissor) ──
extern int   screenWidth;
extern int   screenHeight;
extern float g_ViewZoom;
extern float g_ViewZoomTarget;
extern float g_ZoomCX;
extern float g_ZoomCY;

inline float ZCX() { return g_ZoomCX > 0.0f ? g_ZoomCX : screenWidth  * 0.5f; }
inline float ZCY() { return g_ZoomCY > 0.0f ? g_ZoomCY : screenHeight * 0.5f; }
inline float W2SX(float wx) { return ZCX() + (wx - ZCX()) * g_ViewZoom; }
inline float W2SY(float wy) { return ZCY() + (wy - ZCY()) * g_ViewZoom; }
inline float ScreenToWorldX(float sx) { return ZCX() + (sx - ZCX()) / g_ViewZoom; }
inline float ScreenToWorldY(float sy) { return ZCY() + (sy - ZCY()) / g_ViewZoom; }

inline void WorldScissor(float wx, float wy, float ww, float wh) {
    BatchFlush();
    float sx = W2SX(wx), sy = W2SY(wy);
    float sw = ww * g_ViewZoom, sh = wh * g_ViewZoom;
    glScissor((GLint)sx, (GLint)(screenHeight - (sy + sh)), (GLint)sw, (GLint)sh);
}
