#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Require bounded source geometry to match the already-tested exploration archive."""
import argparse,collections,json,struct,hashlib
from pathlib import Path
from export_scene import MapReader,statics,tiledata,find
from export_world_archive import HEADER,ENTRY,fnv

def audit(client,archive,x,y,side):
    rows=statics(client,x,y,side);actual=collections.Counter((a+x,b+y,z,art,hue) for a,b,z,art,hue in rows)
    expected=collections.Counter();terrain={};terrain_bytes=65*65*3
    with archive.open('rb') as f:
        h=HEADER.unpack(f.read(64))
        if h[:3]!=(b'UOW1',1,64) or x<0 or y<0 or x+side>=h[3] or y+side>=h[4]:raise ValueError('Invalid reference region')
        lands,objects=tiledata(find(client,'tiledata.mul'));objects=objects[:65536]
        metadata=b''.join(struct.pack('<QHH',flags,texture,0) for flags,texture in lands)+b''.join(struct.pack('<QBBH',flags,height,0,0) for flags,height in objects)
        f.seek(64)
        if len(lands)!=h[7] or len(objects)!=h[8] or f.read(len(metadata))!=metadata:raise ValueError('Source tile metadata differs from exploration')
        for sx in range(x//64,(x+side)//64+1):
            for sy in range(y//64,(y+side)//64+1):
                f.seek(h[12]+(sx*h[6]+sy)*ENTRY.size);offset,length,checksum,count,first=ENTRY.unpack(f.read(ENTRY.size));f.seek(offset);raw=f.read(length)
                if len(raw)!=terrain_bytes+count*7 or fnv(raw)!=checksum:raise ValueError('Reference sector checksum/length mismatch')
                for yy in range(max(y,sy*64),min(y+side,sy*64+64)+1):
                    for xx in range(max(x,sx*64),min(x+side,sx*64+64)+1):terrain[(xx,yy)]=struct.unpack_from('<Hb',raw,((yy-sy*64)*65+xx-sx*64)*3)
                for art,xx,yy,z,hue in struct.iter_unpack('<HBBbH',raw[terrain_bytes:]):
                    xx+=sx*64;yy+=sy*64
                    if x<=xx<x+side and y<=yy<y+side:expected[(xx,yy,z,art,hue)]+=1
    reader=MapReader(client)
    try:changed_terrain=sum(reader.get(xx,yy)!=value for (xx,yy),value in terrain.items())
    finally:reader.close()
    missing=expected-actual;extra=actual-expected
    return dict(origin=[x,y],side=side,source_client=str(client),reference_archive=str(archive),original_statics=sum(actual.values()),reference_statics=sum(expected.values()),missing_statics=sum(missing.values()),extra_statics=sum(extra.values()),changed_terrain=changed_terrain,missing_examples=[list(v)+[n] for v,n in list(missing.items())[:12]],source_hashes={name:hashlib.sha256(find(client,name).read_bytes()).hexdigest() for name in ('staidx0.mul','statics0.mul','tiledata.mul')})

def require_match(report):
    if report['missing_statics'] or report['extra_statics'] or report['changed_terrain']:raise ValueError('Source differs from exploration: '+json.dumps({k:report[k] for k in ('missing_statics','extra_statics','changed_terrain')}))

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--client',type=Path,required=True);p.add_argument('--archive',type=Path,required=True);p.add_argument('--x',type=int,required=True);p.add_argument('--y',type=int,required=True);p.add_argument('--side',type=int,required=True);p.add_argument('--out',type=Path);p.add_argument('--report-only',action='store_true');a=p.parse_args()
    report=audit(a.client,a.archive,a.x,a.y,a.side)
    if a.out:a.out.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
    if not a.report_only:require_match(report)
