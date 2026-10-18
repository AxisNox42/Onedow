// BEH-02 regression: a thrown Genesis Swarm's kill credits the combo and the
// codex only — no score, XP/stardust, lifesteal, run kill count (warlord),
// vampire streak, or 무한 세례 kill (DEC-04). A regular Swarm kill is the
// control. Run with scripts/test_thrown_swarm_kill.ps1.
//
// The bullet path (CollisionSystem::Update) is driven here directly. The
// other paths (common kill collection, laser — KillRewards.h) are driven by
// the REG-01 test; the run script checks they call CreditThrownSwarmKill.
#include "CollisionSystem.h"
#include <cassert>
#include <cmath>
#include <cstdio>

namespace Audio { void PlaySfx(Sfx) {} }   // kill sound: no audio in tests
float g_BaseOrtho[16] = {};

struct Snapshot {
    float hp, score, rain;
    long long combo, killCount, vampireStreak, codex;
    size_t stardust;
};

static Snapshot Take(float hp, float score, const PlayerStats& stats) {
    return { hp, score, g_RainKillAccum, g_Combo, stats.killCount,
             stats.vampireKillStreak, g_RunMobKillCounts[CM_SWARM],
             g_StardustPickups.size() };
}

static void Print(const char* label, const Snapshot& a, const Snapshot& b) {
    std::printf("%s: hp %+.3f score %+.1f rain %+.0f combo %+lld kills %+lld "
                "vampire %+lld codex %+lld stardust %+d\n", label,
                b.hp - a.hp, b.score - a.score, b.rain - a.rain, b.combo - a.combo,
                b.killCount - a.killCount, b.vampireStreak - a.vampireStreak,
                b.codex - a.codex, (int)(b.stardust - a.stardust));
    std::fflush(stdout);   // keep the numbers if an assert aborts
}

// Kills one Swarm with a bullet and returns the before/after snapshots.
static void BulletKill(bool thrown, Snapshot& before, Snapshot& after, MobKind kind=MobKind::SWARM) {
    MonsterManager manager;
    Monster* mob = new Monster(500.0f, 300.0f, 1.0f);
    mob->MakeKind(kind);
    mob->genesisThrown = thrown;
    mob->hp = 0.01f;
    manager.monsters.push_back(mob);

    PlayerStats stats;
    stats.lifestealStacks = 2;
    stats.vampire = true;
    stats.critChance = 0;
    stats.pierce = false;
    stats.ricochetChance = 0;

    Bullet b(mob->worldX - 12.0f, mob->worldY, mob->worldX + 12.0f, mob->worldY);
    b.prevX = mob->worldX - 12.0f; b.prevY = mob->worldY;
    b.x = mob->worldX + 12.0f;     b.y = mob->worldY;
    std::vector<Bullet> bullets = { b };

    float hp = 50.0f, scoreAccum = 0.0f;
    long long score = 0;
    before = Take(hp, scoreAccum, stats);
    CollisionSystem::Update(0.0f, 0.0f, manager, bullets, hp, scoreAccum, score, stats);
    assert(!mob->alive && mob->scored);
    after = Take(hp, scoreAccum, stats);
}

int main() {
    Snapshot a, b;

    // Control: a regular Swarm pays everything.
    BulletKill(false, a, b);
    Print("regular swarm", a, b);
    assert(b.combo == a.combo + 1 && b.codex == a.codex + 1);
    assert(b.score > a.score && b.hp > a.hp && b.rain == a.rain + 1.0f);
    assert(b.killCount == a.killCount + 1 && b.vampireStreak == a.vampireStreak + 1);
    assert(b.stardust > a.stardust);

    // Thrown Swarm: combo and codex only.
    BulletKill(true, a, b);
    Print("thrown swarm ", a, b);
    assert(b.combo == a.combo + 1);
    assert(b.codex == a.codex + 1);
    assert(b.score == a.score);
    assert(b.hp == a.hp);
    assert(b.killCount == a.killCount);
    assert(b.vampireStreak == a.vampireStreak);
    assert(b.stardust == a.stardust);
    assert(b.rain == a.rain);              // DEC-04: no 무한 세례 kill

    const auto rotorKills=g_RunMobKillCounts[CM_ROTOR];
    BulletKill(true,a,b,MobKind::ROTOR);
    assert(b.combo==a.combo+1 && g_RunMobKillCounts[CM_ROTOR]==rotorKills+1);
    assert(b.score==a.score && b.hp==a.hp && b.killCount==a.killCount);
    assert(b.vampireStreak==a.vampireStreak && b.stardust==a.stardust && b.rain==a.rain);
    std::printf("launched Rotor: combo and Rotor codex only, no farm rewards: ok\n");
    std::printf("thrown swarm kill regression passed\n");
    return 0;
}
