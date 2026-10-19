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
static int s_RcDifficulty = 1;   // 0=EASY 1=NORMAL 2=HARD, 패널을 다시 열어도 유지
static bool s_RcStateLoaded = false;
static float s_RcEntryT = 0.0f;

namespace {

// ── PLAY 패널 배치 상수 (uiS 를 곱해 쓴다) ──
constexpr float kPlayColumnInset  = 72.0f;   // 직업/난이도 열을 페이지 여백에서 안쪽으로
constexpr float kPlayColumnGap    = 34.0f;   // 좌우 열과 중앙 열 사이
constexpr float kPlayEntryTopGap  = 62.0f;   // 본문 상단 → 첫 선택지
constexpr float kPlayEntryStep    = 108.0f;  // 선택지 간격
constexpr float kPlayEntryHeight  = 84.0f;   // 선택지 한 줄의 세로선 길이/클릭 높이
constexpr float kPlayEntryTextX   = 22.0f;   // 세로선 → 글자
constexpr float kPlayGaugeStep    = 28.0f;   // 난이도 별 게이지 간격
constexpr float kPlayTextShadow   = 0.92f;   // 밝은 바탕화면 위에서도 읽히도록 글자 그림자를 진하게
constexpr float kPlayGaugeSize    = 11.0f;   // 난이도 별 게이지 노드 크기
constexpr float kPlayEntrySelectedScale = 1.12f;   // 선택된 이름의 확대 배율
constexpr float kPlayHeroRevealSec = 1.0f;   // 별자리 등장 연출 길이(초). 앱 실행 후 처음 한 번만 재생, 클릭하면 건너뜀
constexpr float kPlayHeroFadeSec   = 0.30f;  // 직업을 바꿀 때 새 별자리가 나타나는 시간(초)
constexpr float kPlayHeroNodeScale = 1.9f;    // drawDiamond 는 인자의 약 절반으로 그려져 보정
constexpr int   kPlayWeaponCount  = 2;
constexpr int   kPlayDifficultyCount = 3;

struct PlayRGB { float r, g, b; };
constexpr PlayRGB kPlayWeaponColor[kPlayWeaponCount] = {
    { 0.28f, 0.90f, 1.00f },   // 소총
    { 1.00f, 0.72f, 0.24f },   // 전기장
};
// 난이도 강조색: 쉬움=초록, 보통=하늘, 어려움=붉은 주황.
constexpr PlayRGB kPlayDifficultyColor[kPlayDifficultyCount] = {
    { 0.38f, 0.95f, 0.62f }, { 0.48f, 0.82f, 1.00f }, { 1.00f, 0.42f, 0.30f },
};

// ── PLAY 패널 문구 (KR, EN, JP) ──
const LStr kTxtPlay        = { { L"플레이", L"PLAY", L"プレイ" } };
const LStr kTxtClass       = { { L"직업", L"CLASS", L"クラス" } };
const LStr kTxtDifficulty  = { { L"난이도", L"DIFFICULTY", L"難易度" } };
const LStr kTxtConstellation = { { L"무기 별자리", L"WEAPON CONSTELLATION", L"武器星座" } };
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
};

constexpr int kPlayStatCount = 6;
const LStr kTxtStatName[kPlayWeaponCount][kPlayStatCount] = {
    { { { L"공격력", L"DAMAGE", L"攻撃力" } },
      { { L"연사 속도", L"FIRE RATE", L"連射速度" } },
      { { L"간격", L"INTERVAL", L"間隔" } },
      { { L"탄속", L"SPEED", L"弾速" } },
      { { L"퍼짐", L"SPREAD", L"拡散" } },
      { { L"사거리", L"RANGE", L"射程" } } },
    { { { L"펄스 피해", L"PULSE DAMAGE", L"パルス威力" } },
      { { L"초당 펄스", L"PULSES / SEC", L"毎秒パルス" } },
      { { L"펄스 간격", L"PULSE INTERVAL", L"パルス間隔" } },
      { { L"전파 속도", L"TRAVEL SPEED", L"伝播速度" } },
      { { L"공격각", L"COVERAGE", L"攻撃角" } },
      { { L"반경", L"RADIUS", L"半径" } } },
};
const LStr kTxtScreenRange = { { L"화면", L"SCREEN", L"画面" } };
const LStr kTxtInstant = { { L"즉시", L"INSTANT", L"即時" } };
// 스탯 막대 길이(0~1). 값이 아니라 무기 간 상대 비교용이다.
constexpr float kPlayStatBar[kPlayWeaponCount][kPlayStatCount] = {
    { 0.50f, 0.78f, 0.78f, 0.72f, 0.68f, 0.72f },
    { 0.22f, 0.86f, 0.94f, 1.00f, 0.50f, 0.28f },
};

// 별자리 뒷배경: 검은 딤 원판 한 겹 + 직업색 원판 한 겹. 노드마다 테두리·halo 를 두지 않고
// 별자리 전체를 이 두 장으로 배경에서 분리한다. 눕힌 별자리에 맞춰 원판도 타원이다.
void DrawPlayHeroBackdrop(int weapon, float cx, float cy, float R, float alpha) {
    const PlayRGB c = kPlayWeaponColor[weapon];
    // 검은 딤: 넓게 깔아 밝은 바탕화면을 눌러 준다.
    DrawConstellationDiscEllipse(cx, cy, R * 1.75f, R * 1.75f,
                                 0.0f, 0.0f, 0.012f, 0.72f * alpha);
    DrawConstellationDiscEllipse(cx, cy, R * 1.25f, R * 1.25f,
                                 0.0f, 0.0f, 0.012f, 0.72f * alpha);
    // 직업색: 별자리 바로 뒤에 은은한 색 원판.
    DrawConstellationDiscEllipse(cx, cy, R * 1.30f, R * 1.30f,
                                 c.r * 0.30f, c.g * 0.30f, c.b * 0.30f,
                                 0.34f * alpha, false);
}

// ── PLAY 중앙 무기 별자리 ──
// 컨셉 문서의 문법(선만, 빈 원 + 흰 점 노드, 틈이 있는 호, 별자리 줄기)으로 그린 정면 도면.
// 좌표는 외곽 링 반지름을 124 로 본 단위(u)이고 R 이 그 반지름이다.
//   소총   : 줄기가 코어에서 위(표적)·아래(꼬리)로 뻗고, 호 3겹이 서로 다른 방향으로 돈다.
//   전기장 : 코어에서 안쪽 별 3개가 갈라지고, 별 전체가 천천히 돌며 호 3겹이 반대로 돈다.
//   spin   회전 누적 시간(초). 호출 쪽이 속도를 점점 올려 부드럽게 돌기 시작한다.
//   reveal 등장 진행도 0..1. 크기 변화 없이 선이 코어에서 바깥으로 그려지고 별이 차례로 켜진다.
//   clock  연속 시계(초). 탄 이동·호 맥박에 쓴다. 등장 여부와 무관하게 계속 흐른다.
void DrawPlayHeroConstellation(int weapon, float cx, float cy, float R,
                               float uiS, float alpha, float spin, float reveal,
                               float clock) {
    const PlayRGB c = kPlayWeaponColor[weapon];
    const PlayRGB w = weapon == 0 ? PlayRGB{ 1.0f, 1.0f, 1.0f }
                                  : PlayRGB{ 1.0f, 0.96f, 0.87f };
    const float u = R / 124.0f;
    constexpr float kPi = 3.14159265f;

    auto stage = [&](float from, float to) {
        return std::max(0.0f, std::min(1.0f, (reveal - from) / (to - from)));
    };
    auto sstep = [](float x) { return x * x * (3.0f - 2.0f * x); };
    auto S = [&](float from, float to) { return sstep(stage(from, to)); };
    float ga = 1.0f;                                  // 지금 그리는 묶음의 알파 배율
    auto group = [&](float a) { ga = a; };

    // u 좌표 선분(prog 만큼만) / 호 / 빈 원 + 흰 점 / 섬광.
    auto Lp = [&](float x0, float y0, float x1, float y1, float thick, PlayRGB col,
                  float a, float prog) {
        if (prog <= 0.001f) return;
        DrawConstellLineBatched(cx + x0 * u, cy + y0 * u,
                                cx + (x0 + (x1 - x0) * prog) * u,
                                cy + (y0 + (y1 - y0) * prog) * u,
                                thick * uiS, col.r, col.g, col.b, a * alpha * ga);
    };
    auto arc = [&](float r, float a0, float a1, float thick, PlayRGB col, float a,
                   float prog) {
        const int steps = std::max(2, (int)std::ceil(std::fabs(a1 - a0) / 4.0f));
        const int n = (int)std::ceil(steps * std::max(0.0f, std::min(1.0f, prog)));
        for (int i = 0; i < n; ++i) {
            const float t0 = (a0 + (a1 - a0) * i / steps) * kPi / 180.0f;
            const float t1 = (a0 + (a1 - a0) * (i + 1) / steps) * kPi / 180.0f;
            Lp(r * cosf(t0), r * sinf(t0), r * cosf(t1), r * sinf(t1), thick, col, a, 1.0f);
        }
    };
    auto ring = [&](float x, float y, float rpx, float a) {
        const float px = cx + x * u, py = cy + y * u;
        for (int i = 0; i < 12; ++i) {
            const float t0 = 2.0f * kPi * i / 12.0f, t1 = 2.0f * kPi * (i + 1) / 12.0f;
            DrawConstellLineBatched(px + rpx * uiS * cosf(t0), py + rpx * uiS * sinf(t0),
                                    px + rpx * uiS * cosf(t1), py + rpx * uiS * sinf(t1),
                                    1.1f * uiS, c.r, c.g, c.b, a * alpha * ga);
        }
    };
    auto dotW = [&](float x, float y, float rpx, float a) {
        drawCircle(cx + x * u, cy + y * u, rpx * uiS, 1.0f, 1.0f, 1.0f, a * alpha * ga);
    };
    auto star = [&](float x, float y, float rpx, float a) {
        ring(x, y, rpx, a * 0.9f);
        dotW(x, y, rpx * 0.4f, a);
    };
    auto flash = [&](float x, float y, PlayRGB col, float a) {
        const float px = cx + x * u, py = cy + y * u;
        const float dx[4] = { 0, 0, -1, 1 }, dy[4] = { -1, 1, 0, 0 };
        for (int i = 0; i < 4; ++i)
            DrawConstellLineBatched(px + dx[i] * 5.0f * uiS, py + dy[i] * 5.0f * uiS,
                                    px + dx[i] * 12.0f * uiS, py + dy[i] * 12.0f * uiS,
                                    1.3f * uiS, col.r, col.g, col.b, a * alpha * ga);
    };
    auto spark = [&](float x, float y) {
        drawCircle(cx + x * u, cy + y * u, 1.2f * uiS, c.r, c.g, c.b, 0.5f * alpha * ga);
    };
    auto pulse = [&](float period, float phase) {          // 0..1 로 부드럽게 오가는 값
        return 0.5f + 0.5f * sinf(2.0f * kPi * (clock / period - phase));
    };
    auto polar = [&](float r, float deg, float& x, float& y) {
        x = r * cosf(deg * kPi / 180.0f);
        y = r * sinf(deg * kPi / 180.0f);
    };

    // 공통 등장 단계(겹치게): 코어 → 호 3겹 → 줄기/안쪽 별 → 바깥 → 외곽 링.
    const float eCore = S(0.00f, 0.25f);
    const float eLay[3] = { S(0.10f, 0.45f), S(0.20f, 0.55f), S(0.30f, 0.65f) };
    const float eRing = S(0.55f, 0.95f);
    const float lay[3] = { 0.95f, 0.60f, 0.32f };
    const float rr[3] = { 42, 72, 102 };

    if (weapon == 0) {
        // ── 소총 ──
        const float stalk[6][2] = { { -20, 84 }, { -12, 42 }, { 0, 0 },
                                    { -8, -44 }, { 11, -74 }, { 6, -114 } };
        // 줄기: 코어에서 위·아래로 한 마디씩 그려지고, 선이 닿으면 별이 켜진다.
        const float segFrom[5] = { 0.36f, 0.22f, 0.22f, 0.36f, 0.48f };   // tail-n2, n2-core, core-n4, n4-n5, n5-main
        const float segTo[5]   = { 0.58f, 0.40f, 0.40f, 0.54f, 0.68f };
        group(1.0f);
        for (int i = 0; i < 5; ++i) {
            const float p = S(segFrom[i], segTo[i]);
            // 위로 가는 선분은 코어 쪽에서, 아래로 가는 선분은 코어 쪽에서 뻗는다.
            if (i >= 2) Lp(stalk[i][0], stalk[i][1], stalk[i + 1][0], stalk[i + 1][1], 1.5f, c, 0.9f, p);
            else        Lp(stalk[i + 1][0], stalk[i + 1][1], stalk[i][0], stalk[i][1], 1.5f, c, 0.9f, p);
        }
        // 호 3겹: 좌우 괄호 모양이 서로 다른 방향으로 천천히 돈다.
        const float half[3] = { 46, 40, 34 };
        const float rate[3] = { 14.0f, -10.0f, 7.0f };            // 도/초
        for (int li = 0; li < 3; ++li) {
            const float rot = spin * rate[li];
            const float a = lay[li] * (0.8f + 0.2f * pulse(1.6f, li * 0.2f));
            arc(rr[li], 180 - half[li] + rot, 180 + half[li] + rot, 1.7f, c, a, eLay[li]);
            arc(rr[li], -half[li] + rot, half[li] + rot, 1.7f, c, a, eLay[li]);
        }
        // 외곽: 얇은 링 + 밝은 호 4개(끝에 흰 점). 위쪽 호가 가장 길고 전체가 천천히 돈다.
        struct Bright { float a0, a1, thick, a; };
        const Bright bright[4] = { { -142, -38, 2.6f, 0.98f }, { 8, 36, 2.0f, 0.85f },
                                   { 144, 172, 2.0f, 0.85f }, { 78, 102, 1.8f, 0.70f } };
        const float outRot = -spin * 5.0f;
        arc(124, 0, 360, 0.8f, c, 0.18f, eRing);
        for (const Bright& b : bright) {
            arc(124, b.a0 + outRot, b.a1 + outRot, b.thick, c, b.a, eRing);
            if (eRing > 0.98f)
                for (int e = 0; e < 2; ++e) {
                    float x, y; polar(124, (e == 0 ? b.a0 : b.a1) + outRot, x, y);
                    dotW(x, y, 2.2f, b.a);
                }
        }
        // 별(선이 닿은 뒤에 켜짐)
        const float starA[6] = { S(0.52f, 0.66f), S(0.34f, 0.46f), 0.0f,
                                 S(0.34f, 0.46f), S(0.48f, 0.60f), S(0.62f, 0.74f) };
        for (int i : { 0, 1, 3, 4 }) { group(starA[i]); star(stalk[i][0], stalk[i][1], 4.4f, 0.9f); }
        group(S(0.60f, 0.85f));
        for (float sx : { -72.0f, 72.0f }) star(sx, 0, 3.6f, 0.85f);
        const float dust[6][2] = { { 44, -50 }, { -58, 36 }, { -44, -20 },
                                   { 50, 62 }, { -74, -50 }, { 66, 0 } };
        for (const auto& d : dust) spark(d[0], d[1]);
        // 탄: 줄기를 따라 올라가 표적에 닿으면 표적 섬광이 강해진다(등장이 끝난 뒤).
        float len[5], total = 0.0f;
        for (int i = 0; i < 5; ++i) {
            len[i] = hypotf(stalk[i + 1][0] - stalk[i][0], stalk[i + 1][1] - stalk[i][1]);
            total += len[i];
        }
        const float s = fmodf(clock, 1.8f) / 1.8f;
        auto atDist = [&](float d, float& x, float& y) {   // 꼬리에서 표적 방향으로 거리 d
            d = std::max(0.0f, std::min(total, d));
            for (int i = 0; i < 5; ++i) {
                if (d <= len[i] || i == 4) {
                    const float t = len[i] > 0.0f ? std::min(1.0f, d / len[i]) : 0.0f;
                    x = stalk[i][0] + (stalk[i + 1][0] - stalk[i][0]) * t;
                    y = stalk[i][1] + (stalk[i + 1][1] - stalk[i][1]) * t;
                    return;
                }
                d -= len[i];
            }
        };
        group(S(0.88f, 1.00f));
        const float trailD[3] = { 0.0f, 0.07f, 0.14f }, trailA[3] = { 1.0f, 0.5f, 0.28f };
        const float trailS[3] = { 3.0f, 2.2f, 1.7f };
        for (int k = 0; k < 3; ++k) {
            const float d = (s - trailD[k]) * total;
            if (d < 0.0f) continue;
            float x, y; atDist(d, x, y);
            drawDiamond(cx + x * u, cy + y * u, trailS[k] * kPlayHeroNodeScale * uiS,
                        1.0f, 1.0f, 1.0f, trailA[k] * alpha * ga);
        }
        // 표적 + 섬광, 코어
        const float hit = std::max(0.0f, std::min(1.0f, (s - 0.9f) / 0.1f));
        group(starA[5]);
        star(stalk[5][0], stalk[5][1], 8.5f, 1.0f);
        flash(stalk[5][0], stalk[5][1], w, (0.55f + 0.45f * hit) * S(0.85f, 1.0f));
        group(eCore);
        ring(0, 0, 16, 1.0f);
        dotW(0, 0, 5.0f, 1.0f);
    } else {
        // ── 전기장 ──
        const float rot = spin * 3.0f;                       // 별 전체가 천천히 돈다.
        // 호 3겹: 안쪽일수록 진하고 맥박이 바깥으로 번진다. 겹마다 방향이 다르다.
        const float off[3] = { 10, 48, 100 };
        const float rate[3] = { 12.0f, -9.0f, 6.0f };
        group(1.0f);
        for (int li = 0; li < 3; ++li) {
            const float k = 0.55f + 0.45f * powf(pulse(1.3f, li * 0.22f), 2.0f);
            const float r0 = off[li] + spin * rate[li];
            for (int a = 0; a < 3; ++a)
                arc(rr[li], r0 + a * 120.0f, r0 + a * 120.0f + 84.0f, 1.7f, c,
                    lay[li] * k, eLay[li]);
        }
        // 외곽: 얇은 링 + 밝은 호 3개(끝에 흰 점)
        const float outRot = spin * 6.0f;
        arc(124, 0, 360, 0.8f, c, 0.18f, eRing);
        for (int k = 0; k < 3; ++k) {
            const float a0 = k * 120.0f + 8.0f + outRot, a1 = a0 + 80.0f;
            arc(124, a0, a1, 2.2f, c, 0.95f, eRing);
            if (eRing > 0.98f)
                for (int e = 0; e < 2; ++e) {
                    float x, y; polar(124, e == 0 ? a0 : a1, x, y);
                    dotW(x, y, 2.2f, 0.95f);
                }
        }
        // 안쪽 별 3개: 코어에서 가지가 뻗고 닿으면 켜진다.
        const float innerA[3] = { 112, 232, 352 };
        float ix[3], iy[3];
        for (int i = 0; i < 3; ++i) {
            polar(57, innerA[i] + rot, ix[i], iy[i]);
            const float p = S(0.22f + 0.04f * i, 0.46f + 0.04f * i);
            group(1.0f);
            Lp(0, 0, ix[i], iy[i], 1.0f, c, 0.6f, p);
            group(S(0.40f + 0.04f * i, 0.54f + 0.04f * i));
            star(ix[i], iy[i], 5.0f, 0.9f);
        }
        // 바깥 별 5개 + 선, 맞은 별 2개는 섬광
        const float outR[5] = { 106, 100, 104, 98, 104 };
        const float outA[5] = { -80, -12, 45, 140, 195 };
        float ox[5], oy[5];
        for (int i = 0; i < 5; ++i) polar(outR[i], outA[i] + rot, ox[i], oy[i]);
        group(1.0f);
        const float pLine = S(0.50f, 0.78f);
        Lp(ox[0], oy[0], ox[1], oy[1], 1.0f, c, 0.5f, pLine);
        Lp(ox[1], oy[1], ox[2], oy[2], 1.0f, c, 0.5f, pLine);
        Lp(ox[3], oy[3], ox[4], oy[4], 1.0f, c, 0.5f, pLine);
        for (int i = 0; i < 5; ++i) {
            group(S(0.48f + 0.05f * i, 0.62f + 0.05f * i));
            star(ox[i], oy[i], i == 1 || i == 3 ? 4.2f : 3.8f, 0.9f);
        }
        const float fl = 0.5f + 0.5f * powf(pulse(1.3f, 2 * 0.22f), 2.0f);
        group(S(0.80f, 1.00f));
        for (int i : { 1, 3 }) flash(ox[i], oy[i], w, fl);
        // 코어: 링 + 삼각형
        group(eCore);
        ring(0, 0, 16, 1.0f);
        for (int i = 0; i < 3; ++i) {
            const float t0 = (-90.0f + 120.0f * i) * kPi / 180.0f;
            const float t1 = (-90.0f + 120.0f * (i + 1)) * kPi / 180.0f;
            DrawConstellLineBatched(cx + 9.0f * uiS * cosf(t0), cy + 9.0f * uiS * sinf(t0),
                                    cx + 9.0f * uiS * cosf(t1), cy + 9.0f * uiS * sinf(t1),
                                    1.4f * uiS, w.r, w.g, w.b, alpha * eCore);
        }
        dotW(0, 1.5f, 2.2f, 1.0f);
    }
}

}  // namespace

static bool s_RcHeroRevealDone[kPlayWeaponCount] = {};   // 별자리 등장 연출은 직업마다 앱 실행 후 한 번만

void ResetRunConfigUi(float entryStart) {
    s_RcEntryT = std::max(0.0f, std::min(1.0f, entryStart));
    ResetTrials();
    s_RcWeapon = 0;
    s_RcStateLoaded = false;
}


void FinalizeLoadout(const SceneCtx& c, int wIdx) {
    float& fireTimer = *c.fireTimer;

    g_Difficulty = static_cast<Difficulty>(std::max(0, std::min(2, s_RcDifficulty)));
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
    static float heroTimer = 0.0f;   // 별자리 등장 경과 시간(앱 실행 후 첫 재생에만 쓴다)
    static float heroSpin = 0.0f;    // 별자리 회전용 누적 시간. 패널을 다시 열어도 이어서 돈다.
    static float heroClock = 0.0f;   // 탄 이동·호 맥박용 연속 시계
    static float heroFade = 1.0f;    // 직업을 바꾼 직후 새 별자리가 나타나는 진행도 0..1
    static int heroWeapon = -1;
    static int heroStable = 0;       // 연속으로 정상 속도였던 프레임 수
    static float statT[kPlayStatCount] = {};
    static bool statInit = false;
    if (!statInit) {
        for (int i = 0; i < kPlayStatCount; ++i) statT[i] = kPlayStatBar[0][i];
        statInit = true;
    }
    auto weaponName = [&](int id) { return T(kTxtWeaponName[id]); };
    auto weaponSub  = [&](int id) { return T(kTxtWeaponSub[id]); };
    auto statName   = [&](int id) { return T(kTxtStatName[weapon][id]); };
    auto diffName   = [&](int id) { return T(kTxtDifficultyName[id]); };

    if (c.inputFocusChanged) {
        for (bool& k : prevKey) k = true;   // 포커스 복귀 직후 키 입력 무시
    }

    if (!s_RcStateLoaded && !exiting) {
        weapon = std::max(0, std::min(1, s_RcWeapon));
        for (float& h : weaponHover) h = 0.0f;
        for (float& h : diffHover) h = 0.0f;
        backHover = playHover = 0.0f;
        heroTimer = s_RcHeroRevealDone[weapon] ? kPlayHeroRevealSec : 0.0f;
        heroFade = 1.0f;
        heroStable = 0;
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
        if (keyNow[1] && !prevKey[1]) s_RcDifficulty = std::min(2, s_RcDifficulty + 1);
        if (keyNow[2] && !prevKey[2]) { weapon = 0; s_RcWeapon = weapon; }
        if (keyNow[3] && !prevKey[3]) { weapon = 1; s_RcWeapon = weapon; }
    }
    for (int i = 0; i < 4; ++i) prevKey[i] = keyNow[i];

    BatchFlush();
    DrawPersistentSceneSideVignettes(sw, sh,
                                     kWideSceneLinearAlpha * contentA);
    // 별자리 등장 연출은 앱 실행 후 처음 한 번만 재생한다. 이후에는 패널을 열 때 바로 완성
    // 상태로 시작하고, 직업을 바꾸면 짧게 페이드 인만 한다. 회전은 항상 이어서 돈다.
    // 패널을 여는 순간의 일회성 프레임 지연이나 일시적인 끊김이 애니메이션 점프로 보이지 않도록
    // 연출 시간은 한 프레임에 최대 1/30초만 진행하고, 프레임이 안정된 뒤에 시작한다.
    const float heroDt = std::min(dt, 1.0f / 30.0f);
    heroStable = c.delta < 0.030f ? heroStable + 1 : 0;
    heroClock += heroDt;
    // 직업을 바꾼 프레임: 처음 보는 직업이면 등장 연출을 시작하고, 이미 본 직업이면 페이드 인만 한다.
    bool heroSwitchedNow = false;
    if (heroWeapon != weapon) {
        heroWeapon = weapon;
        heroSwitchedNow = true;
        if (s_RcHeroRevealDone[weapon]) {
            heroFade = 0.0f;
        } else {
            heroTimer = 0.0f;
            heroFade = 1.0f;
            heroStable = 4;                         // 이미 화면이 안정된 상태이므로 바로 시작
        }
    }
    heroFade = std::min(1.0f, heroFade + heroDt / kPlayHeroFadeSec);
    if (!s_RcHeroRevealDone[weapon]) {
        if (entryIn > 0.15f && heroStable >= 4) heroTimer += heroDt;   // 전환이 보이고 프레임이 안정된 뒤 시작
        // 클릭하면 건너뛴다. 단, 직업을 고르려고 누른 그 클릭은 건너뛰기로 치지 않는다.
        if (lmbClick && !heroSwitchedNow) heroTimer = kPlayHeroRevealSec;
        if (heroTimer >= kPlayHeroRevealSec) s_RcHeroRevealDone[weapon] = true;
    } else {
        heroTimer = kPlayHeroRevealSec;
    }
    const float heroReveal = std::min(1.0f, heroTimer / kPlayHeroRevealSec);
    // 회전 속도: 등장 중에는 바깥 별이 나온 뒤(40% 지점)부터 서서히 올라가고, 이후에는 일정하다.
    const float spinSpeed = s_RcHeroRevealDone[weapon] ? 1.0f
        : Smoothstep(LogoClamp01((heroTimer - 0.40f * kPlayHeroRevealSec) / 0.6f));
    heroSpin += heroDt * spinSpeed;
    const float heroAlpha = contentA * Smoothstep(heroFade);
    // 별자리 크기·위치: 제목 글자(아래 끝)와 스탯(위 끝) 사이의 빈 공간에 맞춘다.
    const float heroTitleBottom = chartCY - chartR - 58.0f * uiS
        + g_TextL.Height(weaponName(weapon), UiTextScale(g_TextL, UiTextLevel::Title, uiS));
    const float heroGapTop = heroTitleBottom + 14.0f * uiS;
    const float heroGapBottom = chartCY + chartR + 26.0f * uiS - 14.0f * uiS;
    const float heroCY = (heroGapTop + heroGapBottom) * 0.5f;
    const float heroR = std::max(40.0f * uiS, std::min(
        std::min(chartR, (heroGapBottom - heroGapTop) * 0.5f), centerW * 0.5f - 10.0f * uiS));
    DrawPlayHeroBackdrop(weapon, chartCX, heroCY, heroR,
                         heroAlpha * Smoothstep(LogoClamp01(heroReveal / 0.20f)));

    const float playTitleScale = UiTextScale(g_TextL, UiTextLevel::Title, uiS);
    DrawShadowedText(g_TextL, T(kTxtPlay), pageL, headerY,
                     playTitleScale, 1.0f, 1.0f, 1.0f,
                     0.98f * entryHeader, kPlayTextShadow);
    LogoLine(pageL, headerY + 58.0f * uiS, pageR, headerY + 58.0f * uiS,
             0.7f * uiS, weaponR, weaponG, weaponB, 0.12f * contentA);

    // 섹션 머리글: 작은 라벨 + 얇은 선 하나.
    auto drawSectionLabel = [&](const wchar_t* label, float x, float w) {
        DrawShadowedText(g_TextS, label, x, bodyTop, playSubtitleScale,
                         weaponR, weaponG, weaponB, 0.80f * contentA, kPlayTextShadow);
        const float ruleY = bodyTop + g_TextS.Height(label, playSubtitleScale)
                          + 8.0f * uiS;
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
                          float* nameMidY = nullptr) -> bool {
        const bool hov = ready && mx >= x - 14.0f * uiS && mx <= x + w
            && my >= y0 - 8.0f * uiS && my <= y0 + entryH;
        hover = UpdateMenuCommandHover(hover, hov, dt);
        // 선택 안 된 행도 밝은 바탕에서 읽히도록 알파 하한을 높게 둔다.
        const float a = (sel ? 1.0f : 0.80f + 0.20f * hover) * contentA;
        // 선택: 굵고 긴 세로선 + 큰 노드 + 한 단계 큰 이름. 비선택: 가늘고 짧은 선.
        const float lineLen = sel ? entryH : entryH * 0.72f;
        LogoLine(x, y0, x, y0 + lineLen, (sel ? 2.6f : 1.0f) * uiS,
                 col.r, col.g, col.b,
                 (sel ? 1.0f : 0.42f + 0.22f * hover) * contentA);
        if (sel) drawDiamond(x, y0, 12.0f * uiS, col.r, col.g, col.b, 1.0f * contentA);
        const float textX = x + kPlayEntryTextX * uiS;
        const float baseScale = std::min(entryTitleScale,
            entryTitleScale * (w - (kPlayEntryTextX + 4.0f) * uiS) /
            std::max(1.0f, g_TextS.Width(name, entryTitleScale)));
        const float selScale = baseScale * kPlayEntrySelectedScale;
        const float drawScale = sel ? selScale : baseScale;
        // 배치는 선택 크기를 기준으로 고정해, 선택이 바뀌어도 보조 줄이 움직이지 않게 한다.
        const float layoutH = g_TextS.Height(name, selScale);
        const float nameY = y0 + 2.0f * uiS
                          + (layoutH - g_TextS.Height(name, drawScale)) * 0.5f;
        DrawShadowedText(g_TextS, name, textX, nameY, drawScale,
                         sel ? 1.0f : 0.90f, sel ? 1.0f : 0.94f, sel ? 1.0f : 0.98f,
                         a, kPlayTextShadow);
        // 보조 줄은 강조색을 흰색 쪽으로 섞어 밝은 바탕에서도 색이 죽지 않게 한다.
        const float mix = 0.30f;
        DrawShadowedText(g_TextS, sub, textX, y0 + 2.0f * uiS + layoutH + 10.0f * uiS,
                         playBodyScale,
                         col.r + (1.0f - col.r) * mix, col.g + (1.0f - col.g) * mix,
                         col.b + (1.0f - col.b) * mix, a, kPlayTextShadow);
        if (nameMidY) *nameMidY = y0 + 2.0f * uiS + layoutH * 0.5f;
        return hov && lmbClick;
    };

    // 직업 목록(좌)
    drawSectionLabel(T(kTxtClass), leftX, sideW);
    for (int i = 0; i < kPlayWeaponCount; ++i) {
        if (drawChoice(leftX, sideW, entryY(i), weapon == i,
                       weaponHover[i], kPlayWeaponColor[i],
                       weaponName(i), weaponSub(i))) {
            weapon = i;
            s_RcWeapon = weapon;
        }
    }

    // 난이도 목록(우)
    drawSectionLabel(T(kTxtDifficulty), rightX, sideW);
    for (int i = 0; i < kPlayDifficultyCount; ++i) {
        wchar_t bestBuf[64] = {};
        if (g_BestScore[i] > 0) {
            wchar_t grouped[40] = {};
            FormatGrouped(g_BestScore[i], grouped, 40);
            swprintf_s(bestBuf, L"%ls %ls", T(kTxtBest), grouped);
        } else {
            swprintf_s(bestBuf, L"%ls", T(kTxtNoRecord));
        }
        const PlayRGB col = kPlayDifficultyColor[i];
        const bool sel = s_RcDifficulty == i;
        float midY = 0.0f;
        if (drawChoice(rightX, sideW, entryY(i), sel,
                       diffHover[i], col, diffName(i), bestBuf, &midY))
            s_RcDifficulty = i;
        // 이름 줄 오른쪽 끝의 3칸 게이지: 켜진 별 개수가 단계(1~3)다.
        for (int k = 0; k < kPlayDifficultyCount; ++k)
            drawDiamond(rightR - 10.0f * uiS
                            - (kPlayDifficultyCount - 1 - k) * kPlayGaugeStep * uiS,
                        midY, kPlayGaugeSize * uiS, col.r, col.g, col.b,
                        (k <= i ? (sel ? 1.0f : 0.72f) : 0.26f) * contentA);
    }

    // 중앙: 선택 무기 성좌 제목
    const float heroNameY = chartCY - chartR - 58.0f * uiS;
    const float heroNameScale = UiTextScale(g_TextL, UiTextLevel::Title, uiS);
    const float heroNameW = g_TextL.Width(weaponName(weapon), heroNameScale);
    const wchar_t* heroSystemLabel = T(kTxtConstellation);
    DrawShadowedText(g_TextS, heroSystemLabel,
                     chartCX - g_TextS.Width(heroSystemLabel, playSubtitleScale) * 0.5f,
                     heroNameY - 42.0f * uiS, playSubtitleScale,
                     weaponR, weaponG, weaponB, 0.78f * contentA, kPlayTextShadow);
    DrawShadowedText(g_TextL, weaponName(weapon), chartCX - heroNameW * 0.5f,
                     heroNameY, heroNameScale,
                     1.0f, 1.0f, 1.0f, 0.96f * contentA, kPlayTextShadow);

    DrawPlayHeroConstellation(weapon, chartCX, heroCY, heroR, uiS, heroAlpha,
                              heroSpin, heroReveal, heroClock);

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
    const wchar_t* seconds = LangIndex() == 0 ? L"초" : LangIndex() == 2 ? L"秒" : L"s";
    swprintf_s(statValues[0], L"%.1f", damage);
    swprintf_s(statValues[1], L"%.1f/s", 1.0f / interval);
    swprintf_s(statValues[2], L"%.2f%ls", interval, seconds);
    if (field) {
        wcscpy_s(statValues[3], T(kTxtInstant));
        wcscpy_s(statValues[4], L"360°");
        swprintf_s(statValues[5], L"%.0f", STATIC_FIELD_PULSE_RADIUS);
    } else {
        swprintf_s(statValues[3], L"%.0f", previewStats.bulletSpeed);
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
