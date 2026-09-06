#pragma once
#include <glad/glad.h>

// SDF 원형 행성 렌더러
// DrawPlanetSDF 호출 전 BatchFlush() 불필요 — 내부에서 처리함
// 반환 후 MainShader 가 재바인드된 상태

void InitPlanetShader(int screenW, int screenH);

// cx,cy : 화면 픽셀 좌표 (Y-down)
// radius: 행성 반지름 (px)
// r,g,b : 행성 색
// glow  : 0=없음, 1=강한 글로우
// alpha : 전체 투명도
// ring  : 0=솔리드, 1=테두리 하이라이트 강조
void DrawPlanetSDF(float cx, float cy, float radius,
                   float r, float g, float b,
                   float glow, float alpha, float ring = 0.0f);
