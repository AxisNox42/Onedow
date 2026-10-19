#pragma once
// ─────────────────────────────────────────────────────────────
// Enemy roster: the wave each regular enemy first appears on, per
// difficulty, and its spawn odds from then on. First appearances are
// spread over the run instead of all landing in waves 1-6, and never on an
// elite wave (5, 10, 15, 20) so a new enemy and an elite do not share it.
// Kept free of game headers so tests can include it on its own.
// ─────────────────────────────────────────────────────────────
#include <algorithm>

enum class RosterKind {
    SWARM, ROTOR, GIMBAL, GRAVIS, SCOPE,
    REGULUS, ANTARES, MAGNETAR, GENESIS, QUASAR,
    COUNT
};

struct RosterEntry {
    RosterKind kind;
    int firstWave[3];   // EASY, NORMAL, HARD; 0 = never on that difficulty
    int startPct;       // spawn odds on its first wave
    int capPct;         // odds stop rising here (+2% per wave)
};

// ROTOR takes whatever share the others leave once it is in the pool.
// SCOPE only gates the separate ranged spawn path; it has no regular odds.
inline constexpr RosterEntry kEnemyRoster[] = {
    { RosterKind::SWARM,    { 1,  1,  1 }, 12, 12 },
    { RosterKind::ROTOR,    { 2,  2,  1 },  0,  0 },
    { RosterKind::GIMBAL,   { 3,  3,  2 },  6, 20 },
    { RosterKind::GRAVIS,   { 6,  4,  3 },  6, 18 },
    { RosterKind::SCOPE,    { 0,  6,  4 },  0,  0 },
    { RosterKind::REGULUS,  { 0,  8,  6 },  3,  7 },
    { RosterKind::ANTARES,  { 0, 11,  8 },  3,  7 },
    { RosterKind::MAGNETAR, { 0,  0, 11 },  4, 12 },
    { RosterKind::GENESIS,  { 0,  0, 13 },  2,  5 },
    { RosterKind::QUASAR,   { 0,  0, 16 },  2,  5 },
};

inline const RosterEntry& RosterEntryFor(RosterKind kind) {
    for (const RosterEntry& entry : kEnemyRoster)
        if (entry.kind == kind) return entry;
    return kEnemyRoster[0];
}

// difficulty: 0 EASY, 1 NORMAL, 2 HARD.
inline int RosterFirstWave(RosterKind kind, int difficulty) {
    return RosterEntryFor(kind).firstWave[std::clamp(difficulty, 0, 2)];
}

inline bool RosterUnlocked(RosterKind kind, int wave, int difficulty) {
    const int first = RosterFirstWave(kind, difficulty);
    return first > 0 && wave >= first;
}

// Regular spawn odds in percent; rises 2% per wave after the first one.
inline int RosterChancePct(RosterKind kind, int wave, int difficulty) {
    if (!RosterUnlocked(kind, wave, difficulty)) return 0;
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
