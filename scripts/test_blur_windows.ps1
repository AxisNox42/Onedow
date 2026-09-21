$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio C++ tools required' }
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$out = Join-Path $root 'build\tests\blur'
New-Item -ItemType Directory -Force $out | Out-Null
Push-Location $root
try {
    $command = "call `"$vcvars`" >nul && cl /nologo /utf-8 /std:c++17 /EHsc /MT /I Include /I WiNILL\Render /I WiNILL\System tests\blur_render_test.cpp WiNILL\Render\BlurShader.cpp glad.c /Fo`"$out\\`" /Fe`"$out\blur_render_test.exe`" /link Lib\glfw3_mt.lib opengl32.lib user32.lib gdi32.lib shell32.lib"
    & cmd.exe /d /s /c $command
    if ($LASTEXITCODE -ne 0) { throw 'Blur regression build failed' }
    & "$out\blur_render_test.exe"
    if ($LASTEXITCODE -ne 0) { throw 'Blur regression failed' }
} finally { Pop-Location }


