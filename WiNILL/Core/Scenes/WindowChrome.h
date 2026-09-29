#pragma once

// ── 브라우저 크롬 ──────────────────────────────────────────────────
constexpr float BROWSER_CHROME_H = 0.0f;    // 타이틀바 제거

void SceneAppWindow(float sw, float sh, float WW, float WH,
                    const wchar_t* fname, float ar, float ag, float ab,
                    float& outX, float& outY, bool gameOverlay = false,
                    float dimAlpha = 0.0f);
