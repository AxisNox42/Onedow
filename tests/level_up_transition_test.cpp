#include "LevelUpTransition.h"
#include <cassert>
#include <cmath>
#include <cstdio>

static bool Near(float a, float b) { return std::fabs(a - b) < 1e-5f; }

int main() {
    using T = LevelUpTransition;
    T t;
    assert(!t.Active() && t.TimeScale() == 1.0f && t.Progress() == 0.0f);
    assert(!t.Update(1.0f));   // inactive: never fires

    // Starts at full speed and eases down monotonically to the minimum.
    t.Begin();
    assert(t.Active() && Near(t.TimeScale(), 1.0f));
    float prev = t.TimeScale();
    const float step = 1.0f / 120.0f;
    int frames = 0;
    bool fired = false;
    while (!fired && frames < 1000) {
        fired = t.Update(step);
        ++frames;
        if (!fired) {
            const float s = t.TimeScale();
            assert(s <= prev + 1e-6f);
            assert(s >= T::MIN_SCALE - 1e-6f);
            prev = s;
        }
    }
    assert(fired);
    assert(Near(prev, T::MIN_SCALE));   // held at the minimum before firing
    const float expectedFrames = T::TOTAL_DURATION / step;
    assert(std::fabs(frames - expectedFrames) <= 1.5f);

    // Fires exactly once and resets to normal speed.
    assert(!t.Active() && t.TimeScale() == 1.0f);
    assert(!t.Update(step));

    // Halfway through the slow-down the scale is strictly between the ends.
    t.Begin();
    assert(!t.Update(T::SLOW_DURATION * 0.5f));
    assert(t.TimeScale() < 1.0f && t.TimeScale() > T::MIN_SCALE);

    // Zero and negative deltas do not advance; cancel restores full speed.
    const float before = t.Progress();
    assert(!t.Update(0.0f) && !t.Update(-1.0f));
    assert(Near(t.Progress(), before));
    t.Cancel();
    assert(!t.Active() && t.TimeScale() == 1.0f);

    // One huge frame (e.g. a hitch) completes it in a single update.
    t.Begin();
    assert(t.Update(10.0f));
    assert(!t.Active());

    // Begin restarts from the top.
    t.Begin();
    t.Update(T::SLOW_DURATION);
    t.Begin();
    assert(Near(t.TimeScale(), 1.0f) && t.Progress() == 0.0f);

    std::puts("level_up_transition_test: OK");
    return 0;
}
