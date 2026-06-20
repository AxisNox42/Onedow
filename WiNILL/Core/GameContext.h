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

class ReloadRunnerBoss;
class PolymorphBoss;
class BotnetBoss;
class CentipedeBoss;
class TotemBoss;

extern GameManager         g_GameManager;
extern MonsterManager      g_MonsterManager;
extern std::vector<Bullet> g_Bullets;
extern PlayerStats         g_Stats;

extern TextRenderer g_TextL;
extern TextRenderer g_TextS;
extern TextRenderer g_TextXL;

extern ReloadRunnerBoss*   g_RRBoss;
extern PolymorphBoss*      g_PolyBoss;
extern BotnetBoss*         g_BotnetBoss;
extern CentipedeBoss*      g_CentiBoss;
extern TotemBoss*          g_TotemBoss;
// ── UI / 메뉴 씬에서 참조 (main.cpp 정의) ──
extern bool      g_LmbPrev;
extern GameState g_SettingsReturnTo;
extern int       g_WeaponChoices[3];
extern int       g_CurrentWeapon;
extern int       g_ConversionWeapon;
extern std::vector<int> g_OwnedAugs;
extern std::vector<int> g_CreativeStartAugList;
extern int       g_HoveredAug;
extern float     g_PrevHP;
extern int       g_BossRewardPicksLeft;
extern int       g_MetaStartAugs;
extern bool      g_LastRunRecord;
extern float     g_GameOverFade;
extern wchar_t   g_DeathReason[96];
extern bool      g_RunMelee;
extern bool      g_RunBow;
extern bool      g_DevUnlocked;
extern float     g_DevToastTimer;
extern bool      g_VolEdit;
extern wchar_t   g_VolBuf[8];
extern int       g_VolLen;
extern float     g_ScrollAccum;
