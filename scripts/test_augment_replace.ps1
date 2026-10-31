$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$exeDir = Join-Path $root 'build\windows\x64\Release'
$exe = Join-Path $exeDir 'WiNILL.exe'
$stateFile = Join-Path $exeDir 'Screenshots\reg04_state.txt'
if (-not (Test-Path $exe)) { throw "build first: $exe" }
Remove-Item -LiteralPath $stateFile -ErrorAction SilentlyContinue
$env:ASTERIA_SMOKE = 'RUN~0.3;REG04=PICK_REPLACE~0.3;REG04DUMP=RESUMED~0.05;REG04=PICK_CANCEL~0.05;REG04=PICK_CHAIN~0.05;REG04=PICK_REWARD~0.05'
$env:ASTERIA_SMOKE_TAG = 'augment_replace'
$env:ASTERIA_SMOKE_SIZE = '1280x720'
try {
    $p = Start-Process -FilePath $exe -WorkingDirectory $exeDir -PassThru -WindowStyle Hidden
    if (-not $p.WaitForExit(60000)) { $p.Kill(); throw 'augment replacement smoke timed out' }
    if ($p.ExitCode -ne 0) { throw "augment replacement exited with $($p.ExitCode)" }
} finally {
    Remove-Item Env:ASTERIA_SMOKE, Env:ASTERIA_SMOKE_TAG, Env:ASTERIA_SMOKE_SIZE -ErrorAction SilentlyContinue
    Get-ChildItem (Join-Path $exeDir 'Screenshots') -Filter 'augment_replace_*.png' -ErrorAction SilentlyContinue | Remove-Item
}
$snap = @{}
foreach ($line in Get-Content $stateFile) {
    $snap[($line -split ' ')[0]] = $line
}
foreach ($label in @('PICK_OPEN', 'PICK_CHAIN_WAIT')) {
    if ($snap[$label] -notmatch ' state=10 .* rewards=[12] ') { throw "replacement must stay open: $($snap[$label])" }
}
foreach ($label in @('PICK_REPLACE', 'PICK_CANCEL', 'PICK_CHAIN')) {
    if ($snap[$label] -notmatch ' state=6 .* queue=0 .* rewards=0 ') { throw "must resume combat: $($snap[$label])" }
    if ($snap[$label] -notmatch ' grace=0\.50 ') { throw "missing post-pick protection: $($snap[$label])" }
}
if ($snap.PICK_REWARD -notmatch ' state=9 .* rewards=1 ') { throw "next reward must open: $($snap.PICK_REWARD)" }
$resumedTime = [double]([regex]::Match($snap.RESUMED, ' time=([0-9.]+)').Groups[1].Value)
if ($snap.RESUMED -notmatch ' state=6 ' -or $resumedTime -lt 0.2) { throw "combat clock did not resume: $($snap.RESUMED)" }
Write-Host 'Augment replacement, cancellation, chained effects and queued rewards passed'
