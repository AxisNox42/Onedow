#pragma once
// ─────────────────────────────────────────────────────────────
// FloorDirector — 유한 층(탑) 진행 구조
//   12층, 4층마다 보스. 최종 보스 처치 시 VICTORY.
// ─────────────────────────────────────────────────────────────
#include "Settings.h"

namespace FloorDir {

constexpr int TOTAL_FLOORS  = 12;
constexpr int BOSS_INTERVAL = 4;

inline int   g_CurrentFloor      = 1;
inline int   g_FloorKillCount    = 0;
inline bool  g_FloorClearPending = false;
inline float g_FloorBannerTimer  = 0.0f;

inline void Reset() {
    g_CurrentFloor      = 1;
    g_FloorKillCount    = 0;
    g_FloorClearPending = false;
    g_FloorBannerTimer  = 0.0f;
}

inline bool IsBossFloor(int floor = g_CurrentFloor) {
    return floor > 0 && (floor % BOSS_INTERVAL) == 0;
}

inline bool IsFinalBossFloor() {
    return g_CurrentFloor >= TOTAL_FLOORS && IsBossFloor();
}

inline int KillsRequired(int floor = g_CurrentFloor) {
    return 22 + floor * 4;
}

inline void OnMobKill() {
    if (!g_FloorClearPending)
        g_FloorKillCount++;
}

inline bool IsFloorGoalMet() {
    return g_FloorKillCount >= KillsRequired();
}

inline void BeginNonBossClearBanner() {
    g_FloorClearPending = true;
    g_FloorBannerTimer  = 2.0f;
}

inline void AdvanceFloor() {
    if (g_CurrentFloor < TOTAL_FLOORS) {
        g_CurrentFloor++;
        g_FloorKillCount    = 0;
        g_FloorClearPending = false;
        g_FloorBannerTimer  = 0.0f;
    }
}

inline void TickBanner(float delta) {
    if (g_FloorBannerTimer <= 0.0f) return;
    g_FloorBannerTimer -= delta;
    if (g_FloorBannerTimer <= 0.0f) {
        g_FloorBannerTimer  = 0.0f;
        g_FloorClearPending = false;
        AdvanceFloor();
    }
}

inline const wchar_t* FloorLabel() {
    static wchar_t buf[64];
    int li = LangIndex();
    if (li == 1)
        swprintf_s(buf, L"FLOOR %d / %d", g_CurrentFloor, TOTAL_FLOORS);
    else if (li == 2)
        swprintf_s(buf, L"階層 %d / %d", g_CurrentFloor, TOTAL_FLOORS);
    else
        swprintf_s(buf, L"%d층 / %d층", g_CurrentFloor, TOTAL_FLOORS);
    return buf;
}

inline const wchar_t* ClearBannerText() {
    static const wchar_t* KR = L"층 클리어!";
    static const wchar_t* EN = L"Floor cleared!";
    static const wchar_t* JP = L"階層クリア!";
    int li = LangIndex();
    if (li == 1) return EN;
    if (li == 2) return JP;
    return KR;
}

} // namespace FloorDir
