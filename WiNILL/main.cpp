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
#include "Bomber.h"
#include "Bullet.h"
#include "MonsterManager.h"
#include "ReloadRunnerBoss.h"
#include "CentipedeBoss.h"
#include "TesseractGlitchBoss.h"
#include "EtherSwordBoss.h"
#include "BossDirector.h"
#include "AugmentSlots.h"
#include "RunIntermission.h"
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
#include "MainShader.h"
#include "EntityDraw.h"
#include "Scenes.h"
#include "SceneContext.h"
#include "SceneSkills.h"
#include "Input.h"
#include "UiLayout.h"
#include "WindowChrome.h"

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

struct BrokenSightOrb {
    float x = 0, y = 0;
    float vx = 0, vy = 0;
    float wanderTimer = 0.0f;
    bool  active = false;
} g_Orb;

// ?��??�는 죽음 (?�버?? ??죽�? ?�고 ?�원??추격?�는 빨간 ?�각??(?�러 �?가??
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

// ?�탑 (CANNON + DRONE_2 조합)
//   1초마???�레?�어 ?�치??1개씩 배치, �?5�?지????맵에 ~5�??�시
//   ?�력치는 '?�??가 ?�니??'?�총' 기�?(g_TurretStats)?�로 계산
struct Turret {
    float x = 0.0f, y = 0.0f;
    float lifeTimer = 0.0f;   // 0?�TURRET_LIFE
    float fireTimer = 0.0f;
};
static const int MAX_TURRETS = 8;                 // ?�전 ?�한
// �??�기 ??g_Scale �??�작 ???�괄 축소 가?�하?�록 ?��???�?(constexpr ??변??
float TURRET_WIN_W  = 250.0f;
float TURRET_WIN_H  = 250.0f;
// ?�규 보스 개인 �??�기 (본체/HP �?가?�는 ?�라?�니??�?
float RR_WIN_W     = 600.0f;
float POLY_WIN_W   = 840.0f;
float BOTNET_WIN_W = 880.0f;   // C2_RELAY: �͹̳� + ȣ��Ʈ ��
float CENTI_WIN_W = 600.0f;    // FORK.worm: ��ü ��¥ â(PID ü�� â ���� ����)
float TESS_WIN_W  = 620.0f;    // TESS.glitch: tesseract grid arena
float ETHER_WIN_W = 900.0f;    // ETHER_SWORD_MASTER.sys: �Ʒ��� ���� â
float TOTEM_WIN_W = 760.0f;    // ADUN.relay: ���� �߰� �ھ� ���� ����
float SPAM_WIN_W   = 480.0f;   // SPAM.dll: �ھ� + ���� �˾� Ŭ���� ���� ����
float UNKNOWN_WIN_W = 500.0f;  // UNKNOWN.sys: â�� �� ����
float UNKNOWN_WIN_H = 580.0f;
// 봇넷 ?�드(SPAWNER) 개인 ?��? �???고정 ???�기 가�?창을 ?��? (E21)
float SPAWNER_WIN_W = 300.0f;
float DDOS_WIN_W    = 210.0f;
// ?�거�?�?FakeWindow ?�기 (?�더/?�리??공용) ???�작 ??g_Scale ?�용
float g_RfwW = 500.0f, g_RfwH = 500.0f;
static constexpr float TURRET_LIFE   = 5.0f;
static constexpr float TURRET_DEPLOY = 1.0f;
std::vector<Turret> g_Turrets;
float       g_TurretDeployTimer = 0.0f;
PlayerStats g_TurretStats;                        // ���� ���� ȭ��


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
float g_BossLifestealDamageBank = 0.0f;
static constexpr float BOSS_LIFESTEAL_DAMAGE_STEP = 200.0f;

static void ApplyBossLifestealFromDamage(float dealt) {
    if (dealt <= 0.0f) return;
    float heal = g_Stats.GetLifestealPerKill();
    if (g_Stats.vampire || g_Stats.lifesteal2)
        heal = std::max(heal, 0.12f);
    if (heal <= 0.0f || g_Stats.maxHP <= 0.0f) return;

    g_BossLifestealDamageBank += dealt;
    while (g_BossLifestealDamageBank >= BOSS_LIFESTEAL_DAMAGE_STEP) {
        g_BossLifestealDamageBank -= BOSS_LIFESTEAL_DAMAGE_STEP;
        g_GameManager.playerHP = std::min(g_Stats.maxHP, g_GameManager.playerHP + heal);
    }
}

// 취함 (?�버?? ??20�??�이??�?5초간 ?�덤 방향 ?�격
float g_DrunkCycle  = 0.0f;
bool  g_DrunkActive = false;

// LIGHT_STEP ?�격 감�????�전 HP 기록
float g_PrevHP = 100.0f;

// 초당 EXP ?�적??(?�버?????��??�는 죽음, ?�몹 가??
float g_XpTimeAccum = 0.0f;

// 게임 ?�작 ??경과 ?�간 (?�폭�?보스 ?�장 ?�?�밍)
float g_GameTime         = 0.0f;
float g_BomberSpawnTimer = 0.0f;

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
struct SlashFx {
    float x, y, ang, range, life, maxLife;
    bool  active = false;
};
static const int MAX_SLASH = 8;
SlashFx g_Slashes[MAX_SLASH] = {};

// 궁수 차징 (0~1) ??LMB ?�른 만큼 충전, ?�면 발사
float g_ArcherCharge = 0.0f;
static const float BOW_CHARGE_TIME = 0.9f;   // ?�충까�? �?
// 머즐 ?�래??(발사 ?�간 총구 ?�광)
float g_MuzzleX = 0.0f, g_MuzzleY = 0.0f, g_MuzzleAng = 0.0f, g_MuzzleTimer = 0.0f;
inline void TriggerMuzzle(float x, float y, float ang) {
    g_MuzzleX = x; g_MuzzleY = y; g_MuzzleAng = ang; g_MuzzleTimer = 0.05f;
}
static void SpawnSlash(float x, float y, float ang, float range) {
    for (int i = 0; i < MAX_SLASH; i++) {
        if (!g_Slashes[i].active) {
            g_Slashes[i] = { x, y, ang, range, 0.18f, 0.18f, true };
            return;
        }
    }
}

// ?�면 ?�들�?(보스 ?�폰, 충격??
float g_ShakeTime = 0.0f;
float g_ShakeMag  = 0.0f;

// 보스 보상 ???��? 버프 ????(?�버???�이지 skip)
int  g_BossRewardPicksLeft = 0;
// 보스 ?�폰 ???�반 보스??20만점마다, ?�리모프??50만점 고정(1??
static constexpr long long FIRST_BOSS_SCORE = 50000;
long long g_NextBossScore  = FIRST_BOSS_SCORE;
bool      g_CreativeBossPending = false;
ReloadRunnerBoss* g_RRBoss  = nullptr;
EtherSwordBoss*   g_EtherBoss = nullptr;

static void HitEtherBossDirect(float dmg, float px, float py, bool crit = false) {
    auto* eb = g_EtherBoss;
    if (!eb || !eb->alive || eb->BodyInvulnerable()) return;

    dmg *= eb->PlayerDamageToBossMul(px, py);
    float killFloor = eb->taskKillHp();
    float dealt = std::min(dmg, std::max(0.0f, eb->hp - killFloor));
    if (dealt > 0.0f) {
        eb->hp -= dealt;
        SpawnDamageNumber(eb->worldX, eb->worldY, dealt, dealt >= 40.0f || crit);
        ApplyBossLifestealFromDamage(dealt);
    }
    if (eb->hp <= killFloor) {
        eb->hp = killFloor;
        eb->BeginTaskKill(px, py, g_Bullets);
    }
}

static void DisplaceEntitiesInWindow(float rx, float ry, float rw, float rh,
                                     float dx, float dy) {
    if (std::fabs(dx) < 0.01f && std::fabs(dy) < 0.01f) return;
    const float pad = 48.0f;
    auto overlaps = [&](float x, float y) {
        return x >= rx - pad && x <= rx + rw + pad &&
               y >= ry - pad && y <= ry + rh + pad;
    };
    for (auto m : g_MonsterManager.monsters) {
        if (!m->alive) continue;
        if (overlaps(m->worldX, m->worldY)) {
            m->worldX += dx;
            m->worldY += dy;
        }
        if (m->kind == MobKind::BLINKER && m->blinkWarn &&
            overlaps(m->blinkTargetX, m->blinkTargetY)) {
            m->blinkTargetX += dx;
            m->blinkTargetY += dy;
        }
    }
    for (auto bm : g_MonsterManager.bombers) {
        if (!bm->alive) continue;
        if (overlaps(bm->worldX, bm->worldY)) {
            bm->worldX += dx;
            bm->worldY += dy;
        }
    }
    for (auto r : g_MonsterManager.rangedMobs) {
        if (r->deathScale <= 0.0f) continue;
        if (overlaps(r->worldX, r->worldY)) {
            r->worldX += dx;
            r->worldY += dy;
        }
    }
    for (auto& p : g_EnemyParts) {
        if (!p.active) continue;
        if (overlaps(p.x, p.y)) { p.x += dx; p.y += dy; }
    }
}


CentipedeBoss* g_CentiBoss = nullptr;
TesseractGlitchBoss* g_TessBoss = nullptr;
// ?�?�?배드 ?�터 ?�망 ?�류�????�시 감속 구역(?�상 ?�역). ?�에 ?�으�??�동?�도 -10% ?�?�?
//   즉시 ?�기지 ?�고 ZONE_OPEN(0.7�???걸쳐 ?�점 부?�되???�짐(grow factor = age/OPEN).
struct SlowZone { float x, y, w, h, life, maxLife, age; };
std::vector<SlowZone> g_SlowZones;
float g_BadSectorBleed = 0.0f;
static constexpr float SLOWZONE_OPEN = 0.7f;
inline float SlowZoneGrow(const SlowZone& z) {
    float g = z.age / SLOWZONE_OPEN;
    return g < 0.0f ? 0.0f : (g > 1.0f ? 1.0f : g);
}
inline void SpawnBadSectorZone(const Monster* m) {
    if (m->kind != MobKind::BADSECTOR) return;
    float w = 300.0f, h = 300.0f;   // 범위 +25% (240 ??300)
    g_SlowZones.push_back({ m->worldX - w*0.5f, m->worldY - h*0.5f, w, h, 5.0f, 5.0f, 0.0f });
}

// ?�?�??�캔 ?�이?�?(증강) ??주기??관??�?+ ?�이??비주???�?�?
struct LaserBeam { float ox, oy, ex, ey, life, maxLife; float width = 1.0f; };
std::vector<LaserBeam> g_LaserBeams;
float          g_LaserTimer = 0.0f;
constexpr float LASER_INT   = 0.85f;  // 발사 주기(�? ???�프: 0.7


// ?�?�?백신 ?�캔 (증강) ??주기?�으�??�레?�어 주�????�화 ?�스(범위 ?�소) ?�?�?
//   ?�각 링�? 기존 SpawnShockWave(?�창 �? ?�사??
float          g_NovaTimer  = 0.0f;
constexpr float NOVA_INT    = 2.4f;    // ?�스 주기(�? 중첩 ???�축)
constexpr float NOVA_R      = 240.0f;  // 기본 반경(중첩 ???��?)

// ?�?�?보스 ?�장 ?�조(증상) ?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?
//   보스 ?�폰??"결정 ??2.5�??�조(?�마 증상 + 경고 배너) ???�제 ?�성" ?�로 분리.
//   ?�조 ?�안 게임?�레?�는 계속(?�레그래??. 만료 ??결정??보스�??�제�??�성.
float          g_BossWarnTimer = 0.0f;          // >0 ?�면 ?�조 진행 �?(?��? ?�간)
constexpr float BOSS_WARN_DUR  = 2.5f;
int            g_BossWarnPick   = -1;           // 0~8 보스 ?�일 ?�덱??(4=?�리)
const wchar_t* g_BossWarnName   = L"";
float          g_BossWarnHp      = 0.0f;
// ?�이�? ?�승?��? 추적 (?�일 진입 ?�출 1???�생??
bool g_RRWasP2 = false, g_RRWasP3 = false;
bool g_TessWasP2 = false, g_TessWasP3 = false;
// ?�이�? 진입 ?�스??("??과�?????PHASE 2")
float     g_P2ToastTimer = 0.0f;
glm::vec3 g_P2ToastCol   = glm::vec3(1.0f);
bool  g_LastRunRecord   = false;  // 직전 ?�이 ?�기록이?�는지 (GAMEOVER ?�시??
int   g_MetaStartAugs   = 0;      // 메�? ?�금: ?�작 무료 증강 ???�수

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

// ?�?�??�티�??�킬 ?�스?????�??기본) + ?�롯 3�?증강 ?�득, �?차면 교체) ?�?�?
float g_DashCd        = 0.0f;
float g_DashInvuln    = 0.0f;
float g_PostPickGrace = 0.0f;      // C14: 증강 ????짧�? ?�예(무적+발사?�제)�?복�? ?�?
float g_TimeStopTimer = 0.0f;
float g_HyperFocusTimer = 0.0f;

void SyncPlayerBoundsSize(SpatialBounds& pw, float delta, bool animate) {
    if (g_Stats.windowSize < 64.0f)
        g_Stats.windowSize = std::max(400.0f * g_Scale, 64.0f);
    float targetWin = g_Stats.windowSize *
        ((g_HyperFocusTimer > 0.0f) ? 1.5f : 1.0f);
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
        cx = (float)screenWidth  * 0.5f;
        cy = (float)screenHeight * 0.5f;
    }
    pw.width = pw.height = pwSz;
    pw.x = cx - pwSz * 0.5f;
    pw.y = cy - pwSz * 0.5f;
}

static void EnsurePlayerBounds(SpatialBounds& pw) {
    SyncPlayerBoundsSize(pw, 1.0f, false);
}

static constexpr float DASH_CD = 2.9f, DASH_DIST = 300.0f, DASH_INVULN = 0.20f;
static constexpr float DASH_DUR  = 0.11f;
static bool  g_DashActive = false;
static float g_DashT      = 0.0f;
static float g_DashFromX  = 0.0f, g_DashFromY = 0.0f;
static float g_DashToX    = 0.0f, g_DashToY   = 0.0f;
static float g_FocusStandTimer = 0.0f;
static int   g_FocusShotsLeft  = 0;
static int   g_RevolverRound   = 0;
static int   g_DashBoostShotsLeft = 0;
static constexpr float TIMESTOP_DUR = 1.5f, HYPER_FOCUS_DUR = 5.0f;
static constexpr float HYPER_FOCUS_DASH_CD = 2.0f;

static float SkillCooldownMax(SkillType t) {
    switch (t) {
    case SkillType::CLOSE_WINDOW: return 16.0f;
    case SkillType::HYPER_FOCUS:  return 20.0f;
    case SkillType::TIME_STOP:    return 28.0f;
    case SkillType::FOCUS_AIM:    return 14.0f;
    default:                      return 0.0f;
    }
}
static void ResetSkills() {
    for (int i = 0; i < 3; i++) g_Skills[i] = { SkillType::NONE, 0.0f };
    g_SkillReplaceIdx = 0; g_DashCd = 0; g_DashInvuln = 0;
    g_DashActive = false; g_DashT = 0.0f;
    g_FocusStandTimer = 0.0f; g_FocusShotsLeft = 0; g_RevolverRound = 0;
    g_DashBoostShotsLeft = 0;
    g_PlayerShield = 0.0f; g_PlayerShieldTimer = 0.0f;
    g_TimeStopTimer = 0; g_HyperFocusTimer = 0;
    g_BossLifestealDamageBank = 0.0f;
}
// 보스 ?�존 ?�안 ?�면 ?�체�?보스 고유?�으�??�점 물들?�는 ?�출
float     g_BossTintT   = 0.0f;                     // 0..1 (?�존 ???�승, ?�망 ???�강)
glm::vec3 g_BossTintCol = glm::vec3(0.6f, 0.3f, 1.0f);

// HUD: ?�재 측정 FPS (?�단 ?�측 ?�시)
int    g_CurrentFPS  = 0;
double g_FpsLastTime = 0.0;
int    g_FpsFrames   = 0;

// ���� ���� �ε��� ���?(���� �������? �ߺ� ���� ����)
std::vector<int> g_OwnedAugs;

// ũ������Ƽ�� ���?���� ���� ���� ���� (�ε���). ���� ���� �� �ϰ� ����.
std::vector<int> g_CreativeStartAugList;
bool g_CreativeStartPending = false;   // ?�용 ?��?(main 루프가 applyByIdx �?처리)
std::deque<int> g_AugEffectQueue;
bool g_AugEffectProcessing = false;

// 증강 ?�택 hover state (-1 = 미선?? 0/1/2 = 카드 ?�덱??
int  g_HoveredAug    = -1;
bool g_EnterReleased = true;

// 마우???�릭 edge 감�? (?�전 ?�레??left button ?�태)
bool g_LmbPrev = false;

// PAUSED ?�태 ??보유 증강 ?�릭 ???�명 ?�시 (-1 = ?�음, 0..AUG_TOTAL-1 = ?�덱??
int g_PauseSelectedAug = -1;

// ?�정 ?�면 진입 ???�전 ?�태 (?�로 가�???복�?)
GameState g_SettingsReturnTo = GameState::MAIN_MENU;

int g_CurrentWeapon = -1;

// 변??카드 ??AUG_SELECT ??25% ?�률�?4번째 카드 ?�장
// ?�재 무기�??�른 StartWeapon ?�로 ?�환 (기존 무기 ?�과 ?�거 ????무기 ?�용)
// �?= ?�환??StartWeapon ?�덱?? -1 = ?�번 ?�운?�는 변??카드 ?�음
int g_ConversionWeapon = -1;

enum class PlayerShellKind {
    Rifle,
    Cannon,
    StaticField
};

static PlayerShellKind CurrentPlayerShellKind() {
    if (g_CurrentWeapon >= 0 && g_CurrentWeapon < (int)StartWeapon::_COUNT) {
        switch ((StartWeapon)g_CurrentWeapon) {
        case StartWeapon::SMG:      return PlayerShellKind::StaticField;
        case StartWeapon::CANNON:   return PlayerShellKind::Cannon;
        case StartWeapon::RIFLE:    return PlayerShellKind::Rifle;
        case StartWeapon::_COUNT:   break;
        default:                    break;
        }
    }
    return PlayerShellKind::Rifle;
}

static void DrawPlayerRotRect(float cx, float cy, float w, float h, float ang,
                              float r, float g, float b, float a) {
    float hx = w * 0.5f, hy = h * 0.5f;
    float c = cosf(ang), s = sinf(ang);
    float x0 = -hx, y0 = -hy;
    float x1 =  hx, y1 = -hy;
    float x2 =  hx, y2 =  hy;
    float x3 = -hx, y3 =  hy;
    auto tx = [&](float x, float y) { return cx + x * c - y * s; };
    auto ty = [&](float x, float y) { return cy + x * s + y * c; };
    BatchTri(tx(x0,y0), ty(x0,y0), tx(x1,y1), ty(x1,y1), tx(x2,y2), ty(x2,y2), r,g,b,a);
    BatchTri(tx(x0,y0), ty(x0,y0), tx(x2,y2), ty(x2,y2), tx(x3,y3), ty(x3,y3), r,g,b,a);
}

static void DrawPlayerLine(float x0, float y0, float x1, float y1, float thick,
                           float r, float g, float b, float a) {
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.5f) return;
    DrawPlayerRotRect((x0 + x1) * 0.5f, (y0 + y1) * 0.5f, len, thick,
                      atan2f(dy, dx), r, g, b, a);
}

static void DrawPlayerArc(float cx, float cy, float radius,
                          float startAngle, float sweepAngle, float thick,
                          float r, float g, float b, float a) {
    if (radius <= 0.0f || thick <= 0.0f || fabsf(sweepAngle) < 0.001f ||
        radius != radius || sweepAngle != sweepAngle) return;
    const float sweepAbs = fabsf(sweepAngle);
    const int segments = std::max(4, std::min(96,
        (int)ceilf(sweepAbs * radius / 9.0f)));
    const float step = sweepAngle / (float)segments;
    float angle = startAngle;
    float px = cx + cosf(angle) * radius;
    float py = cy + sinf(angle) * radius;
    for (int i = 1; i <= segments; ++i) {
        angle = startAngle + step * (float)i;
        float nx = cx + cosf(angle) * radius;
        float ny = cy + sinf(angle) * radius;
        DrawPlayerLine(px, py, nx, ny, thick, r, g, b, a);
        px = nx;
        py = ny;
    }
}

static void DrawPlayerRadialGauge(float cx, float cy, float radius, float fraction,
                                  float startAngle, float sweepAngle, float thick,
                                  float r, float g, float b, float alpha,
                                  int tickCount, float ghostFraction = -1.0f,
                                  float orbitPhase = -1.0f) {
    fraction = std::max(0.0f, std::min(1.0f, fraction));
    if (ghostFraction >= 0.0f)
        ghostFraction = std::max(fraction, std::min(1.0f, ghostFraction));

    // The unfilled portion is a neutral gray track. It stays visible at all
    // times so the gauge reads as a complete instrument, not only as an
    // event flash.
    DrawPlayerArc(cx, cy, radius, startAngle, sweepAngle,
                  std::max(0.8f, thick * 0.72f),
                  0.34f, 0.39f, 0.47f, 0.16f);

    // Instrument ticks turn the progress arc into a compact radial graph.
    if (tickCount > 0) {
        for (int i = 0; i < tickCount; ++i) {
            const float angle = startAngle + sweepAngle * (float)i / (float)tickCount;
            const bool lit = fraction > ((float)i / (float)tickCount);
            const float inner = radius - thick * 0.95f;
            const float outer = radius + thick * 0.95f;
            DrawPlayerLine(cx + cosf(angle) * inner, cy + sinf(angle) * inner,
                           cx + cosf(angle) * outer, cy + sinf(angle) * outer,
                           std::max(0.8f, thick * 0.42f),
                           lit ? r : 0.34f,
                           lit ? g : 0.34f,
                           lit ? b : 0.42f,
                           lit ? alpha * 0.72f : 0.12f);
        }
    }

    // Damage afterimage: the pre-hit HP stays gray behind the live green HP,
    // then contracts toward the current value as ghostFraction eases down.
    if (ghostFraction > fraction + 0.0001f) {
        DrawPlayerArc(cx, cy, radius, startAngle,
                      sweepAngle * ghostFraction, thick * 1.05f,
                      0.52f, 0.56f, 0.62f, alpha * 0.78f);
    }

    if (fraction > 0.001f) {
        const float progress = sweepAngle * fraction;
        DrawPlayerArc(cx, cy, radius, startAngle, progress, thick,
                      r, g, b, alpha);
        const float endAngle = startAngle + progress;
        drawDiamond(cx + cosf(endAngle) * radius,
                    cy + sinf(endAngle) * radius,
                    std::max(3.5f, thick * 2.7f), r, g, b, alpha * 1.05f);
    }

    // The track itself rotates slowly; this small constellation scanner keeps
    // moving along the outer edge so the gauge still feels alive at idle.
    if (orbitPhase >= 0.0f) {
        orbitPhase -= floorf(orbitPhase);
        const float markerAngle = startAngle + sweepAngle * orbitPhase;
        const float markerRadius = radius + 5.0f;
        const float mx = cx + cosf(markerAngle) * markerRadius;
        const float my = cy + sinf(markerAngle) * markerRadius;
        const float tailSweep = (sweepAngle < 0.0f) ? -0.16f : 0.16f;
        DrawPlayerArc(cx, cy, markerRadius, markerAngle, tailSweep,
                      std::max(0.65f, thick * 0.48f), r, g, b, alpha * 0.62f);
        drawDiamond(mx, my, std::max(2.2f, thick * 1.7f),
                    r, g, b, alpha * 0.88f);
    }
}

static void DrawPlayerHexFrame(float cx, float cy, float rad, float ang,
                               float r, float g, float b, float a, float thick,
                               bool nodes) {
    float vx[6], vy[6];
    for (int i = 0; i < 6; ++i) {
        float th = ang + (float)i * 1.04719755f;
        vx[i] = cx + cosf(th) * rad;
        vy[i] = cy + sinf(th) * rad;
    }
    for (int i = 0; i < 6; ++i) {
        int n = (i + 1) % 6;
        DrawPlayerLine(vx[i], vy[i], vx[n], vy[n], thick, r, g, b, a);
    }
    if (nodes) {
        float ns = std::max(4.0f, thick * 3.1f);
        for (int i = 0; i < 6; ++i)
            drawDiamond(vx[i], vy[i], ns, r, g, b, a * 1.12f);
    }
}

static void DrawPlayerTriangleFrame(float cx, float cy, float rad, float ang,
                                    float r, float g, float b, float a,
                                    float thick, bool nodes) {
    float vx[3], vy[3];
    for (int i = 0; i < 3; ++i) {
        float th = ang - 1.5707963f + (float)i * 2.0943951f;
        vx[i] = cx + cosf(th) * rad;
        vy[i] = cy + sinf(th) * rad;
    }
    for (int i = 0; i < 3; ++i) {
        int n = (i + 1) % 3;
        DrawPlayerLine(vx[i], vy[i], vx[n], vy[n], thick, r, g, b, a);
    }
    if (nodes) {
        float ns = std::max(4.0f, thick * 3.0f);
        for (int i = 0; i < 3; ++i)
            drawDiamond(vx[i], vy[i], ns, r, g, b, a * 1.08f);
    }
}

static void DrawPlayerCrossStarFrame(float cx, float cy, float outerRad, float innerRad,
                                     float ang, float r, float g, float b, float a,
                                     float thick, bool cross, bool nodes) {
    float vx[8], vy[8];
    for (int i = 0; i < 8; ++i) {
        float rad = (i & 1) ? innerRad : outerRad;
        float th = ang - 1.5707963f + (float)i * 0.78539816f;
        vx[i] = cx + cosf(th) * rad;
        vy[i] = cy + sinf(th) * rad;
    }

    for (int i = 0; i < 8; ++i) {
        int n = (i + 1) & 7;
        DrawPlayerLine(vx[i], vy[i], vx[n], vy[n], thick, r, g, b, a);
    }

    if (cross) {
        for (int i = 0; i < 8; i += 2)
            DrawPlayerLine(cx, cy, vx[i], vy[i], std::max(0.8f, thick * 0.42f),
                           r, g, b, a * 0.34f);
    }

    if (nodes) {
        float tipSz = std::max(4.0f, thick * 3.0f);
        for (int i = 0; i < 8; i += 2)
            drawDiamond(vx[i], vy[i], tipSz, r, g, b, a * 1.16f);
    }
}

static void DrawPlayerEllipseFrame(float cx, float cy, float rx, float ry, float ang,
                                   float r, float g, float b, float a,
                                   float thick, bool nodes) {
    const int SEG = 48;
    float ca = cosf(ang), sa = sinf(ang);
    auto tx = [&](float lx, float ly) { return cx + lx * ca - ly * sa; };
    auto ty = [&](float lx, float ly) { return cy + lx * sa + ly * ca; };

    float px = tx(rx, 0.0f);
    float py = ty(rx, 0.0f);
    for (int i = 1; i <= SEG; ++i) {
        float th = (float)i / (float)SEG * 6.2831853f;
        float nx = tx(cosf(th) * rx, sinf(th) * ry);
        float ny = ty(cosf(th) * rx, sinf(th) * ry);
        DrawPlayerLine(px, py, nx, ny, thick, r, g, b, a);
        px = nx; py = ny;
    }

    if (nodes) {
        for (int i = 0; i < 4; ++i) {
            float th = (float)i * 1.5707963f + ang * 0.35f;
            float nx = tx(cosf(th) * rx, sinf(th) * ry);
            float ny = ty(cosf(th) * rx, sinf(th) * ry);
            drawDiamond(nx, ny, std::max(4.0f, thick * 3.2f), r, g, b, a * 1.15f);
        }
    }
}

static void DrawPlayerZigzagLine(float x0, float y0, float x1, float y1,
                                 int steps, float amp, float thick,
                                 float r, float g, float b, float a) {
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1.0f) return;
    float nx = -dy / len, ny = dx / len;
    float px = x0, py = y0;
    for (int i = 1; i <= steps; ++i) {
        float t = (float)i / (float)steps;
        float ox = (i == steps) ? 0.0f : (((i & 1) ? 1.0f : -1.0f) * amp);
        float cx2 = x0 + dx * t + nx * ox;
        float cy2 = y0 + dy * t + ny * ox;
        DrawPlayerLine(px, py, cx2, cy2, thick, r, g, b, a);
        px = cx2; py = cy2;
    }
}

// In-game regions remain rectangular for scissor/collision logic, but their
// visual identity is reduced to a light signal frame instead of a fake window.
static void DrawInGameSignalFrame(float x, float y, float w, float h,
                                  float r, float g, float b, float alpha) {
    if (w < 24.0f || h < 24.0f || w != w || h != h) return;
    const float corner = std::max(10.0f, std::min(22.0f, std::min(w, h) * 0.12f));
    drawConstellFrame(x, y, w, h, r, g, b, alpha, corner, 3.0f, alpha * 0.12f, 1.0f);
}

static void DrawPlayerSightMarker(float cx, float cy, float radius,
                                  float r, float g, float b, float alpha) {
    if (radius <= 0.0f || radius != radius) return;
    if (g_ConstellationCircleTex && g_IconProg) {
        BatchFlush();
        const float screenRadius = radius * std::max(0.01f, g_ViewZoom);
        DrawIcon(g_ConstellationCircleTex, W2SX(cx) - screenRadius,
                 W2SY(cy) - screenRadius, screenRadius * 2.0f,
                 screenRadius * 2.0f, r, g, b, alpha);
    } else {
        drawCircle(cx, cy, radius, r, g, b, alpha);
    }
}

void DrawPlayerWeaponShell(float cx, float cy, float sz, float aimAng) {
    if (!(aimAng == aimAng)) aimAng = 0.0f;

    const float r = 0.34f, g = 1.0f, b = 1.0f;
    const float wr = 0.92f, wg = 1.0f, wb = 1.0f;
    const float ar = 1.0f, ag = 0.62f, ab = 0.16f;
    float t = (float)glfwGetTime();
    float kick = (g_MuzzleTimer > 0.0f) ? (g_MuzzleTimer / 0.05f) : 0.0f;
    float pulse = 0.5f + 0.5f * sinf(t * 4.0f);
    auto rotX = [&](float lx, float ly, float ang) {
        return cx + lx * cosf(ang) - ly * sinf(ang);
    };
    auto rotY = [&](float lx, float ly, float ang) {
        return cy + lx * sinf(ang) + ly * cosf(ang);
    };
    auto triRot = [&](float ax, float ay, float bx, float by, float cx2, float cy2,
                      float ang, float rr, float gg, float bb, float aa) {
        BatchTri(rotX(ax, ay, ang), rotY(ax, ay, ang),
                 rotX(bx, by, ang), rotY(bx, by, ang),
                 rotX(cx2, cy2, ang), rotY(cx2, cy2, ang),
                 rr, gg, bb, aa);
    };
    auto triEdge = [&](float ax, float ay, float bx, float by, float cx2, float cy2,
                       float ang, float thick, float rr, float gg, float bb, float aa) {
        float x0 = rotX(ax, ay, ang), y0 = rotY(ax, ay, ang);
        float x1 = rotX(bx, by, ang), y1 = rotY(bx, by, ang);
        float x2 = rotX(cx2, cy2, ang), y2 = rotY(cx2, cy2, ang);
        DrawPlayerLine(x0, y0, x1, y1, thick, rr, gg, bb, aa);
        DrawPlayerLine(x1, y1, x2, y2, thick, rr, gg, bb, aa);
        DrawPlayerLine(x2, y2, x0, y0, thick, rr, gg, bb, aa);
    };

    PlayerShellKind kind = CurrentPlayerShellKind();
    switch (kind) {
    case PlayerShellKind::Cannon: {
        float bodyAng = aimAng;
        float expand = kick * sz * 0.62f;
        triRot(sz * 1.62f, 0.0f, -sz * 1.10f, -sz * 0.96f, -sz * 0.52f, 0.0f,
               bodyAng, 0.0f, 0.0f, 0.0f, 0.68f);
        triRot(sz * 1.62f, 0.0f, -sz * 0.52f, 0.0f, -sz * 1.10f, sz * 0.96f,
               bodyAng, 0.0f, 0.0f, 0.0f, 0.68f);
        triRot(sz * 1.34f, 0.0f, -sz * 0.82f, -sz * 0.68f, -sz * 0.36f, 0.0f,
               bodyAng, r, g, b, 0.44f);
        triRot(sz * 1.34f, 0.0f, -sz * 0.36f, 0.0f, -sz * 0.82f, sz * 0.68f,
               bodyAng, r, g, b, 0.38f);
        triEdge(sz * 1.34f, 0.0f, -sz * 0.82f, -sz * 0.68f, -sz * 0.36f, 0.0f,
                bodyAng, 2.3f, r, g, b, 0.62f);
        triEdge(sz * 1.34f, 0.0f, -sz * 0.36f, 0.0f, -sz * 0.82f, sz * 0.68f,
                bodyAng, 2.3f, r, g, b, 0.54f);

        DrawPlayerRotRect(rotX(-sz * 0.30f, -sz * 0.80f, bodyAng),
                          rotY(-sz * 0.30f, -sz * 0.80f, bodyAng),
                          sz * 1.24f, sz * 0.24f, bodyAng + 0.26f,
                          r, g, b, 0.32f);
        DrawPlayerRotRect(rotX(-sz * 0.30f, sz * 0.80f, bodyAng),
                          rotY(-sz * 0.30f, sz * 0.80f, bodyAng),
                          sz * 1.24f, sz * 0.24f, bodyAng - 0.26f,
                          r, g, b, 0.32f);
        drawDiamond(cx, cy, sz * 0.82f, 0.0f, 0.0f, 0.0f, 0.64f);
        drawDiamond(cx, cy, sz * 0.54f, ar, ag, ab, 0.86f + kick * 0.10f);
        drawDiamond(cx, cy, sz * 0.30f, wr, wg, wb, 0.78f);

        struct PodSpec { float x, y, w, h, a; };
        const PodSpec pods[6] = {
            { 0.48f, -1.34f, 0.68f, 0.36f, -0.08f },
            { 0.82f, -0.82f, 0.72f, 0.38f,  0.08f },
            { 0.48f,  1.34f, 0.68f, 0.36f,  0.08f },
            { 0.82f,  0.82f, 0.72f, 0.38f, -0.08f },
            {-0.86f, -0.70f, 0.58f, 0.34f,  0.20f },
            {-0.86f,  0.70f, 0.58f, 0.34f, -0.20f },
        };
        for (int i = 0; i < 6; ++i) {
            float lx = pods[i].x * sz;
            float ly = pods[i].y * sz;
            float len = sqrtf(lx * lx + ly * ly);
            float ex = (len > 0.01f) ? lx / len * expand : 0.0f;
            float ey = (len > 0.01f) ? ly / len * expand : 0.0f;
            float px0 = rotX(lx + ex, ly + ey, bodyAng);
            float py0 = rotY(lx + ex, ly + ey, bodyAng);
            float linkX = rotX(lx * 0.42f, ly * 0.42f, bodyAng);
            float linkY = rotY(lx * 0.42f, ly * 0.42f, bodyAng);
            float podAng = bodyAng + atan2f(ly, lx) + pods[i].a
                         + sinf(t * 1.8f + (float)i) * 0.06f;
            DrawPlayerLine(linkX, linkY, px0, py0, 1.7f, r, g, b, 0.34f + kick * 0.20f);
            DrawPlayerRotRect(px0, py0, sz * pods[i].w, sz * pods[i].h, podAng,
                              0.0f, 0.0f, 0.0f, 0.58f);
            DrawPlayerRotRect(px0, py0, sz * pods[i].w * 0.72f, sz * pods[i].h * 0.60f, podAng,
                              r, g, b, 0.58f);
            float portX = px0 + cosf(podAng) * sz * pods[i].w * 0.34f;
            float portY = py0 + sinf(podAng) * sz * pods[i].w * 0.34f;
            drawDiamond(portX, portY, sz * (0.18f + kick * 0.04f),
                        ar, ag, ab, 0.84f + kick * 0.12f);
        }
        break;
    }
    case PlayerShellKind::StaticField: {
        struct ZapTarget { float x, y, d2; };
        ZapTarget targets[4];
        int targetCount = 0;
        float fieldRad = std::max(150.0f, sz * 4.3f);
        float fieldR2 = fieldRad * fieldRad;
        auto addTarget = [&](float tx, float ty) {
            float dx = tx - cx, dy = ty - cy;
            float d2 = dx * dx + dy * dy;
            if (d2 > fieldR2) return;
            if (targetCount < 4) {
                targets[targetCount++] = { tx, ty, d2 };
            } else {
                int farIdx = 0;
                for (int i = 1; i < 4; ++i)
                    if (targets[i].d2 > targets[farIdx].d2) farIdx = i;
                if (d2 < targets[farIdx].d2)
                    targets[farIdx] = { tx, ty, d2 };
            }
        };
        for (auto m : g_MonsterManager.monsters)    if (m->alive)  addTarget(m->worldX,  m->worldY);
        for (auto rm : g_MonsterManager.rangedMobs) if (rm->alive) addTarget(rm->worldX, rm->worldY);
        for (auto bm : g_MonsterManager.bombers)    if (bm->alive) addTarget(bm->worldX, bm->worldY);

        float hitPulse = (targetCount > 0) ? (0.55f + 0.45f * sinf(t * 24.0f)) : 0.0f;
        float bodyPulse = pulse * 0.18f + hitPulse * 0.58f;
        float triAng = t * 1.90f;
        float hexAng = 0.5235988f - t * 0.72f;

        DrawPlayerHexFrame(cx, cy, fieldRad,
                           0.5235988f + t * 0.045f, r, g, b,
                           0.060f + hitPulse * 0.050f, 1.0f + hitPulse * 0.6f, false);
        DrawPlayerHexFrame(cx, cy, fieldRad * 0.72f,
                           0.5235988f - t * 0.030f, r, g, b,
                           0.035f + hitPulse * 0.040f, 0.8f, false);

        DrawPlayerTriangleFrame(cx, cy, sz * (0.82f + bodyPulse * 0.08f),
                                triAng, r, g, b, 0.78f + hitPulse * 0.20f,
                                2.0f + hitPulse * 1.2f, true);
        DrawPlayerHexFrame(cx, cy, sz * (1.17f + bodyPulse * 0.08f),
                           hexAng, r, g, b, 0.48f + hitPulse * 0.32f,
                           1.55f + hitPulse * 1.0f, true);
        DrawPlayerHexFrame(cx, cy, sz * 1.45f,
                           hexAng * 0.70f + 0.30f, r, g, b,
                           0.14f + hitPulse * 0.12f, 1.0f, false);

        drawCircle(cx, cy, sz * (0.34f + bodyPulse * 0.05f), 0.0f, 0.0f, 0.0f, 0.58f);
        drawCircle(cx, cy, sz * (0.27f + bodyPulse * 0.04f), r, g, b, 0.48f + bodyPulse * 0.28f);
        drawDiamond(cx, cy, sz * (0.22f + bodyPulse * 0.02f), wr, wg, wb, 0.78f + hitPulse * 0.18f);

        for (int i = 0; i < 6; ++i) {
            float va = hexAng + (float)i * 1.04719755f;
            float vx = cx + cosf(va) * sz * 1.17f;
            float vy = cy + sinf(va) * sz * 1.17f;
            DrawPlayerLine(cx + cosf(va) * sz * 0.40f, cy + sinf(va) * sz * 0.40f,
                           vx, vy, 0.9f + hitPulse * 0.8f, r, g, b,
                           0.16f + hitPulse * 0.18f);
        }

        for (int i = 0; i < targetCount; ++i) {
            float srcAng = hexAng + (float)(i % 6) * 1.04719755f;
            float sx = cx + cosf(srcAng) * sz * 1.18f;
            float sy = cy + sinf(srcAng) * sz * 1.18f;
            float flicker = 0.70f + 0.30f * sinf(t * 38.0f + (float)i * 1.7f);
            DrawPlayerZigzagLine(sx, sy, targets[i].x, targets[i].y,
                                 5, 5.0f + hitPulse * 4.0f,
                                 1.25f + hitPulse * 0.9f,
                                 wr, wg, wb, (0.28f + hitPulse * 0.36f) * flicker);
            drawDiamond(targets[i].x, targets[i].y, 5.0f + hitPulse * 2.5f,
                        r, g, b, 0.34f + hitPulse * 0.28f);
        }
        break;
    }
    case PlayerShellKind::Rifle:
    default: {
        float bounce = sinf((1.0f - kick) * 3.1415926f) * 0.035f;
        float outerScale = std::max(0.66f, 1.0f - kick * 0.24f + bounce);
        float innerScale = std::max(0.70f, 1.0f - kick * 0.18f + bounce * 0.65f);
        float outerAng = t * 2.25f;
        float innerAng = -t * 1.06f + 0.78539816f;
        float scopeAng = -t * 0.30f;

        auto rx = [&](float lx, float ly) {
            return cx + lx * cosf(scopeAng) - ly * sinf(scopeAng);
        };
        auto ry = [&](float lx, float ly) {
            return cy + lx * sinf(scopeAng) + ly * cosf(scopeAng);
        };

        drawCircle(cx, cy, sz * 0.54f, 0.0f, 0.0f, 0.0f, 0.26f);
        {
            float orbitR = sz * 1.86f;
            const int DASHES = 32;
            for (int i = 0; i < DASHES; ++i) {
                if ((i & 1) != 0) continue;
                float a0 = scopeAng + (float)i / (float)DASHES * 6.2831853f;
                float a1 = scopeAng + ((float)i + 0.42f) / (float)DASHES * 6.2831853f;
                DrawPlayerLine(cx + cosf(a0) * orbitR, cy + sinf(a0) * orbitR,
                               cx + cosf(a1) * orbitR, cy + sinf(a1) * orbitR,
                               0.85f, r, g, b, 0.22f);
            }
            for (int i = 0; i < 2; ++i) {
                float na = scopeAng * 1.55f + (float)i * 3.1415926f;
                drawDiamond(cx + cosf(na) * orbitR, cy + sinf(na) * orbitR,
                            sz * 0.095f, wr, wg, wb, 0.52f + kick * 0.18f);
            }

            float corner = sz * 1.45f;
            float leg = sz * 0.34f;
            for (int ix = -1; ix <= 1; ix += 2) {
                for (int iy = -1; iy <= 1; iy += 2) {
                    float x0 = (float)ix * corner;
                    float y0 = (float)iy * corner;
                    DrawPlayerLine(rx(x0, y0), ry(x0, y0),
                                   rx(x0 - (float)ix * leg, y0), ry(x0 - (float)ix * leg, y0),
                                   1.35f + kick * 0.35f, r, g, b, 0.34f + kick * 0.14f);
                    DrawPlayerLine(rx(x0, y0), ry(x0, y0),
                                   rx(x0, y0 - (float)iy * leg), ry(x0, y0 - (float)iy * leg),
                                   1.35f + kick * 0.35f, r, g, b, 0.34f + kick * 0.14f);
                    drawDiamond(rx(x0, y0), ry(x0, y0),
                                sz * 0.060f, wr, wg, wb, 0.36f + kick * 0.22f);
                }
            }
        }
        DrawPlayerCrossStarFrame(cx, cy, sz * 1.24f * outerScale, sz * 0.33f * outerScale,
                                 outerAng, r, g, b, 0.42f + kick * 0.18f,
                                 1.35f + kick * 0.45f, true, true);
        DrawPlayerCrossStarFrame(cx, cy, sz * 0.76f * innerScale, sz * 0.22f * innerScale,
                                 innerAng, wr, wg, wb, 0.74f + kick * 0.22f,
                                 2.35f + kick * 1.10f, true, true);

        for (int i = 0; i < 4; ++i) {
            float oa = outerAng - 1.5707963f + (float)i * 1.5707963f;
            float ia = innerAng - 1.5707963f + (float)i * 1.5707963f;
            drawDiamond(cx + cosf(oa) * sz * 1.24f * outerScale,
                        cy + sinf(oa) * sz * 1.24f * outerScale,
                        sz * (0.080f + kick * 0.050f), wr, wg, wb,
                        0.24f + kick * 0.66f);
            drawDiamond(cx + cosf(ia) * sz * 0.76f * innerScale,
                        cy + sinf(ia) * sz * 0.76f * innerScale,
                        sz * (0.070f + kick * 0.045f), r, g, b,
                        0.38f + kick * 0.52f);
        }

        float core = sz * (0.30f + kick * 0.025f);
        drawRect(cx - core * 0.52f, cy - core * 0.52f,
                 core * 1.04f, core * 1.04f, 0.0f, 0.0f, 0.0f, 0.66f);
        drawDiamond(cx, cy, core * 0.92f, r, g, b, 0.72f + kick * 0.18f);
        drawDiamond(cx, cy, core * 0.46f, wr, wg, wb, 0.84f + kick * 0.12f);
        break;
    }
    }
}

// DYING ?�망 ?�출 ?�태
float g_DyingTimer    = 0.0f;
bool  g_DeathBoomDone = false;
float g_DeathWinW0    = 0.0f;
float g_GameOverFade  = 0.0f;    // 게임?�버 메뉴 ?�이?�인 (0??, ?�?�크�???2.5�?
// ?�망 ?�출 = 즉시 ?�??��(?�티?? ????�� ?�파 ??GAMEOVER (�??�네마틱 ?�음, ?�순)
static const float DYING_DUR      = 0.8f;   // ??�� ?�파가 ?�생?�는 ?�간 (?�이???�까지)
static const float GAMEOVER_FADE  = 2.5f;   // �޴� 100%���� �ɸ��� �ð�
static const float VICTORY_FADE   = 2.5f;
float g_VictoryFade = 0.0f;

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
    if (atype == AugType::BROKEN_SIGHT) {
        g_Orb.active = true;
        g_Orb.x = (float)(rand() % scrW);
        g_Orb.y = (float)(rand() % scrH);
        float a = (float)(rand() % 628) * 0.01f;
        float s = 100.0f + (float)(rand() % 150);
        g_Orb.vx = cosf(a) * s;
        g_Orb.vy = sinf(a) * s;
        g_Orb.wanderTimer = 0.5f;
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

void SyncPlayerBoundsAfterLoadout() {
    g_WindowSizeCur = g_Stats.windowSize;
    if (s_PlayerBoundsRef) EnsurePlayerBounds(*s_PlayerBoundsRef);
    if (g_Stats.chakram) InitChakramsFromStats();
}

static void RebuildPlayerStatsFromOwned(int scrW, int scrH, bool preserveRuntime = true) {
    int weapon = g_CurrentWeapon;
    bool melee = g_RunMelee;
    bool bow   = g_RunBow;
    std::vector<int> owned = g_OwnedAugs;
    const long long keepKillCount = g_Stats.killCount;
    const int keepVampireKillStreak = g_Stats.vampireKillStreak;
    const bool keepMk2Used = g_Stats.mk2Used;
    const float keepLightStepDisableTimer = g_Stats.lightStepDisableTimer;

    g_Stats = PlayerStats();
    ApplyMeta(g_Stats);
    g_Stats.windowSize *= g_Scale;
    if (weapon >= 0 && weapon < (int)StartWeapon::_COUNT)
        ApplyWeapon(g_Stats, (StartWeapon)weapon);
    if (melee) {
        g_Stats.meleeWeapon  = true;
        g_Stats.fireInterval = 0.26f;
    } else if (bow) {
        g_Stats.bowWeapon    = true;
        g_Stats.bulletSpeed *= 1.4f;
    }
    g_Stats.baseFireInterval = g_Stats.fireInterval;

    memset(g_TypeOwned, 0, sizeof(g_TypeOwned));
    if (weapon >= 0 && weapon < (int)StartWeapon::_COUNT)
        MarkStartWeaponOwnedType((StartWeapon)weapon);
    memset(g_GameManager.takenOnce, 0, sizeof(g_GameManager.takenOnce));

    g_OwnedAugs.clear();
    for (int d = 0; d < MAX_DRONES;   d++) g_Drones[d]   = DroneState{};
    for (int c = 0; c < MAX_CHAKRAMS; c++) g_Chakrams[c] = ChakramState{};
    g_Turrets.clear();
    g_LaserBeams.clear();
    g_Orb = BrokenSightOrb{};
    g_ApproachOrbs.clear();
    g_TurretDeployTimer = 0.0f;
    g_TurretStats = PlayerStats();
    g_DrunkCycle = 0.0f;
    g_DrunkActive = false;
    ResetSkills();

    bool needTurretDeploy = false;
    for (int idx : owned) {
        if (idx < 0 || idx >= AUG_TOTAL) continue;
        AugType atype = ALL_AUGS[idx].type;
        bool prevTurret = g_Stats.turretMode;
        g_Stats.Apply(atype);
        if (ALL_AUGS[idx].rarity == AugRarity::COMMON)
            g_Stats.ApplyCommonMultBoost();
        g_OwnedAugs.push_back(idx);
        g_TypeOwned[(int)atype] = true;
        EquipSkill(SkillForAug(atype));
        if (AugOnceOnly(atype, ALL_AUGS[idx].rarity))
            g_GameManager.takenOnce[idx] = true;
        ApplyAugmentSideEffects(atype, scrW, scrH);
        if (g_Stats.turretMode && !prevTurret) needTurretDeploy = true;
    }
    if (needTurretDeploy) g_TurretDeployTimer = TURRET_DEPLOY;
    if (g_Stats.chakram) InitChakramsFromStats();
    ReequipSkillsFromOwned(g_OwnedAugs.data(), (int)g_OwnedAugs.size());

    if (preserveRuntime) {
        g_Stats.killCount = keepKillCount;
        g_Stats.vampireKillStreak = keepVampireKillStreak;
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
    bool prevTurret = g_Stats.turretMode;
    g_Stats.Apply(atype);
    if (ALL_AUGS[idx].rarity == AugRarity::COMMON)
        g_Stats.ApplyCommonMultBoost();
    g_OwnedAugs.push_back(idx);
    g_TypeOwned[(int)atype] = true;
    MarkAugSeen(idx);
    EquipSkill(SkillForAug(atype));
    if (g_Stats.turretMode && !prevTurret) {
        g_Turrets.clear();
        g_TurretDeployTimer = TURRET_DEPLOY;
    }
    if (AugOnceOnly(atype, ALL_AUGS[idx].rarity))
        g_GameManager.takenOnce[idx] = true;
    if (g_GameManager.playerHP > g_Stats.maxHP)
        g_GameManager.playerHP = g_Stats.maxHP;
    ApplyAugmentSideEffects(atype, scrW, scrH);
}

static void BeginAugReplaceFlow(int newIdx, bool fromShop, int shopSlot) {
    g_GameManager.pendingAugIdx      = newIdx;
    g_GameManager.replaceFromShop    = fromShop;
    g_GameManager.replaceShopSlot    = shopSlot;
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
    g_Orb = BrokenSightOrb{};
    g_ApproachOrbs.clear();
    for (int d = 0; d < MAX_DRONES; d++) g_Drones[d] = DroneState{};
    for (int c = 0; c < MAX_CHAKRAMS; c++) g_Chakrams[c] = ChakramState{};
    g_Turrets.clear();
    g_TurretDeployTimer = 0.0f;
    g_TurretStats = PlayerStats();
    g_LaserBeams.clear();
    g_DrunkCycle = 0.0f;
    g_DrunkActive = false;
    ResetSkills();
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
        g_PostPickGrace = 0.5f;
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
    g_PostPickGrace = 0.5f;
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
            if (g_RunMelee) {
                g_Stats.meleeWeapon = true;
                g_Stats.fireInterval = 0.26f;
            } else if (g_RunBow) {
                g_Stats.bowWeapon = true;
                g_Stats.bulletSpeed *= 1.4f;
            }
            g_Stats.baseFireInterval = g_Stats.fireInterval;
            g_Stats.killCount = keepKillCount;
            g_Stats.vampireKillStreak = keepVampireKillStreak;
            g_Stats.mk2Used = keepMk2Used;
            g_Stats.lightStepDisableTimer = keepLightStepDisableTimer;

            g_OwnedAugs.clear();
            memset(g_GameManager.takenOnce, 0, sizeof(g_GameManager.takenOnce));
            memset(g_TypeOwned, 0, sizeof(g_TypeOwned));
            if (g_CurrentWeapon >= 0 && g_CurrentWeapon < (int)StartWeapon::_COUNT)
                MarkStartWeaponOwnedType((StartWeapon)g_CurrentWeapon);
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
                buffs, nBuffs, false, false, true, false, false);
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
                buffs, 3, g_Stats.sizeAugTaken, g_Stats.distAugTaken,
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
                picks, 3, g_Stats.sizeAugTaken, g_Stats.distAugTaken,
                true, false, false);
            PushAugEffectsFront(picks, got);
            continue;
        }

        if (NeedsReplaceForAug(idx)) {
            BeginAugReplaceFlow(idx, false, -1);
            return;
        }
        ApplySingleAugIdx(idx, scrW, scrH);
    }
}

static void CompleteAugReplaceFlow(int replaceSlot, int scrW, int scrH) {
    if (replaceSlot < 0 || replaceSlot >= g_GameManager.replaceChoiceCount) return;
    int oldIdx = g_GameManager.replaceChoices[replaceSlot];
    int newIdx = g_GameManager.pendingAugIdx;
    bool fromShop = g_GameManager.replaceFromShop;
    int shopSlot  = g_GameManager.replaceShopSlot;

    for (auto it = g_OwnedAugs.begin(); it != g_OwnedAugs.end(); ++it) {
        if (*it == oldIdx) { g_OwnedAugs.erase(it); break; }
    }
    g_GameManager.takenOnce[oldIdx] = false;
    g_TypeOwned[(int)ALL_AUGS[oldIdx].type] = false;

    RebuildPlayerStatsFromOwned(scrW, scrH);
    ApplySingleAugIdx(newIdx, scrW, scrH);
    g_GameManager.maxHP = g_Stats.maxHP;
    g_GameManager.pendingAugIdx = -1;
    g_GameManager.replaceChoiceCount = 0;

    if (fromShop && shopSlot >= 0 && shopSlot < 4) {
        g_RunGold -= g_RunShopPrice[shopSlot];
        g_RunShopStock[shopSlot] = -1;
        g_RunShopPrice[shopSlot]  = 0;
        g_GameManager.currentState = GameState::RUN_SHOP;
        return;
    }
    if (g_AugEffectProcessing) {
        ProcessAugEffectQueue(scrW, scrH);
        if (g_GameManager.currentState == GameState::AUG_REPLACE) return;
        g_AugEffectProcessing = false;
    }
    FinishAugmentEffects();
}

static void CancelAugReplaceFlow() {
    bool fromShop = g_GameManager.replaceFromShop;
    g_GameManager.pendingAugIdx = -1;
    g_GameManager.replaceChoiceCount = 0;
    if (fromShop) {
        g_GameManager.currentState = GameState::RUN_SHOP;
        return;
    }
    if (g_AugEffectProcessing) {
        ProcessAugEffectQueue(g_GameManager.screenW, g_GameManager.screenH);
        if (g_GameManager.currentState == GameState::AUG_REPLACE) return;
        g_AugEffectProcessing = false;
    }
    FinishAugmentEffects();
}

static void TriggerVictory() {
    g_InBossIntermission = false;
    g_IntermissionTimer  = 0.0f;
    if (g_GameManager.currentState == GameState::RUN_SHOP)
        g_GameManager.currentState = GameState::RUNNING;
    g_AchSaveNeeded = true;
    g_LastRunRecord = RecordRunResult((int)g_Difficulty,
                                      g_GameManager.score,
                                      g_Stats.killCount,
                                      (g_SelectedJob == JOB_NONE ? 0 : 1));
    g_GameManager.currentState = GameState::VICTORY;
    g_VictoryFade = 0.0f;
}

static void OnBossKilled(int bossPick, long long goldBonus) {
    FinishBossKill(g_MonsterManager, bossPick, goldBonus,
                   g_GameManager.screenW, g_GameManager.screenH);
}

static void ApplyPurchasedAug(int idx, SpatialBounds& playerWin, int scrW, int scrH) {
    (void)playerWin;
    AugType atype = ALL_AUGS[idx].type;

    bool prevTurret = g_Stats.turretMode;
    g_Stats.Apply(atype);
    if (ALL_AUGS[idx].rarity == AugRarity::COMMON)
        g_Stats.ApplyCommonMultBoost();
    g_OwnedAugs.push_back(idx);
    g_TypeOwned[(int)atype] = true;
    MarkAugSeen(idx);
    EquipSkill(SkillForAug(atype));
    if (g_Stats.turretMode && !prevTurret) {
        g_Turrets.clear();
        g_TurretDeployTimer = TURRET_DEPLOY;
    }
    if (AugOnceOnly(atype, ALL_AUGS[idx].rarity))
        g_GameManager.takenOnce[idx] = true;
    if (g_GameManager.playerHP > g_Stats.maxHP)
        g_GameManager.playerHP = g_Stats.maxHP;
    g_GameManager.maxHP = g_Stats.maxHP;
    ApplyAugmentSideEffects(atype, scrW, scrH);
}

void RunShopPurchase(int slot) {
    if (!s_PlayerBoundsRef || slot < 0 || slot >= 4) return;
    int idx = g_RunShopStock[slot];
    if (idx < 0) return;
    int price = g_RunShopPrice[slot];
    if (g_RunGold < (long long)price) return;
    if (NeedsReplaceForAug(idx)) {
        g_GameManager.replaceShopSlot = slot;
        BeginAugReplaceFlow(idx, true, slot);
        return;
    }
    g_RunGold -= price;
    ApplyPurchasedAug(idx, *s_PlayerBoundsRef, g_GameManager.screenW, g_GameManager.screenH);
    g_RunShopStock[slot] = -1;
    g_RunShopPrice[slot]  = 0;
}

static bool BossFightBusy() {
    return (g_RRBoss && g_RRBoss->alive)
        || (g_CentiBoss && g_CentiBoss->alive)
        || (g_TessBoss && g_TessBoss->alive)
        || (g_EtherBoss && g_EtherBoss->alive)
        || g_BossWarnTimer > 0.0f;
}

static void StartBossWarn(int pick, const wchar_t* name, float hp) {
    BossDir::SetActTheme(pick);
    g_BossWarnPick = pick;
    g_BossWarnName = name;
    g_BossWarnHp   = hp;
    float warnDur = g_CreativeMode ? 0.35f : BOSS_WARN_DUR * TrialBossWarningMult();
    if (warnDur < 0.25f) warnDur = 0.25f;
    g_BossWarnTimer = warnDur;
}

static void QueueCreativeBossPick(int pick, float bossHpC, float polyHpC) {
    (void)polyHpC;
    switch (pick) {
    case 2: StartBossWarn(2, L"VOLLEY", bossHpC);        break;
    case 8: StartBossWarn(8, L"FORK",  bossHpC * 0.7f); break;
    case 10: StartBossWarn(10, TesseractGlitchBoss::BOSS_NAME, bossHpC * 0.92f); break;
    case 20: StartBossWarn(20, EtherSwordBoss::BOSS_NAME, bossHpC * 3.2f);      break;
    case 3: StartBossWarn(3, L"SPAM",   bossHpC * 0.9f); break;
    default: StartBossWarn(2, L"VOLLEY", bossHpC);       break;
    }
}

int main() {
    CrashHandler::Install();   // 강종(E23) 추적 ??처리 ?????�외 ??로그+미니?�프
    srand((unsigned)time(NULL));

    // ?�행 ?�일 ?�더�??�업 ?�렉?�리 ?�동 (Resource/ ?��?경로 로드 보장)
    PlatformChdirToExeDir();
    LoadGame();   // �����?����/���?�ҷ����� (������ �⺻�� ����)
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
    screenHeight = mode->height - 1; // DirectFlip ȸ��: ȭ�麸�� 1px �۰�
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

    // ??screenHeight 가 ?��? mode->height-1 (?�에??DirectFlip ?�피??
    //   ?�면 ?�확??같�? ?�기�??�성?�면 DWM ??DirectFlip ?�로 컴포지???�회
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
    {
        int fullH = GetSystemMetrics(SM_CYSCREEN);
        RECT wa;
        if (SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0)) {
            int bottomGap = fullH - (int)wa.bottom;   // ?�단 ?�업?�시�??�이 (�????�치�?0)
            if (bottomGap > 0 && bottomGap < 120) g_TaskbarH = bottomGap;
        }
    }
#endif

    // ?�상??기�? ?��??????��? ?�면?�서 �??�티?��? 비�? 축소?�도�?계산 ???�괄 ?�용.
    //   (?��? ?��??�라 ?��? ?�면?�수�??��??�으�?컸던 문제 ?�결 + ?�체?�으�?�?축소)
    g_Scale = (float)screenHeight / SCALE_REF_H;
    if (g_Scale > 1.0f) g_Scale = 1.0f;
    if (g_Scale < 0.5f) g_Scale = 0.5f;
    TURRET_WIN_W *= g_Scale; TURRET_WIN_H *= g_Scale;
    RR_WIN_W *= g_Scale; POLY_WIN_W *= g_Scale; BOTNET_WIN_W *= g_Scale;
    CENTI_WIN_W *= g_Scale;
    TESS_WIN_W  *= g_Scale;
    ETHER_WIN_W *= g_Scale;
    TOTEM_WIN_W *= g_Scale;
    UNKNOWN_WIN_W *= g_Scale; UNKNOWN_WIN_H *= g_Scale;
    SPAWNER_WIN_W *= g_Scale;
    DDOS_WIN_W    *= g_Scale;
    g_RfwW *= g_Scale; g_RfwH *= g_Scale;
    glfwMakeContextCurrent(window);
    InputRegisterCallbacks(window);
    glfwSwapInterval((g_FpsCap == 0) ? 1 : 0);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    // ��Ʈ: exe ��ġ�� ���� Resource ���?��ΰ�?�޶��� �� �־� ���� �����ϴ� ��θ�?������.
    {
        auto fileExists = [](const char* path) -> bool {
#ifdef _MSC_VER
            FILE* fp = nullptr;
            if (fopen_s(&fp, path, "rb") != 0 || !fp) return false;
#else
            FILE* fp = std::fopen(path, "rb");
            if (!fp) return false;
#endif
            std::fclose(fp);
            return true;
        };
        auto pickFont = [&](const char* a, const char* b, const char* c,
                            const char* d = nullptr) -> const char* {
            if (a && fileExists(a)) return a;
            if (b && fileExists(b)) return b;
            if (c && fileExists(c)) return c;
            if (d && fileExists(d)) return d;
            return nullptr;
        };

        const char* chain[6] = {};
        int nFonts = 0;
        auto addFont = [&](const char* path) {
            if (path && nFonts < (int)(sizeof(chain) / sizeof(chain[0])))
                chain[nFonts++] = path;
        };

        addFont(pickFont("Resource/Font/ChakraPetch-Regular.ttf",
                         "../Resource/Font/ChakraPetch-Regular.ttf",
                         "../../Resource/Font/ChakraPetch-Regular.ttf"));
        addFont(pickFont("Resource/Font/Orbit-Regular.ttf",
                         "../Resource/Font/Orbit-Regular.ttf",
                         "../../Resource/Font/Orbit-Regular.ttf"));
        addFont(pickFont("Resource/Font/Jua-Regular.ttf",
                         "../Resource/Font/Jua-Regular.ttf",
                         "../../Resource/Font/Jua-Regular.ttf",
                         "C:/Windows/Fonts/malgun.ttf"));
        addFont(pickFont("Resource/Font/KosugiMaru-Regular.ttf",
                         "../Resource/Font/KosugiMaru-Regular.ttf",
                         "../../Resource/Font/KosugiMaru-Regular.ttf",
                         "C:/Windows/Fonts/meiryo.ttc"));
        if (nFonts == 0) {
            addFont(pickFont("C:/Windows/Fonts/malgun.ttf",
                             "C:/Windows/Fonts/arial.ttf",
                             "C:/Windows/Fonts/meiryo.ttc"));
        }

        g_TextL.InitFromFiles(chain, nFonts, 36,  screenWidth, screenHeight);
        g_TextS.InitFromFiles(chain, nFonts, 22,  screenWidth, screenHeight);
        g_TextXL.InitFromFiles(chain, nFonts, 100, screenWidth, screenHeight);
        g_TextS.SetMinScale(0.58f);

        // Warm only characters used by the game. The renderer caches glyphs
        // lazily, so this removes first-use stalls without baking all Hangul.
        for (int si = 0; si < (int)StrId::_COUNT; ++si) {
            for (int li = 0; li < LANG_COUNT; ++li) {
                g_TextL.PreloadText(kStrings[si][li]);
                g_TextS.PreloadText(kStrings[si][li]);
            }
        }
        for (int ai = 0; ai < AUG_TOTAL; ++ai) {
            for (int li = 0; li < LANG_COUNT; ++li) {
                g_TextL.PreloadText(ALL_AUGS[ai].locName[li]);
                g_TextL.PreloadText(ALL_AUGS[ai].locDesc[li]);
                g_TextS.PreloadText(ALL_AUGS[ai].locName[li]);
                g_TextS.PreloadText(ALL_AUGS[ai].locDesc[li]);
            }
        }
    }

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

    float lastFrame        = 0.0f;
    float spawnTimer       = 0.0f;
    float rangedSpawnTimer = 0.0f;
    float fireTimer        = g_Stats.fireInterval;  // ready to fire immediately
    float accumulator      = 0.0f;
    const float FIXED_DT = 1.0f / 60.0f;

    Audio::Init();   // ?�운???�스??(Sounds/ ?�더, ?�일 ?�으�?무음)

    // 최근???????�동조�?·?�도???�환?��?)·?�론·?�탑 공용
    auto findNearestEnemy = [&](float fx, float fy, float& tx, float& ty) -> bool {
        float nd = 1e18f; bool found = false;
        auto consider = [&](float ex, float ey) {
            float ddx = ex - fx, ddy = ey - fy;
            float ds = ddx * ddx + ddy * ddy;
            if (ds < nd) { nd = ds; tx = ex; ty = ey; found = true; }
        };
        auto vis = [&](float ex, float ey) {
            return ex >= playerWin.x && ex <= playerWin.x + playerWin.width &&
                   ey >= playerWin.y && ey <= playerWin.y + playerWin.height;
        };
        for (auto m : g_MonsterManager.monsters)
            if (m->alive && vis(m->worldX, m->worldY)) consider(m->worldX, m->worldY);
        for (auto r : g_MonsterManager.rangedMobs)
            if (r->alive) consider(r->worldX, r->worldY);
        for (auto bm : g_MonsterManager.bombers)
            if (bm->alive && vis(bm->worldX, bm->worldY)) consider(bm->worldX, bm->worldY);
        if (g_RRBoss && g_RRBoss->alive)         consider(g_RRBoss->worldX, g_RRBoss->worldY);
        if (g_CentiBoss && g_CentiBoss->alive) {
            if (g_CentiBoss->vulnerable())
                consider(g_CentiBoss->worldX, g_CentiBoss->worldY);
            for (auto& mb : g_CentiBoss->minis) if (mb.alive) consider(mb.x, mb.y);
        }
        if (g_TessBoss && g_TessBoss->alive) {
            consider(g_TessBoss->worldX, g_TessBoss->worldY);
            for (auto& o : g_TessBoss->orbs) if (o.alive) consider(o.x, o.y);
        }
        return found;
    };

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
            const char* bn = g_RRBoss ? "reload" : g_CentiBoss ? "centi" : "none";
            char bc[200];
            std::snprintf(bc, sizeof(bc),
                "st=%d score=%lld lv=%d mobs=%u boss=%s",
                (int)g_GameManager.currentState,
                (long long)g_GameManager.score,
                g_GameManager.playerLevel,
                (unsigned)g_MonsterManager.monsters.size(), bn);
            CrashHandler::SetBreadcrumb(bc);
        }

        // �??�커?��? ?�으�??�른 ?�으�??�환) ?�동 ?�시?��? ??        //   ?�버?�이???�커???�어??루프가 계속 ?��?�? ??막으�?게임??        //   백그?�운?�에??계속 진행??콤보 3�?창이 ?�러가 리셋?�는 버그 ??.
        {
            static bool s_wasFocused = true;
            bool focused = glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0;
            if (!focused && s_wasFocused &&
                g_GameManager.currentState == GameState::RUNNING) {
                g_GameManager.pauseResumeState = GameState::RUNNING;
                g_GameManager.currentState = GameState::PAUSED;
            }
            s_wasFocused = focused;
        }

        // FPS 측정 (1�??�위)
        ++g_FpsFrames;
        if (now - g_FpsLastTime >= 1.0f) {
            g_CurrentFPS  = g_FpsFrames;
            g_FpsFrames   = 0;
            g_FpsLastTime = now;
        }

        glfwPollEvents();

        // �?부?�럽�?보간 (?�이�? ?�면 ?�장 ??
        //   ?�투/?�투?�버?�이(RUNNING/DYING/PAUSED/증강·?�버???�택)?�선 �??��? ??        //   ?�시?��? ??줌인?�는 �?부?�연?�럽?�는 ?�드�? �??�제???�작�???        //   '진짜 메뉴'�??�갈 ?�만(보스 처치 ?�엔 polyDeath 가 target=1 �?부?�럽�?복원).
        {
            GameState zs = g_GameManager.currentState;
            bool inFight = (zs == GameState::RUNNING || zs == GameState::DYING ||
                            zs == GameState::PAUSED  || zs == GameState::AUG_SELECT ||
                            zs == GameState::DEBUFF_SELECT ||
                            zs == GameState::RUN_SHOP ||
                            (zs == GameState::RUNNING && g_InBossIntermission));
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
        bool lmb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        float wmx = ScreenToWorldX((float)mx);
        float wmy = ScreenToWorldY((float)my);

        // --- ?�력 처리 ---
        GameState prevState = g_GameManager.currentState;
        g_GameManager.HandleInput(window);

        // Keep the OS pointer out of the combat view while the custom
        // crosshair is active. Pause/settings screens use normal input, so
        // the pointer must remain available for their clickable controls.
        auto UpdateCursorVisibility = [&]() {
            const GameState cursorState = g_GameManager.currentState;
            const bool hideCursor = g_ShowCrosshair &&
                (cursorState == GameState::RUNNING ||
                 cursorState == GameState::DYING);
            static bool cursorModeInitialized = false;
            static bool cursorHidden = false;
            if (!cursorModeInitialized || cursorHidden != hideCursor) {
                glfwSetInputMode(window, GLFW_CURSOR,
                                 hideCursor ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);
                cursorHidden = hideCursor;
                cursorModeInitialized = true;
            }
        };
        UpdateCursorVisibility();

        // ??게임 리셋 ?�다 (GAMEOVER ??READY, ?�이???�택 ?? ?�시?�기 버튼 ?�에???�출)
        auto ResetForNewGame = [&]() {
            g_Stats        = PlayerStats();
            g_MetaStartAugs = ApplyMeta(g_Stats);    // 메�? ?�구 ?�그?�이???�용
            g_Stats.windowSize *= g_Scale;           // ?�레?�어 �????�상??비�? 축소
            g_Orb          = BrokenSightOrb{};
            g_ApproachOrbs.clear();
            for (int d = 0; d < MAX_DRONES;   d++) g_Drones[d]   = DroneState{};
            for (int c = 0; c < MAX_CHAKRAMS; c++) g_Chakrams[c] = ChakramState{};
            g_Turrets.clear();
            g_TurretDeployTimer = 0.0f;
            g_TurretStats = PlayerStats();
            g_BulletRainTimer = 0.0f;
            g_DrunkCycle      = 0.0f;
            g_DrunkActive     = false;
            g_PrevHP          = g_Stats.maxHP;
            g_XpTimeAccum     = 0.0f;
            g_OwnedAugs.clear();
            memset(g_TypeOwned, 0, sizeof(g_TypeOwned));
            g_CreativeGodmode  = false;
            g_CreativeFreeGrab = false;
            g_HoveredAug       = -1;
            g_EnterReleased    = true;
            g_ConversionWeapon = -1;
            g_CurrentWeapon    = -1;
            g_PauseSelectedAug = -1;
            g_GameTime         = 0.0f;
            g_GameManager.ClearAugmentRewardQueue();
            g_AugEffectQueue.clear();
            g_AugEffectProcessing = false;
            g_BomberSpawnTimer = 0.0f;
            for (int i = 0; i < MAX_SHOCKS; i++) g_ShockWaves[i].active = false;
            for (int i = 0; i < MAX_SLASH; i++) g_Slashes[i].active = false;
            g_MuzzleTimer = 0.0f;
            g_ArcherCharge = 0.0f;
            g_ShakeTime = 0.0f; g_ShakeMag = 0.0f;
            g_NextBossScore = FIRST_BOSS_SCORE;
            BossDir::ResetRotation();
            BossDir::ResetAct();
            ResetTrials();
            g_RunGold           = 0;
            g_RunStardust       = 0;
            g_InBossIntermission = false;
            g_IntermissionTimer = 0.0f;
            g_ShopZoneHold      = 0.0f;
            g_SkipZoneHold      = 0.0f;
            g_ShopMustLeave     = false;
            g_ShopZoneX         = 0.0f;
            g_ShopZoneY         = 0.0f;
            g_SkipZoneX         = 0.0f;
            g_SkipZoneY         = 0.0f;
            g_BossRewardPicksLeft = 0;
            g_BossWarnTimer = 0.0f; g_BossWarnPick = -1;
            g_RRWasP2 = g_RRWasP3 = false;
            g_TessWasP2 = g_TessWasP3 = false;
            g_LaserBeams.clear(); g_LaserTimer = 0.0f;
            g_StardustPickups.clear(); g_StardustHudPulse = 0.0f;
            g_XpBarPop = 0.0f;
            g_SlowZones.clear(); g_BadSectorBleed = 0.0f;
            g_NovaTimer = 0.0f;
            g_RunMelee = false; g_RunBow = false;
            if (g_RRBoss)    { delete g_RRBoss;    g_RRBoss    = nullptr; }
            if (g_CentiBoss) { delete g_CentiBoss; g_CentiBoss = nullptr; }
            if (g_TessBoss)  { delete g_TessBoss;  g_TessBoss  = nullptr; }
            if (g_EtherBoss) { delete g_EtherBoss; g_EtherBoss = nullptr; }
            g_BossTintT = 0.0f;
            ResetJuice();
            ResetSkills();
            g_WindowSizeCur = g_Stats.windowSize;
            g_WinPrevHP = -1.0f; g_HpGhost = -1.0f;
            g_PlayerDamagePulse = 0.0f;
            g_HurtVignette = 0.0f; g_HpBarPop = 0.0f;
            g_ViewZoom = g_ViewZoomTarget = 1.0f;   // �??�복
            g_ZoomCX = g_ZoomCY = 0.0f;
            g_Difficulty = Difficulty::NORMAL;
            // Keep the first ranged-mob interval active from run start so its
            // silhouette can be inspected without a score-based warm-up.
            rangedSpawnTimer = 0.0f;
            spawnTimer        = 0.0f;
            g_DyingTimer      = 0.0f;
            g_DeathBoomDone   = false;
            g_GameOverFade    = 0.0f;
            g_VictoryFade     = 0.0f;
            g_DeathFlash      = 0.0f;
            for (int i = 0; i < MAX_DEBRIS; i++) g_Debris[i].active = false;
            fireTimer = g_Stats.fireInterval;
            playerWin.width  = g_Stats.windowSize;
            playerWin.height = g_Stats.windowSize;
            playerWin.x = (screenWidth  - g_Stats.windowSize) * 0.5f;
            playerWin.y = (screenHeight - g_Stats.windowSize) * 0.5f;
            // GameManager ?�드??직접 리셋
            g_GameManager.playerHP    = 100.0f;
            g_GameManager.maxHP       = 100.0f;
            g_GameManager.score       = 0;
            g_GameManager.scoreAccum  = 0.0f;
            if (g_CreativeMode) {
                g_GameManager.score      = g_CreativeStartScore;
                g_GameManager.scoreAccum = (float)g_CreativeStartScore;
                // ?�작 ?�수보다 ??�?20�?배수부???�반 보스 (?�꺼번에 ?�아�?방�?)
                g_NextBossScore = ((g_CreativeStartScore / 200000) + 1) * 200000;
                // ?��? 50�??�상?�서 ?�작?�면 ?�리모프 ?�동?�장 ?�략 (보스?�택?�로 ?�환)
                // ?�택 보스 즉시 ?�폰 ?�약 (?�수 무�?)
                g_CreativeBossPending = (g_CreativeBossPick >= 0);
            }
            g_GameManager.xp          = 0;
            g_GameManager.playerLevel = 1;
            memset(g_GameManager.takenOnce, 0, sizeof(g_GameManager.takenOnce));
            g_Bullets.clear();
            g_MonsterManager.Clear();
            // UpdateStateSystem ??중복 reset ?�피
            g_GameManager.lastState = GameState::READY;
        };

        // Restart the current run without returning to the PLAY screen.
        // Keep the selected loadout and run modifiers, then rebuild the
        // combat state so the next frame starts from a clean arena.
        auto RestartCurrentRun = [&]() {
            const int savedWeapon = g_CurrentWeapon;
            const bool savedMelee = g_RunMelee;
            const bool savedBow = g_RunBow;
            const Difficulty savedDifficulty = g_Difficulty;
            const bool savedGodmode = g_CreativeGodmode;
            const std::vector<int> savedOwnedAugs = g_OwnedAugs;
            const bool savedTrialPoolReady = g_TrialPoolReady;
            int savedTrialPool[TRIAL_SLOT_COUNT] = {};
            bool savedTrialSelected[TRIAL_SLOT_COUNT] = {};
            for (int i = 0; i < TRIAL_SLOT_COUNT; ++i) {
                savedTrialPool[i] = g_TrialPool[i];
                savedTrialSelected[i] = g_TrialSelected[i];
            }

            // This clears enemies, projectiles, drops, boss instances,
            // timers, run currencies, score/XP and the death presentation.
            ResetForNewGame();

            // Restore the setup that ResetForNewGame intentionally clears.
            g_CurrentWeapon = savedWeapon;
            g_RunMelee = savedMelee;
            g_RunBow = savedBow;
            g_Difficulty = savedDifficulty;
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

            for (int i = 0; i < 4; ++i) {
                g_RunShopStock[i] = -1;
                g_RunShopPrice[i] = 0;
            }
            g_CreativeStartPending = false;
            g_GameManager.conversionAug = -1;
            g_GameManager.hoveredCard = -1;
            g_GameManager.pendingAugIdx = -1;
            g_GameManager.replaceChoiceCount = 0;
            g_GameManager.replaceFromShop = false;
            g_GameManager.replaceShopSlot = -1;
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
            if (g_GameManager.currentState == GameState::GAMEOVER ||
                g_GameManager.currentState == GameState::VICTORY)
                return;

            g_MonsterManager.Clear();
            g_Bullets.clear();
            if (g_RRBoss)    { delete g_RRBoss;    g_RRBoss    = nullptr; }
            if (g_CentiBoss) { delete g_CentiBoss; g_CentiBoss = nullptr; }
            if (g_TessBoss)  { delete g_TessBoss;  g_TessBoss  = nullptr; }
            if (g_EtherBoss) { delete g_EtherBoss; g_EtherBoss = nullptr; }
            g_Turrets.clear();
            g_LaserBeams.clear();
            g_SlowZones.clear();
            g_ApproachOrbs.clear();
            g_StardustPickups.clear();
            g_Orb.active = false;
            g_MuzzleTimer = 0.0f;
            g_ShakeTime = 0.0f;
            g_ShakeMag = 0.0f;
            for (int d = 0; d < MAX_DRONES;   ++d) g_Drones[d]   = DroneState{};
            for (int c = 0; c < MAX_CHAKRAMS; ++c) g_Chakrams[c] = ChakramState{};
            for (int i = 0; i < MAX_SHOCKS; ++i) g_ShockWaves[i].active = false;
            for (int i = 0; i < MAX_SLASH;  ++i) g_Slashes[i].active = false;
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
                L"런 포기", L"RUN ABANDONED", L"ランを放棄"
            };
            wcscpy_s(g_DeathReason, abandoned[li]);

            if (!g_CreativeMode) {
                g_LastRunRecord = RecordRunResult(
                    (int)g_Difficulty,
                    g_GameManager.score,
                    g_Stats.killCount,
                    (g_SelectedJob == JOB_NONE ? 0 : 1));
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
        g_GameManager.UpdateStateSystem(g_MonsterManager, g_Bullets);
        if (prevState == GameState::RUNNING &&
            g_GameManager.currentState == GameState::AUG_SELECT &&
            !g_GameManager.augmentRewardActive) {
            if (!g_GameManager.ActivateNextAugmentReward())
                g_GameManager.currentState = GameState::RUNNING;
        }

        // ?�?�?BGM ??게임?�레??중엔 메인 루프, 메뉴?�선 ?��? (보스 BGM ?�??�일 ?�기�??�장) ?�?�?
        {
            Audio::SetEnabled(g_SoundVol > 0);            // 0 = ?�기
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
                               * g_Stats.GetDamageMultiplier(0.0f) * 6.0f;
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
                for (auto bm : g_MonsterManager.bombers) {
                    if (!bm->alive) continue;
                    float dx = bm->worldX - pCX, dy = bm->worldY - pCY;
                    if (dx*dx + dy*dy < blastRad * blastRad) {
                        bm->hp -= blastDmg;
                        if (bm->hp <= 0) bm->alive = false;
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
                    for (auto r  : g_MonsterManager.rangedMobs) if (r->alive)  consider(r->worldX,  r->worldY,  MobName(CM_RANGED));
                    for (auto bm : g_MonsterManager.bombers)    if (bm->alive) consider(bm->worldX, bm->worldY, MobName(CM_BOMBER));
                    if (g_RRBoss     && g_RRBoss->alive)     consider(g_RRBoss->worldX,     g_RRBoss->worldY,     L"VOLLEY");
                    if (g_CentiBoss && g_CentiBoss->alive) consider(g_CentiBoss->worldX, g_CentiBoss->worldY, L"FORK");
                    if (g_TessBoss && g_TessBoss->alive) consider(g_TessBoss->worldX, g_TessBoss->worldY, TesseractGlitchBoss::BOSS_NAME);
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
                for (auto bm : g_MonsterManager.bombers) if (bm->alive) {
                    SpawnEnemyExplosion(bm->worldX, bm->worldY, bm->color.r, bm->color.g, bm->color.b, true);
                    bm->alive = false; bm->scored = true;
                }
                // 모든 ?�·보?�·분?�체·총알·?�탑??즉시 ?�전 ??�� ??UpdateAll(RUNNING ?�용)??                //   맡기�?DYING/GAMEOVER ?�안 ?�거�?�?�??�이 ?��? ?�태�??�으므�??�기???�거.
                g_MonsterManager.Clear();   // monsters/ranged/bombers/boss ?��? delete + clear
                g_Bullets.clear();
                if (g_RRBoss)    { delete g_RRBoss;    g_RRBoss    = nullptr; }
                if (g_CentiBoss) { delete g_CentiBoss; g_CentiBoss = nullptr; }
                if (g_TessBoss)  { delete g_TessBoss;  g_TessBoss  = nullptr; }
                if (g_EtherBoss) { delete g_EtherBoss; g_EtherBoss = nullptr; }
                g_Turrets.clear();
                g_BossWarnTimer  = 0.0f; g_BossWarnPick = -1;   // ?�망 ???��?�??�조 취소
                g_RRWasP2 = g_RRWasP3 = false;
                g_TessWasP2 = g_TessWasP3 = false;
                g_LaserBeams.clear();   // ?�캔 ?�이?�?�??�리
                g_SlowZones.clear(); g_BadSectorBleed = 0.0f;   // 배드 ?�터 감속 구역/출혈 ?�리
                g_NovaTimer = 0.0f;   // 백신 ?�캔 ?�리
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
                                                      (g_SelectedJob == JOB_NONE ? 0 : 1));
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
        if (g_GameManager.currentState == GameState::VICTORY && g_VictoryFade < 1.0f) {
            g_VictoryFade += delta / VICTORY_FADE;
            if (g_VictoryFade > 1.0f) g_VictoryFade = 1.0f;
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
                for (auto& sl : g_Slashes)    { if (!sl.active) continue; sl.life -= delta; if (sl.life <= 0.0f) sl.active = false; }
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
        // s_augSpaceReleased: 블록 바깥?�서??release 감�??�도�?static ?�언
        static bool s_augSpaceReleased = true;
        {
            int kSpChk = glfwGetKey(window, GLFW_KEY_SPACE);
            if (kSpChk == GLFW_RELEASE) s_augSpaceReleased = true;
        }
        if (g_GameManager.currentState == GameState::AUG_SELECT ||
            g_GameManager.currentState == GameState::DEBUFF_SELECT) {
            int k1 = glfwGetKey(window, GLFW_KEY_1);
            int k2 = glfwGetKey(window, GLFW_KEY_2);
            int k3 = glfwGetKey(window, GLFW_KEY_3);

            // ?�일 증강 ?�용 ?�퍼 (?��?????RANDOM_AUG/PANDORA 가 ?�출)
            // Special effects are processed by ProcessAugEffectQueue below.
            // Keep the legacy recursive implementation disabled for reference
            // until the queue path has shipped through a full playtest.
#if 0
            std::function<void(int)> applyByIdx;
            applyByIdx = [&](int idx) {
                AugType atype = ALL_AUGS[idx].type;

                // ?�수: ?�스?�치�??�행?�고 Apply ?�출 X
                if (atype == AugType::S_CHAOS) {
                    // ?�?��?: 보유 증강 ?�고, 같�? 개수 ?�덤. ??60% 버프 / 40% ?�버??                    // ?�제 보유 목록(중첩 ?�함) 기�??�로 카운????공격??증�?×4 같�? 중첩??모두 ?�함
                    int prevAugs = (int)g_OwnedAugs.size();
                    float saveSpawnMult = g_Stats.mobSpawnMult;
                    int   saveCapBonus  = g_Stats.mobCapBonus;
                    int   savePackBonus = g_Stats.mobPackBonus;
                    g_Stats = PlayerStats();
                    g_Stats.windowSize *= g_Scale;          // �????�상??비�? ?��?
                    // 무기/직업 ?�체?��? ?��? ??CHAOS ??증강�??�추첨한??
                    //   (검�?궁수가 기본 총으�?바뀌던 버그 fix. 증강?�????�래???�에 ??��)
                    if (g_CurrentWeapon >= 0 && g_CurrentWeapon < (int)StartWeapon::_COUNT)
                        ApplyWeapon(g_Stats, (StartWeapon)g_CurrentWeapon);
                    if (g_RunMelee)    { g_Stats.meleeWeapon = true; g_Stats.fireInterval = 0.26f; }
                    else if (g_RunBow) { g_Stats.bowWeapon = true;  g_Stats.bulletSpeed *= 1.4f; }
                    g_Stats.baseFireInterval = g_Stats.fireInterval;
                    g_OwnedAugs.clear();
                    memset(g_GameManager.takenOnce, 0,
                           sizeof(g_GameManager.takenOnce));
                    memset(g_TypeOwned, 0, sizeof(g_TypeOwned));   // 조합 ?�시??보유??초기??                    //   (?�으�??�블 ?�었?�데 '관???�둥?? 조합???�던 버그)
                    if (g_CurrentWeapon >= 0 && g_CurrentWeapon < (int)StartWeapon::_COUNT)
                        MarkStartWeaponOwnedType((StartWeapon)g_CurrentWeapon);
                    g_GameManager.maxHP = g_Stats.maxHP;
                    if (g_GameManager.playerHP > g_Stats.maxHP)
                        g_GameManager.playerHP = g_Stats.maxHP;
                    // 보유 개수 보존 ???�전??24�?캡해??40�?보유 ??반토�??�던 버그 ?�정.
                    if (prevAugs > 60) prevAugs = 60;       // 배열 ?�한(?�유)
                    int nDebuffs = prevAugs / 3;            // 40% ??33% (??가?�하�?
                    int nBuffs   = prevAugs - nDebuffs;
                    int buffs[64], debuffs[64];
                    g_GameManager.PickRandomAugIndices(buffs, nBuffs,
                        false, false, true, false, /*allowDebuff=*/false);
                    g_GameManager.PickRandomDebuffIndices(debuffs, nDebuffs);
                    for (int k = 0; k < nBuffs;   k++) applyByIdx(buffs[k]);
                    for (int k = 0; k < nDebuffs; k++) applyByIdx(debuffs[k]);
                    // ��ų ���� ? reroll �� ���� ���?���� ������
                    ReequipSkillsFromOwned(g_OwnedAugs.data(), (int)g_OwnedAugs.size());
                    // ���� �з�(����/ĸ/����)�� reroll ���� �� ���� �� ���� ? ��ȥ�� �� �ް� ����
                    g_Stats.mobSpawnMult  = std::min(g_Stats.mobSpawnMult,  saveSpawnMult);
                    g_Stats.mobCapBonus   = std::max(g_Stats.mobCapBonus,   saveCapBonus);
                    g_Stats.mobPackBonus  = std::max(g_Stats.mobPackBonus,  savePackBonus);
                    return;
                }
                if (atype == AugType::S_PANDORA) {
                    // PANDORA: 5??= 3 버프 + 2 ?�버??(명시??분리)
                    int buffs[3], debuffs[2];
                    g_GameManager.PickRandomAugIndices(buffs, 3,
                        g_Stats.sizeAugTaken, g_Stats.distAugTaken,
                        true, false, /*allowDebuff=*/false);
                    g_GameManager.PickRandomDebuffIndices(debuffs, 2);
                    for (int k = 0; k < 3; k++) applyByIdx(buffs[k]);
                    for (int k = 0; k < 2; k++) applyByIdx(debuffs[k]);
                    return;
                }
                if (atype == AugType::RANDOM_AUG) {
                    // RANDOM_AUG: 버프�?(?�버???�외, ?�용???�도 ?��?)
                    int picks[3];
                    g_GameManager.PickRandomAugIndices(picks, 3,
                        g_Stats.sizeAugTaken, g_Stats.distAugTaken,
                        true, /*allowSpecial=*/false,
                        /*allowDebuff=*/false);
                    for (int k = 0; k < 3; k++) applyByIdx(picks[k]);
                    return;
                }

                if (NeedsReplaceForAug(idx)) {
                    BeginAugReplaceFlow(idx, false, -1);
                    return;
                }
                ApplySingleAugIdx(idx, screenWidth, screenHeight);
            };

#endif
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
                    g_PostPickGrace = 0.5f;
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

        if (g_GameManager.currentState == GameState::AUG_REPLACE) {
            constexpr float REP_EXIT_DUR = 0.28f;
            static bool s_repEsc   = true;
            static bool s_repSpace = true;
            static bool s_repKeys[9] = { true,true,true,true,true,true,true,true,true };

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
        if (g_GameManager.currentState == GameState::RUN_SHOP) {
            int k1 = glfwGetKey(window, GLFW_KEY_1);
            int k2 = glfwGetKey(window, GLFW_KEY_2);
            int k3 = glfwGetKey(window, GLFW_KEY_3);
            int k4 = glfwGetKey(window, GLFW_KEY_4);
            static bool s_rs1 = true, s_rs2 = true, s_rs3 = true, s_rs4 = true;
            static bool s_rsSpace = true;
            if (k1 == GLFW_PRESS && s_rs1) { g_HoveredAug = 0; s_rs1 = false; }
            if (k2 == GLFW_PRESS && s_rs2) { g_HoveredAug = 1; s_rs2 = false; }
            if (k3 == GLFW_PRESS && s_rs3) { g_HoveredAug = 2; s_rs3 = false; }
            if (k4 == GLFW_PRESS && s_rs4) { g_HoveredAug = 3; s_rs4 = false; }
            if (k1 == GLFW_RELEASE) s_rs1 = true;
            if (k2 == GLFW_RELEASE) s_rs2 = true;
            if (k3 == GLFW_RELEASE) s_rs3 = true;
            if (k4 == GLFW_RELEASE) s_rs4 = true;

            int kSp = glfwGetKey(window, GLFW_KEY_SPACE);
            if (kSp == GLFW_PRESS && s_rsSpace && g_HoveredAug >= 0 && g_HoveredAug < 4) {
                RunShopPurchase(g_HoveredAug);
                s_rsSpace = false;
            }
            if (kSp == GLFW_RELEASE) s_rsSpace = true;

            if (lmb && !g_LmbPrev) {
                const int  nCards  = 4;
                const float CARD_W = 240.0f;
                const float CARD_H = 360.0f;
                const float GAP    = 32.0f;
                const float TOTAL_W = (float)nCards * CARD_W + (float)(nCards - 1) * GAP;
                float baseX = (screenWidth  - TOTAL_W) * 0.5f;
                float baseY = (screenHeight - CARD_H)  * 0.38f;
                for (int i = 0; i < nCards; i++) {
                    float cx = baseX + i * (CARD_W + GAP);
                    float yOff = (g_HoveredAug == i) ? -14.0f : 0.0f;
                    if (mx >= cx && mx <= cx + CARD_W &&
                        my >= baseY + yOff && my <= baseY + yOff + CARD_H) {
                        g_HoveredAug = i;
                        break;
                    }
                }
            }

            {
                int kEsc = glfwGetKey(window, GLFW_KEY_ESCAPE);
                static bool s_rsEsc = true;
                if (kEsc == GLFW_PRESS && s_rsEsc) {
                    CloseRunShop();
                    g_HoveredAug = -1;
                    g_GameManager.escReleased = false;
                    s_rsEsc = false;
                }
                if (kEsc == GLFW_RELEASE) s_rsEsc = true;
            }
        }

        // --- ?�리?�이?�브 모드: F = 증강 그랩(?�버???�함 ?�드박스), G = 무적 ?��? ---
        if (g_CreativeMode || g_DebugMode) {
            static bool s_fkeyReleased = true;
            int kF = glfwGetKey(window, GLFW_KEY_F);
            if (kF == GLFW_RELEASE) s_fkeyReleased = true;
            if (kF == GLFW_PRESS && s_fkeyReleased &&
                g_GameManager.currentState == GameState::RUNNING &&
                !g_GameManager.augReady) {
                // ?�드박스: ?�버?�도 카드 ?�???�어??무엇?�든 집을 ???�게
                 ++g_GameManager.playerLevel;
                 g_GameManager.xp = 0;
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
            if (kG == GLFW_RELEASE) s_gkeyReleased = true;
            if (kG == GLFW_PRESS && s_gkeyReleased &&
                g_GameManager.currentState == GameState::RUNNING) {
                g_CreativeGodmode = !g_CreativeGodmode;
                s_gkeyReleased = false;
            }
            // B ? ���� ���� ���?���� (None�̸� VOLLEY.sys)
            static bool s_bkeyReleased = true;
            int kB = glfwGetKey(window, GLFW_KEY_B);
            if (kB == GLFW_RELEASE) s_bkeyReleased = true;
            if (kB == GLFW_PRESS && s_bkeyReleased && g_CreativeMode &&
                g_GameManager.currentState == GameState::RUNNING) {
                if (!BossFightBusy()) {
                    float bossHpC = GetDifficultyParams(Difficulty::NORMAL).bossHp * TrialBossHpMult();
                    float polyHpC = 30000.0f * TrialBossHpMult();
                    int pick = g_CreativeBossPick >= 0 ? g_CreativeBossPick : 2;
                    QueueCreativeBossPick(pick, bossHpC, polyHpC);
                }
                s_bkeyReleased = false;
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
                g_PlayerDmgMult = g_Stats.GetDamageTakenMult();
                // WASD ?�동 ???�각??normalize (vec 모아??길이�??�눔)
                float mvX = 0.0f, mvY = 0.0f;
                if (keys[GLFW_KEY_W]) mvY -= 1.0f;
                if (keys[GLFW_KEY_S]) mvY += 1.0f;
                if (keys[GLFW_KEY_A]) mvX -= 1.0f;
                if (keys[GLFW_KEY_D]) mvX += 1.0f;
                float mlen = sqrtf(mvX*mvX + mvY*mvY);
                if (mlen > 0.001f && !g_DashActive) {
                    mvX /= mlen; mvY /= mlen;
                    float moveMult = g_Stats.GetMoveMultiplier(lmb);
                    // 배드 ?�터 감속 구역 ??부?�되???�진 ?�역(grow factor) ?�이�??�동?�도 -10%
                    float zoneSlow = 1.0f;
                    {
                        float pcx = playerWin.x + playerWin.width  * 0.5f;
                        float pcy = playerWin.y + playerWin.height * 0.5f;
                        for (auto& z : g_SlowZones) {
                            float gf = SlowZoneGrow(z);
                            float hw = z.w * 0.5f * gf, hh = z.h * 0.5f * gf;
                            float zx = z.x + z.w * 0.5f, zy = z.y + z.h * 0.5f;
                            if (pcx >= zx - hw && pcx <= zx + hw &&
                                pcy >= zy - hh && pcy <= zy + hh) { zoneSlow = 0.90f; break; }
                        }
                        if (g_EtherBoss && g_EtherBoss->alive &&
                            g_EtherBoss->IsBadSector(pcx, pcy)) {
                            zoneSlow = std::min(zoneSlow, 0.50f);
                        }
                    }
                    float curMove  = MOVE_SPEED * moveMult * zoneSlow;
                    playerWin.x += mvX * curMove * FIXED_DT;
                    playerWin.y += mvY * curMove * FIXED_DT;
                    // FORK.worm ?�래??�????�반 ?�동?�?직선??막힘, ?�??무적 �????�과
                    if (g_CentiBoss && g_CentiBoss->alive && g_DashInvuln <= 0.0f) {
                        float wpcx = playerWin.x + playerWin.width  * 0.5f;
                        float wpcy = playerWin.y + playerWin.height * 0.5f;
                        g_CentiBoss->blockMove(wpcx, wpcy, 18.0f);
                        playerWin.x = wpcx - playerWin.width  * 0.5f;
                        playerWin.y = wpcy - playerWin.height * 0.5f;
                    }
                    if (g_TessBoss && g_TessBoss->alive && g_DashInvuln <= 0.0f) {
                        float wpcx = playerWin.x + playerWin.width  * 0.5f;
                        float wpcy = playerWin.y + playerWin.height * 0.5f;
                        g_TessBoss->blockMove(wpcx, wpcy, 18.0f);
                        playerWin.x = wpcx - playerWin.width  * 0.5f;
                        playerWin.y = wpcy - playerWin.height * 0.5f;
                    }
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

                // ?�레?�어 캐릭??�?중앙)가 보이???�역 밖으�??��?지 못하�??�램??
                // 줌아???�리모프 2?�이�??�면 보이???�드가 ?�어지므�??�동 구역??같이 ?�장.
                // (zoom=1 ?�면 ?�확??[0, screen] ??기존�??�일)
                float zoomNow = (g_ViewZoom < 0.01f) ? 0.01f : g_ViewZoom;
                float halfW = (float)screenWidth  * 0.5f / zoomNow;
                float halfH = (float)screenHeight * 0.5f / zoomNow;
                float ccX   = (float)screenWidth  * 0.5f;
                float ccY   = (float)screenHeight * 0.5f;
                float pCX = playerWin.x + playerWin.width  * 0.5f;
                float pCY = playerWin.y + playerWin.height * 0.5f;
                if (pCX < ccX - halfW) pCX = ccX - halfW;
                if (pCX > ccX + halfW) pCX = ccX + halfW;
                // ���? �÷��̾� �߽��� Ÿ��Ʋ�� �Ʒ��� ��ġ
                {
                    float topLimit = ccY + (BROWSER_CHROME_H - ccY) / zoomNow;
                    if (pCY < topLimit) pCY = topLimit;
                }
                // C17: ?�단 ?�업?�시�??�게??가�?�?+ ?�제 OS ?�업?�시�? 침범 방�? ??                //   ?�업?�시줄�? '?�크�? ?�단 고정 ?��??�라, 줌아???�수 ?�장)?�면 ?�드 ?�위�?                //   barTotal/zoom 만큼 차�??? 보이???�역 ?�단(ccY+halfH)?�서 그만???��? ?�계 ??                //   ?�단 ?�장�??��?�� ?�도�?기존??sh-barTotal 고정?�라 ?�단�????�어?�음).
                float barTotal    = g_GameBarH + (float)g_TaskbarH;
                float bottomLimit = ccY + halfH - barTotal / zoomNow;
                if (pCY > bottomLimit) pCY = bottomLimit;
                playerWin.x = pCX - playerWin.width  * 0.5f;
                playerWin.y = pCY - playerWin.height * 0.5f;

                // ?�?�??�티�??�킬 ??쿨다??지??갱신 + ?�력(Shift ?�??/ Q·E·R ?�롯) ?�?�?
                if (g_DashCd > 0.0f)        g_DashCd        -= FIXED_DT;
                if (g_DashInvuln > 0.0f)    g_DashInvuln    -= FIXED_DT;
                if (g_PostPickGrace > 0.0f) g_PostPickGrace -= FIXED_DT;  // C14
                if (g_TimeStopTimer > 0.0f) g_TimeStopTimer -= FIXED_DT;
                if (g_HyperFocusTimer > 0.0f) g_HyperFocusTimer -= FIXED_DT;
                for (int i = 0; i < 3; i++) if (g_Skills[i].cd > 0.0f) g_Skills[i].cd -= FIXED_DT;

                // �??�기 ???�레?�어 중심 ??�� (?�백 + ?�해)
                auto closeWindowBlast = [&](float cx, float cy) {
                    float dmg = g_Stats.GetBaseDamage() * g_Stats.GetDamageMultiplier(0.0f) * 8.0f;
                    float rad = 380.0f, r2 = rad * rad, knock = 130.0f;
                    auto hitKB = [&](float& ex, float& ey, float& hp, bool& al) {
                        float dx = ex - cx, dy = ey - cy, d2 = dx*dx + dy*dy;
                        if (d2 < r2) { hp -= dmg; float d = std::sqrt(d2)+1e-3f;
                            ex += dx/d*knock; ey += dy/d*knock; if (hp <= 0) al = false; }
                    };
                    for (auto m  : g_MonsterManager.monsters)   if (m->alive)  hitKB(m->worldX,  m->worldY,  m->hp,  m->alive);
                    for (auto r  : g_MonsterManager.rangedMobs) if (r->alive)  hitKB(r->worldX,  r->worldY,  r->hp,  r->alive);
                    for (auto bm : g_MonsterManager.bombers)    if (bm->alive) hitKB(bm->worldX, bm->worldY, bm->hp, bm->alive);
                    auto hitBossBlast = [&](float ex, float ey, float& hp, bool& al) {
                        float dx = ex - cx, dy = ey - cy;
                        if (dx*dx + dy*dy >= r2) return;
                        float dealt = std::min(dmg, hp);
                        hp -= dealt;
                        SpawnDamageNumber(ex, ey, dealt, dealt >= 40.0f);
                        ApplyBossLifestealFromDamage(dealt);
                        if (hp <= 0.0f) al = false;
                    };
                    if (g_RRBoss && g_RRBoss->alive)
                        hitBossBlast(g_RRBoss->worldX, g_RRBoss->worldY, g_RRBoss->hp, g_RRBoss->alive);
                    if (g_CentiBoss && g_CentiBoss->alive && g_CentiBoss->vulnerable())
                        hitBossBlast(g_CentiBoss->worldX, g_CentiBoss->worldY, g_CentiBoss->hp, g_CentiBoss->alive);
                    if (g_TessBoss && g_TessBoss->alive)
                        hitBossBlast(g_TessBoss->worldX, g_TessBoss->worldY, g_TessBoss->hp, g_TessBoss->alive);
                    if (g_EtherBoss && g_EtherBoss->alive) {
                        float dx = g_EtherBoss->worldX - cx, dy = g_EtherBoss->worldY - cy;
                        if (dx*dx + dy*dy < r2) HitEtherBossDirect(dmg, cx, cy, false);
                    }
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
                        g_HyperFocusTimer = HYPER_FOCUS_DUR;
                        TriggerFlash(0.5f, 0.75f, 1.0f, 0.35f);
                        break;
                    case SkillType::TIME_STOP:    g_TimeStopTimer  = TIMESTOP_DUR;
                                                  TriggerFlash(0.4f,0.9f,1.0f,0.4f); break;
                    case SkillType::FOCUS_AIM:
                        if (g_FocusStandTimer >= 0.4f) {
                            g_FocusShotsLeft = 1;
                            TriggerFlash(0.9f, 0.95f, 1.0f, 0.2f);
                            s.cd = SkillCooldownMax(s.type);
                        }
                        return;
                    default: break;
                    }
                    s.cd = SkillCooldownMax(s.type);
                };

                // ���� �ð� (���� ���� ��ų)
                if (mlen <= 0.001f && !g_DashActive)
                    g_FocusStandTimer += FIXED_DT;
                else
                    g_FocusStandTimer = 0.0f;

                // �÷��� ���?? ���� �̵� + ª�� ����
                static bool pDash=false, pQ=false, pE=false, pR=false;
                if (g_DashActive) {
                    g_DashT += FIXED_DT / DASH_DUR;
                    if (g_DashT >= 1.0f) {
                        g_DashT = 1.0f;
                        g_DashActive = false;
                    }
                    float u = g_DashT;
                    pCX = g_DashFromX + (g_DashToX - g_DashFromX) * u;
                    pCY = g_DashFromY + (g_DashToY - g_DashFromY) * u;
                    playerWin.x = pCX - playerWin.width * 0.5f;
                    playerWin.y = pCY - playerWin.height * 0.5f;
                    g_DashInvuln = DASH_INVULN;
                }
                bool cDash = keys[GLFW_KEY_LEFT_SHIFT] || keys[GLFW_KEY_RIGHT_SHIFT];
                const float dashCdMax = (g_HyperFocusTimer > 0.0f) ? HYPER_FOCUS_DASH_CD : DASH_CD;
                if (cDash && !pDash && g_DashCd <= 0.0f && !g_DashActive) {
                    float ddx = mvX, ddy = mvY;
                    if (mlen <= 0.001f) {
                        float ax = wmx - pCX, ay = wmy - pCY;
                        float al = std::sqrt(ax*ax+ay*ay)+1e-3f; ddx = ax/al; ddy = ay/al;
                    }
                    g_DashFromX = pCX;  g_DashFromY = pCY;
                    g_DashToX   = pCX + ddx * DASH_DIST;
                    g_DashToY   = pCY + ddy * DASH_DIST;
                    if (g_DashToX < ccX-halfW) g_DashToX = ccX-halfW;
                    if (g_DashToX > ccX+halfW) g_DashToX = ccX+halfW;
                    { float dtl = ccY + (BROWSER_CHROME_H - ccY) / zoomNow;
                      if (g_DashToY < dtl) g_DashToY = dtl; }
                    if (g_DashToY > ccY+halfH) g_DashToY = ccY+halfH;
                    g_DashActive = true; g_DashT = 0.0f;
                    g_DashCd = dashCdMax;
                    g_DashInvuln = DASH_INVULN;
                    TriggerFlash(0.35f, 0.85f, 1.0f, 0.12f);
                    SpawnShockWave(g_DashFromX, g_DashFromY, 70.0f, 0.25f, 0.4f, 1.0f, 1.0f);
                    if (g_Stats.dashUpgrade) {
                        g_DashBoostShotsLeft = 3;
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
                pDash = cDash;
                bool cQ = keys[GLFW_KEY_Q]; if (cQ && !pQ) useSkill(0); pQ = cQ;
                bool cE = keys[GLFW_KEY_E]; if (cE && !pE) useSkill(1); pE = cE;
                bool cR = keys[GLFW_KEY_R]; if (cR && !pR) useSkill(2); pR = cR;
                // C16: ?�티�??�킬 ?�동 ?�용 ??쿨다???�난 ?�롯???�동 발동
                if (g_AutoSkill) for (int i = 0; i < 3; i++) useSkill(i);

                // GRAVIS field: one force evaluation per active station. It
                // bends velocity only, so bullets keep their forward motion.
                for (auto* gravis : g_MonsterManager.monsters) {
                    if (!gravis || !gravis->alive || gravis->kind != MobKind::GRAVIS) continue;
                    const float fieldR = 285.0f;
                    float gx = gravis->worldX - pCX;
                    float gy = gravis->worldY - pCY;
                    float gd2 = gx * gx + gy * gy;
                    if (!g_DashActive && gd2 > 36.0f && gd2 < fieldR * fieldR) {
                        const float gd = sqrtf(gd2);
                        const float influence = 1.0f - gd / fieldR;
                        const float pullStep = (24.0f + 58.0f * influence) * influence * FIXED_DT;
                        pCX += gx / gd * pullStep;
                        pCY += gy / gd * pullStep;
                    }
                    for (auto& b : g_Bullets) {
                        if (!b.active) continue;
                        float bx = gravis->worldX - b.x;
                        float by = gravis->worldY - b.y;
                        float bd2 = bx * bx + by * by;
                        if (bd2 <= 64.0f || bd2 >= fieldR * fieldR) continue;
                        const float bd = sqrtf(bd2);
                        const float influence = 1.0f - bd / fieldR;
                        const float curve = (0.34f + 1.18f * influence) * FIXED_DT;
                        b.dirX += bx / bd * curve;
                        b.dirY += by / bd * curve;
                        const float dl = sqrtf(b.dirX * b.dirX + b.dirY * b.dirY);
                        if (dl > 0.0001f) { b.dirX /= dl; b.dirY /= dl; }
                    }
                }
                pCX = std::max(ccX - halfW, std::min(ccX + halfW, pCX));
                pCY = std::max(ccY + (BROWSER_CHROME_H - ccY) / zoomNow,
                               std::min(bottomLimit, pCY));
                playerWin.x = pCX - playerWin.width * 0.5f;
                playerWin.y = pCY - playerWin.height * 0.5f;

                // 총알 ?�동 + ?�면 �?비활?�화 (+ ?�도??보정)
                for (auto& b : g_Bullets) {
                    // ?�도?? 가??가까운 ?�을 ?�해 ?�진??방향 보정
                    if (b.homing && !b.isEnemy && b.active) {
                        float tx = 0.0f, ty = 0.0f;
                        if (findNearestEnemy(b.x, b.y, tx, ty)) {
                            // ?�재 방향 ??목표 방향 ?�이�?turn rate 만큼 ?�전
                            float wx = tx - b.x, wy = ty - b.y;
                            float wl = sqrtf(wx*wx + wy*wy);
                            if (wl > 0.001f) {
                                float wdx = wx / wl, wdy = wy / wl;
                                // ?�적/?�적?�로 각도 차이 (?��? ?�계)
                                float curA   = atan2f(b.dirY, b.dirX);
                                float wantA  = atan2f(wdy, wdx);
                                float diff   = wantA - curA;
                                while (diff >  3.14159265f) diff -= 6.2831853f;
                                while (diff < -3.14159265f) diff += 6.2831853f;
                                float maxStep = b.homingTurn * FIXED_DT;
                                if (diff >  maxStep) diff =  maxStep;
                                if (diff < -maxStep) diff = -maxStep;
                                float newA = curA + diff;
                                b.dirX = cosf(newA);
                                b.dirY = sinf(newA);
                            }
                        }
                    }
                    // ???�도???�거리몹 ??: ?�레?�어 쪽으�??�주 ?�하�?방향 보정.
                    //   ?? ?�레?�어 근처(<300px)?�선 ?�도 중단 ??빗나�??�이 공전?��? ?�고
                    //   그�?�?지?�감(?�전 0.9 rad/s 가 ?�레?�어 주위�??�는 문제 ?�정).
                    if (b.homing && b.isEnemy && b.active && g_TimeStopTimer <= 0.0f) {
                        float pCX = playerWin.x + playerWin.width  * 0.5f;
                        float pCY = playerWin.y + playerWin.height * 0.5f;
                        float wx = pCX - b.x, wy = pCY - b.y;
                        float wl = sqrtf(wx*wx + wy*wy);
                        float homDt = FIXED_DT * ((g_HyperFocusTimer > 0.0f) ? 0.7f : 1.0f);
                        if (wl > 300.0f) {
                            float curA  = atan2f(b.dirY, b.dirX);
                            float wantA = atan2f(wy / wl, wx / wl);
                            float diff  = wantA - curA;
                            while (diff >  3.14159265f) diff -= 6.2831853f;
                            while (diff < -3.14159265f) diff += 6.2831853f;
                            float maxStep = b.homingTurn * homDt;
                            if (diff >  maxStep) diff =  maxStep;
                            if (diff < -maxStep) diff = -maxStep;
                            float newA = curA + diff;
                            b.dirX = cosf(newA); b.dirY = sinf(newA);
                        }
                    }
                    // �ð� ����: �� ź�� ����. ������: �� ź 30% ����
                    float bDt = FIXED_DT;
                    if (b.isEnemy && g_TimeStopTimer > 0.0f) { /* skip update below */ }
                    else if (b.isEnemy && g_HyperFocusTimer > 0.0f) bDt *= 0.7f;
                    if (!(b.isEnemy && g_TimeStopTimer > 0.0f)) b.Update(bDt);
                    // ?�면 �?비활?�화 ??줌아???�리모프 2?�이�??�면 보이???�역??                    // ?�어지므�?경계??같이 ?�장 (??그러�??�장 구역?�서 ?�이 즉시 ?�라�?
                    {
                        float zb = (g_ViewZoom < 0.01f) ? 0.01f : g_ViewZoom;
                        float mX = screenWidth  * 0.5f * (1.0f / zb - 1.0f) + 200.0f;
                        float mY = screenHeight * 0.5f * (1.0f / zb - 1.0f) + 200.0f;
                        if (b.x < -mX || b.x > screenWidth  + mX ||
                            b.y < -mY || b.y > screenHeight + mY)
                            b.active = false;
                    }
                }

                // ?�??무적 ???�번 ?�텝 ?�작 HP ?�??(???�해??무효, ?�복?�??��?)
                float hpAtStep = g_GameManager.playerHP;
                TickPlayerShield(FIXED_DT);
                //   ???��? = ?�간?��? ?�킬 OR 증강 ??직후 ~0.05s("?�이 ?�?, 짧게)
                bool  timeStopped = (g_TimeStopTimer > 0.0f) || (g_PostPickGrace > 0.45f);
                float enemyDt = FIXED_DT;
                if (!timeStopped && g_HyperFocusTimer > 0.0f) enemyDt *= 0.7f;
                float focusSlow = (g_HyperFocusTimer > 0.0f) ? 0.7f : 1.0f;

                // 몬스???�데?�트 (?�버??multiplier ?�용) ???�간 ?��? 중엔 ??멈춤
                float rmobMoveMult = 1.0f / g_Stats.rmobDelayMult; // <1 ????빠름
                // Time-based speed ramp. Density is lower now, so enemy pressure should not depend
                // only on score gained from kills.
                BossDir::ActRules actSpd = BossDir::GetActRules();
                float si = g_GameTime / 240.0f;
                if (si > actSpd.intensityCap) si = actSpd.intensityCap;
                float mobSpdRamp = (1.0f + si * 0.075f) * actSpd.speedMult;
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
                                               rmobMoveMult * mobSpdRamp * focusSlow);

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
                if (!timeStopped && g_RRBoss && g_RRBoss->alive)
                    g_RRBoss->Update(pCX, pCY, enemyDt, g_GameManager.playerHP, g_Bullets, g_Difficulty);

                if (!timeStopped && g_TessBoss && g_TessBoss->alive)
                    g_TessBoss->Update(pCX, pCY, enemyDt, g_GameManager.playerHP,
                                       g_Bullets, g_Difficulty, g_DashInvuln > 0.0f);

                if (!timeStopped && g_EtherBoss && g_EtherBoss->alive)
                    g_EtherBoss->Update(pCX, pCY, g_EtherBoss->arenaRadius,
                                        enemyDt, g_GameManager.playerHP,
                                        g_Bullets, g_Difficulty, g_Stats.maxHP);


                // ?�리모프 ?�데?�트 (??변??+ ?�모/?�이?�?차크?? + ?�이�? ?�면 ?�장

                
                // FORK.worm ?�데?�트 (지그재�?배회 + ?�면�??�탈?�재진입 ?�진)
                if (!timeStopped && g_CentiBoss && g_CentiBoss->alive) {
                    g_CentiBoss->Update(pCX, pCY, enemyDt, g_GameManager.playerHP, g_Bullets);
                    if (g_CentiBoss->shakePulse) {          // ?�면 밖으�??�갈 ???�한 진동
                        g_CentiBoss->shakePulse = false;
                        g_ShakeTime = 0.35f; g_ShakeMag = 14.0f;
                    }
                    // ?�킬 ?�전 진동(가벼운 ?�드�? ?�뽕 X) ????진동 중이�???��?��? ?�음
                    if (g_CentiBoss->wantShake > 0.0f) {
                        if (g_ShakeTime <= 0.0f || g_CentiBoss->wantShake > g_ShakeMag) {
                            g_ShakeTime = 0.22f; g_ShakeMag = g_CentiBoss->wantShake;
                        }
                        g_CentiBoss->wantShake = 0.0f;
                    }
                    if (g_CentiBoss->moltGlitchPulse) {
                        g_CentiBoss->moltGlitchPulse = false;
                        TriggerFlash(1.0f, 1.0f, 1.0f, 0.28f);
                        TriggerHitStop(0.05f);
                        g_ShakeTime = 0.18f; g_ShakeMag = 12.0f;
                    }
                }

                // ���� ���� ������ ���� (�����?) ���� ���� ���� ���� + ������ ó�� ����
                auto p2enter = [&](float bx, float by, glm::vec3 col) {
                    g_ShakeTime = 0.55f; g_ShakeMag = 26.0f;
                    TriggerFlash(col.r, col.g, col.b, 0.55f);
                    TriggerHitStop(0.12f);
                    SpawnShockWave(bx, by, 520.0f, 0.9f, col.r, col.g, col.b);
                    SpawnShockWave(bx, by, 300.0f, 0.7f, col.r, col.g, col.b);
                    for (int k = 0; k < 4; k++)
                        SpawnEnemyExplosion(bx + (rand()%200 - 100), by + (rand()%200 - 100),
                                            col.r, col.g, col.b, true);
                    g_P2ToastCol = col; g_P2ToastTimer = 1.8f;
                };
                if (g_RRBoss && g_RRBoss->alive) {
                    if (g_RRBoss->phase2 && !g_RRWasP2) {
                        g_RRWasP2 = true;
                        p2enter(g_RRBoss->worldX, g_RRBoss->worldY, glm::vec3(1.0f, 0.55f, 0.2f));
                    }
                    if (g_RRBoss->phase3 && !g_RRWasP3) {
                        g_RRWasP3 = true;
                        p2enter(g_RRBoss->worldX, g_RRBoss->worldY, glm::vec3(1.0f, 0.22f, 0.08f));
                    }
                } else { g_RRWasP2 = false; g_RRWasP3 = false; }
                if (g_TessBoss && g_TessBoss->alive) {
                    if (g_TessBoss->phase2 && !g_TessWasP2) {
                        g_TessWasP2 = true;
                        p2enter(g_TessBoss->worldX, g_TessBoss->worldY, glm::vec3(0.95f, 0.35f, 1.0f));
                    }
                    if (g_TessBoss->phase3 && !g_TessWasP3) {
                        g_TessWasP3 = true;
                        p2enter(g_TessBoss->worldX, g_TessBoss->worldY, glm::vec3(1.0f, 0.20f, 0.95f));
                    }
                } else { g_TessWasP2 = false; g_TessWasP3 = false; }

                // 충돌 (반환�?= ?�레?�어가 ?�번 ?�레???�격?�는지)
                bool hit = CollisionSystem::Update(pCX, pCY,
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
                if ((g_DashInvuln > 0.0f || g_PostPickGrace > 0.0f) &&
                    g_GameManager.playerHP < hpAtStep)
                    g_GameManager.playerHP = hpAtStep;

                auto silverBossBonus = [&](const Bullet& b, float pd) -> float {
                    if (!b.silverBurn || b.remainingDmg > 0.0f || b.turretDmg > 0.0f)
                        return 0.0f;
                    return g_Stats.GetBaseDamage()
                         * g_Stats.GetDamageMultiplier(pd)
                         * b.dmgMult * 1.20f;
                };

                // 리로???�너 본체 vs ?�레?�어 총알 (?�윕 ?�정)
                if (g_RRBoss && g_RRBoss->alive) {
                    auto* rb = g_RRBoss;
                    for (auto& b : g_Bullets) {
                        if (!b.active || b.isEnemy) continue;
                        if (SegDist(rb->worldX, rb->worldY,
                                    b.prevX, b.prevY, b.x, b.y) < ReloadRunnerBoss::BODY * 0.7f) {
                            float pd = glm::distance(glm::vec2(pCX, pCY),
                                                     glm::vec2(rb->worldX, rb->worldY));
                            float dmg;
                            if (b.remainingDmg > 0.0f)   dmg = b.remainingDmg;
                            else if (b.turretDmg > 0.0f) dmg = b.turretDmg;
                            else dmg = g_Stats.GetBaseDamage()
                                     * g_Stats.GetDamageMultiplier(pd) * b.dmgMult;
                            dmg += silverBossBonus(b, pd);
                            float dealt = (dmg < rb->hp) ? dmg : rb->hp;
                            rb->hp -= dealt;
                            ApplyBossLifestealFromDamage(dealt);
                            if (b.remainingDmg > 0.0f) b.remainingDmg -= dealt;
                            if (rb->hp <= 0.0f) rb->alive = false;
                            if (b.remainingDmg <= 0.001f) b.active = false;
                        }
                    }
                }

                if (g_TessBoss && g_TessBoss->alive) {
                    auto* tb = g_TessBoss;
                    for (auto& b : g_Bullets) {
                        if (!b.active || b.isEnemy) continue;
                        bool consumed = false;
                        for (auto& o : tb->orbs) {
                            if (!o.alive) continue;
                            if (SegDist(o.x, o.y, b.prevX, b.prevY, b.x, b.y) < TesseractGlitchBoss::ORB_R + 8.0f) {
                                float pd = glm::distance(glm::vec2(pCX, pCY), glm::vec2(o.x, o.y));
                                float dmg;
                                if (b.remainingDmg > 0.0f)   dmg = b.remainingDmg;
                                else if (b.turretDmg > 0.0f) dmg = b.turretDmg;
                                else dmg = g_Stats.GetBaseDamage()
                                         * g_Stats.GetDamageMultiplier(pd) * b.dmgMult;
                                dmg += silverBossBonus(b, pd);
                                float dealt = (dmg < o.hp) ? dmg : o.hp;
                                o.hp -= dealt;
                                ApplyBossLifestealFromDamage(dealt);
                                if (b.remainingDmg > 0.0f) b.remainingDmg -= dealt;
                                if (o.hp <= 0.0f) {
                                    o.alive = false;
                                    tb->pushSpark(o.x, o.y, 0.95f, 0.35f, 1.0f);
                                    AddKillCombo();
                                    g_GameManager.scoreAccum += 90.0f;
                                }
                                if (b.remainingDmg <= 0.001f) { b.active = false; consumed = true; }
                                break;
                            }
                        }
                        if (consumed || !b.active) continue;
                        if (SegDist(tb->worldX, tb->worldY,
                                    b.prevX, b.prevY, b.x, b.y) < TesseractGlitchBoss::CORE_R * 0.86f) {
                            float pd = glm::distance(glm::vec2(pCX, pCY),
                                                     glm::vec2(tb->worldX, tb->worldY));
                            float dmg;
                            if (b.remainingDmg > 0.0f)   dmg = b.remainingDmg;
                            else if (b.turretDmg > 0.0f) dmg = b.turretDmg;
                            else dmg = g_Stats.GetBaseDamage()
                                     * g_Stats.GetDamageMultiplier(pd) * b.dmgMult;
                            dmg += silverBossBonus(b, pd);
                            float dealt = (dmg < tb->hp) ? dmg : tb->hp;
                            tb->hp -= dealt;
                            ApplyBossLifestealFromDamage(dealt);
                            if (b.remainingDmg > 0.0f) b.remainingDmg -= dealt;
                            if (tb->hp <= 0.0f) tb->alive = false;
                            if (b.remainingDmg <= 0.001f) b.active = false;
                        }
                    }
                }


                
                // FORK.worm ?�래??�???총알??직선??가로�?르면 �??��??�림.
                //   ?�반?��? 벽에 막�? ?�멸(관??X). 관?�탄(remainingDmg>0)?�??�과.
                if (g_EtherBoss && g_EtherBoss->alive && !g_EtherBoss->BodyInvulnerable()) {
                    auto* eb = g_EtherBoss;
                    for (auto& b : g_Bullets) {
                        if (!b.active || b.isEnemy) continue;
                        float dx = eb->worldX - b.x, dy = eb->worldY - b.y;
                        if (dx*dx + dy*dy < EtherSwordBoss::HIT_RADIUS * EtherSwordBoss::HIT_RADIUS) {
                            float pd  = glm::distance(glm::vec2(pCX, pCY), glm::vec2(eb->worldX, eb->worldY));
                            float dmg;
                            if (b.remainingDmg > 0.0f)   dmg = b.remainingDmg;
                            else if (b.turretDmg > 0.0f) dmg = b.turretDmg;
                            else dmg = g_Stats.GetBaseDamage()
                                     * g_Stats.GetDamageMultiplier(pd) * b.dmgMult;
                            dmg += silverBossBonus(b, pd);
                            dmg *= eb->PlayerDamageToBossMul(pCX, pCY);
                            float killFloor = eb->taskKillHp();
                            float dealt = std::min(dmg, std::max(0.0f, eb->hp - killFloor));
                            eb->hp -= dealt;
                            ApplyBossLifestealFromDamage(dealt);
                            if (b.remainingDmg > 0.0f) b.remainingDmg -= dealt;
                            if (eb->hp <= killFloor) {
                                eb->hp = killFloor;
                                b.active = false;
                                eb->BeginTaskKill(pCX, pCY, g_Bullets);
                                break;
                            }
                            if (b.remainingDmg <= 0.001f) b.active = false;
                        }
                    }
                }

                if (g_TessBoss && g_TessBoss->alive && !g_TessBoss->walls.empty()) {
                    for (auto& b : g_Bullets) {
                        if (!b.active || b.isEnemy) continue;
                        if (g_TessBoss->hitWall(b.prevX, b.prevY, b.x, b.y) && b.remainingDmg <= 0.0f)
                            b.active = false;
                    }
                }
                if (g_CentiBoss && g_CentiBoss->alive && !g_CentiBoss->walls.empty()) {
                    for (auto& b : g_Bullets) {
                        if (!b.active || b.isEnemy) continue;
                        if (g_CentiBoss->hitWall(b.prevX, b.prevY, b.x, b.y) && b.remainingDmg <= 0.0f)
                            b.active = false;
                    }
                }

                // FORK.worm child adds vs ?�레?�어 총알 ????�� ?�격 가?? 경험�?0.
                if (g_CentiBoss && g_CentiBoss->alive && !g_CentiBoss->minis.empty()) {
                    for (auto& mb : g_CentiBoss->minis) {
                        if (!mb.alive) continue;
                        for (auto& b : g_Bullets) {
                            if (!b.active || b.isEnemy) continue;
                            if (SegDist(mb.x, mb.y, b.prevX, b.prevY, b.x, b.y) < CentipedeBoss::MINI_HEAD + 6.0f) {
                                float pd = glm::distance(glm::vec2(pCX, pCY), glm::vec2(mb.x, mb.y));
                                float dmg;
                                if (b.remainingDmg > 0.0f)   dmg = b.remainingDmg;
                                else if (b.turretDmg > 0.0f) dmg = b.turretDmg;
                                else dmg = g_Stats.GetBaseDamage() * g_Stats.GetDamageMultiplier(pd) * b.dmgMult;
                                dmg += silverBossBonus(b, pd);
                                float dealt = (dmg < mb.hp) ? dmg : mb.hp;
                                mb.hp -= dealt;
                                ApplyBossLifestealFromDamage(dealt);
                                if (b.remainingDmg > 0.0f) b.remainingDmg -= dealt;
                                if (mb.hp <= 0.0f) {
                                    mb.alive = false;
                                    AddKillCombo();                 // 콤보�? 경험�?0
                                    g_GameManager.scoreAccum += 80.0f;
                                }
                                if (b.remainingDmg <= 0.001f) b.active = false;
                                break;
                            }
                        }
                    }
                }

                // FORK.worm 몸통 ?�드 ???�격 VFX (?��?지??머리 HP ?�? ?�래?�만)
                if (g_CentiBoss && g_CentiBoss->alive && g_CentiBoss->vulnerable()) {
                    auto* cb2 = g_CentiBoss;
                    for (auto& b : g_Bullets) {
                        if (!b.active || b.isEnemy) continue;
                        for (int si = 1; si <= cb2->activeSeg; si++) {
                            glm::vec2 sp = cb2->segPos(si);
                            float sr = cb2->segSize(si) + 6.0f;
                            if (SegDist(sp.x, sp.y, b.prevX, b.prevY, b.x, b.y) < sr) {
                                cb2->onSegHit(si, sp.x, sp.y);
                                break;
                            }
                        }
                    }
                }
                if (g_CentiBoss && g_CentiBoss->alive && g_CentiBoss->vulnerable()) {
                    auto* cb2 = g_CentiBoss;
                    for (auto& b : g_Bullets) {
                        if (!b.active || b.isEnemy) continue;
                        if (SegDist(cb2->worldX, cb2->worldY,
                                    b.prevX, b.prevY, b.x, b.y) < CentipedeBoss::HEAD * 1.0f) {
                            float pd = glm::distance(glm::vec2(pCX, pCY),
                                                     glm::vec2(cb2->worldX, cb2->worldY));
                            float dmg;
                            if (b.remainingDmg > 0.0f)   dmg = b.remainingDmg;
                            else if (b.turretDmg > 0.0f) dmg = b.turretDmg;
                            else dmg = g_Stats.GetBaseDamage()
                                     * g_Stats.GetDamageMultiplier(pd) * b.dmgMult;
                            dmg += silverBossBonus(b, pd);
                            dmg *= cb2->dmgTakenMult;   // ?�로?��??? 보통 ?�해 감소
                            float dealt = (dmg < cb2->hp) ? dmg : cb2->hp;
                            cb2->hp -= dealt;
                            ApplyBossLifestealFromDamage(dealt);
                            if (b.remainingDmg > 0.0f) b.remainingDmg -= dealt;
                            if (cb2->hp <= 0.0f) cb2->alive = false;
                            if (b.remainingDmg <= 0.001f) b.active = false;
                        }
                    }
                }


                // 처치 보상 ?�산 ??총알/근접???�닌 모든 죽음(?�쇄??��·?�킬·MK2·?�킹 ??�� ??
                //   ???�기????번씩 EXP/?�수/콤보/?�혈??받는??(scored ?�래그로 중복 방�?).
                // Kill XP is carried by the spawned stardust and awarded on pickup.
                auto creditKill = [&](float scoreBase) {
                    AddKillCombo();
                    g_Stats.killCount       += 1;
                    g_GameManager.scoreAccum += scoreBase;
                    g_GameManager.score      = (long long)g_GameManager.scoreAccum;
                    g_RunGold               += 1;
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
                std::vector<Monster*> mobBorn;
                for (auto m : g_MonsterManager.monsters) {
                    if (!m->alive && !m->exploded) {
                        if (!m->scored) {       // ?�직 보상 ??받�? 죽음 ???�산
                            m->scored = true;
                            float xpB, scB; MobKillReward(m->kind, m->splitGen, m->elite, xpB, scB,
                                                          g_Stats.splitterBoost);
                            TryHackFirewallOnKill(m->kind, g_Stats);
                            float rwm = MobRewardMult(m->kind); xpB *= rwm; scB *= rwm;
                            const long long pickupXp = (long long)(
                                (xpB + MobDebuffXpBonus(m->kind, m->elite, g_Stats))
                                * g_Stats.xpMult);
                            SpawnStardust(m->worldX, m->worldY,
                                          StardustRewardFor(m->kind), pCX, pCY, pickupXp);
                            creditKill(scB);
                            if (m->elite) g_RunGold += 1;
                        }
                        if (m->kind == MobKind::DDOS)
                            SpawnNodeDeath(m->worldX, m->worldY, m->color.r, m->color.g, m->color.b);
                        else
                            SpawnEnemyExplosion(m->worldX, m->worldY, m->color.r, m->color.g, m->color.b, false);
                        m->exploded = true;
                        // 처치 ?�출 ???�로?�스 종료 ?�그 (강적?�???�� 강조)
                        {
                            static const wchar_t* W[5] =
                                { L"terminated", L"killed", L"ended", L"exited", L"0x1B" };
                            bool notable = (m->elite != 0 || m->kind == MobKind::BRUTE);
                            if (notable)
                                SpawnKillTag(m->worldX, m->worldY, 1.0f, 0.55f, 0.3f,
                                             L"TERMINATED", true);
                            else
                                SpawnKillTag(m->worldX, m->worldY, 0.85f, 0.95f, 1.0f,
                                             W[rand() % 5], false);
                        }
                        // ??��???�리????죽을 ???�져 ?�레?�어?�게 광역 ?�해
                        if (m->elite == 3) {
                            float vbx = m->worldX, vby = m->worldY, vr = 120.0f;
                            SpawnEnemyExplosion(vbx, vby, 1.0f, 0.4f, 0.1f, true);
                            SpawnShockWave(vbx, vby, vr * 1.5f, 0.45f, 1.0f, 0.4f, 0.1f, true);
                            float ppx = playerWin.x + playerWin.width  * 0.5f;
                            float ppy = playerWin.y + playerWin.height * 0.5f;
                            float dpx = ppx - vbx, dpy = ppy - vby;
                            if (dpx*dpx + dpy*dpy < vr*vr) HurtPlayer(g_GameManager.playerHP, 20.0f);
                        }
                        // ?�쇄 ??��(DEATH_BLAST) ???�망 ?�치?�서 주�? ?�에�?AoE
                        //   ?�프: ?��?지 0.8??.3 + ??���?죽�? 몹�? ?�시 ???�짐(무한?�쇄 차단)
                        if (g_Stats.deathBlast && !m->noBlast) {
                            float blastDmg = g_Stats.GetBaseDamage()
                                           * g_Stats.GetDamageMultiplier(0.0f) * g_Stats.deathBlastDmgPct;
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
                            for (auto bm2 : g_MonsterManager.bombers) {
                                if (!bm2->alive) continue;
                                float ddx = bm2->worldX - bx, ddy = bm2->worldY - by;
                                if (ddx*ddx + ddy*ddy < blastR*blastR) {
                                    bm2->hp -= blastDmg;
                                    if (bm2->hp <= 0.0f) bm2->alive = false;
                                }
                            }
                        }
                        SpawnWormSplit(m, mobBorn);
                        SpawnBadSectorZone(m);   // 배드 ?�터 ???�망 ?�리??감속 구역
                    }
                }
                for (auto* nb : mobBorn) g_MonsterManager.monsters.push_back(nb);
                for (auto r : g_MonsterManager.rangedMobs) {
                    if (!r->alive && !r->exploded) {
                        if (!r->scored) {
                            r->scored = true;
                            const long long pickupXp = (long long)(
                                (25.0f + (float)g_Stats.rangedXpBonus) * g_Stats.xpMult);
                            SpawnStardust(r->worldX, r->worldY, 3,
                                          pCX, pCY, pickupXp);
                            creditKill(300.0f);
                        }
                        SpawnEnemyExplosion(r->worldX, r->worldY,
                                            r->color.r, r->color.g, r->color.b,
                                            /*big=*/true);
                        r->exploded = true;
                        SpawnKillTag(r->worldX, r->worldY, 1.0f, 0.55f, 0.9f,
                                     L"popup closed", true);
                    }
                }
                // ReloadRunner boss kill reward
                if (g_RRBoss && !g_RRBoss->alive && !g_RRBoss->exploded) {
                    auto* rb = g_RRBoss;
                    SpawnEnemyExplosion(rb->worldX, rb->worldY, 1.0f, 0.6f, 0.2f, true);
                    SpawnEnemyExplosion(rb->worldX, rb->worldY, 0.3f, 0.9f, 1.0f, true);
                    SpawnShockWave(rb->worldX, rb->worldY, 360.0f, 0.7f, 1.0f, 0.7f, 0.2f);
                    g_ShakeTime = 0.5f; g_ShakeMag = 20.0f;
                    TriggerFlash(1.0f, 0.6f, 0.2f, 0.6f); TriggerHitStop(0.10f);
                    rb->exploded = true;
                    g_GameManager.scoreAccum += 20000.0f;
                    g_GameManager.score = (long long)g_GameManager.scoreAccum;
                    delete rb;
                    g_RRBoss = nullptr;
                    g_TotalBossKills++;
                    TryUnlockAch(ACH_FIRST_BOSS);
                    if (g_TotalBossKills >= 3) TryUnlockAch(ACH_BOSS_3);
                    if (g_TotalBossKills >= 10) TryUnlockAch(ACH_BOSS_10);
                    g_Bullets.clear();
                    OnBossKilled(2, 35);
                }

                
                // FORK.worm ?�망 ??보상
                if (g_CentiBoss && !g_CentiBoss->alive && !g_CentiBoss->exploded) {
                    auto* cb2 = g_CentiBoss;
                    SpawnEnemyExplosion(cb2->worldX, cb2->worldY, 1.0f, 0.8f, 0.2f, true);
                    SpawnEnemyExplosion(cb2->worldX, cb2->worldY, 0.7f, 1.0f, 0.3f, true);
                    SpawnShockWave(cb2->worldX, cb2->worldY, 420.0f, 0.8f, 0.7f, 1.0f, 0.3f);
                    g_ShakeTime = 0.55f; g_ShakeMag = 24.0f;
                    TriggerFlash(0.7f, 1.0f, 0.3f, 0.6f); TriggerHitStop(0.11f);
                    cb2->exploded = true;
                    g_GameManager.scoreAccum += 20000.0f;
                    g_GameManager.score = (long long)g_GameManager.scoreAccum;
                    delete cb2;
                    g_CentiBoss = nullptr;
                    g_TotalBossKills++;
                    TryUnlockAch(ACH_FIRST_BOSS);
                    if (g_TotalBossKills >= 3) TryUnlockAch(ACH_BOSS_3);
                    if (g_TotalBossKills >= 10) TryUnlockAch(ACH_BOSS_10);
                    g_Bullets.clear();
                    OnBossKilled(8, 35);
                }

                if (g_TessBoss && !g_TessBoss->alive && !g_TessBoss->exploded) {
                    auto* tb = g_TessBoss;
                    SpawnEnemyExplosion(tb->worldX, tb->worldY, 0.95f, 0.35f, 1.0f, true);
                    SpawnEnemyExplosion(tb->worldX, tb->worldY, 0.35f, 1.0f, 0.95f, true);
                    SpawnShockWave(tb->worldX, tb->worldY, 440.0f, 0.82f, 0.95f, 0.35f, 1.0f);
                    g_ShakeTime = 0.55f; g_ShakeMag = 24.0f;
                    TriggerFlash(0.95f, 0.35f, 1.0f, 0.62f); TriggerHitStop(0.11f);
                    tb->exploded = true;
                    g_GameManager.scoreAccum += 20000.0f;
                    g_GameManager.score = (long long)g_GameManager.scoreAccum;
                    delete tb;
                    g_TessBoss = nullptr;
                    g_TotalBossKills++;
                    TryUnlockAch(ACH_FIRST_BOSS);
                    if (g_TotalBossKills >= 3) TryUnlockAch(ACH_BOSS_3);
                    if (g_TotalBossKills >= 10) TryUnlockAch(ACH_BOSS_10);
                    g_Bullets.clear();
                    OnBossKilled(10, 35);
                }

                if (g_EtherBoss && !g_EtherBoss->alive && !g_EtherBoss->exploded) {
                    auto* eb = g_EtherBoss;
                    SpawnEnemyExplosion(eb->worldX, eb->worldY, 0.55f, 0.80f, 1.0f, true);
                    SpawnEnemyExplosion(eb->worldX, eb->worldY, 0.80f, 1.0f, 0.55f, true);
                    SpawnShockWave(eb->worldX, eb->worldY, 600.0f, 0.55f, 0.80f, 1.0f, 0.3f);
                    g_ShakeTime = 0.8f; g_ShakeMag = 36.0f;
                    TriggerFlash(0.55f, 0.80f, 1.0f, 0.65f); TriggerHitStop(0.14f);
                    eb->exploded = true;
                    g_GameManager.scoreAccum += 50000.0f;
                    g_GameManager.score = (long long)g_GameManager.scoreAccum;
                    delete eb;
                    g_EtherBoss = nullptr;
                    g_TotalBossKills++;
                    TryUnlockAch(ACH_FIRST_BOSS);
                    if (g_TotalBossKills >= 3)  TryUnlockAch(ACH_BOSS_3);
                    if (g_TotalBossKills >= 10) TryUnlockAch(ACH_BOSS_10);
                    g_Bullets.clear();
                    OnBossKilled(20, 50);
                }




                // ?�리모프 ?�망 ???�면 ?�복 + 증강 3�?+ ?�수 50% 추�?

                // ?�폭�??�망 (?�화 ????�� OR 총알 격파)
                for (auto bm : g_MonsterManager.bombers) {
                    if (!bm->alive && !bm->exploded) {
                        bool blast = bm->arming;
                        // ?�폭(blast)?�?보상 ?�음. ?�레?�어가 죽인 경우(!blast)�?
                        //   그리�??�직 ?�산 ???�으�?광역 처치 ?? 보상 지�?
                        if (!bm->scored && !blast) {
                            bm->scored = true;
                            const long long pickupXp = (long long)(
                                (25.0f + (float)g_Stats.bomberXpBonus) * g_Stats.xpMult);
                            SpawnStardust(bm->worldX, bm->worldY, 3,
                                          pCX, pCY, pickupXp);
                            creditKill(200.0f);
                        }
                        if (blast) {
                            // ?�폭: ?�레?�어 죽음�???�� (?�발 + ?�중 충격??
                            for (int k = 0; k < 5; k++) {
                                SpawnEnemyExplosion(bm->worldX, bm->worldY,
                                                    1.0f, 0.3f + (k%2)*0.3f, 0.1f, true);
                            }
                            SpawnShockWave(bm->worldX, bm->worldY,
                                           bm->blastRadius * 2.0f, 0.55f,
                                           1.0f, 0.45f, 0.1f, /*needsBg=*/true);
                            SpawnShockWave(bm->worldX, bm->worldY,
                                           bm->blastRadius * 1.2f, 0.35f,
                                           1.0f, 0.95f, 0.4f, /*needsBg=*/true);
                        } else {
                            // 총알 격파: ?�범????�� ?�티??+ 종료 ?�그
                            SpawnEnemyExplosion(bm->worldX, bm->worldY,
                                                0.9f, 0.4f, 0.4f, true);
                            SpawnKillTag(bm->worldX, bm->worldY, 1.0f, 0.5f, 0.45f,
                                         L"ransomware purged", true);
                        }
                        // HACK_BOMBER: ?�킹 ??�� VFX (CollisionSystem ?�서 ?�래�??�정??
                        if (bm->hackBlastPending) {
                            SpawnEnemyExplosion(bm->worldX, bm->worldY,
                                                0.3f, 1.0f, 0.4f, true);
                            SpawnShockWave(bm->worldX, bm->worldY,
                                           150.0f, 0.45f, 0.3f, 1.0f, 0.4f);
                            bm->hackBlastPending = false;
                        }
                        bm->exploded = true;
                    }
                }
                // VFX 체크 ?�료 ??죽�? ?�폭�?메모�??�리
                g_MonsterManager.ClearDeadBombers();

                // ?�몹 근접 ?��?지�?HP 감소?�는지 (???�레??비교)
                if (g_GameManager.playerHP < g_PrevHP - 0.0001f) hit = true;
                if (hit && g_Stats.lightStep)
                    g_Stats.lightStepDisableTimer = g_Stats.lightStepHitLock;
                g_PrevHP = g_GameManager.playerHP;

                // ?�벨?? xp 가 ?�요?�을 ?�으�??�벨??(?��? xp ?�월) + AUG_SELECT
                if (g_GameManager.currentState == GameState::RUNNING) {
                    const bool atMainCap = !BossDir::g_ActEndless && !g_CreativeMode
                                          && g_GameManager.playerLevel >= MAIN_LEVEL_CAP;
                    if (atMainCap) {
                        g_GameManager.xp = 0;
                    } else {
                    long long need = g_ExpSystem.Required(g_GameManager.playerLevel);
                    if (g_GameManager.xp >= need) {
                        g_GameManager.xp -= need;
                        ++g_GameManager.playerLevel;
                        // (?�벨???�?�크�??�래???�거 ???��??? AUG_SELECT 카드�?충분???�내)
                        g_GameManager.QueueAugmentReward(true, false);
                    }
                    } // !atMainCap
                }

                // �ð� ���� (�޽ġ����� �߿��� ����)
                if (!InBossRest())
                    g_GameManager.AddScore(FIXED_DT * 100.0f);
            }
            accumulator -= FIXED_DT;
        }

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
            if (g_P2ToastTimer > 0.0f) g_P2ToastTimer -= delta;
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
            {
                bool inZone = false;
                float pcx = playerWin.x + playerWin.width  * 0.5f;
                float pcy = playerWin.y + playerWin.height * 0.5f;
                for (auto& z : g_SlowZones) {
                    float gf = SlowZoneGrow(z);
                    float zx = z.x + z.w*0.5f, zy = z.y + z.h*0.5f;
                    float hw = z.w*0.5f*gf, hh = z.h*0.5f*gf;
                    if (pcx >= zx-hw && pcx <= zx+hw && pcy >= zy-hh && pcy <= zy+hh) { inZone = true; break; }
                }
                if (inZone) g_BadSectorBleed = 2.0f;
                if (g_BadSectorBleed > 0.0f) {
                    g_BadSectorBleed -= delta;
                    if (g_GameManager.playerHP > 1.0f) {
                        g_GameManager.playerHP -= 4.0f * delta;   // 출혈 DPS
                        if (g_GameManager.playerHP < 1.0f) g_GameManager.playerHP = 1.0f;
                    }
                }
                if (g_EtherBoss && g_EtherBoss->alive &&
                    g_EtherBoss->IsBadSector(pcx, pcy)) {
                    HurtPlayer(g_GameManager.playerHP, 3.5f * delta);
                }
            }

            // ?�감 발견 ???�거�??�폭�?(존재?�면 발견 처리)
            if (!g_MonsterManager.rangedMobs.empty()) MarkMobSeenId(CM_RANGED);
            if (!g_MonsterManager.bombers.empty())    MarkMobSeenId(CM_BOMBER);

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
            for (auto& sl : g_Slashes) {
                if (!sl.active) continue;
                sl.life -= delta;
                if (sl.life <= 0.0f) sl.active = false;
            }

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

            // 배드 ?�터 감속 구역 ?�명 + 부???�산
            for (auto& z : g_SlowZones) { z.life -= delta; z.age += delta; }
            g_SlowZones.erase(std::remove_if(g_SlowZones.begin(), g_SlowZones.end(),
                [](const SlowZone& z){ return z.life <= 0.0f; }), g_SlowZones.end());

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
                    g_XpTimeAccum    -= (float)add;
                }
            }

            if (g_Stats.lightStepDisableTimer > 0.0f)
                g_Stats.lightStepDisableTimer -= delta;

            // 취함: drunkCooldown + drunkActiveDuration ?�이??(중복 ?????�라미터 변??
            if (g_Stats.drunk) {
                g_DrunkCycle += delta;
                if (!g_DrunkActive && g_DrunkCycle >= g_Stats.drunkCooldown) {
                    g_DrunkActive = true;
                    g_DrunkCycle  = g_Stats.drunkCooldown;
                }
                if (g_DrunkActive &&
                    g_DrunkCycle >= g_Stats.drunkCooldown + g_Stats.drunkActiveDuration) {
                    g_DrunkActive = false;
                    g_DrunkCycle  = 0.0f;
                }
            }

            const float p2mult = 1.0f;

            // Debug runs need to reach late enemy silhouettes quickly. Keep
            // this test-only acceleration isolated from release balance.
#if defined(_DEBUG)
            constexpr float kEnemySpawnTimeScale = 4.0f;
            constexpr float kEnemySpawnIntervalScale = 0.45f;
#else
            constexpr float kEnemySpawnTimeScale = 1.0f;
            constexpr float kEnemySpawnIntervalScale = 1.0f;
#endif

            // Time-based low-density spawn curve. Early game starts with only a few stronger mobs,
            // then density opens gradually by elapsed run time instead of score.
            BossDir::ActRules act = BossDir::GetActRules();
            float intensity = (float)g_GameManager.score / 100000.0f;
            if (intensity > act.intensityCap) intensity = act.intensityCap;
            float elapsedSec = g_GameTime * kEnemySpawnTimeScale;
            float spawnT = elapsedSec / 300.0f;
            if (spawnT > 1.0f) spawnT = 1.0f;
            if (spawnT < 0.0f) spawnT = 0.0f;
            float lateSpawnT = (elapsedSec - 300.0f) / 300.0f;
            if (lateSpawnT > 1.0f) lateSpawnT = 1.0f;
            if (lateSpawnT < 0.0f) lateSpawnT = 0.0f;
            float capT = elapsedSec / 420.0f;
            if (capT > 1.0f) capT = 1.0f;
            if (capT < 0.0f) capT = 0.0f;
            float rampSpawn = (0.52f + spawnT * 1.75f + lateSpawnT * 0.55f) * act.spawnMult;
            bool  bossNow = g_RRBoss || g_CentiBoss || g_TessBoss || g_EtherBoss || g_BossWarnTimer > 0.0f;
            float hpIntensity = (float)g_GameManager.score / 100000.0f;
            if (hpIntensity > act.hpIntensityCap) hpIntensity = act.hpIntensityCap;
            float hpT = elapsedSec / 300.0f;
            if (hpT > 1.0f) hpT = 1.0f;
            if (hpT < 0.0f) hpT = 0.0f;
            float hpLateT = (elapsedSec - 300.0f) / 300.0f;
            if (hpLateT > 1.0f) hpLateT = 1.0f;
            if (hpLateT < 0.0f) hpLateT = 0.0f;
            float scoreHpRamp = 1.0f + hpIntensity * 0.15f;
            // Keep the time ramp and trial multipliers meaningful, but stop
            // the combined late-game HP from making regular mobs feel spongy.
            constexpr float kRegularEnemyHpScale = 0.80f;
            float rampHp = (1.22f + hpT * 1.05f + hpLateT * 0.70f)
                         * scoreHpRamp * kRegularEnemyHpScale;
            int   varietyPct = (int)(std::min(42.0f * g_Stats.varietyChanceMult,
                                   spawnT * 34.0f * g_Stats.varietyChanceMult)
                               + (float)act.varietyBias);
            varietyPct += TrialVarietyBiasBonus(g_GameManager.score);
            if (varietyPct > 60) varietyPct = 60;

            // ?�폰 ?�역 ??2?�이�?줌아?????�장??보이?? ?�역 모서리에???�폰.
            float saX = 0.0f, saY = 0.0f;
            int   saW = screenWidth, saH = screenHeight;

            // Mob spawn: time ramp plus controlled augment pressure.
            spawnTimer += delta;
            // Keep the opening readable without making the first minute feel empty.
            float spawnInterval = 0.55f * g_Stats.mobSpawnMult
                                / (p2mult * rampSpawn * TrialSpawnRateMult(g_GameManager.score));
            spawnInterval *= kEnemySpawnIntervalScale;
            if (bossNow) spawnInterval *= 2.5f;   // 보스?? ?�래???�폰 ?�??감소
            float effHpMul = TrialEnemyHpMult(g_GameManager.score);
            // 보스 ?�면?????�성 보스가 ?�으�??�연 ?�몹/?�거�??�폭 ?�폰 ?�전 ?��?
            //   (보스가 직접 ?�환?�는 adds ??�?보스 ?�래???��??�서�?
            auto anyBossAlive = [&]() -> bool {
                if (g_RRBoss && g_RRBoss->alive) return true;
                if (g_CentiBoss && g_CentiBoss->alive) return true;
                if (g_TessBoss && g_TessBoss->alive) return true;
                if (g_EtherBoss && g_EtherBoss->alive) return true;
                return false;
            };
            bool bossDuel = anyBossAlive() || g_BossWarnTimer > 0.0f ||
                              g_InBossIntermission;
            if (bossDuel) spawnInterval = 1e9f;
            if (spawnTimer > spawnInterval) {
                float capBonusT = elapsedSec / 360.0f;
                if (capBonusT > 1.0f) capBonusT = 1.0f;
                if (capBonusT < 0.0f) capBonusT = 0.0f;
                int scaledCapBonus = (int)((float)g_Stats.mobCapBonus * capBonusT * capBonusT);
                int effCap = 1 + (int)(4.0f * spawnT + 45.0f * capT * capT + 30.0f * lateSpawnT)
                           + scaledCapBonus;
                if (effCap < 1) effCap = 1;
                if (effCap > 160) effCap = 160;
                // ??마리 ?�폰 + ?�버??변??(D_MOB_PACK ??군집?�로 ?�러 �?
                auto spawnOne = [&]() {
                    size_t mbefore = g_MonsterManager.monsters.size();
                    g_MonsterManager.SpawnMob(screenWidth, screenHeight,
                                              effCap,
                                              g_Stats.monsterHpMult * rampHp * effHpMul, saX, saY, saW, saH,
                                              varietyPct, 0);
                    // ?�버??보유 ???��? 몹을 분열�??�멸체로 (?�수 ?�몹 ????경우�?
                    if (g_MonsterManager.monsters.size() > mbefore) {
                        Monster* nm = g_MonsterManager.monsters.back();
                        if (nm->kind == MobKind::NORMAL) {
                            int gravisCount = 0;
                            for (auto* mm2 : g_MonsterManager.monsters)
                                if (mm2->alive && mm2->kind == MobKind::GRAVIS)
                                    ++gravisCount;
                            const bool gravisReady = elapsedSec >= 150.0f;
                            const int gravisChance = elapsedSec >= 420.0f ? 4 : 2;
                            // QUASAR (Tier 2.5): a rare long-range lane controller.
                            // Keep the simultaneous count low so crossing laser
                            // telegraphs never dominate the arena.
                            int quasarCount = 0;
                            for (auto* mm2 : g_MonsterManager.monsters)
                                if (mm2->alive && mm2->kind == MobKind::QUASAR)
                                    ++quasarCount;
                            const bool quasarReady = elapsedSec >= 45.0f;
                            const int quasarChance = elapsedSec >= 240.0f ? 8 : 5;
                            if (!bossDuel && gravisReady && gravisCount < 1 &&
                                (rand() % 100) < gravisChance) {
                                nm->MakeKind(MobKind::GRAVIS);
                            }
                            else if (!bossDuel && quasarReady && quasarCount < 2 &&
                                (rand() % 100) < quasarChance) {
                                nm->MakeKind(MobKind::QUASAR);
                            }
                            // Hive spawn: available from the first normal spawn.
                            else if ((rand() % 100) < 12) {
                                nm->MakeKind(MobKind::SPAWNER);
                                nm->blinkTargetX = 160.0f + (float)(rand() % (screenWidth  > 360 ? screenWidth  - 320 : 1));
                                nm->blinkTargetY = 160.0f + (float)(rand() % (screenHeight > 360 ? screenHeight - 320 : 1));
                            }
                            // DDoS swarm: available from the first normal spawn;
                            // its probability and cap still ramp with run time.
                            else {
                                const float ddosT = std::min(1.0f, std::max(0.0f, elapsedSec / 300.0f));
                                const int ddosChance = 4 + (int)(4.0f * ddosT);
                                if (!bossDuel && (rand() % 100) < ddosChance) {
                                    int ddosCount = 0;
                                    for (auto* mm2 : g_MonsterManager.monsters)
                                        if (mm2->alive && mm2->kind == MobKind::DDOS) ++ddosCount;
                                    float capMul = 0.10f + 0.08f * ddosT;
                                    int ddosCap = (int)((float)effCap * capMul);
                                    if (ddosCap < 2) ddosCap = 2;
                                    if (ddosCount < ddosCap) {
                                        nm->MakeKind(MobKind::DDOS);
                                    }
                                }
                            }
                        }
                        if (nm->kind != MobKind::NORMAL && g_Stats.specialMobHpMult != 1.0f)
                            nm->hp *= g_Stats.specialMobHpMult;
                    }
                };
                float packBonusT = (elapsedSec - 120.0f) / 360.0f;
                if (packBonusT > 1.0f) packBonusT = 1.0f;
                if (packBonusT < 0.0f) packBonusT = 0.0f;
                int packN = 2 + (int)((float)g_Stats.mobPackBonus * packBonusT);
                if (elapsedSec >= 120.0f) ++packN;
                if (elapsedSec >= 300.0f) ++packN;
                if (elapsedSec >= 480.0f) ++packN;
                if (packN > 6) packN = 6;
                for (int p = 0; p < packN && (int)g_MonsterManager.monsters.size() < effCap; p++)
                    spawnOne();
                spawnTimer = 0.0f;
            }

            g_GameTime += delta;

            if (!g_CreativeMode && g_GameManager.currentState == GameState::RUNNING) {
            }

            // ���� ���� ? �Ϲ�: �� ���?/ ũ������Ƽ��: ���� ���?
            {
                bool bossActive = g_RRBoss || g_CentiBoss || g_TessBoss || g_EtherBoss || g_BossWarnTimer > 0.0f;
                // ?�운?? ??보스 ?�덩??차단: 보스�??�아 ?�전???�리?�는 ?�간(?�성?�비?�성),
                //   ?�음 보스 ?�계값을 ?�재 ?�수+20만으�?리베?�스 ??최소 20만점 ?�식 보장
                //   (보스??�??�인 ?�수�??�자마자 ??보스 ?�던 ?�순???�거).
                static bool s_prevBossActive = false;
                if (s_prevBossActive && !bossActive) {
                    long long rebase = (long long)g_GameManager.score + 200000;
                    if (g_NextBossScore < rebase) g_NextBossScore = rebase;
                }
                s_prevBossActive = bossActive;
                float bossHpC = GetDifficultyParams(Difficulty::NORMAL).bossHp * TrialBossHpMult();
                float polyHpC = 30000.0f * TrialBossHpMult();

                // ���� ���� ? pick���̸���HP Ȯ�� �� ª�� ���?ũ������Ƽ�� 0.35s)
                auto startWarn = [&](int pick, const wchar_t* name, float hp) {
                    StartBossWarn(pick, name, hp);
                };

                if (!bossActive) {
                    if (g_CreativeBossPending) {
                        g_CreativeBossPending = false;
                        QueueCreativeBossPick(g_CreativeBossPick >= 0 ? g_CreativeBossPick : 2,
                                              bossHpC, polyHpC);
                    }
                    // ���� ���? ���� ���� ���� (���� ���?
                    else if (!g_CreativeMode && !BossDir::g_ActEndless
                             && BossDir::g_ActClears < BossDir::MAIN_ACT_TOTAL
                             && !g_InBossIntermission
                             && g_GameManager.score >= g_NextBossScore) {
                        g_NextBossScore += (long long)BossDir::MAIN_SCORE_STEP;
                        int pick = BossDir::MAIN_SEQUENCE[BossDir::g_ActClears];
                        float sc  = 1.0f + (float)BossDir::g_ActClears * 0.9f;
                        float bossHp = bossHpC * sc * BossDir::HpMul(pick);
                        startWarn(pick, BossDir::DisplayName(pick), bossHp);
                    }
                    // ���� ���? Ŭ���� ���� ? ������ ���� óġ �� ���͹̼� ���� ��
                    else if (!g_CreativeMode && !BossDir::g_ActEndless
                             && BossDir::g_ActClears >= BossDir::MAIN_ACT_TOTAL
                             && !g_InBossIntermission) {
                        TriggerVictory();
                    }
                    else if (g_CreativeMode && g_GameManager.score >= g_NextBossScore) {
                        g_NextBossScore += 200000;
                        float sc = 0.36f + (float)g_GameManager.score / 300000.0f;
                        if (sc > 9.0f) sc = 9.0f;
                        sc *= (1.0f + (float)g_GameManager.playerLevel * 0.022f);
                        float bossHp = GetDifficultyParams(Difficulty::NORMAL).bossHp * TrialBossHpMult() * sc;
                        {
                            int pick = BossDir::RollScorePick();
                            startWarn(pick, BossDir::DisplayName(pick),
                                      bossHp * BossDir::HpMul(pick));
                        }
                    }
                }

                // ?�조 진행 ??만료 ???�제 보스 ?�성 + ?�장 ?�출
                if (g_BossWarnTimer > 0.0f) {
                    g_BossWarnTimer -= delta;
                    if (g_BossWarnTimer <= 0.0f) {
                        g_BossWarnTimer = 0.0f;
                        // C15: ?�환 ?�치 ?�덤??????�� 중앙 근처??거기??캠핑/?�건 ?�훼?�던 문제.
                        //   ?�면 ?�쪽(가?�자�?마진 ?�외) ???�역?�서 무작???�치.
                        float bMargin = 340.0f * g_Scale;
                        float bRangeX = std::max(1.0f, (float)screenWidth  - 2.0f * bMargin);
                        float bRangeY = std::max(1.0f, (float)screenHeight - 2.0f * bMargin);
                        float bsx = bMargin + (float)(rand() % (int)bRangeX);
                        float bsy = bMargin + (float)(rand() % (int)bRangeY);
                        switch (g_BossWarnPick) {
                        case 2:
                            g_RRBoss = new ReloadRunnerBoss(screenWidth, screenHeight, g_BossWarnHp);
                            g_RRBoss->worldX = bsx; g_RRBoss->worldY = bsy;
                            break;
                        case 8:
                            g_CentiBoss = new CentipedeBoss(screenWidth, screenHeight, g_BossWarnHp);
                            g_CentiBoss->worldX = bsx; g_CentiBoss->worldY = bsy;
                            // ?�?�??�로?��??? ?�재 ?�몹 ?�체 체력???�수(?�한) ???�수 보스???�?�?
                            //   ?�몹?�?보스�??�수?�어 ?�라지�? 보스???�안 추�? ?�폰 ????
                            {
                                float absorb = 0.0f;
                                for (auto* m  : g_MonsterManager.monsters)   if (m->alive)  absorb += m->hp;
                                for (auto* r  : g_MonsterManager.rangedMobs)  if (r->alive)  absorb += r->hp;
                                for (auto* bm : g_MonsterManager.bombers)     if (bm->alive) absorb += bm->hp;
                                // ?�한 ?�거 ??진짜 '?�몹 ?�체 체력 + 보스 체력'. ?�반 ?�커 ?�몹
                                //   horde 가 많을?�록 보스??그만???�단(?�몹보다 빨리 죽는 문제 ?�결).
                                g_CentiBoss->hp += absorb; g_CentiBoss->maxHp += absorb;
                                // ?�몹 ?�수 ???�면 ?�리(?�수 ?�??
                                for (auto* m  : g_MonsterManager.monsters)   delete m;
                                g_MonsterManager.monsters.clear();
                                for (auto* r  : g_MonsterManager.rangedMobs)  delete r;
                                g_MonsterManager.rangedMobs.clear();
                                for (auto* bm : g_MonsterManager.bombers)     delete bm;
                                g_MonsterManager.bombers.clear();
                                g_ShakeTime = 0.4f; g_ShakeMag = 14.0f;   // ?�수 ?�간 진동
                            }
                            g_CentiBoss->enterSpawn();   // ?�장 모션 ???�면 밖에??곡선 ?�진?�로 ?�장
                            break;
                        case 10:
                            g_TessBoss = new TesseractGlitchBoss(screenWidth, screenHeight, g_BossWarnHp);
                            g_TessBoss->worldX = bsx; g_TessBoss->worldY = bsy;
                            break;
                        case 20:
                            g_EtherBoss = new EtherSwordBoss(screenWidth, screenHeight, g_BossWarnHp);
                            // EtherSword spawns at screen center (arena center)
                            g_EtherBoss->worldX = screenWidth  * 0.5f;
                            g_EtherBoss->worldY = screenHeight * 0.32f;
                            break;
                        default:
                            g_RRBoss = new ReloadRunnerBoss(screenWidth, screenHeight, g_BossWarnHp);
                            g_RRBoss->worldX = bsx; g_RRBoss->worldY = bsy;
                            break;
                        }
                        if (g_BossWarnPick >= 0)
                            MarkBossSeenPick(g_BossWarnPick);
                        // ���� ���� ����
                        g_ShakeTime = 0.6f; g_ShakeMag = 28.0f;
                        if (!g_CentiBoss) {
                            SpawnShockWave(bsx, bsy, 500.0f, 0.9f, 1.0f, 0.3f, 0.3f);
                            SpawnShockWave(bsx, bsy, 320.0f, 0.7f, 1.0f, 0.8f, 0.2f);
                            for (int k = 0; k < 3; k++) {
                                SpawnEnemyExplosion(bsx + (rand()%200 - 100),
                                                    bsy + (rand()%200 - 100),
                                                    1.0f, 0.3f, 0.3f, true);
                            }
                        }
                    }
                }
            }

            DifficultyParams dp = GetDifficultyParams(Difficulty::NORMAL);

            // ?�거�?�??�폰 ???�이?�별 + ?�버??(D_RMOB_MAX, rmobSpawnDelayBonus)
            rangedSpawnTimer += delta;
            float rangedInterval = dp.rangedSpawnInterval * TrialRangedIntervalMult(g_GameManager.score)
                                  - g_Stats.rmobSpawnDelayBonus;
            if (rangedInterval < 1.0f) rangedInterval = 1.0f;
            rangedInterval /= (p2mult * rampSpawn);
            rangedInterval *= kEnemySpawnIntervalScale;
            rangedInterval *= 2.5f;   // Lens ���� �� ����
            if (bossNow) rangedInterval *= 2.0f;
            int rangedMax = (int)((dp.rangedMaxBase + g_Stats.rmobMaxBonus + TrialRangedMaxBonus(g_GameManager.score)) * p2mult
                                  + intensity * 2.0f);           // ?�수???�시 +2
            if (rangedMax > 16) rangedMax = 16;   // �?개수 = scissor ?�스 ?????�한 (?�능)
            if (rangedSpawnTimer > rangedInterval && !bossDuel) {
                g_MonsterManager.SpawnRangedMob(screenWidth, screenHeight,
                    g_Stats.rmobHpMult * rampHp, rangedMax, saX, saY, saW, saH);
                rangedSpawnTimer = 0.0f;
            }

            // ?��??�는 죽음: 모든 ?�브 ?�원??추격 + ?�촉 ?��?지
            // ?�택 1??= 기본 ?�도, 2???�상부??+20%/?�택
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
            if (g_Stats.drone && !g_Stats.turretMode) {
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
                        if (findNearestEnemy(droneX, droneY, tx, ty)) {
                            Bullet nb(droneX, droneY, tx, ty);
                            nb.speed = g_Stats.bulletSpeed * 0.5f;
                            nb.color = glm::vec3(0.2f, 0.9f, 1.0f);
                            g_Bullets.push_back(nb);
                        }
                        dr.fireTimer = 0.0f;
                    }
                }
            }

            // ?�탑 배치 (CANNON + DRONE_2 조합)
            //   1초마???�레?�어 ?�치???�탑 1�?배치, �?5�?지????맵에 ~5�??�시
            if (g_Stats.turretMode) {
                // ?�총 기�? ?�력�??�계??(보유 증강 개수 변???�만)
                static size_t s_lastTurretAugCount = (size_t)-1;
                if (g_OwnedAugs.size() != s_lastTurretAugCount) {
                    s_lastTurretAugCount = g_OwnedAugs.size();
                    g_TurretStats = PlayerStats();
                    ApplyWeapon(g_TurretStats, StartWeapon::RIFLE);
                    for (int oi : g_OwnedAugs) g_TurretStats.Apply(ALL_AUGS[oi].type);
                }

                // 1초마?????�탑 배치 (?�레?�어 ?�치)
                g_TurretDeployTimer += delta;
                if (g_TurretDeployTimer >= TURRET_DEPLOY) {
                    g_TurretDeployTimer -= TURRET_DEPLOY;
                    if ((int)g_Turrets.size() < MAX_TURRETS) {
                        Turret t; t.x = pCX; t.y = pCY;
                        g_Turrets.push_back(t);
                    }
                }

                // �??�탑: ?�명 + ?�총 발사
                float tInterval = g_TurretStats.fireInterval * g_TurretStats.GetFireIntervalMult();
                float tSpeed    = g_TurretStats.bulletSpeed   + g_TurretStats.GetBulletSpeedBonus();
                for (auto& t : g_Turrets) {
                    t.lifeTimer += delta;
                    t.fireTimer += delta;
                    if (t.fireTimer >= tInterval) {
                        t.fireTimer = 0.0f;
                        float nd2 = 1e9f, ttx = 0.0f, tty = 0.0f;
                        for (auto m : g_MonsterManager.monsters) {
                            if (!m->alive) continue;
                            float ddx = m->worldX - t.x, ddy = m->worldY - t.y;
                            float ds  = ddx*ddx + ddy*ddy;
                            if (ds < nd2) { nd2 = ds; ttx = m->worldX; tty = m->worldY; }
                        }
                        for (auto r2 : g_MonsterManager.rangedMobs) {
                            if (!r2->alive) continue;
                            float ddx = r2->worldX - t.x, ddy = r2->worldY - t.y;
                            float ds  = ddx*ddx + ddy*ddy;
                            if (ds < nd2) { nd2 = ds; ttx = r2->worldX; tty = r2->worldY; }
                        }
                        for (auto bm2 : g_MonsterManager.bombers) {
                            if (!bm2->alive) continue;
                            float ddx = bm2->worldX - t.x, ddy = bm2->worldY - t.y;
                            float ds  = ddx*ddx + ddy*ddy;
                            if (ds < nd2) { nd2 = ds; ttx = bm2->worldX; tty = bm2->worldY; }
                        }
                        if (nd2 < 1e8f) {
                            float dist = sqrtf(nd2);
                            Bullet nb(t.x, t.y, ttx, tty);
                            nb.speed     = tSpeed;
                            nb.color     = glm::vec3(0.1f, 1.0f, 0.55f);  // �?�� (?�탑 고유??
                            nb.turretDmg = g_TurretStats.GetBaseDamage()
                                         * g_TurretStats.GetDamageMultiplier(dist);
                            if (nb.turretDmg < 1.0f) nb.turretDmg = 1.0f;
                            g_Bullets.push_back(nb);
                        }
                    }
                }
                // 만료???�탑 ?�거
                g_Turrets.erase(
                    std::remove_if(g_Turrets.begin(), g_Turrets.end(),
                        [](const Turret& t){ return t.lifeTimer >= TURRET_LIFE; }),
                    g_Turrets.end());
            }

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
                        for (auto bm : g_MonsterManager.bombers) {
                            if (!bm->alive) continue;
                            float ddx = bm->worldX - chx, ddy = bm->worldY - chy;
                            if (ddx*ddx + ddy*ddy < hitR2) { bm->hp = 0.0f; bm->alive = false; ch.hp -= 3.0f; }
                        }
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

            // 고장??조�????�브 배회
            if (g_Stats.brokenSight && g_Orb.active) {
                g_Orb.wanderTimer -= delta;
                if (g_Orb.wanderTimer <= 0.0f) {
                    float angle = (float)(rand() % 628) * 0.01f;
                    float spd   = 100.0f + (float)(rand() % 150);
                    g_Orb.vx = cosf(angle) * spd;
                    g_Orb.vy = sinf(angle) * spd;
                    g_Orb.wanderTimer = 0.5f + (float)(rand() % 100) * 0.015f;
                }
                g_Orb.x += g_Orb.vx * delta;
                g_Orb.y += g_Orb.vy * delta;
                if (g_Orb.x < 20)                    { g_Orb.vx =  fabsf(g_Orb.vx); g_Orb.x = 20; }
                else if (g_Orb.x > screenWidth  - 20){ g_Orb.vx = -fabsf(g_Orb.vx); g_Orb.x = (float)screenWidth  - 20; }
                if (g_Orb.y < 20)                    { g_Orb.vy =  fabsf(g_Orb.vy); g_Orb.y = 20; }
                else if (g_Orb.y > screenHeight - 20){ g_Orb.vy = -fabsf(g_Orb.vy); g_Orb.y = (float)screenHeight - 20; }
            }

            // 총알 발사 (?�혼?�확/미니??보너?? Twin 2�? Cannon ?�존 ?��?지 캐싱)
            // ?�탑 모드: ?�레?�어 ?�동 발사 비활??(?�탑???�??발사)
            float effInterval = g_Stats.fireInterval * g_Stats.GetFireIntervalMult();
            // ?�?? ?�사 %??공격?�으로만 ?�산?�고, ?�제 발사??1�?고정
            if (g_Stats.cannon) effInterval = 1.0f;
            // 과�??????�사 ×2 (발사 간격 ?�반)
            float effSpeed    = g_Stats.bulletSpeed   + g_Stats.GetBulletSpeedBonus();
            if (!g_Stats.turretMode) fireTimer += delta;
            if (g_Stats.minigunHitBoost > 0.0f) {
                fireTimer += g_Stats.minigunHitBoost;
                g_Stats.minigunHitBoost = 0.0f;
            }

            // ??�?spawn ?�퍼 (Twin / Cannon / 취함 / brokenSight 공통)
            // bulletSpread > 0 �?발사 방향???�덤 ?�들�??�용
            auto spawnOne = [&](float tx, float ty) {
                float ftx = tx, fty = ty;
                if (g_Stats.bulletSpread > 0.0f) {
                    float dx = tx - pCX, dy = ty - pCY;
                    float ang = atan2f(dy, dx);
                    float jitter = ((float)(rand() % 200) - 100.0f) / 100.0f
                                  * g_Stats.bulletSpread;
                    ang += jitter;
                    float r = 200.0f;
                    ftx = pCX + cosf(ang) * r;
                    fty = pCY + sinf(ang) * r;
                }
                Bullet nb(pCX, pCY, ftx, fty);
                nb.speed = effSpeed;
                if (g_Stats.cannon) {
                    nb.remainingDmg = g_Stats.GetBaseDamage()
                                    * g_Stats.GetDamageMultiplier(0.0f);
                    nb.sizeScale    = 3.0f;
                }
                // ?�쇄 ?�용(리코?? ???��? ?�수 + ?��?지 배율(-30%) ?�용
                if (g_Stats.ricochetMax > 0) {
                    nb.bouncesLeft = g_Stats.ricochetMax;
                    nb.dmgMult    *= g_Stats.ricochetDmgMult;
                }
                if (g_Stats.berserk) {             // ������: ü�� �������� �ִ� +60%
                    float hpFrac = g_Stats.maxHP > 0.0f
                                 ? g_GameManager.playerHP / g_Stats.maxHP : 1.0f;
                    if (hpFrac < 0.0f) hpFrac = 0.0f; else if (hpFrac > 1.0f) hpFrac = 1.0f;
                    float bMult = 1.0f + (1.0f - hpFrac) * 0.6f;
                    nb.dmgMult *= bMult;
                    if (nb.remainingDmg > 0.0f) nb.remainingDmg *= bMult;
                }
                if (g_Stats.bowWeapon) {           // 궁수 ???�살 비주??(길쭉·갈색??
                    nb.color     = glm::vec3(0.75f, 0.95f, 0.45f);
                    nb.sizeScale = 1.7f;
                }
                if (g_DrunkActive) {               // 취함 ?�성 ??조�? ?�트?�짐 + ?��?지 -40%
                    nb.dmgMult *= 0.6f;
                    if (nb.remainingDmg > 0.0f) nb.remainingDmg *= 0.6f;
                }
                if (g_FocusShotsLeft > 0) {
                    --g_FocusShotsLeft;
                    nb.dmgMult *= 2.5f;
                    if (nb.remainingDmg > 0.0f) nb.remainingDmg *= 2.5f;
                    nb.pierceBonusPct = 30;
                }
                if (g_DashBoostShotsLeft > 0) {
                    --g_DashBoostShotsLeft;
                    nb.dmgMult *= 2.0f;
                    if (nb.remainingDmg > 0.0f) nb.remainingDmg *= 2.0f;
                }
                if (g_Stats.revolverOverload && g_Stats.revolver) {
                    if (g_RevolverRound == 5) {
                        if (g_Stats.revolverSilver) {
                            nb.silverBurn = true;
                            nb.color = glm::vec3(0.85f, 0.92f, 1.0f);
                        } else {
                            nb.dmgMult *= g_Stats.critMult;
                        }
                    }
                    g_RevolverRound = (g_RevolverRound + 1) % 6;
                }
                g_Bullets.push_back(nb);
            };

            // Twin / Shotgun
            auto spawnAimed = [&](float tx, float ty) {
                TriggerMuzzle(pCX, pCY, atan2f(ty - pCY, tx - pCX));
                Audio::PlaySfx(Audio::Sfx::Shoot);
                if (g_Stats.shotgun) {
                    float dx = tx - pCX, dy = ty - pCY;
                    float ang = atan2f(dy, dx);
                    const int N = g_Stats.shotgunSpread ? 7 : 5;
                    float spread = 0.42f;
                    float r = 200.0f;
                    float maxR = g_Stats.shotgunSpread ? 630.0f : 700.0f;
                    for (int s = 0; s < N; s++) {
                        float t = (N > 1) ? (float)s / (float)(N - 1) : 0.5f;
                        float off = (t - 0.5f) * spread;
                        float a = ang + off;
                        Bullet nb(pCX, pCY,
                                  pCX + cosf(a) * r, pCY + sinf(a) * r);
                        nb.speed    = effSpeed;
                        nb.maxRange = maxR;
                        if (g_Stats.cannon) {
                            nb.remainingDmg = g_Stats.GetBaseDamage()
                                            * g_Stats.GetDamageMultiplier(0.0f);
                            nb.sizeScale    = 5.0f;
                        }
                        if (g_Stats.ricochetMax > 0) {
                            nb.bouncesLeft = g_Stats.ricochetMax;
                            nb.dmgMult    *= g_Stats.ricochetDmgMult;
                        }
                        if (g_Stats.berserk) {             // ������
                            float hpFrac = g_Stats.maxHP > 0.0f
                                         ? g_GameManager.playerHP / g_Stats.maxHP : 1.0f;
                            if (hpFrac < 0.0f) hpFrac = 0.0f; else if (hpFrac > 1.0f) hpFrac = 1.0f;
                            float bMult = 1.0f + (1.0f - hpFrac) * 0.6f;
                            nb.dmgMult *= bMult;
                            if (nb.remainingDmg > 0.0f) nb.remainingDmg *= bMult;
                        }
                        if (g_Stats.bowWeapon) {
                            nb.color     = glm::vec3(0.75f, 0.95f, 0.45f);
                            nb.sizeScale = 1.7f;
                        }
                        if (g_DrunkActive) {               // 취함 ?�성 ???��?지 -40%
                            nb.dmgMult *= 0.6f;
                            if (nb.remainingDmg > 0.0f) nb.remainingDmg *= 0.6f;
                        }
                        g_Bullets.push_back(nb);
                    }
                } else if (g_Stats.twin) {
                    float dx = tx - pCX, dy = ty - pCY;
                    float ang = atan2f(dy, dx);
                    float off = 0.087f;
                    float r = 200.0f;
                    int   n = (g_Stats.twinCount < 2) ? 2 : g_Stats.twinCount;  // 2�??�블)/3�??�리??
                    for (int s = 0; s < n; s++) {
                        // 중심 기�? ?��?부채꼴 (-off ??+off)
                        float a = (n > 1) ? ang + (((float)s / (float)(n - 1)) - 0.5f) * (2.0f * off)
                                          : ang;
                        spawnOne(pCX + cosf(a) * r, pCY + sinf(a) * r);
                    }
                } else {
                    spawnOne(tx, ty);
                }
            };

            // 검�?근접 ?�윙 ??조�? 방향 ??arc) ?�의 모든 ?�에�?즉시 ?�해
            auto meleeSwing = [&](float ang) {
                float range = 190.0f * g_Stats.playerSizeMult * (g_Stats.meleeWide ? 1.25f : 1.0f);
                float r2 = range * range;
                float halfArc = g_Stats.meleeWide ? 1.5f : 1.15f;  // 광폭 베기: ???��?
                bool crit = false; float critMult = 1.0f;
                if (g_Stats.critChance > 0 && (rand() % 100) < g_Stats.critChance) {
                    crit = true; critMult = g_Stats.critMult;
                }
                float bMult = 1.0f;
                if (g_Stats.berserk) {
                    float hf = g_Stats.maxHP > 0.0f
                             ? g_GameManager.playerHP / g_Stats.maxHP : 1.0f;
                    if (hf < 0.0f) hf = 0.0f; else if (hf > 1.0f) hf = 1.0f;
                    bMult = 1.0f + (1.0f - hf) * 0.6f;
                }
                float dmg = g_Stats.GetBaseDamage() * g_Stats.GetDamageMultiplier(0.0f)
                          * 1.8f * critMult * bMult;       // 근접 보너??×1.8
                auto inCone = [&](float ex, float ey) -> bool {
                    float dx = ex - pCX, dy = ey - pCY, d2 = dx*dx + dy*dy;
                    if (d2 > r2) return false;
                    float diff = atan2f(dy, dx) - ang;
                    while (diff >  3.14159265f) diff -= 6.2831853f;
                    while (diff < -3.14159265f) diff += 6.2831853f;
                    return fabsf(diff) <= halfArc;
                };
                auto onKill = [&]() {   // ?�혈???�혈�?공통 처리
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
                std::vector<Monster*> swingBorn;
                for (auto m : g_MonsterManager.monsters) {        // ?�몹
                    if (!m->alive || !inCone(m->worldX, m->worldY)) continue;
                    float dmgM = dmg;
                    if (m->kind == MobKind::SHIELDED && m->shieldActive) dmgM *= 0.15f;
                    float dealt = (dmgM < m->hp) ? dmgM : m->hp; m->hp -= dealt;
                    SpawnDamageNumber(m->worldX, m->worldY, dealt, dealt >= 40.0f || crit);
                    if (m->hp <= 0.0f) {
                        m->alive = false; m->scored = true; AddKillCombo();
                        float bx, bs; MobKillReward(m->kind, m->splitGen, m->elite, bx, bs,
                                                      g_Stats.splitterBoost);
                        TryHackFirewallOnKill(m->kind, g_Stats);
                        float rwm = MobRewardMult(m->kind); bx *= rwm; bs *= rwm;
                        const long long pickupXp = (long long)(
                            (bx + MobDebuffXpBonus(m->kind, m->elite, g_Stats)) * g_Stats.xpMult);
                        SpawnStardust(m->worldX, m->worldY,
                                      StardustRewardFor(m->kind), pCX, pCY, pickupXp);
                        g_Stats.killCount++; g_GameManager.scoreAccum += bs;
                        g_GameManager.score = (long long)g_GameManager.scoreAccum;
                        SpawnWormSplit(m, swingBorn);
                        SpawnBadSectorZone(m);
                        onKill();
                    }
                }
                for (auto* nb : swingBorn) g_MonsterManager.monsters.push_back(nb);
                for (auto rr : g_MonsterManager.rangedMobs) {
                    if (!rr->alive || !inCone(rr->worldX, rr->worldY)) continue;
                    float dealt = (dmg < rr->hp) ? dmg : rr->hp; rr->hp -= dealt;
                    SpawnDamageNumber(rr->worldX, rr->worldY, dealt, dealt >= 40.0f || crit);
                    if (rr->hp <= 0.0f) {
                        rr->alive = false; rr->scored = true; AddKillCombo();
                        const long long pickupXp = (long long)(
                            (25.0f + (float)g_Stats.rangedXpBonus) * g_Stats.xpMult);
                        SpawnStardust(rr->worldX, rr->worldY, 3,
                                      pCX, pCY, pickupXp);
                        g_Stats.killCount++; g_GameManager.scoreAccum += 300.0f;
                        g_GameManager.score = (long long)g_GameManager.scoreAccum;
                        onKill();
                    }
                }
                for (auto bm : g_MonsterManager.bombers) {
                    if (!bm->alive || !inCone(bm->worldX, bm->worldY)) continue;
                    float dealt = (dmg < bm->hp) ? dmg : bm->hp; bm->hp -= dealt;
                    SpawnDamageNumber(bm->worldX, bm->worldY, dealt, dealt >= 40.0f || crit);
                    if (bm->hp <= 0.0f) {
                        bm->alive = false; bm->scored = true; AddKillCombo();
                        const long long pickupXp = (long long)(
                            (25.0f + (float)g_Stats.bomberXpBonus) * g_Stats.xpMult);
                        SpawnStardust(bm->worldX, bm->worldY, 3,
                                      pCX, pCY, pickupXp);
                        g_Stats.killCount++; g_GameManager.scoreAccum += 200.0f;
                        g_GameManager.score = (long long)g_GameManager.scoreAccum;
                        onKill();
                    }
                }
                // 보스�???hp�?감소 (보상/?�출?�?�??�망 블록???�당)
                auto hitB = [&](float ex, float ey, float& hp, bool& al) {
                    if (!inCone(ex, ey)) return;
                    float dealt = (dmg < hp) ? dmg : hp; hp -= dealt;
                    SpawnDamageNumber(ex, ey, dealt, dealt >= 40.0f || crit);
                    ApplyBossLifestealFromDamage(dealt);
                    if (hp <= 0.0f) al = false;
                };
                if (g_RRBoss && g_RRBoss->alive)
                    hitB(g_RRBoss->worldX, g_RRBoss->worldY, g_RRBoss->hp, g_RRBoss->alive);
                if (g_CentiBoss && g_CentiBoss->alive && g_CentiBoss->vulnerable())
                    hitB(g_CentiBoss->worldX, g_CentiBoss->worldY, g_CentiBoss->hp, g_CentiBoss->alive);
                if (g_TessBoss && g_TessBoss->alive)
                    hitB(g_TessBoss->worldX, g_TessBoss->worldY, g_TessBoss->hp, g_TessBoss->alive);
                if (g_EtherBoss && g_EtherBoss->alive && inCone(g_EtherBoss->worldX, g_EtherBoss->worldY))
                    HitEtherBossDirect(dmg, pCX, pCY, crit);
                SpawnSlash(pCX, pCY, ang, range);
                TriggerHitStop(0.015f);
                // 칼바?????�윙마다 ?�방?�로 관???�사�?(근접???�거�?견제)
                if (g_Stats.bladeWind) {
                    Bullet bw(pCX, pCY, pCX + cosf(ang)*200.0f, pCY + sinf(ang)*200.0f);
                    bw.speed       = 900.0f;
                    bw.maxRange    = 700.0f;
                    bw.sizeScale   = 2.2f;
                    bw.color       = glm::vec3(0.7f, 0.95f, 1.0f);
                    bw.remainingDmg = dmg * 0.6f;   // 관???�?�식) ??근접 ?��?지??60%
                    g_Bullets.push_back(bw);
                }
            };

            // ?�?�?조�? ?��??�퍼: 좌클�?커서 ?�점?? ?�동(?�릭X)=최근???? ?�동?�데 ???�으�?false.
            auto aimTarget = [&](float& tx, float& ty) -> bool {
                if (lmb) { tx = wmx; ty = wmy; return true; }
                return findNearestEnemy(pCX, pCY, tx, ty);
            };

            // ?�?�??�캔 ?�이?�?(증강) ??0.7초마??조�? 방향 관??�?(군중?�어) ?�?�?
            //   meleeSwing ???��?지/처치보상 루프�?'직선 ?�정(SegDist)' 버전?�로 ?�사??
            if (g_Stats.laser) {
                float laserInt = (g_Stats.laserTier >= 3) ? 0.18f   // ?�화 ?�렴: 거의 ?�속
                               : (g_Stats.laserTier >= 2) ? 0.55f : LASER_INT;
                g_LaserTimer += delta;
                if (g_LaserTimer >= laserInt) {
                    g_LaserTimer -= laserInt;
                    float lang  = atan2f(wmy - pCY, wmx - pCX);   // ?�이?�????�� 커서 방향
                    // ?�거리는 II(760)?�서 ???�리지 ?�음. ?�화 ?�렴?�?'?�비'�?강화.
                    float LASER_RANGE = (g_Stats.laserTier >= 2) ? 760.0f : 560.0f;
                    float lex = pCX + cosf(lang) * LASER_RANGE, ley = pCY + sinf(lang) * LASER_RANGE;
                    // ?�화 ?�렴(tier3): �??�비 2�???광폭 관??(?�몹 ?�인 ?�소)
                    const float beamW    = (g_Stats.laserTier >= 3) ? 2.0f : 1.0f;
                    const float BEAM_HALF = 24.0f * beamW;
                    bool lcrit = false; float lcm = 1.0f;
                    if (g_Stats.critChance > 0 && (rand()%100) < g_Stats.critChance) { lcrit = true; lcm = g_Stats.critMult; }
                    float lbm = 1.0f;
                    if (g_Stats.berserk) {
                        float hf = g_Stats.maxHP > 0.0f ? g_GameManager.playerHP / g_Stats.maxHP : 1.0f;
                        if (hf < 0.0f) hf = 0.0f; else if (hf > 1.0f) hf = 1.0f;
                        lbm = 1.0f + (1.0f - hf) * 0.6f;
                    }
                    float laserDmgMult = (g_Stats.laserTier >= 3) ? 1.2f : 1.4f;
                    float ldmg = g_Stats.GetBaseDamage() * g_Stats.GetDamageMultiplier(0.0f)
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
                    std::vector<Monster*> laserBorn;
                    for (auto m : g_MonsterManager.monsters) {
                        if (!m->alive || !inLine(m->worldX, m->worldY)) continue;
                        float d = ldmg;
                        if (m->kind == MobKind::SHIELDED && m->shieldActive) d *= 0.15f;
                        float dealt = (d < m->hp) ? d : m->hp; m->hp -= dealt;
                        SpawnDamageNumber(m->worldX, m->worldY, dealt, dealt >= 40.0f || lcrit);
                        if (m->hp <= 0.0f) {
                            m->alive = false; m->scored = true; AddKillCombo();
                            float bx, bs; MobKillReward(m->kind, m->splitGen, m->elite, bx, bs,
                                                      g_Stats.splitterBoost);
                            TryHackFirewallOnKill(m->kind, g_Stats);
                            float rwm = MobRewardMult(m->kind); bx *= rwm; bs *= rwm;
                            const long long pickupXp = (long long)(
                                (bx + MobDebuffXpBonus(m->kind, m->elite, g_Stats)) * g_Stats.xpMult);
                            SpawnStardust(m->worldX, m->worldY,
                                          StardustRewardFor(m->kind), pCX, pCY, pickupXp);
                            g_Stats.killCount++; g_GameManager.scoreAccum += bs;
                            g_GameManager.score = (long long)g_GameManager.scoreAccum;
                            SpawnWormSplit(m, laserBorn); SpawnBadSectorZone(m); lOnKill();
                        }
                    }
                    for (auto* nb : laserBorn) g_MonsterManager.monsters.push_back(nb);
                    for (auto rr : g_MonsterManager.rangedMobs) {
                        if (!rr->alive || !inLine(rr->worldX, rr->worldY)) continue;
                        float dealt = (ldmg < rr->hp) ? ldmg : rr->hp; rr->hp -= dealt;
                        SpawnDamageNumber(rr->worldX, rr->worldY, dealt, dealt >= 40.0f || lcrit);
                        if (rr->hp <= 0.0f) {
                            rr->alive = false; rr->scored = true; AddKillCombo();
                            const long long pickupXp = (long long)(
                                (25.0f + (float)g_Stats.rangedXpBonus) * g_Stats.xpMult);
                            SpawnStardust(rr->worldX, rr->worldY, 3,
                                          pCX, pCY, pickupXp);
                            g_Stats.killCount++; g_GameManager.scoreAccum += 300.0f;
                            g_GameManager.score = (long long)g_GameManager.scoreAccum; lOnKill();
                        }
                    }
                    for (auto bm : g_MonsterManager.bombers) {
                        if (!bm->alive || !inLine(bm->worldX, bm->worldY)) continue;
                        float dealt = (ldmg < bm->hp) ? ldmg : bm->hp; bm->hp -= dealt;
                        SpawnDamageNumber(bm->worldX, bm->worldY, dealt, dealt >= 40.0f || lcrit);
                        if (bm->hp <= 0.0f) {
                            bm->alive = false; bm->scored = true; AddKillCombo();
                            const long long pickupXp = (long long)(
                                (25.0f + (float)g_Stats.bomberXpBonus) * g_Stats.xpMult);
                            SpawnStardust(bm->worldX, bm->worldY, 3,
                                          pCX, pCY, pickupXp);
                            g_Stats.killCount++; g_GameManager.scoreAccum += 200.0f;
                            g_GameManager.score = (long long)g_GameManager.scoreAccum; lOnKill();
                        }
                    }
                    auto lhitB = [&](float ex, float ey, float& hp, bool& al) {
                        if (!inLine(ex, ey)) return;
                        float dealt = (ldmg < hp) ? ldmg : hp; hp -= dealt;
                        SpawnDamageNumber(ex, ey, dealt, dealt >= 40.0f || lcrit);
                        ApplyBossLifestealFromDamage(dealt);
                        if (hp <= 0.0f) al = false;
                    };
                    if (g_RRBoss && g_RRBoss->alive)
                        lhitB(g_RRBoss->worldX, g_RRBoss->worldY, g_RRBoss->hp, g_RRBoss->alive);
                    if (g_CentiBoss && g_CentiBoss->alive && g_CentiBoss->vulnerable())
                        lhitB(g_CentiBoss->worldX, g_CentiBoss->worldY, g_CentiBoss->hp, g_CentiBoss->alive);
                    if (g_TessBoss && g_TessBoss->alive)
                        lhitB(g_TessBoss->worldX, g_TessBoss->worldY, g_TessBoss->hp, g_TessBoss->alive);
                    if (g_EtherBoss && g_EtherBoss->alive && inLine(g_EtherBoss->worldX, g_EtherBoss->worldY))
                        HitEtherBossDirect(ldmg, pCX, pCY, lcrit);
                    g_LaserBeams.push_back({ pCX, pCY, lex, ley, 0.13f, 0.13f, beamW });
                    TriggerMuzzle(pCX, pCY, lang);
                }
            }

            // ?�?�?백신 ?�캔 (증강) ??주기?�으�??�레?�어 주�? ?�화 ?�스(범위 ?�소) ?�?�?
            if (g_Stats.purgeNova > 0) {
                int   n      = g_Stats.purgeNova;
                float novaInt = NOVA_INT / (1.0f + 0.2f * (float)(n - 1));
                float novaR   = NOVA_R   * (1.0f + 0.18f * (float)(n - 1));
                g_NovaTimer += delta;
                if (g_NovaTimer >= novaInt) {
                    g_NovaTimer = 0.0f;
                    float dmg = g_Stats.GetBaseDamage() * g_Stats.GetDamageMultiplier(0.0f) * 2.0f;
                    float r2  = novaR * novaR;
                    auto nOnKill = [&]() {
                        if (g_Stats.lifestealPerKill > 0.0f) {
                            g_GameManager.playerHP += g_Stats.lifestealPerKill;
                            if (g_GameManager.playerHP > g_Stats.maxHP) g_GameManager.playerHP = g_Stats.maxHP;
                        }
                    };
                    for (auto m : g_MonsterManager.monsters) {
                        if (!m->alive) continue;
                        float dx=m->worldX-pCX, dy=m->worldY-pCY;
                        if (dx*dx+dy*dy < r2) {
                            m->hp -= dmg;
                            if (m->hp <= 0.0f && !m->scored) {
                                m->alive=false; m->scored=true; AddKillCombo();
                                float bx,bs; MobKillReward(m->kind,m->splitGen,m->elite,bx,bs,
                                                           g_Stats.splitterBoost);
                                TryHackFirewallOnKill(m->kind, g_Stats);
                                float rwm = MobRewardMult(m->kind); bx *= rwm; bs *= rwm;
                                const long long pickupXp = (long long)(
                                    (bx + MobDebuffXpBonus(m->kind, m->elite, g_Stats)) * g_Stats.xpMult);
                                SpawnStardust(m->worldX, m->worldY,
                                              StardustRewardFor(m->kind), pCX, pCY, pickupXp);
                                g_Stats.killCount++; g_GameManager.scoreAccum += bs;
                                g_GameManager.score=(long long)g_GameManager.scoreAccum; nOnKill();
                            }
                        }
                    }
                    for (auto bmb : g_MonsterManager.bombers) {
                        if (!bmb->alive) continue;
                        float dx=bmb->worldX-pCX, dy=bmb->worldY-pCY;
                        if (dx*dx+dy*dy < r2) {
                            bmb->hp -= dmg;
                            if (bmb->hp<=0.0f && !bmb->scored){ bmb->alive=false; bmb->scored=true; AddKillCombo();
                                const long long pickupXp = (long long)((25.0f+(float)g_Stats.bomberXpBonus)*g_Stats.xpMult);
                                SpawnStardust(bmb->worldX, bmb->worldY, 3,
                                              pCX, pCY, pickupXp);
                                g_Stats.killCount++; g_GameManager.scoreAccum+=200.0f;
                                g_GameManager.score=(long long)g_GameManager.scoreAccum; nOnKill(); }
                        }
                    }
                    for (auto r : g_MonsterManager.rangedMobs) {
                        if (!r->alive) continue;
                        float dx=r->worldX-pCX, dy=r->worldY-pCY;
                        if (dx*dx+dy*dy < r2) {
                            r->hp -= dmg;
                            if (r->hp<=0.0f && !r->scored){ r->alive=false; r->scored=true; AddKillCombo();
                                const long long pickupXp = (long long)((25.0f+(float)g_Stats.rangedXpBonus)*g_Stats.xpMult);
                                SpawnStardust(r->worldX, r->worldY, 3,
                                              pCX, pCY, pickupXp);
                                g_Stats.killCount++; g_GameManager.scoreAccum+=300.0f;
                                g_GameManager.score=(long long)g_GameManager.scoreAccum; nOnKill(); }
                        }
                    }
                    // 보스 ??범위 ?�면 ??�?�?보스 체력????�� ?�게 고정??
                    auto nhitB = [&](float ex, float ey, float& hp, bool& al) {
                        float dx=ex-pCX, dy=ey-pCY;
                        if (dx*dx+dy*dy < (novaR+70.0f)*(novaR+70.0f)) {
                            float bdmg = dmg * 2.0f;
                            float dealt = std::min(bdmg, hp);
                            hp -= dealt;
                            ApplyBossLifestealFromDamage(dealt);
                            if (hp<=0.0f) al=false;
                        }
                    };
                    if (g_RRBoss && g_RRBoss->alive) nhitB(g_RRBoss->worldX,g_RRBoss->worldY,g_RRBoss->hp,g_RRBoss->alive);
                    if (g_CentiBoss && g_CentiBoss->alive && g_CentiBoss->vulnerable()) nhitB(g_CentiBoss->worldX,g_CentiBoss->worldY,g_CentiBoss->hp,g_CentiBoss->alive);
                    if (g_TessBoss && g_TessBoss->alive) nhitB(g_TessBoss->worldX,g_TessBoss->worldY,g_TessBoss->hp,g_TessBoss->alive);
                    if (g_EtherBoss && g_EtherBoss->alive) {
                        float dx = g_EtherBoss->worldX - pCX, dy = g_EtherBoss->worldY - pCY;
                        if (dx*dx + dy*dy < (novaR+70.0f)*(novaR+70.0f))
                            HitEtherBossDirect(dmg * 2.0f, pCX, pCY, false);
                    }
                    // ?�각 ???�창 �?SpawnShockWave ?�사?? + ?�맛
                    SpawnShockWave(pCX, pCY, novaR, 0.45f, 0.4f, 1.0f, 0.75f);
                    SpawnSparks(pCX, pCY, 10, 0.4f, 1.0f, 0.7f, 360.0f);
                }
            }

            // ?�탑 모드?�서???�레?�어가 발사?��? ?�음
            if (!g_Stats.turretMode) {
                // C13 ?�동 발사: 기본 ON ?�면 좌클�??�이??조�? 방향?�로 ?�동 발사.
                //   C14 ?�예 중에??발사 ?�제(?�발 방�?). ?�동 모드�?좌클�??�??
                bool fireHeld = (g_AutoFire || lmb) && (g_PostPickGrace <= 0.0f);
                if (g_Stats.meleeWeapon) {       // 검�???근접 ?�윙 (총알 ?�음)
                    // ?��? 방�?: 쿨다??effInterval)?�??�릭/?�??무�? ??�� ?�용.
                    //   (?�전??버튼 ?�면 fireTimer �?즉시 준�??�태�??�려 광클�?                    //    ?�윙 ?�도�?무한???�릴 ???�었????�?리셋???�거)
                    if (fireHeld && fireTimer >= effInterval) {
                        float tx, ty;
                        if (aimTarget(tx, ty)) {   // ?�동: 최근????/ ?�릭: 커서
                            meleeSwing(atan2f(ty - pCY, tx - pCX));
                            fireTimer = 0.0f;
                        }
                    }
                } else if (g_Stats.bowWeapon) {  // 궁수 ???�른 만큼 차징 ??발사
                    // ?�동: ?�차징까�? ?�동 충전 ???�충?�면 ?�동 발사(차�? ?�이??반복).
                    bool bowHold = g_AutoFire ? (g_ArcherCharge < 1.0f && g_PostPickGrace <= 0.0f)
                                              : lmb;
                    if (bowHold) {
                        // Power Draw shortens charge time by 28%; bowChargeRateMult stacks after it.
                        g_ArcherCharge += delta / (BOW_CHARGE_TIME * (g_Stats.powerDraw ? 0.72f : 1.0f))
                                          * g_Stats.bowChargeRateMult;
                        if (g_ArcherCharge > 1.0f) g_ArcherCharge = 1.0f;
                    } else if (g_ArcherCharge > 0.001f) {
                        float charge = g_ArcherCharge;
                        float atx, aty; if (!aimTarget(atx, aty)) { atx = wmx; aty = wmy; }
                        float ang = atan2f(aty - pCY, atx - pCX);
                        // Full charge multiplier: base 4.1x, Power Draw 4.7x before cap bonuses.
                        float chMult = 0.5f + charge *
                                       ((g_Stats.powerDraw ? 4.2f : 3.6f) + g_Stats.bowChargeCapBonus);
                        float arrowDmg = g_Stats.GetBaseDamage()
                                       * g_Stats.GetDamageMultiplier(0.0f) * chMult;
                        if (g_Stats.berserk) {
                            float hf = g_Stats.maxHP > 0.0f
                                     ? g_GameManager.playerHP / g_Stats.maxHP : 1.0f;
                            if (hf < 0.0f) hf = 0.0f; else if (hf > 1.0f) hf = 1.0f;
                            arrowDmg *= (1.0f + (1.0f - hf) * 0.6f);
                        }
                        auto fireArrow = [&](float a, float dmgScale = 1.0f) {
                            Bullet nb(pCX, pCY, pCX + cosf(a) * 200.0f, pCY + sinf(a) * 200.0f);
                            nb.speed       = effSpeed;
                            nb.maxRange    = 1500.0f;
                            nb.remainingDmg= arrowDmg * dmgScale;
                            nb.sizeScale   = 1.0f + charge * 3.0f;     // 최�? 4�??�기
                            nb.color       = glm::vec3(0.75f, 0.95f, 0.45f);
                            g_Bullets.push_back(nb);
                        };
                        // ?�중 ?�격: ?�충(>0.85) 발사 ??3�?부채꼴
                        if (g_Stats.multishot && charge > 0.85f) {
                            fireArrow(ang);
                            fireArrow(ang + 0.16f, 0.58f);
                            fireArrow(ang - 0.16f, 0.58f);
                        } else {
                            fireArrow(ang);
                        }
                        TriggerMuzzle(pCX, pCY, ang);
                        SpawnSparks(pCX + cosf(ang)*26.0f, pCY + sinf(ang)*26.0f,
                                    2 + (int)(charge*4), 0.8f, 1.0f, 0.5f);
                        g_ArcherCharge = 0.0f;
                    }
                } else if (g_Stats.brokenSight) {
                    if (fireTimer >= effInterval && g_Orb.active) {
                        spawnAimed(g_Orb.x, g_Orb.y);
                        fireTimer = 0.0f;
                    }
                } else {
                    // ?�???�사 ??공속(effInterval) 준?? ?�동발사 OFF?�도 LMB ?�?????�사.
                    if (fireHeld && fireTimer >= effInterval) {
                        float tx, ty;
                        bool haveTarget = aimTarget(tx, ty);
                        if (g_DrunkActive) {
                            float a = (float)(rand() % 628) * 0.01f;
                            tx = pCX + cosf(a) * 200.0f;
                            ty = pCY + sinf(a) * 200.0f;
                            haveTarget = true;
                        }
                        if (haveTarget) {
                            spawnAimed(tx, ty);
                            fireTimer = 0.0f;
                        }
                    }
                }
            }
        }

        // GameManager 가 ?�버/변???�태�??????�게 ?�기??(Render ?�서 ?�용)
        if (g_InBossIntermission &&
            g_GameManager.currentState == GameState::RUNNING) {
            g_IntermissionTimer -= delta;
            GameState preShop = g_GameManager.currentState;
            float ipCX = playerWin.x + playerWin.width  * 0.5f;
            float ipCY = playerWin.y + playerWin.height * 0.5f;
            TickIntermissionZones(ipCX, ipCY, delta);
            if (preShop == GameState::RUNNING &&
                g_GameManager.currentState == GameState::RUN_SHOP)
                g_HoveredAug = -1;
        }

        g_GameManager.hoveredCard = g_HoveredAug;
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
                              wgs == GameState::RUN_SHOP ||
                              (wgs == GameState::RUNNING && g_InBossIntermission) ||
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
        //  (d) ?�레?�어 캐릭???�망 ?�펙??        //  (e) ?�레?�어 �??��????�몹·총알 (scissor ?�리?? ??가????        //  (f) BrokenSight ?�브 (?�리???�음)
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
            if (!m->alive || m->kind != MobKind::SPAWNER) continue;
            float w = SPAWNER_WIN_W * m->sizeScale;
            addW(m->worldX, m->worldY, w, w, L"HIVE", 0.06f,0.10f,0.09f, 0.20f,0.85f,0.65f);
        }
        // DDOS ??DrawAppWindow ?�합 ?�스 (e3) ?�서�??�더
        // ?�거�?�?(?�환 ?�서)
        for (auto r : g_MonsterManager.rangedMobs) {
            if (r->deathScale <= 0.0f) continue;
            float sc = r->deathScale;
            addW(r->worldX, r->worldY, RFW_W*sc, RFW_H*sc, L"LENS", 0.05f,0.07f,0.12f, 0.20f,0.75f,0.88f);
        }
        // GLITCH.exe: ���� �÷��̾� â���� �Ϲ� drawMob
        // 보스/분열�?(?�단)
        if (g_RRBoss && g_RRBoss->alive)
            addW(g_RRBoss->worldX, g_RRBoss->worldY, RR_WIN_W, RR_WIN_W,
                 L"VOLLEY", 0.10f,0.07f,0.06f, 1.0f,0.55f,0.20f, WIN_TB,
                 g_RRBoss->hp / g_RRBoss->maxHp);
        if (g_CentiBoss && g_CentiBoss->alive)
            addW(g_CentiBoss->worldX, g_CentiBoss->worldY, CENTI_WIN_W, CENTI_WIN_W,
                 CentipedeBoss::BOSS_NAME, 0.02f,0.03f,0.04f, 0.22f,0.55f,0.72f, WIN_TB,
                 g_CentiBoss->hp / g_CentiBoss->maxHp);
        if (g_TessBoss && g_TessBoss->alive)
            addW(g_TessBoss->worldX, g_TessBoss->worldY, TESS_WIN_W, TESS_WIN_W,
                 TesseractGlitchBoss::BOSS_NAME, 0.05f,0.02f,0.08f, 0.95f,0.35f,1.0f, WIN_TB,
                 g_TessBoss->hp / g_TessBoss->maxHp);
        if (g_EtherBoss && g_EtherBoss->alive && !g_EtherBoss->taskKillActive)
            addW(g_EtherBoss->worldX, g_EtherBoss->worldY,
                 ETHER_WIN_W, ETHER_WIN_W,
                 EtherSwordBoss::BOSS_NAME, 0.02f,0.04f,0.08f, 0.55f,0.80f,1.0f, WIN_TB,
                 g_EtherBoss->hp / g_EtherBoss->maxHp);
        // 터렛 신호 프레임. 실제 터렛 영역/스크리저는 그대로 유지한다.
        if (g_Stats.turretMode) {
            for (auto& t : g_Turrets) {
                DrawInGameSignalFrame(t.x - TURRET_WIN_W * 0.5f,
                                      t.y - TURRET_WIN_H * 0.5f,
                                      TURRET_WIN_W, TURRET_WIN_H,
                                      0.30f, 0.95f, 1.0f, 0.30f);
            }
        }
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

        // Rear sight-field prepass. Collect all black CircleTexture markers and
        // submit them in one icon draw before entity bodies.
        if (g_GameManager.currentState != GameState::DYING &&
            g_GameManager.currentState != GameState::GAMEOVER) {
            static std::vector<EnemySightRearMarker> rearMarkers;
            rearMarkers.clear();
            rearMarkers.reserve(1u + g_MonsterManager.monsters.size()
                                + g_MonsterManager.rangedMobs.size()
                                + g_MonsterManager.bombers.size());
            const float pCX = playerWin.x + playerWin.width * 0.5f;
            const float pCY = playerWin.y + playerWin.height * 0.5f;
            const float playerSize = PLAYER_SIZE * g_Stats.playerSizeMult;
            rearMarkers.push_back({pCX, pCY, playerSize * 2.45f});
            for (auto m : g_MonsterManager.monsters) {
                if (!m || !m->alive || m->sizeScale <= 0.0f) continue;
                const float base = (m->summoned ? 28.0f : 18.0f) * m->sizeScale;
                const float rearScale = m->kind == MobKind::GRAVIS ? 3.35f
                    : (m->kind == MobKind::QUASAR ? 3.05f : 2.55f);
                rearMarkers.push_back({m->worldX, m->worldY, base * rearScale});
            }
            for (auto r : g_MonsterManager.rangedMobs) {
                if (!r || r->deathScale <= 0.0f) continue;
                const float base = RangedMob::VISUAL_BASE_PX * r->deathScale;
                rearMarkers.push_back({r->worldX, r->worldY, base * 2.65f});
            }
            for (auto bm : g_MonsterManager.bombers) {
                if (!bm || !bm->alive) continue;
                rearMarkers.push_back({bm->worldX, bm->worldY, Bomber::SIZE_PX});
            }
            DrawEnemySightRearBatch(rearMarkers);
        }
        BatchFlush();

        // Colored sight fields share one texture and now use one tinted batch.
        // Submit them before bodies so the original visual layering is kept.
        BeginEnemySightFrontBatch();
        for (auto m : g_MonsterManager.monsters)
            QueueMonsterSightFront(m);
        for (auto r : g_MonsterManager.rangedMobs)
            QueueRangedMobSightFront(r);
        FlushEnemySightFrontBatch();
        BatchFlush();

        for (auto m : g_MonsterManager.monsters) {
            if (m->alive) drawMob(m);
        }
        for (auto bm : g_MonsterManager.bombers) {
            if (!bm->alive) continue;
            drawPentagon(bm->worldX, bm->worldY, Bomber::SIZE_PX,
                         bm->color.r, bm->color.g, bm->color.b, 1.0f);
            if (bm->arming)
                drawCircle(bm->worldX, bm->worldY, bm->blastRadius,
                           1.0f, 0.2f, 0.2f, 0.10f);
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

        // (c) player playfield signal. The rectangular region itself remains
        // available to gameplay/scissor code, but no opaque fake window is drawn.
        BatchFlush(); glDisable(GL_SCISSOR_TEST); glEnable(GL_BLEND);
        EnsurePlayerBounds(playerWin);

        if (g_InBossIntermission || g_GameManager.currentState == GameState::RUN_SHOP) {
            float wx = g_ShopZoneX - RUN_SHOP_WIN_W * 0.5f;
            float wy = g_ShopZoneY - RUN_SHOP_WIN_H * 0.5f;
            DrawAppWindow(wx, wy, RUN_SHOP_WIN_W, RUN_SHOP_WIN_H, L"AUGMENT CACHE");
        }
        if (g_InBossIntermission &&
            g_GameManager.currentState != GameState::RUN_SHOP) {
            float pulse = 0.5f + 0.5f * sinf((float)glfwGetTime() * 4.5f);
            float holdF = g_ShopZoneHold / SHOP_ZONE_HOLD_S;
            if (holdF > 1.0f) holdF = 1.0f;
            float shopA = 0.08f + 0.12f * pulse + 0.22f * holdF;
            drawCircle(g_ShopZoneX, g_ShopZoneY, SHOP_ZONE_RADIUS,
                       1.0f, 0.82f, 0.22f, shopA);
            float skipF = g_SkipZoneHold / SKIP_ZONE_HOLD_S;
            if (skipF > 1.0f) skipF = 1.0f;
            float skipA = 0.08f + 0.10f * pulse + 0.24f * skipF;
            drawCircle(g_SkipZoneX, g_SkipZoneY, SKIP_ZONE_RADIUS,
                       0.45f, 0.75f, 1.0f, skipA);
            drawNeonBorder(g_SkipZoneX - SKIP_ZONE_RADIUS, g_SkipZoneY - SKIP_ZONE_RADIUS,
                           SKIP_ZONE_RADIUS * 2.0f, SKIP_ZONE_RADIUS * 2.0f,
                           0.5f, 0.75f, 1.0f);
        }
    
        // (c2) HP/EXP �????�레?�어 �??�단 ?�쪽??부�?(창과 ?�께 ?�동) ?�?�?
        if (g_GameManager.currentState == GameState::RUNNING ||
            g_GameManager.currentState == GameState::PAUSED ||
            g_InBossIntermission ||
            g_GameManager.currentState == GameState::RUN_SHOP ||
            g_GameManager.currentState == GameState::AUG_SELECT ||
            g_GameManager.currentState == GameState::DEBUFF_SELECT ||
            g_GameManager.currentState == GameState::DYING) {
            const float pCX = playerWin.x + playerWin.width * 0.5f;
            const float pCY = playerWin.y + playerWin.height * 0.5f;
            const float sz = PLAYER_SIZE * g_Stats.playerSizeMult;
            const float hpRadius = std::max(66.0f, sz * 2.72f);
            const float xpRadius = hpRadius + 12.0f;
            // HP occupies the lower half around the player. XP mirrors it
            // vertically while keeping the same left-to-right reading flow.
            // The shared slow spin keeps both halves aligned as one radial
            // instrument while preserving their opposite positions.
            const float radialSpin = (float)glfwGetTime() * 0.12f;
            const float hpStartAngle = 3.1415927f + radialSpin;
            const float hpSweepAngle = -3.1415927f;
            const float xpStartAngle = 3.1415927f + radialSpin;
            const float xpSweepAngle =  3.1415927f;
            // One half-orbit takes about 14 seconds. The data arcs remain fixed;
            // only their small outer constellation scanner moves.
            const float orbitPhase = fmodf((float)glfwGetTime() * 0.07f, 1.0f);

            // HP
            float hpFrac = (g_Stats.maxHP > 0.0f) ? g_GameManager.playerHP / g_Stats.maxHP : 0.0f;
            if (hpFrac < 0.0f) hpFrac = 0.0f; if (hpFrac > 1.0f) hpFrac = 1.0f;
            float hpGhostFrac = (g_Stats.maxHP > 0.0f) ? g_HpGhost / g_Stats.maxHP : hpFrac;
            if (hpGhostFrac < hpFrac) hpGhostFrac = hpFrac;
            if (hpGhostFrac > 1.0f) hpGhostFrac = 1.0f;
            const float hpImpact = std::min(1.0f, g_HpBarPop / 0.45f);
            const bool lowHp = hpFrac <= 0.25f;
            const float hpR = 0.16f;
            const float hpG = 1.0f;
            const float hpPulse = (g_HpBarPop > 0.0f)
                ? (0.72f + 0.28f * std::min(1.0f, g_HpBarPop / 2.2f)) : 1.0f;
            // Always-on baseline is quiet; damage and low HP temporarily lift it.
            const float radialAlpha = 0.16f + hpImpact * 0.52f + (lowHp ? 0.18f : 0.0f);
            DrawPlayerRadialGauge(pCX, pCY, hpRadius, hpFrac,
                                  hpStartAngle, hpSweepAngle, 1.15f,
                                  hpR, hpG, 0.28f, radialAlpha * hpPulse,
                                  10, hpGhostFrac, orbitPhase);
            // (HP ?�치??좌상??HUD ???�시 ???�드 ?�션?�서 ?�스?��? 그리�?            //  TextRenderer 가 ?�이??VAO �??�바?�드???�후 ?�티???�더가 깨�?므�?금�?)
            // XP
            {
                const bool atCap = !BossDir::g_ActEndless && !g_CreativeMode
                                   && g_GameManager.playerLevel >= MAIN_LEVEL_CAP;
                float xpFrac = 1.0f;
                if (!atCap) {
                    long long needX = g_ExpSystem.Required(g_GameManager.playerLevel);
                    xpFrac = (needX > 0) ? (float)g_GameManager.xp / (float)needX : 0.0f;
                    if (xpFrac < 0.0f) xpFrac = 0.0f; if (xpFrac > 1.0f) xpFrac = 1.0f;
                }
                const float xr = atCap ? 1.0f : 0.32f;
                const float xg = atCap ? 0.86f : 0.94f;
                const float xb = atCap ? 0.36f : 1.0f;
                const float xpImpact = std::min(1.0f, g_XpBarPop / 1.15f);
                const float xpAlpha = 0.15f + 0.75f * xpImpact;
                DrawPlayerRadialGauge(pCX, pCY, xpRadius, xpFrac,
                                      xpStartAngle, xpSweepAngle, 0.9f,
                                      xr, xg, xb, xpAlpha * (atCap ? 0.72f : 0.82f),
                                      12, -1.0f, fmodf(orbitPhase + 0.5f, 1.0f));
            }

            // A temporary shield becomes a thin outer orbit, preserving the
            // old shield information without bringing back a bar.
            if (g_PlayerShield > 0.0f && g_Stats.maxHP > 0.0f) {
                const float shieldFrac = std::max(0.0f,
                    std::min(1.0f, g_PlayerShield / g_Stats.maxHP));
                DrawPlayerRadialGauge(pCX, pCY, xpRadius + 9.0f, shieldFrac,
                                      hpStartAngle, hpSweepAngle, 0.9f,
                                      0.34f, 0.78f, 1.0f, radialAlpha * 0.72f, 8);
            }
        }
    
        // (c2.5) 배드 ?�터 감속 구역 ??중심?�서 부?�되???��????�상 블록(깜빡???�니메이??
        if (!g_SlowZones.empty()) {
            BindMainShader();
            float zt = (float)glfwGetTime();
            for (auto& z : g_SlowZones) {
                float lifeF = (z.maxLife > 0.0f) ? (z.life / z.maxLife) : 0.0f;
                if (lifeF < 0.0f) lifeF = 0.0f;
                float gf = SlowZoneGrow(z);
                float zx = z.x + z.w*0.5f, zy = z.y + z.h*0.5f;
                float hw = z.w*0.5f*gf, hh = z.h*0.5f*gf;
                drawRect(zx - hw, zy - hh, hw*2, hh*2, 0.32f, 0.08f, 0.45f, 0.14f*lifeF + 0.04f);
                // 부??블록 ???��??�사?�수 깜빡?? 중심?�서 grow factor 까�?�??�출(?�짐)
                const int N = 7;
                float cw = z.w / (float)N, ch = z.h / (float)N;
                for (int iy = 0; iy < N; iy++) for (int ix = 0; ix < N; ix++) {
                    float fx = z.x + ((float)ix + 0.5f) * cw;
                    float fy = z.y + ((float)iy + 0.5f) * ch;
                    float dxn = (fx - zx) / (z.w*0.5f + 1e-3f);
                    float dyn = (fy - zy) / (z.h*0.5f + 1e-3f);
                    if (sqrtf(dxn*dxn + dyn*dyn) > gf) continue;   // ?�직 부?????�음
                    float seed = sinf((float)ix*12.9898f + (float)iy*78.233f) * 43758.5453f;
                    float ph = seed - floorf(seed);
                    float fl = 0.45f + 0.55f * sinf(zt * 6.0f + ph * 6.2831853f);
                    float ca = (0.10f + 0.22f * fl) * lifeF;
                    drawRect(fx - cw*0.42f, fy - ch*0.42f, cw*0.84f, ch*0.84f, 0.62f, 0.2f, 0.88f, ca);
                }
                drawNeonBorder(zx - hw, zy - hh, hw*2, hh*2, 0.75f, 0.3f, 0.95f);
            }
        }
    
        // (c3) ?�캔 ?�이?�?�????�이?�되??�?�� 관??�?(보스 ?�이?�?쿼드 ?�턴)
        if (!g_LaserBeams.empty()) {
            BindMainShader();
            for (auto& lb : g_LaserBeams) {
                float fr = lb.life / lb.maxLife; if (fr < 0) fr = 0; if (fr > 1) fr = 1;
                float dx = lb.ex - lb.ox, dy = lb.ey - lb.oy;
                float dl = sqrtf(dx*dx + dy*dy) + 1e-3f;
                float pxx = -dy/dl, pyy = dx/dl;
                for (int pass = 0; pass < 2; pass++) {
                    float th = ((pass == 0) ? 26.0f * fr + 6.0f : 7.0f * fr + 2.0f) * lb.width;
                    float cr = (pass == 0) ? 0.3f : 0.8f;
                    float cg = 1.0f;
                    float cb = (pass == 0) ? 0.9f : 1.0f;
                    float ca = (pass == 0) ? 0.35f * fr : 0.95f * fr;
                    float p1x=lb.ox+pxx*th, p1y=lb.oy+pyy*th;
                    float p2x=lb.ox-pxx*th, p2y=lb.oy-pyy*th;
                    float p3x=lb.ex+pxx*th, p3y=lb.ey+pyy*th;
                    float p4x=lb.ex-pxx*th, p4y=lb.ey-pyy*th;
                    float v[12]={p1x,p1y,p2x,p2y,p3x,p3y, p2x,p2y,p4x,p4y,p3x,p3y};
                    BatchVerts(v, 6, cr, cg, cb, ca);
                }
            }
        }
    
    
        // (d) ?�레?�어 캐릭??+ 증강 ?�펙??+ ?�망 ?�편
        {
            float pCX = playerWin.x + playerWin.width  * 0.5f;
            float pCY = playerWin.y + playerWin.height * 0.5f;
            // 총�?: 200px ?�내 ?�시 (?��????�안 ??
            if (g_Stats.bayonet)
                drawCircle(pCX, pCY, 200.0f, 0.4f, 1.0f, 0.9f, 0.10f);
    
            // 궁수 차징 게이지 (?�레?�어 ??�? ???�충 ???�색 번쩍
            if (g_Stats.bowWeapon && g_ArcherCharge > 0.001f) {
                float bw = 60.0f, bh = 7.0f;
                float bxp = pCX - bw * 0.5f, byp = pCY - 44.0f;
                drawRect(bxp - 1, byp - 1, bw + 2, bh + 2, 0.0f, 0.0f, 0.0f, 0.7f);
                bool full = (g_ArcherCharge >= 0.999f);
                float cr = full ? 1.0f : 0.6f, cg = 1.0f, cb = full ? 0.7f : 0.3f;
                drawRect(bxp, byp, bw * g_ArcherCharge, bh, cr, cg, cb, 0.95f);
            }
    
            if (g_GameManager.currentState == GameState::DYING) {
                float fade = (g_DyingTimer > 0) ? g_DyingTimer : 0.0f;
    
                float t      = 1.0f - fade;
                float shockR = 60.0f + t * 520.0f;
                float shockA = (1.0f - t) * 0.55f;
                drawCircle(g_DeathCX, g_DeathCY, shockR,
                           1.0f, 0.95f, 0.4f, shockA);
    
                // 중심 ?�광
                if (g_DeathFlash > 0.0f) {
                    float fr = 220.0f * g_DeathFlash;
                    drawCircle(g_DeathCX, g_DeathCY, fr,
                               1.0f, 1.0f, 1.0f, g_DeathFlash * 0.9f);
                    drawCircle(g_DeathCX, g_DeathCY, fr * 1.8f,
                               1.0f, 0.85f, 0.2f, g_DeathFlash * 0.45f);
                }
    
                // ?�망 ?�편
                for (int i = 0; i < MAX_DEBRIS; i++) {
                    if (!g_Debris[i].active) continue;
                    float s  = g_Debris[i].size;
                    float hs = s * 0.5f;
                    drawRect(g_Debris[i].x - hs, g_Debris[i].y - hs, s, s,
                             g_Debris[i].r, g_Debris[i].g, g_Debris[i].b, fade);
                }
            } else if (g_GameManager.currentState != GameState::GAMEOVER) {
                // ?�동 ?�상 ???�레?�어 ??먼�? 그려 ?�래??깔림), ?�명 비�? ?�이??축소
                for (auto& tr : g_Trail) {
                    float f = tr.life / tr.maxLife;        // 1??
                    float s = tr.size * (0.4f + 0.6f * f);
                    drawRect(tr.x - s*0.5f, tr.y - s*0.5f, s, s, tr.r, tr.g, tr.b, 0.28f * f);
                }
                float sz = PLAYER_SIZE * g_Stats.playerSizeMult;
                float hs = sz * 0.5f;
                // Keep the player field persistent so it reads as a HUD
                // identity layer rather than a newly spawned projectile.
                DrawPlayerSightMarker(pCX, pCY, sz * 2.45f,
                                      0.28f, 0.92f, 1.0f, 0.12f);
                DrawPlayerWeaponShell(pCX, pCY, sz, atan2f(wmy - pCY, wmx - pCX));

                // Pickups are combat foreground objects. Draw them after the player field
                // so sight masks and projectile batches cannot hide their short burst.
                for (const auto& dust : g_StardustPickups) {
                    if (!dust.alive) continue;
                    const float pulse = 0.85f + 0.15f * sinf(dust.age * 8.0f);
                    if (dust.value >= 10) {
                        // 대형 (10+): 금빛 다이아몬드
                        drawCircle(dust.x, dust.y, 22.0f, 0.0f, 0.0f, 0.0f, 0.32f);
                        drawCircle(dust.x, dust.y, 16.0f, 1.0f, 0.72f, 0.12f, 0.18f);
                        drawDiamond(dust.x, dust.y, 13.0f * pulse, 1.0f, 0.80f, 0.18f, 1.0f);
                        drawDiamond(dust.x, dust.y, 6.0f, 1.0f, 1.0f, 0.82f, 1.0f);
                    } else if (dust.value >= 5) {
                        // 중형 (5): 청록 십자
                        drawCircle(dust.x, dust.y, 17.0f, 0.0f, 0.0f, 0.0f, 0.32f);
                        drawCircle(dust.x, dust.y, 12.0f, 0.30f, 0.80f, 1.0f, 0.16f);
                        drawDiamond(dust.x, dust.y, 10.0f * pulse, 0.48f, 0.94f, 1.0f, 1.0f);
                        drawRect(dust.x - 1.5f, dust.y - 11.0f, 3.0f, 22.0f,
                                 0.82f, 1.0f, 1.0f, 0.75f);
                    } else {
                        // 소형 (1): 흰 다이아몬드
                        drawCircle(dust.x, dust.y, 13.0f, 0.0f, 0.0f, 0.0f, 0.28f);
                        drawCircle(dust.x, dust.y, 9.0f, 0.70f, 0.90f, 1.0f, 0.14f);
                        drawDiamond(dust.x, dust.y, 7.5f * pulse, 0.82f, 0.98f, 1.0f, 1.0f);
                    }
                }
                // ?�곽: ?�두???�두�?(?��?
                // 본체: 밝�? ?�안
    
                // ?�?�??�나비식 HP 게이지�????�격 ???�레?�어 ?�에 ?�다 ?�이?? ????���??�시 ?�?�?
                if (g_GameManager.currentState == GameState::RUNNING) {
                    float hf = (g_Stats.maxHP > 0.0f) ? g_GameManager.playerHP / g_Stats.maxHP : 0.0f;
                    if (hf < 0.0f) hf = 0.0f; if (hf > 1.0f) hf = 1.0f;
                    bool low = hf < 0.40f;
                    float vis = low ? 1.0f
                              : (g_HpBarPop > 1.6f ? (2.2f - g_HpBarPop) / 0.6f
                                                   : g_HpBarPop / 1.6f);
                    if (vis > 1.0f) vis = 1.0f; if (vis < 0.0f) vis = 0.0f;
                    if (vis > 0.01f) {
                        // ?�고 ?�레?�어??가깝게 ???�쪽 ?�을 ??가리도�?(가린다???�드�?
                        // HP is represented by the radial gauge around the player.
                    }
                }
            }
        }
    
        // (e) ranged enemies share the global combat pass.
        for (auto r : g_MonsterManager.rangedMobs) {
            if (r->deathScale <= 0.0f) continue;
            drawRangedMob(r);
        }
        BatchFlush();
    
        // (e2.1) turret bodies and gauges also use world-space coordinates.
        if (g_Stats.turretMode) {
            for (auto& t : g_Turrets) {
                // ?�탑 본체 ????��??(중앙 ?�각??+ 4방향 ?�출)
                float tc = 12.0f;
                drawRect(t.x - tc, t.y - 4, tc*2, 8, 0.1f, 1.0f, 0.55f, 1.0f);
                drawRect(t.x - 4, t.y - tc, 8, tc*2, 0.1f, 1.0f, 0.55f, 1.0f);
                // ?�명 �?(�??�단)
                float lifeRem = 1.0f - t.lifeTimer / TURRET_LIFE;
                if (lifeRem < 0.0f) lifeRem = 0.0f;
                float barW = TURRET_WIN_W - 24.0f;
                float barX = t.x - TURRET_WIN_W * 0.5f + 12.0f;
                float barY = t.y - TURRET_WIN_H * 0.5f + 8.0f;
                drawRect(barX, barY, barW, 5.0f,
                         0.12f, 0.12f, 0.18f, 0.7f);
                drawRect(barX, barY, barW * lifeRem, 5.0f,
                         0.1f, 1.0f, 0.55f, 0.9f);
            }
            BatchFlush();
        }
        // (f) BrokenSight orb
        if (g_Stats.brokenSight && g_Orb.active) {
            drawCircle(g_Orb.x, g_Orb.y, 16.0f, 1.0f, 1.0f, 0.1f, 0.22f);
            drawDiamond(g_Orb.x, g_Orb.y, 22.0f, 1.0f, 0.92f, 0.0f, 1.0f);
        }
    
        if (g_ShowCrosshair &&
            (g_GameManager.currentState == GameState::RUNNING ||
             g_GameManager.currentState == GameState::PAUSED  ||
             g_GameManager.currentState == GameState::DYING)) {
            float ax = wmx, ay = wmy;   // �?보정 ??�??�용??ortho ?�서 커서 ?�치???�시
            // ?�곽 ?�두????+ 중앙 ??�� (�???��???�센???�마 ??
            float cr = g_AccentR, cg = g_AccentG, cb = g_AccentB;
            drawCircle(ax, ay, 12.0f, 0.0f, 0.0f, 0.0f, 0.6f);
            drawCircle(ax, ay, 10.0f, cr, cg, cb, 0.9f);
            drawCircle(ax, ay, 5.0f, 0.05f, 0.05f, 0.05f, 0.9f);
            drawRect(ax - 1.5f, ay - 1.5f, 3.0f, 3.0f,
                     1.0f, 1.0f, 1.0f, 1.0f);
            // 4방향 짧�? ?�인 (??��)
            drawRect(ax - 14.0f, ay - 1.0f, 6.0f, 2.0f, cr, cg, cb, 0.95f);
            drawRect(ax + 8.0f,  ay - 1.0f, 6.0f, 2.0f, cr, cg, cb, 0.95f);
            drawRect(ax - 1.0f, ay - 14.0f, 2.0f, 6.0f, cr, cg, cb, 0.95f);
            drawRect(ax - 1.0f, ay + 8.0f,  2.0f, 6.0f, cr, cg, cb, 0.95f);
        }
    
        // (g) ?��??�는 죽음 ?�브 ??scissor ?�에?�만 ?�시 ((b)/(e) ?�스???�임)
    
        // (g2) 충격?????�폭�??�폭 / 보스 ?�폰 ?? ??�� ?�에 ?�시
        for (auto& sw : g_ShockWaves) {
            if (!sw.active) continue;
            float t = 1.0f - sw.life / sw.maxLife;  // 0 ??1
            float radius = sw.maxRadius * t;
            float alpha  = (1.0f - t) * 0.55f;
            drawCircle(sw.x, sw.y, radius, sw.r, sw.g, sw.b, alpha);
        }
    
        // (g2b) 검�??�윙 ?�상 ??조�? 방향 부채꼴
        for (auto& sl : g_Slashes) {
            if (!sl.active) continue;
            float t   = 1.0f - sl.life / sl.maxLife;       // 0 ??1
            float rad = sl.range * (0.72f + 0.28f * t);
            float alpha   = (1.0f - t) * 0.5f;
            float halfArc = 1.15f * (1.0f - 0.15f * t);
            drawConeFan(sl.x, sl.y, rad, sl.ang, halfArc, 0.85f, 0.95f, 1.0f, alpha);
        }
    
        // (g2c) ?��??�파??+ 머즐 ?�래????가??additive) 블렌?�으�?밝게
        if (!g_Sparks.empty() || g_MuzzleTimer > 0.0f) {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);     // additive
            for (auto& sp : g_Sparks) {
                float t = sp.life / sp.maxLife;    // 1 ??0
                drawCircle(sp.x, sp.y, sp.size * (0.5f + 0.5f * t),
                           sp.r, sp.g, sp.b, t);
            }
            if (g_MuzzleTimer > 0.0f) {
                float mt = g_MuzzleTimer / 0.05f;  // 1 ??0
                float mx2 = g_MuzzleX + cosf(g_MuzzleAng) * 26.0f;
                float my2 = g_MuzzleY + sinf(g_MuzzleAng) * 26.0f;
                drawCircle(mx2, my2, 22.0f * mt + 6.0f, 1.0f, 0.92f, 0.55f, mt * 0.9f);
                drawCircle(mx2, my2, 11.0f * mt + 3.0f, 1.0f, 1.0f, 0.9f, mt);
            }
            // 기본 분리 블렌??복원
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                                GL_ONE,       GL_ONE_MINUS_SRC_ALPHA);
        }
    

                // VOLLEY.sys ???�조 + 기동 ?�력 ?�론
        if (g_RRBoss && g_RRBoss->alive) {
            auto* rb = g_RRBoss;
            float pCX = playerWin.x + playerWin.width  * 0.5f;
            float pCY = playerWin.y + playerWin.height * 0.5f;
            float gtRR = (float)glfwGetTime();
            BindMainShader();

            rb->renderTelegraphs(pCX, pCY, gtRR, true);
            rb->renderBody(gtRR, pCX, pCY);
            BatchFlush();
        }

        if (g_TessBoss && g_TessBoss->alive) {
            float tt = (float)glfwGetTime();
            float pCX = playerWin.x + playerWin.width  * 0.5f;
            float pCY = playerWin.y + playerWin.height * 0.5f;
            g_TessBoss->renderFx(tt, pCX, pCY);
            g_TessBoss->renderBody(tt);
            BatchFlush();
        }

        if (g_EtherBoss && g_EtherBoss->alive) {
            float et = (float)glfwGetTime();
            if (g_EtherBoss->taskKillActive) {
                BatchFlush();
                glDisable(GL_SCISSOR_TEST);
                g_EtherBoss->renderTaskKill(et);
                BatchFlush();
            } else {
                g_EtherBoss->renderBody(et);

                float pct = std::max(0.0f, g_EtherBoss->hp / g_EtherBoss->maxHp);
                float bW = 440.0f, bH = 13.0f;
                float bX = screenWidth  * 0.5f - bW * 0.5f;
                float bY = 16.0f;
                drawRect(bX-2.0f, bY-2.0f, bW+4.0f, bH+4.0f, 0.0f,0.0f,0.0f, 0.72f);
                drawRect(bX, bY, bW, bH, 0.04f,0.07f,0.12f, 0.88f);
                if (pct > 0.0f) drawRect(bX, bY, bW*pct, bH, 0.45f,0.72f,1.0f, 0.88f);
                drawNeonBorder(bX, bY, bW, bH, 0.45f, 0.70f, 1.0f);
                BatchFlush();
            }
        }

        // (g4f) FORK.worm plasma chain: render body, adds, and FX globally.
        if (g_CentiBoss && g_CentiBoss->alive) {
            float ct = (float)glfwGetTime();
            float centiAimX = playerWin.x + playerWin.width  * 0.5f;
            float centiAimY = playerWin.y + playerWin.height * 0.5f;
            g_CentiBoss->renderFx(ct, centiAimX, centiAimY);
            g_CentiBoss->renderBody(ct);
            for (auto& mb : g_CentiBoss->minis)
                if (mb.alive) g_CentiBoss->drawMini(mb);
            BatchFlush();
        }

        // (h) ?�론 ??1~2�?(?�탑 모드 ???�론 ?�더 비활??
        if (g_Stats.drone && !g_Stats.turretMode &&
            g_GameManager.currentState != GameState::GAMEOVER) {
            float pCX = playerWin.x + playerWin.width  * 0.5f;
            float pCY = playerWin.y + playerWin.height * 0.5f;
            for (int d = 0; d < g_Stats.droneCount && d < MAX_DRONES; d++) {
                float ang = g_Drones[d].angle
                          + (float)d * 6.2831853f / (float)g_Stats.droneCount;
                float dx = pCX + cosf(ang) * 80.0f;
                float dy = pCY + sinf(ang) * 80.0f;
                drawDiamond(dx, dy, 14.0f, 0.2f, 0.9f, 1.0f, 1.0f);
            }
        }

        if (g_GameManager.currentState != GameState::GAMEOVER && g_Stats.chakram) {
            float pCX = playerWin.x + playerWin.width  * 0.5f;
            float pCY = playerWin.y + playerWin.height * 0.5f;
            for (int c = 0; c < g_Stats.chakramCount && c < MAX_CHAKRAMS; c++) {
                auto& ch = g_Chakrams[c];
                if (!ch.alive) continue;
                float chx = pCX + cosf(ch.angle) * CHAKRAM_RADIUS;
                float chy = pCY + sinf(ch.angle) * CHAKRAM_RADIUS;
                // ?�안 ?�날 ?�스?????�파?�웨???��? ??��)?�??�실??구분
                drawCircle(chx, chy, CHAKRAM_SIZE * 0.62f, 0.3f, 0.9f, 1.0f, 0.28f);   // 글로우
                drawDiamond(chx, chy, CHAKRAM_SIZE * 1.15f, 0.5f, 1.0f, 1.0f, 0.45f);  // ?�전???�트
                drawCircle(chx, chy, CHAKRAM_SIZE * 0.5f, 0.2f, 0.85f, 1.0f, 1.0f);
                drawCircle(chx, chy, CHAKRAM_SIZE * 0.22f, 0.04f, 0.12f, 0.18f, 1.0f);
                float hpFrac = (ch.maxHp > 0.0f) ? ch.hp / ch.maxHp : 0.0f;
                if (hpFrac < 0) hpFrac = 0; if (hpFrac > 1) hpFrac = 1;
                drawRect(chx - 14, chy - CHAKRAM_SIZE - 6, 28, 3, 0.2f, 0.2f, 0.2f, 0.7f);
                drawRect(chx - 14, chy - CHAKRAM_SIZE - 6, 28 * hpFrac, 3,
                         1.0f, 0.7f, 0.0f, 0.95f);
            }
        }
        // ?�론/차크??배치�?지�?즉시 flush
        BatchFlush();
    
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

        
    
        // ?�?�??�기부??UI/?�버?�이: 줌·흔?�기 무시?�고 ?�면 고정 좌표(base ortho)�??�?�?
        //    (?�리모프 2?�이�?�?0.5 ?�서 쿨다?�칸·메뉴?�·비?�트·?�래?��? 찌그?��???버그 fix)
        BatchFlush();   // ?�드(�?ortho) ?�형 ?��? 그린 ??base ortho �??�환
        glUniformMatrix4fv(g_MainProjLoc, 1, GL_FALSE, g_BaseOrtho);
        memcpy(g_MainOrtho, g_BaseOrtho, sizeof(g_BaseOrtho));
    
        // (h2) boss tint overlay
        {
            bool bossAlive = false;
            glm::vec3 tc(0.6f, 0.3f, 1.0f);
            if (g_RRBoss && g_RRBoss->alive) {
                bossAlive = true; tc = glm::vec3(1.0f, 0.55f, 0.2f); }
            else if (g_CentiBoss && g_CentiBoss->alive) {
                bossAlive = true; tc = glm::vec3(0.35f, 0.88f, 0.95f); }
            else if (g_TessBoss && g_TessBoss->alive) {
                bossAlive = true; tc = glm::vec3(0.95f, 0.35f, 1.0f); }
            if (bossAlive) {
                g_BossTintCol = tc;
                g_BossTintT  += delta * 0.07f;          // ~14초에 최�?
                if (g_BossTintT > 1.0f) g_BossTintT = 1.0f;
            } else {
                g_BossTintT  -= delta * 0.6f;           // 처치 ??빠르�??�복
                if (g_BossTintT < 0.0f) g_BossTintT = 0.0f;
            }
            if (g_BossTintT > 0.001f) {
                BindMainShader();
                drawRect(0, 0, (float)screenWidth, (float)screenHeight,
                         g_BossTintCol.r, g_BossTintCol.g, g_BossTintCol.b,
                         g_BossTintT * 0.06f);
            }
        }

    
        // (h2b) 보스 ?�이??HP �????�면 ?�단 고정. 몸체 �??��? 바는 ?�반 ?�막??        //   묻�? ??보이므�? ?�성 보스??체력???�단???�게 ?�시?�다 (?�름 + %).
        {
            const wchar_t* bn = nullptr;
            float bhf = 0.0f; glm::vec3 bc(1.0f, 1.0f, 1.0f);
            if (g_RRBoss && g_RRBoss->alive) {
                bn = L"VOLLEY";  bhf = g_RRBoss->hp / g_RRBoss->maxHp;
                bc = glm::vec3(1.0f, 0.55f, 0.2f);
            } else if (g_CentiBoss && g_CentiBoss->alive) {
                bn = CentipedeBoss::BOSS_NAME;  bhf = g_CentiBoss->hp / g_CentiBoss->maxHp;
                bc = glm::vec3(0.35f, 0.88f, 0.95f);
            } else if (g_TessBoss && g_TessBoss->alive) {
                bn = TesseractGlitchBoss::BOSS_NAME; bhf = g_TessBoss->hp / g_TessBoss->maxHp;
                bc = glm::vec3(0.95f, 0.35f, 1.0f);
            }
            int bossPick = -1;
            if (bn) {
                if      (bn == L"VOLLEY")   bossPick = 2;
                else if (bn == CentipedeBoss::BOSS_NAME) bossPick = 8;
                else if (bn == TesseractGlitchBoss::BOSS_NAME) bossPick = 10;
            }
            GameState st = g_GameManager.currentState;
            bool inGame = (st == GameState::RUNNING || st == GameState::PAUSED ||
                           st == GameState::DYING   || st == GameState::AUG_SELECT ||
                           st == GameState::DEBUFF_SELECT);
            if (bn && inGame) {
                if (bhf < 0.0f) bhf = 0.0f; if (bhf > 1.0f) bhf = 1.0f;
                BindMainShader();
                float bw = (float)screenWidth * 0.42f; if (bw > 820.0f) bw = 820.0f;
                float bh = 22.0f;
                float bx = ((float)screenWidth - bw) * 0.5f;
                float by = 78.0f;
                drawRect(bx - 3.0f, by - 3.0f, bw + 6.0f, bh + 6.0f, 0.05f, 0.05f, 0.07f, 0.88f);
                drawRect(bx, by, bw, bh, 0.18f, 0.16f, 0.20f, 0.92f);
                float lit = 0.5f + 0.5f * bhf;
                drawRect(bx, by, bw * bhf, bh, bc.r * lit, bc.g * lit, bc.b * lit, 0.96f);
                // ?�름 (�???중앙)
                float ns = 0.85f;
                float nw = g_TextS.Width(bn, ns);
                g_TextS.Draw(bn, ((float)screenWidth - nw) * 0.5f, by - 28.0f, ns,
                             bc.r, bc.g, bc.b, 1.0f);
                if (bossPick >= 0) {
                    const wchar_t* tag = BossDir::Tagline(bossPick);
                    float ts = 0.58f;
                    float tw = g_TextS.Width(tag, ts);
                    g_TextS.Draw(tag, ((float)screenWidth - tw) * 0.5f, by - 48.0f, ts,
                                 0.75f, 0.78f, 0.82f, 0.88f);
                }
                if (g_RRBoss && g_RRBoss->alive) {
                    wchar_t rrBuf[64];
                    if (g_RRBoss->phase3)
                        swprintf_s(rrBuf, L"OVERCLOCK \uCA0C %ls",
                                   g_RRBoss->state == RRState::ASSAULT ? L"ASSAULT" :
                                   ReloadRunnerBoss::weaponTag(g_RRBoss->weapon));
                    else if (g_RRBoss->phase2)
                        swprintf_s(rrBuf, L"P2 \uCA0C %ls",
                                   g_RRBoss->state == RRState::RELOAD_SPRINT ? L"SPRINT" :
                                   ReloadRunnerBoss::weaponTag(g_RRBoss->weapon));
                    else
                        swprintf_s(rrBuf, L"%ls",
                                   ReloadRunnerBoss::weaponTag(g_RRBoss->weapon));
                    float rs = 0.55f;
                    float rw = g_TextS.Width(rrBuf, rs);
                    g_TextS.Draw(rrBuf, bx + bw - rw - 8.0f, by - 48.0f, rs,
                                 1.0f, 0.55f, 0.2f, 0.85f);
                } else if (g_TessBoss && g_TessBoss->alive) {
                    const wchar_t* tb = g_TessBoss->stateTag();
                    float ts2 = 0.55f;
                    float tw2 = g_TextS.Width(tb, ts2);
                    g_TextS.Draw(tb, bx + bw - tw2 - 8.0f, by - 48.0f, ts2,
                                 0.95f, 0.35f, 1.0f, 0.88f);
                }
                                                // % (�??�측 ???�쪽)
                wchar_t pct[16]; swprintf_s(pct, L"%d%%", (int)(bhf * 100.0f + 0.5f));
                float ps = 0.7f;
                float pw = g_TextS.Width(pct, ps);
                g_TextS.Draw(pct, bx + bw - pw - 8.0f, by + 3.0f, ps, 1.0f, 1.0f, 1.0f, 0.95f);
            }
        }
    

        // (h2c) 보스 ?�장 ?�조(증상) ???�폰 2.5�????�마 ?�출 + 경고 배너
        if (g_BossWarnTimer > 0.0f &&
            (g_GameManager.currentState == GameState::RUNNING ||
             g_GameManager.currentState == GameState::PAUSED)) {
            float warnDur = g_CreativeMode ? 0.35f : BOSS_WARN_DUR * TrialBossWarningMult();
            if (warnDur < 0.25f) warnDur = 0.25f;
            float prog = 1.0f - g_BossWarnTimer / warnDur;   // 0��1
            if (prog < 0.0f) prog = 0.0f; if (prog > 1.0f) prog = 1.0f;
            float t = (float)glfwGetTime();
            float blink = 0.5f + 0.5f * sinf(t * 9.0f);
            glm::vec3 wc = BossDir::WarnColor(g_BossWarnPick);
            BindMainShader();
            float sw2 = (float)screenWidth, sh2 = (float)screenHeight;
            // 공통: 가?�자�?비네??(보스?? progress 비�?�?짙어�?
            float ea = (0.05f + 0.13f * prog) * (0.6f + 0.4f * blink);
            float eb = 70.0f;
            drawRect(0, 0, sw2, eb, wc.r, wc.g, wc.b, ea);
            drawRect(0, sh2 - eb, sw2, eb, wc.r, wc.g, wc.b, ea);
            drawRect(0, 0, eb, sh2, wc.r, wc.g, wc.b, ea);
            drawRect(sw2 - eb, 0, eb, sh2, wc.r, wc.g, wc.b, ea);
    
            // 보스�?증상 ?�마
            switch (g_BossWarnPick) {
            case 1: break;
            case 2: {
                float cx = sw2 * 0.5f, cy = sh2 * 0.42f;
                for (int e = 0; e < 4; e++) {
                    float ex = (e % 2) ? sw2 - 20.0f : 20.0f;
                    float ey = (e < 2) ? 20.0f : sh2 - 20.0f;
                    const int SEG = 8;
                    for (int s = 0; s < SEG; s++) {
                        if ((s & 1) == 0) continue;
                        float u0 = (float)s / (float)SEG, u1 = (float)(s + 1) / (float)SEG;
                        float x0 = ex + (cx - ex) * u0, y0 = ey + (cy - ey) * u0;
                        float x1 = ex + (cx - ex) * u1, y1 = ey + (cy - ey) * u1;
                        drawRect((x0 + x1) * 0.5f - 2, (y0 + y1) * 0.5f - 2, 4, 4,
                                 1.0f, 0.5f, 0.15f, 0.15f + 0.25f * blink * prog);
                    }
                }
                drawRect(cx - 28.0f, cy - 18.0f, 56.0f, 36.0f, 0.08f, 0.06f, 0.07f, 0.5f);
                drawNeonBorder(cx - 28.0f, cy - 18.0f, 56.0f, 36.0f, 1.0f, 0.55f, 0.18f);
                for (int i = 0; i < 4; i++) {
                    float a = t * 2.0f + (float)i * 1.571f;
                    drawRect(cx + cosf(a) * 34.0f - 3, cy + sinf(a) * 22.0f - 3,
                             6, 6, 1.0f, 0.45f, 0.12f, 0.35f + 0.2f * prog);
                }
            } break;
            case 7: {
                drawRect(0, 0, sw2, sh2, 0.02f, 0.06f, 0.03f, 0.06f + 0.08f * prog);
                for (int i = 0; i < 8; i++) {
                    float ly = 40.0f + (float)(i * 47) + fmodf(t * 60.0f, 47.0f);
                    if (ly > sh2) continue;
                    float lw = 80.0f + (float)(rand() % (int)(sw2 * 0.5f));
                    drawRect(30.0f, ly, lw, 3.0f, wc.r, wc.g, wc.b, 0.15f + 0.12f * blink);
                }
                drawCircle(sw2 * 0.5f, sh2 * 0.5f, 90.0f + prog * 60.0f,
                           wc.r, wc.g, wc.b, 0.06f + 0.05f * blink);
                for (int i = 0; i < 6; i++) {
                    float a = (float)i * 1.047f + t * 0.4f;
                    float nx = sw2 * 0.5f + cosf(a) * (120.0f + prog * 80.0f);
                    float ny = sh2 * 0.5f + sinf(a) * (120.0f + prog * 80.0f);
                    drawRect(sw2 * 0.5f, sh2 * 0.5f, nx - sw2 * 0.5f, 2.0f,
                             wc.r, wc.g, wc.b, 0.1f);
                    drawRect(nx - 8.0f, ny - 6.0f, 16.0f, 12.0f,
                             0.1f, 0.2f, 0.12f, 0.35f + 0.2f * blink);
                }
            } break;
            case 8: {  // FORK ???�단?�서 기어?�는 분절 몸통
                float crawl = 40.0f + 90.0f * prog;
                for (int s = 0; s < 9; s++) {
                    float sx = sw2 * 0.08f + s * sw2 * 0.105f;
                    float bob = sinf(t * 6.0f + s * 0.7f) * 6.0f;
                    drawCircle(sx, sh2 - crawl + bob, 14.0f + (float)(s % 3) * 3.0f,
                               0.35f, 0.85f, 0.25f, 0.2f + 0.15f * prog);
                }
            } break;
            case 9: {  // ADUN.relay ? ���� �ݳ� ������(���� ��) + ���� ����
                float cx = sw2 * 0.5f, cy = sh2 * 0.52f;
                float pulse = 0.5f + 0.5f * sinf(t * 4.0f);
                float ringR = sw2 * 0.1f + pulse * sw2 * 0.006f;
                for (int i = 0; i < 24; i++) {
                    float a = (float)i / 24.0f * 6.283f + t * 0.5f;
                    float dx = cx + cosf(a) * ringR, dy = cy + sinf(a) * ringR;
                    drawCircle(dx, dy, 2.4f, wc.r, wc.g, wc.b, 0.35f + 0.25f * prog);
                }
                for (int i = 0; i < 10; i++) {
                    float a = (float)i / 10.0f * 6.283f + t * 0.5f;
                    float ix = cx + cosf(a) * (ringR + 12.0f), iy = cy + sinf(a) * (ringR + 12.0f);
                    drawCircle(ix, iy, 3.0f, wc.r, wc.g, wc.b, 0.5f + 0.3f * blink);
                }
                for (int i = 0; i < 6; i++) {
                    float a = t * 1.6f + (float)i * 1.047f;
                    float rad = sw2 * 0.14f + prog * sw2 * 0.08f;
                    float ix = cx + cosf(a) * rad, iy = cy + sinf(a) * rad * 0.65f;
                    drawDiamond(ix, iy, 7.0f + pulse * 2.0f, 1.0f, 0.85f, 1.0f, 0.4f + 0.3f * blink);
                }
                drawCircle(cx, cy, 6.0f + pulse * 3.0f, wc.r, wc.g, wc.b, 0.3f + pulse * 0.3f);
            } break;
            case 3: {  // SPAM.dll ? ���� �˾��� ȭ�� �����ڸ����� Ƣ����� ����
                float cx = sw2 * 0.5f, cy = sh2 * 0.5f;
                for (int i = 0; i < 8; i++) {
                    float a = (float)i * 0.7853982f + t * 0.3f;
                    float dist = sw2 * 0.36f * (1.0f - prog * 0.3f);
                    float px = cx + cosf(a) * dist, py = cy + sinf(a) * dist * 0.6f;
                    float ww2 = 34.0f, hh2 = 22.0f;
                    drawRect(px - ww2 * 0.5f, py - hh2 * 0.5f, ww2, hh2,
                             wc.r * 0.4f, wc.g * 0.4f, wc.b * 0.4f, 0.10f + 0.14f * prog);
                    drawRect(px - ww2 * 0.5f, py - hh2 * 0.5f, ww2, hh2 * 0.32f,
                             wc.r, wc.g, wc.b, 0.18f + 0.18f * prog * blink);
                }
                drawTriangle(cx, cy, 30.0f + prog * 14.0f, wc.r, wc.g, wc.b, 0.14f + 0.14f * blink);
                drawCircle(cx, cy + 12.0f, 4.0f, wc.r, wc.g, wc.b, 0.16f + 0.16f * blink);
            } break;
            default: break;
            }
    
            // 공통 경고 배너 (중앙 ?�단�?
            float by3 = sh2 * 0.30f;
            // ?�름 (?�?
            wchar_t banner[64]; swprintf_s(banner, L"!! %ls !!", g_BossWarnName);
            float nsc = 1.5f;
            float nw2 = g_TextL.Width(banner, nsc);
            g_TextL.Draw(banner, (sw2 - nw2) * 0.5f, by3, nsc,
                         wc.r, wc.g, wc.b, 0.7f + 0.3f * blink);
            const wchar_t* SUB[3] = { L"\uC704\uD611 \uD504\uB85C\uC138\uC2A4 \uAC10\uC9C0 - \uC2E4\uD589 \uC911...",
                                       L"THREAT PROCESS DETECTED - launching...",
                                       L"\u8105\u5A01\u30D7\u30ED\u30BB\u30B9?\u77E5 - \u8D77\u52D5\u4E2D..." };
            int li5 = LangIndex();
            float ssc = 0.7f;
            float sw3 = g_TextS.Width(SUB[li5], ssc);
            g_TextS.Draw(SUB[li5], (sw2 - sw3) * 0.5f, by3 + 44.0f, ssc, 1.0f, 1.0f, 1.0f, 0.85f);
            // 진행 게이지
            float gw = 320.0f, gh = 8.0f, gx = (sw2 - gw) * 0.5f, gy = by3 + 74.0f;
            BindMainShader();
            drawRect(gx - 2, gy - 2, gw + 4, gh + 4, 0.0f, 0.0f, 0.0f, 0.6f);
            drawRect(gx, gy, gw, gh, 0.15f, 0.15f, 0.18f, 0.9f);
            drawRect(gx, gy, gw * prog, gh, wc.r, wc.g, wc.b, 0.95f);
        }
    
        // (h2d) ?�이�? 진입 ?�스????"??과�?????PHASE 2" (1.8�??�이??
        if (g_P2ToastTimer > 0.0f &&
            (g_GameManager.currentState == GameState::RUNNING ||
             g_GameManager.currentState == GameState::DYING)) {
            float a = (g_P2ToastTimer > 1.4f) ? (1.8f - g_P2ToastTimer) / 0.4f
                                              : (g_P2ToastTimer / 1.4f);
            if (a > 1.0f) a = 1.0f; if (a < 0.0f) a = 0.0f;
            const wchar_t* P2[3] = { L">> \uACFC\uBD80\uD558 >> PHASE 2", L">> OVERLOAD >> PHASE 2",
                                      L">> \u904E\u8CA0\u8377 >> PHASE 2" };
            int li6 = LangIndex();
            float psc = 1.2f;
            float pw3 = g_TextL.Width(P2[li6], psc);
            glm::vec3& pc = g_P2ToastCol;
            g_TextL.Draw(P2[li6], ((float)screenWidth - pw3) * 0.5f,
                         (float)screenHeight * 0.22f, psc, pc.r, pc.g, pc.b, a);
        }
    
        // (h3) ?�면 ?�래?????�벨??보스처치/부???�간 번쩍
        if (g_FlashIntensity > 0.001f) {
            BindMainShader();
            float a = g_FlashIntensity; if (a > 0.85f) a = 0.85f;
            drawRect(0, 0, (float)screenWidth, (float)screenHeight,
                     g_FlashColor.r, g_FlashColor.g, g_FlashColor.b, a);
        }
        // FORK.worm ?�피 ???��???RGB 분리 글리치
        if (g_CentiBoss && g_CentiBoss->glitchOverlay > 0.001f) {
            BindMainShader();
            float go = g_CentiBoss->glitchOverlay * 7.0f;
            if (go > 1.0f) go = 1.0f;
            float off = 7.0f * go;
            float sw = (float)screenWidth, sh = (float)screenHeight;
            drawRect(off, 0.0f, sw, sh, 1.0f, 0.15f, 0.15f, go * 0.07f);
            drawRect(-off, 0.0f, sw, sh, 0.15f, 0.85f, 1.0f, go * 0.07f);
        }
    
        // (h4) ?�간 ?��? ???�면 가?�자�??�안 비네??(?��? ?�출)
        if (g_TimeStopTimer > 0.0f) {
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
    
        // (i) 취함 ?�태 ?�시 (?�면 가?�자�??�홍??비네??
        if (g_DrunkActive) {
            float a = 0.18f;
            drawRect(0, 0, (float)screenWidth, 12.0f, 0.9f, 0.1f, 0.6f, a);
            drawRect(0, (float)screenHeight - 12, (float)screenWidth, 12.0f, 0.9f, 0.1f, 0.6f, a);
            drawRect(0, 0, 12.0f, (float)screenHeight, 0.9f, 0.1f, 0.6f, a);
            drawRect((float)screenWidth - 12, 0, 12.0f, (float)screenHeight, 0.9f, 0.1f, 0.6f, a);
        }
    
        BatchFlush();
        glDisable(GL_SCISSOR_TEST);
    
        }   // if (inWorldRender)
        }   // world render game block

        // [6] HUD
        g_GameManager.Render();
    
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
                bool isApp = (st == GameState::SHOP || st == GameState::CODEX ||
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
                float bw = g_TextS.Width(tb, 1.0f) + 48.0f;
                float bx0 = (sw - bw) * 0.5f, by0 = sh * 0.10f;
                BindMainShader();
                drawRect(bx0, by0, bw, 52.0f, 0.12f, 0.10f, 0.02f, 0.85f * a);
                drawRect(bx0, by0, bw, 4.0f, 1.0f, 0.85f, 0.25f, 0.95f * a);
                g_TextS.Draw(tb, bx0 + 24.0f, by0 + 16.0f, 1.0f, 1.0f, 0.9f, 0.4f, a);
            }
    
            DrawKillTags(g_TextS, st == GameState::RUNNING || st == GameState::DYING);

            // ?�?�??�리?�이?�브 HUD (게임 �? ??F:증강  G:무적 ?�?�?
            if (g_CreativeMode &&
                (st == GameState::RUNNING || st == GameState::READY ||
                 st == GameState::AUG_SELECT || st == GameState::DEBUFF_SELECT)) {
                int li3 = LangIndex();
                const wchar_t* CH[3] = {
                    L"CREATIVE   F: \uC99D\uAC15   G: \uBB34\uC801   B: \uBCF4\uC2A4 \uC2A4\uD3F0",
                    L"CREATIVE   F: Augment   G: Godmode   B: Spawn boss",
                    L"CREATIVE   F: ?\u5316   G: \u30B4\u30C3\u30C9   B: \u30DC\u30B9\u53EC\u559A" };
                g_TextS.Draw(CH[li3], 20.0f, HudY(sh, Hud::CREATIVE_LABEL), 0.8f, 0.7f, 0.85f, 1.0f, 0.85f);
                if (g_CreativeGodmode) {
                    const wchar_t* GOD[3] = { L"* \uBB34\uC801 ON", L"* GODMODE ON", L"* \u30B4\u30C3\u30C9 ON" };
                    float blink = 0.65f + 0.35f * sinf((float)glfwGetTime() * 5.0f);
                    g_TextL.Draw(GOD[li3], 20.0f, 24.0f, 0.95f, 1.0f, 0.85f, 0.2f, blink);
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
                             0.8f, 0.7f, 0.85f, 1.0f, 0.85f);
                if (g_CreativeGodmode) {
                    const wchar_t* GOD[3] = { L"* \uBB34\uC801 ON", L"* GODMODE ON", L"* \u30B4\u30C3\u30C9 ON" };
                    float blink = 0.65f + 0.35f * sinf((float)glfwGetTime() * 5.0f);
                    g_TextL.Draw(GOD[li3], 20.0f, 24.0f, 0.95f,
                                 1.0f, 0.85f, 0.2f, blink);
                }
            }
    
            // ?�?�?[7b] UI ???�스?�치 ??메뉴/�??�태??Scene_* ?�수�?분리 ?�?�?
            //    RUNNING/DYING(?�수 ?�게?????�이 ?�으??컨텍?�트 구성 ?�체�?건너?�?
            // DWM backdrop blur applies to the whole transparent window.
            // During a run that window reaches the auto-hidden taskbar area,
            // so the taskbar would be blurred when it slides into view.
            // Keep OS-level blur for outgame compositions only; the in-game
            // UI must never affect the taskbar or other shell surfaces.
            const bool inGameComposition =
                st == GameState::READY          ||
                st == GameState::RUNNING        ||
                st == GameState::DYING          ||
                st == GameState::PAUSED         ||
                st == GameState::AUG_SELECT     ||
                st == GameState::DEBUFF_SELECT  ||
                st == GameState::AUG_REPLACE    ||
                st == GameState::RUN_SHOP       ||
                st == GameState::BOSS_INTERMISSION;
            const bool backdropBlurActive = g_BackdropBlurEnabled && !inGameComposition;
            ConfigureWindowBackdropBlur(window, backdropBlurActive);

            // Scene_* dispatch still includes READY and the in-run menus;
            // this flag only controls which state gets the scene composition.
            const bool outgameBackdrop =
                st != GameState::RUNNING && st != GameState::DYING;
            if (outgameBackdrop) {
                std::function<void()> resetFn = ResetForNewGame;
                std::function<void()> restartRunFn = RestartCurrentRun;
                std::function<void()> abandonRunFn = AbandonCurrentRun;
                // ������ ũ�� ����(���?64px) ���� Ŭ���� ���� �������� ����
                bool sceneLmb = (my >= BROWSER_CHROME_H) ? lmb : false;
                SceneCtx ctx{ sw, sh, mx, my, sceneLmb, delta, window, &fireTimer,
                              resetFn, restartRunFn, abandonRunFn };
                switch (st) {
                case GameState::MAIN_MENU:         Scene_MainMenu(ctx);         break;
                case GameState::SHOP:              Scene_Shop(ctx);             break;
                case GameState::CODEX:             Scene_Codex(ctx);            break;
                case GameState::TUTORIAL:          Scene_Tutorial(ctx);         break;
                case GameState::JOB_SELECT:        Scene_JobSelect(ctx);        break;
                case GameState::CREATIVE_CONFIG:   Scene_CreativeConfig(ctx);   break;
                case GameState::SETTINGS:          Scene_Settings(ctx);         break;
                case GameState::READY:             Scene_Ready(ctx);            break;
                case GameState::PAUSED:            Scene_Paused(ctx);           break;
                case GameState::GAMEOVER:          Scene_GameOver(ctx);         break;
                case GameState::VICTORY:           Scene_Victory(ctx);          break;
                case GameState::AUG_SELECT:
                case GameState::DEBUFF_SELECT:     Scene_AugSelect(ctx);        break;
                case GameState::AUG_REPLACE:       Scene_AugReplace(ctx);       break;
                case GameState::RUN_SHOP:          Scene_RunShop(ctx);          break;
                default: break;
                }
                if (st == GameState::PAUSED || st == GameState::AUG_SELECT ||
                    st == GameState::DEBUFF_SELECT || st == GameState::AUG_REPLACE ||
                    st == GameState::VICTORY ||
                    st == GameState::RUN_SHOP)
                    Scene_OwnedAugPanel(ctx);
            }
            // Scene_Paused can switch to SETTINGS during this render pass.
            // Apply the new state immediately instead of waiting one frame.
            UpdateCursorVisibility();

            // ?�단 HUD ???�제 게임 진행 ?�태?�서�?(메뉴/?�감/?�점?????�게)
            if (st == GameState::RUNNING || st == GameState::PAUSED ||
                st == GameState::DYING   || st == GameState::AUG_SELECT ||
                st == GameState::DEBUFF_SELECT ||
                st == GameState::RUN_SHOP ||
                (st == GameState::RUNNING && g_InBossIntermission)) {
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
                    g_TextS.Draw(lvBuf2, 12.0f, hudTopY, 0.85f, 0.7f, 1.0f, 0.7f, 0.9f);
                }
                // ?�단 중앙: Score
                wchar_t scoreBuf[64];
                swprintf_s(scoreBuf, L"%ls  %lld", T(StrId::SCORE), g_GameManager.score);
                float scoreW = g_TextS.Width(scoreBuf, 1.0f);
                g_TextS.Draw(scoreBuf, (sw - scoreW) * 0.5f, hudTopY, 1.0f,
                             1.0f, 1.0f, 1.0f, 0.95f);
    
                // ?�상?? FPS
                wchar_t fpsBuf[32];
                swprintf_s(fpsBuf, L"%ls  %d", T(StrId::FPS), g_CurrentFPS);
                float fpsW = g_TextS.Width(fpsBuf, 0.85f);
                g_TextS.Draw(fpsBuf, sw - fpsW - 12.0f, hudTopY, 0.85f,
                             0.7f, 0.9f, 1.0f, 0.85f);

                if (st == GameState::RUNNING || st == GameState::RUN_SHOP ||
                    g_InBossIntermission) {
                    const wchar_t* floorLbl = BossDir::ActLabel();
                    g_TextS.Draw(floorLbl, 12.0f, hudTopY + 42.0f, 0.72f,
                                 0.82f, 0.92f, 1.0f, 0.82f);
                    wchar_t runDustHud[64];
                    const wchar_t* runDustLabel = LangIndex() == 0 ? L"현재 별가루" : L"RUN STARDUST";
                    swprintf_s(runDustHud, L"%ls  %lld", runDustLabel, g_RunStardust);
                    float gdw = g_TextS.Width(runDustHud, 0.82f);
                    g_TextS.Draw(runDustHud, sw - gdw - 12.0f, hudTopY + 22.0f, 0.82f,
                                 1.0f, 0.86f, 0.32f, 0.9f);
                }
                if (g_InBossIntermission && st != GameState::RUN_SHOP) {
                    int sec = (int)(g_IntermissionTimer + 0.99f);
                    if (sec < 0) sec = 0;
                    wchar_t tbuf[48];
                    swprintf_s(tbuf, L"REST  %d:%02d", sec / 60, sec % 60);
                    float tw = g_TextL.Width(tbuf, 0.95f);
                    g_TextL.Draw(tbuf, (sw - tw) * 0.5f, hudTopY + 48.0f, 0.95f,
                                 1.0f, 0.92f, 0.45f, 0.92f);
                    const wchar_t* zhint[3] = {
                        L"\uC911\uC559 ASTRAL CACHE ? \uC7A0\uC2DC \uBA38\uBB3C\uBA74 \uC0C1\uC810 \u00B7 \uC624\uB978\uCABD = \uC2A4\uD0B5",
                        L"Center ASTRAL CACHE ? hold to shop \u00B7 right zone = skip",
                        L"\u4E2D\u592E ASTRAL CACHE ? \u7559\u307E\u308B\u3068\u30B7\u30E7\u30C3\u30D7 \u00B7 \u53F3=\u30B9\u30AD\u30C3\u30D7" };
                    int zli = LangIndex();
                    if (zli < 0 || zli > 2) zli = 0;
                    float zw = g_TextS.Width(zhint[zli], 0.78f);
                    g_TextS.Draw(zhint[zli], (sw - zw) * 0.5f, hudTopY + 82.0f, 0.78f,
                                 0.85f, 0.85f, 0.85f, 0.75f);
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
                    float cw = g_TextS.Width(c, 0.7f);
                    g_TextS.Draw(c, CenterX(sw, cw), HudY(sh, Hud::COMBO_TEXT), 0.7f,
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
                    float hw = g_TextL.Width(hintTxt, 0.92f);
                    float hy = (float)sh * 0.5f - 60.0f;
                    // glow pass
                    g_TextL.Draw(hintTxt, CenterX(sw, hw) - 1.0f, hy + 1.0f, 0.92f,
                                 0.3f, 1.0f, 0.5f, ap * 0.18f);
                    // core
                    g_TextL.Draw(hintTxt, CenterX(sw, hw), hy, 0.92f,
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
                        float bw = 64.0f;
                        drawRect(0, 0, sw, bw, 0.9f, 0.15f, 0.15f, a);
                        drawRect(0, sh - bw, sw, bw, 0.9f, 0.15f, 0.15f, a);
                        drawRect(0, 0, bw, sh, 0.9f, 0.15f, 0.15f, a);
                        drawRect(sw - bw, 0, bw, sh, 0.9f, 0.15f, 0.15f, a);
                        const wchar_t* LOW[3] = { L"! \uC704\uD5D8", L"! LOW HP", L"! \u5371?" };
                        int li4 = LangIndex();
                        float lw = g_TextS.Width(LOW[li4], 0.9f);
                        g_TextS.Draw(LOW[li4], CenterX(sw, lw), HudY(sh, Hud::LOW_HP_WARN),
                                     0.9f, 1.0f, 0.4f, 0.4f, 0.55f + 0.45f * pulse);
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
                    float sc = (d.crit ? 1.05f : 0.72f) * g_ViewZoom * (0.7f + 0.3f * t);
                    float a  = (t > 0.55f) ? 1.0f : (t / 0.55f);
                    float w  = g_TextS.Width(nb, sc);
                    if (d.crit) g_TextS.Draw(nb, sx - w*0.5f, sy, sc, 1.0f, 0.85f, 0.2f, a);
                    else        g_TextS.Draw(nb, sx - w*0.5f, sy, sc, 1.0f, 1.0f, 1.0f, a*0.9f);
                }
                // 콤보 카운??(5콤보 ?�상부?? ?�이 콤보???�라 강해�? ???�정 ?��?
                if (g_ShowCombo && st == GameState::RUNNING && g_Combo >= 5) {
                    bool  ms      = (g_ComboMilestone > 0.0f);
                    float msBoost = ms ? (g_ComboMilestone / 0.7f) : 0.0f;
                    wchar_t cb[32]; swprintf_s(cb, L"%d COMBO", g_Combo);
                    float sc = (1.05f + (g_Combo > 30 ? 0.3f : 0.0f))
                             * (1.0f + g_ComboPulse * 0.4f + msBoost * 0.55f);
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
                const float KW = 54.0f, KH = 48.0f, KG = 8.0f;
                float kx0 = 16.0f, ky0 = HudY(sh, KH + Hud::SKILL_KEYS_Y);
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
                    g_TextS.Draw(key, x + 4.0f, y + 7.0f, 0.55f, 1,1,1, ready ? 0.90f : 0.50f);
                    g_TextS.Draw(tag, x + 4.0f, y + KH - 17.0f, 0.5f, r,g,b, ready ? 1.0f : 0.42f);
                    if (!ready) {
                        wchar_t bf[8]; swprintf_s(bf, L"%d", (int)(cd + 0.99f));
                        float tw = g_TextL.Width(bf, 0.85f);
                        g_TextL.Draw(bf, x + (KW - tw) * 0.5f, y + KH * 0.34f, 0.85f, 1,1,1,0.88f);
                    }
                };
                skillBox(0, L"SHIFT", g_Stats.dashUpgrade ? L"FLASH" : L"DASH",
                         g_DashCd, 0.4f, 1.0f, 1.0f);
                const wchar_t* keys3[3] = { L"Q", L"E", L"R" };
                for (int i = 0; i < 3; i++) {
                    if (g_Skills[i].type == SkillType::NONE) continue;
                    const wchar_t* tag = L""; float r = 1, g = 1, b = 1;
                    switch (g_Skills[i].type) {
                    case SkillType::CLOSE_WINDOW: tag = L"CLOSE"; r=0.5f; g=0.8f; b=1.0f; break;
                    case SkillType::HYPER_FOCUS:  tag = L"FOCUS"; r=0.55f; g=0.85f; b=1.0f; break;
                    case SkillType::TIME_STOP:    tag = L"TIME";  r=0.4f; g=0.9f; b=1.0f; break;
                    case SkillType::FOCUS_AIM:    tag = L"FOCUS"; r=0.7f; g=0.9f; b=1.0f; break;
                    default: break;
                    }
                    skillBox(i + 1, keys3[i], tag, g_Skills[i].cd, r, g, b);
                }
            }
    
            // ?�?�??�티�??�시�?쿨다??UI (좌하?? ?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?
            // 추후 ?�토그램 PNG 가 ?�어?�면 ?�각??placeholder ?�리???�스�??�시
            if (st == GameState::RUNNING || st == GameState::PAUSED) {
                const float SLOT_W = 56.0f, SLOT_H = 48.0f, SLOT_GAP = 8.0f;
                float baseX  = 16.0f;
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
                    g_TextS.Draw(tag, x + 4.0f, y + 6.0f, 0.7f, r, g, b, 1.0f);
                    // ?��? ?�간 (?�수)
                    if (remain > 0.0f) {
                        wchar_t buf[16];
                        swprintf_s(buf, L"%d", (int)(remain + 0.99f));
                        float tw = g_TextL.Width(buf, 0.9f);
                        g_TextL.Draw(buf, x + (SLOT_W - tw) * 0.5f,
                                     y + SLOT_H * 0.40f, 0.9f, 1,1,1,0.95f);
                    }
                };
    
                // ?�환 ?��? ??쿨다??(20 / 15 / 7.5)
                if (g_Stats.bulletRain) {
                    float remain = g_Stats.bulletRainCooldown - g_BulletRainTimer;
                    if (remain < 0) remain = 0;
                    drawSlot(L"RAIN", remain, 1.0f, 0.5f, 0.2f);
                    ++slot;
                }
                // 취함 ??drunkCooldown ?��?/ drunkActiveDuration ?�성
                if (g_Stats.drunk) {
                    float remain = g_DrunkActive
                        ? (g_Stats.drunkCooldown + g_Stats.drunkActiveDuration - g_DrunkCycle)
                        : (g_Stats.drunkCooldown - g_DrunkCycle);
                    if (remain < 0) remain = 0;
                    float r = g_DrunkActive ? 1.0f  : 0.7f;
                    float gC = g_DrunkActive ? 0.2f : 0.1f;
                    float b = g_DrunkActive ? 0.7f  : 0.5f;
                    drawSlot(L"DRNK", remain, r, gC, b);
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
                    g_TextS.Draw(L"DEATH", x + 2.0f, baseY2 + 6.0f, 0.6f,
                                 1.0f, 0.4f, 0.4f, 1.0f);
                    wchar_t buf[8];
                    swprintf_s(buf, L"x%d", g_Stats.approachStacks);
                    float tw = g_TextL.Width(buf, 0.9f);
                    g_TextL.Draw(buf, x + (SLOT_W - tw) * 0.5f,
                                 baseY2 + SLOT_H * 0.40f, 0.9f, 1,1,1,0.95f);
                    ++slot;
                }
            }
            // ���� ������ ũ�� (�׻� �ֻ��?����) ����
            {
                GameState nextSt = st;

                // Ÿ��Ʋ�� �����?���?
                float barRatio = 0.0f, bR = 0.35f, bG = 0.72f, bB = 1.0f;
                bool inGame = (st == GameState::RUNNING || st == GameState::PAUSED ||
                               st == GameState::DYING   || st == GameState::AUG_SELECT ||
                               st == GameState::DEBUFF_SELECT || st == GameState::RUN_SHOP);
                if (inGame) {
                    float bossHp = -1.0f, bossMaxHp = 1.0f; int bossPick = -1;
                    if      (g_RRBoss    && g_RRBoss->alive)    { bossHp = g_RRBoss->hp;    bossMaxHp = g_RRBoss->maxHp;    bossPick = 2;  }
                    else if (g_CentiBoss && g_CentiBoss->alive)  { bossHp = g_CentiBoss->hp;  bossMaxHp = g_CentiBoss->maxHp;  bossPick = 8;  }
                    else if (g_TessBoss  && g_TessBoss->alive)   { bossHp = g_TessBoss->hp;   bossMaxHp = g_TessBoss->maxHp;   bossPick = 10; }
                    else if (g_EtherBoss && g_EtherBoss->alive)  { bossHp = g_EtherBoss->hp;  bossMaxHp = g_EtherBoss->maxHp;  bossPick = 20; }

                    if (bossPick >= 0 && bossMaxHp > 0.0f) {
                        barRatio = bossHp / bossMaxHp;
                        if (barRatio < 0.0f) barRatio = 0.0f;
                        auto c = BossDir::WarnColor(bossPick);
                        bR = c.x; bG = c.y; bB = c.z;
                    } else if (g_NextBossScore > 0) {
                        long long prev = g_NextBossScore - (long long)BossDir::MAIN_SCORE_STEP;
                        if (prev < 0LL) prev = 0LL;
                        barRatio = (float)(g_GameManager.score - prev) / BossDir::MAIN_SCORE_STEP;
                        if (barRatio < 0.0f) barRatio = 0.0f;
                        bR = 0.35f; bG = 0.72f; bB = 1.0f;
                    }
                }

                DrawBrowserChrome(sw, sh, st, mx, my, lmb, g_LmbPrev, nextSt, barRatio, bR, bG, bB);
                if (nextSt != st)
                    g_GameManager.currentState = nextSt;
            }

        }

        g_LmbPrev = lmb;

        // ?�적 ?�금 / ?�감 발견 발생 ???�??(게임 �?즉시 ?�구??
        if (g_AchSaveNeeded || g_CodexDirty) {
            SaveGame(); g_AchSaveNeeded = false; g_CodexDirty = false;
        }

        BatchFlush();   // ?�레??마�?�????��? ?�형 모두 그림
        glfwSwapBuffers(window);

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
    glfwTerminate();
    return 0;
}
