#include "Scenes.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "GameManager.h"
#include "GameContext.h"
#include "SceneUI.h"
#include "SceneInternal.h"
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
#include "../../Render/BlurShader.h"
#include "../../System/Audio.h"
#include "TextRenderer.h"
#include "IconSystem.h"
#include "../System/SystemInfo.h"
#include "Camera.h"
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
static bool s_RcTrialEnabled[TRIAL_DEF_COUNT] = {};
static bool s_RcTrialStateLoaded = false;
static float s_RcEntryT = 0.0f;

void ResetRunConfigUi(float entryStart) {
    s_RcEntryT = std::max(0.0f, std::min(1.0f, entryStart));
    g_Difficulty = Difficulty::NORMAL;
    ResetTrials();
    s_RcWeapon = 0;
    s_RcTrialStateLoaded = false;
    for (int i = 0; i < TRIAL_DEF_COUNT; ++i) s_RcTrialEnabled[i] = false;
}


void FinalizeLoadout(const SceneCtx& c, int wIdx) {
    float& fireTimer = *c.fireTimer;

    g_Difficulty = Difficulty::NORMAL;
    g_Stats.baseFireInterval = g_Stats.fireInterval;
    ApplyWeapon(g_Stats, (StartWeapon)wIdx);
    g_CurrentWeapon = wIdx;
    fireTimer = g_Stats.fireInterval;
    float trialHpMul = TrialPlayerMaxHpMult();
    if (trialHpMul < 0.999f)
        g_Stats.maxHP *= trialHpMul;
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

// PLAY 패널 — "STAR CHART": 무기 성좌(좌) + 굴레 궤도 띠(중) + 런 요약/LAUNCH(우).
// 모든 선택지는 별 노드로 표현하고, 글로우는 선택된 별에만 제한한다.
void Scene_RunConfigInline(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh, dt = std::min(c.delta, 0.05f);
    const double mx = c.mx, my = c.my;
    const bool ko = LangIndex() == 0;
    auto PlayText = [&](const wchar_t* kr, const wchar_t* en,
                        const wchar_t* jp = nullptr) -> const wchar_t* {
        if (LangIndex() == 0) return kr;
        if (LangIndex() == 2 && jp) return jp;
        return en;
    };
    const bool lmb = c.lmb;
    const bool lmbClick = lmb && !g_LmbPrev;

    static float exitT = 0.0f;
    static bool exiting = false;
    static bool launchExit = false;
    static float launchT = 0.0f;
    static bool prevEsc = false;
    static int weapon = 0;
    static int trialFocus = 0;
    static int trialSelectedSlot = 0;
    static bool trialLayoutPending = true;
    static float trialDisplaySlot = 0.0f;
    static float trialTargetSlot = 0.0f;
    static int trialNavHoldDir = 0;
    static float trialNavHoldT = 0.0f;
    static float trialNavRepeatT = 0.0f;
    static bool trialDragging = false;
    static bool trialDragMoved = false;
    static float trialDragLastY = 0.0f;
    static float trialDragLastX = 0.0f;
    static int trialPressedId = -1;
    static float detailScroll = 0.0f;
    static float detailScrollTarget = 0.0f;
    static bool detailDragging = false;
    static bool detailDragMoved = false;
    static float detailDragLastY = 0.0f;
    static float trialHover[TRIAL_DEF_COUNT] = {};
    static float weaponHover[2] = {};
    static float backHover = 0.0f;
    static float playHover = 0.0f;
    static float resetHover = 0.0f;
    static float statT[6] = { 0.50f, 0.78f, 0.78f, 0.72f, 0.68f, 0.72f };
    static const float kStatTargets[2][6] = {
        { 0.50f, 0.78f, 0.78f, 0.72f, 0.68f, 0.72f },
        { 0.22f, 0.86f, 0.94f, 1.00f, 0.50f, 0.28f }
    };
    if (c.inputFocusChanged) {
        trialDragging = false;
        trialDragMoved = false;
        trialPressedId = -1;
        detailDragging = false;
        detailDragMoved = false;
        trialNavHoldDir = 0;
        trialNavHoldT = 0.0f;
        trialNavRepeatT = 0.0f;
    }

    static constexpr int kTrialCatalogCount = TRIAL_DEF_COUNT - 6;
    static const int kTrialOrder[kTrialCatalogCount] = {
        // EARLY
        0, 1, 4, 5, 8, 9, 10,
        // MID
        11, 13,
        // LATE
        6, 7, 14, 15, 16,
    };
    static const wchar_t* kTrialTags[TRIAL_DEF_COUNT] = {
        L"SPEED UP · SCORE +15%", L"SURVIVAL DOWN · SCORE +15%",
        L"",                       L"",
        L"BUILD DOWN · SCORE +15%",L"COOLING DOWN · SCORE +15%",
        L"DURABILITY UP · SCORE +15%",L"PRESSURE UP · SCORE +15%",
        L"EARLY · SCORE +14%",     L"EARLY · SCORE +16%",
        L"OPENING DOWN · SCORE +18%",L"MID · SCORE +18%",
        L"",                            L"MID · SCORE +17%",
        L"LATE · SCORE +22%",       L"LATE · SCORE +24%",
        L"LATE · SCORE +21%",       L"",
        L"",                          L""
    };
    static const wchar_t* kTrialTagsKR[TRIAL_DEF_COUNT] = {
        L"속도 증가 · 점수 +15%", L"생존력 감소 · 점수 +15%",
        L"",                    L"",
        L"빌드 제약 · 점수 +15%", L"쿨다운 증가 · 점수 +15%",
        L"내구도 증가 · 점수 +15%", L"압박 증가 · 점수 +15%",
        L"초반 · 점수 +14%", L"초반 · 점수 +16%",
        L"시작 제약 · 점수 +18%", L"중반 · 점수 +18%",
        L"",                    L"중반 · 점수 +17%",
        L"후반 · 점수 +22%", L"후반 · 점수 +24%",
        L"후반 · 점수 +21%", L"",
        L"",                    L""
    };
    static const wchar_t* kTrialDetail[TRIAL_DEF_COUNT] = {
        L"Hostile movement speed is increased by 20 percent from the start of the run.",
        L"Your maximum HP is reduced by 25 percent. Recovery cannot restore the missing capacity.",
        L"",
        L"",
        L"Augment choices are limited to two cards whenever a selection is generated.",
        L"All skill cooldowns are increased by 25 percent. Timing becomes part of the build.",
        L"All enemies gain 30 percent HP, making sustained damage and target priority matter more.",
        L"Enemy speed and HP both rise by 20 percent. The whole arena becomes less forgiving.",
        L"Ranged enemies begin appearing earlier than normal, before the build is fully online.",
        L"The early normal spawn pressure is increased, compressing the opening economy window.",
        L"Starting maximum HP is reduced. The first few rooms become the cost of entry.",
        L"Midgame enemy pressure rises by 8 percent through spawn rate and HP.",
        L"",
        L"The midgame ranged enemy cap is raised, creating more simultaneous firing lanes.",
        L"The late-game spawn ramp is strengthened. Pressure keeps climbing after the build stabilizes.",
        L"Late enemies receive a stronger HP ramp, so damage scaling must keep pace.",
        L"Late ranged pressure is increased, reducing the value of standing still or holding one lane.",
        L"", L"", L""
    };
    static const wchar_t* kTrialDetailKR[TRIAL_DEF_COUNT] = {
        L"플레이 시작부터 적의 이동 속도가 20퍼센트 증가합니다.",
        L"최대 체력이 25퍼센트 감소합니다. 회복으로 잃은 최대치를 되돌릴 수 없습니다.",
        L"",
        L"",
        L"증강 선택이 생성될 때 선택지가 2장으로 제한됩니다.",
        L"모든 스킬의 쿨다운이 25퍼센트 증가합니다. 빌드에 타이밍 관리가 필요해집니다.",
        L"모든 적의 체력이 30퍼센트 증가해 지속 피해와 우선 처치가 중요해집니다.",
        L"적의 속도와 체력이 모두 20퍼센트 증가합니다. 전장이 전반적으로 더 가혹해집니다.",
        L"원거리 적이 평소보다 일찍 등장해 빌드가 완성되기 전부터 압박을 줍니다.",
        L"초반 일반 적의 스폰 압력이 증가해 초반 경제를 정비할 시간이 줄어듭니다.",
        L"시작 최대 체력이 감소합니다. 첫 방들이 입장 비용이 됩니다.",
        L"중반 적 스폰 속도와 체력이 8% 증가합니다.",
        L"",
        L"중반 원거리 적 수 제한이 증가해 동시에 유지해야 할 사선이 많아집니다.",
        L"후반 스폰 증가 폭이 커집니다. 빌드가 안정된 뒤에도 압박이 계속 상승합니다.",
        L"후반 적의 체력 증가 폭이 커지므로 피해량 성장도 발맞춰야 합니다.",
        L"후반 원거리 압박이 증가해 한 자리에 머물거나 한 방향만 지키기 어려워집니다.",
        L"", L"", L""
    };
    static const wchar_t* kStageLabels[3] = { L"EARLY", L"MID", L"LATE" };
    static const wchar_t* kStageLabelsKR[3] = { L"초반", L"중반", L"후반" };
    static const wchar_t* kTrialNamesKR[TRIAL_DEF_COUNT] = {
        L"오버클럭", L"메모리 누수", L"", L"",
        L"프로세스 제한", L"낮은 대역폭", L"강화", L"급증",
        L"조기 돌입", L"패킷 폭풍", L"콜드 부트", L"중반 압박",
        L"", L"프로세스 노이즈", L"후반 초과", L"강화 코어",
        L"신호 변위", L"", L"", L""
    };
    static const wchar_t* kWeaponNames[2] = { L"RIFLE", L"FIELD" };
    static const wchar_t* kWeaponNamesKR[2] = { L"소총", L"전기장" };
    static const wchar_t* kWeaponSub[2] = {
        L"PRECISION · SINGLE TARGET", L"AREA CONTROL · SUSTAINED"
    };
    static const wchar_t* kWeaponSubKR[2] = {
        L"정밀 · 단일 대상", L"범위 제어 · 지속"
    };
    static const wchar_t* kStatNames[6] = {
        L"DAMAGE", L"FIRE RATE", L"INTERVAL", L"SPEED", L"SPREAD", L"RANGE"
    };
    static const wchar_t* kStatNamesKR[6] = {
        L"공격력", L"연사 속도", L"간격", L"탄속", L"퍼짐", L"사거리"
    };
    static const wchar_t* kStatValues[2][6] = {
        { L"50", L"5.0/s", L"0.20s", L"1200", L"0.04", L"900" },
        { L"18", L"CONT.", L"0.08s", L"320", L"0.15", L"180" }
    };
    static const wchar_t* kStatValuesKR[2][6] = {
        { L"50", L"5.0/s", L"0.20초", L"1200", L"0.04", L"900" },
        { L"18", L"연속", L"0.08초", L"320", L"0.15", L"180" }
    };
    auto trialName = [&](int id) -> const wchar_t* {
        return ko ? kTrialNamesKR[id] : TRIAL_DEFS[id].id;
    };
    auto trialCompact = [&](int id) -> const wchar_t* {
        return TRIAL_DEFS[id].desc[ko ? 0 : 1];
    };
    auto trialTag = [&](int id) -> const wchar_t* {
        return ko ? kTrialTagsKR[id] : kTrialTags[id];
    };
    auto trialDetail = [&](int id) -> const wchar_t* {
        return ko ? kTrialDetailKR[id] : kTrialDetail[id];
    };
    auto stageLabel = [&](int stage) -> const wchar_t* {
        return ko ? kStageLabelsKR[stage] : kStageLabels[stage];
    };
    auto weaponName = [&](int id) -> const wchar_t* {
        return ko ? kWeaponNamesKR[id] : kWeaponNames[id];
    };
    auto weaponSub = [&](int id) -> const wchar_t* {
        return ko ? kWeaponSubKR[id] : kWeaponSub[id];
    };
    auto statName = [&](int id) -> const wchar_t* {
        return ko ? kStatNamesKR[id] : kStatNames[id];
    };
    auto statValue = [&](int weaponId, int statId) -> const wchar_t* {
        return ko ? kStatValuesKR[weaponId][statId] : kStatValues[weaponId][statId];
    };

    auto countEnabled = [&]() {
        int count = 0;
        for (int i = 0; i < TRIAL_DEF_COUNT; ++i)
            if (!TrialDefRetired(i) && s_RcTrialEnabled[i]) ++count;
        return count;
    };
    auto stageIndex = [&](int defId) {
        switch (TrialStageForDef(defId)) {
        case TrialStage::EARLY: return 0;
        case TrialStage::MID:   return 1;
        case TrialStage::LATE:  return 2;
        }
        return 0;
    };
    auto syncTrialsToGame = [&]() {
        int slot = 0;
        for (int id = 0; id < TRIAL_DEF_COUNT; ++id) {
            if (TrialDefRetired(id) || !s_RcTrialEnabled[id]
                || slot >= TRIAL_SLOT_COUNT) continue;
            g_TrialPool[slot] = id;
            g_TrialSelected[slot] = true;
            ++slot;
        }
        for (; slot < TRIAL_SLOT_COUNT; ++slot)
            g_TrialSelected[slot] = false;
        g_TrialPoolReady = true;
    };

    if (!s_RcTrialStateLoaded && !exiting) {
        weapon = std::max(0, std::min(1, s_RcWeapon));
        trialFocus = 0;
        trialSelectedSlot = 0;
        trialLayoutPending = true;
        trialDisplaySlot = 0.0f;
        trialTargetSlot = 0.0f;
        trialNavHoldDir = 0;
        trialNavHoldT = 0.0f;
        trialNavRepeatT = 0.0f;
        trialDragging = false;
        trialDragMoved = false;
        trialDragLastY = 0.0f;
        trialDragLastX = 0.0f;
        trialPressedId = -1;
        detailScroll = 0.0f;
        detailScrollTarget = 0.0f;
        detailDragging = false;
        detailDragMoved = false;
        detailDragLastY = 0.0f;
        for (int i = 0; i < 2; ++i) weaponHover[i] = 0.0f;
        for (int i = 0; i < TRIAL_DEF_COUNT; ++i) trialHover[i] = 0.0f;
        backHover = playHover = 0.0f;
        resetHover = 0.0f;
        if (!s_RcTrialStateLoaded) {
            for (int i = 0; i < TRIAL_DEF_COUNT; ++i) s_RcTrialEnabled[i] = false;
            for (int slot = 0; slot < TRIAL_SLOT_COUNT; ++slot) {
                const int id = g_TrialPool[slot];
                if (g_TrialSelected[slot] && id >= 0 && id < TRIAL_DEF_COUNT
                    && !TrialDefRetired(id))
                    s_RcTrialEnabled[id] = true;
            }
            s_RcTrialStateLoaded = true;
        }
        // A retired trial may still exist in an older saved selection.
        for (int id = 0; id < TRIAL_DEF_COUNT; ++id)
            if (TrialDefRetired(id)) s_RcTrialEnabled[id] = false;
        for (int id = 0; id < TRIAL_DEF_COUNT; ++id) {
            if (!TrialDefRetired(id) && s_RcTrialEnabled[id]) {
                trialFocus = id;
                break;
            }
        }
        for (int i = 0; i < kTrialCatalogCount; ++i) {
            if (kTrialOrder[i] == trialFocus) {
                trialSelectedSlot = i;
                break;
            }
        }
        trialDisplaySlot = trialTargetSlot = (float)trialSelectedSlot;
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
    const float footerH = 42.0f * uiS;
    const float bodyTop = headerY + 76.0f * uiS;
    const float bodyBottom = footerY - 26.0f * uiS;
    const float detailW = std::min(286.0f * uiS, pageW * 0.23f);
    // PLAY uses a denser information layout than the other menu pages, so
    // keep its text comfortably above the small renderer's minimum scale and
    // give each catalogue row a clear title/effect/tag hierarchy.
    const float playMetaScale = UiTextScale(g_TextS, UiTextLevel::Supporting, uiS);
    const float playSubtitleScale = UiTextScale(g_TextS, UiTextLevel::Subtitle, uiS);
    const float playBodyScale = UiTextScale(g_TextS, UiTextLevel::Description, uiS);
    const float trialTitleScale = UiTextScale(g_TextS, UiTextLevel::Title, uiS);
    const float trialEffectScale = UiTextScale(g_TextS, UiTextLevel::Description, uiS);
    const float trialTagScale = UiTextScale(g_TextS, UiTextLevel::Supporting, uiS);
    const float trialListShiftLeft = 54.0f * uiS;
    const float trialListX = pageR - std::max(330.0f * uiS, pageW * 0.245f)
                           - trialListShiftLeft;
    const float trialListRight = pageR - 6.0f * uiS - trialListShiftLeft;
    const float centerL = pageL + detailW + 34.0f * uiS;
    const float centerR = trialListX - 38.0f * uiS;
    const float centerW = std::max(330.0f * uiS, centerR - centerL);
    const float chartCX = centerL + centerW * 0.50f;
    const float chartCY = bodyTop + (bodyBottom - bodyTop) * 0.44f;
    const float chartR = std::min(244.0f * uiS,
        std::min(centerW * 0.34f, (bodyBottom - bodyTop) * 0.29f));
    const float trialListW = std::max(230.0f * uiS, trialListRight - trialListX);
    // Keep all three bottom commands on one shared baseline. The trial reset
    // uses the catalogue's left X anchor, so it reads as the action belonging
    // to the list instead of another header control.
    const float routeX = BottomLeftActionX(uiS);
    const float routeW = 286.0f * uiS;
    const float routeH = 64.0f * uiS;
    const float routeY = BottomLeftActionY(sh, routeH, uiS);
    const float playW = 270.0f * uiS;
    const float playH = 62.0f * uiS;
    const float playX = chartCX - playW * 0.5f;
    const float playY = routeY - 1.0f * uiS;
    const wchar_t* resetLabel = PlayText(L"시련 초기화", L"TRIAL RESET", L"試練をリセット");
    const float resetW = std::min(230.0f * uiS, trialListW);
    const float resetX = trialListRight - resetW;
    const float resetY = routeY;
    const float now = (float)glfwGetTime();
    const float weaponR = weapon == 0 ? 0.28f : 1.00f;
    const float weaponG = weapon == 0 ? 0.90f : 0.72f;
    const float weaponB = weapon == 0 ? 1.00f : 0.24f;
    const float trialR = 1.00f, trialG = 0.60f, trialB = 0.22f;
    const int activeCount = countEnabled();

    for (int i = 0; i < 6; ++i)
        statT[i] = UiApproach(statT[i], kStatTargets[weapon][i], dt, 8.0f);

    // The trial catalogue is a finite, top-anchored list. Its scroll position
    // is the first visible row, while keyboard focus moves independently.
    const float trialRowStep = 108.0f * uiS;
    // The visual row is shorter than the row step. Let adjacent rows'
    // hit regions meet at their centers' midpoint so there is no dead strip
    // between two trial entries.
    const float trialRowHitHalf = trialRowStep * 0.50f;
    const float trialViewTop = bodyTop + 58.0f * uiS;
    // Stop the catalogue above the shared bottom action rail. This leaves the
    // third command visibly owned by the list and prevents rows from entering
    // the BACK / PLAY / TRIAL RESET area.
    const float trialViewBottom = std::max(trialViewTop,
                                           resetY - 18.0f * uiS);
    const float trialViewH = std::max(0.0f, trialViewBottom - trialViewTop);
    const float trialDiagonal = std::min(72.0f * uiS, trialListW * 0.18f);
    const float trialRailTopX = trialListX - 4.0f * uiS;
    const float trialRailLeft = trialRailTopX - 30.0f * uiS;
    const int trialVisibleRows = std::max(1,
        (int)floorf(trialViewH / trialRowStep));
    const float trialMaxScroll = std::max(0.0f,
        std::min((float)(kTrialCatalogCount - 1),
                 (float)kTrialCatalogCount - trialViewH / trialRowStep));
    const float trialFirstRowCenter = trialViewTop + trialRowStep * 0.5f;
    auto trialRailXAt = [&](float y) {
        const float t = std::max(0.0f,
            std::min(1.0f, (y - trialViewTop) / std::max(1.0f, trialViewH)));
        return trialRailTopX + trialDiagonal * t;
    };
    // Clamp the list's first visible row to the catalogue. The prior centered
    // layout put its first item halfway down the panel and wasted the space above.
    auto normalizeTrialSlots = [&]() {
        trialTargetSlot = std::max(0.0f, std::min(trialMaxScroll, trialTargetSlot));
        trialDisplaySlot = std::max(0.0f, std::min(trialMaxScroll, trialDisplaySlot));
    };
    if (trialLayoutPending) {
        const float activeStart = activeCount > 0
            ? std::max(0.0f, (float)trialSelectedSlot - trialVisibleRows * 0.5f)
            : 0.0f;
        trialTargetSlot = trialDisplaySlot = activeStart;
        normalizeTrialSlots();
        trialLayoutPending = false;
    }
    const bool resetHov = ready
        && mx >= resetX - 22.0f * uiS
        && mx <= resetX + resetW + 22.0f * uiS
        && my >= resetY - 8.0f * uiS
        && my <= resetY + routeH + 10.0f * uiS;
    resetHover = UpdateMenuCommandHover(resetHover, resetHov, dt);
    if (resetHov && lmbClick) {
        for (int id = 0; id < TRIAL_DEF_COUNT; ++id)
            s_RcTrialEnabled[id] = false;
        trialFocus = 0;
        trialSelectedSlot = 0;
        trialTargetSlot = trialDisplaySlot = 0.0f;
        detailScroll = 0.0f;
        detailScrollTarget = 0.0f;
        detailDragging = false;
        detailDragMoved = false;
    }
    const float detailX = pageL + 8.0f * uiS;
    const float detailTop = bodyTop + 150.0f * uiS;
    const float detailViewTop = detailTop + 92.0f * uiS;
    // Leave a clear dead zone above the shared BACK/PLAY action rail. The
    // active-trial copy can scroll, but it must never appear underneath the
    // bottom commands.
    const float detailViewBottom = std::min(bodyBottom - 34.0f * uiS,
                                            footerY - 150.0f * uiS);
    const bool overTrialList = ready && mx >= trialRailLeft - 26.0f * uiS
        && mx <= trialListRight + 8.0f * uiS && my >= trialViewTop && my <= trialViewBottom;
    const bool overDetail = ready && mx >= detailX - 16.0f * uiS
        && mx <= detailX + detailW + 16.0f * uiS
        && my >= detailViewTop && my <= detailViewBottom;

    // The explanation column is a real scroll surface as well as a readable
    // list.  Keep the press/drag state separate from the trial catalogue so
    // grabbing the copy never toggles a trial by accident.
    if (lmbClick && overDetail && !trialDragging) {
        detailDragging = true;
        detailDragMoved = false;
        detailDragLastY = (float)my;
        detailScrollTarget = detailScroll;
    }
    if (!lmb && detailDragging) {
        detailDragging = false;
        detailDragMoved = false;
    }
    if (detailDragging && lmb) {
        const float dy = (float)my - detailDragLastY;
        const float dragThreshold = 3.5f * uiS;
        if (!detailDragMoved && fabsf(dy) >= dragThreshold)
            detailDragMoved = true;
        if (detailDragMoved) {
            detailScrollTarget -= dy;
            detailDragLastY = (float)my;
        }
    }

    const bool keyUpNow = c.window &&
        (glfwGetKey(c.window, GLFW_KEY_UP) == GLFW_PRESS ||
         glfwGetKey(c.window, GLFW_KEY_W) == GLFW_PRESS);
    const bool keyDownNow = c.window &&
        (glfwGetKey(c.window, GLFW_KEY_DOWN) == GLFW_PRESS ||
         glfwGetKey(c.window, GLFW_KEY_S) == GLFW_PRESS);
    int trialStepRequest = 0;
    if (!lmb && trialDragging) {
        // A press/release without a meaningful pointer movement is the
        // toggle gesture. Dragging the same row never reaches this branch.
        if (!trialDragMoved && trialPressedId >= 0
            && trialPressedId < TRIAL_DEF_COUNT) {
            const bool wasOn = s_RcTrialEnabled[trialPressedId];
            if (!wasOn && countEnabled() >= TRIAL_SLOT_COUNT) {
            } else {
                s_RcTrialEnabled[trialPressedId] = !wasOn;
                detailScroll = 0.0f;
                detailScrollTarget = 0.0f;
            }
        }
        trialDragging = false;
        trialDragMoved = false;
        trialPressedId = -1;
    }
    if (trialDragging && lmb) {
        const float dx = (float)mx - trialDragLastX;
        const float dy = (float)my - trialDragLastY;
        const float dragThreshold = 3.5f * uiS;
        if (!trialDragMoved && dx * dx + dy * dy
            >= dragThreshold * dragThreshold)
            trialDragMoved = true;
        if (trialDragMoved && fabsf(dy) > 0.0001f) {
            // Keep the press point until the gesture is confirmed. That way
            // the few pixels used to cross the threshold are not discarded,
            // which makes the rail feel responsive instead of sticky.
            const float dragDelta = -(dy / trialRowStep);
            // Move the display directly with the pointer and clamp it to the
            // first/last catalogue entry.
            trialTargetSlot += dragDelta;
            trialDisplaySlot += dragDelta;
            normalizeTrialSlots();
            trialDragMoved = true;
            const float pointerRow = trialDisplaySlot
                + (((float)my - trialFirstRowCenter) / trialRowStep);
            int nearestSlot = (int)std::round(pointerRow);
            nearestSlot = std::max(0, std::min(kTrialCatalogCount - 1,
                                                nearestSlot));
            if (nearestSlot != trialSelectedSlot) {
                trialSelectedSlot = nearestSlot;
                trialFocus = kTrialOrder[trialSelectedSlot];
            }
            normalizeTrialSlots();
        }
        if (trialDragMoved) {
            trialDragLastY = (float)my;
            trialDragLastX = (float)mx;
        }
    }
    const int trialNavDir = keyUpNow == keyDownNow
        ? 0 : (keyUpNow ? -1 : 1);
    if (c.inputFocusChanged) {
        trialNavHoldDir = trialNavDir;
        trialNavHoldT = 0.0f;
        trialNavRepeatT = 0.0f;
    }
    if (!trialDragging && ready && trialNavDir != 0) {
        if (trialNavDir != trialNavHoldDir) {
            trialNavHoldDir = trialNavDir;
            trialNavHoldT = 0.0f;
            trialNavRepeatT = 0.0f;
            trialStepRequest = trialNavDir;
        } else {
            trialNavHoldT += dt;
            if (trialNavHoldT >= 0.25f) {
                trialNavRepeatT += dt;
                const float repeatInterval = std::max(0.055f,
                    0.24f - (trialNavHoldT - 0.25f) * 0.070f);
                if (trialNavRepeatT >= repeatInterval) {
                    trialNavRepeatT = 0.0f;
                    trialStepRequest = trialNavDir;
                }
            }
        }
    } else {
        trialNavHoldDir = 0;
        trialNavHoldT = 0.0f;
        trialNavRepeatT = 0.0f;
    }
    if (g_ScrollAccum != 0.0f) {
        if (!trialDragging && overTrialList) {
            trialTargetSlot -= g_ScrollAccum;
            normalizeTrialSlots();
        } else if (overDetail) {
            detailScrollTarget -= g_ScrollAccum * 34.0f * uiS;
        }
        g_ScrollAccum = 0.0f;
    }
    if (trialStepRequest != 0) {
        trialSelectedSlot = std::max(0, std::min(kTrialCatalogCount - 1,
                            trialSelectedSlot + trialStepRequest));
        trialFocus = kTrialOrder[trialSelectedSlot];
        if ((float)trialSelectedSlot < trialTargetSlot)
            trialTargetSlot = (float)trialSelectedSlot;
        else if ((float)trialSelectedSlot >= trialTargetSlot + trialVisibleRows)
            trialTargetSlot = (float)trialSelectedSlot - trialVisibleRows + 1.0f;
        normalizeTrialSlots();
    }
    {
        const float diff = trialTargetSlot - trialDisplaySlot;
        trialDisplaySlot += diff * std::min(1.0f, dt * 10.0f);
        if (fabsf(diff) < 0.002f)
            trialDisplaySlot = trialTargetSlot;
    }
    // PLAY shares the same wide, mirrored side fields as Settings. The
    // transparent scene remains visible through the center while the edges
    // receive a consistent dark contrast treatment.
    BatchFlush();
    DrawPersistentSceneSideVignettes(sw, sh,
                                     kWideSceneLinearAlpha * contentA);
    // The radial texture is the hero's light source: a colored outer corona
    // makes the constellation read as a focal object, while a small dark core
    // keeps the weapon title and node geometry crisp over bright backdrops.
    DrawRadialGradient(chartCX, chartCY, chartR * 1.88f,
                       weaponR * 0.56f, weaponG * 0.56f, weaponB * 0.56f,
                       0.34f * contentA);
    DrawRadialGradient(chartCX, chartCY, chartR * 1.08f,
                       0.0f, 0.0f, 0.008f,
                       0.26f * contentA);
    // CircleTexture carries the constellation's depth, while the broad edge
    // linear fields and hero radial fields provide the page contrast.
    DrawConstellationDisc(chartCX, chartCY, chartR * 1.74f,
                          0.0f, 0.0f, 0.012f, 0.28f * contentA);
    DrawConstellationDisc(chartCX, chartCY, chartR * 1.34f,
                          weaponR * 0.08f, weaponG * 0.08f, weaponB * 0.10f,
                          (0.10f + 0.025f * sinf(now * 1.2f)) * contentA, false);
    DrawConstellationDisc(trialListX + trialListW * 0.62f,
                          (trialViewTop + trialViewBottom) * 0.52f,
                          trialListW * 0.72f, 0.0f, 0.0f, 0.012f,
                          0.38f * contentA);

    // XL glyphs are baked at ~133px; shrinking them to title size with
    // plain bilinear sampling left stair-stepped edges (QA #16).
    const float playTitleScale = UiTextScale(g_TextL, UiTextLevel::Title, uiS);
    DrawShadowedText(g_TextL, PlayText(L"플레이", L"PLAY"), pageL, headerY,
                     playTitleScale, 1.0f, 1.0f, 1.0f,
                     0.98f * entryHeader, 0.74f);
    const float escW = g_TextS.Width(L"[ESC]", playMetaScale);
    DrawShadowedText(g_TextS, L"[ESC]", pageR - escW, headerY + 8.0f * uiS,
                     playMetaScale, 0.68f, 0.78f, 0.88f,
                     0.72f * entryHeader, 0.54f);
    LogoLine(pageL, headerY + 58.0f * uiS, pageR, headerY + 58.0f * uiS,
             0.7f * uiS, weaponR, weaponG, weaponB, 0.12f * contentA);

    // Weapon choice is a small editorial tab in the upper-left. This is the
    // only weapon selection surface; the bottom rail is reserved for BACK.
    const float tabsY = bodyTop + 12.0f * uiS;
    const float tabScale = UiTextScale(g_TextL, UiTextLevel::Title, uiS);
    float tabX[2] = { detailX, detailX + 170.0f * uiS };
    for (int i = 0; i < 2; ++i) {
        const float tabW = 150.0f * uiS;
        const float textW = g_TextL.Width(weaponName(i), tabScale);
        const float textH = g_TextL.Height(weaponName(i), tabScale);
        const float tabFit = std::min(1.0f, std::min(
            (tabW - 16.0f * uiS) / std::max(1.0f, textW),
            (46.0f * uiS - 8.0f * uiS) / std::max(1.0f, textH)));
        const float fittedTabScale = tabScale * std::max(0.0f, tabFit);
        const float fittedTextW = g_TextL.Width(weaponName(i), fittedTabScale);
        const float fittedTextH = g_TextL.Height(weaponName(i), fittedTabScale);
        const bool hov = ready && PanelButtonHit(mx, my, tabX[i], tabsY - 4.0f * uiS,
                                                  tabW, 46.0f * uiS, 4.0f * uiS);
        weaponHover[i] = UpdateMenuCommandHover(weaponHover[i], hov, dt);
        const bool selected = weapon == i;
        DrawShadowedText(g_TextL, weaponName(i),
                         tabX[i] + (tabW - fittedTextW) * 0.5f,
                         tabsY + (46.0f * uiS - fittedTextH) * 0.5f,
                         fittedTabScale,
                         selected ? weaponR : 0.58f,
                         selected ? weaponG : 0.68f,
                         selected ? weaponB : 0.78f,
                         (selected ? 0.96f : 0.64f + 0.18f * weaponHover[i]) * contentA,
                         0.50f);
        if (hov && lmbClick) { weapon = i; s_RcWeapon = weapon; }
    }
    LogoLine(detailX, tabsY + 48.0f * uiS, detailX + detailW,
             tabsY + 48.0f * uiS, 0.8f * uiS,
             weaponR, weaponG, weaponB, 0.18f * contentA);

    // Hero copy is attached to the constellation, keeping the selected
    // weapon readable before the player reads either side column.
    const float heroNameY = chartCY - chartR - 58.0f * uiS;
    const float heroNameScale = UiTextScale(g_TextL, UiTextLevel::Title, uiS);
    const float heroNameW = g_TextL.Width(weaponName(weapon), heroNameScale);
    const wchar_t* heroSystemLabel = PlayText(L"무기 별자리", L"WEAPON CONSTELLATION");
    DrawShadowedText(g_TextS, heroSystemLabel,
                     chartCX - g_TextS.Width(heroSystemLabel, playSubtitleScale) * 0.5f,
                     heroNameY - 42.0f * uiS, playSubtitleScale,
                     weaponR, weaponG, weaponB, 0.78f * contentA, 0.52f);
    DrawShadowedText(g_TextL, weaponName(weapon), chartCX - heroNameW * 0.5f,
                     heroNameY, heroNameScale,
                     1.0f, 1.0f, 1.0f, 0.96f * contentA, 0.72f);
    const float subW = g_TextS.Width(weaponSub(weapon), playSubtitleScale);
    DrawShadowedText(g_TextS, weaponSub(weapon), chartCX - subW * 0.5f,
                     heroNameY + g_TextL.Height(weaponName(weapon), heroNameScale)
                                 + 8.0f * uiS, playSubtitleScale,
                     weaponR, weaponG, weaponB, 0.76f * contentA, 0.50f);

    // A shallow perspective network gives the hero a dimensional read. The
    // nodes are depth-scaled and the ellipses rotate at different speeds, so
    // RIFLE and FIELD share the language but retain distinct silhouettes.
    auto drawHeroNetwork = [&]() {
        constexpr int kNodeCount = 18;
        float px[kNodeCount] = {}, py[kNodeCount] = {}, depth[kNodeCount] = {};
        const float spin = now * (weapon == 0 ? 0.045f : -0.032f);
        const float yScale = weapon == 0 ? 0.70f : 0.52f;
        for (int i = 0; i < kNodeCount; ++i) {
            const float a = spin + 6.2831853f * (float)i / (float)kNodeCount;
            const float wave = sinf(now * 0.34f + i * 1.73f);
            const float ring = 0.64f + 0.24f * (0.5f + 0.5f * sinf(i * 2.11f + 0.7f));
            depth[i] = wave;
            px[i] = chartCX + cosf(a) * chartR * ring;
            py[i] = chartCY + sinf(a) * chartR * ring * yScale
                   + wave * chartR * 0.055f;
        }
        for (int ring = 0; ring < 3; ++ring) {
            const float rx = chartR * (0.55f + ring * 0.18f);
            const float ry = chartR * yScale * (0.36f + ring * 0.16f);
            const float phase = spin * (ring == 1 ? -0.8f : 0.55f)
                              + (ring == 2 ? 0.22f : 0.0f);
            float lastX = chartCX + cosf(phase) * rx;
            float lastY = chartCY + sinf(phase) * ry;
            for (int n = 1; n <= 32; ++n) {
                const float a = phase + 6.2831853f * (float)n / 32.0f;
                const float x = chartCX + cosf(a) * rx;
                const float y = chartCY + sinf(a) * ry;
                if ((n + ring) % (ring == 2 ? 4 : 3) != 0)
                    DrawVisibleConstellLine(lastX, lastY, x, y,
                                            (0.52f + ring * 0.10f) * uiS,
                                            weaponR, weaponG, weaponB,
                                            (0.08f + ring * 0.025f) * contentA);
                lastX = x; lastY = y;
            }
        }
        for (int i = 0; i < kNodeCount; ++i) {
            const int next = (i + 1) % kNodeCount;
            const int cross = (i + 5 + weapon) % kNodeCount;
            const float edgeA = (0.15f + 0.12f * (depth[i] + 1.0f) * 0.5f) * contentA;
            DrawVisibleConstellLine(px[i], py[i], px[next], py[next],
                                    0.78f * uiS, weaponR, weaponG, weaponB, edgeA);
            if ((i + weapon) % 2 == 0)
                DrawVisibleConstellLine(px[i], py[i], px[cross], py[cross],
                                        0.56f * uiS, weaponR, weaponG, weaponB,
                                        edgeA * 0.62f);
            const float nodeSize = (2.3f + 1.7f * (depth[i] + 1.0f) * 0.5f) * uiS;
            if (depth[i] > 0.12f)
                DrawConstellationDisc(px[i], py[i], nodeSize * 2.2f,
                                      weaponR, weaponG, weaponB, 0.08f * contentA);
            DrawVisibleConstellNode(px[i], py[i], nodeSize,
                                    weaponR, weaponG, weaponB,
                                    (0.42f + 0.24f * (depth[i] + 1.0f) * 0.5f) * contentA,
                                    false, depth[i] > -0.35f);
        }
        // FIELD keeps its wider corona. RIFLE uses only the constellation
        // network; the former targeting spine read as unrelated stray lines.
        if (weapon != 0) {
            for (int i = 0; i < 8; ++i) {
                const float a = now * -0.08f + 6.2831853f * i / 8.0f;
                DrawVisibleConstellLine(chartCX, chartCY,
                                        chartCX + cosf(a) * chartR * 0.72f,
                                        chartCY + sinf(a) * chartR * 0.72f * yScale,
                                        0.62f * uiS, weaponR, weaponG, weaponB,
                                        0.18f * contentA);
            }
        }
        for (int id = 0; id < TRIAL_DEF_COUNT; ++id) {
            if (TrialDefRetired(id) || !s_RcTrialEnabled[id]) continue;
            const int node = (id * 7 + weapon * 3) % kNodeCount;
            DrawVisibleConstellLine(chartCX, chartCY, px[node], py[node],
                                    1.10f * uiS, trialR, trialG, trialB,
                                    0.24f * contentA);
            DrawConstellationDisc(px[node], py[node], 13.0f * uiS,
                                  trialR, trialG, trialB, 0.10f * contentA);
            DrawVisibleConstellNode(px[node], py[node], 4.2f * uiS,
                                    trialR, trialG, trialB, 0.86f * contentA,
                                    true, true);
        }
        const float pulse = 0.5f + 0.5f * sinf(now * (weapon == 0 ? 5.6f : 3.0f));
        DrawConstellationDisc(chartCX, chartCY, (32.0f + pulse * 8.0f) * uiS,
                              0.0f, 0.0f, 0.012f, 0.36f * contentA);
        DrawConstellationDisc(chartCX, chartCY, (17.0f + pulse * 4.0f) * uiS,
                              weaponR, weaponG, weaponB, 0.18f * contentA, false);
        drawDiamond(chartCX, chartCY, (9.0f + pulse * 2.0f) * uiS,
                    weaponR, weaponG, weaponB, 0.86f * contentA);
        drawDiamond(chartCX, chartCY, 3.8f * uiS,
                    0.96f, 1.0f, 1.0f, 0.94f * contentA);
    };
    drawHeroNetwork();

    // Compact telemetry stays under the hero as a 3 x 2 readout. The larger
    // type makes the values scannable without creating a second information
    // panel beside the weapon hero.
    const float statY = chartCY + chartR + 26.0f * uiS;
    const float statRowStep = 60.0f * uiS;
    const float statColumnGap = 30.0f * uiS;
    const float statLabelScale = UiTextScale(g_TextS, UiTextLevel::Description, uiS);
    const float statValueScale = UiTextScale(g_TextS, UiTextLevel::Description, uiS);
    const float statW = std::max(96.0f * uiS,
        (centerW - statColumnGap * 2.0f) / 3.0f);
    for (int i = 0; i < 6; ++i) {
        const int col = i % 3;
        const int row = i / 3;
        const float x = centerL + col * (statW + statColumnGap);
        const float y = statY + row * statRowStep;
        const float valueW = g_TextS.Width(statValue(weapon, i), statValueScale);
        const float textInset = 4.0f * uiS;
        // The track follows the same left/right bounds as the metric text:
        // label starts at barX and the value ends at barRight.
        const float barX = x + textInset;
        const float barRight = x + statW - textInset;
        DrawShadowedText(g_TextS, statName(i), x + textInset, y,
                         statLabelScale, 0.72f, 0.82f, 0.92f,
                         0.78f * contentA, 0.46f);
        DrawShadowedText(g_TextS, statValue(weapon, i),
                         x + statW - valueW - textInset, y,
                         statValueScale, 0.92f, 0.96f, 1.0f,
                         0.90f * contentA, 0.52f);
        // Give the telemetry a little air: the track is inset and sits below
        // the labels instead of competing with the enlarged text.
        LogoLine(barX, y + 31.0f * uiS, barRight,
                 y + 31.0f * uiS, 0.95f * uiS,
                 0.34f, 0.44f, 0.56f, 0.22f * contentA);
        LogoLine(barX, y + 31.0f * uiS,
                 barX + (barRight - barX) * LogoClamp01(statT[i]),
                 y + 31.0f * uiS, 1.75f * uiS,
                 weaponR, weaponG, weaponB, 0.70f * contentA);
    }

    // Left active-trial readout: this is intentionally independent of the
    // hovered catalogue row. It reports every enabled trial, while the
    // catalogue remains free to browse without rewriting the explanation.
    const wchar_t* activeStatusText = activeCount > 0
        ? PlayText(L"활성 시련", L"ACTIVE TRIALS")
        : PlayText(L"활성 시련 없음", L"NO ACTIVE TRIALS");
    float activeStatusScale = playSubtitleScale;
    const float activeStatusW = g_TextS.Width(activeStatusText, activeStatusScale);
    if (activeStatusW > detailW && activeStatusW > 0.0f)
        activeStatusScale *= detailW / activeStatusW;
    const float activeStatusH = g_TextS.Height(activeStatusText, activeStatusScale);
    DrawShadowedText(g_TextS, activeStatusText,
                     detailX, detailTop, activeStatusScale,
                     trialR, trialG, trialB, 0.82f * contentA, 0.52f);
    wchar_t activeReadout[32];
    swprintf_s(activeReadout, ko ? L"%d / %d 활성화" : L"%d / %d ENABLED",
               activeCount, TRIAL_SLOT_COUNT);
    const float activeReadoutScale = UiTextScale(g_TextS, UiTextLevel::Description, uiS);
    DrawShadowedText(g_TextS, activeReadout,
                     detailX,
                     detailTop + activeStatusH + 7.0f * uiS, activeReadoutScale,
                     0.64f, 0.74f, 0.84f, 0.72f * contentA, 0.42f);
    const float activeReadoutH = g_TextS.Height(activeReadout, activeReadoutScale);
    const float activeRuleY = detailTop + activeStatusH + 7.0f * uiS
                            + activeReadoutH + 8.0f * uiS;
    LogoLine(detailX, activeRuleY,
             detailX + detailW, activeRuleY,
             0.75f * uiS, trialR, trialG, trialB, 0.26f * contentA);
    const wchar_t* detailHint = PlayText(L"드래그 이동 · 휠 스크롤",
                                         L"DRAG · WHEEL SCROLL",
                                         L"ドラッグ · ホイール");
    // The hint shares the readout row, right-aligned above the rule. Its old
    // fixed spot 12px above the scroll view overlapped the first heading.
    // The renderer clamps tiny scales, so a long hint (JP) may not shrink
    // enough; it then moves to the status row, and is omitted if neither
    // row has room rather than overlapping either label.
    const float detailHintGap = 16.0f * uiS;
    // Shrink toward the room left of the row's label; < 0 means no fit even
    // after the renderer's minimum-scale clamp.
    auto fitHintBeside = [&](float labelW) {
        const float room = detailW - labelW - detailHintGap;
        const float fullW = g_TextS.Width(detailHint, playMetaScale);
        const float scale = fullW > room && fullW > 0.0f
            ? playMetaScale * std::max(0.0f, room) / fullW : playMetaScale;
        return g_TextS.Width(detailHint, scale) <= room ? scale : -1.0f;
    };
    const float readoutHintScale = fitHintBeside(
        g_TextS.Width(activeReadout, activeReadoutScale));
    const float statusHintScale = readoutHintScale > 0.0f ? -1.0f
        : fitHintBeside(g_TextS.Width(activeStatusText, activeStatusScale));
    const bool hintOnReadout = readoutHintScale > 0.0f;
    const bool hintOnStatus = statusHintScale > 0.0f;
    if (hintOnReadout || hintOnStatus) {
        const float detailHintScale = hintOnReadout ? readoutHintScale
                                                    : statusHintScale;
        const float detailHintW = g_TextS.Width(detailHint, detailHintScale);
        const float rowBottom = hintOnReadout
            ? activeRuleY - 4.0f * uiS
            : detailTop + activeStatusH;
        DrawShadowedText(g_TextS, detailHint,
                         detailX + detailW - detailHintW,
                         rowBottom - g_TextS.Height(detailHint, detailHintScale),
                         detailHintScale,
                         0.42f, 0.70f, 0.80f, 0.68f * contentA, 0.40f);
    }
    // Give the active build an objective, compact readout before the prose
    // descriptions.  These values mirror the multipliers used by the run
    // code; conditional late-game effects are shown as start -> peak rather
    // than pretending they are active from the first room.
    struct ActiveSummaryRow {
        std::wstring label;
        std::wstring value;
        bool buff = false;
    };
    std::vector<ActiveSummaryRow> summaryRows;
    const auto selected = [&](int id) {
        return id >= 0 && id < TRIAL_DEF_COUNT && !TrialDefRetired(id)
            && s_RcTrialEnabled[id];
    };
    auto formatPercent = [](float mult) {
        wchar_t buf[32] = {};
        swprintf_s(buf, L"%+.1f%%", (mult - 1.0f) * 100.0f);
        return std::wstring(buf);
    };
    auto formatPercentRange = [&](float startMult, float peakMult) {
        if (fabsf(startMult - peakMult) < 0.0005f)
            return formatPercent(startMult);
        wchar_t buf[64] = {};
        swprintf_s(buf, L"%+.1f%% → %+.1f%%",
                   (startMult - 1.0f) * 100.0f,
                   (peakMult - 1.0f) * 100.0f);
        return std::wstring(buf);
    };
    auto formatCountRange = [&](int startCount, int peakCount) {
        wchar_t buf[32] = {};
        if (startCount == peakCount)
            swprintf_s(buf, L"+%d", startCount);
        else
            swprintf_s(buf, L"+%d → +%d", startCount, peakCount);
        return std::wstring(buf);
    };
    auto addPercentMetric = [&](const wchar_t* label, float startMult,
                                float peakMult, bool buff) {
        if (fabsf(startMult - 1.0f) < 0.0005f
            && fabsf(peakMult - 1.0f) < 0.0005f)
            return;
        summaryRows.push_back({ label,
                                formatPercentRange(startMult, peakMult),
                                buff });
    };

    float scoreMult = 1.0f;
    for (int id = 0; id < TRIAL_DEF_COUNT; ++id) {
        if (selected(id)) scoreMult += TrialScoreBonusForDef(id);
    }
    if (activeCount > 0) {
        wchar_t scoreBuf[48] = {};
        swprintf_s(scoreBuf, L"x%.2f  (%+.1f%%)", scoreMult,
                   (scoreMult - 1.0f) * 100.0f);
        summaryRows.push_back({ PlayText(L"점수 배율", L"SCORE MULTIPLIER"),
                                scoreBuf, true });
    }

    float playerHpMult = 1.0f;
    if (selected(10)) playerHpMult *= 0.82f;
    addPercentMetric(PlayText(L"플레이어 최대 HP", L"PLAYER MAX HP"),
                     1.0f, playerHpMult, false);

    float enemyHpStart = 1.0f, enemyHpPeak = 1.0f;
    if (selected(11)) enemyHpStart *= 1.08f;
    if (selected(15)) enemyHpPeak *= 1.28f;
    enemyHpPeak *= enemyHpStart;
    addPercentMetric(PlayText(L"일반 적 체력", L"ENEMY HP"),
                     enemyHpStart, enemyHpPeak, false);

    float spawnStart = 1.0f, spawnPeak = 1.0f;
    if (selected(9))  spawnStart *= 1.18f;
    if (selected(11)) spawnStart *= 1.08f;
    spawnPeak = spawnStart;
    if (selected(14)) spawnPeak *= 1.24f;
    if (selected(16)) spawnPeak *= 1.12f;
    addPercentMetric(PlayText(L"일반 스폰 빈도", L"SPAWN FREQUENCY"),
                     spawnStart, spawnPeak, false);

    float rangedStart = 1.0f, rangedPeak = 1.0f;
    if (selected(8))  rangedStart *= 0.78f;
    rangedPeak = rangedStart;
    if (selected(13)) rangedPeak *= 0.86f;
    if (selected(16)) rangedPeak *= 0.82f;
    addPercentMetric(PlayText(L"원거리 스폰 간격", L"RANGED INTERVAL"),
                     rangedStart, rangedPeak, false);

    int rangedMaxStart = 0;
    int rangedMaxPeak = (selected(13) ? 2 : 0)
                      + (selected(16) ? 2 : 0);
    if (rangedMaxStart > 0 || rangedMaxPeak > 0)
        summaryRows.push_back({ PlayText(L"원거리 최대 수", L"RANGED MAX"),
                                formatCountRange(rangedMaxStart, rangedMaxPeak),
                                false });

    const float summaryTitleScale = playSubtitleScale;
    const float summaryTitleH = g_TextS.Height(
        PlayText(L"현재 적용 합계", L"ACTIVE EFFECT TOTAL"), summaryTitleScale)
        + 6.0f * uiS;
    const float summaryValueScale = UiTextScale(g_TextS, UiTextLevel::Description, uiS);
    const float summaryRowH = g_TextS.Height(L"+100.0%", summaryValueScale)
                            + 10.0f * uiS;
    const float summaryBottomGap = 15.0f * uiS;
    const float summaryBlockH = summaryTitleH
                              + std::max<size_t>(1, summaryRows.size()) * summaryRowH
                              + summaryBottomGap;
    struct ActiveDetailLine {
        std::wstring text;
        int kind;
        int tone;
    };
    std::vector<ActiveDetailLine> detailLines;
    auto appendLine = [&](const std::wstring& text, int kind, int tone) {
        detailLines.push_back({ text, kind, tone });
    };
    auto appendWrapped = [&](const std::wstring& text, int kind, int tone) {
        std::wstring line;
        std::wstring word;
        for (size_t i = 0; i <= text.size(); ++i) {
            const bool end = i == text.size();
            const wchar_t ch = end ? L' ' : text[i];
            if (ch != L' ') { word.push_back(ch); continue; }
            std::wstring candidate = line.empty() ? word : line + L" " + word;
            if (!line.empty() && g_TextS.Width(candidate.c_str(), playBodyScale) > detailW) {
                appendLine(line, kind, tone);
                line = word;
            } else if (!word.empty()) {
                line = candidate;
            }
            word.clear();
        }
        if (!line.empty()) appendLine(line, kind, tone);
    };
    if (activeCount <= 0) {
        appendWrapped(PlayText(L"시련 목록에서 시련을 활성화해 플레이 설정을 구성하세요.",
                               L"Enable trials from the catalogue to build the play configuration."),
                      0, 0);
    } else {
        for (int order = 0; order < kTrialCatalogCount; ++order) {
            const int id = kTrialOrder[order];
            if (!s_RcTrialEnabled[id]) continue;
            appendLine(std::wstring(trialName(id)), 1, 0);
            std::wstring stageLine = ko ? L"단계  //  " : L"STAGE  //  ";
            stageLine += stageLabel(stageIndex(id));
            appendLine(stageLine, 2, 0);
            appendLine(PlayText(L"효과", L"EFFECT"), 2, 0);
            appendWrapped(std::wstring(trialDetail(id)), 0, -1);
            appendLine(PlayText(L"간단 요약", L"COMPACT READOUT"), 2, 0);
            appendWrapped(std::wstring(trialCompact(id)), 0, -1);
            appendLine(PlayText(L"보상 · 압박", L"PAYOFF · PRESSURE"), 2, 0);
            appendWrapped(std::wstring(trialTag(id)), 0, 1);
            appendLine(L"", 3, 0);
        }
    }
    const auto detailScale = [&](const ActiveDetailLine& line) {
        if (line.kind == 1) return UiTextScale(g_TextS, UiTextLevel::Title, uiS);
        if (line.kind == 2) return playSubtitleScale;
        if (line.kind == 3) return 0.0f;
        return playBodyScale;
    };
    const auto detailLineHeight = [&](const ActiveDetailLine& line) {
        if (line.kind == 3) return 12.0f * uiS;
        const float textH = g_TextS.Height(line.text.c_str(), detailScale(line));
        return std::max(22.0f * uiS, textH + 4.0f * uiS);
    };
    float detailLinesHeight = 0.0f;
    for (const ActiveDetailLine& line : detailLines)
        detailLinesHeight += detailLineHeight(line);
    const float detailContentH = summaryBlockH + detailLinesHeight;
    const float detailMaxScroll = std::max(0.0f,
        detailContentH - (detailViewBottom - detailViewTop) + 6.0f * uiS);
    detailScrollTarget = std::max(0.0f,
                                  std::min(detailScrollTarget, detailMaxScroll));
    const float detailScrollEase = std::min(1.0f, dt * 14.0f);
    detailScroll += (detailScrollTarget - detailScroll) * detailScrollEase;
    if (fabsf(detailScrollTarget - detailScroll) < 0.08f)
        detailScroll = detailScrollTarget;
    BatchFlush();
    glEnable(GL_SCISSOR_TEST);
    glScissor((GLint)(detailX - 6.0f * uiS),
              (GLint)(sh - detailViewBottom),
              (GLint)(detailW + 18.0f * uiS),
              (GLint)(detailViewBottom - detailViewTop));
    float detailY = detailViewTop - detailScroll;
    DrawShadowedText(g_TextS,
                     PlayText(L"현재 적용 합계", L"ACTIVE EFFECT TOTAL"),
                     detailX, detailY, summaryTitleScale,
                     0.68f, 0.88f, 0.96f, 0.86f * contentA, 0.50f);
    detailY += summaryTitleH;
    if (summaryRows.empty()) {
        DrawShadowedText(g_TextS,
                         PlayText(L"선택된 시련 없음", L"NO ACTIVE MODIFIERS"),
                         detailX, detailY, summaryValueScale,
                         0.72f, 0.78f, 0.84f, 0.72f * contentA, 0.46f);
        detailY += summaryRowH;
    } else {
        for (const ActiveSummaryRow& row : summaryRows) {
            float valueScale = summaryValueScale;
            const float maxValueW = detailW * 0.48f;
            const float fullValueW = g_TextS.Width(row.value.c_str(), valueScale);
            if (fullValueW > maxValueW && fullValueW > 0.0f)
                valueScale *= maxValueW / fullValueW;
            const float valueW = g_TextS.Width(row.value.c_str(), valueScale);
            const float labelMaxW = std::max(20.0f * uiS,
                detailW - valueW - 11.0f * uiS);
            float labelScale = summaryValueScale;
            const float fullLabelW = g_TextS.Width(row.label.c_str(), labelScale);
            if (fullLabelW > labelMaxW && fullLabelW > 0.0f)
                labelScale *= labelMaxW / fullLabelW;
            DrawShadowedText(g_TextS, row.label.c_str(), detailX, detailY,
                             labelScale,
                             0.70f, 0.78f, 0.86f, 0.86f * contentA, 0.44f);
            DrawShadowedText(g_TextS, row.value.c_str(),
                             detailX + detailW - valueW, detailY,
                             valueScale,
                             row.buff ? 0.20f : 1.0f,
                             row.buff ? 0.92f : 0.28f,
                             row.buff ? 1.0f : 0.28f,
                             0.94f * contentA, 0.50f);
            const float summaryTextH = g_TextS.Height(row.value.c_str(), valueScale);
            LogoLine(detailX, detailY + summaryTextH + 4.0f * uiS,
                     detailX + detailW, detailY + summaryTextH + 4.0f * uiS,
                     0.45f * uiS, 0.18f, 0.28f, 0.34f,
                     0.28f * contentA);
            detailY += summaryRowH;
        }
    }
    detailY += summaryBottomGap;
    for (size_t i = 0; i < detailLines.size(); ++i) {
        const ActiveDetailLine& detailLine = detailLines[i];
        if (detailLine.kind == 3 && detailLine.text.empty()) {
            detailY += detailLineHeight(detailLine);
            continue;
        }
        const bool title = detailLine.kind == 1;
        const bool heading = detailLine.kind == 2;
        const bool buff = detailLine.tone > 0;
        const bool nerf = detailLine.tone < 0;
        const float lineScale = detailScale(detailLine);
        DrawShadowedText(g_TextS, detailLine.text.c_str(), detailX, detailY,
                         lineScale,
                         title ? 1.0f : heading ? trialR : buff ? 0.20f : nerf ? 1.0f : 0.80f,
                         title ? 1.0f : heading ? trialG : buff ? 0.92f : nerf ? 0.28f : 0.87f,
                         title ? 1.0f : heading ? trialB : buff ? 1.0f : nerf ? 0.28f : 0.94f,
                         (title ? 0.92f : heading ? 0.72f : 0.86f) * contentA,
                         title ? 0.64f : heading ? 0.50f : 0.54f);
        detailY += detailLineHeight(detailLine);
    }
    BatchFlush();
    glDisable(GL_SCISSOR_TEST);
    if (detailMaxScroll > 0.0f) {
        const float barX = detailX + detailW + 5.0f * uiS;
        const float thumbH = std::max(24.0f * uiS,
            (detailViewBottom - detailViewTop)
            * ((detailViewBottom - detailViewTop) / detailContentH));
        const float thumbY = detailViewTop
            + (detailViewBottom - detailViewTop - thumbH)
            * (detailScroll / detailMaxScroll);
        drawRect(barX, detailViewTop, 2.0f * uiS,
                 detailViewBottom - detailViewTop,
                 0.10f, 0.16f, 0.20f, 0.38f * contentA);
        drawRect(barX, thumbY, 2.0f * uiS, thumbH,
                 trialR, trialG, trialB, 0.66f * contentA);
    }

    // Right codex list: compact rows communicate the decision; the full
    // sentence is reserved for the scrollable detail column on the left.
    const wchar_t* catalogueTitle = PlayText(L"시련 목록", L"TRIAL CATALOGUE", L"試練一覧");
    DrawShadowedText(g_TextS, catalogueTitle, trialListX, bodyTop,
                     playSubtitleScale, trialR, trialG, trialB,
                     1.0f * contentA, 0.56f);
    wchar_t activeLabel[32];
    swprintf_s(activeLabel, PlayText(L"%d / %d 활성", L"%d / %d ACTIVE", L"%d / %d 有効"),
               activeCount, TRIAL_SLOT_COUNT);
    // The counter is part of the header, so it shares the title's font,
    // size and top line instead of reading as a separate larger label.
    // Long localized titles keep a clear gap by shrinking only the counter.
    const float catalogueTitleW = g_TextS.Width(catalogueTitle, playSubtitleScale);
    const float activeRoomW = trialListRight - trialListX - catalogueTitleW
                            - 28.0f * uiS;
    float activeScale = playSubtitleScale;
    float activeW = g_TextS.Width(activeLabel, activeScale);
    if (activeW > activeRoomW && activeW > 1.0f) {
        activeScale *= std::max(0.6f, activeRoomW / activeW);
        activeW = g_TextS.Width(activeLabel, activeScale);
    }
    DrawShadowedText(g_TextS, activeLabel, trialListRight - activeW,
                     bodyTop + g_TextS.BaselineOffset(playSubtitleScale)
                             - g_TextS.BaselineOffset(activeScale),
                     activeScale,
                     trialR, trialG, trialB, 1.0f * contentA, 0.56f);
    const float catalogueRuleY = bodyTop
        + g_TextS.Height(catalogueTitle, playSubtitleScale) + 8.0f * uiS;
    LogoLine(trialListX, catalogueRuleY,
             trialListRight, catalogueRuleY,
             0.8f * uiS, trialR, trialG, trialB, 0.30f * contentA);
    // A quiet cyan linear field follows the catalogue's \ rail. The slanted
    // texture keeps the list's motion language intact without becoming a
    // heavy card behind the readable rows.
    BatchFlush();
    const float trialFieldX = trialListX - 16.0f * uiS;
    const float trialFieldTop = trialViewTop - 14.0f * uiS;
    const float trialFieldW = trialListW + 32.0f * uiS;
    const float trialFieldH = trialViewBottom - trialViewTop + 28.0f * uiS;
    DrawLinearGradientRibbon(trialFieldX, trialFieldTop,
                             trialFieldW, trialFieldH,
                             std::min((trialDiagonal + 18.0f * uiS),
                                      trialFieldW * 0.18f),
                             0.06f, 0.48f, 0.62f,
                             0.075f * contentA, false);
    // The catalogue is a finite diagonal rail. Rows flow from the top edge
    // and scroll as a conventional list instead of orbiting the focused row.
    const float railTopY = trialViewTop + 6.0f * uiS;
    const float railBottomY = trialViewBottom - 6.0f * uiS;
    const float railBottomX = trialRailTopX + trialDiagonal;
    LogoLine(trialRailTopX, railTopY, railBottomX, railBottomY,
             0.78f * uiS, weaponR, weaponG, weaponB,
             0.15f * contentA);
    LogoLine(trialRailTopX + 13.0f * uiS, railTopY,
             railBottomX + 13.0f * uiS, railBottomY,
             0.42f * uiS, weaponR, weaponG, weaponB,
             0.065f * contentA);
    DrawVisibleConstellNode(trialRailTopX, railTopY, 2.2f * uiS,
                            weaponR, weaponG, weaponB, 0.46f * contentA,
                            false, false);
    DrawVisibleConstellNode(railBottomX, railBottomY, 2.2f * uiS,
                            weaponR, weaponG, weaponB, 0.46f * contentA,
                            false, false);

    BatchFlush();
    glEnable(GL_SCISSOR_TEST);
    glScissor((GLint)(trialRailLeft - 26.0f * uiS),
              (GLint)(sh - trialViewBottom),
              (GLint)(trialListRight - trialRailLeft + 44.0f * uiS),
              (GLint)trialViewH);
    for (int slot = 0; slot < kTrialCatalogCount; ++slot) {
        const float relF = (float)slot - trialDisplaySlot;
        const int id = kTrialOrder[slot];
        const float rowCenter = trialFirstRowCenter + relF * trialRowStep;
        const float rowY = rowCenter - 43.0f * uiS;
        if (rowCenter + trialRowHitHalf < trialViewTop - 8.0f * uiS
            || rowCenter - trialRowHitHalf > trialViewBottom + 8.0f * uiS) {
            trialHover[id] = UpdateMenuCommandHover(trialHover[id], false, dt);
            continue;
        }
        const float rowX = trialRailXAt(rowCenter);
        const bool on = s_RcTrialEnabled[id];
        const bool rowHov = ready && overTrialList
            && mx >= rowX - 34.0f * uiS
            && mx <= trialListRight + 8.0f * uiS
            && my >= rowCenter - trialRowHitHalf
            && my <= rowCenter + trialRowHitHalf;
        trialHover[id] = UpdateMenuCommandHover(trialHover[id], rowHov, dt);
        if (rowHov && trialFocus != id) {
            trialFocus = id;
            trialSelectedSlot = slot;
        }
        const float hover = trialHover[id];
        // PLAY keeps each trial anchored to the diagonal rail. Hover nudges
        // the readable row to the right, never across the rail into the hero.
        // An enabled trial intentionally keeps that same visual hover state
        // even when the pointer leaves the catalogue, so the active build is
        // scannable at a glance.
        const float keyboardFocus = slot == trialSelectedSlot ? 0.36f : 0.0f;
        const float visualHover = std::max(std::max(hover, keyboardFocus),
                                           on ? 1.0f : 0.0f);
        const float itemX = rowX + 22.0f * visualHover;
        const float rowA = (on ? 0.84f : 0.68f) + 0.16f * visualHover;
        if (visualHover > 0.02f) {
            const float hoverA = visualHover;
            const float hoverR = on ? trialR : weaponR;
            const float hoverG = on ? trialG : weaponG;
            const float hoverB = on ? trialB : weaponB;
            DrawConstellationDisc(itemX - 2.0f * uiS,
                                  rowY + 39.0f * uiS,
                                  (13.0f + 8.0f * hoverA) * uiS,
                                  hoverR, hoverG, hoverB,
                                  0.07f * hoverA * contentA);
        }
        if (visualHover > 0.02f)
            DrawVisibleConstellLine(rowX, rowCenter,
                                    itemX + 5.0f * uiS, rowCenter,
                                    0.58f * uiS,
                                    on ? trialR : weaponR, on ? trialG : weaponG,
                                    on ? trialB : weaponB,
                                    (0.10f + 0.24f * visualHover) * contentA);
        DrawVisibleConstellNode(itemX + 5.0f * uiS, rowY + 32.0f * uiS,
                                (on ? 4.2f : 3.0f + visualHover) * uiS,
                                on ? trialR : weaponR, on ? trialG : weaponG,
                                on ? trialB : weaponB,
                                rowA * contentA, on, on);
        const float textLift = std::min(1.0f, visualHover * 1.25f);
        const float titleR = on ? 1.0f : 0.94f + 0.06f * textLift;
        const float titleG = on ? 0.78f : 0.97f + 0.03f * textLift;
        const float titleB = on ? 0.42f : 1.0f;
        const float titleMaxW = std::max(2.0f,
            trialListRight - (itemX + 20.0f * uiS) - 16.0f * uiS);
        float titleScale = trialTitleScale;
        while (titleScale > 0.58f * uiS
               && g_TextS.Width(trialName(id), titleScale) > titleMaxW)
            titleScale -= 0.025f * uiS;
        DrawShadowedText(g_TextS, trialName(id),
                         itemX + 20.0f * uiS, rowY + 1.0f * uiS,
                         titleScale, titleR, titleG, titleB,
                         ((on ? 1.0f : 0.94f) + 0.06f * visualHover) * contentA,
                         0.56f);
        // The effect line is the challenge/debuff, so keep it red even when
        // the row is not selected.  The score payoff below is the player
        // benefit and is always cyan for quick scanning.
        const float compactR = 1.0f;
        const float compactG = 0.28f + 0.08f * textLift;
        const float compactB = 0.28f + 0.08f * textLift;
        DrawShadowedText(g_TextS, trialCompact(id),
                         itemX + 20.0f * uiS, rowY + 47.0f * uiS,
                         trialEffectScale, compactR, compactG, compactB,
                         ((on ? 1.0f : 0.90f) + 0.10f * visualHover) * contentA,
                         0.46f);
        const float tagY = rowY + 74.0f * uiS;
        const float tagAlpha = ((on ? 1.0f : 0.88f) + 0.12f * visualHover)
                             * contentA;
        std::wstring stageText = std::wstring(stageLabel(stageIndex(id)))
                                + L"  //";
        DrawShadowedText(g_TextS, stageText.c_str(),
                         itemX + 20.0f * uiS, tagY, trialTagScale,
                         0.66f, 0.76f, 0.84f, tagAlpha, 0.42f);
        wchar_t scoreBuf[32] = {};
        const int scorePct = (int)(TrialScoreBonusForDef(id) * 100.0f + 0.5f);
        swprintf_s(scoreBuf, ko ? L"점수 +%d%%" : L"SCORE +%d%%", scorePct);
        const float stageW = g_TextS.Width(stageText.c_str(), trialTagScale);
        DrawShadowedText(g_TextS, scoreBuf,
                         itemX + 20.0f * uiS + stageW + 9.0f * uiS,
                         tagY, trialTagScale,
                         0.20f, 0.92f, 1.0f, tagAlpha, 0.46f);
        LogoLine(itemX, rowY + 100.0f * uiS,
                 trialListRight, rowY + 100.0f * uiS,
                 0.55f * uiS, trialR, trialG, trialB,
                 (on ? 0.20f : 0.10f) * contentA);
        if (rowHov && lmbClick) {
            // The whole row is one gesture surface. Release without movement
            // toggles this trial; movement turns the same press into a drag.
            trialDragging = true;
            trialDragMoved = false;
            trialPressedId = id;
            trialDragLastY = (float)my;
            trialDragLastX = (float)mx;
            trialFocus = id;
            trialSelectedSlot = slot;
            detailScroll = 0.0f;
        }
    }
    BatchFlush();
    glDisable(GL_SCISSOR_TEST);

    // Shared bottom rail: BACK | PLAY | TRIAL RESET. Each command keeps its
    // own column anchor, while the third command is aligned with the trial
    // catalogue above it.
    const float routeHitX = routeX - 22.0f * uiS;
    const float routeHitW = routeW + 44.0f * uiS;
    const bool backHov = ready && mx >= routeHitX && mx <= routeHitX + routeHitW
        && my >= routeY && my <= routeY + routeH;
    const bool playHov = ready && mx >= playX && mx <= playX + playW
        && my >= playY - 12.0f * uiS && my <= playY + playH + 10.0f * uiS;
    backHover = UpdateMenuCommandHover(backHover, backHov, dt);
    playHover = UpdateMenuCommandHover(playHover, playHov, dt);
    static const wchar_t* backVariants[] = { L"뒤로", L"BACK", L"戻る" };
    static const wchar_t* playVariants[] = { L"플레이", L"PLAY", L"プレイ" };
    static const wchar_t* resetVariants[] = { L"시련 초기화", L"TRIAL RESET", L"試練をリセット" };
    DrawUnifiedMenuCommand(PlayText(L"뒤로", L"BACK", L"戻る"),
                           routeX, routeY, routeW, routeH,
                           0.48f, 0.82f, 1.0f, contentA,
                           backHover, false, 0.0f, now,
                           uiS, true, true, true,
                           backVariants, 3);
    DrawUnifiedMenuCommand(PlayText(L"플레이", L"PLAY", L"プレイ"),
                           playX, playY, playW, playH,
                           weaponR, weaponG, weaponB, contentA,
                           playHover, true,
                           0.12f + 0.10f * sinf(now * 2.2f), now + 0.51f,
                           uiS, true, true, true,
                           playVariants, 3);
    DrawUnifiedMenuCommand(resetLabel,
                           resetX, resetY, resetW, routeH,
                           trialR, trialG, trialB, contentA,
                           resetHover, false,
                           0.08f + 0.08f * sinf(now * 1.8f), now + 0.86f,
                           uiS, true, true, true,
                           resetVariants, 3);

    if (backHov && lmbClick && ready) exiting = true;
    if (playHov && lmbClick && ready) {
        syncTrialsToGame();
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
