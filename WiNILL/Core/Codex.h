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

// ── 적 도감 목록 (9 MobKind + 원거리 + 자폭병) ──
enum CodexMobId {
    CM_NORMAL, CM_SPLITTER, CM_BLINKER, CM_CHARGER, CM_WEAVER, CM_BRUTE,
    CM_ORBITER, CM_SPAWNER, CM_SHIELDED,   // == (int)MobKind 0..8
    CM_RANGED, CM_BOMBER,
    CM_DDOS, CM_BADSECTOR, CM_REGERROR, CM_GRAVIS, CM_QUASAR, // append-only save indices
    CM_COUNT
};
inline bool g_MobSeen[CM_COUNT] = { false };
// 시작창 자동 데모처럼 "발견으로 치면 안 되는" 렌더 중엔 true (drawMob 의 MarkMobSeen 무시)
inline bool g_SuppressMobSeen = false;

inline void MarkMobSeen(MobKind k) {
    if (g_SuppressMobSeen) return;
    int i = (int)k;
    if (i < 0 || i >= 9) return;
    if (!g_MobSeen[i]) { g_MobSeen[i] = true; g_CodexDirty = true; }
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

// 적 이름 / 행동 설명 (KR / EN / JP)
struct MobInfo { const wchar_t* name[3]; const wchar_t* desc[3]; };
inline const MobInfo MOB_INFO[CM_COUNT] = {
    /* NORMAL */ {
        { L"침입체", L"Intruder", L"侵入体" },
        { L"플레이어를 향해 곧장 돌진하는 기본 침입체",
          L"Basic intruder that charges straight at you",
          L"プレイヤーへ直進する基本侵入体" } },
    /* SPLITTER */ {
        { L"웜", L"Worm", L"ワーム" },
        { L"자가복제 — 처치 시 작은 둘로 쪼개짐 (2세대까지)",
          L"Self-replicating — splits into two on death (up to 2 gens)",
          L"自己複製 — 撃破で2体に分裂 (2世代まで)" } },
    /* BLINKER */ {
        { L"트로이목마", L"Trojan", L"トロイの木馬" },
        { L"잔상 경고 후 플레이어 쪽으로 순간이동(잠입)",
          L"Teleports toward you after a ghost telegraph",
          L"残像予告後にプレイヤーへ瞬間移動" } },
    /* CHARGER */ {
        { L"크래셔", L"Crasher", L"クラッシャー" },
        { L"멈춰서 조준(텔레그래프) 후 폭발적으로 돌진",
          L"Winds up (telegraph), then bursts forward in a dash",
          L"静止して照準後、爆発的に突進" } },
    /* WEAVER */ {
        { L"버그", L"Bug", L"バグ" },
        { L"좌우로 지그재그하며 접근 (조준 까다로움)",
          L"Zig-zags side to side while closing in",
          L"左右に蛇行しながら接近" } },
    /* BRUTE */ {
        { L"거대 침입체", L"Titan", L"タイタン" },
        { L"크고 느리지만 체력이 매우 높음 (보상 큼)",
          L"Big and slow but very high HP (big reward)",
          L"大きく遅いが体力が非常に高い" } },
    /* ORBITER */ {
        { L"스파이웨어", L"Spyware", L"スパイウェア" },
        { L"플레이어 주위를 돌며 서서히 반경을 좁혀옴",
          L"Circles you, spiraling steadily inward",
          L"周囲を回り徐々に接近する" } },
    /* SPAWNER */ {
        { L"봇넷", L"Botnet", L"ボットネット" },
        { L"느리지만 작은 파편을 계속 소환",
          L"Slow, but keeps spawning DDoS star shards",
          L"低速だが小さな星片を召喚し続ける" } },
    /* SHIELDED */ {
        { L"방화벽", L"Firewall", L"ファイアウォール" },
        { L"방패 ON 동안 피해 대폭 감소, 주기적으로 OFF (광역엔 무력)",
          L"Huge damage cut while shield is up, cycles off (AoE bypasses)",
          L"盾ON中は被害激減・周期的にOFF (範囲攻撃は貫通)" } },
    /* RANGED */ {
        { L"애드웨어", L"Adware", L"アドウェア" },
        { L"멀리서 멈춰 팝업(탄)을 발사",
          L"Stops at range and fires popups (bullets) at you",
          L"遠くから停止してポップアップ(弾)を撃つ" } },
    /* BOMBER */ {
        { L"랜섬웨어", L"Ransomware", L"ランサムウェア" },
        { L"접근해 점화 후 광역 자폭 (미리 처치 가능)",
          L"Closes in, ignites, then blows up in an AoE",
          L"接近して点火後に範囲自爆" } },
    /* DDOS */ {
        { L"디도스", L"DDoS", L"DDoS" },
        { L"한 번에 떼로 몰려오는 약한 프로세스 (물량 압박)",
          L"Weak star-stream shards that swarm in large numbers",
          L"大量に押し寄せる弱小プロセス" } },
    /* BADSECTOR */ {
        { L"배드 섹터", L"Bad Sector", L"バッドセクタ" },
        { L"처치 시 그 자리에 잠시 감속 구역(손상 영역)을 남김",
          L"On death leaves a temporary slow zone (damaged area)",
          L"撃破時にその場へ一時的な減速領域を残す" } },
    /* REGERROR */ {
        { L"레지스트리 에러", L"Registry Error", L"レジストリエラー" },
        { L"별자리 영역의 적을 강화(이속·공격·체력↑)하는 X형 노드",
          L"X-node that buffs enemies inside its constellation field (speed/atk/HP)",
          L"星座領域の敵を強化するX字ノード" } },
    /* GRAVIS */ {
        { L"그라비스", L"Gravis", L"グラビス" },
        { L"중력장으로 플레이어와 탄환의 궤도를 끌어당기는 3티어 정예 천체",
          L"Tier-3 elite body that bends player and bullet trajectories with gravity",
          L"重力場でプレイヤーと弾道を曲げるTier-3精鋭天体" } },
    /* QUASAR */ {
        { L"퀘이사", L"Quasar", L"クエーサー" },
        { L"장거리 조준선을 교차시켜 전장을 통제하는 희귀 천체",
          L"Rare long-range astral entity that controls the arena with crossing aim lanes",
          L"交差する照準レーンで戦場を制御する希少天体" } },
};

inline const wchar_t* const* MobLocalizedNames(int id) {
    // Keep the canonical active-roster aliases together so search can find
    // the same signal from any supported display language.
    static const wchar_t* const activeNames[6][3] = {
        { L"\uB85C\uD130",     L"ROTOR",   L"\u30ED\u30FC\u30BF\u30FC" },
        { L"\uC81C\uB124\uC2DC\uC2A4", L"GENESIS", L"\u30B8\u30A7\u30CD\u30B7\u30B9" },
        { L"\uC2A4\uCF54\uD504",   L"SCOPE",   L"\u30B9\u30B3\u30FC\u30D7" },
        { L"\uC2A4\uC6DC",     L"SWARM",   L"\u30B9\u30A6\u30A9\u30FC\u30E0" },
        { L"\uADF8\uB77C\uBE44\uC2A4",  L"GRAVIS",  L"\u30B0\u30E9\u30F4\u30A3\u30B9" },
        { L"\uD018\uC774\uC0AC", L"QUASAR", L"\u30AF\u30A8\u30FC\u30B5\u30FC" },
    };
    if (id < 0 || id >= CM_COUNT) return nullptr;
    switch (id) {
    case CM_NORMAL:  return activeNames[0];
    case CM_SPAWNER: return activeNames[1];
    case CM_RANGED:  return activeNames[2];
    case CM_DDOS:    return activeNames[3];
    case CM_GRAVIS:  return activeNames[4];
    case CM_QUASAR:  return activeNames[5];
    default:         return MOB_INFO[id].name;
    }
}
inline const wchar_t* MobName(int id) {
    int li = (int)g_Language; if (li < 0 || li >= LANG_COUNT) li = 0;
    const wchar_t* const* names = MobLocalizedNames(id);
    return names ? names[li] : L"???";
}
inline const wchar_t* MobDesc(int id) {
    int li = (int)g_Language; if (li < 0 || li >= LANG_COUNT) li = 0;
    static const wchar_t* activeDesc[6][3] = {
        { L"단일 회전 프레임으로 플레이어를 집요하게 추적하는 기본 신호",
          L"Basic signal that relentlessly tracks the player with a single rotating frame",
          L"単一の回転フレームでプレイヤーを追跡する基本シグナル" },
        { L"전장에 고정되어 SWARM 신호 조각을 생성하는 생성 코어",
          L"Anchored genesis core that generates SWARM signal shards",
          L"戦場に固定されSWARM信号片を生成するジェネシスコア" },
        { L"중앙 코어와 조준선으로 원거리에서 공격하는 감시 신호",
          L"Ranged surveillance signal that attacks from afar with a central core and aim lanes",
          L"中央コアと照準線で遠距離攻撃する監視シグナル" },
        { L"작은 신호 조각이 무리를 이루어 압박하는 물량형 신호",
          L"Swarm signal made of small shards that pressure the arena in large numbers",
          L"小さな信号片の群れで戦場を圧迫する物量型シグナル" },
        { L"중력장으로 이동과 탄도 궤적을 왜곡하는 3T 정예 신호",
          L"Tier-3 elite signal that bends movement and bullet trajectories with gravity",
          L"重力場で移動と弾道を歪めるTier-3エリートシグナル" },
        { L"장거리 조준선을 교차시켜 전장을 통제하는 희귀 신호",
          L"Rare long-range signal that controls the arena with crossing aim lanes",
          L"交差する照準線で戦場を制御する希少シグナル" },
    };
    switch (id) {
    case CM_NORMAL:  return activeDesc[0][li];
    case CM_SPAWNER: return activeDesc[1][li];
    case CM_RANGED:  return activeDesc[2][li];
    case CM_DDOS:    return activeDesc[3][li];
    case CM_GRAVIS:  return activeDesc[4][li];
    case CM_QUASAR:  return activeDesc[5][li];
    default:         return MOB_INFO[id].desc[li];
    }
}

// CodexMobId(0..8) → MobKind (프리뷰 렌더용)
inline MobKind CodexMobKind(int id) { return (MobKind)id; }

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
