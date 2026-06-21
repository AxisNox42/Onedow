#pragma once
#include "Augment.h"
#include "GameManager.h"
#include "BossDirector.h"
#include "MonsterManager.h"
#include <vector>

extern GameManager g_GameManager;

// ── 보스 클리어 후 휴식 구간 (Rabbit & Steel 스타일) ──
constexpr float INTERMISSION_DUR   = 120.0f;  // 최대 2분
constexpr float SHOP_ZONE_RADIUS   = 78.0f;
constexpr float SHOP_ZONE_HOLD_S   = 0.35f;   // 구역 안에 이만큼 있으면 상점 오픈
constexpr float SHOP_ZONE_OFFSET_X = 200.0f;  // 플레이어 중심 기준 상점 구역

inline float   g_IntermissionTimer = 0.0f;
inline float   g_ShopZoneHold      = 0.0f;
inline float   g_ShopReentryBlock  = 0.0f;   // 상점 닫은 직후 재진입 방지
inline long long g_RunGold         = 0;
inline int     g_RunShopStock[4]   = { -1, -1, -1, -1 };
inline int     g_RunShopPrice[4]   = { 0, 0, 0, 0 };

inline int RunShopPriceFor(AugRarity r) {
    switch (r) {
    case AugRarity::COMMON:    return 10;
    case AugRarity::RARE:      return 18;
    case AugRarity::EPIC:      return 32;
    case AugRarity::LEGENDARY: return 48;
    case AugRarity::MYTHIC:    return 65;
    default:                   return 20;
    }
}

inline void RefillRunShop() {
    g_GameManager.PickRunShopStock(g_RunShopStock, g_RunShopPrice, 4,
                                   false, false);
}

inline void BeginBossIntermission(int bossPick, long long goldBonus) {
    g_IntermissionTimer = INTERMISSION_DUR;
    g_ShopZoneHold      = 0.0f;
    g_ShopReentryBlock  = 0.0f;
    g_RunGold          += goldBonus;
    RefillRunShop();
    BossDir::OnBossDefeated();
    (void)bossPick;
}

inline void FinishBossKill(MonsterManager& mm, int bossPick, long long goldBonus) {
    mm.Clear();
    BeginBossIntermission(bossPick, goldBonus);
    g_GameManager.currentState = GameState::BOSS_INTERMISSION;
}

inline void EndIntermission() {
    g_IntermissionTimer = 0.0f;
    g_ShopZoneHold      = 0.0f;
    g_ShopReentryBlock  = 0.0f;
    g_GameManager.currentState = GameState::RUNNING;
}
