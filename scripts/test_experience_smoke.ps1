$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$exeDir = Join-Path $root 'build\windows\x64\Release'
$exe = Join-Path $exeDir 'WiNILL.exe'
if (-not (Test-Path $exe)) { throw "build first: $exe" }
$stateFile = Join-Path $exeDir 'Screenshots\reg04_state.txt'
Remove-Item -LiteralPath $stateFile -ErrorAction SilentlyContinue
$env:ASTERIA_SMOKE = 'RUN~0.3;XP=5.999~0.05;REG04DUMP=BEFORE~0.05;XP=0.001~0.05;REG04DUMP=AFTER~0.05;RUN~0.3;LEVEL=45~0.05;XP=6.123~0.05;REG04DUMP=CAP~0.05;RUN~0.3;XP=120.125~0.05;REG04DUMP=MULTI~0.05;STARDUST_NEAR=0.001~2.0;REG04DUMP=PICKUP~0.05'
$env:ASTERIA_SMOKE_TAG = 'xp'
$env:ASTERIA_SMOKE_SIZE = '1280x720'
try {
    $p = Start-Process -FilePath $exe -WorkingDirectory $exeDir -PassThru -WindowStyle Hidden
    if (-not $p.WaitForExit(120000)) { $p.Kill(); throw 'XP smoke timed out' }
    if ($p.ExitCode -ne 0) { throw "XP smoke exited with $($p.ExitCode)" }
} finally {
    Remove-Item Env:ASTERIA_SMOKE, Env:ASTERIA_SMOKE_TAG, Env:ASTERIA_SMOKE_SIZE -ErrorAction SilentlyContinue
    Get-ChildItem (Join-Path $exeDir 'Screenshots') -Filter 'xp_*.png' -ErrorAction SilentlyContinue | Remove-Item
}
$lines = Get-Content $stateFile
$before = $lines | Where-Object { $_ -match '^BEFORE ' } | Select-Object -Last 1
$after = $lines | Where-Object { $_ -match '^AFTER ' } | Select-Object -Last 1
$cap = $lines | Where-Object { $_ -match '^CAP ' } | Select-Object -Last 1
$multi = $lines | Where-Object { $_ -match '^MULTI ' } | Select-Object -Last 1
$pickup = $lines | Where-Object { $_ -match '^PICKUP ' } | Select-Object -Last 1
if ($before -notmatch 'xp=5\.999 level=1') { throw "before threshold: $before" }
if ($after -notmatch 'xp=0\.000 level=2') { throw "after threshold: $after" }
if ($cap -notmatch 'xp=0\.000 level=45') { throw "level cap: $cap" }
if ($multi -notmatch 'xp=0\.125 level=4') { throw "multi-level pickup: $multi" }
if ($pickup -notmatch 'xp=0\.001 level=1') { throw "fractional pickup: $pickup" }
Write-Host $before
Write-Host $after
Write-Host $cap
Write-Host $multi
Write-Host $pickup
Write-Host 'XP threshold and cap smoke passed'
