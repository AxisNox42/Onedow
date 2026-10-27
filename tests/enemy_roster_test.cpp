// Enemy roster: first regular appearance per difficulty and the spawn pick.
#include <cstdio>

#include "EnemyRoster.h"
#include "WaveSystem.h"

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

static const int kD = kRosterDifficulties;
static const char* kDiff[4] = { "EASY", "NORMAL", "HARD", "NIGHTMARE" };
static int FinalWave(int d) { return FinalWaveCount(static_cast<Difficulty>(d)); }

// Kinds a regular spawn can turn into on this wave, over every roll.
static bool CanSpawn(RosterKind kind, int wave, int difficulty) {
    for (int roll = 0; roll < 100; ++roll)
        if (RosterPickRegular(wave, difficulty, roll) == kind) return true;
    return false;
}

int main() {
    // Swarm opens every run; Rotor follows from wave 2 (HARD: wave 1 too).
    for (int d = 0; d < kD; ++d) {
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
        for (int d = 0; d < kD; ++d) {
            const int first = entry.firstWave[d];
            for (int wave = 1; wave <= FinalWave(d); ++wave) {
                const bool can = CanSpawn(entry.kind, wave, d);
                if (first == 0 || wave < first)
                    EXPECT(!can, "%s kind %d spawns early on wave %d", kDiff[d], (int)entry.kind, wave);
                else
                    EXPECT(can, "%s kind %d missing on wave %d", kDiff[d], (int)entry.kind, wave);
            }
        }
    }

    // No enemy debuts on an elite or boss wave, and every debut is inside the run.
    for (const RosterEntry& entry : kEnemyRoster)
        for (int d = 0; d < kD; ++d) {
            const int first = entry.firstWave[d];
            EXPECT(first == 0 || !IsSpecialWave(first, static_cast<Difficulty>(d)),
                   "%s kind %d debuts on elite/boss wave %d",
                   kDiff[d], (int)entry.kind, first);
            EXPECT(first <= FinalWave(d), "%s kind %d after the run", kDiff[d], (int)entry.kind);
        }

    // HARD-only enemies wait until wave 10+, NORMAL-only ones skip EASY.
    // Elite kinds (Regulus, Genesis, Quasar) have no roster entry at all:
    // they only enter as an elite wave's scheduled elite.
    EXPECT(RosterFirstWave(RosterKind::MAGNETAR, 2) >= 10, "magnetar late on HARD");
    EXPECT(RosterFirstWave(RosterKind::MAGNETAR, 3) > 0, "magnetar on NIGHTMARE");
    EXPECT(RosterFirstWave(RosterKind::MAGNETAR, 1) == 0, "magnetar not on NORMAL");
    EXPECT((int)(sizeof(kEnemyRoster) / sizeof(kEnemyRoster[0])) == (int)RosterKind::COUNT,
           "one roster entry per kind");
    EXPECT(RosterFirstWave(RosterKind::GRAVIS, 0) == 0, "gravis not on EASY");
    EXPECT(RosterFirstWave(RosterKind::SCOPE, 0) == 0, "scope not on EASY");

    // Gimbal and Scope trade places: Gimbal alone and rising (A), then Scope
    // few while Gimbal falls (B), then Scope heavy with Gimbal low (C).
    for (int d = 0; d < kD; ++d) {
        const GimbalScopePhases& ph = kGimbalScope[d];
        EXPECT(RosterFirstWave(RosterKind::GIMBAL, d) == ph.a[0], "%s gimbal starts with phase A", kDiff[d]);
        EXPECT(RosterFirstWave(RosterKind::SCOPE, d) == ph.b[0], "%s scope starts with phase B", kDiff[d]);
        if (ph.b[0] > 0) {
            EXPECT(ph.b[0] - ph.a[0] >= 3, "%s scope debuts too close to gimbal", kDiff[d]);
            EXPECT(ph.b[0] == ph.a[1] + 1 && ph.c[0] == ph.b[1] + 1, "%s phases are contiguous", kDiff[d]);
            EXPECT(ph.c[1] == FinalWave(d), "%s phase C runs to the end", kDiff[d]);
        }
        for (int wave = 1; wave <= FinalWave(d); ++wave) {
            const int g = GimbalChancePct(wave, d);
            const float sc = ScopeRateMult(wave, d);
            if (wave < ph.a[0]) EXPECT(g == 0 && sc == 0.0f, "%s wave %d before phases", kDiff[d], wave);
            if (InPhase(ph.a, wave)) EXPECT(g > 0 && sc == 0.0f, "%s wave %d: gimbal only", kDiff[d], wave);
            if (InPhase(ph.b, wave)) EXPECT(g > 0 && sc > 0.0f, "%s wave %d: swap", kDiff[d], wave);
            if (InPhase(ph.c, wave)) EXPECT(g > 0 && g < kGimbalPeakPct && sc >= kScopeLateFromRate,
                                           "%s wave %d: scope heavy", kDiff[d], wave);
            EXPECT(g <= kGimbalPeakPct, "%s wave %d gimbal over peak", kDiff[d], wave);
        }
        if (ph.b[0] > 0) {
            EXPECT(GimbalChancePct(ph.b[0], d) < GimbalChancePct(ph.a[1], d) + 1, "%s gimbal falls into B", kDiff[d]);
            EXPECT(ScopeRateMult(ph.c[0], d) > ScopeRateMult(ph.b[1], d), "%s scope rises into C", kDiff[d]);
        }
    }
    EXPECT(GimbalChancePct(2, 3) == 12 && GimbalChancePct(5, 3) == 27, "nightmare gimbal climbs");
    EXPECT(ScopeRateMult(11, 3) == kScopeLateFromRate, "nightmare scope heavy starts");

    // The weighted kinds never exceed 100% together.
    for (int d = 0; d < kD; ++d)
        for (int wave = 1; wave <= FinalWave(d); ++wave) {
            int total = 0;
            for (const RosterEntry& entry : kEnemyRoster)
                total += RosterChancePct(entry.kind, wave, d);
            EXPECT(total <= 100, "%s wave %d total %d%%", kDiff[d], wave, total);
        }

    if (g_failures == 0) std::printf("enemy roster: all checks passed\n");
    return g_failures == 0 ? 0 : 1;
}
