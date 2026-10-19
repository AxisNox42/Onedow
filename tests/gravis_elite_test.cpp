// Run scripts/test_gravis_elite.ps1. Gameplay-level regressions for the Gravis
// elite: two independent machines - the orbital sling (satellites) and the
// compression glide (body) - and how they behave beside each other.
#include "MonsterManager.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>

static constexpr float DT = 1.0f / 120.0f;
static const glm::vec4 ARENA(0, 0, 1200, 800);
static constexpr float PARKED = -1.0e6f;

// A Gravis with both machines parked (they wait forever); tests start the
// machine under test by hand or release it by zeroing its timer.
static Monster MakeGravis(float speed = 0.0f) {
    Monster g(200, 400);
    g.MakeKind(MobKind::GRAVIS);
    g.speed = speed;
    g.contactDmg = 0.0f;   // no contact damage: isolate the patterns
    g.gravisTimer = PARKED;
    g.gravisSlingTimer = PARKED;
    return g;
}
static void Tick(Monster& g, glm::vec2 p, float& hp, float dt = DT, glm::vec4 bounds = ARENA) {
    g.Update(p.x, p.y, dt, hp, 1, -1, -1, -1, -1, nullptr, bounds);
}
static bool Near(float a, float b, float eps) { return std::fabs(a - b) <= eps; }

// Runs until the sling state changes; returns how long the old state lasted.
static float RunSling(Monster& g, glm::vec2 p, float& hp) {
    const int from = g.gravisSling;
    float t = 0.0f;
    for (int i = 0; i < 6000 && g.gravisSling == from; ++i) { Tick(g, p, hp); t += DT; }
    return t;
}
static float RunBody(Monster& g, glm::vec2 p, float& hp) {
    const int from = g.gravisState;
    float t = 0.0f;
    for (int i = 0; i < 4000 && g.gravisState == from; ++i) { Tick(g, p, hp); t += DT; }
    return t;
}

int main() {
    std::srand(1234);
    g_PlayerDmgMult = 1;
    g_PlayerShield = 0;
    const glm::vec2 player(600, 400);

    // Sling timeline on its own: cooldown, wind-up, lock, throw, harass, return.
    {
        auto g = MakeGravis();
        g.gravisSlingTimer = 0.0f;
        float hp = 1000;
        assert(g.gravisSling == Monster::GRAVIS_IDLE && g.GravisFieldActive());
        assert(Near(RunSling(g, player, hp), 3.0f, 0.02f));
        assert(g.gravisSling == Monster::GRAVIS_SLING_PREP && g.GravisFieldActive());
        assert(Near(RunSling(g, player, hp), 0.65f, 0.02f));
        assert(g.gravisSling == Monster::GRAVIS_SLING_LOCK);
        assert(Near(RunSling(g, player, hp), 0.4f, 0.02f));
        assert(g.gravisSling == Monster::GRAVIS_SLING_OUT);
        assert(Near(RunSling(g, player, hp), 1.1f, 0.02f));
        assert(g.gravisSling == Monster::GRAVIS_SLING_HARASS);
        assert(Near(RunSling(g, player, hp), 9.2f, 0.02f));
        assert(g.gravisSling == Monster::GRAVIS_SLING_BACK);
        const float back = RunSling(g, player, hp);
        assert(back > 0.3f && back <= Monster::GRAVIS_BACK_TIMEOUT + 0.02f);
        assert(g.gravisSling == Monster::GRAVIS_SLING_RECOVER);
        assert(Near(RunSling(g, player, hp), 0.35f, 0.02f));
        assert(g.gravisSling == Monster::GRAVIS_IDLE);
        std::printf("sling timeline ok (return %.2fs)\n", back);
    }

    // Glide timeline on its own; the field is off exactly while the body winds
    // up, glides and bursts.
    {
        auto g = MakeGravis();
        g.gravisTimer = 0.0f;
        float hp = 1000;
        assert(g.gravisState == Monster::GRAVIS_IDLE && g.GravisFieldActive());
        assert(Near(RunBody(g, player, hp), 1.8f, 0.02f));
        assert(g.gravisState == Monster::GRAVIS_GLIDE_COMPRESS && !g.GravisFieldActive());
        assert(Near(RunBody(g, player, hp), 0.6f, 0.02f));
        assert(g.gravisState == Monster::GRAVIS_GLIDE_LANE && !g.GravisFieldActive());
        assert(Near(RunBody(g, player, hp), 0.45f, 0.02f));
        assert(g.gravisState == Monster::GRAVIS_GLIDE_RUN && !g.GravisFieldActive());
        assert(Near(RunBody(g, player, hp), 0.45f, 0.02f));
        assert(g.gravisState == Monster::GRAVIS_GLIDE_BURST && !g.GravisFieldActive());
        assert(Near(RunBody(g, player, hp), 0.3f, 0.02f));
        assert(g.gravisState == Monster::GRAVIS_GLIDE_RECOVER && g.GravisFieldActive());
        assert(Near(RunBody(g, player, hp), 0.5f, 0.02f));
        assert(g.gravisState == Monster::GRAVIS_IDLE);
        std::printf("glide timeline ok\n");
    }

    // Independence: with both machines running, Gravis keeps gliding while its
    // satellites are out, the field follows the body alone, and the sling never
    // starts again until it is over.
    {
        auto g = MakeGravis();
        g.gravisTimer = g.gravisSlingTimer = 0.0f;
        float hp = 1000;
        int overlapSteps = 0, slings = 0, glides = 0;
        int lastSling = g.gravisSling, lastBody = g.gravisState;
        for (int i = 0; i < 120 * 60; ++i) {
            Tick(g, player, hp);
            if (g.gravisSling != lastSling) {
                if (lastSling == Monster::GRAVIS_IDLE) ++slings;
                lastSling = g.gravisSling;
            }
            if (g.gravisState != lastBody) {
                if (lastBody == Monster::GRAVIS_IDLE) ++glides;
                lastBody = g.gravisState;
            }
            if (g.gravisSling == Monster::GRAVIS_SLING_HARASS && g.gravisState >= Monster::GRAVIS_GLIDE_COMPRESS &&
                g.gravisState <= Monster::GRAVIS_GLIDE_BURST) {
                ++overlapSteps;
                assert(!g.GravisFieldActive());   // the field follows the body, not the sling
            }
            if (g.gravisSling == Monster::GRAVIS_SLING_HARASS && g.gravisState == Monster::GRAVIS_IDLE)
                assert(g.GravisFieldActive());
        }
        assert(slings >= 3 && glides >= 10);
        assert(overlapSteps > 120);   // Gravis really glides while the satellites harass
        std::printf("independent machines ok (%d slings, %d glides, %d overlapping steps)\n", slings, glides, overlapSteps);
    }

    // Own business: with only the sling running Gravis drifts through every
    // sling state; with only the glide running it holds still for the wind-up,
    // run and burst and drifts otherwise.
    {
        auto g = MakeGravis(150.0f);
        g.gravisSlingTimer = 0.0f;
        float hp = 1000;
        glm::vec2 last(g.worldX, g.worldY);
        float travelled[12] = {};
        for (int i = 0; i < 120 * 16 && !(g.gravisSling == Monster::GRAVIS_IDLE && i > 600); ++i) {
            Tick(g, player, hp);
            const glm::vec2 now(g.worldX, g.worldY);
            travelled[g.gravisSling] += glm::length(now - last);
            last = now;
        }
        assert(travelled[Monster::GRAVIS_SLING_PREP] > 5.0f && travelled[Monster::GRAVIS_SLING_LOCK] > 3.0f);
        assert(travelled[Monster::GRAVIS_SLING_OUT] > 20.0f && travelled[Monster::GRAVIS_SLING_HARASS] > 100.0f);
        assert(travelled[Monster::GRAVIS_SLING_BACK] > 5.0f);
        auto h = MakeGravis(150.0f);
        h.BeginGravisGlide();
        h.gravisState = Monster::GRAVIS_GLIDE_COMPRESS;
        glm::vec2 hl(h.worldX, h.worldY);
        float still = 0.0f, drifting = 0.0f;
        for (int i = 0; i < 120 * 4; ++i) {
            const int before = h.gravisState;
            Tick(h, player, hp);
            const glm::vec2 now(h.worldX, h.worldY);
            const float d = glm::length(now - hl);
            hl = now;
            if (before == Monster::GRAVIS_GLIDE_COMPRESS || before == Monster::GRAVIS_GLIDE_LANE) still += d;
            if (before == Monster::GRAVIS_GLIDE_RECOVER) drifting += d;
        }
        assert(still < 0.001f && drifting > 1.0f);
        std::printf("Gravis drifts during the sling and holds still to glide ok\n");
    }

    // Sling lock: direction and range are read once and never follow the
    // player; the range tracks the distance within its limits.
    {
        for (const float dist : {100.0f, 500.0f, 3000.0f}) {
            auto g = MakeGravis();
            float hp = 1000;
            g.BeginGravisSling();
            const glm::vec2 target = glm::vec2(g.worldX, g.worldY) + glm::vec2(dist, 0);
            while (g.gravisSling == Monster::GRAVIS_SLING_PREP) Tick(g, target, hp, DT, glm::vec4(0));
            const glm::vec2 aim = g.gravisAim;
            const float range = g.gravisSlingRange;
            assert(Near(aim.x, 1.0f, 0.001f) && Near(aim.y, 0.0f, 0.001f));
            assert(range >= Monster::GRAVIS_SLING_RANGE_MIN && range <= Monster::GRAVIS_SLING_RANGE_MAX);
            assert(Near(range, std::clamp(dist + 120.0f, 360.0f, 900.0f), 0.01f));
            for (int i = 0; i < 200; ++i) {
                Tick(g, {300.0f + i, 100.0f + i * 2.0f}, hp, DT, glm::vec4(0));
                assert(g.gravisAim == aim && g.gravisSlingRange == range);
            }
        }
        std::printf("sling lock ok\n");
    }

    // Sling damage: 7 when a thrown mass touches the player on the way out
    // (once); every harass dart that reaches a player left standing hurts for 4.
    {
        auto g = MakeGravis();
        float hp = 1000;
        g.BeginGravisSling();
        const glm::vec2 stand(600, 400);
        int outHits = 0, dartHits = 0, backHits = 0;
        for (int i = 0; i < 4000 && g.gravisSling != Monster::GRAVIS_IDLE; ++i) {
            const float before = hp;
            Tick(g, stand, hp);
            if (hp < before) {
                if (g.gravisSling == Monster::GRAVIS_SLING_OUT) { assert(Near(before - hp, 7.0f, 0.001f)); ++outHits; }
                else if (g.gravisSling == Monster::GRAVIS_SLING_HARASS) { assert(Near(before - hp, 4.0f, 0.001f)); ++dartHits; }
                else if (g.gravisSling == Monster::GRAVIS_SLING_BACK) { assert(Near(before - hp, 7.0f, 0.001f)); ++backHits; }
                else assert(false);
            }
        }
        assert(outHits == 1);
        // Curved darts aimed at a standing player are all hits, as many as the
        // cooldown and the range allowed.
        assert(dartHits >= 6 && dartHits <= Monster::GRAVIS_DART_PER_SAT * Monster::GRAVIS_SLING_COUNT);
        assert(backHits <= Monster::GRAVIS_SLING_COUNT);
        std::printf("sling hits: out %d, darts %d, return %d ok\n", outHits, dartHits, backHits);
    }

    // Natural harass motion: the masses leave the throw with its momentum (no
    // snap), never keep a constant distance, and the centre follows with a lag.
    // Dart schedule: cooldown between darts and only within range.
    {
        auto g = MakeGravis();
        float hp = 1000;
        g.BeginGravisSling();
        const glm::vec2 lockAt(600, 400);
        while (g.gravisSling != Monster::GRAVIS_SLING_HARASS) Tick(g, lockAt, hp);
        std::array<glm::vec2, 3> last = g.gravisSatPos;
        float maxStep = 0.0f, minR = 1e9f, maxR = 0.0f, maxLag = 0.0f, t = 0.0f;
        glm::vec2 p = lockAt, v(0, 330);
        std::array<float, 3> firstAnnounce = {-1, -1, -1}, flightEnd = {-1, -1, -1};
        std::array<int, 3> prevState = {0, 0, 0};
        int announced = 0, farAnnounce = 0;
        float minCooldown = 1e9f;
        while (g.gravisSling == Monster::GRAVIS_SLING_HARASS) {
            p += v * DT;
            if (p.y > 700 || p.y < 100) v = -v;
            const std::array<glm::vec2, 3> before = g.gravisSatPos;
            Tick(g, p, hp);
            t += DT;
            maxLag = std::max(maxLag, glm::length(g.gravisHarassCenter - p));
            for (int i = 0; i < 3; ++i) {
                maxStep = std::max(maxStep, glm::length(g.gravisSatPos[i] - last[i]));
                last[i] = g.gravisSatPos[i];
                if (t > 1.5f) {
                    const float r = glm::length(g.gravisSatPos[i] - g.gravisHarassCenter);
                    minR = std::min(minR, r);
                    maxR = std::max(maxR, r);
                }
                if (prevState[i] == 0 && g.gravisDartState[i] == 1) {
                    ++announced;
                    if (firstAnnounce[i] < 0) firstAnnounce[i] = t;
                    // Only within range of the player, and only after the cooldown.
                    if (glm::length(p - before[i]) > Monster::GRAVIS_DART_RANGE + 8.0f) ++farAnnounce;
                    if (flightEnd[i] >= 0) minCooldown = std::min(minCooldown, t - flightEnd[i]);
                }
                if (prevState[i] == 2 && g.gravisDartState[i] == 0) flightEnd[i] = t;
                prevState[i] = g.gravisDartState[i];
            }
        }
        assert(maxStep < 11.0f);         // continuous, bounded speed (no snapping)
        assert(maxR - minR > 80.0f);     // the orbit breathes: no fixed distance
        assert(maxLag > 60.0f);          // a running player opens the ring
        assert(announced >= 4);
        assert(farAnnounce == 0);        // never announced from beyond the range
        assert(minCooldown >= Monster::GRAVIS_DART_COOLDOWN - 0.02f);
        for (int i = 0; i < 3; ++i)
            assert(firstAnnounce[i] >= Monster::GRAVIS_DART_START + Monster::GRAVIS_DART_STAGGER * i - 0.03f);
        std::printf("harass motion continuous (max step %.1f px, radius %.0f-%.0f, lag %.0f), %d darts, min cooldown %.2fs ok\n",
                    maxStep, minR, maxR, maxLag, announced, minCooldown);
    }

    // The dart is a curve: it flies the arc the telegraph drew, bending through
    // the aim point and on well past it, and the aim leads a moving player.
    {
        auto g = MakeGravis();
        float hp = 1000;
        g.BeginGravisSling();
        const glm::vec2 lockAt(600, 400);
        while (g.gravisSling != Monster::GRAVIS_SLING_HARASS) Tick(g, lockAt, hp);
        glm::vec2 p = lockAt, v(120, 160);
        std::array<int, 3> prevState = {0, 0, 0};
        std::array<Monster::GravisDartPath, 3> shown{};
        std::array<float, 3> closest{}, bend{};
        std::array<glm::vec2, 3> aim{};
        std::array<glm::vec2, 3> endPt{};
        int darts = 0, finished = 0, curved = 0;
        while (g.gravisSling == Monster::GRAVIS_SLING_HARASS) {
            p += v * DT;
            if (p.x > 900 || p.x < 300) v.x = -v.x;
            if (p.y > 650 || p.y < 150) v.y = -v.y;
            std::array<Monster::GravisDartPath, 3> planned{};
            for (int i = 0; i < 3; ++i) planned[i] = g.GravisDartPlan(i);   // what the telegraph shows
            Tick(g, p, hp);
            for (int i = 0; i < 3; ++i) {
                const int st = g.gravisDartState[i];
                if (prevState[i] == 1 && st == 2) {
                    ++darts;
                    const auto& path = g.gravisDartPath[i];
                    // The arc drawn just before launch is the arc flown.
                    assert(glm::length(path.from - planned[i].from) < 8.0f);
                    assert(std::fabs(path.theta0 - planned[i].theta0) < 0.04f);
                    assert(std::fabs(path.length - planned[i].length) < 12.0f);
                    aim[i] = g.gravisDartTarget[i];
                    // It runs well past the aim point (or to the arena edge).
                    assert(path.length - glm::length(aim[i] - path.from) > 40.0f);
                    assert(Near(std::fabs(path.kappa), 1.0f / Monster::GRAVIS_DART_CURVE_RADIUS, 1e-6f));
                    closest[i] = 1e9f;
                    bend[i] = 0.0f;
                    endPt[i] = Monster::GravisDartAt(path, path.length);
                }
                if (st == 2) {
                    const auto& path = g.gravisDartPath[i];
                    // The mass sits exactly on the arc at its progress.
                    assert(glm::length(g.gravisSatPos[i] - Monster::GravisDartAt(path, g.gravisDartProgress[i])) < 0.01f);
                    closest[i] = std::min(closest[i], glm::length(g.gravisSatPos[i] - aim[i]));
                    // Sideways bulge of the flown arc away from the straight chord.
                    const glm::vec2 chord = glm::normalize(endPt[i] - path.from);
                    const glm::vec2 off = g.gravisSatPos[i] - path.from;
                    bend[i] = std::max(bend[i], std::fabs(off.x * chord.y - off.y * chord.x));
                }
                if (prevState[i] == 2 && st == 0) {
                    ++finished;
                    assert(closest[i] < 15.0f);   // it crossed the aim point
                    if (bend[i] > 40.0f) ++curved;
                }
                prevState[i] = st;
            }
        }
        assert(darts >= 4 && finished >= darts - 3);
        assert(curved >= finished - 1);   // the dart really bends (almost always more than 40px off the chord)
        std::printf("darts fly the announced curve through the point and beyond ok (%d darts, %d curved)\n", darts, curved);
    }
    {
        auto g = MakeGravis();
        float hp = 1000;
        g.BeginGravisSling();
        const glm::vec2 lockAt(600, 400);
        while (g.gravisSling != Monster::GRAVIS_SLING_HARASS) Tick(g, lockAt, hp);
        glm::vec2 p = lockAt;
        const glm::vec2 vel(240, 0);
        while (g.gravisSling == Monster::GRAVIS_SLING_HARASS && g.gravisDartState[0] != 1) {
            p += vel * DT;
            if (p.x > 1000) p.x = 1000;
            Tick(g, p, hp);
        }
        const float lead = glm::dot(g.gravisDartTarget[0] - p, glm::vec2(1, 0));
        assert(lead > 30.0f && lead <= Monster::GRAVIS_DART_LEAD_MAX + 1.0f);
        auto h = MakeGravis();
        float hp2 = 1000;
        h.BeginGravisSling();
        while (h.gravisSling != Monster::GRAVIS_SLING_HARASS) Tick(h, lockAt, hp2);
        while (h.gravisSling == Monster::GRAVIS_SLING_HARASS && h.gravisDartState[0] != 1) Tick(h, lockAt, hp2);
        assert(glm::length(h.gravisDartTarget[0] - lockAt) < 3.0f);
        std::printf("dart aim leads a moving player (%.0f px) and is exact on a still one ok\n", lead);
    }

    // Range gate: while the masses are far from the player they never announce;
    // once they close in they do, and never from beyond the range.
    {
        auto g = MakeGravis();
        float hp = 1000;
        g.BeginGravisSling();
        const glm::vec2 stand(600, 400);
        while (g.gravisSling != Monster::GRAVIS_SLING_HARASS) Tick(g, stand, hp);
        float t = 0.0f;
        int farAnnounce = 0, announced = 0, announcedWhileHeldFar = 0;
        std::array<int, 3> prevState = {0, 0, 0};
        while (g.gravisSling == Monster::GRAVIS_SLING_HARASS) {
            const bool heldFar = t < 2.4f;
            if (heldFar)
                for (int i = 0; i < 3; ++i) {   // keep them well out of range
                    g.gravisSatPos[i] = stand + glm::vec2(1400.0f + 60.0f * i, 0);
                    g.gravisSatVel[i] = glm::vec2(0);
                }
            const std::array<glm::vec2, 3> before = g.gravisSatPos;
            Tick(g, stand, hp);
            t += DT;
            for (int i = 0; i < 3; ++i) {
                if (prevState[i] == 0 && g.gravisDartState[i] == 1) {
                    ++announced;
                    if (heldFar) ++announcedWhileHeldFar;
                    if (glm::length(stand - before[i]) > Monster::GRAVIS_DART_RANGE + 8.0f) ++farAnnounce;
                }
                prevState[i] = g.gravisDartState[i];
            }
        }
        assert(announcedWhileHeldFar == 0);
        assert(farAnnounce == 0);
        assert(announced >= 1);   // once they closed in they attacked
        std::printf("range gate ok (%d darts after closing in, none from afar)\n", announced);
    }

    // Wind-up: instead of freezing, the mass is drawn back along the very line
    // it will fly, ends coiled and still, then is released with a snap.
    {
        auto g = MakeGravis();
        float hp = 1000;
        g.BeginGravisSling();
        const glm::vec2 stand(600, 400);
        while (g.gravisSling != Monster::GRAVIS_SLING_HARASS) Tick(g, stand, hp);
        int watched = -1;
        glm::vec2 anchor, target;
        float maxBack = 0.0f, lateral = 0.0f;
        while (g.gravisSling == Monster::GRAVIS_SLING_HARASS && g.gravisDartState[0] != 2) {
            Tick(g, stand, hp);
            if (g.gravisDartState[0] == 1) {
                if (watched < 0) { watched = 1; anchor = g.gravisDartAnchor[0]; target = g.gravisDartTarget[0]; }
                const glm::vec2 line = glm::normalize(target - anchor);
                const glm::vec2 off = g.gravisSatPos[0] - anchor;
                lateral = std::max(lateral, std::fabs(off.x * line.y - off.y * line.x));
                maxBack = std::max(maxBack, -glm::dot(off, line));
                // The drawn line (from the mass to the aim point) keeps its direction.
                const glm::vec2 shown = glm::normalize(target - g.gravisSatPos[0]);
                assert(glm::dot(shown, line) > 0.9999f);
            }
        }
        assert(watched == 1);
        assert(Near(maxBack, Monster::GRAVIS_DART_PULLBACK, 2.5f));   // drawn all the way back
        assert(lateral < 0.5f);                                       // along the line only
        assert(glm::length(g.gravisSatVel[0]) < 400.0f);
        Tick(g, stand, hp);
        assert(g.gravisDartState[0] == 2 && g.gravisDartSpeedNow[0] >= Monster::GRAVIS_DART_LAUNCH_SPEED);
        std::printf("wind-up draws the mass back along the line (%.0f px) and releases ok\n", maxBack);
    }

    // A dart only hurts while it is thrown, never during the announcement.
    {
        auto g = MakeGravis();
        float hp = 1000;
        g.BeginGravisSling();
        const glm::vec2 stand(600, 400);
        while (g.gravisSling != Monster::GRAVIS_SLING_HARASS) Tick(g, stand, hp);
        while (g.gravisSling == Monster::GRAVIS_SLING_HARASS) {
            bool wasFlying = false;
            for (int i = 0; i < 3; ++i) wasFlying = wasFlying || g.GravisDartFlying(i);
            const float before = hp;
            Tick(g, stand, hp);
            if (hp < before) {
                bool inFlight = wasFlying;
                for (int i = 0; i < 3; ++i) inFlight = inFlight || g.GravisDartFlying(i);
                assert(inFlight);
            }
        }
        std::printf("darts hurt only in flight ok\n");
    }

    // The return: after the harass the masses are drawn back to the body on a
    // smooth looping arc (continuous speed, no snap), dock on arrival, and
    // follow a body that is moving.
    for (const float bodySpeed : {0.0f, 150.0f}) {
        auto g = MakeGravis(bodySpeed);
        float hp = 1000;
        g.BeginGravisSling();
        const glm::vec2 stand(600, 400);
        // The game loop keeps Gravis on screen; do the same here.
        auto keepOnScreen = [&]() {
            if (g.worldX < 150 || g.worldX > 1050) g.gravisDriftAngle = 3.14159265f - g.gravisDriftAngle;
            if (g.worldY < 150 || g.worldY > 650) g.gravisDriftAngle = -g.gravisDriftAngle;
            g.worldX = std::clamp(g.worldX, 150.0f, 1050.0f);
            g.worldY = std::clamp(g.worldY, 150.0f, 650.0f);
        };
        while (g.gravisSling != Monster::GRAVIS_SLING_BACK) { Tick(g, stand, hp); keepOnScreen(); }
        std::array<glm::vec2, 3> last = g.gravisSatPos;
        std::array<glm::vec2, 3> lastVel = g.gravisSatVel;
        float maxStep = 0.0f, maxTurn = 0.0f, t = 0.0f;
        int guard = 0;
        while (g.gravisSling == Monster::GRAVIS_SLING_BACK && guard++ < 120 * 5) {
            Tick(g, {1100, 100}, hp);   // the player is out of the way
            keepOnScreen();
            t += DT;
            for (int i = 0; i < 3; ++i) {
                if (g.gravisSatDocked & (1u << i)) continue;
                maxStep = std::max(maxStep, glm::length(g.gravisSatPos[i] - last[i]));
                // The velocity direction changes smoothly (no snapping back).
                const float a = glm::length(g.gravisSatVel[i]), b = glm::length(lastVel[i]);
                if (a > 30.0f && b > 30.0f) {
                    const float c = glm::dot(g.gravisSatVel[i], lastVel[i]) / (a * b);
                    maxTurn = std::max(maxTurn, acosf(std::clamp(c, -1.0f, 1.0f)));
                }
                last[i] = g.gravisSatPos[i];
                lastVel[i] = g.gravisSatVel[i];
            }
        }
        assert(g.gravisSling == Monster::GRAVIS_SLING_RECOVER);
        assert(g.gravisSatDocked == 7u);
        assert(maxStep < 9.0f);      // bounded speed
        assert(maxTurn < 0.25f);     // at most ~14 degrees of turn per step
        assert(t <= Monster::GRAVIS_BACK_TIMEOUT + 0.05f);
        std::printf("return arc smooth, docks on the body (body speed %.0f: %.2fs, max turn %.3f rad)\n", bodySpeed, t, maxTurn);
    }

    // The sling stays inside the arena once the player is.
    {
        auto g = MakeGravis();
        float hp = 1000;
        g.BeginGravisSling();
        const glm::vec2 corner(1150, 100);
        while (g.gravisSling == Monster::GRAVIS_SLING_PREP) Tick(g, corner, hp);
        const glm::vec2 end = g.GravisSatOut(1.0f);
        assert(end.x <= 1200.5f && end.y >= -0.5f);
        std::printf("sling arena bound ok\n");
    }

    // Glide: the destination is the lock-time player position (length 260..700,
    // inside the arena) and never follows afterwards.
    {
        auto g = MakeGravis();
        float hp = 1000;
        g.BeginGravisGlide();
        const glm::vec2 lockAt(650, 400);   // 450 away
        while (g.gravisState == Monster::GRAVIS_GLIDE_COMPRESS) Tick(g, lockAt, hp);
        assert(g.gravisState == Monster::GRAVIS_GLIDE_LANE);
        assert(Near(g.gravisGlideDest.x, 650.0f, 0.5f) && Near(g.gravisGlideDest.y, 400.0f, 0.5f));
        const glm::vec2 dest = g.gravisGlideDest;
        for (int i = 0; i < 100; ++i) {
            Tick(g, {900.0f, 700.0f - i}, hp);
            assert(g.gravisGlideDest == dest);
        }
        for (const float d : {50.0f, 2000.0f}) {
            auto h = MakeGravis();
            h.BeginGravisGlide();
            const glm::vec2 at = glm::vec2(h.worldX, h.worldY) + glm::vec2(d, 0);
            while (h.gravisState == Monster::GRAVIS_GLIDE_COMPRESS) Tick(h, at, hp, DT, glm::vec4(0));
            assert(Near(glm::length(h.gravisGlideDest - glm::vec2(200, 400)),
                        std::clamp(d, 260.0f, 700.0f), 0.5f));
        }
        auto e = MakeGravis();
        e.BeginGravisGlide();
        while (e.gravisState == Monster::GRAVIS_GLIDE_COMPRESS) Tick(e, {1195, 795}, hp);
        assert(e.gravisGlideDest.x <= 1200 - 92 + 0.01f && e.gravisGlideDest.y <= 800 - 92 + 0.01f);
        std::printf("glide destination lock ok\n");
    }

    // Glide path: the body reaches the destination exactly, takes no new damage
    // on the way (contact only, off here), and bursts only at the end.
    {
        auto g = MakeGravis();
        float hp = 1000;
        g.BeginGravisGlide();
        const glm::vec2 lockAt(650, 400);
        while (g.gravisState != Monster::GRAVIS_GLIDE_RUN) Tick(g, lockAt, hp);
        const glm::vec2 dest = g.gravisGlideDest;
        const glm::vec2 onPath(380, 400);
        float last = g.worldX;
        while (g.gravisState == Monster::GRAVIS_GLIDE_RUN) {
            Tick(g, onPath, hp);
            assert(g.worldX >= last - 0.001f);
            last = g.worldX;
            assert(!g.gravisBlasted || g.gravisState == Monster::GRAVIS_GLIDE_BURST);
        }
        assert(g.gravisState == Monster::GRAVIS_GLIDE_BURST);
        assert(Near(g.worldX, dest.x, 0.01f) && Near(g.worldY, dest.y, 0.01f));
        assert(hp == 1000);
        assert(g.gravisSlowRequest == 0);
        std::printf("glide path ok\n");
    }

    // Arrival burst: inside the fixed circle the player takes 8 and is slowed.
    // There is no shove: the player stays where they are, inside the field.
    for (const float inside : {0.0f, 70.0f, 149.0f}) {
        auto g = MakeGravis();
        float hp = 1000;
        g.BeginGravisGlide();
        const glm::vec2 lockAt(650, 400);
        while (g.gravisState != Monster::GRAVIS_GLIDE_RUN) Tick(g, lockAt, hp);
        const glm::vec2 dest = g.gravisGlideDest;
        const glm::vec2 stand = dest + glm::vec2(0, -inside);
        while (g.gravisState == Monster::GRAVIS_GLIDE_RUN) Tick(g, stand, hp);
        assert(Near(1000 - hp, 8.0f, 0.001f));
        assert(Near(g.gravisSlowRequest, 1.5f, 0.0001f));
        assert(glm::length(stand - dest) < GravisField::RADIUS);   // still inside the field
        const float after = hp;
        for (int i = 0; i < 200; ++i) Tick(g, stand, hp);
        assert(hp == after);
        std::printf("arrival burst inside r=%.0f ok (damage + slow, no shove)\n", inside);
    }
    {
        auto g = MakeGravis();
        float hp = 1000;
        g.BeginGravisGlide();
        const glm::vec2 lockAt(650, 400);
        while (g.gravisState != Monster::GRAVIS_GLIDE_RUN) Tick(g, lockAt, hp);
        const glm::vec2 stand = g.gravisGlideDest + glm::vec2(0, -151);
        while (g.gravisState == Monster::GRAVIS_GLIDE_RUN) Tick(g, stand, hp);
        assert(hp == 1000 && g.gravisSlowRequest == 0);
        std::printf("outside the circle is safe ok\n");
    }
    {
        auto g = MakeGravis();
        float hp = 1000;
        g.BeginGravisGlide();
        const glm::vec2 lockAt(650, 400);
        while (g.gravisState != Monster::GRAVIS_GLIDE_LANE) Tick(g, lockAt, hp);
        const glm::vec2 dest = g.gravisGlideDest;
        while (g.gravisState != Monster::GRAVIS_GLIDE_RECOVER) Tick(g, dest + glm::vec2(0, -210), hp);
        assert(hp == 1000);
        std::printf("dodging the circle ok\n");
    }
    {
        // After the burst the field is back and still holds the player: the
        // pull and the slow work together (the slow lasts 1.5s).
        auto g = MakeGravis();
        float hp = 1000;
        g.BeginGravisGlide();
        const glm::vec2 lockAt(650, 400);
        while (g.gravisState != Monster::GRAVIS_GLIDE_RUN) Tick(g, lockAt, hp);
        const glm::vec2 stand = g.gravisGlideDest + glm::vec2(100, 0);
        while (g.gravisState != Monster::GRAVIS_GLIDE_RECOVER) Tick(g, stand, hp);
        assert(g.GravisFieldActive());
        assert(glm::length(stand - glm::vec2(g.worldX, g.worldY)) < GravisField::RADIUS);
        std::printf("field returns around the player after the burst ok\n");
    }

    // The sling throws three masses in a fan, each with its own pass.
    {
        auto g = MakeGravis();
        float hp = 1000;
        g.BeginGravisSling();
        const glm::vec2 lockAt(600, 400);
        while (g.gravisSling == Monster::GRAVIS_SLING_PREP) Tick(g, lockAt, hp);
        for (int i = 0; i < Monster::GRAVIS_SLING_COUNT; ++i) {
            const glm::vec2 end = g.GravisSatOut(1.0f, i);
            const glm::vec2 mid = g.GravisSatOut(1.0f);
            if (i == Monster::GRAVIS_SLING_COUNT / 2) assert(glm::length(end - mid) < 0.01f);
            else assert(glm::length(end - mid) > 150.0f);
        }
        for (const int lane : {0, 2}) {
            auto h = MakeGravis();
            float hp2 = 1000;
            h.BeginGravisSling();
            while (h.gravisSling == Monster::GRAVIS_SLING_PREP) Tick(h, lockAt, hp2);
            const glm::vec2 outerPoint = h.GravisSatOut(0.5f, lane);
            int hits = 0;
            while (h.gravisSling != Monster::GRAVIS_SLING_HARASS) {
                const float before = hp2;
                Tick(h, outerPoint, hp2);
                if (hp2 < before) { assert(Near(before - hp2, 7.0f, 0.001f)); ++hits; }
            }
            assert(hits == 1);
        }
        {
            auto h = MakeGravis();
            float hp2 = 1000;
            h.BeginGravisSling();
            while (h.gravisSling == Monster::GRAVIS_SLING_PREP) Tick(h, lockAt, hp2);
            const glm::vec2 gap = (h.GravisSatOut(0.9f, 0) + h.GravisSatOut(0.9f, 1)) * 0.5f;
            while (h.gravisSling == Monster::GRAVIS_SLING_OUT || h.gravisSling == Monster::GRAVIS_SLING_LOCK)
                Tick(h, gap, hp2);
            assert(hp2 == 1000);
        }
        std::printf("sling fan ok\n");
    }

    // Satellites that are out when Gravis dies are drawn dying with it, and the
    // death effect lasts long enough to show them.
    {
        auto g = MakeGravis();
        float hp = 1000;
        g.BeginGravisSling();
        const glm::vec2 stand(600, 400);
        while (g.gravisSling != Monster::GRAVIS_SLING_HARASS) Tick(g, stand, hp);
        for (int i = 0; i < 120; ++i) Tick(g, stand, hp);
        int away = 0;
        for (int i = 0; i < 3; ++i) away += g.GravisSatAway(i);
        assert(away == 3);
        g.hp = 0;
        g.alive = false;
        g.deathEffectTimer = 0.0f;
        assert(g.DeathEffectVisible());
        for (int i = 0; i < 30; ++i) g.UpdateDeathEffect(DT);   // 0.25s in
        assert(g.DeathEffectVisible());
        for (int i = 0; i < 20; ++i) g.UpdateDeathEffect(DT);   // past 0.40s
        assert(!g.DeathEffectVisible());
        // The effect draws satellites only while they are out.
        int lines = 0;
        g.deathEffectTimer = 0.2f;
        g.DrawDeathEffect([&](float, float, float, float, float, float, float, float, float) { ++lines; },
                          [&](float, float, float, float, float, float, float) {},
                          [&](float, float, float, float, float, float, float, float) {});
        assert(lines > 30);
        std::printf("satellites die with Gravis ok (%d lines)\n", lines);
    }

    // A dead Gravis does nothing and spawns nothing.
    {
        auto g = MakeGravis();
        g.gravisTimer = g.gravisSlingTimer = 0.0f;
        float hp = 1000;
        for (int i = 0; i < 200; ++i) Tick(g, player, hp);
        g.hp = 0; g.alive = false;
        const float before = hp;
        for (int i = 0; i < 2000; ++i) Tick(g, player, hp);
        assert(hp == before && g.gravisSlowRequest == 0);
        std::printf("dead Gravis is inert ok\n");
    }

    std::printf("gravis elite regression passed\n");
    return 0;
}
