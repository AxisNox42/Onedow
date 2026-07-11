#pragma once
// ─────────────────────────────────────────────────────────────
// Identity Slot — EPIC/LEGENDARY/MYTHIC/COMBO 증강 보유 상한 (덱빌딩)
// ─────────────────────────────────────────────────────────────
#include "Augment.h"
#include "Meta.h"
#include "Ascension.h"
#include <vector>

extern std::vector<int> g_OwnedAugs;

inline bool AugUsesIdentitySlot(AugRarity r) {
    return r == AugRarity::EPIC || r == AugRarity::LEGENDARY ||
           r == AugRarity::MYTHIC || r == AugRarity::COMBO;
}

inline bool AugIdxUsesIdentitySlot(int idx) {
    if (idx < 0 || idx >= AUG_TOTAL) return false;
    return AugUsesIdentitySlot(ALL_AUGS[idx].rarity);
}

inline int IdentitySlotMax() {
    int m = 8 + g_MetaLv[META_ID_SLOT] - Asc::CurrentMods().identitySlotPenalty;
    return m < 4 ? 4 : m;
}

inline int CountIdentitySlotsUsed() {
    int n = 0;
    for (int idx : g_OwnedAugs)
        if (AugIdxUsesIdentitySlot(idx)) n++;
    return n;
}

inline bool IdentitySlotsFull() {
    return CountIdentitySlotsUsed() >= IdentitySlotMax();
}

inline bool NeedsReplaceForAug(int idx) {
    return AugIdxUsesIdentitySlot(idx) && IdentitySlotsFull();
}
