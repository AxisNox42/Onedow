$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$installer = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer"
$vswhere = Join-Path $installer 'vswhere.exe'
# vcvars64.bat resolves vswhere through PATH.
$env:PATH = "$installer;$env:PATH"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio C++ tools required' }
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$out = Join-Path $root 'build\tests\kill-lifesteal'
New-Item -ItemType Directory -Force $out | Out-Null
Push-Location $root
try {
    $includes = '/I Include /I WiNILL\Core /I WiNILL\Core\Scenes /I WiNILL\Render /I WiNILL\Entity /I WiNILL\System'
    $command = "call `"$vcvars`" >nul && cl /nologo /utf-8 /std:c++17 /EHsc /MT /DNOMINMAX $includes tests\kill_lifesteal_test.cpp /Fo`"$out\\`" /Fe`"$out\kill_lifesteal_test.exe`""
    & cmd.exe /d /s /c $command
    if ($LASTEXITCODE -ne 0) { throw 'Kill lifesteal regression build failed' }
    & "$out\kill_lifesteal_test.exe"
    if ($LASTEXITCODE -ne 0) { throw 'Kill lifesteal regression failed' }

    # Check that every kill path heals through ApplyKillLifesteal and nothing
    # reads the amount on its own: common collection + laser on mobs + laser
    # on Scopes in KillRewards.h (driven by the REG-01 test), regular mob +
    # Scope bullet kills in CollisionSystem.h, none left in main.cpp.
    $expected = @{ 'WiNILL\Entity\KillRewards.h' = 3; 'WiNILL\Entity\CollisionSystem.h' = 2; 'WiNILL\main.cpp' = 0 }
    foreach ($file in $expected.Keys) {
        $count = (Select-String -Path $file -Pattern 'ApplyKillLifesteal\(' -AllMatches |
                  ForEach-Object { $_.Matches.Count } | Measure-Object -Sum).Sum
        if ($count -ne $expected[$file]) { throw "$file has $count ApplyKillLifesteal calls, expected $($expected[$file])" }
    }
    $direct = Get-ChildItem WiNILL -Recurse -Include *.cpp, *.h, *.inl |
        Where-Object { $_.Name -ne 'PlayerStats.h' -and $_.Name -ne 'DebugToolkit.cpp' } |
        Select-String -Pattern 'GetLifestealPerKill\(|\.lifestealPerKill\b'
    if ($direct) { $direct | Out-String | Write-Host; throw 'kill lifesteal read outside ApplyKillLifesteal' }
    Write-Host 'source check: all 5 kill paths heal through ApplyKillLifesteal'
} finally { Pop-Location }
