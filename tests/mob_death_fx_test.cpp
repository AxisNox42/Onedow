// Headless regression: run with scripts/test_mob_death_fx.ps1.
#include "Monster.h"
#include "RangedMob.h"
#include <cassert>
#include <cstdio>

static int ActiveCount() {
    int count = 0;
    for (const auto& p : g_EnemyParts) if (p.active) ++count;
    return count;
}

static void CheckBounds() {
    for (int frame = 0; frame < 120; ++frame) {
        for (const auto& p : g_EnemyParts) {
            if (!p.active) continue;
            assert(p.maxLife + p.delay <= 0.80001f);
            assert(p.size >= 1.8f && p.size <= 14.0f);
            assert(std::isfinite(p.x) && std::isfinite(p.y));
            const float dx = p.x - p.originX, dy = p.y - p.originY;
            assert(std::sqrt(dx * dx + dy * dy) + p.size * 0.707107f
                   <= p.radiusLimit + 0.001f);
        }
        UpdateEnemyParticles(1.0f / 120.0f);
    }
    assert(ActiveCount() == 0);
}

static void CheckRoster() {
    int ordinaryCount = 0;
    for (const auto kind : {MobKind::ROTOR, MobKind::SWARM, MobKind::GENESIS,
                           MobKind::GRAVIS, MobKind::QUASAR}) {
        for (bool reduced : {false, true}) {
            ResetEnemyParticles();
            Monster mob(100.0f, 200.0f);
            mob.MakeKind(kind);
            mob.quasarState = 3;
            mob.SpawnDeathEffect(2.0f, reduced);
            const int count = ActiveCount();
            assert(count > 0 && mob.exploded);
            bool delayed = false;
            for (const auto& p : g_EnemyParts) {
                if (!p.active) continue;
                delayed = delayed || p.delay > 0.0f;
                assert(p.r >= mob.color.r && p.g >= mob.color.g && p.b >= mob.color.b);
                if (kind == MobKind::GRAVIS && p.delay == 0.0f)
                    assert((p.x - p.originX) * p.vx + (p.y - p.originY) * p.vy < 0.0f);
                if (kind == MobKind::QUASAR) assert(p.size <= 10.0f);
            }
            const bool large = kind == MobKind::GENESIS || kind == MobKind::GRAVIS
                            || kind == MobKind::QUASAR;
            assert(delayed == large);
            if (!reduced) {
                if (kind == MobKind::ROTOR) ordinaryCount = count;
                if (large) assert(count > ordinaryCount);
            }
            mob.SpawnDeathEffect(2.0f, reduced);
            assert(ActiveCount() == count);
            CheckBounds();
        }
    }
    for (const auto state : {RangedMob::State::IDLE, RangedMob::State::CHARGING,
                            RangedMob::State::BURST}) {
        ResetEnemyParticles();
        RangedMob scope(10.0f, 20.0f, 1920, 1080);
        scope.lensState = state;
        scope.chargeAngle = 10.0f;
        scope.deathScale = 0.4f;
        scope.SpawnDeathEffect();
        assert(scope.exploded && scope.deathScale == 0.0f);
        const int count = ActiveCount();
        assert(count > 0);
        for (const auto& p : g_EnemyParts) if (p.active) {
            assert(p.radiusLimit / 1.5f < RangedMob::VISUAL_BASE_PX * 1.6f);
            assert(p.delay == 0.0f);
        }
        scope.SpawnDeathEffect();
        assert(ActiveCount() == count);
        CheckBounds();
    }
}

static void CheckPriorityAndDelay() {
    ResetEnemyParticles();
    while (ActiveCount() < EnemyParticleCap() * 4 / 5) {
        Monster mob(0.0f, 0.0f);
        mob.SpawnDeathEffect(0.0f);
    }
    const int before = ActiveCount();
    Monster crowdedMob(0.0f, 0.0f);
    crowdedMob.SpawnDeathEffect(0.0f);
    assert(ActiveCount() - before <= 10);
    for (int i = 0; i < MAX_ENEMY_PARTS; ++i) {
        Monster mob(0.0f, 0.0f);
        mob.SpawnDeathEffect(0.0f);
    }
    assert(ActiveCount() == MAX_ENEMY_PARTS);
    Monster largeMob(999.0f, 0.0f);
    largeMob.MakeKind(MobKind::QUASAR);
    largeMob.SpawnDeathEffect(0.0f);
    int largeParts = 0;
    for (const auto& p : g_EnemyParts) if (p.active && p.originX == 999.0f) ++largeParts;
    assert(largeParts > 10 && ActiveCount() == MAX_ENEMY_PARTS);
    Monster tinyMob(0.0f, 0.0f);
    tinyMob.MakeKind(MobKind::SWARM);
    tinyMob.SpawnDeathEffect(0.0f);
    int preserved = 0;
    for (const auto& p : g_EnemyParts) if (p.active && p.originX == 999.0f) ++preserved;
    assert(preserved == largeParts);

    ResetEnemyParticles();
    Monster stagedMob(10.0f, 20.0f);
    stagedMob.MakeKind(MobKind::GENESIS);
    stagedMob.SpawnDeathEffect(0.0f);
    EnemyParticle* delayed = nullptr;
    for (auto& p : g_EnemyParts) if (p.active && p.delay > 0.0f) { delayed = &p; break; }
    assert(delayed);
    const float x = delayed->x, life = delayed->life;
    UpdateEnemyParticles(0.02f);
    assert(delayed->x == x && delayed->life == life && delayed->delay > 0.0f);
    UpdateEnemyParticles(0.06f);
    assert(delayed->delay == 0.0f && delayed->life < life);
    CheckBounds();
}

int main() {
    srand(7);
    CheckRoster();
    CheckPriorityAndDelay();
    ResetEnemyParticles();
    // Original aggregate initializers still get the original drag/motion/fade.
    g_EnemyParts[0] = {1, 2, 100, 50, 0.3f, 0.3f, 4, 1, 0.5f, 0.2f, true};
    assert(g_EnemyParts[0].delay == 0.0f && g_EnemyParts[0].radiusLimit == 0.0f);
    UpdateEnemyParticles(0.02f);
    assert(std::abs(g_EnemyParts[0].x - 3.0f) < 0.001f);
    assert(std::abs(g_EnemyParts[0].vx - 95.0f) < 0.001f);
    ResetEnemyParticles();
    auto* mob = new Monster(50.0f, 70.0f);
    mob->MakeKind(MobKind::GRAVIS);
    mob->SpawnDeathEffect(0.0f);
    delete mob;
    assert(ActiveCount() > 0 && g_EnemyParts[0].originX == 50.0f);
    UpdateEnemyParticles(0.8f);
    assert(ActiveCount() == 0);
    std::printf("PASS: original pool/motion, six particle presets, bounds, stages, priority, lifetime, duplicate/deleted mobs. Pool: %zu bytes.\n", sizeof(g_EnemyParts));
}
