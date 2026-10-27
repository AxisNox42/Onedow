// Wave schedule: run length, elite/boss placement and the special-wave
// spawn slowdown (with its exemption hook), per difficulty.
#include <cstdio>

#include "WaveSystem.h"

static int g_failures = 0;
#define EXPECT(cond, ...)                                   \
    do {                                                    \
        if (!(cond)) {                                      \
            std::printf("FAIL %s:%d  ", __FILE__, __LINE__); \
            std::printf(__VA_ARGS__);                       \
            std::printf("\n");                              \
            ++g_failures;                                   \
        }                                                   \
    } while (0)

static int Count(bool (*pred)(int, Difficulty), Difficulty d) {
    int n = 0;
    for (int w = 1; w <= FinalWaveCount(d); ++w) n += pred(w, d) ? 1 : 0;
    return n;
}

int main() {
    const Difficulty easy = Difficulty::EASY, normal = Difficulty::NORMAL,
                     hard = Difficulty::HARD, nightmare = Difficulty::NIGHTMARE;

    EXPECT(FinalWaveCount(easy) == 6, "easy waves");
    EXPECT(FinalWaveCount(normal) == 8, "normal waves");
    EXPECT(FinalWaveCount(hard) == 12, "hard waves");
    EXPECT(FinalWaveCount(nightmare) == 18, "nightmare waves");

    EXPECT(Count(IsEliteWaveNumber, easy) == 1 && Count(IsBossWave, easy) == 1, "easy counts");
    const int normalElites = Count(IsEliteWaveNumber, normal);
    EXPECT(normalElites >= 1 && normalElites <= 2 && Count(IsBossWave, normal) == 1, "normal counts");
    EXPECT(Count(IsEliteWaveNumber, hard) == 3 && Count(IsBossWave, hard) == 1, "hard counts");
    EXPECT(Count(IsEliteWaveNumber, nightmare) == 4 && Count(IsBossWave, nightmare) == 2, "nightmare counts");
    EXPECT(IsEliteWaveNumber(9, nightmare) && IsBossWave(9, nightmare), "nightmare wave 9 overlaps");

    for (int d = 0; d < DIFFICULTY_COUNT; ++d) {
        const Difficulty diff = static_cast<Difficulty>(d);
        const int last = FinalWaveCount(diff);
        EXPECT(IsBossWave(last, diff) && IsFinalBossWave(last, diff), "final wave is a boss wave (d=%d)", d);
        int finals = 0;
        for (int w = 1; w <= last; ++w) {
            finals += IsFinalBossWave(w, diff) ? 1 : 0;
            EXPECT(!IsEliteWaveNumber(w, diff) || EliteOrdinal(w, diff) >= 0, "ordinal d=%d w=%d", d, w);
        }
        EXPECT(finals == 1, "one final boss (d=%d)", d);
        EXPECT(!IsEliteWaveNumber(last, diff) || last == 9, "no elite on the last wave (d=%d)", d);
        EXPECT(CurrentWaveNumber(TotalWaveDurationSeconds(diff) - 0.5f, diff) == last, "ends on last (d=%d)", d);
        EXPECT(CurrentWaveNumber(WaveStartTimeSeconds(last, diff), diff) == last, "boss start (d=%d)", d);
        for (int w = 1; w <= last; ++w)
            EXPECT(CurrentWaveNumber(WaveStartTimeSeconds(w, diff), diff) == w, "wave %d start (d=%d)", w, d);
    }

    // Boss wave lengths: only the last boss gets the long fight.
    EXPECT(WaveDurationSeconds(18, nightmare) == BOSS_WAVE_DURATION_SECONDS, "final boss length");
    EXPECT(WaveDurationSeconds(9, nightmare) == MID_BOSS_WAVE_DURATION_SECONDS, "mid boss length");
    EXPECT(WaveDurationSeconds(1, nightmare) == REGULAR_WAVE_DURATION_SECONDS, "regular length");

    // Slowdown: only on elite/boss waves, off again right after, hook can opt out.
    EXPECT(SpecialWaveSpawnStretch(1, nightmare) == 1.0f, "no slowdown on a regular wave");
    EXPECT(SpecialWaveSpawnStretch(5, nightmare) == SPECIAL_WAVE_SPAWN_STRETCH, "elite slowdown");
    EXPECT(SpecialWaveSpawnStretch(9, nightmare) == SPECIAL_WAVE_SPAWN_STRETCH, "overlap slowdown not doubled");
    EXPECT(SpecialWaveSpawnStretch(18, nightmare) == SPECIAL_WAVE_SPAWN_STRETCH, "boss slowdown");
    EXPECT(SpecialWaveSpawnStretch(6, nightmare) == 1.0f, "restored after the elite wave");
    EXPECT(SpecialWaveSpawnStretch(10, nightmare) == 1.0f, "restored after the mid boss wave");
    EXPECT(SpecialWaveSpawnStretch(5, nightmare, SpawnSource::Regulus) == 1.0f, "regulus exempt on elite wave");
    EXPECT(SpecialWaveSpawnStretch(18, nightmare, SpawnSource::Regulus) == 1.0f, "regulus exempt on boss wave");
    EXPECT(SpecialWaveSpawnStretch(5, nightmare, SpawnSource::Ranged) == SPECIAL_WAVE_SPAWN_STRETCH, "ranged still slowed");

    if (g_failures == 0) std::printf("wave schedule: all checks passed\n");
    return g_failures == 0 ? 0 : 1;
}
