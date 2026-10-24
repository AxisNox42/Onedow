#pragma once
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include "Monster.h"
#include "RangedMob.h"
#include "SpaceRules.h"

class MonsterManager {
public:
    std::vector<Monster*>   monsters;
    std::vector<RangedMob*> rangedMobs;

    // Afterglow of an instant Regulus shield laser: fixed endpoints captured
    // at the shot, purely visual (the shield is already granted).
    struct RegulusBeam {
        float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f;
        float age = 0.0f;
    };
    std::vector<RegulusBeam> regulusBeams;

    // source: the NORMA window space the hit comes from (SpaceRules.h).
    // Player weapons default to the player's space; a target in another
    // space takes nothing.
    float ApplyDamage(Monster& target, float damage,
                      int source = space::kPlayerSource) {
        if (!space::Reaches(source, target.worldX, target.worldY)) return 0.0f;
        const float dealt = target.ApplyDamage(damage);
        HandleShieldBreak(target);
        return dealt;
    }

    float ApplyDamage(RangedMob& target, float damage,
                      int source = space::kPlayerSource) {
        if (!space::Reaches(source, target.worldX, target.worldY)) return 0.0f;
        const float dealt = target.ApplyDamage(damage);
        HandleShieldBreak(target);
        return dealt;
    }

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

    void SpawnMob(int screenW, int screenH, int cap = 100, float hpMul = 1.0f,
                  float aX = 0.0f, float aY = 0.0f, int aW = -1, int aH = -1) {
        if ((int)monsters.size() >= cap) return;
        if (aW < 0) aW = screenW; if (aH < 0) aH = screenH;
        float sx, sy; EdgePoint(aX, aY, aW, aH, sx, sy);
        Monster* nm = new Monster(sx, sy, hpMul);
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

    // Nearest living enemy to (fromX, fromY), anywhere on the field.
    bool FindNearestEnemy(float fromX, float fromY,
                          float& targetX, float& targetY) const {
        float nearestDistanceSq = 1e18f;
        bool found = false;
        auto consider = [&](float x, float y) {
            const float dx = x - fromX;
            const float dy = y - fromY;
            const float distanceSq = dx * dx + dy * dy;
            if (distanceSq < nearestDistanceSq) {
                nearestDistanceSq = distanceSq;
                targetX = x;
                targetY = y;
                found = true;
            }
        };
        for (const auto* monster : monsters)
            if (monster->alive) consider(monster->worldX, monster->worldY);
        for (const auto* ranged : rangedMobs)
            if (ranged->alive) consider(ranged->worldX, ranged->worldY);
        return found;
    }

    // Higher-tier enemies keep their position when colliding with lower-tier
    // enemies. The lighter enemy receives the separation displacement instead
    // of making the important target visibly jitter or stall.
    static int CollisionTier(const Monster* m) {
        if (!m) return 0;
        int tier = 0;
        switch (m->kind) {
        case MobKind::NORMA:    tier = 5; break;   // nothing pushes the boss
        case MobKind::GENESIS:  tier = 4; break;
        case MobKind::GRAVIS:   tier = 3; break;
        case MobKind::QUASAR:   tier = 4; break;
        case MobKind::REGULUS:  tier = 3; break;
        case MobKind::MAGNETAR: tier = 3; break;
        case MobKind::GIMBAL:   tier = 2; break;
        case MobKind::ANTARES:  tier = 2; break;
        default:                tier = 0; break;
        }
        return tier;
    }

    void UpdateDeathEffects(float delta) {
        for (auto* mob : monsters) mob->UpdateDeathEffect(delta);
    }

    // mobSpeedMult: 잡몹 추가 속도 배율 (디버프)
    // rmobMoveMult : 원거리 몹 lerp 가속 (rmobDelayMult <1 → 더 빠름 → moveMult >1)
    void UpdateAll(float playerCX, float playerCY, float dt,
                   float& playerHP, std::vector<Bullet>& bullets,
                   float mobSpeedMult = 1.0f, float rmobMoveMult = 1.0f,
                   float rotorHpMult = 1.0f,
                   float gateWX = -1.0f, float gateWY = -1.0f,
                   float gateWW = -1.0f, float gateWH = -1.0f,
                   const glm::vec4& attackBounds = glm::vec4(0.0f)) {
        for (auto m : monsters) {
            // Whatever this mob does to the player only lands in its own space.
            space::HurtSourceScope hurtSource(space::SpaceAt(m->worldX, m->worldY));
            m->Update(playerCX, playerCY, dt, playerHP, mobSpeedMult,
                      gateWX, gateWY, gateWW, gateWH, &bullets, attackBounds);
            if(m->kind==MobKind::GENESIS && !m->alive)
                for(auto* child:monsters) if(child->thrownOwnerId==m->genesisId && child->genesisRotorPhase==2) {
                    child->genesisRotorPhase=3; child->thrownTimer=0;
                    for(auto& p:child->antaresTrail) p=glm::vec3(child->worldX,child->worldY,child->thrownSpin);
                }
        }

        for (auto r : rangedMobs) {
            space::HurtSourceScope hurtSource(space::SpaceAt(r->worldX, r->worldY));
            r->Update(playerCX, playerCY, dt, bullets, rmobMoveMult);
        }
        ProcessPendingShieldBreaks();

        // rotorHpMult is the Rotor-only HP upgrade; Genesis children scale
        // with the Genesis' own spawn ramp instead.
        (void)rotorHpMult;
        UpdateGenesisLaunchers(dt, playerCX, playerCY);

        // ── 소프트 콜리전 (잡몹 간) ──
        //   너무 가까우면 서로 밀어내고, 4명 이상에게 밀리면 압사 데미지
        //   summoned 몹은 더 큰 반경 (소환물 더 큼)
        ResolveMonsterCrowding(dt);

        UpdateRegulusShots(dt);
        UpdateRegulusBeams(dt);
        // Death animations (Quasar supernova) run on the gameplay clock; a
        // dead mob stays until its animation ends.
        UpdateDeathEffects(dt);

        monsters.erase(
            std::remove_if(monsters.begin(), monsters.end(),
                [](Monster* m) {
                    if (!m->alive && m->scored && m->exploded &&
                        !m->DeathEffectVisible() && !m->dustPending) {
                        delete m;
                        return true;
                    }
                    return false;
                }),
            monsters.end());

        // Keep an unprocessed death until rewards and death particles exist.
        rangedMobs.erase(
            std::remove_if(rangedMobs.begin(), rangedMobs.end(),
                [](RangedMob* r) {
                    if (!r->alive && r->scored && r->exploded && r->deathScale <= 0.0f) {
                        delete r; return true;
                    }
                    return false;
                }),
            rangedMobs.end());

    }

    void Clear() {
        regulusBeams.clear();
        for (auto m : monsters) delete m;
        monsters.clear();
        for (auto r : rangedMobs) delete r;
        rangedMobs.clear();
    }

private:
    static float RegulusTargetTier(const Monster& mob) {
        switch (mob.kind) {
        case MobKind::GENESIS: return 4.0f;
        case MobKind::GRAVIS:
        case MobKind::QUASAR:  return 3.0f;
        case MobKind::MAGNETAR: return 3.0f;
        case MobKind::GIMBAL:
        case MobKind::ANTARES: return 2.0f;
        case MobKind::ROTOR:
        case MobKind::SWARM:   return 1.0f;
        case MobKind::REGULUS: return -1.0f;
        }
        return 0.0f;
    }

    // Granted shields are independent of their Regulus, so a break only
    // clears the pending flag (particles are spawned at the hit).
    void HandleShieldBreak(Monster& target) {
        target.regulusShield.breakPending = false;
    }

    void HandleShieldBreak(RangedMob& target) {
        target.regulusShield.breakPending = false;
    }

    void ProcessPendingShieldBreaks() {
        for (auto* mob : monsters) HandleShieldBreak(*mob);
        for (auto* mob : rangedMobs) HandleShieldBreak(*mob);
    }

    static bool RegulusEligible(const EnemyShieldState& shield) {
        return !shield.acquiredOnce && shield.hp <= 0.0f;
    }

    // Every REGULUS_SHOT_INTERVAL a Regulus fires one instant laser at an
    // ally picked uniformly at random among eligible enemies within
    // REGULUS_SHOT_RANGE. The shield is granted on the spot.
    void UpdateRegulusShots(float dt) {
        constexpr float rangeSq = Monster::REGULUS_SHOT_RANGE *
                                  Monster::REGULUS_SHOT_RANGE;
        std::vector<Monster*> monsterPool;
        std::vector<RangedMob*> rangedPool;
        for (auto* source : monsters) {
            if (!source->alive || source->kind != MobKind::REGULUS) continue;
            source->regulusShotTimer -= dt;
            if (source->regulusShotTimer > 0.0f) continue;

            monsterPool.clear();
            rangedPool.clear();
            // Regulus does not see past a window border, so it never aims
            // a shield laser into another space.
            const int sourceSpace = space::SpaceAt(source->worldX, source->worldY);
            const auto inRange = [&](float x, float y) {
                const float dx = x - source->worldX, dy = y - source->worldY;
                return dx * dx + dy * dy <= rangeSq && space::Reaches(sourceSpace, x, y);
            };
            for (auto* target : monsters)
                if (target->alive && target->kind != MobKind::REGULUS &&
                    target->kind != MobKind::NORMA &&
                    RegulusEligible(target->regulusShield) &&
                    inRange(target->worldX, target->worldY))
                    monsterPool.push_back(target);
            for (auto* target : rangedMobs)
                if (target->alive && RegulusEligible(target->regulusShield) &&
                    inRange(target->worldX, target->worldY))
                    rangedPool.push_back(target);

            const int total = (int)(monsterPool.size() + rangedPool.size());
            if (total == 0) {
                source->regulusShotTimer = 0.2f;   // look again shortly
                continue;
            }
            const int pick = rand() % total;
            EnemyShieldState* shield = nullptr;
            float tx = 0.0f, ty = 0.0f, bodyRadius = 18.0f;
            if (pick < (int)monsterPool.size()) {
                Monster* t = monsterPool[pick];
                shield = &t->regulusShield;
                tx = t->worldX; ty = t->worldY;
                bodyRadius = (t->summoned ? 28.0f : 18.0f) * t->sizeScale;
            } else {
                RangedMob* t = rangedPool[pick - monsterPool.size()];
                shield = &t->regulusShield;
                tx = t->worldX; ty = t->worldY;
                bodyRadius = RangedMob::VISUAL_BASE_PX;
            }
            const float shieldHp = std::max(1.0f, source->regulusBaseHp) * 0.6f;
            *shield = {};
            shield->acquiredOnce = true;
            shield->hp = shield->maxHp = shieldHp;
            shield->flashTimer = MOB_HIT_FLASH_TIME;
            shield->grantTimer = SHIELD_GRANT_TIME;
            SpawnRegulusShieldGrantParticles(tx, ty, bodyRadius);
            regulusBeams.push_back({ source->worldX, source->worldY, tx, ty, 0.0f });
            source->regulusShotTimer = Monster::REGULUS_SHOT_INTERVAL;
        }
    }

    void UpdateRegulusBeams(float dt) {
        for (auto& beam : regulusBeams) beam.age += dt;
        regulusBeams.erase(std::remove_if(regulusBeams.begin(), regulusBeams.end(),
            [](const RegulusBeam& b) { return b.age >= Monster::REGULUS_BEAM_TIME; }),
            regulusBeams.end());
    }

    // Genesis shell-core cycle (Astral_Enemy_Blueprints_v3 2.3): track the
    // player while the three shells charge, lock the aim at 3/3, throw all
    // three Swarms through the muzzle gap, regroup. Thrown Swarms use their
    // own cap.
    void UpdateGenesisRotorLauncher(Monster& g,float dt,float px,float py,
                                    std::vector<Monster*>& born) {
        g.genesisRotorShotAge+=dt;
        // Current aim follows the player throughout production, including the
        // release frame. Already released units keep their own fixed heading.
        float turn=atan2f(py-g.worldY,px-g.worldX)-g.hiveOrbitAngle;
        while(turn>3.14159265f) turn-=6.2831853f;
        while(turn<-3.14159265f) turn+=6.2831853f;
        g.hiveOrbitAngle+=std::clamp(turn,-9.0f*dt,9.0f*dt);
        const float duration=g.hivePhase==4 ? Monster::GENESIS_ROTOR_PREP :
                             g.hivePhase==5 ? Monster::GENESIS_ROTOR_GAP : Monster::GENESIS_ROTOR_RECOVER;
        if(g.hivePhase==6) {
            if(g.spawnTimer>=duration) {
                g.hivePhase=0; g.spawnTimer=0; g.hiveSpawnCount=0;
                g.genesisNextRotor=false;
            }
            return;
        }
        if(!g.genesisRotorLoaded && g.spawnTimer>=duration-Monster::GENESIS_ROTOR_FORM) {
            g.genesisRotorLoaded=true; // a shot-down loaded unit is not replaced
            int own=0,total=(int)born.size();
            for(auto* m:monsters) if(m->alive) {
                ++total;
                if(m->genesisThrown && m->kind==MobKind::ROTOR && m->thrownOwnerId==g.genesisId) ++own;
            }
            for(auto* m:born) if(m->kind==MobKind::ROTOR && m->thrownOwnerId==g.genesisId) ++own;
            if(own<Monster::GENESIS_ROTOR_CAP && total<130) {
                const auto p=g.GenesisRotorFeedPoint(0);
                auto* child=new Monster(p.x,p.y);
                child->MakeKind(MobKind::ROTOR);
                child->genesisRotorFullScale=g.GenesisRotorOutputScale();
                child->sizeScale=Monster::GenesisRotorFormScale(child->genesisRotorFullScale,0);
                child->genesisThrown=true; // reuse summon caps and no-farm rewards
                child->thrownOwnerId=g.genesisId;
                child->genesisRotorPhase=1;
                born.push_back(child);
            }
        }
        const float u=std::clamp((g.spawnTimer-(duration-Monster::GENESIS_ROTOR_FORM))/
                                 Monster::GENESIS_ROTOR_FORM,0.0f,1.0f);
        bool launched=false;
        auto feed=[&](Monster* child) {
            if(!child->alive || child->thrownOwnerId!=g.genesisId || child->genesisRotorPhase!=1) return;
            const auto p=g.GenesisRotorFeedPoint(u);
            child->worldX=p.x; child->worldY=p.y;
            child->sizeScale=Monster::GenesisRotorFormScale(child->genesisRotorFullScale,u);
            child->thrownTimer=u*Monster::GENESIS_ROTOR_FORM;
            child->thrownSpin=g.hiveOrbitAngle;
            if(g.spawnTimer>=duration) {
                child->LaunchGenesisRotor(px,py,atan2f(py-g.worldY,px-g.worldX)); launched=true;
            }
        };
        for(auto* child:monsters) feed(child);
        for(auto* child:born) feed(child);
        if(g.spawnTimer<duration) return;
        ++g.hiveSpawnCount;
        if(launched) {
            g.genesisRotorShotAge=0;
            g.genesisRotorRecoilDir=glm::vec2(cosf(g.hiveOrbitAngle),sinf(g.hiveOrbitAngle));
        }
        g.genesisRotorLoaded=false;
        g.spawnTimer=0; // no catch-up burst after a long frame
        g.hivePhase=g.hiveSpawnCount>=Monster::GENESIS_ROTOR_COUNT ? 6 : 5;
    }

    void UpdateGenesisLaunchers(float dt, float playerCX, float playerCY) {
        std::vector<Monster*> born;
        const int total = (int)monsters.size();

        for (auto* m : monsters) {
            if (m->kind != MobKind::GENESIS) continue;
            if (!m->alive) {
                for(auto* child:monsters) if(child->alive && child->thrownOwnerId==m->genesisId && child->genesisRotorPhase>0) {
                    if(child->genesisRotorPhase==1) {
                        child->alive=false; child->scored=true; child->noBlast=true;
                    } else if(child->genesisRotorPhase==2) {
                        child->genesisRotorPhase=3; child->thrownTimer=0;
                        for(auto& p:child->antaresTrail) p=glm::vec3(child->worldX,child->worldY,child->thrownSpin);
                    }
                }
                continue;
            }
            m->hivePulseTimer=std::max(0.0f,m->hivePulseTimer-dt);
            m->genesisRecoil=std::max(0.0f,m->genesisRecoil-dt*4.0f);
            m->spawnTimer+=dt;
            if(m->hivePhase>=4) {
                UpdateGenesisRotorLauncher(*m,dt,playerCX,playerCY,born);
                continue;
            }
            const float toX = playerCX - m->worldX, toY = playerCY - m->worldY;
            const float toAngle = atan2f(toY, toX);

            if (m->hivePhase == 0) {
                float turn = toAngle - m->hiveOrbitAngle;
                while (turn > 3.14159265f) turn -= 6.28318531f;
                while (turn < -3.14159265f) turn += 6.28318531f;
                const float maxTurn = Monster::GENESIS_TURN_RATE * dt;
                m->hiveOrbitAngle += std::clamp(turn, -maxTurn, maxTurn);
                m->hiveOpenFactor = 0.0f;
                // The next attack owns its waiting animation. Rotor waiting
                // must never fill Swarm slots or perform the Swarm shell spin.
                if(m->genesisNextRotor) {
                    m->genesisShellSpin=0;
                    if(m->spawnTimer>=Monster::GENESIS_ROTOR_WAIT) {
                        m->hivePhase=4; m->spawnTimer=0; m->hiveSpawnCount=0;
                        m->genesisRotorLoaded=false; m->genesisRotorShotAge=1;
                    }
                    continue;
                }
                // The shell ring makes one full turn per charge, easing in
                // and out, so the muzzle gap stops on the facing at 3/3.
                const float fullTurn = 6.2831853f;
                const float u = std::min(1.0f, m->spawnTimer / Monster::GENESIS_TRACK_TIME);
                m->genesisShellSpin = m->genesisSpinBase + fullTurn * u * u * (3.0f - 2.0f * u);
                if (m->GenesisStacks() >= Monster::GENESIS_STACKS) {
                    m->genesisSpinBase = 0.0f;
                    m->genesisShellSpin = m->genesisSpinBase;
                    // Lock the aim now so the aim lines tell the truth.
                    m->hivePhase = 1;
                    m->spawnTimer = 0.0f;
                    m->hiveSpawnCount = 0;
                    m->genesisNextRotor = true;
                    m->genesisAimAngle = m->hiveOrbitAngle;
                    // Landing points, rolled once so the aim lines show
                    // exactly where each Swarm will go: the first on the
                    // player, the rest uniform over a disc around them.
                    for (int i = 0; i < 3; ++i) {
                        const float ur = (float)(rand() % 1001) / 1000.0f;
                        const float ua = (float)(rand() % 1001) / 1000.0f;
                        const float r = i == 0 ? 0.0f
                            : Monster::GENESIS_SCATTER_RADIUS * std::sqrt(ur);
                        const float a = ua * 6.2831853f;
                        m->genesisShotX[i] = playerCX + cosf(a) * r;
                        m->genesisShotY[i] = playerCY + sinf(a) * r;
                    }
                }
            } else if (m->hivePhase == 1) {
                m->hiveOpenFactor = std::min(1.0f, m->spawnTimer / Monster::GENESIS_AIM_TIME);
                if (m->spawnTimer >= Monster::GENESIS_AIM_TIME) {
                    m->hivePhase = 2;
                    // First Swarm is fed into the core, then thrown.
                    m->spawnTimer = Monster::GENESIS_THROW_GAP - Monster::GENESIS_FEED_TIME;
                }
            } else if (m->hivePhase == 2) {
                m->hiveOpenFactor = 1.0f;
                if (m->hiveSpawnCount < Monster::GENESIS_STACKS &&
                    m->spawnTimer >= Monster::GENESIS_THROW_GAP) {
                    m->spawnTimer = 0.0f;
                    int ownAlive = 0;
                    for (auto* s : monsters)
                        if (s->alive && s->genesisThrown && s->kind != MobKind::ROTOR &&
                            s->thrownOwnerId == m->genesisId)
                            ++ownAlive;
                    for (auto* s : born)
                        if (s->kind != MobKind::ROTOR && s->thrownOwnerId == m->genesisId) ++ownAlive;
                    if (ownAlive < Monster::GENESIS_THROWN_CAP &&
                        total + (int)born.size() < 130) {
                        const int shot = std::min(m->hiveSpawnCount, 2);
                        const float hullR = 18.0f * m->sizeScale * 2.05f;
                        const float mx = m->worldX + cosf(m->genesisAimAngle) * hullR * 0.9f;
                        const float my = m->worldY + sinf(m->genesisAimAngle) * hullR * 0.9f;
                        Monster* child = new Monster(mx, my, 1.0f);
                        child->MakeKind(MobKind::SWARM);
                        child->hp *= m->genesisChildHpMult;
                        child->BeginThrownFlight(m->genesisShotX[shot],
                                                 m->genesisShotY[shot]);
                        child->thrownOwnerId = m->genesisId;
                        born.push_back(child);
                    }
                    ++m->hiveSpawnCount;
                    m->hivePulseTimer = 0.22f;
                    m->genesisRecoil = 1.0f;
                }
                if (m->hiveSpawnCount >= Monster::GENESIS_STACKS &&
                    m->spawnTimer >= Monster::GENESIS_THROW_END) {
                    // Shells are empty: regroup, then charge the next volley.
                    m->hivePhase = 3;
                    m->spawnTimer = 0.0f;
                }
            } else {
                m->hiveOpenFactor = std::max(0.0f, 1.0f - m->spawnTimer /
                                                   Monster::GENESIS_RECOVER_TIME);
                if (m->spawnTimer >= Monster::GENESIS_RECOVER_TIME) {
                    m->hivePhase = 0;
                    m->spawnTimer = 0.0f;
                    m->hiveSpawnCount = 0;
                }
            }
        }
        for (auto* child : born) monsters.push_back(child);
    }

    // Position separation only; crowding never damages anyone.
    void ResolveMonsterCrowding(float /*dt*/) {
        constexpr float kNormalGap = 24.0f;
        constexpr float kSummonedGap = 32.0f;
        for (size_t i = 0; i < monsters.size(); ++i) {
            // Thrown Swarms in flight pass through other enemies.
            if (!monsters[i]->alive || monsters[i]->thrownPhase == 1 || monsters[i]->genesisRotorPhase > 0) continue;
            const int tierI = CollisionTier(monsters[i]);
            const float radiusI = monsters[i]->summoned ? kSummonedGap : kNormalGap;
            for (size_t j = i + 1; j < monsters.size(); ++j) {
                if (!monsters[j]->alive || monsters[j]->thrownPhase == 1 || monsters[j]->genesisRotorPhase > 0) continue;
                float dx = monsters[j]->worldX - monsters[i]->worldX;
                float dy = monsters[j]->worldY - monsters[i]->worldY;
                const float distanceSq = dx * dx + dy * dy;
                const float radiusJ = monsters[j]->summoned ? kSummonedGap : kNormalGap;
                const float minDistance = (radiusI + radiusJ) * 0.5f;
                if (distanceSq >= minDistance * minDistance) continue;
                float distance = std::sqrt(distanceSq);
                if (distance <= 0.0001f) {
                    dx = ((i + j) & 1) ? 1.0f : -1.0f;
                    dy = 0.0f;
                    distance = 1.0f;
                }
                const float overlap = minDistance - distance;
                const float nx = dx / distance;
                const float ny = dy / distance;
                const int tierJ = CollisionTier(monsters[j]);
                if (tierI > tierJ) {
                    monsters[j]->worldX += nx * overlap;
                    monsters[j]->worldY += ny * overlap;
                } else if (tierI < tierJ) {
                    monsters[i]->worldX -= nx * overlap;
                    monsters[i]->worldY -= ny * overlap;
                } else {
                    const float half = overlap * 0.5f;
                    monsters[i]->worldX -= nx * half;
                    monsters[i]->worldY -= ny * half;
                    monsters[j]->worldX += nx * half;
                    monsters[j]->worldY += ny * half;
                }
            }
        }
    }
};
