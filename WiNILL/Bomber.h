#pragma once
#include <glm/glm.hpp>
#include <cmath>
#include <cstdlib>

// ─────────────────────────────────────────────────────────────
// 자폭병 (Bomber)
//   - 잡몹보다 크고 느림. 가까이 가면 1.5초 점화 후 폭발 (크리퍼 식)
//   - 점화 중에는 멈추고 흰 ↔ 빨강 점멸
//   - 폭발 반경 안 플레이어에게 데미지
//   - 5각형 모양
// ─────────────────────────────────────────────────────────────
class Bomber {
public:
    float worldX, worldY;
    float speed = 80.0f;           // 잡몹(150)보다 느림
    float hp    = 150.0f;          // 리밸런스: 75 → 150 (잡몹의 3배)
    bool  alive              = true;
    bool  exploded           = false;  // 파티클 스폰 1회 플래그
    bool  scored             = false;  // 처치 보상 지급 완료 플래그
    bool  hackBlastPending   = false;  // HACK_BOMBER 시각효과 트리거 (CollisionSystem → main)

    // 점화 / 폭발 상태
    bool  arming    = false;
    float armTimer  = 0.0f;
    static constexpr float ARM_TIME       = 1.0f;
    static constexpr float TRIGGER_RADIUS = 80.0f;
    static constexpr float BLAST_RADIUS_BASE = 110.0f;
    static constexpr float BLAST_DAMAGE   = 40.0f;
    static constexpr float SIZE_PX        = 30.0f;

    // 인스턴스별 폭발 반경 (디버프로 ×1.5 등)
    float blastRadius = BLAST_RADIUS_BASE;

    glm::vec3 color = glm::vec3(0.90f, 0.90f, 0.90f);

    Bomber(float sx, float sy,
           float hpMul = 1.0f, float speedMul = 1.0f, float blastMul = 1.0f)
        : worldX(sx), worldY(sy)
    {
        hp    *= hpMul;
        speed *= speedMul;
        blastRadius *= blastMul;
    }

    void Update(float playerCX, float playerCY, float dt,
                float& playerHP, float speedMult = 1.0f)
    {
        if (!alive) return;

        float dx = playerCX - worldX;
        float dy = playerCY - worldY;
        float dist = std::sqrt(dx*dx + dy*dy);

        if (!arming && dist < TRIGGER_RADIUS) {
            arming   = true;
            armTimer = 0.0f;
        }

        if (arming) {
            armTimer += dt;
            // 점멸 색상: 처음 천천히 → 끝에 매우 빠르게
            float speed01 = 2.0f + armTimer * 6.0f;        // 2 → 11 Hz
            float t = std::fmod(armTimer * speed01, 1.0f);
            float lerp = (t < 0.5f) ? (t * 2.0f) : (2.0f - t * 2.0f);
            color = glm::vec3(1.0f, 1.0f - lerp * 0.85f, 1.0f - lerp * 0.85f);

            if (armTimer >= ARM_TIME) {
                // 폭발 — 반경 안 플레이어 데미지
                if (dist < blastRadius) {
                    playerHP -= BLAST_DAMAGE;
                }
                alive = false;
            }
            // 점화 중에는 멈춤 (크리퍼 식)
        } else if (dist > 5.0f) {
            worldX += (dx / dist) * speed * speedMult * dt;
            worldY += (dy / dist) * speed * speedMult * dt;
        }
    }
};
