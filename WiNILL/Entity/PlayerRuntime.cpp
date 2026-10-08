#include "PlayerRuntime.h"

#include "../Core/Scenes/SceneSkills.h"
#include "PlayerStats.h"
#include <algorithm>
#include <cmath>

PlayerRuntimeState g_PlayerRuntime;

float PlayerSkillCooldownMax(SkillType type) {
    switch (type) {
    case SkillType::CLOSE_WINDOW: return 16.0f;
    case SkillType::HYPER_FOCUS:  return 20.0f;
    case SkillType::TIME_STOP:    return 28.0f;
    default:                      return 0.0f;
    }
}

void ResetPlayerSkills() {
    for (int i = 0; i < 3; i++) g_Skills[i] = { SkillType::NONE, 0.0f };
    g_SkillReplaceIdx = 0;
    g_PlayerRuntime.dashCooldown = 0.0f;
    g_PlayerRuntime.dashInvulnerability = 0.0f;
    g_PlayerRuntime.dashActive = false;
    g_PlayerRuntime.dashProgress = 0.0f;
    g_PlayerRuntime.dashBoostShotsLeft = 0;
    g_PlayerRuntime.knockbackVX = g_PlayerRuntime.knockbackVY = 0.0f;
    g_PlayerShield = 0.0f;
    g_PlayerShieldTimer = 0.0f;
    g_PlayerRuntime.timeStopTimer = 0.0f;
    g_PlayerRuntime.hyperFocusTimer = 0.0f;
}

void ResetPlayerInputEdges() {
    g_PlayerRuntime.dashInputHeld = false;
    for (bool& held : g_PlayerRuntime.skillInputHeld) held = false;
}

void UpdatePlayerRuntimeTimers(float delta) {
    if (g_PlayerRuntime.dashCooldown > 0.0f) g_PlayerRuntime.dashCooldown -= delta;
    if (g_PlayerRuntime.dashInvulnerability > 0.0f) g_PlayerRuntime.dashInvulnerability -= delta;
    if (g_PlayerRuntime.postPickGrace > 0.0f) g_PlayerRuntime.postPickGrace -= delta;
    if (g_PlayerRuntime.timeStopTimer > 0.0f) g_PlayerRuntime.timeStopTimer -= delta;
    if (g_PlayerRuntime.hyperFocusTimer > 0.0f) g_PlayerRuntime.hyperFocusTimer -= delta;
    for (int i = 0; i < 3; i++)
        if (g_Skills[i].cd > 0.0f) g_Skills[i].cd -= delta;
}

float UpdatePlayerMovement(float& playerX, float& playerY,
                           bool moveUp, bool moveDown,
                           bool moveLeft, bool moveRight,
                           bool canMove, float moveSpeed, float delta,
                           bool dashActive, float& directionX, float& directionY) {
    directionX = (moveRight ? 1.0f : 0.0f) - (moveLeft ? 1.0f : 0.0f);
    directionY = (moveDown ? 1.0f : 0.0f) - (moveUp ? 1.0f : 0.0f);
    const float inputLength = std::sqrt(directionX * directionX + directionY * directionY);
    if (inputLength > 0.001f) {
        directionX /= inputLength;
        directionY /= inputLength;
        if (canMove && !dashActive) {
            playerX += directionX * moveSpeed * delta;
            playerY += directionY * moveSpeed * delta;
        }
    }
    return inputLength;
}

PlayerViewportBounds GetPlayerViewportBounds(float screenWidth,
                                              float screenHeight,
                                              float zoom, float topInset,
                                              float bottomInset) {
    const float safeZoom = std::max(zoom, 0.01f);
    PlayerViewportBounds bounds;
    bounds.centerX = screenWidth * 0.5f;
    bounds.centerY = screenHeight * 0.5f;
    bounds.halfWidth = screenWidth * 0.5f / safeZoom;
    bounds.halfHeight = screenHeight * 0.5f / safeZoom;
    bounds.left = bounds.centerX - bounds.halfWidth;
    bounds.right = bounds.centerX + bounds.halfWidth;
    bounds.top = bounds.centerY + (topInset - bounds.centerY) / safeZoom;
    bounds.bottom = bounds.centerY + bounds.halfHeight - bottomInset / safeZoom;
    return bounds;
}

void ClampPlayerToViewport(float& playerX, float& playerY,
                           float playerWidth, float playerHeight,
                           const PlayerViewportBounds& bounds) {
    float playerCenterX = playerX + playerWidth * 0.5f;
    float playerCenterY = playerY + playerHeight * 0.5f;
    if (playerCenterX < bounds.left) playerCenterX = bounds.left;
    if (playerCenterX > bounds.right) playerCenterX = bounds.right;
    if (playerCenterY < bounds.top) playerCenterY = bounds.top;
    if (playerCenterY > bounds.bottom) playerCenterY = bounds.bottom;
    playerX = playerCenterX - playerWidth * 0.5f;
    playerY = playerCenterY - playerHeight * 0.5f;
}

void ApplyPlayerDamageProtection(float hpBefore, float& hpAfter,
                                 bool invulnerable, bool lightStep,
                                 float lightStepHitLock,
                                 float& lightStepDisableTimer) {
    if (invulnerable && hpAfter < hpBefore) hpAfter = hpBefore;
    if (lightStep && hpAfter < hpBefore)
        lightStepDisableTimer = lightStepHitLock;
}

void BeginPlayerDash(float playerX, float playerY,
                     float directionX, float directionY,
                     float minX, float maxX, float minY, float maxY) {
    g_PlayerRuntime.dashFromX = playerX;
    g_PlayerRuntime.dashFromY = playerY;
    g_PlayerRuntime.dashToX = playerX + directionX * DASH_DIST;
    g_PlayerRuntime.dashToY = playerY + directionY * DASH_DIST;
    if (g_PlayerRuntime.dashToX < minX) g_PlayerRuntime.dashToX = minX;
    if (g_PlayerRuntime.dashToX > maxX) g_PlayerRuntime.dashToX = maxX;
    if (g_PlayerRuntime.dashToY < minY) g_PlayerRuntime.dashToY = minY;
    if (g_PlayerRuntime.dashToY > maxY) g_PlayerRuntime.dashToY = maxY;
    g_PlayerRuntime.dashActive = true;
    g_PlayerRuntime.dashProgress = 0.0f;
    g_PlayerRuntime.dashCooldown = g_PlayerRuntime.hyperFocusTimer > 0.0f
        ? HYPER_FOCUS_DASH_CD : DASH_CD;
    g_PlayerRuntime.dashInvulnerability = DASH_INVULN;
}

void AdvancePlayerDash(float delta, float& playerX, float& playerY) {
    g_PlayerRuntime.dashProgress += delta / DASH_DUR;
    if (g_PlayerRuntime.dashProgress >= 1.0f) {
        g_PlayerRuntime.dashProgress = 1.0f;
        g_PlayerRuntime.dashActive = false;
    }
    const float progress = g_PlayerRuntime.dashProgress;
    playerX = g_PlayerRuntime.dashFromX +
        (g_PlayerRuntime.dashToX - g_PlayerRuntime.dashFromX) * progress;
    playerY = g_PlayerRuntime.dashFromY +
        (g_PlayerRuntime.dashToY - g_PlayerRuntime.dashFromY) * progress;
    g_PlayerRuntime.dashInvulnerability = DASH_INVULN;
}

// Exponential decay: total travel = initial velocity / decay rate, so the
// requested offset is covered in about a quarter second.
static constexpr float KNOCKBACK_DECAY = 12.0f;

void ApplyPlayerKnockback(float offsetX, float offsetY) {
    g_PlayerRuntime.knockbackVX += offsetX * KNOCKBACK_DECAY;
    g_PlayerRuntime.knockbackVY += offsetY * KNOCKBACK_DECAY;
}

void AdvancePlayerKnockback(float delta, float& playerX, float& playerY) {
    playerX += g_PlayerRuntime.knockbackVX * delta;
    playerY += g_PlayerRuntime.knockbackVY * delta;
    const float keep = std::exp(-KNOCKBACK_DECAY * delta);
    g_PlayerRuntime.knockbackVX *= keep;
    g_PlayerRuntime.knockbackVY *= keep;
    if (std::abs(g_PlayerRuntime.knockbackVX) < 1.0f &&
        std::abs(g_PlayerRuntime.knockbackVY) < 1.0f)
        g_PlayerRuntime.knockbackVX = g_PlayerRuntime.knockbackVY = 0.0f;
}
