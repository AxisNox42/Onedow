#pragma once

class Monster;
class MonsterManager;
struct PlayerStats;
enum class Difficulty;
enum class MobKind;

void ResetEnemySpawnState();
void UpdateEnemySpawns(MonsterManager& monsters, const PlayerStats& stats,
                       int screenWidth, int screenHeight,
                       float delta, float gameTime, long long score,
                       Difficulty difficulty);

// The scheduled enemy of a fifth wave. Its kind is rolled when the run
// starts, so the wave HUD can name it before it enters.
struct ScheduledEliteInfo {
    bool scheduled = false;   // wave has a scheduled spawn at all
    MobKind kind{};
    bool spawned = false;     // entered this run
    bool alive = false;       // still on the field
};
ScheduledEliteInfo GetScheduledElite(int wave, Difficulty difficulty);

// NORMA, the boss of the final wave, has entered and then fallen this run.
bool NormaDefeated();
// Hand testing: brings NORMA in now, as the boss wave would.
void DebugSpawnNorma(MonsterManager& monsters, int screenWidth, float hpMultiplier);
