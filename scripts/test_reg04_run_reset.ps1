# REG-04 (D04 baseline): what each run reset path clears and keeps.
# Drives the Release build through the REG04 smoke steps (main.cpp): every
# piece of run state is filled with distinct values, one path runs, and a
# snapshot line is appended to Screenshots/reg04_state.txt. Each path is
# compared with the table below, worked out from the reset code.
#   new run      ResetForNewGame
#   restart      RestartCurrentRun (keeps the build, then a fresh run)
#   rebuild      RebuildPlayerStatsFromOwned (augment replacement)
#   abandon      AbandonCurrentRun (ends the run, report screen)
#   READY x2     GAMEOVER -> READY through the frame's own transition, twice:
#                the first resets once, the second must not reset again.
# Values marked OPEN are the current behavior pinned until decided.
# Saves are untouched: smoke sessions skip SaveGame.
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$exeDir = Join-Path $root 'build\windows\x64\Release'
$exe = Join-Path $exeDir 'WiNILL.exe'
if (-not (Test-Path $exe)) { throw "build first: $exe" }
$stateFile = Join-Path $exeDir 'Screenshots\reg04_state.txt'
Remove-Item -LiteralPath $stateFile -ErrorAction SilentlyContinue

$steps = 'RUN~0.3;REG04=NEWRUN~0.05;RUN~0.3;REG04=RESTART~0.05;RUN~0.3;REG04=REBUILD~0.05;' +
         'RUN~0.3;REG04=REBUILD_NO_TIME~0.05;' +
         'RUN~0.3;REG04=ABANDON~0.3;REG04=READY~0.05;REG04DUMP=READY1~0.05;' +
         'REG04=READY~0.05;REG04DUMP=READY2~0.05'
$env:ONEDOW_SMOKE = $steps
$env:ONEDOW_SMOKE_TAG = 'reg04'
$env:ONEDOW_SMOKE_SIZE = '1280x720'
try {
    $p = Start-Process -FilePath $exe -WorkingDirectory $exeDir -PassThru
    if (-not $p.WaitForExit(120000)) { $p.Kill(); throw 'REG-04 smoke timed out' }
    if ($p.ExitCode -ne 0) { throw "REG-04 smoke exited with $($p.ExitCode)" }
} finally {
    Remove-Item Env:ONEDOW_SMOKE, Env:ONEDOW_SMOKE_TAG, Env:ONEDOW_SMOKE_SIZE -ErrorAction SilentlyContinue
    Get-ChildItem (Join-Path $exeDir 'Screenshots') -Filter 'reg04_*.png' -ErrorAction SilentlyContinue | Remove-Item
}
if (-not (Test-Path $stateFile)) { throw 'no REG-04 snapshot written' }

$snap = @{}
foreach ($line in Get-Content $stateFile) {
    $parts = $line -split ' '
    $kv = @{}
    foreach ($p in $parts[1..($parts.Count - 1)]) { $k, $v = $p -split '=', 2; $kv[$k] = $v }
    $snap[$parts[0]] = $kv
}

# The filled run (REG04 fill): every field non-default.
$filled = @{ drones='4'; chakrams='2'; laser='3'; orbs='2'; skills='3'; replaceIdx='2'
    dashCd='1.50'; timeStop='0.80'; focus='0.60'; timeCd='11.00'; shield='25.0'; dashHeld='1'; skillHeld='1'; grace='0.40'
    bullets='5'; monsters='4'; ranged='1'; stardust='6'; shocks='2'; debris='3'; queue='2'
    owned='3'; typeOwned='3'; takenOnce='3'; score='4321'; xp='55.000'; level='3'; hp='42.0'
    kills='777'; weapon='0'; time='99.0' }
# A fresh run (ResetForNewGame + ResetRunProgress).
$fresh = @{ drones='0'; chakrams='0'; laser='0'; orbs='0'; skills='0'; replaceIdx='0'
    dashCd='0.00'; timeStop='0.00'; focus='0.00'; timeCd='0.00'; shield='0.0'
    dashHeld='0'; skillHeld='0'; grace='0.00'
    bullets='0'; monsters='0'; ranged='0'; stardust='0'; shocks='0'; debris='0'; queue='0'
    owned='0'; typeOwned='0'; takenOnce='0'; score='0'; xp='0.000'; level='1'; hp='100.0'
    kills='0'; weapon='-1'; time='0.0' }
function With($base, $over) { $t = $base.Clone(); foreach ($k in $over.Keys) { $t[$k] = $over[$k] }; $t }

$RUNNING = '6'; $READY = '5'; $GAMEOVER = '8'   # GameState ordinals
$expected = [ordered]@{
    NEWRUN  = With $fresh @{}
    # Restart keeps the build: owned cards, the weapon, the skill they grant;
    # the ledger is reset and latched again for once-only owned cards.
    RESTART = With $fresh @{ state=$RUNNING; skills='1'; owned='3'; typeOwned='3'; takenOnce='1'; weapon='0'; hp=$null }
    # Rebuild re-derives augment objects; retained abilities keep their active
    # timers and cooldown, removed abilities lose them.
    REBUILD = With $filled @{ drones='0'; chakrams='0'; laser='0'; orbs='0'; skills='1'; replaceIdx='0'
        focus='0.00' }
    REBUILD_NO_TIME = With $filled @{ drones='0'; chakrams='0'; laser='0'; orbs='0'; skills='0'; replaceIdx='0'
        timeStop='0.00'; focus='0.00'; timeCd='0.00'; owned='2'; typeOwned='2' }
    # Abandon ends the run for the report: the arena is cleared, the build,
    # score and skills stay; GAMEOVER -> READY resets the rest.
    ABANDON = With $filled @{ state=$GAMEOVER; drones='0'; chakrams='0'; laser='0'; orbs='0'
        bullets='0'; monsters='0'; ranged='0'; stardust='0'; shocks='0'; hp='0.0' }
    READY1  = With $fresh @{ state=$READY }
    READY2  = With $filled @{ state=$READY }             # second entry: no second reset
}

$failures = @()
foreach ($label in $expected.Keys) {
    if (-not $snap.ContainsKey($label)) { $failures += "$label snapshot missing"; continue }
    $want = $expected[$label]
    foreach ($k in $want.Keys) {
        if ($null -eq $want[$k]) { continue }
        if ($snap[$label][$k] -ne $want[$k]) { $failures += "$label ${k}: got $($snap[$label][$k]), expected $($want[$k])" }
    }
}
# Restart: full health on the rebuilt max HP (meta levels come from the save).
if ([double]$snap.RESTART.hp -le 0) { $failures += 'RESTART hp not refilled' }

Get-Content $stateFile | ForEach-Object { Write-Host $_ }
if ($failures) { $failures | ForEach-Object { Write-Host "FAIL: $_" }; throw 'REG-04 run reset regression failed' }
Write-Host "REG-04 run reset regression passed ($($expected.Count) paths)"
