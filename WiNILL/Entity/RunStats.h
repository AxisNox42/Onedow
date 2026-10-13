#pragma once
// Player stat builds of the run paths, as functions of their inputs (weapon,
// owned augments, display scale; meta levels via ApplyMeta and trial state via
// TrialPlayerMaxHpMult). Moved unchanged out of main.cpp (new run reset,
// rebuild on augment replacement, restart, Chaos), SceneRunConfig.cpp
// (FinalizeLoadout) and SceneAugment.cpp (preview) so REG-02/03 can compare
// the paths. Trial max HP is applied before cards in every path, and the base
// fire interval is captured after the weapon but before cards. Runtime-field
// preservation still differs by path (D03).

#include "PlayerStats.h"
#include "Weapons.h"
#include "Augment.h"
#include "Meta.h"
#include "Settings.h"
#include <vector>

// One augment's stat effect: Apply plus the common-card boost (the same two
// statements ApplySingleAugIdx, the rebuild loop and the preview each had).
inline void ApplyAugStats(PlayerStats& s, int idx) {
    s.Apply(ALL_AUGS[idx].type);
    if (ALL_AUGS[idx].rarity == AugRarity::COMMON)
        s.ApplyCommonMultBoost();
}

// Trial max-HP cut, applied after the weapon and before cards on every path.
inline void ApplyTrialMaxHp(PlayerStats& s) {
    float trialHpMul = TrialPlayerMaxHpMult();
    if (trialHpMul < 0.999f)
        s.maxHP *= trialHpMul;
}

// New run, step 1 (ResetForNewGame): no weapon chosen yet.
inline PlayerStats ResetRunStats(float scale) {
    PlayerStats s;
    ApplyMeta(s);
    s.windowSize *= scale;
    return s;
}

// New run, step 2 (FinalizeLoadout): the chosen weapon and the trial cut.
inline void ApplyLoadoutStats(PlayerStats& s, int weapon) {
    ApplyWeapon(s, (StartWeapon)weapon);
    s.baseFireInterval = s.fireInterval;
    ApplyTrialMaxHp(s);
}

// Rebuild from the owned augments (augment replacement; restart with
// keep == nullptr). The trial cut precedes the owned cards.
// keep: the stats before the rebuild, whose run fields carry over.
inline PlayerStats RebuildStats(int weapon, const std::vector<int>& owned, float scale,
                                const PlayerStats* keep) {
    PlayerStats s;
    ApplyMeta(s);
    s.windowSize *= scale;
    if (weapon >= 0 && weapon < (int)StartWeapon::_COUNT)
        ApplyWeapon(s, (StartWeapon)weapon);
    s.baseFireInterval = s.fireInterval;
    ApplyTrialMaxHp(s);
    for (int idx : owned) {
        if (idx < 0 || idx >= AUG_TOTAL) continue;
        ApplyAugStats(s, idx);
    }
    if (keep) {
        s.killCount = keep->killCount;
        s.vampireKillStreak = keep->vampireKillStreak;
        if (s.warlord)
            s.RestoreWarlordStacks(keep->warlordStacks);
        s.mk2Used = keep->mk2Used;
        s.lightStepDisableTimer = keep->lightStepDisableTimer;
    }
    return s;
}

// Chaos: back to the weapon base before its random cards are applied. Keeps
// four run fields; the Warlord stacks reset with the build (DEC-01, decided).
inline PlayerStats ChaosStats(const PlayerStats& before, int weapon, float scale) {
    PlayerStats s;
    ApplyMeta(s);
    s.windowSize *= scale;
    if (weapon >= 0 && weapon < (int)StartWeapon::_COUNT)
        ApplyWeapon(s, (StartWeapon)weapon);
    s.baseFireInterval = s.fireInterval;
    ApplyTrialMaxHp(s);
    s.killCount = before.killCount;
    s.vampireKillStreak = before.vampireKillStreak;
    s.mk2Used = before.mk2Used;
    s.lightStepDisableTimer = before.lightStepDisableTimer;
    return s;
}

// Augment-card preview (SceneAugment): base, trial cut, owned cards, then the
// candidate.
inline PlayerStats PreviewBaseStats(int weapon, float scale) {
    PlayerStats base;
    ApplyMeta(base);
    base.windowSize *= scale;

    if (weapon >= 0 &&
        weapon < (int)StartWeapon::_COUNT) {
        ApplyWeapon(base, (StartWeapon)weapon);
    }
    base.baseFireInterval = base.fireInterval;
    return base;
}

inline PlayerStats PreviewTrialStats(int weapon, float scale) {
    PlayerStats trial = PreviewBaseStats(weapon, scale);
    ApplyTrialMaxHp(trial);
    return trial;
}

inline PlayerStats PreviewOwnedStats(int weapon, const std::vector<int>& owned, float scale) {
    PlayerStats stats = PreviewTrialStats(weapon, scale);
    for (int idx : owned) {
        if (idx < 0 || idx >= AUG_TOTAL) continue;
        ApplyAugStats(stats, idx);
    }
    return stats;
}

inline PlayerStats PreviewCandidateStats(const PlayerStats& owned, const AugDef& candidate) {
    PlayerStats preview = owned;
    preview.Apply(candidate.type);
    if (candidate.rarity == AugRarity::COMMON)
        preview.ApplyCommonMultBoost();
    return preview;
}
