// Windows ?�더�?glad보다 먼�? ??APIENTRY 매크�?중복 ?�의 방�?
#ifdef _WIN32
  #include <windows.h>
  #include <dwmapi.h>   // DwmIsCompositionEnabled (진단??
  #include <timeapi.h>  // timeBeginPeriod / timeEndPeriod (FPS �??��???
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#ifdef _WIN32
  #define GLFW_EXPOSE_NATIVE_WIN32
  #include <GLFW/glfw3native.h>
#endif

#include "Platform.h"
#include "CrashHandler.h"

#include <glm/glm.hpp>
#include <iostream>
#include <vector>
#include <deque>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <cstring>
#include <cwctype>
#include <algorithm>
#include <functional>

#include "FakeWindow.h"
#include "GameManager.h"
#include "Monster.h"
#include "Bullet.h"
#include "MonsterManager.h"
#include "EnemyProgression.h"
#include "EnemySpawner.h"
#include "ProjectileSystem.h"
#include "AugmentSlots.h"
#include "Codex.h"
#include "CollisionSystem.h"
#include "Augment.h"
#include "PlayerStats.h"
#include "TextRenderer.h"
#include "PlanetShader.h"
#include "NebulaGlowShader.h"
#include "Settings.h"
#include "UiColors.h"
#include "ExpSystem.h"
#include "DrawPrim.h"
#include "WindowFx.h"
#include "Translations.h"
#include "Weapons.h"
#include "SaveSystem.h"
#include "IconSystem.h"
#include "Camera.h"
#include "BlurShader.h"
#include "MainShader.h"
#include "EntityDraw.h"
#include "PlayerWeaponShell.h"
#include "GameFonts.h"
#include "PlayerRuntime.h"
#include "Scenes.h"
#include "SceneContext.h"
#include "SceneInternal.h"
#include "SceneSkills.h"
#include "Input.h"
#include "DebugToolkit.h"
#include "UiLayout.h"
#include "SceneUI.h"
#include "WindowChrome.h"
#include "GameplayTelemetry.h"

#ifdef _MSC_VER
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib")   // RegOpenKeyExA / RegQueryValueExA / RegCloseKey
#pragma comment(lib, "winmm.lib")      // timeBeginPeriod / timeEndPeriod (FPS �??��???
// dwmapi.lib ?�?WindowFx.cpp ?�서 링크
#endif

#ifdef _MSC_VER
extern "C++" {
    __declspec(dllexport) DWORD NvOptimusEnablement = 0;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 0;
}
#endif

// ?�명???�퍼??WindowFx.h/cpp �??�동
// (EnableWindowTransparency ?�?TransparencyLog 가 ?�일 기능)
#define DBG TransparencyLog

GameManager    g_GameManager;
MonsterManager g_MonsterManager;
std::vector<Bullet> g_Bullets;

// ?�?�?개발???�리?�이?�브) 모드 ???�감 검?�창 ?�스?�에그로�??�금 ?�?�?
//    출시 빌드???�리?�이?�브 진입?�이 ?�겨???�고, ?�감 검?�에 ?�크�?코드�?//    ?�력?�면 ?�금?�어 ?�이???�면???��????�장?�다. (?�반 ?�레?�어??�?�?
bool    g_DevUnlocked   = false;
float   g_DevToastTimer = 0.0f;            // ?�금 ?�인 ?�스??(�?
// ?�정�?볼륨 ?�자 직접?�력 ?�태
bool    g_VolEdit = false;
wchar_t g_VolBuf[8] = {0};
int     g_VolLen = 0;
PlayerStats    g_Stats;
GameplayTelemetry g_GameplayTelemetry;
bool g_aug1Released = true, g_aug2Released = true, g_aug3Released = true;
float g_AugExitT    = -1.0f;
int   g_AugExitSlot = -1;
float g_RepExitT    = -1.0f;
int   g_RepExitSlot = -3;
TextRenderer   g_TextL;   // ??글??(증강 ?�름, ?�태 ?�?��?)
TextRenderer   g_TextS;   // ?��? 글??(?�명, ?�트)
TextRenderer   g_TextXL;  // 초�????�?��?(?�작�?로고) ?�용 ??고해?�도 ?�스??
#ifdef _WIN32
HANDLE         g_FontMemHandle   = nullptr;
#endif

struct ApproachOrb {
    float x = 0, y = 0;
};
std::vector<ApproachOrb> g_ApproachOrbs;

struct DroneState {
    float angle     = 0.0f;
    float fireTimer = 0.0f;
};
static const int MAX_DRONES = 4;
DroneState g_Drones[MAX_DRONES] = {};

//   1초마???�레?�어 ?�치??1개씩 배치, �?5�?지????맵에 ~5�??�시
// Window sizes for active ranged and signal frames.
float GENESIS_WIN_W = 300.0f;
float SWARM_WIN_W = 210.0f;
float g_RfwW = 500.0f, g_RfwH = 500.0f;

struct ChakramState {
    float angle        = 0.0f;
    float hp           = 150.0f;
    float maxHp        = 150.0f;
    bool  alive        = false;
    float respawnTimer = 0.0f;
};
static const int   MAX_CHAKRAMS    = 3;
ChakramState g_Chakrams[MAX_CHAKRAMS] = {};
static const float CHAKRAM_RADIUS = 130.0f;   // 버프: ?��? 공전 (공전�??�격)
static const float CHAKRAM_SIZE   = 30.0f;    // 버프: ??칼날

float g_BulletRainTimer = 0.0f;

// LIGHT_STEP ?�격 감�????�전 HP 기록
float g_PrevHP = 100.0f;

// 초당 EXP ?�적??(?�버?????��??�는 죽음, ?�몹 가??
float g_XpTimeAccum = 0.0f;

// 게임 ?�작 ??경과 ?�간 (?�폭�?보스 ?�장 ?�?�밍)
float g_GameTime         = 0.0f;

// ??�� 충격??(?�폭�??�폭 / 보스 ?�폰)
struct ShockWave {
    float x, y;
    float life, maxLife;
    float maxRadius;
    float r, g, b;
    bool  active  = false;
    bool  needsBg = false;
};
static const int MAX_SHOCKS = 8;
ShockWave g_ShockWaves[MAX_SHOCKS] = {};

static void SpawnShockWave(float x, float y, float maxR, float life,
                           float r, float g, float b,
                           bool needsBg = false) {
    for (int i = 0; i < MAX_SHOCKS; i++) {
        if (!g_ShockWaves[i].active) {
            g_ShockWaves[i] = { x, y, life, life, maxR, r, g, b, true, needsBg };
            return;
        }
    }
}

// 검�?근접 ?�윙 ?�상 (조�? 방향 ??
float g_MuzzleX = 0.0f, g_MuzzleY = 0.0f, g_MuzzleAng = 0.0f, g_MuzzleTimer = 0.0f;
// Static Field's weapon pulse is gameplay state, not just a player-shell
// visual.  Keeping its timer outside the render path makes the area weapon
// behave consistently at every frame rate and during fixed-step simulation.
float g_StaticFieldPulseTimer = 0.0f;
inline void TriggerMuzzle(float x, float y, float ang) {
    g_MuzzleX = x; g_MuzzleY = y; g_MuzzleAng = ang; g_MuzzleTimer = 0.05f;
}

// ?�면 ?�들�?(보스 ?�폰, 충격??
float g_ShakeTime = 0.0f;
float g_ShakeMag  = 0.0f;

// 보스 보상 ???��? 버프 ????(?�버???�이지 skip)
// ?�?�?배드 ?�터 ?�망 ?�류�????�시 감속 구역(?�상 ?�역). ?�에 ?�으�??�동?�도 -10% ?�?�?
//   즉시 ?�기지 ?�고 ZONE_OPEN(0.7�???걸쳐 ?�점 부?�되???�짐(grow factor = age/OPEN).
struct LaserBeam { float ox, oy, ex, ey, life, maxLife; float width = 1.0f; };
std::vector<LaserBeam> g_LaserBeams;
float          g_LaserTimer = 0.0f;
constexpr float LASER_INT   = 0.85f;  // 발사 주기(�? ???�프: 0.7


// ?�?�?백신 ?�캔 (증강) ??주기?�으�??�레?�어 주�????�화 ?�스(범위 ?�소) ?�?�?
//   ?�각 링�? 기존 SpawnShockWave(?�창 �? ?�사??

// ?�?�?보스 ?�장 ?�조(증상) ?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?
//   보스 ?�폰??"결정 ??2.5�??�조(?�마 증상 + 경고 배너) ???�제 ?�성" ?�로 분리.
//   ?�조 ?�안 게임?�레?�는 계속(?�레그래??. 만료 ??결정??보스�??�제�??�성.
bool  g_LastRunRecord   = false;  // 직전 ?�이 ?�기록이?�는지 (GAMEOVER ?�시??

float g_WindowSizeCur = 0.0f;

static float EffectivePlayerWinSize() {
    if (g_WindowSizeCur >= 64.0f) return g_WindowSizeCur;
    if (g_Stats.windowSize >= 64.0f) return g_Stats.windowSize;
    return std::max(400.0f * g_Scale, 64.0f);
}

static float ClampPlayerWinSize(float sz) {
    if (sz != sz || sz < 64.0f) sz = EffectivePlayerWinSize();
    return sz;
}

float g_WinPrevHP     = -1.0f;    // �?축소??HP 추적
float g_HpGhost       = -1.0f;    // Delayed gray HP afterimage
float g_HurtVignette  = 0.0f;     // ?�격 빨간 비네???�여
float g_HpBarPop      = 0.0f;     // ?�나비식 HP 게이지�????�격 ???�다가 ?�이??�?
float g_XpBarPop       = 0.0f;     // XP pickup feedback pulse

void SyncPlayerBoundsSize(SpatialBounds& pw, float delta, bool animate) {
    if (g_Stats.windowSize < 64.0f)
        g_Stats.windowSize = std::max(400.0f * g_Scale, 64.0f);
    float targetWin = g_Stats.windowSize *
        ((g_PlayerRuntime.hyperFocusTimer > 0.0f) ? 1.5f : 1.0f);
    if (g_WindowSizeCur < 64.0f) g_WindowSizeCur = g_Stats.windowSize;
    if (animate) {
        float winStep = std::min(1.0f, delta * 5.5f);
        g_WindowSizeCur += (targetWin - g_WindowSizeCur) * winStep;
    } else {
        g_WindowSizeCur = targetWin;
    }
    float pwSz = ClampPlayerWinSize(std::max(g_WindowSizeCur, 64.0f));
    float cx = pw.x + pw.width * 0.5f;
    float cy = pw.y + pw.height * 0.5f;
    if (cx != cx || cy != cy || pw.width < 64.0f || pw.height < 64.0f ||
        pw.width != pw.width || pw.height != pw.height) {
        cx = (float)screenWidth * 0.5f;
        cy = (float)screenHeight * 0.5f;
    }
    pw.width = pw.height = pwSz;
    pw.x = cx - pwSz * 0.5f;
    pw.y = cy - pwSz * 0.5f;
}

static void EnsurePlayerBounds(SpatialBounds& pw) {
    SyncPlayerBoundsSize(pw, 1.0f, false);
}

static void UpdateGravisFields(float& playerX, float& playerY,
                               bool playerControl, float dt) {
    constexpr float fieldRadius = 285.0f;
    const float fieldRadiusSq = fieldRadius * fieldRadius;
    for (auto* gravis : g_MonsterManager.monsters) {
        if (!gravis || !gravis->alive || gravis->kind != MobKind::GRAVIS) continue;

        const float gx = gravis->worldX - playerX;
        const float gy = gravis->worldY - playerY;
        const float playerDistanceSq = gx * gx + gy * gy;
        if (playerControl && !g_PlayerRuntime.dashActive &&
            playerDistanceSq > 36.0f && playerDistanceSq < fieldRadiusSq) {
            const float distance = sqrtf(playerDistanceSq);
            const float influence = 1.0f - distance / fieldRadius;
            const float pull = (24.0f + 58.0f * influence) * influence * dt;
            playerX += gx / distance * pull;
            playerY += gy / distance * pull;
        }

        for (auto& bullet : g_Bullets) {
            if (!bullet.active) continue;
            const float bx = gravis->worldX - bullet.x;
            const float by = gravis->worldY - bullet.y;
            const float distanceSq = bx * bx + by * by;
            if (distanceSq <= 64.0f || distanceSq >= fieldRadiusSq) continue;

            const float distance = sqrtf(distanceSq);
            const float influence = 1.0f - distance / fieldRadius;
            const float curve = (0.34f + 1.18f * influence) * dt;
            bullet.dirX += bx / distance * curve;
            bullet.dirY += by / distance * curve;
            const float directionLength = sqrtf(bullet.dirX * bullet.dirX + bullet.dirY * bullet.dirY);
            if (directionLength > 0.0001f) {
                bullet.dirX /= directionLength;
                bullet.dirY /= directionLength;
            }
        }
    }
}


#if defined(_DEBUG)
static constexpr bool kCompileDebugBuild = true;
#else
static constexpr bool kCompileDebugBuild = false;
#endif

// ?�?�??�티�??�킬 ?�스?????�??기본) + ?�롯 3�?증강 ?�득, �?차면 교체) ?�?�?
// HUD: ?�재 측정 FPS (?�단 ?�측 ?�시)
int    g_CurrentFPS  = 0;
double g_FpsLastTime = 0.0;
int    g_FpsFrames   = 0;

// ���� ���� �ε��� ���?(���� �������? �ߺ� ���� ����)
std::vector<int> g_OwnedAugs;

// ũ������Ƽ�� ���?���� ���� ���� ���� (�ε���). ���� ���� �� �ϰ� ����.
bool g_CreativeStartPending = false;   // ?�용 ?��?(main 루프가 applyByIdx �?처리)
std::deque<int> g_AugEffectQueue;
bool g_AugEffectProcessing = false;

// 증강 ?�택 hover state (-1 = 미선?? 0/1/2 = 카드 ?�덱??
int  g_HoveredAug    = -1;

// 마우???�릭 edge 감�? (?�전 ?�레??left button ?�태)
bool g_LmbPrev = false;
bool g_RmbPrev = false;

// PAUSED ?�태 ??보유 증강 ?�릭 ???�명 ?�시 (-1 = ?�음, 0..AUG_TOTAL-1 = ?�덱??
int g_PauseSelectedAug = -1;

// ?�정 ?�면 진입 ???�전 ?�태 (?�로 가�???복�?)
GameState g_SettingsReturnTo = GameState::MAIN_MENU;

int g_CurrentWeapon = -1;

// 변??카드 ??AUG_SELECT ??25% ?�률�?4번째 카드 ?�장
// ?�재 무기�??�른 StartWeapon ?�로 ?�환 (기존 무기 ?�과 ?�거 ????무기 ?�용)
// �?= ?�환??StartWeapon ?�덱?? -1 = ?�번 ?�운?�는 변??카드 ?�음
int g_ConversionWeapon = -1;

// DYING ?�망 ?�출 ?�태
float g_DyingTimer    = 0.0f;
bool  g_DeathBoomDone = false;
float g_DeathWinW0    = 0.0f;
float g_GameOverFade  = 0.0f;    // 게임?�버 메뉴 ?�이?�인 (0??, ?�?�크�???2.5�?
// ?�망 ?�출 = 즉시 ?�??��(?�티?? ????�� ?�파 ??GAMEOVER (�??�네마틱 ?�음, ?�순)
static const float DYING_DUR      = 0.8f;   // ??�� ?�파가 ?�생?�는 ?�간 (?�이???�까지)
static const float GAMEOVER_FADE  = 2.5f;   // �޴� 100%���� �ɸ��� �ð�

struct DeathParticle {
    float x, y, vx, vy;
    float size;       // ??변 길이(px)
    float r, g, b;    // ?�상
    bool  active = false;
};
static const int MAX_DEBRIS = 48;
DeathParticle g_Debris[MAX_DEBRIS] = {};
float g_DeathCX = 0, g_DeathCY = 0;
float g_DeathFlash = 0.0f; // ??�� ?�광 (1.0 ??0.0)
wchar_t g_DeathReason[96] = {0};   // ?�망 ?�인 ("?�○ ???�해 종료??)

static SpatialBounds* s_PlayerBoundsRef = nullptr;

static void InitChakramsFromStats() {
    if (!g_Stats.chakram || g_Stats.chakramCount <= 0) return;
    for (int c = 0; c < g_Stats.chakramCount && c < MAX_CHAKRAMS; c++) {
        g_Chakrams[c].maxHp = 150.0f;
        if (!g_Chakrams[c].alive && g_Chakrams[c].respawnTimer <= 0) {
            g_Chakrams[c].alive        = true;
            g_Chakrams[c].hp           = 150.0f;
            g_Chakrams[c].respawnTimer = 0.0f;
        }
        g_Chakrams[c].angle =
            (float)c / (float)g_Stats.chakramCount * 6.2831853f;
    }
}

void ApplyAugmentSideEffects(AugType atype, int scrW, int scrH) {
    float oldWS  = s_PlayerBoundsRef ? s_PlayerBoundsRef->width : g_Stats.windowSize;
    float oldPCX = s_PlayerBoundsRef
        ? s_PlayerBoundsRef->x + s_PlayerBoundsRef->width  * 0.5f
        : (float)scrW * 0.5f;
    float oldPCY = s_PlayerBoundsRef
        ? s_PlayerBoundsRef->y + s_PlayerBoundsRef->height * 0.5f
        : (float)scrH * 0.5f;

    if (atype == AugType::CHAKRAM || atype == AugType::CHAKRAM_2 ||
        atype == AugType::CHAKRAM_3 || atype == AugType::CHAKRAM_SINGULARITY) {
        InitChakramsFromStats();
    }
    if (atype == AugType::D_APPROACH && g_ApproachOrbs.empty()) {
        ApproachOrb orb;
        int edge = rand() % 4;
        if      (edge == 0) { orb.x = (float)(rand() % scrW);  orb.y = -40.0f; }
        else if (edge == 1) { orb.x = (float)(rand() % scrW);  orb.y = scrH + 40.0f; }
        else if (edge == 2) { orb.x = -40.0f;                  orb.y = (float)(rand() % scrH); }
        else                { orb.x = (float)scrW + 40.0f;     orb.y = (float)(rand() % scrH); }
        g_ApproachOrbs.push_back(orb);
    }
    if (g_Stats.windowSize != oldWS && s_PlayerBoundsRef) {
        s_PlayerBoundsRef->width  = g_Stats.windowSize;
        s_PlayerBoundsRef->height = g_Stats.windowSize;
        s_PlayerBoundsRef->x = oldPCX - g_Stats.windowSize * 0.5f;
        s_PlayerBoundsRef->y = oldPCY - g_Stats.windowSize * 0.5f;
        g_WindowSizeCur = g_Stats.windowSize;
    }
}

static void RebuildPlayerStatsFromOwned(int scrW, int scrH, bool preserveRuntime = true) {
    int weapon = g_CurrentWeapon;
    std::vector<int> owned = g_OwnedAugs;
    const long long keepKillCount = g_Stats.killCount;
    const int keepVampireKillStreak = g_Stats.vampireKillStreak;
    const int keepWarlordStacks = g_Stats.warlordStacks;
    const bool keepMk2Used = g_Stats.mk2Used;
    const float keepLightStepDisableTimer = g_Stats.lightStepDisableTimer;

    g_Stats = PlayerStats();
    ApplyMeta(g_Stats);
    g_Stats.windowSize *= g_Scale;
    if (weapon >= 0 && weapon < (int)StartWeapon::_COUNT)
        ApplyWeapon(g_Stats, (StartWeapon)weapon);
    g_Stats.baseFireInterval = g_Stats.fireInterval;

    memset(g_TypeOwned, 0, sizeof(g_TypeOwned));
    // takenOnce is a per-run acquisition ledger.  Rebuilding derived stats
    // must not reopen cards that were already selected earlier in this run.

    g_OwnedAugs.clear();
    for (int d = 0; d < MAX_DRONES;   d++) g_Drones[d]   = DroneState{};
    for (int c = 0; c < MAX_CHAKRAMS; c++) g_Chakrams[c] = ChakramState{};
    g_LaserBeams.clear();
    g_ApproachOrbs.clear();
    ResetPlayerSkills();

    for (int idx : owned) {
        if (idx < 0 || idx >= AUG_TOTAL) continue;
        AugType atype = ALL_AUGS[idx].type;
        g_Stats.Apply(atype);
        if (ALL_AUGS[idx].rarity == AugRarity::COMMON)
            g_Stats.ApplyCommonMultBoost();
        g_OwnedAugs.push_back(idx);
        g_TypeOwned[(int)atype] = true;
        EquipSkill(SkillForAug(atype));
        if (AugOnceOnly(atype, ALL_AUGS[idx].rarity))
            g_GameManager.takenOnce[idx] = true;
        ApplyAugmentSideEffects(atype, scrW, scrH);
    }
    if (g_Stats.chakram) InitChakramsFromStats();
    ReequipSkillsFromOwned(g_OwnedAugs.data(), (int)g_OwnedAugs.size());

    if (preserveRuntime) {
        g_Stats.killCount = keepKillCount;
        g_Stats.vampireKillStreak = keepVampireKillStreak;
        if (g_Stats.warlord)
            g_Stats.RestoreWarlordStacks(keepWarlordStacks);
        g_Stats.mk2Used = keepMk2Used;
        g_Stats.lightStepDisableTimer = keepLightStepDisableTimer;
    }

    g_GameManager.maxHP = g_Stats.maxHP;
    if (g_GameManager.playerHP > g_Stats.maxHP)
        g_GameManager.playerHP = g_Stats.maxHP;
}

static void ApplySingleAugIdx(int idx, int scrW, int scrH) {
    if (idx < 0 || idx >= AUG_TOTAL) return;
    AugType atype = ALL_AUGS[idx].type;
    const PlayerStats statsBefore = g_Stats;
    g_Stats.Apply(atype);
    if (ALL_AUGS[idx].rarity == AugRarity::COMMON)
        g_Stats.ApplyCommonMultBoost();
    g_OwnedAugs.push_back(idx);
    g_TypeOwned[(int)atype] = true;
    MarkAugSeen(idx);
    EquipSkill(SkillForAug(atype));
    if (AugOnceOnly(atype, ALL_AUGS[idx].rarity))
        g_GameManager.takenOnce[idx] = true;
    if (g_GameManager.playerHP > g_Stats.maxHP)
        g_GameManager.playerHP = g_Stats.maxHP;
    ApplyAugmentSideEffects(atype, scrW, scrH);
    g_GameplayTelemetry.RecordAugment(g_GameTime,
                                      g_GameManager.playerLevel,
                                      idx,
                                      statsBefore,
                                      g_Stats);
}

static void BeginAugReplaceFlow(int newIdx) {
    g_GameManager.pendingAugIdx      = newIdx;
    g_GameManager.replaceChoiceCount = 0;
    for (int oi : g_OwnedAugs) {
        if (!AugIdxUsesIdentitySlot(oi)) continue;
        if (g_GameManager.replaceChoiceCount >= 32) break;
        g_GameManager.replaceChoices[g_GameManager.replaceChoiceCount++] = oi;
    }
    g_HoveredAug = -1;
    g_GameManager.augmentKeyboardFocus = false;
    g_GameManager.currentState = GameState::AUG_REPLACE;
    ++g_GameManager.augmentSelectionSerial;
}

static void ProcessAugEffectQueue(int scrW, int scrH);
static void FinishAugmentEffects();

static void PushAugEffectsFront(const int* effects, int count) {
    for (int i = count - 1; i >= 0; --i) {
        if (effects[i] >= 0 && effects[i] < AUG_TOTAL)
            g_AugEffectQueue.push_front(effects[i]);
    }
}

static void ResetAugmentDerivedRuntime() {
    g_ApproachOrbs.clear();
    for (int d = 0; d < MAX_DRONES; d++) g_Drones[d] = DroneState{};
    for (int c = 0; c < MAX_CHAKRAMS; c++) g_Chakrams[c] = ChakramState{};
    g_LaserBeams.clear();
    ResetPlayerSkills();
}

static void FinishCurrentAugmentReward() {
    if (g_GameManager.augmentRewardActive &&
        !g_GameManager.augmentRewardQueue.empty()) {
        g_GameManager.augmentRewardQueue.pop_front();
    }
    g_GameManager.augmentRewardActive = false;
    g_GameManager.augmentRewardInternalDebuff = false;
    g_GameManager.augmentRewardInDebuff = false;
    if (!g_GameManager.ActivateNextAugmentReward()) {
        g_GameManager.currentState = GameState::RUNNING;
        g_PlayerRuntime.postPickGrace = 0.5f;
    }
}

static void FinishAugmentEffects() {
    if (g_GameManager.augmentRewardActive &&
        !g_GameManager.augmentRewardQueue.empty()) {
        const AugmentRewardEntry& entry = g_GameManager.augmentRewardQueue.front();
        if (entry.needsDebuff && !g_GameManager.augmentRewardInternalDebuff &&
            !g_Stats.mk2SkipDebuff) {
            g_GameManager.PickDebuffChoices();
            if (g_GameManager.augChoiceCount > 0) {
                g_GameManager.augmentRewardInDebuff = true;
                ++g_GameManager.augmentSelectionSerial;
                g_GameManager.currentState = GameState::DEBUFF_SELECT;
                g_HoveredAug = -1;
                return;
            }
        }
        FinishCurrentAugmentReward();
        return;
    }

    g_GameManager.currentState = GameState::RUNNING;
    g_PlayerRuntime.postPickGrace = 0.5f;
}

static void ProcessAugEffectQueue(int scrW, int scrH) {
    while (!g_AugEffectQueue.empty()) {
        const int idx = g_AugEffectQueue.front();
        g_AugEffectQueue.pop_front();
        if (idx < 0 || idx >= AUG_TOTAL) continue;

        const AugType atype = ALL_AUGS[idx].type;
        MarkAugSeen(idx);

        if (atype == AugType::S_CHAOS) {
            const int prevAugs = std::min((int)g_OwnedAugs.size(), 60);
            const long long keepKillCount = g_Stats.killCount;
            const int keepVampireKillStreak = g_Stats.vampireKillStreak;
            const bool keepMk2Used = g_Stats.mk2Used;
            const float keepLightStepDisableTimer = g_Stats.lightStepDisableTimer;
            const float oldWindowSize = g_Stats.windowSize;

            ResetAugmentDerivedRuntime();
            g_Stats = PlayerStats();
            ApplyMeta(g_Stats);
            g_Stats.windowSize *= g_Scale;
            if (g_CurrentWeapon >= 0 && g_CurrentWeapon < (int)StartWeapon::_COUNT)
                ApplyWeapon(g_Stats, (StartWeapon)g_CurrentWeapon);
            g_Stats.baseFireInterval = g_Stats.fireInterval;
            g_Stats.killCount = keepKillCount;
            g_Stats.vampireKillStreak = keepVampireKillStreak;
            g_Stats.mk2Used = keepMk2Used;
            g_Stats.lightStepDisableTimer = keepLightStepDisableTimer;

            g_OwnedAugs.clear();
            // Preserve takenOnce: this is a run-wide acquisition ledger.
            memset(g_TypeOwned, 0, sizeof(g_TypeOwned));
            g_GameManager.maxHP = g_Stats.maxHP;
            if (g_GameManager.playerHP > g_Stats.maxHP)
                g_GameManager.playerHP = g_Stats.maxHP;
            if (s_PlayerBoundsRef && oldWindowSize != g_Stats.windowSize) {
                const float cx = s_PlayerBoundsRef->x + s_PlayerBoundsRef->width * 0.5f;
                const float cy = s_PlayerBoundsRef->y + s_PlayerBoundsRef->height * 0.5f;
                s_PlayerBoundsRef->width = g_Stats.windowSize;
                s_PlayerBoundsRef->height = g_Stats.windowSize;
                s_PlayerBoundsRef->x = cx - g_Stats.windowSize * 0.5f;
                s_PlayerBoundsRef->y = cy - g_Stats.windowSize * 0.5f;
            }
            g_WindowSizeCur = g_Stats.windowSize;

            const int nDebuffs = std::max(0, (prevAugs * 2 + 2) / 5);
            const int nBuffs = prevAugs - nDebuffs;
            int buffs[64] = {}, debuffs[64] = {};
            const int gotBuffs = g_GameManager.PickRandomAugIndices(
                buffs, nBuffs, false, true, false, false);
            const int gotDebuffs = g_GameManager.PickRandomDebuffIndices(
                debuffs, nDebuffs);
            if (gotDebuffs > 0)
                g_GameManager.augmentRewardInternalDebuff = true;
            PushAugEffectsFront(debuffs, gotDebuffs);
            PushAugEffectsFront(buffs, gotBuffs);
            continue;
        }

        if (atype == AugType::S_PANDORA) {
            int buffs[3] = {}, debuffs[2] = {};
            const int gotBuffs = g_GameManager.PickRandomAugIndices(
                buffs, 3, g_Stats.sizeAugTaken,
                true, false, false);
            const int gotDebuffs = g_GameManager.PickRandomDebuffIndices(
                debuffs, 2);
            if (gotDebuffs > 0)
                g_GameManager.augmentRewardInternalDebuff = true;
            PushAugEffectsFront(debuffs, gotDebuffs);
            PushAugEffectsFront(buffs, gotBuffs);
            continue;
        }

        if (atype == AugType::RANDOM_AUG) {
            int picks[3] = {};
            const int got = g_GameManager.PickRandomAugIndices(
                picks, 3, g_Stats.sizeAugTaken,
                true, false, false);
            PushAugEffectsFront(picks, got);
            continue;
        }

        if (NeedsReplaceForAug(idx)) {
            BeginAugReplaceFlow(idx);
            return;
        }
        ApplySingleAugIdx(idx, scrW, scrH);
    }
}

static void CompleteAugReplaceFlow(int replaceSlot, int scrW, int scrH) {
    if (replaceSlot < 0 || replaceSlot >= g_GameManager.replaceChoiceCount) return;
    int oldIdx = g_GameManager.replaceChoices[replaceSlot];
    int newIdx = g_GameManager.pendingAugIdx;

    for (auto it = g_OwnedAugs.begin(); it != g_OwnedAugs.end(); ++it) {
        if (*it == oldIdx) { g_OwnedAugs.erase(it); break; }
    }
    // Keep the acquisition ledger latched.  Replacing a card removes its
    // current effect, but a one-time card must never return in this run.
    g_TypeOwned[(int)ALL_AUGS[oldIdx].type] = false;

    RebuildPlayerStatsFromOwned(scrW, scrH);
    ApplySingleAugIdx(newIdx, scrW, scrH);
    g_GameManager.maxHP = g_Stats.maxHP;
    g_GameManager.pendingAugIdx = -1;
    g_GameManager.replaceChoiceCount = 0;

    if (g_AugEffectProcessing) {
        ProcessAugEffectQueue(scrW, scrH);
        if (g_GameManager.currentState == GameState::AUG_REPLACE) return;
        g_AugEffectProcessing = false;
    }
    FinishAugmentEffects();
}

static void CancelAugReplaceFlow() {
    g_GameManager.pendingAugIdx = -1;
    g_GameManager.replaceChoiceCount = 0;
    if (g_AugEffectProcessing) {
        ProcessAugEffectQueue(g_GameManager.screenW, g_GameManager.screenH);
        if (g_GameManager.currentState == GameState::AUG_REPLACE) return;
        g_AugEffectProcessing = false;
    }
    FinishAugmentEffects();
}



static void BeginGameplayTelemetryIfNeeded() {
    if (!g_DebugMode || !g_BalanceTestMode) return;
    if (g_GameplayTelemetry.IsActive()) return;

    const GameState state = g_GameManager.currentState;
    const bool inRunFlow = state == GameState::READY ||
                           state == GameState::RUNNING ||
                           state == GameState::PAUSED ||
                           state == GameState::AUG_SELECT ||
                           state == GameState::DEBUFF_SELECT;
    if (!inRunFlow) return;

    // FinalizeLoadout() runs from the scene layer.
    const bool hasLoadout = g_CurrentWeapon >= 0 || !g_OwnedAugs.empty();
    if (!hasLoadout) return;

    g_GameplayTelemetry.BeginRun(
        g_GameTime,
        g_GameManager.playerLevel,
        g_GameManager.xp,
        g_Stats,
        g_CreativeMode ? "creative" : "normal");
}

static void SyncGameplayTelemetry() {
    if (!g_DebugMode || !g_BalanceTestMode) {
        if (g_GameplayTelemetry.IsActive()) {
            g_GameplayTelemetry.EndRun(
                g_GameTime,
                g_GameManager.playerLevel,
                g_GameManager.xp,
                g_Stats,
                g_DebugMode ? "balance_test_disabled" : "debug_mode_disabled");
        }
        return;
    }
    BeginGameplayTelemetryIfNeeded();
}

int main() {
    CrashHandler::Install();   // 강종(E23) 추적 ??처리 ?????�외 ??로그+미니?�프
    srand((unsigned)time(NULL));

    // ?�행 ?�일 ?�더�??�업 ?�렉?�리 ?�동 (Resource/ ?��?경로 로드 보장)
    PlatformChdirToExeDir();
    LoadGame();   // �����?����/���?�ҷ����� (������ �⺻�� ����)
#if !defined(_DEBUG)
    // DEBUG MODE is intentionally available in Release packages for QA.
    // Keep balance telemetry opt-in to debug builds so the QA toggle does not
    // enable release telemetry by itself.
    g_BalanceTestMode = false;
#endif
#if defined(__APPLE__)
    // ���� �� ���� 1ȸ: CRT OFF + VFX �淮 (�������� �ٽ� �� �� ����)
    if (!g_MacOptV1) {
        g_MacOptV1 = true;
        g_ShaderFx = false;
        if (g_VfxDensity == VfxDensity::FULL)
            g_VfxDensity = VfxDensity::REDUCED;
        SaveGame();
    }
#endif
    ApplyAccentTheme();   // �����?�׼�Ʈ �׸� �� g_Accent* �ݿ�

    if (!glfwInit()) return -1;
    // Sleep ?�상??1ms �?(FPS �??��??�용). Windows �??��? ?�음
    PlatformTimerBegin();

    GLFWmonitor*       monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode    = glfwGetVideoMode(monitor);
    screenWidth  = mode->width;
#ifdef _WIN32
    // Keep the window one pixel below the monitor height.  An exact-size,
    // unoccluded borderless window can enter DirectFlip and bypass DWM,
    // breaking the transparent backdrop blur. Taskbar visibility is handled
    // separately through ITaskbarList2::MarkFullscreenWindow.
    screenHeight = mode->height - 1;
#else
    screenHeight = mode->height;
#endif

#if defined(__APPLE__)
    {
        int wx = 0, wy = 0, ww = 0, wh = 0;
        glfwGetMonitorWorkarea(monitor, &wx, &wy, &ww, &wh);
        if (ww > 0 && wh > 0) {
            screenWidth  = ww;
            screenHeight = wh;
            if (mode->height > wh)
                g_TaskbarH = mode->height - (wy + wh);
        }
    }
#endif

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,        GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    // macOS ??3.2+ Core ?�서 forward-compatible 컨텍?�트 ?�수
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    // ?�티??HiDPI) 2× 백버???�기 ???�레?�버??= �??�기(?? 1:1
    //   좌표/?�크issor/마우?��? ?��? ???�위�??�치 ??Windows ?�??�일?�게 ?�작
    //   (???�면 게임???�면 좌하??1/4 ?�만 그려지�??�릭 ?�치가 ?�긋??
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
#endif
    glfwWindowHint(GLFW_DECORATED,             GLFW_FALSE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    // GLFW_FLOATING ?�거: WS_EX_TOPMOST + ?�체?�면 ?�기 조합??DWM ?�립 모드�??�발
    // ??DWM 컴포지???�회 ???�파 ?�명??무효?? TOPMOST ?�이 ?�성 ???�동?�로 ?�정.
    glfwWindowHint(GLFW_RESIZABLE,             GLFW_FALSE);
    glfwWindowHint(GLFW_ALPHA_BITS,            8);
    // Text and thin constellation rules are rendered from glyph textures and
    // line primitives.  Request a modest MSAA surface so the same UI path
    // remains readable on integrated and discrete adapters alike; drivers
    // that cannot provide it simply fall back to the default framebuffer.
    // The game is a full-screen desktop overlay. 2x MSAA keeps thin UI and
    // circular VFX clean while avoiding the 4x full-screen sample cost.
    glfwWindowHint(GLFW_SAMPLES,                2);

    // Keep one pixel of desktop composition available on Windows so DWM
    // continues to composite the transparent framebuffer and blur backdrop.
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight,
                                          "Onedow", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }

#if defined(__APPLE__)
    {
        int wx = 0, wy = 0, ww = 0, wh = 0;
        glfwGetMonitorWorkarea(monitor, &wx, &wy, &ww, &wh);
        if (ww > 0 && wh > 0)
            glfwSetWindowPos(window, wx, wy);
        else
            glfwSetWindowPos(window, 0, 0);
    }
#else
    glfwSetWindowPos(window, 0, 0);
#endif

    // ?�단 ?�업?�시�??�이 계산 ???�?�크�??�에 ???�는 ?�업?�시줄에 ?�단 UI 가
    //   가?��?지 ?�도�? (?�업 ?�역???�면보다 ?�으�?�?차이가 ?�업?�시�??�이)
#ifdef _WIN32
    // The Shell keeps the taskbar behind our active borderless window.
    g_TaskbarH = 0;
#endif

    // ?�상??기�? ?��??????��? ?�면?�서 �??�티?��? 비�? 축소?�도�?계산 ???�괄 ?�용.
    //   (?��? ?��??�라 ?��? ?�면?�수�??��??�으�?컸던 문제 ?�결 + ?�체?�으�?�?축소)
    g_Scale = (float)screenHeight / SCALE_REF_H;
    if (g_Scale > 1.0f) g_Scale = 1.0f;
    if (g_Scale < 0.5f) g_Scale = 0.5f;
    GENESIS_WIN_W *= g_Scale;
    SWARM_WIN_W    *= g_Scale;
    g_RfwW *= g_Scale; g_RfwH *= g_Scale;
    glfwMakeContextCurrent(window);
    InputRegisterCallbacks(window);
    glfwSwapInterval((g_FpsCap == 0) ? 1 : 0);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;
    GLint msaaSamples = 0;
    glGetIntegerv(GL_SAMPLES, &msaaSamples);
    if (msaaSamples > 0)
        glEnable(GL_MULTISAMPLE);

    // ��Ʈ: exe ��ġ�� ���� Resource ���?��ΰ�?�޶��� �� �־� ���� �����ϴ� ��θ�?������.
    InitializeGameFonts(screenWidth, screenHeight);

    BatchFlush(); glEnable(GL_BLEND);
    // RGB: ?��? ?�파블렌??/ Alpha: ?�레?�버???�파�??�바르게 ?�적
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                        GL_ONE,       GL_ONE_MINUS_SRC_ALPHA);

    // ?�이�??�토그램) ?�스�??�이?�라??+ 증강 ?�이�?로드
    InitIconGL();
    LoadIcons();
    InitRainMissileTex();   // ?�환 ?��? 로켓 ?�프?�이???�배�??�거+?�트 가??

    EnableWindowTransparency(window);

    // --- �?GL/?��??�트�?종합 진단 (Windows ?�용) ---
#ifdef _WIN32
    {
        HWND hWnd = glfwGetWin32Window(window);
        // GL 3.3 Core Profile: GL_ALPHA_BITS deprecated ??glGetFramebufferAttachmentParameteriv ?�용
        GLint alphaBits = 0, rBits = 0, gBits = 0, bBits = 0;
        glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_BACK_LEFT,
            GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE, &alphaBits);
        glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_BACK_LEFT,
            GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE,   &rBits);
        glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_BACK_LEFT,
            GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE, &gBits);
        glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_BACK_LEFT,
            GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE,  &bBits);
        const char* renderer = (const char*)glGetString(GL_RENDERER);
        const char* glVer    = (const char*)glGetString(GL_VERSION);
        const char* vendor   = (const char*)glGetString(GL_VENDOR);
        // AMD/ATI 감�? ??AMD ?�라?�버??GLSL 컴파?�이 ???�격?�고 ?�명 FBO 처리가
        //   NVIDIA ?�??�라, 벤더�?로그�??�겨 검?�?�면/?�명?�패 ?�인 추적???�용.
        bool isAMD = false;
        if (vendor) {
            std::string vlow = vendor;
            for (auto& ch : vlow) ch = (char)tolower((unsigned char)ch);
            isAMD = (vlow.find("amd") != std::string::npos) ||
                    (vlow.find("ati") != std::string::npos) ||
                    (vlow.find("advanced micro") != std::string::npos);
        }

        // Windows ?�명???�과 ?�정 (?��??�트�?
        DWORD enableTrans = 1;
        HKEY hKey = nullptr;
        if (RegOpenKeyExA(HKEY_CURRENT_USER,
            "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
            0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD sz = sizeof(DWORD);
            RegQueryValueExA(hKey, "EnableTransparency", nullptr, nullptr,
                             (LPBYTE)&enableTrans, &sz);
            RegCloseKey(hKey);
        }

        BOOL dwmOn = FALSE;
        DwmIsCompositionEnabled(&dwmOn);
        LONG ex    = GetWindowLong(hWnd, GWL_EXSTYLE);
        int  trans = glfwGetWindowAttrib(window, GLFW_TRANSPARENT_FRAMEBUFFER);

        DBG("=== 종합 진단 ===\n");
        DBG("  [GL] Vendor           : %s%s\n", vendor ? vendor : "NULL",
            isAMD ? "  (AMD 감�? ???�격 GLSL/?�명 FBO 경로)" : "");
        DBG("  [GL] Renderer         : %s\n", renderer ? renderer : "NULL");
        DBG("  [GL] Version          : %s\n", glVer    ? glVer    : "NULL");
        DBG("  [GL] Framebuffer bits : R=%d G=%d B=%d A=%d\n",
            rBits, gBits, bBits, alphaBits);
        DBG("       ??(Core Profile ?�확??쿼리. A=0?�면 ?�라?�버가 ?�파 채널 거�?)\n");
        DBG("\n");
        DBG("  [GLFW] transparent_framebuffer : %d [%s]\n",
            trans, trans ? "SUPPORTED" : "NOT SUPPORTED");
        DBG("  [DWM]  Composition enabled     : %s\n", dwmOn ? "YES" : "NO");
        DBG("  [Win]  GWL_EXSTYLE             : 0x%08lX\n", ex);
        DBG("  [Win]  WS_EX_LAYERED           : %s\n",
            (ex & WS_EX_LAYERED)     ? "SET"           : "NOT SET");
        DBG("  [Win]  WS_EX_TRANSPARENT       : %s\n",
            (ex & WS_EX_TRANSPARENT) ? "SET (?�릭?�과!)" : "NOT SET (?�상)");
        DBG("\n");
        DBG("  [REG]  EnableTransparency      : %lu [%s]\n",
            enableTrans,
            enableTrans ? "ON (?�상)"
                        : "OFF ???�게 문제! ?�정>개인?�정>???�명???�과 켜기");
        DBG("=================\n\n");
    }
#endif // _WIN32 (진단 블록)

    InitMainShaderPipeline(screenWidth, screenHeight);
    InitMainBatchGeometry(screenWidth, screenHeight);
    InitPlanetShader(screenWidth, screenHeight);
    InitNebulaGlowShader(screenWidth, screenHeight);

    // --- 게임 ?�브?�트 초기??---
    SpatialBounds playerWin(
        (screenWidth  - g_Stats.windowSize) * 0.5f,
        (screenHeight - g_Stats.windowSize) * 0.5f,
        g_Stats.windowSize, g_Stats.windowSize);

    const float PLAYER_SIZE = 25.0f;
    const float MOVE_SPEED  = 400.0f;

    g_GameManager.Init(screenWidth, screenHeight);

    // Runtime debug toolkit bindings. The toolkit is inert unless the
    // persisted DEBUG MODE setting is enabled, but its callbacks are wired
    // once here so the normal game loop remains the single owner of state.
    g_DebugToolkit.Configure({
        &g_Stats,
        &g_GameManager,
        &g_MonsterManager,
        &playerWin,
        &g_WindowSizeCur,
        screenWidth,
        screenHeight,
        &g_BalanceTestMode,
        [&]() {
            if (!std::isfinite(g_Stats.maxHP) || g_Stats.maxHP < 1.0f)
                g_Stats.maxHP = 1.0f;
            if (!std::isfinite(g_Stats.windowSize) || g_Stats.windowSize < 64.0f)
                g_Stats.windowSize = 64.0f;
            g_GameManager.maxHP = g_Stats.maxHP;
            if (!std::isfinite(g_GameManager.playerHP))
                g_GameManager.playerHP = g_Stats.maxHP;
            g_GameManager.playerHP = std::max(0.0f,
                                               std::min(g_Stats.maxHP,
                                                        g_GameManager.playerHP));
            g_GameManager.scoreAccum = (float)g_GameManager.score;
            const float cx = playerWin.x + playerWin.width * 0.5f;
            const float cy = playerWin.y + playerWin.height * 0.5f;
            playerWin.width  = g_Stats.windowSize;
            playerWin.height = g_Stats.windowSize;
            playerWin.x = cx - playerWin.width * 0.5f;
            playerWin.y = cy - playerWin.height * 0.5f;
            g_WindowSizeCur = g_Stats.windowSize;
            if (g_Stats.chakram) InitChakramsFromStats();
            else for (int i = 0; i < MAX_CHAKRAMS; ++i)
                g_Chakrams[i] = ChakramState{};
        },
        [&](int mobKind, int count) {
            if (mobKind < 0 || mobKind > (int)MobKind::QUASAR) return;
            for (int i = 0; i < count; ++i) {
                const size_t before = g_MonsterManager.monsters.size();
                g_MonsterManager.SpawnMob(screenWidth, screenHeight, 160,
                                          std::max(0.01f, g_Stats.monsterHpMult));
                if (g_MonsterManager.monsters.size() <= before) break;
                Monster* mob = g_MonsterManager.monsters.back();
                mob->MakeKind((MobKind)mobKind);
                if (mob->kind == MobKind::ROTOR)
                    mob->hp *= g_Stats.rotorHpMult;
            }
        },
        [&](int count) {
            for (int i = 0; i < count; ++i)
                g_MonsterManager.SpawnRangedMob(
                    screenWidth, screenHeight,
                    std::max(0.01f, g_Stats.rmobHpMult), 160);
        }
    });

    float lastFrame        = 0.0f;
    float fireTimer        = g_Stats.fireInterval;  // ready to fire immediately
    float accumulator      = 0.0f;
    const float FIXED_DT = 1.0f / 60.0f;

    Audio::Init(g_AudioMonoOutput);   // Uses the saved stereo/mono output preference.
    g_AudioMonoOutput = Audio::IsMonoOutput();
    Audio::SetBgmEnabled(g_BgmEnabled);
    Audio::SetSfxEnabled(g_SfxEnabled);

    // ============================================================
    // 메인 루프
    // ============================================================
    while (!glfwWindowShouldClose(window)) {
        s_PlayerBoundsRef = &playerWin;
        float now   = (float)glfwGetTime();
        float delta = now - lastFrame;
        if (delta > 0.1f) delta = 0.1f;
        lastFrame = now;

        // ?�래??추적 브레?�크????마�?�??�태�??�겨 강종(E23) ??로그�??�치 ?�정
        {
            char bc[200];
            std::snprintf(bc, sizeof(bc),
                "st=%d score=%lld lv=%d mobs=%u",
                (int)g_GameManager.currentState,
                (long long)g_GameManager.score,
                g_GameManager.playerLevel,
                (unsigned)g_MonsterManager.monsters.size());
            CrashHandler::SetBreadcrumb(bc);
        }

        // �??�커?��? ?�으�??�른 ?�으�??�환) ?�동 ?�시?��? ??        //   ?�버?�이???�커???�어??루프가 계속 ?��?�? ??막으�?게임??        //   백그?�운?�에??계속 진행??콤보 3�?창이 ?�러가 리셋?�는 버그 ??.
        const InputFocusTransition focus = InputUpdateWindowFocus(window);
        const bool inputFocusChanged = focus.lost || focus.gained;
        if (inputFocusChanged) {
            ResetPlayerInputEdges();
            g_aug1Released = g_aug2Released = g_aug3Released = true;
        }
        if (focus.lost) {
            g_GameManager.spaceReleased = true;
            g_GameManager.escReleased = true;
            if (g_GameManager.currentState == GameState::RUNNING) {
                g_GameManager.pauseResumeState = GameState::RUNNING;
                g_GameManager.currentState = GameState::PAUSED;
            }
        }
        if (focus.gained) {
            g_GameManager.spaceReleased = true;
            g_GameManager.escReleased = true;
        }

        // FPS 측정 (1�??�위)
        ++g_FpsFrames;
        if (now - g_FpsLastTime >= 1.0f) {
            g_CurrentFPS  = g_FpsFrames;
            g_FpsFrames   = 0;
            g_FpsLastTime = now;
        }

        glfwPollEvents();
        UpdateWindowTaskbarPolicy(window);
        if (inputFocusChanged) {
            g_GameManager.spaceReleased =
                glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE;
            g_GameManager.escReleased =
                glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_RELEASE;
            g_aug1Released = glfwGetKey(window, GLFW_KEY_1) == GLFW_RELEASE;
            g_aug2Released = glfwGetKey(window, GLFW_KEY_2) == GLFW_RELEASE;
            g_aug3Released = glfwGetKey(window, GLFW_KEY_3) == GLFW_RELEASE;
            g_PlayerRuntime.dashInputHeld =
                keys[GLFW_KEY_LEFT_SHIFT] || keys[GLFW_KEY_RIGHT_SHIFT];
            g_PlayerRuntime.skillInputHeld[0] = keys[GLFW_KEY_Q];
            g_PlayerRuntime.skillInputHeld[1] = keys[GLFW_KEY_E];
            g_PlayerRuntime.skillInputHeld[2] = keys[GLFW_KEY_R];
        }

        // �?부?�럽�?보간 (?�이�? ?�면 ?�장 ??
        //   ?�투/?�투?�버?�이(RUNNING/DYING/PAUSED/증강·?�버???�택)?�선 �??��? ??        //   ?�시?��? ??줌인?�는 �?부?�연?�럽?�는 ?�드�? �??�제???�작�???        // Restore the base view after leaving the active combat states.
        {
            GameState zs = g_GameManager.currentState;
            bool inFight = (zs == GameState::RUNNING || zs == GameState::DYING ||
                            zs == GameState::PAUSED  || zs == GameState::AUG_SELECT ||
                            zs == GameState::DEBUFF_SELECT);
            if (!inFight) { g_ViewZoom = g_ViewZoomTarget = 1.0f; }
            else {
                // ?�수 비�? 줌아?????�장???�서???�어�?(?�한 1.4�?= zoom 0.714).
                //   200�� ������ �ִ�ġ ����.
                float t = std::min(1.0f, (float)g_GameManager.score / 2000000.0f);
                g_ViewZoomTarget = 1.0f - 0.286f * t;   // 1.0 �� 0.714
            }
        }
        g_ViewZoom += (g_ViewZoomTarget - g_ViewZoom) * std::min(1.0f, delta * 4.0f);

        // ?�장 ?�레????줌아?�된 만큼 보이???�역???�어지므�??�티??배회/?�폰 ?�역???�장
        //   (?�수 줌아?�·폴리모??줌아??공통 처리)
        {
            float zb = (g_ViewZoom < 0.01f) ? 0.01f : g_ViewZoom;
            g_ArenaExX = (float)screenWidth  * 0.5f * (1.0f / zb - 1.0f);
            g_ArenaExY = (float)screenHeight * 0.5f * (1.0f / zb - 1.0f);
        }

        // 마우???�태 (mx,my = ?�면 ?��? / wmx,wmy = �?보정???�드 좌표 = 조�???
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        const bool rawLmb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        const bool rawRmb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
        if (inputFocusChanged) {
            g_LmbPrev = rawLmb;
            g_RmbPrev = rawRmb;
        }
        bool lmb = rawLmb;
        float wmx = ScreenToWorldX((float)mx);
        float wmy = ScreenToWorldY((float)my);

        // --- ?�력 처리 ---
        GameState prevState = g_GameManager.currentState;
        const bool debugInputCaptured = g_DebugToolkit.BeginInput(
            window, (float)screenWidth, (float)screenHeight, mx, my,
            lmb, g_LmbPrev);
        if (!debugInputCaptured) {
            g_GameManager.HandleInput(window);
        } else {
            // Opening the toolkit also pauses simulation and consumes the
            // pointer so gameplay and scene controls cannot leak through.
            delta = 0.0f;
            lmb = false;
            wmx = ScreenToWorldX(-10000.0f);
            wmy = ScreenToWorldY(-10000.0f);
        }

        // Keep the OS pointer out of the combat view while the custom
        // crosshair is active. Pause/settings screens use normal input, so
        // the pointer must remain available for their clickable controls.
        InputUpdateGameplayCursor(window, g_GameManager.currentState,
                                  g_ShowCrosshair, g_DebugToolkit.IsVisible());

        // ??게임 리셋 ?�다 (GAMEOVER ??READY, ?�이???�택 ?? ?�시?�기 버튼 ?�에???�출)
        auto ResetForNewGame = [&]() {
            g_GameplayTelemetry.EndRun(g_GameTime,
                                       g_GameManager.playerLevel,
                                       g_GameManager.xp,
                                       g_Stats,
                                       "reset_or_restart");
            g_Stats        = PlayerStats();
            ApplyMeta(g_Stats);
            g_Stats.windowSize *= g_Scale;           // ?�레?�어 �????�상??비�? 축소
            g_ApproachOrbs.clear();
            for (int d = 0; d < MAX_DRONES;   d++) g_Drones[d]   = DroneState{};
            for (int c = 0; c < MAX_CHAKRAMS; c++) g_Chakrams[c] = ChakramState{};
                                    g_BulletRainTimer = 0.0f;
            g_PrevHP          = g_Stats.maxHP;
            g_XpTimeAccum     = 0.0f;
            g_OwnedAugs.clear();
            memset(g_TypeOwned, 0, sizeof(g_TypeOwned));
            g_CreativeGodmode  = false;
            g_CreativeFreeGrab = false;
            g_HoveredAug       = -1;
            g_ConversionWeapon = -1;
            g_CurrentWeapon    = -1;
            g_PauseSelectedAug = -1;
            g_GameTime         = 0.0f;
            g_GameManager.ClearAugmentRewardQueue();
            g_AugEffectQueue.clear();
            g_AugEffectProcessing = false;
            for (int i = 0; i < MAX_SHOCKS; i++) g_ShockWaves[i].active = false;
            g_MuzzleTimer = 0.0f;
            g_StaticFieldPulseTimer = 0.0f;
            g_ShakeTime = 0.0f; g_ShakeMag = 0.0f;
            ResetTrials();
            g_RunStardust       = 0;
            g_LaserBeams.clear(); g_LaserTimer = 0.0f;
            g_StardustPickups.clear(); g_StardustHudPulse = 0.0f;
            g_XpBarPop = 0.0f;

            ResetJuice();
            ResetPlayerSkills();
            g_WindowSizeCur = g_Stats.windowSize;
            g_WinPrevHP = -1.0f; g_HpGhost = -1.0f;
            g_PlayerDamagePulse = 0.0f;
            g_HurtVignette = 0.0f; g_HpBarPop = 0.0f;
            g_ViewZoom = g_ViewZoomTarget = 1.0f;   // �??�복
            g_ZoomCX = g_ZoomCY = 0.0f;
            g_Difficulty = Difficulty::NORMAL;
            // Keep the first ranged-mob interval active from run start so its
            // silhouette can be inspected without a score-based warm-up.
            ResetEnemySpawnState();
            g_DyingTimer      = 0.0f;
            g_DeathBoomDone   = false;
            g_GameOverFade    = 0.0f;
            g_DeathFlash      = 0.0f;
            for (int i = 0; i < MAX_DEBRIS; i++) g_Debris[i].active = false;
            fireTimer = g_Stats.fireInterval;
            playerWin.width  = g_Stats.windowSize;
            playerWin.height = g_Stats.windowSize;
            playerWin.x = (screenWidth  - g_Stats.windowSize) * 0.5f;
            playerWin.y = (screenHeight - g_Stats.windowSize) * 0.5f;
            const long long initialScore = g_CreativeMode
                ? g_CreativeStartScore : 0;
            g_GameManager.ResetRunProgress(g_MonsterManager, g_Bullets,
                                           100.0f, initialScore);
        };

        // Restart the current run without returning to the PLAY screen.
        // Keep the selected loadout and run modifiers, then rebuild the
        // combat state so the next frame starts from a clean arena.
        auto RestartCurrentRun = [&]() {
            const int savedWeapon = g_CurrentWeapon;
            const bool savedGodmode = g_CreativeGodmode;
            const std::vector<int> savedOwnedAugs = g_OwnedAugs;
            const bool savedTrialPoolReady = g_TrialPoolReady;
            int savedTrialPool[TRIAL_SLOT_COUNT] = {};
            bool savedTrialSelected[TRIAL_SLOT_COUNT] = {};
            for (int i = 0; i < TRIAL_SLOT_COUNT; ++i) {
                savedTrialPool[i] = g_TrialPool[i];
                savedTrialSelected[i] = g_TrialSelected[i];
            }

            // This clears enemies, projectiles, drops,
            // timers, run currencies, score/XP and the death presentation.
            ResetForNewGame();

            // Restore the setup that ResetForNewGame intentionally clears.
            g_CurrentWeapon = savedWeapon;
            g_Difficulty = Difficulty::NORMAL;
            g_CreativeGodmode = savedGodmode;
            g_OwnedAugs = savedOwnedAugs;
            g_TrialPoolReady = savedTrialPoolReady;
            for (int i = 0; i < TRIAL_SLOT_COUNT; ++i) {
                g_TrialPool[i] = savedTrialPool[i];
                g_TrialSelected[i] = savedTrialSelected[i];
            }

            // Re-applying the owned augment list restores the exact build,
            // including class/creative augments and their side effects.
            RebuildPlayerStatsFromOwned(screenWidth, screenHeight, false);
            const float trialHpMul = TrialPlayerMaxHpMult();
            if (trialHpMul < 0.999f)
                g_Stats.maxHP *= trialHpMul;
            g_GameManager.maxHP = g_Stats.maxHP;
            g_GameManager.playerHP = g_Stats.maxHP;

            g_CreativeStartPending = false;
            g_GameManager.conversionAug = -1;
            g_GameManager.hoveredCard = -1;
            g_GameManager.pendingAugIdx = -1;
            g_GameManager.replaceChoiceCount = 0;
            g_GameManager.augChoiceCount = 0;
            for (int i = 0; i < 3; ++i) g_GameManager.augChoices[i] = -1;
            g_AugExitT = -1.0f;
            g_AugExitSlot = -1;
            g_RepExitT = -1.0f;
            g_RepExitSlot = -3;
            g_GameManager.ClearAugmentRewardQueue();
            g_AugEffectQueue.clear();
            g_AugEffectProcessing = false;
            g_GameManager.spaceReleased = true;
            g_GameManager.escReleased = true;
            g_GameManager.pauseResumeState = GameState::RUNNING;
            g_GameManager.currentState = GameState::RUNNING;
            g_GameManager.lastState = GameState::RUNNING;
        };

        // End the current run without rebuilding it. Abandoning from pause or
        // settings should use the same report screen as a normal death while
        // preserving the score, build, level, kills, and earned stardust.
        auto AbandonCurrentRun = [&]() {
            if (g_GameManager.currentState == GameState::GAMEOVER)
                return;

            g_GameplayTelemetry.EndRun(g_GameTime,
                                       g_GameManager.playerLevel,
                                       g_GameManager.xp,
                                       g_Stats,
                                       "abandoned");

            g_MonsterManager.Clear();
            g_Bullets.clear();
                    g_LaserBeams.clear();

            g_ApproachOrbs.clear();
            g_StardustPickups.clear();
            g_MuzzleTimer = 0.0f;
            g_ShakeTime = 0.0f;
            g_ShakeMag = 0.0f;
            for (int d = 0; d < MAX_DRONES;   ++d) g_Drones[d]   = DroneState{};
            for (int c = 0; c < MAX_CHAKRAMS; ++c) g_Chakrams[c] = ChakramState{};
            for (int i = 0; i < MAX_SHOCKS; ++i) g_ShockWaves[i].active = false;
            ResetJuice();

            g_GameManager.playerHP = 0.0f;
            g_GameManager.pauseResumeState = GameState::RUNNING;
            g_DyingTimer = 0.0f;
            g_DeathBoomDone = true;
            g_DeathFlash = 0.0f;
            g_GameOverFade = 0.0f;
            Audio::StopBgm();

            const int li = std::max(0, std::min(2, LangIndex()));
            const wchar_t* abandoned[3] = {
                L"플레이 포기", L"PLAY ABANDONED", L"プレイを放棄"
            };
            wcscpy_s(g_DeathReason, abandoned[li]);

            if (!g_CreativeMode) {
                g_LastRunRecord = RecordRunResult(
                    (int)g_Difficulty,
                    g_GameManager.score,
                    g_Stats.killCount,
                    (g_SelectedJob == JOB_STATIC_FIELD ? 1 : 0));
                if (g_TotalGames >= 10) TryUnlockAch(ACH_GAMES_10);
                if (g_AchSaveNeeded) {
                    SaveGame();
                    g_AchSaveNeeded = false;
                }
            } else {
                g_LastRunRecord = false;
                g_LastRunCoins = 0;
            }

            g_GameManager.currentState = GameState::GAMEOVER;
        };

        // GAMEOVER?�READY ?�동 감�? (ESC ?�으�?직접 ?�환??경우)
        if (prevState == GameState::GAMEOVER &&
            g_GameManager.currentState == GameState::READY) {
            ResetForNewGame();
        }
        // PAUSED ???�른 ?�태: 증강 ?�명 박스 ?�기
        if (prevState == GameState::PAUSED &&
            g_GameManager.currentState != GameState::PAUSED) {
            g_PauseSelectedAug = -1;
        }
        if (!debugInputCaptured)
            g_GameManager.UpdateStateSystem(g_MonsterManager, g_Bullets);
        SyncGameplayTelemetry();
        if (prevState == GameState::RUNNING &&
            g_GameManager.currentState == GameState::AUG_SELECT &&
            !g_GameManager.augmentRewardActive) {
            if (!g_GameManager.ActivateNextAugmentReward())
                g_GameManager.currentState = GameState::RUNNING;
        }

        // Keep the native window title synchronized with the active scene and
        // the live run counters.  This is intentionally throttled so title
        // updates do not compete with rendering, while still making the
        // borderless window useful in task switching and diagnostics.
        static double lastTitleUpdate = -1.0;
        const double titleNow = glfwGetTime();
        if (g_GameManager.currentState != prevState ||
            lastTitleUpdate < 0.0 || titleNow - lastTitleUpdate >= 0.25) {
            lastTitleUpdate = titleNow;
            const char* scene = "Menu";
            switch (g_GameManager.currentState) {
            case GameState::READY:          scene = "Ready"; break;
            case GameState::RUNNING:        scene = "In Game"; break;
            case GameState::PAUSED:         scene = "Paused"; break;
            case GameState::DYING:          scene = "Defeated"; break;
            case GameState::AUG_SELECT:     scene = "Augment Select"; break;
            case GameState::AUG_REPLACE:    scene = "Augment Replace"; break;
            case GameState::DEBUFF_SELECT:  scene = "Modifier Select"; break;
            case GameState::GAMEOVER:       scene = "Game Over"; break;
            case GameState::CODEX:          scene = "Codex"; break;
            case GameState::TUTORIAL:       scene = "Tutorial"; break;
            case GameState::SETTINGS:       scene = "Settings"; break;
            case GameState::CREATIVE_CONFIG:scene = "Creative Config"; break;
            default:                         break;
            }
            std::string title = std::string("Onedow - ") + scene;
            if (g_GameManager.currentState == GameState::RUNNING ||
                g_GameManager.currentState == GameState::DYING ||
                g_GameManager.currentState == GameState::PAUSED ||
                g_GameManager.currentState == GameState::AUG_SELECT ||
                g_GameManager.currentState == GameState::DEBUFF_SELECT) {
                title += " | FPS " + std::to_string(g_CurrentFPS);
                title += " | HP " + std::to_string((int)std::max(0.0f, g_GameManager.playerHP));
                title += " | XP " + std::to_string(g_GameManager.xp);
                title += " | Score " + std::to_string(g_GameManager.score);
                if (g_GameManager.currentState == GameState::AUG_SELECT ||
                    g_GameManager.currentState == GameState::DEBUFF_SELECT)
                    title += " | Choices " + std::to_string(g_GameManager.augChoiceCount);
            }
            glfwSetWindowTitle(window, title.c_str());
        }

        // Keep the main loop music active during gameplay and stop it in menus.
        {
            Audio::SetEnabled(g_AudioEngineEnabled && g_SoundVol > 0);
            Audio::SetBgmEnabled(g_BgmEnabled);
            Audio::SetSfxEnabled(g_SfxEnabled);
            Audio::SetVolume(g_SoundVol / 100.0f);        // 마스??볼륨
            GameState cs = g_GameManager.currentState;
            bool bgmOn = (cs == GameState::RUNNING || cs == GameState::PAUSED ||
                          cs == GameState::AUG_SELECT || cs == GameState::DEBUFF_SELECT ||
                          cs == GameState::READY);
            if (bgmOn) Audio::PlayBgmMain();
            else       Audio::StopBgm();
        }

        // ?�리?�이?�브 무적 ??�??�레??체력 ?�?고정 (?��? 죽�? ?�음)
        if (g_CreativeGodmode && (g_CreativeMode || g_DebugMode) &&
            g_GameManager.currentState == GameState::RUNNING) {
            g_GameManager.playerHP = g_Stats.maxHP;
        }

        // HP 0 ??MK2 부??OR DYING (1�??�로??모션 ??GAMEOVER)
        if (g_GameManager.playerHP <= 0.0f &&
            g_GameManager.currentState == GameState::RUNNING) {
            // MK2: 1??부????공격??비�? ??�� + ?�?HP (?�널???�음)
            if (g_Stats.mk2 && !g_Stats.mk2Used) {
                g_Stats.mk2Used = true;
                float pCX = playerWin.x + playerWin.width  * 0.5f;
                float pCY = playerWin.y + playerWin.height * 0.5f;
                float blastDmg = g_Stats.GetBaseDamage()
                               * g_Stats.GetDamageMultiplier() * 6.0f;
                float blastRad = 280.0f;
                // 주�? ?�에�??��?지
                for (auto m : g_MonsterManager.monsters) {
                    if (!m->alive) continue;
                    float dx = m->worldX - pCX, dy = m->worldY - pCY;
                    if (dx*dx + dy*dy < blastRad * blastRad) {
                        m->hp -= blastDmg;
                        if (m->hp <= 0) m->alive = false;
                    }
                }
                for (auto rm : g_MonsterManager.rangedMobs) {
                    if (!rm->alive) continue;
                    float dx = rm->worldX - pCX, dy = rm->worldY - pCY;
                    if (dx*dx + dy*dy < blastRad * blastRad) {
                        rm->hp -= blastDmg;
                        if (rm->hp <= 0) rm->alive = false;
                    }
                }
                // �ð� ȿ�� ? ���� + �����?+ ȭ�� ����
                SpawnEnemyExplosion(pCX, pCY, 1.0f, 0.8f, 0.3f, true);
                SpawnEnemyExplosion(pCX, pCY, 0.4f, 1.0f, 0.8f, true);
                SpawnShockWave(pCX, pCY, blastRad * 1.4f, 0.55f,
                               1.0f, 0.85f, 0.3f);
                g_ShakeTime = 0.45f; g_ShakeMag = 22.0f;
                TriggerFlash(1.0f, 0.9f, 0.4f, 0.8f);   // 부????강한 번쩍
                TriggerHitStop(0.14f);                  // ?�팩???��?

                // 부?????�력�??�널???�이 ?�?HP �?(?�버???�음)
                g_GameManager.maxHP    = g_Stats.maxHP;
                g_GameManager.playerHP = g_Stats.maxHP;
                g_PrevHP               = g_GameManager.playerHP;
                // RUNNING ?�태 ?��? (DYING ?�이 X)
            } else {
                // ?�반 ?�망 ???�레?�어 기점 ?�??��(모든 ???�짐) ??메뉴 ?�이?�인
                g_GameManager.currentState = GameState::DYING;
                g_GameManager.playerHP     = 0.0f;
                Audio::StopBgm();
                float pCX = playerWin.x + playerWin.width  * 0.5f;
                float pCY = playerWin.y + playerWin.height * 0.5f;
                g_DeathCX = pCX; g_DeathCY = pCY;
                g_DyingTimer    = DYING_DUR;
                g_DeathBoomDone = false;
                g_DeathFlash    = 0.0f;
                // ?�리모프 ??�??�복 (?�망 ?�출??�??�음)
                g_ZoomCX = g_ZoomCY = 0.0f;
                g_ViewZoom = 1.0f; g_ViewZoomTarget = 1.0f;
                // ?�망 ?�인 ???�티????�� ?? 가??가까운 ?�협(?�로?�스)??기록 (결과창에 ?�시??
                {
                    float best = 1e18f; const wchar_t* nm = nullptr;
                    auto consider = [&](float ex, float ey, const wchar_t* n) {
                        float dx = ex-pCX, dy = ey-pCY, d = dx*dx+dy*dy;
                        if (d < best) { best = d; nm = n; }
                    };
                    for (auto m  : g_MonsterManager.monsters)   if (m->alive)  consider(m->worldX,  m->worldY,  MobName((int)m->kind));
                    for (auto r  : g_MonsterManager.rangedMobs) if (r->alive)  consider(r->worldX,  r->worldY,  MobName(CM_SCOPE));
                    int li = LangIndex();
                    const wchar_t* FMT[3] = { L"%ls: signal dispersed", L"Dispersed by %ls", L"%ls dispersed" };
                    const wchar_t* UNK[3] = { L"Unknown error", L"Terminated by unknown error", L"Unknown error" };
                    if (nm) swprintf_s(g_DeathReason, FMT[li], nm);
                    else    wcscpy_s(g_DeathReason, UNK[li]);
                }
            }   // close else (MK2 분기 ??
        }       // close outer if (HP <= 0)

        // DYING ?�망 ?�출 ???�레?�어 기점 ?�??��(모든 ???�짐) ??GAMEOVER (�?변???�음)
        if (g_GameManager.currentState == GameState::DYING) {
            g_DyingTimer -= delta;
            g_DeathFlash -= delta * 4.0f;
            if (g_DeathFlash < 0.0f) g_DeathFlash = 0.0f;

            // ?�??�� ???�레?�어 기점, 모든 ?�이 ?�짐 (즉시, �??�네마틱 ?�음)
            if (!g_DeathBoomDone) {
                g_DeathBoomDone = true;
                float pCX = g_DeathCX, pCY = g_DeathCY;
                // 모든 ????��?�키�??�거 (scored/noBlast ?�시 ???�수/?�쇄 ?�산 ????
                for (auto m : g_MonsterManager.monsters) if (m->alive) {
                    SpawnEnemyExplosion(m->worldX, m->worldY, m->color.r, m->color.g, m->color.b, true);
                    m->alive = false; m->scored = true; m->noBlast = true;
                }
                for (auto r : g_MonsterManager.rangedMobs) if (r->alive) {
                    SpawnEnemyExplosion(r->worldX, r->worldY, r->color.r, r->color.g, r->color.b, true);
                    r->alive = false; r->scored = true;
                }
                // 모든 ?�·보?�·분?�체·총알·?�탑??즉시 ?�전 ??�� ??UpdateAll(RUNNING ?�용)??                //   맡기�?DYING/GAMEOVER ?�안 ?�거�?�?�??�이 ?��? ?�태�??�으므�??�기???�거.
                g_MonsterManager.Clear();   // monsters, ranged mobs, and projectiles are deleted and cleared
                g_Bullets.clear();
                g_LaserBeams.clear();   // ?�캔 ?�이?�?�??�리
                   // 배드 ?�터 감속 구역/출혈 ?�리
                // ?�레?�어 중심 ?�??�� + 충격??+ ?�광 + ?�들�?+ 방사???�편
                for (int k = 0; k < 4; k++)
                    SpawnEnemyExplosion(pCX, pCY, 1.0f, 0.85f - (k%2)*0.4f, 0.3f, true);
                SpawnShockWave(pCX, pCY, 900.0f, 0.7f, 0.4f, 0.9f, 1.0f);
                SpawnShockWave(pCX, pCY, 560.0f, 0.55f, 1.0f, 0.9f, 0.4f);
                g_DeathFlash = 1.0f;
                g_ShakeTime = 0.5f; g_ShakeMag = 30.0f;
                TriggerFlash(0.6f, 0.85f, 1.0f, 0.8f);
                for (int i = 0; i < MAX_DEBRIS; i++) {
                    float angle = (float)i / (float)MAX_DEBRIS * 2.0f * (float)M_PI
                                + ((float)(rand() % 100) - 50.0f) * 0.01f;
                    float spd = 450.0f + (float)(rand() % 650);
                    float sz  = 8.0f + (float)(rand() % 24);
                    int   tint = rand() % 10;
                    float cr, cg, cb;
                    if      (tint < 5) { cr = 0.10f; cg = 0.85f; cb = 1.00f; }
                    else if (tint < 8) { cr = 1.00f; cg = 1.00f; cb = 1.00f; }
                    else               { cr = 1.00f; cg = 0.85f; cb = 0.20f; }
                    g_Debris[i] = { pCX, pCY, cosf(angle)*spd, sinf(angle)*spd, sz, cr, cg, cb, true };
                }
            }

            float sd = delta;
            for (int i = 0; i < MAX_DEBRIS; i++) {
                if (!g_Debris[i].active) continue;
                g_Debris[i].x  += g_Debris[i].vx * sd;
                g_Debris[i].y  += g_Debris[i].vy * sd;
                g_Debris[i].vx *= (1.0f - 1.6f * sd);
                g_Debris[i].vy *= (1.0f - 1.6f * sd);
            }
            if (g_DyingTimer <= 0.0f) {
                g_GameplayTelemetry.EndRun(g_GameTime,
                                           g_GameManager.playerLevel,
                                           g_GameManager.xp,
                                           g_Stats,
                                           "gameover");
                g_ViewZoom = g_ViewZoomTarget = 1.0f;   // �??�복 (메뉴 ?�상??
                g_ZoomCX = g_ZoomCY = 0.0f;             // �?중심 ?�면 중앙?�로
                g_GameManager.currentState = GameState::GAMEOVER;
                g_DyingTimer = 0.0f;
                g_GameOverFade = 0.0f;   // 결과 메뉴 ?�이?�인 ?�작 (2.5�?
                // 기록 ?�?????�이?�별 최고???�적/코인 갱신 (?�기록이�??�시)
                //   ?�리?�이?�브 모드(?�드박스)??코인·기록 ?�외 (?�밍 방�?)
                if (!g_CreativeMode) {
                    g_LastRunRecord = RecordRunResult((int)g_Difficulty,
                                                      g_GameManager.score,
                                                      g_Stats.killCount,
                                                      (g_SelectedJob == JOB_STATIC_FIELD ? 1 : 0));
                    if (g_TotalGames >= 10) TryUnlockAch(ACH_GAMES_10);
                    if (g_AchSaveNeeded) { SaveGame(); g_AchSaveNeeded = false; }
                } else {
                    g_LastRunRecord = false;
                    g_LastRunCoins  = 0;
                }
            }
        }

        if (g_GameManager.currentState == GameState::GAMEOVER && g_GameOverFade < 1.0f) {
            g_GameOverFade += delta / GAMEOVER_FADE;
            if (g_GameOverFade > 1.0f) g_GameOverFade = 1.0f;
        }

        // ?�망 ??�� ?�여�??�티?�·파?�·충격파·?�파????게임?�버 ?�면??굳어버리지 ?�도�?        //   DYING/GAMEOVER ?�안?�도 계속 갱신???�연?�럽�??�라지�??�다.
        {
            GameState gvs = g_GameManager.currentState;
            if (gvs == GameState::DYING || gvs == GameState::GAMEOVER) {
                for (auto& p : g_EnemyParts) {
                    if (!p.active) continue;
                    p.life -= delta;
                    if (p.life <= 0.0f) { p.active = false; continue; }
                    p.x += p.vx * delta; p.y += p.vy * delta;
                    p.vx *= (1.0f - 2.5f * delta); p.vy *= (1.0f - 2.5f * delta);
                }
                for (auto& sw : g_ShockWaves) { if (!sw.active) continue; sw.life -= delta; if (sw.life <= 0.0f) sw.active = false; }
                                for (auto& sp : g_Sparks) {
                    sp.x += sp.vx * delta; sp.y += sp.vy * delta;
                    float drag = std::max(0.0f, 1.0f - 6.0f * delta);
                    sp.vx *= drag; sp.vy *= drag; sp.life -= delta;
                }
                g_Sparks.erase(std::remove_if(g_Sparks.begin(), g_Sparks.end(),
                    [](const Spark& s){ return s.life <= 0.0f; }), g_Sparks.end());
                if (gvs == GameState::GAMEOVER) {
                    for (int i = 0; i < MAX_DEBRIS; i++) { if (!g_Debris[i].active) continue;
                        g_Debris[i].x += g_Debris[i].vx * delta; g_Debris[i].y += g_Debris[i].vy * delta;
                        g_Debris[i].vx *= (1.0f - 1.6f * delta); g_Debris[i].vy *= (1.0f - 1.6f * delta);
                    }
                }
            }
        }

        // --- AUG_SELECT / DEBUFF_SELECT: 1/2/3 focus, SPACE confirm ---
        static bool s_augSpaceReleased = true;
        if (inputFocusChanged)
            s_augSpaceReleased = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE;
        {
            const int spaceState = glfwGetKey(window, GLFW_KEY_SPACE);
            if (spaceState == GLFW_RELEASE) s_augSpaceReleased = true;
        }
        if (!debugInputCaptured &&
            (g_GameManager.currentState == GameState::AUG_SELECT ||
             g_GameManager.currentState == GameState::DEBUFF_SELECT)) {
            const int k1 = glfwGetKey(window, GLFW_KEY_1);
            const int k2 = glfwGetKey(window, GLFW_KEY_2);
            const int k3 = glfwGetKey(window, GLFW_KEY_3);            // Special effects are processed by ProcessAugEffectQueue below.
            auto applyAug = [&](int slot) {
                if (slot < 0 || slot >= g_GameManager.augChoiceCount) return;
                const bool wasBuff = (g_GameManager.currentState == GameState::AUG_SELECT);
                const int selectedIdx = g_GameManager.augChoices[slot];
                if (selectedIdx < 0 || selectedIdx >= AUG_TOTAL) return;
                g_AugEffectQueue.push_back(selectedIdx);
                g_AugEffectProcessing = true;
                ProcessAugEffectQueue(screenWidth, screenHeight);
                if (g_GameManager.currentState == GameState::AUG_REPLACE)
                    return;
                g_AugEffectProcessing = false;
                g_GameManager.maxHP = g_Stats.maxHP;
                if (g_GameManager.augmentRewardInDebuff) {
                    FinishCurrentAugmentReward();
                } else if (wasBuff) {
                    FinishAugmentEffects();
                } else {
                    g_GameManager.currentState = GameState::RUNNING;
                    g_PlayerRuntime.postPickGrace = 0.5f;
                }
            };

            constexpr float AUG_EXIT_DUR = 1.70f;  // 공전 흡수(0~0.65) + 슈퍼노바(0.65~1.20) + 여유

            // exit anim timer -- fires applyAug when done
            bool augExitFired = false;
            if (g_AugExitT >= 0.0f) {
                g_AugExitT += delta;
                if (g_AugExitT >= AUG_EXIT_DUR) {
                    applyAug(g_AugExitSlot);
                    g_HoveredAug  = -1;
                    g_AugExitT    = -1.0f;
                    g_AugExitSlot = -1;
                    s_augSpaceReleased      = false;
                    g_GameManager.spaceReleased = false;
                    augExitFired = true;
                }
            }

            if (!augExitFired) {
                // 1/2/3 focuses a card; SPACE starts the confirmation exit.
                if (g_AugExitT < 0.0f) {
                    if (k1 == GLFW_PRESS && g_aug1Released && g_GameManager.augChoiceCount > 0) {
                        g_HoveredAug = 0; g_GameManager.augmentKeyboardFocus = true;
                        g_aug1Released = false;
                    }
                    if (k2 == GLFW_PRESS && g_aug2Released && g_GameManager.augChoiceCount > 1) {
                        g_HoveredAug = 1; g_GameManager.augmentKeyboardFocus = true;
                        g_aug2Released = false;
                    }
                    if (k3 == GLFW_PRESS && g_aug3Released && g_GameManager.augChoiceCount > 2) {
                        g_HoveredAug = 2; g_GameManager.augmentKeyboardFocus = true;
                        g_aug3Released = false;
                    }
                }
                if (k1 == GLFW_RELEASE) g_aug1Released = true;
                if (k2 == GLFW_RELEASE) g_aug2Released = true;
                if (k3 == GLFW_RELEASE) g_aug3Released = true;

                // SPACE = confirm the focused constellation.
                int kSp = glfwGetKey(window, GLFW_KEY_SPACE);
                if (kSp == GLFW_PRESS && s_augSpaceReleased &&
                    g_HoveredAug >= 0 && g_HoveredAug < g_GameManager.augChoiceCount &&
                    g_AugExitT < 0.0f) {
                    g_AugExitT    = 0.0f;
                    g_AugExitSlot = g_HoveredAug;
                    s_augSpaceReleased = false;
                    g_GameManager.spaceReleased = false;
                }


            }
        }

        if (!debugInputCaptured && g_GameManager.currentState == GameState::AUG_REPLACE) {
            constexpr float REP_EXIT_DUR = 0.28f;
            static bool s_repEsc   = true;
            static bool s_repSpace = true;
            static bool s_repKeys[9] = { true,true,true,true,true,true,true,true,true };
            if (inputFocusChanged) {
                s_repEsc = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_RELEASE;
                s_repSpace = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE;
                for (int i = 0; i < 9; ++i)
                    s_repKeys[i] = glfwGetKey(window, GLFW_KEY_1 + i) == GLFW_RELEASE;
            }

            int kEsc   = glfwGetKey(window, GLFW_KEY_ESCAPE);
            int kSpRep = glfwGetKey(window, GLFW_KEY_SPACE);
            if (kEsc   == GLFW_RELEASE) s_repEsc   = true;
            if (kSpRep == GLFW_RELEASE) s_repSpace  = true;
            for (int ki = 0; ki < 9; ki++)
                if (glfwGetKey(window, GLFW_KEY_1 + ki) == GLFW_RELEASE) s_repKeys[ki] = true;

            bool repExitFired = false;
            if (g_RepExitT >= 0.0f) {
                g_RepExitT += delta;
                if (g_RepExitT >= REP_EXIT_DUR) {
                    if (g_RepExitSlot == -2)
                        CancelAugReplaceFlow();
                    else {
                        CompleteAugReplaceFlow(g_RepExitSlot, screenWidth, screenHeight);
                        g_HoveredAug = -1;
                    }
                    g_RepExitT    = -1.0f;
                    g_RepExitSlot = -3;
                    repExitFired  = true;
                }
            }

            if (!repExitFired && g_RepExitT < 0.0f) {
                if (kEsc == GLFW_PRESS && s_repEsc) {
                    g_RepExitT    = 0.0f;
                    g_RepExitSlot = -2;
                    s_repEsc = false;
                }
                if (kSpRep == GLFW_PRESS && s_repSpace && g_HoveredAug >= 0 &&
                    g_HoveredAug < g_GameManager.replaceChoiceCount) {
                    g_RepExitT    = 0.0f;
                    g_RepExitSlot = g_HoveredAug;
                    s_repSpace = false;
                }
                for (int ki = 0; ki < 9; ki++) {
                    if (ki >= g_GameManager.replaceChoiceCount) break;
                    if (glfwGetKey(window, GLFW_KEY_1 + ki) == GLFW_PRESS && s_repKeys[ki]) {
                        g_HoveredAug = ki;
                        g_GameManager.augmentKeyboardFocus = true;
                        s_repKeys[ki] = false;
                    }
                }
            }
        }

        // --- �� ���� (���� Ŭ���� ��) ---
        // --- ?�리?�이?�브 모드: F = 증강 그랩(?�버???�함 ?�드박스), G = 무적 ?��? ---
        if (!debugInputCaptured && (g_CreativeMode || g_DebugMode || kCompileDebugBuild)) {
            static bool s_fkeyReleased = true;
            int kF = glfwGetKey(window, GLFW_KEY_F);
            if (inputFocusChanged) s_fkeyReleased = kF == GLFW_RELEASE;
            if (kF == GLFW_RELEASE) s_fkeyReleased = true;
            if (kF == GLFW_PRESS && s_fkeyReleased &&
                g_GameManager.currentState == GameState::RUNNING &&
                !g_GameManager.augReady) {
                // ?�드박스: ?�버?�도 카드 ?�???�어??무엇?�든 집을 ???�게
                const int oldLevel = g_GameManager.playerLevel;
                const long long oldXp = g_GameManager.xp;
                 ++g_GameManager.playerLevel;
                 g_GameManager.xp = 0;
                g_GameplayTelemetry.RecordLevelUp(g_GameTime,
                                                  oldLevel,
                                                  g_GameManager.playerLevel,
                                                  0,
                                                  oldXp,
                                                  g_GameManager.xp,
                                                  true);
                if (g_CreativeMode) {
                    g_GameManager.QueueAugmentReward(false, /*allowDebuff=*/true);
                } else {
                    g_GameManager.QueueAugmentReward(true, /*allowDebuff=*/false);
                }
                g_GameManager.ActivateNextAugmentReward();
                g_CreativeFreeGrab = false;
                s_fkeyReleased = false;
            }
            // G ??무적 ON/OFF ?��?
            static bool s_gkeyReleased = true;
            int kG = glfwGetKey(window, GLFW_KEY_G);
            if (inputFocusChanged) s_gkeyReleased = kG == GLFW_RELEASE;
            if (kG == GLFW_RELEASE) s_gkeyReleased = true;
            if (kG == GLFW_PRESS && s_gkeyReleased &&
                g_GameManager.currentState == GameState::RUNNING) {
                g_CreativeGodmode = !g_CreativeGodmode;
                s_gkeyReleased = false;
            }
        }

        // --- ?�트?�톱: ???�벤??직후 ?�깐 ?��??�이???��? (?�더??계속) ---
        if (g_HitStopTimer > 0.0f) g_HitStopTimer -= delta;

        // --- Fixed timestep (DYING = 0.15× ?�로??모션, ?�트?�톱 = ?�전 ?��?) ---
        float physDelta = (g_GameManager.currentState == GameState::DYING)
                          ? delta * 0.15f : delta;
        if (g_HitStopTimer > 0.0f) physDelta = 0.0f;   // ��Ʈ��ž �� ���� ���� �̽���
        accumulator += physDelta;
        while (accumulator >= FIXED_DT) {
            if (g_GameManager.ShouldUpdate()) {
                const bool playerControl = g_GameManager.currentState == GameState::RUNNING;
                if (!playerControl) {
                    g_PlayerRuntime.dashActive = false;
                }
                g_PlayerDmgMult = g_Stats.GetDamageTakenMult();
                const float zoomNow = (g_ViewZoom < 0.01f) ? 0.01f : g_ViewZoom;
                float mvX = 0.0f, mvY = 0.0f;
                const float mlen = UpdatePlayerMovement(
                    playerWin.x, playerWin.y,
                    keys[GLFW_KEY_W], keys[GLFW_KEY_S],
                    keys[GLFW_KEY_A], keys[GLFW_KEY_D],
                    playerControl, MOVE_SPEED * g_Stats.GetMoveMultiplier(lmb),
                    FIXED_DT, g_PlayerRuntime.dashActive, mvX, mvY);
                if (playerControl && mlen > 0.001f && !g_PlayerRuntime.dashActive) {
                    // ?�동 ?�상(afterimage) ???�정 간격?�로 ?�레?�어 중심???��? ?�안 ?�상
                    static float s_trailAcc = 0.0f;
                    s_trailAcc += FIXED_DT;
                    if (s_trailAcc >= 0.028f) {
                        s_trailAcc = 0.0f;
                        SpawnTrail(playerWin.x + playerWin.width  * 0.5f,
                                   playerWin.y + playerWin.height * 0.5f,
                                   24.0f, 0.35f, 0.85f, 1.0f);
                    }
                }

                const PlayerViewportBounds viewport = GetPlayerViewportBounds(
                    (float)screenWidth, (float)screenHeight, zoomNow,
                    BROWSER_CHROME_H, g_GameBarH + (float)g_TaskbarH);
                ClampPlayerToViewport(playerWin.x, playerWin.y,
                                      playerWin.width, playerWin.height,
                                      viewport);
                const float ccX = viewport.centerX;
                const float ccY = viewport.centerY;
                const float halfW = viewport.halfWidth;
                const float halfH = viewport.halfHeight;
                const float topLimit = viewport.top;
                const float bottomLimit = viewport.bottom;
                float pCX = playerWin.x + playerWin.width * 0.5f;
                float pCY = playerWin.y + playerWin.height * 0.5f;

                // ?�?�??�티�??�킬 ??쿨다??지??갱신 + ?�력(Shift ?�??/ Q·E·R ?�롯) ?�?�?
                UpdatePlayerRuntimeTimers(FIXED_DT);


                // �??�기 ???�레?�어 중심 ??�� (?�백 + ?�해)
                auto closeWindowBlast = [&](float cx, float cy) {
                    float dmg = g_Stats.GetBaseDamage() * g_Stats.GetDamageMultiplier() * 8.0f;
                    float rad = 380.0f, r2 = rad * rad, knock = 130.0f;
                    auto hitKB = [&](float& ex, float& ey, float& hp, bool& al) {
                        float dx = ex - cx, dy = ey - cy, d2 = dx*dx + dy*dy;
                        if (d2 < r2) { hp -= dmg; float d = std::sqrt(d2)+1e-3f;
                            ex += dx/d*knock; ey += dy/d*knock; if (hp <= 0) al = false; }
                    };
                    for (auto m  : g_MonsterManager.monsters)   if (m->alive)  hitKB(m->worldX,  m->worldY,  m->hp,  m->alive);
                    for (auto r  : g_MonsterManager.rangedMobs) if (r->alive)  hitKB(r->worldX,  r->worldY,  r->hp,  r->alive);
                    SpawnShockWave(cx, cy, rad*1.3f, 0.6f, 0.5f, 0.8f, 1.0f);
                    SpawnEnemyExplosion(cx, cy, 0.5f, 0.8f, 1.0f, true);
                    g_ShakeTime = 0.4f; g_ShakeMag = 20.0f;
                    TriggerFlash(0.5f, 0.8f, 1.0f, 0.45f); TriggerHitStop(0.07f);
                };

                // ?�롯 ?�킬 발동 ?�퍼
                auto useSkill = [&](int slot) {
                    if (slot < 0 || slot >= 3) return;
                    SkillSlot& s = g_Skills[slot];
                    if (s.type == SkillType::NONE || s.cd > 0.0f) return;
                    switch (s.type) {
                    case SkillType::CLOSE_WINDOW: closeWindowBlast(pCX, pCY); break;
                    case SkillType::HYPER_FOCUS:
                        g_PlayerRuntime.hyperFocusTimer = HYPER_FOCUS_DUR;
                        TriggerFlash(0.5f, 0.75f, 1.0f, 0.35f);
                        break;
                    case SkillType::TIME_STOP:    g_PlayerRuntime.timeStopTimer  = TIMESTOP_DUR;
                                                  TriggerFlash(0.4f,0.9f,1.0f,0.4f); break;
                    default: break;
                    }
                    s.cd = PlayerSkillCooldownMax(s.type);
                };

                // �÷��� ���?? ���� �̵� + ª�� ����
                if (playerControl && g_PlayerRuntime.dashActive) {
                    AdvancePlayerDash(FIXED_DT, pCX, pCY);
                    playerWin.x = pCX - playerWin.width * 0.5f;
                    playerWin.y = pCY - playerWin.height * 0.5f;
                }
                bool cDash = keys[GLFW_KEY_LEFT_SHIFT] || keys[GLFW_KEY_RIGHT_SHIFT];
                if (playerControl && cDash && !g_PlayerRuntime.dashInputHeld && g_PlayerRuntime.dashCooldown <= 0.0f && !g_PlayerRuntime.dashActive) {
                    float ddx = mvX, ddy = mvY;
                    if (mlen <= 0.001f) {
                        float ax = wmx - pCX, ay = wmy - pCY;
                        float al = std::sqrt(ax*ax+ay*ay)+1e-3f; ddx = ax/al; ddy = ay/al;
                    }
                    const float topLimit = ccY + (BROWSER_CHROME_H - ccY) / zoomNow;
                    BeginPlayerDash(pCX, pCY, ddx, ddy,
                                    ccX - halfW, ccX + halfW,
                                    topLimit, ccY + halfH);
                    TriggerFlash(0.35f, 0.85f, 1.0f, 0.12f);
                    SpawnShockWave(g_PlayerRuntime.dashFromX, g_PlayerRuntime.dashFromY, 70.0f, 0.25f, 0.4f, 1.0f, 1.0f);
                    if (g_Stats.dashUpgrade) {
                        g_PlayerRuntime.dashBoostShotsLeft = 3;
                        const int burstN = 3 + rand() % 3;
                        for (int bi = 0; bi < burstN; bi++) {
                            float a = (float)bi / (float)burstN * 2.0f * (float)M_PI
                                    + ((float)(rand() % 100) - 50.0f) * 0.01f;
                            Bullet nb(pCX, pCY,
                                      pCX + cosf(a) * 120.0f,
                                      pCY + sinf(a) * 120.0f);
                            nb.speed      = g_Stats.bulletSpeed * 1.1f;
                            nb.color      = glm::vec3(0.45f, 0.95f, 1.0f);
                            nb.homing     = true;
                            nb.homingTurn = 6.0f;
                            nb.dmgMult    = 0.55f;
                            nb.launchRamp  = 0.08f;
                            nb.launchAccel = 1.4f;
                            g_Bullets.push_back(nb);
                        }
                    }
                }
                g_PlayerRuntime.dashInputHeld = cDash;
                const int skillKeys[3] = { GLFW_KEY_Q, GLFW_KEY_E, GLFW_KEY_R };
                for (int i = 0; i < 3; ++i) {
                    const bool held = keys[skillKeys[i]];
                    if (playerControl && held && !g_PlayerRuntime.skillInputHeld[i])
                        useSkill(i);
                    g_PlayerRuntime.skillInputHeld[i] = held;
                }
                // C16: ?�티�??�킬 ?�동 ?�용 ??쿨다???�난 ?�롯???�동 발동
                if (playerControl && g_AutoSkill) for (int i = 0; i < 3; i++) useSkill(i);

                // Apply each station once per fixed step.
                UpdateGravisFields(pCX, pCY, playerControl, FIXED_DT);
                pCX = std::max(ccX - halfW, std::min(ccX + halfW, pCX));
                pCY = std::max(ccY + (BROWSER_CHROME_H - ccY) / zoomNow,
                               std::min(bottomLimit, pCY));
                playerWin.x = pCX - playerWin.width * 0.5f;
                playerWin.y = pCY - playerWin.height * 0.5f;

                UpdateProjectiles(g_Bullets, g_MonsterManager, playerWin,
                                  FIXED_DT, g_PlayerRuntime.timeStopTimer,
                                  g_PlayerRuntime.hyperFocusTimer,
                                  (float)screenWidth, (float)screenHeight,
                                  g_ViewZoom);

                // ?�??무적 ???�번 ?�텝 ?�작 HP ?�??(???�해??무효, ?�복?�??��?)
                float hpAtStep = g_GameManager.playerHP;
                TickPlayerShield(FIXED_DT);
                //   ???��? = ?�간?��? ?�킬 OR 증강 ??직후 ~0.05s("?�이 ?�?, 짧게)
                bool  timeStopped = (g_PlayerRuntime.timeStopTimer > 0.0f) || (g_PlayerRuntime.postPickGrace > 0.45f);
                float enemyDt = FIXED_DT;
                if (!timeStopped && g_PlayerRuntime.hyperFocusTimer > 0.0f) enemyDt *= 0.7f;
                float focusSlow = (g_PlayerRuntime.hyperFocusTimer > 0.0f) ? 0.7f : 1.0f;

                // 몬스???�데?�트 (?�버??multiplier ?�용) ???�간 ?��? 중엔 ??멈춤
                float rmobMoveMult = 1.0f / g_Stats.rmobDelayMult; // <1 ????빠름
                // Time-based speed ramp. Density is lower now, so enemy pressure should not depend
                // only on score gained from kills.
                float mobSpdRamp = EnemySpeedRamp(g_GameTime);
                // Ư���� ? �� AI ���� �����衤�Ұ�, AI �� �÷��̾� �о��
                if (!timeStopped && g_Stats.chakram && g_Stats.chakramSingularity) {
                    const float pullR2 = 220.0f * 220.0f;
                    const float winSz = g_WindowSizeCur > 1.0f ? g_WindowSizeCur : g_Stats.windowSize;
                    const float safeR  = winSz * 0.52f;
                    const float safeR2 = safeR * safeR;
                    for (int c = 0; c < g_Stats.chakramCount && c < MAX_CHAKRAMS; c++) {
                        if (!g_Chakrams[c].alive) continue;
                        float chx = pCX + cosf(g_Chakrams[c].angle) * CHAKRAM_RADIUS;
                        float chy = pCY + sinf(g_Chakrams[c].angle) * CHAKRAM_RADIUS;
                        for (auto m : g_MonsterManager.monsters) {
                            if (!m->alive) continue;
                            float ddx = chx - m->worldX, ddy = chy - m->worldY;
                            float d2 = ddx * ddx + ddy * ddy;
                            float pdx = m->worldX - pCX, pdy = m->worldY - pCY;
                            float pd2 = pdx * pdx + pdy * pdy;
                            if (d2 < pullR2) {
                                m->singularityGrace = 0.14f;
                                if (d2 > 900.0f && pd2 > safeR2) {
                                    float d = std::sqrt(d2) + 1e-3f;
                                    m->worldX += (ddx / d) * 300.0f * FIXED_DT;
                                    m->worldY += (ddy / d) * 300.0f * FIXED_DT;
                                }
                                m->hp -= 100.0f * FIXED_DT;
                                if (m->hp <= 0.0f) m->alive = false;
                            }
                        }
                    }
                }
                if (!timeStopped) {
                    g_MonsterManager.UpdateAll(pCX, pCY, enemyDt,
                                               g_GameManager.playerHP, g_Bullets,
                                               g_Stats.mobSpeedMult * mobSpdRamp * focusSlow,
                                               rmobMoveMult * mobSpdRamp * focusSlow,
                                               g_Stats.rotorHpMult);

                    // Large controller enemies must remain readable on-screen.
                    // Reflect their free-drift heading when the safe viewport is
                    // reached so they do not idle beyond the visible arena.
                    for (auto* m : g_MonsterManager.monsters) {
                        if (!m || !m->alive ||
                            (m->kind != MobKind::GRAVIS && m->kind != MobKind::QUASAR)) continue;
                        const float margin = m->kind == MobKind::GRAVIS ? 92.0f : 76.0f;
                        const float minX = margin;
                        const float maxX = std::max(minX, (float)screenWidth - margin);
                        const float minY = margin;
                        const float maxY = std::max(minY, (float)screenHeight - margin);
                        const bool hitX = m->worldX < minX || m->worldX > maxX;
                        const bool hitY = m->worldY < minY || m->worldY > maxY;
                        m->worldX = std::max(minX, std::min(maxX, m->worldX));
                        m->worldY = std::max(minY, std::min(maxY, m->worldY));
                        if (m->kind == MobKind::GRAVIS) {
                            if (hitX) m->gravisDriftAngle = 3.14159265f - m->gravisDriftAngle;
                            if (hitY) m->gravisDriftAngle = -m->gravisDriftAngle;
                        } else {
                            if (hitX) m->quasarMoveAngle = 3.14159265f - m->quasarMoveAngle;
                            if (hitY) m->quasarMoveAngle = -m->quasarMoveAngle;
                        }
                    }
                }

                // Static Field is an area weapon with a real fixed-step pulse.
                // It supplements the loadout's short-range field identity and
                // gives the weapon a reliable combat effect instead of leaving
                // the field geometry as a purely cosmetic shell.
                if (g_CurrentWeapon == (int)StartWeapon::SMG && playerControl) {
                    g_StaticFieldPulseTimer -= FIXED_DT;
                    if (g_StaticFieldPulseTimer <= 0.0f && !timeStopped) {
                        constexpr float kPulseInterval = 0.24f;
                        constexpr float kPulseRadius = 190.0f;
                        constexpr float kPulseDamage = 0.42f;
                        const float radius2 = kPulseRadius * kPulseRadius;
                        const float pulseDamage = g_Stats.GetBaseDamage()
                            * g_Stats.GetDamageMultiplier() * kPulseDamage;
                        auto pulseTarget = [&](float tx, float ty, float& hp,
                                               bool& alive) {
                            if (!alive) return;
                            const float dx = tx - pCX, dy = ty - pCY;
                            if (dx * dx + dy * dy > radius2) return;
                            const float dealt = std::min(pulseDamage, hp);
                            hp -= dealt;
                            SpawnDamageNumber(tx, ty, dealt, false);
                            if (hp <= 0.0f) {
                                hp = 0.0f;
                                alive = false;
                            }
                        };
                        for (auto* m : g_MonsterManager.monsters)
                            pulseTarget(m->worldX, m->worldY, m->hp, m->alive);
                        for (auto* r : g_MonsterManager.rangedMobs)
                            pulseTarget(r->worldX, r->worldY, r->hp, r->alive);
                        g_StaticFieldPulseTimer += kPulseInterval;
                    }
                } else {
                    g_StaticFieldPulseTimer = 0.0f;
                }
                if (!timeStopped && g_Stats.chakram && g_Stats.chakramSingularity) {
                    const float winSz = g_WindowSizeCur > 1.0f ? g_WindowSizeCur : g_Stats.windowSize;
                    const float safeR  = winSz * 0.52f;
                    const float safeR2 = safeR * safeR;
                    for (auto m : g_MonsterManager.monsters) {
                        if (!m->alive) continue;
                        float pdx = m->worldX - pCX, pdy = m->worldY - pCY;
                        float pd2 = pdx * pdx + pdy * pdy;
                        if (pd2 >= safeR2) continue;
                        float pd = std::sqrt(pd2) + 1e-3f;
                        float push = (safeR - pd + 20.0f) * 16.0f;
                        m->worldX += (pdx / pd) * push * FIXED_DT;
                        m->worldY += (pdy / pd) * push * FIXED_DT;
                        m->singularityGrace = 0.16f;
                    }
                }

                // 리로???�너 ?�데?�트 (무기 ?�태머신 + ?�전 질주, ??총알 push)
CollisionSystem::Update(pCX, pCY,
                    g_MonsterManager, g_Bullets,
                    g_GameManager.playerHP,
                    g_GameManager.scoreAccum, g_GameManager.score,
                    g_Stats, g_GameManager.xp);
                if (g_Bullets.size() > 2800) {
                    g_Bullets.erase(
                        std::remove_if(g_Bullets.begin(), g_Bullets.end(),
                            [](const Bullet& b){ return !b.active; }),
                        g_Bullets.end());
                    if (g_Bullets.size() > 2800)
                        g_Bullets.erase(g_Bullets.begin() + 2800, g_Bullets.end());
                }
                // ?�??무적 / 증강???�예(C14) ???�번 ?�텝?????�해 무효 (?�복?�??��?)
                ApplyPlayerDamageProtection(
                    hpAtStep, g_GameManager.playerHP,
                    g_PlayerRuntime.dashInvulnerability > 0.0f ||
                        g_PlayerRuntime.postPickGrace > 0.0f,
                    g_Stats.lightStep, g_Stats.lightStepHitLock,
                    g_Stats.lightStepDisableTimer);

                // Regular enemy kill rewards.
                auto creditKill = [&](CodexMobId mobId, float scoreBase) {
                    AddKillCombo();
                    g_Stats.RegisterKill();
                    RegisterCodexMobKill(mobId);
                    g_GameManager.scoreAccum += scoreBase;
                    g_GameManager.score      = (long long)g_GameManager.scoreAccum;
                    if (!g_CreativeMode)
                        if (g_Stats.GetLifestealPerKill() > 0.0f) {
                        g_GameManager.playerHP += g_Stats.GetLifestealPerKill();
                        if (g_GameManager.playerHP > g_Stats.maxHP)
                            g_GameManager.playerHP = g_Stats.maxHP;
                    }
                    if (g_Stats.vampire || g_Stats.lifesteal2) {
                        if (++g_Stats.vampireKillStreak >= g_Stats.GetVampireKillNeed()) {
                            g_Stats.vampireKillStreak = 0;
                            g_GameManager.playerHP += 1.0f;
                            if (g_GameManager.playerHP > g_Stats.maxHP)
                                g_GameManager.playerHP = g_Stats.maxHP;
                        }
                    }
                };

                // ?�망 ??�� spawn
                //   + 분열�??�망 ???��? ?�식 2마리 (�?2?��?까�?)
                for (auto m : g_MonsterManager.monsters) {
                    if (!m->alive && !m->exploded) {
                        if (!m->scored) {       // ?�직 보상 ??받�? 죽음 ???�산
                            m->scored = true;
                            float xpB, scB; MobKillReward(m->kind, xpB, scB);
                            float rwm = MobRewardMult(m->kind); xpB *= rwm; scB *= rwm;
                            const long long pickupXp = (long long)(
                                (xpB + MobXpBonus(m->kind, g_Stats))
                                * g_Stats.xpMult);
                            SpawnStardust(m->worldX, m->worldY,
                                          StardustRewardFor(m->kind), pCX, pCY, pickupXp);
                            creditKill(CodexMobIdForKind(m->kind), scB);
                        }
                        if (m->kind == MobKind::SWARM)
                            SpawnNodeDeath(m->worldX, m->worldY, m->color.r, m->color.g, m->color.b);
                        else
                            SpawnEnemyExplosion(m->worldX, m->worldY, m->color.r, m->color.g, m->color.b, false);
                        m->exploded = true;
                        // 처치 ?�출 ???�로?�스 종료 ?�그 (강적?�???�� 강조)
                        {
                            static const wchar_t* W[5] =
                                { L"terminated", L"killed", L"ended", L"exited", L"0x1B" };
                            SpawnKillTag(m->worldX, m->worldY, 0.85f, 0.95f, 1.0f,
                                         W[rand() % 5], false);
                        }
                        // ??��???�리????죽을 ???�져 ?�레?�어?�게 광역 ?�해
                        // ?�쇄 ??��(DEATH_BLAST) ???�망 ?�치?�서 주�? ?�에�?AoE
                        //   ?�프: ?��?지 0.8??.3 + ??���?죽�? 몹�? ?�시 ???�짐(무한?�쇄 차단)
                        if (g_Stats.deathBlast && !m->noBlast) {
                            float blastDmg = g_Stats.GetBaseDamage()
                                           * g_Stats.GetDamageMultiplier() * g_Stats.deathBlastDmgPct;
                            float blastR = 130.0f * g_Stats.deathBlastMult;
                            float bx = m->worldX, by = m->worldY;
                            SpawnShockWave(bx, by, blastR, 0.35f, 1.0f, 0.55f, 0.15f);
                            for (auto m2 : g_MonsterManager.monsters) {
                                if (!m2->alive || m2 == m) continue;
                                float ddx = m2->worldX - bx, ddy = m2->worldY - by;
                                if (ddx*ddx + ddy*ddy < blastR*blastR) {
                                    m2->hp -= blastDmg;
                                    if (m2->hp <= 0.0f) { m2->alive = false; m2->noBlast = true; }
                                }
                            }
                            for (auto r2 : g_MonsterManager.rangedMobs) {
                                if (!r2->alive) continue;
                                float ddx = r2->worldX - bx, ddy = r2->worldY - by;
                                if (ddx*ddx + ddy*ddy < blastR*blastR) {
                                    r2->hp -= blastDmg;
                                    if (r2->hp <= 0.0f) r2->alive = false;
                                }
                            }
                        }
                    }
                }
                for (auto r : g_MonsterManager.rangedMobs) {
                    if (!r->alive && !r->exploded) {
                        if (!r->scored) {
                            r->scored = true;
                            const long long pickupXp = (long long)(
                                (25.0f + (float)g_Stats.rangedXpBonus) * g_Stats.xpMult);
                            SpawnStardust(r->worldX, r->worldY, 3,
                                          pCX, pCY, pickupXp);
                            creditKill(CM_SCOPE, 300.0f);
                        }
                        SpawnEnemyExplosion(r->worldX, r->worldY,
                                            r->color.r, r->color.g, r->color.b,
                                            /*big=*/true);
                        r->exploded = true;
                        SpawnKillTag(r->worldX, r->worldY, 1.0f, 0.55f, 0.9f,
                                     L"popup closed", true);
                    }
                }
                    const bool atMainCap = !g_CreativeMode
                                          && g_GameManager.playerLevel >= MAIN_LEVEL_CAP;
                    if (atMainCap) {
                        g_GameManager.xp = 0;
                    } else {
                    // Consume every level threshold available in the same
                    // fixed tick.  A large pickup must not leave the player
                    // permanently one level behind or discard XP.
                    constexpr int kMaxLevelUpsPerTick = 64;
                    int levelUps = 0;
                    while (levelUps++ < kMaxLevelUpsPerTick &&
                           (!atMainCap ||
                            g_GameManager.playerLevel < MAIN_LEVEL_CAP)) {
                        const long long need =
                            g_ExpSystem.Required(g_GameManager.playerLevel);
                        if (need <= 0 || g_GameManager.xp < need) break;
                        const int oldLevel = g_GameManager.playerLevel;
                        const long long oldXp = g_GameManager.xp;
                        g_GameManager.xp -= need;
                        ++g_GameManager.playerLevel;
                        g_GameplayTelemetry.RecordLevelUp(g_GameTime,
                                                          oldLevel,
                                                          g_GameManager.playerLevel,
                                                          need,
                                                          oldXp,
                                                          g_GameManager.xp,
                                                          false);
                        g_GameManager.QueueAugmentReward(true, false);
                    }
                    } // !atMainCap

                // �ð� ���� (�޽ġ����� �߿��� ����)
                g_GameManager.AddScore(FIXED_DT * 100.0f);
            }
            accumulator -= FIXED_DT;
        }

        g_GameplayTelemetry.Update(
            g_GameTime,
            g_GameManager.playerLevel,
            g_GameManager.xp,
            g_ExpSystem.Required(g_GameManager.playerLevel),
            g_Stats);

        // ?�?�??�맛: ?��?지 ?�자 / 콤보 / ?�래??�??�레??갱신 (게임 진행 중에�?감쇠) ?�?�?
        if (g_GameManager.currentState == GameState::RUNNING ||
            g_GameManager.currentState == GameState::DYING) {
            for (auto& d : g_DmgNumbers) {
                d.x  += d.vx * delta;
                d.y  += d.vy * delta;
                d.vy += 70.0f * delta;        // ?�한 중력 (?�구쳤다 처짐)
                d.life -= delta;
            }
            g_DmgNumbers.erase(std::remove_if(g_DmgNumbers.begin(), g_DmgNumbers.end(),
                [](const DamageNumber& d){ return d.life <= 0.0f; }), g_DmgNumbers.end());
            if (g_ComboTimer > 0.0f) {
                g_ComboTimer -= delta;
                if (g_ComboTimer <= 0.0f) g_Combo = 0;
            }
            g_ComboPulse -= delta * 4.0f;
            if (g_ComboPulse < 0.0f) g_ComboPulse = 0.0f;
            if (g_ComboMilestone > 0.0f) g_ComboMilestone -= delta;
        }
        if (g_FlashIntensity > 0.0f) {
            g_FlashIntensity -= delta * 3.5f;
            if (g_FlashIntensity < 0.0f) g_FlashIntensity = 0.0f;
        }

        {
            GameState ws = g_GameManager.currentState;
            if (ws == GameState::RUNNING || ws == GameState::PAUSED ||
                ws == GameState::READY  || ws == GameState::AUG_SELECT ||
                ws == GameState::DEBUFF_SELECT || ws == GameState::DYING) {
                SyncPlayerBoundsSize(playerWin, delta, ws == GameState::RUNNING);
            }
        }

        // Keep a delayed HP value for the radial afterimage.  Damage snaps
        // the gray layer to the pre-hit HP; it then eases toward live HP.
        // Non-combat screens synchronize immediately so upgrades or resets
        // cannot be mistaken for damage.
        {
            const GameState hpState = g_GameManager.currentState;
            const bool hpAnimActive = hpState == GameState::RUNNING ||
                                      hpState == GameState::DYING;
            const float hpNow = std::max(0.0f, g_GameManager.playerHP);
            if (!hpAnimActive) {
                g_WinPrevHP = hpNow;
                g_HpGhost = hpNow;
            } else {
                if (g_WinPrevHP < 0.0f) {
                    g_WinPrevHP = hpNow;
                    g_HpGhost = hpNow;
                }

                const float hpBefore = g_WinPrevHP;
                const float hpDelta = hpNow - hpBefore;
                if (g_PlayerDamagePulse > 0.0f) {
                    g_HpBarPop = std::max(g_HpBarPop, g_PlayerDamagePulse);
                    g_PlayerDamagePulse = 0.0f;
                }
                if (hpDelta < -0.0001f) {
                    // Preserve an older afterimage if another hit lands
                    // before it has finished catching up.
                    g_HpGhost = std::max(g_HpGhost, hpBefore);
                    g_HurtVignette = 0.5f;
                    g_HpBarPop = 2.2f;
                } else if (hpDelta > 0.0001f) {
                    // Healing moves the live gauge immediately; do not leave
                    // a misleading gray damage segment behind it.
                    g_HpGhost = hpNow;
                }
                g_WinPrevHP = hpNow;

                if (g_HpGhost < 0.0f) g_HpGhost = hpNow;
                if (g_HpGhost > hpNow) {
                    const float follow = 1.0f - expf(-7.5f * delta);
                    g_HpGhost += (hpNow - g_HpGhost) * follow;
                }

                if (g_HurtVignette > 0.0f) {
                    g_HurtVignette -= delta * 1.6f;
                    if (g_HurtVignette < 0.0f) g_HurtVignette = 0.0f;
                }
                if (g_HpBarPop > 0.0f) {
                    g_HpBarPop -= delta;
                    if (g_HpBarPop < 0.0f) g_HpBarPop = 0.0f;
                }
            }
        }

        if (g_GameManager.currentState == GameState::RUNNING) {
            float pCX = playerWin.x + playerWin.width  * 0.5f;
            float pCY = playerWin.y + playerWin.height * 0.5f;
            bool  moving = keys[GLFW_KEY_W] || keys[GLFW_KEY_S] ||
                           keys[GLFW_KEY_A] || keys[GLFW_KEY_D];

            // ?�?�??�격 ?��???HP 감소 ??빨간 비네?�만 (?�야 변???�거, �??�기 고정) ?�?�?
            // HP ���?(REGEN_UP, �Ŵ�ȭ, ���?II �� regenPerSec �ջ�)
            if (g_Stats.regenPerSec > 0.0f) {
                g_GameManager.playerHP += g_Stats.GetRegenRate(g_GameManager.playerHP) * delta;
                if (g_GameManager.playerHP > g_Stats.maxHP)
                    g_GameManager.playerHP = g_Stats.maxHP;
            }
            // 배드 ?�터 출혈 ??감속 구역 ?�이�?출혈 ?�?�머 2초로 갱신, 빠져?��????�류


            // ?�감 발견 ???�거�??�폭�?(존재?�면 발견 처리)
            if (!g_MonsterManager.rangedMobs.empty()) MarkMobSeenId(CM_SCOPE);

            // ?�적 조건 체크 (???�수/??보유 증강 기�? ??보스 ?�적?�?처치 ?�점?�서 처리)
            {
                long long runScore = g_GameManager.score;
                long long runKills = g_Stats.killCount;
                if (runScore >= 300000)  TryUnlockAch(ACH_SCORE_300K);
                if (runScore >= 500000)  TryUnlockAch(ACH_SCORE_500K);
                if (runScore >= 1000000) TryUnlockAch(ACH_SCORE_1M);
                if (runKills >= 500)     TryUnlockAch(ACH_KILLS_500);
                if (g_Stats.critChance > 0 && runScore >= 200000)
                    TryUnlockAch(ACH_CRIT_SCORE);
                if (g_Stats.deathBlast && runKills >= 300)
                    TryUnlockAch(ACH_DEATHBLAST_KILLS);
                int debuffCnt = 0;
                for (int oi : g_OwnedAugs)
                    if (ALL_AUGS[oi].rarity == AugRarity::DEBUFF) ++debuffCnt;
                if (debuffCnt >= 5) TryUnlockAch(ACH_DEBUFF_5);
            }

            UpdateEnemyFx(delta);

            // 충격???�데?�트
            for (auto& sw : g_ShockWaves) {
                if (!sw.active) continue;
                sw.life -= delta;
                if (sw.life <= 0.0f) sw.active = false;
            }

            // 검�??�윙 ?�상 ?�데?�트

            for (auto& lb : g_LaserBeams) lb.life -= delta;
            g_LaserBeams.erase(std::remove_if(g_LaserBeams.begin(), g_LaserBeams.end(),
                [](const LaserBeam& b){ return b.life <= 0.0f; }), g_LaserBeams.end());

            // 별가루: 스폰 직후 약간 퍼진 뒤 매 프레임 플레이어 방향으로 재조향 — 무조건 수집.
            g_StardustHudPulse = std::max(0.0f, g_StardustHudPulse - delta * 3.8f);
            g_XpBarPop = std::max(0.0f, g_XpBarPop - delta * 2.2f);
            for (auto& dust : g_StardustPickups) {
                if (!dust.alive) continue;
                dust.age += delta;

                // 수집 판정
                float dx = pCX - dust.x, dy = pCY - dust.y;
                float dist = sqrtf(dx * dx + dy * dy);
                if (dist < 60.0f) {
                    g_GameManager.xp += dust.xpValue;
                    g_GameplayTelemetry.RecordExperience(dust.xpValue);
                    if (dust.xpValue > 0)
                        g_XpBarPop = std::max(g_XpBarPop, 1.15f);
                    g_Coins += dust.value;
                    g_RunStardust += dust.value;
                    g_StardustHudPulse = 1.0f;
                    SpawnSparks(dust.x, dust.y, dust.value >= 5 ? 6 : 3,
                                1.0f, 0.85f, 0.30f, 180.0f);
                    dust.alive = false;
                    continue;
                }

                // 초기 0.4s: 퍼짐 (초기 vx/vy 방향 유지하되 감속)
                if (dust.age < 0.40f) {
                    const float drag = 1.0f - delta * 4.5f;
                    dust.vx *= drag;
                    dust.vy *= drag;
                } else {
                    // 이후: 매 프레임 플레이어 방향으로 속도 재조향 (무조건 추적)
                    const float speed = 520.0f + std::min(280.0f, dust.age * 400.0f);
                    if (dist > 0.001f) {
                        dust.vx = (dx / dist) * speed;
                        dust.vy = (dy / dist) * speed;
                    }
                }

                dust.x += dust.vx * delta;
                dust.y += dust.vy * delta;
            }
            g_StardustPickups.erase(std::remove_if(g_StardustPickups.begin(), g_StardustPickups.end(),
                [](const StardustPickup& d) { return !d.alive || d.age > 6.0f; }),
                g_StardustPickups.end());

            // ?��??�파???�데?�트 (?�동 + 감속 + ?�명)
            for (auto& sp : g_Sparks) {
                sp.x += sp.vx * delta;
                sp.y += sp.vy * delta;
                float drag = std::max(0.0f, 1.0f - 6.0f * delta);
                sp.vx *= drag; sp.vy *= drag;
                sp.life -= delta;
            }
            g_Sparks.erase(std::remove_if(g_Sparks.begin(), g_Sparks.end(),
                [](const Spark& s){ return s.life <= 0.0f; }), g_Sparks.end());
            // ?�동 ?�상 ?�명 갱신
            for (auto& tr : g_Trail) tr.life -= delta;
            g_Trail.erase(std::remove_if(g_Trail.begin(), g_Trail.end(),
                [](const Trail& t){ return t.life <= 0.0f; }), g_Trail.end());
            if (g_MuzzleTimer > 0.0f) g_MuzzleTimer -= delta;

            if (g_ShakeTime > 0.0f) g_ShakeTime -= delta;

            // 초당 EXP ?�적 (?��??�는 죽음 + ?�몹 가??
            if (g_Stats.xpPerSec > 0.0f) {
                g_XpTimeAccum += g_Stats.xpPerSec * g_Stats.xpMult * delta;
                if (g_XpTimeAccum >= 1.0f) {
                    long long add = (long long)g_XpTimeAccum;
                    g_GameManager.xp += add;
                    g_GameplayTelemetry.RecordExperience(add);
                    g_XpTimeAccum    -= (float)add;
                }
            }

            if (g_Stats.lightStepDisableTimer > 0.0f)
                g_Stats.lightStepDisableTimer -= delta;


            UpdateEnemySpawns(g_MonsterManager, g_Stats,
                              screenWidth, screenHeight, delta,
                              g_GameTime, g_GameManager.score);
            g_GameTime += delta;
            float approachSpeed = 120.0f *
                (1.0f + 0.20f * (float)(g_Stats.approachStacks > 0 ? g_Stats.approachStacks - 1 : 0));
            for (auto& orb : g_ApproachOrbs) {
                float dx = pCX - orb.x;
                float dy = pCY - orb.y;
                float d  = sqrtf(dx*dx + dy*dy);
                if (d > 1.0f) {
                    orb.x += (dx/d) * approachSpeed * delta;
                    orb.y += (dy/d) * approachSpeed * delta;
                }
                if (d < 25.0f) {
                    g_GameManager.playerHP -= 20.0f * delta;
                }
            }

            // ?�론: ?�레?�어 주위 공전 + ?�동 발사 (?�력�?50%), 1~2�?            // ?�탑 모드 ?�성 ???�론?�?공전/발사 ?�고 ?�탑?�로 ?�체??
            if (g_Stats.drone) {
                for (int d = 0; d < g_Stats.droneCount && d < MAX_DRONES; d++) {
                    auto& dr = g_Drones[d];
                    dr.angle += 1.5f * delta;
                    // 균등 각도 ?�프??(각자 ?�른 ?�치)
                    float ang = dr.angle + (float)d * 6.2831853f / (float)g_Stats.droneCount;
                    float droneX = pCX + cosf(ang) * 80.0f;
                    float droneY = pCY + sinf(ang) * 80.0f;
                    dr.fireTimer += delta;
                    // 군집 지???�화) ??발사 간격 2.0× ??0.6× (초고??
                    float droneInt = g_Stats.fireInterval * (g_Stats.droneRapid ? 0.6f : 2.0f);
                    if (dr.fireTimer >= droneInt) {
                        float tx = 0.0f, ty = 0.0f;
                        if (g_MonsterManager.FindNearestEnemy(
                                droneX, droneY,
                                playerWin.x, playerWin.y,
                                playerWin.x + playerWin.width,
                                playerWin.y + playerWin.height,
                                tx, ty)) {
                            Bullet nb(droneX, droneY, tx, ty);
                            nb.speed = g_Stats.bulletSpeed * 0.5f;
                            // Hive keeps four orbiters, but each projectile
                            // contributes half of the normal rifle damage.
                            nb.dmgMult = 0.5f;
                            nb.color = glm::vec3(0.2f, 0.9f, 1.0f);
                            g_Bullets.push_back(nb);
                        }
                        dr.fireTimer = 0.0f;
                    }
                }
            }

            //   1초마???�레?�어 ?�치???�탑 1�?배치, �?5�?지????맵에 ~5�??�시

            // 차크?? 공전 + ?�몹 즉사 + ??총알 충돌 + ?�생??(n �?
            if (g_Stats.chakram) {
                for (int c = 0; c < g_Stats.chakramCount && c < MAX_CHAKRAMS; c++) {
                    auto& ch = g_Chakrams[c];
                    if (ch.alive) {
                        ch.angle += 5.5f * delta;
                        float chx = pCX + cosf(ch.angle) * CHAKRAM_RADIUS;
                        float chy = pCY + sinf(ch.angle) * CHAKRAM_RADIUS;
                        float hitR2 = CHAKRAM_SIZE * CHAKRAM_SIZE;
                        // Ư���� ? ������/�Ұ��� ����ƽ(�� AI ����). ���⼱ ��ũ�� ������.
                        // ���?���� ���?(Ư���� ���� ��)
                        if (!g_Stats.chakramSingularity) {
                        for (auto m : g_MonsterManager.monsters) {
                            if (!m->alive) continue;
                            float ddx = m->worldX - chx, ddy = m->worldY - chy;
                            if (ddx*ddx + ddy*ddy < hitR2) {
                                m->alive = false;
                                ch.hp -= 2.0f;
                            }
                        }
                        }
                        // ������
                        // ?�거�?�??�촉 ?????�해
                        for (auto rr : g_MonsterManager.rangedMobs) {
                            if (!rr->alive) continue;
                            float ddx = rr->worldX - chx, ddy = rr->worldY - chy;
                            if (ddx*ddx + ddy*ddy < hitR2) { rr->hp -= 120.0f; if (rr->hp <= 0) rr->alive = false; ch.hp -= 3.0f; }
                        }
                        // ??총알 막기
                        for (auto& bb : g_Bullets) {
                            if (!bb.active || !bb.isEnemy) continue;
                            float ddx = bb.x - chx, ddy = bb.y - chy;
                            if (ddx*ddx + ddy*ddy < hitR2) {
                                ch.hp -= 6.0f;
                                bb.active = false;
                            }
                        }
                        if (ch.hp <= 0.0f) {
                            ch.alive = false;
                            ch.respawnTimer = 6.0f;
                        }
                    } else {
                        ch.respawnTimer -= delta;
                        if (ch.respawnTimer <= 0.0f) {
                            ch.alive = true;
                            if (ch.maxHp < 1.0f) ch.maxHp = 150.0f;
                            ch.hp    = ch.maxHp;
                        }
                    }
                }
            }

            if (g_Stats.bulletRain) {
                g_BulletRainTimer += delta;
                // ���� ����(��ȭ): óġ���� ��ٿ�?���� 0.2�� ����. �ּ� 3�� ���� ����.
                if (g_Stats.rainKillReduce && g_RainKillAccum > 0.0f) {
                    float killBoost = g_RainKillAccum * 0.2f;
                    float maxBoost = g_Stats.bulletRainCooldown - 3.0f - g_BulletRainTimer;
                    if (maxBoost < 0.0f) maxBoost = 0.0f;
                    if (killBoost > maxBoost) killBoost = maxBoost;
                    g_BulletRainTimer += killBoost;
                }
                g_RainKillAccum = 0.0f;
                if (g_BulletRainTimer >= g_Stats.bulletRainCooldown) {
                    g_BulletRainTimer = 0.0f;
                    const int N = 20;
                    for (int i = 0; i < N; i++) {
                        float a = (float)i / (float)N * 2.0f * (float)M_PI
                                + ((float)(rand() % 100) - 50.0f) * 0.005f;
                        Bullet nb(pCX, pCY,
                                  pCX + cosf(a) * 100.0f,
                                  pCY + sinf(a) * 100.0f);
                        nb.speed      = g_Stats.bulletSpeed * 1.05f;  // 최고??가???? ??SAM 미사?�식
                        nb.color      = glm::vec3(1.0f, 0.5f, 0.2f);
                        nb.homing     = true;
                        nb.homingTurn = 5.0f;   // rad/s
                        nb.dmgMult    = 0.5f;
                        // ���� �̻��� ? �߻� ���� 5%���� ������ ~0.7�ʿ� 100% ����
                        nb.launchRamp  = 0.05f;
                        nb.launchAccel = 1.35f;
                        // (?�환?��? ??기존 ?�반 ?�환 ?�더�?복원. 로켓 ?�프?�이??미사??
                        g_Bullets.push_back(nb);
                    }
                }
            }



            // Fire bullets and cache the active damage modifiers.
            // ?�탑 모드: ?�레?�어 ?�동 발사 비활??(?�탑???�??발사)
            float effInterval = g_Stats.fireInterval * g_Stats.GetFireIntervalMult();
            float effSpeed    = g_Stats.bulletSpeed + g_Stats.GetBulletSpeedBonus();
            fireTimer += delta;
            auto spawnOne = [&](float tx, float ty) {
                float ftx = tx, fty = ty;
                if (g_Stats.bulletSpread > 0.0f) {
                    float dx = tx - pCX, dy = ty - pCY;
                    float ang = atan2f(dy, dx);
                    float jitter = ((float)(rand() % 200) - 100.0f) / 100.0f
                                 * g_Stats.bulletSpread;
                    ang += jitter;
                    ftx = pCX + cosf(ang) * 200.0f;
                    fty = pCY + sinf(ang) * 200.0f;
                }
                Bullet nb(pCX, pCY, ftx, fty);
                nb.speed = effSpeed;
                if (g_Stats.ricochetMax > 0) {
                    nb.bouncesLeft = g_Stats.ricochetMax;
                    nb.dmgMult *= g_Stats.ricochetDmgMult;
                }
                if (g_Stats.berserk) {
                    float hpFrac = g_Stats.maxHP > 0.0f
                                 ? g_GameManager.playerHP / g_Stats.maxHP : 1.0f;
                    hpFrac = std::max(0.0f, std::min(1.0f, hpFrac));
                    nb.dmgMult *= 1.0f + (1.0f - hpFrac) * 0.6f;
                }
                if (g_PlayerRuntime.dashBoostShotsLeft > 0) {
                    --g_PlayerRuntime.dashBoostShotsLeft;
                    nb.dmgMult *= 2.0f;
                }
                g_Bullets.push_back(nb);
            };

            auto spawnAimed = [&](float tx, float ty) {
                TriggerMuzzle(pCX, pCY, atan2f(ty - pCY, tx - pCX));
                Audio::PlaySfx(Audio::Sfx::Shoot);
                if (g_Stats.twin) {
                    float ang = atan2f(ty - pCY, tx - pCX);
                    const float off = 0.087f;
                    const float r = 200.0f;
                    const int n = (g_Stats.twinCount < 2) ? 2 : g_Stats.twinCount;
                    for (int shot = 0; shot < n; ++shot) {
                        float a = (n > 1)
                            ? ang + (((float)shot / (float)(n - 1)) - 0.5f) * (2.0f * off)
                            : ang;
                        spawnOne(pCX + cosf(a) * r, pCY + sinf(a) * r);
                    }
                } else {
                    spawnOne(tx, ty);
                }
            };

            auto aimTarget = [&](float& tx, float& ty) -> bool {
                if (lmb) { tx = wmx; ty = wmy; return true; }
                return g_MonsterManager.FindNearestEnemy(
                    pCX, pCY, playerWin.x, playerWin.y,
                    playerWin.x + playerWin.width,
                    playerWin.y + playerWin.height, tx, ty);
            };

            // ?�?�??�캔 ?�이?�?(증강) ??0.7초마??조�? 방향 관??�?(군중?�어) ?�?�?
            // Laser sweep uses the shared segment collision path.
            if (g_Stats.laser) {
                float laserInt = (g_Stats.laserTier >= 3) ? 0.26f   // 수렴: 빠르지만 화면을 잠식하지 않음
                               : (g_Stats.laserTier >= 2) ? 0.55f : LASER_INT;
                g_LaserTimer += delta;
                if (g_LaserTimer >= laserInt) {
                    g_LaserTimer -= laserInt;
                    float lang  = atan2f(wmy - pCY, wmx - pCX);   // ?�이?�????�� 커서 방향
                    // ?�거리는 II(760)?�서 ???�리지 ?�음. ?�화 ?�렴?�?'?�비'�?강화.
                    float LASER_RANGE = (g_Stats.laserTier >= 3) ? 700.0f
                                      : (g_Stats.laserTier >= 2) ? 760.0f : 560.0f;
                    float lex = pCX + cosf(lang) * LASER_RANGE, ley = pCY + sinf(lang) * LASER_RANGE;
                    // ?�화 ?�렴(tier3): �??�비 2�???광폭 관??(?�몹 ?�인 ?�소)
                    const float beamW    = (g_Stats.laserTier >= 3) ? 1.5f : 1.0f;
                    const float BEAM_HALF = 24.0f * beamW;
                    bool lcrit = false; float lcm = 1.0f;
                    if (g_Stats.critChance > 0 && (rand()%100) < g_Stats.critChance) { lcrit = true; lcm = g_Stats.critMult; }
                    float lbm = 1.0f;
                    if (g_Stats.berserk) {
                        float hf = g_Stats.maxHP > 0.0f ? g_GameManager.playerHP / g_Stats.maxHP : 1.0f;
                        if (hf < 0.0f) hf = 0.0f; else if (hf > 1.0f) hf = 1.0f;
                        lbm = 1.0f + (1.0f - hf) * 0.6f;
                    }
                    float laserDmgMult = (g_Stats.laserTier >= 3) ? 1.15f : 1.4f;
                    float ldmg = g_Stats.GetBaseDamage() * g_Stats.GetDamageMultiplier()
                               * laserDmgMult * lcm * lbm;
                    auto lOnKill = [&]() {
                        if (g_Stats.lifestealPerKill > 0.0f) {
                            g_GameManager.playerHP += g_Stats.lifestealPerKill;
                            if (g_GameManager.playerHP > g_Stats.maxHP) g_GameManager.playerHP = g_Stats.maxHP;
                        }
                        if (g_Stats.vampire || g_Stats.lifesteal2) {
                            if (++g_Stats.vampireKillStreak >= g_Stats.GetVampireKillNeed()) {
                                g_Stats.vampireKillStreak = 0;
                                g_GameManager.playerHP += 1.0f;
                                if (g_GameManager.playerHP > g_Stats.maxHP) g_GameManager.playerHP = g_Stats.maxHP;
                            }
                        }
                    };
                    auto inLine = [&](float qx, float qy) -> bool {
                        return SegDist(qx, qy, pCX, pCY, lex, ley) < BEAM_HALF;
                    };
                    for (auto m : g_MonsterManager.monsters) {
                        if (!m->alive || !inLine(m->worldX, m->worldY)) continue;
                        float d = ldmg;
                        float dealt = (d < m->hp) ? d : m->hp; m->hp -= dealt;
                        SpawnDamageNumber(m->worldX, m->worldY, dealt, lcrit);
                        if (m->hp <= 0.0f) {
                            m->alive = false; m->scored = true; AddKillCombo();
                            float bx, bs; MobKillReward(m->kind, bx, bs);
                            float rwm = MobRewardMult(m->kind); bx *= rwm; bs *= rwm;
                            const long long pickupXp = (long long)(
                                (bx + MobXpBonus(m->kind, g_Stats)) * g_Stats.xpMult);
                            SpawnStardust(m->worldX, m->worldY,
                                          StardustRewardFor(m->kind), pCX, pCY, pickupXp);
                            g_Stats.RegisterKill(); RegisterCodexMobKill(m->kind);
                            g_GameManager.scoreAccum += bs;
                            g_GameManager.score = (long long)g_GameManager.scoreAccum;
                        }
                    }
                    for (auto rr : g_MonsterManager.rangedMobs) {
                        if (!rr->alive || !inLine(rr->worldX, rr->worldY)) continue;
                        float dealt = (ldmg < rr->hp) ? ldmg : rr->hp; rr->hp -= dealt;
                        SpawnDamageNumber(rr->worldX, rr->worldY, dealt, lcrit);
                        if (rr->hp <= 0.0f) {
                            rr->alive = false; rr->scored = true; AddKillCombo();
                            const long long pickupXp = (long long)(
                                (25.0f + (float)g_Stats.rangedXpBonus) * g_Stats.xpMult);
                            SpawnStardust(rr->worldX, rr->worldY, 3,
                                          pCX, pCY, pickupXp);
                            g_Stats.RegisterKill(); RegisterCodexMobKill(CM_SCOPE);
                            g_GameManager.scoreAccum += 300.0f;
                            g_GameManager.score = (long long)g_GameManager.scoreAccum; lOnKill();
                        }
                    }
                    g_LaserBeams.push_back({ pCX, pCY, lex, ley, 0.13f, 0.13f, beamW });
                    TriggerMuzzle(pCX, pCY, lang);
                }
            }

            bool fireHeld = (g_AutoFire || lmb) && (g_PlayerRuntime.postPickGrace <= 0.0f);
            if (fireHeld && fireTimer >= effInterval) {
                float tx, ty;
                if (aimTarget(tx, ty)) {
                    spawnAimed(tx, ty);
                    fireTimer = 0.0f;
                }
            }
        }

        // GameManager 가 ?�버/변???�태�??????�게 ?�기??(Render ?�서 ?�용)


        g_GameManager.hoveredCard = g_HoveredAug;
        const bool debugCaptureOverride = g_DebugToolkit.CaptureInProgress();
        const GameState debugRestoreState = g_GameManager.currentState;
        if (debugCaptureOverride) {
            // Render each requested scene through the normal dispatch while
            // keeping the real run state untouched between capture frames.
            g_GameManager.currentState =
                g_DebugToolkit.CaptureRenderState(debugRestoreState);
            lmb = false;
            mx = -10000.0;
            my = -10000.0;
        }
        // ============================================================
        // ?�더�?        // ============================================================
        glViewport(0, 0, screenWidth, screenHeight);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(g_MainShader);
        glUniform1i(g_MainFxLoc, g_ShaderFx ? 1 : 0);   // CRT ?�이???�과 ?��? (G)
        // ?�면 ?�들�?+ �??�용 ??game world �? HUD/text(별도 ortho)???�향 ?�음
        float orthoShake[16];
        memcpy(orthoShake, g_BaseOrtho, sizeof(g_BaseOrtho));
        // �?중심 = ZCX/ZCY, 기본 ?�면 중앙). z=1 ?�면 base ortho ?�??�일(identity)
        {
            float z = g_ViewZoom;
            float zcx = ZCX(), zcy = ZCY();
            orthoShake[0]  =  2.0f * z / (float)screenWidth;
            orthoShake[5]  = -2.0f * z / (float)screenHeight;
            orthoShake[12] =  2.0f * zcx * (1.0f - z) / (float)screenWidth  - 1.0f;
            orthoShake[13] =  1.0f - 2.0f * zcy * (1.0f - z) / (float)screenHeight;
        }
        // 게임?�레???�망?�출 ???�태(메뉴·게임?�버 ???�선 ?�면 ?�들�??�여 ?�거
        //   ???�망 ???�작창으�?갔을 ???�면??계속 ?�리??문제 방�?
        {
            GameState gs = g_GameManager.currentState;
            if (gs != GameState::RUNNING && gs != GameState::DYING &&
                gs != GameState::PAUSED && gs != GameState::AUG_SELECT &&
                gs != GameState::DEBUFF_SELECT) {
                g_ShakeTime = 0.0f; g_ShakeMag = 0.0f;
            }
        }
        if (g_ShakeTime > 0.0f) {
            float intensity = g_ShakeMag * (g_ShakeTime / 0.6f);
            if (intensity > g_ShakeMag) intensity = g_ShakeMag;
            float sx = ((float)(rand() % 200) - 100.0f) / 100.0f * intensity;
            float sy = ((float)(rand() % 200) - 100.0f) / 100.0f * intensity;
            orthoShake[12] -= 2.0f * sx / (float)screenWidth;
            orthoShake[13] += 2.0f * sy / (float)screenHeight;
        }
        BatchFlush();   // ortho(�? 바꾸�??????�전 매트�?���??�인 ?�형 먼�? 그림
        glUniformMatrix4fv(g_MainProjLoc, 1, GL_FALSE, orthoShake);
        // 글로벌?�도 ?�기????BindMainShader() 가 ??�??�용
        memcpy(g_MainOrtho, orthoShake, sizeof(orthoShake));
        glBindVertexArray(g_MainVAO);

        // ?�드/?�티???�더??게임?�레???�태?�서�?그린????메뉴(?�작�?????        //   직전 게임???�레?�어 창·잔???�티?��? ?��? ?�태�?비치??문제 방�?.
        //   (g_MainOrtho ???�에???��? 갱신?�으므�?메뉴 ?�스???�모 ?�더???�상)
        {
        GameState wgs = g_GameManager.currentState;
        bool inWorldRender = (wgs == GameState::RUNNING || wgs == GameState::DYING ||
                              wgs == GameState::PAUSED  || wgs == GameState::READY  ||
                              wgs == GameState::AUG_SELECT || wgs == GameState::DEBUFF_SELECT ||
                              wgs == GameState::AUG_REPLACE ||

                              wgs == GameState::GAMEOVER ||
                              (wgs == GameState::SETTINGS &&
                               g_SettingsReturnTo == GameState::PAUSED));
        if (inWorldRender) {

        EnsurePlayerBounds(playerWin);

        // Screen-edge contrast for the HUD. Keep the combat center clear and
        // darken only the perimeter beneath entities, projectiles, and HUD.
        BatchFlush();
        glDisable(GL_SCISSOR_TEST);
        glEnable(GL_BLEND);
        glUniformMatrix4fv(g_MainProjLoc, 1, GL_FALSE, g_BaseOrtho);
        memcpy(g_MainOrtho, g_BaseOrtho, sizeof(g_BaseOrtho));

        constexpr int kVignetteSteps = 24;
        constexpr float kEdgeAlpha = 0.16f;
        const float sideW = std::min(180.0f, std::max(110.0f, screenWidth * 0.09f));
        const float topH = std::min(220.0f, std::max(140.0f, screenHeight * 0.17f));
        for (int i = 0; i < kVignetteSteps; ++i) {
            const float t = (i + 0.5f) / (float)kVignetteSteps;
            const float fade = 1.0f - t * t * (3.0f - 2.0f * t);
            const float a = kEdgeAlpha * fade;
            const float x0 = sideW * i / (float)kVignetteSteps;
            const float x1 = sideW * (i + 1) / (float)kVignetteSteps;
            const float y0 = topH * i / (float)kVignetteSteps;
            const float y1 = topH * (i + 1) / (float)kVignetteSteps;
            drawRect(x0, 0.0f, x1 - x0, (float)screenHeight, 0.0f, 0.0f, 0.0f, a);
            drawRect((float)screenWidth - x1, 0.0f, x1 - x0,
                     (float)screenHeight, 0.0f, 0.0f, 0.0f, a);
            drawRect(0.0f, y0, (float)screenWidth, y1 - y0, 0.0f, 0.0f, 0.0f, a);
            drawRect(0.0f, (float)screenHeight - y1, (float)screenWidth,
                     y1 - y0, 0.0f, 0.0f, 0.0f, a);
        }
        if (g_StrongMenuDim) {
            drawRect(0.0f, 0.0f, (float)screenWidth, (float)screenHeight,
                     0.0f, 0.0f, 0.0f, 0.58f);
        }
        BatchFlush();
        glUniformMatrix4fv(g_MainProjLoc, 1, GL_FALSE, orthoShake);
        memcpy(g_MainOrtho, orthoShake, sizeof(orthoShake));

        // ?�거�?�?FakeWindow ?�기 ?�수 (?�더·?�리??공용)
        const float RFW_W = g_RfwW;
        const float RFW_H = g_RfwH;

        // ============================================================
        // ?�더 z-order (?�래?�위)
        //  (a) ?�거�?�?FakeWindow 배경  ??가???�래
        //  (b) ?�거�?�?�??��????�몹·총알·?�이?�몬??(scissor ?�리??
        //  (c) ?�레?�어 FakeWindow 배경 ???�에????��
        // ============================================================

        // (a0) ?�명 배경 가리기 ??충격??배경 + ?�레그래??배경
        //      glDisable(GL_BLEND) + 불투�??�두???�형 ???�후 FakeWindow �???��?�?
        BatchFlush(); glDisable(GL_BLEND);
        BindMainShader();

        // (a0-1) ?�폭�?충격??배경 ??needsBg ?�래그�? ?�정??충격?�에 ?�해
        for (auto& sw : g_ShockWaves) {
            if (!sw.active || !sw.needsBg) continue;
            float t   = 1.0f - sw.life / sw.maxLife;  // 0??
            float bgR = sw.maxRadius * t + 30.0f;      // 링보??조금 ?�게
            drawCircle(sw.x, sw.y, bgR, 0.08f, 0.08f, 0.10f, 1.0f);
        }

        // (a) FakeWindow 영역은 아래의 스크리저를 위해 유지한다.
        //     시각적으로는 불투명한 창 대신 얇은 신호 프레임만 사용한다.
        // ?�?�?가짜창 ?�합 z-리스??(??��?�높?? 봇넷 < ?�거�?< 보스/?�라??. 같�? ?�?��?
        //    ?�환(벡터) ?�서 = 먼�? ?�환???��? ?�래. 배경+?�온보더�????�서�?'�??�위'
        //    �?그려, ?��? 창의 불투�?배경????? 창의 배경·?�곽?�을 ?�연????��(?�선?�위 가�?.
        //    (?�레?�어 창�? ???�에 ?�로 그려 ??�� 최상??)
        struct FWin { float x, y, w, h; const wchar_t* name;
                      float br, bgc, bbc, nr, ngc, nbc;
                      float gaugePct = -1.0f; float tb = 0.0f; };
        std::vector<FWin> zwins;
        auto addW = [&](float cx, float cy, float w, float h, const wchar_t* nm,
                        float br, float bgc, float bbc, float nr, float ngc, float nbc,
                        float tb = 0.0f, float gaugePct = -1.0f) {
            zwins.push_back({ cx - w*0.5f, cy - h*0.5f, w, h, nm, br,bgc,bbc, nr,ngc,nbc, gaugePct, tb });
        };
        // 봇넷 ?�드 (최하?? ?�환 ?�서)
        for (auto m : g_MonsterManager.monsters) {
            if (!m->alive || m->kind != MobKind::GENESIS) continue;
            float w = GENESIS_WIN_W * m->sizeScale;
            addW(m->worldX, m->worldY, w, w, L"HIVE", 0.06f,0.10f,0.09f, 0.20f,0.85f,0.65f);
        }
        // Swarm app window composite pass.
        // ?�거�?�?(?�환 ?�서)
        for (auto r : g_MonsterManager.rangedMobs) {
            if (r->deathScale <= 0.0f) continue;
            float sc = r->deathScale;
            addW(r->worldX, r->worldY, RFW_W*sc, RFW_H*sc, L"LENS", 0.05f,0.07f,0.12f, 0.20f,0.75f,0.88f);
        }
        // 터렛 신호 프레임. 실제 터렛 영역/스크리저는 그대로 유지한다.
                // z-order signal layer. Scissor/collision geometry is intentionally
        // kept below; only the heavy visual window treatment is removed.
        BatchFlush(); glDisable(GL_SCISSOR_TEST); glEnable(GL_BLEND);
        for (auto& fw : zwins) {
            if (fw.w < 64.0f || fw.h < 64.0f || fw.w != fw.w || fw.h != fw.h) continue;
            const float signalA = (fw.tb > 0.0f) ? 0.26f : 0.22f;
            DrawInGameSignalFrame(fw.x, fw.y, fw.w, fw.h,
                                  fw.nr, fw.ngc, fw.nbc, signalA);
            if (fw.tb > 0.0f) {
                if (fw.gaugePct >= 0.0f) {
                    float pct = fw.gaugePct < 0.0f ? 0.0f : fw.gaugePct > 1.0f ? 1.0f : fw.gaugePct;
                    float gy = fw.y + 4.0f;
                    drawRect(fw.x + 10.0f, gy, fw.w - 20.0f, 2.0f,
                             0.08f, 0.10f, 0.14f, 0.55f);
                    drawRect(fw.x + 10.0f, gy, (fw.w - 20.0f) * pct, 2.0f,
                             fw.nr, fw.ngc, fw.nbc, 0.90f);
                }
            }
        }
        BatchFlush(); glEnable(GL_BLEND);  // ?�후 ?�반 ?�파 블렌??보장

        // (b) Global combat pass.
        // The old implementation rendered the same entities through each
        // fake window's WorldScissor. That made enemies and bullets disappear
        // whenever they crossed a window boundary, and also multiplied draw
        // calls. Fake windows remain visual/collision metadata only.
        BatchFlush();
        glDisable(GL_SCISSOR_TEST);

        // Rear and front sight fields stay before entity bodies to preserve
        // the existing combat render order.
        const float sightPlayerX = playerWin.x + playerWin.width * 0.5f;
        const float sightPlayerY = playerWin.y + playerWin.height * 0.5f;
        const float sightPlayerSize = PLAYER_SIZE * g_Stats.playerSizeMult;
        DrawCombatSightFields(
            g_MonsterManager, sightPlayerX, sightPlayerY, sightPlayerSize,
            g_GameManager.currentState != GameState::DYING &&
                g_GameManager.currentState != GameState::GAMEOVER);

        for (auto m : g_MonsterManager.monsters) {
            if (m->alive) drawMob(m);
        }
        for (auto r : g_MonsterManager.rangedMobs) {
            drawRangedMob(r);
        }
        for (auto& b : g_Bullets) {
            if (b.active) drawBullet(b);
        }
        for (auto& p : g_EnemyParts) {
            if (!p.active) continue;
            float a = p.life / p.maxLife;
            float hs = p.size * 0.5f;
            drawRect(p.x - hs, p.y - hs, p.size, p.size, p.r, p.g, p.b, a);
        }
        for (auto& orb : g_ApproachOrbs) {
            DrawApproachOrb(orb.x, orb.y);
        }
        BatchFlush();

        if (g_GameManager.currentState != GameState::DYING &&
            g_GameManager.currentState != GameState::GAMEOVER) {
            const float pCX = playerWin.x + playerWin.width * 0.5f;
            const float pCY = playerWin.y + playerWin.height * 0.5f;
            const float playerSize = PLAYER_SIZE * g_Stats.playerSizeMult;
            BindMainShader();
            DrawPlayerWeaponShell(pCX, pCY, playerSize,
                                  atan2f(wmy - pCY, wmx - pCX));
            BatchFlush();
        }

        // (c) player playfield signal. The rectangular region itself remains
        // available to gameplay/scissor code, but no opaque fake window is drawn.
        BatchFlush(); glDisable(GL_SCISSOR_TEST); glEnable(GL_BLEND);
        EnsurePlayerBounds(playerWin);



        // (c2) HP/EXP �????�레?�어 �??�단 ?�쪽??부�?(창과 ?�께 ?�동) ?�?�?
        // ?�?�?(h3) 가�?OS �??�롬 ???�?��?�?+ [X] ?�기 (?�스?�톱 ?�계관) ?�?�?
        //    "??= ?�로?�스, 창을 ?�아 종료?�다" ?�체?? ?�드 좌표(�?반영)�?그림.
        {
            GameState gst = g_GameManager.currentState;
            bool inGame = (gst == GameState::RUNNING || gst == GameState::PAUSED ||
                           gst == GameState::DYING || gst == GameState::AUG_SELECT ||
                           gst == GameState::DEBUFF_SELECT);
            if (inGame) {
                auto winChrome = [&](float x, float y, float w, float h,
                                     const wchar_t* title, float tr, float tg, float tb,
                                     float gaugePct = -1.0f) {
                    (void)x; (void)y; (void)w; (void)h;
                    (void)title; (void)tr; (void)tg; (void)tb; (void)gaugePct;
                };
                // 가짜창 ?�?��?�????�에??만든 z-리스????��?�높?? ?�서�?그림.
                //   '?�기보다 ?��? �? ?�는 '?�레?�어 �????�?��?바�? ??���? ?�체�?                //   ?�기??�??�니??**겹친 가�?구간�?* ?�라?�다(부�??�리??.
                //   ?�?��?바의 ?�로 ??y,y+TB]?�?겹치??가림창??x구간??가?�구간에??빼고,
                //   ?��? 구간?�만 glScissor �??�립??그린?? (봇넷<?�거�?보스<?�레?�어,
                //   같�? ?�?��? ?�환?�서)
                const float TBH = 22.0f;
                glEnable(GL_SCISSOR_TEST);
                auto drawBarClipped = [&](float x, float y, float w, float h,
                                          const wchar_t* nm, float nr, float ng, float nb,
                                          size_t selfIdx, bool isPlayer, float gaugePct = -1.0f) {
                    // 가??x구간 리스??(�?vec2: x=?�작, y=??
                    std::vector<glm::vec2> vis{ glm::vec2(x, x + w) };
                    auto subtract = [&](float c0, float c1) {
                        if (c1 <= c0) return;
                        std::vector<glm::vec2> out;
                        for (auto& iv : vis) {
                            float a = iv.x, b = iv.y;
                            if (c1 <= a || c0 >= b) { out.push_back(iv); continue; }
                            if (c0 > a) out.push_back(glm::vec2(a, c0));
                            if (c1 < b) out.push_back(glm::vec2(c1, b));
                        }
                        vis.swap(out);
                    };
                    auto consider = [&](float ox, float oy, float ow, float oh) {
                        if (oy < y + TBH && oy + oh > y)   // 가림창???�?��?�??�로 ?��? 겹침
                            subtract(std::max(x, ox), std::min(x + w, ox + ow));
                    };
                    if (!isPlayer)
                        consider(playerWin.x, playerWin.y, playerWin.width, playerWin.height);
                    for (size_t j = selfIdx + 1; j < zwins.size(); j++)
                        consider(zwins[j].x, zwins[j].y, zwins[j].w, zwins[j].h);
                    // ?��? 구간�??�립??그림
                    for (auto& iv : vis) {
                        float a = iv.x, b = iv.y;
                        if (b - a < 0.5f) continue;
                        WorldScissor(a, y, b - a, TBH);
                        winChrome(x, y, w, h, nm, nr, ng, nb, gaugePct);
                    }
                };
                for (size_t i = 0; i < zwins.size(); i++) {
                    const FWin& fw = zwins[i];
                    drawBarClipped(fw.x, fw.y, fw.w, fw.h, fw.name,
                                   fw.nr, fw.ngc, fw.nbc, i, false, fw.gaugePct);
                }
                BatchFlush();
                // ?�레?�어 �?????�� 최상?? ?�립 ?�이 ?�체
                BatchFlush();
                glDisable(GL_SCISSOR_TEST);
            }
        }

        // Keep health and experience visible on the player's moving playfield.
        {
            const GameState barState = g_GameManager.currentState;
            if (barState == GameState::RUNNING || barState == GameState::PAUSED ||
                barState == GameState::DYING || barState == GameState::AUG_SELECT ||
                barState == GameState::DEBUFF_SELECT) {
                const float pad = 12.0f;
                const float barX = playerWin.x + pad;
                const float barW = std::max(8.0f, playerWin.width - pad * 2.0f);
                constexpr float hpH = 10.0f, xpH = 6.0f, gap = 3.0f;
                const float hpY = playerWin.y + playerWin.height - 16.0f - hpH;
                const float xpY = hpY - gap - xpH;
                const float hpFrac = std::clamp(g_GameManager.playerHP /
                    std::max(1.0f, g_Stats.maxHP), 0.0f, 1.0f);
                const long long xpNeed = g_ExpSystem.Required(g_GameManager.playerLevel);
                const float xpFrac = xpNeed > 0
                    ? std::clamp((float)g_GameManager.xp / (float)xpNeed, 0.0f, 1.0f)
                    : 0.0f;

                BindMainShader();
                drawRect(barX - 2.0f, xpY - 2.0f, barW + 4.0f,
                         hpY + hpH - xpY + 4.0f, 0.0f, 0.0f, 0.0f, 0.72f);
                drawRect(barX, hpY, barW, hpH, 0.20f, 0.05f, 0.05f, 0.95f);
                const float hpR = hpFrac > 0.5f ? 0.1f : 1.0f;
                const float hpG = hpFrac > 0.5f ? 1.0f : hpFrac * 2.0f;
                const float hpGhostFrac = std::clamp(g_HpGhost /
                    std::max(1.0f, g_Stats.maxHP), hpFrac, 1.0f);
                if (hpGhostFrac > hpFrac)
                    drawRect(barX + barW * hpFrac, hpY,
                             barW * (hpGhostFrac - hpFrac), hpH,
                             0.72f, 0.72f, 0.76f, 0.74f);
                drawRect(barX, hpY, barW * hpFrac, hpH,
                         hpR, hpG, 0.1f, 0.97f);
                drawRect(barX, xpY, barW, xpH, 0.06f, 0.10f, 0.07f, 0.95f);
                drawRect(barX, xpY, barW * xpFrac, xpH,
                         0.4f, 1.0f, 0.55f, 0.97f);
            }
        }



        // ?�?�??�기부??UI/?�버?�이: 줌·흔?�기 무시?�고 ?�면 고정 좌표(base ortho)�??�?�?
        //    (?�리모프 2?�이�?�?0.5 ?�서 쿨다?�칸·메뉴?�·비?�트·?�래?��? 찌그?��???버그 fix)
        BatchFlush();   // ?�드(�?ortho) ?�형 ?��? 그린 ??base ortho �??�환
        glUniformMatrix4fv(g_MainProjLoc, 1, GL_FALSE, g_BaseOrtho);
        memcpy(g_MainOrtho, g_BaseOrtho, sizeof(g_BaseOrtho));









        // (h3) ?�면 ?�래?????�벨??보스처치/부???�간 번쩍
        if (g_FlashIntensity > 0.001f) {
            BindMainShader();
            float a = g_FlashIntensity; if (a > 0.85f) a = 0.85f;
            drawRect(0, 0, (float)screenWidth, (float)screenHeight,
                     g_FlashColor.r, g_FlashColor.g, g_FlashColor.b, a);
        }
        // FORK.worm ?�피 ???��???RGB 분리 글리치


        // (h4) ?�간 ?��? ???�면 가?�자�??�안 비네??(?��? ?�출)
        if (g_PlayerRuntime.timeStopTimer > 0.0f) {
            BindMainShader();
            float a = 0.10f + 0.05f * sinf((float)glfwGetTime() * 10.0f);
            float bw = 18.0f;
            drawRect(0, 0, (float)screenWidth, bw, 0.3f, 0.9f, 1.0f, a);
            drawRect(0, (float)screenHeight - bw, (float)screenWidth, bw, 0.3f, 0.9f, 1.0f, a);
            drawRect(0, 0, bw, (float)screenHeight, 0.3f, 0.9f, 1.0f, a);
            drawRect((float)screenWidth - bw, 0, bw, (float)screenHeight, 0.3f, 0.9f, 1.0f, a);
        }

        // (h5) ?�격 ??빨간 가?�자�?비네??(?�격 ?��?
        if (g_HurtVignette > 0.001f) {
            BindMainShader();
            float a = g_HurtVignette * 0.55f;
            float bw = 60.0f * g_HurtVignette + 14.0f;
            drawRect(0, 0, (float)screenWidth, bw, 0.95f, 0.1f, 0.1f, a);
            drawRect(0, (float)screenHeight - bw, (float)screenWidth, bw, 0.95f, 0.1f, 0.1f, a);
            drawRect(0, 0, bw, (float)screenHeight, 0.95f, 0.1f, 0.1f, a);
            drawRect((float)screenWidth - bw, 0, bw, (float)screenHeight, 0.95f, 0.1f, 0.1f, a);
        }


        BatchFlush();
        glDisable(GL_SCISSOR_TEST);

        }   // if (inWorldRender)
        }   // world render game block

        // [6] HUD
        g_GameManager.Render();

        // [6.5] In-game modal backdrop blur. Capture after the world and the
        // GameManager veil, then draw the sharp Scene_* UI on top.
        {
            const GameState blurState = g_GameManager.currentState;
            const bool blurGameplayBackdrop = g_BackdropBlurEnabled &&
                (blurState == GameState::READY ||
                 blurState == GameState::PAUSED ||
                 blurState == GameState::AUG_SELECT ||
                 blurState == GameState::DEBUFF_SELECT ||
                 blurState == GameState::AUG_REPLACE ||
                 blurState == GameState::GAMEOVER ||
                 (blurState == GameState::SETTINGS &&
                  g_SettingsReturnTo == GameState::PAUSED));

            if (blurGameplayBackdrop) {
                InitBlurSystem(screenWidth, screenHeight);
                CaptureBackdrop(BackdropBlurCaptureIntervalSeconds());
                DrawBlurPanel(0.0f, 0.0f,
                              (float)screenWidth, (float)screenHeight,
                              0.72f, 0.004f, 0.010f, 0.020f);
            }
        }

        // ?�?�?[7] ?�국???�스??+ 메뉴 ?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?
        {
            float sw = (float)screenWidth, sh = (float)screenHeight;
            auto  st = g_GameManager.currentState;

            // ���� �� ���̵� ������Ʈ ��������������������������������������������������������������������������������
            if (g_FadeDir == 1) {               // ���̵� �ƿ� (��������)
                g_FadeAlpha += delta / FADE_OUT_DUR;
                if (g_FadeAlpha >= 1.0f) {
                    g_FadeAlpha = 1.0f;
                    g_GameManager.currentState = g_FadeTarget;
                    st = g_FadeTarget;
                    g_AppOpen = 0.0f;
                    g_FadeDir = -1;
                }
            } else if (g_FadeDir == -1) {       // ���̵� �� (���� �� ����)
                g_FadeAlpha -= delta / FADE_IN_DUR;
                if (g_FadeAlpha <= 0.0f) {
                    g_FadeAlpha = 0.0f;
                    g_FadeDir   = 0;
                }
            }


            // ??�?shop/codex/config) 진입 ???�림 ?�니메이???�작 ??�??�레??진행
            {
                static GameState s_prevWinSt = GameState::MAIN_MENU;
                bool isApp = (st == GameState::CODEX ||
                              st == GameState::TUTORIAL || st == GameState::SETTINGS);
                if (st != s_prevWinSt) {
                    if (isApp) g_AppOpen = 0.0f;   // ??�????�기 0?�서 ?�기
                    s_prevWinSt = st;
                }
                if (isApp && g_AppOpen < 1.0f) {
                    g_AppOpen += delta / APP_OPEN_DUR;
                    if (g_AppOpen > 1.0f) g_AppOpen = 1.0f;
                }
            }

            // ?�?�??�게???�업?�시�???메뉴?�??�일??OS ?�레???��? (?�스?�톱 방어 ?��??? ?�?�?
            // ?�?�??�적 ?�금 ?�스??(?�단 중앙 배너, 4�??�시 ???�이?? ?�?�?
            if (g_AchToastTimer > 0.0f && g_AchToastId >= 0 &&
                g_AchToastId < ACH_COUNT) {
                g_AchToastTimer -= delta;
                float a = g_AchToastTimer > 3.0f ? (4.0f - g_AchToastTimer)
                        : std::min(1.0f, g_AchToastTimer);
                if (a < 0.0f) a = 0.0f; if (a > 1.0f) a = 1.0f;
                int li2 = LangIndex();
                const wchar_t* LBL[3] = { L"\uC5C5\uC801 \uB2EC\uC131!", L"Achievement!", L"\u5B9F\u7E3E\u9054\u6210!" };
                wchar_t tb[160];
                swprintf_s(tb, L"%ls  %ls  (+%lld)", LBL[li2],
                           AchName(g_AchToastId), ACH_DEFS[g_AchToastId].coinReward);
                const float toastScale = UiTextScale(g_TextS, UiTextLevel::Description,
                                                     UiScale(sw, sh));
                float bw = g_TextS.Width(tb, toastScale) + 48.0f;
                float bx0 = (sw - bw) * 0.5f, by0 = sh * 0.10f;
                BindMainShader();
                drawRect(bx0, by0, bw, 52.0f, 0.12f, 0.10f, 0.02f, 0.85f * a);
                drawRect(bx0, by0, bw, 4.0f, 1.0f, 0.85f, 0.25f, 0.95f * a);
                g_TextS.Draw(tb, bx0 + 24.0f, by0 + 16.0f, toastScale,
                             1.0f, 0.9f, 0.4f, a);
            }

            DrawKillTags(g_TextS, st == GameState::RUNNING || st == GameState::DYING);

            // ?�?�??�리?�이?�브 HUD (게임 �? ??F:증강  G:무적 ?�?�?
            if (g_CreativeMode &&
                (st == GameState::RUNNING || st == GameState::READY ||
                 st == GameState::AUG_SELECT || st == GameState::DEBUFF_SELECT)) {
                int li3 = LangIndex();
                const wchar_t* CH[3] = {
                    L"CREATIVE   F: \uC99D\uAC15   G: \uBB34\uC801",
                    L"CREATIVE   F: Augment   G: Godmode",
                    L"CREATIVE   F: ?\u5316   G: \u30B4\u30C3\u30C9" };
                g_TextS.Draw(CH[li3], 20.0f, HudY(sh, Hud::CREATIVE_LABEL),
                             UiTextScale(g_TextS, UiTextLevel::Supporting, UiScale(sw, sh)),
                             0.7f, 0.85f, 1.0f, 0.85f);
                if (g_CreativeGodmode) {
                    const wchar_t* GOD[3] = { L"* \uBB34\uC801 ON", L"* GODMODE ON", L"* \u30B4\u30C3\u30C9 ON" };
                    float blink = 0.65f + 0.35f * sinf((float)glfwGetTime() * 5.0f);
                    g_TextL.Draw(GOD[li3], 20.0f, 24.0f,
                                 UiTextScale(g_TextL, UiTextLevel::Description, UiScale(sw, sh)),
                                 1.0f, 0.85f, 0.2f, blink);
                }
            }
            if (g_DebugMode && !g_CreativeMode &&
                (st == GameState::RUNNING || st == GameState::READY ||
                 st == GameState::AUG_SELECT || st == GameState::DEBUFF_SELECT)) {
                int li3 = LangIndex();
                const wchar_t* DBG[3] = {
                    L"\uB514\uBC84\uADF8   F: \uB808\uBCA8\uC5C5   G: \uBB34\uC801",
                    L"DEBUG   F: LEVEL UP   G: GODMODE",
                    L"\u30C7\u30D0\u30C3\u30B0   F: \u30EC\u30D9\u30EB\u30A2\u30C3\u30D7   G: \u7121\u6575"
                };
                g_TextS.Draw(DBG[li3], 20.0f, HudY(sh, Hud::CREATIVE_LABEL),
                             UiTextScale(g_TextS, UiTextLevel::Supporting, UiScale(sw, sh)),
                             0.7f, 0.85f, 1.0f, 0.85f);
                if (g_CreativeGodmode) {
                    const wchar_t* GOD[3] = { L"* \uBB34\uC801 ON", L"* GODMODE ON", L"* \u30B4\u30C3\u30C9 ON" };
                    float blink = 0.65f + 0.35f * sinf((float)glfwGetTime() * 5.0f);
                    g_TextL.Draw(GOD[li3], 20.0f, 24.0f,
                                 UiTextScale(g_TextL, UiTextLevel::Description, UiScale(sw, sh)),
                                 1.0f, 0.85f, 0.2f, blink);
                }
            }

            // ?�?�?[7b] UI ???�스?�치 ??메뉴/�??�태??Scene_* ?�수�?분리 ?�?�?
            //    RUNNING/DYING(?�수 ?�게?????�이 ?�으??컨텍?�트 구성 ?�체�?건너?�?
            // Desktop blur is a window setting, independent of the game scene.
            // World/HUD remain sharp during play; only modal UI blurs game pixels.
            ConfigureWindowBackdropBlur(window, g_BackdropBlurEnabled);

            // Scene_* dispatch still includes READY and the in-run menus;
            // this flag only controls which state gets the scene composition.
            const bool outgameBackdrop =
                st != GameState::RUNNING && st != GameState::DYING;
            if (outgameBackdrop) {
                std::function<void()> resetFn = ResetForNewGame;
                std::function<void()> restartRunFn = RestartCurrentRun;
                std::function<void()> abandonRunFn = AbandonCurrentRun;
                // ������ ũ�� ����(���?64px) ���� Ŭ���� ���� �������� ����
                bool sceneLmb = (!g_DebugToolkit.IsInputCaptured() &&
                                 my >= BROWSER_CHROME_H) ? lmb : false;
                SceneCtx ctx{ sw, sh, mx, my, sceneLmb, delta, window, &fireTimer,
                              resetFn, restartRunFn, abandonRunFn,
                              inputFocusChanged };
                const SceneTextContext sceneText{ g_TextL, g_TextS };
                switch (st) {
                case GameState::MAIN_MENU:         Scene_MainMenu(ctx);         break;
                case GameState::CODEX:             Scene_Codex(ctx);            break;
                case GameState::TUTORIAL: {
                    const TutorialSceneContext tutorialState{
                        g_GameManager.currentState,
                        g_LmbPrev,
                        g_AppOpen,
                        sceneText
                    };
                    Scene_Tutorial(ctx, tutorialState);
                    break;
                }
                case GameState::CREATIVE_CONFIG:   Scene_CreativeConfig(ctx);   break;
                case GameState::SETTINGS:          Scene_Settings(ctx);         break;
                case GameState::READY:             Scene_Ready(ctx, sceneText); break;
                case GameState::PAUSED: {
                    const PauseSceneContext pauseState{
                        g_GameManager.currentState,
                        g_GameManager.pauseResumeState,
                        g_SettingsReturnTo,
                        g_LmbPrev,
                        sceneText,
                        ResetSettingsUi
                    };
                    Scene_Paused(ctx, pauseState);
                    break;
                }
                case GameState::GAMEOVER: {
                    const GameOverSceneContext reportState{
                        g_GameManager.currentState,
                        g_GameOverFade,
                        g_LmbPrev,
                        g_DeathReason,
                        g_LastRunRecord,
                        g_GameManager.score,
                        g_GameManager.playerLevel,
                        g_Stats.killCount,
                        g_OwnedAugs,
                        sceneText
                    };
                    Scene_GameOver(ctx, reportState);
                    break;
                }
                case GameState::AUG_SELECT:
                case GameState::DEBUFF_SELECT:     Scene_AugSelect(ctx);        break;
                case GameState::AUG_REPLACE:       Scene_AugReplace(ctx);       break;
                default: break;
                }
                if (st == GameState::PAUSED || st == GameState::AUG_SELECT ||
                    st == GameState::DEBUFF_SELECT || st == GameState::AUG_REPLACE ||
                    false)
                    Scene_OwnedAugPanel(ctx);
            }
            // Scene_Paused can switch to SETTINGS during this render pass.
            // Apply the new state immediately instead of waiting one frame.
            InputUpdateGameplayCursor(window, g_GameManager.currentState,
                                      g_ShowCrosshair, g_DebugToolkit.IsVisible());

            // ?�단 HUD ???�제 게임 진행 ?�태?�서�?(메뉴/?�감/?�점?????�게)
            if (st == GameState::RUNNING || st == GameState::PAUSED ||
                st == GameState::DYING   || st == GameState::AUG_SELECT ||
                st == GameState::DEBUFF_SELECT) {
                const float hudScale = UiScale(sw, sh);
#ifdef __APPLE__
                const float hudTopY = BROWSER_CHROME_H + 8.0f + 30.0f;
#else
                const float hudTopY = BROWSER_CHROME_H + 4.0f;
#endif
                // 좌상?? Lv. + HP ?�자 (?�각 바는 ?�레?�어 창에 부착됨)
                {
                    wchar_t lvBuf2[64];
                    swprintf_s(lvBuf2, L"%ls%d",
                               T(StrId::LV_PREFIX), g_GameManager.playerLevel);
                    g_TextS.Draw(lvBuf2, 12.0f, hudTopY,
                                 UiTextScale(g_TextS, UiTextLevel::Supporting, hudScale),
                                 0.7f, 1.0f, 0.7f, 0.9f);
                }
                // ?�단 중앙: Score
                wchar_t scoreBuf[64];
                swprintf_s(scoreBuf, L"%ls  %lld", T(StrId::SCORE), g_GameManager.score);
                const float scoreScale = UiTextScale(g_TextS, UiTextLevel::Description, hudScale);
                float scoreW = g_TextS.Width(scoreBuf, scoreScale);
                g_TextS.Draw(scoreBuf, (sw - scoreW) * 0.5f, hudTopY, scoreScale,
                             1.0f, 1.0f, 1.0f, 0.95f);

                // ?�상?? FPS
                wchar_t fpsBuf[32];
                swprintf_s(fpsBuf, L"%ls  %d", T(StrId::FPS), g_CurrentFPS);
                const float fpsScale = UiTextScale(g_TextS, UiTextLevel::Supporting, hudScale);
                float fpsW = g_TextS.Width(fpsBuf, fpsScale);
                g_TextS.Draw(fpsBuf, sw - fpsW - 12.0f, hudTopY, fpsScale,
                             0.7f, 0.9f, 1.0f, 0.85f);

                if (st == GameState::RUNNING) {
                    wchar_t runDustHud[64];
                    const wchar_t* runDustLabel = LangIndex() == 0 ? L"현재 별가루" : L"RUN STARDUST";
                    swprintf_s(runDustHud, L"%ls  %lld", runDustLabel, g_RunStardust);
                    const float dustScale = UiTextScale(g_TextS, UiTextLevel::Supporting, hudScale);
                    float gdw = g_TextS.Width(runDustHud, dustScale);
                    g_TextS.Draw(runDustHud, sw - gdw - 12.0f, hudTopY + 22.0f, dustScale,
                                 1.0f, 0.86f, 0.32f, 0.9f);
                }


                // (HP/EXP ?�각 바는 ?�레?�어 �??�단??부착됨 ????(c2) 참고)
                // ?�단 조작 ?�내 (?��??�게 ??��) ?????�레?�어가 HP/?�킬 ?�치�??�게
                if (st == GameState::RUNNING) {
                    static const wchar_t* kCtrlHint[3] = {
                        L"WASD 이동   마우스 사격   SHIFT 대시   Q/E/R 스킬   ESC 일시정지",
                        L"WASD Move   Mouse Fire   SHIFT Dash   Q/E/R Skills   ESC Pause",
                        L"WASD 移動   マウス 射撃   SHIFT ダッシュ   Q/E/R スキル   ESC 一時停止",
                    };
                    const wchar_t* c = kCtrlHint[LangIndex()];
                    const float hintScale = UiTextScale(g_TextS, UiTextLevel::Supporting, hudScale);
                    float cw = g_TextS.Width(c, hintScale);
                    g_TextS.Draw(c, CenterX(sw, cw), HudY(sh, Hud::COMBO_TEXT), hintScale,
                                 0.7f, 0.82f, 0.95f, 0.72f);   // 가?�성 ??(?�어????보인?�는 ?�드�?
                }
                if (st == GameState::RUNNING && g_GameManager.augReady) {
                    float ap = 0.55f + 0.45f * sinf((float)glfwGetTime() * 4.5f);
                    static const wchar_t* kAugHint[3] = {
                        L"[ SPACE ]  증강 픽업",
                        L"[ SPACE ]  Pick Augment",
                        L"[ SPACE ]  強化 取得",
                    };
                    const wchar_t* hintTxt = kAugHint[LangIndex()];
                    const float augmentHintScale = UiTextScale(g_TextL, UiTextLevel::Subtitle, hudScale);
                    float hw = g_TextL.Width(hintTxt, augmentHintScale);
                    float hy = (float)sh * 0.5f - 60.0f;
                    // glow pass
                    g_TextL.Draw(hintTxt, CenterX(sw, hw) - 1.0f, hy + 1.0f, augmentHintScale,
                                 0.3f, 1.0f, 0.5f, ap * 0.18f);
                    // core
                    g_TextL.Draw(hintTxt, CenterX(sw, hw), hy, augmentHintScale,
                                 0.45f, 1.0f, 0.62f, ap * 0.95f);
                }
                // ?�?�??�체??경고 ??HP 25% ?�하 ??가?�자�?부?�러???�색 ?�스 + ?�스???�?�?
                if (st == GameState::RUNNING || st == GameState::PAUSED) {
                    float hpFrac = (g_Stats.maxHP > 0.0f)
                                 ? g_GameManager.playerHP / g_Stats.maxHP : 1.0f;
                    if (hpFrac > 0.0f && hpFrac < 0.25f) {
                        float pulse = 0.5f + 0.5f * sinf((float)glfwGetTime() * 6.0f);
                        float sev   = 1.0f - hpFrac / 0.25f;
                        float a     = (0.12f + 0.22f * pulse) * (0.6f + 0.4f * sev);
                        BindMainShader();
                        // Build the warning as nested edge bands so the
                        // center of the arena remains readable.  The old
                        // solid 64px frame looked like a hard red box and
                        // became especially harsh at large resolutions.
                        constexpr int kLowHpBands = 8;
                        const float maxBand = 78.0f;
                        for (int band = 0; band < kLowHpBands; ++band) {
                            const float t = (float)(band + 1) / (float)kLowHpBands;
                            const float bandW = maxBand * t;
                            const float bandA = a * (1.0f - t) * 1.55f;
                            drawRect(0.0f, 0.0f, sw, bandW,
                                     0.92f, 0.08f, 0.10f, bandA);
                            drawRect(0.0f, sh - bandW, sw, bandW,
                                     0.92f, 0.08f, 0.10f, bandA);
                            drawRect(0.0f, 0.0f, bandW, sh,
                                     0.92f, 0.08f, 0.10f, bandA);
                            drawRect(sw - bandW, 0.0f, bandW, sh,
                                     0.92f, 0.08f, 0.10f, bandA);
                        }
                        const wchar_t* LOW[3] = { L"! \uC704\uD5D8", L"! LOW HP", L"! \u5371?" };
                        int li4 = LangIndex();
                        const float lowHpScale = UiTextScale(g_TextS, UiTextLevel::Subtitle, hudScale);
                        float lw = g_TextS.Width(LOW[li4], lowHpScale);
                        g_TextS.Draw(LOW[li4], CenterX(sw, lw), HudY(sh, Hud::LOW_HP_WARN),
                                     lowHpScale, 1.0f, 0.4f, 0.4f, 0.55f + 0.45f * pulse);
                    }
                }
            }

            // ?�?�??�맛: ?��?지 ?�자 ?�업 + 콤보 카운??(?�제 ?�레??중에�? ?�?�?
            //    메뉴/?�시?��??�선 ?��? ???�스?��? 메뉴 ?�에 ?�던 버그 fix
            if (st == GameState::RUNNING || st == GameState::DYING) {
                // ?��?지 ?�자 (?�드 ???�크�?변?????�스?? ???�정 ?��?
                for (auto& d : g_DmgNumbers) {
                    float t  = d.life / d.maxLife;                   // 1 ??0
                    float sx = W2SX(d.x), sy = W2SY(d.y);
                    wchar_t nb[16]; swprintf_s(nb, L"%d", d.amount);
                    float sc = UiTextScale(g_TextS, UiTextLevel::Description,
                        (d.crit ? 1.05f : 0.72f) * g_ViewZoom
                        * (0.7f + 0.3f * t) * UiScale(sw, sh));
                    float a  = (t > 0.55f) ? 1.0f : (t / 0.55f);
                    float w  = g_TextS.Width(nb, sc);
                    if (d.crit) {
                        const int critLang = LangIndex();
                        const wchar_t* critLabel = critLang == 0 ? L"치명타"
                            : (critLang == 2 ? L"クリティカル" : L"CRIT");
                        const float critSc = UiTextScale(g_TextS, UiTextLevel::Supporting,
                                                         0.80f + 0.20f * t);
                        const float critW = g_TextS.Width(critLabel, critSc);
                        g_TextS.Draw(critLabel, sx - critW * 0.5f,
                                     sy - g_TextS.Height(critLabel, critSc) - 3.0f,
                                     critSc, 1.0f, 0.74f, 0.18f, a * 0.92f);
                        g_TextS.Draw(nb, sx - w * 0.5f, sy, sc,
                                     1.0f, 0.85f, 0.2f, a);
                    } else {
                        g_TextS.Draw(nb, sx - w*0.5f, sy, sc,
                                     1.0f, 1.0f, 1.0f, a*0.9f);
                    }
                }
                // 콤보 카운??(5콤보 ?�상부?? ?�이 콤보???�라 강해�? ???�정 ?��?
                if (g_ShowCombo && st == GameState::RUNNING && g_Combo >= 5) {
                    bool  ms      = (g_ComboMilestone > 0.0f);
                    float msBoost = ms ? (g_ComboMilestone / 0.7f) : 0.0f;
                    wchar_t cb[32]; swprintf_s(cb, L"%d COMBO", g_Combo);
                    float sc = UiTextScale(g_TextS, UiTextLevel::Subtitle,
                        (1.0f + g_ComboPulse * 0.4f + msBoost * 0.55f) * UiScale(sw, sh));
                    sc *= 1.0f + (g_Combo > 30 ? 0.12f : 0.0f);
                    float cr = 1.0f, cg = 1.0f, cbl = 1.0f;
                    if      (g_Combo >= 50) { cg = 0.25f; cbl = 0.2f; }
                    else if (g_Combo >= 25) { cg = 0.55f; cbl = 0.15f; }
                    else if (g_Combo >= 12) { cg = 0.9f;  cbl = 0.3f; }
                    if (ms) { cr = 1.0f; cg = 0.85f; cbl = 0.30f; }   // 마일?�톤 = 골드 ?�스
                    float w = g_TextS.Width(cb, sc);
                    g_TextS.Draw(cb, (sw - w) * 0.5f, sh * 0.115f, sc, cr, cg, cbl, 0.95f);
                }
            }

            // ?�?�??�티�??�킬 ?�롯 (좌하?? ?�시�?쿨다?????? ?�?�?
            if (st == GameState::RUNNING || st == GameState::PAUSED) {
                const float hudScale = UiScale(sw, sh);
                const float KW = 64.0f * hudScale, KH = 56.0f * hudScale;
                const float KG = 8.0f * hudScale;
                float kx0 = BottomLeftActionX(hudScale);
                float ky0 = HudY(sh, KH + Hud::SKILL_KEYS_Y * hudScale);
                auto skillBox = [&](int idx, const wchar_t* key, const wchar_t* tag,
                                    float cd, float r, float g, float b) {
                    float x = kx0 + idx * (KW + KG), y = ky0;
                    bool ready = (cd <= 0.0f);
                    float bgA = ready ? 0.22f : 0.40f;
                    drawRect(x, y, KW, KH, 0.02f + r*0.04f, 0.02f + g*0.03f, 0.04f + b*0.04f, bgA);
                    drawRect(x, y, KW, 3.5f, r, g, b, ready ? 0.90f : 0.36f);
                    const float cL = 9.0f, ct = 1.2f;
                    float ca = ready ? 0.58f : 0.24f;
                    drawRect(x,       y,       cL, ct, r,g,b, ca);  drawRect(x,       y,       ct, cL, r,g,b, ca);
                    drawRect(x+KW-cL, y,       cL, ct, r,g,b, ca);  drawRect(x+KW-ct, y,       ct, cL, r,g,b, ca);
                    drawRect(x,       y+KH-ct, cL, ct, r,g,b, ca);  drawRect(x,       y+KH-cL, ct, cL, r,g,b, ca);
                    drawRect(x+KW-cL, y+KH-ct, cL, ct, r,g,b, ca);  drawRect(x+KW-ct, y+KH-cL, ct, cL, r,g,b, ca);
                    const float tagScale = UiTextScale(g_TextS, UiTextLevel::Supporting, hudScale);
                    g_TextS.Draw(key, x + 4.0f * hudScale, y + 7.0f * hudScale,
                                 tagScale, 1,1,1, ready ? 0.90f : 0.50f);
                    g_TextS.Draw(tag, x + 4.0f * hudScale, y + KH - 17.0f * hudScale,
                                 tagScale, r,g,b, ready ? 1.0f : 0.42f);
                    if (!ready) {
                        wchar_t bf[8]; swprintf_s(bf, L"%d", (int)(cd + 0.99f));
                        const float cooldownScale = UiTextScale(g_TextL, UiTextLevel::Description, hudScale);
                        float tw = g_TextL.Width(bf, cooldownScale);
                        g_TextL.Draw(bf, x + (KW - tw) * 0.5f, y + KH * 0.34f,
                                     cooldownScale, 1,1,1,0.88f);
                    }
                };
                skillBox(0, L"SHIFT", g_Stats.dashUpgrade ? L"FLASH" : L"DASH",
                         g_PlayerRuntime.dashCooldown, 0.4f, 1.0f, 1.0f);
                const wchar_t* keys3[3] = { L"Q", L"E", L"R" };
                for (int i = 0; i < 3; i++) {
                    if (g_Skills[i].type == SkillType::NONE) continue;
                    const wchar_t* tag = L""; float r = 1, g = 1, b = 1;
                    switch (g_Skills[i].type) {
                    case SkillType::CLOSE_WINDOW: tag = L"CLOSE"; r=0.5f; g=0.8f; b=1.0f; break;
                    case SkillType::HYPER_FOCUS:  tag = L"FOCUS"; r=0.55f; g=0.85f; b=1.0f; break;
                    case SkillType::TIME_STOP:    tag = L"TIME";  r=0.4f; g=0.9f; b=1.0f; break;
                    default: break;
                    }
                    skillBox(i + 1, keys3[i], tag, g_Skills[i].cd, r, g, b);
                }
            }

            // ?�?�??�티�??�시�?쿨다??UI (좌하?? ?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?
            // 추후 ?�토그램 PNG 가 ?�어?�면 ?�각??placeholder ?�리???�스�??�시
            if (st == GameState::RUNNING || st == GameState::PAUSED) {
                const float hudScale = UiScale(sw, sh);
                const float SLOT_W = 64.0f * hudScale, SLOT_H = 56.0f * hudScale;
                const float SLOT_GAP = 8.0f * hudScale;
                float baseX  = BottomLeftActionX(hudScale);
                float baseY2 = HudY(sh, SLOT_H + Hud::SLOT_BAR_BASE);   // HP �??�쪽(?�업?�시�???
                int   slot   = 0;

                auto drawSlot = [&](const wchar_t* tag, float remain,
                                    float r, float g, float b) {
                    float x = baseX + slot * (SLOT_W + SLOT_GAP);
                    float y = baseY2;
                    // 배경
                    drawRect(x, y, SLOT_W, SLOT_H, 0.05f, 0.05f, 0.08f, 0.85f);
                    // 진행??(?�→?�래 채워지지 ?��? 부�?= 쿨�???
                    if (remain > 0.0f) {
                        // ?�두???�버?�이 (?��? 비율만큼 ?�에?��???채�?)
                        // remain ?�규?�는 ?�출 ?�점?�서 처리?�기 ?�려?�니 alpha 0.55 고정
                        drawRect(x, y, SLOT_W, SLOT_H, 0.0f, 0.0f, 0.0f, 0.55f);
                    }
                    // 컬러 ?�두�?(?�쪽 ??
                    drawRect(x, y, SLOT_W, 4.0f, r, g, b, 1.0f);
                    // ?�그 (?�문/?�어 ???�토그램 ?�어?�면 ?�거)
                    const float tagScale = UiTextScale(g_TextS, UiTextLevel::Supporting, hudScale);
                    g_TextS.Draw(tag, x + 4.0f * hudScale, y + 6.0f * hudScale,
                                 tagScale, r, g, b, 1.0f);
                    // ?��? ?�간 (?�수)
                    if (remain > 0.0f) {
                        wchar_t buf[16];
                        swprintf_s(buf, L"%d", (int)(remain + 0.99f));
                        const float cooldownScale = UiTextScale(g_TextL, UiTextLevel::Description, hudScale);
                        float tw = g_TextL.Width(buf, cooldownScale);
                        g_TextL.Draw(buf, x + (SLOT_W - tw) * 0.5f,
                                     y + SLOT_H * 0.40f, cooldownScale, 1,1,1,0.95f);
                    }
                };

                // ?�환 ?��? ??쿨다??(20 / 15 / 7.5)
                if (g_Stats.bulletRain) {
                    float remain = g_Stats.bulletRainCooldown - g_BulletRainTimer;
                    if (remain < 0) remain = 0;
                    drawSlot(L"RAIN", remain, 1.0f, 0.5f, 0.2f);
                    ++slot;
                }
                if (g_Stats.lightStep && g_Stats.lightStepDisableTimer > 0.0f) {
                    drawSlot(L"LSTP", g_Stats.lightStepDisableTimer,
                             0.7f, 0.7f, 0.85f);
                    ++slot;
                }
                // ?��??�는 죽음 ???�성 + stack (?�도 +20%/?�택)
                if (!g_ApproachOrbs.empty()) {
                    float x = baseX + slot * (SLOT_W + SLOT_GAP);
                    drawRect(x, baseY2, SLOT_W, SLOT_H, 0.20f, 0.0f, 0.0f, 0.85f);
                    drawRect(x, baseY2, SLOT_W, 4.0f, 1.0f, 0.1f, 0.1f, 1.0f);
                    g_TextS.Draw(L"DEATH", x + 2.0f * hudScale,
                                 baseY2 + 6.0f * hudScale,
                                 UiTextScale(g_TextS, UiTextLevel::Supporting, hudScale),
                                 1.0f, 0.4f, 0.4f, 1.0f);
                    wchar_t buf[8];
                    swprintf_s(buf, L"x%d", g_Stats.approachStacks);
                    const float stackScale = UiTextScale(g_TextL, UiTextLevel::Description, hudScale);
                    float tw = g_TextL.Width(buf, stackScale);
                    g_TextL.Draw(buf, x + (SLOT_W - tw) * 0.5f,
                                 baseY2 + SLOT_H * 0.40f, stackScale, 1,1,1,0.95f);
                    ++slot;
                }
            }

        }

        if (!g_DebugToolkit.SuppressOverlayForCapture())
            g_DebugToolkit.Render((float)screenWidth, (float)screenHeight,
                                  g_GameManager.currentState);

        // Keep the physical mouse state for edge detection. `lmb` may be
        // cleared above when the debug toolkit captures input; storing that
        // cleared value makes a held button look like a new click every frame.
        g_LmbPrev = rawLmb;
        g_RmbPrev = rawRmb;

        // ?�적 ?�금 / ?�감 발견 발생 ???�??(게임 �?즉시 ?�구??
        if (g_AchSaveNeeded || g_CodexDirty) {
            SaveGame(); g_AchSaveNeeded = false; g_CodexDirty = false;
        }

        BatchFlush();   // ?�레??마�?�????��? ?�형 모두 그림
        if (g_DebugToolkit.SuppressOverlayForCapture())
            g_DebugToolkit.CaptureFrameAfterRender(
                screenWidth, screenHeight, g_GameManager.currentState);
        glfwSwapBuffers(window);
        if (debugCaptureOverride)
            g_GameManager.currentState = debugRestoreState;

        // ?�?�?FPS �?(g_FpsCap > 0 ???�만) ?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?
        // timeBeginPeriod(1) �?Sleep ?�상??1ms. 마�?�?~1ms ??busy-wait
        // C18: 과거 '무제??(-1) ?�이브는 300 ?�로 ?�램??(진짜 무제???�거).
        int capFps = (g_FpsCap < 0) ? 300 : g_FpsCap;
        if (capFps > 0) {
            double target = 1.0 / (double)capFps;
            double frameStart = (double)now;
            double remain = target - (glfwGetTime() - frameStart);
            if (remain > 0.001) {
                unsigned ms = (unsigned)((remain - 0.001) * 1000.0);
                PlatformSleepMs(ms);
            }
#ifndef _WIN32
            // macOS/Linux: ������ ���� busy-spin�� CPU���߿��� �� ª�� sleep
            while (glfwGetTime() - frameStart < target) {
                double left = target - (glfwGetTime() - frameStart);
                if (left > 0.003)
                    PlatformSleepMs(1);
                else
                    break;
            }
#else
            while (glfwGetTime() - frameStart < target) { /* spin */ }
#endif
        }
    }

    g_GameplayTelemetry.EndRun(g_GameTime,
                               g_GameManager.playerLevel,
                               g_GameManager.xp,
                               g_Stats,
                               "application_exit");
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glDeleteVertexArrays(1, &g_MainVAO);
    glDeleteBuffers(1, &g_VBO);
    glDeleteProgram(g_MainShader);
    g_TextL.Cleanup();
    g_TextS.Cleanup();
#ifdef _WIN32
    if (g_FontMemHandle) {
        RemoveFontMemResourceEx(g_FontMemHandle);
        g_FontMemHandle = nullptr;
    }
#endif
    Audio::Shutdown();
    PlatformTimerEnd();
    ReleaseWindowTaskbarPolicy(window);
    glfwTerminate();
    return 0;
}
