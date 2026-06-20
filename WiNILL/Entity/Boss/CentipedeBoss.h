#pragma once
#include <vector>
#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#include "Bullet.h"
#include "DrawPrim.h"   // 보스 자체 렌더(완전 분리) — drawRect/drawNeonBorder/drawDiamond 등

// ─────────────────────────────────────────────────────────────
// FORK.worm — 플라즈마 포크 체인 (체인형 보스)
//   네온 노드 + 에너지 케이블이 이어진 긴 보스. OS/가짜창 UI 비주얼 사용 안 함.
//   실루엣: 시안·마젠타 노드 체인 + 머리 3-prong 포크 글로우.
// ─────────────────────────────────────────────────────────────
class CentipedeBoss {
public:
    static constexpr const wchar_t* BOSS_NAME = L"FORK.worm";
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
    static constexpr float HEAD       = 62.0f;   // 머리 충돌 반경 (작고 날카롭게)
    static constexpr float SEG_NEAR   = 36.0f;   // 머리에 가장 가까운 세그먼트(반크기)
    static constexpr float SEG_FAR    = 15.0f;   // 꼬리 끝(가장 작음)
    static constexpr float WANDER_SPD = 440.0f;  // 기본 배회 속도 (정신 사납게 빠르게)
    static constexpr float WANDER_GAIN= 0.12f;   // 돌진 1회당 +12%
    static constexpr float WANDER_CAP = 2.6f;    // 속도 배율 상한
    static constexpr float DASH_SPD   = 1700.0f; // 돌진/이탈 속도
    static constexpr float WANDER_T   = 7.5f;    // 배회 시간(딜 타임, 더 공격적)
    static constexpr float TELEGRAPH  = 1.2f;    // 곡선 돌진 예고
    static constexpr float TURN_INT   = 0.38f;   // 지그재그 전환 주기(더 잦게 = 산만)

    // 데이터 토사(부채꼴)
    static constexpr float SPIT_INT   = 5.5f;
    static constexpr int   SPIT_N     = 5;
    static constexpr float SPIT_SPD   = 300.0f;
    float spitTimer = 0.0f;
    // 플레이어 직선 돌진
    static constexpr float CHARGE_INT    = 9.0f;
    static constexpr float CHARGE_WINDUP = 0.5f;
    static constexpr float CHARGE_DUR    = 0.65f;
    static constexpr float CHARGE_SPD    = 1500.0f;
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
    // 세그먼트 포격 — 머리→꼬리로 마디가 차례차례 플레이어에게 발사(화면 전체 수렴)
    static constexpr float CANNON_INT  = 9.5f;
    static constexpr float CANNON_STEP = 0.045f;  // 마디 간 발사 간격
    static constexpr float CANNON_SPD  = 360.0f;
    bool  cannonActive = false;
    float cannonCd = 0.0f, cannonT = 0.0f;
    int   cannonIdx = 0;
    // ── 자식 프로세스 — 작은 체인 adds(자체 관리) ──
    struct MiniBug { float x, y, heading, hp; bool alive; std::vector<glm::vec2> trail;
                     float wt; int lungeState; float lungeT; };
    std::vector<MiniBug> minis;
    static constexpr float SUMMON_INT  = 9.0f;
    static constexpr int   SUMMON_COUNT = 2;   // 한 번에 2마리
    static constexpr int   MINI_NSEG   = 5;    // 작은 체인: 머리 + 5 PID 칩
    static constexpr int   MINI_STEP   = 4;
    static constexpr float MINI_HEAD   = 26.0f;
    static constexpr float MINI_SPD    = 230.0f;
    static constexpr float MINI_HP0    = 288.0f; // = 기본 커널 프로세스(BRUTE 90*3.2) 체력
    static constexpr float MINI_WIN_W  = 300.0f; // adds 충돌·조준 반경용
    static constexpr float MINI_WIN_H  = 210.0f;
    static constexpr float MINI_WIN_TB = 14.0f;
    static constexpr wchar_t MINI_WIN_NAME[] = L"child.exe";
    float summonCd = 0.0f;

    // ── VFX (Gemini 가이드 — OS UI 없이 순수 이펙트) ──
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
        if ((int)ghosts.size() >= MAX_GHOST)
            ghosts.erase(ghosts.begin());
        GhostEcho g;
        g.x = x; g.y = y; g.r = r; g.maxLife = g.life = 0.5f;
        ghosts.push_back(g);
    }

    void tickVfx(float dt) {
        for (size_t i = 0; i < segFlashes.size(); ) {
            segFlashes[i].t -= dt;
            if (segFlashes[i].t <= 0.0f) segFlashes.erase(segFlashes.begin() + i);
            else ++i;
        }
        for (size_t i = 0; i < ghosts.size(); ) {
            ghosts[i].life -= dt;
            if (ghosts[i].life <= 0.0f) ghosts.erase(ghosts.begin() + i);
            else ++i;
        }
        for (size_t i = 0; i < hitSparks.size(); ) {
            HitSpark& sp = hitSparks[i];
            sp.life -= dt;
            sp.x += sp.vx * dt; sp.y += sp.vy * dt;
            sp.vx *= 0.90f; sp.vy *= 0.90f;
            if (sp.life <= 0.0f) hitSparks.erase(hitSparks.begin() + i);
            else ++i;
        }
        if (glitchOverlay > 0.0f) glitchOverlay -= dt;
    }

    // 강제 종료 X — 지직거리는 글리치 아이콘
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

    // ── 플라즈마 노드 / 케이블 (공용 드로우) ──
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

    // 자식 adds — 축소 플라즈마 체인 (가짜창 없음)
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
    // 스킬 시전 진동(가벼운 피드백, 눈뽕 X) — main 이 읽고 적용
    float wantShake = 0.0f;
    void shake(float m) { if (m > wantShake) wantShake = m; }
    // ── 크래시 벽 — HP 깎일 때마다 화면 가로지르는 직선 차단(대시로만 통과) ──
    //   시간으로 안 사라짐. 셀 단위로 총 맞으면 그 부분만 뚫림(이동 통로 확보).
    struct WallCell { /* per-cell hp; 0=뚫림 */ };
    struct LineWall {
        bool  horiz;                 // true=가로(고정 y) / false=세로(고정 x)
        float coord;                 // 고정 좌표(가로면 y, 세로면 x)
        float spawnT;                // 등장 애니메이션 잔여(사체가 꿈틀하다 굳음)
        std::vector<int> cellHp;     // 셀별 HP (0 = 뚫림)
    };
    std::vector<LineWall> walls;
    static constexpr float WALL_CELL  = 60.0f;   // 셀 크기(px)
    static constexpr float WALL_THICK = 30.0f;   // 차단 두께(직선 폭)
    static constexpr int   WALL_CELLHP = 10;     // 셀 뚫는 데 필요한 피격 수(대폭 버프)
    static constexpr int   WALL_MAX   = 3;        // 동시 상한(전체 직선이라 적게)
    static constexpr float WALL_STEP  = 0.08f;    // maxHp 8% 깎일 때마다 1개
    static constexpr float WALL_ANIM  = 0.7f;     // 등장 애니메이션 길이
    float lastWallHp = -1.0f;

    // ── 대형 패턴 로테이션(난동/똬리/장벽/잠복) ──
    static constexpr float BIG_INT = 6.0f;       // 대형 패턴 쿨다운(속도 비례 감소)
    float bigCd = 0.0f;
    // 벽 박기 광란 — 고속으로 벽 사이를 튕기며 속도 비례 탄을 뿜고, 박을수록 감속
    static constexpr float RAMP_SPD0      = 1750.0f; // 초기 속도(폭발적)
    static constexpr float RAMP_MIN       = 470.0f;  // 이 속도 밑이면 종료
    static constexpr float RAMP_DECAY     = 0.80f;   // 벽 충돌마다 속도 ×0.80
    static constexpr float RAMP_FIRE_SPD  = 340.0f;  // 분출 탄 속도
    static constexpr float RAMP_FIRE_K    = 135.0f;  // 분출 간격 = K/속도 (빠를수록 자주)
    static constexpr int   RAMP_SHRAP     = 10;      // 벽 충돌 방사 파편
    static constexpr float RAMP_SHRAP_SPD = 360.0f;
    float rampSpeed = 0.0f, rampFireTimer = 0.0f;
    float rampDX = 0.0f, rampDY = 0.0f;
    // 똬리 감기(A1) — 순간이동 X(현재 위치에서 조여듦), 오래 감음
    static constexpr float COIL_DUR    = 4.6f;   // 도는 시간 ↑(안에 있으면 위험)
    static constexpr float COIL_SPIN   = 4.4f;   // rad/s (더 많이 감김)
    static constexpr float COIL_R0     = 300.0f; // 목표 시작 반경(접근 후)
    static constexpr float COIL_RMIN   = 115.0f; // 더 조여듦
    static constexpr float COIL_SHRINK = 42.0f;  // px/s (천천히 조임 → 오래 위협)
    float coilCX = 0, coilCY = 0, coilR = 0, coilAng = 0;
    // 장벽 분할(A3)
    static constexpr float WALL_SPD  = 780.0f;
    static constexpr float WALL_FIRE = 0.16f;
    int   wallDir = 1;
    float wallFireT = 0.0f;
    // 잠복(B4)
    static constexpr float BURROW_WARN      = 0.75f;
    static constexpr float BURROW_SINK      = 0.45f;   // 제자리 가라앉는 시간
    static constexpr int   BURROW_SHRAP     = 18;
    static constexpr float BURROW_SHRAP_SPD = 380.0f;
    int   burrowPhase = 0;       // 0=제자리 가라앉기 / 1=발밑 예고(잠복) / 2=솟구침
    float burrowX = 0, burrowY = 0;
    float burrowScale = 1.0f;    // 가라앉기/솟구침 스케일(1=정상, 0=완전 잠복)

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

    // 벽 박기 광란 시작 — 임의 대각 방향으로 폭발적으로 튕기기 시작
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

    // 등장 모션 — '사라졌다가 돌진' 기술 재활용: 화면 밖 경로 예고 → 곡선 돌진으로 입장
    void enterSpawn() {
        pickDash();
        state = 2; stateTimer = 0.0f;
        worldX = dashFromX; worldY = dashFromY;
        for (auto& p : trail) p = glm::vec2(worldX, worldY);
    }

    // 총알 선분이 벽 직선을 가로지르면 해당 셀 HP 감소(그 부분만 뚫림).
    //   살아있는 셀에 맞았으면 true 반환(일반탄은 main 에서 소멸시킴 = 관통 안 됨).
    bool hitWall(float x0, float y0, float x1, float y1) {
        bool hit = false;
        for (auto& w : walls) {
            if (w.horiz) {
                bool crossed = (y0 - w.coord) * (y1 - w.coord) <= 0.0f && fabsf(y1 - y0) > 1e-4f;
                float cx;
                if (crossed) { float tt = (w.coord - y0) / (y1 - y0); cx = x0 + (x1 - x0) * tt; }
                else if (fabsf((y0 + y1) * 0.5f - w.coord) < WALL_THICK * 0.5f) cx = (x0 + x1) * 0.5f; // 평행 근접(좌우 관통 차단)
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
    // 살아있는 벽 셀이 플레이어 이동을 막음(직선 밖으로 밀어냄). 대시(무적)는 main 에서 제외.
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
        const glm::vec3 BUL(0.95f, 0.35f, 0.95f);   // 보스 탄 색상 — 한 가지(마젠타)로 통일

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
                moltGlitchPulse = true;
                glitchOverlay = 0.12f;
                spawnMini(tail.x, tail.y, px, py);
            }
        }

        // ── 스킬 쿨다운(상태 무관 누적 → 신뢰성). 트리거는 여기서 일괄 ──
        cannonCd += dt;
        if (!cannonActive && cannonCd >= CANNON_INT * cdScale() &&
            state == 0 && chargePhase == 0 && !surging) {
            cannonCd = 0.0f; cannonActive = true; cannonIdx = 0; cannonT = 0.0f;
            shake(7.0f);
        }
        summonCd += dt;
        if (summonCd >= SUMMON_INT * cdScale() && state != 2) {   // 상한 없음 — 일정 시간마다 계속
            summonCd = 0.0f;
            for (int i = 0; i < SUMMON_COUNT; i++) {
                float ex, ey;                                     // 화면 끝(가장자리)에서만 생성
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
        // ── 새끼 버그(작은 지네형 보스) 갱신 — 지그재그 위빙 + 주기적 런지(돌진) 패턴 ──
        for (auto& mb : minis) {
            if (!mb.alive) continue;
            float dx = px - mb.x, dy = py - mb.y, d = std::sqrt(dx*dx + dy*dy) + 1e-3f;
            float toP = atan2f(dy, dx);
            mb.wt += dt; mb.lungeT += dt;
            float spd = MINI_SPD;
            if (mb.lungeState == 0) {                          // 위빙 접근(지그재그)
                mb.heading = toP + sinf(mb.wt * 6.0f) * 0.6f;
                if (mb.lungeT >= 2.4f && d < 430.0f) { mb.lungeState = 1; mb.lungeT = 0.0f; mb.heading = toP; }
            } else {                                            // 짧은 런지(돌진)
                spd = MINI_SPD * 2.7f;
                if (mb.lungeT >= 0.4f) { mb.lungeState = 0; mb.lungeT = 0.0f; }
            }
            mb.x += cosf(mb.heading) * spd * dt;
            mb.y += sinf(mb.heading) * spd * dt;
            mb.trail.insert(mb.trail.begin(), glm::vec2(mb.x, mb.y));
            if ((int)mb.trail.size() > MINI_NSEG*MINI_STEP + 2) mb.trail.pop_back();
            if (d < MINI_HEAD + 14.0f) playerHP -= 6.0f * dt;
        }
        // 새끼끼리 겹침 방지 — 소프트 콜리전(서로 밀어냄)
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
        for (size_t i = 0; i < minis.size(); ) {
            if (!minis[i].alive) minis.erase(minis.begin() + i); else ++i;
        }

        // ── 죽은 지네 벽 — 피 깎인 누적량마다 화면 가로지르는 직선 차단벽 생성 ──
        if (lastWallHp < 0.0f) lastWallHp = hp;             // 첫 프레임 기준
        float step = maxHp * WALL_STEP;
        while (hp <= lastWallHp - step) {
            lastWallHp -= step;
            LineWall lw;
            lw.horiz  = (rand() % 2) == 0;
            lw.spawnT = WALL_ANIM;
            int span  = lw.horiz ? screenH : screenW;       // 고정좌표 축 범위
            int along = lw.horiz ? screenW : screenH;       // 직선이 뻗는 축 범위
            lw.coord  = 90.0f + (float)(rand() % (span > 180 ? span - 180 : 1));
            int nc = along / (int)WALL_CELL + 1;
            lw.cellHp.assign(nc, WALL_CELLHP);
            // 플레이어 위치 셀은 미리 뚫어 둠(즉시 가둠 방지)
            float pAlong = lw.horiz ? px : py;
            int pc = (int)(pAlong / WALL_CELL);
            for (int k = pc - 1; k <= pc + 1; k++)
                if (k >= 0 && k < nc) lw.cellHp[k] = 0;
            walls.push_back(lw);   // 상한 없음 — 계속 누적(시간 소멸 X, 총으로만 뚫림)
            shake(8.0f);
        }
        for (auto& w : walls) if (w.spawnT > 0.0f) w.spawnT -= dt;          // 등장 애니메이션만(시간소멸 X)

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
                    shake(9.0f);                 // 돌진 개시 — 묵직한 진동
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
                            fireDir(bullets, cosf(a), sinf(a), SURGE_SPD, BUL);
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
                            fireDir(bullets, cosf(a), sinf(a), SPIT_SPD, BUL);
                        }
                    }
                    surgeCd += dt;       // 폭주 발동
                    if (surgeCd >= SURGE_INT * cdScale() && chargePhase == 0) {
                        surging = true; surgeT = 0.0f; surgeTick = 0.0f;
                        surgeAng = (float)(rand() % 628) * 0.01f;
                        shake(6.0f);
                    }
                }
                // ── 세그먼트 포격 진행(트리거는 상단) — 마디 머리→꼬리 차례 발사 ──
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
                    shake(8.0f);                 // 대형 패턴 개시 — 진동
                    switch (rand() % 4) {
                    case 0: enterRamp(); break;
                    case 1: {  // COIL — 현재 위치에서 그대로 감기 시작(순간이동 X)
                        state = 5; stateTimer = 0.0f;
                        coilCX = px; coilCY = py;
                        float mg = COIL_RMIN + 40.0f;
                        if (coilCX < mg) coilCX = mg; if (coilCX > screenW - mg) coilCX = screenW - mg;
                        if (coilCY < mg) coilCY = mg; if (coilCY > screenH - mg) coilCY = screenH - mg;
                        float ddx = worldX - coilCX, ddy = worldY - coilCY;
                        coilR = std::sqrt(ddx*ddx + ddy*ddy);   // 클램프 없이 현재 거리 = 점프 0
                        coilAng = atan2f(ddy, ddx);
                        break; }
                    case 2: {  // WALL — 가까운 쪽에서 먼 쪽으로 가로질러 펴기
                        state = 6; stateTimer = 0.0f; wallFireT = 0.0f;
                        wallDir = (worldX < screenW * 0.5f) ? 1 : -1;
                        heading = (wallDir > 0) ? 0.0f : 3.14159265f;
                        break; }
                    default: { // BURROW — 제자리 잠수 → 발밑 예고 → 솟구침
                        state = 7; stateTimer = 0.0f; burrowPhase = 0; burrowScale = 1.0f;
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
        else if (state == 4) {       // ── 벽 박기 광란: 고속 튕기기 + 속도비례 분출 + 박을수록 감속 ──
            worldX += rampDX * rampSpeed * dt;
            worldY += rampDY * rampSpeed * dt;
            heading = atan2f(rampDY, rampDX);
            // 이동속도 비례 탄 분출 — 빠를수록 자주(많이) 좌우로 뿜음
            rampFireTimer += dt;
            float fi = RAMP_FIRE_K / rampSpeed;
            if (rampFireTimer >= fi) {
                rampFireTimer = 0.0f;
                float pxn = -rampDY, pyn = rampDX;
                fireDir(bullets,  pxn,  pyn, RAMP_FIRE_SPD, BUL);
                fireDir(bullets, -pxn, -pyn, RAMP_FIRE_SPD, BUL);
            }
            // 벽 충돌 → 반사 + 감속(박을수록 느려짐) + 방사 파편
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
                rampSpeed *= RAMP_DECAY;     // 박을수록 감속
                for (int i = 0; i < RAMP_SHRAP; i++) {
                    float a = (float)i / (float)RAMP_SHRAP * 6.2831853f + (float)(rand()%100)*0.01f;
                    fireDir(bullets, cosf(a), sinf(a), RAMP_SHRAP_SPD, BUL);
                }
                shakePulse = true;
                spawnMini(worldX, worldY, px, py);   // 벽 박을 때마다 새끼 지네 추가 드롭
                if (rampSpeed < RAMP_MIN) { backToWander(); bigCd = 0.0f; }   // 다 느려지면 종료
            }
        }
        else if (state == 5) {       // ── 똬리 감기(A1): 플레이어 중심 링 + 조임 ──
            coilAng += COIL_SPIN * dt;
            coilR += (COIL_RMIN - coilR) * 1.3f * dt;   // 어디서 시작하든 RMIN 로 부드럽게 수렴(점프 X)
            worldX = coilCX + cosf(coilAng) * coilR;
            worldY = coilCY + sinf(coilAng) * coilR;
            heading = coilAng + 1.5707963f;       // 접선
            // 플레이어가 링 밖으로 빠져나가면 똬리 중단 → 배회 (갇힌 사람만 위협)
            float edx = px - coilCX, edy = py - coilCY;
            bool escaped = (edx*edx + edy*edy) > (coilR + 70.0f) * (coilR + 70.0f);
            if (escaped || stateTimer >= COIL_DUR) { backToWander(); bigCd = 0.0f; }
        }
        else if (state == 6) {       // ── 장벽 분할(A3): 가로질러 펴며 견제 ──
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
        else {                       // ── state 7: 잠복(B4) — 제자리에서 가라앉음 ──
            if (burrowPhase == 0) {  // 제자리 가라앉기(축소) — 날아가서 먼저 사라지지 않음
                burrowScale -= dt / BURROW_SINK;
                if (burrowScale <= 0.0f) {
                    burrowScale = 0.0f; burrowPhase = 1; stateTimer = 0.0f;
                    burrowX = px; burrowY = py;       // 발밑 타겟 고정
                    float m = HEAD;
                    if (burrowX < m) burrowX = m; if (burrowX > screenW - m) burrowX = screenW - m;
                    if (burrowY < m) burrowY = m; if (burrowY > screenH - m) burrowY = screenH - m;
                }
            } else if (burrowPhase == 1) { // 발밑 예고(완전 잠복)
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
            } else {                       // 솟구침(제자리 확대) → 배회
                burrowScale += dt / 0.25f;
                if (burrowScale >= 1.0f) { burrowScale = 1.0f; backToWander(); bigCd = 0.0f; }
            }
        }

        // ── 이동 사출 — 실제로 이동하는 상태에서 좌우로 데이터 탄을 흘림 ──
        //   "이동할 때마다 탄이 날아간다" — 배회/돌진/난동 중 진행 수직 양옆으로 누수.
        bool moving = (state == 0 && chargePhase != 1) || state == 3;   // state4(광란)은 자체 분출
        if (moving && onScreen(worldX, worldY)) {
            shedTimer += dt;
            if (shedTimer >= SHED_INT * cdScale()) {
                shedTimer = 0.0f;
                float pxn = -sinf(heading), pyn = cosf(heading);
                fireDir(bullets,  pxn,  pyn, SHED_SPD, BUL);
                fireDir(bullets, -pxn, -pyn, SHED_SPD, BUL);
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
                    fireFrom(bullets, h.x, h.y, cosf(a), sinf(a), MOLT_SHRAP_SPD, BUL);
                }
                h.life = 0.0f;
            }
            if (h.life <= 0.0f) hazards.erase(hazards.begin() + i);
            else ++i;
        }

        // ── 메모리 누수 잔상 + 꼬리 포크배출 + VFX 틱 ──
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
        for (size_t i = 0; i < errorNodes.size(); ) {
            ErrorNode& en = errorNodes[i];
            if (!en.alive) { errorNodes.erase(errorNodes.begin() + i); continue; }
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
            if (!en.alive) errorNodes.erase(errorNodes.begin() + i);
            else ++i;
        }
        tickVfx(dt);

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

    // ── FX 렌더 (창 클리핑 X) — 잔상/에러노드/조준선/벽/함정 ──
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

        // (00) 크래시 벽 — 네온 샤드가 직선으로 굳음
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

        // (0) 불안정 코어 — 접촉 폭발 (주황 플라즈마 구)
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

        // (0c) child adds 는 centiPass 에서 drawMini 로 렌더
        // (0b) 잠복 발밑 예고 — 솟구침 직전 그림자 링
        if (state == 7 && burrowPhase == 1) {
            float p = stateTimer / BURROW_WARN;
            float blink = 0.5f + 0.5f * sinf(t * 24.0f);
            drawCircle(burrowX, burrowY, 30.0f + 80.0f * p, 0.6f, 0.15f, 0.15f, 0.18f + 0.2f * p);
            drawCircle(burrowX, burrowY, 14.0f + 30.0f * p, 1.0f, 0.3f, 0.2f, 0.3f + 0.4f * blink);
        }
        // (1) 곡선 돌진 예고 — 플라즈마 궤적
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
        // (2) 직선 돌진 조준선
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

    // ── 본체 — 플라즈마 노드 체인 + 포크 코어 ──
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
