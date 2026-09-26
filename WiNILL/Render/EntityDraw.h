#pragma once
#include <vector>
#include "Bullet.h"
#include "Monster.h"

class RangedMob;

void drawBullet(const Bullet& b);
void drawMob(const Monster* m);
void drawRangedMob(const RangedMob* r);
// Render an Astral Log entity preview with the same silhouette as gameplay.
void drawCodexMobPreview(int codexId, float x, float y, float scale = 3.0f);
void DrawEnemySightRear(float x, float y, float foregroundRadius,
                        float alpha = 0.7f);
struct EnemySightRearMarker {
    float x;
    float y;
    float foregroundRadius;
};
void DrawEnemySightRearBatch(const std::vector<EnemySightRearMarker>& markers,
                             float alpha = 0.7f);
void BeginEnemySightFrontBatch();
void QueueMonsterSightFront(const Monster* m);
void QueueRangedMobSightFront(const RangedMob* r);
void FlushEnemySightFrontBatch();
void DrawMonsterSightRear(const Monster* m);
void DrawRangedMobSightRear(const RangedMob* r);

// 창별 컬링 — scissor 패스에서 창 밖 엔티티 draw call 생략
bool inWin(float x, float y, float rx, float ry, float rw, float rh,
           float margin = 48.0f);

inline constexpr float WIN_TB = 22.0f;   // 가짜 창 타이틀바 고정 높이
void DrawAppWindow(float wx, float wy, float w, float h, const wchar_t* title,
                   float tb = WIN_TB);
void DrawApproachOrb(float x, float y);
