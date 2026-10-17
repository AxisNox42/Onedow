#include "EnemySpawner.h"

#include "Monster.h"
#include "MonsterManager.h"
#include "EnemyProgression.h"
#include "PlayerStats.h"
#include "../Core/WaveSystem.h"
#include "../System/Settings.h"
#include <algorithm>
#include <cstdlib>

struct EnemySpawnState {
    float mobTimer = 0.0f;
    float rangedTimer = 0.0f;
    int lastEliteWave = 0;
    int currentWave = 0;
    int spawnedThisWave = 0;
    // One bit per fifth wave picks between the difficulty's two elite
    // kinds; rolled once per run so the HUD can name the elite early.
    unsigned eliteRoll = 0;
    const Monster* eliteMob = nullptr;   // the scheduled elite while alive
};

static EnemySpawnState g_EnemySpawnState;

static int RegularTierHalfStepsForWave(
    int wave, const DifficultyParams& difficultyParams) {
    // 1.5, 2, 2.5, 3, 3.5, 4 unlock on waves 1 through 6.
    return std::min(2 + wave, difficultyParams.maxRegularTierHalfSteps);
}

static void SpecializeRegularMob(Monster& mob, int wave,
                                 const DifficultyParams& difficultyParams,
                                 float rotorHpMultiplier) {
    const int tier = RegularTierHalfStepsForWave(wave, difficultyParams);
    const int roll = rand() % 100;
    int cutoff = 0;
    auto tryKind = [&](int chance, MobKind kind) {
        cutoff += chance;
        if (roll >= cutoff) return false;
        mob.MakeKind(kind);
        return true;
    };

    // Each later wave adds a type to the pool and gradually raises its odds.
    if (tryKind(12, MobKind::SWARM)) return;
    if (tier >= 3 && tryKind(std::min(20, 6 + 2 * (wave - 1)),
                             MobKind::GIMBAL)) return;
    if (tier >= 4 && tryKind(std::min(18, 6 + 2 * (wave - 2)),
                             MobKind::GRAVIS)) return;
    if (tier >= 6) {
        const MobKind kind = (rand() & 1) ? MobKind::REGULUS : MobKind::ANTARES;
        if (tryKind(std::min(14, 4 + 2 * (wave - 4)), kind)) return;
    }
    if (tier >= 7 && tryKind(std::min(12, 4 + 2 * (wave - 5)),
                             MobKind::MAGNETAR)) return;
    if (tier >= 8) {
        const MobKind kind = (rand() & 1) ? MobKind::GENESIS : MobKind::QUASAR;
        if (tryKind(std::min(10, 2 + 2 * (wave - 6)), kind)) return;
    }

    // MakeKind scales HP, so the rotor-only augment applies after conversion.
    if (mob.kind == MobKind::ROTOR) mob.hp *= rotorHpMultiplier;
}

static MobKind ScheduledEliteKind(int wave, const DifficultyParams& difficultyParams) {
    const bool pick = (g_EnemySpawnState.eliteRoll >>
                       ((wave / ELITE_WAVE_INTERVAL) & 31)) & 1u;
    if (difficultyParams.eliteTierHalfSteps <= 6)
        return pick ? MobKind::REGULUS : MobKind::ANTARES;
    return pick ? MobKind::GENESIS : MobKind::QUASAR;
}

static void SpawnScheduledElite(MonsterManager& monsters, int screenWidth,
                                int screenHeight, int wave, float waveSeconds,
                                int waveSpawnCap, float hpMultiplier,
                                const DifficultyParams& difficultyParams) {
    if (wave % ELITE_WAVE_INTERVAL != 0 || waveSeconds < ELITE_SPAWN_WAVE_SECONDS ||
        g_EnemySpawnState.lastEliteWave == wave ||
        g_EnemySpawnState.spawnedThisWave >= waveSpawnCap) return;

    const MobKind elite = ScheduledEliteKind(wave, difficultyParams);

    const size_t before = monsters.monsters.size();
    // Natural spawns share the wave budget, but have no concurrent population cap.
    monsters.SpawnMob(screenWidth, screenHeight,
                      (int)before + 1, hpMultiplier);
    if (monsters.monsters.size() == before) return;
    Monster& mob = *monsters.monsters.back();
    mob.MakeKind(elite);
    if (elite == MobKind::GENESIS) {
        mob.spawnAnchorX = 160.0f +
            (float)(rand() % (screenWidth > 360 ? screenWidth - 320 : 1));
        mob.spawnAnchorY = 160.0f +
            (float)(rand() % (screenHeight > 360 ? screenHeight - 320 : 1));
    }
    g_EnemySpawnState.lastEliteWave = wave;
    g_EnemySpawnState.eliteMob = &mob;
    ++g_EnemySpawnState.spawnedThisWave;
}

// Drops the elite handle once it is no longer a living member of the field.
static void TrackScheduledElite(const MonsterManager& monsters) {
    const Monster* elite = g_EnemySpawnState.eliteMob;
    if (!elite) return;
    for (const Monster* mob : monsters.monsters)
        if (mob == elite && mob->alive) return;
    g_EnemySpawnState.eliteMob = nullptr;
}

void ResetEnemySpawnState() {
    g_EnemySpawnState = {};
    g_EnemySpawnState.eliteRoll = (unsigned)rand() ^ ((unsigned)rand() << 15);
}

ScheduledEliteInfo GetScheduledElite(int wave, Difficulty difficulty) {
    ScheduledEliteInfo info;
    if (wave <= 0 || wave % ELITE_WAVE_INTERVAL != 0) return info;
    info.scheduled = true;
    info.kind = ScheduledEliteKind(wave, GetDifficultyParams(difficulty));
    info.spawned = g_EnemySpawnState.lastEliteWave == wave;
    info.alive = info.spawned && g_EnemySpawnState.eliteMob != nullptr;
    return info;
}

void UpdateEnemySpawns(MonsterManager& monsters, const PlayerStats& stats,
                       int screenWidth, int screenHeight,
                       float delta, float gameTime, long long score,
                       Difficulty difficulty) {
#if defined(_DEBUG)
    constexpr float kEnemySpawnIntervalScale = 0.45f;
#else
    constexpr float kEnemySpawnIntervalScale = 1.0f;
#endif
    const int wave = CurrentWaveNumber(gameTime, difficulty);
    const float waveSeconds = gameTime - (wave - 1) * WAVE_DURATION_SECONDS;
    const DifficultyParams difficultyParams = GetDifficultyParams(difficulty);
    if (g_EnemySpawnState.currentWave != wave) {
        g_EnemySpawnState.currentWave = wave;
        g_EnemySpawnState.spawnedThisWave = 0;
        g_EnemySpawnState.mobTimer = 0.0f;
        g_EnemySpawnState.rangedTimer = 0.0f;
    }
    const float minutePressure = waveSeconds < 60.0f ? 1.0f
        : waveSeconds < 120.0f ? 1.15f : 1.30f;
    const float spawnPressure = (1.0f + 0.10f * (wave - 1)) * minutePressure;
    const int baseWaveSpawnCap = static_cast<int>(
        (100 + 12 * (wave - 1)) * difficultyParams.waveSpawnCapMultiplier + 0.5f);
    const int waveSpawnCap = std::max(1, baseWaveSpawnCap +
        stats.mobCapBonus + stats.rmobMaxBonus + TrialRangedMaxBonus(score));
    const EnemyProgression progression = EvaluateEnemyProgression(
        gameTime, score, stats.mobCapBonus);
    const float hpMultiplier = stats.monsterHpMult * difficultyParams.enemyHpMultiplier *
        progression.hpMultiplier * TrialEnemyHpMult(score);

    TrackScheduledElite(monsters);
    SpawnScheduledElite(monsters, screenWidth, screenHeight, wave, waveSeconds,
                        waveSpawnCap, hpMultiplier, difficultyParams);

    g_EnemySpawnState.mobTimer += delta;
    const float spawnInterval = std::max(0.05f,
        2.4f * stats.mobSpawnMult * kEnemySpawnIntervalScale /
        (spawnPressure * TrialSpawnRateMult(score) *
         difficultyParams.spawnRateMultiplier));
    if (g_EnemySpawnState.spawnedThisWave < waveSpawnCap &&
        g_EnemySpawnState.mobTimer >= spawnInterval) {
        const size_t before = monsters.monsters.size();
        monsters.SpawnMob(screenWidth, screenHeight,
                          (int)before + 1, hpMultiplier);
        if (monsters.monsters.size() > before) {
            SpecializeRegularMob(*monsters.monsters.back(), wave,
                                 difficultyParams,
                                 stats.rotorHpMult);
            ++g_EnemySpawnState.spawnedThisWave;
            g_EnemySpawnState.mobTimer -= spawnInterval;
        }
    }

    // Scope is tier 2.5 and uses its own ranged spawn path.
    if (RegularTierHalfStepsForWave(wave, difficultyParams) < 5) {
        g_EnemySpawnState.rangedTimer = 0.0f;
        return;
    }
    g_EnemySpawnState.rangedTimer += delta;
    float rangedInterval = 5.0f * TrialRangedIntervalMult(score) -
        stats.rmobSpawnDelayBonus;
    rangedInterval = std::max(1.0f, rangedInterval) /
        (spawnPressure * difficultyParams.spawnRateMultiplier) *
        kEnemySpawnIntervalScale * 2.5f;
    if (g_EnemySpawnState.spawnedThisWave < waveSpawnCap &&
        g_EnemySpawnState.rangedTimer >= rangedInterval) {
        const size_t before = monsters.rangedMobs.size();
        monsters.SpawnRangedMob(screenWidth, screenHeight,
                                stats.rmobHpMult * difficultyParams.enemyHpMultiplier *
                                    progression.hpMultiplier,
                                (int)before + 1);
        if (monsters.rangedMobs.size() > before) {
            ++g_EnemySpawnState.spawnedThisWave;
            g_EnemySpawnState.rangedTimer -= rangedInterval;
        }
    }
}
