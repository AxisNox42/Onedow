#pragma once

enum class SkillType;

struct PlayerRuntimeState {
    float dashCooldown = 0.0f;
    float dashInvulnerability = 0.0f;
    float postPickGrace = 0.0f;
    float timeStopTimer = 0.0f;
    float hyperFocusTimer = 0.0f;
    bool dashActive = false;
    float dashProgress = 0.0f;
    float dashFromX = 0.0f, dashFromY = 0.0f;
    float dashToX = 0.0f, dashToY = 0.0f;
    int dashBoostShotsLeft = 0;
    bool dashInputHeld = false;
    bool skillInputHeld[3] = {};
};

extern PlayerRuntimeState g_PlayerRuntime;

struct PlayerViewportBounds {
    float centerX = 0.0f, centerY = 0.0f;
    float halfWidth = 0.0f, halfHeight = 0.0f;
    float left = 0.0f, right = 0.0f, top = 0.0f, bottom = 0.0f;
};

inline constexpr float DASH_CD = 2.9f;
inline constexpr float DASH_DIST = 300.0f;
inline constexpr float DASH_INVULN = 0.20f;
inline constexpr float DASH_DUR = 0.11f;
inline constexpr float TIMESTOP_DUR = 1.5f;
inline constexpr float HYPER_FOCUS_DUR = 5.0f;
inline constexpr float HYPER_FOCUS_DASH_CD = 2.0f;

float PlayerSkillCooldownMax(SkillType type);
void ResetPlayerSkills();
void ResetPlayerInputEdges();
void UpdatePlayerRuntimeTimers(float delta);
float UpdatePlayerMovement(float& playerX, float& playerY,
                           bool moveUp, bool moveDown,
                           bool moveLeft, bool moveRight,
                           bool canMove, float moveSpeed, float delta,
                           bool dashActive, float& directionX, float& directionY);
PlayerViewportBounds GetPlayerViewportBounds(float screenWidth,
                                              float screenHeight,
                                              float zoom, float topInset,
                                              float bottomInset);
void ClampPlayerToViewport(float& playerX, float& playerY,
                           float playerWidth, float playerHeight,
                           const PlayerViewportBounds& bounds);
void ApplyPlayerDamageProtection(float hpBefore, float& hpAfter,
                                 bool invulnerable, bool lightStep,
                                 float lightStepHitLock,
                                 float& lightStepDisableTimer);
void BeginPlayerDash(float playerX, float playerY, float directionX, float directionY,
                     float minX, float maxX, float minY, float maxY);
void AdvancePlayerDash(float delta, float& playerX, float& playerY);
