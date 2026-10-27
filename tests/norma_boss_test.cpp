// NORMA boss body rules (WiNILL/Entity/NormaBoss.h).
#include "NormaBoss.h"

#include <cmath>
#include <cstdio>

static int g_failures = 0;

static void Check(bool ok, const char* what) {
    if (!ok) {
        std::printf("FAIL: %s\n", what);
        ++g_failures;
    }
}

static void PhaseTwoAtFortyPercent() {
    Check(norma::PhaseFor(1000.0f, 1000.0f) == 1, "full HP is phase 1");
    Check(norma::PhaseFor(401.0f, 1000.0f) == 1, "just above 40% is phase 1");
    Check(norma::PhaseFor(400.0f, 1000.0f) == 2, "exactly 40% is phase 2");
    Check(norma::PhaseFor(10.0f, 1000.0f) == 2, "low HP is phase 2");
    Check(norma::PhaseFor(10.0f, 0.0f) == 1, "unknown max HP stays phase 1");
}

static void KeepsHoverDistance() {
    Check(norma::HoverSpeed(norma::kHoverDistance + norma::kHoverBand + 1.0f) > 0.0f,
          "too far: closes in");
    Check(norma::HoverSpeed(norma::kHoverDistance - norma::kHoverBand - 1.0f) < 0.0f,
          "too close: backs off");
    Check(norma::HoverSpeed(norma::kHoverDistance) == 0.0f, "inside the band: holds");
    Check(norma::HoverSpeedSmooth(norma::kHoverDistance) == 0.0f, "smooth: inside the band holds");
    const float edge = norma::kHoverDistance + norma::kHoverBand;
    Check(norma::HoverSpeedSmooth(edge + 1.0f) > 0.0f && norma::HoverSpeedSmooth(edge + 1.0f) < 1.0f,
          "smooth: speed starts from nothing just past the band");
    Check(std::fabs(norma::HoverSpeedSmooth(edge + 1000.0f) - norma::kMoveSpeed) < 1e-3f,
          "smooth: far away reaches the full speed");
    Check(norma::HoverSpeedSmooth(norma::kHoverDistance - norma::kHoverBand - 40.0f) < 0.0f,
          "smooth: too close backs off");
    float prev = 0.0f;
    bool even = true;
    for (float d = edge; d < edge + 200.0f; d += 1.0f) {
        const float v = norma::HoverSpeedSmooth(d);
        if (v < prev - 1e-4f || v - prev > 4.0f) even = false;
        prev = v;
    }
    Check(even, "smooth: speed rises evenly, no jumps");
}

static void PatternsTakeTurns() {
    using norma::Pattern;
    for (int roll = 0; roll < 12; ++roll) {
        const Pattern first = norma::NextPattern(0, 1, roll);
        Check(first == Pattern::Refuge ||
              first == Pattern::Throw || first == Pattern::Ram ||
              first == Pattern::Detonate,
              "phase 1 window turn picks Refuge, Throw, Ram or Detonate");
        const Pattern direct = norma::NextPattern(1, 1, roll);
        Check(direct == Pattern::Aim || direct == Pattern::Dash,
              "odd turns are a direct attack (aimed stream or charge)");
    }
    bool sawAim = false, sawDash = false;
    for (int roll = 0; roll < 40; ++roll) {
        sawAim = sawAim || norma::NextPattern(1, 1, roll) == Pattern::Aim;
        sawDash = sawDash || norma::NextPattern(1, 1, roll) == Pattern::Dash;
    }
    Check(sawAim && sawDash, "direct turns use both the aimed stream and the charge");
    bool sawClose = false;
    for (int roll = 0; roll < 12; ++roll)
        sawClose = sawClose || norma::NextPattern(2, 2, roll) == Pattern::CloseWindow;
    Check(sawClose, "phase 2 adds CloseWindow to the window turns");
    bool sawThrow = false, sawRam = false;
    for (int roll = 0; roll < 12; ++roll) {
        sawThrow = sawThrow || norma::NextPattern(0, 1, roll) == Pattern::Throw;
        sawRam = sawRam || norma::NextPattern(0, 1, roll) == Pattern::Ram;
    }
    Check(sawThrow && sawRam, "window turns include Throw and Ram");
    bool sawDetonate = false;
    for (int roll = 0; roll < 12; ++roll)
        sawDetonate = sawDetonate || norma::NextPattern(0, 1, roll) == Pattern::Detonate;
    Check(sawDetonate, "window turns include Detonate");
    bool closeInPhase1 = false;
    for (int roll = 0; roll < 12; ++roll)
        closeInPhase1 = closeInPhase1 || norma::NextPattern(2, 1, roll) == Pattern::CloseWindow;
    Check(!closeInPhase1, "phase 1 never picks CloseWindow");
    Check(norma::NextPattern(0, 1, -5) != Pattern::None, "negative rolls still pick a pattern");
}

static void LeadsMovingTargets() {
    // Standing still: aim straight at it.
    Check(std::fabs(norma::LeadAngle(0, 0, 100, 0, 0, 0, 300)) < 1e-4f, "still target: direct aim");
    // Moving up across the line of fire: the aim leads upward (positive y).
    const float lead = norma::LeadAngle(0, 0, 300, 0, 0, 200, 400);
    Check(lead > 0.2f, "crossing target: aim leads it");
    // The shot really meets it: solve the meeting time along the lead angle.
    const float vx = std::cos(lead) * 400.0f, vy = std::sin(lead) * 400.0f;
    const float t = 300.0f / vx;
    Check(std::fabs(vy * t - 200.0f * t) < 1.0f, "lead shot meets the target");
    // lead 0 is the current position, lead 2 overshoots the intercept.
    Check(std::fabs(norma::LeadAngle(0, 0, 300, 0, 0, 200, 400, 0.0f)) < 1e-4f, "lead 0 aims at now");
    Check(norma::LeadAngle(0, 0, 300, 0, 0, 200, 400, 2.0f) > lead, "lead 2 aims past the intercept");
    // A target faster than the shot, running away: falls back to direct aim.
    Check(std::fabs(norma::LeadAngle(0, 0, 100, 0, 500, 0, 100)) < 1e-4f, "no intercept: direct aim");
}

static void HealthStarsCountEighths() {
    Check(norma::LitStarsFor(100.0f, 100.0f) == 8, "full health lights all eight stars");
    Check(norma::LitStarsFor(87.6f, 100.0f) == 8, "the first eighth keeps all eight");
    Check(norma::LitStarsFor(87.4f, 100.0f) == 7, "losing an eighth darkens one");
    Check(norma::LitStarsFor(40.0f, 100.0f) == 4, "phase 2 starts with four lit");
    Check(norma::LitStarsFor(1.0f, 100.0f) == 1, "the last star stays lit while alive");
    Check(norma::LitStarsFor(0.0f, 100.0f) == 0, "dead: all dark");
    bool seen[8] = {};
    for (int k = 0; k < 8; ++k) seen[norma::kStarOutOrder[k]] = true;
    bool all = true;
    for (bool s : seen) all = all && s;
    Check(all, "the star order covers every corner once");
}

int main() {
    HealthStarsCountEighths();
    LeadsMovingTargets();
    PhaseTwoAtFortyPercent();
    KeepsHoverDistance();
    PatternsTakeTurns();
    if (g_failures == 0) std::printf("norma boss: all checks passed\n");
    return g_failures == 0 ? 0 : 1;
}
