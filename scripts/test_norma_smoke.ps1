# Live-game smoke for NORMA's window patterns: after each pattern starts, the
# boss, its shots and every pane are sampled to Screenshots\norma_state.txt.
# Usage: test_norma_smoke.ps1 [THROW|RAM|DETONATE|REFUGE] [seconds] [closeAtSecond]
# closeAtSecond > 0 presses the X on every open pane then, to test defusing.
# Needs a Release x64 build: build\windows\x64\Release\WiNILL.exe
param([string]$Pattern = 'THROW', [int]$Seconds = 10, [double]$CloseAt = 0)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$exeDir = Join-Path $root 'build\windows\x64\Release'
$exe = Join-Path $exeDir 'WiNILL.exe'
if (-not (Test-Path $exe)) { throw "build first: $exe" }
$stateFile = Join-Path $exeDir 'Screenshots\norma_state.txt'
Remove-Item -LiteralPath $stateFile -ErrorAction SilentlyContinue
$steps = @('RUN~0.3', 'NORMATEST=PANES~1.5', "NORMATEST=$Pattern~0.02", 'NORMADUMP=T0~0.02')
1..($Seconds * 10) | ForEach-Object {
    if ($CloseAt -gt 0 -and $_ -eq [int]($CloseAt * 10)) { $steps += 'NORMACLOSE~0.02' }
    $steps += "NORMADUMP=T$_~0.1"
}
$env:ASTERIA_SMOKE = $steps -join ';'
$env:ASTERIA_SMOKE_TAG = 'normasmoke'
$env:ASTERIA_SMOKE_SIZE = '1280x720'
try {
    $p = Start-Process -FilePath $exe -WorkingDirectory $exeDir -PassThru -WindowStyle Hidden
    if (-not $p.WaitForExit(120000)) { $p.Kill(); throw 'norma smoke timed out' }
    if ($p.ExitCode -ne 0) { throw "norma smoke exited with $($p.ExitCode)" }
} finally {
    Remove-Item Env:ASTERIA_SMOKE, Env:ASTERIA_SMOKE_TAG, Env:ASTERIA_SMOKE_SIZE -ErrorAction SilentlyContinue
}
Get-Content $stateFile
