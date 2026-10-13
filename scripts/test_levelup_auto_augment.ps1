$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$exeDir = Join-Path $root 'build\windows\x64\Release'
$exe = Join-Path $exeDir 'WiNILL.exe'
if (-not (Test-Path $exe)) { throw "build first: $exe" }
$stateFile = Join-Path $exeDir 'Screenshots\reg04_state.txt'
Remove-Item -LiteralPath $stateFile -ErrorAction SilentlyContinue
$env:ONEDOW_SMOKE = 'RUN~0.3;XP=6~0.05;REG04DUMP=SLOW~0.9;REG04DUMP=OPEN~0.05;RUN~0.3;XP=120.125~0.9;REG04DUMP=MULTI~0.05'
$env:ONEDOW_SMOKE_TAG = 'levelup'
$env:ONEDOW_SMOKE_SIZE = '1280x720'
try {
    $p = Start-Process -FilePath $exe -WorkingDirectory $exeDir -PassThru -WindowStyle Hidden
    if (-not $p.WaitForExit(120000)) { $p.Kill(); throw 'level-up smoke timed out' }
    if ($p.ExitCode -ne 0) { throw "level-up smoke exited with $($p.ExitCode)" }
} finally {
    Remove-Item Env:ONEDOW_SMOKE, Env:ONEDOW_SMOKE_TAG, Env:ONEDOW_SMOKE_SIZE -ErrorAction SilentlyContinue
    Get-ChildItem (Join-Path $exeDir 'Screenshots') -Filter 'levelup_*.png' -ErrorAction SilentlyContinue | Remove-Item
}
$lines = Get-Content $stateFile
$slow = $lines | Where-Object { $_ -match '^SLOW ' } | Select-Object -Last 1
$open = $lines | Where-Object { $_ -match '^OPEN ' } | Select-Object -Last 1
$multi = $lines | Where-Object { $_ -match '^MULTI ' } | Select-Object -Last 1
# GameState: RUNNING=6, AUG_SELECT=9. Right after the level-up the run keeps
# going (slow-motion); once it finishes the selection opens with no input.
if ($slow -notmatch '^SLOW state=6 .* level=2 .* rewards=1 levelup=1$') { throw "slow-motion phase: $slow" }
if ($open -notmatch '^OPEN state=9 .* level=2 .* rewards=1 levelup=0$') { throw "auto open: $open" }
if ($multi -notmatch '^MULTI state=9 .* level=4 .* rewards=3 levelup=0$') { throw "multi-level open: $multi" }
Write-Host $slow
Write-Host $open
Write-Host $multi
Write-Host 'Level-up auto augment smoke passed'
