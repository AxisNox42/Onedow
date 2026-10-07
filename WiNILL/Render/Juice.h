#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <glm/glm.hpp>
#include "Platform.h"
#include "Audio.h"
#include "Camera.h"
#include "TextRenderer.h"
#include "EnemyParticles.h"

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

inline int JuiceDmgCap() {
#if defined(__APPLE__)
    return 90;
#else
    return 140;
#endif
}
inline int JuiceTrailCap() {
#if defined(__APPLE__)
    return 80;
#else
    return 120;
#endif
}

inline void SpawnDamageNumber(float x, float y, float amount, bool crit) {
    // Keep even fractional/DoT hits visible as at least "1".
    if (amount <= 0.0f) return;
    if ((int)g_DmgNumbers.size() >= JuiceDmgCap())
        g_DmgNumbers.erase(g_DmgNumbers.begin());

    DamageNumber d;
    d.x = x + (float)((rand() % 17) - 8);
    d.y = y - 14.0f;
    d.vx = (float)((rand() % 41) - 20);
    d.vy = -90.0f - (float)(rand() % 35);
    d.maxLife = d.life = crit ? 0.75f : 0.62f;
    d.amount = std::max(1, (int)std::floor(amount + 0.5f));
    d.crit = crit;
    g_DmgNumbers.push_back(d);
}

// ── 플레이어 이동 잔상(afterimage) — 이동 시 과거 위치에 옅게 남았다 사라짐 ──
struct Trail { float x, y, life, maxLife, size, r, g, b; };
inline std::vector<Trail> g_Trail;
inline void SpawnTrail(float x, float y, float size, float r, float g, float b) {
    if ((int)g_Trail.size() > JuiceTrailCap()) return;
    Trail t; t.x = x; t.y = y; t.maxLife = t.life = 0.30f;
    t.size = size; t.r = r; t.g = g; t.b = b;
    g_Trail.push_back(t);
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
    }
}

// ── 적 사망 폭발 파티클 ──
inline void SpawnEnemyExplosion(float ex, float ey,
                                float cr, float cg, float cb, bool big) {
    SpawnSparks(ex, ey, big ? 8 : 4, 1.0f, 0.95f, 0.7f,
                big ? 420.0f : 320.0f);
    int count   = big ? 20 : 10;
    float baseS = big ? 200.0f : 100.0f;
    float varS  = big ? 250.0f : 150.0f;
    float lifeT = big ? 0.45f  : 0.30f;
    int placed = 0, j = 0;
    while (placed < count && j < EnemyParticleCap()) {
        if (!g_EnemyParts[j].active) {
            float angle = (float)placed / (float)count * 6.2831853f
                        + ((float)(rand() % 100) - 50.0f) * 0.012f;
            float spd   = baseS + (float)(rand() % (int)varS);
            float sz    = big ? (float)(5 + rand() % 10)
                              : (float)(3 + rand() % 5);
            g_EnemyParts[j] = {
                ex, ey,
                cosf(angle) * spd, sinf(angle) * spd,
                lifeT, lifeT, sz, cr, cg, cb, true
            };
            g_EnemyParts[j].shape = RandomEnemyParticleShape();
            ++placed;
        }
        ++j;
    }
}

inline void UpdateEnemyFx(float delta) {
    UpdateEnemyParticles(delta);
}

inline void ResetEnemyFx() {
    ResetEnemyParticles();
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
    ResetEnemyFx();
}

// ── 별가루 픽업 ──
struct StardustPickup {
    float x, y;
    float vx, vy;
    float age   = 0.0f;
    int   value = 1;
    long long xpValue = 0;
    bool  alive = true;
};
inline std::vector<StardustPickup> g_StardustPickups;
inline float g_StardustHudPulse = 0.0f;

inline void DrawStardustPickups() {
    for (const auto& dust : g_StardustPickups) {
        if (!dust.alive) continue;
        const float radius = (dust.value >= 10 ? 7.0f : dust.value >= 5 ? 5.5f : 4.0f)
                           * (1.0f + 0.12f * sinf(dust.age * 8.0f));
        auto diamond = [&](float size, float r, float g, float b, float alpha) {
            BatchTri(dust.x, dust.y - size, dust.x - size, dust.y,
                     dust.x + size, dust.y, r, g, b, alpha);
            BatchTri(dust.x - size, dust.y, dust.x, dust.y + size,
                     dust.x + size, dust.y, r, g, b, alpha);
        };
        diamond(radius * 1.8f, 1.0f, 0.72f, 0.16f, 0.18f);
        diamond(radius, 1.0f, 0.85f, 0.30f, 0.95f);
        diamond(radius * 0.38f, 1.0f, 1.0f, 0.88f, 1.0f);
    }
}

inline void SpawnStardust(float x, float y, int totalValue,
                          float /*playerX*/, float /*playerY*/,
                          long long xpReward = 0) {
    // XP를 지급해야 하는 처치라면, 기존에 화폐 별가루를 주지 않던 몹도
    // XP를 담은 최소 1개 조각을 반드시 생성한다.
    if (totalValue <= 0) {
        if (xpReward <= 0) return;
        totalValue = 1;
    }
    const int totalDustValue = totalValue;
    const std::size_t pickupBegin = g_StardustPickups.size();
    for (int denomination : { 10, 5, 1 }) {
        while (totalValue >= denomination) {
            totalValue -= denomination;
            // Scatter briefly before the pickup update starts homing.
            const float angle = (float)(rand() % 628) * 0.01f;
            const float speed = 120.0f + (float)(rand() % 80);
            g_StardustPickups.push_back({
                x, y,
                cosf(angle) * speed, sinf(angle) * speed,
                0.0f, denomination, 0, true
            });
        }
    }

    // 여러 조각으로 나뉘어도 XP가 조각마다 중복 지급되지 않도록 비례 분배한다.
    if (xpReward <= 0 || pickupBegin >= g_StardustPickups.size()) return;
    long long remainingXp = xpReward;
    int remainingValue = totalDustValue;
    const std::size_t pickupEnd = g_StardustPickups.size();
    for (std::size_t i = pickupBegin; i < pickupEnd && remainingXp > 0; ++i) {
        const int pieceValue = g_StardustPickups[i].value;
        const bool isLast = (i + 1 == pickupEnd);
        long long pieceXp = isLast
            ? remainingXp
            : (remainingXp * (long long)pieceValue + remainingValue / 2)
              / remainingValue;
        if (pieceXp < 0) pieceXp = 0;
        if (pieceXp > remainingXp) pieceXp = remainingXp;
        g_StardustPickups[i].xpValue = pieceXp;
        remainingXp -= pieceXp;
        remainingValue -= pieceValue;
    }
}
