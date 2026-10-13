$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$installer = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer"
$vswhere = Join-Path $installer 'vswhere.exe'
# vcvars64.bat resolves vswhere through PATH.
$env:PATH = "$installer;$env:PATH"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio C++ tools required' }
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$out = Join-Path $root 'build\tests\reg06-skill-reset'
New-Item -ItemType Directory -Force $out | Out-Null
Push-Location $root
try {
    $includes = '/I Include /I WiNILL\Core /I WiNILL\Core\Scenes /I WiNILL\Render /I WiNILL\Entity /I WiNILL\System'
    $sources = 'tests\reg06_skill_reset_test.cpp WiNILL\Entity\PlayerRuntime.cpp WiNILL\Core\Scenes\SceneSkills.cpp WiNILL\Entity\AugmentCatalog.cpp'
    $command = "call `"$vcvars`" >nul && cl /nologo /utf-8 /std:c++17 /EHsc /MT /DNOMINMAX $includes $sources /Fo`"$out\\`" /Fe`"$out\reg06_skill_reset_test.exe`""
    & cmd.exe /d /s /c $command
    if ($LASTEXITCODE -ne 0) { throw 'REG-06 build failed' }
    & "$out\reg06_skill_reset_test.exe"
    if ($LASTEXITCODE -ne 0) { throw 'REG-06 skill reset regression failed' }

    # D06 target, recorded rather than enforced: the Entity layer still reads
    # the equipped-skill state through the Scene header.
    $entityToScene = Select-String -Path 'WiNILL\Entity\*.cpp', 'WiNILL\Entity\*.h' -Pattern '#include ".*Scenes/'
    Write-Host "current Entity -> Scene includes (D06 target 0): $(@($entityToScene).Count)"
    $entityToScene | ForEach-Object { Write-Host "  $($_.Filename):$($_.LineNumber) $($_.Line.Trim())" }
} finally { Pop-Location }
