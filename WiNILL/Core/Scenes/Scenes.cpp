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
#include <algorithm>
#include <vector>
#include <string>
#include <cmath>
#include <functional>

void Scene_MainMenu(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    GLFWwindow* window = c.window;
    const GameState st = g_GameManager.currentState;
    float& fireTimer = *c.fireTimer;
    const std::function<void()>& ResetForNewGame = c.reset;
                int li2 = LangIndex();
                bool booting = (g_BootAnim > 0.0f);

                // 배경을 칠하지 않음 → 투명 프레임버퍼라 "진짜 윈도우 바탕화면"이
                //   그대로 비친다 (Wallpaper Engine 등 라이브 배경도 그대로 보임).
                //   가독성을 위해 아주 옅은 상/하단 비네트만 깐다.
                BindMainShader();
                drawRect(0, 0, sw, 130.0f, 0.0f, 0.0f, 0.0f, 0.18f);
                drawRect(0, sh - 150.0f, sw, 150.0f, 0.0f, 0.0f, 0.0f, 0.22f);

                // ── 앰비언트 — 은은한 빛 입자 + 스캔라인 (진짜 바탕화면 위) ──
                {
                    float dtp = delta; if (dtp > 0.05f) dtp = 0.05f;
                    BindMainShader();
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
                    for (int i = 0; i < 55; i++) { AP& p = a_ps[i];
                        p.x += p.vx*dtp; p.y += p.vy*dtp;
                        if (p.x < -8) p.x = sw+8; if (p.x > sw+8) p.x = -8;
                        if (p.y < -8) p.y = sh+8; if (p.y > sh+8) p.y = -8;
                        p.tw += dtp*1.6f;
                        float a = 0.10f + 0.10f*sinf(p.tw);
                        drawCircle(p.x, p.y, p.sz, 0.55f, 0.78f, 1.0f, a);
                    }
                    // 스캔라인 (시네마틱) — 옅은 가로줄
                    for (float yy = 0.0f; yy < sh; yy += 4.0f)
                        drawRect(0.0f, yy, sw, 1.0f, 0.40f, 0.70f, 1.0f, 0.025f);
                }

                // ── 중앙 로고 + 부제 ──
                const wchar_t* TITLE = T(StrId::GAME_TITLE);
                float logoY = sh * 0.28f;
                // 로고는 고해상도 전용 렌더러(g_TextXL, 100px)로 — 기존 g_TextL 3배
                //   확대 시 비트맵이 뭉개지던 화질 문제 fix.
                g_TextXL.Draw(TITLE, CenterTextX(sw, g_TextXL, TITLE, 1.05f), logoY, 1.05f,
                              0.6f, 0.85f, 1.0f, 0.96f);
                {
                    const wchar_t* SUBT[3] = { L"데스크톱 디펜스", L"Desktop Defense", L"デスクトップ防衛" };
                    float subw = g_TextS.Width(SUBT[li2], 1.05f);
                    // 로고 실제 높이 아래로 — 겹침 방지
                    float subY = logoY + g_TextXL.Height(TITLE, 1.05f) + 18.0f;
                    g_TextS.Draw(SUBT[li2], (sw - subw) * 0.5f, subY, 1.05f,
                                 0.5f, 0.68f, 0.9f, 0.8f);
                }

                // ── PLAY 버튼 (맥동 글로우) → onedow.exe 부팅 → 난이도 선택 ──
                {
                    float pulse = 0.5f + 0.5f * sinf((float)glfwGetTime() * 2.5f);
                    const float PW = 320.0f, PH = 76.0f;
                    float px = (sw - PW) * 0.5f, py = sh * 0.49f;
                    BindMainShader();
                    drawRect(px - 7, py - 7, PW + 14, PH + 14,
                             0.30f, 0.70f, 1.0f, 0.08f + 0.10f * pulse);   // 글로우
                    const wchar_t* PLAYL[3] = { L"실행", L"PLAY", L"実行" };
                    if (UIButton(px, py, PW, PH, PLAYL[li2], mx, my, lmb, g_LmbPrev) && !booting) {
                        LaunchApp(GameState::DIFFICULTY_SELECT, L"onedow.exe", 0.30f, 0.80f, 1.00f);
                    }
                }

                // ── 보조 버튼 행: 상점 · 도감 · 튜토리얼 · 설정 · 종료 ──
                {
                    const float bw2 = 128.0f, bh2 = 48.0f, gap2 = 10.0f;
                    float totalW = bw2 * 5 + gap2 * 4;
                    float bx2 = (sw - totalW) * 0.5f, by2 = sh * 0.49f + 100.0f;
                    const wchar_t* SHOPL[3] = { L"상점", L"Shop",  L"ショップ" };
                    const wchar_t* CODL [3] = { L"도감", L"Codex", L"図鑑" };
                    const wchar_t* TUTL [3] = { L"튜토리얼", L"Tutorial", L"チュートリアル" };
                    const wchar_t* CFGL [3] = { L"설정", L"Config", L"設定" };
                    // 서브창(상점/도감/튜토리얼/설정)은 부팅 로딩 없이 즉시 — 창 열림 애니메이션만
                    if (UIButton(bx2 + 0*(bw2+gap2), by2, bw2, bh2, SHOPL[li2], mx,my,lmb,g_LmbPrev) && !booting)
                        g_GameManager.currentState = GameState::SHOP;
                    if (UIButton(bx2 + 1*(bw2+gap2), by2, bw2, bh2, CODL[li2], mx,my,lmb,g_LmbPrev) && !booting)
                        g_GameManager.currentState = GameState::CODEX;
                    if (UIButton(bx2 + 2*(bw2+gap2), by2, bw2, bh2, TUTL[li2], mx,my,lmb,g_LmbPrev) && !booting)
                        g_GameManager.currentState = GameState::TUTORIAL;
                    if (UIButton(bx2 + 3*(bw2+gap2), by2, bw2, bh2, CFGL[li2], mx,my,lmb,g_LmbPrev) && !booting) {
                        g_SettingsReturnTo = GameState::MAIN_MENU;
                        g_GameManager.currentState = GameState::SETTINGS;
                    }
                    if (UIButton(bx2 + 4*(bw2+gap2), by2, bw2, bh2, T(StrId::BTN_QUIT), mx,my,lmb,g_LmbPrev) && !booting)
                        glfwSetWindowShouldClose(window, GLFW_TRUE);
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
                    drawRect(0, 0, sw, sh, 0.0f, 0.0f, 0.0f, 0.45f * ease);  // 배경 딤
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
                SceneAppWindow(sw, sh, WW, WH, L"shop.exe", 1.0f, 0.80f, 0.20f, wx, wy, false);
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
    const GameState st = g_GameManager.currentState;
    float& fireTimer = *c.fireTimer;
    const std::function<void()>& ResetForNewGame = c.reset;
                // codex.db = 검색 가능한 위키 스타일 앱 창
                const float WW = std::min(sw * 0.92f, 1480.0f);
                const float WH = std::min(sh * 0.90f, 980.0f);
                float wx, wy;
                SceneAppWindow(sw, sh, WW, WH, L"codex.db", 0.40f, 0.90f, 0.50f, wx, wy, false);
                if (g_AppOpen >= 0.999f) {           // 완전히 열린 뒤에만 콘텐츠
                int li = LangIndex();

                // 백스페이스 (검색어 편집)
                {
                    static bool s_bsPrev = false;
                    bool bs = (glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS);
                    if (bs && !s_bsPrev && g_CodexSearchLen > 0)
                        g_CodexSearch[--g_CodexSearchLen] = 0;
                    s_bsPrev = bs;
                }

                // 검색창 (위키 느낌) — 항상 입력 활성
                float searchX = wx + 40.0f, searchY = wy + 46.0f, searchW = 460.0f, searchH = 40.0f;
                BindMainShader();
                drawRect(searchX, searchY, searchW, searchH, 0.12f, 0.14f, 0.18f, 1.0f);
                drawRect(searchX, searchY, searchW, 2.0f, 0.4f, 0.9f, 0.5f, 0.9f);
                if (g_CodexSearchLen > 0) {
                    g_TextS.Draw(g_CodexSearch, searchX + 14.0f, searchY + 10.0f, 0.85f,
                                 1.0f, 1.0f, 1.0f, 1.0f);
                } else {
                    const wchar_t* PH[3] = { L"검색...", L"Search...", L"検索..." };
                    g_TextS.Draw(PH[li], searchX + 14.0f, searchY + 10.0f, 0.85f,
                                 0.5f, 0.55f, 0.6f, 0.9f);
                }
                // 깜빡이는 캐럿
                if (((int)(glfwGetTime() * 2.0) & 1) == 0) {
                    float cwid = g_CodexSearchLen ? g_TextS.Width(g_CodexSearch, 0.85f) : 0.0f;
                    BindMainShader();
                    drawRect(searchX + 14.0f + cwid + 2.0f, searchY + 8.0f, 2.0f, 24.0f,
                             0.9f, 0.95f, 1.0f, 0.9f);
                }
                // 개발 모드 해금 토스트 (이스터에그) — 검색창 아래 잠깐 표시
                if (g_DevToastTimer > 0.0f) {
                    g_DevToastTimer -= delta;
                    float a = g_DevToastTimer > 2.5f ? (3.0f - g_DevToastTimer) / 0.5f
                                                     : (g_DevToastTimer > 1.0f ? 1.0f : g_DevToastTimer);
                    if (a < 0.0f) a = 0.0f; if (a > 1.0f) a = 1.0f;
                    g_TextS.Draw(L"* DEV MODE UNLOCKED - 난이도 화면에서 크리에이티브 활성",
                                 searchX, searchY + searchH + 8.0f, 0.8f,
                                 0.4f, 1.0f, 0.55f, a);
                }

                static int s_tab = 0;   // 0 적 / 1 증강 / 2 보스
                static int s_codexAugSort = 0;  // 0=티어순(기본) 1=분류순
                const wchar_t* TAB_MOB[3] = { L"적", L"Enemies", L"敵" };
                const wchar_t* TAB_AUG[3] = { L"증강", L"Augments", L"強化" };
                const wchar_t* TAB_BOS[3] = { L"보스", L"Bosses", L"ボス" };
                if (UIButton(wx + 520.0f, searchY, 120.0f, searchH, TAB_MOB[li],
                             mx, my, lmb, g_LmbPrev, s_tab == 0)) s_tab = 0;
                if (UIButton(wx + 648.0f, searchY, 120.0f, searchH, TAB_AUG[li],
                             mx, my, lmb, g_LmbPrev, s_tab == 1)) s_tab = 1;
                if (UIButton(wx + 776.0f, searchY, 120.0f, searchH, TAB_BOS[li],
                             mx, my, lmb, g_LmbPrev, s_tab == 2)) s_tab = 2;

                // 증강 탭 — 정렬: 티어(기본) / 분류
                if (s_tab == 1) {
                    const wchar_t* ST_T[3] = { L"티어순", L"By tier", L"ティア順" };
                    const wchar_t* ST_C[3] = { L"분류순", L"By type", L"分類順" };
                    float sx = wx + WW - 248.0f;
                    if (UIButton(sx, searchY, 112.0f, searchH, ST_T[li],
                                 mx, my, lmb, g_LmbPrev, s_codexAugSort == 0))
                        s_codexAugSort = 0;
                    if (UIButton(sx + 120.0f, searchY, 112.0f, searchH, ST_C[li],
                                 mx, my, lmb, g_LmbPrev, s_codexAugSort == 1))
                        s_codexAugSort = 1;
                }

                float gTop = wy + 110.0f;          // 그리드 상단
                float detailY = wy + WH - 170.0f;  // 상세(article) 영역
                int hoverItem = -1;                // 실제 데이터 인덱스

                if (s_tab == 0) {
                    // 적 — 검색 필터링 후 재배치
                    const int COLS = 6; const float CELL = 150.0f;
                    int vis[CM_COUNT], nv = 0;
                    for (int i = 0; i < CM_COUNT; i++)
                        if (g_CodexSearchLen == 0 || (CodexMobSeen(i) && CodexMatch(MobName(i))))
                            vis[nv++] = i;
                    float gx = wx + (WW - COLS*CELL) * 0.5f;
                    for (int k = 0; k < nv; k++) {
                        int i = vis[k];
                        float cxp = gx + (k % COLS) * CELL, cyp = gTop + (k / COLS) * CELL;
                        float cw = CELL - 14.0f;
                        bool seen = CodexMobSeen(i);
                        bool hv = (mx >= cxp && mx < cxp+cw && my >= cyp && my < cyp+cw);
                        if (hv) hoverItem = i;
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
                            } else if (i == CM_BOMBER) {
                                drawPentagon(ccx, ccy, 32.0f, 1.0f, 0.5f, 0.1f, 1.0f);
                            } else if (i == CM_DDOS) {
                                for (int t = 0; t < 3; t++) {
                                    float a = (float)t * 2.0944f;
                                    drawTriangle(ccx + cosf(a)*13.0f, ccy + sinf(a)*13.0f,
                                                 14.0f, 1.0f, 0.35f, 0.55f, 1.0f);
                                }
                            } else if (i == CM_BADSECTOR) {
                                drawPentagon(ccx, ccy, 30.0f, 0.7f, 0.25f, 0.85f, 1.0f);
                                drawPentagon(ccx, ccy, 13.0f, 0.1f, 0.05f, 0.2f, 1.0f);
                            } else {   // CM_REGERROR — X형 노드
                                drawDiamond(ccx, ccy, 26.0f, 1.0f, 0.3f, 0.3f, 1.0f);
                                drawDiamond(ccx + 17, ccy, 9.0f, 1.0f, 0.3f, 0.3f, 1.0f);
                                drawDiamond(ccx - 17, ccy, 9.0f, 1.0f, 0.3f, 0.3f, 1.0f);
                                drawDiamond(ccx, ccy + 17, 9.0f, 1.0f, 0.3f, 0.3f, 1.0f);
                                drawDiamond(ccx, ccy - 17, 9.0f, 1.0f, 0.3f, 0.3f, 1.0f);
                            }
                            BindMainShader();
                            const wchar_t* nm = MobName(i);
                            float nw = g_TextS.Width(nm, 0.8f);
                            g_TextS.Draw(nm, cxp + (cw - nw)*0.5f, cyp + cw - 34.0f, 0.8f,
                                         0.9f, 0.95f, 1.0f, 0.95f);
                        } else {
                            float qw = g_TextL.Width(L"?", 1.3f);
                            g_TextL.Draw(L"?", ccx - qw*0.5f, ccy - 22.0f, 1.3f,
                                         0.4f, 0.4f, 0.45f, 0.9f);
                        }
                    }
                    if (hoverItem >= 0 && CodexMobSeen(hoverItem)) {
                        const wchar_t* nm = MobName(hoverItem);
                        const wchar_t* d  = MobDesc(hoverItem);
                        g_TextL.Draw(nm, wx + 40.0f, detailY, 1.0f, 0.6f, 0.95f, 0.7f, 1.0f);
                        g_TextS.Draw(d,  wx + 40.0f, detailY + 54.0f, 0.9f, 0.85f, 0.95f, 1.0f, 0.95f);
                    }
                } else if (s_tab == 1) {
                    // 증강 — 스크롤 리스트 (기본: 티어순 / 선택: 분류순)
                    int vis[AUG_TOTAL], nv = 0;
                    for (int i = 0; i < AUG_TOTAL; i++) {
                        if (AugRemoved(ALL_AUGS[i].type)) continue;
                        if (g_CodexSearchLen > 0) {
                            if (!CodexMatch(AugName(ALL_AUGS[i]))) continue;
                            if (!CodexFullReveal() && !g_AugSeen[i]) continue;
                        }
                        vis[nv++] = i;
                    }
                    const bool byCategory = (s_codexAugSort == 1);
                    std::sort(vis, vis + nv, [byCategory](int a, int b) {
                        return byCategory ? AugListIndexLess(a, b) : AugTierIndexLess(a, b);
                    });

                    const float LIST_W = WW - 160.0f;
                    const float LIST_X = wx + (WW - LIST_W) * 0.5f;
                    const float ROW_H  = 24.0f;
                    const float HDR_H  = 22.0f;
                    const float listTop = gTop;
                    const float listBottom = detailY - 12.0f;
                    const float viewH = listBottom - listTop;
                    int hdrCount = 0;
                    if (byCategory) {
                        AugListGroup prevGrp = (AugListGroup)-1;
                        for (int k = 0; k < nv; k++) {
                            AugListGroup g = AugListGroupOf(ALL_AUGS[vis[k]]);
                            if (g != prevGrp) { hdrCount++; prevGrp = g; }
                        }
                    } else {
                        AugRarity prevR = (AugRarity)-1;
                        for (int k = 0; k < nv; k++) {
                            AugRarity r = ALL_AUGS[vis[k]].rarity;
                            if (r != prevR) { hdrCount++; prevR = r; }
                        }
                    }
                    const float contentH = (float)nv * ROW_H + (float)hdrCount * HDR_H;
                    static float s_codexAugScroll = 0.0f;
                    bool overList = (mx >= LIST_X && mx <= LIST_X + LIST_W &&
                                     my >= listTop && my <= listBottom);
                    if (overList && g_ScrollAccum != 0.0f)
                        s_codexAugScroll -= g_ScrollAccum * ROW_H * 1.5f;
                    g_ScrollAccum = 0.0f;
                    float maxScroll = (contentH > viewH) ? (contentH - viewH) : 0.0f;
                    if (s_codexAugScroll < 0.0f) s_codexAugScroll = 0.0f;
                    if (s_codexAugScroll > maxScroll) s_codexAugScroll = maxScroll;

                    BatchFlush(); glEnable(GL_SCISSOR_TEST);
                    glScissor((GLint)LIST_X, (GLint)(sh - listBottom),
                              (GLint)LIST_W, (GLint)viewH);
                    float ry = listTop - s_codexAugScroll;
                    if (byCategory) {
                        AugListGroup prevGrp = (AugListGroup)-1;
                        for (int k = 0; k < nv; k++) {
                            int i = vis[k];
                            AugListGroup grp = AugListGroupOf(ALL_AUGS[i]);
                            if (grp != prevGrp) {
                                if (ry >= listTop - HDR_H && ry <= listBottom)
                                    g_TextS.Draw(AugListGroupLabel(grp), LIST_X, ry, 0.74f,
                                                 0.55f, 0.75f, 0.95f, 0.88f);
                                ry += HDR_H;
                                prevGrp = grp;
                            }
                            if (ry >= listTop - ROW_H && ry <= listBottom) {
                                bool seen = CodexAugSeen(i);
                                bool hv = (overList && my >= ry - 2.0f && my < ry + ROW_H - 4.0f);
                                if (hv) hoverItem = i;
                                float rr, rg, rb;
                                GetRarityColor(ALL_AUGS[i].rarity, rr, rg, rb);
                                if (hv) {
                                    BindMainShader();
                                    drawRect(LIST_X, ry - 2.0f, LIST_W, ROW_H,
                                             0.15f, 0.16f, 0.26f, 0.55f);
                                }
                                wchar_t line[128];
                                if (seen)
                                    swprintf_s(line, L"· [%ls] %ls",
                                               GetAugBadge(ALL_AUGS[i]), AugName(ALL_AUGS[i]));
                                else
                                    swprintf_s(line, L"· ???");
                                float rowSc = 0.80f;
                                while (rowSc > 0.64f &&
                                       g_TextS.Width(line, rowSc) > LIST_W - 8.0f) rowSc -= 0.03f;
                                if (seen)
                                    g_TextS.Draw(line, LIST_X + 2.0f, ry, rowSc, rr, rg, rb, 0.92f);
                                else
                                    g_TextS.Draw(line, LIST_X + 2.0f, ry, rowSc,
                                                 0.45f, 0.45f, 0.5f, 0.85f);
                            }
                            ry += ROW_H;
                        }
                    } else {
                        AugRarity prevR = (AugRarity)-1;
                        for (int k = 0; k < nv; k++) {
                            int i = vis[k];
                            AugRarity rar = ALL_AUGS[i].rarity;
                            if (rar != prevR) {
                                if (ry >= listTop - HDR_H && ry <= listBottom) {
                                    wchar_t rh[48];
                                    swprintf_s(rh, L"-- %ls --", GetRarityKR(rar));
                                    g_TextS.Draw(rh, LIST_X, ry, 0.74f,
                                                 0.55f, 0.75f, 0.95f, 0.88f);
                                }
                                ry += HDR_H;
                                prevR = rar;
                            }
                            if (ry >= listTop - ROW_H && ry <= listBottom) {
                                bool seen = CodexAugSeen(i);
                                bool hv = (overList && my >= ry - 2.0f && my < ry + ROW_H - 4.0f);
                                if (hv) hoverItem = i;
                                float rr, rg, rb;
                                GetRarityColor(ALL_AUGS[i].rarity, rr, rg, rb);
                                if (hv) {
                                    BindMainShader();
                                    drawRect(LIST_X, ry - 2.0f, LIST_W, ROW_H,
                                             0.15f, 0.16f, 0.26f, 0.55f);
                                }
                                wchar_t line[128];
                                if (seen)
                                    swprintf_s(line, L"· [%ls] %ls",
                                               GetAugBadge(ALL_AUGS[i]), AugName(ALL_AUGS[i]));
                                else
                                    swprintf_s(line, L"· ???");
                                float rowSc = 0.80f;
                                while (rowSc > 0.64f &&
                                       g_TextS.Width(line, rowSc) > LIST_W - 8.0f) rowSc -= 0.03f;
                                if (seen)
                                    g_TextS.Draw(line, LIST_X + 2.0f, ry, rowSc, rr, rg, rb, 0.92f);
                                else
                                    g_TextS.Draw(line, LIST_X + 2.0f, ry, rowSc,
                                                 0.45f, 0.45f, 0.5f, 0.85f);
                            }
                            ry += ROW_H;
                        }
                    }
                    BatchFlush(); glDisable(GL_SCISSOR_TEST);
                    if (nv == 0 && g_CodexSearchLen > 0) {
                        const wchar_t* nh[3] = {
                            L"검색 결과 없음 — 검색창을 비우거나 다른 키워드를 입력하세요",
                            L"No results — clear search or try another keyword",
                            L"結果なし — 検索をクリアするか別の語句を入力" };
                        g_TextS.Draw(nh[li], LIST_X, listTop + 24.0f, 0.82f,
                                     0.65f, 0.75f, 0.85f, 0.92f);
                    }
                    if (maxScroll > 0.0f) {
                        BindMainShader();
                        float trackX = LIST_X + LIST_W + 6.0f;
                        drawRect(trackX, listTop, 5.0f, viewH, 0.12f, 0.12f, 0.16f, 0.6f);
                        float thumbH = viewH * (viewH / contentH);
                        float thumbY = listTop + (viewH - thumbH) * (s_codexAugScroll / maxScroll);
                        drawRect(trackX, thumbY, 5.0f, thumbH, 0.5f, 0.6f, 0.8f, 0.9f);
                    }
                    // 상세(article)
                    BindMainShader();
                    if (hoverItem >= 0 && CodexAugSeen(hoverItem)) {
                        const AugDef& d = ALL_AUGS[hoverItem];
                        float rr, rg, rb; GetRarityColor(d.rarity, rr, rg, rb);
                        wchar_t hd[96];
                        swprintf_s(hd, L"[%ls] %ls", GetAugBadge(d), AugName(d));
                        g_TextL.Draw(hd, wx + 40.0f, detailY, 0.95f,
                                     std::min(1.0f, rr*1.4f+0.3f), std::min(1.0f, rg*1.4f+0.3f),
                                     std::min(1.0f, rb*1.4f+0.3f), 1.0f);
                        const wchar_t* ds = AugDesc(d);
                        g_TextS.Draw(ds, wx + 40.0f, detailY + 54.0f, 0.85f,
                                     0.85f, 0.92f, 1.0f, 0.95f);
                        if (d.rarity == AugRarity::COMBO) {
                            for (int c = 0; c < COMBO_COUNT; c++)
                                if (COMBO_DEFS[c].result == d.type) {
                                    const ComboDef& cd = COMBO_DEFS[c];
                                    int ia = AugIndexOfType(cd.reqs[0]);
                                    int ib = AugIndexOfType(cd.reqs[1]);
                                    int ic = cd.reqCount >= 3 ? AugIndexOfType(cd.reqs[2]) : -1;
                                    wchar_t rc[192];
                                    if (cd.reqCount >= 3)
                                        swprintf_s(rc, L"%ls + %ls + %ls",
                                                   ia>=0 ? AugName(ALL_AUGS[ia]) : L"?",
                                                   ib>=0 ? AugName(ALL_AUGS[ib]) : L"?",
                                                   ic>=0 ? AugName(ALL_AUGS[ic]) : L"?");
                                    else
                                        swprintf_s(rc, L"%ls + %ls",
                                                   ia>=0 ? AugName(ALL_AUGS[ia]) : L"?",
                                                   ib>=0 ? AugName(ALL_AUGS[ib]) : L"?");
                                    g_TextS.Draw(rc, wx + 40.0f, detailY + 92.0f, 0.85f,
                                                 0.1f, 0.85f, 0.8f, 0.95f);
                                    break;
                                }
                        }
                    } else if (hoverItem >= 0) {
                        const wchar_t* q[3] = { L"??? - 미발견 (획득 시 공개)",
                                                L"??? - Undiscovered (unlock by acquiring)",
                                                L"??? - 未発見 (取得で公開)" };
                        g_TextL.Draw(q[li], wx + 40.0f, detailY, 0.9f, 0.5f, 0.5f, 0.55f, 0.9f);
                    }
                } else {
                    // 보스 — LTS 4종 (VOLLEY / C2 / FORK / RITE)
                    const int COLS = 5; const float CELL = 200.0f;
                    int vis[BOSS_CODEX_COUNT], nv = 0;
                    for (int i = 0; i < BOSS_CODEX_COUNT; i++)
                        if (g_CodexSearchLen == 0 || (BossCodexSeen(i) && CodexMatch(BossCodexName(i))))
                            vis[nv++] = i;
                    float gx = wx + (WW - COLS*CELL) * 0.5f;
                    for (int k = 0; k < nv; k++) {
                        int i = vis[k];
                        float cxp = gx + (k % COLS) * CELL, cyp = gTop + (k / COLS) * CELL;
                        float cw = CELL - 16.0f, ch = CELL - 8.0f;
                        bool seen = BossCodexSeen(i);
                        bool hv = (mx >= cxp && mx < cxp+cw && my >= cyp && my < cyp+ch);
                        if (hv) hoverItem = i;
                        BindMainShader();
                        drawRect(cxp, cyp, cw, ch, hv?0.13f:0.06f, 0.10f, 0.15f, 0.95f);
                        if (seen) {
                            int pick = BossCodexPick(i);
                            glm::vec3 wc = BossDir::WarnColor(pick);
                            float ww = cw * 0.72f, wh = ch * 0.58f;
                            float wx0 = cxp + (cw - ww) * 0.5f;
                            float wy0 = cyp + (ch - wh) * 0.38f;
                            drawRect(wx0, wy0, ww, wh, 0.10f, 0.11f, 0.14f, 0.92f);
                            drawRect(wx0, wy0, ww, 18.0f, wc.r*0.75f, wc.g*0.75f, wc.b*0.75f, 0.95f);
                            drawNeonBorder(wx0, wy0, ww, wh, wc.r, wc.g, wc.b);
                            const wchar_t* nm = BossCodexName(i);
                            float nw = g_TextS.Width(nm, 0.72f);
                            if (nw > ww - 8.0f) nw = ww - 8.0f;
                            g_TextS.Draw(nm, wx0 + (ww - nw)*0.5f, cyp + ch - 28.0f, 0.72f,
                                         0.9f, 0.95f, 1.0f, 0.95f);
                        } else {
                            float ccx = cxp + cw*0.5f, ccy = cyp + ch*0.45f;
                            float qw = g_TextL.Width(L"?", 1.4f);
                            g_TextL.Draw(L"?", ccx - qw*0.5f, ccy - 24.0f, 1.4f,
                                         0.4f, 0.4f, 0.45f, 0.9f);
                        }
                    }
                    if (hoverItem >= 0 && BossCodexSeen(hoverItem)) {
                        int pick = BossCodexPick(hoverItem);
                        glm::vec3 wc = BossDir::WarnColor(pick);
                        g_TextL.Draw(BossCodexName(hoverItem), wx + 40.0f, detailY, 1.0f,
                                     wc.r, wc.g, wc.b, 1.0f);
                        g_TextS.Draw(BossCodexDesc(hoverItem), wx + 40.0f, detailY + 54.0f, 0.9f,
                                     0.85f, 0.92f, 1.0f, 0.95f);
                    } else if (hoverItem >= 0) {
                        const wchar_t* q[3] = { L"??? - 미발견 (보스 조우 시 공개)",
                                                L"??? - Undiscovered (encounter the boss)",
                                                L"??? - 未発見 (ボス遭遇で公開)" };
                        g_TextL.Draw(q[li], wx + 40.0f, detailY, 0.9f, 0.5f, 0.5f, 0.55f, 0.9f);
                    }
                }

                if (UIButton(wx + WW - 220.0f, wy + WH - 62.0f, 180.0f, 46.0f, T(StrId::BTN_BACK),
                             mx, my, lmb, g_LmbPrev)) {
                    CodexSearchClear();
                    g_GameManager.currentState = GameState::MAIN_MENU;
                }
                }   // close: g_AppOpen open guard
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
    SceneAppWindow(sw, sh, WW, WH, TutorialWinTitle(), 0.35f, 0.85f, 1.0f, wx, wy, false);
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
                SceneFlowWindow(sw, sh, FW, FH, L"career.exe", 0.55f, 0.7f, 1.0f, fx, fy, fcy);
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
                for (int j = 0; j < JOB_PLAYABLE; j++) {   // 검객/궁수(DLC 보류)는 숨김
                    float y = by + j * (BH + BG);
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
                        PickRandomWeapons(g_WeaponChoices);
                        g_GameManager.currentState = GameState::WEAPON_SELECT;
                    }
                }
                if (UIButton(fx + 32.0f, backY, 160.0f, 48.0f, T(StrId::BTN_BACK),
                             mx, my, lmb, g_LmbPrev)) {
                    g_GameManager.currentState = g_CreativeMode
                        ? GameState::CREATIVE_CONFIG : GameState::DIFFICULTY_SELECT;
                }
}

void Scene_WeaponSelect(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    GLFWwindow* window = c.window;
    const GameState st = g_GameManager.currentState;
    float& fireTimer = *c.fireTimer;
    const std::function<void()>& ResetForNewGame = c.reset;
                // 무기 확정 → 직업 시작증강/무기모드 적용 → 시작증강 or READY 로 전이
                auto finalizeLoadout = [&](int wIdx) {
                    g_Stats.baseFireInterval = g_Stats.fireInterval;
                    ApplyWeapon(g_Stats, (StartWeapon)wIdx);
                    g_CurrentWeapon = wIdx;
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
                };
                // 검객/궁수 — 총기 선택이 무의미(무기모드가 덮어씀) → 페이지 건너뛰고 기본 무기로 확정
                if (g_SelectedJob > 0 && g_SelectedJob < JOB_COUNT) {
                    int wm = JOB_DEFS[g_SelectedJob].weaponMode;
                    if (wm == 1 || wm == 2) { finalizeLoadout((int)StartWeapon::RIFLE); return; }
                }

                BindMainShader();
                const float FW = FLOW_PANEL_W, FH = FLOW_PANEL_H;
                float fx, fy, fcy;
                SceneFlowWindow(sw, sh, FW, FH, L"loadout.exe", 0.4f, 0.85f, 1.0f, fx, fy, fcy);
                const float backY = FlowBackY(fy, FH);

                const wchar_t* TIT = L"시작 무기를 선택하세요";
                float titSc = 1.2f;
                while (titSc > 0.85f && g_TextL.Width(TIT, titSc) > FW - 96.0f) titSc -= 0.05f;
                float titleY = fcy + 32.0f;
                float titH = g_TextL.Height(TIT, titSc);
                g_TextL.Draw(TIT, fx + (FW - g_TextL.Width(TIT, titSc)) * 0.5f,
                             titleY, titSc, 1, 1, 1, 0.98f);

                const float GAP = 32.0f;
                const float CARD_W = 380.0f;
                const float totalCardsW = 3.0f * CARD_W + 2.0f * GAP;
                float cardsTop = titleY + titH + 40.0f;
                float cardsBot = backY - 20.0f;
                float CARD_H = std::min(480.0f, cardsBot - cardsTop);
                float baseX = fx + (FW - totalCardsW) * 0.5f;
                float baseY = cardsTop + (cardsBot - cardsTop - CARD_H) * 0.5f;

                for (int i = 0; i < 3; i++) {
                    int idx = g_WeaponChoices[i];
                    if (idx < 0 || idx >= (int)StartWeapon::_COUNT) continue;
                    const WeaponDef& w = ALL_WEAPONS[idx];
                    float cardX = baseX + i * (CARD_W + GAP);

                    // 카드 = 큰 버튼 (라벨 비움 — 이름은 위쪽에 따로 그려 설명과 겹침 방지)
                    if (UIButton(cardX, baseY, CARD_W, CARD_H, L"",
                                 mx, my, lmb, g_LmbPrev)) {
                        finalizeLoadout(idx);
                    }

                    // 무기 이름 — 카드 상단쪽 (설명과 분리)
                    {
                        const wchar_t* nm = WeaponName(w);
                        float nsc = 1.05f;
                        while (nsc > 0.6f && g_TextL.Width(nm, nsc) > CARD_W - 24.0f) nsc -= 0.05f;
                        float nw = g_TextL.Width(nm, nsc);
                        g_TextL.Draw(nm, cardX + (CARD_W - nw) * 0.5f,
                                     baseY + CARD_H * 0.20f, nsc, 1, 1, 1, 0.98f);
                    }

                    // 설명 — 카드 안 하단에 그림 (UIButton 위에 덧그림)
                    BindMainShader();
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
                    // 설명을 카드 안에 가둔다 — 줄 많은 무기(대포 등)는 줄간격/글자 압축
                    float descTop = baseY + CARD_H * 0.44f;
                    float descBot = baseY + CARD_H - 14.0f;
                    int   nL = (int)lines.size(); if (nL < 1) nL = 1;
                    float lineH = 26.0f;
                    if (descTop + nL * lineH > descBot)
                        lineH = (descBot - descTop) / nL;
                    for (int li = 0; li < (int)lines.size(); li++) {
                        const wchar_t* s = lines[li].c_str();
                        float sc = (lineH < 24.0f) ? 0.78f : 0.85f;
                        while (sc > 0.52f &&
                               g_TextS.Width(s, sc) > CARD_W - 24.0f) sc -= 0.05f;
                        float lw = g_TextS.Width(s, sc);
                        g_TextS.Draw(s, cardX + (CARD_W - lw) * 0.5f,
                                     descTop + li * lineH, sc, 0.88f, 0.95f, 1.0f, 0.95f);
                    }
                }

                if (UIButton(fx + 32.0f, backY, 160.0f, 48.0f, T(StrId::BTN_BACK),
                             mx, my, lmb, g_LmbPrev)) {
                    g_GameManager.currentState = GameState::JOB_SELECT;
                }
}

void Scene_DifficultySelect(const SceneCtx& c) {
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
                SceneFlowWindow(sw, sh, FW, FH, L"onedow.exe", 0.30f, 0.8f, 1.0f, fx, fy, fcy);
                const float backY = FlowBackY(fy, FH);

                const wchar_t* TIT = T(StrId::DIFF_TITLE);
                float titSc = 1.35f;
                float titleY = fcy + 28.0f;
                float titH = g_TextL.Height(TIT, titSc);
                g_TextL.Draw(TIT, fx + (FW - g_TextL.Width(TIT, titSc)) * 0.5f,
                             titleY, titSc, 1.0f, 1.0f, 1.0f, 1.0f);

                struct DiffBtn { Difficulty d; StrId label; StrId desc; float r, g, b; };
                DiffBtn btns[3] = {
                    { Difficulty::EASY,   StrId::DIFF_EASY,   StrId::DIFF_EASY_DESC,
                      0.3f, 0.85f, 0.4f },
                    { Difficulty::NORMAL, StrId::DIFF_NORMAL, StrId::DIFF_NORMAL_DESC,
                      0.4f, 0.6f, 1.0f },
                    { Difficulty::HARD,   StrId::DIFF_HARD,   StrId::DIFF_HARD_DESC,
                      1.0f, 0.4f, 0.4f },
                };

                const float BW = 680.0f, BH = 96.0f, BG = 28.0f;
                float bx = fx + (FW - BW) * 0.5f;
                float blockH = 3.0f * BH + 2.0f * BG;
                float by = titleY + titH + 40.0f +
                           (FlowContentH(FH) - titH - 40.0f - blockH) * 0.32f;

                for (int i = 0; i < 3; i++) {
                    float y = by + i * (BH + BG);
                    bool sel = (g_Difficulty == btns[i].d);
                    if (UIButton(bx, y, BW, BH, L"", mx, my, lmb, g_LmbPrev, sel)) {
                        g_Difficulty = btns[i].d;
                        if (g_CreativeMode) {
                            g_GameManager.currentState = GameState::CREATIVE_CONFIG;
                        } else {
                            g_GameManager.currentState = GameState::JOB_SELECT;
                        }
                    }
                    const wchar_t* lbl = T(btns[i].label);
                    float lsc = 0.95f;
                    while (lsc > 0.65f && g_TextL.Width(lbl, lsc) > BW - 24.0f) lsc -= 0.05f;
                    float lw = g_TextL.Width(lbl, lsc);
                    g_TextL.Draw(lbl, bx + (BW - lw) * 0.5f, y + 16.0f, lsc, 1, 1, 1, 0.98f);
                    const wchar_t* desc = T(btns[i].desc);
                    float dsc = 0.82f;
                    while (dsc > 0.58f && g_TextS.Width(desc, dsc) > BW - 24.0f) dsc -= 0.04f;
                    float dw = g_TextS.Width(desc, dsc);
                    g_TextS.Draw(desc, bx + (BW - dw) * 0.5f, y + BH - 34.0f, dsc,
                                 btns[i].r, btns[i].g, btns[i].b, 0.88f);
                }

                if (g_DevUnlocked) {
                    const wchar_t* CLBL = g_CreativeMode
                        ? T(StrId::CREATIVE_ON)
                        : T(StrId::CREATIVE_OFF);
                    float cby = by + blockH + 28.0f;
                    if (cby + 72.0f > backY - 12.0f) cby = backY - 84.0f;
                    float cbw = BW, cbh = 72.0f;
                    float cbx = bx;
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

                if (UIButton(fx + 32.0f, backY, 160.0f, 48.0f, T(StrId::BTN_BACK),
                             mx, my, lmb, g_LmbPrev)) {
                    g_GameManager.currentState = GameState::MAIN_MENU;
                }
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
                BindMainShader();
                drawRect(0, 0, sw, sh, 0.0f, 0.0f, 0.0f, 0.42f);

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
                BossOpt bOpts[7] = {
                    {L"None",-1}, {L"UNKNOWN.sys*",1}, {L"VOLLEY.sys",2}, {L"GLITCH.exe*",4},
                    {L"C2_RELAY",7}, {L"FORK.worm",8}, {L"RITE.CORE",9}
                };
                const float BBW = 112.0f;
                for (int i = 0; i < 7; i++) {
                    int col = i % 4, row = i / 4;
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
                    g_GameManager.currentState = GameState::JOB_SELECT;
                }

                if (UIButton(40.0f, footY, 160.0f, 48.0f, T(StrId::BTN_BACK),
                             mx, my, lmb, g_LmbPrev)) {
                    g_GameManager.currentState = GameState::DIFFICULTY_SELECT;
                }
}

void Scene_Settings(const SceneCtx& c) {
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
                SceneAppWindow(sw, sh, WW, WH, L"config.sys", 0.70f, 0.75f, 0.88f, wx, wy,
                               settingsOverlay);
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
                    drawRect(0, 0, sw, sh, 0.0f, 0.0f, 0.0f, 0.78f);   // 딤
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
                drawRect(0, 0, sw, sh, 0.02f, 0.02f, 0.06f, 0.86f);
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
                drawRect(0, 0, sw, sh, 0.02f, 0.02f, 0.06f, 0.88f * ge);
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
                // ── 카드 레이아웃 (GameManager::Render 와 동기화) ──
                const int  nCards  = 3;
                const float CARD_W = 280.0f;
                const float CARD_H = 400.0f;
                const float GAP    = 48.0f;
                const float TOTAL_W = (float)nCards * CARD_W + (float)(nCards-1) * GAP;
                float baseX = (sw - TOTAL_W) * 0.5f;
                float baseY = (sh - CARD_H)  * 0.4f;

                // 타이틀
                const wchar_t* TIT = (st == GameState::DEBUFF_SELECT)
                                     ? T(StrId::CHOOSE_DEBUFF)
                                     : T(StrId::CHOOSE_AUG);
                g_TextL.Draw(TIT, CenterTextX(sw, g_TextL, TIT, 1.0f), baseY - 56.0f, 1.0f,
                             1,1,1,0.95f);

                // 키 힌트
                const wchar_t* HINT = (g_HoveredAug < 0)
                                      ? T(StrId::KEY_HINT_NO_HOVER)
                                      : T(StrId::KEY_HINT_HOVER);
                g_TextS.Draw(HINT, CenterTextX(sw, g_TextS, HINT, 0.85f),
                             baseY + CARD_H + 200.0f,
                             0.85f, 0.75f,0.75f,0.75f,0.8f);

                // 각 카드: 등급 라벨 + 이름
                static const wchar_t* KEY_LABELS[3] = {L"[ 1 ]", L"[ 2 ]", L"[ 3 ]"};
                for (int i = 0; i < nCards; i++) {
                    float cardX = baseX + i * (CARD_W + GAP);
                    float yOff  = (g_HoveredAug == i) ? -16.0f : 0.0f;

                    const AugDef& def = ALL_AUGS[g_GameManager.augChoices[i]];
                    const wchar_t* cardName = AugName(def);
                    float tr, tg, tb;
                    GetRarityColor(def.rarity, tr, tg, tb);
                    tr = std::min(1.0f, tr * 1.4f + 0.25f);
                    tg = std::min(1.0f, tg * 1.4f + 0.25f);
                    tb = std::min(1.0f, tb * 1.4f + 0.25f);
                    const wchar_t* topLabel = GetAugBadge(def);
                    GLuint icon = IconFor(def.type);
                    if (icon) {
                        float isz = 144.0f;
                        DrawIcon(icon, cardX + (CARD_W - isz) * 0.5f,
                                 baseY + yOff + CARD_H * 0.10f, isz, isz,
                                 1.0f, 1.0f, 1.0f, 0.97f);
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
                                 baseY + yOff + CARD_H * 0.54f, nameSc,
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
                    const wchar_t* hDesc;
                    float hr, hg, hb;
                    int hIdx = g_GameManager.augChoices[g_HoveredAug];
                    if (hIdx < 0) hIdx = 0;
                    const AugDef& hDef = ALL_AUGS[hIdx];
                    hDesc = AugDesc(hDef);
                    GetRarityColor(hDef.rarity, hr, hg, hb);
                    float boxY = baseY + CARD_H + 24.0f;
                    float boxW = TOTAL_W;
                    float boxH = 190.0f;
                    float boxX = (sw - boxW) * 0.5f;

                    // 박스 배경 — DEBUFF_SELECT 는 전체 적갈 오버레이(0.62)와 겹치면
                    // 0.85 알파가 이중으로 쌓여 새까맣게 보임 → 더 투명하게
                    const bool debuffPick = (st == GameState::DEBUFF_SELECT);
                    float boxBgA = debuffPick ? 0.38f : 0.72f;
                    float boxBr = debuffPick ? 0.12f : 0.03f;
                    float boxBg = debuffPick ? 0.04f : 0.03f;
                    float boxBb = debuffPick ? 0.05f : 0.05f;
                    drawRect(boxX, boxY, boxW, boxH, boxBr, boxBg, boxBb, boxBgA);
                    if (debuffPick) {
                        const float bt = 1.0f;
                        drawRect(boxX, boxY, boxW, bt, hr, hg, hb, 0.55f);
                        drawRect(boxX, boxY + boxH - bt, boxW, bt, hr, hg, hb, 0.55f);
                        drawRect(boxX, boxY, bt, boxH, hr, hg, hb, 0.55f);
                        drawRect(boxX + boxW - bt, boxY, bt, boxH, hr, hg, hb, 0.55f);
                    }

                    // 상단 띠
                    drawRect(boxX, boxY, boxW, 4.0f, hr, hg, hb, debuffPick ? 0.88f : 1.0f);

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
                    // 모든 줄을 같은 폰트 크기로 — 가장 긴 줄 기준 한 번만 스케일 결정(일정한 크기)
                    float maxw = 1.0f;
                    for (auto& ln : lines) {
                        float w = g_TextS.Width(ln.c_str(), 1.0f);
                        if (w > maxw) maxw = w;
                    }
                    float sc = 0.95f;
                    if (maxw * sc > boxW - 40.0f) sc = (boxW - 40.0f) / maxw;
                    if (sc < 0.6f) sc = 0.6f;
                    float lineH = 34.0f * sc;
                    float startY = boxY + 22.0f + (boxH - 22.0f - lineH * n) * 0.5f;
                    for (int li = 0; li < n; li++) {
                        const wchar_t* s = lines[li].c_str();
                        float lw = g_TextS.Width(s, sc);
                        g_TextS.Draw(s, boxX + (boxW - lw) * 0.5f,
                                     startY + lineH * (float)li, sc,
                                     1.0f, 1.0f, 1.0f, 0.95f);
                    }
                }
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
        st = g_CreativeMode ? GS::CREATIVE_CONFIG : GS::DIFFICULTY_SELECT;
        return true;
    case GS::WEAPON_SELECT:
        st = GS::JOB_SELECT;
        return true;
    case GS::DIFFICULTY_SELECT:
        st = GS::MAIN_MENU;
        return true;
    case GS::CREATIVE_CONFIG:
        st = GS::DIFFICULTY_SELECT;
        return true;
    case GS::SETTINGS:
        SaveGame();
        st = g_SettingsReturnTo;
        return true;
    default:
        return false;
    }
}
