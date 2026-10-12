// Headless regression for the Genesis v3 launcher and thrown Swarms
// (Docs/Astral_Enemy_Blueprints_v3.md 2.2-2.5). Run with
// scripts/test_genesis_launcher.ps1.
#include "MonsterManager.h"
#include <cassert>
#include <cmath>
#include <cstdio>

static constexpr float DT = 1.0f / 120.0f;

static int CountThrown(const MonsterManager& mm) {
    int n = 0;
    for (auto* m : mm.monsters)
        if (m->alive && m->genesisThrown) ++n;
    return n;
}

static void Step(MonsterManager& mm, float px, float py, float& hp,
                 std::vector<Bullet>& bullets, float seconds) {
    for (float t = 0.0f; t < seconds; t += DT)
        mm.UpdateAll(px, py, DT, hp, bullets);
}

// Genesis tracks the player, locks the aim, throws 2-3 Swarms, recovers.
static void CheckLauncherCycle() {
    MonsterManager mm;
    std::vector<Bullet> bullets;
    float hp = 1000.0f;
    auto* g = new Monster(0.0f, 0.0f);
    g->MakeKind(MobKind::GENESIS);
    g->hiveOrbitAngle = 2.5f;                    // start facing away
    mm.monsters.push_back(g);
    const float px = 600.0f, py = 900.0f;        // well off the fan lines
    Step(mm, px, py, hp, bullets, Monster::GENESIS_TRACK_TIME - 0.05f);
    assert(g->hivePhase == 0);
    const float want = atan2f(py - g->worldY, px - g->worldX);
    float err = g->hiveOrbitAngle - want;
    while (err > 3.14159265f) err -= 6.28318531f;
    while (err < -3.14159265f) err += 6.28318531f;
    assert(std::fabs(err) < 0.05f);              // tracked the player

    Step(mm, px, py, hp, bullets, 0.1f);
    assert(g->hivePhase == 1);                   // producing, aim locked
    const float locked = g->genesisAimAngle;
    assert(g->GenesisStacks() == Monster::GENESIS_STACKS);
    assert(CountThrown(mm) == 0);

    Step(mm, px, py, hp, bullets, Monster::GENESIS_AIM_TIME + 0.60f);
    assert(CountThrown(mm) == 3);                // always all three shells
    assert(g->genesisAimAngle == locked);        // aim never moved mid-volley
    Step(mm, px, py, hp, bullets, 0.4f);
    assert(g->hivePhase == 0);                   // recovered within the 6s cycle
    std::printf("launcher cycle: volley=3 ok\n");
    mm.Clear();
}

// A thrown Swarm that reaches the player deals 10 once and vanishes with no
// reward.
static void CheckFlightHit() {
    float hp = 100.0f;
    Monster s(0.0f, 0.0f);
    s.MakeKind(MobKind::SWARM);
    s.BeginThrownFlight(300.0f, 0.0f);
    for (int i = 0; i < 240 && s.alive; ++i)
        s.Update(300.0f, 0.0f, DT, hp);
    assert(!s.alive);
    assert(s.scored && s.noBlast);               // no kill reward, no chain blast
    assert(std::fabs(hp - (100.0f - Monster::THROWN_SWARM_DAMAGE)) < 0.001f);
    std::printf("flight hit: hp=%.1f ok\n", hp);
}

// Missing the player: land on target, stun 0.6s without contact damage, then
// chase like an ordinary Swarm.
static void CheckLandingStun() {
    float hp = 100.0f;
    Monster s(0.0f, 0.0f);
    s.MakeKind(MobKind::SWARM);
    s.BeginThrownFlight(300.0f, 0.0f);
    float t = 0.0f;
    while (s.thrownPhase == 1) {
        s.Update(0.0f, 2000.0f, DT, hp);
        t += DT;
        assert(t < 1.0f);
    }
    assert(s.thrownPhase == 2);
    assert(std::fabs(s.worldX - 300.0f) < 0.01f && std::fabs(s.worldY) < 0.01f);

    // Player standing on top of the stunned Swarm takes nothing.
    for (float w = 0.0f; w < Monster::THROWN_SWARM_STUN - 0.05f; w += DT)
        s.Update(300.0f, 4.0f, DT, hp);
    assert(s.thrownPhase == 2);
    assert(hp == 100.0f);
    assert(std::fabs(s.worldX - 300.0f) < 0.01f);  // held still

    for (float w = 0.0f; w < 0.1f; w += DT)
        s.Update(300.0f, 400.0f, DT, hp);
    assert(s.thrownPhase == 0);
    const float before = s.worldY;
    for (float w = 0.0f; w < 0.2f; w += DT)
        s.Update(300.0f, 400.0f, DT, hp);
    assert(s.worldY > before + 10.0f);           // chasing again
    std::printf("landing stun: ok\n");
}

// Flight ignores enemy crowding: the Swarm stays on its straight line while
// passing through a heavier enemy.
static void CheckFlightPassesThrough() {
    MonsterManager mm;
    std::vector<Bullet> bullets;
    float hp = 1000.0f;
    auto* wall = new Monster(150.0f, 0.0f);
    wall->MakeKind(MobKind::GRAVIS);
    wall->speed = 0.0f;
    mm.monsters.push_back(wall);
    auto* s = new Monster(0.0f, 0.0f);
    s->MakeKind(MobKind::SWARM);
    s->BeginThrownFlight(300.0f, 0.0f);
    mm.monsters.push_back(s);
    for (int i = 0; i < 40 && s->thrownPhase == 1; ++i) {
        mm.UpdateAll(0.0f, 3000.0f, DT, hp, bullets);
        if (s->thrownPhase == 1) assert(std::fabs(s->worldY) < 0.001f);
    }
    std::printf("flight pass-through: ok\n");
    mm.Clear();
}

// Each Genesis caps its own living thrown Swarms at 15: a full Genesis
// stops throwing while another Genesis keeps throwing.
static void CheckThrownCap() {
    MonsterManager mm;
    std::vector<Bullet> bullets;
    float hp = 1.0e9f;
    auto* full = new Monster(0.0f, 0.0f);
    full->MakeKind(MobKind::GENESIS);
    auto* other = new Monster(300.0f, 0.0f);
    other->MakeKind(MobKind::GENESIS);
    assert(full->genesisId != other->genesisId);
    mm.monsters.push_back(full);
    mm.monsters.push_back(other);
    for (int i = 0; i < Monster::GENESIS_THROWN_CAP; ++i) {
        auto* s = new Monster(-3000.0f, (float)i * 40.0f);
        s->MakeKind(MobKind::SWARM);
        s->genesisThrown = true;
        s->thrownOwnerId = full->genesisId;
        s->speed = 0.0f;
        mm.monsters.push_back(s);
    }
    auto owned = [&](const Monster* g) {
        int n = 0;
        for (auto* m : mm.monsters)
            if (m->alive && m->genesisThrown && m->thrownOwnerId == g->genesisId) ++n;
        return n;
    };
    Step(mm, 600.0f, 900.0f, hp, bullets, 6.2f);   // volley out, none landed yet
    assert(Monster::GENESIS_THROWN_CAP == 15);
    assert(owned(full) == 15);                   // capped: no new throws
    assert(owned(other) == 3);                   // unaffected by the other's cap
    std::printf("thrown cap: per Genesis 15 ok\n");
    mm.Clear();
}

// No range limit: a far player still gets targets within the scatter radius.
static void CheckThrowRange() {
    MonsterManager mm;
    std::vector<Bullet> bullets;
    float hp = 1000.0f;
    auto* g = new Monster(0.0f, 0.0f);
    g->MakeKind(MobKind::GENESIS);
    mm.monsters.push_back(g);
    Step(mm, 2500.0f, 0.0f, hp, bullets, Monster::GENESIS_TRACK_TIME + 0.05f);
    assert(g->hivePhase == 1);
    const float r = Monster::GENESIS_SCATTER_RADIUS;
    assert(r == 110.0f);
    for (int i = 0; i < 3; ++i) {
        const float dx = g->genesisShotX[i] - 2500.0f, dy = g->genesisShotY[i];
        assert(dx * dx + dy * dy <= r * r + 1.0f);
    }
    std::printf("no range limit: target (%.0f, %.0f) ok\n",
                g->genesisShotX[0], g->genesisShotY[0]);
    mm.Clear();
}

// Shells lock in one per GENESIS_STACK_TIME while tracking; the aim starts
// only at 3/3, each throw empties one shell, and regrouping starts empty.
static void CheckShellStacks() {
    MonsterManager mm;
    std::vector<Bullet> bullets;
    float hp = 1000.0f;
    auto* g = new Monster(0.0f, 0.0f);
    g->MakeKind(MobKind::GENESIS);
    mm.monsters.push_back(g);
    const float px = 600.0f, py = 900.0f;
    for (int n = 0; n < 3; ++n) {
        Step(mm, px, py, hp, bullets, Monster::GENESIS_STACK_TIME * 0.5f);
        assert(g->hivePhase == 0 && g->GenesisStacks() == n);
        Step(mm, px, py, hp, bullets, Monster::GENESIS_STACK_TIME * 0.5f);
    }
    Step(mm, px, py, hp, bullets, 2.0f * DT);
    assert(g->hivePhase == 1 && g->GenesisStacks() == 3);   // aims only when full
    const float turn = 6.2831853f;
    float off = fmodf(g->genesisShellSpin, turn);
    if (off > turn * 0.5f) off -= turn;
    assert(std::fabs(off) < 1e-3f);                          // the muzzle gap faces the aim
    int last = 3;
    while (g->hivePhase != 3) {
        Step(mm, px, py, hp, bullets, DT);
        const int s = g->GenesisStacks();
        if (g->hivePhase == 2) assert(s == 3 - g->hiveSpawnCount);
        assert(s <= last);                                    // never refills mid-volley
        last = s;
    }
    assert(g->GenesisStacks() == 0 && CountThrown(mm) == 3);
    while (g->hivePhase != 0) Step(mm, px, py, hp, bullets, DT);
    assert(g->GenesisStacks() == 0);                         // charging restarts empty
    std::printf("shell stacks: 0->3, aim at 3/3, one per throw ok\n");
    mm.Clear();
}

// Every volley sends its first Swarm straight at the player and rolls fresh
// landing points for the rest within the scatter radius.
static void CheckRandomThrows() {
    MonsterManager mm;
    std::vector<Bullet> bullets;
    float hp = 1.0e9f;
    auto* g = new Monster(0.0f, 0.0f);
    g->MakeKind(MobKind::GENESIS);
    mm.monsters.push_back(g);
    const float px = 0.0f, py = 700.0f;
    float firstX[3] = {};
    int differing = 0;
    for (int cycle = 0; cycle < 4; ++cycle) {
        while (g->hivePhase != 1) Step(mm, px, py, hp, bullets, DT);
        for (int i = 0; i < 3; ++i) {
            const float dx = g->genesisShotX[i] - px, dy = g->genesisShotY[i] - py;
            if (i == 0) {
                assert(std::fabs(dx) < 0.01f && std::fabs(dy) < 0.01f);
                continue;
            }
            assert(dx * dx + dy * dy <= Monster::GENESIS_SCATTER_RADIUS *
                                        Monster::GENESIS_SCATTER_RADIUS + 1.0f);
            if (cycle == 0) firstX[i] = g->genesisShotX[i];
            else if (std::fabs(g->genesisShotX[i] - firstX[i]) > 1e-3f) ++differing;
        }
        while (g->hivePhase != 0) Step(mm, px, py, hp, bullets, DT);
        for (auto* m : mm.monsters)                   // clear thrown Swarms
            if (m->genesisThrown) m->alive = false;
    }
    assert(differing >= 4);
    std::printf("random throws: first on player, %d/6 others differ ok\n", differing);
    mm.Clear();
}

// Landing shockwave: 6 damage within 80px of the landing point, once.
// A thrown Swarm spins in proportion to its speed: launch vs landing
// spin rates follow the 1400 / 600 px/s profile, and it stops on landing.
static void CheckSpeedSpin() {
    float hp = 100.0f;
    Monster s(0.0f, 0.0f);
    s.MakeKind(MobKind::SWARM);
    s.BeginThrownFlight(1000.0f, 0.0f);
    float a0 = s.thrownSpin;
    s.Update(0.0f, 3000.0f, DT, hp);
    const float launchRate = (s.thrownSpin - a0) / DT;
    float landRate = 0.0f;
    while (s.thrownPhase == 1) {
        const float u = Monster::ThrownFlightFraction(1000.0f, s.thrownTimer);
        a0 = s.thrownSpin;
        s.Update(0.0f, 3000.0f, DT, hp);
        if (u > 0.85f && u < 0.95f) landRate = (s.thrownSpin - a0) / DT;
    }
    const float want = Monster::THROWN_SWARM_SPIN_PER_PX;
    assert(std::fabs(launchRate - 1400.0f * want) < 0.5f);
    assert(std::fabs(landRate - 600.0f * want) < 0.5f);
    const float landed = s.thrownSpin;
    s.Update(0.0f, 3000.0f, DT, hp);
    assert(s.thrownSpin == landed);                     // no spin while stunned
    std::printf("speed spin: %.1f -> %.1f rad/s ok\n", launchRate, landRate);
}

static void CheckLandingBlast() {
    for (int isNear = 0; isNear <= 1; ++isNear) {
        float hp = 100.0f;
        Monster s(0.0f, 0.0f);
        s.MakeKind(MobKind::SWARM);
        s.BeginThrownFlight(300.0f, 0.0f);
        // Player beside the landing point, off the flight line.
        const float py = isNear ? 70.0f : 95.0f;
        while (s.thrownPhase == 1) s.Update(300.0f, py, DT, hp);
        assert(Monster::THROWN_SWARM_BLAST_RADIUS == 80.0f);
        const float expect = isNear ? 100.0f - Monster::THROWN_SWARM_BLAST_DAMAGE : 100.0f;
        assert(std::fabs(hp - expect) < 0.001f);
        for (float w = 0.0f; w < 0.3f; w += DT) s.Update(300.0f, py, DT, hp);
        assert(std::fabs(hp - expect) < 0.001f);      // once, not per frame
    }
    std::printf("landing blast: ok\n");
}

// Genesis HP is x7, and its thrown Swarms inherit its spawn HP ramp rather
// than the Rotor-only HP upgrade.
static void CheckHpScaling() {
    MonsterManager mm;
    std::vector<Bullet> bullets;
    float hp = 1.0e9f;
    auto* g = new Monster(0.0f, 0.0f, 2.0f);         // spawned under a 2x ramp
    g->MakeKind(MobKind::GENESIS);
    assert(std::fabs(g->hp - 90.0f * 2.0f * 7.0f) < 0.01f);
    mm.monsters.push_back(g);
    while (g->hivePhase != 3)
        mm.UpdateAll(0.0f, 3000.0f, DT, hp, bullets, 1.0f, 1.0f, 5.0f);
    int checked = 0;
    for (auto* m : mm.monsters)
        if (m->genesisThrown) {
            assert(std::fabs(m->hp - 90.0f * 0.35f * 2.0f) < 0.01f);
            ++checked;
        }
    assert(checked > 0);
    std::printf("hp scaling: genesis %.0f, thrown swarm %.1f ok\n",
                g->hp, 90.0f * 0.35f * 2.0f);
    mm.Clear();
}

// Threat check: against a player standing still, a volley must land real
// damage (the old 250px scatter averaged about 2 per volley).
static void CheckVolleyDamage() {
    MonsterManager mm;
    std::vector<Bullet> bullets;
    float hp = 1.0e6f;
    auto* g = new Monster(0.0f, 0.0f);
    g->MakeKind(MobKind::GENESIS);
    mm.monsters.push_back(g);
    const float px = 0.0f, py = 600.0f;
    int volleys = 0;
    int prevPhase = g->hivePhase;
    while (volleys < 20) {
        mm.UpdateAll(px, py, DT, hp, bullets);
        // Landed Swarms would chase; remove them so only the throw counts.
        for (auto* m : mm.monsters)
            if (m->genesisThrown && m->thrownPhase == 0) m->alive = false;
        if (prevPhase == 2 && g->hivePhase == 3) ++volleys;
        prevPhase = g->hivePhase;
    }
    for (float t = 0.0f; t < 1.0f; t += DT) {           // let the last volley land
        mm.UpdateAll(px, py, DT, hp, bullets);
        for (auto* m : mm.monsters)
            if (m->genesisThrown && m->thrownPhase == 0) m->alive = false;
    }
    const float perVolley = (1.0e6f - hp) / (float)volleys;
    assert(perVolley >= 10.0f);
    std::printf("volley damage vs standing player: %.1f per volley ok\n", perVolley);
    mm.Clear();
}

// Speed profile by share of the distance: 1400 px/s for 0-60%, 1100 for
// 60-80%, 600 for 80-100%.
static void CheckThrowSpeedProfile() {
    float hp = 100.0f;
    Monster s(0.0f, 0.0f);
    s.MakeKind(MobKind::SWARM);
    s.BeginThrownFlight(1000.0f, 0.0f);
    const float expectTotal = 600.0f / 1400.0f + 200.0f / 1100.0f + 200.0f / 600.0f;
    assert(std::fabs(s.thrownDuration - expectTotal) < 0.001f);
    float prevX = s.worldX, t = 0.0f;
    float sum[3] = {}, time[3] = {};
    while (s.thrownPhase == 1) {
        s.Update(0.0f, 5000.0f, DT, hp);
        t += DT;
        const float mid = (prevX + s.worldX) * 0.5f;      // segment of this step
        const int seg = mid < 600.0f ? 0 : (mid < 800.0f ? 1 : 2);
        if (s.thrownPhase == 1) { sum[seg] += s.worldX - prevX; time[seg] += DT; }
        prevX = s.worldX;
    }
    const float v0 = sum[0] / time[0], v1 = sum[1] / time[1], v2 = sum[2] / time[2];
    assert(std::fabs(v0 - 1400.0f) < 60.0f);
    assert(std::fabs(v1 - 1100.0f) < 60.0f);
    assert(std::fabs(v2 - 600.0f) < 60.0f);
    assert(std::fabs(t - expectTotal) < 0.03f);
    std::printf("speed profile: %.0f / %.0f / %.0f px/s, %.2fs ok\n", v0, v1, v2, t);
}

int main() {
    srand(11);
    CheckThrowSpeedProfile();
    CheckVolleyDamage();
    CheckLandingBlast();
    CheckSpeedSpin();
    CheckHpScaling();
    CheckLauncherCycle();
    CheckShellStacks();
    CheckRandomThrows();
    CheckThrowRange();
    CheckFlightHit();
    CheckLandingStun();
    CheckFlightPassesThrough();
    CheckThrownCap();
    std::printf("genesis launcher regression passed\n");
    return 0;
}
