# Live-game smoke for the Gravis field and the Regulus elite cycle.
# Needs a Release x64 build: build\windows\x64\Release\WiNILL.exe
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$exeDir = Join-Path $root 'build\windows\x64\Release'
$exe = Join-Path $exeDir 'WiNILL.exe'
if (-not (Test-Path $exe)) { throw "build first: $exe" }
$stateFile = Join-Path $exeDir 'Screenshots\mob_state.txt'
Remove-Item -LiteralPath $stateFile -ErrorAction SilentlyContinue
$steps = @('RUN~0.3', 'MOBCLEAR~0.05', 'SPAWN=3~0.05', 'MOBPLACE=150~0.02', 'MOBDUMP=G0~0.02')
1..4 | ForEach-Object { $steps += "MOBDUMP=G$_~0.5" }
$steps += @('MOBCLEAR~0.05', 'SPAWN=6~0.05', 'MOBPLACE=-300~0.02', 'MOBDUMP=R0~0.02')
1..24 | ForEach-Object { $steps += "MOBDUMP=R$_~0.5" }
$env:ONEDOW_SMOKE = $steps -join ';'
$env:ONEDOW_SMOKE_TAG = 'mobsmoke'
$env:ONEDOW_SMOKE_SIZE = '1280x720'
try {
    $p = Start-Process -FilePath $exe -WorkingDirectory $exeDir -PassThru -WindowStyle Hidden
    if (-not $p.WaitForExit(180000)) { $p.Kill(); throw 'mob smoke timed out' }
    if ($p.ExitCode -ne 0) { throw "mob smoke exited with $($p.ExitCode)" }
} finally {
    Remove-Item Env:ONEDOW_SMOKE, Env:ONEDOW_SMOKE_TAG, Env:ONEDOW_SMOKE_SIZE -ErrorAction SilentlyContinue
}
$lines = Get-Content $stateFile
function Field($line, $name) { if ($line -match "$name=(-?[\d.]+)") { [double]$Matches[1] } else { $null } }
# --- Gravis: not elite, field is on and pulls the player toward it.
$gp = $lines | Where-Object { $_ -match '^G\d player' }
$gm = $lines | Where-Object { $_ -match '^G\d gravis' }
if (-not $gm) { throw 'no Gravis was alive' }
if ((Field $gm[0] 'elite') -ne 0) { throw "Gravis still elite: $($gm[0])" }
$infl = ($gp | ForEach-Object { Field $_ 'influence' } | Measure-Object -Maximum).Maximum
$moved = (Field $gp[-1] 'x') - (Field $gp[0] 'x')
Write-Host "Gravis: max influence $infl, player x moved $moved px toward the field"
if ($infl -le 0.2) { throw 'Gravis field never reached the player' }
if ($moved -le 5) { throw 'Gravis pull did not move the player' }
# --- Regulus: elite, goes through walk -> surge -> cloak, cloak stays visible.
$rm = $lines | Where-Object { $_ -match '^R\d+ regulus' }
if (-not $rm) { throw 'no Regulus was alive' }
if ((Field $rm[0] 'elite') -ne 1) { throw "Regulus not elite: $($rm[0])" }
$phases = $rm | ForEach-Object { [int](Field $_ 'phase') } | Sort-Object -Unique
$alpha = ($rm | ForEach-Object { Field $_ 'alpha' } | Measure-Object -Minimum).Minimum
$shield = ($rm | ForEach-Object { Field $_ 'shield' } | Measure-Object -Minimum).Minimum
Write-Host "Regulus: phases seen $($phases -join ','), min alpha $alpha, min shield $shield"
if (($phases -join ',') -ne '0,1,2') { throw 'Regulus did not cycle walk/surge/cloak' }
if ($alpha -gt 0.40 -or $alpha -lt 0.37) { throw "cloak alpha out of range: $alpha" }
if ($shield -le 0) { throw 'Regulus shield missing' }
# speed per phase from consecutive samples (positions may bounce off the border)
$t = $lines | Where-Object { $_ -match '^R\d+ player' } | ForEach-Object { Field $_ 'time' }
$speed = @{ 0 = @(); 1 = @(); 2 = @() }
for ($i = 1; $i -lt $rm.Count; $i++) {
    $d = [math]::Sqrt([math]::Pow((Field $rm[$i] 'x') - (Field $rm[$i-1] 'x'), 2) + [math]::Pow((Field $rm[$i] 'y') - (Field $rm[$i-1] 'y'), 2))
    $dt = $t[$i] - $t[$i-1]
    if ($dt -gt 0.05 -and (Field $rm[$i] 'phase') -eq (Field $rm[$i-1] 'phase')) { $speed[[int](Field $rm[$i] 'phase')] += $d / $dt }
}
foreach ($k in 0,1,2) { $a = if ($speed[$k].Count) { ($speed[$k] | Measure-Object -Average).Average } else { 0 }; Write-Host ("  phase {0}: avg {1:N0} px/s over {2} samples" -f $k, $a, $speed[$k].Count) }
if ($speed[1].Count -and $speed[0].Count -and ($speed[1] | Measure-Object -Average).Average -le 1.5 * ($speed[0] | Measure-Object -Average).Average) { throw 'surge is not faster than walking' }
Write-Host 'Gravis / Regulus smoke passed'
