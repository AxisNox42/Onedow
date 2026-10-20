// REG-02 (D02 baseline): the same build inputs through every stat path —
// new run (ResetForNewGame + FinalizeLoadout + cards as picked), rebuild on
// augment replacement, restart, Chaos, and the augment-card preview.
// REG-03 (D03 baseline): which run fields survive a rebuild, a replacement,
// a restart and Chaos, including Warlord removal and reacquisition.
// Run with scripts/test_reg02_03_stat_builds.ps1.
//
// The paths are the RunStats.h functions main.cpp / SceneRunConfig.cpp /
// SceneAugment.cpp call. Expected values are worked out from the stat rules
// here (constants below), not through the build functions. Decided rules:
// DEC-03 (trial max HP before the cards, baseFireInterval after the weapon, on
// every path) and DEC-01 (Chaos resets the Warlord stacks), 2026-10-13.
#include "RunStats.h"
#include <cassert>
#include <cmath>
#include <cstdio>

static bool Near(float a, float b) { return std::fabs(a - b) < 1e-4f * std::max(1.0f, std::fabs(b)); }

static int IdxOf(AugType t) {
    for (int i = 0; i < AUG_TOTAL; ++i) if (ALL_AUGS[i].type == t) return i;
    assert(false && "augment type missing from the catalog");
    return -1;
}

// ── Inputs ────────────────────────────────────────────────────────────
static int RIFLE() { return (int)StartWeapon::RIFLE; }
static int SMG()   { return (int)StartWeapon::SMG; }

static void SetMeta(bool on) {
    for (int& lv : g_MetaLv) lv = 0;
    if (on) { g_MetaLv[META_HP] = 1; g_MetaLv[META_DMG] = 1; }
}
static void SetTrial10(bool on) {
    for (int i = 0; i < TRIAL_SLOT_COUNT; ++i) g_TrialSelected[i] = false;
    g_TrialPool[0] = 10;
    g_TrialSelected[0] = on;
    assert(TrialActive(10) == on);
}

// ── Expected values from the rules ───────────────────────────────────
// Defaults: maxHP 100, fireInterval 0.20, damageMultiplier 1, bulletSpeed
// 1200, windowSize 800. Meta: HP +20, damage x1.06. SMG: fire x0.5,
// damage x0.5. Cards: RATE fire /1.04, HP +15, SPD +30, MOVE speed x1.05;
// the first common card boosts damage x1.08 once. Window size is untouched.
// Trial 10: max HP x0.82.
struct Expect { float maxHP, fire, dmg, speed, window; };
static Expect ExpectedNoTrial(bool smg, bool meta, float scale, bool cards) {
    Expect e{ 100.0f, 0.20f, 1.0f, 1200.0f, 800.0f };
    if (meta) { e.maxHP += 20.0f; e.dmg *= 1.06f; }
    e.window *= scale;
    if (smg) { e.fire *= 0.5f; e.dmg *= 0.5f; }
    if (cards) {
        e.fire /= 1.04f; e.dmg *= 1.08f; e.maxHP += 15.0f; e.speed += 30.0f;
    }
    return e;
}
static float PreCardHp(bool meta) { return meta ? 120.0f : 100.0f; }

static const char* F(bool v) { return v ? "on " : "off"; }

static void Print(const char* path, const PlayerStats& s) {
    std::printf("  %-12s maxHP %7.3f base %.4f fire %.5f dmg %.5f spd %6.1f win %6.1f\n",
                path, s.maxHP, s.baseFireInterval, s.fireInterval, s.damageMultiplier,
                s.bulletSpeed, s.windowSize);
    std::fflush(stdout);   // keep the table if an assert aborts
}

static void CheckShared(const PlayerStats& s, const Expect& e) {
    assert(Near(s.bulletSpread, 0.0f));
    assert(Near(s.fireInterval, e.fire));
    assert(Near(s.damageMultiplier, e.dmg));
    assert(Near(s.bulletSpeed, e.speed));
    assert(Near(s.windowSize, e.window));
}

// REG-02: every path, same inputs.
static void CheckPaths() {
    std::printf("-- REG-02 stat paths (cards: RATE, HP, SPD, MOVE)\n");
    std::vector<int> owned = { IdxOf(AugType::RATE_UP), IdxOf(AugType::HP_UP),
                               IdxOf(AugType::SPD_UP), IdxOf(AugType::MOVE_UP) };
    for (int weapon : { RIFLE(), SMG() }) {
        for (bool meta : { false, true }) {
            for (bool trial : { false, true }) {
                for (float scale : { 1.0f, 0.75f }) {
                    SetMeta(meta); SetTrial10(trial);
                    const bool smg = weapon == SMG();
                    std::printf("%s meta %s trial10 %s scale %.2f\n", smg ? "SMG  " : "Rifle",
                                F(meta), F(trial), scale);

                    // New run: reset, loadout, then the cards as they are picked.
                    PlayerStats run = ResetRunStats(scale);
                    ApplyLoadoutStats(run, weapon);
                    for (int idx : owned) ApplyAugStats(run, idx);
                    const PlayerStats rebuild = RebuildStats(weapon, owned, scale, &run);
                    PlayerStats restart = RebuildStats(weapon, owned, scale, nullptr);
                    const PlayerStats preview = PreviewOwnedStats(weapon, owned, scale);
                    Print("new run", run); Print("rebuild", rebuild);
                    Print("restart", restart); Print("preview", preview);

                    const Expect e = ExpectedNoTrial(smg, meta, scale, true);
                    const PlayerStats* paths[] = { &run, &rebuild, &restart, &preview };
                    for (const PlayerStats* s : paths) CheckShared(*s, e);

                    // Max HP: trial cuts the pre-card HP, then HP cards add
                    // their full amount on every path.
                    const float cut = trial ? 0.82f : 1.0f;
                    const float cutThenCards = PreCardHp(meta) * cut + 15.0f;
                    assert(Near(run.maxHP, cutThenCards));
                    assert(Near(preview.maxHP, cutThenCards));
                    assert(Near(restart.maxHP, cutThenCards));
                    assert(Near(rebuild.maxHP, cutThenCards));

                    // The base interval is captured after the weapon and
                    // before cards, including in the preview.
                    const float weaponBase = smg ? 0.10f : 0.20f;
                    for (const PlayerStats* s : paths)
                        assert(Near(s->baseFireInterval, weaponBase));
                }
            }
        }
    }
    SetMeta(false); SetTrial10(false);
}

// REG-02: a candidate card's preview matches the build after taking it, and
// after taking it as an identity-slot replacement (rebuild without the old
// card, then the new one — CompleteAugReplaceFlow).
static void CheckCandidate() {
    std::printf("-- REG-02 candidate preview vs taken\n");
    SetMeta(true); SetTrial10(false);
    const int rate = IdxOf(AugType::RATE_UP), hp = IdxOf(AugType::HP_UP),
              spd = IdxOf(AugType::SPD_UP), dmg = IdxOf(AugType::DMG_UP);
    const std::vector<int> owned = { rate, hp, spd };
    const PlayerStats preview = PreviewCandidateStats(PreviewOwnedStats(SMG(), owned, 1.0f),
                                                      ALL_AUGS[dmg]);
    PlayerStats taken = ResetRunStats(1.0f);
    ApplyLoadoutStats(taken, SMG());
    for (int idx : owned) ApplyAugStats(taken, idx);
    ApplyAugStats(taken, dmg);
    // Replace SPD with DMG.
    PlayerStats replaced = RebuildStats(SMG(), { rate, hp }, 1.0f, &taken);
    ApplyAugStats(replaced, dmg);
    Print("preview", preview); Print("taken", taken); Print("replaced", replaced);
    assert(Near(preview.GetBaseDamage(), taken.GetBaseDamage()));
    assert(Near(preview.damageMultiplier, taken.damageMultiplier));
    assert(Near(preview.fireInterval, taken.fireInterval) && Near(preview.maxHP, taken.maxHP));
    assert(Near(taken.GetBaseDamage(), 50.0f + 9.0f));                 // DMG +9 flat
    assert(Near(replaced.bulletSpeed, 1200.0f) && Near(taken.bulletSpeed, 1230.0f));
    assert(Near(replaced.GetBaseDamage(), taken.GetBaseDamage()));

    // Replacement and preview must also agree with the HP trial active.
    SetTrial10(true);
    const PlayerStats trialPreview = PreviewCandidateStats(
        PreviewOwnedStats(SMG(), owned, 1.0f), ALL_AUGS[dmg]);
    PlayerStats trialReplaced = RebuildStats(SMG(), { rate, hp }, 1.0f, &taken);
    ApplyAugStats(trialReplaced, dmg);
    assert(Near(trialPreview.maxHP, 113.4f));
    assert(Near(trialReplaced.maxHP, trialPreview.maxHP));
    SetTrial10(false);
    SetMeta(false);
}

static void CheckChaosTrial() {
    SetMeta(true); SetTrial10(true);
    const PlayerStats before = PreviewOwnedStats(SMG(), { IdxOf(AugType::HP_UP) }, 1.0f);
    const PlayerStats chaos = ChaosStats(before, SMG(), 1.0f);
    assert(Near(before.maxHP, 113.4f));
    assert(Near(chaos.maxHP, 98.4f)); // card removed, trial still active
    assert(Near(chaos.baseFireInterval, 0.10f));
    SetMeta(false); SetTrial10(false);
}

// REG-03: run fields across rebuild / replacement / restart / Chaos.
static PlayerStats RunState(int weapon, const std::vector<int>& owned, int warlordStacks) {
    PlayerStats s = ResetRunStats(1.0f);
    ApplyLoadoutStats(s, weapon);
    for (int idx : owned) ApplyAugStats(s, idx);
    s.killCount = 2345;
    s.vampireKillStreak = 6;
    if (s.warlord) s.RestoreWarlordStacks(warlordStacks);
    s.mk2Used = true;
    s.lightStepDisableTimer = 4.5f;
    return s;
}

static void PrintKeep(const char* path, const PlayerStats& s) {
    std::printf("  %-22s kills %lld vamp %d warlord %d(%s) mk2 %d lightStep %.1f dmg %.4f\n",
                path, s.killCount, s.vampireKillStreak, s.warlordStacks,
                s.warlord ? "owned" : "-", s.mk2Used ? 1 : 0, s.lightStepDisableTimer,
                s.damageMultiplier);
    std::fflush(stdout);
}

static void CheckRunFields() {
    std::printf("-- REG-03 run fields (kills 2345, vampire 6, warlord 3, mk2, lightStep 4.5)\n");
    SetMeta(false); SetTrial10(false);
    const int warlord = IdxOf(AugType::CB_WARLORD), rate = IdxOf(AugType::RATE_UP);
    const std::vector<int> owned = { rate, warlord };
    const PlayerStats before = RunState(RIFLE(), owned, 3);
    PrintKeep("before", before);
    assert(before.warlord && before.warlordStacks == 3);

    // Rebuild keeping the owned cards: all five fields survive, the Warlord
    // stacks with their effect (x1.05 damage each).
    const PlayerStats rebuild = RebuildStats(RIFLE(), owned, 1.0f, &before);
    PrintKeep("rebuild", rebuild);
    assert(rebuild.killCount == 2345 && rebuild.vampireKillStreak == 6 && rebuild.mk2Used &&
           Near(rebuild.lightStepDisableTimer, 4.5f) && rebuild.warlordStacks == 3);
    assert(Near(rebuild.damageMultiplier, before.damageMultiplier));
    assert(Near(rebuild.damageMultiplier, 1.08f * 1.15f * 1.05f * 1.05f * 1.05f));

    // Replacement that removes the Warlord card: its stacks go with it.
    PlayerStats removed = RebuildStats(RIFLE(), { rate }, 1.0f, &before);
    ApplyAugStats(removed, IdxOf(AugType::SPD_UP));
    PrintKeep("replace warlord -> spd", removed);
    assert(!removed.warlord && removed.warlordStacks == 0 && removed.killCount == 2345);

    // Reacquiring Warlord later starts from zero stacks (current behavior).
    PlayerStats again = RebuildStats(RIFLE(), { rate, IdxOf(AugType::SPD_UP) }, 1.0f, &removed);
    ApplyAugStats(again, warlord);
    PrintKeep("reacquire warlord", again);
    assert(again.warlord && again.warlordStacks == 0);

    // Restart: a fresh run on the same build; no run field carries over.
    const PlayerStats restart = RebuildStats(RIFLE(), owned, 1.0f, nullptr);
    PrintKeep("restart", restart);
    assert(restart.killCount == 0 && restart.vampireKillStreak == 0 && !restart.mk2Used &&
           restart.lightStepDisableTimer == 0.0f && restart.warlordStacks == 0);

    // Chaos: four fields survive; the Warlord stacks reset (DEC-01).
    const PlayerStats chaos = ChaosStats(before, RIFLE(), 1.0f);
    PrintKeep("chaos", chaos);
    assert(chaos.killCount == 2345 && chaos.vampireKillStreak == 6 && chaos.mk2Used &&
           Near(chaos.lightStepDisableTimer, 4.5f));
    assert(!chaos.warlord && chaos.warlordStacks == 0);              // DEC-01
    assert(Near(chaos.damageMultiplier, 1.0f) && Near(chaos.fireInterval, 0.20f));
}

static void CheckComboOnceLedger() {
    const ComboDef& combo = COMBO_DEFS[1]; // Warlord
    bool ledger[AUG_TOTAL] = {};
    for (int i = 0; i < combo.reqCount; ++i)
        g_TypeOwned[(int)combo.reqs[i]] = true;
    assert(ComboRewardEligible(combo, ledger));
    g_TypeOwned[(int)combo.result] = true;
    assert(!ComboRewardEligible(combo, ledger));
    g_TypeOwned[(int)combo.result] = false;
    ledger[IdxOf(combo.result)] = true;
    assert(!ComboRewardEligible(combo, ledger)); // replacement cannot reopen it
    for (int i = 0; i < combo.reqCount; ++i)
        g_TypeOwned[(int)combo.reqs[i]] = false;
}

static void CheckExperienceAugments() {
    SetMeta(false);
    SetTrial10(false);
    const std::vector<int> owned = {
        IdxOf(AugType::D_RMOB_MAX), IdxOf(AugType::D_RMOB_HP),
        IdxOf(AugType::D_RMOB_DELAY), IdxOf(AugType::D_MOB_SPAWN),
        IdxOf(AugType::D_APPROACH), IdxOf(AugType::D_APPROACH),
        IdxOf(AugType::D_APPROACH), IdxOf(AugType::D_MOB_SPEED),
        IdxOf(AugType::D_MOB_HP), IdxOf(AugType::D_GLASS_HEART),
        IdxOf(AugType::D_BULLET_STUCK), IdxOf(AugType::D_SLOW_MOVE),
        IdxOf(AugType::D_BLEED), IdxOf(AugType::D_WEAKEN)
    };
    PlayerStats run = ResetRunStats(1.0f);
    ApplyLoadoutStats(run, RIFLE());
    for (int idx : owned) ApplyAugStats(run, idx);
    const PlayerStats rebuilt = RebuildStats(RIFLE(), owned, 1.0f, &run);
    const PlayerStats restarted = RebuildStats(RIFLE(), owned, 1.0f, nullptr);
    const PlayerStats preview = PreviewOwnedStats(RIFLE(), owned, 1.0f);
    const float multiplier = 1.03f * 1.05f * 1.03f * 1.12f * 1.10f;
    const PlayerStats* paths[] = { &run, &rebuilt, &restarted, &preview };
    for (const PlayerStats* stats : paths) {
        assert(stats->rangedXpBonus == 23);
        assert(stats->mobXpBonus == 1 && stats->rotorXpBonus == 2);
        assert(stats->approachStacks == 3 && Near(stats->xpPerSec, 2.0f));
        assert(Near(stats->xpMult, multiplier));
    }
    PlayerStats capped = run;
    capped.Apply(AugType::D_APPROACH);
    assert(capped.approachStacks == 3 && Near(capped.xpPerSec, 2.0f));
    const PlayerStats chaos = ChaosStats(run, RIFLE(), 1.0f);
    assert(chaos.rangedXpBonus == 0 && chaos.mobXpBonus == 0 &&
           chaos.rotorXpBonus == 0 && Near(chaos.xpPerSec, 0.0f) &&
           Near(chaos.xpMult, 1.0f));
}

static void CheckMinigun() {
    SetMeta(false); SetTrial10(false);
    const int minigun = IdxOf(AugType::MINIGUN);
    const int marksman = IdxOf(AugType::RIFLE_STABILITY);
    const AugDef& card = ALL_AUGS[minigun];
    assert(card.rarity == AugRarity::EPIC && !AugRemoved(card.type));
    assert(AugOnceOnly(card.type, card.rarity) && AugIsWeaponType(card.type));
    for (int weapon : { RIFLE(), SMG() }) {
        PlayerStats base = ResetRunStats(1.0f);
        ApplyLoadoutStats(base, weapon);
        assert(Near(base.bulletSpread, 0.0f));
        assert(Near(PreviewBaseStats(weapon, 1.0f).bulletSpread, 0.0f));
    }
    for (const std::vector<int>& owned : {
            std::vector<int>{minigun}, std::vector<int>{minigun, marksman},
            std::vector<int>{marksman, minigun} }) {
        PlayerStats run = ResetRunStats(1.0f);
        ApplyLoadoutStats(run, RIFLE());
        for (int idx : owned) ApplyAugStats(run, idx);
        const PlayerStats rebuilt = RebuildStats(RIFLE(), owned, 1.0f, &run);
        const PlayerStats restarted = RebuildStats(RIFLE(), owned, 1.0f, nullptr);
        const PlayerStats preview = PreviewOwnedStats(RIFLE(), owned, 1.0f);
        const PlayerStats* paths[] = { &run, &rebuilt, &restarted, &preview };
        for (const PlayerStats* stats : paths) {
            assert(Near(stats->bulletSpeed, 1800.0f));
            assert(Near(stats->fireInterval, 0.20f / 1.75f));
            assert(Near(stats->bulletSpread, 0.15f));
            assert(Near(stats->baseFireInterval, 0.20f));
        }
        const PlayerStats removed = RebuildStats(RIFLE(), {}, 1.0f, &run);
        const PlayerStats chaos = ChaosStats(run, RIFLE(), 1.0f);
        for (const PlayerStats* stats : { &removed, &chaos }) {
            assert(Near(stats->bulletSpread, 0.0f));
            assert(Near(stats->bulletSpeed, 1200.0f));
            assert(Near(stats->fireInterval, 0.20f));
        }
    }
    const PlayerStats candidate = PreviewCandidateStats(PreviewBaseStats(RIFLE(), 1.0f), card);
    assert(Near(candidate.bulletSpeed, 1800.0f));
    assert(Near(candidate.fireInterval, 0.20f / 1.75f));
    assert(Near(candidate.bulletSpread, 0.15f));
}

int main() {
    CheckMinigun();
    CheckPaths();
    CheckCandidate();
    CheckChaosTrial();
    CheckRunFields();
    CheckComboOnceLedger();
    CheckExperienceAugments();
    std::printf("REG-02/03 stat build regression passed\n");
    return 0;
}
