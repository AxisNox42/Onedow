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

#include <glm/glm.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstdarg>
#include <string>
#include <cstring>
#include <algorithm>
#include <functional>

#include "FakeWindow.h"
#include "GameManager.h"
#include "Monster.h"
#include "Bomber.h"
#include "Bullet.h"
#include "MonsterManager.h"
#include "GlitchBoss.h"
#include "ReloadRunnerBoss.h"
#include "PolymorphBoss.h"
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

#ifdef _MSC_VER
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib")   // RegOpenKeyExA / RegQueryValueExA / RegCloseKey
#pragma comment(lib, "winmm.lib")      // timeBeginPeriod / timeEndPeriod (FPS 캡 정밀도)
// dwmapi.lib 은 WindowFx.cpp 에서 링크
#endif


// 투명도 헬퍼는 WindowFx.h/cpp 로 이동
// (EnableWindowTransparency 와 TransparencyLog 가 동일 기능)
#define DBG TransparencyLog

// --- 전역 ---
int   screenWidth  = 0;
int   screenHeight = 0;

// 화면 줌 (보스 페이즈2 "화면 2배 확장" 등). 1.0=기본, 0.5=2배 확장(전체 절반 크기).
//   중심 = 화면 중앙. 줌=1 일 때 모든 헬퍼가 항등(identity)이라 기존 동작 그대로.
float g_ViewZoom       = 1.0f;
float g_ViewZoomTarget = 1.0f;
// 월드 좌표 → 화면 픽셀 (줌 적용)
inline float W2SX(float wx) { return screenWidth  * 0.5f + (wx - screenWidth  * 0.5f) * g_ViewZoom; }
inline float W2SY(float wy) { return screenHeight * 0.5f + (wy - screenHeight * 0.5f) * g_ViewZoom; }
// 가짜창 scissor — 월드 사각형을 줌 적용해 픽셀 scissor 로 (줌=1 이면 기존과 동일)
inline void WorldScissor(float wx, float wy, float ww, float wh) {
    float sx = W2SX(wx), sy = W2SY(wy);
    float sw = ww * g_ViewZoom, sh = wh * g_ViewZoom;
    glScissor((GLint)sx, (GLint)(screenHeight - (sy + sh)), (GLint)sw, (GLint)sh);
}
// 총알 그리기 — 플레이어 탄은 탄속 비례 잔상(streak) + 머리 원, 적 탄은 원만
inline void drawBullet(const Bullet& b) {
    float r = 6.0f * b.sizeScale;
    if (!b.isEnemy) {
        float trailLen = b.speed * 0.020f;          // 탄속 빠를수록 잔상 김
        if (trailLen > 5.0f) {
            float tx = b.x - b.dirX * trailLen;
            float ty = b.y - b.dirY * trailLen;
            float px = -b.dirY * r, py = b.dirX * r;  // 머리 폭(진행방향 수직)
            float v[6] = { b.x + px, b.y + py, b.x - px, b.y - py, tx, ty };
            glUniform4f(g_colorLoc, b.color.r, b.color.g, b.color.b, 0.38f);
            glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
    }
    if (!b.isEnemy) {
        // 글로우 헤일로 (소프트) — 탄환이 더 빛나 보이게 (1겹: 후반 탄막 성능)
        drawCircle(b.x, b.y, r * 1.9f, b.color.r, b.color.g, b.color.b, 0.20f);
    }
    drawCircle(b.x, b.y, r, b.color.r, b.color.g, b.color.b, 1.0f);
}

// 잡몹 그리기 — 종류별 모양 (일반=세모 / 분열체=초록 원 / 점멸체=점멸 다이아)
// 사각 윈도우(스크린/월드 좌표 동일) 안에 점이 들어오는지 — 창별 컬링용
//   각 가짜 창 scissor 패스에서 창 밖 엔티티의 draw call 자체를 건너뛴다.
static inline bool inWin(float x, float y, float rx, float ry, float rw, float rh,
                         float margin = 48.0f) {
    return x >= rx - margin && x <= rx + rw + margin &&
           y >= ry - margin && y <= ry + rh + margin;
}

inline void drawMob(const Monster* m) {
    MarkMobSeen(m->kind);   // 도감 발견 (화면에 그려진 적)
    float base = (m->summoned ? 28.0f : 18.0f) * m->sizeScale;
    if (m->kind == MobKind::SPLITTER) {
        drawCircle(m->worldX, m->worldY, base, m->color.r, m->color.g, m->color.b, 1.0f);
        drawCircle(m->worldX, m->worldY, base*0.42f, 0.05f, 0.22f, 0.08f, 0.9f); // 분할 코어
    } else if (m->kind == MobKind::BLINKER) {
        if (m->blinkWarn) {   // 점멸 목표 위치 잔상 (경고)
            float a = 0.18f + 0.22f * (m->blinkWarnT / Monster::BLINK_WARN);
            drawDiamond(m->blinkTargetX, m->blinkTargetY, base*1.15f,
                        m->color.r, m->color.g, m->color.b, a);
        }
        drawDiamond(m->worldX, m->worldY, base, m->color.r, m->color.g, m->color.b, 1.0f);
        drawDiamond(m->worldX, m->worldY, base*0.4f, 1.0f, 1.0f, 1.0f, 0.9f);
    } else if (m->kind == MobKind::CHARGER) {
        // 준비(텔레그래프) 중엔 깜빡이는 큰 외곽 + 조준선 느낌
        if (m->chargeState == 1) {
            float p = 0.5f + 0.5f * sinf((float)glfwGetTime() * 28.0f);
            drawTriangle(m->worldX, m->worldY, base * (1.5f + 0.4f * p),
                         1.0f, 0.85f, 0.25f, 0.35f);
        } else if (m->chargeState == 2) {   // 돌진 중 잔열
            drawCircle(m->worldX, m->worldY, base * 1.2f, 1.0f, 0.5f, 0.1f, 0.35f);
        }
        drawTriangle(m->worldX, m->worldY, base, m->color.r, m->color.g, m->color.b, 1.0f);
        drawTriangle(m->worldX, m->worldY, base*0.4f, 1.0f, 0.95f, 0.7f, 0.9f);
    } else if (m->kind == MobKind::WEAVER) {
        // 회피체 — 시안 다이아 (좌우로 흔들림)
        drawDiamond(m->worldX, m->worldY, base*0.95f, m->color.r, m->color.g, m->color.b, 1.0f);
        drawDiamond(m->worldX, m->worldY, base*0.35f, 1.0f, 1.0f, 1.0f, 0.85f);
    } else if (m->kind == MobKind::BRUTE) {
        // 거대체 — 겹친 다이아(보석/장갑) + 네 모서리 장갑 스터드 (원형 바디 없음)
        float x = m->worldX, y = m->worldY;
        drawDiamond(x, y, base*1.15f, m->color.r*0.55f, m->color.g*0.55f, m->color.b*0.55f, 1.0f);
        drawDiamond(x, y, base*0.85f, m->color.r, m->color.g, m->color.b, 1.0f);
        drawDiamond(x, y, base*0.40f, 1.0f, 0.6f, 0.45f, 0.95f);
        float s = base*0.28f, o = base*0.50f;
        drawRect(x - o - s*0.5f, y - s*0.5f, s, s, 0.2f, 0.04f, 0.05f, 1.0f);
        drawRect(x + o - s*0.5f, y - s*0.5f, s, s, 0.2f, 0.04f, 0.05f, 1.0f);
        drawRect(x - s*0.5f, y - o - s*0.5f, s, s, 0.2f, 0.04f, 0.05f, 1.0f);
        drawRect(x - s*0.5f, y + o - s*0.5f, s, s, 0.2f, 0.04f, 0.05f, 1.0f);
    } else if (m->kind == MobKind::ORBITER) {
        // 공전체 — 노란 십자(위성) + 중심 다이아
        float x = m->worldX, y = m->worldY;
        float bw = base*1.7f, th = base*0.42f;
        drawRect(x - bw*0.5f, y - th*0.5f, bw, th, m->color.r, m->color.g, m->color.b, 1.0f);
        drawRect(x - th*0.5f, y - bw*0.5f, th, bw, m->color.r, m->color.g, m->color.b, 1.0f);
        drawDiamond(x, y, base*0.6f, 1.0f, 1.0f, 0.85f, 0.95f);
    } else if (m->kind == MobKind::SPAWNER) {
        // 소환체 — 청록 다이아 둥지 + 주위를 도는 작은 세모(알) 3개
        float x = m->worldX, y = m->worldY;
        float ph = (float)glfwGetTime() * 1.2f;
        for (int k = 0; k < 3; k++) {
            float a = ph + (float)k * 2.0944f;
            drawTriangle(x + cosf(a) * base*1.35f, y + sinf(a) * base*1.35f,
                         base*0.45f, m->color.r*1.3f, m->color.g*1.2f, m->color.b*1.2f, 0.95f);
        }
        drawDiamond(x, y, base*1.05f, m->color.r, m->color.g, m->color.b, 1.0f);
        drawDiamond(x, y, base*0.5f, 0.04f, 0.18f, 0.13f, 0.95f);
    } else if (m->kind == MobKind::SHIELDED) {
        // 보호막체 — 파란 다이아 + 방패 ON 동안 플레이어 방향 부채꼴 실드
        float x = m->worldX, y = m->worldY;
        if (m->shieldActive) {
            float ang = atan2f(m->dashDirY, m->dashDirX);
            float pulse = 0.4f + 0.15f * sinf((float)glfwGetTime() * 8.0f);
            drawConeFan(x, y, base*2.0f, ang, 1.0f, 0.35f, 0.75f, 1.0f, pulse);
        }
        drawDiamond(x, y, base, m->color.r, m->color.g, m->color.b, 1.0f);
        drawDiamond(x, y, base*0.4f, 1.0f, 1.0f, 1.0f, 0.85f);
    } else {
        drawTriangle(m->worldX, m->worldY, base, m->color.r, m->color.g, m->color.b, 1.0f);
    }
}
bool  keys[1024]   = {};

GameManager    g_GameManager;
MonsterManager g_MonsterManager;
std::vector<Bullet> g_Bullets;
PlayerStats    g_Stats;
bool g_aug1Released = true, g_aug2Released = true, g_aug3Released = true;
TextRenderer   g_TextL;   // 큰 글자 (증강 이름, 상태 타이틀)
TextRenderer   g_TextS;   // 작은 글자 (설명, 힌트)
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
static const int MAX_DRONES = 2;
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
static constexpr float TURRET_WIN_W  = 250.0f;
static constexpr float TURRET_WIN_H  = 250.0f;
// 신규 보스 개인 창 크기 (본체/HP 를 가두는 따라다니는 창)
static constexpr float GLITCH_WIN_W = 620.0f;
static constexpr float RR_WIN_W     = 600.0f;
static constexpr float POLY_WIN_W   = 840.0f;
static constexpr float TURRET_LIFE   = 5.0f;      // 포탑 지속 5초
static constexpr float TURRET_DEPLOY = 1.0f;      // 1초마다 배치 (대포 공속 고정)
std::vector<Turret> g_Turrets;
float       g_TurretDeployTimer = 0.0f;
PlayerStats g_TurretStats;                        // 소총 기준 능력치

// 차크람 (에픽+) — 주변 공전 + 잡몹 즉사 + HP. 최대 3개
struct ChakramState {
    float angle        = 0.0f;
    float hp           = 100.0f;
    float maxHp        = 100.0f;
    bool  alive        = false;
    float respawnTimer = 0.0f;
};
static const int   MAX_CHAKRAMS    = 3;
ChakramState g_Chakrams[MAX_CHAKRAMS] = {};
static const float CHAKRAM_RADIUS = 70.0f;
static const float CHAKRAM_SIZE   = 22.0f;

// 탄환 세례 (전설) — 20초 쿨다운
float g_BulletRainTimer = 0.0f;

// 취함 (디버프) — 20초 사이클 중 5초간 랜덤 방향 사격
float g_DrunkCycle  = 0.0f;
bool  g_DrunkActive = false;

// 시즈탱크 1초 단위 stack 누적용
float g_SiegeTick = 0.0f;

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
long long g_NextBossScore  = 200000;   // 다음 일반 보스 점수 (도달 시 +20만)
bool      g_PolySpawned    = false;    // 폴리모프(50만 고정) 등장 여부
bool      g_CreativeBossPending = false;  // 크리에이티브: 선택 보스 즉시 스폰 대기
// 글리치 보스 (슬라임과 양자택일로 등장) — 슬라임과 별도 관리
GlitchBoss* g_GlitchBoss = nullptr;
// 리로드 러너 보스 (무기 교체형) — 별도 관리
ReloadRunnerBoss* g_RRBoss = nullptr;
// 폴리모프 보스 (희귀, 폼 변환 + 페이즈2 화면 확장) — 별도 관리
PolymorphBoss* g_PolyBoss = nullptr;
int g_PolyPrevForm = -1;   // 폼 변환 감지용 (변할 때 파티클) — -1 = 미초기화
float g_PolySummonTimer = 0.0f;   // 2페이즈: 7초마다 주변에 원거리/자폭병 5마리
bool  g_PolyWasPhase2   = false;  // 2페이즈 진입 연출 1회용
bool  g_LastRunRecord   = false;  // 직전 판이 신기록이었는지 (GAMEOVER 표시용)
int   g_MetaStartAugs   = 0;      // 메타 해금: 시작 무료 증강 픽 횟수

// ── 피격 시 창(시야) 축소 기믹 ── 체력 비율에 따라 창 크기 변동 + 피격 펀치
float g_WindowSizeCur = 0.0f;     // 현재 애니메이션 창 크기 (0 = 미초기화)
float g_WinPrevHP     = -1.0f;    // 창 축소용 HP 추적
float g_HurtVignette  = 0.0f;     // 피격 빨간 비네트 잔여

// ── 액티브 스킬 시스템 — 대시(기본) + 슬롯 3개(증강 획득, 꽉 차면 교체) ──
enum class SkillType { NONE, CLOSE_WINDOW, OVERCLOCK, TIME_STOP };
struct SkillSlot { SkillType type = SkillType::NONE; float cd = 0.0f; }; // cd = 남은 쿨다운
SkillSlot g_Skills[3];
int   g_SkillReplaceIdx = 0;       // 슬롯 꽉 찼을 때 교체할 슬롯(순환)
float g_DashCd        = 0.0f;      // 대시 쿨다운
float g_DashInvuln    = 0.0f;      // 대시 무적 잔여
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
static SkillType SkillForAug(AugType a) {
    if (a == AugType::SKILL_CLOSE)     return SkillType::CLOSE_WINDOW;
    if (a == AugType::SKILL_OVERCLOCK) return SkillType::OVERCLOCK;
    if (a == AugType::SKILL_TIMESTOP)  return SkillType::TIME_STOP;
    return SkillType::NONE;
}
static void EquipSkill(SkillType t) {
    if (t == SkillType::NONE) return;
    for (int i = 0; i < 3; i++) if (g_Skills[i].type == t) return;      // 이미 보유
    for (int i = 0; i < 3; i++) if (g_Skills[i].type == SkillType::NONE) {
        g_Skills[i] = { t, 0.0f }; return; }
    g_Skills[g_SkillReplaceIdx] = { t, 0.0f };                          // 꽉 참 → 순환 교체
    g_SkillReplaceIdx = (g_SkillReplaceIdx + 1) % 3;
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
std::vector<Boss*> g_Slimelings;
bool g_SlimeEncounter = false;   // 분열 인카운터 진행 중 (끝나면 보상)
// 분열체 생성 (체력 반토막·크기 0.75배·돌진만)
static Boss* MakeSlimeling(float x, float y, float maxHp, float scale, int gen,
                           int sw, int sh) {
    Boss* c = new Boss(x, y, sw, sh, maxHp);
    c->sizeScale  = scale;
    c->splitGen   = gen;
    c->chargeOnly = true;
    c->idleCooldown = 2.2f;                        // 돌진만 하니 자주
    c->color = glm::vec3(0.55f, 0.95f, 0.60f);     // 분열체 = 초록빛 슬라임
    return c;
}

// HUD: 현재 측정 FPS (상단 우측 표시)
int    g_CurrentFPS  = 0;
double g_FpsLastTime = 0.0;
int    g_FpsFrames   = 0;

// 보유 증강 인덱스 목록 (선택 순서대로, 중복 스택 가능)
std::vector<int> g_OwnedAugs;

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

// ── UI 버튼 헬퍼 ─────────────────────────────────────────────
// 호버 시 밝아짐 + 테두리 강조. 클릭 release 시 true 반환 (한 번)
// 호출 시점 마우스 좌표 mx,my 와 lmb 상태를 외부에서 받아옴
static bool UIButton(float x, float y, float w, float h, const wchar_t* label,
                     double mx, double my, bool lmb, bool lmbPrev,
                     bool selected = false)
{
    BindMainShader();  // 직전 text 그리기로 shader 바뀌었을 수 있음

    bool hover = (mx >= x && mx <= x + w && my >= y && my <= y + h);

    // 배경
    float br = selected ? 0.32f : (hover ? 0.22f : 0.10f);
    float bg = selected ? 0.32f : (hover ? 0.22f : 0.10f);
    float bb = selected ? 0.40f : (hover ? 0.28f : 0.14f);
    drawRect(x, y, w, h, br, bg, bb, 0.92f);

    // 테두리
    float er = hover ? 0.95f : (selected ? 0.75f : 0.45f);
    float eg = hover ? 0.95f : (selected ? 0.75f : 0.45f);
    float eb = hover ? 1.00f : (selected ? 0.95f : 0.55f);
    drawRect(x,         y,         w, 2.0f, er, eg, eb, 1.0f);
    drawRect(x,         y + h - 2, w, 2.0f, er, eg, eb, 1.0f);
    drawRect(x,         y,    2.0f, h, er, eg, eb, 1.0f);
    drawRect(x + w - 2, y,    2.0f, h, er, eg, eb, 1.0f);

    // 라벨 (가로/세로 모두 중앙 정렬, 너무 길면 fit)
    float sc = 0.9f;
    while (sc > 0.55f && g_TextL.Width(label, sc) > w - 16.0f) sc -= 0.05f;
    float lw = g_TextL.Width (label, sc);
    float lh = g_TextL.Height(label, sc);
    g_TextL.Draw(label,
                 x + (w - lw) * 0.5f,
                 y + (h - lh) * 0.5f,
                 sc, 1.0f, 1.0f, 1.0f, 0.98f);

    return hover && lmbPrev && !lmb; // release edge over button
}

// ── 적 사망 폭발 파티클 (잡몹/원거리 몹 공용) ──
struct EnemyParticle {
    float x, y, vx, vy;
    float life;     // 남은 시간 (s)
    float maxLife;
    float size;
    float r, g, b;
    bool  active = false;
};
static const int MAX_ENEMY_PARTS = 256;
EnemyParticle g_EnemyParts[MAX_ENEMY_PARTS] = {};

// 폭발 spawn — big=true 면 원거리 몹용 (더 큰 폭발)
static void SpawnEnemyExplosion(float ex, float ey,
                                float cr, float cg, float cb, bool big) {
    // 흰 핫코어 스파크 — 폭발 순간 번쩍이는 밝은 알갱이
    SpawnSparks(ex, ey, big ? 8 : 4, 1.0f, 0.95f, 0.7f, big ? 420.0f : 320.0f);
    int count   = big ? 20 : 10;
    float baseS = big ? 200.0f : 100.0f;
    float varS  = big ? 250.0f : 150.0f;
    float lifeT = big ? 0.45f  : 0.30f;
    int placed = 0, j = 0;
    while (placed < count && j < MAX_ENEMY_PARTS) {
        if (!g_EnemyParts[j].active) {
            float angle = (float)placed / (float)count * 6.2831853f
                        + ((float)(rand() % 100) - 50.0f) * 0.012f;
            float spd   = baseS + (float)(rand() % (int)varS);
            float sz    = big ? (float)(5 + rand() % 10)
                              : (float)(3 + rand() % 5);
            g_EnemyParts[j] = {
                ex, ey,
                cosf(angle) * spd, sinf(angle) * spd,
                lifeT, lifeT, sz, cr, cg, cb, true
            };
            ++placed;
        }
        ++j;
    }
}

// DYING 슬로우 모션 상태
float g_DyingTimer = 0.0f;

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

// --- 셰이더 소스 ---
static const char* vertSrc =
    "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "uniform mat4 projection;\n"
    "void main() { gl_Position = projection * vec4(aPos, 0.0, 1.0); }\n";
static const char* fragSrc =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "uniform vec4 color;\n"
    "void main() { FragColor = color; }\n";

// --- 콜백 ---
void key_callback(GLFWwindow*, int key, int, int action, int) {
    if (key >= 0 && key < 1024) {
        if      (action == GLFW_PRESS)   keys[key] = true;
        else if (action == GLFW_RELEASE) keys[key] = false;
    }
}

static GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    return s;
}

// ============================================================
int main() {
    srand((unsigned)time(NULL));

    // 실행 파일 폴더로 작업 디렉터리 이동 (Resource/ 상대경로 로드 보장)
    //   Windows 는 임베디드 폰트라 no-op, macOS/Linux 는 더블클릭 실행 대응
    PlatformChdirToExeDir();
    LoadGame();   // 저장된 설정/기록 불러오기 (없으면 기본값 유지)

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
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
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
#else
    // macOS/Linux: 디스크의 TTF 폴백 체인
    {
        const char* chain[3];
        int nc = LanguageFontChain(g_Language, chain);
        g_TextL.InitFromFiles(chain, nc, 36, screenWidth, screenHeight);
        g_TextS.InitFromFiles(chain, nc, 22, screenWidth, screenHeight);
    }
#endif

    glEnable(GL_BLEND);
    // RGB: 표준 알파블렌딩 / Alpha: 프레임버퍼 알파값 올바르게 누적
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                        GL_ONE,       GL_ONE_MINUS_SRC_ALPHA);

    // 아이콘(픽토그램) 텍스처 파이프라인 + 증강 아이콘 로드
    InitIconGL();
    LoadIcons();

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

    // --- 셰이더 빌드 ---
    GLuint vs = compileShader(GL_VERTEX_SHADER,   vertSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    GLuint shader = glCreateProgram();
    glAttachShader(shader, vs); glAttachShader(shader, fs);
    glLinkProgram(shader);
    glDeleteShader(vs); glDeleteShader(fs);

    GLint projLoc  = glGetUniformLocation(shader, "projection");
    g_colorLoc     = glGetUniformLocation(shader, "color");

    // UI 코드에서 BindMainShader() 로 재바인드할 수 있게 글로벌에 보관
    g_MainShader  = shader;
    g_MainProjLoc = projLoc;

    // --- VAO / VBO ---
    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &g_VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
    glBufferData(GL_ARRAY_BUFFER, 4096 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // --- 오르토 행렬 (픽셀 좌표계, Y 아래) ---
    float ortho[16] = {
         2.0f / screenWidth,  0,  0,  0,
         0, -2.0f / screenHeight,  0,  0,
         0,  0, -1,  0,
        -1,  1,  0,  1
    };
    // BindMainShader() 가 사용할 수 있게 글로벌에도 복사
    memcpy(g_MainOrtho, ortho, sizeof(ortho));
    g_MainVAO = VAO;

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

    // ============================================================
    // 메인 루프
    // ============================================================
    while (!glfwWindowShouldClose(window)) {
        float now   = (float)glfwGetTime();
        float delta = now - lastFrame;
        if (delta > 0.1f) delta = 0.1f; // 스파이크 클램프
        lastFrame = now;

        // FPS 측정 (1초 단위)
        ++g_FpsFrames;
        if (now - g_FpsLastTime >= 1.0f) {
            g_CurrentFPS  = g_FpsFrames;
            g_FpsFrames   = 0;
            g_FpsLastTime = now;
        }

        glfwPollEvents();

        // 줌 부드럽게 보간 (페이즈2 화면 확장 등)
        g_ViewZoom += (g_ViewZoomTarget - g_ViewZoom) * std::min(1.0f, delta * 4.0f);

        // 마우스 상태 (mx,my = 화면 픽셀 / wmx,wmy = 줌 보정한 월드 좌표 = 조준용)
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        bool lmb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        float wmx = screenWidth  * 0.5f + ((float)mx - screenWidth  * 0.5f) / g_ViewZoom;
        float wmy = screenHeight * 0.5f + ((float)my - screenHeight * 0.5f) / g_ViewZoom;

        // --- 입력 처리 ---
        GameState prevState = g_GameManager.currentState;
        g_GameManager.HandleInput(window);

        // 새 게임 리셋 람다 (GAMEOVER → READY, 난이도 선택 후, 다시하기 버튼 등에서 호출)
        auto ResetForNewGame = [&]() {
            g_Stats        = PlayerStats();
            g_MetaStartAugs = ApplyMeta(g_Stats);    // 메타 영구 업그레이드 적용
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
            g_SiegeTick       = 0.0f;
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
            g_ShakeTime = 0.0f; g_ShakeMag = 0.0f;
            g_NextBossScore = 200000;
            g_PolySpawned   = false;
            g_BossRewardPicksLeft = 0;
            if (g_GlitchBoss) { delete g_GlitchBoss; g_GlitchBoss = nullptr; }
            if (g_RRBoss)     { delete g_RRBoss;     g_RRBoss     = nullptr; }
            if (g_PolyBoss)   { delete g_PolyBoss;   g_PolyBoss   = nullptr; }
            g_PolyPrevForm = -1;
            g_PolySummonTimer = 0.0f;
            g_PolyWasPhase2 = false;
            for (auto* c : g_Slimelings) delete c;   // 분열체 정리
            g_Slimelings.clear();
            g_SlimeEncounter = false;
            g_BossTintT = 0.0f;
            ResetJuice();                            // 데미지숫자/콤보/플래시/히트스톱 초기화
            ResetSkills();                           // 액티브 스킬/대시 초기화
            g_WindowSizeCur = 0.0f; g_WinPrevHP = -1.0f; g_HurtVignette = 0.0f; // 창 시야 기믹
            g_ViewZoom = g_ViewZoomTarget = 1.0f;   // 줌 원복
            rangedSpawnTimer = GetDifficultyParams(g_Difficulty).rangedSpawnInitialDelay;
            spawnTimer        = 0.0f;
            for (int i = 0; i < MAX_ENEMY_PARTS; i++) g_EnemyParts[i].active = false;
            g_DyingTimer      = 0.0f;
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
                // 일반 사망
                g_GameManager.currentState = GameState::DYING;
                g_GameManager.playerHP     = 0.0f;
            g_DyingTimer = 1.0f;
            float pCX = playerWin.x + playerWin.width  * 0.5f;
            float pCY = playerWin.y + playerWin.height * 0.5f;
            g_DeathCX = pCX; g_DeathCY = pCY;
            g_DeathFlash = 1.0f;
            for (int i = 0; i < MAX_DEBRIS; i++) {
                // 방사형 + 약간의 무작위 각도 흔들림
                float angle = (float)i / (float)MAX_DEBRIS * 2.0f * (float)M_PI
                            + ((float)(rand() % 100) - 50.0f) * 0.01f;
                float spd   = 350.0f + (float)(rand() % 500);   // 350~850 (이전 80~200)
                float sz    = 8.0f   + (float)(rand() % 22);    // 8~30 px
                // 색상: 시안 베이스에 흰색·노란색 일부 섞기
                int   tint  = rand() % 10;
                float cr, cg, cb;
                if      (tint < 5) { cr = 0.10f; cg = 0.85f; cb = 1.00f; } // cyan
                else if (tint < 8) { cr = 1.00f; cg = 1.00f; cb = 1.00f; } // white core
                else               { cr = 1.00f; cg = 0.85f; cb = 0.20f; } // yellow spark
                g_Debris[i] = {
                    pCX, pCY,
                    cosf(angle)*spd, sinf(angle)*spd,
                    sz, cr, cg, cb, true
                };
            }
            }   // close else (MK2 분기 외)
        }       // close outer if (HP <= 0)

        // DYING 타이머 (실시간) + 파편 이동 (슬로우) + GAMEOVER 전환
        if (g_GameManager.currentState == GameState::DYING) {
            g_DyingTimer -= delta;
            g_DeathFlash -= delta * 4.0f; // 0.25초 안에 섬광 사라짐
            if (g_DeathFlash < 0.0f) g_DeathFlash = 0.0f;
            float sd = delta * 0.15f;
            for (int i = 0; i < MAX_DEBRIS; i++) {
                if (!g_Debris[i].active) continue;
                g_Debris[i].x  += g_Debris[i].vx * sd;
                g_Debris[i].y  += g_Debris[i].vy * sd;
                // 감속 (드래그)
                g_Debris[i].vx *= (1.0f - 1.2f * sd);
                g_Debris[i].vy *= (1.0f - 1.2f * sd);
            }
            if (g_DyingTimer <= 0.0f) {
                g_GameManager.currentState = GameState::GAMEOVER;
                g_DyingTimer = 0.0f;
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
                    g_OwnedAugs.clear();
                    memset(g_GameManager.takenOnce, 0,
                           sizeof(g_GameManager.takenOnce));
                    g_GameManager.maxHP = g_Stats.maxHP;
                    if (g_GameManager.playerHP > g_Stats.maxHP)
                        g_GameManager.playerHP = g_Stats.maxHP;
                    if (prevAugs > 24) prevAugs = 24;       // 배열/풀 상한
                    int nDebuffs = prevAugs * 2 / 5;        // 40%
                    int nBuffs   = prevAugs - nDebuffs;
                    int buffs[24], debuffs[24];
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

                g_Stats.Apply(atype);
                g_OwnedAugs.push_back(idx);
                g_TypeOwned[(int)atype] = true;   // 조합 레시피 판정용
                MarkAugSeen(idx);                 // 도감 발견
                // 액티브 스킬 증강이면 슬롯에 장착 (꽉 차면 순환 교체)
                EquipSkill(SkillForAug(atype));

                // 조합: CANNON + DRONE_2 → 포탑 배치 (드론 공전 대체)
                if (g_Stats.cannon && g_Stats.drone && g_Stats.droneCount >= 2
                    && !g_Stats.turretMode) {
                    g_Stats.turretMode = true;
                    g_Turrets.clear();
                    g_TurretDeployTimer = TURRET_DEPLOY;  // 즉시 첫 포탑 배치
                }

                // 한 번만 뽑힐 증강 표시:
                //   - EPIC/LEGENDARY 전체
                //   - 티어드 증강 (탄환세례/드론/차크람) — 등급 무관 1회씩
                AugRarity r = ALL_AUGS[idx].rarity;
                bool onceOnly = (r == AugRarity::EPIC || r == AugRarity::LEGENDARY ||
                                 r == AugRarity::COMBO);
                if (atype == AugType::BULLET_RAIN   || atype == AugType::BULLET_RAIN_2 ||
                    atype == AugType::BULLET_RAIN_3 ||
                    atype == AugType::DRONE         || atype == AugType::DRONE_2 ||
                    atype == AugType::CHAKRAM       || atype == AugType::CHAKRAM_2 ||
                    atype == AugType::CHAKRAM_3) {
                    onceOnly = true;
                }
                if (onceOnly) g_GameManager.takenOnce[idx] = true;

                // 흡혈마: 현재 HP -20%
                if (atype == AugType::VAMPIRE)
                    g_GameManager.playerHP *= 0.80f;

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
                            g_Chakrams[c].hp           = 100.0f;
                            g_Chakrams[c].maxHp        = 100.0f;
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

                // 포탑 조합 재판정
                if (g_Stats.cannon && g_Stats.drone && g_Stats.droneCount >= 2) {
                    if (!savedTurret) { g_Turrets.clear(); g_TurretDeployTimer = TURRET_DEPLOY; }
                    g_Stats.turretMode = true;
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
                // 변환 카드 — 25% 확률, 현재 무기와 다른 StartWeapon 으로 전환
                g_ConversionWeapon = -1;
                if ((rand() % 100) < 25 && g_CurrentWeapon >= 0) {
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
                    float curMove  = MOVE_SPEED * moveMult;
                    playerWin.x += mvX * curMove * FIXED_DT;
                    playerWin.y += mvY * curMove * FIXED_DT;
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
                if (pCY > ccY + halfH) pCY = ccY + halfH;
                playerWin.x = pCX - playerWin.width  * 0.5f;
                playerWin.y = pCY - playerWin.height * 0.5f;

                // ── 액티브 스킬 — 쿨다운/지속 갱신 + 입력(Shift 대시 / Q·E·R 슬롯) ──
                if (g_DashCd > 0.0f)        g_DashCd        -= FIXED_DT;
                if (g_DashInvuln > 0.0f)    g_DashInvuln    -= FIXED_DT;
                if (g_TimeStopTimer > 0.0f) g_TimeStopTimer -= FIXED_DT;
                if (g_OverclockTimer > 0.0f)g_OverclockTimer-= FIXED_DT;
                for (int i = 0; i < 3; i++) if (g_Skills[i].cd > 0.0f) g_Skills[i].cd -= FIXED_DT;

                // 창 닫기 — 플레이어 중심 폭발 (넉백 + 피해)
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
                    for (auto* c : g_Slimelings)                if (c->alive) { float dx=c->worldX-cx,dy=c->worldY-cy; if(dx*dx+dy*dy<r2){c->hp-=dmg; if(c->hp<=0)c->alive=false;} }
                    if (g_MonsterManager.boss && g_MonsterManager.boss->alive) { float dx=g_MonsterManager.boss->worldX-cx,dy=g_MonsterManager.boss->worldY-cy; if(dx*dx+dy*dy<r2){g_MonsterManager.boss->hp-=dmg; if(g_MonsterManager.boss->hp<=0)g_MonsterManager.boss->alive=false;} }
                    if (g_GlitchBoss && g_GlitchBoss->alive) { float dx=g_GlitchBoss->worldX-cx,dy=g_GlitchBoss->worldY-cy; if(dx*dx+dy*dy<r2){g_GlitchBoss->hp-=dmg; if(g_GlitchBoss->hp<=0)g_GlitchBoss->alive=false;} }
                    if (g_RRBoss && g_RRBoss->alive) { float dx=g_RRBoss->worldX-cx,dy=g_RRBoss->worldY-cy; if(dx*dx+dy*dy<r2){g_RRBoss->hp-=dmg; if(g_RRBoss->hp<=0)g_RRBoss->alive=false;} }
                    if (g_PolyBoss && g_PolyBoss->alive && g_PolyBoss->damageable()) { float dx=g_PolyBoss->worldX-cx,dy=g_PolyBoss->worldY-cy; if(dx*dx+dy*dy<r2){g_PolyBoss->hp-=dmg; if(g_PolyBoss->hp<=0)g_PolyBoss->alive=false;} }
                    SpawnShockWave(cx, cy, rad*1.3f, 0.6f, 0.5f, 0.8f, 1.0f);
                    SpawnEnemyExplosion(cx, cy, 0.5f, 0.8f, 1.0f, true);
                    g_ShakeTime = 0.4f; g_ShakeMag = 20.0f;
                    TriggerFlash(0.5f, 0.8f, 1.0f, 0.45f); TriggerHitStop(0.07f);
                };

                // 슬롯 스킬 발동 헬퍼
                auto useSkill = [&](int slot) {
                    if (slot < 0 || slot >= 3) return;
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

                // 총알 이동 + 화면 밖 비활성화 (+ 유도탄 보정)
                for (auto& b : g_Bullets) {
                    // 유도탄: 가장 가까운 적을 향해 점진적 방향 보정
                    if (b.homing && !b.isEnemy && b.active) {
                        float nd = 1e9f, tx = 0, ty = 0;
                        for (auto m : g_MonsterManager.monsters) {
                            if (!m->alive) continue;
                            float ddx = m->worldX - b.x, ddy = m->worldY - b.y;
                            float dd  = ddx*ddx + ddy*ddy;
                            if (dd < nd) { nd = dd; tx = m->worldX; ty = m->worldY; }
                        }
                        for (auto r : g_MonsterManager.rangedMobs) {
                            if (!r->alive) continue;
                            float ddx = r->worldX - b.x, ddy = r->worldY - b.y;
                            float dd  = ddx*ddx + ddy*ddy;
                            if (dd < nd) { nd = dd; tx = r->worldX; ty = r->worldY; }
                        }
                        if (nd < 1e8f) {
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
                bool  timeStopped = (g_TimeStopTimer > 0.0f);

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

                // 글리치 보스 업데이트 (페이즈 머신 + 세모 떼 + 레이저)
                if (!timeStopped && g_GlitchBoss && g_GlitchBoss->alive)
                    g_GlitchBoss->Update(pCX, pCY, FIXED_DT, g_GameManager.playerHP);

                // 리로드 러너 업데이트 (무기 상태머신 + 장전 질주, 적 총알 push)
                if (!timeStopped && g_RRBoss && g_RRBoss->alive)
                    g_RRBoss->Update(pCX, pCY, FIXED_DT, g_GameManager.playerHP, g_Bullets);

                // 폴리모프 업데이트 (폼 변환 + 세모/레이저/차크람) + 페이즈2 화면 확장
                if (g_PolyBoss && g_PolyBoss->alive) {
                    if (!timeStopped)
                    g_PolyBoss->Update(pCX, pCY, FIXED_DT, g_GameManager.playerHP, g_Bullets);
                    g_ViewZoomTarget = g_PolyBoss->phase2 ? 0.5f : 1.0f;
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

                // 슬라임 분열체 업데이트 (돌진만, 소환 X) — outSummons 폐기
                if (!g_Slimelings.empty()) {
                    std::vector<Monster*> sink;
                    for (auto* c : g_Slimelings)
                        if (c->alive && !timeStopped)
                            c->Update(pCX, pCY, FIXED_DT, g_GameManager.playerHP, sink);
                    for (auto* m : sink) delete m;   // chargeOnly 라 보통 비어있음
                }

                // 충돌 (반환값 = 플레이어가 이번 프레임 피격됐는지)
                bool hit = CollisionSystem::Update(pCX, pCY,
                    g_MonsterManager, g_Bullets,
                    g_GameManager.playerHP,
                    g_GameManager.scoreAccum, g_GameManager.score,
                    g_Stats, g_GameManager.xp);
                // 대시 무적 — 이번 스텝의 적 피해 무효 (회복은 유지)
                if (g_DashInvuln > 0.0f && g_GameManager.playerHP < hpAtStep)
                    g_GameManager.playerHP = hpAtStep;

                // 글리치 보스 + 미니 세모 vs 플레이어 총알 (스윕 판정)
                if (g_GlitchBoss && g_GlitchBoss->alive) {
                    auto* gb = g_GlitchBoss;
                    for (auto& b : g_Bullets) {
                        if (!b.active || b.isEnemy) continue;
                        // 미니 세모 (1히트 소멸 + 약간의 점수)
                        for (auto& t : gb->minis) {
                            if (!t.alive) continue;
                            if (SegDist(t.x, t.y, b.prevX, b.prevY, b.x, b.y) < 14.0f) {
                                t.alive = false;
                                g_GameManager.scoreAccum += 20.0f;
                                g_GameManager.score = (long long)g_GameManager.scoreAccum;
                                if (b.remainingDmg <= 0.001f) b.active = false;
                                break;
                            }
                        }
                        if (!b.active) continue;
                        // 본체
                        if (SegDist(gb->worldX, gb->worldY,
                                    b.prevX, b.prevY, b.x, b.y) < GlitchBoss::BODY * 0.7f) {
                            float pd = glm::distance(glm::vec2(pCX, pCY),
                                                     glm::vec2(gb->worldX, gb->worldY));
                            float dmg;
                            if (b.remainingDmg > 0.0f)      dmg = b.remainingDmg;
                            else if (b.turretDmg > 0.0f)    dmg = b.turretDmg;
                            else dmg = g_Stats.GetBaseDamage()
                                     * g_Stats.GetDamageMultiplier(pd) * b.dmgMult;
                            float dealt = (dmg < gb->hp) ? dmg : gb->hp;
                            gb->hp -= dealt;
                            if (b.remainingDmg > 0.0f) b.remainingDmg -= dealt;
                            if (gb->hp <= 0.0f) gb->alive = false;
                            if (b.remainingDmg <= 0.001f) b.active = false;
                        }
                    }
                }

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
                if (!g_Slimelings.empty()) {
                    for (auto& b : g_Bullets) {
                        if (!b.active || b.isEnemy) continue;
                        for (auto* c : g_Slimelings) {
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
                            float xpB, scB; MobKillReward(m->kind, m->splitGen, xpB, scB);
                            creditKill(xpB + (float)g_Stats.meleeXpBonus, scB);
                        }
                        SpawnEnemyExplosion(m->worldX, m->worldY,
                                            m->color.r, m->color.g, m->color.b,
                                            /*big=*/false);
                        m->exploded = true;
                        // 연쇄 폭발(DEATH_BLAST) — 사망 위치에서 주변 적에게 AoE
                        //   너프: 데미지 0.8→0.3 + 폭발로 죽은 몹은 다시 안 터짐(무한연쇄 차단)
                        if (g_Stats.deathBlast && !m->noBlast) {
                            float blastDmg = g_Stats.GetBaseDamage()
                                           * g_Stats.GetDamageMultiplier(0.0f) * 0.3f;
                            float blastR = 130.0f;
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
                        if (m->kind == MobKind::SPLITTER && m->splitGen < 2) {
                            for (int c = 0; c < 2; c++) {
                                Monster* ch = new Monster(m->worldX + (c?28:-28), m->worldY,
                                                          1.0f, 1.0f, false);
                                ch->MakeKind(MobKind::SPLITTER, m->splitGen + 1,
                                             m->sizeScale * 0.7f);
                                mobBorn.push_back(ch);
                            }
                        }
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
                    // 슬라임이니까 죽으면 분할 — 절반 체력·0.75배 크기 분열체 2마리 (1세대)
                    float childHp = bs->maxHp * 0.5f;
                    g_Slimelings.push_back(MakeSlimeling(bs->worldX - 40, bs->worldY,
                                           childHp, 0.75f, 1, screenWidth, screenHeight));
                    g_Slimelings.push_back(MakeSlimeling(bs->worldX + 40, bs->worldY,
                                           childHp, 0.75f, 1, screenWidth, screenHeight));
                    g_SlimeEncounter = true;          // 분열 인카운터 시작 (보상은 전멸 시)
                    // 원본 보스 객체 정리 (아직 보상 X)
                    delete bs;
                    g_MonsterManager.boss = nullptr;
                }

                // 슬라임 분열체 사망 처리 — 1세대는 또 분열, 2세대는 최종 사망
                if (!g_Slimelings.empty()) {
                    std::vector<Boss*> born;       // 이번 프레임 새로 분열된 자식 (반복 중 push 금지)
                    for (auto* c : g_Slimelings) {
                        if (c->alive || c->exploded) continue;
                        c->exploded = true;
                        SpawnEnemyExplosion(c->worldX, c->worldY, 0.5f, 0.95f, 0.6f, true);
                        SpawnShockWave(c->worldX, c->worldY, 180.0f, 0.4f, 0.5f, 0.95f, 0.5f);
                        if (c->splitGen < 2) {     // 총 2세대까지만 분열
                            float hp2 = c->maxHp * 0.5f;
                            float sc2 = c->sizeScale * 0.75f;
                            born.push_back(MakeSlimeling(c->worldX - 28, c->worldY,
                                           hp2, sc2, c->splitGen + 1, screenWidth, screenHeight));
                            born.push_back(MakeSlimeling(c->worldX + 28, c->worldY,
                                           hp2, sc2, c->splitGen + 1, screenWidth, screenHeight));
                        } else {
                            g_GameManager.scoreAccum += 4000.0f;   // 최종 분열체 처치 보너스
                            g_GameManager.score = (long long)g_GameManager.scoreAccum;
                        }
                    }
                    // 죽은 분열체 제거
                    for (auto it = g_Slimelings.begin(); it != g_Slimelings.end(); ) {
                        if (!(*it)->alive) { delete *it; it = g_Slimelings.erase(it); }
                        else ++it;
                    }
                    // 새 자식 합류
                    for (auto* nc : born) g_Slimelings.push_back(nc);

                    // 전멸 → 인카운터 종료 + 보상 (점수 + 버프 2개)
                    if (g_SlimeEncounter && g_Slimelings.empty()) {
                        g_SlimeEncounter = false;
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

                // 글리치 보스 사망 → 보상 (슬라임과 동일)
                if (g_GlitchBoss && !g_GlitchBoss->alive && !g_GlitchBoss->exploded) {
                    auto* gb = g_GlitchBoss;
                    SpawnEnemyExplosion(gb->worldX, gb->worldY, 0.4f, 1.0f, 0.6f, true);
                    SpawnEnemyExplosion(gb->worldX, gb->worldY, 1.0f, 0.3f, 0.3f, true);
                    SpawnShockWave(gb->worldX, gb->worldY, 380.0f, 0.7f, 0.3f, 1.0f, 0.6f);
                    g_ShakeTime = 0.5f; g_ShakeMag = 20.0f;
                    TriggerFlash(0.95f, 0.3f, 0.7f, 0.6f); TriggerHitStop(0.10f);
                    gb->exploded = true;
                    g_GameManager.scoreAccum += 20000.0f;
                    g_GameManager.score = (long long)g_GameManager.scoreAccum;
                    delete gb;
                    g_GlitchBoss = nullptr;
                    g_TotalBossKills++;
                    TryUnlockAch(ACH_FIRST_BOSS);
                    if (g_TotalBossKills >= 3) TryUnlockAch(ACH_BOSS_3);
                    g_BossRewardPicksLeft = 2;
                    g_GameManager.PickAugChoices(g_Stats.sizeAugTaken,
                                                 g_Stats.distAugTaken);
                    g_GameManager.currentState = GameState::AUG_SELECT;
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
                            // 총알 격파: 평범한 폭발 파티클
                            SpawnEnemyExplosion(bm->worldX, bm->worldY,
                                                0.9f, 0.4f, 0.4f, true);
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
                        TriggerFlash(0.5f, 1.0f, 0.7f, 0.45f);  // 레벨업 번쩍 (연녹)
                        g_GameManager.PickAugChoices(g_Stats.sizeAugTaken,
                                                     g_Stats.distAugTaken);

                        // 변환 카드 — 25% 확률, 현재 무기와 다른 StartWeapon 으로 전환
                        //   (기존 무기 효과 제거 후 새 무기로 변환)
                        g_ConversionWeapon = -1;
                        if ((rand() % 100) < 25 && g_CurrentWeapon >= 0) {
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

            // ── 피격 시 창(시야) 축소 — HP 비율에 따라 창 크기 변동 + 피격 펀치 ──
            {
                float baseSize = g_Stats.windowSize;
                float hpFrac = (g_GameManager.maxHP > 0.0f)
                             ? g_GameManager.playerHP / g_GameManager.maxHP : 1.0f;
                if (hpFrac < 0.0f) hpFrac = 0.0f; if (hpFrac > 1.0f) hpFrac = 1.0f;
                float target = baseSize * (0.55f + 0.45f * hpFrac);   // 풀피=full, 0%=55%
                if (g_WindowSizeCur <= 0.0f) g_WindowSizeCur = target; // 초기화
                if (g_WinPrevHP < 0.0f) g_WinPrevHP = g_GameManager.playerHP;
                float lost = g_WinPrevHP - g_GameManager.playerHP;
                if (lost > 0.5f) {            // 피격 — 즉시 추가 축소 + 빨간 비네트
                    g_WindowSizeCur -= lost * 3.0f;
                    g_HurtVignette = 0.5f;
                }
                g_WinPrevHP = g_GameManager.playerHP;
                // 부드럽게 target 으로 복귀
                g_WindowSizeCur += (target - g_WindowSizeCur) * (delta * 5.0f > 1.0f ? 1.0f : delta * 5.0f);
                if (g_WindowSizeCur < 180.0f) g_WindowSizeCur = 180.0f;  // 최소 시야
                // 적용 (플레이어 중심 고정)
                playerWin.width = playerWin.height = g_WindowSizeCur;
                playerWin.x = pCX - g_WindowSizeCur * 0.5f;
                playerWin.y = pCY - g_WindowSizeCur * 0.5f;
            }
            if (g_HurtVignette > 0.0f) { g_HurtVignette -= delta * 1.6f; if (g_HurtVignette < 0.0f) g_HurtVignette = 0.0f; }

            // 시즈탱크: 정지 시 매 1초 stack +1 (최대 5)
            if (g_Stats.siegeTank) {
                if (moving) {
                    g_Stats.siegeStacks = 0;
                    g_SiegeTick = 0.0f;
                } else {
                    g_SiegeTick += delta;
                    while (g_SiegeTick >= 1.0f && g_Stats.siegeStacks < 5) {
                        g_SiegeTick -= 1.0f;
                        ++g_Stats.siegeStacks;
                    }
                }
            }

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

            // 적 사망 파티클 업데이트 (drag + 수명)
            for (auto& p : g_EnemyParts) {
                if (!p.active) continue;
                p.life -= delta;
                if (p.life <= 0.0f) { p.active = false; continue; }
                p.x  += p.vx * delta;
                p.y  += p.vy * delta;
                p.vx *= (1.0f - 2.5f * delta);
                p.vy *= (1.0f - 2.5f * delta);
            }

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
            float rampSpawn = 1.0f + intensity * 0.55f;   // 스폰 빈도 (×1.55 @10만 … ×4.3 @60만)
            float rampHp    = 1.0f + intensity * 0.35f;   // 몹 체력
            float rampSpd   = 1.0f + intensity * 0.09f;   // 몹 속도
            // 특수 잡몹(돌진/회피/거대) 출현 확률 — 점수 비례 (초반 0 → 약 13.5만점에 45% 상한)
            int   varietyPct = (int)std::min(45.0f, (float)g_GameManager.score / 3000.0f);

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
            if (spawnTimer > spawnInterval) {
                size_t mbefore = g_MonsterManager.monsters.size();
                // 절대 상한 — 폴리2페이즈×점수램프로 한도가 1000+ 까지 폭주하던 것 방지 (성능)
                int effCap = (int)((100 + g_Stats.mobCapBonus) * p2mult * rampSpawn);
                if (effCap > 240) effCap = 240;
                g_MonsterManager.SpawnMob(screenWidth, screenHeight,
                                          effCap,
                                          g_Stats.monsterHpMult * rampHp, saX, saY, saW, saH,
                                          varietyPct);
                // 디버프 보유 시 일부 몹을 분열체/점멸체로 (특수 잡몹 안 된 경우만 — 중복 변환 방지)
                if (g_MonsterManager.monsters.size() > mbefore) {
                    Monster* nm = g_MonsterManager.monsters.back();
                    if (nm->kind == MobKind::NORMAL) {
                        if (g_Stats.splitterMobs && (rand() % 100) < 25)
                            nm->MakeKind(MobKind::SPLITTER, 0, 1.2f);
                        else if (g_Stats.blinkerMobs && (rand() % 100) < 25)
                            nm->MakeKind(MobKind::BLINKER);
                        else if (g_Stats.orbiterMobs && (rand() % 100) < 22)
                            nm->MakeKind(MobKind::ORBITER);
                        else if (g_Stats.spawnerMobs && (rand() % 100) < 14)
                            nm->MakeKind(MobKind::SPAWNER);
                        else if (g_Stats.shieldedMobs && (rand() % 100) < 22)
                            nm->MakeKind(MobKind::SHIELDED);
                    }
                }
                spawnTimer = 0.0f;
            }

            // 게임 시간 카운트
            g_GameTime += delta;

            // 보스 스폰 — 일반 보스 20만점마다 / 폴리모프 50만점 고정.
            //   (어떤 보스든 살아있으면 대기 = 동시 스폰 방지)
            {
                bool bossActive = g_MonsterManager.boss || g_GlitchBoss ||
                                  g_RRBoss || g_PolyBoss || !g_Slimelings.empty();
                bool spawnedNow = false;
                float bsx = screenWidth  * 0.5f + ((float)(rand() % 200) - 100.0f);
                float bsy = screenHeight * 0.5f + ((float)(rand() % 200) - 100.0f);
                float bossHpC = GetDifficultyParams(g_Difficulty).bossHp;
                float polyHpC = (g_Difficulty == Difficulty::EASY) ? 10000.0f
                              : (g_Difficulty == Difficulty::HARD) ? 75000.0f : 30000.0f;

                if (g_CreativeBossPending && !bossActive) {
                    // 크리에이티브: 선택한 보스 즉시 소환 (점수 무관, 1회)
                    g_CreativeBossPending = false;
                    switch (g_CreativeBossPick) {
                    case 0: g_MonsterManager.boss =
                                new Boss(bsx, bsy, screenWidth, screenHeight, bossHpC); break;
                    case 1: g_GlitchBoss = new GlitchBoss(screenWidth, screenHeight, bossHpC * 0.7f); break;
                    case 2: g_RRBoss = new ReloadRunnerBoss(screenWidth, screenHeight, bossHpC); break;
                    default: g_PolyBoss = new PolymorphBoss(screenWidth, screenHeight, polyHpC);
                             g_PolyPrevForm = -1; break;
                    }
                    spawnedNow = true;
                }
                else if (!bossActive && !g_PolySpawned &&
                    g_GameManager.score >= 500000) {
                    // 폴리모프 — 50만점 고정 (1회) · 자체 HP (이지10k/노말30k/하드75k)
                    g_PolySpawned = true;
                    float pHp = (g_Difficulty == Difficulty::EASY) ? 10000.0f
                              : (g_Difficulty == Difficulty::HARD) ? 75000.0f : 30000.0f;
                    g_PolyBoss = new PolymorphBoss(screenWidth, screenHeight, pHp);
                    g_PolyPrevForm = -1;
                    spawnedNow = true;
                }
                else if (!bossActive &&
                         g_GameManager.score >= g_NextBossScore) {
                    // 일반 보스 (슬라임/글리치/리로드 랜덤) — 다음 임계 +20만
                    g_NextBossScore += 200000;
                    float bossHp = GetDifficultyParams(g_Difficulty).bossHp;
                    int pick = rand() % 3;
                    if (pick == 0) {
                        g_MonsterManager.boss =
                            new Boss(bsx, bsy, screenWidth, screenHeight, bossHp);
                    } else if (pick == 1) {
                        g_GlitchBoss = new GlitchBoss(screenWidth, screenHeight, bossHp * 0.7f);
                    } else {
                        g_RRBoss = new ReloadRunnerBoss(screenWidth, screenHeight, bossHp);
                    }
                    spawnedNow = true;
                }

                if (spawnedNow) {
                    // 공통 등장 연출 — 큰 화면 흔들기 + 충격파 + 빵빵 폭발
                    g_ShakeTime = 0.6f; g_ShakeMag = 28.0f;
                    SpawnShockWave(bsx, bsy, 500.0f, 0.9f, 1.0f, 0.3f, 0.3f);
                    SpawnShockWave(bsx, bsy, 320.0f, 0.7f, 1.0f, 0.8f, 0.2f);
                    for (int k = 0; k < 3; k++) {
                        SpawnEnemyExplosion(bsx + (rand()%200 - 100),
                                            bsy + (rand()%200 - 100),
                                            1.0f, 0.3f, 0.3f, true);
                    }
                }
            }

            // 자폭병 spawn (난이도별 시작 시간/주기, 쉬움은 안 나옴)
            DifficultyParams dp = GetDifficultyParams(g_Difficulty);
            if (g_GameTime >= dp.bomberStartTime) {
                g_BomberSpawnTimer += delta;
                if (g_BomberSpawnTimer >= dp.bomberInterval / (p2mult * rampSpawn)) {
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
            int rangedMax = (int)((dp.rangedMaxBase + g_Stats.rmobMaxBonus) * p2mult
                                  + intensity * 2.0f);           // 점수당 동시 +2
            if (rangedMax > 16) rangedMax = 16;   // 창 개수 = scissor 패스 수 → 상한 (성능)
            if (rangedSpawnTimer > rangedInterval) {
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
                    if (dr.fireTimer >= g_Stats.fireInterval * 2.0f) {
                        float nd = 1e9f, tx = 0, ty = 0;
                        for (auto m : g_MonsterManager.monsters) {
                            if (!m->alive) continue;
                            float ddx = m->worldX - droneX, ddy = m->worldY - droneY;
                            float ds  = ddx*ddx + ddy*ddy;
                            if (ds < nd) { nd = ds; tx = m->worldX; ty = m->worldY; }
                        }
                        for (auto r : g_MonsterManager.rangedMobs) {
                            if (!r->alive) continue;
                            float ddx = r->worldX - droneX, ddy = r->worldY - droneY;
                            float ds  = ddx*ddx + ddy*ddy;
                            if (ds < nd) { nd = ds; tx = r->worldX; ty = r->worldY; }
                        }
                        if (nd < 1e8f) {
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
                        ch.angle += 5.0f * delta;   // 회전 속도 ×2 버프
                        float chx = pCX + cosf(ch.angle) * CHAKRAM_RADIUS;
                        float chy = pCY + sinf(ch.angle) * CHAKRAM_RADIUS;
                        // 잡몹 접촉 — 즉사 + 차크람 hp -5
                        for (auto m : g_MonsterManager.monsters) {
                            if (!m->alive) continue;
                            float ddx = m->worldX - chx, ddy = m->worldY - chy;
                            if (ddx*ddx + ddy*ddy < (CHAKRAM_SIZE*0.6f) * (CHAKRAM_SIZE*0.6f)) {
                                m->alive = false;
                                ch.hp -= 5.0f;
                            }
                        }
                        // 적 총알 충돌
                        for (auto& bb : g_Bullets) {
                            if (!bb.active || !bb.isEnemy) continue;
                            float ddx = bb.x - chx, ddy = bb.y - chy;
                            if (ddx*ddx + ddy*ddy < (CHAKRAM_SIZE*0.6f) * (CHAKRAM_SIZE*0.6f)) {
                                ch.hp -= 10.0f;
                                bb.active = false;
                            }
                        }
                        if (ch.hp <= 0.0f) {
                            ch.alive = false;
                            ch.respawnTimer = 10.0f;
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
                if (g_BulletRainTimer >= g_Stats.bulletRainCooldown) {
                    g_BulletRainTimer = 0.0f;
                    const int N = 20;
                    for (int i = 0; i < N; i++) {
                        float a = (float)i / (float)N * 2.0f * (float)M_PI
                                + ((float)(rand() % 100) - 50.0f) * 0.005f;
                        Bullet nb(pCX, pCY,
                                  pCX + cosf(a) * 100.0f,
                                  pCY + sinf(a) * 100.0f);
                        nb.speed      = g_Stats.bulletSpeed * 0.85f;
                        nb.color      = glm::vec3(1.0f, 0.5f, 0.2f);
                        nb.homing     = true;
                        nb.homingTurn = 6.0f;   // rad/s — 비교적 민첩
                        nb.dmgMult    = 0.5f;
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
                g_Bullets.push_back(nb);
            };

            // Twin: ±5도 2발 / Shotgun: 5발 산탄 (사거리 700)
            auto spawnAimed = [&](float tx, float ty) {
                TriggerMuzzle(pCX, pCY, atan2f(ty - pCY, tx - pCX));  // 총구 섬광
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
                        g_Bullets.push_back(nb);
                    }
                } else if (g_Stats.twin) {
                    float dx = tx - pCX, dy = ty - pCY;
                    float ang = atan2f(dy, dx);
                    float off = 0.087f;
                    float r = 200.0f;
                    spawnOne(pCX + cosf(ang + off) * r, pCY + sinf(ang + off) * r);
                    spawnOne(pCX + cosf(ang - off) * r, pCY + sinf(ang - off) * r);
                } else {
                    spawnOne(tx, ty);
                }
            };

            // 검객 근접 스윙 — 조준 방향 호(arc) 안의 모든 적에게 즉시 피해
            auto meleeSwing = [&](float ang) {
                float range = 190.0f * g_Stats.playerSizeMult;
                float r2 = range * range, halfArc = 1.15f;  // ±~66° (약 130° 호)
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
                for (auto m : g_MonsterManager.monsters) {        // 잡몹
                    if (!m->alive || !inCone(m->worldX, m->worldY)) continue;
                    float dmgM = dmg;
                    if (m->kind == MobKind::SHIELDED && m->shieldActive) dmgM *= 0.15f;
                    float dealt = (dmgM < m->hp) ? dmgM : m->hp; m->hp -= dealt;
                    SpawnDamageNumber(m->worldX, m->worldY, dealt, dealt >= 40.0f || crit);
                    if (m->hp <= 0.0f) {
                        m->alive = false; m->scored = true; AddKillCombo();
                        float bx, bs; MobKillReward(m->kind, m->splitGen, bx, bs);
                        g_GameManager.xp += (long long)((bx + (float)g_Stats.meleeXpBonus) * g_Stats.xpMult);
                        g_Stats.killCount++; g_GameManager.scoreAccum += bs;
                        g_GameManager.score = (long long)g_GameManager.scoreAccum;
                        onKill();
                    }
                }
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
                for (auto* c : g_Slimelings) if (c->alive)
                    hitB(c->worldX, c->worldY, c->hp, c->alive);
                if (g_GlitchBoss && g_GlitchBoss->alive)
                    hitB(g_GlitchBoss->worldX, g_GlitchBoss->worldY, g_GlitchBoss->hp, g_GlitchBoss->alive);
                if (g_RRBoss && g_RRBoss->alive)
                    hitB(g_RRBoss->worldX, g_RRBoss->worldY, g_RRBoss->hp, g_RRBoss->alive);
                if (g_PolyBoss && g_PolyBoss->alive && g_PolyBoss->damageable())
                    hitB(g_PolyBoss->worldX, g_PolyBoss->worldY, g_PolyBoss->hp, g_PolyBoss->alive);
                SpawnSlash(pCX, pCY, ang, range);
                TriggerHitStop(0.015f);
            };

            // 포탑 모드에서는 플레이어가 발사하지 않음
            if (!g_Stats.turretMode) {
                if (g_Stats.meleeWeapon) {       // 검객 — 근접 스윙 (총알 없음)
                    if (lmb && fireTimer >= effInterval) {
                        float ang = atan2f(wmy - pCY, wmx - pCX);
                        meleeSwing(ang);
                        fireTimer = 0.0f;
                    }
                    if (!lmb) fireTimer = effInterval;
                } else if (g_Stats.brokenSight) {
                    if (fireTimer >= effInterval && g_Orb.active) {
                        spawnAimed(g_Orb.x, g_Orb.y);
                        fireTimer = 0.0f;
                    }
                } else {
                    if (lmb && fireTimer >= effInterval) {
                        float tx = wmx, ty = wmy;   // 줌 보정한 월드 조준점
                        if (g_DrunkActive) {
                            float a = (float)(rand() % 628) * 0.01f;
                            tx = pCX + cosf(a) * 200.0f;
                            ty = pCY + sinf(a) * 200.0f;
                        }
                        spawnAimed(tx, ty);
                        fireTimer = 0.0f;
                    }
                    // 클릭 release 시 타이머 리셋 — 다음 클릭에 즉시 발사 가능 (일반 무기 UX)
                    // 단발 고화력 무기(대포·샷건·저격)는 연타로 연사 우회 방지 → 리셋 스킵
                    if (!lmb && !g_Stats.cannon && !g_Stats.shotgun && !g_Stats.sniper)
                        fireTimer = effInterval;
                }
            }
        }

        // GameManager 가 호버/변환 상태를 알 수 있게 동기화 (Render 에서 사용)
        g_GameManager.hoveredCard = g_HoveredAug;
        g_GameManager.conversionAug = g_ConversionWeapon;

        g_GameManager.UpdateTitle(window);

        // ============================================================
        // 렌더링
        // ============================================================
        glViewport(0, 0, screenWidth, screenHeight);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader);
        // 화면 흔들기 + 줌 적용 — game world 만, HUD/text(별도 ortho)는 영향 없음
        float orthoShake[16];
        memcpy(orthoShake, ortho, sizeof(ortho));
        // 줌(중심=화면 중앙). z=1 이면 base ortho 와 동일(identity)
        {
            float z = g_ViewZoom;
            orthoShake[0]  =  2.0f * z / (float)screenWidth;
            orthoShake[5]  = -2.0f * z / (float)screenHeight;
            orthoShake[12] = -z;
            orthoShake[13] =  z;
        }
        if (g_ShakeTime > 0.0f) {
            float intensity = g_ShakeMag * (g_ShakeTime / 0.6f);
            if (intensity > g_ShakeMag) intensity = g_ShakeMag;
            float sx = ((float)(rand() % 200) - 100.0f) / 100.0f * intensity;
            float sy = ((float)(rand() % 200) - 100.0f) / 100.0f * intensity;
            orthoShake[12] -= 2.0f * sx / (float)screenWidth;
            orthoShake[13] += 2.0f * sy / (float)screenHeight;
        }
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, orthoShake);
        // 글로벌에도 동기화 — BindMainShader() 가 이 값 사용
        memcpy(g_MainOrtho, orthoShake, sizeof(orthoShake));
        glBindVertexArray(VAO);

        // 원거리 몹 FakeWindow 크기 상수 (렌더·클리핑 공용)
        const float RFW_W = 500.0f;
        const float RFW_H = 500.0f;

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
        glDisable(GL_BLEND);
        BindMainShader();

        // (a0-1) 자폭병 충격파 배경 — needsBg 플래그가 설정된 충격파에 한해
        for (auto& sw : g_ShockWaves) {
            if (!sw.active || !sw.needsBg) continue;
            float t   = 1.0f - sw.life / sw.maxLife;  // 0→1
            float bgR = sw.maxRadius * t + 30.0f;      // 링보다 조금 크게
            drawCircle(sw.x, sw.y, bgR, 0.08f, 0.08f, 0.10f, 1.0f);
        }

        // (a0-2) 텔레그래프 배경 — 돌진 예고선 주변 ~100px 어두운 띠
        if (g_MonsterManager.boss && g_MonsterManager.boss->alive) {
            auto* bs0 = g_MonsterManager.boss;
            if (bs0->skill == Boss::Skill::TELEGRAPH) {
                float prog0  = bs0->skillTimer / Boss::TELEGRAPH_TIME;
                if (prog0 < 0.0f) prog0 = 0.0f;
                if (prog0 > 1.0f) prog0 = 1.0f;
                float maxLen0 = (float)screenWidth + (float)screenHeight;
                float len0    = maxLen0 * prog0 + 100.0f;   // 진행 방향 100px 여유
                float thick0  = (80.0f + 20.0f * prog0) + 200.0f; // 양쪽 100px

                // 시작점: 보스 뒤 100px (방향 반대)
                float sx0 = bs0->worldX - bs0->chargeDirX * 100.0f;
                float sy0 = bs0->worldY - bs0->chargeDirY * 100.0f;
                float ex0 = bs0->worldX + bs0->chargeDirX * len0;
                float ey0 = bs0->worldY + bs0->chargeDirY * len0;
                float px0 = -bs0->chargeDirY, py0 = bs0->chargeDirX;
                float q1x = sx0 + px0 * thick0 * 0.5f, q1y = sy0 + py0 * thick0 * 0.5f;
                float q2x = sx0 - px0 * thick0 * 0.5f, q2y = sy0 - py0 * thick0 * 0.5f;
                float q3x = ex0 + px0 * thick0 * 0.5f, q3y = ey0 + py0 * thick0 * 0.5f;
                float q4x = ex0 - px0 * thick0 * 0.5f, q4y = ey0 - py0 * thick0 * 0.5f;
                float vt[12] = { q1x, q1y, q2x, q2y, q3x, q3y,
                                 q2x, q2y, q4x, q4y, q3x, q3y };
                glUniform4f(g_colorLoc, 0.08f, 0.08f, 0.10f, 1.0f);
                glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vt), vt);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }

        // (a) 원거리 몹 + 포탑 + 보스 FakeWindow 배경 — 블렌드 OFF 로 직접 덮어쓰기
        //     겹쳐서 또 그려도 같은 색이 그대로 쓰여 누적 없음.
        // 포탑 250×250 창 배경 (다수)
        if (g_Stats.turretMode) {
            for (auto& t : g_Turrets) {
                float twx = t.x - TURRET_WIN_W * 0.5f;
                float twy = t.y - TURRET_WIN_H * 0.5f;
                drawRect(twx, twy, TURRET_WIN_W, TURRET_WIN_H,
                         0.06f, 0.08f, 0.10f, 1.0f);
            }
        }
        for (auto r : g_MonsterManager.rangedMobs) {
            if (r->deathScale <= 0.0f) continue;
            float sc  = r->deathScale;
            float rW  = RFW_W * sc, rH = RFW_H * sc;
            float rwx = r->worldX - rW * 0.5f;
            float rwy = r->worldY - rH * 0.5f;
            drawRect(rwx, rwy, rW, rH,
                     0.08f, 0.08f, 0.10f, 1.0f);
        }
        if (g_MonsterManager.boss && g_MonsterManager.boss->alive) {
            auto* bs = g_MonsterManager.boss;
            float bwx = bs->worldX - Boss::WIN_W * 0.5f;
            float bwy = bs->worldY - Boss::WIN_H * 0.5f;
            drawRect(bwx, bwy, Boss::WIN_W, Boss::WIN_H,
                     0.08f, 0.08f, 0.10f, 1.0f);
        }
        // 신규 보스 — 각자 개인 창 배경 (본체가 맨 배경에 떠 보이지 않도록)
        if (g_GlitchBoss && g_GlitchBoss->alive) {
            float w = GLITCH_WIN_W;
            drawRect(g_GlitchBoss->worldX - w*0.5f, g_GlitchBoss->worldY - w*0.5f,
                     w, w, 0.07f, 0.06f, 0.10f, 1.0f);
        }
        if (g_RRBoss && g_RRBoss->alive) {
            float w = RR_WIN_W;
            drawRect(g_RRBoss->worldX - w*0.5f, g_RRBoss->worldY - w*0.5f,
                     w, w, 0.10f, 0.07f, 0.06f, 1.0f);
        }
        if (g_PolyBoss && g_PolyBoss->alive) {
            float w = POLY_WIN_W;
            drawRect(g_PolyBoss->worldX - w*0.5f, g_PolyBoss->worldY - w*0.5f,
                     w, w, 0.09f, 0.06f, 0.11f, 1.0f);
        }
        // 슬라임 분열체 — 각자 개인 창 (크기 비례)
        for (auto* c : g_Slimelings) {
            if (!c->alive) continue;
            float w = Boss::WIN_W * c->sizeScale;
            drawRect(c->worldX - w*0.5f, c->worldY - w*0.5f, w, w,
                     0.07f, 0.10f, 0.07f, 1.0f);
        }
        glEnable(GL_BLEND);  // 이후는 일반 알파 블렌딩

        // (b) 원거리 몹 + 보스 창 내부 컨텐츠 (잡몹·자폭병·총알·파편)
        //     각 창마다 scissor 패스. 다이아몬드/본체는 (e2)/(e3) 에서 별도로 그림
        glEnable(GL_SCISSOR_TEST);
        for (auto r : g_MonsterManager.rangedMobs) {
            if (r->deathScale <= 0.0f) continue;
            float sc  = r->deathScale;
            float rW  = RFW_W * sc, rH = RFW_H * sc;
            float rwx = r->worldX - rW * 0.5f;
            float rwy = r->worldY - rH * 0.5f;
            WorldScissor(rwx, rwy, rW, rH);
            // 잡몹 (보스 소환물은 더 큼) — 창 밖은 컬링
            for (auto m : g_MonsterManager.monsters) {
                if (!m->alive || !inWin(m->worldX, m->worldY, rwx, rwy, rW, rH)) continue;
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
                drawRect(orb.x - 18, orb.y - 18, 36, 36, 0.95f, 0.0f, 0.0f, 0.30f);
                drawRect(orb.x - 14, orb.y - 14, 28, 28, 0.95f, 0.05f, 0.05f, 1.0f);
            }
        }
        // (b'') 포탑 창 안 컨텐츠 (다수)
        if (g_Stats.turretMode) {
            for (auto& t : g_Turrets) {
                float twx = t.x - TURRET_WIN_W * 0.5f;
                float twy = t.y - TURRET_WIN_H * 0.5f;
                WorldScissor(twx, twy, TURRET_WIN_W, TURRET_WIN_H);
                for (auto m : g_MonsterManager.monsters) {
                    if (!m->alive || !inWin(m->worldX, m->worldY, twx, twy, TURRET_WIN_W, TURRET_WIN_H)) continue;
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
        if (g_MonsterManager.boss && g_MonsterManager.boss->alive) {
            auto* bs = g_MonsterManager.boss;
            float bwx = bs->worldX - Boss::WIN_W * 0.5f;
            float bwy = bs->worldY - Boss::WIN_H * 0.5f;
            WorldScissor(bwx, bwy, Boss::WIN_W, Boss::WIN_H);
            for (auto m : g_MonsterManager.monsters) {
                if (!m->alive || !inWin(m->worldX, m->worldY, bwx, bwy, Boss::WIN_W, Boss::WIN_H)) continue;
                drawMob(m);
            }
            for (auto bm : g_MonsterManager.bombers) {
                if (!bm->alive || !inWin(bm->worldX, bm->worldY, bwx, bwy, Boss::WIN_W, Boss::WIN_H)) continue;
                drawPentagon(bm->worldX, bm->worldY, Bomber::SIZE_PX,
                             bm->color.r, bm->color.g, bm->color.b, 1.0f);
            }
            for (auto& b : g_Bullets) {
                if (!b.active || !inWin(b.x, b.y, bwx, bwy, Boss::WIN_W, Boss::WIN_H)) continue;
                drawBullet(b);
            }
            for (auto& orb : g_ApproachOrbs) {
                drawRect(orb.x - 18, orb.y - 18, 36, 36, 0.95f, 0.0f, 0.0f, 0.30f);
                drawRect(orb.x - 14, orb.y - 14, 28, 28, 0.95f, 0.05f, 0.05f, 1.0f);
            }
        }
        glDisable(GL_SCISSOR_TEST);

        // (c) 플레이어 FakeWindow 배경 — 블렌드 OFF 로 직접 덮어쓰기
        //     원거리 몹 창과 겹친 영역도 player 색으로 깔끔하게 덮임 (누적 없음)
        //     ranged 컨텐츠 (b) 가 player 영역에 그려졌으면 여기서 덮여 사라짐
        //     = "원거리 몹 창이 플레이어 창 안에 들어가면 가려짐" 원래 의도 그대로
        glDisable(GL_BLEND);
        drawRect(playerWin.x, playerWin.y, playerWin.width, playerWin.height,
                 0.08f, 0.08f, 0.10f, 1.0f);
        glEnable(GL_BLEND);

        // (d) 플레이어 캐릭터 + 증강 이펙트 + 사망 파편
        {
            float pCX = playerWin.x + playerWin.width  * 0.5f;
            float pCY = playerWin.y + playerWin.height * 0.5f;
            // 총검: 200px 이내 표시 (희미한 시안 원)
            if (g_Stats.bayonet)
                drawCircle(pCX, pCY, 200.0f, 0.4f, 1.0f, 0.9f, 0.10f);

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
            }
        }

        // (e) 플레이어 창 내부 컨텐츠 — scissor (가장 위 레이어)
        glEnable(GL_SCISSOR_TEST);
        WorldScissor(playerWin.x, playerWin.y, playerWin.width, playerWin.height);
        {
        float pwx = playerWin.x, pwy = playerWin.y, pww = playerWin.width, pwh = playerWin.height;
        // 잡몹 (보스 소환물은 더 큼) — 창 밖 컬링
        for (auto m : g_MonsterManager.monsters) {
            if (!m->alive || !inWin(m->worldX, m->worldY, pwx, pwy, pww, pwh)) continue;
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
            drawRect(orb.x - 18, orb.y - 18, 36, 36, 0.95f, 0.0f, 0.0f, 0.30f);
            drawRect(orb.x - 14, orb.y - 14, 28, 28, 0.95f, 0.05f, 0.05f, 1.0f);
        }
        glDisable(GL_SCISSOR_TEST);

        // (e2) 원거리 몹 다이아몬드 — 각 원거리 몹 창 영역에서 항상 위에 그림
        glEnable(GL_SCISSOR_TEST);
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
        glDisable(GL_SCISSOR_TEST);

        // (e2.1) 포탑 아이콘 + 수명바 (다수) — 각 창 영역 scissor 내에서 표시
        if (g_Stats.turretMode) {
            glEnable(GL_SCISSOR_TEST);
            for (auto& t : g_Turrets) {
                float twx = t.x - TURRET_WIN_W * 0.5f;
                float twy = t.y - TURRET_WIN_H * 0.5f;
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
            glDisable(GL_SCISSOR_TEST);
        }

        // (e2.5) 보스 텔레그래프 — scissor 없이 전체 화면에 표시
        //        보스 창 밖에서도 보이도록 (e3) 의 scissor 이전에 그림
        //        보스 위치에서 돌진 방향으로 점점 늘어나는 빨간 구역
        if (g_MonsterManager.boss && g_MonsterManager.boss->alive) {
            auto* bs = g_MonsterManager.boss;
            if (bs->skill == Boss::Skill::TELEGRAPH) {
                // 0→1 으로 커지는 진행도
                float prog  = bs->skillTimer / Boss::TELEGRAPH_TIME;
                if (prog < 0.0f) prog = 0.0f;
                if (prog > 1.0f) prog = 1.0f;
                float maxLen = (float)screenWidth + (float)screenHeight;
                float len    = maxLen * prog;
                float thick  = 80.0f + 20.0f * prog; // 폭도 살짝 확장
                float alpha  = 0.12f + 0.28f * prog;  // 처음엔 희미, 끝엔 진하게

                float ex = bs->worldX + bs->chargeDirX * len;
                float ey = bs->worldY + bs->chargeDirY * len;
                float perpX = -bs->chargeDirY, perpY = bs->chargeDirX;
                float p1x = bs->worldX + perpX * thick * 0.5f, p1y = bs->worldY + perpY * thick * 0.5f;
                float p2x = bs->worldX - perpX * thick * 0.5f, p2y = bs->worldY - perpY * thick * 0.5f;
                float p3x = ex + perpX * thick * 0.5f, p3y = ey + perpY * thick * 0.5f;
                float p4x = ex - perpX * thick * 0.5f, p4y = ey - perpY * thick * 0.5f;
                float v[12] = { p1x, p1y, p2x, p2y, p3x, p3y,
                                p2x, p2y, p4x, p4y, p3x, p3y };
                BindMainShader();
                glUniform4f(g_colorLoc, 1.0f, 0.08f, 0.08f, alpha);
                glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
            // 소환 경고 — 잡몹이 나올 자리에 점멸 링 (소환 0.7초 전부터)
            if (bs->summonPending) {
                BindMainShader();
                float blink = 0.30f + 0.30f * (0.5f + 0.5f *
                              sinf((float)glfwGetTime() * 16.0f));
                for (int i = 0; i < Boss::SUMMON_COUNT; i++) {
                    float ang = (float)i / Boss::SUMMON_COUNT * 6.2831853f;
                    float sx  = bs->worldX + cosf(ang) * Boss::SUMMON_RING_R;
                    float sy  = bs->worldY + sinf(ang) * Boss::SUMMON_RING_R;
                    drawCircle(sx, sy, 14.0f, 1.0f, 0.55f, 0.2f, blink);
                }
            }
        }

        // (e3) 보스 본체 (Mercedes 모양) + HP 바
        if (g_MonsterManager.boss && g_MonsterManager.boss->alive) {
            auto* bs = g_MonsterManager.boss;

            // 보스 창 영역 scissor (플레이어 창 위에 가려도 보임)
            glEnable(GL_SCISSOR_TEST);
            float bwx = bs->worldX - Boss::WIN_W * 0.5f;
            float bwy = bs->worldY - Boss::WIN_H * 0.5f;
            WorldScissor(bwx, bwy, Boss::WIN_W, Boss::WIN_H);

            // 본체 — Mercedes 로고
            float bodyColR = (bs->skill == Boss::Skill::TELEGRAPH) ? 1.0f : 0.95f;
            float bodyColG = (bs->skill == Boss::Skill::TELEGRAPH) ? 0.4f : 0.85f;
            float bodyColB = (bs->skill == Boss::Skill::TELEGRAPH) ? 0.4f : 0.95f;
            drawMercedes(bs->worldX, bs->worldY, Boss::BODY_SIZE,
                         bodyColR, bodyColG, bodyColB, 1.0f);

            // HP 바 (본체 위)
            float hpFrac = bs->hp / bs->maxHp;
            if (hpFrac < 0) hpFrac = 0; if (hpFrac > 1) hpFrac = 1;
            float hbW = 140.0f, hbH = 8.0f;
            float hbX = bs->worldX - hbW * 0.5f;
            float hbY = bs->worldY - Boss::BODY_SIZE - 18.0f;
            drawRect(hbX, hbY, hbW, hbH, 0.15f, 0.15f, 0.2f, 0.85f);
            drawRect(hbX, hbY, hbW * hpFrac, hbH,
                     0.95f, 0.3f, 0.4f, 0.95f);

            glDisable(GL_SCISSOR_TEST);
        }

        // (e3.5) 슬라임 분열체 — 돌진 경고(전체화면) + 본체/HP/총알(개인 창)
        for (auto* c : g_Slimelings) {
            if (!c->alive) continue;
            float scale = c->sizeScale;
            float body  = Boss::BODY_SIZE * scale;
            float win   = Boss::WIN_W * scale;
            // 돌진 텔레그래프 (전체 화면, 붉은 띠)
            if (c->skill == Boss::Skill::TELEGRAPH) {
                BindMainShader();
                float prog = c->skillTimer / Boss::TELEGRAPH_TIME;
                if (prog < 0) prog = 0; if (prog > 1) prog = 1;
                float len   = ((float)screenWidth + (float)screenHeight) * prog;
                float thick = (50.0f + 14.0f * prog) * scale;
                float ex = c->worldX + c->chargeDirX * len;
                float ey = c->worldY + c->chargeDirY * len;
                float perpX = -c->chargeDirY, perpY = c->chargeDirX;
                float p1x=c->worldX+perpX*thick*0.5f, p1y=c->worldY+perpY*thick*0.5f;
                float p2x=c->worldX-perpX*thick*0.5f, p2y=c->worldY-perpY*thick*0.5f;
                float p3x=ex+perpX*thick*0.5f, p3y=ey+perpY*thick*0.5f;
                float p4x=ex-perpX*thick*0.5f, p4y=ey-perpY*thick*0.5f;
                float v[12]={p1x,p1y,p2x,p2y,p3x,p3y, p2x,p2y,p4x,p4y,p3x,p3y};
                glUniform4f(g_colorLoc, 1.0f, 0.2f, 0.5f, 0.10f + 0.26f * prog);
                glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
            // 본체 + HP + 총알 — 개인 창 클리핑
            glEnable(GL_SCISSOR_TEST);
            WorldScissor(c->worldX - win*0.5f, c->worldY - win*0.5f, win, win);
            for (auto& b : g_Bullets) {
                if (!b.active) continue;
                drawBullet(b);
            }
            bool tel = (c->skill == Boss::Skill::TELEGRAPH);
            drawMercedes(c->worldX, c->worldY, body,
                         tel ? 1.0f : c->color.r,
                         tel ? 0.4f : c->color.g,
                         tel ? 0.4f : c->color.b, 1.0f);
            float hpFrac = c->hp / c->maxHp; if(hpFrac<0)hpFrac=0; if(hpFrac>1)hpFrac=1;
            float hbW = 120.0f * scale, hbH = 7.0f;
            float hbX = c->worldX - hbW*0.5f, hbY = c->worldY - body - 14.0f;
            drawRect(hbX, hbY, hbW, hbH, 0.12f, 0.18f, 0.12f, 0.85f);
            drawRect(hbX, hbY, hbW*hpFrac, hbH, 0.4f, 0.95f, 0.5f, 0.95f);
            glDisable(GL_SCISSOR_TEST);
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
            // 외곽 어두운 원 + 중앙 십자
            drawCircle(ax, ay, 12.0f, 0.0f, 0.0f, 0.0f, 0.6f);
            drawCircle(ax, ay, 10.0f, 1.0f, 0.95f, 0.3f, 0.9f);
            drawCircle(ax, ay, 5.0f, 0.05f, 0.05f, 0.05f, 0.9f);
            // 중심 점
            drawRect(ax - 1.5f, ay - 1.5f, 3.0f, 3.0f,
                     1.0f, 1.0f, 1.0f, 1.0f);
            // 4방향 짧은 라인 (십자)
            drawRect(ax - 14.0f, ay - 1.0f, 6.0f, 2.0f, 1.0f, 0.95f, 0.3f, 0.95f);
            drawRect(ax + 8.0f,  ay - 1.0f, 6.0f, 2.0f, 1.0f, 0.95f, 0.3f, 0.95f);
            drawRect(ax - 1.0f, ay - 14.0f, 2.0f, 6.0f, 1.0f, 0.95f, 0.3f, 0.95f);
            drawRect(ax - 1.0f, ay + 8.0f,  2.0f, 6.0f, 1.0f, 0.95f, 0.3f, 0.95f);
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

        // (g3) 글리치 보스 — 레이저 + 미니 세모 + 본체 + HP 바 (스크린 좌표 최상단)
        if (g_GlitchBoss && g_GlitchBoss->alive) {
            auto* gb = g_GlitchBoss;
            BindMainShader();
            // 레이저 경고선 (발사 전 0.8초) — 얇은 점멸선
            if (gb->laserWarn) {
                float ex = gb->worldX + gb->laserDirX * (float)(screenWidth + screenHeight);
                float ey = gb->worldY + gb->laserDirY * (float)(screenWidth + screenHeight);
                float pxx = -gb->laserDirY, pyy = gb->laserDirX, th = 3.0f;
                float wa = 0.4f + 0.4f * (0.5f + 0.5f * sinf((float)glfwGetTime() * 18.0f));
                float p1x=gb->worldX+pxx*th, p1y=gb->worldY+pyy*th;
                float p2x=gb->worldX-pxx*th, p2y=gb->worldY-pyy*th;
                float p3x=ex+pxx*th, p3y=ey+pyy*th, p4x=ex-pxx*th, p4y=ey-pyy*th;
                float v[12]={p1x,p1y,p2x,p2y,p3x,p3y, p2x,p2y,p4x,p4y,p3x,p3y};
                glUniform4f(g_colorLoc, 1.0f, 0.3f, 0.7f, wa);
                glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
            // 레이저 (BURST 동안 화면 가로지르는 직선)
            if (gb->laserActive) {
                float ex = gb->worldX + gb->laserDirX * (float)(screenWidth + screenHeight);
                float ey = gb->worldY + gb->laserDirY * (float)(screenWidth + screenHeight);
                float pxx = -gb->laserDirY, pyy = gb->laserDirX;
                for (int pass = 0; pass < 2; pass++) {
                    float th = (pass == 0) ? 30.0f : 10.0f;
                    float lr = (pass == 0) ? 1.0f : 1.0f;
                    float lg = (pass == 0) ? 0.2f : 0.8f;
                    float lb = (pass == 0) ? 0.5f : 0.9f;
                    float la = (pass == 0) ? 0.45f : 0.95f;
                    float p1x=gb->worldX+pxx*th, p1y=gb->worldY+pyy*th;
                    float p2x=gb->worldX-pxx*th, p2y=gb->worldY-pyy*th;
                    float p3x=ex+pxx*th, p3y=ey+pyy*th;
                    float p4x=ex-pxx*th, p4y=ey-pyy*th;
                    float v[12]={p1x,p1y,p2x,p2y,p3x,p3y, p2x,p2y,p4x,p4y,p3x,p3y};
                    glUniform4f(g_colorLoc, lr, lg, lb, la);
                    glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }
            }
            // 미니 세모 (작고 빠름 — 유도 중엔 빨강)
            for (auto& t : gb->minis) {
                if (t.homing) drawTriangle(t.x, t.y, 11.0f, 1.0f, 0.2f, 0.2f, 1.0f);
                else          drawTriangle(t.x, t.y, 9.0f,  0.9f, 0.3f, 0.95f, 1.0f);
            }
            // 본체 + HP + 총알 — 개인 창 영역으로 클리핑 (맨 배경에 떠 보이지 않게)
            glEnable(GL_SCISSOR_TEST);
            WorldScissor(gb->worldX - GLITCH_WIN_W*0.5f, gb->worldY - GLITCH_WIN_W*0.5f,
                         GLITCH_WIN_W, GLITCH_WIN_W);
            // 총알 (이 창 안에서도 보이도록)
            for (auto& b : g_Bullets) {
                if (!b.active) continue;
                drawBullet(b);
            }
            // 본체 — RGB 분리된 글리치 다이아몬드
            drawDiamond(gb->worldX + 3, gb->worldY, GlitchBoss::BODY, 1.0f, 0.1f, 0.4f, 0.55f);
            drawDiamond(gb->worldX - 3, gb->worldY, GlitchBoss::BODY, 0.1f, 0.9f, 1.0f, 0.55f);
            drawDiamond(gb->worldX, gb->worldY, GlitchBoss::BODY, 0.92f, 0.92f, 0.98f, 1.0f);
            // HP 바
            float hpFrac = gb->hp / gb->maxHp;
            if (hpFrac < 0) hpFrac = 0; if (hpFrac > 1) hpFrac = 1;
            float hbW = 160.0f, hbH = 8.0f;
            float hbX = gb->worldX - hbW * 0.5f;
            float hbY = gb->worldY - GlitchBoss::BODY - 18.0f;
            drawRect(hbX, hbY, hbW, hbH, 0.15f, 0.1f, 0.15f, 0.85f);
            drawRect(hbX, hbY, hbW * hpFrac, hbH, 0.95f, 0.25f, 0.55f, 0.95f);
            glDisable(GL_SCISSOR_TEST);
        }

        // (g4) 리로드 러너 — 무기 전조(저격선/MG 부채꼴) + 본체 + HP + [RELOADING]
        if (g_RRBoss && g_RRBoss->alive) {
            auto* rb = g_RRBoss;
            float pCX = playerWin.x + playerWin.width  * 0.5f;
            float pCY = playerWin.y + playerWin.height * 0.5f;
            BindMainShader();

            // SNIPER 정지 조준선 (보스 → 플레이어, 깜빡)
            if (rb->aiming) {
                float adx = pCX - rb->worldX, ady = pCY - rb->worldY;
                float ad = sqrtf(adx*adx + ady*ady) + 1e-3f;
                float dxn = adx/ad, dyn = ady/ad;
                float ex = rb->worldX + dxn * (float)(screenWidth + screenHeight);
                float ey = rb->worldY + dyn * (float)(screenWidth + screenHeight);
                float pxx = -dyn, pyy = dxn, th = 3.0f;
                float v[12] = {
                    rb->worldX+pxx*th, rb->worldY+pyy*th, rb->worldX-pxx*th, rb->worldY-pyy*th, ex+pxx*th, ey+pyy*th,
                    rb->worldX-pxx*th, rb->worldY-pyy*th, ex-pxx*th, ey-pyy*th,                   ex+pxx*th, ey+pyy*th };
                float a = 0.55f + 0.35f * sinf((float)glfwGetTime() * 30.0f);
                glUniform4f(g_colorLoc, 0.4f, 1.0f, 1.0f, a);
                glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }

            // MACHINEGUN 부채꼴 범위 예고 (삼각 부채로 채움)
            if (rb->mgTelegraph) {
                float base = atan2f(rb->zoneDirY, rb->zoneDirX);
                float a0 = base - rb->zoneHalfAngle, a1 = base + rb->zoneHalfAngle;
                float prog  = rb->warmUpTimer / ReloadRunnerBoss::MG_WARMUP;
                float alpha = 0.10f + 0.28f * prog;
                const int SEG = 18;
                for (int s = 0; s < SEG; s++) {
                    float aa = a0 + (a1 - a0) * (float)s / SEG;
                    float ab = a0 + (a1 - a0) * (float)(s + 1) / SEG;
                    float L  = rb->zoneLen;
                    float v[6] = { rb->worldX, rb->worldY,
                                   rb->worldX + cosf(aa)*L, rb->worldY + sinf(aa)*L,
                                   rb->worldX + cosf(ab)*L, rb->worldY + sinf(ab)*L };
                    glUniform4f(g_colorLoc, 1.0f, 0.85f, 0.2f, alpha);
                    glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
                    glDrawArrays(GL_TRIANGLES, 0, 3);
                }
            }

            // SHOTGUN 부채꼴 사거리 예고 (사정거리 진입 시 플레이어 방향 옅은 콘)
            if (rb->state == RRState::ACTIVE && rb->weapon == RRWeapon::SHOTGUN) {
                float adx = pCX - rb->worldX, ady = pCY - rb->worldY;
                float ad  = sqrtf(adx*adx + ady*ady) + 1e-3f;
                if (ad < ReloadRunnerBoss::SG_RANGE) {
                    float base = atan2f(ady, adx);
                    float ha   = ReloadRunnerBoss::SG_SPREAD * 0.5f;
                    float a0 = base - ha, a1 = base + ha;
                    float L  = ReloadRunnerBoss::SG_RANGE;
                    const int SEG = 12;
                    for (int s = 0; s < SEG; s++) {
                        float aa = a0 + (a1 - a0) * (float)s / SEG;
                        float ab = a0 + (a1 - a0) * (float)(s + 1) / SEG;
                        float v[6] = { rb->worldX, rb->worldY,
                                       rb->worldX + cosf(aa)*L, rb->worldY + sinf(aa)*L,
                                       rb->worldX + cosf(ab)*L, rb->worldY + sinf(ab)*L };
                        glUniform4f(g_colorLoc, 1.0f, 0.5f, 0.15f, 0.10f);
                        glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
                        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
                        glDrawArrays(GL_TRIANGLES, 0, 3);
                    }
                }
            }

            // 본체 + HP + 총알 — 개인 창 영역으로 클리핑 (맨 배경에 떠 보이지 않게)
            glEnable(GL_SCISSOR_TEST);
            WorldScissor(rb->worldX - RR_WIN_W*0.5f, rb->worldY - RR_WIN_W*0.5f,
                         RR_WIN_W, RR_WIN_W);
            // 총알 (이 창 안에서도 보이도록 — 리로드 러너 탄막 가시성 버그 fix)
            for (auto& b : g_Bullets) {
                if (!b.active) continue;
                drawBullet(b);
            }
            // 본체 — 상태/무기색 다이아몬드
            float br = 0.9f, bg = 0.9f, bb = 0.95f;
            if      (rb->state  == RRState::RELOAD_SPRINT) { br=1.0f; bg=0.9f;  bb=0.3f; }
            else if (rb->weapon == RRWeapon::SHOTGUN)      { br=1.0f; bg=0.5f;  bb=0.2f; }
            else if (rb->weapon == RRWeapon::SNIPER)       { br=0.3f; bg=1.0f;  bb=1.0f; }
            else                                           { br=1.0f; bg=0.85f; bb=0.2f; }
            drawDiamond(rb->worldX, rb->worldY, ReloadRunnerBoss::BODY, br, bg, bb, 1.0f);

            // HP 바
            float hpFrac = rb->hp / rb->maxHp;
            if (hpFrac < 0) hpFrac = 0; if (hpFrac > 1) hpFrac = 1;
            float hbW = 160.0f, hbH = 8.0f;
            float hbX = rb->worldX - hbW * 0.5f;
            float hbY = rb->worldY - ReloadRunnerBoss::BODY - 18.0f;
            drawRect(hbX, hbY, hbW, hbH, 0.15f, 0.12f, 0.1f, 0.85f);
            drawRect(hbX, hbY, hbW * hpFrac, hbH, 0.95f, 0.6f, 0.2f, 0.95f);
            glDisable(GL_SCISSOR_TEST);

            // [RELOADING...] 깜빡 텍스트
            if (rb->state == RRState::RELOAD_SPRINT &&
                ((int)(glfwGetTime() * 5.0) % 2 == 0)) {
                const wchar_t* rl = L"[ RELOADING... ]";
                float rw = g_TextS.Width(rl, 0.8f);
                g_TextS.Draw(rl, rb->worldX - rw * 0.5f,
                             rb->worldY - ReloadRunnerBoss::BODY - 46.0f, 0.8f,
                             1.0f, 0.9f, 0.3f, 0.95f);
            }
        }

        // (g5) 폴리모프 보스 — 마커/세모/레이저/차크람/본체/HP
        if (g_PolyBoss && g_PolyBoss->alive) {
            auto* pb = g_PolyBoss;
            BindMainShader();
            const float PUR_R = 0.6f, PUR_G = 0.2f, PUR_B = 0.95f;
            // 세모 쇄도 경고 (텔레그래프) — 시작 모서리 전체에 깜빡이는 화살표/띠
            if (pb->triWarn) {
                float blink = 0.35f + 0.35f * (0.5f + 0.5f * sinf((float)glfwGetTime() * 16.0f));
                int arrows = 14;
                for (int i = 0; i < arrows; i++) {
                    float t = (arrows > 1) ? (float)i / (arrows - 1) : 0.5f;
                    float ax, ay;
                    if (pb->triDirX != 0.0f) {   // 가로 이동 → 좌/우 모서리에 세로 배열
                        ax = (pb->triDirX > 0) ? 24.0f : (float)screenWidth - 24.0f;
                        ay = t * (float)screenHeight;
                    } else {                     // 세로 이동 → 상/하 모서리에 가로 배열
                        ax = t * (float)screenWidth;
                        ay = (pb->triDirY > 0) ? 24.0f : (float)screenHeight - 24.0f;
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
                    if (pb->triDirX != 0.0f) {     // 가로 이동 → 화면 폭을 따라 배치
                        cx = (pb->triDirX > 0) ? u * screenWidth : (1.0f - u) * screenWidth;
                        cy = screenHeight * 0.5f;
                    } else {                       // 세로 이동
                        cx = screenWidth * 0.5f;
                        cy = (pb->triDirY > 0) ? u * screenHeight : (1.0f - u) * screenHeight;
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
                float ex = pb->laserX + pb->laserDirX * (float)(screenWidth + screenHeight);
                float ey = pb->laserY + pb->laserDirY * (float)(screenWidth + screenHeight);
                float pxx = -pb->laserDirY, pyy = pb->laserDirX;
                float th = 3.0f;
                float warnA = 0.4f + 0.4f * (0.5f + 0.5f * sinf((float)glfwGetTime() * 18.0f));
                float p1x=pb->laserX+pxx*th, p1y=pb->laserY+pyy*th;
                float p2x=pb->laserX-pxx*th, p2y=pb->laserY-pyy*th;
                float p3x=ex+pxx*th, p3y=ey+pyy*th, p4x=ex-pxx*th, p4y=ey-pyy*th;
                float v[12]={p1x,p1y,p2x,p2y,p3x,p3y, p2x,p2y,p4x,p4y,p3x,p3y};
                glUniform4f(g_colorLoc, 1.0f, 0.3f, 1.0f, warnA);
                glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
            // 레이저 (RHOMBUS) — 어두운 시야 밴드 + 밝은 코어
            if (pb->laserActive) {
                float ex = pb->laserX + pb->laserDirX * (float)(screenWidth + screenHeight);
                float ey = pb->laserY + pb->laserDirY * (float)(screenWidth + screenHeight);
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
                    glUniform4f(g_colorLoc, cr, cg, cb, ca);
                    glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }
            }
            // 본체 + 차크람 + HP + 총알 — 개인 창 영역으로 클리핑 (맨 배경에 떠 보이지 않게)
            glEnable(GL_SCISSOR_TEST);
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
            // HP 바
            float hpFrac = pb->hp / pb->maxHp; if (hpFrac<0)hpFrac=0; if(hpFrac>1)hpFrac=1;
            float hbW = 200.0f, hbH = 10.0f;
            float hbX = pb->worldX - hbW * 0.5f, hbY = pb->worldY - bsz - 22.0f;
            drawRect(hbX, hbY, hbW, hbH, 0.15f, 0.1f, 0.2f, 0.85f);
            drawRect(hbX, hbY, hbW * hpFrac, hbH, 0.7f, 0.3f, 1.0f, 0.95f);
            // 폼 변환 파티클 — 보스 개인 창 안에서도 보이도록 (창 밖 데스크톱엔 안 뜸)
            for (auto& p : g_EnemyParts) {
                if (!p.active) continue;
                float a = p.life / p.maxLife, hs = p.size * 0.5f;
                drawRect(p.x - hs, p.y - hs, p.size, p.size, p.r, p.g, p.b, a);
            }
            glDisable(GL_SCISSOR_TEST);
        }

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
                drawCircle(chx, chy, CHAKRAM_SIZE * 0.6f, 1.0f, 0.85f, 0.2f, 0.25f);
                float hs = CHAKRAM_SIZE * 0.5f;
                drawRect(chx - hs, chy - 3, CHAKRAM_SIZE, 6, 1.0f, 0.7f, 0.0f, 1.0f);
                drawRect(chx - 3, chy - hs, 6, CHAKRAM_SIZE, 1.0f, 0.7f, 0.0f, 1.0f);
                // HP 바
                float hpFrac = ch.hp / ch.maxHp;
                if (hpFrac < 0) hpFrac = 0; if (hpFrac > 1) hpFrac = 1;
                drawRect(chx - 14, chy - CHAKRAM_SIZE - 6, 28, 3, 0.2f, 0.2f, 0.2f, 0.7f);
                drawRect(chx - 14, chy - CHAKRAM_SIZE - 6, 28 * hpFrac, 3,
                         1.0f, 0.7f, 0.0f, 0.95f);
            }
        }

        // ── 여기부터 UI/오버레이: 줌·흔들기 무시하고 화면 고정 좌표(base ortho)로 ──
        //    (폴리모프 2페이즈 줌 0.5 에서 쿨다운칸·메뉴딤·비네트·플래시가 찌그러지던 버그 fix)
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, ortho);
        memcpy(g_MainOrtho, ortho, sizeof(ortho));

        // (h2) 보스 고유색 화면 물들이기 — 보스 생존 중 서서히 차오르고, 처치 후 사라짐
        {
            bool bossAlive = false;
            glm::vec3 tc(0.6f, 0.3f, 1.0f);
            if ((g_MonsterManager.boss && g_MonsterManager.boss->alive) ||
                !g_Slimelings.empty()) {
                bossAlive = true; tc = glm::vec3(0.55f, 0.55f, 0.95f); }   // 슬라임/분열체: 연보라
            else if (g_GlitchBoss && g_GlitchBoss->alive) {
                bossAlive = true; tc = glm::vec3(0.95f, 0.2f, 0.6f); }     // 글리치: 마젠타
            else if (g_RRBoss && g_RRBoss->alive) {
                bossAlive = true; tc = glm::vec3(1.0f, 0.55f, 0.2f); }     // 리로드러너: 주황
            else if (g_PolyBoss && g_PolyBoss->alive) {
                bossAlive = true; tc = glm::vec3(0.6f, 0.25f, 1.0f); }     // 폴리모프: 보라

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

        // (h3) 화면 플래시 — 레벨업/보스처치/부활 순간 번쩍
        if (g_FlashIntensity > 0.001f) {
            BindMainShader();
            float a = g_FlashIntensity; if (a > 0.85f) a = 0.85f;
            drawRect(0, 0, (float)screenWidth, (float)screenHeight,
                     g_FlashColor.r, g_FlashColor.g, g_FlashColor.b, a);
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

        // [6] HUD 오버레이 (HP바, 상태 표시)
        g_GameManager.Render();

        // ── [7] 한국어 텍스트 + 메뉴 ─────────────────────────────────────────
        {
            float sw = (float)screenWidth, sh = (float)screenHeight;
            auto  st = g_GameManager.currentState;

            // 중앙 X 정렬 헬퍼
            auto cx = [&](const wchar_t* t, TextRenderer& tr, float scale) -> float {
                return (sw - tr.Width(t, scale)) * 0.5f;
            };

            // ── 업적 해금 토스트 (상단 중앙 배너, 4초 표시 후 페이드) ──
            if (g_AchToastTimer > 0.0f && g_AchToastId >= 0 &&
                g_AchToastId < ACH_COUNT) {
                g_AchToastTimer -= delta;
                float a = g_AchToastTimer > 3.0f ? (4.0f - g_AchToastTimer) // 페이드인
                        : std::min(1.0f, g_AchToastTimer);                 // 페이드아웃
                if (a < 0.0f) a = 0.0f; if (a > 1.0f) a = 1.0f;
                int li2 = (int)g_Language; if (li2 < 0 || li2 >= LANG_COUNT) li2 = 0;
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

            // ── 크리에이티브 HUD (게임 중) — F:증강  G:무적 ──
            if (g_CreativeMode &&
                (st == GameState::RUNNING || st == GameState::READY ||
                 st == GameState::AUG_SELECT || st == GameState::DEBUFF_SELECT)) {
                int li3 = (int)g_Language; if (li3 < 0 || li3 >= LANG_COUNT) li3 = 0;
                const wchar_t* CH[3] = {
                    L"CREATIVE   F: 증강(디버프 포함)   G: 무적",
                    L"CREATIVE   F: Augment(+debuff)   G: Godmode",
                    L"CREATIVE   F: 強化(デバフ含)   G: 無敵" };
                g_TextS.Draw(CH[li3], 20.0f, sh - 36.0f, 0.8f, 0.7f, 0.85f, 1.0f, 0.85f);
                if (g_CreativeGodmode) {
                    const wchar_t* GOD[3] = { L"● 무적 ON", L"● GODMODE ON", L"● 無敵 ON" };
                    float blink = 0.65f + 0.35f * sinf((float)glfwGetTime() * 5.0f);
                    g_TextL.Draw(GOD[li3], 20.0f, 24.0f, 0.95f, 1.0f, 0.85f, 0.2f, blink);
                }
            }

            // ── 메인 메뉴 ─────────────────────────────────────────
            if (st == GameState::MAIN_MENU) {
                BindMainShader();
                drawRect(0, 0, sw, sh, 0.02f, 0.02f, 0.06f, 0.92f);

                // 타이틀
                const wchar_t* TIT = T(StrId::GAME_TITLE);
                g_TextL.Draw(TIT, cx(TIT, g_TextL, 2.4f), sh*0.18f, 2.4f,
                             0.9f, 0.95f, 1.0f, 1.0f);

                // 버튼 4개 (시작 / 상점 / 설정 / 종료)
                const float BW = 320.0f, BH = 64.0f, BG = 16.0f;
                float bx = (sw - BW) * 0.5f;
                float by = sh * 0.38f;

                if (UIButton(bx, by, BW, BH, T(StrId::BTN_START),
                             mx, my, lmb, g_LmbPrev)) {
                    g_GameManager.currentState = GameState::DIFFICULTY_SELECT;
                }
                if (UIButton(bx, by + (BH+BG), BW, BH, T(StrId::BTN_SHOP),
                             mx, my, lmb, g_LmbPrev)) {
                    g_GameManager.currentState = GameState::SHOP;
                }
                {
                    int li2 = (int)g_Language; if (li2 < 0 || li2 >= LANG_COUNT) li2 = 0;
                    const wchar_t* CODEX_LBL[3] = { L"도감", L"Codex", L"図鑑" };
                    if (UIButton(bx, by + (BH+BG)*2, BW, BH, CODEX_LBL[li2],
                                 mx, my, lmb, g_LmbPrev)) {
                        g_GameManager.currentState = GameState::CODEX;
                    }
                }
                if (UIButton(bx, by + (BH+BG)*3, BW, BH, T(StrId::BTN_SETTINGS),
                             mx, my, lmb, g_LmbPrev)) {
                    g_SettingsReturnTo = GameState::MAIN_MENU;
                    g_GameManager.currentState = GameState::SETTINGS;
                }
                if (UIButton(bx, by + (BH+BG)*4, BW, BH, T(StrId::BTN_QUIT),
                             mx, my, lmb, g_LmbPrev)) {
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                }
            }
            // ── 메타 상점 (코인 → 영구 업그레이드) ──────────────────
            else if (st == GameState::SHOP) {
                BindMainShader();
                drawRect(0, 0, sw, sh, 0.02f, 0.02f, 0.06f, 0.94f);
                const wchar_t* TIT = T(StrId::BTN_SHOP);
                g_TextL.Draw(TIT, cx(TIT, g_TextL, 1.6f), sh*0.08f, 1.6f, 1,1,1,1);
                // 보유 코인
                wchar_t cbuf[48]; swprintf_s(cbuf, L"COIN  %lld", g_Coins);
                g_TextL.Draw(cbuf, cx(cbuf, g_TextL, 1.1f), sh*0.16f, 1.1f, 1.0f, 0.9f, 0.3f, 1.0f);

                const float RW = 640.0f, RH = 58.0f, RG = 12.0f;
                float rx = (sw - RW) * 0.5f, ry0 = sh * 0.26f;
                for (int i = 0; i < META_COUNT; i++) {
                    float ry = ry0 + i * (RH + RG);
                    BindMainShader();   // 직전 행의 텍스트 셰이더 뒤 → 사각형 셰이더 복원
                    drawRect(rx, ry, RW, RH, 0.07f, 0.07f, 0.11f, 0.9f);
                    // 이름 + 레벨
                    wchar_t nm[96];
                    swprintf_s(nm, L"%ls   Lv %d/%d", MetaName(i), g_MetaLv[i], META_DEFS[i].maxLv);
                    g_TextS.Draw(nm, rx + 16.0f, ry + 16.0f, 0.95f, 0.9f, 0.95f, 1.0f, 1.0f);
                    // 구매 버튼
                    long long cost = MetaNextCost(i);
                    float btX = rx + RW - 170.0f;
                    if (cost < 0) {
                        g_TextS.Draw(L"MAX", btX + 50.0f, ry + 16.0f, 0.9f, 0.6f, 1.0f, 0.6f, 1.0f);
                    } else {
                        wchar_t bb[32]; swprintf_s(bb, L"%lld", cost);
                        bool can = (g_Coins >= cost);
                        if (can) {
                            if (UIButton(btX, ry + 6.0f, 154.0f, RH - 12.0f, bb,
                                         mx, my, lmb, g_LmbPrev, false)) {
                                g_Coins -= cost;
                                g_MetaLv[i]++;
                                SaveGame();
                            }
                        } else {
                            // 코인 부족 — 빨간 비활성 박스 (클릭 불가)
                            BindMainShader();
                            drawRect(btX, ry + 6.0f, 154.0f, RH - 12.0f, 0.18f, 0.06f, 0.06f, 0.9f);
                            float tw = g_TextS.Width(bb, 0.9f);
                            g_TextS.Draw(bb, btX + (154.0f - tw) * 0.5f, ry + 18.0f, 0.9f,
                                         0.95f, 0.4f, 0.4f, 1.0f);
                        }
                    }
                }

                // ── 업적 목록 (메타 행 아래, 2열) ──
                {
                    int li3 = (int)g_Language; if (li3 < 0 || li3 >= LANG_COUNT) li3 = 0;
                    const wchar_t* ATIT[3] = { L"업적", L"Achievements", L"実績" };
                    float ay0 = ry0 + META_COUNT * (RH + RG) + 24.0f;
                    BindMainShader();
                    g_TextS.Draw(ATIT[li3], rx, ay0, 1.0f, 0.8f, 0.9f, 1.0f, 1.0f);
                    float colW = RW / 3.0f;
                    float rowH = 30.0f;
                    for (int i = 0; i < ACH_COUNT; i++) {
                        bool got = g_AchUnlocked[i];
                        int  col = i / 4, row = i % 4;
                        float ax = rx + col * colW;
                        float ay = ay0 + 34.0f + row * rowH;
                        wchar_t ab[96];
                        swprintf_s(ab, L"%ls %ls", got ? L"★" : L"☆", AchName(i));
                        if (got) g_TextS.Draw(ab, ax, ay, 0.78f, 0.5f, 0.95f, 0.55f, 1.0f);
                        else     g_TextS.Draw(ab, ax, ay, 0.78f, 0.55f, 0.55f, 0.6f, 0.9f);
                    }
                }

                if (UIButton(40.0f, sh - 80.0f, 180.0f, 56.0f, T(StrId::BTN_BACK),
                             mx, my, lmb, g_LmbPrev)) {
                    g_GameManager.currentState = GameState::MAIN_MENU;
                }
            }
            // ── 도감 (적 / 증강) ──────────────────────────────────
            else if (st == GameState::CODEX) {
                BindMainShader();
                drawRect(0, 0, sw, sh, 0.02f, 0.02f, 0.06f, 0.96f);
                int li = (int)g_Language; if (li < 0 || li >= LANG_COUNT) li = 0;
                const wchar_t* CTIT[3] = { L"도감", L"Codex", L"図鑑" };
                g_TextL.Draw(CTIT[li], cx(CTIT[li], g_TextL, 1.5f), sh*0.06f, 1.5f, 1,1,1,1);

                static int s_tab = 0;   // 0 적 / 1 증강
                const wchar_t* TAB_MOB[3] = { L"적", L"Enemies", L"敵" };
                const wchar_t* TAB_AUG[3] = { L"증강", L"Augments", L"強化" };
                if (UIButton(sw*0.5f - 220.0f, sh*0.12f, 200.0f, 48.0f, TAB_MOB[li],
                             mx, my, lmb, g_LmbPrev, s_tab == 0)) s_tab = 0;
                if (UIButton(sw*0.5f + 20.0f,  sh*0.12f, 200.0f, 48.0f, TAB_AUG[li],
                             mx, my, lmb, g_LmbPrev, s_tab == 1)) s_tab = 1;

                int hover = -1;
                if (s_tab == 0) {
                    // 적 그리드
                    const int COLS = 6; const float CELL = 130.0f;
                    float gx = (sw - COLS*CELL) * 0.5f, gy = sh * 0.24f;
                    for (int i = 0; i < CM_COUNT; i++) {
                        float cxp = gx + (i % COLS) * CELL, cyp = gy + (i / COLS) * CELL;
                        float cw = CELL - 12.0f;
                        bool seen = g_MobSeen[i];
                        bool hv = (mx >= cxp && mx < cxp+cw && my >= cyp && my < cyp+cw);
                        if (hv) hover = i;
                        BindMainShader();
                        drawRect(cxp, cyp, cw, cw, hv?0.13f:0.06f, 0.10f, 0.15f, 0.95f);
                        float ccx = cxp + cw*0.5f, ccy = cyp + cw*0.42f;
                        if (seen) {
                            if (i <= CM_SHIELDED) {
                                Monster pm(ccx, ccy);
                                if (i > 0) pm.MakeKind((MobKind)i);
                                pm.worldX = ccx; pm.worldY = ccy;
                                if (i == 0) pm.color = glm::vec3(0.75f,0.75f,0.8f);
                                pm.sizeScale = (i == (int)MobKind::BRUTE) ? 1.3f : 1.7f;
                                drawMob(&pm);
                            } else if (i == CM_RANGED) {
                                drawDiamond(ccx, ccy, 28.0f, 0.85f, 0.0f, 0.85f, 1.0f);
                                drawDiamond(ccx, ccy, 11.0f, 1,1,1, 0.9f);
                            } else { // BOMBER
                                drawPentagon(ccx, ccy, 32.0f, 1.0f, 0.5f, 0.1f, 1.0f);
                            }
                            BindMainShader();
                            const wchar_t* nm = MobName(i);
                            float nw = g_TextS.Width(nm, 0.8f);
                            g_TextS.Draw(nm, cxp + (cw - nw)*0.5f, cyp + cw - 34.0f, 0.8f,
                                         0.9f, 0.95f, 1.0f, 0.95f);
                        } else {
                            g_TextL.Draw(L"?", ccx - 9.0f, ccy - 12.0f, 1.3f,
                                         0.4f, 0.4f, 0.45f, 0.9f);
                        }
                    }
                    if (hover >= 0 && g_MobSeen[hover]) {
                        const wchar_t* d = MobDesc(hover);
                        g_TextS.Draw(d, cx(d, g_TextS, 1.0f), sh - 120.0f, 1.0f,
                                     0.85f, 0.95f, 1.0f, 0.95f);
                    }
                } else {
                    // 증강 그리드
                    const int COLS = 12; const float CELL = 80.0f;
                    float gx = (sw - COLS*CELL) * 0.5f, gy = sh * 0.22f;
                    for (int i = 0; i < AUG_TOTAL; i++) {
                        float cxp = gx + (i % COLS) * CELL, cyp = gy + (i / COLS) * CELL;
                        float cw = CELL - 8.0f;
                        bool seen = g_AugSeen[i];
                        bool hv = (mx >= cxp && mx < cxp+cw && my >= cyp && my < cyp+cw);
                        if (hv) hover = i;
                        float rr, rg, rb; GetRarityColor(ALL_AUGS[i].rarity, rr, rg, rb);
                        BindMainShader();
                        if (seen) drawRect(cxp, cyp, cw, cw, rr*0.35f, rg*0.35f, rb*0.35f, 0.95f);
                        else      drawRect(cxp, cyp, cw, cw, 0.06f, 0.06f, 0.08f, 0.95f);
                        if (seen) {
                            GLuint ic = IconFor(ALL_AUGS[i].type);
                            if (ic) DrawIcon(ic, cxp + (cw-46.0f)*0.5f, cyp + 5.0f, 46.0f, 46.0f,
                                             1,1,1, 0.97f);
                            else {
                                BindMainShader();
                                drawRect(cxp + cw*0.3f, cyp + cw*0.3f, cw*0.4f, cw*0.4f, rr, rg, rb, 0.9f);
                            }
                        } else {
                            g_TextL.Draw(L"?", cxp + cw*0.5f - 7.0f, cyp + cw*0.5f - 14.0f, 1.0f,
                                         0.4f, 0.4f, 0.45f, 0.9f);
                        }
                    }
                    // 상세 패널
                    BindMainShader();
                    float py = sh - 160.0f;
                    if (hover >= 0 && g_AugSeen[hover]) {
                        const AugDef& d = ALL_AUGS[hover];
                        float rr, rg, rb; GetRarityColor(d.rarity, rr, rg, rb);
                        wchar_t hd[96];
                        swprintf_s(hd, L"[%ls] %ls", GetRarityKR(d.rarity), AugName(d));
                        g_TextL.Draw(hd, cx(hd, g_TextL, 0.9f), py, 0.9f,
                                     std::min(1.0f, rr*1.4f+0.3f), std::min(1.0f, rg*1.4f+0.3f),
                                     std::min(1.0f, rb*1.4f+0.3f), 1.0f);
                        const wchar_t* ds = AugDesc(d);
                        g_TextS.Draw(ds, cx(ds, g_TextS, 0.85f), py + 42.0f, 0.85f,
                                     0.85f, 0.92f, 1.0f, 0.95f);
                        if (d.rarity == AugRarity::COMBO) {  // 발견했으니 레시피 공개
                            for (int c = 0; c < COMBO_COUNT; c++)
                                if (COMBO_DEFS[c].result == d.type) {
                                    int ia = AugIndexOfType(COMBO_DEFS[c].reqs[0]);
                                    int ib = AugIndexOfType(COMBO_DEFS[c].reqs[1]);
                                    wchar_t rc[128];
                                    swprintf_s(rc, L"%ls + %ls",
                                               ia>=0 ? AugName(ALL_AUGS[ia]) : L"?",
                                               ib>=0 ? AugName(ALL_AUGS[ib]) : L"?");
                                    g_TextS.Draw(rc, cx(rc, g_TextS, 0.85f), py + 74.0f, 0.85f,
                                                 0.1f, 0.85f, 0.8f, 0.95f);
                                    break;
                                }
                        }
                    } else if (hover >= 0) {
                        const wchar_t* q[3] = { L"??? — 미발견 (획득 시 공개)",
                                                L"??? — Undiscovered (unlock by acquiring)",
                                                L"??? — 未発見 (取得で公開)" };
                        g_TextL.Draw(q[li], cx(q[li], g_TextL, 0.9f), py, 0.9f,
                                     0.5f, 0.5f, 0.55f, 0.9f);
                    }
                }

                if (UIButton(40.0f, sh - 80.0f, 180.0f, 56.0f, T(StrId::BTN_BACK),
                             mx, my, lmb, g_LmbPrev)) {
                    g_GameManager.currentState = GameState::MAIN_MENU;
                }
            }
            // ── 직업(클래스) 선택 ─────────────────────────────────
            else if (st == GameState::JOB_SELECT) {
                BindMainShader();
                drawRect(0, 0, sw, sh, 0.02f, 0.02f, 0.06f, 0.94f);

                int li = (int)g_Language; if (li < 0 || li >= LANG_COUNT) li = 0;
                const wchar_t* JTIT[3] = { L"직업 선택", L"Choose a Class", L"職業を選択" };
                const wchar_t* JHINT[3] = {
                    L"업적을 달성하면 새 직업이 해금됩니다",
                    L"Complete achievements to unlock more classes",
                    L"実績達成で新しい職業が解放されます" };
                const wchar_t* LOCKED[3] = { L"잠김 — ", L"Locked — ", L"未解放 — " };
                const wchar_t* TIT = JTIT[li];
                g_TextL.Draw(TIT, cx(TIT, g_TextL, 1.5f), sh*0.10f, 1.5f, 1,1,1,1);
                const wchar_t* HN = JHINT[li];
                g_TextS.Draw(HN, cx(HN, g_TextS, 0.9f), sh*0.16f, 0.9f, 0.7f,0.8f,0.9f,0.9f);

                const float BW = 600.0f, BH = 60.0f, BG = 11.0f;
                float bx = (sw - BW) * 0.5f;
                float by = sh * 0.20f;
                for (int j = 0; j < JOB_COUNT; j++) {
                    float y = by + j * (BH + BG);
                    bool unlocked = JobUnlocked(j);
                    bool sel = (g_SelectedJob == j);
                    if (unlocked) {
                        if (UIButton(bx, y, BW, BH, JobName(j),
                                     mx, my, lmb, g_LmbPrev, sel)) {
                            g_SelectedJob = j;
                            ResetForNewGame();
                            PickRandomWeapons(g_WeaponChoices);
                            g_GameManager.currentState = GameState::WEAPON_SELECT;
                        }
                        BindMainShader();
                        const wchar_t* d = JobDesc(j);
                        float dw = g_TextS.Width(d, 0.8f);
                        g_TextS.Draw(d, bx + (BW - dw) * 0.5f, y + BH - 24.0f, 0.8f,
                                     0.85f, 0.95f, 1.0f, 0.9f);
                        // 직업 아이콘 (좌측, 흰색)
                        GLuint ji = JobIcon(j);
                        if (ji) {
                            float isz = BH - 14.0f;
                            DrawIcon(ji, bx + 10.0f, y + (BH - isz) * 0.5f, isz, isz,
                                     1.0f, 1.0f, 1.0f, 0.97f);
                        }
                    } else {
                        // 잠긴 직업 — 비활성 박스 + 해금 조건
                        BindMainShader();
                        drawRect(bx, y, BW, BH, 0.08f, 0.06f, 0.06f, 0.9f);
                        GLuint ji = JobIcon(j);
                        if (ji) {
                            float isz = BH - 14.0f;
                            DrawIcon(ji, bx + 10.0f, y + (BH - isz) * 0.5f, isz, isz,
                                     0.45f, 0.45f, 0.5f, 0.9f);  // 잠김 = 회색
                        }
                        float nw = g_TextS.Width(JobName(j), 0.95f);
                        g_TextS.Draw(JobName(j), bx + (BW - nw) * 0.5f, y + 14.0f, 0.95f,
                                     0.5f, 0.5f, 0.55f, 0.95f);
                        int a = JOB_DEFS[j].unlockAch;
                        wchar_t lk[160];
                        swprintf_s(lk, L"%ls%ls", LOCKED[li],
                                   (a >= 0 && a < ACH_COUNT) ? AchName(a) : L"???");
                        float lw = g_TextS.Width(lk, 0.78f);
                        g_TextS.Draw(lk, bx + (BW - lw) * 0.5f, y + BH - 24.0f, 0.78f,
                                     0.85f, 0.45f, 0.45f, 0.95f);
                    }
                }
                if (UIButton(40.0f, sh - 80.0f, 180.0f, 56.0f, T(StrId::BTN_BACK),
                             mx, my, lmb, g_LmbPrev)) {
                    g_GameManager.currentState = GameState::DIFFICULTY_SELECT;
                }
            }
            // ── 무기 선택 (시작 시 랜덤 3개) ──────────────────────
            else if (st == GameState::WEAPON_SELECT) {
                BindMainShader();
                drawRect(0, 0, sw, sh, 0.02f, 0.02f, 0.06f, 0.92f);

                const wchar_t* TIT = L"시작 무기를 선택하세요";
                g_TextL.Draw(TIT, cx(TIT, g_TextL, 1.4f), sh*0.14f, 1.4f,
                             1, 1, 1, 0.98f);

                // 3 카드 — 가로 배치 (DIFFICULTY 와 비슷)
                const float CARD_W = 320.0f, CARD_H = 260.0f, GAP = 32.0f;
                const float TOTAL_W = 3*CARD_W + 2*GAP;
                float baseX = (sw - TOTAL_W) * 0.5f;
                float baseY = sh * 0.30f;

                for (int i = 0; i < 3; i++) {
                    int idx = g_WeaponChoices[i];
                    if (idx < 0 || idx >= (int)StartWeapon::_COUNT) continue;
                    const WeaponDef& w = ALL_WEAPONS[idx];
                    float cardX = baseX + i * (CARD_W + GAP);

                    // 카드 = 큰 버튼
                    if (UIButton(cardX, baseY, CARD_W, CARD_H, WeaponName(w),
                                 mx, my, lmb, g_LmbPrev)) {
                        // #109: 변환 카드 undo 기준점 저장 (무기 적용 전 fireInterval)
                        g_Stats.baseFireInterval = g_Stats.fireInterval;
                        ApplyWeapon(g_Stats, (StartWeapon)idx);
                        g_CurrentWeapon = idx;  // 현재 무기 기록 (변환 카드용)
                        // 발사 타이머 / HUD HP 갱신
                        fireTimer = g_Stats.fireInterval;
                        // 직업 시작 증강 적용 (무기 적용 직후 — 보유 목록에 추가)
                        if (g_SelectedJob > 0 && g_SelectedJob < JOB_COUNT) {
                            const JobDef& jd = JOB_DEFS[g_SelectedJob];
                            for (int a = 0; a < jd.startAugCount; a++) {
                                int ji = AugIndexOf(jd.startAugs[a]);
                                if (ji < 0) continue;
                                g_Stats.Apply(jd.startAugs[a]);
                                g_OwnedAugs.push_back(ji);
                                EquipSkill(SkillForAug(jd.startAugs[a]));
                            }
                            // 직업 무기 모드 (검객/궁수 — 선택한 총 효과 위에 덮어씀)
                            if (jd.weaponMode == 1) {           // 검객: 근접 호 스윙
                                g_Stats.meleeWeapon  = true;
                                g_Stats.fireInterval = 0.26f;   // 스윙 주기 (고정)
                                g_Stats.baseFireInterval = g_Stats.fireInterval;
                            } else if (jd.weaponMode == 2) {    // 궁수: 관통 화살
                                g_Stats.bowWeapon     = true;
                                g_Stats.pierce        = true;
                                g_Stats.pierceChance  = 100;    // 전탄 관통
                                g_Stats.fireInterval *= 1.9f;   // 느린 연사
                                g_Stats.baseFireInterval = g_Stats.fireInterval;
                                g_Stats.damageMultiplier *= 2.2f; // 강한 화살
                                g_Stats.bulletSpeed      *= 1.3f;
                            }
                            fireTimer = g_Stats.fireInterval;
                        }
                        g_GameManager.maxHP    = g_Stats.maxHP;
                        g_GameManager.playerHP = g_Stats.maxHP;
                        g_PrevHP               = g_Stats.maxHP;
                        // 시작 증강 픽 (메타 해금 + 크리에이티브) 있으면 증강 선택 열기
                        int startAugs = g_MetaStartAugs +
                                        ((g_CreativeMode) ? g_CreativeStartAugs : 0);
                        if (startAugs > 0) {
                            g_BossRewardPicksLeft = startAugs;
                            // 크리에이티브: 시작 증강에도 디버프 포함 (샌드박스)
                            g_GameManager.PickAugChoices(g_Stats.sizeAugTaken,
                                                         g_Stats.distAugTaken, g_CreativeMode);
                            g_GameManager.currentState = GameState::AUG_SELECT;
                        } else {
                            g_GameManager.currentState = GameState::READY;
                        }
                    }

                    // 설명 — 카드 안 하단에 그림 (UIButton 위에 덧그림)
                    BindMainShader();
                    float dY = baseY + CARD_H * 0.55f;
                    const wchar_t* d = WeaponDesc(w);
                    // '/' 로 split → 줄 단위
                    std::vector<std::wstring> lines;
                    std::wstring cur;
                    for (const wchar_t* p = d; *p; ++p) {
                        if (*p == L'/') { if (!cur.empty()) lines.push_back(cur); cur.clear(); }
                        else cur += *p;
                    }
                    if (!cur.empty()) lines.push_back(cur);
                    for (auto& s : lines) {
                        while (!s.empty() && s.front() == L' ') s.erase(0, 1);
                        while (!s.empty() && s.back() == L' ') s.pop_back();
                    }
                    float lineH = 26.0f;
                    for (int li = 0; li < (int)lines.size(); li++) {
                        const wchar_t* s = lines[li].c_str();
                        float sc = 0.85f;
                        while (sc > 0.55f &&
                               g_TextS.Width(s, sc) > CARD_W - 24.0f) sc -= 0.05f;
                        float lw = g_TextS.Width(s, sc);
                        g_TextS.Draw(s, cardX + (CARD_W - lw) * 0.5f,
                                     dY + li * lineH, sc, 0.88f, 0.95f, 1.0f, 0.95f);
                    }
                }
            }
            // ── 난이도 선택 ───────────────────────────────────────
            else if (st == GameState::DIFFICULTY_SELECT) {
                BindMainShader();
                drawRect(0, 0, sw, sh, 0.02f, 0.02f, 0.06f, 0.92f);

                const wchar_t* TIT = T(StrId::DIFF_TITLE);
                g_TextL.Draw(TIT, cx(TIT, g_TextL, 1.6f), sh*0.20f, 1.6f,
                             1.0f, 1.0f, 1.0f, 1.0f);

                struct DiffBtn { Difficulty d; StrId label; StrId desc; float r, g, b; };
                DiffBtn btns[3] = {
                    { Difficulty::EASY,   StrId::DIFF_EASY,   StrId::DIFF_EASY_DESC,
                      0.3f, 0.85f, 0.4f },
                    { Difficulty::NORMAL, StrId::DIFF_NORMAL, StrId::DIFF_NORMAL_DESC,
                      0.4f, 0.6f, 1.0f },
                    { Difficulty::HARD,   StrId::DIFF_HARD,   StrId::DIFF_HARD_DESC,
                      1.0f, 0.4f, 0.4f },
                };

                const float BW = 520.0f, BH = 90.0f, BG = 30.0f;
                float totalH = 3 * BH + 2 * BG;
                float bx = (sw - BW) * 0.5f;
                float by = (sh - totalH) * 0.5f;

                for (int i = 0; i < 3; i++) {
                    float y = by + i * (BH + BG);
                    bool sel = (g_Difficulty == btns[i].d);
                    if (UIButton(bx, y, BW, BH, T(btns[i].label),
                                 mx, my, lmb, g_LmbPrev, sel)) {
                        g_Difficulty = btns[i].d;
                        if (g_CreativeMode) {
                            // 크리에이티브: 설정 화면으로 (시작/보스/증강 조정)
                            g_GameManager.currentState = GameState::CREATIVE_CONFIG;
                        } else {
                            // 직업 선택 화면으로 (해금된 직업 선택 후 무기 선택)
                            g_GameManager.currentState = GameState::JOB_SELECT;
                        }
                    }
                    // 버튼 아래 설명
                    const wchar_t* desc = T(btns[i].desc);
                    float dw = g_TextS.Width(desc, 0.85f);
                    g_TextS.Draw(desc, bx + (BW - dw) * 0.5f, y + BH - 28.0f, 0.85f,
                                 btns[i].r, btns[i].g, btns[i].b, 0.85f);
                }

                // 크리에이티브 모드 토글 버튼
                {
                    const wchar_t* CLBL = g_CreativeMode
                        ? T(StrId::CREATIVE_ON)
                        : T(StrId::CREATIVE_OFF);
                    float cby = by + 3 * (BH + BG) + 20.0f;
                    float cbw = BW, cbh = 72.0f;     // 세로 키움 (라벨/설명 겹침 방지)
                    float cbx = (sw - cbw) * 0.5f;
                    if (UIButton(cbx, cby, cbw, cbh, CLBL,
                                 mx, my, lmb, g_LmbPrev, g_CreativeMode)) {
                        g_CreativeMode = !g_CreativeMode;
                    }
                    // 설명 — 버튼 아래쪽에 (버튼 안과 겹치지 않게)
                    const wchar_t* CDESC = g_CreativeMode
                        ? T(StrId::CREATIVE_DESC_ON)
                        : T(StrId::CREATIVE_DESC_OFF);
                    float cdw = g_TextS.Width(CDESC, 0.78f);
                    g_TextS.Draw(CDESC, cbx + (cbw - cdw) * 0.5f,
                                 cby + cbh + 10.0f, 0.78f,
                                 0.85f, 0.95f, 0.6f, 0.85f);
                }

                // 뒤로 버튼
                if (UIButton(40.0f, sh - 80.0f, 180.0f, 56.0f, T(StrId::BTN_BACK),
                             mx, my, lmb, g_LmbPrev)) {
                    g_GameManager.currentState = GameState::MAIN_MENU;
                }
            }
            // ── 크리에이티브 설정 (시작점수 / 보스 / 시작증강) ──────
            else if (st == GameState::CREATIVE_CONFIG) {
                BindMainShader();
                drawRect(0, 0, sw, sh, 0.02f, 0.02f, 0.06f, 0.92f);

                const wchar_t* TIT = L"CREATIVE";
                g_TextL.Draw(TIT, cx(TIT, g_TextL, 1.6f), sh*0.08f, 1.6f,
                             0.85f, 0.95f, 0.6f, 1.0f);

                const float OBW = 150.0f, OBH = 50.0f, OBG = 14.0f;

                // 시작 점수
                g_TextS.Draw(L"Start Score", 60.0f, sh*0.22f, 1.0f, 1,1,1,0.9f);
                struct ScoreOpt { const wchar_t* l; long long v; };
                ScoreOpt sOpts[4] = { {L"0",0},{L"200k",200000},{L"400k",400000},{L"500k",500000} };
                for (int i = 0; i < 4; i++) {
                    float ox = 60.0f + i * (OBW + OBG);
                    bool sel = (g_CreativeStartScore == sOpts[i].v);
                    if (UIButton(ox, sh*0.22f + 28.0f, OBW, OBH, sOpts[i].l,
                                 mx, my, lmb, g_LmbPrev, sel))
                        g_CreativeStartScore = sOpts[i].v;
                }

                // 보스 선택
                g_TextS.Draw(L"Boss", 60.0f, sh*0.40f, 1.0f, 1,1,1,0.9f);
                struct BossOpt { const wchar_t* l; int v; };
                BossOpt bOpts[5] = { {L"None",-1},{L"Slime",0},{L"Glitch",1},
                                     {L"Reload",2},{L"Polymorph",3} };
                for (int i = 0; i < 5; i++) {
                    float ox = 60.0f + i * (OBW + OBG);
                    bool sel = (g_CreativeBossPick == bOpts[i].v);
                    if (UIButton(ox, sh*0.40f + 28.0f, OBW, OBH, bOpts[i].l,
                                 mx, my, lmb, g_LmbPrev, sel))
                        g_CreativeBossPick = bOpts[i].v;
                }

                // 시작 증강 픽 횟수
                g_TextS.Draw(L"Start Augments", 60.0f, sh*0.58f, 1.0f, 1,1,1,0.9f);
                int aOpts[4] = { 0, 3, 5, 10 };
                for (int i = 0; i < 4; i++) {
                    float ox = 60.0f + i * (OBW + OBG);
                    wchar_t lb[8]; swprintf_s(lb, L"%d", aOpts[i]);
                    bool sel = (g_CreativeStartAugs == aOpts[i]);
                    if (UIButton(ox, sh*0.58f + 28.0f, OBW, OBH, lb,
                                 mx, my, lmb, g_LmbPrev, sel))
                        g_CreativeStartAugs = aOpts[i];
                }

                // 시작 버튼
                if (UIButton((sw - 300.0f) * 0.5f, sh*0.78f, 300.0f, 64.0f,
                             L"START", mx, my, lmb, g_LmbPrev)) {
                    ResetForNewGame();
                    PickRandomWeapons(g_WeaponChoices);
                    g_GameManager.currentState = GameState::WEAPON_SELECT;
                }

                // 뒤로 버튼 — 난이도 선택으로
                if (UIButton(40.0f, sh - 80.0f, 180.0f, 56.0f, T(StrId::BTN_BACK),
                             mx, my, lmb, g_LmbPrev)) {
                    g_GameManager.currentState = GameState::DIFFICULTY_SELECT;
                }
            }
            // ── 설정 ──────────────────────────────────────────────
            else if (st == GameState::SETTINGS) {
                BindMainShader();
                drawRect(0, 0, sw, sh, 0.02f, 0.02f, 0.06f, 0.92f);

                const wchar_t* TIT = T(StrId::SET_TITLE);
                g_TextL.Draw(TIT, cx(TIT, g_TextL, 1.6f), sh*0.12f, 1.6f,
                             1.0f, 1.0f, 1.0f, 1.0f);

                // FPS 라인
                float lineY = sh * 0.28f;
                g_TextS.Draw(T(StrId::SET_FPS), 60.0f, lineY, 1.0f, 1,1,1,0.9f);
                struct FpsOpt { const wchar_t* label; int val; };
                FpsOpt fpsOpts[4] = {
                    { L"30", 30 }, { L"60", 60 }, { L"144", 144 },
                    { T(StrId::SET_UNLIMITED), -1 }
                };
                for (int i = 0; i < 4; i++) {
                    float bx = 300.0f + i * 200.0f;
                    bool sel = (g_FpsCap == fpsOpts[i].val);
                    if (UIButton(bx, lineY - 14.0f, 180.0f, 54.0f, fpsOpts[i].label,
                                 mx, my, lmb, g_LmbPrev, sel)) {
                        g_FpsCap = fpsOpts[i].val;
                        glfwSwapInterval((g_FpsCap == 0) ? 1 : 0);
                    }
                }

                // 언어 라인
                lineY = sh * 0.42f;
                g_TextS.Draw(T(StrId::SET_LANG), 60.0f, lineY, 1.0f, 1,1,1,0.9f);
                struct LangOpt { const wchar_t* label; Language lang; };
                LangOpt langOpts[LANG_COUNT] = {
                    { L"한국어",  Language::KR },
                    { L"English", Language::EN },
                    { L"日本語",  Language::JP },
                };
                for (int i = 0; i < LANG_COUNT; i++) {
                    float bx = 300.0f + (i % 4) * 200.0f;
                    float by = lineY - 14.0f + (i / 4) * 70.0f;
                    bool sel = (g_Language == langOpts[i].lang);
                    if (UIButton(bx, by, 180.0f, 54.0f, langOpts[i].label,
                                 mx, my, lmb, g_LmbPrev, sel)) {
                        // 폰트는 한/일/영 항상 동시 로드라 언어만 바꾸면 됨 (재로드 불필요)
                        g_Language = langOpts[i].lang;
                    }
                }

                // ON/OFF 토글 한 줄 그리는 헬퍼 (참조로 bool 토글)
                auto toggleRow = [&](float ly, StrId label, bool& val) {
                    g_TextS.Draw(T(label), 60.0f, ly, 1.0f, 1,1,1,0.9f);
                    if (UIButton(420.0f, ly - 14.0f, 160.0f, 54.0f,
                                 T(StrId::OPT_ON), mx, my, lmb, g_LmbPrev, val))
                        val = true;
                    if (UIButton(600.0f, ly - 14.0f, 160.0f, 54.0f,
                                 T(StrId::OPT_OFF), mx, my, lmb, g_LmbPrev, !val))
                        val = false;
                };

                // 크로스헤어 / 데미지 숫자 / 콤보 토글
                toggleRow(sh * 0.56f, StrId::SET_CROSSHAIR, g_ShowCrosshair);
                toggleRow(sh * 0.66f, StrId::SET_DMGNUM,    g_ShowDamageNumbers);
                toggleRow(sh * 0.76f, StrId::SET_COMBO,     g_ShowCombo);

                // 뒤로 버튼 — 진입 시점의 상태로 복귀 (MAIN_MENU 또는 PAUSED)
                if (UIButton(40.0f, sh - 80.0f, 180.0f, 56.0f, T(StrId::BTN_BACK),
                             mx, my, lmb, g_LmbPrev)) {
                    SaveGame();   // 설정(언어/FPS/토글) 영구 저장
                    g_GameManager.currentState = g_SettingsReturnTo;
                }
            }
            else if (st == GameState::READY) {
                const wchar_t* T1 = T(StrId::PRESS_SPACE_TO_START);
                const wchar_t* T2 = T(StrId::ESC_QUIT);
                g_TextL.Draw(T1, cx(T1, g_TextL, 1.0f), sh*0.42f, 1.0f, 1,1,1,0.95f);
                g_TextS.Draw(T2, cx(T2, g_TextS, 1.0f), sh*0.50f, 1.0f, 0.8f,0.8f,0.8f,0.8f);
                // 조작 안내 (키는 언어 무관 — 새 플레이어가 스킬/대시 존재를 알게)
                const wchar_t* CTRL =
                    L"WASD Move    Mouse Fire    SHIFT Dash    Q/E/R Skills    ESC Pause";
                g_TextS.Draw(CTRL, cx(CTRL, g_TextS, 0.9f), sh*0.62f, 0.9f,
                             0.55f, 0.85f, 1.0f, 0.95f);
            }
            else if (st == GameState::PAUSED) {
                // 전체 화면 딤 — 보스 창/엔티티가 메뉴 뒤로 비치지 않게
                BindMainShader();
                drawRect(0, 0, sw, sh, 0.02f, 0.02f, 0.06f, 0.86f);
                const wchar_t* T1 = T(StrId::PAUSED);
                g_TextL.Draw(T1, cx(T1, g_TextL, 1.4f), sh*0.22f, 1.4f, 1,1,1,0.95f);

                // 버튼 4개: 재개 / 설정 / 메뉴로 / 종료
                const float BW = 280.0f, BH = 64.0f, BG = 16.0f;
                float bx = (sw - BW) * 0.5f;
                float by = sh * 0.36f;

                if (UIButton(bx, by + 0*(BH+BG), BW, BH, T(StrId::BTN_RESUME),
                             mx, my, lmb, g_LmbPrev)) {
                    g_GameManager.currentState = GameState::RUNNING;
                }
                if (UIButton(bx, by + 1*(BH+BG), BW, BH, T(StrId::BTN_SETTINGS),
                             mx, my, lmb, g_LmbPrev)) {
                    g_SettingsReturnTo = GameState::PAUSED;
                    g_GameManager.currentState = GameState::SETTINGS;
                }
                if (UIButton(bx, by + 2*(BH+BG), BW, BH, T(StrId::BTN_MAIN_MENU),
                             mx, my, lmb, g_LmbPrev)) {
                    g_GameManager.currentState = GameState::MAIN_MENU;
                }
                if (UIButton(bx, by + 3*(BH+BG), BW, BH, T(StrId::BTN_QUIT),
                             mx, my, lmb, g_LmbPrev)) {
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                }
            }
            else if (st == GameState::GAMEOVER) {
                // 전체 화면 딤 — 보스 창/엔티티가 결과창 뒤로 비치지 않게
                BindMainShader();
                drawRect(0, 0, sw, sh, 0.02f, 0.02f, 0.06f, 0.88f);
                const wchar_t* T1 = T(StrId::GAMEOVER);
                g_TextL.Draw(T1, cx(T1, g_TextL, 1.6f), sh*0.30f, 1.6f,
                             1, 0.25f, 0.25f, 0.95f);

                // 결과 — 도달 레벨 / 처치 수 / 최종 점수
                wchar_t lvBuf[64], killBuf[64], scoreBuf[64];
                swprintf_s(lvBuf,    L"%ls   Lv. %d", T(StrId::REACHED_LEVEL),
                           g_GameManager.playerLevel);
                swprintf_s(killBuf,  L"%ls   %lld",   T(StrId::KILL_COUNT),
                           g_Stats.killCount);
                swprintf_s(scoreBuf, L"%ls   %lld",   T(StrId::FINAL_SCORE),
                           g_GameManager.score);

                g_TextL.Draw(scoreBuf, cx(scoreBuf, g_TextL, 1.2f), sh*0.44f, 1.2f,
                             1.0f, 1.0f, 0.7f, 0.95f);
                g_TextS.Draw(lvBuf,    cx(lvBuf,    g_TextS, 1.1f), sh*0.52f, 1.1f,
                             0.85f, 0.95f, 0.85f, 0.95f);
                g_TextS.Draw(killBuf,  cx(killBuf,  g_TextS, 1.1f), sh*0.57f, 1.1f,
                             0.85f, 0.95f, 0.85f, 0.95f);

                // 최고 점수 (난이도별) + 신기록 + 획득 코인
                wchar_t bestBuf[64], coinBuf[64];
                swprintf_s(bestBuf, L"BEST   %lld", g_BestScore[(int)g_Difficulty]);
                g_TextS.Draw(bestBuf, cx(bestBuf, g_TextS, 1.1f), sh*0.62f, 1.1f,
                             1.0f, 0.85f, 0.4f, 0.95f);
                swprintf_s(coinBuf, L"+%lld COIN  (total %lld)", g_LastRunCoins, g_Coins);
                g_TextS.Draw(coinBuf, cx(coinBuf, g_TextS, 1.0f), sh*0.665f, 1.0f,
                             1.0f, 0.9f, 0.3f, 0.95f);
                if (g_LastRunRecord) {
                    const wchar_t* rec = L"★ NEW RECORD ★";
                    float blink = 0.6f + 0.4f * sinf((float)glfwGetTime() * 6.0f);
                    g_TextL.Draw(rec, cx(rec, g_TextL, 1.0f), sh*0.37f, 1.0f,
                                 1.0f, 0.9f, 0.2f, blink);
                }

                // 버튼 3개: 다시하기 / 메뉴로 / 종료
                const float BW = 240.0f, BH = 56.0f, BG = 16.0f;
                float totalW = 3 * BW + 2 * BG;
                float bx = (sw - totalW) * 0.5f;
                float by = sh * 0.72f;

                if (UIButton(bx + 0*(BW+BG), by, BW, BH, T(StrId::BTN_RESTART),
                             mx, my, lmb, g_LmbPrev)) {
                    ResetForNewGame();
                    PickRandomWeapons(g_WeaponChoices);
                    g_GameManager.currentState = GameState::WEAPON_SELECT;
                }
                if (UIButton(bx + 1*(BW+BG), by, BW, BH, T(StrId::BTN_MAIN_MENU),
                             mx, my, lmb, g_LmbPrev)) {
                    g_GameManager.currentState = GameState::MAIN_MENU;
                }
                if (UIButton(bx + 2*(BW+BG), by, BW, BH, T(StrId::BTN_QUIT),
                             mx, my, lmb, g_LmbPrev)) {
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                }
            }
            else if (st == GameState::AUG_SELECT || st == GameState::DEBUFF_SELECT) {
                // ── 카드 레이아웃 (GameManager::Render 와 동기화) ──
                bool hasConv = (g_ConversionWeapon >= 0 && st == GameState::AUG_SELECT);
                int  nCards  = hasConv ? 4 : 3;
                const float CARD_W = hasConv ? 240.0f : 280.0f;
                const float CARD_H = 400.0f;
                const float GAP    = hasConv ? 32.0f : 48.0f;
                const float TOTAL_W = (float)nCards * CARD_W + (float)(nCards-1) * GAP;
                float baseX = (sw - TOTAL_W) * 0.5f;
                float baseY = (sh - CARD_H)  * 0.4f;

                // 타이틀
                const wchar_t* TIT = (st == GameState::DEBUFF_SELECT)
                                     ? T(StrId::CHOOSE_DEBUFF)
                                     : T(StrId::CHOOSE_AUG);
                g_TextL.Draw(TIT, cx(TIT, g_TextL, 1.0f), baseY - 56.0f, 1.0f,
                             1,1,1,0.95f);

                // 키 힌트
                const wchar_t* HINT = (g_HoveredAug < 0)
                                      ? T(StrId::KEY_HINT_NO_HOVER)
                                      : T(StrId::KEY_HINT_HOVER);
                g_TextS.Draw(HINT, cx(HINT, g_TextS, 0.85f),
                             baseY + CARD_H + 200.0f,
                             0.85f, 0.75f,0.75f,0.75f,0.8f);

                // 각 카드: 등급 라벨 + 이름만 (4번째는 변환 카드)
                static const wchar_t* KEY_LABELS[4] = {L"[ 1 ]", L"[ 2 ]", L"[ 3 ]", L"[ 4 ]"};
                for (int i = 0; i < nCards; i++) {
                    float cardX = baseX + i * (CARD_W + GAP);
                    float yOff  = (g_HoveredAug == i) ? -16.0f : 0.0f;

                    // 카드 이름 — 3개는 증강, 4번째는 변환 무기
                    const wchar_t* cardName;
                    float tr, tg, tb;
                    const wchar_t* topLabel;
                    if (i < 3) {
                        const AugDef& def = ALL_AUGS[g_GameManager.augChoices[i]];
                        cardName = AugName(def);
                        GetRarityColor(def.rarity, tr, tg, tb);
                        tr = std::min(1.0f, tr * 1.4f + 0.25f);
                        tg = std::min(1.0f, tg * 1.4f + 0.25f);
                        tb = std::min(1.0f, tb * 1.4f + 0.25f);
                        topLabel = GetRarityKR(def.rarity);
                        // 픽토그램 (있으면) — 카드 상단 중앙, 흰색
                        GLuint icon = IconFor(def.type);
                        if (icon) {
                            float isz = 110.0f;
                            DrawIcon(icon, cardX + (CARD_W - isz) * 0.5f,
                                     baseY + yOff + CARD_H * 0.12f, isz, isz,
                                     1.0f, 1.0f, 1.0f, 0.97f);
                        }
                    } else {
                        cardName = (g_ConversionWeapon >= 0)
                                 ? WeaponName(ALL_WEAPONS[g_ConversionWeapon]) : L"?";
                        tr = 1.0f; tg = 0.9f; tb = 0.3f;
                        topLabel = L"변환";
                    }
                    float rw = g_TextS.Width(topLabel, 1.0f);
                    g_TextS.Draw(topLabel,
                                 cardX + (CARD_W - rw) * 0.5f,
                                 baseY + yOff + 22.0f, 1.0f, tr, tg, tb, 0.95f);

                    // 증강/무기 이름
                    float nameSc = 1.2f;
                    while (nameSc > 0.7f &&
                           g_TextL.Width(cardName, nameSc) > CARD_W - 20.0f)
                        nameSc -= 0.05f;
                    float nw = g_TextL.Width(cardName, nameSc);
                    g_TextL.Draw(cardName,
                                 cardX + (CARD_W - nw) * 0.5f,
                                 baseY + yOff + CARD_H * 0.40f, nameSc,
                                 1,1,1,0.95f);

                    // 키 힌트 (카드 하단)
                    float kw = g_TextS.Width(KEY_LABELS[i], 1.0f);
                    g_TextS.Draw(KEY_LABELS[i],
                                 cardX + (CARD_W - kw) * 0.5f,
                                 baseY + yOff + CARD_H - 50.0f, 1.0f,
                                 tr, tg, tb, 0.95f);
                }

                // ── 하단 상세 설명 박스 (호버 카드의 전체 설명) ──
                if (g_HoveredAug >= 0) {
                    // 호버 카드 설명 + 상단 띠 색상 (3개는 증강, 4번째는 변환 무기)
                    const wchar_t* hDesc;
                    float hr, hg, hb;
                    if (g_HoveredAug < 3) {
                        int hIdx = g_GameManager.augChoices[g_HoveredAug];
                        if (hIdx < 0) hIdx = 0;
                        const AugDef& hDef = ALL_AUGS[hIdx];
                        hDesc = AugDesc(hDef);
                        GetRarityColor(hDef.rarity, hr, hg, hb);
                    } else {
                        hDesc = (g_ConversionWeapon >= 0)
                              ? WeaponDesc(ALL_WEAPONS[g_ConversionWeapon])
                              : L"기존 무기 효과 제거 후 새 무기로 전환";
                        hr = 1.0f; hg = 0.78f; hb = 0.10f;  // 변환 = 금색
                    }
                    float boxY = baseY + CARD_H + 24.0f;
                    float boxW = TOTAL_W;
                    float boxH = 150.0f;
                    float boxX = (sw - boxW) * 0.5f;

                    // 박스 배경 (어두운 반투명)
                    drawRect(boxX, boxY, boxW, boxH, 0.03f, 0.03f, 0.05f, 0.85f);

                    // 상단 띠
                    drawRect(boxX, boxY, boxW, 4.0f, hr, hg, hb, 1.0f);

                    // 설명 ('/' 분리, 각 줄 fit)
                    std::vector<std::wstring> lines;
                    std::wstring cur;
                    for (const wchar_t* p = hDesc; *p; ++p) {
                        if (*p == L'/') {
                            if (!cur.empty()) lines.push_back(cur);
                            cur.clear();
                        } else cur += *p;
                    }
                    if (!cur.empty()) lines.push_back(cur);
                    for (auto& s : lines) {
                        while (!s.empty() && (s.front() == L' ' || s.front() == L'\t'))
                            s.erase(0, 1);
                        while (!s.empty() && (s.back() == L' ' || s.back() == L'\t'))
                            s.pop_back();
                    }

                    int n = (int)lines.size();
                    if (n < 1) n = 1;
                    float lineH = 32.0f;
                    float startY = boxY + 22.0f + (boxH - 22.0f - lineH * n) * 0.5f;
                    for (int li = 0; li < n; li++) {
                        const wchar_t* s = lines[li].c_str();
                        // 일부러 안 줄임 — 박스가 카드 3장 폭이라 충분
                        float sc = 1.0f;
                        float lw = g_TextS.Width(s, sc);
                        if (lw > boxW - 40.0f) {
                            // 박스 폭도 안 되면 그때만 축소
                            while (sc > 0.7f && g_TextS.Width(s, sc) > boxW - 40.0f)
                                sc -= 0.05f;
                            lw = g_TextS.Width(s, sc);
                        }
                        g_TextS.Draw(s, boxX + (boxW - lw) * 0.5f,
                                     startY + lineH * (float)li, sc,
                                     1.0f, 1.0f, 1.0f, 0.95f);
                    }
                }
            }

            // 보유 증강 패널 (PAUSED / AUG_SELECT / DEBUFF_SELECT 에서 좌측)
            if (st == GameState::PAUSED ||
                st == GameState::AUG_SELECT ||
                st == GameState::DEBUFF_SELECT) {
                // 같은 인덱스 카운트 (스택)
                int counts[AUG_TOTAL] = {};
                for (int idx : g_OwnedAugs) counts[idx]++;

                const float PX  = 16.0f;
                const float ROW_H = 30.0f;
                float       py  = 60.0f;
                const wchar_t* TITLE = T(StrId::OWNED_AUGS);
                g_TextS.Draw(TITLE, PX, py, 1.1f, 1, 1, 1, 0.95f);
                if (st == GameState::PAUSED) {
                    g_TextS.Draw(L"(클릭 → 설명)", PX + 2.0f, py + 30.0f, 0.70f,
                                 0.6f, 0.7f, 0.9f, 0.7f);
                }
                py += 50.0f;

                for (int i = 0; i < AUG_TOTAL; i++) {
                    if (counts[i] == 0) continue;
                    if (py > sh - 80.0f) break;
                    const AugDef& def = ALL_AUGS[i];
                    float cr, cg, cb;
                    GetRarityColor(def.rarity, cr, cg, cb);
                    cr = std::min(1.0f, cr * 1.3f + 0.25f);
                    cg = std::min(1.0f, cg * 1.3f + 0.25f);
                    cb = std::min(1.0f, cb * 1.3f + 0.25f);

                    // PAUSED: 클릭으로 설명 선택. 선택된 항목은 배경 강조
                    if (st == GameState::PAUSED) {
                        bool rowHover = (mx >= 0 && mx <= 240.0f &&
                                        my >= py - 4.0f && my <= py + ROW_H - 6.0f);
                        if (rowHover) {
                            // 호버 배경
                            BindMainShader();
                            drawRect(0, py - 4.0f, 240.0f, ROW_H, 0.15f, 0.15f, 0.25f, 0.55f);
                        }
                        if (g_PauseSelectedAug == i) {
                            // 선택 배경
                            BindMainShader();
                            drawRect(0, py - 4.0f, 240.0f, ROW_H, 0.12f, 0.20f, 0.35f, 0.75f);
                            drawRect(0, py - 4.0f, 3.0f, ROW_H, cr, cg, cb, 1.0f);
                        }
                        // 클릭 처리
                        if (lmb && !g_LmbPrev && rowHover) {
                            g_PauseSelectedAug = (g_PauseSelectedAug == i) ? -1 : i;
                        }
                    }

                    wchar_t line[96];
                    if (counts[i] > 1)
                        swprintf_s(line, L"· %ls  ×%d", AugName(def), counts[i]);
                    else
                        swprintf_s(line, L"· %ls", AugName(def));
                    g_TextS.Draw(line, PX, py, 0.85f, cr, cg, cb, 0.9f);
                    py += ROW_H;
                }

                // PAUSED: 선택된 증강 설명 박스
                if (st == GameState::PAUSED && g_PauseSelectedAug >= 0 &&
                    g_PauseSelectedAug < AUG_TOTAL) {
                    const AugDef& sd = ALL_AUGS[g_PauseSelectedAug];
                    // 원래 위치 (BX=220, BY=60) 복원
                    const float BX = 220.0f;
                    const float BY = 60.0f;
                    const float BW = std::min(420.0f, sw - BX - 20.0f);
                    const float BH = 270.0f;

                    // 배경 + 등급 색 띠
                    BindMainShader();
                    drawRect(BX, BY, BW, BH, 0.03f, 0.03f, 0.06f, 0.92f);
                    float hr, hg, hb;
                    GetRarityColor(sd.rarity, hr, hg, hb);
                    drawRect(BX, BY, BW, 5.0f, hr, hg, hb, 1.0f);

                    // 등급 라벨 (소)
                    const wchar_t* rLabel = GetRarityKR(sd.rarity);
                    g_TextS.Draw(rLabel, BX + 12.0f, BY + 12.0f, 0.9f,
                                 hr, hg, hb, 0.95f);

                    // 가로 구분선
                    BindMainShader();
                    drawRect(BX + 10.0f, BY + 38.0f, BW - 20.0f, 1.0f,
                             0.3f, 0.3f, 0.4f, 0.5f);

                    // 증강 이름 (대) — 구분선 아래
                    float nSc = 1.05f;
                    while (nSc > 0.65f && g_TextL.Width(AugName(sd), nSc) > BW - 20.0f)
                        nSc -= 0.05f;
                    float nLw   = g_TextL.Width(AugName(sd), nSc);
                    float nameY = BY + 48.0f;
                    g_TextL.Draw(AugName(sd), BX + (BW - nLw) * 0.5f, nameY, nSc,
                                 1.0f, 1.0f, 1.0f, 0.98f);
                    float nameH = g_TextL.Height(AugName(sd), nSc);

                    // 설명 ('/' 분리) — BY+92 부터 (이름 아래 여유)
                    std::vector<std::wstring> dlines;
                    std::wstring cur2;
                    for (const wchar_t* p = AugDesc(sd); *p; ++p) {
                        if (*p == L'/') { if (!cur2.empty()) dlines.push_back(cur2); cur2.clear(); }
                        else cur2 += *p;
                    }
                    if (!cur2.empty()) dlines.push_back(cur2);
                    for (auto& s2 : dlines) {
                        while (!s2.empty() && (s2.front()==L' '||s2.front()==L'\t')) s2.erase(0,1);
                        while (!s2.empty() && (s2.back() ==L' '||s2.back() ==L'\t')) s2.pop_back();
                    }
                    int nd = (int)dlines.size(); if (nd < 1) nd = 1;
                    float dLineH = 26.0f;
                    float dStartY = nameY + nameH + 8.0f;   // 이름 실제 높이 아래에서 시작
                    for (int li = 0; li < nd; li++) {
                        const wchar_t* ds = dlines[li].c_str();
                        float dsc = 0.9f;
                        while (dsc > 0.65f && g_TextS.Width(ds, dsc) > BW - 24.0f)
                            dsc -= 0.05f;
                        float dlw = g_TextS.Width(ds, dsc);
                        g_TextS.Draw(ds, BX + (BW - dlw) * 0.5f,
                                     dStartY + dLineH * (float)li, dsc,
                                     1.0f, 1.0f, 0.95f, 0.9f);
                        if (dStartY + dLineH * (float)(li+1) > BY + BH - 10.0f) break;
                    }
                }
            }

            // 상단 HUD (RUNNING / PAUSED / DYING / AUG_SELECT 모두 노출)
            if (st != GameState::READY && st != GameState::GAMEOVER) {
                // macOS: 상단 메뉴바(~25px)가 화면 맨 위를 가리므로 HUD 를 아래로
#ifdef __APPLE__
                const float hudTopY = 8.0f + 30.0f;
#else
                const float hudTopY = 8.0f;
#endif
                // 좌상단: Lv. / XP
                long long need = g_ExpSystem.Required(g_GameManager.playerLevel);
                wchar_t xpBuf[64];
                swprintf_s(xpBuf, L"%ls%d   %lld / %lld",
                           T(StrId::LV_PREFIX), g_GameManager.playerLevel,
                           g_GameManager.xp, need);
                g_TextS.Draw(xpBuf, 12.0f, hudTopY, 0.85f, 0.7f,1.0f,0.7f,0.9f);

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
            }

            // ── 손맛: 데미지 숫자 팝업 + 콤보 카운터 ──────────────────
            if (st == GameState::RUNNING || st == GameState::DYING ||
                st == GameState::PAUSED) {
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
                    wchar_t cb[32]; swprintf_s(cb, L"%d COMBO", g_Combo);
                    float sc = (1.05f + (g_Combo > 30 ? 0.3f : 0.0f)) * (1.0f + g_ComboPulse * 0.4f);
                    float cr = 1.0f, cg = 1.0f, cbl = 1.0f;
                    if      (g_Combo >= 50) { cg = 0.25f; cbl = 0.2f; }
                    else if (g_Combo >= 25) { cg = 0.55f; cbl = 0.15f; }
                    else if (g_Combo >= 12) { cg = 0.9f;  cbl = 0.3f; }
                    float w = g_TextS.Width(cb, sc);
                    g_TextS.Draw(cb, (sw - w) * 0.5f, sh * 0.115f, sc, cr, cg, cbl, 0.95f);
                }
            }

            // ── 액티브 스킬 슬롯 (좌하단, 패시브 쿨다운 위 행) ──
            if (st == GameState::RUNNING || st == GameState::PAUSED) {
                const float KW = 54.0f, KH = 54.0f, KG = 8.0f;
                float kx0 = 16.0f, ky0 = sh - KH - 40.0f - (56.0f + 8.0f);
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
                }
            }

            // ── 액티브/패시브 쿨다운 UI (좌하단) ─────────────────────
            // 추후 픽토그램 PNG 가 들어오면 사각형 placeholder 자리에 텍스처 표시
            if (st == GameState::RUNNING || st == GameState::PAUSED) {
                const float SLOT_W = 56.0f, SLOT_H = 56.0f, SLOT_GAP = 8.0f;
                float baseX  = 16.0f;
                float baseY2 = sh - SLOT_H - 40.0f;   // HP 바 위쪽
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
                // 시즈 탱크 stack 표시 — 증강 제거됨 (siegeTank 항상 false). dead code 유지
                if (false && g_Stats.siegeTank) {
                    float x = baseX + slot * (SLOT_W + SLOT_GAP);
                    drawRect(x, baseY2, SLOT_W, SLOT_H, 0.05f, 0.05f, 0.08f, 0.85f);
                    drawRect(x, baseY2, SLOT_W, 4.0f, 0.6f, 0.4f, 0.9f, 1.0f);
                    g_TextS.Draw(L"SIEGE", x + 2.0f, baseY2 + 6.0f, 0.6f,
                                 0.7f, 0.5f, 1.0f, 1.0f);
                    wchar_t buf[8];
                    swprintf_s(buf, L"%d", g_Stats.siegeStacks);
                    float tw = g_TextL.Width(buf, 0.9f);
                    g_TextL.Draw(buf, x + (SLOT_W - tw) * 0.5f,
                                 baseY2 + SLOT_H * 0.40f, 0.9f, 1,1,1,0.95f);
                    ++slot;
                }
            }
        }

        // ── 글리치 보스 화면 연출 (리소스 없이, 최상단) ──
        //   1) 글리치 가로 찢김(시안/마젠타 바)  2) "펑!" 화이트아웃  3) 텍스트 노이즈
        if (g_GlitchBoss && g_GlitchBoss->alive) {
            auto* gb = g_GlitchBoss;
            BindMainShader();
            // 1) 글리치 가로 찢김
            if (gb->glitchAmount > 0.02f) {
                for (int i = 0; i < 7; i++) {
                    float by  = (float)(rand() % screenHeight);
                    float bh  = 2.0f + (float)(rand() % 9);
                    float off = (float)(rand() % 40 - 20) * gb->glitchAmount;
                    float a   = gb->glitchAmount * 0.45f;
                    drawRect(off,  by,      (float)screenWidth, bh, 0.0f, 1.0f, 1.0f, a);
                    drawRect(-off, by + bh, (float)screenWidth, bh, 1.0f, 0.0f, 1.0f, a);
                }
            }
            // 2) "펑!" 화이트아웃 깜빡임
            if (gb->burstFlash > 0.01f) {
                drawRect(0, 0, (float)screenWidth, (float)screenHeight,
                         1.0f, 1.0f, 1.0f, gb->burstFlash * 0.85f);
            }
            // 3) 텍스트 노이즈 — 외계어 에러 깜빡
            if (gb->textNoise > 0.3f && (rand() % 2 == 0)) {
                static const wchar_t* errs[4] =
                    { L"Err_0x7B", L"SYS_FAULT", L"0xDEADBEEF", L"SEGFAULT" };
                for (int i = 0; i < 4; i++) {
                    float ex = (float)(rand() % screenWidth);
                    float ey = (float)(rand() % screenHeight);
                    g_TextL.Draw(errs[rand() % 4], ex, ey, 0.75f,
                                 1.0f, 0.1f, 0.3f, gb->textNoise * 0.85f);
                }
            }
        }

        // 마우스 click edge 감지용 — 이번 프레임 상태를 저장
        g_LmbPrev = lmb;

        // 업적 해금 / 도감 발견 발생 시 저장 (게임 중 즉시 영구화)
        if (g_AchSaveNeeded || g_CodexDirty) {
            SaveGame(); g_AchSaveNeeded = false; g_CodexDirty = false;
        }

        glfwSwapBuffers(window);

        // ── FPS 캡 (g_FpsCap > 0 일 때만) ─────────────────────────────
        // timeBeginPeriod(1) 로 Sleep 해상도 1ms. 마지막 ~1ms 는 busy-wait
        if (g_FpsCap > 0) {
            double target = 1.0 / (double)g_FpsCap;
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

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &g_VBO);
    glDeleteProgram(shader);
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
    PlatformTimerEnd();
    glfwTerminate();
    return 0;
}
