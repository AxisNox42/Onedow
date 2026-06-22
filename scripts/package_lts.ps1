# LTS 패키징 — bin/Onedow/ (Windows + macOS 슬롯)
$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$Onedow = Join-Path $Root "bin\Onedow"
$WinOut = Join-Path $Onedow "WindowsOS"
$MacOut = Join-Path $Onedow "macOS"
$SrcExe = Join-Path $Root "WiNILL\bin\WiNILL.exe"

& (Join-Path $PSScriptRoot "build_windows.ps1")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

New-Item -ItemType Directory -Force -Path $WinOut | Out-Null
New-Item -ItemType Directory -Force -Path $MacOut | Out-Null

Copy-Item $SrcExe (Join-Path $WinOut "Onedow.exe") -Force
Copy-Item -Recurse -Force (Join-Path $Root "WiNILL\Resource") (Join-Path $Onedow "Resource")
Copy-Item -Recurse -Force (Join-Path $Root "WiNILL\Font") (Join-Path $Onedow "Font")
if (Test-Path (Join-Path $Root "WiNILL\Icons")) {
    Copy-Item -Recurse -Force (Join-Path $Root "WiNILL\Icons") (Join-Path $Onedow "Icons")
}

Write-Host ""
Write-Host "LTS Windows -> $WinOut\Onedow.exe"
Write-Host "LTS 폴더    -> $Onedow"
