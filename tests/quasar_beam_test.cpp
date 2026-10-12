// Headless regression for the Quasar beam: the axis is chosen once, at the
// start of the wind-up, and never follows the player; the beam fires
// QUASAR_LOCK_TIME after the lock and then turns at a constant slow rate;
// the damage band is QUASAR_BEAM_RADIUS wide.
// Run with scripts/test_quasar_beam.ps1.
#include "Monster.h"
#include <cassert>
#include <cmath>
#include <cstdio>

static constexpr float DT = 1.0f / 120.0f;

static Monster MakeQuasar() {
    Monster q(0.0f, 0.0f);
    q.MakeKind(MobKind::QUASAR);
    q.speed = 0.0f;
    q.quasarState = 1;            // cooldown just ended (wind-up skipped)
    q.quasarTimer = 0.0f;
    q.quasarAimX = 1.0f;
    q.quasarAimY = 0.0f;
    q.quasarAimSet = false;
    return q;
}

static float AimAngle(const Monster& q) { return atan2f(q.quasarAimY, q.quasarAimX); }

// Wind-up: drift stops, the axis is taken once at its start and then holds
// through the wind-up and the lock even though the player keeps moving.
static void CheckAimChosenOnce() {
    float hp = 1.0e9f;
    Monster q(0.0f, 0.0f);
    q.MakeKind(MobKind::QUASAR);
    q.quasarState = 0;
    q.quasarTimer = 0.0f;
    const float startPre = Monster::QUASAR_COOLDOWN_TIME - Monster::QUASAR_PRECHARGE_TIME;
    while (q.quasarTimer < startPre - 0.2f) q.Update(5000.0f, 0.0f, DT, hp);
    const float x0 = q.worldX, y0 = q.worldY;
    q.Update(5000.0f, 0.0f, DT, hp);
    assert(q.worldX != x0 || q.worldY != y0);                 // drifting
    while (q.QuasarPrecharge() <= 0.0f) q.Update(5000.0f, 0.0f, DT, hp);
    const float hx = q.worldX, hy = q.worldY;
    const float chosen = atan2f(0.0f - hy, 5000.0f - hx);
    assert(fabsf(AimAngle(q) - chosen) < 1e-4f);              // taken at wind-up start
    float py = 0.0f, held = 0.0f;
    while (q.quasarState != 3) {
        py += 500.0f * DT;                                    // player runs off
        q.Update(5000.0f, py, DT, hp);
        if (q.quasarState == 0) assert(q.worldX == hx && q.worldY == hy);
        if (q.quasarState != 3) assert(fabsf(AimAngle(q) - chosen) < 1e-5f);
        held += DT;
    }
    std::printf("aim chosen once, held %.2fs while the player moved: ok\n", held);
}

// A forced lock (no wind-up) still takes the aim, then holds it.
static void CheckForcedLock() {
    float hp = 1.0e9f;
    Monster q = MakeQuasar();
    q.Update(300.0f, 400.0f, DT, hp);
    assert(q.quasarState == 2);
    assert(fabsf(q.quasarAimX - 0.6f) < 1e-4f && fabsf(q.quasarAimY - 0.8f) < 1e-4f);
    const float locked = AimAngle(q);
    float t = 0.0f, py = 400.0f;
    while (q.quasarState == 2) {
        py += 300.0f * DT;
        q.Update(300.0f, py, DT, hp);
        t += DT;
        assert(fabsf(AimAngle(q) - locked) < 1e-5f);
    }
    assert(fabsf(t - Monster::QUASAR_LOCK_TIME) < 2.0f * DT);
    std::printf("lock: fires after %.2fs ok\n", t);
}

// Firing: a constant slow turn that ignores the player entirely, in a
// direction rolled per attack; over many attacks both directions occur.
static void CheckConstantTurnWhileFiring() {
    int cw = 0, ccw = 0;
    for (int run = 0; run < 24; ++run) {
        float hp = 1.0e9f;
        Monster q = MakeQuasar();
        while (q.quasarState != 3) q.Update(600.0f, 0.0f, DT, hp);
        float prev = AimAngle(q), turnSign = 0.0f;
        float py = 0.0f;
        while (q.quasarState == 3) {
            py += 400.0f * DT * (run % 2 ? 1.0f : -1.0f);      // player on either side
            q.Update(600.0f, py, DT, hp);
            const float a = AimAngle(q);
            const float step = a - prev;
            assert(std::fabs(std::fabs(step) - Monster::QUASAR_BEAM_TURN * DT) < 1e-4f);
            if (turnSign == 0.0f) turnSign = step > 0.0f ? 1.0f : -1.0f;
            assert(step * turnSign > 0.0f);                      // one direction per attack
            prev = a;
        }
        (turnSign > 0.0f ? cw : ccw)++;
    }
    assert(cw > 0 && ccw > 0);
    std::printf("constant turn while firing: %d one way, %d the other ok\n", cw, ccw);
}

// Damage band: inside the radius hurts at QUASAR_BEAM_DPS, outside does not.
static void CheckBand() {
    for (int inside = 0; inside <= 1; ++inside) {
        float hp = 1000.0f;   // small enough that per-frame damage registers
        Monster q = MakeQuasar();
        while (q.quasarState != 3) q.Update(600.0f, 0.0f, DT, hp);
        for (int i = 0; i < 90; ++i) q.Update(600.0f, 0.0f, DT, hp);   // reach out
        // Measure across the band at the beam's current angle.
        const float a = AimAngle(q);
        const float off = inside ? Monster::QUASAR_BEAM_RADIUS - 10.0f
                                 : Monster::QUASAR_BEAM_RADIUS + 10.0f;
        const float tx = cosf(a) * 600.0f - sinf(a) * off;
        const float ty = sinf(a) * 600.0f + cosf(a) * off;
        const float before = hp;
        for (int i = 0; i < 12; ++i) q.Update(tx, ty, DT, hp);         // 0.1s
        if (inside) assert(fabsf((before - hp) - Monster::QUASAR_BEAM_DPS * 0.1f) < 0.2f);
        else assert(hp == before);
    }
    std::printf("band: %.0f px radius, %.0f dps ok\n",
                Monster::QUASAR_BEAM_RADIUS, Monster::QUASAR_BEAM_DPS);
}

int main() {
    CheckAimChosenOnce();
    CheckForcedLock();
    CheckConstantTurnWhileFiring();
    CheckBand();
    std::printf("quasar beam regression passed\n");
    return 0;
}
