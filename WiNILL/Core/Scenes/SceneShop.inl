// Included by SceneMenus.cpp to keep shared menu state and helpers private.

void Scene_Shop(const SceneCtx& c) {
    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    // ── State ──────────────────────────────────────────────────────────
    static constexpr int SHOP_TAB_COUNT = 3;
    static int   s_tab        = 0;
    static float s_tabHov[SHOP_TAB_COUNT] = {};
    static float s_itemHov[300] = {};
    static float s_backHov = 0.0f;
    static int   s_selKey     = -1;
    static float s_scroll     = 0.0f;
    static float s_detailT    = 0.0f;
    static int   s_prevKey    = -2;
    static int   s_prevTab    = -1;
    static bool  s_backExit   = false;
    static float s_backOutT   = 0.0f;
    static float s_listTransitionDir = 1.0f;
    static int   s_listFromKey = -2;
    static float s_listTransitionT = 1.0f;
    static int   s_navHoldDir  = 0;
    static float s_navHoldT    = 0.0f;
    static float s_navRepeatT  = 0.0f;
    static bool  s_dragging    = false;
    static bool  s_dragMoved   = false;
    static float s_dragAccum   = 0.0f;
    static double s_dragLastY  = 0.0;
    static bool  s_prevEsc    = false;
    static int   s_prevTagKey = -2;
    static float s_tagFlickerT = 1.0f;
    static bool  s_browseItems = false;
    static float s_browseT = 0.0f;
    static bool  s_browseInputLock = false;
    static int   s_fieldColorTab = -1;
    static float s_fieldColorR = 0.30f;
    static float s_fieldColorG = 0.86f;
    static float s_fieldColorB = 1.00f;
    static float s_fieldTargetR = 0.30f;
    static float s_fieldTargetG = 0.86f;
    static float s_fieldTargetB = 1.00f;

    struct SLItem {
        bool           isGroup;
        int            key;
        const wchar_t* label;
        float          r, g, b;
    };
    static SLItem s_items[300];
    static int    s_itemCount = 0;

    const float now = (float)glfwGetTime();
    if (!s_MainMenuArmoryPanel)
        DrawMenuBackground(sw, sh, delta, false);

    float dt = delta; if (dt > 0.05f) dt = 0.05f;
    if (s_ShopNeedsOpenInit) {
        // Open ARMORY directly on the first category. The start-module node
        // should be visible as soon as the shop finishes entering; requiring
        // an extra click on the already-selected tab made it look missing.
        s_browseItems = true;
        s_browseT = 0.0f;
        s_browseInputLock = true;
        s_ShopNeedsOpenInit = false;
    }
    g_ShopEntryT += dt;
    if (g_ShopEntryT > 1.0f) g_ShopEntryT = 1.0f;

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

    const float browseTarget = s_browseItems ? 1.0f : 0.0f;
    s_browseT = UiApproach(s_browseT, browseTarget, dt, 9.0f);
    if (s_browseInputLock && std::abs(s_browseT - browseTarget) < 0.035f)
        s_browseInputLock = false;
    const bool inputReady = !s_backExit && !s_browseInputLock && g_ShopEntryT >= 0.55f;
    auto beginBack = [&]() {
        if (!inputReady) return;
        s_backExit = true;
        s_backOutT = 0.0f;
        s_ShopBackRequested = false;
    };
    if (s_ShopBackRequested) {
        if (inputReady) beginBack();
        else s_ShopBackRequested = false;
    }
    if (rmbClick || escClick) {
        if (inputReady && s_browseItems) {
            s_browseItems = false;
            s_browseInputLock = true;
        } else {
            beginBack();
        }
    }
    bool finishBackAfterRender = false;
    if (s_backExit) {
        s_backOutT += dt;
        if (s_backOutT >= kOutgameTransitionDuration) {
            s_backOutT = kOutgameTransitionDuration;
            finishBackAfterRender = true;
        }
    }

    const float entryOldOut = SceneTransitionEase(g_ShopEntryT / kOutgameTransitionDuration);
    const float entryTreeIn = SceneTransitionEase(
        (g_ShopEntryT - 0.08f) / (kOutgameTransitionDuration - 0.08f));
    const float entryDetailIn = SceneTransitionEase(
        (g_ShopEntryT - 0.14f) / (kOutgameTransitionDuration - 0.14f));
    const float backP = s_backExit
        ? SceneTransitionEase(s_backOutT / kOutgameTransitionDuration) : 0.0f;
    const float oldMenuA = s_backExit ? backP : (1.0f - entryOldOut);
    const float wake = s_backExit ? (1.0f - backP) : entryTreeIn;
    const float rightWake = s_backExit ? (1.0f - backP) : entryDetailIn;
    int li = LangIndex(); if (li < 0 || li >= 3) li = 0;
    const int nli = (li == 0) ? 0 : 1;
    if (s_tab < 0 || s_tab >= SHOP_TAB_COUNT) { s_tab = 0; s_prevTab = -1; }

    // ── Layout ─────────────────────────────────────────────────────────
    const float TARGET_W = 1640.0f, TARGET_H = 910.0f;
    const float uiS = UiScale(sw, sh);
    const float mainBW = std::min(560.0f * uiS, std::max(420.0f * uiS, sw * 0.34f));
    const float mainBH = 70.0f * uiS;
    const float mainX = std::max(58.0f * uiS, sw * 0.075f);
    const float mainY = MainMenuCommandStartY(sh, uiS);

    const float rootX = mainX + (s_backExit ? backP * 180.0f * uiS : -(1.0f - wake) * 82.0f * uiS);
    const float rootY = mainY;
    const float rootW = std::min(340.0f * uiS, mainBW * 0.66f);
    const float rootH = mainBH;
    const float rootGap = 10.0f * uiS;
    const float shopRailH = (SHOP_TAB_COUNT + 1) * rootH + SHOP_TAB_COUNT * rootGap;
    // Keep SHOP's Back row on the same lower-left anchor as the lobby and Codex.
    const float shopRailY = OutgameButtonRailStartY(
        sh, uiS, SHOP_TAB_COUNT + 1, rootH, rootGap);
    DrawSharedMenuDim(sw, sh, mainX, shopRailY, mainBW, shopRailH,
                      std::max(oldMenuA, wake), true);

    // SHOP keeps the live background visible, but an optional soft backdrop
    // blur removes the high-frequency text/windows that compete with the
    // constellation labels. Capture only after the persistent dim layer and
    // before any SHOP UI is drawn, so the UI itself always stays sharp.
    if (g_BackdropBlurEnabled) {
        InitBlurSystem((int)sw, (int)sh);
        CaptureBackdrop(BackdropBlurCaptureIntervalSeconds());
        DrawBlurPanel(0.0f, 0.0f, sw, sh,
                      0.16f * std::max(oldMenuA, wake),
                      0.004f, 0.010f, 0.020f);
    }
    const InlineThreeColumnLayout columns = BuildInlineThreeColumnLayout(
        sw, sh, uiS, rootX, rootY, rootW, rootH);
    // ARMORY keeps the main-menu command rail, then opens one RUN_CONFIG-sized
    // work surface. The item tree and transaction detail are columns inside
    // that surface rather than two unrelated floating cards.
    const float workY = std::max(74.0f, sh * 0.12f);
    const float workBottom = sh - std::max(88.0f, 104.0f * uiS);
    const float workH = std::max(520.0f * uiS, workBottom - workY);
    const float workX = columns.contentX - 18.0f * uiS;
    const float workRight = sw - std::max(32.0f, 38.0f * uiS);
    const float workW = std::max(780.0f * uiS, workRight - workX);
    // These surfaces are intentionally borderless and bounded to readable
    // content. A real fill is required on bright desktop/game backgrounds;
    // text shadows alone cannot establish enough local contrast.
    // ARMORY uses the codex-style persistent category rail: categories stay
    // visible on the left while the selected tab's nodes remain visible beside
    // it. The first tab is selected by default, but is not a separate screen.
    const float categoryA = 1.0f;
    const float itemA = SceneTransitionEase(
        (g_ShopEntryT - 0.08f) / (kOutgameTransitionDuration - 0.08f));
    const float detailWake = rightWake * itemA;
    if (detailWake > 0.002f) {
        // Keep ARMORY surface-free like the codex: only the connecting
        // constellation rules remain, with no opaque work plate behind them.
        DrawAstralDataPlate(workX, workY, workW, workH,
                            0.38f, 0.82f, 1.0f, 0.0f);
    }
    static constexpr int KEY_META  = 0;
    static constexpr int KEY_THEME = 100;
    static constexpr int KEY_AUG   = 1000;

    // SHOP ownership is independent from PLAY loadout state. A core module
    // becomes owned after its first level; profiles use their save bit.
    auto shopItemPurchased = [&](int key) {
        if (key >= KEY_AUG) return false;
        if (key >= KEY_THEME) {
            const int themeId = key - KEY_THEME;
            return themeId >= 0 && themeId < ACCENT_COUNT
                && ThemeOwned(themeId);
        }
        const int metaId = key - KEY_META;
        return metaId >= 0 && metaId < META_COUNT && g_MetaLv[metaId] > 0;
    };

    auto augWeapCat = [](AugType t) -> int {
        switch (t) {
        case AugType::RIFLE_STABILITY:
            return 0;
        case AugType::STATIC_FIELD:
        case AugType::STATIC_FIELD_2:
            return 1;
        default:
            return -1;
        }
    };

    if (s_prevTab != s_tab) {
        ShopStaticFieldPalette(s_tab, s_fieldTargetR,
                               s_fieldTargetG, s_fieldTargetB);
        if (s_fieldColorTab < 0) {
            s_fieldColorR = s_fieldTargetR;
            s_fieldColorG = s_fieldTargetG;
            s_fieldColorB = s_fieldTargetB;
        }
        s_fieldColorTab = s_tab;
        s_prevTab   = s_tab;
        s_itemCount = 0;
        s_scroll    = 0.0f;
        for (float& hover : s_itemHov) hover = 0.0f;

        auto addGroup = [&](const wchar_t* lbl, float r, float g2, float b) {
            if (s_itemCount < 300) s_items[s_itemCount++] = { true, -1, lbl, r, g2, b };
        };
        auto addItem = [&](int key, const wchar_t* lbl, float r, float g2, float b) {
            if (s_itemCount < 300) s_items[s_itemCount++] = { false, key, lbl, r, g2, b };
        };

        if (s_tab == 0) {
            addGroup(nli == 0 ? L"\xCF54\xC5B4 \xD574\xAE08" : L"CORE UNLOCKS",
                     0.38f, 0.82f, 1.00f);
            for (int i = 0; i < META_COUNT; i++)
                if (i != META_RESERVED_STARTAUG)
                    addItem(KEY_META + i, MetaName(i), 0.38f, 0.82f, 1.00f);
            addGroup(nli == 0 ? L"\xD654\xBA74 \xD504\xB85C\xD30C\xC77C" : L"DISPLAY PROFILES",
                     1.00f, 0.84f, 0.28f);
            for (int i = 1; i < ACCENT_COUNT; ++i)
                addItem(KEY_THEME + i, AccentName(i),
                        ACCENT_THEMES[i].r, ACCENT_THEMES[i].g, ACCENT_THEMES[i].b);
        }
        if (s_tab == 1) {
            addGroup(nli == 0 ? L"\xC18C\xCD1D \xC2DC\xC2A4\xD15C" : L"RIFLE CONSTELLATION", 0.72f, 0.92f, 0.42f);
            for (int i = 0; i < AUG_TOTAL; i++)
                if (!AugRemoved(ALL_AUGS[i].type) && augWeapCat(ALL_AUGS[i].type) == 0)
                    addItem(KEY_AUG + i, ALL_AUGS[i].locName[li], 0.72f, 0.92f, 0.42f);
        }
        if (s_tab == 2) {
            addGroup(nli == 0 ? L"\xC704\xC131 \xBC30\xC5F4" : L"FIELD ARRAY", 0.62f, 0.48f, 1.00f);
            for (int i = 0; i < AUG_TOTAL; i++)
                if (!AugRemoved(ALL_AUGS[i].type) && augWeapCat(ALL_AUGS[i].type) == 1)
                    addItem(KEY_AUG + i, ALL_AUGS[i].locName[li], 0.62f, 0.48f, 1.00f);
        }

        s_listTransitionDir = 1.0f;
        s_listFromKey = s_selKey;
        s_listTransitionT = 0.0f;
        s_selKey = -1;
        for (int i = 0; i < s_itemCount; i++)
            if (!s_items[i].isGroup) { s_selKey = s_items[i].key; break; }
    }

    // Keep the static field stable between tabs; only a category change
    // eases it toward the next palette.
    s_fieldColorR = UiApproach(s_fieldColorR, s_fieldTargetR, dt, 3.8f);
    s_fieldColorG = UiApproach(s_fieldColorG, s_fieldTargetG, dt, 3.8f);
    s_fieldColorB = UiApproach(s_fieldColorB, s_fieldTargetB, dt, 3.8f);

    if (s_selKey != s_prevKey) {
        s_listFromKey = s_prevKey;
        s_listTransitionT = 0.0f;
        s_prevKey = s_selKey;
        s_detailT = 0.0f;
    }
    if (s_selKey != s_prevTagKey) { s_prevTagKey = s_selKey; s_tagFlickerT = 0.0f; }
    s_detailT = UiApproach(s_detailT, 1.0f, dt, 6.0f);
    s_listTransitionT = UiApproach(s_listTransitionT, 1.0f, dt, 8.5f);
    s_tagFlickerT = UiApproach(s_tagFlickerT, 1.0f, dt, 16.0f);

    // SHOP_ARCHIVE_RENDERER
    // This intentionally follows the Codex renderer's composition: one large
    // orbital field, a curved observation rail of entries, and a fixed record
    // block. SHOP only changes where that archive field lives and what the
    // record block can purchase.
    {
        BeginDeferredSceneText();
        const float designA = std::max(0.0f, std::min(1.0f, wake));
        const float detailA = std::max(0.0f, std::min(1.0f, rightWake * s_detailT));
        // CircleTexture layers are page-owned: they reveal once with the page
        // and then remain stable while records move along the rail.  This is
        // deliberately independent from designA/detailA so list selection
        // cannot make the static fields disappear and re-render.
        const float textureReveal = s_backExit
            ? std::max(0.0f, 1.0f - backP)
            : SceneTransitionEase(g_ShopEntryT / kOutgameTransitionDuration);
        SetSceneTextureReveal(textureReveal);
        const float staticFieldA = 1.0f;
        const float staticGeometryA = textureReveal;
        const float ui = std::max(0.62f, std::min(sw * 0.90f / 1800.0f,
                                                  sh * 0.86f / 1020.0f));
        const float cyanR = 0.30f, cyanG = 0.86f, cyanB = 1.00f;
        // Static field lights use the selected category palette.  Their
        // colour is eased in only when the category changes, never animated
        // continuously while the user is browsing one category.
        const float fieldR = s_fieldColorR;
        const float fieldG = s_fieldColorG;
        const float fieldB = s_fieldColorB;
        const float goldR = 1.00f, goldG = 0.76f, goldB = 0.24f;
        const float whiteR = 0.88f, whiteG = 0.94f, whiteB = 1.00f;
        // Let the item explanation lead the hierarchy. Purchase/status copy
        // stays readable but subordinate, while constellation labels use the
        // same larger explanatory scale.
        const float detailDescriptionSc = UiTextScale(
            g_TextS, UiTextLevel::Description, ui);
        const float purchaseInfoSc = UiTextScale(
            g_TextS, UiTextLevel::Supporting, ui);
        const float purchaseButtonSc = UiTextScale(
            g_TextL, UiTextLevel::Title, ui);
        const bool lmbClick = lmb && !g_LmbPrev;

        auto drawFit = [&](const wchar_t* text, float x, float y, float scale,
                           float maxW, float r, float g2, float b, float a) {
            if (!text || !text[0] || maxW <= 2.0f) return;
            float sc = scale;
            const float tw = g_TextS.Width(text, sc);
            if (tw > maxW && tw > 0.0f) sc *= maxW / tw;
            DrawShadowedText(g_TextS, text, x, y, sc, r, g2, b,
                             a * designA, 0.84f);
        };
        auto drawRightFit = [&](TextRenderer& renderer, const wchar_t* text,
                                float rightX, float y, float scale, float maxW,
                                float r, float g2, float b, float a,
                                float shadow = 0.84f) {
            if (!text || !text[0] || maxW <= 2.0f) return;
            float sc = scale;
            float tw = renderer.Width(text, sc);
            if (tw > maxW && tw > 0.0f) {
                sc *= maxW / tw;
                tw = renderer.Width(text, sc);
            }
            DrawShadowedText(renderer, text, rightX - tw, y, sc,
                             r, g2, b, a * designA, shadow);
        };
        auto drawModuleGlyph = [&](int key, float cx, float cy, float size,
                                   float r, float g2, float b, float a) {
            if (key >= KEY_AUG && key - KEY_AUG >= 0 && key - KEY_AUG < AUG_TOTAL) {
                DrawAugIcon(ALL_AUGS[key - KEY_AUG].type,
                            cx - size * 0.5f, cy - size * 0.5f, size,
                            r, g2, b, a * designA);
                return;
            }
            BindMainShader();
            const float h = size * 0.5f;
            const float t = std::max(1.0f * ui, size * 0.045f);
            drawRect(cx - h, cy - h, size, t, r, g2, b, a * designA);
            drawRect(cx - h, cy + h - t, size, t, r, g2, b, a * designA);
            drawRect(cx - h, cy - h, t, size, r, g2, b, a * designA);
            drawRect(cx + h - t, cy - h, t, size, r, g2, b, a * designA);
            LogoLine(cx - size * 0.22f, cy, cx + size * 0.22f, cy,
                     t, r, g2, b, a * designA);
            LogoLine(cx, cy - size * 0.22f, cx, cy + size * 0.22f,
                     t, r, g2, b, a * designA);
            drawDiamond(cx, cy, size * 0.10f, r, g2, b, a * designA);
        };
        enum ShopActionState {
            SHOP_ACTION_READY = 0,
            SHOP_ACTION_ACTIVE,
            SHOP_ACTION_BLOCKED
        };
        auto drawFixedAction = [&](float x, float y, float w, const wchar_t* label,
                                   float r, float g2, float b,
                                   bool enabled, bool hovered,
                                   ShopActionState state) {
            const bool hot = enabled && hovered;
            static float actionColorR = 0.28f;
            static float actionColorG = 0.88f;
            static float actionColorB = 1.00f;
            static float actionHoverT = 0.0f;
            static float actionEnabledT = 1.0f;
            static bool actionColorReady = false;
            // Give the action a little more vertical breathing room. The
            // matching hitbox is expanded below so the larger ribbon remains
            // comfortable to click.
            const float top = y - 54.0f * ui;
            const float bottom = y + 40.0f * ui;
            float buttonR = r, buttonG = g2, buttonB = b;
            if (state == SHOP_ACTION_READY) {
                buttonR = 0.28f; buttonG = 0.88f; buttonB = 1.00f;
            } else if (state == SHOP_ACTION_ACTIVE) {
                buttonR = 0.72f; buttonG = 0.98f; buttonB = 0.96f;
            } else if (state == SHOP_ACTION_BLOCKED) {
                buttonR = 0.96f; buttonG = 0.30f; buttonB = 0.34f;
            }
            if (!actionColorReady) {
                actionColorR = buttonR;
                actionColorG = buttonG;
                actionColorB = buttonB;
                actionColorReady = true;
            }
            // State colour, hover glow and disabled fade all settle through
            // the same short UI interpolation so changing records never
            // causes the action ribbon to snap between colours.
            actionColorR = UiApproach(actionColorR, buttonR, dt, 8.0f);
            actionColorG = UiApproach(actionColorG, buttonG, dt, 8.0f);
            actionColorB = UiApproach(actionColorB, buttonB, dt, 8.0f);
            actionHoverT = UiApproach(actionHoverT, hot ? 1.0f : 0.0f,
                                      dt, 10.0f);
            actionEnabledT = UiApproach(actionEnabledT,
                                        enabled ? 1.0f : 0.0f, dt, 8.0f);
            const float aa = 0.28f + 0.44f * actionEnabledT
                           + 0.28f * actionEnabledT * actionHoverT;
            const float cut = std::min(56.0f * ui, w * 0.24f);
            const float fillA = (0.34f + 0.18f * actionHoverT)
                              * aa * designA;
            // A soft dark ribbon beneath the coloured one separates the
            // action from nearby constellation labels without introducing a
            // rectangular panel or a new opaque layout surface.
            const float dimX = x - 12.0f * ui;
            const float dimTop = top - 7.0f * ui;
            const float dimW = w + 12.0f * ui;
            const float dimH = (bottom - top) + 14.0f * ui;
            const float dimCut = std::min(56.0f * ui, dimW * 0.24f);
            DrawLinearGradientRibbon(dimX, dimTop, dimW, dimH, dimCut,
                                     0.0f, 0.0f, 0.012f,
                                     (0.34f + 0.12f * actionEnabledT)
                                         * designA,
                                     true);
            // The action is a directional ribbon. Its slanted end points back
            // into the detail record without adding a rigid button frame.
            DrawLinearGradientRibbon(x, top, w, bottom - top, cut,
                                     actionColorR, actionColorG, actionColorB,
                                     fillA, true);
            // A small lock-on node gives the action endpoint a game-like
            // interaction cue without changing the ribbon geometry.
            const float actionNodeX = x + w - cut * 0.42f;
            const float actionNodePulse = 0.30f + 0.26f * actionHoverT;
            DrawVisibleConstellNode(actionNodeX, y + 1.0f * ui,
                                    (2.6f + 1.0f * actionHoverT) * ui,
                                    actionColorR, actionColorG, actionColorB,
                                    actionNodePulse * aa * designA, false);
            // The action label uses one stable high-contrast colour. State
            // colours belong to the ribbon itself; changing them on the text
            // makes the control look disabled or errored at a glance.
            float textSc = purchaseButtonSc;
            float textW = g_TextL.Width(label, textSc);
            const float textRight = x + w - cut * 0.56f - 18.0f * ui;
            const float textMaxW = std::max(48.0f * ui,
                                            textRight - x - 18.0f * ui);
            if (textW > textMaxW && textW > 0.0f) {
                textSc *= textMaxW / textW;
                textW = g_TextL.Width(label, textSc);
            }
            const float textR = 0.94f;
            const float textG = 0.98f;
            const float textB = 1.00f;
            DrawShadowedText(g_TextL, label, textRight - textW,
                             y - 20.0f * ui, textSc, textR, textG, textB,
                             0.56f + 0.42f * actionEnabledT, 0.84f);
        };

        DrawPersistentSceneLeftVignette(sw, sh, 0.055f);

        // Keep this loop byte-for-byte in the same visual family as the other
        // pages' shared root commands.
        static const wchar_t* kShopRoute[SHOP_TAB_COUNT][3] = {
            { L"\xD574\xAE08", L"UNLOCK", L"\u89E3\u653E" },
            { L"\xC18C\xCD1D", L"RIFLE", L"\u30E9\u30A4\u30D5\u30EB" },
            { L"\xC704\xC131", L"FIELD", L"\u30D5\u30A3\u30FC\u30EB\u30C9" },
        };
        static const wchar_t* kShopBack[3] = { L"\uB4A4\uB85C", L"BACK", L"\u623B\u308B" };
        const int shopLanguage = std::max(0, std::min(2, LangIndex()));
        static const float kShopR[SHOP_TAB_COUNT] = { 0.38f, 0.72f, 0.62f };
        static const float kShopG[SHOP_TAB_COUNT] = { 0.82f, 0.92f, 0.48f };
        static const float kShopB[SHOP_TAB_COUNT] = { 1.00f, 0.42f, 1.00f };
        const float railX = rootX;
        const float railY = shopRailY;
        const float railW = rootW;
        const float railH = rootH;
        const float categoryWake = wake * categoryA;
        for (int i = 0; i < SHOP_TAB_COUNT + 1; ++i) {
            const bool isBack = i == SHOP_TAB_COUNT;
            const float tx = railX;
            const float ty = railY + (float)i * (railH + rootGap);
            const float reveal = s_backExit ? wake : MenuCommandReveal(g_ShopEntryT, i);
            float& hovT = isBack ? s_backHov : s_tabHov[i];
            const float hitX = tx + (1.0f - reveal) * 34.0f - 10.0f * hovT;
            const bool hov = inputReady
                          && PanelButtonHit(mx, my, hitX, ty, railW, railH,
                                            18.0f * ui);
            hovT = UpdateMenuCommandHover(hovT, hov, dt);
            if (hov && lmbClick) {
                if (isBack) beginBack();
                else {
                    s_tab = i;
                    s_prevTab = -1;
                    s_browseItems = true;
                    s_browseInputLock = true;
                }
            }
            const bool selected = !isBack && s_tab == i;
            const float rr = isBack ? 0.42f : kShopR[i];
            const float gg = isBack ? 0.62f : kShopG[i];
            const float bb = isBack ? 0.78f : kShopB[i];
            const wchar_t* route = isBack ? kShopBack[shopLanguage]
                                          : kShopRoute[i][shopLanguage];
            const wchar_t* routeVariants[3] = {
                isBack ? kShopBack[0] : kShopRoute[i][0],
                isBack ? kShopBack[1] : kShopRoute[i][1],
                isBack ? kShopBack[2] : kShopRoute[i][2]
            };
            const float rowA = reveal * categoryWake;
            // Once SHOP is interactive, every category label remains fully
            // readable. Only the page exit is allowed to lower the rail.
            const float tabRowA = (!s_backExit && g_ShopEntryT >= 0.55f)
                ? 1.0f : rowA;
            const float selectPulse = selected
                ? 0.16f + 0.16f * sinf(now * 4.0f) : 0.0f;
            const float slide = (1.0f - reveal) * 34.0f;
            DrawUnifiedMenuCommand(route,
                                   tx + slide - 10.0f * hovT, ty,
                                   railW, railH, rr, gg, bb, tabRowA, hovT,
                                   selected, selectPulse,
                                   now + (float)i * 0.17f,
                                   uiS, true, false, false,
                                   routeVariants, 3);
        }

        float detailR = cyanR, detailG = cyanG, detailB = cyanB;
        const wchar_t* detailTitle = L"모듈 없음";
        const wchar_t* detailType = L"상점 항목";
        bool detailIsMeta = false, detailIsTheme = false, detailIsAug = false;
        int detailId = -1;
        if (s_selKey >= 0) {
            if (s_selKey < KEY_THEME) {
                detailIsMeta = true; detailId = s_selKey - KEY_META;
                detailType = L"코어 모듈";
                if (detailId >= 0 && detailId < META_COUNT) {
                    detailTitle = MetaName(detailId);
                }
            } else if (s_selKey < KEY_AUG) {
                detailIsTheme = true; detailId = s_selKey - KEY_THEME;
                detailType = L"화면 프로필";
                if (detailId >= 0 && detailId < ACCENT_COUNT) {
                    detailTitle = AccentName(detailId);
                    detailR = ACCENT_THEMES[detailId].r;
                    detailG = ACCENT_THEMES[detailId].g;
                    detailB = ACCENT_THEMES[detailId].b;
                }
            } else {
                detailIsAug = true; detailId = s_selKey - KEY_AUG;
                detailType = L"페이로드 모듈";
                if (detailId >= 0 && detailId < AUG_TOTAL) {
                    detailTitle = ALL_AUGS[detailId].locName[li];
                    GetRarityColor(ALL_AUGS[detailId].rarity,
                                   detailR, detailG, detailB);
                }
            }
        }

        // The shop constellation is intentionally oversized and right-biased,
        // like the Codex's corner-anchored chart. Its origin sits near the
        // upper-right edge while the visible product rail falls inward as a
        // giant lower semicircle. This makes the constellation the stage
        // instead of a small illustration floating between the UI columns.
        const float chartCX = sw * 0.30f;
        const float chartCY = sh * 0.10f;
        const float chartR = std::min(sw * 0.52f, sh * 0.70f);
        const float itemR = chartR * 0.94f;
        const float rowStep = std::min(250.0f * ui, sh * 0.24f);
        // The catalogue follows one clean lower semicircle. The screen-space
        // Y axis grows downward, so 0 -> PI draws only the lower half.
        const float railRadius = chartR * 0.68f;
        const float railStartAngle = 0.02f;
        const float railSweep = 3.10f;
        const auto railXAt = [&](float row) {
            const float angle = railStartAngle + railSweep * (row / 4.0f);
            return chartCX + cosf(angle) * railRadius;
        };
        const auto railYAt = [&](float row) {
            const float angle = railStartAngle + railSweep * (row / 4.0f);
            return chartCY + sinf(angle) * railRadius;
        };
        // The record readout is a right-center observation block. Keeping its
        // anchor above the lower HUD leaves the lower-right CircleTexture as
        // atmosphere instead of forcing every line of copy into the corner.
        const float detailX = sw * 0.63f;
        const float detailW = std::max(360.0f * ui,
                                       std::min(sw * 0.29f,
                                                sw - detailX - 42.0f * ui));
        // Lift the complete readout slightly so the wallet/status rows keep
        // clear air above the bottom action ribbon.
        const ShopDetailLayout detailLayout = MakeShopDetailLayout(
            detailX, sh * 0.43f, detailW, sh * 0.91f, ui);
        const float detailRight = detailLayout.right;
        const float infoY = detailLayout.titleY;

        if (g_ConstellationCircleTex && textureReveal > 0.001f) {
            // Normalized world-space corner: (1, 1) is the lower-right
            // background corner. The radius is sized from the farthest
            // upper-left detail text so the whole record stays in the dark
            // texture field without adding an opaque panel.
            const float detailTextLeft = detailX - 32.0f * ui;
            const float detailTextTop = detailLayout.top - 28.0f * ui;
            const float detailReach = sqrtf(
                (sw - detailTextLeft) * (sw - detailTextLeft)
                + (sh - detailTextTop) * (sh - detailTextTop));
            // The readout owns the entire lower-right field.  Grow the halo
            // beyond the measured text reach so the title, copy, price and
            // action all sit inside one continuous contrast well.
            const float circleR = std::max(std::min(sw, sh) * 1.04f,
                                           detailReach * 1.28f);
            const float circleCX = sw * 0.95f;
            const float circleCY = sh * 0.95f;
            BatchFlush();
            DrawIcon(g_ConstellationCircleTex,
                     circleCX - circleR, circleCY - circleR,
                     circleR * 2.0f, circleR * 2.0f,
                     0.0f, 0.0f, 0.012f, 0.80f * textureReveal);
            // Inner chromatic field shares the exact (1, 1) corner origin and
            // remains inside the black visibility halo.
            const float colorCircleR = circleR * 0.85f;
            DrawIcon(g_ConstellationCircleTex,
                     circleCX - colorCircleR, circleCY - colorCircleR,
                     colorCircleR * 2.0f, colorCircleR * 2.0f,
                     fieldR, fieldG, fieldB, 0.055f * textureReveal);
        }

        int itemIndices[300] = {};
        int itemTotal = 0;
        int selectedSlot = 0;
        for (int i = 0; i < s_itemCount; ++i) {
            if (s_items[i].isGroup) continue;
            if (itemTotal < 300) {
                itemIndices[itemTotal] = i;
                if (s_items[i].key == s_selKey) selectedSlot = itemTotal;
                ++itemTotal;
            }
        }

        if (itemTotal > 0) {
            const bool overOrbit = inputReady
                && mx >= chartCX - itemR - 100.0f * ui
                && mx <= sw
                && my >= chartCY - rowStep * 4.6f
                && my <= chartCY + rowStep * 4.6f;

            // Match the Codex list controller: W/S (and arrow keys) use an
            // immediate first step followed by a held-key repeat, while the
            // pointer can scrub the same vertical rail by dragging. All input
            // paths feed one step request so the orbit transition stays
            // identical regardless of how the item was changed.
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
                    stepRequest = navDir;
                } else {
                    s_navHoldT += dt;
                    if (s_navHoldT >= 0.25f) {
                        s_navRepeatT += dt;
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
                const float dragStepThreshold = rowStep * 0.88f;
                if (fabsf(s_dragAccum) >= dragStepThreshold) {
                    stepRequest = s_dragAccum > 0.0f ? -1 : 1;
                    s_dragAccum += s_dragAccum > 0.0f
                        ? -dragStepThreshold : dragStepThreshold;
                    s_dragMoved = true;
                }
            }

            if (overOrbit && g_ScrollAccum != 0.0f)
                stepRequest = g_ScrollAccum > 0.0f ? -1 : 1;
            g_ScrollAccum = 0.0f;

            if (stepRequest != 0) {
                selectedSlot = (selectedSlot + stepRequest + itemTotal) % itemTotal;
                s_listTransitionDir = (float)stepRequest;
                s_listFromKey = s_selKey;
                s_listTransitionT = 0.0f;
                s_selKey = s_items[itemIndices[selectedSlot]].key;
                s_detailT = 0.0f;
            }
            selectedSlot = 0;
            for (int i = 0; i < itemTotal; ++i)
                if (s_items[itemIndices[i]].key == s_selKey) selectedSlot = i;

            int fromSlot = -1;
            for (int i = 0; i < itemTotal; ++i)
                if (s_items[itemIndices[i]].key == s_listFromKey) fromSlot = i;
            const bool hasFrom = fromSlot >= 0 && fromSlot != selectedSlot;
            const float listEase = Smoothstep(s_listTransitionT);
            const auto listRelFor = [&](int slot, int focusSlot) {
                if (itemTotal <= 1) return 2.0f;
                int offset = slot - focusSlot;
                while (offset > itemTotal / 2) offset -= itemTotal;
                while (offset < -itemTotal / 2) offset += itemTotal;
                // Five positions: two before, selected middle, two after.
                if (offset < -2 || offset > 2) return -1000.0f;
                return (float)(offset + 2);
            };
            const bool listTransitionActive = hasFrom && listEase < 0.995f;
            // Input remains live while records travel.  Only the candidate
            // resolution is consolidated so overlapping animated hit regions
            // cannot let render-loop order change the selected item.
            const bool listClickReady = inputReady;
            int clickSlot = -1;
            float clickDist2 = 1.0e30f;
            // These fields belong to the SHOP viewport itself. They must not
            // inherit s_detailT, because selecting another record should not
            // make the fixed chart/background CircleTextures fade and redraw
            // as if they were part of the record being replaced.
            // Static chart field: this anchor belongs to the constellation
            // chart, not to the selected record, so it never follows list
            // transitions. Record nodes travel over this fixed field.
            const float staticChartCX = chartCX;
            const float staticChartCY = chartCY;
            const float codexFieldCX = staticChartCX - chartR * 0.10f;
            // Chromatic origin marker for the chart's fixed local (0, 0).
            DrawConstellationDisc(staticChartCX, staticChartCY, chartR * 0.82f,
                                  fieldR, fieldG, fieldB,
                                  0.11f * staticFieldA);
            DrawConstellationDisc(staticChartCX, staticChartCY,
                                  chartR * 0.56f, fieldR, fieldG, fieldB,
                                  0.04f * staticFieldA);
            // Draw the black contrast field after the chromatic origin so a
            // dark overlap remains dark instead of being lightened by it.
            DrawConstellationDisc(codexFieldCX, staticChartCY,
                                  chartR * 1.22f, 0.0f, 0.0f, 0.0f,
                                  0.16f * staticFieldA, false);
            // A large local black field behind the focused middle entry gives
            // the active constellation the same layered depth as the Codex
            // record viewer, without introducing an opaque panel.
            const float focusCX = railXAt(2.0f);
            const float focusCY = railYAt(2.0f);
            DrawConstellationDisc(focusCX, focusCY, chartR * 0.16f,
                                  fieldR, fieldG, fieldB,
                                  0.09f * staticFieldA);
            DrawConstellationDisc(focusCX - chartR * 0.08f, focusCY,
                                  chartR * 0.26f,
                                  0.0f, 0.0f, 0.012f,
                                  0.30f * staticFieldA);
            BindMainShader();
            for (int ring = 0; ring < 4; ++ring) {
                const float rr = chartR * (0.525f + 0.125f * (float)ring);
                const int ringSegments = 30;
                for (int j = 0; j < ringSegments; ++j) {
                    if ((j + ring * 2) % 5 == 3) continue;
                    const float a0 = railStartAngle + (float)j * railSweep / (float)ringSegments;
                    const float a1 = railStartAngle + ((float)j + 0.66f) * railSweep / (float)ringSegments;
                    LogoLine(chartCX + cosf(a0) * rr, chartCY + sinf(a0) * rr,
                             chartCX + cosf(a1) * rr, chartCY + sinf(a1) * rr,
                             (ring == 3 ? 1.0f : 0.65f) * ui,
                             fieldR, fieldG, fieldB,
                             (ring == 3 ? 0.095f : 0.042f) * staticGeometryA);
                }
            }
            // Keep one faint, unbroken rail underneath the segmented Codex
            // rings. It belongs to the static chart, so it never vanishes or
            // breaks while records are travelling between list positions.
            const int continuousRailSegments = 96;
            for (int j = 0; j < continuousRailSegments; ++j) {
                const float a0 = railStartAngle
                    + railSweep * (float)j / (float)continuousRailSegments;
                const float a1 = railStartAngle
                    + railSweep * ((float)j + 1.0f)
                        / (float)continuousRailSegments;
                LogoLine(chartCX + cosf(a0) * railRadius,
                         chartCY + sinf(a0) * railRadius,
                         chartCX + cosf(a1) * railRadius,
                         chartCY + sinf(a1) * railRadius,
                         0.46f * ui,
                         fieldR, fieldG, fieldB,
                         0.052f * staticGeometryA);
            }

            // Secondary Codex-style orbit system: three smaller elliptical
            // tracks that repeat the same lower-half silhouette, plus anchor
            // lights. There is deliberately no upper-half track.
            for (int track = 0; track < 3; ++track) {
                const float rx = chartR * (0.28f + 0.105f * (float)track);
                const float ry = rx * (0.54f + 0.07f * (float)track);
                const float start = 0.08f + 0.05f * (float)track;
                const float sweep = 2.96f - 0.10f * (float)track;
                const int segments = 24;
                for (int seg = 0; seg < segments; ++seg) {
                    if ((seg + track * 2) % 5 == 2) continue;
                    const float a0 = start + sweep * (float)seg / (float)segments;
                    const float a1 = start + sweep * ((float)seg + 0.62f) / (float)segments;
                    LogoLine(chartCX + cosf(a0) * rx,
                             chartCY + sinf(a0) * ry,
                             chartCX + cosf(a1) * rx,
                             chartCY + sinf(a1) * ry,
                             (0.52f + 0.12f * (float)track) * ui,
                             fieldR, fieldG, fieldB,
                             (0.072f - 0.012f * (float)track) * staticGeometryA);
                }
                const float anchorA = start + sweep * 0.86f;
                DrawVisibleConstellNode(chartCX + cosf(anchorA) * rx,
                                        chartCY + sinf(anchorA) * ry,
                                        (2.0f - 0.18f * (float)track) * ui,
                                        fieldR, fieldG, fieldB,
                                        (0.36f - 0.05f * (float)track) * staticGeometryA,
                                        false);
            }
            float chainX[5] = {}, chainY[5] = {};
            float chainR[5] = {}, chainG[5] = {}, chainB[5] = {};
            float chainA[5] = {};
            bool chainValid[5] = {};
            float selectedNodeX = railXAt(2.0f);
            float selectedNodeY = railYAt(2.0f);
            for (int slot = 0; slot < itemTotal; ++slot) {
                const int itemIndex = itemIndices[slot];
                const SLItem& item = s_items[itemIndex];
                const float newRel = listRelFor(slot, selectedSlot);
                const float oldRel = hasFrom ? listRelFor(slot, fromSlot) : newRel;
                const bool oldVisible = oldRel > -999.0f;
                const bool newVisible = newRel > -999.0f;
                if (!oldVisible && !newVisible) continue;
                const float sourceRel = oldVisible
                    ? oldRel : newRel + s_listTransitionDir;
                const float targetRel = newVisible
                    ? newRel : oldRel - s_listTransitionDir;
                const float drawRel = sourceRel + (targetRel - sourceRel) * listEase;
                const float itemFade =
                    (oldVisible ? (1.0f - listEase) : 0.0f)
                    + (newVisible ? listEase : 0.0f);
                const bool oldSelected = hasFrom && slot == fromSlot;
                const bool newSelected = slot == selectedSlot;
                const float focus = std::max(
                    oldSelected ? (1.0f - listEase) : 0.0f,
                    newSelected ? listEase : 0.0f);
                const float hover = s_itemHov[itemIndex];
                const float active = std::max(focus, hover);
                const bool isCoreNode = item.key < KEY_THEME;
                const bool isProfileNode = item.key >= KEY_THEME
                                        && item.key < KEY_AUG;
                // Core unlocks are the gameplay-critical records. The whole
                // shop chart is large, so give its product constellations a
                // matching scale instead of leaving tiny nodes on a giant
                // field. Profiles and payloads remain slightly lighter.
                // Reduce the constellation body itself, including the
                // selected middle entry. Hover only changes emphasis/alpha;
                // it must never make a constellation grow again.
                const float baseMiniR = (isCoreNode ? 88.0f : 72.0f)
                                      + (isCoreNode ? 78.0f : 58.0f) * focus;
                // Hover is a readability state, not a scale-up state. Keep a
                // small shrink while the pointer rests on a record so the
                // constellation never swells against its label.
                const float hoverPreview = s_itemHov[itemIndex];
                const float miniR = baseMiniR * (1.0f - 0.15f * hoverPreview);
                const float miniRadius = miniR * ui;
                const float drawAx = railXAt(drawRel);
                const float drawAy = railYAt(drawRel);
                const bool itemPurchased = shopItemPurchased(item.key);
                // Purchased entries retain their own palette. Unpurchased
                // entries stay neutral; the selected one receives only a
                // restrained cyan focus so it does not read as owned.
                const float visualR = itemPurchased ? item.r
                    : (newSelected ? cyanR : 0.42f);
                const float visualG = itemPurchased ? item.g
                    : (newSelected ? cyanG : 0.52f);
                const float visualB = itemPurchased ? item.b
                    : (newSelected ? cyanB : 0.62f);
                const float labelW = std::max(180.0f * ui,
                                              std::min(300.0f * ui,
                                                       sw * 0.18f));
                // Hit testing belongs to the constellation node only. The
                // right-side label is intentionally excluded; including its
                // width made neighbouring records share one hover region.
                const float hoverPad = 16.0f * ui;
                const float hitRadiusX = baseMiniR * ui * 0.82f + hoverPad;
                const float hitRadiusY = baseMiniR * ui * 0.72f + hoverPad;
                const bool hov = inputReady
                              && mx >= drawAx - hitRadiusX
                              && mx <= drawAx + hitRadiusX
                              && my >= drawAy - hitRadiusY
                              && my <= drawAy + hitRadiusY;
                s_itemHov[itemIndex] = UpdateMenuCommandHover(s_itemHov[itemIndex], hov, dt);
                if (hov && lmbClick && listClickReady && !s_dragMoved) {
                    // Multiple large constellations may touch at their edge.
                    // Resolve the press once, by nearest node, instead of
                    // allowing iteration order to decide which item wins.
                    const float dx = (float)mx - drawAx;
                    const float dy = (float)my - drawAy;
                    const float dist2 = dx * dx + dy * dy;
                    if (dist2 < clickDist2) {
                        clickDist2 = dist2;
                        clickSlot = slot;
                    }
                }
                const bool fullItemFields = !listTransitionActive
                                          || oldSelected || newSelected;
                // The node field follows the travelling constellation itself.
                // Records that remain in the five visible slots keep their
                // field at full strength throughout the move; only records
                // entering or leaving the rail use the edge fade. This avoids
                // the CircleTexture appearing only after the list settles.
                const float nodeFieldA = designA
                    * ((oldVisible && newVisible) ? 1.0f : itemFade);
                DrawConstellationDisc(drawAx, drawAy, miniRadius * 0.90f,
                                      visualR, visualG, visualB,
                                      (itemPurchased
                                           ? (0.100f + 0.060f * focus)
                                           : (0.018f + 0.052f * focus))
                                          * nodeFieldA);
                DrawConstellationDisc(drawAx, drawAy, miniRadius * 1.32f,
                                      0.0f, 0.0f, 0.0f,
                                      (0.14f + 0.06f * active) * nodeFieldA,
                                      false);
                DrawConstellationDisc(drawAx, drawAy, miniRadius * 1.08f,
                                      0.0f, 0.0f, 0.0f,
                                      (0.26f + 0.20f * focus) * nodeFieldA);
                DrawArchiveConstellation(drawAx, drawAy, miniRadius,
                                         isCoreNode ? 3 : (item.key >= KEY_AUG ? 2 : 1),
                                         item.key, now * 0.18f,
                                         visualR, visualG, visualB,
                                         (itemPurchased
                                              ? (0.34f + 0.48f * focus + 0.28f * hover)
                                              : (0.20f + 0.34f * focus + 0.16f * hover))
                                             * designA * itemFade,
                                         ui, fullItemFields);
                DrawVisibleConstellLine(drawAx + miniRadius * 0.74f,
                                        drawAy,
                                        drawAx + miniRadius + 15.0f * ui,
                                        drawAy,
                                        (0.68f + 0.72f * focus) * ui,
                                        visualR, visualG, visualB,
                                        (itemPurchased
                                             ? (0.18f + 0.44f * focus + 0.18f * hover)
                                             : (0.10f + 0.30f * focus + 0.12f * hover))
                                            * designA * itemFade);
                DrawVisibleConstellNode(drawAx + miniRadius + 15.0f * ui, drawAy,
                                        (2.6f + 1.4f * focus) * ui,
                                        visualR, visualG, visualB,
                                        (itemPurchased
                                             ? (0.34f + 0.52f * focus + 0.22f * hover)
                                             : (0.18f + 0.38f * focus + 0.14f * hover))
                                            * designA * itemFade,
                                        false);
                const float labelContrast = isCoreNode
                    ? (0.66f + 0.34f * focus)
                    : (0.54f + 0.46f * focus);
                const float labelR = visualR + (whiteR - visualR) * labelContrast;
                const float labelG = visualG + (whiteG - visualG) * labelContrast;
                const float labelB = visualB + (whiteB - visualB) * labelContrast;
                const float itemLabelScale = UiTextScale(
                    g_TextS, UiTextLevel::Description, ui)
                    + 0.10f * focus * ui;
                drawFit(item.label, drawAx + miniRadius + 25.0f * ui,
                        drawAy - 12.0f * ui,
                        itemLabelScale,
                        labelW, labelR, labelG, labelB,
                        (0.64f + 0.38f * focus + 0.18f * hover) * itemFade);
                const float typeR = visualR + (whiteR - visualR) * 0.34f;
                const float typeG = visualG + (whiteG - visualG) * 0.34f;
                const float typeB = visualB + (whiteB - visualB) * 0.34f;
                DrawShadowedText(g_TextS,
                                 isCoreNode ? L"코어"
                                 : (isProfileNode ? L"프로필" : L"페이로드"),
                                 drawAx + miniRadius + 25.0f * ui,
                                 drawAy + 22.0f * ui,
                                 UiTextScale(g_TextS, UiTextLevel::Supporting,
                                             ui * (1.0f + 0.12f * focus)),
                                  typeR, typeG, typeB,
                                  (0.48f + 0.38f * focus + 0.20f * hover)
                                      * designA * itemFade,
                                  0.58f);
                if (newSelected) {
                    selectedNodeX = drawAx;
                    selectedNodeY = drawAy;
                }
                if (newVisible) {
                    const int chainSlot = (int)(newRel + 0.5f);
                    if (chainSlot >= 0 && chainSlot < 5) {
                        chainX[chainSlot] = drawAx;
                        chainY[chainSlot] = drawAy;
                        chainR[chainSlot] = visualR;
                        chainG[chainSlot] = visualG;
                        chainB[chainSlot] = visualB;
                        chainA[chainSlot] = itemFade;
                        chainValid[chainSlot] = true;
                    }
                }
            }
            if (clickSlot >= 0 && clickSlot != selectedSlot) {
                int deltaSlot = clickSlot - selectedSlot;
                // Follow the shortest direction around the circular list.
                // Without wrapping this sign, clicking an item near either
                // end can animate through the wrong side of the catalogue.
                if (deltaSlot > itemTotal / 2) deltaSlot -= itemTotal;
                if (deltaSlot < -itemTotal / 2) deltaSlot += itemTotal;
                s_listTransitionDir = (deltaSlot >= 0) ? 1.0f : -1.0f;
                s_listFromKey = s_selKey;
                s_listTransitionT = 0.0f;
                s_selKey = s_items[itemIndices[clickSlot]].key;
                s_detailT = 0.0f;
            }
            for (int chainSlot = 1; chainSlot < 5; ++chainSlot) {
                if (!chainValid[chainSlot - 1] || !chainValid[chainSlot]) continue;
                DrawVisibleConstellLine(chainX[chainSlot - 1], chainY[chainSlot - 1],
                                        chainX[chainSlot], chainY[chainSlot],
                                        0.72f * ui,
                                        chainR[chainSlot], chainG[chainSlot], chainB[chainSlot],
                                        0.16f * designA * chainA[chainSlot]);
            }
        }

        // Fixed right-center record block. Keep the field borderless; the
        // CircleTexture supplies contrast while typography carries the read.
        // The detail glow is a fixed UI light, independent of the selected
        // item colour, and is anchored around the action button.
        const float detailFieldRCol = cyanR;
        const float detailFieldGCol = cyanG;
        const float detailFieldBCol = cyanB;
        const float detailFieldCX = detailLayout.actionX
                                  + detailLayout.actionW * 0.50f;
        const float detailFieldCY = detailLayout.actionY;
        const float detailFieldR = std::max(300.0f * ui, detailW * 0.98f);
        // Give the lower-right readout the same chromatic depth as the
        // constellation field. Keep the light behind the dark contrast layer
        // so the copy remains readable over bright gameplay backgrounds.
        DrawConstellationDisc(detailFieldCX, detailFieldCY, detailFieldR * 0.92f,
                              detailFieldRCol, detailFieldGCol, detailFieldBCol,
                              0.040f * staticFieldA);
        DrawConstellationDisc(detailFieldCX, detailFieldCY, detailFieldR * 0.78f,
                              detailFieldRCol, detailFieldGCol, detailFieldBCol,
                              0.032f * staticFieldA);
        DrawConstellationDisc(detailFieldCX, detailFieldCY, detailFieldR * 0.44f,
                              detailFieldRCol, detailFieldGCol, detailFieldBCol,
                              0.022f * staticFieldA);
        DrawConstellationDisc(detailFieldCX, detailFieldCY, detailFieldR,
                              0.0f, 0.0f, 0.012f, 0.14f * staticFieldA,
                              false);

        // The detail readout intentionally stays typographic. Its orbital
        // lines and marker nodes compete with the information, especially on
        // the bright background, so the right edge is now the only anchor.
        drawRightFit(g_TextS, detailType, detailRight,
                     detailLayout.titleY - 38.0f * ui,
                     UiTextScale(g_TextS, UiTextLevel::Supporting, ui),
                     detailW, detailR, detailG, detailB,
                     0.96f * detailA, 0.72f);
        float titleSc = 1.08f * ui;
        while (titleSc > 0.74f * ui && g_TextL.Width(detailTitle, titleSc) > detailW)
            titleSc -= 0.04f * ui;
        drawRightFit(g_TextL, detailTitle, detailRight, detailLayout.titleY,
                     titleSc, detailW, 0.96f, 0.98f, 1.00f,
                     0.99f * detailA, 0.92f);

        const float copyY = detailLayout.copyY;
        if (detailIsMeta) {
            drawRightFit(g_TextS, L"출격 전 적용",
                         detailRight, copyY, detailDescriptionSc, detailW,
                         0.82f, 0.90f, 0.98f, 0.90f * detailA, 0.88f);
        } else if (detailIsTheme) {
            drawRightFit(g_TextS, L"아웃게임 색상 적용",
                         detailRight, copyY, detailDescriptionSc, detailW,
                         detailR, detailG, detailB, 0.88f * detailA, 0.86f);
        } else if (detailIsAug && detailId >= 0 && detailId < AUG_TOTAL) {
            const wchar_t* desc = AugDesc(ALL_AUGS[detailId]);
            if (desc && desc[0]) {
                const std::vector<std::wstring> segments = TarotWrap(
                    desc, detailDescriptionSc, detailW);
                int line = 0;
                for (const std::wstring& segment : segments) {
                    if (line >= 3) break;
                    drawRightFit(g_TextS, segment.c_str(), detailRight,
                                 copyY + line * 32.0f * ui,
                                 detailDescriptionSc, detailW,
                                 0.76f, 0.84f, 0.94f, 0.82f, 0.86f);
                    ++line;
                }
            }
        }

        const float tradeY = detailLayout.tradeY;
        const float actionX = detailLayout.actionX;
        const float actionW = detailLayout.actionW;
        const float actionY = detailLayout.actionY;
        // PLAY's readout uses a much larger information scale. Keep the
        // transaction/status copy at that same visual weight instead of
        // leaving level, balance, and purchase-result lines in tiny helper
        // text.
        const float detailInfoSc = purchaseInfoSc;
        auto drawWalletSummary = [&](float y, long long cost,
                                     bool canAfford, bool owned,
                                     bool finished) {
            auto drawStatusMarker = [&](const wchar_t* label, float markerY) {
                const float labelW = g_TextS.Width(label, detailInfoSc);
                DrawVisibleConstellNode(detailRight - labelW - 15.0f * ui,
                                        markerY + 6.0f * ui, 2.3f * ui,
                                        detailFieldRCol, detailFieldGCol,
                                        detailFieldBCol,
                                        0.58f * detailA, false);
            };
            wchar_t balanceBuf[64];
            swprintf_s(balanceBuf, L"보유 별가루  %lld", g_Coins);
            drawRightFit(g_TextS, balanceBuf, detailRight, y + 12.0f * ui,
                         detailInfoSc, detailW, whiteR, whiteG, whiteB,
                         0.78f * detailA, 0.60f);

            if (finished) {
                drawStatusMarker(L"최대 레벨", y - 30.0f * ui);
                drawRightFit(g_TextS, L"최대 레벨", detailRight,
                             y - 30.0f * ui, detailInfoSc, detailW,
                             detailR, detailG, detailB,
                             0.92f * detailA, 0.64f);
                return;
            }
            if (owned) {
                drawStatusMarker(L"구매 완료", y - 30.0f * ui);
                drawRightFit(g_TextS, L"구매 완료",
                             detailRight, y - 30.0f * ui, detailInfoSc,
                             detailW, detailR, detailG, detailB,
                             0.92f * detailA, 0.64f);
                return;
            }

            wchar_t costBuf[64];
            swprintf_s(costBuf, L"비용  %lld 별가루", cost);
            drawRightFit(g_TextS, costBuf, detailRight, y - 30.0f * ui,
                         detailInfoSc, detailW, goldR, goldG, goldB,
                         0.98f * detailA, 0.64f);

            if (canAfford) {
                wchar_t verdictBuf[64];
                swprintf_s(verdictBuf, L"구매 후  %lld 별가루",
                           g_Coins - cost);
                drawRightFit(g_TextS, verdictBuf, detailRight,
                             y + 52.0f * ui, detailInfoSc, detailW,
                             detailR, detailG, detailB,
                             0.86f * detailA, 0.62f);
            }
        };
        if (detailIsMeta && detailId >= 0 && detailId < META_COUNT) {
            const MetaDef& md = META_DEFS[detailId];
            const int curLv = g_MetaLv[detailId];
            const long long cost = MetaNextCost(detailId);
            wchar_t levelBuf[48]; swprintf_s(levelBuf, L"레벨  %d / %d", curLv, md.maxLv);
            drawRightFit(g_TextS, levelBuf, detailRight, tradeY - 60.0f * ui,
                         detailInfoSc, detailW, whiteR, whiteG, whiteB,
                         0.82f * detailA, 0.60f);
            const bool canBuy = cost >= 0 && g_Coins >= cost && curLv < md.maxLv;
            drawWalletSummary(tradeY, cost, canBuy, false,
                              curLv >= md.maxLv || cost < 0);
            const bool actionHover = inputReady && mx >= actionX
                                   && mx <= actionX + actionW
                                   && my >= actionY - 52.0f * ui
                                   && my <= actionY + 52.0f * ui;
            if (actionHover && lmbClick && canBuy) {
                g_Coins -= cost; ++g_MetaLv[detailId]; SaveGame();
            }
            wchar_t actionBuf[64];
            if (curLv >= md.maxLv) swprintf_s(actionBuf, L"최대 레벨");
            else if (!canBuy) swprintf_s(actionBuf, L"별가루 부족");
            else if (curLv <= 0) swprintf_s(actionBuf, L"구매");
            else swprintf_s(actionBuf, L"모듈 강화");
            drawFixedAction(actionX, actionY, actionW, actionBuf,
                            detailR, detailG, detailB,
                            canBuy || curLv >= md.maxLv, actionHover,
                            curLv >= md.maxLv ? SHOP_ACTION_ACTIVE
                                : (canBuy ? SHOP_ACTION_READY
                                          : SHOP_ACTION_BLOCKED));
        } else if (detailIsTheme && detailId >= 0 && detailId < ACCENT_COUNT) {
            const AccentTheme& theme = ACCENT_THEMES[detailId];
            const bool owned = ThemeOwned(detailId);
            const long long cost = theme.cost;
            const bool canBuy = !owned && g_Coins >= cost;
            // Shop owns purchase only. Loadout/equip remains a PLAY action.
            drawWalletSummary(tradeY, cost, canBuy, owned, false);
            const bool actionHover = inputReady && mx >= actionX
                                   && mx <= actionX + actionW
                                   && my >= actionY - 52.0f * ui
                                   && my <= actionY + 52.0f * ui;
            if (actionHover && lmbClick && canBuy) {
                g_Coins -= cost;
                g_ThemeOwned |= (1 << detailId);
                SaveGame();
            }
            drawFixedAction(actionX, actionY, actionW,
                            owned ? L"구매 완료"
                                  : (canBuy ? L"구매" : L"별가루 부족"),
                            detailR, detailG, detailB,
                            canBuy, actionHover,
                            owned ? SHOP_ACTION_ACTIVE
                                  : (canBuy ? SHOP_ACTION_READY
                                            : SHOP_ACTION_BLOCKED));
        } else if (detailIsAug) {
            drawRightFit(g_TextS, L"플레이 중 획득 효과 · 도감 전용",
                         detailRight, tradeY - 20.0f * ui,
                         detailDescriptionSc, detailW, whiteR, whiteG, whiteB,
                         0.82f * detailA, 0.60f);
        }

        DrawShadowedText(g_TextS, L"ESC / RMB  뒤로가기",
                         sw - std::max(74.0f, sw * 0.075f) -
                         g_TextS.Width(L"ESC / RMB  뒤로가기", 0.31f * ui),
                         sh - 48.0f * ui, 0.31f * ui,
                         0.52f, 0.68f, 0.82f, 0.60f * designA, 0.58f);

        // All CircleTexture/icon work for the SHOP frame is complete here.
        // Release the final text pass only now so no late glow can cover a
        // tab, list label, price, or action label.
        EndDeferredSceneText();

        if (finishBackAfterRender) {
            s_backExit = false;
            s_backOutT = 0.0f;
            s_ShopBackRequested = false;
            s_MainMenuArmoryPanel = false;
            s_MainMenuResumeFromPanel = true;
            g_MainMenuEntryT = 1.0f;
            g_GameManager.currentState = GameState::MAIN_MENU;
        }
        return;
    }

}
