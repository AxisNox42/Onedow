#pragma once
// ─────────────────────────────────────────────────────────────
// Enemy roster: the wave each regular enemy first appears on, per
// difficulty, and its spawn odds from then on. First appearances are
// spread over the run instead of all landing in waves 1-6, and never on an
// elite or boss wave (see kWaveSchedules in WaveSystem.h) so a new enemy and
// an elite do not share it.
// Elite kinds (Regulus, Antares, Genesis, Quasar) are not on the roster: they only
// enter as the scheduled elite of an elite wave (EnemySpawner.cpp).
// Kept free of game headers so tests can include it on its own.
// ─────────────────────────────────────────────────────────────
#include <algorithm>

inline constexpr int kRosterDifficulties = 4;   // EASY, NORMAL, HARD, NIGHTMARE

enum class RosterKind {
    SWARM, ROTOR, GIMBAL, SCOPE, GRAVIS, MAGNETAR,
    COUNT
};

struct RosterEntry {
    RosterKind kind;
    int firstWave[kRosterDifficulties];   // 0 = never on that difficulty
    int startPct;       // spawn odds on its first wave
    int capPct;         // odds stop rising here (+2% per wave)
};

// Gimbal and Scope trade places over a run so they never crowd together:
//   A  Gimbal only, its share climbing   (Scope absent)
//   B  swap: Scope few, Gimbal falling
//   C  Scope heavy, Gimbal low
// Each phase is an inclusive wave range; {0, 0} = the phase does not exist.
// Scope's first wave (RosterEntry) is the start of B; Gimbal's is the start of A.
struct GimbalScopePhases {
    int a[2], b[2], c[2];
};
inline constexpr GimbalScopePhases kGimbalScope[kRosterDifficulties] = {
    /* EASY      */ { { 2, 6 }, { 0, 0 },  { 0, 0 } },
    /* NORMAL    */ { { 2, 4 }, { 5, 6 },  { 7, 8 } },
    /* HARD      */ { { 2, 4 }, { 5, 7 },  { 8, 12 } },
    /* NIGHTMARE */ { { 2, 5 }, { 6, 10 }, { 11, 18 } },
};

inline constexpr int kGimbalStartPct = 12, kGimbalRisePct = 5, kGimbalPeakPct = 30;
inline constexpr int kGimbalSwapFromPct = 18, kGimbalSwapToPct = 10, kGimbalLatePct = 8;
inline constexpr float kScopeSwapFromRate = 0.5f, kScopeSwapToRate = 1.0f;
inline constexpr float kScopeLateFromRate = 1.4f, kScopeLateToRate = 2.2f;

inline bool InPhase(const int (&range)[2], int wave) {
    return range[0] > 0 && wave >= range[0] && wave <= range[1];
}

// Position inside a phase, 0 at its first wave and 1 at its last.
inline float PhaseT(const int (&range)[2], int wave) {
    const int len = range[1] - range[0];
    return len <= 0 ? 0.0f : (float)(wave - range[0]) / (float)len;
}

inline const GimbalScopePhases& GimbalScopeFor(int difficulty) {
    return kGimbalScope[std::clamp(difficulty, 0, kRosterDifficulties - 1)];
}

inline int GimbalChancePct(int wave, int difficulty) {
    const GimbalScopePhases& ph = GimbalScopeFor(difficulty);
    if (InPhase(ph.a, wave))
        return std::min(kGimbalPeakPct, kGimbalStartPct + kGimbalRisePct * (wave - ph.a[0]));
    if (InPhase(ph.b, wave))
        return (int)((float)kGimbalSwapFromPct +
            (float)(kGimbalSwapToPct - kGimbalSwapFromPct) * PhaseT(ph.b, wave) + 0.5f);
    if (InPhase(ph.c, wave)) return kGimbalLatePct;
    return 0;
}

// Multiplier on the ranged (Scope) spawn rate; 0 = no Scope this wave.
inline float ScopeRateMult(int wave, int difficulty) {
    const GimbalScopePhases& ph = GimbalScopeFor(difficulty);
    if (InPhase(ph.b, wave))
        return kScopeSwapFromRate + (kScopeSwapToRate - kScopeSwapFromRate) * PhaseT(ph.b, wave);
    if (InPhase(ph.c, wave))
        return kScopeLateFromRate + (kScopeLateToRate - kScopeLateFromRate) * PhaseT(ph.c, wave);
    return 0.0f;
}

// ROTOR takes whatever share the others leave once it is in the pool.
// SCOPE only gates the separate ranged spawn path; it has no regular odds.
inline constexpr RosterEntry kEnemyRoster[] = {
    { RosterKind::SWARM,    { 1,  1,  1,  1 }, 12, 12 },
    { RosterKind::ROTOR,    { 2,  2,  1,  1 },  0,  0 },
    { RosterKind::GIMBAL,   { 2,  2,  2,  2 },  0,  0 },   // odds: kGimbalScope
    { RosterKind::SCOPE,    { 0,  5,  5,  6 },  0,  0 },   // rate: kGimbalScope
    { RosterKind::GRAVIS,   { 0,  7,  6,  4 },  3,  7 },
    { RosterKind::MAGNETAR, { 0,  0, 11,  8 },  4, 12 },
};

inline const RosterEntry& RosterEntryFor(RosterKind kind) {
    for (const RosterEntry& entry : kEnemyRoster)
        if (entry.kind == kind) return entry;
    return kEnemyRoster[0];
}

// difficulty: 0 EASY, 1 NORMAL, 2 HARD, 3 NIGHTMARE.
inline int RosterFirstWave(RosterKind kind, int difficulty) {
    return RosterEntryFor(kind).firstWave[std::clamp(difficulty, 0, kRosterDifficulties - 1)];
}

inline bool RosterUnlocked(RosterKind kind, int wave, int difficulty) {
    const int first = RosterFirstWave(kind, difficulty);
    return first > 0 && wave >= first;
}

// Regular spawn odds in percent; rises 2% per wave after the first one
// (Gimbal follows kGimbalScope instead).
inline int RosterChancePct(RosterKind kind, int wave, int difficulty) {
    if (!RosterUnlocked(kind, wave, difficulty)) return 0;
    if (kind == RosterKind::GIMBAL) return GimbalChancePct(wave, difficulty);
    const RosterEntry& entry = RosterEntryFor(kind);
    const int grown = entry.startPct + 2 * (wave - RosterFirstWave(kind, difficulty));
    return std::min(entry.capPct, grown);
}

// Picks the kind of one regular spawn for roll100 in [0, 100). Weighted
// pieces go in table order and ROTOR keeps the remainder. Before ROTOR is
// in the pool the roll is spread over the unlocked kinds only.
inline RosterKind RosterPickRegular(int wave, int difficulty, int roll100) {
    int total = 0;
    for (const RosterEntry& entry : kEnemyRoster)
        total += RosterChancePct(entry.kind, wave, difficulty);
    const bool rotorIn = RosterUnlocked(RosterKind::ROTOR, wave, difficulty);
    if (total <= 0) return RosterKind::ROTOR;
    const int space = rotorIn ? 100 : total;
    const int roll = std::clamp(roll100, 0, 99) * space / 100;
    int cutoff = 0;
    for (const RosterEntry& entry : kEnemyRoster) {
        cutoff += RosterChancePct(entry.kind, wave, difficulty);
        if (roll < cutoff) return entry.kind;
    }
    return RosterKind::ROTOR;
}
