#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Preserve save identity only for audited door-only scene changes."""
import argparse,collections,struct
from pathlib import Path
from export_scene import HEAD,LAND,OBJECT

def fnv(raw):
    h=2166136261
    for b in raw:h=((h^b)*16777619)&0xffffffff
    return h

def scene(path):
    raw=path.read_bytes();h=HEAD.unpack_from(raw);at=HEAD.size
    vertices=raw[at:at+(h[2]+1)*(h[3]+1)];at+=len(vertices)
    lands=[];objects=[]
    for _ in range(h[2]*h[3]):lands.append(LAND.unpack_from(raw,at));at+=LAND.size
    for _ in range(h[5]):objects.append(OBJECT.unpack_from(raw,at));at+=OBJECT.size
    if at!=len(raw):raise ValueError('Invalid scene')
    return raw,h,vertices,lands,objects

def export(previous,current):
    old=scene(previous);new=scene(current)
    if old[1][2:4]!=new[1][2:4] or old[1][6:8]!=new[1][6:8] or old[2]!=new[2] or [(l[0],l[2:]) for l in old[3]]!=[(l[0],l[2:]) for l in new[3]]:raise ValueError('Terrain differs; migration refused')
    geometry=lambda s:collections.Counter(o[:6]+o[7:8] for o in s[4] if not o[7]&0x20000000)
    if geometry(old)!=geometry(new):raise ValueError('Non-door geometry differs; migration refused')
    raw,h,vertices,lands,objects=old;key=[]
    add=lambda n:key.append(struct.pack('<Q',n&0xffffffffffffffff))
    for n in (h[2],h[3],h[6],h[7]):add(n)
    for z in vertices:add(z if z<128 else z-256)
    for art,image,flags,mode in lands:add(art);add(flags)
    inv=bytearray(struct.pack('<4I',h[6],h[7],h[2],h[3]))
    for x,y,z,height,art,hue,image,flags,order,pad in objects:
        for n in (art,x,y,z,height,flags):add(n)
        inv+=struct.pack('<4I',art,x,y,z&0xffffffff)
    # Existing compatibility aliases remain stable across later door-only updates.
    alias=Path(str(previous)+'.saves')
    pos_id,inv_id=fnv(b''.join(key)),fnv(inv)
    if alias.exists():
        magic,checksum,pos_id,inv_id=struct.unpack('<4sIII',alias.read_bytes())
        if magic!=b'UWC1' or checksum!=fnv(raw):raise ValueError('Invalid previous migration')
    out=Path(str(current)+'.saves');out.write_bytes(struct.pack('<4sIII',b'UWC1',fnv(new[0]),pos_id,inv_id))
    return out
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('previous',type=Path);p.add_argument('current',type=Path);a=p.parse_args();print(export(a.previous,a.current))
