"""Compose actual Rotor smoke frames; Pillow required, no synthetic motion."""
from pathlib import Path
import argparse
from PIL import Image, ImageDraw

parser=argparse.ArgumentParser()
parser.add_argument('source',type=Path)
parser.add_argument('output',type=Path)
parser.add_argument('--baseline',type=Path)
args=parser.parse_args()
def age(path): return float(path.stem.rsplit('=',1)[-1])
files=sorted(args.source.glob('*_GENESISROTOR=*.png'),key=age)
if not files: raise SystemExit('No GENESISROTOR smoke captures')
args.output.mkdir(parents=True,exist_ok=True)
frame_ms=round((age(files[1])-age(files[0]))*1000) if len(files)>1 else 40
frames=[]
bombardment=[]
closeups=[]
production=[]
comparisons=[]
old={round(age(p)*1000):p for p in args.baseline.glob('*_GENESISROTOR=*.png')} if args.baseline else {}
crop=(260,150,720,560) # fixed 1440x900 smoke fixture: body, rear cradle and muzzle
for file in files:
    im=Image.open(file).convert('RGB')
    if 0.72<=age(file)<=3.60:
        focus=im.crop((300,200,1100,700)).resize((960,600))
        ImageDraw.Draw(focus).text((10,10),f'ROTOR BOMBARDMENT | {age(file):.2f}s',fill='white')
        bombardment.append(focus)
    if 0.72<=age(file)<=1.64:
        focus=im.crop(crop).resize((690,615))
        ImageDraw.Draw(focus).text((10,10),f'FORM -> TRANSFER -> LAUNCH | {age(file):.2f}s',fill='white')
        production.append(focus)
    if age(file)<=1.24:
        focus=im.crop(crop).resize((690,615))
        ImageDraw.Draw(focus).text((10,10),f'PREP 1.0s | {age(file):.2f}s',fill='white')
        closeups.append(focus)
        if round(age(file)*1000) in old:
            before=Image.open(old[round(age(file)*1000)]).convert('RGB').crop(crop)
            pair=Image.new('RGB',(960,450))
            pair.paste(before,(10,30)); pair.paste(im.crop(crop),(490,30))
            draw=ImageDraw.Draw(pair)
            draw.text((12,10),'BEFORE: 0.4s',fill='white')
            draw.text((492,10),f'AFTER: 1.0s | {age(file):.2f}s',fill='#80e0ec')
            comparisons.append(pair)
    im.thumbnail((960,600))
    ImageDraw.Draw(im).text((12,45),'GENESIS Rotor | '+file.stem.split('=')[-1]+'s',fill='white')
    frames.append(im)
frames[0].save(args.output/'Rotor_Preview.gif',save_all=True,
               append_images=frames[1:],duration=frame_ms,loop=0)
closeups[0].save(args.output/'Prep_Closeup.gif',save_all=True,
                  append_images=closeups[1:],duration=frame_ms,loop=0)
if production:
    production[0].save(args.output/'Production_Closeup.gif',save_all=True,
                       append_images=production[1:],duration=frame_ms,loop=0)
if bombardment:
    bombardment[0].save(args.output/'Bombardment_Closeup.gif',save_all=True,
                        append_images=bombardment[1:],duration=frame_ms,loop=0)
if comparisons:
    comparisons[0].save(args.output/'Prep_BeforeAfter.gif',save_all=True,
                         append_images=comparisons[1:],duration=frame_ms,loop=0)
sheet=Image.new('RGB',(1280,600))
for slot in range(12):
    index=round(slot*(len(frames)-1)/11)
    sheet.paste(frames[index].resize((320,200)),((slot%4)*320,(slot//4)*200))
sheet.save(args.output/'Motion_ContactSheet.png')
print(args.output/'Rotor_Preview.gif')
