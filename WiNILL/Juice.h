#pragma once
#include <vector>
#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#include "Audio.h"

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

// ── 타격 스파크 (명중/피격 불꽃) ──
struct Spark {
    float x, y, vx, vy;
    float life, maxLife, size;
    float r, g, b;
};
inline std::vector<Spark> g_Sparks;

// ── 플레이어 이동 잔상(afterimage) — 이동 시 과거 위치에 옅게 남았다 사라짐 ──
struct Trail { float x, y, life, maxLife, size, r, g, b; };
inline std::vector<Trail> g_Trail;
inline void SpawnTrail(float x, float y, float size, float r, float g, float b) {
    if ((int)g_Trail.size() > 120) return;
    Trail t; t.x = x; t.y = y; t.maxLife = t.life = 0.30f;
    t.size = size; t.r = r; t.g = g; t.b = b;
    g_Trail.push_back(t);
}
inline void SpawnSparks(float x, float y, int n,
                        float r, float g, float b, float speed = 300.0f) {
    if ((int)g_Sparks.size() > 500) return;          // 풀 상한
    for (int i = 0; i < n; i++) {
        float a = (float)(rand() % 628) * 0.01f;
        float s = speed * (0.35f + (rand() % 100) * 0.01f);
        Spark sp;
        sp.x = x; sp.y = y;
        sp.vx = cosf(a) * s; sp.vy = sinf(a) * s;
        sp.maxLife = sp.life = 0.16f + (rand() % 80) * 0.001f;
        sp.size = 2.5f + (float)(rand() % 3);
        sp.r = r; sp.g = g; sp.b = b;
        g_Sparks.push_back(sp);
    }
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

// ── 콤보 / 킬스트릭 ──
inline int   g_Combo      = 0;
inline float g_ComboTimer = 0.0f;     // 남은 유지 시간
inline float g_ComboPulse = 0.0f;     // 증가 시 팝 애니메이션 (0..1)
inline float g_ComboMilestone = 0.0f; // 마일스톤 달성 강조 연출 잔여(초)
inline constexpr float COMBO_WINDOW = 3.0f;
inline bool ComboIsMilestone(int c) {
    return c == 10 || c == 25 || c == 50 || c == 100 ||
           (c > 100 && c % 50 == 0);
}
// 무한 세례(신화) — 처치 수 누적, main 의 탄환세례 업데이트가 소비해 쿨다운 감소
inline float g_RainKillAccum = 0.0f;
inline void AddKillCombo() {
    ++g_Combo;
    g_ComboTimer = COMBO_WINDOW;
    g_ComboPulse = 1.0f;
    g_RainKillAccum += 1.0f;            // 처치 1건 (무한 세례용)
    Audio::PlaySfx(Audio::Sfx::Kill);   // 적 처치음
    // 콤보 마일스톤(10/25/50/100/…) — 카운터 강조 연출 + 칩 사운드 (전체화면 번쩍 X — 눈뽕 방지)
    if (ComboIsMilestone(g_Combo)) {
        g_ComboMilestone = 0.7f;
        Audio::PlaySfx(Audio::Sfx::Phase2);        // 마일스톤 칩(페이즈2 사운드 재사용)
    }
}

// 새 게임/리셋 시 호출
inline void ResetJuice() {
    g_DmgNumbers.clear();
    g_Sparks.clear();
    g_Trail.clear();
    g_Combo = 0; g_ComboTimer = 0.0f; g_ComboPulse = 0.0f;
    g_ComboMilestone = 0.0f;
    g_RainKillAccum = 0.0f;
    g_HitStopTimer = 0.0f;
    g_FlashIntensity = 0.0f;
}
