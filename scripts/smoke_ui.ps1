# UI 스모크 캡처: Release 빌드를 스크립트 장면 목록으로 실행하고 PNG를 모은다.
#   예) .\scripts\smoke_ui.ps1 -Steps "GAMEOVER~3.5;SETTINGS" -Langs KR,EN,JP -Tag qa07
# 단계 문법은 WiNILL/System/SmokeCapture.h 참고. 결과는 build/smoke/<Tag>/ 로 복사된다.
param(
    [Parameter(Mandatory = $true)][string]$Steps,
    [string[]]$Langs = @("KR"),
    [string]$Tag = "smoke",
    [string]$Size = "",
    [string]$Blur = "",
    [int]$TimeoutSec = 120
)
$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$ExeDir = Join-Path $Root "build\windows\x64\Release"
$Exe = Join-Path $ExeDir "WiNILL.exe"
if (-not (Test-Path $Exe)) { throw "빌드 결과가 없습니다: $Exe" }
$Shots = Join-Path $ExeDir "Screenshots"
$Out = Join-Path $Root "build\smoke\$Tag"
New-Item -ItemType Directory -Force $Out | Out-Null

foreach ($lang in $Langs) {
    $env:ONEDOW_SMOKE = $Steps
    $env:ONEDOW_SMOKE_LANG = $lang
    $env:ONEDOW_SMOKE_TAG = $Tag
    if ($Size) { $env:ONEDOW_SMOKE_SIZE = $Size }
    if ($Blur) { $env:ONEDOW_SMOKE_BLUR = $Blur }
    try {
        $p = Start-Process -FilePath $Exe -WorkingDirectory $ExeDir -PassThru
        if (-not $p.WaitForExit($TimeoutSec * 1000)) {
            $p.Kill()
            throw "스모크 실행 시간 초과 ($lang)"
        }
    } finally {
        Remove-Item Env:ONEDOW_SMOKE, Env:ONEDOW_SMOKE_LANG, Env:ONEDOW_SMOKE_TAG, Env:ONEDOW_SMOKE_SIZE, Env:ONEDOW_SMOKE_BLUR -ErrorAction SilentlyContinue
    }
    Get-ChildItem $Shots -Filter "${Tag}_${lang}_*.png" | Move-Item -Destination $Out -Force
}
Get-ChildItem $Out -Filter *.png | ForEach-Object { $_.FullName }
