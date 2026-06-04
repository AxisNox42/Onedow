#pragma once
// ─────────────────────────────────────────────────────────────
// 업적 + 직업(클래스) 시스템
//   업적: 특정 조건(점수/킬/보스/증강 보유) 달성 시 1회 코인 보상 + 일부는 직업 해금.
//   직업: 시작 시 미리 정해진 증강을 들고 출발. 기본(방랑자)은 항상 선택 가능.
//   해금 상태/누적 보스킬은 SaveSystem 으로 영구 저장.
// ─────────────────────────────────────────────────────────────
#include "Meta.h"      // g_Coins, g_Language, LANG_COUNT
#include "Augment.h"   // AugType, ALL_AUGS, AUG_TOTAL

// ── 직업(클래스) ───────────────────────────────────
enum JobId {
    JOB_NONE,        // 방랑자 — 항상 선택 가능, 보너스 없음
    JOB_ASSASSIN,    // 암살자 — 치명타
    JOB_BERSERKER,   // 광전사 — 광전사 + 유리대포
    JOB_BOMBARDIER,  // 폭격수 — 연쇄 폭발
    JOB_VAMPIRE,     // 흡혈귀 — 흡혈탄 + 흡혈마
    JOB_SWORDSMAN,   // 검객 — 근접 호 스윙 (총 대신 칼)
    JOB_ARCHER,      // 궁수 — 관통 화살 (총 대신 활)
    JOB_COUNT
};

// ── 업적 ───────────────────────────────────────────
enum AchId {
    ACH_FIRST_BOSS,        // 보스 1마리 처치 (누적)
    ACH_BOSS_3,            // 보스 3마리 처치 (누적) → 흡혈귀 해금
    ACH_SCORE_300K,        // 한 판 30만 점
    ACH_SCORE_1M,          // 한 판 100만 점
    ACH_KILLS_500,         // 한 판 500 처치 → 광전사 해금
    ACH_CRIT_SCORE,        // 치명타 보유 + 한 판 20만 점 → 암살자 해금
    ACH_DEATHBLAST_KILLS,  // 연쇄폭발 보유 + 한 판 300 처치 → 폭격수 해금
    ACH_DEBUFF_5,          // 디버프 5개 동시 보유
    ACH_GAMES_10,          // 누적 10판 플레이
    ACH_SCORE_500K,        // 한 판 50만 점 → 검객 해금
    ACH_BOSS_10,           // 보스 10마리 처치 (누적) → 궁수 해금
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
        { L"보스 누적 3마리 처치  ·  흡혈귀 해금",
          L"Defeat 3 bosses total  ·  unlocks Vampire",
          L"ボス累計3体撃破  ·  吸血鬼解放" },
        300, JOB_VAMPIRE },
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
        { L"한 판에 500 처치  ·  광전사 해금",
          L"500 kills in one run  ·  unlocks Berserker",
          L"1ランで500撃破  ·  バーサーカー解放" },
        400, JOB_BERSERKER },
    /* ACH_CRIT_SCORE */ {
        { L"급소 강타", L"Vital Strike", L"急所打ち" },
        { L"치명타 보유 + 한 판 20만 점  ·  암살자 해금",
          L"Own Crit + 200k in one run  ·  unlocks Assassin",
          L"クリ所持 + 1ラン20万点  ·  アサシン解放" },
        350, JOB_ASSASSIN },
    /* ACH_DEATHBLAST_KILLS */ {
        { L"연쇄 학살", L"Chain Massacre", L"連鎖虐殺" },
        { L"연쇄 폭발 보유 + 한 판 300 처치  ·  폭격수 해금",
          L"Own Death Blast + 300 kills  ·  unlocks Bombardier",
          L"連鎖爆発所持 + 300撃破  ·  ボンバー解放" },
        400, JOB_BOMBARDIER },
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
        { L"한 판에 50만 점 달성  ·  검객 해금",
          L"Reach 500k in one run  ·  unlocks Swordsman",
          L"1ランで50万点  ·  剣士解放" },
        600, JOB_SWORDSMAN },
    /* ACH_BOSS_10 */ {
        { L"보스 학살자", L"Boss Slayer", L"ボススレイヤー" },
        { L"보스 누적 10마리 처치  ·  궁수 해금",
          L"Defeat 10 bosses total  ·  unlocks Archer",
          L"ボス累計10体撃破  ·  弓兵解放" },
        600, JOB_ARCHER },
};

struct JobDef {
    const wchar_t* name[3];
    const wchar_t* desc[3];
    int            unlockAch;          // AchId or -1 (항상 해금)
    AugType        startAugs[4];
    int            startAugCount;
    int            weaponMode;         // 0=총(기본) 1=근접(검) 2=관통화살(활)
};
inline const JobDef JOB_DEFS[JOB_COUNT] = {
    /* JOB_NONE */ {
        { L"방랑자", L"Wanderer", L"放浪者" },
        { L"보너스 없음 — 순수 실력", L"No bonus — pure skill", L"ボーナス無し — 実力勝負" },
        -1, {}, 0, 0 },
    /* JOB_ASSASSIN */ {
        { L"암살자", L"Assassin", L"アサシン" },
        { L"치명타 보유 시작", L"Start with Critical Strike", L"クリティカル所持で開始" },
        ACH_CRIT_SCORE, { AugType::CRIT }, 1, 0 },
    /* JOB_BERSERKER */ {
        { L"광전사", L"Berserker", L"バーサーカー" },
        { L"광전사 + 유리대포 (고위험·고화력)",
          L"Berserk + Glass Cannon (high risk/reward)",
          L"バーサーク + ガラスの大砲 (高リスク)" },
        ACH_KILLS_500, { AugType::BERSERK, AugType::GLASS_CANNON }, 2, 0 },
    /* JOB_BOMBARDIER */ {
        { L"폭격수", L"Bombardier", L"ボンバー" },
        { L"연쇄 폭발 보유 시작", L"Start with Death Blast", L"連鎖爆発所持で開始" },
        ACH_DEATHBLAST_KILLS, { AugType::DEATH_BLAST }, 1, 0 },
    /* JOB_VAMPIRE */ {
        { L"흡혈귀", L"Vampire", L"吸血鬼" },
        { L"흡혈탄 + 흡혈마 보유 시작", L"Start with Lifesteal + Vampire",
          L"吸血弾 + 吸血鬼で開始" },
        ACH_BOSS_3, { AugType::LIFESTEAL, AugType::VAMPIRE }, 2, 0 },
    /* JOB_SWORDSMAN */ {
        { L"검객", L"Swordsman", L"剣士" },
        { L"근접 칼 — 조준 방향 호 스윙 (근거리 고화력)",
          L"Melee blade — arc swing (high close-range DPS)",
          L"近接剣 — 扇状の斬撃 (近距離高火力)" },
        ACH_SCORE_500K, {}, 0, 1 },
    /* JOB_ARCHER */ {
        { L"궁수", L"Archer", L"弓兵" },
        { L"활 — 누른 만큼 강해지는 차징 관통 화살 (크기 최대 4배)",
          L"Bow — charge for a stronger piercing arrow (up to 4x size)",
          L"弓 — チャージで強い貫通矢 (最大4倍)" },
        ACH_BOSS_10, {}, 0, 2 },
};

// ── 영구/런타임 상태 ────────────────────────────────
inline bool      g_AchUnlocked[ACH_COUNT] = {};
inline long long g_TotalBossKills = 0;     // 누적 보스 처치 (저장됨)
inline int       g_SelectedJob    = JOB_NONE; // 현재 런 선택 직업 (저장 안 함)
inline bool      g_AchSaveNeeded  = false; // 해금 발생 → main 에서 SaveGame 호출

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

// 직업 해금 여부
inline bool JobUnlocked(int j) {
    if (j == JOB_NONE) return true;
    if (j < 0 || j >= JOB_COUNT) return false;
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
