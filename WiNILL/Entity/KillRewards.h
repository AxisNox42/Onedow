#pragma once
// Weapon-specific timing lives at the caller. The shared reward rules and
// scored guard live in CollisionSystem.h for bullet, laser and death collection.
#include "CollisionSystem.h"

inline void CreditLaserMonsterKill(KillRewardSink& rewards, Monster& monster) {
    if (!monster.alive) CollectMonsterKill(rewards, monster);
}

inline void CreditLaserScopeKill(KillRewardSink& rewards, RangedMob& scope) {
    if (!scope.alive) CollectScopeKill(rewards, scope);
}
