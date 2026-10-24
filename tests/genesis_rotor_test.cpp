#include "MonsterManager.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <set>

static constexpr float DT=1.0f/120.0f;
struct Scene {
    MonsterManager mm;
    std::vector<Bullet> bullets;
    float hp=10000;
    Monster* g;
    Scene() {
        g=new Monster(200,300); g->MakeKind(MobKind::GENESIS); g->speed=0;
        g->genesisNextRotor=true; g->spawnTimer=Monster::GENESIS_TRACK_TIME+0.01f;
        mm.monsters.push_back(g); tick(0);
        assert(g->hivePhase==4);
    }
    ~Scene(){ mm.Clear(); }
    void tick(float dt=DT,float px=750,float py=350) {
        mm.UpdateAll(px,py,dt,hp,bullets);
    }
};

int main() {
    {
        Scene s; std::set<Monster*> launched;
        std::vector<float> times,headings;
        int prev=0;
        for(int frame=1;frame<500 && s.g->hivePhase>=4;++frame) {
            const float t=frame*DT,px=750,py=350+180*sinf(t*5);
            s.tick(DT,px,py);
            if(s.g->hiveSpawnCount>prev) times.push_back(t);
            prev=s.g->hiveSpawnCount;
            for(auto* m:s.mm.monsters) if(m->kind==MobKind::ROTOR) {
                assert(m->genesisThrown && m->thrownOwnerId==s.g->genesisId);
                assert(m->sizeScale>0 && m->hp>0);
                if(m->genesisRotorPhase==2 && launched.insert(m).second) {
                    assert(m->sizeScale==1.0f);
                    Monster regular(0,0); regular.MakeKind(MobKind::ROTOR);
                    assert(m->hp==regular.hp && m->color==regular.color);
                    assert(std::fabs(m->thrownHeading-atan2f(py-m->thrownStartY,px-m->thrownStartX))<0.0001f);
                    headings.push_back(m->thrownHeading);
                }
            }
        }
        assert(times.size()==6 && launched.size()==6);
        assert(times.front()>=1.0f-DT && times.front()<1.0f+2*DT);
        for(size_t i=1;i<times.size();++i) assert(std::fabs(times[i]-times[i-1]-0.4f)<2*DT);
        assert(std::fabs(headings[0]-headings[2])>0.1f);
        assert(s.g->hivePhase==0 && !s.g->genesisNextRotor);
        for(int i=0;i<650 && s.g->hivePhase==0;++i) s.tick();
        assert(s.g->hivePhase==1); // returns to original Swarm aim/throw
        for(int i=0;i<1000 && s.g->hivePhase!=4;++i) s.tick();
        assert(s.g->hivePhase==4); // original Swarm volley selects Rotor again
        int swarms=0;
        for(auto* m:s.mm.monsters) if(m->kind==MobKind::SWARM && m->genesisThrown) ++swarms;
        assert(swarms>0);
        std::puts("six ordinary Rotors, .4s cadence, current aim every shot, Rotor->Swarm alternation: ok");
    }
    {
        Scene s;
        for(int i=0;i<96;++i) s.tick(); // first small frame forms inside the rear body
        Monster* loaded=nullptr;
        for(auto* m:s.mm.monsters) if(m->genesisRotorPhase==1) loaded=m;
        assert(loaded);
        const glm::vec2 d(cosf(s.g->hiveOrbitAngle),sinf(s.g->hiveOrbitAngle));
        assert(glm::dot(glm::vec2(loaded->worldX,loaded->worldY)-s.g->GenesisRotorCenter(),d)<0);
        const float hp=s.hp;
        loaded->Update(loaded->worldX,loaded->worldY,DT,s.hp); assert(s.hp==hp);
        loaded->alive=false; // no replacement for a shot-down loaded unit
        for(int i=0;i<29;++i) s.tick();
        int n=0; for(auto* m:s.mm.monsters) if(m->genesisRotorPhase>0) ++n;
        assert(n==1 && s.g->hiveSpawnCount==1);
        std::puts("dedicated rear feed is harmless, shot-down ammunition stays missing: ok");
    }
    {
        Monster r(0,0); r.MakeKind(MobKind::ROTOR);
        r.LaunchGenesisRotor(400,0,0);
        float hp=100;
        r.Update(200,0,0.10f,hp);
        assert(r.alive && hp==100 && r.genesisRotorPhase==2); // no direct flight hit
        r.Update(400,79,0.20f,hp);
        assert(r.alive && hp==90 && r.genesisRotorPhase==3 && r.genesisRotorDetonated);
        assert(std::fabs(r.worldX-400)<0.001f && std::fabs(r.worldY)<0.001f);
        for(int i=0;i<20;++i) r.Update(400,79,DT,hp);
        assert(hp==90); // visible pulse cannot hit repeatedly
        for(int i=0;i<20;++i) r.Update(1000,500,DT,hp);
        assert(r.genesisRotorPhase==0);
        Monster miss(0,0); miss.MakeKind(MobKind::ROTOR); miss.LaunchGenesisRotor(500,0,0);
        miss.Update(500,81,1.0f,hp); // beyond the displayed 80px radius
        assert(hp==90 && miss.genesisRotorDetonated);
        Monster immune(0,0); immune.MakeKind(MobKind::ROTOR); immune.LaunchGenesisRotor(500,0,0);
        immune.singularityGrace=2;
        immune.Update(500,0,0.5f,hp);
        assert(hp==90 && immune.genesisRotorDetonated);
        Monster frozen(0,0); frozen.MakeKind(MobKind::ROTOR); frozen.LaunchGenesisRotor(500,0,0);
        frozen.thrownTimer=frozen.thrownDuration;
        frozen.Update(500,0,0,hp);
        assert(hp==90 && frozen.genesisRotorPhase==2 && !frozen.genesisRotorDetonated);
        std::puts("landing AoE: 80px radius, one 10 damage hit, no flight hit, immunity and pause: ok");
    }
    {
        Scene s; for(int i=0;i<124;++i) s.tick();
        Monster* flying=nullptr;
        for(auto* r:s.mm.monsters) if(r->genesisRotorPhase==2) flying=r;
        assert(flying);
        const float tx=flying->thrownTargetX,ty=flying->thrownTargetY;
        s.g->alive=false; const float hp=s.hp;
        s.tick(1.0f,tx,ty); // cancellation must suppress the pending landing hit
        assert(s.hp==hp);
        assert(!flying->genesisRotorDetonated);
        for(auto* m:s.mm.monsters) assert(m->genesisRotorPhase!=1 && m->genesisRotorPhase!=2);
        std::puts("owner death cancels feed and harmful flight before contact: ok");
    }
    {
        Scene s;
        for(int i=0;i<15;++i) {
            auto* r=new Monster(20.0f*i,20); r->MakeKind(MobKind::SWARM);
            r->genesisThrown=true; r->thrownOwnerId=s.g->genesisId; s.mm.monsters.push_back(r);
        }
        for(int i=0;i<300;++i) s.tick();
        int rotors=0;
        for(auto* m:s.mm.monsters) if(m->kind==MobKind::ROTOR && m->thrownOwnerId==s.g->genesisId) ++rotors;
        assert(rotors>0 && rotors<=Monster::GENESIS_ROTOR_CAP);   // full Swarm cap does not block Rotors
        std::puts("Rotor cap is separate from the Swarm cap: ok");
    }
    {
        Scene s;
        for(int i=0;i<Monster::GENESIS_ROTOR_CAP;++i) {
            auto* r=new Monster(-3000.0f,20.0f*i); r->MakeKind(MobKind::ROTOR);
            r->genesisThrown=true; r->thrownOwnerId=s.g->genesisId; r->speed=0; s.mm.monsters.push_back(r);
        }
        for(int i=0;i<300;++i) s.tick();
        int rotors=0;
        for(auto* m:s.mm.monsters) if(m->alive && m->kind==MobKind::ROTOR && m->thrownOwnerId==s.g->genesisId) ++rotors;
        assert(rotors<=Monster::GENESIS_ROTOR_CAP);
        std::puts("Rotor cap holds per Genesis: ok");
    }
    {
        Scene s;
        for(int i=0;i<129;++i) {
            auto* m=new Monster(i*10.0f,0); s.mm.monsters.push_back(m);
        }
        s.tick(1.0f);
        assert(s.mm.monsters.size()==130 && s.g->GenesisRotorKick()==0);
        std::puts("global 130 cap, no recoil without actual ammunition: ok");
    }
    {
        Scene s;
        s.g->hiveOrbitAngle=0;
        for(int i=0;i<124;++i) s.tick(DT,220,300);
        for(auto* r:s.mm.monsters) if(r->genesisRotorPhase==2)
            assert(r->thrownTargetX==220 && r->thrownTargetY==300);
        std::puts("near-muzzle target has no minimum range offset: ok");
    }
    {
        Scene s; s.tick(5.0f);
        assert(s.g->hiveSpawnCount==1 && s.g->hivePhase==5);
        s.tick(0); assert(s.g->hiveSpawnCount==1);
        s.tick(0.2f); assert(s.g->hiveSpawnCount==1);
        const auto rear=s.g->GenesisRotorFeedPoint(0),front=s.g->GenesisRotorFeedPoint(1);
        assert(glm::length(front-rear)>100 && glm::length(front-rear)<115);
        s.g->genesisRotorShotAge=0; assert(s.g->GenesisRotorKick()==0);
        s.g->genesisRotorShotAge=0.03f; assert(s.g->GenesisRotorKick()>0.99f);
        s.g->genesisRotorShotAge=0.29f; assert(s.g->GenesisRotorKick()==0);
        std::puts("no catch-up burst, zero-time freeze, rear-to-front transfer, strong bounded recoil: ok");
    }
    {
        Scene s;
        s.tick(0.2f); const float early=s.g->GenesisRotorPose();
        assert(s.g->GenesisRotorWindup()>0.1f && s.g->hiveSpawnCount==0);
        s.tick(0.3f); const float middle=s.g->GenesisRotorPose();
        s.tick(0.25f); const float ready=s.g->GenesisRotorPose();
        assert(early<middle && ready>0.999f && s.g->hiveSpawnCount==0);
        s.tick(0.10f); assert(s.g->GenesisRotorPose()>0.999f && s.g->hiveSpawnCount==0);
        assert(std::fabs(s.g->GenesisRotorWindup())<0.00001f);
        std::puts("1s preparation: windup -> assembly -> readable hold -> feed, no early shot: ok");
    }
    {
        Scene s;
        s.g->hivePhase=0; s.g->spawnTimer=0; s.g->hiveSpawnCount=0;
        assert(s.g->GenesisRotorWaiting() && s.g->GenesisRotorMode());
        for(int i=0;i<29;++i) {
            s.tick();
            assert(s.g->hivePhase==0 && s.g->GenesisStackFill()==0);
            assert(s.g->genesisShellSpin==0 && s.g->GenesisRotorPose()==0);
            assert(s.mm.monsters.size()==1);
        }
        for(int i=0;i<5 && s.g->hivePhase==0;++i) s.tick();
        assert(s.g->hivePhase==4 && s.g->GenesisRotorMode());
        assert(s.g->GenesisStackFill()==0 && s.g->GenesisRotorPose()<0.001f);
        std::puts(".25s Rotor pause: no Swarm stacks/spin, immediate continuous preparation afterward: ok");
    }
    {
        Scene s;
        const float aperture=s.g->GenesisRotorAperture();
        float previous=0;
        for(int i=0;i<=100;++i) {
            const float u=i/100.0f;
            const float scale=Monster::GenesisRotorFormScale(s.g->GenesisRotorOutputScale(),u);
            const float widest=18.0f*scale*(2.0f*0.78f*1.4142136f+0.32f);
            assert(widest+11.9f<=aperture && scale>=previous);
            previous=scale;
        }
        Monster* unit=nullptr; float lastScale=0; bool wasSmall=false,exited=false;
        for(int i=0;i<130;++i) {
            s.tick();
            for(auto* r:s.mm.monsters) if(r->kind==MobKind::ROTOR && (!unit || unit==r)) {
                if(!unit) { unit=r; wasSmall=r->sizeScale<0.3f; }
                if(lastScale>0) assert(r->sizeScale-lastScale<0.06f);
                lastScale=r->sizeScale;
                if(r->genesisRotorPhase==2) exited=true;
            }
        }
        assert(unit && wasSmall && exited);
        assert(std::fabs(unit->sizeScale-unit->genesisRotorFullScale)<0.0001f);
        std::puts("one real unit forms and moves, grows continuously, fits aperture with 12px clearance: ok");
    }
    {
        for(float distance: {0.0f,20.0f,2500.0f,10000.0f}) {
            Monster r(0,0); r.MakeKind(MobKind::ROTOR);
            r.LaunchGenesisRotor(distance,0,0);
            assert(r.thrownDistance==distance && r.thrownTargetX==distance);
            float hp=100;
            r.Update(distance,0,r.thrownDuration+DT,hp);
            assert(r.genesisRotorDetonated && hp==90 && r.worldX==distance);
        }
        std::puts("unlimited target range: 0 / 20 / 2500 / 10000px landing AoE: ok");
    }
    std::puts("Genesis Rotor regression passed");
}
