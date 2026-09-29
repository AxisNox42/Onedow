#pragma once
// ─────────────────────────────────────────────────────────────
// GameContext — main.cpp 에 정의된 전역 상태의 extern 선언 허브.
// ─────────────────────────────────────────────────────────────
#include <vector>
#include "GameManager.h"
#include "MonsterManager.h"
#include "Bullet.h"
#include "PlayerStats.h"
#include "TextRenderer.h"
#include "Camera.h"

extern GameManager         g_GameManager;
extern MonsterManager      g_MonsterManager;
extern std::vector<Bullet> g_Bullets;
extern PlayerStats         g_Stats;

extern TextRenderer g_TextL;
extern TextRenderer g_TextS;
extern TextRenderer g_TextXL;

// ── UI / 메뉴 씬에서 참조 (main.cpp 정의) ──
extern bool      g_LmbPrev;
extern bool      g_RmbPrev;
extern GameState g_SettingsReturnTo;
extern int       g_CurrentWeapon;
extern int       g_ConversionWeapon;
extern std::vector<int> g_OwnedAugs;
extern int       g_HoveredAug;
extern float     g_PrevHP;
extern bool      g_LastRunRecord;
extern float     g_GameOverFade;
extern wchar_t   g_DeathReason[96];
extern bool      g_DevUnlocked;
extern float     g_DevToastTimer;
extern float     g_AugExitT;     // -1 = idle, 0..AUG_EXIT_DUR = exit anim in progress
extern int       g_AugExitSlot;  // which card slot was confirmed
extern float     g_RepExitT;     // AUG_REPLACE exit anim timer (-1 = idle)
extern int       g_RepExitSlot;  // -2 = cancel, 0+ = replace slot index
extern bool      g_VolEdit;
extern wchar_t   g_VolBuf[8];
extern int       g_VolLen;
extern float     g_ScrollAccum;
extern float     g_WindowSizeCur;
class SpatialBounds;
void ApplyAugmentSideEffects(AugType atype, int scrW, int scrH);
void SyncPlayerBoundsSize(SpatialBounds& bounds, float delta, bool animate);
