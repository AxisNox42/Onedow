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
inline bool g_MobSeen[CM_COUNT] = { false };
inline bool g_SuppressMobSeen = false;
inline void MarkMobSeenId(CodexMobId id);

inline void MarkMobSeen(MobKind k) {
    if (g_SuppressMobSeen) return;
    CodexMobId id = CM_ROTOR;
    switch (k) {
    case MobKind::ROTOR:  id = CM_ROTOR;  break;
    case MobKind::GENESIS: id = CM_GENESIS; break;
    case MobKind::SWARM:    id = CM_SWARM;    break;
    case MobKind::GRAVIS:  id = CM_GRAVIS;  break;
    case MobKind::QUASAR:  id = CM_QUASAR;  break;
    default: return;
    }
    MarkMobSeenId(id);
}
inline void MarkMobSeenId(CodexMobId id) {
    if (id < 0 || id >= CM_COUNT) return;
    if (!g_MobSeen[id]) { g_MobSeen[id] = true; g_CodexDirty = true; }
}

inline bool CodexMobSeen(int id) {
    if (CodexFullReveal() && id >= 0 && id < CM_COUNT) return true;
    if (id < 0 || id >= CM_COUNT) return false;
    return g_MobSeen[id];
}

struct MobInfo { const wchar_t* name[3]; const wchar_t* desc[3]; };
inline const MobInfo MOB_INFO[CM_COUNT] = {
    { { L"로터", L"ROTOR", L"ローター" },
      { L"단일 회전 프레임으로 플레이어를 집요하게 추적하는 기본 신호",
        L"Basic signal that relentlessly tracks the player with a single rotating frame",
        L"単一の回転フレームでプレイヤーを追跡する基本シグナル" } },
    { { L"제네시스", L"GENESIS", L"ジェネシス" },
      { L"전장에 고정되어 SWARM 신호 조각을 생성하는 생성 코어",
        L"Anchored genesis core that generates SWARM signal shards",
        L"戦場に固定されSWARM信号片を生成するジェネシスコア" } },
    { { L"스코프", L"SCOPE", L"スコープ" },
      { L"중앙 코어와 조준선으로 원거리에서 공격하는 감시 신호",
        L"Ranged surveillance signal that attacks from afar with a central core and aim lanes",
        L"中央コアと照準線で遠距離攻撃する監視シグナル" } },
    { { L"스웜", L"SWARM", L"スウォーム" },
      { L"작은 신호 조각이 무리를 이루어 압박하는 물량형 신호",
        L"Swarm signal made of small shards that pressure the arena in large numbers",
        L"小さな信号片の群れで戦場を圧迫する物量型シグナル" } },
    { { L"그라비스", L"GRAVIS", L"グラビス" },
      { L"중력장으로 이동과 탄도 궤적을 왜곡하는 고위험 신호",
        L"High-threat signal that bends movement and bullet trajectories with gravity",
        L"重力場で移動と弾道を歪める高脅威シグナル" } },
    { { L"퀘이사", L"QUASAR", L"クエーサー" },
      { L"장거리 조준선을 교차시켜 전장을 통제하는 희귀 천체",
        L"Rare long-range signal that controls the arena with crossing aim lanes",
        L"交差する照準線で戦場を制御する希少シグナル" } },
};

inline const wchar_t* const* MobLocalizedNames(int id) {
    if (id < 0 || id >= CM_COUNT) return nullptr;
    return MOB_INFO[id].name;
}
inline const wchar_t* MobName(int id) {
    int li = (int)g_Language; if (li < 0 || li >= LANG_COUNT) li = 0;
    const wchar_t* const* names = MobLocalizedNames(id);
    return names ? names[li] : L"???";
}
inline const wchar_t* MobDesc(int id) {
    int li = (int)g_Language; if (li < 0 || li >= LANG_COUNT) li = 0;
    if (id < 0 || id >= CM_COUNT) return L"???";
    return MOB_INFO[id].desc[li];
}

// Threat labels are authored per signal, rather than inferred from the enum
// order.  Enum order is a save/codex identity detail and does not represent
// combat strength.
inline const wchar_t* MobThreatLabel(int id) {
    switch (id) {
    case CM_ROTOR:   return L"LOW";
    case CM_SCOPE:   return L"MODERATE";
    case CM_SWARM:   return L"MODERATE";
    case CM_GENESIS: return L"HIGH";
    case CM_GRAVIS:  return L"HIGH";
    case CM_QUASAR:  return L"HIGH";
    default:         return L"UNKNOWN";
    }
}

inline MobKind CodexMobKind(int id) {
    switch (id) {
    case CM_ROTOR:  return MobKind::ROTOR;
    case CM_GENESIS: return MobKind::GENESIS;
    case CM_SWARM:    return MobKind::SWARM;
    case CM_GRAVIS:  return MobKind::GRAVIS;
    case CM_QUASAR:  return MobKind::QUASAR;
    default:         return MobKind::ROTOR;
    }
}

// ── 보스 도감 (활성 로스터: LEVIATHAN, VOLLEY, TESSERACT, FORK) ──
#include "BossDirector.h"

inline bool g_BossSeenPick[32] = { false };

inline void MarkBossSeenPick(int pick) {
    if (pick < 0 || pick >= 32) return;
    if (!g_BossSeenPick[pick]) { g_BossSeenPick[pick] = true; g_CodexDirty = true; }
}

inline const int BOSS_CODEX_PICKS[] = { 30, 2, 10, 8 };
inline const int BOSS_CODEX_COUNT = 4;

inline bool BossCodexSeen(int idx) {
    if (CodexFullReveal() && idx >= 0 && idx < BOSS_CODEX_COUNT) return true;
    if (idx < 0 || idx >= BOSS_CODEX_COUNT) return false;
    return g_BossSeenPick[BOSS_CODEX_PICKS[idx]];
}
inline int BossCodexPick(int idx) {
    if (idx < 0 || idx >= BOSS_CODEX_COUNT) return -1;
    return BOSS_CODEX_PICKS[idx];
}
inline const wchar_t* BossCodexName(int idx) {
    return BossDir::DisplayName(BossCodexPick(idx));
}
inline const wchar_t* const* BossCodexLocalizedNames(int idx) {
    static const wchar_t* const names[BOSS_CODEX_COUNT][3] = {
        { L"LEVIATHAN", L"LEVIATHAN", L"리바이어던" },
        { L"VOLLEY",    L"VOLLEY",    L"\u30DC\u30EC\u30FC" },
        { L"TESSERACT", L"TESSERACT", L"\u30C6\u30C3\u30BB\u30E9\u30AF\u30C8" },
        { L"FORK",      L"FORK",      L"\u30D5\u30A9\u30FC\u30AF" },
    };
    if (idx < 0 || idx >= BOSS_CODEX_COUNT) return nullptr;
    return names[idx];
}
inline const wchar_t* BossCodexDesc(int idx) {
    return BossDir::Tagline(BossCodexPick(idx));
}
