#include "EnemySpawner.h"

#include "Monster.h"
#include "MonsterManager.h"
#include "EnemyProgression.h"
#include "EnemyRoster.h"
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
    // Index into kEliteKinds for each scheduled elite wave, rolled once per
    // run so the HUD can mark it before the elite enters.
    // Sized for kMaxScheduledElites.
    unsigned char elitePick[kMaxScheduledElites] = {};
    const Monster* eliteMob = nullptr;   // the scheduled elite while alive
    int lastBossWave = 0;                // the boss wave that already got its boss
    bool normaSpawned = false;           // the final boss entered this run
    bool normaAlive = false;             // any boss is on the field
    bool normaDefeated = false;          // the final boss fell: the run is won
};

static EnemySpawnState g_EnemySpawnState;

static MobKind MobKindFor(RosterKind kind) {
    switch (kind) {
    case RosterKind::SWARM:    return MobKind::SWARM;
    case RosterKind::GIMBAL:   return MobKind::GIMBAL;
    case RosterKind::GRAVIS:   return MobKind::GRAVIS;
    case RosterKind::MAGNETAR: return MobKind::MAGNETAR;
    default:                   return MobKind::ROTOR;
    }
}

// Converts a freshly spawned ROTOR into this wave's pick from the roster
// (EnemyRoster.h holds each enemy's first wave per difficulty and its odds).
static void SpecializeRegularMob(Monster& mob, int wave, Difficulty difficulty,
                                 float rotorHpMultiplier) {
    const RosterKind pick = RosterPickRegular(wave, (int)difficulty, rand() % 100);
    if (pick != RosterKind::ROTOR) {
        mob.MakeKind(MobKindFor(pick));
        return;
    }
    // MakeKind scales HP, so the rotor-only augment applies after conversion.
    mob.hp *= rotorHpMultiplier;
}

// Every elite wave, on every difficulty, brings one of these at random.
static constexpr MobKind kEliteKinds[] = {
    MobKind::QUASAR, MobKind::REGULUS, MobKind::GENESIS, MobKind::ANTARES
};
static constexpr int kEliteKindCount = (int)(sizeof(kEliteKinds) / sizeof(kEliteKinds[0]));

static MobKind ScheduledEliteKind(int wave, Difficulty difficulty) {
    constexpr int slots = (int)sizeof(EnemySpawnState::elitePick);
    const int slot = std::clamp(EliteOrdinal(wave, difficulty), 0, slots - 1);
    return kEliteKinds[g_EnemySpawnState.elitePick[slot] % kEliteKindCount];
}

static void SpawnScheduledElite(MonsterManager& monsters, int screenWidth,
                                int screenHeight, int wave, float waveSeconds,
                                Difficulty difficulty,
                                int waveSpawnCap, float hpMultiplier) {
    if (!IsEliteWaveNumber(wave, difficulty) ||
        waveSeconds < ELITE_SPAWN_WAVE_SECONDS ||
        g_EnemySpawnState.lastEliteWave == wave ||
        g_EnemySpawnState.spawnedThisWave >= waveSpawnCap) return;

    const MobKind elite = ScheduledEliteKind(wave, difficulty);

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

// A boss enters once per boss wave, at its start, from above the middle of
// the screen. Ordinary spawns keep going around it. Only the final wave's
// boss decides the run.
static void PlaceNormaBoss(MonsterManager& monsters, int screenWidth,
                           float hpMultiplier, bool finalBoss) {
    Monster* boss = new Monster((float)screenWidth * 0.5f, -160.0f, hpMultiplier);
    boss->MakeKind(MobKind::NORMA);
    monsters.monsters.push_back(boss);
    g_EnemySpawnState.normaAlive = true;
    if (finalBoss) g_EnemySpawnState.normaSpawned = true;
}

static void SpawnNormaBoss(MonsterManager& monsters, int screenWidth, int wave,
                           Difficulty difficulty, float hpMultiplier) {
    if (!IsBossWave(wave, difficulty) || g_EnemySpawnState.lastBossWave == wave) return;
    g_EnemySpawnState.lastBossWave = wave;
    PlaceNormaBoss(monsters, screenWidth, hpMultiplier,
                   IsFinalBossWave(wave, difficulty));
}

void DebugSpawnNorma(MonsterManager& monsters, int screenWidth, float hpMultiplier,
                     Difficulty difficulty) {
    if (g_EnemySpawnState.normaSpawned && !g_EnemySpawnState.normaDefeated) return;
    g_EnemySpawnState.normaDefeated = false;
    const float bossWaveStart = WaveStartTimeSeconds(FinalWaveCount(difficulty), difficulty);
    const EnemyProgression ramp = EvaluateEnemyProgression(bossWaveStart, 0, 0);
    PlaceNormaBoss(monsters, screenWidth, hpMultiplier * ramp.hpMultiplier, true);
}

// Looked up by kind rather than by pointer: dead mobs are freed elsewhere.
static void TrackNormaBoss(const MonsterManager& monsters) {
    bool alive = false;
    for (const Monster* mob : monsters.monsters)
        if (mob->kind == MobKind::NORMA && mob->alive) { alive = true; break; }
    g_EnemySpawnState.normaAlive = alive;
    if (!g_EnemySpawnState.normaSpawned || g_EnemySpawnState.normaDefeated || alive) return;
    g_EnemySpawnState.normaDefeated = true;
}

bool NormaDefeated() { return g_EnemySpawnState.normaDefeated; }

void ResetEnemySpawnState() {
    g_EnemySpawnState = {};
    for (unsigned char& pick : g_EnemySpawnState.elitePick)
        pick = (unsigned char)(rand() % kEliteKindCount);
}

ScheduledEliteInfo GetScheduledElite(int wave, Difficulty difficulty) {
    ScheduledEliteInfo info;
    if (!IsEliteWaveNumber(wave, difficulty)) return info;
    info.scheduled = true;
    info.kind = ScheduledEliteKind(wave, difficulty);
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
    const float waveSeconds = gameTime - WaveStartTimeSeconds(wave, difficulty);
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
    TrackNormaBoss(monsters);
    SpawnNormaBoss(monsters, screenWidth, wave, difficulty, hpMultiplier);
    SpawnScheduledElite(monsters, screenWidth, screenHeight, wave, waveSeconds,
                        difficulty,
                        waveSpawnCap, hpMultiplier);

    // While NORMA fights, regular mobs come at half the rate. Elite and boss
    // waves slow them too; the two do not stack, the stronger one applies.
    // The wave term follows the current wave, so it lifts when the wave ends.
    const float bossFight = g_EnemySpawnState.normaAlive ? 2.0f : 1.0f;
    const float bossStretch = std::max(bossFight,
        SpecialWaveSpawnStretch(wave, difficulty, SpawnSource::Regular));
    const float rangedStretch = std::max(bossFight,
        SpecialWaveSpawnStretch(wave, difficulty, SpawnSource::Ranged));
    g_EnemySpawnState.mobTimer += delta;
    const float spawnInterval = std::max(0.05f,
        2.4f * stats.mobSpawnMult * kEnemySpawnIntervalScale * bossStretch /
        (spawnPressure * TrialSpawnRateMult(score) *
         difficultyParams.spawnRateMultiplier));
    if (g_EnemySpawnState.spawnedThisWave < waveSpawnCap &&
        g_EnemySpawnState.mobTimer >= spawnInterval) {
        const size_t before = monsters.monsters.size();
        monsters.SpawnMob(screenWidth, screenHeight,
                          (int)before + 1, hpMultiplier);
        if (monsters.monsters.size() > before) {
            SpecializeRegularMob(*monsters.monsters.back(), wave,
                                 difficulty, stats.rotorHpMult);
            ++g_EnemySpawnState.spawnedThisWave;
            g_EnemySpawnState.mobTimer -= spawnInterval;
        }
    }

    // Scope uses its own ranged spawn path, gated by its roster entry.
    const float scopeRate = ScopeRateMult(wave, (int)difficulty);
    if (!RosterUnlocked(RosterKind::SCOPE, wave, (int)difficulty) || scopeRate <= 0.0f) {
        g_EnemySpawnState.rangedTimer = 0.0f;
        return;
    }
    g_EnemySpawnState.rangedTimer += delta;
    float rangedInterval = 5.0f * TrialRangedIntervalMult(score) -
        stats.rmobSpawnDelayBonus;
    rangedInterval = std::max(1.0f, rangedInterval) /
        (spawnPressure * difficultyParams.spawnRateMultiplier * scopeRate) *
        kEnemySpawnIntervalScale * 2.5f * rangedStretch;
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
