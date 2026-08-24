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

extern wchar_t g_CodexSearch[32];
extern int     g_CodexSearchLen;

inline void CodexSearchClear() { g_CodexSearch[0] = 0; g_CodexSearchLen = 0; }

inline bool CodexMatch(const wchar_t* name) {
    if (g_CodexSearchLen == 0) return true;
    std::wstring a(name), b(g_CodexSearch);
    auto lc = [](std::wstring s) {
        for (auto& c : s) if (c < 128) c = (wchar_t)towlower(c);
        return s;
    };
    return lc(a).find(lc(b)) != std::wstring::npos;
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
    CM_DDOS, CM_BADSECTOR, CM_REGERROR,    // 확장 적 (꼬리 추가 — 세이브 인덱스 보존)
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
        { L"프로세스", L"Process", L"プロセス" },
        { L"플레이어를 향해 곧장 돌진하는 기본 프로세스",
          L"Basic process that charges straight at you",
          L"プレイヤーへ直進する基本プロセス" } },
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
        { L"커널 프로세스", L"Kernel", L"カーネル" },
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
        { L"느리지만 작은 프로세스를 계속 소환",
          L"Slow, but keeps spawning DDoS swarm shards",
          L"低速だが小プロセスを召喚し続ける" } },
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
        { L"가짜창 내부의 적을 강화(이속·공격·체력↑)하는 X형 노드",
          L"X-node that buffs enemies inside its window (speed/atk/HP)",
          L"窓内の敵を強化するX字ノード" } },
};

inline const wchar_t* MobName(int id) {
    int li = (int)g_Language; if (li < 0 || li >= LANG_COUNT) li = 0;
    return MOB_INFO[id].name[li];
}
inline const wchar_t* MobDesc(int id) {
    int li = (int)g_Language; if (li < 0 || li >= LANG_COUNT) li = 0;
    return MOB_INFO[id].desc[li];
}

// CodexMobId(0..8) → MobKind (프리뷰 렌더용)
inline MobKind CodexMobKind(int id) { return (MobKind)id; }

// ── 보스 도감 (활성 로스터: 2,8,10 — 나머지는 크리에이티브 전용/미참전) ──
#include "BossDirector.h"

inline bool g_BossSeenPick[11] = { false };

inline void MarkBossSeenPick(int pick) {
    if (pick < 0 || pick > 10) return;
    if (!g_BossSeenPick[pick]) { g_BossSeenPick[pick] = true; g_CodexDirty = true; }
}

inline const int BOSS_CODEX_PICKS[] = { 2, 10, 8 };
inline const int BOSS_CODEX_COUNT = 3;

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
inline const wchar_t* BossCodexDesc(int idx) {
    return BossDir::Tagline(BossCodexPick(idx));
}
