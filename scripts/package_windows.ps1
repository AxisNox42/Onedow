# Release x64 빌드 후 bin/Onedow/WindowsOS/ 로 패키징
$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$Out  = Join-Path $Root "bin\Onedow\WindowsOS"
$Vcx  = Join-Path $Root "WiNILL\WiNILL.vcxproj"

$Msbuild = @(
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe",
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
) | Where-Object { Test-Path $_ } | Select-Object -First 1

if (-not $Msbuild) { throw "MSBuild를 찾을 수 없습니다." }

Write-Host "==> Building Release x64..."
& $Msbuild $Vcx /p:Configuration=Release /p:Platform=x64 /v:minimal
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$SrcExe = Join-Path $Root "WiNILL\bin\WiNILL.exe"
if (-not (Test-Path $SrcExe)) { throw "빌드 결과 없음: $SrcExe" }

Write-Host "==> Packaging -> $Out"
if (Test-Path $Out) { Remove-Item $Out -Recurse -Force }
New-Item -ItemType Directory -Force -Path $Out | Out-Null

Copy-Item $SrcExe (Join-Path $Out "Onedow.exe")
Copy-Item (Join-Path $Root "WiNILL\Resource") (Join-Path $Out "Resource") -Recurse
if (Test-Path (Join-Path $Root "WiNILL\Font")) {
    Copy-Item (Join-Path $Root "WiNILL\Font") (Join-Path $Out "Font") -Recurse
}
if (Test-Path (Join-Path $Root "WiNILL\Icons")) {
    Copy-Item (Join-Path $Root "WiNILL\Icons") (Join-Path $Out "Icons") -Recurse
}

Write-Host ""
Write-Host "완료: $Out\Onedow.exe"
