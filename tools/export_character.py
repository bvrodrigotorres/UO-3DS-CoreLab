#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Privately export original human customization layers; no client art in Git.

Styles/palettes follow CharacterCreationValues.cs in MobileUO/ClassicUO,
revision cb04dae2a935adfa358cc24df225ca70fea6eb18. Output is sparse RGBA5551
layers, composed once on outfit cache misses, never during normal drawing.
"""
import argparse,json,struct
from pathlib import Path
from functools import lru_cache
from PIL import Image,ImageOps
from export_equipment import layer
from export_avatar import DIRECTION_MAP
from export_journal import decode_gump,fnv
from ultima_sdk.uop import UopFile

HAIR=[700,701,702,703,710,903,900,901,902,712]
BEARD=[800,801,802,803,904,905,906]
CLOTH=[404,546,545,434,435,430,449,480,477,469,622]

def pixels(im):
    return [((r>>3)<<11)|((g>>3)<<6)|((b>>3)<<1)|int(a>=128) if a>=128 else 0 for r,g,b,a in im.getdata()]

def sparse(values):
    result=bytearray();pos=0
    while pos<len(values):
        end=pos
        while end<len(values) and values[end]==0:end+=1
        skip=end-pos;pos=end
        while end<len(values) and values[end]!=0:end+=1
        count=end-pos;result.extend(struct.pack('<HH',skip,count))
        if count:result.extend(struct.pack('<'+'H'*count,*values[pos:end]))
        pos=end
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--combat',action='store_true');p.add_argument('--client',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
    if a.out.exists():raise FileExistsError(a.out)
    a.out.mkdir(parents=True)
    u=UopFile(a.client/'gumpartLegacyMUL.uop','build/gumpartlegacymul/{0:D8}.tga',has_extra=True)
    fallback=[]
    @lru_cache(None)
    def seq(body,act,d):
        try:return layer(a.client,body,act,d)
        except ValueError:
            if body in BEARD:
                fallback.append([body,act,d,"hidden rear-facing beard"]);return [(Image.new("RGBA",(1,1)),0,0)]
            if a.combat and body not in (400,401):
                fallback.append([body,act,d,4]);return layer(a.client,body,4,d)
            # Some beard assets only supply idle/walking, as in the old client.
            alternative=4 if act==4 else 0
            if act==alternative:raise
            fallback.append([body,act,d,alternative]);return layer(a.client,body,alternative,d)
    report=[]
    for sex in range(2):
        body=400+sex
        for graphic in [body]+HAIR+([] if sex else BEARD)+CLOTH:
            samples=[]
            for armed in (False,True):
                walk,run=(1,3) if armed else (0,2)
                for d,mirror in DIRECTION_MAP:
                    samples.append((d,mirror,4,0));samples.extend((d,mirror,walk,i) for i in range(4))
                for d,mirror in DIRECTION_MAP:samples.extend((d,mirror,run,i) for i in range(4))
            if a.combat:
                samples=[]
                for armed in (False,True):
                    attack=9 if armed else 10
                    for d,mirror in DIRECTION_MAP:
                        samples.append((d,mirror,7,0));samples.extend((d,mirror,attack,i) for i in range(4))
                    for d,mirror in DIRECTION_MAP:samples.extend((d,mirror,attack,i) for i in range(4))
            frames=[]
            for d,mirror,act,phase in samples:
                base=seq(body,act,d);b,cx,cy=base[0 if act in (4,7) else (phase*(len(base)-1)//3 if a.combat else phase*len(base)//4)]
                ax=(96-b.width)//2+cx;ay=107+cy
                s=seq(graphic,act,d);im,x,y=s[0 if act in (4,7) else (phase*(len(s)-1)//3 if a.combat else phase*len(s)//4)]
                canvas=Image.new('RGBA',(96,112));canvas.alpha_composite(im,(ax-x,ay-im.height-y))
                if mirror:canvas=ImageOps.mirror(canvas)
                frames.append(sparse(pixels(canvas)))
            if graphic==body:gump=decode_gump(u.read_raw(12+sex))
            else:
                try:gump=decode_gump(u.read_raw((60000 if sex else 50000)+graphic))
                except (ValueError,KeyError,TypeError):gump=decode_gump(u.read_raw(50000+graphic))
            assert gump.size==(260,237), (graphic,gump.size)
            frames.append(sparse(pixels(gump)))
            offsets=[0]
            for f in frames:offsets.append(offsets[-1]+len(f))
            payload=struct.pack('<146I',*offsets)+b''.join(frames)
            header=struct.pack('<4sHHHHII',b'UAP3',sex,graphic,144,0,len(payload),fnv(payload))
            path=a.out/f'{sex}-{graphic}.bin';path.write_bytes(header+payload)
            report.append(dict(file=path.name,bytes=path.stat().st_size,fnv=fnv(header+payload)))
            print(path.name,path.stat().st_size,flush=True)
        seq.cache_clear()
    # Full original palettes, indexed by the original hue number (1..3000).
    raw=(a.client/'hues.mul').read_bytes();colors=[]
    for hue in range(1,3001):
        e=hue-1;at=(e//8)*708+4+(e%8)*88
        colors.extend(struct.unpack_from('<32H',raw,at))
    payload=struct.pack('<'+'H'*len(colors),*colors)
    (a.out/'hues.bin').write_bytes(struct.pack('<4sIII',b'UHP1',3000,len(payload),fnv(payload))+payload)
    (a.out/'manifest.json').write_text(json.dumps(dict(combat=a.combat,combat_actions=[7,9,10] if a.combat else [],race='human',bodies=[400,401],frames=144,canvas=[96,112],paperdoll=[260,237],hair=HAIR,beards=BEARD,files=report,animation_fallback=fallback),indent=2)+'\n')
if __name__=='__main__':main()
