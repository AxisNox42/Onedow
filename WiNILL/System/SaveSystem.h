#pragma once
// ─────────────────────────────────────────────────────────────
// 저장 시스템 — 설정 + 기록을 exe 옆 텍스트 파일(key=value)로 영구 저장.
//   PlatformChdirToExeDir() 로 작업 폴더가 exe 위치라 단일 파일로 따라다님.
// ─────────────────────────────────────────────────────────────
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <filesystem>
#include <system_error>
#include "Settings.h"
#include "Meta.h"
#include "Achievements.h"
#include "Codex.h"

// ── 세이브 난독화(F25) — 평문 편집 방지. 롤링 XOR(키는 exe 내장 상수).
//   강한 암호는 아니지만 일반 사용자가 메모장으로 점수/코인 조작 못 하게 함.
//   XOR 은 자기역원이라 같은 함수로 암·복호화. (자기 자신 호출)
//   키는 마스킹된 바이트로만 exe 에 남는다(문자열 검색으로 바로 보이지 않게).
namespace save_detail {
inline constexpr int kKeyLen = 35;
inline constexpr char kKeyMask = 0x5A;
struct MaskedKey { char bytes[kKeyLen]; };
constexpr MaskedKey MaskKey(const char (&plain)[kKeyLen + 1]) {
    MaskedKey k{};
    for (int i = 0; i < kKeyLen; ++i) k.bytes[i] = (char)(plain[i] ^ kKeyMask);
    return k;
}
inline constexpr MaskedKey kKey = MaskKey("0n3d0w-d3sktop-d3f3ns3-x0r-k3y-2026");

// CRC-32 (IEEE) of the plain text, salted so a hand-edited file fails.
inline unsigned int Checksum(const std::string& text) {
    unsigned int crc = 0xFFFFFFFFu ^ 0x0D3A5E11u;
    for (unsigned char c : text) {
        crc ^= c;
        for (int b = 0; b < 8; ++b) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}
}  // namespace save_detail

inline void OdwCrypt(std::string& s) {
    using namespace save_detail;
    for (size_t i = 0; i < s.size(); i++)
        s[i] = (char)((unsigned char)s[i]
                      ^ (unsigned char)(kKey.bytes[i % kKeyLen] ^ kKeyMask)
                      ^ (unsigned char)((i * 31u + 7u) & 0xFF));
}
// ODW2 = magic + CRC-32(plain, little endian) + encrypted plain text.
// ODW1 (no checksum) still loads once and is rewritten as ODW2 on the next
// save. Plain-text files are no longer accepted.
inline const char ODW_MAGIC[4]        = { 'O','D','W','2' };
inline const char ODW_MAGIC_LEGACY[4] = { 'O','D','W','1' };

// Wraps plain key=value text in the on-disk ODW2 format.
inline std::string EncodeSaveText(std::string text) {
    const unsigned int crc = save_detail::Checksum(text);
    OdwCrypt(text);
    std::string out(ODW_MAGIC, 4);
    for (int i = 0; i < 4; ++i) out += (char)((crc >> (8 * i)) & 0xFF);
    return out + text;
}

// False when the bytes are not a valid save: unknown magic (plain text
// included) or a checksum mismatch.
inline bool DecodeSaveText(const std::string& raw, std::string& text) {
    if (raw.size() >= 8 && std::memcmp(raw.data(), ODW_MAGIC, 4) == 0) {
        unsigned int crc = 0;
        for (int i = 0; i < 4; ++i)
            crc |= (unsigned int)(unsigned char)raw[4 + i] << (8 * i);
        text = raw.substr(8);
        OdwCrypt(text);
        return save_detail::Checksum(text) == crc;
    }
    if (raw.size() >= 4 && std::memcmp(raw.data(), ODW_MAGIC_LEGACY, 4) == 0) {
        text = raw.substr(4);
        OdwCrypt(text);
        return true;
    }
    return false;
}

// 영구 데이터 (설정은 Settings.h 전역을 그대로 직렬화)
inline long long g_BestScore[3] = { 0, 0, 0 };   // 난이도별 최고 점수 (EASY/NORMAL/HARD)
inline long long g_TotalKills   = 0;             // 누적 처치 수
inline long long g_TotalGames   = 0;             // 누적 플레이 횟수
inline long long g_LastRunCoins = 0;             // 직전 판 획득 코인 (GAMEOVER 표시용)
inline long long g_RunStardust  = 0;             // 현재 런에서 획득한 별가루

// 무기별 기록 [0]=RIFLE [1]=STATIC FIELD
inline long long g_WeaponBestScore[2]  = { 0, 0 };
inline long long g_WeaponBestKills[2]  = { 0, 0 };  // 한 판 최다 처치
inline long long g_WeaponTotalKills[2] = { 0, 0 };  // 누적 처치
inline long long g_WeaponRunCount[2]   = { 0, 0 };  // 플레이 횟수

inline const char* SaveFilePath() { return "onedow_save.cfg"; }
inline std::string SaveBackupPath() { return std::string(SaveFilePath()) + ".bak"; }
inline std::string SaveCorruptPath() { return std::string(SaveFilePath()) + ".corrupt"; }

inline void SaveGame() {
    if (g_SmokeCapture) return;   // 스모크 세션은 사용자 세이브를 건드리지 않음
    // 1) 평문(key=value) 을 문자열로 빌드
    std::string buf;
    char ln[96];
    auto add = [&](const char* fmt, long long v) {
        std::snprintf(ln, sizeof(ln), fmt, v); buf += ln;
    };
    add("lang=%lld\n",      (int)g_Language);
    add("fps=%lld\n",       g_FpsCap);
    add("crosshair=%lld\n", g_ShowCrosshair     ? 1 : 0);
    add("combo=%lld\n",     g_ShowCombo         ? 1 : 0);
    add("soundvol=%lld\n",  g_SoundVol);
    add("bgmenabled=%lld\n", g_BgmEnabled ? 1 : 0);
    add("sfxenabled=%lld\n", g_SfxEnabled ? 1 : 0);
    add("audiomono=%lld\n", g_AudioMonoOutput ? 1 : 0);
    add("audioenabled=%lld\n", g_AudioEngineEnabled ? 1 : 0);
    add("autoskill=%lld\n", g_AutoSkill ? 1 : 0);
    add("strongmenudim=%lld\n", g_StrongMenuDim ? 1 : 0);
    add("backdropblur=%lld\n", g_BackdropBlurEnabled ? 1 : 0);
    add("blurcapturehz=%lld\n", g_BackdropBlurCaptureHz);
    add("debugmode=%lld\n",    g_DebugMode ? 1 : 0);
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
    for (int i = 0; i < 2; i++) {
        std::snprintf(ln, sizeof(ln), "wbscore%d=%lld\n",  i, g_WeaponBestScore[i]);  buf += ln;
        std::snprintf(ln, sizeof(ln), "wbkills%d=%lld\n",  i, g_WeaponBestKills[i]);  buf += ln;
        std::snprintf(ln, sizeof(ln), "wtkills%d=%lld\n",  i, g_WeaponTotalKills[i]); buf += ln;
        std::snprintf(ln, sizeof(ln), "wruns%d=%lld\n",    i, g_WeaponRunCount[i]);    buf += ln;
    }
    add("themeowned=%lld\n", (long long)g_ThemeOwned);
    add("themesel=%lld\n",   (long long)g_ThemeSel);
    for (int i = 0; i < META_COUNT; i++) {
        std::snprintf(ln, sizeof(ln), "meta%d=%d\n", i, g_MetaLv[i]); buf += ln;
    }
    for (int i = 0; i < ACH_COUNT; i++) {
        std::snprintf(ln, sizeof(ln), "ach%d=%d\n", i, g_AchUnlocked[i] ? 1 : 0); buf += ln;
    }
    for (int i = 0; i < AUG_TOTAL; i++)
        if (g_AugSeen[i]) { std::snprintf(ln, sizeof(ln), "augseen%d=1\n", i); buf += ln; }
    for (int i = 0; i < CM_COUNT; i++)
        if (g_MobSeen[i]) { std::snprintf(ln, sizeof(ln), "mobseen%d=1\n", i); buf += ln; }
    for (int i = 0; i < CM_COUNT; i++)
        if (g_MobKillCounts[i] > 0) {
            std::snprintf(ln, sizeof(ln), "mobkills%d=%lld\n", i, g_MobKillCounts[i]);
            buf += ln;
        }
    for (int i = 1; i < JOB_COUNT; i++)
        if (g_JobBought[i]) { std::snprintf(ln, sizeof(ln), "jobbought%d=1\n", i); buf += ln; }

    // 2) 체크섬 + 난독화 후 바이너리(매직+CRC+암호문)로 기록
    buf = EncodeSaveText(std::move(buf));
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable:4996)
#endif
    // Write a temporary file first and swap it in only after every byte is
    // on disk, so a crash or power loss mid-write cannot corrupt the save.
    const std::string tmpPath = std::string(SaveFilePath()) + ".tmp";
    FILE* f = std::fopen(tmpPath.c_str(), "wb");
#ifdef _MSC_VER
#  pragma warning(pop)
#endif
    if (!f) return;
    bool ok = std::fwrite(buf.data(), 1, buf.size(), f) == buf.size();
    ok = (std::fflush(f) == 0) && ok;
    ok = (std::fclose(f) == 0) && ok;
    std::error_code ec;
    if (ok) {
        // The previous good save becomes the backup; a crash between the two
        // renames leaves the backup for LoadGame to fall back on.
        if (std::filesystem::exists(SaveFilePath(), ec))
            std::filesystem::rename(SaveFilePath(), SaveBackupPath(), ec);
        ec.clear();
        std::filesystem::rename(tmpPath, SaveFilePath(), ec);
    }
    if (!ok || ec) std::filesystem::remove(tmpPath, ec);
}

inline bool ReadSaveFile(const std::string& path, std::string& text) {
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable:4996)
#endif
    FILE* f = std::fopen(path.c_str(), "rb");
#ifdef _MSC_VER
#  pragma warning(pop)
#endif
    if (!f) return false;
    // Real saves are a few KB; anything far larger is not ours.
    constexpr size_t kMaxSaveBytes = 1u << 20;
    std::string raw;
    char tmp[1024]; size_t n;
    while (raw.size() <= kMaxSaveBytes && (n = std::fread(tmp, 1, sizeof(tmp), f)) > 0)
        raw.append(tmp, n);
    std::fclose(f);
    return raw.size() <= kMaxSaveBytes && DecodeSaveText(raw, text);
}

inline void LoadGame() {
    // 본 파일이 손상(체크섬 불일치·평문·쓰레기)이면 .corrupt 로 치워 두고
    // 직전 백업을 읽는다. 다음 저장이 손상본으로 백업을 덮어쓰지 않게 하기 위함.
    std::string content;
    std::error_code ec;
    if (!ReadSaveFile(SaveFilePath(), content)) {
        if (std::filesystem::exists(SaveFilePath(), ec)) {
            std::filesystem::remove(SaveCorruptPath(), ec);
            std::filesystem::rename(SaveFilePath(), SaveCorruptPath(), ec);
        }
        if (!ReadSaveFile(SaveBackupPath(), content)) return;
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
        else if (!std::strcmp(key, "fps")) {
            // Only the caps the settings screen offers (0 = VSync, -1 = legacy unlimited).
            if (val == 0 || val == 30 || val == 60 || val == 144 || val == 300 || val == -1)
                g_FpsCap = (int)val;
        }
        else if (!std::strcmp(key, "crosshair"))   g_ShowCrosshair     = (val != 0);
        else if (!std::strcmp(key, "dmgnum"))      g_ShowDamageNumbers = true;
        else if (!std::strcmp(key, "combo"))       g_ShowCombo         = (val != 0);
        else if (!std::strcmp(key, "soundvol"))    g_SoundVol          = (int)(val < 0 ? 0 : val > 100 ? 100 : val);
        else if (!std::strcmp(key, "bgmenabled"))  g_BgmEnabled        = (val != 0);
        else if (!std::strcmp(key, "sfxenabled"))  g_SfxEnabled        = (val != 0);
        else if (!std::strcmp(key, "audiomono"))   g_AudioMonoOutput   = (val != 0);
        else if (!std::strcmp(key, "audioenabled")) g_AudioEngineEnabled = (val != 0);
        else if (!std::strcmp(key, "autoskill"))   g_AutoSkill         = (val != 0);
        else if (!std::strcmp(key, "strongmenudim")) g_StrongMenuDim    = (val != 0);
        else if (!std::strcmp(key, "backdropblur")) g_BackdropBlurEnabled = (val != 0);
        else if (!std::strcmp(key, "blurcapturehz")) {
            if (val == 1 || val == 20 || val == 60)
                g_BackdropBlurCaptureHz = (int)val;
        }
        else if (!std::strcmp(key, "debugmode"))    g_DebugMode = (val != 0);
        else if (!std::strcmp(key, "shaderfx"))    g_ShaderFx          = (val != 0);
        else if (!std::strcmp(key, "mobstyle"))   { int v = (int)val; if (v >= 0 && v <= 1) g_MobVisualStyle = (MobVisualStyle)v; }
        else if (!std::strcmp(key, "vfxdens"))    { int v = (int)val; if (v >= 0 && v <= 2) g_VfxDensity = (VfxDensity)v; }
        else if (!std::strcmp(key, "macopt"))     g_MacOptV1          = (val != 0);
        // Scores, counts and coins are never negative; a damaged value reads as 0.
        else if (!std::strcmp(key, "best_easy"))   g_BestScore[0]      = ClampSaveValue(val);
        else if (!std::strcmp(key, "best_normal")) g_BestScore[1]      = ClampSaveValue(val);
        else if (!std::strcmp(key, "best_hard"))   g_BestScore[2]      = ClampSaveValue(val);
        else if (!std::strcmp(key, "kills"))       g_TotalKills        = ClampSaveValue(val);
        else if (!std::strcmp(key, "games"))       g_TotalGames        = ClampSaveValue(val);
        else if (!std::strncmp(key, "wbscore", 7))  { int i = atoi(key+7); if (i>=0&&i<2) g_WeaponBestScore[i]  = ClampSaveValue(val); }
        else if (!std::strncmp(key, "wbkills", 7))  { int i = atoi(key+7); if (i>=0&&i<2) g_WeaponBestKills[i]  = ClampSaveValue(val); }
        else if (!std::strncmp(key, "wtkills", 7))  { int i = atoi(key+7); if (i>=0&&i<2) g_WeaponTotalKills[i] = ClampSaveValue(val); }
        else if (!std::strncmp(key, "wruns",   5))  { int i = atoi(key+5); if (i>=0&&i<2) g_WeaponRunCount[i]   = ClampSaveValue(val); }
        else if (!std::strcmp(key, "coins"))       g_Coins             = ClampSaveValue(val);
        else if (!std::strcmp(key, "themeowned"))  g_ThemeOwned        = (int)val | 1;
        // Ownership is checked by ApplyAccentTheme (themeowned may load later).
        else if (!std::strcmp(key, "themesel"))  { if (val >= 0 && val < ACCENT_COUNT) g_ThemeSel = (int)val; }
        else if (!std::strcmp(key, "bosskills")) { (void)val; }
        else if (!std::strncmp(key, "meta", 4)) {
            int mi = atoi(key + 4);
            if (mi >= 0 && mi < META_COUNT)
                g_MetaLv[mi] = (int)(val < 0 ? 0 : val > META_DEFS[mi].maxLv
                                                   ? META_DEFS[mi].maxLv : val);
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
        else if (!std::strncmp(key, "mobkills", 8)) {
            int mi = atoi(key + 8);
            if (mi >= 0 && mi < CM_COUNT) g_MobKillCounts[mi] = ClampSaveValue(val);
        }
        else if (!std::strncmp(key, "bossseen", 8)) { (void)val; }
        else if (!std::strncmp(key, "jobbought", 9)) {
            int ji = atoi(key + 9);
            if (ji >= 1 && ji < JOB_COUNT) g_JobBought[ji] = (val != 0);
        }
    }
}

// 세이브 진행도 초기화 — 점수/코인/메타/업적/도감/테마를 전부 리셋(설정/언어는 유지).
//   설정 화면의 "세이브 초기화" 버튼에서 호출. 즉시 파일에도 반영.
inline void ResetSaveProgress() {
    for (int i = 0; i < 3; i++) g_BestScore[i] = 0;
    g_TotalKills = 0; g_TotalGames = 0;
    for (int i = 0; i < 2; i++) {
        g_WeaponBestScore[i] = g_WeaponBestKills[i] = g_WeaponTotalKills[i] = g_WeaponRunCount[i] = 0;
    }
    g_Coins = 0; g_LastRunCoins = 0;
    for (int i = 0; i < META_COUNT; i++) g_MetaLv[i] = 0;
    for (int i = 0; i < ACH_COUNT;  i++) g_AchUnlocked[i] = false;
    for (int i = 0; i < AUG_TOTAL;  i++) g_AugSeen[i] = false;
    for (int i = 0; i < CM_COUNT;   i++) g_MobSeen[i] = false;
    for (int i = 0; i < CM_COUNT;   i++) {
        g_MobKillCounts[i] = 0;
        g_RunMobKillCounts[i] = 0;
    }
    g_ThemeOwned = 1; g_ThemeSel = 0; ApplyAccentTheme();
    for (int i = 0; i < JOB_COUNT; i++) g_JobBought[i] = false;
    SaveGame();
}

// 한 판 종료 시 호출 — 최고점/누적 기록 갱신 후 저장. 신기록이면 true.
// weaponIdx: 0=RIFLE, 1=STATIC FIELD, -1=미지정
inline bool RecordRunResult(int difficultyIdx, long long score, long long kills, int weaponIdx = -1) {
    if (difficultyIdx < 0 || difficultyIdx > 2) difficultyIdx = 1;
    score = ClampSaveValue(score);
    kills = ClampSaveValue(kills);
    bool isRecord = (score > g_BestScore[difficultyIdx]);
    if (isRecord) g_BestScore[difficultyIdx] = score;
    AddSaveValue(g_TotalKills, kills);
    AddSaveValue(g_TotalGames, 1);
    if (weaponIdx >= 0 && weaponIdx < 2) {
        if (score > g_WeaponBestScore[weaponIdx])  g_WeaponBestScore[weaponIdx]  = score;
        if (kills > g_WeaponBestKills[weaponIdx])  g_WeaponBestKills[weaponIdx]  = kills;
        AddSaveValue(g_WeaponTotalKills[weaponIdx], kills);
        AddSaveValue(g_WeaponRunCount[weaponIdx], 1);
    }
    for (int i = 0; i < CM_COUNT; ++i) {
        AddSaveValue(g_MobKillCounts[i], g_RunMobKillCounts[i]);
        g_RunMobKillCounts[i] = 0;
    }
    // 코인 적립 — 점수/1000 + 처치/2 (난이도 보너스: 보통×1.2, 어려움×1.5)
    float diffMul = (difficultyIdx == 2) ? 1.5f : (difficultyIdx == 1) ? 1.2f : 1.0f;
    long long earned = (long long)((score / 1000 + kills / 2) * diffMul);
    AddSaveValue(g_Coins, earned);
    g_LastRunCoins = earned;
    AddSaveValue(g_RunStardust, earned);
    SaveGame();
    return isRecord;
}
