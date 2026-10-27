# Live-game smoke for the Gravis field and the Regulus buff lasers.
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
# Regulus with a Gravis (as a buff target) 350 px away, sampled for ~15 s.
$steps += @('MOBCLEAR~0.05', 'SPAWN=3~0.05', 'SPAWN=3~0.05', 'SPAWN=3~0.05', 'MOBPLACE=150~0.02', 'SPAWN=6~0.05', 'MOBPLACE=-200~0.02', 'MOBDUMP=R0~0.02')
1..50 | ForEach-Object { $steps += "MOBDUMP=R$_~0.5" }
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
# --- Regulus: elite support. Random buff lasers: shield / speed / stealth.
$rm = $lines | Where-Object { $_ -match '^R\d+ regulus' }
if (-not $rm) { throw 'no Regulus was alive' }
if ((Field $rm[0] 'elite') -ne 1) { throw "Regulus not elite: $($rm[0])" }
$shield = ($rm | ForEach-Object { Field $_ 'shield' } | Measure-Object -Minimum).Minimum
if ($shield -le 0) { throw 'Regulus shield missing' }
$bf = $lines | Where-Object { $_ -match '^R\d+ buffed' }
if (-not $bf) { throw 'no mob was ever buffed by the Regulus' }
$speed = $bf | Where-Object { (Field $_ 'speedAge') -ge 1.0 }
$stealth = $bf | Where-Object { (Field $_ 'stealthAge') -ge 1.0 }
$both = $bf | Where-Object { (Field $_ 'speedAge') -ge 0 -and (Field $_ 'stealthAge') -ge 0 }
$shielded = $bf | Where-Object { (Field $_ 'shield') -eq 1 }
$both += @($bf | Where-Object { (Field $_ 'shield') -eq 1 -and ((Field $_ 'speedAge') -ge 0 -or (Field $_ 'stealthAge') -ge 0) })
$maxSpeed = ($bf | ForEach-Object { Field $_ 'speedx' } | Measure-Object -Maximum).Maximum
$minAlpha = ($bf | ForEach-Object { Field $_ 'alpha' } | Measure-Object -Minimum).Minimum
Write-Host "Regulus: speed samples $(@($speed).Count), stealth samples $(@($stealth).Count), shield samples $(@($shielded).Count), stacked $(@($both).Count), max speed x$maxSpeed, min alpha $minAlpha, own shield $shield"
if ($both) { throw 'a mob carried more than one buff' }
if (-not $speed -and -not $stealth -and -not $shielded) { throw 'no buff ever seen' }
if ($speed -and $maxSpeed -lt 1.5) { throw "speed buff too low: $maxSpeed" }
if ($stealth -and $minAlpha -gt 0.40) { throw "stealth cloak too faint: $minAlpha" }
# Permanence: the buffed set only grows over the run.
$count = $lines | Where-Object { $_ -match '^R\d+ buffed' } | Group-Object { ($_ -split ' ')[0] } | ForEach-Object { $_.Count }
$prev = 0; foreach ($c in $count) { if ($c -lt $prev -and $true) { throw 'a buff wore off' } ; $prev = $c }
Write-Host 'Gravis / Regulus smoke passed'
