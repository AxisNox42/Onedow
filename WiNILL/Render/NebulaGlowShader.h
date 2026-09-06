#pragma once
#include <glad/glad.h>

// 소프트 성운 글로우 쿼드 렌더러 (SDF 원형 금지 — 순수 방사형 그라디언트)
// DrawNebulaGlow 호출 전 BatchFlush() 불필요 — 내부에서 처리
// 반환 후 MainShader 가 재바인드된 상태
void InitNebulaGlowShader(int screenW, int screenH);

// cx,cy   : 화면 픽셀 좌표 (Y-down)
// radius  : 글로우 반지름 (px) — 이 거리에서 알파 0
// r,g,b   : 색상 (rainbow=true 시 무시)
// alpha   : 중심 최대 알파
// time    : 현재 시간 (rainbow 애니메이션용)
// rainbow : true = SPECIAL 등급 무지개 모드
void DrawNebulaGlow(float cx, float cy, float radius,
                    float r, float g, float b, float alpha,
                    float time = 0.0f, bool rainbow = false);
