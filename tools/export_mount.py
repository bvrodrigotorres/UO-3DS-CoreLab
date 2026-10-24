#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Export private classic horse + mounted human layers, including every cycle frame.

ClassicUO MobileAnimation: human ride 23/24, idle 25, melee 26/unarmed 29;
animal walk/run/stand 0/1/2. Shared MUL anchor, standard horse offset 0.
UAM1 stores 256 sparse 128x128 poses: 128 unarmed, 128 armed. Each bank
contains 8 * (idle + five walk + five gallop) followed by 8 * five attacks.
No proprietary client data is written to the public repository.
"""
import argparse,json,struct
from functools import lru_cache
from pathlib import Path
from PIL import Image,ImageOps
from export_avatar import DIRECTION_MAP
from export_population import read_layer
from export_character import pixels,sparse,HAIR,BEARD
from export_journal import fnv

GRAPHICS=[404,546,545,434,435,430,449,480,477,469,622,618,631,653,544,528,542,538,527]

def samples(armed):
    result=[]
    for d,mirror in DIRECTION_MAP:
        result.append((d,mirror,25,2,0))
        result.extend((d,mirror,23,0,i) for i in range(5))
        result.extend((d,mirror,24,1,i) for i in range(5))
    for d,mirror in DIRECTION_MAP:result.extend((d,mirror,26 if armed else 29,2,i) for i in range(5))
    assert len(result)==128
    return result

def export(client,out):
    out.mkdir(parents=True,exist_ok=True);report=[];fallback=[]
    @lru_cache(None)
    def sequence(body,action,direction):
        try:return read_layer(client,body,action,direction)
        except ValueError:
            if body in BEARD:
                fallback.append([body,action,direction,'hidden unavailable beard'])
                return [(Image.new('RGBA',(1,1)),0,0)]
            if body in (200,400,401):raise
            if action==25:
                fallback.append([body,action,direction,"hidden unavailable rear layer"])
                return [(Image.new("RGBA",(1,1)),0,0)]
            fallback.append([body,action,direction,25])
            return read_layer(client,body,25,direction)
    for sex in range(2):
        for graphic in [400+sex]+HAIR+([] if sex else BEARD)+GRAPHICS+([200] if sex==0 else []):
            frames=[]
            for armed in (False,True):
                for direction,mirror,action,horse_action,phase in samples(armed):
                    act=horse_action if graphic==200 else action
                    seq=sequence(graphic,act,direction)
                    # All five original frames, in order (no 4-frame subsampling).
                    im,cx,cy=seq[min(len(seq)-1,phase*len(seq)//5)]
                    left,top=64-cx,108-im.height-cy
                    bbox=im.getbbox()
                    if bbox and (left+bbox[0]<0 or left+bbox[2]>128 or top+bbox[1]<0 or top+bbox[3]>128):
                        raise ValueError(f'Mounted art clipped: {sex}/{graphic}/{act}/{direction}/{phase}: {(left,top,im.size,bbox)}')
                    canvas=Image.new('RGBA',(128,128));canvas.alpha_composite(im,(left,top))
                    if mirror:canvas=ImageOps.mirror(canvas)
                    frames.append(sparse(pixels(canvas)))
            offsets=[0]
            for frame in frames:offsets.append(offsets[-1]+len(frame))
            payload=struct.pack('<257I',*offsets)+b''.join(frames)
            data=struct.pack('<4sHHHHII',b'UAM1',sex,graphic,256,0,len(payload),fnv(payload))+payload
            path=out/f'{sex}-{graphic}.bin';path.write_bytes(data)
            report.append(dict(file=path.name,bytes=len(data),fnv=fnv(data)))
            print(path.name,len(data),flush=True)
        sequence.cache_clear()
    (out/'manifest.json').write_text(json.dumps(dict(version=1,horse=200,horse_item=0x3e9f,canvas=[128,128],anchor=[64,108],rider_offset=0,frames=256,atlas_frames=128,human_actions=[23,24,25,26,29],horse_actions=[0,1,2],files=report,fallback=fallback),indent=2)+'\n')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--client',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args();export(a.client,a.out)
