# 테스트용 Windows 빌드 → build/windows/x64/Release/WiNILL.exe
$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$Vcx = Join-Path $Root "WiNILL\WiNILL.vcxproj"
$Out = Join-Path $Root "build\windows\x64\Release\WiNILL.exe"

$Msbuild = @(
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe",
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
) | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $Msbuild) { throw "MSBuild를 찾을 수 없습니다." }

& $Msbuild $Vcx /p:Configuration=Release /p:Platform=x64 /v:minimal
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host ""
Write-Host "테스트용: $Out"
