#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Export private dressed NPC walk/idle sprites from original MUL layers."""
import argparse
import json
from pathlib import Path
import struct
import shutil
import subprocess
from export_avatar import decode_frame,select_frames,DIRECTION_MAP

def layer(client,body,action,direction):
    idx=client/'anim.idx';mul=client/'anim.mul';entry=35000+(body-400)*175+action*5+direction
    with idx.open('rb') as f:f.seek(entry*12);raw=f.read(12)
    if len(raw)!=12:raise ValueError('Missing NPC animation index')
    offset,size,_=struct.unpack('<iii',raw)
    if offset<0 or size<520 or size>48*1024*1024:raise ValueError(f'Missing NPC layer {body}/{action}/{direction}')
    with mul.open('rb') as f:f.seek(offset);data=f.read(size)
    if len(data)!=size:raise ValueError('Truncated NPC layer')
    palette=struct.unpack_from('<256H',data);count=struct.unpack_from('<I',data,512)[0]
    if not 1<=count<=200:raise ValueError('Invalid NPC frame count')
    result=[]
    for i in range(count):
        start=512+struct.unpack_from('<I',data,516+4*i)[0]
        cx,cy,w,h=struct.unpack_from('<hhhh',data,start)
        result.append((decode_frame(data,start,shared_palette=palette),cx,cy))
    return result

def hue_image(image,hues,hue):
    from PIL import Image
    entry=hue-1;start=(entry//8)*708+4+(entry%8)*88
    if start+64>len(hues):raise ValueError('Missing original hue')
    colors=struct.unpack_from('<32H',hues,start);out=[]
    for r,g,b,a in image.getdata():
        c=colors[r>>3];out.append((((c>>10)&31)*255//31,((c>>5)&31)*255//31,(c&31)*255//31,a))
    result=Image.new('RGBA',image.size);result.putdata(out);return result

def main():
    from PIL import Image,ImageOps,ImageDraw
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--client',type=Path,required=True);p.add_argument('--out',type=Path,required=True);args=p.parse_args()
    if args.out.exists():raise FileExistsError(args.out)
    hues=(args.client/'hues.mul').read_bytes();args.out.mkdir(parents=True)
    cache={}
    for body in (400,477,469,701):
        for action in (0,4):
            for d in range(5):cache[body,action,d]=layer(args.client,body,action,d)
    manifests=[]
    for style,robe_hue in enumerate((0x21,0x59,0x3F)):
        folder=args.out/f'style{style}';folder.mkdir();frames=[]
        for direction,mirror in DIRECTION_MAP:
            for frame in range(5):
                action=4 if frame==0 else 0;canvas=Image.new('RGBA',(96,112))
                for body,hue in ((400,0x3EA),(477,0x455),(469,robe_hue),(701,0x455)):
                    sequence=cache[body,action,direction];image,cx,cy=sequence[0 if frame==0 else ((frame-1)*len(sequence))//4]
                    image=hue_image(image,hues,hue)
                    # UO's shared animation anchor keeps wearable layers aligned.
                    canvas.alpha_composite(image,(48-cx,107-image.height-cy))
                if mirror:canvas=ImageOps.mirror(canvas)
                frames.append(canvas)
        for i,image in enumerate(frames):image.save(folder/f'{i:02d}.png')
        (folder/'npc.t3s').write_text('--atlas -f rgba5551 -z none\n'+''.join(f'{i:02d}.png\n' for i in range(40)))
        output=(args.out/f'npc_{style:02d}.t3x').resolve()
        subprocess.run([shutil.which('tex3ds') or '/opt/devkitpro/tools/bin/tex3ds','-i','npc.t3s','-o',str(output)],cwd=folder,check=True)
        preview=Image.new('RGB',(480,8*112),(60,55,48))
        for i,image in enumerate(frames):preview.paste(image,((i%5)*96,(i//5)*112),image)
        preview.save(args.out/f'npc_{style:02d}-preview.png')
        manifests.append(dict(style=style,body=400,boots=477,robe=469,hair=701,robe_hue=robe_hue,walk_action=0,idle_action=4,frames=40))
    (args.out/'manifest.json').write_text(json.dumps(manifests,indent=2)+'\n')
    print('Exported 3 dressed NPC styles, 40 original frames each')
if __name__=='__main__':main()
