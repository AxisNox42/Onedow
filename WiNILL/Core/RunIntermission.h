#pragma once

#include "GameManager.h"
#include "BossDirector.h"
#include "MonsterManager.h"

extern GameManager g_GameManager;

// Boss intermissions stay behind kBossEncountersEnabled so the encounter
// system can be reactivated later without restoring retired run-state UI.
constexpr float INTERMISSION_DUR     = 120.0f;
constexpr float SKIP_ZONE_RADIUS     = 78.0f;
constexpr float SKIP_ZONE_HOLD_S     = 0.45f;

inline bool  g_InBossIntermission = false;
inline float g_IntermissionTimer  = 0.0f;
inline float g_SkipZoneHold       = 0.0f;
inline float g_SkipZoneX          = 0.0f;
inline float g_SkipZoneY          = 0.0f;

inline bool InBossRest() {
    return g_InBossIntermission;
}

inline bool PlayerInZone(float px, float py, float zx, float zy, float r) {
    const float dx = px - zx;
    const float dy = py - zy;
    return dx * dx + dy * dy <= r * r;
}

inline void PlaceIntermissionZones(int screenW, int screenH) {
    const float bar = 90.0f;
    const float midY = ((float)screenH - bar) * 0.5f;
    g_SkipZoneX = (float)screenW - 190.0f;
    g_SkipZoneY = midY;
}

inline void EndIntermission() {
    g_InBossIntermission = false;
    g_IntermissionTimer  = 0.0f;
    g_SkipZoneHold       = 0.0f;
}

inline void TickIntermissionZones(float pCX, float pCY, float delta) {
    if (!g_InBossIntermission) return;
    if (PlayerInZone(pCX, pCY, g_SkipZoneX, g_SkipZoneY, SKIP_ZONE_RADIUS)) {
        g_SkipZoneHold += delta;
        if (g_SkipZoneHold >= SKIP_ZONE_HOLD_S)
            EndIntermission();
    } else {
        g_SkipZoneHold = 0.0f;
    }
    if (g_IntermissionTimer <= 0.0f)
        EndIntermission();
}

inline void BeginBossIntermission(int bossPick, int screenW, int screenH) {
    if (!kBossEncountersEnabled) {
        EndIntermission();
        (void)bossPick;
        (void)screenW;
        (void)screenH;
        return;
    }
    g_InBossIntermission = true;
    g_IntermissionTimer  = INTERMISSION_DUR;
    g_SkipZoneHold       = 0.0f;
    PlaceIntermissionZones(screenW, screenH);
    BossDir::OnBossDefeated();
    (void)bossPick;
}

inline void FinishBossKill(MonsterManager& mm, int bossPick,
                           int screenW, int screenH) {
    mm.Clear();
    if (!kBossEncountersEnabled) {
        EndIntermission();
        g_GameManager.currentState = GameState::RUNNING;
        (void)bossPick;
        (void)screenW;
        (void)screenH;
        return;
    }
    BeginBossIntermission(bossPick, screenW, screenH);
    g_GameManager.currentState = GameState::RUNNING;
}
