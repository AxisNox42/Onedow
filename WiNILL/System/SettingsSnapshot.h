#pragma once
// The settings screen's snapshot of the options it edits: taken on entry and
// after saving, restored when changes are discarded, compared to light the
// save button. Moved unchanged out of SceneSettings.cpp (the capture was
// written out three times there) so REG-26 can drive it.

#include "Settings.h"
#include "Audio.h"

struct SettingsSnapshot {
    int fps = 60;
    VfxDensity density = VfxDensity::FULL;
    bool shaderFx = false;
    MobVisualStyle mobStyle = MobVisualStyle::CLASSIC;
    bool combo = true;
    int soundVol = 100;
    bool bgmEnabled = true;
    bool sfxEnabled = true;
    bool audioMonoOutput = false;
    bool audioEngineEnabled = true;
    bool autoSkill = false;
    bool crosshair = true;
    bool debugMode = false;
    Language language = Language::KR;
    int bgmVol = 100;
    int sfxVol = 100;
};

inline SettingsSnapshot CaptureSettingsSnapshot() {
    return { g_FpsCap, g_VfxDensity, g_ShaderFx, g_MobVisualStyle,
             g_ShowCombo,
             g_SoundVol, g_BgmEnabled, g_SfxEnabled,
             g_AudioMonoOutput, g_AudioEngineEnabled,
             g_AutoSkill, g_ShowCrosshair, g_DebugMode,
             g_Language, g_BgmVol, g_SfxVol };
}

// Puts every snapshot value back and re-applies the audio device state.
inline void RestoreSettingsSnapshot(const SettingsSnapshot& saved) {
    g_FpsCap = saved.fps;
    g_VfxDensity = saved.density;
    g_ShaderFx = saved.shaderFx;
    g_MobVisualStyle = saved.mobStyle;
    g_ShowCombo = saved.combo;
    g_SoundVol = saved.soundVol;
    g_BgmEnabled = saved.bgmEnabled;
    g_SfxEnabled = saved.sfxEnabled;
    g_BgmVol = saved.bgmVol;
    g_SfxVol = saved.sfxVol;
    g_AudioMonoOutput = saved.audioMonoOutput;
    g_AudioEngineEnabled = saved.audioEngineEnabled;
    Audio::SetBgmVolume(g_BgmVol / 100.0f);
    Audio::SetSfxVolume(g_SfxVol / 100.0f);
    Audio::SetBgmEnabled(g_BgmEnabled && g_BgmVol > 0);
    Audio::SetSfxEnabled(g_SfxEnabled && g_SfxVol > 0);
    Audio::SetEnabled(g_AudioEngineEnabled && g_SoundVol > 0);
    Audio::SetVolume(g_SoundVol / 100.0f);
    Audio::SetMonoOutput(g_AudioMonoOutput);
    g_AudioMonoOutput = Audio::IsMonoOutput();
    g_AutoSkill = saved.autoSkill;
    g_ShowCrosshair = saved.crosshair;
    g_DebugMode = saved.debugMode;
    g_Language = saved.language;
}

inline bool SettingsSnapshotChanged(const SettingsSnapshot& saved) {
    return g_FpsCap != saved.fps
        || g_VfxDensity != saved.density
        || g_ShaderFx != saved.shaderFx
        || g_MobVisualStyle != saved.mobStyle
        || g_ShowCombo != saved.combo
        || g_SoundVol != saved.soundVol
        || g_BgmEnabled != saved.bgmEnabled
        || g_SfxEnabled != saved.sfxEnabled
        || g_AudioMonoOutput != saved.audioMonoOutput
        || g_AudioEngineEnabled != saved.audioEngineEnabled
        || g_AutoSkill != saved.autoSkill
        || g_ShowCrosshair != saved.crosshair
        || g_DebugMode != saved.debugMode
        || g_Language != saved.language
        || g_BgmVol != saved.bgmVol
        || g_SfxVol != saved.sfxVol;
}
