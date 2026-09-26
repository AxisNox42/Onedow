#pragma once
#include "PlayerStats.h"

// The live player loadout contains only the rifle and the static field.
enum class StartWeapon {
    SMG,
    RIFLE,
    _COUNT
};

struct WeaponDef {
    const wchar_t* locName[LANG_COUNT];
    const wchar_t* locDesc[LANG_COUNT];
};

inline const WeaponDef ALL_WEAPONS[] = {
    { { L"전기장", L"Static Field", L"静電場" },
      { L"플레이어 주변을 지속 전기장으로 제압하는 범위형 무기",
        L"Area weapon that controls nearby space with a sustained electric field",
        L"プレイヤー周囲を持続電場で制圧する範囲武器" } },
    { { L"소총", L"Rifle", L"ライフル" },
      { L"소총 — 표준 연사, 균형 잡힌 범용 무기",
        L"Balanced all-rounder",
        L"ライフル — 標準連射、バランス型汎用武器" } },
};

inline const wchar_t* WeaponName(const WeaponDef& w) { return w.locName[CurLangIdx()]; }
inline const wchar_t* WeaponDesc(const WeaponDef& w) { return w.locDesc[CurLangIdx()]; }

extern PlayerStats g_Stats;
extern int         g_CurrentWeapon;

inline const wchar_t* CurrentWeaponLabel() {
    if (g_CurrentWeapon >= 0 && g_CurrentWeapon < (int)StartWeapon::_COUNT)
        return WeaponName(ALL_WEAPONS[g_CurrentWeapon]);
    static const wchar_t* none[3] = { L"(??)", L"(none)", L"(none)" };
    return none[CurLangIdx()];
}

inline const wchar_t* CurrentWeaponDescText() {
    if (g_CurrentWeapon >= 0 && g_CurrentWeapon < (int)StartWeapon::_COUNT)
        return WeaponDesc(ALL_WEAPONS[g_CurrentWeapon]);
    static const wchar_t* none[3] = { L"", L"", L"" };
    return none[CurLangIdx()];
}

inline void ApplyWeapon(PlayerStats& s, StartWeapon w) {
    switch (w) {
    case StartWeapon::SMG:
        s.fireInterval     *= 0.50f;
        s.damageMultiplier *= 0.50f;
        s.bulletSpread      = 0.15f;
        break;
    case StartWeapon::RIFLE:
        s.bulletSpread = 0.04f;
        break;
    case StartWeapon::_COUNT:
        break;
    }
}

inline void MarkStartWeaponOwnedType(StartWeapon w) {
    if (w == StartWeapon::SMG)
        g_TypeOwned[(int)AugType::STATIC_FIELD] = true;
}
