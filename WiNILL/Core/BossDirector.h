#pragma once
#include <glm/glm.hpp>
#include "Settings.h"

// Active boss picks:
//   2 = VOLLEY.sys
//   8 = FORK.worm
namespace BossDir {

inline int& RotIdx() {
    static int idx = 0;
    return idx;
}

inline void ResetRotation() { RotIdx() = 0; }

inline bool IsLtsRosterPick(int pick) {
    return pick == 2 || pick == 8;
}

inline int RollScorePick() {
    static const int kRot[] = { 2, 8 };
    return kRot[RotIdx()++ % 2];
}

inline int  g_ActBossPick = -1;
inline int  g_ActClears   = 0;
inline bool g_ActEndless  = false;

inline void ResetAct() {
    g_ActBossPick = -1;
    g_ActClears   = 0;
    g_ActEndless  = false;
}

inline void SetActTheme(int pick) { g_ActBossPick = pick; }

inline void OnBossDefeated() {
    ++g_ActClears;
    if (g_ActClears >= 5) g_ActEndless = true;
}

struct ActRules {
    float intensityCap;
    float hpIntensityCap;
    float spawnMult;
    float speedMult;
    int   eliteBias;
    int   varietyBias;
};

inline ActRules WarmupRules() {
    return { 0.55f, 1.2f, 0.88f, 0.92f, 0, 0 };
}

inline ActRules EndlessRules() {
    return { 6.0f, 18.0f, 1.08f, 1.0f, 8, 6 };
}

inline ActRules RulesForPick(int pick) {
    switch (pick) {
    case 2:  return { 3.0f, 5.0f, 1.05f, 1.06f, 4, 2 };
    case 8:  return { 4.0f, 7.0f, 1.22f, 1.04f, 2, 10 };
    default: return { 2.0f, 3.0f, 1.0f,  1.0f,  2, 2 };
    }
}

inline ActRules GetActRules() {
    if (g_ActEndless) return EndlessRules();
    if (g_ActBossPick < 0) return WarmupRules();
    return RulesForPick(g_ActBossPick);
}

inline int GetActNumber() {
    if (g_ActEndless) return 6;
    if (g_ActBossPick < 0) return 0;
    return g_ActClears + 1;
}

inline const wchar_t* DisplayName(int pick) {
    switch (pick) {
    case 8:  return L"FORK.worm";
    case 2:
    default: return L"VOLLEY.sys";
    }
}

inline float HpMul(int pick) {
    switch (pick) {
    case 2:  return 1.08f;
    case 8:  return 0.70f;
    default: return 1.0f;
    }
}

inline glm::vec3 WarnColor(int pick) {
    switch (pick) {
    case 2:  return { 1.0f,  0.55f, 0.20f };
    case 8:  return { 0.35f, 0.88f, 0.95f };
    default: return { 1.0f,  0.55f, 0.20f };
    }
}

inline const wchar_t* Tagline(int pick) {
    int li = LangIndex();
    if (pick == 8) {
        if (li == 0) return L"프로세스 포크 - 연쇄 분열";
        return L"Fork bomb - chained child processes";
    }
    if (li == 0) return L"연발 포격 - 사거리 전조 표시";
    return L"Volley fire - telegraphed danger zones";
}

inline bool ActBiasSplitter()  { return g_ActBossPick == 8; }
inline bool ActBiasSpawner()   { return g_ActBossPick == 8; }
inline bool ActBiasRanged()    { return g_ActBossPick == 2; }

inline const wchar_t* ActLabel() {
    int li = LangIndex();
    if (g_ActEndless) {
        if (li == 0) return L"ACT INF - ENDLESS";
        return L"ACT INF - ENDLESS";
    }
    if (g_ActBossPick < 0) {
        if (li == 0) return L"ACT 0 - BOOT";
        return L"ACT 0 - BOOT";
    }
    return DisplayName(g_ActBossPick);
}

} // namespace BossDir
