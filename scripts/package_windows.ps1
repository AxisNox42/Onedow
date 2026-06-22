# Windows 빌드 → bin/Onedow/WindowsOS/Onedow.exe (에셋은 Onedow 루트 공용)
$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$Onedow = Join-Path $Root "bin\Onedow"
$WinOut = Join-Path $Onedow "WindowsOS"
$Vcx = Join-Path $Root "WiNILL\WiNILL.vcxproj"

$Msbuild = @(
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe",
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
) | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $Msbuild) { throw "MSBuild를 찾을 수 없습니다." }

& $Msbuild $Vcx /p:Configuration=Release /p:Platform=x64 /v:minimal
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& (Join-Path $PSScriptRoot "sync_onedow_assets.ps1")

if (Test-Path $WinOut) { Remove-Item $WinOut -Recurse -Force }
New-Item -ItemType Directory -Force -Path $WinOut | Out-Null
$SrcExe = Join-Path $Root "WiNILL\bin\WiNILL.exe"
Copy-Item $SrcExe (Join-Path $WinOut "Onedow.exe") -Force

Write-Host "Windows -> $WinOut\Onedow.exe"
