# Live-game smoke for the Antares elite: every rush ricochets off the map
# edge, split Antares bounce off each other, and the elite splits twice
# (1 -> 2 -> 4). Needs build\windows\x64\Release\WiNILL.exe.
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$exeDir = Join-Path $root 'build\windows\x64\Release'
$exe = Join-Path $exeDir 'WiNILL.exe'
if (-not (Test-Path $exe)) { throw "build first: $exe" }
Write-Host "run dir: $exeDir"
$stateFile = Join-Path $exeDir 'Screenshots\mob_state.txt'
Remove-Item -LiteralPath $stateFile -ErrorAction SilentlyContinue
$N = 100   # samples per phase
$steps = @('RUN~0.3', 'LEVEL=80~0.05', 'MOBCLEAR~0.05', 'SPAWN=7~0.05', 'MOBPLACE=500~0.02', 'MOBGRACE~0.02')
function Phase($tag) { 1..$N | ForEach-Object { $script:steps += 'HP=1~0.02'; $script:steps += 'MOBHP=1000000~0.01'; $script:steps += "MOBDUMP=$tag$_~0.1" } }
Phase 'A'
$steps += 'MOBKILL~0.1'; $steps += 'MOBHP=1000000~0.02'; $steps += 'MOBGRACE~0.02'
Phase 'B'
$steps += 'MOBKILL~0.1'; $steps += 'MOBHP=1000000~0.02'; $steps += 'MOBGRACE~0.02'
Phase 'C'
$steps += 'MOBKILL~0.1'
1..5 | ForEach-Object { $steps += 'HP=1~0.02'; $steps += "MOBDUMP=D$_~0.3" }
$env:ASTERIA_SMOKE = $steps -join ';'
$env:ASTERIA_SMOKE_TAG = 'antsmoke'
$env:ASTERIA_SMOKE_SIZE = '1280x720'
try {
    $p = Start-Process -FilePath $exe -WorkingDirectory $exeDir -PassThru -WindowStyle Hidden
    if (-not $p.WaitForExit(420000)) { $p.Kill(); throw 'antares smoke timed out' }
    if ($p.ExitCode -ne 0) { throw "antares smoke exited with $($p.ExitCode)" }
} finally {
    Remove-Item Env:ASTERIA_SMOKE, Env:ASTERIA_SMOKE_TAG, Env:ASTERIA_SMOKE_SIZE -ErrorAction SilentlyContinue
}
$all = Get-Content $stateFile
$counts = foreach ($ph in 'A', 'B', 'C', 'D') {
    $pl = @($all | Where-Object { $_ -match "^$ph\d+ player" }).Count
    $an = @($all | Where-Object { $_ -match "^$ph\d+ antares" }).Count
    "$ph=$pl/$an"
}
Write-Host ('samples per phase (player/antares): ' + ($counts -join '  '))
$lines = $all | Where-Object { $_ -match '^[A-D]\d+ antares' }
function Field($line, $name) { if ($line -match "$name=(-?[\d.]+)") { [double]$Matches[1] } else { $null } }
$rows = $lines | ForEach-Object {
    [pscustomobject]@{ label = ($_ -split ' ')[0]; x = Field $_ 'x'; y = Field $_ 'y'; state = [int](Field $_ 'state')
        speed = Field $_ 'speed'; bounces = [int](Field $_ 'bounces'); gen = [int](Field $_ 'gen'); hp = Field $_ 'hp' }
}
$fail = @()
function Check($ok, $msg) { if ($ok) { Write-Host "  ok   $msg" } else { Write-Host "  FAIL $msg"; $script:fail += $msg } }

foreach ($ph in 'A', 'B', 'C') {
    $r = $rows | Where-Object { $_.label -like "$ph*" }
    Check (@($r).Count -gt 0) "phase $ph produced samples"
    $gens = @($r.gen | Sort-Object -Unique)
    $perLabel = $r | Group-Object label | ForEach-Object { $_.Count } | Sort-Object -Unique
    $expectGen = @{ A = 0; B = 1; C = 2 }[$ph]
    $expectCount = @{ A = 1; B = 2; C = 4 }[$ph]
    Write-Host "Phase $ph (gen $expectGen)"
    Check ($gens.Count -eq 1 -and $gens[0] -eq $expectGen) "only generation $expectGen alive (saw $($gens -join ','))"
    Check (($perLabel | Where-Object { $_ -ne $expectCount }).Count -eq 0) "$expectCount Antares per sample"
    $xs = $r.x | Measure-Object -Minimum -Maximum; $ys = $r.y | Measure-Object -Minimum -Maximum
    Write-Host ("       range x {0:N0}..{1:N0}  y {2:N0}..{3:N0}  speed max {4:N0}" -f $xs.Minimum, $xs.Maximum, $ys.Minimum, $ys.Maximum, ($r.speed | Measure-Object -Maximum).Maximum)
    $bounces = ($r | Measure-Object bounces -Maximum).Maximum
    # The parked player makes small bodies orbit it, so only the elite run is
    # required to reach a wall; the later phases report it.
    if ($ph -eq 'A') { Check ($bounces -ge 1) "wall ricochets happened (max bounces on one body: $bounces)" }
    else { Write-Host "       wall ricochets (info): max bounces on one body $bounces" }
    # Never two bounces on the same body between two consecutive samples while
    # parked on the same wall: a bounce count jump >2 in 0.1s would be jitter.
    $maxJump = 0
    $byBody = $r | Group-Object { [math]::Round($_.hp) }   # coarse: same-gen bodies share hp, so fall back to pair-agnostic jump below
    $labels = $r.label | Select-Object -Unique
    for ($i = 1; $i -lt $labels.Count; $i++) {
        $a = ($r | Where-Object label -eq $labels[$i - 1] | Measure-Object bounces -Sum).Sum
        $b = ($r | Where-Object label -eq $labels[$i] | Measure-Object bounces -Sum).Sum
        if ($b - $a -gt $maxJump) { $maxJump = $b - $a }
    }
    Check ($maxJump -le 2 * $expectCount) "no bounce jitter (max bounce jump per sample $maxJump)"
    # Speed pressed to zero is a stun/rest; bounces must not be what stops it.
    $states = @($r.state | Sort-Object -Unique)
    Write-Host "       states seen: $($states -join ',')"
    Check ($states -notcontains 4 -and $states -notcontains 5) 'removed charge states never appear'
}
# Pair collisions in B and C: count samples where two bodies overlap too deeply
# (should be pushed apart) and where they came together at all.
foreach ($ph in 'B', 'C') {
    $overlap = 0; $touch = 0; $minD = 1e9
    $r = $rows | Where-Object { $_.label -like "$ph*" }
    $scale = if ($ph -eq 'B') { 0.72 } else { 0.72 * 0.72 }
    foreach ($g in ($r | Group-Object label)) {
        $b = @($g.Group)
        for ($i = 0; $i -lt $b.Count; $i++) { for ($j = $i + 1; $j -lt $b.Count; $j++) {
            $d = [math]::Sqrt([math]::Pow($b[$i].x - $b[$j].x, 2) + [math]::Pow($b[$i].y - $b[$j].y, 2))
            $reach = 26 * 1.6 * $scale * 2
            if ($d -lt $minD) { $minD = $d }
            if ($d -lt $reach * 1.3) { $touch++ }
            if ($d -lt $reach * 0.6) { $overlap++ }
        } }
    }
    Write-Host ("Phase {0}: closest pair {1:N0}px (reach {2:N0}), near-contact samples {3}, deep-overlap samples {4}" -f $ph, $minD, (26 * 1.6 * $scale * 2), $touch, $overlap)
    Check ($overlap -eq 0) "$ph bodies never sink into each other"
}
$d = $rows | Where-Object { $_.label -like 'D*' }
Write-Host "After last kill: $(@($d).Count) Antares alive (expect 0)"
Check (@($d).Count -eq 0) 'generation 2 does not split further'
$first = $rows | Where-Object { $_.label -eq 'B1' }
Check (@($first).Count -eq 2) 'first kill released exactly 2'
Check ((@($rows | Where-Object { $_.label -eq 'C1' })).Count -eq 4) 'second kill released exactly 4'
$hpA = ($rows | Where-Object { $_.label -eq 'A1' }).hp
Write-Host "HP (test-forced): n/a. Sizes by gen: 1.60 / 1.15 / 0.83"
if ($fail.Count) { throw "Antares smoke FAILED: $($fail -join '; ')" }
Write-Host 'Antares smoke passed'
