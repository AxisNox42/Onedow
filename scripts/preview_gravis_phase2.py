"""Compose real GRAVISREBIRTH smoke frames; requires Pillow."""
from pathlib import Path
import argparse
from PIL import Image, ImageDraw
parser=argparse.ArgumentParser()
parser.add_argument('source',type=Path)
parser.add_argument('output',type=Path)
parser.add_argument('--tag',default='*')
args=parser.parse_args()
files=sorted(args.source.glob(args.tag+'_*_GRAVISREBIRTH=*.png'),key=lambda p:float(p.stem.rsplit('=',1)[1]))
if not files: raise SystemExit('No Gravis captures')
args.output.mkdir(parents=True,exist_ok=True)
frames=[]
for p in files:
    t=float(p.stem.rsplit('=',1)[1])
    im=Image.open(p).convert('RGB')
    # Fixed 1280x900 fixture: original body at (538,387).
    crop=im.crop((298,147,778,627))
    draw=ImageDraw.Draw(crop)
    label='BLACKOUT / REASSEMBLY' if t<2.35 else 'UNSTABLE STAR / CONTINUOUS EJECTA'
    draw.rectangle((0,0,480,34),fill='#10101c')
    draw.text((10,10),f'{label} | {t:.2f}s',fill='#f0c4e8')
    frames.append(crop)
    if min(abs(t-a) for a in [0,0.5,1.2,2.2,3,4.2,5.8,7])<0.011:
        im.save(args.output/f'Gravis_{t:.2f}s.png',optimize=True)
frames[0].save(args.output/'Gravis_Phase2_Preview.gif',save_all=True,append_images=frames[1:],duration=100,loop=0)
choices=[0,0.5,1.2,2.2,3,4.2,5.8,7]
sheet=Image.new('RGB',(960,480))
for i,t in enumerate(choices):
    j=min(range(len(files)),key=lambda j:abs(float(files[j].stem.rsplit('=',1)[1])-t))
    sheet.paste(frames[j].resize((240,240)),((i%4)*240,(i//4)*240))
sheet.save(args.output/'Gravis_Phase2_ContactSheet.png')
print(args.output/'Gravis_Phase2_Preview.gif')
