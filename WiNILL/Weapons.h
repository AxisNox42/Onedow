#pragma once
#include "PlayerStats.h"

// ─────────────────────────────────────────────────────────────
// 시작 무기 (Starter Weapons)
//   게임 시작 시 6개 중 랜덤 3개를 카드로 제시
//   고른 무기는 PlayerStats 에 즉시 적용 (증강처럼)
// ─────────────────────────────────────────────────────────────
enum class StartWeapon {
    SMG, SNIPER, RIFLE, SHOTGUN, CANNON, REVOLVER, _COUNT
};

struct WeaponDef {
    const wchar_t* krName;
    const wchar_t* krDesc;
};

inline const WeaponDef ALL_WEAPONS[] = {
    /* SMG      */ { L"기관단총", L"연사 ×2  /  공격력 -50%  /  탄 흩어짐" },
    /* SNIPER   */ { L"저격총",   L"공격력 ×2  /  탄속 ×1.8  /  관통 40%  /  연사 -33%" },
    /* RIFLE    */ { L"소총",     L"균형형  /  약간 흩어짐" },
    /* SHOTGUN  */ { L"샷건",     L"한 번에 5발  /  사거리 700  /  연사 -33%" },
    /* CANNON   */ { L"대포",     L"연사 1초 고정  /  공격력 ×10  /  큰 탄  /  [조합] 드론II = 포탑" },
    /* REVOLVER */ { L"리볼버",   L"공격력 ×1.4  /  연사 -29%  /  정확" },
};

inline void ApplyWeapon(PlayerStats& s, StartWeapon w) {
    switch (w) {
    case StartWeapon::SMG:
        s.fireInterval     *= 0.50f;
        s.damageMultiplier *= 0.50f;
        s.bulletSpread      = 0.18f;
        break;
    case StartWeapon::SNIPER:
        s.fireInterval     *= 1.50f;
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
        s.fireInterval    *= 1.5f;
        s.bulletSpread     = 0.0f;
        break;
    case StartWeapon::CANNON:
        s.cannon           = true;
        s.fireInterval     = 1.0f;
        s.damageMultiplier *= 10.0f;
        break;
    case StartWeapon::REVOLVER:
        s.fireInterval     *= 1.4f;
        s.damageMultiplier *= 1.4f;
        s.bulletSpread     = 0.0f;
        break;
    case StartWeapon::_COUNT: break;
    }
}

// 6개 중 랜덤 3개 (중복 없음) → outArr[0..2]
inline void PickRandomWeapons(int outArr[3]) {
    int pool[(int)StartWeapon::_COUNT];
    int n = (int)StartWeapon::_COUNT;
    for (int i = 0; i < n; i++) pool[i] = i;
    for (int i = 0; i < 3 && i < n; i++) {
        int j = i + rand() % (n - i);
        int tmp = pool[i]; pool[i] = pool[j]; pool[j] = tmp;
        outArr[i] = pool[i];
    }
}
