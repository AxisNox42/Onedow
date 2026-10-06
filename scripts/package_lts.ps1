# LTS 패키징 — bin/Onedow/ (Windows + macOS 슬롯)
$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$Onedow = Join-Path $Root "bin\Onedow"
$WinOut = Join-Path $Onedow "WindowsOS"
$MacOut = Join-Path $Onedow "macOS"
$SrcExe = Join-Path $Root "build\windows\x64\Release\WiNILL.exe"
$VersionHeader = Get-Content (Join-Path $Root "WiNILL\Core\GameVersion.h") -Raw
$Version = [regex]::Match($VersionHeader, 'ONEDOW_VERSION\[\]\s*=\s*L"([0-9.]+)"').Groups[1].Value
if (-not $Version) { throw "GameVersion.h에서 버전을 찾을 수 없습니다." }

& (Join-Path $PSScriptRoot "build_windows.ps1")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

New-Item -ItemType Directory -Force -Path $WinOut | Out-Null
New-Item -ItemType Directory -Force -Path $MacOut | Out-Null

Copy-Item $SrcExe (Join-Path $WinOut "Onedow.exe") -Force
$ResourceOut = Join-Path $Onedow "Resource"
if (Test-Path -LiteralPath $ResourceOut) {
    Remove-Item -LiteralPath $ResourceOut -Recurse -Force
}
Copy-Item -Recurse -Force (Join-Path $Root "WiNILL\Resource") $ResourceOut

$ReadmePath = Join-Path $Onedow "README.md"
$Readme = Get-Content -LiteralPath $ReadmePath -Raw -Encoding UTF8
$Readme = $Readme -replace '(?m)^Version:.*$', "Version: $Version"
Set-Content -LiteralPath $ReadmePath -Value $Readme -Encoding UTF8 -NoNewline

Write-Host ""
Write-Host "LTS Windows -> $WinOut\Onedow.exe"
Write-Host "LTS 폴더    -> $Onedow"
