#include "DebugToolkit.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cwchar>
#include <cwctype>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <glad/glad.h>

#include "DrawPrim.h"
#include "TextRenderer.h"
#include "Settings.h"
#include "Input.h"
#include "AugmentSlots.h"

extern TextRenderer g_TextS;
extern TextRenderer g_TextL;
extern float g_BatchAlpha;

DebugToolkit g_DebugToolkit;

namespace {

constexpr float PANEL_W = 760.0f;
constexpr float PANEL_TOP = 72.0f;
constexpr float TAB_Y = 68.0f;
constexpr float TAB_W = 142.0f;
constexpr float TAB_GAP = 8.0f;
constexpr float TAB_H = 32.0f;
constexpr float CONTENT_Y = 112.0f;
constexpr float AUG_SEARCH_Y = 118.0f;
constexpr float AUG_FILTER_Y = 160.0f;
constexpr float AUG_ROWS_Y = 224.0f;
constexpr float AUG_BOTTOM = 156.0f;
const wchar_t* AUG_FILTERS[] = {
    L"전체", L"보유", L"일반", L"희귀", L"에픽", L"전설",
    L"디버프", L"특수", L"조합", L"신화"
};
constexpr float STATS_RUNTIME_Y = 132.0f;
constexpr float STATS_RUNTIME_W = 166.0f;
constexpr float STATS_RUNTIME_GAP = 13.0f;
constexpr float STATS_STATUS_Y = 174.0f;
constexpr float STATS_CLEAR_Y = 168.0f;
constexpr float STATS_CLEAR_W = 96.0f;
constexpr float STATS_HELP_Y = 198.0f;
constexpr float STATS_ROWS_Y = 220.0f;
constexpr float ROW_H = 32.0f;
int AugmentVisibleRows(float h) {
    return std::max(1, (int)((h - AUG_ROWS_Y - AUG_BOTTOM) / ROW_H));
}
constexpr float SPAWN_COUNT_Y = 136.0f;
constexpr float SPAWN_GRID_Y = 190.0f;
constexpr float SPAWN_ACTION_BOTTOM = 60.0f;
constexpr float CAPTURE_CURRENT_Y = 140.0f;
constexpr float CAPTURE_ALL_Y = 190.0f;
constexpr float FOOTER_BOTTOM = 52.0f;
constexpr float FOOTER_BUTTON_H = 32.0f;
constexpr float FOOTER_BUTTON_GAP = 12.0f;
// Stats 탭 푸터 버튼 폭. 입력과 그리기가 같은 표로 위치를 계산한다.
enum FooterButton { FOOTER_SYNC, FOOTER_BALANCE, FOOTER_HITSTOP, FOOTER_NEXT_WAVE,
                    FOOTER_ORBIT, FOOTER_DEBUFF_PREVIEW, FOOTER_DEBUFF_LOG, FOOTER_COUNT };
constexpr float FOOTER_BUTTON_W[FOOTER_COUNT] = { 140.0f, 130.0f, 140.0f, 140.0f, 128.0f,
                                                  170.0f, 170.0f };
// Buttons from FOOTER_DEBUFF_PREVIEW on sit in a second row above the first.
constexpr int FOOTER_ROW2_FIRST = FOOTER_DEBUFF_PREVIEW;
constexpr float FOOTER_ROW2_LIFT = FOOTER_BUTTON_H + 8.0f;
inline float FooterButtonX(float panelX, int index) {
    float x = panelX + 16.0f;
    const int first = index >= FOOTER_ROW2_FIRST ? FOOTER_ROW2_FIRST : 0;
    for (int i = first; i < index; ++i) x += FOOTER_BUTTON_W[i] + FOOTER_BUTTON_GAP;
    return x;
}
inline float FooterButtonY(float footerY, int index) {
    return index >= FOOTER_ROW2_FIRST ? footerY - FOOTER_ROW2_LIFT : footerY;
}

struct PanelLayout {
    float x;
    float y;
    float w;
    float h;
};

PanelLayout GetPanelLayout(float sw, float sh) {
    const float w = std::min(PANEL_W, std::max(420.0f, sw - 32.0f));
    const float h = std::min(920.0f, std::max(520.0f, sh - 96.0f));
    return { sw - w - 16.0f, PANEL_TOP, w, h };
}

float Clamp01(float v) {
    return std::max(0.0f, std::min(1.0f, v));
}

float SpawnGridStep(float panelHeight) {
    const float available = panelHeight - SPAWN_GRID_Y -
                            SPAWN_ACTION_BOTTOM - 16.0f;
    return std::max(32.0f, std::min(76.0f, available / 9.0f));
}

float SpawnButtonHeight(float panelHeight) {
    return std::max(26.0f, SpawnGridStep(panelHeight) - 6.0f);
}

std::wstring FormatFloat(float value) {
    wchar_t buf[64] = {};
    std::swprintf(buf, sizeof(buf) / sizeof(buf[0]), L"%.5g", value);
    return buf;
}

std::wstring FormatInt(int value) {
    wchar_t buf[64] = {};
    std::swprintf(buf, sizeof(buf) / sizeof(buf[0]), L"%d", value);
    return buf;
}

std::wstring FormatLongLong(long long value) {
    wchar_t buf[64] = {};
    std::swprintf(buf, sizeof(buf) / sizeof(buf[0]), L"%lld", value);
    return buf;
}

uint32_t Crc32(const unsigned char* data, size_t size) {
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xEDB88320u & (-(int)(crc & 1u)));
    }
    return ~crc;
}

uint32_t Adler32(const std::vector<unsigned char>& data) {
    uint32_t a = 1u;
    uint32_t b = 0u;
    for (unsigned char byte : data) {
        a = (a + byte) % 65521u;
        b = (b + a) % 65521u;
    }
    return (b << 16) | a;
}

void AppendU32BE(std::vector<unsigned char>& out, uint32_t value) {
    out.push_back((unsigned char)((value >> 24) & 0xFFu));
    out.push_back((unsigned char)((value >> 16) & 0xFFu));
    out.push_back((unsigned char)((value >> 8) & 0xFFu));
    out.push_back((unsigned char)(value & 0xFFu));
}

void AppendPngChunk(std::vector<unsigned char>& png, const char type[4],
                    const std::vector<unsigned char>& payload) {
    AppendU32BE(png, (uint32_t)payload.size());
    const size_t typeOffset = png.size();
    png.insert(png.end(), type, type + 4);
    png.insert(png.end(), payload.begin(), payload.end());
    const uint32_t crc = Crc32(png.data() + typeOffset,
                               4 + payload.size());
    AppendU32BE(png, crc);
}

bool SaveRgbaPng(const std::filesystem::path& path, int width, int height,
                 const std::vector<unsigned char>& pixelsBottomUp) {
    if (width <= 0 || height <= 0 ||
        pixelsBottomUp.size() != (size_t)width * (size_t)height * 4u)
        return false;

    std::vector<unsigned char> raw;
    raw.reserve((size_t)height * ((size_t)width * 4u + 1u));
    for (int y = height - 1; y >= 0; --y) {
        raw.push_back(0); // PNG filter: none
        const unsigned char* row = pixelsBottomUp.data() +
                                   (size_t)y * (size_t)width * 4u;
        raw.insert(raw.end(), row, row + (size_t)width * 4u);
    }

    // A valid zlib stream made entirely of stored DEFLATE blocks. This keeps
    // the toolkit self-contained and avoids adding a platform-specific image
    // library just for screenshots.
    std::vector<unsigned char> idat;
    idat.push_back(0x78);
    idat.push_back(0x01);
    size_t offset = 0;
    while (offset < raw.size()) {
        const size_t remaining = raw.size() - offset;
        const uint16_t blockSize = (uint16_t)std::min<size_t>(65535u, remaining);
        const bool finalBlock = offset + blockSize == raw.size();
        idat.push_back(finalBlock ? 0x01 : 0x00);
        idat.push_back((unsigned char)(blockSize & 0xFFu));
        idat.push_back((unsigned char)((blockSize >> 8) & 0xFFu));
        const uint16_t inverse = (uint16_t)~blockSize;
        idat.push_back((unsigned char)(inverse & 0xFFu));
        idat.push_back((unsigned char)((inverse >> 8) & 0xFFu));
        idat.insert(idat.end(), raw.begin() + (ptrdiff_t)offset,
                    raw.begin() + (ptrdiff_t)(offset + blockSize));
        offset += blockSize;
    }
    AppendU32BE(idat, Adler32(raw));

    std::vector<unsigned char> png = {
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A
    };
    std::vector<unsigned char> ihdr;
    AppendU32BE(ihdr, (uint32_t)width);
    AppendU32BE(ihdr, (uint32_t)height);
    ihdr.push_back(8); // bit depth
    ihdr.push_back(6); // RGBA
    ihdr.push_back(0); // compression
    ihdr.push_back(0); // filter
    ihdr.push_back(0); // interlace
    AppendPngChunk(png, "IHDR", ihdr);
    AppendPngChunk(png, "IDAT", idat);
    AppendPngChunk(png, "IEND", {});

    std::ofstream file(path, std::ios::binary);
    if (!file) return false;
    file.write(reinterpret_cast<const char*>(png.data()),
               (std::streamsize)png.size());
    return file.good();
}

std::wstring Timestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    const int millis = (int)(std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count() % 1000);
    std::tm localTime = {};
#ifdef _WIN32
    localtime_s(&localTime, &nowTime);
#else
    localtime_r(&nowTime, &localTime);
#endif
    wchar_t buf[64] = {};
    std::swprintf(buf, sizeof(buf) / sizeof(buf[0]),
                  L"%04d-%02d-%02d_%02d%02d%02d%03d",
                  localTime.tm_year + 1900, localTime.tm_mon + 1,
                  localTime.tm_mday, localTime.tm_hour,
                  localTime.tm_min, localTime.tm_sec, millis);
    return buf;
}

const wchar_t* LocalizedFieldName(const wchar_t* raw) {
    struct FieldNamePair {
        const wchar_t* raw;
        const wchar_t* localized;
    };
    static const FieldNamePair names[] = {
        { L"damageMultiplier", L"피해 배율" },
        { L"baseDamage", L"기본 피해" },
        { L"maxHP", L"최대 체력" },
        { L"bulletSpeed", L"탄속" },
        { L"fireInterval", L"발사 간격" },
        { L"windowSize", L"플레이어 크기" },
        { L"moveSpeedMult", L"이동 속도 배율" },
        { L"regenPerSec", L"초당 회복" },
        { L"damageReduction", L"피해 감소" },
        { L"regenLowHpMult", L"저체력 회복 배율" },
        { L"vampireKillNeed", L"흡혈 필요 처치 수" },
        { L"lightStepHitLock", L"라이트스텝 피격 잠금" },
        { L"playerSizeMult", L"플레이어 크기 배율" },
        { L"xpMult", L"경험치 배율" },
        { L"bulletSpread", L"탄 퍼짐" },
        { L"pierceChance", L"관통 확률" },
        { L"mobXpBonus", L"전체 몬스터 경험치 보너스" },
        { L"xpPerSec", L"초당 경험치" },
        { L"rmobSpawnDelayBonus", L"원거리 생성 지연 보너스" },
        { L"mobCapBonus", L"웨이브 소환 수 보너스" },
        { L"totalAugs", L"총 증강 수" },
        { L"sizeAugTaken", L"크기 증강 획득" },
        { L"lightStep", L"라이트스텝" },
        { L"lightStepDisableTimer", L"라이트스텝 비활성 타이머" },
        { L"vampire", L"흡혈" },
        { L"vampireKillStreak", L"흡혈 처치 연속" },
        { L"miniaturize", L"소형화" },
        { L"gigantify", L"거대화" },
        { L"pierce", L"관통" },
        { L"twin", L"트윈" },
        { L"twinCount", L"트윈 수" },
        { L"chakram", L"차크람" },
        { L"ricochetMax", L"도탄 최대 횟수" },
        { L"ricochetChance", L"도탄 확률" },
        { L"ricochetDmgMult", L"도탄 피해 배율" },
        { L"mk2", L"MK2" },
        { L"mk2Used", L"MK2 사용" },
        { L"hackRanged", L"해킹 원거리" },
        { L"dashUpgrade", L"대시 강화" },
        { L"powerSurgeStacks", L"파워 서지 스택" },
        { L"commonMultBoosts", L"일반 증폭 수" },
        { L"critChance", L"치명타 확률" },
        { L"critMult", L"치명타 배율" },
        { L"lifestealPerKill", L"처치당 흡혈" },
        { L"lifestealStacks", L"흡혈 스택" },
        { L"lifesteal2", L"흡혈 II" },
        { L"berserk", L"광전사" },
        { L"deathBlast", L"죽음의 폭발" },
        { L"deathBlastMult", L"죽음의 폭발 배율" },
        { L"deathBlastDmgPct", L"죽음의 폭발 피해 비율" },
        { L"drone", L"드론" },
        { L"droneCount", L"드론 수" },
        { L"droneRapid", L"드론 연사" },
        { L"laser", L"레이저" },
        { L"laserTier", L"레이저 단계" },
        { L"bulletRain", L"탄환 비" },
        { L"bulletRainCooldown", L"탄환 비 쿨다운" },
        { L"rainKillReduce", L"탄환 비 처치 쿨다운 감소" },
        { L"chakramCount", L"차크람 수" },
        { L"chakramSingularity", L"차크람 특이점" },
        { L"killCount", L"처치 수" },
        { L"baseFireInterval", L"기본 발사 간격" },
        { L"rmobMaxBonus", L"웨이브 소환 수 보너스(원거리)" },
        { L"rmobHpMult", L"원거리 체력 배율" },
        { L"rmobDmgMult", L"원거리 피해 배율" },
        { L"rmobDelayMult", L"원거리 지연 배율" },
        { L"rmobDelayStacks", L"원거리 지연 스택" },
        { L"mobSpawnMult", L"몬스터 생성 배율" },
        { L"mobSpeedMult", L"몬스터 속도 배율" },
        { L"approachingDeath", L"죽음 접근" },
        { L"approachStacks", L"접근 스택" },
        { L"monsterHpMult", L"몬스터 체력 배율" },
        { L"rotorHpMult", L"로터 체력 배율" },
        { L"rotorXpBonus", L"로터 처치 경험치 보너스" },
        { L"flatDamageBonus", L"고정 피해 보너스" }
    };
    for (const FieldNamePair& pair : names) {
        if (std::wcscmp(pair.raw, raw) == 0) return pair.localized;
    }
    return raw;
}

} // namespace

const std::vector<DebugToolkit::Field>& DebugToolkit::Fields() {
    static const std::vector<Field> fields = [] {
        std::vector<Field> f;
        f.reserve(128);
#define DT_WIDE2(x) L##x
#define DT_WIDE(x) DT_WIDE2(x)
#define ADD_FLOAT(name) f.push_back({ LocalizedFieldName(DT_WIDE(#name)), FieldType::Float, offsetof(PlayerStats, name) })
#define ADD_INT(name)   f.push_back({ LocalizedFieldName(DT_WIDE(#name)), FieldType::Int, offsetof(PlayerStats, name) })
#define ADD_BOOL(name)  f.push_back({ LocalizedFieldName(DT_WIDE(#name)), FieldType::Bool, offsetof(PlayerStats, name) })
#define ADD_LL(name)    f.push_back({ LocalizedFieldName(DT_WIDE(#name)), FieldType::LongLong, offsetof(PlayerStats, name) })

        ADD_FLOAT(damageMultiplier); ADD_FLOAT(baseDamage); ADD_FLOAT(maxHP);
        ADD_FLOAT(bulletSpeed); ADD_FLOAT(fireInterval); ADD_FLOAT(windowSize);
        ADD_FLOAT(moveSpeedMult); ADD_FLOAT(regenPerSec); ADD_FLOAT(damageReduction);
        ADD_FLOAT(regenLowHpMult); ADD_INT(vampireKillNeed);
        ADD_FLOAT(lightStepHitLock); ADD_FLOAT(playerSizeMult); ADD_FLOAT(xpMult);
        ADD_FLOAT(bulletSpread); ADD_INT(pierceChance); ADD_INT(mobXpBonus);
        ADD_FLOAT(rotorHpMult); ADD_INT(rotorXpBonus);

        // Keep the debug list aligned with the live monster signals. The
        // close-range values are stored by MobKind, while Scope has its own
        // ranged XP accumulator.
        const auto addMobXp = [&](const wchar_t* label, MobKind kind) {
            f.push_back({ label, FieldType::Int,
                          offsetof(PlayerStats, mobKindXpBonus) +
                              sizeof(int) * (size_t)static_cast<int>(kind) });
        };
        addMobXp(L"로터 경험치 보너스", MobKind::ROTOR);
        f.push_back({ L"스코프 경험치 보너스", FieldType::Int,
                      offsetof(PlayerStats, rangedXpBonus) });
        addMobXp(L"그라비스 경험치 보너스", MobKind::GRAVIS);
        addMobXp(L"제네시스 경험치 보너스", MobKind::GENESIS);
        addMobXp(L"스웜 경험치 보너스", MobKind::SWARM);
        addMobXp(L"퀘이사 경험치 보너스", MobKind::QUASAR);
        addMobXp(L"Gimbal 경험치 보너스", MobKind::GIMBAL);
        addMobXp(L"Regulus 경험치 보너스", MobKind::REGULUS);
        addMobXp(L"안타레스 경험치 보너스", MobKind::ANTARES);
        addMobXp(L"Magnetar 경험치 보너스", MobKind::MAGNETAR);

        ADD_FLOAT(xpPerSec); ADD_FLOAT(rmobSpawnDelayBonus);
        ADD_INT(mobCapBonus); ADD_INT(totalAugs);
        ADD_BOOL(sizeAugTaken);
        ADD_BOOL(vampire); ADD_INT(vampireKillStreak);
        ADD_BOOL(pierce); ADD_BOOL(twin); ADD_INT(twinCount); ADD_BOOL(chakram);
        ADD_INT(ricochetMax); ADD_INT(ricochetChance); ADD_FLOAT(ricochetDmgMult);
        ADD_BOOL(mk2); ADD_BOOL(mk2Used);
        ADD_INT(commonMultBoosts); ADD_INT(critChance); ADD_FLOAT(critMult);
        ADD_FLOAT(lifestealPerKill); ADD_INT(lifestealStacks); ADD_BOOL(lifesteal2);
        ADD_BOOL(berserk); ADD_BOOL(deathBlast); ADD_FLOAT(deathBlastMult);
        ADD_BOOL(drone); ADD_INT(droneCount); ADD_BOOL(droneRapid); ADD_BOOL(laser);
        ADD_INT(laserTier); ADD_BOOL(bulletRain);
        ADD_FLOAT(bulletRainCooldown); ADD_BOOL(rainKillReduce); ADD_INT(chakramCount);
        ADD_LL(killCount); ADD_FLOAT(baseFireInterval);
        ADD_INT(rmobMaxBonus); ADD_FLOAT(rmobHpMult);
        ADD_FLOAT(rmobDmgMult); ADD_FLOAT(rmobDelayMult); ADD_INT(rmobDelayStacks);

#undef ADD_FLOAT
#undef ADD_INT
#undef ADD_BOOL
#undef ADD_LL
#undef DT_WIDE
#undef DT_WIDE2
        return f;
    }();
    return fields;
}

const std::vector<GameState>& DebugToolkit::CaptureStates() {
    static const std::vector<GameState> states = {
        GameState::MAIN_MENU,
        GameState::CODEX,
        GameState::TUTORIAL,
        GameState::CREATIVE_CONFIG,
        GameState::SETTINGS,
        GameState::READY,
        GameState::RUNNING,
        GameState::PAUSED,
        GameState::DYING,
        GameState::GAMEOVER,
        GameState::AUG_SELECT,
        GameState::DEBUFF_SELECT,
        GameState::AUG_REPLACE
    };
    return states;
}

const wchar_t* DebugToolkit::FieldTypeLabel(FieldType type) {
    switch (type) {
    case FieldType::Float:    return L"실수";
    case FieldType::Int:      return L"정수";
    case FieldType::Bool:     return L"불리언";
    case FieldType::LongLong: return L"64비트 정수";
    }
    return L"알 수 없음";
}

const wchar_t* DebugToolkit::MobLabel(int index) {
    static const wchar_t* labels[] = {
        L"로터", L"제네시스", L"스웜", L"그라비스", L"퀘이사", L"스코프",
        L"Gimbal", L"Regulus", L"안타레스", L"MAGNETAR"
    };
    if (index < 0 || index >= (int)(sizeof(labels) / sizeof(labels[0])))
        return L"알 수 없음";
    return labels[index];
}

const wchar_t* DebugToolkit::SceneLabel(GameState state) {
    switch (state) {
    case GameState::MAIN_MENU:       return L"메인 메뉴";
    case GameState::CODEX:           return L"도감";
    case GameState::TUTORIAL:        return L"튜토리얼";
    case GameState::CREATIVE_CONFIG: return L"크리에이티브 설정";
    case GameState::SETTINGS:        return L"설정";
    case GameState::READY:           return L"준비";
    case GameState::RUNNING:         return L"진행 중";
    case GameState::PAUSED:          return L"일시정지";
    case GameState::DYING:           return L"사망 처리";
    case GameState::GAMEOVER:        return L"게임 오버";
    case GameState::AUG_SELECT:      return L"증강 선택";
    case GameState::DEBUFF_SELECT:   return L"디버프 선택";
    case GameState::AUG_REPLACE:     return L"증강 교체";
    default:                         return L"장면";
    }
}

void DebugToolkit::Configure(const DebugToolkitContext& context) {
    context_ = context;
    configured_ = context_.stats && context_.game && context_.monsters;
    selectedMob_ = -1;
    spawnCount_ = 1;
    ResetTransientInput();
}

void DebugToolkit::ResetTransientInput() {
    augmentSearchFocused_ = false;
    augmentComposition_.clear();
    augmentSearchSelectAll_ = false;
    editing_ = false;
    replaceEditBuffer_ = false;
    editBuffer_.clear();
    editingRuntime_ = -1;
}

bool DebugToolkit::IsAugmentSearchActive() const {
    return g_DebugMode && visible_ && tab_ == 3 && augmentSearchFocused_;
}

void DebugToolkit::InsertAugmentSearchText(const std::wstring& text) {
    if (!IsAugmentSearchActive()) return;
    if (augmentSearchSelectAll_) augmentSearch_.clear();
    augmentSearchSelectAll_ = false;
    for (wchar_t c : text)
        if (c >= L' ' && augmentSearch_.size() < 48) augmentSearch_ += c;
    augmentComposition_.clear();
    augmentOffset_ = 0;
}

void DebugToolkit::EditAugmentSearch(int key, bool selectAll) {
    if (selectAll) { augmentSearchSelectAll_ = true; return; }
    if (key == GLFW_KEY_BACKSPACE) {
        if (augmentSearchSelectAll_) augmentSearch_.clear();
        else if (!augmentSearch_.empty()) augmentSearch_.pop_back();
        augmentSearchSelectAll_ = false;
    } else if (key == GLFW_KEY_DELETE) augmentSearch_.clear();
    else if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_ENTER)
        augmentSearchFocused_ = false;
    augmentOffset_ = 0;
}

int DebugToolkit::AugmentCount(int index) const {
    return context_.ownedAugments ? (int)std::count(context_.ownedAugments->begin(),
        context_.ownedAugments->end(), index) : 0;
}

std::vector<int> DebugToolkit::FilteredAugments() const {
    auto lower = [](std::wstring text) {
        for (auto& c : text) c = (wchar_t)std::towlower(c);
        return text;
    };
    const std::wstring query = lower(augmentSearch_);
    std::vector<int> result;
    for (int i = 0; i < AUG_TOTAL; ++i) {
        const AugDef& def = ALL_AUGS[i];
        if (AugRemoved(def.type)) continue;
        if (augmentFilter_ == 1 && AugmentCount(i) == 0) continue;
        if (augmentFilter_ >= 2 && (int)def.rarity != augmentFilter_ - 2) continue;
        std::wstring text(def.name, def.name + std::strlen(def.name));
        for (const auto* name : def.locName) { text += L' '; text += name; }
        if (!query.empty() && lower(text).find(query) == std::wstring::npos) continue;
        result.push_back(i);
    }
    return result;
}

bool DebugToolkit::CanEditAugments() const {
    if (!context_.game || !context_.editAugment) return false;
    const GameState state = context_.game->currentState;
    return state == GameState::RUNNING || state == GameState::READY || state == GameState::PAUSED;
}

void DebugToolkit::ApplyAugmentAction(DebugAugmentAction action, int index) {
    if (!CanEditAugments()) return;
    const bool applied = context_.editAugment(action, index);
    SetMessage(applied ? L"증강 적용 완료" : L"적용할 수 없습니다. 일회성 증강은 바로 얻기를 사용하세요.");
    if (applied && (action == DebugAugmentAction::Draw ||
                    action == DebugAugmentAction::OpenSelection ||
                    !CanEditAugments())) SetVisible(false);
}

void DebugToolkit::SetMessage(const std::wstring& message) {
    lastMessage_ = message;
}

bool DebugToolkit::Hit(float x, float y, float w, float h,
                       float mouseX, float mouseY) const {
    return mouseX >= x && mouseX <= x + w &&
           mouseY >= y && mouseY <= y + h;
}

void DebugToolkit::BeginFieldEdit(int fieldIndex) {
    if (!context_.stats || fieldIndex < 0 ||
        fieldIndex >= (int)Fields().size()) return;
    selectedField_ = fieldIndex;
    editingRuntime_ = -1;
    const Field& field = Fields()[(size_t)fieldIndex];
    const char* raw = reinterpret_cast<const char*>(context_.stats);
    const void* ptr = raw + field.offset;
    switch (field.type) {
    case FieldType::Float:
        editBuffer_ = FormatFloat(*reinterpret_cast<const float*>(ptr));
        break;
    case FieldType::Int:
        editBuffer_ = FormatInt(*reinterpret_cast<const int*>(ptr));
        break;
    case FieldType::LongLong:
        editBuffer_ = FormatLongLong(*reinterpret_cast<const long long*>(ptr));
        break;
    case FieldType::Bool:
        editBuffer_.clear();
        break;
    }
    editing_ = field.type != FieldType::Bool;
    replaceEditBuffer_ = editing_;
}

void DebugToolkit::BeginRuntimeEdit(int runtimeIndex) {
    if (!context_.game || runtimeIndex < 0 || runtimeIndex > 3) return;
    selectedField_ = -1;
    editingRuntime_ = runtimeIndex;
    editing_ = true;
    replaceEditBuffer_ = true;
    if (runtimeIndex == 0) editBuffer_ = FormatFloat(context_.game->playerHP);
    else if (runtimeIndex == 1) editBuffer_ = FormatInt(context_.game->playerLevel);
    else if (runtimeIndex == 2) editBuffer_ = FormatLongLong(context_.game->xp);
    else editBuffer_ = FormatLongLong(context_.game->score);
}

void DebugToolkit::ApplyFieldEdit() {
    if (!editing_ || !context_.stats || !context_.game) return;
    if (editingRuntime_ >= 0) {
        try {
            if (editingRuntime_ == 0) {
                size_t used = 0;
                const float value = std::stof(editBuffer_, &used);
                if (used != editBuffer_.size() || !std::isfinite(value)) {
                    SetMessage(L"잘못된 숫자입니다");
                    return;
                }
                context_.game->playerHP = std::max(-1000000.0f,
                                                    std::min(1000000.0f, value));
            } else if (editingRuntime_ == 1) {
                size_t used = 0;
                const long long value = std::stoll(editBuffer_, &used);
                if (used != editBuffer_.size()) {
                    SetMessage(L"잘못된 숫자입니다");
                    return;
                }
                context_.game->playerLevel = (int)std::max<long long>(
                    1LL, std::min(1000000000LL, value));
            } else if (editingRuntime_ == 2) {
                size_t used = 0;
                const long long value = std::stoll(editBuffer_, &used);
                if (used != editBuffer_.size()) {
                    SetMessage(L"잘못된 숫자입니다");
                    return;
                }
                context_.game->xp = std::max(0LL, value);
            } else {
                size_t used = 0;
                const long long value = std::stoll(editBuffer_, &used);
                if (used != editBuffer_.size()) {
                    SetMessage(L"잘못된 숫자입니다");
                    return;
                }
                context_.game->score = std::max(0LL, value);
                context_.game->scoreAccum = (float)context_.game->score;
            }
        } catch (...) {
            SetMessage(L"잘못된 숫자입니다");
            return;
        }
        if (context_.syncRuntime) context_.syncRuntime();
        SetMessage(L"런타임 값이 적용되었습니다");
        ResetTransientInput();
        return;
    }
    if (selectedField_ < 0 || selectedField_ >= (int)Fields().size()) return;
    const Field& field = Fields()[(size_t)selectedField_];
    char* raw = reinterpret_cast<char*>(context_.stats);
    void* ptr = raw + field.offset;
    try {
        if (field.type == FieldType::Float) {
            size_t used = 0;
            const float value = std::stof(editBuffer_, &used);
            if (used != editBuffer_.size() || !std::isfinite(value)) {
                SetMessage(L"잘못된 숫자입니다");
                return;
            }
            *reinterpret_cast<float*>(ptr) = std::max(-1000000.0f,
                                                       std::min(1000000.0f, value));
        } else if (field.type == FieldType::Int) {
            size_t used = 0;
            const long long parsed = std::stoll(editBuffer_, &used);
            if (used != editBuffer_.size()) {
                SetMessage(L"잘못된 숫자입니다");
                return;
            }
            *reinterpret_cast<int*>(ptr) = (int)std::max<long long>(
                -1000000000LL, std::min(1000000000LL, parsed));
        } else if (field.type == FieldType::LongLong) {
            size_t used = 0;
            const long long parsed = std::stoll(editBuffer_, &used);
            if (used != editBuffer_.size()) {
                SetMessage(L"잘못된 숫자입니다");
                return;
            }
            *reinterpret_cast<long long*>(ptr) = parsed;
        }
    } catch (...) {
    SetMessage(L"잘못된 숫자입니다");
        return;
    }
    if (context_.syncRuntime) context_.syncRuntime();
    SetMessage(L"필드가 적용되었습니다");
    editing_ = false;
    replaceEditBuffer_ = false;
}

void DebugToolkit::HandleNumericKeys(GLFWwindow* window) {
    if (!editing_ || !window) return;
    static std::array<bool, 32> latched = {};
    auto pressedOnce = [&](int key, int slot) {
        const int state = glfwGetKey(window, key);
        if (state == GLFW_RELEASE) latched[(size_t)slot] = false;
        if (state == GLFW_PRESS && !latched[(size_t)slot]) {
            latched[(size_t)slot] = true;
            return true;
        }
        return false;
    };
    auto append = [&](wchar_t ch) {
        if (replaceEditBuffer_) {
            editBuffer_.clear();
            replaceEditBuffer_ = false;
        }
        if (editBuffer_.size() < 30) editBuffer_.push_back(ch);
    };
    for (int i = 0; i < 10; ++i) {
        if (pressedOnce(GLFW_KEY_0 + i, i)) append((wchar_t)(L'0' + i));
        if (pressedOnce(GLFW_KEY_KP_0 + i, 10 + i))
            append((wchar_t)(L'0' + i));
    }
    const bool ctrlDown = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                          glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
    const bool aPressed = pressedOnce(GLFW_KEY_A, 28);
    if (ctrlDown && aPressed) {
        editBuffer_.clear();
        replaceEditBuffer_ = false;
        SetMessage(L"입력을 지웠습니다");
    }
    if (pressedOnce(GLFW_KEY_MINUS, 20)) append(L'-');
    if (pressedOnce(GLFW_KEY_PERIOD, 21) || pressedOnce(GLFW_KEY_KP_DECIMAL, 22))
        append(L'.');
    if (pressedOnce(GLFW_KEY_BACKSPACE, 23)) {
        if (replaceEditBuffer_) {
            editBuffer_.clear();
            replaceEditBuffer_ = false;
        } else if (!editBuffer_.empty()) {
            editBuffer_.pop_back();
        }
    }
    if (pressedOnce(GLFW_KEY_DELETE, 27)) {
        editBuffer_.clear();
        replaceEditBuffer_ = false;
        SetMessage(L"입력을 지웠습니다");
    }
    if (pressedOnce(GLFW_KEY_ENTER, 24) || pressedOnce(GLFW_KEY_KP_ENTER, 25)) {
        if (editBuffer_.empty())
            SetMessage(L"값을 입력하세요");
        else
            ApplyFieldEdit();
    }
    if (pressedOnce(GLFW_KEY_ESCAPE, 26)) {
        ResetTransientInput();
        SetMessage(L"편집을 취소했습니다");
    }
}

bool DebugToolkit::BeginInput(GLFWwindow* window, float screenW, float screenH,
                              double mouseX, double mouseY,
                              bool lmb, bool lmbPrev) {
    if (!configured_) return false;
    if (!g_DebugMode) {
        visible_ = false;
        captureCurrent_ = false;
        captureActive_ = false;
        ResetTransientInput();
        return false;
    }

    const bool wasVisible = visible_;
    static bool f1Released = true;
    static bool f12Released = true;
    if (window) {
        if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_RELEASE) f1Released = true;
        if (glfwGetKey(window, GLFW_KEY_F12) == GLFW_RELEASE) f12Released = true;
        if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS && f1Released) {
            visible_ = !visible_;
            f1Released = false;
            if (!visible_) ResetTransientInput();
        }
        if (glfwGetKey(window, GLFW_KEY_F12) == GLFW_PRESS && f12Released) {
            RequestCurrentScreenshot();
            f12Released = false;
        }
    }

    if (!visible_) return wasVisible;

    const PanelLayout p = GetPanelLayout(screenW, screenH);
    const bool click = lmb && !lmbPrev;
    const float mx = (float)mouseX;
    const float my = (float)mouseY;

    if (g_ScrollAccum != 0.0f) {
        if (tab_ == 0) {
            fieldOffset_ -= (int)std::round(g_ScrollAccum * 2.0f);
            const int visibleRows = std::max(1, (int)((p.h - 340.0f) / ROW_H));
            const int maxOffset = std::max(0,
                                           (int)Fields().size() - visibleRows);
            fieldOffset_ = std::max(0, std::min(maxOffset, fieldOffset_));
        } else if (tab_ == 3) {
            augmentOffset_ -= (int)std::round(g_ScrollAccum * 2.0f);
        }
        g_ScrollAccum = 0.0f;
    }

    if (click && Hit(p.x + p.w - 52.0f, p.y + 12.0f, 36.0f, 28.0f, mx, my)) {
        visible_ = false;
        ResetTransientInput();
        return true;
    }

    for (int t = 0; t < 4; ++t) {
        if (click && Hit(p.x + 16.0f + t * (TAB_W + TAB_GAP),
                        p.y + TAB_Y, TAB_W, TAB_H, mx, my)) {
            tab_ = t;
            ResetTransientInput();
        }
    }

    if (click && tab_ == 0) {
        if (Hit(p.x + p.w - STATS_CLEAR_W - 16.0f,
                p.y + STATS_CLEAR_Y, STATS_CLEAR_W, 26.0f, mx, my) &&
            editing_) {
            editBuffer_.clear();
            replaceEditBuffer_ = false;
            SetMessage(L"입력을 지웠습니다");
        }
        for (int runtime = 0; runtime < 4; ++runtime) {
            const float rx = p.x + 16.0f +
                             runtime * (STATS_RUNTIME_W + STATS_RUNTIME_GAP);
            if (Hit(rx, p.y + STATS_RUNTIME_Y, STATS_RUNTIME_W, 34.0f,
                    mx, my)) {
                BeginRuntimeEdit(runtime);
                break;
            }
        }
        const float rowsY = p.y + STATS_ROWS_Y;
        const int visibleRows = std::max(1, (int)((p.h - 340.0f) / ROW_H));
        for (int row = 0; row < visibleRows; ++row) {
            const int fieldIndex = fieldOffset_ + row;
            if (fieldIndex >= (int)Fields().size()) break;
            if (Hit(p.x + 16.0f, rowsY + row * ROW_H,
                    p.w - 32.0f, ROW_H - 2.0f, mx, my)) {
                const Field& field = Fields()[(size_t)fieldIndex];
                selectedField_ = fieldIndex;
                if (field.type == FieldType::Bool) {
                    char* raw = reinterpret_cast<char*>(context_.stats);
                    bool* value = reinterpret_cast<bool*>(raw + field.offset);
                    *value = !*value;
                    if (context_.syncRuntime) context_.syncRuntime();
                    SetMessage(L"켜짐/꺼짐을 전환했습니다");
                } else {
                    BeginFieldEdit(fieldIndex);
                }
            }
        }
        const float footerY = p.y + p.h - FOOTER_BOTTOM;
        auto footerHit = [&](int index) {
            return Hit(FooterButtonX(p.x, index), FooterButtonY(footerY, index),
                       FOOTER_BUTTON_W[index],
                       FOOTER_BUTTON_H, mx, my);
        };
        if (click && footerHit(FOOTER_SYNC)) {
            if (context_.syncRuntime) context_.syncRuntime();
            SetMessage(L"런타임을 동기화했습니다");
        }
        if (click && footerHit(FOOTER_BALANCE) && context_.balanceTestMode) {
            *context_.balanceTestMode = !*context_.balanceTestMode;
            SetMessage(*context_.balanceTestMode
                ? L"밸런스 테스트 시작 - 로그 기록 켜짐"
                : L"밸런스 테스트 중지 - 로그 기록 꺼짐");
        }
        if (click && footerHit(FOOTER_HITSTOP) && context_.eliteKillHitStop) {
            *context_.eliteKillHitStop = !*context_.eliteKillHitStop;
            SetMessage(*context_.eliteKillHitStop
                ? L"엘리트 처치 히트스톱 켜짐"
                : L"엘리트 처치 히트스톱 꺼짐");
        }
        if (click && footerHit(FOOTER_NEXT_WAVE) &&
            context_.advanceWave && context_.game &&
            context_.game->currentState == GameState::RUNNING) {
            context_.advanceWave();
            SetMessage(context_.game->waveVictory
                ? L"마지막 웨이브 완료 - 승리"
                : L"다음 웨이브로 이동했습니다");
            if (context_.game->waveVictory) SetVisible(false);
        }
        if (click && footerHit(FOOTER_DEBUFF_PREVIEW) && context_.game) {
            context_.game->showDebuffPreview = !context_.game->showDebuffPreview;
            SetMessage(context_.game->showDebuffPreview
                ? L"디버프 예고 켜짐" : L"디버프 예고 꺼짐");
        }
        if (click && footerHit(FOOTER_DEBUFF_LOG) && context_.game) {
            context_.game->showDebuffLog = !context_.game->showDebuffLog;
            SetMessage(context_.game->showDebuffLog
                ? L"디버프 기록 켜짐" : L"디버프 기록 꺼짐");
        }
        if (click && footerHit(FOOTER_ORBIT) && context_.constellationOrbits) {
            *context_.constellationOrbits = !*context_.constellationOrbits;
            SetMessage(*context_.constellationOrbits
                ? L"무기 별자리 궤도 켜짐"
                : L"무기 별자리 궤도 꺼짐");
        }
    } else if (click && tab_ == 1) {
        if (Hit(p.x + 16.0f, p.y + SPAWN_COUNT_Y, 44.0f, 32.0f, mx, my))
            spawnCount_ = std::max(1, spawnCount_ - 1);
        if (Hit(p.x + 126.0f, p.y + SPAWN_COUNT_Y, 44.0f, 32.0f, mx, my))
            spawnCount_ = std::min(20, spawnCount_ + 1);

        const float buttonY = p.y + SPAWN_GRID_Y;
        const float buttonH = SpawnButtonHeight(p.h);
        const float rowStep = SpawnGridStep(p.h);
        const float buttonW = (p.w - 48.0f) * 0.5f;
        const float gap = 16.0f;
        for (int index = 0; index < 10; ++index) {
            const int col = index & 1;
            const int row = index / 2;
            const float bx = p.x + 16.0f + col * (buttonW + gap);
            const float by = buttonY + row * rowStep;
            if (!Hit(bx, by, buttonW, buttonH, mx, my)) continue;
            selectedMob_ = index;
            SetMessage(L"대상을 선택했습니다 - 소환 버튼을 누르세요");
        }
        const float summonY = p.y + p.h - SPAWN_ACTION_BOTTOM;
        if (click && Hit(p.x + 16.0f, summonY, p.w - 32.0f,
                         38.0f, mx, my)) {
            if (selectedMob_ < 0) {
                SetMessage(L"먼저 대상을 선택하세요");
            } else {
                int spawned = 0;
                switch (selectedMob_) {
                case 0:
                    if (context_.spawnMob)
                        spawned = context_.spawnMob((int)MobKind::ROTOR,
                                                    spawnCount_);
                    break;
                case 1:
                    if (context_.spawnMob)
                        spawned = context_.spawnMob((int)MobKind::GENESIS,
                                                    spawnCount_);
                    break;
                case 2:
                    if (context_.spawnMob)
                        spawned = context_.spawnMob((int)MobKind::SWARM,
                                                    spawnCount_);
                    break;
                case 3:
                    if (context_.spawnMob)
                        spawned = context_.spawnMob((int)MobKind::GRAVIS,
                                                    spawnCount_);
                    break;
                case 4:
                    if (context_.spawnMob)
                        spawned = context_.spawnMob((int)MobKind::QUASAR,
                                                    spawnCount_);
                    break;
                case 5:
                    if (context_.spawnRanged)
                        spawned = context_.spawnRanged(spawnCount_);
                    break;
                case 6:
                    if (context_.spawnMob)
                        spawned = context_.spawnMob((int)MobKind::GIMBAL,
                                                    spawnCount_);
                    break;
                case 7:
                    if (context_.spawnMob)
                        spawned = context_.spawnMob((int)MobKind::REGULUS,
                                                    spawnCount_);
                    break;
                case 8:
                    if (context_.spawnMob)
                        spawned = context_.spawnMob((int)MobKind::ANTARES,
                                                    spawnCount_);
                    break;
                case 9:
                    if (context_.spawnMob)
                        spawned = context_.spawnMob((int)MobKind::MAGNETAR,
                                                    spawnCount_);
                    break;
                }
                if (spawned <= 0) {
                    SetMessage(L"소환 실패: 몬스터 수용 한도에 도달했습니다");
                    return true;
                }
                std::wstring message = L"소환 완료: ";
                message += MobLabel(selectedMob_);
                message += L" x";
                message += FormatInt(spawned);
                SetMessage(message);
            }
        }
    } else if (click && tab_ == 2) {
        if (Hit(p.x + 16.0f, p.y + CAPTURE_CURRENT_Y,
                p.w - 32.0f, 42.0f, mx, my)) {
            RequestCurrentScreenshot();
        } else if (Hit(p.x + 16.0f, p.y + CAPTURE_ALL_Y,
                       p.w - 32.0f, 42.0f, mx, my)) {
            RequestAllScreenshots();
        }
    } else if (tab_ == 3) {
        const float filterW = (p.w - 68.0f) / 10.0f;
        if (click) {
            augmentSearchFocused_ = Hit(p.x + 16, p.y + AUG_SEARCH_Y, p.w - 136, 32, mx, my);
            if (Hit(p.x + p.w - 112, p.y + AUG_SEARCH_Y, 96, 32, mx, my)) {
                augmentSearch_.clear(); augmentOffset_ = 0;
            }
            for (int i = 0; i < 10; ++i)
                if (Hit(p.x + 16 + i * (filterW + 4), p.y + AUG_FILTER_Y,
                        filterW, 28, mx, my)) { augmentFilter_ = i; augmentOffset_ = 0; }
        }
        const auto list = FilteredAugments();
        const int rows = AugmentVisibleRows(p.h);
        if (click && Hit(p.x + p.w - 112, p.y + 195, 44, 24, mx, my)) augmentOffset_ -= rows;
        if (click && Hit(p.x + p.w - 60, p.y + 195, 44, 24, mx, my)) augmentOffset_ += rows;
        augmentOffset_ = std::max(0, std::min(std::max(0, (int)list.size() - rows), augmentOffset_));
        if (click) {
            for (int row = 0; row < rows && augmentOffset_ + row < (int)list.size(); ++row) {
                const int idx = list[augmentOffset_ + row];
                const float ry = p.y + AUG_ROWS_Y + row * ROW_H;
                if (!Hit(p.x + 16, ry, p.w - 32, ROW_H - 2, mx, my)) continue;
                selectedAugment_ = idx;
                if (Hit(p.x + p.w - 182, ry, 68, 28, mx, my)) ApplyAugmentAction(DebugAugmentAction::Toggle, idx);
                else if (Hit(p.x + p.w - 108, ry, 40, 28, mx, my)) ApplyAugmentAction(DebugAugmentAction::Remove, idx);
                else if (Hit(p.x + p.w - 60, ry, 44, 28, mx, my)) ApplyAugmentAction(DebugAugmentAction::Add, idx);
            }
            const float actionW = (p.w - 64) / 5;
            const DebugAugmentAction actions[] = { DebugAugmentAction::Draw, DebugAugmentAction::Grant, DebugAugmentAction::OpenSelection,
                DebugAugmentAction::FillSlots, DebugAugmentAction::Clear };
            for (int i = 0; i < 5; ++i)
                if (Hit(p.x + 16 + i * (actionW + 8), p.y + p.h - 76,
                        actionW, 32, mx, my) && (i >= 2 || selectedAugment_ >= 0))
                    ApplyAugmentAction(actions[i], selectedAugment_);
        }
    }

    HandleNumericKeys(window);
    return true;
}

void DebugToolkit::DrawButton(float x, float y, float w, float h,
                              const wchar_t* label, bool selected,
                              bool enabled) const {
    const float r = enabled ? (selected ? 0.22f : 0.06f) : 0.025f;
    const float g = enabled ? (selected ? 0.70f : 0.16f) : 0.05f;
    const float b = enabled ? (selected ? 0.95f : 0.24f) : 0.08f;
    const float a = enabled ? 0.82f : 0.35f;
    BindMainShader();
    drawRect(x, y, w, h, r * 0.35f, g * 0.35f, b * 0.35f, a);
    drawConstellFrame(x, y, w, h, r + 0.1f, g + 0.1f, b + 0.1f,
                      selected ? 0.95f : 0.48f, 9.0f, 3.0f);
    if (label) {
        float textScale = 0.58f;
        float tw = g_TextS.Width(label, textScale);
        const float maxWidth = std::max(12.0f, w - 18.0f);
        if (tw > maxWidth)
            textScale *= maxWidth / tw;
        tw = g_TextS.Width(label, textScale);
        const float textH = g_TextS.Height(label, textScale);
        const float textY = y + std::max(3.0f, (h - textH) * 0.5f);
        g_TextS.Draw(label, x + (w - tw) * 0.5f, textY, textScale,
                     enabled ? 0.82f : 0.45f,
                     enabled ? 0.94f : 0.50f,
                     enabled ? 1.0f : 0.58f,
                     enabled ? 0.95f : 0.58f);
    }
}

void DebugToolkit::RenderStatsTab(float x, float y, float w, float h,
                                  float /*mouseX*/, float /*mouseY*/,
                                  bool /*click*/) {
    if (!context_.stats || !context_.game) return;
    std::wstring hp = L"체력 " + FormatFloat(context_.game->playerHP);
    std::wstring level = L"레벨 " + FormatInt(context_.game->playerLevel);
    std::wstring xp = L"경험치 " + FormatLongLong(context_.game->xp);
    std::wstring score = L"점수 " + FormatLongLong(context_.game->score);
    const std::wstring runtimeValues[] = { hp, level, xp, score };

    g_TextS.Draw(L"런타임 값", x + 20.0f, y + CONTENT_Y, 0.55f,
                 0.70f, 0.90f, 1.0f, 0.94f);
    for (int runtime = 0; runtime < 4; ++runtime) {
        const float rx = x + 16.0f +
                         runtime * (STATS_RUNTIME_W + STATS_RUNTIME_GAP);
        DrawButton(rx, y + STATS_RUNTIME_Y, STATS_RUNTIME_W, 34.0f,
                   runtimeValues[runtime].c_str(), editingRuntime_ == runtime,
                   true);
    }

    std::wstring editTarget;
    if (editingRuntime_ == 0) editTarget = L"체력";
    else if (editingRuntime_ == 1) editTarget = L"레벨";
    else if (editingRuntime_ == 2) editTarget = L"경험치";
    else if (editingRuntime_ == 3) editTarget = L"점수";
    else if (selectedField_ >= 0 && selectedField_ < (int)Fields().size())
        editTarget = Fields()[(size_t)selectedField_].name;

    if (editing_) {
        std::wstring status = L"수정 중 ";
        status += editTarget.empty() ? L"값" : editTarget;
        status += L"  >  ";
        status += editBuffer_.empty() ? L"<비어 있음>" : editBuffer_;
        g_TextS.Draw(status.c_str(), x + 20.0f, y + STATS_STATUS_Y, 0.48f,
                     1.0f, 0.84f, 0.32f, 0.96f);
    } else {
        g_TextS.Draw(lastMessage_.empty() ? L"수정할 값을 선택하세요" : lastMessage_.c_str(),
                     x + 20.0f, y + STATS_STATUS_Y,
                     0.48f, 0.56f, 0.76f, 0.88f, 0.86f);
    }
    g_TextS.Draw(L"CTRL+A 전체 삭제   BACKSPACE 지우기   ENTER 적용   ESC 취소",
                 x + 20.0f, y + STATS_HELP_Y, 0.42f,
                 0.48f, 0.64f, 0.76f, 0.84f);
    DrawButton(x + w - STATS_CLEAR_W - 16.0f, y + STATS_CLEAR_Y,
               STATS_CLEAR_W, 26.0f, L"지우기", false, editing_);

    const float rowsY = y + STATS_ROWS_Y;
    const int visibleRows = std::max(1, (int)((h - 340.0f) / ROW_H));
    const auto& fields = Fields();
    const int maxOffset = std::max(0, (int)fields.size() - visibleRows);
    fieldOffset_ = std::max(0, std::min(maxOffset, fieldOffset_));
    for (int row = 0; row < visibleRows; ++row) {
        const int index = fieldOffset_ + row;
        if (index >= (int)fields.size()) break;
        const Field& field = fields[(size_t)index];
        const float ry = rowsY + row * ROW_H;
        const bool selected = selectedField_ == index;
        BindMainShader();
        drawRect(x + 16.0f, ry, w - 32.0f, ROW_H - 3.0f,
                 selected ? 0.06f : 0.018f,
                 selected ? 0.18f : 0.032f,
                 selected ? 0.25f : 0.055f,
                 selected ? 0.90f : 0.72f);
        g_TextS.Draw(field.name, x + 28.0f, ry + 6.0f, 0.52f,
                     0.80f, 0.88f, 0.98f, 0.92f);
        g_TextS.Draw(FieldTypeLabel(field.type), x + w - 166.0f, ry + 7.0f,
                     0.42f, 0.45f, 0.65f, 0.78f, 0.78f);

        const char* raw = reinterpret_cast<const char*>(context_.stats);
        const void* ptr = raw + field.offset;
        std::wstring value;
        if (editing_ && selectedField_ == index)
            value = editBuffer_.empty() ? L"<입력>" : editBuffer_;
        else if (field.type == FieldType::Float)
            value = FormatFloat(*reinterpret_cast<const float*>(ptr));
        else if (field.type == FieldType::Int)
            value = FormatInt(*reinterpret_cast<const int*>(ptr));
        else if (field.type == FieldType::LongLong)
            value = FormatLongLong(*reinterpret_cast<const long long*>(ptr));
        else
            value = *reinterpret_cast<const bool*>(ptr) ? L"켜짐" : L"꺼짐";
        g_TextS.Draw(value.c_str(), x + w - 108.0f, ry + 6.0f, 0.52f,
                     selected ? 1.0f : 0.78f,
                     selected ? 0.90f : 0.84f,
                     selected ? 0.42f : 0.92f, 0.96f);
    }

    const float footerY = y + h - FOOTER_BOTTOM;
    auto drawFooter = [&](int index, const wchar_t* label, bool selected, bool enabled) {
        DrawButton(FooterButtonX(x, index), FooterButtonY(footerY, index),
                   FOOTER_BUTTON_W[index], FOOTER_BUTTON_H, label, selected, enabled);
    };
    const bool balanceActive = context_.balanceTestMode &&
                               *context_.balanceTestMode;
    const bool hitStopActive = context_.eliteKillHitStop && *context_.eliteKillHitStop;
    const bool orbitActive = context_.constellationOrbits && *context_.constellationOrbits;
    drawFooter(FOOTER_SYNC, L"런타임 동기화", false, true);
    drawFooter(FOOTER_BALANCE, balanceActive ? L"밸런스: 켜짐" : L"밸런스: 꺼짐",
               balanceActive, context_.balanceTestMode != nullptr);
    drawFooter(FOOTER_HITSTOP, hitStopActive ? L"히트스톱: 켜짐" : L"히트스톱: 꺼짐",
               hitStopActive, context_.eliteKillHitStop != nullptr);
    drawFooter(FOOTER_NEXT_WAVE, L"다음 웨이브", false,
               context_.advanceWave && context_.game->currentState == GameState::RUNNING);
    drawFooter(FOOTER_ORBIT, orbitActive ? L"궤도: 켜짐" : L"궤도: 꺼짐",
               orbitActive, context_.constellationOrbits != nullptr);
    const bool previewOn = context_.game && context_.game->showDebuffPreview;
    const bool logOn = context_.game && context_.game->showDebuffLog;
    drawFooter(FOOTER_DEBUFF_PREVIEW, previewOn ? L"디버프 예고: 켜짐" : L"디버프 예고: 꺼짐",
               previewOn, context_.game != nullptr);
    drawFooter(FOOTER_DEBUFF_LOG, logOn ? L"디버프 기록: 켜짐" : L"디버프 기록: 꺼짐",
               logOn, context_.game != nullptr);
    wchar_t page[64] = {};
    std::swprintf(page, sizeof(page) / sizeof(page[0]),
                  L"필드 %d-%d / %d", fieldOffset_ + 1,
                  std::min(fieldOffset_ + visibleRows, (int)fields.size()),
                  (int)fields.size());
    g_TextS.Draw(page, x + w - 190.0f, y + h - 76.0f - FOOTER_ROW2_LIFT, 0.46f,
                 0.52f, 0.68f, 0.78f, 0.78f);
}

void DebugToolkit::RenderSpawnTab(float x, float y, float w, float h,
                                  float /*mouseX*/, float /*mouseY*/,
                                  bool /*click*/) {
    wchar_t count[64] = {};
    std::swprintf(count, sizeof(count) / sizeof(count[0]),
                  L"수량  %d", spawnCount_);
    g_TextS.Draw(L"소환 설정", x + 20.0f, y + CONTENT_Y, 0.55f,
                 0.70f, 0.94f, 1.0f, 0.94f);
    DrawButton(x + 16.0f, y + SPAWN_COUNT_Y, 44.0f, 32.0f,
               L"-", false, true);
    DrawButton(x + 66.0f, y + SPAWN_COUNT_Y, 54.0f, 32.0f,
               count, false, true);
    DrawButton(x + 126.0f, y + SPAWN_COUNT_Y, 44.0f, 32.0f,
               L"+", false, true);
    const std::wstring target = selectedMob_ >= 0
        ? std::wstring(L"대상  ") + MobLabel(selectedMob_)
        : L"대상  <없음>";
    g_TextS.Draw(target.c_str(), x + 204.0f, y + SPAWN_COUNT_Y + 7.0f,
                 0.50f, 0.98f, 0.86f, 0.34f, 0.94f);
    g_TextS.Draw(L"아레나 가장자리에서 소환됩니다. 기존 제한은 유지됩니다.",
                 x + 204.0f, y + SPAWN_COUNT_Y + 22.0f, 0.40f,
                 0.50f, 0.68f, 0.80f, 0.82f);

    const float buttonY = y + SPAWN_GRID_Y;
    const float buttonH = SpawnButtonHeight(h);
    const float rowStep = SpawnGridStep(h);
    const float buttonW = (w - 48.0f) * 0.5f;
    const float gap = 16.0f;
    for (int index = 0; index < 10; ++index) {
        const int col = index & 1;
        const int row = index / 2;
        const float bx = x + 16.0f + col * (buttonW + gap);
        const float by = buttonY + row * rowStep;
        DrawButton(bx, by, buttonW, buttonH, MobLabel(index),
                   selectedMob_ == index, true);
    }
    const std::wstring summonLabel = selectedMob_ >= 0
        ? std::wstring(L"소환 ") + MobLabel(selectedMob_) + L"  x" +
          FormatInt(spawnCount_)
        : L"소환할 대상을 선택하세요";
    DrawButton(x + 16.0f, y + h - SPAWN_ACTION_BOTTOM, w - 32.0f,
               38.0f, summonLabel.c_str(), false, selectedMob_ >= 0);
    g_TextS.Draw(L"대상 선택  >  수량 설정  >  소환 클릭",
                 x + 20.0f, y + h - 16.0f, 0.42f,
                 0.52f, 0.68f, 0.80f, 0.84f);
}

void DebugToolkit::RenderCaptureTab(float x, float y, float w, float h,
                                    float /*mouseX*/, float /*mouseY*/,
                                    bool /*click*/) {
    g_TextS.Draw(L"스크린샷 출력", x + 20.0f, y + CONTENT_Y, 0.55f,
                 0.70f, 0.94f, 1.0f, 0.94f);
    DrawButton(x + 16.0f, y + CAPTURE_CURRENT_Y, w - 32.0f, 42.0f,
               L"현재 장면 캡처  [F12]", false, !captureActive_);
    DrawButton(x + 16.0f, y + CAPTURE_ALL_Y, w - 32.0f, 42.0f,
               L"모든 장면 캡처", false, !captureActive_);
    g_TextS.Draw(L"출력  ./Screenshots/*.png", x + 20.0f, y + 248.0f,
                 0.46f, 0.62f, 0.80f, 0.92f, 0.86f);
    if (captureActive_) {
        wchar_t progress[96] = {};
        std::swprintf(progress, sizeof(progress) / sizeof(progress[0]),
                      L"캡처 중 %d / %d",
                      (int)captureIndex_ + 1, (int)CaptureStates().size());
        g_TextS.Draw(progress, x + 20.0f, y + 282.0f, 0.54f,
                     1.0f, 0.84f, 0.30f, 0.96f);
    } else {
        g_TextS.Draw(L"플레이 상태를 변경하지 않고 각 장면을 렌더링합니다.",
                     x + 20.0f, y + 282.0f, 0.42f,
                     0.52f, 0.68f, 0.80f, 0.84f);
    }
    if (!lastMessage_.empty())
        g_TextS.Draw(lastMessage_.c_str(), x + 20.0f, y + h - 42.0f,
                     0.46f, 0.55f, 0.86f, 0.98f, 0.92f);
}

void DebugToolkit::Render(float screenW, float screenH, GameState sceneState) {
    if (!configured_ || !g_DebugMode || !visible_ || SuppressOverlayForCapture())
        return;

    const PanelLayout p = GetPanelLayout(screenW, screenH);
    const float oldAlpha = g_BatchAlpha;
    g_BatchAlpha = 1.0f;
    BindMainShader();
    drawRect(p.x - 8.0f, p.y - 8.0f, p.w + 16.0f, p.h + 16.0f,
             0.005f, 0.012f, 0.025f, 0.82f);
    drawConstellFrame(p.x - 8.0f, p.y - 8.0f, p.w + 16.0f, p.h + 16.0f,
                      0.20f, 0.75f, 1.0f, 0.90f, 18.0f, 5.0f);
    g_TextL.Draw(L"디버그 도구", p.x + 18.0f, p.y + 12.0f, 0.78f,
                 0.68f, 0.92f, 1.0f, 0.98f);
    g_TextS.Draw(L"F1 열기/닫기  |  디버그 모드 전용", p.x + 20.0f,
                 p.y + 43.0f, 0.40f, 0.52f, 0.72f, 0.86f, 0.84f);
    g_TextS.Draw(SceneLabel(sceneState), p.x + p.w - 178.0f, p.y + 43.0f,
                 0.40f, 0.55f, 0.74f, 0.88f, 0.84f);
    DrawButton(p.x + p.w - 52.0f, p.y + 12.0f, 36.0f, 28.0f,
               L"X", false, true);

    const wchar_t* tabs[] = { L"능력치", L"소환", L"캡처", L"증강" };
    for (int t = 0; t < 4; ++t)
        DrawButton(p.x + 16.0f + t * (TAB_W + TAB_GAP),
                   p.y + TAB_Y, TAB_W, TAB_H, tabs[t], tab_ == t, true);

    if (tab_ == 0) RenderStatsTab(p.x, p.y, p.w, p.h, 0.0f, 0.0f, false);
    else if (tab_ == 1) RenderSpawnTab(p.x, p.y, p.w, p.h, 0.0f, 0.0f, false);
    else if (tab_ == 2) RenderCaptureTab(p.x, p.y, p.w, p.h, 0.0f, 0.0f, false);
    else RenderAugmentTab(p.x, p.y, p.w, p.h);
    g_BatchAlpha = oldAlpha;
}

void DebugToolkit::RenderAugmentTab(float x, float y, float w, float h) {
    const bool enabled = CanEditAugments();
    auto text = [&](const std::wstring& label, float tx, float ty, float maxWidth, float scale = 0.46f) {
        const float width = g_TextS.Width(label.c_str(), scale);
        if (width > maxWidth) scale *= maxWidth / width;
        g_TextS.Draw(label.c_str(), tx, ty, scale, 0.80f, 0.92f, 1.0f, 0.96f);
    };
    const std::wstring query = augmentSearch_ + augmentComposition_;
    const std::wstring search = query.empty() ? L"검색: 증강 이름 / 영문 ID" : query;
    DrawButton(x + 16, y + AUG_SEARCH_Y, w - 136, 32, search.c_str(), augmentSearchFocused_);
    DrawButton(x + w - 112, y + AUG_SEARCH_Y, 96, 32, L"검색 지우기");
    if (IsAugmentSearchActive()) {
        // Keep Korean IME candidates beside this search box.
        InputSetCodexImeAnchor(x + 24, x + 24, y + AUG_SEARCH_Y + 4, 24);
    }
    const float filterW = (w - 68) / 10;
    for (int i = 0; i < 10; ++i)
        DrawButton(x + 16 + i * (filterW + 4), y + AUG_FILTER_Y, filterW, 28,
                   AUG_FILTERS[i], augmentFilter_ == i);
    const auto list = FilteredAugments();
    const int rows = AugmentVisibleRows(h);
    augmentOffset_ = std::max(0, std::min(std::max(0, (int)list.size() - rows), augmentOffset_));
    const std::wstring status = L"슬롯 " + FormatInt(CountIdentitySlotsUsed()) + L" / " + FormatInt(IdentitySlotMax()) +
        L"   |   " + FormatInt((int)list.size()) + L"개   |   선택 화면: 확인 후 획득 · 바로 얻기: 즉시 적용";
    text(status, x + 20, y + 201, w - 140, 0.40f);
    DrawButton(x + w - 112, y + 195, 44, 24, L"<");
    DrawButton(x + w - 60, y + 195, 44, 24, L">");
    for (int row = 0; row < rows && augmentOffset_ + row < (int)list.size(); ++row) {
        const int idx = list[augmentOffset_ + row];
        const auto& def = ALL_AUGS[idx];
        const int count = AugmentCount(idx);
        const float ry = y + AUG_ROWS_Y + row * ROW_H;
        const bool instant = def.type == AugType::RANDOM_AUG || def.type == AugType::S_CHAOS || def.type == AugType::S_PANDORA;
        BindMainShader();
        drawRect(x + 16, ry, w - 32, ROW_H - 2, selectedAugment_ == idx ? 0.10f : 0.025f,
                 count > 0 ? 0.20f : 0.07f, 0.19f, 0.9f);
        text(std::wstring(AugName(def)), x + 24, ry + 7, w - 380);
        text(std::wstring(GetRarityKR(def.rarity)), x + w - 350, ry + 8, 84, 0.40f);
        text(L"x" + FormatInt(count), x + w - 252, ry + 8, 62);
        DrawButton(x + w - 182, ry, 68, 28, count > 0 ? L"ON" : L"OFF", count > 0, enabled && !instant);
        DrawButton(x + w - 108, ry, 40, 28, L"-", false, enabled && count > 0 && !instant);
        DrawButton(x + w - 60, ry, 44, 28, L"+", false, enabled && !instant);
    }
    if (list.empty()) text(L"검색 결과가 없습니다", x + 24, y + AUG_ROWS_Y + 8, w - 48);
    if (selectedAugment_ >= 0 && selectedAugment_ < AUG_TOTAL) {
        const auto& def = ALL_AUGS[selectedAugment_];
        text(std::wstring(AugName(def)) + L"  [" + std::wstring(def.name, def.name + std::strlen(def.name)) + L"]",
             x + 20, y + h - 148, w - 40);
        text(AugStat(def), x + 20, y + h - 122, w - 40, 0.42f);
    } else text(L"행을 선택한 뒤 선택 화면에 띄우거나 바로 얻을 수 있습니다", x + 20, y + h - 140, w - 40);
    const float actionW = (w - 64) / 5;
    const wchar_t* actions[] = { L"선택 화면에 띄우기", L"바로 얻기", L"랜덤 선택 화면", L"슬롯 채우기", L"전체 OFF" };
    for (int i = 0; i < 5; ++i)
        DrawButton(x + 16 + i * (actionW + 8), y + h - 76, actionW, 32, actions[i], false,
                   enabled && (i >= 2 || selectedAugment_ >= 0));
    text(enabled ? lastMessage_ : L"전투 / 준비 / 일시정지에서 사용 가능 · 선택/교체 화면을 먼저 완료하세요",
         x + 20, y + h - 28, w - 40, 0.40f);
}

void DebugToolkit::RequestCurrentScreenshot() {
    if (!g_DebugMode || captureActive_) return;
    captureCurrent_ = true;
    SetMessage(L"스크린샷을 대기열에 추가했습니다");
}

void DebugToolkit::RequestNamedScreenshot(const std::wstring& name) {
    if (!g_DebugMode || captureActive_) return;
    captureCurrent_ = true;
    captureName_ = name;
}

void DebugToolkit::RequestAllScreenshots() {
    if (!g_DebugMode || captureActive_) return;
    captureCurrent_ = false;
    captureActive_ = true;
    captureIndex_ = 0;
    SetMessage(L"전체 장면 캡처를 대기열에 추가했습니다");
}

GameState DebugToolkit::CaptureRenderState(GameState restoreState) const {
    if (!captureActive_ || captureIndex_ >= CaptureStates().size())
        return restoreState;
    return CaptureStates()[captureIndex_];
}

void DebugToolkit::CaptureFrameAfterRender(int width, int height,
                                           GameState renderedState) {
    if (!captureActive_ && !captureCurrent_) return;

    std::vector<unsigned char> pixels((size_t)width * (size_t)height * 4u);
    glFinish();
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    const std::filesystem::path directory = std::filesystem::path("Screenshots");
    std::error_code ec;
    std::filesystem::create_directories(directory, ec);
    const std::wstring stamp = Timestamp();
    std::wstring fileName = captureName_.empty()
        ? stamp + L"_" + SceneLabel(renderedState) : captureName_;
    captureName_.clear();
    if (captureActive_) {
        wchar_t index[32] = {};
        std::swprintf(index, sizeof(index) / sizeof(index[0]), L"_%02d",
                      (int)captureIndex_ + 1);
        fileName += index;
    }
    fileName += L".png";

    const bool ok = SaveRgbaPng(directory / std::filesystem::path(fileName),
                                width, height, pixels);
    SetMessage(ok ? L"스크린샷을 저장했습니다" : L"스크린샷 저장에 실패했습니다");
    captureCurrent_ = false;
    if (captureActive_) {
        ++captureIndex_;
        if (captureIndex_ >= CaptureStates().size()) {
            captureActive_ = false;
            captureIndex_ = 0;
            SetMessage(ok ? L"모든 장면을 저장했습니다" : L"장면 캡처에 실패했습니다");
        }
    }
}
