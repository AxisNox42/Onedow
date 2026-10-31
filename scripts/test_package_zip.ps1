# FIX-03 regression: both ZIP scripts package only scripts/package_manifest.txt.
# Builds a temporary fixture (real Resource + dummy binaries + user files that
# must stay out) under build/tests/package; bin/Asteria is never touched.
# zip_asteria.sh needs Info-ZIP `zip`, missing on Windows, so a stand-in on
# PATH zips the staged folder; Unix modes from it are not checked here (the
# macOS CI "Verify package" step checks the real zip's permission).
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$work = Join-Path $root 'build\tests\package'
$fixture = Join-Path $work 'fixture\Asteria'
$out = Join-Path $work 'out'
$bash = Join-Path $env:ProgramFiles 'Git\bin\bash.exe'
if (-not (Test-Path $bash)) { throw 'Git Bash required for the zip_asteria.sh path' }
if (Test-Path $work) { Remove-Item -LiteralPath $work -Recurse -Force }

# Distribution files.
New-Item -ItemType Directory -Force (Join-Path $fixture 'WindowsOS'), (Join-Path $fixture 'macOS'), $out | Out-Null
Copy-Item (Join-Path $root 'bin\Asteria\README.md') $fixture
Copy-Item (Join-Path $root 'bin\Asteria\macOS\README.txt') (Join-Path $fixture 'macOS')
Copy-Item -Recurse (Join-Path $root 'WiNILL\Resource') (Join-Path $fixture 'Resource')
Set-Content (Join-Path $fixture 'WindowsOS\Asteria.exe') 'dummy windows binary'
Set-Content (Join-Path $fixture 'macOS\Asteria') 'dummy macos binary'
# User and build files that must never be packaged.
$junk = 'asteria_save.cfg', 'WindowsOS\asteria_save.cfg', 'WindowsOS\Asteria.pdb', 'WindowsOS\Asteria.lib',
        'WindowsOS\Asteria.exp', 'WindowsOS\Resource\stale.txt', 'Screenshots\shot.png',
        'asteria_crash.log', 'asteria_crash_20261013_120000.dmp', 'asteria_gameplay_telemetry.txt',
        'macOS\.DS_Store'
foreach ($j in $junk) {
    $p = Join-Path $fixture $j
    New-Item -ItemType Directory -Force (Split-Path $p -Parent) | Out-Null
    Set-Content $p 'user data'
}

# Path 1: Windows release script.
$psZip = Join-Path $out 'ps.zip'
& (Join-Path $PSScriptRoot 'zip_asteria.ps1') -Source $fixture -Output $psZip | Out-Null
python (Join-Path $PSScriptRoot 'verify_package_zip.py') $psZip --require WindowsOS/Asteria.exe --require macOS/Asteria
if ($LASTEXITCODE -ne 0) { throw 'zip_asteria.ps1 package check failed' }

# Path 2: macOS release script (also what the CI Package step runs).
$shim = Join-Path $work 'zipshim'
New-Item -ItemType Directory -Force $shim | Out-Null
$shimPy = Join-Path $shim 'zipshim.py'
@'
import os, sys, zipfile
# Stand-in for `zip -r -q OUT DIR` (the only form zip_asteria.sh uses).
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
$cmd = "export PATH=`"$(ToUnix $shim):`$PATH`"; bash `"$(ToUnix (Join-Path $PSScriptRoot 'zip_asteria.sh'))`" `"$(ToUnix $fixture)`" `"$(ToUnix $shZip)`""
& $bash -c $cmd | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'zip_asteria.sh failed' }
python (Join-Path $PSScriptRoot 'verify_package_zip.py') $shZip --require macOS/Asteria --no-exec-check
if ($LASTEXITCODE -ne 0) { throw 'zip_asteria.sh package check failed' }

# Both paths must ship the same files.
$psList = python (Join-Path $PSScriptRoot 'verify_package_zip.py') $psZip --list --no-exec-check | Select-Object -SkipLast 1
$shList = python (Join-Path $PSScriptRoot 'verify_package_zip.py') $shZip --list --no-exec-check | Select-Object -SkipLast 1
$diff = Compare-Object $psList $shList
if ($diff) { $diff | Format-Table | Out-String | Write-Host; throw 'ZIP contents differ between the two scripts' }
Write-Host "PASS: both ZIP scripts ship the same $($psList.Count) manifest files; $($junk.Count) user/build files excluded"
