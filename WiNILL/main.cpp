// Windows 헤더를 glad보다 먼저 — APIENTRY 매크로 중복 정의 방지
#ifdef _WIN32
  #include <windows.h>
  #include <dwmapi.h>   // DwmIsCompositionEnabled (진단용)
  #include <timeapi.h>  // timeBeginPeriod / timeEndPeriod (FPS 캡 정밀도)
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
#include "SpamBoss.h"
#include "KernelBoss.h"
#include "FirewallBoss.h"
#include "BotnetBoss.h"
#include "CentipedeBoss.h"
#include "TotemBoss.h"
#include "BossDirector.h"
#include "CollisionSystem.h"
#include "Augment.h"
#include "PlayerStats.h"
#include "TextRenderer.h"
#include "Settings.h"
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
#pragma comment(lib, "winmm.lib")      // timeBeginPeriod / timeEndPeriod (FPS 캡 정밀도)
// dwmapi.lib 은 WindowFx.cpp 에서 링크
#endif

extern "C++" {
    __declspec(dllexport) DWORD NvOptimusEnablement = 0;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 0;
}

// 투명도 헬퍼는 WindowFx.h/cpp 로 이동
// (EnableWindowTransparency 와 TransparencyLog 가 동일 기능)
#define DBG TransparencyLog

GameManager    g_GameManager;
MonsterManager g_MonsterManager;
std::vector<Bullet> g_Bullets;

// ── 개발자(크리에이티브) 모드 — 도감 검색창 이스터에그로만 해금 ──
//    출시 빌드엔 크리에이티브 진입점이 숨겨져 있고, 도감 검색에 시크릿 코드를
//    입력하면 해금되어 난이도 화면에 토글이 등장한다. (일반 플레이어는 못 켬)
bool    g_DevUnlocked   = false;
float   g_DevToastTimer = 0.0f;            // 해금 확인 토스트 (초)
// 설정창 볼륨 숫자 직접입력 상태
bool    g_VolEdit = false;
wchar_t g_VolBuf[8] = {0};
int     g_VolLen = 0;
PlayerStats    g_Stats;
bool g_aug1Released = true, g_aug2Released = true, g_aug3Released = true;
TextRenderer   g_TextL;   // 큰 글자 (증강 이름, 상태 타이틀)
TextRenderer   g_TextS;   // 작은 글자 (설명, 힌트)
TextRenderer   g_TextXL;  // 초대형 타이틀(시작창 로고) 전용 — 고해상도 래스터

#ifdef _WIN32
HANDLE         g_FontMemHandle   = nullptr; // Dongle (한국어)
HANDLE         g_OswaldMemHandle = nullptr; // Oswald (라틴/키릴)
#endif

// BROKEN_SIGHT 오브 — 맵 위를 랜덤 배회하는 황금 목표물
struct BrokenSightOrb {
    float x = 0, y = 0;
    float vx = 0, vy = 0;
    float wanderTimer = 0.0f;
    bool  active = false;
} g_Orb;

// 다가오는 죽음 (디버프) — 죽지 않고 영원히 추격하는 빨간 사각형 (여러 개 가능)
struct ApproachOrb {
    float x = 0, y = 0;
};
std::vector<ApproachOrb> g_ApproachOrbs;

// 드론 (희귀+) — 플레이어 주위 공전 + 자동 발사. 최대 2기
struct DroneState {
    float angle     = 0.0f;
    float fireTimer = 0.0f;
};
static const int MAX_DRONES = 4;
DroneState g_Drones[MAX_DRONES] = {};

// 포탑 (CANNON + DRONE_2 조합)
//   1초마다 플레이어 위치에 1개씩 배치, 각 5초 지속 → 맵에 ~5개 상시
//   능력치는 '대포'가 아니라 '소총' 기준(g_TurretStats)으로 계산
struct Turret {
    float x = 0.0f, y = 0.0f;
    float lifeTimer = 0.0f;   // 0→TURRET_LIFE
    float fireTimer = 0.0f;
};
static const int MAX_TURRETS = 8;                 // 안전 상한
// 창 크기 — g_Scale 로 시작 시 일괄 축소 가능하도록 런타임 값 (constexpr → 변수)
float TURRET_WIN_W  = 250.0f;
float TURRET_WIN_H  = 250.0f;
// 신규 보스 개인 창 크기 (본체/HP 를 가두는 따라다니는 창)
float RR_WIN_W     = 600.0f;
float POLY_WIN_W   = 840.0f;
float SPAM_WIN_W   = 660.0f;
float KERNEL_WIN_W = 760.0f;   // 커널: 거대 코어 (큰 창)
float FIREWALL_WIN_W = 720.0f; // 방화벽: 본체+회전 보호막
float BOTNET_WIN_W = 880.0f;   // C2_RELAY: 대형 터미널 + 호스트 링
float CENTI_WIN_W = 600.0f;    // FORK.worm: 본체 가짜 창 (PID 체인도 창 안 렌더)
float TOTEM_WIN_W = 720.0f;    // RITE.CORE: 코어 + 기둥 의식 공간
// 봇넷 노드(SPAWNER) 개인 작은 창 — 고정 후 자기 가짜 창을 띄움 (E21)
float SPAWNER_WIN_W = 300.0f;
float DDOS_WIN_W    = 210.0f;
// 원거리 몹 FakeWindow 크기 (렌더/클리핑 공용) — 시작 시 g_Scale 적용
float g_RfwW = 500.0f, g_RfwH = 500.0f;
static constexpr float TURRET_LIFE   = 5.0f;      // 포탑 지속 5초
static constexpr float TURRET_DEPLOY = 1.0f;      // 1초마다 배치 (대포 공속 고정)
std::vector<Turret> g_Turrets;
float       g_TurretDeployTimer = 0.0f;
PlayerStats g_TurretStats;                        // 소총 기준 능력치

// 차크람 (에픽+) — 주변 공전 + 잡몹 즉사 + HP. 최대 3개
struct ChakramState {
    float angle        = 0.0f;
    float hp           = 150.0f;
    float maxHp        = 150.0f;
    bool  alive        = false;
    float respawnTimer = 0.0f;
};
static const int   MAX_CHAKRAMS    = 3;
ChakramState g_Chakrams[MAX_CHAKRAMS] = {};
static const float CHAKRAM_RADIUS = 130.0f;   // 버프: 넓은 공전 (공전체 요격)
static const float CHAKRAM_SIZE   = 30.0f;    // 버프: 큰 칼날

// 탄환 세례 (전설) — 20초 쿨다운
float g_BulletRainTimer = 0.0f;

// 취함 (디버프) — 20초 사이클 중 5초간 랜덤 방향 사격
float g_DrunkCycle  = 0.0f;
bool  g_DrunkActive = false;

// LIGHT_STEP 피격 감지용 이전 HP 기록
float g_PrevHP = 100.0f;

// 초당 EXP 누적용 (디버프 — 다가오는 죽음, 잡몹 가속)
float g_XpTimeAccum = 0.0f;

// 게임 시작 후 경과 시간 (자폭병/보스 등장 타이밍)
float g_GameTime         = 0.0f;
float g_BomberSpawnTimer = 0.0f;

// 폭발 충격파 (자폭병 자폭 / 보스 스폰)
struct ShockWave {
    float x, y;
    float life, maxLife;
    float maxRadius;
    float r, g, b;
    bool  active  = false;
    bool  needsBg = false;  // true → 투명 배경 위에서 어두운 원 배경 그리기
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

// 검객 근접 스윙 잔상 (조준 방향 호)
struct SlashFx {
    float x, y, ang, range, life, maxLife;
    bool  active = false;
};
static const int MAX_SLASH = 8;
SlashFx g_Slashes[MAX_SLASH] = {};

// 궁수 차징 (0~1) — LMB 누른 만큼 충전, 떼면 발사
float g_ArcherCharge = 0.0f;
static const float BOW_CHARGE_TIME = 0.9f;   // 완충까지 초

// 머즐 플래시 (발사 순간 총구 섬광)
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

// 화면 흔들기 (보스 스폰, 충격파)
float g_ShakeTime = 0.0f;
float g_ShakeMag  = 0.0f;

// 보스 보상 — 남은 버프 픽 수 (디버프 페이지 skip)
int  g_BossRewardPicksLeft = 0;
// 보스 스폰 — 일반 보스는 20만점마다, 폴리모프는 50만점 고정(1회)
long long g_NextBossScore  = 50000;    // 첫 보스 5만점(테스터들이 보스를 못 만남) → 이후 +20만
bool      g_PolySpawned    = false;    // 폴리모프(50만 고정) 등장 여부
bool      g_CreativeBossPending = false;  // 크리에이티브: 선택 보스 즉시 스폰 대기
// 글리치 보스 (슬라임과 양자택일로 등장) — 슬라임과 별도 관리
// 리로드 러너 보스 (무기 교체형) — 별도 관리
ReloadRunnerBoss* g_RRBoss = nullptr;
// 폴리모프 보스 (희귀, 폼 변환 + 페이즈2 화면 확장) — 별도 관리
PolymorphBoss* g_PolyBoss = nullptr;
// SPAM.dll 보스 (탄막/불릿헬 — 회전 나선탄 + 방사 버스트) — 별도 관리
SpamBoss* g_SpamBoss = nullptr;
// KERNEL.sys 보스 (고정형 DPS 체크 — 자가붕괴 + 팽창/수축) — 별도 관리
KernelBoss* g_KernelBoss = nullptr;
// FIREWALL.sys 보스 (방어형 — 회전 보호막, 가변속도) — 별도 관리
FirewallBoss* g_FirewallBoss = nullptr;
// C2_RELAY.sys 보스 (C&C 터미널 — 좀비 호스트 릴레이) — 별도 관리
BotnetBoss* g_BotnetBoss = nullptr;
// FORK.worm 보스 (프로세스 체인 — 지그재그 배회 + 벽 돌진) — 별도 관리
CentipedeBoss* g_CentiBoss = nullptr;
TotemBoss* g_TotemBoss = nullptr;
// ── 배드 섹터 사망 잔류물 — 임시 감속 구역(손상 영역). 안에 있으면 이동속도 -10% ──
//   즉시 생기지 않고 ZONE_OPEN(0.7초)에 걸쳐 점점 부식되어 퍼짐(grow factor = age/OPEN).
struct SlowZone { float x, y, w, h, life, maxLife, age; };
std::vector<SlowZone> g_SlowZones;
float g_BadSectorBleed = 0.0f;   // 배드 섹터 구역 안에 있으면 2초로 갱신 → 빠져나와도 출혈 지속
static constexpr float SLOWZONE_OPEN = 0.7f;   // 부식 확산 시간
inline float SlowZoneGrow(const SlowZone& z) {
    float g = z.age / SLOWZONE_OPEN;
    return g < 0.0f ? 0.0f : (g > 1.0f ? 1.0f : g);
}
inline void SpawnBadSectorZone(const Monster* m) {
    if (m->kind != MobKind::BADSECTOR) return;
    float w = 300.0f, h = 300.0f;   // 범위 +25% (240 → 300)
    g_SlowZones.push_back({ m->worldX - w*0.5f, m->worldY - h*0.5f, w, h, 5.0f, 5.0f, 0.0f });
}

// ── 스캔 레이저 (증강) — 주기적 관통 빔 + 페이드 비주얼 ──
struct LaserBeam { float ox, oy, ex, ey, life, maxLife; float width = 1.0f; };
std::vector<LaserBeam> g_LaserBeams;
float          g_LaserTimer = 0.0f;
constexpr float LASER_INT   = 0.85f;  // 발사 주기(초) — 너프: 0.7

// ── 백신 스캔 (증강) — 주기적으로 플레이어 주변에 정화 펄스(범위 일소) ──
//   시각 링은 기존 SpawnShockWave(팽창 링) 재사용.
float          g_NovaTimer  = 0.0f;
constexpr float NOVA_INT    = 2.4f;    // 펄스 주기(초, 중첩 시 단축)
constexpr float NOVA_R      = 240.0f;  // 기본 반경(중첩 시 확대)

// ── 보스 등장 전조(증상) ──────────────────────────────────────
//   보스 스폰을 "결정 → 2.5초 전조(테마 증상 + 경고 배너) → 실제 생성" 으로 분리.
//   전조 동안 게임플레이는 계속(텔레그래프). 만료 시 결정된 보스를 실제로 생성.
float          g_BossWarnTimer = 0.0f;          // >0 이면 전조 진행 중 (남은 시간)
constexpr float BOSS_WARN_DUR  = 2.5f;
int            g_BossWarnPick   = -1;           // 0~8 보스 통일 인덱스 (4=폴리)
const wchar_t* g_BossWarnName   = L"";          // 배너에 띄울 프로세스명
float          g_BossWarnHp      = 0.0f;        // 전조 시작 시 확정한 maxHp (만료 시 생성에 사용)
// 페이즈2 상승엣지 추적 (통일 진입 연출 1회 재생용)
bool g_LeakWasP2 = false, g_LeakWasP3 = false, g_RRWasP2 = false, g_RRWasP3 = false, g_SpamWasP2 = false;
bool g_BotnetWasP2 = false;
// 페이즈2 진입 토스트 ("■ 과부하 — PHASE 2")
float     g_P2ToastTimer = 0.0f;
glm::vec3 g_P2ToastCol   = glm::vec3(1.0f);
int g_PolyPrevForm = -1;   // 폼 변환 감지용 (변할 때 파티클) — -1 = 미초기화
float g_PolySummonTimer = 0.0f;   // 2페이즈: 7초마다 주변에 원거리/자폭병 5마리
bool  g_PolyWasPhase2   = false;  // 2페이즈 진입 연출 1회용
bool  g_LastRunRecord   = false;  // 직전 판이 신기록이었는지 (GAMEOVER 표시용)
int   g_MetaStartAugs   = 0;      // 메타 해금: 시작 무료 증강 픽 횟수

// ── 피격 시 창(시야) 축소 기믹 ── 체력 비율에 따라 창 크기 변동 + 피격 펀치
float g_WindowSizeCur = 0.0f;     // 현재 애니메이션 창 크기 (0 = 미초기화)
float g_WinPrevHP     = -1.0f;    // 창 축소용 HP 추적
float g_HurtVignette  = 0.0f;     // 피격 빨간 비네트 잔여
float g_HpBarPop      = 0.0f;     // 산나비식 HP 게이지바 — 피격 시 떴다가 페이드(초)

// ── 액티브 스킬 시스템 — 대시(기본) + 슬롯 3개(증강 획득, 꽉 차면 교체) ──
float g_DashCd        = 0.0f;      // 대시 쿨다운
float g_DashInvuln    = 0.0f;      // 대시 무적 잔여
float g_PostPickGrace = 0.0f;      // C14: 증강 픽 후 짧은 유예(무적+발사억제)로 복귀 텀
float g_TimeStopTimer = 0.0f;      // >0 = 적·적탄 정지 중
float g_OverclockTimer= 0.0f;      // >0 = 연사/공격력 버프 중

static constexpr float DASH_CD = 3.5f, DASH_DIST = 300.0f, DASH_INVULN = 0.25f;
static constexpr float TIMESTOP_DUR = 1.5f, OVERCLOCK_DUR = 5.0f;

static float SkillCooldownMax(SkillType t) {
    switch (t) {
    case SkillType::CLOSE_WINDOW: return 16.0f;
    case SkillType::OVERCLOCK:    return 20.0f;
    case SkillType::TIME_STOP:    return 28.0f;
    default:                      return 0.0f;
    }
}
static void ResetSkills() {
    for (int i = 0; i < 3; i++) g_Skills[i] = { SkillType::NONE, 0.0f };
    g_SkillReplaceIdx = 0; g_DashCd = 0; g_DashInvuln = 0;
    g_TimeStopTimer = 0; g_OverclockTimer = 0;
}
// 보스 생존 동안 화면 전체를 보스 고유색으로 점점 물들이는 연출
float     g_BossTintT   = 0.0f;                     // 0..1 (생존 시 상승, 사망 시 하강)
glm::vec3 g_BossTintCol = glm::vec3(0.6f, 0.3f, 1.0f);

// 슬라임 분열체 (원본 사망 시 2마리 → 각자 또 1번 분열, 총 2세대) — main 이 직접 관리
std::vector<Boss*> g_LeakNodes;
bool g_LeakEncounter = false;   // 분열 인카운터 진행 중 (끝나면 보상)
// 누수 위성 노드 — 이동 궤적만 (ALLOC/DRIP 없음)
static Boss* MakeLeakNode(float x, float y, float maxHp, float scale, int gen,
                          int sw, int sh) {
    Boss* c = new Boss(x, y, sw, sh, maxHp);
    c->sizeScale  = scale;
    c->splitGen   = gen;
    c->leakNode   = true;
    c->color = glm::vec3(0.35f, 0.72f, 1.0f);
    return c;
}

// HUD: 현재 측정 FPS (상단 우측 표시)
int    g_CurrentFPS  = 0;
double g_FpsLastTime = 0.0;
int    g_FpsFrames   = 0;

// 보유 증강 인덱스 목록 (선택 순서대로, 중복 스택 가능)
std::vector<int> g_OwnedAugs;

// 크리에이티브 — 시작 증강 직접 선택 (인덱스). 게임 시작 시 일괄 적용.
std::vector<int> g_CreativeStartAugList;
bool g_CreativeStartPending = false;   // 적용 대기 (main 루프가 applyByIdx 로 처리)

// 증강 선택 hover state (-1 = 미선택, 0/1/2 = 카드 인덱스)
int  g_HoveredAug    = -1;
bool g_EnterReleased = true;

// 마우스 클릭 edge 감지 (이전 프레임 left button 상태)
bool g_LmbPrev = false;

// PAUSED 상태 — 보유 증강 클릭 시 설명 표시 (-1 = 없음, 0..AUG_TOTAL-1 = 인덱스)
int g_PauseSelectedAug = -1;

// 설정 화면 진입 시 이전 상태 (뒤로 가기 시 복귀)
GameState g_SettingsReturnTo = GameState::MAIN_MENU;

// 시작 무기 선택 화면 — 6 중 랜덤 3 인덱스
int g_WeaponChoices[3] = {0, 1, 2};

// 현재 보유 중인 시작 무기 (StartWeapon 인덱스). -1 = 아직 선택 안 함
int g_CurrentWeapon = -1;

// 변환 카드 — AUG_SELECT 시 25% 확률로 4번째 카드 등장
// 현재 무기를 다른 StartWeapon 으로 전환 (기존 무기 효과 제거 후 새 무기 적용)
// 값 = 전환할 StartWeapon 인덱스. -1 = 이번 라운드는 변환 카드 없음
int g_ConversionWeapon = -1;

// DYING 사망 연출 상태
float g_DyingTimer    = 0.0f;
bool  g_DeathBoomDone = false;   // 창 수축 후 대폭발 1회 트리거
float g_DeathWinW0    = 0.0f;    // 사망 시점 플레이어 창 크기 (수축 기준)
float g_GameOverFade  = 0.0f;    // 게임오버 메뉴 페이드인 (0→1, 풀스크린 후 2.5초)
// 사망 연출 = 즉시 대폭발(파티클) → 폭발 여파 → GAMEOVER (줌/시네마틱 없음, 단순)
static const float DYING_DUR      = 0.8f;   // 폭발 여파가 재생되는 시간 (페이드 전까지)
static const float GAMEOVER_FADE  = 2.5f;   // 메뉴 100% 까지 걸리는 시간

// 사망 파편 파티클
struct DeathParticle {
    float x, y, vx, vy;
    float size;       // 한 변 길이(px)
    float r, g, b;    // 색상
    bool  active = false;
};
static const int MAX_DEBRIS = 48;
DeathParticle g_Debris[MAX_DEBRIS] = {};
float g_DeathCX = 0, g_DeathCY = 0;
float g_DeathFlash = 0.0f; // 폭발 섬광 (1.0 → 0.0)
wchar_t g_DeathReason[96] = {0};   // 사망 원인 ("○○ 에 의해 종료됨")

int main() {
    CrashHandler::Install();   // 강종(E23) 추적 — 처리 안 된 예외 시 로그+미니덤프
    srand((unsigned)time(NULL));

    // 실행 파일 폴더로 작업 디렉터리 이동 (Resource/ 상대경로 로드 보장)
    //   Windows 는 임베디드 폰트라 no-op, macOS/Linux 는 더블클릭 실행 대응
    PlatformChdirToExeDir();
    LoadGame();   // 저장된 설정/기록 불러오기 (없으면 기본값 유지)
    ApplyAccentTheme();   // 저장된 액센트 테마 → g_Accent* 반영

    if (!glfwInit()) return -1;
    // Sleep 해상도 1ms 로 (FPS 캡 정밀도용). Windows 만 의미 있음
    PlatformTimerBegin();

    GLFWmonitor*       monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode    = glfwGetVideoMode(monitor);
    screenWidth  = mode->width;
    screenHeight = mode->height - 1; // ★ DirectFlip 회피: 화면보다 1px 작게

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,        GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    // macOS 는 3.2+ Core 에서 forward-compatible 컨텍스트 필수
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    // 레티나(HiDPI) 2× 백버퍼 끄기 → 프레임버퍼 = 창 크기(점) 1:1
    //   좌표/스크issor/마우스가 전부 점 단위로 일치 → Windows 와 동일하게 동작
    //   (안 끄면 게임이 화면 좌하단 1/4 에만 그려지고 클릭 위치가 어긋남)
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
#endif
    glfwWindowHint(GLFW_DECORATED,             GLFW_FALSE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    // GLFW_FLOATING 제거: WS_EX_TOPMOST + 전체화면 크기 조합이 DWM 플립 모드를 유발
    // → DWM 컴포지팅 우회 → 알파 투명도 무효화. TOPMOST 없이 생성 후 수동으로 설정.
    glfwWindowHint(GLFW_RESIZABLE,             GLFW_FALSE);
    glfwWindowHint(GLFW_ALPHA_BITS,            8);

    // ★ screenHeight 가 이미 mode->height-1 (위에서 DirectFlip 회피용)
    //   화면 정확히 같은 크기로 생성하면 DWM 이 DirectFlip 으로 컴포지팅 우회
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight,
                                          "Onedow", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }

    glfwSetWindowPos(window, 0, 0);

    // 하단 작업표시줄 높이 계산 — 풀스크린 위에 떠 있는 작업표시줄에 하단 UI 가
    //   가려지지 않도록. (작업 영역이 화면보다 작으면 그 차이가 작업표시줄 높이)
#ifdef _WIN32
    {
        int fullH = GetSystemMetrics(SM_CYSCREEN);
        RECT wa;
        if (SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0)) {
            int bottomGap = fullH - (int)wa.bottom;   // 하단 작업표시줄 높이 (그 외 위치면 0)
            if (bottomGap > 0 && bottomGap < 120) g_TaskbarH = bottomGap;
        }
    }
#endif

    // 해상도 기준 스케일 — 작은 화면에서 창/엔티티가 비례 축소되도록 계산 후 일괄 적용.
    //   (절대 픽셀이라 작은 화면일수록 상대적으로 컸던 문제 해결 + 전체적으로 창 축소)
    g_Scale = (float)screenHeight / SCALE_REF_H;
    if (g_Scale > 1.0f) g_Scale = 1.0f;
    if (g_Scale < 0.5f) g_Scale = 0.5f;
    Boss::WIN_W *= g_Scale; Boss::WIN_H *= g_Scale; Boss::BODY_SIZE *= g_Scale;
    TURRET_WIN_W *= g_Scale; TURRET_WIN_H *= g_Scale;
    RR_WIN_W *= g_Scale; POLY_WIN_W *= g_Scale; SPAM_WIN_W *= g_Scale;
    KERNEL_WIN_W *= g_Scale; FIREWALL_WIN_W *= g_Scale; BOTNET_WIN_W *= g_Scale;
    CENTI_WIN_W *= g_Scale;
    TOTEM_WIN_W *= g_Scale;
    SPAWNER_WIN_W *= g_Scale;
    DDOS_WIN_W    *= g_Scale;
    g_RfwW *= g_Scale; g_RfwH *= g_Scale;
    glfwMakeContextCurrent(window);
    InputRegisterCallbacks(window);
    // g_FpsCap == 0 : VSync, 그 외 : VSync 끄고 수동 캡
    glfwSwapInterval((g_FpsCap == 0) ? 1 : 0);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    // 폰트 초기화 — Jua(한글)/KosugiMaru(일본어)/Oswald(라틴) 폴백 체인.
    //   세 폰트를 항상 동시 로드 → 언어와 무관하게 모든 글리프(한/일/영) 표시.
#ifdef _WIN32
    // EXE 임베디드 RCDATA 바이트를 stb_truetype 으로 직접 래스터화
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
    g_TextXL.InitFromMemory(datas, sizes, 3, 100, screenWidth, screenHeight);  // 로고 고해상도
#else
    // macOS/Linux: 디스크의 TTF 폴백 체인
    {
        const char* chain[3];
        int nc = LanguageFontChain(g_Language, chain);
        g_TextL.InitFromFiles(chain, nc, 36, screenWidth, screenHeight);
        g_TextS.InitFromFiles(chain, nc, 22, screenWidth, screenHeight);
        g_TextXL.InitFromFiles(chain, nc, 100, screenWidth, screenHeight);  // 로고 고해상도
    }
#endif

    BatchFlush(); glEnable(GL_BLEND);
    // RGB: 표준 알파블렌딩 / Alpha: 프레임버퍼 알파값 올바르게 누적
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                        GL_ONE,       GL_ONE_MINUS_SRC_ALPHA);

    // 아이콘(픽토그램) 텍스처 파이프라인 + 증강 아이콘 로드
    InitIconGL();
    LoadIcons();
    InitRainMissileTex();   // 탄환 세례 로켓 스프라이트(흰배경 제거+틴트 가능)

    EnableWindowTransparency(window);

    // --- 창/GL/레지스트리 종합 진단 (Windows 전용) ---
#ifdef _WIN32
    {
        HWND hWnd = glfwGetWin32Window(window);
        // GL 3.3 Core Profile: GL_ALPHA_BITS deprecated → glGetFramebufferAttachmentParameteriv 사용
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
        // AMD/ATI 감지 — AMD 드라이버는 GLSL 컴파일이 더 엄격하고 투명 FBO 처리가
        //   NVIDIA 와 달라, 벤더를 로그로 남겨 검은화면/투명실패 원인 추적에 사용.
        bool isAMD = false;
        if (vendor) {
            std::string vlow = vendor;
            for (auto& ch : vlow) ch = (char)tolower((unsigned char)ch);
            isAMD = (vlow.find("amd") != std::string::npos) ||
                    (vlow.find("ati") != std::string::npos) ||
                    (vlow.find("advanced micro") != std::string::npos);
        }

        // Windows 투명도 효과 설정 (레지스트리)
        DWORD enableTrans = 1; // 기본값: 켜진 것으로 가정
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
            isAMD ? "  (AMD 감지 — 엄격 GLSL/투명 FBO 경로)" : "");
        DBG("  [GL] Renderer         : %s\n", renderer ? renderer : "NULL");
        DBG("  [GL] Version          : %s\n", glVer    ? glVer    : "NULL");
        DBG("  [GL] Framebuffer bits : R=%d G=%d B=%d A=%d\n",
            rBits, gBits, bBits, alphaBits);
        DBG("       → (Core Profile 정확한 쿼리. A=0이면 드라이버가 알파 채널 거부)\n");
        DBG("\n");
        DBG("  [GLFW] transparent_framebuffer : %d [%s]\n",
            trans, trans ? "SUPPORTED" : "NOT SUPPORTED");
        DBG("  [DWM]  Composition enabled     : %s\n", dwmOn ? "YES" : "NO");
        DBG("  [Win]  GWL_EXSTYLE             : 0x%08lX\n", ex);
        DBG("  [Win]  WS_EX_LAYERED           : %s\n",
            (ex & WS_EX_LAYERED)     ? "SET"           : "NOT SET");
        DBG("  [Win]  WS_EX_TRANSPARENT       : %s\n",
            (ex & WS_EX_TRANSPARENT) ? "SET (클릭통과!)" : "NOT SET (정상)");
        DBG("\n");
        DBG("  [REG]  EnableTransparency      : %lu [%s]\n",
            enableTrans,
            enableTrans ? "ON (정상)"
                        : "OFF ← 이게 문제! 설정>개인설정>색>투명도 효과 켜기");
        DBG("=================\n\n");
    }
#endif // _WIN32 (진단 블록)

    InitMainShaderPipeline(screenWidth, screenHeight);
    InitMainBatchGeometry(screenWidth, screenHeight);

    // --- 게임 오브젝트 초기화 ---
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

    Audio::Init();   // 사운드 시스템 (Sounds/ 폴더, 파일 없으면 무음)

    // 최근접 적 — 자동조준·유도탄(탄환세례)·드론·포탑 공용
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
        if (g_MonsterManager.boss && g_MonsterManager.boss->alive)
            consider(g_MonsterManager.boss->worldX, g_MonsterManager.boss->worldY);
        for (auto* c : g_LeakNodes) if (c->alive) consider(c->worldX, c->worldY);
        if (g_RRBoss && g_RRBoss->alive)         consider(g_RRBoss->worldX, g_RRBoss->worldY);
        if (g_PolyBoss && g_PolyBoss->alive && g_PolyBoss->damageable())
            consider(g_PolyBoss->worldX, g_PolyBoss->worldY);
        if (g_SpamBoss && g_SpamBoss->alive)     consider(g_SpamBoss->worldX, g_SpamBoss->worldY);
        if (g_KernelBoss && g_KernelBoss->alive) consider(g_KernelBoss->worldX, g_KernelBoss->worldY);
        if (g_FirewallBoss && g_FirewallBoss->alive) consider(g_FirewallBoss->worldX, g_FirewallBoss->worldY);
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
        return found;
    };

    // ============================================================
    // 메인 루프
    // ============================================================
    while (!glfwWindowShouldClose(window)) {
        float now   = (float)glfwGetTime();
        float delta = now - lastFrame;
        if (delta > 0.1f) delta = 0.1f; // 스파이크 클램프
        lastFrame = now;

        // 크래시 추적 브레드크럼 — 마지막 상태를 남겨 강종(E23) 시 로그로 위치 특정
        {
            const char* bn = g_MonsterManager.boss ? "swordsman" :
                             g_RRBoss      ? "reload"  :
                             g_PolyBoss     ? "poly"     : g_SpamBoss    ? "spam"    :
                             g_KernelBoss   ? "kernel"   : g_FirewallBoss? "firewall":
                             g_BotnetBoss   ? "botnet"   : g_CentiBoss   ? "centi"   :
                             g_TotemBoss    ? "totem"   : "none";
            char bc[200];
            std::snprintf(bc, sizeof(bc),
                "st=%d score=%lld lv=%d mobs=%u boss=%s",
                (int)g_GameManager.currentState,
                (long long)g_GameManager.score,
                g_GameManager.playerLevel,
                (unsigned)g_MonsterManager.monsters.size(), bn);
            CrashHandler::SetBreadcrumb(bc);
        }

        // 창 포커스를 잃으면(다른 앱으로 전환) 자동 일시정지 —
        //   오버레이라 포커스 없어도 루프가 계속 도므로, 안 막으면 게임이
        //   백그라운드에서 계속 진행됨(콤보 3초 창이 흘러가 리셋되는 버그 등).
        {
            static bool s_wasFocused = true;
            bool focused = glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0;
            if (!focused && s_wasFocused &&
                g_GameManager.currentState == GameState::RUNNING)
                g_GameManager.currentState = GameState::PAUSED;
            s_wasFocused = focused;
        }

        // FPS 측정 (1초 단위)
        ++g_FpsFrames;
        if (now - g_FpsLastTime >= 1.0f) {
            g_CurrentFPS  = g_FpsFrames;
            g_FpsFrames   = 0;
            g_FpsLastTime = now;
        }

        glfwPollEvents();

        // 줌 부드럽게 보간 (페이즈2 화면 확장 등)
        //   전투/전투오버레이(RUNNING/DYING/PAUSED/증강·디버프 선택)에선 줌 유지 —
        //   일시정지 시 줌인되는 게 부자연스럽다는 피드백. 줌 해제는 시작창 등
        //   '진짜 메뉴'로 나갈 때만(보스 처치 시엔 polyDeath 가 target=1 로 부드럽게 복원).
        {
            GameState zs = g_GameManager.currentState;
            bool inFight = (zs == GameState::RUNNING || zs == GameState::DYING ||
                            zs == GameState::PAUSED  || zs == GameState::AUG_SELECT ||
                            zs == GameState::DEBUFF_SELECT);
            if (!inFight) { g_ViewZoom = g_ViewZoomTarget = 1.0f; }
            else {
                // 점수 비례 줌아웃 — 전장이 서서히 넓어짐 (상한 1.4배 = zoom 0.714).
                //   200만점에서 최대치 도달. 폴리모프 페이즈2(0.5)면 그쪽이 우선(min).
                float t = std::min(1.0f, (float)g_GameManager.score / 2000000.0f);
                float scoreZoom = 1.0f - 0.286f * t;   // 1.0 → 0.714
                float polyZoom  = (g_PolyBoss && g_PolyBoss->alive && g_PolyBoss->phase2) ? 0.5f : 1.0f;
                g_ViewZoomTarget = std::min(scoreZoom, polyZoom);
            }
        }
        g_ViewZoom += (g_ViewZoomTarget - g_ViewZoom) * std::min(1.0f, delta * 4.0f);

        // 확장 아레나 — 줌아웃된 만큼 보이는 영역이 넓어지므로 엔티티 배회/스폰 영역도 확장
        //   (점수 줌아웃·폴리모프 줌아웃 공통 처리)
        {
            float zb = (g_ViewZoom < 0.01f) ? 0.01f : g_ViewZoom;
            g_ArenaExX = (float)screenWidth  * 0.5f * (1.0f / zb - 1.0f);
            g_ArenaExY = (float)screenHeight * 0.5f * (1.0f / zb - 1.0f);
        }

        // 마우스 상태 (mx,my = 화면 픽셀 / wmx,wmy = 줌 보정한 월드 좌표 = 조준용)
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        bool lmb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        float wmx = ScreenToWorldX((float)mx);
        float wmy = ScreenToWorldY((float)my);

        // --- 입력 처리 ---
        GameState prevState = g_GameManager.currentState;
        g_GameManager.HandleInput(window);

        // 새 게임 리셋 람다 (GAMEOVER → READY, 난이도 선택 후, 다시하기 버튼 등에서 호출)
        auto ResetForNewGame = [&]() {
            g_Stats        = PlayerStats();
            g_MetaStartAugs = ApplyMeta(g_Stats);    // 메타 영구 업그레이드 적용
            g_Stats.windowSize *= g_Scale;           // 플레이어 창 — 해상도 비례 축소
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
            memset(g_TypeOwned, 0, sizeof(g_TypeOwned));  // 조합 레시피 보유 초기화
            g_CreativeGodmode  = false;   // 무적 토글 초기화
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
            g_NextBossScore = 50000;   // 첫 보스 5만점
            g_PolySpawned   = false;
            BossDir::ResetRotation();
            g_BossRewardPicksLeft = 0;
            g_BossWarnTimer = 0.0f; g_BossWarnPick = -1;   // 보스 전조 초기화
            g_LeakWasP2 = g_LeakWasP3 = g_RRWasP2 = g_RRWasP3 = g_SpamWasP2 = g_BotnetWasP2 = false;
            g_LaserBeams.clear(); g_LaserTimer = 0.0f;     // 스캔 레이저 초기화
            g_SlowZones.clear(); g_BadSectorBleed = 0.0f;   // 배드 섹터 감속 구역/출혈 초기화
            g_NovaTimer = 0.0f;                            // 백신 스캔 초기화
            g_RunMelee = false; g_RunBow = false;          // 클래스 게이팅 초기화
            if (g_RRBoss)     { delete g_RRBoss;     g_RRBoss     = nullptr; }
            if (g_PolyBoss)   { delete g_PolyBoss;   g_PolyBoss   = nullptr; }
            if (g_SpamBoss)   { delete g_SpamBoss;   g_SpamBoss   = nullptr; }
            if (g_KernelBoss) { delete g_KernelBoss; g_KernelBoss = nullptr; }
            if (g_FirewallBoss) { delete g_FirewallBoss; g_FirewallBoss = nullptr; }
            if (g_BotnetBoss) { delete g_BotnetBoss; g_BotnetBoss = nullptr; }
            if (g_CentiBoss) { delete g_CentiBoss; g_CentiBoss = nullptr; }
            if (g_TotemBoss) { delete g_TotemBoss; g_TotemBoss = nullptr; }
            g_PolyPrevForm = -1;
            g_PolySummonTimer = 0.0f;
            g_PolyWasPhase2 = false;
            for (auto* c : g_LeakNodes) delete c;   // 분열체 정리
            g_LeakNodes.clear();
            g_LeakEncounter = false;
            g_BossTintT = 0.0f;
            ResetJuice();                            // 데미지숫자/콤보/플래시/히트스톱/적 폭발·처치태그 초기화
            ResetSkills();                           // 액티브 스킬/대시 초기화
            g_WindowSizeCur = 0.0f; g_WinPrevHP = -1.0f; g_HurtVignette = 0.0f; g_HpBarPop = 0.0f; // 창 시야 기믹
            g_ViewZoom = g_ViewZoomTarget = 1.0f;   // 줌 원복
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
            // GameManager 필드도 직접 리셋
            g_GameManager.playerHP    = 100.0f;
            g_GameManager.maxHP       = 100.0f;
            g_GameManager.score       = 0;
            g_GameManager.scoreAccum  = 0.0f;
            if (g_CreativeMode) {
                g_GameManager.score      = g_CreativeStartScore;
                g_GameManager.scoreAccum = (float)g_CreativeStartScore;
                // 시작 점수보다 위 첫 20만 배수부터 일반 보스 (한꺼번에 쏟아짐 방지)
                g_NextBossScore = ((g_CreativeStartScore / 200000) + 1) * 200000;
                // 이미 50만 이상에서 시작하면 폴리모프 자동등장 생략 (보스선택으로 소환)
                g_PolySpawned   = (g_CreativeStartScore >= 500000);
                // 선택 보스 즉시 스폰 예약 (점수 무관)
                g_CreativeBossPending = (g_CreativeBossPick >= 0);
            }
            g_GameManager.xp          = 0;
            g_GameManager.playerLevel = 1;
            memset(g_GameManager.takenOnce, 0, sizeof(g_GameManager.takenOnce));
            g_Bullets.clear();
            g_MonsterManager.Clear();
            // UpdateStateSystem 의 중복 reset 회피
            g_GameManager.lastState = GameState::READY;
        };

        // GAMEOVER→READY 자동 감지 (ESC 등으로 직접 전환된 경우)
        if (prevState == GameState::GAMEOVER &&
            g_GameManager.currentState == GameState::READY) {
            ResetForNewGame();
        }
        // PAUSED → 다른 상태: 증강 설명 박스 닫기
        if (prevState == GameState::PAUSED &&
            g_GameManager.currentState != GameState::PAUSED) {
            g_PauseSelectedAug = -1;
        }
        g_GameManager.UpdateStateSystem(g_MonsterManager, g_Bullets);

        // ── BGM — 게임플레이 중엔 메인 루프, 메뉴에선 정지 (보스 BGM 은 파일 생기면 확장) ──
        {
            Audio::SetEnabled(g_SoundVol > 0);            // 0 = 끄기
            Audio::SetVolume(g_SoundVol / 100.0f);        // 마스터 볼륨
            GameState cs = g_GameManager.currentState;
            bool bgmOn = (cs == GameState::RUNNING || cs == GameState::PAUSED ||
                          cs == GameState::AUG_SELECT || cs == GameState::DEBUFF_SELECT ||
                          cs == GameState::READY);
            (void)bgmOn; Audio::StopBgm();   // BGM 험("우우웅") 제거 — 항상 정지
        }

        // 크리에이티브 무적 — 매 프레임 체력 풀 고정 (절대 죽지 않음)
        if (g_CreativeGodmode && g_CreativeMode &&
            g_GameManager.currentState == GameState::RUNNING) {
            g_GameManager.playerHP = g_Stats.maxHP;
        }

        // HP 0 → MK2 부활 OR DYING (1초 슬로우 모션 → GAMEOVER)
        if (g_GameManager.playerHP <= 0.0f &&
            g_GameManager.currentState == GameState::RUNNING) {
            // MK2: 1회 부활 — 공격력 비례 폭발 + 풀 HP (페널티 없음)
            if (g_Stats.mk2 && !g_Stats.mk2Used) {
                g_Stats.mk2Used = true;
                float pCX = playerWin.x + playerWin.width  * 0.5f;
                float pCY = playerWin.y + playerWin.height * 0.5f;
                float blastDmg = g_Stats.GetBaseDamage()
                               * g_Stats.GetDamageMultiplier(0.0f) * 6.0f;
                float blastRad = 280.0f;
                // 주변 적에게 데미지
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
                if (g_MonsterManager.boss && g_MonsterManager.boss->alive) {
                    auto* bs = g_MonsterManager.boss;
                    float dx = bs->worldX - pCX, dy = bs->worldY - pCY;
                    if (dx*dx + dy*dy < blastRad * blastRad) {
                        bs->hp -= blastDmg;
                        if (bs->hp <= 0) bs->alive = false;
                    }
                }
                // 시각 효과 — 큰 폭발 + 충격파 + 화면 흔들기
                SpawnEnemyExplosion(pCX, pCY, 1.0f, 0.8f, 0.3f, true);
                SpawnEnemyExplosion(pCX, pCY, 0.4f, 1.0f, 0.8f, true);
                SpawnShockWave(pCX, pCY, blastRad * 1.4f, 0.55f,
                               1.0f, 0.85f, 0.3f);
                g_ShakeTime = 0.45f; g_ShakeMag = 22.0f;
                TriggerFlash(1.0f, 0.9f, 0.4f, 0.8f);   // 부활 — 강한 번쩍
                TriggerHitStop(0.14f);                  // 임팩트 정지

                // 부활 — 능력치 페널티 없이 풀 HP 로 (디버프 없음)
                g_GameManager.maxHP    = g_Stats.maxHP;
                g_GameManager.playerHP = g_Stats.maxHP;
                g_PrevHP               = g_GameManager.playerHP;
                // RUNNING 상태 유지 (DYING 전이 X)
            } else {
                // 일반 사망 — 플레이어 기점 대폭발(모든 적 터짐) 후 메뉴 페이드인
                g_GameManager.currentState = GameState::DYING;
                g_GameManager.playerHP     = 0.0f;
                Audio::PlaySfx(Audio::Sfx::Death);   // 플레이어 사망음
                Audio::StopBgm();
                float pCX = playerWin.x + playerWin.width  * 0.5f;
                float pCY = playerWin.y + playerWin.height * 0.5f;
                g_DeathCX = pCX; g_DeathCY = pCY;
                g_DyingTimer    = DYING_DUR;
                g_DeathBoomDone = false;
                g_DeathFlash    = 0.0f;
                // 폴리모프 등 줌 원복 (사망 연출엔 줌 없음)
                g_ZoomCX = g_ZoomCY = 0.0f;
                g_ViewZoom = 1.0f; g_ViewZoomTarget = 1.0f;
                // 사망 원인 — 엔티티 삭제 전, 가장 가까운 위협(프로세스)을 기록 (결과창에 표시용)
                {
                    float best = 1e18f; const wchar_t* nm = nullptr;
                    auto consider = [&](float ex, float ey, const wchar_t* n) {
                        float dx = ex-pCX, dy = ey-pCY, d = dx*dx+dy*dy;
                        if (d < best) { best = d; nm = n; }
                    };
                    for (auto m  : g_MonsterManager.monsters)   if (m->alive)  consider(m->worldX,  m->worldY,  MobName((int)m->kind));
                    for (auto r  : g_MonsterManager.rangedMobs) if (r->alive)  consider(r->worldX,  r->worldY,  MobName(CM_RANGED));
                    for (auto bm : g_MonsterManager.bombers)    if (bm->alive) consider(bm->worldX, bm->worldY, MobName(CM_BOMBER));
                    if (g_MonsterManager.boss && g_MonsterManager.boss->alive) consider(g_MonsterManager.boss->worldX, g_MonsterManager.boss->worldY, L"LEAK.sys");
                    if (g_RRBoss     && g_RRBoss->alive)     consider(g_RRBoss->worldX,     g_RRBoss->worldY,     L"VOLLEY.sys");
                    if (g_PolyBoss   && g_PolyBoss->alive)   consider(g_PolyBoss->worldX,   g_PolyBoss->worldY,   L"POLYMORPH.vir");
                    if (g_SpamBoss   && g_SpamBoss->alive)   consider(g_SpamBoss->worldX,   g_SpamBoss->worldY,   L"SPAM.dll");
                    if (g_KernelBoss && g_KernelBoss->alive) consider(g_KernelBoss->worldX, g_KernelBoss->worldY, L"KERNEL.sys");
                    if (g_FirewallBoss && g_FirewallBoss->alive) consider(g_FirewallBoss->worldX, g_FirewallBoss->worldY, L"FIREWALL.sys");
                    if (g_BotnetBoss && g_BotnetBoss->alive) consider(g_BotnetBoss->worldX, g_BotnetBoss->worldY, L"C2_RELAY.sys");
                    if (g_CentiBoss && g_CentiBoss->alive) consider(g_CentiBoss->worldX, g_CentiBoss->worldY, L"FORK.worm");
                    if (g_TotemBoss && g_TotemBoss->alive) consider(g_TotemBoss->worldX, g_TotemBoss->worldY, L"RITE.CORE");
                    int li = LangIndex();
                    const wchar_t* FMT[3] = { L"%ls 에 의해 종료됨", L"Terminated by %ls", L"%ls により終了" };
                    const wchar_t* UNK[3] = { L"알 수 없는 오류로 종료됨", L"Terminated by unknown error", L"不明なエラーで終了" };
                    if (nm) swprintf_s(g_DeathReason, FMT[li], nm);
                    else    wcscpy_s(g_DeathReason, UNK[li]);
                }
            }   // close else (MK2 분기 외)
        }       // close outer if (HP <= 0)

        // DYING 사망 연출 — 플레이어 기점 대폭발(모든 적 터짐) → GAMEOVER (창 변형 없음)
        if (g_GameManager.currentState == GameState::DYING) {
            g_DyingTimer -= delta;
            g_DeathFlash -= delta * 4.0f;
            if (g_DeathFlash < 0.0f) g_DeathFlash = 0.0f;

            // 대폭발 — 플레이어 기점, 모든 적이 터짐 (즉시, 줌/시네마틱 없음)
            if (!g_DeathBoomDone) {
                g_DeathBoomDone = true;
                float pCX = g_DeathCX, pCY = g_DeathCY;
                // 모든 적 폭발시키며 제거 (scored/noBlast 표시 → 점수/연쇄 정산 안 함)
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
                // 모든 적·보스·분열체·총알·포탑을 즉시 완전 삭제 — UpdateAll(RUNNING 전용)에
                //   맡기면 DYING/GAMEOVER 동안 원거리 몹 창 등이 정지 상태로 남으므로 여기서 제거.
                g_MonsterManager.Clear();   // monsters/ranged/bombers/boss 전부 delete + clear
                g_Bullets.clear();
                if (g_RRBoss)     { delete g_RRBoss;     g_RRBoss     = nullptr; }
                if (g_PolyBoss)   { delete g_PolyBoss;   g_PolyBoss   = nullptr; }
                if (g_SpamBoss)   { delete g_SpamBoss;   g_SpamBoss   = nullptr; }
                if (g_KernelBoss) { delete g_KernelBoss; g_KernelBoss = nullptr; }
                if (g_FirewallBoss) { delete g_FirewallBoss; g_FirewallBoss = nullptr; }
                if (g_BotnetBoss) { delete g_BotnetBoss; g_BotnetBoss = nullptr; }
                if (g_CentiBoss) { delete g_CentiBoss; g_CentiBoss = nullptr; }
            if (g_TotemBoss) { delete g_TotemBoss; g_TotemBoss = nullptr; }
                for (auto* c : g_LeakNodes) delete c;
                g_LeakNodes.clear();
                g_Turrets.clear();
                g_PolyWasPhase2  = false;
                g_BossWarnTimer  = 0.0f; g_BossWarnPick = -1;   // 사망 시 대기 중 전조 취소
                g_LeakWasP2 = g_LeakWasP3 = g_RRWasP2 = g_RRWasP3 = g_SpamWasP2 = g_BotnetWasP2 = false;
                g_LaserBeams.clear();   // 스캔 레이저 빔 정리
                g_SlowZones.clear(); g_BadSectorBleed = 0.0f;   // 배드 섹터 감속 구역/출혈 정리
                g_NovaTimer = 0.0f;   // 백신 스캔 정리
                // 플레이어 중심 대폭발 + 충격파 + 섬광 + 흔들기 + 방사형 파편
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

            // 파편 이동 — 폭발과 함께 실시간으로 날아감
            float sd = delta;
            for (int i = 0; i < MAX_DEBRIS; i++) {
                if (!g_Debris[i].active) continue;
                g_Debris[i].x  += g_Debris[i].vx * sd;
                g_Debris[i].y  += g_Debris[i].vy * sd;
                g_Debris[i].vx *= (1.0f - 1.6f * sd);
                g_Debris[i].vy *= (1.0f - 1.6f * sd);
            }
            if (g_DyingTimer <= 0.0f) {
                g_ViewZoom = g_ViewZoomTarget = 1.0f;   // 줌 원복 (메뉴 정상화)
                g_ZoomCX = g_ZoomCY = 0.0f;             // 줌 중심 화면 중앙으로
                g_GameManager.currentState = GameState::GAMEOVER;
                g_DyingTimer = 0.0f;
                g_GameOverFade = 0.0f;   // 결과 메뉴 페이드인 시작 (2.5초)
                // 기록 저장 — 난이도별 최고점/누적/코인 갱신 (신기록이면 표시)
                //   크리에이티브 모드(샌드박스)는 코인·기록 제외 (파밍 방지)
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

        // GAMEOVER 메뉴 페이드인 — 폭발 후 0→1 까지 2.5초에 걸쳐 차오름
        if (g_GameManager.currentState == GameState::GAMEOVER && g_GameOverFade < 1.0f) {
            g_GameOverFade += delta / GAMEOVER_FADE;
            if (g_GameOverFade > 1.0f) g_GameOverFade = 1.0f;
        }

        // 사망 폭발 잔여물(파티클·파편·충격파·스파크)이 게임오버 화면에 굳어버리지 않도록
        //   DYING/GAMEOVER 동안에도 계속 갱신해 자연스럽게 사라지게 한다.
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
                // 파편은 DYING 블록이 이미 갱신 → GAMEOVER 에서만 추가로 굴린다
                if (gvs == GameState::GAMEOVER) {
                    for (int i = 0; i < MAX_DEBRIS; i++) { if (!g_Debris[i].active) continue;
                        g_Debris[i].x += g_Debris[i].vx * delta; g_Debris[i].y += g_Debris[i].vy * delta;
                        g_Debris[i].vx *= (1.0f - 1.6f * delta); g_Debris[i].vy *= (1.0f - 1.6f * delta);
                    }
                }
            }
        }

        // --- AUG_SELECT / DEBUFF_SELECT: 1/2/3 키로 선택 ---
        // s_augSpaceReleased: 블록 바깥에서도 release 감지하도록 static 선언
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

            // 단일 증강 적용 헬퍼 (재귀용 — RANDOM_AUG/PANDORA 가 호출)
            std::function<void(int)> applyByIdx;
            applyByIdx = [&](int idx) {
                AugType atype = ALL_AUGS[idx].type;
                float oldWS   = g_Stats.windowSize;
                float oldPCX  = playerWin.x + playerWin.width  * 0.5f;
                float oldPCY  = playerWin.y + playerWin.height * 0.5f;

                // 특수: 디스패치만 수행하고 Apply 호출 X
                if (atype == AugType::S_CHAOS) {
                    // 대혼란: 보유 증강 잊고, 같은 개수 랜덤. 약 60% 버프 / 40% 디버프
                    // 실제 보유 목록(중첩 포함) 기준으로 카운트 — 공격력 증가×4 같은 중첩도 모두 포함
                    int prevAugs = (int)g_OwnedAugs.size();
                    g_Stats = PlayerStats();
                    g_Stats.windowSize *= g_Scale;          // 창 — 해상도 비례 유지
                    // 무기/직업 정체성은 유지 — CHAOS 는 증강만 재추첨한다.
                    //   (검객/궁수가 기본 총으로 바뀌던 버그 fix. 증강은 이 아래서 위에 덮임)
                    if (g_CurrentWeapon >= 0 && g_CurrentWeapon < (int)StartWeapon::_COUNT)
                        ApplyWeapon(g_Stats, (StartWeapon)g_CurrentWeapon);
                    if (g_RunMelee)    { g_Stats.meleeWeapon = true; g_Stats.fireInterval = 0.26f; }
                    else if (g_RunBow) { g_Stats.bowWeapon = true;  g_Stats.bulletSpeed *= 1.4f; }
                    g_Stats.baseFireInterval = g_Stats.fireInterval;
                    g_OwnedAugs.clear();
                    memset(g_GameManager.takenOnce, 0,
                           sizeof(g_GameManager.takenOnce));
                    memset(g_TypeOwned, 0, sizeof(g_TypeOwned));   // 조합 레시피 보유도 초기화
                    //   (없으면 더블 잃었는데 '관통 쌍둥이' 조합이 뜨던 버그)
                    g_GameManager.maxHP = g_Stats.maxHP;
                    if (g_GameManager.playerHP > g_Stats.maxHP)
                        g_GameManager.playerHP = g_Stats.maxHP;
                    // 보유 개수 보존 — 이전엔 24로 캡해서 40개 보유 시 반토막 나던 버그 수정.
                    if (prevAugs > 60) prevAugs = 60;       // 배열 상한(여유)
                    int nDebuffs = prevAugs / 3;            // 40% → 33% (덜 가혹하게)
                    int nBuffs   = prevAugs - nDebuffs;
                    int buffs[64], debuffs[64];
                    g_GameManager.PickRandomAugIndices(buffs, nBuffs,
                        false, false, true, false, /*allowDebuff=*/false);
                    g_GameManager.PickRandomDebuffIndices(debuffs, nDebuffs);
                    for (int k = 0; k < nBuffs;   k++) applyByIdx(buffs[k]);
                    for (int k = 0; k < nDebuffs; k++) applyByIdx(debuffs[k]);
                    return;
                }
                if (atype == AugType::S_PANDORA) {
                    // PANDORA: 5장 = 3 버프 + 2 디버프 (명시적 분리)
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
                    // RANDOM_AUG: 버프만 (디버프 제외, 사용자 의도 유지)
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
                g_OwnedAugs.push_back(idx);
                g_TypeOwned[(int)atype] = true;   // 조합 레시피 판정용
                MarkAugSeen(idx);                 // 도감 발견
                // 액티브 스킬 증강이면 슬롯에 장착 (꽉 차면 순환 교체)
                EquipSkill(SkillForAug(atype));

                // 포탑 배치(CB_TURRET 조합) 픽 → 즉시 첫 포탑 배치
                if (g_Stats.turretMode && !prevTurret) {
                    g_Turrets.clear();
                    g_TurretDeployTimer = TURRET_DEPLOY;  // 즉시 첫 포탑 배치
                }

                // 한 번만 뽑힐 증강 표시:
                //   - EPIC/LEGENDARY 전체
                //   - 티어드 증강 (탄환세례/드론/차크람) — 등급 무관 1회씩
                if (AugOnceOnly(atype, ALL_AUGS[idx].rarity))
                    g_GameManager.takenOnce[idx] = true;

                // (흡혈마 버프: 현재 HP -20% 패널티 제거 — 최대 체력 +25/흡혈로 변경)

                // 미니화: 강제로 maxHP 10 적용 후 현재 HP cap
                if (g_GameManager.playerHP > g_Stats.maxHP)
                    g_GameManager.playerHP = g_Stats.maxHP;

                // 차크람 (티어 적용 후 chakramCount 만큼 활성화)
                if (atype == AugType::CHAKRAM ||
                    atype == AugType::CHAKRAM_2 ||
                    atype == AugType::CHAKRAM_3) {
                    for (int c = 0; c < g_Stats.chakramCount && c < MAX_CHAKRAMS; c++) {
                        if (!g_Chakrams[c].alive && g_Chakrams[c].respawnTimer <= 0) {
                            g_Chakrams[c].alive        = true;
                            g_Chakrams[c].hp           = 150.0f;
                            g_Chakrams[c].maxHp        = 150.0f;
                            g_Chakrams[c].respawnTimer = 0.0f;
                        }
                        // 균등 각도 배치
                        g_Chakrams[c].angle =
                            (float)c / (float)g_Stats.chakramCount * 6.2831853f;
                    }
                }

                // 고장난 조준선: 황금 오브 초기화
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
                // 다가오는 죽음: 첫 픽에서만 오브 스폰. 이후 픽은 속도만 +20%
                // (속도 누적은 PlayerStats::Apply 의 approachStacks ++ 가 담당)
                if (atype == AugType::D_APPROACH && g_ApproachOrbs.empty()) {
                    ApproachOrb orb;
                    int edge = rand() % 4;
                    if      (edge == 0) { orb.x = (float)(rand()%screenWidth);  orb.y = -40.0f; }
                    else if (edge == 1) { orb.x = (float)(rand()%screenWidth);  orb.y = screenHeight + 40.0f; }
                    else if (edge == 2) { orb.x = -40.0f;                       orb.y = (float)(rand()%screenHeight); }
                    else                { orb.x = screenWidth + 40.0f;          orb.y = (float)(rand()%screenHeight); }
                    g_ApproachOrbs.push_back(orb);
                }
                // 시야/크기 변경 → playerWin 크기·위치 갱신
                if (g_Stats.windowSize != oldWS) {
                    playerWin.width  = g_Stats.windowSize;
                    playerWin.height = g_Stats.windowSize;
                    playerWin.x = oldPCX - g_Stats.windowSize * 0.5f;
                    playerWin.y = oldPCY - g_Stats.windowSize * 0.5f;
                }
            };

            // 무기 변환 — 기존 무기 효과 제거 후 새 무기 적용 (#109)
            //   g_Stats 를 처음부터 재계산: fresh + 새 무기 + 보유 증강 전부 재적용
            //   런타임 카운터(killCount 등)는 보존
            auto convertWeapon = [&](int newWeapon) {
                long long savedKills    = g_Stats.killCount;
                int   savedVampStreak   = g_Stats.vampireKillStreak;
                bool  savedMk2Used      = g_Stats.mk2Used;
                float savedLightTimer   = g_Stats.lightStepDisableTimer;
                bool  savedTurret       = g_Stats.turretMode;

                PlayerStats fresh;
                ApplyWeapon(fresh, (StartWeapon)newWeapon);
                fresh.baseFireInterval = fresh.fireInterval;
                // 보유 증강 전부 재적용 (원래 픽 순서 유지)
                for (int ownedIdx : g_OwnedAugs)
                    fresh.Apply(ALL_AUGS[ownedIdx].type);

                // 런타임 카운터 복원
                fresh.killCount             = savedKills;
                fresh.vampireKillStreak     = savedVampStreak;
                fresh.mk2Used               = savedMk2Used;
                fresh.lightStepDisableTimer = savedLightTimer;

                g_Stats = fresh;
                g_CurrentWeapon = newWeapon;

                // 포탑 모드는 CB_TURRET 증강 보유 시 fresh.Apply 가 이미 복원함.
                // 무기 변환으로 새로 켜졌으면(이전 false) 첫 배치 타이머만 초기화.
                if (g_Stats.turretMode && !savedTurret) {
                    g_Turrets.clear(); g_TurretDeployTimer = TURRET_DEPLOY;
                }

                // HUD / 발사 타이머 동기화
                g_GameManager.maxHP = g_Stats.maxHP;
                if (g_GameManager.playerHP > g_Stats.maxHP)
                    g_GameManager.playerHP = g_Stats.maxHP;
                fireTimer = g_Stats.fireInterval;
            };

            auto applyAug = [&](int slot) {
                bool wasBuff  = (g_GameManager.currentState == GameState::AUG_SELECT);
                if (slot == 3) {
                    // 변환 카드 — 무기 전환 (기존 무기 효과 제거)
                    if (g_ConversionWeapon >= 0) convertWeapon(g_ConversionWeapon);
                    g_ConversionWeapon = -1;  // 변환 카드 소모
                } else {
                    applyByIdx(g_GameManager.augChoices[slot]);
                }
                g_GameManager.maxHP = g_Stats.maxHP;
                if (wasBuff) {
                    // 보스 보상 중이면 디버프 페이지 skip
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
                        // MK2 부활 후 / 크리에이티브 F 그랩: DEBUFF_SELECT 스킵
                        //   (크리에이티브라도 레벨업은 정상 디버프 페이지 = "디버프도 뜨는 게임")
                        g_CreativeFreeGrab = false;
                        g_GameManager.currentState = GameState::RUNNING;
                    } else {
                        // 일반: 버프 → 디버프 강제 선택 페이지
                        g_GameManager.PickDebuffChoices();
                        g_GameManager.currentState = GameState::DEBUFF_SELECT;
                    }
                } else {
                    // 디버프 픽 끝 → 게임 재개
                    g_GameManager.currentState = GameState::RUNNING;
                }
                // C14: 인게임 복귀 시 짧은 유예 — 앞 0.25s 는 적 정지("텀"), 전체 무적+발사억제
                //   로 즉사/오발 방지하며 자연스러운 전이 딜레이를 준다.
                if (g_GameManager.currentState == GameState::RUNNING)
                    g_PostPickGrace = 0.5f;
            };

            // 1/2/3/4 = hover (선택 후보 변경만, 적용 X). 4는 변환 카드 (있을 때만)
            static bool s_aug4Released = true;
            int k4 = glfwGetKey(window, GLFW_KEY_4);
            if (k1 == GLFW_PRESS && g_aug1Released) { g_HoveredAug = 0; g_aug1Released = false; }
            if (k2 == GLFW_PRESS && g_aug2Released) { g_HoveredAug = 1; g_aug2Released = false; }
            if (k3 == GLFW_PRESS && g_aug3Released) { g_HoveredAug = 2; g_aug3Released = false; }
            if (k4 == GLFW_PRESS && s_aug4Released && g_ConversionWeapon >= 0 &&
                g_GameManager.currentState == GameState::AUG_SELECT) {
                g_HoveredAug = 3; s_aug4Released = false;
            }
            if (k1 == GLFW_RELEASE) g_aug1Released = true;
            if (k2 == GLFW_RELEASE) g_aug2Released = true;
            if (k3 == GLFW_RELEASE) g_aug3Released = true;
            if (k4 == GLFW_RELEASE) s_aug4Released = true;

            // Space = 적용 (hover 된 카드만). Enter 키 제거 → Space 로 통일
            int kSp = glfwGetKey(window, GLFW_KEY_SPACE);
            if (kSp == GLFW_PRESS && s_augSpaceReleased && g_HoveredAug >= 0) {
                applyAug(g_HoveredAug);
                g_HoveredAug = -1;
                s_augSpaceReleased = false;
                g_GameManager.spaceReleased = false; // RUNNING 직후 pause 방지
            }

            // 마우스 클릭: 카드 hit-test → hover 만 (적용은 Space 키로만)
            if (lmb && !g_LmbPrev) {
                bool hasConv = (g_ConversionWeapon >= 0 &&
                                g_GameManager.currentState == GameState::AUG_SELECT);
                int  nCards  = hasConv ? 4 : 3;
                const float CARD_W = hasConv ? 240.0f : 280.0f;
                const float CARD_H = 400.0f;
                const float GAP    = hasConv ? 32.0f : 48.0f;
                const float TOTAL_W = (float)nCards * CARD_W + (float)(nCards-1) * GAP;
                float baseX = (screenWidth  - TOTAL_W) * 0.5f;
                float baseY = (screenHeight - CARD_H)  * 0.4f;
                for (int i = 0; i < nCards; i++) {
                    float cx = baseX + i * (CARD_W + GAP);
                    float yOff = (g_HoveredAug == i) ? -16.0f : 0.0f;
                    if (mx >= cx && mx <= cx + CARD_W &&
                        my >= baseY + yOff && my <= baseY + yOff + CARD_H) {
                        g_HoveredAug = i;  // 클릭 = hover 만 (적용은 Space)
                        break;
                    }
                }
            }
        }

        // --- 크리에이티브 모드: F = 증강 그랩(디버프 포함 샌드박스), G = 무적 토글 ---
        if (g_CreativeMode) {
            static bool s_fkeyReleased = true;
            int kF = glfwGetKey(window, GLFW_KEY_F);
            if (kF == GLFW_RELEASE) s_fkeyReleased = true;
            if (kF == GLFW_PRESS && s_fkeyReleased &&
                g_GameManager.currentState == GameState::RUNNING) {
                // 샌드박스: 디버프도 카드 풀에 섞어서 무엇이든 집을 수 있게
                g_GameManager.PickAugChoices(g_Stats.sizeAugTaken,
                                             g_Stats.distAugTaken, /*allowDebuff=*/true);
                g_CreativeFreeGrab = true;   // 이 픽 뒤엔 디버프 페이지 강제 X
                // 변환 카드 — 25% 확률 (검객/궁수 제외)
                g_ConversionWeapon = -1;
                if (!g_Stats.meleeWeapon && !g_Stats.bowWeapon &&
                    (rand() % 100) < 25 && g_CurrentWeapon >= 0) {
                    int wc = (int)StartWeapon::_COUNT;
                    int pick = rand() % wc;
                    if (pick == g_CurrentWeapon) pick = (pick + 1) % wc;
                    g_ConversionWeapon = pick;
                }
                g_GameManager.currentState = GameState::AUG_SELECT;
                s_fkeyReleased = false;
            }
            // G — 무적 ON/OFF 토글
            static bool s_gkeyReleased = true;
            int kG = glfwGetKey(window, GLFW_KEY_G);
            if (kG == GLFW_RELEASE) s_gkeyReleased = true;
            if (kG == GLFW_PRESS && s_gkeyReleased) {
                g_CreativeGodmode = !g_CreativeGodmode;
                s_gkeyReleased = false;
            }
        }

        // --- 히트스톱: 큰 이벤트 직후 잠깐 시뮬레이션 정지 (렌더는 계속) ---
        if (g_HitStopTimer > 0.0f) g_HitStopTimer -= delta;

        // --- Fixed timestep (DYING = 0.15× 슬로우 모션, 히트스톱 = 완전 정지) ---
        float physDelta = (g_GameManager.currentState == GameState::DYING)
                          ? delta * 0.15f : delta;
        if (g_HitStopTimer > 0.0f) physDelta = 0.0f;   // 누적 안 함 → 스텝 미실행
        accumulator += physDelta;
        while (accumulator >= FIXED_DT) {
            if (g_GameManager.ShouldUpdate()) {
                // WASD 이동 — 대각선 normalize (vec 모아서 길이로 나눔)
                float mvX = 0.0f, mvY = 0.0f;
                if (keys[GLFW_KEY_W]) mvY -= 1.0f;
                if (keys[GLFW_KEY_S]) mvY += 1.0f;
                if (keys[GLFW_KEY_A]) mvX -= 1.0f;
                if (keys[GLFW_KEY_D]) mvX += 1.0f;
                float mlen = sqrtf(mvX*mvX + mvY*mvY);
                if (mlen > 0.001f) {
                    mvX /= mlen; mvY /= mlen;
                    float moveMult = g_Stats.GetMoveMultiplier(lmb);
                    // 배드 섹터 감속 구역 — 부식되어 퍼진 영역(grow factor) 안이면 이동속도 -10%
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
                    // FORK.worm 크래시 벽 — 일반 이동은 직선에 막힘, 대시(무적 중)는 통과
                    if (g_CentiBoss && g_CentiBoss->alive && g_DashInvuln <= 0.0f) {
                        float wpcx = playerWin.x + playerWin.width  * 0.5f;
                        float wpcy = playerWin.y + playerWin.height * 0.5f;
                        g_CentiBoss->blockMove(wpcx, wpcy, 18.0f);
                        playerWin.x = wpcx - playerWin.width  * 0.5f;
                        playerWin.y = wpcy - playerWin.height * 0.5f;
                    }
                    // 이동 잔상(afterimage) — 일정 간격으로 플레이어 중심에 옅은 시안 잔상
                    static float s_trailAcc = 0.0f;
                    s_trailAcc += FIXED_DT;
                    if (s_trailAcc >= 0.028f) {
                        s_trailAcc = 0.0f;
                        SpawnTrail(playerWin.x + playerWin.width  * 0.5f,
                                   playerWin.y + playerWin.height * 0.5f,
                                   24.0f, 0.35f, 0.85f, 1.0f);
                    }
                }

                // 플레이어 캐릭터(창 중앙)가 보이는 영역 밖으로 나가지 못하게 클램프.
                // 줌아웃(폴리모프 2페이즈)되면 보이는 월드가 넓어지므로 이동 구역도 같이 확장.
                // (zoom=1 이면 정확히 [0, screen] — 기존과 동일)
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
                // C17: 하단 작업표시줄(인게임 가짜 바 + 실제 OS 작업표시줄) 침범 방지 —
                //   작업표시줄은 '스크린' 하단 고정 픽셀이라, 줌아웃(점수 확장)되면 월드 단위로
                //   barTotal/zoom 만큼 차지함. 보이는 영역 하단(ccY+halfH)에서 그만큼 위가 한계 →
                //   상단 확장과 대칭이 되도록(기존엔 sh-barTotal 고정이라 하단만 안 늘어났음).
                float barTotal    = g_GameBarH + (float)g_TaskbarH;
                float bottomLimit = ccY + halfH - barTotal / zoomNow;
                if (pCY > bottomLimit) pCY = bottomLimit;
                playerWin.x = pCX - playerWin.width  * 0.5f;
                playerWin.y = pCY - playerWin.height * 0.5f;

                // ── 액티브 스킬 — 쿨다운/지속 갱신 + 입력(Shift 대시 / Q·E·R 슬롯) ──
                if (g_DashCd > 0.0f)        g_DashCd        -= FIXED_DT;
                if (g_DashInvuln > 0.0f)    g_DashInvuln    -= FIXED_DT;
                if (g_PostPickGrace > 0.0f) g_PostPickGrace -= FIXED_DT;  // C14
                if (g_TimeStopTimer > 0.0f) g_TimeStopTimer -= FIXED_DT;
                if (g_OverclockTimer > 0.0f)g_OverclockTimer-= FIXED_DT;
                for (int i = 0; i < 3; i++) if (g_Skills[i].cd > 0.0f) g_Skills[i].cd -= FIXED_DT;

                // 창 닫기 — 플레이어 중심 폭발 (넉백 + 피해)
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
                    for (auto* c : g_LeakNodes)                if (c->alive) { float dx=c->worldX-cx,dy=c->worldY-cy; if(dx*dx+dy*dy<r2){c->hp-=dmg; if(c->hp<=0)c->alive=false;} }
                    if (g_MonsterManager.boss && g_MonsterManager.boss->alive) { float dx=g_MonsterManager.boss->worldX-cx,dy=g_MonsterManager.boss->worldY-cy; if(dx*dx+dy*dy<r2){g_MonsterManager.boss->hp-=dmg; if(g_MonsterManager.boss->hp<=0)g_MonsterManager.boss->alive=false;} }
                    if (g_RRBoss && g_RRBoss->alive) { float dx=g_RRBoss->worldX-cx,dy=g_RRBoss->worldY-cy; if(dx*dx+dy*dy<r2){g_RRBoss->hp-=dmg; if(g_RRBoss->hp<=0)g_RRBoss->alive=false;} }
                    if (g_PolyBoss && g_PolyBoss->alive && g_PolyBoss->damageable()) { float dx=g_PolyBoss->worldX-cx,dy=g_PolyBoss->worldY-cy; if(dx*dx+dy*dy<r2){g_PolyBoss->hp-=dmg; if(g_PolyBoss->hp<=0)g_PolyBoss->alive=false;} }
                    if (g_SpamBoss && g_SpamBoss->alive) { float dx=g_SpamBoss->worldX-cx,dy=g_SpamBoss->worldY-cy; if(dx*dx+dy*dy<r2){g_SpamBoss->hp-=dmg; if(g_SpamBoss->hp<=0)g_SpamBoss->alive=false;} }
                    if (g_KernelBoss && g_KernelBoss->alive) { float dx=g_KernelBoss->worldX-cx,dy=g_KernelBoss->worldY-cy; if(dx*dx+dy*dy<r2){g_KernelBoss->hp-=dmg; if(g_KernelBoss->hp<=0)g_KernelBoss->alive=false;} }
                    if (g_FirewallBoss && g_FirewallBoss->alive) { float dx=g_FirewallBoss->worldX-cx,dy=g_FirewallBoss->worldY-cy; if(dx*dx+dy*dy<r2){g_FirewallBoss->hp-=dmg; if(g_FirewallBoss->hp<=0)g_FirewallBoss->alive=false;} }
                    if (g_BotnetBoss && g_BotnetBoss->alive) { float dx=g_BotnetBoss->worldX-cx,dy=g_BotnetBoss->worldY-cy; if(dx*dx+dy*dy<r2){g_BotnetBoss->hp-=dmg; if(g_BotnetBoss->hp<=0)g_BotnetBoss->alive=false;} }
                    if (g_CentiBoss && g_CentiBoss->alive && g_CentiBoss->vulnerable()) { float dx=g_CentiBoss->worldX-cx,dy=g_CentiBoss->worldY-cy; if(dx*dx+dy*dy<r2){g_CentiBoss->hp-=dmg; if(g_CentiBoss->hp<=0)g_CentiBoss->alive=false;} }
                    if (g_TotemBoss && g_TotemBoss->alive && g_TotemBoss->vulnerable()) { float dx=g_TotemBoss->worldX-cx,dy=g_TotemBoss->worldY-cy; if(dx*dx+dy*dy<r2){g_TotemBoss->hp-=dmg; if(g_TotemBoss->hp<=0)g_TotemBoss->alive=false;} }
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

                // 슬롯 스킬 발동 헬퍼
                auto useSkill = [&](int slot) {
                    if (slot < 0 || slot >= 3) return;
                    if (g_TotemBoss && g_TotemBoss->alive && g_TotemBoss->isSkillSealed(slot)) return;
                    SkillSlot& s = g_Skills[slot];
                    if (s.type == SkillType::NONE || s.cd > 0.0f) return;
                    switch (s.type) {
                    case SkillType::CLOSE_WINDOW: closeWindowBlast(pCX, pCY); break;
                    case SkillType::OVERCLOCK:    g_OverclockTimer = OVERCLOCK_DUR; break;
                    case SkillType::TIME_STOP:    g_TimeStopTimer  = TIMESTOP_DUR;
                                                  TriggerFlash(0.4f,0.9f,1.0f,0.4f); break;
                    default: break;
                    }
                    s.cd = SkillCooldownMax(s.type);
                };

                // 입력 (엣지 검출)
                static bool pDash=false, pQ=false, pE=false, pR=false;
                bool cDash = keys[GLFW_KEY_LEFT_SHIFT] || keys[GLFW_KEY_RIGHT_SHIFT];
                if (cDash && !pDash && g_DashCd <= 0.0f) {
                    float ddx = mvX, ddy = mvY;
                    if (mlen <= 0.001f) {              // 안 움직이면 조준 방향으로
                        float ax = wmx - pCX, ay = wmy - pCY;
                        float al = std::sqrt(ax*ax+ay*ay)+1e-3f; ddx = ax/al; ddy = ay/al;
                    }
                    float ox = pCX, oy = pCY;
                    pCX += ddx * DASH_DIST; pCY += ddy * DASH_DIST;
                    if (pCX < ccX-halfW) pCX = ccX-halfW; if (pCX > ccX+halfW) pCX = ccX+halfW;
                    if (pCY < ccY-halfH) pCY = ccY-halfH; if (pCY > ccY+halfH) pCY = ccY+halfH;
                    playerWin.x = pCX - playerWin.width*0.5f;
                    playerWin.y = pCY - playerWin.height*0.5f;
                    g_DashInvuln = DASH_INVULN; g_DashCd = DASH_CD;
                    SpawnShockWave(ox, oy, 70.0f, 0.25f, 0.4f, 1.0f, 1.0f);
                }
                pDash = cDash;
                bool cQ = keys[GLFW_KEY_Q]; if (cQ && !pQ) useSkill(0); pQ = cQ;
                bool cE = keys[GLFW_KEY_E]; if (cE && !pE) useSkill(1); pE = cE;
                bool cR = keys[GLFW_KEY_R]; if (cR && !pR) useSkill(2); pR = cR;
                // C16: 액티브 스킬 자동 사용 — 쿨다운 끝난 슬롯을 자동 발동
                if (g_AutoSkill) for (int i = 0; i < 3; i++) useSkill(i);

                // 총알 이동 + 화면 밖 비활성화 (+ 유도탄 보정)
                for (auto& b : g_Bullets) {
                    // 유도탄: 가장 가까운 적을 향해 점진적 방향 보정
                    if (b.homing && !b.isEnemy && b.active) {
                        float tx = 0.0f, ty = 0.0f;
                        if (findNearestEnemy(b.x, b.y, tx, ty)) {
                            // 현재 방향 → 목표 방향 사이를 turn rate 만큼 회전
                            float wx = tx - b.x, wy = ty - b.y;
                            float wl = sqrtf(wx*wx + wy*wy);
                            if (wl > 0.001f) {
                                float wdx = wx / wl, wdy = wy / wl;
                                // 외적/내적으로 각도 차이 (작은 단계)
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
                    // 적 유도탄(원거리몹 탄): 플레이어 쪽으로 아주 약하게 방향 보정.
                    //   단, 플레이어 근처(<300px)에선 유도 중단 → 빗나간 탄이 공전하지 않고
                    //   그대로 지나감(이전 0.9 rad/s 가 플레이어 주위를 도는 문제 수정).
                    if (b.homing && b.isEnemy && b.active && g_TimeStopTimer <= 0.0f) {
                        float pCX = playerWin.x + playerWin.width  * 0.5f;
                        float pCY = playerWin.y + playerWin.height * 0.5f;
                        float wx = pCX - b.x, wy = pCY - b.y;
                        float wl = sqrtf(wx*wx + wy*wy);
                        if (wl > 300.0f) {
                            float curA  = atan2f(b.dirY, b.dirX);
                            float wantA = atan2f(wy / wl, wx / wl);
                            float diff  = wantA - curA;
                            while (diff >  3.14159265f) diff -= 6.2831853f;
                            while (diff < -3.14159265f) diff += 6.2831853f;
                            float maxStep = b.homingTurn * FIXED_DT;   // 작은 turn rate
                            if (diff >  maxStep) diff =  maxStep;
                            if (diff < -maxStep) diff = -maxStep;
                            float newA = curA + diff;
                            b.dirX = cosf(newA); b.dirY = sinf(newA);
                        }
                    }
                    // 시간 정지: 적 총알은 멈춤 (플레이어 총알은 계속 이동)
                    if (!(b.isEnemy && g_TimeStopTimer > 0.0f)) b.Update(FIXED_DT);
                    // 화면 밖 비활성화 — 줌아웃(폴리모프 2페이즈)되면 보이는 영역이
                    // 넓어지므로 경계도 같이 확장 (안 그러면 확장 구역에서 탄이 즉시 사라짐)
                    {
                        float zb = (g_ViewZoom < 0.01f) ? 0.01f : g_ViewZoom;
                        float mX = screenWidth  * 0.5f * (1.0f / zb - 1.0f) + 200.0f;
                        float mY = screenHeight * 0.5f * (1.0f / zb - 1.0f) + 200.0f;
                        if (b.x < -mX || b.x > screenWidth  + mX ||
                            b.y < -mY || b.y > screenHeight + mY)
                            b.active = false;
                    }
                }

                // 대시 무적 — 이번 스텝 시작 HP 저장 (적 피해는 무효, 회복은 유지)
                float hpAtStep = g_GameManager.playerHP;
                //   적 정지 = 시간정지 스킬 OR 증강 픽 직후 ~0.05s("전이 텀", 짧게)
                bool  timeStopped = (g_TimeStopTimer > 0.0f) || (g_PostPickGrace > 0.45f);

                // 몬스터 업데이트 (디버프 multiplier 적용) — 시간 정지 중엔 적 멈춤
                float rmobMoveMult = 1.0f / g_Stats.rmobDelayMult; // <1 → 더 빠름
                // 점수 기반 속도 램프 (지루함 방지)
                float si = (float)g_GameManager.score / 100000.0f; if (si > 6.0f) si = 6.0f;
                float mobSpdRamp = 1.0f + si * 0.09f;
                if (!timeStopped)
                    g_MonsterManager.UpdateAll(pCX, pCY, FIXED_DT,
                                               g_GameManager.playerHP, g_Bullets,
                                               g_Stats.mobSpeedMult * mobSpdRamp,
                                               rmobMoveMult * mobSpdRamp);

                // 리로드 러너 업데이트 (무기 상태머신 + 장전 질주, 적 총알 push)
                if (!timeStopped && g_RRBoss && g_RRBoss->alive)
                    g_RRBoss->Update(pCX, pCY, FIXED_DT, g_GameManager.playerHP, g_Bullets);

                // 폴리모프 업데이트 (폼 변환 + 세모/레이저/차크람) + 페이즈2 화면 확장
                if (g_PolyBoss && g_PolyBoss->alive) {
                    if (!timeStopped)
                    g_PolyBoss->Update(pCX, pCY, FIXED_DT, g_GameManager.playerHP, g_Bullets);
                    // 페이즈2 = 상위 보스: 화면 줌아웃으로 더 넓은 구간에서 싸움 (의도된 기능)
                    //   점수 줌아웃과 충돌 않게 더 줌아웃된 쪽(min) 채택. (페이즈1은 점수줌 유지)
                    g_ViewZoomTarget = std::min(g_ViewZoomTarget, g_PolyBoss->phase2 ? 0.5f : 1.0f);
                    // ── 2페이즈 진입 연출 — 보스 포효 + 다중 충격파 (1회) ──
                    if (g_PolyBoss->phase2 && !g_PolyWasPhase2) {
                        g_PolyWasPhase2 = true;
                        float bx = g_PolyBoss->worldX, by = g_PolyBoss->worldY;
                        g_ShakeTime = 1.0f; g_ShakeMag = 42.0f;        // 강한 흔들림
                        TriggerFlash(0.6f, 0.25f, 1.0f, 0.85f);        // 보라 화이트아웃
                        TriggerHitStop(0.20f);                         // 임팩트 정지
                        // 퍼져나가는 보라 충격파 3겹 (포효)
                        SpawnShockWave(bx, by, 760.0f, 1.1f, 0.7f, 0.3f, 1.0f);
                        SpawnShockWave(bx, by, 500.0f, 0.9f, 0.85f, 0.45f, 1.0f);
                        SpawnShockWave(bx, by, 280.0f, 0.7f, 1.0f, 0.8f, 1.0f);
                        for (int k = 0; k < 6; k++)
                            SpawnEnemyExplosion(bx + (rand()%320 - 160),
                                                by + (rand()%320 - 160),
                                                0.7f, 0.3f, 1.0f, true);
                    }
                    // 폼 변환 순간 — 보라 파티클 분출 + 링
                    int curForm = (int)g_PolyBoss->form;
                    if (curForm != g_PolyPrevForm) {
                        if (g_PolyPrevForm != -1) {   // 최초 동기화는 연출 생략
                            SpawnEnemyExplosion(g_PolyBoss->worldX, g_PolyBoss->worldY,
                                                0.7f, 0.3f, 1.0f, true);
                            SpawnEnemyExplosion(g_PolyBoss->worldX, g_PolyBoss->worldY,
                                                0.95f, 0.6f, 1.0f, true);
                            SpawnShockWave(g_PolyBoss->worldX, g_PolyBoss->worldY,
                                           170.0f, 0.45f, 0.7f, 0.3f, 1.0f);
                        }
                        g_PolyPrevForm = curForm;
                    }
                }

                // SPAM.dll 업데이트 (회전 나선탄 + 방사 버스트, 적 총알 push)
                if (!timeStopped && g_SpamBoss && g_SpamBoss->alive)
                    g_SpamBoss->Update(pCX, pCY, FIXED_DT, g_GameManager.playerHP, g_Bullets);

                // KERNEL.sys 업데이트 (자가붕괴 + 팽창/수축)
                if (!timeStopped && g_KernelBoss && g_KernelBoss->alive)
                    g_KernelBoss->Update(pCX, pCY, FIXED_DT, g_GameManager.playerHP, g_Bullets);

                // FIREWALL.sys 업데이트 (회전 보호막 가변속도 + 견제 사격)
                if (!timeStopped && g_FirewallBoss && g_FirewallBoss->alive)
                    g_FirewallBoss->Update(pCX, pCY, FIXED_DT, g_GameManager.playerHP, g_Bullets);

                // C2_RELAY.sys 업데이트 (호스트 릴레이 + OVERLOAD)
                if (!timeStopped && g_BotnetBoss && g_BotnetBoss->alive)
                    g_BotnetBoss->Update(pCX, pCY, FIXED_DT, g_GameManager.playerHP, g_Bullets);
                if (g_BotnetBoss && g_BotnetBoss->shakePulse) {
                    g_BotnetBoss->shakePulse = false;
                    g_ShakeTime = 0.32f; g_ShakeMag = 14.0f;
                }

                // FORK.worm 업데이트 (지그재그 배회 + 화면밖 이탈→재진입 돌진)
                if (!timeStopped && g_CentiBoss && g_CentiBoss->alive) {
                    g_CentiBoss->Update(pCX, pCY, FIXED_DT, g_GameManager.playerHP, g_Bullets);
                    if (g_CentiBoss->shakePulse) {          // 화면 밖으로 나갈 때 약한 진동
                        g_CentiBoss->shakePulse = false;
                        g_ShakeTime = 0.35f; g_ShakeMag = 14.0f;
                    }
                    // 스킬 시전 진동(가벼운 피드백, 눈뽕 X) — 큰 진동 중이면 덮어쓰지 않음
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

                // TOTEM.sys 업데이트 (토템 봉인 + 순간이동 공격)
                if (!timeStopped && g_TotemBoss && g_TotemBoss->alive) {
                    float pullX = 0.0f, pullY = 0.0f;
                    g_TotemBoss->Update(pCX, pCY, FIXED_DT, g_GameManager.playerHP,
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

                // 슬라임 분열체 업데이트 (돌진만, 소환 X) — outSummons 폐기
                if (!g_LeakNodes.empty()) {
                    std::vector<Monster*> sink;
                    for (auto* c : g_LeakNodes)
                        if (c->alive && !timeStopped)
                            c->Update(pCX, pCY, FIXED_DT, g_GameManager.playerHP, sink);
                    for (auto* m : sink) delete m;   // chargeOnly 라 보통 비어있음
                }

                // ── 보스 페이즈2 진입 (상승엣지) — 통일 연출 + 보스별 처리 ──
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
                // LEAK.sys — P2 HEAP+ 노드 + 누수 버스트
                if (g_MonsterManager.boss && g_MonsterManager.boss->alive) {
                    auto* b = g_MonsterManager.boss;
                    if (b->phase2 && !g_LeakWasP2) {
                        g_LeakWasP2 = true;
                        p2enter(b->worldX, b->worldY, glm::vec3(0.45f, 0.55f, 1.0f));
                        float aHp = b->maxHp * 0.16f;
                        g_LeakNodes.push_back(MakeLeakNode(b->worldX - 65, b->worldY,
                                              aHp, 0.48f, 1, screenWidth, screenHeight));
                        g_LeakNodes.push_back(MakeLeakNode(b->worldX + 65, b->worldY,
                                              aHp, 0.48f, 1, screenWidth, screenHeight));
                        std::vector<Monster*> burst;
                        b->onPhase2Burst(burst);
                        for (auto* m : burst) g_MonsterManager.monsters.push_back(m);
                    }
                    if (b->phase3 && !g_LeakWasP3) {
                        g_LeakWasP3 = true;
                        g_ShakeTime = 0.65f; g_ShakeMag = 34.0f;
                        TriggerFlash(0.35f, 0.45f, 1.0f, 0.45f);
                        TriggerHitStop(0.14f);
                        SpawnShockWave(b->worldX, b->worldY, 480.0f, 0.85f, 0.35f, 0.55f, 1.0f);
                    }
                } else { g_LeakWasP2 = false; g_LeakWasP3 = false; }
                // RELOADER — 오버클럭 + ASSAULT 전면전
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
                // SPAM — 역회전 이중 나선 + 조준 버스트
                if (g_SpamBoss && g_SpamBoss->alive) {
                    if (g_SpamBoss->phase2 && !g_SpamWasP2) {
                        g_SpamWasP2 = true;
                        p2enter(g_SpamBoss->worldX, g_SpamBoss->worldY, glm::vec3(1.0f, 0.4f, 0.8f));
                    }
                } else g_SpamWasP2 = false;
                // C2_RELAY — OVERLOAD 페이즈
                if (g_BotnetBoss && g_BotnetBoss->alive) {
                    if (g_BotnetBoss->phase2 && !g_BotnetWasP2) {
                        g_BotnetWasP2 = true;
                        p2enter(g_BotnetBoss->worldX, g_BotnetBoss->worldY,
                                glm::vec3(0.25f, 0.95f, 0.45f));
                    }
                } else g_BotnetWasP2 = false;

                // 충돌 (반환값 = 플레이어가 이번 프레임 피격됐는지)
                bool hit = CollisionSystem::Update(pCX, pCY,
                    g_MonsterManager, g_Bullets,
                    g_GameManager.playerHP,
                    g_GameManager.scoreAccum, g_GameManager.score,
                    g_Stats, g_GameManager.xp);
                // 대시 무적 / 증강픽 유예(C14) — 이번 스텝의 적 피해 무효 (회복은 유지)
                if ((g_DashInvuln > 0.0f || g_PostPickGrace > 0.0f) &&
                    g_GameManager.playerHP < hpAtStep)
                    g_GameManager.playerHP = hpAtStep;

                // 리로드 러너 본체 vs 플레이어 총알 (스윕 판정)
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

                // SPAM.dll 본체 vs 플레이어 총알 (스윕 판정)
                if (g_SpamBoss && g_SpamBoss->alive) {
                    auto* sb = g_SpamBoss;
                    for (auto& b : g_Bullets) {
                        if (!b.active || b.isEnemy) continue;
                        if (SegDist(sb->worldX, sb->worldY,
                                    b.prevX, b.prevY, b.x, b.y) < SpamBoss::BODY * 0.8f) {
                            float pd = glm::distance(glm::vec2(pCX, pCY),
                                                     glm::vec2(sb->worldX, sb->worldY));
                            float dmg;
                            if (b.remainingDmg > 0.0f)   dmg = b.remainingDmg;
                            else if (b.turretDmg > 0.0f) dmg = b.turretDmg;
                            else dmg = g_Stats.GetBaseDamage()
                                     * g_Stats.GetDamageMultiplier(pd) * b.dmgMult;
                            float dealt = (dmg < sb->hp) ? dmg : sb->hp;
                            sb->hp -= dealt;
                            if (b.remainingDmg > 0.0f) b.remainingDmg -= dealt;
                            if (sb->hp <= 0.0f) sb->alive = false;
                            if (b.remainingDmg <= 0.001f) b.active = false;
                        }
                    }
                }

                // KERNEL.sys 본체 vs 플레이어 총알 (스윕 판정) — 큰 코어
                if (g_KernelBoss && g_KernelBoss->alive) {
                    auto* kb = g_KernelBoss;
                    for (auto& b : g_Bullets) {
                        if (!b.active || b.isEnemy) continue;
                        if (SegDist(kb->worldX, kb->worldY,
                                    b.prevX, b.prevY, b.x, b.y) < KernelBoss::BODY * 0.95f) {
                            float pd = glm::distance(glm::vec2(pCX, pCY),
                                                     glm::vec2(kb->worldX, kb->worldY));
                            float dmg;
                            if (b.remainingDmg > 0.0f)   dmg = b.remainingDmg;
                            else if (b.turretDmg > 0.0f) dmg = b.turretDmg;
                            else dmg = g_Stats.GetBaseDamage()
                                     * g_Stats.GetDamageMultiplier(pd) * b.dmgMult;
                            float dealt = (dmg < kb->hp) ? dmg : kb->hp;
                            kb->hp -= dealt;
                            if (b.remainingDmg > 0.0f) b.remainingDmg -= dealt;
                            if (kb->hp <= 0.0f) kb->alive = false;
                            if (b.remainingDmg <= 0.001f) b.active = false;
                        }
                    }
                }

                // FIREWALL.sys vs 플레이어 총알 — 보호막 사이 틈으로만 본체 타격
                if (g_FirewallBoss && g_FirewallBoss->alive) {
                    auto* fb = g_FirewallBoss;
                    const float SR = FirewallBoss::SHIELD_R;
                    for (auto& b : g_Bullets) {
                        if (!b.active || b.isEnemy) continue;
                        float dx = b.x - fb->worldX, dy = b.y - fb->worldY;
                        float dist2 = dx*dx + dy*dy;
                        if (dist2 >= SR * SR) continue;          // 보호막 밖 — 무시
                        float ang = atan2f(dy, dx);
                        // 보호막 막힌 각도면 흡수(무피해)
                        if (dist2 > (FirewallBoss::BODY*0.9f)*(FirewallBoss::BODY*0.9f)
                            && fb->shieldBlocks(ang)) {
                            b.active = false;
                            SpawnSparks(b.x, b.y, 3, 1.0f, 0.5f, 0.2f);
                            continue;
                        }
                        // 틈으로 들어와 본체 타격
                        if (SegDist(fb->worldX, fb->worldY, b.prevX, b.prevY, b.x, b.y)
                            < FirewallBoss::BODY * 0.95f) {
                            float pd = glm::distance(glm::vec2(pCX, pCY),
                                                     glm::vec2(fb->worldX, fb->worldY));
                            float dmg;
                            if (b.remainingDmg > 0.0f)   dmg = b.remainingDmg;
                            else if (b.turretDmg > 0.0f) dmg = b.turretDmg;
                            else dmg = g_Stats.GetBaseDamage()
                                     * g_Stats.GetDamageMultiplier(pd) * b.dmgMult;
                            float dealt = (dmg < fb->hp) ? dmg : fb->hp;
                            fb->hp -= dealt;
                            if (b.remainingDmg > 0.0f) b.remainingDmg -= dealt;
                            if (fb->hp <= 0.0f) fb->alive = false;
                            if (b.remainingDmg <= 0.001f) b.active = false;
                        }
                    }
                }

                // C2_RELAY.sys — 좀비 호스트 + 본체(피해 감소) vs 플레이어 총알
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

                // FORK.worm 크래시 벽 — 총알이 직선을 가로지르면 그 셀만 뚫림.
                //   일반탄은 벽에 막혀 소멸(관통 X). 관통탄(remainingDmg>0)은 통과.
                if (g_CentiBoss && g_CentiBoss->alive && !g_CentiBoss->walls.empty()) {
                    for (auto& b : g_Bullets) {
                        if (!b.active || b.isEnemy) continue;
                        if (g_CentiBoss->hitWall(b.prevX, b.prevY, b.x, b.y) && b.remainingDmg <= 0.0f)
                            b.active = false;
                    }
                }

                // FORK.worm child adds vs 플레이어 총알 — 항상 피격 가능. 경험치 0.
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
                                    AddKillCombo();                 // 콤보만, 경험치 0
                                    g_GameManager.scoreAccum += 80.0f;
                                }
                                if (b.remainingDmg <= 0.001f) b.active = false;
                                break;
                            }
                        }
                    }
                }

                // FORK.worm 몸통 노드 — 피격 VFX (데미지는 머리 HP 풀, 플래시만)
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

                // FORK.worm 머리 vs 플레이어 총알 — 배회(피격가능) 중에만 타격
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
                            dmg *= cb2->dmgTakenMult;   // 프로토타입: 보통 피해 감소
                            float dealt = (dmg < cb2->hp) ? dmg : cb2->hp;
                            cb2->hp -= dealt;
                            if (b.remainingDmg > 0.0f) b.remainingDmg -= dealt;
                            if (cb2->hp <= 0.0f) cb2->alive = false;
                            if (b.remainingDmg <= 0.001f) b.active = false;
                        }
                    }
                }

                // TOTEM.sys — 토템 + 본체(무적/취약) + 팽창 보호막
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

                // 폴리모프 보스 vs 플레이어 총알 (차크람 / 본체 반사·방어막 / 세모 EXP)
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

                        // 1) 세모 무리 — 격파 시 EXP 2
                        bool consumed = false;
                        for (auto& s : pb->swarm) {
                            if (!s.alive) continue;
                            if (SegDist(s.x, s.y, b.prevX, b.prevY, b.x, b.y) < 13.0f) {
                                s.alive = false;
                                AddKillCombo();
                                g_GameManager.xp += 2;
                                g_GameManager.scoreAccum += 10.0f;
                                if (b.remainingDmg <= 0.001f) { b.active = false; consumed = true; }
                                break;
                            }
                        }
                        if (consumed || !b.active) continue;

                        // 2) 차크람 (페이즈2 방어막)
                        for (auto& c : pb->chakrams) {
                            if (!c.alive) continue;
                            float cx = pb->worldX + cosf(c.angle) * 150.0f;
                            float cy = pb->worldY + sinf(c.angle) * 150.0f;
                            if (SegDist(cx, cy, b.prevX, b.prevY, b.x, b.y) < 26.0f) {
                                c.hp -= dmg;
                                if (c.hp <= 0.0f) c.alive = false;
                                if (b.remainingDmg > 0.0f) b.remainingDmg -= dmg;
                                if (b.remainingDmg <= 0.001f) { b.active = false; consumed = true; }
                                break;
                            }
                        }
                        if (consumed || !b.active) continue;

                        // 3) 본체
                        if (SegDist(pb->worldX, pb->worldY,
                                    b.prevX, b.prevY, b.x, b.y) < PolymorphBoss::BODY * 0.55f) {
                            if (pb->reflecting()) {
                                // 다이아몬드 폼: 반사 — 원래 위력 그대로 적 총알로
                                b.dirX = -b.dirX; b.dirY = -b.dirY;
                                b.prevX = b.x;    b.prevY = b.y;
                                b.isEnemy  = true;
                                b.enemyDmg = dmg;          // 플레이어 위력 그대로
                                b.color    = glm::vec3(0.8f, 0.3f, 1.0f);
                            } else if (pb->shielded()) {
                                // 차크람 남음 → 무적 (총알만 소모)
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

                // 슬라임 분열체 vs 플레이어 총알 (수동 충돌)
                if (!g_LeakNodes.empty()) {
                    for (auto& b : g_Bullets) {
                        if (!b.active || b.isEnemy) continue;
                        for (auto* c : g_LeakNodes) {
                            if (!c->alive) continue;
                            float bodyR = Boss::BODY_SIZE * c->sizeScale * 0.65f;
                            if (SegDist(c->worldX, c->worldY, b.prevX, b.prevY, b.x, b.y) < bodyR) {
                                float pd = glm::distance(glm::vec2(pCX, pCY),
                                                         glm::vec2(c->worldX, c->worldY));
                                float dmg;
                                if (b.remainingDmg > 0.0f)   dmg = b.remainingDmg;
                                else if (b.turretDmg > 0.0f) dmg = b.turretDmg;
                                else dmg = g_Stats.GetBaseDamage()
                                         * g_Stats.GetDamageMultiplier(pd) * b.dmgMult;
                                float dealt = (dmg < c->hp) ? dmg : c->hp;
                                c->hp -= dealt;
                                if (b.remainingDmg > 0.0f) b.remainingDmg -= dealt;
                                SpawnDamageNumber(c->worldX, c->worldY, dealt, dealt >= 40.0f);
                                if (c->hp <= 0.0f) { c->alive = false; AddKillCombo(); }
                                bool keep = (b.remainingDmg > 0.001f) ||
                                            (g_Stats.pierce && (rand()%100) < g_Stats.pierceChance);
                                if (!keep) b.active = false;
                                break;
                            }
                        }
                    }
                }

                // 처치 보상 정산 — 총알/근접이 아닌 모든 죽음(연쇄폭발·스킬·MK2·해킹 폭발 등)
                //   도 여기서 한 번씩 EXP/점수/콤보/흡혈을 받는다 (scored 플래그로 중복 방지).
                auto creditKill = [&](float xpBase, float scoreBase) {
                    AddKillCombo();
                    g_GameManager.xp        += (long long)(xpBase * g_Stats.xpMult);
                    g_Stats.killCount       += 1;
                    g_GameManager.scoreAccum += scoreBase;
                    g_GameManager.score      = (long long)g_GameManager.scoreAccum;
                    if (g_Stats.lifestealPerKill > 0.0f) {
                        g_GameManager.playerHP += g_Stats.lifestealPerKill;
                        if (g_GameManager.playerHP > g_Stats.maxHP)
                            g_GameManager.playerHP = g_Stats.maxHP;
                    }
                    if (g_Stats.vampire && ++g_Stats.vampireKillStreak >= 10) {
                        g_Stats.vampireKillStreak = 0;
                        g_GameManager.playerHP += 1.0f;
                        if (g_GameManager.playerHP > g_Stats.maxHP)
                            g_GameManager.playerHP = g_Stats.maxHP;
                    }
                };

                // 사망 폭발 spawn (다음 UpdateAll 이 실제 erase 하기 전에 위치 캡처)
                //   + 분열체 사망 시 작은 자식 2마리 (총 2세대까지)
                std::vector<Monster*> mobBorn;
                for (auto m : g_MonsterManager.monsters) {
                    if (!m->alive && !m->exploded) {
                        if (!m->scored) {       // 아직 보상 안 받은 죽음 → 정산
                            m->scored = true;
                            float xpB, scB; MobKillReward(m->kind, m->splitGen, m->elite, xpB, scB);
                            creditKill(xpB + (float)g_Stats.meleeXpBonus, scB);
                        }
                        SpawnEnemyExplosion(m->worldX, m->worldY,
                                            m->color.r, m->color.g, m->color.b,
                                            /*big=*/false);
                        m->exploded = true;
                        // 처치 연출 — 프로세스 종료 태그 (강적은 항상 강조)
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
                        // 폭발성 엘리트 — 죽을 때 터져 플레이어에게 광역 피해
                        if (m->elite == 3) {
                            float vbx = m->worldX, vby = m->worldY, vr = 120.0f;
                            SpawnEnemyExplosion(vbx, vby, 1.0f, 0.4f, 0.1f, true);
                            SpawnShockWave(vbx, vby, vr * 1.5f, 0.45f, 1.0f, 0.4f, 0.1f, true);
                            float ppx = playerWin.x + playerWin.width  * 0.5f;
                            float ppy = playerWin.y + playerWin.height * 0.5f;
                            float dpx = ppx - vbx, dpy = ppy - vby;
                            if (dpx*dpx + dpy*dpy < vr*vr) g_GameManager.playerHP -= 20.0f;
                        }
                        // 연쇄 폭발(DEATH_BLAST) — 사망 위치에서 주변 적에게 AoE
                        //   너프: 데미지 0.8→0.3 + 폭발로 죽은 몹은 다시 안 터짐(무한연쇄 차단)
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
                            if (g_MonsterManager.boss && g_MonsterManager.boss->alive) {
                                float ddx = g_MonsterManager.boss->worldX - bx;
                                float ddy = g_MonsterManager.boss->worldY - by;
                                if (ddx*ddx + ddy*ddy < blastR*blastR)
                                    g_MonsterManager.boss->hp -= blastDmg;
                            }
                        }
                        SpawnWormSplit(m, mobBorn);
                        SpawnBadSectorZone(m);   // 배드 섹터 — 사망 자리에 감속 구역
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
                // 보스 사망 → 보상 (score +20000, 버프 2개 픽 / 디버프 없이)
                if (g_MonsterManager.boss && !g_MonsterManager.boss->alive &&
                    !g_MonsterManager.boss->exploded) {
                    Boss* bs = g_MonsterManager.boss;
                    // 분열 폭발 + 충격파 + 화면 흔들기
                    SpawnEnemyExplosion(bs->worldX, bs->worldY, 0.6f, 0.95f, 0.6f, true);
                    SpawnEnemyExplosion(bs->worldX, bs->worldY, 0.4f, 0.9f, 0.4f, true);
                    SpawnShockWave(bs->worldX, bs->worldY, 350.0f, 0.7f,
                                   0.5f, 0.95f, 0.5f);
                    g_ShakeTime = 0.5f; g_ShakeMag = 18.0f;
                    TriggerFlash(0.5f, 1.0f, 0.6f, 0.5f); TriggerHitStop(0.08f);
                    bs->exploded = true;
                    // LEAK.sys 사망 → 힙 조각 2개 분리 (보상은 전멸 시)
                    float childHp = bs->maxHp * 0.38f;
                    g_LeakNodes.push_back(MakeLeakNode(bs->worldX - 48, bs->worldY,
                                          childHp, 0.62f, 1, screenWidth, screenHeight));
                    g_LeakNodes.push_back(MakeLeakNode(bs->worldX + 48, bs->worldY,
                                          childHp, 0.62f, 1, screenWidth, screenHeight));
                    g_LeakEncounter = true;          // 분열 인카운터 시작 (보상은 전멸 시)
                    // 원본 보스 객체 정리 (아직 보상 X)
                    delete bs;
                    g_MonsterManager.boss = nullptr;
                }

                // 슬라임 분열체 사망 처리 — 1세대는 또 분열, 2세대는 최종 사망
                if (!g_LeakNodes.empty()) {
                    std::vector<Boss*> born;       // 이번 프레임 새로 분열된 자식 (반복 중 push 금지)
                    for (auto* c : g_LeakNodes) {
                        if (c->alive || c->exploded) continue;
                        c->exploded = true;
                        SpawnEnemyExplosion(c->worldX, c->worldY, 0.5f, 0.95f, 0.6f, true);
                        SpawnShockWave(c->worldX, c->worldY, 180.0f, 0.4f, 0.5f, 0.95f, 0.5f);
                        if (c->splitGen < 2) {
                            float hp2 = c->maxHp * 0.46f;
                            float sc2 = c->sizeScale * 0.7f;
                            born.push_back(MakeLeakNode(c->worldX - 30, c->worldY,
                                           hp2, sc2, c->splitGen + 1, screenWidth, screenHeight));
                            born.push_back(MakeLeakNode(c->worldX + 30, c->worldY,
                                           hp2, sc2, c->splitGen + 1, screenWidth, screenHeight));
                        } else {
                            g_GameManager.scoreAccum += 4000.0f;   // 최종 분열체 처치 보너스
                            g_GameManager.score = (long long)g_GameManager.scoreAccum;
                        }
                    }
                    // 죽은 분열체 제거
                    for (auto it = g_LeakNodes.begin(); it != g_LeakNodes.end(); ) {
                        if (!(*it)->alive) { delete *it; it = g_LeakNodes.erase(it); }
                        else ++it;
                    }
                    // 새 자식 합류
                    for (auto* nc : born) g_LeakNodes.push_back(nc);

                    // 전멸 → 인카운터 종료 + 보상 (점수 + 버프 2개)
                    if (g_LeakEncounter && g_LeakNodes.empty()) {
                        g_LeakEncounter = false;
                        g_GameManager.scoreAccum += 20000.0f;
                        g_GameManager.score = (long long)g_GameManager.scoreAccum;
                        g_TotalBossKills++;
                        TryUnlockAch(ACH_FIRST_BOSS);
                        if (g_TotalBossKills >= 3) TryUnlockAch(ACH_BOSS_3);
                        if (g_TotalBossKills >= 10) TryUnlockAch(ACH_BOSS_10);
                        g_BossRewardPicksLeft = 2;
                        g_GameManager.PickAugChoices(g_Stats.sizeAugTaken,
                                                     g_Stats.distAugTaken);
                        g_GameManager.currentState = GameState::AUG_SELECT;
                    }
                }

                // 리로드 러너 사망 → 보상
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
                    g_BossRewardPicksLeft = 2;
                    g_GameManager.PickAugChoices(g_Stats.sizeAugTaken,
                                                 g_Stats.distAugTaken);
                    g_GameManager.currentState = GameState::AUG_SELECT;
                }

                // SPAM.dll 사망 → 보상
                if (g_SpamBoss && !g_SpamBoss->alive && !g_SpamBoss->exploded) {
                    auto* sb = g_SpamBoss;
                    SpawnEnemyExplosion(sb->worldX, sb->worldY, 1.0f, 0.4f, 0.85f, true);
                    SpawnEnemyExplosion(sb->worldX, sb->worldY, 1.0f, 0.8f, 0.3f, true);
                    SpawnShockWave(sb->worldX, sb->worldY, 380.0f, 0.7f, 1.0f, 0.4f, 0.85f);
                    g_ShakeTime = 0.5f; g_ShakeMag = 22.0f;
                    TriggerFlash(1.0f, 0.5f, 0.9f, 0.6f); TriggerHitStop(0.10f);
                    sb->exploded = true;
                    g_GameManager.scoreAccum += 20000.0f;
                    g_GameManager.score = (long long)g_GameManager.scoreAccum;
                    delete sb;
                    g_SpamBoss = nullptr;
                    g_TotalBossKills++;
                    TryUnlockAch(ACH_FIRST_BOSS);
                    if (g_TotalBossKills >= 3) TryUnlockAch(ACH_BOSS_3);
                    g_BossRewardPicksLeft = 2;
                    g_GameManager.PickAugChoices(g_Stats.sizeAugTaken,
                                                 g_Stats.distAugTaken);
                    g_GameManager.currentState = GameState::AUG_SELECT;
                }

                // KERNEL.sys 사망 → 보상
                if (g_KernelBoss && !g_KernelBoss->alive && !g_KernelBoss->exploded) {
                    auto* kb = g_KernelBoss;
                    SpawnEnemyExplosion(kb->worldX, kb->worldY, 1.0f, 0.6f, 0.3f, true);
                    SpawnEnemyExplosion(kb->worldX, kb->worldY, 1.0f, 1.0f, 0.6f, true);
                    SpawnShockWave(kb->worldX, kb->worldY, 460.0f, 0.8f, 1.0f, 0.6f, 0.25f);
                    g_ShakeTime = 0.55f; g_ShakeMag = 24.0f;
                    TriggerFlash(1.0f, 0.7f, 0.3f, 0.6f); TriggerHitStop(0.11f);
                    kb->exploded = true;
                    g_GameManager.scoreAccum += 20000.0f;
                    g_GameManager.score = (long long)g_GameManager.scoreAccum;
                    delete kb;
                    g_KernelBoss = nullptr;
                    g_TotalBossKills++;
                    TryUnlockAch(ACH_FIRST_BOSS);
                    if (g_TotalBossKills >= 3) TryUnlockAch(ACH_BOSS_3);
                    g_BossRewardPicksLeft = 2;
                    g_GameManager.PickAugChoices(g_Stats.sizeAugTaken,
                                                 g_Stats.distAugTaken);
                    g_GameManager.currentState = GameState::AUG_SELECT;
                }

                // FIREWALL.sys 사망 → 보상
                if (g_FirewallBoss && !g_FirewallBoss->alive && !g_FirewallBoss->exploded) {
                    auto* fb = g_FirewallBoss;
                    SpawnEnemyExplosion(fb->worldX, fb->worldY, 1.0f, 0.5f, 0.2f, true);
                    SpawnEnemyExplosion(fb->worldX, fb->worldY, 1.0f, 0.9f, 0.4f, true);
                    SpawnShockWave(fb->worldX, fb->worldY, 440.0f, 0.8f, 1.0f, 0.5f, 0.2f);
                    g_ShakeTime = 0.55f; g_ShakeMag = 24.0f;
                    TriggerFlash(1.0f, 0.6f, 0.25f, 0.6f); TriggerHitStop(0.11f);
                    fb->exploded = true;
                    g_GameManager.scoreAccum += 20000.0f;
                    g_GameManager.score = (long long)g_GameManager.scoreAccum;
                    delete fb;
                    g_FirewallBoss = nullptr;
                    g_TotalBossKills++;
                    TryUnlockAch(ACH_FIRST_BOSS);
                    if (g_TotalBossKills >= 3) TryUnlockAch(ACH_BOSS_3);
                    g_BossRewardPicksLeft = 2;
                    g_GameManager.PickAugChoices(g_Stats.sizeAugTaken,
                                                 g_Stats.distAugTaken);
                    g_GameManager.currentState = GameState::AUG_SELECT;
                }

                // C2_RELAY.sys 사망 → 보상
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
                    g_BossRewardPicksLeft = 2;
                    g_GameManager.PickAugChoices(g_Stats.sizeAugTaken,
                                                 g_Stats.distAugTaken);
                    g_GameManager.currentState = GameState::AUG_SELECT;
                }

                // FORK.worm 사망 → 보상
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
                    g_BossRewardPicksLeft = 2;
                    g_GameManager.PickAugChoices(g_Stats.sizeAugTaken,
                                                 g_Stats.distAugTaken);
                    g_GameManager.currentState = GameState::AUG_SELECT;
                }

                // TOTEM.sys 사망 → 보상
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
                    g_BossRewardPicksLeft = 2;
                    g_GameManager.PickAugChoices(g_Stats.sizeAugTaken,
                                                 g_Stats.distAugTaken);
                    g_GameManager.currentState = GameState::AUG_SELECT;
                }

                // 폴리모프 사망 → 화면 원복 + 증강 3개 + 점수 50% 추가
                if (g_PolyBoss && !g_PolyBoss->alive && !g_PolyBoss->exploded) {
                    auto* pb = g_PolyBoss;
                    SpawnEnemyExplosion(pb->worldX, pb->worldY, 0.6f, 0.2f, 0.9f, true);
                    SpawnEnemyExplosion(pb->worldX, pb->worldY, 0.9f, 0.3f, 1.0f, true);
                    SpawnShockWave(pb->worldX, pb->worldY, 450.0f, 0.8f, 0.7f, 0.3f, 1.0f);
                    g_ShakeTime = 0.6f; g_ShakeMag = 24.0f;
                    TriggerFlash(0.7f, 0.3f, 1.0f, 0.7f); TriggerHitStop(0.13f);
                    pb->exploded = true;
                    g_ViewZoomTarget = 1.0f;               // 화면 정상화
                    g_GameManager.scoreAccum += 30000.0f;  // 다른 보스 +50%
                    g_GameManager.score = (long long)g_GameManager.scoreAccum;
                    delete pb;
                    g_PolyBoss = nullptr;
                    g_PolyPrevForm = -1;
                    g_PolyWasPhase2 = false;
                    g_TotalBossKills++;
                    TryUnlockAch(ACH_FIRST_BOSS);
                    if (g_TotalBossKills >= 3) TryUnlockAch(ACH_BOSS_3);
                    g_BossRewardPicksLeft = 3;             // 증강 1개 더
                    g_GameManager.PickAugChoices(g_Stats.sizeAugTaken,
                                                 g_Stats.distAugTaken);
                    g_GameManager.currentState = GameState::AUG_SELECT;
                }

                // 자폭병 사망 (점화 후 폭발 OR 총알 격파)
                for (auto bm : g_MonsterManager.bombers) {
                    if (!bm->alive && !bm->exploded) {
                        bool blast = bm->arming;
                        // 자폭(blast)은 보상 없음. 플레이어가 죽인 경우(!blast)만,
                        //   그리고 아직 정산 안 됐으면(광역 처치 등) 보상 지급.
                        if (!bm->scored && !blast) {
                            bm->scored = true;
                            creditKill(25.0f + (float)g_Stats.meleeXpBonus, 200.0f);
                        }
                        if (blast) {
                            // 자폭: 플레이어 죽음급 폭발 (다발 + 이중 충격파)
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
                            // 총알 격파: 평범한 폭발 파티클 + 종료 태그
                            SpawnEnemyExplosion(bm->worldX, bm->worldY,
                                                0.9f, 0.4f, 0.4f, true);
                            SpawnKillTag(bm->worldX, bm->worldY, 1.0f, 0.5f, 0.45f,
                                         L"ransomware purged", true);
                        }
                        // HACK_BOMBER: 해킹 폭발 VFX (CollisionSystem 에서 플래그 설정됨)
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
                // VFX 체크 완료 후 죽은 자폭병 메모리 정리
                g_MonsterManager.ClearDeadBombers();

                // 잡몹 근접 데미지로 HP 감소했는지 (전 프레임 비교)
                if (g_GameManager.playerHP < g_PrevHP - 0.0001f) hit = true;
                if (hit && g_Stats.lightStep)
                    g_Stats.lightStepDisableTimer = 10.0f;
                g_PrevHP = g_GameManager.playerHP;

                // 레벨업: xp 가 필요량을 넘으면 레벨업 (남은 xp 이월) + AUG_SELECT
                if (g_GameManager.currentState == GameState::RUNNING) {
                    long long need = g_ExpSystem.Required(g_GameManager.playerLevel);
                    if (g_GameManager.xp >= need) {
                        g_GameManager.xp -= need;
                        ++g_GameManager.playerLevel;
                        // (레벨업 풀스크린 플래시 제거 — 눈부심. AUG_SELECT 카드로 충분히 안내)
                        g_GameManager.PickAugChoices(g_Stats.sizeAugTaken,
                                                     g_Stats.distAugTaken);

                        // 변환 카드 — 25% 확률, 현재 무기와 다른 StartWeapon 으로 전환
                        //   검객/궁수는 총을 안 쓰므로 변환 카드 제외
                        g_ConversionWeapon = -1;
                        if (!g_Stats.meleeWeapon && !g_Stats.bowWeapon &&
                            (rand() % 100) < 25 && g_CurrentWeapon >= 0) {
                            int wc = (int)StartWeapon::_COUNT;
                            int pick = rand() % wc;
                            if (pick == g_CurrentWeapon) pick = (pick + 1) % wc;
                            g_ConversionWeapon = pick;
                        }

                        g_GameManager.currentState = GameState::AUG_SELECT;
                    }
                }

                // 시간 점수
                g_GameManager.AddScore(FIXED_DT * 100.0f);
            }
            accumulator -= FIXED_DT;
        }

        // ── 손맛: 데미지 숫자 / 콤보 / 플래시 매 프레임 갱신 (게임 진행 중에만 감쇠) ──
        if (g_GameManager.currentState == GameState::RUNNING ||
            g_GameManager.currentState == GameState::DYING) {
            for (auto& d : g_DmgNumbers) {
                d.x  += d.vx * delta;
                d.y  += d.vy * delta;
                d.vy += 70.0f * delta;        // 약한 중력 (솟구쳤다 처짐)
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
            if (g_P2ToastTimer > 0.0f) g_P2ToastTimer -= delta;   // 페이즈2 토스트
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

            // ── 피격 펀치 — HP 감소 시 빨간 비네트만 (시야 변동 제거, 창 크기 고정) ──
            {
                if (g_WinPrevHP < 0.0f) g_WinPrevHP = g_GameManager.playerHP;
                float lost = g_WinPrevHP - g_GameManager.playerHP;
                if (lost > 0.5f) { g_HurtVignette = 0.5f; g_HpBarPop = 2.2f; Audio::PlaySfx(Audio::Sfx::Hurt); }   // 피격 비네트 + HP바 팝 + 피격음
                g_WinPrevHP = g_GameManager.playerHP;
                // 창 크기는 g_Stats.windowSize 로 고정 (HP 와 무관)
                g_WindowSizeCur = g_Stats.windowSize;
                playerWin.width = playerWin.height = g_WindowSizeCur;
                playerWin.x = pCX - g_WindowSizeCur * 0.5f;
                playerWin.y = pCY - g_WindowSizeCur * 0.5f;
            }
            if (g_HurtVignette > 0.0f) { g_HurtVignette -= delta * 1.6f; if (g_HurtVignette < 0.0f) g_HurtVignette = 0.0f; }
            if (g_HpBarPop > 0.0f) { g_HpBarPop -= delta; if (g_HpBarPop < 0.0f) g_HpBarPop = 0.0f; }

            // HP 재생 (REGEN_UP, 거대화, 미니화 모두 regenPerSec 에 합산됨)
            if (g_Stats.regenPerSec > 0.0f) {
                g_GameManager.playerHP += g_Stats.regenPerSec * delta;
                if (g_GameManager.playerHP > g_Stats.maxHP)
                    g_GameManager.playerHP = g_Stats.maxHP;
            }
            // 출혈(D_BLEED) — 초당 HP 감소 (재생으로 상쇄 가능, 죽지는 않음 하한 1)
            if (g_Stats.bleedPerSec > 0.0f && g_GameManager.playerHP > 1.0f) {
                g_GameManager.playerHP -= g_Stats.bleedPerSec * delta;
                if (g_GameManager.playerHP < 1.0f) g_GameManager.playerHP = 1.0f;
            }
            // 배드 섹터 출혈 — 감속 구역 안이면 출혈 타이머 2초로 갱신, 빠져나와도 잔류
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
            }

            // 도감 발견 — 원거리/자폭병 (존재하면 발견 처리)
            if (!g_MonsterManager.rangedMobs.empty()) MarkMobSeenId(CM_RANGED);
            if (!g_MonsterManager.bombers.empty())    MarkMobSeenId(CM_BOMBER);

            // 업적 조건 체크 (런 점수/킬/보유 증강 기준 — 보스 업적은 처치 시점에서 처리)
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

            // 충격파 업데이트
            for (auto& sw : g_ShockWaves) {
                if (!sw.active) continue;
                sw.life -= delta;
                if (sw.life <= 0.0f) sw.active = false;
            }

            // 검객 스윙 잔상 업데이트
            for (auto& sl : g_Slashes) {
                if (!sl.active) continue;
                sl.life -= delta;
                if (sl.life <= 0.0f) sl.active = false;
            }

            // 스캔 레이저 빔 페이드
            for (auto& lb : g_LaserBeams) lb.life -= delta;
            g_LaserBeams.erase(std::remove_if(g_LaserBeams.begin(), g_LaserBeams.end(),
                [](const LaserBeam& b){ return b.life <= 0.0f; }), g_LaserBeams.end());

            // 배드 섹터 감속 구역 수명 + 부식 확산
            for (auto& z : g_SlowZones) { z.life -= delta; z.age += delta; }
            g_SlowZones.erase(std::remove_if(g_SlowZones.begin(), g_SlowZones.end(),
                [](const SlowZone& z){ return z.life <= 0.0f; }), g_SlowZones.end());

            // 타격 스파크 업데이트 (이동 + 감속 + 수명)
            for (auto& sp : g_Sparks) {
                sp.x += sp.vx * delta;
                sp.y += sp.vy * delta;
                float drag = std::max(0.0f, 1.0f - 6.0f * delta);
                sp.vx *= drag; sp.vy *= drag;
                sp.life -= delta;
            }
            g_Sparks.erase(std::remove_if(g_Sparks.begin(), g_Sparks.end(),
                [](const Spark& s){ return s.life <= 0.0f; }), g_Sparks.end());
            // 이동 잔상 수명 갱신
            for (auto& tr : g_Trail) tr.life -= delta;
            g_Trail.erase(std::remove_if(g_Trail.begin(), g_Trail.end(),
                [](const Trail& t){ return t.life <= 0.0f; }), g_Trail.end());
            // 머즐 플래시 카운트다운
            if (g_MuzzleTimer > 0.0f) g_MuzzleTimer -= delta;

            // 화면 흔들기 카운트다운
            if (g_ShakeTime > 0.0f) g_ShakeTime -= delta;

            // 초당 EXP 누적 (다가오는 죽음 + 잡몹 가속)
            if (g_Stats.xpPerSec > 0.0f) {
                g_XpTimeAccum += g_Stats.xpPerSec * g_Stats.xpMult * delta;
                if (g_XpTimeAccum >= 1.0f) {
                    long long add = (long long)g_XpTimeAccum;
                    g_GameManager.xp += add;
                    g_XpTimeAccum    -= (float)add;
                }
            }

            // 가벼운 발걸음 비활성 카운트다운
            if (g_Stats.lightStepDisableTimer > 0.0f)
                g_Stats.lightStepDisableTimer -= delta;

            // 취함: drunkCooldown + drunkActiveDuration 사이클 (중복 픽 시 파라미터 변동)
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

            // 폴리모프 2페이즈 = 진짜 최종보스: 모든 적 소환 3배 (보스 죽으면 1배 복귀)
            bool  polyP2  = (g_PolyBoss && g_PolyBoss->phase2);
            float p2mult  = polyP2 ? 3.0f : 1.0f;

            // 점수 기반 난이도 램프 — 점수 오를수록 스폰↑·체력↑·속도↑ (중후반 지루함 방지)
            //   intensity: 10만점=1, 20만점=2 ... 상한 6 (60만점)
            float intensity = (float)g_GameManager.score / 100000.0f;
            if (intensity > 6.0f) intensity = 6.0f;
            float rampSpawn = 1.0f + intensity * 0.40f;   // 스폰 빈도 (너프: 0.55 → 0.40, ×3.4 @60만)
            float rampSpd   = 1.0f + intensity * 0.09f;   // 몹 속도
            // 보스전 중(전조 포함)엔 트래시를 대폭 줄여 보스에 집중 가능하게
            bool  bossNow = g_MonsterManager.boss || g_RRBoss ||
                            g_PolyBoss || g_SpamBoss || g_KernelBoss || g_FirewallBoss ||
                            g_BotnetBoss || g_CentiBoss || g_TotemBoss ||
                            !g_LeakNodes.empty() || g_BossWarnTimer > 0.0f;
            // 몹 체력은 별도로 더 높은 상한까지 계속 증가 — 후반 치명타에 즉사 방지
            //   (스폰/속도는 성능·체감 위해 60만에서 캡, 체력만 140만까지 램프)
            float hpIntensity = (float)g_GameManager.score / 100000.0f;
            if (hpIntensity > 16.0f) hpIntensity = 16.0f;
            float rampHp    = 1.0f + hpIntensity * 0.55f; // 몹 체력 스케일 ↑ (초반 강화 보정용)
            // 특수 잡몹(돌진/회피/거대) 출현 확률 — 점수 비례 (초반 0 → 약 13.5만점에 45% 상한)
            //   D_MOB_FRENZY 디버프 보유 시 배율 ↑ (상한도 비례 확대)
            int   varietyPct = (int)std::min(45.0f * g_Stats.varietyChanceMult,
                                   (float)g_GameManager.score / 3000.0f * g_Stats.varietyChanceMult);
            // 엘리트 변종 확률 — 점수 비례 (초반 0 → 약 14만점에 12% 상한)
            //   D_MOB_ELITE 디버프 보유 시 배율 ↑
            int   elitePct   = (int)std::min(40.0f,
                                   (float)g_GameManager.score / 12000.0f * g_Stats.eliteChanceMult);

            // 스폰 영역 — 2페이즈 줌아웃 시 확장된(보이는) 영역 모서리에서 스폰.
            //   player 이동 클램프와 동일한 [화면/줌] 범위 사용 → 일관됨
            float saX = 0.0f, saY = 0.0f;
            int   saW = screenWidth, saH = screenHeight;
            if (polyP2) {
                float zb = (g_ViewZoom < 0.01f) ? 0.01f : g_ViewZoom;
                saW = (int)((float)screenWidth  / zb);
                saH = (int)((float)screenHeight / zb);
                saX = (float)screenWidth  * 0.5f - saW * 0.5f;
                saY = (float)screenHeight * 0.5f - saH * 0.5f;
            }

            // 잡몹 스폰 (D_MOB_SPAWN → 더 자주 + cap +200, D_MOB_HP → HP+, 점수 램프)
            spawnTimer += delta;
            float spawnInterval = 0.3f * g_Stats.mobSpawnMult / (p2mult * rampSpawn);
            if (bossNow) spawnInterval *= 2.5f;   // 보스전: 트래시 스폰 대폭 감소
            // 쉬움 난이도 — 잡몹 스폰 느리게(이지도 어렵다는 피드백). 체력은 아래 effHpMul 에서 ↓
            float effHpMul = 1.0f;
            if (g_Difficulty == Difficulty::EASY) { spawnInterval *= 1.6f; effHpMul = 0.65f; }
            else if (g_Difficulty == Difficulty::HARD) { effHpMul = 1.1f; }
            // 보스 전면전 — 활성 보스가 있으면 자연 잡몹/원거리/자폭 스폰 완전 정지
            //   (보스가 직접 소환하는 adds 는 각 보스 클래스 내부에서만)
            auto anyBossAlive = [&]() -> bool {
                if (g_MonsterManager.boss && g_MonsterManager.boss->alive) return true;
                if (g_RRBoss && g_RRBoss->alive) return true;
                if (g_PolyBoss && g_PolyBoss->alive) return true;
                if (g_SpamBoss && g_SpamBoss->alive) return true;
                if (g_KernelBoss && g_KernelBoss->alive) return true;
                if (g_FirewallBoss && g_FirewallBoss->alive) return true;
                if (g_BotnetBoss && g_BotnetBoss->alive) return true;
                if (g_CentiBoss && g_CentiBoss->alive) return true;
                if (g_TotemBoss && g_TotemBoss->alive) return true;
                for (auto* s : g_LeakNodes) if (s && s->alive) return true;
                return false;
            };
            bool bossDuel = anyBossAlive() || g_BossWarnTimer > 0.0f;
            if (bossDuel) spawnInterval = 1e9f;
            if (spawnTimer > spawnInterval) {
                // 절대 상한 — 폴리2페이즈×점수램프로 한도가 1000+ 까지 폭주하던 것 방지 (성능)
                int effCap = (int)((100 + g_Stats.mobCapBonus) * p2mult * rampSpawn);
                if (effCap > 180) effCap = 180;
                // 한 마리 스폰 + 디버프 변환 (D_MOB_PACK 시 군집으로 여러 번)
                auto spawnOne = [&]() {
                    size_t mbefore = g_MonsterManager.monsters.size();
                    g_MonsterManager.SpawnMob(screenWidth, screenHeight,
                                              effCap,
                                              g_Stats.monsterHpMult * rampHp * effHpMul, saX, saY, saW, saH,
                                              varietyPct, elitePct);
                    // 디버프 보유 시 일부 몹을 분열체/점멸체로 (특수 잡몹 안 된 경우만)
                    if (g_MonsterManager.monsters.size() > mbefore) {
                        Monster* nm = g_MonsterManager.monsters.back();
                        if (nm->kind == MobKind::NORMAL) {
                            if (g_Stats.splitterMobs && (rand() % 100) < 25)
                                nm->MakeKind(MobKind::SPLITTER, 0, 1.2f);
                            else if (g_Stats.blinkerMobs && (rand() % 100) < 25)
                                nm->MakeKind(MobKind::BLINKER);
                            else if (g_Stats.orbiterMobs && (rand() % 100) < 22)
                                nm->MakeKind(MobKind::ORBITER);
                            else if (g_Stats.spawnerMobs && (rand() % 100) < 14) {
                                nm->MakeKind(MobKind::SPAWNER);
                                // 플레이어를 쫓지 않게 — 화면 안 임의 지점을 배치 목표로 (가장자리 여백 안)
                                nm->blinkTargetX = 160.0f + (float)(rand() % (screenWidth  > 360 ? screenWidth  - 320 : 1));
                                nm->blinkTargetY = 160.0f + (float)(rand() % (screenHeight > 360 ? screenHeight - 320 : 1));
                            }
                            else if (g_Stats.shieldedMobs && (rand() % 100) < 22)
                                nm->MakeKind(MobKind::SHIELDED);
                            // 배드 섹터(M) — 디버프 보유 시 흔함, 자연 스폰은 매우 낮음. 50만점 넘으면 미등장.
                            else if (g_GameManager.score < 500000 &&
                                     ((g_Stats.badsectorMobs && (rand() % 100) < 12) ||
                                      (rand() % 1000) < 5))
                                nm->MakeKind(MobKind::BADSECTOR);
                            // 레지스트리 에러(M) — 더 희귀. 50만점 넘으면 미등장.
                            else if (g_GameManager.score < 500000 &&
                                     ((g_Stats.regerrorMobs && (rand() % 100) < 8) ||
                                      (rand() % 1000) < 2))
                                nm->MakeKind(MobKind::REGERROR);
                            // 디도스 — 22만점 이후·보스전 제외. 1마리 위주, cap 10~18%.
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
                        // 스케쥴러 강화 — 특수(비-NORMAL) 잡몹 HP 추가 배율
                        if (nm->kind != MobKind::NORMAL && g_Stats.specialMobHpMult != 1.0f)
                            nm->hp *= g_Stats.specialMobHpMult;
                        // 트로이목마 강화 — 점멸체(트로이목마) 점멸 재사용시간 단축
                        if (nm->kind == MobKind::BLINKER && g_Stats.trojanBoost)
                            nm->blinkIntervalMul = 0.55f;
                    }
                };
                int packN = 1 + g_Stats.mobPackBonus;       // D_MOB_PACK: 군집 스폰
                for (int p = 0; p < packN; p++) spawnOne();
                spawnTimer = 0.0f;
            }

            // 게임 시간 카운트
            g_GameTime += delta;

            // 보스 스폰 — 일반 보스 20만점마다 / 폴리모프 50만점 고정.
            //   (어떤 보스든 살아있으면 대기 = 동시 스폰 방지)
            {
                bool bossActive = g_MonsterManager.boss ||
                                  g_RRBoss || g_PolyBoss || g_SpamBoss || g_KernelBoss ||
                                  g_FirewallBoss || g_BotnetBoss || g_CentiBoss || g_TotemBoss ||
                                  !g_LeakNodes.empty() || g_BossWarnTimer > 0.0f;
                // 라운드2 — 보스 눈덩이 차단: 보스를 잡아 완전히 정리되는 순간(활성→비활성),
                //   다음 보스 임계값을 현재 점수+20만으로 리베이스 → 최소 20만점 휴식 보장
                //   (보스전 중 쌓인 점수로 잡자마자 또 보스 뜨던 악순환 제거).
                static bool s_prevBossActive = false;
                if (s_prevBossActive && !bossActive) {
                    long long rebase = (long long)g_GameManager.score + 200000;
                    if (g_NextBossScore < rebase) g_NextBossScore = rebase;
                }
                s_prevBossActive = bossActive;
                float bossHpC = GetDifficultyParams(g_Difficulty).bossHp;
                float polyHpC = (g_Difficulty == Difficulty::EASY) ? 10000.0f
                              : (g_Difficulty == Difficulty::HARD) ? 75000.0f : 30000.0f;

                // 전조 시작 — pick·이름·HP 확정 후 2.5초 경고. 실제 생성은 만료 시.
                auto startWarn = [&](int pick, const wchar_t* name, float hp) {
                    g_BossWarnPick = pick; g_BossWarnName = name;
                    g_BossWarnHp = hp;     g_BossWarnTimer = BOSS_WARN_DUR;
                };

                if (!bossActive) {
                    if (g_CreativeBossPending) {
                        // 크리에이티브: 선택한 보스 (점수 무관, 1회)
                        g_CreativeBossPending = false;
                        switch (g_CreativeBossPick) {
                        case 0:  startWarn(0, L"LEAK.sys",    bossHpC);         break;
                        case 2:  startWarn(2, L"VOLLEY.sys",  bossHpC);         break;
                        case 3:  startWarn(3, L"SPAM.dll",      bossHpC * 0.65f); break;
                        case 4:  startWarn(4, L"POLYMORPH.vir", polyHpC);         break;
                        case 5:  startWarn(5, L"KERNEL.sys",    bossHpC * 0.45f); break;  // DPS체크 — 자가붕괴 보정 위해 HP↓
                        case 6:  startWarn(6, L"FIREWALL.sys",  bossHpC * 0.7f);  break;
                        case 7:  startWarn(7, L"C2_RELAY.sys",    bossHpC * 0.75f); break;
                        case 8:  startWarn(8, L"FORK.worm",        bossHpC * 0.7f);  break;
                        case 9:  startWarn(9, L"RITE.CORE",        bossHpC * 0.72f); break;
                        default: startWarn(4, L"POLYMORPH.vir",   polyHpC);        break;
                        }
                    }
                    else if (g_GameManager.score >= g_NextBossScore) {
                        // 20만점마다 로테이션 (50만 1회 폴리 고정)
                        g_NextBossScore += 200000;
                        float sc = 0.36f + (float)g_GameManager.score / 300000.0f;
                        if (sc > 9.0f) sc = 9.0f;
                        sc *= (1.0f + (float)g_GameManager.playerLevel * 0.022f);
                        float bossHp = GetDifficultyParams(g_Difficulty).bossHp * sc;
                        if (!g_PolySpawned && g_GameManager.score >= 500000) {
                            g_PolySpawned = true;
                            startWarn(4, L"POLYMORPH.vir", polyHpC);
                        } else {
                            int pick = BossDir::RollScorePick();
                            startWarn(pick, BossDir::DisplayName(pick),
                                      bossHp * BossDir::HpMul(pick));
                        }
                    }
                }

                // 전조 진행 → 만료 시 실제 보스 생성 + 등장 연출
                if (g_BossWarnTimer > 0.0f) {
                    g_BossWarnTimer -= delta;
                    if (g_BossWarnTimer <= 0.0f) {
                        g_BossWarnTimer = 0.0f;
                        // C15: 소환 위치 랜덤화 — 항상 중앙 근처라 거기서 캠핑/샷건 파훼되던 문제.
                        //   화면 안쪽(가장자리 마진 제외) 전 영역에서 무작위 위치.
                        float bMargin = 340.0f * g_Scale;
                        float bRangeX = std::max(1.0f, (float)screenWidth  - 2.0f * bMargin);
                        float bRangeY = std::max(1.0f, (float)screenHeight - 2.0f * bMargin);
                        float bsx = bMargin + (float)(rand() % (int)bRangeX);
                        float bsy = bMargin + (float)(rand() % (int)bRangeY);
                        switch (g_BossWarnPick) {
                        case 0:
                            g_MonsterManager.boss =
                                new Boss(bsx, bsy, screenWidth, screenHeight, g_BossWarnHp);
                            break;
                        case 2:
                            g_RRBoss = new ReloadRunnerBoss(screenWidth, screenHeight, g_BossWarnHp);
                            g_RRBoss->worldX = bsx; g_RRBoss->worldY = bsy;
                            break;
                        case 3:
                            g_SpamBoss = new SpamBoss(screenWidth, screenHeight, g_BossWarnHp);
                            g_SpamBoss->worldX = g_SpamBoss->baseX = bsx;
                            g_SpamBoss->worldY = g_SpamBoss->baseY = bsy;
                            break;
                        case 5:
                            g_KernelBoss = new KernelBoss(screenWidth, screenHeight, g_BossWarnHp);
                            g_KernelBoss->worldX = bsx; g_KernelBoss->worldY = bsy;
                            break;
                        case 6:
                            g_FirewallBoss = new FirewallBoss(screenWidth, screenHeight, g_BossWarnHp);
                            g_FirewallBoss->worldX = bsx; g_FirewallBoss->worldY = bsy;
                            break;
                        case 7:
                            g_BotnetBoss = new BotnetBoss(screenWidth, screenHeight, g_BossWarnHp);
                            g_BotnetBoss->worldX = bsx; g_BotnetBoss->worldY = bsy;
                            // 순수 전면전 — 잡몹 흡수 + 초기 패킷 러시
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
                            // ── 프로토타입: 현재 잡몹 전체 체력을 흡수(상한) → 순수 보스전 ──
                            //   잡몹은 보스로 흡수되어 사라지고, 보스전 동안 추가 스폰 안 됨.
                            {
                                float absorb = 0.0f;
                                for (auto* m  : g_MonsterManager.monsters)   if (m->alive)  absorb += m->hp;
                                for (auto* r  : g_MonsterManager.rangedMobs)  if (r->alive)  absorb += r->hp;
                                for (auto* bm : g_MonsterManager.bombers)     if (bm->alive) absorb += bm->hp;
                                // 상한 제거 — 진짜 '잡몹 전체 체력 + 보스 체력'. 후반 탱커 잡몹
                                //   horde 가 많을수록 보스도 그만큼 단단(잡몹보다 빨리 죽는 문제 해결).
                                g_CentiBoss->hp += absorb; g_CentiBoss->maxHp += absorb;
                                // 잡몹 흡수 — 화면 정리(순수 듀얼)
                                for (auto* m  : g_MonsterManager.monsters)   delete m;
                                g_MonsterManager.monsters.clear();
                                for (auto* r  : g_MonsterManager.rangedMobs)  delete r;
                                g_MonsterManager.rangedMobs.clear();
                                for (auto* bm : g_MonsterManager.bombers)     delete bm;
                                g_MonsterManager.bombers.clear();
                                g_ShakeTime = 0.4f; g_ShakeMag = 14.0f;   // 흡수 순간 진동
                            }
                            g_CentiBoss->enterSpawn();   // 등장 모션 — 화면 밖에서 곡선 돌진으로 입장
                            break;
                        case 9:
                            g_TotemBoss = new TotemBoss(screenWidth, screenHeight, g_BossWarnHp);
                            g_TotemBoss->worldX = bsx; g_TotemBoss->worldY = bsy;
                            break;
                        default:
                            g_PolyBoss = new PolymorphBoss(screenWidth, screenHeight, g_BossWarnHp);
                            g_PolyBoss->worldX = bsx; g_PolyBoss->worldY = bsy;
                            g_PolyPrevForm = -1;
                            break;
                        }
                        // 공통 등장 연출 — 큰 화면 흔들기 + 충격파 + 빵빵 폭발
                        //   FORK.worm 은 화면 밖에서 곡선 돌진으로 '들어오는' 등장이라, 중앙 폭발이
                        //   위치상 안 맞음 → 흔들기만 두고 중앙 파티클은 생략(자체 입장 연출 사용).
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

            // 자폭병 spawn (난이도별 시작 시간/주기, 쉬움은 안 나옴)
            DifficultyParams dp = GetDifficultyParams(g_Difficulty);
            if (g_GameTime >= dp.bomberStartTime && !bossDuel) {
                g_BomberSpawnTimer += delta;
                float bomberInt = dp.bomberInterval / (p2mult * rampSpawn);
                if (bossNow) bomberInt *= 2.0f;   // 보스전: 자폭병도 덜 나오게
                if (g_BomberSpawnTimer >= bomberInt) {
                    g_MonsterManager.SpawnBomber(screenWidth, screenHeight,
                                                 g_Stats.bomberHpMult * rampHp,
                                                 g_Stats.bomberSpeedMult,
                                                 g_Stats.bomberBlastMult,
                                                 saX, saY, saW, saH);
                    g_BomberSpawnTimer = 0.0f;
                }
            }

            // 원거리 몹 스폰 — 난이도별 + 디버프 (D_RMOB_MAX, rmobSpawnDelayBonus)
            rangedSpawnTimer += delta;
            float rangedInterval = dp.rangedSpawnInterval - g_Stats.rmobSpawnDelayBonus;
            if (rangedInterval < 1.0f) rangedInterval = 1.0f;   // 최소 1초
            rangedInterval /= (p2mult * rampSpawn);              // 2페이즈 + 점수 램프
            if (bossNow) rangedInterval *= 2.0f;                 // 보스전: 원거리몹도 덜
            int rangedMax = (int)((dp.rangedMaxBase + g_Stats.rmobMaxBonus) * p2mult
                                  + intensity * 2.0f);           // 점수당 동시 +2
            if (rangedMax > 16) rangedMax = 16;   // 창 개수 = scissor 패스 수 → 상한 (성능)
            if (rangedSpawnTimer > rangedInterval && !bossDuel) {
                g_MonsterManager.SpawnRangedMob(screenWidth, screenHeight,
                    g_Stats.rmobHpMult * rampHp, rangedMax, saX, saY, saW, saH);
                rangedSpawnTimer = 0.0f;
            }

            // 폴리모프 2페이즈 전용 — 7초마다 플레이어 주변에 원거리/자폭병 5마리
            //   (스폰 제한 무시 — 벡터에 직접 push)
            if (polyP2) {
                g_PolySummonTimer += delta;
                if (g_PolySummonTimer >= 7.0f) {
                    g_PolySummonTimer = 0.0f;
                    float pCX = playerWin.x + playerWin.width  * 0.5f;
                    float pCY = playerWin.y + playerWin.height * 0.5f;
                    for (int k = 0; k < 5; k++) {
                        float ang = (float)k / 5.0f * 6.2831853f
                                  + (float)(rand() % 100) * 0.01f;
                        float rad = 280.0f + (float)(rand() % 160);
                        float sx = pCX + cosf(ang) * rad;
                        float sy = pCY + sinf(ang) * rad;
                        if (rand() % 2 == 0) {
                            if ((int)g_MonsterManager.rangedMobs.size() >= 16) continue;
                            RangedMob* rm = new RangedMob(sx, sy, screenWidth, screenHeight);
                            rm->hp *= g_Stats.rmobHpMult;
                            g_MonsterManager.rangedMobs.push_back(rm);
                        } else {
                            if ((int)g_MonsterManager.bombers.size() >= 30) continue;
                            g_MonsterManager.bombers.push_back(
                                new Bomber(sx, sy, g_Stats.bomberHpMult,
                                           g_Stats.bomberSpeedMult, g_Stats.bomberBlastMult));
                        }
                    }
                    // 소환 연출
                    SpawnShockWave(pCX, pCY, 320.0f, 0.5f, 0.7f, 0.3f, 1.0f);
                }
            }

            // 다가오는 죽음: 모든 오브 영원히 추격 + 접촉 데미지
            // 스택 1회 = 기본 속도, 2회 이상부터 +20%/스택
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

            // 드론: 플레이어 주위 공전 + 자동 발사 (능력치 50%), 1~2기
            // 포탑 모드 활성 시 드론은 공전/발사 않고 포탑으로 대체됨
            if (g_Stats.drone && !g_Stats.turretMode) {
                for (int d = 0; d < g_Stats.droneCount && d < MAX_DRONES; d++) {
                    auto& dr = g_Drones[d];
                    dr.angle += 1.5f * delta;
                    // 균등 각도 오프셋 (각자 다른 위치)
                    float ang = dr.angle + (float)d * 6.2831853f / (float)g_Stats.droneCount;
                    float droneX = pCX + cosf(ang) * 80.0f;
                    float droneY = pCY + sinf(ang) * 80.0f;
                    dr.fireTimer += delta;
                    // 군집 지능(신화) — 발사 간격 2.0× → 0.6× (초고속)
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

            // 포탑 배치 (CANNON + DRONE_2 조합)
            //   1초마다 플레이어 위치에 포탑 1개 배치, 각 5초 지속 → 맵에 ~5개 상시
            //   능력치는 '소총' 기준(g_TurretStats) — 대포 ×5/1초고정 미적용
            if (g_Stats.turretMode) {
                // 소총 기준 능력치 재계산 (보유 증강 개수 변할 때만)
                static size_t s_lastTurretAugCount = (size_t)-1;
                if (g_OwnedAugs.size() != s_lastTurretAugCount) {
                    s_lastTurretAugCount = g_OwnedAugs.size();
                    g_TurretStats = PlayerStats();
                    ApplyWeapon(g_TurretStats, StartWeapon::RIFLE);
                    for (int oi : g_OwnedAugs) g_TurretStats.Apply(ALL_AUGS[oi].type);
                }

                // 1초마다 새 포탑 배치 (플레이어 위치)
                g_TurretDeployTimer += delta;
                if (g_TurretDeployTimer >= TURRET_DEPLOY) {
                    g_TurretDeployTimer -= TURRET_DEPLOY;
                    if ((int)g_Turrets.size() < MAX_TURRETS) {
                        Turret t; t.x = pCX; t.y = pCY;
                        g_Turrets.push_back(t);
                    }
                }

                // 각 포탑: 수명 + 소총 발사
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
                        if (g_MonsterManager.boss && g_MonsterManager.boss->alive) {
                            auto* bs2 = g_MonsterManager.boss;
                            float ddx = bs2->worldX - t.x, ddy = bs2->worldY - t.y;
                            float ds  = ddx*ddx + ddy*ddy;
                            if (ds < nd2) { nd2 = ds; ttx = bs2->worldX; tty = bs2->worldY; }
                        }
                        if (nd2 < 1e8f) {
                            float dist = sqrtf(nd2);
                            Bullet nb(t.x, t.y, ttx, tty);
                            nb.speed     = tSpeed;
                            nb.color     = glm::vec3(0.1f, 1.0f, 0.55f);  // 청록 (포탑 고유색)
                            nb.turretDmg = g_TurretStats.GetBaseDamage()
                                         * g_TurretStats.GetDamageMultiplier(dist);
                            if (nb.turretDmg < 1.0f) nb.turretDmg = 1.0f;
                            g_Bullets.push_back(nb);
                        }
                    }
                }
                // 만료된 포탑 제거
                g_Turrets.erase(
                    std::remove_if(g_Turrets.begin(), g_Turrets.end(),
                        [](const Turret& t){ return t.lifeTimer >= TURRET_LIFE; }),
                    g_Turrets.end());
            }

            // 차크람: 공전 + 잡몹 즉사 + 적 총알 충돌 + 재생성 (n 개)
            if (g_Stats.chakram) {
                for (int c = 0; c < g_Stats.chakramCount && c < MAX_CHAKRAMS; c++) {
                    auto& ch = g_Chakrams[c];
                    if (ch.alive) {
                        ch.angle += 5.5f * delta;
                        float chx = pCX + cosf(ch.angle) * CHAKRAM_RADIUS;
                        float chy = pCY + sinf(ch.angle) * CHAKRAM_RADIUS;
                        float hitR2 = CHAKRAM_SIZE * CHAKRAM_SIZE;   // 버프: 큰 히트박스
                        // 잡몹 접촉 — 즉사 (hp 소모 -2 로 완화 → 오래 버팀)
                        for (auto m : g_MonsterManager.monsters) {
                            if (!m->alive) continue;
                            float ddx = m->worldX - chx, ddy = m->worldY - chy;
                            if (ddx*ddx + ddy*ddy < hitR2) {
                                m->alive = false;
                                ch.hp -= 2.0f;
                            }
                        }
                        // 자폭병 접촉 — 즉사(자폭 전 처리)
                        for (auto bm : g_MonsterManager.bombers) {
                            if (!bm->alive) continue;
                            float ddx = bm->worldX - chx, ddy = bm->worldY - chy;
                            if (ddx*ddx + ddy*ddy < hitR2) { bm->hp = 0.0f; bm->alive = false; ch.hp -= 3.0f; }
                        }
                        // 원거리 몹 접촉 — 큰 피해
                        for (auto rr : g_MonsterManager.rangedMobs) {
                            if (!rr->alive) continue;
                            float ddx = rr->worldX - chx, ddy = rr->worldY - chy;
                            if (ddx*ddx + ddy*ddy < hitR2) { rr->hp -= 120.0f; if (rr->hp <= 0) rr->alive = false; ch.hp -= 3.0f; }
                        }
                        // 적 총알 막기
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
                            ch.respawnTimer = 6.0f;   // 버프: 재생성 10 → 6초
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

            // 탄환 세례: 쿨다운(20/15/7.5)마다 유도탄 20발 (데미지 50%)
            if (g_Stats.bulletRain) {
                g_BulletRainTimer += delta;
                // 무한 세례(신화) — 처치마다 쿨다운 진행 가속(0.4s/처치). 미보유 시 처치 카운트만 비움.
                if (g_Stats.rainKillReduce) g_BulletRainTimer += g_RainKillAccum * 0.4f;
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
                        nb.speed      = g_Stats.bulletSpeed * 1.05f;  // 최고속(가속 끝) — SAM 미사일식
                        nb.color      = glm::vec3(1.0f, 0.5f, 0.2f);
                        nb.homing     = true;
                        nb.homingTurn = 5.0f;   // rad/s
                        nb.dmgMult    = 0.5f;
                        // 지대공 미사일 발사 느낌 — 5%에서 출발해 ~0.7초에 100%로 가속
                        nb.launchRamp  = 0.05f;
                        nb.launchAccel = 1.35f;
                        // (탄환세례 — 기존 일반 탄환 렌더로 복원. 로켓 스프라이트 미사용)
                        g_Bullets.push_back(nb);
                    }
                }
            }

            // 고장난 조준선 오브 배회
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

            // 총알 발사 (영혼수확/미니화 보너스, Twin 2발, Cannon 잔존 데미지 캐싱)
            // 포탑 모드: 플레이어 수동 발사 비활성 (포탑이 대신 발사)
            float effInterval = g_Stats.fireInterval * g_Stats.GetFireIntervalMult();
            // 대포: 연사 %는 공격력으로만 환산되고, 실제 발사는 1초 고정
            if (g_Stats.cannon) effInterval = 1.0f;
            // 과부하 — 연사 ×2 (발사 간격 절반)
            if (g_OverclockTimer > 0.0f) effInterval *= 0.5f;
            float effSpeed    = g_Stats.bulletSpeed   + g_Stats.GetBulletSpeedBonus();
            if (!g_Stats.turretMode) fireTimer += delta;

            // 한 발 spawn 헬퍼 (Twin / Cannon / 취함 / brokenSight 공통)
            // bulletSpread > 0 면 발사 방향에 랜덤 흔들기 적용
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
                    nb.sizeScale    = 3.0f;  // 대포 총알 크기 3배
                }
                // 연쇄 작용(리코셰) — 튕김 횟수 + 데미지 배율(-30%) 적용
                if (g_Stats.ricochetMax > 0) {
                    nb.bouncesLeft = g_Stats.ricochetMax;
                    nb.dmgMult    *= g_Stats.ricochetDmgMult;
                }
                if (g_OverclockTimer > 0.0f) {     // 과부하 — 공격력 +50%
                    nb.dmgMult *= 1.5f;
                    if (nb.remainingDmg > 0.0f) nb.remainingDmg *= 1.5f;
                }
                if (g_Stats.berserk) {             // 광전사 — 체력 낮을수록 최대 +60%
                    float hpFrac = g_Stats.maxHP > 0.0f
                                 ? g_GameManager.playerHP / g_Stats.maxHP : 1.0f;
                    if (hpFrac < 0.0f) hpFrac = 0.0f; else if (hpFrac > 1.0f) hpFrac = 1.0f;
                    float bMult = 1.0f + (1.0f - hpFrac) * 0.6f;
                    nb.dmgMult *= bMult;
                    if (nb.remainingDmg > 0.0f) nb.remainingDmg *= bMult;
                }
                if (g_Stats.bowWeapon) {           // 궁수 — 화살 비주얼 (길쭉·갈색녹)
                    nb.color     = glm::vec3(0.75f, 0.95f, 0.45f);
                    nb.sizeScale = 1.7f;
                }
                if (g_DrunkActive) {               // 취함 활성 — 조준 흐트러짐 + 데미지 -40%
                    nb.dmgMult *= 0.6f;
                    if (nb.remainingDmg > 0.0f) nb.remainingDmg *= 0.6f;
                }
                g_Bullets.push_back(nb);
            };

            // Twin: ±5도 2발 / Shotgun: 5발 산탄 (사거리 700)
            auto spawnAimed = [&](float tx, float ty) {
                TriggerMuzzle(pCX, pCY, atan2f(ty - pCY, tx - pCX));  // 총구 섬광
                Audio::PlaySfx(Audio::Sfx::Shoot);                    // 발사음
                if (g_Stats.shotgun) {
                    float dx = tx - pCX, dy = ty - pCY;
                    float ang = atan2f(dy, dx);
                    const int N = 5;
                    float spread = 0.42f;  // 전체 spread ≈ ±12도
                    float r = 200.0f;
                    for (int s = 0; s < N; s++) {
                        float t = (float)s / (float)(N - 1); // 0..1
                        float off = (t - 0.5f) * spread;
                        float a = ang + off;
                        // 직접 spawn (maxRange 적용)
                        Bullet nb(pCX, pCY,
                                  pCX + cosf(a) * r, pCY + sinf(a) * r);
                        nb.speed    = effSpeed;
                        nb.maxRange = 700.0f;
                        if (g_Stats.cannon) {
                            nb.remainingDmg = g_Stats.GetBaseDamage()
                                            * g_Stats.GetDamageMultiplier(0.0f);
                            nb.sizeScale    = 5.0f;
                        }
                        if (g_Stats.ricochetMax > 0) {     // 연쇄 작용 — 샷건 펠릿도
                            nb.bouncesLeft = g_Stats.ricochetMax;
                            nb.dmgMult    *= g_Stats.ricochetDmgMult;
                        }
                        if (g_OverclockTimer > 0.0f) {     // 과부하 +50%
                            nb.dmgMult *= 1.5f;
                            if (nb.remainingDmg > 0.0f) nb.remainingDmg *= 1.5f;
                        }
                        if (g_Stats.berserk) {             // 광전사 — 체력 낮을수록 최대 +60%
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
                        if (g_DrunkActive) {               // 취함 활성 — 데미지 -40%
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
                    int   n = (g_Stats.twinCount < 2) ? 2 : g_Stats.twinCount;  // 2발(더블)/3발(트리플)
                    for (int s = 0; s < n; s++) {
                        // 중심 기준 대칭 부채꼴 (-off … +off)
                        float a = (n > 1) ? ang + (((float)s / (float)(n - 1)) - 0.5f) * (2.0f * off)
                                          : ang;
                        spawnOne(pCX + cosf(a) * r, pCY + sinf(a) * r);
                    }
                } else {
                    spawnOne(tx, ty);
                }
            };

            // 검객 근접 스윙 — 조준 방향 호(arc) 안의 모든 적에게 즉시 피해
            auto meleeSwing = [&](float ang) {
                float range = 190.0f * g_Stats.playerSizeMult * (g_Stats.meleeWide ? 1.25f : 1.0f);
                float r2 = range * range;
                float halfArc = g_Stats.meleeWide ? 1.5f : 1.15f;  // 광폭 베기: 호 확대
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
                          * 1.8f * critMult * bMult;       // 근접 보너스 ×1.8
                if (g_TotemBoss && g_TotemBoss->alive) dmg *= g_TotemBoss->statDamageMult();
                auto inCone = [&](float ex, float ey) -> bool {
                    float dx = ex - pCX, dy = ey - pCY, d2 = dx*dx + dy*dy;
                    if (d2 > r2) return false;
                    float diff = atan2f(dy, dx) - ang;
                    while (diff >  3.14159265f) diff -= 6.2831853f;
                    while (diff < -3.14159265f) diff += 6.2831853f;
                    return fabsf(diff) <= halfArc;
                };
                auto onKill = [&]() {   // 흡혈탄/흡혈마 공통 처리
                    if (g_Stats.lifestealPerKill > 0.0f) {
                        g_GameManager.playerHP += g_Stats.lifestealPerKill;
                        if (g_GameManager.playerHP > g_Stats.maxHP)
                            g_GameManager.playerHP = g_Stats.maxHP;
                    }
                    if (g_Stats.vampire && ++g_Stats.vampireKillStreak >= 10) {
                        g_Stats.vampireKillStreak = 0;
                        g_GameManager.playerHP += 1.0f;
                        if (g_GameManager.playerHP > g_Stats.maxHP)
                            g_GameManager.playerHP = g_Stats.maxHP;
                    }
                };
                std::vector<Monster*> swingBorn;
                for (auto m : g_MonsterManager.monsters) {        // 잡몹
                    if (!m->alive || !inCone(m->worldX, m->worldY)) continue;
                    float dmgM = dmg;
                    if (m->kind == MobKind::SHIELDED && m->shieldActive) dmgM *= 0.15f;
                    float dealt = (dmgM < m->hp) ? dmgM : m->hp; m->hp -= dealt;
                    SpawnDamageNumber(m->worldX, m->worldY, dealt, dealt >= 40.0f || crit);
                    if (m->hp <= 0.0f) {
                        m->alive = false; m->scored = true; AddKillCombo();
                        float bx, bs; MobKillReward(m->kind, m->splitGen, m->elite, bx, bs);
                        g_GameManager.xp += (long long)((bx + (float)g_Stats.meleeXpBonus) * g_Stats.xpMult);
                        g_Stats.killCount++; g_GameManager.scoreAccum += bs;
                        g_GameManager.score = (long long)g_GameManager.scoreAccum;
                        SpawnWormSplit(m, swingBorn);
                        SpawnBadSectorZone(m);
                        onKill();
                    }
                }
                for (auto* nb : swingBorn) g_MonsterManager.monsters.push_back(nb);
                for (auto rr : g_MonsterManager.rangedMobs) {     // 원거리
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
                for (auto bm : g_MonsterManager.bombers) {        // 자폭병
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
                // 보스류 — hp만 감소 (보상/연출은 각 사망 블록이 담당)
                auto hitB = [&](float ex, float ey, float& hp, bool& al) {
                    if (!inCone(ex, ey)) return;
                    float dealt = (dmg < hp) ? dmg : hp; hp -= dealt;
                    SpawnDamageNumber(ex, ey, dealt, dealt >= 40.0f || crit);
                    if (hp <= 0.0f) al = false;
                };
                if (g_MonsterManager.boss && g_MonsterManager.boss->alive)
                    hitB(g_MonsterManager.boss->worldX, g_MonsterManager.boss->worldY,
                         g_MonsterManager.boss->hp, g_MonsterManager.boss->alive);
                for (auto* c : g_LeakNodes) if (c->alive)
                    hitB(c->worldX, c->worldY, c->hp, c->alive);
                if (g_RRBoss && g_RRBoss->alive)
                    hitB(g_RRBoss->worldX, g_RRBoss->worldY, g_RRBoss->hp, g_RRBoss->alive);
                if (g_PolyBoss && g_PolyBoss->alive && g_PolyBoss->damageable())
                    hitB(g_PolyBoss->worldX, g_PolyBoss->worldY, g_PolyBoss->hp, g_PolyBoss->alive);
                if (g_SpamBoss && g_SpamBoss->alive)
                    hitB(g_SpamBoss->worldX, g_SpamBoss->worldY, g_SpamBoss->hp, g_SpamBoss->alive);
                if (g_KernelBoss && g_KernelBoss->alive)
                    hitB(g_KernelBoss->worldX, g_KernelBoss->worldY, g_KernelBoss->hp, g_KernelBoss->alive);
                if (g_FirewallBoss && g_FirewallBoss->alive)
                    hitB(g_FirewallBoss->worldX, g_FirewallBoss->worldY, g_FirewallBoss->hp, g_FirewallBoss->alive);
                if (g_BotnetBoss && g_BotnetBoss->alive)
                    hitB(g_BotnetBoss->worldX, g_BotnetBoss->worldY, g_BotnetBoss->hp, g_BotnetBoss->alive);
                if (g_CentiBoss && g_CentiBoss->alive && g_CentiBoss->vulnerable())
                    hitB(g_CentiBoss->worldX, g_CentiBoss->worldY, g_CentiBoss->hp, g_CentiBoss->alive);
                if (g_TotemBoss && g_TotemBoss->alive && g_TotemBoss->vulnerable())
                    hitB(g_TotemBoss->worldX, g_TotemBoss->worldY, g_TotemBoss->hp, g_TotemBoss->alive);
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
                // 칼바람 — 스윙마다 전방으로 관통 투사체 (근접의 원거리 견제)
                if (g_Stats.bladeWind) {
                    Bullet bw(pCX, pCY, pCX + cosf(ang)*200.0f, pCY + sinf(ang)*200.0f);
                    bw.speed       = 900.0f;
                    bw.maxRange    = 700.0f;
                    bw.sizeScale   = 2.2f;
                    bw.color       = glm::vec3(0.7f, 0.95f, 1.0f);
                    bw.remainingDmg = dmg * 0.6f;   // 관통(대포식) — 근접 데미지의 60%
                    g_Bullets.push_back(bw);
                }
            };

            // ── 조준 타깃 헬퍼: 좌클릭=커서 일점사, 자동(클릭X)=최근접 적. 자동인데 적 없으면 false.
            auto aimTarget = [&](float& tx, float& ty) -> bool {
                if (lmb) { tx = wmx; ty = wmy; return true; }       // 일점사
                return findNearestEnemy(pCX, pCY, tx, ty);          // 드론식 자동조준
            };

            // ── 스캔 레이저 (증강) — 0.7초마다 조준 방향 관통 빔 (군중제어) ──
            //   meleeSwing 의 데미지/처치보상 루프를 '직선 판정(SegDist)' 버전으로 재사용.
            if (g_Stats.laser) {
                float laserInt = (g_Stats.laserTier >= 3) ? 0.18f   // 신화 수렴: 거의 연속
                               : (g_Stats.laserTier >= 2) ? 0.55f : LASER_INT;
                g_LaserTimer += delta;
                if (g_LaserTimer >= laserInt) {
                    g_LaserTimer -= laserInt;
                    float lang  = atan2f(wmy - pCY, wmx - pCX);   // 레이저는 항상 커서 방향
                    // 사거리는 II(760)에서 더 늘리지 않음. 신화 수렴은 '너비'로 강화.
                    float LASER_RANGE = (g_Stats.laserTier >= 2) ? 760.0f : 560.0f;
                    float lex = pCX + cosf(lang) * LASER_RANGE, ley = pCY + sinf(lang) * LASER_RANGE;
                    // 신화 수렴(tier3): 빔 너비 2배 → 광폭 관통 (잡몹 라인 일소)
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
                               * 1.4f * lcm * lbm;   // 너프: 2.5 → 1.4 (잡몹 정리용, 보스 칩 최소)
                    if (g_TotemBoss && g_TotemBoss->alive) ldmg *= g_TotemBoss->statDamageMult();
                    auto lOnKill = [&]() {
                        if (g_Stats.lifestealPerKill > 0.0f) {
                            g_GameManager.playerHP += g_Stats.lifestealPerKill;
                            if (g_GameManager.playerHP > g_Stats.maxHP) g_GameManager.playerHP = g_Stats.maxHP;
                        }
                        if (g_Stats.vampire && ++g_Stats.vampireKillStreak >= 10) {
                            g_Stats.vampireKillStreak = 0;
                            g_GameManager.playerHP += 1.0f;
                            if (g_GameManager.playerHP > g_Stats.maxHP) g_GameManager.playerHP = g_Stats.maxHP;
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
                    if (g_MonsterManager.boss && g_MonsterManager.boss->alive)
                        lhitB(g_MonsterManager.boss->worldX, g_MonsterManager.boss->worldY,
                              g_MonsterManager.boss->hp, g_MonsterManager.boss->alive);
                    for (auto* c : g_LeakNodes) if (c->alive)
                        lhitB(c->worldX, c->worldY, c->hp, c->alive);
                    if (g_RRBoss && g_RRBoss->alive)
                        lhitB(g_RRBoss->worldX, g_RRBoss->worldY, g_RRBoss->hp, g_RRBoss->alive);
                    if (g_PolyBoss && g_PolyBoss->alive && g_PolyBoss->damageable())
                        lhitB(g_PolyBoss->worldX, g_PolyBoss->worldY, g_PolyBoss->hp, g_PolyBoss->alive);
                    if (g_SpamBoss && g_SpamBoss->alive)
                        lhitB(g_SpamBoss->worldX, g_SpamBoss->worldY, g_SpamBoss->hp, g_SpamBoss->alive);
                    if (g_KernelBoss && g_KernelBoss->alive)
                        lhitB(g_KernelBoss->worldX, g_KernelBoss->worldY, g_KernelBoss->hp, g_KernelBoss->alive);
                    if (g_FirewallBoss && g_FirewallBoss->alive)
                        lhitB(g_FirewallBoss->worldX, g_FirewallBoss->worldY, g_FirewallBoss->hp, g_FirewallBoss->alive);
                    if (g_BotnetBoss && g_BotnetBoss->alive)
                        lhitB(g_BotnetBoss->worldX, g_BotnetBoss->worldY, g_BotnetBoss->hp, g_BotnetBoss->alive);
                    if (g_CentiBoss && g_CentiBoss->alive && g_CentiBoss->vulnerable())
                        lhitB(g_CentiBoss->worldX, g_CentiBoss->worldY, g_CentiBoss->hp, g_CentiBoss->alive);
                    if (g_TotemBoss && g_TotemBoss->alive && g_TotemBoss->vulnerable())
                        lhitB(g_TotemBoss->worldX, g_TotemBoss->worldY, g_TotemBoss->hp, g_TotemBoss->alive);
                    g_LaserBeams.push_back({ pCX, pCY, lex, ley, 0.13f, 0.13f, beamW });
                    TriggerMuzzle(pCX, pCY, lang);
                }
            }

            // ── 백신 스캔 (증강) — 주기적으로 플레이어 주변 정화 펄스(범위 일소) ──
            if (g_Stats.purgeNova > 0) {
                int   n      = g_Stats.purgeNova;
                float novaInt = NOVA_INT / (1.0f + 0.2f * (float)(n - 1));      // 중첩 시 주기↓
                float novaR   = NOVA_R   * (1.0f + 0.18f * (float)(n - 1));     // 중첩 시 범위↑
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
                    // 보스 — 범위 내면 한 방 칩(보스 체력의 폭주 없게 고정량)
                    auto nhitB = [&](float ex, float ey, float& hp, bool& al) {
                        float dx=ex-pCX, dy=ey-pCY;
                        if (dx*dx+dy*dy < (novaR+70.0f)*(novaR+70.0f)) { hp -= dmg * 2.0f; if (hp<=0.0f) al=false; }
                    };
                    if (g_MonsterManager.boss && g_MonsterManager.boss->alive)
                        nhitB(g_MonsterManager.boss->worldX, g_MonsterManager.boss->worldY,
                              g_MonsterManager.boss->hp, g_MonsterManager.boss->alive);
                    for (auto* c : g_LeakNodes) if (c->alive) nhitB(c->worldX,c->worldY,c->hp,c->alive);
                    if (g_RRBoss && g_RRBoss->alive) nhitB(g_RRBoss->worldX,g_RRBoss->worldY,g_RRBoss->hp,g_RRBoss->alive);
                    if (g_PolyBoss && g_PolyBoss->alive && g_PolyBoss->damageable()) nhitB(g_PolyBoss->worldX,g_PolyBoss->worldY,g_PolyBoss->hp,g_PolyBoss->alive);
                    if (g_SpamBoss && g_SpamBoss->alive) nhitB(g_SpamBoss->worldX,g_SpamBoss->worldY,g_SpamBoss->hp,g_SpamBoss->alive);
                    if (g_KernelBoss && g_KernelBoss->alive) nhitB(g_KernelBoss->worldX,g_KernelBoss->worldY,g_KernelBoss->hp,g_KernelBoss->alive);
                    if (g_FirewallBoss && g_FirewallBoss->alive) nhitB(g_FirewallBoss->worldX,g_FirewallBoss->worldY,g_FirewallBoss->hp,g_FirewallBoss->alive);
                    if (g_BotnetBoss && g_BotnetBoss->alive) nhitB(g_BotnetBoss->worldX,g_BotnetBoss->worldY,g_BotnetBoss->hp,g_BotnetBoss->alive);
                    if (g_CentiBoss && g_CentiBoss->alive && g_CentiBoss->vulnerable()) nhitB(g_CentiBoss->worldX,g_CentiBoss->worldY,g_CentiBoss->hp,g_CentiBoss->alive);
                    if (g_TotemBoss && g_TotemBoss->alive && g_TotemBoss->vulnerable()) nhitB(g_TotemBoss->worldX,g_TotemBoss->worldY,g_TotemBoss->hp,g_TotemBoss->alive);
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
                    // 시각 — 팽창 링(SpawnShockWave 재사용) + 손맛
                    SpawnShockWave(pCX, pCY, novaR, 0.45f, 0.4f, 1.0f, 0.75f);
                    SpawnSparks(pCX, pCY, 10, 0.4f, 1.0f, 0.7f, 360.0f);
                }
            }

            // 포탑 모드에서는 플레이어가 발사하지 않음
            if (!g_Stats.turretMode) {
                // C13 자동 발사: 기본 ON 이면 좌클릭 없이도 조준 방향으로 자동 발사.
                //   C14 유예 중에는 발사 억제(오발 방지). 수동 모드면 좌클릭 홀드.
                bool fireHeld = (g_AutoFire || lmb) && (g_PostPickGrace <= 0.0f);
                if (g_Stats.meleeWeapon) {       // 검객 — 근접 스윙 (총알 없음)
                    // 단타 방지: 쿨다운(effInterval)은 클릭/홀드 무관 항상 적용.
                    //   (예전엔 버튼 떼면 fireTimer 를 즉시 준비 상태로 돌려 광클로
                    //    스윙 속도를 무한히 올릴 수 있었음 — 그 리셋을 제거)
                    if (fireHeld && fireTimer >= effInterval) {
                        float tx, ty;
                        if (aimTarget(tx, ty)) {   // 자동: 최근접 적 / 클릭: 커서
                            meleeSwing(atan2f(ty - pCY, tx - pCX));
                            fireTimer = 0.0f;
                        }
                    }
                } else if (g_Stats.bowWeapon) {  // 궁수 — 누른 만큼 차징 후 발사
                    // 자동: 풀차징까지 자동 충전 → 완충되면 자동 발사(차지 사이클 반복).
                    bool bowHold = g_AutoFire ? (g_ArcherCharge < 1.0f && g_PostPickGrace <= 0.0f)
                                              : lmb;
                    if (bowHold) {
                        // 강궁(40% 빠름) + 연사증강 변환(bowChargeRateMult)
                        g_ArcherCharge += delta / (BOW_CHARGE_TIME * (g_Stats.powerDraw ? 0.6f : 1.0f))
                                          * g_Stats.bowChargeRateMult;
                        if (g_ArcherCharge > 1.0f) g_ArcherCharge = 1.0f;
                    } else if (g_ArcherCharge > 0.001f) {
                        float charge = g_ArcherCharge;
                        float atx, aty; if (!aimTarget(atx, aty)) { atx = wmx; aty = wmy; }
                        float ang = atan2f(aty - pCY, atx - pCX);
                        // 완충 위력 (기본 4.1× / 강궁 5.1×) + 공격력증강 변환(bowChargeCapBonus)
                        float chMult = 0.5f + charge *
                                       ((g_Stats.powerDraw ? 4.6f : 3.6f) + g_Stats.bowChargeCapBonus);
                        float arrowDmg = g_Stats.GetBaseDamage()
                                       * g_Stats.GetDamageMultiplier(0.0f) * chMult;
                        if (g_OverclockTimer > 0.0f) arrowDmg *= 1.5f;
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
                            nb.sizeScale   = 1.0f + charge * 3.0f;     // 최대 4배 크기
                            nb.color       = glm::vec3(0.75f, 0.95f, 0.45f);
                            g_Bullets.push_back(nb);
                        };
                        // 다중 사격: 완충(>0.85) 발사 시 3발 부채꼴
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
                    // 홀드 연사 — 공속(effInterval) 준수. 자동발사 OFF여도 LMB 홀드 시 연사.
                    if (fireHeld && fireTimer >= effInterval) {
                        float tx, ty;
                        bool haveTarget = aimTarget(tx, ty);   // 클릭=커서 일점사 / 자동=최근접 적
                        if (g_DrunkActive) {                    // 취함: 조준 무작위 (적 없어도 발사)
                            float a = (float)(rand() % 628) * 0.01f;
                            tx = pCX + cosf(a) * 200.0f;
                            ty = pCY + sinf(a) * 200.0f;
                            haveTarget = true;
                        }
                        if (haveTarget) {                       // 자동인데 적 없으면 발사 안 함
                            spawnAimed(tx, ty);
                            fireTimer = 0.0f;
                        }
                    }
                }
            }
        }

        // GameManager 가 호버/변환 상태를 알 수 있게 동기화 (Render 에서 사용)
        g_GameManager.hoveredCard = g_HoveredAug;
        g_GameManager.conversionAug = g_ConversionWeapon;

        // ============================================================
        // 렌더링
        // ============================================================
        glViewport(0, 0, screenWidth, screenHeight);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    
        glUseProgram(g_MainShader);
        glUniform1i(g_MainFxLoc, g_ShaderFx ? 1 : 0);   // CRT 셰이더 효과 토글 (G)
        // 화면 흔들기 + 줌 적용 — game world 만, HUD/text(별도 ortho)는 영향 없음
        float orthoShake[16];
        memcpy(orthoShake, g_BaseOrtho, sizeof(g_BaseOrtho));
        // 줌(중심 = ZCX/ZCY, 기본 화면 중앙). z=1 이면 base ortho 와 동일(identity)
        {
            float z = g_ViewZoom;
            float zcx = ZCX(), zcy = ZCY();
            orthoShake[0]  =  2.0f * z / (float)screenWidth;
            orthoShake[5]  = -2.0f * z / (float)screenHeight;
            orthoShake[12] =  2.0f * zcx * (1.0f - z) / (float)screenWidth  - 1.0f;
            orthoShake[13] =  1.0f - 2.0f * zcy * (1.0f - z) / (float)screenHeight;
        }
        // 게임플레이/사망연출 외 상태(메뉴·게임오버 등)에선 화면 흔들림 잔여 제거
        //   — 사망 후 시작창으로 갔을 때 화면이 계속 떨리던 문제 방지
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
        BatchFlush();   // ortho(줌) 바꾸기 전 — 이전 매트릭스로 쌓인 도형 먼저 그림
        glUniformMatrix4fv(g_MainProjLoc, 1, GL_FALSE, orthoShake);
        // 글로벌에도 동기화 — BindMainShader() 가 이 값 사용
        memcpy(g_MainOrtho, orthoShake, sizeof(orthoShake));
        glBindVertexArray(g_MainVAO);
    
        // 월드/엔티티 렌더는 게임플레이 상태에서만 그린다 — 메뉴(시작창 등)에
        //   직전 게임의 플레이어 창·잔여 엔티티가 정지 상태로 비치던 문제 방지.
        //   (g_MainOrtho 는 위에서 이미 갱신했으므로 메뉴 텍스트/데모 렌더는 정상)
        {
        GameState wgs = g_GameManager.currentState;
        bool inWorldRender = (wgs == GameState::RUNNING || wgs == GameState::DYING ||
                              wgs == GameState::PAUSED  || wgs == GameState::READY  ||
                              wgs == GameState::AUG_SELECT || wgs == GameState::DEBUFF_SELECT ||
                              wgs == GameState::GAMEOVER);
        if (inWorldRender) {
    
        // 원거리 몹 FakeWindow 크기 상수 (렌더·클리핑 공용)
        const float RFW_W = g_RfwW;
        const float RFW_H = g_RfwH;
    
        // ============================================================
        // 렌더 z-order (아래→위)
        //  (a) 원거리 몹 FakeWindow 배경  ← 가장 아래
        //  (b) 원거리 몹 창 내부의 잡몹·총알·다이아몬드 (scissor 클리핑)
        //  (c) 플레이어 FakeWindow 배경 ← 위에서 덮음
        //  (d) 플레이어 캐릭터/사망 이펙트
        //  (e) 플레이어 창 내부의 잡몹·총알 (scissor 클리핑) ← 가장 위
        //  (f) BrokenSight 오브 (클리핑 없음)
        // ============================================================
    
        // (a0) 투명 배경 가리기 — 충격파 배경 + 텔레그래프 배경
        //      glDisable(GL_BLEND) + 불투명 어두운 도형 → 이후 FakeWindow 로 덮어씀
        //      투명 영역에만 남아 VFX / 텔레그래프가 데스크톱 위에 뜨지 않게 함
        BatchFlush(); glDisable(GL_BLEND);
        BindMainShader();
    
        // (a0-1) 자폭병 충격파 배경 — needsBg 플래그가 설정된 충격파에 한해
        for (auto& sw : g_ShockWaves) {
            if (!sw.active || !sw.needsBg) continue;
            float t   = 1.0f - sw.life / sw.maxLife;  // 0→1
            float bgR = sw.maxRadius * t + 30.0f;      // 링보다 조금 크게
            drawCircle(sw.x, sw.y, bgR, 0.08f, 0.08f, 0.10f, 1.0f);
        }
    
        // (a0-2) DRIP 경고 배경 — 누수 낙하 지점 어두운 원
        if (g_MonsterManager.boss && g_MonsterManager.boss->alive) {
            auto* bs0 = g_MonsterManager.boss;
            if (bs0->dripPending) {
                float warnT = bs0->dripTimer - (Boss::DRIP_INTERVAL - Boss::DRIP_WARN);
                float prog0 = warnT / Boss::DRIP_WARN;
                if (prog0 < 0.0f) prog0 = 0.0f;
                if (prog0 > 1.0f) prog0 = 1.0f;
                float r0 = 40.0f + prog0 * 110.0f;
                drawCircle(bs0->dripX, bs0->dripY, r0 + 30.0f, 0.06f, 0.07f, 0.12f, 0.85f);
            }
        }
    
        // (a) 원거리 몹 + 포탑 + 보스 FakeWindow 배경 — 블렌드 OFF 로 직접 덮어쓰기
        //     겹쳐서 또 그려도 같은 색이 그대로 쓰여 누적 없음.
        // 포탑 250×250 창 배경 (다수)
        if (g_Stats.turretMode) {
            for (auto& t : g_Turrets) {
                float twx = WinOrigin(t.x, TURRET_WIN_W);
                float twy = WinOrigin(t.y, TURRET_WIN_H);
                drawRect(twx, twy, TURRET_WIN_W, TURRET_WIN_H,
                         0.06f, 0.08f, 0.10f, 1.0f);
            }
        }
        // ── 가짜창 통합 z-리스트 (낮음→높음: 봇넷 < 원거리 < 보스/슬라임). 같은 타입은
        //    소환(벡터) 순서 = 먼저 소환된 애가 아래. 배경+네온보더를 이 순서로 '창 단위'
        //    로 그려, 높은 창의 불투명 배경이 낮은 창의 배경·외곽선을 자연히 덮음(우선순위 가림).
        //    (플레이어 창은 이 뒤에 따로 그려 항상 최상단.)
        struct FWin { float x, y, w, h; const wchar_t* name;
                      float br, bgc, bbc, nr, ngc, nbc; };
        std::vector<FWin> zwins;
        auto addW = [&](float cx, float cy, float w, float h, const wchar_t* nm,
                        float br, float bgc, float bbc, float nr, float ngc, float nbc) {
            zwins.push_back({ cx - w*0.5f, cy - h*0.5f, w, h, nm, br,bgc,bbc, nr,ngc,nbc });
        };
        // 봇넷 노드 (최하단, 소환 순서)
        for (auto m : g_MonsterManager.monsters) {
            if (!m->alive || m->kind != MobKind::SPAWNER) continue;
            float w = SPAWNER_WIN_W * m->sizeScale;
            addW(m->worldX, m->worldY, w, w, L"botnet.node", 0.06f,0.10f,0.09f, 0.20f,0.85f,0.65f);
        }
        // DDOS — DrawAppWindow 통합 패스 (e3) 에서만 렌더
        // 원거리 몹 (소환 순서)
        for (auto r : g_MonsterManager.rangedMobs) {
            if (r->deathScale <= 0.0f) continue;
            float sc = r->deathScale;
            addW(r->worldX, r->worldY, RFW_W*sc, RFW_H*sc, L"popup.exe", 0.08f,0.08f,0.10f, 0.85f,0.20f,0.95f);
        }
        // 보스/분열체 (상단)
        if (g_MonsterManager.boss && g_MonsterManager.boss->alive)
            addW(g_MonsterManager.boss->worldX, g_MonsterManager.boss->worldY, Boss::WIN_W, Boss::WIN_H,
                 L"LEAK.sys", 0.07f,0.08f,0.12f, 0.45f,0.55f,1.0f);
        for (auto* c : g_LeakNodes) if (c->alive) {
            float w = Boss::WIN_W * c->sizeScale;
            addW(c->worldX, c->worldY, w, w, L"leak.node", 0.06f,0.08f,0.12f, 0.35f,0.65f,1.0f);
        }
        if (g_RRBoss && g_RRBoss->alive)
            addW(g_RRBoss->worldX, g_RRBoss->worldY, RR_WIN_W, RR_WIN_W,
                 L"VOLLEY.sys", 0.10f,0.07f,0.06f, 1.0f,0.55f,0.20f);
        if (g_PolyBoss && g_PolyBoss->alive)
            addW(g_PolyBoss->worldX, g_PolyBoss->worldY, POLY_WIN_W, POLY_WIN_W,
                 L"POLYMORPH.vir", 0.09f,0.06f,0.11f, 0.60f,0.30f,1.0f);
        if (g_SpamBoss && g_SpamBoss->alive)
            addW(g_SpamBoss->worldX, g_SpamBoss->worldY, SPAM_WIN_W, SPAM_WIN_W,
                 L"SPAM.dll", 0.10f,0.06f,0.09f, 1.0f,0.40f,0.80f);
        if (g_KernelBoss && g_KernelBoss->alive)
            addW(g_KernelBoss->worldX, g_KernelBoss->worldY, KERNEL_WIN_W, KERNEL_WIN_W,
                 L"KERNEL.sys", 0.10f,0.07f,0.05f, 1.0f,0.65f,0.25f);
        if (g_FirewallBoss && g_FirewallBoss->alive)
            addW(g_FirewallBoss->worldX, g_FirewallBoss->worldY, FIREWALL_WIN_W, FIREWALL_WIN_W,
                 L"FIREWALL.sys", 0.10f,0.06f,0.05f, 1.0f,0.45f,0.2f);
        if (g_BotnetBoss && g_BotnetBoss->alive)
            addW(g_BotnetBoss->worldX, g_BotnetBoss->worldY, BOTNET_WIN_W, BOTNET_WIN_W,
                 L"C2_RELAY.sys", 0.04f,0.07f,0.05f, 0.25f,0.92f,0.45f);
        if (g_CentiBoss && g_CentiBoss->alive)
            addW(g_CentiBoss->worldX, g_CentiBoss->worldY, CENTI_WIN_W, CENTI_WIN_W,
                 CentipedeBoss::BOSS_NAME, 0.02f,0.03f,0.04f, 0.22f,0.55f,0.72f);
        if (g_TotemBoss && g_TotemBoss->alive)
            addW(g_TotemBoss->worldX, g_TotemBoss->worldY, TOTEM_WIN_W, TOTEM_WIN_W,
                 L"RITE.CORE", 0.08f,0.04f,0.10f, 0.85f,0.45f,0.95f);
        // 포탑 창 배경+보더 (최하단, 플레이어 소유라 z-리스트 밖)
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
        // z-리스트 — 창 단위로 (불투명 배경 → 네온 보더). 높은 창이 낮은 창을 자연 가림.
        for (auto& fw : zwins) {
            BatchFlush(); glDisable(GL_BLEND);
            drawRect(fw.x, fw.y, fw.w, fw.h, fw.br, fw.bgc, fw.bbc, 1.0f);
            BatchFlush(); glEnable(GL_BLEND);
            drawNeonBorder(fw.x, fw.y, fw.w, fw.h, fw.nr, fw.ngc, fw.nbc);
        }
        BatchFlush(); glEnable(GL_BLEND);  // 이후 일반 알파 블렌딩 보장
    
        // (b) 원거리 몹 + 보스 창 내부 컨텐츠 (잡몹·자폭병·총알·파편)
        //     각 창마다 scissor 패스. 다이아몬드/본체는 (e2)/(e3) 에서 별도로 그림
        BatchFlush(); glEnable(GL_SCISSOR_TEST);
        for (auto r : g_MonsterManager.rangedMobs) {
            if (r->deathScale <= 0.0f) continue;
            float sc  = r->deathScale;
            float rW  = RFW_W * sc, rH = RFW_H * sc;
            float rwx = r->worldX - rW * 0.5f;
            float rwy = r->worldY - rH * 0.5f;
            WorldScissor(rwx, rwy, rW, rH);
            // 잡몹 (보스 소환물은 더 큼) — 창 밖은 컬링
            for (auto m : g_MonsterManager.monsters) {
                if (!m->alive || m->kind == MobKind::DDOS || !inWin(m->worldX, m->worldY, rwx, rwy, rW, rH)) continue;
                drawMob(m);
            }
            // 자폭병 (5각형)
            for (auto bm : g_MonsterManager.bombers) {
                if (!bm->alive || !inWin(bm->worldX, bm->worldY, rwx, rwy, rW, rH)) continue;
                drawPentagon(bm->worldX, bm->worldY, Bomber::SIZE_PX,
                             bm->color.r, bm->color.g, bm->color.b, 1.0f);
                // 점화 중 — 트리거/폭발 반경 표시
                if (bm->arming) {
                    drawCircle(bm->worldX, bm->worldY, bm->blastRadius,
                               1.0f, 0.2f, 0.2f, 0.10f);
                }
            }
            // 총알
            for (auto& b : g_Bullets) {
                if (!b.active || !inWin(b.x, b.y, rwx, rwy, rW, rH)) continue;
                drawBullet(b);
            }
            // 사망 파티클
            for (auto& p : g_EnemyParts) {
                if (!p.active || !inWin(p.x, p.y, rwx, rwy, rW, rH)) continue;
                float a  = p.life / p.maxLife;
                float hs = p.size * 0.5f;
                drawRect(p.x - hs, p.y - hs, p.size, p.size, p.r, p.g, p.b, a);
            }
            // 다가오는 죽음 오브 (창 안에서만)
            for (auto& orb : g_ApproachOrbs) {
                DrawApproachOrb(orb.x, orb.y);
            }
        }
        // (b'') 포탑 창 안 컨텐츠 (다수)
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
    
        // (b') 보스 창 안 컨텐츠 — 같은 잡몹/자폭병/총알을 보스 창 영역으로도 노출
        //     모든 보스 종류(슬라임/글리치/리로드/폴리/스팸/슬라임분열체) 공통 처리.
        //     E22: 이전엔 슬라임 보스(g_MonsterManager.boss)만 노출돼 다른 보스 창에선
        //          잡몹/탄이 컬링되어 안 보였음 → 보스별 창 영역마다 scissor 패스 추가.
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
        if (g_MonsterManager.boss && g_MonsterManager.boss->alive) {
            auto* bs = g_MonsterManager.boss;
            drawBossWinContent(bs->worldX - Boss::WIN_W * 0.5f,
                               bs->worldY - Boss::WIN_H * 0.5f, Boss::WIN_W, Boss::WIN_H);
        }
        if (g_RRBoss && g_RRBoss->alive)
            drawBossWinContent(g_RRBoss->worldX - RR_WIN_W * 0.5f,
                               g_RRBoss->worldY - RR_WIN_W * 0.5f, RR_WIN_W, RR_WIN_W);
        if (g_PolyBoss && g_PolyBoss->alive)
            drawBossWinContent(g_PolyBoss->worldX - POLY_WIN_W * 0.5f,
                               g_PolyBoss->worldY - POLY_WIN_W * 0.5f, POLY_WIN_W, POLY_WIN_W);
        if (g_SpamBoss && g_SpamBoss->alive)
            drawBossWinContent(g_SpamBoss->worldX - SPAM_WIN_W * 0.5f,
                               g_SpamBoss->worldY - SPAM_WIN_W * 0.5f, SPAM_WIN_W, SPAM_WIN_W);
        if (g_KernelBoss && g_KernelBoss->alive)
            drawBossWinContent(g_KernelBoss->worldX - KERNEL_WIN_W * 0.5f,
                               g_KernelBoss->worldY - KERNEL_WIN_W * 0.5f, KERNEL_WIN_W, KERNEL_WIN_W);
        if (g_FirewallBoss && g_FirewallBoss->alive)
            drawBossWinContent(g_FirewallBoss->worldX - FIREWALL_WIN_W * 0.5f,
                               g_FirewallBoss->worldY - FIREWALL_WIN_W * 0.5f, FIREWALL_WIN_W, FIREWALL_WIN_W);
        if (g_BotnetBoss && g_BotnetBoss->alive)
            drawBossWinContent(g_BotnetBoss->worldX - BOTNET_WIN_W * 0.5f,
                               g_BotnetBoss->worldY - BOTNET_WIN_W * 0.5f, BOTNET_WIN_W, BOTNET_WIN_W);
        if (g_CentiBoss && g_CentiBoss->alive)
            drawBossWinContent(g_CentiBoss->worldX - CENTI_WIN_W * 0.5f,
                               g_CentiBoss->worldY - CENTI_WIN_W * 0.5f, CENTI_WIN_W, CENTI_WIN_W);
        if (g_TotemBoss && g_TotemBoss->alive)
            drawBossWinContent(g_TotemBoss->worldX - TOTEM_WIN_W * 0.5f,
                               g_TotemBoss->worldY - TOTEM_WIN_W * 0.5f, TOTEM_WIN_W, TOTEM_WIN_W);
        for (auto* c : g_LeakNodes) {
            if (!c->alive) continue;
            float w = Boss::WIN_W * c->sizeScale;
            drawBossWinContent(c->worldX - w * 0.5f, c->worldY - w * 0.5f, w, w);
        }
        // 봇넷 노드(SPAWNER) 창 내부 컨텐츠 — 노드 본체/소환 알이 자기 창에서 보이도록 (E21)
        for (auto m : g_MonsterManager.monsters) {
            if (!m->alive || m->kind != MobKind::SPAWNER) continue;
            float w = SPAWNER_WIN_W * m->sizeScale;
            drawBossWinContent(m->worldX - w * 0.5f, m->worldY - w * 0.5f, w, w);
        }
        BatchFlush(); glDisable(GL_SCISSOR_TEST);

        // FORK.worm child adds — 각자 가짜 앱 창 (y 오름차순 = 아래가 위로 겹침)
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

        // TOTEM.sys 토템 — 종류별 가짜 앱 창
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

        // (c) 플레이어 FakeWindow 배경
        //     원거리 몹 창과 겹친 영역도 player 색으로 깔끔하게 덮임 (누적 없음)
        //     ranged 컨텐츠 (b) 가 player 영역에 그려졌으면 여기서 덮여 사라짐
        //     = "원거리 몹 창이 플레이어 창 안에 들어가면 가려짐" 원래 의도 그대로
        BatchFlush(); glDisable(GL_BLEND);
        drawRect(playerWin.x, playerWin.y, playerWin.width, playerWin.height,
                 0.05f, 0.06f, 0.09f, 1.0f);
        BatchFlush(); glEnable(GL_BLEND);
        // 사이버펑크 네온 터미널 — 플레이어 창 네온 보더 (액센트 테마 색)
        drawNeonBorder(playerWin.x, playerWin.y, playerWin.width, playerWin.height,
                       g_AccentR, g_AccentG, g_AccentB);
    
        // (c2) HP/EXP 바 — 플레이어 창 하단 안쪽에 부착 (창과 함께 이동) ──
        if (g_GameManager.currentState == GameState::RUNNING ||
            g_GameManager.currentState == GameState::PAUSED ||
            g_GameManager.currentState == GameState::AUG_SELECT ||
            g_GameManager.currentState == GameState::DEBUFF_SELECT ||
            g_GameManager.currentState == GameState::DYING) {
            float pad = 12.0f, bx = playerWin.x + pad;
            float bw = playerWin.width - pad * 2.0f;
            float hpH = 14.0f, xpH = 8.0f, gap = 3.0f;   // 두껍게 (가시성)
            float hpY = playerWin.y + playerWin.height - 18.0f - hpH;   // 하단 안쪽
            float xpY = hpY - gap - xpH;
            // 공통 불투명 패널 — 탄막/적과 겹쳐도 바가 묻히지 않도록
            drawRect(bx - 5, xpY - 5, bw + 10, (hpY + hpH) - (xpY) + 10, 0.03f, 0.03f, 0.05f, 0.96f);
            // HP
            float hpFrac = (g_Stats.maxHP > 0.0f) ? g_GameManager.playerHP / g_Stats.maxHP : 0.0f;
            if (hpFrac < 0.0f) hpFrac = 0.0f; if (hpFrac > 1.0f) hpFrac = 1.0f;
            float hpR = (hpFrac > 0.5f) ? 0.1f : 1.0f;
            float hpG = (hpFrac > 0.5f) ? 1.0f : hpFrac * 2.0f;
            drawRect(bx, hpY, bw, hpH, 0.22f, 0.04f, 0.04f, 1.0f);
            drawRect(bx, hpY, bw * hpFrac, hpH, hpR, hpG, 0.1f, 1.0f);
            // (HP 수치는 좌상단 HUD 에 표시 — 월드 섹션에서 텍스트를 그리면
            //  TextRenderer 가 셰이더/VAO 를 언바인드해 이후 엔티티 렌더가 깨지므로 금지)
            // XP
            long long needX = g_ExpSystem.Required(g_GameManager.playerLevel);
            float xpFrac = (needX > 0) ? (float)g_GameManager.xp / (float)needX : 0.0f;
            if (xpFrac < 0.0f) xpFrac = 0.0f; if (xpFrac > 1.0f) xpFrac = 1.0f;
            drawRect(bx, xpY, bw, xpH, 0.06f, 0.10f, 0.07f, 1.0f);
            drawRect(bx, xpY, bw * xpFrac, xpH, 0.4f, 1.0f, 0.55f, 1.0f);
        }
    
        // (c2.5) 배드 섹터 감속 구역 — 중심에서 부식되어 퍼지는 손상 블록(깜빡임 애니메이션)
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
                // 부식 블록 — 셀별 의사난수 깜빡임, 중심에서 grow factor 까지만 노출(퍼짐)
                const int N = 7;
                float cw = z.w / (float)N, ch = z.h / (float)N;
                for (int iy = 0; iy < N; iy++) for (int ix = 0; ix < N; ix++) {
                    float fx = z.x + ((float)ix + 0.5f) * cw;
                    float fy = z.y + ((float)iy + 0.5f) * ch;
                    float dxn = (fx - zx) / (z.w*0.5f + 1e-3f);
                    float dyn = (fy - zy) / (z.h*0.5f + 1e-3f);
                    if (sqrtf(dxn*dxn + dyn*dyn) > gf) continue;   // 아직 부식 안 닿음
                    float seed = sinf((float)ix*12.9898f + (float)iy*78.233f) * 43758.5453f;
                    float ph = seed - floorf(seed);
                    float fl = 0.45f + 0.55f * sinf(zt * 6.0f + ph * 6.2831853f);
                    float ca = (0.10f + 0.22f * fl) * lifeF;
                    drawRect(fx - cw*0.42f, fy - ch*0.42f, cw*0.84f, ch*0.84f, 0.62f, 0.2f, 0.88f, ca);
                }
                drawNeonBorder(zx - hw, zy - hh, hw*2, hh*2, 0.75f, 0.3f, 0.95f);
            }
        }
    
        // (c3) 스캔 레이저 빔 — 페이드되는 청록 관통 빔 (보스 레이저 쿼드 패턴)
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
    
    
        // (d) 플레이어 캐릭터 + 증강 이펙트 + 사망 파편
        {
            float pCX = playerWin.x + playerWin.width  * 0.5f;
            float pCY = playerWin.y + playerWin.height * 0.5f;
            // 총검: 200px 이내 표시 (희미한 시안 원)
            if (g_Stats.bayonet)
                drawCircle(pCX, pCY, 200.0f, 0.4f, 1.0f, 0.9f, 0.10f);
    
            // 궁수 차징 게이지 (플레이어 위 바) — 완충 시 흰색 번쩍
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
    
                // 폭발 충격파
                float t      = 1.0f - fade;
                float shockR = 60.0f + t * 520.0f;
                float shockA = (1.0f - t) * 0.55f;
                drawCircle(g_DeathCX, g_DeathCY, shockR,
                           1.0f, 0.95f, 0.4f, shockA);
    
                // 중심 섬광
                if (g_DeathFlash > 0.0f) {
                    float fr = 220.0f * g_DeathFlash;
                    drawCircle(g_DeathCX, g_DeathCY, fr,
                               1.0f, 1.0f, 1.0f, g_DeathFlash * 0.9f);
                    drawCircle(g_DeathCX, g_DeathCY, fr * 1.8f,
                               1.0f, 0.85f, 0.2f, g_DeathFlash * 0.45f);
                }
    
                // 사망 파편
                for (int i = 0; i < MAX_DEBRIS; i++) {
                    if (!g_Debris[i].active) continue;
                    float s  = g_Debris[i].size;
                    float hs = s * 0.5f;
                    drawRect(g_Debris[i].x - hs, g_Debris[i].y - hs, s, s,
                             g_Debris[i].r, g_Debris[i].g, g_Debris[i].b, fade);
                }
            } else if (g_GameManager.currentState != GameState::GAMEOVER) {
                // 이동 잔상 — 플레이어 뒤(먼저 그려 아래에 깔림), 수명 비례 페이드+축소
                for (auto& tr : g_Trail) {
                    float f = tr.life / tr.maxLife;        // 1→0
                    float s = tr.size * (0.4f + 0.6f * f);
                    drawRect(tr.x - s*0.5f, tr.y - s*0.5f, s, s, tr.r, tr.g, tr.b, 0.28f * f);
                }
                float sz = PLAYER_SIZE * g_Stats.playerSizeMult;
                float hs = sz * 0.5f;
                // 외곽: 어두운 테두리 (대비)
                drawRect(pCX - hs - 3, pCY - hs - 3, sz + 6, sz + 6,
                         0.0f, 0.0f, 0.0f, 0.8f);
                // 본체: 밝은 시안
                drawRect(pCX - hs, pCY - hs, sz, sz,
                         0.3f, 1.0f, 1.0f, 1.0f);
                // 중심 코어: 흰색 작은 사각형
                float core = sz * 0.35f;
                drawRect(pCX - core * 0.5f, pCY - core * 0.5f, core, core,
                         1.0f, 1.0f, 1.0f, 1.0f);
    
                // ── 산나비식 HP 게이지바 — 피격 시 플레이어 위에 떴다 페이드, 피 낮으면 상시 ──
                if (g_GameManager.currentState == GameState::RUNNING) {
                    float hf = (g_Stats.maxHP > 0.0f) ? g_GameManager.playerHP / g_Stats.maxHP : 0.0f;
                    if (hf < 0.0f) hf = 0.0f; if (hf > 1.0f) hf = 1.0f;
                    bool low = hf < 0.40f;
                    float vis = low ? 1.0f
                              : (g_HpBarPop > 1.6f ? (2.2f - g_HpBarPop) / 0.6f   // 빠른 페이드인
                                                   : g_HpBarPop / 1.6f);          // 느린 페이드아웃
                    if (vis > 1.0f) vis = 1.0f; if (vis < 0.0f) vis = 0.0f;
                    if (vis > 0.01f) {
                        // 작고 플레이어에 가깝게 — 위쪽 적을 덜 가리도록 (가린다는 피드백)
                        float bw = 46.0f * g_Stats.playerSizeMult, bh = 5.0f;
                        float bx = pCX - bw * 0.5f, by = pCY - hs - 15.0f;
                        drawRect(bx - 1.5f, by - 1.5f, bw + 3, bh + 3, 0.0f, 0.0f, 0.0f, 0.6f * vis);
                        drawRect(bx, by, bw, bh, 0.25f, 0.05f, 0.05f, 0.7f * vis);
                        float r = hf > 0.5f ? 0.2f : 1.0f;
                        float g = hf > 0.5f ? 1.0f : hf * 2.0f;
                        drawRect(bx, by, bw * hf, bh, r, g, 0.15f, 0.92f * vis);
                    }
                }
    
                // ── 위치 강조 표시 (혼잡한 탄막 속에서 플레이어를 쉽게 찾도록) ──
                //   + 자형 레티클(중심 비움) + 옅은 헤일로. HP 낮을수록 강해지고 붉어짐.
                if (g_GameManager.currentState == GameState::RUNNING) {
                    float hpFrac = (g_Stats.maxHP > 0.0f)
                                 ? g_GameManager.playerHP / g_Stats.maxHP : 1.0f;
                    if (hpFrac < 0.0f) hpFrac = 0.0f; if (hpFrac > 1.0f) hpFrac = 1.0f;
                    bool low = hpFrac < 0.3f;
                    float spd   = low ? 9.0f : 3.5f;
                    float pulse = 0.5f + 0.5f * sinf((float)glfwGetTime() * spd);
                    float a = (low ? (0.45f + 0.4f * pulse) : (0.22f + 0.12f * pulse));
                    // 색: 평상시 시안, 위험 시 붉게
                    float rr = low ? 1.0f : 0.4f;
                    float gg = low ? 0.35f : 1.0f;
                    float bb = low ? 0.35f : 1.0f;
                    // 옅은 헤일로(채워진 원) — 멀리서도 위치가 보이도록
                    drawCircle(pCX, pCY, hs + 22.0f * g_Stats.playerSizeMult,
                               rr, gg, bb, a * 0.18f);
                    // + 자형 레티클 (중심은 비워서 본체를 가리지 않음)
                    float gap = hs + 6.0f;
                    float L   = 16.0f * g_Stats.playerSizeMult;
                    float t   = 3.0f;
                    drawRect(pCX - t*0.5f, pCY - gap - L, t, L, rr, gg, bb, a);  // 위
                    drawRect(pCX - t*0.5f, pCY + gap,     t, L, rr, gg, bb, a);  // 아래
                    drawRect(pCX - gap - L, pCY - t*0.5f, L, t, rr, gg, bb, a);  // 좌
                    drawRect(pCX + gap,     pCY - t*0.5f, L, t, rr, gg, bb, a);  // 우
                }
            }
        }
    
        // (e) 플레이어 창 내부 컨텐츠 — scissor (가장 위 레이어)
        BatchFlush(); glEnable(GL_SCISSOR_TEST);
        WorldScissor(playerWin.x, playerWin.y, playerWin.width, playerWin.height);
        {
        float pwx = playerWin.x, pwy = playerWin.y, pww = playerWin.width, pwh = playerWin.height;
        // 잡몹 (보스 소환물은 더 큼) — 창 밖 컬링
        for (auto m : g_MonsterManager.monsters) {
            if (!m->alive || m->kind == MobKind::DDOS || !inWin(m->worldX, m->worldY, pwx, pwy, pww, pwh)) continue;
            drawMob(m);
        }
        // 자폭병
        for (auto bm : g_MonsterManager.bombers) {
            if (!bm->alive || !inWin(bm->worldX, bm->worldY, pwx, pwy, pww, pwh)) continue;
            drawPentagon(bm->worldX, bm->worldY, Bomber::SIZE_PX,
                         bm->color.r, bm->color.g, bm->color.b, 1.0f);
            if (bm->arming) {
                drawCircle(bm->worldX, bm->worldY, bm->blastRadius,
                           1.0f, 0.2f, 0.2f, 0.10f);
            }
        }
        // 총알
        for (auto& b : g_Bullets) {
            if (!b.active || !inWin(b.x, b.y, pwx, pwy, pww, pwh)) continue;
            drawBullet(b);
        }
        // 사망 파티클
        for (auto& p : g_EnemyParts) {
            if (!p.active || !inWin(p.x, p.y, pwx, pwy, pww, pwh)) continue;
            float a  = p.life / p.maxLife;
            float hs = p.size * 0.5f;
            drawRect(p.x - hs, p.y - hs, p.size, p.size, p.r, p.g, p.b, a);
        }
        }
        // 다가오는 죽음 오브 (플레이어 창 안에서만)
        for (auto& orb : g_ApproachOrbs) {
            DrawApproachOrb(orb.x, orb.y);
        }
        BatchFlush(); glDisable(GL_SCISSOR_TEST);

        // (e3) flood.exe — y-sort 앱 창 (플레이어 위 레이어)
        {
            std::vector<Monster*> ddos;
            for (auto m : g_MonsterManager.monsters) {
                if (!m->alive || m->kind != MobKind::DDOS) continue;
                ddos.push_back(m);
            }
            if (!ddos.empty()) {
                std::sort(ddos.begin(), ddos.end(),
                          [](Monster* a, Monster* b) { return a->worldY < b->worldY; });
                const float DTB = 14.0f * g_Scale;
                for (auto* m : ddos) {
                    float w = DDOS_WIN_W * m->sizeScale;
                    float h = w * 0.82f;
                    float wx = m->worldX - w * 0.5f, wy = m->worldY - h * 0.5f;
                    DrawAppWindow(wx, wy, w, h, L"flood.exe", DTB);
                    BatchFlush(); glEnable(GL_SCISSOR_TEST);
                    BindMainShader();
                    WorldScissor(wx, wy, w, h);
                    drawMob(m);
                    BatchFlush(); glDisable(GL_SCISSOR_TEST);
                }
            }
        }

        // (e2) 원거리 몹 다이아몬드 — 각 원거리 몹 창 영역에서 항상 위에 그림
        BatchFlush(); glEnable(GL_SCISSOR_TEST);
        for (auto r : g_MonsterManager.rangedMobs) {
            if (r->deathScale <= 0.0f) continue;
            float sc  = r->deathScale;
            float rW  = RFW_W * sc, rH = RFW_H * sc;
            float rwx = r->worldX - rW * 0.5f;
            float rwy = r->worldY - rH * 0.5f;
            WorldScissor(rwx, rwy, rW, rH);
            // 죽은 몹은 다이아몬드도 축소 + 페이드
            float dSize = 32.0f * sc;
            float dAlpha = sc;
            drawDiamond(r->worldX, r->worldY, dSize,
                        r->color.r, r->color.g, r->color.b, dAlpha);
        }
        BatchFlush(); glDisable(GL_SCISSOR_TEST);
    
        // (e2.1) 포탑 아이콘 + 수명바 (다수) — 각 창 영역 scissor 내에서 표시
        if (g_Stats.turretMode) {
            BatchFlush(); glEnable(GL_SCISSOR_TEST);
            for (auto& t : g_Turrets) {
                float twx = WinOrigin(t.x, TURRET_WIN_W);
                float twy = WinOrigin(t.y, TURRET_WIN_H);
                WorldScissor(twx, twy, TURRET_WIN_W, TURRET_WIN_H);
                // 포탑 본체 — 십자형 (중앙 사각형 + 4방향 돌출)
                float tc = 12.0f;
                drawRect(t.x - tc, t.y - 4, tc*2, 8, 0.1f, 1.0f, 0.55f, 1.0f);
                drawRect(t.x - 4, t.y - tc, 8, tc*2, 0.1f, 1.0f, 0.55f, 1.0f);
                // 수명 바 (창 상단)
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
    
        // (e2.4) LEAK 누수 구역 — 전체 화면 (가짜창 밖에도 보임)
        {
            float leakGt = (float)glfwGetTime();
            BindMainShader();
            if (g_MonsterManager.boss && g_MonsterManager.boss->alive)
                g_MonsterManager.boss->renderPools(leakGt);
            for (auto* c : g_LeakNodes)
                if (c->alive) c->renderPools(leakGt);
        }

        // (e2.5) LEAK.sys 경고 — DRIP / ALLOC (scissor 없음)
        if (g_MonsterManager.boss && g_MonsterManager.boss->alive) {
            auto* bs = g_MonsterManager.boss;
            BindMainShader();
            if (bs->dripPending) {
                float warnT = bs->dripTimer - (Boss::DRIP_INTERVAL - Boss::DRIP_WARN);
                float prog = warnT / Boss::DRIP_WARN;
                if (prog < 0.0f) prog = 0.0f;
                if (prog > 1.0f) prog = 1.0f;
                float r = 28.0f + prog * 95.0f;
                float blink = 0.25f + 0.35f * (0.5f + 0.5f * sinf((float)glfwGetTime() * 18.0f));
                drawCircle(bs->dripX, bs->dripY, r, 0.45f, 0.55f, 1.0f, blink * (0.2f + 0.45f * prog));
                drawCircle(bs->dripX, bs->dripY, r * 0.45f, 0.75f, 0.85f, 1.0f, blink * 0.35f);
            }
            if (bs->summonPending) {
                float blink = 0.30f + 0.30f * (0.5f + 0.5f *
                              sinf((float)glfwGetTime() * 16.0f));
                int sc = bs->phase3 ? Boss::ALLOC_COUNT + 2 : Boss::ALLOC_COUNT;
                for (int i = 0; i < sc; i++) {
                    float ang = (float)i / (float)sc * 6.2831853f;
                    float sx  = bs->worldX + cosf(ang) * Boss::ALLOC_RING_R;
                    float sy  = bs->worldY + sinf(ang) * Boss::ALLOC_RING_R;
                    drawCircle(sx, sy, 12.0f, 0.55f, 0.45f, 1.0f, blink);
                }
            }
        }
    
        // (e3) LEAK.sys 본체
        if (g_MonsterManager.boss && g_MonsterManager.boss->alive) {
            auto* bs = g_MonsterManager.boss;
            BatchFlush(); glEnable(GL_SCISSOR_TEST);
            float bwx = bs->worldX - Boss::WIN_W * 0.5f;
            float bwy = bs->worldY - Boss::WIN_H * 0.5f;
            WorldScissor(bwx, bwy, Boss::WIN_W, Boss::WIN_H);
            bs->renderBody((float)glfwGetTime());
            BatchFlush(); glDisable(GL_SCISSOR_TEST);
        }
    
        // (e3.5) 누수 노드 — 본체/HP/총알(개인 창)
        for (auto* c : g_LeakNodes) {
            if (!c->alive) continue;
            float scale = c->sizeScale;
            float body  = Boss::BODY_SIZE * scale;
            float win   = Boss::WIN_W * scale;
            BatchFlush(); glEnable(GL_SCISSOR_TEST);
            WorldScissor(c->worldX - win*0.5f, c->worldY - win*0.5f, win, win);
            for (auto& b : g_Bullets) {
                if (!b.active) continue;
                drawBullet(b);
            }
            c->renderBody((float)glfwGetTime());
            float hpFrac = c->hp / c->maxHp; if(hpFrac<0)hpFrac=0; if(hpFrac>1)hpFrac=1;
            float hbW = 120.0f * scale, hbH = 7.0f;
            float hbX = c->worldX - hbW*0.5f, hbY = c->worldY - body - 14.0f;
            drawRect(hbX, hbY, hbW, hbH, 0.10f, 0.12f, 0.18f, 0.85f);
            drawRect(hbX, hbY, hbW*hpFrac, hbH, 0.35f, 0.65f, 1.0f, 0.95f);
            BatchFlush(); glDisable(GL_SCISSOR_TEST);
        }
    
        // (f) BrokenSight 오브 — 항상 표시 (클리핑 없음, 무적)
        if (g_Stats.brokenSight && g_Orb.active) {
            drawCircle(g_Orb.x, g_Orb.y, 16.0f, 1.0f, 1.0f, 0.1f, 0.22f);
            drawDiamond(g_Orb.x, g_Orb.y, 22.0f, 1.0f, 0.92f, 0.0f, 1.0f);
        }
    
        // 크로스헤어 — 게임 중에만, g_ShowCrosshair true 일 때
        if (g_ShowCrosshair &&
            (g_GameManager.currentState == GameState::RUNNING ||
             g_GameManager.currentState == GameState::PAUSED  ||
             g_GameManager.currentState == GameState::DYING)) {
            float ax = wmx, ay = wmy;   // 줌 보정 → 줌 적용된 ortho 에서 커서 위치에 표시
            // 외곽 어두운 원 + 중앙 십자 (링/십자는 액센트 테마 색)
            float cr = g_AccentR, cg = g_AccentG, cb = g_AccentB;
            drawCircle(ax, ay, 12.0f, 0.0f, 0.0f, 0.0f, 0.6f);
            drawCircle(ax, ay, 10.0f, cr, cg, cb, 0.9f);
            drawCircle(ax, ay, 5.0f, 0.05f, 0.05f, 0.05f, 0.9f);
            // 중심 점
            drawRect(ax - 1.5f, ay - 1.5f, 3.0f, 3.0f,
                     1.0f, 1.0f, 1.0f, 1.0f);
            // 4방향 짧은 라인 (십자)
            drawRect(ax - 14.0f, ay - 1.0f, 6.0f, 2.0f, cr, cg, cb, 0.95f);
            drawRect(ax + 8.0f,  ay - 1.0f, 6.0f, 2.0f, cr, cg, cb, 0.95f);
            drawRect(ax - 1.0f, ay - 14.0f, 2.0f, 6.0f, cr, cg, cb, 0.95f);
            drawRect(ax - 1.0f, ay + 8.0f,  2.0f, 6.0f, cr, cg, cb, 0.95f);
        }
    
        // (g) 다가오는 죽음 오브 — scissor 안에서만 표시 ((b)/(e) 패스에 위임)
    
        // (g2) 충격파 — 자폭병 자폭 / 보스 스폰 등. 항상 위에 표시
        for (auto& sw : g_ShockWaves) {
            if (!sw.active) continue;
            float t = 1.0f - sw.life / sw.maxLife;  // 0 → 1
            float radius = sw.maxRadius * t;
            float alpha  = (1.0f - t) * 0.55f;
            drawCircle(sw.x, sw.y, radius, sw.r, sw.g, sw.b, alpha);
        }
    
        // (g2b) 검객 스윙 잔상 — 조준 방향 부채꼴
        for (auto& sl : g_Slashes) {
            if (!sl.active) continue;
            float t   = 1.0f - sl.life / sl.maxLife;       // 0 → 1
            float rad = sl.range * (0.72f + 0.28f * t);
            float alpha   = (1.0f - t) * 0.5f;
            float halfArc = 1.15f * (1.0f - 0.15f * t);
            drawConeFan(sl.x, sl.y, rad, sl.ang, halfArc, 0.85f, 0.95f, 1.0f, alpha);
        }
    
        // (g2c) 타격 스파크 + 머즐 플래시 — 가산(additive) 블렌딩으로 밝게
        if (!g_Sparks.empty() || g_MuzzleTimer > 0.0f) {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);     // additive
            for (auto& sp : g_Sparks) {
                float t = sp.life / sp.maxLife;    // 1 → 0
                drawCircle(sp.x, sp.y, sp.size * (0.5f + 0.5f * t),
                           sp.r, sp.g, sp.b, t);
            }
            if (g_MuzzleTimer > 0.0f) {
                float mt = g_MuzzleTimer / 0.05f;  // 1 → 0
                float mx2 = g_MuzzleX + cosf(g_MuzzleAng) * 26.0f;
                float my2 = g_MuzzleY + sinf(g_MuzzleAng) * 26.0f;
                drawCircle(mx2, my2, 22.0f * mt + 6.0f, 1.0f, 0.92f, 0.55f, mt * 0.9f);
                drawCircle(mx2, my2, 11.0f * mt + 3.0f, 1.0f, 1.0f, 0.9f, mt);
            }
            // 기본 분리 블렌딩 복원
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                                GL_ONE,       GL_ONE_MINUS_SRC_ALPHA);
        }
    

                // VOLLEY.sys — 전조 + 기동 화력 드론
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
    
        // (g4b) SPAM.dll — 회전 나선포 본체 + 탄막(개인 창 클리핑) + HP
        if (g_SpamBoss && g_SpamBoss->alive) {
            auto* sb = g_SpamBoss;
            BindMainShader();
            BatchFlush(); glEnable(GL_SCISSOR_TEST);
            WorldScissor(sb->worldX - SPAM_WIN_W*0.5f, sb->worldY - SPAM_WIN_W*0.5f,
                         SPAM_WIN_W, SPAM_WIN_W);
            // 탄막이 창 안에서도 보이도록
            for (auto& b : g_Bullets) { if (b.active) drawBullet(b); }
            // 회전하는 나선 팔(발사구) — 본체 둘레에서 뻗는 작은 핑크 다이아
            for (int a = 0; a < SpamBoss::ARMS; a++) {
                float ang = sb->spiralAngle + (float)a * (6.2831853f / (float)SpamBoss::ARMS);
                float ox = sb->worldX + cosf(ang) * (SpamBoss::BODY * 1.15f);
                float oy = sb->worldY + sinf(ang) * (SpamBoss::BODY * 1.15f);
                drawDiamond(ox, oy, SpamBoss::BODY * 0.32f, 1.0f, 0.45f, 0.85f, 0.95f);
            }
            // 본체 — 펄스하는 핑크/마젠타 다이아
            float pulse = 0.5f + 0.5f * sinf((float)glfwGetTime() * 6.0f);
            drawDiamond(sb->worldX, sb->worldY, SpamBoss::BODY,
                        1.0f, 0.35f + 0.25f * pulse, 0.8f, 1.0f);
            drawDiamond(sb->worldX, sb->worldY, SpamBoss::BODY * 0.5f,
                        1.0f, 0.85f, 0.95f, 1.0f);
            // (HP 바는 화면 상단 고정 보스 바로 이동)
            BatchFlush(); glDisable(GL_SCISSOR_TEST);
        }
    
        // (g4c) KERNEL.sys — 고정형 거대 코어 + 팽창 예고 링 + 자가붕괴 비주얼
        if (g_KernelBoss && g_KernelBoss->alive) {
            auto* kb = g_KernelBoss;
            BindMainShader();
            BatchFlush(); glEnable(GL_SCISSOR_TEST);
            WorldScissor(kb->worldX - KERNEL_WIN_W*0.5f, kb->worldY - KERNEL_WIN_W*0.5f,
                         KERNEL_WIN_W, KERNEL_WIN_W);
            for (auto& b : g_Bullets) { if (b.active) drawBullet(b); }
            // 팽창 예고 — 곧 뿜을 링을 미리 옅게(자라나는 호박 디스크)
            if (kb->telegraphing()) {
                float tp = kb->telegraphProg();
                drawCircle(kb->worldX, kb->worldY, KernelBoss::BODY * (1.2f + tp * 2.2f),
                           1.0f, 0.55f, 0.2f, 0.10f + 0.10f * tp);
            }
            // 본체 — 펄스하는 코어 (호박/주황 네스티드 + 회전 십자 프로세스)
            float kp = 0.5f + 0.5f * sinf(kb->pulse * 3.0f);
            float br = KernelBoss::BODY * (0.96f + 0.06f * kp);
            drawCircle(kb->worldX, kb->worldY, br,            0.85f, 0.45f, 0.12f, 1.0f);
            drawCircle(kb->worldX, kb->worldY, br * 0.66f,    1.0f,  0.65f, 0.2f,  1.0f);
            drawCircle(kb->worldX, kb->worldY, br * 0.34f,    1.0f,  0.9f,  0.55f, 1.0f);
            // 회전 프로세스 바 (십자)
            float ra = kb->pulse * 0.8f;
            for (int s = 0; s < 4; s++) {
                float a = ra + (float)s * 1.5707963f;
                float ox = kb->worldX + cosf(a) * br * 1.18f;
                float oy = kb->worldY + sinf(a) * br * 1.18f;
                drawRect(ox - 9.0f, oy - 9.0f, 18.0f, 18.0f, 0.95f, 0.55f, 0.15f, 0.95f);
            }
            BatchFlush(); glDisable(GL_SCISSOR_TEST);
        }
    
        // (g4d) FIREWALL.sys — 본체 + 회전 보호막 아크(가변속도) + 견제탄
        if (g_FirewallBoss && g_FirewallBoss->alive) {
            auto* fb = g_FirewallBoss;
            BindMainShader();
            BatchFlush(); glEnable(GL_SCISSOR_TEST);
            WorldScissor(fb->worldX - FIREWALL_WIN_W*0.5f, fb->worldY - FIREWALL_WIN_W*0.5f,
                         FIREWALL_WIN_W, FIREWALL_WIN_W);
            for (auto& b : g_Bullets) { if (b.active) drawBullet(b); }
            // 차단 펄스 예고 — 곧 방사형 탄막 방출 (커지는 옅은 링)
            if (fb->pulseWarning()) {
                float wp = 0.5f + 0.5f * sinf((float)glfwGetTime() * 24.0f);
                drawCircle(fb->worldX, fb->worldY, FirewallBoss::SHIELD_R * (1.1f + 0.5f * wp),
                           1.0f, 0.55f, 0.15f, 0.10f + 0.12f * wp);
            }
            // 본체 — 방화벽 코어: 외곽 다이아 + 어두운 내곽 + 회전 십자 코어 + 맥동 중심
            float fb_t = (float)glfwGetTime();
            float fbp  = 0.5f + 0.5f * sinf(fb_t * 4.0f);
            drawDiamond(fb->worldX, fb->worldY, FirewallBoss::BODY,        0.95f, 0.45f, 0.15f, 1.0f);
            drawDiamond(fb->worldX, fb->worldY, FirewallBoss::BODY * 0.74f, 0.5f, 0.22f, 0.08f, 1.0f);
            drawDiamond(fb->worldX, fb->worldY, FirewallBoss::BODY * 0.5f,  1.0f, 0.7f,  0.3f,  1.0f);
            float fra = fb_t * 0.7f;
            for (int s = 0; s < 4; s++) {
                float a  = fra + (float)s * 1.5707963f;
                float ox = fb->worldX + cosf(a) * FirewallBoss::BODY * 0.30f;
                float oy = fb->worldY + sinf(a) * FirewallBoss::BODY * 0.30f;
                drawRect(ox - 7.0f, oy - 7.0f, 14.0f, 14.0f, 1.0f, 0.8f, 0.4f, 0.95f);
            }
            drawCircle(fb->worldX, fb->worldY,
                       FirewallBoss::BODY * 0.16f * (0.9f + 0.2f*fbp), 1.0f, 0.95f, 0.7f, 1.0f);
            // 보호막 아크 3개 — 부메랑 띠. 페이즈2엔 보호막마다 색상(능력별), 평소엔 주황.
            float sb = fb->fast ? 1.0f : 0.7f;
            for (int s = 0; s < FirewallBoss::SHIELDS; s++) {
                float c = fb->shieldRot + (float)s * (6.2831853f / (float)FirewallBoss::SHIELDS);
                float scr, scg, scb; fb->shieldColor(s, scr, scg, scb);
                const int seg = 17;
                for (int i = 0; i < seg; i++) {
                    float t = (float)i / (float)(seg - 1);            // 0..1
                    float a = c + (t - 0.5f) * 2.0f * FirewallBoss::SHIELD_HALF;
                    float mid = 0.5f - fabsf(t - 0.5f);               // 0(끝)~0.5(중앙)
                    float rr  = FirewallBoss::SHIELD_R + mid * 52.0f; // 중앙이 바깥으로 — 부메랑 굴곡
                    float dsz = 9.0f + mid * 24.0f;                   // 중앙 두껍고 끝 뾰족
                    float ox  = fb->worldX + cosf(a) * rr;
                    float oy  = fb->worldY + sinf(a) * rr;
                    drawDiamond(ox, oy, dsz, scr*sb, scg*sb, scb*sb, 0.95f);
                }
            }
            BatchFlush(); glDisable(GL_SCISSOR_TEST);
        }
    
        // (g4e) C2_RELAY.sys — 코어(창 안) + 패킷 adds(가짜창 없음·단순 도형)
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
    
        // (g4f) FORK.worm — 플라즈마 체인: 가짜 창 scissor 안에 본체·adds·FX
        //   창마다 scissor 패스 → 다른 창에서도 보이되, 가짜창 밖 사막엔 안 그림.
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

        // (g4g) TOTEM.sys — 보스 코어·레이저 + 타 창 겹침 시 토템 재렌더
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

        // (g5) 폴리모프 보스 — 마커/세모/레이저/차크람/본체/HP
        if (g_PolyBoss && g_PolyBoss->alive) {
            auto* pb = g_PolyBoss;
            BindMainShader();
            const float PUR_R = 0.6f, PUR_G = 0.2f, PUR_B = 0.95f;
            // 세모 쇄도 경고 (텔레그래프) — 시작 모서리 전체에 깜빡이는 화살표/띠
            if (pb->triWarn) {
                // 페이즈2 줌아웃 확장 영역에 맞춰 경고를 '보이는' 끝까지 확장
                float exX = pb->arenaExX(), exY = pb->arenaExY();
                float fullW = (float)screenWidth + 2*exX, fullH = (float)screenHeight + 2*exY;
                float blink = 0.35f + 0.35f * (0.5f + 0.5f * sinf((float)glfwGetTime() * 16.0f));
                int arrows = 14;
                for (int i = 0; i < arrows; i++) {
                    float t = (arrows > 1) ? (float)i / (arrows - 1) : 0.5f;
                    float ax, ay;
                    if (pb->triDirX != 0.0f) {   // 가로 이동 → 좌/우 (확장)모서리에 세로 배열
                        ax = (pb->triDirX > 0) ? -exX + 24.0f
                                               : (float)screenWidth + exX - 24.0f;
                        ay = -exY + t * fullH;
                    } else {                     // 세로 이동 → 상/하 (확장)모서리에 가로 배열
                        ax = -exX + t * fullW;
                        ay = (pb->triDirY > 0) ? -exY + 24.0f
                                               : (float)screenHeight + exY - 24.0f;
                    }
                    // 진행 방향을 가리키는 작은 세모
                    drawTriangle(ax + pb->triDirX * 6.0f, ay + pb->triDirY * 6.0f,
                                 14.0f, 1.0f, 0.4f, 1.0f, blink);
                }
                // 뒷배경에 옅게 깔리는 대형 방향 화살표(쉐브론) — 진행 경로 안내
                float bgA = 0.08f + 0.05f * (0.5f + 0.5f * sinf((float)glfwGetTime() * 8.0f));
                for (int c = 0; c < 4; c++) {
                    float u = (c + 0.5f) / 4.0f;   // 진행축 방향 위치 비율
                    float cx, cy;
                    if (pb->triDirX != 0.0f) {     // 가로 이동 → 확장 폭을 따라 배치
                        cx = (pb->triDirX > 0) ? -exX + u * fullW
                                               : (float)screenWidth + exX - u * fullW;
                        cy = screenHeight * 0.5f;
                    } else {                       // 세로 이동
                        cx = screenWidth * 0.5f;
                        cy = (pb->triDirY > 0) ? -exY + u * fullH
                                               : (float)screenHeight + exY - u * fullH;
                    }
                    drawTriangle(cx + pb->triDirX * 30.0f, cy + pb->triDirY * 30.0f,
                                 80.0f, 0.8f, 0.4f, 1.0f, bgA);
                }
            }
            // 세모 무리
            for (auto& s : pb->swarm)
                drawTriangle(s.x, s.y, 11.0f, 0.7f, 0.3f, 1.0f, 1.0f);
            // 레이저 경고선 (발사 전) — 얇은 보라 점멸선
            if (pb->laserWarn) {
                float ex = pb->laserX + pb->laserDirX * pb->laserReach();
                float ey = pb->laserY + pb->laserDirY * pb->laserReach();
                float pxx = -pb->laserDirY, pyy = pb->laserDirX;
                float th = 3.0f;
                float warnA = 0.4f + 0.4f * (0.5f + 0.5f * sinf((float)glfwGetTime() * 18.0f));
                float p1x=pb->laserX+pxx*th, p1y=pb->laserY+pyy*th;
                float p2x=pb->laserX-pxx*th, p2y=pb->laserY-pyy*th;
                float p3x=ex+pxx*th, p3y=ey+pyy*th, p4x=ex-pxx*th, p4y=ey-pyy*th;
                float v[12]={p1x,p1y,p2x,p2y,p3x,p3y, p2x,p2y,p4x,p4y,p3x,p3y};
                BatchVerts(v, 6, 1.0f, 0.3f, 1.0f, warnA);
            }
            // 레이저 (RHOMBUS) — 어두운 시야 밴드 + 밝은 코어
            if (pb->laserActive) {
                float ex = pb->laserX + pb->laserDirX * pb->laserReach();
                float ey = pb->laserY + pb->laserDirY * pb->laserReach();
                float pxx = -pb->laserDirY, pyy = pb->laserDirX;
                for (int pass = 0; pass < 2; pass++) {
                    float th = (pass == 0) ? pb->laserHalf : 5.0f;
                    float cr = (pass == 0) ? 0.0f : 1.0f;
                    float cg = (pass == 0) ? 0.0f : 0.4f;
                    float cb = (pass == 0) ? 0.0f : 1.0f;
                    float ca = (pass == 0) ? 0.82f : 0.95f;
                    float p1x=pb->laserX+pxx*th, p1y=pb->laserY+pyy*th;
                    float p2x=pb->laserX-pxx*th, p2y=pb->laserY-pyy*th;
                    float p3x=ex+pxx*th, p3y=ey+pyy*th, p4x=ex-pxx*th, p4y=ey-pyy*th;
                    float v[12]={p1x,p1y,p2x,p2y,p3x,p3y, p2x,p2y,p4x,p4y,p3x,p3y};
                    BatchVerts(v, 6, cr, cg, cb, ca);
                }
            }
            // 본체 + 차크람 + HP + 총알 — 개인 창 영역으로 클리핑 (맨 배경에 떠 보이지 않게)
            BatchFlush(); glEnable(GL_SCISSOR_TEST);
            WorldScissor(pb->worldX - POLY_WIN_W*0.5f, pb->worldY - POLY_WIN_W*0.5f,
                         POLY_WIN_W, POLY_WIN_W);
            // 총알 (이 창 안에서도 보이도록)
            for (auto& b : g_Bullets) {
                if (!b.active) continue;
                drawBullet(b);
            }
            // 본체 — 폼별 모양 (큼)
            float bsz = PolymorphBoss::BODY;
            if (pb->form == PForm::TRIANGLE)
                drawTriangle(pb->worldX, pb->worldY, bsz, PUR_R, PUR_G, PUR_B, 1.0f);
            else
                drawDiamond(pb->worldX, pb->worldY, bsz, PUR_R, PUR_G, PUR_B, 1.0f);
            if (pb->reflecting())  // 반사 오라
                drawCircle(pb->worldX, pb->worldY, bsz * 0.95f, 1.0f, 1.0f, 1.0f, 0.18f);
            // 차크람 (다이아몬드 방어막)
            for (auto& c : pb->chakrams) {
                if (!c.alive) continue;
                float cx = pb->worldX + cosf(c.angle) * 150.0f;
                float cy = pb->worldY + sinf(c.angle) * 150.0f;
                drawDiamond(cx, cy, 30.0f, 0.7f, 0.3f, 1.0f, 1.0f);
                float cf = c.hp / 1000.0f; if (cf < 0) cf = 0;
                drawRect(cx - 18, cy - 34, 36.0f, 4.0f, 0.2f, 0.1f, 0.2f, 0.8f);
                drawRect(cx - 18, cy - 34, 36.0f * cf, 4.0f, 0.8f, 0.4f, 1.0f, 0.9f);
            }
            // (HP 바는 화면 상단 고정 보스 바로 이동)
            // 폼 변환 파티클 — 보스 개인 창 안에서도 보이도록 (창 밖 데스크톱엔 안 뜸)
            for (auto& p : g_EnemyParts) {
                if (!p.active) continue;
                float a = p.life / p.maxLife, hs = p.size * 0.5f;
                drawRect(p.x - hs, p.y - hs, p.size, p.size, p.r, p.g, p.b, a);
            }
            BatchFlush(); glDisable(GL_SCISSOR_TEST);
        }
    
        // 드론/차크람은 데스크톱 최상단(클립 없음)에 그린다 — 위쪽 보스창 scissor 패스가
        //   poly 보스 없을 땐 안 닫혀 드론/차크람이 통째로 클립되던 버그 방지(무조건 해제).
        BatchFlush(); glDisable(GL_SCISSOR_TEST); BindMainShader();
    
        // (h) 드론 — 1~2기 (포탑 모드 시 드론 렌더 비활성)
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
    
        // (h2) 차크람 — 1~3개
        if (g_Stats.chakram && g_GameManager.currentState != GameState::GAMEOVER) {
            float pCX = playerWin.x + playerWin.width  * 0.5f;
            float pCY = playerWin.y + playerWin.height * 0.5f;
            for (int c = 0; c < g_Stats.chakramCount && c < MAX_CHAKRAMS; c++) {
                auto& ch = g_Chakrams[c];
                if (!ch.alive) continue;
                float chx = pCX + cosf(ch.angle) * CHAKRAM_RADIUS;
                float chy = pCY + sinf(ch.angle) * CHAKRAM_RADIUS;
                // 시안 톱날 디스크 — 스파이웨어(노란 십자)와 확실히 구분
                drawCircle(chx, chy, CHAKRAM_SIZE * 0.62f, 0.3f, 0.9f, 1.0f, 0.28f);   // 글로우
                drawDiamond(chx, chy, CHAKRAM_SIZE * 1.15f, 0.5f, 1.0f, 1.0f, 0.45f);  // 회전날 힌트
                drawCircle(chx, chy, CHAKRAM_SIZE * 0.5f, 0.2f, 0.85f, 1.0f, 1.0f);    // 시안 디스크
                drawCircle(chx, chy, CHAKRAM_SIZE * 0.22f, 0.04f, 0.12f, 0.18f, 1.0f); // 어두운 허브
                // HP 바
                float hpFrac = ch.hp / ch.maxHp;
                if (hpFrac < 0) hpFrac = 0; if (hpFrac > 1) hpFrac = 1;
                drawRect(chx - 14, chy - CHAKRAM_SIZE - 6, 28, 3, 0.2f, 0.2f, 0.2f, 0.7f);
                drawRect(chx - 14, chy - CHAKRAM_SIZE - 6, 28 * hpFrac, 3,
                         1.0f, 0.7f, 0.0f, 0.95f);
            }
        }
        // 드론/차크람 배치를 지금 즉시 flush — 바로 아래 타이틀바 패스가 scissor 를
        //   재활성(직전 작은 창 rect)하면 미flush 지오메트리가 통째로 클립되던 진짜 원인.
        BatchFlush();
    
        // ── (h3) 가짜 OS 창 크롬 — 타이틀바 + [X] 닫기 (데스크톱 세계관) ──
        //    "적 = 프로세스, 창을 닫아 종료한다" 정체성. 월드 좌표(줌 반영)로 그림.
        {
            GameState gst = g_GameManager.currentState;
            bool inGame = (gst == GameState::RUNNING || gst == GameState::PAUSED ||
                           gst == GameState::DYING || gst == GameState::AUG_SELECT ||
                           gst == GameState::DEBUFF_SELECT);
            if (inGame) {
                auto winChrome = [&](float x, float y, float w, float h,
                                     const wchar_t* title, float tr, float tg, float tb) {
                    BindMainShader();
                    const float TB = 22.0f;
                    drawRect(x, y, w, TB, tr*0.45f, tg*0.45f, tb*0.55f, 1.0f);   // 타이틀바
                    drawRect(x, y, w, 2.0f, tr, tg, tb, 1.0f);                   // 상단 강조선
                    // (창 외곽선 = 아래/좌/우 테두리선 제거 — 창 끝에서 그려 플레이어 창 위로
                    //  삐져나오던 문제. 외곽 프레임은 drawNeonBorder(플레이어 배경보다 먼저
                    //  그려져 겹친 부분이 올바르게 가려짐)가 담당.)
                    // 창 컨트롤 (─ □ X) 우측
                    float bs = 13.0f, byc = y + (TB-bs)*0.5f, bxc = x + w - 19.0f;
                    drawRect(bxc - 2*(bs+5), byc, bs, bs, 0.22f,0.22f,0.28f,0.9f); // ─
                    drawRect(bxc - (bs+5),   byc, bs, bs, 0.22f,0.22f,0.28f,0.9f); // □
                    drawRect(bxc, byc, bs, bs, 0.85f, 0.2f, 0.2f, 0.95f);          // X = 빨강
                    // 텍스트는 TextRenderer 자체 스크린 ortho(줌 무시)라, 월드 좌표를
                    //   W2SX/W2SY 로 변환 + g_ViewZoom 스케일 → 줌된 창에 정확히 붙음
                    g_TextS.Draw(L"X", W2SX(bxc + 3.0f), W2SY(byc - 2.0f),
                                 0.5f * g_ViewZoom, 1,1,1, 0.95f);
                    g_TextS.Draw(title, W2SX(x + 8.0f), W2SY(y + 3.0f),
                                 0.55f * g_ViewZoom, 0.92f,0.96f,1.0f, 0.95f);
                };
                int li2 = LangIndex();
                const wchar_t* PNAME = (li2==0) ? L"onedow.exe" : L"onedow.exe";
                // 가짜창 타이틀바 — 위에서 만든 z-리스트(낮음→높음) 순서로 그림.
                //   '자기보다 높은 창' 또는 '플레이어 창'이 타이틀바를 덮으면, 전체를
                //   숨기는 게 아니라 **겹친 가로 구간만** 잘라낸다(부분 클리핑).
                //   타이틀바의 세로 띠[y,y+TB]와 겹치는 가림창의 x구간을 가시구간에서 빼고,
                //   남은 구간들만 glScissor 로 클립해 그린다. (봇넷<원거리<보스<플레이어,
                //   같은 타입은 소환순서)
                const float TBH = 22.0f;
                glEnable(GL_SCISSOR_TEST);
                auto drawBarClipped = [&](float x, float y, float w, float h,
                                          const wchar_t* nm, float nr, float ng, float nb,
                                          size_t selfIdx, bool isPlayer) {
                    // 가시 x구간 리스트 (각 vec2: x=시작, y=끝)
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
                        if (oy < y + TBH && oy + oh > y)   // 가림창이 타이틀바 세로 띠와 겹침
                            subtract(std::max(x, ox), std::min(x + w, ox + ow));
                    };
                    // 플레이어 창은 모든 가짜창보다 위 — 가짜창 그릴 땐 항상 가림
                    if (!isPlayer)
                        consider(playerWin.x, playerWin.y, playerWin.width, playerWin.height);
                    // 자기보다 높은 z(뒤 인덱스) 가짜창들
                    for (size_t j = selfIdx + 1; j < zwins.size(); j++)
                        consider(zwins[j].x, zwins[j].y, zwins[j].w, zwins[j].h);
                    // 남은 구간만 클립해 그림
                    for (auto& iv : vis) {
                        float a = iv.x, b = iv.y;
                        if (b - a < 0.5f) continue;
                        WorldScissor(a, y, b - a, TBH);
                        winChrome(x, y, w, h, nm, nr, ng, nb);
                    }
                };
                for (size_t i = 0; i < zwins.size(); i++) {
                    const FWin& fw = zwins[i];
                    drawBarClipped(fw.x, fw.y, fw.w, fw.h, fw.name,
                                   fw.nr, fw.ngc, fw.nbc, i, false);
                }
                BatchFlush();
                // 플레이어 창 — 항상 최상단, 클립 없이 전체
                glScissor(0, 0, (GLint)screenWidth, (GLint)screenHeight);
                winChrome(playerWin.x, playerWin.y, playerWin.width, playerWin.height,
                          PNAME, g_AccentR, g_AccentG, g_AccentB);
                BatchFlush();
                glDisable(GL_SCISSOR_TEST);
            }
        }

    
        // ── 여기부터 UI/오버레이: 줌·흔들기 무시하고 화면 고정 좌표(base ortho)로 ──
        //    (폴리모프 2페이즈 줌 0.5 에서 쿨다운칸·메뉴딤·비네트·플래시가 찌그러지던 버그 fix)
        BatchFlush();   // 월드(줌 ortho) 도형 전부 그린 뒤 base ortho 로 전환
        glUniformMatrix4fv(g_MainProjLoc, 1, GL_FALSE, g_BaseOrtho);
        memcpy(g_MainOrtho, g_BaseOrtho, sizeof(g_BaseOrtho));
    
        // (h2) 보스 고유색 화면 물들이기 — 보스 생존 중 서서히 차오르고, 처치 후 사라짐
        {
            bool bossAlive = false;
            glm::vec3 tc(0.6f, 0.3f, 1.0f);
            if ((g_MonsterManager.boss && g_MonsterManager.boss->alive) ||
                !g_LeakNodes.empty()) {
                bossAlive = true; tc = glm::vec3(0.45f, 0.55f, 1.0f); }   // LEAK/노드: 청보라
            else if (g_RRBoss && g_RRBoss->alive) {
                bossAlive = true; tc = glm::vec3(1.0f, 0.55f, 0.2f); }     // 리로드러너: 주황
            else if (g_PolyBoss && g_PolyBoss->alive) {
                bossAlive = true; tc = glm::vec3(0.6f, 0.25f, 1.0f); }     // 폴리모프: 보라
            else if (g_SpamBoss && g_SpamBoss->alive) {
                bossAlive = true; tc = glm::vec3(1.0f, 0.4f, 0.8f); }      // 스팸: 핑크
    
            if (bossAlive) {
                g_BossTintCol = tc;
                g_BossTintT  += delta * 0.07f;          // ~14초에 최대
                if (g_BossTintT > 1.0f) g_BossTintT = 1.0f;
            } else {
                g_BossTintT  -= delta * 0.6f;           // 처치 후 빠르게 원복
                if (g_BossTintT < 0.0f) g_BossTintT = 0.0f;
            }
            if (g_BossTintT > 0.001f) {
                BindMainShader();
                drawRect(0, 0, (float)screenWidth, (float)screenHeight,
                         g_BossTintCol.r, g_BossTintCol.g, g_BossTintCol.b,
                         g_BossTintT * 0.06f);
            }
        }
    
        // (h2b) 보스 레이드 HP 바 — 화면 상단 고정. 몸체 밑 작은 바는 후반 탄막에
        //   묻혀 안 보이므로, 활성 보스의 체력을 상단에 크게 표시한다 (이름 + %).
        {
            const wchar_t* bn = nullptr;
            float bhf = 0.0f; glm::vec3 bc(1.0f, 1.0f, 1.0f);
            if (g_MonsterManager.boss && g_MonsterManager.boss->alive) {
                bn = L"LEAK.sys";    bhf = g_MonsterManager.boss->hp / g_MonsterManager.boss->maxHp;
                bc = glm::vec3(0.45f, 0.55f, 1.0f);
            } else if (g_RRBoss && g_RRBoss->alive) {
                bn = L"VOLLEY.sys";  bhf = g_RRBoss->hp / g_RRBoss->maxHp;
                bc = glm::vec3(1.0f, 0.55f, 0.2f);
            } else if (g_PolyBoss && g_PolyBoss->alive) {
                bn = L"POLYMORPH.vir"; bhf = g_PolyBoss->hp / g_PolyBoss->maxHp;
                bc = glm::vec3(0.6f, 0.25f, 1.0f);
            } else if (g_SpamBoss && g_SpamBoss->alive) {
                bn = L"SPAM.dll";      bhf = g_SpamBoss->hp / g_SpamBoss->maxHp;
                bc = glm::vec3(1.0f, 0.4f, 0.8f);
            } else if (g_KernelBoss && g_KernelBoss->alive) {
                bn = L"KERNEL.sys";    bhf = g_KernelBoss->hp / g_KernelBoss->maxHp;
                bc = glm::vec3(1.0f, 0.65f, 0.25f);
            } else if (g_FirewallBoss && g_FirewallBoss->alive) {
                bn = L"FIREWALL.sys";  bhf = g_FirewallBoss->hp / g_FirewallBoss->maxHp;
                bc = glm::vec3(1.0f, 0.45f, 0.2f);
            } else if (g_BotnetBoss && g_BotnetBoss->alive) {
                bn = L"C2_RELAY.sys";  bhf = g_BotnetBoss->hp / g_BotnetBoss->maxHp;
                bc = glm::vec3(0.25f, 0.92f, 0.48f);
            } else if (g_CentiBoss && g_CentiBoss->alive) {
                bn = CentipedeBoss::BOSS_NAME;  bhf = g_CentiBoss->hp / g_CentiBoss->maxHp;
                bc = glm::vec3(0.35f, 0.88f, 0.95f);
            } else if (g_TotemBoss && g_TotemBoss->alive) {
                bn = L"RITE.CORE";  bhf = g_TotemBoss->hp / g_TotemBoss->maxHp;
                bc = glm::vec3(0.85f, 0.45f, 0.95f);
            }
            int bossPick = -1;
            if (bn) {
                if      (bn == L"LEAK.sys")     bossPick = 0;
                else if (bn == L"VOLLEY.sys")   bossPick = 2;
                else if (bn == L"SPAM.dll")       bossPick = 3;
                else if (bn == L"POLYMORPH.vir")  bossPick = 4;
                else if (bn == L"KERNEL.sys")     bossPick = 5;
                else if (bn == L"FIREWALL.sys")   bossPick = 6;
                else if (bn == L"C2_RELAY.sys")   bossPick = 7;
                else if (bn == CentipedeBoss::BOSS_NAME) bossPick = 8;
                else if (bn == L"RITE.CORE")             bossPick = 9;
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
                // 체력 — 낮을수록 어두워지는 보스 고유색
                float lit = 0.5f + 0.5f * bhf;
                drawRect(bx, by, bw * bhf, bh, bc.r * lit, bc.g * lit, bc.b * lit, 0.96f);
                // 이름 (바 위 중앙)
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
                if (g_MonsterManager.boss && g_MonsterManager.boss->alive) {
                    const wchar_t* stg = g_MonsterManager.boss->stateTag();
                    float ss = 0.62f;
                    float sw = g_TextS.Width(stg, ss);
                    g_TextS.Draw(stg, bx + bw - sw - 10.0f, by - 28.0f, ss,
                                 0.45f, 0.55f, 1.0f, 0.92f);
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
                        swprintf_s(rrBuf, L"OVERCLOCK · %ls",
                                   g_RRBoss->state == RRState::ASSAULT ? L"ASSAULT" :
                                   ReloadRunnerBoss::weaponTag(g_RRBoss->weapon));
                    else if (g_RRBoss->phase2)
                        swprintf_s(rrBuf, L"P2 · %ls",
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
                        swprintf_s(totBuf, L"W%d · %d/4", g_TotemBoss->wave, g_TotemBoss->aliveTotems());
                    float ts = 0.55f;
                    float tw = g_TextS.Width(totBuf, ts);
                    g_TextS.Draw(totBuf, bx + bw - tw - 8.0f, by - 48.0f, ts,
                                 0.85f, 0.45f, 0.95f, 0.85f);
                }
                // % (바 우측 끝 안쪽)
                wchar_t pct[16]; swprintf_s(pct, L"%d%%", (int)(bhf * 100.0f + 0.5f));
                float ps = 0.7f;
                float pw = g_TextS.Width(pct, ps);
                g_TextS.Draw(pct, bx + bw - pw - 8.0f, by + 3.0f, ps, 1.0f, 1.0f, 1.0f, 0.95f);
            }
        }
    
        // (h2c) 보스 등장 전조(증상) — 스폰 2.5초 전 테마 연출 + 경고 배너
        if (g_BossWarnTimer > 0.0f &&
            (g_GameManager.currentState == GameState::RUNNING ||
             g_GameManager.currentState == GameState::PAUSED)) {
            float prog = 1.0f - g_BossWarnTimer / BOSS_WARN_DUR;   // 0→1
            if (prog < 0.0f) prog = 0.0f; if (prog > 1.0f) prog = 1.0f;
            float t = (float)glfwGetTime();
            float blink = 0.5f + 0.5f * sinf(t * 9.0f);
            // 보스 고유색
            glm::vec3 wc = BossDir::WarnColor(g_BossWarnPick);
            BindMainShader();
            float sw2 = (float)screenWidth, sh2 = (float)screenHeight;
            // 공통: 가장자리 비네트 (보스색, progress 비례로 짙어짐)
            float ea = (0.05f + 0.13f * prog) * (0.6f + 0.4f * blink);
            float eb = 70.0f;
            drawRect(0, 0, sw2, eb, wc.r, wc.g, wc.b, ea);
            drawRect(0, sh2 - eb, sw2, eb, wc.r, wc.g, wc.b, ea);
            drawRect(0, 0, eb, sh2, wc.r, wc.g, wc.b, ea);
            drawRect(sw2 - eb, 0, eb, sh2, wc.r, wc.g, wc.b, ea);
    
            // 보스별 증상 테마
            switch (g_BossWarnPick) {
            case 0: {  // LEAK — RAM 미터 차오름 + 누수 바이트
                float rise = (30.0f + 130.0f * prog);
                drawRect(0, sh2 - rise, sw2, rise, 0.12f, 0.18f, 0.42f, 0.10f + 0.12f * prog);
                for (int i = 0; i < 9; i++) {
                    float bx = 24.0f + (float)(i % 3) * (sw2 * 0.32f);
                    float by = sh2 - rise - 20.0f - (float)(i / 3) * 38.0f;
                    float bh = 12.0f + (float)(rand() % 40) * prog;
                    drawRect(bx, by - bh, 18.0f, bh, 0.35f, 0.55f, 1.0f, 0.22f);
                }
                float meterW = sw2 * 0.55f;
                drawRect(sw2 * 0.5f - meterW * 0.5f, 36.0f, meterW, 14.0f, 0.08f, 0.08f, 0.12f, 0.6f);
                drawRect(sw2 * 0.5f - meterW * 0.5f, 36.0f, meterW * prog, 14.0f,
                         0.45f, 0.55f, 1.0f, 0.75f);
            } break;
            case 1: break;
            case 2: {  // VOLLEY — 교차 포격선 + 드론 실루엣
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
            case 3: {  // SPAM — 분홍 popup.exe 창들이 깜빡이며 증식
                int pops = 3 + (int)(prog * 8);
                for (int i = 0; i < pops; i++) {
                    float pw2 = 70.0f + (float)(rand() % 90);
                    float ph2 = 50.0f + (float)(rand() % 60);
                    float px2 = (float)(rand() % (int)(sw2 - pw2));
                    float py2 = (float)(rand() % (int)(sh2 - ph2));
                    drawRect(px2, py2, pw2, ph2, 0.10f, 0.06f, 0.09f, 0.5f);
                    drawRect(px2, py2, pw2, 12.0f, 1.0f, 0.4f, 0.8f, 0.7f);  // 타이틀바
                }
            } break;
            case 4: {  // POLYMORPH — 중앙에서 퍼지는 보라 맥동 링
                for (int i = 0; i < 3; i++) {
                    float r = (80.0f + i * 120.0f) + prog * 200.0f;
                    drawCircle(sw2 * 0.5f, sh2 * 0.5f, r, wc.r, wc.g, wc.b,
                               (0.05f + 0.05f * blink) * (1.0f - (float)i * 0.25f));
                }
            } break;
            case 5: {  // KERNEL — BSOD 청색 번쩍 + 가로 스트라이프
                drawRect(0, 0, sw2, sh2, 0.05f, 0.12f, 0.45f, 0.08f + 0.12f * prog);
                int stripes = 6 + (int)(prog * 8);
                for (int i = 0; i < stripes; i++) {
                    float sy = (float)(rand() % (int)sh2);
                    drawRect(0, sy, sw2, 3.0f + (float)(rand() % 6),
                             0.2f, 0.5f, 1.0f, 0.12f + 0.15f * blink);
                }
            } break;
            case 6: {  // FIREWALL — 붉은 차단 벽이 좌우에서 좁혀짐
                float wall = (30.0f + 120.0f * prog);
                drawRect(0, 0, wall, sh2, 1.0f, 0.25f, 0.15f, 0.15f + 0.12f * prog);
                drawRect(sw2 - wall, 0, wall, sh2, 1.0f, 0.25f, 0.15f, 0.15f + 0.12f * prog);
                for (int i = 0; i < 5; i++) {
                    float ly = sh2 * (0.15f + 0.17f * i);
                    drawRect(wall - 8.0f, ly, sw2 - 2.0f * wall + 16.0f, 4.0f,
                             1.0f, 0.5f, 0.2f, 0.35f * blink);
                }
            } break;
            case 7: {  // C2_RELAY — 터미널 로그 스크롤 + 녹색 노드 링
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
            case 8: {  // FORK — 하단에서 기어오는 분절 몸통
                float crawl = 40.0f + 90.0f * prog;
                for (int s = 0; s < 9; s++) {
                    float sx = sw2 * 0.08f + s * sw2 * 0.105f;
                    float bob = sinf(t * 6.0f + s * 0.7f) * 6.0f;
                    drawCircle(sx, sh2 - crawl + bob, 14.0f + (float)(s % 3) * 3.0f,
                               0.35f, 0.85f, 0.25f, 0.2f + 0.15f * prog);
                }
            } break;
            case 9: {  // RITE — 궤도 링 + 기둥 + 육각 코어
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
    
            // 공통 경고 배너 (중앙 상단쪽)
            float by3 = sh2 * 0.30f;
            // 이름 (대)
            wchar_t banner[64]; swprintf_s(banner, L"⚠  %ls  ⚠", g_BossWarnName);
            float nsc = 1.5f;
            float nw2 = g_TextL.Width(banner, nsc);
            g_TextL.Draw(banner, (sw2 - nw2) * 0.5f, by3, nsc,
                         wc.r, wc.g, wc.b, 0.7f + 0.3f * blink);
            // 부제
            const wchar_t* SUB[3] = { L"위협 프로세스 감지 — 실행 중...",
                                       L"THREAT PROCESS DETECTED — launching...",
                                       L"脅威プロセス検出 — 実行中..." };
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
    
        // (h2d) 페이즈2 진입 토스트 — "■ 과부하 — PHASE 2" (1.8초 페이드)
        if (g_P2ToastTimer > 0.0f &&
            (g_GameManager.currentState == GameState::RUNNING ||
             g_GameManager.currentState == GameState::DYING)) {
            float a = (g_P2ToastTimer > 1.4f) ? (1.8f - g_P2ToastTimer) / 0.4f
                                              : (g_P2ToastTimer / 1.4f);
            if (a > 1.0f) a = 1.0f; if (a < 0.0f) a = 0.0f;
            const wchar_t* P2[3] = { L"■ 과부하 — PHASE 2", L"■ OVERLOAD — PHASE 2",
                                      L"■ 過負荷 — PHASE 2" };
            int li6 = LangIndex();
            float psc = 1.2f;
            float pw3 = g_TextL.Width(P2[li6], psc);
            glm::vec3& pc = g_P2ToastCol;
            g_TextL.Draw(P2[li6], ((float)screenWidth - pw3) * 0.5f,
                         (float)screenHeight * 0.22f, psc, pc.r, pc.g, pc.b, a);
        }
    
        // (h3) 화면 플래시 — 레벨업/보스처치/부활 순간 번쩍
        if (g_FlashIntensity > 0.001f) {
            BindMainShader();
            float a = g_FlashIntensity; if (a > 0.85f) a = 0.85f;
            drawRect(0, 0, (float)screenWidth, (float)screenHeight,
                     g_FlashColor.r, g_FlashColor.g, g_FlashColor.b, a);
        }
        // FORK.worm 탈피 — 저가형 RGB 분리 글리치
        if (g_CentiBoss && g_CentiBoss->glitchOverlay > 0.001f) {
            BindMainShader();
            float go = g_CentiBoss->glitchOverlay * 7.0f;
            if (go > 1.0f) go = 1.0f;
            float off = 7.0f * go;
            float sw = (float)screenWidth, sh = (float)screenHeight;
            drawRect(off, 0.0f, sw, sh, 1.0f, 0.15f, 0.15f, go * 0.07f);
            drawRect(-off, 0.0f, sw, sh, 0.15f, 0.85f, 1.0f, go * 0.07f);
        }
    
        // (h4) 시간 정지 — 화면 가장자리 시안 비네트 (정지 연출)
        if (g_TimeStopTimer > 0.0f) {
            BindMainShader();
            float a = 0.10f + 0.05f * sinf((float)glfwGetTime() * 10.0f);
            float bw = 18.0f;
            drawRect(0, 0, (float)screenWidth, bw, 0.3f, 0.9f, 1.0f, a);
            drawRect(0, (float)screenHeight - bw, (float)screenWidth, bw, 0.3f, 0.9f, 1.0f, a);
            drawRect(0, 0, bw, (float)screenHeight, 0.3f, 0.9f, 1.0f, a);
            drawRect((float)screenWidth - bw, 0, bw, (float)screenHeight, 0.3f, 0.9f, 1.0f, a);
        }
    
        // (h5) 피격 — 빨간 가장자리 비네트 (피격 펀치)
        if (g_HurtVignette > 0.001f) {
            BindMainShader();
            float a = g_HurtVignette * 0.55f;
            float bw = 60.0f * g_HurtVignette + 14.0f;
            drawRect(0, 0, (float)screenWidth, bw, 0.95f, 0.1f, 0.1f, a);
            drawRect(0, (float)screenHeight - bw, (float)screenWidth, bw, 0.95f, 0.1f, 0.1f, a);
            drawRect(0, 0, bw, (float)screenHeight, 0.95f, 0.1f, 0.1f, a);
            drawRect((float)screenWidth - bw, 0, bw, (float)screenHeight, 0.95f, 0.1f, 0.1f, a);
        }
    
        // (i) 취함 상태 표시 (화면 가장자리 자홍색 비네트)
        if (g_DrunkActive) {
            // 가장자리 4겹 사각 테두리 — alpha 빠르게 누적되며 비네트
            float a = 0.18f;
            drawRect(0, 0, (float)screenWidth, 12.0f, 0.9f, 0.1f, 0.6f, a);
            drawRect(0, (float)screenHeight - 12, (float)screenWidth, 12.0f, 0.9f, 0.1f, 0.6f, a);
            drawRect(0, 0, 12.0f, (float)screenHeight, 0.9f, 0.1f, 0.6f, a);
            drawRect((float)screenWidth - 12, 0, 12.0f, (float)screenHeight, 0.9f, 0.1f, 0.6f, a);
        }
    
        }   // if (inWorldRender)
        }   // 월드 렌더 게이트 블록
    
        // [6] HUD 오버레이 (HP바, 상태 표시)
        g_GameManager.Render();
    
        // ── [7] 한국어 텍스트 + 메뉴 ─────────────────────────────────────────
        {
            float sw = (float)screenWidth, sh = (float)screenHeight;
            auto  st = g_GameManager.currentState;
    
    
            // 앱 창(shop/codex/config) 진입 시 열림 애니메이션 시작 — 매 프레임 진행
            {
                static GameState s_prevWinSt = GameState::MAIN_MENU;
                bool isApp = (st == GameState::SHOP || st == GameState::CODEX ||
                              st == GameState::SETTINGS);
                if (st != s_prevWinSt) {
                    if (isApp) g_AppOpen = 0.0f;   // 새 창 → 크기 0에서 열기
                    s_prevWinSt = st;
                }
                if (isApp && g_AppOpen < 1.0f) {
                    g_AppOpen += delta / APP_OPEN_DUR;
                    if (g_AppOpen > 1.0f) g_AppOpen = 1.0f;
                }
            }
    
            // ── 인게임 작업표시줄 — 메뉴와 동일한 OS 프레임 유지 (데스크톱 방어 일관성) ──
            if (st == GameState::RUNNING || st == GameState::PAUSED || st == GameState::DYING ||
                st == GameState::AUG_SELECT || st == GameState::DEBUFF_SELECT) {
                DrawIngameTaskbar(sw, sh, st);
            }
    
            // ── 업적 해금 토스트 (상단 중앙 배너, 4초 표시 후 페이드) ──
            if (g_AchToastTimer > 0.0f && g_AchToastId >= 0 &&
                g_AchToastId < ACH_COUNT) {
                g_AchToastTimer -= delta;
                float a = g_AchToastTimer > 3.0f ? (4.0f - g_AchToastTimer) // 페이드인
                        : std::min(1.0f, g_AchToastTimer);                 // 페이드아웃
                if (a < 0.0f) a = 0.0f; if (a > 1.0f) a = 1.0f;
                int li2 = LangIndex();
                const wchar_t* LBL[3] = { L"업적 달성!", L"Achievement!", L"実績解除!" };
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

            // ── 크리에이티브 HUD (게임 중) — F:증강  G:무적 ──
            if (g_CreativeMode &&
                (st == GameState::RUNNING || st == GameState::READY ||
                 st == GameState::AUG_SELECT || st == GameState::DEBUFF_SELECT)) {
                int li3 = LangIndex();
                const wchar_t* CH[3] = {
                    L"CREATIVE   F: 증강(디버프 포함)   G: 무적",
                    L"CREATIVE   F: Augment(+debuff)   G: Godmode",
                    L"CREATIVE   F: 強化(デバフ含)   G: 無敵" };
                g_TextS.Draw(CH[li3], 20.0f, HudY(sh, Hud::CREATIVE_LABEL), 0.8f, 0.7f, 0.85f, 1.0f, 0.85f);
                if (g_CreativeGodmode) {
                    const wchar_t* GOD[3] = { L"● 무적 ON", L"● GODMODE ON", L"● 無敵 ON" };
                    float blink = 0.65f + 0.35f * sinf((float)glfwGetTime() * 5.0f);
                    g_TextL.Draw(GOD[li3], 20.0f, 24.0f, 0.95f, 1.0f, 0.85f, 0.2f, blink);
                }
            }
    
            // ── [7b] UI 씬 디스패치 — 메뉴/창 상태는 Scene_* 함수로 분리 ──
            //    RUNNING/DYING(순수 인게임)엔 씬이 없으니 컨텍스트 구성 자체를 건너뜀.
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
                default: break;
                }
                if (st == GameState::PAUSED || st == GameState::AUG_SELECT ||
                    st == GameState::DEBUFF_SELECT || st == GameState::GAMEOVER)
                    Scene_OwnedAugPanel(ctx);
            }
    
            // 상단 HUD — 실제 게임 진행 상태에서만 (메뉴/도감/상점엔 안 뜨게)
            if (st == GameState::RUNNING || st == GameState::PAUSED ||
                st == GameState::DYING   || st == GameState::AUG_SELECT ||
                st == GameState::DEBUFF_SELECT) {
                // macOS: 상단 메뉴바(~25px)가 화면 맨 위를 가리므로 HUD 를 아래로
    #ifdef __APPLE__
                const float hudTopY = 8.0f + 30.0f;
    #else
                const float hudTopY = 8.0f;
    #endif
                // 좌상단: Lv. + HP 숫자 (시각 바는 플레이어 창에 부착됨)
                {
                    int hpCur = (int)(g_GameManager.playerHP + 0.5f);
                    int hpMax = (int)(g_Stats.maxHP + 0.5f);
                    wchar_t lvBuf2[64];
                    swprintf_s(lvBuf2, L"%ls%d    HP %d/%d",
                               T(StrId::LV_PREFIX), g_GameManager.playerLevel, hpCur, hpMax);
                    g_TextS.Draw(lvBuf2, 12.0f, hudTopY, 0.85f, 0.7f, 1.0f, 0.7f, 0.9f);
                }
    
                // 상단 중앙: Score
                wchar_t scoreBuf[64];
                swprintf_s(scoreBuf, L"%ls  %lld", T(StrId::SCORE), g_GameManager.score);
                float scoreW = g_TextS.Width(scoreBuf, 1.0f);
                g_TextS.Draw(scoreBuf, (sw - scoreW) * 0.5f, hudTopY, 1.0f,
                             1.0f, 1.0f, 1.0f, 0.95f);
    
                // 우상단: FPS
                wchar_t fpsBuf[32];
                swprintf_s(fpsBuf, L"%ls  %d", T(StrId::FPS), g_CurrentFPS);
                float fpsW = g_TextS.Width(fpsBuf, 0.85f);
                g_TextS.Draw(fpsBuf, sw - fpsW - 12.0f, hudTopY, 0.85f,
                             0.7f, 0.9f, 1.0f, 0.85f);
    
                // (HP/EXP 시각 바는 플레이어 창 하단에 부착됨 — 위 (c2) 참고)
                // 하단 조작 안내 (희미하게 항상) — 새 플레이어가 HP/스킬 위치를 알게
                if (st == GameState::RUNNING) {
                    const wchar_t* c =
                        (g_Language == Language::KR)
                            ? L"WASD 이동   마우스 발사   SHIFT 대시   Q/E/R 스킬   ESC 일시정지"
                        : (g_Language == Language::JP)
                            ? L"WASD 移動   マウス 射撃   SHIFT ダッシュ   Q/E/R スキル   ESC 一時停止"
                            : L"WASD Move   Mouse Fire   SHIFT Dash   Q/E/R Skills   ESC Pause";
                    float cw = g_TextS.Width(c, 0.7f);
                    g_TextS.Draw(c, CenterX(sw, cw), HudY(sh, Hud::COMBO_TEXT), 0.7f,
                                 0.7f, 0.82f, 0.95f, 0.72f);   // 가시성 ↑ (옅어서 안 보인다는 피드백)
                }
                // ── 저체력 경고 — HP 25% 이하 시 가장자리 부드러운 적색 펄스 + 텍스트 ──
                if (st == GameState::RUNNING || st == GameState::PAUSED) {
                    float hpFrac = (g_Stats.maxHP > 0.0f)
                                 ? g_GameManager.playerHP / g_Stats.maxHP : 1.0f;
                    if (hpFrac > 0.0f && hpFrac < 0.25f) {
                        float pulse = 0.5f + 0.5f * sinf((float)glfwGetTime() * 6.0f);
                        float sev   = 1.0f - hpFrac / 0.25f;            // 낮을수록 강하게
                        float a     = (0.08f + 0.13f * pulse) * (0.5f + 0.5f * sev);
                        BindMainShader();
                        float bw = 64.0f;
                        drawRect(0, 0, sw, bw, 0.9f, 0.15f, 0.15f, a);
                        drawRect(0, sh - bw, sw, bw, 0.9f, 0.15f, 0.15f, a);
                        drawRect(0, 0, bw, sh, 0.9f, 0.15f, 0.15f, a);
                        drawRect(sw - bw, 0, bw, sh, 0.9f, 0.15f, 0.15f, a);
                        const wchar_t* LOW[3] = { L"● 위험", L"● LOW HP", L"● 危険" };
                        int li4 = LangIndex();
                        float lw = g_TextS.Width(LOW[li4], 0.9f);
                        g_TextS.Draw(LOW[li4], CenterX(sw, lw), HudY(sh, Hud::LOW_HP_WARN),
                                     0.9f, 1.0f, 0.4f, 0.4f, 0.55f + 0.45f * pulse);
                    }
                }
            }
    
            // ── 손맛: 데미지 숫자 팝업 + 콤보 카운터 (실제 플레이 중에만) ──
            //    메뉴/일시정지에선 숨김 — 텍스트가 메뉴 위에 남던 버그 fix
            if (st == GameState::RUNNING || st == GameState::DYING) {
                // 데미지 숫자 (월드 → 스크린 변환 후 텍스트) — 설정 토글
                if (g_ShowDamageNumbers)
                for (auto& d : g_DmgNumbers) {
                    float t  = d.life / d.maxLife;                   // 1 → 0
                    float sx = W2SX(d.x), sy = W2SY(d.y);
                    wchar_t nb[16]; swprintf_s(nb, L"%d", d.amount);
                    float sc = (d.crit ? 1.05f : 0.72f) * g_ViewZoom * (0.7f + 0.3f * t);
                    float a  = (t > 0.55f) ? 1.0f : (t / 0.55f);
                    float w  = g_TextS.Width(nb, sc);
                    if (d.crit) g_TextS.Draw(nb, sx - w*0.5f, sy, sc, 1.0f, 0.85f, 0.2f, a);
                    else        g_TextS.Draw(nb, sx - w*0.5f, sy, sc, 1.0f, 1.0f, 1.0f, a*0.9f);
                }
                // 콤보 카운터 (5콤보 이상부터, 색이 콤보에 따라 강해짐) — 설정 토글
                if (g_ShowCombo && st == GameState::RUNNING && g_Combo >= 5) {
                    bool  ms      = (g_ComboMilestone > 0.0f);   // 마일스톤 강조 중
                    float msBoost = ms ? (g_ComboMilestone / 0.7f) : 0.0f;  // 1→0
                    wchar_t cb[32]; swprintf_s(cb, L"%d COMBO", g_Combo);
                    float sc = (1.05f + (g_Combo > 30 ? 0.3f : 0.0f))
                             * (1.0f + g_ComboPulse * 0.4f + msBoost * 0.55f);
                    float cr = 1.0f, cg = 1.0f, cbl = 1.0f;
                    if      (g_Combo >= 50) { cg = 0.25f; cbl = 0.2f; }
                    else if (g_Combo >= 25) { cg = 0.55f; cbl = 0.15f; }
                    else if (g_Combo >= 12) { cg = 0.9f;  cbl = 0.3f; }
                    if (ms) { cr = 1.0f; cg = 0.85f; cbl = 0.30f; }   // 마일스톤 = 골드 펄스
                    float w = g_TextS.Width(cb, sc);
                    g_TextS.Draw(cb, (sw - w) * 0.5f, sh * 0.115f, sc, cr, cg, cbl, 0.95f);
                }
            }
    
            // ── 액티브 스킬 슬롯 (좌하단, 패시브 쿨다운 위 행) ──
            if (st == GameState::RUNNING || st == GameState::PAUSED) {
                const float KW = 54.0f, KH = 54.0f, KG = 8.0f;
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
                skillBox(0, L"SHIFT", L"DASH", g_DashCd, 0.4f, 1.0f, 1.0f);
                const wchar_t* keys3[3] = { L"Q", L"E", L"R" };
                for (int i = 0; i < 3; i++) {
                    if (g_Skills[i].type == SkillType::NONE) continue;
                    const wchar_t* tag = L""; float r = 1, g = 1, b = 1;
                    switch (g_Skills[i].type) {
                    case SkillType::CLOSE_WINDOW: tag = L"CLOSE"; r=0.5f; g=0.8f; b=1.0f; break;
                    case SkillType::OVERCLOCK:    tag = L"OVCLK"; r=1.0f; g=0.6f; b=0.2f; break;
                    case SkillType::TIME_STOP:    tag = L"TIME";  r=0.4f; g=0.9f; b=1.0f; break;
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
    
            // ── 액티브/패시브 쿨다운 UI (좌하단) ─────────────────────
            // 추후 픽토그램 PNG 가 들어오면 사각형 placeholder 자리에 텍스처 표시
            if (st == GameState::RUNNING || st == GameState::PAUSED) {
                const float SLOT_W = 56.0f, SLOT_H = 56.0f, SLOT_GAP = 8.0f;
                float baseX  = 16.0f;
                float baseY2 = HudY(sh, SLOT_H + Hud::SLOT_BAR_BASE);   // HP 바 위쪽(작업표시줄 위)
                int   slot   = 0;
    
                auto drawSlot = [&](const wchar_t* tag, float remain,
                                    float r, float g, float b) {
                    float x = baseX + slot * (SLOT_W + SLOT_GAP);
                    float y = baseY2;
                    // 배경
                    drawRect(x, y, SLOT_W, SLOT_H, 0.05f, 0.05f, 0.08f, 0.85f);
                    // 진행도 (위→아래 채워지지 않은 부분 = 쿨타임)
                    if (remain > 0.0f) {
                        // 어두운 오버레이 (남은 비율만큼 위에서부터 채움)
                        // remain 정규화는 호출 시점에서 처리하기 어려우니 alpha 0.55 고정
                        drawRect(x, y, SLOT_W, SLOT_H, 0.0f, 0.0f, 0.0f, 0.55f);
                    }
                    // 컬러 테두리 (위쪽 띠)
                    drawRect(x, y, SLOT_W, 4.0f, r, g, b, 1.0f);
                    // 태그 (영문/약어 — 픽토그램 들어오면 제거)
                    g_TextS.Draw(tag, x + 4.0f, y + 6.0f, 0.7f, r, g, b, 1.0f);
                    // 남은 시간 (정수)
                    if (remain > 0.0f) {
                        wchar_t buf[16];
                        swprintf_s(buf, L"%d", (int)(remain + 0.99f));
                        float tw = g_TextL.Width(buf, 0.9f);
                        g_TextL.Draw(buf, x + (SLOT_W - tw) * 0.5f,
                                     y + SLOT_H * 0.40f, 0.9f, 1,1,1,0.95f);
                    }
                };
    
                // 탄환 세례 — 쿨다운 (20 / 15 / 7.5)
                if (g_Stats.bulletRain) {
                    float remain = g_Stats.bulletRainCooldown - g_BulletRainTimer;
                    if (remain < 0) remain = 0;
                    drawSlot(L"RAIN", remain, 1.0f, 0.5f, 0.2f);
                    ++slot;
                }
                // 취함 — drunkCooldown 대기 / drunkActiveDuration 활성
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
                // 가벼운 발걸음 비활성 카운트다운
                if (g_Stats.lightStep && g_Stats.lightStepDisableTimer > 0.0f) {
                    drawSlot(L"LSTP", g_Stats.lightStepDisableTimer,
                             0.7f, 0.7f, 0.85f);
                    ++slot;
                }
                // 다가오는 죽음 — 활성 + stack (속도 +20%/스택)
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

        // 업적 해금 / 도감 발견 발생 시 저장 (게임 중 즉시 영구화)
        if (g_AchSaveNeeded || g_CodexDirty) {
            SaveGame(); g_AchSaveNeeded = false; g_CodexDirty = false;
        }

        BatchFlush();   // 프레임 마지막 — 남은 도형 모두 그림
        glfwSwapBuffers(window);

        // ── FPS 캡 (g_FpsCap > 0 일 때만) ─────────────────────────────
        // timeBeginPeriod(1) 로 Sleep 해상도 1ms. 마지막 ~1ms 는 busy-wait
        // C18: 과거 '무제한'(-1) 세이브는 300 으로 클램프 (진짜 무제한 제거).
        int capFps = (g_FpsCap < 0) ? 300 : g_FpsCap;
        if (capFps > 0) {
            double target = 1.0 / (double)capFps;
            double frameStart = (double)now;
            double remain = target - (glfwGetTime() - frameStart);
            if (remain > 0.001) {
                // 마지막 1ms 만 남기고 Sleep
                unsigned ms = (unsigned)((remain - 0.001) * 1000.0);
                PlatformSleepMs(ms);
            }
            // 잔여 busy-wait (정확한 캡)
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
