#pragma once
#include <cstdlib>

enum class AugType {
    // ── 일반 (7) ──────────────────────────────
    DMG_UP, RATE_UP, SPD_UP, MOVE_UP, VISION_UP, REGEN_UP,
    // ── 희귀 ──
    GLASS_CANNON, LIGHT_AMMO, LIGHT_STEP, GUN_RUNNER,
    BULLET_RAIN,
    CANNON,                // 전설 → 희귀
    // ── 에픽 ──
    VAMPIRE, BROKEN_SIGHT, SNIPER, BAYONET,
    MINIATURIZE, GIGANTIFY, PIERCE, TWIN, CHAKRAM,
    BULLET_RAIN_2, CHAKRAM_2,
    DRONE,                 // 희귀 → 에픽
    MINIGUN, HACK_RANGED, SHOTGUN,    // 신규
    // ── 전설 ──
    RANDOM_AUG, SOUL_HARVEST,
    BULLET_RAIN_3, DRONE_2, CHAKRAM_3,
    MK2, HACK_BOMBER,                  // 신규
    // ── 디버프 ──
    D_RMOB_MAX, D_RMOB_HP, D_RMOB_DELAY,
    D_MOB_SPAWN, D_APPROACH, D_MOB_SPEED,
    D_GLASS_HEART, D_BULLET_STUCK, D_DRUNK,
    D_BOMBER_BLAST, D_BOMBER_BUFF, D_BOMBER_SPEED,   // 신규
    D_MOB_HP, D_SLOW_MOVE,                            // 신규
    // ── 특수 (2) ──────────────────────────────
    S_CHAOS, S_PANDORA,
    // ── deprecated (ALL_AUGS 에서 제외, 코드는 남음) ──
    SIEGE_TANK, D_RMOB_DMG, XP_UP
};

enum class AugRarity { COMMON, RARE, EPIC, LEGENDARY, DEBUFF, SPECIAL };

// 고유 카테고리 — 같은 카테고리 내에서 1개만 선택 가능
enum class AugUnique { NONE, SIZE, DISTANCE };

struct AugDef {
    AugType        type;
    AugRarity      rarity;
    AugUnique      unique;
    const char*    name;       // 영문 짧은 이름 (윈도우 타이틀)
    const wchar_t* krName;     // 카드 상단 한글
    const wchar_t* krDesc;     // 카드 본문 한글
};

static const AugDef ALL_AUGS[] = {
    // ── 일반 ───────────────────────────────────────────
    { AugType::DMG_UP,        AugRarity::COMMON,    AugUnique::NONE,
      "DMG+4%",      L"공격력 증가",       L"공격력 +4%  (곱연산 누적)" },
    { AugType::RATE_UP,       AugRarity::COMMON,    AugUnique::NONE,
      "RATE+1.5%",   L"연사 속도 증가",    L"연사 속도 +1.5%" },
    { AugType::SPD_UP,        AugRarity::COMMON,    AugUnique::NONE,
      "BSPD+10",     L"탄속 증가",         L"탄환 속도 +10" },
    { AugType::MOVE_UP,       AugRarity::COMMON,    AugUnique::NONE,
      "MOVE+2%",     L"이동속도 증가",     L"이동 속도 +2%" },
    { AugType::VISION_UP,     AugRarity::COMMON,    AugUnique::NONE,
      "VISION+50",   L"시야 증가",         L"시야(창 크기) +50  /  최대 4중첩 (+200)" },
    { AugType::REGEN_UP,      AugRarity::COMMON,    AugUnique::NONE,
      "REGEN",       L"체력 재생",         L"5초마다 체력 1 추가 회복  (중첩 가능)" },
    // XP_UP deprecated — pool에서 제외됨 (enum 에는 유지)

    // ── 희귀 ───────────────────────────────────────────
    { AugType::GLASS_CANNON,  AugRarity::RARE,      AugUnique::NONE,
      "GLASSCANNON", L"유리대포",          L"공격력 +50%  /  최대 체력 -35%" },
    { AugType::LIGHT_AMMO,    AugRarity::RARE,      AugUnique::NONE,
      "LIGHTAMMO",   L"가벼운 탄환",       L"연사 +10%  /  탄속 +30%  /  공격력 -10%" },
    { AugType::LIGHT_STEP,    AugRarity::RARE,      AugUnique::NONE,
      "LIGHTSTEP",   L"가벼운 발걸음",     L"이동 속도 +50%  /  피격 시 10초간 이 효과 정지" },
    { AugType::GUN_RUNNER,    AugRarity::RARE,      AugUnique::NONE,
      "GUNRUNNER",   L"건 앤 러너",        L"사격하지 않는 동안 이동 속도 +80%" },
    { AugType::BULLET_RAIN,   AugRarity::RARE,      AugUnique::NONE,
      "BULLETRAIN",  L"탄환 세례",         L"15초마다 유도탄 20발 일제 발사  /  발당 데미지 50%" },
    // CANNON/SNIPER/SHOTGUN 은 시작 무기로 이동 — 변환 카드로만 획득 (ALL_AUGS 에서 제거)

    // ── 에픽 ───────────────────────────────────────────
    { AugType::VAMPIRE,       AugRarity::EPIC,      AugUnique::NONE,
      "VAMPIRE",     L"흡혈마",            L"10킬마다 체력 +1  /  최대 체력 -10%  /  획득 즉시 현재 체력 -20%" },
    { AugType::BROKEN_SIGHT,  AugRarity::EPIC,      AugUnique::NONE,
      "BROKENSIGHT", L"고장난 조준선",     L"마우스 무시, 황금 오브 방향으로 자동 발사  /  공격력 +250%" },
    { AugType::BAYONET,       AugRarity::EPIC,      AugUnique::DISTANCE,
      "BAYONET",     L"총검",              L"200px 이내의 적에게 피해 +50%  (고유 · 거리)" },
    { AugType::MINIATURIZE,   AugRarity::EPIC,      AugUnique::SIZE,
      "MINI",        L"축소화",            L"최대 체력 10 고정  /  보유 증강당 공격력 +10·연사 +2%  /  이속 +20%  /  크기 -20%  (고유 · 크기)  [원거리 한 방에 사망 주의]" },
    { AugType::GIGANTIFY,     AugRarity::EPIC,      AugUnique::SIZE,
      "GIGA",        L"거대화",            L"최대 체력 ×2  /  초당 체력 약 1.3 회복  /  이속 -40%  /  크기 +50%  (고유 · 크기)" },
    { AugType::PIERCE,        AugRarity::EPIC,      AugUnique::NONE,
      "PIERCE",      L"관통",              L"명중 시 30% 확률로 적·장애물 관통" },
    { AugType::TWIN,          AugRarity::EPIC,      AugUnique::NONE,
      "TWIN",        L"더블",              L"한 번에 2발 발사  /  공격력 -40%" },
    { AugType::CHAKRAM,       AugRarity::EPIC,      AugUnique::NONE,
      "CHAKRAM",     L"차크람",            L"주위를 도는 차크람 1개  /  닿은 잡몹 즉사  /  파괴 시 10초 후 재생성" },
    { AugType::BULLET_RAIN_2, AugRarity::EPIC,      AugUnique::NONE,
      "BULLETRAIN II", L"탄환 세례 II",    L"탄환 세례 쿨다운 15초 → 10초  (요구: 탄환 세례)" },
    { AugType::CHAKRAM_2,     AugRarity::EPIC,      AugUnique::NONE,
      "CHAKRAM II",  L"차크람 II",         L"차크람 1개 → 2개  (요구: 차크람)" },
    { AugType::DRONE,         AugRarity::EPIC,      AugUnique::NONE,
      "DRONE",       L"드론",              L"자동 조준 드론 1기 소환  /  연사·탄속 50%" },
    { AugType::MINIGUN,       AugRarity::EPIC,      AugUnique::NONE,
      "MINIGUN",     L"미니건",            L"연사 ×2  /  공격력 -30%  /  20% 확률 관통" },
    { AugType::HACK_RANGED,   AugRarity::EPIC,      AugUnique::NONE,
      "HACK_RANGED", L"해킹: 원거리",      L"원거리 몹 처치 시 20% 확률로 유도탄 5발  (적에게만 피해)" },

    // ── 전설 ───────────────────────────────────────────
    { AugType::RANDOM_AUG,    AugRarity::LEGENDARY, AugUnique::NONE,
      "RANDOM",      L"랜덤 증강",         L"등급 무관 랜덤 버프 3개 즉시 획득  (디버프 없음)" },
    { AugType::SOUL_HARVEST,  AugRarity::LEGENDARY, AugUnique::NONE,
      "SOULHARVEST", L"영혼 수확",         L"100킬마다 공격력 +5% · 연사 +2% · 탄속 +2%  (영구 누적)" },
    { AugType::MK2,           AugRarity::LEGENDARY, AugUnique::NONE,
      "MK2",         L"MK2 내장",          L"사망 시 공격력 비례 대폭발 + 풀 HP 부활  (1회, 페널티 없음)" },
    { AugType::HACK_BOMBER,   AugRarity::LEGENDARY, AugUnique::NONE,
      "HACK_BMBR",   L"해킹: 자폭병",      L"자폭병 처치 시 20% 확률로 폭발  (적에게만 피해)" },
    { AugType::BULLET_RAIN_3, AugRarity::LEGENDARY, AugUnique::NONE,
      "BULLETRAIN III", L"탄환 세례 III",  L"탄환 세례 쿨다운 10초 → 5초  (요구: 탄환 세례 II)" },
    { AugType::DRONE_2,       AugRarity::LEGENDARY, AugUnique::NONE,
      "DRONE II",    L"드론 II",           L"드론 1기 → 2기  (요구: 드론)  /  [조합] 대포 보유 시 포탑 배치로 전환" },
    { AugType::CHAKRAM_3,     AugRarity::LEGENDARY, AugUnique::NONE,
      "CHAKRAM III", L"차크람 III",        L"차크람 2개 → 3개  (요구: 차크람 II)" },

    // ── 디버프 ─────────────────────────────────────────
    { AugType::D_RMOB_MAX,    AugRarity::DEBUFF,    AugUnique::NONE,
      "D_RMOBMAX",   L"원거리 몹 증원",    L"원거리 몹 최대 수 +1 · 스폰 0.5초 빨라짐 · 원거리 처치 EXP +12" },
    { AugType::D_RMOB_HP,     AugRarity::DEBUFF,    AugUnique::NONE,
      "D_RMOBPOW",   L"원거리 몹 강화",    L"원거리 몹 체력·공격력 +20% · 원거리 처치 EXP +12" },
    { AugType::D_RMOB_DELAY,  AugRarity::DEBUFF,    AugUnique::NONE,
      "D_RMOBDELAY", L"원거리 몹 가속",    L"원거리 몹 이동 속도 +20% · 원거리 처치 EXP +5" },
    { AugType::D_MOB_SPAWN,   AugRarity::DEBUFF,    AugUnique::NONE,
      "D_MOBSPAWN",  L"잡몹 폭주",         L"잡몹 생성 빈도 증가 · 동시 한도 +200 · 처치 EXP +1" },
    { AugType::D_APPROACH,    AugRarity::DEBUFF,    AugUnique::NONE,
      "D_APPROACH",  L"다가오는 죽음",     L"무적 빨간 오브가 영원히 추격 · 초당 EXP +0.5  (중복 시 오브 속도 +20%)" },
    { AugType::D_MOB_SPEED,   AugRarity::DEBUFF,    AugUnique::NONE,
      "D_MOBSPD",    L"잡몹 가속",         L"잡몹 이동 속도 +10% · 초당 EXP +0.5" },
    { AugType::D_GLASS_HEART, AugRarity::DEBUFF,    AugUnique::NONE,
      "D_GLASS",     L"유리 심장",         L"최대 체력 -20% · 전체 EXP +5%" },
    { AugType::D_BULLET_STUCK,AugRarity::DEBUFF,    AugUnique::NONE,
      "D_STUCK",     L"총알 걸림",         L"연사 속도 -20% · 전체 EXP +5%" },
    { AugType::D_DRUNK,       AugRarity::DEBUFF,    AugUnique::NONE,
      "D_DRUNK",     L"취함",              L"20초마다 5초간 조준이 랜덤 방향 · 전체 EXP +10%  (중복: 지속 +1초·쿨 -2초)" },
    { AugType::D_BOMBER_BLAST,AugRarity::DEBUFF,    AugUnique::NONE,
      "D_BMB_BLAST", L"자폭병 폭발 확장",  L"자폭병 폭발 반경 ×1.5 · 자폭병 처치 EXP +12" },
    { AugType::D_BOMBER_BUFF, AugRarity::DEBUFF,    AugUnique::NONE,
      "D_BMB_BUFF",  L"자폭병 강화",       L"자폭병 체력 ×1.5 · 자폭병 처치 EXP +10" },
    { AugType::D_BOMBER_SPEED,AugRarity::DEBUFF,    AugUnique::NONE,
      "D_BMB_SPD",   L"자폭병 가속",       L"자폭병 이동 속도 +30% · 자폭병 처치 EXP +5" },
    { AugType::D_MOB_HP,      AugRarity::DEBUFF,    AugUnique::NONE,
      "D_MOBHP",     L"잡몹 체력 강화",    L"잡몹 체력 +20% · 처치 EXP +1" },
    { AugType::D_SLOW_MOVE,   AugRarity::DEBUFF,    AugUnique::NONE,
      "D_SLOWMV",    L"이동속도 너프",     L"플레이어 이동 속도 -5% · 전체 EXP +10%" },

    // ── 특수 ───────────────────────────────────────────
    { AugType::S_CHAOS,       AugRarity::SPECIAL,   AugUnique::NONE,
      "CHAOS",       L"대혼란",            L"보유 증강을 모두 잊고 같은 개수만큼 랜덤 재배분  (약 60% 버프 / 40% 디버프)" },
    { AugType::S_PANDORA,     AugRarity::SPECIAL,   AugUnique::NONE,
      "PANDORA",     L"판도라의 상자",     L"버프 3개 + 디버프 2개 즉시 획득" },
};

static constexpr int AUG_TOTAL = 47;  // XP_UP·CANNON·SNIPER·SHOTGUN 제거

// 등급별 카드 색상 (배경 RGB)
inline void GetRarityColor(AugRarity r, float& cr, float& cg, float& cb) {
    switch (r) {
    case AugRarity::COMMON:    cr = 0.15f; cg = 0.65f; cb = 0.20f; break; // 녹
    case AugRarity::RARE:      cr = 0.10f; cg = 0.30f; cb = 0.85f; break; // 청
    case AugRarity::EPIC:      cr = 0.55f; cg = 0.10f; cb = 0.80f; break; // 보라
    case AugRarity::LEGENDARY: cr = 0.95f; cg = 0.70f; cb = 0.10f; break; // 금
    case AugRarity::DEBUFF:    cr = 0.55f; cg = 0.10f; cb = 0.10f; break; // 어두운 빨강
    case AugRarity::SPECIAL:   cr = 0.85f; cg = 0.10f; cb = 0.55f; break; // 마젠타
    }
}

inline const wchar_t* GetRarityKR(AugRarity r) {
    switch (r) {
    case AugRarity::COMMON:    return L"일반";
    case AugRarity::RARE:      return L"희귀";
    case AugRarity::EPIC:      return L"에픽";
    case AugRarity::LEGENDARY: return L"전설";
    case AugRarity::DEBUFF:    return L"디버프";
    case AugRarity::SPECIAL:   return L"특수";
    }
    return L"?";
}
