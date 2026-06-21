#pragma once
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#include "Bullet.h"
#include "PlayerStats.h"

// ?????????????????????????????????????????????????????????????
// ?대━紐⑦봽 蹂댁뒪 (蹂대씪??쨌 ??쨌 ?ш? ?깆옣) ????蹂?섑삎
//   ?? TRIANGLE / RHOMBUS / DIAMOND 瑜??쇱젙 ?쒓컙留덈떎 臾댁옉???꾪솚
//     TRIANGLE : ?붾㈃ ?쒖そ ????諛섎?履??앹쑝濡?媛濡쒖?瑜대뒗 '怨좎냽 ?몃え ?꾨룄'
//                癒쇱? ?쒖옉 紐⑥꽌由ъ뿉 寃쎄퀬 ?쒖떆(?붾젅洹몃옒?? ???댄썑 3珥덇컙 留ㅼ슦 鍮좊Ⅸ
//                ?몃え?ㅼ씠 以꾩쨪???잛븘??援ъ뿭??媛??梨꾩슦硫??대룞 (?≪쑝硫?EXP, 異붿쟻 X)
//     RHOMBUS  : ?쒕뜡 諛⑺뼢 ?덉씠? ??癒쇱? 寃쎄퀬???쒖떆 ??諛쒖궗. 二쇰? ?대몢???쒖빞 諛대뱶.
//     DIAMOND  : 珥앹븣 諛섏궗(臾댁쟻). 諛섏궗?꾩? ?뚮젅?댁뼱 ?먮옒 ?꾨젰 洹몃?濡?
//   ?섏씠利? (HP 50% ?댄븯): ?붾㈃ 2諛??뺤옣(main ??g_ViewZoom 泥섎━) + ??媛뺥솕 +
//     二쇰? 李⑦겕??3媛?媛?1000HP, ?쒓굅 ?꾧퉴吏 蹂몄껜 臾댁쟻)
// ?????????????????????????????????????????????????????????????

enum class PForm { TRIANGLE, RHOMBUS, DIAMOND };

struct PSwarm  { float x, y, vx, vy; float life; bool alive; };
struct PChakram{ float angle; float hp; bool alive; };

class PolymorphBoss {
public:
    float worldX, worldY;
    float hp, maxHp;
    bool  alive = true, exploded = false;
    int   screenW, screenH;

    PForm form = PForm::TRIANGLE;
    float formTimer = 0.0f;
    float formDuration = 6.0f;
    bool  phase2 = false;

    // wander
    float targetX, targetY, wanderTimer = 0.0f;

    // TRIANGLE ???앹뿉???앹쑝濡?媛濡쒖?瑜대뒗 怨좎냽 ?꾨룄
    std::vector<PSwarm> swarm;
    bool  triWarn   = false;
    bool  triActive = false;
    float triWarnTimer = 0.0f;
    float triTimer     = 0.0f;
    float triCd        = 0.4f;
    float triSpawnAccum = 0.0f;
    float triDirX = 1.0f, triDirY = 0.0f;  // 吏꾪뻾 諛⑺뼢 (?곹븯/醫뚯슦)

    // RHOMBUS (laser)
    bool  laserActive = false;
    bool  laserWarn   = false;
    float laserWarnTimer = 0.0f;
    float laserX = 0, laserY = 0, laserDirX = 1, laserDirY = 0;
    float laserTimer = 0.0f, laserCd = 0.0f;
    float laserHalf = 20.0f;     // ?쒖빞 諛대뱶 諛섑룺 (?섏씠利? +10)

    // CHAKRAM (phase2 諛⑹뼱留?
    std::vector<PChakram> chakrams;

    static constexpr float BODY = 105.0f;         // ??(1.5諛?
    static constexpr float SWARM_DMG = 4.0f;      // ?몃え ?묒큺(?〓す ?덈컲湲?
    static constexpr float LASER_DPS = 12.0f;     // 猷⑥떆?쒖떇: ?먯＜ ?섎릺 ?쒖? ?꾩＜ ??쾶

    PolymorphBoss(int sw, int sh, float hpInit) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.5f;  worldY = sh * 0.30f;
        targetX = worldX; targetY = worldY;
        pickForm(true);
    }

    // ?? ?곹깭 吏덉쓽 ??
    bool shielded()   const { return phase2 && !chakrams.empty(); } // 李⑦겕???⑥쑝硫?臾댁쟻
    bool reflecting() const { return form == PForm::DIAMOND; }       // 諛섏궗
    bool damageable() const { return !reflecting() && !shielded(); }

    void pickNewTarget() {
        float m = 160.0f;
        targetX = m + (float)(rand() % std::max(1, screenW - 2*(int)m));
        targetY = m + (float)(rand() % std::max(1, screenH - 2*(int)m));
    }

    void pickForm(bool first) {
        PForm prev = form;
        do { form = (PForm)(rand() % 3); } while (!first && form == prev);
        formTimer = 0.0f;
        triWarn = triActive = false;
        triCd = 0.4f; triSpawnAccum = 0.0f; triTimer = 0.0f;
        laserActive = laserWarn = false;
        laserCd = 0.6f;
    }

    void enterPhase2() {
        phase2 = true;
        laserHalf = 30.0f;                 // 留덈쫫紐?踰붿쐞 +10
        formDuration = 5.0f;               // ???ъ궗??荑?媛먯냼
        chakrams.clear();
        for (int i = 0; i < 3; i++) {
            PChakram c; c.angle = (float)i / 3.0f * 6.2831853f;
            c.hp = 1000.0f; c.alive = true;
            chakrams.push_back(c);
        }
    }

    // ?? TRIANGLE: ?앹뿉???앹쑝濡?媛濡쒖?瑜대뒗 ?꾨룄 ??
    void startTriWarn() {
        triWarn = true;
        triWarnTimer = phase2 ? 0.8f : 1.0f;   // 寃쎄퀬 ?쒓컙
        switch (rand() % 4) {
        case 0: triDirX =  1; triDirY =  0; break;
        case 1: triDirX = -1; triDirY =  0; break;
        case 2: triDirX =  0; triDirY =  1; break;
        default:triDirX =  0; triDirY = -1; break;
        }
    }

    void spawnTriRow() {
        // ?쒖옉 紐⑥꽌由??꾩껜??嫄몄퀜 臾댁옉?꾨줈 ??以?遺꾨웾 spawn (怨좎냽, 吏곸꽑 ?대룞)
        // ?섏씠利?: ?붾㈃??2諛곕줈 ?뺤옣?섎?濡??뺤옣???곸뿭 ?꾩껜瑜???룄濡?spawn 踰붿쐞/?섎웾 利앷?
        float sp = (phase2 ? 920.0f : 800.0f) + (float)(rand() % 160);
        int   perRow = phase2 ? 6 : 3;
        float life   = phase2 ? 5.5f : 3.5f;
        float exX = phase2 ? (float)screenW * 0.5f : 0.0f;
        float exY = phase2 ? (float)screenH * 0.5f : 0.0f;
        float minX = -exX, maxX = (float)screenW + exX;
        float minY = -exY, maxY = (float)screenH + exY;
        int   spanX = std::max(1, (int)(maxX - minX));
        int   spanY = std::max(1, (int)(maxY - minY));
        for (int i = 0; i < perRow; i++) {
            PSwarm s; s.life = life; s.alive = true;
            if (triDirX != 0.0f) {                    // 媛濡??대룞
                s.x  = (triDirX > 0) ? minX - 20.0f : maxX + 20.0f;
                s.y  = minY + (float)(rand() % spanY);
                s.vx = triDirX * sp; s.vy = 0.0f;
            } else {                                  // ?몃줈 ?대룞
                s.x  = minX + (float)(rand() % spanX);
                s.y  = (triDirY > 0) ? minY - 20.0f : maxY + 20.0f;
                s.vx = 0.0f; s.vy = triDirY * sp;
            }
            swarm.push_back(s);
        }
    }

    float arenaExX() const { return phase2 ? (float)screenW * 0.5f : 0.0f; }
    float arenaExY() const { return phase2 ? (float)screenH * 0.5f : 0.0f; }
    // ?덉씠?媛 ?뺤옣 ?곸뿭 ?앷퉴吏 ?용룄濡?異⑸텇??湲?湲몄씠
    float laserReach() const { return (phase2 ? 2.2f : 1.0f) * (float)(screenW + screenH); }

    // ?? RHOMBUS: 寃쎄퀬??議곗?留?諛쒖궗 X) ?? ?뺤옣??紐⑥꽌由ъ뿉???쒖옉
    void aimLaser(float px, float py) {
        float exX = arenaExX(), exY = arenaExY(), m = 20.0f;
        int spanX = std::max(1, (int)(screenW + 2*exX));
        int spanY = std::max(1, (int)(screenH + 2*exY));
        switch (rand() % 4) {
        case 0: laserX = -exX + (float)(rand()%spanX); laserY = -exY - m;            break;
        case 1: laserX = -exX + (float)(rand()%spanX); laserY = screenH + exY + m;   break;
        case 2: laserX = -exX - m;            laserY = -exY + (float)(rand()%spanY); break;
        default:laserX = screenW + exX + m;   laserY = -exY + (float)(rand()%spanY); break;
        }
        float dx = px - laserX, dy = py - laserY;
        float d  = std::sqrt(dx*dx + dy*dy) + 1e-3f;
        laserDirX = dx/d; laserDirY = dy/d;
    }

    static float segDist(float px, float py, float ax, float ay, float bx, float by) {
        float abx=bx-ax, aby=by-ay, l2=abx*abx+aby*aby, t=0.0f;
        if (l2>1e-6f){ t=((px-ax)*abx+(py-ay)*aby)/l2; t=t<0?0:(t>1?1:t);}
        float cx=ax+abx*t, cy=ay+aby*t, dx=px-cx, dy=py-cy;
        return std::sqrt(dx*dx+dy*dy);
    }

    void Update(float px, float py, float dt, float& playerHP,
                std::vector<Bullet>& bullets) {
        if (!alive) return;

        // ?섏씠利? 吏꾩엯
        if (!phase2 && hp <= maxHp * 0.5f) enterPhase2();

        // 蹂몄껜 wander
        wanderTimer += dt;
        if (wanderTimer >= 2.5f) { pickNewTarget(); wanderTimer = 0.0f; }
        worldX += (targetX - worldX) * 1.2f * dt;
        worldY += (targetY - worldY) * 1.2f * dt;

        // 李⑦겕??怨듭쟾
        for (auto& c : chakrams) { if (c.alive) c.angle += 2.0f * dt; }
        chakrams.erase(std::remove_if(chakrams.begin(), chakrams.end(),
            [](const PChakram& c){ return !c.alive; }), chakrams.end());

        // ???꾪솚 (?꾨룄/?덉씠? 吏꾪뻾 以묒뿉???딆? ?딆쓬)
        formTimer += dt;
        if (formTimer >= formDuration && !triActive && !triWarn &&
            !laserActive && !laserWarn)
            pickForm(false);

        // ?? ?쇰퀎 ?됰룞 ??
        switch (form) {
        case PForm::TRIANGLE: {
            if (!triActive && !triWarn) {
                triCd -= dt;
                if (triCd <= 0.0f) startTriWarn();
            }
            if (triWarn) {
                triWarnTimer -= dt;
                if (triWarnTimer <= 0.0f) {
                    triWarn = false; triActive = true;
                    triTimer = 0.0f; triSpawnAccum = 0.0f;
                }
            }
            if (triActive) {
                triTimer += dt;
                triSpawnAccum += dt;
                float rowInterval = phase2 ? 0.09f : 0.13f;
                if (triSpawnAccum >= rowInterval) {
                    triSpawnAccum = 0.0f;
                    spawnTriRow();
                }
                if (triTimer >= 3.0f) {           // 3珥덇컙 ?꾨룄
                    triActive = false;
                    triCd = phase2 ? 1.0f : 1.8f;
                }
            }
            break;
        }
        case PForm::RHOMBUS: {
            // 猷⑥떆?쒖떇: 吏㏃? 寃쎄퀬 ??吏㏃? 鍮???吏㏃? 荑???鍮좊Ⅴ寃?諛섎났
            if (!laserActive && !laserWarn) {
                laserCd -= dt;
                if (laserCd <= 0.0f) {
                    aimLaser(px, py);
                    laserWarn = true;
                    laserWarnTimer = phase2 ? 0.35f : 0.45f;
                }
            }
            if (laserWarn) {
                laserWarnTimer -= dt;
                if (laserWarnTimer <= 0.0f) {
                    laserWarn = false; laserActive = true; laserTimer = 0.0f;
                }
            }
            if (laserActive) {
                laserTimer += dt;
                float ex = laserX + laserDirX * laserReach();
                float ey = laserY + laserDirY * laserReach();
                if (segDist(px, py, laserX, laserY, ex, ey) < 14.0f)
                    HurtPlayer(playerHP, LASER_DPS * dt);
                if (laserTimer >= 0.7f) {            // 鍮?吏??吏㏐쾶
                    laserActive = false;
                    laserCd = phase2 ? 0.35f : 0.6f;  // ?ㅼ쓬 鍮붽퉴吏 荑?吏㏐쾶
                }
            }
            break;
        }
        case PForm::DIAMOND:
            break;  // 諛섏궗??main 異⑸룎?먯꽌 泥섎━
        }

        // ?몃え 臾대━ (吏곸꽑 ?대룞, 異붿쟻 ????
        for (auto& s : swarm) {
            if (!s.alive) continue;
            s.x += s.vx * dt; s.y += s.vy * dt;
            s.life -= dt;
            float dx = px - s.x, dy = py - s.y;
            if (dx*dx + dy*dy < 15.0f*15.0f) { HurtPlayer(playerHP, SWARM_DMG); s.alive = false; }
            // ?섏씠利? ?뺤옣 ?곸뿭源뚯? 媛濡쒖쭏?ъ빞 ?섎?濡?despawn 寃쎄퀎???뺤옣
            float mxB = (phase2 ? (float)screenW * 0.5f : 0.0f) + 160.0f;
            float myB = (phase2 ? (float)screenH * 0.5f : 0.0f) + 160.0f;
            if (s.life <= 0.0f ||
                s.x < -mxB || s.x > screenW + mxB ||
                s.y < -myB || s.y > screenH + myB)
                s.alive = false;
        }
        swarm.erase(std::remove_if(swarm.begin(), swarm.end(),
            [](const PSwarm& s){ return !s.alive; }), swarm.end());
    }
};
