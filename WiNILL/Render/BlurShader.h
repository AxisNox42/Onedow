#pragma once
#include <glad/glad.h>

// ── Backdrop Blur (Frosted-Glass 패널용) ──────────────────────────────
// 아키텍처: full-res 캡처 → 1/4 해상도 다운샘플 → 3× H+V 블러 (7패스 총계)
// scissor 상태를 자동 저장/복원 — 다른 UI 오염 없음
//
// 사용 순서:
//   1. InitBlurSystem(sw, sh)  — 초기화 시 1회
//   2. CaptureBackdrop()       — DrawMenuBackground() + BatchFlush() 직후
//   3. DrawBlurPanel(...)      — 블러 패널 영역 출력

void InitBlurSystem(int screenW, int screenH);
void ResizeBlurSystem(int screenW, int screenH);

// 현재 프레임버퍼를 캡처하고 H→V 2-pass Gaussian blur 적용.
// BatchFlush() 후 호출할 것.
void CaptureBackdrop();

// 블러된 배경을 screen-space 사각형에 blit.
// alpha: 패널 불투명도  tint*: 별자리 테마 색조 (accentR/G/B * 0.05 ~ 0.10 수준)
void DrawBlurPanel(float x, float y, float w, float h,
                   float alpha,
                   float tintR, float tintG, float tintB);
