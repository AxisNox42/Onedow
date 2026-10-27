$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$installer = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer"
$env:PATH = "$installer;$env:PATH"
$vswhere = Join-Path $installer 'vswhere.exe'
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio C++ tools required' }
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$out = Join-Path $root 'build\tests\wave_schedule'
New-Item -ItemType Directory -Force $out | Out-Null
Push-Location $root
try {
    $includes = '/I Include /I WiNILL\Core /I WiNILL\Core\Scenes /I WiNILL\Render /I WiNILL\Entity /I WiNILL\System'
    $command = "call `"$vcvars`" >nul && cl /nologo /utf-8 /std:c++17 /EHsc /MT /DNOMINMAX $includes tests\wave_schedule_test.cpp /Fo`"$out\\`" /Fe`"$out\wave_schedule_test.exe`""
    & cmd.exe /d /s /c $command
    if ($LASTEXITCODE -ne 0) { throw 'wave schedule build failed' }
    & "$out\wave_schedule_test.exe"
    if ($LASTEXITCODE -ne 0) { throw 'wave schedule test failed' }
} finally { Pop-Location }
