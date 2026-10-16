$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$installer = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer"
$env:PATH = "$installer;$env:PATH"
$vswhere = Join-Path $installer 'vswhere.exe'
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio C++ tools required' }
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$out = Join-Path $root 'build\tests\game-manager-shutdown'
New-Item -ItemType Directory -Force $out | Out-Null
Push-Location $root
try {
    $includes = '/I Include /I WiNILL\Core /I WiNILL\Core\Scenes /I WiNILL\Render /I WiNILL\Entity /I WiNILL\System'
    $sources = 'tests\game_manager_shutdown_test.cpp WiNILL\Core\GameManager.cpp WiNILL\Entity\AugmentCatalog.cpp glad.c'
    $command = "call `"$vcvars`" >nul && cl /nologo /utf-8 /std:c++17 /EHsc /MT /DNOMINMAX /Gy $includes $sources /Fo`"$out\\`" /Fe`"$out\game_manager_shutdown_test.exe`" /link /OPT:REF Lib\glfw3_mt.lib opengl32.lib user32.lib gdi32.lib shell32.lib"
    & cmd.exe /d /s /c $command
    if ($LASTEXITCODE -ne 0) { throw 'GameManager shutdown test build failed' }
    & "$out\game_manager_shutdown_test.exe"
    if ($LASTEXITCODE -ne 0) { throw 'GameManager shutdown test failed' }
} finally { Pop-Location }
