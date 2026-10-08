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
    static int   s_prevSelectionCategory = -1;
    static int   s_searchSelectedCategory = 0;
    static int   s_searchSelectedKey = -1;
    static int   s_prevMobDataStage = -1;
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
    static float s_searchHover = 0.0f;
    bool& s_searchFocused = g_CodexSearchInputEnabled;
    static std::wstring s_prevSearch;

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

    if (c.inputFocusChanged) {
        s_navHoldDir = 0;
        s_navHoldT = 0.0f;
        s_navRepeatT = 0.0f;
        s_dragging = false;
        s_dragAccum = 0.0f;
    }

    const std::wstring currentSearch = CodexSearchQuery();
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
    if (rmbClick) beginBack();
    if (escClick) {
        if (s_searchFocused) s_searchFocused = false;
        else beginBack();
    }
    if (inputReady && s_searchFocused && c.window &&
        glfwGetKey(c.window, GLFW_KEY_ENTER) == GLFW_PRESS)
        s_searchFocused = false;

    bool finishBackAfterRender = false;
    if (s_backExit) {
        s_backOutT += dt;
        if (s_backOutT >= kOutgameTransitionDuration) {
            s_backOutT = kOutgameTransitionDuration;
            finishBackAfterRender = true;
        }
    }

    const float entryOldOut = SceneTransitionEase(g_CodexEntryT / kOutgameTransitionDuration);
    const float entryTreeIn = SceneTransitionEase(
        (g_CodexEntryT - 0.08f) / (kOutgameTransitionDuration - 0.08f));
    const float entryDetailIn = SceneTransitionEase(
        (g_CodexEntryT - 0.14f) / (kOutgameTransitionDuration - 0.14f));
    const float backP = s_backExit
        ? SceneTransitionEase(s_backOutT / kOutgameTransitionDuration) : 0.0f;
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
        : SceneTransitionEase(g_CodexEntryT / kOutgameTransitionDuration);
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
    const float rootGap = 10.0f * uiS;
    const float rootTotalH = rootH * (CAT_COUNT + 1) + rootGap * CAT_COUNT;
    const float railY = OutgameButtonRailStartY(
        sh, uiS, CAT_COUNT + 1, rootH, rootGap);
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

    DrawSharedMenuDim(sw, sh, mainX, railY, mainBW, rootTotalH,
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
                               uiS, false, false, false,
                               rd.label, 3);
    }

    struct CItem {
        bool isGroup;
        int category;
        int key;
        const wchar_t* label;
        bool seen;
        float r, g, b;
    };
    const bool searchActive = !currentSearch.empty();
    CItem items[512];
    int itemCount = 0;
    auto addGroup = [&](int category, const wchar_t* label, float r, float g, float b) {
        if (itemCount < 512) items[itemCount++] = { true, category, -1, label, true, r, g, b };
    };
    auto addItem = [&](int category, int key, const wchar_t* label, bool seen,
                       const wchar_t* const* searchNames, int searchNameCount,
                       float r, float g, float b) {
        if (searchActive && (!seen ||
            !CodexSearchMatchesAny(searchNames, searchNameCount))) return;
        if (itemCount < 512)
            items[itemCount++] = { false, category, key, label, seen, r, g, b };
    };

    if (searchActive) {
        int groupStart = itemCount;
        addGroup(0, ROOTS[0].label[rootLanguage], 0.48f, 0.82f, 1.00f);
        ForEachCodexMobEntry([&](int i) {
            const bool nameUnlocked = CodexMobNameUnlocked(i);
            addItem(0, i, CodexMobListLabel(i), nameUnlocked,
                    nameUnlocked ? MobLocalizedNames(i) : nullptr,
                    nameUnlocked ? LANG_COUNT : 0,
                    0.48f, 0.82f, 1.00f);
        });
        if (itemCount == groupStart + 1) itemCount = groupStart;

        groupStart = itemCount;
        addGroup(1, ROOTS[1].label[rootLanguage], 0.62f, 0.52f, 1.00f);
        int sorted[AUG_TOTAL], n = 0;
        for (int i = 0; i < AUG_TOTAL; ++i)
            if (!AugRemoved(ALL_AUGS[i].type)) sorted[n++] = i;
        std::sort(sorted, sorted + n, [](int a, int b) { return AugTierIndexLess(a, b); });
        for (int k = 0; k < n; ++k) {
            int i = sorted[k];
            AugRarity rar = ALL_AUGS[i].rarity;
            float rr, rg, rb; GetRarityColor(rar, rr, rg, rb);
            addItem(1, i, CodexAugSeen(i) ? AugName(ALL_AUGS[i]) : L"???",
                    CodexAugSeen(i), ALL_AUGS[i].locName, LANG_COUNT, rr, rg, rb);
        }
        if (itemCount == groupStart + 1) itemCount = groupStart;
    } else if (s_cat == 0) {
        addGroup(0, L"SIGNAL CLASS", 0.48f, 0.82f, 1.00f);
        ForEachCodexMobEntry([&](int i) {
            const bool nameUnlocked = CodexMobNameUnlocked(i);
            addItem(0, i, CodexMobListLabel(i), CodexMobSeen(i),
                    nameUnlocked ? MobLocalizedNames(i) : nullptr,
                    nameUnlocked ? LANG_COUNT : 0,
                    0.48f, 0.82f, 1.00f);
        });
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
                addGroup(1, GetRarityKR(rar), rr, rg, rb);
                prevR = rar;
            }
            addItem(1, i, CodexAugSeen(i) ? AugName(ALL_AUGS[i]) : L"???",
                    CodexAugSeen(i), ALL_AUGS[i].locName, LANG_COUNT, rr, rg, rb);
        }
    }

    // Count the visible roster, not CM_COUNT: reserved save slots keep enum
    // indices stable but are never shown, so they must not raise the total.
    int unlockTotal[CAT_COUNT] = {};
    int unlockSeen[CAT_COUNT] = {};
    ForEachCodexMobEntry([&](int id) {
        ++unlockTotal[0];
        if (CodexMobNameUnlocked(id)) ++unlockSeen[0];
    });
    for (int i = 0; i < AUG_TOTAL; ++i) {
        if (AugRemoved(ALL_AUGS[i].type)) continue;
        ++unlockTotal[1];
        if (CodexAugSeen(i)) ++unlockSeen[1];
    }

    int recordCount = 0;
    for (int i = 0; i < itemCount; ++i) {
        if (items[i].isGroup) continue;
        ++recordCount;
    }
    wchar_t archivePath[96];
    swprintf_s(archivePath, L"ASTRAL ARCHIVE / %ls",
               ROOTS[s_cat].label[rootLanguage]);
    // The restored layout deliberately carries no page header or enclosing
    // archive panel: the root commands, orbit, and record itself are enough.
    (void)archivePath;

    // Use the open upper-left canvas for search and a compact global unlock
    // summary. A query searches discovered records across both categories.
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
    // Vertical rhythm for the search / unlock column (QA #41): every block is
    // stacked from the measured height of the one above plus a fixed gap.
    const float searchLabelH = g_TextS.Height(
        L"A", UiTextScale(g_TextS, UiTextLevel::Subtitle, uiS));
    const float searchY = utilityY + searchLabelH + 14.0f * uiS;
    const float searchH = 46.0f * uiS;
    const bool searchHover = inputReady && mx >= utilityX && mx <= utilityX + searchW
        && my >= searchY && my <= searchY + searchH;
    s_searchHover = UiApproach(s_searchHover, searchHover ? 1.0f : 0.0f, dt, 10.0f);
    const float clearW = 32.0f * uiS;
    const bool hasSearch = searchActive;
    auto formatResultSummary = [&](wchar_t* out, size_t count, int value) {
        if (li == 0)
            swprintf_s(out, count, L"\uAC80\uC0C9 \uACB0\uACFC %d\uAC74", value);
        else if (li == 2)
            swprintf_s(out, count, L"\u691C\u7D22\u7D50\u679C %d\u4EF6", value);
        else
            swprintf_s(out, count, L"%d MATCHES", value);
    };
    wchar_t searchResultSummary[48] = {};
    if (hasSearch)
        formatResultSummary(searchResultSummary, 48, recordCount);
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
    if (!g_CodexSearchInputEnabled) CodexSearchClearComposition();

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
        searchDisplay = currentSearch;
    else if (!s_searchFocused)
        searchDisplay = std::wstring(codexText(L"\uC785\uB825\uD558\uC5EC \uAC80\uC0C9", L"TYPE TO FILTER", L"\u5165\u529B\u3057\u3066\u691C\u7D22"));
    const bool searchCaretOn = s_searchFocused && (((int)(now * 2.0f)) & 1) == 0;
    float searchSc = UiTextScale(g_TextS, hasSearch
        ? UiTextLevel::Description : UiTextLevel::Supporting, uiS);
    const float searchMaxW = std::max(30.0f * uiS, searchW - clearW - 58.0f * uiS);
    const float searchTextW0 = g_TextS.Width(searchDisplay.c_str(), searchSc);
    if (searchTextW0 > searchMaxW && searchTextW0 > 0.0f)
        searchSc *= searchMaxW / searchTextW0;
    const float searchTextX = utilityX + 16.0f * uiS;
    float searchTextH = g_TextS.Height(searchDisplay.c_str(), searchSc);
    if (searchTextH <= 0.0f) searchTextH = g_TextS.Height(L"Ag", searchSc);
    const float searchTextY = searchMidY - searchTextH * 0.50f;
    const int searchCaretIndex = std::clamp(
        CodexSearchQueryCaret(), 0, (int)currentSearch.size());
    const std::wstring caretPrefix(currentSearch.begin(),
                                   currentSearch.begin() + searchCaretIndex);
    const float searchCaretX = searchTextX + g_TextS.Width(caretPrefix.c_str(), searchSc);
    const int compositionStart = std::clamp(
        CodexSearchCompositionStart(), 0, (int)currentSearch.size());
    const int compositionEnd = std::clamp(
        compositionStart + g_CodexSearchCompositionLen,
        compositionStart, (int)currentSearch.size());
    const std::wstring compositionPrefix(currentSearch.begin(),
                                         currentSearch.begin() + compositionStart);
    const float compositionX = searchTextX
        + g_TextS.Width(compositionPrefix.c_str(), searchSc);
    if (s_searchFocused && c.window)
        InputSetCodexImeAnchor(compositionX, searchCaretX,
                               searchTextY, searchTextH);
    if (CodexSearchHasSelection() && g_CodexSearchCompositionLen == 0) {
        const int selBegin = CodexSearchSelectionStart();
        const int selEnd = CodexSearchSelectionEnd();
        const std::wstring before(g_CodexSearch, g_CodexSearch + selBegin);
        const std::wstring selected(g_CodexSearch + selBegin, g_CodexSearch + selEnd);
        const float highlightX = searchTextX + g_TextS.Width(before.c_str(), searchSc);
        const float highlightW = g_TextS.Width(selected.c_str(), searchSc);
        if (highlightW > 0.0f)
            drawRect(highlightX, searchY + 8.0f * uiS, highlightW,
                     searchH - 16.0f * uiS,
                     curRoot.r, curRoot.g, curRoot.b, 0.32f * wake);
    }
    DrawShadowedText(g_TextS, searchDisplay.c_str(),
                     searchTextX, searchTextY,
                     searchSc,
                     hasSearch ? 0.92f : 0.42f,
                     hasSearch ? 0.96f : 0.56f,
                     hasSearch ? 1.00f : 0.70f,
                     (hasSearch ? 0.92f : 0.70f) * wake, 0.62f);
    if (g_CodexSearchCompositionLen > 0) {
        const std::wstring compositionText(
            currentSearch.begin() + compositionStart,
            currentSearch.begin() + compositionEnd);
        const float compositionW = g_TextS.Width(compositionText.c_str(), searchSc);
        if (compositionW > 0.0f)
            drawRect(compositionX, searchTextY + searchTextH + 1.0f * uiS,
                     compositionW, std::max(1.0f, 1.2f * uiS),
                     curRoot.r, curRoot.g, curRoot.b, 0.95f * wake);
    }
    if (searchCaretOn)
        drawRect(searchCaretX, searchTextY + searchTextH * 0.08f,
                 std::max(1.0f, 1.4f * uiS), searchTextH * 0.84f,
                 0.80f + 0.20f * curRoot.r,
                 0.84f + 0.16f * curRoot.g,
                 0.90f + 0.10f * curRoot.b,
                 0.96f * wake);
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
    // The key-hint line under the field was removed (QA #42); the unlock
    // title now sits a clear gap below the search field.
    const float progressTitleY = searchY + searchH + 40.0f * uiS;
    const wchar_t* progressTitle = codexText(
        L"\uB3C4\uAC10 \uD574\uAE08 \uBAA9\uB85D", L"UNLOCK PROGRESS", L"\u56F3\u9451\u89E3\u653E\u9032\u6357");
    const float progressBarX = utilityX;
    const float progressBarW = std::max(48.0f * uiS,
                                        std::min(320.0f * uiS,
                                                 utilityW - 16.0f * uiS));
    const float resultSc = UiTextScale(g_TextS, UiTextLevel::Supporting, uiS);
    // Reserve room for a two-digit result count whether or not a search is
    // active, so the title never runs into it (EN "UNLOCK PROGRESS") and
    // does not change size while typing.
    wchar_t resultReserve[48] = {};
    formatResultSummary(resultReserve, 48, 88);
    const float resultReserveW = g_TextS.Width(resultReserve, resultSc)
                               + 16.0f * uiS;
    float progressTitleFactor = uiS;
    float progressTitleScale = UiTextScale(g_TextS, UiTextLevel::Subtitle,
                                           progressTitleFactor);
    while (progressTitleFactor > 0.6f * uiS &&
           g_TextS.Width(progressTitle, progressTitleScale) + resultReserveW
               > progressBarW) {
        progressTitleFactor -= 0.03f * uiS;
        progressTitleScale = UiTextScale(g_TextS, UiTextLevel::Subtitle,
                                         progressTitleFactor);
    }
    DrawShadowedText(g_TextS,
                     progressTitle,
                     utilityX, progressTitleY, progressTitleScale,
                     curRoot.r, curRoot.g, curRoot.b, 0.84f * wake, 0.58f);
    if (hasSearch) {
        const float resultW = g_TextS.Width(searchResultSummary, resultSc);
        const bool besideTitle = g_TextS.Width(progressTitle, progressTitleScale)
                               + resultReserveW <= progressBarW;
        // On small windows the title is already at the minimum text scale;
        // the count then moves into the search field, left of the clear X.
        const float resultX = besideTitle
            ? progressBarX + progressBarW - resultW
            : utilityX + searchW - clearW - 8.0f * uiS - resultW;
        const float resultY = besideTitle
            ? progressTitleY + (g_TextS.Height(progressTitle, progressTitleScale)
                                - g_TextS.Height(searchResultSummary, resultSc)) * 0.5f
            : searchY + (searchH - g_TextS.Height(searchResultSummary, resultSc)) * 0.5f;
        DrawShadowedText(g_TextS, searchResultSummary, resultX, resultY,
                         resultSc, 0.62f, 0.72f, 0.86f, 0.66f * wake, 0.52f);
    }
    // Give each unlock entry its own two-line block: a large readout first,
    // then the archive line directly underneath it.  This keeps the progress
    // data legible without competing with the search field above.
    const float progressRowY = progressTitleY
        + g_TextS.Height(progressTitle, progressTitleScale) + 22.0f * uiS;
    const float progressRowStep = 80.0f * uiS;
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

    int selectedCategory = searchActive ? s_searchSelectedCategory : s_cat;
    int selectedKey = searchActive ? s_searchSelectedKey : s_sel[s_cat];
    auto setCurrentSelection = [&](int category, int key) {
        if (category < 0 || category >= CAT_COUNT) return;
        s_sel[category] = key;
        if (searchActive) {
            s_searchSelectedCategory = category;
            s_searchSelectedKey = key;
            selectedCategory = category;
            selectedKey = key;
        }
    };
    bool selectedVisible = false;
    for (int i = 0; i < itemCount; ++i) {
        if (!items[i].isGroup && items[i].category == selectedCategory &&
            items[i].key == selectedKey) {
            selectedVisible = true;
            break;
        }
    }
    if (!selectedVisible) {
        selectedKey = -1;
        if (searchActive) s_searchSelectedKey = -1;
        else s_sel[s_cat] = -1;
    }

    if (selectedKey < 0) {
        for (int i = 0; i < itemCount; ++i) {
            if (!items[i].isGroup) {
                setCurrentSelection(items[i].category, items[i].key);
                break;
            }
        }
    }
    selectedCategory = searchActive ? s_searchSelectedCategory : s_cat;
    selectedKey = searchActive ? s_searchSelectedKey : s_sel[s_cat];
    const int selectedMobStage = selectedCategory == 0 && selectedKey >= 0
        ? CodexMobDataStage(selectedKey) : -1;
    if (selectedKey != s_prevSel || selectedCategory != s_prevSelectionCategory ||
        selectedMobStage != s_prevMobDataStage) {
        s_prevSel = selectedKey;
        s_prevSelectionCategory = selectedCategory;
        s_prevMobDataStage = selectedMobStage;
        s_decryptT = 0.0f;
    }
    s_decryptT = UiApproach(s_decryptT, 1.0f, dt, 8.0f);

    // Orbital record list.  Selection owns one angular target; rendering reads
    // only the damped angle so labels and miniature constellations never apply
    // a second, conflicting interpolation.
    int recordSlots[512] = {};
    int recordSlotCount = 0;
    int selectedSlot = 0;
    const int orbitState = searchActive ? 0 : s_cat;
    for (int i = 0; i < itemCount; ++i) {
        if (items[i].isGroup) continue;
        if (items[i].category == selectedCategory && items[i].key == selectedKey)
            selectedSlot = recordSlotCount;
        recordSlots[recordSlotCount++] = i;
    }
    if (recordSlotCount > 0 && selectedKey < 0) {
        const CItem& first = items[recordSlots[0]];
        setCurrentSelection(first.category, first.key);
        selectedSlot = 0;
    }
    if (s_displaySlot[orbitState] == 0.0f && s_displayTarget[orbitState] == 0.0f && selectedSlot != 0) {
        s_displaySlot[orbitState] = s_displayTarget[orbitState] = (float)selectedSlot;
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
    const float chartCY = sh * 0.51f;
    const float chartR = std::max(sh * 0.58f, sw * 0.44f);
    // Entity silhouettes use their gameplay footprint (Gravis includes a
    // wide gravity field), so give that category more vertical breathing room.
    const float rowStep = 245.0f * uiS;
    const float orbitInputLeft = chartCX - itemR - 80.0f * uiS;
    const float orbitInputRight = std::min(sw, rightX - 24.0f * uiS);
    const bool overOrbit = inputReady &&
        mx >= orbitInputLeft && mx <= orbitInputRight &&
        my >= archivePanelY && my <= archivePanelBottom;
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
        const CItem& next = items[recordSlots[selectedSlot]];
        setCurrentSelection(next.category, next.key);
        s_displayTarget[orbitState] += (float)stepRequest;
    }
    {
        const float diff = s_displayTarget[orbitState] - s_displaySlot[orbitState];
        const float speed = 10.0f;
        s_displaySlot[orbitState] += diff * std::min(1.0f, dt * speed);
        if (fabsf(diff) < 0.002f) {
            s_displaySlot[orbitState] = s_displayTarget[orbitState];
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
        float relF = (float)slot - s_displaySlot[orbitState];
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
        const bool hov = inputReady && mx >= orbitInputLeft &&
                         mx <= orbitInputRight &&
                         mx >= ax - 80.0f * uiS && mx <= ax + hitW &&
                         my >= ay - hitH * 0.5f && my <= ay + hitH * 0.5f;
        if (hov && lmb && !g_LmbPrev && !s_dragMoved) {
            float jump = (float)slot - (float)selectedSlot;
            while (jump > (float)recordSlotCount * 0.5f) jump -= (float)recordSlotCount;
            while (jump < -(float)recordSlotCount * 0.5f) jump += (float)recordSlotCount;
            setCurrentSelection(itm.category, itm.key);
            selectedSlot = slot;
            s_displayTarget[orbitState] += jump;
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
        const float miniX = ax + ((itm.category == 0 && itm.key == CM_QUASAR && isSel)
                                  ? 22.0f * uiS : 0.0f);
        // Keep the gameplay silhouette readable, but compact enough that the
        // wider entity row spacing still shows roughly 3-4 records per page.
        const float focusScale = std::max(0.0f, std::min(1.0f, s_itemHover[slot]));
        const float miniBase = (itm.category == 1) ? 34.0f : 18.0f;
        const float miniFocus = (itm.category == 1) ? 38.0f : 24.0f;
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
        if (itm.category == 0 && !itm.seen) {
            DrawUnknownConstellation(miniX, ay, miniR * 1.12f,
                                     now * 0.18f, (0.58f + 0.30f * active)
                                     * reveal, uiS, itm.key);
        } else {
            DrawArchiveConstellation(miniX, ay, miniR, itm.category, itm.key,
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
        // Stack the hint under the measured title height; the fixed +62
        // offset was shorter than a Title-level line and the two overlapped.
        const float emptyTitleY = emptyY + 30.0f * uiS;
        const float emptyHintY = emptyTitleY
                               + g_TextL.Height(emptyTitle, titleSc)
                               + 10.0f * uiS;
        DrawShadowedText(g_TextL, emptyTitle,
                         emptyCX - g_TextL.Width(emptyTitle, titleSc) * 0.5f,
                         emptyTitleY, titleSc,
                         0.92f, 0.96f, 1.0f, 0.94f * rightWake, 0.66f);
        DrawShadowedText(g_TextS, emptyHint,
                         emptyCX - g_TextS.Width(emptyHint, hintSc) * 0.5f,
                         emptyHintY, hintSc,
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
        int key = selectedKey;
        if (key < 0) return false;
        if (selectedCategory == 0) return CodexMobSeen(key);
        return selectedCategory == 1 && key >= 0 && key < AUG_TOTAL && CodexAugSeen(key);
    };
    auto selectedTitle = [&]() -> const wchar_t* {
        int key = selectedKey;
        if (key < 0) return L"UNASSIGNED";
        if (selectedCategory == 0) return CodexMobNameUnlocked(key) ? MobName(key) : L"";
        return selectedSeen() ? AugName(ALL_AUGS[key]) : L"???";
    };
    auto selectedDesc = [&]() -> const wchar_t* {
        int key = selectedKey;
        if (!selectedSeen()) {
            static const wchar_t* kUnknownDesc[3] = {
                L"\uBBF8\uD655\uC778 \uC2E0\uD638",
                L"UNDISCOVERED SIGNAL",
                L"\u672A\u78BA\u8A8D\u30B7\u30B0\u30CA\u30EB"
            };
            return kUnknownDesc[std::max(0, std::min(2, li))];
        }
        if (selectedCategory == 0) {
            if (CodexMobDataStage(key) == 0) return L"";
            return MobDesc(key);
        }
        return AugDesc(ALL_AUGS[key]);
    };

    const int selKey = selectedKey < 0 ? 0 : selectedKey;
    const bool seen = selectedSeen();
    const RootDef& selectedRoot = ROOTS[std::max(0, std::min(CAT_COUNT - 1, selectedCategory))];
    float cr = selectedRoot.r, cg = selectedRoot.g, cb = selectedRoot.b;
    if (selectedCategory == 1 && selKey >= 0 && selKey < AUG_TOTAL)
        GetRarityColor(ALL_AUGS[selKey].rarity, cr, cg, cb);

    // Fixed record block inside the chart, matching the original wide-open
    // composition.  It does not move with the orbit or label animation.
    const float detailX = sw * 0.68f;
    const float detailW = std::max(320.0f * uiS,
                                   std::min(sw * 0.26f, sw - detailX - 54.0f * uiS));
    // The selected record always sits on the chartCY row and its selection
    // line runs right toward this block, so the block's title row is centred
    // on that same line instead of floating at a fixed screen height (#40).
    const float recordTitleH = g_TextL.Height(
        L"A", UiTextScale(g_TextL, UiTextLevel::Title, uiS));
    const float infoY = chartCY - 44.0f * uiS - recordTitleH * 0.5f;
    const int mobDataStage = selectedCategory == 0 ? CodexMobDataStage(selKey) : 0;
    BindMainShader();

    if (selectedCategory == 0 && mobDataStage == 0) {
        if (seen) {
            const wchar_t* formTitle = CodexLocalizedText(
                L"관측된 형태", L"OBSERVED FORM", L"観測された形状");
            const float titleScale = UiTextScale(g_TextL, UiTextLevel::Title, uiS);
            drawScanTextL(L"???", detailX, infoY + 44.0f * uiS,
                          titleScale, 0.94f, 0.98f, 1.0f,
                          0.96f * rightWake, selKey + 41);
            const float formLabelScale = UiTextScale(
                g_TextS, UiTextLevel::Supporting, uiS);
            drawScanTextS(formTitle, detailX, infoY + 94.0f * uiS,
                          formLabelScale, cr, cg, cb, 0.80f * rightWake, selKey + 42);
            const float formY = infoY + 126.0f * uiS;
            const float formScale = UiTextScale(g_TextS, UiTextLevel::Description, uiS);
            drawScanDescription(CodexMobFormDescription(selKey), detailX, formY,
                                detailW, formScale, 0.76f, 0.84f, 0.94f,
                                0.86f * rightWake, selKey + 43);
        } else {
            const wchar_t* undiscovered = CodexLocalizedText(
                L"미발견", L"UNDISCOVERED", L"未発見");
            const float labelSc = UiTextScale(g_TextS, UiTextLevel::Supporting, uiS);
            DrawShadowedText(g_TextS, undiscovered,
                detailX + (detailW - g_TextS.Width(undiscovered, labelSc)) * 0.5f,
                infoY + 58.0f * uiS, labelSc,
                0.58f, 0.70f, 0.84f, 0.76f * rightWake, 0.56f);
        }
    } else {
    wchar_t idBuf[96];
    if (selectedCategory == 0) {
        if (seen) swprintf_s(idBuf, L"ID : ENTITY_%02d", selKey);
        else      swprintf_s(idBuf, L"ID : ENTITY_??");
    }
    else if (selectedCategory == 1) swprintf_s(idBuf, L"ID : MODULE_%03d", selKey);
    else swprintf_s(idBuf, L"ID : APEX_%02d", selKey);
    wchar_t statBuf[128];
    if (selectedCategory == 0)
        swprintf_s(statBuf, mobDataStage > 0 ? L"TIER : %ls" : L"TIER : LOCKED",
                   mobDataStage > 0 ? MobTierLabel(selKey) : L"");
    else if (selectedCategory == 1)
        swprintf_s(statBuf, L"TYPE : %ls", GetRarityKR(ALL_AUGS[selKey].rarity));
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
                  0.74f * rightWake, selKey + selectedCategory * 31);
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
    drawScanTextS(statBuf, detailX, metaY,
                  idSc, 0.84f, 0.90f, 0.98f,
                  0.88f * rightWake, selKey + 19);

    if (selectedCategory == 0 && seen) {
        const long long kills = CodexMobKillCount(selKey);
        const int nextMilestone = mobDataStage == 0 ? 0 : mobDataStage == 1 ? 1 : 2;
        wchar_t progressBuf[128];
        if (mobDataStage < 3)
            swprintf_s(progressBuf, L"KILLS : %lld / %lld", kills,
                       CodexMobKillThreshold(selKey, nextMilestone));
        else
            swprintf_s(progressBuf, L"KILLS : %lld | DATA COMPLETE", kills);
        float dataY = metaY + 26.0f * uiS;
        drawScanTextS(progressBuf, detailX, dataY, idSc,
                      0.48f, 0.82f, 1.0f, 0.88f * rightWake, selKey + 25);
        dataY += 25.0f * uiS;
        if (mobDataStage == 0 || mobDataStage == 1 || mobDataStage == 2) {
            const wchar_t* next = mobDataStage == 0
                ? CodexLocalizedText(L"다음 해금 : 티어와 개요", L"NEXT : TIER AND OVERVIEW", L"次の解放 : ティアと概要")
                : mobDataStage == 1
                ? CodexLocalizedText(L"다음 해금 : 전투 능력치", L"NEXT : COMBAT STATS", L"次の解放 : 戦闘ステータス")
                : CodexLocalizedText(L"다음 해금 : 공격 패턴", L"NEXT : ATTACK PROFILE", L"次の解放 : 攻撃パターン");
            drawScanTextS(next, detailX, dataY, idSc,
                          0.62f, 0.72f, 0.84f, 0.76f * rightWake, selKey + 26);
            dataY += 26.0f * uiS;
        }
        if (mobDataStage >= 2) {
            wchar_t hpBuf[64];
            swprintf_s(hpBuf, L"HP : %d (BASE)", CodexMobBaseHp(selKey));
            const wchar_t* rows[] = {
                hpBuf,
                CodexMobDamageValue(selKey),
                CodexMobBaseSpeed(selKey),
            };
            const wchar_t* labels[] = { nullptr, L"DAMAGE : ", L"MOVE SPEED : " };
            for (int row = 0; row < 3; ++row) {
                std::wstring line = row == 0 ? std::wstring(rows[row])
                    : std::wstring(labels[row]) + rows[row];
                drawScanTextS(line.c_str(), detailX, dataY, idSc,
                              0.78f, 0.88f, 0.98f, 0.84f * rightWake,
                              selKey + 27 + row);
                dataY += 25.0f * uiS;
            }
            const wchar_t* note = CodexLocalizedText(
                L"기본 수치 · 플레이 진행도 보정 제외", L"BASE VALUES · RUN SCALING EXCLUDED",
                L"基本値 · ラン進行度補正を除く");
            drawScanTextS(note, detailX, dataY, idSc,
                          0.55f, 0.64f, 0.76f, 0.68f * rightWake, selKey + 31);
            dataY += 25.0f * uiS;
        }
        if (mobDataStage >= 3) {
            std::wstring attack = std::wstring(L"ATTACK : ") + CodexMobAttackPattern(selKey);
            drawScanTextS(attack.c_str(), detailX, dataY, idSc,
                          0.86f, 0.92f, 1.0f, 0.90f * rightWake, selKey + 32);
        }
    } else if (selectedCategory != 0) {
        if (selectedCategory != 1) {
            wchar_t recordBuf[128];
            swprintf_s(recordBuf, L"RECORD TYPE : APEX ENCOUNTER | ACCESS : READ ONLY");
            drawScanTextS(recordBuf, detailX, metaY + 25.0f * uiS,
                          idSc, 0.62f, 0.70f, 0.82f,
                          0.70f * rightWake, selKey + 23);
            g_TextS.Draw(L"[ ARCHIVE_READ_ONLY ]", detailX, metaY + 52.0f * uiS,
                         idSc, cr, cg, cb, 0.82f * rightWake);
        }
    }
    }

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
