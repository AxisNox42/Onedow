$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$installer = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer"
$vswhere = Join-Path $installer 'vswhere.exe'
# vcvars64.bat resolves vswhere through PATH.
$env:PATH = "$installer;$env:PATH"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio C++ tools required' }
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$out = Join-Path $root 'build\tests\magnetar-field'
New-Item -ItemType Directory -Force $out | Out-Null
Push-Location $root
try {
    $command = "call `"$vcvars`" >nul && cl /nologo /utf-8 /std:c++17 /EHsc /MT /DNOMINMAX /I Include /I WiNILL\Entity /I WiNILL\System tests\magnetar_field_test.cpp /Fo`"$out\\`" /Fe`"$out\magnetar_field_test.exe`""
    & cmd.exe /d /s /c $command
    if ($LASTEXITCODE -ne 0) { throw 'Magnetar field regression build failed' }
    & "$out\magnetar_field_test.exe"
    if ($LASTEXITCODE -ne 0) { throw 'Magnetar field regression failed' }
} finally { Pop-Location }
