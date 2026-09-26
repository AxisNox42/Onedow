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
    float xpMult           = 1.0f;   // Global experience multiplier.
    float bulletSpread     = 0.0f;   // 발사 시 각도 흔들기 (라디안). 0 = 정확
    int   pierceChance     = 30;     // Pierce chance (%).
    // The active close-range roster has five MobKind entries. Scope is the
    // separate ranged mob and uses rangedXpBonus below.
    static constexpr int MOB_KIND_XP_SLOTS = 5;
    int   mobXpBonus       = 0;      // Process kill EXP bonus for generic mob debuffs.
    int   mobKindXpBonus[MOB_KIND_XP_SLOTS] = {};
    int   rangedXpBonus    = 0;      // Ranged mob kill EXP bonus.
    float xpPerSec         = 0.0f;   // 초당 누적 EXP (다가오는 죽음, 잡몹 가속)
    float rmobSpawnDelayBonus = 0.0f;// 원거리 몹 스폰 가속 (초)
    int   mobCapBonus      = 0;      // 잡몹 동시 존재 한도 추가

    // ── 카운터/락 ───────────────────────────────────────
    int  visionStacks   = 0;          // 최대 5 (총 +350)
    int  totalAugs      = 0;
    bool sizeAugTaken   = false;

    // ── 희귀 ───────────────────────────────────────────
    bool  lightStep            = false;
    float lightStepDisableTimer = 0.0f;  // 피격 후 카운트다운 (s)

    // ── 에픽 ───────────────────────────────────────────
    bool  vampire     = false;   // 10킬당 HP +1
    int   vampireKillStreak = 0; // 10에 도달하면 회복 + 0 리셋
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
    bool  hackRanged   = false;  // 원거리 처치 20% 유도탄 5
    bool  dashUpgrade  = false;  // SKILL_DASH_UP — 대시 유도탄 + 3발 2배
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
    float deathBlastMult = 1.0f;  // 연쇄 폭발 반경 배율
    float deathBlastDmgPct = 0.30f; // 폭발 피해 (공격력 대비)
    // Player weapons are restricted to the rifle and static field.
    bool  drone        = false;
    int   droneCount   = 0;       // 1 = DRONE, 2 = DRONE_2, 4 = DRONE_HIVE
    bool  droneRapid   = false;   // 예전 군집 지능 연사 가속 플래그(현재 비활성)
    bool  laser        = false;   // 스캔 레이저 — 주기적 관통 빔 (군중제어)
    int   laserTier    = 1;       // 1 = LASER, 2 = LASER_2 (간격↓·사거리↑)
    bool  bulletRain   = false;
    float bulletRainCooldown = 15.0f; // 15 → 10 (II) → 5 (III)
    bool  rainKillReduce = false;     // 무한 세례(신화) — 처치마다 쿨다운 감소
    int   chakramCount = 0;       // 1, 2, 3 — CHAKRAM / II / III
    bool  chakramSingularity = false; // 신화 — 끌어당김
    long long killCount = 0;     // Total kills in the current run.

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
    float mobSpeedMult  = 1.0f;
    bool  approachingDeath = false;
    int   approachStacks   = 0;   // D_APPROACH 누적 횟수 (속도 +20%/스택)
    // 잡몹 HP 디버프
    float monsterHpMult   = 1.0f;
    // 잡몹 강화 디버프 (확장)
    // 프로세스류(잡몹) 출현 디버프 (확장, 중첩 가능)
    int   mobPackBonus    = 0;     // 스폰당 추가 마리 수 (군집)
    // 핵앤슬래쉬 디버프


    // 일반(COMMON) 증강 = 가산(flat) — 곱연산 복리 폭주(원펀맨) 방지.
    //   초반엔 baseDamage 대비 큰 비중, 후반엔 큰 base 대비 상대값 자동 감소.
    //   후반 스케일링은 희귀/에픽의 곱연산 증강(damageMultiplier)이 담당.
    float flatDamageBonus = 0.0f;   // DMG_UP 누적 가산 데미지 (일반 증강 = 고정값)

    // ─────────────────────────────────────────────────────
    void Apply(AugType t) {
        ++totalAugs;
        switch (t) {
        // ── 일반 (버프: QA 피드백 — 일반 증강이 너무 약함) ──
        case AugType::DMG_UP:
            flatDamageBonus   += 9.0f;
            break;
        case AugType::RATE_UP:
            fireInterval /= 1.04f;
            break;
        case AugType::SPD_UP:
            bulletSpeed += 30.0f;
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
            damageMultiplier *= 0.85f;   // 공격력 -15%
            break;
        case AugType::LIGHT_STEP:
            lightStep      = true;
            moveSpeedMult *= 1.30f;      // 이동속도 +30% (너프: +50% → +30%)
            break;
        case AugType::FIREWALL:
            damageReduction += 0.12f;
            break;
        case AugType::VAMPIRE:
            vampire = true;
            maxHP  += 20.0f;
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
            regenPerSec   += 0.7f;
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
        case AugType::HACK_RANGED: hackRanged = true; break;
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
        case AugType::SKILL_DASH_UP:
            dashUpgrade = true;
            break;
        case AugType::MK2:         mk2        = true; break;
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
            deathBlast = true;
            deathBlastDmgPct = 0.30f;
            break;
        case AugType::DEATH_BLAST_2:
            deathBlast = true;
            deathBlastDmgPct = 0.40f;
            deathBlastMult  *= 1.4f;
            break;

        // ── 조합 (COMBO) — 레시피 충족 시에만 등장 ──
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
        case AugType::CB_WARLORD:       // Berserk + chain explosion.
            damageMultiplier *= 1.15f;
            break;
        case AugType::BULLET_RAIN_ETERNAL:   // 신화 — 무한 세례 (4초 쿨 + 처치 가속)
            bulletRain         = true;
            rainKillReduce     = true;
            bulletRainCooldown = 4.0f;
            break;
        case AugType::DRONE_HIVE:            // 신화 — 군집 지능
            drone      = true;
            droneCount = 4;
            droneRapid = false;
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
        case AugType::LASER_2:
            laser = true;  laserTier = 2;   // 발사 간격↓·사거리↑ (main 이 tier 분기)
            break;
        case AugType::PIERCE_2:
            pierce = true;
            pierceChance = std::min(100, pierceChance + 30);
            break;
        case AugType::TWIN_2:
            twin = true;  twinCount = 3;     // 트리플 샷
            damageMultiplier *= 0.88f;
            break;
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
        case AugType::D_RMOB_MAX:
            if (rmobMaxBonus < 2) ++rmobMaxBonus;
            rmobSpawnDelayBonus += 0.5f;
            rangedXpBonus       += 12;
            break;
        case AugType::D_RMOB_HP:
            rmobHpMult     *= 1.20f;
            rmobDmgMult    *= 1.20f;
            rangedXpBonus  += 6;
            break;
        case AugType::D_RMOB_DELAY:
            if (rmobDelayStacks < 10) {
                rmobDelayMult *= 0.80f;
                ++rmobDelayStacks;
            }
            rangedXpBonus  += 5;
            break;
        case AugType::D_MOB_SPAWN:
            // Controlled pressure increase; do not unlock the full cap at once.
            mobSpawnMult   *= 0.84f;
            mobCapBonus    += 12;
            mobXpBonus     += 1;
            break;
        case AugType::D_APPROACH:
            approachingDeath = true;
            if (approachStacks < 3) {
                ++approachStacks;
                xpPerSec   += 0.5f;
            }
            break;
        case AugType::D_MOB_SPEED:
            mobSpeedMult   *= 1.10f;
            xpPerSec       += 0.5f;
            break;
        case AugType::D_GLASS_HEART:
            maxHP          *= 0.80f;
            xpMult         *= 1.03f;
            break;
        case AugType::D_BULLET_STUCK:
            fireInterval   /= 0.90f;
            xpMult         *= 1.05f;
            break;
        case AugType::D_MOB_HP:
            monsterHpMult   *= 1.30f;
            mobXpBonus      += 2;
            break;
        case AugType::D_SLOW_MOVE:
            moveSpeedMult   *= 0.95f;
            xpMult          *= 1.03f;
            break;
        case AugType::D_BLEED:
            regenPerSec      = std::max(0.0f, regenPerSec - 1.0f);
            xpMult          *= 1.12f;
            break;
        case AugType::D_WEAKEN:
            damageMultiplier *= 0.88f;
            xpMult           *= 1.10f;
            break;
        case AugType::D_MOB_PACK:
            // The time curve already increases wave size, so one extra body is enough.
            mobPackBonus    += 1;
            mobXpBonus      += 6;
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
            ricochetDmgMult = 0.85f;
            break;
        case AugType::RIFLE_STABILITY:
            bulletSpread    = 0.0f;
            flatDamageBonus += 8.0f;
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

    // Final base damage with active size scaling.
    float GetBaseDamage() const {
        float base = baseDamage + flatDamageBonus;   // 일반 증강 가산 데미지
        if (miniaturize) base += 10.0f * (float)totalAugs;
        return base;
    }

    // Final damage multiplier.
    float GetDamageMultiplier() const {
        return damageMultiplier;
    }

    float GetFireIntervalMult() const {
        float mult = 1.0f;
        if (miniaturize)
            mult /= (1.0f + 0.02f * (float)totalAugs);
        return mult;
    }
    float GetBulletSpeedBonus() const { return 0.0f; }

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

    // 일반 증강 초반 부스트 (첫 일반 증강 1회만)
    void ApplyCommonMultBoost() {
        if (commonMultBoosts < 1) {
            damageMultiplier *= 1.08f;
            ++commonMultBoosts;
        }
    }
    // Current movement multiplier.
    float GetMoveMultiplier(bool /*isFiring*/) const {
        float m = moveSpeedMult;
        if (lightStep && lightStepDisableTimer > 0.0f)
            m /= 1.30f;
        return m;
    }
};

// 전투 피해 감소 — main 에서 g_Stats.GetDamageTakenMult() 로 매 프레임 동기화
inline float g_PlayerDmgMult = 1.0f;
inline float g_PlayerShield     = 0.0f;
inline float g_PlayerShieldTimer = 0.0f;
inline float g_PlayerDamagePulse = 0.0f;

inline void GrantPlayerShield(float amount, float durationSec) {
    if (amount <= 0.0f || durationSec <= 0.0f) return;
    g_PlayerShield      = amount;
    g_PlayerShieldTimer = durationSec;
}

inline void TickPlayerShield(float dt) {
    if (g_PlayerShieldTimer > 0.0f) {
        g_PlayerShieldTimer -= dt;
        if (g_PlayerShieldTimer <= 0.0f) {
            g_PlayerShieldTimer = 0.0f;
            g_PlayerShield      = 0.0f;
        }
    }
}

inline void HurtPlayer(float& hp, float raw) {
    if (raw <= 0.0f) return;
    float dmg = raw * g_PlayerDmgMult;
    if (g_PlayerShield > 0.0f) {
        if (dmg <= g_PlayerShield) {
            g_PlayerShield -= dmg;
            return;
        }
        dmg -= g_PlayerShield;
        g_PlayerShield = 0.0f;
    }
    hp -= dmg;
    // Notify the combat HUD at the source of the hit, including tiny hits
    // that can be obscured by several fixed-step updates in one frame.
    g_PlayerDamagePulse = std::max(g_PlayerDamagePulse, 2.2f);
}
