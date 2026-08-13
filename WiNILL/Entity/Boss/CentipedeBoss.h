#pragma once
#include <vector>
#include <deque>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#include "Bullet.h"
#include "DrawPrim.h"   // 蹂댁뒪 ?먯껜 ?뚮뜑(?꾩쟾 遺꾨━) ??drawRect/drawNeonBorder/drawDiamond ??
#include "PlayerStats.h"
// ?????????????????????????????????????????????????????????????
// FORK.worm ???뚮씪利덈쭏 ?ы겕 泥댁씤 (泥댁씤??蹂댁뒪)
//   ?ㅼ삩 ?몃뱶 + ?먮꼫吏 耳?대툝???댁뼱吏?湲?蹂댁뒪. OS/媛吏쒖갹 UI 鍮꾩＜???ъ슜 ????
//   ?ㅻ（?? ?쒖븞쨌留덉젨? ?몃뱶 泥댁씤 + 癒몃━ 3-prong ?ы겕 湲濡쒖슦.
// ?????????????????????????????????????????????????????????????
class CentipedeBoss {
public:
    static constexpr const wchar_t* BOSS_NAME = L"FORK.worm";
    float worldX, worldY;          // 癒몃━ ?꾩튂
    float hp, maxHp;
    bool  alive = true;
    bool  exploded = false;
    int   screenW, screenH;
    // ?꾨줈?좏??? ?쒖닔 蹂댁뒪?????〓す ?≪닔 HP + ?쇳빐 媛먯냼(?몃━?똌G/誘몃땲嫄?DPS 濡?利됱궡 諛⑹?)
    float dmgTakenMult = 0.65f;    // 諛쏅뒗 ?쇳빐 횞0.65 (35% 媛먯냼)

    // ?곹깭: 0=諛고쉶 / 1=?붾㈃諛??댄깉 / 2=寃쎈줈?덇퀬 / 3=怨≪꽑 ?ъ쭊??    //       4=踰??쒕룞 / 5=?щ━ / 6=?λ꼍 / 7=?좊났
    int   state      = 0;
    float stateTimer = 0.0f;
    float wanderTimer = 0.0f;
    float heading    = 0.0f;       // 吏꾪뻾 諛⑺뼢(rad)

    // ?뚯쭊 怨≪꽑 寃쎈줈 (2李?踰좎???
    float dashFromX = 0, dashFromY = 0, dashToX = 0, dashToY = 0;
    float dashCtrlX = 0, dashCtrlY = 0;
    float dashT = 0.0f;

    bool  wasInside  = true;
    bool  shakePulse = false;
    int   dashCount  = 0;

    std::deque<glm::vec2> trail;   // 癒몃━ 沅ㅼ쟻(?멸렇癒쇳듃 異붿쥌)

    static constexpr int   NSEG       = 18;      // 瑗щ━ ?멸렇癒쇳듃 ???꾩껌 湲멸쾶)
    static constexpr int   SEG_STEP   = 6;       // ?멸렇癒쇳듃 媛?沅ㅼ쟻 ?몃뜳??媛꾧꺽(?묒쓣?섎줉 珥섏킌)
    static constexpr float HEAD       = 62.0f;   // 癒몃━ 異⑸룎 諛섍꼍 (?묎퀬 ?좎뭅濡?쾶)
    static constexpr float SEG_NEAR   = 36.0f;   // 癒몃━??媛??媛源뚯슫 ?멸렇癒쇳듃(諛섑겕湲?
    static constexpr float SEG_FAR    = 15.0f;   // 瑗щ━ ??媛???묒쓬)
    static constexpr float WANDER_SPD = 440.0f;  // 湲곕낯 諛고쉶 ?띾룄 (?뺤떊 ?щ궔寃?鍮좊Ⅴ寃?
    static constexpr float WANDER_GAIN= 0.12f;   // ?뚯쭊 1?뚮떦 +12%
    static constexpr float WANDER_CAP = 2.6f;    // ?띾룄 諛곗쑉 ?곹븳
    static constexpr float DASH_SPD   = 1700.0f; // ?뚯쭊/?댄깉 ?띾룄
    static constexpr float WANDER_T   = 7.5f;    // 諛고쉶 ?쒓컙(????? ??怨듦꺽??
    static constexpr float TELEGRAPH  = 1.2f;    // 怨≪꽑 ?뚯쭊 ?덇퀬
    static constexpr float TURN_INT   = 0.38f;   // 吏洹몄옱洹??꾪솚 二쇨린(????쾶 = ?곕쭔)

    // ?곗씠???좎궗(遺梨꾧섦)
    static constexpr float SPIT_INT   = 5.5f;
    static constexpr int   SPIT_N     = 5;
    static constexpr float SPIT_SPD   = 300.0f;
    float spitTimer = 0.0f;
    // ?뚮젅?댁뼱 吏곸꽑 ?뚯쭊
    static constexpr float CHARGE_INT    = 9.0f;
    static constexpr float CHARGE_WINDUP = 0.5f;
    static constexpr float CHARGE_DUR    = 0.65f;
    static constexpr float CHARGE_SPD    = 1500.0f;
    int   chargePhase = 0;       // 0=諛고쉶 / 1=議곗? / 2=?뚯쭊
    float chargeCdTimer = 0.0f;
    float chargeTimer = 0.0f;
    float chargeDX = 0.0f, chargeDY = 0.0f;
    bool  chargeTelegraph = false;
    // ?곗씠????＜(?섏꽑)
    static constexpr float SURGE_INT  = 8.5f;
    static constexpr float SURGE_DUR  = 1.4f;
    static constexpr float SURGE_TICK = 0.09f;
    static constexpr float SURGE_SPD  = 280.0f;
    float surgeCd = 0.0f, surgeT = 0.0f, surgeTick = 0.0f, surgeAng = 0.0f;
    bool  surging = false;
    // ?멸렇癒쇳듃 ?ш꺽 ??癒몃━?믨섕由щ줈 留덈뵒媛 李⑤?李⑤? ?뚮젅?댁뼱?먭쾶 諛쒖궗(?붾㈃ ?꾩껜 ?섎졃)
    static constexpr float CANNON_INT  = 9.5f;
    static constexpr float CANNON_STEP = 0.045f;  // 留덈뵒 媛?諛쒖궗 媛꾧꺽
    static constexpr float CANNON_SPD  = 360.0f;
    bool  cannonActive = false;
    float cannonCd = 0.0f, cannonT = 0.0f;
    int   cannonIdx = 0;
    // ?? ?먯떇 ?꾨줈?몄뒪 ???묒? 泥댁씤 adds(?먯껜 愿由? ??
    struct MiniBug { float x, y, heading, hp; bool alive; std::deque<glm::vec2> trail;
                     float wt; int lungeState; float lungeT; };
    std::vector<MiniBug> minis;
    static constexpr float SUMMON_INT  = 9.0f;
    static constexpr int   SUMMON_COUNT = 2;   // ??踰덉뿉 2留덈━
    static constexpr int   MINI_NSEG   = 5;
    static constexpr int   MINI_STEP   = 4;
    static constexpr float MINI_HEAD   = 26.0f;
    static constexpr float MINI_SPD    = 230.0f;
    static constexpr float MINI_HP0    = 288.0f; // = 湲곕낯 而ㅻ꼸 ?꾨줈?몄뒪(BRUTE 90*3.2) 泥대젰
    static constexpr float MINI_WIN_W  = 300.0f;
    static constexpr float MINI_WIN_H  = 210.0f;
    static constexpr float MINI_WIN_TB = 14.0f;
    static constexpr wchar_t MINI_WIN_NAME[] = L"child.exe";
    float summonCd = 0.0f;

    // ?? VFX (Gemini 媛?대뱶 ??OS UI ?놁씠 ?쒖닔 ?댄럺?? ??
    float prevWorldX = 0.0f, prevWorldY = 0.0f;
    struct SegFlash { int seg; float t; };
    std::vector<SegFlash> segFlashes;
    struct GhostEcho { float x, y, r, life, maxLife; };
    std::vector<GhostEcho> ghosts;
    static constexpr int   MAX_GHOST = 96;
    struct HitSpark { float x, y, vx, vy, life; };
    std::vector<HitSpark> hitSparks;
    struct ErrorNode { float x, y, fuse; bool alive; };
    std::vector<ErrorNode> errorNodes;
    float tailDropCd = 0.5f;
    bool  moltGlitchPulse = false;
    float glitchOverlay   = 0.0f;

    bool lockOnActive() const {
        return chargeTelegraph || chargePhase == 1 || state == 2;
    }

    void onSegHit(int seg, float bx, float by) {
        for (auto& f : segFlashes)
            if (f.seg == seg && f.t > 0.02f) return;
        SegFlash sf; sf.seg = seg; sf.t = 0.05f;
        segFlashes.push_back(sf);
        for (int i = 0; i < 2; i++) {
            float a = (float)(rand() % 628) * 0.01f;
            HitSpark sp;
            sp.x = bx; sp.y = by;
            sp.vx = cosf(a) * (140.0f + (float)(rand() % 60));
            sp.vy = sinf(a) * (140.0f + (float)(rand() % 60));
            sp.life = 0.18f;
            hitSparks.push_back(sp);
        }
    }

    void spawnGhost(float x, float y, float r) {
        if ((int)ghosts.size() >= MAX_GHOST) return;
        GhostEcho g;
        g.x = x; g.y = y; g.r = r; g.maxLife = g.life = 0.5f;
        ghosts.push_back(g);
    }

    void tickVfx(float dt) {
        for (auto& f : segFlashes) f.t -= dt;
        segFlashes.erase(std::remove_if(segFlashes.begin(), segFlashes.end(),
            [](const SegFlash& f){ return f.t <= 0.0f; }), segFlashes.end());

        for (auto& g : ghosts) g.life -= dt;
        ghosts.erase(std::remove_if(ghosts.begin(), ghosts.end(),
            [](const GhostEcho& g){ return g.life <= 0.0f; }), ghosts.end());

        for (auto& sp : hitSparks) {
            sp.life -= dt;
            sp.x += sp.vx * dt; sp.y += sp.vy * dt;
            sp.vx *= 0.90f; sp.vy *= 0.90f;
        }
        hitSparks.erase(std::remove_if(hitSparks.begin(), hitSparks.end(),
            [](const HitSpark& sp){ return sp.life <= 0.0f; }), hitSparks.end());

        if (glitchOverlay > 0.0f) glitchOverlay -= dt;
    }

    void drawKillMark(float cx, float cy, float r, float tm, bool glitchy) const {
        float flick = glitchy
            ? (sinf(tm * 44.0f) > 0.0f ? 1.0f : 0.35f)
            : (0.82f + 0.18f * sinf(tm * 9.0f));
        float jx = glitchy ? sinf(tm * 61.0f) * r * 0.08f : 0.0f;
        float jy = glitchy ? cosf(tm * 53.0f) * r * 0.08f : 0.0f;
        cx += jx; cy += jy;
        drawCircle(cx, cy, r * 1.15f, 0.12f, 0.04f, 0.06f, 0.88f);
        drawCircle(cx, cy, r * 0.92f, 0.22f, 0.06f, 0.08f, 0.75f);
        auto xArm = [&](float ang) {
            float ca = cosf(ang), sa = sinf(ang);
            for (int k = 0; k < 9; k++) {
                float u = ((float)k / 8.0f - 0.5f) * 1.85f;
                drawCircle(cx + ca * r * u * 0.48f, cy + sa * r * u * 0.48f,
                           r * 0.11f, 1.0f, 1.0f, 1.0f, flick);
            }
        };
        xArm(0.785398f);
        xArm(-0.785398f);
        if (glitchy) {
            drawCircle(cx + r * 0.12f, cy - r * 0.08f, r * 0.08f,
                       1.0f, 0.25f, 0.25f, flick * 0.7f);
        }
    }

    bool segFlashing(int seg) const {
        for (auto& f : segFlashes)
            if (f.seg == seg && f.t > 0.0f) return true;
        return false;
    }

    void drawErrorNode(float cx, float cy, float fuseLeft, float tm) const {
        float pulse = 0.6f + 0.4f * sinf(tm * 16.0f);
        float sz = 22.0f + pulse * 3.0f;
        float warn = fuseLeft < 0.6f ? (0.5f + 0.5f * sinf(tm * 24.0f)) : 0.35f;
        drawRect(cx - sz, cy - sz * 0.75f, sz * 2.0f, sz * 1.5f, 0.14f, 0.06f, 0.10f, 0.92f);
        drawRect(cx - sz, cy - sz * 0.75f, sz * 2.0f, sz * 0.28f,
                 0.85f + warn * 0.15f, 0.18f, 0.22f, 1.0f);
        drawKillMark(cx, cy, sz * 0.55f, tm, fuseLeft < 0.5f);
    }

    void spawnMini(float ex, float ey, float tx, float ty) {
        MiniBug mb; mb.x = ex; mb.y = ey;
        mb.heading = atan2f(ty - ey, tx - ex); mb.hp = MINI_HP0; mb.alive = true;
        mb.wt = (float)(rand()%628)*0.01f; mb.lungeState = 0; mb.lungeT = 0.0f;
        mb.trail.assign(MINI_NSEG*MINI_STEP + 2, glm::vec2(ex, ey));
        minis.push_back(mb);
    }

    // ?? ?뚮씪利덈쭏 ?몃뱶 / 耳?대툝 (怨듭슜 ?쒕줈?? ??
    void drawPlasmaNode(float cx, float cy, float r, float br, float tm, int idx,
                        bool forkGlow, float forkAng) const {
        float ph = tm * 5.5f + (float)idx * 0.65f;
        float pulse = 0.72f + 0.28f * sinf(ph);
        bool alt = (idx & 1) != 0;
        float cr = alt ? 0.95f : 0.30f;
        float cg = alt ? 0.35f : 0.88f;
        float cb = alt ? 0.75f : 1.00f;

        drawCircle(cx, cy, r * 1.45f, cr * 0.35f, cg * 0.35f, cb * 0.35f, 0.14f * br);
        drawCircle(cx, cy, r * 1.12f, cr * 0.25f, cg * 0.25f, cb * 0.25f, 0.28f * br);
        drawDiamond(cx, cy, r * 1.18f, cr * br, cg * br, cb * br, 0.82f);
        drawCircle(cx, cy, r * 0.62f, 0.12f, 0.08f, 0.16f, 0.92f);
        drawCircle(cx, cy, r * 0.42f * pulse, cr * pulse, cg * pulse, cb * pulse, 1.0f);
        drawCircle(cx, cy, r * 0.16f, 1.0f, 0.96f, 1.0f, 1.0f);

        if (forkGlow) {
            for (int pr = 0; pr < 3; pr++) {
                float ang = forkAng + (float)pr * 2.094395f;
                float len = r * 1.55f;
                for (int k = 1; k <= 5; k++) {
                    float u = (float)k / 5.0f;
                    drawCircle(cx + cosf(ang) * len * u, cy + sinf(ang) * len * u,
                               4.5f - u * 2.0f, cr, cg, cb, 0.55f * br * (1.0f - u * 0.35f));
                }
            }
        }
    }

    void drawPlasmaLink(glm::vec2 a, glm::vec2 b, float thick, float tm, int idx) const {
        float dx = b.x - a.x, dy = b.y - a.y;
        float len = std::sqrt(dx * dx + dy * dy) + 1e-3f;
        int nd = (int)(len / 11.0f);
        if (nd < 3) nd = 3;
        for (int k = 0; k <= nd; k++) {
            float u = (float)k / (float)nd;
            float wob = sinf(tm * 9.0f + u * 12.0f + (float)idx) * 2.5f;
            float pxn = -dy / len, pyn = dx / len;
            float px = a.x + dx * u + pxn * wob;
            float py = a.y + dy * u + pyn * wob;
            float t01 = 0.35f + 0.65f * (1.0f - u);
            drawCircle(px, py, thick * t01, 0.55f, 0.18f, 0.92f, 0.42f);
            if (k % 2 == 0)
                drawCircle(px, py, thick * 0.35f, 0.95f, 0.45f, 0.85f, 0.75f);
        }
    }

    // ?먯떇 adds ??異뺤냼 ?뚮씪利덈쭏 泥댁씤 (媛吏쒖갹 ?놁쓬)
    void drawMini(const MiniBug& mb) const {
        float tm = (float)mb.x * 0.003f;
        glm::vec2 prev = glm::vec2(mb.x, mb.y);
        for (int i = MINI_NSEG; i >= 1; i--) {
            int idx = i * MINI_STEP;
            if (idx >= (int)mb.trail.size()) idx = (int)mb.trail.size() - 1;
            if (idx < 0) idx = 0;
            glm::vec2 s = mb.trail[idx];
            float br = 0.55f + 0.45f * (1.0f - (float)(i - 1) / (float)MINI_NSEG);
            float r = MINI_HEAD * (0.55f - 0.06f * (float)i);
            drawPlasmaLink(prev, s, 4.5f, tm, i + 100);
            drawPlasmaNode(s.x, s.y, r, br, tm, i, (i % 3) == 0, mb.heading);
            prev = s;
        }
        drawKillMark(mb.x, mb.y, MINI_HEAD * 0.62f, tm, mb.lungeState == 1);
    }
    // ?ㅽ궗 ?쒖쟾 吏꾨룞(媛踰쇱슫 ?쇰뱶諛? ?덈퐬 X) ??main ???쎄퀬 ?곸슜
    float wantShake = 0.0f;
    void shake(float m) { if (m > wantShake) wantShake = m; }
    // ?? ?щ옒??踰???HP 源롮씪 ?뚮쭏???붾㈃ 媛濡쒖?瑜대뒗 吏곸꽑 李⑤떒(??쒕줈留??듦낵) ??
    //   ?쒓컙?쇰줈 ???щ씪吏? ? ?⑥쐞濡?珥?留욎쑝硫?洹?遺遺꾨쭔 ?ル┝(?대룞 ?듬줈 ?뺣낫).
    struct WallCell { /* per-cell hp; 0=?ル┝ */ };
    struct LineWall {
        bool  horiz;                 // true=媛濡?怨좎젙 y) / false=?몃줈(怨좎젙 x)
        float coord;                 // 怨좎젙 醫뚰몴(媛濡쒕㈃ y, ?몃줈硫?x)
        float spawnT;                // ?깆옣 ?좊땲硫붿씠???붿뿬(?ъ껜媛 轅덊??섎떎 援녹쓬)
        std::vector<int> cellHp;     // ?蹂?HP (0 = ?ル┝)
    };
    std::vector<LineWall> walls;
    static constexpr float WALL_CELL  = 60.0f;   // ? ?ш린(px)
    static constexpr float WALL_THICK = 30.0f;   // 李⑤떒 ?먭퍡(吏곸꽑 ??
    static constexpr int   WALL_CELLHP = 10;     // ? ?ル뒗 ???꾩슂???쇨꺽 ?????踰꾪봽)
    static constexpr int   WALL_MAX   = 3;        // ?숈떆 ?곹븳(?꾩껜 吏곸꽑?대씪 ?곴쾶)
    static constexpr float WALL_STEP  = 0.08f;
    static constexpr float WALL_ANIM  = 0.7f;
    float lastWallHp = -1.0f;

    // ?? ????⑦꽩 濡쒗뀒?댁뀡(?쒕룞/?щ━/?λ꼍/?좊났) ??
    static constexpr float BIG_INT = 6.0f;       // ????⑦꽩 荑⑤떎???띾룄 鍮꾨? 媛먯냼)
    float bigCd = 0.0f;
    // 踰?諛뺢린 愿묐? ??怨좎냽?쇰줈 踰??ъ씠瑜??뺢린硫??띾룄 鍮꾨? ?꾩쓣 肉쒓퀬, 諛뺤쓣?섎줉 媛먯냽
    static constexpr float RAMP_SPD0      = 1750.0f; // 珥덇린 ?띾룄(??컻??
    static constexpr float RAMP_MIN       = 470.0f;  // ???띾룄 諛묒씠硫?醫낅즺
    static constexpr float RAMP_DECAY     = 0.80f;   // 踰?異⑸룎留덈떎 ?띾룄 횞0.80
    static constexpr float RAMP_FIRE_SPD  = 340.0f;  // 遺꾩텧 ???띾룄
    static constexpr float RAMP_FIRE_K    = 135.0f;  // 遺꾩텧 媛꾧꺽 = K/?띾룄 (鍮좊??섎줉 ?먯＜)
    static constexpr int   RAMP_SHRAP     = 10;      // 踰?異⑸룎 諛⑹궗 ?뚰렪
    static constexpr float RAMP_SHRAP_SPD = 360.0f;
    float rampSpeed = 0.0f, rampFireTimer = 0.0f;
    float rampDX = 0.0f, rampDY = 0.0f;
    // ?щ━ 媛먭린(A1) ???쒓컙?대룞 X(?꾩옱 ?꾩튂?먯꽌 議곗뿬??, ?ㅻ옒 媛먯쓬
    static constexpr float COIL_DUR    = 4.6f;   // ?꾨뒗 ?쒓컙 ???덉뿉 ?덉쑝硫??꾪뿕)
    static constexpr float COIL_SPIN   = 4.4f;   // rad/s (??留롮씠 媛먭?)
    static constexpr float COIL_R0     = 300.0f; // 紐⑺몴 ?쒖옉 諛섍꼍(?묎렐 ??
    static constexpr float COIL_RMIN   = 115.0f;
    static constexpr float COIL_SHRINK = 42.0f;
    float coilCX = 0, coilCY = 0, coilR = 0, coilAng = 0;
    // ?λ꼍 遺꾪븷(A3)
    static constexpr float WALL_SPD  = 780.0f;
    static constexpr float WALL_FIRE = 0.16f;
    int   wallDir = 1;
    float wallFireT = 0.0f;
    // ?좊났(B4)
    static constexpr float BURROW_WARN      = 0.75f;
    static constexpr float BURROW_SINK      = 0.45f;   // ?쒖옄由?媛?쇱븠???쒓컙
    static constexpr int   BURROW_SHRAP     = 18;
    static constexpr float BURROW_SHRAP_SPD = 380.0f;
    int   burrowPhase = 0;
    float burrowX = 0, burrowY = 0;
    float burrowScale = 1.0f;    // 媛?쇱븠湲??잕뎄移??ㅼ???1=?뺤긽, 0=?꾩쟾 ?좊났)

    // ?? ?대룞 ?ъ텧 ???吏곸씠硫?醫뚯슦濡??곗씠???꾩쓣 ?섎┝(?곸떆 ?뺣컯) ??
    static constexpr float SHED_INT = 0.22f;     // 珥섏킌?섍쾶(?먯＜)
    static constexpr float SHED_SPD = 200.0f;
    float shedTimer = 0.0f;
    // ?? ?덊뵾 吏猶???
    struct Hazard { float x, y, life, maxLife, r; };
    std::vector<Hazard> hazards;
    // ?덊뵾/?ъ텧(A2)
    int   activeSeg = NSEG;      // ?꾩옱 ?댁븘?덈뒗 ?멸렇癒쇳듃 ???덊뵾濡?以꾩뼱??
    int   moltLevel = 0;
    static constexpr float MOLT_R         = 42.0f;
    static constexpr float MOLT_LIFE      = 9.0f;
    static constexpr int   MOLT_SHRAP     = 10;
    static constexpr float MOLT_SHRAP_SPD = 320.0f;

    CentipedeBoss(int sw, int sh, float hpInit) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.5f; worldY = sh * 0.4f;   // main ???ㅽ룿 ????뼱?
        heading = (float)(rand() % 628) * 0.01f;
        chargeCdTimer = CHARGE_INT * 0.7f;
        bigCd = BIG_INT * 0.5f;
        trail.assign(NSEG * SEG_STEP + 8, glm::vec2(worldX, worldY));
        prevWorldX = worldX;
        prevWorldY = worldY;
    }

    bool vulnerable() const { return state == 0 || state == 4 || state == 5 || state == 6; }

    float segSize(int i) const {
        int n = activeSeg;
        float t = (n > 1) ? (float)(i - 1) / (float)(n - 1) : 0.0f;
        return SEG_NEAR + (SEG_FAR - SEG_NEAR) * t;
    }
    float wanderSpeed() const {
        float mul = 1.0f + WANDER_GAIN * (float)dashCount + 0.12f * (float)moltLevel;
        if (mul > WANDER_CAP) mul = WANDER_CAP;
        return WANDER_SPD * mul;
    }
    // ?띾룄 鍮꾨? 荑⑤떎??諛곗쑉: 鍮좊??섎줉 < 1.0 (?ㅽ궗??????븘吏?
    float cdScale() const { return WANDER_SPD / wanderSpeed(); }
    bool onScreen(float x, float y) const {
        return x >= 0.0f && x <= (float)screenW && y >= 0.0f && y <= (float)screenH;
    }

    glm::vec2 bezier(float t) const {
        float u = 1.0f - t;
        float bx = u*u*dashFromX + 2.0f*u*t*dashCtrlX + t*t*dashToX;
        float by = u*u*dashFromY + 2.0f*u*t*dashCtrlY + t*t*dashToY;
        return glm::vec2(bx, by);
    }
    glm::vec2 bezierTangent(float t) const {
        float u = 1.0f - t;
        float tx = 2.0f*u*(dashCtrlX - dashFromX) + 2.0f*t*(dashToX - dashCtrlX);
        float ty = 2.0f*u*(dashCtrlY - dashFromY) + 2.0f*t*(dashToY - dashCtrlY);
        return glm::vec2(tx, ty);
    }
    void pickDash() {
        float cx = screenW * 0.5f, cy = screenH * 0.5f;
        float R  = std::sqrt(cx*cx + cy*cy) + 140.0f;
        float a0 = (float)(rand() % 628) * 0.01f;
        float a1 = a0 + 3.14159265f + ((float)(rand()%120 - 60)) * 0.01f;
        dashFromX = cx + cosf(a0) * R;  dashFromY = cy + sinf(a0) * R;
        dashToX   = cx + cosf(a1) * R;  dashToY   = cy + sinf(a1) * R;
        float mx = (dashFromX + dashToX) * 0.5f, my = (dashFromY + dashToY) * 0.5f;
        float dx = dashToX - dashFromX, dy = dashToY - dashFromY;
        float len = std::sqrt(dx*dx + dy*dy) + 1e-3f;
        float pxn = -dy / len, pyn = dx / len;
        float curve = (0.35f + (rand()%50)*0.006f) * len * ((rand()%2) ? 1.0f : -1.0f);
        dashCtrlX = mx + pxn * curve;  dashCtrlY = my + pyn * curve;
    }

    void fireFrom(std::vector<Bullet>& b, float ox, float oy,
                  float dx, float dy, float sp, glm::vec3 col) {
        Bullet bb(ox, oy, ox + dx * 100.0f, oy + dy * 100.0f);
        bb.isEnemy = true; bb.speed = sp; bb.color = col;
        b.push_back(bb);
    }
    void fireDir(std::vector<Bullet>& b, float dx, float dy, float sp, glm::vec3 col) {
        fireFrom(b, worldX, worldY, dx, dy, sp, col);
    }

    // 踰?諛뺢린 愿묐? ?쒖옉 ???꾩쓽 ?媛?諛⑺뼢?쇰줈 ??컻?곸쑝濡??뺢린湲??쒖옉
    void enterRamp() {
        state = 4; stateTimer = 0.0f; chargeTelegraph = false;
        float a = (float)(rand() % 628) * 0.01f;
        rampDX = cosf(a); rampDY = sinf(a);
        rampSpeed = RAMP_SPD0; rampFireTimer = 0.0f;
        heading = a;
    }
    void backToWander() {
        state = 0; stateTimer = 0.0f;
        spitTimer = 0.0f; surging = false; surgeCd = 0.0f;
        chargePhase = 0; chargeTelegraph = false; chargeCdTimer = CHARGE_INT * 0.5f;
    }

    // ?깆옣 紐⑥뀡 ??'?щ씪議뚮떎媛 ?뚯쭊' 湲곗닠 ?ы솢?? ?붾㈃ 諛?寃쎈줈 ?덇퀬 ??怨≪꽑 ?뚯쭊?쇰줈 ?낆옣
    void enterSpawn() {
        pickDash();
        state = 2; stateTimer = 0.0f;
        worldX = dashFromX; worldY = dashFromY;
        for (auto& p : trail) p = glm::vec2(worldX, worldY);
    }

    // 珥앹븣 ?좊텇??踰?吏곸꽑??媛濡쒖?瑜대㈃ ?대떦 ? HP 媛먯냼(洹?遺遺꾨쭔 ?ル┝).
    //   ?댁븘?덈뒗 ???留욎븯?쇰㈃ true 諛섑솚(?쇰컲?꾩? main ?먯꽌 ?뚮㈇?쒗궡 = 愿??????.
    bool hitWall(float x0, float y0, float x1, float y1) {
        bool hit = false;
        for (auto& w : walls) {
            if (w.horiz) {
                bool crossed = (y0 - w.coord) * (y1 - w.coord) <= 0.0f && fabsf(y1 - y0) > 1e-4f;
                float cx;
                if (crossed) { float tt = (w.coord - y0) / (y1 - y0); cx = x0 + (x1 - x0) * tt; }
                else if (fabsf((y0 + y1) * 0.5f - w.coord) < WALL_THICK * 0.5f) cx = (x0 + x1) * 0.5f; // ?됲뻾 洹쇱젒(醫뚯슦 愿??李⑤떒)
                else continue;
                int ci = (int)(cx / WALL_CELL);
                if (ci >= 0 && ci < (int)w.cellHp.size() && w.cellHp[ci] > 0) { w.cellHp[ci]--; hit = true; }
            } else {
                bool crossed = (x0 - w.coord) * (x1 - w.coord) <= 0.0f && fabsf(x1 - x0) > 1e-4f;
                float cy;
                if (crossed) { float tt = (w.coord - x0) / (x1 - x0); cy = y0 + (y1 - y0) * tt; }
                else if (fabsf((x0 + x1) * 0.5f - w.coord) < WALL_THICK * 0.5f) cy = (y0 + y1) * 0.5f;
                else continue;
                int ci = (int)(cy / WALL_CELL);
                if (ci >= 0 && ci < (int)w.cellHp.size() && w.cellHp[ci] > 0) { w.cellHp[ci]--; hit = true; }
            }
        }
        return hit;
    }
    // ?댁븘?덈뒗 踰?????뚮젅?댁뼱 ?대룞??留됱쓬(吏곸꽑 諛뽰쑝濡?諛?대깂). ???臾댁쟻)??main ?먯꽌 ?쒖쇅.
    void blockMove(float& pcx, float& pcy, float plr) const {
        float half = WALL_THICK * 0.5f + plr;
        for (const auto& w : walls) {
            if (w.horiz) {
                int ci = (int)(pcx / WALL_CELL);
                if (ci < 0 || ci >= (int)w.cellHp.size() || w.cellHp[ci] <= 0) continue;
                float dy = pcy - w.coord;
                if (fabsf(dy) < half) pcy = w.coord + (dy >= 0.0f ? half : -half);
            } else {
                int ci = (int)(pcy / WALL_CELL);
                if (ci < 0 || ci >= (int)w.cellHp.size() || w.cellHp[ci] <= 0) continue;
                float dx = pcx - w.coord;
                if (fabsf(dx) < half) pcx = w.coord + (dx >= 0.0f ? half : -half);
            }
        }
    }

    void Update(float px, float py, float dt, float& playerHP, std::vector<Bullet>& bullets) {
        if (!alive) return;
        const glm::vec3 BUL(0.95f, 0.35f, 0.95f);   // 蹂댁뒪 ???됱긽 ????媛吏(留덉젨?)濡??듭씪

        // ?? ?덊뵾(Molt) ??HP ?꾧퀎留덈떎 瑗щ━ 留덈뵒 ?ъ텧 ??吏猶?+ 媛????
        {
            static const float TH[4] = { 0.8f, 0.6f, 0.4f, 0.2f };
            if (moltLevel < 4 && hp <= maxHp * TH[moltLevel]) {
                ++moltLevel;
                glm::vec2 tail = segPos(activeSeg);
                for (int i = 0; i < 2; i++) {
                    float a = (float)(rand() % 628) * 0.01f;
                    Hazard h; h.x = tail.x + cosf(a) * 30.0f; h.y = tail.y + sinf(a) * 30.0f;
                    h.maxLife = h.life = MOLT_LIFE; h.r = MOLT_R;
                    hazards.push_back(h);
                }
                activeSeg -= 3; if (activeSeg < 6) activeSeg = 6;
                shakePulse = true;
                moltGlitchPulse = true;
                glitchOverlay = 0.12f;
                spawnMini(tail.x, tail.y, px, py);
            }
        }

        // ?? ?ㅽ궗 荑⑤떎???곹깭 臾닿? ?꾩쟻 ???좊ː??. ?몃━嫄곕뒗 ?ш린???쇨큵 ??
        cannonCd += dt;
        if (!cannonActive && cannonCd >= CANNON_INT * cdScale() &&
            state == 0 && chargePhase == 0 && !surging) {
            cannonCd = 0.0f; cannonActive = true; cannonIdx = 0; cannonT = 0.0f;
            shake(7.0f);
        }
        summonCd += dt;
        if (summonCd >= SUMMON_INT * cdScale() && state != 2) {   // ?곹븳 ?놁쓬 ???쇱젙 ?쒓컙留덈떎 怨꾩냽
            summonCd = 0.0f;
            for (int i = 0; i < SUMMON_COUNT; i++) {
                float ex, ey;                                     // ?붾㈃ ??媛?μ옄由??먯꽌留??앹꽦
                switch (rand() % 4) {
                case 0:  ex = (float)(rand()%screenW);   ey = -24.0f;                 break;
                case 1:  ex = (float)(rand()%screenW);   ey = (float)screenH + 24.0f; break;
                case 2:  ex = -24.0f;                    ey = (float)(rand()%screenH); break;
                default: ex = (float)screenW + 24.0f;    ey = (float)(rand()%screenH); break;
                }
                spawnMini(ex, ey, px, py);
            }
            shake(5.0f);
        }
        // ?? ?덈겮 踰꾧렇(?묒? 吏?ㅽ삎 蹂댁뒪) 媛깆떊 ??吏洹몄옱洹??꾨튃 + 二쇨린???곗?(?뚯쭊) ?⑦꽩 ??
        for (auto& mb : minis) {
            if (!mb.alive) continue;
            float dx = px - mb.x, dy = py - mb.y, d = std::sqrt(dx*dx + dy*dy) + 1e-3f;
            float toP = atan2f(dy, dx);
            mb.wt += dt; mb.lungeT += dt;
            float spd = MINI_SPD;
            if (mb.lungeState == 0) {                          // ?꾨튃 ?묎렐(吏洹몄옱洹?
                mb.heading = toP + sinf(mb.wt * 6.0f) * 0.6f;
                if (mb.lungeT >= 2.4f && d < 430.0f) { mb.lungeState = 1; mb.lungeT = 0.0f; mb.heading = toP; }
            } else {                                            // 吏㏃? ?곗?(?뚯쭊)
                spd = MINI_SPD * 2.7f;
                if (mb.lungeT >= 0.4f) { mb.lungeState = 0; mb.lungeT = 0.0f; }
            }
            mb.x += cosf(mb.heading) * spd * dt;
            mb.y += sinf(mb.heading) * spd * dt;
            mb.trail.push_front(glm::vec2(mb.x, mb.y));
            if ((int)mb.trail.size() > MINI_NSEG*MINI_STEP + 2) mb.trail.pop_back();
            if (d < MINI_HEAD + 14.0f) HurtPlayer(playerHP, 6.0f * dt);
        }
        // ?덈겮?쇰━ 寃뱀묠 諛⑹? ???뚰봽??肄쒕━???쒕줈 諛?대깂)
        for (size_t i = 0; i < minis.size(); i++) {
            if (!minis[i].alive) continue;
            for (size_t j = i + 1; j < minis.size(); j++) {
                if (!minis[j].alive) continue;
                float dx = minis[j].x - minis[i].x, dy = minis[j].y - minis[i].y;
                float d2 = dx*dx + dy*dy, minD = MINI_HEAD * 2.4f;
                if (d2 > 1e-4f && d2 < minD * minD) {
                    float d = std::sqrt(d2), push = (minD - d) * 0.5f, nx = dx/d, ny = dy/d;
                    minis[i].x -= nx*push; minis[i].y -= ny*push;
                    minis[j].x += nx*push; minis[j].y += ny*push;
                }
            }
        }
        minis.erase(std::remove_if(minis.begin(), minis.end(),
            [](const MiniBug& m){ return !m.alive; }), minis.end());

        // ?? 二쎌? 吏??踰?????源롮씤 ?꾩쟻?됰쭏???붾㈃ 媛濡쒖?瑜대뒗 吏곸꽑 李⑤떒踰??앹꽦 ??
        if (lastWallHp < 0.0f) lastWallHp = hp;             // 泥??꾨젅??湲곗?
        float step = maxHp * WALL_STEP;
        while (hp <= lastWallHp - step) {
            lastWallHp -= step;
            LineWall lw;
            lw.horiz  = (rand() % 2) == 0;
            lw.spawnT = WALL_ANIM;
            int span  = lw.horiz ? screenH : screenW;       // 怨좎젙醫뚰몴 異?踰붿쐞
            int along = lw.horiz ? screenW : screenH;       // 吏곸꽑??六쀫뒗 異?踰붿쐞
            lw.coord  = 90.0f + (float)(rand() % (span > 180 ? span - 180 : 1));
            int nc = along / (int)WALL_CELL + 1;
            lw.cellHp.assign(nc, WALL_CELLHP);
            // ?뚮젅?댁뼱 ?꾩튂 ?? 誘몃━ ?レ뼱 ??利됱떆 媛??諛⑹?)
            float pAlong = lw.horiz ? px : py;
            int pc = (int)(pAlong / WALL_CELL);
            for (int k = pc - 1; k <= pc + 1; k++)
                if (k >= 0 && k < nc) lw.cellHp[k] = 0;
            walls.push_back(lw);   // ?곹븳 ?놁쓬 ??怨꾩냽 ?꾩쟻(?쒓컙 ?뚮㈇ X, 珥앹쑝濡쒕쭔 ?ル┝)
            shake(8.0f);
        }
        for (auto& w : walls) if (w.spawnT > 0.0f) w.spawnT -= dt;          // ?깆옣 ?좊땲硫붿씠?섎쭔(?쒓컙?뚮㈇ X)

        // 諛고쉶 ??대㉧??議곗?쨌?뚯쭊 以?硫덉땄(?????
        if (state != 0 || chargePhase == 0)
            stateTimer += dt;

        if (state == 0) {            // ?? 諛고쉶 + ?쇱씠??寃ъ젣 ??
            if (chargePhase == 1) {  // 吏곸꽑 ?뚯쭊 議곗?(硫덉땄)
                chargeTimer += dt;
                float d = atan2f(py - worldY, px - worldX);
                heading = d;
                if (chargeTimer >= CHARGE_WINDUP) {
                    chargePhase = 2; chargeTimer = 0.0f;
                    chargeDX = cosf(d); chargeDY = sinf(d);
                    chargeTelegraph = false;
                    shake(9.0f);                 // ?뚯쭊 媛쒖떆 ??臾듭쭅??吏꾨룞
                }
            } else if (chargePhase == 2) { // 吏곸꽑 ?뚯쭊
                chargeTimer += dt;
                worldX += chargeDX * CHARGE_SPD * dt;
                worldY += chargeDY * CHARGE_SPD * dt;
                heading = atan2f(chargeDY, chargeDX);
                if (chargeTimer >= CHARGE_DUR) {
                    chargePhase = 0; chargeTimer = 0.0f; chargeCdTimer = 0.0f;
                }
            } else {                 // ?쇰컲 諛고쉶
                wanderTimer += dt;
                if (wanderTimer >= TURN_INT) {
                    wanderTimer = 0.0f;
                    heading += ((rand() % 2) ? 1.0f : -1.0f) * 0.7f;
                }
                if (surging) {       // ?곗씠????＜(?섏꽑)
                    surgeT += dt; surgeTick += dt; surgeAng += dt * 3.2f;
                    if (surgeTick >= SURGE_TICK) {
                        surgeTick = 0.0f;
                        for (int i = 0; i < 2; i++) {
                            float a = surgeAng + (float)i * 3.14159265f;
                            fireDir(bullets, cosf(a), sinf(a), SURGE_SPD, BUL);
                        }
                    }
                    if (surgeT >= SURGE_DUR) { surging = false; surgeCd = 0.0f; }
                } else {
                    spitTimer += dt;     // ?곗씠???좎궗(遺梨꾧섦)
                    if (spitTimer >= SPIT_INT * cdScale()) {
                        spitTimer = 0.0f;
                        float base = atan2f(py - worldY, px - worldX);
                        for (int i = 0; i < SPIT_N; i++) {
                            float a = base + ((float)i / (float)(SPIT_N - 1) - 0.5f) * 0.8f;
                            fireDir(bullets, cosf(a), sinf(a), SPIT_SPD, BUL);
                        }
                    }
                    surgeCd += dt;       // ??＜ 諛쒕룞
                    if (surgeCd >= SURGE_INT * cdScale() && chargePhase == 0) {
                        surging = true; surgeT = 0.0f; surgeTick = 0.0f;
                        surgeAng = (float)(rand() % 628) * 0.01f;
                        shake(6.0f);
                    }
                }
                // ?? ?멸렇癒쇳듃 ?ш꺽 吏꾪뻾(?몃━嫄곕뒗 ?곷떒) ??留덈뵒 癒몃━?믨섕由?李⑤? 諛쒖궗 ??
                if (cannonActive) {
                    cannonT += dt;
                    while (cannonT >= CANNON_STEP) {
                        cannonT -= CANNON_STEP;
                        glm::vec2 s = (cannonIdx == 0) ? glm::vec2(worldX, worldY) : segPos(cannonIdx);
                        float a = atan2f(py - s.y, px - s.x);
                        fireFrom(bullets, s.x, s.y, cosf(a), sinf(a), CANNON_SPD, BUL);
                        ++cannonIdx;
                        if (cannonIdx > activeSeg) { cannonActive = false; cannonT = 0.0f; break; }
                    }
                }
                // ?붾㈃ 寃쎄퀎?먯꽌 以묒븰?쇰줈 遺?쒕읇寃??좏쉶
                float toC = atan2f(screenH*0.5f - worldY, screenW*0.5f - worldX);
                float m = 120.0f;
                if (worldX < m || worldX > screenW - m || worldY < m || worldY > screenH - m) {
                    float d = toC - heading;
                    while (d >  3.14159265f) d -= 6.2831853f;
                    while (d < -3.14159265f) d += 6.2831853f;
                    heading += d * 2.0f * dt;
                }
                // ?뚮젅?댁뼱 吏곸꽑 ?뚯쭊 諛쒕룞
                chargeCdTimer += dt;
                if (chargeCdTimer >= CHARGE_INT * cdScale()) {
                    chargePhase = 1; chargeTimer = 0.0f; chargeTelegraph = true;
                } else {
                    float ws = wanderSpeed();
                    worldX += cosf(heading) * ws * dt;
                    worldY += sinf(heading) * ws * dt;
                }
                // ????⑦꽩 濡쒗뀒?댁뀡 諛쒕룞 ???쒕룞/?щ━/?λ꼍/?좊났 (?띾룄 鍮꾨? 荑④컧)
                bigCd += dt;
                if (bigCd >= BIG_INT * cdScale() && chargePhase == 0 && !surging) {
                    bigCd = 0.0f; chargeTelegraph = false;
                    shake(8.0f);                 // ????⑦꽩 媛쒖떆 ??吏꾨룞
                    switch (rand() % 4) {
                    case 0: enterRamp(); break;
                    case 1: {  // COIL ???꾩옱 ?꾩튂?먯꽌 洹몃?濡?媛먭린 ?쒖옉(?쒓컙?대룞 X)
                        state = 5; stateTimer = 0.0f;
                        coilCX = px; coilCY = py;
                        float mg = COIL_RMIN + 40.0f;
                        if (coilCX < mg) coilCX = mg; if (coilCX > screenW - mg) coilCX = screenW - mg;
                        if (coilCY < mg) coilCY = mg; if (coilCY > screenH - mg) coilCY = screenH - mg;
                        float ddx = worldX - coilCX, ddy = worldY - coilCY;
                        coilR = std::sqrt(ddx*ddx + ddy*ddy);   // ?대옩???놁씠 ?꾩옱 嫄곕━ = ?먰봽 0
                        coilAng = atan2f(ddy, ddx);
                        break; }
                    case 2: {  // WALL ??媛源뚯슫 履쎌뿉??癒?履쎌쑝濡?媛濡쒖쭏???닿린
                        state = 6; stateTimer = 0.0f; wallFireT = 0.0f;
                        wallDir = (worldX < screenW * 0.5f) ? 1 : -1;
                        heading = (wallDir > 0) ? 0.0f : 3.14159265f;
                        break; }
                    default: { // BURROW ???쒖옄由??좎닔 ??諛쒕컩 ?덇퀬 ???잕뎄移?                        state = 7; stateTimer = 0.0f; burrowPhase = 0; burrowScale = 1.0f;
                        break; }
                    }
                }
                // 諛고쉶 ?쒓컙 醫낅즺 ???붾㈃ 諛??댄깉(怨≪꽑 ?뚯쭊 吏꾩엯)
                if (state == 0 && stateTimer >= WANDER_T) {
                    state = 1; stateTimer = 0.0f;
                    chargePhase = 0; chargeTelegraph = false;
                    heading = atan2f(worldY - screenH*0.5f, worldX - screenW*0.5f);
                    wasInside = onScreen(worldX, worldY);
                }
            }
        }
        else if (state == 1) {       // ?? ?붾㈃諛??댄깉(臾댁쟻) ??
            worldX += cosf(heading) * DASH_SPD * dt;
            worldY += sinf(heading) * DASH_SPD * dt;
            bool inside = onScreen(worldX, worldY);
            if (wasInside && !inside) shakePulse = true;
            wasInside = inside;
            float M = 200.0f;
            if (worldX < -M || worldX > screenW + M || worldY < -M || worldY > screenH + M) {
                pickDash();
                state = 2; stateTimer = 0.0f;
                worldX = dashFromX; worldY = dashFromY;
                for (auto& p : trail) p = glm::vec2(worldX, worldY);
            }
        }
        else if (state == 2) {       // ?? 寃쎈줈 ?덇퀬(?붾㈃諛? 臾댁쟻) ??
            if (stateTimer >= TELEGRAPH) { state = 3; stateTimer = 0.0f; dashT = 0.0f; }
        }
        else if (state == 3) {       // ?? 怨≪꽑 ?ъ쭊???뚯쭊(臾댁쟻) ??
            float chord = std::sqrt((dashToX-dashFromX)*(dashToX-dashFromX) +
                                    (dashToY-dashFromY)*(dashToY-dashFromY)) + 1e-3f;
            dashT += DASH_SPD * dt / chord;
            if (dashT > 1.0f) dashT = 1.0f;
            glm::vec2 p = bezier(dashT);
            worldX = p.x; worldY = p.y;
            glm::vec2 tan = bezierTangent(dashT);
            if (tan.x*tan.x + tan.y*tan.y > 1e-6f) heading = atan2f(tan.y, tan.x);
            if (dashT >= 1.0f) {
                state = 0; stateTimer = 0.0f; ++dashCount;
                spitTimer = 0.0f; surging = false; surgeCd = 0.0f;
                chargePhase = 0; chargeCdTimer = CHARGE_INT * 0.7f; chargeTelegraph = false;
            }
        }
        else if (state == 4) {       // ?? 踰?諛뺢린 愿묐?: 怨좎냽 ?뺢린湲?+ ?띾룄鍮꾨? 遺꾩텧 + 諛뺤쓣?섎줉 媛먯냽 ??
            worldX += rampDX * rampSpeed * dt;
            worldY += rampDY * rampSpeed * dt;
            heading = atan2f(rampDY, rampDX);
            // ?대룞?띾룄 鍮꾨? ??遺꾩텧 ??鍮좊??섎줉 ?먯＜(留롮씠) 醫뚯슦濡?肉쒖쓬
            rampFireTimer += dt;
            float fi = RAMP_FIRE_K / rampSpeed;
            if (rampFireTimer >= fi) {
                rampFireTimer = 0.0f;
                float pxn = -rampDY, pyn = rampDX;
                fireDir(bullets,  pxn,  pyn, RAMP_FIRE_SPD, BUL);
                fireDir(bullets, -pxn, -pyn, RAMP_FIRE_SPD, BUL);
            }
            // 踰?異⑸룎 ??諛섏궗 + 媛먯냽(諛뺤쓣?섎줉 ?먮젮吏? + 諛⑹궗 ?뚰렪
            float mg = HEAD * 0.55f;
            bool hitX = (worldX <= mg) || (worldX >= (float)screenW - mg);
            bool hitY = (worldY <= mg) || (worldY >= (float)screenH - mg);
            if (hitX || hitY) {
                if (worldX < mg) worldX = mg;
                if (worldX > (float)screenW - mg) worldX = (float)screenW - mg;
                if (worldY < mg) worldY = mg;
                if (worldY > (float)screenH - mg) worldY = (float)screenH - mg;
                if (hitX) rampDX = -rampDX;
                if (hitY) rampDY = -rampDY;
                rampSpeed *= RAMP_DECAY;     // 諛뺤쓣?섎줉 媛먯냽
                for (int i = 0; i < RAMP_SHRAP; i++) {
                    float a = (float)i / (float)RAMP_SHRAP * 6.2831853f + (float)(rand()%100)*0.01f;
                    fireDir(bullets, cosf(a), sinf(a), RAMP_SHRAP_SPD, BUL);
                }
                shakePulse = true;
                spawnMini(worldX, worldY, px, py);   // 踰?諛뺤쓣 ?뚮쭏???덈겮 吏??異붽? ?쒕∼
                if (rampSpeed < RAMP_MIN) { backToWander(); bigCd = 0.0f; }   // ???먮젮吏硫?醫낅즺
            }
        }
        else if (state == 5) {       // ?? ?щ━ 媛먭린(A1): ?뚮젅?댁뼱 以묒떖 留?+ 議곗엫 ??
            coilAng += COIL_SPIN * dt;
            coilR += (COIL_RMIN - coilR) * 1.3f * dt;   // ?대뵒???쒖옉?섎뱺 RMIN 濡?遺?쒕읇寃??섎졃(?먰봽 X)
            worldX = coilCX + cosf(coilAng) * coilR;
            worldY = coilCY + sinf(coilAng) * coilR;
            heading = coilAng + 1.5707963f;       // ?묒꽑
            // ?뚮젅?댁뼱媛 留?諛뽰쑝濡?鍮좎졇?섍?硫??щ━ 以묐떒 ??諛고쉶 (媛뉙엺 ?щ엺留??꾪삊)
            float edx = px - coilCX, edy = py - coilCY;
            bool escaped = (edx*edx + edy*edy) > (coilR + 70.0f) * (coilR + 70.0f);
            if (escaped || stateTimer >= COIL_DUR) { backToWander(); bigCd = 0.0f; }
        }
        else if (state == 6) {       // ?? ?λ꼍 遺꾪븷(A3): 媛濡쒖쭏???대ŉ 寃ъ젣 ??
            worldX += (float)wallDir * WALL_SPD * dt;
            heading = (wallDir > 0) ? 0.0f : 3.14159265f;
            wallFireT += dt;
            if (wallFireT >= WALL_FIRE) {
                wallFireT = 0.0f;
                float base = atan2f(py - worldY, px - worldX);
                fireDir(bullets, cosf(base), sinf(base), 330.0f, BUL);
            }
            float M = HEAD;
            if (worldX < -M || worldX > screenW + M) {
                backToWander(); bigCd = 0.0f;
                heading = atan2f(screenH*0.5f - worldY, screenW*0.5f - worldX);
            }
        }
        else {                       // ?? state 7: ?좊났(B4) ???쒖옄由ъ뿉??媛?쇱븠????
            if (burrowPhase == 0) {  // ?쒖옄由?媛?쇱븠湲?異뺤냼) ???좎븘媛??癒쇱? ?щ씪吏吏 ?딆쓬
                burrowScale -= dt / BURROW_SINK;
                if (burrowScale <= 0.0f) {
                    burrowScale = 0.0f; burrowPhase = 1; stateTimer = 0.0f;
                    burrowX = px; burrowY = py;       // 諛쒕컩 ?寃?怨좎젙
                    float m = HEAD;
                    if (burrowX < m) burrowX = m; if (burrowX > screenW - m) burrowX = screenW - m;
                    if (burrowY < m) burrowY = m; if (burrowY > screenH - m) burrowY = screenH - m;
                }
            } else if (burrowPhase == 1) { // 諛쒕컩 ?덇퀬(?꾩쟾 ?좊났)
                if (stateTimer >= BURROW_WARN) {
                    burrowPhase = 2; stateTimer = 0.0f;
                    worldX = burrowX; worldY = burrowY;
                    for (auto& p : trail) p = glm::vec2(worldX, worldY);
                    for (int i = 0; i < BURROW_SHRAP; i++) {
                        float a = (float)i / (float)BURROW_SHRAP * 6.2831853f + (float)(rand()%100)*0.01f;
                        fireDir(bullets, cosf(a), sinf(a), BURROW_SHRAP_SPD, BUL);
                    }
                    shakePulse = true;
                }
            } else {                       // ?잕뎄移??쒖옄由??뺣?) ??諛고쉶
                burrowScale += dt / 0.25f;
                if (burrowScale >= 1.0f) { burrowScale = 1.0f; backToWander(); bigCd = 0.0f; }
            }
        }

        // ?? ?대룞 ?ъ텧 ???ㅼ젣濡??대룞?섎뒗 ?곹깭?먯꽌 醫뚯슦濡??곗씠???꾩쓣 ?섎┝ ??
        //   "?대룞???뚮쭏???꾩씠 ?좎븘媛꾨떎" ??諛고쉶/?뚯쭊/?쒕룞 以?吏꾪뻾 ?섏쭅 ?묒쁿?쇰줈 ?꾩닔.
        bool moving = (state == 0 && chargePhase != 1) || state == 3;   // state4(愿묐?)? ?먯껜 遺꾩텧
        if (moving && onScreen(worldX, worldY)) {
            shedTimer += dt;
            if (shedTimer >= SHED_INT * cdScale()) {
                shedTimer = 0.0f;
                float pxn = -sinf(heading), pyn = cosf(heading);
                fireDir(bullets,  pxn,  pyn, SHED_SPD, BUL);
                fireDir(bullets, -pxn, -pyn, SHED_SPD, BUL);
            }
        }
        // ?? ?덊뵾 吏猶?媛깆떊 + ?묒큺 ??컻 ??
        for (size_t i = 0; i < hazards.size(); ) {
            Hazard& h = hazards[i];
            h.life -= dt;
            float dx = px - h.x, dy = py - h.y;
            if ((dx*dx + dy*dy) < h.r * h.r) {   // ?묒큺 ??諛⑹궗???뚰렪 ??컻
                for (int k = 0; k < MOLT_SHRAP; k++) {
                    float a = (float)k / (float)MOLT_SHRAP * 6.2831853f;
                    fireFrom(bullets, h.x, h.y, cosf(a), sinf(a), MOLT_SHRAP_SPD, BUL);
                }
                h.life = 0.0f;
            }
            if (h.life <= 0.0f) { ++i; continue; }
            ++i;
        }
        hazards.erase(std::remove_if(hazards.begin(), hazards.end(),
            [](const Hazard& h){ return h.life <= 0.0f; }), hazards.end());

        // ?? 硫붾え由??꾩닔 ?붿긽 + 瑗щ━ ?ы겕諛곗텧 + VFX ????
        {
            float mvx = worldX - prevWorldX, mvy = worldY - prevWorldY;
            float moveSpd = std::sqrt(mvx * mvx + mvy * mvy) / (dt > 1e-4f ? dt : 1e-4f);
            bool fastMove = moveSpd > 620.0f || state == 1 || state == 3 || state == 4 ||
                            state == 5 || state == 6 || chargePhase == 2;
            if (fastMove && !(state == 7 && burrowPhase < 2)) {
                if (rand() % 2 == 0)
                    spawnGhost(worldX, worldY, HEAD * 0.85f);
                for (int gi = 2; gi <= activeSeg; gi += 3)
                    spawnGhost(segPos(gi).x, segPos(gi).y, segSize(gi) * 1.05f);
            }
            prevWorldX = worldX;
            prevWorldY = worldY;
        }
        if (state != 7) {
            tailDropCd -= dt;
            if (tailDropCd <= 0.0f) {
                tailDropCd = 1.0f;
                glm::vec2 tail = segPos(activeSeg);
                ErrorNode en; en.x = tail.x; en.y = tail.y; en.fuse = 2.0f; en.alive = true;
                errorNodes.push_back(en);
            }
        }
        for (auto& en : errorNodes) {
            if (!en.alive) continue;
            en.fuse -= dt;
            if (en.fuse <= 0.0f) {
                for (int d = 0; d < 4; d++) {
                    float ang = (float)d * 1.5707963f;
                    fireFrom(bullets, en.x, en.y, cosf(ang), sinf(ang), 270.0f,
                             glm::vec3(1.0f, 0.35f, 0.55f));
                }
                for (int k = 0; k < 8; k++) {
                    float a = (float)k / 8.0f * 6.2831853f;
                    HitSpark sp;
                    sp.x = en.x; sp.y = en.y;
                    sp.vx = cosf(a) * 220.0f; sp.vy = sinf(a) * 220.0f;
                    sp.life = 0.25f;
                    hitSparks.push_back(sp);
                }
                Hazard h;
                h.x = en.x; h.y = en.y; h.maxLife = h.life = 3.0f; h.r = 34.0f;
                hazards.push_back(h);
                en.alive = false;
            }
        }
        errorNodes.erase(std::remove_if(errorNodes.begin(), errorNodes.end(),
            [](const ErrorNode& en){ return !en.alive; }), errorNodes.end());
        tickVfx(dt);

        // 沅ㅼ쟻 湲곕줉
        trail.push_front(glm::vec2(worldX, worldY));
        if ((int)trail.size() > NSEG * SEG_STEP + 8) trail.pop_back();

        // ?묒큺 ?곕?吏 (?좊났 ?좎닔/?덇퀬 以묒뿉??蹂몄껜媛 ?놁쓬 = 臾댁젒珥?
        bool intangible = (state == 7 && burrowPhase < 2);
        if (!intangible) {
            bool dashing = (state == 1 || state == 3 || state == 4 || state == 5 ||
                            state == 6 || chargePhase == 2 || (state == 7 && burrowPhase == 2));
            float hcr = HEAD * 0.78f;
            float hdx = px - worldX, hdy = py - worldY;
            if (hdx*hdx + hdy*hdy < hcr * hcr)
                HurtPlayer(playerHP, (dashing ? 22.0f : 12.0f) * dt);
            for (int i = 1; i <= activeSeg; i++) {
                glm::vec2 s = segPos(i);
                float sr = segSize(i) + 4.0f;
                float sdx = px - s.x, sdy = py - s.y;
                if (sdx*sdx + sdy*sdy < sr * sr) HurtPlayer(playerHP, 8.0f * dt);
            }
        }
    }

    glm::vec2 segPos(int i) const {
        int idx = i * SEG_STEP;
        if (idx >= (int)trail.size()) idx = (int)trail.size() - 1;
        if (idx < 0) idx = 0;
        return trail[idx];
    }

    // ?? FX ?뚮뜑 (李??대━??X) ???붿긽/?먮윭?몃뱶/議곗???踰??⑥젙 ??
    void renderFx(float t, float aimX, float aimY) const {
        BindMainShader();

        for (const auto& g : ghosts) {
            float a = (g.maxLife > 0.0f) ? (g.life / g.maxLife) : 0.0f;
            float sz = g.r * (0.55f + 0.45f * a);
            drawRect(g.x - sz * 0.6f, g.y - sz * 0.45f, sz * 1.2f, sz * 0.9f,
                     0.95f, 0.25f, 0.75f, 0.22f * a);
            drawCircle(g.x, g.y, sz * 0.35f, 0.95f, 0.35f, 0.85f, 0.35f * a);
        }
        for (const auto& en : errorNodes) {
            if (!en.alive) continue;
            drawErrorNode(en.x, en.y, en.fuse, t);
        }
        for (const auto& sp : hitSparks) {
            float a = sp.life / 0.18f; if (a > 1.0f) a = 1.0f;
            drawCircle(sp.x, sp.y, 4.5f * a, 0.35f, 0.95f, 1.0f, a);
            drawCircle(sp.x, sp.y, 2.0f * a, 1.0f, 1.0f, 1.0f, a);
        }
        if (lockOnActive() && !(state == 7 && burrowPhase == 1)) {
            float dx = aimX - worldX, dy = aimY - worldY;
            float len = std::sqrt(dx * dx + dy * dy) + 1e-3f;
            int steps = (int)(len / 12.0f);
            if (steps < 4) steps = 4;
            float blink = 0.55f + 0.45f * sinf(t * 20.0f);
            for (int i = 0; i <= steps; i++) {
                if (i % 2 != 0) continue;
                float u = (float)i / (float)steps;
                drawCircle(worldX + dx * u, worldY + dy * u, 2.8f,
                           0.35f, 0.95f, 1.0f, (0.35f + 0.45f * blink) * (1.0f - u * 0.2f));
            }
            drawCircle(aimX, aimY, 7.0f, 0.35f, 0.95f, 1.0f, 0.25f * blink);
        }

        // (00) ?щ옒??踰????ㅼ삩 ?ㅻ뱶媛 吏곸꽑?쇰줈 援녹쓬
        for (const auto& w : walls) {
            int nc = (int)w.cellHp.size();
            float anim = (w.spawnT > 0.0f) ? (w.spawnT / WALL_ANIM) : 0.0f;
            for (int i = 0; i < nc; i++) {
                if (w.cellHp[i] <= 0) continue;
                float along = ((float)i + 0.5f) * WALL_CELL;
                float cx = w.horiz ? along : w.coord;
                float cy = w.horiz ? w.coord : along;
                float delay = anim - (float)i * 0.02f;
                float drop = (delay > 0.0f) ? delay * 42.0f * sinf((float)i + t * 20.0f) : 0.0f;
                float ca = (delay > 0.0f) ? (1.0f - delay) : 1.0f; if (ca < 0.2f) ca = 0.2f;
                float oy = cy + (w.horiz ? drop : 0.0f);
                float ox = cx + (w.horiz ? 0.0f : drop);
                float dmg01 = (float)w.cellHp[i] / (float)WALL_CELLHP;
                bool alt = (i & 1) != 0;
                float sr = WALL_CELL * 0.38f;
                drawCircle(ox, oy, sr * 1.2f, 0.55f, 0.15f, 0.85f, 0.10f * ca);
                drawDiamond(ox, oy, sr,
                            alt ? 0.95f : 0.30f, alt ? 0.35f : 0.88f, alt ? 0.75f : 1.0f,
                            ca * (0.35f + 0.65f * dmg01));
                drawCircle(ox, oy, sr * 0.35f * dmg01, 1.0f, 0.92f, 1.0f, ca * 0.85f);
            }
        }

        // (0) 遺덉븞??肄붿뼱 ???묒큺 ??컻 (二쇳솴 ?뚮씪利덈쭏 援?
        for (const auto& h : hazards) {
            float pul = 0.5f + 0.5f * sinf(t * 14.0f + h.x * 0.05f);
            drawCircle(h.x, h.y, h.r * 1.3f, 1.0f, 0.45f, 0.12f, 0.14f);
            drawCircle(h.x, h.y, h.r * (0.85f + pul * 0.15f), 1.0f, 0.55f, 0.18f, 0.55f);
            drawCircle(h.x, h.y, h.r * 0.45f, 1.0f, 0.85f, 0.35f, 0.95f);
            for (int sp = 0; sp < 8; sp++) {
                float ang = (float)sp / 8.0f * 6.2831853f + t * 2.0f;
                drawDiamond(h.x + cosf(ang) * h.r * 0.9f, h.y + sinf(ang) * h.r * 0.9f,
                            10.0f + pul * 4.0f, 1.0f, 0.65f, 0.22f, 0.75f);
            }
        }

        // (0c) child adds ??centiPass ?먯꽌 drawMini 濡??뚮뜑
        if (state == 7 && burrowPhase == 1) {
            float p = stateTimer / BURROW_WARN;
            float blink = 0.5f + 0.5f * sinf(t * 24.0f);
            drawCircle(burrowX, burrowY, 30.0f + 80.0f * p, 0.6f, 0.15f, 0.15f, 0.18f + 0.2f * p);
            drawCircle(burrowX, burrowY, 14.0f + 30.0f * p, 1.0f, 0.3f, 0.2f, 0.3f + 0.4f * blink);
        }
        // (1) 怨≪꽑 ?뚯쭊 ?덇퀬 ???뚮씪利덈쭏 沅ㅼ쟻
        if (state == 2) {
            float blink = 0.55f + 0.45f * sinf(t * 22.0f);
            int n = 36;
            glm::vec2 prev = bezier(0.0f);
            for (int i = 1; i <= n; i++) {
                glm::vec2 p = bezier((float)i / (float)n);
                drawPlasmaLink(prev, p, 5.0f, t, i);
                if (i % 3 == 0)
                    drawPlasmaNode(p.x, p.y, 10.0f + blink * 4.0f, blink, t, i, false, 0.0f);
                prev = p;
            }
            int marks = 5;
            for (int kk = 1; kk <= marks; kk++) {
                float tt = (float)kk / (float)(marks + 1);
                glm::vec2 p  = bezier(tt);
                glm::vec2 pf = bezier(tt + 0.025f);
                float ang = atan2f(pf.y - p.y, pf.x - p.x);
                drawDiamond(p.x + cosf(ang) * 14.0f, p.y + sinf(ang) * 14.0f,
                            14.0f, 1.0f, 0.72f, 0.28f, 0.55f + 0.4f * blink);
            }
        }
        if (chargeTelegraph) {
            float blink = 0.5f + 0.5f * sinf(t * 18.0f);
            float dxn = cosf(heading), dyn = sinf(heading);
            glm::vec2 prev = glm::vec2(worldX, worldY);
            for (int i = 1; i <= 14; i++) {
                float tt = (float)i / 14.0f;
                glm::vec2 p(worldX + dxn * tt * 420.0f, worldY + dyn * tt * 420.0f);
                drawPlasmaLink(prev, p, 4.5f, t, i + 50);
                if (i % 2 == 0)
                    drawCircle(p.x, p.y, 7.0f + tt * 3.0f, 1.0f, 0.55f, 0.22f, 0.35f + 0.45f * blink);
                prev = p;
            }
        }
        BatchFlush();
    }

    // ?? 蹂몄껜 ???뚮씪利덈쭏 ?몃뱶 泥댁씤 + ?ы겕 肄붿뼱 ??
    void renderBody(float t) const {
        if (state == 7 && burrowPhase == 1) return;
        BindMainShader();
        float sc = (state == 7) ? burrowScale : 1.0f;

        glm::vec2 prev = glm::vec2(worldX, worldY);
        for (int i = 1; i <= activeSeg; i++) {
            glm::vec2 s = segPos(i);
            float sz = segSize(i) * sc;
            float br = 0.5f + 0.5f * (1.0f - (float)(i - 1) / (float)(activeSeg > 1 ? activeSeg - 1 : 1));
            drawPlasmaLink(prev, s, 7.0f, t, i);
            if (segFlashing(i)) {
                drawCircle(s.x, s.y, sz * 1.35f, 1.0f, 1.0f, 1.0f, 0.92f);
                drawCircle(s.x, s.y, sz * 0.9f, 0.35f, 0.95f, 1.0f, 0.55f);
            } else {
                drawPlasmaNode(s.x, s.y, sz * 1.05f, br, t, i, (i % 3) == 0, heading);
            }
            prev = s;
        }

        bool dash = (state == 1 || state == 3 || state == 4 || state == 5 ||
                     state == 6 || chargePhase == 2);
        float H = HEAD * sc;
        drawKillMark(worldX, worldY, H * 0.95f, t, lockOnActive() || dash);

        BatchFlush();
    }
};
