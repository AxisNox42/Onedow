// Headless regression for the Magnetar lattice ward: each of the 20
// icosahedron faces is its own shield. Run with scripts/test_magnetar_field.ps1.
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

// A fresh Magnetar whose lattice does not turn (so a side keeps its face).
static Monster StillMagnetar() {
    Monster m(0.0f, 0.0f);
    m.MakeKind(MobKind::MAGNETAR);
    m.speed = 0.0f;
    return m;
}

static int IntactFaces(const Monster& m) {
    int n = 0;
    for (float b : m.magnetarFaceBroken) if (b < 0.0f) ++n;
    return n;
}

// The N pole turns toward the player at a limited rate.
static void CheckFacing() {
    float hp = 1000.0f;
    Monster m = StillMagnetar();
    assert(m.sizeScale == 1.8f);
    m.magnetarFacing = 3.0f;
    for (float t = 0.0f; t < 1.5f; t += DT) m.Update(0.0f, 900.0f, DT, hp);
    assert(std::fabs(m.magnetarFacing - 1.5707963f) < 0.05f);
    std::printf("facing: ok\n");
}

// Every rim side maps to a face, and opposite sides map to different faces.
static void CheckFaceMapping() {
    const Monster m = StillMagnetar();
    for (int k = 0; k < 16; ++k) {
        const float a = (float)k * 0.3926991f;
        const int f = m.MagnetarFaceToward(a);
        assert(f >= 0 && f < Monster::MAGNETAR_FACES);
        assert(f != m.MagnetarFaceToward(a + 3.14159265f));
    }
    std::printf("face mapping: ok\n");
}

// Shots from the left wear down only the left face: it stops
// MAGNETAR_FACE_HITS of them, breaks, and later shots from the left pass
// through while the right side still blocks. Outside the field nothing
// happens.
static void CheckPerFaceBreak() {
    float hp = 1000.0f;
    Monster m = StillMagnetar();
    const int left = m.MagnetarFaceToward(3.14159265f);
    std::vector<Bullet> bullets;
    bullets.push_back(ShotThrough(-400.0f, 200.0f, 400.0f, 200.0f));   // outside
    m.Update(0.0f, 2000.0f, DT, hp, 1.0f, -1, -1, -1, -1, &bullets);
    assert(bullets[0].active && IntactFaces(m) == Monster::MAGNETAR_FACES);
    for (int i = 0; i < Monster::MAGNETAR_FACE_HITS; ++i) {
        bullets.assign(1, ShotThrough(-400.0f, 0.0f, 0.0f, 0.0f));
        m.Update(0.0f, 2000.0f, 0.0f, hp, 1.0f, -1, -1, -1, -1, &bullets);
        assert(!bullets[0].active);                                    // blocked
    }
    assert(m.magnetarFaceBroken[left] >= 0.0f);
    assert(IntactFaces(m) == Monster::MAGNETAR_FACES - 1);            // only that one
    bullets.assign(1, ShotThrough(-400.0f, 0.0f, 0.0f, 0.0f));
    m.Update(0.0f, 2000.0f, 0.0f, hp, 1.0f, -1, -1, -1, -1, &bullets);
    assert(bullets[0].active && bullets[0].wardPassed == &m);         // through
    m.Update(0.0f, 2000.0f, 0.0f, hp, 1.0f, -1, -1, -1, -1, &bullets);
    assert(bullets[0].active);                                         // not rechecked
    bullets.assign(1, ShotThrough(400.0f, 0.0f, 0.0f, 0.0f));
    m.Update(0.0f, 2000.0f, 0.0f, hp, 1.0f, -1, -1, -1, -1, &bullets);
    assert(!bullets[0].active);                                        // right holds
    std::printf("per-face break: face %d broke after %d shots ok\n",
                left, Monster::MAGNETAR_FACE_HITS);
}

// A broken face restores MAGNETAR_FACE_RESTORE seconds after it broke, with
// its full shot count; the restore timer restarts for the snap-back look.
static void CheckFaceRestore() {
    float hp = 1000.0f;
    std::vector<Bullet> bullets;
    Monster m = StillMagnetar();
    m.magnetarFaceHits[3] = 0;
    m.magnetarFaceBroken[3] = 0.0f;
    float t = 0.0f;
    while (m.magnetarFaceBroken[3] >= 0.0f) {
        m.Update(0.0f, 2000.0f, DT, hp, 1.0f, -1, -1, -1, -1, &bullets);
        t += DT;
        assert(t < 10.0f);
    }
    assert(std::fabs(t - Monster::MAGNETAR_FACE_RESTORE) < 2.0f * DT);
    assert(m.magnetarFaceHits[3] == Monster::MAGNETAR_FACE_HITS);
    assert(m.magnetarFaceRestored[3] < 2.0f * DT);
    std::printf("face restore: after %.2fs ok\n", t);
}

// Hit look: a stopped shot flashes only the face that stopped it, briefly.
static void CheckHitFlash() {
    float hp = 1000.0f;
    Monster m = StillMagnetar();
    std::vector<Bullet> bullets;
    bullets.push_back(ShotThrough(-300.0f, -300.0f, 0.0f, 0.0f));   // from upper left
    m.Update(0.0f, 2000.0f, DT, hp, 1.0f, -1, -1, -1, -1, &bullets);
    assert(!bullets[0].active);                                     // absorbed
    const int face = m.MagnetarFaceToward(-0.75f * 3.14159265f);
    for (int f = 0; f < Monster::MAGNETAR_FACES; ++f)
        assert((m.magnetarFaceHitFlash[f] > 0.0f) == (f == face));  // that face only
    for (float t = 0.0f; t < Monster::MAGNETAR_HIT_FLASH + 0.05f; t += DT)
        m.Update(0.0f, 2000.0f, DT, hp, 1.0f, -1, -1, -1, -1, &bullets);
    for (float flash : m.magnetarFaceHitFlash) assert(flash == 0.0f);  // gone
    std::printf("hit flash: face %d only ok\n", face);
}

int main() {
    CheckFacing();
    CheckFaceMapping();
    CheckPerFaceBreak();
    CheckFaceRestore();
    CheckHitFlash();
    std::printf("magnetar field regression passed\n");
    return 0;
}
