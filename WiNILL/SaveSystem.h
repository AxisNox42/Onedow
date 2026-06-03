#pragma once
// ─────────────────────────────────────────────────────────────
// 저장 시스템 — 설정 + 기록을 exe 옆 텍스트 파일(key=value)로 영구 저장.
//   PlatformChdirToExeDir() 로 작업 폴더가 exe 위치라 단일 파일로 따라다님.
// ─────────────────────────────────────────────────────────────
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "Settings.h"
#include "Meta.h"
#include "Achievements.h"

// 영구 데이터 (설정은 Settings.h 전역을 그대로 직렬화)
inline long long g_BestScore[3] = { 0, 0, 0 };   // 난이도별 최고 점수 (EASY/NORMAL/HARD)
inline long long g_TotalKills   = 0;             // 누적 처치 수
inline long long g_TotalGames   = 0;             // 누적 플레이 횟수
inline long long g_LastRunCoins = 0;             // 직전 판 획득 코인 (GAMEOVER 표시용)

inline const char* SaveFilePath() { return "onedow_save.cfg"; }

inline void SaveGame() {
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable:4996)
#endif
    FILE* f = std::fopen(SaveFilePath(), "w");
#ifdef _MSC_VER
#  pragma warning(pop)
#endif
    if (!f) return;
    std::fprintf(f, "lang=%d\n",      (int)g_Language);
    std::fprintf(f, "fps=%d\n",       g_FpsCap);
    std::fprintf(f, "crosshair=%d\n", g_ShowCrosshair      ? 1 : 0);
    std::fprintf(f, "dmgnum=%d\n",    g_ShowDamageNumbers  ? 1 : 0);
    std::fprintf(f, "combo=%d\n",     g_ShowCombo          ? 1 : 0);
    std::fprintf(f, "best_easy=%lld\n",   g_BestScore[0]);
    std::fprintf(f, "best_normal=%lld\n", g_BestScore[1]);
    std::fprintf(f, "best_hard=%lld\n",   g_BestScore[2]);
    std::fprintf(f, "kills=%lld\n",   g_TotalKills);
    std::fprintf(f, "games=%lld\n",   g_TotalGames);
    std::fprintf(f, "coins=%lld\n",   g_Coins);
    for (int i = 0; i < META_COUNT; i++)
        std::fprintf(f, "meta%d=%d\n", i, g_MetaLv[i]);
    std::fprintf(f, "bosskills=%lld\n", g_TotalBossKills);
    for (int i = 0; i < ACH_COUNT; i++)
        std::fprintf(f, "ach%d=%d\n", i, g_AchUnlocked[i] ? 1 : 0);
    std::fclose(f);
}

inline void LoadGame() {
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable:4996)
#endif
    FILE* f = std::fopen(SaveFilePath(), "r");
#ifdef _MSC_VER
#  pragma warning(pop)
#endif
    if (!f) return;
    char line[160];
    while (std::fgets(line, sizeof(line), f)) {
        char key[64]; long long val = 0;
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable:4996)
#endif
        int matched = std::sscanf(line, "%63[^=]=%lld", key, &val);
#ifdef _MSC_VER
#  pragma warning(pop)
#endif
        if (matched != 2) continue;
        if      (!std::strcmp(key, "lang"))        { int l = (int)val; if (l >= 0 && l < LANG_COUNT) g_Language = (Language)l; }
        else if (!std::strcmp(key, "fps"))         g_FpsCap            = (int)val;
        else if (!std::strcmp(key, "crosshair"))   g_ShowCrosshair     = (val != 0);
        else if (!std::strcmp(key, "dmgnum"))      g_ShowDamageNumbers = (val != 0);
        else if (!std::strcmp(key, "combo"))       g_ShowCombo         = (val != 0);
        else if (!std::strcmp(key, "best_easy"))   g_BestScore[0]      = val;
        else if (!std::strcmp(key, "best_normal")) g_BestScore[1]      = val;
        else if (!std::strcmp(key, "best_hard"))   g_BestScore[2]      = val;
        else if (!std::strcmp(key, "kills"))       g_TotalKills        = val;
        else if (!std::strcmp(key, "games"))       g_TotalGames        = val;
        else if (!std::strcmp(key, "coins"))       g_Coins             = val;
        else if (!std::strcmp(key, "bosskills")) g_TotalBossKills = val;
        else if (!std::strncmp(key, "meta", 4)) {
            int mi = atoi(key + 4);
            if (mi >= 0 && mi < META_COUNT) g_MetaLv[mi] = (int)val;
        }
        else if (!std::strncmp(key, "ach", 3)) {
            int ai = atoi(key + 3);
            if (ai >= 0 && ai < ACH_COUNT) g_AchUnlocked[ai] = (val != 0);
        }
    }
    std::fclose(f);
}

// 한 판 종료 시 호출 — 최고점/누적 기록 갱신 후 저장. 신기록이면 true.
inline bool RecordRunResult(int difficultyIdx, long long score, long long kills) {
    if (difficultyIdx < 0 || difficultyIdx > 2) difficultyIdx = 1;
    bool isRecord = (score > g_BestScore[difficultyIdx]);
    if (isRecord) g_BestScore[difficultyIdx] = score;
    g_TotalKills += kills;
    g_TotalGames += 1;
    // 코인 적립 — 점수/1000 + 처치/2 (난이도 보너스: 보통×1.2, 어려움×1.5)
    float diffMul = (difficultyIdx == 2) ? 1.5f : (difficultyIdx == 1) ? 1.2f : 1.0f;
    long long earned = (long long)((score / 1000 + kills / 2) * diffMul);
    g_Coins += earned;
    g_LastRunCoins = earned;
    SaveGame();
    return isRecord;
}
