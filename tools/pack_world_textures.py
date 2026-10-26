#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Combine existing private T3X pages without changing image IDs or artwork."""
from pathlib import Path
import argparse,struct,hashlib,json

def pack(root,out):
    b=(root/'image_sizes.bin').read_bytes();images,count=struct.unpack_from('<HH',b,4)
    assert b[:4]==b'UOI1' and len(b)==8+4*images+4*count
    costs=struct.unpack_from('<'+'I'*count,b,8+4*images)
    out.mkdir(parents=True,exist_ok=True);index=bytearray(struct.pack('<4sI',b'UTP1',count));offset=0
    temp=out/'textures.pack.tmp'
    with temp.open('wb') as f:
        for page,size in enumerate(costs):
            data=(root/f'atlas_{page:02d}.t3x').read_bytes();assert len(data)==size
            checksum=2166136261
            for value in data:checksum=((checksum^value)*16777619)&0xffffffff
            index.extend(struct.pack('<QII',offset,size,checksum));f.write(data);offset+=size
    temp.replace(out/'textures.pack');(out/'textures.idx.tmp').write_bytes(index);(out/'textures.idx.tmp').replace(out/'textures.idx')
    report={'format':'UTP1','pages':count,'bytes':offset,'sha256':hashlib.sha256((out/'textures.pack').read_bytes()).hexdigest(),'index_sha256':hashlib.sha256(index).hexdigest()}
    (out/'textures-report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report),flush=True)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args();pack(a.source,a.out)
