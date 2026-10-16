// REG-01 (D01 baseline): every kill entry point x every target, with the
// observable rewards. Run with scripts/test_reg01_kill_rewards.ps1.
//
// Entry points: bullet on a regular mob / on a Scope (CollisionSystem::Update),
// common death collection (CollectMonsterKill / CollectScopeKill) and laser
// (CreditLaserMonsterKill / CreditLaserScopeKill). All paths use the shared
// reward owner in CollisionSystem.h.
//
// Expected values are literals worked out from the rules (default stats), not
// recomputed through the reward functions. Decided 2026-10-13: laser kills of
// regular mobs advance the vampire streak; Creative heals on every path and
// never counts codex kills (DEC-02).
#include "KillRewards.h"
#include <cassert>
#include <cmath>
#include <cstdio>

namespace Audio { void PlaySfx(Sfx) {} }   // kill sound: no audio in tests
float g_BaseOrtho[16] = {};

static constexpr float HEAL = 0.16f;        // 2 lifesteal stacks x 0.08
static constexpr float START_HP = 50.0f;

enum class Target { Rotor, Swarm, ThrownSwarm, Quasar, Scope };
enum class Means { Bullet, Collect, Laser };

static const char* Name(Target t) {
    switch (t) {
    case Target::Rotor: return "rotor";
    case Target::Swarm: return "swarm";
    case Target::ThrownSwarm: return "thrown swarm";
    case Target::Quasar: return "quasar";
    default: return "scope";
    }
}
static const char* Name(Means m) {
    return m == Means::Bullet ? "bullet" : m == Means::Collect ? "collect" : "laser";
}

struct Snap {
    float hp = 0.0f, score = 0.0f, rain = 0.0f;
    long long combo = 0, kills = 0, warlord = 0, vampire = 0, codex = 0;
    int dustValue = 0;
    XpAmount dustXp = 0;
};

struct Outcome {
    Snap d;                         // after - before
    bool pending = false;           // delayed drop held on the mob
    int pendingValue = 0;
    XpAmount pendingXp = 0;
    bool scored = false;
};

struct World {
    MonsterManager manager;
    PlayerStats stats;
    float hp = START_HP, scoreAccum = 0.0f;
    long long score = 0;
    World() {
        stats.lifestealStacks = 2;
        stats.vampire = true;          // N-kill heal, need 10
        stats.warlord = true;          // stack every 1000 kills
        stats.critChance = 0;
        stats.pierce = false;
        stats.ricochetChance = 0;
    }
    KillRewardSink Sink() { return { hp, scoreAccum, score, stats, 0.0f, 0.0f }; }
};

static CodexMobId CodexOf(Target t) {
    switch (t) {
    case Target::Rotor: return CM_ROTOR;
    case Target::Swarm: case Target::ThrownSwarm: return CM_SWARM;
    case Target::Quasar: return CM_QUASAR;
    default: return CM_SCOPE;
    }
}

static Snap Take(World& w, Target t) {
    Snap s;
    s.hp = w.hp; s.score = w.scoreAccum; s.rain = g_RainKillAccum;
    s.combo = g_Combo; s.kills = w.stats.killCount; s.warlord = w.stats.warlordStacks;
    s.vampire = w.stats.vampireKillStreak; s.codex = g_RunMobKillCounts[CodexOf(t)];
    for (const auto& p : g_StardustPickups) { s.dustValue += p.value; s.dustXp += p.xpValue; }
    return s;
}

static Snap Diff(const Snap& a, const Snap& b) {
    Snap d;
    d.hp = b.hp - a.hp; d.score = b.score - a.score; d.rain = b.rain - a.rain;
    d.combo = b.combo - a.combo; d.kills = b.kills - a.kills; d.warlord = b.warlord - a.warlord;
    d.vampire = b.vampire - a.vampire; d.codex = b.codex - a.codex;
    d.dustValue = b.dustValue - a.dustValue; d.dustXp = b.dustXp - a.dustXp;
    return d;
}

static std::vector<Bullet> ShotThrough(float x, float y) {
    Bullet b(x - 12.0f, y, x + 12.0f, y);
    b.prevX = x - 12.0f; b.prevY = y;
    b.x = x + 12.0f;     b.y = y;
    return { b };
}

// Puts one nearly dead target in the world.
static Monster* AddMob(World& w, Target t) {
    Monster* m = new Monster(500.0f, 300.0f, 1.0f);
    m->MakeKind(t == Target::Rotor ? MobKind::ROTOR
              : t == Target::Quasar ? MobKind::QUASAR : MobKind::SWARM);
    m->genesisThrown = t == Target::ThrownSwarm;
    m->hp = 0.01f;
    w.manager.monsters.push_back(m);
    return m;
}
static RangedMob* AddScope(World& w) {
    RangedMob* r = new RangedMob(500.0f, 300.0f, 1920, 1080);
    r->hp = 0.01f;
    w.manager.rangedMobs.push_back(r);
    return r;
}

// Kills the target through one entry point, as the game does.
static void Kill(World& w, Target t, Means means, Monster* m, RangedMob* r) {
    KillRewardSink sink = w.Sink();
    if (means == Means::Bullet) {
        std::vector<Bullet> bullets = m ? ShotThrough(m->worldX, m->worldY)
                                        : ShotThrough(r->worldX, r->worldY);
        CollisionSystem::Update(0.0f, 0.0f, w.manager, bullets, w.hp, w.scoreAccum,
                                w.score, w.stats);
    } else if (means == Means::Collect) {
        // Killed by something that pays nothing itself (burn, drone, orb...).
        if (m) { m->ApplyDamage(1.0f); CollectMonsterKill(sink, *m); }
        else   { r->ApplyDamage(1.0f); CollectScopeKill(sink, *r); }
    } else {
        if (m) { w.manager.ApplyDamage(*m, 1.0f); CreditLaserMonsterKill(sink, *m); }
        else   { w.manager.ApplyDamage(*r, 1.0f); CreditLaserScopeKill(sink, *r); }
    }
    (void)t;
}

static Outcome Run(Target t, Means means, World& w) {
    g_StardustPickups.clear();
    Monster* m = t == Target::Scope ? nullptr : AddMob(w, t);
    RangedMob* r = t == Target::Scope ? AddScope(w) : nullptr;
    const Snap before = Take(w, t);
    Kill(w, t, means, m, r);
    Outcome o;
    o.d = Diff(before, Take(w, t));
    if (m) {
        assert(!m->alive);
        o.scored = m->scored;
        o.pending = m->dustPending; o.pendingValue = m->pendingDustValue; o.pendingXp = m->pendingDustXp;
        // Reprocessing the same death must pay nothing more (the collection
        // pass runs over every dead mob each frame until it explodes).
        KillRewardSink sink = w.Sink();
        const Snap again = Take(w, t);
        CollectMonsterKill(sink, *m);
        CollectMonsterKill(sink, *m);
        CreditLaserMonsterKill(sink, *m);
        const Snap d = Diff(again, Take(w, t));
        assert(d.hp == 0.0f && d.score == 0.0f && d.combo == 0 && d.kills == 0 &&
               d.codex == 0 && d.dustValue == 0 && d.rain == 0.0f);
    } else {
        assert(!r->alive);
        o.scored = r->scored;
        KillRewardSink sink = w.Sink();
        const Snap again = Take(w, t);
        CollectScopeKill(sink, *r);
        CreditLaserScopeKill(sink, *r);
        const Snap d = Diff(again, Take(w, t));
        assert(d.hp == 0.0f && d.score == 0.0f && d.combo == 0 && d.kills == 0 && d.codex == 0);
    }
    return o;
}

static void Print(Target t, Means m, const Outcome& o) {
    std::printf("%-12s %-7s | score %+7.1f xp %+.3f dust %+2d pend %d/%d/%.3f | hp %+.3f "
                "kills %+lld warlord %+lld vamp %+lld | combo %+lld codex %+lld rain %+.0f\n",
                Name(t), Name(m), o.d.score, XpToDouble(o.d.dustXp), o.d.dustValue, o.pending ? 1 : 0,
                o.pendingValue, XpToDouble(o.pendingXp), o.d.hp, o.d.kills, o.d.warlord, o.d.vampire,
                o.d.combo, o.d.codex, o.d.rain);
    std::fflush(stdout);   // keep the table if an assert aborts
}

static bool Near(float a, float b) { return std::fabs(a - b) < 1e-3f; }

// Expected rewards of a paying kill, per target (default stats).
struct Pay { float score; int dust; XpAmount xp; bool delayed; };
static Pay Expected(Target t) {
    switch (t) {
    case Target::Rotor:  return { 100.0f, 1, XpFromWhole(2), false };
    case Target::Swarm:  return { 2.4f, 1, XpFromWhole(1), false };     // Swarm: 20% score
    case Target::Quasar: return { 480.0f, 7, XpFromWhole(12), true };  // dropped on the blast
    default:             return { 300.0f, 3, XpFromWhole(8), false };   // Scope
    }
}

static void CheckMatrix() {
    std::printf("-- matrix (start HP %.0f, lifesteal %.2f, vampire need 10, warlord on)\n",
                START_HP, HEAL);
    for (Target t : { Target::Rotor, Target::Swarm, Target::ThrownSwarm, Target::Quasar,
                      Target::Scope }) {
        for (Means m : { Means::Bullet, Means::Collect, Means::Laser }) {
            World w;
            const Outcome o = Run(t, m, w);
            Print(t, m, o);
            assert(o.scored);
            assert(o.d.combo == 1 && o.d.codex == 1);
            assert(o.d.warlord == 0);
            if (t == Target::ThrownSwarm) {          // BEH-02: combo + codex only
                assert(o.d.score == 0.0f && o.d.dustValue == 0 && o.d.dustXp == 0 && !o.pending);
                assert(o.d.hp == 0.0f && o.d.kills == 0 && o.d.vampire == 0 && o.d.rain == 0.0f);
                continue;
            }
            const Pay p = Expected(t);
            assert(Near(o.d.score, p.score));
            if (p.delayed) {
                assert(o.pending && o.pendingValue == p.dust && o.pendingXp == p.xp);
                assert(o.d.dustValue == 0 && o.d.dustXp == 0);
            } else {
                assert(!o.pending && o.d.dustValue == p.dust && o.d.dustXp == p.xp);
            }
            assert(Near(o.d.hp, HEAL));              // BEH-01: full lifesteal everywhere
            assert(o.d.kills == 1 && o.d.rain == 1.0f);
            assert(o.d.vampire == 1);
        }
    }
}

// Heal never passes maxHP; the vampire N-kill heal and lifesteal are separate.
static void CheckHealing() {
    std::printf("-- healing\n");
    for (Means m : { Means::Bullet, Means::Collect, Means::Laser }) {
        World w;
        w.hp = w.stats.maxHP - 0.05f;
        Run(Target::Rotor, m, w);
        assert(Near(w.hp, w.stats.maxHP));

        World v;                                     // 9 kills banked of 10
        v.stats.vampireKillStreak = 9;
        Run(Target::Rotor, m, v);
        std::printf("%-7s vampire 9/10 -> streak %d, hp %+.3f\n", Name(m),
                    v.stats.vampireKillStreak, v.hp - START_HP);
        assert(v.stats.vampireKillStreak == 0 && Near(v.hp, START_HP + HEAL + 1.0f));

        World swarm;
        swarm.stats.vampireKillStreak = 9;
        Run(Target::Swarm, m, swarm);
        assert(swarm.stats.vampireKillStreak == 0 &&
               Near(swarm.hp, START_HP + HEAL + 1.0f));

        World thrown;
        thrown.stats.vampireKillStreak = 9;
        Run(Target::ThrownSwarm, m, thrown);
        assert(thrown.stats.vampireKillStreak == 9 && Near(thrown.hp, START_HP));
    }
    for (Means m : { Means::Bullet, Means::Collect, Means::Laser }) {
        World s;                                     // Scope: streak advances on every path
        s.stats.vampireKillStreak = 9;
        Run(Target::Scope, m, s);
        assert(s.stats.vampireKillStreak == 0 && Near(s.hp, START_HP + HEAL + 1.0f));

        World full;                                  // the 1 HP heal stops at maxHP too
        full.stats.vampireKillStreak = 9;
        full.hp = full.stats.maxHP - 0.5f;
        Run(Target::Scope, m, full);
        assert(full.stats.vampireKillStreak == 0 && Near(full.hp, full.stats.maxHP));
    }
}

// Warlord stacks on the 1000th run kill; a thrown Swarm is not a run kill.
static void CheckWarlord() {
    std::printf("-- warlord 999 -> next kill\n");
    for (Means m : { Means::Bullet, Means::Collect, Means::Laser }) {
        World w;
        w.stats.killCount = 999;
        Run(Target::Rotor, m, w);
        assert(w.stats.killCount == 1000 && w.stats.warlordStacks == 1);

        World th;
        th.stats.killCount = 999;
        Run(Target::ThrownSwarm, m, th);
        assert(th.stats.killCount == 999 && th.stats.warlordStacks == 0);
    }
    World w;
    w.stats.killCount = 998;
    Run(Target::Rotor, Means::Bullet, w);
    assert(w.stats.killCount == 999 && w.stats.warlordStacks == 0);
}

// DEC-02 (decided 2026-10-13): Creative heals like a real run on every path
// (lifesteal and the vampire heal), and the codex never counts Creative kills.
static void CheckCreative() {
    std::printf("-- creative (DEC-02)\n");
    g_CreativeMode = true;
    for (Target t : { Target::Rotor, Target::Scope }) {
        for (Means m : { Means::Bullet, Means::Collect, Means::Laser }) {
            World w;
            const Outcome o = Run(t, m, w);
            Print(t, m, o);
            assert(o.d.codex == 0 && o.d.kills == 1 && o.d.vampire == 1);
            assert(Near(o.d.score, t == Target::Scope ? 300.0f : 100.0f));
            assert(Near(o.d.hp, HEAL));                       // same heal on every path
        }
    }
    g_CreativeMode = false;
}

static void CheckFractionalXp() {
    for (Means means : { Means::Bullet, Means::Collect, Means::Laser }) {
        World swarm;
        swarm.stats.xpMult = 1.03f;
        assert(Run(Target::Swarm, means, swarm).d.dustXp == 1030);

        World rotor;
        rotor.stats.xpMult = 1.03f;
        rotor.stats.mobXpBonus = 1;
        rotor.stats.rotorXpBonus = 2;
        assert(Run(Target::Rotor, means, rotor).d.dustXp == 5150);

        World scope;
        scope.stats.xpMult = 1.03f;
        scope.stats.rangedXpBonus = 12;
        assert(Run(Target::Scope, means, scope).d.dustXp == 20600);

        World quasar;
        quasar.stats.xpMult = 1.03f;
        quasar.stats.mobKindXpBonus[(int)MobKind::QUASAR] = 1;
        assert(Run(Target::Quasar, means, quasar).pendingXp == 13390);
    }

    g_StardustPickups.clear();
    SpawnStardust(0, 0, 20, 0, 0, 1); // 0.001 XP over two pieces
    assert(g_StardustPickups.size() == 2);
    XpAmount total = 0;
    for (const auto& piece : g_StardustPickups) total += piece.xpValue;
    assert(total == 1);
    g_StardustPickups.clear();
    SpawnStardust(0, 0, 0, 0, 0, 1); // no currency, still carries XP
    assert(g_StardustPickups.size() == 1 && g_StardustPickups[0].xpValue == 1);
}

int main() {
    CheckMatrix();
    CheckHealing();
    CheckWarlord();
    CheckCreative();
    CheckFractionalXp();
    std::printf("REG-01 kill reward regression passed\n");
    return 0;
}
