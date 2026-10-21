#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Privately compose original male UO animation layers and matching paperdolls."""
import argparse,json,shutil,struct,subprocess
from pathlib import Path
from export_avatar import DIRECTION_MAP,decode_frame
from export_npcs import hue_image
from export_journal import decode_gump,fnv

def layer(client,body,action,direction):
    from PIL import Image
    index=35000+(body-400)*175+action*5+direction
    with (client/'anim.idx').open('rb') as f:
        f.seek(index*12);offset,size,_=struct.unpack('<iii',f.read(12))
    if offset<0 or not 520<=size<=48*1024*1024:raise ValueError('Missing equipment animation')
    with (client/'anim.mul').open('rb') as f:f.seek(offset);data=f.read(size)
    if len(data)!=size:raise ValueError('Truncated equipment animation')
    palette=struct.unpack_from('<256H',data);count=struct.unpack_from('<I',data,512)[0]
    if not 1<=count<=200:raise ValueError('Invalid equipment frame count')
    result=[]
    for i in range(count):
        pos=512+struct.unpack_from('<I',data,516+i*4)[0]
        cx,cy,w,h=struct.unpack_from('<hhhh',data,pos)
        # Original weapon art legitimately hides a hand behind the body.
        if w==0 or h==0:
            if w<0 or h<0 or max(w,h)>512:raise ValueError('Invalid empty layer')
            im=Image.new('RGBA',(1,1))
        else:im=decode_frame(data,pos,shared_palette=palette)
        result.append((im,cx,cy))
    return result

LAYERS=((4,477,0x455),(2,469,0x59),(1,622,0))
def main():
    from PIL import Image,ImageOps
    from ultima_sdk.uop import UopFile
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--client',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
    if a.out.exists():raise FileExistsError(a.out)
    a.out.mkdir(parents=True);hues=(a.client/'hues.mul').read_bytes();cache={}
    for body in (400,477,469,622):
        for action in (0,1,2,3,4):
            for d in range(5):cache[body,action,d]=layer(a.client,body,action,d)
    u=UopFile(a.client/'gumpartLegacyMUL.uop','build/gumpartlegacymul/{0:D8}.tga',has_extra=True)
    gumps={g:decode_gump(u.read_raw(g)) for g in (12,50477,50469,50622)}
    dolls=[];hashes=[]
    for mask in range(8):
        folder=a.out/f'outfit{mask}';folder.mkdir();frames=[]
        samples=[]
        walk,run=(1,3) if mask&1 else (0,2)
        for d,mirror in DIRECTION_MAP:
            samples.append((d,mirror,4,0));samples.extend((d,mirror,walk,i) for i in range(4))
        for d,mirror in DIRECTION_MAP:samples.extend((d,mirror,run,i) for i in range(4))
        for d,mirror,action,phase in samples:
            base=cache[400,action,d];body,cx,cy=base[0 if action==4 else phase*len(base)//4]
            image=Image.new('RGBA',(96,112));bx=(96-body.width)//2;by=107-body.height
            ax,ay=bx+cx,by+body.height+cy
            selected=[(400,0)]+[(b,h) for bit,b,h in LAYERS if mask&bit]
            # The hand/weapon passes behind the body in rear-facing views.
            if mask&1 and d>=3:selected=[(622,0)]+[(b,h) for b,h in selected if b!=622]
            for b,hue in selected:
                seq=cache[b,action,d];im,x,y=seq[0 if action==4 else phase*len(seq)//4]
                if hue:im=hue_image(im,hues,hue)
                image.alpha_composite(im,(ax-x,ay-im.height-y))
            if mirror:image=ImageOps.mirror(image)
            frames.append(image)
        for n,im in enumerate(frames):im.save(folder/f'{n:02d}.png')
        (folder/'equipment.t3s').write_text('--atlas -f rgba5551 -z none\n'+''.join(f'{i:02d}.png\n' for i in range(72)))
        target=(a.out/f'equip_{mask:02d}.t3x').resolve()
        subprocess.run([shutil.which('tex3ds') or '/opt/devkitpro/tools/bin/tex3ds','-i','equipment.t3s','-o',str(target)],cwd=folder,check=True)
        hashes.append(fnv(target.read_bytes()))
        doll=gumps[12].copy()
        for bit,b,hue in LAYERS:
            if mask&bit:
                im=gumps[50000+b]
                if hue:im=hue_image(im,hues,hue)
                doll.alpha_composite(im)
        # All outfits use the same crop and scale, never recenter each layer.
        doll=doll.crop((38,52,144,226));doll.thumbnail((76,104),Image.Resampling.LANCZOS)
        canvas=Image.new('RGBA',(76,104));canvas.alpha_composite(doll,((76-doll.width)//2,104-doll.height));dolls.append(canvas)
        canvas.save(a.out/f'doll_{mask:02d}.png')
    payload=b''.join(d.tobytes() for d in dolls)
    (a.out/'equipment.bin').write_bytes(struct.pack('<4sHHII8I',b'UEQ1',76,104,len(payload),fnv(payload),*hashes)+payload)
    (a.out/'manifest.json').write_text(json.dumps(dict(body=400,frames_per_outfit=72,masks=8,weapon_animation=622,robe_animation=469,boots_animation=477,robe_hue=0x59,boots_hue=0x455,unarmed_actions=[4,0,2],armed_actions=[4,1,3],atlas_checksums=hashes),indent=2)+'\n')
    preview=Image.new('RGBA',(76*8,104),(216,194,148,255))
    for n,im in enumerate(dolls):preview.alpha_composite(im,(n*76,0))
    preview.resize((1216,208),Image.Resampling.NEAREST).save(a.out/'outfits-preview.png')
    print('8 original outfits: idle, walk and run; matching original gumps')
if __name__=='__main__':main()
