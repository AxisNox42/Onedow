#pragma once
#include <cmath>

// ─────────────────────────────────────────────────────────────
// 레벨/경험치 시스템
//   nextLevelEXP(level) = baseExp * (level ^ weight)
//   level 1 → 2 로 가는 데 필요한 EXP : baseExp * 1^weight = baseExp
//   level 2 → 3                       : baseExp * 2^weight
//   level 3 → 4                       : baseExp * 3^weight
//   ...
//   weight ≈ 1.4 → 자연스러운 곡선 (L5 ≒ 953, L10 ≒ 2512)
// ─────────────────────────────────────────────────────────────
struct ExpSystem {
    float baseExp = 100.0f;
    float weight  = 1.4f;

    // 현재 레벨에서 다음 레벨까지 필요한 EXP
    // 20레벨마다 baseExp 급증: 1-19=100, 20-39=2000, 40-59=4000, 60-79=6000 ...
    long long Required(int currentLevel) const {
        if (currentLevel < 1) currentLevel = 1;
        int   tier = currentLevel / 20; // 0=Lv1-19, 1=Lv20-39, 2=Lv40-59 ...
        float eff  = (tier == 0) ? 100.0f : (float)(tier * 2000);
        return (long long)(eff * std::pow((float)currentLevel, weight));
    }
};

inline ExpSystem g_ExpSystem;
