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
    static constexpr float LASER_WARN_LEAD = 0.8f;   // 발사 0.8초 전부터 경고

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

        // 효과 자연 감쇠
        burstFlash = std::max(0.0f, burstFlash - dt * 3.5f);
        textNoise  = std::max(0.0f, textNoise  - dt * 2.0f);

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
            glitchAmount = 0.05f;
            spawnAccum += dt;
            if (spawnAccum >= 0.8f) {             // '뚝뚝' 끊기듯 소량 스폰 (양 대폭 감소)
                spawnAccum = 0.0f;
                for (int i = 0; i < 2; i++) spawnMini(playerCX, playerCY);
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
            glitchAmount = 0.0f;
            if (stateTimer >= T_COOL) {
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

        // ── 레이저 포격 (BURST 동안) ──
        if (laserActive) {
            float ex = worldX + laserDirX * (float)(screenW + screenH);
            float ey = worldY + laserDirY * (float)(screenW + screenH);
            if (segDist(playerCX, playerCY, worldX, worldY, ex, ey) < 32.0f)
                playerHP -= 45.0f * dt;
        }
    }
};
