// Headless regression for the Quasar supernova death: the kill reward is held
// until the blast (QUASAR_NOVA_BLAST) and released exactly once, the dead
// Quasar stays for the whole animation (QUASAR_NOVA_TIME) and is then removed,
// and a death without a body animation releases the reward at once.
// Run with scripts/test_quasar_nova.ps1.
#include "MonsterManager.h"
#include <cassert>
#include <cmath>
#include <cstdio>

static constexpr float DT = 1.0f / 120.0f;

static Monster* KillQuasar(MonsterManager& mm, bool animate) {
    auto* q = new Monster(0.0f, 0.0f);
    q->MakeKind(MobKind::QUASAR);
    q->alive = false;
    q->scored = true;
    q->dustPending = true;
    q->pendingDustValue = 7;
    q->pendingDustXp = XpFromWhole(123);
    q->SpawnDeathEffect(0.0f, true, animate);
    mm.monsters.push_back(q);
    return q;
}

static void CheckHeldUntilBlast() {
    MonsterManager mm;
    std::vector<Bullet> bullets;
    float hp = 100.0f;
    Monster* q = KillQuasar(mm, true);
    float t = 0.0f;
    int released = 0;
    float releasedAt = -1.0f;
    while (!mm.monsters.empty()) {
        mm.UpdateAll(5000.0f, 0.0f, DT, hp, bullets);
        t += DT;
        if (!mm.monsters.empty() && mm.monsters[0]->TakePendingDust()) {
            ++released;
            releasedAt = t;
            assert(q->pendingDustValue == 7 && q->pendingDustXp == XpFromWhole(123));
        }
        assert(t < 3.0f);
    }
    assert(released == 1);
    assert(fabsf(releasedAt - Monster::QUASAR_NOVA_BLAST) < 2.0f * DT);
    assert(fabsf(t - Monster::QUASAR_NOVA_TIME) < 3.0f * DT);
    std::printf("reward at %.2fs (blast), body removed at %.2fs ok\n", releasedAt, t);
}

static void CheckNoAnimationReleasesNow() {
    MonsterManager mm;
    Monster* q = KillQuasar(mm, false);
    assert(q->TakePendingDust());
    assert(!q->TakePendingDust());
    std::printf("no-animation death releases at once: ok\n");
    mm.Clear();
}

static void CheckPendingBlocksRemoval() {
    MonsterManager mm;
    std::vector<Bullet> bullets;
    float hp = 100.0f;
    Monster* q = KillQuasar(mm, true);
    q->deathEffectTimer = Monster::QUASAR_NOVA_TIME;      // animation over, reward not taken
    mm.UpdateAll(5000.0f, 0.0f, DT, hp, bullets);
    assert(mm.monsters.size() == 1);                       // kept until released
    assert(q->TakePendingDust());
    mm.UpdateAll(5000.0f, 0.0f, DT, hp, bullets);
    assert(mm.monsters.empty());
    std::printf("pending reward blocks removal: ok\n");
}

// Genesis: same build-up as the Quasar (reward at the first wave, 1.25s),
// but a longer wave tail before the body is removed.
static void CheckGenesisTiming() {
    MonsterManager mm;
    std::vector<Bullet> bullets;
    float hp = 100.0f;
    auto* g = new Monster(0.0f, 0.0f);
    g->MakeKind(MobKind::GENESIS);
    g->alive = false;
    g->scored = true;
    g->dustPending = true;
    g->SpawnDeathEffect(0.0f, true, true);
    mm.monsters.push_back(g);
    float t = 0.0f, releasedAt = -1.0f;
    while (!mm.monsters.empty()) {
        mm.UpdateAll(5000.0f, 0.0f, DT, hp, bullets);
        t += DT;
        if (!mm.monsters.empty() && mm.monsters[0]->TakePendingDust()) releasedAt = t;
        assert(t < 5.0f);
    }
    assert(Monster::GENESIS_WAVE_1 == Monster::QUASAR_NOVA_BLAST);
    assert(fabsf(releasedAt - Monster::GENESIS_WAVE_1) < 2.0f * DT);
    assert(Monster::GENESIS_DEATH_TIME - Monster::GENESIS_WAVE_1 ==
           Monster::QUASAR_NOVA_TIME - Monster::QUASAR_NOVA_BLAST);      // same wave time
    assert(fabsf(t - Monster::GENESIS_DEATH_TIME) < 3.0f * DT);
    std::printf("genesis: reward at %.2fs, %d waves, removed at %.2fs ok\n",
                releasedAt, Monster::GENESIS_WAVE_COUNT, t);
}

int main() {
    CheckGenesisTiming();
    CheckHeldUntilBlast();
    CheckNoAnimationReleasesNow();
    CheckPendingBlocksRemoval();
    std::printf("quasar nova regression passed\n");
    return 0;
}
