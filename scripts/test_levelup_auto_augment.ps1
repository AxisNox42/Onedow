$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$exeDir = Join-Path $root 'build\windows\x64\Release'
$exe = Join-Path $exeDir 'WiNILL.exe'
if (-not (Test-Path $exe)) { throw "build first: $exe" }
$stateFile = Join-Path $exeDir 'Screenshots\reg04_state.txt'
Remove-Item -LiteralPath $stateFile -ErrorAction SilentlyContinue
$env:ONEDOW_SMOKE = 'RUN~0.3;XP=6~0.05;REG04DUMP=SIGNAL~0.9;REG04DUMP=OPEN~0.05;RUN~0.3;XP=120.125~0.9;REG04DUMP=MULTI~0.05'
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
$signal = $lines | Where-Object { $_ -match '^SIGNAL ' } | Select-Object -Last 1
$open = $lines | Where-Object { $_ -match '^OPEN ' } | Select-Object -Last 1
$multi = $lines | Where-Object { $_ -match '^MULTI ' } | Select-Object -Last 1
# GameState: RUNNING=6, AUG_SELECT=9. Right after the level-up the run keeps
# going at full speed during the short signal; then the selection opens
# with no input.
if ($signal -notmatch '^SIGNAL state=6 .* level=2 .* rewards=1 levelup=1( |$)') { throw "level-up signal: $signal" }
if ($open -notmatch '^OPEN state=9 .* level=2 .* rewards=1 levelup=0( |$)') { throw "auto open: $open" }
if ($multi -notmatch '^MULTI state=9 .* level=4 .* rewards=3 levelup=0( |$)') { throw "multi-level open: $multi" }
# No slow-motion: the game clock keeps real pace until the selection opens
# (signal 0.35s, first dump 0.05s in => ~0.3s more game time).
$tSignal = [double]([regex]::Match($signal, ' time=([0-9.]+)').Groups[1].Value)
$tOpen = [double]([regex]::Match($open, ' time=([0-9.]+)').Groups[1].Value)
if (($tOpen - $tSignal) -lt 0.15) { throw "game clock slowed during signal: $tSignal -> $tOpen" }
Write-Host $signal
Write-Host $open
Write-Host $multi
Write-Host 'Level-up auto augment smoke passed'
