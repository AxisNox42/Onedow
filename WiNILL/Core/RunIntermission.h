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
constexpr float SHOP_ZONE_OFFSET_X = 200.0f;  // 휴식 시작 시 플레이어 위치에서 이격

inline float   g_IntermissionTimer = 0.0f;
inline float   g_ShopZoneHold      = 0.0f;
inline float   g_ShopReentryBlock  = 0.0f;   // 상점 닫은 직후 재진입 방지
inline float   g_ShopZoneX         = 0.0f;   // 월드 고정 (플레이어 따라다니지 않음)
inline float   g_ShopZoneY         = 0.0f;
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

inline void PlaceShopZone(float playerCx, float playerCy, int screenW, int screenH) {
    const float margin = 140.0f;
    float zx = playerCx + SHOP_ZONE_OFFSET_X;
    float zy = playerCy;
    float minX = margin + SHOP_ZONE_RADIUS;
    float maxX = (float)screenW - margin - SHOP_ZONE_RADIUS;
    float minY = margin + SHOP_ZONE_RADIUS;
    float maxY = (float)screenH - margin - SHOP_ZONE_RADIUS;
    if (maxX < minX) maxX = minX;
    if (maxY < minY) maxY = minY;
    if (zx < minX) zx = minX;
    if (zx > maxX) zx = maxX;
    if (zy < minY) zy = minY;
    if (zy > maxY) zy = maxY;
    g_ShopZoneX = zx;
    g_ShopZoneY = zy;
}

inline void BeginBossIntermission(int bossPick, long long goldBonus,
                                  float playerCx, float playerCy,
                                  int screenW, int screenH) {
    g_IntermissionTimer = INTERMISSION_DUR;
    g_ShopZoneHold      = 0.0f;
    g_ShopReentryBlock  = 0.0f;
    PlaceShopZone(playerCx, playerCy, screenW, screenH);
    g_RunGold          += goldBonus;
    RefillRunShop();
    BossDir::OnBossDefeated();
    (void)bossPick;
}

inline void FinishBossKill(MonsterManager& mm, int bossPick, long long goldBonus,
                           float playerCx, float playerCy, int screenW, int screenH) {
    mm.Clear();
    BeginBossIntermission(bossPick, goldBonus, playerCx, playerCy, screenW, screenH);
    g_GameManager.currentState = GameState::BOSS_INTERMISSION;
}

inline void EndIntermission() {
    g_IntermissionTimer = 0.0f;
    g_ShopZoneHold      = 0.0f;
    g_ShopReentryBlock  = 0.0f;
    g_GameManager.currentState = GameState::RUNNING;
}
