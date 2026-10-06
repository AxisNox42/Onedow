// Headless regression: run with scripts/test_mob_death_fx.ps1.
#include "Monster.h"
#include "RangedMob.h"
#include "../WiNILL/Render/Camera.h"
#include <cassert>
#include <cstdio>

int screenWidth = 1920;
int screenHeight = 1080;
float g_ViewZoom = 1.0f;
float g_ViewZoomTarget = 1.0f;
float g_ZoomCX = 0.0f;
float g_ZoomCY = 0.0f;

static void CheckGimbalAndWorldDiscCoordinates() {
    assert(Monster::GIMBAL_PREFERRED_DISTANCE == 700.0f);

    Monster gimbal(0.0f, 0.0f);
    gimbal.MakeKind(MobKind::GIMBAL);
    const float margin = 34.0f * gimbal.sizeScale;

    g_ZoomCX = 743.0f;
    g_ZoomCY = 391.0f;
    const float worldX = 1267.0f, worldY = -183.0f, worldRadius = 29.0f;
    for (const float zoom : {1.0f, 0.86f, 0.714f}) {
        g_ViewZoom = zoom;
        const WorldDiscProjection projection = ProjectWorldDisc(
            worldX, worldY, worldRadius);
        assert(std::abs(ScreenToWorldX(projection.screenX) - worldX) < 0.001f);
        assert(std::abs(ScreenToWorldY(projection.screenY) - worldY) < 0.001f);
        assert(std::abs(projection.screenRadius - worldRadius * zoom) < 0.001f);
        assert(std::abs(projection.worldX + projection.worldSize * 0.5f - worldX) < 0.001f);
        assert(std::abs(projection.worldY + projection.worldSize * 0.5f - worldY) < 0.001f);
        assert(std::abs(projection.worldSize - worldRadius * 2.0f) < 0.001f);

        const float left = ScreenToWorldX(0.0f), top = ScreenToWorldY(0.0f);
        const float right = ScreenToWorldX((float)screenWidth);
        const float bottom = ScreenToWorldY((float)screenHeight);
        gimbal.worldX = right + 500.0f;
        gimbal.worldY = bottom + 500.0f;
        gimbal.ConstrainGimbalToViewport(left, top, right, bottom);
        const float sx = W2SX(gimbal.worldX), sy = W2SY(gimbal.worldY);
        assert(sx >= margin * zoom - 0.001f);
        assert(sx <= screenWidth - margin * zoom + 0.001f);
        assert(sy >= margin * zoom - 0.001f);
        assert(sy <= screenHeight - margin * zoom + 0.001f);
    }

    float playerHp = 100.0f;
    Monster farGimbal(1000.0f, 0.0f);
    farGimbal.MakeKind(MobKind::GIMBAL);
    farGimbal.gimbalAttackTimer = 10.0f;
    farGimbal.UpdateGimbal(0.0f, 0.0f, 0.1f, 1000.0f, -1000.0f, 0.0f,
                          playerHp, 1.0f, nullptr, -1.0f, -1.0f, -1.0f, -1.0f);
    assert(farGimbal.worldX < 1000.0f);
    Monster closeGimbal(500.0f, 0.0f);
    closeGimbal.MakeKind(MobKind::GIMBAL);
    closeGimbal.gimbalAttackTimer = 10.0f;
    closeGimbal.UpdateGimbal(0.0f, 0.0f, 0.1f, 500.0f, -500.0f, 0.0f,
                             playerHp, 1.0f, nullptr, -1.0f, -1.0f, -1.0f, -1.0f);
    assert(closeGimbal.worldX > 500.0f);

    Monster shootingGimbal(700.0f, 0.0f);
    shootingGimbal.MakeKind(MobKind::GIMBAL);
    shootingGimbal.gimbalBursting = true;
    std::vector<Bullet> bullets;
    shootingGimbal.UpdateGimbal(0.0f, 0.0f, 0.1f, 700.0f, -700.0f, 0.0f,
                                playerHp, 1.0f, &bullets,
                                -1.0f, -1.0f, -1.0f, -1.0f);
    assert(bullets.size() == 1 && bullets[0].isEnemy);
    assert(!bullets[0].shootableEnemy && bullets[0].sizeScale > 0.0f);

    g_ZoomCX = g_ZoomCY = 0.0f;
    g_ViewZoom = g_ViewZoomTarget = 1.0f;
}

static void CheckBulletHaloLayers() {
    Bullet regular(10.0f, 20.0f, 30.0f, 40.0f);
    Bullet missile(10.0f, 20.0f, 30.0f, 40.0f);
    missile.rainMissile = true;
    Bullet crescent(10.0f, 20.0f, 30.0f, 40.0f);
    crescent.crescentBlade = true;
    crescent.traveled = 240.0f;

    for (const Bullet* bullet : {&regular, &missile, &crescent}) {
        const BulletHaloStyle style = GetBulletHaloStyle(*bullet);
        assert(style.innerRadius > 0.0f);
        assert(std::abs(style.outerRadius - style.innerRadius * 2.0f) < 0.001f);
        assert(style.outerAlpha > 0.0f && style.outerAlpha < style.innerAlpha);
    }
    assert(std::abs(GetBulletHaloStyle(regular).innerAlpha - 0.20f) < 0.001f);
    assert(std::abs(GetBulletHaloStyle(missile).innerAlpha - 0.18f) < 0.001f);
    assert(std::abs(GetBulletHaloStyle(crescent).innerAlpha - 0.10f) < 0.001f);
    assert(std::abs(GetBulletHaloStyle(regular).outerAlpha - 0.07f) < 0.001f);
    assert(std::abs(GetBulletHaloStyle(missile).outerAlpha - 0.063f) < 0.001f);
    assert(std::abs(GetBulletHaloStyle(crescent).outerAlpha - 0.035f) < 0.001f);
}

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
            mob.alive = false;
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
            assert(std::abs(p.radiusLimit - RangedMob::VISUAL_BASE_PX
                            * 1.35f * 1.7f * 2.0f) < 0.001f);
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
    stagedMob.alive = false;
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
    CheckGimbalAndWorldDiscCoordinates();
    CheckBulletHaloLayers();
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
    std::printf("PASS: Gimbal range, zoomed viewport clamp and CircleTexture coordinates; particle pool/presets/bounds/lifetime. Pool: %zu bytes.\n", sizeof(g_EnemyParts));
}
