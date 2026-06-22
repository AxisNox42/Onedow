# Onedow 공용 에셋 (Resource / Font / Icons)
$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$Onedow = Join-Path $Root "bin\Onedow"

New-Item -ItemType Directory -Force -Path $Onedow | Out-Null

$Res = Join-Path $Onedow "Resource"
$Font = Join-Path $Onedow "Font"
$Icons = Join-Path $Onedow "Icons"

if (Test-Path $Res) { Remove-Item $Res -Recurse -Force }
if (Test-Path $Font) { Remove-Item $Font -Recurse -Force }
if (Test-Path $Icons) { Remove-Item $Icons -Recurse -Force }

Copy-Item (Join-Path $Root "WiNILL\Resource") $Res -Recurse
if (Test-Path (Join-Path $Root "WiNILL\Font")) {
    Copy-Item (Join-Path $Root "WiNILL\Font") $Font -Recurse
}
if (Test-Path (Join-Path $Root "WiNILL\Icons")) {
    Copy-Item (Join-Path $Root "WiNILL\Icons") $Icons -Recurse
}

Write-Host "공용 에셋 -> $Onedow"
