// Windows ?ㅻ뜑瑜?glad蹂대떎 癒쇱? ??APIENTRY 留ㅽ겕濡?以묐났 ?뺤쓽 諛⑹?
#ifdef _WIN32
  #include <windows.h>
  #include <dwmapi.h>   // DwmIsCompositionEnabled (吏꾨떒??
  #include <timeapi.h>  // timeBeginPeriod / timeEndPeriod (FPS 罹??뺣???
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
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstdarg>
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
#include "PolymorphBoss.h"
#include "BotnetBoss.h"
#include "CentipedeBoss.h"
#include "TotemBoss.h"
#include "UnknownBoss.h"
#include "BossDirector.h"
#include "RunIntermission.h"
#include "Codex.h"
#include "CollisionSystem.h"
#include "Augment.h"
#include "PlayerStats.h"
#include "TextRenderer.h"
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
#pragma comment(lib, "winmm.lib")      // timeBeginPeriod / timeEndPeriod (FPS 罹??뺣???
// dwmapi.lib ? WindowFx.cpp ?먯꽌 留곹겕
#endif

extern "C++" {
    __declspec(dllexport) DWORD NvOptimusEnablement = 0;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 0;
}

// ?щ챸???ы띁??WindowFx.h/cpp 濡??대룞
// (EnableWindowTransparency ? TransparencyLog 媛 ?숈씪 湲곕뒫)
#define DBG TransparencyLog

GameManager    g_GameManager;
MonsterManager g_MonsterManager;
std::vector<Bullet> g_Bullets;

// ?? 媛쒕컻???щ━?먯씠?곕툕) 紐⑤뱶 ???꾧컧 寃?됱갹 ?댁뒪?곗뿉洹몃줈留??닿툑 ??
//    異쒖떆 鍮뚮뱶???щ━?먯씠?곕툕 吏꾩엯?먯씠 ?④꺼???덇퀬, ?꾧컧 寃?됱뿉 ?쒗겕由?肄붾뱶瑜?//    ?낅젰?섎㈃ ?닿툑?섏뼱 ?쒖씠???붾㈃???좉????깆옣?쒕떎. (?쇰컲 ?뚮젅?댁뼱??紐?耳?
bool    g_DevUnlocked   = false;
float   g_DevToastTimer = 0.0f;            // ?닿툑 ?뺤씤 ?좎뒪??(珥?
// ?ㅼ젙李?蹂쇰ⅷ ?レ옄 吏곸젒?낅젰 ?곹깭
bool    g_VolEdit = false;
wchar_t g_VolBuf[8] = {0};
int     g_VolLen = 0;
PlayerStats    g_Stats;
bool g_aug1Released = true, g_aug2Released = true, g_aug3Released = true;
TextRenderer   g_TextL;   // ??湲??(利앷컯 ?대쫫, ?곹깭 ??댄?)
TextRenderer   g_TextS;   // ?묒? 湲??(?ㅻ챸, ?뚰듃)
TextRenderer   g_TextXL;  // 珥덈?????댄?(?쒖옉李?濡쒓퀬) ?꾩슜 ??怨좏빐?곷룄 ?섏뒪??
#ifdef _WIN32
HANDLE         g_FontMemHandle   = nullptr; // Dongle (?쒓뎅??
HANDLE         g_OswaldMemHandle = nullptr; // Oswald (?쇳떞/?ㅻ┫)
#endif

struct BrokenSightOrb {
    float x = 0, y = 0;
    float vx = 0, vy = 0;
    float wanderTimer = 0.0f;
    bool  active = false;
} g_Orb;

// ?ㅺ??ㅻ뒗 二쎌쓬 (?붾쾭?? ??二쎌? ?딄퀬 ?곸썝??異붽꺽?섎뒗 鍮④컙 ?ш컖??(?щ윭 媛?媛??
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

// ?ы깙 (CANNON + DRONE_2 議고빀)
//   1珥덈쭏???뚮젅?댁뼱 ?꾩튂??1媛쒖뵫 諛곗튂, 媛?5珥?吏????留듭뿉 ~5媛??곸떆
//   ?λ젰移섎뒗 '???媛 ?꾨땲??'?뚯킑' 湲곗?(g_TurretStats)?쇰줈 怨꾩궛
struct Turret {
    float x = 0.0f, y = 0.0f;
    float lifeTimer = 0.0f;   // 0?뭈URRET_LIFE
    float fireTimer = 0.0f;
};
static const int MAX_TURRETS = 8;                 // ?덉쟾 ?곹븳
// 李??ш린 ??g_Scale 濡??쒖옉 ???쇨큵 異뺤냼 媛?ν븯?꾨줉 ?고???媛?(constexpr ??蹂??
float TURRET_WIN_W  = 250.0f;
float TURRET_WIN_H  = 250.0f;
// ?좉퇋 蹂댁뒪 媛쒖씤 李??ш린 (蹂몄껜/HP 瑜?媛?먮뒗 ?곕씪?ㅻ땲??李?
float RR_WIN_W     = 600.0f;
float POLY_WIN_W   = 840.0f;
float BOTNET_WIN_W = 880.0f;   // C2_RELAY: 터미널 + 호스트 맵
float CENTI_WIN_W = 600.0f;    // FORK.worm: 본체 가짜 창(PID 체인 창 별도 렌더)
float TOTEM_WIN_W = 720.0f;    // RITE.CORE: 코어 + 기둥 의식 공간
float UNKNOWN_WIN_W = 500.0f;  // UNKNOWN.sys: 창연 검 보스
float UNKNOWN_WIN_H = 580.0f;
// 遊뉖꽬 ?몃뱶(SPAWNER) 媛쒖씤 ?묒? 李???怨좎젙 ???먭린 媛吏?李쎌쓣 ?꾩? (E21)
float SPAWNER_WIN_W = 300.0f;
float DDOS_WIN_W    = 210.0f;
// ?먭굅由?紐?FakeWindow ?ш린 (?뚮뜑/?대━??怨듭슜) ???쒖옉 ??g_Scale ?곸슜
float g_RfwW = 500.0f, g_RfwH = 500.0f;
static constexpr float TURRET_LIFE   = 5.0f;
static constexpr float TURRET_DEPLOY = 1.0f;
std::vector<Turret> g_Turrets;
float       g_TurretDeployTimer = 0.0f;
PlayerStats g_TurretStats;                        // 소총 기준 화력


struct ChakramState {
    float angle        = 0.0f;
    float hp           = 150.0f;
    float maxHp        = 150.0f;
    bool  alive        = false;
    float respawnTimer = 0.0f;
};
static const int   MAX_CHAKRAMS    = 3;
ChakramState g_Chakrams[MAX_CHAKRAMS] = {};
static const float CHAKRAM_RADIUS = 130.0f;   // 踰꾪봽: ?볦? 怨듭쟾 (怨듭쟾泥??붽꺽)
static const float CHAKRAM_SIZE   = 30.0f;    // 踰꾪봽: ??移쇰궇

float g_BulletRainTimer = 0.0f;

// 痍⑦븿 (?붾쾭?? ??20珥??ъ씠??以?5珥덇컙 ?쒕뜡 諛⑺뼢 ?ш꺽
float g_DrunkCycle  = 0.0f;
bool  g_DrunkActive = false;

// LIGHT_STEP ?쇨꺽 媛먯????댁쟾 HP 湲곕줉
float g_PrevHP = 100.0f;

// 珥덈떦 EXP ?꾩쟻??(?붾쾭?????ㅺ??ㅻ뒗 二쎌쓬, ?〓す 媛??
float g_XpTimeAccum = 0.0f;

// 寃뚯엫 ?쒖옉 ??寃쎄낵 ?쒓컙 (?먰룺蹂?蹂댁뒪 ?깆옣 ??대컢)
float g_GameTime         = 0.0f;
float g_BomberSpawnTimer = 0.0f;

// ??컻 異⑷꺽??(?먰룺蹂??먰룺 / 蹂댁뒪 ?ㅽ룿)
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

// 寃媛?洹쇱젒 ?ㅼ쐷 ?붿긽 (議곗? 諛⑺뼢 ??
struct SlashFx {
    float x, y, ang, range, life, maxLife;
    bool  active = false;
};
static const int MAX_SLASH = 8;
SlashFx g_Slashes[MAX_SLASH] = {};

// 沅곸닔 李⑥쭠 (0~1) ??LMB ?꾨Ⅸ 留뚰겮 異⑹쟾, ?쇰㈃ 諛쒖궗
float g_ArcherCharge = 0.0f;
static const float BOW_CHARGE_TIME = 0.9f;   // ?꾩땐源뚯? 珥?
// 癒몄쫹 ?뚮옒??(諛쒖궗 ?쒓컙 珥앷뎄 ?ш킅)
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

// ?붾㈃ ?붾뱾湲?(蹂댁뒪 ?ㅽ룿, 異⑷꺽??
float g_ShakeTime = 0.0f;
float g_ShakeMag  = 0.0f;

// 蹂댁뒪 蹂댁긽 ???⑥? 踰꾪봽 ????(?붾쾭???섏씠吏 skip)
int  g_BossRewardPicksLeft = 0;
// 蹂댁뒪 ?ㅽ룿 ???쇰컲 蹂댁뒪??20留뚯젏留덈떎, ?대━紐⑦봽??50留뚯젏 怨좎젙(1??
static constexpr long long FIRST_BOSS_SCORE = 20000;  // 시연용 (원래 50000)
long long g_NextBossScore  = FIRST_BOSS_SCORE;
bool      g_CreativeBossPending = false;
ReloadRunnerBoss* g_RRBoss = nullptr;
PolymorphBoss* g_PolyBoss = nullptr;
BotnetBoss* g_BotnetBoss = nullptr;

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
TotemBoss* g_TotemBoss = nullptr;
UnknownBoss* g_UnknownBoss = nullptr;
// ?? 諛곕뱶 ?뱁꽣 ?щ쭩 ?붾쪟臾????꾩떆 媛먯냽 援ъ뿭(?먯긽 ?곸뿭). ?덉뿉 ?덉쑝硫??대룞?띾룄 -10% ??
//   利됱떆 ?앷린吏 ?딄퀬 ZONE_OPEN(0.7珥???嫄몄퀜 ?먯젏 遺?앸릺???쇱쭚(grow factor = age/OPEN).
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
    float w = 300.0f, h = 300.0f;   // 踰붿쐞 +25% (240 ??300)
    g_SlowZones.push_back({ m->worldX - w*0.5f, m->worldY - h*0.5f, w, h, 5.0f, 5.0f, 0.0f });
}

// ?? ?ㅼ틪 ?덉씠? (利앷컯) ??二쇨린??愿??鍮?+ ?섏씠??鍮꾩＜????
struct LaserBeam { float ox, oy, ex, ey, life, maxLife; float width = 1.0f; };
std::vector<LaserBeam> g_LaserBeams;
float          g_LaserTimer = 0.0f;
constexpr float LASER_INT   = 0.85f;  // 諛쒖궗 二쇨린(珥? ???덊봽: 0.7

// ?? 諛깆떊 ?ㅼ틪 (利앷컯) ??二쇨린?곸쑝濡??뚮젅?댁뼱 二쇰????뺥솕 ?꾩뒪(踰붿쐞 ?쇱냼) ??
//   ?쒓컖 留곸? 湲곗〈 SpawnShockWave(?쎌갹 留? ?ъ궗??
float          g_NovaTimer  = 0.0f;
constexpr float NOVA_INT    = 2.4f;    // ?꾩뒪 二쇨린(珥? 以묒꺽 ???⑥텞)
constexpr float NOVA_R      = 240.0f;  // 湲곕낯 諛섍꼍(以묒꺽 ???뺣?)

// ?? 蹂댁뒪 ?깆옣 ?꾩“(利앹긽) ??????????????????????????????????????
//   蹂댁뒪 ?ㅽ룿??"寃곗젙 ??2.5珥??꾩“(?뚮쭏 利앹긽 + 寃쎄퀬 諛곕꼫) ???ㅼ젣 ?앹꽦" ?쇰줈 遺꾨━.
//   ?꾩“ ?숈븞 寃뚯엫?뚮젅?대뒗 怨꾩냽(?붾젅洹몃옒??. 留뚮즺 ??寃곗젙??蹂댁뒪瑜??ㅼ젣濡??앹꽦.
float          g_BossWarnTimer = 0.0f;          // >0 ?대㈃ ?꾩“ 吏꾪뻾 以?(?⑥? ?쒓컙)
constexpr float BOSS_WARN_DUR  = 2.5f;
int            g_BossWarnPick   = -1;           // 0~8 蹂댁뒪 ?듭씪 ?몃뜳??(4=?대━)
const wchar_t* g_BossWarnName   = L"";
float          g_BossWarnHp      = 0.0f;
// ?섏씠利? ?곸듅?ｌ? 異붿쟻 (?듭씪 吏꾩엯 ?곗텧 1???ъ깮??
bool g_RRWasP2 = false, g_RRWasP3 = false;
bool g_BotnetWasP2 = false;
// ?섏씠利? 吏꾩엯 ?좎뒪??("??怨쇰?????PHASE 2")
float     g_P2ToastTimer = 0.0f;
glm::vec3 g_P2ToastCol   = glm::vec3(1.0f);
int g_PolyPrevForm = -1;   // ??蹂??媛먯???(蹂?????뚰떚?? ??-1 = 誘몄큹湲고솕
bool  g_LastRunRecord   = false;  // 吏곸쟾 ?먯씠 ?좉린濡앹씠?덈뒗吏 (GAMEOVER ?쒖떆??
int   g_MetaStartAugs   = 0;      // 硫뷀? ?닿툑: ?쒖옉 臾대즺 利앷컯 ???잛닔

float g_WindowSizeCur = 0.0f;
float g_WinPrevHP     = -1.0f;    // 李?異뺤냼??HP 異붿쟻
float g_HurtVignette  = 0.0f;     // ?쇨꺽 鍮④컙 鍮꾨꽕???붿뿬
float g_HpBarPop      = 0.0f;     // ?곕굹鍮꾩떇 HP 寃뚯씠吏諛????쇨꺽 ???대떎媛 ?섏씠??珥?

// ?? ?≫떚釉??ㅽ궗 ?쒖뒪???????湲곕낯) + ?щ’ 3媛?利앷컯 ?띾뱷, 苑?李⑤㈃ 援먯껜) ??
float g_DashCd        = 0.0f;
float g_DashInvuln    = 0.0f;
float g_PostPickGrace = 0.0f;      // C14: 利앷컯 ????吏㏃? ?좎삁(臾댁쟻+諛쒖궗?듭젣)濡?蹂듦? ?
float g_TimeStopTimer = 0.0f;
float g_HyperFocusTimer = 0.0f;
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
    g_TimeStopTimer = 0; g_HyperFocusTimer = 0;
}
// 蹂댁뒪 ?앹〈 ?숈븞 ?붾㈃ ?꾩껜瑜?蹂댁뒪 怨좎쑀?됱쑝濡??먯젏 臾쇰뱾?대뒗 ?곗텧
float     g_BossTintT   = 0.0f;                     // 0..1 (?앹〈 ???곸듅, ?щ쭩 ???섍컯)
glm::vec3 g_BossTintCol = glm::vec3(0.6f, 0.3f, 1.0f);

// HUD: ?꾩옱 痢≪젙 FPS (?곷떒 ?곗륫 ?쒖떆)
int    g_CurrentFPS  = 0;
double g_FpsLastTime = 0.0;
int    g_FpsFrames   = 0;

// 보유 증강 인덱스 목록 (선택 순서대로, 중복 스택 가능)
std::vector<int> g_OwnedAugs;

// 크리에이티브 모드 시작 증강 직접 선택 (인덱스). 게임 시작 시 일괄 적용.
std::vector<int> g_CreativeStartAugList;
bool g_CreativeStartPending = false;   // ?곸슜 ?湲?(main 猷⑦봽媛 applyByIdx 濡?泥섎━)

// 利앷컯 ?좏깮 hover state (-1 = 誘몄꽑?? 0/1/2 = 移대뱶 ?몃뜳??
int  g_HoveredAug    = -1;
bool g_EnterReleased = true;

// 留덉슦???대┃ edge 媛먯? (?댁쟾 ?꾨젅??left button ?곹깭)
bool g_LmbPrev = false;

// PAUSED ?곹깭 ??蹂댁쑀 利앷컯 ?대┃ ???ㅻ챸 ?쒖떆 (-1 = ?놁쓬, 0..AUG_TOTAL-1 = ?몃뜳??
int g_PauseSelectedAug = -1;

// ?ㅼ젙 ?붾㈃ 吏꾩엯 ???댁쟾 ?곹깭 (?ㅻ줈 媛湲???蹂듦?)
GameState g_SettingsReturnTo = GameState::MAIN_MENU;

int g_WeaponChoices[3] = {0, 1, 2};

int g_CurrentWeapon = -1;

// 蹂??移대뱶 ??AUG_SELECT ??25% ?뺣쪧濡?4踰덉㎏ 移대뱶 ?깆옣
// ?꾩옱 臾닿린瑜??ㅻⅨ StartWeapon ?쇰줈 ?꾪솚 (湲곗〈 臾닿린 ?④낵 ?쒓굅 ????臾닿린 ?곸슜)
// 媛?= ?꾪솚??StartWeapon ?몃뜳?? -1 = ?대쾲 ?쇱슫?쒕뒗 蹂??移대뱶 ?놁쓬
int g_ConversionWeapon = -1;

// DYING ?щ쭩 ?곗텧 ?곹깭
float g_DyingTimer    = 0.0f;
bool  g_DeathBoomDone = false;
float g_DeathWinW0    = 0.0f;
float g_GameOverFade  = 0.0f;    // 寃뚯엫?ㅻ쾭 硫붾돱 ?섏씠?쒖씤 (0??, ??ㅽ겕由???2.5珥?
// ?щ쭩 ?곗텧 = 利됱떆 ???컻(?뚰떚?? ????컻 ?ы뙆 ??GAMEOVER (以??쒕꽕留덊떛 ?놁쓬, ?⑥닚)
static const float DYING_DUR      = 0.8f;   // ??컻 ?ы뙆媛 ?ъ깮?섎뒗 ?쒓컙 (?섏씠???꾧퉴吏)
static const float GAMEOVER_FADE  = 2.5f;   // 硫붾돱 100% 源뚯? 嫄몃━???쒓컙

struct DeathParticle {
    float x, y, vx, vy;
    float size;       // ??蹂 湲몄씠(px)
    float r, g, b;    // ?됱긽
    bool  active = false;
};
static const int MAX_DEBRIS = 48;
DeathParticle g_Debris[MAX_DEBRIS] = {};
float g_DeathCX = 0, g_DeathCY = 0;
float g_DeathFlash = 0.0f; // ??컻 ?ш킅 (1.0 ??0.0)
wchar_t g_DeathReason[96] = {0};   // ?щ쭩 ?먯씤 ("?뗢뿃 ???섑빐 醫낅즺??)

static FakeWindow* s_PlayerWinRef = nullptr;

static void ApplyPurchasedAug(int idx, FakeWindow& playerWin, int scrW, int scrH) {
    AugType atype = ALL_AUGS[idx].type;
    float oldWS   = playerWin.width;
    float oldPCX  = playerWin.x + playerWin.width  * 0.5f;
    float oldPCY  = playerWin.y + playerWin.height * 0.5f;

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

    if (atype == AugType::CHAKRAM || atype == AugType::CHAKRAM_2 ||
        atype == AugType::CHAKRAM_3 || atype == AugType::CHAKRAM_SINGULARITY) {
        for (int c = 0; c < g_Stats.chakramCount && c < MAX_CHAKRAMS; c++) {
            if (!g_Chakrams[c].alive && g_Chakrams[c].respawnTimer <= 0) {
                g_Chakrams[c].alive        = true;
                g_Chakrams[c].hp           = 150.0f;
                g_Chakrams[c].maxHp        = 150.0f;
                g_Chakrams[c].respawnTimer = 0.0f;
            }
            g_Chakrams[c].angle =
                (float)c / (float)g_Stats.chakramCount * 6.2831853f;
        }
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
    if (g_Stats.windowSize != oldWS) {
        playerWin.width  = g_Stats.windowSize;
        playerWin.height = g_Stats.windowSize;
        playerWin.x = oldPCX - g_Stats.windowSize * 0.5f;
        playerWin.y = oldPCY - g_Stats.windowSize * 0.5f;
    }
}

void RunShopPurchase(int slot) {
    if (!s_PlayerWinRef || slot < 0 || slot >= 4) return;
    int idx = g_RunShopStock[slot];
    if (idx < 0) return;
    int price = g_RunShopPrice[slot];
    if (g_RunGold < (long long)price) return;
    g_RunGold -= price;
    ApplyPurchasedAug(idx, *s_PlayerWinRef, g_GameManager.screenW, g_GameManager.screenH);
    g_RunShopStock[slot] = -1;
    g_RunShopPrice[slot]  = 0;
}

static bool BossFightBusy() {
    return (g_RRBoss && g_RRBoss->alive)
        || (g_PolyBoss && g_PolyBoss->alive)
        || (g_BotnetBoss && g_BotnetBoss->alive)
        || (g_CentiBoss && g_CentiBoss->alive)
        || (g_TotemBoss && g_TotemBoss->alive)
        || (g_UnknownBoss && g_UnknownBoss->alive)
        || g_BossWarnTimer > 0.0f;
}

static void StartBossWarn(int pick, const wchar_t* name, float hp) {
    BossDir::SetActTheme(pick);
    g_BossWarnPick = pick;
    g_BossWarnName = name;
    g_BossWarnHp   = hp;
    g_BossWarnTimer = g_CreativeMode ? 0.35f : BOSS_WARN_DUR;
}

static void QueueCreativeBossPick(int pick, float bossHpC, float polyHpC) {
    switch (pick) {
    case 1: StartBossWarn(1, L"UNKNOWN.sys",  bossHpC * 0.7f);  break;
    case 2: StartBossWarn(2, L"VOLLEY.sys",   bossHpC);         break;
    case 4: StartBossWarn(4, L"GLITCH.exe",   polyHpC);         break;
    case 7: StartBossWarn(7, L"C2_RELAY.sys", bossHpC * 0.75f); break;
    case 8: StartBossWarn(8, L"FORK.worm",     bossHpC * 0.7f);  break;
    case 9: StartBossWarn(9, L"RITE.CORE",     bossHpC * 0.72f); break;
    default: StartBossWarn(2, L"VOLLEY.sys",   bossHpC);         break;
    }
}

int main() {
    CrashHandler::Install();   // 媛뺤쥌(E23) 異붿쟻 ??泥섎━ ?????덉쇅 ??濡쒓렇+誘몃땲?ㅽ봽
    srand((unsigned)time(NULL));

    // ?ㅽ뻾 ?뚯씪 ?대뜑濡??묒뾽 ?붾젆?곕━ ?대룞 (Resource/ ?곷?寃쎈줈 濡쒕뱶 蹂댁옣)
    PlatformChdirToExeDir();
    LoadGame();   // ??λ맂 ?ㅼ젙/湲곕줉 遺덈윭?ㅺ린 (?놁쑝硫?湲곕낯媛??좎?)
    ApplyAccentTheme();   // ??λ맂 ?≪꽱???뚮쭏 ??g_Accent* 諛섏쁺

    if (!glfwInit()) return -1;
    // Sleep ?댁긽??1ms 濡?(FPS 罹??뺣??꾩슜). Windows 留??섎? ?덉쓬
    PlatformTimerBegin();

    GLFWmonitor*       monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode    = glfwGetVideoMode(monitor);
    screenWidth  = mode->width;
    screenHeight = mode->height - 1; // ??DirectFlip ?뚰뵾: ?붾㈃蹂대떎 1px ?묎쾶

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,        GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    // macOS ??3.2+ Core ?먯꽌 forward-compatible 而⑦뀓?ㅽ듃 ?꾩닔
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    // ?덊떚??HiDPI) 2횞 諛깅쾭???꾧린 ???꾨젅?꾨쾭??= 李??ш린(?? 1:1
    //   醫뚰몴/?ㅽ겕issor/留덉슦?ㅺ? ?꾨? ???⑥쐞濡??쇱튂 ??Windows ? ?숈씪?섍쾶 ?숈옉
    //   (???꾨㈃ 寃뚯엫???붾㈃ 醫뚰븯??1/4 ?먮쭔 洹몃젮吏怨??대┃ ?꾩튂媛 ?닿툔??
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
#endif
    glfwWindowHint(GLFW_DECORATED,             GLFW_FALSE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    // GLFW_FLOATING ?쒓굅: WS_EX_TOPMOST + ?꾩껜?붾㈃ ?ш린 議고빀??DWM ?뚮┰ 紐⑤뱶瑜??좊컻
    // ??DWM 而댄룷吏???고쉶 ???뚰뙆 ?щ챸??臾댄슚?? TOPMOST ?놁씠 ?앹꽦 ???섎룞?쇰줈 ?ㅼ젙.
    glfwWindowHint(GLFW_RESIZABLE,             GLFW_FALSE);
    glfwWindowHint(GLFW_ALPHA_BITS,            8);

    // ??screenHeight 媛 ?대? mode->height-1 (?꾩뿉??DirectFlip ?뚰뵾??
    //   ?붾㈃ ?뺥솗??媛숈? ?ш린濡??앹꽦?섎㈃ DWM ??DirectFlip ?쇰줈 而댄룷吏???고쉶
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight,
                                          "Onedow", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }

    glfwSetWindowPos(window, 0, 0);

    // ?섎떒 ?묒뾽?쒖떆以??믪씠 怨꾩궛 ????ㅽ겕由??꾩뿉 ???덈뒗 ?묒뾽?쒖떆以꾩뿉 ?섎떒 UI 媛
    //   媛?ㅼ?吏 ?딅룄濡? (?묒뾽 ?곸뿭???붾㈃蹂대떎 ?묒쑝硫?洹?李⑥씠媛 ?묒뾽?쒖떆以??믪씠)
#ifdef _WIN32
    {
        int fullH = GetSystemMetrics(SM_CYSCREEN);
        RECT wa;
        if (SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0)) {
            int bottomGap = fullH - (int)wa.bottom;   // ?섎떒 ?묒뾽?쒖떆以??믪씠 (洹????꾩튂硫?0)
            if (bottomGap > 0 && bottomGap < 120) g_TaskbarH = bottomGap;
        }
    }
#endif

    // ?댁긽??湲곗? ?ㅼ??????묒? ?붾㈃?먯꽌 李??뷀떚?곌? 鍮꾨? 異뺤냼?섎룄濡?怨꾩궛 ???쇨큵 ?곸슜.
    //   (?덈? ?쎌??대씪 ?묒? ?붾㈃?쇱닔濡??곷??곸쑝濡?而몃뜕 臾몄젣 ?닿껐 + ?꾩껜?곸쑝濡?李?異뺤냼)
    g_Scale = (float)screenHeight / SCALE_REF_H;
    if (g_Scale > 1.0f) g_Scale = 1.0f;
    if (g_Scale < 0.5f) g_Scale = 0.5f;
    TURRET_WIN_W *= g_Scale; TURRET_WIN_H *= g_Scale;
    RR_WIN_W *= g_Scale; POLY_WIN_W *= g_Scale; BOTNET_WIN_W *= g_Scale;
    CENTI_WIN_W *= g_Scale;
    TOTEM_WIN_W *= g_Scale;
    UNKNOWN_WIN_W *= g_Scale; UNKNOWN_WIN_H *= g_Scale;
    SPAWNER_WIN_W *= g_Scale;
    DDOS_WIN_W    *= g_Scale;
    g_RfwW *= g_Scale; g_RfwH *= g_Scale;
    glfwMakeContextCurrent(window);
    InputRegisterCallbacks(window);
    glfwSwapInterval((g_FpsCap == 0) ? 1 : 0);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    // ?고듃 珥덇린????Jua(?쒓?)/KosugiMaru(?쇰낯??/Oswald(?쇳떞) ?대갚 泥댁씤.
    //   ???고듃瑜???긽 ?숈떆 濡쒕뱶 ???몄뼱? 臾닿??섍쾶 紐⑤뱺 湲由ы봽(?????? ?쒖떆.
#ifdef _WIN32
    // EXE ?꾨쿋?붾뱶 RCDATA 諛붿씠?몃? stb_truetype ?쇰줈 吏곸젒 ?섏뒪?고솕
    auto loadRc = [&](const char* resName, int& outSize) -> const unsigned char* {
        HMODULE hMod = GetModuleHandleA(nullptr);
        HRSRC   hRes = FindResourceA(hMod, resName, (LPCSTR)RT_RCDATA);
        if (!hRes) { outSize = 0; return nullptr; }
        outSize = (int)SizeofResource(hMod, hRes);
        HGLOBAL hData = LoadResource(hMod, hRes);
        return (const unsigned char*)LockResource(hData);
    };
    int szJ = 0, szK = 0, szO = 0;
    const unsigned char* datas[3] = {
        loadRc("JUA_FONT", szJ), loadRc("KOSUGI_FONT", szK), loadRc("OSWALD_FONT", szO)
    };
    int sizes[3] = { szJ, szK, szO };
    g_TextL.InitFromMemory(datas, sizes, 3, 36, screenWidth, screenHeight);
    g_TextS.InitFromMemory(datas, sizes, 3, 22, screenWidth, screenHeight);
    g_TextXL.InitFromMemory(datas, sizes, 3, 100, screenWidth, screenHeight);  // 濡쒓퀬 怨좏빐?곷룄
#else
    // macOS/Linux: ?붿뒪?ъ쓽 TTF ?대갚 泥댁씤
    {
        const char* chain[3];
        int nc = LanguageFontChain(g_Language, chain);
        g_TextL.InitFromFiles(chain, nc, 36, screenWidth, screenHeight);
        g_TextS.InitFromFiles(chain, nc, 22, screenWidth, screenHeight);
        g_TextXL.InitFromFiles(chain, nc, 100, screenWidth, screenHeight);  // 濡쒓퀬 怨좏빐?곷룄
    }
#endif

    BatchFlush(); glEnable(GL_BLEND);
    // RGB: ?쒖? ?뚰뙆釉붾젋??/ Alpha: ?꾨젅?꾨쾭???뚰뙆媛??щ컮瑜닿쾶 ?꾩쟻
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                        GL_ONE,       GL_ONE_MINUS_SRC_ALPHA);

    // ?꾩씠肄??쏀넗洹몃옩) ?띿뒪泥??뚯씠?꾨씪??+ 利앷컯 ?꾩씠肄?濡쒕뱶
    InitIconGL();
    LoadIcons();
    InitRainMissileTex();   // ?꾪솚 ?몃? 濡쒖폆 ?ㅽ봽?쇱씠???곕같寃??쒓굅+?댄듃 媛??

    EnableWindowTransparency(window);

    // --- 李?GL/?덉??ㅽ듃由?醫낇빀 吏꾨떒 (Windows ?꾩슜) ---
#ifdef _WIN32
    {
        HWND hWnd = glfwGetWin32Window(window);
        // GL 3.3 Core Profile: GL_ALPHA_BITS deprecated ??glGetFramebufferAttachmentParameteriv ?ъ슜
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
        // AMD/ATI 媛먯? ??AMD ?쒕씪?대쾭??GLSL 而댄뙆?쇱씠 ???꾧꺽?섍퀬 ?щ챸 FBO 泥섎━媛
        //   NVIDIA ? ?щ씪, 踰ㅻ뜑瑜?濡쒓렇濡??④꺼 寃??붾㈃/?щ챸?ㅽ뙣 ?먯씤 異붿쟻???ъ슜.
        bool isAMD = false;
        if (vendor) {
            std::string vlow = vendor;
            for (auto& ch : vlow) ch = (char)tolower((unsigned char)ch);
            isAMD = (vlow.find("amd") != std::string::npos) ||
                    (vlow.find("ati") != std::string::npos) ||
                    (vlow.find("advanced micro") != std::string::npos);
        }

        // Windows ?щ챸???④낵 ?ㅼ젙 (?덉??ㅽ듃由?
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

        DBG("=== 醫낇빀 吏꾨떒 ===\n");
        DBG("  [GL] Vendor           : %s%s\n", vendor ? vendor : "NULL",
            isAMD ? "  (AMD 媛먯? ???꾧꺽 GLSL/?щ챸 FBO 寃쎈줈)" : "");
        DBG("  [GL] Renderer         : %s\n", renderer ? renderer : "NULL");
        DBG("  [GL] Version          : %s\n", glVer    ? glVer    : "NULL");
        DBG("  [GL] Framebuffer bits : R=%d G=%d B=%d A=%d\n",
            rBits, gBits, bBits, alphaBits);
        DBG("       ??(Core Profile ?뺥솗??荑쇰━. A=0?대㈃ ?쒕씪?대쾭媛 ?뚰뙆 梨꾨꼸 嫄곕?)\n");
        DBG("\n");
        DBG("  [GLFW] transparent_framebuffer : %d [%s]\n",
            trans, trans ? "SUPPORTED" : "NOT SUPPORTED");
        DBG("  [DWM]  Composition enabled     : %s\n", dwmOn ? "YES" : "NO");
        DBG("  [Win]  GWL_EXSTYLE             : 0x%08lX\n", ex);
        DBG("  [Win]  WS_EX_LAYERED           : %s\n",
            (ex & WS_EX_LAYERED)     ? "SET"           : "NOT SET");
        DBG("  [Win]  WS_EX_TRANSPARENT       : %s\n",
            (ex & WS_EX_TRANSPARENT) ? "SET (?대┃?듦낵!)" : "NOT SET (?뺤긽)");
        DBG("\n");
        DBG("  [REG]  EnableTransparency      : %lu [%s]\n",
            enableTrans,
            enableTrans ? "ON (?뺤긽)"
                        : "OFF ???닿쾶 臾몄젣! ?ㅼ젙>媛쒖씤?ㅼ젙>???щ챸???④낵 耳쒓린");
        DBG("=================\n\n");
    }
#endif // _WIN32 (吏꾨떒 釉붾줉)

    InitMainShaderPipeline(screenWidth, screenHeight);
    InitMainBatchGeometry(screenWidth, screenHeight);

    // --- 寃뚯엫 ?ㅻ툕?앺듃 珥덇린??---
    FakeWindow playerWin(0, "Onedow",
        (screenWidth  - g_Stats.windowSize) * 0.5f,
        (screenHeight - g_Stats.windowSize) * 0.5f,
        g_Stats.windowSize, g_Stats.windowSize);
    playerWin.isFocused = true;

    const float PLAYER_SIZE = 25.0f;
    const float MOVE_SPEED  = 400.0f;

    g_GameManager.Init(screenWidth, screenHeight);

    float lastFrame        = 0.0f;
    float spawnTimer       = 0.0f;
    float rangedSpawnTimer = 0.0f;
    float fireTimer        = g_Stats.fireInterval;  // ready to fire immediately
    float accumulator      = 0.0f;
    const float FIXED_DT = 1.0f / 60.0f;

    Audio::Init();   // ?ъ슫???쒖뒪??(Sounds/ ?대뜑, ?뚯씪 ?놁쑝硫?臾댁쓬)

    // 理쒓렐???????먮룞議곗?쨌?좊룄???꾪솚?몃?)쨌?쒕줎쨌?ы깙 怨듭슜
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
        if (g_PolyBoss && g_PolyBoss->alive && g_PolyBoss->damageable())
            consider(g_PolyBoss->worldX, g_PolyBoss->worldY);
        if (g_BotnetBoss && g_BotnetBoss->alive) {
            consider(g_BotnetBoss->worldX, g_BotnetBoss->worldY);
            for (auto& mn : g_BotnetBoss->minions) if (mn.alive) consider(mn.x, mn.y);
        }
        if (g_CentiBoss && g_CentiBoss->alive) {
            if (g_CentiBoss->vulnerable())
                consider(g_CentiBoss->worldX, g_CentiBoss->worldY);
            for (auto& mb : g_CentiBoss->minis) if (mb.alive) consider(mb.x, mb.y);
        }
        if (g_TotemBoss && g_TotemBoss->alive) {
            if (g_TotemBoss->vulnerable())
                consider(g_TotemBoss->worldX, g_TotemBoss->worldY);
            for (int ti = 0; ti < TotemBoss::N_TOTEM; ti++) {
                auto& tt = g_TotemBoss->totems[ti];
                if (tt.alive) consider(tt.x, tt.y);
            }
        }
        if (g_UnknownBoss && g_UnknownBoss->alive)
            consider(g_UnknownBoss->worldX, g_UnknownBoss->worldY);
        return found;
    };

    // ============================================================
    // 硫붿씤 猷⑦봽
    // ============================================================
    while (!glfwWindowShouldClose(window)) {
        s_PlayerWinRef = &playerWin;
        float now   = (float)glfwGetTime();
        float delta = now - lastFrame;
        if (delta > 0.1f) delta = 0.1f;
        lastFrame = now;

        // ?щ옒??異붿쟻 釉뚮젅?쒗겕????留덉?留??곹깭瑜??④꺼 媛뺤쥌(E23) ??濡쒓렇濡??꾩튂 ?뱀젙
        {
            const char* bn = g_RRBoss ? "reload" :
                             g_PolyBoss ? "poly" :
                             g_BotnetBoss ? "botnet" : g_CentiBoss ? "centi" :
                             g_TotemBoss ? "totem" : "none";
            char bc[200];
            std::snprintf(bc, sizeof(bc),
                "st=%d score=%lld lv=%d mobs=%u boss=%s",
                (int)g_GameManager.currentState,
                (long long)g_GameManager.score,
                g_GameManager.playerLevel,
                (unsigned)g_MonsterManager.monsters.size(), bn);
            CrashHandler::SetBreadcrumb(bc);
        }

        // 李??ъ빱?ㅻ? ?껋쑝硫??ㅻⅨ ?깆쑝濡??꾪솚) ?먮룞 ?쇱떆?뺤? ??        //   ?ㅻ쾭?덉씠???ъ빱???놁뼱??猷⑦봽媛 怨꾩냽 ?꾨?濡? ??留됱쑝硫?寃뚯엫??        //   諛깃렇?쇱슫?쒖뿉??怨꾩냽 吏꾪뻾??肄ㅻ낫 3珥?李쎌씠 ?섎윭媛 由ъ뀑?섎뒗 踰꾧렇 ??.
        {
            static bool s_wasFocused = true;
            bool focused = glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0;
            if (!focused && s_wasFocused &&
                g_GameManager.currentState == GameState::RUNNING)
                g_GameManager.currentState = GameState::PAUSED;
            s_wasFocused = focused;
        }

        // FPS 痢≪젙 (1珥??⑥쐞)
        ++g_FpsFrames;
        if (now - g_FpsLastTime >= 1.0f) {
            g_CurrentFPS  = g_FpsFrames;
            g_FpsFrames   = 0;
            g_FpsLastTime = now;
        }

        glfwPollEvents();

        // 以?遺?쒕읇寃?蹂닿컙 (?섏씠利? ?붾㈃ ?뺤옣 ??
        //   ?꾪닾/?꾪닾?ㅻ쾭?덉씠(RUNNING/DYING/PAUSED/利앷컯쨌?붾쾭???좏깮)?먯꽑 以??좎? ??        //   ?쇱떆?뺤? ??以뚯씤?섎뒗 寃?遺?먯뿰?ㅻ읇?ㅻ뒗 ?쇰뱶諛? 以??댁젣???쒖옉李???        //   '吏꾩쭨 硫붾돱'濡??섍컝 ?뚮쭔(蹂댁뒪 泥섏튂 ?쒖뿏 polyDeath 媛 target=1 濡?遺?쒕읇寃?蹂듭썝).
        {
            GameState zs = g_GameManager.currentState;
            bool inFight = (zs == GameState::RUNNING || zs == GameState::DYING ||
                            zs == GameState::PAUSED  || zs == GameState::AUG_SELECT ||
                            zs == GameState::DEBUFF_SELECT ||
                            zs == GameState::RUN_SHOP ||
                            (zs == GameState::RUNNING && g_InBossIntermission));
            if (!inFight) { g_ViewZoom = g_ViewZoomTarget = 1.0f; }
            else {
                // ?먯닔 鍮꾨? 以뚯븘?????꾩옣???쒖꽌???볦뼱吏?(?곹븳 1.4諛?= zoom 0.714).
                //   200만 점에서 최대치 도달.
                float t = std::min(1.0f, (float)g_GameManager.score / 2000000.0f);
                g_ViewZoomTarget = 1.0f - 0.286f * t;   // 1.0 → 0.714
            }
        }
        g_ViewZoom += (g_ViewZoomTarget - g_ViewZoom) * std::min(1.0f, delta * 4.0f);

        // ?뺤옣 ?꾨젅????以뚯븘?껊맂 留뚰겮 蹂댁씠???곸뿭???볦뼱吏誘濡??뷀떚??諛고쉶/?ㅽ룿 ?곸뿭???뺤옣
        //   (?먯닔 以뚯븘?꺜룻뤃由щえ??以뚯븘??怨듯넻 泥섎━)
        {
            float zb = (g_ViewZoom < 0.01f) ? 0.01f : g_ViewZoom;
            g_ArenaExX = (float)screenWidth  * 0.5f * (1.0f / zb - 1.0f);
            g_ArenaExY = (float)screenHeight * 0.5f * (1.0f / zb - 1.0f);
        }

        // 留덉슦???곹깭 (mx,my = ?붾㈃ ?쎌? / wmx,wmy = 以?蹂댁젙???붾뱶 醫뚰몴 = 議곗???
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        bool lmb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        float wmx = ScreenToWorldX((float)mx);
        float wmy = ScreenToWorldY((float)my);

        // --- ?낅젰 泥섎━ ---
        GameState prevState = g_GameManager.currentState;
        g_GameManager.HandleInput(window);

        // ??寃뚯엫 由ъ뀑 ?뚮떎 (GAMEOVER ??READY, ?쒖씠???좏깮 ?? ?ㅼ떆?섍린 踰꾪듉 ?깆뿉???몄텧)
        auto ResetForNewGame = [&]() {
            g_Stats        = PlayerStats();
            g_MetaStartAugs = ApplyMeta(g_Stats);    // 硫뷀? ?곴뎄 ?낃렇?덉씠???곸슜
            g_Stats.windowSize *= g_Scale;           // ?뚮젅?댁뼱 李????댁긽??鍮꾨? 異뺤냼
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
            g_BomberSpawnTimer = 0.0f;
            for (int i = 0; i < MAX_SHOCKS; i++) g_ShockWaves[i].active = false;
            for (int i = 0; i < MAX_SLASH; i++) g_Slashes[i].active = false;
            g_MuzzleTimer = 0.0f;
            g_ArcherCharge = 0.0f;
            g_ShakeTime = 0.0f; g_ShakeMag = 0.0f;
            g_NextBossScore = FIRST_BOSS_SCORE;
            BossDir::ResetRotation();
            BossDir::ResetAct();
            g_RunGold           = 0;
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
            g_RRWasP2 = g_RRWasP3 = g_BotnetWasP2 = false;
            g_LaserBeams.clear(); g_LaserTimer = 0.0f;
            g_SlowZones.clear(); g_BadSectorBleed = 0.0f;
            g_NovaTimer = 0.0f;
            g_RunMelee = false; g_RunBow = false;
            if (g_RRBoss)     { delete g_RRBoss;     g_RRBoss     = nullptr; }
            if (g_PolyBoss)   { delete g_PolyBoss;   g_PolyBoss   = nullptr; }
            if (g_BotnetBoss) { delete g_BotnetBoss; g_BotnetBoss = nullptr; }
            if (g_CentiBoss) { delete g_CentiBoss; g_CentiBoss = nullptr; }
            if (g_TotemBoss) { delete g_TotemBoss; g_TotemBoss = nullptr; }
            if (g_UnknownBoss) { delete g_UnknownBoss; g_UnknownBoss = nullptr; }
            g_PolyPrevForm = -1;
            g_BossTintT = 0.0f;
            ResetJuice();
            ResetSkills();
            g_WindowSizeCur = 0.0f; g_WinPrevHP = -1.0f; g_HurtVignette = 0.0f; g_HpBarPop = 0.0f;
            g_ViewZoom = g_ViewZoomTarget = 1.0f;   // 以??먮났
            g_ZoomCX = g_ZoomCY = 0.0f;
            rangedSpawnTimer = GetDifficultyParams(g_Difficulty).rangedSpawnInitialDelay;
            spawnTimer        = 0.0f;
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
            // GameManager ?꾨뱶??吏곸젒 由ъ뀑
            g_GameManager.playerHP    = 100.0f;
            g_GameManager.maxHP       = 100.0f;
            g_GameManager.score       = 0;
            g_GameManager.scoreAccum  = 0.0f;
            if (g_CreativeMode) {
                g_GameManager.score      = g_CreativeStartScore;
                g_GameManager.scoreAccum = (float)g_CreativeStartScore;
                // ?쒖옉 ?먯닔蹂대떎 ??泥?20留?諛곗닔遺???쇰컲 蹂댁뒪 (?쒓볼踰덉뿉 ?잛븘吏?諛⑹?)
                g_NextBossScore = ((g_CreativeStartScore / 200000) + 1) * 200000;
                // ?대? 50留??댁긽?먯꽌 ?쒖옉?섎㈃ ?대━紐⑦봽 ?먮룞?깆옣 ?앸왂 (蹂댁뒪?좏깮?쇰줈 ?뚰솚)
                // ?좏깮 蹂댁뒪 利됱떆 ?ㅽ룿 ?덉빟 (?먯닔 臾닿?)
                g_CreativeBossPending = (g_CreativeBossPick >= 0);
            }
            g_GameManager.xp          = 0;
            g_GameManager.playerLevel = 1;
            memset(g_GameManager.takenOnce, 0, sizeof(g_GameManager.takenOnce));
            g_Bullets.clear();
            g_MonsterManager.Clear();
            // UpdateStateSystem ??以묐났 reset ?뚰뵾
            g_GameManager.lastState = GameState::READY;
        };

        // GAMEOVER?뭃EADY ?먮룞 媛먯? (ESC ?깆쑝濡?吏곸젒 ?꾪솚??寃쎌슦)
        if (prevState == GameState::GAMEOVER &&
            g_GameManager.currentState == GameState::READY) {
            ResetForNewGame();
        }
        // PAUSED ???ㅻⅨ ?곹깭: 利앷컯 ?ㅻ챸 諛뺤뒪 ?リ린
        if (prevState == GameState::PAUSED &&
            g_GameManager.currentState != GameState::PAUSED) {
            g_PauseSelectedAug = -1;
        }
        g_GameManager.UpdateStateSystem(g_MonsterManager, g_Bullets);

        // ?? BGM ??寃뚯엫?뚮젅??以묒뿏 硫붿씤 猷⑦봽, 硫붾돱?먯꽑 ?뺤? (蹂댁뒪 BGM ? ?뚯씪 ?앷린硫??뺤옣) ??
        {
            Audio::SetEnabled(g_SoundVol > 0);            // 0 = ?꾧린
            Audio::SetVolume(g_SoundVol / 100.0f);        // 留덉뒪??蹂쇰ⅷ
            GameState cs = g_GameManager.currentState;
            bool bgmOn = (cs == GameState::RUNNING || cs == GameState::PAUSED ||
                          cs == GameState::AUG_SELECT || cs == GameState::DEBUFF_SELECT ||
                          cs == GameState::READY);
            (void)bgmOn; Audio::StopBgm();   // BGM ??"?곗슦??) ?쒓굅 ????긽 ?뺤?
        }

        // ?щ━?먯씠?곕툕 臾댁쟻 ??留??꾨젅??泥대젰 ? 怨좎젙 (?덈? 二쎌? ?딆쓬)
        if (g_CreativeGodmode && g_CreativeMode &&
            g_GameManager.currentState == GameState::RUNNING) {
            g_GameManager.playerHP = g_Stats.maxHP;
        }

        // HP 0 ??MK2 遺??OR DYING (1珥??щ줈??紐⑥뀡 ??GAMEOVER)
        if (g_GameManager.playerHP <= 0.0f &&
            g_GameManager.currentState == GameState::RUNNING) {
            // MK2: 1??遺????怨듦꺽??鍮꾨? ??컻 + ? HP (?섎꼸???놁쓬)
            if (g_Stats.mk2 && !g_Stats.mk2Used) {
                g_Stats.mk2Used = true;
                float pCX = playerWin.x + playerWin.width  * 0.5f;
                float pCY = playerWin.y + playerWin.height * 0.5f;
                float blastDmg = g_Stats.GetBaseDamage()
                               * g_Stats.GetDamageMultiplier(0.0f) * 6.0f;
                float blastRad = 280.0f;
                // 二쇰? ?곸뿉寃??곕?吏
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
                // 시각 효과 — 폭발 + 충격파 + 화면 흔들기
                SpawnEnemyExplosion(pCX, pCY, 1.0f, 0.8f, 0.3f, true);
                SpawnEnemyExplosion(pCX, pCY, 0.4f, 1.0f, 0.8f, true);
                SpawnShockWave(pCX, pCY, blastRad * 1.4f, 0.55f,
                               1.0f, 0.85f, 0.3f);
                g_ShakeTime = 0.45f; g_ShakeMag = 22.0f;
                TriggerFlash(1.0f, 0.9f, 0.4f, 0.8f);   // 遺????媛뺥븳 踰덉찉
                TriggerHitStop(0.14f);                  // ?꾪뙥???뺤?

                // 遺?????λ젰移??섎꼸???놁씠 ? HP 濡?(?붾쾭???놁쓬)
                g_GameManager.maxHP    = g_Stats.maxHP;
                g_GameManager.playerHP = g_Stats.maxHP;
                g_PrevHP               = g_GameManager.playerHP;
                // RUNNING ?곹깭 ?좎? (DYING ?꾩씠 X)
            } else {
                // ?쇰컲 ?щ쭩 ???뚮젅?댁뼱 湲곗젏 ???컻(紐⑤뱺 ???곗쭚) ??硫붾돱 ?섏씠?쒖씤
                g_GameManager.currentState = GameState::DYING;
                g_GameManager.playerHP     = 0.0f;
                Audio::PlaySfx(Audio::Sfx::Death);
                Audio::StopBgm();
                float pCX = playerWin.x + playerWin.width  * 0.5f;
                float pCY = playerWin.y + playerWin.height * 0.5f;
                g_DeathCX = pCX; g_DeathCY = pCY;
                g_DyingTimer    = DYING_DUR;
                g_DeathBoomDone = false;
                g_DeathFlash    = 0.0f;
                // ?대━紐⑦봽 ??以??먮났 (?щ쭩 ?곗텧??以??놁쓬)
                g_ZoomCX = g_ZoomCY = 0.0f;
                g_ViewZoom = 1.0f; g_ViewZoomTarget = 1.0f;
                // ?щ쭩 ?먯씤 ???뷀떚????젣 ?? 媛??媛源뚯슫 ?꾪삊(?꾨줈?몄뒪)??湲곕줉 (寃곌낵李쎌뿉 ?쒖떆??
                {
                    float best = 1e18f; const wchar_t* nm = nullptr;
                    auto consider = [&](float ex, float ey, const wchar_t* n) {
                        float dx = ex-pCX, dy = ey-pCY, d = dx*dx+dy*dy;
                        if (d < best) { best = d; nm = n; }
                    };
                    for (auto m  : g_MonsterManager.monsters)   if (m->alive)  consider(m->worldX,  m->worldY,  MobName((int)m->kind));
                    for (auto r  : g_MonsterManager.rangedMobs) if (r->alive)  consider(r->worldX,  r->worldY,  MobName(CM_RANGED));
                    for (auto bm : g_MonsterManager.bombers)    if (bm->alive) consider(bm->worldX, bm->worldY, MobName(CM_BOMBER));
                    if (g_RRBoss     && g_RRBoss->alive)     consider(g_RRBoss->worldX,     g_RRBoss->worldY,     L"VOLLEY.sys");
                    if (g_PolyBoss   && g_PolyBoss->alive)   consider(g_PolyBoss->worldX,   g_PolyBoss->worldY,   L"GLITCH.exe");
                    if (g_BotnetBoss && g_BotnetBoss->alive) consider(g_BotnetBoss->worldX, g_BotnetBoss->worldY, L"C2_RELAY.sys");
                    if (g_CentiBoss && g_CentiBoss->alive) consider(g_CentiBoss->worldX, g_CentiBoss->worldY, L"FORK.worm");
                    if (g_TotemBoss && g_TotemBoss->alive) consider(g_TotemBoss->worldX, g_TotemBoss->worldY, L"RITE.CORE");
                    if (g_UnknownBoss && g_UnknownBoss->alive) consider(g_UnknownBoss->worldX, g_UnknownBoss->worldY, L"UNKNOWN.sys");
                    int li = LangIndex();
                    const wchar_t* FMT[3] = { L"%ls: process ended", L"Terminated by %ls", L"%ls ended" };
                    const wchar_t* UNK[3] = { L"Unknown error", L"Terminated by unknown error", L"Unknown error" };
                    if (nm) swprintf_s(g_DeathReason, FMT[li], nm);
                    else    wcscpy_s(g_DeathReason, UNK[li]);
                }
            }   // close else (MK2 遺꾧린 ??
        }       // close outer if (HP <= 0)

        // DYING ?щ쭩 ?곗텧 ???뚮젅?댁뼱 湲곗젏 ???컻(紐⑤뱺 ???곗쭚) ??GAMEOVER (李?蹂???놁쓬)
        if (g_GameManager.currentState == GameState::DYING) {
            g_DyingTimer -= delta;
            g_DeathFlash -= delta * 4.0f;
            if (g_DeathFlash < 0.0f) g_DeathFlash = 0.0f;

            // ???컻 ???뚮젅?댁뼱 湲곗젏, 紐⑤뱺 ?곸씠 ?곗쭚 (利됱떆, 以??쒕꽕留덊떛 ?놁쓬)
            if (!g_DeathBoomDone) {
                g_DeathBoomDone = true;
                float pCX = g_DeathCX, pCY = g_DeathCY;
                // 紐⑤뱺 ????컻?쒗궎硫??쒓굅 (scored/noBlast ?쒖떆 ???먯닔/?곗뇙 ?뺤궛 ????
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
                // 紐⑤뱺 ?겶룸낫?ㅒ룸텇?댁껜쨌珥앹븣쨌?ы깙??利됱떆 ?꾩쟾 ??젣 ??UpdateAll(RUNNING ?꾩슜)??                //   留↔린硫?DYING/GAMEOVER ?숈븞 ?먭굅由?紐?李??깆씠 ?뺤? ?곹깭濡??⑥쑝誘濡??ш린???쒓굅.
                g_MonsterManager.Clear();   // monsters/ranged/bombers/boss ?꾨? delete + clear
                g_Bullets.clear();
                if (g_RRBoss)     { delete g_RRBoss;     g_RRBoss     = nullptr; }
                if (g_PolyBoss)   { delete g_PolyBoss;   g_PolyBoss   = nullptr; }
                if (g_BotnetBoss) { delete g_BotnetBoss; g_BotnetBoss = nullptr; }
                if (g_CentiBoss) { delete g_CentiBoss; g_CentiBoss = nullptr; }
            if (g_TotemBoss) { delete g_TotemBoss; g_TotemBoss = nullptr; }
            if (g_UnknownBoss) { delete g_UnknownBoss; g_UnknownBoss = nullptr; }
                g_Turrets.clear();
                g_BossWarnTimer  = 0.0f; g_BossWarnPick = -1;   // ?щ쭩 ???湲?以??꾩“ 痍⑥냼
                g_RRWasP2 = g_RRWasP3 = g_BotnetWasP2 = false;
                g_LaserBeams.clear();   // ?ㅼ틪 ?덉씠? 鍮??뺣━
                g_SlowZones.clear(); g_BadSectorBleed = 0.0f;   // 諛곕뱶 ?뱁꽣 媛먯냽 援ъ뿭/異쒗삁 ?뺣━
                g_NovaTimer = 0.0f;   // 諛깆떊 ?ㅼ틪 ?뺣━
                // ?뚮젅?댁뼱 以묒떖 ???컻 + 異⑷꺽??+ ?ш킅 + ?붾뱾湲?+ 諛⑹궗???뚰렪
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
                g_ViewZoom = g_ViewZoomTarget = 1.0f;   // 以??먮났 (硫붾돱 ?뺤긽??
                g_ZoomCX = g_ZoomCY = 0.0f;             // 以?以묒떖 ?붾㈃ 以묒븰?쇰줈
                g_GameManager.currentState = GameState::GAMEOVER;
                g_DyingTimer = 0.0f;
                g_GameOverFade = 0.0f;   // 寃곌낵 硫붾돱 ?섏씠?쒖씤 ?쒖옉 (2.5珥?
                // 湲곕줉 ??????쒖씠?꾨퀎 理쒓퀬???꾩쟻/肄붿씤 媛깆떊 (?좉린濡앹씠硫??쒖떆)
                //   ?щ━?먯씠?곕툕 紐⑤뱶(?뚮뱶諛뺤뒪)??肄붿씤쨌湲곕줉 ?쒖쇅 (?뚮컢 諛⑹?)
                if (!g_CreativeMode) {
                    g_LastRunRecord = RecordRunResult((int)g_Difficulty,
                                                      g_GameManager.score,
                                                      g_Stats.killCount);
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

        // ?щ쭩 ??컻 ?붿뿬臾??뚰떚?는룻뙆?맞룹땐寃⑺뙆쨌?ㅽ뙆????寃뚯엫?ㅻ쾭 ?붾㈃??援녹뼱踰꾨━吏 ?딅룄濡?        //   DYING/GAMEOVER ?숈븞?먮룄 怨꾩냽 媛깆떊???먯뿰?ㅻ읇寃??щ씪吏寃??쒕떎.
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

        // --- AUG_SELECT / DEBUFF_SELECT: 1/2/3 ?ㅻ줈 ?좏깮 ---
        // s_augSpaceReleased: 釉붾줉 諛붽묑?먯꽌??release 媛먯??섎룄濡?static ?좎뼵
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

            // ?⑥씪 利앷컯 ?곸슜 ?ы띁 (?ш?????RANDOM_AUG/PANDORA 媛 ?몄텧)
            std::function<void(int)> applyByIdx;
            applyByIdx = [&](int idx) {
                AugType atype = ALL_AUGS[idx].type;
                float oldWS   = g_Stats.windowSize;
                float oldPCX  = playerWin.x + playerWin.width  * 0.5f;
                float oldPCY  = playerWin.y + playerWin.height * 0.5f;

                // ?뱀닔: ?붿뒪?⑥튂留??섑뻾?섍퀬 Apply ?몄텧 X
                if (atype == AugType::S_CHAOS) {
                    // ??쇰?: 蹂댁쑀 利앷컯 ?딄퀬, 媛숈? 媛쒖닔 ?쒕뜡. ??60% 踰꾪봽 / 40% ?붾쾭??                    // ?ㅼ젣 蹂댁쑀 紐⑸줉(以묒꺽 ?ы븿) 湲곗??쇰줈 移댁슫????怨듦꺽??利앷?횞4 媛숈? 以묒꺽??紐⑤몢 ?ы븿
                    int prevAugs = (int)g_OwnedAugs.size();
                    float saveSpawnMult = g_Stats.mobSpawnMult;
                    int   saveCapBonus  = g_Stats.mobCapBonus;
                    int   savePackBonus = g_Stats.mobPackBonus;
                    g_Stats = PlayerStats();
                    g_Stats.windowSize *= g_Scale;          // 李????댁긽??鍮꾨? ?좎?
                    // 臾닿린/吏곸뾽 ?뺤껜?깆? ?좎? ??CHAOS ??利앷컯留??ъ텛泥⑦븳??
                    //   (寃媛?沅곸닔媛 湲곕낯 珥앹쑝濡?諛붾뚮뜕 踰꾧렇 fix. 利앷컯? ???꾨옒???꾩뿉 ??엫)
                    if (g_CurrentWeapon >= 0 && g_CurrentWeapon < (int)StartWeapon::_COUNT)
                        ApplyWeapon(g_Stats, (StartWeapon)g_CurrentWeapon);
                    if (g_RunMelee)    { g_Stats.meleeWeapon = true; g_Stats.fireInterval = 0.26f; }
                    else if (g_RunBow) { g_Stats.bowWeapon = true;  g_Stats.bulletSpeed *= 1.4f; }
                    g_Stats.baseFireInterval = g_Stats.fireInterval;
                    g_OwnedAugs.clear();
                    memset(g_GameManager.takenOnce, 0,
                           sizeof(g_GameManager.takenOnce));
                    memset(g_TypeOwned, 0, sizeof(g_TypeOwned));   // 議고빀 ?덉떆??蹂댁쑀??珥덇린??                    //   (?놁쑝硫??붾툝 ?껋뿀?붾뜲 '愿???띾뫁?? 議고빀???⑤뜕 踰꾧렇)
                    g_GameManager.maxHP = g_Stats.maxHP;
                    if (g_GameManager.playerHP > g_Stats.maxHP)
                        g_GameManager.playerHP = g_Stats.maxHP;
                    // 蹂댁쑀 媛쒖닔 蹂댁〈 ???댁쟾??24濡?罹≫빐??40媛?蹂댁쑀 ??諛섑넗留??섎뜕 踰꾧렇 ?섏젙.
                    if (prevAugs > 60) prevAugs = 60;       // 諛곗뿴 ?곹븳(?ъ쑀)
                    int nDebuffs = prevAugs / 3;            // 40% ??33% (??媛?뱁븯寃?
                    int nBuffs   = prevAugs - nDebuffs;
                    int buffs[64], debuffs[64];
                    g_GameManager.PickRandomAugIndices(buffs, nBuffs,
                        false, false, true, false, /*allowDebuff=*/false);
                    g_GameManager.PickRandomDebuffIndices(debuffs, nDebuffs);
                    for (int k = 0; k < nBuffs;   k++) applyByIdx(buffs[k]);
                    for (int k = 0; k < nDebuffs; k++) applyByIdx(debuffs[k]);
                    // 스킬 슬롯 — reroll 후 보유 목록 기준 재장착
                    ReequipSkillsFromOwned(g_OwnedAugs.data(), (int)g_OwnedAugs.size());
                    // 스폰 압력(간격/캡/군집)은 reroll 전후 중 강한 쪽 유지 — 대혼란 후 급감 방지
                    g_Stats.mobSpawnMult  = std::min(g_Stats.mobSpawnMult,  saveSpawnMult);
                    g_Stats.mobCapBonus   = std::max(g_Stats.mobCapBonus,   saveCapBonus);
                    g_Stats.mobPackBonus  = std::max(g_Stats.mobPackBonus,  savePackBonus);
                    return;
                }
                if (atype == AugType::S_PANDORA) {
                    // PANDORA: 5??= 3 踰꾪봽 + 2 ?붾쾭??(紐낆떆??遺꾨━)
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
                    // RANDOM_AUG: 踰꾪봽留?(?붾쾭???쒖쇅, ?ъ슜???섎룄 ?좎?)
                    int picks[3];
                    g_GameManager.PickRandomAugIndices(picks, 3,
                        g_Stats.sizeAugTaken, g_Stats.distAugTaken,
                        true, /*allowSpecial=*/false,
                        /*allowDebuff=*/false);
                    for (int k = 0; k < 3; k++) applyByIdx(picks[k]);
                    return;
                }

                bool prevTurret = g_Stats.turretMode;
                g_Stats.Apply(atype);
                if (ALL_AUGS[idx].rarity == AugRarity::COMMON)
                    g_Stats.ApplyCommonMultBoost();
                g_OwnedAugs.push_back(idx);
                g_TypeOwned[(int)atype] = true;
                MarkAugSeen(idx);
                // ?≫떚釉??ㅽ궗 利앷컯?대㈃ ?щ’???μ갑 (苑?李⑤㈃ ?쒗솚 援먯껜)
                EquipSkill(SkillForAug(atype));

                // ?ы깙 諛곗튂(CB_TURRET 議고빀) ????利됱떆 泥??ы깙 諛곗튂
                if (g_Stats.turretMode && !prevTurret) {
                    g_Turrets.clear();
                    g_TurretDeployTimer = TURRET_DEPLOY;  // 利됱떆 泥??ы깙 諛곗튂
                }

                // ??踰덈쭔 戮묓옄 利앷컯 ?쒖떆:
                //   - EPIC/LEGENDARY ?꾩껜
                //   - ?곗뼱??利앷컯 (?꾪솚?몃?/?쒕줎/李⑦겕?? ???깃툒 臾닿? 1?뚯뵫
                if (AugOnceOnly(atype, ALL_AUGS[idx].rarity))
                    g_GameManager.takenOnce[idx] = true;

                // (?≫삁留?踰꾪봽: ?꾩옱 HP -20% ?⑤꼸???쒓굅 ??理쒕? 泥대젰 +25/?≫삁濡?蹂寃?

                // 誘몃땲?? 媛뺤젣濡?maxHP 10 ?곸슜 ???꾩옱 HP cap
                if (g_GameManager.playerHP > g_Stats.maxHP)
                    g_GameManager.playerHP = g_Stats.maxHP;

                // 李⑦겕??(?곗뼱 ?곸슜 ??chakramCount 留뚰겮 ?쒖꽦??
                if (atype == AugType::CHAKRAM ||
                    atype == AugType::CHAKRAM_2 ||
                    atype == AugType::CHAKRAM_3 ||
                    atype == AugType::CHAKRAM_SINGULARITY) {
                    for (int c = 0; c < g_Stats.chakramCount && c < MAX_CHAKRAMS; c++) {
                        if (!g_Chakrams[c].alive && g_Chakrams[c].respawnTimer <= 0) {
                            g_Chakrams[c].alive        = true;
                            g_Chakrams[c].hp           = 150.0f;
                            g_Chakrams[c].maxHp        = 150.0f;
                            g_Chakrams[c].respawnTimer = 0.0f;
                        }
                        // 洹좊벑 媛곷룄 諛곗튂
                        g_Chakrams[c].angle =
                            (float)c / (float)g_Stats.chakramCount * 6.2831853f;
                    }
                }

                if (atype == AugType::BROKEN_SIGHT) {
                    g_Orb.active = true;
                    g_Orb.x = (float)(rand() % screenWidth);
                    g_Orb.y = (float)(rand() % screenHeight);
                    float a = (float)(rand() % 628) * 0.01f;
                    float s = 100.0f + (float)(rand() % 150);
                    g_Orb.vx = cosf(a) * s;
                    g_Orb.vy = sinf(a) * s;
                    g_Orb.wanderTimer = 0.5f;
                }
                // ?ㅺ??ㅻ뒗 二쎌쓬: 泥??쎌뿉?쒕쭔 ?ㅻ툕 ?ㅽ룿. ?댄썑 ?쎌? ?띾룄留?+20%
                // (?띾룄 ?꾩쟻? PlayerStats::Apply ??approachStacks ++ 媛 ?대떦)
                if (atype == AugType::D_APPROACH && g_ApproachOrbs.empty()) {
                    ApproachOrb orb;
                    int edge = rand() % 4;
                    if      (edge == 0) { orb.x = (float)(rand()%screenWidth);  orb.y = -40.0f; }
                    else if (edge == 1) { orb.x = (float)(rand()%screenWidth);  orb.y = screenHeight + 40.0f; }
                    else if (edge == 2) { orb.x = -40.0f;                       orb.y = (float)(rand()%screenHeight); }
                    else                { orb.x = screenWidth + 40.0f;          orb.y = (float)(rand()%screenHeight); }
                    g_ApproachOrbs.push_back(orb);
                }
                // ?쒖빞/?ш린 蹂寃???playerWin ?ш린쨌?꾩튂 媛깆떊
                if (g_Stats.windowSize != oldWS) {
                    playerWin.width  = g_Stats.windowSize;
                    playerWin.height = g_Stats.windowSize;
                    playerWin.x = oldPCX - g_Stats.windowSize * 0.5f;
                    playerWin.y = oldPCY - g_Stats.windowSize * 0.5f;
                }
            };

            auto applyAug = [&](int slot) {
                bool wasBuff  = (g_GameManager.currentState == GameState::AUG_SELECT);
                applyByIdx(g_GameManager.augChoices[slot]);
                g_GameManager.maxHP = g_Stats.maxHP;
                if (wasBuff) {
                    // 蹂댁뒪 蹂댁긽 以묒씠硫??붾쾭???섏씠吏 skip
                    if (g_BossRewardPicksLeft > 0) {
                        --g_BossRewardPicksLeft;
                        if (g_BossRewardPicksLeft > 0) {
                            g_GameManager.PickAugChoices(g_Stats.sizeAugTaken,
                                                         g_Stats.distAugTaken, g_CreativeMode);
                            g_GameManager.currentState = GameState::AUG_SELECT;
                        } else {
                            g_GameManager.currentState = GameState::RUNNING;
                        }
                    } else if (g_Stats.mk2SkipDebuff || g_CreativeFreeGrab) {
                        // MK2 遺????/ ?щ━?먯씠?곕툕 F 洹몃옪: DEBUFF_SELECT ?ㅽ궢
                        //   (?щ━?먯씠?곕툕?쇰룄 ?덈꺼?낆? ?뺤긽 ?붾쾭???섏씠吏 = "?붾쾭?꾨룄 ?⑤뒗 寃뚯엫")
                        g_CreativeFreeGrab = false;
                        g_GameManager.currentState = GameState::RUNNING;
                    } else {
                        // ?쇰컲: 踰꾪봽 ???붾쾭??媛뺤젣 ?좏깮 ?섏씠吏
                        g_GameManager.PickDebuffChoices();
                        g_GameManager.currentState = GameState::DEBUFF_SELECT;
                    }
                } else {
                    // ?붾쾭????????寃뚯엫 ?ш컻
                    g_GameManager.currentState = GameState::RUNNING;
                }
                // C14: ?멸쾶??蹂듦? ??吏㏃? ?좎삁 ????0.25s ?????뺤?("?"), ?꾩껜 臾댁쟻+諛쒖궗?듭젣
                //   濡?利됱궗/?ㅻ컻 諛⑹??섎ŉ ?먯뿰?ㅻ윭???꾩씠 ?쒕젅?대? 以??
                if (g_GameManager.currentState == GameState::RUNNING)
                    g_PostPickGrace = 0.5f;
            };

            // 1/2/3 = hover (Space로 적용)
            if (k1 == GLFW_PRESS && g_aug1Released) { g_HoveredAug = 0; g_aug1Released = false; }
            if (k2 == GLFW_PRESS && g_aug2Released) { g_HoveredAug = 1; g_aug2Released = false; }
            if (k3 == GLFW_PRESS && g_aug3Released) { g_HoveredAug = 2; g_aug3Released = false; }
            if (k1 == GLFW_RELEASE) g_aug1Released = true;
            if (k2 == GLFW_RELEASE) g_aug2Released = true;
            if (k3 == GLFW_RELEASE) g_aug3Released = true;

            // Space = ?곸슜 (hover ??移대뱶留?. Enter ???쒓굅 ??Space 濡??듭씪
            int kSp = glfwGetKey(window, GLFW_KEY_SPACE);
            if (kSp == GLFW_PRESS && s_augSpaceReleased && g_HoveredAug >= 0) {
                applyAug(g_HoveredAug);
                g_HoveredAug = -1;
                s_augSpaceReleased = false;
                g_GameManager.spaceReleased = false; // RUNNING 吏곹썑 pause 諛⑹?
            }

            // 留덉슦???대┃: 移대뱶 hit-test ??hover 留?(?곸슜? Space ?ㅻ줈留?
            if (lmb && !g_LmbPrev) {
                const int  nCards  = 3;
                const float CARD_W = 280.0f;
                const float CARD_H = 400.0f;
                const float GAP    = 48.0f;
                const float TOTAL_W = (float)nCards * CARD_W + (float)(nCards-1) * GAP;
                float baseX = (screenWidth  - TOTAL_W) * 0.5f;
                float baseY = (screenHeight - CARD_H)  * 0.4f;
                for (int i = 0; i < nCards; i++) {
                    float cx = baseX + i * (CARD_W + GAP);
                    float yOff = (g_HoveredAug == i) ? -16.0f : 0.0f;
                    if (mx >= cx && mx <= cx + CARD_W &&
                        my >= baseY + yOff && my <= baseY + yOff + CARD_H) {
                        g_HoveredAug = i;  // ?대┃ = hover 留?(?곸슜? Space)
                        break;
                    }
                }
            }
        }

        // --- 런 상점 (보스 클리어 후) ---
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

        // --- ?щ━?먯씠?곕툕 紐⑤뱶: F = 利앷컯 洹몃옪(?붾쾭???ы븿 ?뚮뱶諛뺤뒪), G = 臾댁쟻 ?좉? ---
        if (g_CreativeMode) {
            static bool s_fkeyReleased = true;
            int kF = glfwGetKey(window, GLFW_KEY_F);
            if (kF == GLFW_RELEASE) s_fkeyReleased = true;
            if (kF == GLFW_PRESS && s_fkeyReleased &&
                g_GameManager.currentState == GameState::RUNNING) {
                // ?뚮뱶諛뺤뒪: ?붾쾭?꾨룄 移대뱶 ????욎뼱??臾댁뾿?대뱺 吏묒쓣 ???덇쾶
                g_GameManager.PickAugChoices(g_Stats.sizeAugTaken,
                                             g_Stats.distAugTaken, /*allowDebuff=*/true);
                g_CreativeFreeGrab = true;
                g_GameManager.currentState = GameState::AUG_SELECT;
                s_fkeyReleased = false;
            }
            // G ??臾댁쟻 ON/OFF ?좉?
            static bool s_gkeyReleased = true;
            int kG = glfwGetKey(window, GLFW_KEY_G);
            if (kG == GLFW_RELEASE) s_gkeyReleased = true;
            if (kG == GLFW_PRESS && s_gkeyReleased) {
                g_CreativeGodmode = !g_CreativeGodmode;
                s_gkeyReleased = false;
            }
            // B — 선택 보스 즉시 스폰 (None이면 VOLLEY.sys)
            static bool s_bkeyReleased = true;
            int kB = glfwGetKey(window, GLFW_KEY_B);
            if (kB == GLFW_RELEASE) s_bkeyReleased = true;
            if (kB == GLFW_PRESS && s_bkeyReleased &&
                g_GameManager.currentState == GameState::RUNNING) {
                if (!BossFightBusy()) {
                    float bossHpC = GetDifficultyParams(g_Difficulty).bossHp;
                    float polyHpC = (g_Difficulty == Difficulty::EASY) ? 10000.0f
                                  : (g_Difficulty == Difficulty::HARD) ? 75000.0f : 30000.0f;
                    int pick = g_CreativeBossPick >= 0 ? g_CreativeBossPick : 2;
                    QueueCreativeBossPick(pick, bossHpC, polyHpC);
                }
                s_bkeyReleased = false;
            }
        }

        // --- ?덊듃?ㅽ넲: ???대깽??吏곹썑 ?좉퉸 ?쒕??덉씠???뺤? (?뚮뜑??怨꾩냽) ---
        if (g_HitStopTimer > 0.0f) g_HitStopTimer -= delta;

        // --- Fixed timestep (DYING = 0.15횞 ?щ줈??紐⑥뀡, ?덊듃?ㅽ넲 = ?꾩쟾 ?뺤?) ---
        float physDelta = (g_GameManager.currentState == GameState::DYING)
                          ? delta * 0.15f : delta;
        if (g_HitStopTimer > 0.0f) physDelta = 0.0f;   // 히트스탑 중 물리 스텝 미실행
        accumulator += physDelta;
        while (accumulator >= FIXED_DT) {
            if (g_GameManager.ShouldUpdate()) {
                g_PlayerDmgMult = g_Stats.GetDamageTakenMult();
                // WASD ?대룞 ???媛곸꽑 normalize (vec 紐⑥븘??湲몄씠濡??섎닎)
                float mvX = 0.0f, mvY = 0.0f;
                if (keys[GLFW_KEY_W]) mvY -= 1.0f;
                if (keys[GLFW_KEY_S]) mvY += 1.0f;
                if (keys[GLFW_KEY_A]) mvX -= 1.0f;
                if (keys[GLFW_KEY_D]) mvX += 1.0f;
                float mlen = sqrtf(mvX*mvX + mvY*mvY);
                if (mlen > 0.001f && !g_DashActive) {
                    mvX /= mlen; mvY /= mlen;
                    float moveMult = g_Stats.GetMoveMultiplier(lmb);
                    // 諛곕뱶 ?뱁꽣 媛먯냽 援ъ뿭 ??遺?앸릺???쇱쭊 ?곸뿭(grow factor) ?덉씠硫??대룞?띾룄 -10%
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
                    }
                    float curMove  = MOVE_SPEED * moveMult * zoneSlow;
                    playerWin.x += mvX * curMove * FIXED_DT;
                    playerWin.y += mvY * curMove * FIXED_DT;
                    // FORK.worm ?щ옒??踰????쇰컲 ?대룞? 吏곸꽑??留됲옒, ???臾댁쟻 以????듦낵
                    if (g_CentiBoss && g_CentiBoss->alive && g_DashInvuln <= 0.0f) {
                        float wpcx = playerWin.x + playerWin.width  * 0.5f;
                        float wpcy = playerWin.y + playerWin.height * 0.5f;
                        g_CentiBoss->blockMove(wpcx, wpcy, 18.0f);
                        playerWin.x = wpcx - playerWin.width  * 0.5f;
                        playerWin.y = wpcy - playerWin.height * 0.5f;
                    }
                    // ?대룞 ?붿긽(afterimage) ???쇱젙 媛꾧꺽?쇰줈 ?뚮젅?댁뼱 以묒떖???낆? ?쒖븞 ?붿긽
                    static float s_trailAcc = 0.0f;
                    s_trailAcc += FIXED_DT;
                    if (s_trailAcc >= 0.028f) {
                        s_trailAcc = 0.0f;
                        SpawnTrail(playerWin.x + playerWin.width  * 0.5f,
                                   playerWin.y + playerWin.height * 0.5f,
                                   24.0f, 0.35f, 0.85f, 1.0f);
                    }
                }

                // ?뚮젅?댁뼱 罹먮┃??李?以묒븰)媛 蹂댁씠???곸뿭 諛뽰쑝濡??섍?吏 紐삵븯寃??대옩??
                // 以뚯븘???대━紐⑦봽 2?섏씠利??섎㈃ 蹂댁씠???붾뱶媛 ?볦뼱吏誘濡??대룞 援ъ뿭??媛숈씠 ?뺤옣.
                // (zoom=1 ?대㈃ ?뺥솗??[0, screen] ??湲곗〈怨??숈씪)
                float zoomNow = (g_ViewZoom < 0.01f) ? 0.01f : g_ViewZoom;
                float halfW = (float)screenWidth  * 0.5f / zoomNow;
                float halfH = (float)screenHeight * 0.5f / zoomNow;
                float ccX   = (float)screenWidth  * 0.5f;
                float ccY   = (float)screenHeight * 0.5f;
                float pCX = playerWin.x + playerWin.width  * 0.5f;
                float pCY = playerWin.y + playerWin.height * 0.5f;
                if (pCX < ccX - halfW) pCX = ccX - halfW;
                if (pCX > ccX + halfW) pCX = ccX + halfW;
                if (pCY < ccY - halfH) pCY = ccY - halfH;
                // C17: ?섎떒 ?묒뾽?쒖떆以??멸쾶??媛吏?諛?+ ?ㅼ젣 OS ?묒뾽?쒖떆以? 移⑤쾾 諛⑹? ??                //   ?묒뾽?쒖떆以꾩? '?ㅽ겕由? ?섎떒 怨좎젙 ?쎌??대씪, 以뚯븘???먯닔 ?뺤옣)?섎㈃ ?붾뱶 ?⑥쐞濡?                //   barTotal/zoom 留뚰겮 李⑥??? 蹂댁씠???곸뿭 ?섎떒(ccY+halfH)?먯꽌 洹몃쭔???꾧? ?쒓퀎 ??                //   ?곷떒 ?뺤옣怨??移?씠 ?섎룄濡?湲곗〈??sh-barTotal 怨좎젙?대씪 ?섎떒留????섏뼱?ъ쓬).
                float barTotal    = g_GameBarH + (float)g_TaskbarH;
                float bottomLimit = ccY + halfH - barTotal / zoomNow;
                if (pCY > bottomLimit) pCY = bottomLimit;
                playerWin.x = pCX - playerWin.width  * 0.5f;
                playerWin.y = pCY - playerWin.height * 0.5f;

                // ?? ?≫떚釉??ㅽ궗 ??荑⑤떎??吏??媛깆떊 + ?낅젰(Shift ???/ Q쨌E쨌R ?щ’) ??
                if (g_DashCd > 0.0f)        g_DashCd        -= FIXED_DT;
                if (g_DashInvuln > 0.0f)    g_DashInvuln    -= FIXED_DT;
                if (g_PostPickGrace > 0.0f) g_PostPickGrace -= FIXED_DT;  // C14
                if (g_TimeStopTimer > 0.0f) g_TimeStopTimer -= FIXED_DT;
                if (g_HyperFocusTimer > 0.0f) g_HyperFocusTimer -= FIXED_DT;
                for (int i = 0; i < 3; i++) if (g_Skills[i].cd > 0.0f) g_Skills[i].cd -= FIXED_DT;

                // 李??リ린 ???뚮젅?댁뼱 以묒떖 ??컻 (?됰갚 + ?쇳빐)
                auto closeWindowBlast = [&](float cx, float cy) {
                    float dmg = g_Stats.GetBaseDamage() * g_Stats.GetDamageMultiplier(0.0f) * 8.0f;
                    if (g_TotemBoss && g_TotemBoss->alive) dmg *= g_TotemBoss->statDamageMult();
                    float rad = 380.0f, r2 = rad * rad, knock = 130.0f;
                    auto hitKB = [&](float& ex, float& ey, float& hp, bool& al) {
                        float dx = ex - cx, dy = ey - cy, d2 = dx*dx + dy*dy;
                        if (d2 < r2) { hp -= dmg; float d = std::sqrt(d2)+1e-3f;
                            ex += dx/d*knock; ey += dy/d*knock; if (hp <= 0) al = false; }
                    };
                    for (auto m  : g_MonsterManager.monsters)   if (m->alive)  hitKB(m->worldX,  m->worldY,  m->hp,  m->alive);
                    for (auto r  : g_MonsterManager.rangedMobs) if (r->alive)  hitKB(r->worldX,  r->worldY,  r->hp,  r->alive);
                    for (auto bm : g_MonsterManager.bombers)    if (bm->alive) hitKB(bm->worldX, bm->worldY, bm->hp, bm->alive);
                    if (g_RRBoss && g_RRBoss->alive) { float dx=g_RRBoss->worldX-cx,dy=g_RRBoss->worldY-cy; if(dx*dx+dy*dy<r2){g_RRBoss->hp-=dmg; if(g_RRBoss->hp<=0)g_RRBoss->alive=false;} }
                    if (g_PolyBoss && g_PolyBoss->alive && g_PolyBoss->damageable()) { float dx=g_PolyBoss->worldX-cx,dy=g_PolyBoss->worldY-cy; if(dx*dx+dy*dy<r2){g_PolyBoss->hp-=dmg; if(g_PolyBoss->hp<=0)g_PolyBoss->alive=false;} }
                    if (g_BotnetBoss && g_BotnetBoss->alive) { float dx=g_BotnetBoss->worldX-cx,dy=g_BotnetBoss->worldY-cy; if(dx*dx+dy*dy<r2){g_BotnetBoss->hp-=dmg; if(g_BotnetBoss->hp<=0)g_BotnetBoss->alive=false;} }
                    if (g_CentiBoss && g_CentiBoss->alive && g_CentiBoss->vulnerable()) { float dx=g_CentiBoss->worldX-cx,dy=g_CentiBoss->worldY-cy; if(dx*dx+dy*dy<r2){g_CentiBoss->hp-=dmg; if(g_CentiBoss->hp<=0)g_CentiBoss->alive=false;} }
                    if (g_TotemBoss && g_TotemBoss->alive && g_TotemBoss->vulnerable()) { float dx=g_TotemBoss->worldX-cx,dy=g_TotemBoss->worldY-cy; if(dx*dx+dy*dy<r2){g_TotemBoss->hp-=dmg; if(g_TotemBoss->hp<=0)g_TotemBoss->alive=false;} }
                    if (g_UnknownBoss && g_UnknownBoss->alive) { float dx=g_UnknownBoss->worldX-cx,dy=g_UnknownBoss->worldY-cy; if(dx*dx+dy*dy<r2){float ud=dmg*g_UnknownBoss->damageTakenMult(); g_UnknownBoss->hp-=ud; if(g_UnknownBoss->hp<=0)g_UnknownBoss->alive=false;} }
                    for (int ti = 0; g_TotemBoss && g_TotemBoss->alive && ti < TotemBoss::N_TOTEM; ti++) {
                        auto& tt = g_TotemBoss->totems[ti];
                        if (!tt.alive) continue;
                        float dx = tt.x - cx, dy = tt.y - cy;
                        if (dx*dx + dy*dy < r2) { tt.hp -= dmg; if (tt.hp <= 0) { tt.alive = false; g_TotemBoss->onTotemKilled(ti); } }
                    }
                    SpawnShockWave(cx, cy, rad*1.3f, 0.6f, 0.5f, 0.8f, 1.0f);
                    SpawnEnemyExplosion(cx, cy, 0.5f, 0.8f, 1.0f, true);
                    g_ShakeTime = 0.4f; g_ShakeMag = 20.0f;
                    TriggerFlash(0.5f, 0.8f, 1.0f, 0.45f); TriggerHitStop(0.07f);
                };

                // ?щ’ ?ㅽ궗 諛쒕룞 ?ы띁
                auto useSkill = [&](int slot) {
                    if (slot < 0 || slot >= 3) return;
                    if (g_TotemBoss && g_TotemBoss->alive && g_TotemBoss->isSkillSealed(slot)) return;
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

                // 정지 시간 (집중 조준 스킬)
                if (mlen <= 0.001f && !g_DashActive)
                    g_FocusStandTimer += FIXED_DT;
                else
                    g_FocusStandTimer = 0.0f;

                // 플래시 대시 — 보간 이동 + 짧은 무적
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
                    if (g_DashToY < ccY-halfH) g_DashToY = ccY-halfH;
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
                // C16: ?≫떚釉??ㅽ궗 ?먮룞 ?ъ슜 ??荑⑤떎???앸궃 ?щ’???먮룞 諛쒕룞
                if (g_AutoSkill) for (int i = 0; i < 3; i++) useSkill(i);

                // 珥앹븣 ?대룞 + ?붾㈃ 諛?鍮꾪솢?깊솕 (+ ?좊룄??蹂댁젙)
                for (auto& b : g_Bullets) {
                    // ?좊룄?? 媛??媛源뚯슫 ?곸쓣 ?ν빐 ?먯쭊??諛⑺뼢 蹂댁젙
                    if (b.homing && !b.isEnemy && b.active) {
                        float tx = 0.0f, ty = 0.0f;
                        if (findNearestEnemy(b.x, b.y, tx, ty)) {
                            // ?꾩옱 諛⑺뼢 ??紐⑺몴 諛⑺뼢 ?ъ씠瑜?turn rate 留뚰겮 ?뚯쟾
                            float wx = tx - b.x, wy = ty - b.y;
                            float wl = sqrtf(wx*wx + wy*wy);
                            if (wl > 0.001f) {
                                float wdx = wx / wl, wdy = wy / wl;
                                // ?몄쟻/?댁쟻?쇰줈 媛곷룄 李⑥씠 (?묒? ?④퀎)
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
                    // ???좊룄???먭굅由щす ??: ?뚮젅?댁뼱 履쎌쑝濡??꾩＜ ?쏀븯寃?諛⑺뼢 蹂댁젙.
                    //   ?? ?뚮젅?댁뼱 洹쇱쿂(<300px)?먯꽑 ?좊룄 以묐떒 ??鍮쀫굹媛??꾩씠 怨듭쟾?섏? ?딄퀬
                    //   洹몃?濡?吏?섍컧(?댁쟾 0.9 rad/s 媛 ?뚮젅?댁뼱 二쇱쐞瑜??꾨뒗 臾몄젣 ?섏젙).
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
                    // 시간 정지: 적 탄만 멈춤. 초집중: 적 탄 30% 감속
                    float bDt = FIXED_DT;
                    if (b.isEnemy && g_TimeStopTimer > 0.0f) { /* skip update below */ }
                    else if (b.isEnemy && g_HyperFocusTimer > 0.0f) bDt *= 0.7f;
                    if (!(b.isEnemy && g_TimeStopTimer > 0.0f)) b.Update(bDt);
                    // ?붾㈃ 諛?鍮꾪솢?깊솕 ??以뚯븘???대━紐⑦봽 2?섏씠利??섎㈃ 蹂댁씠???곸뿭??                    // ?볦뼱吏誘濡?寃쎄퀎??媛숈씠 ?뺤옣 (??洹몃윭硫??뺤옣 援ъ뿭?먯꽌 ?꾩씠 利됱떆 ?щ씪吏?
                    {
                        float zb = (g_ViewZoom < 0.01f) ? 0.01f : g_ViewZoom;
                        float mX = screenWidth  * 0.5f * (1.0f / zb - 1.0f) + 200.0f;
                        float mY = screenHeight * 0.5f * (1.0f / zb - 1.0f) + 200.0f;
                        if (b.x < -mX || b.x > screenWidth  + mX ||
                            b.y < -mY || b.y > screenHeight + mY)
                            b.active = false;
                    }
                }

                // ???臾댁쟻 ???대쾲 ?ㅽ뀦 ?쒖옉 HP ???(???쇳빐??臾댄슚, ?뚮났? ?좎?)
                float hpAtStep = g_GameManager.playerHP;
                //   ???뺤? = ?쒓컙?뺤? ?ㅽ궗 OR 利앷컯 ??吏곹썑 ~0.05s("?꾩씠 ?", 吏㏐쾶)
                bool  timeStopped = (g_TimeStopTimer > 0.0f) || (g_PostPickGrace > 0.45f);
                float enemyDt = FIXED_DT;
                if (!timeStopped && g_HyperFocusTimer > 0.0f) enemyDt *= 0.7f;
                float focusSlow = (g_HyperFocusTimer > 0.0f) ? 0.7f : 1.0f;

                // 紐ъ뒪???낅뜲?댄듃 (?붾쾭??multiplier ?곸슜) ???쒓컙 ?뺤? 以묒뿏 ??硫덉땄
                float rmobMoveMult = 1.0f / g_Stats.rmobDelayMult; // <1 ????鍮좊쫫
                // ?먯닔 湲곕컲 ?띾룄 ?⑦봽 (ACT 테마 반영)
                BossDir::ActRules actSpd = BossDir::GetActRules();
                float si = (float)g_GameManager.score / 100000.0f;
                if (si > actSpd.intensityCap) si = actSpd.intensityCap;
                float mobSpdRamp = (1.0f + si * 0.09f) * actSpd.speedMult;
                // 특이점 — 몹 AI 전에 끌어당김·소각, AI 후 플레이어 밀어내기
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
                    float gWX = -1.0f, gWY = -1.0f, gWW = -1.0f, gWH = -1.0f;
                    if (g_PolyBoss && g_PolyBoss->alive) {
                        gWX = playerWin.x; gWY = playerWin.y;
                        gWW = playerWin.width; gWH = playerWin.height;
                    }
                    g_MonsterManager.UpdateAll(pCX, pCY, enemyDt,
                                               g_GameManager.playerHP, g_Bullets,
                                               g_Stats.mobSpeedMult * mobSpdRamp * focusSlow,
                                               rmobMoveMult * mobSpdRamp * focusSlow,
                                               gWX, gWY, gWW, gWH);
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

                // 由щ줈???щ꼫 ?낅뜲?댄듃 (臾닿린 ?곹깭癒몄떊 + ?μ쟾 吏덉＜, ??珥앹븣 push)
                if (!timeStopped && g_RRBoss && g_RRBoss->alive)
                    g_RRBoss->Update(pCX, pCY, enemyDt, g_GameManager.playerHP, g_Bullets);

                // ?대━紐⑦봽 ?낅뜲?댄듃 (??蹂??+ ?몃え/?덉씠?/李⑦겕?? + ?섏씠利? ?붾㈃ ?뺤옣
                if (g_PolyBoss && g_PolyBoss->alive) {
                    float prevPWx = playerWin.x, prevPWy = playerWin.y;
                    if (!timeStopped)
                    g_PolyBoss->Update(pCX, pCY, enemyDt, g_GameManager.playerHP, g_Bullets,
                        playerWin.x, playerWin.y, playerWin.width, playerWin.height);
                    if (!timeStopped) {
                        float wdx = playerWin.x - prevPWx, wdy = playerWin.y - prevPWy;
                        DisplaceEntitiesInWindow(prevPWx, prevPWy,
                            playerWin.width, playerWin.height, wdx, wdy);
                    }
                    if (g_PolyBoss->fx.shakePulse) {
                        g_PolyBoss->fx.shakePulse = false;
                        g_ShakeTime = 0.28f; g_ShakeMag = 16.0f;
                    }
                    // ?섏씠利? = ?곸쐞 蹂댁뒪: ?붾㈃ 以뚯븘?껋쑝濡????볦? 援ш컙?먯꽌 ?몄? (?섎룄??湲곕뒫)
                    //   ?먯닔 以뚯븘?껉낵 異⑸룎 ?딄쾶 ??以뚯븘?껊맂 履?min) 梨꾪깮. (?섏씠利?? ?먯닔以??좎?)
                    // ?? 2?섏씠利?吏꾩엯 ?곗텧 ??蹂댁뒪 ?ы슚 + ?ㅼ쨷 異⑷꺽??(1?? ??
                    int curForm = (int)g_PolyBoss->form;
                    if (curForm != g_PolyPrevForm) {
                        if (g_PolyPrevForm != -1) {
                            float fr = 0.2f, fg = 1.0f, fb = 0.85f;
                            if (g_PolyBoss->form == PForm::DISPLACE) { fr = 1.0f; fg = 0.35f; fb = 0.2f; }
                            else if (g_PolyBoss->form == PForm::PHANTOM) { fr = 0.85f; fg = 0.85f; fb = 1.0f; }
                            TriggerFlash(fr, fg, fb, 0.45f);
                            TriggerHitStop(0.06f);
                            SpawnShockWave(g_PolyBoss->worldX, g_PolyBoss->worldY,
                                           220.0f, 0.55f, fr, fg, fb);
                        }
                        g_PolyPrevForm = curForm;
                    }
                    pCX = playerWin.x + playerWin.width  * 0.5f;
                    pCY = playerWin.y + playerWin.height * 0.5f;
                }

                // C2_RELAY.sys Update
                if (!timeStopped && g_BotnetBoss && g_BotnetBoss->alive)
                    g_BotnetBoss->Update(pCX, pCY, enemyDt, g_GameManager.playerHP, g_Bullets);
                if (g_BotnetBoss && g_BotnetBoss->shakePulse) {
                    g_BotnetBoss->shakePulse = false;
                    g_ShakeTime = 0.32f; g_ShakeMag = 14.0f;
                }

                // FORK.worm ?낅뜲?댄듃 (吏洹몄옱洹?諛고쉶 + ?붾㈃諛??댄깉?믪옱吏꾩엯 ?뚯쭊)
                if (!timeStopped && g_CentiBoss && g_CentiBoss->alive) {
                    g_CentiBoss->Update(pCX, pCY, enemyDt, g_GameManager.playerHP, g_Bullets);
                    if (g_CentiBoss->shakePulse) {          // ?붾㈃ 諛뽰쑝濡??섍컝 ???쏀븳 吏꾨룞
                        g_CentiBoss->shakePulse = false;
                        g_ShakeTime = 0.35f; g_ShakeMag = 14.0f;
                    }
                    // ?ㅽ궗 ?쒖쟾 吏꾨룞(媛踰쇱슫 ?쇰뱶諛? ?덈퐬 X) ????吏꾨룞 以묒씠硫???뼱?곗? ?딆쓬
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

                // TOTEM.sys 업데이트 (토템 루틴 + 시간정지 공격)
                if (!timeStopped && g_TotemBoss && g_TotemBoss->alive) {
                    float pullX = 0.0f, pullY = 0.0f;
                    g_TotemBoss->Update(pCX, pCY, enemyDt, g_GameManager.playerHP,
                                        g_Bullets, pullX, pullY);
                    if (pullX != 0.0f || pullY != 0.0f) {
                        pCX += pullX; pCY += pullY;
                        if (pCX < ccX - halfW) pCX = ccX - halfW;
                        if (pCX > ccX + halfW) pCX = ccX + halfW;
                        if (pCY < ccY - halfH) pCY = ccY - halfH;
                        if (pCY > bottomLimit) pCY = bottomLimit;
                        playerWin.x = pCX - playerWin.width * 0.5f;
                        playerWin.y = pCY - playerWin.height * 0.5f;
                    }
                    for (auto& sp : g_TotemBoss->mobSpawnQueue) {
                        if ((int)g_MonsterManager.monsters.size() >= 40) break;
                        Monster* nm = new Monster(sp.first, sp.second,
                                                  g_Stats.monsterHpMult * 0.55f,
                                                  0.85f, true);
                        nm->color = glm::vec3(0.5f, 0.92f, 0.42f);
                        g_MonsterManager.monsters.push_back(nm);
                    }
                    g_TotemBoss->mobSpawnQueue.clear();
                }

                if (!timeStopped && g_UnknownBoss && g_UnknownBoss->alive) {
                    g_UnknownBoss->Update(pCX, pCY,
                                          playerWin.x, playerWin.y,
                                          playerWin.width, playerWin.height,
                                          enemyDt, g_GameManager.playerHP, g_Bullets);
                    if (g_UnknownBoss->shakePulse > 0.0f) {
                        g_UnknownBoss->shakePulse = 0.0f;
                        g_ShakeTime = 0.16f; g_ShakeMag = 10.0f;
                    }
                }

                // ── 보스 페이즈 진입 (상승음?) ── 공통 등장 연출 + 보스별 처리 ──
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
                if (g_BotnetBoss && g_BotnetBoss->alive) {
                    if (g_BotnetBoss->phase2 && !g_BotnetWasP2) {
                        g_BotnetWasP2 = true;
                        p2enter(g_BotnetBoss->worldX, g_BotnetBoss->worldY,
                                glm::vec3(0.25f, 0.95f, 0.45f));
                    }
                } else g_BotnetWasP2 = false;

                // 異⑸룎 (諛섑솚媛?= ?뚮젅?댁뼱媛 ?대쾲 ?꾨젅???쇨꺽?먮뒗吏)
                bool hit = CollisionSystem::Update(pCX, pCY,
                    g_MonsterManager, g_Bullets,
                    g_GameManager.playerHP,
                    g_GameManager.scoreAccum, g_GameManager.score,
                    g_Stats, g_GameManager.xp);
                // ???臾댁쟻 / 利앷컯???좎삁(C14) ???대쾲 ?ㅽ뀦?????쇳빐 臾댄슚 (?뚮났? ?좎?)
                if ((g_DashInvuln > 0.0f || g_PostPickGrace > 0.0f) &&
                    g_GameManager.playerHP < hpAtStep)
                    g_GameManager.playerHP = hpAtStep;

                // 由щ줈???щ꼫 蹂몄껜 vs ?뚮젅?댁뼱 珥앹븣 (?ㅼ쐲 ?먯젙)
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
                            float dealt = (dmg < rb->hp) ? dmg : rb->hp;
                            rb->hp -= dealt;
                            if (b.remainingDmg > 0.0f) b.remainingDmg -= dealt;
                            if (rb->hp <= 0.0f) rb->alive = false;
                            if (b.remainingDmg <= 0.001f) b.active = false;
                        }
                    }
                }

                if (g_UnknownBoss && g_UnknownBoss->alive) {
                    auto* ub = g_UnknownBoss;
                    for (auto& b : g_Bullets) {
                        if (!b.active || b.isEnemy) continue;
                        if (SegDist(ub->worldX, ub->worldY,
                                    b.prevX, b.prevY, b.x, b.y) < UnknownBoss::BODY * 0.75f) {
                            float pd = glm::distance(glm::vec2(pCX, pCY),
                                                     glm::vec2(ub->worldX, ub->worldY));
                            float dmg;
                            if (b.remainingDmg > 0.0f)   dmg = b.remainingDmg;
                            else if (b.turretDmg > 0.0f) dmg = b.turretDmg;
                            else dmg = g_Stats.GetBaseDamage()
                                     * g_Stats.GetDamageMultiplier(pd) * b.dmgMult;
                            dmg *= ub->damageTakenMult();
                            float dealt = (dmg < ub->hp) ? dmg : ub->hp;
                            ub->hp -= dealt;
                            if (b.remainingDmg > 0.0f) b.remainingDmg -= dealt;
                            if (ub->hp <= 0.0f) ub->alive = false;
                            if (b.remainingDmg <= 0.001f) b.active = false;
                        }
                    }
                }

                // C2_RELAY.sys — 호스트 + 본체(피해 감소) vs 플레이어 총알
                if (g_BotnetBoss && g_BotnetBoss->alive) {
                    auto* nb2 = g_BotnetBoss;
                    for (auto& b : g_Bullets) {
                        if (!b.active || b.isEnemy) continue;
                        auto dmgAt = [&](float ex, float ey) {
                            float pd = glm::distance(glm::vec2(pCX, pCY), glm::vec2(ex, ey));
                            if (b.remainingDmg > 0.0f)   return b.remainingDmg;
                            if (b.turretDmg > 0.0f)      return b.turretDmg;
                            return g_Stats.GetBaseDamage()
                                 * g_Stats.GetDamageMultiplier(pd) * b.dmgMult;
                        };
                        bool consumed = false;
                        for (auto& mn : nb2->minions) {
                            if (!mn.alive) continue;
                            if (SegDist(mn.x, mn.y, b.prevX, b.prevY, b.x, b.y)
                                    < BotnetBoss::minionHit(mn.kind)) {
                                float dmg = dmgAt(mn.x, mn.y);
                                float dealt = (dmg < mn.hp) ? dmg : mn.hp;
                                mn.hp -= dealt;
                                if (b.remainingDmg > 0.0f) b.remainingDmg -= dealt;
                                if (mn.hp <= 0.0f) {
                                    mn.alive = false;
                                    AddKillCombo();
                                    float sc = 45.0f;
                                    if (mn.kind == BotnetBoss::MinionKind::Heavy) sc = 80.0f;
                                    if (mn.kind == BotnetBoss::MinionKind::Pulse) sc = 60.0f;
                                    g_GameManager.scoreAccum += sc;
                                    g_GameManager.score = (long long)g_GameManager.scoreAccum;
                                }
                                if (b.remainingDmg <= 0.001f) { b.active = false; consumed = true; }
                                break;
                            }
                        }
                        if (consumed || !b.active) continue;
                        for (int hi = 0; hi < BotnetBoss::NHOST; hi++) {
                            auto& h = nb2->hosts[hi];
                            if (!h.alive) continue;
                            if (SegDist(h.x, h.y, b.prevX, b.prevY, b.x, b.y) < BotnetBoss::HOST_HIT) {
                                float dmg = dmgAt(h.x, h.y);
                                float dealt = (dmg < h.hp) ? dmg : h.hp;
                                h.hp -= dealt;
                                if (b.remainingDmg > 0.0f) b.remainingDmg -= dealt;
                                if (h.hp <= 0.0f) {
                                    h.alive = false;
                                    AddKillCombo();
                                    g_GameManager.scoreAccum += 180.0f;
                                    g_GameManager.score = (long long)g_GameManager.scoreAccum;
                                }
                                if (b.remainingDmg <= 0.001f) { b.active = false; consumed = true; }
                                break;
                            }
                        }
                        if (consumed || !b.active) continue;
                        if (SegDist(nb2->worldX, nb2->worldY,
                                    b.prevX, b.prevY, b.x, b.y) < BotnetBoss::BODY * 0.95f) {
                            float dmg = dmgAt(nb2->worldX, nb2->worldY) * nb2->bodyDamageTakenMult();
                            float dealt = (dmg < nb2->hp) ? dmg : nb2->hp;
                            nb2->hp -= dealt;
                            if (b.remainingDmg > 0.0f) b.remainingDmg -= dealt;
                            if (nb2->hp <= 0.0f) nb2->alive = false;
                            if (b.remainingDmg <= 0.001f) b.active = false;
                        }
                    }
                }

                // FORK.worm ?щ옒??踰???珥앹븣??吏곸꽑??媛濡쒖?瑜대㈃ 洹??留??ル┝.
                //   ?쇰컲?꾩? 踰쎌뿉 留됲? ?뚮㈇(愿??X). 愿?듯깂(remainingDmg>0)? ?듦낵.
                if (g_CentiBoss && g_CentiBoss->alive && !g_CentiBoss->walls.empty()) {
                    for (auto& b : g_Bullets) {
                        if (!b.active || b.isEnemy) continue;
                        if (g_CentiBoss->hitWall(b.prevX, b.prevY, b.x, b.y) && b.remainingDmg <= 0.0f)
                            b.active = false;
                    }
                }

                // FORK.worm child adds vs ?뚮젅?댁뼱 珥앹븣 ????긽 ?쇨꺽 媛?? 寃쏀뿕移?0.
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
                                float dealt = (dmg < mb.hp) ? dmg : mb.hp;
                                mb.hp -= dealt;
                                if (b.remainingDmg > 0.0f) b.remainingDmg -= dealt;
                                if (mb.hp <= 0.0f) {
                                    mb.alive = false;
                                    AddKillCombo();                 // 肄ㅻ낫留? 寃쏀뿕移?0
                                    g_GameManager.scoreAccum += 80.0f;
                                }
                                if (b.remainingDmg <= 0.001f) b.active = false;
                                break;
                            }
                        }
                    }
                }

                // FORK.worm 紐명넻 ?몃뱶 ???쇨꺽 VFX (?곕?吏??癒몃━ HP ?, ?뚮옒?쒕쭔)
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
                            dmg *= cb2->dmgTakenMult;   // ?꾨줈?좏??? 蹂댄넻 ?쇳빐 媛먯냼
                            float dealt = (dmg < cb2->hp) ? dmg : cb2->hp;
                            cb2->hp -= dealt;
                            if (b.remainingDmg > 0.0f) b.remainingDmg -= dealt;
                            if (cb2->hp <= 0.0f) cb2->alive = false;
                            if (b.remainingDmg <= 0.001f) b.active = false;
                        }
                    }
                }

                if (g_TotemBoss && g_TotemBoss->alive) {
                    auto* tb = g_TotemBoss;
                    for (auto& b : g_Bullets) {
                        if (!b.active || b.isEnemy) continue;
                        auto dmgAt = [&](float ex, float ey) {
                            float pd = glm::distance(glm::vec2(pCX, pCY), glm::vec2(ex, ey));
                            float dmg;
                            if (b.remainingDmg > 0.0f)   dmg = b.remainingDmg;
                            else if (b.turretDmg > 0.0f) dmg = b.turretDmg;
                            else dmg = g_Stats.GetBaseDamage()
                                     * g_Stats.GetDamageMultiplier(pd) * b.dmgMult;
                            return dmg * tb->statDamageMult();
                        };
                        bool consumed = false;
                        for (int ti = 0; ti < TotemBoss::N_TOTEM; ti++) {
                            auto& tt = tb->totems[ti];
                            if (!tt.alive) continue;
                            if (SegDist(tt.x, tt.y, b.prevX, b.prevY, b.x, b.y) < TotemBoss::TOTEM_HIT) {
                                float dmg = dmgAt(tt.x, tt.y);
                                float dealt = (dmg < tt.hp) ? dmg : tt.hp;
                                tt.hp -= dealt;
                                tb->onTotemDamaged(ti);
                                if (b.remainingDmg > 0.0f) b.remainingDmg -= dealt;
                                if (tt.hp <= 0.0f) {
                                    tt.alive = false;
                                    tb->onTotemKilled(ti);
                                    AddKillCombo();
                                    g_GameManager.scoreAccum += 120.0f;
                                    g_GameManager.score = (long long)g_GameManager.scoreAccum;
                                }
                                if (b.remainingDmg <= 0.001f) { b.active = false; consumed = true; }
                                break;
                            }
                        }
                        if (consumed || !b.active) continue;
                        if (tb->expanding() && tb->expandShield > 0.0f) {
                            float rr = TotemBoss::BODY * (1.4f + tb->expandPull * 2.2f);
                            if (SegDist(tb->worldX, tb->worldY, b.prevX, b.prevY, b.x, b.y) < rr) {
                                float dmg = dmgAt(tb->worldX, tb->worldY);
                                tb->damageExpandShield(dmg);
                                b.active = false;
                                continue;
                            }
                        }
                        if (tb->vulnerable()) {
                            if (SegDist(tb->worldX, tb->worldY,
                                        b.prevX, b.prevY, b.x, b.y) < TotemBoss::BODY) {
                                float dmg = dmgAt(tb->worldX, tb->worldY);
                                float dealt = (dmg < tb->hp) ? dmg : tb->hp;
                                tb->hp -= dealt;
                                if (b.remainingDmg > 0.0f) b.remainingDmg -= dealt;
                                if (tb->hp <= 0.0f) tb->alive = false;
                                if (b.remainingDmg <= 0.001f) b.active = false;
                            }
                        }
                    }
                }

                // ?대━紐⑦봽 蹂댁뒪 vs ?뚮젅?댁뼱 珥앹븣 (李⑦겕??/ 蹂몄껜 諛섏궗쨌諛⑹뼱留?/ ?몃え EXP)
                if (g_PolyBoss && g_PolyBoss->alive) {
                    auto* pb = g_PolyBoss;
                    for (auto& b : g_Bullets) {
                        if (!b.active || b.isEnemy) continue;
                        float pd = glm::distance(glm::vec2(pCX, pCY),
                                                 glm::vec2(pb->worldX, pb->worldY));
                        float dmg;
                        if (b.remainingDmg > 0.0f)   dmg = b.remainingDmg;
                        else if (b.turretDmg > 0.0f) dmg = b.turretDmg;
                        else dmg = g_Stats.GetBaseDamage()
                                 * g_Stats.GetDamageMultiplier(pd) * b.dmgMult;

                        // 1) ?몃え 臾대━ ??寃⑺뙆 ??EXP 2
                        bool consumed = false;
                        for (auto& s : pb->swarm) {
                            if (!s.alive) continue;
                            if (SegDist(s.x, s.y, b.prevX, b.prevY, b.x, b.y) < 13.0f) {
                                s.alive = false;
                                pb->onTriangleShot();
                                if (b.remainingDmg <= 0.001f) { b.active = false; consumed = true; }
                                break;
                            }
                        }
                        if (consumed || !b.active) continue;

                        // 3) 본체
                        if (SegDist(pb->worldX, pb->worldY,
                                    b.prevX, b.prevY, b.x, b.y) < PolymorphBoss::BODY * 0.55f) {
                            if (!pb->damageable()) {
                                if (b.remainingDmg <= 0.001f) b.active = false;
                            } else {
                                float dealt = (dmg < pb->hp) ? dmg : pb->hp;
                                pb->hp -= dealt;
                                if (b.remainingDmg > 0.0f) b.remainingDmg -= dealt;
                                SpawnDamageNumber(pb->worldX, pb->worldY, dealt, true);
                                if (pb->hp <= 0.0f) pb->alive = false;
                                if (b.remainingDmg <= 0.001f) b.active = false;
                            }
                        }
                    }
                }

                // 泥섏튂 蹂댁긽 ?뺤궛 ??珥앹븣/洹쇱젒???꾨땶 紐⑤뱺 二쎌쓬(?곗뇙??컻쨌?ㅽ궗쨌MK2쨌?댄궧 ??컻 ??
                //   ???ш린????踰덉뵫 EXP/?먯닔/肄ㅻ낫/?≫삁??諛쏅뒗??(scored ?뚮옒洹몃줈 以묐났 諛⑹?).
                auto creditKill = [&](float xpBase, float scoreBase) {
                    AddKillCombo();
                    g_GameManager.xp        += (long long)(xpBase * g_Stats.xpMult);
                    g_Stats.killCount       += 1;
                    g_GameManager.scoreAccum += scoreBase;
                    g_GameManager.score      = (long long)g_GameManager.scoreAccum;
                    g_RunGold               += 1;
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

                // ?щ쭩 ??컻 spawn
                //   + 遺꾩뿴泥??щ쭩 ???묒? ?먯떇 2留덈━ (珥?2?몃?源뚯?)
                std::vector<Monster*> mobBorn;
                for (auto m : g_MonsterManager.monsters) {
                    if (!m->alive && !m->exploded) {
                        if (!m->scored) {       // ?꾩쭅 蹂댁긽 ??諛쏆? 二쎌쓬 ???뺤궛
                            m->scored = true;
                            float xpB, scB; MobKillReward(m->kind, m->splitGen, m->elite, xpB, scB);
                            float rwm = MobRewardMult(m->kind); xpB *= rwm; scB *= rwm;
                            creditKill(xpB + (float)g_Stats.meleeXpBonus, scB);
                            if (m->elite) g_RunGold += 1;
                        }
                        SpawnEnemyExplosion(m->worldX, m->worldY,
                                            m->color.r, m->color.g, m->color.b,
                                            /*big=*/false);
                        m->exploded = true;
                        // 泥섏튂 ?곗텧 ???꾨줈?몄뒪 醫낅즺 ?쒓렇 (媛뺤쟻? ??긽 媛뺤“)
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
                        // ??컻???섎━????二쎌쓣 ???곗졇 ?뚮젅?댁뼱?먭쾶 愿묒뿭 ?쇳빐
                        if (m->elite == 3) {
                            float vbx = m->worldX, vby = m->worldY, vr = 120.0f;
                            SpawnEnemyExplosion(vbx, vby, 1.0f, 0.4f, 0.1f, true);
                            SpawnShockWave(vbx, vby, vr * 1.5f, 0.45f, 1.0f, 0.4f, 0.1f, true);
                            float ppx = playerWin.x + playerWin.width  * 0.5f;
                            float ppy = playerWin.y + playerWin.height * 0.5f;
                            float dpx = ppx - vbx, dpy = ppy - vby;
                            if (dpx*dpx + dpy*dpy < vr*vr) HurtPlayer(g_GameManager.playerHP, 20.0f);
                        }
                        // ?곗뇙 ??컻(DEATH_BLAST) ???щ쭩 ?꾩튂?먯꽌 二쇰? ?곸뿉寃?AoE
                        //   ?덊봽: ?곕?吏 0.8??.3 + ??컻濡?二쎌? 紐뱀? ?ㅼ떆 ???곗쭚(臾댄븳?곗뇙 李⑤떒)
                        if (g_Stats.deathBlast && !m->noBlast) {
                            float blastDmg = g_Stats.GetBaseDamage()
                                           * g_Stats.GetDamageMultiplier(0.0f) * 0.3f;
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
                        SpawnBadSectorZone(m);   // 諛곕뱶 ?뱁꽣 ???щ쭩 ?먮━??媛먯냽 援ъ뿭
                    }
                }
                for (auto* nb : mobBorn) g_MonsterManager.monsters.push_back(nb);
                for (auto r : g_MonsterManager.rangedMobs) {
                    if (!r->alive && !r->exploded) {
                        if (!r->scored) {
                            r->scored = true;
                            creditKill(25.0f + (float)g_Stats.rangedXpBonus, 300.0f);
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
                    g_Bullets.clear();
                    FinishBossKill(g_MonsterManager, 2, 35, screenWidth, screenHeight);
                }

                // C2_RELAY.sys kill reward
                if (g_BotnetBoss && !g_BotnetBoss->alive && !g_BotnetBoss->exploded) {
                    auto* nb2 = g_BotnetBoss;
                    SpawnEnemyExplosion(nb2->worldX, nb2->worldY, 1.0f, 0.3f, 0.9f, true);
                    SpawnEnemyExplosion(nb2->worldX, nb2->worldY, 1.0f, 0.5f, 1.0f, true);
                    SpawnShockWave(nb2->worldX, nb2->worldY, 430.0f, 0.8f, 0.3f, 0.7f, 1.0f);
                    g_ShakeTime = 0.55f; g_ShakeMag = 24.0f;
                    TriggerFlash(0.4f, 0.6f, 1.0f, 0.6f); TriggerHitStop(0.11f);
                    nb2->exploded = true;
                    g_GameManager.scoreAccum += 20000.0f;
                    g_GameManager.score = (long long)g_GameManager.scoreAccum;
                    delete nb2;
                    g_BotnetBoss = nullptr;
                    g_TotalBossKills++;
                    TryUnlockAch(ACH_FIRST_BOSS);
                    if (g_TotalBossKills >= 3) TryUnlockAch(ACH_BOSS_3);
                    g_Bullets.clear();
                    FinishBossKill(g_MonsterManager, 7, 35, screenWidth, screenHeight);
                }

                // FORK.worm ?щ쭩 ??蹂댁긽
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
                    g_Bullets.clear();
                    FinishBossKill(g_MonsterManager, 8, 35, screenWidth, screenHeight);
                }

                // TOTEM.sys 처치 보상
                if (g_TotemBoss && !g_TotemBoss->alive && !g_TotemBoss->exploded) {
                    auto* tb = g_TotemBoss;
                    SpawnEnemyExplosion(tb->worldX, tb->worldY, 0.85f, 0.35f, 1.0f, true);
                    SpawnEnemyExplosion(tb->worldX, tb->worldY, 1.0f, 0.5f, 0.95f, true);
                    SpawnShockWave(tb->worldX, tb->worldY, 440.0f, 0.85f, 0.4f, 0.95f, 1.0f);
                    g_ShakeTime = 0.55f; g_ShakeMag = 24.0f;
                    TriggerFlash(0.8f, 0.4f, 1.0f, 0.6f); TriggerHitStop(0.11f);
                    tb->exploded = true;
                    g_GameManager.scoreAccum += 20000.0f;
                    g_GameManager.score = (long long)g_GameManager.scoreAccum;
                    delete tb;
                    g_TotemBoss = nullptr;
                    g_TotalBossKills++;
                    TryUnlockAch(ACH_FIRST_BOSS);
                    if (g_TotalBossKills >= 3) TryUnlockAch(ACH_BOSS_3);
                    g_Bullets.clear();
                    FinishBossKill(g_MonsterManager, 9, 35, screenWidth, screenHeight);
                }

                if (g_UnknownBoss && !g_UnknownBoss->alive && !g_UnknownBoss->exploded) {
                    auto* ub = g_UnknownBoss;
                    SpawnEnemyExplosion(ub->worldX, ub->worldY, 0.95f, 0.25f, 0.58f, true);
                    SpawnEnemyExplosion(ub->worldX, ub->worldY, 1.0f, 0.45f, 0.75f, true);
                    SpawnShockWave(ub->worldX, ub->worldY, 400.0f, 0.75f, 0.95f, 0.30f, 0.62f);
                    g_ShakeTime = 0.5f; g_ShakeMag = 20.0f;
                    TriggerFlash(0.95f, 0.28f, 0.58f, 0.6f); TriggerHitStop(0.10f);
                    ub->exploded = true;
                    g_GameManager.scoreAccum += 20000.0f;
                    g_GameManager.score = (long long)g_GameManager.scoreAccum;
                    delete ub;
                    g_UnknownBoss = nullptr;
                    g_TotalBossKills++;
                    TryUnlockAch(ACH_FIRST_BOSS);
                    if (g_TotalBossKills >= 3) TryUnlockAch(ACH_BOSS_3);
                    g_Bullets.clear();
                    FinishBossKill(g_MonsterManager, 1, 35, screenWidth, screenHeight);
                }

                // ?대━紐⑦봽 ?щ쭩 ???붾㈃ ?먮났 + 利앷컯 3媛?+ ?먯닔 50% 異붽?
                if (g_PolyBoss && !g_PolyBoss->alive && !g_PolyBoss->exploded) {
                    auto* pb = g_PolyBoss;
                    SpawnEnemyExplosion(pb->worldX, pb->worldY, 0.6f, 0.2f, 0.9f, true);
                    SpawnEnemyExplosion(pb->worldX, pb->worldY, 0.9f, 0.3f, 1.0f, true);
                    SpawnShockWave(pb->worldX, pb->worldY, 450.0f, 0.8f, 0.7f, 0.3f, 1.0f);
                    g_ShakeTime = 0.6f; g_ShakeMag = 24.0f;
                    TriggerFlash(0.7f, 0.3f, 1.0f, 0.7f); TriggerHitStop(0.13f);
                    pb->exploded = true;
                    g_GameManager.scoreAccum += 30000.0f;
                    g_GameManager.score = (long long)g_GameManager.scoreAccum;
                    delete pb;
                    g_PolyBoss = nullptr;
                    g_PolyPrevForm = -1;
                    g_TotalBossKills++;
                    TryUnlockAch(ACH_FIRST_BOSS);
                    if (g_TotalBossKills >= 3) TryUnlockAch(ACH_BOSS_3);
                    g_Bullets.clear();
                    FinishBossKill(g_MonsterManager, 4, 45, screenWidth, screenHeight);
                }

                // ?먰룺蹂??щ쭩 (?먰솕 ????컻 OR 珥앹븣 寃⑺뙆)
                for (auto bm : g_MonsterManager.bombers) {
                    if (!bm->alive && !bm->exploded) {
                        bool blast = bm->arming;
                        // ?먰룺(blast)? 蹂댁긽 ?놁쓬. ?뚮젅?댁뼱媛 二쎌씤 寃쎌슦(!blast)留?
                        //   洹몃━怨??꾩쭅 ?뺤궛 ???먯쑝硫?愿묒뿭 泥섏튂 ?? 蹂댁긽 吏湲?
                        if (!bm->scored && !blast) {
                            bm->scored = true;
                            creditKill(25.0f + (float)g_Stats.meleeXpBonus, 200.0f);
                        }
                        if (blast) {
                            // ?먰룺: ?뚮젅?댁뼱 二쎌쓬湲???컻 (?ㅻ컻 + ?댁쨷 異⑷꺽??
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
                            // 珥앹븣 寃⑺뙆: ?됰쾾????컻 ?뚰떚??+ 醫낅즺 ?쒓렇
                            SpawnEnemyExplosion(bm->worldX, bm->worldY,
                                                0.9f, 0.4f, 0.4f, true);
                            SpawnKillTag(bm->worldX, bm->worldY, 1.0f, 0.5f, 0.45f,
                                         L"ransomware purged", true);
                        }
                        // HACK_BOMBER: ?댄궧 ??컻 VFX (CollisionSystem ?먯꽌 ?뚮옒洹??ㅼ젙??
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
                // VFX 泥댄겕 ?꾨즺 ??二쎌? ?먰룺蹂?硫붾え由??뺣━
                g_MonsterManager.ClearDeadBombers();

                // ?〓す 洹쇱젒 ?곕?吏濡?HP 媛먯냼?덈뒗吏 (???꾨젅??鍮꾧탳)
                if (g_GameManager.playerHP < g_PrevHP - 0.0001f) hit = true;
                if (hit && g_Stats.lightStep)
                    g_Stats.lightStepDisableTimer = g_Stats.lightStepHitLock;
                g_PrevHP = g_GameManager.playerHP;

                // ?덈꺼?? xp 媛 ?꾩슂?됱쓣 ?섏쑝硫??덈꺼??(?⑥? xp ?댁썡) + AUG_SELECT
                if (g_GameManager.currentState == GameState::RUNNING) {
                    long long need = g_ExpSystem.Required(g_GameManager.playerLevel);
                    if (g_GameManager.xp >= need) {
                        g_GameManager.xp -= need;
                        ++g_GameManager.playerLevel;
                        // (?덈꺼????ㅽ겕由??뚮옒???쒓굅 ???덈??? AUG_SELECT 移대뱶濡?異⑸텇???덈궡)
                        g_GameManager.PickAugChoices(g_Stats.sizeAugTaken,
                                                     g_Stats.distAugTaken);

                        g_GameManager.currentState = GameState::AUG_SELECT;
                    }
                }

                // 시간 점수 (휴식·상점 중에는 정지)
                if (!InBossRest())
                    g_GameManager.AddScore(FIXED_DT * 100.0f);
            }
            accumulator -= FIXED_DT;
        }

        // ?? ?먮쭧: ?곕?吏 ?レ옄 / 肄ㅻ낫 / ?뚮옒??留??꾨젅??媛깆떊 (寃뚯엫 吏꾪뻾 以묒뿉留?媛먯뇿) ??
        if (g_GameManager.currentState == GameState::RUNNING ||
            g_GameManager.currentState == GameState::DYING) {
            for (auto& d : g_DmgNumbers) {
                d.x  += d.vx * delta;
                d.y  += d.vy * delta;
                d.vy += 70.0f * delta;        // ?쏀븳 以묐젰 (?잕뎄爾ㅻ떎 泥섏쭚)
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

        if (g_GameManager.currentState == GameState::RUNNING) {
            float pCX = playerWin.x + playerWin.width  * 0.5f;
            float pCY = playerWin.y + playerWin.height * 0.5f;
            bool  moving = keys[GLFW_KEY_W] || keys[GLFW_KEY_S] ||
                           keys[GLFW_KEY_A] || keys[GLFW_KEY_D];

            // ?? ?쇨꺽 ?移???HP 媛먯냼 ??鍮④컙 鍮꾨꽕?몃쭔 (?쒖빞 蹂???쒓굅, 李??ш린 怨좎젙) ??
            {
                if (g_WinPrevHP < 0.0f) g_WinPrevHP = g_GameManager.playerHP;
                float lost = g_WinPrevHP - g_GameManager.playerHP;
                if (lost > 0.5f) { g_HurtVignette = 0.5f; g_HpBarPop = 2.2f; Audio::PlaySfx(Audio::Sfx::Hurt); }
                g_WinPrevHP = g_GameManager.playerHP;
                // 창 크기 — g_Stats.windowSize 기준, 초집중 시 +50% (부드럽게 보간)
                {
                    float targetWin = g_Stats.windowSize *
                        ((g_HyperFocusTimer > 0.0f) ? 1.5f : 1.0f);
                    if (g_WindowSizeCur < 1.0f) g_WindowSizeCur = g_Stats.windowSize;
                    float winStep = std::min(1.0f, delta * 5.5f);
                    g_WindowSizeCur += (targetWin - g_WindowSizeCur) * winStep;
                }
                playerWin.width = playerWin.height = g_WindowSizeCur;
                playerWin.x = pCX - g_WindowSizeCur * 0.5f;
                playerWin.y = pCY - g_WindowSizeCur * 0.5f;
            }
            if (g_HurtVignette > 0.0f) { g_HurtVignette -= delta * 1.6f; if (g_HurtVignette < 0.0f) g_HurtVignette = 0.0f; }
            if (g_HpBarPop > 0.0f) { g_HpBarPop -= delta; if (g_HpBarPop < 0.0f) g_HpBarPop = 0.0f; }

            // HP 재생 (REGEN_UP, 거대화, 재생 II 등 regenPerSec 합산)
            if (g_Stats.regenPerSec > 0.0f) {
                g_GameManager.playerHP += g_Stats.GetRegenRate(g_GameManager.playerHP) * delta;
                if (g_GameManager.playerHP > g_Stats.maxHP)
                    g_GameManager.playerHP = g_Stats.maxHP;
            }
            // 異쒗삁(D_BLEED) ??珥덈떦 HP 媛먯냼 (?ъ깮?쇰줈 ?곸뇙 媛?? 二쎌????딆쓬 ?섑븳 1)
            if (g_Stats.bleedPerSec > 0.0f && g_GameManager.playerHP > 1.0f) {
                g_GameManager.playerHP -= g_Stats.bleedPerSec * delta;
                if (g_GameManager.playerHP < 1.0f) g_GameManager.playerHP = 1.0f;
            }
            // 諛곕뱶 ?뱁꽣 異쒗삁 ??媛먯냽 援ъ뿭 ?덉씠硫?異쒗삁 ??대㉧ 2珥덈줈 媛깆떊, 鍮좎졇?섏????붾쪟
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
                        g_GameManager.playerHP -= 4.0f * delta;   // 異쒗삁 DPS
                        if (g_GameManager.playerHP < 1.0f) g_GameManager.playerHP = 1.0f;
                    }
                }
            }

            // ?꾧컧 諛쒓껄 ???먭굅由??먰룺蹂?(議댁옱?섎㈃ 諛쒓껄 泥섎━)
            if (!g_MonsterManager.rangedMobs.empty()) MarkMobSeenId(CM_RANGED);
            if (!g_MonsterManager.bombers.empty())    MarkMobSeenId(CM_BOMBER);

            // ?낆쟻 議곌굔 泥댄겕 (???먯닔/??蹂댁쑀 利앷컯 湲곗? ??蹂댁뒪 ?낆쟻? 泥섏튂 ?쒖젏?먯꽌 泥섎━)
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

            // 異⑷꺽???낅뜲?댄듃
            for (auto& sw : g_ShockWaves) {
                if (!sw.active) continue;
                sw.life -= delta;
                if (sw.life <= 0.0f) sw.active = false;
            }

            // 寃媛??ㅼ쐷 ?붿긽 ?낅뜲?댄듃
            for (auto& sl : g_Slashes) {
                if (!sl.active) continue;
                sl.life -= delta;
                if (sl.life <= 0.0f) sl.active = false;
            }

            for (auto& lb : g_LaserBeams) lb.life -= delta;
            g_LaserBeams.erase(std::remove_if(g_LaserBeams.begin(), g_LaserBeams.end(),
                [](const LaserBeam& b){ return b.life <= 0.0f; }), g_LaserBeams.end());

            // 諛곕뱶 ?뱁꽣 媛먯냽 援ъ뿭 ?섎챸 + 遺???뺤궛
            for (auto& z : g_SlowZones) { z.life -= delta; z.age += delta; }
            g_SlowZones.erase(std::remove_if(g_SlowZones.begin(), g_SlowZones.end(),
                [](const SlowZone& z){ return z.life <= 0.0f; }), g_SlowZones.end());

            // ?寃??ㅽ뙆???낅뜲?댄듃 (?대룞 + 媛먯냽 + ?섎챸)
            for (auto& sp : g_Sparks) {
                sp.x += sp.vx * delta;
                sp.y += sp.vy * delta;
                float drag = std::max(0.0f, 1.0f - 6.0f * delta);
                sp.vx *= drag; sp.vy *= drag;
                sp.life -= delta;
            }
            g_Sparks.erase(std::remove_if(g_Sparks.begin(), g_Sparks.end(),
                [](const Spark& s){ return s.life <= 0.0f; }), g_Sparks.end());
            // ?대룞 ?붿긽 ?섎챸 媛깆떊
            for (auto& tr : g_Trail) tr.life -= delta;
            g_Trail.erase(std::remove_if(g_Trail.begin(), g_Trail.end(),
                [](const Trail& t){ return t.life <= 0.0f; }), g_Trail.end());
            if (g_MuzzleTimer > 0.0f) g_MuzzleTimer -= delta;

            if (g_ShakeTime > 0.0f) g_ShakeTime -= delta;

            // 珥덈떦 EXP ?꾩쟻 (?ㅺ??ㅻ뒗 二쎌쓬 + ?〓す 媛??
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

            // 痍⑦븿: drunkCooldown + drunkActiveDuration ?ъ씠??(以묐났 ?????뚮씪誘명꽣 蹂??
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

            // ?먯닔 湲곕컲 ?쒖씠???⑦봽 — ACT 테마 (BossDirector)
            BossDir::ActRules act = BossDir::GetActRules();
            float intensity = (float)g_GameManager.score / 100000.0f;
            if (intensity > act.intensityCap) intensity = act.intensityCap;
            float rampSpawn = (1.0f + intensity * 0.40f) * act.spawnMult;
            float rampSpd   = (1.0f + intensity * 0.09f) * act.speedMult;
            bool  bossNow = g_RRBoss || g_PolyBoss || g_BotnetBoss || g_CentiBoss || g_TotemBoss || g_UnknownBoss ||
                            g_BossWarnTimer > 0.0f;
            float hpIntensity = (float)g_GameManager.score / 100000.0f;
            if (hpIntensity > act.hpIntensityCap) hpIntensity = act.hpIntensityCap;
            float rampHp    = 1.0f + hpIntensity * 0.55f;
            int   varietyPct = (int)(std::min(45.0f * g_Stats.varietyChanceMult,
                                   (float)g_GameManager.score / 3000.0f * g_Stats.varietyChanceMult)
                               + (float)act.varietyBias);
            int   elitePct   = (int)(std::min(40.0f,
                                   (float)g_GameManager.score / 12000.0f * g_Stats.eliteChanceMult)
                               + (float)act.eliteBias);
            (void)rampSpd;

            // ?ㅽ룿 ?곸뿭 ??2?섏씠利?以뚯븘?????뺤옣??蹂댁씠?? ?곸뿭 紐⑥꽌由ъ뿉???ㅽ룿.
            float saX = 0.0f, saY = 0.0f;
            int   saW = screenWidth, saH = screenHeight;

            // ?〓す ?ㅽ룿 (D_MOB_SPAWN ?????먯＜ + cap +200, D_MOB_HP ??HP+, ?먯닔 ?⑦봽)
            spawnTimer += delta;
            float spawnInterval = 0.3f * g_Stats.mobSpawnMult / (p2mult * rampSpawn);
            if (bossNow) spawnInterval *= 2.5f;   // 蹂댁뒪?? ?몃옒???ㅽ룿 ???媛먯냼
            float effHpMul = 1.0f;
            if (g_Difficulty == Difficulty::EASY) { spawnInterval *= 1.6f; effHpMul = 0.65f; }
            else if (g_Difficulty == Difficulty::HARD) { effHpMul = 1.1f; }
            // 蹂댁뒪 ?꾨㈃?????쒖꽦 蹂댁뒪媛 ?덉쑝硫??먯뿰 ?〓す/?먭굅由??먰룺 ?ㅽ룿 ?꾩쟾 ?뺤?
            //   (蹂댁뒪媛 吏곸젒 ?뚰솚?섎뒗 adds ??媛?蹂댁뒪 ?대옒???대??먯꽌留?
            auto anyBossAlive = [&]() -> bool {
                if (g_RRBoss && g_RRBoss->alive) return true;
                if (g_PolyBoss && g_PolyBoss->alive) return true;
                if (g_BotnetBoss && g_BotnetBoss->alive) return true;
                if (g_CentiBoss && g_CentiBoss->alive) return true;
                if (g_TotemBoss && g_TotemBoss->alive) return true;
                if (g_UnknownBoss && g_UnknownBoss->alive) return true;
                return false;
            };
            bool bossDuel = anyBossAlive() || g_BossWarnTimer > 0.0f ||
                              g_InBossIntermission;
            if (bossDuel) spawnInterval = 1e9f;
            if (spawnTimer > spawnInterval) {
                // ?덈? ?곹븳 ???대━2?섏씠利댠쀬젏?섎옩?꾨줈 ?쒕룄媛 1000+ 源뚯? ??＜?섎뜕 寃?諛⑹? (?깅뒫)
                int effCap = (int)((100 + g_Stats.mobCapBonus) * p2mult * rampSpawn);
                if (effCap > 180) effCap = 180;
                // ??留덈━ ?ㅽ룿 + ?붾쾭??蹂??(D_MOB_PACK ??援곗쭛?쇰줈 ?щ윭 踰?
                auto spawnOne = [&]() {
                    size_t mbefore = g_MonsterManager.monsters.size();
                    g_MonsterManager.SpawnMob(screenWidth, screenHeight,
                                              effCap,
                                              g_Stats.monsterHpMult * rampHp * effHpMul, saX, saY, saW, saH,
                                              varietyPct, elitePct);
                    // ?붾쾭??蹂댁쑀 ???쇰? 紐뱀쓣 遺꾩뿴泥??먮㈇泥대줈 (?뱀닔 ?〓す ????寃쎌슦留?
                    if (g_MonsterManager.monsters.size() > mbefore) {
                        Monster* nm = g_MonsterManager.monsters.back();
                        if (nm->kind == MobKind::NORMAL) {
                            if (BossDir::ActBiasSplitter() && (rand() % 100) < 32)
                                nm->MakeKind(MobKind::SPLITTER, 0, 1.2f);
                            else if (BossDir::ActBiasSpawner() && (rand() % 100) < 26) {
                                nm->MakeKind(MobKind::SPAWNER);
                                nm->blinkTargetX = 160.0f + (float)(rand() % (screenWidth  > 360 ? screenWidth  - 320 : 1));
                                nm->blinkTargetY = 160.0f + (float)(rand() % (screenHeight > 360 ? screenHeight - 320 : 1));
                            }
                            else if (BossDir::ActBiasShielded() && (rand() % 100) < 28)
                                nm->MakeKind(MobKind::SHIELDED);
                            else if (g_Stats.splitterMobs && (rand() % 100) < 25)
                                nm->MakeKind(MobKind::SPLITTER, 0, 1.2f);
                            else if (g_Stats.blinkerMobs && (rand() % 100) < 25)
                                nm->MakeKind(MobKind::BLINKER);
                            else if (g_Stats.orbiterMobs && (rand() % 100) < 22)
                                nm->MakeKind(MobKind::ORBITER);
                            else if (g_Stats.spawnerMobs && (rand() % 100) < 14) {
                                nm->MakeKind(MobKind::SPAWNER);
                                // ?뚮젅?댁뼱瑜?已볦? ?딄쾶 ???붾㈃ ???꾩쓽 吏?먯쓣 諛곗튂 紐⑺몴濡?(媛?μ옄由??щ갚 ??
                                nm->blinkTargetX = 160.0f + (float)(rand() % (screenWidth  > 360 ? screenWidth  - 320 : 1));
                                nm->blinkTargetY = 160.0f + (float)(rand() % (screenHeight > 360 ? screenHeight - 320 : 1));
                            }
                            else if (g_Stats.shieldedMobs && (rand() % 100) < 22)
                                nm->MakeKind(MobKind::SHIELDED);
                            else if (g_Stats.ddosMobs && (rand() % 100) < 10)
                                nm->MakeKind(MobKind::DDOS);
                            // 諛곕뱶 ?뱁꽣(M)
                            else if (g_GameManager.score < 500000 &&
                                     ((g_Stats.badsectorMobs && (rand() % 100) < 12) ||
                                      (rand() % 1000) < 5))
                                nm->MakeKind(MobKind::BADSECTOR);
                            // ?덉??ㅽ듃由??먮윭(M) ?????ш?. 50留뚯젏 ?섏쑝硫?誘몃벑??
                            else if (g_GameManager.score < 500000 &&
                                     ((g_Stats.regerrorMobs && (rand() % 100) < 8) ||
                                      (rand() % 1000) < 2))
                                nm->MakeKind(MobKind::REGERROR);
                            // ?붾룄????22留뚯젏 ?댄썑쨌蹂댁뒪???쒖쇅. 1留덈━ ?꾩＜, cap 10~18%.
                            else if (!bossDuel && g_GameManager.score > 220000 &&
                                     (rand() % 100) < (g_GameManager.score < 400000 ? 4 : 8)) {
                                int ddosCount = 0;
                                for (auto* mm2 : g_MonsterManager.monsters)
                                    if (mm2->alive && mm2->kind == MobKind::DDOS) ++ddosCount;
                                float capMul = (g_GameManager.score < 350000) ? 0.10f : 0.18f;
                                int ddosCap = (int)((float)effCap * capMul);
                                if (ddosCap < 2) ddosCap = 2;
                                if (ddosCount < ddosCap) {
                                    nm->MakeKind(MobKind::DDOS);
                                    if (g_GameManager.score > 450000 && ddosCount + 1 < ddosCap &&
                                        (int)g_MonsterManager.monsters.size() < effCap) {
                                        Monster* dn = new Monster(
                                            nm->worldX + (float)(rand() % 50 - 25),
                                            nm->worldY + (float)(rand() % 50 - 25),
                                            g_Stats.monsterHpMult * rampHp * effHpMul, 1.0f, false);
                                        dn->MakeKind(MobKind::DDOS);
                                        g_MonsterManager.monsters.push_back(dn);
                                    }
                                }
                            }
                        }
                        // ?ㅼ?伊대윭 媛뺥솕 ???뱀닔(鍮?NORMAL) ?〓す HP 異붽? 諛곗쑉
                        if (nm->kind != MobKind::NORMAL && g_Stats.specialMobHpMult != 1.0f)
                            nm->hp *= g_Stats.specialMobHpMult;
                        // ?몃줈?대ぉ留?媛뺥솕 ???먮㈇泥??몃줈?대ぉ留? ?먮㈇ ?ъ궗?⑹떆媛??⑥텞
                        if (nm->kind == MobKind::BLINKER && g_Stats.trojanBoost)
                            nm->blinkIntervalMul = 0.55f;
                        if (nm->kind == MobKind::WEAVER && g_Stats.weaverBoost) {
                            nm->speed    *= 1.15f;
                            nm->weaveAmp  = 0.98f;
                        }
                        if (nm->kind == MobKind::BRUTE && g_Stats.bruteBoost) {
                            nm->hp        *= 1.25f;
                            nm->contactDmg = 6.0f;
                        }
                    }
                };
                int packN = 1 + g_Stats.mobPackBonus;       // D_MOB_PACK: 援곗쭛 ?ㅽ룿
                for (int p = 0; p < packN; p++) spawnOne();
                spawnTimer = 0.0f;
            }

            g_GameTime += delta;

            // 蹂댁뒪 ?ㅽ룿 ???쇰컲 蹂댁뒪 20留뚯젏留덈떎 / ?대━紐⑦봽 50留뚯젏 怨좎젙.
            //   (?대뼡 蹂댁뒪???댁븘?덉쑝硫??湲?= ?숈떆 ?ㅽ룿 諛⑹?)
            {
                bool bossActive = g_RRBoss || g_PolyBoss || g_BotnetBoss || g_CentiBoss || g_TotemBoss || g_UnknownBoss ||
                                  g_BossWarnTimer > 0.0f;
                // ?쇱슫?? ??蹂댁뒪 ?덈뜦??李⑤떒: 蹂댁뒪瑜??≪븘 ?꾩쟾???뺣━?섎뒗 ?쒓컙(?쒖꽦?믩퉬?쒖꽦),
                //   ?ㅼ쓬 蹂댁뒪 ?꾧퀎媛믪쓣 ?꾩옱 ?먯닔+20留뚯쑝濡?由щ쿋?댁뒪 ??理쒖냼 20留뚯젏 ?댁떇 蹂댁옣
                //   (蹂댁뒪??以??볦씤 ?먯닔濡??≪옄留덉옄 ??蹂댁뒪 ?⑤뜕 ?낆닚???쒓굅).
                static bool s_prevBossActive = false;
                if (s_prevBossActive && !bossActive) {
                    long long rebase = (long long)g_GameManager.score + 200000;
                    if (g_NextBossScore < rebase) g_NextBossScore = rebase;
                }
                s_prevBossActive = bossActive;
                float bossHpC = GetDifficultyParams(g_Difficulty).bossHp;
                float polyHpC = (g_Difficulty == Difficulty::EASY) ? 10000.0f
                              : (g_Difficulty == Difficulty::HARD) ? 75000.0f : 30000.0f;

                // 예고 시작 — pick·이름·HP 확정 후 짧은 경고(크리에이티브 0.35s)
                auto startWarn = [&](int pick, const wchar_t* name, float hp) {
                    StartBossWarn(pick, name, hp);
                };

                if (!bossActive) {
                    if (g_CreativeBossPending) {
                        g_CreativeBossPending = false;
                        QueueCreativeBossPick(g_CreativeBossPick >= 0 ? g_CreativeBossPick : 2,
                                              bossHpC, polyHpC);
                    }
                    else if (g_GameManager.score >= g_NextBossScore) {
                        // 20留뚯젏留덈떎 濡쒗뀒?댁뀡 (50留?1???대━ 怨좎젙)
                        g_NextBossScore += 200000;
                        float sc = 0.36f + (float)g_GameManager.score / 300000.0f;
                        if (sc > 9.0f) sc = 9.0f;
                        sc *= (1.0f + (float)g_GameManager.playerLevel * 0.022f);
                        float bossHp = GetDifficultyParams(g_Difficulty).bossHp * sc;
                        {
                            int pick = BossDir::RollScorePick();
                            startWarn(pick, BossDir::DisplayName(pick),
                                      bossHp * BossDir::HpMul(pick));
                        }
                    }
                }

                // ?꾩“ 吏꾪뻾 ??留뚮즺 ???ㅼ젣 蹂댁뒪 ?앹꽦 + ?깆옣 ?곗텧
                if (g_BossWarnTimer > 0.0f) {
                    g_BossWarnTimer -= delta;
                    if (g_BossWarnTimer <= 0.0f) {
                        g_BossWarnTimer = 0.0f;
                        // C15: ?뚰솚 ?꾩튂 ?쒕뜡??????긽 以묒븰 洹쇱쿂??嫄곌린??罹좏븨/?룰굔 ?뚰쎕?섎뜕 臾몄젣.
                        //   ?붾㈃ ?덉そ(媛?μ옄由?留덉쭊 ?쒖쇅) ???곸뿭?먯꽌 臾댁옉???꾩튂.
                        float bMargin = 340.0f * g_Scale;
                        float bRangeX = std::max(1.0f, (float)screenWidth  - 2.0f * bMargin);
                        float bRangeY = std::max(1.0f, (float)screenHeight - 2.0f * bMargin);
                        float bsx = bMargin + (float)(rand() % (int)bRangeX);
                        float bsy = bMargin + (float)(rand() % (int)bRangeY);
                        switch (g_BossWarnPick) {
                        case 1:
                            g_UnknownBoss = new UnknownBoss(screenWidth, screenHeight, g_BossWarnHp);
                            g_UnknownBoss->worldX = bsx; g_UnknownBoss->worldY = bsy;
                            break;
                        case 2:
                            g_RRBoss = new ReloadRunnerBoss(screenWidth, screenHeight, g_BossWarnHp);
                            g_RRBoss->worldX = bsx; g_RRBoss->worldY = bsy;
                            break;
                        case 4:
                            g_PolyBoss = new PolymorphBoss(screenWidth, screenHeight, g_BossWarnHp);
                            g_PolyBoss->worldX = bsx; g_PolyBoss->worldY = bsy;
                            g_PolyPrevForm = -1;
                            break;
                        case 7:
                            g_BotnetBoss = new BotnetBoss(screenWidth, screenHeight, g_BossWarnHp);
                            g_BotnetBoss->worldX = bsx; g_BotnetBoss->worldY = bsy;
                            // ?쒖닔 ?꾨㈃?????〓す ?≪닔 + 珥덇린 ?⑦궥 ?ъ떆
                            {
                                float absorb = 0.0f;
                                for (auto* m  : g_MonsterManager.monsters)   if (m->alive)  absorb += m->hp;
                                for (auto* r  : g_MonsterManager.rangedMobs)  if (r->alive)  absorb += r->hp;
                                for (auto* bm : g_MonsterManager.bombers)     if (bm->alive) absorb += bm->hp;
                                g_BotnetBoss->hp += absorb * 0.35f;
                                g_BotnetBoss->maxHp += absorb * 0.35f;
                                for (auto* m  : g_MonsterManager.monsters)   delete m;
                                g_MonsterManager.monsters.clear();
                                for (auto* r  : g_MonsterManager.rangedMobs)  delete r;
                                g_MonsterManager.rangedMobs.clear();
                                for (auto* bm : g_MonsterManager.bombers)     delete bm;
                                g_MonsterManager.bombers.clear();
                                {
                                    float pCX = playerWin.x + playerWin.width  * 0.5f;
                                    float pCY = playerWin.y + playerWin.height * 0.5f;
                                    g_BotnetBoss->deployRushFromHosts(3, pCX, pCY);
                                    g_BotnetBoss->deployMapRush(pCX, pCY, 3);
                                }
                                g_ShakeTime = 0.45f; g_ShakeMag = 16.0f;
                            }
                            break;
                        case 8:
                            g_CentiBoss = new CentipedeBoss(screenWidth, screenHeight, g_BossWarnHp);
                            g_CentiBoss->worldX = bsx; g_CentiBoss->worldY = bsy;
                            // ?? ?꾨줈?좏??? ?꾩옱 ?〓す ?꾩껜 泥대젰???≪닔(?곹븳) ???쒖닔 蹂댁뒪????
                            //   ?〓す? 蹂댁뒪濡??≪닔?섏뼱 ?щ씪吏怨? 蹂댁뒪???숈븞 異붽? ?ㅽ룿 ????
                            {
                                float absorb = 0.0f;
                                for (auto* m  : g_MonsterManager.monsters)   if (m->alive)  absorb += m->hp;
                                for (auto* r  : g_MonsterManager.rangedMobs)  if (r->alive)  absorb += r->hp;
                                for (auto* bm : g_MonsterManager.bombers)     if (bm->alive) absorb += bm->hp;
                                // ?곹븳 ?쒓굅 ??吏꾩쭨 '?〓す ?꾩껜 泥대젰 + 蹂댁뒪 泥대젰'. ?꾨컲 ?깆빱 ?〓す
                                //   horde 媛 留롮쓣?섎줉 蹂댁뒪??洹몃쭔???⑤떒(?〓す蹂대떎 鍮⑤━ 二쎈뒗 臾몄젣 ?닿껐).
                                g_CentiBoss->hp += absorb; g_CentiBoss->maxHp += absorb;
                                // ?〓す ?≪닔 ???붾㈃ ?뺣━(?쒖닔 ???
                                for (auto* m  : g_MonsterManager.monsters)   delete m;
                                g_MonsterManager.monsters.clear();
                                for (auto* r  : g_MonsterManager.rangedMobs)  delete r;
                                g_MonsterManager.rangedMobs.clear();
                                for (auto* bm : g_MonsterManager.bombers)     delete bm;
                                g_MonsterManager.bombers.clear();
                                g_ShakeTime = 0.4f; g_ShakeMag = 14.0f;   // ?≪닔 ?쒓컙 吏꾨룞
                            }
                            g_CentiBoss->enterSpawn();   // ?깆옣 紐⑥뀡 ???붾㈃ 諛뽰뿉??怨≪꽑 ?뚯쭊?쇰줈 ?낆옣
                            break;
                        case 9:
                            g_TotemBoss = new TotemBoss(screenWidth, screenHeight, g_BossWarnHp);
                            g_TotemBoss->worldX = bsx; g_TotemBoss->worldY = bsy;
                            break;
                        default:
                            g_RRBoss = new ReloadRunnerBoss(screenWidth, screenHeight, g_BossWarnHp);
                            g_RRBoss->worldX = bsx; g_RRBoss->worldY = bsy;
                            break;
                        }
                        if (g_BossWarnPick >= 0)
                            MarkBossSeenPick(g_BossWarnPick);
                        // 공통 등장 연출
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

            // ?먰룺蹂?spawn (?쒖씠?꾨퀎 ?쒖옉 ?쒓컙/二쇨린, ?ъ?? ???섏샂)
            DifficultyParams dp = GetDifficultyParams(g_Difficulty);
            if (g_GameTime >= dp.bomberStartTime && !bossDuel) {
                g_BomberSpawnTimer += delta;
                float bomberInt = dp.bomberInterval / (p2mult * rampSpawn);
                if (bossNow) bomberInt *= 2.0f;
                if (g_BomberSpawnTimer >= bomberInt) {
                    g_MonsterManager.SpawnBomber(screenWidth, screenHeight,
                                                 g_Stats.bomberHpMult * rampHp,
                                                 g_Stats.bomberSpeedMult,
                                                 g_Stats.bomberBlastMult,
                                                 saX, saY, saW, saH);
                    g_BomberSpawnTimer = 0.0f;
                }
            }

            // ?먭굅由?紐??ㅽ룿 ???쒖씠?꾨퀎 + ?붾쾭??(D_RMOB_MAX, rmobSpawnDelayBonus)
            rangedSpawnTimer += delta;
            float rangedInterval = dp.rangedSpawnInterval - g_Stats.rmobSpawnDelayBonus;
            if (rangedInterval < 1.0f) rangedInterval = 1.0f;
            rangedInterval /= (p2mult * rampSpawn);
            if (bossNow) rangedInterval *= 2.0f;
            int rangedMax = (int)((dp.rangedMaxBase + g_Stats.rmobMaxBonus) * p2mult
                                  + intensity * 2.0f);           // ?먯닔???숈떆 +2
            if (rangedMax > 16) rangedMax = 16;   // 李?媛쒖닔 = scissor ?⑥뒪 ?????곹븳 (?깅뒫)
            if (rangedSpawnTimer > rangedInterval && !bossDuel) {
                g_MonsterManager.SpawnRangedMob(screenWidth, screenHeight,
                    g_Stats.rmobHpMult * rampHp, rangedMax, saX, saY, saW, saH);
                rangedSpawnTimer = 0.0f;
            }

            // ?ㅺ??ㅻ뒗 二쎌쓬: 紐⑤뱺 ?ㅻ툕 ?곸썝??異붽꺽 + ?묒큺 ?곕?吏
            // ?ㅽ깮 1??= 湲곕낯 ?띾룄, 2???댁긽遺??+20%/?ㅽ깮
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

            // ?쒕줎: ?뚮젅?댁뼱 二쇱쐞 怨듭쟾 + ?먮룞 諛쒖궗 (?λ젰移?50%), 1~2湲?            // ?ы깙 紐⑤뱶 ?쒖꽦 ???쒕줎? 怨듭쟾/諛쒖궗 ?딄퀬 ?ы깙?쇰줈 ?泥대맖
            if (g_Stats.drone && !g_Stats.turretMode) {
                for (int d = 0; d < g_Stats.droneCount && d < MAX_DRONES; d++) {
                    auto& dr = g_Drones[d];
                    dr.angle += 1.5f * delta;
                    // 洹좊벑 媛곷룄 ?ㅽ봽??(媛곸옄 ?ㅻⅨ ?꾩튂)
                    float ang = dr.angle + (float)d * 6.2831853f / (float)g_Stats.droneCount;
                    float droneX = pCX + cosf(ang) * 80.0f;
                    float droneY = pCY + sinf(ang) * 80.0f;
                    dr.fireTimer += delta;
                    // 援곗쭛 吏???좏솕) ??諛쒖궗 媛꾧꺽 2.0횞 ??0.6횞 (珥덇퀬??
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

            // ?ы깙 諛곗튂 (CANNON + DRONE_2 議고빀)
            //   1珥덈쭏???뚮젅?댁뼱 ?꾩튂???ы깙 1媛?諛곗튂, 媛?5珥?吏????留듭뿉 ~5媛??곸떆
            if (g_Stats.turretMode) {
                // ?뚯킑 湲곗? ?λ젰移??ш퀎??(蹂댁쑀 利앷컯 媛쒖닔 蹂???뚮쭔)
                static size_t s_lastTurretAugCount = (size_t)-1;
                if (g_OwnedAugs.size() != s_lastTurretAugCount) {
                    s_lastTurretAugCount = g_OwnedAugs.size();
                    g_TurretStats = PlayerStats();
                    ApplyWeapon(g_TurretStats, StartWeapon::RIFLE);
                    for (int oi : g_OwnedAugs) g_TurretStats.Apply(ALL_AUGS[oi].type);
                }

                // 1珥덈쭏?????ы깙 諛곗튂 (?뚮젅?댁뼱 ?꾩튂)
                g_TurretDeployTimer += delta;
                if (g_TurretDeployTimer >= TURRET_DEPLOY) {
                    g_TurretDeployTimer -= TURRET_DEPLOY;
                    if ((int)g_Turrets.size() < MAX_TURRETS) {
                        Turret t; t.x = pCX; t.y = pCY;
                        g_Turrets.push_back(t);
                    }
                }

                // 媛??ы깙: ?섎챸 + ?뚯킑 諛쒖궗
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
                            nb.color     = glm::vec3(0.1f, 1.0f, 0.55f);  // 泥?줉 (?ы깙 怨좎쑀??
                            nb.turretDmg = g_TurretStats.GetBaseDamage()
                                         * g_TurretStats.GetDamageMultiplier(dist);
                            if (nb.turretDmg < 1.0f) nb.turretDmg = 1.0f;
                            g_Bullets.push_back(nb);
                        }
                    }
                }
                // 留뚮즺???ы깙 ?쒓굅
                g_Turrets.erase(
                    std::remove_if(g_Turrets.begin(), g_Turrets.end(),
                        [](const Turret& t){ return t.lifeTimer >= TURRET_LIFE; }),
                    g_Turrets.end());
            }

            // 李⑦겕?? 怨듭쟾 + ?〓す 利됱궗 + ??珥앹븣 異⑸룎 + ?ъ깮??(n 媛?
            if (g_Stats.chakram) {
                for (int c = 0; c < g_Stats.chakramCount && c < MAX_CHAKRAMS; c++) {
                    auto& ch = g_Chakrams[c];
                    if (ch.alive) {
                        ch.angle += 5.5f * delta;
                        float chx = pCX + cosf(ch.angle) * CHAKRAM_RADIUS;
                        float chy = pCY + sinf(ch.angle) * CHAKRAM_RADIUS;
                        float hitR2 = CHAKRAM_SIZE * CHAKRAM_SIZE;
                        // 특이점 — 끌어당김/소각은 고정틱(몹 AI 전후). 여기선 차크람 생존만.
                        // 잡몹 접촉 즉사 (특이점 제외 시)
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
                        // 자폭병
                        for (auto bm : g_MonsterManager.bombers) {
                            if (!bm->alive) continue;
                            float ddx = bm->worldX - chx, ddy = bm->worldY - chy;
                            if (ddx*ddx + ddy*ddy < hitR2) { bm->hp = 0.0f; bm->alive = false; ch.hp -= 3.0f; }
                        }
                        // ?먭굅由?紐??묒큺 ?????쇳빐
                        for (auto rr : g_MonsterManager.rangedMobs) {
                            if (!rr->alive) continue;
                            float ddx = rr->worldX - chx, ddy = rr->worldY - chy;
                            if (ddx*ddx + ddy*ddy < hitR2) { rr->hp -= 120.0f; if (rr->hp <= 0) rr->alive = false; ch.hp -= 3.0f; }
                        }
                        // ??珥앹븣 留됯린
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
                            ch.hp    = ch.maxHp;
                        }
                    }
                }
            }

            if (g_Stats.bulletRain) {
                g_BulletRainTimer += delta;
                // 臾댄븳 ?몃?(?좏솕) ??泥섏튂留덈떎 荑⑤떎??吏꾪뻾 媛??0.4s/泥섏튂). 誘몃낫????泥섏튂 移댁슫?몃쭔 鍮꾩?.
                if (g_Stats.rainKillReduce && g_RainKillAccum > 0.0f) {
                    float killBoost = g_RainKillAccum * 0.4f;
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
                        nb.speed      = g_Stats.bulletSpeed * 1.05f;  // 理쒓퀬??媛???? ??SAM 誘몄궗?쇱떇
                        nb.color      = glm::vec3(1.0f, 0.5f, 0.2f);
                        nb.homing     = true;
                        nb.homingTurn = 5.0f;   // rad/s
                        nb.dmgMult    = 0.5f;
                        // 지연 미사일 — 발사 직후 5%에서 시작해 ~0.7초에 100% 가속
                        nb.launchRamp  = 0.05f;
                        nb.launchAccel = 1.35f;
                        // (?꾪솚?몃? ??湲곗〈 ?쇰컲 ?꾪솚 ?뚮뜑濡?蹂듭썝. 濡쒖폆 ?ㅽ봽?쇱씠??誘몄궗??
                        g_Bullets.push_back(nb);
                    }
                }
            }

            // 怨좎옣??議곗????ㅻ툕 諛고쉶
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

            // 珥앹븣 諛쒖궗 (?곹샎?섑솗/誘몃땲??蹂대꼫?? Twin 2諛? Cannon ?붿〈 ?곕?吏 罹먯떛)
            // ?ы깙 紐⑤뱶: ?뚮젅?댁뼱 ?섎룞 諛쒖궗 鍮꾪솢??(?ы깙?????諛쒖궗)
            float effInterval = g_Stats.fireInterval * g_Stats.GetFireIntervalMult();
            // ??? ?곗궗 %??怨듦꺽?μ쑝濡쒕쭔 ?섏궛?섍퀬, ?ㅼ젣 諛쒖궗??1珥?怨좎젙
            if (g_Stats.cannon) effInterval = 1.0f;
            // 怨쇰??????곗궗 횞2 (諛쒖궗 媛꾧꺽 ?덈컲)
            float effSpeed    = g_Stats.bulletSpeed   + g_Stats.GetBulletSpeedBonus();
            if (!g_Stats.turretMode) fireTimer += delta;

            // ??諛?spawn ?ы띁 (Twin / Cannon / 痍⑦븿 / brokenSight 怨듯넻)
            // bulletSpread > 0 硫?諛쒖궗 諛⑺뼢???쒕뜡 ?붾뱾湲??곸슜
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
                // ?곗뇙 ?묒슜(由ъ퐫?? ???뺢? ?잛닔 + ?곕?吏 諛곗쑉(-30%) ?곸슜
                if (g_Stats.ricochetMax > 0) {
                    nb.bouncesLeft = g_Stats.ricochetMax;
                    nb.dmgMult    *= g_Stats.ricochetDmgMult;
                }
                if (g_Stats.berserk) {             // 광전사: 체력 낮을수록 최대 +60%
                    float hpFrac = g_Stats.maxHP > 0.0f
                                 ? g_GameManager.playerHP / g_Stats.maxHP : 1.0f;
                    if (hpFrac < 0.0f) hpFrac = 0.0f; else if (hpFrac > 1.0f) hpFrac = 1.0f;
                    float bMult = 1.0f + (1.0f - hpFrac) * 0.6f;
                    nb.dmgMult *= bMult;
                    if (nb.remainingDmg > 0.0f) nb.remainingDmg *= bMult;
                }
                if (g_Stats.bowWeapon) {           // 沅곸닔 ???붿궡 鍮꾩＜??(湲몄춬쨌媛덉깋??
                    nb.color     = glm::vec3(0.75f, 0.95f, 0.45f);
                    nb.sizeScale = 1.7f;
                }
                if (g_DrunkActive) {               // 痍⑦븿 ?쒖꽦 ??議곗? ?먰듃?ъ쭚 + ?곕?吏 -40%
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
                    if (g_RevolverRound == 5)
                        nb.dmgMult *= g_Stats.critMult;
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
                        if (g_Stats.berserk) {             // 광전사
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
                        if (g_DrunkActive) {               // 痍⑦븿 ?쒖꽦 ???곕?吏 -40%
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
                    int   n = (g_Stats.twinCount < 2) ? 2 : g_Stats.twinCount;  // 2諛??붾툝)/3諛??몃━??
                    for (int s = 0; s < n; s++) {
                        // 以묒떖 湲곗? ?移?遺梨꾧섦 (-off ??+off)
                        float a = (n > 1) ? ang + (((float)s / (float)(n - 1)) - 0.5f) * (2.0f * off)
                                          : ang;
                        spawnOne(pCX + cosf(a) * r, pCY + sinf(a) * r);
                    }
                } else {
                    spawnOne(tx, ty);
                }
            };

            // 寃媛?洹쇱젒 ?ㅼ쐷 ??議곗? 諛⑺뼢 ??arc) ?덉쓽 紐⑤뱺 ?곸뿉寃?利됱떆 ?쇳빐
            auto meleeSwing = [&](float ang) {
                float range = 190.0f * g_Stats.playerSizeMult * (g_Stats.meleeWide ? 1.25f : 1.0f);
                float r2 = range * range;
                float halfArc = g_Stats.meleeWide ? 1.5f : 1.15f;  // 愿묓룺 踰좉린: ???뺣?
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
                          * 1.8f * critMult * bMult;       // 洹쇱젒 蹂대꼫??횞1.8
                if (g_TotemBoss && g_TotemBoss->alive) dmg *= g_TotemBoss->statDamageMult();
                auto inCone = [&](float ex, float ey) -> bool {
                    float dx = ex - pCX, dy = ey - pCY, d2 = dx*dx + dy*dy;
                    if (d2 > r2) return false;
                    float diff = atan2f(dy, dx) - ang;
                    while (diff >  3.14159265f) diff -= 6.2831853f;
                    while (diff < -3.14159265f) diff += 6.2831853f;
                    return fabsf(diff) <= halfArc;
                };
                auto onKill = [&]() {   // ?≫삁???≫삁留?怨듯넻 泥섎━
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
                for (auto m : g_MonsterManager.monsters) {        // ?〓す
                    if (!m->alive || !inCone(m->worldX, m->worldY)) continue;
                    float dmgM = dmg;
                    if (m->kind == MobKind::SHIELDED && m->shieldActive) dmgM *= 0.15f;
                    float dealt = (dmgM < m->hp) ? dmgM : m->hp; m->hp -= dealt;
                    SpawnDamageNumber(m->worldX, m->worldY, dealt, dealt >= 40.0f || crit);
                    if (m->hp <= 0.0f) {
                        m->alive = false; m->scored = true; AddKillCombo();
                        float bx, bs; MobKillReward(m->kind, m->splitGen, m->elite, bx, bs);
                        float rwm = MobRewardMult(m->kind); bx *= rwm; bs *= rwm;
                        g_GameManager.xp += (long long)((bx + (float)g_Stats.meleeXpBonus) * g_Stats.xpMult);
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
                        g_GameManager.xp += (long long)((25.0f + (float)g_Stats.rangedXpBonus) * g_Stats.xpMult);
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
                        g_GameManager.xp += (long long)((25.0f + (float)g_Stats.meleeXpBonus) * g_Stats.xpMult);
                        g_Stats.killCount++; g_GameManager.scoreAccum += 200.0f;
                        g_GameManager.score = (long long)g_GameManager.scoreAccum;
                        onKill();
                    }
                }
                // 蹂댁뒪瑜???hp留?媛먯냼 (蹂댁긽/?곗텧? 媛??щ쭩 釉붾줉???대떦)
                auto hitB = [&](float ex, float ey, float& hp, bool& al) {
                    if (!inCone(ex, ey)) return;
                    float dealt = (dmg < hp) ? dmg : hp; hp -= dealt;
                    SpawnDamageNumber(ex, ey, dealt, dealt >= 40.0f || crit);
                    if (hp <= 0.0f) al = false;
                };
                if (g_RRBoss && g_RRBoss->alive)
                    hitB(g_RRBoss->worldX, g_RRBoss->worldY, g_RRBoss->hp, g_RRBoss->alive);
                if (g_PolyBoss && g_PolyBoss->alive && g_PolyBoss->damageable())
                    hitB(g_PolyBoss->worldX, g_PolyBoss->worldY, g_PolyBoss->hp, g_PolyBoss->alive);
                if (g_BotnetBoss && g_BotnetBoss->alive)
                    hitB(g_BotnetBoss->worldX, g_BotnetBoss->worldY, g_BotnetBoss->hp, g_BotnetBoss->alive);
                if (g_CentiBoss && g_CentiBoss->alive && g_CentiBoss->vulnerable())
                    hitB(g_CentiBoss->worldX, g_CentiBoss->worldY, g_CentiBoss->hp, g_CentiBoss->alive);
                if (g_TotemBoss && g_TotemBoss->alive && g_TotemBoss->vulnerable())
                    hitB(g_TotemBoss->worldX, g_TotemBoss->worldY, g_TotemBoss->hp, g_TotemBoss->alive);
                if (g_UnknownBoss && g_UnknownBoss->alive)
                    hitB(g_UnknownBoss->worldX, g_UnknownBoss->worldY, g_UnknownBoss->hp, g_UnknownBoss->alive);
                if (g_TotemBoss && g_TotemBoss->alive) {
                    for (int ti = 0; ti < TotemBoss::N_TOTEM; ti++) {
                        auto& tt = g_TotemBoss->totems[ti];
                        if (!tt.alive || !inCone(tt.x, tt.y)) continue;
                        tt.hp -= dmg;
                        g_TotemBoss->onTotemDamaged(ti);
                        if (tt.hp <= 0.0f) { tt.alive = false; g_TotemBoss->onTotemKilled(ti); }
                    }
                }
                SpawnSlash(pCX, pCY, ang, range);
                TriggerHitStop(0.015f);
                // 移쇰컮?????ㅼ쐷留덈떎 ?꾨갑?쇰줈 愿???ъ궗泥?(洹쇱젒???먭굅由?寃ъ젣)
                if (g_Stats.bladeWind) {
                    Bullet bw(pCX, pCY, pCX + cosf(ang)*200.0f, pCY + sinf(ang)*200.0f);
                    bw.speed       = 900.0f;
                    bw.maxRange    = 700.0f;
                    bw.sizeScale   = 2.2f;
                    bw.color       = glm::vec3(0.7f, 0.95f, 1.0f);
                    bw.remainingDmg = dmg * 0.6f;   // 愿????ъ떇) ??洹쇱젒 ?곕?吏??60%
                    g_Bullets.push_back(bw);
                }
            };

            // ?? 議곗? ?源??ы띁: 醫뚰겢由?而ㅼ꽌 ?쇱젏?? ?먮룞(?대┃X)=理쒓렐???? ?먮룞?몃뜲 ???놁쑝硫?false.
            auto aimTarget = [&](float& tx, float& ty) -> bool {
                if (lmb) { tx = wmx; ty = wmy; return true; }
                return findNearestEnemy(pCX, pCY, tx, ty);
            };

            // ?? ?ㅼ틪 ?덉씠? (利앷컯) ??0.7珥덈쭏??議곗? 諛⑺뼢 愿??鍮?(援곗쨷?쒖뼱) ??
            //   meleeSwing ???곕?吏/泥섏튂蹂댁긽 猷⑦봽瑜?'吏곸꽑 ?먯젙(SegDist)' 踰꾩쟾?쇰줈 ?ъ궗??
            if (g_Stats.laser) {
                float laserInt = (g_Stats.laserTier >= 3) ? 0.18f   // ?좏솕 ?섎졃: 嫄곗쓽 ?곗냽
                               : (g_Stats.laserTier >= 2) ? 0.55f : LASER_INT;
                g_LaserTimer += delta;
                if (g_LaserTimer >= laserInt) {
                    g_LaserTimer -= laserInt;
                    float lang  = atan2f(wmy - pCY, wmx - pCX);   // ?덉씠?????긽 而ㅼ꽌 諛⑺뼢
                    // ?ш굅由щ뒗 II(760)?먯꽌 ???섎━吏 ?딆쓬. ?좏솕 ?섎졃? '?덈퉬'濡?媛뺥솕.
                    float LASER_RANGE = (g_Stats.laserTier >= 2) ? 760.0f : 560.0f;
                    float lex = pCX + cosf(lang) * LASER_RANGE, ley = pCY + sinf(lang) * LASER_RANGE;
                    // ?좏솕 ?섎졃(tier3): 鍮??덈퉬 2諛???愿묓룺 愿??(?〓す ?쇱씤 ?쇱냼)
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
                    float ldmg = g_Stats.GetBaseDamage() * g_Stats.GetDamageMultiplier(0.0f)
                               * 1.4f * lcm * lbm;   // ?덊봽: 2.5 ??1.4 (?〓す ?뺣━?? 蹂댁뒪 移?理쒖냼)
                    if (g_TotemBoss && g_TotemBoss->alive) ldmg *= g_TotemBoss->statDamageMult();
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
                            float bx, bs; MobKillReward(m->kind, m->splitGen, m->elite, bx, bs);
                            float rwm = MobRewardMult(m->kind); bx *= rwm; bs *= rwm;
                            g_GameManager.xp += (long long)((bx + (float)g_Stats.meleeXpBonus) * g_Stats.xpMult);
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
                            g_GameManager.xp += (long long)((25.0f + (float)g_Stats.rangedXpBonus) * g_Stats.xpMult);
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
                            g_GameManager.xp += (long long)((25.0f + (float)g_Stats.meleeXpBonus) * g_Stats.xpMult);
                            g_Stats.killCount++; g_GameManager.scoreAccum += 200.0f;
                            g_GameManager.score = (long long)g_GameManager.scoreAccum; lOnKill();
                        }
                    }
                    auto lhitB = [&](float ex, float ey, float& hp, bool& al) {
                        if (!inLine(ex, ey)) return;
                        float dealt = (ldmg < hp) ? ldmg : hp; hp -= dealt;
                        SpawnDamageNumber(ex, ey, dealt, dealt >= 40.0f || lcrit);
                        if (hp <= 0.0f) al = false;
                    };
                    if (g_RRBoss && g_RRBoss->alive)
                        lhitB(g_RRBoss->worldX, g_RRBoss->worldY, g_RRBoss->hp, g_RRBoss->alive);
                    if (g_PolyBoss && g_PolyBoss->alive && g_PolyBoss->damageable())
                        lhitB(g_PolyBoss->worldX, g_PolyBoss->worldY, g_PolyBoss->hp, g_PolyBoss->alive);
                    if (g_BotnetBoss && g_BotnetBoss->alive)
                        lhitB(g_BotnetBoss->worldX, g_BotnetBoss->worldY, g_BotnetBoss->hp, g_BotnetBoss->alive);
                    if (g_CentiBoss && g_CentiBoss->alive && g_CentiBoss->vulnerable())
                        lhitB(g_CentiBoss->worldX, g_CentiBoss->worldY, g_CentiBoss->hp, g_CentiBoss->alive);
                    if (g_TotemBoss && g_TotemBoss->alive && g_TotemBoss->vulnerable())
                        lhitB(g_TotemBoss->worldX, g_TotemBoss->worldY, g_TotemBoss->hp, g_TotemBoss->alive);
                    if (g_UnknownBoss && g_UnknownBoss->alive)
                        lhitB(g_UnknownBoss->worldX, g_UnknownBoss->worldY, g_UnknownBoss->hp, g_UnknownBoss->alive);
                    g_LaserBeams.push_back({ pCX, pCY, lex, ley, 0.13f, 0.13f, beamW });
                    TriggerMuzzle(pCX, pCY, lang);
                }
            }

            // ?? 諛깆떊 ?ㅼ틪 (利앷컯) ??二쇨린?곸쑝濡??뚮젅?댁뼱 二쇰? ?뺥솕 ?꾩뒪(踰붿쐞 ?쇱냼) ??
            if (g_Stats.purgeNova > 0) {
                int   n      = g_Stats.purgeNova;
                float novaInt = NOVA_INT / (1.0f + 0.2f * (float)(n - 1));
                float novaR   = NOVA_R   * (1.0f + 0.18f * (float)(n - 1));
                g_NovaTimer += delta;
                if (g_NovaTimer >= novaInt) {
                    g_NovaTimer = 0.0f;
                    float dmg = g_Stats.GetBaseDamage() * g_Stats.GetDamageMultiplier(0.0f) * 2.0f;
                    if (g_TotemBoss && g_TotemBoss->alive) dmg *= g_TotemBoss->statDamageMult();
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
                                float bx,bs; MobKillReward(m->kind,m->splitGen,m->elite,bx,bs);
                                float rwm = MobRewardMult(m->kind); bx *= rwm; bs *= rwm;
                                g_GameManager.xp += (long long)((bx+(float)g_Stats.meleeXpBonus)*g_Stats.xpMult);
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
                                g_GameManager.xp+=(long long)((25.0f+(float)g_Stats.meleeXpBonus)*g_Stats.xpMult);
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
                                g_GameManager.xp+=(long long)((25.0f+(float)g_Stats.rangedXpBonus)*g_Stats.xpMult);
                                g_Stats.killCount++; g_GameManager.scoreAccum+=300.0f;
                                g_GameManager.score=(long long)g_GameManager.scoreAccum; nOnKill(); }
                        }
                    }
                    // 蹂댁뒪 ??踰붿쐞 ?대㈃ ??諛?移?蹂댁뒪 泥대젰????＜ ?녾쾶 怨좎젙??
                    auto nhitB = [&](float ex, float ey, float& hp, bool& al) {
                        float dx=ex-pCX, dy=ey-pCY;
                        if (dx*dx+dy*dy < (novaR+70.0f)*(novaR+70.0f)) { hp -= dmg * 2.0f; if (hp<=0.0f) al=false; }
                    };
                    if (g_RRBoss && g_RRBoss->alive) nhitB(g_RRBoss->worldX,g_RRBoss->worldY,g_RRBoss->hp,g_RRBoss->alive);
                    if (g_PolyBoss && g_PolyBoss->alive && g_PolyBoss->damageable()) nhitB(g_PolyBoss->worldX,g_PolyBoss->worldY,g_PolyBoss->hp,g_PolyBoss->alive);
                    if (g_BotnetBoss && g_BotnetBoss->alive) nhitB(g_BotnetBoss->worldX,g_BotnetBoss->worldY,g_BotnetBoss->hp,g_BotnetBoss->alive);
                    if (g_CentiBoss && g_CentiBoss->alive && g_CentiBoss->vulnerable()) nhitB(g_CentiBoss->worldX,g_CentiBoss->worldY,g_CentiBoss->hp,g_CentiBoss->alive);
                    if (g_TotemBoss && g_TotemBoss->alive && g_TotemBoss->vulnerable()) nhitB(g_TotemBoss->worldX,g_TotemBoss->worldY,g_TotemBoss->hp,g_TotemBoss->alive);
                    if (g_UnknownBoss && g_UnknownBoss->alive) nhitB(g_UnknownBoss->worldX,g_UnknownBoss->worldY,g_UnknownBoss->hp,g_UnknownBoss->alive);
                    if (g_TotemBoss && g_TotemBoss->alive) {
                        for (int ti = 0; ti < TotemBoss::N_TOTEM; ti++) {
                            auto& tt = g_TotemBoss->totems[ti];
                            if (!tt.alive) continue;
                            float dx = tt.x - pCX, dy = tt.y - pCY;
                            if (dx*dx + dy*dy < r2) {
                                tt.hp -= dmg;
                                g_TotemBoss->onTotemDamaged(ti);
                                if (tt.hp <= 0.0f) { tt.alive = false; g_TotemBoss->onTotemKilled(ti); }
                            }
                        }
                    }
                    // ?쒓컖 ???쎌갹 留?SpawnShockWave ?ъ궗?? + ?먮쭧
                    SpawnShockWave(pCX, pCY, novaR, 0.45f, 0.4f, 1.0f, 0.75f);
                    SpawnSparks(pCX, pCY, 10, 0.4f, 1.0f, 0.7f, 360.0f);
                }
            }

            // ?ы깙 紐⑤뱶?먯꽌???뚮젅?댁뼱媛 諛쒖궗?섏? ?딆쓬
            if (!g_Stats.turretMode) {
                // C13 ?먮룞 諛쒖궗: 湲곕낯 ON ?대㈃ 醫뚰겢由??놁씠??議곗? 諛⑺뼢?쇰줈 ?먮룞 諛쒖궗.
                //   C14 ?좎삁 以묒뿉??諛쒖궗 ?듭젣(?ㅻ컻 諛⑹?). ?섎룞 紐⑤뱶硫?醫뚰겢由????
                bool fireHeld = (g_AutoFire || lmb) && (g_PostPickGrace <= 0.0f);
                if (g_Stats.meleeWeapon) {       // 寃媛???洹쇱젒 ?ㅼ쐷 (珥앹븣 ?놁쓬)
                    // ?⑦? 諛⑹?: 荑⑤떎??effInterval)? ?대┃/???臾닿? ??긽 ?곸슜.
                    //   (?덉쟾??踰꾪듉 ?쇰㈃ fireTimer 瑜?利됱떆 以鍮??곹깭濡??뚮젮 愿묓겢濡?                    //    ?ㅼ쐷 ?띾룄瑜?臾댄븳???щ┫ ???덉뿀????洹?由ъ뀑???쒓굅)
                    if (fireHeld && fireTimer >= effInterval) {
                        float tx, ty;
                        if (aimTarget(tx, ty)) {   // ?먮룞: 理쒓렐????/ ?대┃: 而ㅼ꽌
                            meleeSwing(atan2f(ty - pCY, tx - pCX));
                            fireTimer = 0.0f;
                        }
                    }
                } else if (g_Stats.bowWeapon) {  // 沅곸닔 ???꾨Ⅸ 留뚰겮 李⑥쭠 ??諛쒖궗
                    // ?먮룞: ?李⑥쭠源뚯? ?먮룞 異⑹쟾 ???꾩땐?섎㈃ ?먮룞 諛쒖궗(李⑥? ?ъ씠??諛섎났).
                    bool bowHold = g_AutoFire ? (g_ArcherCharge < 1.0f && g_PostPickGrace <= 0.0f)
                                              : lmb;
                    if (bowHold) {
                        // 媛뺢턿(40% 鍮좊쫫) + ?곗궗利앷컯 蹂??bowChargeRateMult)
                        g_ArcherCharge += delta / (BOW_CHARGE_TIME * (g_Stats.powerDraw ? 0.6f : 1.0f))
                                          * g_Stats.bowChargeRateMult;
                        if (g_ArcherCharge > 1.0f) g_ArcherCharge = 1.0f;
                    } else if (g_ArcherCharge > 0.001f) {
                        float charge = g_ArcherCharge;
                        float atx, aty; if (!aimTarget(atx, aty)) { atx = wmx; aty = wmy; }
                        float ang = atan2f(aty - pCY, atx - pCX);
                        // ?꾩땐 ?꾨젰 (湲곕낯 4.1횞 / 媛뺢턿 5.1횞) + 怨듦꺽?μ쬆媛?蹂??bowChargeCapBonus)
                        float chMult = 0.5f + charge *
                                       ((g_Stats.powerDraw ? 4.6f : 3.6f) + g_Stats.bowChargeCapBonus);
                        float arrowDmg = g_Stats.GetBaseDamage()
                                       * g_Stats.GetDamageMultiplier(0.0f) * chMult;
                        if (g_Stats.berserk) {
                            float hf = g_Stats.maxHP > 0.0f
                                     ? g_GameManager.playerHP / g_Stats.maxHP : 1.0f;
                            if (hf < 0.0f) hf = 0.0f; else if (hf > 1.0f) hf = 1.0f;
                            arrowDmg *= (1.0f + (1.0f - hf) * 0.6f);
                        }
                        auto fireArrow = [&](float a) {
                            Bullet nb(pCX, pCY, pCX + cosf(a) * 200.0f, pCY + sinf(a) * 200.0f);
                            nb.speed       = effSpeed;
                            nb.maxRange    = 1500.0f;
                            nb.remainingDmg= arrowDmg;
                            nb.sizeScale   = 1.0f + charge * 3.0f;     // 理쒕? 4諛??ш린
                            nb.color       = glm::vec3(0.75f, 0.95f, 0.45f);
                            g_Bullets.push_back(nb);
                        };
                        // ?ㅼ쨷 ?ш꺽: ?꾩땐(>0.85) 諛쒖궗 ??3諛?遺梨꾧섦
                        if (g_Stats.multishot && charge > 0.85f) {
                            fireArrow(ang);
                            fireArrow(ang + 0.16f);
                            fireArrow(ang - 0.16f);
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
                    // ????곗궗 ??怨듭냽(effInterval) 以?? ?먮룞諛쒖궗 OFF?щ룄 LMB ??????곗궗.
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

        // GameManager 媛 ?몃쾭/蹂???곹깭瑜??????덇쾶 ?숆린??(Render ?먯꽌 ?ъ슜)
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
        // ?뚮뜑留?        // ============================================================
        glViewport(0, 0, screenWidth, screenHeight);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    
        glUseProgram(g_MainShader);
        glUniform1i(g_MainFxLoc, g_ShaderFx ? 1 : 0);   // CRT ?곗씠???④낵 ?좉? (G)
        // ?붾㈃ ?붾뱾湲?+ 以??곸슜 ??game world 留? HUD/text(蹂꾨룄 ortho)???곹뼢 ?놁쓬
        float orthoShake[16];
        memcpy(orthoShake, g_BaseOrtho, sizeof(g_BaseOrtho));
        // 以?以묒떖 = ZCX/ZCY, 湲곕낯 ?붾㈃ 以묒븰). z=1 ?대㈃ base ortho ? ?숈씪(identity)
        {
            float z = g_ViewZoom;
            float zcx = ZCX(), zcy = ZCY();
            orthoShake[0]  =  2.0f * z / (float)screenWidth;
            orthoShake[5]  = -2.0f * z / (float)screenHeight;
            orthoShake[12] =  2.0f * zcx * (1.0f - z) / (float)screenWidth  - 1.0f;
            orthoShake[13] =  1.0f - 2.0f * zcy * (1.0f - z) / (float)screenHeight;
        }
        // 寃뚯엫?뚮젅???щ쭩?곗텧 ???곹깭(硫붾돱쨌寃뚯엫?ㅻ쾭 ???먯꽑 ?붾㈃ ?붾뱾由??붿뿬 ?쒓굅
        //   ???щ쭩 ???쒖옉李쎌쑝濡?媛붿쓣 ???붾㈃??怨꾩냽 ?⑤━??臾몄젣 諛⑹?
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
        BatchFlush();   // ortho(以? 諛붽씀湲??????댁쟾 留ㅽ듃由?뒪濡??볦씤 ?꾪삎 癒쇱? 洹몃┝
        glUniformMatrix4fv(g_MainProjLoc, 1, GL_FALSE, orthoShake);
        // 湲濡쒕쾶?먮룄 ?숆린????BindMainShader() 媛 ??媛??ъ슜
        memcpy(g_MainOrtho, orthoShake, sizeof(orthoShake));
        glBindVertexArray(g_MainVAO);
    
        // ?붾뱶/?뷀떚???뚮뜑??寃뚯엫?뚮젅???곹깭?먯꽌留?洹몃┛????硫붾돱(?쒖옉李?????        //   吏곸쟾 寃뚯엫???뚮젅?댁뼱 李승룹옍???뷀떚?곌? ?뺤? ?곹깭濡?鍮꾩튂??臾몄젣 諛⑹?.
        //   (g_MainOrtho ???꾩뿉???대? 媛깆떊?덉쑝誘濡?硫붾돱 ?띿뒪???곕え ?뚮뜑???뺤긽)
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
    
        // ?먭굅由?紐?FakeWindow ?ш린 ?곸닔 (?뚮뜑쨌?대━??怨듭슜)
        const float RFW_W = g_RfwW;
        const float RFW_H = g_RfwH;
    
        // ============================================================
        // ?뚮뜑 z-order (?꾨옒?믪쐞)
        //  (a) ?먭굅由?紐?FakeWindow 諛곌꼍  ??媛???꾨옒
        //  (b) ?먭굅由?紐?李??대????〓す쨌珥앹븣쨌?ㅼ씠?꾨が??(scissor ?대━??
        //  (c) ?뚮젅?댁뼱 FakeWindow 諛곌꼍 ???꾩뿉????쓬
        //  (d) ?뚮젅?댁뼱 罹먮┃???щ쭩 ?댄럺??        //  (e) ?뚮젅?댁뼱 李??대????〓す쨌珥앹븣 (scissor ?대━?? ??媛????        //  (f) BrokenSight ?ㅻ툕 (?대━???놁쓬)
        // ============================================================
    
        // (a0) ?щ챸 諛곌꼍 媛由ш린 ??異⑷꺽??諛곌꼍 + ?붾젅洹몃옒??諛곌꼍
        //      glDisable(GL_BLEND) + 遺덊닾紐??대몢???꾪삎 ???댄썑 FakeWindow 濡???뼱?
        BatchFlush(); glDisable(GL_BLEND);
        BindMainShader();
    
        // (a0-1) ?먰룺蹂?異⑷꺽??諛곌꼍 ??needsBg ?뚮옒洹멸? ?ㅼ젙??異⑷꺽?뚯뿉 ?쒗빐
        for (auto& sw : g_ShockWaves) {
            if (!sw.active || !sw.needsBg) continue;
            float t   = 1.0f - sw.life / sw.maxLife;  // 0??
            float bgR = sw.maxRadius * t + 30.0f;      // 留곷낫??議곌툑 ?ш쾶
            drawCircle(sw.x, sw.y, bgR, 0.08f, 0.08f, 0.10f, 1.0f);
        }
    
        // (a) ?먭굅由?紐?+ ?ы깙 + 蹂댁뒪 FakeWindow 諛곌꼍 ??釉붾젋??OFF 濡?吏곸젒 ??뼱?곌린
        //     寃뱀퀜????洹몃젮??媛숈? ?됱씠 洹몃?濡??곗뿬 ?꾩쟻 ?놁쓬.
        // ?ы깙 250횞250 李?諛곌꼍 (?ㅼ닔)
        if (g_Stats.turretMode) {
            for (auto& t : g_Turrets) {
                float twx = WinOrigin(t.x, TURRET_WIN_W);
                float twy = WinOrigin(t.y, TURRET_WIN_H);
                drawRect(twx, twy, TURRET_WIN_W, TURRET_WIN_H,
                         0.06f, 0.08f, 0.10f, 1.0f);
            }
        }
        // ?? 媛吏쒖갹 ?듯빀 z-由ъ뒪??(??쓬?믩넂?? 遊뉖꽬 < ?먭굅由?< 蹂댁뒪/?щ씪??. 媛숈? ??낆?
        //    ?뚰솚(踰≫꽣) ?쒖꽌 = 癒쇱? ?뚰솚???좉? ?꾨옒. 諛곌꼍+?ㅼ삩蹂대뜑瑜????쒖꽌濡?'李??⑥쐞'
        //    濡?洹몃젮, ?믪? 李쎌쓽 遺덊닾紐?諛곌꼍????? 李쎌쓽 諛곌꼍쨌?멸낸?좎쓣 ?먯뿰????쓬(?곗꽑?쒖쐞 媛由?.
        //    (?뚮젅?댁뼱 李쎌? ???ㅼ뿉 ?곕줈 洹몃젮 ??긽 理쒖긽??)
        struct FWin { float x, y, w, h; const wchar_t* name;
                      float br, bgc, bbc, nr, ngc, nbc;
                      float gaugePct = -1.0f; };
        std::vector<FWin> zwins;
        auto addW = [&](float cx, float cy, float w, float h, const wchar_t* nm,
                        float br, float bgc, float bbc, float nr, float ngc, float nbc) {
            zwins.push_back({ cx - w*0.5f, cy - h*0.5f, w, h, nm, br,bgc,bbc, nr,ngc,nbc });
        };
        // 遊뉖꽬 ?몃뱶 (理쒗븯?? ?뚰솚 ?쒖꽌)
        for (auto m : g_MonsterManager.monsters) {
            if (!m->alive || m->kind != MobKind::SPAWNER) continue;
            float w = SPAWNER_WIN_W * m->sizeScale;
            addW(m->worldX, m->worldY, w, w, L"botnet.node", 0.06f,0.10f,0.09f, 0.20f,0.85f,0.65f);
        }
        // DDOS ??DrawAppWindow ?듯빀 ?⑥뒪 (e3) ?먯꽌留??뚮뜑
        // ?먭굅由?紐?(?뚰솚 ?쒖꽌)
        for (auto r : g_MonsterManager.rangedMobs) {
            if (r->deathScale <= 0.0f) continue;
            float sc = r->deathScale;
            addW(r->worldX, r->worldY, RFW_W*sc, RFW_H*sc, L"popup.exe", 0.08f,0.08f,0.10f, 0.85f,0.20f,0.95f);
        }
        // GLITCH.exe: 플레이어 창 밖 근접 몹은 (e)에서 mob.exe 통합 렌더
        // 蹂댁뒪/遺꾩뿴泥?(?곷떒)
        if (g_RRBoss && g_RRBoss->alive)
            addW(g_RRBoss->worldX, g_RRBoss->worldY, RR_WIN_W, RR_WIN_W,
                 L"VOLLEY.sys", 0.10f,0.07f,0.06f, 1.0f,0.55f,0.20f);
        if (g_PolyBoss && g_PolyBoss->alive)
            addW(g_PolyBoss->worldX, g_PolyBoss->worldY, POLY_WIN_W, POLY_WIN_W,
                 L"GLITCH.exe", 0.04f,0.02f,0.06f, 0.85f,0.25f,1.0f);
        if (g_PolyBoss && g_PolyBoss->alive && g_PolyBoss->blackHoleActive) {
            float hw = g_PolyBoss->holeWinSize();
            float hx = g_PolyBoss->holeX - hw * 0.5f;
            float hy = g_PolyBoss->holeY - hw * 0.5f;
            zwins.push_back({ hx, hy, hw, hw, L"GRAVITY.core",
                0.02f, 0.01f, 0.05f, 0.12f, 0.92f, 0.78f, g_PolyBoss->holeFillPct() });
        }
        if (g_BotnetBoss && g_BotnetBoss->alive)
            addW(g_BotnetBoss->worldX, g_BotnetBoss->worldY, BOTNET_WIN_W, BOTNET_WIN_W,
                 L"C2_RELAY.sys", 0.04f,0.07f,0.05f, 0.25f,0.92f,0.45f);
        if (g_CentiBoss && g_CentiBoss->alive)
            addW(g_CentiBoss->worldX, g_CentiBoss->worldY, CENTI_WIN_W, CENTI_WIN_W,
                 CentipedeBoss::BOSS_NAME, 0.02f,0.03f,0.04f, 0.22f,0.55f,0.72f);
        if (g_TotemBoss && g_TotemBoss->alive)
            addW(g_TotemBoss->worldX, g_TotemBoss->worldY, TOTEM_WIN_W, TOTEM_WIN_W,
                 L"RITE.CORE", 0.08f,0.04f,0.10f, 0.85f,0.45f,0.95f);
        if (g_UnknownBoss && g_UnknownBoss->alive) {
            addW(g_UnknownBoss->worldX, g_UnknownBoss->worldY, UNKNOWN_WIN_W, UNKNOWN_WIN_H,
                 UnknownBoss::BOSS_NAME, 0.05f,0.03f,0.06f, 0.95f,0.28f,0.62f);
            for (auto& bw : g_UnknownBoss->bladeWins) {
                if (!bw.active) continue;
                float nr = bw.recall ? 0.85f : (bw.telegraph ? 0.55f : 0.92f);
                float ng = bw.recall ? 0.22f : (bw.telegraph ? 0.85f : 0.35f);
                float nb = bw.recall ? 0.48f : (bw.telegraph ? 0.95f : 0.65f);
                addW(bw.x + bw.w * 0.5f, bw.y + bw.h * 0.5f, bw.w, bw.h,
                     UnknownBoss::BLADE_NAME, 0.05f,0.03f,0.06f, nr, ng, nb);
            }
        }
        // trap.exe / vaccine.exe — (e.sat)에서 플레이어 창 위에 통째로 그림
        // ?ы깙 李?諛곌꼍+蹂대뜑 (理쒗븯?? ?뚮젅?댁뼱 ?뚯쑀??z-由ъ뒪??諛?
        if (g_Stats.turretMode) {
            for (auto& t : g_Turrets) {
                BatchFlush(); glDisable(GL_BLEND);
                drawRect(t.x - TURRET_WIN_W*0.5f, t.y - TURRET_WIN_H*0.5f,
                         TURRET_WIN_W, TURRET_WIN_H, 0.06f, 0.08f, 0.10f, 1.0f);
                BatchFlush(); glEnable(GL_BLEND);
                drawNeonBorder(t.x - TURRET_WIN_W*0.5f, t.y - TURRET_WIN_H*0.5f,
                               TURRET_WIN_W, TURRET_WIN_H, 0.30f, 0.95f, 1.0f);
            }
        }
        // GLITCH.exe world hazards — draw below fake window chrome
        if (g_PolyBoss && g_PolyBoss->alive) {
            auto* pb = g_PolyBoss;
            auto hidePt = [&](float px, float py) {
                if (px >= playerWin.x && px <= playerWin.x + playerWin.width &&
                    py >= playerWin.y && py <= playerWin.y + playerWin.height)
                    return true;
                for (auto& fw : zwins)
                    if (px >= fw.x && px <= fw.x + fw.w &&
                        py >= fw.y && py <= fw.y + fw.h)
                        return true;
                if (g_Stats.turretMode)
                    for (auto& t : g_Turrets) {
                        float tx0 = t.x - TURRET_WIN_W * 0.5f;
                        float ty0 = t.y - TURRET_WIN_H * 0.5f;
                        if (px >= tx0 && px <= tx0 + TURRET_WIN_W &&
                            py >= ty0 && py <= ty0 + TURRET_WIN_H)
                            return true;
                    }
                return false;
            };
            BindMainShader();
            float gt = (float)glfwGetTime();
            if (pb->form == PForm::SINGULARITY) {
                if (pb->triWarn) {
                    float blink = 0.45f + 0.4f * (0.5f + 0.5f * sinf(gt * 16.0f));
                    for (int e = 0; e < 4; e++)
                        for (int i = 0; i < 18; i++) {
                            float t = (float)i / 17.0f;
                            float ax, ay, dx = 0, dy = 0;
                            if (e == 0) { ax = t * screenWidth; ay = 10.0f; dy = 1; }
                            else if (e == 1) { ax = t * screenWidth; ay = screenHeight - 10.0f; dy = -1; }
                            else if (e == 2) { ax = 10.0f; ay = t * screenHeight; dx = 1; }
                            else { ax = screenWidth - 10.0f; ay = t * screenHeight; dx = -1; }
                            drawTriangle(ax + dx * 12.0f, ay + dy * 12.0f,
                                         11.0f, 0.1f, 1.0f, 0.88f, blink);
                        }
                }
                if (pb->blackHoleActive) {
                    for (auto& s : pb->swarm) {
                        if (!s.alive || hidePt(s.x, s.y)) continue;
                        drawTriangle(s.x, s.y, 11.0f, 0.2f, 1.0f, 0.92f, 1.0f);
                    }
                }
            }
            if (pb->form == PForm::DISPLACE) {
                if (pb->fx.snapWarn || pb->snapTimer > 0.0f) {
                    auto& fx = pb->fx;
                    float tx = fx.snapWarnX + fx.snapWarnW * 0.5f;
                    float ty = fx.snapWarnY + fx.snapWarnH * 0.5f;
                    float warnF = pb->snapTimer > 0.0f ? (pb->snapTimer / 1.05f) : 1.0f;
                    if (warnF > 1.0f) warnF = 1.0f;
                    float sa = (0.55f + 0.35f * (0.5f + 0.5f * sinf(gt * 20.0f))) * warnF;
                    drawNeonBorder(fx.snapWarnX, fx.snapWarnY, fx.snapWarnW, fx.snapWarnH,
                                   1.0f, 0.50f, 0.12f);
                    drawNeonBorder(fx.snapWarnX - 4.0f, fx.snapWarnY - 4.0f,
                                   fx.snapWarnW + 8.0f, fx.snapWarnH + 8.0f,
                                   1.0f, 0.35f, 0.08f);
                    drawRect(fx.snapWarnX, fx.snapWarnY, fx.snapWarnW, fx.snapWarnH,
                             1.0f, 0.32f, 0.06f, 0.14f * sa);
                    float dx = tx - fx.snapFromX, dy = ty - fx.snapFromY;
                    float len = sqrtf(dx * dx + dy * dy) + 1e-3f;
                    int segs = (int)(len / 12.0f);
                    if (segs < 6) segs = 6;
                    for (int i = 0; i <= segs; i++) {
                        float u = (float)i / (float)segs;
                        drawCircle(fx.snapFromX + dx * u, fx.snapFromY + dy * u,
                                   6.0f, 1.0f, 0.50f, 0.12f, 0.70f * sa);
                    }
                    drawCircle(tx, ty, 14.0f, 1.0f, 0.45f, 0.10f, 0.85f * sa);
                }
                for (auto& bar : pb->bars) {
                    if (!bar.alive || hidePt(bar.x + bar.w * 0.5f, bar.y + bar.h * 0.5f)) continue;
                    drawRect(bar.x, bar.y, bar.w, bar.h, 0.07f, 0.05f, 0.09f, 0.82f);
                    drawNeonBorder(bar.x, bar.y, bar.w, bar.h, 1.0f, 0.38f, 0.18f);
                }
            }
            if (pb->form == PForm::PHANTOM) {
                float warnPulse = pb->fx.slipWarn
                    ? (0.35f + 0.45f * (1.0f - pb->fx.slipWarnT / 0.90f)) : 0.055f;
                for (int i = 0; i < 6; i++) {
                    float by = fmodf(pb->fx.staticBand * (float)screenHeight + (float)i * 110.0f,
                                     (float)screenHeight + 40.0f) - 20.0f;
                    drawRect(0.0f, by, (float)screenWidth, 4.0f,
                             0.55f, 0.45f, 1.0f, warnPulse);
                }
                if (pb->fx.slipWarn) {
                    auto& fx = pb->fx;
                    float pcx = playerWin.x + playerWin.width * 0.5f;
                    float pcy = playerWin.y + playerWin.height * 0.5f;
                    float dist = pb->enraged ? 90.0f : 70.0f;
                    float tx = pcx + fx.slipDirX * dist;
                    float ty = pcy + fx.slipDirY * dist;
                    float sa = 0.45f + 0.40f * (1.0f - fx.slipWarnT / 0.90f);
                    int segs = 8;
                    for (int i = 0; i <= segs; i++) {
                        float u = (float)i / (float)segs;
                        drawCircle(pcx + (tx - pcx) * u, pcy + (ty - pcy) * u,
                                   5.0f, 0.75f, 0.55f, 1.0f, 0.65f * sa);
                    }
                    drawNeonBorder(tx - playerWin.width * 0.5f, ty - playerWin.height * 0.5f,
                                   playerWin.width, playerWin.height,
                                   0.75f, 0.55f, 1.0f);
                }
            }
        }
        // z-由ъ뒪????李??⑥쐞濡?
        for (auto& fw : zwins) {
            BatchFlush(); glDisable(GL_BLEND);
            drawRect(fw.x, fw.y, fw.w, fw.h, fw.br, fw.bgc, fw.bbc, 1.0f);
            BatchFlush(); glEnable(GL_BLEND);
            drawNeonBorder(fw.x, fw.y, fw.w, fw.h, fw.nr, fw.ngc, fw.nbc);
        }
        BatchFlush(); glEnable(GL_BLEND);  // ?댄썑 ?쇰컲 ?뚰뙆 釉붾젋??蹂댁옣
    
        // (b) ?먭굅由?紐?+ 蹂댁뒪 李??대? 而⑦뀗痢?(?〓す쨌?먰룺蹂뫢룹킑?뙿룻뙆??
        //     媛?李쎈쭏??scissor ?⑥뒪. ?ㅼ씠?꾨が??蹂몄껜??(e2)/(e3) ?먯꽌 蹂꾨룄濡?洹몃┝
        BatchFlush(); glEnable(GL_SCISSOR_TEST);
        for (auto r : g_MonsterManager.rangedMobs) {
            if (r->deathScale <= 0.0f) continue;
            float sc  = r->deathScale;
            float rW  = RFW_W * sc, rH = RFW_H * sc;
            float rwx = r->worldX - rW * 0.5f;
            float rwy = r->worldY - rH * 0.5f;
            WorldScissor(rwx, rwy, rW, rH);
            // ?〓す (蹂댁뒪 ?뚰솚臾쇱? ???? ??李?諛뽰? 而щ쭅
            for (auto m : g_MonsterManager.monsters) {
                if (!m->alive || m->kind == MobKind::DDOS || !inWin(m->worldX, m->worldY, rwx, rwy, rW, rH)) continue;
                drawMob(m);
            }
            // ?먰룺蹂?(5媛곹삎)
            for (auto bm : g_MonsterManager.bombers) {
                if (!bm->alive || !inWin(bm->worldX, bm->worldY, rwx, rwy, rW, rH)) continue;
                drawPentagon(bm->worldX, bm->worldY, Bomber::SIZE_PX,
                             bm->color.r, bm->color.g, bm->color.b, 1.0f);
                // ?먰솕 以????몃━嫄???컻 諛섍꼍 ?쒖떆
                if (bm->arming) {
                    drawCircle(bm->worldX, bm->worldY, bm->blastRadius,
                               1.0f, 0.2f, 0.2f, 0.10f);
                }
            }
            // 珥앹븣
            for (auto& b : g_Bullets) {
                if (!b.active || !inWin(b.x, b.y, rwx, rwy, rW, rH)) continue;
                drawBullet(b);
            }
            for (auto& p : g_EnemyParts) {
                if (!p.active || !inWin(p.x, p.y, rwx, rwy, rW, rH)) continue;
                float a  = p.life / p.maxLife;
                float hs = p.size * 0.5f;
                drawRect(p.x - hs, p.y - hs, p.size, p.size, p.r, p.g, p.b, a);
            }
            // ?ㅺ??ㅻ뒗 二쎌쓬 ?ㅻ툕 (李??덉뿉?쒕쭔)
            for (auto& orb : g_ApproachOrbs) {
                DrawApproachOrb(orb.x, orb.y);
            }
        }
        // (b'') ?ы깙 李???而⑦뀗痢?(?ㅼ닔)
        if (g_Stats.turretMode) {
            for (auto& t : g_Turrets) {
                float twx = WinOrigin(t.x, TURRET_WIN_W);
                float twy = WinOrigin(t.y, TURRET_WIN_H);
                WorldScissor(twx, twy, TURRET_WIN_W, TURRET_WIN_H);
                for (auto m : g_MonsterManager.monsters) {
                    if (!m->alive || m->kind == MobKind::DDOS ||
                        !inWin(m->worldX, m->worldY, twx, twy, TURRET_WIN_W, TURRET_WIN_H)) continue;
                    drawMob(m);
                }
                for (auto bm : g_MonsterManager.bombers) {
                    if (!bm->alive || !inWin(bm->worldX, bm->worldY, twx, twy, TURRET_WIN_W, TURRET_WIN_H)) continue;
                    drawPentagon(bm->worldX, bm->worldY, Bomber::SIZE_PX,
                                 bm->color.r, bm->color.g, bm->color.b, 1.0f);
                }
                for (auto& b : g_Bullets) {
                    if (!b.active || !inWin(b.x, b.y, twx, twy, TURRET_WIN_W, TURRET_WIN_H)) continue;
                    drawBullet(b);
                }
                for (auto& p : g_EnemyParts) {
                    if (!p.active || !inWin(p.x, p.y, twx, twy, TURRET_WIN_W, TURRET_WIN_H)) continue;
                    float a  = p.life / p.maxLife;
                    float hs = p.size * 0.5f;
                    drawRect(p.x - hs, p.y - hs, p.size, p.size, p.r, p.g, p.b, a);
                }
            }
        }
        // (b') 蹂댁뒪 李???而⑦뀗痢???媛숈? ?〓す/?먰룺蹂?珥앹븣??蹂댁뒪 李??곸뿭?쇰줈???몄텧
        //     紐⑤뱺 蹂댁뒪 醫낅쪟(?щ씪??湲由ъ튂/由щ줈???대━/?ㅽ뙵/?щ씪?꾨텇?댁껜) 怨듯넻 泥섎━.
        //     E22: ?댁쟾???щ씪??蹂댁뒪(g_MonsterManager.boss)留??몄텧???ㅻⅨ 蹂댁뒪 李쎌뿉??        //          ?〓す/?꾩씠 而щ쭅?섏뼱 ??蹂댁?????蹂댁뒪蹂?李??곸뿭留덈떎 scissor ?⑥뒪 異붽?.
        auto drawBossWinContent = [&](float bwx, float bwy, float ww, float wh, bool withBullets = true) {
            WorldScissor(bwx, bwy, ww, wh);
            for (auto m : g_MonsterManager.monsters) {
                if (!m->alive || m->kind == MobKind::DDOS || !inWin(m->worldX, m->worldY, bwx, bwy, ww, wh)) continue;
                drawMob(m);
            }
            for (auto bm : g_MonsterManager.bombers) {
                if (!bm->alive || !inWin(bm->worldX, bm->worldY, bwx, bwy, ww, wh)) continue;
                drawPentagon(bm->worldX, bm->worldY, Bomber::SIZE_PX,
                             bm->color.r, bm->color.g, bm->color.b, 1.0f);
                if (bm->arming)
                    drawCircle(bm->worldX, bm->worldY, bm->blastRadius, 1.0f, 0.2f, 0.2f, 0.10f);
            }
            if (withBullets) {
                for (auto& b : g_Bullets) {
                    if (!b.active || !inWin(b.x, b.y, bwx, bwy, ww, wh)) continue;
                    drawBullet(b);
                }
            }
            for (auto& p : g_EnemyParts) {
                if (!p.active || !inWin(p.x, p.y, bwx, bwy, ww, wh)) continue;
                float a  = p.life / p.maxLife;
                float hs = p.size * 0.5f;
                drawRect(p.x - hs, p.y - hs, p.size, p.size, p.r, p.g, p.b, a);
            }
            for (auto& orb : g_ApproachOrbs) {
                DrawApproachOrb(orb.x, orb.y);
            }
        };
        if (g_RRBoss && g_RRBoss->alive)
            drawBossWinContent(g_RRBoss->worldX - RR_WIN_W * 0.5f,
                               g_RRBoss->worldY - RR_WIN_W * 0.5f, RR_WIN_W, RR_WIN_W);
        if (g_PolyBoss && g_PolyBoss->alive)
            drawBossWinContent(g_PolyBoss->worldX - POLY_WIN_W * 0.5f,
                               g_PolyBoss->worldY - POLY_WIN_W * 0.5f, POLY_WIN_W, POLY_WIN_W);
        if (g_BotnetBoss && g_BotnetBoss->alive)
            drawBossWinContent(g_BotnetBoss->worldX - BOTNET_WIN_W * 0.5f,
                               g_BotnetBoss->worldY - BOTNET_WIN_W * 0.5f, BOTNET_WIN_W, BOTNET_WIN_W);
        if (g_CentiBoss && g_CentiBoss->alive)
            drawBossWinContent(g_CentiBoss->worldX - CENTI_WIN_W * 0.5f,
                               g_CentiBoss->worldY - CENTI_WIN_W * 0.5f, CENTI_WIN_W, CENTI_WIN_W);
        if (g_TotemBoss && g_TotemBoss->alive)
            drawBossWinContent(g_TotemBoss->worldX - TOTEM_WIN_W * 0.5f,
                               g_TotemBoss->worldY - TOTEM_WIN_W * 0.5f, TOTEM_WIN_W, TOTEM_WIN_W);
        if (g_UnknownBoss && g_UnknownBoss->alive) {
            drawBossWinContent(g_UnknownBoss->worldX - UNKNOWN_WIN_W * 0.5f,
                               g_UnknownBoss->worldY - UNKNOWN_WIN_H * 0.5f,
                               UNKNOWN_WIN_W, UNKNOWN_WIN_H);
            for (auto& bw : g_UnknownBoss->bladeWins) {
                if (!bw.active) continue;
                drawBossWinContent(bw.x, bw.y, bw.w, bw.h);
            }
        }
        // 遊뉖꽬 ?몃뱶(SPAWNER) 李??대? 而⑦뀗痢????몃뱶 蹂몄껜/?뚰솚 ?뚯씠 ?먭린 李쎌뿉??蹂댁씠?꾨줉 (E21)
        for (auto m : g_MonsterManager.monsters) {
            if (!m->alive || m->kind != MobKind::SPAWNER) continue;
            float w = SPAWNER_WIN_W * m->sizeScale;
            drawBossWinContent(m->worldX - w * 0.5f, m->worldY - w * 0.5f, w, w);
        }
        BatchFlush(); glDisable(GL_SCISSOR_TEST);

        // FORK.worm child adds ??媛곸옄 媛吏???李?(y ?ㅻ쫫李⑥닚 = ?꾨옒媛 ?꾨줈 寃뱀묠)
        if (g_CentiBoss && g_CentiBoss->alive && !g_CentiBoss->minis.empty()) {
            const float MW = CentipedeBoss::MINI_WIN_W, MH = CentipedeBoss::MINI_WIN_H;
            std::vector<CentipedeBoss::MiniBug*> ord;
            for (auto& mb : g_CentiBoss->minis) if (mb.alive) ord.push_back(&mb);
            std::sort(ord.begin(), ord.end(),
                      [](CentipedeBoss::MiniBug* a, CentipedeBoss::MiniBug* b) { return a->y < b->y; });
            for (auto* mbp : ord) {
                auto& mb = *mbp;
                float wx = mb.x - MW * 0.5f, wy = mb.y - MH * 0.5f;
                DrawAppWindow(wx, wy, MW, MH, CentipedeBoss::MINI_WIN_NAME, CentipedeBoss::MINI_WIN_TB);
                BatchFlush(); glEnable(GL_SCISSOR_TEST);
                WorldScissor(wx, wy, MW, MH);
                for (auto& b : g_Bullets)
                    if (b.active && inWin(b.x, b.y, wx, wy, MW, MH)) drawBullet(b);
                BatchFlush(); glDisable(GL_SCISSOR_TEST);
                g_CentiBoss->drawMini(mb);
                BatchFlush();
            }
        }

        if (g_TotemBoss && g_TotemBoss->alive) {
            const float TW = TotemBoss::WIN_W * g_Scale, TH = TotemBoss::WIN_H * g_Scale;
            const float TTB = TotemBoss::WIN_TB * g_Scale;
            struct TotemPtr { TotemBoss::Totem* p; };
            std::vector<TotemPtr> ord;
            for (int ti = 0; ti < TotemBoss::N_TOTEM; ti++)
                if (g_TotemBoss->totems[ti].alive) ord.push_back({ &g_TotemBoss->totems[ti] });
            std::sort(ord.begin(), ord.end(),
                      [](TotemPtr a, TotemPtr b) { return a.p->y < b.p->y; });
            float gtTot = (float)glfwGetTime();
            for (auto& tp : ord) {
                auto& tt = *tp.p;
                float wx = tt.x - TW * 0.5f, wy = tt.y - TH * 0.5f;
                DrawAppWindow(wx, wy, TW, TH,
                              TotemBoss::totemWinTitle(tt.kind), TTB);
                BatchFlush(); glEnable(GL_SCISSOR_TEST);
                WorldScissor(wx, wy, TW, TH);
                for (auto& b : g_Bullets)
                    if (b.active && inWin(b.x, b.y, wx, wy, TW, TH)) drawBullet(b);
                for (auto m : g_MonsterManager.monsters)
                    if (m->alive && m->kind != MobKind::DDOS &&
                        inWin(m->worldX, m->worldY, wx, wy, TW, TH)) drawMob(m);
                g_TotemBoss->renderTotem(tt, gtTot);
                BatchFlush(); glDisable(GL_SCISSOR_TEST);
            }
        }

        // (c) player FakeWindow background
        BatchFlush(); glDisable(GL_BLEND);
        drawRect(playerWin.x, playerWin.y, playerWin.width, playerWin.height,
                 0.05f, 0.06f, 0.09f, 1.0f);
        BatchFlush(); glEnable(GL_BLEND);
        // ?ъ씠踰꾪럱???ㅼ삩 ?곕??????뚮젅?댁뼱 李??ㅼ삩 蹂대뜑 (?≪꽱???뚮쭏 ??
        drawNeonBorder(playerWin.x, playerWin.y, playerWin.width, playerWin.height,
                       g_AccentR, g_AccentG, g_AccentB);
        if (g_PolyBoss && g_PolyBoss->alive && g_PolyBoss->form == PForm::PHANTOM) {
            float pulse = 0.5f + 0.5f * sinf((float)glfwGetTime() * 9.0f);
            drawNeonBorder(playerWin.x - 3, playerWin.y - 3,
                           playerWin.width + 6, playerWin.height + 6,
                           0.75f, 0.55f, 1.0f);
            drawRect(playerWin.x, playerWin.y, playerWin.width, playerWin.height,
                     0.45f, 0.35f, 0.85f, 0.025f * pulse);
        }

        if (g_InBossIntermission || g_GameManager.currentState == GameState::RUN_SHOP) {
            float wx = g_ShopZoneX - RUN_SHOP_WIN_W * 0.5f;
            float wy = g_ShopZoneY - RUN_SHOP_WIN_H * 0.5f;
            DrawAppWindow(wx, wy, RUN_SHOP_WIN_W, RUN_SHOP_WIN_H, L"AUGMENT.store");
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
    
        // (c2) HP/EXP 諛????뚮젅?댁뼱 李??섎떒 ?덉そ??遺李?(李쎄낵 ?④퍡 ?대룞) ??
        if (g_GameManager.currentState == GameState::RUNNING ||
            g_GameManager.currentState == GameState::PAUSED ||
            g_InBossIntermission ||
            g_GameManager.currentState == GameState::RUN_SHOP ||
            g_GameManager.currentState == GameState::AUG_SELECT ||
            g_GameManager.currentState == GameState::DEBUFF_SELECT ||
            g_GameManager.currentState == GameState::DYING) {
            float pad = 12.0f, bx = playerWin.x + pad;
            float bw = playerWin.width - pad * 2.0f;
            float hpH = 14.0f, xpH = 8.0f, gap = 3.0f;   // ?먭퍖寃?(媛?쒖꽦)
            float hpY = playerWin.y + playerWin.height - 18.0f - hpH;   // ?섎떒 ?덉そ
            float xpY = hpY - gap - xpH;
            drawRect(bx - 5, xpY - 5, bw + 10, (hpY + hpH) - (xpY) + 10, 0.03f, 0.03f, 0.05f, 0.96f);
            // HP
            float hpFrac = (g_Stats.maxHP > 0.0f) ? g_GameManager.playerHP / g_Stats.maxHP : 0.0f;
            if (hpFrac < 0.0f) hpFrac = 0.0f; if (hpFrac > 1.0f) hpFrac = 1.0f;
            float hpR = (hpFrac > 0.5f) ? 0.1f : 1.0f;
            float hpG = (hpFrac > 0.5f) ? 1.0f : hpFrac * 2.0f;
            drawRect(bx, hpY, bw, hpH, 0.22f, 0.04f, 0.04f, 1.0f);
            drawRect(bx, hpY, bw * hpFrac, hpH, hpR, hpG, 0.1f, 1.0f);
            // (HP ?섏튂??醫뚯긽??HUD ???쒖떆 ???붾뱶 ?뱀뀡?먯꽌 ?띿뒪?몃? 洹몃━硫?            //  TextRenderer 媛 ?곗씠??VAO 瑜??몃컮?몃뱶???댄썑 ?뷀떚???뚮뜑媛 源⑥?誘濡?湲덉?)
            // XP
            long long needX = g_ExpSystem.Required(g_GameManager.playerLevel);
            float xpFrac = (needX > 0) ? (float)g_GameManager.xp / (float)needX : 0.0f;
            if (xpFrac < 0.0f) xpFrac = 0.0f; if (xpFrac > 1.0f) xpFrac = 1.0f;
            drawRect(bx, xpY, bw, xpH, 0.06f, 0.10f, 0.07f, 1.0f);
            drawRect(bx, xpY, bw * xpFrac, xpH, 0.4f, 1.0f, 0.55f, 1.0f);
        }
    
        // (c2.5) 諛곕뱶 ?뱁꽣 媛먯냽 援ъ뿭 ??以묒떖?먯꽌 遺?앸릺???쇱????먯긽 釉붾줉(源쒕묀???좊땲硫붿씠??
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
                // 遺??釉붾줉 ???蹂??섏궗?쒖닔 源쒕묀?? 以묒떖?먯꽌 grow factor 源뚯?留??몄텧(?쇱쭚)
                const int N = 7;
                float cw = z.w / (float)N, ch = z.h / (float)N;
                for (int iy = 0; iy < N; iy++) for (int ix = 0; ix < N; ix++) {
                    float fx = z.x + ((float)ix + 0.5f) * cw;
                    float fy = z.y + ((float)iy + 0.5f) * ch;
                    float dxn = (fx - zx) / (z.w*0.5f + 1e-3f);
                    float dyn = (fy - zy) / (z.h*0.5f + 1e-3f);
                    if (sqrtf(dxn*dxn + dyn*dyn) > gf) continue;   // ?꾩쭅 遺?????우쓬
                    float seed = sinf((float)ix*12.9898f + (float)iy*78.233f) * 43758.5453f;
                    float ph = seed - floorf(seed);
                    float fl = 0.45f + 0.55f * sinf(zt * 6.0f + ph * 6.2831853f);
                    float ca = (0.10f + 0.22f * fl) * lifeF;
                    drawRect(fx - cw*0.42f, fy - ch*0.42f, cw*0.84f, ch*0.84f, 0.62f, 0.2f, 0.88f, ca);
                }
                drawNeonBorder(zx - hw, zy - hh, hw*2, hh*2, 0.75f, 0.3f, 0.95f);
            }
        }
    
        // (c3) ?ㅼ틪 ?덉씠? 鍮????섏씠?쒕릺??泥?줉 愿??鍮?(蹂댁뒪 ?덉씠? 荑쇰뱶 ?⑦꽩)
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
    
    
        // (d) ?뚮젅?댁뼱 罹먮┃??+ 利앷컯 ?댄럺??+ ?щ쭩 ?뚰렪
        {
            float pCX = playerWin.x + playerWin.width  * 0.5f;
            float pCY = playerWin.y + playerWin.height * 0.5f;
            // 珥앷?: 200px ?대궡 ?쒖떆 (?щ????쒖븞 ??
            if (g_Stats.bayonet)
                drawCircle(pCX, pCY, 200.0f, 0.4f, 1.0f, 0.9f, 0.10f);
    
            // 沅곸닔 李⑥쭠 寃뚯씠吏 (?뚮젅?댁뼱 ??諛? ???꾩땐 ???곗깋 踰덉찉
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
    
                // 以묒떖 ?ш킅
                if (g_DeathFlash > 0.0f) {
                    float fr = 220.0f * g_DeathFlash;
                    drawCircle(g_DeathCX, g_DeathCY, fr,
                               1.0f, 1.0f, 1.0f, g_DeathFlash * 0.9f);
                    drawCircle(g_DeathCX, g_DeathCY, fr * 1.8f,
                               1.0f, 0.85f, 0.2f, g_DeathFlash * 0.45f);
                }
    
                // ?щ쭩 ?뚰렪
                for (int i = 0; i < MAX_DEBRIS; i++) {
                    if (!g_Debris[i].active) continue;
                    float s  = g_Debris[i].size;
                    float hs = s * 0.5f;
                    drawRect(g_Debris[i].x - hs, g_Debris[i].y - hs, s, s,
                             g_Debris[i].r, g_Debris[i].g, g_Debris[i].b, fade);
                }
            } else if (g_GameManager.currentState != GameState::GAMEOVER) {
                // ?대룞 ?붿긽 ???뚮젅?댁뼱 ??癒쇱? 洹몃젮 ?꾨옒??源붾┝), ?섎챸 鍮꾨? ?섏씠??異뺤냼
                for (auto& tr : g_Trail) {
                    float f = tr.life / tr.maxLife;        // 1??
                    float s = tr.size * (0.4f + 0.6f * f);
                    drawRect(tr.x - s*0.5f, tr.y - s*0.5f, s, s, tr.r, tr.g, tr.b, 0.28f * f);
                }
                float sz = PLAYER_SIZE * g_Stats.playerSizeMult;
                float hs = sz * 0.5f;
                // ?멸낸: ?대몢???뚮몢由?(?鍮?
                drawRect(pCX - hs - 3, pCY - hs - 3, sz + 6, sz + 6,
                         0.0f, 0.0f, 0.0f, 0.8f);
                // 蹂몄껜: 諛앹? ?쒖븞
                drawRect(pCX - hs, pCY - hs, sz, sz,
                         0.3f, 1.0f, 1.0f, 1.0f);
                float core = sz * 0.35f;
                drawRect(pCX - core * 0.5f, pCY - core * 0.5f, core, core,
                         1.0f, 1.0f, 1.0f, 1.0f);
    
                // ?? ?곕굹鍮꾩떇 HP 寃뚯씠吏諛????쇨꺽 ???뚮젅?댁뼱 ?꾩뿉 ?대떎 ?섏씠?? ????쑝硫??곸떆 ??
                if (g_GameManager.currentState == GameState::RUNNING) {
                    float hf = (g_Stats.maxHP > 0.0f) ? g_GameManager.playerHP / g_Stats.maxHP : 0.0f;
                    if (hf < 0.0f) hf = 0.0f; if (hf > 1.0f) hf = 1.0f;
                    bool low = hf < 0.40f;
                    float vis = low ? 1.0f
                              : (g_HpBarPop > 1.6f ? (2.2f - g_HpBarPop) / 0.6f
                                                   : g_HpBarPop / 1.6f);
                    if (vis > 1.0f) vis = 1.0f; if (vis < 0.0f) vis = 0.0f;
                    if (vis > 0.01f) {
                        // ?묎퀬 ?뚮젅?댁뼱??媛源앷쾶 ???꾩そ ?곸쓣 ??媛由щ룄濡?(媛由곕떎???쇰뱶諛?
                        float bw = 46.0f * g_Stats.playerSizeMult, bh = 5.0f;
                        float bx = pCX - bw * 0.5f, by = pCY - hs - 15.0f;
                        drawRect(bx - 1.5f, by - 1.5f, bw + 3, bh + 3, 0.0f, 0.0f, 0.0f, 0.6f * vis);
                        drawRect(bx, by, bw, bh, 0.25f, 0.05f, 0.05f, 0.7f * vis);
                        float r = hf > 0.5f ? 0.2f : 1.0f;
                        float g = hf > 0.5f ? 1.0f : hf * 2.0f;
                        drawRect(bx, by, bw * hf, bh, r, g, 0.15f, 0.92f * vis);
                    }
                }
    
                // ?? ?꾩튂 媛뺤“ ?쒖떆 (?쇱옟???꾨쭑 ?띿뿉???뚮젅?댁뼱瑜??쎄쾶 李얜룄濡? ??
                //   + ?먰삎 ?덊떚??以묒떖 鍮꾩?) + ?낆? ?ㅼ씪濡? HP ??쓣?섎줉 媛뺥빐吏怨?遺됱뼱吏?
                if (g_GameManager.currentState == GameState::RUNNING) {
                    float hpFrac = (g_Stats.maxHP > 0.0f)
                                 ? g_GameManager.playerHP / g_Stats.maxHP : 1.0f;
                    if (hpFrac < 0.0f) hpFrac = 0.0f; if (hpFrac > 1.0f) hpFrac = 1.0f;
                    bool low = hpFrac < 0.3f;
                    float spd   = low ? 9.0f : 3.5f;
                    float pulse = 0.5f + 0.5f * sinf((float)glfwGetTime() * spd);
                    float a = (low ? (0.45f + 0.4f * pulse) : (0.22f + 0.12f * pulse));
                    // ?? ?됱긽???쒖븞, ?꾪뿕 ??遺됯쾶
                    float rr = low ? 1.0f : 0.4f;
                    float gg = low ? 0.35f : 1.0f;
                    float bb = low ? 0.35f : 1.0f;
                    // ?낆? ?ㅼ씪濡?梨꾩썙吏??? ??硫由ъ꽌???꾩튂媛 蹂댁씠?꾨줉
                    drawCircle(pCX, pCY, hs + 22.0f * g_Stats.playerSizeMult,
                               rr, gg, bb, a * 0.18f);
                    // + ?먰삎 ?덊떚??(以묒떖? 鍮꾩썙??蹂몄껜瑜?媛由ъ? ?딆쓬)
                    float gap = hs + 6.0f;
                    float L   = 16.0f * g_Stats.playerSizeMult;
                    float t   = 3.0f;
                    drawRect(pCX - t*0.5f, pCY - gap - L, t, L, rr, gg, bb, a);
                    drawRect(pCX - t*0.5f, pCY + gap,     t, L, rr, gg, bb, a);
                    drawRect(pCX - gap - L, pCY - t*0.5f, L, t, rr, gg, bb, a);
                    drawRect(pCX + gap,     pCY - t*0.5f, L, t, rr, gg, bb, a);
                }
            }
        }
    
        // (e) ?뚮젅?댁뼱 李??대? 而⑦뀗痢???scissor (媛?????덉씠??
        BatchFlush(); glEnable(GL_SCISSOR_TEST);
        WorldScissor(playerWin.x, playerWin.y, playerWin.width, playerWin.height);
        {
        float pwx = playerWin.x, pwy = playerWin.y, pww = playerWin.width, pwh = playerWin.height;
        // ?〓す (蹂댁뒪 ?뚰솚臾쇱? ???? ??李?諛?而щ쭅
        for (auto m : g_MonsterManager.monsters) {
            if (!m->alive) continue;
            if (g_PolyBoss && g_PolyBoss->alive &&
                m->kind != MobKind::DDOS && m->kind != MobKind::SPAWNER) {
                if (!inWin(m->worldX, m->worldY, pwx, pwy, pww, pwh, 10.0f)) continue;
            } else if (!inWin(m->worldX, m->worldY, pwx, pwy, pww, pwh)) {
                continue;
            }
            drawMob(m);
        }
        for (auto bm : g_MonsterManager.bombers) {
            if (!bm->alive || !inWin(bm->worldX, bm->worldY, pwx, pwy, pww, pwh)) continue;
            drawPentagon(bm->worldX, bm->worldY, Bomber::SIZE_PX,
                         bm->color.r, bm->color.g, bm->color.b, 1.0f);
            if (bm->arming) {
                drawCircle(bm->worldX, bm->worldY, bm->blastRadius,
                           1.0f, 0.2f, 0.2f, 0.10f);
            }
        }
        // 珥앹븣
        for (auto& b : g_Bullets) {
            if (!b.active || !inWin(b.x, b.y, pwx, pwy, pww, pwh)) continue;
            drawBullet(b);
        }
        for (auto& p : g_EnemyParts) {
            if (!p.active || !inWin(p.x, p.y, pwx, pwy, pww, pwh)) continue;
            float a  = p.life / p.maxLife;
            float hs = p.size * 0.5f;
            drawRect(p.x - hs, p.y - hs, p.size, p.size, p.r, p.g, p.b, a);
        }
        if (g_PolyBoss && g_PolyBoss->alive && g_PolyBoss->form == PForm::SINGULARITY) {
            for (auto& s : g_PolyBoss->swarm) {
                if (!s.alive || !inWin(s.x, s.y, pwx, pwy, pww, pwh)) continue;
                drawTriangle(s.x, s.y, 11.0f, 0.2f, 1.0f, 0.92f, 1.0f);
            }
        }
        // ?ㅺ??ㅻ뒗 二쎌쓬 ?ㅻ툕 (?뚮젅?댁뼱 李??덉뿉?쒕쭔)
        for (auto& orb : g_ApproachOrbs) {
            DrawApproachOrb(orb.x, orb.y);
        }
        }
        BatchFlush(); glDisable(GL_SCISSOR_TEST);

        // GLITCH mob.exe — 플레이어 창 밖 근접 몹만 chrome+내용 통합 렌더 (z/클리핑 오류 방지)
        if (g_PolyBoss && g_PolyBoss->alive) {
            const float GMW = g_RfwW * 0.62f, GMH = g_RfwH * 0.62f;
            const float MTB = WIN_TB;
            struct MobPtr { Monster* m; float y; };
            std::vector<MobPtr> outside;
            for (auto m : g_MonsterManager.monsters) {
                if (!m->alive || m->kind == MobKind::DDOS || m->kind == MobKind::SPAWNER) continue;
                if (inWin(m->worldX, m->worldY, playerWin.x, playerWin.y,
                          playerWin.width, playerWin.height, 10.0f))
                    continue;
                outside.push_back({ m, m->worldY });
            }
            std::sort(outside.begin(), outside.end(),
                      [](const MobPtr& a, const MobPtr& b) { return a.y < b.y; });
            BindMainShader();
            for (auto& mp : outside) {
                Monster* m = mp.m;
                float sc = m->sizeScale;
                float mw = GMW * sc, mh = GMH * sc;
                float wx = m->worldX - mw * 0.5f;
                float wy = m->worldY - mh * 0.5f;
                if (wx < 4.0f) wx = 4.0f;
                if (wy < 4.0f) wy = 4.0f;
                if (wx + mw > screenWidth - 4.0f) wx = screenWidth - 4.0f - mw;
                if (wy + mh > screenHeight - 94.0f) wy = screenHeight - 94.0f - mh;
                DrawAppWindow(wx, wy, mw, mh, L"mob.exe", MTB);
                BatchFlush(); glEnable(GL_SCISSOR_TEST);
                WorldScissor(wx, wy + MTB, mw, mh - MTB);
                drawMob(m);
                BatchFlush(); glDisable(GL_SCISSOR_TEST);
            }
        }

        // GLITCH GRAVITY.core — 플레이어 창 위에 블랙홀 오버레이
        if (g_PolyBoss && g_PolyBoss->alive && g_PolyBoss->blackHoleActive) {
            auto* pb = g_PolyBoss;
            float hw = pb->holeWinSize();
            float hx = pb->holeX - hw * 0.5f, hy = pb->holeY - hw * 0.5f;
            const float TBH = 22.0f;
            BatchFlush(); glEnable(GL_SCISSOR_TEST);
            WorldScissor(hx, hy, hw, hw);
            BatchFlush(); glDisable(GL_BLEND);
            drawRect(hx, hy, hw, hw, 0.01f, 0.02f, 0.05f, 0.94f);
            drawRect(hx, hy, hw, TBH, 0.07f, 0.42f, 0.36f, 1.0f);
            BatchFlush(); glEnable(GL_BLEND);
            drawNeonBorder(hx, hy, hw, hw, 0.15f, 0.98f, 0.82f);
            float gt = (float)glfwGetTime();
            float hr = pb->holeR;
            float pulse = 0.5f + 0.5f * sinf(gt * 5.0f);
            drawCircle(pb->holeX, pb->holeY, hr * 1.60f, 0.12f, 0.58f, 1.0f, 0.22f + 0.10f * pulse);
            drawCircle(pb->holeX, pb->holeY, hr * 1.22f, 0.0f, 0.90f, 0.80f, 0.40f);
            drawCircle(pb->holeX, pb->holeY, hr * 0.66f, 0.0f, 0.0f, 0.0f, 0.98f);
            for (int ri = 0; ri < 6; ri++) {
                float ang = gt * (2.4f + ri * 0.28f) + ri * 1.047f;
                drawTriangle(pb->holeX + cosf(ang) * hr * 0.85f,
                             pb->holeY + sinf(ang) * hr * 0.85f,
                             17.0f, 0.12f, 1.0f, 0.92f, 0.60f);
            }
            float barW = hw - 28.0f;
            float fill = pb->holeFillPct();
            drawRect(hx + 14.0f, hy + hw - 20.0f, barW, 8.0f, 0.04f, 0.03f, 0.06f, 0.95f);
            drawRect(hx + 14.0f, hy + hw - 20.0f, barW * fill, 8.0f, 0.10f, 0.98f, 0.80f, 1.0f);
            for (auto& s : pb->swarm) {
                if (!s.alive || !inWin(s.x, s.y, hx, hy + TBH, hw, hw - TBH)) continue;
                drawTriangle(s.x, s.y, 12.0f, 0.2f, 1.0f, 0.92f, 1.0f);
            }
            for (auto& b : g_Bullets) {
                if (!b.active || !inWin(b.x, b.y, hx, hy, hw, hw)) continue;
                drawBullet(b);
            }
            BatchFlush(); glDisable(GL_SCISSOR_TEST);
            wchar_t gp[16];
            swprintf_s(gp, L"%d%%", (int)(fill * 100.0f + 0.5f));
            float ts = 0.52f * g_ViewZoom, gs = 0.46f * g_ViewZoom;
            g_TextS.Draw(L"GRAVITY.core", W2SX(hx + 8.0f), W2SY(hy + 3.0f),
                         ts, 0.90f, 0.98f, 0.95f, 0.98f);
            float gw = g_TextS.Width(gp, gs);
            g_TextS.Draw(gp, W2SX(hx + hw - gw - 52.0f), W2SY(hy + 3.0f),
                         gs, 0.55f, 0.98f, 0.85f, 0.98f);
        }

        // (e2) ?먭굅由?紐??ㅼ씠?꾨が????媛??먭굅由?紐?李??곸뿭?먯꽌 ??긽 ?꾩뿉 洹몃┝
        BatchFlush(); glEnable(GL_SCISSOR_TEST);
        for (auto r : g_MonsterManager.rangedMobs) {
            if (r->deathScale <= 0.0f) continue;
            float sc  = r->deathScale;
            float rW  = RFW_W * sc, rH = RFW_H * sc;
            float rwx = r->worldX - rW * 0.5f;
            float rwy = r->worldY - rH * 0.5f;
            WorldScissor(rwx, rwy, rW, rH);
            float dSize = 32.0f * sc;
            float dAlpha = sc;
            drawDiamond(r->worldX, r->worldY, dSize,
                        r->color.r, r->color.g, r->color.b, dAlpha);
        }
        BatchFlush(); glDisable(GL_SCISSOR_TEST);
    
        // (e2.1) ?ы깙 ?꾩씠肄?+ ?섎챸諛?(?ㅼ닔) ??媛?李??곸뿭 scissor ?댁뿉???쒖떆
        if (g_Stats.turretMode) {
            BatchFlush(); glEnable(GL_SCISSOR_TEST);
            for (auto& t : g_Turrets) {
                float twx = WinOrigin(t.x, TURRET_WIN_W);
                float twy = WinOrigin(t.y, TURRET_WIN_H);
                WorldScissor(twx, twy, TURRET_WIN_W, TURRET_WIN_H);
                // ?ы깙 蹂몄껜 ????옄??(以묒븰 ?ш컖??+ 4諛⑺뼢 ?뚯텧)
                float tc = 12.0f;
                drawRect(t.x - tc, t.y - 4, tc*2, 8, 0.1f, 1.0f, 0.55f, 1.0f);
                drawRect(t.x - 4, t.y - tc, 8, tc*2, 0.1f, 1.0f, 0.55f, 1.0f);
                // ?섎챸 諛?(李??곷떒)
                float lifeRem = 1.0f - t.lifeTimer / TURRET_LIFE;
                if (lifeRem < 0.0f) lifeRem = 0.0f;
                float barW = TURRET_WIN_W - 24.0f;
                drawRect(twx + 12.0f, twy + 8.0f, barW, 5.0f,
                         0.12f, 0.12f, 0.18f, 0.7f);
                drawRect(twx + 12.0f, twy + 8.0f, barW * lifeRem, 5.0f,
                         0.1f, 1.0f, 0.55f, 0.9f);
            }
            BatchFlush(); glDisable(GL_SCISSOR_TEST);
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
            float ax = wmx, ay = wmy;   // 以?蹂댁젙 ??以??곸슜??ortho ?먯꽌 而ㅼ꽌 ?꾩튂???쒖떆
            // ?멸낸 ?대몢????+ 以묒븰 ??옄 (留???옄???≪꽱???뚮쭏 ??
            float cr = g_AccentR, cg = g_AccentG, cb = g_AccentB;
            drawCircle(ax, ay, 12.0f, 0.0f, 0.0f, 0.0f, 0.6f);
            drawCircle(ax, ay, 10.0f, cr, cg, cb, 0.9f);
            drawCircle(ax, ay, 5.0f, 0.05f, 0.05f, 0.05f, 0.9f);
            drawRect(ax - 1.5f, ay - 1.5f, 3.0f, 3.0f,
                     1.0f, 1.0f, 1.0f, 1.0f);
            // 4諛⑺뼢 吏㏃? ?쇱씤 (??옄)
            drawRect(ax - 14.0f, ay - 1.0f, 6.0f, 2.0f, cr, cg, cb, 0.95f);
            drawRect(ax + 8.0f,  ay - 1.0f, 6.0f, 2.0f, cr, cg, cb, 0.95f);
            drawRect(ax - 1.0f, ay - 14.0f, 2.0f, 6.0f, cr, cg, cb, 0.95f);
            drawRect(ax - 1.0f, ay + 8.0f,  2.0f, 6.0f, cr, cg, cb, 0.95f);
        }
    
        // (g) ?ㅺ??ㅻ뒗 二쎌쓬 ?ㅻ툕 ??scissor ?덉뿉?쒕쭔 ?쒖떆 ((b)/(e) ?⑥뒪???꾩엫)
    
        // (g2) 異⑷꺽?????먰룺蹂??먰룺 / 蹂댁뒪 ?ㅽ룿 ?? ??긽 ?꾩뿉 ?쒖떆
        for (auto& sw : g_ShockWaves) {
            if (!sw.active) continue;
            float t = 1.0f - sw.life / sw.maxLife;  // 0 ??1
            float radius = sw.maxRadius * t;
            float alpha  = (1.0f - t) * 0.55f;
            drawCircle(sw.x, sw.y, radius, sw.r, sw.g, sw.b, alpha);
        }
    
        // (g2b) 寃媛??ㅼ쐷 ?붿긽 ??議곗? 諛⑺뼢 遺梨꾧섦
        for (auto& sl : g_Slashes) {
            if (!sl.active) continue;
            float t   = 1.0f - sl.life / sl.maxLife;       // 0 ??1
            float rad = sl.range * (0.72f + 0.28f * t);
            float alpha   = (1.0f - t) * 0.5f;
            float halfArc = 1.15f * (1.0f - 0.15f * t);
            drawConeFan(sl.x, sl.y, rad, sl.ang, halfArc, 0.85f, 0.95f, 1.0f, alpha);
        }
    
        // (g2c) ?寃??ㅽ뙆??+ 癒몄쫹 ?뚮옒????媛??additive) 釉붾젋?⑹쑝濡?諛앷쾶
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
            // 湲곕낯 遺꾨━ 釉붾젋??蹂듭썝
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                                GL_ONE,       GL_ONE_MINUS_SRC_ALPHA);
        }
    

                // VOLLEY.sys ???꾩“ + 湲곕룞 ?붾젰 ?쒕줎
        if (g_RRBoss && g_RRBoss->alive) {
            auto* rb = g_RRBoss;
            float pCX = playerWin.x + playerWin.width  * 0.5f;
            float pCY = playerWin.y + playerWin.height * 0.5f;
            float gtRR = (float)glfwGetTime();
            BindMainShader();

            rb->renderTelegraphs(pCX, pCY, gtRR);
            BatchFlush(); glEnable(GL_SCISSOR_TEST);
            WorldScissor(playerWin.x, playerWin.y, playerWin.width, playerWin.height);
            rb->renderTelegraphs(pCX, pCY, gtRR);
            BatchFlush(); glDisable(GL_SCISSOR_TEST);

            BatchFlush(); glEnable(GL_SCISSOR_TEST);
            WorldScissor(rb->worldX - RR_WIN_W * 0.5f, rb->worldY - RR_WIN_W * 0.5f,
                         RR_WIN_W, RR_WIN_W);
            for (auto& b : g_Bullets) {
                if (!b.active) continue;
                drawBullet(b);
            }
            rb->renderBody(gtRR, pCX, pCY);
            BatchFlush(); glDisable(GL_SCISSOR_TEST);
        }

        if (g_UnknownBoss && g_UnknownBoss->alive) {
            auto* ub = g_UnknownBoss;
            float gtUB = (float)glfwGetTime();
            float pCX = playerWin.x + playerWin.width  * 0.5f;
            float pCY = playerWin.y + playerWin.height * 0.5f;
            BindMainShader();

            float uwx = ub->worldX - UNKNOWN_WIN_W * 0.5f;
            float uwy = ub->worldY - UNKNOWN_WIN_H * 0.5f;
            BatchFlush(); glEnable(GL_SCISSOR_TEST);
            WorldScissor(uwx, uwy, UNKNOWN_WIN_W, UNKNOWN_WIN_H);
            for (auto& b : g_Bullets)
                if (b.active) drawBullet(b);
            ub->renderBody(gtUB, pCX, pCY);
            BatchFlush(); glDisable(GL_SCISSOR_TEST);
        }

        if (g_BotnetBoss && g_BotnetBoss->alive) {
            auto* nb2 = g_BotnetBoss;
            float ct = (float)glfwGetTime();
            BatchFlush(); glEnable(GL_SCISSOR_TEST);
            WorldScissor(nb2->worldX - BOTNET_WIN_W * 0.5f, nb2->worldY - BOTNET_WIN_W * 0.5f,
                         BOTNET_WIN_W, BOTNET_WIN_W);
            nb2->renderCore(ct);
            BatchFlush(); glDisable(GL_SCISSOR_TEST);

            BatchFlush(); glEnable(GL_SCISSOR_TEST);
            auto c2MinionPass = [&](float wx, float wy, float ww, float wh) {
                WorldScissor(wx, wy, ww, wh);
                for (auto& b : g_Bullets)
                    if (b.active) drawBullet(b);
                nb2->renderMinions(ct);
            };
            for (auto& fw : zwins) c2MinionPass(fw.x, fw.y, fw.w, fw.h);
            c2MinionPass(playerWin.x, playerWin.y, playerWin.width, playerWin.height);
            if (g_Stats.turretMode)
                for (auto& tr : g_Turrets)
                    c2MinionPass(tr.x - TURRET_WIN_W * 0.5f, tr.y - TURRET_WIN_H * 0.5f,
                                 TURRET_WIN_W, TURRET_WIN_H);
            BatchFlush(); glDisable(GL_SCISSOR_TEST);
        }
    
        // (g4f) FORK.worm ???뚮씪利덈쭏 泥댁씤: 媛吏?李?scissor ?덉뿉 蹂몄껜쨌adds쨌FX
        //   李쎈쭏??scissor ?⑥뒪 ???ㅻⅨ 李쎌뿉?쒕룄 蹂댁씠?? 媛吏쒖갹 諛??щ쭑????洹몃┝.
        if (g_CentiBoss && g_CentiBoss->alive) {
            float ct = (float)glfwGetTime();
            float centiAimX = playerWin.x + playerWin.width  * 0.5f;
            float centiAimY = playerWin.y + playerWin.height * 0.5f;
            BatchFlush(); glEnable(GL_SCISSOR_TEST);
            auto centiPass = [&](float wx, float wy, float ww, float wh) {
                WorldScissor(wx, wy, ww, wh);
                g_CentiBoss->renderFx(ct, centiAimX, centiAimY);
                g_CentiBoss->renderBody(ct);
                for (auto& mb : g_CentiBoss->minis)
                    if (mb.alive) g_CentiBoss->drawMini(mb);
            };
            for (auto& fw : zwins) centiPass(fw.x, fw.y, fw.w, fw.h);
            centiPass(playerWin.x, playerWin.y, playerWin.width, playerWin.height);
            if (g_Stats.turretMode)
                for (auto& tr : g_Turrets)
                    centiPass(tr.x - TURRET_WIN_W*0.5f, tr.y - TURRET_WIN_H*0.5f,
                              TURRET_WIN_W, TURRET_WIN_H);
            BatchFlush(); glDisable(GL_SCISSOR_TEST);
        }

        if (g_TotemBoss && g_TotemBoss->alive) {
            auto* tb = g_TotemBoss;
            float gt = (float)glfwGetTime();
            BatchFlush(); glEnable(GL_SCISSOR_TEST);
            auto totemPass = [&](float wx, float wy, float ww, float wh) {
                WorldScissor(wx, wy, ww, wh);
                tb->renderOrbitGuide(gt);
                tb->renderRiteWeb(gt);
                tb->renderLinks(gt);
                tb->renderCore(gt);
                tb->renderLaser();
                for (int ti = 0; ti < TotemBoss::N_TOTEM; ti++) {
                    auto& tt = tb->totems[ti];
                    if (!tt.alive) continue;
                    float tx0 = tt.x - TotemBoss::WIN_W * g_Scale * 0.5f;
                    float ty0 = tt.y - TotemBoss::WIN_H * g_Scale * 0.5f;
                    float tww = TotemBoss::WIN_W * g_Scale;
                    float thh = TotemBoss::WIN_H * g_Scale;
                    if (tx0 + tww < wx || tx0 > wx + ww ||
                        ty0 + thh < wy || ty0 > wy + wh) continue;
                    tb->renderTotem(tt, gt);
                }
            };
            for (auto& fw : zwins) totemPass(fw.x, fw.y, fw.w, fw.h);
            totemPass(playerWin.x, playerWin.y, playerWin.width, playerWin.height);
            if (g_Stats.turretMode)
                for (auto& tr : g_Turrets)
                    totemPass(tr.x - TURRET_WIN_W * 0.5f, tr.y - TURRET_WIN_H * 0.5f,
                              TURRET_WIN_W, TURRET_WIN_H);
            BatchFlush(); glDisable(GL_SCISSOR_TEST);
        }

        // (g5) ?대━紐⑦봽 蹂댁뒪 ??留덉빱/?몃え/?덉씠?/李⑦겕??蹂몄껜/HP
        if (g_PolyBoss && g_PolyBoss->alive) {
            auto* pb = g_PolyBoss;
            BindMainShader();
            float gt = (float)glfwGetTime();
            BatchFlush(); glEnable(GL_SCISSOR_TEST);
            WorldScissor(pb->worldX - POLY_WIN_W*0.5f, pb->worldY - POLY_WIN_W*0.5f,
                         POLY_WIN_W, POLY_WIN_W);
            // 珥앹븣 (??李??덉뿉?쒕룄 蹂댁씠?꾨줉)
            for (auto& b : g_Bullets) {
                if (!b.active) continue;
                drawBullet(b);
            }
            // 蹂몄껜 ???쇰퀎 紐⑥뼇 (??
            float bsz = PolymorphBoss::BODY;
            float bx = pb->worldX, by = pb->worldY;
            float fr = 0.2f, fg = 1.0f, fb = 0.85f;
            if (pb->form == PForm::DISPLACE) { fr = 1.0f; fg = 0.38f; fb = 0.15f; }
            else if (pb->form == PForm::PHANTOM) { fr = 0.75f; fg = 0.65f; fb = 1.0f; }
            drawCircle(bx, by, bsz * 0.55f, 0.03f, 0.02f, 0.05f, 0.95f);
            drawCircle(bx, by, bsz * 0.72f, fr * 0.35f, fg * 0.35f, fb * 0.35f, 0.45f);
            drawDiamond(bx, by, bsz * 0.82f, fr, fg, fb, 1.0f);
            for (int i = 0; i < PolymorphBoss::DRONE_N; i++) {
                float da = pb->droneAng[i] + pb->droneSpin * (pb->droneActive(i) ? 1.0f : 0.35f);
                float dr = 88.0f + (float)(i % 3) * 16.0f;
                float dx = bx + cosf(da) * dr;
                float dy = by + sinf(da) * dr;
                bool on = pb->droneActive(i);
                float ds = on ? 15.0f : 9.0f;
                float da_a = on ? 0.95f : 0.22f;
                drawCircle(dx, dy, ds * 0.55f, fr, fg, fb, da_a * 0.35f);
                drawDiamond(dx, dy, ds, fr, fg, fb, da_a);
            }
            if (pb->form == PForm::SINGULARITY && pb->blackHoleActive) {
                float mini = std::min(28.0f, pb->holeR * 0.12f);
                drawCircle(bx, by - bsz * 0.12f, mini, 0.0f, 0.0f, 0.0f, 0.75f);
                drawCircle(bx, by - bsz * 0.12f, mini * 1.5f, 0.1f, 0.90f, 0.78f, 0.30f);
            }
            if (pb->damageable()) {
                float pulse = 0.5f + 0.5f * sinf(gt * 14.0f);
                drawCircle(bx, by, bsz * 1.15f, 0.2f, 1.0f, 0.7f, 0.12f + 0.18f * pulse);
            }
            // (HP 諛붾뒗 ?붾㈃ ?곷떒 怨좎젙 蹂댁뒪 諛붾줈 ?대룞)
            // ??蹂???뚰떚????蹂댁뒪 媛쒖씤 李??덉뿉?쒕룄 蹂댁씠?꾨줉 (李?諛??곗뒪?ы넲??????
            for (auto& p : g_EnemyParts) {
                if (!p.active) continue;
                float a = p.life / p.maxLife, hs = p.size * 0.5f;
                drawRect(p.x - hs, p.y - hs, p.size, p.size, p.r, p.g, p.b, a);
            }
            BatchFlush(); glDisable(GL_SCISSOR_TEST);
        }
    
        // ?쒕줎/李⑦겕?뚯? ?곗뒪?ы넲 理쒖긽???대┰ ?놁쓬)??洹몃┛?????꾩そ 蹂댁뒪李?scissor ?⑥뒪媛
        //   poly 蹂댁뒪 ?놁쓣 ?????ロ? ?쒕줎/李⑦겕?뚯씠 ?듭㎏濡??대┰?섎뜕 踰꾧렇 諛⑹?(臾댁“嫄??댁젣).
        BatchFlush(); glDisable(GL_SCISSOR_TEST); BindMainShader();
    
        // (h) ?쒕줎 ??1~2湲?(?ы깙 紐⑤뱶 ???쒕줎 ?뚮뜑 鍮꾪솢??
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
                // ?쒖븞 ?깅궇 ?붿뒪?????ㅽ뙆?댁썾???몃? ??옄)? ?뺤떎??援щ텇
                drawCircle(chx, chy, CHAKRAM_SIZE * 0.62f, 0.3f, 0.9f, 1.0f, 0.28f);   // 湲濡쒖슦
                drawDiamond(chx, chy, CHAKRAM_SIZE * 1.15f, 0.5f, 1.0f, 1.0f, 0.45f);  // ?뚯쟾???뚰듃
                drawCircle(chx, chy, CHAKRAM_SIZE * 0.5f, 0.2f, 0.85f, 1.0f, 1.0f);
                drawCircle(chx, chy, CHAKRAM_SIZE * 0.22f, 0.04f, 0.12f, 0.18f, 1.0f);
                float hpFrac = ch.hp / ch.maxHp;
                if (hpFrac < 0) hpFrac = 0; if (hpFrac > 1) hpFrac = 1;
                drawRect(chx - 14, chy - CHAKRAM_SIZE - 6, 28, 3, 0.2f, 0.2f, 0.2f, 0.7f);
                drawRect(chx - 14, chy - CHAKRAM_SIZE - 6, 28 * hpFrac, 3,
                         1.0f, 0.7f, 0.0f, 0.95f);
            }
        }
        // ?쒕줎/李⑦겕??諛곗튂瑜?吏湲?利됱떆 flush
        BatchFlush();
    
        // ?? (h3) 媛吏?OS 李??щ＼ ????댄?諛?+ [X] ?リ린 (?곗뒪?ы넲 ?멸퀎愿) ??
        //    "??= ?꾨줈?몄뒪, 李쎌쓣 ?レ븘 醫낅즺?쒕떎" ?뺤껜?? ?붾뱶 醫뚰몴(以?諛섏쁺)濡?洹몃┝.
        {
            GameState gst = g_GameManager.currentState;
            bool inGame = (gst == GameState::RUNNING || gst == GameState::PAUSED ||
                           gst == GameState::DYING || gst == GameState::AUG_SELECT ||
                           gst == GameState::DEBUFF_SELECT);
            if (inGame) {
                auto winChrome = [&](float x, float y, float w, float h,
                                     const wchar_t* title, float tr, float tg, float tb,
                                     float gaugePct = -1.0f) {
                    BindMainShader();
                    const float TB = 22.0f;
                    drawRect(x, y, w, TB, tr*0.45f, tg*0.45f, tb*0.55f, 1.0f);
                    drawRect(x, y, w, 2.0f, tr, tg, tb, 1.0f);                    // (李??멸낸??= ?꾨옒/醫????뚮몢由ъ꽑 ?쒓굅 ??李??앹뿉??洹몃젮 ?뚮젅?댁뼱 李??꾨줈
                    //  ?먯졇?섏삤??臾몄젣. ?멸낸 ?꾨젅?꾩? drawNeonBorder(?뚮젅?댁뼱 諛곌꼍蹂대떎 癒쇱?
                    //  洹몃젮??寃뱀튇 遺遺꾩씠 ?щ컮瑜닿쾶 媛?ㅼ쭚)媛 ?대떦.)
                    // 李?而⑦듃濡?(? ??X) ?곗륫
                    float bs = 13.0f, byc = y + (TB-bs)*0.5f, bxc = x + w - 19.0f;
                    drawRect(bxc - 2*(bs+5), byc, bs, bs, 0.22f,0.22f,0.28f,0.9f); // ?
                    drawRect(bxc - (bs+5),   byc, bs, bs, 0.22f,0.22f,0.28f,0.9f);
                    drawRect(bxc, byc, bs, bs, 0.85f, 0.2f, 0.2f, 0.95f);
                    // ?띿뒪?몃뒗 TextRenderer ?먯껜 ?ㅽ겕由?ortho(以?臾댁떆)?? ?붾뱶 醫뚰몴瑜?                    //   W2SX/W2SY 濡?蹂??+ g_ViewZoom ?ㅼ?????以뚮맂 李쎌뿉 ?뺥솗??遺숈쓬
                    g_TextS.Draw(L"X", W2SX(bxc + 3.0f), W2SY(byc - 2.0f),
                                 0.5f * g_ViewZoom, 1,1,1, 0.95f);
                    g_TextS.Draw(title, W2SX(x + 8.0f), W2SY(y + 3.0f),
                                 0.55f * g_ViewZoom, 0.92f,0.96f,1.0f, 0.95f);
                    if (gaugePct >= 0.0f) {
                        wchar_t gp[16];
                        swprintf_s(gp, L"%d%%", (int)(gaugePct * 100.0f + 0.5f));
                        float gs = 0.48f * g_ViewZoom;
                        float gw = g_TextS.Width(gp, gs);
                        g_TextS.Draw(gp, W2SX(x + w - gw - 52.0f), W2SY(y + 3.0f),
                                     gs, 0.55f, 0.98f, 0.85f, 0.95f);
                    }
                };
                int li2 = LangIndex();
                const wchar_t* PNAME = (li2==0) ? L"onedow.exe" : L"onedow.exe";
                // 媛吏쒖갹 ??댄?諛????꾩뿉??留뚮뱺 z-由ъ뒪????쓬?믩넂?? ?쒖꽌濡?洹몃┝.
                //   '?먭린蹂대떎 ?믪? 李? ?먮뒗 '?뚮젅?댁뼱 李?????댄?諛붾? ??쑝硫? ?꾩껜瑜?                //   ?④린??寃??꾨땲??**寃뱀튇 媛濡?援ш컙留?* ?섎씪?몃떎(遺遺??대━??.
                //   ??댄?諛붿쓽 ?몃줈 ??y,y+TB]? 寃뱀튂??媛由쇱갹??x援ш컙??媛?쒓뎄媛꾩뿉??鍮쇨퀬,
                //   ?⑥? 援ш컙?ㅻ쭔 glScissor 濡??대┰??洹몃┛?? (遊뉖꽬<?먭굅由?蹂댁뒪<?뚮젅?댁뼱,
                //   媛숈? ??낆? ?뚰솚?쒖꽌)
                const float TBH = 22.0f;
                glEnable(GL_SCISSOR_TEST);
                auto drawBarClipped = [&](float x, float y, float w, float h,
                                          const wchar_t* nm, float nr, float ng, float nb,
                                          size_t selfIdx, bool isPlayer, float gaugePct = -1.0f) {
                    // 媛??x援ш컙 由ъ뒪??(媛?vec2: x=?쒖옉, y=??
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
                        if (oy < y + TBH && oy + oh > y)   // 媛由쇱갹????댄?諛??몃줈 ?좎? 寃뱀묠
                            subtract(std::max(x, ox), std::min(x + w, ox + ow));
                    };
                    if (!isPlayer)
                        consider(playerWin.x, playerWin.y, playerWin.width, playerWin.height);
                    for (size_t j = selfIdx + 1; j < zwins.size(); j++)
                        consider(zwins[j].x, zwins[j].y, zwins[j].w, zwins[j].h);
                    // ?⑥? 援ш컙留??대┰??洹몃┝
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
                // ?뚮젅?댁뼱 李?????긽 理쒖긽?? ?대┰ ?놁씠 ?꾩껜
                glScissor(0, 0, (GLint)screenWidth, (GLint)screenHeight);
                winChrome(playerWin.x, playerWin.y, playerWin.width, playerWin.height,
                          PNAME, g_AccentR, g_AccentG, g_AccentB);
                BatchFlush();
                glDisable(GL_SCISSOR_TEST);
            }
        }

        // UNKNOWN.sys — blade.sys 미니창 + 화면 가장자리 pin
        if (g_UnknownBoss && g_UnknownBoss->alive) {
            auto* ub = g_UnknownBoss;
            float gtUB = (float)glfwGetTime();
            BindMainShader();
            auto ubWinPass = [&](float wx, float wy, float ww, float wh) {
                BatchFlush(); glEnable(GL_SCISSOR_TEST);
                WorldScissor(wx, wy, ww, wh);
                ub->renderWindowPass(gtUB, wx, wy, ww, wh);
                BatchFlush(); glDisable(GL_SCISSOR_TEST);
            };
            float uwx = ub->worldX - UNKNOWN_WIN_W * 0.5f;
            float uwy = ub->worldY - UNKNOWN_WIN_H * 0.5f;
            ubWinPass(uwx, uwy, UNKNOWN_WIN_W, UNKNOWN_WIN_H);
            ubWinPass(playerWin.x, playerWin.y, playerWin.width, playerWin.height);
            for (auto& bw : ub->bladeWins) {
                if (!bw.active) continue;
                ubWinPass(bw.x, bw.y, bw.w, bw.h);
            }
            const float edgeBand = 84.0f;
            float sw = (float)screenWidth, sh = (float)screenHeight;
            ubWinPass(0.0f, 0.0f, sw, edgeBand);
            ubWinPass(0.0f, sh - edgeBand, sw, edgeBand);
            ubWinPass(0.0f, 0.0f, edgeBand, sh);
            ubWinPass(sw - edgeBand, 0.0f, edgeBand, sh);
        }

    
        // ?? ?ш린遺??UI/?ㅻ쾭?덉씠: 以뙿룻쓷?ㅺ린 臾댁떆?섍퀬 ?붾㈃ 怨좎젙 醫뚰몴(base ortho)濡???
        //    (?대━紐⑦봽 2?섏씠利?以?0.5 ?먯꽌 荑⑤떎?댁뭏쨌硫붾돱?ㅒ룸퉬?ㅽ듃쨌?뚮옒?쒓? 李뚭렇?ъ???踰꾧렇 fix)
        BatchFlush();   // ?붾뱶(以?ortho) ?꾪삎 ?꾨? 洹몃┛ ??base ortho 濡??꾪솚
        glUniformMatrix4fv(g_MainProjLoc, 1, GL_FALSE, g_BaseOrtho);
        memcpy(g_MainOrtho, g_BaseOrtho, sizeof(g_BaseOrtho));
    
        // (h2) boss tint overlay
        {
            bool bossAlive = false;
            glm::vec3 tc(0.6f, 0.3f, 1.0f);
            if (g_RRBoss && g_RRBoss->alive) {
                bossAlive = true; tc = glm::vec3(1.0f, 0.55f, 0.2f); }
            else if (g_PolyBoss && g_PolyBoss->alive) {
                bossAlive = true; tc = glm::vec3(0.6f, 0.25f, 1.0f); }
            else if (g_BotnetBoss && g_BotnetBoss->alive) {
                bossAlive = true; tc = glm::vec3(0.25f, 0.92f, 0.48f); }
            else if (g_CentiBoss && g_CentiBoss->alive) {
                bossAlive = true; tc = glm::vec3(0.35f, 0.88f, 0.95f); }
            else if (g_TotemBoss && g_TotemBoss->alive) {
                bossAlive = true; tc = glm::vec3(0.85f, 0.45f, 0.95f); }
            else if (g_UnknownBoss && g_UnknownBoss->alive) {
                bossAlive = true; tc = glm::vec3(0.95f, 0.28f, 0.58f); }
            if (bossAlive) {
                g_BossTintCol = tc;
                g_BossTintT  += delta * 0.07f;          // ~14珥덉뿉 理쒕?
                if (g_BossTintT > 1.0f) g_BossTintT = 1.0f;
            } else {
                g_BossTintT  -= delta * 0.6f;           // 泥섏튂 ??鍮좊Ⅴ寃??먮났
                if (g_BossTintT < 0.0f) g_BossTintT = 0.0f;
            }
            if (g_BossTintT > 0.001f) {
                BindMainShader();
                drawRect(0, 0, (float)screenWidth, (float)screenHeight,
                         g_BossTintCol.r, g_BossTintCol.g, g_BossTintCol.b,
                         g_BossTintT * 0.06f);
            }
        }
    
        // (h2b) 蹂댁뒪 ?덉씠??HP 諛????붾㈃ ?곷떒 怨좎젙. 紐몄껜 諛??묒? 諛붾뒗 ?꾨컲 ?꾨쭑??        //   臾삵? ??蹂댁씠誘濡? ?쒖꽦 蹂댁뒪??泥대젰???곷떒???ш쾶 ?쒖떆?쒕떎 (?대쫫 + %).
        {
            const wchar_t* bn = nullptr;
            float bhf = 0.0f; glm::vec3 bc(1.0f, 1.0f, 1.0f);
            if (g_RRBoss && g_RRBoss->alive) {
                bn = L"VOLLEY.sys";  bhf = g_RRBoss->hp / g_RRBoss->maxHp;
                bc = glm::vec3(1.0f, 0.55f, 0.2f);
            } else if (g_PolyBoss && g_PolyBoss->alive) {
                bn = L"GLITCH.exe"; bhf = g_PolyBoss->hp / g_PolyBoss->maxHp;
                bc = glm::vec3(0.6f, 0.25f, 1.0f);
            } else if (g_BotnetBoss && g_BotnetBoss->alive) {
                bn = L"C2_RELAY.sys";  bhf = g_BotnetBoss->hp / g_BotnetBoss->maxHp;
                bc = glm::vec3(0.25f, 0.92f, 0.48f);
            } else if (g_CentiBoss && g_CentiBoss->alive) {
                bn = CentipedeBoss::BOSS_NAME;  bhf = g_CentiBoss->hp / g_CentiBoss->maxHp;
                bc = glm::vec3(0.35f, 0.88f, 0.95f);
            } else if (g_TotemBoss && g_TotemBoss->alive) {
                bn = L"RITE.CORE";  bhf = g_TotemBoss->hp / g_TotemBoss->maxHp;
                bc = glm::vec3(0.85f, 0.45f, 0.95f);
            } else if (g_UnknownBoss && g_UnknownBoss->alive) {
                bn = UnknownBoss::BOSS_NAME; bhf = g_UnknownBoss->hp / g_UnknownBoss->maxHp;
                bc = glm::vec3(0.95f, 0.28f, 0.58f);
            }
            int bossPick = -1;
            if (bn) {
                if      (bn == L"VOLLEY.sys")   bossPick = 2;
                else if (bn == L"GLITCH.exe") bossPick = 4;
                else if (bn == L"C2_RELAY.sys")  bossPick = 7;
                else if (bn == CentipedeBoss::BOSS_NAME) bossPick = 8;
                else if (bn == L"RITE.CORE")     bossPick = 9;
                else if (bn == UnknownBoss::BOSS_NAME) bossPick = 1;
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
                // ?대쫫 (諛???以묒븰)
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
                if (g_BotnetBoss && g_BotnetBoss->alive) {
                    wchar_t hostBuf[56];
                    swprintf_s(hostBuf, L"HOST %d/%d  SHIELD %d%%  PKT %d",
                               g_BotnetBoss->aliveHosts(), BotnetBoss::NHOST,
                               (int)(g_BotnetBoss->hostShieldPercent() + 0.5f),
                               g_BotnetBoss->aliveMinions());
                    float hs = 0.55f;
                    float hw = g_TextS.Width(hostBuf, hs);
                    g_TextS.Draw(hostBuf, bx + bw - hw - 8.0f, by - 48.0f, hs,
                                 0.25f, 0.92f, 0.48f, 0.85f);
                }
                if (g_RRBoss && g_RRBoss->alive) {
                    wchar_t rrBuf[64];
                    if (g_RRBoss->phase3)
                        swprintf_s(rrBuf, L"OVERCLOCK 쨌 %ls",
                                   g_RRBoss->state == RRState::ASSAULT ? L"ASSAULT" :
                                   ReloadRunnerBoss::weaponTag(g_RRBoss->weapon));
                    else if (g_RRBoss->phase2)
                        swprintf_s(rrBuf, L"P2 쨌 %ls",
                                   g_RRBoss->state == RRState::RELOAD_SPRINT ? L"SPRINT" :
                                   ReloadRunnerBoss::weaponTag(g_RRBoss->weapon));
                    else
                        swprintf_s(rrBuf, L"%ls",
                                   ReloadRunnerBoss::weaponTag(g_RRBoss->weapon));
                    float rs = 0.55f;
                    float rw = g_TextS.Width(rrBuf, rs);
                    g_TextS.Draw(rrBuf, bx + bw - rw - 8.0f, by - 48.0f, rs,
                                 1.0f, 0.55f, 0.2f, 0.85f);
                }
                if (g_TotemBoss && g_TotemBoss->alive) {
                    wchar_t totBuf[64];
                    if (g_TotemBoss->vulnerable())
                        swprintf_s(totBuf, L"RITE DOWN %.0fs", g_TotemBoss->vulnTimer);
                    else
                        swprintf_s(totBuf, L"W%d 쨌 %d/4", g_TotemBoss->wave, g_TotemBoss->aliveTotems());
                    float ts = 0.55f;
                    float tw = g_TextS.Width(totBuf, ts);
                    g_TextS.Draw(totBuf, bx + bw - tw - 8.0f, by - 48.0f, ts,
                                 0.85f, 0.45f, 0.95f, 0.85f);
                }
                if (g_UnknownBoss && g_UnknownBoss->alive) {
                    wchar_t ubBuf[64];
                    swprintf_s(ubBuf, L"edge:%d cage:%d pin:%d · %ls",
                               g_UnknownBoss->edgeBladesLeft(),
                               g_UnknownBoss->quiver,
                               (int)g_UnknownBoss->pins.size(),
                               UnknownBoss::stateTag(g_UnknownBoss->state));
                    float us = 0.55f;
                    float uw = g_TextS.Width(ubBuf, us);
                    g_TextS.Draw(ubBuf, bx + bw - uw - 8.0f, by - 48.0f, us,
                                 0.95f, 0.35f, 0.65f, 0.88f);
                }
                // % (諛??곗륫 ???덉そ)
                wchar_t pct[16]; swprintf_s(pct, L"%d%%", (int)(bhf * 100.0f + 0.5f));
                float ps = 0.7f;
                float pw = g_TextS.Width(pct, ps);
                g_TextS.Draw(pct, bx + bw - pw - 8.0f, by + 3.0f, ps, 1.0f, 1.0f, 1.0f, 0.95f);
            }
        }
    
        // (h2c) 蹂댁뒪 ?깆옣 ?꾩“(利앹긽) ???ㅽ룿 2.5珥????뚮쭏 ?곗텧 + 寃쎄퀬 諛곕꼫
        if (g_BossWarnTimer > 0.0f &&
            (g_GameManager.currentState == GameState::RUNNING ||
             g_GameManager.currentState == GameState::PAUSED)) {
            float warnDur = g_CreativeMode ? 0.35f : BOSS_WARN_DUR;
            float prog = 1.0f - g_BossWarnTimer / warnDur;   // 0→1
            if (prog < 0.0f) prog = 0.0f; if (prog > 1.0f) prog = 1.0f;
            float t = (float)glfwGetTime();
            float blink = 0.5f + 0.5f * sinf(t * 9.0f);
            glm::vec3 wc = BossDir::WarnColor(g_BossWarnPick);
            BindMainShader();
            float sw2 = (float)screenWidth, sh2 = (float)screenHeight;
            // 怨듯넻: 媛?μ옄由?鍮꾨꽕??(蹂댁뒪?? progress 鍮꾨?濡?吏숈뼱吏?
            float ea = (0.05f + 0.13f * prog) * (0.6f + 0.4f * blink);
            float eb = 70.0f;
            drawRect(0, 0, sw2, eb, wc.r, wc.g, wc.b, ea);
            drawRect(0, sh2 - eb, sw2, eb, wc.r, wc.g, wc.b, ea);
            drawRect(0, 0, eb, sh2, wc.r, wc.g, wc.b, ea);
            drawRect(sw2 - eb, 0, eb, sh2, wc.r, wc.g, wc.b, ea);
    
            // 蹂댁뒪蹂?利앹긽 ?뚮쭏
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
            case 4: {
                for (int i = 0; i < 3; i++) {
                    float r = (80.0f + i * 120.0f) + prog * 200.0f;
                    drawCircle(sw2 * 0.5f, sh2 * 0.5f, r, wc.r, wc.g, wc.b,
                               (0.05f + 0.05f * blink) * (1.0f - (float)i * 0.25f));
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
            case 8: {  // FORK ???섎떒?먯꽌 湲곗뼱?ㅻ뒗 遺꾩젅 紐명넻
                float crawl = 40.0f + 90.0f * prog;
                for (int s = 0; s < 9; s++) {
                    float sx = sw2 * 0.08f + s * sw2 * 0.105f;
                    float bob = sinf(t * 6.0f + s * 0.7f) * 6.0f;
                    drawCircle(sx, sh2 - crawl + bob, 14.0f + (float)(s % 3) * 3.0f,
                               0.35f, 0.85f, 0.25f, 0.2f + 0.15f * prog);
                }
            } break;
            case 9: {  // RITE ??沅ㅻ룄 留?+ 湲곕뫁 + ?↔컖 肄붿뼱
                float cx = sw2 * 0.5f, cy = sh2 * 0.5f;
                float R = (sw2 < sh2 ? sw2 : sh2) * 0.28f;
                for (int i = 0; i < 16; i++) {
                    if ((i & 1) == 0) continue;
                    float a0 = (float)i / 16.0f * 6.283f + t * 0.4f;
                    float a1 = (float)(i + 1) / 16.0f * 6.283f + t * 0.4f;
                    float x0 = cx + cosf(a0) * R, y0 = cy + sinf(a0) * R * 0.85f;
                    float x1 = cx + cosf(a1) * R, y1 = cy + sinf(a1) * R * 0.85f;
                    drawRect((x0 + x1) * 0.5f - 2, (y0 + y1) * 0.5f - 2, 4, 4,
                             wc.r, wc.g, wc.b, 0.12f + 0.15f * blink);
                }
                for (int c = 0; c < 4; c++) {
                    float a = (float)c * 1.571f + t * 0.5f;
                    float px = cx + cosf(a) * R, py = cy + sinf(a) * R * 0.85f;
                    float th = 24.0f + 90.0f * prog;
                    drawRect(px - 8.0f, py - th * 0.5f, 16.0f, th, wc.r, wc.g, wc.b,
                             0.18f + 0.22f * blink);
                }
                for (int i = 0; i < 6; i++) {
                    float a = t * 1.2f + (float)i * 1.047f;
                    float hx = cx + cosf(a) * (38.0f + prog * 22.0f);
                    float hy = cy + sinf(a) * (38.0f + prog * 22.0f);
                    drawTriangle(hx, hy, 10.0f, wc.r, wc.g, wc.b, 0.35f + 0.25f * blink);
                }
            } break;
            default: break;
            }
    
            // 怨듯넻 寃쎄퀬 諛곕꼫 (以묒븰 ?곷떒履?
            float by3 = sh2 * 0.30f;
            // ?대쫫 (?)
            wchar_t banner[64]; swprintf_s(banner, L"!! %ls !!", g_BossWarnName);
            float nsc = 1.5f;
            float nw2 = g_TextL.Width(banner, nsc);
            g_TextL.Draw(banner, (sw2 - nw2) * 0.5f, by3, nsc,
                         wc.r, wc.g, wc.b, 0.7f + 0.3f * blink);
            const wchar_t* SUB[3] = { L"?꾪삊 ?꾨줈?몄뒪 媛먯? ???ㅽ뻾 以?..",
                                       L"THREAT PROCESS DETECTED ??launching...",
                                       L"?끻쮤?쀣꺆?삠궧濾쒎눣 ??若잒죱訝?.." };
            int li5 = LangIndex();
            float ssc = 0.7f;
            float sw3 = g_TextS.Width(SUB[li5], ssc);
            g_TextS.Draw(SUB[li5], (sw2 - sw3) * 0.5f, by3 + 44.0f, ssc, 1.0f, 1.0f, 1.0f, 0.85f);
            // 吏꾪뻾 寃뚯씠吏
            float gw = 320.0f, gh = 8.0f, gx = (sw2 - gw) * 0.5f, gy = by3 + 74.0f;
            BindMainShader();
            drawRect(gx - 2, gy - 2, gw + 4, gh + 4, 0.0f, 0.0f, 0.0f, 0.6f);
            drawRect(gx, gy, gw, gh, 0.15f, 0.15f, 0.18f, 0.9f);
            drawRect(gx, gy, gw * prog, gh, wc.r, wc.g, wc.b, 0.95f);
        }
    
        // (h2d) ?섏씠利? 吏꾩엯 ?좎뒪????"??怨쇰?????PHASE 2" (1.8珥??섏씠??
        if (g_P2ToastTimer > 0.0f &&
            (g_GameManager.currentState == GameState::RUNNING ||
             g_GameManager.currentState == GameState::DYING)) {
            float a = (g_P2ToastTimer > 1.4f) ? (1.8f - g_P2ToastTimer) / 0.4f
                                              : (g_P2ToastTimer / 1.4f);
            if (a > 1.0f) a = 1.0f; if (a < 0.0f) a = 0.0f;
            const wchar_t* P2[3] = { L"??怨쇰?????PHASE 2", L"??OVERLOAD ??PHASE 2",
                                      L"???롨쿋????PHASE 2" };
            int li6 = LangIndex();
            float psc = 1.2f;
            float pw3 = g_TextL.Width(P2[li6], psc);
            glm::vec3& pc = g_P2ToastCol;
            g_TextL.Draw(P2[li6], ((float)screenWidth - pw3) * 0.5f,
                         (float)screenHeight * 0.22f, psc, pc.r, pc.g, pc.b, a);
        }
    
        // (h3) ?붾㈃ ?뚮옒?????덈꺼??蹂댁뒪泥섏튂/遺???쒓컙 踰덉찉
        if (g_FlashIntensity > 0.001f) {
            BindMainShader();
            float a = g_FlashIntensity; if (a > 0.85f) a = 0.85f;
            drawRect(0, 0, (float)screenWidth, (float)screenHeight,
                     g_FlashColor.r, g_FlashColor.g, g_FlashColor.b, a);
        }
        // FORK.worm ?덊뵾 ???媛??RGB 遺꾨━ 湲由ъ튂
        if (g_CentiBoss && g_CentiBoss->glitchOverlay > 0.001f) {
            BindMainShader();
            float go = g_CentiBoss->glitchOverlay * 7.0f;
            if (go > 1.0f) go = 1.0f;
            float off = 7.0f * go;
            float sw = (float)screenWidth, sh = (float)screenHeight;
            drawRect(off, 0.0f, sw, sh, 1.0f, 0.15f, 0.15f, go * 0.07f);
            drawRect(-off, 0.0f, sw, sh, 0.15f, 0.85f, 1.0f, go * 0.07f);
        }
    
        // (h4) ?쒓컙 ?뺤? ???붾㈃ 媛?μ옄由??쒖븞 鍮꾨꽕??(?뺤? ?곗텧)
        if (g_TimeStopTimer > 0.0f) {
            BindMainShader();
            float a = 0.10f + 0.05f * sinf((float)glfwGetTime() * 10.0f);
            float bw = 18.0f;
            drawRect(0, 0, (float)screenWidth, bw, 0.3f, 0.9f, 1.0f, a);
            drawRect(0, (float)screenHeight - bw, (float)screenWidth, bw, 0.3f, 0.9f, 1.0f, a);
            drawRect(0, 0, bw, (float)screenHeight, 0.3f, 0.9f, 1.0f, a);
            drawRect((float)screenWidth - bw, 0, bw, (float)screenHeight, 0.3f, 0.9f, 1.0f, a);
        }
    
        // (h5) ?쇨꺽 ??鍮④컙 媛?μ옄由?鍮꾨꽕??(?쇨꺽 ?移?
        if (g_HurtVignette > 0.001f) {
            BindMainShader();
            float a = g_HurtVignette * 0.55f;
            float bw = 60.0f * g_HurtVignette + 14.0f;
            drawRect(0, 0, (float)screenWidth, bw, 0.95f, 0.1f, 0.1f, a);
            drawRect(0, (float)screenHeight - bw, (float)screenWidth, bw, 0.95f, 0.1f, 0.1f, a);
            drawRect(0, 0, bw, (float)screenHeight, 0.95f, 0.1f, 0.1f, a);
            drawRect((float)screenWidth - bw, 0, bw, (float)screenHeight, 0.95f, 0.1f, 0.1f, a);
        }
    
        // (i) 痍⑦븿 ?곹깭 ?쒖떆 (?붾㈃ 媛?μ옄由??먰솉??鍮꾨꽕??
        if (g_DrunkActive) {
            float a = 0.18f;
            drawRect(0, 0, (float)screenWidth, 12.0f, 0.9f, 0.1f, 0.6f, a);
            drawRect(0, (float)screenHeight - 12, (float)screenWidth, 12.0f, 0.9f, 0.1f, 0.6f, a);
            drawRect(0, 0, 12.0f, (float)screenHeight, 0.9f, 0.1f, 0.6f, a);
            drawRect((float)screenWidth - 12, 0, 12.0f, (float)screenHeight, 0.9f, 0.1f, 0.6f, a);
        }
    
        }   // if (inWorldRender)
        }   // world render game block
    
        // [6] HUD
        g_GameManager.Render();
    
        // ?? [7] ?쒓뎅???띿뒪??+ 硫붾돱 ?????????????????????????????????????????
        {
            float sw = (float)screenWidth, sh = (float)screenHeight;
            auto  st = g_GameManager.currentState;
    
    
            // ??李?shop/codex/config) 吏꾩엯 ???대┝ ?좊땲硫붿씠???쒖옉 ??留??꾨젅??吏꾪뻾
            {
                static GameState s_prevWinSt = GameState::MAIN_MENU;
                bool isApp = (st == GameState::SHOP || st == GameState::CODEX ||
                              st == GameState::SETTINGS);
                if (st != s_prevWinSt) {
                    if (isApp) g_AppOpen = 0.0f;   // ??李????ш린 0?먯꽌 ?닿린
                    s_prevWinSt = st;
                }
                if (isApp && g_AppOpen < 1.0f) {
                    g_AppOpen += delta / APP_OPEN_DUR;
                    if (g_AppOpen > 1.0f) g_AppOpen = 1.0f;
                }
            }
    
            // ?? ?멸쾶???묒뾽?쒖떆以???硫붾돱? ?숈씪??OS ?꾨젅???좎? (?곗뒪?ы넲 諛⑹뼱 ?쇨??? ??
            if (st == GameState::RUNNING || st == GameState::PAUSED || st == GameState::DYING ||
                st == GameState::AUG_SELECT || st == GameState::DEBUFF_SELECT ||
                (st == GameState::SETTINGS && g_SettingsReturnTo == GameState::PAUSED)) {
                DrawIngameTaskbar(sw, sh,
                    (st == GameState::SETTINGS) ? GameState::PAUSED : st);
            }
    
            // ?? ?낆쟻 ?닿툑 ?좎뒪??(?곷떒 以묒븰 諛곕꼫, 4珥??쒖떆 ???섏씠?? ??
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

            // ?? ?щ━?먯씠?곕툕 HUD (寃뚯엫 以? ??F:利앷컯  G:臾댁쟻 ??
            if (g_CreativeMode &&
                (st == GameState::RUNNING || st == GameState::READY ||
                 st == GameState::AUG_SELECT || st == GameState::DEBUFF_SELECT)) {
                int li3 = LangIndex();
                const wchar_t* CH[3] = {
                    L"CREATIVE   F: 증강   G: 무적   B: 보스 스폰",
                    L"CREATIVE   F: Augment   G: Godmode   B: Spawn boss",
                    L"CREATIVE   F: 強化   G: ゴッド   B: ボス召喚" };
                g_TextS.Draw(CH[li3], 20.0f, HudY(sh, Hud::CREATIVE_LABEL), 0.8f, 0.7f, 0.85f, 1.0f, 0.85f);
                if (g_CreativeGodmode) {
                    const wchar_t* GOD[3] = { L"★ 무적 ON", L"★ GODMODE ON", L"★ ゴッド ON" };
                    float blink = 0.65f + 0.35f * sinf((float)glfwGetTime() * 5.0f);
                    g_TextL.Draw(GOD[li3], 20.0f, 24.0f, 0.95f, 1.0f, 0.85f, 0.2f, blink);
                }
            }
    
            // ?? [7b] UI ???붿뒪?⑥튂 ??硫붾돱/李??곹깭??Scene_* ?⑥닔濡?遺꾨━ ??
            //    RUNNING/DYING(?쒖닔 ?멸쾶?????ъ씠 ?놁쑝??而⑦뀓?ㅽ듃 援ъ꽦 ?먯껜瑜?嫄대꼫?.
            if (st != GameState::RUNNING && st != GameState::DYING) {
                std::function<void()> resetFn = ResetForNewGame;
                SceneCtx ctx{ sw, sh, mx, my, lmb, delta, window, &fireTimer, resetFn };
                switch (st) {
                case GameState::MAIN_MENU:         Scene_MainMenu(ctx);         break;
                case GameState::SHOP:              Scene_Shop(ctx);             break;
                case GameState::CODEX:             Scene_Codex(ctx);            break;
                case GameState::JOB_SELECT:        Scene_JobSelect(ctx);        break;
                case GameState::WEAPON_SELECT:     Scene_WeaponSelect(ctx);     break;
                case GameState::DIFFICULTY_SELECT: Scene_DifficultySelect(ctx); break;
                case GameState::CREATIVE_CONFIG:   Scene_CreativeConfig(ctx);   break;
                case GameState::SETTINGS:          Scene_Settings(ctx);         break;
                case GameState::READY:             Scene_Ready(ctx);            break;
                case GameState::PAUSED:            Scene_Paused(ctx);           break;
                case GameState::GAMEOVER:          Scene_GameOver(ctx);         break;
                case GameState::AUG_SELECT:
                case GameState::DEBUFF_SELECT:     Scene_AugSelect(ctx);        break;
                case GameState::RUN_SHOP:          Scene_RunShop(ctx);          break;
                default: break;
                }
                if (st == GameState::PAUSED || st == GameState::AUG_SELECT ||
                    st == GameState::DEBUFF_SELECT || st == GameState::GAMEOVER ||
                    st == GameState::RUN_SHOP)
                    Scene_OwnedAugPanel(ctx);
            }
    
            // ?곷떒 HUD ???ㅼ젣 寃뚯엫 吏꾪뻾 ?곹깭?먯꽌留?(硫붾돱/?꾧컧/?곸젏?????④쾶)
            if (st == GameState::RUNNING || st == GameState::PAUSED ||
                st == GameState::DYING   || st == GameState::AUG_SELECT ||
                st == GameState::DEBUFF_SELECT ||
                st == GameState::RUN_SHOP ||
                (st == GameState::RUNNING && g_InBossIntermission)) {
#ifdef __APPLE__
                const float hudTopY = 8.0f + 30.0f;
#else
                const float hudTopY = 8.0f;
#endif
                // 醫뚯긽?? Lv. + HP ?レ옄 (?쒓컖 諛붾뒗 ?뚮젅?댁뼱 李쎌뿉 遺李⑸맖)
                {
                    int hpCur = (int)(g_GameManager.playerHP + 0.5f);
                    int hpMax = (int)(g_Stats.maxHP + 0.5f);
                    wchar_t lvBuf2[64];
                    swprintf_s(lvBuf2, L"%ls%d    HP %d/%d",
                               T(StrId::LV_PREFIX), g_GameManager.playerLevel, hpCur, hpMax);
                    g_TextS.Draw(lvBuf2, 12.0f, hudTopY, 0.85f, 0.7f, 1.0f, 0.7f, 0.9f);
                }
    
                // ?곷떒 以묒븰: Score
                wchar_t scoreBuf[64];
                swprintf_s(scoreBuf, L"%ls  %lld", T(StrId::SCORE), g_GameManager.score);
                float scoreW = g_TextS.Width(scoreBuf, 1.0f);
                g_TextS.Draw(scoreBuf, (sw - scoreW) * 0.5f, hudTopY, 1.0f,
                             1.0f, 1.0f, 1.0f, 0.95f);
    
                // ?곗긽?? FPS
                wchar_t fpsBuf[32];
                swprintf_s(fpsBuf, L"%ls  %d", T(StrId::FPS), g_CurrentFPS);
                float fpsW = g_TextS.Width(fpsBuf, 0.85f);
                g_TextS.Draw(fpsBuf, sw - fpsW - 12.0f, hudTopY, 0.85f,
                             0.7f, 0.9f, 1.0f, 0.85f);

                if (st == GameState::RUNNING || st == GameState::RUN_SHOP ||
                    g_InBossIntermission) {
                    const wchar_t* actLbl = BossDir::ActLabel();
                    g_TextS.Draw(actLbl, 12.0f, hudTopY + 22.0f, 0.72f,
                                 0.82f, 0.92f, 1.0f, 0.82f);
                    wchar_t goldHud[32];
                    swprintf_s(goldHud, L"G  %lld", g_RunGold);
                    float gdw = g_TextS.Width(goldHud, 0.82f);
                    g_TextS.Draw(goldHud, sw - gdw - 12.0f, hudTopY + 22.0f, 0.82f,
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
                        L"중앙 AUGMENT.store — 잠시 머물면 상점 · 오른쪽 = 스킵",
                        L"Center AUGMENT.store — hold to shop · right zone = skip",
                        L"中央 AUGMENT.store — 留まるとショップ · 右=スキップ" };
                    int zli = LangIndex();
                    if (zli < 0 || zli > 2) zli = 0;
                    float zw = g_TextS.Width(zhint[zli], 0.78f);
                    g_TextS.Draw(zhint[zli], (sw - zw) * 0.5f, hudTopY + 82.0f, 0.78f,
                                 0.85f, 0.85f, 0.85f, 0.75f);
                }
    
                // (HP/EXP ?쒓컖 諛붾뒗 ?뚮젅?댁뼱 李??섎떒??遺李⑸맖 ????(c2) 李멸퀬)
                // ?섎떒 議곗옉 ?덈궡 (?щ??섍쾶 ??긽) ?????뚮젅?댁뼱媛 HP/?ㅽ궗 ?꾩튂瑜??뚭쾶
                if (st == GameState::RUNNING) {
                    const wchar_t* c =
                        (g_Language == Language::KR)
                            ? L"WASD Move   Mouse Fire   SHIFT Dash   Q/E/R Skills   ESC Pause"
                        : (g_Language == Language::JP)
                            ? L"WASD Move   Mouse Fire   SHIFT Dash   Q/E/R Skills   ESC Pause"
                            : L"WASD Move   Mouse Fire   SHIFT Dash   Q/E/R Skills   ESC Pause";
                    float cw = g_TextS.Width(c, 0.7f);
                    g_TextS.Draw(c, CenterX(sw, cw), HudY(sh, Hud::COMBO_TEXT), 0.7f,
                                 0.7f, 0.82f, 0.95f, 0.72f);   // 媛?쒖꽦 ??(?낆뼱????蹂댁씤?ㅻ뒗 ?쇰뱶諛?
                }
                // ?? ?泥대젰 寃쎄퀬 ??HP 25% ?댄븯 ??媛?μ옄由?遺?쒕윭???곸깋 ?꾩뒪 + ?띿뒪????
                if (st == GameState::RUNNING || st == GameState::PAUSED) {
                    float hpFrac = (g_Stats.maxHP > 0.0f)
                                 ? g_GameManager.playerHP / g_Stats.maxHP : 1.0f;
                    if (hpFrac > 0.0f && hpFrac < 0.25f) {
                        float pulse = 0.5f + 0.5f * sinf((float)glfwGetTime() * 6.0f);
                        float sev   = 1.0f - hpFrac / 0.25f;
                        float a     = (0.08f + 0.13f * pulse) * (0.5f + 0.5f * sev);
                        BindMainShader();
                        float bw = 64.0f;
                        drawRect(0, 0, sw, bw, 0.9f, 0.15f, 0.15f, a);
                        drawRect(0, sh - bw, sw, bw, 0.9f, 0.15f, 0.15f, a);
                        drawRect(0, 0, bw, sh, 0.9f, 0.15f, 0.15f, a);
                        drawRect(sw - bw, 0, bw, sh, 0.9f, 0.15f, 0.15f, a);
                        const wchar_t* LOW[3] = { L"???꾪뿕", L"??LOW HP", L"???깁쇇" };
                        int li4 = LangIndex();
                        float lw = g_TextS.Width(LOW[li4], 0.9f);
                        g_TextS.Draw(LOW[li4], CenterX(sw, lw), HudY(sh, Hud::LOW_HP_WARN),
                                     0.9f, 1.0f, 0.4f, 0.4f, 0.55f + 0.45f * pulse);
                    }
                }
            }
    
            // ?? ?먮쭧: ?곕?吏 ?レ옄 ?앹뾽 + 肄ㅻ낫 移댁슫??(?ㅼ젣 ?뚮젅??以묒뿉留? ??
            //    硫붾돱/?쇱떆?뺤??먯꽑 ?④? ???띿뒪?멸? 硫붾돱 ?꾩뿉 ?⑤뜕 踰꾧렇 fix
            if (st == GameState::RUNNING || st == GameState::DYING) {
                // ?곕?吏 ?レ옄 (?붾뱶 ???ㅽ겕由?蹂?????띿뒪?? ???ㅼ젙 ?좉?
                if (g_ShowDamageNumbers)
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
                // 肄ㅻ낫 移댁슫??(5肄ㅻ낫 ?댁긽遺?? ?됱씠 肄ㅻ낫???곕씪 媛뺥빐吏? ???ㅼ젙 ?좉?
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
                    if (ms) { cr = 1.0f; cg = 0.85f; cbl = 0.30f; }   // 留덉씪?ㅽ넠 = 怨⑤뱶 ?꾩뒪
                    float w = g_TextS.Width(cb, sc);
                    g_TextS.Draw(cb, (sw - w) * 0.5f, sh * 0.115f, sc, cr, cg, cbl, 0.95f);
                }
            }
    
            // ?? ?≫떚釉??ㅽ궗 ?щ’ (醫뚰븯?? ?⑥떆釉?荑⑤떎?????? ??
            if (st == GameState::RUNNING || st == GameState::PAUSED) {
                const float KW = 54.0f, KH = 48.0f, KG = 8.0f;
                float kx0 = 16.0f, ky0 = HudY(sh, KH + Hud::SKILL_KEYS_Y);
                auto skillBox = [&](int idx, const wchar_t* key, const wchar_t* tag,
                                    float cd, float r, float g, float b) {
                    float x = kx0 + idx * (KW + KG), y = ky0;
                    bool ready = (cd <= 0.0f);
                    drawRect(x, y, KW, KH, 0.05f, 0.05f, 0.08f, 0.88f);
                    if (!ready) drawRect(x, y, KW, KH, 0.0f, 0.0f, 0.0f, 0.55f);
                    drawRect(x, y, KW, 4.0f, r, g, b, ready ? 1.0f : 0.45f);
                    g_TextS.Draw(key, x + 4.0f, y + 4.0f, 0.55f, 1, 1, 1, 0.9f);
                    g_TextS.Draw(tag, x + 4.0f, y + KH - 17.0f, 0.5f, r, g, b, ready ? 1.0f : 0.5f);
                    if (!ready) {
                        wchar_t bf[8]; swprintf_s(bf, L"%d", (int)(cd + 0.99f));
                        float tw = g_TextL.Width(bf, 0.85f);
                        g_TextL.Draw(bf, x + (KW - tw) * 0.5f, y + KH * 0.34f, 0.85f, 1,1,1,0.95f);
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
                    if (g_TotemBoss && g_TotemBoss->alive && g_TotemBoss->isSkillSealed(i)) {
                        float x = kx0 + (float)(i + 1) * (KW + KG), y = ky0;
                        drawRect(x, y, KW, KH, 0.05f, 0.05f, 0.05f, 0.55f);
                        g_TextS.Draw(L"SEAL", x + 6.0f, y + 20.0f, 0.45f, 0.9f, 0.3f, 0.35f, 0.9f);
                    }
                }
            }
    
            // ?? ?≫떚釉??⑥떆釉?荑⑤떎??UI (醫뚰븯?? ?????????????????????
            // 異뷀썑 ?쏀넗洹몃옩 PNG 媛 ?ㅼ뼱?ㅻ㈃ ?ш컖??placeholder ?먮━???띿뒪泥??쒖떆
            if (st == GameState::RUNNING || st == GameState::PAUSED) {
                const float SLOT_W = 56.0f, SLOT_H = 48.0f, SLOT_GAP = 8.0f;
                float baseX  = 16.0f;
                float baseY2 = HudY(sh, SLOT_H + Hud::SLOT_BAR_BASE);   // HP 諛??꾩そ(?묒뾽?쒖떆以???
                int   slot   = 0;
    
                auto drawSlot = [&](const wchar_t* tag, float remain,
                                    float r, float g, float b) {
                    float x = baseX + slot * (SLOT_W + SLOT_GAP);
                    float y = baseY2;
                    // 諛곌꼍
                    drawRect(x, y, SLOT_W, SLOT_H, 0.05f, 0.05f, 0.08f, 0.85f);
                    // 吏꾪뻾??(?꾟넂?꾨옒 梨꾩썙吏吏 ?딆? 遺遺?= 荑⑦???
                    if (remain > 0.0f) {
                        // ?대몢???ㅻ쾭?덉씠 (?⑥? 鍮꾩쑉留뚰겮 ?꾩뿉?쒕???梨꾩?)
                        // remain ?뺢퇋?붾뒗 ?몄텧 ?쒖젏?먯꽌 泥섎━?섍린 ?대젮?곕땲 alpha 0.55 怨좎젙
                        drawRect(x, y, SLOT_W, SLOT_H, 0.0f, 0.0f, 0.0f, 0.55f);
                    }
                    // 而щ윭 ?뚮몢由?(?꾩そ ??
                    drawRect(x, y, SLOT_W, 4.0f, r, g, b, 1.0f);
                    // ?쒓렇 (?곷Ц/?쎌뼱 ???쏀넗洹몃옩 ?ㅼ뼱?ㅻ㈃ ?쒓굅)
                    g_TextS.Draw(tag, x + 4.0f, y + 6.0f, 0.7f, r, g, b, 1.0f);
                    // ?⑥? ?쒓컙 (?뺤닔)
                    if (remain > 0.0f) {
                        wchar_t buf[16];
                        swprintf_s(buf, L"%d", (int)(remain + 0.99f));
                        float tw = g_TextL.Width(buf, 0.9f);
                        g_TextL.Draw(buf, x + (SLOT_W - tw) * 0.5f,
                                     y + SLOT_H * 0.40f, 0.9f, 1,1,1,0.95f);
                    }
                };
    
                // ?꾪솚 ?몃? ??荑⑤떎??(20 / 15 / 7.5)
                if (g_Stats.bulletRain) {
                    float remain = g_Stats.bulletRainCooldown - g_BulletRainTimer;
                    if (remain < 0) remain = 0;
                    drawSlot(L"RAIN", remain, 1.0f, 0.5f, 0.2f);
                    ++slot;
                }
                // 痍⑦븿 ??drunkCooldown ?湲?/ drunkActiveDuration ?쒖꽦
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
                // ?ㅺ??ㅻ뒗 二쎌쓬 ???쒖꽦 + stack (?띾룄 +20%/?ㅽ깮)
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
        }

        g_LmbPrev = lmb;

        // ?낆쟻 ?닿툑 / ?꾧컧 諛쒓껄 諛쒖깮 ?????(寃뚯엫 以?利됱떆 ?곴뎄??
        if (g_AchSaveNeeded || g_CodexDirty) {
            SaveGame(); g_AchSaveNeeded = false; g_CodexDirty = false;
        }

        BatchFlush();   // ?꾨젅??留덉?留????⑥? ?꾪삎 紐⑤몢 洹몃┝
        glfwSwapBuffers(window);

        // ?? FPS 罹?(g_FpsCap > 0 ???뚮쭔) ?????????????????????????????
        // timeBeginPeriod(1) 濡?Sleep ?댁긽??1ms. 留덉?留?~1ms ??busy-wait
        // C18: 怨쇨굅 '臾댁젣??(-1) ?몄씠釉뚮뒗 300 ?쇰줈 ?대옩??(吏꾩쭨 臾댁젣???쒓굅).
        int capFps = (g_FpsCap < 0) ? 300 : g_FpsCap;
        if (capFps > 0) {
            double target = 1.0 / (double)capFps;
            double frameStart = (double)now;
            double remain = target - (glfwGetTime() - frameStart);
            if (remain > 0.001) {
                // 留덉?留?1ms 留??④린怨?Sleep
                unsigned ms = (unsigned)((remain - 0.001) * 1000.0);
                PlatformSleepMs(ms);
            }
            // ?붿뿬 busy-wait (?뺥솗??罹?
            while (glfwGetTime() - frameStart < target) { /* spin */ }
        }
    }

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
    if (g_OswaldMemHandle) {
        RemoveFontMemResourceEx(g_OswaldMemHandle);
        g_OswaldMemHandle = nullptr;
    }
#endif
    Audio::Shutdown();
    PlatformTimerEnd();
    glfwTerminate();
    return 0;
}
