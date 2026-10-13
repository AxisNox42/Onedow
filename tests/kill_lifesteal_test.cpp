// BEH-01 regression: per-kill lifesteal is the full GetLifestealPerKill()
// for every mob and every kill path; the Swarm's 20% reward multiplier
// applies to score only. Run with scripts/test_kill_lifesteal.ps1.
//
// The bullet paths (CollisionSystem::Update, regular mob and Scope) are
// driven here directly. The other paths (common kill collection, laser —
// KillRewards.h) are driven by the REG-01 test; the run script checks that
// every path heals through PlayerStats::ApplyKillLifesteal.
#include "CollisionSystem.h"
#include <cassert>
#include <cmath>
#include <cstdio>

namespace Audio { void PlaySfx(Sfx) {} }   // kill sound: no audio in tests
float g_BaseOrtho[16] = {};

static constexpr float LIFESTEAL = 0.16f;   // 2 stacks x 0.08

static PlayerStats MakeStats() {
    PlayerStats stats;
    stats.lifestealStacks = 2;
    stats.critChance = 0;
    stats.pierce = false;
    stats.ricochetChance = 0;
    assert(std::fabs(stats.GetLifestealPerKill() - LIFESTEAL) < 1e-6f);
    return stats;
}

// One player bullet crossing (x, y) this frame.
static std::vector<Bullet> ShotThrough(float x, float y) {
    Bullet b(x - 12.0f, y, x + 12.0f, y);
    b.prevX = x - 12.0f; b.prevY = y;
    b.x = x + 12.0f;     b.y = y;
    return { b };
}

struct KillResult { float healed; float score; };

static KillResult KillMob(MobKind kind, float startHp) {
    MonsterManager manager;
    Monster* mob = new Monster(500.0f, 300.0f, 1.0f);
    mob->MakeKind(kind);
    mob->hp = 0.01f;
    manager.monsters.push_back(mob);
    PlayerStats stats = MakeStats();
    std::vector<Bullet> bullets = ShotThrough(mob->worldX, mob->worldY);
    float hp = startHp, scoreAccum = 0.0f;
    long long score = 0;
    CollisionSystem::Update(0.0f, 0.0f, manager, bullets, hp, scoreAccum, score, stats);
    assert(!mob->alive && mob->scored);
    return { hp - startHp, scoreAccum };
}

static float KillScope(float startHp) {
    MonsterManager manager;
    RangedMob* scope = new RangedMob(500.0f, 300.0f, 1920, 1080);
    scope->hp = 0.01f;
    manager.rangedMobs.push_back(scope);
    PlayerStats stats = MakeStats();
    std::vector<Bullet> bullets = ShotThrough(scope->worldX, scope->worldY);
    float hp = startHp, scoreAccum = 0.0f;
    long long score = 0;
    CollisionSystem::Update(0.0f, 0.0f, manager, bullets, hp, scoreAccum, score, stats);
    assert(!scope->alive && scope->scored);
    return hp - startHp;
}

int main() {
    const KillResult swarm = KillMob(MobKind::SWARM, 50.0f);
    const KillResult rotor = KillMob(MobKind::ROTOR, 50.0f);
    std::printf("bullet kill heal: swarm %.3f, rotor %.3f (expected %.3f each)\n",
                swarm.healed, rotor.healed, LIFESTEAL);
    std::fflush(stdout);   // keep the numbers if an assert aborts
    assert(std::fabs(rotor.healed - LIFESTEAL) < 1e-4f);
    assert(std::fabs(swarm.healed - LIFESTEAL) < 1e-4f);     // BEH-01: was 20%

    // The Swarm keeps its 20% score multiplier (12 base score).
    float baseXp = 0.0f, baseScore = 0.0f;
    MobKillReward(MobKind::SWARM, baseXp, baseScore);
    assert(std::fabs(swarm.score - baseScore * MobRewardMult(MobKind::SWARM)) < 1e-4f);
    assert(MobRewardMult(MobKind::SWARM) < 1.0f);

    // Healing stops at maxHP.
    const float maxHP = PlayerStats().maxHP;
    assert(std::fabs((maxHP - 0.05f + KillMob(MobKind::SWARM, maxHP - 0.05f).healed) - maxHP) < 1e-4f);

    // Scope bullet kill: full lifesteal (unchanged).
    const float scope = KillScope(50.0f);
    assert(std::fabs(scope - LIFESTEAL) < 1e-4f);

    std::printf("kill lifesteal regression passed\n");
    return 0;
}
