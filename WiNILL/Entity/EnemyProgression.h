#pragma once

#include <algorithm>

// Preserve the active pre-removal run tuning while keeping its progression rules together.
struct EnemyProgression {
    float scoreIntensity = 0.0f;
    float spawnT = 0.0f;
    float lateSpawnT = 0.0f;
    float capT = 0.0f;
    float spawnRate = 0.0f;
    float hpMultiplier = 0.0f;
    int mobCap = 1;
};

inline float EnemySpeedRamp(float elapsedSeconds) {
    const float intensity = std::min(0.55f, std::max(0.0f, elapsedSeconds / 240.0f));
    return (1.0f + intensity * 0.075f) * 0.92f;
}

inline EnemyProgression EvaluateEnemyProgression(float elapsedSeconds,
                                                  long long score,
                                                  int mobCapBonus) {
    const auto clamp01 = [](float value) {
        return std::min(1.0f, std::max(0.0f, value));
    };
    EnemyProgression result;
    result.scoreIntensity = std::min(0.55f, std::max(0.0f, (float)score / 100000.0f));
    result.spawnT = clamp01(elapsedSeconds / 300.0f);
    result.lateSpawnT = clamp01((elapsedSeconds - 300.0f) / 300.0f);
    result.capT = clamp01(elapsedSeconds / 420.0f);
    result.spawnRate = (0.52f + result.spawnT * 1.75f + result.lateSpawnT * 0.55f) * 0.88f;

    const float hpT = result.spawnT;
    const float hpLateT = result.lateSpawnT;
    const float hpIntensity = std::min(1.2f, std::max(0.0f, (float)score / 100000.0f));
    const float scoreHpRamp = 1.0f + hpIntensity * 0.15f;
    constexpr float kRegularEnemyHpScale = 0.80f;
    result.hpMultiplier = (1.22f + hpT * 1.05f + hpLateT * 0.70f)
                        * scoreHpRamp * kRegularEnemyHpScale;

    const float capBonusT = clamp01(elapsedSeconds / 360.0f);
    const int scaledCapBonus = (int)((float)mobCapBonus * capBonusT * capBonusT);
    result.mobCap = 1 + (int)(4.0f * result.spawnT
                            + 45.0f * result.capT * result.capT
                            + 30.0f * result.lateSpawnT)
                  + scaledCapBonus;
    result.mobCap = std::min(160, std::max(1, result.mobCap));
    return result;
}
