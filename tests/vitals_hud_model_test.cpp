// Run scripts/test_vitals_hud_model.ps1. Vitals HUD model: the segmented HP
// arc around the player and the XP edge line at the bottom of the screen.
#include "VitalsHudModel.h"
#include <cassert>
#include <cmath>
#include <cstdio>

using namespace vitals_hud;

static bool Near(float a, float b) { return std::fabs(a - b) < 1e-4f; }

static void danger_threshold() {
    assert(IsDanger(0.24f));
    assert(!IsDanger(0.25f));
    assert(!IsDanger(0.0f));   // dead: no warning pulse
    assert(IsDanger(0.01f));
}

static void ghost_holds_then_drains() {
    HpGhost g;
    g.Reset(1.0f);
    g.Update(0.6f, 0.1f);            // hit: ghost stays at the old value
    assert(Near(g.frac, 1.0f));
    g.Update(0.6f, kGhostHold - 0.1f);   // still inside the hold window
    assert(Near(g.frac, 1.0f));
    g.Update(0.6f, 0.1f);            // hold over: drains toward the HP
    assert(g.frac < 1.0f && g.frac > 0.6f);
    for (int i = 0; i < 100; ++i) g.Update(0.6f, 0.05f);
    assert(Near(g.frac, 0.6f));      // never drains below the HP
}

static void ghost_follows_heal_at_once() {
    HpGhost g;
    g.Reset(0.4f);
    g.Update(0.7f, 0.016f);
    assert(Near(g.frac, 0.7f));
}

static void second_hit_restarts_hold() {
    HpGhost g;
    g.Reset(1.0f);
    g.Update(0.8f, 0.3f);
    g.Update(0.5f, 0.016f);          // new hit inside the hold: hold restarts
    assert(Near(g.frac, 1.0f));
    g.Update(0.5f, kGhostHold - 0.1f);
    assert(Near(g.frac, 1.0f));
}

static void xp_fill() {
    assert(Near(XpFill(31, 50, false), 0.62f));
    assert(Near(XpFill(80, 50, false), 1.0f));
    assert(Near(XpFill(10, 0, false), 0.0f));
    assert(Near(XpFill(0, 50, true), 1.0f));   // level cap: full line
}

static void arc_angles() {
    // 240° arc open at the top: left end at 210°, right end at -30°, bottom (90°) in the middle.
    const float mid = kArcStart + kArcSweep * 0.5f;
    assert(Near(std::sin(mid), 1.0f));
    assert(Near(kArcSweep, -240.0f * kDeg));
}

int main() {
    danger_threshold();
    ghost_holds_then_drains();
    ghost_follows_heal_at_once();
    second_hit_restarts_hold();
    xp_fill();
    arc_angles();
    std::puts("vitals_hud_model_test: ok");
    return 0;
}
