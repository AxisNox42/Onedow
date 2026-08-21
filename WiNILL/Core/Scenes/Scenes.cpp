#include "Scenes.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "GameManager.h"
#include "GameContext.h"
#include "SceneSkills.h"
#include "SceneUI.h"
#include "UiLayout.h"
#include "WindowChrome.h"
#include "Settings.h"
#include "Translations.h"
#include "TutorialText.h"
#include "Meta.h"
#include "Achievements.h"
#include "Codex.h"
#include "Augment.h"
#include "PlayerStats.h"
#include "Weapons.h"
#include "ExpSystem.h"
#include "SaveSystem.h"
#include "DrawPrim.h"
#include "MainShader.h"
#include "TextRenderer.h"
#include "IconSystem.h"
#include "../System/SystemInfo.h"
#include "Camera.h"
#include "EntityDraw.h"
#include "Monster.h"
#include "RunIntermission.h"
#include "BossDirector.h"
#include "AugmentSlots.h"
#include <algorithm>
#include <vector>
#include <string>
#include <cmath>
#include <functional>

constexpr float SCENE_BG_ALPHA = 0.0f;
static int   g_RunConfigStep   = 0;   // 0=loadout, 1=trials/summary
static float g_RunConfigEntryT = 0.0f;
static float g_RunConfigFocusT[3] = { 0.0f, 0.0f, 0.0f };
static float g_SettingsEntryT  = 0.0f;
static float g_CodexEntryT     = 0.0f;
static float s_RadarCur[6]     = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
static float s_StatGlitchT     = 1.0f;
static bool  s_TrialsEnabled   = false;
static bool  s_TrialSelectPage = false;
static int   s_TrialTargetCount = 1;
static float s_TrialHover[TRIAL_SLOT_COUNT] = {};
static float s_PanelFadeT      = 1.0f;   // 0→1: 우측 패널 콘텐츠 페이드인
static int   s_PanelPrevSel    = -1;     // 마지막으로 표시된 무기 인덱스

static float UiApproach(float v, float target, float dt, float speed) {
    float k = dt * speed;
    if (k > 1.0f) k = 1.0f;
    return v + (target - v) * k;
}

static void ResetRunConfigUi() {
    g_RunConfigStep = 0;
    g_Difficulty = Difficulty::NORMAL;
    g_RunConfigEntryT = 0.0f;
    g_RunConfigFocusT[0] = g_RunConfigFocusT[1] = g_RunConfigFocusT[2] = 0.0f;
    for (int i = 0; i < 6; ++i) s_RadarCur[i] = 0.0f;
    s_StatGlitchT = 1.0f;
    s_TrialsEnabled = false;
    s_TrialSelectPage = false;
    s_TrialTargetCount = 1;
    for (int i = 0; i < TRIAL_SLOT_COUNT; ++i) s_TrialHover[i] = 0.0f;
    s_PanelFadeT   = 1.0f;
    s_PanelPrevSel = -1;
    ResetTrials();
}

static void ResetSettingsUi() {
    g_SettingsEntryT = 0.0f;
}

static void ResetCodexUi() {
    g_CodexEntryT = 0.0f;
}

// 메인메뉴·난이도선택 공용 앰비언트 배경 (파티클 + 스캔라인 + 비네트)
static void DrawMenuBackground(float sw, float sh, float delta) {
    float dtp = delta; if (dtp > 0.05f) dtp = 0.05f;
    BindMainShader();

    // 딥 네이비 반투명 — 배경화면이 비치도록
    drawRect(0, 0, sw, sh, 0.04f, 0.06f, 0.14f, 0.20f);

    struct AP { float x, y, vx, vy, sz, tw; };
    static AP a_ps[55]; static bool ap_init = false;
    if (!ap_init) { ap_init = true;
        for (int i = 0; i < 55; i++) {
            a_ps[i].x = (float)(rand()%(int)sw); a_ps[i].y = (float)(rand()%(int)sh);
            float ang = (rand()%628)*0.01f, spd = 5.0f + (rand()%16);
            a_ps[i].vx = cosf(ang)*spd; a_ps[i].vy = sinf(ang)*spd;
            a_ps[i].sz = 1.2f + (rand()%26)*0.1f; a_ps[i].tw = (rand()%628)*0.01f;
        }
    }
    // 성운 3색 파티클 (바이올렛 / 마젠타 / 차가운 별빛)
    static const float kNebR[3] = { 0.42f, 0.92f, 0.72f };
    static const float kNebG[3] = { 0.28f, 0.22f, 0.82f };
    static const float kNebB[3] = { 1.00f, 0.78f, 1.00f };
    for (int i = 0; i < 55; i++) { AP& p = a_ps[i];
        p.x += p.vx*dtp; p.y += p.vy*dtp;
        if (p.x < -8) p.x = sw+8; if (p.x > sw+8) p.x = -8;
        if (p.y < -8) p.y = sh+8; if (p.y > sh+8) p.y = -8;
        p.tw += dtp*1.6f;
        int ci = i % 3;
        drawCircle(p.x, p.y, p.sz, kNebR[ci], kNebG[ci], kNebB[ci], 0.12f + 0.10f*sinf(p.tw));
    }
    // 성운 보라 스캔라인
    for (float yy = 0.0f; yy < sh; yy += 4.0f)
        drawRect(0.0f, yy, sw, 1.0f, 0.35f, 0.20f, 0.85f, 0.018f);
    // 상하 비네트
    drawRect(0, 0, sw, 110.0f, 0.0f, 0.0f, 0.0f, 0.28f);
    drawRect(0, sh - 90.0f, sw, 90.0f, 0.0f, 0.0f, 0.0f, 0.28f);
}

void Scene_MainMenu(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    GLFWwindow* window = c.window;
    const GameState st = g_GameManager.currentState; (void)st;
    (void)*c.fireTimer;
    int li2 = LangIndex();
    bool booting = (g_BootAnim > 0.0f);
    float uiA = 1.0f - g_FadeAlpha; if (uiA < 0.0f) uiA = 0.0f;

    DrawMenuBackground(sw, sh, delta);

    // ── 인트로 애니메이션 상태 ─────────────────────────────────────
    static float s_introT = 0.0f;
    constexpr float kTitleDur  = 0.55f;   // 타이틀 페이드인
    constexpr float kBtnDur    = 0.45f;   // 버튼 1개 확장 시간
    constexpr float kBtnStag   = 0.09f;   // 버튼 간 시작 지연
    constexpr float kTextStart = kTitleDur + 4.0f * kBtnStag + kBtnDur;  // 텍스트 시작 (5개 기준)
    constexpr float kTextDur   = 0.35f;
    constexpr float kIntroDone = kTextStart + kTextDur;

    bool introWasActive = (s_introT < kIntroDone);
    if (!booting && lmb && !g_LmbPrev && introWasActive)
        s_introT = kIntroDone;   // 클릭 시 즉시 스킵
    if (!booting && s_introT < kIntroDone)
        s_introT = std::min(s_introT + delta, kIntroDone);

    const bool introActive = (s_introT < kIntroDone);

    float titlePhA = std::min(s_introT / kTitleDur, 1.0f);
    const float titleA = Smoothstep(titlePhA) * uiA;

    float textPhA  = (s_introT > kTextStart)
        ? std::min((s_introT - kTextStart) / kTextDur, 1.0f) : 0.0f;
    const float textA  = Smoothstep(textPhA) * uiA;

    // ── 상단 중앙 로고 ──
    {
        const wchar_t* TITLE = T(StrId::GAME_TITLE);
        float titleSc = 1.05f;
        g_TextXL.Draw(TITLE, CenterTextX(sw, g_TextXL, TITLE, titleSc),
                      sh * 0.10f, titleSc, 0.72f, 0.82f, 1.0f, 0.96f * titleA);
    }

    // ── 중앙 버튼 목록 (Mindustry + Rain World 하이브리드) ──
    struct SBtnDef { const wchar_t* label[3]; float ir, ig, ib; };
    static const SBtnDef kBtns[] = {
        { { L"시작",      L"Start",    L"スタート"  }, 0.35f, 0.90f, 0.55f },
        { { L"상점",      L"Shop",     L"ショップ"  }, 0.95f, 0.80f, 0.25f },
        { { L"도감",      L"Codex",    L"図鑑"      }, 0.45f, 0.72f, 0.95f },
        { { L"설정",      L"Setting",  L"設定"      }, 0.75f, 0.75f, 0.85f },
        { { L"게임 종료", L"Quit",     L"終了"      }, 0.90f, 0.38f, 0.35f },
    };
    static float kHoverT[5] = {};
    const int   kBtnCount = 5;
    const float BW = 450.0f, BH = 70.0f, BGAP = 30.0f;
    const float HOVER_DW = 30.0f;  // 호버 시 가로만 확장
    float totalBH = kBtnCount * BH + (kBtnCount - 1) * BGAP;
    float btnX0   = (sw - BW) * 0.5f;
    // 로고 아래부터 시작 — 화면 남은 공간 중앙
    float logoBot = sh * 0.10f + g_TextXL.Height(T(StrId::GAME_TITLE), 1.05f) + 36.0f;
    float btnY0   = logoBot + ((sh - logoBot) - totalBH) * 0.5f;

    // 가로 육각형 채우기 (fan 분해)
    auto hexFill = [&](float hx, float hy, float hW, float hH,
                        float r, float g, float b, float a) {
        float cut = hH * 0.38f;
        float cx = hx + hW * 0.5f, cy = hy + hH * 0.5f;
        float vx[6] = { hx+cut, hx+hW-cut, hx+hW,      hx+hW-cut, hx+cut,  hx     };
        float vy[6] = { hy,     hy,         hy+hH*0.5f, hy+hH,     hy+hH,   hy+hH*0.5f };
        for (int k = 0; k < 6; k++) {
            int n = (k + 1) % 6;
            BatchTri(cx, cy, vx[k], vy[k], vx[n], vy[n], r, g, b, a);
        }
    };
    // 가로 육각형 테두리 (각 변을 얇은 쿼드로)
    auto hexBorder = [&](float hx, float hy, float hW, float hH,
                          float r, float g, float b, float a, float BL) {
        float cut = hH * 0.38f;
        float vx[6] = { hx+cut, hx+hW-cut, hx+hW,      hx+hW-cut, hx+cut,  hx     };
        float vy[6] = { hy,     hy,         hy+hH*0.5f, hy+hH,     hy+hH,   hy+hH*0.5f };
        for (int k = 0; k < 6; k++) {
            int  n  = (k + 1) % 6;
            float ex = vx[n]-vx[k], ey = vy[n]-vy[k];
            float len = sqrtf(ex*ex + ey*ey); if (len < 1.0f) continue;
            float nx = -ey/len * BL*0.5f, ny = ex/len * BL*0.5f;
            BatchTri(vx[k]-nx, vy[k]-ny, vx[n]-nx, vy[n]-ny, vx[n]+nx, vy[n]+ny, r,g,b,a);
            BatchTri(vx[k]-nx, vy[k]-ny, vx[n]+nx, vy[n]+ny, vx[k]+nx, vy[k]+ny, r,g,b,a);
        }
    };
    // 육각형 내부 히트테스트
    auto hexHit = [&](float px, float py, float hx, float hy, float hW, float hH) -> bool {
        if (px < hx || px > hx+hW || py < hy || py > hy+hH) return false;
        float cut = hH * 0.38f;
        float d = fabsf(py - (hy + hH * 0.5f)) / (hH * 0.5f);
        return px >= hx + cut*d && px <= hx + hW - cut*d;
    };

    for (int i = 0; i < kBtnCount; i++) {
        float baseBx = btnX0, baseBy = btnY0 + i * (BH + BGAP);
        // 인트로 중엔 호버 비활성 + 스킵 클릭도 버튼 동작 안 함
        bool hov = (!booting && !introActive && !introWasActive && g_FadeDir == 0
                    && hexHit((float)mx, (float)my, baseBx, baseBy, BW, BH));

        // 호버 애니메이션 t (0→1)
        float spd = delta * 8.0f; if (spd > 1.0f) spd = 1.0f;
        kHoverT[i] += ((hov ? 1.0f : 0.0f) - kHoverT[i]) * spd;
        float t  = kHoverT[i];

        // 버튼 확장 애니메이션: 중앙 사각형 → 좌우로 늘어나며 육각형
        float rawBtnPh = (s_introT - kTitleDur - (float)i * kBtnStag) / kBtnDur;
        rawBtnPh = std::max(0.0f, std::min(rawBtnPh, 1.0f));
        float btnExpand = Smoothstep(rawBtnPh);
        float animBW = std::max(2.0f, BW * btnExpand);
        float animBX = baseBx + (BW - animBW) * 0.5f;

        float dw = HOVER_DW * t;
        float bx = animBX - dw * 0.5f, by = baseBy;
        float bW = animBW + dw,         bH = BH;

        BindMainShader();

        // 호버 글로우 (본체 뒤, 3단 확산) — 바이올렛
        if (t > 0.01f) {
            const float GR = 0.42f, GG = 0.22f, GB = 0.95f;
            hexFill(bx-14, by-7,   bW+28, bH+14, GR, GG, GB, 0.035f * t * uiA);
            hexFill(bx- 8, by-4,   bW+16, bH+ 8, GR, GG, GB, 0.065f * t * uiA);
            hexFill(bx- 3, by-1.5f,bW+ 6, bH+ 3, GR, GG, GB, 0.10f  * t * uiA);
        }

        // 육각형 본체 — 딥 네이비 틴트
        hexFill(bx, by, bW, bH, 0.055f, 0.062f, 0.130f, uiA);

        // 테두리: 다크 바이올렛 → 마젠타 lerp
        float bcR = 0.32f + (0.92f - 0.32f) * t;
        float bcG = 0.22f + (0.22f - 0.22f) * t;
        float bcB = 0.62f + (0.78f - 0.62f) * t;
        float bcA = 0.32f + (0.90f - 0.32f) * t;
        hexBorder(bx, by, bW, bH, bcR, bcG, bcB, bcA * uiA, 3.0f);

        // 라벨 중앙 정렬
        float labelSc = 0.95f;
        const wchar_t* lbl = kBtns[i].label[li2];
        float labelX  = bx + (bW - g_TextL.Width(lbl, labelSc)) * 0.5f;
        float labelY  = by + (bH - g_TextL.Height(lbl, labelSc)) * 0.5f;
        g_TextL.Draw(lbl, labelX, labelY, labelSc,
                     1.0f, 1.0f, 1.0f, (hov ? 1.0f : 0.72f) * textA);

        // 클릭
        if (hov && lmb && !g_LmbPrev) {
            switch (i) {
            case 0:
                ResetRunConfigUi();
                StartSceneFade(GameState::RUN_CONFIG);
                break;
            case 1: g_GameManager.currentState = GameState::SHOP;    break;
            case 2: ResetCodexUi(); g_GameManager.currentState = GameState::CODEX; break;
            case 3:
                g_SettingsReturnTo = GameState::MAIN_MENU;
                ResetSettingsUi();
                g_GameManager.currentState = GameState::SETTINGS;
                break;
            case 4: glfwSetWindowShouldClose(window, GLFW_TRUE);     break;
            }
        }
    }

    // ── 실행(부팅) 스플래시 — 앱 아이콘 클릭 시 창이 열리며 로딩 로그 ──
                if (g_BootAnim > 0.0f) {
                    g_BootAnim -= delta;
                    float prog = 1.0f - g_BootAnim / BOOT_DUR;        // 0→1
                    if (prog < 0.0f) prog = 0.0f; if (prog > 1.0f) prog = 1.0f;
                    float ease = prog < 0.25f ? (prog / 0.25f) : 1.0f; // 창 열림 0~25%
                    float ar = g_BootAr, ag = g_BootAg, ab = g_BootAb;
                    float WW = 520.0f, WH = 300.0f;
                    float cw = WW * ease;              // 크기 0 → 지정 크기
                    float chh= WH * ease;
                    float wx = sw * 0.5f - cw * 0.5f;
                    float wy = sh * 0.5f - chh * 0.5f;
                    BindMainShader();
                    drawRect(0, 0, sw, sh, 0.0f, 0.0f, 0.0f, 0.45f * ease * SCENE_BG_ALPHA);  // 배경 딤
                    // 시네마틱 — 아래로 훑는 스캔 스윕 라인 (화이트 플래시는 눈 아파서 제거)
                    {
                        float sweepY = fmodf(prog * 1.3f, 1.0f) * sh;
                        drawRect(0, sweepY, sw, 2.0f, ar, ag, ab, 0.30f * ease);
                        drawRect(0, sweepY - 40.0f, sw, 40.0f, ar, ag, ab, 0.05f * ease);
                    }
                    drawRect(wx, wy, cw, chh, 0.06f, 0.07f, 0.10f, 0.99f);   // 창 본체
                    drawRect(wx, wy, cw, 28.0f, ar*0.55f, ag*0.55f, ab*0.6f, 1.0f); // 타이틀바
                    drawRect(wx, wy+28.0f, cw, 2.0f, ar, ag, ab, 0.9f);      // 강조 라인
                    drawRect(wx+cw-22, wy+8, 12, 12, 0.9f, 0.25f, 0.25f, 0.95f); // [X]
                    if (ease > 0.9f) {
                        g_TextS.Draw(g_BootName, wx + 12.0f, wy + 6.0f, 0.6f,
                                     0.95f, 0.97f, 1.0f, 1.0f);
                        // 부팅 로그 — 진행도에 따라 한 줄씩 나타남
                        static const wchar_t* LOG[6] = {
                            L"> mounting modules ...",
                            L"> loading assets        [ OK ]",
                            L"> init renderer         [ OK ]",
                            L"> linking onedow.dll    [ OK ]",
                            L"> verify save data      [ OK ]",
                            L"> ready." };
                        int shown = (int)(prog * 6.5f); if (shown > 6) shown = 6;
                        for (int i = 0; i < shown; i++) {
                            bool last = (i == 5);
                            g_TextS.Draw(LOG[i], wx + 22.0f, wy + 44.0f + i * 24.0f, 0.6f,
                                         last ? ag : 0.65f, last ? 1.0f : 0.78f,
                                         last ? ag : 0.7f, 0.95f);
                        }
                        // 진행 바 (창 하단)
                        float barW = cw - 44.0f, barX = wx + 22.0f, barY = wy + chh - 30.0f;
                        BindMainShader();
                        drawRect(barX, barY, barW, 14.0f, 0.12f, 0.14f, 0.20f, 1.0f);
                        drawRect(barX, barY, barW * prog, 14.0f, ar, ag, ab, 1.0f);
                        wchar_t pct[16]; swprintf_s(pct, L"%d%%", (int)(prog * 100.0f));
                        g_TextS.Draw(pct, barX + barW - 44.0f, barY - 22.0f, 0.6f,
                                     0.8f, 0.9f, 1.0f, 1.0f);
                    }
                    if (g_BootAnim <= 0.0f) {
                        g_BootAnim = 0.0f;
                        g_GameManager.currentState = g_BootTarget;
                    }
                }
}

void Scene_Shop(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    GLFWwindow* window = c.window;
    const GameState st = g_GameManager.currentState;
    float& fireTimer = *c.fireTimer;
    const std::function<void()>& ResetForNewGame = c.reset;
                const float WW = std::min(sw * 0.88f, 840.0f);
                const float WH = std::min(sh * 0.88f, 920.0f);
                float wx, wy;
                DrawMenuBackground(sw, sh, delta);
                SceneAppWindow(sw, sh, WW, WH, L"shop.exe", 1.0f, 0.80f, 0.20f, wx, wy, false, 0.0f);
                if (g_AppOpen >= 0.999f) {           // 완전히 열린 뒤에만 콘텐츠

                const wchar_t* TIT = T(StrId::BTN_SHOP);
                float titSc = 1.3f;
                float titleY = wy + 48.0f;
                float tw0 = g_TextL.Width(TIT, titSc);
                g_TextL.Draw(TIT, wx + (WW - tw0) * 0.5f, titleY, titSc, 1, 1, 1, 1);
                wchar_t cbuf[48]; swprintf_s(cbuf, L"COIN  %lld", g_Coins);
                float coinSc = 1.0f;
                float coinY = titleY + g_TextL.Height(TIT, titSc) + 18.0f;
                float cw0 = g_TextL.Width(cbuf, coinSc);
                g_TextL.Draw(cbuf, wx + (WW - cw0) * 0.5f, coinY, coinSc, 1.0f, 0.9f, 0.3f, 1.0f);

                const float RW = 640.0f, RH = 56.0f, RG = 10.0f;
                float rx = wx + (WW - RW) * 0.5f;
                float ry0 = coinY + g_TextL.Height(cbuf, coinSc) + 28.0f;
                for (int i = 0; i < META_COUNT; i++) {
                    float ry = ry0 + i * (RH + RG);
                    BindMainShader();
                    drawRect(rx, ry, RW, RH, 0.07f, 0.07f, 0.11f, 0.9f);
                    wchar_t nm[96];
                    swprintf_s(nm, L"%ls   Lv %d/%d", MetaName(i), g_MetaLv[i], META_DEFS[i].maxLv);
                    g_TextS.Draw(nm, rx + 16.0f, ry + 16.0f, 0.95f, 0.9f, 0.95f, 1.0f, 1.0f);
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
                            BindMainShader();
                            drawRect(btX, ry + 6.0f, 154.0f, RH - 12.0f, 0.18f, 0.06f, 0.06f, 0.9f);
                            float tw = g_TextS.Width(bb, 0.9f);
                            g_TextS.Draw(bb, btX + (154.0f - tw) * 0.5f, ry + 18.0f, 0.9f,
                                         0.95f, 0.4f, 0.4f, 1.0f);
                        }
                    }
                }

                // ── 액센트 테마 (네온 색) 코스메틱 — 스와치 행 ──
                float themeBottom;
                {
                    int li4 = LangIndex();
                    const wchar_t* TTIT[3] = { L"테마  (창 네온 색)", L"Theme  (window neon)", L"テーマ  (窓ネオン)" };
                    float ty0 = ry0 + META_COUNT * (RH + RG) + 16.0f;
                    BindMainShader();
                    g_TextS.Draw(TTIT[li4], rx, ty0, 1.0f, 0.8f, 0.9f, 1.0f, 1.0f);
                    float swY = ty0 + 30.0f;
                    float gap = 8.0f;
                    float swW = (RW - gap * (ACCENT_COUNT - 1)) / (float)ACCENT_COUNT;
                    float swH = 64.0f;
                    for (int i = 0; i < ACCENT_COUNT; i++) {
                        const AccentTheme& th = ACCENT_THEMES[i];
                        float sx = rx + i * (swW + gap);
                        bool owned = ThemeOwned(i);
                        bool sel   = (g_ThemeSel == i);
                        bool hover = (mx >= sx && mx <= sx + swW && my >= swY && my <= swY + swH);
                        bool clicked = hover && lmb && !g_LmbPrev;
                        BindMainShader();
                        // 색 스와치 (미보유는 어둡게)
                        float dim = owned ? 1.0f : 0.30f;
                        drawRect(sx, swY, swW, swH, th.r * dim, th.g * dim, th.b * dim, 1.0f);
                        // 테두리 — 선택=흰색 두껍게 / 호버=옅게
                        float br = sel ? 1.0f : (hover ? 0.85f : 0.35f);
                        float bt = sel ? 3.0f : 1.5f;
                        drawRect(sx, swY, swW, bt, br, br, br, 1.0f);
                        drawRect(sx, swY + swH - bt, swW, bt, br, br, br, 1.0f);
                        drawRect(sx, swY, bt, swH, br, br, br, 1.0f);
                        drawRect(sx + swW - bt, swY, bt, swH, br, br, br, 1.0f);
                        // 라벨 / 비용
                        if (owned) {
                            if (sel) {
                                float ew = g_TextS.Width(L"*", 0.7f);
                                g_TextS.Draw(L"*", sx + (swW - ew) * 0.5f, swY + swH * 0.5f - 10.0f,
                                             0.7f, 0.05f, 0.05f, 0.08f, 1.0f);
                            }
                        } else {
                            wchar_t cb[24]; swprintf_s(cb, L"%lld", th.cost);
                            float cwd = g_TextS.Width(cb, 0.62f);
                            g_TextS.Draw(cb, sx + (swW - cwd) * 0.5f, swY + swH * 0.5f - 9.0f,
                                         0.62f, 1.0f, 0.95f, 0.5f, 1.0f);
                        }
                        // 이름 (아래)
                        float nwd = g_TextS.Width(AccentName(i), 0.55f);
                        g_TextS.Draw(AccentName(i), sx + (swW - nwd) * 0.5f, swY + swH + 3.0f,
                                     0.55f, 0.85f, 0.9f, 0.95f, owned ? 1.0f : 0.6f);
                        // 클릭 처리 — 보유면 장착, 미보유면 코인 충분 시 구매+장착
                        if (clicked) {
                            if (owned) {
                                g_ThemeSel = i; ApplyAccentTheme(); SaveGame();
                            } else if (g_Coins >= th.cost) {
                                g_Coins -= th.cost;
                                g_ThemeOwned |= (1 << i);
                                g_ThemeSel = i; ApplyAccentTheme(); SaveGame();
                            }
                        }
                    }
                    themeBottom = swY + swH + 22.0f;
                }

                // ── 업적 목록 (테마 행 아래, 3열) ──
                {
                    int li3 = LangIndex();
                    const wchar_t* ATIT[3] = { L"업적", L"Achievements", L"実績" };
                    float ay0 = themeBottom;
                    BindMainShader();
                    g_TextS.Draw(ATIT[li3], rx, ay0, 1.0f, 0.8f, 0.9f, 1.0f, 1.0f);
                    float colW = RW / 3.0f;
                    float rowH = 28.0f;
                    for (int i = 0; i < ACH_COUNT; i++) {
                        bool got = g_AchUnlocked[i];
                        int  col = i / 4, row = i % 4;
                        float ax = rx + col * colW;
                        float ay = ay0 + 32.0f + row * rowH;
                        wchar_t ab[96];
                        swprintf_s(ab, L"%ls %ls", got ? L"*" : L"-", AchName(i));
                        if (got) g_TextS.Draw(ab, ax, ay, 0.72f, 0.5f, 0.95f, 0.55f, 1.0f);
                        else     g_TextS.Draw(ab, ax, ay, 0.72f, 0.55f, 0.55f, 0.6f, 0.9f);
                    }
                }

                if (UIButton(wx + 40.0f, wy + WH - 62.0f, 180.0f, 46.0f, T(StrId::BTN_BACK),
                             mx, my, lmb, g_LmbPrev)) {
                    g_GameManager.currentState = GameState::MAIN_MENU;
                }
                }   // close: g_AppOpen open guard
}

void Scene_Codex(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    GLFWwindow* window = c.window;
    (void)c.fireTimer; (void)c.reset;

    // ── State ─────────────────────────────────────────────────────────────
    static int   s_cat         = 0;
    static int   s_prevCat     = -1;
    static float s_catHover[3] = {};
    static float s_catFocus[3] = {};
    static float s_panelT      = 1.0f;
    static int   s_sel[3]      = { -1, -1, -1 };
    static float s_scroll[3]   = {};
    static float s_detailFadeT = 0.0f;
    static int   s_prevSel     = -2;

    DrawMenuBackground(sw, sh, delta);

    float dt = delta; if (dt > 0.05f) dt = 0.05f;
    g_CodexEntryT += dt;
    if (g_CodexEntryT > 1.0f) g_CodexEntryT = 1.0f;
    const float now = (float)glfwGetTime();

    const float wake      = Smoothstep(std::min(1.0f, g_CodexEntryT / 0.50f));
    const float rightWake = Smoothstep(std::min(1.0f,
        std::max(0.0f, g_CodexEntryT - 0.10f) / 0.44f));

    int li = LangIndex(); if (li < 0 || li >= 3) li = 0;
    const int nli = (li == 0) ? 0 : 1;

    // ── Layout ────────────────────────────────────────────────────────────
    const float TARGET_W = 1640.0f, TARGET_H = 910.0f;
    float uiS = std::max(0.72f, std::min(sw * 0.94f / TARGET_W, sh * 0.90f / TARGET_H));
    if (uiS > 1.30f) uiS = 1.30f;
    const float panelW  = TARGET_W * uiS;
    const float panelH  = TARGET_H * uiS;
    const float panelX  = (sw - panelW) * 0.5f;
    const float panelY  = (sh - panelH) * 0.5f;
    const float headerH = 86.0f * uiS;
    const float footerH = 64.0f * uiS;
    const float bodyY   = panelY + headerH;
    const float bodyH   = panelH - headerH - footerH;
    const float footY   = panelY + panelH - footerH + 14.0f * uiS;
    const float leftW   = panelW * 0.27f;
    const float colGap  = 18.0f * uiS;
    const float leftX   = panelX;
    const float rightX  = leftX + leftW + colGap;
    const float rightW  = panelW - leftW - colGap;

    // ── Category definitions ──────────────────────────────────────────────
    struct CatDef { const wchar_t* id; const wchar_t* name[2]; float r, g, b; };
    static const CatDef kCats[] = {
        { L"ENEMIES",  { L"\xC801",  L"ENEMIES"  }, 1.00f, 0.42f, 0.38f },
        { L"AUGMENTS", { L"\xC99D\xAC15", L"AUGMENTS" }, 0.38f, 0.82f, 1.00f },
        { L"BOSSES",   { L"\xBCF4\xC2A4", L"BOSSES"   }, 0.96f, 0.72f, 0.22f },
    };
    const int kCatCount = 3;
    const CatDef& cat = kCats[s_cat];

    auto hit = [&](float x, float y, float w, float h) -> bool {
        return mx >= x && mx <= x + w && my >= y && my <= y + h;
    };

    // ── Transitions ───────────────────────────────────────────────────────
    if (s_prevCat != s_cat) { s_prevCat = s_cat; s_panelT = 0.0f; }
    s_panelT = UiApproach(s_panelT, 1.0f, delta, 10.0f);

    int curSel = s_sel[s_cat];
    if (curSel != s_prevSel) { s_prevSel = curSel; s_detailFadeT = 0.0f; }
    s_detailFadeT = UiApproach(s_detailFadeT, 1.0f, delta, 8.0f);

    // ── Helper lambdas ────────────────────────────────────────────────────
    auto drawBorder = [](float x, float y, float w, float h,
                         float r, float g, float b, float a, float t) {
        drawRect(x, y, w, t, r, g, b, a);
        drawRect(x, y + h - t, w, t, r, g, b, a);
        drawRect(x, y, t, h, r, g, b, a);
        drawRect(x + w - t, y, t, h, r, g, b, a);
    };
    auto drawCenterS = [&](const wchar_t* text, float x, float y, float w,
                           float sc, float r, float g, float b, float a) {
        while (sc > 0.36f * uiS && g_TextS.Width(text, sc) > w - 12.0f * uiS)
            sc -= 0.025f * uiS;
        float tw = g_TextS.Width(text, sc);
        g_TextS.Draw(text, x + (w - tw) * 0.5f, y, sc, r, g, b, a);
    };
    auto drawFitS = [&](const wchar_t* text, float x, float y, float maxW,
                        float sc, float minSc, float r, float g, float b, float a) {
        while (sc > minSc && g_TextS.Width(text, sc) > maxW)
            sc -= 0.025f * uiS;
        g_TextS.Draw(text, x, y, sc, r, g, b, a);
    };
    auto drawCBkt = [&](float x, float y, float w, float h,
                        float r, float g, float b, float a) {
        float cL = std::min(w * 0.24f, 10.0f * uiS), ct = 1.3f * uiS;
        drawRect(x,       y,       cL, ct, r, g, b, a);
        drawRect(x,       y,       ct, cL, r, g, b, a);
        drawRect(x+w-cL,  y,       cL, ct, r, g, b, a);
        drawRect(x+w-ct,  y,       ct, cL, r, g, b, a);
        drawRect(x,       y+h-ct,  cL, ct, r, g, b, a);
        drawRect(x,       y+h-cL,  ct, cL, r, g, b, a);
        drawRect(x+w-cL,  y+h-ct,  cL, ct, r, g, b, a);
        drawRect(x+w-ct,  y+h-cL,  ct, cL, r, g, b, a);
        float ns = 3.0f * uiS;
        drawDiamond(x,   y,   ns, r, g, b, a);
        drawDiamond(x+w, y,   ns, r, g, b, a);
        drawDiamond(x,   y+h, ns, r, g, b, a);
        drawDiamond(x+w, y+h, ns, r, g, b, a);
    };

    // ── HEADER ────────────────────────────────────────────────────────────
    const float hSlide = (1.0f - wake) * 14.0f * uiS;
    BindMainShader();
    g_TextL.Draw(L"CODEX.DB",
                 leftX, panelY + 8.0f * uiS - hSlide,
                 0.88f * uiS, 1.0f, 1.0f, 1.0f, 0.98f * wake);
    g_TextS.Draw(nli == 0
        ? L"\xBC1C\xACAC\xD55C \xC801\xC640 \xC99D\xAC15\xC758 \xAE30\xB85D \xB370\xC774\xD130\xBCA0\xC774\xC2A4"
        : L"Database of discovered enemies, augments, and bosses",
                 leftX, panelY + 52.0f * uiS - hSlide,
                 0.44f * uiS, 0.48f, 0.58f, 0.78f, 0.72f * wake);

    // Category tabs (segment buttons, right side of header)
    const float tabW  = 170.0f * uiS;
    const float tabH  = 46.0f * uiS;
    const float tabGp = 10.0f * uiS;
    const float tabsX = panelX + panelW - (tabW * kCatCount + tabGp * (kCatCount - 1));
    const float tabsY = panelY + 20.0f * uiS - hSlide;

    for (int i = 0; i < kCatCount; ++i) {
        float tx = tabsX + (float)i * (tabW + tabGp);
        float ty = tabsY;
        bool hov = hit(tx, ty, tabW, tabH);
        s_catHover[i] = UiApproach(s_catHover[i], hov ? 1.0f : 0.0f, delta, 10.0f);
        s_catFocus[i] = UiApproach(s_catFocus[i], (i == s_cat) ? 1.0f : 0.0f, delta, 8.5f);
        if (hov && lmb && !g_LmbPrev && i != s_cat) s_cat = i;
        bool sel = (i == s_cat);
        const CatDef& cd = kCats[i];
        BindMainShader();
        drawRect(tx, ty, tabW, tabH,
                 0.045f + cd.r*(sel ? 0.105f : 0.030f),
                 0.052f + cd.g*(sel ? 0.085f : 0.028f),
                 0.070f + cd.b*(sel ? 0.070f : 0.024f), 0.94f * wake);
        float ba  = (sel ? 0.88f : (hov ? 0.55f : 0.24f)) * wake;
        float cL2 = std::min(tabW * 0.22f, 9.0f * uiS), ct2 = 1.3f * uiS;
        drawRect(tx,        ty,        cL2, ct2, cd.r, cd.g, cd.b, ba);
        drawRect(tx,        ty,        ct2, cL2, cd.r, cd.g, cd.b, ba);
        drawRect(tx+tabW-cL2,ty,       cL2, ct2, cd.r, cd.g, cd.b, ba);
        drawRect(tx+tabW-ct2,ty,       ct2, cL2, cd.r, cd.g, cd.b, ba);
        drawRect(tx,        ty+tabH-ct2,cL2, ct2, cd.r, cd.g, cd.b, ba);
        drawRect(tx,        ty+tabH-cL2,ct2, cL2, cd.r, cd.g, cd.b, ba);
        drawRect(tx+tabW-cL2,ty+tabH-ct2,cL2,ct2, cd.r, cd.g, cd.b, ba);
        drawRect(tx+tabW-ct2,ty+tabH-cL2,ct2,cL2, cd.r, cd.g, cd.b, ba);
        float ns2 = (sel ? 4.5f : 3.0f) * uiS;
        drawDiamond(tx,      ty,      ns2, cd.r, cd.g, cd.b, ba);
        drawDiamond(tx+tabW, ty,      ns2, cd.r, cd.g, cd.b, ba);
        drawDiamond(tx,      ty+tabH, ns2, cd.r, cd.g, cd.b, ba);
        drawDiamond(tx+tabW, ty+tabH, ns2, cd.r, cd.g, cd.b, ba);
        g_TextS.Draw(cd.name[nli], tx + 10.0f*uiS, ty + 5.0f*uiS,
                     0.36f*uiS, 1.0f, 1.0f, 1.0f, (sel ? 0.82f : 0.52f) * wake);
        drawCenterS(cd.id, tx, ty + 20.0f*uiS, tabW, 0.48f*uiS,
                    sel ? 1.0f : 0.72f, sel ? 1.0f : 0.80f, sel ? 1.0f : 0.92f,
                    (sel ? 0.96f : 0.74f) * wake);
    }

    // ── LEFT PANEL ────────────────────────────────────────────────────────
    float panelE = Smoothstep(s_panelT);

    BindMainShader();
    drawRect(panelX + 8.0f*uiS, bodyY + 10.0f*uiS, leftW, bodyH,
             0.0f, 0.0f, 0.0f, 0.14f * wake);
    drawRect(panelX, bodyY, leftW, bodyH,
             0.025f + cat.r*0.010f, 0.032f + cat.g*0.008f, 0.052f + cat.b*0.010f,
             0.88f * wake);
    drawRect(panelX, bodyY, leftW, 28.0f*uiS,
             cat.r*0.20f, cat.g*0.20f, cat.b*0.22f, 0.94f * wake);
    drawRect(panelX, bodyY + 28.0f*uiS, leftW, 1.5f*uiS,
             cat.r, cat.g, cat.b, 0.28f * panelE * wake);
    drawBorder(panelX, bodyY, leftW, bodyH, cat.r, cat.g, cat.b,
               (0.16f + 0.30f*panelE) * wake, 1.2f*uiS);
    if (wake > 0.02f) {
        BatchFlush(); SetGlowFx(true);
        drawConstellFrame(panelX, bodyY, leftW, bodyH,
                          cat.r, cat.g, cat.b, 0.12f * panelE * wake,
                          14.0f*uiS, 3.5f*uiS, 0.12f, wake);
        BatchFlush(); SetGlowFx(false);
    }
    BindMainShader();
    g_TextS.Draw(cat.name[nli], panelX + 10.0f*uiS, bodyY + 7.0f*uiS,
                 0.40f*uiS, 1.0f, 1.0f, 1.0f, 0.65f * wake);
    g_TextL.Draw(cat.id, panelX + 10.0f*uiS, bodyY + 30.0f*uiS,
                 0.54f*uiS, cat.r, cat.g, cat.b, 0.80f * panelE * wake);

    // List layout constants
    const float listX     = panelX + 10.0f * uiS;
    const float listW2    = leftW  - 20.0f * uiS;
    const float listTopY  = bodyY  + 68.0f * uiS;
    const float listBotY  = bodyY  + bodyH - 12.0f * uiS;
    const float listViewH = listBotY - listTopY;
    const float itemH     = 30.0f * uiS;
    const float grpH      = 22.0f * uiS;

    // Build item list for current category
    struct ListItem {
        bool           isGroup;
        int            dataIdx;
        const wchar_t* label;
        bool           seen;
        float          r, g, b;
    };
    static ListItem s_items[512];
    int nItems = 0;
    float contentH = 0.0f;

    if (s_cat == 0) {
        struct MobGroup { const wchar_t* name; int ids[9]; int count; float r, g, b; };
        static const MobGroup MGRPS[] = {
            { L"BASIC",
              { CM_NORMAL, CM_SPLITTER, CM_BLINKER, CM_CHARGER,
                CM_WEAVER, CM_BRUTE, CM_ORBITER, CM_SPAWNER, CM_SHIELDED },
              9, 0.88f, 0.52f, 0.52f },
            { L"EXTENDED",
              { CM_RANGED, CM_BOMBER, CM_DDOS, CM_BADSECTOR, CM_REGERROR, 0, 0, 0, 0 },
              5, 0.72f, 0.48f, 0.90f },
        };
        for (int gi = 0; gi < 2; ++gi) {
            const MobGroup& mg = MGRPS[gi];
            ListItem& lh = s_items[nItems++];
            lh.isGroup = true; lh.dataIdx = -1; lh.seen = true;
            lh.label = mg.name; lh.r = mg.r; lh.g = mg.g; lh.b = mg.b;
            contentH += grpH;
            for (int j = 0; j < mg.count; ++j) {
                int id = mg.ids[j];
                ListItem& litem = s_items[nItems++];
                litem.isGroup = false; litem.dataIdx = id;
                litem.seen = CodexMobSeen(id);
                litem.label = litem.seen ? MobName(id) : L"???";
                litem.r = mg.r; litem.g = mg.g; litem.b = mg.b;
                contentH += itemH;
            }
        }
    } else if (s_cat == 1) {
        int sorted[AUG_TOTAL], ns_aug = 0;
        for (int i = 0; i < AUG_TOTAL; ++i)
            if (!AugRemoved(ALL_AUGS[i].type)) sorted[ns_aug++] = i;
        std::sort(sorted, sorted + ns_aug,
                  [](int a, int b) { return AugTierIndexLess(a, b); });
        AugRarity prevR = (AugRarity)-1;
        for (int k = 0; k < ns_aug; ++k) {
            int i = sorted[k];
            AugRarity rar = ALL_AUGS[i].rarity;
            if (rar != prevR) {
                float rr, rg, rb; GetRarityColor(rar, rr, rg, rb);
                ListItem& lh = s_items[nItems++];
                lh.isGroup = true; lh.dataIdx = -1; lh.seen = true;
                lh.label = GetRarityKR(rar); lh.r = rr; lh.g = rg; lh.b = rb;
                prevR = rar; contentH += grpH;
            }
            float rr, rg, rb; GetRarityColor(ALL_AUGS[i].rarity, rr, rg, rb);
            ListItem& litem = s_items[nItems++];
            litem.isGroup = false; litem.dataIdx = i;
            litem.seen = CodexAugSeen(i);
            litem.label = litem.seen ? AugName(ALL_AUGS[i]) : L"???";
            litem.r = rr; litem.g = rg; litem.b = rb;
            contentH += itemH;
        }
    } else {
        ListItem& lh = s_items[nItems++];
        lh.isGroup = true; lh.dataIdx = -1; lh.seen = true;
        lh.label = L"ACTIVE ROSTER";
        lh.r = cat.r; lh.g = cat.g; lh.b = cat.b;
        contentH += grpH;
        for (int i = 0; i < BOSS_CODEX_COUNT; ++i) {
            int pick = BossCodexPick(i);
            glm::vec3 wc = BossDir::WarnColor(pick);
            ListItem& litem = s_items[nItems++];
            litem.isGroup = false; litem.dataIdx = i;
            litem.seen = BossCodexSeen(i);
            litem.label = litem.seen ? BossCodexName(i) : L"???";
            litem.r = wc.r; litem.g = wc.g; litem.b = wc.b;
            contentH += itemH;
        }
    }

    // Scroll
    float& scroll = s_scroll[s_cat];
    float maxScroll = std::max(0.0f, contentH - listViewH);
    bool overList = (mx >= listX && mx <= listX + listW2 &&
                     my >= listTopY && my <= listBotY);
    if (overList && g_ScrollAccum != 0.0f)
        scroll -= g_ScrollAccum * itemH * 1.3f;
    g_ScrollAccum = 0.0f;
    scroll = std::max(0.0f, std::min(scroll, maxScroll));

    // Render list (scissored)
    BatchFlush();
    glEnable(GL_SCISSOR_TEST);
    glScissor((GLint)listX, (GLint)(sh - listBotY), (GLint)listW2, (GLint)listViewH);

    float ry = listTopY - scroll;
    for (int k = 0; k < nItems; ++k) {
        const ListItem& litem = s_items[k];
        float ih = litem.isGroup ? grpH : itemH;
        if (ry + ih < listTopY) { ry += ih; continue; }
        if (ry > listBotY)      break;

        if (litem.isGroup) {
            BindMainShader();
            float mid = ry + grpH * 0.5f;
            drawRect(listX, mid - 0.75f*uiS, listW2, 1.5f*uiS,
                     litem.r, litem.g, litem.b, 0.18f * wake);
            drawDiamond(listX,          mid, 4.0f*uiS, litem.r, litem.g, litem.b, 0.60f * wake);
            drawDiamond(listX + listW2, mid, 4.0f*uiS, litem.r, litem.g, litem.b, 0.60f * wake);
            g_TextS.Draw(litem.label, listX + 14.0f*uiS, ry + 4.0f*uiS,
                         0.38f*uiS, litem.r, litem.g, litem.b, 0.82f * wake);
        } else {
            bool isSel = (s_sel[s_cat] == litem.dataIdx);
            bool hov2  = overList && hit(listX, ry, listW2, itemH);
            if (hov2 && lmb && !g_LmbPrev && litem.seen)
                s_sel[s_cat] = litem.dataIdx;

            float ia = wake * (0.65f + 0.35f * panelE);
            BindMainShader();
            drawRect(listX, ry, listW2, itemH,
                     0.040f + litem.r*(isSel ? 0.080f : hov2 ? 0.030f : 0.010f),
                     0.048f + litem.g*(isSel ? 0.065f : hov2 ? 0.024f : 0.008f),
                     0.068f + litem.b*(isSel ? 0.052f : hov2 ? 0.020f : 0.006f),
                     (isSel ? 0.94f : hov2 ? 0.80f : 0.64f) * ia);
            drawRect(listX, ry, 3.5f*uiS, itemH, litem.r, litem.g, litem.b,
                     (isSel ? 0.90f : hov2 ? 0.52f : 0.24f) * ia);
            if (isSel || hov2)
                drawCBkt(listX, ry, listW2, itemH, litem.r, litem.g, litem.b,
                         (isSel ? 0.78f : 0.32f) * ia);
            g_TextS.Draw(litem.label, listX + 14.0f*uiS, ry + 7.0f*uiS, 0.42f*uiS,
                         litem.seen ? (isSel ? 1.0f : hov2 ? 0.92f : 0.78f) : 0.38f,
                         litem.seen ? (isSel ? 1.0f : hov2 ? 0.94f : 0.82f) : 0.38f,
                         litem.seen ? (isSel ? 1.0f : hov2 ? 0.98f : 0.90f) : 0.42f,
                         (litem.seen ? (isSel ? 0.98f : hov2 ? 0.86f : 0.72f) : 0.36f) * ia);
        }
        ry += ih;
    }

    BatchFlush();
    glDisable(GL_SCISSOR_TEST);

    // Scrollbar
    if (maxScroll > 0.0f) {
        BindMainShader();
        float tkX = panelX + leftW - 5.0f*uiS;
        drawRect(tkX, listTopY, 3.5f*uiS, listViewH,
                 0.10f, 0.10f, 0.14f, 0.50f * wake);
        float thumbH = listViewH * (listViewH / contentH);
        if (thumbH < 18.0f*uiS) thumbH = 18.0f*uiS;
        float thumbY = listTopY + (listViewH - thumbH) * (scroll / maxScroll);
        drawRect(tkX, thumbY, 3.5f*uiS, thumbH, cat.r, cat.g, cat.b, 0.80f * wake);
    }

    // ── RIGHT PANEL ───────────────────────────────────────────────────────
    float detailA    = Smoothstep(s_detailFadeT) * rightWake;
    float entryShift = (1.0f - rightWake) * 70.0f * uiS;
    float rpx        = rightX + entryShift;

    BindMainShader();
    drawRect(rpx + 9.0f*uiS, bodyY + 11.0f*uiS, rightW, bodyH,
             0.0f, 0.0f, 0.0f, (0.14f + 0.10f*panelE)*rightWake);
    drawRect(rpx, bodyY, rightW, bodyH,
             0.025f + cat.r*0.012f, 0.032f + cat.g*0.010f, 0.052f + cat.b*0.010f,
             (0.86f + 0.10f*panelE)*rightWake);
    drawRect(rpx, bodyY, rightW, 30.0f*uiS,
             cat.r*(0.20f + 0.38f*panelE),
             cat.g*(0.20f + 0.38f*panelE),
             cat.b*(0.22f + 0.38f*panelE), 0.94f*rightWake);
    drawRect(rpx, bodyY + 30.0f*uiS, rightW, 2.0f*uiS,
             cat.r, cat.g, cat.b, (0.22f + 0.55f*panelE)*rightWake);
    float sweepW = rightW * 0.28f;
    float sweepX = rpx + fmodf(now * 180.0f, rightW + sweepW) - sweepW;
    drawRect(sweepX, bodyY + 30.0f*uiS, sweepW, 2.0f*uiS,
             cat.r, cat.g, cat.b, 0.18f*panelE*rightWake);
    float scanY2 = bodyY + 44.0f*uiS + fmodf(now * 110.0f,
                   std::max(1.0f, bodyH - 56.0f*uiS));
    drawRect(rpx + 2.0f*uiS, scanY2, rightW - 4.0f*uiS, 1.0f*uiS,
             cat.r, cat.g, cat.b, 0.045f*panelE*rightWake);
    drawBorder(rpx, bodyY, rightW, bodyH, cat.r, cat.g, cat.b,
               (0.20f + 0.44f*panelE)*rightWake, 1.5f*uiS);
    if (rightWake > 0.02f) {
        BatchFlush(); SetGlowFx(true);
        drawConstellFrame(rpx, bodyY, rightW, bodyH,
                          cat.r, cat.g, cat.b, 0.20f*panelE*rightWake,
                          22.0f*uiS, 6.0f*uiS, 0.14f, rightWake);
        BatchFlush(); SetGlowFx(false);
    }
    BindMainShader();
    float bsz2 = 10.0f*uiS, bb2y = bodyY + 10.0f*uiS;
    float bb2x = rpx + rightW - 18.0f*uiS;
    drawRect(bb2x - 2.0f*(bsz2+7.0f*uiS), bb2y, bsz2, bsz2, 1,1,1, (0.10f+0.14f*panelE)*rightWake);
    drawRect(bb2x - (bsz2+7.0f*uiS),      bb2y, bsz2, bsz2, 1,1,1, (0.10f+0.14f*panelE)*rightWake);
    drawRect(bb2x,                          bb2y, bsz2, bsz2, 0.9f,0.25f,0.25f, (0.20f+0.40f*panelE)*rightWake);
    g_TextS.Draw(cat.name[nli], rpx + 14.0f*uiS, bodyY + 9.0f*uiS,
                 0.44f*uiS, 1.0f, 1.0f, 1.0f, 0.68f*rightWake);
    g_TextL.Draw(cat.id, rpx + 20.0f*uiS, bodyY + 40.0f*uiS,
                 1.05f*uiS, 1.0f, 1.0f, 1.0f, 0.96f*panelE*rightWake);

    // Content area
    int selItem = s_sel[s_cat];
    const float cntX = rpx + 22.0f*uiS;
    const float cntY = bodyY + 100.0f*uiS;
    const float cntW = rightW - 44.0f*uiS;

    if (selItem < 0) {
        const wchar_t* hint[2] = {
            L"\xBAA9\xB85D\xC5D0\xC11C \xD56D\xBAA9\xC744 \xD074\xB9AD\xD558\xBA74 \xC815\xBCF4\xAC00 \xD45C\xC2DC\xB429\xB2C8\xB2E4",
            L"Click an entry in the list to view details"
        };
        BindMainShader();
        float hw = g_TextS.Width(hint[nli], 0.48f*uiS);
        g_TextS.Draw(hint[nli], rpx + (rightW - hw) * 0.5f,
                     bodyY + bodyH * 0.5f - 10.0f*uiS,
                     0.48f*uiS, 0.36f, 0.46f, 0.60f, 0.55f*rightWake);
        float cx0 = rpx + rightW*0.5f, cy0 = bodyY + bodyH*0.5f + 28.0f*uiS;
        drawDiamond(cx0, cy0, (5.0f + 1.5f*sinf(now*1.6f))*uiS,
                    cat.r, cat.g, cat.b, 0.18f*rightWake);
    } else {
        const float iconSz = 130.0f * uiS;
        const float iconX  = cntX;
        const float iconY  = cntY;
        const float txtX   = iconX + iconSz + 26.0f*uiS;
        const float txtW   = cntW  - iconSz - 26.0f*uiS;

        BindMainShader();
        drawRect(iconX, iconY, iconSz, iconSz,
                 0.040f + cat.r*0.040f, 0.048f + cat.g*0.032f, 0.068f + cat.b*0.032f,
                 0.92f * detailA);
        drawCBkt(iconX, iconY, iconSz, iconSz, cat.r, cat.g, cat.b, 0.56f * detailA);

        if (s_cat == 0) {
            // ── Enemy detail ──────────────────────────────────────────────
            bool seen = CodexMobSeen(selItem);
            float ccx = iconX + iconSz*0.5f, ccy = iconY + iconSz*0.46f;
            if (seen) {
                if (selItem <= (int)MobKind::SHIELDED) {
                    Monster pm(ccx, ccy);
                    if (selItem > 0) pm.MakeKind((MobKind)selItem);
                    pm.worldX = ccx; pm.worldY = ccy;
                    if (selItem == 0) pm.color = glm::vec3(0.75f, 0.75f, 0.8f);
                    pm.sizeScale = (selItem == (int)MobKind::BRUTE) ? 2.2f : 2.8f;
                    drawMob(&pm);
                } else if (selItem == CM_RANGED) {
                    BindMainShader();
                    drawDiamond(ccx, ccy, 40.0f, 0.85f, 0.0f, 0.85f, 1.0f);
                    drawDiamond(ccx, ccy, 16.0f, 1,1,1, 0.9f);
                } else if (selItem == CM_BOMBER) {
                    BindMainShader();
                    drawPentagon(ccx, ccy, 44.0f, 1.0f, 0.5f, 0.1f, 1.0f);
                } else if (selItem == CM_DDOS) {
                    BindMainShader();
                    for (int t = 0; t < 3; t++) {
                        float ang = (float)t * 2.0944f;
                        drawTriangle(ccx + cosf(ang)*18.0f, ccy + sinf(ang)*18.0f,
                                     18.0f, 1.0f, 0.35f, 0.55f, 1.0f);
                    }
                } else if (selItem == CM_BADSECTOR) {
                    BindMainShader();
                    drawPentagon(ccx, ccy, 40.0f, 0.7f, 0.25f, 0.85f, 1.0f);
                    drawPentagon(ccx, ccy, 17.0f, 0.1f, 0.05f, 0.2f, 1.0f);
                } else {
                    BindMainShader();
                    drawDiamond(ccx,      ccy,      36.0f, 1.0f, 0.3f, 0.3f, 1.0f);
                    drawDiamond(ccx + 24, ccy,      13.0f, 1.0f, 0.3f, 0.3f, 1.0f);
                    drawDiamond(ccx - 24, ccy,      13.0f, 1.0f, 0.3f, 0.3f, 1.0f);
                    drawDiamond(ccx,      ccy + 24, 13.0f, 1.0f, 0.3f, 0.3f, 1.0f);
                    drawDiamond(ccx,      ccy - 24, 13.0f, 1.0f, 0.3f, 0.3f, 1.0f);
                }
            } else {
                BindMainShader();
                float qw = g_TextL.Width(L"?", 1.6f*uiS);
                g_TextL.Draw(L"?", iconX + (iconSz - qw)*0.5f, iconY + iconSz*0.38f,
                             1.6f*uiS, 0.4f, 0.4f, 0.45f, 0.9f * detailA);
            }
            BindMainShader();
            g_TextS.Draw(L"ENEMY", txtX, cntY + 4.0f*uiS,
                         0.38f*uiS, cat.r, cat.g, cat.b, 0.68f*detailA);
            g_TextL.Draw(MobName(selItem), txtX, cntY + 22.0f*uiS,
                         0.90f*uiS, 1.0f, 1.0f, 1.0f, 0.96f*detailA);
            drawRect(txtX, cntY + 78.0f*uiS, txtW, 1.5f*uiS,
                     cat.r, cat.g, cat.b, 0.28f*detailA);
            drawFitS(seen ? MobDesc(selItem) : L"???",
                     txtX, cntY + 90.0f*uiS, txtW,
                     0.44f*uiS, 0.32f*uiS, 0.82f, 0.90f, 1.0f, 0.88f*detailA);

        } else if (s_cat == 1) {
            // ── Augment detail ────────────────────────────────────────────
            bool seen = (selItem >= 0 && selItem < AUG_TOTAL && CodexAugSeen(selItem));
            if (seen) {
                const AugDef& d = ALL_AUGS[selItem];
                float rr, rg, rb; GetRarityColor(d.rarity, rr, rg, rb);
                float ccx = iconX + iconSz*0.5f, ccy = iconY + iconSz*0.46f;
                BindMainShader();
                drawDiamond(ccx, ccy, 38.0f*uiS, rr, rg, rb, 0.88f * detailA);
                const wchar_t* badge = GetAugBadge(d);
                float bsc = 0.62f*uiS;
                float bw3 = g_TextS.Width(badge, bsc);
                g_TextS.Draw(badge, ccx - bw3*0.5f, ccy - 13.0f*uiS,
                             bsc, 1.0f, 1.0f, 1.0f, 0.96f * detailA);
                BindMainShader();
                g_TextS.Draw(L"AUGMENT", txtX, cntY + 4.0f*uiS,
                             0.38f*uiS, rr, rg, rb, 0.68f*detailA);
                g_TextL.Draw(AugName(d), txtX, cntY + 22.0f*uiS,
                             0.90f*uiS, 1.0f, 1.0f, 1.0f, 0.96f*detailA);
                wchar_t rarLine[64];
                swprintf_s(rarLine, L"[%ls]  %ls", badge, GetRarityKR(d.rarity));
                g_TextS.Draw(rarLine, txtX, cntY + 70.0f*uiS,
                             0.42f*uiS, rr, rg, rb, 0.80f*detailA);
                drawRect(txtX, cntY + 88.0f*uiS, txtW, 1.5f*uiS,
                         rr, rg, rb, 0.28f*detailA);
                drawFitS(AugDesc(d), txtX, cntY + 100.0f*uiS, txtW,
                         0.44f*uiS, 0.32f*uiS, 0.82f, 0.90f, 1.0f, 0.88f*detailA);
                if (d.rarity == AugRarity::COMBO) {
                    for (int ci = 0; ci < COMBO_COUNT; ++ci) {
                        if (COMBO_DEFS[ci].result != d.type) continue;
                        const ComboDef& cd2 = COMBO_DEFS[ci];
                        int ia = AugIndexOfType(cd2.reqs[0]);
                        int ib = AugIndexOfType(cd2.reqs[1]);
                        int ic = cd2.reqCount >= 3 ? AugIndexOfType(cd2.reqs[2]) : -1;
                        wchar_t rc[192];
                        if (cd2.reqCount >= 3)
                            swprintf_s(rc, L"RECIPE: %ls + %ls + %ls",
                                       ia>=0 ? AugName(ALL_AUGS[ia]) : L"?",
                                       ib>=0 ? AugName(ALL_AUGS[ib]) : L"?",
                                       ic>=0 ? AugName(ALL_AUGS[ic]) : L"?");
                        else
                            swprintf_s(rc, L"RECIPE: %ls + %ls",
                                       ia>=0 ? AugName(ALL_AUGS[ia]) : L"?",
                                       ib>=0 ? AugName(ALL_AUGS[ib]) : L"?");
                        BindMainShader();
                        drawRect(txtX, cntY + 158.0f*uiS, txtW, 1.5f*uiS,
                                 0.10f, 0.88f, 0.78f, 0.28f*detailA);
                        drawFitS(rc, txtX, cntY + 170.0f*uiS, txtW,
                                 0.42f*uiS, 0.30f*uiS,
                                 0.12f, 0.92f, 0.82f, 0.90f*detailA);
                        break;
                    }
                }
            } else {
                BindMainShader();
                float qw = g_TextL.Width(L"?", 1.6f*uiS);
                g_TextL.Draw(L"?", iconX + (iconSz - qw)*0.5f, iconY + iconSz*0.38f,
                             1.6f*uiS, 0.4f, 0.4f, 0.45f, 0.9f * detailA);
                const wchar_t* q[2] = { L"\xBBF8\xBC1C\xACAC \xAE4C\xC9C0 \xBE44\xACF5\xAC1C",
                                         L"Undiscovered — unlock by acquiring" };
                g_TextS.Draw(q[nli], txtX, cntY + 40.0f*uiS,
                             0.46f*uiS, 0.50f, 0.50f, 0.55f, 0.80f*detailA);
            }
        } else {
            // ── Boss detail ───────────────────────────────────────────────
            bool seen = (selItem >= 0 && selItem < BOSS_CODEX_COUNT &&
                         BossCodexSeen(selItem));
            if (seen) {
                int pick = BossCodexPick(selItem);
                glm::vec3 wc = BossDir::WarnColor(pick);
                float cww = iconSz*0.72f, cwh = iconSz*0.60f;
                float cwx = iconX + (iconSz - cww)*0.5f;
                float cwy = iconY + (iconSz - cwh)*0.40f;
                BindMainShader();
                drawRect(cwx, cwy, cww, cwh, 0.08f, 0.09f, 0.12f, 0.92f * detailA);
                drawRect(cwx, cwy, cww, 18.0f*uiS,
                         wc.r*0.72f, wc.g*0.72f, wc.b*0.72f, 0.95f * detailA);
                drawNeonBorder(cwx, cwy, cww, cwh, wc.r, wc.g, wc.b);
                BindMainShader();
                g_TextS.Draw(L"BOSS", txtX, cntY + 4.0f*uiS,
                             0.38f*uiS, wc.r, wc.g, wc.b, 0.68f*detailA);
                g_TextL.Draw(BossCodexName(selItem), txtX, cntY + 22.0f*uiS,
                             0.90f*uiS, 1.0f, 1.0f, 1.0f, 0.96f*detailA);
                drawRect(txtX, cntY + 78.0f*uiS, txtW, 1.5f*uiS,
                         wc.r, wc.g, wc.b, 0.28f*detailA);
                drawFitS(BossCodexDesc(selItem), txtX, cntY + 90.0f*uiS, txtW,
                         0.44f*uiS, 0.32f*uiS, 0.82f, 0.90f, 1.0f, 0.88f*detailA);
            } else {
                BindMainShader();
                float qw = g_TextL.Width(L"?", 1.6f*uiS);
                g_TextL.Draw(L"?", iconX + (iconSz - qw)*0.5f, iconY + iconSz*0.38f,
                             1.6f*uiS, 0.4f, 0.4f, 0.45f, 0.9f * detailA);
                const wchar_t* q[2] = {
                    L"\xBBF8\xBC1C\xACAC \x2014 \xBCF4\xC2A4 \xC870\xC6B0 \xC2DC \xACF5\xAC1C",
                    L"Undiscovered — encounter the boss" };
                g_TextS.Draw(q[nli], txtX, cntY + 40.0f*uiS,
                             0.46f*uiS, 0.50f, 0.50f, 0.55f, 0.80f*detailA);
            }
        }
    }

    // ── FOOTER: BACK button ───────────────────────────────────────────────
    const wchar_t* backLbl[2] = { L"\xB4A4\xB85C", L"BACK" };
    float bw = 160.0f*uiS, bh = 40.0f*uiS;
    float bx = panelX, by = footY;
    bool backHov = hit(bx, by, bw, bh);
    BindMainShader();
    drawRect(bx, by, bw, bh,
             0.038f + cat.r*(backHov ? 0.036f : 0.012f),
             0.044f + cat.g*(backHov ? 0.030f : 0.010f),
             0.062f + cat.b*(backHov ? 0.026f : 0.008f), 0.90f * wake);
    drawCBkt(bx, by, bw, bh, cat.r, cat.g, cat.b, (backHov ? 0.68f : 0.28f) * wake);
    drawCenterS(backLbl[nli], bx, by + 10.0f*uiS, bw, 0.46f*uiS,
                backHov ? 1.0f : 0.78f, backHov ? 1.0f : 0.84f, backHov ? 1.0f : 0.96f,
                0.96f * wake);
    if (backHov && lmb && !g_LmbPrev) {
        CodexSearchClear();
        g_GameManager.currentState = GameState::MAIN_MENU;
    }
}


void Scene_Tutorial(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    GLFWwindow* window = c.window;
    const GameState st = g_GameManager.currentState;
    (void)window;

    static int s_page = 0;
    static GameState s_prevSt = GameState::MAIN_MENU;
    if (st != s_prevSt) {
        if (st == GameState::TUTORIAL) s_page = 0;
        s_prevSt = st;
    }

    auto drawSlashText = [&](const wchar_t* text, float x, float y, float maxW,
                             float sc, float r, float g, float b, float a) -> float {
        if (!text || !text[0]) return y;
        std::vector<std::wstring> lines;
        std::wstring cur;
        for (const wchar_t* p = text; *p; ++p) {
            if (*p == L'/') { if (!cur.empty()) lines.push_back(cur); cur.clear(); }
            else cur += *p;
        }
        if (!cur.empty()) lines.push_back(cur);
        for (auto& s : lines) {
            while (!s.empty() && s.front() == L' ') s.erase(0, 1);
            while (!s.empty() && s.back() == L' ') s.pop_back();
        }
        float lineH = 26.0f * sc;
        float cy = y;
        for (int li = 0; li < (int)lines.size(); li++) {
            float lsc = sc;
            while (lsc > 0.55f && g_TextS.Width(lines[li].c_str(), lsc) > maxW) lsc -= 0.04f;
            g_TextS.Draw(lines[li].c_str(), x, cy + li * lineH, lsc, r, g, b, a);
        }
        return y + (float)lines.size() * lineH;
    };

    const float WW = std::min(sw * 0.92f, 1180.0f);
    const float WH = std::min(sh * 0.88f, 880.0f);
    float wx, wy;
    DrawMenuBackground(sw, sh, c.delta);
    SceneAppWindow(sw, sh, WW, WH, TutorialWinTitle(), 0.35f, 0.85f, 1.0f, wx, wy, false, 0.0f);
    if (g_AppOpen < 0.999f) return;

    int li = LangIndexTutorial();
    const float sideW = 210.0f;
    const float pad = 24.0f;
    float sideX = wx + pad;
    float sideY = wy + 46.0f;
    float contentX = wx + sideW + pad * 2.0f;
    float contentW = WW - sideW - pad * 3.0f;
    float contentY = wy + 46.0f;

    BindMainShader();
    drawRect(sideX, sideY, sideW, WH - 120.0f, 0.10f, 0.12f, 0.16f, 0.95f);

    const wchar_t* SIDE[3] = { L"목차", L"Contents", L"目次" };
    g_TextS.Draw(SIDE[li], sideX + 14.0f, sideY + 10.0f, 0.82f, 0.55f, 0.85f, 1.0f, 0.95f);

    const float rowH = 44.0f;
    for (int i = 0; i < TUTORIAL_PAGE_COUNT; i++) {
        float ry = sideY + 38.0f + i * rowH;
        bool sel = (i == s_page);
        bool hov = (mx >= sideX + 6.0f && mx <= sideX + sideW - 6.0f &&
                    my >= ry && my <= ry + rowH - 4.0f);
        if (sel) drawRect(sideX + 6.0f, ry, sideW - 12.0f, rowH - 4.0f, 0.25f, 0.55f, 0.95f, 0.35f);
        else if (hov) drawRect(sideX + 6.0f, ry, sideW - 12.0f, rowH - 4.0f, 0.18f, 0.22f, 0.28f, 0.85f);
        const wchar_t* pt = TutorialPageTitle(i);
        g_TextS.Draw(pt, sideX + 16.0f, ry + 12.0f, 0.78f,
                     sel ? 0.95f : 0.82f, sel ? 0.98f : 0.88f, 1.0f, 1.0f);
        if (hov && lmb && !g_LmbPrev) s_page = i;
    }

    const wchar_t* pageTitle = TutorialPageTitle(s_page);
    g_TextL.Draw(pageTitle, contentX, contentY, 1.15f, 0.55f, 0.90f, 1.0f, 1.0f);
    float bodyY = contentY + 52.0f;
    bodyY = drawSlashText(TutorialPageBody(s_page), contentX, bodyY, contentW,
                          0.88f, 0.90f, 0.94f, 1.0f, 0.95f);

    const wchar_t* ctrl = TutorialControls();
    float ctrlW = g_TextS.Width(ctrl, 0.72f);
    g_TextS.Draw(ctrl, contentX + (contentW - ctrlW) * 0.5f, wy + WH - 108.0f, 0.72f,
                 0.50f, 0.80f, 1.0f, 0.90f);

    const wchar_t* PREV[3] = { L"< 이전", L"< Prev", L"< 前へ" };
    const wchar_t* NEXT[3] = { L"다음 >", L"Next >", L"次へ >" };
    float navY = wy + WH - 62.0f;
    if (s_page > 0 && UIButton(wx + pad, navY, 130.0f, 46.0f, PREV[li], mx, my, lmb, g_LmbPrev))
        s_page--;
    if (s_page < TUTORIAL_PAGE_COUNT - 1 &&
        UIButton(wx + pad + 140.0f, navY, 130.0f, 46.0f, NEXT[li], mx, my, lmb, g_LmbPrev))
        s_page++;

    wchar_t pg[32];
    swprintf_s(pg, L"%d / %d", s_page + 1, TUTORIAL_PAGE_COUNT);
    float pgW = g_TextS.Width(pg, 0.85f);
    g_TextS.Draw(pg, wx + WW * 0.5f - pgW * 0.5f, navY + 14.0f, 0.85f, 0.7f, 0.8f, 0.9f, 0.95f);

    {
        static bool s_lPrev = false, s_rPrev = false;
        bool lk = (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS);
        bool rk = (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS);
        if (lk && !s_lPrev && s_page > 0) s_page--;
        if (rk && !s_rPrev && s_page < TUTORIAL_PAGE_COUNT - 1) s_page++;
        s_lPrev = lk;
        s_rPrev = rk;
    }

    if (UIButton(wx + WW - pad - 180.0f, navY, 180.0f, 46.0f, T(StrId::BTN_BACK),
                 mx, my, lmb, g_LmbPrev))
        g_GameManager.currentState = GameState::MAIN_MENU;
}

// 직업 확정 → 고정 무기/조작 적용 → 시작증강 or READY 로 전이.
//   직업제 개편: 총기 3택1 화면 없이 직업 = 고정 무기 1:1 매핑.
void FinalizeLoadout(const SceneCtx& c, int wIdx) {
    const float sw = c.sw, sh = c.sh;
    float& fireTimer = *c.fireTimer;

    g_Difficulty = Difficulty::NORMAL;
    g_Stats.baseFireInterval = g_Stats.fireInterval;
    ApplyWeapon(g_Stats, (StartWeapon)wIdx);
    g_CurrentWeapon = wIdx;
    MarkStartWeaponOwnedType((StartWeapon)wIdx);
    fireTimer = g_Stats.fireInterval;
    bool classJob = false;
    if (g_SelectedJob > 0 && g_SelectedJob < JOB_COUNT) {
        const JobDef& jd = JOB_DEFS[g_SelectedJob];
        for (int a = 0; a < jd.startAugCount; a++) {
            int ji = AugIndexOf(jd.startAugs[a]);
            if (ji < 0) continue;
            g_Stats.Apply(jd.startAugs[a]);
            g_OwnedAugs.push_back(ji);
            // 조합 레시피(g_TypeOwned)와 분리 — 런 중 획득한 증강만 조합에 사용
            MarkAugSeen(ji);
            if (AugOnceOnly(jd.startAugs[a], ALL_AUGS[ji].rarity))
                g_GameManager.takenOnce[ji] = true;
            EquipSkill(SkillForAug(jd.startAugs[a]));
            ApplyAugmentSideEffects(jd.startAugs[a], (int)sw, (int)sh);
        }
        if (jd.weaponMode == 1) {           // 검객: 근접 호 스윙
            g_Stats.meleeWeapon  = true;
            g_Stats.fireInterval = 0.26f;
            g_Stats.baseFireInterval = g_Stats.fireInterval;
            g_RunMelee = true; classJob = true;
        } else if (jd.weaponMode == 2) {    // 궁수: 차징 화살
            g_Stats.bowWeapon    = true;
            g_Stats.bulletSpeed *= 1.4f;
            g_RunBow = true; classJob = true;
        }
        fireTimer = g_Stats.fireInterval;
    }
    // 검객/궁수는 총기 표기가 무의미 → 변환/게임오버 표시용 무기 제거
    if (classJob) g_CurrentWeapon = -1;
    // 크리에이티브: 직접 고른 시작 증강 즉시 적용 (스탯+보유목록 직접)
    if (g_CreativeMode) {
        for (int aidx : g_CreativeStartAugList) {
            if (aidx < 0 || aidx >= AUG_TOTAL) continue;
            AugType atype = ALL_AUGS[aidx].type;
            g_Stats.Apply(atype);
            g_OwnedAugs.push_back(aidx);
            g_TypeOwned[(int)atype] = true;
            MarkAugSeen(aidx);
            EquipSkill(SkillForAug(atype));
            if (AugOnceOnly(atype, ALL_AUGS[aidx].rarity))
                g_GameManager.takenOnce[aidx] = true;
            ApplyAugmentSideEffects(atype, (int)sw, (int)sh);
        }
        SyncPlayerWindowAfterLoadout();
    }
    float trialHpMul = TrialPlayerMaxHpMult();
    if (trialHpMul < 0.999f)
        g_Stats.maxHP *= trialHpMul;
    g_GameManager.maxHP    = g_Stats.maxHP;
    g_GameManager.playerHP = g_Stats.maxHP;
    g_PrevHP               = g_Stats.maxHP;
    int startAugs = g_MetaStartAugs + ((g_CreativeMode) ? g_CreativeStartAugs : 0);
    if (startAugs > 0) {
        g_BossRewardPicksLeft = startAugs;
        g_GameManager.PickAugChoices(g_Stats.sizeAugTaken,
                                     g_Stats.distAugTaken, g_CreativeMode);
        g_GameManager.currentState = GameState::AUG_SELECT;
    } else {
        g_GameManager.currentState = GameState::READY;
    }
}

// 선택된 직업(g_SelectedJob)의 고정 무기 인덱스. 근접/활 직업은 내부적으로
// RIFLE 을 더미 베이스로 쓰고 weaponMode 가 이후 덮어쓴다.
static int FixedWeaponForSelectedJob() {
    if (g_SelectedJob > 0 && g_SelectedJob < JOB_COUNT &&
        JOB_DEFS[g_SelectedJob].fixedWeapon >= 0)
        return JOB_DEFS[g_SelectedJob].fixedWeapon;
    return (int)StartWeapon::RIFLE;
}

void Scene_JobSelect(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    GLFWwindow* window = c.window;
    const GameState st = g_GameManager.currentState;
    float& fireTimer = *c.fireTimer;
    const std::function<void()>& ResetForNewGame = c.reset;
                BindMainShader();
                const float FW = FLOW_PANEL_W, FH = FLOW_PANEL_H;
                float fx, fy, fcy;
                DrawMenuBackground(sw, sh, delta);
                SceneFlowWindow(sw, sh, FW, FH, L"career.exe", 0.55f, 0.7f, 1.0f, fx, fy, fcy, 0.0f);
                const float backY = FlowBackY(fy, FH);

                int li = LangIndex();
                const wchar_t* JTIT[3] = { L"직업 선택", L"Choose a Class", L"職業を選択" };
                const wchar_t* JHINT[3] = {
                    L"업적을 달성하면 새 직업이 해금됩니다",
                    L"Complete achievements to unlock more classes",
                    L"実績達成で新しい職業が解放されます" };
                const wchar_t* LOCKED[3] = { L"잠김 — ", L"Locked — ", L"未解放 — " };
                const wchar_t* TIT = JTIT[li];
                float titSc = 1.35f;
                float titleY = fcy + 28.0f;
                float titW = g_TextL.Width(TIT, titSc);
                g_TextL.Draw(TIT, fx + (FW - titW) * 0.5f, titleY, titSc, 1, 1, 1, 1);
                const wchar_t* HN = JHINT[li];
                float hintSc = 0.88f;
                float hintY = titleY + g_TextL.Height(TIT, titSc) + 20.0f;
                float hintW = g_TextS.Width(HN, hintSc);
                g_TextS.Draw(HN, fx + (FW - hintW) * 0.5f, hintY, hintSc,
                             0.7f, 0.8f, 0.9f, 0.9f);

                const float BW = 920.0f, BH = 84.0f, BG = 14.0f;
                const float ICON_W = 58.0f;
                const float TX = 14.0f;
                float bx = fx + (FW - BW) * 0.5f;
                float by = hintY + g_TextS.Height(HN, hintSc) + 36.0f;
                static const int kShownJobs[] = { JOB_NONE, JOB_SWORDSMAN, JOB_ARCHER };
                for (int jRow = 0; jRow < 3; jRow++) {
                    int j = kShownJobs[jRow];
                    float y = by + jRow * (BH + BG);
                    bool unlocked = JobUnlocked(j);
                    bool sel = (g_SelectedJob == j);
                    bool clicked = false;
                    if (unlocked) {
                        clicked = UIButton(bx, y, BW, BH, L"", mx, my, lmb, g_LmbPrev, sel);
                    } else {
                        BindMainShader();
                        drawRect(bx, y, BW, BH, 0.08f, 0.06f, 0.06f, 0.9f);
                        drawRect(bx, y, BW, 2.0f, 0.4f,0.3f,0.3f,0.8f);
                        drawRect(bx, y+BH-2, BW, 2.0f, 0.4f,0.3f,0.3f,0.8f);
                    }
                    GLuint ji = JobIcon(j);
                    if (ji) {
                        float isz = BH - 24.0f;
                        DrawIcon(ji, bx + 10.0f, y + (BH - isz)*0.5f, isz, isz,
                                 unlocked?1.0f:0.45f, unlocked?1.0f:0.45f, unlocked?1.0f:0.5f, 0.95f);
                    }
                    BindMainShader();
                    float textX = bx + ICON_W + TX;
                    float textW = BW - ICON_W - TX - 12.0f;
                    float nsc = 0.82f;
                    while (nsc > 0.52f && g_TextL.Width(JobName(j), nsc) > textW) nsc -= 0.04f;
                    g_TextL.Draw(JobName(j), textX, y + 12.0f, nsc,
                                 unlocked?1.0f:0.55f, unlocked?1.0f:0.55f, unlocked?1.0f:0.6f, 0.98f);
                    wchar_t line[200];
                    float lr=0.85f, lg=0.95f, lb=1.0f;
                    if (unlocked) {
                        swprintf_s(line, L"%ls", JobDesc(j));
                    } else {
                        int a = JOB_DEFS[j].unlockAch;
                        swprintf_s(line, L"%ls%ls", LOCKED[li],
                                   (a>=0 && a<ACH_COUNT) ? AchName(a) : L"???");
                        lr=0.9f; lg=0.45f; lb=0.45f;
                    }
                    float dsc = 0.70f;
                    while (dsc > 0.48f && g_TextS.Width(line, dsc) > textW) dsc -= 0.03f;
                    g_TextS.Draw(line, textX, y + 46.0f, dsc, lr, lg, lb, 0.92f);

                    if (clicked) {
                        g_SelectedJob = j;
                        ResetForNewGame();
                        FinalizeLoadout(c, FixedWeaponForSelectedJob());
                    }
                }
                if (UIButton(fx + 32.0f, backY, 160.0f, 48.0f, T(StrId::BTN_BACK),
                             mx, my, lmb, g_LmbPrev)) {
                    g_GameManager.currentState = g_CreativeMode
                        ? GameState::CREATIVE_CONFIG : GameState::RUN_CONFIG;
                }
}

void Scene_RunConfig(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    (void)c.window;
    const std::function<void()>& ResetForNewGame = c.reset;

    if (!g_TrialPoolReady) RerollTrialPool();

    float dt = c.delta;
    if (dt > 0.05f) dt = 0.05f;
    g_RunConfigEntryT += dt;
    if (g_RunConfigEntryT > 1.0f) g_RunConfigEntryT = 1.0f;
    const float wake = Smoothstep(g_RunConfigEntryT / 0.55f);
    const float now = (float)glfwGetTime();

    DrawMenuBackground(sw, sh, c.delta);

    { float ua = 1.0f - g_FadeAlpha; if (ua < 0.0f) ua = 0.0f; g_BatchAlpha = ua; }

    const float TARGET_W = 1640.0f;
    const float TARGET_H = 910.0f;
    const float uiS = std::max(0.70f, std::min(sw * 0.94f / TARGET_W, sh * 0.90f / TARGET_H));
    const float panelW = TARGET_W * uiS;
    const float panelH = TARGET_H * uiS;
    const float panelX = (sw - panelW) * 0.5f;
    const float panelY = (sh - panelH) * 0.5f;
    const float headerH = 88.0f * uiS;
    const float footerH = 74.0f * uiS;
    const float gap = 20.0f * uiS;
    const float colY = panelY + headerH;
    const float colH = panelH - headerH - footerH;
    const float leftW = panelW * 0.335f;
    const float midW = panelW * 0.385f;
    const float rightW = panelW - leftW - midW - gap * 2.0f;
    const float leftX = panelX;
    const float midX = leftX + leftW + gap;
    const float rightX = midX + midW + gap;
    const float footY = panelY + panelH - footerH + 12.0f * uiS;

    int li = LangIndex();
    int nli = (li == 0) ? 0 : 1;

    auto hit = [&](float x, float y, float w, float h) {
        return mx >= x && mx <= x + w && my >= y && my <= y + h;
    };

    auto drawBorder = [](float x, float y, float w, float h,
                         float r, float g, float b, float a, float thick) {
        drawRect(x, y, w, thick, r, g, b, a);
        drawRect(x, y + h - thick, w, thick, r, g, b, a);
        drawRect(x, y, thick, h, r, g, b, a);
        drawRect(x + w - thick, y, thick, h, r, g, b, a);
    };

    auto drawFitS = [&](const wchar_t* text, float x, float y, float maxW,
                        float sc, float minSc, float r, float g, float b, float a) {
        while (sc > minSc && g_TextS.Width(text, sc) > maxW)
            sc -= 0.025f * uiS;
        g_TextS.Draw(text, x, y, sc, r, g, b, a);
    };

    auto drawCenterS = [&](const wchar_t* text, float x, float y, float w,
                           float sc, float r, float g, float b, float a) {
        while (sc > 0.42f * uiS && g_TextS.Width(text, sc) > w - 16.0f * uiS)
            sc -= 0.025f * uiS;
        float tw = g_TextS.Width(text, sc);
        g_TextS.Draw(text, x + (w - tw) * 0.5f, y, sc, r, g, b, a);
    };

    auto drawCenterL = [&](const wchar_t* text, float x, float y, float w,
                           float sc, float r, float g, float b, float a) {
        while (sc > 0.48f * uiS && g_TextL.Width(text, sc) > w - 20.0f * uiS)
            sc -= 0.035f * uiS;
        float tw = g_TextL.Width(text, sc);
        g_TextL.Draw(text, x + (w - tw) * 0.5f, y, sc, r, g, b, a);
    };

    auto drawWindow = [&](float x, float y, float w, float h, const wchar_t* title,
                          float r, float g, float b, float focus, bool current) {
        BindMainShader();
        float bodyA = 0.62f + 0.28f * focus;
        drawRect(x + 8.0f * uiS, y + 10.0f * uiS, w, h, 0.0f, 0.0f, 0.0f, 0.20f + 0.10f * focus);
        drawRect(x, y, w, h,
                 0.025f + r * 0.016f * focus,
                 0.032f + g * 0.014f * focus,
                 0.052f + b * 0.014f * focus, bodyA);
        drawRect(x, y, w, 30.0f * uiS,
                 r * (0.20f + 0.38f * focus),
                 g * (0.20f + 0.38f * focus),
                 b * (0.22f + 0.38f * focus), 0.94f);
        drawRect(x, y + 30.0f * uiS, w, 2.0f * uiS, r, g, b, 0.22f + 0.55f * focus);
        drawBorder(x, y, w, h, r, g, b, 0.15f + 0.55f * focus, 1.5f * uiS);
        float bs = 10.0f * uiS;
        float by = y + 10.0f * uiS;
        float bx = x + w - 18.0f * uiS;
        drawRect(bx - 2.0f * (bs + 7.0f * uiS), by, bs, bs, 1, 1, 1, 0.12f + 0.15f * focus);
        drawRect(bx - (bs + 7.0f * uiS), by, bs, bs, 1, 1, 1, 0.12f + 0.15f * focus);
        drawRect(bx, by, bs, bs, 0.9f, 0.25f, 0.25f, 0.22f + 0.45f * focus);
        g_TextS.Draw(title, x + 12.0f * uiS, y + 7.0f * uiS, 0.56f * uiS,
                     0.72f + 0.28f * focus, 0.82f + 0.16f * focus, 1.0f, 0.52f + 0.46f * focus);
        if (current && focus > 0.25f) {
            float sweepY = y + 35.0f * uiS + fmodf(now * 95.0f, h - 48.0f * uiS);
            drawRect(x + 1.5f * uiS, sweepY, w - 3.0f * uiS, 1.0f * uiS, r, g, b, 0.045f * focus);
        }
    };

    auto drawDisabledVeil = [&](float x, float y, float w, float h,
                                const wchar_t* msg, float a) {
        BindMainShader();
        drawRect(x, y + 30.0f * uiS, w, h - 30.0f * uiS, 0.0f, 0.0f, 0.0f, a);
        drawCenterS(msg, x, y + h * 0.50f - 9.0f * uiS, w,
                    0.60f * uiS, 0.48f, 0.58f, 0.72f, 0.70f);
    };

    struct WCardDef {
        int job;
        const wchar_t* name[2];
        const wchar_t* desc[2];
        long long cost;
    };
    static const WCardDef kWC[] = {
        { JOB_NONE,      { L"소총",     L"RIFLE"        },
          { L"표준 연사 / 균형 범용",          L"Standard auto fire / balanced"   }, 0 },
        { JOB_BERSERKER, { L"화포",     L"CANNON"       },
          { L"고화력 단발 포격",               L"Heavy single-shot artillery"     }, 0 },
        { JOB_VAMPIRE,   { L"정전기장", L"STATIC FIELD" },
          { L"범위 전기장 지속 피해",          L"Area electric field / sustained" }, 0 },
    };
    static const int kWCCount = 3;
    static const wchar_t* kWeaponRole[][2] = {
        { L"균형형", L"BALANCED" },
        { L"단발형", L"BURST"    },
        { L"범위형", L"AREA"     },
    };
    static const wchar_t* kWeaponStatLabels[6][2] = {
        { L"피해량",   L"DAMAGE"    },
        { L"공격속도", L"FIRE RATE" },
        { L"연사간격", L"INTERVAL"  },
        { L"탄속",     L"SPEED"     },
        { L"흔들림",   L"SPREAD"    },
        { L"사거리",   L"RANGE"     },
    };
    struct WDetailDef {
        const wchar_t* profile[2];
        const wchar_t* notes[2][3];
        const wchar_t* stat[2][6];
        float r, g, b;
        float norm[6]; // 레이더 축 [0-1]: 피해량, 공격속도, 연사간격, 탄속, 정밀도, 사거리
    };
    static const WDetailDef kWeaponDetail[] = {
        {   // 소총 — 균형형, 전 축 중상위
            { L"안정적인 표준 화력", L"Stable standard fire" },
            {
                { L"기본 조작과 성장 효율이 가장 안정적",
                  L"거리 유지와 보스전 대응이 무난함",
                  L"큰 약점이 적은 입문 기준 무기" },
                { L"Most stable handling and growth curve",
                  L"Reliable range control and boss response",
                  L"Baseline weapon with few sharp weaknesses" }
            },
            {
                { L"50", L"5.00/s", L"0.20s", L"1200", L"0.04 rad", L"표준" },
                { L"50", L"5.00/s", L"0.20s", L"1200", L"0.04 rad", L"Standard" }
            },
            0.35f, 0.72f, 1.00f,
            { 0.50f, 0.78f, 0.78f, 0.72f, 0.68f, 0.72f }
        },
        {   // 화포 — 피해+정밀 극대, 속도 극소
            { L"한 발로 전선을 바꾸는 중포격기", L"One shot that reshapes the front" },
            {
                { L"단발 피해량이 소총의 약 5배",
                  L"느린 재장전 — 헛발 하나가 치명적",
                  L"군집 포격 시 가장 강력한 순간 화력" },
                { L"Single-shot damage ~5x that of rifle",
                  L"Slow reload — each miss is costly",
                  L"Most effective against clustered enemies" }
            },
            {
                { L"240", L"0.67/s", L"1.50s", L"800", L"0 rad", L"표준" },
                { L"240", L"0.67/s", L"1.50s", L"800", L"0 rad", L"Standard" }
            },
            1.00f, 0.55f, 0.18f,
            { 1.00f, 0.18f, 0.12f, 0.42f, 1.00f, 0.52f }
        },
        {   // 정전기장 — 속도·연사 극대, 피해·사거리 극소
            { L"탄환 없이 전장을 전기로 지배한다", L"Control the field with no ammunition" },
            {
                { L"플레이어 주변 반경에 전기장을 지속 전개",
                  L"범위 내 모든 적에게 틱 피해 — 탄약 없음",
                  L"원거리 적에게 무력, 근접 통제에 특화" },
                { L"Sustains an electric field around the player",
                  L"Tick damage to all enemies in range — no ammo",
                  L"Ineffective at range, dominates close control" }
            },
            {
                { L"18/틱", L"연속", L"0.08s", L"즉시", L"-", L"180 px" },
                { L"18/tick", L"Continuous", L"0.08s", L"Instant", L"-", L"180 px" }
            },
            0.55f, 0.90f, 1.00f,
            { 0.22f, 1.00f, 0.94f, 1.00f, 0.50f, 0.28f }
        },
    };
    static int   s_WeaponSel      = 0;
    static float s_WeaponHover[3] = {};
    if (s_WeaponSel < 0 || s_WeaponSel >= kWCCount) s_WeaponSel = 0;

    // 레이더 차트 애니메이션: 선택된 무기 norm 값으로 매 프레임 접근
    {
        const float* tgt = kWeaponDetail[s_WeaponSel].norm;
        float spd = (g_RunConfigStep >= 1) ? 12.0f : 0.0f;
        for (int i = 0; i < 6; ++i)
            s_RadarCur[i] = UiApproach(s_RadarCur[i], tgt[i], dt, spd);
    }

    // 무기 전환 트랜지션: 선택이 바뀌면 패널 콘텐츠를 페이드아웃 후 인
    if (g_RunConfigStep >= 1 && s_WeaponSel != s_PanelPrevSel) {
        s_PanelFadeT   = 0.0f;
        s_StatGlitchT  = 0.0f;
        s_PanelPrevSel = s_WeaponSel;
    }
    s_PanelFadeT = std::min(1.0f, s_PanelFadeT + dt * 5.5f);
    s_StatGlitchT = std::min(1.0f, s_StatGlitchT + dt * 8.0f);
    const float panelContentA = Smoothstep(s_PanelFadeT);

    const float accentR = (g_RunConfigStep >= 1) ? kWeaponDetail[s_WeaponSel].r : 0.42f;
    const float accentG = (g_RunConfigStep >= 1) ? kWeaponDetail[s_WeaponSel].g : 0.28f;
    const float accentB = (g_RunConfigStep >= 1) ? kWeaponDetail[s_WeaponSel].b : 1.00f;

    auto clearTrialSelection = [&]() {
        for (int i = 0; i < TRIAL_SLOT_COUNT; ++i)
            g_TrialSelected[i] = false;
    };

    auto startConfiguredRun = [&]() {
        if (!s_TrialsEnabled)
            clearTrialSelection();
        if (!JobUnlocked(kWC[s_WeaponSel].job))
            s_WeaponSel = 0;
        g_SelectedJob = kWC[s_WeaponSel].job;
        g_Difficulty = Difficulty::NORMAL;
        s_TrialSelectPage = false;
        if (g_CreativeMode) {
            g_GameManager.currentState = GameState::CREATIVE_CONFIG;
        } else {
            ResetForNewGame();
            FinalizeLoadout(c, FixedWeaponForSelectedJob());
        }
    };

    if (s_TrialTargetCount < 1) s_TrialTargetCount = 1;
    if (s_TrialTargetCount > TRIAL_SLOT_COUNT) s_TrialTargetCount = TRIAL_SLOT_COUNT;

    if (s_TrialSelectPage) {
        s_TrialsEnabled = true;

        const float pageW = std::min(panelW * 0.72f, 1040.0f * uiS);
        const float pageH = 620.0f * uiS;
        const float pageX = (sw - pageW) * 0.5f;
        const float pageY = (sh - pageH) * 0.5f;
        const float pagePad = 30.0f * uiS;
        const float rowH = 82.0f * uiS;
        const float rowG = 12.0f * uiS;

        BindMainShader();
        drawRect(pageX + 10.0f*uiS, pageY + 12.0f*uiS, pageW, pageH,
                 0.0f, 0.0f, 0.0f, 0.30f);
        drawRect(pageX, pageY, pageW, pageH,
                 0.020f + accentR * 0.012f,
                 0.026f + accentG * 0.010f,
                 0.042f + accentB * 0.012f, 0.94f);
        drawRect(pageX, pageY, pageW, 3.0f*uiS, accentR, accentG, accentB, 0.82f);
        BatchFlush();
        SetGlowFx(true);
        drawConstellFrame(pageX, pageY, pageW, pageH, accentR, accentG, accentB,
                          0.70f, 22.0f*uiS, 6.0f*uiS, 0.12f);
        BatchFlush();
        SetGlowFx(false);

        g_TextL.Draw(L"TRIAL SELECT", pageX + pagePad, pageY + 24.0f*uiS,
                     0.96f*uiS, 0.92f, 0.96f, 1.0f, 0.98f);
        g_TextS.Draw(L"C:\\ONEDOW\\RUN_CONFIG > TRIAL_SELECT",
                     pageX + pagePad, pageY + 66.0f*uiS,
                     0.46f*uiS, 0.48f, 0.58f, 0.78f, 0.84f);

        wchar_t needBuf[48];
        swprintf_s(needBuf, L"%d / %d", TrialCount(), s_TrialTargetCount);
        const float needSc = 0.66f*uiS;
        g_TextL.Draw(needBuf, pageX + pageW - pagePad - g_TextL.Width(needBuf, needSc),
                     pageY + 29.0f*uiS, needSc,
                     TrialCount() == s_TrialTargetCount ? 0.60f : 1.0f,
                     TrialCount() == s_TrialTargetCount ? 0.96f : 0.64f,
                     TrialCount() == s_TrialTargetCount ? 0.54f : 0.42f, 0.96f);

        const float listX = pageX + pagePad;
        const float listY = pageY + 110.0f*uiS;
        const float listW = pageW - pagePad * 2.0f;
        for (int slot = 0; slot < TRIAL_SLOT_COUNT; ++slot) {
            const float sx = listX;
            const float sy = listY + (float)slot * (rowH + rowG);
            const int defIdx = std::max(0, std::min(TRIAL_DEF_COUNT - 1, g_TrialPool[slot]));
            const bool hovSlot = hit(sx, sy, listW, rowH);
            if (hovSlot && lmb && !g_LmbPrev) {
                if (g_TrialSelected[slot]) {
                    g_TrialSelected[slot] = false;
                } else if (TrialCount() < s_TrialTargetCount) {
                    g_TrialSelected[slot] = true;
                }
            }

            s_TrialHover[slot] = UiApproach(s_TrialHover[slot], hovSlot ? 1.0f : 0.0f, dt, 12.0f);
            const float hvSlot = s_TrialHover[slot];
            const bool picked = g_TrialSelected[slot];
            const bool locked = !picked && TrialCount() >= s_TrialTargetCount;
            const float rowA = locked ? 0.20f : (picked ? 0.58f : 0.34f + 0.10f * hvSlot);
            const float lineA = locked ? 0.10f : (picked ? 0.82f : 0.22f + 0.24f * hvSlot);

            BindMainShader();
            drawRect(sx, sy, listW, rowH,
                     0.014f + accentR * (picked ? 0.050f : 0.012f),
                     0.018f + accentG * (picked ? 0.044f : 0.010f),
                     0.030f + accentB * (picked ? 0.038f : 0.010f), rowA);
            drawRect(sx, sy, 4.0f*uiS, rowH, accentR, accentG, accentB, lineA);
            drawRect(sx, sy + rowH - 1.0f*uiS, listW, 1.0f*uiS,
                     accentR, accentG, accentB, lineA * 0.55f);

            const float cbx = sx + 22.0f*uiS;
            const float cby = sy + 14.0f*uiS;
            g_TextS.Draw(L"[   ]", cbx, cby, 0.50f*uiS,
                         locked ? 0.36f : 0.78f,
                         locked ? 0.40f : 0.86f,
                         locked ? 0.48f : 0.94f,
                         locked ? 0.34f : 0.88f);
            if (picked) {
                BindMainShader();
                drawRect(cbx + 8.0f*uiS, cby + 7.0f*uiS,
                         7.0f*uiS, 7.0f*uiS, accentR, accentG, accentB, 0.94f);
            }

            wchar_t titleBuf[96];
            swprintf_s(titleBuf, L"%ls / %ls",
                       TrialStageLabel(TrialStageForDef(defIdx), nli),
                       TRIAL_DEFS[defIdx].id);
            const int bonusPct = (int)(TrialScoreBonusForDef(defIdx) * 100.0f + 0.5f);
            wchar_t bonusBuf[24];
            swprintf_s(bonusBuf, L"+%02d%%", bonusPct);

            const float textX = sx + 86.0f*uiS;
            const float bonusSc = 0.62f*uiS;
            const float bonusW = g_TextL.Width(bonusBuf, bonusSc);
            const float bonusX = sx + listW - bonusW - 24.0f*uiS;
            drawFitS(titleBuf, textX, sy + 13.0f*uiS,
                     bonusX - textX - 24.0f*uiS, 0.56f*uiS, 0.40f*uiS,
                     picked ? accentR : 0.74f,
                     picked ? accentG : 0.84f,
                     picked ? accentB : 0.98f,
                     locked ? 0.36f : (picked ? 0.98f : 0.82f));
            drawFitS(TRIAL_DEFS[defIdx].desc[nli], textX, sy + 45.0f*uiS,
                     bonusX - textX - 24.0f*uiS, 0.42f*uiS, 0.30f*uiS,
                     0.48f, 0.58f, 0.76f,
                     locked ? 0.26f : (picked ? 0.72f : 0.56f));
            g_TextL.Draw(bonusBuf, bonusX, sy + 24.0f*uiS, bonusSc,
                         picked ? 0.72f : 0.48f,
                         picked ? 1.0f  : 0.62f,
                         picked ? 0.58f : 0.78f,
                         locked ? 0.34f : 0.88f);
        }

        const float backW = 168.0f*uiS;
        const float btnH = 50.0f*uiS;
        const float btnY = pageY + pageH - pagePad - btnH;
        const float confirmW = 292.0f*uiS;
        const float confirmX = pageX + pageW - pagePad - confirmW;
        const bool backHov = hit(pageX + pagePad, btnY, backW, btnH);
        const bool ready = TrialCount() == s_TrialTargetCount;
        const bool confirmHov = ready && hit(confirmX, btnY, confirmW, btnH);
        if (backHov && lmb && !g_LmbPrev) {
            clearTrialSelection();
            s_TrialSelectPage = false;
        }
        if (confirmHov && g_LmbPrev && !lmb)
            startConfiguredRun();

        BindMainShader();
        drawRect(pageX + pagePad, btnY, backW, btnH, 0.028f, 0.034f, 0.052f, 0.72f);
        drawBorder(pageX + pagePad, btnY, backW, btnH,
                   0.50f, 0.60f, 0.78f, backHov ? 0.70f : 0.34f, 1.3f*uiS);
        drawCenterS(L"BACK", pageX + pagePad, btnY + 15.0f*uiS, backW, 0.48f*uiS,
                    0.78f, 0.86f, 1.0f, 0.88f);

        drawRect(confirmX, btnY, confirmW, btnH,
                 (0.028f + accentR * 0.026f) * (ready ? 1.0f : 0.34f),
                 (0.034f + accentG * 0.022f) * (ready ? 1.0f : 0.34f),
                 (0.052f + accentB * 0.020f) * (ready ? 1.0f : 0.34f),
                 ready ? 0.84f : 0.46f);
        BatchFlush();
        SetGlowFx(true);
        drawConstellFrame(confirmX, btnY, confirmW, btnH, accentR, accentG, accentB,
                          ready ? (confirmHov ? 1.0f : 0.78f) : 0.18f,
                          16.0f*uiS, 5.0f*uiS, ready ? 0.16f : 0.0f);
        BatchFlush();
        SetGlowFx(false);
        const wchar_t* confirmLabel = ready ? L"EXECUTE RUN" : L"SELECT REQUIRED";
        drawCenterL(confirmLabel, confirmX,
                    btnY + (btnH - g_TextL.Height(confirmLabel, 0.70f*uiS)) * 0.5f,
                    confirmW, 0.70f*uiS, 1.0f, 1.0f, 1.0f, ready ? 0.98f : 0.42f);

        BatchFlush();
        g_BatchAlpha = 1.0f;
        return;
    }

    wchar_t coinBuf[64];
    swprintf_s(coinBuf, L"CREDITS :: %06lld", g_Coins);

    BindMainShader();
    g_TextL.Draw(L"RUN CONFIG", panelX, panelY + 8.0f * uiS, 1.00f * uiS,
                 0.72f, 0.82f, 1.0f, 0.98f);
    const wchar_t* pathText = L"C:\\ONEDOW\\RUN_CONFIG > WEAPON_SELECT";
    if (g_RunConfigStep >= 1) {
        if (s_TrialsEnabled)
            pathText = L"C:\\ONEDOW\\RUN_CONFIG > TRIAL_COUNT";
        else
            pathText = L"C:\\ONEDOW\\RUN_CONFIG > LOADOUT_READY";
    }
    g_TextS.Draw(pathText,
                 panelX, panelY + 58.0f * uiS, 0.56f * uiS,
                 0.48f, 0.58f, 0.78f, 0.86f);
    float coinSc = 0.58f * uiS;
    const float coinH = 34.0f * uiS;
    const float coinW = g_TextS.Width(coinBuf, coinSc) + 48.0f * uiS;
    const float coinX = panelX + panelW - coinW;
    const float coinY = panelY + 10.0f * uiS;
    drawRect(coinX, coinY, coinW, coinH, 0.030f, 0.034f, 0.050f, 0.62f);
    BatchFlush();
    SetGlowFx(true);
    drawConstellFrame(coinX, coinY, coinW, coinH, 1.0f, 0.84f, 0.22f,
                      0.58f, 8.0f * uiS, 2.4f * uiS, 0.08f);
    drawDiamond(coinX + 17.0f * uiS, coinY + coinH * 0.5f,
                4.0f * uiS, 1.0f, 0.84f, 0.22f, 0.86f);
    BatchFlush();
    SetGlowFx(false);
    g_TextS.Draw(coinBuf, coinX + 30.0f * uiS,
                 coinY + (coinH - g_TextS.Height(coinBuf, coinSc)) * 0.5f,
                 coinSc, 1.0f, 0.88f, 0.34f, 0.94f);

    // ── 좌측: 무기 선택 카드 목록 (고정 높이, 여유 간격) ─────────
    const float cH      = 100.0f * uiS;
    const float cGap    = 24.0f * uiS;
    const float cTopPad = 24.0f * uiS;
    const float cPad    = 20.0f * uiS;

    for (int i = 0; i < kWCCount; ++i) {
        float cx = leftX;
        float cy = colY + cTopPad + (float)i * (cH + cGap);
        bool hov = hit(cx, cy, leftW, cH);
        bool selected = (g_RunConfigStep >= 1 && s_WeaponSel == i);
        s_WeaponHover[i] = UiApproach(s_WeaponHover[i], hov ? 1.0f : 0.0f, dt, 10.0f);
        if (hov && lmb && !g_LmbPrev) { s_WeaponSel = i; g_RunConfigStep = 1; }

        const WDetailDef& wd = kWeaponDetail[i];
        float hv = s_WeaponHover[i];
        const float inactiveA = selected ? 1.0f : (0.30f + 0.10f * hv);
        const float inactiveR = selected ? 1.0f : 0.56f;
        const float inactiveG = selected ? 1.0f : 0.60f;
        const float inactiveB = selected ? 1.0f : 0.68f;

        // 카드 바디 (어두운 배경 — non-glow)
        BindMainShader();
        drawRect(cx - 5.0f*uiS, cy - 5.0f*uiS, leftW + 10.0f*uiS, cH + 10.0f*uiS,
                 wd.r, wd.g, wd.b, selected ? 0.10f : 0.018f * hv);
        drawRect(cx, cy, leftW, cH,
                 0.038f + wd.r * (selected ? 0.022f : 0.008f),
                 0.044f + wd.g * (selected ? 0.016f : 0.006f),
                 0.066f + wd.b * (selected ? 0.016f : 0.006f),
                 selected ? 0.78f : (0.42f + 0.08f * hv));

        // 컬러 스트라이프 + 별자리 프레임 (glow on)
        BatchFlush();
        SetGlowFx(true);
        drawRect(cx, cy, 5.0f*uiS, cH, wd.r, wd.g, wd.b,
                 selected ? 0.95f : 0.14f + 0.18f * hv);
        drawConstellFrame(cx, cy, leftW, cH, wd.r, wd.g, wd.b,
                          selected ? 0.80f : 0.12f + 0.22f * hv,
                          14.0f * uiS, 4.5f * uiS,
                          selected ? 0.14f : 0.0f);
        BatchFlush();
        SetGlowFx(false);

        // 텍스트
        g_TextL.Draw(kWC[i].name[nli], cx + cPad, cy + 13.0f*uiS,
                     0.96f * uiS, inactiveR, inactiveG, inactiveB,
                     selected ? 0.98f : inactiveA);
        drawFitS(kWC[i].desc[nli], cx + cPad, cy + 52.0f*uiS,
                 leftW - cPad - 14.0f*uiS, 0.46f*uiS, 0.34f*uiS,
                 selected ? 0.60f : 0.46f,
                 selected ? 0.72f : 0.50f,
                 selected ? 0.90f : 0.58f,
                 selected ? 0.76f : inactiveA);
        float roleW = g_TextS.Width(kWeaponRole[i][nli], 0.46f*uiS);
        g_TextS.Draw(kWeaponRole[i][nli], cx + leftW - roleW - cPad,
                     cy + cH - 20.0f*uiS, 0.46f*uiS,
                     selected ? wd.r : 0.50f,
                     selected ? wd.g : 0.54f,
                     selected ? wd.b : 0.62f,
                     selected ? 0.70f : inactiveA);
        if (selected) {
            float msw = g_TextS.Width(L"SELECTED", 0.40f*uiS);
            g_TextS.Draw(L"SELECTED", cx + leftW - msw - cPad, cy + 13.0f*uiS,
                         0.40f*uiS, wd.r, wd.g, wd.b, 0.80f);
        }
    }

    // ── 우측: 무기 상세 패널 (배경 카드 통합) ─────────────────────
    const float rDX  = midX + 12.0f * uiS;
    const float rDW  = (panelX + panelW) - rDX - 8.0f * uiS;
    const float rPad = 22.0f * uiS;

    // 전체 배경 카드 (반투명 — 배경화면 살짝 비침)
    BindMainShader();
    drawRect(rDX, colY, rDW, colH, 0.038f, 0.044f, 0.068f, 0.68f);
    BatchFlush();
    SetGlowFx(true);
    drawConstellFrame(rDX, colY, rDW, colH, accentR, accentG, accentB,
                      (g_RunConfigStep >= 1) ? 0.60f : 0.22f,
                      20.0f * uiS, 6.0f * uiS,
                      (g_RunConfigStep >= 1) ? 0.18f : 0.06f);
    BatchFlush();
    SetGlowFx(false);

    if (g_RunConfigStep < 1) {
        const wchar_t* ph = (li == 0) ? L"무기를 선택하세요" : L"SELECT A WEAPON";
        float phSc = 0.80f * uiS;
        float phW  = g_TextL.Width(ph, phSc);
        g_TextL.Draw(ph, rDX + (rDW - phW) * 0.5f, colY + colH * 0.44f,
                     phSc, 0.32f, 0.38f, 0.58f, 0.48f);
    } else {
        const WCardDef&   wc = kWC[s_WeaponSel];
        const WDetailDef& wd = kWeaponDetail[s_WeaponSel];
        const float wr = wd.r, wg = wd.g, wb = wd.b;
        float ry = colY + rPad;

        // 전환 페이드 — 이 블록 내 모든 드로에 panelContentA 적용
        BatchFlush();
        g_BatchAlpha = panelContentA;

        // 무기명 대형 + 역할 태그
        BindMainShader();
        g_TextL.Draw(wc.name[nli], rDX + rPad, ry, 1.10f*uiS, wr, wg, wb, 0.98f);
        g_TextS.Draw(kWeaponRole[s_WeaponSel][nli], rDX + rPad, ry + 44.0f*uiS,
                     0.48f*uiS, wr, wg, wb, 0.62f);

        // 프로파일 서브카드
        const float profY = ry + 66.0f * uiS;
        const float profH = 40.0f * uiS;
        const float profW = rDW - rPad * 2.0f;
        BindMainShader();
        drawRect(rDX + rPad, profY, profW, profH, 0.028f, 0.034f, 0.052f, 0.72f);
        drawRect(rDX + rPad, profY, 4.0f*uiS, profH, wr, wg, wb, 0.55f);
        BatchFlush();
        SetGlowFx(true);
        drawConstellFrame(rDX + rPad, profY, profW, profH, wr, wg, wb, 0.36f,
                          10.0f*uiS, 3.0f*uiS);
        BatchFlush();
        SetGlowFx(false);
        drawFitS(wd.profile[nli],
                 rDX + rPad + 16.0f*uiS,
                 profY + (profH - g_TextS.Height(wd.profile[nli], 0.52f*uiS)) * 0.5f,
                 profW - 26.0f*uiS, 0.52f*uiS, 0.40f*uiS,
                 0.68f, 0.80f, 0.96f, 0.84f);

        // 구분선 (glow)
        float divY1 = profY + profH + 16.0f * uiS;
        BatchFlush();
        SetGlowFx(true);
        drawRect(rDX + rPad, divY1, rDW - rPad * 2.0f, 1.0f*uiS, wr, wg, wb, 0.22f);
        BatchFlush();
        SetGlowFx(false);

        // 노트 서브카드
        const float noteCardY = divY1 + 10.0f * uiS;
        const float noteLineH = 30.0f * uiS;
        const float noteCardH = 3.0f * noteLineH + 16.0f * uiS;
        const float noteW     = rDW - rPad * 2.0f;
        BindMainShader();
        drawRect(rDX + rPad, noteCardY, noteW, noteCardH, 0.028f, 0.034f, 0.052f, 0.68f);
        BatchFlush();
        SetGlowFx(true);
        drawConstellFrame(rDX + rPad, noteCardY, noteW, noteCardH, wr, wg, wb, 0.28f,
                          10.0f*uiS, 3.0f*uiS);
        // bullet 다이아몬드
        for (int i = 0; i < 3; ++i) {
            float ny = noteCardY + 8.0f*uiS + (float)i * noteLineH;
            drawDiamond(rDX + rPad + 14.0f*uiS, ny + noteLineH * 0.5f - 2.0f*uiS,
                        5.0f*uiS, wr, wg, wb, 0.62f);
        }
        BatchFlush();
        SetGlowFx(false);
        for (int i = 0; i < 3; ++i) {
            float ny = noteCardY + 8.0f*uiS + (float)i * noteLineH;
            drawFitS(wd.notes[nli][i], rDX + rPad + 26.0f*uiS, ny + 6.0f*uiS,
                     noteW - 36.0f*uiS, 0.50f*uiS, 0.38f*uiS,
                     0.68f, 0.78f, 0.96f, 0.84f);
        }

        // 구분선 (glow)
        float divY2 = noteCardY + noteCardH + 14.0f * uiS;
        BatchFlush();
        SetGlowFx(true);
        drawRect(rDX + rPad, divY2, rDW - rPad * 2.0f, 1.0f*uiS, wr, wg, wb, 0.18f);
        BatchFlush();
        SetGlowFx(false);

        // ── 레이더 차트 + 우측 수치 리스트 ────────────────────────
        const float statsY  = divY2 + 10.0f * uiS;
        const float fullW   = rDW - rPad * 2.0f;
        const float radarSW = fullW * 0.46f;
        const float radarH  = 210.0f * uiS;
        const float radarCX = rDX + rPad + radarSW * 0.5f;
        const float radarCY = statsY + radarH * 0.5f;
        const float radarR  = 72.0f * uiS;

        // 6축 각도 + 최대점 좌표
        const int RN = 6;
        float axAng[RN], axTX[RN], axTY[RN];
        for (int i = 0; i < RN; ++i) {
            axAng[i] = -(float)M_PI * 0.5f + (float)i * 2.0f * (float)M_PI / (float)RN;
            axTX[i]  = radarCX + cosf(axAng[i]) * radarR;
            axTY[i]  = radarCY + sinf(axAng[i]) * radarR;
        }

        // 임의 각도 선 그리기 (BatchTri 기반)
        auto drawLine2 = [](float x1, float y1, float x2, float y2,
                            float r, float g, float b, float a, float thick) {
            float dx = x2-x1, dy = y2-y1;
            float len = sqrtf(dx*dx + dy*dy); if (len < 0.5f) return;
            float nx = -dy/len*thick*0.5f, ny = dx/len*thick*0.5f;
            BatchTri(x1+nx, y1+ny, x1-nx, y1-ny, x2+nx, y2+ny, r, g, b, a);
            BatchTri(x1-nx, y1-ny, x2-nx, y2-ny, x2+nx, y2+ny, r, g, b, a);
        };

        // 그리드 링 4개 (어두운 육각형)
        BindMainShader();
        for (int ring = 1; ring <= 4; ++ring) {
            float rr = radarR * (float)ring * 0.25f;
            float ga = 0.07f + 0.05f * (float)ring;
            for (int i = 0; i < RN; ++i) {
                int j = (i+1) % RN;
                float px = radarCX + cosf(axAng[i])*rr, py = radarCY + sinf(axAng[i])*rr;
                float qx = radarCX + cosf(axAng[j])*rr, qy = radarCY + sinf(axAng[j])*rr;
                drawLine2(px, py, qx, qy, wr, wg, wb, ga, 1.0f);
            }
        }
        // 6개 축 선
        for (int i = 0; i < RN; ++i)
            drawLine2(radarCX, radarCY, axTX[i], axTY[i], wr, wg, wb, 0.18f, 1.0f);

        // 스탯 폴리곤 채움 + 외곽선 + 버텍스 노드 (glow)
        BatchFlush();
        SetGlowFx(true);
        for (int i = 0; i < RN; ++i) {
            int j = (i+1) % RN;
            float px = radarCX + cosf(axAng[i])*radarR*s_RadarCur[i];
            float py = radarCY + sinf(axAng[i])*radarR*s_RadarCur[i];
            float qx = radarCX + cosf(axAng[j])*radarR*s_RadarCur[j];
            float qy = radarCY + sinf(axAng[j])*radarR*s_RadarCur[j];
            BatchTri(radarCX, radarCY, px, py, qx, qy, wr, wg, wb, 0.24f);
        }
        for (int i = 0; i < RN; ++i) {
            int j = (i+1) % RN;
            float px = radarCX + cosf(axAng[i])*radarR*s_RadarCur[i];
            float py = radarCY + sinf(axAng[i])*radarR*s_RadarCur[i];
            float qx = radarCX + cosf(axAng[j])*radarR*s_RadarCur[j];
            float qy = radarCY + sinf(axAng[j])*radarR*s_RadarCur[j];
            drawLine2(px, py, qx, qy, wr, wg, wb, 0.88f, 1.8f);
        }
        for (int i = 0; i < RN; ++i) {
            float px = radarCX + cosf(axAng[i])*radarR*s_RadarCur[i];
            float py = radarCY + sinf(axAng[i])*radarR*s_RadarCur[i];
            drawDiamond(px, py, 5.0f*uiS, wr, wg, wb, 0.92f);
        }
        BatchFlush();
        SetGlowFx(false);

        // 축 레이블 (각 꼭짓점 바깥)
        for (int i = 0; i < RN; ++i) {
            float tox = cosf(axAng[i]), toy = sinf(axAng[i]);
            float lx = radarCX + tox * (radarR + 14.0f*uiS);
            float ly = radarCY + toy * (radarR + 14.0f*uiS);
            float sc = 0.36f * uiS;
            float lw = g_TextS.Width(kWeaponStatLabels[i][nli], sc);
            float lh = g_TextS.Height(kWeaponStatLabels[i][nli], sc);
            float tx = lx - lw * (0.5f - tox * 0.5f);
            float ty = ly - lh * (0.5f - toy * 0.5f);
            g_TextS.Draw(kWeaponStatLabels[i][nli], tx, ty, sc, 0.92f, 0.96f, 1.0f, 0.84f);
        }

        // 우측 수치 리스트 (6행)
        const float listX  = rDX + rPad + radarSW + 10.0f*uiS;
        const float listW  = fullW - radarSW - 10.0f*uiS;
        const float listRH = radarH / 6.0f;
        BindMainShader();
        drawRect(listX - 8.0f*uiS, statsY + 10.0f*uiS, 1.0f*uiS, radarH - 20.0f*uiS,
                 0.82f, 0.90f, 1.0f, 0.18f);
        for (int i = 0; i < 6; ++i) {
            float lry = statsY + (float)i * listRH;
            BindMainShader();
            drawRect(listX, lry + 2.0f*uiS, listW, listRH - 4.0f*uiS,
                     0.028f + wr*0.010f, 0.034f + wg*0.008f, 0.052f + wb*0.008f, 0.70f);
            drawRect(listX, lry + 2.0f*uiS, 3.0f*uiS, listRH - 4.0f*uiS,
                     wr, wg, wb, 0.52f);
            g_TextS.Draw(kWeaponStatLabels[i][nli], listX + 10.0f*uiS, lry + 5.0f*uiS,
                         0.36f*uiS, 0.78f, 0.88f, 1.0f, 0.82f);
            const wchar_t* statText = wd.stat[nli][i];
            wchar_t glitchBuf[64];
            float statValueA = 0.90f;
            if (s_StatGlitchT < 1.0f) {
                const wchar_t* glyphs = L"0123456789ABCDEF";
                int tick = (int)(now * 54.0f);
                int p = 0;
                for (; statText[p] && p < 63; ++p) {
                    wchar_t ch = statText[p];
                    bool digit = (ch >= L'0' && ch <= L'9');
                    bool upper = (ch >= L'A' && ch <= L'Z');
                    bool lower = (ch >= L'a' && ch <= L'z');
                    if (digit || ((upper || lower) && s_StatGlitchT < 0.72f)) {
                        glitchBuf[p] = glyphs[(tick + s_WeaponSel * 17 + i * 11 + p * 5) & 15];
                    } else {
                        glitchBuf[p] = ch;
                    }
                }
                glitchBuf[p] = 0;
                statText = glitchBuf;
                statValueA = 0.76f + 0.18f * (float)((tick + i) & 1);
            }
            drawFitS(statText, listX + 10.0f*uiS, lry + listRH * 0.42f,
                     listW - 16.0f*uiS, 0.48f*uiS, 0.34f*uiS,
                     0.92f, 0.96f, 1.0f, statValueA);
        }

        // ── 시련 토글 서브카드 ────────────────────────────────────
        const float trialCardY = statsY + radarH + 16.0f * uiS;
        const float trialCardH = 66.0f * uiS;
        const float trialW     = rDW - rPad * 2.0f;
        const float trialX     = rDX + rPad;
        const float stepW      = 154.0f * uiS;
        const float stepH      = 38.0f * uiS;
        const float stepX      = trialX + trialW - stepW - 18.0f * uiS;
        const float stepY      = trialCardY + (trialCardH - stepH) * 0.5f;
        const float arrowW     = 36.0f * uiS;
        const bool decHov      = s_TrialsEnabled && hit(stepX, stepY, arrowW, stepH);
        const bool incHov      = s_TrialsEnabled && hit(stepX + stepW - arrowW, stepY, arrowW, stepH);
        const bool stepHov     = s_TrialsEnabled && hit(stepX, stepY, stepW, stepH);
        const float toggleW    = trialW - (s_TrialsEnabled ? (stepW + 34.0f * uiS) : 0.0f);
        bool trialToggleHov = hit(trialX, trialCardY, toggleW, trialCardH);
        bool trialHov = trialToggleHov || stepHov;
        if (trialToggleHov && lmb && !g_LmbPrev) {
            s_TrialsEnabled = !s_TrialsEnabled;
            if (!s_TrialsEnabled) {
                clearTrialSelection();
                s_TrialSelectPage = false;
            }
        }
        if (s_TrialsEnabled && lmb && !g_LmbPrev) {
            if (decHov) {
                s_TrialTargetCount = std::max(1, s_TrialTargetCount - 1);
                clearTrialSelection();
            } else if (incHov) {
                s_TrialTargetCount = std::min(TRIAL_SLOT_COUNT, s_TrialTargetCount + 1);
                clearTrialSelection();
            }
        }

        float scoreMult = 1.0f;
        wchar_t rateBuf[32];
        swprintf_s(rateBuf, L"x%.2f", scoreMult);
        float rateSc = 0.52f*uiS;

        BindMainShader();
        drawRect(trialX, trialCardY, trialW, trialCardH,
                 0.018f, 0.022f, 0.036f, trialHov ? 0.46f : 0.34f);
        drawRect(trialX, trialCardY, trialW, 1.0f*uiS,
                 wr, wg, wb, s_TrialsEnabled ? 0.34f : 0.10f);
        drawRect(trialX, trialCardY + trialCardH - 1.0f*uiS, trialW, 1.0f*uiS,
                 wr, wg, wb, s_TrialsEnabled ? 0.22f : 0.06f);
        if (s_TrialsEnabled) {
            drawRect(trialX, trialCardY, 4.0f*uiS, trialCardH,
                     wr, wg, wb, 0.52f);
        }
        const float checkSc = 0.58f * uiS;
        const float checkX = trialX + 18.0f * uiS;
        const float checkY = trialCardY + 12.0f * uiS;
        const wchar_t* checkText = L"[   ]";
        g_TextS.Draw(checkText, checkX, checkY, checkSc,
                     s_TrialsEnabled ? 0.94f : 0.46f,
                     s_TrialsEnabled ? 1.00f : 0.50f,
                     s_TrialsEnabled ? 1.00f : 0.58f,
                     s_TrialsEnabled ? 0.96f : 0.54f);
        if (s_TrialsEnabled) {
            BindMainShader();
            drawRect(checkX + 8.0f*uiS, checkY + 7.0f*uiS,
                     7.0f*uiS, 7.0f*uiS, wr, wg, wb, 0.92f);
        }

        const wchar_t* stateText = s_TrialsEnabled
            ? L"TRIAL MODULE : ACTIVE"
            : L"TRIAL MODULE : DISABLED";
        const wchar_t* hintText = s_TrialsEnabled
            ? L"SET TRIAL COUNT BEFORE LAUNCH"
            : L"OPTIONAL RISK LAYER STANDBY";
        const float stateX = checkX + 72.0f * uiS;
        const float rateW = g_TextL.Width(rateBuf, rateSc);
        const float rateX = trialX + trialW - rateW - 64.0f*uiS;
        const float textLimitX = s_TrialsEnabled ? stepX : rateX;
        drawFitS(stateText, stateX, checkY,
                 textLimitX - stateX - 18.0f*uiS, 0.52f*uiS, 0.38f*uiS,
                 s_TrialsEnabled ? wr : 0.52f,
                 s_TrialsEnabled ? wg : 0.58f,
                 s_TrialsEnabled ? wb : 0.68f,
                 s_TrialsEnabled ? 0.96f : 0.62f);
        drawFitS(hintText, stateX, trialCardY + 38.0f*uiS,
                 textLimitX - stateX - 18.0f*uiS, 0.40f*uiS, 0.32f*uiS,
                 0.50f, 0.60f, 0.78f, s_TrialsEnabled ? 0.70f : 0.44f);

        BindMainShader();
        if (s_TrialsEnabled) {
            drawRect(stepX, stepY, stepW, stepH, 0.018f, 0.024f, 0.038f, stepHov ? 0.64f : 0.46f);
            drawRect(stepX, stepY, stepW, 1.0f*uiS, wr, wg, wb, 0.34f);
            drawRect(stepX, stepY + stepH - 1.0f*uiS, stepW, 1.0f*uiS, wr, wg, wb, 0.22f);
            drawCenterL(L"<", stepX, stepY + 5.0f*uiS, arrowW, 0.58f*uiS,
                        decHov ? 1.0f : 0.58f, decHov ? 1.0f : 0.70f, decHov ? 1.0f : 0.90f, 0.92f);
            drawCenterL(L">", stepX + stepW - arrowW, stepY + 5.0f*uiS, arrowW, 0.58f*uiS,
                        incHov ? 1.0f : 0.58f, incHov ? 1.0f : 0.70f, incHov ? 1.0f : 0.90f, 0.92f);
            wchar_t countBuf[16];
            swprintf_s(countBuf, L"%02d", s_TrialTargetCount);
            drawCenterL(countBuf, stepX + arrowW, stepY + 4.0f*uiS, stepW - arrowW * 2.0f,
                        0.60f*uiS, wr, wg, wb, 0.98f);
        } else {
            g_TextL.Draw(rateBuf, rateX,
                         trialCardY + (trialCardH - g_TextL.Height(rateBuf, rateSc)) * 0.5f, rateSc,
                         0.38f, 0.54f, 0.72f, 0.92f);
        }

        // 전환 페이드 복구
        BatchFlush();
        g_BatchAlpha = 1.0f;
    }


    if (UIButton(panelX, footY, 168.0f * uiS, 50.0f * uiS, T(StrId::BTN_BACK),
                 mx, my, lmb, g_LmbPrev)) {
        ResetRunConfigUi();
        g_GameManager.currentState = GameState::MAIN_MENU;
    }

    const float execW = 292.0f * uiS;
    const float execH = 54.0f * uiS;
    const float execX = panelX + panelW - execW;
    const float execY = footY - 2.0f * uiS;
    const bool execReady = (g_RunConfigStep >= 1);
    const bool execHov = execReady && hit(execX, execY, execW, execH);
    const bool execClick = execReady && execHov && g_LmbPrev && !lmb;
    BindMainShader();
    float eDim = execReady ? 1.0f : 0.36f;
    if (execReady) {
        drawRect(execX - 9.0f * uiS, execY - 7.0f * uiS,
                 execW + 18.0f * uiS, execH + 14.0f * uiS,
                 accentR, accentG, accentB, execHov ? 0.18f : 0.08f);
    }
    drawRect(execX, execY, execW, execH,
             (0.028f + accentR * 0.026f) * eDim,
             (0.034f + accentG * 0.022f) * eDim,
             (0.052f + accentB * 0.020f) * eDim,
             execReady ? 0.84f : 0.52f);
    if (execHov) {
        drawRect(execX + 4.0f * uiS, execY + 4.0f * uiS,
                 execW - 8.0f * uiS, execH - 8.0f * uiS,
                 accentR, accentG, accentB, 0.58f);
    }
    BatchFlush();
    SetGlowFx(true);
    drawConstellFrame(execX, execY, execW, execH, accentR, accentG, accentB,
                      execReady ? (execHov ? 1.00f : 0.78f) : 0.20f,
                      16.0f * uiS, 5.0f * uiS,
                      execReady ? (execHov ? 0.26f : 0.16f) : 0.03f);
    if (execReady) {
        drawRect(execX + 18.0f*uiS, execY + execH - 8.0f*uiS,
                 execW - 36.0f*uiS, 2.0f*uiS,
                 accentR, accentG, accentB, execHov ? 0.92f : 0.54f);
    }
    BatchFlush();
    SetGlowFx(false);
    const wchar_t* execLabel = execReady
        ? (s_TrialsEnabled ? L"SELECT TRIALS" : L"EXECUTE RUN")
        : L"CONFIG LOCKED";
    const float execSc = 0.78f * uiS;
    drawCenterL(execLabel,
                execX, execY + (execH - g_TextL.Height(execLabel, execSc)) * 0.5f,
                execW, execSc,
                execHov ? 0.04f : 1.0f,
                execHov ? 0.06f : 1.0f,
                execHov ? 0.08f : 1.0f,
                execReady ? 0.99f : 0.42f);
    if (execClick) {
        if (s_TrialsEnabled) {
            clearTrialSelection();
            s_TrialSelectPage = true;
        } else {
            startConfiguredRun();
        }
    }

    const float devW = 128.0f * uiS;
    const float devX = execX - devW - 18.0f * uiS;
    bool devHov = hit(devX, footY, devW, 50.0f * uiS);
    if (devHov && lmb && !g_LmbPrev)
        g_CreativeMode = !g_CreativeMode;
    BindMainShader();
    bool devOn = g_CreativeMode;
    drawRect(devX, footY, devW, 50.0f * uiS,
             devOn ? 0.13f : 0.05f,
             devOn ? 0.08f : 0.06f,
             devOn ? 0.03f : 0.08f, 0.92f);
    drawBorder(devX, footY, devW, 50.0f * uiS,
               devOn ? 1.0f : 0.30f,
               devOn ? 0.55f : 0.40f,
               devOn ? 0.10f : 0.55f,
               devOn ? 0.86f : (devHov ? 0.34f : 0.20f), 1.5f * uiS);
    drawCenterS(devOn ? L"DEV ON" : L"DEV OFF", devX, footY + 15.0f * uiS, devW,
                0.54f * uiS,
                devOn ? 1.0f : 0.55f,
                devOn ? 0.68f : 0.62f,
                devOn ? 0.18f : 0.78f, 0.90f);

    BatchFlush();
    g_BatchAlpha = 1.0f;
}

void Scene_CreativeConfig(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    GLFWwindow* window = c.window;
    const GameState st = g_GameManager.currentState;
    float& fireTimer = *c.fireTimer;
    const std::function<void()>& ResetForNewGame = c.reset;
                DrawMenuBackground(sw, sh, delta);
                BindMainShader();

                const float topY = 44.0f;
                const wchar_t* TIT = L"CREATIVE";
                g_TextL.Draw(TIT, (sw - g_TextL.Width(TIT, 1.4f)) * 0.5f,
                             topY, 1.4f, 0.85f, 0.95f, 0.6f, 1.0f);

                const float OBW = 130.0f, OBH = 44.0f, OBG = 10.0f;
                const float leftX = 40.0f;
                const float contentTop = topY + 52.0f;
                const float footY = sh - 64.0f;

                g_TextS.Draw(L"Start Score", leftX, contentTop, 0.95f, 1, 1, 1, 0.9f);
                struct ScoreOpt { const wchar_t* l; long long v; };
                ScoreOpt sOpts[5] = { {L"0",0},{L"200k",200000},{L"400k",400000},{L"500k",500000} };
                for (int i = 0; i < 4; i++) {
                    float ox = leftX + i * (OBW + OBG);
                    bool sel = (g_CreativeStartScore == sOpts[i].v);
                    if (UIButton(ox, contentTop + 28.0f, OBW, OBH, sOpts[i].l,
                                 mx, my, lmb, g_LmbPrev, sel))
                        g_CreativeStartScore = sOpts[i].v;
                }

                g_TextS.Draw(L"Boss", leftX, contentTop + 90.0f, 1.0f, 1, 1, 1, 0.9f);
                g_TextS.Draw(L"(런 중 B = 선택 보스 즉시 스폰)", leftX, contentTop + 108.0f,
                             0.52f, 0.72f, 0.78f, 0.88f, 0.85f);
                g_TextS.Draw(L"* 일반 런 미포함 (개발용)", leftX, contentTop + 124.0f,
                             0.48f, 0.68f, 0.75f, 0.82f, 0.75f);
                struct BossOpt { const wchar_t* l; int v; };
                BossOpt bOpts[5] = {
                    {L"None",-1}, {L"VOLLEY.sys",2}, {L"TESS.glitch",10},
                    {L"FORK.worm",8}, {L"ETHER_SWORD",20}
                };
                const float BBW = 112.0f;
                for (int i = 0; i < 5; i++) {
                    int col = i % 3, row = i / 3;
                    float ox = leftX + col * (BBW + OBG);
                    float oy = contentTop + 144.0f + row * (OBH + 8.0f);
                    bool sel = (g_CreativeBossPick == bOpts[i].v);
                    if (UIButton(ox, oy, BBW, OBH, bOpts[i].l,
                                 mx, my, lmb, g_LmbPrev, sel))
                        g_CreativeBossPick = bOpts[i].v;
                }

                g_TextS.Draw(L"Start Augments", leftX, contentTop + 236.0f, 1.0f, 1, 1, 1, 0.9f);
                int aOpts[4] = { 0, 3, 5, 10 };
                for (int i = 0; i < 4; i++) {
                    float ox = leftX + i * (OBW + OBG);
                    wchar_t lb[8]; swprintf_s(lb, L"%d", aOpts[i]);
                    bool sel = (g_CreativeStartAugs == aOpts[i]);
                    if (UIButton(ox, contentTop + 264.0f, OBW, OBH, lb,
                                 mx, my, lmb, g_LmbPrev, sel))
                        g_CreativeStartAugs = aOpts[i];
                }

                {
                    const float LIST_W = std::min(820.0f, sw * 0.46f);
                    float gx = sw - LIST_W - 40.0f;
                    if (gx < leftX + 440.0f) gx = leftX + 440.0f;
                    g_TextS.Draw(L"Pick Start Augments (click)", gx, contentTop, 0.95f, 1, 1, 1, 0.9f);
                    int avail[AUG_TOTAL], na = 0;
                    for (int i = 0; i < AUG_TOTAL; i++) {
                        if (AugRemoved(ALL_AUGS[i].type)) continue;
                        avail[na++] = i;
                    }
                    std::sort(avail, avail + na, [](int a, int b) {
                        return AugListIndexLess(a, b);
                    });
                    const float ROW_H = 24.0f, HDR_H = 22.0f;
                    float gTop = contentTop + 28.0f;
                    float gBottom = footY - 12.0f;
                    float viewH = gBottom - gTop;
                    int hdrCount = 0;
                    AugListGroup prevGrp = (AugListGroup)-1;
                    for (int k = 0; k < na; k++) {
                        AugListGroup g = AugListGroupOf(ALL_AUGS[avail[k]]);
                        if (g != prevGrp) { hdrCount++; prevGrp = g; }
                    }
                    float contentH = (float)na * ROW_H + (float)hdrCount * HDR_H;
                    static float s_caScroll = 0.0f;
                    bool over = (mx >= gx && mx <= gx + LIST_W && my >= gTop && my <= gBottom);
                    if (over && g_ScrollAccum != 0.0f) s_caScroll -= g_ScrollAccum * ROW_H * 1.5f;
                    g_ScrollAccum = 0.0f;
                    float maxS = (contentH > viewH) ? (contentH - viewH) : 0.0f;
                    if (s_caScroll < 0) s_caScroll = 0; if (s_caScroll > maxS) s_caScroll = maxS;
                    BatchFlush(); glEnable(GL_SCISSOR_TEST);
                    glScissor((GLint)gx, (GLint)(sh - gBottom), (GLint)(LIST_W + 4), (GLint)viewH);
                    prevGrp = (AugListGroup)-1;
                    float ry = gTop - s_caScroll;
                    for (int k = 0; k < na; k++) {
                        int i = avail[k];
                        AugListGroup grp = AugListGroupOf(ALL_AUGS[i]);
                        if (grp != prevGrp) {
                            if (ry >= gTop - HDR_H && ry <= gBottom)
                                g_TextS.Draw(AugListGroupLabel(grp), gx, ry, 0.72f,
                                             0.55f, 0.75f, 0.95f, 0.88f);
                            ry += HDR_H;
                            prevGrp = grp;
                        }
                        if (ry >= gTop - ROW_H && ry <= gBottom) {
                            int selPos = -1;
                            for (int s = 0; s < (int)g_CreativeStartAugList.size(); s++)
                                if (g_CreativeStartAugList[s] == i) { selPos = s; break; }
                            bool selected = (selPos >= 0);
                            bool hv = (over && my >= ry - 2.0f && my < ry + ROW_H - 4.0f);
                            float rr, rg, rb;
                            GetRarityColor(ALL_AUGS[i].rarity, rr, rg, rb);
                            if (selected || hv) {
                                BindMainShader();
                                drawRect(gx, ry - 2.0f, LIST_W, ROW_H,
                                         selected ? 0.18f : 0.12f,
                                         selected ? 0.22f : 0.14f,
                                         selected ? 0.30f : 0.18f, 0.75f);
                                if (selected)
                                    drawRect(gx, ry - 2.0f, 3.0f, ROW_H, rr, rg, rb, 1.0f);
                            }
                            wchar_t line[128];
                            swprintf_s(line, L"%ls [%ls] %ls",
                                       selected ? L"v" : L"-",
                                       GetAugBadge(ALL_AUGS[i]), AugName(ALL_AUGS[i]));
                            float rowSc = 0.76f;
                            while (rowSc > 0.60f &&
                                   g_TextS.Width(line, rowSc) > LIST_W - 8.0f) rowSc -= 0.03f;
                            g_TextS.Draw(line, gx + 4.0f, ry, rowSc, rr, rg, rb, selected ? 1.0f : 0.88f);
                            if (hv && lmb && !g_LmbPrev) {
                                if (selected) g_CreativeStartAugList.erase(
                                    g_CreativeStartAugList.begin() + selPos);
                                else          g_CreativeStartAugList.push_back(i);
                            }
                        }
                        ry += ROW_H;
                    }
                    BatchFlush(); glDisable(GL_SCISSOR_TEST);
                    wchar_t cb[48]; swprintf_s(cb, L"selected: %d", (int)g_CreativeStartAugList.size());
                    g_TextS.Draw(cb, gx, gBottom + 10.0f, 0.85f, 1.0f, 0.9f, 0.4f, 0.95f);
                }

                if (UIButton((sw - 280.0f) * 0.5f, footY, 280.0f, 52.0f,
                             L"START", mx, my, lmb, g_LmbPrev)) {
                    ResetForNewGame();
                    FinalizeLoadout(c, FixedWeaponForSelectedJob());
                }

                if (UIButton(40.0f, footY, 160.0f, 48.0f, T(StrId::BTN_BACK),
                             mx, my, lmb, g_LmbPrev)) {
                    g_GameManager.currentState = GameState::RUN_CONFIG;
                }
}

void Scene_Settings(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    GLFWwindow* window = c.window;
    (void)c.fireTimer;
    (void)c.reset;

    static int   s_tab         = 0;
    static int   s_prevTab     = 0;
    static float s_tabHover[3] = {};
    static float s_tabFocus[3] = {};
    static float s_panelT      = 1.0f;
    static bool  s_showCredits = false;
    static bool  s_bsPrev      = false;
    static bool  s_enPrev      = false;
    static bool  s_resetDialog          = false;
    static bool  s_resetDialogJustOpened = false;
    static bool  s_resetItems[4] = {};
    static bool  s_resetDone   = false;

    struct SettingsTabDef {
        const wchar_t* id;
        const wchar_t* name[2];
        const wchar_t* brief[2];
        float r, g, b;
    };
    static const SettingsTabDef kTabs[] = {
        { L"GAME",   { L"게임",   L"GAME"   },
          { L"화면, 언어, 전투 보조, HUD",    L"Screen, language, combat assist, HUD" },
          0.42f, 0.62f, 1.00f },
        { L"AUDIO",  { L"오디오", L"AUDIO"  },
          { L"음량과 사운드 채널",          L"Volume and sound channels" },
          1.00f, 0.76f, 0.30f },
        { L"SYSTEM", { L"시스템", L"SYSTEM" },
          { L"저장 기록, 리셋, 크레딧", L"Save, records, reset, credits" },
          1.00f, 0.45f, 0.42f },
    };
    constexpr int kTabCount = 3;
    if (s_tab < 0 || s_tab >= kTabCount) s_tab = 0;

    const bool settingsOverlay = (g_SettingsReturnTo == GameState::PAUSED);
    if (!settingsOverlay) DrawMenuBackground(sw, sh, delta);

    // Entry animation timer
    float dt = delta; if (dt > 0.05f) dt = 0.05f;
    g_SettingsEntryT += dt;
    if (g_SettingsEntryT > 1.0f) g_SettingsEntryT = 1.0f;
    const float now  = (float)glfwGetTime();

    const float wake = Smoothstep(std::min(1.0f, g_SettingsEntryT / 0.52f));
    float cardWake[3];
    for (int i = 0; i < 3; ++i)
        cardWake[i] = Smoothstep(std::min(1.0f,
            std::max(0.0f, g_SettingsEntryT - (float)i * 0.09f) / 0.40f));
    const float rightWake = Smoothstep(std::min(1.0f,
        std::max(0.0f, g_SettingsEntryT - 0.10f) / 0.44f));

    int li = LangIndex();
    if (li < 0 || li >= 3) li = 0;
    const int nli = (li == 0) ? 0 : 1;

    // RunConfig-style raw panel layout
    const float TARGET_W = 1640.0f;
    const float TARGET_H = 910.0f;
    float uiS = std::max(0.72f, std::min(sw * 0.94f / TARGET_W, sh * 0.90f / TARGET_H));
    if (uiS > 1.30f) uiS = 1.30f;
    const float panelW  = TARGET_W * uiS;
    const float panelH  = TARGET_H * uiS;
    const float panelX  = (sw - panelW) * 0.5f;
    const float panelY  = (sh - panelH) * 0.5f;
    const float headerH = 86.0f * uiS;
    const float footerH = 72.0f * uiS;
    const float bodyY   = panelY + headerH;
    const float bodyH   = panelH - headerH - footerH;
    const float leftW   = panelW * 0.235f;
    const float colGap  = 20.0f * uiS;
    const float leftX   = panelX;
    const float rightX  = leftX + leftW + colGap;
    const float rightW  = panelW - leftW - colGap;
    const float footY   = panelY + panelH - footerH + 14.0f * uiS;

    const SettingsTabDef& tab = kTabs[s_tab];

    auto hit = [&](float x, float y, float w, float h) {
        return mx >= x && mx <= x + w && my >= y && my <= y + h;
    };
    const bool modalActive = s_resetDialog || s_resetDone;
    auto clampVol = [](int v) { return v < 0 ? 0 : (v > 100 ? 100 : v); };
    auto commitVol = [&]() {
        int v = 0;
        for (int i = 0; i < g_VolLen; ++i) v = v * 10 + (g_VolBuf[i] - L'0');
        if (g_VolLen > 0) g_SoundVol = clampVol(v);
        g_VolEdit = false;
    };
    auto drawBorder = [](float x, float y, float w, float h,
                         float r, float g, float b, float a, float t) {
        drawRect(x, y, w, t, r, g, b, a);
        drawRect(x, y + h - t, w, t, r, g, b, a);
        drawRect(x, y, t, h, r, g, b, a);
        drawRect(x + w - t, y, t, h, r, g, b, a);
    };
    auto drawFitS = [&](const wchar_t* text, float x, float y, float maxW,
                        float sc, float minSc, float r, float g, float b, float a) {
        while (sc > minSc && g_TextS.Width(text, sc) > maxW)
            sc -= 0.025f * uiS;
        g_TextS.Draw(text, x, y, sc, r, g, b, a);
    };
    auto drawCenterS = [&](const wchar_t* text, float x, float y, float w,
                           float sc, float r, float g, float b, float a) {
        while (sc > 0.36f * uiS && g_TextS.Width(text, sc) > w - 12.0f * uiS)
            sc -= 0.025f * uiS;
        float tw = g_TextS.Width(text, sc);
        g_TextS.Draw(text, x + (w - tw) * 0.5f, y, sc, r, g, b, a);
    };
    auto segment = [&](float x, float y, float w, float h, const wchar_t* label,
                       bool selected, float r, float g, float b, bool enabled = true) {
        bool hov = enabled && hit(x, y, w, h);
        float dim = enabled ? 1.0f : 0.36f;
        float ctlA = rightWake * (0.58f + 0.42f * Smoothstep(s_panelT));
        BindMainShader();
        drawRect(x, y, w, h,
                 (0.045f + r * (selected ? 0.105f : 0.030f)) * dim,
                 (0.052f + g * (selected ? 0.085f : 0.028f)) * dim,
                 (0.070f + b * (selected ? 0.070f : 0.024f)) * dim,
                 (enabled ? 0.94f : 0.64f) * ctlA);
        // Corner brackets + diamond nodes instead of full border
        float ba = (enabled ? (selected ? 0.88f : (hov ? 0.55f : 0.24f)) : 0.12f) * ctlA;
        float cL = std::min(w * 0.28f, 9.0f * uiS);
        float ct = 1.3f * uiS;
        drawRect(x,       y,       cL, ct, r, g, b, ba);
        drawRect(x,       y,       ct, cL, r, g, b, ba);
        drawRect(x+w-cL,  y,       cL, ct, r, g, b, ba);
        drawRect(x+w-ct,  y,       ct, cL, r, g, b, ba);
        drawRect(x,       y+h-ct,  cL, ct, r, g, b, ba);
        drawRect(x,       y+h-cL,  ct, cL, r, g, b, ba);
        drawRect(x+w-cL,  y+h-ct,  cL, ct, r, g, b, ba);
        drawRect(x+w-ct,  y+h-cL,  ct, cL, r, g, b, ba);
        float ns = (selected ? 4.5f : 3.0f) * uiS;
        drawDiamond(x,   y,   ns, r, g, b, ba);
        drawDiamond(x+w, y,   ns, r, g, b, ba);
        drawDiamond(x,   y+h, ns, r, g, b, ba);
        drawDiamond(x+w, y+h, ns, r, g, b, ba);
        drawCenterS(label, x, y + 12.0f * uiS, w, 0.45f * uiS,
                    enabled ? (selected ? 1.0f : 0.70f) : 0.40f,
                    enabled ? (selected ? 1.0f : 0.78f) : 0.44f,
                    enabled ? (selected ? 1.0f : 0.90f) : 0.52f,
                    (enabled ? 0.96f : 0.52f) * ctlA);
        return hov && lmb && !g_LmbPrev && !modalActive;
    };

    if (s_prevTab != s_tab) {
        s_prevTab = s_tab;
        s_panelT  = 0.0f;
    }
    s_panelT = UiApproach(s_panelT, 1.0f, delta, 10.0f);
    if (s_tab != 1 && g_VolEdit) commitVol();

    // === HEADER ===
    const wchar_t* langShort[3] = { L"KR", L"EN", L"JP" };
    wchar_t fpsBuf[24];
    if (g_FpsCap == 0) swprintf_s(fpsBuf, L"VSYNC");
    else swprintf_s(fpsBuf, L"%d FPS", g_FpsCap);
    wchar_t profileBuf[96];
    swprintf_s(profileBuf, L"%ls / %ls / SOUND %d", langShort[li], fpsBuf, g_SoundVol);

    const float hSlide = (1.0f - wake) * 16.0f * uiS;
    BindMainShader();
    g_TextL.Draw(L"SETTINGS",
                 leftX, panelY + 8.0f * uiS - hSlide,
                 0.88f * uiS, 1.0f, 1.0f, 1.0f, 0.98f * wake);
    g_TextS.Draw(nli == 0
                 ? L"\xD654\xBA74, \xC5B8\xC5B4, \xC624\xB514\xC624 \xC124\xC815\xACFC \xAE30\xD0C0 \xACF5\xD1B5\xC5D0 \xB300\xD55C \xC635\xC158"
                 : L"Display, control, and audio options in one panel",
                 leftX, panelY + 52.0f * uiS - hSlide,
                 0.50f * uiS, 0.48f, 0.58f, 0.78f, 0.72f * wake);
    float profileW = 330.0f * uiS;
    float profileX = panelX + panelW - profileW + (1.0f - wake) * 50.0f * uiS;
    drawRect(profileX, panelY + 8.0f * uiS, profileW, 44.0f * uiS,
             0.040f, 0.050f, 0.070f, 0.82f * wake);
    drawBorder(profileX, panelY + 8.0f * uiS, profileW, 44.0f * uiS,
               tab.r, tab.g, tab.b,
               (0.24f + 0.08f * sinf(now * 2.2f)) * wake, 1.2f * uiS);
    drawCenterS(profileBuf, profileX, panelY + 20.0f * uiS, profileW,
                0.44f * uiS, 0.76f, 0.86f, 1.0f, 0.88f * wake);

    // === LEFT COLUMN: Tab cards ===
    const float tabCardGap = 16.0f * uiS;
    const float tabCardH   = (bodyH - tabCardGap * (kTabCount - 1)) / (float)kTabCount;

    for (int i = 0; i < kTabCount; ++i) {
        const SettingsTabDef& td = kTabs[i];
        float cardY  = bodyY + (float)i * (tabCardH + tabCardGap);
        float cwi    = cardWake[i];
        float cxOff  = (1.0f - cwi) * 62.0f * uiS;
        float cxBase = leftX - cxOff;
        bool sel = (i == s_tab);
        bool hov = hit(leftX, cardY, leftW, tabCardH);
        s_tabHover[i] = UiApproach(s_tabHover[i], hov ? 1.0f : 0.0f, delta, 10.0f);
        s_tabFocus[i] = UiApproach(s_tabFocus[i], sel ? 1.0f : 0.0f, delta, 8.5f);
        float foc = s_tabFocus[i];
        float hv  = s_tabHover[i];
        if (hov && lmb && !g_LmbPrev && !sel && !modalActive) {
            if (g_VolEdit) commitVol();
            s_tab = i; s_prevTab = i; s_panelT = 0.0f;
            s_resetDialog = false;
        }
        BindMainShader();
        drawRect(cxBase + 7.0f*uiS, cardY + 9.0f*uiS, leftW, tabCardH,
                 0.0f, 0.0f, 0.0f, (0.16f + 0.10f*foc)*cwi);
        drawRect(cxBase, cardY, leftW, tabCardH,
                 0.025f + td.r*(0.018f + 0.055f*foc + 0.015f*hv),
                 0.032f + td.g*(0.015f + 0.044f*foc + 0.012f*hv),
                 0.052f + td.b*(0.015f + 0.036f*foc + 0.012f*hv),
                 (0.82f + 0.14f*foc)*cwi);
        drawRect(cxBase, cardY, leftW, 30.0f*uiS,
                 td.r*(0.22f + 0.36f*foc),
                 td.g*(0.22f + 0.36f*foc),
                 td.b*(0.24f + 0.36f*foc), 0.94f*cwi);
        drawRect(cxBase, cardY + 30.0f*uiS, leftW, 2.0f*uiS,
                 td.r, td.g, td.b, (0.22f + 0.55f*foc)*cwi);
        drawBorder(cxBase, cardY, leftW, tabCardH, td.r, td.g, td.b,
                   (0.16f + 0.56f*foc + 0.28f*hv)*cwi, 1.5f*uiS);
        if (foc > 0.01f) {
            float barH = tabCardH * (0.30f + 0.70f*foc);
            drawRect(cxBase, cardY + (tabCardH - barH)*0.5f,
                     5.0f*uiS, barH, td.r, td.g, td.b, 0.88f*foc*cwi);
        }
        float bsz = 9.0f*uiS, bby = cardY + 11.0f*uiS;
        float bbx = cxBase + leftW - 14.0f*uiS;
        drawRect(bbx - 2.0f*(bsz+6.0f*uiS), bby, bsz, bsz, 1,1,1, (0.10f+0.14f*foc)*cwi);
        drawRect(bbx - (bsz+6.0f*uiS),      bby, bsz, bsz, 1,1,1, (0.10f+0.14f*foc)*cwi);
        drawRect(bbx,                         bby, bsz, bsz, 0.9f,0.25f,0.25f, (0.20f+0.42f*foc)*cwi);
        // Korean sub (small, in colored strip)
        g_TextS.Draw(td.name[nli], cxBase + 10.0f*uiS, cardY + 8.0f*uiS,
                     0.40f*uiS, 1.0f, 1.0f, 1.0f, (0.50f+0.32f*foc)*cwi);
        // English ID (large, primary)
        g_TextL.Draw(td.id, cxBase + 14.0f*uiS, cardY + 36.0f*uiS,
                     0.82f*uiS,
                     0.86f + td.r*0.14f*foc, 0.90f + td.g*0.10f*foc, 1.0f,
                     (0.72f+0.26f*foc)*cwi);
        drawFitS(td.brief[nli], cxBase + 14.0f*uiS, cardY + 72.0f*uiS,
                 leftW - 28.0f*uiS, 0.40f*uiS, 0.30f*uiS,
                 td.r, td.g, td.b, (0.44f+0.32f*foc)*cwi);
        float frameProg = std::min(foc > 0.01f ? 1.0f : cwi, cwi);
        if (frameProg > 0.02f) {
            BatchFlush(); SetGlowFx(true);
            drawConstellFrame(cxBase, cardY, leftW, tabCardH,
                              td.r, td.g, td.b,
                              (foc > 0.05f ? 0.55f*foc : 0.22f)*cwi,
                              14.0f*uiS, 4.0f*uiS,
                              foc > 0.05f ? 0.20f : 0.10f, frameProg);
            BatchFlush(); SetGlowFx(false);
        }
    }

    // === RIGHT PANEL: Content window ===
    float panelE = Smoothstep(s_panelT);
    float entryShift = (1.0f - rightWake) * 70.0f * uiS;
    float panelShift = (1.0f - panelE) * 40.0f * uiS;
    float rpx = rightX + entryShift + panelShift;

    // Tab → panel connector: 선택된 탭 우측 끝 → 우측 패널 좌측 끝 가이드라인
    {
        float connFoc = s_tabFocus[s_tab];
        float connCwi = cardWake[s_tab];
        float connA   = connFoc * rightWake * connCwi;
        if (connA > 0.01f) {
            float selCardY = bodyY + (float)s_tab * (tabCardH + tabCardGap);
            float connY    = selCardY + tabCardH * 0.5f;
            float cx0      = leftX + leftW;
            float cx1      = rpx;
            float connH    = 1.5f * uiS;
            BindMainShader();
            drawRect(cx0, connY - connH*0.5f, cx1 - cx0, connH,
                     tab.r, tab.g, tab.b, 0.40f * connA);
            drawDiamond(cx0, connY, 4.5f*uiS, tab.r, tab.g, tab.b, 0.70f * connA);
            drawDiamond(cx1, connY, 4.5f*uiS, tab.r, tab.g, tab.b, 0.70f * connA);
        }
    }

    BindMainShader();
    drawRect(rpx + 9.0f*uiS, bodyY + 11.0f*uiS, rightW, bodyH,
             0.0f, 0.0f, 0.0f, (0.16f + 0.10f*panelE)*rightWake);
    drawRect(rpx, bodyY, rightW, bodyH,
             0.025f + tab.r*0.012f,
             0.032f + tab.g*0.010f,
             0.052f + tab.b*0.010f, (0.86f + 0.10f*panelE)*rightWake);
    drawRect(rpx, bodyY, rightW, 30.0f*uiS,
             tab.r*(0.20f + 0.38f*panelE),
             tab.g*(0.20f + 0.38f*panelE),
             tab.b*(0.22f + 0.38f*panelE), 0.94f*rightWake);
    drawRect(rpx, bodyY + 30.0f*uiS, rightW, 2.0f*uiS,
             tab.r, tab.g, tab.b, (0.22f + 0.55f*panelE)*rightWake);
    float sweepW = rightW * 0.28f;
    float sweepX = rpx + fmodf(now * 180.0f, rightW + sweepW) - sweepW;
    drawRect(sweepX, bodyY + 30.0f*uiS, sweepW, 2.0f*uiS,
             tab.r, tab.g, tab.b, 0.18f*panelE*rightWake);
    float scanY = bodyY + 44.0f*uiS + fmodf(now * 110.0f, std::max(1.0f, bodyH - 56.0f*uiS));
    drawRect(rpx + 2.0f*uiS, scanY, rightW - 4.0f*uiS, 1.0f*uiS,
             tab.r, tab.g, tab.b, 0.045f*panelE*rightWake);
    drawBorder(rpx, bodyY, rightW, bodyH, tab.r, tab.g, tab.b,
               (0.22f + 0.44f*panelE)*rightWake, 1.5f*uiS);
    if (rightWake > 0.02f) {
        BatchFlush(); SetGlowFx(true);
        drawConstellFrame(rpx, bodyY, rightW, bodyH,
                          tab.r, tab.g, tab.b, 0.22f*panelE*rightWake,
                          22.0f*uiS, 6.0f*uiS, 0.14f, rightWake);
        BatchFlush(); SetGlowFx(false);
    }
    float bsz2 = 10.0f*uiS, bb2y = bodyY + 10.0f*uiS;
    float bb2x = rpx + rightW - 18.0f*uiS;
    BindMainShader();
    drawRect(bb2x - 2.0f*(bsz2+7.0f*uiS), bb2y, bsz2, bsz2, 1,1,1, (0.10f+0.14f*panelE)*rightWake);
    drawRect(bb2x - (bsz2+7.0f*uiS),      bb2y, bsz2, bsz2, 1,1,1, (0.10f+0.14f*panelE)*rightWake);
    drawRect(bb2x,                          bb2y, bsz2, bsz2, 0.9f,0.25f,0.25f, (0.20f+0.40f*panelE)*rightWake);
    // Korean sub (small, in colored strip)
    g_TextS.Draw(tab.name[nli], rpx + 14.0f*uiS, bodyY + 9.0f*uiS,
                 0.44f*uiS, 1.0f, 1.0f, 1.0f, 0.68f*rightWake);
    // English ID (large, primary)
    g_TextL.Draw(tab.id, rpx + 20.0f*uiS, bodyY + 40.0f*uiS,
                 1.05f*uiS, 1.0f, 1.0f, 1.0f, 0.96f*panelE*rightWake);
    drawFitS(tab.brief[nli], rpx + 24.0f*uiS, bodyY + 83.0f*uiS,
             rightW - 48.0f*uiS, 0.46f*uiS, 0.34f*uiS,
             tab.r, tab.g, tab.b, 0.72f*panelE*rightWake);

    // === ROW LAYOUT ===
    const float rowX    = rpx + 22.0f * uiS;
    const float rowW    = rightW - 44.0f * uiS;
    const float rowH    = 72.0f * uiS;
    const float rowGap  = 12.0f * uiS;
    const float rowStart = bodyY + 120.0f * uiS;
    const float ctlW    = 420.0f * uiS;
    const float ctlH    = 42.0f * uiS;

    auto rowShell = [&](float y, const wchar_t* label, const wchar_t* desc,
                        bool disabled, float h) {
        bool hov = !disabled && !modalActive && hit(rowX, y, rowW, h);
        float rowIdx = (y - rowStart) / (rowH + rowGap);
        if (rowIdx < 0.0f) rowIdx = 0.0f;
        float revealRaw = panelE * 1.22f - rowIdx * 0.075f;
        if (revealRaw < 0.0f) revealRaw = 0.0f;
        if (revealRaw > 1.0f) revealRaw = 1.0f;
        float rowA = Smoothstep(revealRaw) * rightWake;
        float dim = disabled ? 0.48f : 1.0f;
        BindMainShader();
        drawRect(rowX, y, rowW, h,
                 (0.044f + tab.r*(hov ? 0.018f : 0.008f)) * dim,
                 (0.052f + tab.g*(hov ? 0.016f : 0.007f)) * dim,
                 0.074f + tab.b*(hov ? 0.015f : 0.006f), (disabled ? 0.56f : 0.88f)*rowA);
        drawRect(rowX, y, 5.0f*uiS, h, tab.r, tab.g, tab.b,
                 (disabled ? 0.22f : 0.48f + 0.18f*(hov ? 1.0f : 0.0f))*rowA);
        drawBorder(rowX, y, rowW, h, tab.r, tab.g, tab.b,
                   (disabled ? 0.07f : 0.10f + 0.14f*(hov ? 1.0f : 0.0f))*rowA, 1.0f*uiS);
        if (hov && !disabled) {
            float sweep = rowW * (0.16f + 0.08f * sinf(now * 4.0f + rowIdx));
            drawRect(rowX + 7.0f*uiS, y + h - 3.0f*uiS,
                     sweep, 2.0f*uiS, tab.r, tab.g, tab.b, 0.22f*rowA);
        }
        float labelMax = rowW - ctlW - 54.0f*uiS;
        drawFitS(label, rowX + 20.0f*uiS, y + 13.0f*uiS, labelMax,
                 0.56f*uiS, 0.40f*uiS,
                 disabled ? 0.45f : 0.92f, disabled ? 0.48f : 0.96f, disabled ? 0.56f : 1.0f,
                 (disabled ? 0.55f : 0.94f)*rowA);
        drawFitS(desc, rowX + 20.0f*uiS, y + 41.0f*uiS, labelMax,
                 0.40f*uiS, 0.31f*uiS,
                 0.50f, 0.60f, 0.74f, (disabled ? 0.38f : 0.66f)*rowA);
    };
    auto rowShellStd = [&](float y, const wchar_t* label, const wchar_t* desc,
                           bool disabled = false) {
        rowShell(y, label, desc, disabled, rowH);
    };
    auto boolRow = [&](float y, const wchar_t* label, const wchar_t* desc, bool& value) {
        rowShellStd(y, label, desc);
        float bx   = rowX + rowW - ctlW - 16.0f*uiS;
        float by   = y + (rowH - ctlH)*0.5f;
        float bw   = (ctlW - 10.0f*uiS) * 0.5f;
        float offX = bx + bw + 10.0f*uiS;
        float ctlA = rightWake * (0.58f + 0.42f * Smoothstep(s_panelT));
        bool hovOn  = !modalActive && hit(bx,   by, bw, ctlH);
        bool hovOff = !modalActive && hit(offX, by, bw, ctlH);
        if (hovOn  && lmb && !g_LmbPrev) value = true;
        if (hovOff && lmb && !g_LmbPrev) value = false;

        // Corner-bracket highlight helper (inline lambda ok in C++17)
        auto drawBkt = [&](float ox, bool active, bool hov) {
            float ba = active ? 0.86f * ctlA : (hov ? 0.30f * ctlA : 0.0f);
            if (ba < 0.004f) return;
            float cL = std::min(bw * 0.28f, 9.0f * uiS), ct = 1.3f * uiS;
            drawRect(ox,      by,        cL, ct, tab.r, tab.g, tab.b, ba);
            drawRect(ox,      by,        ct, cL, tab.r, tab.g, tab.b, ba);
            drawRect(ox+bw-cL,by,        cL, ct, tab.r, tab.g, tab.b, ba);
            drawRect(ox+bw-ct,by,        ct, cL, tab.r, tab.g, tab.b, ba);
            drawRect(ox,      by+ctlH-ct,cL, ct, tab.r, tab.g, tab.b, ba);
            drawRect(ox,      by+ctlH-cL,ct, cL, tab.r, tab.g, tab.b, ba);
            drawRect(ox+bw-cL,by+ctlH-ct,cL, ct, tab.r, tab.g, tab.b, ba);
            drawRect(ox+bw-ct,by+ctlH-cL,ct, cL, tab.r, tab.g, tab.b, ba);
            float ns = (active ? 3.5f : 2.5f) * uiS;
            drawDiamond(ox,    by,      ns, tab.r, tab.g, tab.b, ba);
            drawDiamond(ox+bw, by,      ns, tab.r, tab.g, tab.b, ba);
            drawDiamond(ox,    by+ctlH, ns, tab.r, tab.g, tab.b, ba);
            drawDiamond(ox+bw, by+ctlH, ns, tab.r, tab.g, tab.b, ba);
        };

        BindMainShader();
        if (value)
            drawRect(bx,   by, bw, ctlH, tab.r*0.08f, tab.g*0.07f, tab.b*0.07f, 0.90f * ctlA);
        else if (!value)
            drawRect(offX, by, bw, ctlH, tab.r*0.08f, tab.g*0.07f, tab.b*0.07f, 0.90f * ctlA);
        drawBkt(bx,   value,  hovOn);
        drawBkt(offX, !value, hovOff);
        drawCenterS(L"ON", bx, by + 12.0f*uiS, bw, 0.48f*uiS,
                    value  ? 1.0f : 0.44f, value  ? 1.0f : 0.48f, value  ? 1.0f : 0.58f,
                    (value  ? 0.96f : 0.50f) * ctlA);
        drawCenterS(L"OFF", offX, by + 12.0f*uiS, bw, 0.48f*uiS,
                    !value ? 1.0f : 0.44f, !value ? 1.0f : 0.48f, !value ? 1.0f : 0.58f,
                    (!value ? 0.96f : 0.50f) * ctlA);
    };
    auto choiceRow = [&](float y, const wchar_t* label, const wchar_t* desc,
                         const wchar_t* const* labels, const int* values, int count,
                         int current, auto setValue) {
        rowShellStd(y, label, desc);
        float bx = rowX + rowW - ctlW - 16.0f*uiS;
        float by = y + (rowH - ctlH)*0.5f;
        float sg = 8.0f*uiS;
        float bw = (ctlW - sg*(count-1)) / (float)count;
        for (int i = 0; i < count; ++i) {
            if (segment(bx + i*(bw+sg), by, bw, ctlH, labels[i],
                        current == values[i], tab.r, tab.g, tab.b))
                setValue(values[i]);
        }
    };
    auto infoRow = [&](float y, const wchar_t* label, const wchar_t* desc,
                       const wchar_t* value, bool disabled = false) {
        rowShellStd(y, label, desc, disabled);
        float infoA = rightWake * (0.58f + 0.42f*panelE);
        float bx = rowX + rowW - ctlW - 16.0f*uiS;
        float by = y + (rowH - ctlH)*0.5f;
        BindMainShader();
        drawRect(bx, by, ctlW, ctlH, 0.050f, 0.058f, 0.075f, (disabled ? 0.50f : 0.82f)*infoA);
        {   // Corner brackets
            float ba = (disabled ? 0.10f : 0.26f) * infoA;
            float cL = std::min(ctlW * 0.22f, 10.0f * uiS), ct = 1.3f * uiS;
            drawRect(bx,        by,        cL, ct, tab.r,tab.g,tab.b, ba);
            drawRect(bx,        by,        ct, cL, tab.r,tab.g,tab.b, ba);
            drawRect(bx+ctlW-cL,by,        cL, ct, tab.r,tab.g,tab.b, ba);
            drawRect(bx+ctlW-ct,by,        ct, cL, tab.r,tab.g,tab.b, ba);
            drawRect(bx,        by+ctlH-ct,cL, ct, tab.r,tab.g,tab.b, ba);
            drawRect(bx,        by+ctlH-cL,ct, cL, tab.r,tab.g,tab.b, ba);
            drawRect(bx+ctlW-cL,by+ctlH-ct,cL, ct, tab.r,tab.g,tab.b, ba);
            drawRect(bx+ctlW-ct,by+ctlH-cL,ct, cL, tab.r,tab.g,tab.b, ba);
            float ns = 3.0f * uiS;
            drawDiamond(bx,       by,       ns, tab.r,tab.g,tab.b, ba);
            drawDiamond(bx+ctlW,  by,       ns, tab.r,tab.g,tab.b, ba);
            drawDiamond(bx,       by+ctlH,  ns, tab.r,tab.g,tab.b, ba);
            drawDiamond(bx+ctlW,  by+ctlH,  ns, tab.r,tab.g,tab.b, ba);
        }
        drawCenterS(value, bx, by + 12.0f*uiS, ctlW, 0.45f*uiS,
                    disabled ? 0.48f : 0.82f, disabled ? 0.50f : 0.90f, disabled ? 0.56f : 1.0f,
                    (disabled ? 0.52f : 0.90f)*infoA);
    };
    auto volumeRow = [&](float y, const wchar_t* label, const wchar_t* desc, int& vol) {
        rowShellStd(y, label, desc);
        float volA = rightWake * (0.58f + 0.42f*panelE);
        float bx = rowX + rowW - ctlW - 16.0f*uiS;
        float by = y + (rowH - ctlH)*0.5f;
        float smallW = 42.0f*uiS;
        if (segment(bx, by, smallW, ctlH, L"-", false, tab.r, tab.g, tab.b)) {
            g_VolEdit = false; vol = clampVol(vol - 5);
        }
        float barX  = bx + smallW + 12.0f*uiS;
        float fieldW = 76.0f*uiS;
        float plusX  = bx + ctlW - fieldW - smallW - 16.0f*uiS;
        float barW   = plusX - barX - 12.0f*uiS;
        float trackY = by + ctlH*0.5f;
        float fillW  = barW * (vol / 100.0f);
        BindMainShader();
        drawRect(barX, trackY-4.0f*uiS, barW, 8.0f*uiS, 0.11f,0.12f,0.16f, 0.96f*volA);
        drawRect(barX, trackY-4.0f*uiS, fillW, 8.0f*uiS, tab.r,tab.g,tab.b, 0.90f*volA);
        float knobX = barX + fillW;
        drawCircle(knobX, trackY, 10.0f*uiS, tab.r,tab.g,tab.b, 0.92f*volA);
        drawCircle(knobX, trackY, 5.2f*uiS, 0.96f,0.98f,1.0f, 0.96f*volA);
        bool barHover = !modalActive && hit(barX, by, barW, ctlH);
        if (lmb && barHover) {
            g_VolEdit = false;
            vol = clampVol((int)(((float)mx - barX) / barW * 100.0f + 0.5f));
        }
        if (segment(plusX, by, smallW, ctlH, L"+", false, tab.r, tab.g, tab.b)) {
            g_VolEdit = false; vol = clampVol(vol + 5);
        }
        float fieldX = bx + ctlW - fieldW;
        bool fieldHover = hit(fieldX, by, fieldW, ctlH);
        BindMainShader();
        drawRect(fieldX, by, fieldW, ctlH,
                 g_VolEdit?0.15f:0.060f, g_VolEdit?0.17f:0.068f,
                 g_VolEdit?0.22f:0.090f, 0.92f*volA);
        drawBorder(fieldX, by, fieldW, ctlH, tab.r, tab.g, tab.b,
                   (g_VolEdit?0.72f:(fieldHover?0.38f:0.18f))*volA, 1.2f*uiS);
        if (lmb && !g_LmbPrev && !modalActive) {
            if (fieldHover) { g_VolEdit = true; g_VolLen = 0; g_VolBuf[0] = 0; }
            else if (g_VolEdit) commitVol();
        }
        wchar_t shown[16];
        if (g_VolEdit) {
            bool caret = (((int)(glfwGetTime()*2.0)) & 1) == 0;
            swprintf_s(shown, L"%ls%ls", g_VolLen ? g_VolBuf : L"", caret ? L"|" : L"");
        } else {
            swprintf_s(shown, L"%d", vol);
        }
        drawCenterS(shown, fieldX, by + 12.0f*uiS, fieldW, 0.46f*uiS, 1.0f,1.0f,1.0f, 0.94f*volA);
    };
    auto disabledMeterRow = [&](float y, const wchar_t* label, const wchar_t* desc) {
        rowShellStd(y, label, desc, true);
        float meterA = rightWake * (0.58f + 0.42f*panelE);
        float bx = rowX + rowW - ctlW - 16.0f*uiS;
        float by = y + (rowH - ctlH)*0.5f;
        BindMainShader();
        drawRect(bx, by, ctlW, ctlH, 0.045f,0.048f,0.060f, 0.50f*meterA);
        {   // Corner brackets (disabled style)
            float ba = 0.12f * meterA;
            float cL = std::min(ctlW * 0.22f, 10.0f * uiS), ct = 1.3f * uiS;
            drawRect(bx,        by,        cL, ct, tab.r,tab.g,tab.b, ba);
            drawRect(bx,        by,        ct, cL, tab.r,tab.g,tab.b, ba);
            drawRect(bx+ctlW-cL,by,        cL, ct, tab.r,tab.g,tab.b, ba);
            drawRect(bx+ctlW-ct,by,        ct, cL, tab.r,tab.g,tab.b, ba);
            drawRect(bx,        by+ctlH-ct,cL, ct, tab.r,tab.g,tab.b, ba);
            drawRect(bx,        by+ctlH-cL,ct, cL, tab.r,tab.g,tab.b, ba);
            drawRect(bx+ctlW-cL,by+ctlH-ct,cL, ct, tab.r,tab.g,tab.b, ba);
            drawRect(bx+ctlW-ct,by+ctlH-cL,ct, cL, tab.r,tab.g,tab.b, ba);
        }
        drawCenterS((li==0) ? L"MASTER\xC5D0 \xC5F0\xB3D9"
                            : (li==1) ? L"LINKED TO MASTER"
                                      : L"MASTERに連動",
                    bx, by+12.0f*uiS, ctlW, 0.42f*uiS, 0.48f,0.52f,0.62f, 0.62f*meterA);
    };

    if (g_VolEdit) {
        bool bs = (glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS);
        if (bs && !s_bsPrev && g_VolLen > 0) g_VolBuf[--g_VolLen] = 0;
        s_bsPrev = bs;
        bool en = (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS);
        if (en && !s_enPrev) commitVol();
        s_enPrev = en;
    } else {
        s_bsPrev = false;
        s_enPrev = false;
    }

    float y = rowStart;
    if (s_tab == 0) {
        const wchar_t* fpsLabels[5] = { L"VSYNC", L"30", L"60", L"144", L"300" };
        const int fpsVals[5] = { 0, 30, 60, 144, 300 };
        choiceRow(y, T(StrId::SET_FPS),
                  nli == 0 ? L"\xD504\xB808\xC784 \xC81C\xD55C\xACFC \xC218\xC9C1\xB3D9\xAE30\xD654"
                           : L"Frame cap and vertical sync",
                  fpsLabels, fpsVals, 5, g_FpsCap, [&](int v) {
                      g_FpsCap = v;
                      glfwSwapInterval((g_FpsCap == 0) ? 1 : 0);
                  });
        y += rowH + rowGap;
        const wchar_t* langLabels[2] = { L"\xD55C\xAD6D\xC5B4", L"English" };
        const int langVals[2] = { 0, 1 };
        choiceRow(y, T(StrId::SET_LANG),
                  nli == 0 ? L"UI \xD45C\xC2DC \xC5B8\xC5B4" : L"Interface language",
                  langLabels, langVals, 2, std::min((int)g_Language, 1), [&](int v) {
                      if (v >= 0 && v < LANG_COUNT) g_Language = (Language)v;
                  });
        y += rowH + rowGap;
        boolRow(y,
                nli == 0 ? L"CRT \xC170\xC774\xB354" : L"CRT Shader",
                nli == 0 ? L"\xC2A4\xCE94\xB77C\xC778\xACFC \xD654\xBA74 \xD3EC\xC2A4\xD2B8 \xC774\xD399\xD2B8"
                         : L"Scanline and screen post effect",
                g_ShaderFx);
        y += rowH + rowGap;
        const int twoVals[2] = { 0, 1 };
        const wchar_t* vfxLabels[2] = { nli == 0 ? L"\xC804\xCCB4" : L"Full",
                                        nli == 0 ? L"\xAC10\xC18C" : L"Reduced" };
        choiceRow(y,
                  nli == 0 ? L"VFX \xBC00\xB3C4" : L"VFX Density",
                  nli == 0 ? L"\xBC18\xBCF5 \xC774\xD399\xD2B8 \xBC00\xB3C4" : L"Repeated effect density",
                  vfxLabels, twoVals, 2, (int)g_VfxDensity, [&](int v) {
                      g_VfxDensity = (v == 0) ? VfxDensity::FULL : VfxDensity::REDUCED;
                  });
        y += rowH + rowGap;
        boolRow(y,
                nli == 0 ? L"\xC790\xB3D9 \xBC1C\xC0AC" : L"Auto-Fire",
                nli == 0 ? L"\xC870\xC900 \xC2DC \xC790\xB3D9\xC73C\xB85C \xAE30\xBCF8 \xACF5\xACA9"
                         : L"Primary fire while aiming",
                g_AutoFire);
        y += rowH + rowGap;
        boolRow(y, T(StrId::SET_CROSSHAIR),
                nli == 0 ? L"\xCEE4\xC11C \xC870\xC900 \xC778\xB514\xCF00\xC774\xD130" : L"Cursor aim indicator",
                g_ShowCrosshair);
        y += rowH + rowGap;
        boolRow(y, T(StrId::SET_DMGNUM),
                nli == 0 ? L"\xBC1B\xC740 \xD53C\xD574\xB7C9 \xD45C\xC2DC" : L"Show hit damage values",
                g_ShowDamageNumbers);
    } else if (s_tab == 1) {
        volumeRow(y,
                  nli == 0 ? L"\xB9C8\xC2A4\xD130 \xBCFC\xB968" : L"Master Volume",
                  nli == 0 ? L"\xC804\xCCB4 \xC18C\xB9AC \xCD9C\xB825" : L"Overall sound output",
                  g_SoundVol);
        y += rowH + rowGap;
        disabledMeterRow(y,
                         nli == 0 ? L"BGM \xBCFC\xB968" : L"BGM Volume",
                         nli == 0 ? L"\xC608\xC57D \xC911: \xCC44\xB110 \xBD84\xB9AC \xC608\xC815" : L"Reserved for channel split");
        y += rowH + rowGap;
        disabledMeterRow(y,
                         nli == 0 ? L"SFX \xBCFC\xB968" : L"SFX Volume",
                         nli == 0 ? L"\xC608\xC57D \xC774\xD399\xD2B8 \xCC44\xB110" : L"Reserved effect channel");
        y += rowH + rowGap;
        disabledMeterRow(y,
                         nli == 0 ? L"UI \xBCFC\xB968" : L"UI Volume",
                         nli == 0 ? L"\xC608\xC57D UI \xC0AC\xC6B4\xB4DC" : L"Reserved for UI sounds");
    } else {
        wchar_t bestBuf[128];
        swprintf_s(bestBuf, L"E %lld / N %lld / H %lld", g_BestScore[0], g_BestScore[1], g_BestScore[2]);
        wchar_t recordBuf[96];
        swprintf_s(recordBuf, L"KILLS %lld / RUNS %lld", g_TotalKills, g_TotalGames);
        wchar_t coinBuf[64];
        swprintf_s(coinBuf, L"%lld G", g_Coins);
        infoRow(y,
                nli == 0 ? L"\xC800\xC7A5 \xBC29\xC2DD" : L"Save Mode",
                nli == 0 ? L"\xC885\xB8CC\xC2DC\xB098 \xC911\xC694 \xC2E4\xD589 \xB54C \xC790\xB3D9 \xC800\xC7A5"
                         : L"Saved on exit and progress events",
                nli == 0 ? L"\xC790\xB3D9 \xC800\xC7A5" : L"AUTO SAVE");
        y += rowH + rowGap;
        infoRow(y,
                nli == 0 ? L"\xCD5C\xACE0 \xAE30\xB85D" : L"Best Score",
                nli == 0 ? L"\xB09C\xC774\xB3C4\xBCC4 \xCD5C\xACE0 \xC810\xC218" : L"Best score by difficulty",
                bestBuf);
        y += rowH + rowGap;
        infoRow(y,
                nli == 0 ? L"\xB204\xC801 \xAE30\xB85D" : L"Run Record",
                nli == 0 ? L"\xCD1D\xD569 \xCC98\xCE58 \xD69F\xC218\xC640 \xD310\xC218" : L"Total kills and run count",
                recordBuf);
        y += rowH + rowGap;
        infoRow(y,
                nli == 0 ? L"\xBCF4\xC720 \xCF54\xC778" : L"Coin",
                nli == 0 ? L"\xC0C1\xC810\xACFC \xD574\xAE08\xC5D0 \xC0AC\xC6A9" : L"Used for shop and unlocks",
                coinBuf);
        y += rowH + rowGap;
        rowShellStd(y,
                    nli == 0 ? L"\xAE30\xB85D \xCD08\xAE30\xD654" : L"Reset Records",
                    nli == 0 ? L"\xD56D\xBAA9 \xBC94\xC704\xB97C \xC120\xD0DD \xD6C4 \xCD08\xAE30\xD654 \xC2E4\xD589"
                             : L"Select scope and confirm reset");
        {
            float bx = rowX + rowW - ctlW - 16.0f * uiS;
            float by2 = y + (rowH - ctlH) * 0.5f;
            bool rHov = hit(bx, by2, ctlW, ctlH);
            BindMainShader();
            drawRect(bx, by2, ctlW, ctlH,
                     0.28f + (rHov ? 0.08f : 0.0f), 0.04f, 0.04f, 0.92f);
            BatchFlush(); SetGlowFx(true);
            drawConstellFrame(bx, by2, ctlW, ctlH, 1.0f, 0.35f, 0.35f,
                              rHov ? 0.90f : 0.52f, 10.0f*uiS, 3.0f*uiS);
            BatchFlush(); SetGlowFx(false);
            drawCenterS(nli == 0 ? L"\xCD08\xAE30\xD654 \xC120\xD0DD..." : L"Select Range...",
                        bx, by2 + 12.0f*uiS, ctlW, 0.45f*uiS, 1.0f, 0.55f, 0.55f, 0.96f);
            if (rHov && lmb && !g_LmbPrev) {
                s_resetDialog           = true;
                s_resetDialogJustOpened = true;
                s_resetDone   = false;
                s_resetItems[0] = s_resetItems[1] = s_resetItems[2] = s_resetItems[3] = false;
            }
        }
        y += rowH + rowGap;
        rowShellStd(y,
                    nli == 0 ? L"\xD06C\xB808\xB515" : L"Credits",
                    nli == 0 ? L"\xC0AC\xC6A9 \xB77C\xC774\xBE0C\xB7EC\xB9AC\xC640 \xBE4C\xB4DC \xC815\xBCF4"
                             : L"Libraries and build info");
        {
            float bx = rowX + rowW - ctlW - 16.0f * uiS;
            float by2 = y + (rowH - ctlH) * 0.5f;
            if (segment(bx, by2, ctlW, ctlH,
                        nli == 0 ? L"\xBCF4\xAE30" : L"View", false, tab.r, tab.g, tab.b))
                s_showCredits = true;
        }
    }

    const float backW = 178.0f * uiS;
    const float footX = panelX + (panelW - backW) * 0.5f;
    const bool lmbMain = lmb && !modalActive;
    if (UIButton(footX, footY, backW, 48.0f * uiS, T(StrId::BTN_BACK),
                 mx, my, lmbMain, g_LmbPrev)) {
        if (g_VolEdit) commitVol();
        SaveGame();
        g_GameManager.currentState = g_SettingsReturnTo;
    }

    if (modalActive) {
        BindMainShader();
        drawRect(panelX, panelY, panelW, panelH, 0.0f, 0.0f, 0.0f, 0.52f);
    }

    if (s_resetDialog) {
        const float dW = 560.0f * uiS, dH = 380.0f * uiS;
        const float dX = (sw - dW) * 0.5f, dY = (sh - dH) * 0.5f;
        BindMainShader();
        drawRect(dX + 8.0f*uiS, dY + 10.0f*uiS, dW, dH, 0.0f, 0.0f, 0.0f, 0.32f);
        drawRect(dX, dY, dW, dH, 0.032f, 0.040f, 0.060f, 0.97f);
        BatchFlush(); SetGlowFx(true);
        drawConstellFrame(dX, dY, dW, dH, 1.0f, 0.35f, 0.35f, 0.72f, 16.0f*uiS, 5.0f*uiS, 0.22f);
        BatchFlush(); SetGlowFx(false);

        const wchar_t* dlgTitle = nli == 0 ? L"\xCD08\xAE30\xD654 \xBC94\xC704 \xC120\xD0DD" : L"SELECT RESET SCOPE";
        float tw = g_TextL.Width(dlgTitle, 0.80f*uiS);
        BindMainShader();
        g_TextL.Draw(dlgTitle, dX + (dW - tw) * 0.5f, dY + 18.0f*uiS, 0.80f*uiS, 1.0f, 0.55f, 0.55f, 0.96f);
        drawRect(dX + 20.0f*uiS, dY + 54.0f*uiS, dW - 40.0f*uiS, 1.0f*uiS, 1.0f, 0.4f, 0.4f, 0.30f);

        const wchar_t* itemLabels[2][4] = {
            { L"\xCD5C\xACE0 \xAE30\xB85D (\xB09C\xC774\xB3C4\xBCC4)", L"\xB204\xC801 \xAE30\xB85D (\xCD1D\xD569/\xD310)", L"\xBCF4\xC720 \xCF54\xC778", L"\xD574\xAE08 \xD56D\xBAA9" },
            { L"Best Score",        L"Run Records",        L"Coins",         L"Unlocks"   },
        };
        for (int i = 0; i < 4; ++i) {
            float iy = dY + 72.0f*uiS + (float)i * 56.0f*uiS;
            float cbX = dX + 28.0f*uiS, cbY = iy + 8.0f*uiS;
            float cbS = 26.0f*uiS;
            bool cbHov = hit(cbX, cbY, cbS, cbS);
            BindMainShader();
            drawRect(cbX, cbY, cbS, cbS,
                     s_resetItems[i] ? 0.60f : 0.06f,
                     s_resetItems[i] ? 0.10f : 0.06f,
                     s_resetItems[i] ? 0.10f : 0.06f, 0.94f);
            BatchFlush(); SetGlowFx(true);
            drawConstellFrame(cbX, cbY, cbS, cbS, 1.0f, 0.40f, 0.40f,
                              s_resetItems[i] ? 0.90f : (cbHov ? 0.55f : 0.28f),
                              7.0f*uiS, 2.5f*uiS);
            BatchFlush(); SetGlowFx(false);
            if (s_resetItems[i]) {
                drawDiamond(cbX + cbS*0.5f, cbY + cbS*0.5f, cbS*0.45f, 1.0f, 0.55f, 0.55f, 0.92f);
            }
            if (cbHov && lmb && !g_LmbPrev) s_resetItems[i] = !s_resetItems[i];
            g_TextS.Draw(itemLabels[nli][i], cbX + cbS + 14.0f*uiS, iy + 12.0f*uiS,
                         0.52f*uiS, 0.92f, 0.92f, 1.0f, 0.90f);
        }

        const float btnY = dY + dH - 62.0f*uiS;
        const float btnH = 42.0f*uiS, btnW = 180.0f*uiS;
        const float cancelX = dX + dW * 0.5f - btnW - 12.0f*uiS;
        const float execX   = dX + dW * 0.5f + 12.0f*uiS;
        bool anyItem = s_resetItems[0] || s_resetItems[1] || s_resetItems[2] || s_resetItems[3];

        if (UIButton(cancelX, btnY, btnW, btnH, nli == 0 ? L"\xCDE8\xC18C" : L"Cancel",
                     mx, my, lmb, g_LmbPrev)) {
            s_resetDialog = false;
        }

        bool execHov = anyItem && hit(execX, btnY, btnW, btnH);
        BindMainShader();
        drawRect(execX, btnY, btnW, btnH,
                 anyItem ? (0.30f + (execHov ? 0.08f : 0.0f)) : 0.08f,
                 0.03f, 0.03f, anyItem ? 0.94f : 0.50f);
        BatchFlush(); SetGlowFx(true);
        drawConstellFrame(execX, btnY, btnW, btnH, 1.0f, 0.35f, 0.35f,
                          anyItem ? (execHov ? 0.90f : 0.55f) : 0.18f,
                          10.0f*uiS, 3.0f*uiS);
        BatchFlush(); SetGlowFx(false);
        drawCenterS(nli == 0 ? L"\xCD08\xAE30\xD654 \xC2E4\xD589" : L"Confirm Reset",
                    execX, btnY + 12.0f*uiS, btnW, 0.46f*uiS,
                    1.0f, anyItem ? 0.50f : 0.35f, anyItem ? 0.50f : 0.35f,
                    anyItem ? 0.96f : 0.42f);

        if (execHov && anyItem && lmb && !g_LmbPrev) {
            if (s_resetItems[0]) { g_BestScore[0] = g_BestScore[1] = g_BestScore[2] = 0; }
            if (s_resetItems[1]) { g_TotalKills = 0; g_TotalGames = 0; }
            if (s_resetItems[2]) { g_Coins = 0; }
            if (s_resetItems[3]) { ResetSaveProgress(); }
            SaveGame();
            s_resetDialog = false;
            s_resetDone   = true;
        }

        bool justOpened = s_resetDialogJustOpened;
        s_resetDialogJustOpened = false;
        if (!justOpened && lmb && !g_LmbPrev && !hit(dX, dY, dW, dH))
            s_resetDialog = false;
    }

    if (s_resetDone) {
        const float nW = 460.0f*uiS, nH = 210.0f*uiS;
        const float nX = (sw - nW) * 0.5f, nY = (sh - nH) * 0.5f;
        BindMainShader();
        drawRect(nX + 7.0f*uiS, nY + 9.0f*uiS, nW, nH, 0.0f, 0.0f, 0.0f, 0.32f);
        drawRect(nX, nY, nW, nH, 0.032f, 0.040f, 0.060f, 0.97f);
        BatchFlush(); SetGlowFx(true);
        drawConstellFrame(nX, nY, nW, nH, 1.0f, 0.35f, 0.35f, 0.72f, 16.0f*uiS, 5.0f*uiS, 0.22f);
        BatchFlush(); SetGlowFx(false);
        const wchar_t* doneTitle = nli == 0 ? L"\xCD08\xAE30\xD654 \xC644\xB8CC" : L"Reset Complete";
        float dtw = g_TextL.Width(doneTitle, 0.80f*uiS);
        BindMainShader();
        g_TextL.Draw(doneTitle, nX + (nW - dtw)*0.5f, nY + 20.0f*uiS,
                     0.80f*uiS, 1.0f, 0.55f, 0.55f, 0.96f);
        drawRect(nX + 20.0f*uiS, nY + 58.0f*uiS, nW - 40.0f*uiS, 1.0f*uiS,
                 1.0f, 0.4f, 0.4f, 0.28f);
        const wchar_t* line1 = nli==0
            ? L"\xC120\xD0DD\xD55C \xAE30\xB85D\xC774 \xCD08\xAE30\xD654\xB418\xC5C8\xC2B5\xB2C8\xB2E4."
            : L"Selected records have been reset.";
        const wchar_t* line2 = nli==0
            ? L"\xBCC0\xACBD \xC0AC\xD56D\xC744 \xC801\xC6A9\xD558\xB824\xBA74 \xAC8C\xC784\xC744 \xC7AC\xC2DC\xC791\xD558\xC138\xC694."
            : L"Restart the game to apply changes.";
        float sc1 = 0.48f*uiS;
        while (sc1 > 0.34f*uiS && g_TextS.Width(line1, sc1) > nW - 60.0f*uiS) sc1 -= 0.02f*uiS;
        float sc2 = 0.44f*uiS;
        while (sc2 > 0.30f*uiS && g_TextS.Width(line2, sc2) > nW - 60.0f*uiS) sc2 -= 0.02f*uiS;
        g_TextS.Draw(line1, nX + (nW - g_TextS.Width(line1, sc1))*0.5f,
                     nY + 72.0f*uiS, sc1, 0.88f, 0.92f, 1.0f, 0.90f);
        g_TextS.Draw(line2, nX + (nW - g_TextS.Width(line2, sc2))*0.5f,
                     nY + 102.0f*uiS, sc2, 0.56f, 0.64f, 0.80f, 0.76f);
        const float qW = 160.0f*uiS, qH = 44.0f*uiS;
        const float qX = nX + (nW - qW)*0.5f, qY = nY + nH - 60.0f*uiS;
        bool qHov = hit(qX, qY, qW, qH);
        BindMainShader();
        drawRect(qX, qY, qW, qH, 0.28f+(qHov?0.08f:0.0f), 0.04f, 0.04f, 0.92f);
        BatchFlush(); SetGlowFx(true);
        drawConstellFrame(qX, qY, qW, qH, 1.0f, 0.35f, 0.35f,
                          qHov ? 0.90f : 0.55f, 10.0f*uiS, 3.0f*uiS);
        BatchFlush(); SetGlowFx(false);
        drawCenterS(nli==0 ? L"\xC885\xB8CC" : L"Quit", qX, qY + 12.0f*uiS, qW,
                    0.48f*uiS, 1.0f, 0.60f, 0.60f, 0.96f);
        if (qHov && lmb && !g_LmbPrev)
            glfwSetWindowShouldClose(window, 1);
    }

    if (s_showCredits) {
        BindMainShader();
        float cw = 660.0f * uiS;
        float ch = 456.0f * uiS;
        float cx = (sw - cw) * 0.5f;
        float cy = (sh - ch) * 0.5f;
        drawRect(cx + 8.0f * uiS, cy + 10.0f * uiS, cw, ch, 0.0f, 0.0f, 0.0f, 0.28f);
        drawRect(cx, cy, cw, ch, 0.050f, 0.060f, 0.085f, 0.98f);
        drawRect(cx, cy, cw, 5.0f * uiS, 0.35f, 0.82f, 1.0f, 0.95f);
        drawBorder(cx, cy, cw, ch, 0.35f, 0.82f, 1.0f, 0.46f, 1.5f * uiS);
        const wchar_t* ct = L"CREDITS";
        g_TextL.Draw(ct, cx + (cw - g_TextL.Width(ct, 1.04f * uiS)) * 0.5f,
                     cy + 28.0f * uiS, 1.04f * uiS, 1.0f, 1.0f, 1.0f, 0.98f);
        const wchar_t* lines[] = {
            L"ONEDOW - Desktop Defense",
            L"Fonts: Jua / Kosugi Maru / Chakra Petch / Orbit",
            L"Icons: game-icons.net",
            L"Audio engine: miniaudio",
            L"Built with OpenGL, GLFW, GLAD, glm, stb",
            L"Made with Claude Code",
        };
        float ly = cy + 88.0f * uiS;
        for (auto* ln : lines) {
            drawFitS(ln, cx + 42.0f * uiS, ly, cw - 84.0f * uiS,
                     0.54f * uiS, 0.40f * uiS, 0.82f, 0.90f, 1.0f, 0.90f);
            ly += 38.0f * uiS;
        }
        if (UIButton(cx + (cw - 180.0f * uiS) * 0.5f, cy + ch - 62.0f * uiS,
                     180.0f * uiS, 44.0f * uiS, T(StrId::BTN_BACK),
                     mx, my, lmb, g_LmbPrev))
            s_showCredits = false;
    }
}

static void Scene_Settings_Legacy_UNUSED(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    GLFWwindow* window = c.window;
    const GameState st = g_GameManager.currentState;
    float& fireTimer = *c.fireTimer;
    const std::function<void()>& ResetForNewGame = c.reset;
                const float WW = std::min(sw * 0.88f, 1220.0f);
                const float WH = std::min(sh * 0.86f, 780.0f);
                float wx, wy;
                const bool settingsOverlay = (g_SettingsReturnTo == GameState::PAUSED);
                if (!settingsOverlay) DrawMenuBackground(sw, sh, delta);
                SceneAppWindow(sw, sh, WW, WH, L"setting.odw", 0.70f, 0.75f, 0.88f, wx, wy,
                               settingsOverlay, 0.0f);
                if (g_AppOpen >= 0.999f) {           // 완전히 열린 뒤에만 콘텐츠
                // ── 중앙 정렬 2열 그리드 (좌: 표시·그래픽 / 우: 조작·효과) ──
                const float OW = 112.0f, OH = 44.0f, OG = 8.0f;
                const float rowH = 58.0f;
                const float contentW = 980.0f;
                const float colW = (contentW - 48.0f) * 0.5f;
                const float cx0 = wx + (WW - contentW) * 0.5f;
                const float colL = cx0;
                const float colR = cx0 + colW + 48.0f;
                const float btnXL = 236.0f;   // 좌열 라벨 폭 (긴 한글 라벨용)
                const float btnXR = 188.0f;

                g_TextL.Draw(T(StrId::SET_TITLE), cx0, wy + 44.0f, 1.05f, 1, 1, 1, 1);

                auto toggleAt = [&](float labX, float btnBase, float ly,
                                    const wchar_t* label, bool& val, float labMaxW) {
                    float lsc = 0.85f;
                    while (lsc > 0.62f && g_TextS.Width(label, lsc) > labMaxW) lsc -= 0.03f;
                    g_TextS.Draw(label, labX, ly + 14.0f, lsc, 1, 1, 1, 0.9f);
                    if (UIButton(btnBase, ly, OW, OH, T(StrId::OPT_ON),
                                 mx, my, lmb, g_LmbPrev, val)) val = true;
                    if (UIButton(btnBase + OW + OG, ly, OW, OH, T(StrId::OPT_OFF),
                                 mx, my, lmb, g_LmbPrev, !val)) val = false;
                };

                // FPS (전체 폭)
                float lineY = wy + 96.0f;
                g_TextS.Draw(T(StrId::SET_FPS), colL, lineY + 12.0f, 0.85f, 1, 1, 1, 0.9f);
                struct FpsOpt { const wchar_t* label; int val; };
                FpsOpt fpsOpts[4] = {
                    { L"30", 30 }, { L"60", 60 }, { L"144", 144 }, { L"300", 300 }
                };
                for (int i = 0; i < 4; i++) {
                    float bx = colL + btnXL + i * (OW + OG);
                    bool sel = (g_FpsCap == fpsOpts[i].val);
                    if (UIButton(bx, lineY, OW, OH, fpsOpts[i].label,
                                 mx, my, lmb, g_LmbPrev, sel)) {
                        g_FpsCap = fpsOpts[i].val;
                        glfwSwapInterval((g_FpsCap == 0) ? 1 : 0);
                    }
                }

                // 언어 (전체 폭)
                lineY = wy + 96.0f + rowH;
                g_TextS.Draw(T(StrId::SET_LANG), colL, lineY + 12.0f, 0.85f, 1, 1, 1, 0.9f);
                struct LangOpt { const wchar_t* label; Language lang; };
                LangOpt langOpts[LANG_COUNT] = {
                    { L"한국어",  Language::KR },
                    { L"English", Language::EN },
                    { L"日本語",  Language::JP },
                };
                for (int i = 0; i < LANG_COUNT; i++) {
                    float bx = colL + btnXL + i * (OW + OG);
                    bool sel = (g_Language == langOpts[i].lang);
                    if (UIButton(bx, lineY, OW, OH, langOpts[i].label,
                                 mx, my, lmb, g_LmbPrev, sel)) {
                        g_Language = langOpts[i].lang;
                    }
                }

                const wchar_t* afLabel = (g_Language==Language::EN)?L"Auto-Fire":
                                         (g_Language==Language::JP)?L"自動発射":L"자동 발사";
                const wchar_t* asLabel = (g_Language==Language::EN)?L"Auto-Skill":
                                         (g_Language==Language::JP)?L"自動スキル":L"자동 스킬";
                const wchar_t* sfLabel = (g_Language==Language::EN)?L"CRT Shader":
                                         (g_Language==Language::JP)?L"CRTシェーダー":L"CRT 셰이더";
                float tY0 = wy + 96.0f + rowH * 2;
                toggleAt(colL, colL + btnXL, tY0,            T(StrId::SET_CROSSHAIR), g_ShowCrosshair, btnXL - 8.0f);
                toggleAt(colL, colL + btnXL, tY0 + rowH,     T(StrId::SET_DMGNUM),    g_ShowDamageNumbers, btnXL - 8.0f);
                toggleAt(colL, colL + btnXL, tY0 + rowH * 2,  T(StrId::SET_COMBO),     g_ShowCombo, btnXL - 8.0f);
                toggleAt(colR, colR + btnXR, tY0,            afLabel, g_AutoFire, btnXR - 8.0f);
                toggleAt(colR, colR + btnXR, tY0 + rowH,     asLabel, g_AutoSkill, btnXR - 8.0f);
                toggleAt(colR, colR + btnXR, tY0 + rowH * 2, sfLabel, g_ShaderFx, btnXR - 8.0f);

                // 몹 외형 · 위성 VFX
                float gfxY = tY0 + rowH * 3;
                const wchar_t* mobLabel = (g_Language==Language::EN)?L"Mob look":
                                          (g_Language==Language::JP)?L"敵見た目":L"몹 외형";
                const wchar_t* mobA = (g_Language==Language::EN)?L"Classic":
                                      (g_Language==Language::JP)?L"クラシック":L"기본";
                const wchar_t* mobB = (g_Language==Language::EN)?L"Soft":
                                      (g_Language==Language::JP)?L"ソフト":L"부드럽";
                g_TextS.Draw(mobLabel, colL, gfxY + 12.0f, 0.85f, 1, 1, 1, 0.9f);
                if (UIButton(colL + btnXL, gfxY, OW, OH, mobA, mx, my, lmb, g_LmbPrev,
                             g_MobVisualStyle == MobVisualStyle::CLASSIC))
                    g_MobVisualStyle = MobVisualStyle::CLASSIC;
                if (UIButton(colL + btnXL + OW + OG, gfxY, OW, OH, mobB, mx, my, lmb, g_LmbPrev,
                             g_MobVisualStyle == MobVisualStyle::SOFT))
                    g_MobVisualStyle = MobVisualStyle::SOFT;

                const wchar_t* vfxLabel = (g_Language==Language::EN)?L"Satellite VFX":
                                          (g_Language==Language::JP)?L"衛星VFX":L"위성 VFX";
                const wchar_t* vfxA = (g_Language==Language::EN)?L"Full":
                                      (g_Language==Language::JP)?L"通常":L"보통";
                const wchar_t* vfxB = (g_Language==Language::EN)?L"Reduced":
                                      (g_Language==Language::JP)?L"節約":L"절약";
                g_TextS.Draw(vfxLabel, colR, gfxY + 12.0f, 0.85f, 1, 1, 1, 0.9f);
                if (UIButton(colR + btnXR, gfxY, OW, OH, vfxA, mx, my, lmb, g_LmbPrev,
                             g_VfxDensity == VfxDensity::FULL))
                    g_VfxDensity = VfxDensity::FULL;
                if (UIButton(colR + btnXR + OW + OG, gfxY, OW, OH, vfxB, mx, my, lmb, g_LmbPrev,
                             g_VfxDensity == VfxDensity::REDUCED))
                    g_VfxDensity = VfxDensity::REDUCED;

                // 사운드 볼륨 — 전체 폭 한 줄 (그래픽 행과 분리)
                {
                    float vy = gfxY + rowH + 8.0f;
                    g_TextS.Draw(T(StrId::SET_SOUND), colL, vy + 12.0f, 0.85f, 1, 1, 1, 0.9f);
                    auto clampVol = [](int v){ return v < 0 ? 0 : (v > 100 ? 100 : v); };
                    float bx0 = colL + btnXL;

                    // [−]
                    if (UIButton(bx0, vy, 44.0f, OH, L"−", mx, my, lmb, g_LmbPrev)) {
                        g_VolEdit = false; g_SoundVol = clampVol(g_SoundVol - 5);
                    }
                    // 슬라이더 (트랙 + 손잡이) — 클릭/드래그로 직접 설정
                    float barX = bx0 + 56.0f, barW = 300.0f;
                    float trackY = vy + OH * 0.5f, trackH = 6.0f;
                    float kx = barX + barW * (g_SoundVol / 100.0f);
                    drawRect(barX, trackY - trackH*0.5f, barW, trackH, 0.10f, 0.12f, 0.16f, 1.0f);     // 트랙
                    drawRect(barX, trackY - trackH*0.5f, kx - barX, trackH, 0.35f, 0.75f, 1.0f, 0.95f); // 채움
                    drawCircle(kx, trackY, 11.0f, 0.35f, 0.8f, 1.0f, 1.0f);    // 손잡이 외곽
                    drawCircle(kx, trackY,  6.0f, 0.95f, 0.98f, 1.0f, 1.0f);   // 손잡이 코어
                    bool barHover = (mx >= barX && mx <= barX + barW && my >= vy && my <= vy + OH);
                    if (lmb && barHover) {   // 누르는 동안(드래그) 마우스 X 로 값 설정
                        g_VolEdit = false;
                        g_SoundVol = clampVol((int)((float)(mx - barX) / barW * 100.0f + 0.5f));
                    }
                    // [+]
                    float plusX = barX + barW + 8.0f;
                    if (UIButton(plusX, vy, 44.0f, OH, L"+", mx, my, lmb, g_LmbPrev)) {
                        g_VolEdit = false; g_SoundVol = clampVol(g_SoundVol + 5);
                    }
                    // 숫자 직접입력 필드
                    float fldX = plusX + 44.0f + 14.0f, fldW = 92.0f;
                    bool fldHover = (mx >= fldX && mx <= fldX + fldW && my >= vy && my <= vy + OH);
                    BindMainShader();
                    drawRect(fldX, vy, fldW, OH, g_VolEdit ? 0.16f : 0.09f,
                             g_VolEdit ? 0.18f : 0.10f, 0.22f, 1.0f);
                    drawRect(fldX, vy, fldW, 2.0f, 0.4f, 0.9f, 0.6f, 0.8f);
                    auto commitVol = [&]() {
                        int v = 0; for (int i = 0; i < g_VolLen; i++) v = v*10 + (g_VolBuf[i]-L'0');
                        if (g_VolLen > 0) g_SoundVol = clampVol(v);
                        g_VolEdit = false;
                    };
                    if (lmb && !g_LmbPrev) {
                        if (fldHover) { g_VolEdit = true; g_VolLen = 0; g_VolBuf[0] = 0; }
                        else if (g_VolEdit) commitVol();   // 다른 곳 클릭 = 확정
                    }
                    // 백스페이스 / 엔터 (엣지 감지)
                    {
                        static bool bsPrev = false, enPrev = false;
                        bool bs = (glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS);
                        if (g_VolEdit && bs && !bsPrev && g_VolLen > 0) g_VolBuf[--g_VolLen] = 0;
                        bsPrev = bs;
                        bool en = (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS);
                        if (g_VolEdit && en && !enPrev) commitVol();
                        enPrev = en;
                    }
                    wchar_t shown[16];
                    if (g_VolEdit) {
                        bool caret = (((int)(glfwGetTime()*2.0)) & 1) == 0;
                        swprintf_s(shown, L"%ls%ls", g_VolLen ? g_VolBuf : L"", caret ? L"|" : L"");
                    } else swprintf_s(shown, L"%d", g_SoundVol);
                    g_TextS.Draw(shown, fldX + 12.0f, vy + 12.0f, 0.9f, 1,1,1,0.95f);
                }

                // PC 사양 — 베타 피드백용 (설정 하단)
                {
                    const wchar_t* specTitle = (g_Language==Language::EN)?L"System specs (for feedback)":
                                               (g_Language==Language::JP)?L"PCスペック (フィードバック用)":
                                               L"PC 사양 (피드백용)";
                    float sy = wy + WH - 118.0f;
                    g_TextS.Draw(specTitle, cx0, sy, 0.72f, 0.62f, 0.72f, 0.82f, 0.88f);
                    int fw = 0, fh = 0;
                    glfwGetFramebufferSize(window, &fw, &fh);
                    std::wstring s1 = GetSystemSpecLine1();
                    std::wstring s2 = GetSystemSpecLine2(fw, fh);
                    float specSc = 0.60f;
                    float maxW = contentW;
                    while (specSc > 0.48f && g_TextS.Width(s1.c_str(), specSc) > maxW) specSc -= 0.02f;
                    g_TextS.Draw(s1.c_str(), cx0, sy + 20.0f, specSc, 0.78f, 0.88f, 0.98f, 0.90f);
                    g_TextS.Draw(s2.c_str(), cx0, sy + 38.0f, specSc, 0.72f, 0.82f, 0.92f, 0.85f);
                }

                // 하단 버튼 — 중앙 정렬
                const float footY = wy + WH - 64.0f;
                const float backW = 180.0f, resetW = 210.0f, credW = 150.0f;
                const float footGap = 16.0f;
                const float footTotal = backW + resetW + credW + footGap * 2.0f;
                const float footX = wx + (WW - footTotal) * 0.5f;
                if (UIButton(footX, footY, backW, 48.0f, T(StrId::BTN_BACK),
                             mx, my, lmb, g_LmbPrev)) {
                    SaveGame();
                    g_GameManager.currentState = g_SettingsReturnTo;
                }
                // 세이브 초기화 (2단계 확인) — 점수/코인/메타/업적/도감/테마 전부 리셋
                {
                    static bool s_resetConfirm = false;
                    const wchar_t* rl = s_resetConfirm
                        ? ((g_Language==Language::EN)?L"Sure? (click again)":
                           (g_Language==Language::JP)?L"本当に？(再クリック)":L"정말? (다시 클릭)")
                        : ((g_Language==Language::EN)?L"Reset Save":
                           (g_Language==Language::JP)?L"セーブ初期化":L"세이브 초기화");
                    float rx = footX + backW + footGap, ry = footY, rwid = resetW;
                    bool rh = (mx>=rx && mx<=rx+rwid && my>=ry && my<=ry+48.0f);
                    BindMainShader();
                    drawRect(rx, ry, rwid, 48.0f, s_resetConfirm?0.40f:0.18f, 0.06f, 0.06f, rh?1.0f:0.9f);
                    drawRect(rx, ry, rwid, 2.0f, 0.95f, 0.3f, 0.3f, 0.9f);
                    float rtw = g_TextS.Width(rl, 0.82f);
                    g_TextS.Draw(rl, rx+(rwid-rtw)*0.5f, ry+15.0f, 0.82f, 1.0f, 0.65f, 0.6f, 1.0f);
                    if (lmb && !g_LmbPrev) {
                        if (rh) {
                            if (!s_resetConfirm) s_resetConfirm = true;
                            else { ResetSaveProgress(); s_resetConfirm = false; }
                        } else s_resetConfirm = false;   // 딴 곳 클릭 = 확인 취소
                    }
                }

                // 크레딧 (오픈소스 에셋 출처) — 버튼 → 오버레이
                static bool s_showCredits = false;
                {
                    const wchar_t* cl = (g_Language==Language::EN)?L"Credits":
                                        (g_Language==Language::JP)?L"クレジット":L"크레딧";
                    float cxp = footX + backW + resetW + footGap * 2.0f;
                    if (UIButton(cxp, footY, credW, 48.0f, cl, mx, my, lmb, g_LmbPrev))
                        s_showCredits = true;
                }
                if (s_showCredits) {
                    BindMainShader();
                    drawRect(0, 0, sw, sh, 0.0f, 0.0f, 0.0f, 0.78f * SCENE_BG_ALPHA);   // 딤
                    float CW = 620.0f, CH = 440.0f;
                    float CX = (sw - CW) * 0.5f, CY = (sh - CH) * 0.5f;
                    drawRect(CX, CY, CW, CH, 0.05f, 0.06f, 0.10f, 0.98f);
                    drawRect(CX, CY, CW, 4.0f, 0.3f, 0.8f, 1.0f, 1.0f);
                    const wchar_t* CT = L"CREDITS";
                    g_TextL.Draw(CT, CX + (CW - g_TextL.Width(CT,1.1f))*0.5f, CY + 24.0f, 1.1f, 1,1,1,1);
                    const wchar_t* lines[] = {
                        L"ONEDOW  —  Desktop Defense",
                        L"",
                        L"Fonts:  Jua / Kosugi Maru  (SIL OFL)",
                        L"Icons:  game-icons.net  (CC BY 3.0)",
                        L"         Lorc · Delapouite · Skoll",
                        L"Missile sprite:  Saepul Nahwan  (Noun Project)",
                        L"Audio engine:  miniaudio  (public domain)",
                        L"Built with:  OpenGL · GLFW · GLAD · glm · stb",
                        L"",
                        L"Made with Claude Code",
                    };
                    float ly = CY + 78.0f;
                    for (auto* ln : lines) {
                        g_TextS.Draw(ln, CX + 36.0f, ly, 0.82f, 0.85f, 0.92f, 1.0f, 0.95f);
                        ly += 32.0f;
                    }
                    if (UIButton(CX + (CW-180.0f)*0.5f, CY + CH - 60.0f, 180.0f, 44.0f,
                                 T(StrId::BTN_BACK), mx, my, lmb, g_LmbPrev))
                        s_showCredits = false;
                }
                }   // close: g_AppOpen open guard
}

void Scene_Ready(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    (void)c;

    auto drawSlashText = [&](const wchar_t* text, float y, float sc, float r, float g, float b, float a) {
        if (!text || !text[0]) return;
        std::vector<std::wstring> lines;
        std::wstring cur;
        for (const wchar_t* p = text; *p; ++p) {
            if (*p == L'/') { if (!cur.empty()) lines.push_back(cur); cur.clear(); }
            else cur += *p;
        }
        if (!cur.empty()) lines.push_back(cur);
        for (auto& s : lines) {
            while (!s.empty() && s.front() == L' ') s.erase(0, 1);
            while (!s.empty() && s.back() == L' ') s.pop_back();
        }
        float lineH = 30.0f * sc;
        float maxW = sw * 0.78f;
        for (int li = 0; li < (int)lines.size(); li++) {
            float lsc = sc;
            while (lsc > 0.55f && g_TextS.Width(lines[li].c_str(), lsc) > maxW) lsc -= 0.04f;
            float lw = g_TextS.Width(lines[li].c_str(), lsc);
            g_TextS.Draw(lines[li].c_str(), (sw - lw) * 0.5f, y + li * lineH, lsc, r, g, b, a);
        }
    };

    const wchar_t* title = ReadyTitle();
    g_TextL.Draw(title, CenterTextX(sw, g_TextL, title, 1.2f), sh * 0.22f, 1.2f,
                 0.55f, 0.90f, 1.0f, 0.98f);

    drawSlashText(ReadyBrief(), sh * 0.36f, 0.95f, 0.92f, 0.95f, 1.0f, 0.92f);

    const wchar_t* hint = ReadyHint();
    g_TextS.Draw(hint, CenterTextX(sw, g_TextS, hint, 0.88f), sh * 0.52f, 0.88f,
                 0.55f, 0.82f, 1.0f, 0.88f);

    const wchar_t* T1 = T(StrId::PRESS_SPACE_TO_START);
    const wchar_t* T2 = T(StrId::ESC_QUIT);
    g_TextL.Draw(T1, CenterTextX(sw, g_TextL, T1, 1.0f), sh * 0.72f, 1.0f, 1, 1, 1, 0.95f);
    g_TextS.Draw(T2, CenterTextX(sw, g_TextS, T2, 1.0f), sh * 0.78f, 1.0f, 0.8f, 0.8f, 0.8f, 0.8f);

    const wchar_t* ctrl = TutorialControls();
    g_TextS.Draw(ctrl, CenterTextX(sw, g_TextS, ctrl, 0.88f), sh * 0.88f, 0.88f,
                 0.55f, 0.85f, 1.0f, 0.95f);
}

void Scene_Paused(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    GLFWwindow* window = c.window;
    const GameState st = g_GameManager.currentState;
    float& fireTimer = *c.fireTimer;
    const std::function<void()>& ResetForNewGame = c.reset;
                // 전체 화면 딤 — 보스 창/엔티티가 메뉴 뒤로 비치지 않게
                BatchFlush();
                glDisable(GL_SCISSOR_TEST);
                glUniformMatrix4fv(g_MainProjLoc, 1, GL_FALSE, g_BaseOrtho);
                memcpy(g_MainOrtho, g_BaseOrtho, sizeof(g_BaseOrtho));
                BindMainShader();
                drawRect(0, 0, sw, sh, 0.02f, 0.02f, 0.06f, 0.86f * SCENE_BG_ALPHA);
                const wchar_t* T1 = T(StrId::PAUSED);
                g_TextL.Draw(T1, CenterTextX(sw, g_TextL, T1, 1.4f), sh*0.22f, 1.4f, 1,1,1,0.95f);

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
                    ResetSettingsUi();
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

void Scene_GameOver(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    GLFWwindow* window = c.window;
    const GameState st = g_GameManager.currentState;
    float& fireTimer = *c.fireTimer;
    const std::function<void()>& ResetForNewGame = c.reset;
                // ── 메뉴 페이드인: 폭발 직후 결과 메뉴가 서서히 떠오름 (0→1, 2.5초) ──
                float gof = g_GameOverFade;          // 0..1
                float ge  = Smoothstep(gof);  // smoothstep (부드럽게)

                // 전체 화면 딤 — 보스 창/엔티티가 결과창 뒤로 비치지 않게 (페이드)
                BindMainShader();
                drawRect(0, 0, sw, sh, 0.02f, 0.02f, 0.06f, 0.88f * ge * SCENE_BG_ALPHA);
                const wchar_t* T1 = T(StrId::GAMEOVER);
                g_TextL.Draw(T1, CenterTextX(sw, g_TextL, T1, 1.6f), sh*0.30f, 1.6f,
                             1, 0.25f, 0.25f, 0.95f * ge);
                // 사망 원인 (프로세스 종료 사유)
                if (g_DeathReason[0]) {
                    g_TextS.Draw(g_DeathReason, CenterTextX(sw, g_TextS, g_DeathReason, 1.0f), sh*0.385f, 1.0f,
                                 1.0f, 0.55f, 0.45f, 0.92f * ge);
                }

                // 결과 — 점수 카운트업(띠리릭) + Best(통계 바로 위) + 레벨/처치/코인
                float cu = gof / 0.75f; if (cu > 1.0f) cu = 1.0f;
                cu = Smoothstep(cu);          // smoothstep → 숫자 롤업 느낌
                long long curScore = (long long)(g_GameManager.score * cu);
                long long bestVal  = g_BestScore[(int)g_Difficulty];
                long long curBest  = g_LastRunRecord ? (long long)(bestVal * cu) : bestVal;  // 신기록이면 같이 롤업

                wchar_t bestBuf[64], scoreBuf[64], lvBuf[64], killBuf[64], coinBuf[64];
                // BEST — 통계(최종점수) 바로 위
                swprintf_s(bestBuf, L"BEST   %lld", curBest);
                g_TextS.Draw(bestBuf, CenterTextX(sw, g_TextS, bestBuf, 1.1f), sh*0.405f, 1.1f,
                             1.0f, 0.85f, 0.4f, 0.95f * ge);
                // 최종 점수 (카운트업, 대)
                swprintf_s(scoreBuf, L"%ls   %lld", T(StrId::FINAL_SCORE), curScore);
                g_TextL.Draw(scoreBuf, CenterTextX(sw, g_TextL, scoreBuf, 1.3f), sh*0.455f, 1.3f,
                             1.0f, 1.0f, 0.7f, 0.95f * ge);
                // 도달 레벨 / 처치 수
                swprintf_s(lvBuf,   L"%ls   Lv. %d", T(StrId::REACHED_LEVEL), g_GameManager.playerLevel);
                swprintf_s(killBuf, L"%ls   %lld",   T(StrId::KILL_COUNT),    g_Stats.killCount);
                g_TextS.Draw(lvBuf,   CenterTextX(sw, g_TextS, lvBuf, 1.05f), sh*0.545f, 1.05f, 0.85f,0.95f,0.85f, 0.95f*ge);
                g_TextS.Draw(killBuf, CenterTextX(sw, g_TextS, killBuf, 1.05f), sh*0.59f,  1.05f, 0.85f,0.95f,0.85f, 0.95f*ge);
                // 코인
                swprintf_s(coinBuf, L"+%lld COIN  (total %lld)", g_LastRunCoins, g_Coins);
                g_TextS.Draw(coinBuf, CenterTextX(sw, g_TextS, coinBuf, 1.0f), sh*0.645f, 1.0f, 1.0f,0.9f,0.3f, 0.95f*ge);
                if (g_LastRunRecord) {
                    const wchar_t* rec = L"* NEW RECORD *";
                    float blink = 0.6f + 0.4f * sinf((float)glfwGetTime() * 6.0f);
                    g_TextL.Draw(rec, CenterTextX(sw, g_TextL, rec, 1.0f), sh*0.355f, 1.0f,
                                 1.0f, 0.9f, 0.2f, blink * ge);
                }

                // (사망 시점 보유 증강은 Scene_OwnedAugPanel — 일시정지와 동일한 호버 패널로 표시.
                //  기존 다열 목록은 제거: 패널과 겹쳐 이중 표시되던 문제 fix)

                // 버튼 3개: 다시하기 / 메뉴로 / 종료 — 페이드 완료 후에만 표시/활성
                if (gof >= 0.999f) {
                    const float BW = 240.0f, BH = 56.0f, BG = 16.0f;
                    float totalW = 3 * BW + 2 * BG;
                    float bx = (sw - totalW) * 0.5f;
                    float by = sh * 0.72f;

                    if (UIButton(bx + 0*(BW+BG), by, BW, BH, T(StrId::BTN_RESTART),
                                 mx, my, lmb, g_LmbPrev)) {
                        ResetForNewGame();
                        FinalizeLoadout(c, FixedWeaponForSelectedJob());
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
}

void Scene_Victory(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    GLFWwindow* window = c.window;
    const std::function<void()>& ResetForNewGame = c.reset;

    float vf = g_VictoryFade;
    float ve = Smoothstep(vf);

    BindMainShader();
    drawRect(0, 0, sw, sh, 0.02f, 0.06f, 0.04f, 0.88f * ve * SCENE_BG_ALPHA);

    static const wchar_t* T1[3] = { L"런 클리어!", L"RUN CLEARED!", L"ランクリア!" };
    int li = LangIndex();
    if (li < 0 || li > 2) li = 0;
    g_TextL.Draw(T1[li], CenterTextX(sw, g_TextL, T1[li], 1.7f), sh * 0.26f, 1.7f,
                 0.35f, 1.0f, 0.55f, 0.98f * ve);

    wchar_t scoreBuf[64], lvBuf[64], killBuf[64], coinBuf[64];
    swprintf_s(scoreBuf, L"%ls   %lld", T(StrId::FINAL_SCORE), g_GameManager.score);
    swprintf_s(lvBuf,   L"%ls   Lv. %d", T(StrId::REACHED_LEVEL), g_GameManager.playerLevel);
    swprintf_s(killBuf, L"%ls   %lld", T(StrId::KILL_COUNT), g_Stats.killCount);
    swprintf_s(coinBuf, L"+%lld COIN  (total %lld)", g_LastRunCoins, g_Coins);
    g_TextL.Draw(scoreBuf, CenterTextX(sw, g_TextL, scoreBuf, 1.25f), sh * 0.46f, 1.25f,
                 1.0f, 1.0f, 0.85f, 0.95f * ve);
    g_TextS.Draw(lvBuf,   CenterTextX(sw, g_TextS, lvBuf, 1.0f), sh * 0.54f, 1.0f,
                 0.85f, 0.95f, 0.85f, 0.95f * ve);
    g_TextS.Draw(killBuf, CenterTextX(sw, g_TextS, killBuf, 1.0f), sh * 0.59f, 1.0f,
                 0.85f, 0.95f, 0.85f, 0.95f * ve);
    g_TextS.Draw(coinBuf, CenterTextX(sw, g_TextS, coinBuf, 1.0f), sh * 0.64f, 1.0f,
                 1.0f, 0.9f, 0.35f, 0.95f * ve);

    if (vf >= 0.999f) {
        const float BW = 240.0f, BH = 56.0f, BG = 16.0f;
        float totalW = 3 * BW + 2 * BG;
        float bx = (sw - totalW) * 0.5f;
        float by = sh * 0.72f;
        if (UIButton(bx + 0*(BW+BG), by, BW, BH, T(StrId::BTN_RESTART),
                     mx, my, lmb, g_LmbPrev)) {
            ResetForNewGame();
            FinalizeLoadout(c, FixedWeaponForSelectedJob());
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
}

void Scene_AugSelect(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    GLFWwindow* window = c.window;
    const GameState st = g_GameManager.currentState;
    float& fireTimer = *c.fireTimer;
    const std::function<void()>& ResetForNewGame = c.reset;

    // ── 진입/종료/hover 애니메이션 상태 ──
    static float s_enterT      = 1.0f;
    static float s_hovY[3]     = {0.f, 0.f, 0.f};  // spring-animated Y offset per card
    static float s_pickFlash[3]= {0.f, 0.f, 0.f};  // 0→1 pick pulse per card
    static int   s_prevHov     = -1;

    auto easeOut = [](float t) -> float {
        float inv = 1.0f - t; return 1.0f - inv * inv * inv;
    };

    // fresh entry: reset all anim state
    {
        const GameState prev = g_GameManager.lastState;
        if (prev != GameState::AUG_SELECT && prev != GameState::DEBUFF_SELECT) {
            s_enterT = 0.0f;
            for (int i = 0; i < 3; i++) { s_hovY[i] = 0.f; s_pickFlash[i] = 0.f; }
            s_prevHov = -1;
        }
    }
    const float ENTER_DUR = 0.38f;
    s_enterT = std::min(s_enterT + delta / ENTER_DUR, 1.0f);

    // pick flash: trigger when hover changes to a new card
    if (g_HoveredAug != s_prevHov && g_HoveredAug >= 0 && g_HoveredAug < 3)
        s_pickFlash[g_HoveredAug] = 1.0f;
    s_prevHov = g_HoveredAug;

    // spring hover Y + flash decay
    for (int i = 0; i < 3; i++) {
        float targetY = (g_HoveredAug == i) ? -18.0f : 0.0f;
        s_hovY[i] += (targetY - s_hovY[i]) * std::min(1.0f, delta * 16.0f);
        if (s_pickFlash[i] > 0.f) s_pickFlash[i] = std::max(0.f, s_pickFlash[i] - delta / 0.22f);
    }

    // exit animation (g_AugExitT driven by main.cpp)
    const float EXIT_DUR = 0.30f;
    float exitPct  = (g_AugExitT >= 0.f) ? std::min(g_AugExitT / EXIT_DUR, 1.0f) : -1.0f;
    float exitE    = (exitPct >= 0.f) ? easeOut(exitPct) : 0.f;

    // ── 레이아웃 상수 ──
    const int   nCards  = 3;
    const float CARD_W  = 280.0f;
    const float CARD_H  = 430.0f;
    const float GAP     = 40.0f;
    const float TOTAL_W = nCards * CARD_W + (nCards - 1) * GAP;
    const float TB      = WIN_TB;
    float baseX = (sw - TOTAL_W) * 0.5f;
    float baseY = (sh - CARD_H)  * 0.40f;

    const bool isDebuff = (st == GameState::DEBUFF_SELECT);

    // ── 다크 오버레이 페이드인/아웃 ──
    float overlayE  = easeOut(s_enterT);
    float overlayA  = overlayE * (exitPct >= 0.f ? (1.0f - exitE) : 1.0f);
    BindMainShader();
    if (isDebuff)
        drawRect(0, 0, sw, sh, 0.14f, 0.02f, 0.02f, overlayA * 0.72f * SCENE_BG_ALPHA);
    else
        drawRect(0, 0, sw, sh, 0.02f, 0.02f, 0.09f, overlayA * 0.72f * SCENE_BG_ALPHA);
    BatchFlush();

    // ── 타이틀 페이드인/아웃 ──
    const wchar_t* TIT = isDebuff ? T(StrId::CHOOSE_DEBUFF) : T(StrId::CHOOSE_AUG);
    float titA = easeOut(std::min(s_enterT * 1.6f, 1.0f));
    if (exitPct >= 0.f) titA *= std::max(0.f, 1.0f - exitPct * 2.5f);
    g_TextL.Draw(TIT, CenterTextX(sw, g_TextL, TIT, 1.0f), baseY - 62.0f, 1.0f,
                 1.0f, 1.0f, 1.0f, 0.95f * titA);
    if (st == GameState::AUG_SELECT) {
        wchar_t slotBuf[96];
        swprintf_s(slotBuf, L"ID %d / %d", CountIdentitySlotsUsed(), IdentitySlotMax());
        g_TextS.Draw(slotBuf, CenterTextX(sw, g_TextS, slotBuf, 0.80f),
                     baseY - 32.0f, 0.80f, 0.72f, 0.92f, 1.0f, 0.85f * titA);
    }
    BatchFlush();

    // ── 카드 3장 ──
    static const wchar_t* KEY_LABELS[3] = { L"[ 1 ]", L"[ 2 ]", L"[ 3 ]" };
    for (int i = 0; i < nCards; i++) {
        // 스태거 슬라이드인
        float stag = (float)i * 0.07f;
        float ct   = std::max(0.0f, std::min((s_enterT - stag) / (1.0f - stag), 1.0f));
        float ce   = easeOut(ct);
        float slideY = (1.0f - ce) * 110.0f;
        float cardA  = ce;

        bool  hov   = (g_HoveredAug == i);
        bool  isConf= (exitPct >= 0.f && g_AugExitSlot == i);  // 선택 확정된 카드
        float flash = s_pickFlash[i];   // 0→1→0 pick pulse

        // exit 애니메이션 오프셋/알파
        float exitOffY = 0.f;
        if (exitPct >= 0.f) {
            if (isConf) {
                // 선택 카드: 위로 날아오르며 페이드
                exitOffY = -exitE * 55.f;
                cardA   *= std::max(0.f, 1.0f - exitE * 1.2f);
            } else {
                // 나머지: 아래로 내려가며 빠르게 페이드
                exitOffY = exitE * 45.f;
                cardA   *= std::max(0.f, 1.0f - exitE * 1.8f);
            }
        }

        float cx = baseX + i * (CARD_W + GAP);
        float cy = baseY + slideY + s_hovY[i] + exitOffY;

        const AugDef& def = ALL_AUGS[g_GameManager.augChoices[i]];
        float tr, tg, tb2;
        GetRarityColor(def.rarity, tr, tg, tb2);
        // 선택 확정 순간 밝기 부스트 (pick flash + isConf)
        float flashBoost = flash * 0.7f + (isConf ? exitE * (1.0f - exitE) * 4.0f * (1.0f - exitPct) : 0.0f);
        float bMul = (hov ? 1.55f : 1.05f) + flashBoost;
        float nr = std::min(1.0f, tr * bMul + 0.08f);
        float ng = std::min(1.0f, tg * bMul + 0.08f);
        float nb = std::min(1.0f, tb2 * bMul + 0.08f);

        // 카드 배경
        BindMainShader();
        float bgMul = (hov ? 0.14f : 0.09f) + flash * 0.06f;
        drawRect(cx, cy, CARD_W, CARD_H,
                 tr * bgMul, tg * bgMul, tb2 * bgMul, 0.94f * cardA);

        // 타이틀바 (rarity 색 다크)
        drawRect(cx, cy, CARD_W, TB,
                 tr * 0.38f, tg * 0.38f, tb2 * 0.38f, 0.97f * cardA);

        // 테두리 (pick flash 시 잠깐 더 밝게)
        BatchFlush(); glEnable(GL_BLEND);
        float bord = (hov ? 1.0f : 0.60f) + flash * 0.5f;
        drawNeonBorder(cx, cy, CARD_W, CARD_H, nr * bord, ng * bord, nb * bord);

        // 타이틀바 텍스트 (등급 배지)
        const wchar_t* badge = GetAugBadge(def);
        BatchFlush();
        float bs = 0.48f;
        g_TextS.Draw(badge, cx + (CARD_W - g_TextS.Width(badge, bs)) * 0.5f,
                     cy + (TB - 11.0f * bs) * 0.5f,
                     bs, nr, ng, nb, 0.95f * cardA);
        BatchFlush();

        // 아이콘
        GLuint icon = IconFor(def.type);
        BindMainShader();
        if (icon) {
            float isz = 112.0f;
            BatchFlush();
            DrawIcon(icon, cx + (CARD_W - isz) * 0.5f, cy + TB + 20.0f,
                     isz, isz, 1.0f, 1.0f, 1.0f, 0.97f * cardA);
            BindMainShader();
        }

        // 구분선
        drawRect(cx + 18.0f, cy + TB + 142.0f, CARD_W - 36.0f, 1.5f,
                 nr, ng, nb, 0.40f * cardA);

        // 이름
        const wchar_t* cardName = AugName(def);
        float nameSc = 1.05f;
        while (nameSc > 0.66f && g_TextL.Width(cardName, nameSc) > CARD_W - 22.0f)
            nameSc -= 0.04f;
        BatchFlush();
        g_TextL.Draw(cardName,
                     cx + (CARD_W - g_TextL.Width(cardName, nameSc)) * 0.5f,
                     cy + TB + 156.0f, nameSc, 1.0f, 1.0f, 1.0f, 0.96f * cardA);
        BatchFlush();

        // 설명 첫 줄 (카드 안)
        const wchar_t* desc = AugDesc(def);
        std::wstring firstLine;
        for (const wchar_t* p = desc; *p && *p != L'/'; ++p) firstLine += *p;
        while (!firstLine.empty() && firstLine.front() == L' ') firstLine.erase(0, 1);
        float dsc = 0.66f;
        while (dsc > 0.48f && g_TextS.Width(firstLine.c_str(), dsc) > CARD_W - 22.0f)
            dsc -= 0.04f;
        BindMainShader();
        g_TextS.Draw(firstLine.c_str(),
                     cx + (CARD_W - g_TextS.Width(firstLine.c_str(), dsc)) * 0.5f,
                     cy + TB + 195.0f, dsc, 0.82f, 0.82f, 0.82f, 0.78f * cardA);
        BatchFlush();

        // 키 힌트 (카드 하단)
        BindMainShader();
        g_TextS.Draw(KEY_LABELS[i],
                     cx + (CARD_W - g_TextS.Width(KEY_LABELS[i], 0.95f)) * 0.5f,
                     cy + CARD_H - 46.0f, 0.95f,
                     nr, ng, nb, 0.90f * cardA);
        BatchFlush();
    }

    // ── 하단 상세 설명 박스 (항상 표시 — 호버 카드 or 0번) ──
    {
        int descIdx = (g_HoveredAug >= 0) ? g_HoveredAug : 0;
        int hIdx    = g_GameManager.augChoices[descIdx];
        if (hIdx < 0) hIdx = 0;
        const AugDef& hDef = ALL_AUGS[hIdx];
        float hr, hg, hb;
        GetRarityColor(hDef.rarity, hr, hg, hb);
        const wchar_t* hDesc = AugDesc(hDef);

        float boxY = baseY + CARD_H + 18.0f;
        float boxW = TOTAL_W;
        float boxH = 142.0f;
        float boxX = (sw - boxW) * 0.5f;
        float boxA = overlayA;  // follows overlay fade (페이드아웃 포함)

        BindMainShader();
        float bgA = isDebuff ? 0.32f : 0.60f;
        drawRect(boxX, boxY, boxW, boxH, 0.03f, 0.03f, 0.07f, bgA * boxA);
        drawRect(boxX, boxY, boxW, 3.0f, hr, hg, hb, 0.90f * boxA);
        BatchFlush();

        std::vector<std::wstring> lines;
        std::wstring cur;
        for (const wchar_t* p = hDesc; *p; ++p) {
            if (*p == L'/') { if (!cur.empty()) lines.push_back(cur); cur.clear(); }
            else cur += *p;
        }
        if (!cur.empty()) lines.push_back(cur);
        for (auto& s : lines) {
            while (!s.empty() && s.front() == L' ') s.erase(0, 1);
            while (!s.empty() && s.back()  == L' ') s.pop_back();
        }
        int n = (int)lines.size(); if (n < 1) n = 1;
        float maxw = 1.0f;
        for (auto& ln : lines) { float w = g_TextS.Width(ln.c_str(), 1.0f); if (w > maxw) maxw = w; }
        float sc = 0.88f;
        if (maxw * sc > boxW - 40.0f) sc = (boxW - 40.0f) / maxw;
        if (sc < 0.56f) sc = 0.56f;
        float lineH  = 30.0f * sc;
        float startY = boxY + 14.0f + (boxH - 14.0f - lineH * n) * 0.5f;
        for (int li = 0; li < n; li++) {
            float lw = g_TextS.Width(lines[li].c_str(), sc);
            g_TextS.Draw(lines[li].c_str(), boxX + (boxW - lw) * 0.5f,
                         startY + lineH * li, sc, 1.0f, 1.0f, 1.0f, 0.90f * boxA);
        }
        BatchFlush();

        // 키 힌트 (박스 아래)
        const wchar_t* HINT = (g_HoveredAug < 0) ? T(StrId::KEY_HINT_NO_HOVER)
                                                  : T(StrId::KEY_HINT_HOVER);
        BindMainShader();
        g_TextS.Draw(HINT, CenterTextX(sw, g_TextS, HINT, 0.80f),
                     boxY + boxH + 12.0f, 0.80f,
                     0.70f, 0.70f, 0.70f, 0.75f * boxA);
        BatchFlush();
    }
}

void Scene_AugReplace(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const float delta = c.delta;
    int newIdx = g_GameManager.pendingAugIdx;
    if (newIdx < 0 || newIdx >= AUG_TOTAL) return;

    // ── 애니메이션 상태 ──
    static float s_enterT      = 0.0f;
    static float s_hovY[32]    = {};
    static float s_pickFlash[32] = {};
    static int   s_prevHov     = -1;

    auto easeOut = [](float t) -> float {
        float inv = 1.0f - t; return 1.0f - inv * inv * inv;
    };

    // 새 진입 감지
    const GameState prev = g_GameManager.lastState;
    if (prev != GameState::AUG_REPLACE) {
        s_enterT = 0.0f;
        std::fill(std::begin(s_hovY),      std::end(s_hovY),      0.f);
        std::fill(std::begin(s_pickFlash), std::end(s_pickFlash), 0.f);
        s_prevHov = -1;
    }
    s_enterT = std::min(s_enterT + delta / 0.32f, 1.0f);

    // pick flash 트리거
    if (g_HoveredAug != s_prevHov && g_HoveredAug >= 0 && g_HoveredAug < 32)
        s_pickFlash[g_HoveredAug] = 1.0f;
    s_prevHov = g_HoveredAug;

    // hover spring + flash decay
    int n = g_GameManager.replaceChoiceCount;
    for (int i = 0; i < n && i < 32; i++) {
        float targetY = (g_HoveredAug == i) ? -10.0f : 0.0f;
        s_hovY[i] += (targetY - s_hovY[i]) * std::min(1.0f, delta * 16.0f);
        if (s_pickFlash[i] > 0.f) s_pickFlash[i] = std::max(0.f, s_pickFlash[i] - delta / 0.20f);
    }

    // exit anim
    const float EXIT_DUR = 0.28f;
    float exitPct = (g_RepExitT >= 0.f) ? std::min(g_RepExitT / EXIT_DUR, 1.0f) : -1.0f;
    float exitE   = (exitPct >= 0.f) ? easeOut(exitPct) : 0.f;
    bool  isCancelExit = (g_RepExitSlot == -2);

    float enterE = easeOut(s_enterT);

    int li = LangIndex();
    if (li < 0 || li > 2) li = 0;

    // ── 오버레이 ──
    float overlayA = enterE * (exitPct >= 0.f ? (1.0f - exitE) : 1.0f);
    BindMainShader();
    drawRect(0, 0, sw, sh, 0.03f, 0.02f, 0.08f, overlayA * 0.68f * SCENE_BG_ALPHA);
    BatchFlush();

    // ── 타이틀 ──
    float titA = enterE;
    if (exitPct >= 0.f) titA *= std::max(0.f, 1.0f - exitPct * 2.5f);

    const wchar_t* TIT[3] = {
        L"식별 슬롯 가득 — 교체할 증강 선택",
        L"Identity slots full — pick one to replace",
        L"識別スロット満杯 — 交換する強化を選択" };
    g_TextL.Draw(TIT[li], CenterTextX(sw, g_TextL, TIT[li], 0.95f), sh * 0.12f,
                 0.95f, 1, 1, 1, 0.95f * titA);

    // NEW 증강 표시
    wchar_t newLine[160];
    swprintf_s(newLine, L"NEW: [%ls] %ls",
               GetAugBadge(ALL_AUGS[newIdx]), AugName(ALL_AUGS[newIdx]));
    g_TextS.Draw(newLine, CenterTextX(sw, g_TextS, newLine, 0.88f), sh * 0.18f,
                 0.88f, 0.9f, 1.0f, 0.7f, 0.95f * titA);
    BatchFlush();

    // ── 카드 목록 ──
    const float CARD_W = 260.0f, CARD_H = 120.0f, GAP = 16.0f;
    float totalW = (float)n * CARD_W + (float)(n > 0 ? n - 1 : 0) * GAP;
    if (totalW > sw - 40.0f) totalW = sw - 40.0f;
    float baseX = (sw - totalW) * 0.5f;
    float baseY = sh * 0.32f;

    static const wchar_t* KEYS[9] = { L"[1]",L"[2]",L"[3]",L"[4]",L"[5]",L"[6]",L"[7]",L"[8]",L"[9]" };

    for (int i = 0; i < n && i < 32; i++) {
        int idx = g_GameManager.replaceChoices[i];
        if (idx < 0 || idx >= AUG_TOTAL) continue;

        // 슬라이드인 스태거
        float stag = (float)i * 0.04f;
        float ct   = std::max(0.f, std::min((s_enterT - stag) / (1.0f - stag + 0.001f), 1.0f));
        float cardA = easeOut(ct);
        float slideY = (1.0f - easeOut(ct)) * 60.0f;

        bool  isConf = (exitPct >= 0.f && g_RepExitSlot == i);
        float flash  = s_pickFlash[i];

        // exit 오프셋
        float exitOffY = 0.f;
        if (exitPct >= 0.f) {
            if (isCancelExit) {
                exitOffY = exitE * 50.0f;
                cardA   *= std::max(0.f, 1.0f - exitE * 1.6f);
            } else if (isConf) {
                exitOffY = -exitE * 50.0f;
                cardA   *= std::max(0.f, 1.0f - exitE * 1.2f);
            } else {
                exitOffY = exitE * 40.0f;
                cardA   *= std::max(0.f, 1.0f - exitE * 1.8f);
            }
        }

        float cx = baseX + i * (CARD_W + GAP);
        float cy = baseY + slideY + s_hovY[i] + exitOffY;

        const AugDef& def = ALL_AUGS[idx];
        float tr, tg, tb;
        GetRarityColor(def.rarity, tr, tg, tb);
        float flashBoost = flash * 0.5f + (isConf ? exitE * (1.0f - exitE) * 3.0f * (1.0f - exitPct) : 0.0f);
        bool  hov = (g_HoveredAug == i);

        BindMainShader();
        float bgMul = (hov ? 0.42f : 0.28f) + flash * 0.12f;
        drawRect(cx, cy, CARD_W, CARD_H, tr * bgMul, tg * bgMul, tb * bgMul, 0.92f * cardA);

        BatchFlush(); glEnable(GL_BLEND);
        float bord = (hov ? 1.0f : 0.55f) + flash * 0.5f + flashBoost;
        float nr = std::min(1.0f, tr * (1.4f + flashBoost) + 0.08f);
        float ng = std::min(1.0f, tg * (1.4f + flashBoost) + 0.08f);
        float nb = std::min(1.0f, tb * (1.4f + flashBoost) + 0.08f);
        drawNeonBorder(cx, cy, CARD_W, CARD_H, nr * bord, ng * bord, nb * bord);

        wchar_t lbl[128];
        swprintf_s(lbl, L"[%d] %ls", i + 1, AugName(def));
        float lsc = 0.78f;
        while (lsc > 0.52f && g_TextL.Width(lbl, lsc) > CARD_W - 16.0f) lsc -= 0.04f;
        BatchFlush();
        g_TextL.Draw(lbl, cx + 10.0f, cy + 16.0f, lsc, 1, 1, 1, 0.95f * cardA);
        if (i < 9) {
            BatchFlush();
            g_TextS.Draw(KEYS[i], cx + 10.0f, cy + CARD_H - 32.0f, 0.85f,
                         nr, ng, nb, 0.90f * cardA);
        }
        BatchFlush();
    }

    // ── 키 힌트 ──
    const wchar_t* HINT[3] = {
        L"1~9: 선택 · Space: 교체 · Esc: 새 증강 포기",
        L"1~9: select · Space: replace · Esc: skip new augment",
        L"1~9: 選択 · Space: 交換 · Esc: 新規強化を見送り" };
    BindMainShader();
    g_TextS.Draw(HINT[li], CenterTextX(sw, g_TextS, HINT[li], 0.82f), sh * 0.78f,
                 0.82f, 0.75f, 0.75f, 0.75f, 0.85f * overlayA);
    BatchFlush();
}

void Scene_RunShop(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const GameState st = g_GameManager.currentState;

    const int  nCards  = 4;
    const float CARD_W = 240.0f;
    const float CARD_H = 360.0f;
    const float GAP    = 32.0f;
    const float TOTAL_W = (float)nCards * CARD_W + (float)(nCards - 1) * GAP;
    float baseX = (sw - TOTAL_W) * 0.5f;
    float baseY = (sh - CARD_H) * 0.38f;

    const wchar_t* TIT[3] = {
        L"런 상점 — 골드로 증강 구매",
        L"Run Shop — buy augments with gold",
        L"ランショップ — ゴールドで強化購入" };
    int li = LangIndex();
    if (li < 0 || li > 2) li = 0;
    g_TextL.Draw(TIT[li], CenterTextX(sw, g_TextL, TIT[li], 0.95f), baseY - 72.0f,
                 0.95f, 1, 1, 1, 0.95f);

    wchar_t goldBuf[48];
    swprintf_s(goldBuf, L"G  %lld", g_RunGold);
    float gw = g_TextL.Width(goldBuf, 0.9f);
    g_TextL.Draw(goldBuf, sw - gw - 24.0f, 18.0f, 0.9f, 1.0f, 0.88f, 0.35f, 0.95f);

    const wchar_t* HINT[3] = {
        L"1~4 선택 · Space 구매 · ESC 나가기 (구역 밖으로 나갔다가 재입장)",
        L"1~4 select · Space buy · ESC leave (re-enter zone after exit)",
        L"1~4 選択 · Space 購入 · ESC 退出 (再入場は一度離れる)" };
    g_TextS.Draw(HINT[li], CenterTextX(sw, g_TextS, HINT[li], 0.82f),
                 baseY + CARD_H + 168.0f, 0.82f, 0.75f, 0.75f, 0.75f, 0.85f);

    static const wchar_t* KEY_LABELS[4] = { L"[ 1 ]", L"[ 2 ]", L"[ 3 ]", L"[ 4 ]" };
    for (int i = 0; i < nCards; i++) {
        float cardX = baseX + i * (CARD_W + GAP);
        float yOff  = (g_HoveredAug == i) ? -14.0f : 0.0f;
        int idx = g_RunShopStock[i];
        bool sold = (idx < 0);

        drawRect(cardX, baseY + yOff, CARD_W, CARD_H,
                 0.04f, 0.05f, 0.07f, sold ? 0.55f : 0.88f);

        if (sold) {
            const wchar_t* SOLD[3] = { L"매진", L"SOLD", L"売切" };
            float tw = g_TextL.Width(SOLD[li], 1.1f);
            g_TextL.Draw(SOLD[li], cardX + (CARD_W - tw) * 0.5f,
                         baseY + yOff + CARD_H * 0.45f, 1.1f,
                         0.5f, 0.5f, 0.55f, 0.7f);
            continue;
        }

        const AugDef& def = ALL_AUGS[idx];
        const wchar_t* cardName = AugName(def);
        float tr, tg, tb;
        GetRarityColor(def.rarity, tr, tg, tb);
        tr = std::min(1.0f, tr * 1.4f + 0.25f);
        tg = std::min(1.0f, tg * 1.4f + 0.25f);
        tb = std::min(1.0f, tb * 1.4f + 0.25f);
        const wchar_t* topLabel = GetAugBadge(def);
        GLuint icon = IconFor(def.type);
        if (icon) {
            float isz = 120.0f;
            DrawIcon(icon, cardX + (CARD_W - isz) * 0.5f,
                     baseY + yOff + CARD_H * 0.12f, isz, isz,
                     1.0f, 1.0f, 1.0f, 0.97f);
        }
        float rw = g_TextS.Width(topLabel, 1.0f);
        g_TextS.Draw(topLabel, cardX + (CARD_W - rw) * 0.5f,
                     baseY + yOff + 18.0f, 1.0f, tr, tg, tb, 0.95f);

        float nameSc = 1.05f;
        while (nameSc > 0.65f &&
               g_TextL.Width(cardName, nameSc) > CARD_W - 16.0f)
            nameSc -= 0.05f;
        float nw = g_TextL.Width(cardName, nameSc);
        g_TextL.Draw(cardName, cardX + (CARD_W - nw) * 0.5f,
                     baseY + yOff + CARD_H * 0.52f, nameSc, 1, 1, 1, 0.95f);

        wchar_t priceBuf[32];
        swprintf_s(priceBuf, L"G %d", g_RunShopPrice[i]);
        bool afford = (g_RunGold >= g_RunShopPrice[i]);
        float pw = g_TextS.Width(priceBuf, 1.0f);
        g_TextS.Draw(priceBuf, cardX + (CARD_W - pw) * 0.5f,
                     baseY + yOff + CARD_H - 72.0f, 1.0f,
                     afford ? 1.0f : 0.55f,
                     afford ? 0.85f : 0.4f,
                     afford ? 0.35f : 0.35f, 0.95f);

        float kw = g_TextS.Width(KEY_LABELS[i], 1.0f);
        g_TextS.Draw(KEY_LABELS[i], cardX + (CARD_W - kw) * 0.5f,
                     baseY + yOff + CARD_H - 40.0f, 1.0f, tr, tg, tb, 0.95f);
    }

    if (g_HoveredAug >= 0 && g_HoveredAug < nCards &&
        g_RunShopStock[g_HoveredAug] >= 0) {
        int hIdx = g_RunShopStock[g_HoveredAug];
        const AugDef& hDef = ALL_AUGS[hIdx];
        const wchar_t* hDesc = AugDesc(hDef);
        float hr, hg, hb;
        GetRarityColor(hDef.rarity, hr, hg, hb);
        float boxY = baseY + CARD_H + 20.0f;
        float boxW = TOTAL_W;
        float boxH = 130.0f;
        float boxX = (sw - boxW) * 0.5f;
        drawRect(boxX, boxY, boxW, boxH, 0.03f, 0.03f, 0.05f, 0.85f);
        drawRect(boxX, boxY, boxW, 4.0f, hr, hg, hb, 1.0f);
        std::vector<std::wstring> lines;
        std::wstring cur;
        for (const wchar_t* p = hDesc; *p; ++p) {
            if (*p == L'/') {
                if (!cur.empty()) lines.push_back(cur);
                cur.clear();
            } else cur += *p;
        }
        if (!cur.empty()) lines.push_back(cur);
        int n = (int)lines.size();
        if (n < 1) n = 1;
        float maxw = 1.0f;
        for (auto& ln : lines) {
            float w = g_TextS.Width(ln.c_str(), 1.0f);
            if (w > maxw) maxw = w;
        }
        float sc = 0.9f;
        if (maxw * sc > boxW - 40.0f) sc = (boxW - 40.0f) / maxw;
        if (sc < 0.6f) sc = 0.6f;
        float lineH = 30.0f * sc;
        float startY = boxY + 18.0f + (boxH - 18.0f - lineH * n) * 0.5f;
        for (int li2 = 0; li2 < n; li2++) {
            const wchar_t* s = lines[li2].c_str();
            float lw = g_TextS.Width(s, sc);
            g_TextS.Draw(s, boxX + (boxW - lw) * 0.5f,
                         startY + lineH * (float)li2, sc,
                         1.0f, 1.0f, 1.0f, 0.95f);
        }
    }
    (void)st;
}

void Scene_OwnedAugPanel(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    GLFWwindow* window = c.window;
    const GameState st = g_GameManager.currentState;
    float& fireTimer = *c.fireTimer;
    const std::function<void()>& ResetForNewGame = c.reset;
                // 같은 인덱스 카운트 (스택)
                int counts[AUG_TOTAL] = {};
                for (int idx : g_OwnedAugs) counts[idx]++;
                // 보유 증강을 티어(등급)순으로 정렬
                int ord[AUG_TOTAL], nord = 0;
                for (int i = 0; i < AUG_TOTAL; i++) if (counts[i] > 0) ord[nord++] = i;
                std::sort(ord, ord + nord, [](int a, int b) {
                    return AugTierIndexLess(a, b);
                });

                const float PX  = 16.0f;
                const float ROW_H = 24.0f;
                const float HDR_H = 20.0f;
                const float COLW  = 320.0f;          // 리스트 클릭/호버 가로 범위
                const wchar_t* TITLE = T(StrId::OWNED_AUGS);
                g_TextS.Draw(TITLE, PX, 60.0f, 1.1f, 1, 1, 1, 0.95f);
                // 현재 무기 (항상 표시)
                {
                    const wchar_t* wPrefix = (g_Language==Language::EN)?L"Weapon:":
                                             (g_Language==Language::JP)?L"武器:":
                                             L"현재 무기:";
                    wchar_t wLine[160];
                    swprintf_s(wLine, L"%ls %ls", wPrefix, CurrentWeaponLabel());
                    float wSc = 0.78f;
                    while (wSc > 0.62f && g_TextS.Width(wLine, wSc) > COLW - 4.0f) wSc -= 0.03f;
                    g_TextS.Draw(wLine, PX + 2.0f, 90.0f, wSc,
                                 0.55f, 0.85f, 1.0f, 0.92f);
                }

                // 리스트 뷰 영역 — 하단 스킬/HP HUD 바로 위까지 (넘치면 스크롤)
                const float listTop    = 110.0f;
                float listBottom = sh - 175.0f;               // 스킬 슬롯/HP 패널 위까지
                if (listBottom < listTop + 4.0f * ROW_H) listBottom = listTop + 4.0f * ROW_H;
                const float viewH      = listBottom - listTop;
                int hdrCount = 0;
                AugRarity prevR = (AugRarity)-1;
                for (int oi = 0; oi < nord; oi++) {
                    AugRarity r = ALL_AUGS[ord[oi]].rarity;
                    if (r != prevR) { hdrCount++; prevR = r; }
                }
                const float contentH   = (float)nord * ROW_H + (float)hdrCount * HDR_H;

                // 마우스 휠 스크롤 (리스트 위에서만 소비)
                static float s_ownScroll = 0.0f;
                bool overList = (mx >= 0 && mx <= COLW && my >= listTop && my <= listBottom);
                if (overList && g_ScrollAccum != 0.0f)
                    s_ownScroll -= g_ScrollAccum * ROW_H * 1.5f;
                g_ScrollAccum = 0.0f;   // 매 프레임 소비 (다른 곳에서 안 쓰면 무시)
                float maxScroll = (contentH > viewH) ? (contentH - viewH) : 0.0f;
                if (s_ownScroll < 0.0f)        s_ownScroll = 0.0f;
                if (s_ownScroll > maxScroll)   s_ownScroll = maxScroll;

                // 리스트 (scissor 클립 + 스크롤)
                int   hoverAug = -1;
                float hoverRowY = 0.0f;
                BatchFlush(); glEnable(GL_SCISSOR_TEST);
                glScissor(0, (GLint)(sh - listBottom), (GLint)(COLW + 10.0f), (GLint)viewH);
                prevR = (AugRarity)-1;
                float ry = listTop - s_ownScroll;
                for (int oi = 0; oi < nord; oi++) {
                    int i = ord[oi];
                    AugRarity rar = ALL_AUGS[i].rarity;
                    if (rar != prevR) {
                        if (ry >= listTop - HDR_H && ry <= listBottom) {
                            wchar_t rh[48];
                            swprintf_s(rh, L"-- %ls --", GetRarityKR(rar));
                            g_TextS.Draw(rh, PX, ry, 0.72f, 0.55f, 0.75f, 0.95f, 0.88f);
                        }
                        ry += HDR_H;
                        prevR = rar;
                    }
                    if (ry < listTop - ROW_H || ry > listBottom) { ry += ROW_H; continue; }
                    const AugDef& def = ALL_AUGS[i];
                    float cr, cg, cb;
                    GetRarityColor(def.rarity, cr, cg, cb);
                    cr = std::min(1.0f, cr * 1.3f + 0.25f);
                    cg = std::min(1.0f, cg * 1.3f + 0.25f);
                    cb = std::min(1.0f, cb * 1.3f + 0.25f);
                    bool rowHover = (overList && my >= ry - 2.0f && my < ry + ROW_H - 4.0f);
                    if (rowHover) {
                        hoverAug = i; hoverRowY = ry;
                        BindMainShader();
                        drawRect(0, ry - 2.0f, COLW, ROW_H, 0.15f, 0.16f, 0.26f, 0.6f);
                        drawRect(0, ry - 2.0f, 3.0f, ROW_H, cr, cg, cb, 1.0f);
                    }
                    wchar_t line[128];
                    if (counts[i] > 1)
                        swprintf_s(line, L"· [%ls] %ls  ×%d", GetAugBadge(def), AugName(def), counts[i]);
                    else
                        swprintf_s(line, L"· [%ls] %ls", GetAugBadge(def), AugName(def));
                    float rowSc = 0.82f;
                    while (rowSc > 0.62f && g_TextS.Width(line, rowSc) > COLW - PX - 10.0f) rowSc -= 0.03f;
                    g_TextS.Draw(line, PX, ry, rowSc, cr, cg, cb, 0.9f);
                    ry += ROW_H;
                }
                BatchFlush(); glDisable(GL_SCISSOR_TEST);

                // 스크롤바 (내용이 넘칠 때만)
                if (maxScroll > 0.0f) {
                    BindMainShader();
                    float trackX = COLW + 2.0f;
                    drawRect(trackX, listTop, 4.0f, viewH, 0.12f, 0.12f, 0.16f, 0.6f);
                    float thumbH = viewH * (viewH / contentH);
                    float thumbY = listTop + (viewH - thumbH) * (s_ownScroll / maxScroll);
                    drawRect(trackX, thumbY, 4.0f, thumbH, 0.5f, 0.6f, 0.8f, 0.9f);
                }

                // 무기 줄 호버 (리스트 y=110 이전 — 증강 행과 분리)
                const float WEAPON_Y = 90.0f;
                bool overWeapon = (mx >= 0 && mx <= COLW &&
                                   my >= WEAPON_Y - 4.0f && my <= WEAPON_Y + 18.0f);

                // 우측 상세 패널 — 증강 행: 증강 설명 / 무기 줄: 무기 설명
                auto wrapDescLines = [&](const wchar_t* src, float dsc, float dWmax,
                                         std::vector<std::wstring>& out) {
                    std::vector<std::wstring> dl; std::wstring cur2;
                    for (const wchar_t* p = src; *p; ++p) {
                        if (*p == L'/') { if (!cur2.empty()) dl.push_back(cur2); cur2.clear(); }
                        else cur2 += *p;
                    }
                    if (!cur2.empty()) dl.push_back(cur2);
                    out.clear();
                    for (auto& ln : dl) {
                        while (!ln.empty() && ln.front() == L' ') ln.erase(0, 1);
                        if (g_TextS.Width(ln.c_str(), dsc) <= dWmax) { out.push_back(ln); continue; }
                        std::wstring acc, word;
                        auto fw = [&]() {
                            if (word.empty()) return;
                            std::wstring tr = acc.empty() ? word : acc + L" " + word;
                            if (g_TextS.Width(tr.c_str(), dsc) > dWmax && !acc.empty()) {
                                out.push_back(acc); acc = word;
                            } else acc = tr;
                            word.clear();
                        };
                        for (wchar_t ch : ln) { if (ch == L' ') fw(); else word += ch; }
                        fw();
                        if (!acc.empty()) out.push_back(acc);
                    }
                };

                auto drawSidePanel = [&](float BY, float br, float bg, float bb,
                                         const wchar_t* title, const wchar_t* descSrc) {
                    const float BW = std::min(380.0f, sw - (COLW + 30.0f));
                    const float BH = 138.0f;
                    float BX = COLW + 18.0f;
                    if (BY + BH > sh - 20.0f) BY = sh - 20.0f - BH;
                    if (BY < 20.0f) BY = 20.0f;
                    BindMainShader();
                    drawRect(BX, BY, BW, BH, 0.03f, 0.03f, 0.06f, 0.95f);
                    drawRect(BX, BY, BW, 4.0f, br, bg, bb, 1.0f);
                    g_TextS.Draw(title, BX + 12.0f, BY + 12.0f, 0.90f,
                                 std::min(1.0f, br * 1.4f + 0.3f),
                                 std::min(1.0f, bg * 1.4f + 0.3f),
                                 std::min(1.0f, bb * 1.4f + 0.3f), 1.0f);
                    const float dsc = 0.76f, dWmax = BW - 24.0f;
                    std::vector<std::wstring> wrapped;
                    wrapDescLines(descSrc, dsc, dWmax, wrapped);
                    float dy = BY + 40.0f;
                    for (auto& w : wrapped) {
                        if (dy > BY + BH - 14.0f) break;
                        g_TextS.Draw(w.c_str(), BX + 12.0f, dy, dsc, 1.0f, 1.0f, 0.95f, 0.90f);
                        dy += 22.0f;
                    }
                };

                if (hoverAug >= 0) {
                    const AugDef& sd = ALL_AUGS[hoverAug];
                    float hr, hg, hb;
                    GetRarityColor(sd.rarity, hr, hg, hb);
                    wchar_t hd[128];
                    swprintf_s(hd, L"[%ls] %ls", GetAugBadge(sd), AugName(sd));
                    drawSidePanel(hoverRowY - 6.0f, hr, hg, hb, hd, AugDesc(sd));
                } else if (overWeapon) {
                    drawSidePanel(WEAPON_Y - 4.0f, 0.35f, 0.75f, 1.0f,
                                  CurrentWeaponLabel(), CurrentWeaponDescText());
                }
}

bool TryEscNavigateBack() {
    using GS = GameState;
    GameState& st = g_GameManager.currentState;
    switch (st) {
    case GS::SHOP:
        st = GS::MAIN_MENU;
        return true;
    case GS::CODEX:
        CodexSearchClear();
        st = GS::MAIN_MENU;
        return true;
    case GS::TUTORIAL:
        st = GS::MAIN_MENU;
        return true;
    case GS::JOB_SELECT:
        st = g_CreativeMode ? GS::CREATIVE_CONFIG : GS::RUN_CONFIG;
        return true;
    case GS::RUN_CONFIG:
        st = GS::MAIN_MENU;
        return true;
    case GS::CREATIVE_CONFIG:
        st = GS::RUN_CONFIG;
        return true;
    case GS::SETTINGS:
        SaveGame();
        st = g_SettingsReturnTo;
        return true;
    default:
        return false;
    }
}
