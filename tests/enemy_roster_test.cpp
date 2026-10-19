// Enemy roster: first regular appearance per difficulty and the spawn pick.
#include <cstdio>

#include "EnemyRoster.h"

static int g_failures = 0;
#define EXPECT(cond, ...)                                   \
    do {                                                    \
        if (!(cond)) {                                      \
            std::printf("FAIL %s:%d  ", __FILE__, __LINE__); \
            std::printf(__VA_ARGS__);                       \
            std::printf("\n");                              \
            ++g_failures;                                   \
        }                                                   \
    } while (0)

static const int kFinalWave[3] = { 10, 15, 20 };
static const char* kDiff[3] = { "EASY", "NORMAL", "HARD" };

// Kinds a regular spawn can turn into on this wave, over every roll.
static bool CanSpawn(RosterKind kind, int wave, int difficulty) {
    for (int roll = 0; roll < 100; ++roll)
        if (RosterPickRegular(wave, difficulty, roll) == kind) return true;
    return false;
}

int main() {
    // Swarm opens every run; Rotor follows from wave 2 (HARD: wave 1 too).
    for (int d = 0; d < 3; ++d) {
        EXPECT(RosterFirstWave(RosterKind::SWARM, d) == 1, "%s swarm first", kDiff[d]);
        EXPECT(CanSpawn(RosterKind::SWARM, 1, d), "%s swarm on wave 1", kDiff[d]);
    }
    for (int roll = 0; roll < 100; ++roll) {
        EXPECT(RosterPickRegular(1, 0, roll) == RosterKind::SWARM, "EASY wave 1 roll %d", roll);
        EXPECT(RosterPickRegular(1, 1, roll) == RosterKind::SWARM, "NORMAL wave 1 roll %d", roll);
    }
    EXPECT(CanSpawn(RosterKind::ROTOR, 1, 2), "HARD rotor on wave 1");
    EXPECT(CanSpawn(RosterKind::ROTOR, 2, 0), "EASY rotor on wave 2");

    // Every regular kind spawns exactly from its first wave, never before,
    // and kinds with no first wave never spawn on that difficulty.
    for (const RosterEntry& entry : kEnemyRoster) {
        if (entry.kind == RosterKind::SCOPE) continue;   // ranged path
        for (int d = 0; d < 3; ++d) {
            const int first = entry.firstWave[d];
            for (int wave = 1; wave <= kFinalWave[d]; ++wave) {
                const bool can = CanSpawn(entry.kind, wave, d);
                if (first == 0 || wave < first)
                    EXPECT(!can, "%s kind %d spawns early on wave %d", kDiff[d], (int)entry.kind, wave);
                else
                    EXPECT(can, "%s kind %d missing on wave %d", kDiff[d], (int)entry.kind, wave);
            }
        }
    }

    // No enemy debuts on an elite wave, and every debut is inside the run.
    for (const RosterEntry& entry : kEnemyRoster)
        for (int d = 0; d < 3; ++d) {
            const int first = entry.firstWave[d];
            EXPECT(first == 0 || first % 5 != 0, "%s kind %d debuts on elite wave %d",
                   kDiff[d], (int)entry.kind, first);
            EXPECT(first <= kFinalWave[d], "%s kind %d after the run", kDiff[d], (int)entry.kind);
        }

    // HARD-only enemies wait until wave 10+, NORMAL-only ones skip EASY.
    EXPECT(RosterFirstWave(RosterKind::MAGNETAR, 2) >= 10, "magnetar late on HARD");
    EXPECT(RosterFirstWave(RosterKind::GENESIS, 2) >= 10, "genesis late on HARD");
    EXPECT(RosterFirstWave(RosterKind::QUASAR, 2) >= 15, "quasar from 15 on HARD");
    EXPECT(RosterFirstWave(RosterKind::REGULUS, 0) == 0, "regulus not on EASY");
    EXPECT(RosterFirstWave(RosterKind::SCOPE, 0) == 0, "scope not on EASY");

    // Odds start low on the first wave, rise 2% per wave and stop at the cap;
    // the weighted kinds never exceed 100% together.
    EXPECT(RosterChancePct(RosterKind::GIMBAL, 3, 0) == 6, "gimbal start");
    EXPECT(RosterChancePct(RosterKind::GIMBAL, 4, 0) == 8, "gimbal +2");
    EXPECT(RosterChancePct(RosterKind::GIMBAL, 30, 0) == 20, "gimbal cap");
    for (int d = 0; d < 3; ++d)
        for (int wave = 1; wave <= kFinalWave[d]; ++wave) {
            int total = 0;
            for (const RosterEntry& entry : kEnemyRoster)
                total += RosterChancePct(entry.kind, wave, d);
            EXPECT(total <= 100, "%s wave %d total %d%%", kDiff[d], wave, total);
        }

    if (g_failures == 0) std::printf("enemy roster: all checks passed\n");
    return g_failures == 0 ? 0 : 1;
}
