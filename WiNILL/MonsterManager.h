#pragma once
#include <vector>
#include <algorithm>
#include <cstdlib>
#include "Monster.h"
#include "RangedMob.h"
#include "Bomber.h"
#include "Boss.h"

class MonsterManager {
public:
    std::vector<Monster*>   monsters;
    std::vector<RangedMob*> rangedMobs;
    std::vector<Bomber*>    bombers;
    Boss*                   boss = nullptr;   // 단일 보스 (검객) — 추후 vector 로 확장 가능

    ~MonsterManager() { Clear(); }

    // 스폰 영역의 모서리에서 무작위 한 점 (aX,aY = 영역 원점 / aW,aH = 영역 크기)
    //   폴리모프 2페이즈처럼 줌아웃되면 확장된 영역 모서리에서 스폰하도록 영역을 넘김
    static void EdgePoint(float aX, float aY, int aW, int aH, float& sx, float& sy) {
        if (aW < 1) aW = 1; if (aH < 1) aH = 1;
        float padding = 150.0f;
        switch (rand() % 4) {
        case 0:  sx = aX + (float)(rand() % aW); sy = aY - padding;             break;
        case 1:  sx = aX + (float)(rand() % aW); sy = aY + aH + padding;        break;
        case 2:  sx = aX - padding;              sy = aY + (float)(rand() % aH);break;
        default: sx = aX + aW + padding;         sy = aY + (float)(rand() % aH);break;
        }
    }

    // varietyPct: 특수 잡몹(돌진/회피/거대) 으로 스폰될 확률(%). 점수 비례로 main 이 전달.
    void SpawnMob(int screenW, int screenH, int cap = 100, float hpMul = 1.0f,
                  float aX = 0.0f, float aY = 0.0f, int aW = -1, int aH = -1,
                  int varietyPct = 0, int elitePct = 0) {
        if ((int)monsters.size() >= cap) return;
        if (aW < 0) aW = screenW; if (aH < 0) aH = screenH;
        float sx, sy; EdgePoint(aX, aY, aW, aH, sx, sy);
        Monster* nm = new Monster(sx, sy, hpMul);
        if (varietyPct > 0 && (rand() % 100) < varietyPct) {
            int r = rand() % 10;                       // 40% 돌진 / 40% 회피 / 20% 거대
            MobKind k = (r < 4) ? MobKind::CHARGER
                      : (r < 8) ? MobKind::WEAVER
                                : MobKind::BRUTE;
            nm->MakeKind(k);
        }
        // 엘리트 변종 (드물게) — 신속/강인 (폭발성 e==3 제거: 빨강 자폭류 빼달라 요청)
        if (elitePct > 0 && (rand() % 100) < elitePct)
            nm->MakeElite(1 + rand() % 2);
        monsters.push_back(nm);
    }

    // hpMult: 원거리 몹 HP 배율 (디버프). maxCount: 최대 동시 존재량
    void SpawnRangedMob(int screenW, int screenH,
                        float hpMult = 1.0f, int maxCount = 5,
                        float aX = 0.0f, float aY = 0.0f, int aW = -1, int aH = -1) {
        if ((int)rangedMobs.size() >= maxCount) return;
        if (aW < 0) aW = screenW; if (aH < 0) aH = screenH;
        float sx, sy; EdgePoint(aX, aY, aW, aH, sx, sy);
        RangedMob* rm = new RangedMob(sx, sy, screenW, screenH);
        rm->hp *= hpMult;
        rangedMobs.push_back(rm);
    }

    // 자폭병 — 영역 가장자리에서 spawn (디버프 mult 전달). cap: 동시 존재 상한
    void SpawnBomber(int screenW, int screenH,
                     float hpMul = 1.0f, float speedMul = 1.0f, float blastMul = 1.0f,
                     float aX = 0.0f, float aY = 0.0f, int aW = -1, int aH = -1,
                     int cap = 30) {
        if ((int)bombers.size() >= cap) return;
        if (aW < 0) aW = screenW; if (aH < 0) aH = screenH;
        float sx, sy; EdgePoint(aX, aY, aW, aH, sx, sy);
        bombers.push_back(new Bomber(sx, sy, hpMul, speedMul, blastMul));
    }

    // mobSpeedMult: 잡몹 추가 속도 배율 (디버프)
    // rmobMoveMult : 원거리 몹 lerp 가속 (rmobDelayMult <1 → 더 빠름 → moveMult >1)
    void UpdateAll(float playerCX, float playerCY, float dt,
                   float& playerHP, std::vector<Bullet>& bullets,
                   float mobSpeedMult = 1.0f, float rmobMoveMult = 1.0f) {
        for (auto m : monsters)
            m->Update(playerCX, playerCY, dt, playerHP, mobSpeedMult);

        // ── 소환체(SPAWNER) — 주기마다 작은 잡몹 2마리 (전체 몹 수 제한) ──
        {
            std::vector<Monster*> born;
            int total = (int)monsters.size();
            for (auto m : monsters) {
                if (!m->alive || m->kind != MobKind::SPAWNER) continue;
                m->spawnTimer -= dt;
                if (m->spawnTimer <= 0.0f && total + (int)born.size() < 140) {
                    m->spawnTimer = 3.0f;
                    for (int s = 0; s < 2; s++) {
                        float a = (float)(rand() % 628) * 0.01f;
                        born.push_back(new Monster(m->worldX + cosf(a) * 32.0f,
                                                   m->worldY + sinf(a) * 32.0f, 0.5f));
                    }
                }
            }
            for (auto* b : born) monsters.push_back(b);
        }

        // ── 소프트 콜리전 (잡몹 간) ──
        //   너무 가까우면 서로 밀어내고, 4명 이상에게 밀리면 압사 데미지
        //   summoned 몹은 더 큰 반경 (소환물 더 큼)
        const float MIN_GAP_NORM = 24.0f;
        const float MIN_GAP_SUMM = 32.0f;
        const float CRUSH_DPS    = 50.0f;
        for (size_t i = 0; i < monsters.size(); i++) {
            if (!monsters[i]->alive) continue;
            int pushCount = 0;
            float radi = monsters[i]->summoned ? MIN_GAP_SUMM : MIN_GAP_NORM;
            for (size_t j = 0; j < monsters.size(); j++) {
                if (i == j) continue;
                if (!monsters[j]->alive) continue;
                float dx = monsters[j]->worldX - monsters[i]->worldX;
                float dy = monsters[j]->worldY - monsters[i]->worldY;
                float d2 = dx*dx + dy*dy;
                float radj = monsters[j]->summoned ? MIN_GAP_SUMM : MIN_GAP_NORM;
                float minD = (radi + radj) * 0.5f;
                if (d2 > 0.0001f && d2 < minD * minD) {
                    float d  = std::sqrt(d2);
                    float overlap = (minD - d) * 0.5f;
                    monsters[i]->worldX -= (dx / d) * overlap * 0.5f;
                    monsters[i]->worldY -= (dy / d) * overlap * 0.5f;
                    pushCount++;
                }
            }
            // 4+ 이웃에 끼이면 압사 (디도스는 물량 swarm 정체성이라 압사 면제)
            if (pushCount >= 4 && monsters[i]->kind != MobKind::DDOS) {
                monsters[i]->hp -= CRUSH_DPS * dt;
                if (monsters[i]->hp <= 0.0f) monsters[i]->alive = false;
            }
        }

        monsters.erase(
            std::remove_if(monsters.begin(), monsters.end(),
                [](Monster* m) { if (!m->alive) { delete m; return true; } return false; }),
            monsters.end());

        for (auto r : rangedMobs)
            r->Update(playerCX, playerCY, dt, bullets, rmobMoveMult);
        // deathScale 이 0 이하가 돼야 실제 삭제 (사망 애니메이션 완료 후)
        rangedMobs.erase(
            std::remove_if(rangedMobs.begin(), rangedMobs.end(),
                [](RangedMob* r) {
                    if (!r->alive && r->deathScale <= 0.0f) { delete r; return true; }
                    return false;
                }),
            rangedMobs.end());

        for (auto b : bombers)
            b->Update(playerCX, playerCY, dt, playerHP, mobSpeedMult);

        // ── 자폭병 소프트 콜리전 — 자폭병끼리 + 잡몹과도 분리 ──
        //   예전엔 자폭병이 서로 겹쳐 쌓여 "한 마리"처럼 보였고, 들어갔다가
        //   겹친 다수가 동시 폭발 → 즉사하던 버그. 잡몹처럼 서로 밀어내 개체 구분.
        {
            const float BOMB_GAP = Bomber::SIZE_PX * 0.95f;
            for (size_t i = 0; i < bombers.size(); i++) {
                if (!bombers[i]->alive) continue;
                // 자폭병끼리 (서로 균등하게 밀어냄)
                for (size_t j = i + 1; j < bombers.size(); j++) {
                    if (!bombers[j]->alive) continue;
                    float dx = bombers[j]->worldX - bombers[i]->worldX;
                    float dy = bombers[j]->worldY - bombers[i]->worldY;
                    float d2 = dx*dx + dy*dy;
                    if (d2 > 0.0001f && d2 < BOMB_GAP * BOMB_GAP) {
                        float d = std::sqrt(d2);
                        float push = (BOMB_GAP - d) * 0.5f;
                        float nx = dx / d, ny = dy / d;
                        bombers[i]->worldX -= nx * push; bombers[i]->worldY -= ny * push;
                        bombers[j]->worldX += nx * push; bombers[j]->worldY += ny * push;
                    }
                }
                // 자폭병 vs 잡몹 (자폭병만 비켜남 — 잡몹 추격 흐름은 유지)
                float minDM = (BOMB_GAP + MIN_GAP_NORM) * 0.5f;
                for (size_t j = 0; j < monsters.size(); j++) {
                    if (!monsters[j]->alive) continue;
                    float dx = monsters[j]->worldX - bombers[i]->worldX;
                    float dy = monsters[j]->worldY - bombers[i]->worldY;
                    float d2 = dx*dx + dy*dy;
                    if (d2 > 0.0001f && d2 < minDM * minDM) {
                        float d = std::sqrt(d2);
                        float push = (minDM - d);
                        bombers[i]->worldX -= (dx / d) * push;
                        bombers[i]->worldY -= (dy / d) * push;
                    }
                }
            }
        }
        // 죽은 자폭병은 여기서 삭제하지 않음 — main.cpp 의 VFX 체크 후 ClearDeadBombers() 호출

        // 보스 (단일) — 소환물은 monsters 에 그대로 push
        if (boss && boss->alive) {
            std::vector<Monster*> newSummons;
            boss->Update(playerCX, playerCY, dt, playerHP, newSummons);
            for (auto m : newSummons) monsters.push_back(m);
        }
        // 보스가 죽은 경우는 main 에서 보상 처리 후 직접 nullify
    }

    // main.cpp 의 VFX 처리 후 호출 — 죽은 자폭병 실제 삭제
    void ClearDeadBombers() {
        bombers.erase(
            std::remove_if(bombers.begin(), bombers.end(),
                [](Bomber* b) { if (!b->alive) { delete b; return true; } return false; }),
            bombers.end());
    }

    void Clear() {
        for (auto m : monsters) delete m;
        monsters.clear();
        for (auto r : rangedMobs) delete r;
        rangedMobs.clear();
        for (auto b : bombers) delete b;
        bombers.clear();
        if (boss) { delete boss; boss = nullptr; }
    }
};
