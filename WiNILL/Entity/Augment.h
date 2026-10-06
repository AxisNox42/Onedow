#pragma once
#include <cstdlib>
#include "Platform.h"
#include "Settings.h"   // g_Language, LANG_COUNT

enum class AugType {
    // ── 일반 (7) ──────────────────────────────
    DMG_UP, RATE_UP, SPD_UP, MOVE_UP, VISION_UP, REGEN_UP,
    // ── 희귀 ──
    GLASS_CANNON, LIGHT_AMMO, LIGHT_STEP, RESERVED_AUG_037,
    BULLET_RAIN,
    RESERVED_AUG_004, // Reserved legacy slot.
    CRIT, LIFESTEAL, BERSERK,   // 핵앤슬래쉬 (치명타/흡혈/광전사)
    OVERDRIVE,                  // 희귀 공격력 +10 (가산)
    // ── 에픽 ──
    CORE_OVERLOAD,              // 에픽 공격력 +20 (가산)
    VAMPIRE, RESERVED_AUG_003, RESERVED_AUG_057, RESERVED_AUG_001,
    MINIATURIZE, GIGANTIFY, PIERCE, TWIN, CHAKRAM,
    BULLET_RAIN_2, CHAKRAM_2,
    DRONE,                 // 희귀 → 에픽
    RESERVED_AUG_043, HACK_RANGED, RESERVED_AUG_053, // Reserved legacy slot.
    LASER,                 // 스캔 레이저 (주기적 관통 빔 — 군중제어)
    RESERVED_AUG_050, // Reserved legacy slot.
    RESERVED_AUG_042, RESERVED_AUG_002, // Reserved legacy slot.
    RESERVED_AUG_049, RESERVED_AUG_046, // Reserved legacy slot.
    LASER_2, PIERCE_2, TWIN_2,        // 티어 연장 (레이저II · 관통II · 트리플)
    PROB_CHAIN,            // 확률적 연쇄 작용 (30% 3튕김)
    DEATH_BLAST,           // 연쇄 폭발 (적 사망 시 폭발)
    SKILL_CLOSE, SKILL_OVERCLOCK,     // 액티브 스킬 (창 닫기 · 초집중 — enum명 유지)
    // ── 전설 ──
    POWER_SURGE,                // 전설 공격력 ×1.05 (유일 곱연산 스케일러)
    RANDOM_AUG, RESERVED_AUG_059,
    BULLET_RAIN_3, DRONE_2, CHAKRAM_3,
    MK2, RESERVED_AUG_038, // Reserved legacy slot.
    CHAIN,                 // 연쇄 작용 (무조건 2튕김, -30% 데미지)
    SKILL_TIMESTOP,        // 액티브 스킬 (시간 정지)
    // ── 디버프 ──
    D_RMOB_MAX, D_RMOB_HP, D_RMOB_DELAY,
    D_MOB_SPAWN, D_APPROACH, D_MOB_SPEED,
    D_GLASS_HEART, D_BULLET_STUCK, RESERVED_AUG_023,
    RESERVED_AUG_017, RESERVED_AUG_018, RESERVED_AUG_019, // Reserved legacy slot.
    D_MOB_HP, D_SLOW_MOVE,                            // 신규
    RESERVED_AUG_031, RESERVED_AUG_016, // Reserved legacy slot.
    RESERVED_AUG_026, RESERVED_AUG_030, RESERVED_AUG_029, // Reserved legacy slot.
    D_BLEED, D_WEAKEN,                                // 출혈 · 약화
    // ── 특수 (2) ──────────────────────────────
    S_CHAOS, S_PANDORA,
    // ── 조합 (COMBO) — 레시피 충족 시에만 등장. 일반 추첨 X ──
    RESERVED_AUG_005, // Reserved legacy slot.
    CB_BLOODLORD,     // 흡혈탄 + 흡혈마
    RESERVED_AUG_009, // Reserved legacy slot.
    RESERVED_AUG_011, // Reserved legacy slot.
    // ── 로터류(잡몹) 전용 디버프 (확장) — 끝에 추가해 기존 인덱스/세이브 보존 ──
    RESERVED_AUG_061, // Retired mob-pack debuff slot.
    // Retired generation modifiers. Their numeric slots stay reserved so
    // old augment ownership/save indices remain valid.
    RESERVED_AUG_024,
    RESERVED_AUG_025,
    // ── 조합 (COMBO) 확장 — 끝에 추가해 기존 인덱스/세이브 보존 ──
    RESERVED_AUG_010, // Reserved legacy slot.
    RESERVED_AUG_006, // Reserved legacy slot.
    CB_WARLORD,      // 광전사 + 연쇄폭발 → 전쟁군주
    RESERVED_AUG_013, // Reserved legacy slot.
    RESERVED_AUG_008, // Reserved legacy slot.
    RESERVED_AUG_007, // Reserved legacy slot.
    // ── 신화(MYTHIC) — 전설보다 높은 등급. 티어 자체가 고유 메커니즘으로 바뀜 ──
    BULLET_RAIN_ETERNAL, // 무한 세례 — 쿨다운↓ + 처치마다 쿨다운 감소(스노우볼)
    DRONE_HIVE,          // 군집 지능 — 드론 4기 운용
    LASER_CONVERGE,      // 수렴 — 레이저 거의 연속 발사 + 초장거리
    PIERCE_RAILSLUG,     // 철갑탄 — 관통 100% + 관통탄 강화
    // ── 조합 (COMBO) 추가 — 끝에 추가해 기존 인덱스/세이브 보존 ──
    RESERVED_AUG_014, // Reserved legacy slot.
    // ── 디버프 (확장) — 끝에 추가해 기존 인덱스/세이브 보존 ──
    RESERVED_AUG_028, // Reserved legacy slot.
    RESERVED_AUG_033, // Reserved legacy slot.
    RESERVED_AUG_021, // Reserved legacy slot.
    // ── 적 출현 디버프 (확장 — 신규 적) ──
    RESERVED_AUG_015, // Reserved legacy slot.
    RESERVED_AUG_027, // Reserved legacy slot.
    // ── 확장 (끝에 추가 — 세이브 인덱스 보존) ──
    RESERVED_AUG_022, // Reserved legacy slot.
    RESERVED_AUG_034, // Reserved legacy slot.
    RESERVED_AUG_020, // Reserved legacy slot.
    LIFESTEAL_2,         // 흡혈탄 II (에픽 티어)
    CHAIN_2,             // 연쇄 작용 II (전설 티어)
    RESERVED_AUG_054, // Reserved legacy slot.
    RESERVED_AUG_051, // Reserved legacy slot.
    RESERVED_AUG_040, // Reserved legacy slot.
    CHAKRAM_SINGULARITY, // 특이점 (신화 — 차크람 III 진화)
    RESERVED_AUG_055, // Reserved legacy slot.
    SKILL_DASH_UP,       // [스킬] 대시 강화
    // ── 총기 전용 (끝에 추가 — 세이브 인덱스 보존) ──
    RESERVED_AUG_056, // Reserved legacy slot.
    RIFLE_STABILITY,     // 소총 — 정조준
    RESERVED_AUG_058, // Reserved legacy slot.
    // ── 위성/FIELD/DROP (끝에 추가 — 세이브 인덱스 보존) ──
    STATIC_FIELD,        // 정전기장 — 근접 펄스 링 DoT
    STATIC_FIELD_2,      // 정전기장 II
    RESERVED_AUG_035, // Reserved legacy slot.
    RESERVED_AUG_047, // Reserved legacy slot.
    RESERVED_AUG_060, // Reserved legacy slot.
    RESERVED_AUG_048, // Reserved legacy slot.
    RESERVED_AUG_036, // Reserved legacy slot.
    // ── 생존 빌드 (끝에 추가 — 세이브 인덱스 보존) ──
    HP_UP,               // 체력 증가 (일반)
    FIREWALL,            // 방화벽 — 받는 피해 감소
    REGEN_2,             // 재생 II (에픽)
    CB_BASTION,          // 조합: 거대화 + MK2 + 방화벽
    CB_LIFEBUOY,         // 조합: 재생 II + 흡혈마 + 가벼운 발걸음
    // Reserved augment tier slots.
    RESERVED_AUG_044, // Reserved legacy slot.
    RESERVED_AUG_045, // Reserved legacy slot.
    DEATH_BLAST_2,       // 연쇄 폭발 II (전설 티어)
    RESERVED_AUG_012, // Reserved legacy slot.
    // ── 확장 (끝에 추가 — 세이브 인덱스 보존) ──
    RESERVED_AUG_039, // Reserved legacy slot.
    RESERVED_AUG_052, // Reserved legacy slot.
    RESERVED_AUG_041, // Reserved legacy slot.
    RESERVED_AUG_032, // Reserved legacy slot.
};

// (int)AugType 으로 g_TypeOwned·g_IconTex 등에 인덱싱 — enum 끝에만 추가
static constexpr int AUG_TYPE_SLOTS = 160;
static_assert((int)AugType::RESERVED_AUG_032 < AUG_TYPE_SLOTS,
              "AugType enum grew past AUG_TYPE_SLOTS — bump the constant");

enum class AugRarity { COMMON, RARE, EPIC, LEGENDARY, DEBUFF, SPECIAL, COMBO, MYTHIC };

// 고유 카테고리 — 같은 카테고리 내에서 1개만 선택 가능
enum class AugUnique { NONE, SIZE };

// 위성/시스템 계열 (빌드 연결·동기화 판정용)
enum class AugSubsystem { NONE, BEAM, ORBIT, BURST, FIELD, DROP, SUMMON };

struct AugDef {
    AugType        type;
    AugRarity      rarity;
    AugUnique      unique;
    const char*    name;                  // 영문 코드명 (윈도우 타이틀/디버그)
    const wchar_t* locName[LANG_COUNT];   // [KR, EN, JP] 카드 이름
    const wchar_t* locDesc[LANG_COUNT];   // [KR, EN, JP] 카드 설명
    const wchar_t* stat;                  // 핵심 수치 (단일 라인, 언어 무관)
};

// 현재 언어 인덱스 (범위 클램프)
inline int CurLangIdx() {
    int li = (int)g_Language;
    if (li < 0 || li >= LANG_COUNT) li = 0;
    return li;
}
inline const wchar_t* AugName(const AugDef& d) { return d.locName[CurLangIdx()]; }
inline const wchar_t* AugStat(const AugDef& d) { return (d.stat && d.stat[0]) ? d.stat : L"—"; }

static constexpr int AUG_TOTAL = 125;
extern const AugDef ALL_AUGS[];

// ── 조합 레시피 — result 는 COMBO 등급 AugType, reqs 를 모두 보유하면 등장 ──
struct ComboDef {
    AugType result;
    AugType reqs[3];
    int     reqCount;
};
inline const ComboDef COMBO_DEFS[] = {
    // 재료 3개 · 최대 티어/전설 선행 — 단순 스탯 합친 조합은 AugRemoved
    { AugType::CB_BLOODLORD, { AugType::LIFESTEAL, AugType::LIFESTEAL_2, AugType::VAMPIRE }, 3 },
    { AugType::CB_WARLORD,   { AugType::BERSERK,   AugType::DEATH_BLAST, AugType::CHAIN_2  }, 3 },
    { AugType::CB_BASTION,   { AugType::GIGANTIFY, AugType::MK2,         AugType::FIREWALL  }, 3 },
    { AugType::CB_LIFEBUOY,  { AugType::REGEN_2,   AugType::VAMPIRE,     AugType::LIGHT_STEP }, 3 },
};
inline const int COMBO_COUNT = (int)(sizeof(COMBO_DEFS) / sizeof(COMBO_DEFS[0]));

// ALL_AUGS 에서 특정 AugType 의 인덱스 (없으면 -1)
inline int AugIndexOfType(AugType t) {
    for (int i = 0; i < AUG_TOTAL; i++)
        if (ALL_AUGS[i].type == t) return i;
    return -1;
}

// Retired augment slots stay addressable for save compatibility but never enter gameplay.
inline bool AugRemoved(AugType t) {
    switch (t) {
    // Reserved legacy slot.
    case AugType::RESERVED_AUG_004:
    case AugType::RESERVED_AUG_040:
    case AugType::RESERVED_AUG_041:
    case AugType::RESERVED_AUG_014:
    case AugType::RESERVED_AUG_057:
    case AugType::RESERVED_AUG_053:
    case AugType::RESERVED_AUG_043:
    case AugType::RESERVED_AUG_044:
    case AugType::RESERVED_AUG_045:
    case AugType::RESERVED_AUG_054:
    case AugType::RESERVED_AUG_051:
    case AugType::RESERVED_AUG_052:
    case AugType::RESERVED_AUG_055:
    case AugType::RESERVED_AUG_056:
    case AugType::RESERVED_AUG_058:
    case AugType::RESERVED_AUG_010:
    case AugType::RESERVED_AUG_012:
    case AugType::RESERVED_AUG_001:
    case AugType::RESERVED_AUG_042:
    case AugType::RESERVED_AUG_002:
    case AugType::RESERVED_AUG_049:
    case AugType::RESERVED_AUG_046:
    case AugType::RESERVED_AUG_003:
    case AugType::RESERVED_AUG_050:
    case AugType::RESERVED_AUG_037:
    case AugType::RESERVED_AUG_023:
    case AugType::RESERVED_AUG_059:
    case AugType::RESERVED_AUG_005: // Reserved legacy slot.
    case AugType::RESERVED_AUG_009: // Reserved legacy slot.
    case AugType::RESERVED_AUG_011: // Reserved legacy slot.
    case AugType::RESERVED_AUG_013: // Reserved legacy slot.
    case AugType::RESERVED_AUG_008: // Reserved legacy slot.
    case AugType::RESERVED_AUG_006: // Reserved legacy slot.
    case AugType::RESERVED_AUG_007: // Reserved legacy slot.
    // Reserved legacy slot.
    case AugType::RESERVED_AUG_035:
    case AugType::RESERVED_AUG_047:
    case AugType::RESERVED_AUG_060:
    case AugType::RESERVED_AUG_048:
    case AugType::RESERVED_AUG_036:
    // Reserved legacy slot.
    // Reserved legacy slot.
    // Reserved legacy slot.
    case AugType::RESERVED_AUG_031: // Reserved legacy slot.
    case AugType::RESERVED_AUG_016: // Reserved legacy slot.
    case AugType::RESERVED_AUG_026: // Reserved legacy slot.
    case AugType::RESERVED_AUG_030: // Reserved legacy slot.
    case AugType::RESERVED_AUG_028: // Reserved legacy slot.
    case AugType::RESERVED_AUG_029: // Reserved legacy slot.
    case AugType::RESERVED_AUG_015: // Reserved legacy slot.
    case AugType::RESERVED_AUG_027: // Reserved legacy slot.
    case AugType::RESERVED_AUG_022: // Reserved legacy slot.
    case AugType::RESERVED_AUG_032: // Reserved legacy slot.
    case AugType::RESERVED_AUG_033: // Reserved legacy slot.
    case AugType::RESERVED_AUG_021: // Reserved legacy slot.
    case AugType::RESERVED_AUG_034: // Reserved legacy slot.
    case AugType::RESERVED_AUG_020: // Reserved legacy slot.
    case AugType::RESERVED_AUG_017: // Reserved legacy slot.
    case AugType::RESERVED_AUG_018:
    case AugType::RESERVED_AUG_019:
    case AugType::RESERVED_AUG_038: // Reserved legacy slot.
    case AugType::RESERVED_AUG_039: // Reserved legacy slot.
    case AugType::RESERVED_AUG_061: // Retired mob-pack debuff slot.
    case AugType::RESERVED_AUG_024:
    case AugType::RESERVED_AUG_025:
        return true;
    default:
        return false;
    }
}

// 현재 런에서 해당 AugType 을 보유 중인지 ((int)AugType 인덱스). 조합 레시피 판정용.
//   main 의 applyByIdx 가 갱신, ResetForNewGame 이 초기화.
inline bool g_TypeOwned[AUG_TYPE_SLOTS] = { false };

// 한 번만 등장해야 하는 증강/디버프 (스택 불가 플래그형) — 픽 풀에서 takenOnce 로 제외
inline bool AugOnceOnly(AugType t, AugRarity r) {
    // Identity-slot tiers and debuff cards are unique within a run.
    if (r == AugRarity::EPIC || r == AugRarity::LEGENDARY ||
        r == AugRarity::COMBO || r == AugRarity::MYTHIC ||
        r == AugRarity::DEBUFF)
        return true;

    // These are rare cards whose effects are explicitly non-stackable. They
    // cannot be covered by rarity alone because other rare cards (crit,
    // lifesteal, regen, etc.) are intentionally stackable.
    switch (t) {
    case AugType::BULLET_RAIN: // Tiered card; only the base tier once.
    case AugType::GLASS_CANNON:
    case AugType::LIGHT_AMMO:
    case AugType::LIGHT_STEP:
    case AugType::BERSERK:
    case AugType::FIREWALL:
        return true;
    default:
        return false;
    }
}

inline void GetRarityColor(AugRarity r, float& cr, float& cg, float& cb) {
    switch (r) {
    case AugRarity::COMMON:    cr = 0.15f; cg = 0.65f; cb = 0.20f; break; // 녹
    case AugRarity::RARE:      cr = 0.10f; cg = 0.30f; cb = 0.85f; break; // 청
    case AugRarity::EPIC:      cr = 0.55f; cg = 0.10f; cb = 0.80f; break; // 보라
    case AugRarity::LEGENDARY: cr = 0.95f; cg = 0.70f; cb = 0.10f; break; // 금
    case AugRarity::DEBUFF:    cr = 0.55f; cg = 0.10f; cb = 0.10f; break; // 어두운 빨강
    case AugRarity::SPECIAL:   cr = 0.85f; cg = 0.10f; cb = 0.55f; break; // 마젠타
    case AugRarity::COMBO:     cr = 0.10f; cg = 0.85f; cb = 0.80f; break; // 청록(시안)
    case AugRarity::MYTHIC:    cr = 1.00f; cg = 0.25f; cb = 0.35f; break; // 신화(진홍)
    }
}

// 도감·일시정지·크리에이티브 공통 — 카테고리 그룹
enum class AugListGroup {
    SKILL, WEAPON, ORBIT, COMBO, MYTHIC, SPECIAL, SIZE, STAT, DEBUFF
};

inline bool AugIsSkillType(AugType t) {
    return t == AugType::SKILL_CLOSE     || t == AugType::SKILL_OVERCLOCK ||
           t == AugType::SKILL_TIMESTOP  ||
           t == AugType::SKILL_DASH_UP;
}

inline bool AugIsWeaponType(AugType t) {
    switch (t) {
    case AugType::HACK_RANGED:
    case AugType::RIFLE_STABILITY:
        return true;
    default:
        return false;
    }
}

inline bool AugIsOrbitType(AugType t) {
    switch (t) {
    case AugType::DRONE: case AugType::DRONE_2: case AugType::DRONE_HIVE:
    case AugType::CHAKRAM: case AugType::CHAKRAM_2: case AugType::CHAKRAM_3:
    case AugType::CHAKRAM_SINGULARITY:
    case AugType::LASER: case AugType::LASER_2: case AugType::LASER_CONVERGE:
    case AugType::BULLET_RAIN: case AugType::BULLET_RAIN_2:
    case AugType::BULLET_RAIN_3: case AugType::BULLET_RAIN_ETERNAL:
        return true;
    default:
        return false;
    }
}

inline AugListGroup AugListGroupOf(const AugDef& d) {
    if (d.rarity == AugRarity::DEBUFF) return AugListGroup::DEBUFF;
    if (d.rarity == AugRarity::COMBO)  return AugListGroup::COMBO;
    if (d.rarity == AugRarity::MYTHIC) return AugListGroup::MYTHIC;
    if (d.rarity == AugRarity::SPECIAL)return AugListGroup::SPECIAL;
    if (AugIsSkillType(d.type))        return AugListGroup::SKILL;
    if (AugIsWeaponType(d.type))       return AugListGroup::WEAPON;
    if (AugIsOrbitType(d.type))        return AugListGroup::ORBIT;
    if (d.unique == AugUnique::SIZE)     return AugListGroup::SIZE;
    return AugListGroup::STAT;
}

inline int AugListGroupOrder(AugListGroup g) {
    switch (g) {
    case AugListGroup::SKILL:    return 0;
    case AugListGroup::WEAPON:   return 1;
    case AugListGroup::ORBIT:    return 2;
    case AugListGroup::COMBO:    return 3;
    case AugListGroup::MYTHIC:   return 4;
    case AugListGroup::SPECIAL:  return 5;
    case AugListGroup::SIZE:     return 6;
    case AugListGroup::STAT:     return 8;
    case AugListGroup::DEBUFF:   return 9;
    default: return 99;
    }
}

inline const wchar_t* AugListGroupLabel(AugListGroup g) {
    int li = CurLangIdx();
    switch (g) {
    case AugListGroup::SKILL:
        { static const wchar_t* s[3]={L"-- 스킬 --",L"-- Skills --",L"-- スキル --"}; return s[li]; }
    case AugListGroup::WEAPON:
        { static const wchar_t* s[3]={L"-- 무기 --",L"-- Weapons --",L"-- 武器 --"}; return s[li]; }
    case AugListGroup::ORBIT:
        { static const wchar_t* s[3]={L"-- 오빗·동료 --",L"-- Orbit·Allies --",L"-- 軌道·僚 --"}; return s[li]; }
    case AugListGroup::COMBO:
        { static const wchar_t* s[3]={L"-- 조합 --",L"-- Combo --",L"-- 組合 --"}; return s[li]; }
    case AugListGroup::MYTHIC:
        { static const wchar_t* s[3]={L"-- 신화 --",L"-- Mythic --",L"-- 神話 --"}; return s[li]; }
    case AugListGroup::SPECIAL:
        { static const wchar_t* s[3]={L"-- 특수 --",L"-- Special --",L"-- 特殊 --"}; return s[li]; }
    case AugListGroup::SIZE:
        { static const wchar_t* s[3]={L"-- 크기 --",L"-- Size --",L"-- サイズ --"}; return s[li]; }
    case AugListGroup::STAT:
        { static const wchar_t* s[3]={L"-- 강화 --",L"-- Stats --",L"-- 強化 --"}; return s[li]; }
    case AugListGroup::DEBUFF:
        { static const wchar_t* s[3]={L"-- 디버프 --",L"-- Debuffs --",L"-- デバフ --"}; return s[li]; }
    default:
        return L"-- ? --";
    }
}

// 보유 증강·도감 리스트 표시 순서 (enum 값과 무관)
inline int OwnedAugListOrder(AugRarity r) {
    switch (r) {
    case AugRarity::COMMON:    return 0;
    case AugRarity::RARE:      return 1;
    case AugRarity::EPIC:      return 2;
    case AugRarity::LEGENDARY: return 3;
    case AugRarity::COMBO:     return 4;
    case AugRarity::MYTHIC:    return 5;
    case AugRarity::SPECIAL:   return 6;
    case AugRarity::DEBUFF:    return 7;
    default: return 99;
    }
}

// 카테고리 → 등급 순 정렬 (도감·보유·크리에이티브 공통)
inline bool AugListIndexLess(int a, int b) {
    AugListGroup ga = AugListGroupOf(ALL_AUGS[a]);
    AugListGroup gb = AugListGroupOf(ALL_AUGS[b]);
    int oa = AugListGroupOrder(ga), ob = AugListGroupOrder(gb);
    if (oa != ob) return oa < ob;
    int ra = OwnedAugListOrder(ALL_AUGS[a].rarity);
    int rb = OwnedAugListOrder(ALL_AUGS[b].rarity);
    if (ra != rb) return ra < rb;
    return a < b;
}

// 등급(티어) 순 정렬 — 도감 기본값
inline bool AugTierIndexLess(int a, int b) {
    int ra = OwnedAugListOrder(ALL_AUGS[a].rarity);
    int rb = OwnedAugListOrder(ALL_AUGS[b].rarity);
    if (ra != rb) return ra < rb;
    return a < b;
}

// 등급 라벨 (현재 언어)
inline const wchar_t* GetRarityKR(AugRarity r) {
    int li = CurLangIdx();
    switch (r) {
    case AugRarity::COMMON:    { static const wchar_t* s[3]={L"일반",L"Common",L"コモン"};       return s[li]; }
    case AugRarity::RARE:      { static const wchar_t* s[3]={L"희귀",L"Rare",L"レア"};           return s[li]; }
    case AugRarity::EPIC:      { static const wchar_t* s[3]={L"에픽",L"Epic",L"エピック"};        return s[li]; }
    case AugRarity::LEGENDARY: { static const wchar_t* s[3]={L"전설",L"Legendary",L"レジェンド"}; return s[li]; }
    case AugRarity::DEBUFF:    { static const wchar_t* s[3]={L"디버프",L"Debuff",L"デバフ"};      return s[li]; }
    case AugRarity::SPECIAL:   { static const wchar_t* s[3]={L"특수",L"Special",L"スペシャル"};   return s[li]; }
    case AugRarity::COMBO:     { static const wchar_t* s[3]={L"조합",L"Combo",L"組合"};           return s[li]; }
    case AugRarity::MYTHIC:    { static const wchar_t* s[3]={L"신화",L"Mythic",L"神話"};         return s[li]; }
    }
    return L"?";
}

// 카테고리 태그 (고유 크기, 스킬) — 없으면 nullptr
inline const wchar_t* GetAugTag(const AugDef& a) {
    int li = CurLangIdx();
    if (a.unique == AugUnique::SIZE)     { static const wchar_t* s[3]={L"크기",L"Size",L"サイズ"}; return s[li]; }
    if (a.type == AugType::SKILL_CLOSE || a.type == AugType::SKILL_OVERCLOCK ||
        a.type == AugType::SKILL_TIMESTOP ||
        a.type == AugType::SKILL_DASH_UP)
        { static const wchar_t* s[3]={L"스킬",L"Skill",L"スキル"}; return s[li]; }
    return nullptr;
}

// 등급 + 카테고리 배지 — "희귀|크기" · "에픽|스킬" 식. 태그 없으면 등급만.
//   (단일 정적 버퍼 — UI 단일 스레드 1회 사용 가정)
inline const wchar_t* GetAugBadge(const AugDef& a) {
    static wchar_t buf[48];
    const wchar_t* rar = GetRarityKR(a.rarity);
    const wchar_t* tag = GetAugTag(a);
    if (tag) swprintf_s(buf, L"%ls|%ls", rar, tag);
    else     swprintf_s(buf, L"%ls", rar);
    return buf;
}

#include "AugmentDescKR.h"
inline const wchar_t* AugDesc(const AugDef& d) {
    if (CurLangIdx() == 0) {
        const wchar_t* kr = AugDescKR(d.type);
        if (kr && kr[0]) return kr;
    }
    return d.locDesc[CurLangIdx()];
}
