// The real spawner path: a Genesis that enters on an elite wave summons random regular
// enemies only on NIGHTMARE, and its capsules then really open into them.
// Includes the spawner source so its private spawn state can be driven directly.
// Run with scripts/test_genesis_nightmare_spawn.ps1.
#include "EnemySpawner.cpp"
#include <cassert>
#include <cstdio>

static constexpr float DT = 1.0f / 120.0f;

// Enters the scheduled Genesis of the first elite wave of `difficulty` and returns it.
static Monster* EnterGenesis(MonsterManager& mm, Difficulty difficulty) {
    ResetEnemySpawnState();
    constexpr int GENESIS_PICK = 2;                       // kEliteKinds = { QUASAR, REGULUS, GENESIS, ANTARES }
    static_assert(kEliteKinds[GENESIS_PICK] == MobKind::GENESIS, "elite order changed: update this test");
    for (unsigned char& p : g_EnemySpawnState.elitePick) p = GENESIS_PICK;
    const WaveSchedule& sched = WaveScheduleFor(difficulty);
    const int wave = sched.eliteWaves[0];
    const float t = WaveStartTimeSeconds(wave, difficulty) + ELITE_SPAWN_WAVE_SECONDS + 1.0f;
    PlayerStats stats;
    UpdateEnemySpawns(mm, stats, 1280, 720, 0.1f, t, 0, difficulty);
    for (Monster* m : mm.monsters) if (m->kind == MobKind::GENESIS) return m;
    return nullptr;
}

int main() {
    const char* names[4] = { "EASY", "NORMAL", "HARD", "NIGHTMARE" };
    for (int d = 0; d < 4; ++d) {
        MonsterManager mm;
        Monster* g = EnterGenesis(mm, (Difficulty)d);
        assert(g != nullptr);                              // the elite wave brought the Genesis in
        const bool expected = (Difficulty)d == Difficulty::NIGHTMARE;
        std::printf("%-9s Genesis random summons: %s\n", names[d], g->genesisRandomSummons ? "ON" : "off");
        assert(g->genesisRandomSummons == expected);       // NIGHTMARE only
        mm.Clear();
    }
    // Any other way of making a Genesis (the debug spawn) follows the run's difficulty too.
    {
        const Difficulty saved = g_Difficulty;
        for (int d = 0; d < 4; ++d) {
            g_Difficulty = (Difficulty)d;
            Monster made(0.0f, 0.0f);
            made.MakeKind(MobKind::GENESIS);
            assert(made.genesisRandomSummons == ((Difficulty)d == Difficulty::NIGHTMARE));
        }
        g_Difficulty = saved;
        std::printf("a Genesis made outside the spawner follows the difficulty ok\n");
    }
    // On NIGHTMARE, let it run: what it throws is not (only) Swarms.
    {
        MonsterManager mm;
        Monster* g = EnterGenesis(mm, Difficulty::NIGHTMARE);
        assert(g && g->genesisRandomSummons);
        g->speed = 0.0f;
        g->hp = 1.0e9f;
        std::vector<Bullet> bullets;
        float hp = 1.0e9f;
        int kindsSeen[16] = {}, scopes = 0;
        for (float t = 0.0f; t < 60.0f; t += DT) {
            mm.UpdateAll(200.0f, 300.0f, DT, hp, bullets);
            for (Monster* m : mm.monsters)
                if (m->alive && m->genesisSummoned) kindsSeen[(int)m->kind] = 1;
            for (RangedMob* r : mm.rangedMobs) if (r->alive && r->genesisSummoned) ++scopes;
            for (Monster* m : mm.monsters) if (m != g && m->alive) m->hp = 1.0e9f;    // keep them around
        }
        int kinds = 0;
        for (int k = 0; k < 16; ++k) kinds += kindsSeen[k];
        std::printf("in 60 s of a NIGHTMARE Genesis: %d ground kinds summoned, Scope seen %s\n",
                    kinds, scopes > 0 ? "yes" : "no");
        assert(kinds >= 3);                                // a real mix, not just Swarms and Rotors
        mm.Clear();
    }
    std::printf("genesis nightmare spawn regression passed\n");
    return 0;
}
