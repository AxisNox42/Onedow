#pragma once
#include "Augment.h"
#include "GameManager.h"
#include "BossDirector.h"
#include "MonsterManager.h"
#include <vector>

extern GameManager g_GameManager;

// ── 보스 클리어 후 휴식 구간 ──
constexpr float INTERMISSION_DUR     = 120.0f;
constexpr float SHOP_ZONE_RADIUS     = 115.0f;
constexpr float SHOP_ZONE_HOLD_S     = 0.35f;
constexpr float SKIP_ZONE_RADIUS     = 78.0f;
constexpr float SKIP_ZONE_HOLD_S     = 0.45f;
constexpr float RUN_SHOP_WIN_W       = 300.0f;
constexpr float RUN_SHOP_WIN_H       = 220.0f;

inline float   g_IntermissionTimer = 0.0f;
inline float   g_ShopZoneHold      = 0.0f;
inline float   g_SkipZoneHold      = 0.0f;
inline bool    g_ShopMustLeave     = false;   // 상점 나온 뒤 구역 밖으로 나가야 재입장
inline float   g_ShopZoneX         = 0.0f;
inline float   g_ShopZoneY         = 0.0f;
inline float   g_SkipZoneX         = 0.0f;
inline float   g_SkipZoneY         = 0.0f;
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

inline bool PlayerInZone(float px, float py, float zx, float zy, float r) {
    float dx = px - zx, dy = py - zy;
    return dx * dx + dy * dy <= r * r;
}

inline void PlaceIntermissionZones(int screenW, int screenH) {
    const float bar = 90.0f;
    float midY = ((float)screenH - bar) * 0.5f;
    g_ShopZoneX = (float)screenW * 0.5f;
    g_ShopZoneY = midY;
    g_SkipZoneX = (float)screenW - 190.0f;
    g_SkipZoneY = midY;
}

inline void CloseRunShop() {
    g_GameManager.currentState = GameState::BOSS_INTERMISSION;
    g_ShopMustLeave  = true;
    g_ShopZoneHold   = 0.0f;
}

inline void EndIntermission() {
    g_IntermissionTimer = 0.0f;
    g_ShopZoneHold      = 0.0f;
    g_SkipZoneHold      = 0.0f;
    g_ShopMustLeave     = false;
    g_GameManager.currentState = GameState::RUNNING;
}

inline void TickIntermissionZones(float pCX, float pCY, float delta) {
    if (g_ShopMustLeave) {
        if (!PlayerInZone(pCX, pCY, g_ShopZoneX, g_ShopZoneY, SHOP_ZONE_RADIUS))
            g_ShopMustLeave = false;
        g_ShopZoneHold = 0.0f;
    } else if (PlayerInZone(pCX, pCY, g_ShopZoneX, g_ShopZoneY, SHOP_ZONE_RADIUS)) {
        g_ShopZoneHold += delta;
        if (g_ShopZoneHold >= SHOP_ZONE_HOLD_S) {
            g_GameManager.currentState = GameState::RUN_SHOP;
            g_ShopZoneHold = 0.0f;
        }
    } else {
        g_ShopZoneHold = 0.0f;
    }

    if (PlayerInZone(pCX, pCY, g_SkipZoneX, g_SkipZoneY, SKIP_ZONE_RADIUS)) {
        g_SkipZoneHold += delta;
        if (g_SkipZoneHold >= SKIP_ZONE_HOLD_S)
            EndIntermission();
    } else {
        g_SkipZoneHold = 0.0f;
    }

    if (g_IntermissionTimer <= 0.0f)
        EndIntermission();
}

inline void BeginBossIntermission(int bossPick, long long goldBonus,
                                  int screenW, int screenH) {
    g_IntermissionTimer = INTERMISSION_DUR;
    g_ShopZoneHold      = 0.0f;
    g_SkipZoneHold      = 0.0f;
    g_ShopMustLeave     = false;
    PlaceIntermissionZones(screenW, screenH);
    g_RunGold          += goldBonus;
    RefillRunShop();
    BossDir::OnBossDefeated();
    (void)bossPick;
}

inline void FinishBossKill(MonsterManager& mm, int bossPick, long long goldBonus,
                           int screenW, int screenH) {
    mm.Clear();
    BeginBossIntermission(bossPick, goldBonus, screenW, screenH);
    g_GameManager.currentState = GameState::BOSS_INTERMISSION;
}
