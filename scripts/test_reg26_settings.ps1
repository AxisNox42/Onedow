$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$installer = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer"
$vswhere = Join-Path $installer 'vswhere.exe'
# vcvars64.bat resolves vswhere through PATH.
$env:PATH = "$installer;$env:PATH"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio C++ tools required' }
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$out = Join-Path $root 'build\tests\reg26-settings'
New-Item -ItemType Directory -Force $out | Out-Null
Push-Location $root
try {
    $includes = '/I Include /I WiNILL\Core /I WiNILL\Core\Scenes /I WiNILL\Render /I WiNILL\Entity /I WiNILL\System'
    $command = "call `"$vcvars`" >nul && cl /nologo /utf-8 /std:c++17 /EHsc /MT /DNOMINMAX $includes tests\reg26_settings_test.cpp WiNILL\Entity\AugmentCatalog.cpp /Fo`"$out\\`" /Fe`"$out\reg26_settings_test.exe`""
    & cmd.exe /d /s /c $command
    if ($LASTEXITCODE -ne 0) { throw 'REG-26 build failed' }
} finally { Pop-Location }

# The save path is relative: run in an empty folder, far from the real save.
$sandbox = Join-Path $out 'sandbox'
if (Test-Path $sandbox) { Remove-Item -LiteralPath $sandbox -Recurse -Force }
New-Item -ItemType Directory -Force $sandbox | Out-Null
Push-Location $sandbox
try {
    & "$out\reg26_settings_test.exe"
    if ($LASTEXITCODE -ne 0) { throw 'REG-26 settings regression failed' }
} finally { Pop-Location }

# Coverage: every option the settings screen edits is in the snapshot, and
# every snapshot option has a save key.
Push-Location $root
try {
    $uiOnly = 'g_SettingsEntryT', 'g_MainMenuEntryT', 'g_ScrollAccum'
    $edited = Select-String -Path 'WiNILL\Core\Scenes\SceneSettings.cpp' -Pattern '\b(g_\w+)\s*=(?!=)' -AllMatches |
        ForEach-Object { $_.Matches } | ForEach-Object { $_.Groups[1].Value } | Sort-Object -Unique |
        Where-Object { $uiOnly -notcontains $_ }
    $snapshot = (Get-Content 'WiNILL\System\SettingsSnapshot.h' -Raw) -match 'CaptureSettingsSnapshot\(\) \{\s*return \{([^}]*)\}'
    $captured = ($Matches[1] -split ',') | ForEach-Object { $_.Trim() }
    $missing = $edited | Where-Object { $captured -notcontains $_ }
    if ($missing) { throw "edited on the settings screen but not in the snapshot: $($missing -join ', ')" }
    $saveKey = @{ g_FpsCap='fps'; g_VfxDensity='vfxdens'; g_ShaderFx='shaderfx'; g_MobVisualStyle='mobstyle'
        g_ShowCombo='combo'; g_BackdropBlurEnabled='backdropblur'; g_BackdropBlurCaptureHz='blurcapturehz'
        g_SoundVol='soundvol'; g_BgmEnabled='bgmenabled'; g_SfxEnabled='sfxenabled'; g_AudioMonoOutput='audiomono'
        g_AudioEngineEnabled='audioenabled'; g_AutoSkill='autoskill'
        g_ShowCrosshair='crosshair'; g_DebugMode='debugmode'; g_Language='lang' }
    $save = Get-Content 'WiNILL\System\SaveSystem.h' -Raw
    foreach ($g in $captured) {
        $key = $saveKey[$g]
        if (-not $key) { throw "snapshot option $g has no known save key" }
        if ($save -notmatch "add\(`"$key=" -or $save -notmatch "strcmp\(key, `"$key`"\)") { throw "$g ($key) is not both saved and loaded" }
    }
    Write-Host "coverage: $(@($edited).Count) edited options in the $($captured.Count)-field snapshot; all saved and loaded"
} finally { Pop-Location }
