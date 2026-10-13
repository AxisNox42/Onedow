#pragma once
// Kill rewards of the main.cpp kill paths: the common death collection and
// the laser. Moved out of main() lambdas so a test can drive them (REG-01);
// the bullet paths live in CollisionSystem::Update. The Creative guard still
// differs between paths; unifying the remaining reward rules is D01.

#include "CollisionSystem.h"

// The run state a kill reward writes. main.cpp binds it to g_GameManager and
// g_Stats; tests bind it to locals.
struct KillRewardSink {
    float& playerHP;
    float& scoreAccum;
    long long& score;
    PlayerStats& stats;
    float playerCX, playerCY;   // where kill stardust heads
    bool creative;              // g_CreativeMode
};

// Rewards of a regular kill on the common collection (was creditKill).
inline void CreditRegularKill(KillRewardSink& s, CodexMobId mobId, float scoreBase) {
    AddKillCombo();
    s.stats.RegisterKill();
    RegisterCodexMobKill(mobId);
    s.scoreAccum += scoreBase;
    s.score       = (long long)s.scoreAccum;
    if (!s.creative)
        s.playerHP = s.stats.ApplyKillLifesteal(s.playerHP);
    s.playerHP = s.stats.ApplyVampireKill(s.playerHP);
}

// Common collection of a dead regular mob that no kill path credited yet.
inline void CollectMonsterKill(KillRewardSink& s, Monster& m) {
    if (!m.scored && m.genesisThrown) {
        CreditThrownSwarmKill(m);   // combo + codex only
    } else if (!m.scored) {
        m.scored = true;
        float xpB, scB; MobKillReward(m.kind, xpB, scB);
        float rwm = MobRewardMult(m.kind); scB *= rwm;
        const XpAmount pickupXp = MobExperienceReward(m.kind, s.stats);
        SpawnKillStardust(m, StardustRewardFor(m.kind),
                          s.playerCX, s.playerCY, pickupXp);
        CreditRegularKill(s, CodexMobIdForKind(m.kind), scB);
    }
}

// Common collection of a dead Scope that no kill path credited yet.
inline void CollectScopeKill(KillRewardSink& s, RangedMob& r) {
    if (!r.scored) {
        r.scored = true;
        const XpAmount pickupXp = RangedMob::ExperienceReward(s.stats);
        SpawnStardust(r.worldX, r.worldY, 3,
                      s.playerCX, s.playerCY, pickupXp);
        CreditRegularKill(s, CM_SCOPE, 300.0f);
    }
}

// A regular mob the laser beam just hit; credits it if the hit killed it.
inline void CreditLaserMonsterKill(KillRewardSink& s, Monster& m) {
    if (!m.alive && m.genesisThrown) {
        CreditThrownSwarmKill(m);   // combo + codex only
    } else if (!m.alive) {
        m.scored = true; AddKillCombo();
        float bx, bs; MobKillReward(m.kind, bx, bs);
        float rwm = MobRewardMult(m.kind); bs *= rwm;
        const XpAmount pickupXp = MobExperienceReward(m.kind, s.stats);
        SpawnKillStardust(m, StardustRewardFor(m.kind),
                          s.playerCX, s.playerCY, pickupXp);
        s.stats.RegisterKill(); RegisterCodexMobKill(m.kind);
        s.scoreAccum += bs;
        s.score = (long long)s.scoreAccum;
        s.playerHP = s.stats.ApplyKillLifesteal(s.playerHP);
        s.playerHP = s.stats.ApplyVampireKill(s.playerHP);
    }
}

// A Scope the laser beam just hit; credits it if the hit killed it.
inline void CreditLaserScopeKill(KillRewardSink& s, RangedMob& rr) {
    if (!rr.alive) {
        rr.scored = true; AddKillCombo();
        const XpAmount pickupXp = RangedMob::ExperienceReward(s.stats);
        SpawnStardust(rr.worldX, rr.worldY, 3,
                      s.playerCX, s.playerCY, pickupXp);
        s.stats.RegisterKill();
        RegisterCodexMobKill(CM_SCOPE);
        s.scoreAccum += 300.0f;
        s.score = (long long)s.scoreAccum;
        // was lOnKill
        s.playerHP = s.stats.ApplyKillLifesteal(s.playerHP);
        s.playerHP = s.stats.ApplyVampireKill(s.playerHP);
    }
}
