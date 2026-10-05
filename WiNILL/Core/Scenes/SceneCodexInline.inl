// Included by SceneMenus.cpp to keep shared menu state and helpers private.

static void Scene_CodexInline(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    (void)c.fireTimer; (void)c.reset;

    static constexpr int CAT_COUNT = 2;
    static int   s_cat = 0;
    static float s_rootHover[CAT_COUNT + 1] = {};
    static int   s_sel[CAT_COUNT] = { -1, -1 };
    static float s_scroll[CAT_COUNT] = {};
    static bool  s_backExit = false;
    static float s_backOutT = 0.0f;
    static bool  s_prevEsc = false;
    static int   s_prevCat = -1;
    static int   s_prevSel = -9999;
    static float s_decryptT = 1.0f;
    static float s_itemHover[512] = {};
    static float s_itemReveal[512] = {};
    static float s_orbitAngle[CAT_COUNT] = {};
    static float s_orbitTarget[CAT_COUNT] = {};
    static float s_orbitVelocity[CAT_COUNT] = {};
    static float s_displaySlot[CAT_COUNT] = {};
    static float s_displayTarget[CAT_COUNT] = {};
    static int   s_navHoldDir = 0;
    static float s_navHoldT = 0.0f;
    static float s_navRepeatT = 0.0f;
    static bool  s_dragging = false;
    static bool  s_dragMoved = false;
    static float s_dragAccum = 0.0f;
    static double s_dragLastY = 0.0;
    static bool  s_prevSearchBackspace = false;
    static float s_searchBackspaceT = 0.0f;
    static bool  s_searchBackspaceRepeating = false;
    static float s_searchHover = 0.0f;
    static bool  s_searchFocused = false;
    static std::wstring s_prevSearch;

    g_CodexSearchInputEnabled = s_searchFocused && !s_backExit;

    float dt = delta; if (dt > 0.05f) dt = 0.05f;
    g_CodexEntryT += dt;
    if (g_CodexEntryT > 1.0f) g_CodexEntryT = 1.0f;
    const float now = (float)glfwGetTime();

    const bool rawRmb = c.window && glfwGetMouseButton(c.window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    const bool rmbClick = rawRmb && !g_RmbPrev;
    const bool rawEsc = c.window && glfwGetKey(c.window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    if (c.inputFocusChanged) s_prevEsc = rawEsc;
    const bool escClick = rawEsc && !s_prevEsc;
    s_prevEsc = rawEsc;

    const bool rawSearchBackspace = c.window &&
        glfwGetKey(c.window, GLFW_KEY_BACKSPACE) == GLFW_PRESS;
    if (c.inputFocusChanged) {
        s_navHoldDir = 0;
        s_navHoldT = 0.0f;
        s_navRepeatT = 0.0f;
        s_dragging = false;
        s_dragAccum = 0.0f;
        s_prevSearchBackspace = rawSearchBackspace;
        s_searchBackspaceT = 0.0f;
        s_searchBackspaceRepeating = false;
    }
    if (!s_searchFocused || !rawSearchBackspace) {
        s_searchBackspaceT = 0.0f;
        s_searchBackspaceRepeating = false;
    } else if (!s_prevSearchBackspace) {
        if (g_CodexSearchLen > 0)
            g_CodexSearch[--g_CodexSearchLen] = 0;
        s_searchBackspaceT = 0.0f;
        s_searchBackspaceRepeating = false;
    } else if (g_CodexSearchLen > 0) {
        s_searchBackspaceT += dt;
        const float repeatDelay = s_searchBackspaceRepeating ? 0.055f : 0.30f;
        if (s_searchBackspaceT >= repeatDelay) {
            g_CodexSearch[--g_CodexSearchLen] = 0;
            s_searchBackspaceT = 0.0f;
            s_searchBackspaceRepeating = true;
        }
    }
    s_prevSearchBackspace = rawSearchBackspace;

    const std::wstring currentSearch(g_CodexSearch);
    if (currentSearch != s_prevSearch) {
        s_prevSearch = currentSearch;
        for (int i = 0; i < CAT_COUNT; ++i) {
            s_displaySlot[i] = 0.0f;
            s_displayTarget[i] = 0.0f;
        }
        s_prevSel = -9999;
        s_decryptT = 0.0f;
    }

    const bool inputReady = !s_backExit && g_CodexEntryT >= 0.55f;
    auto beginBack = [&]() {
        if (!inputReady) return;
        s_backExit = true;
        s_backOutT = 0.0f;
        s_CodexBackRequested = false;
    };
    if (s_CodexBackRequested) {
        if (inputReady) beginBack();
        else s_CodexBackRequested = false;
    }
    if (rmbClick || escClick) beginBack();

    bool finishBackAfterRender = false;
    if (s_backExit) {
        s_backOutT += dt;
        if (s_backOutT >= 0.42f) {
            s_backOutT = 0.42f;
            finishBackAfterRender = true;
        }
    }

    const float entryOldOut = Smoothstep(LogoClamp01(g_CodexEntryT / 0.35f));
    const float entryTreeIn = Smoothstep(LogoClamp01((g_CodexEntryT - 0.20f) / 0.35f));
    const float entryDetailIn = Smoothstep(LogoClamp01((g_CodexEntryT - 0.34f) / 0.30f));
    const float backP = s_backExit ? Smoothstep(LogoClamp01(s_backOutT / 0.42f)) : 0.0f;
    const float oldMenuA = s_backExit ? backP : (1.0f - entryOldOut);
    const float wake = s_backExit ? (1.0f - backP) : entryTreeIn;
    const float rightWake = s_backExit ? (1.0f - backP) : entryDetailIn;

    int li = LangIndex(); if (li < 0 || li >= 3) li = 0;
    const int nli = (li == 0) ? 0 : 1;

    // All archive CircleTextures share one page-level reveal. It is not tied
    // to the selected record, so scrolling/dragging only moves existing
    // nodes instead of making every texture pop back in.
    const float textureReveal = s_backExit
        ? std::max(0.0f, 1.0f - backP)
        : Smoothstep(LogoClamp01(g_CodexEntryT / 0.72f));
    SetSceneTextureReveal(textureReveal);

    const float uiS = UiScale(sw, sh);
    const float mainBW = std::min(560.0f * uiS, std::max(420.0f * uiS, sw * 0.34f));
    const float mainBH = 70.0f * uiS;
    const float mainGap = 15.0f * uiS;
    const float mainTotalH = 5.0f * mainBH + 4.0f * mainGap;
    const float mainX = std::max(58.0f * uiS, sw * 0.075f);
    const float mainY = MainMenuCommandStartY(sh, uiS);
    const float astralY = mainY + 2.0f * (mainBH + mainGap);
    const float astralMidY = astralY + mainBH * 0.5f;

    const float rootX = mainX + (s_backExit ? backP * 180.0f * uiS : -(1.0f - wake) * 82.0f * uiS);
    const float rootY = mainY;
    const float rootW = std::min(360.0f * uiS, mainBW * 0.70f);
    const float rootH = mainBH;
    const float rootGap = mainGap;
    // Keep the archive columns anchored to the existing canvas, while the
    // left command rail itself sits in the lower-left corner.
    const float rootTotalH = rootH * (CAT_COUNT + 1) + rootGap * CAT_COUNT;
    const float railY = sh - rootTotalH - 48.0f * uiS;
    const InlineThreeColumnLayout columns = BuildInlineThreeColumnLayout(
        sw, sh, uiS, rootX, rootY, rootW, rootH);
    // Match the RUN_CONFIG canvas bounds so both inline screens carry the
    // same visual weight instead of growing from the lower root-menu anchor.
    const float archivePanelY = std::max(74.0f, sh * 0.12f);
    const float archivePanelBottom = sh - std::max(88.0f, 104.0f * uiS);
    const float archivePanelH = std::max(260.0f * uiS,
                                         archivePanelBottom - archivePanelY);
    const float depth2X = columns.contentX;
    const float depth2Y = archivePanelY;
    const float depth2W = columns.contentW;
    const float depth2H = archivePanelH;
    const float rightX = columns.detailX;
    const float rightW = columns.detailW;
    const float rightY = archivePanelY;
    const float rightH = archivePanelH;
    const float archiveHeaderH = 68.0f * uiS;
    const float listX = depth2X;
    const float listY = depth2Y + archiveHeaderH;
    const float listW = depth2W;
    const float listH = std::max(180.0f * uiS, depth2H - archiveHeaderH - 10.0f * uiS);
    const float archiveX = depth2X - 18.0f * uiS;
    const float archiveY = depth2Y - 4.0f * uiS;
    const float archiveRight = rightX + rightW;
    const float archiveW = archiveRight - archiveX;
    const float archiveH = depth2H + 8.0f * uiS;

    DrawSharedMenuDim(sw, sh, mainX, railY, mainBW, mainTotalH,
                      std::max(oldMenuA, wake), true);

    struct RootDef { const wchar_t* label[3]; float r, g, b; };
    static const RootDef ROOTS[CAT_COUNT + 1] = {
        { { L"\uAD00\uCE21\uCCB4", L"ENTITIES", L"\u89B3\u6E2C\u5BFE\u8C61" },
          0.48f, 0.82f, 1.00f },
        { { L"\uBAA8\uB4C8", L"MODULES", L"\u30E2\u30B8\u30E5\u30FC\u30EB" },
          0.62f, 0.52f, 1.00f },
        { { L"\uB4A4\uB85C", L"BACK", L"\u623B\u308B" },
          0.42f, 0.62f, 0.78f },
    };
    const int rootLanguage = std::max(0, std::min(2, LangIndex()));
    const RootDef& curRoot = ROOTS[s_cat];

    // Keep the archive on the main-menu canvas.  The hierarchy is carried by
    // the root rail, orbital chart and typography instead of a window-shaped
    // data plate.
    BindMainShader();

    // Main-menu expansion ghost.
    const float oldOut = 1.0f - oldMenuA;
    const float returnLogoA = s_backExit ? (0.42f + 0.58f * backP) : 0.42f;
    DrawMainOnedowLogo(sw, sh,
                       returnLogoA * oldMenuA,
                       LogoClamp01(oldMenuA + 0.22f * wake),
                       sw * 0.30f - 160.0f * oldOut,
                       0.82f);
    {
        const int ghostLang = LangIndex();
        const float ghostSlide = 250.0f * oldOut;
        const float ghostRailY = MainMenuButtonRailStartY(sh, uiS);
        const float ghostGap = 10.0f * uiS;
        const float anchorX = mainX - ghostSlide - 36.0f;
        BindMainShader();
        drawRect(anchorX, std::max(22.0f, MainLogoTop(sh) - 20.0f),
                 1.2f, mainTotalH + 170.0f,
                 0.48f, 0.82f, 1.0f, 0.12f * oldMenuA);
        for (int i = 0; i < 5; ++i) {
            const bool focus = (!s_backExit && i == 2);
            const float rowY = ghostRailY + (float)i * (mainBH + ghostGap);
            const float rowX = mainX - ghostSlide - (focus ? 0.0f : 26.0f * oldOut);
            const float rowA = (s_backExit ? 0.82f : focus ? 0.76f : 0.18f) * oldMenuA;
            const float active = focus ? 1.0f : 0.0f;
            if (focus) {
                drawRect(rowX - 18.0f, rowY + mainBH * 0.5f - 31.0f,
                         2.0f, 62.0f, 0.48f, 0.82f, 1.0f, 0.56f * oldMenuA);
                drawRect(rowX - 4.0f, rowY + mainBH * 0.50f,
                         mainBW * 0.42f, 1.1f, 0.48f, 0.82f, 1.0f, 0.12f * oldMenuA);
                drawDiamond(rowX - 17.0f, rowY + mainBH * 0.5f,
                            5.0f, 0.48f, 0.82f, 1.0f, 0.72f * oldMenuA);
            }
            const wchar_t* route = MainMenuRouteLabel(ghostLang, i);
            const wchar_t* routeVariants[] = {
                MainMenuRouteLabel(0, i),
                MainMenuRouteLabel(1, i),
                MainMenuRouteLabel(2, i),
            };
            float routeSc = UiTextScale(g_TextL, UiTextLevel::Title, uiS);
            const float availableW = std::max(1.0f, mainBW - 16.0f);
            const float availableH = std::max(1.0f, mainBH - 8.0f);
            const float widestRoute = MaxLocalizedTextWidth(
                g_TextL, routeVariants, 3, routeSc);
            const float routeH = g_TextL.Height(route, routeSc);
            const float textFit = std::min(1.0f,
                std::min(widestRoute > 0.0f ? availableW / widestRoute : 1.0f,
                         routeH > 0.0f ? availableH / routeH : 1.0f));
            routeSc *= textFit;
            const float routeY2 = rowY
                + std::max(0.0f, (mainBH - g_TextL.Height(route, routeSc)) * 0.5f);
            const float gr = s_backExit ? 1.0f : 1.0f - 0.52f * (1.0f - active);
            const float gg = s_backExit ? 1.0f : 1.0f - 0.18f * (1.0f - active);
            g_TextL.Draw(route, rowX + 8.0f, routeY2, routeSc,
                         gr, gg, 1.0f, rowA);
        }
        const float bridgeA = std::min(oldMenuA, wake);
        LogoLine(mainX - ghostSlide + mainBW * 0.42f, astralMidY,
                 rootX - 26.0f * uiS, astralMidY,
                 1.0f * uiS, 0.48f, 0.82f, 1.0f, 0.22f * bridgeA);
        drawDiamond(rootX - 26.0f * uiS, astralMidY,
                    3.3f * uiS, 0.48f, 0.82f, 1.0f, 0.46f * bridgeA);
    }

    if (s_prevCat != s_cat) {
        s_prevCat = s_cat;
        s_decryptT = 0.0f;
        for (float& hover : s_itemHover) hover = 0.0f;
        for (float& reveal : s_itemReveal) reveal = 0.0f;
    }

    // Depth 1 root.
    BindMainShader();
    drawRect(rootX - 36.0f, railY - 20.0f,
             1.2f, rootH * 4.0f + rootGap * 3.0f + 40.0f,
             0.48f, 0.82f, 1.00f, 0.13f * wake);
    for (int i = 0; i < CAT_COUNT + 1; ++i) {
        const bool isBack = (i == CAT_COUNT);
        const float tx = rootX;
        const float ty = railY + (float)i * (rootH + rootGap);
        const float reveal = s_backExit ? wake : MenuCommandReveal(g_CodexEntryT, i);
        const float priorBx = tx + (1.0f - reveal) * 34.0f - 10.0f * s_rootHover[i];
        const float hitX = priorBx - 26.0f;
        const float hitRight = std::min(priorBx + rootW + 18.0f * uiS,
                                        depth2X - 34.0f * uiS);
        const bool hov = inputReady
            && (mx >= hitX && mx < hitRight && my >= ty && my < ty + rootH);
        s_rootHover[i] = UpdateMenuCommandHover(s_rootHover[i], hov, dt);

        if (hov && lmb && !g_LmbPrev) {
            if (isBack) beginBack();
            else if (s_cat != i) s_cat = i;
        }

        const bool sel = (!isBack && s_cat == i);
        const RootDef& rd = ROOTS[i];
        const float rowA = reveal * wake;
        const float selectPulse = sel ? (0.12f + 0.12f * sinf(now * 3.3f)) : 0.0f;
        const float bx = tx + (1.0f - reveal) * 34.0f - 10.0f * s_rootHover[i];
        const float by = ty;

        DrawUnifiedMenuCommand(rd.label[rootLanguage], bx, by, rootW, rootH,
                               rd.r, rd.g, rd.b, rowA, s_rootHover[i], sel,
                               selectPulse, now + (float)i * 0.17f,
                               uiS, false, true, false,
                               rd.label, 3);
    }

    struct CItem {
        bool isGroup;
        int key;
        const wchar_t* label;
        bool seen;
        float r, g, b;
    };
    CItem items[512];
    int itemCount = 0;
    static const int kCodexMobIds[] = {
        CM_ROTOR, CM_SCOPE, CM_SWARM, CM_GENESIS, CM_GRAVIS, CM_QUASAR
    };
    auto addGroup = [&](const wchar_t* label, float r, float g, float b) {
        if (itemCount < 512) items[itemCount++] = { true, -1, label, true, r, g, b };
    };
    auto addItem = [&](int key, const wchar_t* label, bool seen,
                       const wchar_t* const* searchNames, int searchNameCount,
                       float r, float g, float b) {
        if (g_CodexSearchLen > 0 && (!seen ||
            !CodexMatchAny(searchNames, searchNameCount))) return;
        if (itemCount < 512) items[itemCount++] = { false, key, label, seen, r, g, b };
    };

    if (s_cat == 0) {
        addGroup(L"SIGNAL CLASS", 0.48f, 0.82f, 1.00f);
        for (int i : kCodexMobIds)
            addItem(i, CodexMobSeen(i) ? MobName(i) : L"???", CodexMobSeen(i),
                    MobLocalizedNames(i), LANG_COUNT,
                    0.48f, 0.82f, 1.00f);
    } else if (s_cat == 1) {
        int sorted[AUG_TOTAL], n = 0;
        for (int i = 0; i < AUG_TOTAL; ++i)
            if (!AugRemoved(ALL_AUGS[i].type)) sorted[n++] = i;
        std::sort(sorted, sorted + n, [](int a, int b) { return AugTierIndexLess(a, b); });
        AugRarity prevR = (AugRarity)-1;
        for (int k = 0; k < n; ++k) {
            int i = sorted[k];
            AugRarity rar = ALL_AUGS[i].rarity;
            float rr, rg, rb; GetRarityColor(rar, rr, rg, rb);
            if (rar != prevR) {
                addGroup(GetRarityKR(rar), rr, rg, rb);
                prevR = rar;
            }
            addItem(i, CodexAugSeen(i) ? AugName(ALL_AUGS[i]) : L"???",
                    CodexAugSeen(i), ALL_AUGS[i].locName, LANG_COUNT, rr, rg, rb);
        }
    }

    int unlockTotal[CAT_COUNT] = {
        (int)(sizeof(kCodexMobIds) / sizeof(kCodexMobIds[0])), 0
    };
    int unlockSeen[CAT_COUNT] = {};
    for (int id : kCodexMobIds)
        if (CodexMobSeen(id)) ++unlockSeen[0];
    for (int i = 0; i < AUG_TOTAL; ++i) {
        if (AugRemoved(ALL_AUGS[i].type)) continue;
        ++unlockTotal[1];
        if (CodexAugSeen(i)) ++unlockSeen[1];
    }

    int observedCount = 0;
    int recordCount = 0;
    for (int i = 0; i < itemCount; ++i) {
        if (items[i].isGroup) continue;
        ++recordCount;
        if (items[i].seen) ++observedCount;
    }
    wchar_t archivePath[96];
    wchar_t archiveProgress[64];
    swprintf_s(archivePath, L"ASTRAL ARCHIVE / %ls",
               ROOTS[s_cat].label[rootLanguage]);
    swprintf_s(archiveProgress, L"%02d / %02d OBSERVED", observedCount, recordCount);
    // The restored layout deliberately carries no page header or enclosing
    // archive panel: the root commands, orbit, and record itself are enough.
    (void)archivePath;

    // Use the open upper-left canvas for search and a compact global unlock
    // summary. Search filters only discovered records in the active category.
    auto codexText = [&](const wchar_t* kr, const wchar_t* en,
                         const wchar_t* jp = nullptr) -> const wchar_t* {
        if (li == 0) return kr;
        if (li == 2 && jp) return jp;
        return en;
    };
    const float utilityX = mainX;
    const float utilityW = std::min(sw * 0.40f, rootW + 120.0f * uiS);
    const float searchW = std::max(180.0f * uiS,
                                   std::min(420.0f * uiS,
                                            utilityW - 24.0f * uiS));
    const float utilityY = std::max(76.0f, sh * 0.11f);
    const float searchY = utilityY + 44.0f * uiS;
    const float searchH = 46.0f * uiS;
    const bool searchHover = inputReady && mx >= utilityX && mx <= utilityX + searchW
        && my >= searchY && my <= searchY + searchH;
    s_searchHover = UiApproach(s_searchHover, searchHover ? 1.0f : 0.0f, dt, 10.0f);
    const float clearW = 32.0f * uiS;
    const bool hasSearch = g_CodexSearchLen > 0;
    const bool clearHover = hasSearch && inputReady
        && mx >= utilityX + searchW - clearW && mx <= utilityX + searchW
        && my >= searchY && my <= searchY + searchH;
    const bool searchClick = (searchHover || clearHover) && lmb && !g_LmbPrev;
    if (searchClick)
        s_searchFocused = true;
    else if (lmb && !g_LmbPrev)
        s_searchFocused = false;
    if (clearHover && searchClick)
        CodexSearchClear();
    g_CodexSearchInputEnabled = s_searchFocused && !s_backExit;

    BindMainShader();
    const wchar_t* searchTitle = s_searchFocused
        ? codexText(L"\uAC80\uC0C9 / \uC785\uB825\uC911", L"SEARCH / ACTIVE", L"\u691C\u7D22\u4E2D")
        : codexText(L"\uAC80\uC0C9", L"SEARCH", L"\u691C\u7D22");
    const float searchTitleScale = UiTextScale(g_TextS, UiTextLevel::Subtitle, uiS);
    DrawShadowedText(g_TextS, searchTitle,
                     utilityX, utilityY, searchTitleScale,
                     curRoot.r, curRoot.g, curRoot.b,
                     (0.72f + 0.18f * s_searchFocused) * wake, 0.58f);
    drawRect(utilityX, searchY, searchW, searchH,
             0.010f, 0.020f, 0.038f,
             (0.24f + 0.05f * s_searchHover + 0.06f * s_searchFocused) * wake);
    drawConstellFrame(utilityX, searchY, searchW, searchH,
                      curRoot.r, curRoot.g, curRoot.b,
                      (0.28f + 0.22f * s_searchHover +
                       0.18f * s_searchFocused) * wake,
                      12.0f * uiS, 2.2f * uiS,
                      0.025f * s_searchFocused * wake);
    const float searchMidY = searchY + searchH * 0.50f;
    std::wstring searchDisplay;
    if (hasSearch)
        searchDisplay = std::wstring(g_CodexSearch);
    else if (!s_searchFocused)
        searchDisplay = std::wstring(codexText(L"\uC785\uB825\uD558\uC5EC \uAC80\uC0C9", L"TYPE TO FILTER", L"\u5165\u529B\u3057\u3066\u691C\u7D22"));
    const bool searchCaretOn = s_searchFocused && (((int)(now * 2.0f)) & 1) == 0;
    if (searchCaretOn)
        searchDisplay += L"|";
    float searchSc = UiTextScale(g_TextS, hasSearch
        ? UiTextLevel::Description : UiTextLevel::Supporting, uiS);
    const float searchMaxW = std::max(30.0f * uiS, searchW - clearW - 58.0f * uiS);
    const float searchTextW0 = g_TextS.Width(searchDisplay.c_str(), searchSc);
    if (searchTextW0 > searchMaxW && searchTextW0 > 0.0f)
        searchSc *= searchMaxW / searchTextW0;
    const float searchTextX = utilityX + 16.0f * uiS;
    const float searchTextY = searchMidY
        - g_TextS.Height(searchDisplay.c_str(), searchSc) * 0.50f;
    DrawShadowedText(g_TextS, searchDisplay.c_str(),
                     searchTextX, searchTextY,
                     searchSc,
                     hasSearch ? 0.92f : 0.42f,
                     hasSearch ? 0.96f : 0.56f,
                     hasSearch ? 1.00f : 0.70f,
                     (hasSearch ? 0.92f : 0.70f) * wake, 0.62f);
    if (hasSearch) {
        const float clearSc = 0.42f * uiS;
        DrawShadowedText(g_TextS, L"X",
                         utilityX + searchW - clearW * 0.68f,
                         searchMidY - g_TextS.Height(L"X", clearSc) * 0.50f,
                         clearSc,
                         clearHover ? 1.0f : 0.58f,
                         clearHover ? 1.0f : 0.70f,
                         clearHover ? 1.0f : 0.82f,
                         (clearHover ? 0.98f : 0.62f) * wake, 0.60f);
    }

    const float progressTitleY = searchY + searchH + 24.0f * uiS;
    const wchar_t* progressTitle = codexText(
        L"\uB3C4\uAC10 \uD574\uAE08 \uBAA9\uB85D", L"UNLOCK PROGRESS", L"\u56F3\u9451\u89E3\u653E\u9032\u6357");
    const float progressTitleScale = UiTextScale(g_TextS, UiTextLevel::Subtitle, uiS);
    DrawShadowedText(g_TextS,
                     progressTitle,
                     utilityX, progressTitleY, progressTitleScale,
                     curRoot.r, curRoot.g, curRoot.b, 0.84f * wake, 0.58f);
    const float progressBarX = utilityX;
    const float progressBarW = std::max(48.0f * uiS,
                                        std::min(320.0f * uiS,
                                                 utilityW - 16.0f * uiS));
    const float resultSc = UiTextScale(g_TextS, UiTextLevel::Supporting, uiS);
    const float resultW = g_TextS.Width(archiveProgress, resultSc);
    DrawShadowedText(g_TextS, archiveProgress,
                     progressBarX + progressBarW - resultW,
                     progressTitleY + (g_TextS.Height(progressTitle, progressTitleScale)
                                      - g_TextS.Height(archiveProgress, resultSc)) * 0.5f,
                     resultSc, 0.62f, 0.72f, 0.86f, 0.66f * wake, 0.52f);
    // Give each unlock entry its own two-line block: a large readout first,
    // then the archive line directly underneath it.  This keeps the progress
    // data legible without competing with the search field above.
    const float progressRowY = progressTitleY
        + g_TextS.Height(progressTitle, progressTitleScale) + 10.0f * uiS;
    const float progressRowStep = 68.0f * uiS;
    for (int i = 0; i < CAT_COUNT; ++i) {
        const float rowY = progressRowY + (float)i * progressRowStep;
        const RootDef& progressRoot = ROOTS[i];
        const float progressRootScale = UiTextScale(g_TextS, UiTextLevel::Subtitle, uiS);
        DrawShadowedText(g_TextS, progressRoot.label[rootLanguage],
                         utilityX, rowY, progressRootScale,
                         progressRoot.r, progressRoot.g, progressRoot.b,
                         (s_cat == i ? 0.98f : 0.72f) * wake, 0.54f);
        wchar_t progressBuf[32];
        swprintf_s(progressBuf, L"%02d / %02d", unlockSeen[i], unlockTotal[i]);
        const float progressSc = UiTextScale(g_TextS, UiTextLevel::Description, uiS);
        const float progressW = g_TextS.Width(progressBuf, progressSc);
        DrawShadowedText(g_TextS, progressBuf,
                         progressBarX + progressBarW - progressW,
                         rowY + (g_TextS.Height(progressRoot.label[rootLanguage], progressRootScale)
                               - g_TextS.Height(progressBuf, progressSc)) * 0.5f,
                         progressSc,
                         0.86f, 0.92f, 1.00f, 0.88f * wake, 0.54f);
        const float trackY = rowY + g_TextS.Height(progressRoot.label[rootLanguage], progressRootScale)
                           + 8.0f * uiS;
        const float fillW = progressBarW * (unlockTotal[i] > 0
            ? (float)unlockSeen[i] / (float)unlockTotal[i] : 0.0f);
        DrawVisibleConstellLine(progressBarX, trackY,
                                progressBarX + progressBarW, trackY,
                                0.75f * uiS, progressRoot.r, progressRoot.g, progressRoot.b,
                                0.18f * wake);
        if (fillW > 0.0f)
            DrawVisibleConstellLine(progressBarX, trackY,
                                    progressBarX + fillW, trackY,
                                    1.7f * uiS, progressRoot.r, progressRoot.g, progressRoot.b,
                                    0.76f * wake);
        DrawVisibleConstellNode(progressBarX + fillW, trackY, 2.4f * uiS,
                                progressRoot.r, progressRoot.g, progressRoot.b,
                                0.64f * wake, false);
    }

    bool selectedVisible = false;
    for (int i = 0; i < itemCount; ++i) {
        if (!items[i].isGroup && items[i].key == s_sel[s_cat]) {
            selectedVisible = true;
            break;
        }
    }
    if (!selectedVisible) s_sel[s_cat] = -1;

    if (s_sel[s_cat] < 0) {
        for (int i = 0; i < itemCount; ++i) {
            if (!items[i].isGroup) { s_sel[s_cat] = items[i].key; break; }
        }
    }
    if (s_sel[s_cat] != s_prevSel) {
        s_prevSel = s_sel[s_cat];
        s_decryptT = 0.0f;
    }
    s_decryptT = UiApproach(s_decryptT, 1.0f, dt, 8.0f);

    // Orbital record list.  Selection owns one angular target; rendering reads
    // only the damped angle so labels and miniature constellations never apply
    // a second, conflicting interpolation.
    int recordSlots[512] = {};
    int recordSlotCount = 0;
    int selectedSlot = 0;
    for (int i = 0; i < itemCount; ++i) {
        if (items[i].isGroup) continue;
        if (items[i].key == s_sel[s_cat]) selectedSlot = recordSlotCount;
        recordSlots[recordSlotCount++] = i;
    }
    if (recordSlotCount > 0 && s_sel[s_cat] < 0) {
        s_sel[s_cat] = items[recordSlots[0]].key;
        selectedSlot = 0;
    }
    if (s_displaySlot[s_cat] == 0.0f && s_displayTarget[s_cat] == 0.0f && selectedSlot != 0) {
        s_displaySlot[s_cat] = s_displayTarget[s_cat] = (float)selectedSlot;
    }

    const bool keyUpNow = c.window &&
        (glfwGetKey(c.window, GLFW_KEY_UP) == GLFW_PRESS ||
         glfwGetKey(c.window, GLFW_KEY_W) == GLFW_PRESS);
    const bool keyDownNow = c.window &&
        (glfwGetKey(c.window, GLFW_KEY_DOWN) == GLFW_PRESS ||
         glfwGetKey(c.window, GLFW_KEY_S) == GLFW_PRESS);
    int stepRequest = 0;
    const int navDir = keyUpNow == keyDownNow ? 0 : (keyUpNow ? -1 : 1);
    if (c.inputFocusChanged) {
        s_navHoldDir = navDir;
        s_navHoldT = 0.0f;
        s_navRepeatT = 0.0f;
    }
    if (inputReady && navDir != 0) {
        if (navDir != s_navHoldDir) {
            s_navHoldDir = navDir;
            s_navHoldT = 0.0f;
            s_navRepeatT = 0.0f;
            stepRequest = navDir; // immediate first move
        } else {
            s_navHoldT += dt;
            // Hold for ~0.25s, then accelerate by shrinking the repeat interval.
            if (s_navHoldT >= 0.25f) {
                s_navRepeatT += dt;
                // Cap the navigation rate so long holds never skip too fast.
                const float repeatInterval = std::max(0.05f,
                    0.26f - (s_navHoldT - 0.25f) * 0.075f);
                if (s_navRepeatT >= repeatInterval) {
                    s_navRepeatT = 0.0f;
                    stepRequest = navDir;
                }
            }
        }
    } else {
        s_navHoldDir = 0;
        s_navHoldT = 0.0f;
        s_navRepeatT = 0.0f;
    }
    // Keep the selected record's orbital node on the screen center while
    // preserving the existing arc, row spacing and list presentation.
    const float itemR = sw * 0.70f;
    const float chartCX = sw * 0.50f + itemR;
    const float chartShiftX = chartCX - sw;
    const float chartCY = sh * 0.51f;
    const float chartR = std::max(sh * 0.58f, sw * 0.44f);
    // Entity silhouettes use their gameplay footprint (Gravis includes a
    // wide gravity field), so give that category more vertical breathing room.
    const float rowStep = 245.0f * uiS;
    const bool overOrbit = inputReady &&
        mx >= depth2X - 30.0f * uiS + chartShiftX &&
        mx <= sw && my >= archivePanelY && my <= archivePanelBottom;
    if (overOrbit && lmb && !g_LmbPrev) {
        s_dragging = true;
        s_dragMoved = false;
        s_dragAccum = 0.0f;
        s_dragLastY = my;
    } else if (!lmb) {
        s_dragging = false;
        s_dragAccum = 0.0f;
    }
    if (s_dragging && lmb) {
        s_dragAccum += (float)(my - s_dragLastY);
        s_dragLastY = my;
        // Dragging is intentionally less sensitive than wheel/keyboard input:
        // require almost a full visual row before advancing one record.
        const float dragStepThreshold = rowStep * 0.88f;
        if (fabsf(s_dragAccum) >= dragStepThreshold) {
            // Drag direction follows the reversed wheel convention.
            stepRequest = s_dragAccum > 0.0f ? -1 : 1;
            s_dragAccum += s_dragAccum > 0.0f ? -dragStepThreshold : dragStepThreshold;
            s_dragMoved = true;
        }
    }
    if (overOrbit && g_ScrollAccum != 0.0f)
        stepRequest = g_ScrollAccum > 0.0f ? -1 : 1; // reversed wheel direction
    g_ScrollAccum = 0.0f;
    if (recordSlotCount > 0 && stepRequest != 0) {
        selectedSlot = (selectedSlot + stepRequest + recordSlotCount) % recordSlotCount;
        s_sel[s_cat] = items[recordSlots[selectedSlot]].key;
        s_displayTarget[s_cat] += (float)stepRequest;
    }
    {
        const float diff = s_displayTarget[s_cat] - s_displaySlot[s_cat];
        const float speed = 10.0f;
        s_displaySlot[s_cat] += diff * std::min(1.0f, dt * speed);
        if (fabsf(diff) < 0.002f) {
            s_displaySlot[s_cat] = s_displayTarget[s_cat];
        }
    }

    // The real CircleTexture pair is deliberately behind chart and copy.
    DrawConstellationDisc(chartCX - chartR * 0.12f, chartCY, chartR * 1.48f,
                          0.0f, 0.0f, 0.0f, 0.68f * rightWake);
    DrawConstellationDisc(chartCX - chartR * 0.30f, chartCY, chartR * 0.72f,
                          0.22f, 0.30f, 0.38f, 0.42f * rightWake);
    // A dedicated contrast field keeps the wrapped record rail readable on
    // white/bright gameplay backgrounds. It overlaps the main sight field so
    // the list feels embedded in the constellation instead of boxed in.
    const float listFieldCX = chartCX - itemR + 180.0f * uiS;
    DrawConstellationDisc(listFieldCX, chartCY, 700.0f * uiS,
                          0.0f, 0.0f, 0.0f, 0.22f * rightWake);
    DrawConstellationDisc(listFieldCX + 90.0f * uiS, chartCY, 510.0f * uiS,
                          curRoot.r * 0.10f, curRoot.g * 0.10f, curRoot.b * 0.12f,
                          0.075f * rightWake);
    BindMainShader();
    for (int ring = 0; ring < 4; ++ring) {
        const float rr = chartR * (0.56f + 0.145f * (float)ring);
        const int segments = 72;
        for (int j = 0; j < segments; ++j) {
            if ((j + ring * 2) % 6 == 3) continue;
            const float a0 = (float)j * 6.2831853f / (float)segments;
            const float a1 = ((float)j + 0.64f) * 6.2831853f / (float)segments;
            LogoLine(chartCX + cosf(a0) * rr, chartCY + sinf(a0) * rr,
                     chartCX + cosf(a1) * rr, chartCY + sinf(a1) * rr,
                     (ring == 3 ? 1.0f : 0.65f) * uiS,
                     curRoot.r, curRoot.g, curRoot.b,
                     (ring == 3 ? 0.16f : 0.075f) * wake);
        }
    }
    // Selection datum: no radial spokes across the chart interior.
    LogoLine(chartCX - chartR, chartCY, chartCX - chartR * 0.72f, chartCY,
             1.1f * uiS, curRoot.r, curRoot.g, curRoot.b, 0.42f * wake);
    drawDiamond(chartCX - chartR, chartCY, 4.0f * uiS,
                curRoot.r, curRoot.g, curRoot.b, 0.78f * wake);

    // Connected observation rail. Entries slide one row at a time along this
    // datum, including the wrapped first↔last transition.
    auto railXAt = [&](float row) {
        const float dy = row * rowStep;
        const float span = std::max(40.0f, itemR * itemR - dy * dy);
        return chartCX - sqrtf(span);
    };
    for (int r = -5; r < 5; ++r) {
        LogoLine(railXAt((float)r), chartCY + (float)r * rowStep,
                 railXAt((float)(r + 1)), chartCY + (float)(r + 1) * rowStep,
                 0.72f * uiS, curRoot.r, curRoot.g, curRoot.b, 0.12f * wake);
    }

    for (int slot = 0; slot < recordSlotCount; ++slot) {
        float relF = (float)slot - s_displaySlot[s_cat];
        while (relF > (float)recordSlotCount * 0.5f) relF -= (float)recordSlotCount;
        while (relF < -(float)recordSlotCount * 0.5f) relF += (float)recordSlotCount;
        const int rel = (int)std::round(relF);
        if (rel < -6 || rel > 6) continue;
        const CItem& itm = items[recordSlots[slot]];
        const float ax = railXAt(relF);
        const float ay = chartCY + relF * rowStep;
        const float distanceFade = std::max(0.0f, 1.0f - fabsf((float)rel) / 6.0f);
        const bool isSel = (rel == 0);
        const float hitW = 440.0f * uiS;
        const float hitH = 100.0f * uiS;
        const bool hov = inputReady && mx >= ax - 80.0f * uiS && mx <= ax + hitW &&
                         my >= ay - hitH * 0.5f && my <= ay + hitH * 0.5f;
        if (hov && lmb && !g_LmbPrev && !s_dragMoved) {
            float jump = (float)slot - (float)selectedSlot;
            while (jump > (float)recordSlotCount * 0.5f) jump -= (float)recordSlotCount;
            while (jump < -(float)recordSlotCount * 0.5f) jump += (float)recordSlotCount;
            s_sel[s_cat] = itm.key;
            selectedSlot = slot;
            s_displayTarget[s_cat] += jump;
        }
        const float targetFocus = isSel ? 1.0f : (hov ? 0.72f : 0.0f);
        // Per-record focus smoothing makes the horizontal datum grow/shrink
        // instead of snapping when the active record changes.
        s_itemHover[slot] = UiApproach(s_itemHover[slot], targetFocus, dt, 8.0f);
        const float targetReveal = (fabsf(relF) <= 5.0f) ? 1.0f : 0.0f;
        s_itemReveal[slot] = UiApproach(s_itemReveal[slot], targetReveal, dt, 6.5f);
        const float reveal = s_itemReveal[slot] * wake;
        if (reveal <= 0.005f) continue;
        const float active = std::max(s_itemHover[slot], distanceFade * 0.38f);
        const float miniX = ax;
        // Keep the gameplay silhouette readable, but compact enough that the
        // wider entity row spacing still shows roughly 3-4 records per page.
        const float focusScale = std::max(0.0f, std::min(1.0f, s_itemHover[slot]));
        const float miniBase = (s_cat == 1) ? 34.0f : 18.0f;
        const float miniFocus = (s_cat == 1) ? 38.0f : 24.0f;
        const float miniR = (miniBase + miniFocus * focusScale) *
                           (0.58f + 0.42f * reveal) * uiS;
        const float signalR = itm.seen ? itm.r : 0.26f;
        const float signalG = itm.seen ? itm.g : 0.32f;
        const float signalB = itm.seen ? itm.b : 0.40f;
        // Local black halo around each record keeps the constellation core
        // legible without darkening the entire archive surface.
        DrawConstellationDisc(miniX, ay, miniR * 2.80f,
                              0.0f, 0.0f, 0.0f,
                              (0.16f + 0.10f * active) * reveal);
        // Category-colored core light: every record gets a restrained glow,
        // while the selected record naturally becomes brighter via `active`.
        DrawConstellationDisc(miniX, ay, miniR * 1.14f,
                              signalR, signalG, signalB,
                              (itm.seen ? (0.055f + 0.095f * active)
                                        : (0.018f + 0.030f * active)) * reveal);
        if (s_cat == 0 && !itm.seen) {
            DrawUnknownConstellation(miniX, ay, miniR * 1.12f,
                                     now * 0.18f, (0.58f + 0.30f * active)
                                     * reveal, uiS, itm.key);
        } else {
            DrawArchiveConstellation(miniX, ay, miniR, s_cat, itm.key,
                                     now * 0.18f, signalR, signalG, signalB,
                                     (0.28f + 0.66f * active) * reveal, uiS);
        }
        const float lineLength = (250.0f + 130.0f * s_itemHover[slot]) * uiS;
        const float lineEndX = isSel
            ? std::min(ax + lineLength, rightX - 18.0f * uiS)
            : ax + lineLength;
        LogoLine(miniX + miniR * 0.68f, ay, lineEndX, ay,
                 (0.96f + 0.56f * s_itemHover[slot]) * uiS,
                 signalR, signalG, signalB,
                 (itm.seen ? 0.12f : 0.06f
                     + 0.12f * active) * reveal);
        DrawVisibleConstellNode(lineEndX, ay, (isSel ? 4.0f : 2.8f) * uiS,
                                signalR, signalG, signalB,
                                (itm.seen ? 0.20f : 0.10f
                                    + 0.24f * active) * reveal, false);
        if (isSel)
            // The vertical selection datum is a stable anchor; hover only
            // changes the horizontal line length and must not shift this bar.
            drawRect(ax + 118.0f * uiS,
                     ay - 22.0f * uiS, 2.0f * uiS, 44.0f * uiS,
                     signalR, signalG, signalB,
                     (itm.seen ? 0.76f : 0.42f) * reveal);
        float tsc = (isSel ? 0.68f : 0.58f) * uiS;
        DrawShadowedText(g_TextS, itm.label, ax + 132.0f * uiS,
                         ay - g_TextS.Height(itm.label, tsc) * 0.5f,
                         tsc,
                         itm.seen ? 0.84f + 0.16f * active : 0.58f,
                         itm.seen ? 0.88f + 0.12f * active : 0.62f,
                         itm.seen ? 0.94f + 0.06f * active : 0.70f,
                         (0.38f + 0.60f * active) * reveal, 0.68f);
    }

    // A search can legitimately hide every record.  Keep this state explicit
    // instead of rendering an "UNASSIGNED" detail panel over an empty list.
    // The chart field is already drawn above, so the empty state sits in the
    // same archive surface and remains readable on every background.
    if (recordSlotCount == 0 && hasSearch) {
        const float emptyCX = rightX + rightW * 0.50f;
        const float emptyY = rightY + rightH * 0.46f;
        const wchar_t* emptyTitle = codexText(L"검색 결과 없음", L"NO MATCHES", L"検索結果なし");
        const wchar_t* emptyHint = codexText(L"다른 검색어를 입력하세요", L"TRY A DIFFERENT SEARCH", L"別の語句を入力してください");
        DrawConstellationDisc(emptyCX, emptyY, 34.0f * uiS,
                              curRoot.r, curRoot.g, curRoot.b,
                              0.12f * rightWake);
        DrawVisibleConstellNode(emptyCX, emptyY, 7.0f * uiS,
                                curRoot.r, curRoot.g, curRoot.b,
                                0.62f * rightWake, false);
        const float titleSc = UiTextScale(g_TextL, UiTextLevel::Title, uiS);
        const float hintSc = UiTextScale(g_TextS, UiTextLevel::Supporting, uiS);
        DrawShadowedText(g_TextL, emptyTitle,
                         emptyCX - g_TextL.Width(emptyTitle, titleSc) * 0.5f,
                         emptyY + 30.0f * uiS, titleSc,
                         0.92f, 0.96f, 1.0f, 0.94f * rightWake, 0.66f);
        DrawShadowedText(g_TextS, emptyHint,
                         emptyCX - g_TextS.Width(emptyHint, hintSc) * 0.5f,
                         emptyY + 62.0f * uiS, hintSc,
                         0.64f, 0.76f, 0.88f, 0.78f * rightWake, 0.56f);
        if (finishBackAfterRender) {
            s_backExit = false;
            s_backOutT = 0.0f;
            s_CodexBackRequested = false;
            s_searchFocused = false;
            g_CodexSearchInputEnabled = false;
            s_MainMenuCodexPanel = false;
            s_MainMenuResumeFromPanel = true;
            g_MainMenuEntryT = 1.0f;
            g_GameManager.currentState = GameState::MAIN_MENU;
        }
        return;
    }


    auto scramble = [&](const wchar_t* src, int seed) -> std::wstring {
        static const wchar_t* glyphs = L"01/\\#*@+-=_";
        if (!src) return L"";
        std::wstring out(src);
        int gCount = 10;
        int tick = (int)(now * 90.0f);
        for (size_t i = 0; i < out.size(); ++i) {
            if (out[i] == L' ' || out[i] == L':' || out[i] == L'|' || out[i] == L'[' || out[i] == L']')
                continue;
            out[i] = glyphs[(seed + tick + (int)i * 7) % gCount];
        }
        return out;
    };
    auto drawScanTextS = [&](const wchar_t* text, float x, float y, float sc,
                             float r, float g, float b, float a, int seed) {
        if (s_decryptT < 0.36f) {
            std::wstring t = scramble(text, seed);
            DrawShadowedText(g_TextS, t.c_str(), x, y, sc, r, g, b,
                             a * (0.72f + 0.28f * s_decryptT), 0.62f);
        } else {
            DrawShadowedText(g_TextS, text, x, y, sc, r, g, b, a, 0.66f);
        }
    };
    auto drawScanTextL = [&](const wchar_t* text, float x, float y, float sc,
                             float r, float g, float b, float a, int seed) {
        if (s_decryptT < 0.36f) {
            std::wstring t = scramble(text, seed);
            DrawShadowedText(g_TextL, t.c_str(), x, y, sc, r, g, b,
                             a * (0.72f + 0.28f * s_decryptT), 0.68f);
        } else {
            DrawShadowedText(g_TextL, text, x, y, sc, r, g, b, a, 0.74f);
        }
    };
    auto drawScanDescription = [&](const wchar_t* text, float x, float y,
                                   float maxW, float sc,
                                   float r, float g, float b, float a,
                                   int seed) {
        std::vector<std::wstring> lines;
        std::wstring line;
        if (text) {
            for (const wchar_t* p = text; *p; ++p) {
                if (*p == L'\n') {
                    lines.push_back(line);
                    line.clear();
                    continue;
                }
                std::wstring candidate = line;
                candidate += *p;
                if (!line.empty() && g_TextS.Width(candidate.c_str(), sc) > maxW) {
                    const size_t split = line.find_last_of(L" \t");
                    if (split != std::wstring::npos) {
                        std::wstring carry = line.substr(split + 1);
                        line.resize(split);
                        if (!line.empty()) lines.push_back(line);
                        line = std::move(carry);
                        if (*p != L' ') line.push_back(*p);
                    } else {
                        lines.push_back(line);
                        line.clear();
                        if (*p != L' ') line.push_back(*p);
                    }
                } else {
                    line = std::move(candidate);
                }
            }
        }
        if (!line.empty()) lines.push_back(line);
        const float lineH = std::max(22.0f * uiS,
            g_TextS.Height(L"Ag", sc) + 5.0f * uiS);
        for (size_t i = 0; i < lines.size(); ++i)
            drawScanTextS(lines[i].c_str(), x, y + (float)i * lineH,
                          sc, r, g, b, a, seed + (int)i);
        return y + (float)lines.size() * lineH;
    };

    auto selectedSeen = [&]() -> bool {
        int key = s_sel[s_cat];
        if (key < 0) return false;
        if (s_cat == 0) return CodexMobSeen(key);
        return s_cat == 1 && key >= 0 && key < AUG_TOTAL && CodexAugSeen(key);
    };
    auto selectedTitle = [&]() -> const wchar_t* {
        int key = s_sel[s_cat];
        if (key < 0) return L"UNASSIGNED";
        if (s_cat == 0) return selectedSeen() ? MobName(key) : L"???";
        return selectedSeen() ? AugName(ALL_AUGS[key]) : L"???";
    };
    auto selectedDesc = [&]() -> const wchar_t* {
        int key = s_sel[s_cat];
        if (!selectedSeen()) {
            static const wchar_t* kUnknownDesc[3] = {
                L"\uBBF8\uD655\uC778 \uC2E0\uD638",
                L"UNDISCOVERED SIGNAL",
                L"\u672A\u78BA\u8A8D\u30B7\u30B0\u30CA\u30EB"
            };
            return kUnknownDesc[std::max(0, std::min(2, li))];
        }
        if (s_cat == 0) return MobDesc(key);
        return AugDesc(ALL_AUGS[key]);
    };

    const int selKey = s_sel[s_cat] < 0 ? 0 : s_sel[s_cat];
    const bool seen = selectedSeen();
    float cr = curRoot.r, cg = curRoot.g, cb = curRoot.b;
    if (s_cat == 1 && selKey >= 0 && selKey < AUG_TOTAL)
        GetRarityColor(ALL_AUGS[selKey].rarity, cr, cg, cb);

    // Fixed record block inside the chart, matching the original wide-open
    // composition.  It does not move with the orbit or label animation.
    const float detailX = sw * 0.68f;
    const float detailW = std::max(320.0f * uiS,
                                   std::min(sw * 0.26f, sw - detailX - 54.0f * uiS));
    const float infoY = sh * 0.42f;
    BindMainShader();
    drawRect(detailX, infoY, detailW, 1.1f * uiS,
             cr, cg, cb, 0.28f * rightWake);
    drawDiamond(detailX, infoY, 3.0f * uiS,
                cr, cg, cb, 0.62f * rightWake);
    if (s_decryptT < 0.96f) {
        const float scanY = infoY + 10.0f * uiS
            + (rightY + rightH - infoY - 28.0f * uiS) * Smoothstep(s_decryptT);
        drawRect(detailX, scanY, detailW, 1.0f * uiS,
                 cr, cg, cb, 0.22f * rightWake * (1.0f - Smoothstep(s_decryptT)));
    }

    wchar_t idBuf[96];
    if (s_cat == 0) {
        if (seen) swprintf_s(idBuf, L"ID : ENTITY_%02d", selKey);
        else      swprintf_s(idBuf, L"ID : ENTITY_??");
    }
    else if (s_cat == 1) swprintf_s(idBuf, L"ID : MODULE_%03d", selKey);
    else swprintf_s(idBuf, L"ID : APEX_%02d", selKey);
    wchar_t statBuf[128];
    if (s_cat == 0)
        swprintf_s(statBuf, seen
            ? L"CLASS : HOSTILE_SIGNAL | STATUS : CATALOGUED"
            : L"CLASS : UNKNOWN_SIGNAL | STATUS : UNRESOLVED");
    else if (s_cat == 1)
        swprintf_s(statBuf, L"CLASS : MODULE_ARCHIVE | STATUS : %ls", seen ? L"ACQUIRED" : L"NO_DATA");
    else
        swprintf_s(statBuf, L"CLASS : APEX_ENTITY | STATUS : %ls", seen ? L"OBSERVED" : L"NO_DATA");

    float titleSc = UiTextScale(g_TextL, UiTextLevel::Title, uiS);
    const float selectedTitleW = g_TextL.Width(selectedTitle(), titleSc);
    if (selectedTitleW > detailW && selectedTitleW > 0.0f)
        titleSc *= detailW / selectedTitleW;
    const float idSc = UiTextScale(g_TextS, UiTextLevel::Supporting, uiS);
    const float descSc = UiTextScale(g_TextS, UiTextLevel::Description, uiS);

    drawScanTextS(idBuf, detailX, infoY + 16.0f * uiS,
                  idSc, 0.62f, 0.72f, 0.86f,
                  0.74f * rightWake, selKey + s_cat * 31);
    drawScanTextL(selectedTitle(), detailX, infoY + 44.0f * uiS,
                  titleSc, 0.94f, 0.98f, 1.0f,
                  0.98f * rightWake, selKey + 7);

    const float descY = infoY + 44.0f * uiS
        + g_TextL.Height(selectedTitle(), titleSc) + 12.0f * uiS;
    const float summarySc = UiTextScale(g_TextS, UiTextLevel::Subtitle, uiS);
    const wchar_t* summaryText = codexText(L"기록 요약", L"RECORD SUMMARY", L"記録の概要");
    DrawShadowedText(g_TextS, summaryText, detailX, descY,
                     summarySc, cr, cg, cb, 0.72f * rightWake, 0.54f);
    const float descriptionY = descY + g_TextS.Height(summaryText, summarySc)
                             + 8.0f * uiS;
    const float descriptionEndY = drawScanDescription(
        selectedDesc(), detailX, descriptionY, detailW, descSc,
        0.76f, 0.84f, 0.94f, 0.86f * rightWake, selKey + 13);

    const float metaY = descriptionEndY + 14.0f * uiS;
    LogoLine(detailX, metaY - 10.0f * uiS,
             detailX + detailW, metaY - 10.0f * uiS,
             0.7f * uiS, cr, cg, cb, 0.14f * rightWake);
    drawScanTextS(statBuf, detailX, metaY,
                  idSc, 0.84f, 0.90f, 0.98f,
                  0.88f * rightWake, selKey + 19);

    wchar_t recordBuf[128];
    if (s_cat == 0) {
        if (seen) swprintf_s(recordBuf,
                             L"RECORD TYPE : HOSTILE ENTITY | ACCESS : READ ONLY");
        else swprintf_s(recordBuf,
                        L"RECORD TYPE : UNKNOWN SIGNAL | ACCESS : LOCKED");
    }
    else if (s_cat == 1)
        swprintf_s(recordBuf, L"RECORD TYPE : AUGMENT MODULE | SOURCE : IN-RUN");
    else
        swprintf_s(recordBuf, L"RECORD TYPE : APEX ENCOUNTER | ACCESS : READ ONLY");
        drawScanTextS(recordBuf, detailX, metaY + 25.0f * uiS,
                  idSc, 0.62f, 0.70f, 0.82f,
                  0.70f * rightWake, selKey + 23);
    g_TextS.Draw(L"[ ARCHIVE_READ_ONLY ]", detailX, metaY + 52.0f * uiS,
                 idSc, cr, cg, cb, 0.82f * rightWake);

    if (finishBackAfterRender) {
        s_backExit = false;
        s_backOutT = 0.0f;
        s_CodexBackRequested = false;
        s_searchFocused = false;
        g_CodexSearchInputEnabled = false;
        s_MainMenuCodexPanel = false;
        s_MainMenuResumeFromPanel = true;
        g_MainMenuEntryT = 1.0f;
        g_GameManager.currentState = GameState::MAIN_MENU;
    }
}
