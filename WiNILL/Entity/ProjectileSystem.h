#pragma once

#include <vector>

class Bullet;
class MonsterManager;
class SpatialBounds;

void UpdateProjectiles(std::vector<Bullet>& bullets,
                       const MonsterManager& monsters,
                       const SpatialBounds& playerBounds,
                       float fixedDelta, float timeStopTimer,
                       float hyperFocusTimer,
                       float screenWidth, float screenHeight,
                       float viewZoom);
