#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Export private original UO ASCII font 9 as compact monochrome glyph masks."""
import argparse,struct
from pathlib import Path
from export_journal import fnv

def export(raw):
    at=0;selected=[]
    for font in range(10):
        if at>=len(raw):raise ValueError('Missing font header')
        at+=1
        for char in range(32,256):
            if at+3>len(raw):raise ValueError('Short glyph header')
            width,height,_=raw[at:at+3];at+=3;end=at+width*height*2
            if end>len(raw):raise ValueError('Short glyph pixels')
            if font==9 and char<127:
                pixels=list(struct.unpack_from('<'+str(width*height)+'H',raw,at))
                ink=[(x,y) for y in range(height) for x in range(width) if pixels[y*width+x]]
                selected.append((char,width,height,ink))
            at=end
    allInk=[y for _,_,_,ink in selected for _,y in ink];top=min(allInk)
    advance=[6]*128;masks=[[0]*16 for _ in range(128)]
    for char,width,height,ink in selected:
        if not ink:advance[char]=4;continue
        left=min(x for x,y in ink);right=max(x for x,y in ink);advance[char]=right-left+3
        for x,y in ink:
            x=x-left+1;y=y-top+1
            if x>=15 or y>=15:raise ValueError('Font requires a larger glyph cell')
            masks[char][y]|=1<<(15-x)
    body=bytes(advance)+b''.join(struct.pack('<16H',*rows) for rows in masks)
    return struct.pack('<4sHHII',b'UOF1',1,128,len(body),fnv(body))+body
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--client',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args();a.out.write_bytes(export((a.client/'fonts.mul').read_bytes()));print(a.out,a.out.stat().st_size)
