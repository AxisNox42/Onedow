#pragma once
// ─────────────────────────────────────────────────────────────
// Ascension(승천) — 탑 클리어 후 해금되는 격층 난이도 모디파이어
//   g_TowerClears: 누적 클리어 횟수 (SaveSystem 영구 저장)
//   g_SelectedAscension: 이번 런 선택 승천 단계 (런타임만)
// ─────────────────────────────────────────────────────────────
#include "Settings.h"

namespace Asc {

inline int g_TowerClears       = 0;
inline int g_SelectedAscension = 0;

inline int MaxSelectableAscension() {
    return g_TowerClears;
}

struct Mods {
    float enemyHpMult;
    float spawnMult;
    int   identitySlotPenalty;
};

inline Mods GetMods(int level) {
    Mods m;
    m.enemyHpMult           = 1.0f + 0.12f * (float)level;
    m.spawnMult             = 1.0f + 0.08f * (float)level;
    m.identitySlotPenalty   = 0;
    if (level >= 3) m.identitySlotPenalty = 1;
    if (level >= 6) m.identitySlotPenalty = 2;
    return m;
}

inline Mods CurrentMods() {
    return GetMods(g_SelectedAscension);
}

inline float ClearCoinBonusMul(int ascension) {
    return 1.0f + 0.15f * (float)ascension;
}

inline const wchar_t* Label(int asc) {
    static const wchar_t* KR[] = {
        L"승천 0", L"승천 I", L"승천 II", L"승천 III",
        L"승천 IV", L"승천 V", L"승천 VI", L"승천 VII",
        L"승천 VIII", L"승천 IX", L"승천 X",
    };
    static const wchar_t* EN[] = {
        L"Ascension 0", L"Ascension I", L"Ascension II", L"Ascension III",
        L"Ascension IV", L"Ascension V", L"Ascension VI", L"Ascension VII",
        L"Ascension VIII", L"Ascension IX", L"Ascension X",
    };
    static const wchar_t* JP[] = {
        L"昇天 0", L"昇天 I", L"昇天 II", L"昇天 III",
        L"昇天 IV", L"昇天 V", L"昇天 VI", L"昇天 VII",
        L"昇天 VIII", L"昇天 IX", L"昇天 X",
    };
    if (asc < 0) asc = 0;
    if (asc > 10) asc = 10;
    int li = LangIndex();
    if (li == 1) return EN[asc];
    if (li == 2) return JP[asc];
    return KR[asc];
}

} // namespace Asc
