// REG-26 (D26 baseline): options and progress through save -> reload, old
// and damaged saves, and the settings screen's snapshot (discard restores the
// wanted values and the audio device state). Run with
// scripts/test_reg26_settings.ps1 from an empty temporary folder — the save
// path is relative, so the real save is never near.
// Damaged values are rejected or clamped on load (theme index and records
// since 2026-10-13).
#include "SaveSystem.h"
#include "SettingsSnapshot.h"
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <string>

// Audio device stand-in: records the last call of each kind.
namespace Audio {
static bool s_bgm = true, s_sfx = true, s_enabled = true, s_mono = false, s_monoAvailable = true;
static float s_volume = 1.0f;
static int s_calls = 0;
void SetBgmEnabled(bool on) { s_bgm = on; ++s_calls; }
void SetSfxEnabled(bool on) { s_sfx = on; ++s_calls; }
void SetEnabled(bool on) { s_enabled = on; ++s_calls; }
void SetVolume(float v) { s_volume = v; ++s_calls; }
bool SetMonoOutput(bool mono) { s_mono = mono && s_monoAvailable; ++s_calls; return s_mono; }
bool IsMonoOutput() { return s_mono; }
}

// Every option the save file carries, as one comparable value.
struct Options {
    int lang, fps, soundVol, blurHz, mobStyle, vfx;
    bool crosshair, combo, bgm, sfx, mono, audio, autoSkill, menuDim, blur,
         debug, shaderFx, macOpt;
    bool operator==(const Options& o) const {
        return lang == o.lang && fps == o.fps && soundVol == o.soundVol && blurHz == o.blurHz &&
               mobStyle == o.mobStyle && vfx == o.vfx && crosshair == o.crosshair &&
               combo == o.combo && bgm == o.bgm && sfx == o.sfx && mono == o.mono &&
               audio == o.audio && autoSkill == o.autoSkill &&
               menuDim == o.menuDim && blur == o.blur && debug == o.debug &&
               shaderFx == o.shaderFx && macOpt == o.macOpt;
    }
};
static Options Read() {
    return { (int)g_Language, g_FpsCap, g_SoundVol, g_BackdropBlurCaptureHz,
             (int)g_MobVisualStyle, (int)g_VfxDensity, g_ShowCrosshair, g_ShowCombo,
             g_BgmEnabled, g_SfxEnabled, g_AudioMonoOutput, g_AudioEngineEnabled,
             g_AutoSkill, g_StrongMenuDim, g_BackdropBlurEnabled, g_DebugMode, g_ShaderFx,
             g_MacOptV1 };
}
static void Write(const Options& o) {
    g_Language = (Language)o.lang; g_FpsCap = o.fps; g_SoundVol = o.soundVol;
    g_BackdropBlurCaptureHz = o.blurHz; g_MobVisualStyle = (MobVisualStyle)o.mobStyle;
    g_VfxDensity = (VfxDensity)o.vfx; g_ShowCrosshair = o.crosshair; g_ShowCombo = o.combo;
    g_BgmEnabled = o.bgm; g_SfxEnabled = o.sfx; g_AudioMonoOutput = o.mono;
    g_AudioEngineEnabled = o.audio; g_AutoSkill = o.autoSkill;
    g_StrongMenuDim = o.menuDim; g_BackdropBlurEnabled = o.blur; g_DebugMode = o.debug;
    g_ShaderFx = o.shaderFx; g_MacOptV1 = o.macOpt;
}

static Options s_defaults;   // the Settings.h defaults, read at start

// Every option away from its default (and inside its valid range).
static Options Edited() {
    Options o = s_defaults;
    o.lang = s_defaults.lang == 1 ? 2 : 1; o.fps = 144; o.soundVol = 37; o.blurHz = 60;
    o.mobStyle = 1 - s_defaults.mobStyle; o.vfx = s_defaults.vfx == 2 ? 0 : 2;
    o.crosshair = !o.crosshair; o.combo = !o.combo; o.bgm = !o.bgm; o.sfx = !o.sfx;
    o.mono = !o.mono; o.audio = !o.audio; o.autoSkill = !o.autoSkill;
    o.menuDim = !o.menuDim; o.blur = !o.blur; o.debug = !o.debug; o.shaderFx = !o.shaderFx;
    o.macOpt = !o.macOpt;
    return o;
}

static void WriteBytes(const std::string& path, const std::string& bytes) {
    FILE* f = nullptr;
    fopen_s(&f, path.c_str(), "wb");
    assert(f);
    std::fwrite(bytes.data(), 1, bytes.size(), f);
    std::fclose(f);
}

// A save as the game writes it (ODW2: checksum + obfuscation).
static void WriteRawSave(const std::string& text) {
    WriteBytes(SaveFilePath(), EncodeSaveText(text));
}

static void ResetAll() {
    Write(s_defaults);
    ResetSaveProgress();                       // progress back to zero (also saves)
    std::filesystem::remove(SaveFilePath());
    std::filesystem::remove(SaveBackupPath());
    std::filesystem::remove(SaveCorruptPath());
}

static void CheckRoundTrip() {
    const Options edited = Edited();
    Write(edited);
    g_BestScore[0] = 11; g_BestScore[1] = 22; g_BestScore[2] = 33;
    g_TotalKills = 444; g_TotalGames = 55; g_Coins = 6666;
    g_WeaponBestScore[1] = 77; g_WeaponRunCount[0] = 8;
    g_MetaLv[META_HP] = 2; g_AchUnlocked[0] = true; g_AugSeen[3] = true;
    g_MobSeen[CM_SWARM] = true; g_MobKillCounts[CM_ROTOR] = 99; g_JobBought[1] = true;
    g_ThemeOwned = 5; g_ThemeSel = 2;
    SaveGame();

    // On disk: the obfuscated format (magic first, no plain keys).
    FILE* f = nullptr;
    fopen_s(&f, SaveFilePath(), "rb");
    assert(f);
    char head[4] = {};
    assert(std::fread(head, 1, 4, f) == 4 && std::memcmp(head, ODW_MAGIC, 4) == 0);
    std::fclose(f);

    Write(s_defaults);
    for (auto& v : g_BestScore) v = 0;
    g_TotalKills = g_TotalGames = g_Coins = 0;
    g_WeaponBestScore[1] = 0; g_WeaponRunCount[0] = 0;
    g_MetaLv[META_HP] = 0; g_AchUnlocked[0] = false; g_AugSeen[3] = false;
    g_MobSeen[CM_SWARM] = false; g_MobKillCounts[CM_ROTOR] = 0; g_JobBought[1] = false;
    g_ThemeOwned = 1; g_ThemeSel = 0;
    LoadGame();
    assert(Read() == edited);
    assert(g_BestScore[0] == 11 && g_BestScore[1] == 22 && g_BestScore[2] == 33);
    assert(g_TotalKills == 444 && g_TotalGames == 55 && g_Coins == 6666);
    assert(g_WeaponBestScore[1] == 77 && g_WeaponRunCount[0] == 8);
    assert(g_MetaLv[META_HP] == 2 && g_AchUnlocked[0] && g_AugSeen[3] && g_MobSeen[CM_SWARM]);
    assert(g_MobKillCounts[CM_ROTOR] == 99 && g_JobBought[1]);
    assert(g_ThemeOwned == 5 && g_ThemeSel == 2);
    std::printf("round trip: 18 options + progress survive save -> reload\n");
}

// An old plain-text save with only a few keys, unknown keys and junk lines:
// the rest keeps its defaults.
static void CheckOldSave() {
    ResetAll();
    WriteRawSave("lang=1\nsoundvol=40\nfuturekey=5\nnot a line\n=7\nkills\n");
    LoadGame();
    Options want = s_defaults;
    want.lang = 1; want.soundVol = 40;
    assert(Read() == want);
    std::printf("old plain save: missing keys keep defaults, unknown/junk lines ignored\n");
}

// Out-of-range values: rejected or clamped, never taken as they are.
static void CheckRanges() {
    ResetAll();
    WriteRawSave("fps=999\nsoundvol=150\nlang=9\nblurcapturehz=33\nmobstyle=5\nvfxdens=7\n"
                 "meta0=99\ncoins=-5\nmobkills0=-3\nthemesel=42\nbest_easy=-10\nbest_normal=-1\n"
                 "best_hard=-7\nkills=-4\ngames=-2\nwbscore0=-1\nwbkills1=-1\nwtkills0=-9\nwruns1=-3\n");
    LoadGame();
    Options want = s_defaults;
    want.soundVol = 100;                                    // clamped to 0..100
    assert(Read() == want);                                 // fps/lang/hz/style/density rejected
    assert(g_MetaLv[0] == META_DEFS[0].maxLv && g_Coins == 0 && g_MobKillCounts[0] == 0);
    assert(g_ThemeSel == 0);                                // index outside ACCENT_THEMES rejected
    assert(g_BestScore[0] == 0 && g_BestScore[1] == 0 && g_BestScore[2] == 0);
    assert(g_TotalKills == 0 && g_TotalGames == 0);         // records never negative
    assert(g_WeaponBestScore[0] == 0 && g_WeaponBestKills[1] == 0 &&
           g_WeaponTotalKills[0] == 0 && g_WeaponRunCount[1] == 0);

    // A valid theme index loads; an index the save does not own is then put
    // back to the default by ApplyAccentTheme (ownership may load later).
    ResetAll();
    WriteRawSave("themesel=1\n");
    LoadGame();
    assert(ACCENT_COUNT > 1 && g_ThemeSel == 1);
    ApplyAccentTheme();
    assert(g_ThemeSel == 0);                                // theme 1 not owned
    ResetAll();
    WriteRawSave("themeowned=3\nthemesel=1\n");
    LoadGame();
    ApplyAccentTheme();
    assert(g_ThemeSel == 1);

    ResetAll();
    WriteRawSave("soundvol=-5\n");
    LoadGame();
    assert(g_SoundVol == 0);
    std::printf("ranges: invalid choices rejected; volume/meta/coins/records clamped; "
                "theme index range-checked on load, ownership on apply\n");
}

// Hand-edited, tampered or junk files are refused; the previous save (the
// backup every SaveGame keeps) loads instead and the bad file is set aside.
// The legacy ODW1 format still loads. Values cannot overflow.
static void CheckIntegrity() {
    ResetAll();
    WriteBytes(SaveFilePath(), "coins=999999\n");            // plain text
    LoadGame();
    assert(g_Coins == 0 && std::filesystem::exists(SaveCorruptPath()));

    ResetAll();
    std::string legacy = "coins=321\n";
    OdwCrypt(legacy);
    WriteBytes(SaveFilePath(), std::string(ODW_MAGIC_LEGACY, 4) + legacy);
    LoadGame();
    assert(g_Coins == 321);                                  // ODW1 migrates

    ResetAll();
    g_Coins = 100; SaveGame();                               // becomes the backup
    g_Coins = 200; SaveGame();
    assert(std::filesystem::exists(SaveBackupPath()));
    std::string bytes;
    {
        FILE* f = nullptr;
        fopen_s(&f, SaveFilePath(), "rb");
        char buf[4096]; size_t n;
        while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0) bytes.append(buf, n);
        std::fclose(f);
    }
    bytes[bytes.size() / 2] ^= 0x01;                         // one flipped bit
    WriteBytes(SaveFilePath(), bytes);
    g_Coins = 0;
    LoadGame();
    assert(g_Coins == 100);                                  // backup loaded
    assert(std::filesystem::exists(SaveCorruptPath()) && !std::filesystem::exists(SaveFilePath()));

    ResetAll();
    WriteRawSave("coins=9223372036854775807\nkills=9223372036854775807\n");
    LoadGame();
    assert(g_Coins == SAVE_VALUE_MAX && g_TotalKills == SAVE_VALUE_MAX);
    AddSaveValue(g_Coins, 500);
    assert(g_Coins == SAVE_VALUE_MAX);
    RecordRunResult(1, 9223372036854775807LL, 9223372036854775807LL, 0);
    assert(g_Coins == SAVE_VALUE_MAX && g_TotalKills == SAVE_VALUE_MAX &&
           g_BestScore[1] == SAVE_VALUE_MAX);
    std::printf("integrity: plain/tampered files refused, backup restored, ODW1 migrates, "
                "values saturate at %lld\n", SAVE_VALUE_MAX);
}

// Discarding on the settings screen restores the wanted values and the
// audio device state.
static void CheckSnapshot() {
    ResetAll();
    const SettingsSnapshot snap = CaptureSettingsSnapshot();
    const Options before = Read();
    assert(!SettingsSnapshotChanged(snap));
    Options changed = Edited();
    changed.menuDim = before.menuDim; changed.macOpt = before.macOpt;   // not on the screen
    Write(changed);
    assert(SettingsSnapshotChanged(snap));
    Audio::s_calls = 0;
    RestoreSettingsSnapshot(snap);
    assert(Read() == before && !SettingsSnapshotChanged(snap));
    assert(Audio::s_calls == 5);
    assert(Audio::s_bgm == before.bgm && Audio::s_sfx == before.sfx);
    assert(Audio::s_enabled == (before.audio && before.soundVol > 0));
    assert(Audio::s_volume == before.soundVol / 100.0f && Audio::s_mono == before.mono);

    // Each edited field alone lights the save button.
    for (int field = 0; field < 16; ++field) {
        RestoreSettingsSnapshot(snap);
        switch (field) {
        case 0: g_FpsCap = 30; break;               case 1: g_VfxDensity = VfxDensity::MEDIUM; break;
        case 2: g_ShaderFx = !g_ShaderFx; break;    case 3: g_MobVisualStyle = (MobVisualStyle)(1 - (int)g_MobVisualStyle); break;
        case 4: g_ShowCombo = !g_ShowCombo; break;  case 5: g_BackdropBlurEnabled = !g_BackdropBlurEnabled; break;
        case 6: g_BackdropBlurCaptureHz = 1; break; case 7: g_SoundVol = 3; break;
        case 8: g_BgmEnabled = !g_BgmEnabled; break; case 9: g_SfxEnabled = !g_SfxEnabled; break;
        case 10: g_AudioMonoOutput = !g_AudioMonoOutput; break;
        case 11: g_AudioEngineEnabled = !g_AudioEngineEnabled; break;
        case 12: g_AutoSkill = !g_AutoSkill; break; case 13: g_ShowCrosshair = !g_ShowCrosshair; break;
        case 14: g_DebugMode = !g_DebugMode; break; case 15: g_Language = Language::JP; break;
        }
        if (field == 1 && snap.density == VfxDensity::MEDIUM) g_VfxDensity = VfxDensity::FULL;
        if (field == 15 && snap.language == Language::JP) g_Language = Language::EN;
        assert(SettingsSnapshotChanged(snap));
    }
    RestoreSettingsSnapshot(snap);

    // A device that cannot switch to mono: the restore reports what the
    // device took, so the screen still shows a change (current behavior).
    g_AudioMonoOutput = true;
    const SettingsSnapshot monoSnap = CaptureSettingsSnapshot();
    Audio::s_monoAvailable = false;
    RestoreSettingsSnapshot(monoSnap);
    assert(!g_AudioMonoOutput && SettingsSnapshotChanged(monoSnap));
    Audio::s_monoAvailable = true;
    std::printf("snapshot: discard restores 16 options + 5 audio calls; every field is "
                "detected; refused mono stays a change\n");
}

int main() {
    s_defaults = Read();
    g_SmokeCapture = false;                 // real SaveGame, in the temporary folder
    CheckRoundTrip();
    CheckOldSave();
    CheckRanges();
    CheckIntegrity();
    CheckSnapshot();
    ResetAll();
    std::printf("REG-26 settings regression passed\n");
    return 0;
}
