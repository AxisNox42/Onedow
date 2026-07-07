#pragma once
// ─────────────────────────────────────────────────────────────
// 저장 시스템 — 설정 + 기록을 exe 옆 텍스트 파일(key=value)로 영구 저장.
//   PlatformChdirToExeDir() 로 작업 폴더가 exe 위치라 단일 파일로 따라다님.
// ─────────────────────────────────────────────────────────────
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include "Settings.h"
#include "Meta.h"
#include "Achievements.h"
#include "Codex.h"

// ── 세이브 난독화(F25) — 평문 편집 방지. 롤링 XOR(키는 exe 내장 상수).
//   강한 암호는 아니지만 일반 사용자가 메모장으로 점수/코인 조작 못 하게 함.
//   XOR 은 자기역원이라 같은 함수로 암·복호화. (자기 자신 호출)
inline void OdwCrypt(std::string& s) {
    static const char KEY[] = "0n3d0w-d3sktop-d3f3ns3-x0r-k3y-2026";
    const int kl = (int)sizeof(KEY) - 1;
    for (size_t i = 0; i < s.size(); i++)
        s[i] = (char)((unsigned char)s[i] ^ (unsigned char)KEY[i % kl]
                      ^ (unsigned char)((i * 31u + 7u) & 0xFF));
}
inline const char ODW_MAGIC[4] = { 'O','D','W','1' };

// 영구 데이터 (설정은 Settings.h 전역을 그대로 직렬화)
inline long long g_BestScore[3] = { 0, 0, 0 };   // 난이도별 최고 점수 (EASY/NORMAL/HARD)
inline long long g_TotalKills   = 0;             // 누적 처치 수
inline long long g_TotalGames   = 0;             // 누적 플레이 횟수
inline long long g_LastRunCoins = 0;             // 직전 판 획득 코인 (GAMEOVER 표시용)

inline const char* SaveFilePath() { return "onedow_save.cfg"; }

inline void SaveGame() {
    // 1) 평문(key=value) 을 문자열로 빌드
    std::string buf;
    char ln[96];
    auto add = [&](const char* fmt, long long v) {
        std::snprintf(ln, sizeof(ln), fmt, v); buf += ln;
    };
    add("lang=%lld\n",      (int)g_Language);
    add("fps=%lld\n",       g_FpsCap);
    add("crosshair=%lld\n", g_ShowCrosshair     ? 1 : 0);
    add("dmgnum=%lld\n",    g_ShowDamageNumbers ? 1 : 0);
    add("combo=%lld\n",     g_ShowCombo         ? 1 : 0);
    add("soundvol=%lld\n",  g_SoundVol);
    add("autofire=%lld\n",  g_AutoFire  ? 1 : 0);
    add("autoskill=%lld\n", g_AutoSkill ? 1 : 0);
    add("shaderfx=%lld\n",  g_ShaderFx  ? 1 : 0);
    add("mobstyle=%lld\n",  (int)g_MobVisualStyle);
    add("vfxdens=%lld\n",   (int)g_VfxDensity);
    add("macopt=%lld\n",    g_MacOptV1 ? 1 : 0);
    add("best_easy=%lld\n",   g_BestScore[0]);
    add("best_normal=%lld\n", g_BestScore[1]);
    add("best_hard=%lld\n",   g_BestScore[2]);
    add("kills=%lld\n",   g_TotalKills);
    add("games=%lld\n",   g_TotalGames);
    add("coins=%lld\n",   g_Coins);
    add("themeowned=%lld\n", (long long)g_ThemeOwned);
    add("themesel=%lld\n",   (long long)g_ThemeSel);
    for (int i = 0; i < META_COUNT; i++) {
        std::snprintf(ln, sizeof(ln), "meta%d=%d\n", i, g_MetaLv[i]); buf += ln;
    }
    add("bosskills=%lld\n", g_TotalBossKills);
    for (int i = 0; i < ACH_COUNT; i++) {
        std::snprintf(ln, sizeof(ln), "ach%d=%d\n", i, g_AchUnlocked[i] ? 1 : 0); buf += ln;
    }
    for (int i = 0; i < AUG_TOTAL; i++)
        if (g_AugSeen[i]) { std::snprintf(ln, sizeof(ln), "augseen%d=1\n", i); buf += ln; }
    for (int i = 0; i < CM_COUNT; i++)
        if (g_MobSeen[i]) { std::snprintf(ln, sizeof(ln), "mobseen%d=1\n", i); buf += ln; }
    for (int i = 0; i < 11; i++)
        if (g_BossSeenPick[i]) { std::snprintf(ln, sizeof(ln), "bossseen%d=1\n", i); buf += ln; }

    // 2) 난독화 후 바이너리(매직+암호문)로 기록
    OdwCrypt(buf);
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable:4996)
#endif
    FILE* f = std::fopen(SaveFilePath(), "wb");
#ifdef _MSC_VER
#  pragma warning(pop)
#endif
    if (!f) return;
    std::fwrite(ODW_MAGIC, 1, 4, f);
    if (!buf.empty()) std::fwrite(buf.data(), 1, buf.size(), f);
    std::fclose(f);
}

inline void LoadGame() {
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable:4996)
#endif
    FILE* f = std::fopen(SaveFilePath(), "rb");
#ifdef _MSC_VER
#  pragma warning(pop)
#endif
    if (!f) return;
    // 전체를 읽어 매직 확인 → 난독화면 복호화, 아니면 구버전 평문으로 처리.
    std::string raw;
    {
        char tmp[1024]; size_t n;
        while ((n = std::fread(tmp, 1, sizeof(tmp), f)) > 0) raw.append(tmp, n);
    }
    std::fclose(f);
    std::string content;
    if (raw.size() >= 4 && std::memcmp(raw.data(), ODW_MAGIC, 4) == 0) {
        content = raw.substr(4);
        OdwCrypt(content);            // 복호화 (XOR 자기역원)
    } else {
        content = raw;                // 구버전 평문 세이브 호환
    }
    // 줄 단위 파싱 (기존 sscanf 로직 그대로)
    size_t pos = 0;
    char line[160];
    while (pos < content.size()) {
        size_t nl = content.find('\n', pos);
        if (nl == std::string::npos) nl = content.size();
        size_t len = nl - pos; if (len >= sizeof(line)) len = sizeof(line) - 1;
        std::memcpy(line, content.data() + pos, len); line[len] = '\0';
        pos = nl + 1;
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
        else if (!std::strcmp(key, "soundvol"))    g_SoundVol          = (int)val;
        else if (!std::strcmp(key, "autofire"))    g_AutoFire          = (val != 0);
        else if (!std::strcmp(key, "autoskill"))   g_AutoSkill         = (val != 0);
        else if (!std::strcmp(key, "shaderfx"))    g_ShaderFx          = (val != 0);
        else if (!std::strcmp(key, "mobstyle"))   { int v = (int)val; if (v >= 0 && v <= 1) g_MobVisualStyle = (MobVisualStyle)v; }
        else if (!std::strcmp(key, "vfxdens"))    { int v = (int)val; if (v >= 0 && v <= 1) g_VfxDensity = (VfxDensity)v; }
        else if (!std::strcmp(key, "macopt"))     g_MacOptV1          = (val != 0);
        else if (!std::strcmp(key, "best_easy"))   g_BestScore[0]      = val;
        else if (!std::strcmp(key, "best_normal")) g_BestScore[1]      = val;
        else if (!std::strcmp(key, "best_hard"))   g_BestScore[2]      = val;
        else if (!std::strcmp(key, "kills"))       g_TotalKills        = val;
        else if (!std::strcmp(key, "games"))       g_TotalGames        = val;
        else if (!std::strcmp(key, "coins"))       g_Coins             = val;
        else if (!std::strcmp(key, "themeowned"))  g_ThemeOwned        = (int)val | 1;
        else if (!std::strcmp(key, "themesel"))    g_ThemeSel          = (int)val;
        else if (!std::strcmp(key, "bosskills")) g_TotalBossKills = val;
        else if (!std::strncmp(key, "meta", 4)) {
            int mi = atoi(key + 4);
            if (mi >= 0 && mi < META_COUNT) g_MetaLv[mi] = (int)val;
        }
        else if (!std::strncmp(key, "ach", 3)) {
            int ai = atoi(key + 3);
            if (ai >= 0 && ai < ACH_COUNT) g_AchUnlocked[ai] = (val != 0);
        }
        else if (!std::strncmp(key, "augseen", 7)) {
            int ai = atoi(key + 7);
            if (ai >= 0 && ai < AUG_TOTAL) g_AugSeen[ai] = (val != 0);
        }
        else if (!std::strncmp(key, "mobseen", 7)) {
            int mi = atoi(key + 7);
            if (mi >= 0 && mi < CM_COUNT) g_MobSeen[mi] = (val != 0);
        }
        else if (!std::strncmp(key, "bossseen", 8)) {
            int bi = atoi(key + 8);
            if (bi >= 0 && bi < 11) g_BossSeenPick[bi] = (val != 0);
        }
    }
}

// 세이브 진행도 초기화 — 점수/코인/메타/업적/도감/테마를 전부 리셋(설정/언어는 유지).
//   설정 화면의 "세이브 초기화" 버튼에서 호출. 즉시 파일에도 반영.
inline void ResetSaveProgress() {
    for (int i = 0; i < 3; i++) g_BestScore[i] = 0;
    g_TotalKills = 0; g_TotalGames = 0; g_TotalBossKills = 0;
    g_Coins = 0; g_LastRunCoins = 0;
    for (int i = 0; i < META_COUNT; i++) g_MetaLv[i] = 0;
    for (int i = 0; i < ACH_COUNT;  i++) g_AchUnlocked[i] = false;
    for (int i = 0; i < AUG_TOTAL;  i++) g_AugSeen[i] = false;
    for (int i = 0; i < CM_COUNT;   i++) g_MobSeen[i] = false;
    for (int i = 0; i < 11; i++) g_BossSeenPick[i] = false;
    g_ThemeOwned = 1; g_ThemeSel = 0; ApplyAccentTheme();
    SaveGame();
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
