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
    LASER_2, PIERCE_2, TWIN_2,        // 티어 연장 (레이저II / 관통II / 트리플)
    PROB_CHAIN,            // 확률적 연쇄 작용 (30% 3튕김)
    DEATH_BLAST,           // 연쇄 폭발 (적 사망 시 폭발)
    SKILL_CLOSE, SKILL_OVERCLOCK,     // 액티브 스킬 (창 닫기 / 초집중 — enum명 유지)
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
    D_BLEED, D_WEAKEN,                                // 출혈 / 약화
    // ── 특수 (2) ──────────────────────────────
    S_CHAOS, S_PANDORA,
    // ── 조합 (COMBO) — 레시피 충족 시에만 등장. 일반 추첨 X ──
    RESERVED_AUG_005, // Reserved legacy slot.
    CB_BLOODLORD,     // 흡혈탄 + 흡혈마
    RESERVED_AUG_009, // Reserved legacy slot.
    RESERVED_AUG_011, // Reserved legacy slot.
    // ── 로터류(잡몹) 전용 디버프 (확장) — 끝에 추가해 기존 인덱스/세이브 보존 ──
    D_MOB_PACK,    // 군집 스폰 (한 번에 여러 마리)
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

static const AugDef ALL_AUGS[] = {
    // ── 일반 ───────────────────────────────────────────
    { AugType::DMG_UP,        AugRarity::COMMON,    AugUnique::NONE, "DMG+5",
      { L"공격력 증가", L"Attack Up", L"攻撃力アップ" },
      { L"공격력 +9  (고정 가산)", L"Attack +9  (flat)", L"攻撃力 +9  (固定)" },
      L"ATK +9  (가산)" },
    { AugType::RATE_UP,       AugRarity::COMMON,    AugUnique::NONE, "RATE+4%",
      { L"연사 속도 증가", L"Fire Rate Up", L"連射速度アップ" },
      { L"연사 속도 +4%  (백분율 누적)", L"Fire rate +4%  (percent)", L"連射速度 +4%  (%累積)" },
      L"연사 +4%" },
    { AugType::SPD_UP,        AugRarity::COMMON,    AugUnique::NONE, "BSPD+30",
      { L"탄속 증가", L"Bullet Speed Up", L"弾速アップ" },
      { L"탄환 속도 +30", L"Bullet speed +30", L"弾速 +30" },
      L"탄속 +30" },
    { AugType::MOVE_UP,       AugRarity::COMMON,    AugUnique::NONE, "MOVE+5%",
      { L"이동속도 증가", L"Move Speed Up", L"移動速度アップ" },
      { L"이동 속도 +5%", L"Move speed +5%", L"移動速度 +5%" },
      L"이동 +5%" },
    { AugType::VISION_UP,     AugRarity::COMMON,    AugUnique::NONE, "VISION+70",
      { L"시야 증가", L"Vision Up", L"視界アップ" },
      { L"시야(필드 크기) +70  /  최대 5중첩 (+350)",
        L"View (field) +70  /  max 5 stacks (+350)",
        L"視界(フィールド) +70  /  最大5重 (+350)" },
      L"시야 +70  /  최대 5중첩" },
    { AugType::REGEN_UP,      AugRarity::COMMON,    AugUnique::NONE, "REGEN",
      { L"체력 재생", L"Regeneration", L"体力リジェネ" },
      { L"약 3.5초마다 체력 1 추가 회복  (중첩 가능)",
        L"Heal ~1 HP every 3.5s  (stackable)",
        L"約3.5秒ごとに体力1回復  (重複可)" },
      L"재생 +0.28/s" },

    // ── 희귀 ───────────────────────────────────────────
    { AugType::GLASS_CANNON,  AugRarity::RARE,      AugUnique::NONE, "GLASSCANNON",
      { L"유리대포", L"Glass Cannon", L"ガラスの大砲" },
      { L"공격력 +50%  /  최대 체력 -35%", L"Attack +50%  /  Max HP -35%", L"攻撃力 +50%  /  最大体力 -35%" },
      L"ATK +50%  /  Max HP -35%" },
    { AugType::LIGHT_AMMO,    AugRarity::RARE,      AugUnique::NONE, "LIGHTAMMO",
      { L"가벼운 탄환", L"Light Ammo", L"軽量弾" },
      { L"연사 +10%  /  탄속 +30%  /  공격력 -15%",
        L"Fire rate +10%  /  speed +30%  /  Attack -15%",
        L"連射 +10%  /  弾速 +30%  /  攻撃力 -15%" },
      L"연사 +10%  /  탄속 +30%  /  ATK -15%" },
    { AugType::LIGHT_STEP,    AugRarity::RARE,      AugUnique::NONE, "LIGHTSTEP",
      { L"가벼운 발걸음", L"Light Step", L"軽い足取り" },
      { L"이동 속도 +30%  /  피격 시 10초간 이 효과 정지",
        L"Move speed +30%  /  disabled 10s when hit",
        L"移動速度 +30%  /  被弾時10秒間 無効" },
      L"이동 +30%  /  피격 10초 해제" },
    { AugType::RESERVED_AUG_037, AugRarity::RARE, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::BULLET_RAIN,   AugRarity::RARE,      AugUnique::NONE, "BULLETRAIN",
      { L"탄환 세례", L"Bullet Rain", L"弾幕の雨" },
      { L"15초마다 유도탄 20발 일제 발사  /  발당 데미지 50%",
        L"Every 15s fire 20 homing shots  /  50% dmg each",
        L"15秒ごとに誘導弾20発  /  1発50%ダメージ" },
      L"유도탄 ×20발  /  쿨 15초" },
    { AugType::CRIT,          AugRarity::RARE,      AugUnique::NONE, "CRIT",
      { L"치명타", L"Critical Strike", L"クリティカル" },
      { L"15% 확률로 치명타 (피해 ×2.0)  (중첩 시 확률 +15%, 최대 75%)",
        L"15% chance to crit (×2.0 dmg)  (stacks +15%, max 75%)",
        L"15%でクリティカル (×2.0)  (重複で+15%, 最大75%)" },
      L"크리 +15%  /  배율 ×2.0" },
    { AugType::LIFESTEAL,     AugRarity::RARE,      AugUnique::NONE, "LIFESTEAL",
      { L"흡혈탄", L"Lifesteal", L"吸血弾" },
      { L"처치당 HP +0.06  (최대 4중첩 · 합 0.24/kill)",
        L"Heal +0.06/kill  (max 4 stacks · 0.24 total)",
        L"撃破毎 +0.06  (最大4重 · 合計0.24)" },
      L"처치당 HP +0.06  (4중첩)" },
    { AugType::BERSERK,       AugRarity::RARE,      AugUnique::NONE, "BERSERK",
      { L"광전사", L"Berserker", L"バーサーカー" },
      { L"체력이 낮을수록 공격력 증가  (최대 +60% · 빈사 시)",
        L"Lower HP = more attack  (up to +60% near death)",
        L"低HPほど攻撃力上昇  (瀕死時 最大+60%)" },
      L"저체력 시 ATK 최대 +60%" },
    { AugType::OVERDRIVE,     AugRarity::RARE,      AugUnique::NONE, "OVERDRIVE",
      { L"오버드라이브", L"Overdrive", L"オーバードライブ" },
      { L"공격력 +14  (고정 가산)", L"Attack +14  (flat)", L"攻撃力 +14  (固定)" },
      L"ATK +14  (가산)" },

    // ── 에픽 ───────────────────────────────────────────
    { AugType::CORE_OVERLOAD, AugRarity::EPIC,      AugUnique::NONE, "CORE_OVERLOAD",
      { L"코어 과부하", L"Core Overload", L"コア過負荷" },
      { L"공격력 +24  (고정 가산)", L"Attack +24  (flat)", L"攻撃力 +24  (固定)" },
      L"ATK +24  (가산)" },
    { AugType::VAMPIRE,       AugRarity::EPIC,      AugUnique::NONE, "VAMPIRE",
      { L"흡혈마", L"Vampire", L"吸血鬼" },
      { L"흡혈 한도 0.48 · Max HP +20 · 10킬마다 HP +1",
        L"Lifesteal cap 0.48 · Max HP +20 · +1 HP per 10 kills",
        L"吸血上限0.48 · 最大HP+20 · 10キル毎HP+1" },
      L"흡혈 상한 0.48  /  Max HP +20" },
    { AugType::RESERVED_AUG_003, AugRarity::EPIC, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_001, AugRarity::EPIC, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::MINIATURIZE,   AugRarity::EPIC,      AugUnique::SIZE, "MINI",
      { L"축소화", L"Miniaturize", L"小型化" },
      { L"최대 체력 1/2  /  보유 증강당 공격력 +10·연사 +2%  /  이속 +20%  /  크기 -20%",
        L"Max HP halved  /  per aug: ATK +10·rate +2%  /  move +20%  /  size -20%",
        L"最大体力1/2  /  強化1つ毎 攻撃+10・連射+2%  /  移動+20%  /  サイズ-20%" },
      L"Max HP 1/2  /  증강당 ATK +10" },
    { AugType::GIGANTIFY,     AugRarity::EPIC,      AugUnique::SIZE, "GIGA",
      { L"거대화", L"Gigantify", L"巨大化" },
      { L"최대 체력 ×2  /  초당 체력 약 1.0 회복  /  이속 -40%  /  크기 +50%",
        L"Max HP ×2  /  ~1.0 HP/s regen  /  move -40%  /  size +50%",
        L"最大体力 ×2  /  毎秒 約1.0回復  /  移動 -40%  /  サイズ +50%" },
      L"Max HP ×2  /  재생 ~1.0/s  /  이동 -40%" },
    { AugType::PIERCE,        AugRarity::EPIC,      AugUnique::NONE, "PIERCE",
      { L"관통", L"Pierce", L"貫通" },
      { L"명중 시 30% 확률로 적·장애물 관통",
        L"30% chance to pierce on hit",
        L"命中時30%で敵・障害物を貫通" },
      L"관통 확률 +30%" },
    { AugType::TWIN,          AugRarity::EPIC,      AugUnique::NONE, "TWIN",
      { L"더블", L"Twin", L"ダブル" },
      { L"한 번에 2발 발사  /  공격력 -40%", L"Fire 2 shots at once  /  Attack -40%", L"一度に2発発射  /  攻撃力 -40%" },
      L"발사 ×2  /  ATK -40%" },
    { AugType::CHAKRAM,       AugRarity::EPIC,      AugUnique::NONE, "CHAKRAM",
      { L"차크람", L"Chakram", L"チャクラム" },
      { L"넓게 공전하는 차크람 1개  /  닿은 잡몹·일반 적 즉사, 원거리 적 큰 피해·적탄 막기  /  파괴 시 6초 후 재생성",
        L"1 wide-orbiting chakram  /  instakills large mobs, big dmg to gunners, blocks bullets  /  respawns 6s",
        L"広く公転するチャクラム1個  /  雑魚・自爆兵即死, 遠距離に大ダメージ・敵弾を防ぐ  /  破壊後6秒で再生成" },
      L"차크람 ×1  /  6초 재생성" },
    { AugType::BULLET_RAIN_2, AugRarity::EPIC,      AugUnique::NONE, "BULLETRAIN II",
      { L"탄환 세례 II", L"Bullet Rain II", L"弾幕の雨 II" },
      { L"탄환 세례 쿨다운 15초 → 10초  (요구: 탄환 세례)",
        L"Bullet Rain cooldown 15s → 10s  (req: Bullet Rain)",
        L"弾幕の雨 クールダウン 15秒 → 10秒  (要: 弾幕の雨)" },
      L"세례 쿨 15초 → 10초" },
    { AugType::CHAKRAM_2,     AugRarity::EPIC,      AugUnique::NONE, "CHAKRAM II",
      { L"차크람 II", L"Chakram II", L"チャクラム II" },
      { L"차크람 1개 → 2개  (요구: 차크람)", L"1 → 2 chakrams  (req: Chakram)", L"1個 → 2個  (要: チャクラム)" },
      L"차크람 1 → 2" },
    { AugType::DRONE,         AugRarity::EPIC,      AugUnique::NONE, "DRONE",
      { L"드론", L"Drone", L"ドローン" },
      { L"자동 조준 드론 1기 소환  /  연사·탄속 50%",
        L"Summon 1 auto-aim drone  /  50% fire rate·speed",
        L"自動照準ドローン1機  /  連射・弾速50%" },
      L"드론 ×1  /  화력 50%" },
    { AugType::LASER,         AugRarity::EPIC,      AugUnique::NONE, "LASER",
      { L"스캔 레이저", L"Scan Laser", L"スキャンレーザー" },
      { L"0.85초마다 조준 방향으로 중거리 관통 레이저 — 직선상 적 일소 (군중 제어)",
        L"Every 0.85s, a mid-range piercing laser along your aim — clears enemies in a line",
        L"0.85秒毎に照準方向へ中距離貫通レーザー — 直線上の敵を一掃 (群衆制御)" },
      L"0.85초 관통 빔  /  사거리 560" },
    { AugType::RESERVED_AUG_050, AugRarity::EPIC, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::LASER_2,       AugRarity::LEGENDARY, AugUnique::NONE, "LASER II",
      { L"스캔 레이저 II", L"Scan Laser II", L"スキャンレーザー II" },
      { L"레이저 발사 0.85→0.55초 · 사거리 560→760  (요구: 스캔 레이저)",
        L"Laser interval 0.85→0.55s · range 560→760  (req: Scan Laser)",
        L"レーザー 0.85→0.55秒・射程 560→760  (要: スキャンレーザー)" },
      L"간격 0.85→0.55초  /  사거리 +200" },
    { AugType::PIERCE_2,      AugRarity::LEGENDARY, AugUnique::NONE, "PIERCE II",
      { L"관통 II", L"Pierce II", L"貫通 II" },
      { L"관통 확률 +30%  (요구: 관통)", L"Pierce chance +30%  (req: Pierce)", L"貫通確率 +30%  (要: 貫通)" },
      L"관통 +30%p" },
    { AugType::TWIN_2,        AugRarity::LEGENDARY, AugUnique::NONE, "TRIPLE",
      { L"트리플 샷", L"Triple Shot", L"トリプルショット" },
      { L"한 번에 2발 → 3발 · 공격력 -12%  (요구: 더블)", L"2 → 3 shots at once · Attack -12%  (req: Twin)", L"一度に2発 → 3発 · 攻撃-12%  (要: ダブル)" },
      L"발사 2→3  /  ATK -12%" },
    { AugType::RESERVED_AUG_042, AugRarity::EPIC, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_002, AugRarity::EPIC, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_049, AugRarity::EPIC, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_046, AugRarity::EPIC, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_043, AugRarity::EPIC, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::HACK_RANGED,   AugRarity::EPIC,      AugUnique::NONE, "HACK_RANGED",
      { L"해킹: 원거리", L"Hack: Ranged", L"ハック: 遠距離" },
      { L"원거리 몹 처치 시 20% 확률로 유도탄 5발  (적에게만 피해)",
        L"On ranged-mob kill, 20% chance: 5 homing shots  (enemy-only dmg)",
        L"遠距離敵撃破時20%で誘導弾5発  (敵のみ)" },
      L"원거리 처치 20%  /  유도탄 ×5" },
    { AugType::PROB_CHAIN,    AugRarity::EPIC,      AugUnique::NONE, "PROBCHAIN",
      { L"확률적 연쇄 작용", L"Chance Ricochet", L"確率的連鎖" },
      { L"명중 시 30% 확률로 가까운 적에게 튕김  (최대 3회 · 뮤탈 글레이브)",
        L"On hit, 30% chance to ricochet to nearest enemy  (up to 3x)",
        L"命中時30%で近い敵に跳弾  (最大3回)" },
      L"명중 30%  /  3회 튕김" },
    { AugType::DEATH_BLAST,   AugRarity::LEGENDARY, AugUnique::NONE, "DEATHBLAST",
      { L"연쇄 폭발", L"Death Blast", L"連鎖爆発" },
      { L"적 처치 시 폭발 — 주변 적에게 공격력 30% 피해 (폭발로 죽은 적은 추가 폭발 X)",
        L"On kill, explode — 30% ATK dmg nearby (blast-killed don't re-explode)",
        L"撃破時に爆発 — 周囲に攻撃力30%ダメージ (爆死は再爆発しない)" },
      L"처치 시 ATK 30% 폭발" },
    { AugType::SKILL_CLOSE,   AugRarity::EPIC,      AugUnique::NONE, "SKILL_CLOSE",
      { L"[스킬] 창 닫기", L"[Skill] Close Window", L"[スキル] ウィンドウを閉じる" },
      { L"액티브 스킬 획득 — 플레이어 중심 대폭발(넉백+피해)  (쿨 16초)",
        L"Active skill — explosion at the player (knockback+dmg)  (16s CD)",
        L"アクティブスキル — 自分中心の大爆発(ノックバック+ダメージ)  (CD16秒)" },
      L"쿨 16초  /  폭발 넉백" },
    { AugType::SKILL_OVERCLOCK,AugRarity::EPIC,     AugUnique::NONE, "SKILL_OVCLK",
      { L"[스킬] 초집중", L"[Skill] Hyper Focus", L"[スキル] 超集中" },
      { L"액티브 — 5초간 시야 +50% · 적 행동 -30% · 대시 쿨 2초  (쿨 20초)",
        L"Active — 5s: vision +50%, enemies -30% speed, dash CD 2s  (20s CD)",
        L"アクティブ — 5秒: 視界+50%・敵-30%・ダッシュCD2秒  (CD20秒)" },
      L"쿨 20초  /  5초 집중 모드" },

    // ── 전설 ───────────────────────────────────────────
    { AugType::POWER_SURGE,   AugRarity::LEGENDARY, AugUnique::NONE, "POWER_SURGE",
      { L"전력 증폭", L"Power Surge", L"パワーサージ" },
      { L"공격력 ×1.05  (3스택까지 · 이후 ×1.03 · 중첩)",
        L"Attack ×1.05  (stacks to 3 · then ×1.03)",
        L"攻撃力 ×1.05  (3まで · 以降 ×1.03)" },
      L"ATK ×1.05  (3중첩)" },
    { AugType::RANDOM_AUG,    AugRarity::LEGENDARY, AugUnique::NONE, "RANDOM",
      { L"랜덤 증강", L"Random Augment", L"ランダム強化" },
      { L"등급 무관 랜덤 버프 3개 즉시 획득  (디버프 없음)",
        L"Instantly gain 3 random buffs (any rarity, no debuff)",
        L"等級無関係のバフ3個を即獲得  (デバフ無し)" },
      L"랜덤 버프 ×3 즉시 획득" },
    { AugType::RESERVED_AUG_059, AugRarity::LEGENDARY, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::MK2,           AugRarity::LEGENDARY, AugUnique::NONE, "MK2",
      { L"MK2 내장", L"MK2 Core", L"MK2 内蔵" },
      { L"사망 시 공격력 비례 대폭발 + 풀 HP 부활  (1회, 페널티 없음)",
        L"On death: ATK-scaled blast + full-HP revive  (once, no penalty)",
        L"死亡時 攻撃力比例の大爆発+全回復で復活  (1回・ペナルティ無)" },
      L"사망 시 1회 부활  /  대폭발" },
    { AugType::RESERVED_AUG_038, AugRarity::LEGENDARY, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::CHAIN,         AugRarity::LEGENDARY, AugUnique::NONE, "CHAIN",
      { L"연쇄 작용", L"Chain Reaction", L"連鎖反応" },
      { L"모든 총알이 무조건 2회 튕김  /  총알 공격력 -30%",
        L"All bullets always ricochet 2x  /  bullet damage -30%",
        L"全弾が必ず2回跳弾  /  弾ダメージ -30%" },
      L"전탄 2회 튕김  /  ATK -30%" },
    { AugType::SKILL_TIMESTOP,AugRarity::LEGENDARY, AugUnique::NONE, "SKILL_TSTOP",
      { L"[스킬] 시간 정지", L"[Skill] Time Stop", L"[スキル] 時間停止" },
      { L"액티브 스킬 획득 — 1.5초간 적·적탄막 정지 (나는 계속 행동)  (쿨 28초)",
        L"Active skill — freeze enemies & enemy bullets 1.5s  (28s CD)",
        L"アクティブスキル — 1.5秒 敵と敵弾を停止  (CD28秒)" },
      L"쿨 28초  /  1.5초 시간 정지" },
    { AugType::BULLET_RAIN_3, AugRarity::LEGENDARY, AugUnique::NONE, "BULLETRAIN III",
      { L"탄환 세례 III", L"Bullet Rain III", L"弾幕の雨 III" },
      { L"탄환 세례 쿨다운 10초 → 5초  (요구: 탄환 세례 II)",
        L"Bullet Rain cooldown 10s → 5s  (req: Bullet Rain II)",
        L"弾幕の雨 クールダウン 10秒 → 5秒  (要: 弾幕の雨 II)" },
      L"세례 쿨 10초 → 5초" },
    { AugType::DRONE_2,       AugRarity::LEGENDARY, AugUnique::NONE, "DRONE II",
      { L"드론 II", L"Drone II", L"ドローン II" },
      { L"드론 1기 → 2기  (요구: 드론)",
        L"1 → 2 drones  (req: Drone)",
        L"1機 → 2機  (要: ドローン)" },
      L"드론 1 → 2" },
    { AugType::CHAKRAM_3,     AugRarity::LEGENDARY, AugUnique::NONE, "CHAKRAM III",
      { L"차크람 III", L"Chakram III", L"チャクラム III" },
      { L"차크람 2개 → 3개  (요구: 차크람 II)", L"2 → 3 chakrams  (req: Chakram II)", L"2個 → 3個  (要: チャクラム II)" },
      L"차크람 2 → 3" },

    // ── 디버프 ─────────────────────────────────────────
    { AugType::D_RMOB_MAX,    AugRarity::DEBUFF,    AugUnique::NONE, "D_RMOBMAX",
      { L"원거리 몹 증원", L"Ranged Reinforce", L"遠距離増援" },
      { L"원거리 몹 최대 수 +1 · 스폰 0.5초 빨라짐 · 원거리 처치 EXP +12",
        L"Ranged max +1 · spawn 0.5s faster · ranged-kill EXP +12",
        L"遠距離敵の最大数+1・出現0.5秒短縮・撃破EXP +12" },
      L"원거리몹 최대 +1  /  EXP +12" },
    { AugType::D_RMOB_HP,     AugRarity::DEBUFF,    AugUnique::NONE, "D_RMOBPOW",
      { L"원거리 몹 강화", L"Ranged Empower", L"遠距離強化" },
      { L"원거리 몹 체력·공격력 +20% · 원거리 처치 EXP +6",
        L"Ranged HP·ATK +20% · ranged-kill EXP +6",
        L"遠距離敵の体力・攻撃+20%・撃破EXP +6" },
      L"원거리몹 HP·ATK +20%  /  EXP +6" },
    { AugType::D_RMOB_DELAY,  AugRarity::DEBUFF,    AugUnique::NONE, "D_RMOBDELAY",
      { L"원거리 몹 가속", L"Ranged Haste", L"遠距離加速" },
      { L"원거리 몹 이동 속도 +20% · 원거리 처치 EXP +5",
        L"Ranged move speed +20% · ranged-kill EXP +5",
        L"遠距離敵の移動+20%・撃破EXP +5" },
      L"원거리몹 이동 +20%  /  EXP +5" },
    { AugType::D_MOB_SPAWN,   AugRarity::DEBUFF,    AugUnique::NONE, "D_MOBSPAWN",
      { L"로터 폭주", L"Rotor Surge", L"ローター暴走" },
      { L"로터 생성 빈도 증가 · 동시 한도 +12 · 처치 EXP +1",
        L"Rotor spawn frequency increased · cap +12 · kill EXP +1",
        L"ローター出現頻度増加・同時上限+12・撃破EXP +1" },
      L"생성 빈도 ↑  /  EXP +1" },
    { AugType::D_APPROACH,    AugRarity::DEBUFF,    AugUnique::NONE, "D_APPROACH",
      { L"다가오는 죽음", L"Approaching Death", L"Approaching Death" },
      { L"무적 붉은 오브가 추적 - 초당 EXP +0.5 - 최대 3회, 중복 시 속도 +20%",
        L"Invincible red orb chases you - EXP +0.5/s - max 3, +20% orb speed per stack",
        L"Invincible red orb chases you - EXP +0.5/s - max 3" },
      L"무적 오브 추적  /  EXP +0.5/s" },
    { AugType::D_MOB_SPEED,   AugRarity::DEBUFF,    AugUnique::NONE, "D_MOBSPD",
      { L"로터 가속", L"Rotor Haste", L"ローター加速" },
      { L"로터 이동 속도 +10% · 초당 EXP +0.5",
        L"Rotor move speed +10% · EXP +0.5/s",
        L"ローター移動速度+10%・毎秒EXP +0.5" },
      L"로터 이동 +10%  /  EXP +0.5/s" },
    { AugType::D_GLASS_HEART, AugRarity::DEBUFF,    AugUnique::NONE, "D_GLASS",
      { L"유리 심장", L"Glass Heart", L"Glass Heart" },
      { L"최대 체력 -20% - 전체 EXP +3%", L"Max HP -20% - all EXP +3%", L"Max HP -20% - all EXP +3%" },
      L"Max HP -20%  /  전체EXP +3%" },
    { AugType::D_BULLET_STUCK,AugRarity::DEBUFF,    AugUnique::NONE, "D_STUCK",
      { L"탄 걸림", L"Jammed", L"Jammed" },
      { L"연사 속도 -10% - 전체 EXP +5%", L"Fire rate -10% - all EXP +5%", L"Fire rate -10% - all EXP +5%" },
      L"연사 -10%  /  전체EXP +5%" },
    { AugType::RESERVED_AUG_023, AugRarity::DEBUFF, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_017, AugRarity::DEBUFF, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_018, AugRarity::DEBUFF, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_019, AugRarity::DEBUFF, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::D_MOB_HP,      AugRarity::DEBUFF,    AugUnique::NONE, "D_MOBHP",
      { L"일반 적 체력 강화", L"Normal Enemy HP Up", L"通常敵体力強化" },
      { L"일반 적 체력 +30% · 일반 적 처치 EXP +2",
        L"All normal enemy HP +30% · normal kill EXP +2",
        L"通常敵体力+30%・通常敵撃破EXP +2" },
      L"일반 적 HP +30%  /  EXP +2" },
    { AugType::D_SLOW_MOVE,   AugRarity::DEBUFF,    AugUnique::NONE, "D_SLOWMV",
      { L"무거운 다리", L"Heavy Legs", L"Heavy Legs" },
      { L"플레이어 이동 속도 -5% - 전체 EXP +3%", L"Player move speed -5% - all EXP +3%", L"Player move speed -5% - all EXP +3%" },
      L"이동 -5%  /  전체EXP +3%" },
    { AugType::RESERVED_AUG_031, AugRarity::DEBUFF, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_016, AugRarity::DEBUFF, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_026, AugRarity::DEBUFF, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_030, AugRarity::DEBUFF, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_029, AugRarity::DEBUFF, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::D_BLEED,       AugRarity::DEBUFF,    AugUnique::NONE, "D_BLEED",
      { L"출혈", L"Bleed", L"Bleed" },
      { L"체력 회복 -1.0/s - 회복이 0이면 등장하지 않음 - 전체 EXP +12%",
        L"Regen -1.0/s - hidden at 0 regen - all EXP +12%",
        L"Regen -1.0/s - hidden at 0 regen - all EXP +12%" },
      L"재생 -1.0/s  /  전체EXP +12%" },
    { AugType::D_WEAKEN,      AugRarity::DEBUFF,    AugUnique::NONE, "D_WEAKEN",
      { L"약화", L"Weaken", L"弱体化" },
      { L"공격력 -12% · 전체 EXP +10%", L"Attack -12% · all EXP +10%", L"攻撃力 -12%・全EXP +10%" },
      L"ATK -12%  /  전체EXP +10%" },
    // ── 로터류(잡몹) 전용 디버프 (확장, 중첩 가능) ──
    { AugType::D_MOB_PACK,    AugRarity::DEBUFF,    AugUnique::NONE, "D_MOBPACK",
      { L"병렬 처리", L"Parallel Spawn", L"並列処理" },
      { L"로터가 나올 때마다 추가 개체 1마리 등장 · 처치 EXP +6",
        L"Each Rotor spawn brings 1 extra unit · kill EXP +6",
        L"ローター出現時に追加1体・撃破EXP +6" },
      L"동시 +1  /  EXP +6" },
    { AugType::RESERVED_AUG_024, AugRarity::DEBUFF, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_025, AugRarity::DEBUFF, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::S_CHAOS,       AugRarity::SPECIAL,   AugUnique::NONE, "CHAOS",
      { L"대혼란", L"Chaos", L"大混乱" },
      { L"보유 증강을 모두 잊고 같은 개수만큼 무작위 재배열  (약 60% 버프 / 40% 디버프)",
        L"Forget all augments and randomly recompose the same count  (~60% buffs / 40% debuffs)",
        L"所持強化を全て忘れ、同数をランダムに再構成  (約60%バフ / 40%デバフ)" },
      L"—  (전체 재배분)" },
    { AugType::S_PANDORA,     AugRarity::SPECIAL,   AugUnique::NONE, "PANDORA",
      { L"판도라의 상자", L"Pandora's Box", L"パンドラの箱" },
      { L"버프 3개 + 디버프 2개 즉시 획득", L"Instantly gain 3 buffs + 2 debuffs", L"バフ3個 + デバフ2個を即獲得" },
      L"버프 +3  /  디버프 +2" },

    // ── 조합 (COMBO) — 레시피 충족 시에만 카드로 등장 ───────
    { AugType::RESERVED_AUG_005, AugRarity::COMBO, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::CB_BLOODLORD,   AugRarity::COMBO,    AugUnique::NONE, "CB_BLOOD",
      { L"피의 군주", L"Bloodlord", L"血の君主" },
      { L"[조합] 최대 체력 +15 · 재생 +0.25/s",
        L"[Combo] Max HP +15 · regen +0.25/s",
        L"[組合] 最大HP+15・再生+0.25/s" },
      L"Max HP +15  /  재생 +0.25/s" },
    { AugType::RESERVED_AUG_009, AugRarity::COMBO, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_011, AugRarity::COMBO, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_010, AugRarity::COMBO, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_006, AugRarity::COMBO, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::CB_WARLORD,     AugRarity::COMBO,    AugUnique::NONE, "CB_WAR",
      { L"전쟁군주", L"Warlord", L"戦争君主" },
      { L"[조합] 공격력 +15%",
        L"[Combo] ATK +15%",
        L"[組合] 攻撃+15%" },
      L"ATK +15%" },
    { AugType::RESERVED_AUG_013, AugRarity::COMBO, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_008, AugRarity::COMBO, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_007, AugRarity::COMBO, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::BULLET_RAIN_ETERNAL, AugRarity::MYTHIC, AugUnique::NONE, "RAIN_ETERNAL",
      { L"무한 세례", L"Endless Rain", L"無限の雨" },
      { L"쿨다운 4초 · 적 처치마다 쿨다운 0.2초 감소 (최소 3초)",
        L"Cooldown 4s · each kill cuts cooldown by 0.2s (min 3s)",
        L"クールダウン4秒 · 撃破毎にCD0.2秒短縮 (最短3秒)" },
      L"쿨 4초  /  처치마다 -0.2초" },
    { AugType::DRONE_HIVE,    AugRarity::MYTHIC, AugUnique::NONE, "DRONE_HIVE",
      { L"군집 지능", L"Hive Mind", L"群知能" },
      { L"드론 4기 운용 · 각 드론 화력은 50% 유지",
        L"Run 4 drones · each drone keeps 50% output",
        L"ドローン4機運用 · 各ドローン火力50%維持" },
      L"드론 ×4  /  각 화력 50%" },
    { AugType::LASER_CONVERGE, AugRarity::MYTHIC, AugUnique::NONE, "LASER_CONV",
      { L"수렴", L"Convergence", L"収束" },
      { L"스캔 레이저가 거의 연속으로 발사 + 광폭 빔 (직선 일소)",
        L"Scan laser fires almost continuously + extra-wide beam",
        L"スキャンレーザーがほぼ連続発射 + 極太ビーム" },
      L"레이저 거의 연속  /  광폭 빔" },
    { AugType::PIERCE_RAILSLUG, AugRarity::MYTHIC, AugUnique::NONE, "PIERCE_RAIL",
      { L"철갑탄", L"Railslug", L"徹甲弾" },
      { L"관통 90% 고정 · 공격력 +25 (가산) · 탄속 +50% (멈추지 않는 탄)",
        L"90% pierce · attack +25 (flat) · bullet speed +50%",
        L"貫通90% · 攻撃+25(加算) · 弾速+50%" },
      L"관통 90%  /  ATK +25  /  탄속 +50%" },
    // Reserved augment metadata.
    { AugType::RESERVED_AUG_014, AugRarity::COMBO, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_028, AugRarity::DEBUFF, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_033, AugRarity::DEBUFF, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_021, AugRarity::DEBUFF, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_015, AugRarity::DEBUFF, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_027, AugRarity::DEBUFF, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_022, AugRarity::DEBUFF, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_034, AugRarity::DEBUFF, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_020, AugRarity::DEBUFF, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::LIFESTEAL_2,   AugRarity::EPIC,     AugUnique::NONE, "LIFESTEAL2",
      { L"흡혈탄 II", L"Lifesteal II", L"吸血弾 II" },
      { L"흡혈 한도 0.24→0.36 · 10킬마다 HP +1  (요구: 흡혈탄)",
        L"Lifesteal cap 0.24→0.36 · +1 HP per 10 kills  (req: Lifesteal)",
        L"吸収上限0.24→0.36 · 10キル毎HP+1  (要:吸収弾)" },
      L"흡혈 상한 0.24→0.36" },
    { AugType::CHAIN_2,       AugRarity::LEGENDARY, AugUnique::NONE, "CHAIN2",
      { L"연쇄 작용 II", L"Chain Reaction II", L"連鎖反応 II" },
      { L"튕김 2→3회 · 총알 -30%→-15%  (요구: 연쇄 작용)",
        L"Ricochet 2→3 · bullet dmg -30%→-15%  (req: Chain)",
        L"跳弾2→3 · 弾-30%→-15%  (要:連鎖反応)" },
      L"튕김 2→3  /  ATK -15%" },
    { AugType::RESERVED_AUG_054, AugRarity::EPIC, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_051, AugRarity::EPIC, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_040, AugRarity::EPIC, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::CHAKRAM_SINGULARITY, AugRarity::MYTHIC, AugUnique::NONE, "CHAK_SING",
      { L"특이점", L"Singularity", L"特異点" },
      { L"차크람이 적을 끌어당김 · 접촉 지속 피해  (요구: 차크람 III)",
        L"Chakrams pull enemies in · contact DOT  (req: Chakram III)",
        L"チャクラムが敵を吸引 · 接触DoT  (要:チャクラムIII)" },
      L"흡인 + DoT" },
    { AugType::RESERVED_AUG_055, AugRarity::EPIC, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::SKILL_DASH_UP, AugRarity::EPIC,    AugUnique::NONE, "DASH_UP",
      { L"[스킬] 섬광 돌진", L"[Skill] Flash Dash", L"[スキル] 閃光突進" },
      { L"대시 시 유도탄 3~5발 · 이후 3발 ×2 공격력  (SHIFT · 대시 강화)",
        L"Dash — 3~5 homing shots · next 3 shots ×2 dmg  (SHIFT upgrade)",
        L"ダッシュ — 誘導弾3~5 · 次3発×2  (SHIFT強化)" },
      L"대시 시 유도탄  /  이후 ×2" },
    { AugType::RESERVED_AUG_056, AugRarity::EPIC, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RIFLE_STABILITY, AugRarity::EPIC,    AugUnique::NONE, "RIFLE_STAB",
      { L"정조준", L"Marksman", L"精密照準" },
      { L"[소총] 흩어짐 제거 · 공격력 +8",
        L"[Rifle] no spread · Attack +8",
        L"[ライフル] 拡散なし · 攻撃+8" },
      L"퍼짐 제거  /  ATK +8" },
    { AugType::RESERVED_AUG_058, AugRarity::EPIC, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::HP_UP,         AugRarity::COMMON,    AugUnique::NONE, "HP_UP",
      { L"체력 증가", L"HP Up", L"体力アップ" },
      { L"최대 체력 +15  (중첩 가능)",
        L"Max HP +15  (stackable)",
        L"最大体力 +15  (重複可)" },
      L"Max HP +15" },
    { AugType::FIREWALL,      AugRarity::RARE,      AugUnique::NONE, "FIREWALL",
      { L"방화벽", L"Firewall", L"ファイアウォール" },
      { L"받는 피해 -12%  (1회 획득)",
        L"Damage taken -12%  (once)",
        L"被ダメ -12%  (1回のみ)" },
      L"받는 피해 -12%" },
    { AugType::REGEN_2,       AugRarity::EPIC,      AugUnique::NONE, "REGEN2",
      { L"재생 II", L"Regen II", L"再生 II" },
      { L"재생 +0.45/s  /  체력 40% 이하 시 재생 ×2  (요구: 재생 증강 권장)",
        L"Regen +0.45/s  /  below 40% HP regen ×2  (works best with Regen Up)",
        L"再生 +0.45/s  /  体力40%以下で再生×2" },
      L"재생 +0.45/s  /  저체력 ×2" },
    { AugType::CB_BASTION,    AugRarity::COMBO,     AugUnique::NONE, "CB_BAST",
      { L"철벽", L"Bastion", L"鉄壁" },
      { L"[조합] 최대 체력 +15% · 재생 +0.35/s · 받는 피해 -8%p",
        L"[Combo] Max HP +15% · regen +0.35/s · damage taken -8%p",
        L"[組合] 最大HP+15% · 再生+0.35/s · 被ダメ-8%p" },
      L"Max HP +15%  /  재생 +0.35  /  피해 -8%" },
    { AugType::CB_LIFEBUOY,   AugRarity::COMBO,     AugUnique::NONE, "CB_LIFE",
      { L"구명줄", L"Lifebuoy", L"救命浮輪" },
      { L"[조합] 재생 +0.25/s · 이동 +12% · 10킬→7킬 회복 · 피격 정지 6초",
        L"[Combo] regen +0.25/s · move +12% · heal every 7 kills · hit lock 6s",
        L"[組合] 再生+0.25/s · 移動+12% · 7キル回復 · 被弾停止6秒" },
      L"재생 +0.25  /  이동 +12%  /  7킬 회복" },
    { AugType::RESERVED_AUG_044, AugRarity::EPIC, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_045, AugRarity::MYTHIC, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::DEATH_BLAST_2, AugRarity::LEGENDARY, AugUnique::NONE, "DBLST2",
      { L"연쇄 폭발 II", L"Death Blast II", L"連鎖爆発 II" },
      { L"폭발 피해 30%→40% · 반경 +40%  (요구: 연쇄 폭발)",
        L"Blast dmg 30%→40% · radius +40%  (req: Death Blast)",
        L"爆発30%→40% · 範囲+40%  (要:連鎖爆発)" },
      L"폭발 30%→40%  /  반경 +40%" },
    { AugType::RESERVED_AUG_012, AugRarity::COMBO, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_039, AugRarity::EPIC, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_052, AugRarity::LEGENDARY, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_041, AugRarity::LEGENDARY, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
    { AugType::RESERVED_AUG_032, AugRarity::DEBUFF, AugUnique::NONE, "REMOVED",
      { L"?? ? ??", L"Removed augment", L"?? ??? ??" },
      { L"?? ?? ?? ?? ? ? ?? ??.",
        L"This augment is retired from the current gameplay roster.",
        L"?? ??? ??? ??? ??? ??? ???" },
      L"REMOVED" },
};

static constexpr int AUG_TOTAL = (int)(sizeof(ALL_AUGS) / sizeof(ALL_AUGS[0]));

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

// 등급 + 카테고리 배지 — "희귀|크기" / "에픽|스킬" 식. 태그 없으면 등급만.
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
