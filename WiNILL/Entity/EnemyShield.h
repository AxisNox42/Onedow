#pragma once

#include <algorithm>
#include "../Render/EnemyParticles.h"

inline constexpr float MOB_HIT_FLASH_TIME = 0.12f;

inline float ApplyMobDamage(float& hp, bool& alive, float& hitFlashTimer,
                            float damage) {
    if (!alive) return 0.0f;
    if (hp <= 0.0f) {
        hp = 0.0f;
        alive = false;
        return 0.0f;
    }
    if (!(damage > 0.0f)) return 0.0f;
    const float dealt = std::min(hp, damage);
    if (dealt <= 0.0f) return 0.0f;
    hp -= dealt;
    hitFlashTimer = MOB_HIT_FLASH_TIME;
    if (hp <= 0.0f) {
        hp = 0.0f;
        alive = false;
    }
    return dealt;
}

struct EnemyShieldState {
    float hp = 0.0f;
    float maxHp = 0.0f;
    float flashTimer = 0.0f;
    bool acquiredOnce = false;
    bool breakPending = false;
};

inline const glm::vec3 REGULUS_SHIELD_COLOR(0.722f, 0.937f, 1.0f);

inline void SpawnRegulusShieldBreakParticles(float x, float y, glm::vec3 color,
                                             float bodyRadius, bool reinforced) {
    const float radius = std::max(8.0f, bodyRadius * 2.05f);
    EnemyParticleBurst burst;
    burst.count = reinforced ? 36 : 24;
    burst.life = 0.34f;
    burst.startRadius = 1.0f;
    burst.speedMin = radius * 1.05f;
    burst.speedMax = radius * 1.75f;
    burst.sizeMin = 2.5f;
    burst.sizeMax = reinforced ? 9.0f : 7.0f;
    burst.colorLift = 0.50f;
    burst.rangeScale = 1.8f;
    SpawnEnemyParticleBurst(x, y, color, radius, burst);
}

inline void SpawnRegulusLinkBreakParticles(float x, float y, glm::vec3 color) {
    EnemyParticleBurst burst;
    burst.count = 10;
    burst.life = 0.20f;
    burst.randomPlacement = true;
    burst.startRadius = 0.0f;
    burst.speedMin = 75.0f;
    burst.speedMax = 150.0f;
    burst.sizeMin = 1.8f;
    burst.sizeMax = 4.0f;
    burst.colorLift = 0.58f;
    burst.rangeScale = 1.3f;
    SpawnEnemyParticleBurst(x, y, color, 10.0f, burst);
}

inline float ApplyEnemyDamageWithShield(float& hp, bool& alive,
                                        float& hitFlashTimer,
                                        EnemyShieldState& shield,
                                        float damage, float x, float y,
                                        float bodyRadius,
                                        bool reinforcedShield = false) {
    if (!alive) return 0.0f;
    if (hp <= 0.0f) {
        hp = 0.0f;
        alive = false;
        return 0.0f;
    }
    if (!(damage > 0.0f)) return 0.0f;

    float applied = 0.0f;
    float remaining = damage;
    if (shield.hp > 0.0f) {
        const float absorbed = std::min(shield.hp, remaining);
        shield.hp -= absorbed;
        remaining -= absorbed;
        applied += absorbed;
        shield.flashTimer = MOB_HIT_FLASH_TIME;
        if (shield.hp <= 0.0001f) {
            shield.hp = 0.0f;
            shield.breakPending = true;
            SpawnRegulusShieldBreakParticles(x, y, REGULUS_SHIELD_COLOR, bodyRadius,
                                             reinforcedShield);
        }
    }
    if (remaining > 0.0f)
        applied += ApplyMobDamage(hp, alive, hitFlashTimer, remaining);
    return applied;
}
