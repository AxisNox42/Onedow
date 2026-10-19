$ErrorActionPreference = 'Stop'
# Smoke test of the Gravis elite on the real game loop (needs a Release build):
#  - sling: a player standing still is hit by the throw and the harass darts
#  - glide: the arrival burst hurts for 8 and slows (no shove); the field is back afterwards
$root = Split-Path $PSScriptRoot -Parent
# GRAVIS_SMOKE_EXE_DIR points at another build folder (e.g. while a running game locks the Release exe).
$exeDir = if ($env:GRAVIS_SMOKE_EXE_DIR) { $env:GRAVIS_SMOKE_EXE_DIR } else { Join-Path $root 'build\windows\x64\Release' }
$exe = Join-Path $exeDir 'WiNILL.exe'
if (-not (Test-Path $exe)) { throw "build first: $exe" }
$shots = Join-Path $exeDir 'Screenshots'
$stateFile = Join-Path $shots 'gravis_state.txt'
Remove-Item -LiteralPath $stateFile -ErrorAction SilentlyContinue
$env:ONEDOW_SMOKE = 'RUN~0.3;GRAVISTEST=SLING~10.0;GRAVISDUMP=SLING_MID~0.05;HP=1~4.8;GRAVISDUMP=SLING_END~0.05;GRAVISTEST=GLIDE~1.9;GRAVISDUMP=GLIDE_SLOW~1.5;GRAVISDUMP=GLIDE_END~0.05'
$env:ONEDOW_SMOKE_TAG = 'gravis-elite-test'
$env:ONEDOW_SMOKE_SIZE = '1280x900'
try {
    $p = Start-Process -FilePath $exe -WorkingDirectory $exeDir -PassThru -WindowStyle Hidden
    if (-not $p.WaitForExit(120000)) { $p.Kill(); throw 'Gravis elite smoke timed out' }
    if ($p.ExitCode -ne 0) { throw "Gravis elite smoke exited with $($p.ExitCode)" }
} finally {
    Remove-Item Env:ONEDOW_SMOKE, Env:ONEDOW_SMOKE_TAG, Env:ONEDOW_SMOKE_SIZE -ErrorAction SilentlyContinue
    Get-ChildItem $shots -Filter 'gravis-elite-test_*.png' -ErrorAction SilentlyContinue | Remove-Item
}
function Read-State($label) {
    $line = Get-Content $stateFile | Where-Object { $_ -match "^$label " } | Select-Object -Last 1
    if (-not $line) { throw "missing state line: $label" }
    $values = @{}
    foreach ($m in [regex]::Matches($line, '(\w+)=(-?[0-9.]+)')) { $values[$m.Groups[1].Value] = [double]$m.Groups[2].Value }
    Write-Host $line
    return $values
}
$mid = Read-State 'SLING_MID'
# Mid-harass: a player who stands still has taken the throw (7) and the darts so far (4 each).
$slingLoss = $mid.maxhp - $mid.hp
if ($mid.sling -ne 11) { throw "should be in the harass phase at 10 s: sling state $($mid.sling)" }
if ($slingLoss -lt 4.0 -or $slingLoss -gt 70.0) { throw "throw + darts should cost 4..70 (timing under load varies): $slingLoss" }
if ($mid.field -ne 1) { throw 'the field should be on again while the masses harass' }
$sling = Read-State 'SLING_END'
if ($sling.sling -ne 0 -or $sling.field -ne 1) { throw "the sling should be over and the field on: sling $($sling.sling) field $($sling.field)" }
$slow = Read-State 'GLIDE_SLOW'
$loss = $slow.maxhp - $slow.hp
if ($loss -lt 7.0 -or $loss -gt 14.0) { throw "burst should cost 8 (plus a little contact): $loss" }
if ($slow.slow -le 0.0) { throw 'player should be slowed after the burst' }
# No shove: the player stays at the burst and is inside the field when it returns.
if ($slow.dist -gt 120.0) { throw "player should not be shoved away: $($slow.dist)" }
if ($slow.field -ne 1) { throw 'the field should be back during the recovery' }
$end = Read-State 'GLIDE_END'
if ($end.slow -gt 0.0) { throw "slow should have worn off: $($end.slow)" }
if ($end.field -ne 1) { throw 'the field should be back after the recovery' }
Write-Host 'Gravis elite smoke passed'
