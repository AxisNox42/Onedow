$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$installer = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer"
$vswhere = Join-Path $installer 'vswhere.exe'
# vcvars64.bat resolves vswhere through PATH.
$env:PATH = "$installer;$env:PATH"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio C++ tools required' }
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$out = Join-Path $root 'build\tests\reg02-03-stat-builds'
New-Item -ItemType Directory -Force $out | Out-Null
Push-Location $root
try {
    $includes = '/I Include /I WiNILL\Core /I WiNILL\Core\Scenes /I WiNILL\Render /I WiNILL\Entity /I WiNILL\System'
    $command = "call `"$vcvars`" >nul && cl /nologo /utf-8 /std:c++17 /EHsc /MT /DNOMINMAX $includes tests\reg02_03_stat_builds_test.cpp WiNILL\Entity\AugmentCatalog.cpp /Fo`"$out\\`" /Fe`"$out\reg02_03_stat_builds_test.exe`""
    & cmd.exe /d /s /c $command
    if ($LASTEXITCODE -ne 0) { throw 'REG-02/03 build failed' }
    & "$out\reg02_03_stat_builds_test.exe"
    if ($LASTEXITCODE -ne 0) { throw 'REG-02/03 stat build regression failed' }

    # The test drives the RunStats.h builds; pin that the game paths call them.
    $calls = @{
        'WiNILL\main.cpp' = 'RebuildStats\(', 'ChaosStats\(', 'ResetRunStats\(', 'ApplyAugStats\('
        'WiNILL\Core\Scenes\SceneRunConfig.cpp' = @('ApplyLoadoutStats\(')
        'WiNILL\Core\Scenes\SceneAugment.cpp' = 'PreviewBaseStats\(', 'PreviewTrialStats\(', 'PreviewOwnedStats\(', 'PreviewCandidateStats\('
    }
    foreach ($file in $calls.Keys) {
        foreach ($pattern in $calls[$file]) {
            if (-not (Select-String -Path $file -Pattern $pattern -Quiet)) { throw "$file no longer calls $pattern" }
        }
    }
    # No stat build left outside RunStats.h (the old inline sequence).
    $inline = Select-String -Path 'WiNILL\main.cpp', 'WiNILL\Core\Scenes\SceneRunConfig.cpp', 'WiNILL\Core\Scenes\SceneAugment.cpp' `
        -Pattern 'ApplyMeta\(|ApplyWeapon\(|TrialPlayerMaxHpMult\(|ApplyCommonMultBoost\(|baseFireInterval'
    if ($inline) { $inline | Out-String | Write-Host; throw 'stat build code outside RunStats.h' }

    # REG-03 ledger: takenOnce is only ever latched in main.cpp, and only the
    # new-run reset (GameManager::ResetRunProgress) clears it.
    $unlatch = Select-String -Path 'WiNILL\main.cpp' -Pattern 'memset\([^)]*takenOnce|takenOnce\[[^\]]+\]\s*=(?!=)(?!\s*true\b)'
    if ($unlatch) { $unlatch | Out-String | Write-Host; throw 'main.cpp writes takenOnce other than latching it' }
    $gm = Get-Content 'WiNILL\Core\GameManager.cpp'
    $clears = Select-String -Path 'WiNILL\Core\GameManager.cpp' -Pattern 'memset\(takenOnce|takenOnce\[\w+\] = false'
    foreach ($c in $clears) {
        $owner = ($gm[0..($c.LineNumber - 1)] | Select-String -Pattern '^\w[\w:<>&\* ]*\b(\w+::\w+)\(' | Select-Object -Last 1).Matches[0].Groups[1].Value
        if ($owner -ne 'GameManager::ResetRunProgress') { throw "takenOnce cleared in $owner" }
    }
    if (-not (Select-String -Path 'WiNILL\Core\GameManager.cpp' -Pattern 'ComboRewardEligible\(COMBO_DEFS\[c\], takenOnce\)' -Quiet)) {
        throw 'combo injection bypasses the once-only ledger'
    }
    Write-Host "source check: game paths call RunStats.h; takenOnce cleared only by ResetRunProgress ($($clears.Count))"
} finally { Pop-Location }
