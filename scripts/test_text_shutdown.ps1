$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$installer = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer"
$vswhere = Join-Path $installer 'vswhere.exe'
# vcvars64.bat resolves vswhere through PATH.
$env:PATH = "$installer;$env:PATH"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio C++ tools required' }
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$out = Join-Path $root 'build\tests\text-shutdown'
New-Item -ItemType Directory -Force $out | Out-Null
Push-Location $root
try {
    $sources = 'tests\text_shutdown_test.cpp WiNILL\Render\GameFonts.cpp WiNILL\Entity\AugmentCatalog.cpp WiNILL\Render\stb_impl.cpp glad.c'
    $includes = '/I Include /I WiNILL\Core /I WiNILL\Core\Scenes /I WiNILL\Render /I WiNILL\Entity /I WiNILL\System'
    $command = "call `"$vcvars`" >nul && cl /nologo /utf-8 /std:c++17 /EHsc /MT /DNOMINMAX $includes $sources /Fo`"$out\\`" /Fe`"$out\text_shutdown_test.exe`" /link Lib\glfw3_mt.lib opengl32.lib user32.lib gdi32.lib shell32.lib"
    & cmd.exe /d /s /c $command
    if ($LASTEXITCODE -ne 0) { throw 'Text shutdown regression build failed' }
} finally { Pop-Location }
# Run from WiNILL so the disk font chain (Resource/Font) resolves.
Push-Location (Join-Path $root 'WiNILL')
try {
    & "$out\text_shutdown_test.exe"
    if ($LASTEXITCODE -ne 0) { throw 'Text shutdown regression failed' }
} finally { Pop-Location }
