// Headless run of NORMA's pattern loop (Entity/Monster.h + NormaBoss.h).
// Serves the boss's Asks the way the game loop does when no pane can be had,
// and checks that every turn really fires.
// Run with scripts/test_norma_pattern_sim.ps1.
#include "Monster.h"

#include <cstdio>
#include <vector>

static constexpr float DT = 1.0f / 60.0f;
static int g_failures = 0;

static void Check(bool ok, const char* what) {
    if (!ok) {
        std::printf("FAIL: %s\n", what);
        ++g_failures;
    }
}

struct Tally {
    int hideShots = 0, refugeShots = 0, aimShots = 0;
    int aimTurns = 0, dashTurns = 0, windowTurns = 0;
    float dashTravel = 0.0f;   // how far the body moved during charges
};

// noPane: answer every pane Ask with "none" (-1), as on a desktop with no
// usable window and no room to open one.
static Tally Run(float seconds, bool noPane, int phase) {
    Monster boss(0.0f, 0.0f);
    boss.MakeKind(MobKind::NORMA);
    if (phase == 2) boss.hp = boss.normaMaxHp * 0.3f;
    norma::g_Asks.clear();
    norma::g_Events.clear();
    std::vector<Bullet> bullets;
    float hp = 1.0e9f;
    Tally t;
    norma::Pattern last = norma::Pattern::None;
    const float playerX = 420.0f, playerY = 0.0f;
    for (float time = 0.0f; time < seconds; time += DT) {
        const size_t before = bullets.size();
        const float bx = boss.worldX, by = boss.worldY;
        boss.Update(playerX, playerY, DT, hp, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, &bullets);
        const int fired = (int)(bullets.size() - before);
        if (boss.normaPattern == norma::Pattern::Hide) t.hideShots += fired;
        if (boss.normaPattern == norma::Pattern::Refuge) t.refugeShots += fired;
        if (boss.normaPattern == norma::Pattern::Aim) t.aimShots += fired;
        if (boss.normaPattern == norma::Pattern::Dash)
            t.dashTravel += std::sqrt((boss.worldX - bx) * (boss.worldX - bx) +
                                      (boss.worldY - by) * (boss.worldY - by));
        if (boss.normaPattern != last && boss.normaPattern != norma::Pattern::None) {
            if (boss.normaPattern == norma::Pattern::Aim) ++t.aimTurns;
            else if (boss.normaPattern == norma::Pattern::Dash) ++t.dashTurns;
            else ++t.windowTurns;
        }
        last = boss.normaPattern;
        for (norma::Ask ask : norma::g_Asks) {
            if (ask == norma::Ask::ClaimFarWindow || ask == norma::Ask::ClosePlayerWindow ||
                ask == norma::Ask::RamWindow) {
                if (noPane) {
                    boss.normaPaneId = -1;
                } else {
                    boss.normaPaneId = 7;
                    boss.normaTargetX = -300.0f;
                    boss.normaTargetY = 0.0f;
                }
            }
        }
        norma::g_Asks.clear();
        norma::g_Events.clear();
        bullets.clear();   // shots are only counted here
    }
    return t;
}

int main() {
    const Tally withPanes = Run(120.0f, false, 1);
    std::printf("phase 1 with panes: aim %d (%d shots), dash %d (%.0f px), window turns %d, hide %d, refuge %d\n",
                withPanes.aimTurns, withPanes.aimShots, withPanes.dashTurns, withPanes.dashTravel,
                withPanes.windowTurns, withPanes.hideShots, withPanes.refugeShots);
    Check(withPanes.aimTurns + withPanes.dashTurns >= 3, "a direct attack comes round every other turn");
    Check(withPanes.dashTurns == 0 || withPanes.dashTravel > 500.0f, "the charges really move the body");
    Check(withPanes.aimShots > 0, "the aimed stream fires");
    Check(withPanes.windowTurns >= 3, "window patterns take their turns");

    const Tally noPanes = Run(120.0f, true, 1);
    std::printf("phase 1 no panes: aim turns %d (%d shots), dash turns %d, window turns %d\n",
                noPanes.aimTurns, noPanes.aimShots, noPanes.dashTurns, noPanes.windowTurns);
    Check(noPanes.aimShots > 0, "with no pane the boss still fires");

    const Tally phase2 = Run(60.0f, false, 2);
    std::printf("phase 2: aim turns %d (%d shots), dash turns %d\n", phase2.aimTurns,
                phase2.aimShots, phase2.dashTurns);
    Check(phase2.aimTurns + phase2.dashTurns > 0, "phase 2 uses its direct attacks");

    if (g_failures == 0) std::printf("norma pattern sim: all checks passed\n");
    return g_failures == 0 ? 0 : 1;
}
