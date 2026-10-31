# Buff-mark overlap check: each mob kind alone, captured with no buff, shield,
# speed, stealth and all three. Look at the contact sheet this writes:
# Screenshots\buffmarks_sheet.png (rows = kinds, columns = buffs).
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$exeDir = Join-Path $root 'build\windows\x64\Release'
$exe = Join-Path $exeDir 'WiNILL.exe'
if (-not (Test-Path $exe)) { throw "build first: $exe" }
$shots = Join-Path $exeDir 'Screenshots'
Remove-Item (Join-Path $shots 'buffmarks_*') -ErrorAction SilentlyContinue
$steps = @('RUN~0.3')
$kinds = @('0','1','2','4','5','7','8','SCOPE','3')   # Gravis last: its field drags the player
foreach ($k in $kinds) {
    $steps += @('MOBCLEAR~0.02', "SPAWN=$k~0.02", 'MOBGRACE~0.01', 'MOBGRID~0.02')
    foreach ($m in 0,1,2,4,7) { $steps += @("MOBBUFF=$m~0.01", 'MOBGRID~0.01', "S_${k}_$m~0.1") }
}
$env:ASTERIA_SMOKE = $steps -join ';'
$env:ASTERIA_SMOKE_TAG = 'buffmarks'
$env:ASTERIA_SMOKE_SIZE = '1280x720'
try {
    $p = Start-Process -FilePath $exe -WorkingDirectory $exeDir -PassThru -WindowStyle Hidden
    if (-not $p.WaitForExit(240000)) { $p.Kill(); throw 'buff marks smoke timed out' }
    if ($p.ExitCode -ne 0) { throw "buff marks smoke exited with $($p.ExitCode)" }
} finally {
    Remove-Item Env:ASTERIA_SMOKE, Env:ASTERIA_SMOKE_TAG, Env:ASTERIA_SMOKE_SIZE -ErrorAction SilentlyContinue
}
$py = @'
import glob, re, sys
from PIL import Image
d = sys.argv[1]
kinds = sys.argv[2].split(',')
masks = [0, 1, 2, 4, 7]
W, H = 340, 300
sheet = Image.new('RGB', (W * len(masks), H * len(kinds)))
for r, k in enumerate(kinds):
    for c, m in enumerate(masks):
        f = glob.glob(f'{d}/buffmarks_KR_*_S_{k}_{m}.png')
        if not f: continue
        im = Image.open(f[0]).convert('RGB')
        cx, cy = 640, 210
        sheet.paste(im.crop((cx - W // 2, cy - H // 2, cx + W // 2, cy + H // 2)), (c * W, r * H))
sheet.save(f'{d}/buffmarks_sheet.png')
'@
$tmp = Join-Path $env:TEMP 'buff_sheet.py'
Set-Content $tmp $py
python -I $tmp $shots ($kinds -join ',')
Write-Host "sheet: $shots\buffmarks_sheet.png"
