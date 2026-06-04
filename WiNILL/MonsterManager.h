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
                  int varietyPct = 0) {
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

    // 자폭병 — 영역 가장자리에서 spawn (디버프 mult 전달)
    void SpawnBomber(int screenW, int screenH,
                     float hpMul = 1.0f, float speedMul = 1.0f, float blastMul = 1.0f,
                     float aX = 0.0f, float aY = 0.0f, int aW = -1, int aH = -1) {
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
            // 4+ 이웃에 끼이면 압사
            if (pushCount >= 4) {
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
