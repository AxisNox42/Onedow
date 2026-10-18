// REG-06 (D06 baseline): the two skill resets and the re-equip, with every
// slot and runtime field filled first. Run with scripts/test_reg06_skill_reset.ps1.
//   ClearEquippedSkills (SceneSkills.cpp): the three slots and the
//     replacement index only.
//   ResetPlayerSkills (PlayerRuntime.cpp): the slots plus the skill/dash/
//     shield runtime it lists; post-pick grace, dash endpoints and input
//     edges are left alone (ResetPlayerInputEdges owns the edges).
//   ReequipSkillsFromOwned: clears, then equips in owned order.
#include "PlayerRuntime.h"
#include "SceneSkills.h"
#include "PlayerStats.h"
#include <cassert>
#include <cstdio>

static int IdxOf(AugType t) {
    for (int i = 0; i < AUG_TOTAL; ++i) if (ALL_AUGS[i].type == t) return i;
    assert(false && "augment type missing from the catalog");
    return -1;
}

static void FillEverything() {
    g_Skills[0] = { SkillType::CLOSE_WINDOW, 3.0f };
    g_Skills[1] = { SkillType::HYPER_FOCUS, 7.0f };
    g_Skills[2] = { SkillType::TIME_STOP, 11.0f };
    g_SkillReplaceIdx = 2;
    PlayerRuntimeState& r = g_PlayerRuntime;
    r.dashCooldown = 1.5f; r.dashInvulnerability = 0.1f; r.dashActive = true;
    r.dashProgress = 0.4f; r.dashBoostShotsLeft = 3;
    r.knockbackVX = 50.0f; r.knockbackVY = -20.0f;
    r.timeStopTimer = 0.8f; r.hyperFocusTimer = 2.0f;
    r.dashCooldownMax = 2.9f;
    r.postPickGrace = 0.3f;
    r.dashFromX = 1.0f; r.dashFromY = 2.0f; r.dashToX = 3.0f; r.dashToY = 4.0f;
    r.dashInputHeld = true; r.skillInputHeld[0] = r.skillInputHeld[2] = true;
    g_PlayerShield = 25.0f; g_PlayerShieldTimer = 4.0f;
}

static bool SlotsCleared() {
    for (const SkillSlot& s : g_Skills)
        if (s.type != SkillType::NONE || s.cd != 0.0f) return false;
    return g_SkillReplaceIdx == 0;
}

static bool RuntimeUntouched() {
    const PlayerRuntimeState& r = g_PlayerRuntime;
    return r.dashCooldown == 1.5f && r.dashInvulnerability == 0.1f && r.dashActive &&
           r.dashProgress == 0.4f && r.dashBoostShotsLeft == 3 && r.knockbackVX == 50.0f &&
           r.knockbackVY == -20.0f && r.timeStopTimer == 0.8f && r.hyperFocusTimer == 2.0f &&
           g_PlayerShield == 25.0f && g_PlayerShieldTimer == 4.0f;
}

static bool KeptByBothResets() {
    const PlayerRuntimeState& r = g_PlayerRuntime;
    return r.postPickGrace == 0.3f && r.dashFromX == 1.0f && r.dashFromY == 2.0f &&
           r.dashToX == 3.0f && r.dashToY == 4.0f && r.dashInputHeld &&
           r.skillInputHeld[0] && !r.skillInputHeld[1] && r.skillInputHeld[2];
}

int main() {
    // Slot clear: slots and replacement index only.
    FillEverything();
    ClearEquippedSkills();
    assert(SlotsCleared());
    assert(RuntimeUntouched());
    assert(KeptByBothResets());
    std::printf("ClearEquippedSkills: slots cleared, runtime untouched\n");

    // Full skill reset: slots plus the listed runtime.
    FillEverything();
    ResetPlayerSkills();
    assert(SlotsCleared());
    const PlayerRuntimeState& r = g_PlayerRuntime;
    assert(r.dashCooldown == 0.0f && r.dashInvulnerability == 0.0f && !r.dashActive &&
           r.dashProgress == 0.0f && r.dashBoostShotsLeft == 0 && r.knockbackVX == 0.0f &&
           r.knockbackVY == 0.0f && r.timeStopTimer == 0.0f && r.hyperFocusTimer == 0.0f &&
           r.dashCooldownMax == 0.0f);
    assert(g_PlayerShield == 0.0f && g_PlayerShieldTimer == 0.0f);
    assert(KeptByBothResets());
    std::printf("ResetPlayerSkills: slots + dash/shield/time-stop/hyper-focus cleared; "
                "grace, dash endpoints, input edges kept\n");

    // Dash cooldown max: the value each dash started with (HUD wedge base).
    ResetPlayerSkills();
    g_PlayerRuntime.hyperFocusTimer = 1.0f;
    BeginPlayerDash(0.0f, 0.0f, 1.0f, 0.0f, -10000.0f, 10000.0f, -10000.0f, 10000.0f);
    assert(g_PlayerRuntime.dashCooldownMax == HYPER_FOCUS_DASH_CD);
    g_PlayerRuntime.hyperFocusTimer = 0.0f;
    BeginPlayerDash(0.0f, 0.0f, 1.0f, 0.0f, -10000.0f, 10000.0f, -10000.0f, 10000.0f);
    assert(g_PlayerRuntime.dashCooldownMax == DASH_CD);
    assert(g_PlayerRuntime.dashCooldown == g_PlayerRuntime.dashCooldownMax);
    std::printf("BeginPlayerDash: dashCooldownMax records the dash's cooldown\n");

    // Re-equip: clears first, equips in owned order, skips non-skill cards
    // and duplicates, leaves the runtime alone.
    FillEverything();
    const int owned[] = { IdxOf(AugType::RATE_UP), IdxOf(AugType::SKILL_TIMESTOP),
                          IdxOf(AugType::SKILL_CLOSE), IdxOf(AugType::SKILL_TIMESTOP), -1 };
    ReequipSkillsFromOwned(owned, 5);
    assert(g_Skills[0].type == SkillType::TIME_STOP && g_Skills[0].cd == 0.0f);
    assert(g_Skills[1].type == SkillType::CLOSE_WINDOW);
    assert(g_Skills[2].type == SkillType::NONE && g_SkillReplaceIdx == 0);
    assert(RuntimeUntouched());
    ReequipSkillsFromOwned(nullptr, 0);
    assert(SlotsCleared());
    std::printf("ReequipSkillsFromOwned: owned order, duplicates and non-skills skipped\n");

    // Full slots: a new skill replaces round-robin from the replacement index.
    ClearEquippedSkills();
    g_Skills[0] = { SkillType::CLOSE_WINDOW, 1.0f };
    g_Skills[1] = { SkillType::CLOSE_WINDOW, 2.0f };
    g_Skills[2] = { SkillType::HYPER_FOCUS, 3.0f };
    EquipSkill(SkillType::TIME_STOP);
    assert(g_Skills[0].type == SkillType::TIME_STOP && g_Skills[0].cd == 0.0f);
    assert(g_SkillReplaceIdx == 1);
    EquipSkill(SkillType::TIME_STOP);                      // already equipped: no-op
    assert(g_SkillReplaceIdx == 1);
    std::printf("EquipSkill: round-robin replacement when full\n");

    std::printf("REG-06 skill reset regression passed\n");
    return 0;
}
