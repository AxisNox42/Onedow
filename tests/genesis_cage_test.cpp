// Headless regression for the Genesis elite cage: live thrown Swarms become
// the vertices of a polygon around the player that locks and shrinks
// (Docs/superpowers/specs/2026-10-17-genesis-cage-design.md).
// Run with scripts/test_genesis_cage.ps1.
#include "GenesisCage.h"
#include "MonsterManager.h"
#include <cassert>
#include <cmath>
#include <cstdio>

static bool Near(float a, float b, float eps = 1e-3f) { return std::fabs(a - b) < eps; }

static void CheckGeometry() {
    using namespace GenesisCage;
    assert(Near(SlotAngle(0.3f, 2, 5), 0.3f + 2.0f * 6.2831853f / 5.0f));
    assert(Near(DeployBlend(0.0f), 0.0f) && Near(DeployBlend(DEPLOY_TIME * 0.5f), 0.5f));
    assert(Near(DeployBlend(DEPLOY_TIME), 1.0f) && Near(DeployBlend(5.0f), 1.0f));
    assert(Near(LockRadius(0.0f), 240.0f) && Near(LockRadius(0.75f), 120.0f));
    assert(Near(LockRadius(1.5f), 0.0f) && Near(LockRadius(9.0f), 0.0f));
    assert(Near(SegmentDistSq(0, 5, -10, 0, 10, 0), 25.0f));
    assert(Near(SegmentDistSq(20, 0, -10, 0, 10, 0), 100.0f));   // past the end
    assert(Near(SegmentDistSq(3, 4, 0, 0, 0, 0), 25.0f));        // zero-length
    std::printf("geometry ok\n");
}

int main() {
    srand(11);
    CheckGeometry();
    std::printf("genesis cage regression passed\n");
    return 0;
}
