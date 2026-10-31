# Drive the fourth debug tab with the real mouse hit targets at 1280x720.
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$exeDir = Join-Path $root 'build\windows\x64\Release'
$exe = Join-Path $exeDir 'WiNILL.exe'
$stateFile = Join-Path $exeDir 'Screenshots\reg04_state.txt'
if (-not (Test-Path $exe)) { throw "build first: $exe" }
Remove-Item -LiteralPath $stateFile -ErrorAction SilentlyContinue
$steps = @(
    'RUN~0.2', 'REG04DUMP=BASE~0.05',
    'DEBUG@0.8133,0.2167!~0.4', 'SEARCH@0.60,0.2861!~0.2', 'DEBUGSEARCH=TWIN~0.05',
    'SELECT@0.55,0.4306!~0.2',
    'TOGGLE@0.8719,0.4306!~0.2', 'REG04DUMP=ON~0.05',
    'ADD@0.9578,0.4306!~0.2', 'REG04DUMP=STACK~0.05',
    'REMOVE@0.9188,0.4306!~0.2', 'REG04DUMP=SINGLE~0.05',
    'TOGGLE@0.8719,0.4306!~0.2', 'REG04DUMP=OFF~0.05',
    'GRANT@0.5756,0.8833!~0.2', 'REG04DUMP=GRANT~0.05',
    'CLEAR@0.9213,0.8833!~0.2', 'REG04DUMP=CLEAR~0.05',
    'RANDOM@0.6906,0.8833!~0.2', 'REG04DUMP=RANDOM~0.05',
    'RUN~0.2', 'DEBUG~0.1',
    'FILL@0.8056,0.8833!~0.2', 'REG04DUMP=FULL~0.05',
    'GRANT@0.5756,0.8833!~0.2', 'REG04DUMP=FULL_GRANT~0.05',
    'DRAW@0.4606,0.8833!~0.2', 'REG04DUMP=OFFERED~0.05',
    'DEBUG~0.1', 'ADD@0.9578,0.4306!~0.2', 'REG04DUMP=LOCKED~0.05'
)
$env:ASTERIA_SMOKE = $steps -join ';'
$env:ASTERIA_SMOKE_TAG = 'debug_augments'
$env:ASTERIA_SMOKE_SIZE = '1280x720'
try {
    $p = Start-Process -FilePath $exe -WorkingDirectory $exeDir -PassThru -WindowStyle Hidden
    if (-not $p.WaitForExit(60000)) { $p.Kill(); throw 'debug augment smoke timed out' }
    if ($p.ExitCode -ne 0) { throw "debug augment smoke exited with $($p.ExitCode)" }
} finally {
    Remove-Item Env:ASTERIA_SMOKE, Env:ASTERIA_SMOKE_TAG, Env:ASTERIA_SMOKE_SIZE -ErrorAction SilentlyContinue
    Get-ChildItem (Join-Path $exeDir 'Screenshots') -Filter 'debug_augments_*.png' -ErrorAction SilentlyContinue | Remove-Item
}
$snap = @{}
foreach ($line in Get-Content $stateFile) {
    $parts = $line -split ' '
    $kv = @{}
    foreach ($part in $parts[1..($parts.Count - 1)]) { $k, $v = $part -split '=', 2; $kv[$k] = $v }
    $snap[$parts[0]] = $kv
}
foreach ($label in @('BASE', 'OFF', 'CLEAR')) {
    if ($snap[$label].owned -ne '0' -or $snap[$label].twin -ne '0') { throw "$label must have no augment" }
}
foreach ($label in @('ON', 'SINGLE', 'GRANT')) {
    if ($snap[$label].owned -ne '1' -or $snap[$label].twin -ne '1') { throw "$label must have one Twin" }
}
if ($snap.STACK.owned -ne '2') { throw 'stack increment failed' }
if ([math]::Abs([double]$snap.OFF.damage - [double]$snap.BASE.damage) -gt 0.0001) { throw 'OFF did not restore base damage' }
if ([double]$snap.ON.damage -ge [double]$snap.BASE.damage) { throw 'ON did not apply Twin damage penalty' }
if ([math]::Abs([double]$snap.SINGLE.damage - [double]$snap.ON.damage) -gt 0.0001) { throw 'removing one stack did not rebuild damage' }
if ($snap.RANDOM.state -ne '9' -or $snap.RANDOM.rewards -ne '1') { throw 'random selection did not open' }
if ([int]$snap.FULL.slots -lt 8) { throw 'identity slots were not filled' }
if ($snap.GRANT.state -ne '6' -or $snap.GRANT.rewards -ne '0') { throw 'direct grant must apply without opening a reward' }
if ([int]$snap.FULL_GRANT.owned -ne ([int]$snap.FULL.owned + 1) -or $snap.FULL_GRANT.state -ne '6') {
    throw 'direct grant must work immediately even with full identity slots'
}
foreach ($label in @('OFFERED', 'LOCKED')) {
    if ($snap[$label].state -ne '9' -or $snap[$label].choice0 -ne 'TWIN' -or $snap[$label].rewards -ne '1') {
        throw "$label must offer Twin in the level-up selection screen"
    }
    if ($snap[$label].owned -ne $snap.FULL_GRANT.owned) { throw "$label granted the card before confirmation" }
}
Write-Host 'Debug augment tab: search, ON/OFF, stacks, direct grant, selection preview, full slots and selection guard passed'
