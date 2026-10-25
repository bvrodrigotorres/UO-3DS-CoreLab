#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Export the complete private Felucca geometry in independently validated sectors.
Not a playable resource pack: graphics, server decoration and runtime activation
must be integrated separately. Never commit original client data.
"""
import argparse,json,struct,os,time
from pathlib import Path
from export_scene import MapReader,tiledata,find,differences
HEADER=struct.Struct('<4sHH8I3Q')
ENTRY=struct.Struct('<QIIII')
SIDE=64

def fnv(data,h=2166136261):
    for byte in data:h=((h^byte)*16777619)&0xffffffff
    return h

def header(width,height,lands,statics,checksum):
    count=width//64*(height//64);index=64+(lands+statics)*12;data=index+count*ENTRY.size
    return HEADER.pack(b'UOW1',1,SIDE,width,height,width//64,height//64,lands,statics,count,checksum,64,index,data)

class StaticReader:
    def __init__(self,client):
        self.index=find(client,'staidx0.mul').read_bytes()
        if len(self.index)%(12*512):raise ValueError('Invalid map index dimensions')
        self.width=len(self.index)//(12*512)*8;self.height=4096
        if self.width<64 or self.width>7168 or self.width%64:raise ValueError('Unsupported map width')
        self.data=find(client,'statics0.mul').open('rb');self.length=os.fstat(self.data.fileno()).st_size
        self.patches=differences(client,'stadif','stadifi0.mul')
    def records(self,ox,oy):
        result=[]
        for bx in range(ox//8,ox//8+8):
            for by in range(oy//8,oy//8+8):
                block=bx*512+by
                if block in self.patches:raw=self.patches[block]
                else:
                    offset,size,_=struct.unpack_from('<III',self.index,block*12)
                    if offset==0xffffffff or size in (0,0xffffffff):continue
                    if size%7 or offset+size>self.length:raise ValueError('Invalid statics block')
                    self.data.seek(offset);raw=self.data.read(size)
                    if len(raw)!=size:raise ValueError('Truncated statics')
                for art,x,y,z,hue in struct.iter_unpack('<HBBbH',raw):
                    if x>=8 or y>=8:raise ValueError('Invalid static local coordinate')
                    result.append((art,bx*8+x-ox,by*8+y-oy,z,hue))
        return result
    def close(self):self.data.close()

def export(client,out):
    if out.exists():raise FileExistsError('Choose a new output directory')
    out.mkdir(parents=True)
    lands,objects=tiledata(find(client,'tiledata.mul'));objects=objects[:65536]
    if len(lands)>16384 or not objects:raise ValueError('Unsupported tile metadata')
    metadata=b''.join(struct.pack('<QHH',flags,texture,0) for flags,texture in lands)+b''.join(struct.pack('<QBBH',flags,height,0,0) for flags,height in objects)
    source=StaticReader(client);world=MapReader(client);width,height=source.width,source.height
    count=width//64*(height//64);catalogue=[];total=0;maximum=0;seen=set();started=time.monotonic()
    path=out/'britannia.uow';temporary=out/'britannia.uow.tmp'
    try:
        with temporary.open('w+b') as f:
            f.write(header(width,height,len(lands),len(objects),0));f.write(metadata);f.write(bytes(count*ENTRY.size))
            for sx in range(width//64):
                for sy in range(height//64):
                    ox,oy=sx*64,sy*64
                    payload=bytearray()
                    for y in range(oy,oy+65):
                        for x in range(ox,ox+65):
                            art,z=world.get(min(x,width-1),min(y,height-1))
                            if art>=len(lands):raise ValueError('Missing land tile metadata')
                            payload.extend(struct.pack('<Hb',art,z))
                    rows=source.records(ox,oy)
                    for art,x,y,z,hue in rows:
                        if art>=len(objects):raise ValueError('Missing static tile metadata')
                        seen.add(art);payload.extend(struct.pack('<HBBbH',art,x,y,z,hue))
                    if len(rows)>65536 or len(payload)>512*1024:raise ValueError('Sector exceeds streaming budget')
                    catalogue.append(ENTRY.pack(f.tell(),len(payload),fnv(payload),len(rows),total));total+=len(rows);maximum=max(maximum,len(payload));f.write(payload)
                if sx%8==0:print(f'{sx+1}/{width//64} columnas de sectores, {total:,} objetos',flush=True)
            index=b''.join(catalogue);checksum=fnv(index,fnv(metadata));f.seek(64+len(metadata));f.write(index);f.seek(0);f.write(header(width,height,len(lands),len(objects),checksum));f.flush();os.fsync(f.fileno())
        temporary.replace(path)
    finally:world.close();source.close()
    report={'format':'UOW1','width':width,'height':height,'sector_side':64,'sectors':count,'static_objects':total,'static_graphics':len(seen),'maximum_sector_bytes':maximum,'archive_bytes':path.stat().st_size,'catalogue_fnv1a':f'{checksum:08x}','seconds':round(time.monotonic()-started,2),'coverage':'Original map0 terrain, statics and client differences; no visibility filtering','not_included':['GPU atlas integration','ServUO dynamic decoration and spawns','Live game transitions or save migration'],'activated_in_game':False}
    (out/'manifest.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2),flush=True)

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--client',type=Path,required=True);parser.add_argument('--out',type=Path,required=True);args=parser.parse_args();export(args.client.expanduser().resolve(),args.out.resolve())
