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

    # The three entry paths reach CollectMonsterKill; only it decides the
    # thrown-Swarm exception. REG-01 exercises each path at runtime.
    $expected = @{ 'WiNILL\main.cpp' = 1; 'WiNILL\Entity\KillRewards.h' = 1;
                   'WiNILL\Entity\CollisionSystem.h' = 1 }
    foreach ($file in $expected.Keys) {
        $count = @(Select-String -Path $file -Pattern '^\s*(?!inline).*\bCollectMonsterKill\(').Count
        if ($count -ne $expected[$file]) { throw "$file has $count CollectMonsterKill calls, expected $($expected[$file])" }
    }
    $central = @(Select-String -Path 'WiNILL\Entity\CollisionSystem.h' -Pattern '^\s*CreditThrownSwarmKill\(').Count
    if ($central -ne 1) { throw "expected one central thrown-Swarm branch, found $central" }
    Write-Host 'source check: 3 entry paths share one thrown-Swarm branch'
} finally { Pop-Location }
