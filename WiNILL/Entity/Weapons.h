#pragma once
#include "PlayerStats.h"

// ─────────────────────────────────────────────────────────────
// 시작 무기 (Starter Weapons)
//   직업(JobId)마다 고정 무기 1개로 매핑됨 (Achievements.h JOB_DEFS::fixedWeapon).
//   검객/궁수는 weaponMode(1/2)가 이 무기를 덮어써 근접/활 조작으로 대체.
// ─────────────────────────────────────────────────────────────
enum class StartWeapon {
    SMG, SNIPER, RIFLE, SHOTGUN, CANNON, REVOLVER, _COUNT
};

struct WeaponDef {
    const wchar_t* locName[LANG_COUNT];   // [KR, EN, JP]
    const wchar_t* locDesc[LANG_COUNT];   // [KR, EN, JP]
};

inline const WeaponDef ALL_WEAPONS[] = {
    /* SMG */ {
      { L"기관단총", L"SMG", L"サブマシンガン" },
      { L"연사 2배 — 탄막 압박형 / 공격력 -50% / 탄이 넓게 퍼짐 — 근거리 화력 보조",
        L"2x fire rate — bullet hose / -50% attack / wide spread — close-range pressure",
        L"連射2倍の弾幕 / 攻撃-50% / 拡散大 — 近距離押し" } },
    /* SNIPER */ {
      { L"저격총", L"Sniper", L"スナイパー" },
      { L"공격력 ×2 · 탄속 ×1.8 / 관통 40% / 연사 -33% — 멀수록 유리, 정조준 권장",
        L"×2 attack · ×1.8 bullet speed / 40% pierce / -33% fire rate — favors distance",
        L"攻撃×2・弾速×1.8 / 貫通40% / 連射-33% — 遠距離向き" } },
    /* RIFLE */ {
      { L"소총", L"Rifle", L"ライフル" },
      { L"만능 균형형 무기 / 약한 탄 퍼짐 / 증강과 궁합이 좋은 기본 선택",
        L"Balanced all-rounder / slight spread / pairs well with most augments",
        L"バランス万能 / わずかな拡散 / 強化と相性良" } },
    /* SHOTGUN */ {
      { L"샷건", L"Shotgun", L"ショットガン" },
      { L"한 방에 펠릿 5발 부채꼴 / 사거리 700 · 연사 느림 / 근접 일소·군중 제어",
        L"5 pellets per shot in a cone / range 700 · slow fire / close crowd clear",
        L"1射撃5発の扇 / 射程700・連射遅 / 近距離掃討" } },
    /* CANNON */ {
      { L"대포", L"Cannon", L"大砲" },
      { L"1초마다 거대 탄 1발 / 공격력 ×5 / 연사 증강 1%당 추가 피해 +2%",
        L"One big shell per second / ×5 attack / +2% dmg per 1% fire rate from augs",
        L"1秒毎に巨大弾 / 攻撃×5 / 連射強化1%毎に+2%ダメ" } },
    /* REVOLVER */ {
      { L"리볼버", L"Revolver", L"リボルバー" },
      { L"공격력 ×1.4 / 연사 -29% / 탄 퍼짐 없음 — 정확한 단발·치명타 빌드에 적합",
        L"×1.4 attack / -29% fire rate / no spread — precise single-shot builds",
        L"攻撃×1.4 / 連射-29% / 拡散なし — 単発精密向き" } },
};

inline const wchar_t* WeaponName(const WeaponDef& w) { return w.locName[CurLangIdx()]; }
inline const wchar_t* WeaponDesc(const WeaponDef& w) { return w.locDesc[CurLangIdx()]; }

extern PlayerStats g_Stats;
extern int         g_CurrentWeapon;

inline const wchar_t* CurrentWeaponLabel() {
    if (g_Stats.meleeWeapon) {
        static const wchar_t* M[3] = { L"검객 (근접)", L"Blade (Melee)", L"剣客 (近接)" };
        return M[CurLangIdx()];
    }
    if (g_Stats.bowWeapon) {
        static const wchar_t* B[3] = { L"궁수 (활)", L"Archer (Bow)", L"弓師 (弓)" };
        return B[CurLangIdx()];
    }
    if (g_CurrentWeapon >= 0 && g_CurrentWeapon < (int)StartWeapon::_COUNT)
        return WeaponName(ALL_WEAPONS[g_CurrentWeapon]);
    static const wchar_t* None[3] = { L"(없음)", L"(none)", L"(なし)" };
    return None[CurLangIdx()];
}

inline const wchar_t* CurrentWeaponDescText() {
    if (g_Stats.meleeWeapon || g_Stats.bowWeapon) {
        static const wchar_t* C[3] = { L"클래스 전용 무기", L"Class weapon", L"クラス専用武器" };
        return C[CurLangIdx()];
    }
    if (g_CurrentWeapon >= 0 && g_CurrentWeapon < (int)StartWeapon::_COUNT)
        return WeaponDesc(ALL_WEAPONS[g_CurrentWeapon]);
    static const wchar_t* None[3] = { L"", L"", L"" };
    return None[CurLangIdx()];
}

inline void ApplyWeapon(PlayerStats& s, StartWeapon w) {
    switch (w) {
    case StartWeapon::SMG:
        s.fireInterval     *= 0.50f;
        s.damageMultiplier *= 0.50f;
        s.bulletSpread      = 0.18f;
        break;
    case StartWeapon::SNIPER:
        s.fireInterval     *= 1.90f;   // 연사 너프 (1.5 → 1.9)
        s.damageMultiplier *= 2.0f;
        s.bulletSpeed      *= 1.8f;
        s.pierce           = true;
        s.pierceChance     = 40;
        s.bulletSpread     = 0.0f;
        break;
    case StartWeapon::RIFLE:
        s.bulletSpread     = 0.04f;
        break;
    case StartWeapon::SHOTGUN:
        s.shotgun          = true;
        s.distAugTaken     = true;   // 거리 카테고리 점유
        s.fireInterval    *= 3.0f;   // 연사 대폭 너프 (1.5 → 3.0)
        s.bulletSpread     = 0.0f;
        break;
    case StartWeapon::CANNON:
        s.cannon           = true;
        s.fireInterval     = 1.0f;
        s.damageMultiplier *= 5.0f;  // 자체 공격력 ×10 → ×5
        break;
    case StartWeapon::REVOLVER:
        s.revolver         = true;
        s.fireInterval     *= 1.4f;
        s.damageMultiplier *= 1.4f;
        s.bulletSpread     = 0.0f;
        break;
    case StartWeapon::_COUNT: break;
    }
}

