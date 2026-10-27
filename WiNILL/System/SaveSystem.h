#pragma once
// ─────────────────────────────────────────────────────────────
// 저장 시스템 — 설정 + 기록을 사용자 AppData 폴더의 파일(key=value)로 영구 저장.
//   예전 exe 옆 onedow_save.cfg 는 LoadGame 에서 1회 이전한다.
// ─────────────────────────────────────────────────────────────
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <chrono>
#include <random>
#include <string>
#include <filesystem>
#include <system_error>
#include "Settings.h"
#include "Meta.h"
#include "Achievements.h"
#include "Codex.h"

// ── 세이브 보호 ───────────────────────────────────────────────
//   ODW3 = "ODW3" + nonce(16) + 암호문 + HMAC-SHA256 태그(32).
//   암호문은 HMAC-SHA256(CTR) 키스트림 XOR, 태그는 encrypt-then-MAC 이라
//   키 없이 한 비트라도 고치면 로드 단계에서 거부된다. 키는 exe 내장 비밀값에서
//   파생하며 마스킹된 바이트로만 남는다(문자열 검색으로 바로 보이지 않게).
//   오프라인 게임이라 exe 를 리버싱하면 뚫리지만, 메모장·헥스 에디터·치트 도구
//   수준의 편집은 막는다. ODW1/ODW2(구형) 세이브는 읽기만 하고 다음 저장 때 ODW3 로 바뀐다.
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
inline constexpr int kSecretLen = 48;
struct MaskedSecret { unsigned char bytes[kSecretLen]; };
constexpr MaskedSecret MaskSecret(const char (&plain)[kSecretLen + 1]) {
    MaskedSecret k{};
    for (int i = 0; i < kSecretLen; ++i)
        k.bytes[i] = (unsigned char)(plain[i] ^ (kKeyMask + i * 7));
    return k;
}
inline constexpr MaskedSecret kSecret =
    MaskSecret("aSt3r1a/sav3-s3cr3t/v3-Qx9#mL2$pR7!zW4&nB8*cD5^k");

// CRC-32 (IEEE) of the plain text, salted — only used to verify ODW2 files.
inline unsigned int Checksum(const std::string& text) {
    unsigned int crc = 0xFFFFFFFFu ^ 0x0D3A5E11u;
    for (unsigned char c : text) {
        crc ^= c;
        for (int b = 0; b < 8; ++b) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

// ── SHA-256 / HMAC-SHA256 (의존성 없는 자체 구현) ──
struct Sha256 {
    unsigned int h[8] = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
                         0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    unsigned char buf[64] = {}; size_t fill = 0; unsigned long long total = 0;
    static unsigned int R(unsigned int x, int n) { return (x >> n) | (x << (32 - n)); }
    void Block(const unsigned char* p) {
        static const unsigned int K[64] = {
            0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
            0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
            0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
            0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
            0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
            0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
            0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
            0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
        unsigned int w[64];
        for (int i = 0; i < 16; ++i)
            w[i] = (unsigned)p[4*i] << 24 | (unsigned)p[4*i+1] << 16 | (unsigned)p[4*i+2] << 8 | p[4*i+3];
        for (int i = 16; i < 64; ++i) {
            const unsigned s0 = R(w[i-15],7) ^ R(w[i-15],18) ^ (w[i-15] >> 3);
            const unsigned s1 = R(w[i-2],17) ^ R(w[i-2],19) ^ (w[i-2] >> 10);
            w[i] = w[i-16] + s0 + w[i-7] + s1;
        }
        unsigned a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],hh=h[7];
        for (int i = 0; i < 64; ++i) {
            const unsigned t1 = hh + (R(e,6)^R(e,11)^R(e,25)) + ((e&f)^(~e&g)) + K[i] + w[i];
            const unsigned t2 = (R(a,2)^R(a,13)^R(a,22)) + ((a&b)^(a&c)^(b&c));
            hh=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
        }
        h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
    }
    void Update(const void* data, size_t n) {
        const unsigned char* p = (const unsigned char*)data; total += n;
        while (n) {
            const size_t k = (64 - fill < n) ? 64 - fill : n;
            std::memcpy(buf + fill, p, k); fill += k; p += k; n -= k;
            if (fill == 64) { Block(buf); fill = 0; }
        }
    }
    void Final(unsigned char out[32]) {
        const unsigned long long bits = total * 8;
        const unsigned char pad = 0x80; Update(&pad, 1);
        const unsigned char zero = 0; while (fill != 56) Update(&zero, 1);
        unsigned char len[8];
        for (int i = 0; i < 8; ++i) len[i] = (unsigned char)(bits >> (56 - 8 * i));
        Update(len, 8);
        for (int i = 0; i < 8; ++i) {
            out[4*i] = (unsigned char)(h[i] >> 24); out[4*i+1] = (unsigned char)(h[i] >> 16);
            out[4*i+2] = (unsigned char)(h[i] >> 8); out[4*i+3] = (unsigned char)h[i];
        }
    }
};
inline void HmacSha256(const unsigned char* key, size_t keyLen,
                       const std::string& msg, unsigned char out[32]) {
    unsigned char k[64] = {};
    if (keyLen > 64) { Sha256 s; s.Update(key, keyLen); s.Final(k); }
    else std::memcpy(k, key, keyLen);
    unsigned char ip[64], op[64];
    for (int i = 0; i < 64; ++i) { ip[i] = k[i] ^ 0x36; op[i] = k[i] ^ 0x5c; }
    unsigned char inner[32];
    Sha256 a; a.Update(ip, 64); a.Update(msg.data(), msg.size()); a.Final(inner);
    Sha256 b; b.Update(op, 64); b.Update(inner, 32); b.Final(out);
}
// exe 비밀값에서 용도별 키를 파생한다.
inline void DeriveKey(const char* label, unsigned char out[32]) {
    unsigned char secret[kSecretLen];
    for (int i = 0; i < kSecretLen; ++i)
        secret[i] = (unsigned char)(kSecret.bytes[i] ^ (unsigned char)(kKeyMask + i * 7));
    HmacSha256(secret, kSecretLen, label, out);
}
inline void Ctr(std::string& data, const std::string& nonce) {
    unsigned char key[32]; DeriveKey("odw3-enc", key);
    for (size_t off = 0, ctr = 0; off < data.size(); off += 32, ++ctr) {
        std::string block = nonce;
        for (int i = 0; i < 8; ++i) block += (char)((ctr >> (8 * i)) & 0xFF);
        unsigned char ks[32]; HmacSha256(key, 32, block, ks);
        for (size_t i = 0; i < 32 && off + i < data.size(); ++i) data[off + i] ^= (char)ks[i];
    }
}
inline void Tag(const std::string& signedBytes, unsigned char out[32]) {
    unsigned char key[32]; DeriveKey("odw3-mac", key);
    HmacSha256(key, 32, signedBytes, out);
}
inline std::string RandomNonce() {
    std::random_device rd;
    std::string n;
    unsigned long long t = (unsigned long long)std::chrono::steady_clock::now().time_since_epoch().count();
    for (int i = 0; i < 16; ++i) n += (char)((rd() ^ (unsigned)(t >> ((i % 8) * 8))) & 0xFF);
    return n;
}
}  // namespace save_detail

// ODW1/ODW2 의 롤링 XOR(구형 읽기 전용).
inline void OdwCrypt(std::string& s) {
    using namespace save_detail;
    for (size_t i = 0; i < s.size(); i++)
        s[i] = (char)((unsigned char)s[i]
                      ^ (unsigned char)(kKey.bytes[i % kKeyLen] ^ kKeyMask)
                      ^ (unsigned char)((i * 31u + 7u) & 0xFF));
}
inline const char ODW_MAGIC[4]        = { 'O','D','W','3' };   // 현재 형식
inline const char ODW_MAGIC_V2[4]     = { 'O','D','W','2' };   // magic + CRC32 + XOR
inline const char ODW_MAGIC_LEGACY[4] = { 'O','D','W','1' };   // magic + XOR

// Wraps plain key=value text in the on-disk ODW3 format.
inline std::string EncodeSaveText(std::string text) {
    const std::string nonce = save_detail::RandomNonce();
    save_detail::Ctr(text, nonce);
    std::string out(ODW_MAGIC, 4);
    out += nonce;
    out += text;
    unsigned char tag[32];
    save_detail::Tag(out, tag);
    out.append((const char*)tag, 32);
    return out;
}

// False when the bytes are not a valid save: unknown magic (plain text
// included), a failed tag/checksum, or a truncated file.
inline bool DecodeSaveText(const std::string& raw, std::string& text) {
    if (raw.size() >= 4 + 16 + 32 && std::memcmp(raw.data(), ODW_MAGIC, 4) == 0) {
        const std::string signedPart = raw.substr(0, raw.size() - 32);
        unsigned char tag[32]; save_detail::Tag(signedPart, tag);
        unsigned char diff = 0;
        for (int i = 0; i < 32; ++i) diff |= (unsigned char)(tag[i] ^ (unsigned char)raw[raw.size() - 32 + i]);
        if (diff != 0) return false;
        text = raw.substr(20, raw.size() - 52);
        save_detail::Ctr(text, raw.substr(4, 16));
        return true;
    }
    if (raw.size() >= 8 && std::memcmp(raw.data(), ODW_MAGIC_V2, 4) == 0) {
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
inline long long g_BestScore[4] = { 0, 0, 0, 0 }; // 난이도별 최고 점수 (EASY/NORMAL/HARD/NIGHTMARE)
inline long long g_BestWave[4]     = { 0, 0, 0, 0 };   // 난이도별 도달한 최대 웨이브
inline long long g_BestWaveSec[4]  = { 0, 0, 0, 0 };   // 최대 웨이브에 도달한 판의 진행 시간(초)
inline long long g_BestClearSec[4] = { 0, 0, 0, 0 };   // 난이도별 가장 빠른 클리어 시간(초). 0 = 클리어 없음
inline long long g_TotalKills   = 0;             // 누적 처치 수
inline long long g_TotalGames   = 0;             // 누적 플레이 횟수
inline long long g_LastRunCoins = 0;             // 직전 판 획득 코인 (GAMEOVER 표시용)
inline long long g_RunStardust  = 0;             // 현재 런에서 획득한 별가루

// 무기별 기록 [0]=RIFLE [1]=STATIC FIELD
inline long long g_WeaponBestScore[2]  = { 0, 0 };
inline long long g_WeaponBestKills[2]  = { 0, 0 };  // 한 판 최다 처치
inline long long g_WeaponTotalKills[2] = { 0, 0 };  // 누적 처치
inline long long g_WeaponRunCount[2]   = { 0, 0 };  // 플레이 횟수

// 세이브 위치: Windows %APPDATA%\Onedow, macOS ~/Library/Application Support/Onedow,
// 그 외 ~/.local/share/Onedow. ONEDOW_SAVE_DIR 로 덮어쓸 수 있다(테스트용).
// 폴더를 못 정하면 예전처럼 작업 폴더(exe 옆)에 둔다.
inline const char* kLegacySaveName = "onedow_save.cfg";
inline std::string SaveDir() {
    auto env = [](const char* name) -> std::string {
#ifdef _MSC_VER
        char* v = nullptr; size_t n = 0;
        std::string s;
        if (_dupenv_s(&v, &n, name) == 0 && v) { s = v; std::free(v); }
        return s;
#else
        const char* v = std::getenv(name);
        return v ? v : "";
#endif
    };
    std::string dir = env("ONEDOW_SAVE_DIR");
    if (dir.empty()) {
#ifdef _WIN32
        dir = env("APPDATA");
        if (!dir.empty()) dir += "\\Onedow";
#else
        const std::string home = env("HOME");
        if (!home.empty())
#  ifdef __APPLE__
            dir = home + "/Library/Application Support/Onedow";
#  else
            dir = home + "/.local/share/Onedow";
#  endif
#endif
    }
    return dir;
}
inline const char* SaveFilePath() {
    static const std::string path = [] {
        const std::string dir = SaveDir();
        if (dir.empty()) return std::string(kLegacySaveName);
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        return (std::filesystem::path(dir) / kLegacySaveName).string();
    }();
    return path.c_str();
}
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
    add("savever=%lld\n",   3);   // 형식 버전. 로더는 모르는 키를 무시하므로 키 추가는 호환된다
    add("lang=%lld\n",      (int)g_Language);
    add("fps=%lld\n",       g_FpsCap);
    add("crosshair=%lld\n", g_ShowCrosshair     ? 1 : 0);
    add("combo=%lld\n",     g_ShowCombo         ? 1 : 0);
    add("soundvol=%lld\n",  g_SoundVol);
    add("bgmvol=%lld\n",    g_BgmVol);
    add("sfxvol=%lld\n",    g_SfxVol);
    add("bgmenabled=%lld\n", g_BgmEnabled ? 1 : 0);
    add("sfxenabled=%lld\n", g_SfxEnabled ? 1 : 0);
    add("audiomono=%lld\n", g_AudioMonoOutput ? 1 : 0);
    add("audioenabled=%lld\n", g_AudioEngineEnabled ? 1 : 0);
    add("autoskill=%lld\n", g_AutoSkill ? 1 : 0);
    add("strongmenudim=%lld\n", g_StrongMenuDim ? 1 : 0);
    add("debugmode=%lld\n",    g_DebugMode ? 1 : 0);
    add("shaderfx=%lld\n",  g_ShaderFx  ? 1 : 0);
    add("mobstyle=%lld\n",  (int)g_MobVisualStyle);
    add("vfxdens=%lld\n",   (int)g_VfxDensity);
    add("macopt=%lld\n",    g_MacOptV1 ? 1 : 0);
    add("best_easy=%lld\n",   g_BestScore[0]);
    add("best_normal=%lld\n", g_BestScore[1]);
    add("best_hard=%lld\n",   g_BestScore[2]);
    for (int i = 0; i < 3; i++) {
        std::snprintf(ln, sizeof(ln), "bwave%d=%lld\n",  i, g_BestWave[i]);     buf += ln;
        std::snprintf(ln, sizeof(ln), "bclear%d=%lld\n", i, g_BestClearSec[i]); buf += ln;
        std::snprintf(ln, sizeof(ln), "bwsec%d=%lld\n",  i, g_BestWaveSec[i]);  buf += ln;
    }
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
    // 예전 exe 옆 세이브가 있으면 새 위치로 1회 옮긴다(새 세이브가 없을 때만).
    if (std::string(SaveFilePath()) != kLegacySaveName &&
        !std::filesystem::exists(SaveFilePath(), ec) &&
        std::filesystem::exists(kLegacySaveName, ec)) {
        const std::string oldBak = std::string(kLegacySaveName) + ".bak";
        ec.clear();
        std::filesystem::copy_file(kLegacySaveName, SaveFilePath(), ec);
        if (!ec) {
            std::filesystem::remove(kLegacySaveName, ec);
            if (std::filesystem::exists(oldBak, ec)) {
                std::filesystem::copy_file(oldBak, SaveBackupPath(),
                    std::filesystem::copy_options::overwrite_existing, ec);
                std::filesystem::remove(oldBak, ec);
            }
        }
        ec.clear();
    }
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
        else if (!std::strcmp(key, "bgmvol"))      g_BgmVol            = (int)(val < 0 ? 0 : val > 100 ? 100 : val);
        else if (!std::strcmp(key, "sfxvol"))      g_SfxVol            = (int)(val < 0 ? 0 : val > 100 ? 100 : val);
        else if (!std::strcmp(key, "bgmenabled"))  { g_BgmEnabled = (val != 0); if (val == 0) g_BgmVol = 0; }
        else if (!std::strcmp(key, "sfxenabled"))  { g_SfxEnabled = (val != 0); if (val == 0) g_SfxVol = 0; }
        else if (!std::strcmp(key, "audiomono"))   g_AudioMonoOutput   = (val != 0);
        else if (!std::strcmp(key, "audioenabled")) g_AudioEngineEnabled = (val != 0);
        else if (!std::strcmp(key, "autoskill"))   g_AutoSkill         = (val != 0);
        else if (!std::strcmp(key, "strongmenudim")) g_StrongMenuDim    = (val != 0);
        else if (!std::strcmp(key, "debugmode"))    g_DebugMode = (val != 0);
        else if (!std::strcmp(key, "shaderfx"))    g_ShaderFx          = (val != 0);
        else if (!std::strcmp(key, "mobstyle"))   { int v = (int)val; if (v >= 0 && v <= 1) g_MobVisualStyle = (MobVisualStyle)v; }
        else if (!std::strcmp(key, "vfxdens"))    { int v = (int)val; if (v >= 0 && v <= 2) g_VfxDensity = (VfxDensity)v; }
        else if (!std::strcmp(key, "macopt"))     g_MacOptV1          = (val != 0);
        // Scores, counts and coins are never negative; a damaged value reads as 0.
        else if (!std::strcmp(key, "best_easy"))   g_BestScore[0]      = ClampSaveValue(val);
        else if (!std::strcmp(key, "best_normal")) g_BestScore[1]      = ClampSaveValue(val);
        else if (!std::strcmp(key, "best_hard"))   g_BestScore[2]      = ClampSaveValue(val);
        else if (!std::strcmp(key, "best_nightmare")) g_BestScore[3]   = ClampSaveValue(val);
        else if (!std::strncmp(key, "bwave",  5))   { int i = atoi(key+5); if (i>=0&&i<4) g_BestWave[i]     = ClampSaveValue(val); }
        else if (!std::strncmp(key, "bwsec",  5))   { int i = atoi(key+5); if (i>=0&&i<4) g_BestWaveSec[i]  = ClampSaveValue(val); }
        else if (!std::strncmp(key, "bclear", 6))   { int i = atoi(key+6); if (i>=0&&i<4) g_BestClearSec[i] = ClampSaveValue(val); }
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
    for (int i = 0; i < 4; i++) g_BestScore[i] = g_BestWave[i] = g_BestWaveSec[i] = g_BestClearSec[i] = 0;
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
// wave: 이번 판에 도달한 웨이브. cleared 면 clearSeconds 가 클리어 시간이다.
inline bool RecordRunResult(int difficultyIdx, long long score, long long kills, int weaponIdx = -1,
                            int wave = 0, bool cleared = false, float clearSeconds = 0.0f) {
    if (difficultyIdx < 0 || difficultyIdx > 3) difficultyIdx = 1;
    score = ClampSaveValue(score);
    kills = ClampSaveValue(kills);
    bool isRecord = (score > g_BestScore[difficultyIdx]);
    if (isRecord) g_BestScore[difficultyIdx] = score;
    const long long runSec = clearSeconds > 0.0f ? ClampSaveValue((long long)(clearSeconds + 0.5f)) : 0;
    // 최대 웨이브가 같으면 더 짧은 시간을 남긴다(기록된 시간이 없으면 이번 시간).
    if (wave > g_BestWave[difficultyIdx] ||
        (wave > 0 && wave == g_BestWave[difficultyIdx] && runSec > 0 &&
         (g_BestWaveSec[difficultyIdx] == 0 || runSec < g_BestWaveSec[difficultyIdx]))) {
        g_BestWave[difficultyIdx] = std::min(wave, 99);
        g_BestWaveSec[difficultyIdx] = runSec;
    }
    if (cleared && clearSeconds > 0.0f) {
        const long long sec = ClampSaveValue((long long)(clearSeconds + 0.5f));
        if (sec > 0 && (g_BestClearSec[difficultyIdx] == 0 || sec < g_BestClearSec[difficultyIdx]))
            g_BestClearSec[difficultyIdx] = sec;
    }
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
    // 코인 적립 — 점수/1000 + 처치/2 (난이도 보너스: 보통×1.2, 어려움×1.5, 악몽×2.0)
    float diffMul = (difficultyIdx == 3) ? 2.0f : (difficultyIdx == 2) ? 1.5f
                  : (difficultyIdx == 1) ? 1.2f : 1.0f;
    long long earned = (long long)((score / 1000 + kills / 2) * diffMul);
    AddSaveValue(g_Coins, earned);
    g_LastRunCoins = earned;
    AddSaveValue(g_RunStardust, earned);
    SaveGame();
    return isRecord;
}
