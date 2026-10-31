#include "Scenes.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "GameManager.h"
#include "GameContext.h"
#include "SceneUI.h"
#include "SceneInternal.h"
#include "RunStats.h"
#include "UiLayout.h"
#include "WindowChrome.h"
#include "Settings.h"
#include "Translations.h"
#include "TutorialText.h"
#include "Meta.h"
#include "Achievements.h"
#include "Codex.h"
#include "PlayerStats.h"
#include "Weapons.h"
#include "ExpSystem.h"
#include "SaveSystem.h"
#include "DrawPrim.h"
#include "../../Render/UiColors.h"
#include "../../Render/PlanetShader.h"
#include "../../Render/NebulaGlowShader.h"
#include "MainShader.h"
#include "../../System/Audio.h"
#include "TextRenderer.h"
#include "IconSystem.h"
#include "../System/SystemInfo.h"
#include "Camera.h"
#include "../../Render/PlayerWeaponShell.h"
#include "../WaveSystem.h"
#include "EntityDraw.h"
#include "Monster.h"
#include "../../Render/Juice.h"
#include <algorithm>
#include <vector>
#include <string>
#include <cmath>
#include <functional>

#include "SceneInternal.h"

static int s_RcWeapon = 0;
static int s_RcDifficulty = 1;   // 0=EASY 1=NORMAL 2=HARD 3=NIGHTMARE, 패널을 다시 열어도 유지
static bool s_RcStateLoaded = false;
static float s_RcEntryT = 0.0f;

namespace {

// ── PLAY 패널 배치 상수 (uiS 를 곱해 쓴다) ──
constexpr float kPlayColumnInset  = 72.0f;   // 직업/난이도 열을 페이지 여백에서 안쪽으로
constexpr float kPlayColumnGap    = 34.0f;   // 좌우 열과 중앙 열 사이
constexpr float kPlayEntryTopGap  = 84.0f;   // 본문 상단 → 첫 선택지
constexpr float kPlayEntryStep    = 108.0f;  // 선택지 간격
constexpr float kPlayEntryHeight  = 84.0f;   // 선택지 한 줄의 세로선 길이/클릭 높이
constexpr float kPlayEntryTextX   = 22.0f;   // 세로선 → 글자
constexpr float kPlayGaugeStep    = 28.0f;   // 난이도 별 게이지 간격
constexpr float kPlayTextShadow   = 0.92f;   // 밝은 바탕화면 위에서도 읽히도록 글자 그림자를 진하게
constexpr float kPlayGaugeSize    = 11.0f;   // 난이도 별 게이지 노드 크기
constexpr float kPlayEntrySelectedScale = 1.12f;   // 선택된 이름의 확대 배율
constexpr float kPlayHeroFadeSec   = 0.30f;  // 직업을 바꿀 때 새 외형이 나타나는 시간(초)
constexpr float kPlayLabelScale     = 1.20f;   // 직업/난이도 머리글 확대 배율(Title 단계 기준)
constexpr int   kPlayWeaponCount  = 2;
constexpr int   kPlaySelectableWeapons = 1;   // 전기장(1)은 코드에 남기되 선택 UI에서만 뺀다
constexpr int   kPlayDifficultyCount = 4;

struct PlayRGB { float r, g, b; };
constexpr PlayRGB kPlayWeaponColor[kPlayWeaponCount] = {
    { 0.28f, 0.90f, 1.00f },   // 소총
    { 1.00f, 0.72f, 0.24f },   // 전기장
};
// 난이도 강조색: 쉬움=초록, 보통=하늘, 어려움=붉은 주황, 나이트메어=보라.
constexpr PlayRGB kPlayDifficultyColor[kPlayDifficultyCount] = {
    { 0.38f, 0.95f, 0.62f }, { 0.48f, 0.82f, 1.00f }, { 1.00f, 0.42f, 0.30f },
    { 0.80f, 0.34f, 1.00f },
};

// ── PLAY 패널 문구 (KR, EN, JP) ──
const LStr kTxtPlay        = { { L"플레이", L"PLAY", L"プレイ" } };
const LStr kTxtClass       = { { L"직업", L"CLASS", L"クラス" } };
const LStr kTxtDifficulty  = { { L"난이도", L"DIFFICULTY", L"難易度" } };
const LStr kTxtBest        = { { L"최고 점수", L"BEST", L"ベスト" } };
const LStr kTxtNoRecord    = { { L"기록 없음", L"NO RECORD", L"記録なし" } };
const LStr kTxtWeaponName[kPlayWeaponCount] = {
    { { L"소총", L"RIFLE", L"ライフル" } },
    { { L"전기장", L"FIELD", L"フィールド" } },
};
const LStr kTxtWeaponSub[kPlayWeaponCount] = {
    { { L"정밀 · 단일 대상", L"PRECISION · SINGLE TARGET", L"精密 · 単一目標" } },
    { { L"범위 제어 · 지속", L"AREA CONTROL · SUSTAINED", L"範囲制御 · 持続" } },
};
const LStr kTxtDifficultyName[kPlayDifficultyCount] = {
    { { L"쉬움", L"EASY", L"イージー" } },
    { { L"보통", L"NORMAL", L"ノーマル" } },
    { { L"어려움", L"HARD", L"ハード" } },
    { { L"악몽", L"NIGHTMARE", L"ナイトメア" } },
};

constexpr int kPlayStatCount = 6;
const LStr kTxtStatName[kPlayWeaponCount][kPlayStatCount] = {
    { { { L"공격력", L"DAMAGE", L"攻撃力" } },
      { { L"연사 속도", L"FIRE RATE", L"連射速度" } },
      { { L"초당 피해", L"DPS", L"毎秒ダメージ" } },
      { { L"탄속", L"SPEED", L"弾速" } },
      { { L"퍼짐", L"SPREAD", L"拡散" } },
      { { L"사거리", L"RANGE", L"射程" } } },
    { { { L"펄스 피해", L"PULSE DAMAGE", L"パルス威力" } },
      { { L"초당 펄스", L"PULSES / SEC", L"毎秒パルス" } },
      { { L"초당 펄스 피해", L"PULSE DPS", L"毎秒パルス威力" } },
      { { L"전파 속도", L"TRAVEL SPEED", L"伝播速度" } },
      { { L"공격각", L"COVERAGE", L"攻撃角" } },
      { { L"반경", L"RADIUS", L"半径" } } },
};
const LStr kTxtScreenRange = { { L"화면", L"SCREEN", L"画面" } };
const LStr kTxtInstant = { { L"즉시", L"INSTANT", L"即時" } };
// 스탯 막대 길이(0~1). 값이 아니라 무기 간 상대 비교용이다.
constexpr float kPlayStatBar[kPlayWeaponCount][kPlayStatCount] = {
    { 0.50f, 0.78f, 0.78f, 0.72f, 0.68f, 0.72f },
    { 0.22f, 0.86f, 0.36f, 1.00f, 0.50f, 0.28f },   // DPS: single pulse target, far below the rifle
};

}  // namespace

void ResetRunConfigUi(float entryStart) {
    s_RcEntryT = std::max(0.0f, std::min(1.0f, entryStart));
    ResetTrials();
    s_RcWeapon = 0;
    s_RcStateLoaded = false;
}


void FinalizeLoadout(const SceneCtx& c, int wIdx) {
    float& fireTimer = *c.fireTimer;

    g_Difficulty = static_cast<Difficulty>(std::max(0, std::min(kPlayDifficultyCount - 1, s_RcDifficulty)));
    ApplyLoadoutStats(g_Stats, wIdx);   // RunStats.h: weapon + trial max HP
    g_CurrentWeapon = wIdx;
    fireTimer = g_Stats.fireInterval;
    g_GameManager.maxHP    = g_Stats.maxHP;
    g_GameManager.playerHP = g_Stats.maxHP;
    g_PrevHP               = g_Stats.maxHP;
    g_GameManager.currentState = GameState::READY;
}

// 선택된 직업의 고정 무기 인덱스.  현재 플레이 가능한 직업은 소총과
// 전기장 두 가지이며, 예약 슬롯은 항상 기본 소총으로 폴백한다.
int FixedWeaponForSelectedJob() {
    if (IsPlayableJob(g_SelectedJob) && g_SelectedJob != JOB_NONE &&
        JOB_DEFS[g_SelectedJob].fixedWeapon >= 0)
        return JOB_DEFS[g_SelectedJob].fixedWeapon;
    return (int)StartWeapon::RIFLE;
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

                const float uiS = UiScale(sw, sh);
                const float topY = 44.0f * uiS;
                const wchar_t* TIT = L"CREATIVE";
                const float titleScale = UiTextScale(g_TextL, UiTextLevel::Title, uiS);
                g_TextL.Draw(TIT, (sw - g_TextL.Width(TIT, titleScale)) * 0.5f,
                             topY, titleScale, 0.85f, 0.95f, 0.6f, 1.0f);

                const float OBW = 130.0f * uiS, OBH = 44.0f * uiS, OBG = 10.0f * uiS;
                const float leftX = 40.0f * uiS;
                const float contentTop = topY + 52.0f * uiS;
                const float footH = 64.0f * uiS;
                const float footY = BottomLeftActionY(sh, footH, uiS);

                g_TextS.Draw(L"Start Score", leftX, contentTop,
                             UiTextScale(g_TextS, UiTextLevel::Description, uiS), 1, 1, 1, 0.9f);
                struct ScoreOpt { const wchar_t* l; long long v; };
                ScoreOpt sOpts[5] = { {L"0",0},{L"200k",200000},{L"400k",400000},{L"500k",500000} };
                for (int i = 0; i < 4; i++) {
                    float ox = leftX + i * (OBW + OBG);
                    bool sel = (g_CreativeStartScore == sOpts[i].v);
                    if (UIButton(ox, contentTop + 28.0f, OBW, OBH, sOpts[i].l,
                                 mx, my, lmb, g_LmbPrev, sel,
                                 PanelButtonSlideSide::Both, uiS))
                        g_CreativeStartScore = sOpts[i].v;
                }

                {
                    static float s_creStartHov = 0.0f, s_creBackHov = 0.0f;
                    const float fnow = (float)glfwGetTime();
                    const float fdt  = std::min(delta, 0.05f);

                    const float SW = 280.0f * uiS, SH = footH, SX = (sw - SW) * 0.5f;
                    const bool sHov = (mx >= SX - 12.0f && mx <= SX + SW + 12.0f &&
                                       my >= footY && my <= footY + SH);
                    s_creStartHov += ((sHov ? 1.0f : 0.0f) - s_creStartHov) * std::min(1.0f, fdt * 10.0f);
                    DrawUnifiedMenuCommand(L"START", SX, footY, SW, SH,
                                          0.38f, 0.95f, 0.62f, 1.0f,
                                          s_creStartHov, false, 0.0f, fnow,
                                          uiS, false, true, false);
                    if (sHov && lmb && !g_LmbPrev) {
                        ResetForNewGame();
                        FinalizeLoadout(c, FixedWeaponForSelectedJob());
                    }

                    const float BW = 286.0f * uiS, BH = footH;
                    const float BX = BottomLeftActionX(uiS);
                    const bool bHov = (mx >= BX - 12.0f && mx <= BX + BW + 12.0f &&
                                       my >= footY && my <= footY + BH);
                    s_creBackHov += ((bHov ? 1.0f : 0.0f) - s_creBackHov) * std::min(1.0f, fdt * 10.0f);
                    DrawUnifiedMenuCommand(T(StrId::BTN_BACK), BX, footY, BW, BH,
                                          0.48f, 0.82f, 1.0f, 1.0f,
                                          s_creBackHov, false, 0.0f, fnow + 0.17f,
                                          uiS, false, true, false);
                    if (bHov && lmb && !g_LmbPrev)
                        g_GameManager.currentState = GameState::MAIN_MENU;
                }
}

// PLAY 패널 — 직업(좌) + 무기 성좌(중) + 난이도(우). 박스/판 없이 글자·선·노드만 쓴다.
// 선택 상태는 밝기와 얇은 세로선, 별 노드로만 구분한다.
void Scene_RunConfigInline(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh, dt = std::min(c.delta, 0.05f);
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const bool lmbClick = lmb && !g_LmbPrev;

    static float exitT = 0.0f;
    static bool exiting = false;
    static bool launchExit = false;
    static float launchT = 0.0f;
    static bool prevEsc = false;
    static bool prevKey[4] = {};   // up, down, left, right
    static int weapon = 0;
    static float weaponHover[kPlayWeaponCount] = {};
    static float diffHover[kPlayDifficultyCount] = {};
    static float backHover = 0.0f;
    static float playHover = 0.0f;
    static float heroClock = 0.0f;   // 외형 회전용 연속 시계. 패널을 다시 열어도 이어서 돈다.
    static float heroFade = 1.0f;    // 직업을 바꾼 직후 새 외형이 나타나는 진행도 0..1
    static int heroWeapon = -1;
    static float statT[kPlayStatCount] = {};
    static bool statInit = false;
    if (!statInit) {
        for (int i = 0; i < kPlayStatCount; ++i) statT[i] = kPlayStatBar[0][i];
        statInit = true;
    }
    // 이 패널의 모든 글자는 얇은 검은 테두리를 두른다: 8방향으로 어둡게 한 번씩 먼저 그린다.
    auto DrawShadowedText = [&](TextRenderer& rd, const wchar_t* text, float x, float y,
                                float scale, float r, float g, float b, float a, float shadow) {
        const float o = std::max(0.6f, 0.65f * UiScale(c.sw, c.sh));
        const float d = o * 0.7071f;
        const float dx[8] = { o, -o, 0, 0, d, -d, d, -d };
        const float dy[8] = { 0, 0, o, -o, d, d, -d, -d };
        for (int i = 0; i < 8; ++i)
            ::DrawShadowedText(rd, text, x + dx[i], y + dy[i], scale,
                               0.0f, 0.0f, 0.012f, a * 0.85f, 0.0f);
        ::DrawShadowedText(rd, text, x, y, scale, r, g, b, a, shadow);
    };
    auto weaponName = [&](int id) { return T(kTxtWeaponName[id]); };
    auto weaponSub  = [&](int id) { return T(kTxtWeaponSub[id]); };
    auto statName   = [&](int id) { return T(kTxtStatName[weapon][id]); };
    auto diffName   = [&](int id) { return T(kTxtDifficultyName[id]); };

    if (c.inputFocusChanged) {
        for (bool& k : prevKey) k = true;   // 포커스 복귀 직후 키 입력 무시
    }

    if (!s_RcStateLoaded && !exiting) {
        weapon = std::max(0, std::min(kPlaySelectableWeapons - 1, s_RcWeapon));
        for (float& h : weaponHover) h = 0.0f;
        for (float& h : diffHover) h = 0.0f;
        backHover = playHover = 0.0f;
        heroFade = 1.0f;
        heroWeapon = weapon;
        s_RcStateLoaded = true;
        launchExit = false;
        launchT = 0.0f;
    }

    s_RcEntryT = std::min(1.0f, s_RcEntryT + dt);
    if (launchExit) launchT = std::min(0.50f, launchT + dt);

    const bool esc = c.window && glfwGetKey(c.window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    const bool rmb = c.window && glfwGetMouseButton(c.window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    if (c.inputFocusChanged) prevEsc = esc;
    const bool backInput = (esc && !prevEsc) || (rmb && !g_RmbPrev);
    prevEsc = esc;

    const float entryIn = SceneTransitionEase(s_RcEntryT / kOutgameTransitionDuration);
    const float entryHeader = SceneTransitionEase(
        (s_RcEntryT - 0.04f) / (kOutgameTransitionDuration - 0.04f));
    if (backInput && !exiting && !launchExit && s_RcEntryT >= 0.55f)
        exiting = true;
    if (exiting) exitT = std::min(kOutgameTransitionDuration, exitT + dt);
    const float backP = exiting
        ? SceneTransitionEase(exitT / kOutgameTransitionDuration) : 0.0f;
    const float launchP = Smoothstep(LogoClamp01((launchT - 0.28f) / 0.22f));
    const float contentA = entryIn * (1.0f - backP) * (1.0f - launchP);
    const bool ready = !exiting && !launchExit && s_RcEntryT >= 0.55f;

    SetSceneTextureReveal(exiting
        ? std::max(0.0f, 1.0f - backP)
        : SceneTransitionEase(s_RcEntryT / kOutgameTransitionDuration));

    const float uiS = UiScale(sw, sh);
    const float pageL = std::max(42.0f, 72.0f * uiS);
    const float pageR = sw - std::max(42.0f, 72.0f * uiS);
    const float pageW = std::max(420.0f, pageR - pageL);
    const float headerY = std::max(30.0f, 48.0f * uiS);
    const float footerY = sh - std::max(62.0f, 84.0f * uiS);
    const float bodyTop = headerY + 76.0f * uiS;
    const float bodyBottom = footerY - 26.0f * uiS;
    const float sideW = std::min(286.0f * uiS, pageW * 0.23f);
    const float playMetaScale = UiTextScale(g_TextS, UiTextLevel::Supporting, uiS);
    const float playSubtitleScale = UiTextScale(g_TextS, UiTextLevel::Subtitle, uiS);
    const float playBodyScale = UiTextScale(g_TextS, UiTextLevel::Description, uiS);
    const float entryTitleScale = UiTextScale(g_TextS, UiTextLevel::Title, uiS);
    // 좌우 열은 같은 너비로 페이지 양끝에 붙이고, 중앙 열이 화면 중앙이 되게 한다.
    // 머리글·ESC 는 다른 패널과 같은 페이지 여백에 두고, 선택 열만 안쪽으로 들인다.
    const float leftX = pageL + kPlayColumnInset * uiS;
    const float rightR = pageR - kPlayColumnInset * uiS;
    const float rightX = rightR - sideW;
    const float centerL = leftX + sideW + kPlayColumnGap * uiS;
    const float centerR = rightX - kPlayColumnGap * uiS;
    const float centerW = std::max(330.0f * uiS, centerR - centerL);
    const float chartCX = centerL + centerW * 0.50f;
    const float chartCY = bodyTop + (bodyBottom - bodyTop) * 0.44f;
    const float chartR = std::min(244.0f * uiS,
        std::min(centerW * 0.34f, (bodyBottom - bodyTop) * 0.29f));
    const float routeX = BottomLeftActionX(uiS);
    const float routeW = 286.0f * uiS;
    const float routeH = 64.0f * uiS;
    const float routeY = BottomLeftActionY(sh, routeH, uiS);
    const float playW = 270.0f * uiS;
    const float playH = 62.0f * uiS;
    const float playX = chartCX - playW * 0.5f;
    const float playY = routeY - 1.0f * uiS;
    const float now = (float)glfwGetTime();
    const float weaponR = kPlayWeaponColor[weapon].r;
    const float weaponG = kPlayWeaponColor[weapon].g;
    const float weaponB = kPlayWeaponColor[weapon].b;

    for (int i = 0; i < kPlayStatCount; ++i)
        statT[i] = UiApproach(statT[i], kPlayStatBar[weapon][i], dt, 8.0f);

    // 키보드: 위/아래(W/S)=난이도, 좌/우(A/D)=직업. 누른 순간에만 한 칸 이동.
    auto keyDown = [&](int a, int b) {
        return c.window && (glfwGetKey(c.window, a) == GLFW_PRESS ||
                            glfwGetKey(c.window, b) == GLFW_PRESS);
    };
    const bool keyNow[4] = {
        keyDown(GLFW_KEY_UP, GLFW_KEY_W), keyDown(GLFW_KEY_DOWN, GLFW_KEY_S),
        keyDown(GLFW_KEY_LEFT, GLFW_KEY_A), keyDown(GLFW_KEY_RIGHT, GLFW_KEY_D)
    };
    if (ready) {
        if (keyNow[0] && !prevKey[0]) s_RcDifficulty = std::max(0, s_RcDifficulty - 1);
        if (keyNow[1] && !prevKey[1]) s_RcDifficulty = std::min(kPlayDifficultyCount - 1, s_RcDifficulty + 1);
        if (keyNow[2] && !prevKey[2]) { weapon = 0; s_RcWeapon = weapon; }
        if (keyNow[3] && !prevKey[3] && kPlaySelectableWeapons > 1) { weapon = 1; s_RcWeapon = weapon; }
    }
    for (int i = 0; i < 4; ++i) prevKey[i] = keyNow[i];

    BatchFlush();
    DrawPersistentSceneSideVignettes(sw, sh,
                                     kWideSceneLinearAlpha * contentA);
    // 직업을 바꾸면 새 외형이 짧게 페이드 인한다. 회전 시계는 패널을 다시 열어도 이어서 돈다.
    // 패널을 여는 순간의 프레임 지연이 애니메이션 점프로 보이지 않게 한 프레임에 최대 1/30초만 진행한다.
    const float heroDt = std::min(dt, 1.0f / 30.0f);
    heroClock += heroDt;
    if (heroWeapon != weapon) {
        heroWeapon = weapon;
        heroFade = 0.0f;
    }
    heroFade = std::min(1.0f, heroFade + heroDt / kPlayHeroFadeSec);
    const float heroAlpha = contentA * Smoothstep(heroFade);
    // 외형 크기·위치: 제목 글자(아래 끝)와 스탯(위 끝) 사이의 빈 공간에 맞춘다.
    const float heroTitleBottom = chartCY - chartR - 58.0f * uiS
        + g_TextL.Height(weaponName(weapon), UiTextScale(g_TextL, UiTextLevel::Title, uiS));
    const float heroGapTop = heroTitleBottom + 14.0f * uiS;
    const float heroGapBottom = chartCY + chartR + 26.0f * uiS - 14.0f * uiS;
    const float heroCY = (heroGapTop + heroGapBottom) * 0.5f;
    const float heroR = std::max(40.0f * uiS, std::min(
        std::min(chartR, (heroGapBottom - heroGapTop) * 0.5f), centerW * 0.5f - 10.0f * uiS));

    const float playTitleScale = UiTextScale(g_TextL, UiTextLevel::Title, uiS);
    DrawShadowedText(g_TextL, T(kTxtPlay), pageL, headerY,
                     playTitleScale, 1.0f, 1.0f, 1.0f,
                     0.98f * entryHeader, kPlayTextShadow);
    LogoLine(pageL, headerY + 58.0f * uiS, pageR, headerY + 58.0f * uiS,
             0.7f * uiS, weaponR, weaponG, weaponB, 0.12f * contentA);

    // 섹션 머리글: 작은 라벨 + 얇은 선 하나.
    auto drawSectionLabel = [&](const wchar_t* label, float x, float w) {
        // 큰 글자는 g_TextL(큰 글리프로 구운 아틀라스)로 그려 확대 번짐과 테두리가 생기지 않게 한다.
        const float labelScale = UiTextScale(g_TextL, UiTextLevel::Title, uiS) * kPlayLabelScale;
        DrawShadowedText(g_TextL, label, x, bodyTop, labelScale,
                         weaponR, weaponG, weaponB, 0.92f * contentA, kPlayTextShadow);
        const float ruleY = bodyTop + g_TextL.Height(label, labelScale)
                          + 10.0f * uiS;
        LogoLine(x, ruleY, x + w, ruleY, 0.9f * uiS,
                 weaponR, weaponG, weaponB, 0.36f * contentA);
    };
    const float entryTop = bodyTop + kPlayEntryTopGap * uiS;
    const float entryStep = kPlayEntryStep * uiS;
    const float entryH = kPlayEntryHeight * uiS;
    // 두 열 모두 같은 위쪽 기준선에서 시작한다.
    auto entryY = [&](int slot) { return entryTop + slot * entryStep; };

    // 선택지 한 줄: 세로선 + (선택 시) 노드 + 이름 + 보조 줄. 직업과 난이도가 공유한다.
    // 클릭되면 true. 이름 줄의 세로 중심을 nameMidY 로 돌려준다.
    auto drawChoice = [&](float x, float w, float y0, bool sel, float& hover,
                          PlayRGB col, const wchar_t* name, const wchar_t* sub,
                          float* nameMidY = nullptr, bool brackets = false,
                          const wchar_t* subBig = nullptr, bool bars = false) -> bool {
        const bool hov = ready && mx >= x - 14.0f * uiS && mx <= x + w
            && my >= y0 - 8.0f * uiS && my <= y0 + entryH;
        hover = UpdateMenuCommandHover(hover, hov, dt);
        // 선택 안 된 행도 밝은 바탕에서 읽히도록 알파 하한을 높게 둔다.
        const float a = (sel ? 1.0f : 0.80f + 0.20f * hover) * contentA;

        // ── 배치: 글자를 먼저 재서 브라켓 박스가 글자를 감싸도록 한다 ──
        // 브라켓 행은 글자를 박스 안에서 좌우 같은 안쪽 여백으로 앉힌다.
        const bool boxed = brackets || bars;
        const float textOff = boxed ? 22.0f : kPlayEntryTextX;
        const float textRightPad = boxed ? 28.0f : 4.0f;
        const float textFitW = (w - (textOff + textRightPad) * uiS);
        const float textX = x + textOff * uiS;
        const float baseScale = std::min(entryTitleScale,
            entryTitleScale * textFitW /
            std::max(1.0f, g_TextS.Width(name, entryTitleScale)));
        const float selScale = baseScale * kPlayEntrySelectedScale;
        const float drawScale = sel ? selScale : baseScale;
        // 배치는 선택 크기를 기준으로 고정해, 선택이 바뀌어도 보조 줄이 움직이지 않게 한다.
        const float layoutH = g_TextS.Height(name, selScale);
        const float nameY = y0 + 2.0f * uiS
                          + (layoutH - g_TextS.Height(name, drawScale)) * 0.5f;
        const float subTop = y0 + 2.0f * uiS + layoutH + (boxed ? 4.0f : 10.0f) * uiS;
        // 보조 줄: 큰 숫자(subBig) + 작은 꼬리(sub), 또는 sub 한 덩어리. 폭을 넘으면 같이 줄인다.
        const float subScale0 = boxed ? playBodyScale * 0.88f : playBodyScale;
        const float bigGap = 4.0f * uiS;
        float subScale = subScale0, bigScale = 0.0f;
        if (subBig) {
            const float bigScale0 = subScale0 * 1.45f;
            const float total = g_TextS.Width(subBig, bigScale0) + bigGap
                              + g_TextS.Width(sub, subScale0);
            const float f = total > textFitW ? textFitW / total : 1.0f;
            bigScale = bigScale0 * f;
            subScale = subScale0 * f;
        } else {
            const float subW = g_TextS.Width(sub, subScale0);
            if (subW > textFitW) subScale = subScale0 * textFitW / subW;
        }
        const float subBottom = subTop + g_TextS.Height(sub, subBig ? bigScale : subScale);

        if (brackets) {
            // 외곽 브라켓: 네 모서리에 ㄱ자 팔만 둔다. 선택되면 팔이 길고 굵어지고 색이 켜진다.
            const float bx0 = x - 2.0f * uiS, bx1 = x + w - 2.0f * uiS;
            const float by0 = y0 - 4.0f * uiS;
            const float by1 = std::max(y0 + entryH - 8.0f * uiS, subBottom + 6.0f * uiS);
            const float arm = (sel ? 9.0f : 5.0f + 3.0f * hover) * uiS;
            const float th = (sel ? 1.6f : 0.9f) * uiS;
            const float ba = (sel ? 1.0f : 0.34f + 0.30f * hover) * contentA;
            const float ov = th * 0.5f;   // 모서리 이음새가 비지 않게 팔을 반 굵기만큼 겹친다
            for (int ix = 0; ix < 2; ++ix) {
                for (int iy = 0; iy < 2; ++iy) {
                    const float cx0 = ix ? bx1 : bx0, cy0 = iy ? by1 : by0;
                    const float sx = ix ? -1.0f : 1.0f, sy = iy ? -1.0f : 1.0f;
                    LogoLine(cx0 - sx * ov, cy0, cx0 + sx * arm, cy0, th, col.r, col.g, col.b, ba);
                    LogoLine(cx0, cy0 - sy * ov, cx0, cy0 + sy * arm, th, col.r, col.g, col.b, ba);
                }
            }
        } else if (bars) {
            // 뒤로 버튼과 같은 문법: 호버할 때만 양옆 세로 슬라이드 바 + 바깥쪽 다이아 + 얇은 스캔선이 켜진다.
            const float active = hover;
            const float top = y0 - 4.0f * uiS;
            const float bot = std::max(y0 + entryH - 8.0f * uiS, subBottom + 6.0f * uiS);
            if (active > 0.01f) {
            const float midY = (top + bot) * 0.5f;
            const float lineH = (30.0f + 56.0f * active) * uiS;
            const float slide = (10.0f + 14.0f * active) * uiS;
            const float barA = (0.24f + 0.55f * active) * contentA;
            const float barW = 2.2f * uiS;
            drawRect(x - slide, midY - lineH * 0.5f, barW, lineH, col.r, col.g, col.b, barA);
            drawRect(x + w + slide - barW, midY - lineH * 0.5f, barW, lineH, col.r, col.g, col.b, barA);
            const float dia = (3.0f + 2.6f * active) * uiS;
            const float diaA = (0.40f + 0.34f * active) * contentA;
            drawDiamond(x - slide - 6.0f * uiS, midY, dia, col.r, col.g, col.b, diaA);
            drawDiamond(x + w + slide + 4.0f * uiS, midY, dia, col.r, col.g, col.b, diaA);
            const float scanY = top + (bot - top) * (0.34f + 0.32f * fmodf(now * 0.55f, 1.0f));
            drawRect(x - 4.0f * uiS, scanY, w * (0.58f + 0.24f * active), 1.1f * uiS,
                     col.r, col.g, col.b, (0.05f + 0.09f * active) * contentA);
            }
        } else {
            // 선택: 굵고 긴 세로선 + 큰 노드 + 한 단계 큰 이름. 비선택: 가늘고 짧은 선.
            const float lineLen = sel ? entryH : entryH * 0.72f;
            LogoLine(x, y0, x, y0 + lineLen, (sel ? 2.6f : 1.0f) * uiS,
                     col.r, col.g, col.b,
                     (sel ? 1.0f : 0.42f + 0.22f * hover) * contentA);
            if (sel) drawDiamond(x, y0, 12.0f * uiS, col.r, col.g, col.b, 1.0f * contentA);
        }

        const float nameX = textX;
        DrawShadowedText(g_TextS, name, nameX, nameY, drawScale,
                         sel ? 1.0f : 0.90f, sel ? 1.0f : 0.94f, sel ? 1.0f : 0.98f,
                         a, kPlayTextShadow);
        // 보조 줄은 강조색을 흰색 쪽으로 섞어 밝은 바탕에서도 색이 죽지 않게 한다.
        const float mix = 0.30f;
        const float sr = col.r + (1.0f - col.r) * mix, sg = col.g + (1.0f - col.g) * mix,
                    sb = col.b + (1.0f - col.b) * mix;
        if (subBig) {
            // 큰 숫자와 작은 꼬리의 기준선을 맞춘다.
            const float bigW = g_TextS.Width(subBig, bigScale);
            const float baseline = subTop + g_TextS.BaselineOffset(bigScale);
            DrawShadowedText(g_TextS, subBig, textX, subTop, bigScale,
                             1.0f, 1.0f, 1.0f, a, kPlayTextShadow);
            DrawShadowedText(g_TextS, sub, textX + bigW + bigGap,
                             baseline - g_TextS.BaselineOffset(subScale), subScale,
                             sr, sg, sb, a, kPlayTextShadow);
        } else {
            const float subX = textX;
            DrawShadowedText(g_TextS, sub, subX, subTop, subScale,
                             sr, sg, sb, a, kPlayTextShadow);
        }
        if (nameMidY) *nameMidY = y0 + 2.0f * uiS + layoutH * 0.5f;
        return hov && lmbClick;
    };

    // 직업 목록(좌)
    drawSectionLabel(T(kTxtClass), leftX, sideW);
    for (int i = 0; i < kPlaySelectableWeapons; ++i) {
        if (drawChoice(leftX, sideW, entryY(i), weapon == i,
                       weaponHover[i], kPlayWeaponColor[i],
                       weaponName(i), weaponSub(i),
                       nullptr, false, nullptr, true)) {
            weapon = i;
            s_RcWeapon = weapon;
        }
    }

    // 난이도 목록(우)
    drawSectionLabel(T(kTxtDifficulty), rightX, sideW);
    for (int i = 0; i < kPlayDifficultyCount; ++i) {
        // 큰 숫자 = 이 난이도에서 내가 도달한 최대 웨이브, 작은 꼬리 = 이 난이도의 마지막 웨이브.
        const int finalWave = FinalWaveCount(static_cast<Difficulty>(i));
        const int reached = static_cast<int>(std::min<long long>(g_BestWave[i], finalWave));
        wchar_t bigBuf[16] = {};
        wchar_t tailBuf[48] = {};
        const wchar_t* bigText = nullptr;
        if (reached > 0) {
            swprintf_s(bigBuf, L"%d", reached);
            bigText = bigBuf;
            swprintf_s(tailBuf, L"/%d", finalWave);
        } else {
            swprintf_s(tailBuf, L"%ls", T(kTxtNoRecord));
        }
        const PlayRGB col = kPlayDifficultyColor[i];
        const bool sel = s_RcDifficulty == i;
        float midY = 0.0f;
        if (drawChoice(rightX, sideW, entryY(i), sel,
                       diffHover[i], col, diffName(i), tailBuf, &midY, true, bigText))
            s_RcDifficulty = i;
        // 이름 줄 오른쪽 끝의 4칸 게이지: 켜진 별 개수가 단계(1~4)다.
        for (int k = 0; k < kPlayDifficultyCount; ++k)
            drawDiamond(rightR - 10.0f * uiS
                            - (kPlayDifficultyCount - 1 - k) * kPlayGaugeStep * uiS,
                        midY, kPlayGaugeSize * uiS, col.r, col.g, col.b,
                        (k <= i ? (sel ? 1.0f : 0.72f) : 0.26f) * contentA);
    }

    // 중앙: 선택 직업 이름
    const float heroNameY = chartCY - chartR - 58.0f * uiS;
    const float heroNameScale = UiTextScale(g_TextL, UiTextLevel::Title, uiS);
    const float heroNameW = g_TextL.Width(weaponName(weapon), heroNameScale);
    DrawShadowedText(g_TextL, weaponName(weapon), chartCX - heroNameW * 0.5f,
                     heroNameY, heroNameScale,
                     1.0f, 1.0f, 1.0f, 0.96f * contentA, kPlayTextShadow);

    // 별자리 뒤의 CircleTexture 한 쌍: 어두운 큰 원이 밝은 바탕에서 대비를 주고,
    // 직업 색을 살짝 띤 작은 원이 중심을 받친다.
    DrawConstellationDisc(chartCX, heroCY, heroR * 1.45f,
                          0.0f, 0.0f, 0.0f, 0.88f * heroAlpha);
    DrawConstellationDisc(chartCX, heroCY, heroR * 0.86f,
                          weaponR * 0.16f, weaponG * 0.16f, weaponB * 0.16f,
                          0.40f * heroAlpha, false);
    BindMainShader();

    // 소총 외곽이 가장 크다(약 1.9 sz). 그 반지름이 heroR 에 들어가게 맞춘다.
    DrawPlayerShellPreview(weapon, chartCX, heroCY, heroR / 2.0f, heroClock,
                           weaponR, weaponG, weaponB, heroAlpha);

    const float statY = chartCY + chartR + 26.0f * uiS;
    const float statRowStep = 60.0f * uiS;
    const float statColumnGap = 30.0f * uiS;
    const float statLabelScale = UiTextScale(g_TextS, UiTextLevel::Description, uiS);
    const float statValueScale = UiTextScale(g_TextS, UiTextLevel::Description, uiS);
    const float statW = std::max(96.0f * uiS,
        (centerW - statColumnGap * 2.0f) / 3.0f);
    const PlayerStats previewStats = PreviewBaseStats(
        static_cast<int>(weapon == 0 ? StartWeapon::RIFLE : StartWeapon::SMG), 1.0f);
    wchar_t statValues[kPlayStatCount][32] = {};
    const bool field = weapon == 1;
    const float damage = previewStats.GetBaseDamage()
        * previewStats.GetDamageMultiplier()
        * (field ? STATIC_FIELD_PULSE_DAMAGE_MULT : 1.0f);
    const float interval = field ? STATIC_FIELD_PULSE_INTERVAL : previewStats.fireInterval;
    // Every rate reads per second (/s); counts use thousands separators.
    swprintf_s(statValues[0], L"%.1f", damage);
    swprintf_s(statValues[1], L"%.1f/s", 1.0f / interval);
    swprintf_s(statValues[2], L"%.1f/s", damage / interval);
    if (field) {
        wcscpy_s(statValues[3], T(kTxtInstant));
        wcscpy_s(statValues[4], L"360°");
        FormatGrouped(std::llround(STATIC_FIELD_PULSE_RADIUS), statValues[5], 32);
    } else {
        wchar_t speedText[24];
        FormatGrouped(std::llround(previewStats.bulletSpeed), speedText, 24);
        swprintf_s(statValues[3], L"%ls/s", speedText);
        swprintf_s(statValues[4], L"%.2f", previewStats.bulletSpread);
        wcscpy_s(statValues[5], T(kTxtScreenRange));
    }
    for (int i = 0; i < kPlayStatCount; ++i) {
        const int col = i % 3;
        const int row = i / 3;
        const float x = centerL + col * (statW + statColumnGap);
        const float y = statY + row * statRowStep;
        const float valueW = g_TextS.Width(statValues[i], statValueScale);
        const float textInset = 4.0f * uiS;
        const float barX = x + textInset;
        const float barRight = x + statW - textInset;
        DrawShadowedText(g_TextS, statName(i), x + textInset, y,
                         statLabelScale, 0.72f, 0.82f, 0.92f,
                         0.78f * contentA, kPlayTextShadow);
        DrawShadowedText(g_TextS, statValues[i],
                         x + statW - valueW - textInset, y,
                         statValueScale, 0.92f, 0.96f, 1.0f,
                         0.90f * contentA, kPlayTextShadow);
        LogoLine(barX, y + 31.0f * uiS, barRight,
                 y + 31.0f * uiS, 0.95f * uiS,
                 0.34f, 0.44f, 0.56f, 0.38f * contentA);
        LogoLine(barX, y + 31.0f * uiS,
                 barX + (barRight - barX) * LogoClamp01(statT[i]),
                 y + 31.0f * uiS, 1.75f * uiS,
                 weaponR, weaponG, weaponB, 0.70f * contentA);
    }
    BatchFlush();

    // 하단: BACK | PLAY
    const float routeHitX = routeX - 22.0f * uiS;
    const float routeHitW = routeW + 44.0f * uiS;
    const bool backHov = ready && mx >= routeHitX && mx <= routeHitX + routeHitW
        && my >= routeY && my <= routeY + routeH;
    const bool playHov = ready && mx >= playX && mx <= playX + playW
        && my >= playY - 12.0f * uiS && my <= playY + playH + 10.0f * uiS;
    backHover = UpdateMenuCommandHover(backHover, backHov, dt);
    playHover = UpdateMenuCommandHover(playHover, playHov, dt);
    const wchar_t* const* backVariants = kStrings[(int)StrId::BTN_BACK];
    DrawUnifiedMenuCommand(T(StrId::BTN_BACK),
                           routeX, routeY, routeW, routeH,
                           0.48f, 0.82f, 1.0f, contentA,
                           backHover, false, 0.0f, now,
                           uiS, true, true, true,
                           backVariants, LANG_COUNT);
    DrawUnifiedMenuCommand(T(kTxtPlay),
                           playX, playY, playW, playH,
                           weaponR, weaponG, weaponB, contentA,
                           playHover, true,
                           0.12f + 0.10f * sinf(now * 2.2f), now + 0.51f,
                           uiS, true, true, true,
                           kTxtPlay.t, LANG_COUNT);

    if (backHov && lmbClick && ready) exiting = true;
    if (playHov && lmbClick && ready) {
        // 시련 시스템은 UI에서 빠졌으므로 선택 없는 상태로 고정한다.
        ResetTrials();
        g_TrialPoolReady = true;
        launchExit = true;
        launchT = 0.0f;
    }

    g_BatchAlpha = 1.0f;
    if (launchExit && launchT >= 0.50f) {
        g_SelectedJob = weapon == 0 ? JOB_NONE : JOB_STATIC_FIELD;
        s_RcWeapon = weapon;
        s_MainMenuRunConfigPanel = false;
        if (g_CreativeMode) g_GameManager.currentState = GameState::CREATIVE_CONFIG;
        else { c.reset(); FinalizeLoadout(c, FixedWeaponForSelectedJob()); }
        launchExit = false;
        launchT = 0.0f;
    }
    if (exiting && exitT >= kOutgameTransitionDuration) {
        s_MainMenuRunConfigPanel = false;
        s_MainMenuResumeFromPanel = true;
        ResetRunConfigUi();
        g_MainMenuEntryT = 1.0f;
        s_RcEntryT = 0.0f;
        exitT = 0.0f;
        exiting = false;
        launchExit = false;
        launchT = 0.0f;
        g_GameManager.currentState = GameState::MAIN_MENU;
    }
}
