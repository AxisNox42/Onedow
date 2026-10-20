"""Actual full-cycle captures, including the formerly skipped Rotor wait."""
from pathlib import Path
import argparse
from PIL import Image, ImageDraw

parser=argparse.ArgumentParser()
parser.add_argument('source',type=Path)
parser.add_argument('output',type=Path)
parser.add_argument('--focus-end',type=float,default=7.6)
args=parser.parse_args()
def age(p): return float(p.stem.rsplit('=',1)[-1])
files=sorted(args.source.glob('*_GENESISCYCLE=*.png'),key=age)
if not files: raise SystemExit('No GENESISCYCLE smoke captures')
args.output.mkdir(parents=True,exist_ok=True)
frames=[]
for path in files:
    with Image.open(path) as source:
        im=source.convert('RGB').crop((260,150,760,560)).resize((600,492))
    ImageDraw.Draw(im).text((10,10),f'GENESIS / SWARM -> ROTOR | {age(path):.2f}s',fill='white')
    frames.append(im)
frames[0].save(args.output/'Cycle_Preview.gif',save_all=True,
               append_images=frames[1:],duration=80,loop=0)
focus=[im for im,path in zip(frames,files) if 1.6<=age(path)<=args.focus_end]
focus[0].save(args.output/'WaitToRotor_Closeup.gif',save_all=True,
              append_images=focus[1:],duration=80,loop=0)
sheet=Image.new('RGB',(1200,738))
for slot in range(12):
    index=round(slot*(len(files)-1)/11)
    sheet.paste(frames[index].resize((300,246)),((slot%4)*300,(slot//4)*246))
sheet.save(args.output/'Transition_ContactSheet.png')
print(args.output/'WaitToRotor_Closeup.gif')
