$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$installer = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer"
$vswhere = Join-Path $installer 'vswhere.exe'
# vcvars64.bat resolves vswhere through PATH.
$env:PATH = "$installer;$env:PATH"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio C++ tools required' }
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$out = Join-Path $root 'build\tests\thrown-swarm-kill'
New-Item -ItemType Directory -Force $out | Out-Null
Push-Location $root
try {
    $includes = '/I Include /I WiNILL\Core /I WiNILL\Core\Scenes /I WiNILL\Render /I WiNILL\Entity /I WiNILL\System'
    $command = "call `"$vcvars`" >nul && cl /nologo /utf-8 /std:c++17 /EHsc /MT /DNOMINMAX $includes tests\thrown_swarm_kill_test.cpp /Fo`"$out\\`" /Fe`"$out\thrown_swarm_kill_test.exe`""
    & cmd.exe /d /s /c $command
    if ($LASTEXITCODE -ne 0) { throw 'Thrown swarm kill regression build failed' }
    & "$out\thrown_swarm_kill_test.exe"
    if ($LASTEXITCODE -ne 0) { throw 'Thrown swarm kill regression failed' }

    # Check that the common collection and the laser (KillRewards.h, driven by
    # the REG-01 test) credit thrown Swarms through CreditThrownSwarmKill, as
    # the bullet path in CollisionSystem.h does.
    $expected = @{ 'WiNILL\Entity\KillRewards.h' = 2; 'WiNILL\Entity\CollisionSystem.h' = 1 }
    foreach ($file in $expected.Keys) {
        $count = @(Select-String -Path $file -Pattern '^\s*(?!inline).*\bCreditThrownSwarmKill\(\*?\w').Count
        if ($count -ne $expected[$file]) { throw "$file has $count CreditThrownSwarmKill calls, expected $($expected[$file])" }
    }
    # Every kill-path test of genesisThrown must lead to the helper.
    $branches = Select-String -Path 'WiNILL\Entity\KillRewards.h', 'WiNILL\Entity\CollisionSystem.h' -Pattern '(alive|scored) && .*genesisThrown\)'
    foreach ($b in $branches) {
        $next = (Get-Content $b.Path)[$b.LineNumber]
        if ($next -notmatch 'CreditThrownSwarmKill') { throw "genesisThrown kill branch without CreditThrownSwarmKill: $($b.Path):$($b.LineNumber)" }
    }
    if ($branches.Count -ne 3) { throw "expected 3 thrown-Swarm kill branches, found $($branches.Count)" }
    Write-Host "source check: $($branches.Count) thrown-Swarm kill branches credit through CreditThrownSwarmKill"
} finally { Pop-Location }
