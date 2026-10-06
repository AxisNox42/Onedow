#pragma once
// ─────────────────────────────────────────────────────────────
// 도감(Codex) — 발견 추적 + 적 정보
//   증강/조합/적은 "처음 획득/조우" 해야 도감에 공개. 그 전엔 ??? (정보 비공개).
//   발견 상태는 SaveSystem 으로 영구 저장.
// ─────────────────────────────────────────────────────────────
#include "Augment.h"   // AUG_TOTAL
#include "Monster.h"   // MobKind
#include "Settings.h"  // g_Language, g_CreativeMode
#include <string>
#include <cwctype>
#include <algorithm>

extern bool g_DevUnlocked;   // main.cpp — 도감 develop_mod 해금

inline bool CodexFullReveal() {
    return g_CreativeMode || g_DevUnlocked;
}

extern wchar_t g_CodexSearch[64];
extern int     g_CodexSearchLen;
inline bool    g_CodexSearchInputEnabled = false;
inline wchar_t g_CodexSearchComposition[64] = {};
inline int     g_CodexSearchCompositionLen = 0;
inline int     g_CodexSearchCompositionCaret = 0;

inline int g_CodexSearchCaret = 0;
inline int g_CodexSearchAnchor = -1;

inline bool CodexSearchHasSelection() {
    return g_CodexSearchAnchor >= 0 && g_CodexSearchAnchor != g_CodexSearchCaret;
}
inline int CodexSearchSelectionStart() {
    return CodexSearchHasSelection()
        ? (g_CodexSearchAnchor < g_CodexSearchCaret ? g_CodexSearchAnchor : g_CodexSearchCaret)
        : g_CodexSearchCaret;
}
inline int CodexSearchSelectionEnd() {
    return CodexSearchHasSelection()
        ? (g_CodexSearchAnchor > g_CodexSearchCaret ? g_CodexSearchAnchor : g_CodexSearchCaret)
        : g_CodexSearchCaret;
}
inline void CodexSearchSetComposition(const wchar_t* text, int caret = -1) {
    g_CodexSearchCompositionLen = 0;
    g_CodexSearchComposition[0] = 0;
    g_CodexSearchCompositionCaret = 0;
    if (!text) return;

    const bool replaceSelection = CodexSearchHasSelection();
    const int replaceLen = replaceSelection
        ? CodexSearchSelectionEnd() - CodexSearchSelectionStart() : 0;
    const int maxLen = std::max(0, 63 - (g_CodexSearchLen - replaceLen));
    for (int i = 0; text[i] && g_CodexSearchCompositionLen < maxLen; ++i) {
        const int units = text[i] >= 0xD800 && text[i] <= 0xDBFF &&
                          text[i + 1] >= 0xDC00 && text[i + 1] <= 0xDFFF ? 2 : 1;
        if (text[i] < 32 || text[i] == 127) continue;
        if (g_CodexSearchCompositionLen + units > maxLen) break;
        g_CodexSearchComposition[g_CodexSearchCompositionLen++] = text[i];
        if (units == 2)
            g_CodexSearchComposition[g_CodexSearchCompositionLen++] = text[++i];
    }
    g_CodexSearchComposition[g_CodexSearchCompositionLen] = 0;
    g_CodexSearchCompositionCaret = caret < 0
        ? g_CodexSearchCompositionLen
        : std::clamp(caret, 0, g_CodexSearchCompositionLen);
}
inline void CodexSearchClearComposition() {
    g_CodexSearchComposition[0] = 0;
    g_CodexSearchCompositionLen = 0;
    g_CodexSearchCompositionCaret = 0;
}
inline std::wstring CodexSearchQuery() {
    if (g_CodexSearchCompositionLen <= 0)
        return std::wstring(g_CodexSearch, g_CodexSearch + g_CodexSearchLen);
    const int begin = CodexSearchHasSelection()
        ? CodexSearchSelectionStart() : g_CodexSearchCaret;
    const int end = CodexSearchHasSelection()
        ? CodexSearchSelectionEnd() : g_CodexSearchCaret;
    std::wstring query;
    query.reserve((size_t)g_CodexSearchLen + g_CodexSearchCompositionLen);
    query.append(g_CodexSearch, g_CodexSearch + begin);
    query.append(g_CodexSearchComposition,
                 g_CodexSearchComposition + g_CodexSearchCompositionLen);
    query.append(g_CodexSearch + end, g_CodexSearch + g_CodexSearchLen);
    return query;
}
inline int CodexSearchQueryCaret() {
    if (g_CodexSearchCompositionLen <= 0) return g_CodexSearchCaret;
    const int begin = CodexSearchHasSelection()
        ? CodexSearchSelectionStart() : g_CodexSearchCaret;
    return begin + g_CodexSearchCompositionCaret;
}
inline int CodexSearchCompositionStart() {
    return CodexSearchHasSelection()
        ? CodexSearchSelectionStart() : g_CodexSearchCaret;
}
inline int CodexSearchPreviousBoundary(int pos) {
    if (pos <= 0) return 0;
    --pos;
    if (pos > 0 && g_CodexSearch[pos] >= 0xDC00 && g_CodexSearch[pos] <= 0xDFFF &&
        g_CodexSearch[pos - 1] >= 0xD800 && g_CodexSearch[pos - 1] <= 0xDBFF) --pos;
    return pos;
}
inline int CodexSearchNextBoundary(int pos) {
    if (pos >= g_CodexSearchLen) return g_CodexSearchLen;
    ++pos;
    if (pos < g_CodexSearchLen && g_CodexSearch[pos - 1] >= 0xD800 &&
        g_CodexSearch[pos - 1] <= 0xDBFF && g_CodexSearch[pos] >= 0xDC00 &&
        g_CodexSearch[pos] <= 0xDFFF) ++pos;
    return pos;
}
inline void CodexSearchEraseRange(int begin, int end) {
    if (begin < 0) begin = 0;
    if (end > g_CodexSearchLen) end = g_CodexSearchLen;
    if (begin >= end) return;
    for (int i = end; i <= g_CodexSearchLen; ++i)
        g_CodexSearch[i - (end - begin)] = g_CodexSearch[i];
    g_CodexSearchLen -= end - begin;
    g_CodexSearchCaret = begin;
    g_CodexSearchAnchor = -1;
}
inline void CodexSearchDeleteSelection() {
    if (CodexSearchHasSelection())
        CodexSearchEraseRange(CodexSearchSelectionStart(), CodexSearchSelectionEnd());
}
inline void CodexSearchClear() {
    g_CodexSearch[0] = 0;
    g_CodexSearchLen = 0;
    g_CodexSearchCaret = 0;
    g_CodexSearchAnchor = -1;
    CodexSearchClearComposition();
}
inline void CodexSearchMoveCaret(int target, bool extend) {
    if (extend) {
        if (g_CodexSearchAnchor < 0) g_CodexSearchAnchor = g_CodexSearchCaret;
        g_CodexSearchCaret = target;
        if (g_CodexSearchCaret == g_CodexSearchAnchor) g_CodexSearchAnchor = -1;
    } else {
        g_CodexSearchCaret = target;
        g_CodexSearchAnchor = -1;
    }
}
inline void CodexSearchMoveLeft(bool extend) {
    if (!extend && CodexSearchHasSelection())
        CodexSearchMoveCaret(CodexSearchSelectionStart(), false);
    else CodexSearchMoveCaret(CodexSearchPreviousBoundary(g_CodexSearchCaret), extend);
}
inline void CodexSearchMoveRight(bool extend) {
    if (!extend && CodexSearchHasSelection())
        CodexSearchMoveCaret(CodexSearchSelectionEnd(), false);
    else CodexSearchMoveCaret(CodexSearchNextBoundary(g_CodexSearchCaret), extend);
}
inline void CodexSearchMoveHome(bool extend) { CodexSearchMoveCaret(0, extend); }
inline void CodexSearchMoveEnd(bool extend) { CodexSearchMoveCaret(g_CodexSearchLen, extend); }
inline void CodexSearchSelectAll() {
    g_CodexSearchAnchor = 0;
    g_CodexSearchCaret = g_CodexSearchLen;
    if (g_CodexSearchLen == 0) g_CodexSearchAnchor = -1;
}
inline std::wstring CodexSearchSelectedText() {
    if (!CodexSearchHasSelection()) return L"";
    return std::wstring(g_CodexSearch + CodexSearchSelectionStart(),
                        g_CodexSearch + CodexSearchSelectionEnd());
}
inline void CodexSearchInsertText(const wchar_t* text) {
    if (!text) return;
    CodexSearchClearComposition();
    CodexSearchDeleteSelection();
    for (int i = 0; text[i] && g_CodexSearchLen < 63; ++i) {
        const wchar_t ch = text[i];
        if (ch < 32 || ch == 127) continue;
        const int units = (ch >= 0xD800 && ch <= 0xDBFF &&
                           text[i + 1] >= 0xDC00 && text[i + 1] <= 0xDFFF) ? 2 : 1;
        if (g_CodexSearchLen + units > 63) break;
        for (int j = g_CodexSearchLen; j >= g_CodexSearchCaret; --j)
            g_CodexSearch[j + units] = g_CodexSearch[j];
        g_CodexSearch[g_CodexSearchCaret++] = ch;
        ++g_CodexSearchLen;
        if (units == 2) {
            g_CodexSearch[g_CodexSearchCaret++] = text[++i];
            ++g_CodexSearchLen;
        }
    }
    g_CodexSearchAnchor = -1;
}
inline void CodexSearchInsertCodepoint(unsigned int cp) {
    if (cp < 32 || cp == 127 || cp > 0x10FFFF) return;
    wchar_t chars[3] = {};
    if (cp <= 0xFFFF) chars[0] = (wchar_t)cp;
    else {
        cp -= 0x10000;
        chars[0] = (wchar_t)(0xD800 + (cp >> 10));
        chars[1] = (wchar_t)(0xDC00 + (cp & 0x3FF));
    }
    CodexSearchInsertText(chars);
}
inline void CodexSearchBackspace() {
    if (CodexSearchHasSelection()) CodexSearchDeleteSelection();
    else if (g_CodexSearchCaret > 0)
        CodexSearchEraseRange(CodexSearchPreviousBoundary(g_CodexSearchCaret), g_CodexSearchCaret);
}
inline void CodexSearchDeleteForward() {
    if (CodexSearchHasSelection()) CodexSearchDeleteSelection();
    else if (g_CodexSearchCaret < g_CodexSearchLen)
        CodexSearchEraseRange(g_CodexSearchCaret, CodexSearchNextBoundary(g_CodexSearchCaret));
}

inline bool CodexMatch(const wchar_t* name) {
    const std::wstring query = CodexSearchQuery();
    if (query.empty()) return true;
    if (!name || !name[0]) return false;
    std::wstring a(name);
    auto lc = [](std::wstring s) {
        for (auto& c : s) if (c < 128) c = (wchar_t)towlower(c);
        return s;
    };
    return lc(a).find(lc(query)) != std::wstring::npos;
}

inline bool CodexMatchAny(const wchar_t* const* names, int count) {
    if (CodexSearchQuery().empty()) return true;
    if (!names || count <= 0) return false;
    for (int i = 0; i < count; ++i)
        if (CodexMatch(names[i])) return true;
    return false;
}

inline bool CodexSearchMatchesAny(const wchar_t* const* names, int count) {
    return CodexMatchAny(names, count);
}

// ── 증강 발견 (ALL_AUGS 인덱스 기준) ──
inline bool g_AugSeen[AUG_TOTAL] = { false };
inline bool g_CodexDirty   = false;

inline void MarkAugSeen(int augIdx) {
    if (augIdx < 0 || augIdx >= AUG_TOTAL) return;
    if (!g_AugSeen[augIdx]) { g_AugSeen[augIdx] = true; g_CodexDirty = true; }
}

inline bool CodexAugSeen(int augIdx) {
    if (CodexFullReveal() && augIdx >= 0 && augIdx < AUG_TOTAL) return true;
    if (augIdx < 0 || augIdx >= AUG_TOTAL) return false;
    return g_AugSeen[augIdx];
}

// ── 새 적은 enum, kRosterOrder, CODEX_MOB_PROFILES에 추가하면 두 UI에 반영된다. ──
enum CodexMobId {
    CM_ROTOR,
    CM_GENESIS,
    CM_SCOPE,
    CM_SWARM,
    CM_GRAVIS,
    CM_QUASAR,
    CM_GIMBAL,
    CM_COUNT
};


struct CodexMobProfile {
    const wchar_t* name[3];
    const wchar_t* description[3];
    const wchar_t* threat;
    int tier;
    int baseHp;
    const wchar_t* damage;
    const wchar_t* speed;
    const wchar_t* attack[3];
    const wchar_t* form[3];
};

inline const CodexMobProfile CODEX_MOB_PROFILES[CM_COUNT] = {
    { { L"로터", L"ROTOR", L"ローター" },
      { L"단일 회전 프레임으로 플레이어를 집요하게 추적하는 기본 신호", L"Basic signal that relentlessly tracks the player with a single rotating frame", L"単一の回転フレームでプレイヤーを追跡する基本シグナル" },
      L"LOW", 1, 90, L"5 / sec (contact)", L"120-180 px/s",
      { L"플레이어 추적", L"Pursues the player", L"プレイヤーを追跡" },
      { L"네 개의 신호점이 회전하는 사각 프레임의 꼭짓점을 이룹니다.", L"Four signal nodes sit at the corners of a rotating square frame.", L"4つの信号点が回転する四角いフレームの頂点に配置されています。" } },
    { { L"제네시스", L"GENESIS", L"ジェネシス" },
      { L"전장에 고정되어 SWARM 신호 조각을 생성하는 생성 코어", L"Anchored genesis core that generates SWARM signal shards", L"戦場に固定されSWARM信号片を生成するジェネシスコア" },
      L"HIGH", 2, 315, L"5 / sec (contact)", L"42-63 px/s",
      { L"고정 후 스웜 조각 생성", L"Anchors and spawns Swarm shards", L"固定してスウォーム片を生成" },
      { L"육각 외곽 구조 안에 방사형 레일과 중심 코어가 배치되어 있습니다.", L"Radial rails and a central core sit inside a six-sided outer frame.", L"六角形の外枠の内側に、放射状のレールと中心コアがあります。" } },
    { { L"스코프", L"SCOPE", L"スコープ" },
      { L"중앙 코어와 조준선으로 원거리에서 공격하는 감시 신호", L"Ranged surveillance signal that attacks from afar with a central core and aim lanes", L"中央コアと照準線で遠距離攻撃する監視シグナル" },
      L"MODERATE", 2, 360, L"10 / shot (3-shot burst)", L"Variable",
      { L"유도탄 3발 연속 발사", L"Fires 3 homing shots per burst", L"誘導弾を3発連続発射" },
      { L"중앙 렌즈를 원형 고리와 네 개의 관측점이 둘러싸고 있습니다.", L"A central lens is surrounded by circular rings and four observation nodes.", L"中央のレンズを円形のリングと4つの観測点が囲んでいます。" } },
    { { L"스웜", L"SWARM", L"スウォーム" },
      { L"작은 신호 조각이 무리를 이루어 압박하는 물량형 신호", L"Swarm signal made of small shards that pressure the arena in large numbers", L"小さな信号片の群れで戦場を圧迫する物量型シグナル" },
      L"MODERATE", 1, 32, L"5 / sec (contact)", L"126-189 px/s",
      { L"무리를 이루어 접근", L"Pressures the player in groups", L"群れでプレイヤーに接近" },
      { L"세 개의 작은 신호점이 삼각형을 이루는 소형 형상입니다.", L"A small triangular form made from three signal nodes.", L"3つの小さな信号点で三角形を形作る小型の存在です。" } },
    { { L"그라비스", L"GRAVIS", L"グラビス" },
      { L"중력장으로 이동과 탄도 궤적을 왜곡하는 고위험 신호", L"High-threat signal that bends movement and bullet trajectories with gravity", L"重力場で移動と弾道を歪める高脅威シグナル" },
      L"HIGH", 3, 720, L"6 / sec (contact)", L"29-43 px/s",
      { L"중력장으로 이동과 탄도 왜곡", L"Distorts movement and projectiles", L"重力場で移動と弾道を歪める" },
      { L"중심 코어에서 여섯 신호점이 뻗으며 넓은 원형 장이 둘러쌉니다.", L"Six nodes extend from a central core, surrounded by a broad circular field.", L"中心コアから6つの信号点が伸び、広い円形の場が周囲を囲みます。" } },
    { { L"퀘이사", L"QUASAR", L"クエーサー" },
      { L"장거리 조준선을 교차시켜 전장을 통제하는 희귀 천체", L"Rare long-range signal that controls the arena with crossing aim lanes", L"交差する照準線で戦場を制御する希少シグナル" },
      L"HIGH", 2, 576, L"34 / sec (beam), 4 / sec (contact)", L"62-94 px/s",
      { L"2.5초 빔 공격 (재사용 3.2초)", L"2.5s beam (3.2s cooldown)", L"2.5秒ビーム（再使用3.2秒）" },
      { L"중심 코어를 길쭉한 타원 고리와 양쪽 축의 신호점이 감쌉니다.", L"An elongated elliptical ring and axial nodes surround the central core.", L"細長い楕円リングと軸上の信号点が中心コアを囲んでいます。" } },
    { { L"Gimbal", L"Gimbal", L"ジンバル" },
      { L"작은 스코프 형상으로 약 1200px 거리를 유지하며 느린 비유도탄 한 발을 쏘는 밝은 민트색 신호", L"A bright mint mini-Scope that holds about 1200px distance and fires one slow unguided shot", L"約1200pxの距離を保ちながら遅い非誘導弾を一発撃つ明るいミント色の小型スコープ" },
      L"MODERATE", 2, 149, L"8 / shot, 4 / sec (contact)", L"70-105 px/s",
      { L"스코프처럼 고속 회전 충전 후 비유도탄 1발 발사", L"Charges with Scope-like accelerating rotation before one unguided shot", L"スコープのような加速回転で充填後、非誘導弾を1発発射" },
      { L"중앙 렌즈와 회전 고리 하나, 네 개의 관측점으로 된 스코프의 소형 형상입니다.", L"A smaller Scope silhouette with one rotating ring, a central lens, and four observation nodes.", L"中央レンズ、1つの回転リング、4つの観測点を備えた小型スコープ形状です。" } },
};

inline const CodexMobProfile* CodexMobProfileFor(int id) {
    return id >= 0 && id < CM_COUNT ? &CODEX_MOB_PROFILES[id] : nullptr;
}

inline const wchar_t* CodexMobProfileText(const wchar_t* const* localized) {
    int li = (int)g_Language;
    if (li < 0 || li >= LANG_COUNT) li = 0;
    return localized[li];
}

inline int CodexMobTier(int id) {
    const CodexMobProfile* profile = CodexMobProfileFor(id);
    return profile ? profile->tier : 0;
}

inline int CodexMobBaseHp(int id) {
    const CodexMobProfile* profile = CodexMobProfileFor(id);
    return profile ? profile->baseHp : 0;
}

inline const wchar_t* CodexMobDamageValue(int id) {
    const CodexMobProfile* profile = CodexMobProfileFor(id);
    return profile ? profile->damage : L"?";
}

inline const wchar_t* CodexMobBaseSpeed(int id) {
    const CodexMobProfile* profile = CodexMobProfileFor(id);
    return profile ? profile->speed : L"?";
}

inline const wchar_t* CodexMobAttackPattern(int id) {
    const CodexMobProfile* profile = CodexMobProfileFor(id);
    return profile ? CodexMobProfileText(profile->attack) : L"?";
}

inline const wchar_t* CodexMobFormDescription(int id) {
    const CodexMobProfile* profile = CodexMobProfileFor(id);
    return profile ? CodexMobProfileText(profile->form) : L"";
}


inline bool g_MobSeen[CM_COUNT] = { false };
inline long long g_MobKillCounts[CM_COUNT] = {};
inline long long g_RunMobKillCounts[CM_COUNT] = {};
inline bool g_SuppressMobSeen = false;
inline void MarkMobSeenId(CodexMobId id);

inline CodexMobId CodexMobIdForKind(MobKind k) {
    switch (k) {
    case MobKind::ROTOR:   return CM_ROTOR;
    case MobKind::GENESIS: return CM_GENESIS;
    case MobKind::SWARM:   return CM_SWARM;
    case MobKind::GRAVIS:  return CM_GRAVIS;
    case MobKind::QUASAR:  return CM_QUASAR;
    case MobKind::GIMBAL:  return CM_GIMBAL;
    default:               return CM_COUNT;
    }
}

template <typename Fn>
inline void ForEachCodexMobEntry(Fn&& fn) {
    static constexpr CodexMobId kRosterOrder[] = {
        CM_ROTOR, CM_SCOPE, CM_GIMBAL, CM_SWARM, CM_GENESIS, CM_GRAVIS, CM_QUASAR
    };
    for (CodexMobId id : kRosterOrder) fn((int)id);
}

inline void MarkMobSeen(MobKind k) {
    if (g_SuppressMobSeen) return;
    MarkMobSeenId(CodexMobIdForKind(k));
}
inline void MarkMobSeenId(CodexMobId id) {
    if (id < 0 || id >= CM_COUNT) return;
    if (!g_MobSeen[id]) { g_MobSeen[id] = true; g_CodexDirty = true; }
}
inline void RegisterCodexMobKill(CodexMobId id) {
    if (g_CreativeMode || id < 0 || id >= CM_COUNT) return;
    ++g_RunMobKillCounts[id];
}
inline void RegisterCodexMobKill(MobKind kind) {
    RegisterCodexMobKill(CodexMobIdForKind(kind));
}

inline bool CodexMobSeen(int id) {
    if (CodexFullReveal() && id >= 0 && id < CM_COUNT) return true;
    if (id < 0 || id >= CM_COUNT) return false;
    return g_MobSeen[id];
}

inline long long CodexMobKillCount(int id) {
    if (id < 0 || id >= CM_COUNT) return 0;
    return g_MobKillCounts[id] + g_RunMobKillCounts[id];
}

inline long long CodexMobKillThreshold(int id, int milestone) {
    const int tier = CodexMobTier(id);
    if (tier < 1 || milestone < 0 || milestone > 2) return 0;
    static const long long kThresholds[3][3] = {
        { 1, 500, 2000 },
        { 1, 50, 400 },
        { 1, 20, 100 },
    };
    return kThresholds[tier - 1][milestone];
}

// 0=overview locked, 1=overview, 2=combat stats, 3=full profile.
inline int CodexMobDataStage(int id) {
    if (id < 0 || id >= CM_COUNT) return 0;
    const long long kills = CodexMobKillCount(id);
    if (CodexFullReveal()) return 3;
    if (kills >= CodexMobKillThreshold(id, 2)) return 3;
    if (kills >= CodexMobKillThreshold(id, 1)) return 2;
    if (kills >= CodexMobKillThreshold(id, 0)) return 1;
    return 0;
}

inline const wchar_t* CodexLocalizedText(const wchar_t* kr, const wchar_t* en,
                                         const wchar_t* jp) {
    int li = (int)g_Language;
    if (li < 0 || li >= LANG_COUNT) li = 0;
    const wchar_t* text[3] = { kr, en, jp };
    return text[li];
}

inline const wchar_t* const* MobLocalizedNames(int id) {
    const CodexMobProfile* profile = CodexMobProfileFor(id);
    return profile ? profile->name : nullptr;
}
inline const wchar_t* MobName(int id) {
    int li = (int)g_Language; if (li < 0 || li >= LANG_COUNT) li = 0;
    const wchar_t* const* names = MobLocalizedNames(id);
    return names ? names[li] : L"???";
}
inline bool CodexMobNameUnlocked(int id) {
    return CodexMobDataStage(id) >= 1;
}
inline const wchar_t* CodexMobListLabel(int id) {
    if (CodexMobNameUnlocked(id)) return MobName(id);
    if (!CodexMobSeen(id))
        return CodexLocalizedText(L"미발견", L"UNDISCOVERED", L"未発見");
    return L"???";
}
inline const wchar_t* MobDesc(int id) {
    int li = (int)g_Language; if (li < 0 || li >= LANG_COUNT) li = 0;
    if (id < 0 || id >= CM_COUNT) return L"???";
    return CODEX_MOB_PROFILES[id].description[li];
}


inline const wchar_t* MobThreatLabel(int id) {
    const CodexMobProfile* profile = CodexMobProfileFor(id);
    return profile ? profile->threat : L"UNKNOWN";
}

inline const wchar_t* MobTierLabel(int id) {
    switch (CodexMobTier(id)) {
    case 1: return L"1";
    case 2: return L"2";
    case 3: return L"3";
    default: return L"?";
    }
}

inline MobKind CodexMobKind(int id) {
    switch (id) {
    case CM_ROTOR:   return MobKind::ROTOR;
    case CM_GENESIS: return MobKind::GENESIS;
    case CM_SWARM:   return MobKind::SWARM;
    case CM_GRAVIS:  return MobKind::GRAVIS;
    case CM_QUASAR:  return MobKind::QUASAR;
    case CM_GIMBAL:  return MobKind::GIMBAL;
    default:         return MobKind::ROTOR;
    }
}
