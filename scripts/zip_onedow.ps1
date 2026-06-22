# bin/Zip/Onedow → bin/Zip/Onedow.zip
$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$Onedow = Join-Path $Root "bin\Zip\Onedow"
$ZipDir = Join-Path $Root "bin\Zip"
$Zip = Join-Path $ZipDir "Onedow.zip"

if (-not (Test-Path $Onedow)) { throw "bin/Zip/Onedow 없음" }
if (-not (Test-Path (Join-Path $Onedow "WindowsOS\Onedow.exe"))) {
    throw "WindowsOS\Onedow.exe 없음 — package_windows.ps1 먼저"
}

New-Item -ItemType Directory -Force -Path $ZipDir | Out-Null
if (Test-Path $Zip) { Remove-Item $Zip -Force }
Compress-Archive -Path $Onedow -DestinationPath $Zip -CompressionLevel Optimal

Write-Host ""
Write-Host "배포 ZIP: $Zip"
