# Quasar beam smoothness smoke: replays the beam at many ages (QUASARBEAM=<s>),
# then measures how much the picture changes from one frame to the next. A pop
# (the band vanishing, the beam cutting out, a width jump) shows as a spike in
# that change; a smooth animation has no spike next to its neighbours.
# Writes Screenshots\beamsmooth_report.txt and beamsmooth_sheet.png.
# Needs a Release x64 build: build\windows\x64\Release\WiNILL.exe
param(
    [string]$Tag = 'beamsmooth',
    [string]$Reflect = '1',
    [double]$Step = 0.1,
    [double]$From = 0.3,
    [double]$To = 6.0,
    [switch]$AnalyzeOnly
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$exeDir = Join-Path $root 'build\windows\x64\Release'
$exe = Join-Path $exeDir 'WiNILL.exe'
if (-not (Test-Path $exe)) { throw "build first: $exe" }
$shots = Join-Path $exeDir 'Screenshots'
if (-not $AnalyzeOnly) {
Remove-Item (Join-Path $shots "${Tag}_*") -ErrorAction SilentlyContinue
$steps = @('RUN~0.3')
$ages = @()
for ($a = $From; $a -le $To + 1e-6; $a += $Step) { $ages += [math]::Round($a, 2) }
foreach ($a in $ages) { $steps += ("QUASARBEAM={0:0.00}~0.04" -f $a) }
$env:ASTERIA_SMOKE = $steps -join ';'
$env:ASTERIA_SMOKE_TAG = $Tag
$env:ASTERIA_SMOKE_SIZE = '1280x720'
$env:ASTERIA_QREFLECT = $Reflect
try {
    $p = Start-Process -FilePath $exe -WorkingDirectory $exeDir -PassThru -WindowStyle Hidden
    if (-not $p.WaitForExit(600000)) { $p.Kill(); throw 'beam smooth smoke timed out' }
    if ($p.ExitCode -ne 0) { throw "beam smooth smoke exited with $($p.ExitCode)" }
} finally {
    Remove-Item Env:ASTERIA_SMOKE, Env:ASTERIA_SMOKE_TAG, Env:ASTERIA_SMOKE_SIZE, Env:ASTERIA_QREFLECT -ErrorAction SilentlyContinue
}
}
$py = @'
import glob, re, sys
import numpy as np
from PIL import Image
d, tag = sys.argv[1], sys.argv[2]
files = sorted(glob.glob(f'{d}/{tag}_KR_*_QUASARBEAM=*.png'),
               key=lambda f: float(re.search(r'QUASARBEAM=(\d+\.\d+)', f).group(1)))
ages = [float(re.search(r'QUASARBEAM=(\d+\.\d+)', f).group(1)) for f in files]
ims = [np.asarray(Image.open(f).convert('RGB').resize((640, 360)), dtype=np.float32) for f in files]
# Beam-ish energy: bright pixels of the quasar blue family, away from the HUD strip.
def energy(im):
    r, g, b = im[..., 0], im[..., 1], im[..., 2]
    m = (b > 120) & (b > r + 25)
    m[:40, :] = False; m[300:, :] = False
    return float(m.sum())
en = [energy(im) for im in ims]
diff = [0.0] + [float(np.abs(ims[i] - ims[i - 1]).mean()) for i in range(1, len(ims))]
dd = [0.0] + [abs(diff[i] - diff[i - 1]) for i in range(1, len(diff))]
en_jump = [0.0] + [abs(en[i] - en[i - 1]) / max(1.0, max(en[i], en[i - 1])) for i in range(1, len(en))]
lines = ['age   energy   frame-diff   diff-delta   energy-jump']
for i, a in enumerate(ages):
    lines.append(f'{a:4.2f}  {en[i]:7.0f}  {diff[i]:9.3f}   {dd[i]:9.3f}   {en_jump[i]:8.3f}')
# Soft energy: how much blue light is on the playfield (continuous, no threshold).
def soft(im):
    e = np.clip(im[..., 2] - 25.0, 0, None)
    e[:40, :] = 0; e[300:, :] = 0
    return float(e.mean())
se = [soft(im) for im in ims]
dips, cliffs = [], []
for i in range(1, len(se) - 1):
    lo = min(se[i - 1], se[i + 1])
    if se[i] < 0.7 * lo and lo > 0.5: dips.append((ages[i], se[i], lo))
for i in range(1, len(se)):
    if se[i - 1] > 2.0 and se[i] < 0.35 * se[i - 1]: cliffs.append((ages[i], se[i - 1], se[i]))
lines_extra = ['', 'soft energy per frame: ' + ' '.join(f'{a:.1f}:{v:.1f}' for a, v in zip(ages, se))]
lines_extra.append('DIPS (a frame much darker than both neighbours = a blink): ' + (', '.join(f'{a:.2f}s' for a, _, _ in dips) if dips else 'none'))
lines_extra.append('CLIFFS (energy loses >65% in one frame = the beam cutting out): ' + (', '.join(f'{a:.2f}s({p:.1f}->{c:.1f})' for a, p, c in cliffs) if cliffs else 'none'))
# Sustain window (the beam is up, well clear of the onset rift and the fade-out):
# MOTION = how much the picture changes between neighbouring frames (0 = a frozen
# beam); LIT = share of the playfield with any light (how far the glow spreads);
# HALO = mean brightness of pixels that are lit but not part of the bright core.
sus = [i for i, a in enumerate(ages) if 2.0 <= a <= 4.0]
if len(sus) > 2:
    motion = float(np.mean([diff[i] for i in sus if i > 0]))
    def playfield(im): return im[40:300, :, :]
    lit = float(np.mean([(playfield(ims[i])[..., 2] > 45).mean() for i in sus]))
    def halo(im):
        p = playfield(im)[..., 2]
        m = (p > 45) & (p < 150)
        return float(p[m].mean()) if m.any() else 0.0
    halo_v = float(np.mean([halo(ims[i]) for i in sus]))
    lines_extra.append(f'SUSTAIN (2.0-4.0 s): MOTION {motion:.3f}   LIT {lit:.3f}   HALO {halo_v:.1f}')
    # Telegraph vs beam: how much brighter the firing beam is than the warning band
    # that came just before it (lock window ends where the beam fires, ~1.55 s).
    lock = [se[i] for i, a in enumerate(ages) if 1.25 <= a <= 1.50]
    if lock:
        lock_m = float(np.mean(lock)); sus_m = float(np.mean([se[i] for i in sus]))
        lines_extra.append(f'CONTRAST (soft energy): lock band {lock_m:.1f}  ->  firing beam {sus_m:.1f}   ratio x{sus_m / max(lock_m, 0.1):.2f}')
# Pop detector: a frame whose energy change dwarfs its neighbours' (> 3x the median step, and big).
steps = np.array(en_jump[1:])
med = float(np.median(steps)) if len(steps) else 0.0
pops = [(ages[i], en_jump[i]) for i in range(1, len(ages)) if en_jump[i] > max(0.25, 3.0 * med)]
lines.append('')
lines.append(f'median energy step {med:.3f}; max energy step {steps.max():.3f}')
lines.append('POPS (energy change > 25% and > 3x median): ' + (', '.join(f'{a:.2f}s({j:.2f})' for a, j in pops) if pops else 'none'))
lines += lines_extra
open(f'{d}/{tag}_report.txt', 'w').write('\n'.join(lines))
print('\n'.join(lines_extra[1:]))
# contact sheet of every 3rd frame
pick = ims[::3][:24]
cols = 4
rows = (len(pick) + cols - 1) // cols
sheet = Image.new('RGB', (cols * 320, rows * 180))
for k, im in enumerate(pick):
    sheet.paste(Image.fromarray(im.astype(np.uint8)).resize((320, 180)), ((k % cols) * 320, (k // cols) * 180))
sheet.save(f'{d}/{tag}_sheet.png')
'@
$tmp = Join-Path $env:TEMP 'beam_smooth.py'
Set-Content $tmp $py
python -I $tmp $shots $Tag
