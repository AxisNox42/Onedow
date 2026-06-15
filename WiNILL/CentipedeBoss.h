#pragma once
#include <vector>
#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#include "Bullet.h"

// ─────────────────────────────────────────────────────────────
// BUG.proc — 버그 (지네형 보스)
//   큰 머리 + 점점 작아지는 꼬리 세그먼트가 따라오는 지네. 평소엔 맵을
//   지그재그로 배회하며 압박(이때 피격 가능 = 딜 타임).
//   주기적 '곡선 돌진':
//     ① 멈추지 않고 곧바로 화면 밖으로 빠져나감(나갈 때 화면 진동)
//     ② 사라진 뒤에야 돌진 경로(곡선)를 예고선으로 표시
//     ③ 반대편에서 곡선으로 고속 재진입 돌진(대각선 가능, 이때 무적)
//   4번째 패턴 — 플레이어 돌진: 잠깐 멈춰 조준(텔레그래프) → 고속 직선 돌진
//   돌진을 반복할수록 기본 배회 속도가 점점 빨라진다(누적 압박).
// ─────────────────────────────────────────────────────────────
class CentipedeBoss {
public:
    float worldX, worldY;          // 머리 위치
    float hp, maxHp;
    bool  alive = true;
    bool  exploded = false;
    int   screenW, screenH;
    // 프로토타입: 순수 보스전 — 잡몹 흡수 HP + 보통 피해 감소(피감 너무 높으면 손맛 죽음)
    float dmgTakenMult = 0.85f;    // 받는 피해 ×0.85 (15% 감소)

    // 상태: 0=배회(피격가능) / 1=화면밖 이탈 / 2=경로 예고(화면밖) / 3=곡선 재진입 돌진
    int   state      = 0;
    float stateTimer = 0.0f;
    float wanderTimer = 0.0f;
    float heading    = 0.0f;       // 배회/이탈 진행 방향(rad)

    // 돌진 곡선 경로 (2차 베지어: From → Ctrl → To). 렌더가 예고선으로 읽음.
    float dashFromX = 0, dashFromY = 0, dashToX = 0, dashToY = 0;
    float dashCtrlX = 0, dashCtrlY = 0;
    float dashT = 0.0f;

    bool  wasInside  = true;       // 직전 프레임 머리가 화면 안이었는지
    bool  shakePulse = false;      // 화면 밖으로 나가는 순간 1회 — main 이 읽고 끔
    int   dashCount  = 0;          // 완료한 돌진 횟수 (배회 속도 가속용)

    std::vector<glm::vec2> trail;  // 머리 궤적(세그먼트 추종용)

    static constexpr int   NSEG       = 9;       // 꼬리 세그먼트 수
    static constexpr int   SEG_STEP   = 6;       // 세그먼트 간 궤적 인덱스 간격
    static constexpr float HEAD       = 88.0f * 2.0f / 3.0f;   // 머리 크기 (기존 대비 2/3)
    static constexpr float SEG_NEAR   = 55.0f;   // 머리에 가장 가까운 세그먼트
    static constexpr float SEG_FAR    = 22.0f;   // 꼬리 끝 세그먼트(가장 작음)
    static constexpr float WANDER_SPD = 230.0f;  // 기본 배회 속도(돌진마다 가속)
    static constexpr float WANDER_GAIN= 0.14f;   // 돌진 1회당 배회 속도 +14%
    static constexpr float WANDER_CAP = 2.4f;    // 배회 속도 배율 상한(×2.4)
    static constexpr float DASH_SPD   = 1500.0f; // 돌진/이탈 속도
    static constexpr float WANDER_T   = 10.0f;   // 배회 시간(딜 타임) — 피격 가능 구간
    static constexpr float TELEGRAPH  = 1.2f;    // 곡선 돌진 경로 예고(사라진 뒤)
    static constexpr float TURN_INT   = 0.5f;    // 지그재그 방향 전환 주기
    // 2번째 패턴 — 데이터 토사(원거리 견제): 배회 중 주기적으로 플레이어에 부채꼴 토사
    static constexpr float SPIT_INT   = 5.5f;    // 토사 주기 (딜 타임 방해 ↓)
    static constexpr int   SPIT_N     = 5;       // 부채꼴 탄 수
    static constexpr float SPIT_SPD   = 300.0f;  // 토사 탄 속도
    float spitTimer = 0.0f;
    // 플레이어 돌진 — 배회 중 가끔 멈춰 조준 후 직선 돌진 (쿨 길게 → 딜 타임 확보)
    static constexpr float CHARGE_INT    = 12.0f;   // 돌진 쿨다운
    static constexpr float CHARGE_WINDUP = 0.55f;   // 조준(멈춤) 시간
    static constexpr float CHARGE_DUR    = 0.65f;   // 돌진 지속
    static constexpr float CHARGE_SPD    = 1280.0f; // 돌진 속도
    int   chargePhase = 0;       // 0=쿨다운/배회 / 1=조준 / 2=돌진
    float chargeCdTimer = 0.0f;  // 다음 돌진까지
    float chargeTimer = 0.0f;    // 조준·돌진 경과
    float chargeDX = 0.0f, chargeDY = 0.0f;
    bool  chargeTelegraph = false; // 조준 중 — main 이 예고선 렌더
    // 신규 스킬 — 데이터 폭주(나선 살포): 배회 중 가끔 머리가 회전하며 2갈래 나선 탄막
    static constexpr float SURGE_INT  = 8.5f;    // 폭주 주기
    static constexpr float SURGE_DUR  = 1.4f;    // 폭주 지속
    static constexpr float SURGE_TICK = 0.09f;   // 발사 간격
    static constexpr float SURGE_SPD  = 280.0f;
    float surgeCd = 0.0f, surgeT = 0.0f, surgeTick = 0.0f, surgeAng = 0.0f;
    bool  surging = false;
    // 신규 패턴 — 벽 들이박기 난동: 벽에 연속으로 박으며 파편 살포 + 자해 + 화면 진동
    static constexpr float RAMP_INT       = 13.0f;   // 난동 쿨다운
    static constexpr int   RAMP_HITS      = 4;       // 들이박는 횟수
    static constexpr float RAMP_SPD       = 1400.0f; // 들이박기 속도
    static constexpr int   RAMP_SHRAP     = 12;      // 충돌 파편 수
    static constexpr float RAMP_SHRAP_SPD = 360.0f;
    float rampCd = 0.0f;
    int   rampHits = 0;
    float rampDX = 0.0f, rampDY = 0.0f;

    CentipedeBoss(int sw, int sh, float hpInit) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.5f; worldY = sh * 0.4f;   // main 이 스폰 시 덮어씀
        heading = (float)(rand() % 628) * 0.01f;
        chargeCdTimer = CHARGE_INT * 0.7f;   // 스폰 직후·재진입 직후 바로 돌진 X
        trail.assign(NSEG * SEG_STEP + 8, glm::vec2(worldX, worldY));
    }

    bool vulnerable() const { return state == 0 || state == 4; }   // 배회·난동 중 피격 가능

    static float segSize(int i) {
        float t = (NSEG > 1) ? (float)(i - 1) / (float)(NSEG - 1) : 0.0f;
        return SEG_NEAR + (SEG_FAR - SEG_NEAR) * t;
    }
    float wanderSpeed() const {
        float mul = 1.0f + WANDER_GAIN * (float)dashCount;
        if (mul > WANDER_CAP) mul = WANDER_CAP;
        return WANDER_SPD * mul;
    }
    bool onScreen(float x, float y) const {
        return x >= 0.0f && x <= (float)screenW && y >= 0.0f && y <= (float)screenH;
    }

    // 2차 베지어 위치
    glm::vec2 bezier(float t) const {
        float u = 1.0f - t;
        float bx = u*u*dashFromX + 2.0f*u*t*dashCtrlX + t*t*dashToX;
        float by = u*u*dashFromY + 2.0f*u*t*dashCtrlY + t*t*dashToY;
        return glm::vec2(bx, by);
    }

    // 2차 베지어 접선(진행 방향)
    glm::vec2 bezierTangent(float t) const {
        float u = 1.0f - t;
        float tx = 2.0f*u*(dashCtrlX - dashFromX) + 2.0f*t*(dashToX - dashCtrlX);
        float ty = 2.0f*u*(dashCtrlY - dashFromY) + 2.0f*t*(dashToY - dashCtrlY);
        return glm::vec2(tx, ty);
    }

    // 재진입 곡선 경로 결정 — 대각선 가능 + 곡선(컨트롤점 측면 오프셋)
    void pickDash() {
        float cx = screenW * 0.5f, cy = screenH * 0.5f;
        float R  = std::sqrt(cx*cx + cy*cy) + 140.0f;   // 화면 모서리 밖
        float a0 = (float)(rand() % 628) * 0.01f;       // 진입 각
        float a1 = a0 + 3.14159265f + ((float)(rand()%120 - 60)) * 0.01f; // 대략 반대(대각 가능)
        dashFromX = cx + cosf(a0) * R;  dashFromY = cy + sinf(a0) * R;
        dashToX   = cx + cosf(a1) * R;  dashToY   = cy + sinf(a1) * R;
        // 컨트롤점 = 중점 + 경로 수직 방향으로 곡률 (좌/우 랜덤)
        float mx = (dashFromX + dashToX) * 0.5f, my = (dashFromY + dashToY) * 0.5f;
        float dx = dashToX - dashFromX, dy = dashToY - dashFromY;
        float len = std::sqrt(dx*dx + dy*dy) + 1e-3f;
        float pxn = -dy / len, pyn = dx / len;          // 수직 단위벡터
        float curve = (0.35f + (rand()%50)*0.006f) * len * ((rand()%2) ? 1.0f : -1.0f);
        dashCtrlX = mx + pxn * curve;  dashCtrlY = my + pyn * curve;
    }

    void fireDir(std::vector<Bullet>& b, float dx, float dy, float sp, glm::vec3 col) {
        Bullet bb(worldX, worldY, worldX + dx * 100.0f, worldY + dy * 100.0f);
        bb.isEnemy = true; bb.speed = sp; bb.color = col;
        b.push_back(bb);
    }

    void Update(float px, float py, float dt, float& playerHP, std::vector<Bullet>& bullets) {
        if (!alive) return;
        // 배회 시간은 돌진 조준·돌진 중에는 멈춤 (딜 타임 보장)
        if (state != 0 || chargePhase == 0)
            stateTimer += dt;

        if (state == 0) {            // ── 배회(지그재그) + 데이터 토사 + 플레이어 돌진 ──
            if (chargePhase == 1) {  // 조준(멈춤) — 플레이어 방향 고정
                chargeTimer += dt;
                float d = atan2f(py - worldY, px - worldX);
                heading = d;
                if (chargeTimer >= CHARGE_WINDUP) {
                    chargePhase = 2;
                    chargeTimer = 0.0f;
                    chargeDX = cosf(d);
                    chargeDY = sinf(d);
                    chargeTelegraph = false;
                }
            } else if (chargePhase == 2) { // 직선 돌진
                chargeTimer += dt;
                worldX += chargeDX * CHARGE_SPD * dt;
                worldY += chargeDY * CHARGE_SPD * dt;
                heading = atan2f(chargeDY, chargeDX);
                if (chargeTimer >= CHARGE_DUR) {
                    chargePhase = 0;
                    chargeTimer = 0.0f;
                    chargeCdTimer = 0.0f;
                }
            } else {                 // 일반 배회
                wanderTimer += dt;
                if (wanderTimer >= TURN_INT) {
                    wanderTimer = 0.0f;
                    heading += ((rand() % 2) ? 1.0f : -1.0f) * 0.7f;
                }
                // 데이터 폭주(나선 살포) — 폭주 중엔 토사 안 함(패턴 겹침 방지). 회전하며 2갈래 나선.
                if (surging) {
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
                    // 데이터 토사 — 주기적으로 플레이어 향해 부채꼴 탄 (돌진 사이 원거리 압박)
                    spitTimer += dt;
                    if (spitTimer >= SPIT_INT) {
                        spitTimer = 0.0f;
                        float base = atan2f(py - worldY, px - worldX);
                        for (int i = 0; i < SPIT_N; i++) {
                            float a = base + ((float)i / (float)(SPIT_N - 1) - 0.5f) * 0.8f;
                            fireDir(bullets, cosf(a), sinf(a), SPIT_SPD, glm::vec3(0.6f, 1.0f, 0.5f));
                        }
                    }
                    // 폭주 발동 — 돌진(charge) 중이 아닐 때만 (패턴 관리: 동시 발동 방지)
                    surgeCd += dt;
                    if (surgeCd >= SURGE_INT && chargePhase == 0) {
                        surging = true; surgeT = 0.0f; surgeTick = 0.0f;
                        surgeAng = (float)(rand() % 628) * 0.01f;
                    }
                }
                float toC = atan2f(screenH*0.5f - worldY, screenW*0.5f - worldX);
                float m = 120.0f;
                if (worldX < m || worldX > screenW - m || worldY < m || worldY > screenH - m) {
                    float d = toC - heading;
                    while (d >  3.14159265f) d -= 6.2831853f;
                    while (d < -3.14159265f) d += 6.2831853f;
                    heading += d * 2.0f * dt;
                }
                chargeCdTimer += dt;
                if (chargeCdTimer >= CHARGE_INT) {
                    chargePhase = 1;
                    chargeTimer = 0.0f;
                    chargeTelegraph = true;
                } else {
                    float ws = wanderSpeed();
                    worldX += cosf(heading) * ws * dt;
                    worldY += sinf(heading) * ws * dt;
                }
                // 벽 들이박기 난동 발동 — 돌진/폭주 중이 아닐 때, 가장 가까운 벽으로 첫 돌진
                rampCd += dt;
                if (rampCd >= RAMP_INT && chargePhase == 0 && !surging) {
                    state = 4; stateTimer = 0.0f; rampHits = 0; chargeTelegraph = false;
                    float dl = worldX, dr = (float)screenW - worldX;
                    float dtp = worldY, db = (float)screenH - worldY;
                    float mn = dl; rampDX = -1.0f; rampDY = 0.0f;
                    if (dr < mn)  { mn = dr;  rampDX = 1.0f;  rampDY = 0.0f; }
                    if (dtp < mn) { mn = dtp; rampDX = 0.0f;  rampDY = -1.0f; }
                    if (db < mn)  { mn = db;  rampDX = 0.0f;  rampDY = 1.0f; }
                    heading = atan2f(rampDY, rampDX);
                }
                if (state == 0 && stateTimer >= WANDER_T) {
                    // 멈추지 않고 곧바로 이탈 — 현재 위치에서 화면 바깥(중앙 반대)으로
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
            if (wasInside && !inside) shakePulse = true;   // 경계 넘는 순간 진동
            wasInside = inside;
            float M = 200.0f;
            if (worldX < -M || worldX > screenW + M || worldY < -M || worldY > screenH + M) {
                // 사라진 뒤에야 경로 결정 + 예고 시작
                pickDash();
                state = 2; stateTimer = 0.0f;
                worldX = dashFromX; worldY = dashFromY;
                for (auto& p : trail) p = glm::vec2(worldX, worldY);  // 잔상 방지(화면 밖)
            }
        }
        else if (state == 2) {       // ── 경로 예고(화면밖, 무적) ──
            if (stateTimer >= TELEGRAPH) { state = 3; stateTimer = 0.0f; dashT = 0.0f; }
        }
        else if (state == 3) {       // ── 곡선 재진입 돌진(무적) ──
            float chord = std::sqrt((dashToX-dashFromX)*(dashToX-dashFromX) +
                                    (dashToY-dashFromY)*(dashToY-dashFromY)) + 1e-3f;
            dashT += DASH_SPD * dt / chord;   // 0→1 (대략 일정 속도)
            if (dashT > 1.0f) dashT = 1.0f;
            glm::vec2 p = bezier(dashT);
            worldX = p.x; worldY = p.y;
            glm::vec2 tan = bezierTangent(dashT);
            if (tan.x * tan.x + tan.y * tan.y > 1e-6f)
                heading = atan2f(tan.y, tan.x);
            if (dashT >= 1.0f) {
                state = 0; stateTimer = 0.0f; ++dashCount;     // 돌진 완료 → 배회 가속
                spitTimer = 0.0f;                              // 재진입 직후 즉시 토사 방지
                surging = false; surgeCd = 0.0f;               // 폭주 리셋
                chargePhase = 0; chargeCdTimer = CHARGE_INT * 0.7f; chargeTelegraph = false;
            }
        }
        else {                       // ── state 4: 벽 들이박기 난동(파편+자해+진동) ──
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
                // 파편 탄환 (방사형) + 자해 + 화면 진동
                for (int i = 0; i < RAMP_SHRAP; i++) {
                    float a = (float)i / (float)RAMP_SHRAP * 6.2831853f + (float)(rand()%100)*0.01f;
                    fireDir(bullets, cosf(a), sinf(a), RAMP_SHRAP_SPD, glm::vec3(0.95f, 0.9f, 0.3f));
                }
                hp -= maxHp * 0.02f; if (hp < 1.0f) hp = 1.0f;   // 들이박을 때마다 피 조금
                shakePulse = true;                               // 화면 진동
                ++rampHits;
                if (rampHits >= RAMP_HITS) {                     // 난동 종료 → 배회 복귀
                    state = 0; stateTimer = 0.0f; rampCd = 0.0f;
                    spitTimer = 0.0f; surging = false; surgeCd = 0.0f;
                } else {                                         // 임의 내부 지점 향해 재돌진(다른 벽으로)
                    int rx = screenW - (int)(2.0f*mg); if (rx < 1) rx = 1;
                    int ry = screenH - (int)(2.0f*mg); if (ry < 1) ry = 1;
                    float tx = mg + (float)(rand()%rx), ty = mg + (float)(rand()%ry);
                    float dx = tx - worldX, dy = ty - worldY;
                    float d = std::sqrt(dx*dx+dy*dy) + 1e-3f;
                    rampDX = dx/d; rampDY = dy/d;
                }
            }
        }

        // 궤적 기록
        trail.insert(trail.begin(), glm::vec2(worldX, worldY));
        if ((int)trail.size() > NSEG * SEG_STEP + 8) trail.pop_back();

        bool dashing = (state == 1 || state == 3 || state == 4 || chargePhase == 2);
        float hcr = HEAD * 0.78f;
        float hdx = px - worldX, hdy = py - worldY;
        if (hdx*hdx + hdy*hdy < hcr * hcr)
            playerHP -= (dashing ? 22.0f : 12.0f) * dt;
        for (int i = 1; i <= NSEG; i++) {
            glm::vec2 s = segPos(i);
            float sr = segSize(i) + 4.0f;
            float sdx = px - s.x, sdy = py - s.y;
            if (sdx*sdx + sdy*sdy < sr * sr) playerHP -= 8.0f * dt;
        }
    }

    glm::vec2 segPos(int i) const {
        int idx = i * SEG_STEP;
        if (idx >= (int)trail.size()) idx = (int)trail.size() - 1;
        if (idx < 0) idx = 0;
        return trail[idx];
    }
};
