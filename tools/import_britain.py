#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Audit private ServUO placements and door-frame definitions for a bounded region."""
import argparse
import hashlib
import json
import re
from pathlib import Path
from export_scene import statics,tiledata,find
FACING=('WestCW','EastCCW','WestCCW','EastCW','SouthCW','NorthCCW','SouthCCW','NorthCW')
OFFSETS=((-1,1),(1,1),(-1,0),(1,-1),(1,1),(1,-1),(0,0),(0,-1))

# Component art/offsets from pinned ServUO pub57 addon constructors.
ADDONS={
 'StoneOvenEastAddon':[(0x92C,0,0,0),(0x92B,0,1,0)],
 'StoneFireplaceEastAddon':[(0x959,0,0,0),(0x953,0,1,0)],
 'WaterTroughEastAddon':[(0xB41,0,0,0),(0xB42,0,1,0)],
 'WaterTroughSouthAddon':[(0xB43,0,0,0),(0xB44,1,0,0)],
 'AnvilEastAddon':[(0xFAF,0,0,0)],'AnvilSouthAddon':[(0xFB0,0,0,0)],
 'SmallForgeAddon':[(0xFB1,0,0,0)],'SpinningWheelSouthAddon':[(0x1015,0,0,0)],
 'SpinningWheelEastAddon':[(0x1019,0,0,0)],
 'LoomEastAddon':[(0x1060,0,0,0),(0x105F,0,1,0)],
 'LoomSouthAddon':[(0x1061,0,0,0),(0x1062,1,0,0)]}

def placements(text,ox,oy,side):
    result=[];header=None
    for number,line in enumerate(text.splitlines(),1):
        s=line.split('#',1)[0].strip()
        if not s:continue
        m=re.fullmatch(r'(\w+)\s+(0x[\da-fA-F]+|\d+)(?:\s+\((.*)\))?',s)
        if m:header=(m[1],int(m[2],0),m[3] or '');continue
        m=re.fullmatch(r'(-?\d+)\s+(-?\d+)\s+(-?\d+)(?:\s+\((.*)\))?',s)
        if not m or header is None:raise ValueError(f'Unparsed decoration line {number}: {s}')
        x,y,z=map(int,m.group(1,2,3))
        if ox<=x<ox+side and oy<=y<oy+side:
            kind,art,props=header;props+=';'+(m[4] or '')
            if kind.endswith('Addon') and kind not in ADDONS:raise ValueError(f'Addon requires component expansion: {kind}')
            hue=re.search(r'\bHue\s*=\s*(0x[\da-fA-F]+|\d+)',props)
            for graphic,dx,dy,dz in ADDONS.get(kind,[(art,0,0,0)]):
                if not ox<=x+dx<ox+side or not oy<=y+dy<oy+side:raise ValueError('Addon crosses region boundary')
                result.append(dict(x=x+dx-ox,y=y+dy-oy,z=z+dz,art=graphic,hue=int(hue[1],0) if hue else 0,type=kind,props=props,line=number))
    return result

def generate_doors(rows,data,source,ox,oy,side,boundary=None):
    frames={}
    for direction in ('North','South','West','East'):
        m=re.search(r'm_'+direction+r'Frames\s*=.*?\{(.*?)\}',source,re.S)
        if not m:raise ValueError('Missing frame definitions')
        frames[direction]=set(int(n,16) for n in re.findall(r'0x[\da-fA-F]+',m[1]))
    cells={}
    for x,y,z,art,hue in rows:cells.setdefault((x,y,z),set()).add(art)
    def frame(x,y,z,d):return bool(cells.get((x,y,z),set())&frames[d])
    def fits(x,y,z):
        if not 0<=x<side or not 0<=y<side:return False
        for rx,ry,rz,art,hue in rows:
            flags,height=data[art]
            if rx==x and ry==y and flags&0x40 and rz<z+16 and rz+max(1,height)>z:return False
        return True
    result={};pairs=[]
    for x,y,z,art,hue in rows:
        group=[]
        if art in frames['West']:
            if frame(x+2,y,z,'East'):group=[(x+1,y,z,0)]
            elif frame(x+3,y,z,'East'):group=[(x+1,y,z,0),(x+2,y,z,1)]
        elif art in frames['North']:
            if frame(x,y+2,z,'South'):group=[(x,y+1,z,4)]
            elif frame(x,y+3,z,'South'):group=[(x,y+1,z,5),(x,y+2,z,4)]
        if group and all(fits(a,b,c) for a,b,c,f in group):
            if any(not 0<=a+OFFSETS[f][0]<side or not 0<=b+OFFSETS[f][1]<side for a,b,c,f in group):
                if boundary is None:raise ValueError('Open door crosses exported boundary')
                for a,b,c,f in group:boundary.append(dict(x=a,y=b,z=c,art=0x6A5+2*f,hue=0,type='BoundaryDoor'))
                continue
            for a,b,c,f in group:
                closed=0x6A5+2*f;dx,dy=OFFSETS[f]
                if not 0<=a+dx<side or not 0<=b+dy<side:raise ValueError('Open door crosses exported boundary')
                result[(a,b,c)]=dict(x=a,y=b,z=c,art=closed,hue=0,type='DarkWoodDoor',open_art=closed+1,dx=dx,dy=dy,facing=FACING[f])
            if len(group)==2:pairs.append([g[:3] for g in group])
    doors=list(result.values())
    for i,d in enumerate(doors):
        d['pair']=None
        for pair in pairs:
            if (d['x'],d['y'],d['z']) in pair:
                other=next(p for p in pair if p!=(d['x'],d['y'],d['z']))
                d['pair']=next(k for k,t in enumerate(doors) if (t['x'],t['y'],t['z'])==other)
    return doors

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--client',type=Path,required=True);p.add_argument('--reference',type=Path,required=True);p.add_argument('--out',type=Path,required=True);p.add_argument('--x',type=int,default=1488);p.add_argument('--y',type=int,default=1596);p.add_argument('--side',type=int,default=32);args=p.parse_args()
    if args.out.exists():raise FileExistsError(args.out)
    ox,oy,side=args.x,args.y,args.side
    additions=[]
    for name in ('britain.cfg','patches.cfg'):
        for r in placements((args.reference/name).read_text(),ox,oy,side):r['source']=name;additions.append(r)
    signs=[]
    for line in (args.reference/'signs.cfg').read_text().splitlines():
        m=re.match(r'(\d+)\s+(\d+)\s+(-?\d+)\s+(-?\d+)\s+(-?\d+)',line)
        if m:
            facet,art,x,y,z=map(int,m.groups())
            if facet in (0,1) and ox<=x<ox+side and oy<=y<oy+side:signs.append(dict(x=x-ox,y=y-oy,z=z,art=art,hue=0,type='Sign',source='signs.cfg'))
    additions+=signs
    rows=statics(args.client,ox,oy,side);_,data=tiledata(find(args.client,'tiledata.mul'))
    boundary=[]
    doors=generate_doors(rows,data,(args.reference/'DoorGenerator.cs').read_text(),ox,oy,side,boundary)
    additions+=boundary
    existing=set(rows);unique=[];duplicates=0
    for r in additions:
        key=(r['x'],r['y'],r['z'],r['art'],r['hue'])
        if key in existing:duplicates+=1;continue
        existing.add(key);unique.append(r)
    report=dict(origin=[ox,oy],side=side,source='ServUO pub57 Britain / patches / signs / door frames',base_objects=len(rows),decoration_candidates=len(additions),boundary_doors=len(boundary),duplicates=duplicates,placements=unique,doors=doors,source_sha256={f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in args.reference.iterdir() if f.is_file()})
    args.out.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(base=len(rows),decoration=len(unique),doors=len(doors),types=sorted(set(r['type'] for r in unique))),indent=2))
if __name__=='__main__':main()
