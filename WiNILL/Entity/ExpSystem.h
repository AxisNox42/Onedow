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
    // 평탄 바닥값(flat floor) — 모든 레벨에 동일 가산. 값이 작은 초반에선 비중이 커
    //   첫 레벨업들을 늦추고, 값이 큰 후반에선 무시할 수준(곡선 유지).
    //   "극초반 1~3킬 레벨업"이 너무 잦아 진행이 끊긴다는 피드백 → 초반만 완만하게.
    float flatFloor = 30.0f;

    // 현재 레벨에서 다음 레벨까지 필요한 EXP
    //   L1=36  L2=62  L3=113  L5=285  L10≈1.5k  L30≈22k (후반은 사실상 동일)
    long long Required(int currentLevel) const {
        if (currentLevel < 1) currentLevel = 1;
        return (long long)(baseExp * std::pow((float)currentLevel, weight) + flatFloor);
    }
};

inline ExpSystem g_ExpSystem;
