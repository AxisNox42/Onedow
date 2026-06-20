#pragma once

// ─────────────────────────────────────────────────────────────
// 윈도우 투명도 헬퍼 (크로스플랫폼)
//   Windows : DWM per-pixel 알파 합성 (DwmExtendFrameIntoClientArea)
//   macOS/Linux : GLFW_TRANSPARENT_FRAMEBUFFER 힌트로 처리 → 이 함수는 no-op
// ─────────────────────────────────────────────────────────────
struct GLFWwindow;

void EnableWindowTransparency(GLFWwindow* window);

// transparency_debug.txt 에 로그 기록 (printf 스타일, 크로스플랫폼)
void TransparencyLog(const char* fmt, ...);
