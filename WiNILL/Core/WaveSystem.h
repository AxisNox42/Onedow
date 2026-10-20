#pragma once

#include "../System/Settings.h"
#include <algorithm>

inline constexpr float REGULAR_WAVE_DURATION_SECONDS = 60.0f;
inline constexpr float ELITE_WAVE_DURATION_SECONDS = 120.0f;
inline constexpr float BOSS_WAVE_DURATION_SECONDS = 15.0f * 60.0f;
inline constexpr int   ELITE_WAVE_INTERVAL = 5;
inline constexpr float ELITE_SPAWN_WAVE_SECONDS = ELITE_WAVE_DURATION_SECONDS * 0.5f;

// Keep difficulty-specific run length in one place.
inline int FinalWaveCount(Difficulty difficulty) {
    switch (difficulty) {
    case Difficulty::EASY:
        return 10;
    case Difficulty::NORMAL:
        return 15;
    case Difficulty::HARD:
        return 20;
    }
    return 15;
}

// The final wave is reserved for the boss encounter; regular elite waves
// keep their existing fifth-wave schedule before it.
inline bool IsBossWave(int wave, Difficulty difficulty) {
    return wave == FinalWaveCount(difficulty);
}

inline bool IsEliteWaveNumber(int wave, Difficulty difficulty) {
    return wave > 0 && wave < FinalWaveCount(difficulty) &&
           wave % ELITE_WAVE_INTERVAL == 0;
}

inline float WaveDurationSeconds(int wave, Difficulty difficulty) {
    if (IsBossWave(wave, difficulty)) return BOSS_WAVE_DURATION_SECONDS;
    if (IsEliteWaveNumber(wave, difficulty)) return ELITE_WAVE_DURATION_SECONDS;
    return REGULAR_WAVE_DURATION_SECONDS;
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
