#pragma once
#include <vector>
#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>

// ─────────────────────────────────────────────────────────────
// 손맛(juice) 공용 시스템 — 데미지 숫자 / 콤보 / 히트스톱 / 화면 플래시
//   CollisionSystem 과 main 이 함께 쓰는 전역 상태 (header-only, inline)
// ─────────────────────────────────────────────────────────────

// ── 데미지 숫자 팝업 ──
struct DamageNumber {
    float x, y, vx, vy;
    float life, maxLife;
    int   amount;
    bool  crit;
};
inline std::vector<DamageNumber> g_DmgNumbers;

inline void SpawnDamageNumber(float x, float y, float amount, bool crit) {
    if (amount < 1.0f) return;
    if (g_DmgNumbers.size() >= 140) return;     // 풀 상한 (성능)
    DamageNumber d;
    d.x  = x + (float)((rand() % 24) - 12);
    d.y  = y - 8.0f;
    d.vx = (float)((rand() % 50) - 25);
    d.vy = -80.0f - (float)(rand() % 40);
    d.maxLife = d.life = crit ? 0.85f : 0.6f;
    d.amount  = (int)(amount + 0.5f);
    d.crit    = crit;
    g_DmgNumbers.push_back(d);
}

// ── 콤보 / 킬스트릭 ──
inline int   g_Combo      = 0;
inline float g_ComboTimer = 0.0f;     // 남은 유지 시간
inline float g_ComboPulse = 0.0f;     // 증가 시 팝 애니메이션 (0..1)
inline constexpr float COMBO_WINDOW = 3.0f;
inline void AddKillCombo() {
    ++g_Combo;
    g_ComboTimer = COMBO_WINDOW;
    g_ComboPulse = 1.0f;
}

// ── 히트스톱 (큰 이벤트 때 잠깐 정지) ──
inline float g_HitStopTimer = 0.0f;
inline void TriggerHitStop(float t) {
    if (t > g_HitStopTimer) g_HitStopTimer = t;
}

// ── 화면 플래시 ──
inline glm::vec3 g_FlashColor     = glm::vec3(1.0f);
inline float     g_FlashIntensity = 0.0f;
inline void TriggerFlash(float r, float g, float b, float intensity) {
    g_FlashColor = glm::vec3(r, g, b);
    if (intensity > g_FlashIntensity) g_FlashIntensity = intensity;
}

// 새 게임/리셋 시 호출
inline void ResetJuice() {
    g_DmgNumbers.clear();
    g_Combo = 0; g_ComboTimer = 0.0f; g_ComboPulse = 0.0f;
    g_HitStopTimer = 0.0f;
    g_FlashIntensity = 0.0f;
}
