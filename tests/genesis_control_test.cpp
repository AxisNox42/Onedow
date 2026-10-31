// Headless regression for the enemies a Genesis controls: they share Genesis' own
// colour family, run to where the player WILL be (intercept), sidestep incoming
// player shots in short bursts, and fall back to plain AI and their own colour
// when the Genesis dies. Run with scripts/test_genesis_control.ps1.
#include "MonsterManager.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <set>

static constexpr float DT = 1.0f / 120.0f;

static bool Near(float a, float b, float eps = 1e-3f) { return std::fabs(a - b) < eps; }

static Monster* FreeSwarm(float x, float y, float speed = 200.0f) {
    auto* m = new Monster(x, y);
    m->MakeKind(MobKind::SWARM);
    m->speed = speed;
    return m;
}

static void CheckPalette() {
    Monster swarm(0, 0); swarm.MakeKind(MobKind::SWARM);
    Monster rotor(0, 0); rotor.MakeKind(MobKind::ROTOR);
    Monster gen(0, 0);   gen.MakeKind(MobKind::GENESIS);
    assert(gen.color == Monster::GENESIS_COLOR);
    // Free Swarms/Rotors are red/orange (little blue); Genesis and its controlled
    // enemies are magenta (strong blue) and differ from each other.
    assert(swarm.color.b < 0.2f && rotor.color.b < 0.2f);
    assert(Monster::GENESIS_COLOR.b > 0.4f && Monster::GENESIS_CONTROLLED_COLOR.b > 0.6f);
    assert(Monster::GENESIS_CONTROLLED_COLOR != Monster::GENESIS_COLOR);
    assert(Monster::GENESIS_CONTROLLED_COLOR != swarm.color && Monster::GENESIS_CONTROLLED_COLOR != rotor.color);
    // Taking and releasing control swaps the colour and remembers the original.
    Monster m(0, 0); m.MakeKind(MobKind::SWARM);
    const glm::vec3 own = m.color;
    m.TakeGenesisControl(7);
    assert(m.genesisControlled && m.thrownOwnerId == 7 && m.color == Monster::GENESIS_CONTROLLED_COLOR && m.freeColor == own);
    m.TakeGenesisControl(7);                                   // taking it twice must not lose the original
    assert(m.freeColor == own);
    m.ReleaseGenesisControl();
    assert(!m.genesisControlled && m.color == Monster::GENESIS_CONTROLLED_COLOR);   // keeps the marking after release
    std::printf("palette ok\n");
}

// The intercept point is where mob and player meet at the same time.
static void CheckIntercept() {
    const glm::vec2 mob(0.0f, 0.0f);
    // A player crossing at 200 px/s, a mob at 400 px/s: arrival times must agree.
    const glm::vec2 player(200.0f, 0.0f), vel(0.0f, 200.0f);     // meeting time 0.58 s: inside the 0.7 s look-ahead
    const float s = 400.0f;
    const glm::vec2 hit = Monster::GenesisInterceptPoint(mob, player, vel, s);
    const float t = glm::length(hit - player) / glm::length(vel);
    assert(Near(glm::length(hit - mob), s * t, 2.0f));
    assert(hit.y > 50.0f);                                      // it aims ahead of the player, not at them
    // A standing player: aim straight at them.
    assert(Monster::GenesisInterceptPoint(mob, player, glm::vec2(0.0f), s) == player);
    // Already close: just close in.
    assert(Monster::GenesisInterceptPoint(mob, glm::vec2(40.0f, 0.0f), vel, s) == glm::vec2(40.0f, 0.0f));
    // A player running away faster than the mob can follow: no exact meeting, so it
    // leads by the capped look-ahead instead of aiming at the far future.
    const glm::vec2 away = Monster::GenesisInterceptPoint(mob, player, glm::vec2(600.0f, 0.0f), 300.0f);
    assert(away.x > player.x && away.x <= player.x + 600.0f * Monster::GENESIS_CTRL_LEAD_MAX + 1.0f);
    std::printf("intercept ok\n");
}

// In the game loop: a controlled enemy cuts the corner on a moving player.
static void CheckLeadPursuit() {
    float meY[2] = { 0.0f, 0.0f };
    for (int controlled = 0; controlled <= 1; ++controlled) {
        Monster* m = FreeSwarm(0.0f, 0.0f, 200.0f);
        if (controlled) m->TakeGenesisControl(1);
        float hp = 1.0e9f;
        for (float t = 0.0f; t < 0.7f; t += DT) {
            const glm::vec2 p(400.0f, 250.0f * t);           // the player walks straight up
            m->Update(p.x, p.y, DT, hp);
        }
        meY[controlled] = m->worldY;
        delete m;
    }
    // The free one heads for where the player is (near y=0 at first); the controlled
    // one has already turned up toward where they are going.
    assert(meY[1] > meY[0] + 15.0f);
    std::printf("lead pursuit ok (free y=%.0f, controlled y=%.0f)\n", meY[0], meY[1]);
}

static Bullet PlayerShot(float x, float y, float tx, float ty) {
    Bullet b(x, y, tx, ty);
    b.isEnemy = false;
    b.speed = 1200.0f;
    return b;
}

// Sidestep: short bursts across the shot's line, not at every shot.
static void CheckDodge() {
    const auto run = [&](bool controlled, std::vector<Bullet> bullets, float seconds, float* firstDodgeAt) {
        Monster* m = FreeSwarm(0.0f, 0.0f, 150.0f);
        if (controlled) m->TakeGenesisControl(1);
        float hp = 1.0e9f, ymax = 0.0f;
        if (firstDodgeAt) *firstDodgeAt = -1.0f;
        float t = 0.0f;
        for (; t < seconds; t += DT) {
            m->Update(900.0f, 0.0f, DT, hp, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, &bullets);
            for (Bullet& b : bullets) if (b.active) b.Update(DT);     // the shots fly on
            ymax = std::max(ymax, std::fabs(m->worldY));
            if (firstDodgeAt && *firstDodgeAt < 0.0f && std::fabs(m->worldY) > 8.0f) *firstDodgeAt = t;
        }
        const float y = ymax;
        delete m;
        return y;
    };
    // A shot flying straight at it: the controlled enemy leaves the line, the free one does not.
    const std::vector<Bullet> straight = { PlayerShot(-220.0f, 0.0f, 100.0f, 0.0f) };
    float at = 0.0f;
    const float free0 = run(false, straight, 0.4f, nullptr);
    const float ctl0 = run(true, straight, 0.4f, &at);
    assert(free0 < 1.0f);                                      // a free mob never moves off its line
    assert(ctl0 > 30.0f);                                      // the controlled one sidestepped clear
    assert(at >= 0.0f && at < 0.15f);                          // and did it right away
    // A shot that would miss by a wide margin is ignored; so are enemy shots.
    const std::vector<Bullet> wide = { PlayerShot(-220.0f, 140.0f, 100.0f, 140.0f) };
    assert(run(true, wide, 0.4f, nullptr) < 1.0f);
    std::vector<Bullet> hostile = { PlayerShot(-220.0f, 0.0f, 100.0f, 0.0f) };
    hostile[0].isEnemy = true;
    assert(run(true, hostile, 0.4f, nullptr) < 1.0f);
    // It cannot sidestep forever: a second shot right after is not dodged until the
    // cooldown is over, so a steady stream still hits it.
    Monster* m = FreeSwarm(0.0f, 0.0f, 150.0f);
    m->TakeGenesisControl(1);
    std::vector<Bullet> shots = { PlayerShot(-220.0f, 0.0f, 100.0f, 0.0f) };
    float hp = 1.0e9f, t = 0.0f;
    int sidesteps = 0;
    bool wasDodging = false;
    for (; t < 3.0f; t += DT) {
        if (std::fmod(t, 0.25f) < DT) { shots.push_back(PlayerShot(m->worldX - 220.0f, m->worldY, m->worldX + 100.0f, m->worldY)); }
        m->Update(900.0f, 0.0f, DT, hp, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, &shots);
        for (Bullet& b : shots) if (b.active) b.Update(DT);
        const bool d = m->ctrlDodgeTimer > 0.0f;
        if (d && !wasDodging) ++sidesteps;
        wasDodging = d;
    }
    // 3 s of shots every 0.25 s, one sidestep per (burst + cooldown) at most.
    const float cycle = Monster::GENESIS_CTRL_DODGE_BURST + Monster::GENESIS_CTRL_DODGE_COOLDOWN;
    assert(sidesteps >= 2 && sidesteps <= (int)std::ceil(3.0f / cycle) + 1);
    delete m;
    std::printf("dodge ok (%d sidesteps in 3 s of fire)\n", sidesteps);
}

// A player who jukes back and forth must not make the enemy dumber than a plain chaser. The
// same zig-zag is replayed against (a) a plain chaser, (b) the old pure-lead chaser (rebuilt
// from the same intercept maths, with no doubt about the lead) and (c) the controlled enemy.
// Two measures: how long the first contact takes, and how close it stays once it has arrived.
struct JukeResult { float contact; float lateMean; };

static JukeResult RunJuke(int mode, float freq, float amp) {
    Monster* m = FreeSwarm(0.0f, 0.0f, 230.0f);
    if (mode == 2) m->TakeGenesisControl(1);
    glm::vec2 prev(0.0f), vel(0.0f);
    bool seen = false;
    float hp = 1.0e9f, sum = 0.0f, contact = -1.0f;
    int n = 0;
    for (float t = 0.0f; t < 8.0f; t += DT) {
        const glm::vec2 p(420.0f, amp * std::sin(6.2831853f * freq * t));    // the player jukes up and down
        if (mode == 1) {                                                        // the old behaviour: lead, no doubt
            if (seen) vel += ((p - prev) / DT - vel) * (1.0f - std::exp(-DT * 10.0f));
            prev = p; seen = true;
            const glm::vec2 aim = Monster::GenesisInterceptPoint(glm::vec2(m->worldX, m->worldY), p, vel, m->speed);
            glm::vec2 to = aim - glm::vec2(m->worldX, m->worldY);
            const float len = glm::length(to);
            if (len > 1e-3f) { m->worldX += to.x / len * m->speed * DT; m->worldY += to.y / len * m->speed * DT; }
        } else {
            m->Update(p.x, p.y, DT, hp);
        }
        const float d = glm::length(glm::vec2(m->worldX, m->worldY) - p);
        if (contact < 0.0f && d < 45.0f) contact = t;
        if (t > 4.0f) { sum += d; ++n; }
    }
    delete m;
    return { contact, sum / (float)n };
}

static void CheckJukeIsNotDumb() {
    for (float freq : { 0.6f, 1.0f, 1.6f }) {
        const float amp = 110.0f;
        const JukeResult plain = RunJuke(0, freq, amp), old = RunJuke(1, freq, amp), now = RunJuke(2, freq, amp);
        std::printf("  juke %.1f Hz: first contact plain %.2fs / old pure lead %.2fs / now %.2fs;  late distance %.0f / %.0f / %.0f px\n",
                    freq, plain.contact, old.contact, now.contact, plain.lateMean, old.lateMean, now.lateMean);
        std::fflush(stdout);
        assert(plain.contact > 0.0f && now.contact > 0.0f);
        assert(now.contact <= plain.contact * 1.10f);          // as quick to reach a juker as a plain chaser
        assert(now.lateMean <= plain.lateMean * 1.05f);        // and no further away once it is there
        assert(now.contact <= old.contact + 0.05f);            // never slower than the old pure lead
        if (freq >= 1.0f) assert(old.contact - now.contact >= 0.5f);   // which was badly fooled by fast jukes
    }
    // Reversals really do cost the lead its trust, and steady running earns it back.
    Monster* m = FreeSwarm(0.0f, 0.0f, 200.0f);
    m->TakeGenesisControl(1);
    float hp = 1.0e9f;
    float t = 0.0f;
    float y = 0.0f;                                                          // one continuous path, no jumps
    for (; t < 1.0f; t += DT) { y += 300.0f * DT; m->Update(500.0f, y, DT, hp); }      // steady run: full trust
    assert(m->ctrlConfidence > 0.95f);
    for (; t < 1.5f; t += DT) { y -= 300.0f * DT; m->Update(500.0f, y, DT, hp); }      // turns right around
    assert(m->ctrlConfidence < 0.5f);
    for (; t < 3.6f; t += DT) { y -= 300.0f * DT; m->Update(500.0f, y, DT, hp); }      // keeps going the new way
    assert(m->ctrlConfidence > 0.7f);
    delete m;
    std::printf("juke handling ok\n");
}

// The control ends with its Genesis: plain AI again, the controlled colour stays.
static void CheckReleaseOnOwnerDeath() {
    MonsterManager mm;
    auto* g = new Monster(600.0f, 300.0f);
    g->MakeKind(MobKind::GENESIS);
    auto* child = FreeSwarm(300.0f, 300.0f);
    child->TakeGenesisControl(g->genesisId);
    mm.monsters.push_back(g);
    mm.monsters.push_back(child);
    std::vector<Bullet> bullets;
    float hp = 1.0e9f;
    mm.UpdateAll(100.0f, 100.0f, DT, hp, bullets);
    assert(child->genesisControlled && child->color == Monster::GENESIS_CONTROLLED_COLOR);
    g->hp = 0.0f; g->alive = false;                            // the Genesis dies
    mm.UpdateAll(100.0f, 100.0f, DT, hp, bullets);
    assert(!child->genesisControlled && child->color == Monster::GENESIS_CONTROLLED_COLOR);   // plain AI, colour kept
    mm.Clear();
    std::printf("release on owner death ok\n");
}

// The enemies a running Genesis makes are controlled from the start.
static void CheckSpawnedAreControlled() {
    MonsterManager mm;
    auto* g = new Monster(700.0f, 300.0f);
    g->MakeKind(MobKind::GENESIS);
    g->speed = 0.0f;
    mm.monsters.push_back(g);
    std::vector<Bullet> bullets;
    float hp = 1.0e9f;
    int controlledSwarms = 0, controlledRotors = 0;
    for (float t = 0.0f; t < 30.0f && (controlledSwarms == 0 || controlledRotors == 0); t += DT) {
        mm.UpdateAll(200.0f, 300.0f, DT, hp, bullets);
        for (auto* m : mm.monsters) {
            if (m == g || !m->alive || m->thrownOwnerId != g->genesisId) continue;
            assert(m->genesisControlled && m->color == Monster::GENESIS_CONTROLLED_COLOR);
            if (m->kind == MobKind::SWARM) ++controlledSwarms;
            if (m->kind == MobKind::ROTOR) ++controlledRotors;
        }
    }
    assert(controlledSwarms > 0 && controlledRotors > 0);
    mm.Clear();
    std::printf("spawned enemies are controlled ok\n");
}

// Nightmare: a Genesis' thrown capsules open into random regular enemies.
static Monster* Capsule(float x, float y, int owner, bool carrier = true, int phase = 0) {
    Monster* c = FreeSwarm(x, y);
    c->TakeGenesisControl(owner);
    c->genesisThrown = true;
    c->genesisSummonCarrier = carrier;
    c->genesisSummonHpMult = 1.5f;
    c->thrownPhase = phase;
    return c;
}

static void CheckRandomSummons() {
    std::vector<Bullet> bullets;
    float hp = 1.0e9f;
    // A landed Scope capsule bursts into a Scope that starts under control, with the Genesis'
    // HP scaling and its normal size. (Ground enemies are thrown as themselves, see below.)
    {
        MonsterManager mm;
        auto* g = new Monster(2000.0f, 2000.0f); g->MakeKind(MobKind::GENESIS); g->speed = 0.0f;
        mm.monsters.push_back(g);
        for (int i = 0; i < 20; ++i) {
            Monster* c = Capsule(100.0f + (float)i, 100.0f, g->genesisId);
            c->genesisCarryScope = true;
            mm.monsters.push_back(c);
        }
        mm.UpdateAll(5000.0f, 5000.0f, DT, hp, bullets);
        assert((int)mm.rangedMobs.size() == 20);
        for (RangedMob* r : mm.rangedMobs) {
            assert(r->alive && r->genesisSummoned && r->summonOwnerId == g->genesisId);
            assert(r->color == Monster::GENESIS_CONTROLLED_COLOR && r->freeColor == glm::vec3(0.15f, 0.85f, 1.0f));
            assert(std::fabs(r->hp - RangedMob::BASE_HP * 1.5f) < 0.01f);   // the Genesis' HP multiplier
        }
        mm.Clear();
    }
    // The Genesis' own body grows with the difficulty (its throws do not).
    {
        const Difficulty saved = g_Difficulty;
        float prev = 0.0f;
        for (int d = 0; d < 4; ++d) {
            g_Difficulty = (Difficulty)d;
            Monster g(0, 0); g.MakeKind(MobKind::GENESIS);
            assert(g.sizeScale > prev); prev = g.sizeScale;
            Monster sw(0, 0); sw.MakeKind(MobKind::SWARM);
            assert(sw.sizeScale == 1.0f);
        }
        g_Difficulty = Difficulty::EASY;
        Monster e(0, 0); e.MakeKind(MobKind::GENESIS);
        assert(std::fabs(e.sizeScale - 1.7f) < 1e-4f);
        g_Difficulty = Difficulty::NIGHTMARE;
        Monster n(0, 0); n.MakeKind(MobKind::GENESIS);
        assert(n.sizeScale > 1.7f * 1.25f);
        g_Difficulty = saved;
    }
    // Not a capsule (other difficulties): nothing changes. In flight or stunned: not open yet.
    {
        MonsterManager mm;
        auto* g = new Monster(2000.0f, 2000.0f); g->MakeKind(MobKind::GENESIS); g->speed = 0.0f;
        mm.monsters.push_back(g);
        auto* plain = Capsule(100.0f, 100.0f, g->genesisId, false, 0);
        auto* flying = Capsule(200.0f, 100.0f, g->genesisId, true, 1);
        auto* stunned = Capsule(300.0f, 100.0f, g->genesisId, true, 2);
        mm.monsters.push_back(plain); mm.monsters.push_back(flying); mm.monsters.push_back(stunned);
        for (auto* c : { plain, flying, stunned }) c->thrownTargetX = c->thrownStartX = c->worldX;
        mm.UpdateAll(5000.0f, 5000.0f, DT, hp, bullets);
        assert(plain->kind == MobKind::SWARM && !plain->genesisSummoned);
        assert(flying->genesisSummonCarrier && stunned->genesisSummonCarrier);
        mm.Clear();
    }
    // The Genesis dies before the capsule opens: it lands as a plain Swarm, not as a summon.
    {
        MonsterManager mm;
        auto* g = new Monster(2000.0f, 2000.0f); g->MakeKind(MobKind::GENESIS); g->speed = 0.0f;
        mm.monsters.push_back(g);
        auto* c = Capsule(100.0f, 100.0f, g->genesisId, true, 2);
        mm.monsters.push_back(c);
        g->hp = 0.0f; g->alive = false;
        mm.UpdateAll(5000.0f, 5000.0f, DT, hp, bullets);
        assert(!c->genesisControlled && !c->genesisSummonCarrier);
        c->thrownPhase = 0;
        mm.UpdateAll(5000.0f, 5000.0f, DT, hp, bullets);
        assert(c->kind == MobKind::SWARM && !c->genesisSummoned);
        mm.Clear();
    }
    // The Genesis dies: a Scope it summoned gets its own colour back (and stays a half-reward kill).
    {
        MonsterManager mm;
        auto* g = new Monster(2000.0f, 2000.0f); g->MakeKind(MobKind::GENESIS); g->speed = 0.0f;
        mm.monsters.push_back(g);
        auto* r = new RangedMob(300.0f, 300.0f, 1280, 720);
        r->genesisSummoned = true; r->summonOwnerId = g->genesisId;
        r->freeColor = r->color; r->color = Monster::GENESIS_CONTROLLED_COLOR;
        mm.rangedMobs.push_back(r);
        g->hp = 0.0f; g->alive = false;
        mm.UpdateAll(5000.0f, 5000.0f, DT, hp, bullets);
        assert(r->color == Monster::GENESIS_CONTROLLED_COLOR && r->summonOwnerId == 0 && r->genesisSummoned);
        mm.Clear();
    }
    // A live Nightmare Genesis: its throws summon real enemies, within the cap, with its
    // Rotor launches unaffected by them.
    {
        MonsterManager mm;
        auto* g = new Monster(700.0f, 300.0f); g->MakeKind(MobKind::GENESIS); g->speed = 0.0f;
        g->genesisRandomSummons = true;
        g->hp = 1.0e9f;
        mm.monsters.push_back(g);
        int maxOwned = 0, summonedSeen = 0, rotorsFired = 0;
        bool nonSwarm = false, flewAsKind = false;
        int launcherKinds[16] = {}; bool launcherSettled = false;
        std::set<Monster*> rotorSeen;
        for (float t = 0.0f; t < 40.0f; t += DT) {
            mm.UpdateAll(200.0f, 300.0f, DT, hp, bullets);
            int owned = 0, summoned = 0;
            for (auto* m : mm.monsters) {
                if (m == g || !m->alive || m->thrownOwnerId != g->genesisId) continue;
                if (!m->genesisFromLauncher) ++owned;
                if (m->genesisSummoned && m->thrownPhase == 1 && m->kind != MobKind::SWARM) {
                    flewAsKind = true;                                // already its own kind in the air
                }
                if (m->genesisFromLauncher && m->genesisSummoned) {   // the Rotor launcher loads random enemies too
                    launcherKinds[(int)m->kind] = 1;
                    Monster ref(0, 0); ref.MakeKind(m->kind);
                    if (m->genesisRotorPhase == 0 && m->thrownDuration > 0.0f && std::fabs(m->sizeScale - ref.sizeScale) < 1e-4f)
                        launcherSettled = true;                       // landed: its own size
                }
                if (m->genesisSummoned) { ++summoned; assert(!IsEliteMob(m->kind) && m->kind != MobKind::NORMA); if (m->kind != MobKind::SWARM) nonSwarm = true; }
                if (m->genesisFromLauncher && rotorSeen.insert(m).second) ++rotorsFired;
            }
            for (auto* r : mm.rangedMobs) if (r->alive && r->summonOwnerId == g->genesisId) ++owned;   // Scopes count too
            maxOwned = std::max(maxOwned, owned);
            summonedSeen = std::max(summonedSeen, summoned);
            for (auto* m : mm.monsters) if (m != g && m->alive) m->hp = 1.0e9f;   // keep them around
        }
        assert(summonedSeen > 0 && nonSwarm && flewAsKind);
        int lk = 0; for (int v : launcherKinds) lk += v;
        assert(lk >= 2 && launcherSettled);                           // launcher units are a mix, and settle to their size
        assert(maxOwned <= Monster::GENESIS_THROWN_CAP);
        assert(rotorsFired >= Monster::GENESIS_ROTOR_COUNT);           // the Rotor attack still runs
        mm.Clear();
    }
    std::printf("random summons ok\n");
}

// A summoned shooter always leads its shot (forced), even against a juking player.
static void CheckControlledGimbalAim() {
    Monster g(0.0f, 0.0f);
    g.MakeKind(MobKind::GIMBAL);
    g.TakeGenesisControl(1);
    g.ctrlPlayerVel = glm::vec2(0.0f, 300.0f);
    for (float conf : { 1.0f, 0.0f }) {
        g.ctrlConfidence = conf;
        const glm::vec2 aim = g.GenesisShotAimPoint(500.0f, 0.0f, 360.0f);
        assert(aim.x == 500.0f && aim.y > 100.0f);            // ahead of the player, along their heading
    }
    std::printf("summoned gimbal forced lead ok\n");
}

// A summoned Scope fires at where a moving player will be, not at where they are.
static void CheckSummonedScopeLead() {
    RangedMob r(0.0f, 0.0f, 1280, 720);
    r.genesisSummoned = true;
    r.targetX = r.worldX; r.targetY = r.worldY;
    std::vector<Bullet> bullets;
    float py = 0.0f;
    for (float t = 0.0f; t < 12.0f && bullets.empty(); t += DT) {
        py += 300.0f * DT;
        r.targetX = r.worldX; r.targetY = r.worldY; r.wanderTimer = 0.0f;
        r.Update(500.0f, py, DT, bullets);
    }
    assert(!bullets.empty());
    const Bullet& b = bullets[0];
    assert(!b.homing);
    assert(b.dirY > 0.0f && b.dirY / std::max(0.001f, std::fabs(b.dirX)) > 0.2f);   // aimed ahead of a player moving +y
    std::printf("summoned scope lead ok\n");
}

// Summoned melee enemies speed up while they chase, and stop at the limit.
static void CheckSummonedRamp() {
    for (MobKind k : { MobKind::SWARM, MobKind::GRAVIS, MobKind::ROTOR }) {
        Monster m(0.0f, 0.0f);
        m.MakeKind(k);
        m.genesisSummoned = true;
        m.TakeGenesisControl(1);
        std::vector<Bullet> bullets;
        float hp = 1.0e9f;
        float prev = 1.0f;
        for (float t = 0.0f; t < 4.0f; t += DT) {
            m.worldX = -3000.0f; m.worldY = 0.0f;
            m.Update(0.0f, 0.0f, DT, hp, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, &bullets);
            assert(m.summonRamp >= prev);
            prev = m.summonRamp;
        }
        assert(m.summonRamp > 1.2f);
        for (float t = 0.0f; t < 30.0f; t += DT) {
            m.worldX = -3000.0f; m.worldY = 0.0f;
            m.Update(0.0f, 0.0f, DT, hp, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, &bullets);
        }
        assert(m.summonRamp == Monster::SUMMON_RAMP_MAX);
    }
    Monster plain(0.0f, 0.0f);                                   // a free (non-summoned) mob never ramps
    plain.MakeKind(MobKind::SWARM);
    std::vector<Bullet> bl; float hp = 1.0e9f;
    for (float t = 0.0f; t < 5.0f; t += DT) { plain.worldX = -3000.0f; plain.Update(0.0f, 0.0f, DT, hp, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, &bl); }
    assert(plain.summonRamp == 1.0f);
    std::printf("summoned melee ramp ok\n");
}

int main() {
    CheckPalette();
    CheckIntercept();
    CheckLeadPursuit();
    CheckJukeIsNotDumb();
    CheckDodge();
    CheckReleaseOnOwnerDeath();
    CheckSpawnedAreControlled();
    CheckRandomSummons();
    CheckControlledGimbalAim();
    CheckSummonedScopeLead();
    CheckSummonedRamp();
    std::printf("genesis control regression passed\n");
    return 0;
}
