#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Private lossless pose streaming from the already exported original-client PNGs."""
import argparse,json,struct
from pathlib import Path
from PIL import Image

def fnv(data):
    h=2166136261
    for b in data:h=((h^b)*16777619)&0xffffffff
    return h

def export(source,out):
    meta=json.loads((source/'manifest.json').read_text())['styles']
    frames=meta[0]['frames'];assert frames in (40,48)
    out.mkdir(parents=True,exist_ok=True)
    manifest=bytearray(struct.pack('<4sHH',b'UPS1',len(meta),frames))
    total=0
    for style,m in enumerate(meta):
        w,h=m['size'];assert 0<w<=128 and 0<h<=144 and m['frames']==frames
        manifest.extend(struct.pack('<hhHHII',*m['pivot'],w,h,m['bytes'],m['checksum']))
        folder=out/'npc-poses'/f'{style:02d}';folder.mkdir(parents=True,exist_ok=True)
        for n in range(frames):
            im=Image.open(source/f'style{style:02d}'/f'{n:02d}.png').convert('RGBA');assert im.size==(w,h)
            values=[((r>>3)<<11)|((g>>3)<<6)|((b>>3)<<1)|1 if a>=128 else 0 for r,g,b,a in im.getdata()]
            raw=struct.pack('<'+'H'*len(values),*values)
            (folder/f'{n:02d}.bin').write_bytes(raw);manifest.extend(struct.pack('<I',fnv(raw)));total+=len(raw)
    (out/'npc-poses.bin').write_bytes(manifest)
    return dict(styles=len(meta),frames=frames,poses=len(meta)*frames,bytes=total,gpu_bytes=4*1024*1024)
if __name__=='__main__':
    a=argparse.ArgumentParser();a.add_argument('source',type=Path);a.add_argument('out',type=Path);v=a.parse_args();print(json.dumps(export(v.source,v.out)))
