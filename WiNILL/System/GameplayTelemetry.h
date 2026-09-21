#pragma once

// Runtime gameplay telemetry.  This intentionally has no _DEBUG/NDEBUG gate:
// development Release builds should produce the same data as Debug builds.

#include <algorithm>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

#include "ExpSystem.h"
#include "Platform.h"
#include "PlayerStats.h"

class GameplayTelemetry {
public:
    // Change this to 60.0f when a once-per-minute sample is more useful.
    static constexpr float kDefaultSampleIntervalSeconds = 10.0f;

    struct StatsSnapshot {
        float baseDamage = 0.0f;
        float effectiveDamage = 0.0f;
        float damageMultiplier = 0.0f;
        float maxHP = 0.0f;
        float bulletSpeed = 0.0f;
        float fireInterval = 0.0f;
        float moveSpeedMult = 0.0f;
        float regenPerSec = 0.0f;
        float damageReduction = 0.0f;
        float xpMult = 0.0f;
        float xpPerSec = 0.0f;
        float windowSize = 0.0f;
        float flatDamageBonus = 0.0f;
        float critMult = 0.0f;
        float bulletSpread = 0.0f;

        int totalAugs = 0;
        int pierceChance = 0;
        int visionStacks = 0;
        int droneCount = 0;
        int chakramCount = 0;
        int minigunTier = 0;
        int laserTier = 0;
        int critChance = 0;
        int mobXpBonus = 0;
        int eliteXpBonus = 0;
        int specialMobXpBonus = 0;
        int bomberXpBonus = 0;
        int rangedXpBonus = 0;
        long long killCount = 0;

        bool sniper = false;
        bool vampire = false;
        bool miniaturize = false;
        bool gigantify = false;
        bool pierce = false;
        bool twin = false;
        bool chakram = false;
        bool minigun = false;
        bool drone = false;
        bool laser = false;
        bool bulletRain = false;
        bool cannon = false;
        bool shotgun = false;
        bool revolver = false;
        bool berserk = false;
        bool deathBlast = false;
        bool meleeWeapon = false;
        bool bowWeapon = false;
    };

    GameplayTelemetry()
        : m_sampleIntervalSeconds(kDefaultSampleIntervalSeconds) {}

    void SetSampleIntervalSeconds(float seconds) {
        if (!std::isfinite(seconds)) return;
        m_sampleIntervalSeconds = std::max(1.0f, std::min(seconds, 3600.0f));
    }

    float SampleIntervalSeconds() const { return m_sampleIntervalSeconds; }
    bool IsActive() const { return m_active; }

    static StatsSnapshot Capture(const PlayerStats& stats) {
        StatsSnapshot out;
        out.baseDamage = stats.GetBaseDamage();
        out.effectiveDamage = stats.GetBaseDamage() * stats.GetDamageMultiplier(0.0f);
        out.damageMultiplier = stats.damageMultiplier;
        out.maxHP = stats.maxHP;
        out.bulletSpeed = stats.bulletSpeed;
        out.fireInterval = stats.fireInterval;
        out.moveSpeedMult = stats.moveSpeedMult;
        out.regenPerSec = stats.regenPerSec;
        out.damageReduction = stats.damageReduction;
        out.xpMult = stats.xpMult;
        out.xpPerSec = stats.xpPerSec;
        out.windowSize = stats.windowSize;
        out.flatDamageBonus = stats.flatDamageBonus;
        out.critMult = stats.critMult;
        out.bulletSpread = stats.bulletSpread;

        out.totalAugs = stats.totalAugs;
        out.pierceChance = stats.pierceChance;
        out.visionStacks = stats.visionStacks;
        out.droneCount = stats.droneCount;
        out.chakramCount = stats.chakramCount;
        out.minigunTier = stats.minigunTier;
        out.laserTier = stats.laserTier;
        out.critChance = stats.critChance;
        out.mobXpBonus = stats.mobXpBonus;
        out.eliteXpBonus = stats.eliteXpBonus;
        out.specialMobXpBonus = stats.specialMobXpBonus;
        out.bomberXpBonus = stats.bomberXpBonus;
        out.rangedXpBonus = stats.rangedXpBonus;
        out.killCount = stats.killCount;

        out.sniper = stats.sniper;
        out.vampire = stats.vampire;
        out.miniaturize = stats.miniaturize;
        out.gigantify = stats.gigantify;
        out.pierce = stats.pierce;
        out.twin = stats.twin;
        out.chakram = stats.chakram;
        out.minigun = stats.minigun;
        out.drone = stats.drone;
        out.laser = stats.laser;
        out.bulletRain = stats.bulletRain;
        out.cannon = stats.cannon;
        out.shotgun = stats.shotgun;
        out.revolver = stats.revolver;
        out.berserk = stats.berserk;
        out.deathBlast = stats.deathBlast;
        out.meleeWeapon = stats.meleeWeapon;
        out.bowWeapon = stats.bowWeapon;
        return out;
    }

    void BeginRun(float gameTime, int level, long long xp,
                  const PlayerStats& stats, const char* mode) {
        if (m_active) {
            // A run reset can happen from the menu without a separate end
            // event. Close the old section before starting the new one.
            m_file << "RUN_END,reason=replaced_by_new_run\n";
            m_file.flush();
            m_file.close();
        }

        // Use an absolute path so this stays beside the executable even when
        // the resource loader changes the process working directory.
        m_file.open(OutputFilePath(), std::ios::out | std::ios::app);
        if (!m_file.is_open()) {
            m_active = false;
            return;
        }

        m_active = true;
        ++m_runSerial;
        m_totalExperience = 0;
        m_lastSampleGameTime = gameTime;
        m_lastSampleTotalExperience = 0;
        m_nextSampleGameTime = gameTime + m_sampleIntervalSeconds;
        m_lastLevelUpGameTime = gameTime;
        m_lastLevelUpTotalExperience = 0;

        m_file << "\n============================================================\n";
        m_file << "RUN_BEGIN,serial=" << m_runSerial
               << ",timestamp=" << Timestamp()
               << ",build=" << BuildName()
               << ",mode=" << (mode ? mode : "unknown")
               << ",sample_interval_sec=" << std::fixed << std::setprecision(2)
               << m_sampleIntervalSeconds << "\n";
        m_file << "# SAMPLE fields: game_sec,level,current_xp,required_xp,"
                  "progress_pct,interval_xp,interval_xp_per_sec,"
                  "interval_xp_per_min,total_xp,stats...\n";
        WriteSample(gameTime, level, xp, RequiredForLevel(level), stats, 0, 0.0);
        m_file.flush();
    }

    void RecordExperience(long long amount) {
        if (!m_active || amount <= 0) return;
        m_totalExperience += amount;
    }

    void RecordLevelUp(float gameTime, int oldLevel, int newLevel,
                       long long requiredXp, long long xpBefore,
                       long long xpAfter, bool manualShortcut = false) {
        if (!m_active) return;
        const double levelIntervalSeconds = std::max(
            0.0, (double)gameTime - m_lastLevelUpGameTime);
        const long long levelIntervalXp =
            m_totalExperience - m_lastLevelUpTotalExperience;
        const double levelIntervalPerMinute = levelIntervalSeconds > 0.001
            ? (double)levelIntervalXp * 60.0 / levelIntervalSeconds : 0.0;
        const double averagePerMinute = gameTime > 0.001f
            ? (double)m_totalExperience * 60.0 / (double)gameTime : 0.0;
        m_file << "LEVEL_UP,game_sec=" << std::fixed << std::setprecision(2)
               << gameTime
               << ",level_before=" << oldLevel
               << ",level_after=" << newLevel
               << ",required_xp=" << requiredXp
               << ",xp_before=" << xpBefore
               << ",xp_after=" << xpAfter
               << ",level_interval_sec=" << levelIntervalSeconds
               << ",level_interval_xp=" << levelIntervalXp
               << ",level_interval_xp_per_min=" << levelIntervalPerMinute
               << ",run_total_xp=" << m_totalExperience
               << ",run_average_xp_per_min=" << std::setprecision(2)
               << averagePerMinute
               << ",source=" << (manualShortcut ? "manual_shortcut" : "experience")
               << "\n";
        m_lastLevelUpGameTime = gameTime;
        m_lastLevelUpTotalExperience = m_totalExperience;
        m_file.flush();
    }

    void RecordAugment(float gameTime, int level, int augmentIndex,
                       const PlayerStats& before, const PlayerStats& after) {
        if (!m_active) return;

        const StatsSnapshot b = Capture(before);
        const StatsSnapshot a = Capture(after);
        const char* code = "unknown";
        const int augmentCount = (int)(sizeof(ALL_AUGS) / sizeof(ALL_AUGS[0]));
        if (augmentIndex >= 0 && augmentIndex < augmentCount && ALL_AUGS[augmentIndex].name)
            code = ALL_AUGS[augmentIndex].name;

        m_file << "\nAUGMENT,game_sec=" << std::fixed << std::setprecision(2)
               << gameTime << ",level=" << level
               << ",index=" << augmentIndex << ",code=" << code << "\n";
        WriteFloatDiff("base_damage", b.baseDamage, a.baseDamage);
        WriteFloatDiff("effective_damage_at_zero_distance", b.effectiveDamage, a.effectiveDamage);
        WriteFloatDiff("damage_multiplier", b.damageMultiplier, a.damageMultiplier);
        WriteFloatDiff("max_hp", b.maxHP, a.maxHP);
        WriteFloatDiff("bullet_speed", b.bulletSpeed, a.bulletSpeed);
        WriteFloatDiff("fire_interval_sec", b.fireInterval, a.fireInterval);
        WriteFloatDiff("move_speed_multiplier", b.moveSpeedMult, a.moveSpeedMult);
        WriteFloatDiff("regen_per_sec", b.regenPerSec, a.regenPerSec);
        WriteFloatDiff("damage_reduction", b.damageReduction, a.damageReduction);
        WriteFloatDiff("xp_multiplier", b.xpMult, a.xpMult);
        WriteFloatDiff("xp_per_sec", b.xpPerSec, a.xpPerSec);
        WriteFloatDiff("window_size", b.windowSize, a.windowSize);
        WriteFloatDiff("flat_damage_bonus", b.flatDamageBonus, a.flatDamageBonus);
        WriteFloatDiff("crit_multiplier", b.critMult, a.critMult);
        WriteFloatDiff("bullet_spread", b.bulletSpread, a.bulletSpread);

        WriteIntDiff("total_augments", b.totalAugs, a.totalAugs);
        WriteIntDiff("pierce_chance_pct", b.pierceChance, a.pierceChance);
        WriteIntDiff("vision_stacks", b.visionStacks, a.visionStacks);
        WriteIntDiff("drone_count", b.droneCount, a.droneCount);
        WriteIntDiff("chakram_count", b.chakramCount, a.chakramCount);
        WriteIntDiff("minigun_tier", b.minigunTier, a.minigunTier);
        WriteIntDiff("laser_tier", b.laserTier, a.laserTier);
        WriteIntDiff("crit_chance_pct", b.critChance, a.critChance);
        WriteIntDiff("mob_xp_bonus", b.mobXpBonus, a.mobXpBonus);
        WriteIntDiff("elite_xp_bonus", b.eliteXpBonus, a.eliteXpBonus);
        WriteIntDiff("special_mob_xp_bonus", b.specialMobXpBonus, a.specialMobXpBonus);
        WriteIntDiff("bomber_xp_bonus", b.bomberXpBonus, a.bomberXpBonus);
        WriteIntDiff("ranged_xp_bonus", b.rangedXpBonus, a.rangedXpBonus);
        WriteIntDiff("kill_count", b.killCount, a.killCount);

        WriteBoolDiff("sniper", b.sniper, a.sniper);
        WriteBoolDiff("vampire", b.vampire, a.vampire);
        WriteBoolDiff("miniaturize", b.miniaturize, a.miniaturize);
        WriteBoolDiff("gigantify", b.gigantify, a.gigantify);
        WriteBoolDiff("pierce", b.pierce, a.pierce);
        WriteBoolDiff("twin", b.twin, a.twin);
        WriteBoolDiff("chakram", b.chakram, a.chakram);
        WriteBoolDiff("minigun", b.minigun, a.minigun);
        WriteBoolDiff("drone", b.drone, a.drone);
        WriteBoolDiff("laser", b.laser, a.laser);
        WriteBoolDiff("bullet_rain", b.bulletRain, a.bulletRain);
        WriteBoolDiff("cannon", b.cannon, a.cannon);
        WriteBoolDiff("shotgun", b.shotgun, a.shotgun);
        WriteBoolDiff("revolver", b.revolver, a.revolver);
        WriteBoolDiff("berserk", b.berserk, a.berserk);
        WriteBoolDiff("death_blast", b.deathBlast, a.deathBlast);
        WriteBoolDiff("melee_weapon", b.meleeWeapon, a.meleeWeapon);
        WriteBoolDiff("bow_weapon", b.bowWeapon, a.bowWeapon);
        m_file.flush();
    }

    void Update(float gameTime, int level, long long xp, long long requiredXp,
                const PlayerStats& stats) {
        if (!m_active || gameTime + 0.0001f < m_nextSampleGameTime) return;

        // Normally only one sample is due per frame. The loop keeps the
        // cadence correct if a debugger or a long frame skips an interval.
        while (gameTime + 0.0001f >= m_nextSampleGameTime) {
            const double intervalSeconds = std::max(
                0.001, (double)gameTime - m_lastSampleGameTime);
            const long long intervalXp = m_totalExperience - m_lastSampleTotalExperience;
            WriteSample(gameTime, level, xp, requiredXp, stats,
                        intervalXp, (double)intervalSeconds);
            m_lastSampleGameTime = gameTime;
            m_lastSampleTotalExperience = m_totalExperience;
            m_nextSampleGameTime += m_sampleIntervalSeconds;
        }
        m_file.flush();
    }

    void EndRun(float gameTime, int level, long long xp,
                const PlayerStats& stats, const char* reason) {
        if (!m_active) return;

        if (gameTime > m_lastSampleGameTime + 0.01f) {
            const double intervalSeconds = (double)gameTime - m_lastSampleGameTime;
            const long long intervalXp = m_totalExperience - m_lastSampleTotalExperience;
            WriteSample(gameTime, level, xp, RequiredForLevel(level), stats,
                        intervalXp, intervalSeconds);
        }

        const double averagePerMinute = gameTime > 0.001f
            ? (double)m_totalExperience * 60.0 / (double)gameTime : 0.0;
        m_file << "RUN_END,game_sec=" << std::fixed << std::setprecision(2)
               << gameTime << ",level=" << level << ",current_xp=" << xp
               << ",total_xp=" << m_totalExperience
               << ",average_xp_per_min=" << std::setprecision(2) << averagePerMinute
               << ",reason=" << (reason ? reason : "unknown") << "\n";
        m_file << "============================================================\n";
        m_file.flush();
        m_file.close();
        m_active = false;
    }

private:
    static std::string OutputFilePath() {
        const std::string exeDir = PlatformExeDirectory();
        if (exeDir.empty()) return "onedow_gameplay_telemetry.txt";
        return exeDir + "/onedow_gameplay_telemetry.txt";
    }

    static const char* BuildName() {
#ifdef NDEBUG
        return "Release";
#else
        return "Debug";
#endif
    }

    static std::string Timestamp() {
        const std::time_t now = std::time(nullptr);
        std::tm local = {};
#ifdef _WIN32
        localtime_s(&local, &now);
#else
        localtime_r(&now, &local);
#endif
        std::ostringstream out;
        out << std::put_time(&local, "%Y-%m-%d_%H-%M-%S");
        return out.str();
    }

    static long long RequiredForLevel(int level) {
        if (level < 1) return 0;
        return g_ExpSystem.Required(level);
    }

    void WriteSample(float gameTime, int level, long long xp,
                     long long requiredXp, const PlayerStats& stats,
                     long long intervalXp, double intervalSeconds) {
        const double progress = requiredXp > 0
            ? std::max(0.0, std::min(100.0,
                (double)xp * 100.0 / (double)requiredXp)) : 0.0;
        const double xpPerSecond = intervalSeconds > 0.0
            ? (double)intervalXp / intervalSeconds : 0.0;
        const double xpPerMinute = xpPerSecond * 60.0;

        const StatsSnapshot s = Capture(stats);
        m_file << "SAMPLE,game_sec=" << std::fixed << std::setprecision(2)
               << gameTime << ",level=" << level
               << ",current_xp=" << xp
               << ",required_xp=" << requiredXp
               << ",progress_pct=" << std::setprecision(2) << progress
               << ",interval_xp=" << intervalXp
               << ",interval_xp_per_sec=" << xpPerSecond
               << ",interval_xp_per_min=" << xpPerMinute
               << ",total_xp=" << m_totalExperience
               << ",max_hp=" << s.maxHP
               << ",damage_multiplier=" << s.damageMultiplier
               << ",effective_damage=" << s.effectiveDamage
               << ",fire_interval_sec=" << s.fireInterval
               << ",move_speed_multiplier=" << s.moveSpeedMult
               << ",regen_per_sec=" << s.regenPerSec
               << ",xp_multiplier=" << s.xpMult
               << ",xp_per_sec=" << s.xpPerSec
               << ",total_augments=" << s.totalAugs
               << ",kill_count=" << s.killCount << "\n";
    }

    void WriteFloatDiff(const char* name, float before, float after) {
        m_file << "  stat=" << name << ",before=" << std::fixed
               << std::setprecision(4) << before
               << ",after=" << after
               << ",delta=" << (after - before) << "\n";
    }

    template <typename T>
    void WriteIntDiff(const char* name, T before, T after) {
        m_file << "  stat=" << name << ",before=" << before
               << ",after=" << after
               << ",delta=" << (after - before) << "\n";
    }

    void WriteBoolDiff(const char* name, bool before, bool after) {
        m_file << "  stat=" << name << ",before=" << (before ? 1 : 0)
               << ",after=" << (after ? 1 : 0)
               << ",delta=" << ((after ? 1 : 0) - (before ? 1 : 0)) << "\n";
    }

    std::ofstream m_file;
    bool m_active = false;
    unsigned long long m_runSerial = 0;
    float m_sampleIntervalSeconds = kDefaultSampleIntervalSeconds;
    float m_nextSampleGameTime = 0.0f;
    double m_lastSampleGameTime = 0.0;
    long long m_totalExperience = 0;
    long long m_lastSampleTotalExperience = 0;
    double m_lastLevelUpGameTime = 0.0;
    long long m_lastLevelUpTotalExperience = 0;
};
