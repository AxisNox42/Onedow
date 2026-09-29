#include "EnemySpawner.h"

#include "Monster.h"
#include "MonsterManager.h"
#include "EnemyProgression.h"
#include "PlayerStats.h"
#include "../System/Settings.h"
#include <algorithm>
#include <cstdlib>

struct EnemySpawnState {
    float mobTimer = 0.0f;
    float rangedTimer = 0.0f;
};

static EnemySpawnState g_EnemySpawnState;

static void ApplyRotorSpawnSpecialization(MonsterManager& monsters, Monster& mob,
                                          float rotorHpMultiplier,
                                          int screenWidth, int screenHeight,
                                          float elapsedSeconds, int mobCap) {
    if (mob.kind != MobKind::ROTOR) return;
    mob.hp *= rotorHpMultiplier;

    int gravisCount = 0;
    for (auto* existing : monsters.monsters)
        if (existing->alive && existing->kind == MobKind::GRAVIS) ++gravisCount;
    const bool gravisReady = elapsedSeconds >= 150.0f;
    const int gravisChance = elapsedSeconds >= 420.0f ? 4 : 2;

    int quasarCount = 0;
    for (auto* existing : monsters.monsters)
        if (existing->alive && existing->kind == MobKind::QUASAR) ++quasarCount;
    const bool quasarReady = elapsedSeconds >= 45.0f;
    const int quasarChance = elapsedSeconds >= 240.0f ? 8 : 5;

    if (gravisReady && gravisCount < 1 && (rand() % 100) < gravisChance) {
        mob.MakeKind(MobKind::GRAVIS);
    } else if (quasarReady && quasarCount < 2 &&
               (rand() % 100) < quasarChance) {
        mob.MakeKind(MobKind::QUASAR);
    } else if ((rand() % 100) < 12) {
        mob.MakeKind(MobKind::GENESIS);
        mob.spawnAnchorX = 160.0f +
            (float)(rand() % (screenWidth > 360 ? screenWidth - 320 : 1));
        mob.spawnAnchorY = 160.0f +
            (float)(rand() % (screenHeight > 360 ? screenHeight - 320 : 1));
    } else {
        const float swarmT = std::min(1.0f, std::max(0.0f, elapsedSeconds / 300.0f));
        const int swarmChance = 4 + (int)(4.0f * swarmT);
        if ((rand() % 100) < swarmChance) {
            int swarmCount = 0;
            for (auto* existing : monsters.monsters)
                if (existing->alive && existing->kind == MobKind::SWARM) ++swarmCount;
            const float capMul = 0.10f + 0.08f * swarmT;
            int swarmCap = (int)((float)mobCap * capMul);
            if (swarmCap < 2) swarmCap = 2;
            if (swarmCount < swarmCap) mob.MakeKind(MobKind::SWARM);
        }
    }
}
void ResetEnemySpawnState() {
    g_EnemySpawnState = {};
}

void UpdateEnemySpawns(MonsterManager& monsters, const PlayerStats& stats,
                       int screenWidth, int screenHeight,
                       float delta, float gameTime, long long score) {
#if defined(_DEBUG)
    constexpr float kEnemySpawnTimeScale = 4.0f;
    constexpr float kEnemySpawnIntervalScale = 0.45f;
#else
    constexpr float kEnemySpawnTimeScale = 1.0f;
    constexpr float kEnemySpawnIntervalScale = 1.0f;
#endif

    const float elapsedSeconds = gameTime * kEnemySpawnTimeScale;
    const EnemyProgression progression = EvaluateEnemyProgression(
        elapsedSeconds, score, stats.mobCapBonus);
    const float intensity = progression.scoreIntensity;
    const float spawnRate = progression.spawnRate;
    const float hpRamp = progression.hpMultiplier;
    const int mobCap = progression.mobCap;
    const int spawnX = 0, spawnY = 0;
    const int spawnWidth = screenWidth, spawnHeight = screenHeight;

    g_EnemySpawnState.mobTimer += delta;
    float spawnInterval = 0.55f * stats.mobSpawnMult /
        (spawnRate * TrialSpawnRateMult(score));
    spawnInterval *= kEnemySpawnIntervalScale;
    const float effectiveHpMultiplier = TrialEnemyHpMult(score);
    if (g_EnemySpawnState.mobTimer > spawnInterval) {
        auto spawnOne = [&]() {
            const size_t countBefore = monsters.monsters.size();
            monsters.SpawnMob(screenWidth, screenHeight, mobCap,
                              stats.monsterHpMult * hpRamp * effectiveHpMultiplier,
                              (float)spawnX, (float)spawnY,
                              spawnWidth, spawnHeight);
            if (monsters.monsters.size() > countBefore) {
                ApplyRotorSpawnSpecialization(
                    monsters, *monsters.monsters.back(), stats.rotorHpMult,
                    screenWidth, screenHeight, elapsedSeconds, mobCap);
            }
        };

        int packCount = 2;
        if (elapsedSeconds >= 120.0f) ++packCount;
        if (elapsedSeconds >= 300.0f) ++packCount;
        if (elapsedSeconds >= 480.0f) ++packCount;
        if (packCount > 6) packCount = 6;
        for (int i = 0; i < packCount &&
             (int)monsters.monsters.size() < mobCap; ++i) {
            spawnOne();
        }
        g_EnemySpawnState.mobTimer = 0.0f;
    }

    const DifficultyParams difficulty = GetDifficultyParams(Difficulty::NORMAL);
    g_EnemySpawnState.rangedTimer += delta;
    float rangedInterval = difficulty.rangedSpawnInterval *
        TrialRangedIntervalMult(score) - stats.rmobSpawnDelayBonus;
    if (rangedInterval < 1.0f) rangedInterval = 1.0f;
    rangedInterval /= spawnRate;
    rangedInterval *= kEnemySpawnIntervalScale;
    rangedInterval *= 2.5f;
    int rangedMax = (int)(difficulty.rangedMaxBase + stats.rmobMaxBonus +
                          TrialRangedMaxBonus(score) + intensity * 2.0f);
    if (rangedMax > 16) rangedMax = 16;
    if (g_EnemySpawnState.rangedTimer > rangedInterval) {
        monsters.SpawnRangedMob(screenWidth, screenHeight,
                                stats.rmobHpMult * hpRamp, rangedMax,
                                (float)spawnX, (float)spawnY,
                                spawnWidth, spawnHeight);
        g_EnemySpawnState.rangedTimer = 0.0f;
    }
}
