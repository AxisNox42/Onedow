#pragma once
#include <cmath>

// ─────────────────────────────────────────────────────────────
// 레벨/경험치 시스템 — 재설계 (B8)
//   목표: "초반엔 빨리 레벨업(파워 급성장) → 후반 갈수록 점점 어려워짐(정체)".
//   단일 매끄러운 볼록(convex) 곡선: Required(L) = baseExp * L^weight
//     baseExp=6, weight=2.4 →
//       L1=6   (잡몹 ~6마리면 첫 레벨업 — 극초반 쾌속)
//       L2=32  L3=83  L5=255
//       L10≈1.5k  L20≈7.9k  L30≈22k  L40≈44k  L60≈120k
//   (잡몹 1xp, 큰몹/엘리트 3~20xp, AoE 후반 폭딜이라 후반 통이 커도 시간당 획득은 늘어남)
//   이전: baseExp=100·weight=1.4 + 20레벨 tier 점프 → 초반(L1=100)이 너무 비싸
//         "초반=6마리" 의도와 정반대였음. tier 불연속도 제거해 매끈한 곡선으로.
// ─────────────────────────────────────────────────────────────
struct ExpSystem {
    float baseExp = 6.0f;
    float weight  = 2.4f;

    // 현재 레벨에서 다음 레벨까지 필요한 EXP
    long long Required(int currentLevel) const {
        if (currentLevel < 1) currentLevel = 1;
        return (long long)(baseExp * std::pow((float)currentLevel, weight));
    }
};

inline ExpSystem g_ExpSystem;
