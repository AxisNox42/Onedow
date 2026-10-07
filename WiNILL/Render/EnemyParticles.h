#pragma once
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>
#include <glm/glm.hpp>
#include "../System/Settings.h"

// Original hit sparks, also used as the short bright layer of death bursts.
struct Spark {
    float x, y, vx, vy;
    float life, maxLife, size;
    float r, g, b;
};
inline std::vector<Spark> g_Sparks;

inline int JuiceSparkCap() {
#if defined(__APPLE__)
    return g_VfxDensity == VfxDensity::REDUCED ? 160
         : g_VfxDensity == VfxDensity::MEDIUM ? 190 : 220;
#else
    return g_VfxDensity == VfxDensity::REDUCED ? 350
         : g_VfxDensity == VfxDensity::MEDIUM ? 425 : 500;
#endif
}

inline void SpawnSparks(float x, float y, int n,
                        float r, float g, float b, float speed = 300.0f) {
    n = std::min(n, std::max(0, JuiceSparkCap() - (int)g_Sparks.size()));
    for (int i = 0; i < n; ++i) {
        const float angle = (float)(rand() % 628) * 0.01f;
        const float velocity = speed * (0.35f + (rand() % 100) * 0.01f);
        Spark sp;
        sp.x = x; sp.y = y;
        sp.vx = cosf(angle) * velocity; sp.vy = sinf(angle) * velocity;
        sp.maxLife = sp.life = 0.16f + (rand() % 80) * 0.001f;
        sp.size = 2.5f + (float)(rand() % 3);
        sp.r = r; sp.g = g; sp.b = b;
        g_Sparks.push_back(sp);
    }
}

inline void SpawnMobDeathSparks(float x, float y, glm::vec3 color,
                                 float bodyRadius, int count, bool reduced = false) {
    const glm::vec3 bright = color + (glm::vec3(1.0f) - color) * 0.75f;
    SpawnSparks(x, y, reduced ? std::max(2, count / 2) : count,
                bright.r, bright.g, bright.b, bodyRadius * 4.0f);
}

// The original explosion particles and pool, shared by Juice and mob recipes.
enum class EnemyParticleShape { CIRCLE, SQUARE, TRIANGLE };

inline EnemyParticleShape RandomEnemyParticleShape() {
    return static_cast<EnemyParticleShape>(rand() % 3);
}

struct EnemyParticle {
    float x, y, vx, vy;
    float life, maxLife, size;
    float r, g, b;
    bool active = false;
    // Generic explosions leave these zero. Death bursts can wait briefly and
    // stay within the body-based range; the range also ranks crowded effects.
    float delay = 0.0f, originX = 0.0f, originY = 0.0f, radiusLimit = 0.0f;
    EnemyParticleShape shape = EnemyParticleShape::SQUARE;
};
// Allocate once; quality limits the slots used to 256 / 512 / 1024.
inline constexpr int MAX_ENEMY_PARTS = 1024;
inline EnemyParticle g_EnemyParts[MAX_ENEMY_PARTS] = {};

inline float MobDeathParticleRadius(float visualBaseRadius) {
    return visualBaseRadius * 1.35f * 1.7f;
}

struct EnemyParticleBurst {
    int count = 10, directions = 0;
    float life = 0.20f, speedMin = 100.0f, speedMax = 250.0f;
    float sizeMin = 3.0f, sizeMax = 6.0f;
    float startRadius = 0.0f, phase = 0.0f, jitter = 0.25f;
    float tangent = 0.0f, delay = 0.0f, colorLift = 0.0f;
    float rangeScale = 2.0f;
    bool inward = false;
    bool randomPlacement = false;
};

inline void ClampEnemyParticleRange(EnemyParticle& p) {
    if (p.radiusLimit <= 0.0f) return;
    const float dx = p.x - p.originX, dy = p.y - p.originY;
    const float distance = std::sqrt(dx * dx + dy * dy);
    const float limit = std::max(0.0f, p.radiusLimit - p.size * 0.707107f);
    if (distance <= limit || distance == 0.0f) return;
    p.x = p.originX + dx * limit / distance;
    p.y = p.originY + dy * limit / distance;
    // Remove outward velocity so particles do not keep pushing against the rim.
    const float outward = (p.vx * dx + p.vy * dy) / distance;
    if (outward > 0.0f) {
        p.vx -= outward * dx / distance;
        p.vy -= outward * dy / distance;
    }
}

inline EnemyParticle* FindEnemyParticleSlot(float radiusLimit) {
    EnemyParticle* victim = nullptr;
    for (int i = 0; i < EnemyParticleCap(); ++i) {
        auto& p = g_EnemyParts[i];
        if (!p.active) return &p;
        if ((p.radiusLimit < radiusLimit ||
             (p.radiusLimit == radiusLimit && p.life < p.maxLife * 0.35f)) &&
            (!victim || p.radiusLimit < victim->radiusLimit ||
             (p.radiusLimit == victim->radiusLimit && p.life < victim->life)))
            victim = &p;
    }
    return victim; // A larger death burst replaces lower-priority particles.
}

inline void SpawnEnemyParticleBurst(float x, float y, glm::vec3 color,
                                    float bodyRadius, const EnemyParticleBurst& burst,
                                    bool reduced = false) {
    int active = 0;
    const int cap = EnemyParticleCap();
    for (int i = 0; i < cap; ++i) if (g_EnemyParts[i].active) ++active;
    int count = reduced ? std::max(3, burst.count / 2) : burst.count;
    if (active >= cap * 4 / 5 && bodyRadius < 48.0f)
        count = std::max(6, count / 2);
    const float range = std::max(4.0f, bodyRadius)
                      * std::clamp(burst.rangeScale, 1.3f, 2.0f);
    const float delay = std::clamp(burst.delay, 0.0f, 0.08f);
    const float life = std::clamp(burst.life, 0.05f, 0.80f - delay);
    for (int i = 0; i < count; ++i) {
        auto* slot = FindEnemyParticleSlot(range);
        if (!slot) break;
        const int directions = burst.directions > 0 ? burst.directions : count;
        const float angle = burst.phase + (burst.randomPlacement
                          ? (float)(rand() % 10001) / 10000.0f * 6.2831853f
                          : (float)(i % directions) * 6.2831853f / directions)
                          + ((float)(rand() % 101) / 100.0f - 0.5f) * burst.jitter;
        const float ox = cosf(angle), oy = sinf(angle);
        const float originAngle = burst.randomPlacement
            ? (float)(rand() % 10001) / 10000.0f * 6.2831853f : angle;
        const float startDistance = bodyRadius * burst.startRadius *
            (burst.randomPlacement ? std::sqrt((float)(rand() % 10001) / 10000.0f) : 1.0f);
        const float particleLife = life * (burst.randomPlacement
            ? 0.80f + (float)(rand() % 101) / 100.0f * 0.20f : 1.0f);
        const float speed = burst.speedMin + (burst.speedMax - burst.speedMin)
                          * (float)(rand() % 101) / 100.0f;
        const float size = std::clamp(burst.sizeMin + (burst.sizeMax - burst.sizeMin)
                         * (float)(rand() % 101) / 100.0f, 1.8f, 14.0f);
        const float radial = burst.inward ? -speed : speed;
        const float lift = std::clamp(burst.colorLift, 0.0f, 0.65f);
        *slot = {
            x + cosf(originAngle) * startDistance,
            y + sinf(originAngle) * startDistance,
            ox * radial - oy * speed * burst.tangent,
            oy * radial + ox * speed * burst.tangent,
            particleLife, particleLife, size,
            color.r + (1.0f - color.r) * lift,
            color.g + (1.0f - color.g) * lift,
            color.b + (1.0f - color.b) * lift,
            true, delay, x, y, range
        };
        slot->shape = RandomEnemyParticleShape();
        ClampEnemyParticleRange(*slot);
    }
}

inline void UpdateEnemyParticles(float delta) {
    const int cap = EnemyParticleCap();
    for (int i = cap; i < MAX_ENEMY_PARTS; ++i) g_EnemyParts[i].active = false;
    for (int i = 0; i < cap; ++i) {
        auto& p = g_EnemyParts[i];
        if (!p.active) continue;
        float step = std::max(0.0f, delta);
        if (p.delay > 0.0f) {
            const float wait = std::min(p.delay, step);
            p.delay -= wait;
            step -= wait;
            if (step <= 0.0f) continue;
        }
        p.life -= step;
        if (p.life <= 0.0f) { p.active = false; continue; }
        p.x += p.vx * step;
        p.y += p.vy * step;
        const float drag = std::max(0.0f, 1.0f - 2.5f * step);
        p.vx *= drag;
        p.vy *= drag;
        ClampEnemyParticleRange(p);
    }
}

inline void ResetEnemyParticles() {
    for (auto& p : g_EnemyParts) p.active = false;
}
