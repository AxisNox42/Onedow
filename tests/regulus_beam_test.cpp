// Headless regression for Regulus shield lasers (Docs/Astral_Enemy_Blueprints_v3.md
// 3A). Run with scripts/test_regulus_beam.ps1.
#include "MonsterManager.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <set>

static constexpr float DT = 1.0f / 120.0f;

static Monster* Add(MonsterManager& mm, MobKind kind, float x, float y) {
    auto* m = new Monster(x, y);
    m->MakeKind(kind);
    m->speed = 0.0f;
    mm.monsters.push_back(m);
    return m;
}

static void Step(MonsterManager& mm, float seconds) {
    std::vector<Bullet> bullets;
    float hp = 1.0e9f;
    for (float t = 0.0f; t < seconds; t += DT)
        mm.UpdateAll(0.0f, 5000.0f, DT, hp, bullets);   // player far away
}

// The laser is instant: the ally inside 500px is shielded the moment the
// shot fires, the beam is only a short afterglow, and nothing stays linked.
static void CheckInstantGrant() {
    MonsterManager mm;
    auto* reg = Add(mm, MobKind::REGULUS, 0.0f, 0.0f);
    reg->regulusDriftTimer = 1.0e6f;
    auto* inside = Add(mm, MobKind::GRAVIS, 450.0f, 0.0f);
    auto* outside = Add(mm, MobKind::GRAVIS, 560.0f, 0.0f);
    Step(mm, 0.51f);
    assert(inside->regulusShield.acquiredOnce);                 // no travel time
    assert(std::fabs(inside->regulusShield.hp - reg->regulusBaseHp * 0.6f) < 0.01f);
    assert(inside->regulusShield.grantTimer > 0.0f);            // snap-in running
    assert(mm.regulusBeams.size() == 1);
    assert(std::fabs(mm.regulusBeams[0].x1 - 450.0f) < 0.01f);
    Step(mm, Monster::REGULUS_BEAM_TIME + 0.05f);
    assert(mm.regulusBeams.empty());                            // afterglow gone
    assert(inside->regulusShield.grantTimer == 0.0f);
    Step(mm, 3.0f);
    assert(!outside->regulusShield.acquiredOnce);               // beyond 500px
    reg->alive = false;                                         // shield is independent
    Step(mm, 0.5f);
    assert(inside->regulusShield.hp > 0.0f);
    std::printf("instant grant: ok\n");
    mm.Clear();
}

// One laser per second, and each ally is shielded at most once.
static void CheckRateAndOnce() {
    MonsterManager mm;
    auto* reg = Add(mm, MobKind::REGULUS, 0.0f, 0.0f);
    reg->regulusDriftTimer = 1.0e6f;
    std::vector<Monster*> allies;
    for (int i = 0; i < 6; ++i)
        allies.push_back(Add(mm, MobKind::ROTOR, 150.0f * cosf(i * 1.047f),
                             150.0f * sinf(i * 1.047f)));
    Step(mm, 2.6f);                                             // shots at 0.5, 1.5, 2.5
    int shielded = 0;
    for (auto* a : allies) if (a->regulusShield.acquiredOnce) ++shielded;
    assert(shielded == 3);
    Step(mm, 6.0f);
    shielded = 0;
    for (auto* a : allies) if (a->regulusShield.acquiredOnce) ++shielded;
    assert(shielded == 6);
    for (auto* a : allies) a->regulusShield.hp = 0.0f;
    Step(mm, 3.0f);
    for (auto* a : allies) assert(a->regulusShield.hp == 0.0f); // never again
    std::printf("rate and once: ok\n");
    mm.Clear();
}

// Target choice is uniform random, not tier or distance.
static void CheckRandomTarget() {
    std::set<int> firstPicks;
    for (int trial = 0; trial < 40; ++trial) {
        srand(1000 + trial);
        MonsterManager mm;
        auto* reg = Add(mm, MobKind::REGULUS, 0.0f, 0.0f);
        reg->regulusDriftTimer = 1.0e6f;
        Add(mm, MobKind::ROTOR, 100.0f, 0.0f);                  // nearest, lowest tier
        Add(mm, MobKind::GRAVIS, 0.0f, 300.0f);                 // highest tier
        Add(mm, MobKind::GIMBAL, -400.0f, 0.0f);                // farthest
        Step(mm, 0.51f);
        for (int i = 1; i <= 3; ++i)
            if (mm.monsters[i]->regulusShield.acquiredOnce) firstPicks.insert(i);
        mm.Clear();
    }
    assert(firstPicks.size() == 3);
    std::printf("random target: ok\n");
}

int main() {
    CheckInstantGrant();
    CheckRateAndOnce();
    CheckRandomTarget();
    std::printf("regulus beam regression passed\n");
    return 0;
}
