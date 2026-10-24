#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Validate every private mounted layer, checksum, pose and sparse pixel bound."""
import argparse,json,struct
from pathlib import Path

def fnv(data):
    h=2166136261
    for b in data:h=((h^b)*16777619)&0xffffffff
    return h

def verify(root):
    manifest=json.loads((root/'manifest.json').read_text())
    assert manifest['canvas']==[128,128] and manifest['anchor']==[64,108] and manifest['frames']==256
    assert len(manifest['files'])==68
    required={(sex,g) for sex in range(2) for g in [400+sex,404,546,545,434,435,430,449,480,477,469,622,618,631,653,544,528,542,538,527]}
    seen=set();poses=0
    for entry in manifest['files']:
        data=(root/entry['file']).read_bytes();assert len(data)==entry['bytes'] and fnv(data)==entry['fnv']
        magic,sex,graphic,count,reserved,size,checksum=struct.unpack_from('<4sHHHHII',data)
        assert magic==b'UAM1' and sex in (0,1) and count==256 and reserved==0 and size==len(data)-20 and checksum==fnv(data[20:])
        assert entry['file']==f'{sex}-{graphic}.bin';seen.add((sex,graphic))
        offsets=struct.unpack_from('<257I',data,20);assert offsets[0]==0 and offsets[-1]==len(data)-1048
        for begin,end in zip(offsets,offsets[1:]):
            assert begin<=end<=len(data)-1048
            pos=1048+begin;limit=1048+end;pixels=0
            while pos<limit:
                assert pos+4<=limit
                skip,n=struct.unpack_from('<HH',data,pos);pos+=4;pixels+=skip+n
                assert pixels<=16384 and (skip or n) and pos+n*2<=limit
                pos+=n*2
            assert pos==limit and pixels==16384
            poses+=1
    assert required<=seen and (0,200) in seen
    return len(seen),poses
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('directory',type=Path);a=p.parse_args();files,poses=verify(a.directory);print(f'Mounted assets verified: {files} files, {poses} poses, all checksums and 128x128 bounds valid')
