// NORMA boss body rules (WiNILL/Entity/NormaBoss.h).
#include "NormaBoss.h"

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
}

int main() {
    PhaseTwoAtFortyPercent();
    KeepsHoverDistance();
    if (g_failures == 0) std::printf("norma boss: all checks passed\n");
    return g_failures == 0 ? 0 : 1;
}
