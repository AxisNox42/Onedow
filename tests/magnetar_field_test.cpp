// Headless regression for the Magnetar v3 field coil and pole-cell stacks
// (Docs/Astral_Enemy_Blueprints_v3.md 3). Run with
// scripts/test_magnetar_field.ps1.
#include "Monster.h"
#include <cassert>
#include <cmath>
#include <cstdio>

static constexpr float DT = 1.0f / 120.0f;

static Bullet ShotThrough(float fromX, float fromY, float toX, float toY) {
    Bullet b(toX, toY, toX + 1.0f, toY);
    b.prevX = fromX;
    b.prevY = fromY;
    b.x = toX;
    b.y = toY;
    return b;
}

// The N pole turns toward the player at a limited rate.
static void CheckFacing() {
    float hp = 1000.0f;
    Monster m(0.0f, 0.0f);
    m.MakeKind(MobKind::MAGNETAR);
    assert(m.sizeScale == 1.8f);
    m.speed = 0.0f;
    m.magnetarFacing = 3.0f;
    for (float t = 0.0f; t < 1.5f; t += DT) m.Update(0.0f, 900.0f, DT, hp);
    assert(std::fabs(m.magnetarFacing - 1.5707963f) < 0.05f);
    std::printf("facing: ok\n");
}

// The field shows while at least one stack is ready, vanishes at zero, and
// unfolds again (timer restarts) when the first stack returns.
static void CheckFieldVisibility() {
    float hp = 1000.0f;
    std::vector<Bullet> bullets;
    Monster m(0.0f, 0.0f);
    m.MakeKind(MobKind::MAGNETAR);
    m.speed = 0.0f;
    m.Update(0.0f, 2000.0f, DT, hp, 1.0f, -1, -1, -1, -1, &bullets);
    assert(m.magnetarFieldTimer > 1.0f);                 // full field at spawn
    m.magnetarStacks = 0.5f;
    m.Update(0.0f, 2000.0f, DT, hp, 1.0f, -1, -1, -1, -1, &bullets);
    assert(m.magnetarStacks < 1.0f && m.magnetarFieldTimer == 0.0f);   // hidden
    while (m.magnetarStacks < 1.0f)
        m.Update(0.0f, 2000.0f, DT, hp, 1.0f, -1, -1, -1, -1, &bullets);
    assert(m.magnetarFieldTimer < 0.05f);                // just unfolding
    for (float t = 0.0f; t < 0.5f; t += DT)
        m.Update(0.0f, 2000.0f, DT, hp, 1.0f, -1, -1, -1, -1, &bullets);
    assert(m.magnetarFieldTimer > 0.35f);                // fully open
    std::printf("field visibility: ok\n");
}

// Interception is unchanged: inside 145px it costs one stack per shot, never
// goes below zero, and nothing is blocked without a ready stack.
static void CheckInterception() {
    float hp = 1000.0f;
    Monster m(0.0f, 0.0f);
    m.MakeKind(MobKind::MAGNETAR);
    m.speed = 0.0f;
    m.magnetarRegenRate = 0.0f;
    std::vector<Bullet> bullets;
    bullets.push_back(ShotThrough(-400.0f, 200.0f, 400.0f, 200.0f));   // outside
    m.Update(0.0f, 2000.0f, DT, hp, 1.0f, -1, -1, -1, -1, &bullets);
    assert(bullets[0].active && m.magnetarStacks == 8.0f);
    bullets.clear();
    for (int i = 0; i < 10; ++i)
        bullets.push_back(ShotThrough(-400.0f, 10.0f * i, 400.0f, 10.0f * i));
    m.Update(0.0f, 2000.0f, DT, hp, 1.0f, -1, -1, -1, -1, &bullets);
    int blocked = 0;
    for (auto& b : bullets) if (!b.active) ++blocked;
    assert(blocked == 8);                                // only 8 stacks
    assert(m.magnetarStacks >= 0.0f && m.magnetarStacks < 1.0f);
    assert(m.magnetarInterceptTimer > 0.0f);
    std::printf("interception: blocked %d of 10 ok\n", blocked);
}

// Deflect look: a shot coming down-right from the upper left records a flash
// on the upper-left rim and a mirrored outgoing direction pointing back away
// from the field; the shot itself is still absorbed (visual only).
static void CheckDeflectLook() {
    float hp = 1000.0f;
    Monster m(0.0f, 0.0f);
    m.MakeKind(MobKind::MAGNETAR);
    m.speed = 0.0f;
    m.magnetarRegenRate = 0.0f;
    std::vector<Bullet> bullets;
    bullets.push_back(ShotThrough(-300.0f, -300.0f, 0.0f, 0.0f));   // heading +x +y
    m.Update(0.0f, 2000.0f, DT, hp, 1.0f, -1, -1, -1, -1, &bullets);
    assert(!bullets[0].active && m.magnetarStacks == 7.0f);         // absorbed
    int live = 0;
    glm::vec3 d{};
    for (const auto& s : m.magnetarDeflects) if (s.z > 0.0f) { ++live; d = s; }
    assert(live == 1);                                              // one flash
    const float pi = 3.14159265f;
    assert(std::fabs(d.x - (-0.75f * pi)) < 0.01f);                // upper-left rim
    // Head-on shot reflects straight back out along the rim normal.
    assert(std::fabs(std::cos(d.y - d.x) - 1.0f) < 0.01f);
    for (float t = 0.0f; t < Monster::MAGNETAR_DEFLECT_TIME + 0.05f; t += DT)
        m.Update(0.0f, 2000.0f, DT, hp, 1.0f, -1, -1, -1, -1, &bullets);
    for (const auto& s : m.magnetarDeflects) assert(s.z == 0.0f);  // flash gone
    std::printf("deflect look: rim %.2f rad, out %.2f rad ok\n", d.x, d.y);
}

// Full refill: once the last stack is spent nothing trickles back; all 8
// return at once after MAGNETAR_RECHARGE_TIME alone, faster with support but
// never under MAGNETAR_RECHARGE_MIN.
static void CheckFullRefill() {
    for (float support : {0.3f, 0.6f, 1.2f}) {
        float hp = 1000.0f;
        std::vector<Bullet> bullets;
        Monster m(0.0f, 0.0f);
        m.MakeKind(MobKind::MAGNETAR);
        m.speed = 0.0f;
        m.magnetarRegenRate = support;
        m.magnetarStacks = 0.0f;
        const float want = std::max(Monster::MAGNETAR_RECHARGE_MIN,
                                    Monster::MAGNETAR_RECHARGE_TIME * 0.3f / support);
        float t = 0.0f;
        while (m.magnetarStacks < 1.0f) {
            m.Update(0.0f, 2000.0f, DT, hp, 1.0f, -1, -1, -1, -1, &bullets);
            t += DT;
            if (m.magnetarStacks < 1.0f) assert(m.magnetarStacks == 0.0f);  // no trickle
            assert(t < 10.0f);
        }
        assert(m.magnetarStacks == (float)Monster::MAGNETAR_MAX_STACKS);   // all at once
        assert(std::fabs(t - want) < 2.0f * DT);
        std::printf("full refill (support %.1f): 8 stacks after %.2fs ok\n", support, t);
    }
}

int main() {
    CheckFullRefill();
    CheckFacing();
    CheckDeflectLook();
    CheckFieldVisibility();
    CheckInterception();
    std::printf("magnetar field regression passed\n");
    return 0;
}
