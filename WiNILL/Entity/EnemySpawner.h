#pragma once

class Monster;
class MonsterManager;
struct PlayerStats;

void ResetEnemySpawnState();
void UpdateEnemySpawns(MonsterManager& monsters, const PlayerStats& stats,
                       int screenWidth, int screenHeight,
                       float delta, float gameTime, long long score);
