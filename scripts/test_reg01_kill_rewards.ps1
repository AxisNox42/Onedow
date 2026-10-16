$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$installer = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer"
$vswhere = Join-Path $installer 'vswhere.exe'
# vcvars64.bat resolves vswhere through PATH.
$env:PATH = "$installer;$env:PATH"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio C++ tools required' }
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$out = Join-Path $root 'build\tests\reg01-kill-rewards'
New-Item -ItemType Directory -Force $out | Out-Null
Push-Location $root
try {
    $includes = '/I Include /I WiNILL\Core /I WiNILL\Core\Scenes /I WiNILL\Render /I WiNILL\Entity /I WiNILL\System'
    $command = "call `"$vcvars`" >nul && cl /nologo /utf-8 /std:c++17 /EHsc /MT /DNOMINMAX $includes tests\reg01_kill_rewards_test.cpp /Fo`"$out\\`" /Fe`"$out\reg01_kill_rewards_test.exe`""
    & cmd.exe /d /s /c $command
    if ($LASTEXITCODE -ne 0) { throw 'REG-01 build failed' }
    & "$out\reg01_kill_rewards_test.exe"
    if ($LASTEXITCODE -ne 0) { throw 'REG-01 kill reward regression failed' }

    # The test drives the kill entry points directly; this pins that main.cpp
    # still goes through them and has not grown kill-reward code of its own.
    $main = 'WiNILL\main.cpp'
    foreach ($fn in 'CollectMonsterKill', 'CollectScopeKill', 'CreditLaserMonsterKill', 'CreditLaserScopeKill') {
        if (-not (Select-String -Path $main -Pattern "\b$fn\(" -Quiet)) { throw "main.cpp no longer calls $fn" }
    }
    $own = Select-String -Path $main -Pattern 'MobKillReward\(|ApplyKillLifesteal|CreditThrownSwarmKill|RegisterCodexMobKill|\.RegisterKill\(|AddKillCombo\(|scoreAccum \+='
    if ($own) { $own | Out-String | Write-Host; throw 'kill-reward code in main.cpp outside KillRewards.h' }
    Write-Host 'source check: main.cpp kill paths go through KillRewards.h'

    # All five entry paths now share one reward application.
    $vampireCalls = @{ 'WiNILL\Entity\CollisionSystem.h' = 1; 'WiNILL\Entity\KillRewards.h' = 0; 'WiNILL\main.cpp' = 0 }
    foreach ($file in $vampireCalls.Keys) {
        $n = @(Select-String -Path $file -Pattern '\bApplyVampireKill\(').Count
        if ($n -ne $vampireCalls[$file]) { throw "$file has $n ApplyVampireKill calls, expected $($vampireCalls[$file])" }
    }
    $streak = Get-ChildItem WiNILL -Recurse -Include *.cpp, *.h, *.inl |
        Where-Object { $_.Name -ne 'PlayerStats.h' } |
        Select-String -Pattern 'vampireKillStreak\s*(\+\+|>=|<)|\+\+\s*\w*\.?vampireKillStreak|GetVampireKillNeed\('
    if ($streak) { $streak | Out-String | Write-Host; throw 'vampire streak handled outside PlayerStats::ApplyVampireKill' }
    Write-Host 'source check: all 5 kill paths share one ApplyVampireKill'
} finally { Pop-Location }
