#pragma once
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <glm/glm.hpp>
#include "Bullet.h"
#include "DrawPrim.h"
#include "TextRenderer.h"
#include "Camera.h"
#include "PlayerStats.h"

extern TextRenderer g_TextS;

// ─────────────────────────────────────────────────────────────
// C2_RELAY.sys — 장님 케이블 촉수 (리워크 v3)
//   · 코어(터미널) + 케이블 촉수 5가닥. 촉수 끝 커넥터가 약점(=hosts[])
//   · 보스는 장님: 이동·사격 노이즈로만 플레이어를 추적 (SIG 게이지)
//     조용히 움직이면 촉수가 허공을 더듬고, 난사하면 정확히 조여온다
//   · 그랩: 감지 지점으로 촉수를 쏘아 잡히면 코어로 끌고 감 (물리면 물림 피해)
//   · 스윕: 감지 방향을 중심으로 넓은 호를 그리며 휩쓸기
//   · 촉수 1가닥당 코어 피해 18% 감소. 공격 중인 촉수를 끊으면 코어 노출(1.6배)
//   · 끊긴 촉수는 시간이 지나면 재생. 전부 끊기면 코어 패닉(방사탄)
// ─────────────────────────────────────────────────────────────
class BotnetBoss {
public:
    float worldX, worldY;
    float hp, maxHp;
    bool  alive = true;
    bool  exploded = false;
    int   screenW, screenH;
    bool  shakePulse = false;

    bool  phase2 = false;
    float orbitRot = 0.0f;
    float driftT = 0.0f;
    float logScroll = 0.0f;
    float hostHpBase = 0.0f;

    // ── 촉수 끝 커넥터 = 피격 노드 (main.cpp 호환 위해 Host 이름 유지) ──
    struct Host {
        float slotAngle = 0.0f;
        float x = 0.0f, y = 0.0f;
        float hp = 0.0f, maxHp = 0.0f;
        bool  alive = true;
        float fireCd = 0.0f;
        float rebuildCd = 0.0f;
    };
    static constexpr int NHOST = 5;   // 촉수 수
    Host hosts[NHOST];

    // ── 미니언 (v3 미사용 — main.cpp 호환용 빈 시스템) ──
    enum class MinionKind { Rush, Heavy, Pulse };
    struct Minion {
        MinionKind kind = MinionKind::Rush;
        float x = 0.0f, y = 0.0f;
        float vx = 0.0f, vy = 0.0f;
        float hp = 0.0f, maxHp = 0.0f;
        bool  alive = false;
        float fireCd = 0.0f;
        float pulseAng = 0.0f;
    };
    std::vector<Minion> minions;
    static float minionHit(MinionKind k) {
        switch (k) {
        case MinionKind::Heavy: return 22.0f;
        case MinionKind::Pulse: return 16.0f;
        default:                return 13.0f;
        }
    }
    int aliveMinions() const {
        int n = 0;
        for (auto& m : minions) if (m.alive) ++n;
        return n;
    }
    void deployRushFromHosts(int, float, float) {}
    void deployMapRush(float, float, int) {}

    // ── 촉수 체인 ──
    static constexpr int   NSEG    = 12;
    static constexpr float SEG_LEN = 42.0f;   // 도달 거리 ≈ 462
    float segX[NHOST][NSEG];
    float segY[NHOST][NSEG];
    float tipTx[NHOST];      // 틱 목표 (배회/공격)
    float tipTy[NHOST];
    float wanderPhase[NHOST];
    bool  prevAlive[NHOST];

    // ── 장님 감지 ──
    float noise = 0.0f;           // 0..1
    float senseX = 0.0f, senseY = 0.0f;
    float wanderOffX = 0.0f, wanderOffY = 0.0f;
    float wanderCd = 0.0f;
    float lastPx = 0.0f, lastPy = 0.0f;
    bool  havePrevP = false;

    // ── 그랩 ──
    enum class GrabState { Idle, Aim, Lash, Hold, Recover };
    GrabState grabState = GrabState::Idle;
    int   grabTent = -1;
    float grabT = 0.0f;
    float grabTx = 0.0f, grabTy = 0.0f;
    float grabCd = 5.0f;
    bool  grabConnected = false;

    // ── 스윕 ──
    enum class SweepState { Idle, Aim, Swing, Recover };
    SweepState sweepState = SweepState::Idle;
    int   sweepTent = -1;
    float sweepT = 0.0f;
    float sweepA0 = 0.0f, sweepA1 = 0.0f;
    float sweepCd = 8.0f;
    bool  sweepHit = false;

    // ── 노출 / 패닉 ──
    float exposedT = 0.0f;
    float stunT = 0.0f;
    float panicCd = 2.4f;

    static constexpr float BODY       = 62.0f;
    static constexpr float TERM_W     = 148.0f;
    static constexpr float TERM_H     = 108.0f;
    static constexpr float HOST_HIT   = 24.0f;
    static constexpr float PKT_SPD    = 430.0f;

    static constexpr float FW_PER_ARM   = 0.18f;
    static constexpr float EXPOSED_MULT = 1.6f;
    static constexpr float EXPOSED_DUR  = 4.0f;
    static constexpr float EXPOSED_DUR2 = 3.2f;
    static constexpr float REGROW_T     = 12.0f;
    static constexpr float REGROW_T2    = 8.0f;

    static constexpr float GRAB_AIM_DUR   = 0.9f;
    static constexpr float GRAB_LASH_DUR  = 0.22f;
    static constexpr float GRAB_HOLD_DUR  = 1.15f;
    static constexpr float GRAB_CD        = 5.5f;
    static constexpr float GRAB_CD2       = 3.8f;
    static constexpr float GRAB_PULL_SPD  = 235.0f;
    static constexpr float GRAB_RADIUS    = 30.0f;

    static constexpr float SWEEP_AIM_DUR  = 0.8f;
    static constexpr float SWEEP_DUR      = 0.55f;
    static constexpr float SWEEP_CD       = 7.5f;
    static constexpr float SWEEP_CD2      = 5.2f;
    static constexpr float SWEEP_R        = 300.0f;
    static constexpr float SWEEP_ARC      = 2.45f;   // ≈140°

    BotnetBoss(int sw, int sh, float hpInit) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.5f;
        worldY = sh * 0.45f;
        senseX = worldX; senseY = worldY + 200.0f;
        hostHpBase = hpInit * 0.16f;
        if (hostHpBase < 180.0f) hostHpBase = 180.0f;
        for (int i = 0; i < NHOST; i++) {
            hosts[i].slotAngle = (float)i * (6.2831853f / (float)NHOST);
            hosts[i].hp = hosts[i].maxHp = hostHpBase;
            hosts[i].rebuildCd = 0.0f;
            wanderPhase[i] = (float)(rand() % 628) * 0.01f;
            prevAlive[i] = true;
            float ax, ay;
            anchorOf(i, ax, ay);
            float dx = cosf(hosts[i].slotAngle), dy = sinf(hosts[i].slotAngle);
            for (int s = 0; s < NSEG; s++) {
                segX[i][s] = ax + dx * SEG_LEN * (float)s * 0.5f;
                segY[i][s] = ay + dy * SEG_LEN * (float)s * 0.5f;
            }
            tipTx[i] = segX[i][NSEG - 1];
            tipTy[i] = segY[i][NSEG - 1];
            hosts[i].x = segX[i][NSEG - 1];
            hosts[i].y = segY[i][NSEG - 1];
        }
    }

    int aliveHosts() const {
        int n = 0;
        for (int i = 0; i < NHOST; i++) if (hosts[i].alive) ++n;
        return n;
    }

    bool exposed() const { return exposedT > 0.0f; }
    float senseLevel() const { return noise; }

    float bodyDamageTakenMult() const {
        if (exposedT > 0.0f) return EXPOSED_MULT;
        float red = FW_PER_ARM * (float)aliveHosts();
        if (red > 0.9f) red = 0.9f;
        return 1.0f - red;
    }

    float hostShieldPercent() const {
        if (exposedT > 0.0f) return 0.0f;
        return (1.0f - bodyDamageTakenMult()) * 100.0f;
    }

    void anchorOf(int i, float& ax, float& ay) const {
        float a = hosts[i].slotAngle + orbitRot * 0.25f;
        ax = worldX + cosf(a) * (TERM_W * 0.52f);
        ay = worldY + sinf(a) * (TERM_H * 0.52f);
    }

    void firePacket(std::vector<Bullet>& bullets, float fx, float fy,
                    float tx, float ty, float spd, glm::vec3 col, float edmg) {
        Bullet bb(fx, fy, tx, ty);
        bb.isEnemy = true;
        bb.speed = spd;
        bb.enemyDmg = edmg;
        bb.color = col;
        bullets.push_back(bb);
    }

    bool tentBusy(int i) const {
        if (grabTent == i && grabState != GrabState::Idle) return true;
        if (sweepTent == i && sweepState != SweepState::Idle) return true;
        return false;
    }

    void cancelAttacksOf(int i) {
        if (grabTent == i && grabState != GrabState::Idle) {
            grabState = GrabState::Idle;
            grabTent = -1;
            grabConnected = false;
            grabCd = (phase2 ? GRAB_CD2 : GRAB_CD) * 0.6f;
        }
        if (sweepTent == i && sweepState != SweepState::Idle) {
            sweepState = SweepState::Idle;
            sweepTent = -1;
            sweepCd = (phase2 ? SWEEP_CD2 : SWEEP_CD) * 0.6f;
        }
    }

    int pickFreeTent() const {
        int order[NHOST];
        for (int i = 0; i < NHOST; i++) order[i] = i;
        for (int i = NHOST - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            int t = order[i]; order[i] = order[j]; order[j] = t;
        }
        for (int k = 0; k < NHOST; k++) {
            int i = order[k];
            if (hosts[i].alive && !tentBusy(i)) return i;
        }
        return -1;
    }

    // ── 노이즈 감지 ──
    void updateSense(float px, float py, float dt, const std::vector<Bullet>& bullets) {
        if (dt <= 0.0f) return;

        float decay = phase2 ? 0.22f : 0.32f;
        noise -= decay * dt;

        if (havePrevP) {
            float mdx = px - lastPx, mdy = py - lastPy;
            float spd = sqrtf(mdx * mdx + mdy * mdy) / dt;
            if (spd > 40.0f)  noise += 0.5f * dt;
            if (spd > 260.0f) noise += 0.9f * dt;   // 대시급 이동은 크게 울림
        }
        lastPx = px; lastPy = py; havePrevP = true;

        int nPB = 0;
        for (const auto& b : bullets) {
            if (b.active && !b.isEnemy) { nPB++; if (nPB >= 8) break; }
        }
        noise += 0.22f * (float)nPB * dt;

        if (noise < 0.0f) noise = 0.0f;
        if (noise > 1.0f) noise = 1.0f;

        // 감지 지점: 노이즈 높을수록 실제 위치로 빠르게 수렴
        float track = noise * noise * 7.0f;
        senseX += (px - senseX) * std::min(1.0f, track * dt);
        senseY += (py - senseY) * std::min(1.0f, track * dt);

        // 노이즈 낮으면 허공을 더듬는 오차 벡터
        wanderCd -= dt;
        if (wanderCd <= 0.0f) {
            wanderCd = 1.2f + (float)(rand() % 100) * 0.012f;
            float wa = (float)(rand() % 628) * 0.01f;
            float wr = (1.0f - noise) * (60.0f + (float)(rand() % 130));
            wanderOffX = cosf(wa) * wr;
            wanderOffY = sinf(wa) * wr;
        }
    }

    float senseAimX() const { return senseX + wanderOffX * (1.0f - noise); }
    float senseAimY() const { return senseY + wanderOffY * (1.0f - noise); }

    // ── 촉수 체인 이동 (FABRIK 2패스) ──
    void solveChain(int i) {
        float ax, ay;
        anchorOf(i, ax, ay);
        // backward: 끝을 목표에 두고 베이스 방향으로 간격 고정
        segX[i][NSEG - 1] = tipTx[i];
        segY[i][NSEG - 1] = tipTy[i];
        for (int s = NSEG - 2; s >= 0; s--) {
            float dx = segX[i][s] - segX[i][s + 1];
            float dy = segY[i][s] - segY[i][s + 1];
            float d = sqrtf(dx * dx + dy * dy) + 1e-4f;
            segX[i][s] = segX[i][s + 1] + dx / d * SEG_LEN;
            segY[i][s] = segY[i][s + 1] + dy / d * SEG_LEN;
        }
        // forward: 베이스를 앵커에 고정하고 끝 방향으로 간격 고정
        segX[i][0] = ax; segY[i][0] = ay;
        for (int s = 1; s < NSEG; s++) {
            float dx = segX[i][s] - segX[i][s - 1];
            float dy = segY[i][s] - segY[i][s - 1];
            float d = sqrtf(dx * dx + dy * dy) + 1e-4f;
            segX[i][s] = segX[i][s - 1] + dx / d * SEG_LEN;
            segY[i][s] = segY[i][s - 1] + dy / d * SEG_LEN;
        }
        hosts[i].x = segX[i][NSEG - 1];
        hosts[i].y = segY[i][NSEG - 1];
    }

    void moveTipToward(int i, float tx, float ty, float maxSpd, float dt) {
        float dx = tx - tipTx[i], dy = ty - tipTy[i];
        float d = sqrtf(dx * dx + dy * dy);
        if (d < 1e-3f) return;
        float step = maxSpd * dt;
        if (step > d) step = d;
        tipTx[i] += dx / d * step;
        tipTy[i] += dy / d * step;
    }

    void updateTentacles(float px, float py, float dt, float& playerHP, float t) {
        float sx = senseAimX(), sy = senseAimY();
        for (int i = 0; i < NHOST; i++) {
            if (!hosts[i].alive) continue;
            if (tentBusy(i)) { solveChain(i); continue; }   // 공격 상태는 전용 로직이 틱 제어

            // 배회: 감지 지점 방향 + 개별 위상 흔들림
            float ax, ay;
            anchorOf(i, ax, ay);
            float toSx = sx - ax, toSy = sy - ay;
            float sd = sqrtf(toSx * toSx + toSy * toSy) + 1e-3f;
            float bias = 0.35f + noise * 0.5f;   // 노이즈 높을수록 감지 쪽으로 쏠림
            float baseA = hosts[i].slotAngle + orbitRot * 0.25f;
            float wobA = baseA + sinf(t * 0.9f + wanderPhase[i]) * 0.8f;
            float reach = SEG_LEN * (NSEG - 1);
            float wr = reach * (0.5f + 0.18f * sinf(t * 1.3f + wanderPhase[i] * 2.0f));
            float wx = ax + (cosf(wobA) * (1.0f - bias) + toSx / sd * bias) * wr;
            float wy = ay + (sinf(wobA) * (1.0f - bias) + toSy / sd * bias) * wr;
            float spd = (stunT > 0.0f) ? 60.0f : (150.0f + noise * 180.0f);
            moveTipToward(i, wx, wy, spd, dt);
            solveChain(i);

            // 배회 중 끝단 접촉 피해 (소량)
            float ddx = px - hosts[i].x, ddy = py - hosts[i].y;
            if (ddx * ddx + ddy * ddy < 24.0f * 24.0f)
                HurtPlayer(playerHP, 7.0f * dt);
        }
    }

    // ── 그랩 ──
    void updateGrab(float px, float py, float dt, float& playerHP,
                    float& pullX, float& pullY) {
        switch (grabState) {
        case GrabState::Idle: {
            float rate = (noise > 0.85f) ? 2.0f : 1.0f;   // 시끄러우면 공격적으로
            grabCd -= dt * rate;
            if (grabCd <= 0.0f && stunT <= 0.0f) {
                grabTent = pickFreeTent();
                if (grabTent >= 0) {
                    grabState = GrabState::Aim;
                    grabT = 0.0f;
                    grabTx = senseAimX();   // 감지 지점에 고정 조준 — 장님이라 빗나갈 수 있음
                    grabTy = senseAimY();
                    grabConnected = false;
                } else {
                    grabCd = 0.8f;
                }
            }
        } break;
        case GrabState::Aim: {
            grabT += dt;
            int i = grabTent;
            float ax, ay;
            anchorOf(i, ax, ay);
            // 움츠림: 목표 반대쪽으로 당김
            float dx = grabTx - ax, dy = grabTy - ay;
            float d = sqrtf(dx * dx + dy * dy) + 1e-3f;
            moveTipToward(i, ax + dx / d * SEG_LEN * 2.0f, ay + dy / d * SEG_LEN * 2.0f,
                          520.0f, dt);
            solveChain(i);
            if (grabT >= GRAB_AIM_DUR) { grabState = GrabState::Lash; grabT = 0.0f; }
        } break;
        case GrabState::Lash: {
            grabT += dt;
            int i = grabTent;
            moveTipToward(i, grabTx, grabTy, 2800.0f, dt);
            solveChain(i);
            float ddx = px - hosts[i].x, ddy = py - hosts[i].y;
            if (ddx * ddx + ddy * ddy < GRAB_RADIUS * GRAB_RADIUS) {
                grabState = GrabState::Hold;
                grabT = 0.0f;
                grabConnected = true;
                shakePulse = true;
                noise = 1.0f;   // 잡았으니 위치 확정
            } else if (grabT >= GRAB_LASH_DUR) {
                grabState = GrabState::Recover;
                grabT = 0.0f;
            }
        } break;
        case GrabState::Hold: {
            grabT += dt;
            int i = grabTent;
            tipTx[i] = px; tipTy[i] = py;   // 끝단이 플레이어에 부착
            solveChain(i);
            float cdx = worldX - px, cdy = worldY - py;
            float cd = sqrtf(cdx * cdx + cdy * cdy) + 1e-3f;
            pullX += cdx / cd * GRAB_PULL_SPD * dt;
            pullY += cdy / cd * GRAB_PULL_SPD * dt;
            HurtPlayer(playerHP, 8.0f * dt);
            if (cd < BODY + 36.0f) {
                // 코어까지 끌려옴 → 물림
                HurtPlayer(playerHP, 15.0f);
                shakePulse = true;
                grabState = GrabState::Recover;
                grabT = 0.0f;
                grabConnected = false;
            } else if (grabT >= GRAB_HOLD_DUR) {
                grabState = GrabState::Recover;
                grabT = 0.0f;
                grabConnected = false;
            }
        } break;
        case GrabState::Recover: {
            grabT += dt;
            int i = grabTent;
            if (i >= 0 && hosts[i].alive) solveChain(i);
            if (grabT >= 0.55f) {
                grabState = GrabState::Idle;
                grabTent = -1;
                grabCd = (phase2 ? GRAB_CD2 : GRAB_CD) + (float)(rand() % 15) * 0.1f;
            }
        } break;
        }
    }

    // ── 스윕 ──
    void updateSweep(float px, float py, float dt, float& playerHP) {
        switch (sweepState) {
        case SweepState::Idle:
            sweepCd -= dt;
            if (sweepCd <= 0.0f && stunT <= 0.0f) {
                sweepTent = pickFreeTent();
                if (sweepTent >= 0) {
                    sweepState = SweepState::Aim;
                    sweepT = 0.0f;
                    sweepHit = false;
                    float mid = atan2f(senseAimY() - worldY, senseAimX() - worldX);
                    float dir = (rand() % 2 == 0) ? 1.0f : -1.0f;
                    sweepA0 = mid - SWEEP_ARC * 0.5f * dir;
                    sweepA1 = mid + SWEEP_ARC * 0.5f * dir;
                } else {
                    sweepCd = 0.8f;
                }
            }
            break;
        case SweepState::Aim: {
            sweepT += dt;
            int i = sweepTent;
            moveTipToward(i, worldX + cosf(sweepA0) * SWEEP_R,
                          worldY + sinf(sweepA0) * SWEEP_R, 900.0f, dt);
            solveChain(i);
            if (sweepT >= SWEEP_AIM_DUR) { sweepState = SweepState::Swing; sweepT = 0.0f; }
        } break;
        case SweepState::Swing: {
            sweepT += dt;
            int i = sweepTent;
            float u = sweepT / SWEEP_DUR;
            if (u > 1.0f) u = 1.0f;
            float a = sweepA0 + (sweepA1 - sweepA0) * u;
            tipTx[i] = worldX + cosf(a) * SWEEP_R;
            tipTy[i] = worldY + sinf(a) * SWEEP_R;
            solveChain(i);
            if (!sweepHit) {
                float ddx = px - hosts[i].x, ddy = py - hosts[i].y;
                if (ddx * ddx + ddy * ddy < 34.0f * 34.0f) {
                    HurtPlayer(playerHP, 16.0f);
                    sweepHit = true;
                }
            }
            if (sweepT >= SWEEP_DUR) { sweepState = SweepState::Recover; sweepT = 0.0f; }
        } break;
        case SweepState::Recover:
            sweepT += dt;
            if (sweepTent >= 0 && hosts[sweepTent].alive) solveChain(sweepTent);
            if (sweepT >= 0.5f) {
                sweepState = SweepState::Idle;
                sweepTent = -1;
                sweepCd = (phase2 ? SWEEP_CD2 : SWEEP_CD) + (float)(rand() % 15) * 0.1f;
            }
            break;
        }
    }

    void updateRegrow(float dt) {
        for (int i = 0; i < NHOST; i++) {
            Host& h = hosts[i];
            if (h.alive) continue;
            if (h.rebuildCd <= 0.0f)
                h.rebuildCd = phase2 ? REGROW_T2 : REGROW_T;
            h.rebuildCd -= dt;
            if (h.rebuildCd <= 0.0f) {
                h.alive = true;
                h.hp = h.maxHp = hostHpBase * 0.5f;
                h.rebuildCd = 0.0f;
                float ax, ay;
                anchorOf(i, ax, ay);
                tipTx[i] = ax; tipTy[i] = ay;
                for (int s = 0; s < NSEG; s++) { segX[i][s] = ax; segY[i][s] = ay; }
            }
        }
    }

    // 촉수 사망 감지 — 공격 중이던 촉수가 끊기면 코어 노출
    void checkSevered() {
        for (int i = 0; i < NHOST; i++) {
            bool nowAlive = hosts[i].alive;
            if (prevAlive[i] && !nowAlive) {
                bool wasBusy = tentBusy(i);
                cancelAttacksOf(i);
                if (wasBusy) {
                    exposedT = phase2 ? EXPOSED_DUR2 : EXPOSED_DUR;
                    stunT = 1.4f;
                    shakePulse = true;
                }
            }
            prevAlive[i] = nowAlive;
        }
    }

    void Update(float px, float py, float dt, float& playerHP,
                std::vector<Bullet>& bullets, float& pullX, float& pullY) {
        if (!alive) return;
        if (!phase2 && hp <= maxHp * 0.5f) phase2 = true;

        if (exposedT > 0.0f) exposedT -= dt;
        if (stunT > 0.0f) stunT -= dt;

        driftT += dt;
        logScroll += dt * 32.0f;
        float drift = (stunT > 0.0f) ? 0.0f : (phase2 ? 46.0f : 58.0f);
        worldX += cosf(driftT * 0.22f) * drift * dt;
        worldY += sinf(driftT * 0.31f) * drift * dt;
        float m = 90.0f;
        if (worldX < m) worldX = m;
        if (worldX > screenW - m) worldX = screenW - m;
        if (worldY < m) worldY = m;
        if (worldY > screenH - m) worldY = screenH - m;

        if (stunT <= 0.0f)
            orbitRot += 0.2f * dt;

        checkSevered();
        updateSense(px, py, dt, bullets);
        updateRegrow(dt);

        float t = driftT;
        updateTentacles(px, py, dt, playerHP, t);
        updateGrab(px, py, dt, playerHP, pullX, pullY);
        updateSweep(px, py, dt, playerHP);

        // 촉수 전멸 → 패닉 방사탄
        if (aliveHosts() == 0 && stunT <= 0.0f) {
            panicCd -= dt;
            if (panicCd <= 0.0f) {
                panicCd = phase2 ? 1.9f : 2.4f;
                int n = 8;
                float off = (float)(rand() % 628) * 0.01f;
                for (int i = 0; i < n; i++) {
                    float a = off + (float)i / (float)n * 6.2831853f;
                    firePacket(bullets, worldX, worldY,
                               worldX + cosf(a) * 200.0f, worldY + sinf(a) * 200.0f,
                               PKT_SPD, glm::vec3(1.0f, 0.35f, 0.25f), 8.5f);
                }
            }
        }

        float bdx = px - worldX, bdy = py - worldY;
        if (bdx * bdx + bdy * bdy < BODY * BODY)
            HurtPlayer(playerHP, (phase2 ? 13.0f : 8.0f) * dt);
    }

    // ── 렌더 ──
    static void wireSeg(float x0, float y0, float x1, float y1, float thick,
                        float r, float g, float b, float a) {
        float dx = x1 - x0, dy = y1 - y0;
        float len = sqrtf(dx * dx + dy * dy);
        if (len < 0.5f) return;
        float nx = -dy / len * thick * 0.5f;
        float ny =  dx / len * thick * 0.5f;
        BatchTri(x0 + nx, y0 + ny, x1 + nx, y1 + ny, x1 - nx, y1 - ny, r, g, b, a);
        BatchTri(x0 + nx, y0 + ny, x1 - nx, y1 - ny, x0 - nx, y0 - ny, r, g, b, a);
    }

    static void hollowRing(float cx, float cy, float rad, int seg, float thick,
                           float r, float g, float b, float a) {
        float px = cx + rad, py = cy;
        for (int s = 1; s <= seg; s++) {
            float th = (float)s / (float)seg * 6.2831853f;
            float nx = cx + rad * cosf(th), ny = cy + rad * sinf(th);
            wireSeg(px, py, nx, ny, thick, r, g, b, a);
            px = nx; py = ny;
        }
    }

    void renderTentacle(int i, float time) {
        const Host& h = hosts[i];
        if (!h.alive) {
            // 절단 스텀프 — 앵커에서 짧게 꿈틀 + 스파크
            float ax, ay;
            anchorOf(i, ax, ay);
            float ja = hosts[i].slotAngle + orbitRot * 0.25f + sinf(time * 7.0f + (float)i) * 0.4f;
            float jx = ax + cosf(ja) * SEG_LEN * 1.2f;
            float jy = ay + sinf(ja) * SEG_LEN * 1.2f;
            wireSeg(ax, ay, jx, jy, 6.0f, 0.1f, 0.14f, 0.12f, 0.95f);
            if ((rand() % 5) == 0)
                drawCircle(jx, jy, 3.0f + (float)(rand() % 3),
                           0.4f + (float)(rand() % 60) * 0.01f, 0.95f, 0.55f, 0.8f);
            return;
        }

        bool busyGrab  = (grabTent == i && grabState != GrabState::Idle);
        bool busySweep = (sweepTent == i && sweepState != SweepState::Idle);
        bool hot = busyGrab || busySweep;

        // 케이블 몸통: 굵은 어두운 피복 + 밝은 심선. 중간 마디에 시각적 물결
        for (int s = 0; s < NSEG - 1; s++) {
            float u = (float)s / (float)(NSEG - 1);
            float wob = sinf(time * 3.2f + (float)s * 0.8f + wanderPhase[i]) *
                        6.0f * sinf(u * 3.14159f);
            float dx = segX[i][s + 1] - segX[i][s];
            float dy = segY[i][s + 1] - segY[i][s];
            float dl = sqrtf(dx * dx + dy * dy) + 1e-3f;
            float ox = -dy / dl * wob, oy = dx / dl * wob;

            float x0 = segX[i][s] + ox * (s > 0 ? 1.0f : 0.0f);
            float y0 = segY[i][s] + oy * (s > 0 ? 1.0f : 0.0f);
            float x1 = segX[i][s + 1] + ox;
            float y1 = segY[i][s + 1] + oy;

            float thick = 9.0f - u * 5.0f;
            // 피복 (어두운)
            wireSeg(x0, y0, x1, y1, thick, 0.07f, 0.1f, 0.09f, 0.96f);
            // 심선 (테마 그린 / 공격 시 적색)
            float cr = hot ? 1.0f : 0.2f;
            float cg = hot ? 0.4f : 0.85f;
            float cb = hot ? 0.2f : 0.45f;
            float pulse = 0.5f + 0.5f * sinf(time * 6.0f - u * 9.0f + (float)i);
            wireSeg(x0, y0, x1, y1, thick * 0.32f, cr, cg, cb, 0.35f + 0.45f * pulse);
        }

        // 끝단 커넥터 (RJ45풍 각진 헤드) — 피격 노드
        {
            float dx = segX[i][NSEG - 1] - segX[i][NSEG - 2];
            float dy = segY[i][NSEG - 1] - segY[i][NSEG - 2];
            float a = atan2f(dy, dx);
            float cx = hosts[i].x, cy = hosts[i].y;
            float c = cosf(a), s = sinf(a);
            float hw = 13.0f, hh = 9.0f;
            float pxs[4] = { -hw, hw, hw, -hw };
            float pys[4] = { -hh, -hh, hh, hh };
            float wx[4], wy[4];
            for (int k = 0; k < 4; k++) {
                wx[k] = cx + pxs[k] * c - pys[k] * s;
                wy[k] = cy + pxs[k] * s + pys[k] * c;
            }
            float br = hot ? 1.0f : 0.16f;
            float bg = hot ? 0.45f : 0.24f;
            float bb = hot ? 0.2f : 0.2f;
            BatchTri(wx[0], wy[0], wx[1], wy[1], wx[2], wy[2], br, bg, bb, 1.0f);
            BatchTri(wx[0], wy[0], wx[2], wy[2], wx[3], wy[3], br, bg, bb, 1.0f);
            // 커넥터 핀 (밝은 눈금)
            for (int p = -1; p <= 1; p++) {
                float ppx = cx + c * (hw * 0.45f) - s * (float)p * 4.5f;
                float ppy = cy + s * (hw * 0.45f) + c * (float)p * 4.5f;
                drawCircle(ppx, ppy, 1.8f, 0.9f, 0.95f, 0.7f, 0.9f);
            }
            if (h.hp < h.maxHp) {
                float hf = h.hp / h.maxHp;
                if (hf < 0.0f) hf = 0.0f;
                drawRect(cx - 15.0f, cy - 20.0f, 30.0f * hf, 3.5f,
                         0.22f, 0.92f, 0.48f, 0.9f);
            }
        }
    }

    // 촉수+예고+감지 고스트 — 창마다 반복 렌더 (촉수가 보스 창 밖까지 뻗음)
    void renderArms(float time) {
        BindMainShader();

        for (int i = 0; i < NHOST; i++)
            renderTentacle(i, time);

        // ── 그랩 예고 (조준선 + 목표 마커) ──
        if (grabState == GrabState::Aim && grabTent >= 0 && hosts[grabTent].alive) {
            float warn = 0.3f + 0.35f * sinf(time * 16.0f);
            float fx = hosts[grabTent].x, fy = hosts[grabTent].y;
            float dx = grabTx - fx, dy = grabTy - fy;
            float d = sqrtf(dx * dx + dy * dy) + 1e-3f;
            for (int s = 0; s < 12; s++) {
                float u = (float)s / 12.0f;
                drawCircle(fx + dx * u, fy + dy * u, 2.6f, 1.0f, 0.45f, 0.2f, warn * (1.0f - u * 0.3f));
            }
            hollowRing(grabTx, grabTy, GRAB_RADIUS + 6.0f, 12, 2.0f, 1.0f, 0.4f, 0.2f, warn + 0.2f);
        }

        // ── 스윕 예고 (호 점선) ──
        if (sweepState == SweepState::Aim && sweepTent >= 0 && hosts[sweepTent].alive) {
            float warn = 0.28f + 0.3f * sinf(time * 14.0f);
            for (int s = 0; s <= 16; s++) {
                float u = (float)s / 16.0f;
                float a = sweepA0 + (sweepA1 - sweepA0) * u;
                drawCircle(worldX + cosf(a) * SWEEP_R, worldY + sinf(a) * SWEEP_R,
                           3.0f, 1.0f, 0.55f, 0.15f, warn);
            }
        }

        // ── 감지 고스트 (보스가 '생각하는' 플레이어 위치) ──
        {
            float gx = senseAimX(), gy = senseAimY();
            float ga = 0.1f + noise * 0.3f;
            float gr2 = 1.0f, gg2 = 1.0f - noise * 0.6f, gb2 = 0.4f;
            float jit = (1.0f - noise) * 4.0f;
            gx += sinf(time * 21.0f) * jit;
            gy += cosf(time * 17.0f) * jit;
            hollowRing(gx, gy, 14.0f + (1.0f - noise) * 8.0f, 10, 1.4f, gr2, gg2, gb2, ga);
            drawCircle(gx, gy, 2.5f, gr2, gg2, gb2, ga + 0.15f);
        }
    }

    void renderCore(float time) {
        BindMainShader();
        float pulse = 0.5f + 0.5f * sinf(time * 4.0f);
        const float GR = 0.22f, GG = 0.92f, GB = 0.48f;
        bool exp = exposed();

        // ── 코어 터미널 ──
        float tx = worldX - TERM_W * 0.5f, ty = worldY - TERM_H * 0.5f;
        drawRect(tx - 6.0f, ty - 6.0f, TERM_W + 12.0f, TERM_H + 12.0f, 0.04f, 0.06f, 0.05f, 0.96f);
        drawRect(tx, ty, TERM_W, TERM_H, 0.02f, 0.04f, 0.03f, 1.0f);
        float hdR = exp ? 0.7f : (phase2 ? 0.55f : 0.1f);
        float hdG = exp ? 0.1f : (phase2 ? 0.14f : 0.26f);
        float hdB = exp ? 0.08f : (phase2 ? 0.08f : 0.12f);
        drawRect(tx, ty, TERM_W, 14.0f, hdR, hdG, hdB, 1.0f);

        const wchar_t* title = exp    ? L"C2_RELAY.sys  !! EXPOSED !!"
                             : (stunT > 0.0f) ? L"C2_RELAY.sys  ** SEIZURE **"
                             : phase2 ? L"C2_RELAY.sys  !!! DEFCON 2 !!!"
                                      : L"C2_RELAY.sys  :: listening...";
        float tScale = 0.38f * g_ViewZoom;
        g_TextS.Draw(title, W2SX(tx + 8.0f), W2SY(ty + 2.0f), tScale, 1.0f, 0.95f, 0.9f, 1.0f);

        wchar_t stat[80];
        if (exp)
            swprintf_s(stat, L"ARM %d/%d  FW --- BREACH  SIG %d%%",
                       aliveHosts(), NHOST, (int)(noise * 100.0f + 0.5f));
        else
            swprintf_s(stat, L"ARM %d/%d  FW %d%%  SIG %d%%",
                       aliveHosts(), NHOST, (int)(hostShieldPercent() + 0.5f),
                       (int)(noise * 100.0f + 0.5f));
        g_TextS.Draw(stat, W2SX(tx + 8.0f), W2SY(ty + TERM_H - 22.0f),
                     0.30f * g_ViewZoom, exp ? 1.0f : GR, exp ? 0.35f : GG, exp ? 0.25f : GB, 0.85f);

        float lineY = ty + 18.0f;
        for (int ln = 0; ln < 6; ln++) {
            float ly = lineY + ln * 12.0f;
            float wob = fmodf(logScroll + ln * 41.0f, 140.0f);
            drawRect(tx + 8.0f, ly, 22.0f + wob, 2.0f, GR * 0.7f, GG * 0.7f, GB * 0.7f, 0.55f);
        }
        float curBlink = (sinf(time * 8.0f) > 0.0f) ? 1.0f : 0.0f;
        drawRect(tx + 10.0f + fmodf(logScroll, 60.0f), ty + TERM_H - 28.0f,
                 7.0f, 10.0f, GR, GG, GB, curBlink);

        if (exp) {
            float fl = 0.5f + 0.5f * sinf(time * 20.0f);
            drawNeonBorder(tx - 3.0f, ty - 3.0f, TERM_W + 6.0f, TERM_H + 6.0f,
                           1.0f, 0.15f + 0.2f * fl, 0.1f);
            float ef = exposedT / (phase2 ? EXPOSED_DUR2 : EXPOSED_DUR);
            if (ef < 0.0f) ef = 0.0f;
            drawRect(tx, ty - 14.0f, TERM_W * ef, 5.0f, 1.0f, 0.3f, 0.15f, 0.95f);
        } else {
            drawNeonBorder(tx - 3.0f, ty - 3.0f, TERM_W + 6.0f, TERM_H + 6.0f, GR, GG * pulse, GB);
        }

        if (hp < maxHp) {
            float hf = hp / maxHp;
            if (hf < 0.0f) hf = 0.0f;
            drawRect(tx, ty + TERM_H + 8.0f, TERM_W * hf, 5.0f,
                     phase2 ? 1.0f : GR, phase2 ? 0.4f : GG, phase2 ? 0.2f : GB, 0.95f);
        }
    }

    void renderMinions(float time) {
        BindMainShader();
        const float GR = 0.22f, GG = 0.92f, GB = 0.48f;
        for (auto& m : minions) {
            if (!m.alive) continue;
            drawCircle(m.x, m.y, 11.0f, GR, GG, GB, 0.16f);
            drawRect(m.x - 8.0f, m.y - 3.0f, 16.0f, 6.0f, GR, GG, GB, 0.9f);
        }
        (void)time;
    }
};
