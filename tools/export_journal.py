#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Export private radar colors and the original male paperdoll; no client assets in Git."""
import argparse
import json
from pathlib import Path
import struct
from export_scene import HEAD, LAND, OBJECT, MapReader, find, statics, tiledata

HEADER=struct.Struct('<4sHHiiIIHH')

def fnv(data):
    h=2166136261
    for b in data:
        h=((h^b)*16777619)&0xffffffff
    return h

def rgb(color):
    return ((color>>10&31)*255//31,(color>>5&31)*255//31,(color&31)*255//31)

def decode_gump(raw):
    # GumpsLoader.Decode in ClassicUO: DWORD row offsets from the lookup
    # table start, followed by (color:uint16, run:uint16), zero transparent.
    from PIL import Image
    if len(raw)<8:
        raise ValueError('Short gump')
    width,height=struct.unpack_from('<II',raw)
    if not 0<width<=1024 or not 0<height<=1024 or len(raw)<8+height*4:
        raise ValueError('Invalid gump dimensions')
    image=Image.new('RGBA',(width,height));pixels=image.load()
    for y in range(height):
        pos=8+struct.unpack_from('<I',raw,8+y*4)[0]*4
        if pos<8+height*4:
            raise ValueError('Invalid row offset')
        x=0
        while x<width:
            if pos+4>len(raw):
                raise ValueError('Truncated gump row')
            color,run=struct.unpack_from('<HH',raw,pos);pos+=4
            if run==0 or x+run>width:
                raise ValueError('Invalid gump run')
            rgba=(*rgb(color),255) if color else (0,0,0,0)
            for xx in range(x,x+run):pixels[xx,y]=rgba
            x+=run
    return image

def main():
    from PIL import Image
    from ultima_sdk.uop import UopFile
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--client',type=Path,required=True)
    p.add_argument('--doll-client',type=Path,help='Compatible paperdoll artwork source')
    p.add_argument('--scene',type=Path,required=True)
    p.add_argument('--out',type=Path,required=True)
    args=p.parse_args()
    if args.out.exists():raise FileExistsError('Use a new output directory')
    raw=args.scene.read_bytes();h=HEAD.unpack_from(raw)
    if h[0]!=b'UCL1' or h[1]!=1:raise ValueError('Invalid scene')
    side=max(128,h[2],h[3]);ox=h[6]+h[2]//2-side//2;oy=h[7]+h[3]//2-side//2
    radar=find(args.client,'radarcol.mul').read_bytes()
    def color(index):
        if index*2+2>len(radar):raise ValueError('Missing radar color')
        return rgb(struct.unpack_from('<H',radar,index*2)[0])
    world=MapReader(args.client);image=Image.new('RGB',(side,side));pix=image.load();top=[]
    try:
        for y in range(side):
            for x in range(side):
                art,z=world.get(ox+x,oy+y);pix[x,y]=color(art);top.append(z)
    finally:world.close()
    _,data=tiledata(find(args.client,'tiledata.mul'))
    for x,y,z,art,hue in statics(args.client,ox,oy,side):
        if art>=len(data):raise ValueError('Missing static tiledata')
        flags,height=data[art];i=y*side+x
        if art>1 and z+height>=top[i]:pix[x,y]=color(0x4000+art);top[i]=z+height
    uop=UopFile(find(args.doll_client or args.client,'gumpartLegacyMUL.uop'),'build/gumpartlegacymul/{0:D8}.tga',has_extra=True)
    gump=uop.read_raw(12)
    if not gump:raise ValueError('Missing original human paperdoll gump 12')
    original=decode_gump(gump);bounds=original.getbbox()
    if bounds is None:raise ValueError('Empty paperdoll')
    doll=original.crop(bounds);doll.thumbnail((76,104),Image.Resampling.LANCZOS)
    payload=image.tobytes()+doll.tobytes()
    args.out.mkdir(parents=True)
    (args.out/'journal.bin').write_bytes(HEADER.pack(b'UOJ1',side,side,ox,oy,fnv(raw),fnv(payload),doll.width,doll.height)+payload)
    image.save(args.out/'radar-preview.png');doll.save(args.out/'paperdoll-preview.png')
    start=HEAD.size+(h[2]+1)*(h[3]+1)+h[2]*h[3]*LAND.size
    at=[]
    for i in range(h[5]):
        o=OBJECT.unpack_from(raw,start+i*OBJECT.size)
        if o[:2]==(1501-h[6],1611-h[7]):at.append(dict(z=o[2],height=o[3],art=o[4],flags=hex(o[7])))
    report=dict(map_origin=[ox,oy],map_size=side,paperdoll_gump=12,paperdoll_size=doll.size,scene_fnv=hex(fnv(raw)),floor_inspection_1501_1611=at)
    (args.out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
