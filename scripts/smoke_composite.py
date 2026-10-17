"""Composite transparent smoke captures over wallpaper colours.

The game draws on a transparent framebuffer, so a capture PNG shows only the
HUD. This puts each PNG of a folder on solid backgrounds to check readability:

    python scripts/smoke_composite.py build/smoke/<tag> "#F3F3F3" "#808080"

Writes <folder>/composite_<RRGGBB>/<name>.png for each colour.
"""
import sys
from pathlib import Path

from PIL import Image


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    folder = Path(argv[1])
    pngs = sorted(folder.glob("*.png"))
    for colour in argv[2:]:
        hexv = colour.lstrip("#")
        rgb = tuple(int(hexv[i:i + 2], 16) for i in (0, 2, 4))
        out = folder / f"composite_{hexv.upper()}"
        out.mkdir(exist_ok=True)
        for png in pngs:
            src = Image.open(png).convert("RGBA")
            bg = Image.new("RGBA", src.size, rgb + (255,))
            bg.alpha_composite(src)
            bg.convert("RGB").save(out / png.name)
        print(f"{len(pngs)} → {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
