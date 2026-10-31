// Headless regression for the Quasar elite changes: the beam bounces off the
// arena edge (none below HARD, 1x on HARD, 2x on NIGHTMARE) and every third
// starfall shot is aimed at the player's predicted position.
// Run with scripts/test_quasar_elite.ps1.
#include "MonsterManager.h"
#include <cassert>
#include <cmath>
#include <cstdio>

static constexpr float DT = 1.0f / 120.0f;
static const glm::vec4 kArena(0.0f, 0.0f, 1000.0f, 600.0f);

static Monster MakeQuasar(float x, float y, float aimX, float aimY, int reflections) {
    Monster q(x, y);
    q.MakeKind(MobKind::QUASAR);
    q.speed = 0.0f;
    const float len = std::sqrt(aimX * aimX + aimY * aimY);
    q.quasarAimX = aimX / len;
    q.quasarAimY = aimY / len;
    q.quasarAimSet = true;
    q.quasarReflections = reflections;
    q.quasarBounds = kArena;
    return q;
}

static bool Near(float a, float b, float eps = 0.05f) { return std::fabs(a - b) < eps; }

static void CheckReflectionsByDifficulty() {
    assert(Monster::QuasarReflectionsFor(0) == 0);   // EASY
    assert(Monster::QuasarReflectionsFor(1) == 0);   // NORMAL
    assert(Monster::QuasarReflectionsFor(2) == 1);   // HARD
    assert(Monster::QuasarReflectionsFor(3) == 2);   // NIGHTMARE
    std::printf("reflections per difficulty ok\n");
}

// Both ends bounce; legs are chained, mirrored at the arena edge, and the
// total length per end stays the reach.
static void CheckSegments() {
    std::array<glm::vec4, Monster::QUASAR_BEAM_MAX_SEGMENTS> seg;
    std::array<int, Monster::QUASAR_BEAM_MAX_SEGMENTS> bnc{};
    // No reflection: one straight leg per end.
    Monster none = MakeQuasar(500.0f, 300.0f, 1.0f, 0.0f, 0);
    assert(none.QuasarBeamSegments(6000.0f, seg, &bnc) == 2);
    assert(Near(seg[0].z, 500.0f - 6000.0f) && Near(seg[1].z, 500.0f + 6000.0f));
    // One bounce, diagonal at 45 degrees from the arena centre.
    Monster one = MakeQuasar(500.0f, 300.0f, 1.0f, 1.0f, 1);
    const int n1 = one.QuasarBeamSegments(6000.0f, seg, &bnc);
    assert(n1 == 4);
    assert(bnc[0] == 0 && bnc[1] == 1 && bnc[2] == 0 && bnc[3] == 1);
    // End 1 (sgn -1 is first) travels up-left in screen terms: the top wall y=0 is
    // nearer than the left wall, so it bounces at (200, 0).
    assert(Near(seg[0].z, 200.0f, 0.1f) && Near(seg[0].w, 0.0f, 0.1f));
    // After the bounce y flips (now increasing) while x keeps decreasing.
    assert(seg[1].z < seg[1].x && seg[1].w > seg[1].y);
    // Chained: the next leg starts where the previous one ended.
    assert(Near(seg[1].x, seg[0].z) && Near(seg[1].y, seg[0].w));
    // Total travelled length per end equals the reach.
    const auto len = [](const glm::vec4& s) { return std::hypot(s.z - s.x, s.w - s.y); };
    assert(Near(len(seg[0]) + len(seg[1]), 6000.0f, 1.0f));
    // Two bounces: three legs per end, six in total.
    Monster two = MakeQuasar(500.0f, 300.0f, 1.0f, 0.3f, 2);
    assert(two.QuasarBeamSegments(6000.0f, seg, &bnc) == 6);
    assert(bnc[0] == 0 && bnc[1] == 1 && bnc[2] == 2);
    // A short reach that never reaches the wall adds no extra legs.
    Monster shortReach = MakeQuasar(500.0f, 300.0f, 1.0f, 0.0f, 2);
    assert(shortReach.QuasarBeamSegments(100.0f, seg, &bnc) == 2);
    // Without an arena there is nothing to bounce off.
    Monster open = MakeQuasar(500.0f, 300.0f, 1.0f, 1.0f, 2);
    open.quasarBounds = glm::vec4(0.0f);
    assert(open.QuasarBeamSegments(6000.0f, seg, &bnc) == 2);
    std::printf("beam segments ok\n");
}

// The reflected leg hurts: stand on it and only a reflecting beam hits.
static void CheckReflectedLegHurts() {
    std::array<glm::vec4, Monster::QUASAR_BEAM_MAX_SEGMENTS> seg;
    for (int reflections = 0; reflections <= 2; ++reflections) {
        Monster q = MakeQuasar(500.0f, 300.0f, 1.0f, 1.0f, 1);
        q.QuasarBeamSegments(6000.0f, seg);
        // 150 px past the first bounce (close, so the slow beam turn cannot matter).
        const glm::vec2 from(seg[1].x, seg[1].y), to(seg[1].z, seg[1].w);
        const glm::vec2 on = from + glm::normalize(to - from) * 150.0f;
        const float px = on.x, py = on.y;
        Monster fire = MakeQuasar(500.0f, 300.0f, 1.0f, 1.0f, reflections);
        fire.quasarState = 3;
        fire.quasarTimer = 0.7f;           // fully grown
        float hp = 1000.0f;
        for (int i = 0; i < 60; ++i)
            fire.Update(px, py, DT, hp, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, nullptr, kArena);
        if (reflections == 0) assert(hp == 1000.0f);   // no reflection, nothing there
        else assert(hp < 1000.0f);
    }
    std::printf("reflected leg damage ok\n");
}

// The player's velocity is estimated from consecutive frames and capped.
static void CheckVelocityEstimate() {
    Monster q = MakeQuasar(0.0f, 0.0f, 1.0f, 0.0f, 0);
    q.quasarState = 0;
    float hp = 1.0e9f;
    float px = 3000.0f;
    for (int i = 0; i < 240; ++i) {                   // 2 s at 200 px/s
        px += 200.0f * DT;
        q.Update(px, 0.0f, DT, hp);
    }
    assert(Near(q.quasarPlayerVel.x, 200.0f, 5.0f) && Near(q.quasarPlayerVel.y, 0.0f, 1.0f));
    for (int i = 0; i < 240; ++i) {                   // farQ faster than the cap
        px += 5000.0f * DT;
        q.Update(px, 0.0f, DT, hp);
    }
    assert(q.quasarPlayerVel.x <= Monster::QUASAR_PREDICT_MAX_SPEED + 1.0f);
    std::printf("player velocity estimate ok\n");
}

// Every 3rd starfall shot leads the player by a capped vector; the others are scattered.
static void CheckPredictedShots() {
    Monster q = MakeQuasar(0.0f, 0.0f, 1.0f, 0.0f, 0);
    q.quasarPlayerVel = glm::vec2(300.0f, -100.0f);
    const float px = 2000.0f, py = 1500.0f;
    q.BeginQuasarFall();
    for (int i = 0; i < Monster::QUASAR_FALL_COUNT; ++i) {
        q.LaunchQuasarFall(px, py, glm::vec4(0.0f));
        const bool predicted = (q.quasarFallPredictedMask >> i) & 1u;
        assert(predicted == (i % 3 == 2));
        if (predicted) {
            // |v| = 316 px/s x 0.5 s = 158 px, capped to 140 px along the heading.
            const glm::vec2 lead = q.quasarFallTargets[i] - glm::vec2(px, py);
            assert(Near(glm::length(lead), Monster::QUASAR_PREDICT_MAX_LEAD, 0.1f));
            const glm::vec2 dir = glm::normalize(glm::vec2(300.0f, -100.0f));
            assert(Near(lead.x, dir.x * 140.0f, 0.1f) && Near(lead.y, dir.y * 140.0f, 0.1f));
        } else {
            const glm::vec2 d = q.quasarFallTargets[i] - glm::vec2(px, py);
            assert(glm::length(d) <= 180.5f);          // the old scatter, around the player
        }
    }
    int count = 0;
    for (int i = 0; i < 32; ++i) count += (q.quasarFallPredictedMask >> i) & 1u;
    assert(count == Monster::QUASAR_FALL_COUNT / 3);
    // A slow walk leads by speed x lead time only: 100 px/s -> 50 px, same heading.
    Monster slow = MakeQuasar(0.0f, 0.0f, 1.0f, 0.0f, 0);
    slow.quasarPlayerVel = glm::vec2(0.0f, 100.0f);
    slow.BeginQuasarFall();
    for (int i = 0; i < 3; ++i) slow.LaunchQuasarFall(px, py, glm::vec4(0.0f));
    assert(Near(slow.quasarFallTargets[2].x, px) && Near(slow.quasarFallTargets[2].y, py + 50.0f));
    // A standing player gets the shot right on them.
    Monster still = MakeQuasar(0.0f, 0.0f, 1.0f, 0.0f, 0);
    still.quasarPlayerVel = glm::vec2(20.0f, 0.0f);          // below the minimum speed
    still.BeginQuasarFall();
    for (int i = 0; i < 3; ++i) still.LaunchQuasarFall(px, py, glm::vec4(0.0f));
    assert(Near(still.quasarFallTargets[2].x, px) && Near(still.quasarFallTargets[2].y, py));
    // A new fall clears the marks.
    q.BeginQuasarFall();
    assert(q.quasarFallPredictedMask == 0);
    std::printf("predicted shots ok\n");
}

// Starfall power: wider and harder the farther the landing point is from the Quasar.
static void CheckDistancePower() {
    Monster q = MakeQuasar(0.0f, 0.0f, 1.0f, 0.0f, 0);
    q.BeginQuasarFall();
    // Scaling by power: 0 -> base, 1 -> max, in between linear.
    q.quasarFallPower[0] = 0.0f; q.quasarFallPower[1] = 0.5f; q.quasarFallPower[2] = 1.0f;
    assert(Near(q.QuasarFallRadius(0), Monster::QUASAR_FALL_RADIUS));
    assert(Near(q.QuasarFallDamage(0), Monster::QUASAR_FALL_DAMAGE));
    assert(Near(q.QuasarFallRadius(2), Monster::QUASAR_FALL_RADIUS * Monster::QUASAR_FALL_MAX_RADIUS_SCALE));
    assert(Near(q.QuasarFallDamage(2), Monster::QUASAR_FALL_DAMAGE * Monster::QUASAR_FALL_MAX_DAMAGE_SCALE));
    assert(q.QuasarFallRadius(1) > q.QuasarFallRadius(0) && q.QuasarFallRadius(1) < q.QuasarFallRadius(2));
    assert(q.QuasarFallDamage(1) > q.QuasarFallDamage(0) && q.QuasarFallDamage(1) < q.QuasarFallDamage(2));
    // No slowdown near the Quasar: every shot has the same warning whatever its power.
    assert(Near(q.QuasarFallWarning(0), Monster::QUASAR_FALL_WARNING));
    assert(Near(q.QuasarFallWarning(1), Monster::QUASAR_FALL_WARNING));
    assert(Near(q.QuasarFallWarning(2), Monster::QUASAR_FALL_WARNING));
    q.quasarFallLaunchTimes[2] = 2.0f;
    assert(Near(q.QuasarFallImpactTime(2), 2.0f + Monster::QUASAR_FALL_WARNING));
    // Launch derives power from the distance to the Quasar.
    Monster nearQ = MakeQuasar(0.0f, 0.0f, 1.0f, 0.0f, 0);
    nearQ.BeginQuasarFall();
    nearQ.LaunchQuasarFall(40.0f, 0.0f, glm::vec4(0.0f));      // lands within ~220 px at most
    assert(nearQ.quasarFallPower[0] < 0.2f);
    Monster farQ = MakeQuasar(0.0f, 0.0f, 1.0f, 0.0f, 0);
    farQ.BeginQuasarFall();
    for (int i = 0; i < 3; ++i) farQ.LaunchQuasarFall(1500.0f, 0.0f, glm::vec4(0.0f));
    for (int i = 0; i < 3; ++i) assert(farQ.quasarFallPower[i] == 1.0f);
    // A farQ impact reaches a player 100 px away and hurts for the scaled damage;
    // a near one (radius 64) does not reach the same spot.
    for (int strong = 0; strong <= 1; ++strong) {
        Monster s = MakeQuasar(0.0f, 0.0f, 1.0f, 0.0f, 0);
        s.BeginQuasarFall();
        s.quasarFallFired = 1;
        s.quasarFallLaunchTimes[0] = 0.0f;
        s.quasarFallTargets[0] = glm::vec2(500.0f, 0.0f);
        s.quasarFallPower[0] = strong ? 1.0f : 0.0f;
        s.quasarFallTargetsMask = 1u;
        s.quasarTimer = s.QuasarFallImpactTime(0);
        float hp = 100.0f;
        s.UpdateQuasarFall(600.0f, 0.0f, hp, glm::vec4(0.0f));
        if (strong) assert(Near(100.0f - hp, Monster::QUASAR_FALL_DAMAGE * Monster::QUASAR_FALL_MAX_DAMAGE_SCALE, 0.01f));
        else assert(hp == 100.0f);
    }
    std::printf("distance power ok\n");
}

// Starfall cadence: a random 0.1-0.3 s gap, the same wherever the player stands.
static void CheckFallGap() {
    float mn = 1e9f, mx = 0.0f, sum = 0.0f;
    const int N = 4000;
    for (int i = 0; i < N; ++i) {
        const float g = Monster::QuasarFallNextGap();
        mn = std::min(mn, g); mx = std::max(mx, g); sum += g;
    }
    assert(mn >= Monster::QUASAR_FALL_GAP_MIN - 1e-4f && mx <= Monster::QUASAR_FALL_GAP_MAX + 1e-4f);
    assert(mn < 0.11f && mx > 0.29f);                    // real spread over the whole range
    assert(Near(sum / N, 0.2f, 0.01f));
    std::printf("fall gap variance ok\n");
}

// Eruption: concentric zone rings around the Quasar, doors in every ring, safe
// lanes between rings, everything detonates together after the warning.
static const glm::vec4 kBigArena(0.0f, 0.0f, 1280.0f, 720.0f);

static Monster MakeEruptionQuasar() {
    Monster q = MakeQuasar(640.0f, 360.0f, 1.0f, 0.0f, 0);
    q.quasarBounds = kBigArena;
    q.quasarTimer = -1000.0f;       // no attack of its own: these tests look at the eruption alone
    q.BeginQuasarEruption();
    return q;
}

static void CheckEruptionZones() {
    for (int trial = 0; trial < 20; ++trial) {
        srand(500 + trial);
        Monster q = MakeEruptionQuasar();
        assert(q.quasarEruptActive);
        int ringZones = 0, laneZones = 0, bodyZones = 0;
        for (int i = 0; i < q.quasarEruptCount; ++i) {
            if (q.quasarEruptBeat[i] == 0) ++ringZones;
            else if (q.quasarEruptRing[i] == Monster::QUASAR_ERUPT_CENTER_RING) ++bodyZones;
            else ++laneZones;
        }
        assert(ringZones >= 12 && ringZones <= 110);  // three rings around the Quasar, not the whole map
        assert(laneZones >= 10 && bodyZones == 1);    // beat 1: the lane band and one zone on the body
        const glm::vec2 c(640.0f, 360.0f);
        float nearest = 1.0e9f;
        for (int i = 0; i < q.quasarEruptCount; ++i) {
            const glm::vec2 z = q.quasarEruptZones[i];
            assert(glm::length(z - glm::vec2(640.0f, 360.0f)) <= Monster::QUASAR_ERUPT_MAX_REACH);   // only around the Quasar
            assert(z.x >= kBigArena.x - 40.0f && z.x <= kBigArena.z + 40.0f);          // inside the arena (+ slack)
            assert(z.y >= kBigArena.y - 40.0f && z.y <= kBigArena.w + 40.0f);
            if (q.quasarEruptBeat[i] == 0) nearest = std::min(nearest, glm::length(z - c));
            assert(q.QuasarEruptCovers(z));                                      // a zone covers its own centre
        }
        assert(Near(nearest, Monster::QUASAR_ERUPT_FIRST_RING, 0.5f));          // the guard ring
        // The pocket beside the body and the lanes between rings are safe all around.
        for (int a = 0; a < 360; a += 3) {
            const glm::vec2 dir(cosf(a * 0.0174533f), sinf(a * 0.0174533f));
            assert(!q.QuasarEruptCovers(c + dir * 100.0f, 0));
            for (int k = 0; k < 2; ++k) {
                const float lane = Monster::QUASAR_ERUPT_FIRST_RING + Monster::QUASAR_ERUPT_RING_STEP * ((float)k + 0.5f);
                assert(!q.QuasarEruptCovers(c + dir * lane, 0));
            }
        }
        // Every ring that exists has a door: some angle on it is uncovered.
        for (int k = 0; k < 2; ++k) {
            const float r = Monster::QUASAR_ERUPT_FIRST_RING + Monster::QUASAR_ERUPT_RING_STEP * (float)k;
            bool anyZone = false, anyDoor = false;
            for (int a = 0; a < 720; ++a) {
                const glm::vec2 dir(cosf(a * 0.0087266f), sinf(a * 0.0087266f));
                const bool covered = q.QuasarEruptCovers(c + dir * r, 0);
                anyZone |= covered; anyDoor |= !covered;
            }
            assert(anyDoor);
            (void)anyZone;
        }
    }
    // Beat 1 covers the body and every lane zone centre, and leaves the ring ground and the pocket alone.
    {
        srand(77);
        Monster q = MakeEruptionQuasar();
        const glm::vec2 c(640.0f, 360.0f);
        assert(q.QuasarEruptCovers(c, 1));
        assert(q.QuasarEruptCovers(c + glm::vec2(Monster::QUASAR_ERUPT_CENTER_RADIUS - 2.0f, 0.0f), 1));
        for (int a = 0; a < 360; a += 3) {
            const glm::vec2 dir(cosf(a * 0.0174533f), sinf(a * 0.0174533f));
            assert(!q.QuasarEruptCovers(c + dir * Monster::QUASAR_ERUPT_FIRST_RING, 1));   // beat 0's ring ground is safe for beat 1
            assert(!q.QuasarEruptCovers(c + dir * 120.0f, 1));                            // the pocket stays safe
        }
        for (int i = 0; i < q.quasarEruptCount; ++i)
            if (q.quasarEruptBeat[i] == 1) assert(q.QuasarEruptCovers(q.quasarEruptZones[i], 1));
    }
    std::printf("eruption zones ok\n");
}

static void CheckEruptionTiming() {
    srand(11);
    Monster q = MakeEruptionQuasar();
    const glm::vec2 c(640.0f, 360.0f);
    // Stand on a zone (hurt) and on a lane (safe).
    const glm::vec2 hit = q.quasarEruptZones[0];
    const glm::vec2 lane = c + glm::vec2(1.0f, 0.0f) * (Monster::QUASAR_ERUPT_FIRST_RING + Monster::QUASAR_ERUPT_RING_STEP * 0.5f);
    float hpHit = 100.0f, hpSafe = 100.0f;
    Monster qs = q;
    for (float t = 0.0f; t < Monster::QUASAR_ERUPT_BLAST_TIME - 0.02f; t += DT) {
        q.Update(hit.x, hit.y, DT, hpHit, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, nullptr, kBigArena);
        qs.Update(lane.x, lane.y, DT, hpSafe, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, nullptr, kArena);
    }
    assert(hpHit == 100.0f && hpSafe == 100.0f);              // the whole warning is harmless
    assert(!q.quasarEruptBlasted);
    for (int i = 0; i < 12; ++i) {
        q.Update(hit.x, hit.y, DT, hpHit, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, nullptr, kBigArena);
        qs.Update(lane.x, lane.y, DT, hpSafe, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, nullptr, kArena);
    }
    assert(q.quasarEruptBlasted);
    assert(Near(100.0f - hpHit, Monster::QUASAR_ERUPT_DAMAGE, 0.01f));    // one hit, not one per zone
    assert(hpSafe == 100.0f);
    // It goes off exactly once, then the Quasar recovers and returns to normal.
    for (float t = 0.0f; t < 3.0f; t += DT)
        q.Update(hit.x, hit.y, DT, hpHit, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, nullptr, kArena);
    assert(Near(100.0f - hpHit, Monster::QUASAR_ERUPT_DAMAGE, 0.01f));
    assert(q.quasarState == 0 && !q.quasarNextFall && !q.quasarEruptActive && q.quasarEruptCooldown > 0.0f);
    std::printf("eruption timing ok\n");
}

// Two beats: the lane is safe for the first blast and hit by the second; a zone of
// the first beat is hit once by the first only; the pocket is never hit.
static void CheckEruptionTwoBeats() {
    srand(21);
    Monster base = MakeEruptionQuasar();
    glm::vec2 lanePos(0.0f), ringPos(0.0f);
    for (int i = 0; i < base.quasarEruptCount; ++i) {
        if (base.quasarEruptBeat[i] == 1 && base.quasarEruptRing[i] != Monster::QUASAR_ERUPT_CENTER_RING && lanePos == glm::vec2(0.0f))
            lanePos = base.quasarEruptZones[i];
        if (base.quasarEruptBeat[i] == 0 && ringPos == glm::vec2(0.0f)) ringPos = base.quasarEruptZones[i];
    }
    assert(!base.QuasarEruptCovers(lanePos, 0) && base.QuasarEruptCovers(lanePos, 1));
    assert(base.QuasarEruptCovers(ringPos, 0) && !base.QuasarEruptCovers(ringPos, 1));
    Monster ql = base, qr = base;
    float hpLane = 100.0f, hpRing = 100.0f;
    const auto step = [&](float until) {
        while (ql.quasarEruptTimer < until) {
            ql.Update(lanePos.x, lanePos.y, DT, hpLane, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, nullptr, kBigArena);
            qr.Update(ringPos.x, ringPos.y, DT, hpRing, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, nullptr, kBigArena);
        }
    };
    step(Monster::QUASAR_ERUPT_BLAST_TIME + 0.1f);                       // just after the first blast
    assert(hpLane == 100.0f && Near(100.0f - hpRing, Monster::QUASAR_ERUPT_DAMAGE, 0.01f));
    step(Monster::QUASAR_ERUPT_SECOND_BLAST - 0.05f);                    // still before the second
    assert(hpLane == 100.0f);
    step(Monster::QUASAR_ERUPT_SECOND_BLAST + 0.1f);
    assert(Near(100.0f - hpLane, Monster::QUASAR_ERUPT_DAMAGE, 0.01f));  // the old safe spot is the new danger
    assert(Near(100.0f - hpRing, Monster::QUASAR_ERUPT_DAMAGE, 0.01f));  // the ring ground is safe the second time
    assert(Monster::QUASAR_ERUPT_SECOND_SHOW + 1.0f <= Monster::QUASAR_ERUPT_SECOND_BLAST);   // beat 1 is up for >= 1 s
    std::printf("eruption two beats ok\n");
}

// Staging: the wind-up has no zones, rings unfold inside-out, and every zone is
// up for at least QUASAR_ERUPT_MIN_WARNING before the blast (also in a big arena).
static void CheckEruptionStaging() {
    for (const glm::vec4 arena : { glm::vec4(0.0f, 0.0f, 1280.0f, 720.0f), glm::vec4(0.0f, 0.0f, 1920.0f, 1080.0f),
                                   glm::vec4(0.0f, 0.0f, 2560.0f, 1440.0f) }) {
        srand(3);
        Monster q = MakeQuasar(arena.z * 0.5f, arena.w * 0.5f, 1.0f, 0.0f, 0);
        q.quasarBounds = arena;
        q.BeginQuasarEruption();
        assert(q.quasarEruptCount > 0);
        int lastRing = -1;
        float lastShow = -1.0f;
        for (int i = 0; i < q.quasarEruptCount; ++i) {
            const int ring = q.quasarEruptRing[i];
            const float show = q.QuasarEruptZoneShow(i);
            assert(show >= Monster::QUASAR_ERUPT_WINDUP - 1e-4f);                     // nothing during the wind-up
            assert(q.QuasarEruptZoneBlast(i) - show >= Monster::QUASAR_ERUPT_MIN_WARNING - 1e-3f);
            if (q.quasarEruptBeat[i] == 1) continue;                                  // beat 1 is listed after the rings
            assert(ring >= lastRing);                                                 // built from the inside out
            if (ring > lastRing && lastRing >= 0) assert(show > lastShow);            // later rings start later
            lastRing = ring; lastShow = show;
        }
    }
    // The body heats up as the blast nears.
    Monster b = MakeEruptionQuasar();
    b.quasarEruptTimer = 0.2f;
    const float h0 = b.QuasarHeat();
    b.quasarEruptTimer = 1.0f;
    const float h1 = b.QuasarHeat();
    b.quasarEruptTimer = Monster::QUASAR_ERUPT_BLAST_TIME;
    assert(h0 < h1 && h1 < b.QuasarHeat() && b.QuasarHeat() > 0.8f);
    std::printf("eruption staging ok\n");
}

// The rotation is beam, starfall, beam, starfall ... (the eruption is not in it) and
// a player who stays away never sees an eruption.
static void CheckSpecialAlternation() {
    Monster q = MakeQuasar(640.0f, 360.0f, 1.0f, 0.0f, 0);
    q.quasarState = 0;
    q.quasarAimSet = false;
    float hp = 1.0e9f;
    const auto stepUntil = [&](auto done) {
        for (float t = 0.0f; t < 40.0f && !done(); t += DT)
            q.Update(100.0f, 100.0f, DT, hp, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, nullptr, kArena);
        assert(done());
    };
    for (int cycle = 0; cycle < 4; ++cycle) {
        stepUntil([&] { return q.quasarState == 3; });                    // beam
        stepUntil([&] { return q.quasarState == Monster::QUASAR_FALL_STATE; });
        assert(!q.quasarEruptActive);                                     // a far player never triggers it
        stepUntil([&] { return q.quasarState == 0 && !q.quasarNextFall; });
    }
    std::printf("rotation is beam / starfall only ok\n");
}

// The eruption is conditional: the player has to stay close for the trigger time.
static void CheckEruptionCondition() {
    const glm::vec2 c(640.0f, 360.0f);
    const auto mk = [&]() {
        Monster q = MakeQuasar(c.x, c.y, 1.0f, 0.0f, 0);
        q.quasarState = 0; q.quasarAimSet = false; q.quasarBounds = kBigArena;
        q.quasarNextFall = true;        // a starfall is next: no beam wind-up in the way, so only the condition counts
        q.quasarTimer = 0.0f;           // (MakeKind starts it at a random 0-1.2 s)
        return q;
    };
    const auto run = [&](Monster& q, glm::vec2 player, float seconds, float& hp) {
        for (float t = 0.0f; t < seconds; t += DT)
            q.Update(player.x, player.y, DT, hp, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, nullptr, kBigArena);
    };
    const float T = Monster::QUASAR_ERUPT_TRIGGER_TIME;
    const glm::vec2 closeP(c.x + 300.0f, c.y), awayP(c.x + 800.0f, c.y);
    float hp = 1.0e9f;
    // Close for the trigger time -> the eruption starts, and not before.
    {
        Monster q = mk();
        run(q, closeP, T - 0.1f, hp);
        assert(!q.quasarEruptActive);
        run(q, closeP, 0.25f, hp);
        assert(q.quasarEruptActive && q.quasarEruptCount > 0);
    }
    // Staying away: never.
    {
        Monster q = mk();
        run(q, awayP, 12.0f, hp);
        assert(!q.quasarEruptActive && q.quasarNearTimer == 0.0f);
    }
    // Leaving drains the timer 1:1, so edge-dancing does not dodge it for free...
    {
        Monster q = mk();
        run(q, closeP, 0.8f, hp);
        run(q, awayP, 0.2f, hp);
        assert(Near(q.quasarNearTimer, 0.6f, 0.03f));
        run(q, closeP, 0.7f, hp);                    // 0.6 + 0.7 = 1.3 s of stay: not yet
        assert(!q.quasarEruptActive);
        run(q, closeP, 0.3f, hp);
        assert(q.quasarEruptActive);
    }
    // ...and a long absence resets it completely.
    {
        Monster q = mk();
        run(q, closeP, 1.2f, hp);
        run(q, awayP, 1.5f, hp);
        assert(q.quasarNearTimer == 0.0f);
    }
    // It is an overlay: with the timer full during a beam or a starfall it goes off at
    // once, and the attack underneath runs its course untouched.
    {
        Monster q = mk();
        q.quasarState = 3; q.quasarTimer = 0.1f; q.quasarNearTimer = T;
        q.Update(closeP.x, closeP.y, DT, hp, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, nullptr, kBigArena);
        assert(q.quasarEruptActive && q.quasarState == 3);                      // both at the same time
        float t = 0.0f;
        while (q.quasarState == 3 && t < 6.0f) { q.Update(closeP.x, closeP.y, DT, hp, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, nullptr, kBigArena); t += DT; }
        assert(q.quasarState == 4 && t > Monster::QUASAR_BEAM_TIME - 0.5f);     // the beam ran its full course
        assert(q.quasarEruptTimer > Monster::QUASAR_ERUPT_BLAST_TIME - 0.01f);  // the eruption kept its own clock
    }
    {
        Monster q = mk();
        q.BeginQuasarFall(); q.quasarNearTimer = T;
        q.Update(closeP.x, closeP.y, DT, hp, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, nullptr, kBigArena);
        assert(q.quasarEruptActive && q.quasarState == Monster::QUASAR_FALL_STATE);
        const float fallStart = q.quasarTimer;
        assert(fallStart < 0.1f);                                               // the starfall did not restart or stop
    }
    // The Quasar stays put while an eruption runs, even idle.
    {
        Monster q = mk();
        q.speed = 80.0f;
        q.quasarNearTimer = T;
        const glm::vec2 at(q.worldX, q.worldY);
        run(q, closeP, 2.0f, hp);
        assert(q.quasarEruptActive || q.quasarEruptCooldown > 0.0f);
        assert(q.QuasarHoldsPosition() == q.quasarEruptActive);
        assert(glm::length(glm::vec2(q.worldX, q.worldY) - at) < 1.0f);
    }
    // After an eruption there is a cooldown, even for a player who stays.
    {
        Monster q = mk();
        float firstStart = -1.0f, secondStart = -1.0f, t = 0.0f;
        bool inErupt = false;
        while (t < 40.0f && secondStart < 0.0f) {
            q.Update(closeP.x, closeP.y, DT, hp, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, nullptr, kBigArena);
            const bool now = q.quasarEruptActive;
            if (now && !inErupt) { if (firstStart < 0.0f) firstStart = t; else secondStart = t; }
            inErupt = now;
            t += DT;
        }
        assert(firstStart > 0.0f && secondStart > firstStart);
        assert(secondStart - firstStart >= Monster::QUASAR_ERUPT_SECOND_BLAST + Monster::QUASAR_ERUPT_COOLDOWN);
    }
    std::printf("eruption condition ok\n");
}

// The beam thins out instead of cutting off: full width until the last
// QUASAR_BEAM_FADE_OUT, then an eased fall to 0, and the hit width follows it.
static void CheckBeamFadeOut() {
    Monster q = MakeQuasar(0.0f, 0.0f, 1.0f, 0.0f, 0);
    q.quasarState = 3;
    const float start = Monster::QUASAR_BEAM_TIME - Monster::QUASAR_BEAM_FADE_OUT;
    q.quasarTimer = 0.0f;                    assert(q.QuasarBeamWidth() == 1.0f);
    q.quasarTimer = start;                   assert(Near(q.QuasarBeamWidth(), 1.0f, 1e-4f));
    q.quasarTimer = Monster::QUASAR_BEAM_TIME; assert(Near(q.QuasarBeamWidth(), 0.0f, 1e-4f));
    float prev = 1.0f;
    for (float t = start; t <= Monster::QUASAR_BEAM_TIME + 1e-4f; t += 0.01f) {   // never rises, never jumps
        q.quasarTimer = t;
        const float w = q.QuasarBeamWidth();
        assert(w <= prev + 1e-5f && prev - w < 0.12f);
        prev = w;
    }
    // A player 45 px off the axis is inside the band (55 px) at full width but
    // out of the beam once it has thinned below 45/55.
    for (int late = 0; late <= 1; ++late) {
        Monster f = MakeQuasar(0.0f, 0.0f, 1.0f, 0.0f, 0);
        f.quasarState = 3;
        f.quasarTimer = late ? Monster::QUASAR_BEAM_TIME - 0.06f : 1.0f;
        float hp = 1000.0f;
        f.Update(500.0f, 45.0f, DT, hp, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, nullptr, kArena);
        if (late) assert(hp == 1000.0f); else assert(hp < 1000.0f);
    }
    std::printf("beam fade-out width ok\n");
}

// A Quasar in its attack (wind-up to recovery) cannot be moved by anything:
// crowding by an equal-tier enemy pushes the other one, never the Quasar. Before the
// attack, while it drifts, it still gives way like any other enemy.
static void CheckHoldsPosition() {
    for (int phase = 0; phase < 3; ++phase) {
        MonsterManager mm;
        auto* q = new Monster(500.0f, 300.0f);
        q->MakeKind(MobKind::QUASAR);
        q->speed = 0.0f;
        q->quasarAimSet = true;
        if (phase == 0) { q->quasarState = 3; q->quasarTimer = 0.2f; }                 // beam growing
        if (phase == 1) { q->quasarState = Monster::QUASAR_FALL_STATE; q->quasarTimer = 0.2f; }
        if (phase == 2) { q->quasarState = 0; q->quasarTimer = 0.0f; q->quasarNextFall = true; }   // drifting
        auto* g = new Monster(506.0f, 300.0f);                 // an equal-tier Genesis sitting on it
        g->MakeKind(MobKind::GENESIS);
        g->speed = 0.0f;
        mm.monsters.push_back(q);
        mm.monsters.push_back(g);
        assert(q->QuasarHoldsPosition() == (phase != 2));
        const float qx = q->worldX, qy = q->worldY;
        std::vector<Bullet> bullets;
        float hp = 1.0e9f;
        for (int i = 0; i < 40; ++i) {
            mm.UpdateAll(5000.0f, 5000.0f, DT, hp, bullets);
            if (phase != 2) assert(q->quasarState != 0 || q->QuasarPrecharge() > 0.0f);   // still holding
        }
        if (phase != 2) assert(q->worldX == qx && q->worldY == qy);   // not a pixel
        else assert(std::fabs(q->worldX - qx) > 0.5f);               // a drifting Quasar is pushed as before
        mm.Clear();
    }
    // The wind-up counts as holding too: it stops drifting and cannot be pushed.
    Monster w(0.0f, 0.0f);
    w.MakeKind(MobKind::QUASAR);
    w.quasarState = 0;
    w.quasarTimer = Monster::QUASAR_COOLDOWN_TIME - Monster::QUASAR_PRECHARGE_TIME + 0.2f;
    assert(w.QuasarPrecharge() > 0.0f && w.QuasarHoldsPosition());
    w.quasarTimer = 0.5f;
    assert(!w.QuasarHoldsPosition());
    std::printf("quasar holds position during its attack ok\n");
}

int main() {
    CheckReflectionsByDifficulty();
    CheckSegments();
    CheckReflectedLegHurts();
    CheckVelocityEstimate();
    CheckPredictedShots();
    CheckDistancePower();
    CheckBeamFadeOut();
    CheckHoldsPosition();
    CheckFallGap();
    CheckEruptionZones();
    CheckEruptionTiming();
    CheckEruptionTwoBeats();
    CheckEruptionStaging();
    CheckSpecialAlternation();
    CheckEruptionCondition();
    std::printf("quasar elite regression passed\n");
    return 0;
}
