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
    float damageReduction  = 0.0f;   // 받는 피해 감소 (0~0.35 캡)
    float regenLowHpMult   = 1.0f;   // REGEN_2 — 저체력 재생 배율
    int   vampireKillNeed  = 10;     // 흡혈마/흡혈탄 II — N킬당 HP +1
    float lightStepHitLock = 10.0f;  // 가벼운 발걸음 — 피격 후 비활성 시간(s)
    float playerSizeMult   = 1.0f;
    float xpMult           = 1.0f;   // 전체 EXP 곱연산 (유리심장, 총알걸림, 취함)
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
    bool  minigun      = false;  // 연사 ×2, 데미지 -30% (관통 없음 — 제거됨)
    bool  hackBomber   = false;  // 자폭병 처치 20% 폭발
    bool  hackRanged   = false;  // 원거리 처치 20% 유도탄 5
    bool  shotgun      = false;  // 5발 산탄 / 사거리 700
    bool  revolver     = false;  // 리볼버 시작무기/변환
    bool  shotgunSpread= false;  // 산탄 확장 — 7발
    bool  revolverOverload = false;
    bool  heShells     = false;
    bool  dashUpgrade  = false;  // SKILL_DASH_UP — 대시 유도탄 + 3발 2배
    float sniperDistBonusPct = 0.0f;  // SNIPER_AMPLIFIER — 거리 보너스 +%p
    int   powerSurgeStacks = 0;  // 전력 증폭 중첩 (3 이후 diminishing)
    int   commonMultBoosts = 0;  // 초반 일반 증강 ×1.08 (최대 3)
    // ── 핵앤슬래쉬 ──
    int   critChance   = 0;       // 치명타 확률 (%) — 25%/스택
    float critMult     = 2.5f;    // 치명타 데미지 배율
    float lifestealPerKill = 0.0f;// (legacy — GetLifestealPerKill() 사용)
    int   lifestealStacks  = 0;   // 흡혈탄 중첩 (최대 4)
    bool  lifesteal2       = false; // 흡혈탄 II — 한도 0.36
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
    bool  droneRapid   = false;   // 군집 지능(신화) — 드론 초고속 사격
    bool  laser        = false;   // 스캔 레이저 — 주기적 관통 빔 (군중제어)
    int   laserTier    = 1;       // 1 = LASER, 2 = LASER_2 (간격↓·사거리↑)
    int   purgeNova    = 0;       // 백신 스캔 — 주기적 범위 펄스 (중첩 시 강화)
    bool  bulletRain   = false;
    float bulletRainCooldown = 15.0f; // 15 → 10 (II) → 5 (III)
    bool  rainKillReduce = false;     // 무한 세례(신화) — 처치마다 쿨다운 감소
    int   chakramCount = 0;       // 1, 2, 3 — CHAKRAM / II / III
    bool  chakramSingularity = false; // 신화 — 끌어당김
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
    int   rmobDelayStacks = 0;   // 원거리 몹 가속 누적 (10 제한 — 과다 시 화면 밖으로 사라짐)
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
    // 잡몹 강화 디버프 (확장)
    float specialMobHpMult = 1.0f;  // 스케쥴러 강화 — 특수(비-NORMAL) 잡몹 HP 배율
    bool  trojanBoost     = false;  // 트로이목마 강화 — 점멸 쿨다운 단축(D_BLINKER 보유 시)
    bool  crasherBoost    = false;  // 크래셔 강화 — 돌진 중 받는 피해 -10%
    bool  badsectorMobs   = false;  // 배드 섹터 출현 (죽으면 감속 구역)
    bool  regerrorMobs    = false;  // 레지스트리 에러 출현 (강화 오라)
    bool  ddosMobs        = false;  // 디도스 침투
    bool  weaverBoost     = false;  // 위버 강화
    bool  bruteBoost      = false;  // 브루트 강화
    // 프로세스류(잡몹) 출현 디버프 (확장, 중첩 가능)
    int   mobPackBonus    = 0;     // 스폰당 추가 마리 수 (군집)
    float eliteChanceMult = 1.0f;  // 엘리트 변종 출현 확률 배율
    float varietyChanceMult = 1.0f;// 특수 잡몹(돌진/회피/거대) 출현 확률 배율
    // 핵앤슬래쉬 디버프
    float bleedPerSec     = 0.0f;  // 초당 HP 감소 (출혈)


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
            flatDamageBonus   += 9.0f;
            break;
        case AugType::RATE_UP:
            if (meleeWeapon)      flatDamageBonus   += 4.0f;
            else if (bowWeapon)   bowChargeRateMult *= 1.05f;
            else                  fireInterval      /= 1.04f;
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
        case AugType::REGEN_UP:  regenPerSec += 0.28f; break;
        case AugType::HP_UP:     maxHP += 15.0f; break;

        // ── 등급별 공격력 (가산) — 초반 강세. 곱연산 폭주 제거 ──
        case AugType::OVERDRIVE:      flatDamageBonus += 14.0f; break;
        case AugType::CORE_OVERLOAD:  flatDamageBonus += 24.0f; break;
        case AugType::POWER_SURGE:
            if (powerSurgeStacks < 3) damageMultiplier *= 1.05f;
            else                      damageMultiplier *= 1.03f;
            ++powerSurgeStacks;
            break; // 전설 +7%(유일 곱연산) — 3스택 이후 diminishing

        // ── 희귀 ──
        case AugType::GLASS_CANNON:
            damageMultiplier *= 1.50f;
            maxHP            *= 0.65f;
            break;
        case AugType::LIGHT_AMMO:
            fireInterval     /= 1.10f;   // 연사 +10%
            bulletSpeed      *= 1.30f;
            damageMultiplier *= 0.80f;   // 공격력 -20%
            break;
        case AugType::LIGHT_STEP:
            lightStep      = true;
            moveSpeedMult *= 1.30f;      // 이동속도 +30% (너프: +50% → +30%)
            break;
        case AugType::FIREWALL:
            damageReduction += 0.12f;
            break;
        case AugType::GUN_RUNNER:
            gunRunner = true;
            break;

        // ── 에픽 ──
        case AugType::VAMPIRE:
            vampire = true;
            maxHP  += 20.0f;
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
        case AugType::MINIATURIZE:
            miniaturize    = true;
            sizeAugTaken   = true;
            maxHP         *= 0.5f;
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
        case AugType::SKILL_FOCUS:
            break;
        case AugType::SKILL_DASH_UP:
            dashUpgrade = true;
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
            critChance = std::min(75, critChance + 15);   // 15%/스택, 최대 75% (카드 설명과 일치)
            critMult   = 2.0f;                            // 배율 너프: 2.5 → 2.0
            break;
        case AugType::LIFESTEAL:
            if (lifestealStacks < 4) ++lifestealStacks;
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
        case AugType::CB_BLOODLORD:     // 흡혈탄 II + 흡혈마
            maxHP            += 15.0f;
            regenPerSec      += 0.25f;
            break;
        case AugType::CB_BASTION:       // 거대화 + MK2 + 방화벽
            maxHP            *= 1.15f;
            regenPerSec      += 0.35f;
            damageReduction  += 0.08f;
            break;
        case AugType::CB_LIFEBUOY:      // 재생 II + 흡혈마 + 가벼운 발걸음
            regenPerSec      += 0.25f;
            moveSpeedMult    *= 1.12f;
            vampireKillNeed  = 7;
            lightStepHitLock = 6.0f;
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
        case AugType::CB_RAILGUN:       // 저격 + 관통 → 레일건 (철갑탄과 분리: 확률 관통+거리)
            sniper       = true;
            pierce       = true;
            pierceChance = std::min(85, pierceChance + 15);
            sniperDistBonusPct += 0.20f;
            damageMultiplier *= 1.30f;
            bulletSpeed  *= 1.35f;
            break;
        case AugType::CB_GLASS_REAPER:  // 유리대포 + 흡혈탄 → 유리 사신
            damageMultiplier *= 1.20f;
            lifestealPerKill += 0.20f;
            maxHP            += 20.0f;
            break;
        case AugType::CB_WARLORD:       // 광전사 + 연쇄폭발 → 전쟁군주 (영혼 수확 능력)
            damageMultiplier *= 1.15f;
            soulHarvest       = true;   // 100킬마다 영구 누적 (영혼수확 이전)
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
        case AugType::BULLET_RAIN_ETERNAL:   // 신화 — 무한 세례 (III 쿨 유지 + 처치 가속)
            bulletRain         = true;
            rainKillReduce     = true;
            if (bulletRainCooldown > 8.0f) bulletRainCooldown = 8.0f;
            break;
        case AugType::DRONE_HIVE:            // 신화 — 군집 지능
            drone      = true;
            droneCount = 2;                  // 최대치(MAX_DRONES)
            droneRapid = true;               // main: 드론 발사 간격 대폭 단축
            break;
        case AugType::LASER_CONVERGE:        // 신화 — 수렴
            laser     = true;
            laserTier = 3;                   // main: 거의 연속 발사 + 초장거리
            break;
        case AugType::PIERCE_RAILSLUG:       // 신화 — 철갑탄 (관통 90% 고정 너프)
            pierce          = true;
            pierceChance    = 90;
            flatDamageBonus += 25.0f;
            bulletSpeed     *= 1.50f;
            break;
        case AugType::CB_HELLFIRE:      // 연쇄폭발 + 탄환세례 → 지옥불
            deathBlast        = true;
            deathBlastMult   *= 1.6f;
            bulletRain        = true;
            if (bulletRainCooldown > 5.0f) bulletRainCooldown = 5.0f;
            break;
        case AugType::CB_TURRET:        // 대포 + 드론 II → 포탑 배치
            turretMode = true;          // main: 드론 공전 대신 자동 포탑 전개
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
            rangedXpBonus  += 6;           // (너프: 12 → 6)
            break;
        case AugType::D_RMOB_DELAY:
            if (rmobDelayStacks < 10) {     // 10 제한 (13쯤부터 너무 빨라 화면 밖으로 사라짐)
                rmobDelayMult *= 0.80f;
                ++rmobDelayStacks;
            }
            rangedXpBonus  += 5;           // (너프: 10 → 5)
            break;
        case AugType::D_MOB_SPAWN:
            mobSpawnMult   *= 0.70f;
            mobCapBonus    += 200;
            meleeXpBonus   += 1;
            break;
        case AugType::D_SPLITTER:    // 웜 침투 (죽으면 쪼개짐) · 처치 EXP +2
            splitterMobs   = true;
            meleeXpBonus   += 2;           // (너프: 3 → 2)
            break;
        case AugType::D_BLINKER:     // 트로이목마 침투 (순간이동 추격) · 처치 EXP +3
            blinkerMobs    = true;
            meleeXpBonus   += 3;           // (너프: 6 → 3)
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
            meleeXpBonus    += 5;          // (너프: 10 → 5)
            break;
        case AugType::D_BOMBER_SPEED:
            bomberSpeedMult *= 1.30f;
            meleeXpBonus    += 3;          // (너프: 5 → 3)
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
        case AugType::D_MOB_ELITE:         // 권한 상승 — 엘리트 변종(변종개체) 확률 ↑
            eliteChanceMult *= 2.2f;
            meleeXpBonus    += 3;          // (너프: 5 → 3)
            break;
        case AugType::D_MOB_FRENZY:        // 특수 잡몹 확률 ↑
            varietyChanceMult *= 1.8f;
            meleeXpBonus    += 4;
            break;
        case AugType::D_SCHEDULER:         // 스케쥴러 강화 — 특수 잡몹 HP +10%
            specialMobHpMult *= 1.10f;
            meleeXpBonus    += 2;          // (너프: 3 → 2)
            break;
        case AugType::D_TROJAN_BOOST:      // 트로이목마 강화 — 점멸 쿨다운 단축
            trojanBoost     = true;
            meleeXpBonus    += 2;          // (너프: 4 → 2)
            break;
        case AugType::D_CRASHER_BOOST:     // 크래셔 강화 — 돌진 중 받는 피해 -10%
            crasherBoost    = true;
            meleeXpBonus    += 4;
            break;
        case AugType::D_BADSECTOR:         // 배드 섹터 출현
            badsectorMobs   = true;
            meleeXpBonus    += 8;
            break;
        case AugType::D_REGERROR:          // 레지스트리 에러 출현
            regerrorMobs    = true;
            meleeXpBonus    += 10;
            break;
        case AugType::D_DDOS:
            ddosMobs        = true;
            meleeXpBonus    += 4;
            break;
        case AugType::D_WEAVER_BOOST:
            weaverBoost     = true;
            meleeXpBonus    += 3;
            break;
        case AugType::D_BRUTE_BOOST:
            bruteBoost      = true;
            meleeXpBonus    += 4;
            break;
        case AugType::LIFESTEAL_2:
            lifesteal2      = true;
            break;
        case AugType::REGEN_2:
            regenPerSec    += 0.45f;
            regenLowHpMult  = 2.0f;
            break;
        case AugType::CHAIN_2:
            ricochetMax     = 3;
            ricochetChance  = 100;
            ricochetDmgMult = 0.80f;
            break;
        case AugType::SHOTGUN_SPREAD:
            shotgunSpread   = true;
            break;
        case AugType::REVOLVER_OVERLOAD:
            revolverOverload = true;
            break;
        case AugType::HE_SHELLS:
            heShells        = true;
            break;
        case AugType::SMG_COMPRESSOR:
            bulletSpread   *= 0.50f;
            fireInterval   /= 1.08f;
            break;
        case AugType::RIFLE_STABILITY:
            bulletSpread    = 0.0f;
            flatDamageBonus += 12.0f;
            break;
        case AugType::SNIPER_AMPLIFIER:
            sniperDistBonusPct += 0.30f;
            break;
        case AugType::CHAKRAM_SINGULARITY:
            chakramSingularity = true;
            chakram         = true;
            if (chakramCount < 3) chakramCount = 3;
            break;

        // ── 특수 ──
        case AugType::S_CHAOS:   /* main 에서 디스패치 */ break;
        case AugType::S_PANDORA: /* main 에서 디스패치 */ break;
        }
    }

    float GetDamageTakenMult() const {
        float dr = damageReduction;
        if (dr > 0.35f) dr = 0.35f;
        return 1.0f - dr;
    }

    float GetRegenRate(float curHP) const {
        float r = regenPerSec;
        if (regenLowHpMult > 1.0f && curHP < maxHP * 0.40f)
            r *= regenLowHpMult;
        return r;
    }

    int GetVampireKillNeed() const { return vampireKillNeed; }

    // 최종 베이스 피해량 (미니화·대포 포함)
    float GetBaseDamage() const {
        float base = baseDamage + flatDamageBonus;   // 일반 증강 가산 데미지
        if (miniaturize) base += 10.0f * (float)totalAugs;
        return base;
    }

    // 최종 데미지 배율 (거리·시즈·영혼수확·미니화 연사 등)
    float GetDamageMultiplier(float distFromPlayer) const {
        float m = damageMultiplier;

        // 저격: 거리 비례 (기본 50% + SNIPER_AMPLIFIER %p)
        if (sniper || sniperDistBonusPct > 0.0f) {
            float f = std::min(distFromPlayer / 1000.0f, 1.0f);
            m *= (1.0f + f * (0.5f + sniperDistBonusPct));
        }

        // 총검: 200px 이내 +50%
        if (bayonet && distFromPlayer < 200.0f)
            m *= 1.5f;

        // 영혼 수확: 1500킬당 +5% (최대 7스택)
        if (soulHarvest) {
            int souls = (int)(killCount / 1500); if (souls > 7) souls = 7;
            m *= (1.0f + (float)souls * 0.05f);
        }

        // 대포: 연사→공격 변환 상한 +80%
        if (cannon) {
            float fireRateMult = 1.0f / std::max(0.001f, fireInterval);
            float extraPct     = fireRateMult - 1.0f;
            if (extraPct > 0.0f) {
                float bonus = extraPct * 2.0f;
                if (bonus > 0.80f) bonus = 0.80f;
                m *= (1.0f + bonus);
            }
        }

        return m;
    }

    // 영혼 수확 연사·탄속 보너스 (외부에서 조회)
    float GetFireIntervalMult() const {
        float mult = 1.0f;
        if (soulHarvest) {
            int souls = (int)(killCount / 1500); if (souls > 7) souls = 7;
            mult /= (1.0f + (float)souls * 0.02f);
        }
        if (miniaturize)
            mult /= (1.0f + 0.02f * (float)totalAugs);       // 너프: 5% → 2%
        return mult;
    }
    float GetBulletSpeedBonus() const {
        float b = 0.0f;
        if (soulHarvest) {
            int souls = (int)(killCount / 1500); if (souls > 7) souls = 7;
            b += bulletSpeed * (float)souls * 0.02f;
        }
        return b;
    }

    // 흡혈탄 티어 — 스택×0.06, 한도: 0.24 / II 0.36 / 흡혈마 0.48
    float GetLifestealCap() const {
        if (vampire)     return 0.48f;
        if (lifesteal2)  return 0.36f;
        return 0.24f;
    }
    float GetLifestealPerKill() const {
        float v = (float)lifestealStacks * 0.06f;
        float cap = GetLifestealCap();
        if (v > cap) v = cap;
        return v;
    }

    // 일반 증강 초반 부스트 (최대 3회 ×1.08)
    void ApplyCommonMultBoost() {
        if (commonMultBoosts < 3) {
            damageMultiplier *= 1.08f;
            ++commonMultBoosts;
        }
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

// 전투 피해 감소 — main 에서 g_Stats.GetDamageTakenMult() 로 매 프레임 동기화
inline float g_PlayerDmgMult = 1.0f;
inline void HurtPlayer(float& hp, float raw) {
    if (raw <= 0.0f) return;
    hp -= raw * g_PlayerDmgMult;
}
