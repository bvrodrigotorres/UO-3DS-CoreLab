#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Export private five-channel ServUO combat profiles bound to a scene/population."""
import argparse,json,re,struct
from pathlib import Path
from export_npc_names import class_files
from export_journal import fnv
CHANNELS=('Physical','Fire','Cold','Poison','Energy')
def profile(kind,files):
    chain=[];seen=set()
    while kind.lower() in files and kind.lower() not in seen:
        seen.add(kind.lower());raw=files[kind.lower()].read_text(errors='replace')
        declaration=re.search(r'class\s+'+re.escape(kind)+r'\s*:\s*(\w+)',raw,re.I)
        if not declaration:break
        chain.append(raw[declaration.start():]);kind=declaration[1]
        if kind.lower() in ('basecreature','mobile'):break
    res=[0]*5;damage=[100,0,0,0,0]
    for raw in reversed(chain):
        for c in CHANNELS:
            for match in re.finditer(r'SetResistance\(\s*ResistanceType\.'+c+r'\s*,\s*(\d+)\s*(?:,\s*(\d+)\s*)?\)',raw):
                lo=int(match[1]);hi=int(match[2] or lo);res[CHANNELS.index(c)]=min(100,(lo+hi)//2)
        values={m[1]:int(m[2]) for m in re.finditer(r'SetDamageType\(\s*ResistanceType\.(Physical|Fire|Cold|Poison|Energy)\s*,\s*(\d+)\s*\)',raw)}
        if values:
            damage=[values.get(c,0) for c in CHANNELS];total=sum(damage)
            if not total:damage=[100,0,0,0,0]
            else:
                damage=[v*100//total for v in damage];damage[max(range(5),key=lambda i:damage[i])]+=100-sum(damage)
    return res,damage

def export(requests,reference,scene,population,out):
    files=class_files(reference);records=[];report=[]
    for row in requests:
        res,damage=profile(row['type'],files)
        if row.get('spawner')=='corelab-sewers-boss':res=[40,20,15,40,15]
        records.append(bytes(res+damage));report.append(dict(type=row['type'],resists=res,damage=damage,source_found=row['type'].lower() in files))
    payload=b''.join(records);out.parent.mkdir(parents=True,exist_ok=True)
    out.write_bytes(struct.pack('<4sHHIII',b'UCR1',len(records),0,fnv(scene.read_bytes()),fnv(population.read_bytes()),fnv(payload))+payload)
    out.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n')
    assert all(r['source_found'] for r in report),[r for r in report if not r['source_found']]
    print('Exported',len(records),'bound original-reference combat profiles')
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for key in ('requests','reference','scene','population','out'):p.add_argument('--'+key,type=Path,required=True)
    a=p.parse_args();export(json.loads(a.requests.read_text()),a.reference,a.scene,a.population,a.out)
