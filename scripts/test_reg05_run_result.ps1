$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$installer = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer"
$vswhere = Join-Path $installer 'vswhere.exe'
# vcvars64.bat resolves vswhere through PATH.
$env:PATH = "$installer;$env:PATH"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio C++ tools required' }
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$out = Join-Path $root 'build\tests\reg05-run-result'
New-Item -ItemType Directory -Force $out | Out-Null
Push-Location $root
try {
    $includes = '/I Include /I WiNILL\Core /I WiNILL\Core\Scenes /I WiNILL\Render /I WiNILL\Entity /I WiNILL\System'
    $command = "call `"$vcvars`" >nul && cl /nologo /utf-8 /std:c++17 /EHsc /MT /DNOMINMAX $includes tests\reg05_run_result_test.cpp /Fo`"$out\\`" /Fe`"$out\reg05_run_result_test.exe`""
    & cmd.exe /d /s /c $command
    if ($LASTEXITCODE -ne 0) { throw 'REG-05 build failed' }
} finally { Pop-Location }

# Run in an empty folder: the save path is relative, so no real save is near.
$sandbox = Join-Path $out 'sandbox'
if (Test-Path $sandbox) { Remove-Item -LiteralPath $sandbox -Recurse -Force }
New-Item -ItemType Directory -Force $sandbox | Out-Null
Push-Location $sandbox
try {
    & "$out\reg05_run_result_test.exe"
    if ($LASTEXITCODE -ne 0) { throw 'REG-05 run result regression failed' }
    if (Get-ChildItem -LiteralPath $sandbox) { throw 'the test wrote files (expected none)' }
} finally { Pop-Location }

# Callers' run-once guards (main.cpp).
Push-Location $root
try {
    $main = Get-Content 'WiNILL\main.cpp'
    $calls = Select-String -Path 'WiNILL\main.cpp' -Pattern '\bCommitRunResult\('
    if (@($calls).Count -ne 3) { throw "expected 3 CommitRunResult calls in main.cpp, found $(@($calls).Count)" }
    if (Select-String -Path 'WiNILL\main.cpp' -Pattern '\bRecordRunResult\(') { throw 'main.cpp records a run outside CommitRunResult' }
    # Abandon: returns at once when the run already ended.
    $abandon = (Select-String -Path 'WiNILL\main.cpp' -Pattern 'auto AbandonCurrentRun = ').LineNumber
    if ($main[$abandon] -notmatch 'currentState == GameState::GAMEOVER' -or $main[$abandon + 1] -notmatch 'return;') {
        throw 'AbandonCurrentRun no longer starts with the GAMEOVER guard'
    }
    # Run victories (all waves survived, or NORMA felled) commit once in
    # FinishRunVictory, then immediately leave RUNNING. Re-entry is blocked by
    # waveVictory and by the current-state check.
    $wave = ($calls | Where-Object { $_.LineNumber -lt $abandon } | Select-Object -First 1).LineNumber
    $waveStart = (Select-String -Path 'WiNILL\main.cpp' -Pattern '^static void FinishRunVictory\(').LineNumber
    $waveEnd = (Select-String -Path 'WiNILL\main.cpp' -Pattern '^static void ProcessWaveTransitions\(').LineNumber
    if ($wave -le $waveStart -or $wave -ge $waveEnd) { throw 'wave completion commit moved outside FinishRunVictory' }
    $waveBody = $main[($waveStart - 1)..($waveEnd - 2)] -join "`n"
    if ($waveBody -notmatch 'waveVictory \|\|.*currentState != GameState::RUNNING' -or
        $waveBody -notmatch 'waveVictory = true' -or
        $waveBody -notmatch 'currentState = GameState::GAMEOVER') {
        throw 'wave completion lost its run-once guards or GAMEOVER transition'
    }
    # Game over: committed inside the DYING timer-expiry branch, after the
    # state moved to GAMEOVER (so the branch cannot run again).
    $gameOver = ($calls | Select-Object -Last 1).LineNumber
    $window = $main[($gameOver - 25)..($gameOver - 1)] -join "`n"
    if ($window -notmatch 'g_DyingTimer <= 0\.0f' -or $window -notmatch 'currentState = GameState::GAMEOVER') {
        throw 'game-over commit no longer sits after the DYING -> GAMEOVER transition'
    }
    Write-Host 'source check: wave, abandon and DYING paths each have a run-once transition'
} finally { Pop-Location }
