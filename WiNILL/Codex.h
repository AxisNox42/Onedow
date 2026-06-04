#pragma once
// ─────────────────────────────────────────────────────────────
// 도감(Codex) — 발견 추적 + 적 정보
//   증강/조합/적은 "처음 획득/조우" 해야 도감에 공개. 그 전엔 ??? (정보 비공개).
//   발견 상태는 SaveSystem 으로 영구 저장.
// ─────────────────────────────────────────────────────────────
#include "Augment.h"   // AUG_TOTAL
#include "Monster.h"   // MobKind
#include "Settings.h"  // g_Language, LANG_COUNT

// ── 증강 발견 (ALL_AUGS 인덱스 기준) ──
inline bool g_AugSeen[96]  = { false };
inline bool g_CodexDirty   = false;   // 새 발견 발생 → main 에서 SaveGame 호출

inline void MarkAugSeen(int augIdx) {
    if (augIdx < 0 || augIdx >= 96) return;
    if (!g_AugSeen[augIdx]) { g_AugSeen[augIdx] = true; g_CodexDirty = true; }
}

// ── 적 도감 목록 (9 MobKind + 원거리 + 자폭병) ──
enum CodexMobId {
    CM_NORMAL, CM_SPLITTER, CM_BLINKER, CM_CHARGER, CM_WEAVER, CM_BRUTE,
    CM_ORBITER, CM_SPAWNER, CM_SHIELDED,   // == (int)MobKind 0..8
    CM_RANGED, CM_BOMBER,
    CM_COUNT
};
inline bool g_MobSeen[CM_COUNT] = { false };

inline void MarkMobSeen(MobKind k) {
    int i = (int)k;
    if (i < 0 || i >= 9) return;
    if (!g_MobSeen[i]) { g_MobSeen[i] = true; g_CodexDirty = true; }
}
inline void MarkMobSeenId(CodexMobId id) {
    if (id < 0 || id >= CM_COUNT) return;
    if (!g_MobSeen[id]) { g_MobSeen[id] = true; g_CodexDirty = true; }
}

// 적 이름 / 행동 설명 (KR / EN / JP)
struct MobInfo { const wchar_t* name[3]; const wchar_t* desc[3]; };
inline const MobInfo MOB_INFO[CM_COUNT] = {
    /* NORMAL */ {
        { L"일반체", L"Grunt", L"雑魚" },
        { L"플레이어를 향해 곧장 돌진하는 기본 잡몹",
          L"Basic mob that charges straight at you",
          L"プレイヤーへ直進する基本の雑魚" } },
    /* SPLITTER */ {
        { L"분열체", L"Splitter", L"分裂体" },
        { L"처치 시 작은 둘로 쪼개짐 (2세대까지)",
          L"Splits into two smaller ones on death (up to 2 gens)",
          L"撃破で2体に分裂 (2世代まで)" } },
    /* BLINKER */ {
        { L"점멸체", L"Blinker", L"点滅体" },
        { L"잔상 경고 후 플레이어 쪽으로 순간이동",
          L"Teleports toward you after a ghost telegraph",
          L"残像予告後にプレイヤーへ瞬間移動" } },
    /* CHARGER */ {
        { L"돌진체", L"Charger", L"突進体" },
        { L"멈춰서 조준(텔레그래프) 후 폭발적으로 돌진",
          L"Winds up (telegraph), then bursts forward in a dash",
          L"静止して照準後、爆発的に突進" } },
    /* WEAVER */ {
        { L"회피체", L"Weaver", L"回避体" },
        { L"좌우로 지그재그하며 접근 (조준 까다로움)",
          L"Zig-zags side to side while closing in",
          L"左右に蛇行しながら接近" } },
    /* BRUTE */ {
        { L"거대체", L"Brute", L"巨体" },
        { L"크고 느리지만 체력이 매우 높음 (보상 큼)",
          L"Big and slow but very high HP (big reward)",
          L"大きく遅いが体力が非常に高い" } },
    /* ORBITER */ {
        { L"공전체", L"Orbiter", L"公転体" },
        { L"플레이어 주위를 돌며 서서히 반경을 좁혀옴",
          L"Circles you, spiraling steadily inward",
          L"周囲を回り徐々に接近する" } },
    /* SPAWNER */ {
        { L"소환체", L"Spawner", L"召喚体" },
        { L"느리지만 작은 잡몹을 계속 소환",
          L"Slow, but keeps summoning small mobs",
          L"低速だが小雑魚を召喚し続ける" } },
    /* SHIELDED */ {
        { L"보호막체", L"Shielded", L"防御体" },
        { L"방패 ON 동안 피해 대폭 감소, 주기적으로 OFF (광역엔 무력)",
          L"Huge damage cut while shield is up, cycles off (AoE bypasses)",
          L"盾ON中は被害激減・周期的にOFF (範囲攻撃は貫通)" } },
    /* RANGED */ {
        { L"원거리체", L"Gunner", L"遠距離体" },
        { L"멀리서 멈춰 플레이어에게 탄을 발사",
          L"Stops at range and fires bullets at you",
          L"遠くから停止して弾を撃つ" } },
    /* BOMBER */ {
        { L"자폭병", L"Bomber", L"自爆兵" },
        { L"접근해 점화 후 광역 자폭 (총으로 미리 처치 가능)",
          L"Closes in, ignites, then blows up in an AoE",
          L"接近して点火後に範囲自爆" } },
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
