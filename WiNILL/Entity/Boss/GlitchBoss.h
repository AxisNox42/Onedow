#pragma once
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>

// ─────────────────────────────────────────────────────────────
// 글리치 보스 (Glitch / "Err_0x7B")
//   페이즈 머신:
//   1) GLITCH_WARNING : 출현 전조 — 화면 글리치 + UI 텍스트 노이즈 (3~5초)
//   2) SPAWN_MINI     : 본체는 구석에 숨고, 플레이어 주변에 '뚝뚝' 끊기듯
//                       아주 빠른 작은 세모 떼 대량 스폰 (공격력 0.5)
//   3) BURST_ATTACK   : "펑!" 화이트아웃 → 남은 세모 전부 플레이어로 유도 가속
//                       + 본체는 화면 가로지르는 직선 레이저 포격
//   4) COOLDOWN       : 회복 후 다시 2)
//   효과(글리치/플래시/텍스트노이즈)는 신호값만 노출 → main.cpp 가 렌더
// ─────────────────────────────────────────────────────────────

enum class BossState { GLITCH_WARNING, SPAWN_MINI, BURST_ATTACK, COOLDOWN };

struct MiniTri {
    float x = 0, y = 0;
    float vx = 0, vy = 0;
    float angle = 0;
    bool  homing = false;   // BURST 시 유도 가속 ON
    bool  alive  = true;
};

class GlitchBoss {
public:
    float worldX, worldY;          // 본체 (화면 구석에 숨음)
    float hp = 9000.0f, maxHp = 9000.0f;
    bool  alive    = true;
    bool  exploded = false;
    int   screenW, screenH;

    BossState state = BossState::GLITCH_WARNING;
    float stateTimer = 0.0f;

    std::vector<MiniTri> minis;
    float spawnAccum = 0.0f;

    // 레이저 (BURST 동안 활성)
    bool  laserActive = false;
    bool  laserWarn   = false;   // 발사 전 경고선 (SPAWN_MINI 막바지)
    float laserDirX = 1.0f, laserDirY = 0.0f;
    float laser2DirX = 0.0f, laser2DirY = 1.0f;   // 페이즈2: 직교 두 번째 레이저(X자)
    static constexpr float LASER_WARN_LEAD = 0.8f;   // 발사 0.8초 전부터 경고

    // 페이즈2 (HP 50% 이하) — 듀얼 레이저 + 과밀 스웜 + 상시 글리치
    bool  phase2 = false;

    // 이동(드리프트) — 고정형 일점사 방지
    float driftTX = 0, driftTY = 0, driftTimer = 0.0f;
    bool  driftInit = false;
    static constexpr float DRIFT_SPD = 70.0f;

    // 페이즈2 잔상(디코이) — 본체와 똑같이 보이는 가짜. 쏘면 '이벤트' 발생(재배치+노이즈).
    struct Decoy { float x = 0, y = 0; float respawn = 0.0f; bool alive = true; };
    std::vector<Decoy> decoys;
    float decoyFlash = 0.0f;     // 잔상 파괴 연출 신호 (main 렌더)
    float novaTimer  = 0.0f;     // 신규 스킬: 글리치 노바(방사 세모) 쿨다운
    static constexpr float NOVA_INT = 7.0f;
    static constexpr int   NOVA_N   = 16;

    // 효과 신호 (0~1) — main.cpp 가 읽어서 화면 연출
    float glitchAmount = 0.0f;   // 화면 좌우 찢김 강도
    float burstFlash   = 0.0f;   // 화이트아웃 깜빡임
    float textNoise    = 0.0f;   // UI 텍스트 깨짐 강도

    static constexpr float BODY = 46.0f;          // 플레이어(25) 보다 큼

    // 페이즈 지속 시간
    static constexpr float T_WARNING = 4.0f;
    static constexpr float T_SPAWN   = 6.0f;
    static constexpr float T_BURST   = 1.3f;
    static constexpr float T_COOL    = 3.0f;

    static constexpr float MINI_SPEED       = 520.0f;
    static constexpr float MINI_BURST_ACCEL = 1500.0f;  // BURST 유도 가속
    static constexpr float MINI_DMG         = 6.0f;     // 접촉 1회 (일반 몹의 약 절반)
    static constexpr float MINI_HIT_R       = 16.0f;

    GlitchBoss(int sw, int sh, float hpInit = 9000.0f) : screenW(sw), screenH(sh) {
        hp = maxHp = hpInit;
        worldX = sw * 0.85f; worldY = sh * 0.15f;
    }

    void hideInCorner() {
        int c = rand() % 4;
        float m = 0.12f;
        worldX = (c & 1) ? screenW * (1.0f - m) : screenW * m;
        worldY = (c & 2) ? screenH * (1.0f - m) : screenH * m;
    }

    void spawnMini(float px, float py) {
        // 화면 '밖' 가장자리에서 등장 → 플레이어 방향으로 진입
        float m = 60.0f;   // 화면 밖 여유
        float sx, sy;
        switch (rand() % 4) {
        case 0:  sx = (float)(rand() % screenW); sy = -m;             break; // 위
        case 1:  sx = (float)(rand() % screenW); sy = screenH + m;    break; // 아래
        case 2:  sx = -m;            sy = (float)(rand() % screenH);  break; // 왼
        default: sx = screenW + m;   sy = (float)(rand() % screenH);  break; // 오른
        }
        MiniTri t;
        t.x = sx;  t.y = sy;
        float dx = px - sx, dy = py - sy;
        float d  = std::sqrt(dx*dx + dy*dy) + 1e-3f;
        float sp = MINI_SPEED * (0.6f + (float)(rand() % 60) * 0.01f);
        t.vx = dx / d * sp;  t.vy = dy / d * sp;
        t.angle = atan2f(dy, dx);
        minis.push_back(t);
    }

    // 본체 주변에 소량 미니 세모 분출 (잔상 파괴 이벤트용)
    void burstMiniAt(float cx, float cy, int n) {
        for (int i = 0; i < n; i++) {
            float a = (float)i / (float)n * 6.2831853f + (float)(rand()%100)*0.01f;
            MiniTri t; t.x = cx; t.y = cy;
            float sp = MINI_SPEED * 0.7f;
            t.vx = cosf(a) * sp; t.vy = sinf(a) * sp; t.angle = a;
            minis.push_back(t);
        }
    }
    Decoy makeDecoy() {
        Decoy d;
        float m = 0.18f;
        d.x = screenW * (m + (float)(rand()%1000)*0.001f * (1.0f - 2.0f*m));
        d.y = screenH * (m + (float)(rand()%1000)*0.001f * (1.0f - 2.0f*m));
        d.alive = true; d.respawn = 0.0f;
        return d;
    }
    // 잔상이 맞으면 '이벤트': 글리치 노이즈 + 본체/잔상 전부 재배치 + 미니 분출. (일점사 방지)
    void onDecoyDestroyed(float dx, float dy) {
        decoyFlash = 1.0f; glitchAmount = 0.6f; burstFlash = 0.7f; textNoise = 0.8f;
        burstMiniAt(dx, dy, 7);
        hideInCorner();                       // 본체 순간이동
        driftInit = false;                    // 새 드리프트 목표 강제
        for (auto& d : decoys) d = makeDecoy();
    }
    // main 의 총알 충돌이 호출 — 점이 살아있는 잔상에 닿으면 파괴+이벤트 (true 반환)
    bool tryHitDecoy(float bx, float by) {
        for (auto& d : decoys) {
            if (!d.alive) continue;
            float dx = bx - d.x, dy = by - d.y;
            if (dx*dx + dy*dy < BODY * BODY) {
                d.alive = false; d.respawn = 1.4f;
                onDecoyDestroyed(d.x, d.y);
                return true;
            }
        }
        return false;
    }

    // 점-선분 최단거리 (레이저 판정)
    static float segDist(float px, float py, float ax, float ay, float bx, float by) {
        float abx = bx - ax, aby = by - ay;
        float l2 = abx*abx + aby*aby, t = 0.0f;
        if (l2 > 1e-6f) { t = ((px-ax)*abx + (py-ay)*aby) / l2; t = t<0?0:(t>1?1:t); }
        float cx = ax + abx*t, cy = ay + aby*t;
        float dx = px - cx, dy = py - cy;
        return std::sqrt(dx*dx + dy*dy);
    }

    void Update(float playerCX, float playerCY, float dt, float& playerHP) {
        if (!alive) return;
        stateTimer += dt;

        // 페이즈2 진입 (HP 50% 이하)
        if (!phase2 && hp <= maxHp * 0.5f) phase2 = true;

        // 효과 자연 감쇠
        burstFlash = std::max(0.0f, burstFlash - dt * 3.5f);
        textNoise  = std::max(0.0f, textNoise  - dt * 2.0f);
        decoyFlash = std::max(0.0f, decoyFlash - dt * 2.5f);

        // ── 느린 드리프트 이동 (고정형 일점사 방지) ──
        driftTimer -= dt;
        float dmarg = BODY + 80.0f;
        if (!driftInit || driftTimer <= 0.0f) {
            driftInit = true;
            driftTimer = 1.8f + (float)(rand()%150)*0.01f;
            int rx = screenW - (int)(2.0f*dmarg); if (rx < 1) rx = 1;
            int ry = screenH - (int)(2.0f*dmarg); if (ry < 1) ry = 1;
            driftTX = dmarg + (float)(rand()%rx);
            driftTY = dmarg + (float)(rand()%ry);
        }
        {
            float mdx = driftTX - worldX, mdy = driftTY - worldY;
            float md  = std::sqrt(mdx*mdx + mdy*mdy);
            float spd = DRIFT_SPD * (phase2 ? 1.4f : 1.0f);
            if (md > 1.0f) { float step = spd*dt; if (step>md) step=md;
                worldX += mdx/md*step; worldY += mdy/md*step; }
        }

        // ── 페이즈2 잔상(디코이) 유지/부활 ──
        if (phase2) {
            while ((int)decoys.size() < 2) decoys.push_back(makeDecoy());
            for (auto& d : decoys) {
                if (!d.alive) { d.respawn -= dt; if (d.respawn <= 0.0f) d = makeDecoy(); }
            }
        } else decoys.clear();

        // ── 신규 스킬: 글리치 노바 — 주기적으로 본체에서 방사형 미니 세모 ──
        novaTimer += dt;
        if (novaTimer >= (phase2 ? NOVA_INT * 0.65f : NOVA_INT)) {
            novaTimer = 0.0f;
            glitchAmount = std::max(glitchAmount, 0.35f);
            for (int i = 0; i < NOVA_N; i++) {
                float a = (float)i / (float)NOVA_N * 6.2831853f;
                MiniTri t; t.x = worldX; t.y = worldY;
                t.vx = cosf(a) * MINI_SPEED * 0.55f;
                t.vy = sinf(a) * MINI_SPEED * 0.55f; t.angle = a;
                minis.push_back(t);
            }
        }

        switch (state) {
        case BossState::GLITCH_WARNING:
            glitchAmount = 0.30f + 0.30f * sinf(stateTimer * 14.0f);  // 깜빡 찢김
            textNoise    = 0.7f;
            if (stateTimer >= T_WARNING) {
                glitchAmount = 0.0f;
                hideInCorner();
                state = BossState::SPAWN_MINI; stateTimer = 0.0f;
            }
            break;

        case BossState::SPAWN_MINI:
            glitchAmount = phase2 ? 0.14f : 0.05f;   // 페이즈2: 상시 글리치 강화
            spawnAccum += dt;
            if (spawnAccum >= (phase2 ? 0.6f : 0.8f)) {  // 페이즈2: 더 자주·더 많이
                spawnAccum = 0.0f;
                int n = phase2 ? 5 : 2;
                for (int i = 0; i < n; i++) spawnMini(playerCX, playerCY);
            }
            // 발사 0.8초 전: 레이저 방향 락 + 경고선 표시 (조준선 == 실제 발사선)
            if (!laserWarn && stateTimer >= T_SPAWN - LASER_WARN_LEAD) {
                float dx = playerCX - worldX, dy = playerCY - worldY;
                float d  = std::sqrt(dx*dx + dy*dy) + 1e-3f;
                laserDirX = dx / d; laserDirY = dy / d;
                laserWarn = true;
            }
            if (stateTimer >= T_SPAWN) {
                // ── "펑!" BURST 돌입 (락된 방향으로 레이저 발사) ──
                state = BossState::BURST_ATTACK; stateTimer = 0.0f;
                burstFlash = 1.0f; textNoise = 1.0f;
                glitchAmount = 0.5f;                          // 레이저 쏠 때 글리치 재발동
                for (auto& t : minis) t.homing = true;       // 전부 유도 가속
                laserWarn   = false;
                laserActive = true;                          // 방향은 경고 때 락된 값 사용
                // 페이즈2: 두 번째 레이저 — 본체에서 +28° 벌어진 트윈 빔(플레이어 쪽으로)
                {
                    float c = cosf(0.49f), s = sinf(0.49f);
                    laser2DirX = laserDirX * c - laserDirY * s;
                    laser2DirY = laserDirX * s + laserDirY * c;
                }
            }
            break;

        case BossState::BURST_ATTACK:
            // 레이저 발사 동안 약한 글리치가 서서히 잦아듦
            glitchAmount = 0.5f * (1.0f - stateTimer / T_BURST);
            if (stateTimer >= T_BURST) {
                state = BossState::COOLDOWN; stateTimer = 0.0f;
                laserActive = false;
            }
            break;

        case BossState::COOLDOWN:
            glitchAmount = phase2 ? 0.08f : 0.0f;
            if (stateTimer >= (phase2 ? T_COOL * 0.5f : T_COOL)) {  // 페이즈2: 회복 짧게
                for (auto& t : minis) t.homing = false;
                state = BossState::SPAWN_MINI; stateTimer = 0.0f;
            }
            break;
        }

        // ── 작은 세모 업데이트 ──
        for (auto& t : minis) {
            if (!t.alive) continue;
            if (t.homing) {  // 플레이어로 유도 가속
                float dx = playerCX - t.x, dy = playerCY - t.y;
                float d  = std::sqrt(dx*dx + dy*dy) + 1e-3f;
                t.vx += dx / d * MINI_BURST_ACCEL * dt;
                t.vy += dy / d * MINI_BURST_ACCEL * dt;
            }
            t.x += t.vx * dt;
            t.y += t.vy * dt;
            t.angle += dt * 9.0f;
            // 플레이어 접촉 → 데미지 0.5배 후 소멸
            float dx = playerCX - t.x, dy = playerCY - t.y;
            if (dx*dx + dy*dy < MINI_HIT_R * MINI_HIT_R) {
                playerHP -= MINI_DMG;
                t.alive = false;
            }
            if (t.x < -150 || t.x > screenW + 150 ||
                t.y < -150 || t.y > screenH + 150) t.alive = false;
        }
        minis.erase(
            std::remove_if(minis.begin(), minis.end(),
                [](const MiniTri& t){ return !t.alive; }),
            minis.end());

        // ── 레이저 포격 (BURST 동안) ── 페이즈2 는 직교 두 번째 레이저까지
        if (laserActive) {
            float reach = (float)(screenW + screenH);
            float ex = worldX + laserDirX * reach, ey = worldY + laserDirY * reach;
            if (segDist(playerCX, playerCY, worldX, worldY, ex, ey) < 32.0f)
                playerHP -= 45.0f * dt;
            if (phase2) {
                // 본체에서 바깥으로 나가는 단방향 트윈 빔 (관통 십자 X)
                float ex2 = worldX + laser2DirX * reach, ey2 = worldY + laser2DirY * reach;
                if (segDist(playerCX, playerCY, worldX, worldY, ex2, ey2) < 30.0f)
                    playerHP -= 40.0f * dt;
            }
        }
    }
};
