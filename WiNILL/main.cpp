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

// 화면 흔들기 (보스 스폰, 충격파)
float g_ShakeTime = 0.0f;
float g_ShakeMag  = 0.0f;

// 보스 보상 — 남은 버프 픽 수 (디버프 페이지 skip)
int  g_BossRewardPicksLeft = 0;
// 보스 스폰 1회 트리거 플래그 (같은 라운드에 중복 스폰 방지)
bool g_BossSpawnedThisRun  = false;

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
    int count   = big ? 14 : 6;
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
                                          "WiNILL FakeWindow", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }

    glfwSetWindowPos(window, 0, 0);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    // g_FpsCap == 0 : VSync, 그 외 : VSync 끄고 수동 캡
    glfwSwapInterval((g_FpsCap == 0) ? 1 : 0);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    // 폰트 초기화
#ifdef _WIN32
    // Windows: EXE 임베디드 TTF (Resource.rc 의 RCDATA) 로 GDI 에 등록 후 시스템 face 사용
    auto loadRcFont = [&](const char* resName) -> HANDLE {
        HMODULE hMod = GetModuleHandleA(nullptr);
        HRSRC   hRes = FindResourceA(hMod, resName, (LPCSTR)RT_RCDATA);
        if (!hRes) return nullptr;
        DWORD   sz    = SizeofResource(hMod, hRes);
        HGLOBAL hData = LoadResource(hMod, hRes);
        void*   pData = LockResource(hData);
        if (!pData || sz == 0) return nullptr;
        DWORD numFonts = 0;
        return AddFontMemResourceEx(pData, sz, nullptr, &numFonts);
    };
    g_FontMemHandle   = loadRcFont("DONGLE_FONT");
    g_OswaldMemHandle = loadRcFont("OSWALD_FONT");

    const char* face = LanguageFace(g_Language);
    if (g_FontMemHandle || g_OswaldMemHandle) {
        g_TextL.InitWithFace(face, 36, screenWidth, screenHeight);
        g_TextS.InitWithFace(face, 22, screenWidth, screenHeight);
    } else {
        const char* fp = (g_Language == Language::KR)
                         ? "Resource/Font/Dongle-Regular.ttf"
                         : "Resource/Font/Oswald-VariableFont_wght.ttf";
        g_TextL.Init(fp, face, 36, screenWidth, screenHeight);
        g_TextS.Init(fp, face, 22, screenWidth, screenHeight);
    }
#else
    // macOS/Linux: 디스크의 TTF 폴백 체인을 stb_truetype 으로 직접 래스터화
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
    FakeWindow playerWin(0, "WiNILL",
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

        // 마우스 상태
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        bool lmb = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

        // --- 입력 처리 ---
        GameState prevState = g_GameManager.currentState;
        g_GameManager.HandleInput(window);

        // 새 게임 리셋 람다 (GAMEOVER → READY, 난이도 선택 후, 다시하기 버튼 등에서 호출)
        auto ResetForNewGame = [&]() {
            g_Stats        = PlayerStats();
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
            g_HoveredAug       = -1;
            g_EnterReleased    = true;
            g_ConversionWeapon = -1;
            g_CurrentWeapon    = -1;
            g_PauseSelectedAug = -1;
            g_GameTime         = 0.0f;
            g_BomberSpawnTimer = 0.0f;
            for (int i = 0; i < MAX_SHOCKS; i++) g_ShockWaves[i].active = false;
            g_ShakeTime = 0.0f; g_ShakeMag = 0.0f;
            g_BossSpawnedThisRun  = false;
            g_BossRewardPicksLeft = 0;
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
                g_GameManager.score      = 100000;
                g_GameManager.scoreAccum = 100000.0f;
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
                    int prevAugs = g_Stats.totalAugs;
                    g_Stats = PlayerStats();
                    g_OwnedAugs.clear();
                    memset(g_GameManager.takenOnce, 0,
                           sizeof(g_GameManager.takenOnce));
                    g_GameManager.maxHP = g_Stats.maxHP;
                    if (g_GameManager.playerHP > g_Stats.maxHP)
                        g_GameManager.playerHP = g_Stats.maxHP;
                    if (prevAugs > 16) prevAugs = 16;
                    int nDebuffs = prevAugs * 2 / 5;       // 40%
                    int nBuffs   = prevAugs - nDebuffs;
                    int buffs[16], debuffs[16];
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
                bool onceOnly = (r == AugRarity::EPIC || r == AugRarity::LEGENDARY);
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
                                                         g_Stats.distAugTaken);
                            g_GameManager.currentState = GameState::AUG_SELECT;
                        } else {
                            g_GameManager.currentState = GameState::RUNNING;
                        }
                    } else if (g_Stats.mk2SkipDebuff || g_CreativeMode) {
                        // MK2 부활 후 / 크리에이티브 모드: DEBUFF_SELECT 스킵
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

        // --- 크리에이티브 모드: RUNNING 중 F 키 → AUG_SELECT 즉시 열기 ---
        if (g_CreativeMode) {
            static bool s_fkeyReleased = true;
            int kF = glfwGetKey(window, GLFW_KEY_F);
            if (kF == GLFW_RELEASE) s_fkeyReleased = true;
            if (kF == GLFW_PRESS && s_fkeyReleased &&
                g_GameManager.currentState == GameState::RUNNING) {
                g_GameManager.PickAugChoices(g_Stats.sizeAugTaken,
                                             g_Stats.distAugTaken);
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
        }

        // --- Fixed timestep (DYING = 0.15× 슬로우 모션) ---
        float physDelta = (g_GameManager.currentState == GameState::DYING)
                          ? delta * 0.15f : delta;
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

                // 플레이어 캐릭터(창 중앙)가 화면 밖으로 나가지 못하게 클램프
                // 창 자체는 화면 밖으로 일부 나갈 수 있음
                float pCX = playerWin.x + playerWin.width  * 0.5f;
                float pCY = playerWin.y + playerWin.height * 0.5f;
                if (pCX < 0)                   pCX = 0;
                if (pCY < 0)                   pCY = 0;
                if (pCX > (float)screenWidth)  pCX = (float)screenWidth;
                if (pCY > (float)screenHeight) pCY = (float)screenHeight;
                playerWin.x = pCX - playerWin.width  * 0.5f;
                playerWin.y = pCY - playerWin.height * 0.5f;

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
                    b.Update(FIXED_DT);
                    if (b.x < -200 || b.x > screenWidth  + 200 ||
                        b.y < -200 || b.y > screenHeight + 200)
                        b.active = false;
                }

                // 몬스터 업데이트 (디버프 multiplier 적용)
                float rmobMoveMult = 1.0f / g_Stats.rmobDelayMult; // <1 → 더 빠름
                g_MonsterManager.UpdateAll(pCX, pCY, FIXED_DT,
                                           g_GameManager.playerHP, g_Bullets,
                                           g_Stats.mobSpeedMult, rmobMoveMult);

                // 충돌 (반환값 = 플레이어가 이번 프레임 피격됐는지)
                bool hit = CollisionSystem::Update(pCX, pCY,
                    g_MonsterManager, g_Bullets,
                    g_GameManager.playerHP,
                    g_GameManager.scoreAccum, g_GameManager.score,
                    g_Stats, g_GameManager.xp);

                // 사망 폭발 spawn (다음 UpdateAll 이 실제 erase 하기 전에 위치 캡처)
                for (auto m : g_MonsterManager.monsters) {
                    if (!m->alive && !m->exploded) {
                        SpawnEnemyExplosion(m->worldX, m->worldY,
                                            m->color.r, m->color.g, m->color.b,
                                            /*big=*/false);
                        m->exploded = true;
                    }
                }
                for (auto r : g_MonsterManager.rangedMobs) {
                    if (!r->alive && !r->exploded) {
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
                    // 큰 폭발 + 충격파 + 화면 흔들기
                    SpawnEnemyExplosion(bs->worldX, bs->worldY, 1.0f, 0.5f, 0.9f, true);
                    SpawnEnemyExplosion(bs->worldX, bs->worldY, 0.9f, 0.4f, 1.0f, true);
                    SpawnShockWave(bs->worldX, bs->worldY, 350.0f, 0.7f,
                                   0.9f, 0.5f, 1.0f);
                    g_ShakeTime = 0.5f; g_ShakeMag = 18.0f;
                    bs->exploded = true;
                    // 점수 보상
                    g_GameManager.scoreAccum += 20000.0f;
                    g_GameManager.score = (long long)g_GameManager.scoreAccum;
                    // 보스 객체 정리
                    delete bs;
                    g_MonsterManager.boss = nullptr;
                    // 버프 2개 픽 (디버프 없이)
                    g_BossRewardPicksLeft = 2;
                    g_GameManager.PickAugChoices(g_Stats.sizeAugTaken,
                                                 g_Stats.distAugTaken);
                    g_GameManager.currentState = GameState::AUG_SELECT;
                }

                // 자폭병 사망 (점화 후 폭발 OR 총알 격파)
                for (auto bm : g_MonsterManager.bombers) {
                    if (!bm->alive && !bm->exploded) {
                        bool blast = bm->arming;
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

        if (g_GameManager.currentState == GameState::RUNNING) {
            float pCX = playerWin.x + playerWin.width  * 0.5f;
            float pCY = playerWin.y + playerWin.height * 0.5f;
            bool  moving = keys[GLFW_KEY_W] || keys[GLFW_KEY_S] ||
                           keys[GLFW_KEY_A] || keys[GLFW_KEY_D];

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

            // 잡몹 스폰 (D_MOB_SPAWN → 더 자주 + cap +200, D_MOB_HP → HP+)
            spawnTimer += delta;
            float spawnInterval = 0.3f * g_Stats.mobSpawnMult;
            if (spawnTimer > spawnInterval) {
                g_MonsterManager.SpawnMob(screenWidth, screenHeight,
                                          100 + g_Stats.mobCapBonus,
                                          g_Stats.monsterHpMult);
                spawnTimer = 0.0f;
            }

            // 게임 시간 카운트
            g_GameTime += delta;

            // 보스 (슬라임) 스폰 — 한 라운드 1회, score 10만 도달 시
            if (!g_BossSpawnedThisRun &&
                g_GameManager.score >= 100000 &&
                g_MonsterManager.boss == nullptr) {
                g_BossSpawnedThisRun = true;
                // 화면 중앙 근처에서 등장
                float bsx = screenWidth * 0.5f
                          + ((float)(rand() % 200) - 100.0f);
                float bsy = screenHeight * 0.5f
                          + ((float)(rand() % 200) - 100.0f);
                float bossHp = GetDifficultyParams(g_Difficulty).bossHp;
                g_MonsterManager.boss =
                    new Boss(bsx, bsy, screenWidth, screenHeight, bossHp);
                // 등장 연출 — 큰 화면 흔들기 + 충격파 + 빵빵 폭발
                g_ShakeTime = 0.6f; g_ShakeMag = 28.0f;
                SpawnShockWave(bsx, bsy, 500.0f, 0.9f, 1.0f, 0.3f, 0.3f);
                SpawnShockWave(bsx, bsy, 320.0f, 0.7f, 1.0f, 0.8f, 0.2f);
                for (int k = 0; k < 3; k++) {
                    SpawnEnemyExplosion(bsx + (rand()%200 - 100),
                                        bsy + (rand()%200 - 100),
                                        1.0f, 0.3f, 0.3f, true);
                }
            }

            // 자폭병 spawn (난이도별 시작 시간/주기, 쉬움은 안 나옴)
            DifficultyParams dp = GetDifficultyParams(g_Difficulty);
            if (g_GameTime >= dp.bomberStartTime) {
                g_BomberSpawnTimer += delta;
                if (g_BomberSpawnTimer >= dp.bomberInterval) {
                    g_MonsterManager.SpawnBomber(screenWidth, screenHeight,
                                                 g_Stats.bomberHpMult,
                                                 g_Stats.bomberSpeedMult,
                                                 g_Stats.bomberBlastMult);
                    g_BomberSpawnTimer = 0.0f;
                }
            }

            // 원거리 몹 스폰 — 난이도별 + 디버프 (D_RMOB_MAX, rmobSpawnDelayBonus)
            rangedSpawnTimer += delta;
            float rangedInterval = dp.rangedSpawnInterval - g_Stats.rmobSpawnDelayBonus;
            if (rangedInterval < 1.0f) rangedInterval = 1.0f;   // 최소 1초
            int rangedMax = dp.rangedMaxBase + g_Stats.rmobMaxBonus;
            if (rangedSpawnTimer > rangedInterval) {
                g_MonsterManager.SpawnRangedMob(screenWidth, screenHeight,
                    g_Stats.rmobHpMult, rangedMax);
                rangedSpawnTimer = 0.0f;
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
                g_Bullets.push_back(nb);
            };

            // Twin: ±5도 2발 / Shotgun: 5발 산탄 (사거리 700)
            auto spawnAimed = [&](float tx, float ty) {
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

            // 포탑 모드에서는 플레이어가 발사하지 않음
            if (!g_Stats.turretMode) {
                if (g_Stats.brokenSight) {
                    if (fireTimer >= effInterval && g_Orb.active) {
                        spawnAimed(g_Orb.x, g_Orb.y);
                        fireTimer = 0.0f;
                    }
                } else {
                    if (lmb && fireTimer >= effInterval) {
                        float tx = (float)mx, ty = (float)my;
                        if (g_DrunkActive) {
                            float a = (float)(rand() % 628) * 0.01f;
                            tx = pCX + cosf(a) * 200.0f;
                            ty = pCY + sinf(a) * 200.0f;
                        }
                        spawnAimed(tx, ty);
                        fireTimer = 0.0f;
                    }
                    // 클릭 release 시 타이머 리셋 — 다음 클릭에 즉시 발사 가능 (일반 무기 UX)
                    // CANNON 은 연사 1초 고정이라 클릭 연타로 우회되면 안 됨 → 리셋 스킵
                    if (!lmb && !g_Stats.cannon) fireTimer = effInterval;
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
        // 화면 흔들기 적용 — game world 만 흔들리고 HUD/text 는 안 흔들림
        float orthoShake[16];
        memcpy(orthoShake, ortho, sizeof(ortho));
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
            glScissor(
                (GLint) rwx,
                (GLint)(screenHeight - (rwy + rH)),
                (GLint) rW,
                (GLint) rH
            );
            // 잡몹 (보스 소환물은 더 큼)
            for (auto m : g_MonsterManager.monsters) {
                if (!m->alive) continue;
                float sz = m->summoned ? 28.0f : 18.0f;
                drawTriangle(m->worldX, m->worldY, sz,
                             m->color.r, m->color.g, m->color.b, 1.0f);
            }
            // 자폭병 (5각형)
            for (auto bm : g_MonsterManager.bombers) {
                if (!bm->alive) continue;
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
                if (!b.active) continue;
                drawCircle(b.x, b.y, 6.0f * b.sizeScale, b.color.r, b.color.g, b.color.b, 1.0f);
            }
            // 사망 파티클
            for (auto& p : g_EnemyParts) {
                if (!p.active) continue;
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
                glScissor(
                    (GLint) twx,
                    (GLint)(screenHeight - (twy + TURRET_WIN_H)),
                    (GLint) TURRET_WIN_W,
                    (GLint) TURRET_WIN_H
                );
                for (auto m : g_MonsterManager.monsters) {
                    if (!m->alive) continue;
                    float sz = m->summoned ? 28.0f : 18.0f;
                    drawTriangle(m->worldX, m->worldY, sz,
                                 m->color.r, m->color.g, m->color.b, 1.0f);
                }
                for (auto bm : g_MonsterManager.bombers) {
                    if (!bm->alive) continue;
                    drawPentagon(bm->worldX, bm->worldY, Bomber::SIZE_PX,
                                 bm->color.r, bm->color.g, bm->color.b, 1.0f);
                }
                for (auto& b : g_Bullets) {
                    if (!b.active) continue;
                    drawCircle(b.x, b.y, 6.0f * b.sizeScale, b.color.r, b.color.g, b.color.b, 1.0f);
                }
                for (auto& p : g_EnemyParts) {
                    if (!p.active) continue;
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
            glScissor(
                (GLint) bwx,
                (GLint)(screenHeight - (bwy + Boss::WIN_H)),
                (GLint) Boss::WIN_W,
                (GLint) Boss::WIN_H
            );
            for (auto m : g_MonsterManager.monsters) {
                if (!m->alive) continue;
                float sz = m->summoned ? 28.0f : 18.0f;
                drawTriangle(m->worldX, m->worldY, sz,
                             m->color.r, m->color.g, m->color.b, 1.0f);
            }
            for (auto bm : g_MonsterManager.bombers) {
                if (!bm->alive) continue;
                drawPentagon(bm->worldX, bm->worldY, Bomber::SIZE_PX,
                             bm->color.r, bm->color.g, bm->color.b, 1.0f);
            }
            for (auto& b : g_Bullets) {
                if (!b.active) continue;
                drawCircle(b.x, b.y, 6.0f * b.sizeScale, b.color.r, b.color.g, b.color.b, 1.0f);
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
        glScissor(
            (GLint) playerWin.x,
            (GLint)(screenHeight - (playerWin.y + playerWin.height)),
            (GLint) playerWin.width,
            (GLint) playerWin.height
        );
        // 잡몹 (보스 소환물은 더 큼)
        for (auto m : g_MonsterManager.monsters) {
            if (!m->alive) continue;
            float sz = m->summoned ? 28.0f : 18.0f;
            drawTriangle(m->worldX, m->worldY, sz,
                         m->color.r, m->color.g, m->color.b, 1.0f);
        }
        // 자폭병
        for (auto bm : g_MonsterManager.bombers) {
            if (!bm->alive) continue;
            drawPentagon(bm->worldX, bm->worldY, Bomber::SIZE_PX,
                         bm->color.r, bm->color.g, bm->color.b, 1.0f);
            if (bm->arming) {
                drawCircle(bm->worldX, bm->worldY, bm->blastRadius,
                           1.0f, 0.2f, 0.2f, 0.10f);
            }
        }
        // 총알
        for (auto& b : g_Bullets) {
            if (!b.active) continue;
            drawCircle(b.x, b.y, 6.0f * b.sizeScale, b.color.r, b.color.g, b.color.b, 1.0f);
        }
        // 사망 파티클
        for (auto& p : g_EnemyParts) {
            if (!p.active) continue;
            float a  = p.life / p.maxLife;
            float hs = p.size * 0.5f;
            drawRect(p.x - hs, p.y - hs, p.size, p.size, p.r, p.g, p.b, a);
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
            glScissor(
                (GLint) rwx,
                (GLint)(screenHeight - (rwy + rH)),
                (GLint) rW,
                (GLint) rH
            );
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
                glScissor(
                    (GLint) twx,
                    (GLint)(screenHeight - (twy + TURRET_WIN_H)),
                    (GLint) TURRET_WIN_W,
                    (GLint) TURRET_WIN_H
                );
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
        }

        // (e3) 보스 본체 (Mercedes 모양) + HP 바
        if (g_MonsterManager.boss && g_MonsterManager.boss->alive) {
            auto* bs = g_MonsterManager.boss;

            // 보스 창 영역 scissor (플레이어 창 위에 가려도 보임)
            glEnable(GL_SCISSOR_TEST);
            float bwx = bs->worldX - Boss::WIN_W * 0.5f;
            float bwy = bs->worldY - Boss::WIN_H * 0.5f;
            glScissor(
                (GLint) bwx,
                (GLint)(screenHeight - (bwy + Boss::WIN_H)),
                (GLint) Boss::WIN_W,
                (GLint) Boss::WIN_H
            );

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
            float ax = (float)mx, ay = (float)my;
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

            // ── 메인 메뉴 ─────────────────────────────────────────
            if (st == GameState::MAIN_MENU) {
                BindMainShader();
                drawRect(0, 0, sw, sh, 0.02f, 0.02f, 0.06f, 0.92f);

                // 타이틀
                const wchar_t* TIT = T(StrId::GAME_TITLE);
                g_TextL.Draw(TIT, cx(TIT, g_TextL, 2.4f), sh*0.18f, 2.4f,
                             0.9f, 0.95f, 1.0f, 1.0f);

                // 버튼 3개
                const float BW = 320.0f, BH = 70.0f, BG = 20.0f;
                float bx = (sw - BW) * 0.5f;
                float by = sh * 0.40f;

                if (UIButton(bx, by, BW, BH, T(StrId::BTN_START),
                             mx, my, lmb, g_LmbPrev)) {
                    g_GameManager.currentState = GameState::DIFFICULTY_SELECT;
                }
                if (UIButton(bx, by + (BH+BG), BW, BH, T(StrId::BTN_SETTINGS),
                             mx, my, lmb, g_LmbPrev)) {
                    g_SettingsReturnTo = GameState::MAIN_MENU;
                    g_GameManager.currentState = GameState::SETTINGS;
                }
                if (UIButton(bx, by + (BH+BG)*2, BW, BH, T(StrId::BTN_QUIT),
                             mx, my, lmb, g_LmbPrev)) {
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
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
                        g_GameManager.maxHP    = g_Stats.maxHP;
                        g_GameManager.playerHP = g_Stats.maxHP;
                        g_PrevHP               = g_Stats.maxHP;
                        g_GameManager.currentState = GameState::READY;
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

                const float BW = 340.0f, BH = 90.0f, BG = 30.0f;
                float totalH = 3 * BH + 2 * BG;
                float bx = (sw - BW) * 0.5f;
                float by = (sh - totalH) * 0.5f;

                for (int i = 0; i < 3; i++) {
                    float y = by + i * (BH + BG);
                    bool sel = (g_Difficulty == btns[i].d);
                    if (UIButton(bx, y, BW, BH, T(btns[i].label),
                                 mx, my, lmb, g_LmbPrev, sel)) {
                        g_Difficulty = btns[i].d;
                        ResetForNewGame();
                        PickRandomWeapons(g_WeaponChoices);
                        g_GameManager.currentState = GameState::WEAPON_SELECT;
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
                        ? L"★ 크리에이티브  [ON]"
                        : L"☆ 크리에이티브  [OFF]";
                    float cby = by + 3 * (BH + BG) + 20.0f;
                    float cbw = BW, cbh = 52.0f;
                    float cbx = (sw - cbw) * 0.5f;
                    if (UIButton(cbx, cby, cbw, cbh, CLBL,
                                 mx, my, lmb, g_LmbPrev, g_CreativeMode)) {
                        g_CreativeMode = !g_CreativeMode;
                    }
                    // 설명 한 줄
                    const wchar_t* CDESC = g_CreativeMode
                        ? L"시작 시 10만점 · F키로 증강 획득 · 디버프 없음"
                        : L"10만점부터 시작 (보스 즉시) + F키 증강";
                    float cdw = g_TextS.Width(CDESC, 0.78f);
                    g_TextS.Draw(CDESC, cbx + (cbw - cdw) * 0.5f,
                                 cby + cbh - 20.0f, 0.78f,
                                 0.85f, 0.95f, 0.6f, 0.80f);
                }

                // 뒤로 버튼
                if (UIButton(40.0f, sh - 80.0f, 180.0f, 56.0f, T(StrId::BTN_BACK),
                             mx, my, lmb, g_LmbPrev)) {
                    g_GameManager.currentState = GameState::MAIN_MENU;
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
                        if (g_Language != langOpts[i].lang) {
                            g_Language = langOpts[i].lang;
                            // 폰트 재설정 (캐시도 갈아엎음)
                            g_TextL.Cleanup();
                            g_TextS.Cleanup();
#ifdef _WIN32
                            const char* nf = LanguageFace(g_Language);
                            g_TextL.InitWithFace(nf, 36, screenWidth, screenHeight);
                            g_TextS.InitWithFace(nf, 22, screenWidth, screenHeight);
#else
                            const char* chain[3];
                            int nc = LanguageFontChain(g_Language, chain);
                            g_TextL.InitFromFiles(chain, nc, 36, screenWidth, screenHeight);
                            g_TextS.InitFromFiles(chain, nc, 22, screenWidth, screenHeight);
#endif
                        }
                    }
                }

                // 크로스헤어 토글
                lineY = sh * 0.62f;
                g_TextS.Draw(T(StrId::SET_CROSSHAIR), 60.0f, lineY, 1.0f, 1,1,1,0.9f);
                if (UIButton(300.0f, lineY - 14.0f, 180.0f, 54.0f,
                             T(StrId::OPT_ON), mx, my, lmb, g_LmbPrev,
                             g_ShowCrosshair)) {
                    g_ShowCrosshair = true;
                }
                if (UIButton(500.0f, lineY - 14.0f, 180.0f, 54.0f,
                             T(StrId::OPT_OFF), mx, my, lmb, g_LmbPrev,
                             !g_ShowCrosshair)) {
                    g_ShowCrosshair = false;
                }

                // 뒤로 버튼 — 진입 시점의 상태로 복귀 (MAIN_MENU 또는 PAUSED)
                if (UIButton(40.0f, sh - 80.0f, 180.0f, 56.0f, T(StrId::BTN_BACK),
                             mx, my, lmb, g_LmbPrev)) {
                    g_GameManager.currentState = g_SettingsReturnTo;
                }
            }
            else if (st == GameState::READY) {
                const wchar_t* T1 = T(StrId::PRESS_SPACE_TO_START);
                const wchar_t* T2 = T(StrId::ESC_QUIT);
                g_TextL.Draw(T1, cx(T1, g_TextL, 1.0f), sh*0.45f, 1.0f, 1,1,1,0.95f);
                g_TextS.Draw(T2, cx(T2, g_TextS, 1.0f), sh*0.53f, 1.0f, 0.8f,0.8f,0.8f,0.8f);
            }
            else if (st == GameState::PAUSED) {
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

        // 마우스 click edge 감지용 — 이번 프레임 상태를 저장
        g_LmbPrev = lmb;

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
