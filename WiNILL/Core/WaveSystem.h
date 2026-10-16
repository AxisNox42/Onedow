#pragma once

#include "../System/Settings.h"
#include <algorithm>

inline constexpr float WAVE_DURATION_SECONDS = 180.0f;

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

inline int CurrentWaveNumber(float elapsedSeconds, Difficulty difficulty) {
    const int finalWave = FinalWaveCount(difficulty);
    const int elapsedWaves = static_cast<int>(
        std::max(0.0f, elapsedSeconds) / WAVE_DURATION_SECONDS);
    return std::min(finalWave, elapsedWaves + 1);
}

inline float CurrentWaveSecondsRemaining(float elapsedSeconds,
                                         Difficulty difficulty) {
    if (elapsedSeconds >= FinalWaveCount(difficulty) * WAVE_DURATION_SECONDS)
        return 0.0f;
    const int wave = CurrentWaveNumber(elapsedSeconds, difficulty);
    const float waveEnd = wave * WAVE_DURATION_SECONDS;
    return std::max(0.0f, waveEnd - std::max(0.0f, elapsedSeconds));
}

inline bool WaveRunComplete(float elapsedSeconds, Difficulty difficulty) {
    return elapsedSeconds >= FinalWaveCount(difficulty) * WAVE_DURATION_SECONDS;
}
