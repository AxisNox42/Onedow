"""Compose actual deterministic smoke captures; requires Pillow. No synthetic frames."""
from pathlib import Path
import argparse
from PIL import Image, ImageDraw

parser = argparse.ArgumentParser()
parser.add_argument("source", type=Path)
parser.add_argument("output", type=Path)
args = parser.parse_args()
files = sorted(args.source.glob("*_GENESISASSAULT=*.png"))
if not files:
    raise SystemExit("No Genesis assault smoke frames")
args.output.mkdir(parents=True, exist_ok=True)
frames = []
for path in files:
    im = Image.open(path).convert("RGB")
    im.thumbnail((960, 600))
    ImageDraw.Draw(im).text((12, 45), "GENESIS assault | " + path.stem.split("=")[-1] + "s", fill="white")
    frames.append(im)
frames[0].save(args.output / "Assault_Preview.gif", save_all=True,
               append_images=frames[1:], duration=80, loop=0)
indices = [0, 3, 8, 15, 18, 23, 27, 36, 40, 43, 47, 54]
sheet = Image.new("RGB", (1280, 600))
for slot, index in enumerate(indices):
    frame = frames[min(index, len(frames)-1)].resize((320, 200))
    sheet.paste(frame, ((slot % 4)*320, (slot//4)*200))
sheet.save(args.output / "Motion_ContactSheet.png")
print(args.output / "Assault_Preview.gif")
