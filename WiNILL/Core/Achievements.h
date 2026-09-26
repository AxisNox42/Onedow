#pragma once
// ─────────────────────────────────────────────────────────────
// 업적 + 직업(클래스) 시스템
//   업적: 특정 조건(점수/킬/보스/증강 보유) 달성 시 1회 코인 보상 + 일부는 직업 해금.
//   직업: 시작 증강 효과는 적용되나 g_TypeOwned에는 넣지 않음 → 조합 레시피와 분리
//   해금 상태/누적 보스킬은 SaveSystem 으로 영구 저장.
// ─────────────────────────────────────────────────────────────
#include "Meta.h"      // g_Coins, g_Language, LANG_COUNT
#include "Augment.h"   // AugType, ALL_AUGS, AUG_TOTAL
#include "Weapons.h"   // StartWeapon (직업별 고정 무기)

// ── 직업(클래스) ───────────────────────────────────
enum JobId {
    JOB_NONE,              // Standard rifle.
    JOB_RESERVED_1,
    JOB_RESERVED_2,
    JOB_RESERVED_3,
    JOB_STATIC_FIELD,      // Static field; retains the old SMG save slot.
    JOB_RESERVED_4,
    JOB_RESERVED_5,
    JOB_COUNT
};
// Only the rifle and static field are reachable from a new run. Reserved
// slots keep old save-file indices stable without exposing retired jobs.
inline constexpr int JOB_PLAYABLE = 2;
inline bool IsPlayableJob(int j) {
    return j == JOB_NONE || j == JOB_STATIC_FIELD;
}

// ── 업적 ───────────────────────────────────────────
enum AchId {
    ACH_FIRST_BOSS,        // 보스 1마리 처치 (누적)
    ACH_BOSS_3,            // Cumulative boss defeats.
    ACH_SCORE_300K,        // 한 판 30만 점
    ACH_SCORE_1M,          // 한 판 100만 점
    ACH_KILLS_500,         // Cumulative kill milestone.
    ACH_CRIT_SCORE,       // Crit score milestone.
    ACH_DEATHBLAST_KILLS, // Death blast kill milestone.
    ACH_DEBUFF_5,          // 디버프 5개 동시 보유
    ACH_GAMES_10,          // 누적 10판 플레이
    ACH_SCORE_500K,        // Score milestone.
    ACH_BOSS_10,           // Cumulative boss defeats.
    ACH_COUNT
};

struct AchDef {
    const wchar_t* name[3];   // KR / EN / JP
    const wchar_t* desc[3];
    long long      coinReward;
    int            unlockJob;  // JobId or -1
};
inline const AchDef ACH_DEFS[ACH_COUNT] = {
    /* ACH_FIRST_BOSS */ {
        { L"첫 사냥", L"First Hunt", L"初狩り" },
        { L"보스를 처음으로 처치", L"Defeat your first boss", L"初めてボスを撃破" },
        200, -1 },
    /* ACH_BOSS_3 */ {
        { L"보스 사냥꾼", L"Boss Hunter", L"ボスハンター" },
        { L"보스 누적 3마리 처치",
          L"Defeat 3 bosses total",
          L"ボス累計3体撃破" },
        300, -1 },
    /* ACH_SCORE_300K */ {
        { L"고득점", L"High Scorer", L"高得点" },
        { L"한 판에 30만 점 달성", L"Reach 300k score in one run", L"1ランで30万点達成" },
        300, -1 },
    /* ACH_SCORE_1M */ {
        { L"백만장자", L"Millionaire", L"ミリオネア" },
        { L"한 판에 100만 점 달성", L"Reach 1,000,000 in one run", L"1ランで100万点達成" },
        1000, -1 },
    /* ACH_KILLS_500 */ {
        { L"학살자", L"Slaughterer", L"虐殺者" },
        { L"한 판에 500 처치",
          L"500 kills in one run",
          L"1ランで500撃破" },
        400, -1 },
    /* ACH_CRIT_SCORE */ {
        { L"급소 강타", L"Vital Strike", L"急所打ち" },
        { L"치명타 보유 + 한 판 20만 점",
          L"Own Crit + 200k in one run",
          L"クリ所持 + 1ラン20万点" },
        350, -1 },
    /* ACH_DEATHBLAST_KILLS */ {
        { L"연쇄 학살", L"Chain Massacre", L"連鎖虐殺" },
        { L"연쇄 폭발 보유 + 한 판 300 처치",
          L"Own Death Blast + 300 kills",
          L"連鎖爆発所持 + 300撃破" },
        400, -1 },
    /* ACH_DEBUFF_5 */ {
        { L"위험 감수", L"Risk Taker", L"危険を冒す" },
        { L"디버프 5개를 동시에 보유", L"Hold 5 debuffs at once", L"デバフを同時に5個所持" },
        300, -1 },
    /* ACH_GAMES_10 */ {
        { L"단골", L"Regular", L"常連" },
        { L"누적 10판 플레이", L"Play 10 runs total", L"累計10ランプレイ" },
        200, -1 },
    /* ACH_SCORE_500K */ {
        { L"베테랑", L"Veteran", L"ベテラン" },
        { L"한 판에 50만 점 달성", L"Reach 500k in one run", L"1ランで50万点達成" },
        600, -1 },
    /* ACH_BOSS_10 */ {
        { L"보스 학살자", L"Boss Slayer", L"ボススレイヤー" },
        { L"보스 누적 10마리 처치", L"Defeat 10 bosses total", L"ボス累計10体撃破" },
        600, -1 },
};

struct JobDef {
    const wchar_t* name[3];
    const wchar_t* desc[3];
    int            unlockAch;          // AchId or -1 (항상 해금)
    AugType        startAugs[4];
    int            startAugCount;
    int            fixedWeapon;        // StartWeapon 인덱스 (-1=해당 없음)
};
inline const JobDef JOB_DEFS[JOB_COUNT] = {
    /* JOB_NONE */ {
        { L"방랑자", L"Wanderer", L"放浪者" },
        { L"소총 — 표준 연사, 균형 잡힌 범용 무기",
          L"Rifle — standard fire rate, balanced all-rounder",
          L"ライフル — 標準連射、バランス型汎用武器" },
        -1, {}, 0, (int)StartWeapon::RIFLE },
    /* JOB_RESERVED_1 */ {
        { L"REMOVED", L"REMOVED", L"REMOVED" },
        { L"This job is no longer available", L"This job is no longer available", L"This job is no longer available" },
        -1, {}, 0, -1 },
    /* JOB_RESERVED_2 */ {
        { L"REMOVED", L"REMOVED", L"REMOVED" },
        { L"This job is no longer available", L"This job is no longer available", L"This job is no longer available" },
        -1, {}, 0, -1 },
    /* JOB_RESERVED_3 */ {
        { L"REMOVED", L"REMOVED", L"REMOVED" },
        { L"This job is no longer available", L"This job is no longer available", L"This job is no longer available" },
        -1, {}, 0, -1 },
    /* JOB_STATIC_FIELD */ {
        { L"정전기장", L"STATIC FIELD", L"静電場" },
        { L"플레이어 주변을 지속 전기장으로 제압하는 범위형 무기",
          L"Area weapon that controls nearby space with a sustained electric field",
          L"プレイヤー周囲を持続電場で制圧する範囲武器" },
        -1, {}, 0, (int)StartWeapon::SMG },
    /* JOB_RESERVED_4 */ {
        { L"REMOVED", L"REMOVED", L"REMOVED" },
        { L"This job is no longer available", L"This job is no longer available", L"This job is no longer available" },
        -1, {}, 0, -1 },
    /* JOB_RESERVED_5 */ {
        { L"REMOVED", L"REMOVED", L"REMOVED" },
        { L"This job is no longer available", L"This job is no longer available", L"This job is no longer available" },
        -1, {}, 0, -1 },
};

// ── 영구/런타임 상태 ────────────────────────────────
inline bool      g_AchUnlocked[ACH_COUNT] = {};
inline long long g_TotalBossKills = 0;     // 누적 보스 처치 (저장됨)
inline int       g_SelectedJob    = JOB_NONE; // 현재 런 선택 직업 (저장 안 함)
inline bool      g_AchSaveNeeded  = false; // 해금 발생 → main 에서 SaveGame 호출
inline bool      g_JobBought[JOB_COUNT] = {}; // 골드로 구매한 직업 해금 (저장됨)

// 해금 토스트 (in-game 배너)
inline int       g_AchToastId    = -1;
inline float     g_AchToastTimer = 0.0f;

// 로컬라이즈 헬퍼
inline const wchar_t* AchName(int id) {
    int li = (int)g_Language; if (li < 0 || li >= LANG_COUNT) li = 0;
    return ACH_DEFS[id].name[li];
}
inline const wchar_t* AchDesc(int id) {
    int li = (int)g_Language; if (li < 0 || li >= LANG_COUNT) li = 0;
    return ACH_DEFS[id].desc[li];
}
inline const wchar_t* JobName(int id) {
    int li = (int)g_Language; if (li < 0 || li >= LANG_COUNT) li = 0;
    return JOB_DEFS[id].name[li];
}
inline const wchar_t* JobDesc(int id) {
    int li = (int)g_Language; if (li < 0 || li >= LANG_COUNT) li = 0;
    return JOB_DEFS[id].desc[li];
}

// AugType → ALL_AUGS 인덱스 (직업 시작 증강 적용용)
inline int AugIndexOf(AugType t) {
    for (int i = 0; i < AUG_TOTAL; i++)
        if (ALL_AUGS[i].type == t) return i;
    return -1;
}

// 직업 해금 여부 (항상 해금 or 골드 구매 or 업적)
inline bool JobUnlocked(int j) {
    if (j < 0 || j >= JOB_COUNT) return false;
    if (!IsPlayableJob(j)) return false;
    if (j == JOB_NONE || j == JOB_STATIC_FIELD) return true;
    if (g_JobBought[j]) return true;
    int a = JOB_DEFS[j].unlockAch;
    return (a < 0) || (a < ACH_COUNT && g_AchUnlocked[a]);
}

// 업적 1회 해금 — 코인 지급 + 토스트 + 저장 예약 (이미 해금이면 무시)
inline void TryUnlockAch(int id) {
    if (id < 0 || id >= ACH_COUNT || g_AchUnlocked[id]) return;
    g_AchUnlocked[id] = true;
    g_Coins          += ACH_DEFS[id].coinReward;
    g_AchToastId      = id;
    g_AchToastTimer   = 4.0f;
    g_AchSaveNeeded   = true;
}
