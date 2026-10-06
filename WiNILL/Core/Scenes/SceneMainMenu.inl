// Included by SceneMenus.cpp to keep shared menu state and helpers private.

void Scene_MainMenu(const SceneCtx& c) {
    // The main menu owns no page-entry texture fade. Inline pages set their
    // own reveal immediately before drawing, so returning here can never
    // inherit a partially faded CircleTexture from the previous page.
    SetSceneTextureReveal(1.0f);
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    GLFWwindow* window = c.window;
    const float uiScale = UiScale(sw, sh);
    const GameState st = g_GameManager.currentState; (void)st;
    (void)*c.fireTimer;
    int li2 = LangIndex();
    bool booting = (g_BootAnim > 0.0f);
    float uiA = 1.0f - g_FadeAlpha; if (uiA < 0.0f) uiA = 0.0f;

    // Inline panels own their local reveal; keep the shared background stable
    // during the handoff so the old fade cannot create a brightness flash.
    DrawMenuBackground(sw, sh, delta, 1.0f);
    if (s_MainMenuArmoryPanel) {
        Scene_Shop(c);
        return;
    }
    if (s_MainMenuCodexPanel) {
        Scene_Codex(c);
        return;
    }
    if (s_MainMenuSettingsPanel) {
        Scene_SettingsInline(c);
        return;
    }
    if (s_MainMenuRunConfigPanel) {
        Scene_RunConfigInline(c);
        return;
    }

    // Intro animation (original)
    static float s_introT = 0.0f;
    static int   s_menuSelect = -1;
    static float s_menuExitT  = 0.0f;
    static float kHoverT[5] = {};
    constexpr float kTitleDur  = 0.55f;
    constexpr float kBtnDur    = 0.45f;
    constexpr float kBtnStag   = 0.14f;
    constexpr float kTextStart = kTitleDur + 4.0f * kBtnStag + kBtnDur;
    constexpr float kTextDur   = 0.35f;
    constexpr float kIntroDone = kTextStart + kTextDur;

    if (g_MainMenuEntryT <= 0.0f) {
        s_introT = 0.0f;
        s_menuSelect = -1;
        s_menuExitT = 0.0f;
        for (int i = 0; i < 5; ++i) kHoverT[i] = 0.0f;
        g_MainMenuEntryT = 0.001f;
    } else {
        g_MainMenuEntryT += delta;
    }
    if (s_MainMenuResumeFromPanel) {
        s_menuSelect = -1;
        s_menuExitT = 0.0f;
        for (int i = 0; i < 5; ++i) kHoverT[i] = 0.0f;
        s_MainMenuResumeFromPanel = false;
    }
    const bool exitActive = (s_menuSelect >= 0);
    const float menuExitDuration = s_menuSelect == 4
        ? 0.52f : kOutgameTransitionDuration;
    if (exitActive) {
        s_menuExitT += std::min(delta, 0.05f);
        if (s_menuExitT > 0.70f) s_menuExitT = 0.70f;
    }

    bool introWasActive = (s_introT < kIntroDone);
    if (!booting && lmb && !g_LmbPrev && introWasActive)
        s_introT = kIntroDone;
    if (!booting && s_introT < kIntroDone)
        s_introT = std::min(s_introT + delta, kIntroDone);
    const bool introActive = (s_introT < kIntroDone);

    float titlePhA = std::min(s_introT / kTitleDur, 1.0f);
    const float titleA = Smoothstep(titlePhA) * uiA;
    float textPhA  = (s_introT > kTextStart)
        ? std::min((s_introT - kTextStart) / kTextDur, 1.0f) : 0.0f;
    const float textA = Smoothstep(textPhA) * uiA;

    // Button layout constants
    struct SBtnDef {
        float ir, ig, ib;
    };
    static const SBtnDef kBtns[] = {
        { 0.18f, 0.62f, 0.96f },
        { 0.18f, 0.62f, 0.96f },
        { 0.18f, 0.62f, 0.96f },
        { 0.18f, 0.62f, 0.96f },
        { 0.18f, 0.62f, 0.96f },
    };
    // Lobby buttons use one localized route label; route IDs must not leak
    // through as English-only text when Korean or Japanese is selected.
    const int   kBtnCount = 5;
    const float BW = std::min(560.0f * uiScale,
                              std::max(420.0f * uiScale, sw * 0.34f));
    const float BH = 70.0f * uiScale;
    const float BGAP = 10.0f * uiScale;
    float totalBH = kBtnCount * BH + (kBtnCount - 1) * BGAP;
    float btnX0   = std::max(58.0f * uiScale, sw * 0.075f);
    // The lobby command rail belongs to the lower-left corner, matching the
    // navigation language used by the other pages. Keep a responsive bottom
    // margin, but never let the rail climb into the logo on short windows.
    float btnY0 = MainMenuButtonRailStartY(sh, uiScale);

    const float now = (float)glfwGetTime();
    const float menuFieldA = Smoothstep(LogoClamp01((s_introT - 0.10f) / 0.62f)) * uiA;
    DrawMainMenuAstralVeil(sw, sh, btnX0, btnY0, BW, totalBH,
                           kHoverT, menuFieldA);
    DrawSharedMenuDim(sw, sh, btnX0, btnY0, BW, totalBH,
                      Smoothstep(LogoClamp01((s_introT - 0.12f) / 0.58f)) * uiA);
    {
        const UiThemePalette& theme = GetUiTheme();
        const float anchorX = btnX0 - 36.0f;
        const float anchorY = std::max(22.0f, MainLogoTop(sh) - 20.0f);
        const float anchorH = std::min(sh - anchorY - 26.0f, totalBH + 170.0f);
        const float anchorA = Smoothstep(LogoClamp01((s_introT - 0.28f) / 0.48f)) * uiA;
        BindMainShader();
        drawRect(anchorX, anchorY, 1.2f, anchorH,
                 theme.Frame.r, theme.Frame.g, theme.Frame.b, 0.17f * anchorA);
        drawDiamond(anchorX + 0.6f, anchorY + 4.0f, 2.8f,
                    theme.Frame.r, theme.Frame.g, theme.Frame.b, 0.22f * anchorA);
        drawDiamond(anchorX + 0.6f, anchorY + anchorH - 4.0f, 2.8f,
                    theme.Frame.r, theme.Frame.g, theme.Frame.b, 0.18f * anchorA);
    }
    DrawMainOnedowLogo(sw, sh, titleA, titlePhA, sw * 0.30f, 0.82f);
    for (int i = 0; i < kBtnCount; i++) {
        float baseBx = btnX0;
        float baseBy = btnY0 + (float)i * (BH + BGAP);
        float rawBtnPh = (s_introT - kTitleDur - (float)i * kBtnStag) / kBtnDur;
        rawBtnPh = std::max(0.0f, std::min(rawBtnPh, 1.0f));
        float reveal = Smoothstep(rawBtnPh);
        bool selected = (s_menuSelect == i);
        float exitP = SceneTransitionEase(s_menuExitT / menuExitDuration);
        const bool buttonReady = !booting && !introActive && !introWasActive
                              && !exitActive && g_FadeDir == 0;
        const float priorBx = baseBx + (1.0f - reveal) * 34.0f - 10.0f * kHoverT[i];
        const float priorBy = baseBy + (exitActive && !selected ? exitP * 28.0f : 0.0f);
        bool hov = UpdatePanelButtonHover(kHoverT[i], buttonReady,
                                          mx, my,
                                          priorBx, priorBy, BW, BH,
                                          delta, 4.0f * uiScale);
        float t = kHoverT[i];
        float rowA = reveal * uiA;
        if (exitActive && !selected) rowA *= (1.0f - exitP * 0.86f);
        float slide = (1.0f - reveal) * 34.0f;
        float bx = baseBx + slide - 10.0f * t;
        float by = baseBy + (exitActive && !selected ? exitP * 28.0f : 0.0f);
        float ar = kBtns[i].ir, ag = kBtns[i].ig, ab = kBtns[i].ib;
        if (i == 4) {
            // Keep the idle command in the shared family color. The warmer
            // warning hue appears only while SHUTDOWN is actively targeted.
            const float warnT = std::max(t, selected ? exitP : 0.0f);
            ar += (0.96f - ar) * warnT;
            ag += (0.72f - ag) * warnT;
            ab += (0.28f - ab) * warnT;
        }
        float selectPulse = selected ? (0.50f + 0.50f * sinf(now * 18.0f)) * exitP : 0.0f;
        const wchar_t* route = MainMenuRouteLabel(li2, i);
        const wchar_t* routeVariants[] = {
            MainMenuRouteLabel(0, i),
            MainMenuRouteLabel(1, i),
            MainMenuRouteLabel(2, i),
        };
        DrawPanelButton(route, bx, by, BW, BH,
                        ar, ag, ab, rowA, t, selected, selectPulse,
                        now + (float)i * 0.17f,
                        uiScale, false, PanelButtonSlideSide::Left, false,
                        routeVariants, 3);

        if (hov && lmb && !g_LmbPrev) {
            s_menuSelect = i;
            s_menuExitT = 0.0f;
        }
    }

    {
        int morphIdx = -1;
        float bestHover = 0.18f;
        for (int i = 0; i < kBtnCount; ++i) {
            if (kHoverT[i] > bestHover) { bestHover = kHoverT[i]; morphIdx = i; }
        }
        if (s_menuSelect >= 0) morphIdx = s_menuSelect;

        UpdateMainMenuConstellationMotion(morphIdx, delta);
        const int focusIdx = morphIdx >= 0 ? morphIdx : 0;
        const float palette[5][3] = {
            {0.18f, 0.62f, 0.96f}, {0.08f, 0.70f, 0.82f},
            {0.70f, 0.62f, 1.00f}, {0.96f, 0.78f, 0.34f},
            {0.52f, 0.64f, 0.78f}
        };
        const int colorIdx = focusIdx % 5;
        const float colR = palette[colorIdx][0];
        const float colG = palette[colorIdx][1];
        const float colB = palette[colorIdx][2];
        // Preserve the shared cyan family while allowing the focused command
        // to tint the constellation's active geometry.
        const float paletteMix = 0.12f + 0.20f * bestHover;
        const float lineR = 0.18f + (colR - 0.18f) * paletteMix;
        const float lineG = 0.62f + (colG - 0.62f) * paletteMix;
        const float lineB = 1.00f + (colB - 1.00f) * paletteMix;
        const float canvasReveal = Smoothstep(LogoClamp01((s_introT - kTitleDur - 0.18f) / 0.70f)) * uiA;
        const float collapse = exitActive
            ? SceneTransitionEase(s_menuExitT / menuExitDuration) : 0.0f;
        const float canvasLeft = btnX0 + BW + 40.0f;
        const float canvasRight = std::max(canvasLeft + 280.0f, sw - 30.0f);
        const float availableCanvasW = canvasRight - canvasLeft;
        const float cw = std::min(sw * 0.48f, availableCanvasW);
        const float ch = std::min(sh * 0.66f, cw * 0.78f);
        const float parallaxX = ((float)mx - sw * 0.5f) * -0.012f;
        const float parallaxY = ((float)my - sh * 0.5f) * -0.009f;
        const float cx = canvasLeft + availableCanvasW * 0.5f + parallaxX;
        const float cy = sh * 0.53f + parallaxY;
        DrawRadialGradient(cx, cy, cw * 0.54f, 0.0f, 0.0f, 0.012f,
                          0.52f * canvasReveal * (1.0f - collapse * 0.35f));
        const float sphereRadius = std::min(cw, ch) * 0.345f
                                 * (1.0f + 0.045f * bestHover);
        const float sphereA = (0.72f + 0.20f * bestHover)
                            * canvasReveal * (1.0f - 0.84f * collapse);
        DrawMainMenuOrbitalInstrument(cx, cy, sphereRadius, focusIdx,
                                      kHoverT, bestHover,
                                      s_MainMenuConstellationMotion.current,
                                      s_MainMenuConstellationMotion.phase,
                                      sphereA, 1.0f,
                                      lineR, lineG, lineB);
        if (collapse > 0.01f)
            drawCircle(cx, cy, 18.0f + 52.0f * collapse, lineR, lineG, lineB, 0.080f * collapse * canvasReveal);
    }

    const float kExitDelay = (s_menuSelect == 4) ? 0.52f : 0.18f;
    if (s_menuSelect >= 0 && s_menuExitT >= kExitDelay && g_FadeDir == 0) {
        int selectedMenu = s_menuSelect;
        const float handoffT = s_menuExitT;
        s_menuSelect = -1;
        s_menuExitT = 0.0f;
        switch (selectedMenu) {
        case 0:
            ResetRunConfigUi(handoffT);
            s_MainMenuRunConfigPanel = true;
            break;
        case 1:
            ResetShopUi(handoffT);
            s_MainMenuArmoryPanel = true;
            break;
        case 2:
            ResetCodexUi(handoffT);
            s_MainMenuCodexPanel = true;
            break;
        case 3:
            ResetSettingsUi(handoffT);
            s_MainMenuSettingsPanel = true;
            break;
        case 4:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        }
    }



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
                    if (ease > 0.9f) {
                        // 부팅 로그 — 진행도에 따라 한 줄씩 나타남
                        static const wchar_t* LOG[3][6] = {
                            {
                                L"> 모듈 마운트 중 ...",
                                L"> 애셋 로드             [ OK ]",
                                L"> 렌더러 초기화         [ OK ]",
                                L"> 별자리 격자 연결     [ OK ]",
                                L"> 저장 데이터 검증      [ OK ]",
                                L"> 준비 완료."
                            },
                            {
                                L"> mounting modules ...",
                                L"> loading assets        [ OK ]",
                                L"> init renderer         [ OK ]",
                                L"> linking star lattice    [ OK ]",
                                L"> verify save data      [ OK ]",
                                L"> ready."
                            },
                            {
                                L"> モジュールをマウント ...",
                                L"> アセット読込          [ OK ]",
                                L"> レンダラー初期化      [ OK ]",
                                L"> 星座ラティス接続       [ OK ]",
                                L"> セーブデータ検証      [ OK ]",
                                L"> 準備完了。"
                            }
                        };
                        int shown = (int)(prog * 6.5f); if (shown > 6) shown = 6;
                        for (int i = 0; i < shown; i++) {
                            bool last = (i == 5);
                            DrawShadowedText(g_TextS, LOG[li2][i],
                                             wx + 22.0f, wy + 14.0f + i * 24.0f, 0.6f,
                                             last ? ag : 0.65f, last ? 1.0f : 0.78f,
                                             last ? ag : 0.7f, 0.95f, 0.70f);
                        }
                        // 진행 바 (창 하단)
                        float barW = cw - 44.0f, barX = wx + 22.0f, barY = wy + chh - 30.0f;
                        BindMainShader();
                        drawRect(barX, barY, barW, 14.0f, 0.12f, 0.14f, 0.20f, 1.0f);
                        drawRect(barX, barY, barW * prog, 14.0f, ar, ag, ab, 1.0f);
                        wchar_t pct[16]; swprintf_s(pct, L"%d%%", (int)(prog * 100.0f));
                        DrawShadowedText(g_TextS, pct,
                                         barX + barW - 44.0f, barY - 22.0f, 0.6f,
                                         0.8f, 0.9f, 1.0f, 1.0f, 0.70f);
                    }
                    if (g_BootAnim <= 0.0f) {
                        g_BootAnim = 0.0f;
                        g_GameManager.currentState = g_BootTarget;
                    }
                }
}
