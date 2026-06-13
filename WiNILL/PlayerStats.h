#pragma once
#include <algorithm>
#include <cmath>
#include "Augment.h"

struct PlayerStats {
    // ── 기본 스탯 ───────────────────────────────────────
    float damageMultiplier = 1.0f;
    float baseDamage       = 50.0f;
    float maxHP            = 100.0f;
    float bulletSpeed      = 1200.0f;
    float fireInterval     = 0.20f;   // 기본 연사력 하향 (0.15 → 0.20, 더 느리게)
    float windowSize       = 800.0f;
    float moveSpeedMult    = 1.0f;
    float regenPerSec      = 1.0f / 3.0f;  // 기본 3초당 HP 1 회복
    float playerSizeMult   = 1.0f;
    float xpMult           = 1.0f;   // 전체 EXP 곱연산 (XP_UP, 유리심장, 총알걸림, 취함)
    float bulletSpread     = 0.0f;   // 발사 시 각도 흔들기 (라디안). 0 = 정확
    int   pierceChance     = 30;     // PIERCE 활성 시 관통 확률 (%). MINIGUN 등이 덮어씀
    int   meleeXpBonus     = 0;      // 잡몹 처치 추가 EXP (잡몹 폭주)
    int   rangedXpBonus    = 0;      // 원거리 처치 추가 EXP (원거리 디버프들)
    float xpPerSec         = 0.0f;   // 초당 누적 EXP (다가오는 죽음, 잡몹 가속)
    float rmobSpawnDelayBonus = 0.0f;// 원거리 몹 스폰 가속 (초)
    int   mobCapBonus      = 0;      // 잡몹 동시 존재 한도 추가

    // ── 카운터/락 ───────────────────────────────────────
    int  visionStacks   = 0;          // 최대 5 (총 +350)
    int  totalAugs      = 0;
    bool sizeAugTaken   = false;
    bool distAugTaken   = false;

    // ── 희귀 ───────────────────────────────────────────
    bool  lightStep            = false;
    float lightStepDisableTimer = 0.0f;  // 피격 후 카운트다운 (s)
    bool  gunRunner            = false;

    // ── 에픽 ───────────────────────────────────────────
    bool  vampire     = false;   // 10킬당 HP +1
    int   vampireKillStreak = 0; // 10에 도달하면 회복 + 0 리셋
    bool  brokenSight = false;
    bool  sniper      = false;
    bool  bayonet     = false;
    bool  siegeTank   = false;
    int   siegeStacks = 0;       // 0~5 (1중첩당 25%)
    bool  miniaturize = false;
    bool  gigantify   = false;
    bool  pierce      = false;   // 매 hit 30% 확률 관통
    bool  twin        = false;   // 한 번에 2발 발사
    int   twinCount   = 2;       // TWIN=2, TWIN_2(트리플)=3
    bool  chakram     = false;   // 주변 도는 차크람 1개
    // 연쇄 작용 (리코셰) — 명중 시 가까운 적으로 튕김
    int   ricochetMax    = 0;        // 튕김 최대 횟수 (0 = 없음)
    int   ricochetChance = 0;        // 튕김 확률 (%) — 100 = 무조건
    float ricochetDmgMult= 1.0f;     // 튕김 가능 총알의 데미지 배율 (연쇄 작용 0.7)
    // 신규 (에픽/전설)
    bool  mk2          = false;  // 사망 시 1회 부활
    bool  mk2Used      = false;
    bool  minigun      = false;  // 연사 ×2, 데미지 -30%, pierce 20%
    bool  hackBomber   = false;  // 자폭병 처치 20% 폭발
    bool  hackRanged   = false;  // 원거리 처치 20% 유도탄 5
    bool  shotgun      = false;  // 5발 산탄 / 사거리 700
    // ── 핵앤슬래쉬 ──
    int   critChance   = 0;       // 치명타 확률 (%) — 25%/스택
    float critMult     = 2.5f;    // 치명타 데미지 배율
    float lifestealPerKill = 0.0f;// 처치당 회복 HP (흡혈탄)
    bool  berserk      = false;   // 체력 낮을수록 공격력 ↑ (최대 +60%)
    bool  deathBlast   = false;   // 적 사망 시 주변 폭발
    float deathBlastMult = 1.0f;  // 연쇄 폭발 반경 배율 (CB_WARLORD)
    // ── 직업 무기 모드 (검객/궁수) ──
    bool  meleeWeapon  = false;   // 검객 — 총알 대신 근접 호 스윙
    bool  bowWeapon    = false;   // 궁수 — 관통 화살 (느리고 강함)
    // 클래스 전용 증강
    bool  meleeWide    = false;   // [검객] 광폭 베기 — 호·사거리 확대
    bool  bladeWind    = false;   // [검객] 칼바람 — 스윙마다 전방 관통탄
    bool  powerDraw    = false;   // [궁수] 강궁 — 차징 빠름·완충 위력↑
    bool  multishot    = false;   // [궁수] 다중 사격 — 완충 3발 부채꼴
    // 클래스 증강 변환 누적치 (검객/궁수에서 무의미한 스탯 증강을 재해석)
    float bowChargeRateMult = 1.0f;   // [궁수] 연사 증강 → 차징 속도 배수
    float bowChargeCapBonus = 0.0f;   // [궁수] 공격력 증강 → 풀차징 위력 한도 가산

    // ── 희귀/전설 (티어드) ───────────────────────────────
    bool  drone        = false;
    int   droneCount   = 0;       // 1 = DRONE (RARE), 2 = DRONE_2 (LEGENDARY)
    bool  laser        = false;   // 스캔 레이저 — 주기적 관통 빔 (군중제어)
    int   laserTier    = 1;       // 1 = LASER, 2 = LASER_2 (간격↓·사거리↑)
    int   purgeNova    = 0;       // 백신 스캔 — 주기적 범위 펄스 (중첩 시 강화)
    bool  bulletRain   = false;
    float bulletRainCooldown = 15.0f; // 15 → 10 (II) → 5 (III)
    int   chakramCount = 0;       // 1, 2, 3 — CHAKRAM / II / III
    bool  cannon       = false;
    bool  turretMode   = false;  // CANNON + DRONE_2 조합: 포탑 배치
    bool  soulHarvest  = false;
    long long killCount = 0;     // 영혼 수확용 (외부에서 +1)

    // ── 무기/부활 ─────────────────────────────────────
    float baseFireInterval   = 0.15f; // 무기 선택 전 기본값 (변환 카드 undo 기준)
    bool  mk2SkipDebuff      = false; // MK2 부활 후 DEBUFF_SELECT 영구 스킵

    // ── 디버프 ─────────────────────────────────────────
    int   rmobMaxBonus  = 0;    // +1 (최대 1)
    float rmobHpMult    = 1.0f;
    float rmobDmgMult   = 1.0f;
    float rmobDelayMult = 1.0f;  // <1.0 = 더 빠름
    float mobSpawnMult  = 1.0f;  // <1.0 = 더 자주
    bool  splitterMobs  = false; // 분열체(죽으면 분열) 등장 (디버프)
    bool  blinkerMobs   = false; // 점멸체(순간이동) 등장 (디버프)
    bool  orbiterMobs   = false; // 공전체(스파이럴 인) 등장 (디버프)
    bool  spawnerMobs   = false; // 소환체(잡몹 소환) 등장 (디버프)
    bool  shieldedMobs  = false; // 보호막체(주기 방패) 등장 (디버프)
    float mobSpeedMult  = 1.0f;
    bool  approachingDeath = false;
    int   approachStacks   = 0;   // D_APPROACH 누적 횟수 (속도 +20%/스택)
    bool  drunk              = false;
    float drunkActiveDuration = 5.0f;  // 활성 지속시간 (s). 중복 픽 시 +1s
    float drunkCooldown       = 20.0f; // 쿨타임 (s). 중복 픽 시 -2s (최소 4s)
    // 자폭병 디버프 (몹 spawn 시 적용)
    float bomberHpMult    = 1.0f;
    float bomberSpeedMult = 1.0f;
    float bomberBlastMult = 1.0f;
    // 잡몹 HP 디버프
    float monsterHpMult   = 1.0f;
    // 프로세스류(잡몹) 출현 디버프 (확장, 중첩 가능)
    int   mobPackBonus    = 0;     // 스폰당 추가 마리 수 (군집)
    float eliteChanceMult = 1.0f;  // 엘리트 변종 출현 확률 배율
    float varietyChanceMult = 1.0f;// 특수 잡몹(돌진/회피/거대) 출현 확률 배율
    // 핵앤슬래쉬 디버프
    float bleedPerSec     = 0.0f;  // 초당 HP 감소 (출혈)

    // ── 런타임 ──────────────────────────────────────────
    float siegeBonus    = 0.0f;  // 시즈탱크 누적 보너스 (외부에서 갱신)

    // 일반(COMMON) 증강 = 가산(flat) — 곱연산 복리 폭주(원펀맨) 방지.
    //   초반엔 baseDamage 대비 큰 비중, 후반엔 큰 base 대비 상대값 자동 감소.
    //   후반 스케일링은 희귀/에픽의 곱연산 증강(damageMultiplier)이 담당.
    float flatDamageBonus = 0.0f;   // DMG_UP 누적 가산 데미지 (일반 증강 = 고정값)

    // ─────────────────────────────────────────────────────
    void Apply(AugType t) {
        ++totalAugs;
        switch (t) {
        // ── 일반 (버프: QA 피드백 — 일반 증강이 너무 약함) ──
        //   ※ 검객/궁수 변환: 무의미한 스탯 증강을 클래스에 맞게 재해석
        case AugType::DMG_UP:
            flatDamageBonus   += 12.0f;                   // 일반 가산 공격력 +12 (초반 강화)
            break;
        case AugType::RATE_UP:
            if (meleeWeapon)      flatDamageBonus   += 5.0f;   // 검객: 연사 무의미 → 가산 공격력
            else if (bowWeapon)   bowChargeRateMult *= 1.07f;  // 궁수: 연사 → 차징 빠름
            else                  fireInterval      /= 1.07f;  // 총기: 연사 +7% (백분율 곱연산)
            break;
        case AugType::SPD_UP:
            if (meleeWeapon)      damageMultiplier *= 1.04f;    // 검객: 탄속 무의미 → 공격력 +4%
            else                  bulletSpeed      += 30.0f;    // 총기/궁수: 탄속 +30
            break;
        case AugType::MOVE_UP:   moveSpeedMult    *= 1.05f; break;  // +2% → +5%
        case AugType::VISION_UP:
            if (visionStacks < 5) {                                 // 4 → 5중첩
                windowSize += 70.0f;                                // +50 → +70 (최대 +350)
                ++visionStacks;
            }
            break;
        case AugType::REGEN_UP:  regenPerSec += 0.34f; break;  // 5초당 1 → 약 3초당 1
        case AugType::XP_UP:     xpMult       *= 1.05f; break;

        // ── 등급별 공격력 (가산) — 초반 강세. 곱연산 폭주 제거 ──
        case AugType::OVERDRIVE:      flatDamageBonus += 18.0f; break;  // 희귀 +18 (버프)
        case AugType::CORE_OVERLOAD:  flatDamageBonus += 30.0f; break;  // 에픽 +30 (버프)
        case AugType::POWER_SURGE:    damageMultiplier *= 1.07f; break; // 전설 +7%(유일 곱연산)

        // ── 희귀 ──
        case AugType::GLASS_CANNON:
            damageMultiplier *= 1.50f;
            maxHP            *= 0.65f;
            break;
        case AugType::LIGHT_AMMO:
            fireInterval     /= 1.10f;   // 연사 +10%
            bulletSpeed      *= 1.30f;
            damageMultiplier *= 0.80f;   // 공격력 -20% (롤백: -10% → -20%)
            break;
        case AugType::LIGHT_STEP:
            lightStep      = true;
            moveSpeedMult *= 1.50f;
            break;
        case AugType::GUN_RUNNER:
            gunRunner = true;
            break;

        // ── 에픽 ──
        case AugType::VAMPIRE:
            vampire = true;
            maxHP  += 25.0f;             // 버프: 패널티(-10%) 제거 → 최대 체력 +25
            lifestealPerKill += 0.15f;   // 버프: 처치당 소량 흡혈도 추가(흡혈탄과 경쟁)
            break;
        case AugType::BROKEN_SIGHT:
            brokenSight       = true;
            damageMultiplier *= 3.50f;   // +250%
            break;
        case AugType::SNIPER:
            sniper       = true;
            distAugTaken = true;
            break;
        case AugType::BAYONET:
            bayonet      = true;
            distAugTaken = true;
            break;
        case AugType::SIEGE_TANK:
            siegeTank   = true;
            siegeBonus  = 0.0f;
            siegeStacks = 0;
            break;
        case AugType::MINIATURIZE:
            miniaturize    = true;
            sizeAugTaken   = true;
            maxHP          = 10.0f;
            regenPerSec   += 0.1f;
            moveSpeedMult *= 1.20f;
            playerSizeMult *= 0.80f;
            break;
        case AugType::GIGANTIFY:
            gigantify      = true;
            sizeAugTaken   = true;
            moveSpeedMult *= 0.60f;
            playerSizeMult *= 1.50f;
            maxHP         *= 2.0f;
            regenPerSec   += 1.0f;
            break;
        case AugType::PIERCE:
            pierce = true;
            break;
        case AugType::TWIN:
            twin = true;
            damageMultiplier *= 0.60f;  // -40%
            break;
        case AugType::CHAKRAM:
            chakram      = true;
            chakramCount = 1;
            break;
        // ── 신규 에픽/전설 ──
        case AugType::MINIGUN:
            minigun           = true;
            fireInterval     /= 2.0f;
            damageMultiplier *= 0.70f;
            // (미니건 관통 제거 — 요청)
            break;
        case AugType::HACK_RANGED: hackRanged = true; break;
        // 확률적 연쇄 작용 — 30% 확률, 최대 3튕김
        case AugType::PROB_CHAIN:
            if (ricochetMax < 3) ricochetMax = 3;
            if (ricochetChance < 30) ricochetChance = 30;
            break;
        // 연쇄 작용 — 무조건(100%) 2튕김, 총알 데미지 -30%
        case AugType::CHAIN:
            if (ricochetMax < 2) ricochetMax = 2;
            ricochetChance = 100;
            ricochetDmgMult *= 0.70f;
            break;
        // 액티브 스킬 — 장착은 main.cpp(EquipSkill)에서 처리, 스탯 변화 없음
        case AugType::SKILL_CLOSE:
        case AugType::SKILL_OVERCLOCK:
        case AugType::SKILL_TIMESTOP:
            break;
        case AugType::SHOTGUN:
            shotgun      = true;
            distAugTaken = true;
            fireInterval = baseFireInterval * 1.5f;  // #109: 이전 무기 공속 무시
            break;
        case AugType::MK2:         mk2        = true; break;
        case AugType::HACK_BOMBER: hackBomber = true; break;

        // ── 핵앤슬래쉬 (희귀) ──
        case AugType::CRIT:
            critChance = std::min(75, critChance + 20);   // 20%/스택, 최대 75% (너프: 25%/100%)
            critMult   = 2.0f;                            // 배율 너프: 2.5 → 2.0
            break;
        case AugType::LIFESTEAL:
            lifestealPerKill += 0.12f;  // 처치당 HP +0.12 (너프: 0.5 → 0.25 → 0.12)
            break;
        case AugType::BERSERK:
            berserk = true;             // 체력 낮을수록 공격력 ↑ (발사 시 반영)
            break;
        // ── 핵앤슬래쉬 (에픽) ──
        case AugType::DEATH_BLAST:
            deathBlast = true;          // 적 사망 시 주변 폭발
            break;

        // ── 조합 (COMBO) — 레시피 충족 시에만 등장 ──
        case AugType::CB_EXECUTIONER:   // 치명타 + 광전사
            critChance = std::min(90, critChance + 30);   // 너프: 35/100 → 30/90
            critMult  += 1.2f;                            // 너프: +1.5 → +1.2
            damageMultiplier *= 1.20f;
            break;
        case AugType::CB_BLOODLORD:     // 흡혈탄 + 흡혈마 (너프)
            lifestealPerKill += 0.30f;
            maxHP            += 25.0f;
            regenPerSec      += 0.3f;
            break;
        case AugType::CB_PIERCE_TWIN:   // 더블 + 관통 (너프: 100%→60%)
            pierce       = true;
            if (pierceChance < 60) pierceChance = 60;
            damageMultiplier *= 1.40f;
            break;
        case AugType::CB_STORMCALLER:   // 탄환세례 + 드론
            bulletRain         = true;
            bulletRainCooldown = 4.0f;
            drone              = true;
            if (droneCount < 3) ++droneCount;
            fireInterval      /= 1.15f;
            break;
        case AugType::CB_RAILGUN:       // 저격 + 관통 → 레일건
            pierce       = true;
            pierceChance = 100;
            damageMultiplier *= 1.35f;
            bulletSpeed  *= 1.40f;
            break;
        case AugType::CB_GLASS_REAPER:  // 유리대포 + 흡혈탄 → 유리 사신
            damageMultiplier *= 1.20f;
            lifestealPerKill += 0.20f;
            maxHP            += 20.0f;
            break;
        case AugType::CB_WARLORD:       // 광전사 + 연쇄폭발 → 전쟁군주
            damageMultiplier *= 1.25f;
            moveSpeedMult    *= 1.12f;
            deathBlast        = true;
            deathBlastMult   *= 1.6f;
            break;
        case AugType::CB_TEMPEST:       // 차크람 + 드론 → 난기류
            if (chakramCount < 3) ++chakramCount;
            if (droneCount   < 3) ++droneCount;
            fireInterval     /= 1.10f;
            break;
        case AugType::CB_OVERLORD:      // 오버드라이브 + 코어과부하 → 과부하 군주
            flatDamageBonus  += 35.0f;
            damageMultiplier *= 1.12f;
            break;
        case AugType::CB_HELLFIRE:      // 연쇄폭발 + 탄환세례 → 지옥불
            deathBlast        = true;
            deathBlastMult   *= 1.6f;
            bulletRain        = true;
            if (bulletRainCooldown > 5.0f) bulletRainCooldown = 5.0f;
            break;

        // ── 희귀: 탄환세례 / 드론 ──
        case AugType::BULLET_RAIN:
            bulletRain          = true;
            bulletRainCooldown  = 15.0f;
            break;
        case AugType::DRONE:
            drone      = true;
            droneCount = 1;
            break;
        case AugType::LASER:
            laser = true;
            break;
        case AugType::PURGE_NOVA:
            ++purgeNova;   // 중첩 시 주기↓·범위↑
            break;
        // ── 티어 연장 ──
        case AugType::LASER_2:
            laser = true;  laserTier = 2;   // 발사 간격↓·사거리↑ (main 이 tier 분기)
            break;
        case AugType::PIERCE_2:
            pierce = true;
            pierceChance = std::min(100, pierceChance + 30);
            break;
        case AugType::TWIN_2:
            twin = true;  twinCount = 3;     // 트리플 샷
            break;
        // ── 클래스 전용 (검객/궁수) ──
        case AugType::MELEE_WIDE:  meleeWide = true; break;
        case AugType::BLADE_WIND:  bladeWind = true; break;
        case AugType::POWER_DRAW:  powerDraw = true; break;
        case AugType::MULTISHOT:   multishot = true; break;
        // ── 에픽 강화 ──
        case AugType::BULLET_RAIN_2:
            bulletRain         = true;          // 안전망 (선행 조건 우회 대비)
            bulletRainCooldown = 10.0f;
            break;
        case AugType::CHAKRAM_2:
            chakram      = true;
            chakramCount = 2;
            break;
        // ── 전설 강화 ──
        case AugType::BULLET_RAIN_3:
            bulletRain         = true;
            bulletRainCooldown = 5.0f;
            break;
        case AugType::DRONE_2:
            drone      = true;
            droneCount = 2;
            break;
        case AugType::CHAKRAM_3:
            chakram      = true;
            chakramCount = 3;
            break;
        // ── 전설 (기존) ──
        case AugType::RANDOM_AUG:   /* main 에서 디스패치 */ break;
        case AugType::CANNON:
            cannon       = true;
            fireInterval = 1.0f;
            break;
        case AugType::SOUL_HARVEST: soulHarvest = true; break;

        // ── 디버프 ──
        case AugType::D_RMOB_MAX:
            if (rmobMaxBonus < 2) ++rmobMaxBonus;
            rmobSpawnDelayBonus += 0.5f;
            rangedXpBonus       += 12;     // (너프: 25 → 12)
            break;
        case AugType::D_RMOB_HP:
            rmobHpMult     *= 1.20f;
            rmobDmgMult    *= 1.20f;
            rangedXpBonus  += 12;          // (너프: 25 → 12)
            break;
        case AugType::D_RMOB_DMG:       // deprecated — dead code 유지
            rmobDmgMult    *= 1.10f;
            rangedXpBonus  += 5;
            break;
        case AugType::D_RMOB_DELAY:
            rmobDelayMult  *= 0.80f;
            rangedXpBonus  += 5;           // (너프: 10 → 5)
            break;
        case AugType::D_MOB_SPAWN:
            mobSpawnMult   *= 0.70f;
            mobCapBonus    += 200;
            meleeXpBonus   += 1;
            break;
        case AugType::D_SPLITTER:    // 분열체 출현 (죽으면 쪼개짐) · 처치 EXP +3
            splitterMobs   = true;
            meleeXpBonus   += 3;
            break;
        case AugType::D_BLINKER:     // 점멸체 출현 (순간이동 추격) · 처치 EXP +6
            blinkerMobs    = true;
            meleeXpBonus   += 6;
            break;
        case AugType::D_ORBITER:     // 공전체 출현 (스파이럴 인) · 처치 EXP +5
            orbiterMobs    = true;
            meleeXpBonus   += 5;
            break;
        case AugType::D_SPAWNER:     // 소환체 출현 (잡몹 소환) · 처치 EXP +7
            spawnerMobs    = true;
            meleeXpBonus   += 7;
            break;
        case AugType::D_SHIELDED:    // 보호막체 출현 (주기 방패) · 처치 EXP +5
            shieldedMobs   = true;
            meleeXpBonus   += 5;
            break;
        case AugType::D_APPROACH:
            approachingDeath = true;
            ++approachStacks;
            xpPerSec       += 0.5f;        // (너프: 1.0 → 0.5)
            break;
        case AugType::D_MOB_SPEED:
            mobSpeedMult   *= 1.10f;
            xpPerSec       += 0.5f;        // (너프: 1.0 → 0.5)
            break;
        case AugType::D_GLASS_HEART:
            maxHP          *= 0.80f;
            xpMult         *= 1.05f;       // (너프: 10% → 5%)
            break;
        case AugType::D_BULLET_STUCK:
            fireInterval   *= 1.25f;
            xpMult         *= 1.05f;       // (너프: 10% → 5%)
            break;
        case AugType::D_DRUNK:
            if (!drunk) {
                // 첫 픽
                drunk = true;
            } else {
                // 중복 픽: 활성 지속 +1s, 쿨타임 -2s (최소 4s)
                drunkActiveDuration += 1.0f;
                drunkCooldown = std::max(4.0f, drunkCooldown - 2.0f);
            }
            xpMult *= 1.05f;              // (너프: 20% → 10% → 5%; 활성 중 데미지 -40% 페널티 추가)
            break;
        // ── 자폭병 디버프 ──
        case AugType::D_BOMBER_BLAST:
            bomberBlastMult *= 1.50f;
            meleeXpBonus    += 12;         // (너프: 25 → 12)
            break;
        case AugType::D_BOMBER_BUFF:
            bomberHpMult    *= 1.50f;
            meleeXpBonus    += 10;         // (너프: 20 → 10)
            break;
        case AugType::D_BOMBER_SPEED:
            bomberSpeedMult *= 1.30f;
            meleeXpBonus    += 5;          // (너프: 10 → 5)
            break;
        // ── 잡몹/플레이어 디버프 ──
        case AugType::D_MOB_HP:
            monsterHpMult   *= 1.45f;        // 잡몹 체력 증가량 ↑ (1.20 → 1.45)
            meleeXpBonus    += 2;
            break;
        case AugType::D_SLOW_MOVE:
            moveSpeedMult   *= 0.95f;
            xpMult          *= 1.10f;      // (너프: 20% → 10%)
            break;
        // ── 핵앤슬래쉬 디버프 ──
        case AugType::D_BLEED:
            bleedPerSec     += 0.8f;       // 초당 HP -0.8
            xpMult          *= 1.12f;
            break;
        case AugType::D_WEAKEN:
            damageMultiplier *= 0.88f;     // 공격력 -12%
            xpMult           *= 1.10f;
            break;
        // ── 프로세스류(잡몹) 출현 디버프 (확장) ──
        case AugType::D_MOB_PACK:          // 군집 스폰 (스폰당 +2)
            mobPackBonus    += 2;
            meleeXpBonus    += 6;
            break;
        case AugType::D_MOB_ELITE:         // 엘리트 변종 확률 ↑
            eliteChanceMult *= 2.2f;
            meleeXpBonus    += 5;
            break;
        case AugType::D_MOB_FRENZY:        // 특수 잡몹 확률 ↑
            varietyChanceMult *= 1.8f;
            meleeXpBonus    += 4;
            break;

        // ── 특수 ──
        case AugType::S_CHAOS:   /* main 에서 디스패치 */ break;
        case AugType::S_PANDORA: /* main 에서 디스패치 */ break;
        }
    }

    // 최종 베이스 피해량 (미니화·대포 포함)
    float GetBaseDamage() const {
        float base = baseDamage + flatDamageBonus;   // 일반 증강 가산 데미지
        if (miniaturize) base += 10.0f * (float)totalAugs;
        return base;
    }

    // 최종 데미지 배율 (거리·시즈·영혼수확·미니화 연사 등)
    float GetDamageMultiplier(float distFromPlayer) const {
        float m = damageMultiplier;

        // 시즈탱크: 25% × stacks
        if (siegeTank)
            m *= (1.0f + 0.25f * (float)siegeStacks);

        // 저격수: 거리 비례 최대 +50%
        if (sniper) {
            float f = std::min(distFromPlayer / 1000.0f, 1.0f);
            m *= (1.0f + f * 0.5f);
        }

        // 총검: 200px 이내 +50%
        if (bayonet && distFromPlayer < 200.0f)
            m *= 1.5f;

        // 영혼 수확: 100킬당 +5%
        if (soulHarvest) {
            float bonus = (float)(killCount / 100) * 0.05f;
            m *= (1.0f + bonus);
        }

        // 대포: 추가 연사 1%당 공격력 +2% (연사력은 발사에 반영 안 되고 전부 공격력으로)
        if (cannon) {
            float fireRateMult = 1.0f / std::max(0.001f, fireInterval);
            float extraPct     = fireRateMult - 1.0f;  // 0% = 연사 1초 기준
            if (extraPct > 0.0f) m *= (1.0f + extraPct * 2.0f);
        }

        return m;
    }

    // 영혼 수확 연사·탄속 보너스 (외부에서 조회)
    float GetFireIntervalMult() const {
        float mult = 1.0f;
        if (soulHarvest) {
            float bonus = (float)(killCount / 100) * 0.02f;  // 너프: 5% → 2%
            mult /= (1.0f + bonus);
        }
        if (miniaturize)
            mult /= (1.0f + 0.02f * (float)totalAugs);       // 너프: 5% → 2%
        return mult;
    }
    float GetBulletSpeedBonus() const {
        float b = 0.0f;
        if (soulHarvest) {
            float bonus = (float)(killCount / 100) * 0.02f;
            b += bulletSpeed * bonus;
        }
        return b;
    }

    // 현재 이동속도 배율 (가벼운 발걸음·건러너 상태 반영)
    float GetMoveMultiplier(bool isFiring) const {
        float m = moveSpeedMult;
        // 가벼운 발걸음: 피격 후 비활성 동안 50% 보너스 제거
        if (lightStep && lightStepDisableTimer > 0.0f)
            m /= 1.50f;
        // 건 앤 러너: 미사격 시 +80%
        if (gunRunner && !isFiring)
            m *= 1.80f;
        return m;
    }
};
