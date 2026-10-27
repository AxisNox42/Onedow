// Headless regression for the Antares elite: ricochet charge, split on death
// and the Antares-vs-Antares bounce. Run with scripts/test_antares_elite.ps1.
#include "MonsterManager.h"
#include <cassert>
#include <cmath>
#include <cstdio>

static constexpr float DT = 1.0f / 120.0f;

static Monster* Add(MonsterManager& mm, float x, float y) {
    auto* m = new Monster(x, y);
    m->MakeKind(MobKind::ANTARES);
    mm.monsters.push_back(m);
    return m;
}

static void CheckEliteSet() {
    assert(IsEliteMob(MobKind::ANTARES));
    Monster m(0.0f, 0.0f);
    m.MakeKind(MobKind::ANTARES);
    assert(std::fabs(m.hp - Monster::ANTARES_BASE_HP * Monster::ANTARES_ELITE_HP_MULT) < 0.01f);
    assert(m.antaresGen == 0 && !m.antaresSplit);
    std::printf("elite set ok\n");
}

static float PathLength(const Monster& m) {
    float len = 0.0f;
    for (int i = 0; i + 1 < m.antaresPathCount; ++i)
        len += glm::length(m.antaresPath[i + 1] - m.antaresPath[i]);
    return len;
}

static void CheckChargePath() {
    Monster m(500.0f, 300.0f);
    m.MakeKind(MobKind::ANTARES);
    m.antaresBounds = glm::vec4(50.0f, 50.0f, 950.0f, 550.0f);
    m.antaresHeadingX = 1.0f; m.antaresHeadingY = 0.0f;
    m.PlanAntaresCharge();
    // Right wall after 450px, then back across 900px to the left wall (1350px);
    // the last 550px end mid-field: start, two edge points, end.
    assert(m.antaresPathCount == 4);
    assert(std::fabs(m.antaresPath[2].x - 50.0f) < 0.01f);
    assert(std::fabs(m.antaresPath[1].x - 950.0f) < 0.01f);
    assert(std::fabs(PathLength(m) - Monster::ANTARES_CHARGE_LENGTH) < 0.5f);
    // Diagonal: the corner reflection flips only the component that left.
    m.antaresHeadingX = 0.6f; m.antaresHeadingY = 0.8f;
    m.PlanAntaresCharge();
    assert(m.antaresPathCount >= 3);
    assert(std::fabs(m.antaresPath[1].y - 550.0f) < 0.01f);
    assert(m.antaresPath[2].x > m.antaresPath[1].x);   // x kept going, y reversed
    assert(std::fabs(PathLength(m) - Monster::ANTARES_CHARGE_LENGTH) < 0.5f);
    // No bounds yet: one straight segment.
    m.antaresBounds = glm::vec4(0.0f);
    m.PlanAntaresCharge();
    assert(m.antaresPathCount == 2);
    std::printf("charge path ok\n");
}

// Chases the parked player (ramming off, so the run is never cut short);
// every second run must be an aimed ricochet charge.
static void CheckChargeCycle() {
    MonsterManager mm;
    Monster* a = Add(mm, 800.0f, 450.0f);
    a->antaresInArena = true;
    std::vector<Bullet> bullets;
    float hp = 1.0e9f;
    bool aimed = false, charged = false;
    int maxBounces = 0;
    for (float t = 0.0f; t < 60.0f; t += DT) {
        a->singularityGrace = 1.0e9f;   // the charge passes through instead of ramming
        mm.UpdateAll(100.0f, 100.0f, DT, hp, bullets);
        a->ConfineAntares(0.0f, 0.0f, 1000.0f, 600.0f);
        if (a->antaresState == Monster::AntaresState::AIM) {
            aimed = true;
            assert(a->antaresPathCount >= 2);
            assert(a->antaresMoveSpeed == 0.0f);
        }
        if (a->antaresState == Monster::AntaresState::CHARGE) {
            charged = true;
            assert(std::fabs(a->antaresMoveSpeed - Monster::ANTARES_CHARGE_SPEED) < 0.01f);
            maxBounces = std::max(maxBounces, a->antaresBounces);
            // Always inside the map (reflected, never carried out).
            assert(a->worldX >= 25.0f && a->worldX <= 975.0f);
            assert(a->worldY >= 25.0f && a->worldY <= 575.0f);
        }
    }
    assert(aimed && charged);
    assert(maxBounces >= 1);
    std::printf("charge cycle ok (max bounces %d)\n", maxBounces);
}

static void CheckSplit() {
    MonsterManager mm;
    Monster* a = Add(mm, 300.0f, 300.0f);
    a->antaresInArena = true;
    a->antaresHeadingX = 1.0f;
    a->ApplyDamage(a->hp);
    assert(!a->alive);
    std::vector<Bullet> bullets;
    float hp = 1.0e9f;
    mm.UpdateAll(5000.0f, 5000.0f, DT, hp, bullets);
    int children = 0;
    for (Monster* m : mm.monsters) {
        if (m == a) continue;
        ++children;
        assert(m->kind == MobKind::ANTARES && m->antaresGen == 1 && m->alive);
        assert(m->sizeScale < a->sizeScale);
        assert(std::fabs(m->hp - Monster::ANTARES_BASE_HP * Monster::ANTARES_CHILD_HP) < 0.01f);
        assert(m->antaresMoveSpeed > 0.0f);
    }
    assert(children == Monster::ANTARES_SPLIT_COUNT);
    // Once only: the parent is not split again, and a child's death releases nothing.
    mm.UpdateAll(5000.0f, 5000.0f, DT, hp, bullets);
    assert((int)mm.monsters.size() == 1 + Monster::ANTARES_SPLIT_COUNT);
    for (Monster* m : mm.monsters)
        if (m != a) { m->ApplyDamage(m->hp); break; }
    mm.UpdateAll(5000.0f, 5000.0f, DT, hp, bullets);
    assert((int)mm.monsters.size() == 1 + Monster::ANTARES_SPLIT_COUNT);
    // A silent clear (noBlast) splits nothing.
    MonsterManager quiet;
    Monster* q = Add(quiet, 300.0f, 300.0f);
    q->alive = false; q->noBlast = true; q->scored = true;
    quiet.UpdateAll(5000.0f, 5000.0f, DT, hp, bullets);
    for (Monster* m : quiet.monsters) assert(m->antaresGen == 0);
    std::printf("split ok\n");
}

static void CheckAntaresBounce() {
    MonsterManager mm;
    Monster* a = Add(mm, 0.0f, 0.0f);
    Monster* b = Add(mm, 60.0f, 0.0f);   // reach is 83px, so they overlap
    for (Monster* m : {a, b}) m->antaresInArena = true;
    a->antaresHeadingX = 1.0f; a->antaresMoveSpeed = 300.0f;
    b->antaresHeadingX = -1.0f; b->antaresMoveSpeed = 300.0f;
    mm.ResolveAntaresCollisions();
    // Head-on, equal masses: they swap directions and leave 20% faster.
    assert(a->antaresHeadingX < 0.0f && b->antaresHeadingX > 0.0f);
    assert(std::fabs(a->antaresMoveSpeed - 360.0f) < 0.5f);
    assert(std::fabs(b->antaresMoveSpeed - 360.0f) < 0.5f);
    assert(b->worldX - a->worldX >= 83.0f - 0.01f);   // pushed apart
    // Cooldown: the same overlap does not bounce twice.
    const float speed = a->antaresMoveSpeed;
    mm.ResolveAntaresCollisions();
    assert(a->antaresMoveSpeed == speed);
    // The speed stays under the Antares cap.
    Monster* c = Add(mm, 500.0f, 0.0f);
    Monster* d = Add(mm, 560.0f, 0.0f);
    for (Monster* m : {c, d}) m->antaresInArena = true;
    c->antaresHeadingX = 1.0f; c->antaresMoveSpeed = Monster::ANTARES_MAX_SPEED;
    d->antaresHeadingX = -1.0f; d->antaresMoveSpeed = Monster::ANTARES_MAX_SPEED;
    mm.ResolveAntaresCollisions();
    assert(c->antaresMoveSpeed <= Monster::ANTARES_MAX_SPEED);
    // A resting Antares hit by a runner is knocked into a run of its own.
    Monster* e = Add(mm, 1000.0f, 0.0f);
    Monster* f = Add(mm, 1060.0f, 0.0f);
    for (Monster* m : {e, f}) m->antaresInArena = true;
    e->antaresHeadingX = 1.0f; e->antaresMoveSpeed = 400.0f;
    f->antaresState = Monster::AntaresState::REST; f->antaresMoveSpeed = 0.0f;
    mm.ResolveAntaresCollisions();
    assert(f->antaresState == Monster::AntaresState::CHASE && f->antaresMoveSpeed >= 400.0f);
    assert(f->antaresHeadingX > 0.0f);
    std::printf("antares bounce ok\n");
}

int main() {
    CheckEliteSet();
    CheckChargePath();
    CheckChargeCycle();
    CheckSplit();
    CheckAntaresBounce();
    std::printf("antares elite regression passed\n");
    return 0;
}
