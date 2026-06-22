# bin/Onedow → bin/Onedow.zip
$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$Onedow = Join-Path $Root "bin\Onedow"
$Zip = Join-Path $Root "bin\Onedow.zip"

if (-not (Test-Path $Onedow)) { throw "bin/Onedow 없음 — package_lts.ps1 먼저" }
if (-not (Test-Path (Join-Path $Onedow "WindowsOS\Onedow.exe"))) {
    throw "WindowsOS\Onedow.exe 없음 — package_lts.ps1 먼저"
}

if (Test-Path $Zip) { Remove-Item $Zip -Force }
Compress-Archive -Path $Onedow -DestinationPath $Zip -CompressionLevel Optimal

Write-Host ""
Write-Host "LTS ZIP: $Zip"
