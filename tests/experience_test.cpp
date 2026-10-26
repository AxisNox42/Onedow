#include "ExpSystem.h"
#include "GameplayTelemetry.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <limits>

int main() {
    const int levels[] = {1, 2, 3, 5, 10, 20, 30, 40, 45, 60};
    const long long expected[] = {6, 31, 83, 285, 1507, 7954, 21049, 41985, 55700, 111100};
    for (int i = 0; i < 10; ++i) {
        assert(g_ExpSystem.Required(levels[i]) == expected[i]);
        assert(g_ExpSystem.RequiredAmount(levels[i]) == XpFromWhole(expected[i]));
    }

    XpAmount xp = 5999;
    int level = 1;
    assert(!g_ExpSystem.ConsumeNextLevel(xp, level));
    ++xp;
    assert(g_ExpSystem.ConsumeNextLevel(xp, level));
    assert(level == 2 && xp == 0);

    xp = XpFromWhole(6 + 31 + 83) + 125;
    level = 1;
    int gained = 0;
    while (level < MAIN_LEVEL_CAP && g_ExpSystem.ConsumeNextLevel(xp, level)) ++gained;
    assert(gained == 3 && level == 4 && xp == 125);

    assert(XpFromReward(1.0 * 1.03f) == 1030);
    assert(XpFromReward(2.0 * 1.03f) == 2060);
    assert(XpFromReward(-1.0) == 0);
    assert(XpFromWhole(std::numeric_limits<XpAmount>::max()) ==
           std::numeric_limits<XpAmount>::max());
    assert(XpAdd(std::numeric_limits<XpAmount>::max() - 1, 2) ==
           std::numeric_limits<XpAmount>::max());
    assert(FormatXp(1030) == "1.030");

    GameplayTelemetry telemetry;
    PlayerStats stats;
    telemetry.BeginRun(0.0f, 1, 0, stats, "xp_test");
    assert(telemetry.IsActive());
    telemetry.RecordExperience(1030);
    telemetry.Update(10.0f, 1, 1030, XpFromWhole(6), stats);
    telemetry.RecordLevelUp(10.0f, 1, 2, XpFromWhole(6), XpFromWhole(6) + 125, 125);
    telemetry.EndRun(10.0f, 2, 125, stats, "xp_test");
    std::ifstream log(PlatformExeDirectory() + "/onedow_gameplay_telemetry.txt");
    const std::string contents((std::istreambuf_iterator<char>(log)), std::istreambuf_iterator<char>());
    assert(contents.find("current_xp=1.030,required_xp=6.000") != std::string::npos);
    assert(contents.find("interval_xp=1.030") != std::string::npos);
    assert(contents.find("required_xp=6.000,xp_before=6.125,xp_after=0.125") != std::string::npos);
    assert(contents.find("total_xp=1.030") != std::string::npos);
    std::puts("experience curve, fractional rewards and timed XP passed");
}
