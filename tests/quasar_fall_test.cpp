// Run scripts/test_quasar_fall.ps1. Gameplay-level barrage regressions.
#include "MonsterManager.h"
#include <cassert>
#include <cmath>
#include <cstdio>

static constexpr float DT = 1.0f / 120.0f;
static const glm::vec4 ARENA(0, 0, 1200, 800);
static Monster MakeFall() {
    Monster q(150, 150);
    q.MakeKind(MobKind::QUASAR);
    q.speed = q.contactDmg = 0;
    return q;
}
static void Tick(Monster& q, glm::vec2 p, float dt, float& hp, glm::vec4 bounds = ARENA) {
    q.Update(p.x, p.y, dt, hp, 1, -1, -1, -1, -1, nullptr, bounds);
}
int main() {
    std::srand(1234);
    g_PlayerDmgMult = 1;
    g_PlayerShield = 0;
    // Exactly 18 shots. Each samples the current moving player, fixes its
    // point forever, and receives a full warning; only ~5 marks coexist.
    {
        auto q = MakeFall();
        q.BeginQuasarFall();
        float hp = 10000;
        std::array<glm::vec2, 18> locked{};
        int seen = 0, nearCount = 0, wideCount = 0;
        bool allHit = false;
        for (int frame = 0; frame < 900 && q.QuasarFalling(); ++frame) {
            const glm::vec2 player(450 + frame * DT * 65, 400);
            Tick(q, player, DT, hp);
            for (int i = 0; i < seen; ++i)
                assert(q.quasarFallTargets[i] == locked[i]);
            if (q.quasarFallFired > seen) {
                assert(q.quasarFallFired == seen + 1);
                const auto target = q.quasarFallTargets[seen];
                const float d = glm::length(target - player);
                assert(d <= 180.01f);
                if (d <= 90) ++nearCount; else ++wideCount;
                locked[seen] = target;
                if (seen) assert(std::fabs(q.quasarFallLaunchTimes[seen] -
                    q.quasarFallLaunchTimes[seen - 1] - 0.2f) < DT);
                ++seen;
            }
            int pending = 0;
            for (int i = 0; i < seen; ++i) {
                const bool hit = (q.quasarFallImpacts & (1u << i)) != 0;
                if (hit) assert(q.quasarTimer >= q.QuasarFallImpactTime(i) || !q.QuasarFalling());
                else ++pending;
            }
            assert(pending <= 6);
            if (q.quasarFallImpacts == (1u << 18) - 1) allHit = true;
        }
        assert(seen == 18 && nearCount > 0 && wideCount > 0 && allHit);
        assert(q.quasarFallLaunchTimes[17] - q.quasarFallLaunchTimes[0] < 3.45f);
        assert(q.quasarState == Monster::QUASAR_FALL_RECOVER_STATE);
    }
    // Isolate damage policy with known test points: no damage during travel,
    // 6 per impact, adjacent 0.2s impacts suppressed, later impacts hurt again.
    for (const bool outside : {false, true}) {
        auto q = MakeFall();
        q.BeginQuasarFall();
        float hp = 100;
        int hits = 0;
        for (int frame = 0; frame < 900 && q.QuasarFalling(); ++frame) {
            const float before = hp;
            Tick(q, {600, outside ? 335.0f : 400.0f}, DT, hp);
            // Give all newly emitted shards the same known ground location.
            for (int i = 0; i < q.quasarFallFired; ++i) q.quasarFallTargets[i] = {600, 400};
            if (q.QuasarFalling() && q.quasarFallFired && q.quasarTimer < q.QuasarFallImpactTime(0)) assert(hp == 100);
            if (hp < before) {
                assert(before - hp == 6);
                ++hits;
            }
        }
        assert(hits == (outside ? 0 : 9));
        assert(hp == (outside ? 100 : 46));
        const float after = hp;
        Tick(q, {600,400}, 0.5f, hp);
        assert(hp == after); // no residual damage
    }
    // Hitches do not dump overdue shots or consume a new shot's warning.
    {
        auto q = MakeFall();
        q.BeginQuasarFall();
        float hp = 100;
        Tick(q, {600,400}, 5, hp);
        assert(q.quasarFallFired == 1 && q.quasarFallImpacts == 0 && hp == 100);
        assert(q.QuasarFallImpactTime(0) >= q.quasarTimer + 0.999f);
        Tick(q, {600,400}, 0.1f, hp);
        assert(q.quasarFallFired == 1 && hp == 100);
        Tick(q, {600,400}, 0.1f, hp);
        assert(q.quasarFallFired == 2 && hp == 100);
    }
    // Every new target remains within the actual map, including offset maps.
    for (const glm::vec4 bounds : {ARENA, glm::vec4(500,500,640,640), glm::vec4(0,0,50,50)}) {
        MonsterManager mm;
        auto* q = new Monster(100,100);
        q->MakeKind(MobKind::QUASAR);
        q->BeginQuasarFall();
        mm.monsters.push_back(q);
        std::vector<Bullet> bullets;
        float hp = 10000;
        for (int frame = 0; frame < 620; ++frame) {
            mm.UpdateAll(bounds.x, bounds.y, DT, hp, bullets, 1,1,1,-1,-1,-1,-1,bounds);
            for (int i = 0; i < q->quasarFallFired; ++i) {
                if (!(q->quasarFallTargetsMask & (1u << i))) continue;
                const auto t = q->quasarFallTargets[i];
                assert(t.x >= bounds.x + 64 && t.x <= bounds.z - 64);
                assert(t.y >= bounds.y + 64 && t.y <= bounds.w - 64);
            }
        }
        assert(q->quasarFallFired == 18);
        if (bounds.z - bounds.x < 128) assert(q->quasarFallTargetsMask == 0);
    }
    // Bowed continuous flight, exact fixed endpoint, and death cancellation.
    {
        auto q = MakeFall();
        q.BeginQuasarFall();
        float hp = 100;
        for (int i=0;i<250 && q.quasarFallFired < 2;++i) Tick(q,{600,400},DT,hp);
        for (int i=0;i<q.quasarFallFired;++i) {
            const float launch = q.quasarFallLaunchTimes[i];
            assert(glm::length(q.QuasarFallPosition(i,launch)-q.quasarFallOrigins[i]) < 0.001f);
            const float end=q.QuasarFallImpactTime(i);
            const auto origin=q.quasarFallOrigins[i], target=q.quasarFallTargets[i];
            const auto axis=target-origin;
            float maxBend=0;
            for (int k=1;k<10;++k) {
                const auto offset=q.QuasarFallPosition(i,launch+k*0.1f)-origin;
                maxBend=std::max(maxBend,std::fabs(offset.x*axis.y-offset.y*axis.x)/glm::length(axis));
            }
            assert(maxBend > 10); // it visibly bows away from a straight flight
            for (const float seam : {0.14f, 0.75f}) {
                constexpr float h=0.0001f;
                const float t=launch+seam;
                const auto v0=(q.QuasarFallPosition(i,t)-q.QuasarFallPosition(i,t-h))/h;
                const auto v1=(q.QuasarFallPosition(i,t+h)-q.QuasarFallPosition(i,t))/h;
                assert(glm::length(v1-v0)<20); // no peel/approach/drop velocity jumps
            }
            assert(glm::length(q.QuasarFallPosition(i,end)-target) < 0.001f);
        }
        const int fired=q.quasarFallFired;
        q.ApplyDamage(100000);
        Tick(q,{600,400},10,hp);
        assert(!q.QuasarFalling() && q.quasarFallFired==fired && hp==100);
    }
    // Charge/recovery envelopes join without jumps; anticipation precedes opening.
    {
        auto q=MakeFall(); q.BeginQuasarFall();
        assert(q.QuasarFallOpen()==0 && q.QuasarHeat()==0 && q.QuasarSpinTarget()==2.2f);
        q.quasarTimer=Monster::QUASAR_FALL_SPLIT*0.28f;
        assert(q.QuasarFallGather()>0.99f && q.QuasarFallOpen()==0);
        q.quasarTimer=Monster::QUASAR_FALL_SPLIT;
        assert(q.QuasarFallOpen()==1 && q.QuasarFallGather()==0);
        const float heat=q.QuasarHeat(), speed=q.QuasarSpinTarget();
        q.quasarState=Monster::QUASAR_FALL_RECOVER_STATE; q.quasarTimer=0;
        assert(q.QuasarFallOpen()==1 && q.QuasarHeat()==heat && q.QuasarSpinTarget()==speed);
        q.quasarTimer=Monster::QUASAR_FALL_RECOVER-Monster::QUASAR_FALL_IMPACT_FX;
        assert(q.QuasarFallOpen()==0 && q.QuasarHeat()==0 && q.QuasarSpinTarget()==2.2f);
        q.quasarState=0; q.quasarNextFall=false; q.quasarTimer=Monster::QUASAR_COOLDOWN_TIME;
        const float lockSpeed=q.QuasarSpinTarget();
        q.quasarState=2; q.quasarTimer=0;
        assert(q.QuasarSpinTarget()==lockSpeed);
        q.quasarTimer=Monster::QUASAR_LOCK_TIME;
        const float fireSpeed=q.QuasarSpinTarget();
        q.quasarState=3; q.quasarTimer=0;
        assert(std::fabs(q.QuasarSpinTarget()-fireSpeed)<0.0001f);
    }
    // The full lifecycle returns to the original beam after all 18 impacts.
    {
        auto q=MakeFall(); q.quasarTimer=0;
        float hp=10000;
        bool beam=false, fall=false, again=false;
        for(int i=0;i<4800 && !again;++i) {
            const float oldSpeed=q.quasarSpinSpeed;
            const float oldCore=q.quasarCoreAngle;
            Tick(q,{600,400},DT,hp);
            assert(std::fabs(q.quasarSpinSpeed-oldSpeed)<0.7f);
            assert(std::fabs(q.quasarCoreAngle-oldCore)<0.27f);
            if(q.quasarState==3) { if(fall) again=true; beam=true; }
            if(q.QuasarFalling()) { assert(beam && q.QuasarPrecharge()==0); fall=true; }
        }
        assert(beam && fall && again && q.quasarFallFired==18);
    }
    std::puts("Quasar barrage: 18 shots, cadence, live aiming, fixed warnings, damage grace, bounds, motion, death and alternation OK");
}
