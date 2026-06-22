# Windows 빌드 + ZIP 한 번에
$ErrorActionPreference = "Stop"
& (Join-Path $PSScriptRoot "package_windows.ps1")
& (Join-Path $PSScriptRoot "zip_onedow.ps1")
