#pragma once
// ─────────────────────────────────────────────────────────────
// 메타 진행 — 판마다 코인 적립 → 영구 업그레이드 구매 (저장됨)
//   모든 런 시작 시 ApplyMeta() 로 g_Stats 에 누적 적용.
// ─────────────────────────────────────────────────────────────
#include "PlayerStats.h"
#include "Settings.h"   // CurLangIdx 는 Augment.h 에 있지만 언어 인덱스만 필요

enum MetaId { META_HP, META_DMG, META_MOVE, META_VISION, META_STARTAUG, META_COUNT };

struct MetaDef {
    const wchar_t* name[3];   // KR / EN / JP
    int            maxLv;
    long long      baseCost;  // 1레벨 비용 (레벨마다 ×1.6)
};
inline const MetaDef META_DEFS[META_COUNT] = {
    { { L"최대 체력 +20",  L"Max HP +20",       L"最大HP +20" },   5, 100 },
    { { L"공격력 +6%",     L"Attack +6%",       L"攻撃力 +6%" },   5, 150 },
    { { L"이동속도 +5%",   L"Move Speed +5%",   L"移動速度 +5%" }, 3, 120 },
    { { L"시작 시야 +60",  L"Start Vision +60", L"初期視界 +60" }, 3, 120 },
    { { L"시작 증강 +1",   L"Start Augment +1", L"初期強化 +1" },  2, 400 },
};

inline long long g_Coins = 0;
inline int       g_MetaLv[META_COUNT] = { 0 };

// 다음 레벨 구매 비용 (이미 만렙이면 -1)
inline long long MetaNextCost(int id) {
    if (id < 0 || id >= META_COUNT) return -1;
    if (g_MetaLv[id] >= META_DEFS[id].maxLv) return -1;
    double c = (double)META_DEFS[id].baseCost;
    for (int i = 0; i < g_MetaLv[id]; i++) c *= 1.6;
    return (long long)c;
}

inline const wchar_t* MetaName(int id) {
    int li = (int)g_Language; if (li < 0 || li >= LANG_COUNT) li = 0;
    return META_DEFS[id].name[li];
}

// 런 시작 시 g_Stats 에 누적 적용 → 시작 무료 증강 픽 횟수 반환
inline int ApplyMeta(PlayerStats& s) {
    s.maxHP            += 20.0f * g_MetaLv[META_HP];
    s.damageMultiplier *= (1.0f + 0.06f * (float)g_MetaLv[META_DMG]);
    s.moveSpeedMult    *= (1.0f + 0.05f * (float)g_MetaLv[META_MOVE]);
    s.windowSize       += 60.0f * g_MetaLv[META_VISION];
    return g_MetaLv[META_STARTAUG];
}
