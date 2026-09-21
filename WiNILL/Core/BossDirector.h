#pragma once
#include <glm/glm.hpp>
#include "Settings.h"

// Active boss picks:
//   2  = VOLLEY
//   8  = FORK
//   10 = TESSERACT
//   20 = ETHER SWORD
//   30 = LEVIATHAN
namespace BossDir {

inline int& RotIdx() {
    static int idx = 0;
    return idx;
}

inline void ResetRotation() { RotIdx() = 0; }

inline bool IsLtsRosterPick(int pick) {
    return pick == 2 || pick == 8 || pick == 10 || pick == 20 || pick == 30;
}

inline int RollScorePick() {
    static const int kRot[] = { 30, 2, 10, 8 };
    return kRot[RotIdx()++ % 4];
}

inline int  g_ActBossPick = -1;
inline int  g_ActClears   = 0;
inline bool g_ActEndless  = false;
inline bool g_TrialMode   = false;

inline void ResetAct() {
    g_ActBossPick = -1;
    g_ActClears   = 0;
    g_ActEndless  = false;
    g_TrialMode   = false;
}

inline void SetActTheme(int pick) { g_ActBossPick = pick; }

// 메인 모드 보스 등장 순서 (LEVIATHAN → FORK → VOLLEY → TESS)
// 최종 보스 추가 시 여기에 pick 번호 추가 + MAIN_ACT_TOTAL 증가
inline constexpr int MAIN_ACT_TOTAL        = 4;
inline constexpr int MAIN_SEQUENCE[]       = { 30, 8, 2, 10 };
inline constexpr float MAIN_SCORE_STEP     = 200000.0f;  // 보스 간 점수 간격

inline void OnBossDefeated() {
    ++g_ActClears;
    if (!g_TrialMode && g_ActClears >= MAIN_ACT_TOTAL) g_ActEndless = true;
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
    case 30: return { 2.8f, 4.8f, 0.95f, 0.95f, 0, 0 };
    case 2:  return { 3.0f, 5.0f, 1.05f, 1.06f, 4, 2 };
    case 10: return { 3.5f, 6.0f, 1.10f, 1.02f, 3, 6 };
    case 8:  return { 4.0f, 7.0f, 1.22f, 1.04f, 2, 10 };
    case 20: return { 4.8f, 9.0f, 1.18f, 1.05f, 4, 8 };
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
    case 30: return L"LEVIATHAN";
    case 8:  return L"FORK";
    case 10: return L"TESSERACT";
    case 20: return L"ETHER SWORD";
    case 2:
    default: return L"VOLLEY";
    }
}

inline float HpMul(int pick) {
    switch (pick) {
    case 30: return 1.00f;
    case 2:  return 1.08f;
    case 10: return 0.92f;
    case 8:  return 0.70f;
    case 20: return 3.20f;
    default: return 1.0f;
    }
}

inline glm::vec3 WarnColor(int pick) {
    switch (pick) {
    case 30: return { 0.20f,  0.78f, 1.0f   };
    case 2:  return { 1.0f,  0.55f, 0.20f };
    case 10: return { 0.95f, 0.35f, 1.0f  };
    case 8:  return { 0.35f, 0.88f, 0.95f };
    case 20: return { 0.55f, 0.80f, 1.0f  };
    default: return { 1.0f,  0.55f, 0.20f };
    }
}

inline const wchar_t* Tagline(int pick) {
    int li = LangIndex();
    if (pick == 30) {
        if (li == 0) return L"별자리를 삼키는 거대한 유영체";
        return L"A star-eating leviathan crosses the constellation";
    }
    if (pick == 8) {
        if (li == 0) return L"별자리 포크 - 연쇄 분열";
        return L"Constellation split - chained star fragments";
    }
    if (pick == 10) {
        return L"Tesseract fracture - dash through collapsing geometry";
    }
    if (pick == 20) {
        if (li == 0) return L"신호 소멸 — 생존자 없음";
        return L"Signal extinguished — no survivors expected";
    }
    if (li == 0) return L"연발 포격 - 사거리 전조 표시";
    return L"Volley fire - telegraphed danger zones";
}

inline bool ActBiasSplitter()  { return g_ActBossPick == 8; }
inline bool ActBiasSpawner()   { return g_ActBossPick == 8 || g_ActBossPick == 10; }
inline bool ActBiasRanged()    { return g_ActBossPick == 2 || g_ActBossPick == 10 || g_ActBossPick == 20; }

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
