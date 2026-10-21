#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Privately export compact original status gump and five animal corpse poses."""
import argparse,json,struct,subprocess
from pathlib import Path
from export_journal import decode_gump,fnv
from export_population import read_layer,DIRECTION_MAP,ANIMALS

def main():
    from PIL import Image
    from ultima_sdk.uop import UopFile
    p=argparse.ArgumentParser(description=__doc__)
    for key in ('client','population','manifest','out'):p.add_argument('--'+key,type=Path,required=True)
    a=p.parse_args();a.out.mkdir(parents=True,exist_ok=True)
    u=UopFile(a.client/'gumpartLegacyMUL.uop','build/gumpartlegacymul/{0:D8}.tga',has_extra=True)
    original=decode_gump(u.read_raw(0x2a6c))
    panel=original.crop((20,20,548,156)).resize((300,82),Image.Resampling.NEAREST)
    panel.paste(original.crop((25,25,542,43)).resize((292,12),Image.Resampling.NEAREST),(4,3))
    for col,x in enumerate((40,124,207,288)):
        for row in range(3):
            box=(x,68+row*28,x+82,96+row*28)
            if col==3 and row==1:box=(453,154,535,178)
            panel.paste(original.crop(box).resize((60,20),Image.Resampling.NEAREST),(8+60*col,17+20*row))
    panel.paste(original.crop((453,68,535,151)).resize((44,60),Image.Resampling.NEAREST),(248,17))
    panel.paste(panel.crop((0,0,300,4)).transpose(Image.Transpose.FLIP_TOP_BOTTOM),(0,78))
    panel.save(a.out/'status.png');raw=panel.tobytes();(a.out/'status.bin').write_bytes(struct.pack('<4sHHII',b'UST1',300,82,len(raw),fnv(raw))+raw)
    pivots=[];names=list(ANIMALS);folder=a.out/'corpses';folder.mkdir(exist_ok=True)
    for number,name in enumerate(names):
        body=ANIMALS[name];frames=[]
        for direction,mirror in DIRECTION_MAP:
            seq=read_layer(a.client,body,2 if body<200 else 8,direction);image,cx,cy=seq[-1]
            canvas=Image.new('RGBA',(128,144));canvas.alpha_composite(image,(64-cx,139-image.height-cy))
            if mirror:canvas=canvas.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
            frames.append(canvas)
        boxes=[im.getbbox() for im in frames];assert all(boxes)
        bounds=(min(b[0] for b in boxes),min(b[1] for b in boxes),max(b[2] for b in boxes),max(b[3] for b in boxes));pivots.append((64-bounds[0],139-bounds[1]))
        for d,im in enumerate(frames):im.crop(bounds).save(folder/f'{number*8+d:02d}.png')
    (folder/'corpses.t3s').write_text('--atlas -f rgba5551 -z none\n'+''.join(f'{i:02d}.png\n' for i in range(40)))
    subprocess.run(['/opt/devkitpro/tools/bin/tex3ds','-i','corpses.t3s','-o',str((a.out/'corpses.t3x').resolve())],cwd=folder,check=True)
    styles=json.loads(a.manifest.read_text())['styles'];mapping=bytes(names.index(s['name']) if s['name'] in names else 255 for s in styles)
    data=b''.join(struct.pack('<hh',*v) for v in pivots)+mapping;atlas=(a.out/'corpses.t3x').read_bytes()
    (a.out/'corpses.bin').write_bytes(struct.pack('<4sHHIIII',b'UCR1',len(styles),5,fnv(a.population.read_bytes()),len(atlas),fnv(atlas),fnv(data))+data)
    print('Original status panel, five corpse types, forty death poses:',len(atlas),'atlas bytes')
if __name__=='__main__':main()
