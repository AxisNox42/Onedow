// Headless regression for the Genesis elite cage: live thrown Swarms become
// the vertices of a polygon around the player that locks and shrinks
// (Docs/superpowers/specs/2026-10-17-genesis-cage-design.md).
// Run with scripts/test_genesis_cage.ps1.
#include "GenesisCage.h"
#include "MonsterManager.h"
#include <cassert>
#include <cmath>
#include <cstdio>

static bool Near(float a, float b, float eps = 1e-3f) { return std::fabs(a - b) < eps; }

static void CheckGeometry() {
    using namespace GenesisCage;
    assert(Near(SlotAngle(0.3f, 2, 5), 0.3f + 2.0f * 6.2831853f / 5.0f));
    assert(Near(DeployBlend(0.0f), 0.0f) && Near(DeployBlend(DEPLOY_TIME * 0.5f), 0.5f));
    assert(Near(DeployBlend(DEPLOY_TIME), 1.0f) && Near(DeployBlend(5.0f), 1.0f));
    assert(Near(LockRadius(0.0f), 240.0f) && Near(LockRadius(0.75f), 120.0f));
    assert(Near(LockRadius(1.5f), 0.0f) && Near(LockRadius(9.0f), 0.0f));
    assert(Near(SegmentDistSq(0, 5, -10, 0, 10, 0), 25.0f));
    assert(Near(SegmentDistSq(20, 0, -10, 0, 10, 0), 100.0f));   // past the end
    assert(Near(SegmentDistSq(3, 4, 0, 0, 0, 0), 25.0f));        // zero-length
    std::printf("geometry ok\n");
}

static constexpr float DT = 1.0f / 120.0f;
static constexpr float PX = 400.0f, PY = 0.0f;   // default player position

static void Step(MonsterManager& mm, float px, float py, float& hp,
                 std::vector<Bullet>& bullets, float seconds) {
    for (float t = 0.0f; t < seconds; t += DT)
        mm.UpdateAll(px, py, DT, hp, bullets);
}

static Monster* AddGenesis(MonsterManager& mm, float x, float y) {
    auto* g = new Monster(x, y);
    g->MakeKind(MobKind::GENESIS);
    mm.monsters.push_back(g);
    return g;
}

static Monster* AddOwnSwarm(MonsterManager& mm, const Monster* g, float x, float y) {
    auto* s = new Monster(x, y, 1.0f);
    s->MakeKind(MobKind::SWARM);
    s->genesisThrown = true;
    s->thrownOwnerId = g->genesisId;
    s->thrownPhase = 0;
    mm.monsters.push_back(s);
    return s;
}

// n own Swarms on a 150px circle around the player.
static void AddRing(MonsterManager& mm, const Monster* g, int n) {
    for (int i = 0; i < n; ++i) {
        const float a = 6.2831853f * (float)i / (float)n;
        AddOwnSwarm(mm, g, PX + cosf(a) * 150.0f, PY + sinf(a) * 150.0f);
    }
}

// Regroup ends on the next step, cooldown ready.
static void ArmCage(Monster* g) {
    g->hivePhase = 3;
    g->spawnTimer = Monster::GENESIS_RECOVER_TIME;
    g->cageCooldown = 0.0f;
}

static int CountCaged(const MonsterManager& mm) {
    int n = 0;
    for (auto* m : mm.monsters)
        if (m->alive && m->thrownPhase == 3) ++n;
    return n;
}

static int CountEdges(const MonsterManager& mm, const Monster* g) {
    int n = 0;
    mm.ForEachGenesisCageEdge(g, [&](float, float, float, float) { ++n; });
    return n;
}

// Steps until the cage leaves `stage` (or ends), at most `limit` seconds.
static void StepWhileStage(MonsterManager& mm, Monster* g, int stage, float px, float py,
                           float& hp, std::vector<Bullet>& bullets, float limit = 4.0f) {
    for (float t = 0.0f; t < limit && g->hivePhase == 4 && g->cageStage == stage; t += DT)
        mm.UpdateAll(px, py, DT, hp, bullets);
}

static void StepWhileCaged(MonsterManager& mm, Monster* g, float px, float py,
                           float& hp, std::vector<Bullet>& bullets, float limit = 4.0f) {
    for (float t = 0.0f; t < limit && g->hivePhase == 4; t += DT)
        mm.UpdateAll(px, py, DT, hp, bullets);
}

static void CheckTriggerNeedsThreeNodes() {
    std::vector<Bullet> bullets;
    float hp = 1000.0f;
    {
        MonsterManager mm;
        Monster* g = AddGenesis(mm, 0.0f, 0.0f);
        AddRing(mm, g, 2);
        ArmCage(g);
        Step(mm, PX, PY, hp, bullets, DT);
        assert(g->hivePhase == 0);
        assert(CountCaged(mm) == 0);
        mm.Clear();
    }
    MonsterManager mm;
    Monster* g = AddGenesis(mm, 0.0f, 0.0f);
    AddRing(mm, g, 3);
    ArmCage(g);
    Step(mm, PX, PY, hp, bullets, DT);
    assert(g->hivePhase == 4 && g->cageStage == 0);
    assert(CountCaged(mm) == 3);
    std::printf("trigger needs three nodes ok\n");
    mm.Clear();
}

static void CheckCooldownBlocksTrigger() {
    MonsterManager mm;
    std::vector<Bullet> bullets;
    float hp = 1000.0f;
    Monster* g = AddGenesis(mm, 0.0f, 0.0f);
    assert(Near(g->cageCooldown, GenesisCage::COOLDOWN));
    AddRing(mm, g, 3);
    ArmCage(g);
    g->cageCooldown = 5.0f;
    Step(mm, PX, PY, hp, bullets, DT);
    assert(g->hivePhase == 0);
    assert(CountCaged(mm) == 0);
    std::printf("cooldown blocks trigger ok\n");
    mm.Clear();
}

static void CheckRecruitRangeAndCap() {
    MonsterManager mm;
    std::vector<Bullet> bullets;
    float hp = 1000.0f;
    Monster* g = AddGenesis(mm, 0.0f, 0.0f);
    AddRing(mm, g, 8);
    Monster* farSwarm = AddOwnSwarm(mm, g, PX + 900.0f, PY);
    Monster* flying = AddOwnSwarm(mm, g, PX, PY + 300.0f);
    flying->BeginThrownFlight(PX + 1000.0f, PY + 800.0f);
    ArmCage(g);
    Step(mm, PX, PY, hp, bullets, DT);
    assert(g->hivePhase == 4);
    assert(g->cageCount == GenesisCage::MAX_NODES);
    assert(CountCaged(mm) == GenesisCage::MAX_NODES);
    assert(farSwarm->thrownPhase != 3 && flying->thrownPhase != 3);
    std::printf("recruit range and cap ok\n");
    mm.Clear();
}

static void CheckDeployReachesRing() {
    MonsterManager mm;
    std::vector<Bullet> bullets;
    float hp = 1000.0f;
    Monster* g = AddGenesis(mm, 0.0f, 0.0f);
    AddRing(mm, g, 4);
    ArmCage(g);
    Step(mm, PX, PY, hp, bullets, DT);
    assert(g->hivePhase == 4);
    // The player walks away; the cage center stays where it was cast.
    StepWhileStage(mm, g, 0, 700.0f, 0.0f, hp, bullets);
    StepWhileStage(mm, g, 1, 700.0f, 0.0f, hp, bullets);
    assert(g->hivePhase == 4 && g->cageStage == 2);
    assert(Near(g->cageCX, PX) && Near(g->cageCY, PY));
    int seen = 0;
    for (auto* m : mm.monsters) {
        if (!m->alive || m->thrownPhase != 3) continue;
        const float d = std::sqrt((m->worldX - PX) * (m->worldX - PX) +
                                  (m->worldY - PY) * (m->worldY - PY));
        assert(std::fabs(d - GenesisCage::RADIUS) < 1.5f);
        ++seen;
    }
    assert(seen == 4);
    std::printf("deploy reaches ring ok\n");
    mm.Clear();
}

static void CheckLockHitsOnceAndEnds() {
    MonsterManager mm;
    std::vector<Bullet> bullets;
    float hp = 100.0f;
    Monster* g = AddGenesis(mm, 0.0f, 0.0f);
    AddRing(mm, g, 4);
    ArmCage(g);
    Step(mm, PX, PY, hp, bullets, DT);
    assert(g->hivePhase == 4);
    StepWhileCaged(mm, g, PX, PY, hp, bullets);
    assert(Near(hp, 100.0f - GenesisCage::EDGE_DAMAGE));
    assert(g->hivePhase == 0);
    assert(std::fabs(g->cageCooldown - GenesisCage::COOLDOWN) <= DT + 1e-4f);
    assert(CountCaged(mm) == 0);
    std::printf("lock hits once and ends ok\n");
    mm.Clear();
}

static void CheckEscapeOutsideNoDamage() {
    MonsterManager mm;
    std::vector<Bullet> bullets;
    float hp = 100.0f;
    Monster* g = AddGenesis(mm, 0.0f, 0.0f);
    AddRing(mm, g, 4);
    ArmCage(g);
    Step(mm, PX, PY, hp, bullets, DT);
    StepWhileStage(mm, g, 0, PX, PY, hp, bullets);
    assert(g->cageStage == 1);
    StepWhileCaged(mm, g, 1000.0f, 0.0f, hp, bullets);
    assert(Near(hp, 100.0f));
    assert(g->hivePhase == 0);
    assert(CountCaged(mm) == 0);
    std::printf("escape outside no damage ok\n");
    mm.Clear();
}

static Monster* CagedInSlot(MonsterManager& mm, const Monster* g, int slot) {
    for (auto* m : mm.monsters)
        if (m->alive && m->thrownPhase == 3 && m->thrownOwnerId == g->genesisId &&
            m->cageSlot == slot) return m;
    return nullptr;
}

static void CheckKilledNodeOpensGap() {
    MonsterManager mm;
    std::vector<Bullet> bullets;
    float hp = 1000.0f;
    Monster* g = AddGenesis(mm, 0.0f, 0.0f);
    AddRing(mm, g, 3);
    ArmCage(g);
    Step(mm, PX, PY, hp, bullets, DT);
    StepWhileStage(mm, g, 0, PX, PY, hp, bullets);
    assert(g->cageStage == 1);
    assert(CountEdges(mm, g) == 3);
    assert(mm.GenesisCageNode(g, 1) == CagedInSlot(mm, g, 1));
    CagedInSlot(mm, g, 1)->alive = false;
    assert(mm.GenesisCageNode(g, 1) == nullptr);
    assert(CountEdges(mm, g) == 1);
    StepWhileStage(mm, g, 1, 1000.0f, 0.0f, hp, bullets);
    assert(g->hivePhase == 4 && g->cageStage == 2);
    assert(CountEdges(mm, g) == 1);
    CagedInSlot(mm, g, 2)->alive = false;
    Step(mm, 1000.0f, 0.0f, hp, bullets, DT);
    assert(g->hivePhase == 0);
    assert(CountCaged(mm) == 0);
    std::printf("killed node opens gap ok\n");
    mm.Clear();
}

static void CheckGenesisDeathReleasesNodes() {
    MonsterManager mm;
    std::vector<Bullet> bullets;
    float hp = 1000.0f;
    Monster* g = AddGenesis(mm, 0.0f, 0.0f);
    AddRing(mm, g, 3);
    ArmCage(g);
    Step(mm, PX, PY, hp, bullets, DT);
    StepWhileStage(mm, g, 0, PX, PY, hp, bullets);
    assert(CountCaged(mm) == 3);
    g->alive = false;
    Step(mm, PX, PY, hp, bullets, DT);
    assert(CountCaged(mm) == 0);
    std::printf("genesis death releases nodes ok\n");
    mm.Clear();
}

static void CheckTwoGenesisKeepTheirOwnNodes() {
    MonsterManager mm;
    std::vector<Bullet> bullets;
    float hp = 1000.0f;
    Monster* a = AddGenesis(mm, 0.0f, 0.0f);
    Monster* b = AddGenesis(mm, 800.0f, 0.0f);
    for (int i = 0; i < 3; ++i) {
        const float ang = 6.2831853f * (float)i / 6.0f;
        AddOwnSwarm(mm, a, PX + cosf(ang) * 150.0f, PY + sinf(ang) * 150.0f);
        const float bng = ang + 6.2831853f / 6.0f * 3.0f;
        AddOwnSwarm(mm, b, PX + cosf(bng) * 150.0f, PY + sinf(bng) * 150.0f);
    }
    ArmCage(a);
    ArmCage(b);
    Step(mm, PX, PY, hp, bullets, DT);
    assert(a->hivePhase == 4 && b->hivePhase == 4);
    assert(a->cageCount == 3 && b->cageCount == 3);
    StepWhileStage(mm, a, 0, PX, PY, hp, bullets);
    StepWhileStage(mm, a, 1, PX, PY, hp, bullets);
    auto isOwnVertex = [&](const Monster* g, float x, float y) {
        for (auto* m : mm.monsters)
            if (m->alive && m->thrownPhase == 3 && m->thrownOwnerId == g->genesisId &&
                Near(m->worldX, x) && Near(m->worldY, y)) return true;
        return false;
    };
    for (const Monster* g : { a, b }) {
        int edges = 0;
        mm.ForEachGenesisCageEdge(g, [&](float ax, float ay, float bx, float by) {
            assert(isOwnVertex(g, ax, ay) && isOwnVertex(g, bx, by));
            ++edges;
        });
        assert(edges == 3);
    }
    std::printf("two genesis keep their own nodes ok\n");
    mm.Clear();
}

int main() {
    srand(11);
    CheckGeometry();
    CheckTriggerNeedsThreeNodes();
    CheckCooldownBlocksTrigger();
    CheckRecruitRangeAndCap();
    CheckDeployReachesRing();
    CheckLockHitsOnceAndEnds();
    CheckEscapeOutsideNoDamage();
    CheckKilledNodeOpensGap();
    CheckGenesisDeathReleasesNodes();
    CheckTwoGenesisKeepTheirOwnNodes();
    std::printf("genesis cage regression passed\n");
    return 0;
}
