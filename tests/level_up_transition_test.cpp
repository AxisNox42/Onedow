#include "LevelUpTransition.h"
#include <cassert>
#include <cmath>
#include <cstdio>

static bool Near(float a, float b) { return std::fabs(a - b) < 1e-5f; }

int main() {
    using T = LevelUpTransition;
    T t;
    assert(!t.Active() && t.Progress() == 0.0f);
    assert(!t.Update(1.0f));   // inactive: never fires

    // Runs for DURATION of real time, progress rising, then fires once.
    t.Begin();
    assert(t.Active() && t.Progress() == 0.0f);
    const float step = 1.0f / 120.0f;
    float prev = 0.0f;
    int frames = 0;
    bool fired = false;
    while (!fired && frames < 1000) {
        fired = t.Update(step);
        ++frames;
        if (!fired) {
            assert(t.Progress() >= prev && t.Progress() <= 1.0f);
            prev = t.Progress();
        }
    }
    assert(fired);
    assert(std::fabs(frames - T::DURATION / step) <= 1.5f);
    assert(!t.Active() && t.Progress() == 0.0f);
    assert(!t.Update(step));

    // Halfway point.
    t.Begin();
    assert(!t.Update(T::DURATION * 0.5f));
    assert(Near(t.Progress(), 0.5f));

    // Zero and negative deltas do not advance; cancel stops it.
    assert(!t.Update(0.0f) && !t.Update(-1.0f));
    assert(Near(t.Progress(), 0.5f));
    t.Cancel();
    assert(!t.Active() && t.Progress() == 0.0f);

    // One huge frame (e.g. a hitch) completes it in a single update.
    t.Begin();
    assert(t.Update(10.0f));
    assert(!t.Active());

    // Begin restarts from the top.
    t.Begin();
    t.Update(T::DURATION * 0.5f);
    t.Begin();
    assert(t.Progress() == 0.0f);

    std::puts("level_up_transition_test: OK");
    return 0;
}
