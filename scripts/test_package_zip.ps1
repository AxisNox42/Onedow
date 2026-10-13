# FIX-03 regression: both ZIP scripts package only scripts/package_manifest.txt.
# Builds a temporary fixture (real Resource + dummy binaries + user files that
# must stay out) under build/tests/package; bin/Onedow is never touched.
# zip_onedow.sh needs Info-ZIP `zip`, missing on Windows, so a stand-in on
# PATH zips the staged folder; Unix modes from it are not checked here (the
# macOS CI "Verify package" step checks the real zip's permission).
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$work = Join-Path $root 'build\tests\package'
$fixture = Join-Path $work 'fixture\Onedow'
$out = Join-Path $work 'out'
$bash = Join-Path $env:ProgramFiles 'Git\bin\bash.exe'
if (-not (Test-Path $bash)) { throw 'Git Bash required for the zip_onedow.sh path' }
if (Test-Path $work) { Remove-Item -LiteralPath $work -Recurse -Force }

# Distribution files.
New-Item -ItemType Directory -Force (Join-Path $fixture 'WindowsOS'), (Join-Path $fixture 'macOS'), $out | Out-Null
Copy-Item (Join-Path $root 'bin\Onedow\README.md') $fixture
Copy-Item (Join-Path $root 'bin\Onedow\macOS\README.txt') (Join-Path $fixture 'macOS')
Copy-Item -Recurse (Join-Path $root 'WiNILL\Resource') (Join-Path $fixture 'Resource')
Set-Content (Join-Path $fixture 'WindowsOS\Onedow.exe') 'dummy windows binary'
Set-Content (Join-Path $fixture 'macOS\Onedow') 'dummy macos binary'
# User and build files that must never be packaged.
$junk = 'onedow_save.cfg', 'WindowsOS\onedow_save.cfg', 'WindowsOS\Onedow.pdb', 'WindowsOS\Onedow.lib',
        'WindowsOS\Onedow.exp', 'WindowsOS\Resource\stale.txt', 'Screenshots\shot.png',
        'onedow_crash.log', 'onedow_crash_20261013_120000.dmp', 'onedow_gameplay_telemetry.txt',
        'macOS\.DS_Store'
foreach ($j in $junk) {
    $p = Join-Path $fixture $j
    New-Item -ItemType Directory -Force (Split-Path $p -Parent) | Out-Null
    Set-Content $p 'user data'
}

# Path 1: Windows release script.
$psZip = Join-Path $out 'ps.zip'
& (Join-Path $PSScriptRoot 'zip_onedow.ps1') -Source $fixture -Output $psZip | Out-Null
python (Join-Path $PSScriptRoot 'verify_package_zip.py') $psZip --require WindowsOS/Onedow.exe --require macOS/Onedow
if ($LASTEXITCODE -ne 0) { throw 'zip_onedow.ps1 package check failed' }

# Path 2: macOS release script (also what the CI Package step runs).
$shim = Join-Path $work 'zipshim'
New-Item -ItemType Directory -Force $shim | Out-Null
$shimPy = Join-Path $shim 'zipshim.py'
@'
import os, sys, zipfile
# Stand-in for `zip -r -q OUT DIR` (the only form zip_onedow.sh uses).
args = [a for a in sys.argv[1:] if not a.startswith("-")]
out, top = args[0], args[1]
with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
    for base, dirs, files in os.walk(top):
        dirs.sort()
        z.write(base, base + "/")
        for f in sorted(files):
            z.write(os.path.join(base, f))
'@ | Set-Content -Encoding ascii $shimPy
"#!/usr/bin/env bash`nexec python `"`$(dirname `"`$0`")/zipshim.py`" `"`$@`"`n" |
    ForEach-Object { [IO.File]::WriteAllText((Join-Path $shim 'zip'), $_) }
$shZip = Join-Path $out 'sh.zip'
function ToUnix([string]$p) {
    $p = $p -replace '\\', '/'
    if ($p -match '^([A-Za-z]):(.*)$') { return '/' + $Matches[1].ToLower() + $Matches[2] }
    return $p
}
$cmd = "export PATH=`"$(ToUnix $shim):`$PATH`"; bash `"$(ToUnix (Join-Path $PSScriptRoot 'zip_onedow.sh'))`" `"$(ToUnix $fixture)`" `"$(ToUnix $shZip)`""
& $bash -c $cmd | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'zip_onedow.sh failed' }
python (Join-Path $PSScriptRoot 'verify_package_zip.py') $shZip --require macOS/Onedow --no-exec-check
if ($LASTEXITCODE -ne 0) { throw 'zip_onedow.sh package check failed' }

# Both paths must ship the same files.
$psList = python (Join-Path $PSScriptRoot 'verify_package_zip.py') $psZip --list --no-exec-check | Select-Object -SkipLast 1
$shList = python (Join-Path $PSScriptRoot 'verify_package_zip.py') $shZip --list --no-exec-check | Select-Object -SkipLast 1
$diff = Compare-Object $psList $shList
if ($diff) { $diff | Format-Table | Out-String | Write-Host; throw 'ZIP contents differ between the two scripts' }
Write-Host "PASS: both ZIP scripts ship the same $($psList.Count) manifest files; $($junk.Count) user/build files excluded"
