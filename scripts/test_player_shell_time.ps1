$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$installer = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer"
$env:PATH = "$installer;$env:PATH"
$vs = & (Join-Path $installer 'vswhere.exe') -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio C++ tools required' }
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$out = Join-Path $root 'build\tests\player_shell_time'
New-Item -ItemType Directory -Force $out | Out-Null
Push-Location $root
try {
    $includes = '/I Include /I WiNILL\Core /I WiNILL\Core\Scenes /I WiNILL\Render /I WiNILL\Entity /I WiNILL\System'
    $command = "call `"$vcvars`" >nul && cl /nologo /utf-8 /std:c++17 /EHsc /MT /DNOMINMAX /Gy $includes tests\player_shell_time_test.cpp WiNILL\Render\PlayerWeaponShell.cpp /Fo`"$out\\`" /Fe`"$out\player_shell_time_test.exe`" /link /OPT:REF Lib\glfw3_mt.lib opengl32.lib user32.lib gdi32.lib shell32.lib"
    & cmd.exe /d /s /c $command
    if ($LASTEXITCODE -ne 0) { throw 'Player shell time test build failed' }
    & "$out\player_shell_time_test.exe"
    if ($LASTEXITCODE -ne 0) { throw 'Player shell time regression failed' }
} finally { Pop-Location }
