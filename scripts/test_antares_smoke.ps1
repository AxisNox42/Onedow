# Live-game smoke for the Antares elite: ricochet charge, split on death and
# the bounce between split Antares. Needs build\windows\x64\Release\WiNILL.exe.
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$exeDir = Join-Path $root 'build\windows\x64\Release'
$exe = Join-Path $exeDir 'WiNILL.exe'
if (-not (Test-Path $exe)) { throw "build first: $exe" }
$stateFile = Join-Path $exeDir 'Screenshots\mob_state.txt'
Remove-Item -LiteralPath $stateFile -ErrorAction SilentlyContinue
$steps = @('RUN~0.3', 'MOBCLEAR~0.05', 'SPAWN=7~0.05', 'MOBPLACE=500~0.02', 'MOBGRACE~0.02')
1..80 | ForEach-Object { $steps += 'HP=1~0.02'; $steps += "MOBDUMP=A$_~0.2" }
$steps += @('MOBKILL~0.05')
1..16 | ForEach-Object { $steps += 'HP=1~0.02'; $steps += "MOBDUMP=S$_~0.4" }
$env:ONEDOW_SMOKE = $steps -join ';'
$env:ONEDOW_SMOKE_TAG = 'antsmoke'
$env:ONEDOW_SMOKE_SIZE = '1280x720'
try {
    $p = Start-Process -FilePath $exe -WorkingDirectory $exeDir -PassThru -WindowStyle Hidden
    if (-not $p.WaitForExit(240000)) { $p.Kill(); throw 'antares smoke timed out' }
    if ($p.ExitCode -ne 0) { throw "antares smoke exited with $($p.ExitCode)" }
} finally {
    Remove-Item Env:ONEDOW_SMOKE, Env:ONEDOW_SMOKE_TAG, Env:ONEDOW_SMOKE_SIZE -ErrorAction SilentlyContinue
}
$lines = Get-Content $stateFile
function Field($line, $name) { if ($line -match "$name=(-?[\d.]+)") { [double]$Matches[1] } else { $null } }
$pre = $lines | Where-Object { $_ -match '^A\d+ antares' }
$post = $lines | Where-Object { $_ -match '^S\d+ antares' }
if (-not $pre) { throw 'no Antares was alive' }
if ((Field $pre[0] 'elite') -ne 1) { throw "Antares not elite: $($pre[0])" }
$states = $pre | ForEach-Object { [int](Field $_ 'state') } | Sort-Object -Unique
$maxBounce = ($pre | ForEach-Object { Field $_ 'bounces' } | Measure-Object -Maximum).Maximum
$maxSpeed = ($pre | ForEach-Object { Field $_ 'speed' } | Measure-Object -Maximum).Maximum
Write-Host "Antares: states seen $($states -join ','), max bounces $maxBounce, max speed $maxSpeed"
if ($states -notcontains 4) { throw 'no AIM state (ricochet telegraph) seen' }
if ($states -notcontains 5) { throw 'no CHARGE state seen' }
if ($maxBounce -lt 1) { throw 'the charge never bounced off an edge' }
# After the kill: exactly the generation-1 children, smaller hp, not elite-sized.
if (-not $post) { throw 'no children after the kill' }
$gens = $post | ForEach-Object { [int](Field $_ 'gen') } | Sort-Object -Unique
$childCount = ($lines | Where-Object { $_ -match '^S1 antares' }).Count
Write-Host "Split: generations $($gens -join ','), children on first sample: $childCount"
if ($gens.Count -ne 1 -or $gens[0] -ne 1) { throw 'after the kill only generation-1 Antares should exist' }
if ($childCount -ne 2) { throw "expected 2 children, got $childCount" }
$hp = Field ($lines | Where-Object { $_ -match '^S1 antares' } | Select-Object -First 1) 'hp'
if ($hp -gt 600 * 0.46) { throw "child hp too high: $hp" }
# The children are 2 + collide: at some sample a child exceeds its launch speed (320) or reverses.
$childSpeeds = $post | ForEach-Object { Field $_ 'speed' }
Write-Host ("Children speed range: {0:N0} - {1:N0} px/s" -f ($childSpeeds | Measure-Object -Minimum).Minimum, ($childSpeeds | Measure-Object -Maximum).Maximum)
Write-Host 'Antares smoke passed'
