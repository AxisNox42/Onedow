# bin/Onedow 폴더 전체를 Onedow.zip 으로 (배포용)
$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$Onedow = Join-Path $Root "bin\Onedow"
$Zip = Join-Path $Root "bin\Onedow.zip"

if (-not (Test-Path $Onedow)) { throw "bin/Onedow 없음. 먼저 package_windows.ps1 실행" }
if (-not (Test-Path (Join-Path $Onedow "WindowsOS\Onedow.exe"))) {
    throw "WindowsOS\Onedow.exe 없음"
}

if (Test-Path $Zip) { Remove-Item $Zip -Force }
Compress-Archive -Path $Onedow -DestinationPath $Zip -CompressionLevel Optimal

Write-Host ""
Write-Host "배포 ZIP: $Zip"
Write-Host '  압축 해제 -> Onedow/WindowsOS, macOS, Resource, Font'
