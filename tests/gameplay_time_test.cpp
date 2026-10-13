#include "GameplayTime.h"
#include <cassert>
#include <cmath>
#include <cstdio>

static bool Near(float a, float b) { return std::fabs(a - b) < 1e-6f; }

int main() {
    GameplayTimeInputs normal;
    assert(Near(ComputeGameplayDelta(0.016f, normal), 0.016f));

    GameplayTimeInputs dying;
    dying.dying = true;
    assert(Near(ComputeGameplayDelta(0.1f, dying), 0.1f * DYING_TIME_SCALE));

    // Hit-stop freezes the simulation even during the death slow-motion.
    GameplayTimeInputs frozen;
    frozen.hitStop = true;
    frozen.dying = true;
    assert(ComputeGameplayDelta(0.1f, frozen) == 0.0f);

    // Level-up slow-motion scales normal play, but hit-stop and death win.
    GameplayTimeInputs levelUp;
    levelUp.levelUpScale = 0.15f;
    assert(Near(ComputeGameplayDelta(0.1f, levelUp), 0.015f));
    levelUp.dying = true;
    assert(Near(ComputeGameplayDelta(0.1f, levelUp), 0.1f * DYING_TIME_SCALE));
    levelUp.hitStop = true;
    assert(ComputeGameplayDelta(0.1f, levelUp) == 0.0f);
    GameplayTimeInputs outOfRange;
    outOfRange.levelUpScale = 3.0f;
    assert(Near(ComputeGameplayDelta(0.1f, outOfRange), 0.1f));
    outOfRange.levelUpScale = -1.0f;
    assert(ComputeGameplayDelta(0.1f, outOfRange) == 0.0f);

    assert(ComputeGameplayDelta(-0.5f, normal) == 0.0f);
    assert(ComputeGameplayDelta(0.0f, dying) == 0.0f);

    std::puts("gameplay_time_test: OK");
    return 0;
}
