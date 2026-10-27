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

static int BuffCount(const Monster* m) {
    return (m->regulusShield.acquiredOnce ? 1 : 0) + (m->regulusSpeedAge >= 0.0f ? 1 : 0) +
           (m->regulusStealthAge >= 0.0f ? 1 : 0);
}

// The laser is instant: the ally inside 500px gets one buff the moment the
// shot fires, the beam is only a short afterglow (coloured by the buff), and
// nothing stays linked to the Regulus afterwards.
static void CheckInstantGrant() {
    MonsterManager mm;
    auto* reg = Add(mm, MobKind::REGULUS, 0.0f, 0.0f);
    reg->regulusDriftTimer = 1.0e6f;
    auto* inside = Add(mm, MobKind::GRAVIS, 450.0f, 0.0f);
    auto* outside = Add(mm, MobKind::GRAVIS, 560.0f, 0.0f);
    Step(mm, 0.51f);
    assert(BuffCount(inside) == 1);                             // no travel time
    assert(mm.regulusBeams.size() == 1);
    assert(std::fabs(mm.regulusBeams[0].x1 - 450.0f) < 0.01f);
    const int buff = mm.regulusBeams[0].buff;
    assert(buff >= 0 && buff <= 2);
    if (buff == Monster::REGULUS_BUFF_SHIELD) {
        assert(std::fabs(inside->regulusShield.hp - reg->regulusBaseHp * 0.6f) < 0.01f);
        assert(inside->regulusShield.grantTimer > 0.0f);        // snap-in running
    }
    Step(mm, Monster::REGULUS_BEAM_TIME + 0.05f);
    assert(mm.regulusBeams.empty());                            // afterglow gone
    Step(mm, 3.0f);
    assert(BuffCount(outside) == 0);                            // beyond 500px
    reg->alive = false;                                         // buffs are independent
    Step(mm, 0.5f);
    assert(BuffCount(inside) >= 1);
    std::printf("instant grant: ok\n");
    mm.Clear();
}

// Each buff has its own laser colour.
static void CheckBuffColors() {
    const glm::vec3 a = Monster::RegulusBuffColor(Monster::REGULUS_BUFF_SHIELD);
    const glm::vec3 b = Monster::RegulusBuffColor(Monster::REGULUS_BUFF_SPEED);
    const glm::vec3 c = Monster::RegulusBuffColor(Monster::REGULUS_BUFF_STEALTH);
    assert(a != b && b != c && a != c);
    std::printf("buff colours distinct ok\n");
}

// A volley hits up to 3 distinct allies at once, then waits the full interval.
static void CheckVolley() {
    MonsterManager mm;
    auto* reg = Add(mm, MobKind::REGULUS, 0.0f, 0.0f);
    reg->regulusDriftTimer = 1.0e6f;
    std::vector<Monster*> allies;
    for (int i = 0; i < 5; ++i)
        allies.push_back(Add(mm, MobKind::ROTOR, 150.0f * cosf(i * 1.2f),
                             150.0f * sinf(i * 1.2f)));
    Step(mm, 0.51f);                                            // first volley at 0.5 s
    int buffed = 0;
    for (auto* a : allies) { assert(BuffCount(a) <= 1); buffed += BuffCount(a); }
    assert(buffed == Monster::REGULUS_VOLLEY_COUNT);
    assert((int)mm.regulusBeams.size() == Monster::REGULUS_VOLLEY_COUNT);
    Step(mm, 2.0f);                                             // t = 2.5 s: still waiting
    buffed = 0;
    for (auto* a : allies) buffed += BuffCount(a);
    assert(buffed == Monster::REGULUS_VOLLEY_COUNT);
    Step(mm, 1.2f);                                             // second volley at 3.5 s
    buffed = 0;
    for (auto* a : allies) { assert(BuffCount(a) <= 1); buffed += BuffCount(a); }
    assert(buffed == 5);                                        // only the 2 free ones left
    std::printf("volley: ok\n");
    mm.Clear();
}

// Buffs are permanent and a mob only ever gets one: even after its shield is
// broken it is never buffed again, and speed / stealth never wear off.
static void CheckPermanentOneBuff() {
    srand(77);
    MonsterManager mm;
    auto* reg = Add(mm, MobKind::REGULUS, 0.0f, 0.0f);
    reg->regulusDriftTimer = 1.0e6f;
    std::vector<Monster*> allies;
    for (int i = 0; i < 6; ++i)
        allies.push_back(Add(mm, MobKind::ROTOR, 120.0f * cosf(i * 1.047f),
                             120.0f * sinf(i * 1.047f)));
    std::vector<int> firstBuff(allies.size(), -1);
    for (int step = 0; step < 90 * 8; ++step) {                 // 90 s
        Step(mm, 1.0f / 8.0f);
        for (size_t i = 0; i < allies.size(); ++i) {
            auto* a = allies[i];
            assert(BuffCount(a) <= 1);
            const int kind = a->regulusShield.acquiredOnce ? 0 : a->regulusSpeedAge >= 0.0f ? 1
                           : a->regulusStealthAge >= 0.0f ? 2 : -1;
            if (firstBuff[i] < 0) firstBuff[i] = kind;
            else assert(firstBuff[i] == kind);                  // never changes
        }
        for (auto* a : allies) a->regulusShield.hp = 0.0f;      // break every shield
    }
    int speed = 0, stealth = 0;
    for (size_t i = 0; i < allies.size(); ++i) {
        assert(BuffCount(allies[i]) == 1);                      // all buffed, none twice
        if (allies[i]->regulusSpeedAge >= 0.0f) {
            ++speed;
            assert(std::fabs(allies[i]->RegulusSpeedBuff() - Monster::REGULUS_SPEED_BUFF_MULT) < 1e-4f);
        }
        if (allies[i]->regulusStealthAge >= 0.0f) {
            ++stealth;
            assert(std::fabs(allies[i]->RegulusCloakAlpha() - Monster::REGULUS_STEALTH_ALPHA) < 1e-4f);
        }
    }
    assert(speed + stealth >= 1);
    // Regulus gone: the buffs stay.
    reg->alive = false;
    Step(mm, 20.0f);
    for (size_t i = 0; i < allies.size(); ++i) assert(BuffCount(allies[i]) == 1 || firstBuff[i] == 0);
    for (auto* a : allies) {
        if (a->regulusSpeedAge >= 0.0f) assert(a->RegulusSpeedBuff() > 1.5f);
    }
    std::printf("permanent, one buff per mob: ok\n");
    mm.Clear();
}

// Target and buff choice are random, not tier or distance.
static void CheckRandomTarget() {
    std::set<int> picked;
    std::set<int> buffs;
    for (int trial = 0; trial < 80; ++trial) {
        srand(1000 + trial);
        MonsterManager mm;
        auto* reg = Add(mm, MobKind::REGULUS, 0.0f, 0.0f);
        reg->regulusDriftTimer = 1.0e6f;
        Add(mm, MobKind::ROTOR, 100.0f, 0.0f);                  // nearest, lowest tier
        Add(mm, MobKind::GRAVIS, 0.0f, 300.0f);                 // highest tier
        Add(mm, MobKind::GIMBAL, -400.0f, 0.0f);                // farthest
        Add(mm, MobKind::ROTOR, 0.0f, -200.0f);
        Step(mm, 0.51f);
        for (int i = 1; i <= 4; ++i)
            if (BuffCount(mm.monsters[i]) == 0) picked.insert(i);   // the one left out
        for (const auto& beam : mm.regulusBeams) buffs.insert(beam.buff);
        mm.Clear();
    }
    assert(picked.size() == 4);                                 // every ally gets skipped sometimes
    assert(buffs.size() == 3);
    std::printf("random target and buff: ok\n");
}

// Buff effects: speed multiplies movement, stealth only dims the drawing; both
// fade in after landing and then stay.
static void CheckBuffEffects() {
    Monster m(0.0f, 0.0f);
    m.MakeKind(MobKind::ROTOR);
    assert(!m.RegulusBuffed());
    assert(m.RegulusSpeedBuff() == 1.0f && m.RegulusCloakAlpha() == 1.0f);
    m.regulusSpeedAge = 0.0f;
    assert(m.RegulusBuffed() && m.RegulusSpeedBuff() == 1.0f);   // fade-in starts at 0
    m.regulusSpeedAge = 100.0f;
    assert(std::fabs(m.RegulusSpeedBuff() - Monster::REGULUS_SPEED_BUFF_MULT) < 1e-4f);
    assert(m.RegulusCloakAlpha() == 1.0f);
    m.regulusStealthAge = 100.0f;
    assert(std::fabs(m.RegulusCloakAlpha() - Monster::REGULUS_STEALTH_ALPHA) < 1e-4f);
    m.regulusStealthAge = Monster::REGULUS_BUFF_FADE * 0.5f;     // fading in
    assert(m.RegulusCloakAlpha() > Monster::REGULUS_STEALTH_ALPHA && m.RegulusCloakAlpha() < 1.0f);
    std::printf("buff effects ok\n");
}

int main() {
    CheckInstantGrant();
    CheckBuffColors();
    CheckVolley();
    CheckPermanentOneBuff();
    CheckRandomTarget();
    CheckBuffEffects();
    std::printf("regulus beam regression passed\n");
    return 0;
}
