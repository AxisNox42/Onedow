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
    bool backdropBlur = true;
    int backdropBlurCaptureHz = 20;
    int soundVol = 100;
    bool bgmEnabled = true;
    bool sfxEnabled = true;
    bool audioMonoOutput = false;
    bool audioEngineEnabled = true;
    bool autoFire = true;
    bool autoSkill = false;
    bool crosshair = true;
    bool debugMode = false;
    Language language = Language::KR;
};

inline SettingsSnapshot CaptureSettingsSnapshot() {
    return { g_FpsCap, g_VfxDensity, g_ShaderFx, g_MobVisualStyle,
             g_ShowCombo, g_BackdropBlurEnabled, g_BackdropBlurCaptureHz,
             g_SoundVol, g_BgmEnabled, g_SfxEnabled,
             g_AudioMonoOutput, g_AudioEngineEnabled,
             g_AutoFire, g_AutoSkill, g_ShowCrosshair, g_DebugMode,
             g_Language };
}

// Puts every snapshot value back and re-applies the audio device state.
inline void RestoreSettingsSnapshot(const SettingsSnapshot& saved) {
    g_FpsCap = saved.fps;
    g_VfxDensity = saved.density;
    g_ShaderFx = saved.shaderFx;
    g_MobVisualStyle = saved.mobStyle;
    g_ShowCombo = saved.combo;
    g_BackdropBlurEnabled = saved.backdropBlur;
    g_BackdropBlurCaptureHz = saved.backdropBlurCaptureHz;
    g_SoundVol = saved.soundVol;
    g_BgmEnabled = saved.bgmEnabled;
    g_SfxEnabled = saved.sfxEnabled;
    g_AudioMonoOutput = saved.audioMonoOutput;
    g_AudioEngineEnabled = saved.audioEngineEnabled;
    Audio::SetBgmEnabled(g_BgmEnabled);
    Audio::SetSfxEnabled(g_SfxEnabled);
    Audio::SetEnabled(g_AudioEngineEnabled && g_SoundVol > 0);
    Audio::SetVolume(g_SoundVol / 100.0f);
    Audio::SetMonoOutput(g_AudioMonoOutput);
    g_AudioMonoOutput = Audio::IsMonoOutput();
    g_AutoFire = saved.autoFire;
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
        || g_BackdropBlurEnabled != saved.backdropBlur
        || g_BackdropBlurCaptureHz != saved.backdropBlurCaptureHz
        || g_SoundVol != saved.soundVol
        || g_BgmEnabled != saved.bgmEnabled
        || g_SfxEnabled != saved.sfxEnabled
        || g_AudioMonoOutput != saved.audioMonoOutput
        || g_AudioEngineEnabled != saved.audioEngineEnabled
        || g_AutoFire != saved.autoFire
        || g_AutoSkill != saved.autoSkill
        || g_ShowCrosshair != saved.crosshair
        || g_DebugMode != saved.debugMode
        || g_Language != saved.language;
}
