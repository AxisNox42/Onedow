#pragma once
// Run result commit, shared by the two ways a run ends in main.cpp:
// abandoning (AbandonCurrentRun) and dying (DYING -> GAMEOVER). Both had the
// same statements; moved here unchanged so REG-05 can drive them. The callers
// keep the run-once guards (abandon returns when already GAMEOVER; the death
// path commits once, on the DYING -> GAMEOVER transition).

#include "SaveSystem.h"
#include "Achievements.h"

// Records the run (best score, totals, codex kills, coins — RecordRunResult
// saves), unlocks the 10-games achievement and saves for it. Creative and
// debug plays (callers pass !PlayRecordsProgress()) record nothing and earn
// no coins. Returns the new-record flag
// (g_LastRunRecord).
inline bool CommitRunResult(bool creative, int difficultyIdx, long long score,
                            long long kills, int weaponIdx,
                            int wave = 0, bool cleared = false, float clearSeconds = 0.0f) {
    if (!creative) {
        const bool record = RecordRunResult(difficultyIdx, score, kills, weaponIdx,
                                              wave, cleared, clearSeconds);
        if (g_TotalGames >= 10) TryUnlockAch(ACH_GAMES_10);
        if (g_AchSaveNeeded) {
            SaveGame();
            g_AchSaveNeeded = false;
        }
        return record;
    }
    g_LastRunCoins = 0;
    for (int i = 0; i < CM_COUNT; ++i) g_RunMobKillCounts[i] = 0;
    return false;
}
