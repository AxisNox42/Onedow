#pragma once
#include <cmath>
#include "Experience.h"

// ─────────────────────────────────────────────────────────────
// 레벨/경험치 시스템 — 재설계 (B8)
//   목표: "초반엔 빨리 레벨업(파워 급성장) → 후반 갈수록 점점 어려워짐(정체)".
//   단일 매끄러운 볼록(convex) 곡선: Required(L) = baseExp * L^weight
//     baseExp=6, weight=2.4 →
//       L1=6   (잡몹 ~6마리면 첫 레벨업 — 극초반 쾌속)
//       L2=31  L3=83  L5=285
//       L10=1507  L20=7954  L30=21049  L40=41985  L60=111100
//   (잡몹 1xp, 큰몹/엘리트 3~20xp, AoE 후반 폭딜이라 후반 통이 커도 시간당 획득은 늘어남)
//   이전: baseExp=100·weight=1.4 + 20레벨 tier 점프 → 초반(L1=100)이 너무 비싸
//         "초반=6마리" 의도와 정반대였음. tier 불연속도 제거해 매끈한 곡선으로.
// ─────────────────────────────────────────────────────────────
struct ExpSystem {
    float baseExp = 6.0f;
    float weight  = 2.4f;
    // 현재 레벨에서 다음 레벨까지 필요한 EXP
    //   The threshold remains whole XP; earned XP keeps three decimals.
    long long Required(int currentLevel) const {
        if (currentLevel < 1) currentLevel = 1;
        return (long long)std::floor((double)baseExp * std::pow((double)currentLevel, (double)weight));
    }

    XpAmount RequiredAmount(int currentLevel) const {
        return XpFromWhole(Required(currentLevel));
    }

    bool ConsumeNextLevel(XpAmount& xp, int& level) const {
        const XpAmount need = RequiredAmount(level);
        if (need <= 0 || xp < need) return false;
        xp -= need;
        ++level;
        return true;
    }
};

inline ExpSystem g_ExpSystem;

constexpr int MAIN_LEVEL_CAP = 45;
