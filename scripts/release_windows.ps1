# LTS: 빌드 + bin/Onedow 패키징 + ZIP
$ErrorActionPreference = "Stop"
& (Join-Path $PSScriptRoot "package_lts.ps1")
& (Join-Path $PSScriptRoot "zip_onedow.ps1")
