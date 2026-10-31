// Headless regression for the Antares elite: wall ricochet, two-stage split on death
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

// Every rush ricochets: a stage 2+ run that leaves the map reverses the
// component that left and keeps its speed; a stage 1 crawl only slides.
static void CheckWallRicochet() {
    Monster m(960.0f, 300.0f);
    m.MakeKind(MobKind::ANTARES);
    m.antaresInArena = true;
    m.antaresState = Monster::AntaresState::CHASE;
    m.antaresHeadingX = 0.8f; m.antaresHeadingY = 0.6f;
    m.antaresMoveSpeed = 400.0f;
    m.ConfineAntares(0.0f, 0.0f, 1000.0f, 600.0f);   // x past 1000 - radius
    assert(m.antaresHeadingX < 0.0f && m.antaresHeadingY > 0.0f);
    assert(m.antaresMoveSpeed == 400.0f && m.antaresBounces == 1);
    assert(m.antaresState == Monster::AntaresState::CHASE);
    // Left wall flips the other way, a skid ricochets too.
    m.worldX = 10.0f; m.worldY = 300.0f;
    m.antaresState = Monster::AntaresState::SKID;
    m.antaresHeadingX = -0.6f; m.antaresHeadingY = -0.8f;
    m.ConfineAntares(0.0f, 0.0f, 1000.0f, 600.0f);
    assert(m.antaresHeadingX > 0.0f && m.antaresHeadingY < 0.0f && m.antaresBounces == 2);
    // Stage 1 crawl: slides along the edge, no bounce.
    m.worldX = 10.0f;
    m.antaresState = Monster::AntaresState::CHASE;
    m.antaresMoveSpeed = 100.0f; m.antaresHeadingX = -1.0f; m.antaresHeadingY = 0.0f;
    m.ConfineAntares(0.0f, 0.0f, 1000.0f, 600.0f);
    assert(m.antaresHeadingX == -1.0f && m.antaresBounces == 2);
    std::printf("wall ricochet ok\n");
}

// A parked player never gets rammed (ramming off): the rush must stay in the map.
static void CheckRushStaysInside() {
    MonsterManager mm;
    Monster* a = Add(mm, 800.0f, 450.0f);
    a->antaresInArena = true;
    std::vector<Bullet> bullets;
    float hp = 1.0e9f;
    for (float t = 0.0f; t < 60.0f; t += DT) {
        a->singularityGrace = 1.0e9f;
        mm.UpdateAll(100.0f, 100.0f, DT, hp, bullets);
        a->ConfineAntares(0.0f, 0.0f, 1000.0f, 600.0f);
        assert(a->worldX >= 25.0f && a->worldX <= 975.0f);
        assert(a->worldY >= 25.0f && a->worldY <= 575.0f);
    }
    std::printf("rush stays inside ok (bounces %d)\n", a->antaresBounces);
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
    // Second split: killing a child releases gen-2 grandchildren, and those release nothing.
    Monster* child = nullptr;
    for (Monster* m : mm.monsters) if (m != a) { child = m; break; }
    const size_t before = mm.monsters.size();
    child->ApplyDamage(child->hp);
    mm.UpdateAll(5000.0f, 5000.0f, DT, hp, bullets);
    assert(mm.monsters.size() == before + Monster::ANTARES_SPLIT_COUNT);
    Monster* grand = nullptr;
    for (Monster* m : mm.monsters) if (m->antaresGen == 2) { grand = m; break; }
    assert(grand && grand->alive);
    assert(grand->sizeScale < child->sizeScale);
    assert(std::fabs(grand->hp - Monster::ANTARES_BASE_HP * Monster::ANTARES_CHILD_HP * Monster::ANTARES_CHILD_HP) < 0.01f);
    const size_t total = mm.monsters.size();
    grand->ApplyDamage(grand->hp);
    mm.UpdateAll(5000.0f, 5000.0f, DT, hp, bullets);
    assert(mm.monsters.size() == total);   // generation 2 is the last
    // A dead parent is not split again.
    mm.UpdateAll(5000.0f, 5000.0f, DT, hp, bullets);
    assert(mm.monsters.size() == total);
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
    CheckWallRicochet();
    CheckRushStaysInside();
    CheckSplit();
    CheckAntaresBounce();
    std::printf("antares elite regression passed\n");
    return 0;
}
