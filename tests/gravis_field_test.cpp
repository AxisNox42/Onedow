// Headless regression for the Gravis elite field: the pull is stronger but
// capped against the player's move speed, and the time-dilation influence
// slows fire / dash recovery without stacking across several Gravis.
// Run with scripts/test_gravis_field.ps1.
#include "GravisField.h"
#include "Monster.h"
#include <cassert>
#include <cmath>
#include <cstdio>

static bool Near(float a, float b, float eps = 1e-3f) { return std::fabs(a - b) < eps; }

static void CheckInfluenceCurve() {
    using namespace GravisField;
    assert(Near(Influence(RADIUS * RADIUS), 0.0f));
    assert(Near(Influence((RADIUS + 10.0f) * (RADIUS + 10.0f)), 0.0f));
    assert(Near(Influence(0.0f), 1.0f));
    assert(Near(Influence((RADIUS * 0.5f) * (RADIUS * 0.5f)), 0.5f));
    std::printf("influence curve ok\n");
}

static void CheckPullStrength() {
    using namespace GravisField;
    assert(Near(PullSpeed(0.0f), 0.0f));
    assert(Near(PullSpeed(1.0f), 150.0f));
    assert(PullSpeed(0.5f) > 0.0f && PullSpeed(0.5f) < PullSpeed(1.0f));
    std::printf("pull strength ok (center %.0f px/s)\n", PullSpeed(1.0f));
}

static void CheckPullCapAcrossSeveralGravis() {
    using namespace GravisField;
    const float moveSpeed = 400.0f;
    FieldAccum acc;
    // Five Gravis stacked on the player's side, all at point blank.
    for (int i = 0; i < 5; ++i) acc.Add(10.0f, 0.0f);
    const float cap = PullCap(moveSpeed);
    assert(Near(cap, 220.0f));
    const float rawLen = std::sqrt(acc.pullVX * acc.pullVX + acc.pullVY * acc.pullVY);
    assert(rawLen > cap);
    acc.ClampPull(cap);
    const float len = std::sqrt(acc.pullVX * acc.pullVX + acc.pullVY * acc.pullVY);
    assert(Near(len, cap, 0.01f));
    assert(acc.pullVX > 0.0f && Near(acc.pullVY, 0.0f));
    std::printf("pull cap ok (%.0f -> %.0f px/s)\n", rawLen, len);
}

static void CheckCapScalesWithMoveSpeed() {
    using namespace GravisField;
    assert(Near(PullCap(400.0f * 0.68f), 400.0f * 0.68f * 0.55f));
    assert(PullCap(400.0f * 0.68f) < PullCap(400.0f));
    std::printf("cap follows move speed ok\n");
}

static void CheckOpposingPullsCancel() {
    using namespace GravisField;
    FieldAccum acc;
    acc.Add(100.0f, 0.0f);
    acc.Add(-100.0f, 0.0f);
    acc.ClampPull(PullCap(400.0f));
    assert(Near(acc.pullVX, 0.0f, 0.01f));
    std::printf("opposing pulls cancel ok\n");
}

static void CheckDilationUsesStrongestOnly() {
    using namespace GravisField;
    FieldAccum solo;
    solo.Add(100.0f, 0.0f);
    FieldAccum stacked;
    for (int i = 0; i < 4; ++i) stacked.Add(100.0f, 0.0f);
    assert(Near(solo.maxInfluence, stacked.maxInfluence));
    assert(Near(FireIntervalMult(stacked.maxInfluence), FireIntervalMult(solo.maxInfluence)));
    std::printf("dilation does not stack ok\n");
}

static void CheckDilationValues() {
    using namespace GravisField;
    assert(Near(FireIntervalMult(0.0f), 1.0f));
    assert(Near(FireIntervalMult(1.0f), 1.35f));
    assert(Near(DashRecoveryMult(0.0f), 1.0f));
    assert(Near(DashRecoveryMult(1.0f), 0.5f));
    assert(DashRecoveryMult(0.5f) > 0.5f && DashRecoveryMult(0.5f) < 1.0f);
    std::printf("dilation values ok\n");
}

static void CheckOutsideFieldIsNeutral() {
    using namespace GravisField;
    FieldAccum acc;
    acc.Add(RADIUS + 1.0f, 0.0f);
    assert(Near(acc.pullVX, 0.0f) && Near(acc.maxInfluence, 0.0f));
    // Sitting almost on the core exerts no direction-less pull.
    FieldAccum core;
    core.Add(1.0f, 0.0f);
    assert(Near(core.pullVX, 0.0f));
    std::printf("outside / core neutral ok\n");
}

static void CheckEliteSet() {
    assert(IsEliteMob(MobKind::GRAVIS));
    assert(IsEliteMob(MobKind::GENESIS));
    assert(IsEliteMob(MobKind::QUASAR));
    assert(!IsEliteMob(MobKind::ROTOR));
    assert(!IsEliteMob(MobKind::MAGNETAR));
    std::printf("elite set ok\n");
}

int main() {
    Monster original(0.0f, 0.0f);
    original.MakeKind(MobKind::GRAVIS);
    assert(Near(original.hp, 720.0f));
    original.ApplyDamage(original.hp);
    float playerHP = 100.0f;
    std::vector<Bullet> bullets;
    original.Update(0, 0, 5.0f, playerHP, 1, -1, -1, -1, -1, &bullets);
    assert(!original.alive && original.hp == 0 && bullets.empty());
    std::printf("original Gravis: single death, no revival or ejecta ok\n");
    CheckInfluenceCurve();
    CheckPullStrength();
    CheckPullCapAcrossSeveralGravis();
    CheckCapScalesWithMoveSpeed();
    CheckOpposingPullsCancel();
    CheckDilationUsesStrongestOnly();
    CheckDilationValues();
    CheckOutsideFieldIsNeutral();
    CheckEliteSet();
    std::printf("gravis field regression passed\n");
    return 0;
}
