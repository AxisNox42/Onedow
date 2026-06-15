#pragma once
#include <vector>
#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#include "Bullet.h"
#include "DrawPrim.h"   // 보스 자체 렌더(완전 분리) — drawRect/drawNeonBorder/drawDiamond 등

// ─────────────────────────────────────────────────────────────
// BUG.proc — 버그 (지네형 보스) — "창(window) 으로 이루어진 거대 웜"
//   머리 = 큰 가짜 창(BUG.proc), 몸통 = 줄줄이 이어진 작은 가짜 창들.
//   몸체가 매우 길어 화면을 휘감고 다니는 "진짜 보스" 스케일.
//
//   ■ 평상시(배회): 지그재그로 압박(피격 가능 = 딜 타임).
//   ■ 라이트 견제: 데이터 토사(부채꼴), 데이터 폭주(나선), 플레이어 직선 돌진.
//   ■ 곡선 돌진: 화면 밖 이탈(무적) → 경로 예고선 → 반대편 곡선 재진입.
//   ■ 대형 패턴(BIG, 4종 랜덤 로테이션):
//       0 벽 들이박기 난동(파편 살포 + 진동)         [RAMP]
//       1 똬리 감기(플레이어를 몸통 링으로 가둠·조임)  [COIL]   ← A1
//       2 장벽 분할(몸을 가로질러 펴 화면을 반으로)     [WALL]   ← A3
//       3 잠복 후 솟구침(발밑 예고 → 폭발)             [BURROW] ← B4
//   ■ 상시: EMP 트레일(고속 이동 중 잔류 감전장)         [EMP]    ← B6
//           탈피/사출(HP 임계마다 꼬리 마디 → 지뢰 + 가속) [MOLT]   ← A2
//
//   설계: 트리플MG/미니건 대시로 "다가오는 머리"를 즉살하지 못하도록
//         거리 강제·공간 분할·잠복 위주. 받는 피해 35% 감소(dmgTakenMult).
//         이동이 빠를수록(돌진/탈피 누적) 스킬 쿨다운이 짧아짐(속도 비례).
// ─────────────────────────────────────────────────────────────
class CentipedeBoss {
public:
    float worldX, worldY;          // 머리 위치
    float hp, maxHp;
    bool  alive = true;
    bool  exploded = false;
    int   screenW, screenH;
    // 프로토타입: 순수 보스전 — 잡몹 흡수 HP + 피해 감소(트리플MG/미니건 DPS 로 즉살 방지)
    float dmgTakenMult = 0.65f;    // 받는 피해 ×0.65 (35% 감소)

    // 상태: 0=배회 / 1=화면밖 이탈 / 2=경로예고 / 3=곡선 재진입
    //       4=벽 난동 / 5=똬리 / 6=장벽 / 7=잠복
    int   state      = 0;
    float stateTimer = 0.0f;
    float wanderTimer = 0.0f;
    float heading    = 0.0f;       // 진행 방향(rad)

    // 돌진 곡선 경로 (2차 베지어)
    float dashFromX = 0, dashFromY = 0, dashToX = 0, dashToY = 0;
    float dashCtrlX = 0, dashCtrlY = 0;
    float dashT = 0.0f;

    bool  wasInside  = true;
    bool  shakePulse = false;      // 화면 진동 1회 트리거 — main 이 읽고 끔
    int   dashCount  = 0;          // 완료 돌진 횟수(배회 가속)

    std::vector<glm::vec2> trail;  // 머리 궤적(세그먼트 추종)

    static constexpr int   NSEG       = 18;      // 꼬리 세그먼트 수(엄청 길게)
    static constexpr int   SEG_STEP   = 6;       // 세그먼트 간 궤적 인덱스 간격(작을수록 촘촘)
    static constexpr float HEAD       = 92.0f;   // 머리 충돌 반경
    static constexpr float SEG_NEAR   = 44.0f;   // 머리에 가장 가까운 세그먼트(반크기)
    static constexpr float SEG_FAR    = 20.0f;   // 꼬리 끝(가장 작음)
    static constexpr float WANDER_SPD = 230.0f;  // 기본 배회 속도
    static constexpr float WANDER_GAIN= 0.12f;   // 돌진 1회당 +12%
    static constexpr float WANDER_CAP = 2.6f;    // 속도 배율 상한
    static constexpr float DASH_SPD   = 1500.0f; // 돌진/이탈 속도
    static constexpr float WANDER_T   = 10.0f;   // 배회 시간(딜 타임)
    static constexpr float TELEGRAPH  = 1.2f;    // 곡선 돌진 예고
    static constexpr float TURN_INT   = 0.5f;    // 지그재그 전환 주기

    // 데이터 토사(부채꼴)
    static constexpr float SPIT_INT   = 5.5f;
    static constexpr int   SPIT_N     = 5;
    static constexpr float SPIT_SPD   = 300.0f;
    float spitTimer = 0.0f;
    // 플레이어 직선 돌진
    static constexpr float CHARGE_INT    = 12.0f;
    static constexpr float CHARGE_WINDUP = 0.55f;
    static constexpr float CHARGE_DUR    = 0.65f;
    static constexpr float CHARGE_SPD    = 1280.0f;
    int   chargePhase = 0;       // 0=배회 / 1=조준 / 2=돌진
    float chargeCdTimer = 0.0f;
    float chargeTimer = 0.0f;
    float chargeDX = 0.0f, chargeDY = 0.0f;
    bool  chargeTelegraph = false;
    // 데이터 폭주(나선)
    static constexpr float SURGE_INT  = 8.5f;
    static constexpr float SURGE_DUR  = 1.4f;
    static constexpr float SURGE_TICK = 0.09f;
    static constexpr float SURGE_SPD  = 280.0f;
    float surgeCd = 0.0f, surgeT = 0.0f, surgeTick = 0.0f, surgeAng = 0.0f;
    bool  surging = false;

    // ── 대형 패턴 로테이션(난동/똬리/장벽/잠복) ──
    static constexpr float BIG_INT = 7.5f;       // 대형 패턴 쿨다운(속도 비례 감소)
    float bigCd = 0.0f;
    // 벽 들이박기 난동
    static constexpr int   RAMP_HITS      = 4;
    static constexpr float RAMP_SPD       = 1400.0f;
    static constexpr int   RAMP_SHRAP     = 12;
    static constexpr float RAMP_SHRAP_SPD = 360.0f;
    int   rampHits = 0;
    float rampDX = 0.0f, rampDY = 0.0f;
    // 똬리 감기(A1)
    static constexpr float COIL_DUR    = 2.6f;
    static constexpr float COIL_SPIN   = 3.6f;   // rad/s
    static constexpr float COIL_R0     = 330.0f;
    static constexpr float COIL_RMIN   = 150.0f;
    static constexpr float COIL_SHRINK = 70.0f;  // px/s (조임)
    float coilCX = 0, coilCY = 0, coilR = 0, coilAng = 0;
    // 장벽 분할(A3)
    static constexpr float WALL_SPD  = 780.0f;
    static constexpr float WALL_FIRE = 0.16f;
    int   wallDir = 1;
    float wallFireT = 0.0f;
    // 잠복(B4)
    static constexpr float BURROW_WARN      = 0.75f;
    static constexpr int   BURROW_SHRAP     = 18;
    static constexpr float BURROW_SHRAP_SPD = 380.0f;
    int   burrowPhase = 0;       // 0=잠수(화면밖) / 1=발밑 예고 / 2=솟구침
    float burrowX = 0, burrowY = 0;

    // ── 이동 사출 — 움직이며 좌우로 데이터 탄을 흘림(상시 압박) ──
    static constexpr float SHED_INT = 0.22f;     // 촘촘하게(자주)
    static constexpr float SHED_SPD = 200.0f;
    float shedTimer = 0.0f;
    // ── 탈피 지뢰 ──
    struct Hazard { float x, y, life, maxLife, r; };
    std::vector<Hazard> hazards;
    // 탈피/사출(A2)
    int   activeSeg = NSEG;      // 현재 살아있는 세그먼트 수(탈피로 줄어듦)
    int   moltLevel = 0;
    static constexpr float MOLT_R         = 42.0f;
    static constexpr float MOLT_LIFE      = 9.0f;
    static constexpr int   MOLT_SHRAP     = 10;
    static constexpr float MOLT_SHRAP_SPD = 320.0f;

    CentipedeBoss(int sw, int sh, float hpInit) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.5f; worldY = sh * 0.4f;   // main 이 스폰 시 덮어씀
        heading = (float)(rand() % 628) * 0.01f;
        chargeCdTimer = CHARGE_INT * 0.7f;
        bigCd = BIG_INT * 0.5f;
        trail.assign(NSEG * SEG_STEP + 8, glm::vec2(worldX, worldY));
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
    // 속도 비례 쿨다운 배율: 빠를수록 < 1.0 (스킬이 더 잦아짐)
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

    // 가장 가까운 벽 방향으로 난동 시작
    void enterRamp() {
        state = 4; stateTimer = 0.0f; rampHits = 0; chargeTelegraph = false;
        float dl = worldX, dr = (float)screenW - worldX;
        float dtp = worldY, db = (float)screenH - worldY;
        float mn = dl; rampDX = -1.0f; rampDY = 0.0f;
        if (dr < mn)  { mn = dr;  rampDX = 1.0f;  rampDY = 0.0f; }
        if (dtp < mn) { mn = dtp; rampDX = 0.0f;  rampDY = -1.0f; }
        if (db < mn)  { mn = db;  rampDX = 0.0f;  rampDY = 1.0f; }
        heading = atan2f(rampDY, rampDX);
    }
    void backToWander() {
        state = 0; stateTimer = 0.0f;
        spitTimer = 0.0f; surging = false; surgeCd = 0.0f;
        chargePhase = 0; chargeTelegraph = false; chargeCdTimer = CHARGE_INT * 0.5f;
    }

    void Update(float px, float py, float dt, float& playerHP, std::vector<Bullet>& bullets) {
        if (!alive) return;

        // ── 탈피(Molt) — HP 임계마다 꼬리 마디 사출 → 지뢰 + 가속 ──
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
            }
        }

        // 배회 타이머는 조준·돌진 중 멈춤(딜 타임)
        if (state != 0 || chargePhase == 0)
            stateTimer += dt;

        if (state == 0) {            // ── 배회 + 라이트 견제 ──
            if (chargePhase == 1) {  // 직선 돌진 조준(멈춤)
                chargeTimer += dt;
                float d = atan2f(py - worldY, px - worldX);
                heading = d;
                if (chargeTimer >= CHARGE_WINDUP) {
                    chargePhase = 2; chargeTimer = 0.0f;
                    chargeDX = cosf(d); chargeDY = sinf(d);
                    chargeTelegraph = false;
                }
            } else if (chargePhase == 2) { // 직선 돌진
                chargeTimer += dt;
                worldX += chargeDX * CHARGE_SPD * dt;
                worldY += chargeDY * CHARGE_SPD * dt;
                heading = atan2f(chargeDY, chargeDX);
                if (chargeTimer >= CHARGE_DUR) {
                    chargePhase = 0; chargeTimer = 0.0f; chargeCdTimer = 0.0f;
                }
            } else {                 // 일반 배회
                wanderTimer += dt;
                if (wanderTimer >= TURN_INT) {
                    wanderTimer = 0.0f;
                    heading += ((rand() % 2) ? 1.0f : -1.0f) * 0.7f;
                }
                if (surging) {       // 데이터 폭주(나선)
                    surgeT += dt; surgeTick += dt; surgeAng += dt * 3.2f;
                    if (surgeTick >= SURGE_TICK) {
                        surgeTick = 0.0f;
                        for (int i = 0; i < 2; i++) {
                            float a = surgeAng + (float)i * 3.14159265f;
                            fireDir(bullets, cosf(a), sinf(a), SURGE_SPD, glm::vec3(0.7f, 1.0f, 0.4f));
                        }
                    }
                    if (surgeT >= SURGE_DUR) { surging = false; surgeCd = 0.0f; }
                } else {
                    spitTimer += dt;     // 데이터 토사(부채꼴)
                    if (spitTimer >= SPIT_INT * cdScale()) {
                        spitTimer = 0.0f;
                        float base = atan2f(py - worldY, px - worldX);
                        for (int i = 0; i < SPIT_N; i++) {
                            float a = base + ((float)i / (float)(SPIT_N - 1) - 0.5f) * 0.8f;
                            fireDir(bullets, cosf(a), sinf(a), SPIT_SPD, glm::vec3(0.6f, 1.0f, 0.5f));
                        }
                    }
                    surgeCd += dt;       // 폭주 발동
                    if (surgeCd >= SURGE_INT * cdScale() && chargePhase == 0) {
                        surging = true; surgeT = 0.0f; surgeTick = 0.0f;
                        surgeAng = (float)(rand() % 628) * 0.01f;
                    }
                }
                // 화면 경계에서 중앙으로 부드럽게 선회
                float toC = atan2f(screenH*0.5f - worldY, screenW*0.5f - worldX);
                float m = 120.0f;
                if (worldX < m || worldX > screenW - m || worldY < m || worldY > screenH - m) {
                    float d = toC - heading;
                    while (d >  3.14159265f) d -= 6.2831853f;
                    while (d < -3.14159265f) d += 6.2831853f;
                    heading += d * 2.0f * dt;
                }
                // 플레이어 직선 돌진 발동
                chargeCdTimer += dt;
                if (chargeCdTimer >= CHARGE_INT * cdScale()) {
                    chargePhase = 1; chargeTimer = 0.0f; chargeTelegraph = true;
                } else {
                    float ws = wanderSpeed();
                    worldX += cosf(heading) * ws * dt;
                    worldY += sinf(heading) * ws * dt;
                }
                // 대형 패턴 로테이션 발동 — 난동/똬리/장벽/잠복 (속도 비례 쿨감)
                bigCd += dt;
                if (bigCd >= BIG_INT * cdScale() && chargePhase == 0 && !surging) {
                    bigCd = 0.0f; chargeTelegraph = false;
                    switch (rand() % 4) {
                    case 0: enterRamp(); break;
                    case 1: {  // COIL — 플레이어를 중심으로 똬리
                        state = 5; stateTimer = 0.0f;
                        coilCX = px; coilCY = py;
                        float mg = COIL_R0 + 40.0f;
                        if (coilCX < mg) coilCX = mg; if (coilCX > screenW - mg) coilCX = screenW - mg;
                        if (coilCY < mg) coilCY = mg; if (coilCY > screenH - mg) coilCY = screenH - mg;
                        coilR = COIL_R0;
                        coilAng = atan2f(worldY - coilCY, worldX - coilCX);
                        break; }
                    case 2: {  // WALL — 가까운 쪽에서 먼 쪽으로 가로질러 펴기
                        state = 6; stateTimer = 0.0f; wallFireT = 0.0f;
                        wallDir = (worldX < screenW * 0.5f) ? 1 : -1;
                        heading = (wallDir > 0) ? 0.0f : 3.14159265f;
                        break; }
                    default: { // BURROW — 잠수 → 발밑 예고 → 솟구침
                        state = 7; stateTimer = 0.0f; burrowPhase = 0;
                        heading = atan2f(worldY - screenH*0.5f, worldX - screenW*0.5f);
                        wasInside = onScreen(worldX, worldY);
                        break; }
                    }
                }
                // 배회 시간 종료 → 화면 밖 이탈(곡선 돌진 진입)
                if (state == 0 && stateTimer >= WANDER_T) {
                    state = 1; stateTimer = 0.0f;
                    chargePhase = 0; chargeTelegraph = false;
                    heading = atan2f(worldY - screenH*0.5f, worldX - screenW*0.5f);
                    wasInside = onScreen(worldX, worldY);
                }
            }
        }
        else if (state == 1) {       // ── 화면밖 이탈(무적) ──
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
        else if (state == 2) {       // ── 경로 예고(화면밖, 무적) ──
            if (stateTimer >= TELEGRAPH) { state = 3; stateTimer = 0.0f; dashT = 0.0f; }
        }
        else if (state == 3) {       // ── 곡선 재진입 돌진(무적) ──
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
        else if (state == 4) {       // ── 벽 들이박기 난동(파편 + 진동) ──
            worldX += rampDX * RAMP_SPD * dt;
            worldY += rampDY * RAMP_SPD * dt;
            heading = atan2f(rampDY, rampDX);
            float mg = HEAD * 0.55f;
            bool hit = (worldX <= mg) || (worldX >= (float)screenW - mg) ||
                       (worldY <= mg) || (worldY >= (float)screenH - mg);
            if (hit) {
                if (worldX < mg) worldX = mg;
                if (worldX > (float)screenW - mg) worldX = (float)screenW - mg;
                if (worldY < mg) worldY = mg;
                if (worldY > (float)screenH - mg) worldY = (float)screenH - mg;
                for (int i = 0; i < RAMP_SHRAP; i++) {
                    float a = (float)i / (float)RAMP_SHRAP * 6.2831853f + (float)(rand()%100)*0.01f;
                    fireDir(bullets, cosf(a), sinf(a), RAMP_SHRAP_SPD, glm::vec3(0.95f, 0.9f, 0.3f));
                }
                shakePulse = true;          // 화면 진동(자해 제거 — 들이박아도 HP 안 깎임)
                ++rampHits;
                if (rampHits >= RAMP_HITS) {
                    backToWander(); bigCd = 0.0f;
                } else {                     // 다른 벽으로 재돌진
                    int rx = screenW - (int)(2.0f*mg); if (rx < 1) rx = 1;
                    int ry = screenH - (int)(2.0f*mg); if (ry < 1) ry = 1;
                    float tx = mg + (float)(rand()%rx), ty = mg + (float)(rand()%ry);
                    float dx = tx - worldX, dy = ty - worldY;
                    float d = std::sqrt(dx*dx+dy*dy) + 1e-3f;
                    rampDX = dx/d; rampDY = dy/d;
                }
            }
        }
        else if (state == 5) {       // ── 똬리 감기(A1): 플레이어 중심 링 + 조임 ──
            coilAng += COIL_SPIN * dt;
            coilR -= COIL_SHRINK * dt; if (coilR < COIL_RMIN) coilR = COIL_RMIN;
            worldX = coilCX + cosf(coilAng) * coilR;
            worldY = coilCY + sinf(coilAng) * coilR;
            heading = coilAng + 1.5707963f;       // 접선
            if (stateTimer >= COIL_DUR) { backToWander(); bigCd = 0.0f; }
        }
        else if (state == 6) {       // ── 장벽 분할(A3): 가로질러 펴며 견제 ──
            worldX += (float)wallDir * WALL_SPD * dt;
            heading = (wallDir > 0) ? 0.0f : 3.14159265f;
            wallFireT += dt;
            if (wallFireT >= WALL_FIRE) {
                wallFireT = 0.0f;
                float base = atan2f(py - worldY, px - worldX);
                fireDir(bullets, cosf(base), sinf(base), 330.0f, glm::vec3(0.5f, 1.0f, 0.5f));
            }
            float M = HEAD;
            if (worldX < -M || worldX > screenW + M) {
                backToWander(); bigCd = 0.0f;
                heading = atan2f(screenH*0.5f - worldY, screenW*0.5f - worldX);
            }
        }
        else {                       // ── state 7: 잠복(B4) ──
            if (burrowPhase == 0) {  // 잠수 — 화면 밖으로 빠르게
                worldX += cosf(heading) * DASH_SPD * dt;
                worldY += sinf(heading) * DASH_SPD * dt;
                bool inside = onScreen(worldX, worldY);
                if (wasInside && !inside) shakePulse = true;
                wasInside = inside;
                float M = 160.0f;
                if (worldX < -M || worldX > screenW + M || worldY < -M || worldY > screenH + M) {
                    burrowPhase = 1; stateTimer = 0.0f;
                    burrowX = px; burrowY = py;       // 발밑 타겟 고정
                    float m = HEAD;
                    if (burrowX < m) burrowX = m; if (burrowX > screenW - m) burrowX = screenW - m;
                    if (burrowY < m) burrowY = m; if (burrowY > screenH - m) burrowY = screenH - m;
                }
            } else if (burrowPhase == 1) { // 발밑 예고
                if (stateTimer >= BURROW_WARN) {
                    burrowPhase = 2; stateTimer = 0.0f;
                    worldX = burrowX; worldY = burrowY;
                    for (auto& p : trail) p = glm::vec2(worldX, worldY);
                    for (int i = 0; i < BURROW_SHRAP; i++) {
                        float a = (float)i / (float)BURROW_SHRAP * 6.2831853f + (float)(rand()%100)*0.01f;
                        fireDir(bullets, cosf(a), sinf(a), BURROW_SHRAP_SPD, glm::vec3(0.8f, 1.0f, 0.4f));
                    }
                    shakePulse = true;
                }
            } else {                       // 솟구침 정착 → 배회
                if (stateTimer >= 0.25f) { backToWander(); bigCd = 0.0f; }
            }
        }

        // ── 이동 사출 — 실제로 이동하는 상태에서 좌우로 데이터 탄을 흘림 ──
        //   "이동할 때마다 탄이 날아간다" — 배회/돌진/난동 중 진행 수직 양옆으로 누수.
        bool moving = (state == 0 && chargePhase != 1) || state == 3 || state == 4;
        if (moving && onScreen(worldX, worldY)) {
            shedTimer += dt;
            if (shedTimer >= SHED_INT * cdScale()) {
                shedTimer = 0.0f;
                float pxn = -sinf(heading), pyn = cosf(heading);
                fireDir(bullets,  pxn,  pyn, SHED_SPD, glm::vec3(0.5f, 1.0f, 0.45f));
                fireDir(bullets, -pxn, -pyn, SHED_SPD, glm::vec3(0.5f, 1.0f, 0.45f));
            }
        }
        // ── 탈피 지뢰 갱신 + 접촉 폭발 ──
        for (size_t i = 0; i < hazards.size(); ) {
            Hazard& h = hazards[i];
            h.life -= dt;
            float dx = px - h.x, dy = py - h.y;
            if ((dx*dx + dy*dy) < h.r * h.r) {   // 접촉 → 방사형 파편 폭발
                for (int k = 0; k < MOLT_SHRAP; k++) {
                    float a = (float)k / (float)MOLT_SHRAP * 6.2831853f;
                    fireFrom(bullets, h.x, h.y, cosf(a), sinf(a), MOLT_SHRAP_SPD, glm::vec3(1.0f, 0.6f, 0.2f));
                }
                h.life = 0.0f;
            }
            if (h.life <= 0.0f) hazards.erase(hazards.begin() + i);
            else ++i;
        }

        // 궤적 기록
        trail.insert(trail.begin(), glm::vec2(worldX, worldY));
        if ((int)trail.size() > NSEG * SEG_STEP + 8) trail.pop_back();

        // 접촉 데미지 (잠복 잠수/예고 중에는 본체가 없음 = 무접촉)
        bool intangible = (state == 7 && burrowPhase < 2);
        if (!intangible) {
            bool dashing = (state == 1 || state == 3 || state == 4 || state == 5 ||
                            state == 6 || chargePhase == 2 || (state == 7 && burrowPhase == 2));
            float hcr = HEAD * 0.78f;
            float hdx = px - worldX, hdy = py - worldY;
            if (hdx*hdx + hdy*hdy < hcr * hcr)
                playerHP -= (dashing ? 22.0f : 12.0f) * dt;
            for (int i = 1; i <= activeSeg; i++) {
                glm::vec2 s = segPos(i);
                float sr = segSize(i) + 4.0f;
                float sdx = px - s.x, sdy = py - s.y;
                if (sdx*sdx + sdy*sdy < sr * sr) playerHP -= 8.0f * dt;
            }
        }
    }

    glm::vec2 segPos(int i) const {
        int idx = i * SEG_STEP;
        if (idx >= (int)trail.size()) idx = (int)trail.size() - 1;
        if (idx < 0) idx = 0;
        return trail[idx];
    }

    // ── 월드 렌더 (보스 자체 렌더 — main.cpp 밖, 완전 분리). t = glfwGetTime() ──
    //   창 클리핑 없이 전체 화면에 직접 그림. 탄·창 HUD 는 main 공용 처리.
    void render(float t) const {
        BindMainShader();

        // (0) 탈피 지뢰 — 주황 마름모(접촉 폭발) — 본체 뒤에 깔기
        for (const auto& h : hazards) {
            float pul = 0.5f + 0.5f * sinf(t * 12.0f + h.x * 0.05f);
            drawCircle(h.x, h.y, h.r, 1.0f, 0.5f, 0.15f, 0.12f);
            drawDiamond(h.x, h.y, 22.0f + 6.0f * pul, 1.0f, 0.6f, 0.2f, 0.85f);
            drawDiamond(h.x, h.y, 10.0f, 1.0f, 0.9f, 0.4f, 1.0f);
        }

        // (0b) 잠복 발밑 예고 — 솟구침 직전 그림자 링
        if (state == 7 && burrowPhase == 1) {
            float p = stateTimer / BURROW_WARN;       // 0→1
            float blink = 0.5f + 0.5f * sinf(t * 24.0f);
            drawCircle(burrowX, burrowY, 30.0f + 80.0f * p, 0.6f, 0.15f, 0.15f, 0.18f + 0.2f * p);
            drawCircle(burrowX, burrowY, 14.0f + 30.0f * p, 1.0f, 0.3f, 0.2f, 0.3f + 0.4f * blink);
        }

        // (1) 곡선 돌진 예고선 (state==2)
        if (state == 2) {
            float blink = 0.55f + 0.45f * sinf(t * 22.0f);
            int n = 40;
            for (int i = 0; i <= n; i++) {
                glm::vec2 p = bezier((float)i / (float)n);
                drawCircle(p.x, p.y, 11.0f, 1.0f, 0.3f, 0.18f, 0.30f + 0.22f * blink);
            }
            int arrows = 7;
            for (int kk = 1; kk <= arrows; kk++) {
                float tt = (float)kk / (float)(arrows + 1);
                glm::vec2 p  = bezier(tt);
                glm::vec2 pf = bezier(tt + 0.02f);
                float ang = atan2f(pf.y - p.y, pf.x - p.x);
                float dxn = cosf(ang), dyn = sinf(ang), pxn = -dyn, pyn = dxn;
                const float L = 34.0f, W = 19.0f;
                float tx = p.x + dxn*L*0.6f, ty = p.y + dyn*L*0.6f;
                float b1x = p.x - dxn*L*0.4f + pxn*W, b1y = p.y - dyn*L*0.4f + pyn*W;
                float b2x = p.x - dxn*L*0.4f - pxn*W, b2y = p.y - dyn*L*0.4f - pyn*W;
                float v[6] = { tx,ty, b1x,b1y, b2x,b2y };
                BatchVerts(v, 3, 1.0f, 0.35f, 0.15f, 0.55f + 0.4f*blink);
            }
        }
        // (2) 플레이어 직선 돌진 조준선 (chargePhase==1)
        if (chargeTelegraph) {
            float blink = 0.5f + 0.5f * sinf(t * 18.0f);
            float dxn = cosf(heading), dyn = sinf(heading);
            for (int i = 1; i <= 12; i++) {
                float tt = (float)i / 12.0f;
                drawCircle(worldX + dxn*tt*420.0f, worldY + dyn*tt*420.0f,
                           8.0f + tt*4.0f, 1.0f, 0.25f, 0.15f, 0.35f + 0.45f*blink);
            }
        }

        // 잠복 잠수/예고 중엔 본체(머리+몸통)가 화면에 없음
        bool hidden = (state == 7 && burrowPhase < 2);
        if (hidden) { BatchFlush(); return; }

        bool dash = (state == 1 || state == 3 || state == 4 || state == 5 ||
                     state == 6 || chargePhase == 2);

        // 삼각형 채움 헬퍼
        auto tri = [&](float ax, float ay, float bx, float by, float cx, float cy,
                       float r, float g, float b, float a) {
            float v[6] = { ax,ay, bx,by, cx,cy };
            BatchVerts(v, 3, r, g, b, a);
        };

        // ── 몸통 — 줄줄이 네온 '프로세스 블록'(데이터 패킷 체인). Onedow OS 톤 ──
        for (int i = activeSeg; i >= 1; i--) {
            glm::vec2 s = segPos(i);
            float sz = segSize(i);
            float head01 = 1.0f - (float)(i - 1) / (float)(activeSeg > 1 ? activeSeg - 1 : 1);
            float br = 0.5f + 0.5f * head01;          // 머리에 가까울수록 밝게
            drawRect(s.x - sz, s.y - sz, sz*2.0f, sz*2.0f, 0.05f, 0.12f, 0.06f, 0.95f);   // 본문(어두움)
            drawNeonBorder(s.x - sz, s.y - sz, sz*2.0f, sz*2.0f, 0.20f*br+0.1f, 0.92f*br, 0.32f*br);
            drawRect(s.x - sz*0.32f, s.y - sz*0.32f, sz*0.64f, sz*0.64f,
                     0.40f, 0.95f*br, 0.42f, 1.0f);  // 코어 블록
        }

        // ── 머리 — 진행방향으로 뾰족한 화살촉(레이어드 녹색) + 맥동 코어 ──
        //   몸통과 같은 녹색 톤. 뒤 끝을 V로 파서 날카로운 '▸' 느낌.
        float pulse = dash ? 1.0f : 0.82f;
        float dxn = cosf(heading), dyn = sinf(heading), pxn = -dyn, pyn = dxn;
        float H = HEAD;
        // 화살촉 1겹 = 앞 꼭짓점 + 뒤 좌/우 + 뒤 중앙 노치(앞으로 당겨 V)
        auto arrow = [&](float fwd, float back, float side, float notch,
                         float r, float g, float b) {
            float tx  = worldX + dxn*fwd,            ty  = worldY + dyn*fwd;          // 앞 끝(뾰족)
            float l1x = worldX - dxn*back + pxn*side, l1y = worldY - dyn*back + pyn*side; // 뒤 좌
            float l2x = worldX - dxn*back - pxn*side, l2y = worldY - dyn*back - pyn*side; // 뒤 우
            float nx  = worldX - dxn*(back - notch),  ny  = worldY - dyn*(back - notch);  // 뒤 중앙 노치
            tri(tx, ty, l1x, l1y, nx, ny, r, g, b, 1.0f);
            tri(tx, ty, nx, ny, l2x, l2y, r, g, b, 1.0f);
        };
        float ext = dash ? 0.18f : 0.0f;                                    // 돌진 시 살짝 길어짐
        arrow(H*(1.45f+ext), H*0.78f, H*0.98f, H*0.42f, 0.08f, 0.30f, 0.10f);   // 외곽(어둠)
        arrow(H*(1.15f+ext), H*0.58f, H*0.70f, H*0.32f, 0.22f, 0.62f*pulse, 0.24f); // 중간
        arrow(H*(0.85f+ext), H*0.40f, H*0.46f, H*0.22f, 0.45f, 0.98f*pulse, 0.42f); // 밝은 갑각
        // 맥동 코어
        float cpul = 0.7f + 0.3f * sinf(t * 4.0f);
        drawCircle(worldX, worldY, H*0.22f, 0.10f, 0.20f, 0.10f, 1.0f);
        drawCircle(worldX, worldY, H*0.15f, 0.5f, 1.0f*cpul, 0.55f, 1.0f);
        drawCircle(worldX, worldY, H*0.07f, 0.95f, 1.0f, 0.9f, 1.0f);
        BatchFlush();
    }
};
