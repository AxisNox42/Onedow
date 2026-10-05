// Included by SceneMenus.cpp to keep shared menu state and helpers private.

void Scene_Codex(const SceneCtx& c) {
    if (s_MainMenuCodexPanel) {
        Scene_CodexInline(c);
        return;
    }

    const float sw = c.sw, sh = c.sh;
    const double mx = c.mx, my = c.my;
    const bool lmb = c.lmb;
    const float delta = c.delta;
    GLFWwindow* window = c.window;
    (void)c.fireTimer; (void)c.reset;

    // ── State ─────────────────────────────────────────────────────────────
    static int   s_cat         = 0;
    static int   s_prevCat     = -1;
    static float s_catHover[2] = {};
    static float s_catFocus[2] = {};
    static float s_panelT      = 1.0f;
    static bool  s_catLoaded[2]= {};
    static int   s_sel[2]      = { -1, -1 };
    static float s_scroll[2]   = {};
    static float s_detailFadeT = 0.0f;
    static int   s_prevSel     = -2;

    DrawMenuBackground(sw, sh, delta, false);

    float dt = delta; if (dt > 0.05f) dt = 0.05f;
    g_CodexEntryT += dt;
    if (g_CodexEntryT > 1.0f) g_CodexEntryT = 1.0f;
    const float now = (float)glfwGetTime();

    const float wake      = Smoothstep(std::min(1.0f, g_CodexEntryT / 0.50f));
    const float rightWake = Smoothstep(std::min(1.0f,
        std::max(0.0f, g_CodexEntryT - 0.10f) / 0.44f));
    SetSceneTextureReveal(Smoothstep(LogoClamp01(g_CodexEntryT / 0.72f)));

    int li = LangIndex(); if (li < 0 || li >= 3) li = 0;
    const int nli = (li == 0) ? 0 : 1;

    // ── Layout ────────────────────────────────────────────────────────────
    const float TARGET_W = 1760.0f, TARGET_H = 930.0f;
    float uiS = UiScale(sw, sh, 0.975f, 0.920f, 0.72f, 1.30f);
    const float panelW  = TARGET_W * uiS;
    const float panelH  = TARGET_H * uiS;
    const float panelX  = (sw - panelW) * 0.5f;
    const float panelY  = (sh - panelH) * 0.5f;
    const float headerH = 82.0f * uiS;
    const float footerH = 74.0f * uiS;
    const float bodyY   = panelY + headerH;
    const float bodyH   = panelH - headerH - footerH;
    const float footY   = panelY + panelH - footerH + 12.0f * uiS;
    const float leftW   = panelW * 0.260f;
    const float colGap  = 14.0f * uiS;
    const float leftX   = panelX;
    const float rightX  = leftX + leftW + colGap;
    const float rightW  = (panelX + panelW) - rightX - 8.0f * uiS;
    const float tabH    = 54.0f * uiS;
    const float listAreaY = bodyY + 64.0f * uiS;
    const float listAreaH = bodyH - 64.0f * uiS;
    DrawPersistentSceneLeftVignette(sw, sh, 0.64f);
    DrawSceneRadialVignette(rightX + rightW * 0.44f, bodyY + bodyH * 0.50f,
                            std::min(rightW, bodyH) * 0.60f, 0.44f * rightWake);

    // ── Category definitions ──────────────────────────────────────────────
    struct CatDef { const wchar_t* id; const wchar_t* name[3]; float r, g, b; };
    static const CatDef kCats[] = {
        { L"ENTITIES", { L"관측체", L"ENTITIES", L"観測対象" }, 0.48f, 0.82f, 1.00f },
        { L"MODULES",  { L"모듈", L"MODULES", L"モジュール" }, 0.62f, 0.52f, 1.00f },
    };
    const int kCatCount = 2;
    const CatDef& cat = kCats[s_cat];
    const float uiR = cat.r, uiG = cat.g, uiB = cat.b;
    const float inkR = 0.88f, inkG = 0.94f, inkB = 1.00f;
    const float idleR = 0.50f, idleG = 0.62f, idleB = 0.76f;

    auto hit = [&](float x, float y, float w, float h) -> bool {
        return mx >= x && mx <= x + w && my >= y && my <= y + h;
    };

    // ── Transitions ───────────────────────────────────────────────────────
    if (s_prevCat != s_cat) {
        s_prevCat = s_cat;
        s_panelT = s_catLoaded[s_cat] ? 1.0f : 0.0f;
    }
    s_panelT = UiApproach(s_panelT, 1.0f, delta, 10.0f);
    if (s_panelT > 0.995f)
        s_catLoaded[s_cat] = true;

    int curSel = s_sel[s_cat];
    if (curSel != s_prevSel) { s_prevSel = curSel; s_detailFadeT = 0.0f; }
    s_detailFadeT = UiApproach(s_detailFadeT, 1.0f, delta, 8.0f);

    // ── Helper lambdas ────────────────────────────────────────────────────
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
    auto drawWrappedS = [&](const wchar_t* text, float x, float y,
                            float maxW, float maxH, float sc,
                            float r, float g, float b, float a) {
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
            g_TextS.Height(L"Ag", sc) + 4.0f * uiS);
        for (size_t i = 0; i < lines.size() && (i + 1) * lineH <= maxH; ++i)
            g_TextS.Draw(lines[i].c_str(), x, y + (float)i * lineH,
                         sc, r, g, b, a);
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
    auto drawAstrolabeGrid = [&](float x, float y, float w, float h,
                                 float r, float g, float b, float a) {
        if (a <= 0.001f) return;
        const float cx = x + w * 0.50f;
        const float cy = y + h * 0.50f;
        const float rad = std::min(w, h) * 0.31f;
        const float tLine = std::max(0.70f, 0.82f * uiS);
        auto ep = [&](float ang, float rx, float ry, float rot, float& ox, float& oy) {
            float ca = cosf(ang), sa = sinf(ang);
            float cr = cosf(rot), sr2 = sinf(rot);
            float lx = ca * rx;
            float ly = sa * ry;
            ox = cx + lx * cr - ly * sr2;
            oy = cy + lx * sr2 + ly * cr;
        };

        DrawSceneRadialVignette(cx, cy, rad * 1.95f, 0.34f * a);
        BindMainShader();
        for (int axis = 0; axis < 8; ++axis) {
            float a0 = now * 0.035f + (float)axis * 0.7853982f;
            DrawVisibleConstellLine(cx, cy,
                                    cx + cosf(a0) * rad * 0.82f,
                                    cy + sinf(a0) * rad * 0.82f,
                                    tLine, r, g, b, 0.050f * a);
        }

        for (int ring = 0; ring < 4; ++ring) {
            const float rx = rad * (0.66f + 0.13f * (float)ring);
            const float ry = rad * ((ring == 0) ? 0.66f : (0.23f + 0.10f * (float)ring));
            const float rot = now * (0.07f + 0.018f * (float)ring) + (float)ring * 0.72f;
            const int steps = 72;
            for (int s = 0; s < steps; ++s) {
                if (((s + ring) % 5) == 2) continue;
                float p0 = (float)s / (float)steps;
                float p1 = ((float)s + 0.56f) / (float)steps;
                float x0, y0, x1, y1;
                ep(p0 * 6.2831853f, rx, ry, rot, x0, y0);
                ep(p1 * 6.2831853f, rx, ry, rot, x1, y1);
                DrawVisibleConstellLine(x0, y0, x1, y1, tLine, r, g, b,
                                        (ring == 0 ? 0.155f : 0.105f) * a);
            }
        }

        for (int n = 0; n < 12; ++n) {
            float ang = now * 0.11f + (float)n * 0.5235988f;
            float nr = rad * (0.48f + 0.42f * ((n % 3) / 2.0f));
            DrawVisibleConstellNode(cx + cosf(ang) * nr,
                                    cy + sinf(ang) * nr,
                                    ((n % 4) == 0 ? 3.7f : 2.5f) * uiS,
                                    r, g, b, ((n % 4) == 0 ? 0.30f : 0.20f) * a);
        }
        DrawVisibleConstellNode(cx, cy, 7.0f * uiS, r, g, b, 0.36f * a);
    };
    auto drawInfoQuad = [&](float x, float y, float w, float h, float a) {
        if (a <= 0.001f) return;
        BindMainShader();
        drawRect(x + 5.0f*uiS, y + 6.0f*uiS, w, h, 0.0f, 0.0f, 0.0f, 0.12f * a);
        drawRect(x, y, w, h, 0.038f, 0.054f, 0.078f, 0.38f * a);
        drawRect(x, y, w, h, uiR, uiG, uiB, 0.052f * a);
        drawCBkt(x, y, w, h, uiR, uiG, uiB, 0.26f * a);
    };

    // ── HEADER ────────────────────────────────────────────────────────────
    const float hSlide = (1.0f - wake) * 30.0f * uiS;
    const wchar_t* archiveTitle = (li == 0) ? L"도감"
        : (li == 1) ? L"ASTRAL LOG" : L"星界記録";
    BindMainShader();
    g_TextL.Draw(archiveTitle,
                 leftX, panelY + 4.0f * uiS - hSlide,
                 UiTextScale(g_TextL, UiTextLevel::Title, uiS),
                 1.0f, 1.0f, 1.0f, 0.98f * wake);

    drawRect(leftX, panelY + headerH - 2.0f * uiS, panelW, 1.5f * uiS,
             uiR, uiG, uiB, 0.30f * wake);

    // Category tabs: same quiet strip language as RUN CONFIG / ARMORY.
    const float tabGp = 0.0f;
    const float tabW  = (leftW - tabGp * (float)(kCatCount - 1)) / (float)kCatCount;
    const float tabsX = leftX;
    const float tabsY = bodyY;

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
        if (sel || hov) {
            drawRect(tx + 8.0f * uiS, ty + tabH * 0.52f, tabW - 16.0f * uiS,
                     1.1f * uiS, cd.r, cd.g, cd.b,
                     (sel ? 0.070f : 0.035f + s_catHover[i] * 0.030f) * wake);
        }
        if (sel) {
            drawRect(tx, ty + tabH - 2.0f * uiS, tabW, 2.0f * uiS,
                     cd.r, cd.g, cd.b, 0.56f * wake);
            drawDiamond(tx + 15.0f * uiS, ty + tabH * 0.5f,
                        3.0f * uiS, cd.r, cd.g, cd.b, 0.72f * wake);
        }
        const wchar_t* tabLabel = kCats[i].name[li];
        const float idSc = UiTextScale(g_TextS, UiTextLevel::Title, uiS);
        const float idY  = ty + (tabH - g_TextS.Height(tabLabel, idSc)) * 0.5f;
        drawCenterS(tabLabel, tx, idY, tabW, idSc,
                    sel ? inkR : idleR,
                    sel ? inkG : idleG,
                    sel ? inkB : idleB,
                    (sel ? 0.92f : 0.48f + s_catHover[i] * 0.20f) * wake);
    }

    // ── LEFT PANEL ────────────────────────────────────────────────────────
    float panelE = Smoothstep(s_panelT);

    BindMainShader();
    {
        const float masterX = leftX - 8.0f * uiS;
        const float masterY = bodyY - 6.0f * uiS;
        const float masterW = (rightX + rightW) - masterX;
        const float masterH = bodyH + 10.0f * uiS;
        drawRect(masterX + 12.0f*uiS, masterY + 13.0f*uiS, masterW, masterH,
                 0.0f, 0.0f, 0.0f, 0.035f * wake);
        drawConstellFrame(masterX, masterY, masterW, masterH,
                          uiR, uiG, uiB, 0.070f * panelE * wake,
                          24.0f * uiS, 2.4f * uiS, 0.010f * wake, 0.34f);
    }
    drawRect(leftX + 8.0f*uiS, listAreaY + 10.0f*uiS, leftW, listAreaH,
             0.0f, 0.0f, 0.0f, 0.030f * wake);
    drawRect(leftX, listAreaY, leftW, listAreaH,
             0.030f, 0.042f, 0.062f, 0.055f * wake);
    drawRect(leftX + 8.0f*uiS, listAreaY + 8.0f*uiS,
             leftW - 16.0f*uiS, listAreaH - 16.0f*uiS,
             0.052f, 0.072f, 0.098f, 0.050f * wake);
    drawRect(leftX + 2.0f * uiS, listAreaY, 1.2f * uiS, listAreaH,
             uiR, uiG, uiB, 0.14f * wake);
    if (wake > 0.02f) {
        BatchFlush(); SetGlowFx(true);
        drawConstellFrame(leftX, listAreaY, leftW, listAreaH,
                          uiR, uiG, uiB, 0.070f * panelE * wake,
                          18.0f*uiS, 2.4f*uiS, 0.010f * wake, 0.42f);
        BatchFlush(); SetGlowFx(false);
    }
    // List layout constants
    const float listX     = leftX + 6.0f * uiS;
    const float listW2    = leftW  - 12.0f * uiS;
    const float listTopY  = listAreaY + 8.0f * uiS;
    const float listBotY  = listAreaY + listAreaH - 8.0f * uiS;
    const float listViewH = listBotY - listTopY;
    const float itemH     = 58.0f * uiS;
    const float grpH      = 54.0f * uiS;

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
            { L"ACTIVE SIGNALS",
              { CM_ROTOR, CM_SCOPE, CM_SWARM, CM_GENESIS, CM_GRAVIS, CM_QUASAR, 0, 0, 0 },
              6, 0.48f, 0.82f, 1.00f },
        };
        for (int gi = 0; gi < 1; ++gi) {
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
            const float gy = ry + 3.0f * uiS;
            const float gh = grpH - 6.0f * uiS;
            const float mid = gy + gh * 0.5f;
            drawRect(listX + 9.0f*uiS, mid - 0.5f*uiS,
                     listW2 - 28.0f*uiS, 1.0f*uiS,
                     uiR, uiG, uiB, 0.12f * wake);
            drawRect(listX + 4.0f*uiS, gy + 7.0f*uiS, 1.6f*uiS, gh - 14.0f*uiS,
                     litem.r, litem.g, litem.b, 0.48f * wake);
            drawDiamond(listX + 5.0f*uiS, mid, 3.4f*uiS,
                        litem.r, litem.g, litem.b, 0.62f * wake);
            const float groupSc = UiTextScale(g_TextS, UiTextLevel::Subtitle, uiS);
            g_TextS.Draw(litem.label, listX + 22.0f*uiS,
                         gy + (gh - g_TextS.Height(litem.label, groupSc)) * 0.5f,
                         groupSc, 0.82f, 0.92f, 1.0f, 0.90f * wake);
        } else {
            bool isSel = (s_sel[s_cat] == litem.dataIdx);
            const float cardY = ry + 4.0f * uiS;
            const float cardH = itemH - 8.0f * uiS;
            bool hov2  = overList && hit(listX, cardY, listW2, cardH);
            if (hov2 && lmb && !g_LmbPrev && litem.seen)
                s_sel[s_cat] = litem.dataIdx;

            float ia = wake * (0.65f + 0.35f * panelE);
            BindMainShader();
            const float focus = isSel ? 1.0f : (hov2 ? 0.55f : 0.0f);
            if (isSel || hov2) {
                drawRect(listX + 22.0f * uiS, cardY + cardH * 0.5f,
                         listW2 - 30.0f * uiS, 1.1f * uiS,
                         uiR, uiG, uiB, (isSel ? 0.115f : 0.055f) * ia);
            }
            if (isSel) {
                drawRect(listX + 4.0f * uiS, cardY + 7.0f * uiS,
                         2.0f * uiS, cardH - 14.0f * uiS,
                         uiR, uiG, uiB, 0.74f * ia);
                g_TextS.Draw(L">", listX + 12.0f * uiS,
                             cardY + (cardH - g_TextS.Height(L">", 0.60f * uiS)) * 0.5f,
                             0.60f * uiS, uiR, uiG, uiB, 0.96f * ia);
            } else if (hov2) {
                drawRect(listX + 5.0f * uiS, cardY + 10.0f * uiS,
                         1.4f * uiS, cardH - 20.0f * uiS,
                         uiR, uiG, uiB, 0.34f * ia);
            }

            const float ndy = cardY + cardH * 0.5f;
            const float ndx = listX + 6.0f*uiS;
            drawDiamond(ndx, ndy, (isSel ? 4.4f : 3.0f)*uiS,
                        litem.r, litem.g, litem.b,
                        (isSel ? 0.92f : hov2 ? 0.62f : 0.32f) * ia);

            float itemSc = UiTextScale(g_TextS, UiTextLevel::Title, uiS);
            while (itemSc > UiTextScale(g_TextS, UiTextLevel::Supporting, uiS)
                   && g_TextS.Width(litem.label, itemSc) > listW2 - 54.0f * uiS)
                itemSc -= 0.025f * uiS;
            const float textY = cardY + (cardH - g_TextS.Height(litem.label, itemSc)) * 0.5f;
            g_TextS.Draw(litem.label, listX + (isSel ? 34.0f : 22.0f)*uiS, textY, itemSc,
                         litem.seen ? (isSel ? 1.0f : hov2 ? 0.94f : 0.68f) : 0.46f,
                         litem.seen ? (isSel ? 1.0f : hov2 ? 0.97f : 0.76f) : 0.48f,
                         litem.seen ? (isSel ? 1.0f : hov2 ? 1.00f : 0.86f) : 0.54f,
                         (litem.seen ? (isSel ? 0.98f : hov2 ? 0.86f : 0.64f) : 0.52f) * ia);
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
        drawRect(tkX, thumbY, 3.5f*uiS, thumbH, uiR, uiG, uiB, 0.80f * wake);
    }

    // ── RIGHT PANEL ───────────────────────────────────────────────────────
    float detailA    = Smoothstep(s_detailFadeT) * rightWake;
    float entryShift = (1.0f - rightWake) * 70.0f * uiS;
    float rpx        = rightX + entryShift;

    BindMainShader();
    drawRect(rpx + 9.0f*uiS, bodyY + 11.0f*uiS, rightW, bodyH,
             0.0f, 0.0f, 0.0f, (0.045f + 0.035f*panelE)*rightWake);
    drawRect(rpx, bodyY, rightW, bodyH,
             0.026f, 0.038f, 0.058f, (0.26f + 0.08f*panelE)*rightWake);
    drawRect(rpx + 14.0f*uiS, bodyY + 14.0f*uiS,
             rightW - 28.0f*uiS, bodyH - 28.0f*uiS,
             0.050f, 0.070f, 0.096f, 0.070f * rightWake);
    float sweepW = rightW * 0.28f;
    float sweepX = rpx + fmodf(now * 180.0f, rightW + sweepW) - sweepW;
    drawRect(sweepX, bodyY + 1.0f*uiS, sweepW, 1.5f*uiS,
             uiR, uiG, uiB, 0.12f*panelE*rightWake);
    float scanY2 = bodyY + 18.0f*uiS + fmodf(now * 110.0f,
                   std::max(1.0f, bodyH - 30.0f*uiS));
    drawRect(rpx + 2.0f*uiS, scanY2, rightW - 4.0f*uiS, 1.0f*uiS,
             uiR, uiG, uiB, 0.040f*panelE*rightWake);
    if (rightWake > 0.02f) {
        BatchFlush(); SetGlowFx(true);
        drawConstellFrame(rpx, bodyY, rightW, bodyH,
                          uiR, uiG, uiB, 0.22f*panelE*rightWake,
                          24.0f*uiS, 5.5f*uiS, 0.040f * rightWake, 0.40f);
        BatchFlush(); SetGlowFx(false);
    }
    // Content area
    int selItem = s_sel[s_cat];
    const float cntX = rpx + 22.0f*uiS;
    const float cntY = bodyY + 34.0f*uiS;
    const float cntW = rightW - 44.0f*uiS;

    if (selItem < 0) {
        drawAstrolabeGrid(rpx, bodyY, rightW, bodyH, uiR, uiG, uiB,
                          panelE * rightWake);
    } else {
        // Layout: left viz panel (38%) | right info block (62%)
        const float cntH   = bodyH - 70.0f*uiS;
        const float vizW   = std::min(cntW * 0.46f, 560.0f * uiS);
        const float vizH   = cntH;
        const float vizX   = cntX;
        const float vizCX  = vizX + vizW * 0.5f;
        const float vizCY  = cntY + vizH * 0.5f;
        const float infoX  = cntX + vizW + 24.0f*uiS;
        const float infoW  = cntW - vizW - 24.0f*uiS;

        // Viz panel background
        BindMainShader();
        drawRect(vizX, cntY, vizW, vizH,
                 0.014f, 0.022f, 0.036f,
                 0.18f * detailA);
        drawRect(vizX + 8.0f*uiS, cntY + 8.0f*uiS, vizW - 16.0f*uiS, vizH - 16.0f*uiS,
                 uiR, uiG, uiB, 0.018f * detailA);
        drawCBkt(vizX, cntY, vizW, vizH, uiR, uiG, uiB, 0.40f * detailA);
        float vzScan = cntY + fmodf(now * 55.0f, std::max(1.0f, vizH));
        drawRect(vizX + 2.0f*uiS, vzScan, vizW - 4.0f*uiS, 1.0f*uiS,
                 uiR, uiG, uiB, 0.038f * detailA);

        if (s_cat == 0) {
            // Entity previews use the exact gameplay silhouettes (ROTOR,
            // GENESIS, SCOPE, SWARM and GRAVIS).
            const float previewScale = std::max(2.4f, std::min(vizW, vizH) / 72.0f);
            drawCodexMobPreview(selItem, vizCX, vizCY, previewScale);
        } else {
        // Rotating constellation node viewer
        {
            int nNodes = (s_cat == 0) ? 5 : (s_cat == 1) ? 6 : 4;
            float orbR  = std::min(vizW, vizH) * 0.36f;
            float spd   = (s_cat == 0) ? 0.28f : (s_cat == 1) ? 0.18f : 0.14f;
            float baseA = now * spd + (float)(selItem % 13) * 0.61f;
            float innerR = orbR * 0.42f;
            float baseA2 = -now * spd * 0.55f + (float)(selItem % 7) * 1.13f;

            float px[8], py[8];
            for (int ni = 0; ni < nNodes; ++ni) {
                float ang = baseA + (float)ni * (6.2832f / (float)nNodes);
                px[ni] = vizCX + cosf(ang) * orbR;
                py[ni] = vizCY + sinf(ang) * orbR;
            }
            // Outer ring edges (constellation dashes)
            BindMainShader();
            for (int ni = 0; ni < nNodes; ++ni) {
                int nj = (ni + 1) % nNodes;
                float dx2 = px[nj]-px[ni], dy2 = py[nj]-py[ni];
                float len2 = sqrtf(dx2*dx2+dy2*dy2);
                int nd = std::max(2,(int)(len2/(6.0f*uiS)));
                for (int d=1; d<nd; ++d) {
                    float t = (float)d/(float)nd;
                    drawDiamond(px[ni]+dx2*t, py[ni]+dy2*t, 1.1f*uiS,
                                uiR, uiG, uiB, 0.44f*detailA);
                }
            }
            // Center spokes (every other node)
            for (int ni = 0; ni < nNodes; ni += 2) {
                float dx2 = px[ni]-vizCX, dy2 = py[ni]-vizCY;
                float len2 = sqrtf(dx2*dx2+dy2*dy2);
                int nd = std::max(2,(int)(len2/(7.0f*uiS)));
                for (int d=1; d<nd; ++d) {
                    float t = (float)d/(float)nd;
                    drawDiamond(vizCX+dx2*t, vizCY+dy2*t, 0.9f*uiS,
                                uiR, uiG, uiB, 0.26f*detailA);
                }
            }
            // Inner counter-rotating ring
            int nInner = (s_cat == 1) ? 3 : 2;
            for (int ni = 0; ni < nInner; ++ni) {
                float ang = baseA2 + (float)ni*(6.2832f/(float)nInner);
                float ipx = vizCX + cosf(ang)*innerR;
                float ipy = vizCY + sinf(ang)*innerR;
                float dx2 = ipx-vizCX, dy2 = ipy-vizCY;
                float len2 = sqrtf(dx2*dx2+dy2*dy2);
                int nd = std::max(2,(int)(len2/(6.0f*uiS)));
                for (int d=1; d<nd; ++d) {
                    float t = (float)d/(float)nd;
                    drawDiamond(vizCX+dx2*t, vizCY+dy2*t, 1.0f*uiS,
                                uiR, uiG, uiB, 0.20f*detailA);
                }
                BindMainShader();
                drawDiamond(ipx, ipy, 2.5f*uiS, uiR, uiG, uiB, 0.55f*detailA);
            }
            // Outer node diamonds
            BindMainShader();
            for (int ni = 0; ni < nNodes; ++ni) {
                float nsz = (ni == 0) ? 5.0f : 3.5f;
                drawDiamond(px[ni], py[ni], nsz*uiS, uiR, uiG, uiB,
                            (ni==0 ? 0.90f : 0.68f)*detailA);
            }
            // Center node
            drawDiamond(vizCX, vizCY, 4.0f*uiS, uiR, uiG, uiB, 0.80f*detailA);
            drawDiamond(vizCX, vizCY, 2.0f*uiS, 1.0f, 1.0f, 1.0f, 0.42f*detailA);
        }
        }

        const float infoGap   = 18.0f * uiS;
        const float titleBoxY = cntY;
        const float titleBoxH = 104.0f * uiS;
        const float descBoxY  = titleBoxY + titleBoxH + infoGap;
        const float descBoxH  = 150.0f * uiS;
        const float logBoxY   = descBoxY + descBoxH + infoGap;
        const float logBoxH   = std::max(132.0f * uiS, cntY + cntH - logBoxY);

        if (s_cat == 0) {
            // ── Entity detail ─────────────────────────────────────────────
            bool seen = CodexMobSeen(selItem);
            BindMainShader();
            drawInfoQuad(infoX, titleBoxY, infoW, titleBoxH, detailA);
            g_TextS.Draw(L"ENTITY", infoX + 18.0f*uiS, titleBoxY + 10.0f*uiS,
                         UiTextScale(g_TextS, UiTextLevel::Supporting, uiS),
                         uiR, uiG, uiB, 0.78f*detailA);
            g_TextL.Draw(MobName(selItem), infoX + 18.0f*uiS, titleBoxY + 34.0f*uiS,
                         UiTextScale(g_TextL, UiTextLevel::Title, uiS),
                         1.0f, 1.0f, 1.0f, 0.96f*detailA);
            drawRect(infoX + 18.0f*uiS, titleBoxY + titleBoxH - 16.0f*uiS, infoW - 36.0f*uiS, 1.5f*uiS,
                     uiR, uiG, uiB, 0.28f*detailA);
            drawInfoQuad(infoX, descBoxY, infoW, descBoxH, detailA);
            drawWrappedS(seen ? MobDesc(selItem) : L"???",
                         infoX + 18.0f*uiS, descBoxY + 24.0f*uiS,
                         infoW - 36.0f*uiS, descBoxH - 42.0f*uiS,
                         UiTextScale(g_TextS, UiTextLevel::Description, uiS),
                         0.86f, 0.92f, 1.0f, 0.92f*detailA);
            // ── 시스템 로그 블록 ──
            if (seen) {
                drawInfoQuad(infoX, logBoxY, infoW, logBoxH, detailA);
                const wchar_t* threatLv = MobThreatLabel(selItem);
                wchar_t pidBuf[16]; swprintf_s(pidBuf, L"0x%02X", (selItem * 17 + 0x40) & 0xFF);
                struct { const wchar_t* k; const wchar_t* v; } logR[] = {
                    { L"THREAT_LV  ", threatLv },
                    { L"PATTERN_ID ", pidBuf   },
                    { L"STATUS     ", L"CATALOGUED" },
                };
                for (int ll = 0; ll < 3; ++ll) {
                    float ly = logBoxY + 28.0f*uiS + ll * 38.0f * uiS;
                    const float logScale = UiTextScale(g_TextS, UiTextLevel::Supporting, uiS);
                    g_TextS.Draw(logR[ll].k, infoX + 18.0f*uiS,              ly, logScale, uiR, uiG, uiB, 0.72f*detailA);
                    g_TextS.Draw(L": ",       infoX + 150.0f*uiS,            ly, logScale, uiR, uiG, uiB, 0.54f*detailA);
                    g_TextS.Draw(logR[ll].v,  infoX + 172.0f*uiS,            ly, logScale, 1.0f, 1.0f, 1.0f, 0.88f*detailA);
                }
            }

        } else if (s_cat == 1) {
            // ── Module detail ─────────────────────────────────────────────
            bool seen = (selItem >= 0 && selItem < AUG_TOTAL && CodexAugSeen(selItem));
            if (seen) {
                const AugDef& d = ALL_AUGS[selItem];
                float rr, rg, rb; GetRarityColor(d.rarity, rr, rg, rb);
                BindMainShader();
                drawInfoQuad(infoX, titleBoxY, infoW, titleBoxH, detailA);
                g_TextS.Draw(L"MODULE", infoX + 18.0f*uiS, titleBoxY + 10.0f*uiS,
                             UiTextScale(g_TextS, UiTextLevel::Supporting, uiS),
                             uiR, uiG, uiB, 0.78f*detailA);
                g_TextL.Draw(AugName(d), infoX + 18.0f*uiS, titleBoxY + 34.0f*uiS,
                             UiTextScale(g_TextL, UiTextLevel::Title, uiS),
                             1.0f, 1.0f, 1.0f, 0.96f*detailA);
                drawRect(infoX + 18.0f*uiS, titleBoxY + titleBoxH - 16.0f*uiS, infoW - 36.0f*uiS, 1.5f*uiS,
                         uiR, uiG, uiB, 0.28f*detailA);
                drawInfoQuad(infoX, descBoxY, infoW, descBoxH, detailA);
                drawWrappedS(AugDesc(d), infoX + 18.0f*uiS,
                             descBoxY + 24.0f*uiS, infoW - 36.0f*uiS,
                             descBoxH - 42.0f*uiS,
                             UiTextScale(g_TextS, UiTextLevel::Description, uiS),
                             0.86f, 0.92f, 1.0f, 0.92f*detailA);
                // ── 모듈 데이터 블록 ──
                {
                    drawInfoQuad(infoX, logBoxY, infoW, logBoxH, detailA);
                    wchar_t tidBuf[24]; swprintf_s(tidBuf, L"0x%03X", (int)d.type & 0xFFF);
                    struct { const wchar_t* k; const wchar_t* v; } mlogR[] = {
                        { L"MODULE_CLASS", GetRarityKR(d.rarity) },
                        { L"TYPE_ID     ", tidBuf },
                        { L"ACQUISITION ", L"IN-RUN PICK" },
                    };
                    for (int ml = 0; ml < 3; ++ml) {
                        float mly = logBoxY + 28.0f*uiS + ml * 38.0f*uiS;
                        const float logScale = UiTextScale(g_TextS, UiTextLevel::Supporting, uiS);
                        g_TextS.Draw(mlogR[ml].k, infoX + 18.0f*uiS,  mly, logScale, uiR, uiG, uiB, 0.72f*detailA);
                        g_TextS.Draw(L": ", infoX + 164.0f*uiS,       mly, logScale, uiR, uiG, uiB, 0.54f*detailA);
                        g_TextS.Draw(mlogR[ml].v, infoX + 188.0f*uiS, mly, logScale, 1.0f, 1.0f, 1.0f, 0.88f*detailA);
                    }
                }
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
                        drawFitS(rc, infoX + 18.0f*uiS, logBoxY + 150.0f*uiS, infoW - 36.0f*uiS,
                                 0.48f*uiS, 0.34f*uiS,
                                 0.82f, 0.94f, 1.0f, 0.90f*detailA);
                        break;
                    }
                }
            } else {
                BindMainShader();
                const wchar_t* q[3] = { L"수집까지 비공개",
                                         L"Undiscovered — unlock by acquiring",
                                         L"入手すると記録が解放されます" };
                drawInfoQuad(infoX, titleBoxY, infoW, titleBoxH, detailA);
                g_TextS.Draw(q[li], infoX + 18.0f*uiS, titleBoxY + 32.0f*uiS,
                             UiTextScale(g_TextS, UiTextLevel::Description, uiS),
                             0.62f, 0.66f, 0.76f, 0.88f*detailA);
            }
        }
    }

    // ── FOOTER: BACK button ───────────────────────────────────────────────
    float bw = 160.0f*uiS, bh = 40.0f*uiS;
    float bx = panelX, by = footY;
    bool backHov = hit(bx, by, bw, bh);
    BindMainShader();
    drawRect(bx, by, bw, bh,
             0.020f + uiR*(backHov ? 0.045f : 0.018f),
             0.030f + uiG*(backHov ? 0.035f : 0.014f),
             0.046f + uiB*(backHov ? 0.030f : 0.012f), 0.94f * wake);
    drawCBkt(bx, by, bw, bh, uiR, uiG, uiB, (backHov ? 0.68f : 0.28f) * wake);
    const float backTextScale = UiTextScale(g_TextS, UiTextLevel::Title, uiS);
    drawCenterS(T(StrId::BTN_BACK), bx,
                by + (bh - g_TextS.Height(T(StrId::BTN_BACK), backTextScale)) * 0.5f,
                bw, backTextScale,
                backHov ? 1.0f : 0.78f, backHov ? 1.0f : 0.84f, backHov ? 1.0f : 0.96f,
                0.96f * wake);
    if (backHov && lmb && !g_LmbPrev) {
        CodexSearchClear();
        g_CodexSearchInputEnabled = false;
        g_GameManager.currentState = GameState::MAIN_MENU;
    }
}
