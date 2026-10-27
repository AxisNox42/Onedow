#pragma once

#include "../System/Settings.h"
#include <algorithm>

inline constexpr float REGULAR_WAVE_DURATION_SECONDS = 60.0f;
inline constexpr float ELITE_WAVE_DURATION_SECONDS = 120.0f;
inline constexpr float BOSS_WAVE_DURATION_SECONDS = 15.0f * 60.0f;
// A boss wave that is not the run's last one only lasts like an elite wave.
inline constexpr float MID_BOSS_WAVE_DURATION_SECONDS = ELITE_WAVE_DURATION_SECONDS;
inline constexpr float ELITE_SPAWN_WAVE_SECONDS = ELITE_WAVE_DURATION_SECONDS * 0.5f;

// Regular spawns slow down on elite and boss waves (interval x this).
inline constexpr float SPECIAL_WAVE_SPAWN_STRETCH = 1.5f;

// ─── Difficulty wave table ───────────────────────────────────────────────────
// One row per difficulty: run length and the waves that host elites/bosses.
// The last boss wave is always the final wave; its defeat wins the run. A
// wave may be both an elite wave and a boss wave (NIGHTMARE wave 9).
inline constexpr int kMaxScheduledElites = 4;
inline constexpr int kMaxScheduledBosses = 2;

struct WaveSchedule {
    int finalWave;
    int eliteCount;
    int eliteWaves[kMaxScheduledElites];
    int bossCount;
    int bossWaves[kMaxScheduledBosses];
};

inline constexpr WaveSchedule kWaveSchedules[DIFFICULTY_COUNT] = {
    /* EASY      */ {  6, 1, { 3, 0, 0, 0 },    1, { 6, 0 } },
    /* NORMAL    */ {  8, 2, { 4, 6, 0, 0 },    1, { 8, 0 } },
    /* HARD      */ { 12, 3, { 4, 7, 10, 0 },   1, { 12, 0 } },
    /* NIGHTMARE */ { 18, 4, { 5, 9, 13, 16 },  2, { 9, 18 } },
};

inline const WaveSchedule& WaveScheduleFor(Difficulty difficulty) {
    const int idx = static_cast<int>(difficulty);
    return kWaveSchedules[idx >= 0 && idx < DIFFICULTY_COUNT ? idx : 1];
}

inline int FinalWaveCount(Difficulty difficulty) {
    return WaveScheduleFor(difficulty).finalWave;
}

inline bool IsBossWave(int wave, Difficulty difficulty) {
    const WaveSchedule& s = WaveScheduleFor(difficulty);
    for (int i = 0; i < s.bossCount; ++i)
        if (s.bossWaves[i] == wave) return true;
    return false;
}

// The boss whose defeat wins the run.
inline bool IsFinalBossWave(int wave, Difficulty difficulty) {
    return wave == FinalWaveCount(difficulty);
}

inline bool IsEliteWaveNumber(int wave, Difficulty difficulty) {
    const WaveSchedule& s = WaveScheduleFor(difficulty);
    for (int i = 0; i < s.eliteCount; ++i)
        if (s.eliteWaves[i] == wave) return true;
    return false;
}

// Zero-based position of `wave` among the elite waves, or -1.
inline int EliteOrdinal(int wave, Difficulty difficulty) {
    const WaveSchedule& s = WaveScheduleFor(difficulty);
    for (int i = 0; i < s.eliteCount; ++i)
        if (s.eliteWaves[i] == wave) return i;
    return -1;
}

inline bool IsSpecialWave(int wave, Difficulty difficulty) {
    return IsBossWave(wave, difficulty) || IsEliteWaveNumber(wave, difficulty);
}

inline float WaveDurationSeconds(int wave, Difficulty difficulty) {
    if (IsFinalBossWave(wave, difficulty)) return BOSS_WAVE_DURATION_SECONDS;
    if (IsBossWave(wave, difficulty)) return MID_BOSS_WAVE_DURATION_SECONDS;
    if (IsEliteWaveNumber(wave, difficulty)) return ELITE_WAVE_DURATION_SECONDS;
    return REGULAR_WAVE_DURATION_SECONDS;
}

// Who a spawn belongs to, for the slowdown exemption hook.
enum class SpawnSource { Regular, Ranged, Regulus };

// Hook for spawns that must ignore the elite/boss-wave slowdown. Regulus is
// exempt (it is becoming an elite). It still rolls through the shared regular
// timer today; give it its own timer when it moves to the elite path and ask
// this hook with SpawnSource::Regulus.
inline bool ExemptFromSpecialWaveSlowdown(SpawnSource source) {
    return source == SpawnSource::Regulus;
}

// Interval multiplier from the wave type (1.0 = unchanged). It is derived
// from the current wave every frame, so it ends with the wave on its own.
inline float SpecialWaveSpawnStretch(int wave, Difficulty difficulty,
                                     SpawnSource source = SpawnSource::Regular) {
    if (!IsSpecialWave(wave, difficulty)) return 1.0f;
    if (ExemptFromSpecialWaveSlowdown(source)) return 1.0f;
    return SPECIAL_WAVE_SPAWN_STRETCH;
}

// Wave numbers are one-based. Asking for finalWave + 1 returns total run time.
inline float WaveStartTimeSeconds(int wave, Difficulty difficulty) {
    const int finalWave = FinalWaveCount(difficulty);
    const int completedWaves = std::clamp(wave - 1, 0, finalWave);
    float start = 0.0f;
    for (int completed = 1; completed <= completedWaves; ++completed)
        start += WaveDurationSeconds(completed, difficulty);
    return start;
}

inline float TotalWaveDurationSeconds(Difficulty difficulty) {
    return WaveStartTimeSeconds(FinalWaveCount(difficulty) + 1, difficulty);
}

inline int CurrentWaveNumber(float elapsedSeconds, Difficulty difficulty) {
    const int finalWave = FinalWaveCount(difficulty);
    const float elapsed = std::max(0.0f, elapsedSeconds);
    for (int wave = 1; wave < finalWave; ++wave) {
        if (elapsed < WaveStartTimeSeconds(wave + 1, difficulty)) return wave;
    }
    return finalWave;
}

inline float CurrentWaveSecondsRemaining(float elapsedSeconds,
                                         Difficulty difficulty) {
    const float elapsed = std::max(0.0f, elapsedSeconds);
    if (elapsed >= TotalWaveDurationSeconds(difficulty)) return 0.0f;
    const int wave = CurrentWaveNumber(elapsed, difficulty);
    return std::max(0.0f, WaveStartTimeSeconds(wave + 1, difficulty) - elapsed);
}

inline bool WaveRunComplete(float elapsedSeconds, Difficulty difficulty) {
    return elapsedSeconds >= TotalWaveDurationSeconds(difficulty);
}
