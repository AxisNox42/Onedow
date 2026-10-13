// REG-05 (D05 baseline): the run result commit shared by abandon and game
// over (RunResult.h), with every record it writes. Run with
// scripts/test_reg05_run_result.ps1, which also pins the callers' run-once
// guards in main.cpp.
//
// Saving is stubbed: g_SmokeCapture makes SaveGame return before touching
// disk, and the run script starts the test in an empty temporary folder and
// checks that no save file appeared there.
#include "RunResult.h"
#include <cassert>
#include <cstdio>
#include <filesystem>

static void ResetProfile() {
    for (auto& v : g_BestScore) v = 0;
    g_TotalKills = 0; g_TotalGames = 0; g_Coins = 0; g_LastRunCoins = 0; g_RunStardust = 0;
    for (int i = 0; i < 2; ++i) {
        g_WeaponBestScore[i] = 0; g_WeaponBestKills[i] = 0;
        g_WeaponTotalKills[i] = 0; g_WeaponRunCount[i] = 0;
    }
    for (int i = 0; i < CM_COUNT; ++i) { g_MobKillCounts[i] = 0; g_RunMobKillCounts[i] = 0; }
    for (int i = 0; i < ACH_COUNT; ++i) g_AchUnlocked[i] = false;
    g_AchSaveNeeded = false;
}

int main() {
    g_SmokeCapture = true;   // SaveGame: no disk writes
    ResetProfile();

    // A normal run (difficulty 1 = x1.2 coins), static-field weapon slot 1.
    g_RunMobKillCounts[CM_ROTOR] = 5;
    bool record = CommitRunResult(false, 1, 12345, 77, 1);
    std::printf("run 1: record %d games %lld best %lld kills %lld coins %lld codex rotor %lld\n",
                record, g_TotalGames, g_BestScore[1], g_TotalKills, g_Coins,
                g_MobKillCounts[CM_ROTOR]);
    std::fflush(stdout);
    assert(record && g_TotalGames == 1 && g_BestScore[1] == 12345 && g_TotalKills == 77);
    assert(g_Coins == 60 && g_LastRunCoins == 60 && g_RunStardust == 60);   // (12 + 38) x 1.2
    assert(g_WeaponBestScore[1] == 12345 && g_WeaponBestKills[1] == 77 &&
           g_WeaponTotalKills[1] == 77 && g_WeaponRunCount[1] == 1 && g_WeaponRunCount[0] == 0);
    assert(g_MobKillCounts[CM_ROTOR] == 5 && g_RunMobKillCounts[CM_ROTOR] == 0);

    // A lower second run: totals grow, no new record.
    record = CommitRunResult(false, 1, 5000, 10, 1);
    assert(!record && g_TotalGames == 2 && g_BestScore[1] == 12345 && g_TotalKills == 87);
    assert(g_LastRunCoins == 12 && g_Coins == 72);                            // (5 + 5) x 1.2

    // Out-of-range difficulty counts as normal; hard pays x1.5.
    record = CommitRunResult(false, 7, 20000, 0, 0);
    assert(record && g_BestScore[1] == 20000 && g_LastRunCoins == 24);        // 20 x 1.2
    CommitRunResult(false, 2, 2000, 4, 0);
    assert(g_BestScore[2] == 2000 && g_LastRunCoins == 6);                    // (2 + 2) x 1.5

    // The 10th game unlocks its achievement once and clears the save flag.
    g_TotalGames = 9;
    const long long coinsBefore = g_Coins;
    CommitRunResult(false, 0, 0, 0, 0);
    assert(g_TotalGames == 10 && g_AchUnlocked[ACH_GAMES_10] && !g_AchSaveNeeded);
    assert(g_Coins == coinsBefore + ACH_DEFS[ACH_GAMES_10].coinReward);
    CommitRunResult(false, 0, 0, 0, 0);
    assert(g_Coins == coinsBefore + ACH_DEFS[ACH_GAMES_10].coinReward);      // no second unlock
    std::printf("10th game: achievement once (+%d coins)\n", ACH_DEFS[ACH_GAMES_10].coinReward);

    // Creative: nothing recorded, no coins, the run's codex kills stay put.
    const long long games = g_TotalGames, kills = g_TotalKills, coins = g_Coins;
    g_LastRunCoins = 99;
    g_RunMobKillCounts[CM_SWARM] = 3;
    record = CommitRunResult(true, 1, 999999, 500, 1);
    assert(!record && g_TotalGames == games && g_TotalKills == kills && g_Coins == coins);
    assert(g_LastRunCoins == 0 && g_BestScore[1] == 20000 && g_RunMobKillCounts[CM_SWARM] == 3);
    std::printf("creative: nothing recorded\n");

    assert(!std::filesystem::exists(SaveFilePath()));
    std::printf("REG-05 run result regression passed\n");
    return 0;
}
