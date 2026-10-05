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

extern bool g_DevUnlocked;   // main.cpp — 도감 develop_mod 해금

inline bool CodexFullReveal() {
    return g_CreativeMode || g_DevUnlocked;
}

extern wchar_t g_CodexSearch[64];
extern int     g_CodexSearchLen;
inline bool    g_CodexSearchInputEnabled = false;

inline void CodexSearchClear() { g_CodexSearch[0] = 0; g_CodexSearchLen = 0; }

inline bool CodexMatch(const wchar_t* name) {
    if (g_CodexSearchLen == 0) return true;
    if (!name || !name[0]) return false;
    std::wstring a(name), b(g_CodexSearch);
    auto lc = [](std::wstring s) {
        for (auto& c : s) if (c < 128) c = (wchar_t)towlower(c);
        return s;
    };
    return lc(a).find(lc(b)) != std::wstring::npos;
}

inline bool CodexMatchAny(const wchar_t* const* names, int count) {
    if (g_CodexSearchLen == 0) return true;
    if (!names || count <= 0) return false;
    for (int i = 0; i < count; ++i)
        if (CodexMatch(names[i])) return true;
    return false;
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

// ── 적 도감 목록 (현재 등장하는 6종) ──
enum CodexMobId {
    CM_ROTOR,
    CM_GENESIS,
    CM_SCOPE,
    CM_SWARM,
    CM_GRAVIS,
    CM_QUASAR,
    CM_COUNT
};

struct CodexMobProfile {
    const wchar_t* name[3];
    MobKind kind;
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
    { { L"로터", L"ROTOR", L"ローター" }, MobKind::ROTOR,
      { L"단일 회전 프레임으로 플레이어를 집요하게 추적하는 기본 신호", L"Basic signal that relentlessly tracks the player with a single rotating frame", L"単一の回転フレームでプレイヤーを追跡する基本シグナル" },
      L"LOW", 1, 90, L"5 / sec (contact)", L"120-180 px/s",
      { L"플레이어 추적", L"Pursues the player", L"プレイヤーを追跡" },
      { L"네 개의 신호점이 회전하는 사각 프레임의 꼭짓점을 이룹니다.", L"Four signal nodes sit at the corners of a rotating square frame.", L"4つの信号点が回転する四角いフレームの頂点に配置されています。" } },
    { { L"제네시스", L"GENESIS", L"ジェネシス" }, MobKind::GENESIS,
      { L"전장에 고정되어 SWARM 신호 조각을 생성하는 생성 코어", L"Anchored genesis core that generates SWARM signal shards", L"戦場に固定されSWARM信号片を生成するジェネシスコア" },
      L"HIGH", 2, 315, L"5 / sec (contact)", L"42-63 px/s",
      { L"고정 후 스웜 조각 생성", L"Anchors and spawns Swarm shards", L"固定してスウォーム片を生成" },
      { L"육각 외곽 구조 안에 방사형 레일과 중심 코어가 배치되어 있습니다.", L"Radial rails and a central core sit inside a six-sided outer frame.", L"六角形の外枠の内側に、放射状のレールと中心コアがあります。" } },
    { { L"스코프", L"SCOPE", L"スコープ" }, MobKind::SCOPE,
      { L"중앙 코어와 조준선으로 원거리에서 공격하는 감시 신호", L"Ranged surveillance signal that attacks from afar with a central core and aim lanes", L"中央コアと照準線で遠距離攻撃する監視シグナル" },
      L"MODERATE", 2, 360, L"10 / shot (3-shot burst)", L"Variable",
      { L"유도탄 3발 연속 발사", L"Fires 3 homing shots per burst", L"誘導弾を3発連続発射" },
      { L"중앙 렌즈를 원형 고리와 네 개의 관측점이 둘러싸고 있습니다.", L"A central lens is surrounded by circular rings and four observation nodes.", L"中央のレンズを円形のリングと4つの観測点が囲んでいます。" } },
    { { L"스웜", L"SWARM", L"スウォーム" }, MobKind::SWARM,
      { L"작은 신호 조각이 무리를 이루어 압박하는 물량형 신호", L"Swarm signal made of small shards that pressure the arena in large numbers", L"小さな信号片の群れで戦場を圧迫する物量型シグナル" },
      L"MODERATE", 1, 32, L"5 / sec (contact)", L"126-189 px/s",
      { L"무리를 이루어 접근", L"Pressures the player in groups", L"群れでプレイヤーに接近" },
      { L"세 개의 작은 신호점이 삼각형을 이루는 소형 형상입니다.", L"A small triangular form made from three signal nodes.", L"3つの小さな信号点で三角形を形作る小型の存在です。" } },
    { { L"그라비스", L"GRAVIS", L"グラビス" }, MobKind::GRAVIS,
      { L"중력장으로 이동과 탄도 궤적을 왜곡하는 고위험 신호", L"High-threat signal that bends movement and bullet trajectories with gravity", L"重力場で移動と弾道を歪める高脅威シグナル" },
      L"HIGH", 3, 720, L"6 / sec (contact)", L"29-43 px/s",
      { L"중력장으로 이동과 탄도 왜곡", L"Distorts movement and projectiles", L"重力場で移動と弾道を歪める" },
      { L"중심 코어에서 여섯 신호점이 뻗으며 넓은 원형 장이 둘러쌉니다.", L"Six nodes extend from a central core, surrounded by a broad circular field.", L"中心コアから6つの信号点が伸び、広い円形の場が周囲を囲みます。" } },
    { { L"퀘이사", L"QUASAR", L"クエーサー" }, MobKind::QUASAR,
      { L"장거리 조준선을 교차시켜 전장을 통제하는 희귀 천체", L"Rare long-range signal that controls the arena with crossing aim lanes", L"交差する照準線で戦場を制御する希少シグナル" },
      L"HIGH", 2, 576, L"34 / sec (beam), 4 / sec (contact)", L"62-94 px/s",
      { L"2.5초 빔 공격 (재사용 3.2초)", L"2.5s beam (3.2s cooldown)", L"2.5秒ビーム（再使用3.2秒）" },
      { L"중심 코어를 길쭉한 타원 고리와 양쪽 축의 신호점이 감쌉니다.", L"An elongated elliptical ring and axial nodes surround the central core.", L"細長い楕円リングと軸上の信号点が中心コアを囲んでいます。" } },
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
    for (int id = 0; id < CM_COUNT; ++id)
        if (CODEX_MOB_PROFILES[id].kind == k) return (CodexMobId)id;
    return CM_COUNT;
}
template <typename Fn>
inline void ForEachCodexMobEntry(Fn&& fn) {
    static constexpr CodexMobId kRosterOrder[] = {
        CM_ROTOR, CM_SCOPE, CM_SWARM, CM_GENESIS, CM_GRAVIS, CM_QUASAR
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

inline const wchar_t* const* MobLocalizedNames(int id) {
    const CodexMobProfile* profile = CodexMobProfileFor(id);
    return profile ? profile->name : nullptr;
}
inline const wchar_t* MobName(int id) {
    int li = (int)g_Language; if (li < 0 || li >= LANG_COUNT) li = 0;
    const wchar_t* const* names = MobLocalizedNames(id);
    return names ? names[li] : L"???";
}
inline const wchar_t* MobDesc(int id) {
    int li = (int)g_Language; if (li < 0 || li >= LANG_COUNT) li = 0;
    if (id < 0 || id >= CM_COUNT) return L"???";
    return CODEX_MOB_PROFILES[id].description[li];
}

// Threat labels are authored per signal, rather than inferred from the enum
// order.  Enum order is a save/codex identity detail and does not represent
// combat strength.
inline const wchar_t* MobThreatLabel(int id) {
    const CodexMobProfile* profile = CodexMobProfileFor(id);
    return profile ? profile->threat : L"UNKNOWN";
}
inline MobKind CodexMobKind(int id) {
    const CodexMobProfile* profile = CodexMobProfileFor(id);
    return profile ? profile->kind : MobKind::ROTOR;
}
